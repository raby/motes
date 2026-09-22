#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "motes/motes.hpp"

#include <cstdint>
#include <vector>

using namespace motes;

// Slice 1 pins the API shape and the harness. With the stub labeller these pass because there is
// nothing to find; Slices 2–5 add the real cases (single blob, multiple, 4- vs 8-connectivity,
// holes, edges) and the brute-force property test.

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
  // A single lit pixel at (2, 1) in a 4x3 mask.
  std::vector<std::uint8_t> pixels(4 * 3, 0);
  pixels[1 * 4 + 2] = 255;
  const Mask mask{pixels.data(), 4, 3};
  CHECK(mask.foreground(2, 1));
  CHECK_FALSE(mask.foreground(0, 0));
  CHECK_FALSE(mask.foreground(3, 2));
}
