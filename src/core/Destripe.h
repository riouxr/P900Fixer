#pragma once
// Removes the Sony P900's fixed column pattern: every even column brighter
// than the odd column beside it by a nearly constant offset (an exact
// 2-pixel stripe at the horizontal Nyquist frequency).

namespace p900 {

struct DestripeResult {
    float medianOffset = 0.0f;  // half the even/odd difference removed
};

// plane: width x height floats, row-major. firstX: the absolute x of
// column 0 (the data window's min x), so the even/odd phase follows the
// sensor, not the crop. block: estimation block size in rows and column pairs.
DestripeResult destripe(float* plane, int width, int height, int firstX, int block = 24);

}  // namespace p900
