#include "Destripe.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace p900 {
namespace {

// Splits n items into roughly equal blocks of about `size`; returns the edges.
std::vector<int> blockEdges(int n, int size) {
    int count = std::max(1, (int)std::lround((double)n / size));
    std::vector<int> edges(count + 1);
    for (int k = 0; k <= count; ++k) edges[k] = (int)((long long)k * n / count);
    return edges;
}

// Linear interpolation between block centres, clamped at the ends.
struct Axis {
    std::vector<int> index0, index1;
    std::vector<float> t;
    Axis(const std::vector<int>& edges, int n) : index0(n), index1(n), t(n) {
        int count = (int)edges.size() - 1;
        std::vector<float> centre(count);
        for (int k = 0; k < count; ++k) centre[k] = 0.5f * (edges[k] + edges[k + 1] - 1);
        int k = 0;
        for (int p = 0; p < n; ++p) {
            while (k + 1 < count - 1 && centre[k + 1] < p) ++k;
            if (count == 1 || p <= centre[0]) {
                index0[p] = index1[p] = 0; t[p] = 0;
            } else if (p >= centre[count - 1]) {
                index0[p] = index1[p] = count - 1; t[p] = 0;
            } else {
                index0[p] = k; index1[p] = k + 1;
                t[p] = (p - centre[k]) / (centre[k + 1] - centre[k]);
            }
        }
    }
};

}  // namespace

DestripeResult destripe(float* plane, int width, int height, int firstX, int block) {
    DestripeResult result;
    const int first = (firstX % 2 == 0) ? 0 : 1;  // column of the first even x
    const int pairs = (width - first) / 2;
    if (pairs < 2 || height < 1) return result;

    // Per pair: the stripe plus content's second difference (slopes cancel),
    // ((v[e]-v[o]) + (v[e+2]-v[o])) / 4.
    std::vector<float> d((size_t)pairs * height);
    for (int y = 0; y < height; ++y) {
        const float* row = plane + (size_t)y * width;
        float* out = d.data() + (size_t)y * pairs;
        for (int k = 0; k < pairs; ++k) {
            int e = first + 2 * k, o = e + 1;
            float d1 = 0.5f * (row[e] - row[o]);
            float d2 = (e + 2 < width) ? 0.5f * (row[e + 2] - row[o]) : d1;
            out[k] = 0.5f * (d1 + d2);
        }
    }

    // Median per block: real vertical edges (railings) and fine texture
    // are outliers there, the stripe is everywhere.
    std::vector<int> rowEdges = blockEdges(height, block), pairEdges = blockEdges(pairs, block);
    int bh = (int)rowEdges.size() - 1, bw = (int)pairEdges.size() - 1;
    std::vector<float> blocks((size_t)bh * bw), scratch;
    for (int by = 0; by < bh; ++by)
        for (int bx = 0; bx < bw; ++bx) {
            scratch.clear();
            for (int y = rowEdges[by]; y < rowEdges[by + 1]; ++y)
                for (int k = pairEdges[bx]; k < pairEdges[bx + 1]; ++k)
                    if (std::isfinite(d[(size_t)y * pairs + k])) scratch.push_back(d[(size_t)y * pairs + k]);
            float m = 0;
            if (!scratch.empty()) {
                auto mid = scratch.begin() + scratch.size() / 2;
                std::nth_element(scratch.begin(), mid, scratch.end());
                m = *mid;
            }
            blocks[(size_t)by * bw + bx] = m;
        }
    scratch.assign(blocks.begin(), blocks.end());
    std::nth_element(scratch.begin(), scratch.begin() + scratch.size() / 2, scratch.end());
    result.medianOffset = scratch[scratch.size() / 2];

    // Smooth offset map; even columns lose it, odd columns gain it.
    Axis ay(rowEdges, height), ax(pairEdges, pairs);
    for (int y = 0; y < height; ++y) {
        float* row = plane + (size_t)y * width;
        const float* b0 = blocks.data() + (size_t)ay.index0[y] * bw;
        const float* b1 = blocks.data() + (size_t)ay.index1[y] * bw;
        float ty = ay.t[y];
        for (int i = 0; i < width; ++i) {
            int k = std::clamp((i - first) / 2 - ((i - first) < 0 ? 1 : 0), 0, pairs - 1);
            float tx = ax.t[k];
            int x0 = ax.index0[k], x1 = ax.index1[k];
            float top = b0[x0] + (b0[x1] - b0[x0]) * tx;
            float bottom = b1[x0] + (b1[x1] - b1[x0]) * tx;
            float s = top + (bottom - top) * ty;
            bool even = ((firstX + i) % 2 + 2) % 2 == 0;
            row[i] += even ? -s : s;
        }
    }
    return result;
}

}  // namespace p900
