# Changelog

All notable changes to this project are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versioning: [SemVer](https://semver.org/).

## [Unreleased]

## [0.1.1] - 2026-09-25

### Fixed
- Build the WebAssembly module with `-sDYNAMIC_EXECUTION=0` so embind no longer generates its call
  invokers with `new Function(...)`. Under a strict Content-Security-Policy (no `'unsafe-eval'`) the
  previous build threw `EvalError` from `createJsInvoker` on first use; the module now runs with only
  the narrow `'wasm-unsafe-eval'` allowed. No API change; the `.wasm` and all smoke checks are
  unchanged, and the JS glue shrinks from ~30 KB to ~28 KB.

## [0.1.0] - 2026-09-25

### Added
- Project scaffold: header-only CMake library (`motes::motes`), public API — `Mask`, `Blob`,
  `Connectivity`, and a stub `label()` — a doctest harness, GitHub Actions CI (Linux + macOS),
  README, MIT licence.
- Reference labeller: `label()` now finds real blobs via an iterative flood fill
  (`detail::reference_label`) — the correctness oracle for the fast path to come. Full blob stats
  (bounding box, pixel area, area-weighted centroid), 4- and 8-connectivity. Tests cover a single
  blob's stats, separate regions, the diagonal 4-vs-8 split, a ring/hole, and raster-order labels.
- Fast path: `label()` now uses a two-pass **union-find** connected-components labeller
  (`detail::union_find_label`, with a path-halving / union-by-size disjoint-set); the flood-fill
  reference is retained as the oracle. Both emit the same blobs, in the same raster order, with full
  stats. Parity tests confirm agreement on fixed masks, including a U-shape that merges labels.
- Property test: 800 random masks (varied size and density, fixed seed) assert the fast path equals
  the oracle for both connectivities — ~1600 comparisons — printing the offending mask if they ever
  differ. The labeller's correctness is pinned, not just spot-checked.
- Benchmark: a self-contained `std::chrono` harness (`bench/bench.cpp`, built at `-O3`, not run by
  ctest) with a naive iterative baseline. On an Apple Silicon Mac a 640×480 frame labels in ~0.87 ms
  (353 MPix/s, ~1150 fps) — ~1.1× a good flood fill (linear either way) and ~31× the naive iterative
  approach. Full table in the README.
- WebAssembly: an Emscripten/embind binding (`web/binding.cpp`) exposing `labelMask` to JavaScript,
  a `web/build.sh` producing a ~16 KB `.wasm` plus an ES-module loader, and a Node smoke test
  (`web/smoke.mjs`) confirming the WASM path returns the same blobs as native. CI builds and
  smoke-tests it (setup-emsdk).
- npm package: an ergonomic wrapper (`web/index.mjs`) — `label(mask, w, h, opts)` and
  `labelImageData(image, opts)` (threshold an RGBA image, then label), hiding the malloc/free — with
  TypeScript types (`index.d.ts`) and package metadata (`@raby/motes@0.1.0`, `files`, `exports`, a
  `prepublishOnly` that builds + smoke-tests). The published tarball is ~20 KB (7 files). The smoke
  test now exercises the public API including `labelImageData` and undersized-mask error handling.
