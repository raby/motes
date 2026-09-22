// motes — small, fast, dependency-free connected-components / blob detection for C++.
//
// Header-only, C++17, zero dependencies. Give it a binary mask; get back the connected
// regions ("motes"), each with a bounding box, pixel area and centroid.
//
// Slice 3: `label` now uses `detail::union_find_label`, a two-pass connected-components labeller
// (the fast path). `detail::reference_label` (flood fill) is kept as the obviously-correct oracle
// the fast path is checked against here, and property-tested against over random images (Slice 5).
#ifndef MOTES_MOTES_HPP
#define MOTES_MOTES_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
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

// Disjoint-set (union-find) over provisional labels, with path halving and union by size.
class UnionFind {
 public:
  // Add a new singleton set; returns its id.
  int make() {
    parent_.push_back(static_cast<int>(parent_.size()));
    size_.push_back(1);
    return static_cast<int>(parent_.size()) - 1;
  }
  int count() const { return static_cast<int>(parent_.size()); }
  int find(int x) {
    while (parent_[static_cast<std::size_t>(x)] != x) {
      parent_[static_cast<std::size_t>(x)] =
          parent_[static_cast<std::size_t>(parent_[static_cast<std::size_t>(x)])]; // path halving
      x = parent_[static_cast<std::size_t>(x)];
    }
    return x;
  }
  void unite(int a, int b) {
    a = find(a);
    b = find(b);
    if (a == b) return;
    if (size_[static_cast<std::size_t>(a)] < size_[static_cast<std::size_t>(b)]) std::swap(a, b);
    parent_[static_cast<std::size_t>(b)] = a;
    size_[static_cast<std::size_t>(a)] += size_[static_cast<std::size_t>(b)];
  }

 private:
  std::vector<int> parent_;
  std::vector<int> size_;
};

// Fast path: classic two-pass connected-components labelling. Pass one assigns provisional labels
// and unions equivalences from already-visited neighbours; pass two resolves each pixel to its
// set's root, maps roots to final 1-based labels in raster order, and accumulates blob stats.
// Produces the same blobs as `reference_label`. Assumes width*height fits in an int.
inline std::vector<Blob> union_find_label(const Mask& mask, Connectivity conn) {
  std::vector<Blob> blobs;
  if (mask.data == nullptr || mask.width <= 0 || mask.height <= 0) return blobs;

  const int w = mask.width;
  const int h = mask.height;
  const bool eight = (conn == Connectivity::Eight);
  const auto at = [w](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x); };

  UnionFind uf;
  uf.make(); // id 0 is the "no label" sentinel; provisional labels start at 1
  std::vector<int> prov(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);

  // Pass one: provisional labels + equivalences from prior neighbours (W, N, and NW, NE for 8-conn).
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (!mask.foreground(x, y)) continue;
      int best = 0;
      const auto consider = [&](int nx, int ny) {
        if (nx < 0 || nx >= w || ny < 0 || ny >= h) return;
        const int lab = prov[at(nx, ny)];
        if (lab == 0) return;
        if (best == 0) {
          best = lab;
        } else {
          uf.unite(best, lab);
          best = uf.find(best);
        }
      };
      consider(x - 1, y); // W
      consider(x, y - 1); // N
      if (eight) {
        consider(x - 1, y - 1); // NW
        consider(x + 1, y - 1); // NE
      }
      if (best == 0) best = uf.make();
      prov[at(x, y)] = best;
    }
  }

  // Pass two: resolve roots -> final 1-based labels (raster order), accumulate stats.
  std::vector<int> final_of_root(static_cast<std::size_t>(uf.count()), 0);
  int next_final = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int pl = prov[at(x, y)];
      if (pl == 0) continue;
      const int root = uf.find(pl);
      int fl = final_of_root[static_cast<std::size_t>(root)];
      if (fl == 0) {
        fl = ++next_final;
        final_of_root[static_cast<std::size_t>(root)] = fl;
        Blob nb;
        nb.label = fl;
        nb.min_x = nb.max_x = x;
        nb.min_y = nb.max_y = y;
        blobs.push_back(nb);
      }
      Blob& b = blobs[static_cast<std::size_t>(fl) - 1];
      ++b.area;
      b.centroid_x += x; // running sum, divided out below
      b.centroid_y += y;
      if (x < b.min_x) b.min_x = x;
      if (x > b.max_x) b.max_x = x;
      if (y < b.min_y) b.min_y = y;
      if (y > b.max_y) b.max_y = y;
    }
  }
  for (Blob& b : blobs) {
    b.centroid_x /= static_cast<double>(b.area);
    b.centroid_y /= static_cast<double>(b.area);
  }
  return blobs;
}

} // namespace detail

// Label the foreground of `mask` into connected blobs, one `Blob` per region.
inline std::vector<Blob> label(const Mask& mask, Connectivity conn = Connectivity::Eight) {
  return detail::union_find_label(mask, conn);
}

} // namespace motes

#endif // MOTES_MOTES_HPP
