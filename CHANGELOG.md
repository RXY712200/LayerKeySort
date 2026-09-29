# Changelog

Notable changes to LayerKeySort are documented here.

## [2.0.0-preview.2] - Unreleased

### Changed

- Comparator-driven Tree insertion can sparsely rebuild a bounded, complete
  local subtree when a Path becomes too deep or its prefix topology is absent.
  It may accept a deeper Path when local repair costs too much, retaining the
  atomic full-Tree rebuild as the final fallback.
- Local replacement prepares and validates an independent branch before an
  allocation-free splice. Allocation failure preserves the original Tree.
- Internal diagnostic builds count local attempts, successes, fallbacks,
  affected nodes, region growth, deeper-Path accepts, and full rebuilds.

The public API and caller ownership rules are unchanged. Preview window sizes,
depth thresholds, and sparse slot spacing remain provisional policy.

## [2.0.0-preview.1]

### Added

- `lks_sort` for stable sorting of a caller-owned pointer array.
- CMake production library, example, and diagnostic test targets; cross-platform CI.
- Sparse 26-way bulk Path construction and focused V2 regression coverage.

### Changed

- Group build and merge now assign a fresh balanced Path layout from the final
  stable item order. Inputs remain immutable; result Paths need not retain the
  exact values used in the source Groups.
- Batch merge combines ordered pointer sequences before one final Group build.
- Online Tree insertion uses available gaps and atomically rebuilds a Tree
  when Path growth crosses the preview policy threshold.
- Production allocation no longer updates process-global test counters;
  fault injection and detailed accounting remain available in diagnostic builds.

Path-allocation heuristics are provisional and may change before v2.0.0.
Paths are ordering coordinates, not persistent IDs or a serialization format.

## [1.0.0] - 2026-09-26

### Added

- Initial public C17 release of the LayerKeySort library.
- Hierarchical Path keys and public Path construction, navigation, formatting, and comparison APIs.
- Tree operations for inserting, locating, and navigating items by Path or comparator.
- Group and GroupBatch construction, stable group merging, and caller-owned item-pointer support.
- A standalone example program and a public API usage smoke test.
- Deterministic property, stress, allocation-failure, and out-of-memory validation.
- MSVC x64 Debug, Release, and AddressSanitizer validation.
