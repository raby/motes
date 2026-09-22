// motes — small, fast, dependency-free connected-components / blob detection for C++.
//
// Header-only, C++17, zero dependencies. Give it a binary mask; get back the connected
// regions ("motes"), each with a bounding box, pixel area and centroid.
//
// Slice 1 (scaffold): the public API and value types are fixed here; `label` is a stub that
// returns no blobs. The two-pass union-find labeller lands in Slice 3, blob stats in Slice 4.
#ifndef MOTES_MOTES_HPP
#define MOTES_MOTES_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace motes {

// Pixel connectivity used when deciding whether two foreground pixels belong to the same blob.
enum class Connectivity { Four = 4, Eight = 8 };

// A read-only view over a binary mask: row-major, `width * height` bytes, any non-zero byte is
// foreground. `motes` never owns, copies or frees the pixels — the caller keeps them alive.
struct Mask {
  const std::uint8_t* data = nullptr;
  int width = 0;
  int height = 0;

  // Is the pixel at (x, y) foreground? No bounds check — callers stay within [0,width)×[0,height).
  bool foreground(int x, int y) const {
    return data[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x)] != 0;
  }
};

// One connected region ("mote"): a label (1-based), an inclusive bounding box, the pixel area,
// and the area-weighted centroid.
struct Blob {
  int label = 0;
  int min_x = 0;
  int min_y = 0;
  int max_x = 0;
  int max_y = 0;
  int area = 0;
  double centroid_x = 0.0;
  double centroid_y = 0.0;
};

// Label the foreground of `mask` into connected blobs, one `Blob` per region.
//
// Slice 1 STUB: returns no blobs. The real union-find labeller arrives in Slice 3.
inline std::vector<Blob> label(const Mask& mask, Connectivity conn = Connectivity::Eight) {
  (void)mask;
  (void)conn;
  return {};
}

} // namespace motes

#endif // MOTES_MOTES_HPP
