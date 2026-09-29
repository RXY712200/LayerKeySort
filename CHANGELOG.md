# Changelog

Notable changes to LayerKeySort are documented here.

## [2.0.0-preview.3] - Unreleased

Preview.3 is the current development snapshot; it has not been tagged or
released.

### Added

- A private fixed-width radix-54 slot codec covering every legal 16-bit slot,
  with the exact ordered alphabet specified in `docs/API.md`.
- Focused exhaustive slot-codec, Path-text, boundary, bulk-distribution,
  local-repair, and fault-injection coverage for the changed representation.

### Changed

- Path slots now cover the full 16-bit range `0..65535`; bulk roots spread
  sparsely over that range while the Preview.2 repair heuristics remain.
- Path text uses fixed three-character slot tokens, decimal nonzero first
  levels, and decimal later level deltas; `/` now only separates steps.
  Skipped levels no longer use repeated slash counts.
- README, API, usage, and development documentation now specify current slot
  rank, Path comparison, formatter grammar, coordinate lifetime, and
  provisional policy separately from historical v1 behavior.

### Internal

- Historical v1 owned-base merge helpers remain available to diagnostic
  regression builds through `LKS_ENABLE_V1_REGRESSION_HELPERS` and are excluded
  from the ordinary production library. Public V2 merge still creates a fresh
  result Group and leaves its inputs unchanged.

### Validation

- For ascending online N=1024, measured maximum depth changed from 4 to 1,
  repair attempts from 563 to 0, and peak live bytes from 210176 to 90200.
  These representative results do not imply a worst-case bound or production
  readiness. See `docs/DEVELOPMENT.md` for the complete measured comparison.

## [2.0.0-preview.2] - 2026-09-29

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
