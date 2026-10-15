#include "ExrFix.h"

#include "Destripe.h"

#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <set>

namespace fs = std::filesystem;

namespace p900 {
namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

// Colour data gets fixed; mattes, depth and ids keep their values.
bool isImageChannel(const std::string& name) {
    std::string base = lower(name.substr(name.rfind('.') == std::string::npos ? 0 : name.rfind('.') + 1));
    static const std::set<std::string> skip = {"a", "alpha", "z", "depth", "id", "objectid", "materialid"};
    return skip.count(base) == 0;
}

// OpenEXR takes UTF-8 paths on Windows.
std::string utf8(const fs::path& p) {
    std::u8string s = p.u8string();
    return {s.begin(), s.end()};
}

struct Plane {
    std::string name;
    Imf::PixelType type;
    int xSampling, ySampling, width, height;
    std::vector<char> raw;  // in the file's own pixel type, read and written as is

    size_t pixelSize() const { return type == Imf::HALF ? sizeof(half) : 4; }
    Imf::Slice slice(const Imath::Box2i& dw) {
        const std::ptrdiff_t origin =
            (std::ptrdiff_t)(dw.min.y / ySampling) * width + dw.min.x / xSampling;
        return Imf::Slice(type, raw.data() - origin * (std::ptrdiff_t)pixelSize(), pixelSize(),
                          pixelSize() * width, xSampling, ySampling);
    }
    std::vector<float> toFloat() const {
        std::vector<float> f((size_t)width * height);
        if (type == Imf::HALF) {
            const half* h = (const half*)raw.data();
            for (size_t i = 0; i < f.size(); ++i) f[i] = h[i];
        } else {
            std::memcpy(f.data(), raw.data(), f.size() * sizeof(float));
        }
        return f;
    }
    void fromFloat(const std::vector<float>& f) {
        if (type == Imf::HALF) {
            half* h = (half*)raw.data();
            for (size_t i = 0; i < f.size(); ++i) h[i] = half(f[i]);
        } else {
            std::memcpy(raw.data(), f.data(), f.size() * sizeof(float));
        }
    }
};

}  // namespace

fs::path cleanedUpFolder(const fs::path& input) { return input.parent_path() / "CleanedUp"; }

FixResult fixExr(const fs::path& input) {
    FixResult result;
    try {
        Imf::InputFile in(utf8(input).c_str());
        Imf::Header header = in.header();
        if (header.hasType() && (header.type() == "deepscanline" || header.type() == "deeptile")) {
            result.message = "deep EXR, not supported";
            return result;
        }
        const Imath::Box2i dw = header.dataWindow();
        const int width = dw.max.x - dw.min.x + 1, height = dw.max.y - dw.min.y + 1;

        std::vector<Plane> planes;
        Imf::FrameBuffer fb;
        for (auto it = header.channels().begin(); it != header.channels().end(); ++it) {
            const Imf::Channel& ch = it.channel();
            Plane p{it.name(), ch.type, ch.xSampling, ch.ySampling, width / ch.xSampling, height / ch.ySampling, {}};
            p.raw.assign((size_t)p.width * p.height * p.pixelSize(), 0);
            planes.push_back(std::move(p));
        }
        for (Plane& p : planes) fb.insert(p.name, p.slice(dw));
        in.setFrameBuffer(fb);
        in.readPixels(dw.min.y, dw.max.y);

        int fixed = 0;
        float offset = 0;
        for (Plane& p : planes) {
            if (!isImageChannel(p.name) || p.type == Imf::UINT || p.xSampling != 1 || p.ySampling != 1) continue;
            std::vector<float> pixels = p.toFloat();
            DestripeResult r = destripe(pixels.data(), p.width, p.height, dw.min.x);
            p.fromFloat(pixels);
            offset = std::max(offset, std::abs(r.medianOffset));
            ++fixed;
        }
        if (fixed == 0) {
            result.message = "no colour channels to fix";
            return result;
        }

        fs::path folder = cleanedUpFolder(input);
        fs::create_directories(folder);
        result.output = folder / input.filename();
        if (fs::exists(result.output) && fs::equivalent(result.output, input)) {
            result.message = "output would overwrite the input";
            return result;
        }
        // Written as scanlines; tiling isn't needed for frames.
        if (header.hasTileDescription()) header.erase("tiles");
        // Write beside the target, then rename, so a crash never leaves half a frame.
        fs::path temp = result.output;
        temp += ".part";
        try {
            Imf::OutputFile out(utf8(temp).c_str(), header);
            out.setFrameBuffer(fb);
            out.writePixels(height);
        } catch (...) {
            std::error_code ec;
            fs::remove(temp, ec);
            throw;
        }
        fs::rename(temp, result.output);

        char text[96];
        std::snprintf(text, sizeof text, "%d channel%s, stripe %.4f removed", fixed, fixed == 1 ? "" : "s",
                      2.0f * offset);
        result.message = text;
        result.ok = true;
    } catch (const std::exception& e) {
        result.message = e.what();
    }
    return result;
}

std::vector<fs::path> collectFrames(const std::vector<fs::path>& dropped) {
    std::set<fs::path> frames;
    auto isExr = [](const fs::path& p) { return lower(p.extension().string()) == ".exr"; };
    for (const fs::path& path : dropped) {
        std::error_code ec;
        if (fs::is_directory(path, ec)) {
            if (lower(path.filename().string()) == "cleanedup") continue;
            for (const auto& entry : fs::directory_iterator(path, ec))
                if (entry.is_regular_file() && isExr(entry.path())) frames.insert(entry.path());
        } else if (fs::is_regular_file(path, ec) && isExr(path)) {
            frames.insert(path);
        }
    }
    return {frames.begin(), frames.end()};
}

}  // namespace p900
