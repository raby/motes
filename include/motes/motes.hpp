// motes — small, fast, dependency-free connected-components / blob detection for C++.
//
// Header-only, C++17, zero dependencies. Give it a binary mask; get back the connected
// regions ("motes"), each with a bounding box, pixel area and centroid.
//
// Slice 2: `label` now delegates to `detail::reference_label`, a simple, obviously-correct
// flood-fill labeller. It is both the first real behaviour and the ORACLE that the fast
// two-pass union-find labeller (Slice 3) will be property-tested against (Slice 5).
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

// One connected region ("mote"): a label (1-based, in raster order of first encounter), an
// inclusive bounding box, the pixel area, and the area-weighted centroid.
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

namespace detail {

// Reference labeller: an iterative flood fill, one region at a time. Simple and obviously
// correct — the oracle for testing the fast path, and the current implementation of `label`.
// Assumes `mask.width * mask.height` fits in an int (true for any realistic image).
inline std::vector<Blob> reference_label(const Mask& mask, Connectivity conn) {
  std::vector<Blob> blobs;
  if (mask.data == nullptr || mask.width <= 0 || mask.height <= 0) return blobs;

  const int w = mask.width;
  const int h = mask.height;
  // Neighbour offsets: [0,4) are the orthogonal (4-connected) directions, [4,8) the diagonals.
  static constexpr int dx8[] = {1, -1, 0, 0, 1, 1, -1, -1};
  static constexpr int dy8[] = {0, 0, 1, -1, 1, -1, 1, -1};
  const int ndir = (conn == Connectivity::Eight) ? 8 : 4;

  std::vector<int> labels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);
  std::vector<int> stack; // pixel indices (y*w + x)
  int next_label = 0;

  for (int sy = 0; sy < h; ++sy) {
    for (int sx = 0; sx < w; ++sx) {
      const std::size_t seed = static_cast<std::size_t>(sy) * static_cast<std::size_t>(w) + sx;
      if (!mask.foreground(sx, sy) || labels[seed] != 0) continue;

      ++next_label;
      Blob b;
      b.label = next_label;
      b.min_x = b.max_x = sx;
      b.min_y = b.max_y = sy;
      double sum_x = 0.0;
      double sum_y = 0.0;
      int area = 0;

      labels[seed] = next_label;
      stack.clear();
      stack.push_back(static_cast<int>(seed));

      while (!stack.empty()) {
        const int p = stack.back();
        stack.pop_back();
        const int px = p % w;
        const int py = p / w;

        ++area;
        sum_x += px;
        sum_y += py;
        if (px < b.min_x) b.min_x = px;
        if (px > b.max_x) b.max_x = px;
        if (py < b.min_y) b.min_y = py;
        if (py > b.max_y) b.max_y = py;

        for (int d = 0; d < ndir; ++d) {
          const int nx = px + dx8[d];
          const int ny = py + dy8[d];
          if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
          const std::size_t n = static_cast<std::size_t>(ny) * static_cast<std::size_t>(w) + nx;
          if (labels[n] != 0 || !mask.foreground(nx, ny)) continue;
          labels[n] = next_label;
          stack.push_back(static_cast<int>(n));
        }
      }

      b.area = area;
      b.centroid_x = sum_x / static_cast<double>(area);
      b.centroid_y = sum_y / static_cast<double>(area);
      blobs.push_back(b);
    }
  }
  return blobs;
}

} // namespace detail

// Label the foreground of `mask` into connected blobs, one `Blob` per region.
inline std::vector<Blob> label(const Mask& mask, Connectivity conn = Connectivity::Eight) {
  return detail::reference_label(mask, conn);
}

} // namespace motes

#endif // MOTES_MOTES_HPP
