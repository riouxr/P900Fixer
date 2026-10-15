# P900 Fixer

Removes the vertical column lines from Sony P900 footage exported as EXR frames.

## Use

Drop EXR frames or folders of frames on the window (or on `P900Fixer.exe`'s icon).
Each fixed frame goes to a `CleanedUp` folder beside the original, under the same
name, header (timecode, metadata) and compression. Originals are never touched.

`p900fix.exe` does the same from the command line: `p900fix <frames or folders>...`.

## What it fixes

The sensor's columns alternate in level: every even column is brighter than the
odd column next to it by about 0.017 (an exact 2-pixel stripe). The offset is
fixed: the same in every frame, nearly the same across the frame, in R, G and B
alike, a little smaller in highlights. A denoiser doesn't remove it because it
isn't random.

For each colour channel (`src/core/Destripe.cpp`):
1. per pair of columns, half the even/odd difference, averaged with the next
   pair's so real gradients cancel out;
2. the median of that over 24 x 24 blocks, so real vertical edges (railings) and
   fine texture don't count;
3. that map, smoothed between block centres, is subtracted from even columns and
   added to odd ones.

Only the stripe goes; nothing is blurred. Alpha, depth and id channels are left as
they are. The even/odd phase follows the absolute x of the data window.

## Build

Needs Visual Studio's C++ tools, vcpkg at `C:\vcpkg` and Qt 6.8 at
`C:\Qt\6.8.3\msvc2022_64` (override with `VCPKG_ROOT` / `QT_DIR`).

```
build.cmd
```

Configures into `C:\b\p900`, builds Release, runs the tests and copies the app
with its DLLs to `dist\`. The MSVC runtime isn't bundled (it's present wherever
Visual Studio or most apps are installed).
