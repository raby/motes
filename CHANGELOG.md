# Changelog

All notable changes to this project are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versioning: [SemVer](https://semver.org/).

## [Unreleased]

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
