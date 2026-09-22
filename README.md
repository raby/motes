# motes

> Small, fast, dependency-free **connected-components / blob detection** for C++ — and, soon, the
> browser via WebAssembly.

`motes` finds the connected regions ("motes") in a binary mask and returns each as a blob with a
bounding box, pixel area and centroid. No OpenCV, no dependencies — one header for C++, and (later)
an npm package for the web.

> **Status: early build.** Slice 1 — scaffold + public API. The union-find labeller, blob stats,
> a brute-force property test, a benchmark, the WebAssembly build and a live webcam demo are on the
> way (see [Roadmap](#roadmap)).

## 30-second example

```cpp
#include <motes/motes.hpp>

// `mask` is width*height bytes, row-major; any non-zero byte is foreground.
motes::Mask mask{pixels.data(), width, height};

std::vector<motes::Blob> blobs = motes::label(mask, motes::Connectivity::Eight);
for (const motes::Blob& b : blobs) {
    // b.label, b.min_x..b.max_y, b.area, b.centroid_x, b.centroid_y
}
```

## Why

Detecting the blobs in a thresholded image is a small, classic computer-vision job — object
counting, particle analysis, OCR pre-processing, motion blobs from a camera. The usual answer is to
pull in all of OpenCV. `motes` is the small alternative: a single header, provably correct, fast, and
portable enough to run in a browser tab.

## Design

- **Header-only C++17**, zero dependencies.
- Two-pass **union-find** connected-components labelling, 4- or 8-connectivity.
- Correctness pinned by a **property test** against a brute-force reference.
- A **JMH-style benchmark** with published numbers.
- Compiles to **WebAssembly** (Emscripten) and ships on **npm** for the web.

## Build & test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Header-only, so to *use* it you only need the `include/` directory (or add this repo via CMake
`FetchContent` and link `motes::motes`).

## Roadmap

Built in small, reviewed slices: C++ core (labeller → blob stats → property test → benchmark) →
WebAssembly + npm → a live webcam demo. A full write-up will land as a case study on
[digitalbluebird.com](https://digitalbluebird.com).

## Licence

[MIT](LICENSE) © Raby Whyte
