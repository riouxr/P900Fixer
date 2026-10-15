// Synthetic check: a gradient with vertical bars plus a 2-pixel column
// stripe comes back as the gradient with its bars. Exit code 0 on success.
#include "core/Destripe.h"

#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    int failures = 0;
    for (int firstX : {0, 1, -3}) {
        const int w = 400, h = 240;
        std::vector<float> clean(w * h), img(w * h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                float v = 0.05f + 0.3f * x / w + 0.1f * y / h;
                if (x % 50 < 3) v *= 0.3f;  // railings: sharp vertical bars
                clean[y * w + x] = v;
                bool even = ((firstX + x) % 2 + 2) % 2 == 0;
                img[y * w + x] = v + (even ? 0.009f : -0.009f);
            }
        p900::DestripeResult r = p900::destripe(img.data(), w, h, firstX);
        double maxErr = 0, sum = 0;
        for (int i = 0; i < w * h; ++i) {
            double e = std::abs(img[i] - clean[i]);
            maxErr = std::max(maxErr, e);
            sum += e;
        }
        double mean = sum / (w * h);
        bool ok = std::abs(r.medianOffset - 0.009f) < 5e-4 && mean < 5e-4 && maxErr < 3e-3;
        std::printf("firstX %d: offset %.5f, mean error %.6f, max error %.6f %s\n", firstX, r.medianOffset, mean,
                    maxErr, ok ? "ok" : "FAILED");
        failures += !ok;
    }
    return failures;
}
