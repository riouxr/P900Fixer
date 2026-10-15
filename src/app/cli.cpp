// Command-line version: p900fix <frames or folders>... (same output as the app).
#include "core/ExrFix.h"

#include <cstdio>

int wmain(int argc, wchar_t** argv) {
    std::vector<std::filesystem::path> paths(argv + 1, argv + argc);
    int failed = 0;
    for (const auto& frame : p900::collectFrames(paths)) {
        p900::FixResult r = p900::fixExr(frame);
        std::printf("%s %s: %s\n", r.ok ? "OK " : "ERR", frame.filename().string().c_str(), r.message.c_str());
        std::fflush(stdout);
        failed += !r.ok;
    }
    return failed ? 1 : 0;
}
