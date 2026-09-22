// Standalone microbenchmark for motes. This is NOT a correctness test — that is the doctest
// suite. The CMake target forces -O3, because an unoptimised benchmark measures nothing.
//
// The story is honest by construction: connected-components labelling is inherently linear, so
// `motes` (two-pass union-find) is only *modestly* faster than a good flood fill — we print that
// ratio rather than dress it up. The real headlines are (a) real-time throughput (MPix/s and fps
// on a webcam-sized frame) and (b) the speed-up over a genuinely naive iterative labeller.
// Numbers are machine-dependent.
#include "motes/motes.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using namespace motes;
using Clock = std::chrono::steady_clock;

namespace {

// A representative synthetic frame: `rects` random filled rectangles on a black background — a
// rough stand-in for a thresholded camera frame (a handful of coherent regions, not pure noise).
std::vector<std::uint8_t> make_frame(int w, int h, int rects, std::uint32_t seed) {
  std::vector<std::uint8_t> px(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> rx(0, w - 1);
  std::uniform_int_distribution<int> ry(0, h - 1);
  std::uniform_int_distribution<int> rw(4, std::max(4, w / 6));
  std::uniform_int_distribution<int> rh(4, std::max(4, h / 6));
  for (int i = 0; i < rects; ++i) {
    const int x0 = rx(rng);
    const int y0 = ry(rng);
    const int x1 = std::min(w, x0 + rw(rng));
    const int y1 = std::min(h, y0 + rh(rng));
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        px[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] = 255;
      }
    }
  }
  return px;
}

// Naive iterative connected-components: give every foreground pixel a unique label, then relax
// each to the minimum of itself and its foreground neighbours, sweeping until nothing changes.
// Correct, but O(n * passes) — the honest "did it the naive way" baseline. Returns component count.
int naive_component_count(const Mask& mask, Connectivity conn) {
  const int w = mask.width;
  const int h = mask.height;
  if (w <= 0 || h <= 0) return 0;
  const int dx8[] = {1, -1, 0, 0, 1, 1, -1, -1};
  const int dy8[] = {0, 0, 1, -1, 1, -1, 1, -1};
  const int ndir = (conn == Connectivity::Eight) ? 8 : 4;

  std::vector<int> lab(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);
  int n = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (mask.foreground(x, y)) {
        lab[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] = ++n;
      }
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const std::size_t idx =
            static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x);
        int m = lab[idx];
        if (m == 0) continue;
        for (int d = 0; d < ndir; ++d) {
          const int nx = x + dx8[d];
          const int ny = y + dy8[d];
          if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
          const int nl =
              lab[static_cast<std::size_t>(ny) * static_cast<std::size_t>(w) + static_cast<std::size_t>(nx)];
          if (nl != 0 && nl < m) m = nl;
        }
        if (m != lab[idx]) {
          lab[idx] = m;
          changed = true;
        }
      }
    }
  }

  std::sort(lab.begin(), lab.end());
  int count = 0;
  int prev = 0;
  for (const int v : lab) {
    if (v != 0 && v != prev) {
      ++count;
      prev = v;
    }
  }
  return count;
}

template <class F>
double best_ms(F&& f, int reps) {
  f(); // warmup
  double best = 1e300;
  for (int i = 0; i < reps; ++i) {
    const auto t0 = Clock::now();
    f();
    const auto t1 = Clock::now();
    best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
  }
  return best;
}

double mpix_per_s(int w, int h, double ms) {
  return (static_cast<double>(w) * static_cast<double>(h)) / (ms * 1000.0);
}

} // namespace

int main() {
  std::printf("motes benchmark  (8-connectivity, best-of-N, machine-dependent)\n");
  std::printf("input: random filled rectangles on black, a stand-in for a thresholded frame\n\n");

  volatile std::size_t sink = 0;

  struct Case {
    int w;
    int h;
    int rects;
    bool with_naive;
  };
  const Case cases[] = {
      {640, 480, 60, true}, // webcam — the demo resolution, where "real-time" and "vs naive" live
      {1280, 720, 120, false},
      {1920, 1080, 200, false},
  };

  for (const Case& c : cases) {
    const auto px = make_frame(c.w, c.h, c.rects, 12345u);
    const Mask mask{px.data(), c.w, c.h};
    const auto blobs = label(mask, Connectivity::Eight);

    const int reps = (c.w * c.h < 1'000'000) ? 300 : 80;
    const double motes_ms = best_ms([&] { sink += label(mask, Connectivity::Eight).size(); }, reps);
    const double flood_ms =
        best_ms([&] { sink += detail::reference_label(mask, Connectivity::Eight).size(); }, reps);

    std::printf("%d x %d — %zu blobs\n", c.w, c.h, blobs.size());
    std::printf("  motes (union-find):  %8.3f ms   %7.1f MPix/s   %5.0f fps\n", motes_ms,
                mpix_per_s(c.w, c.h, motes_ms), 1000.0 / motes_ms);
    std::printf("  flood-fill (oracle): %8.3f ms   (motes %.2fx)\n", flood_ms, flood_ms / motes_ms);

    if (c.with_naive) {
      const int nn = naive_component_count(mask, Connectivity::Eight);
      if (nn != static_cast<int>(blobs.size())) {
        std::printf("  !! naive count mismatch: %d vs %zu — aborting\n", nn, blobs.size());
        return 1;
      }
      const double naive_ms =
          best_ms([&] { sink += static_cast<std::size_t>(naive_component_count(mask, Connectivity::Eight)); }, 5);
      std::printf("  naive iterative:     %8.1f ms   (motes %.0fx faster)\n", naive_ms,
                  naive_ms / motes_ms);
    }
    std::printf("\n");
  }

  std::printf("(checksum %zu)\n", static_cast<std::size_t>(sink));
  return 0;
}
