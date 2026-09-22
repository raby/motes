#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "motes/motes.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <random>
#include <string>
#include <vector>

using namespace motes;

namespace {

// A mask built from ASCII rows: '#' is foreground, anything else is background. Keeps its own
// pixel buffer, so hold the object alive while the Mask view is in use.
struct AsciiMask {
  std::vector<std::uint8_t> pixels;
  int width = 0;
  int height = 0;
  Mask mask() const { return Mask{pixels.data(), width, height}; }
};

AsciiMask make(std::initializer_list<const char*> rows) {
  AsciiMask m;
  m.height = static_cast<int>(rows.size());
  m.width = (m.height > 0) ? static_cast<int>(std::strlen(*rows.begin())) : 0;
  m.pixels.reserve(static_cast<std::size_t>(m.width) * static_cast<std::size_t>(m.height));
  for (const char* row : rows) {
    for (int x = 0; x < m.width; ++x) {
      m.pixels.push_back(row[x] == '#' ? std::uint8_t{255} : std::uint8_t{0});
    }
  }
  return m;
}

// Field-wise blob comparison (centroids within a tiny epsilon).
bool same_blobs(const std::vector<Blob>& a, const std::vector<Blob>& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i].label != b[i].label) return false;
    if (a[i].min_x != b[i].min_x || a[i].max_x != b[i].max_x) return false;
    if (a[i].min_y != b[i].min_y || a[i].max_y != b[i].max_y) return false;
    if (a[i].area != b[i].area) return false;
    if (std::abs(a[i].centroid_x - b[i].centroid_x) > 1e-9) return false;
    if (std::abs(a[i].centroid_y - b[i].centroid_y) > 1e-9) return false;
  }
  return true;
}

} // namespace

TEST_CASE("an empty mask has no blobs") {
  std::vector<std::uint8_t> pixels; // 0x0
  const Mask mask{pixels.data(), 0, 0};
  CHECK(label(mask).empty());
}

TEST_CASE("an all-background mask has no blobs") {
  std::vector<std::uint8_t> pixels(4 * 3, 0); // 4x3, all zero
  const Mask mask{pixels.data(), 4, 3};
  CHECK(label(mask, Connectivity::Four).empty());
  CHECK(label(mask, Connectivity::Eight).empty());
}

TEST_CASE("Mask::foreground reads the row-major buffer") {
  std::vector<std::uint8_t> pixels(4 * 3, 0);
  pixels[1 * 4 + 2] = 255; // (2, 1) lit
  const Mask mask{pixels.data(), 4, 3};
  CHECK(mask.foreground(2, 1));
  CHECK_FALSE(mask.foreground(0, 0));
  CHECK_FALSE(mask.foreground(3, 2));
}

TEST_CASE("a single filled rectangle is one blob, with the right stats") {
  const AsciiMask m = make({
      "......",
      ".###..",
      ".###..",
      "......",
  });
  const auto blobs = label(m.mask());
  REQUIRE(blobs.size() == 1);
  const Blob& b = blobs[0];
  CHECK(b.area == 6);
  CHECK(b.min_x == 1);
  CHECK(b.max_x == 3);
  CHECK(b.min_y == 1);
  CHECK(b.max_y == 2);
  CHECK(b.centroid_x == doctest::Approx(2.0));
  CHECK(b.centroid_y == doctest::Approx(1.5));
}

TEST_CASE("separate regions are separate blobs") {
  const AsciiMask m = make({
      "#..#",
      "#..#",
      "....",
      ".##.",
  });
  CHECK(label(m.mask(), Connectivity::Four).size() == 3); // left bar, right bar, bottom pair
}

TEST_CASE("a diagonal touch joins under 8-connectivity and splits under 4-connectivity") {
  const AsciiMask m = make({
      "#.",
      ".#",
  });
  CHECK(label(m.mask(), Connectivity::Eight).size() == 1);
  CHECK(label(m.mask(), Connectivity::Four).size() == 2);
}

TEST_CASE("a ring is a single blob — a hole does not split the foreground") {
  const AsciiMask m = make({
      "#####",
      "#...#",
      "#...#",
      "#####",
  });
  const auto blobs = label(m.mask());
  REQUIRE(blobs.size() == 1);
  const Blob& b = blobs[0];
  CHECK(b.area == 14); // 20 pixels minus a 3x2 hole
  CHECK(b.min_x == 0);
  CHECK(b.max_x == 4);
  CHECK(b.min_y == 0);
  CHECK(b.max_y == 3);
}

TEST_CASE("blobs are labelled 1-based in raster order of first encounter") {
  const AsciiMask m = make({
      "#.#",
      "...",
      "#..",
  });
  const auto blobs = label(m.mask(), Connectivity::Four);
  REQUIRE(blobs.size() == 3);
  CHECK(blobs[0].label == 1); // top-left
  CHECK(blobs[1].label == 2); // top-right
  CHECK(blobs[2].label == 3); // bottom-left
  CHECK(blobs[0].min_x == 0);
  CHECK(blobs[1].min_x == 2);
  CHECK(blobs[2].min_y == 2);
}

TEST_CASE("the union-find labeller matches the reference oracle") {
  const AsciiMask masks[] = {
      make({".###.", ".#.#.", ".###."}),                   // ring
      make({"#.#", "#.#", "###"}),                         // U: two labels merge at the bottom
      make({"#..#", "#..#", "....", ".##."}),              // separate regions
      make({"#.", ".#"}),                                  // diagonal touch
      make({"#####", "#...#", "#.#.#", "#...#", "#####"}), // frame + an isolated speck
      make({".##..##.", ".##..##.", "........", "..####..", "..####.."}),
  };
  for (const AsciiMask& m : masks) {
    for (const Connectivity conn : {Connectivity::Four, Connectivity::Eight}) {
      CAPTURE(m.width);
      CAPTURE(static_cast<int>(conn));
      CHECK(same_blobs(detail::union_find_label(m.mask(), conn),
                       detail::reference_label(m.mask(), conn)));
    }
  }
}

TEST_CASE("property: the fast path agrees with the oracle on random masks") {
  // Deterministic seed so any failure reproduces exactly.
  std::mt19937 rng(0xC0FFEEu);
  std::uniform_int_distribution<int> dim(0, 24); // include 0 (empty) up to 24x24
  std::uniform_real_distribution<double> density(0.0, 1.0);

  for (int iter = 0; iter < 800; ++iter) {
    const int w = dim(rng);
    const int h = dim(rng);
    std::bernoulli_distribution fg(density(rng)); // vary sparse..dense
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (std::uint8_t& px : pixels) px = fg(rng) ? std::uint8_t{255} : std::uint8_t{0};
    const Mask mask{pixels.data(), w, h};

    for (const Connectivity conn : {Connectivity::Four, Connectivity::Eight}) {
      const bool ok = same_blobs(detail::union_find_label(mask, conn),
                                 detail::reference_label(mask, conn));
      if (!ok) {
        std::string art; // reproduce the exact mask on failure
        for (int y = 0; y < h; ++y) {
          for (int x = 0; x < w; ++x) art += mask.foreground(x, y) ? '#' : '.';
          art += '\n';
        }
        CAPTURE(iter);
        CAPTURE(w);
        CAPTURE(h);
        CAPTURE(static_cast<int>(conn));
        CAPTURE(art);
      }
      REQUIRE(ok);
    }
  }
}
