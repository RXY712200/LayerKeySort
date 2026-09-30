# Changelog

Notable changes to LayerKeySort are documented here.

## [2.0.0-rc.1] - 2026-09-30

RC.1 is the published V2 feature, public API, Path display, and LK1 format
freeze candidate. It remains a prerelease, not stable or production-ready
2.0.0. No new public function, production algorithm, or external-key version
is introduced.

- Defined the intended 2.x source and semantic compatibility boundary in
  `docs/COMPATIBILITY.md`, distinguishing public contracts from generated
  Path values and private implementation details.
- Rechecked the 52-function public surface, canonical Path display grammar,
  LK1 v1 golden vectors, and documented consumer integration paths.
- Retained Preview.5's five-platform CI, deterministic mutation soak, and
  benchmark correctness smoke coverage. RC.1 does not resolve the documented
  full-rebuild cost or establish a formal insertion bound.

## [2.0.0-preview.5] - 2026-09-30

Preview.5 is a released Preview snapshot, not stable 2.0 or a production-ready
release. It concentrates on usability, integration, long-run validation,
platform coverage, and preparation for a V2 public-contract freeze.

- Added a compilable dynamic layer-list example using existing public Path and
  Tree APIs for between placement, rekey, removal, display text, and LK1
  round trip. Application item IDs remain separate from Path coordinates.
- Added use-case guidance and a source/CMake integration guide, including
  `add_subdirectory`, `FetchContent`, and direct C17 source integration.
- Added deterministic long-run mixed Tree mutation soak coverage, with a
  bounded CI run and larger manual mode. Existing focused property, OOM, and
  deep-Path tests remain in place.
- Extended CI to macOS/AppleClang and runs both examples. The public C API
  remains 52 functions; LK1 version 1 is unchanged.

## [2.0.0-preview.4] - 2026-09-30

Preview.4 is a released Preview snapshot, not a production-ready release.

- Decoupled mutable Tree topology from Path hierarchy with a Path-keyed
  balanced index. Explicit unique Paths no longer require physical prefixes.
- Removed mutable ChildBlock storage and linear equal-run successor scanning.
  Comparator insertion can relabel a bounded logical-order range; full rebuild
  remains a failure-atomic fallback.
- Deep Path values no longer impose equivalent physical Tree depth. Physical
  navigation is implementation-defined; ownership and failure atomicity remain.
- Added Path-addressed Tree removal without allocation or Path compaction,
  and failure-atomic rekey of one caller-owned item's coordinate. Published
  Groups remain immutable; comparator compatibility remains the caller's
  responsibility for subsequent comparator-driven operations.
- Added strict canonical display parsing and independent, versioned `LK1:`
  durable Path keys whose bytewise lexical order matches Path order. Parsing
  checks malformed input, integer overflow, and allocation failure; deep
  Paths use iterative processing. Equal-Path rekey preserves borrowed views.
- Converged logical coordinate policy names, removed a redundant repair
  threshold, and added fixed version-1 key vectors, gap boundaries, and
  mixed Tree mutation/Path representation regression coverage.
- Added an optional deterministic public-API benchmark harness and documented
  Preview.3/Stage 4/Stage 5 timing, memory, Path size, and complexity limits.
  Comparator insertion now adopts its generated Path instead of cloning it;
  open-end insertion skips repeatedly ineffective local repair while within
  the hard depth allowance. Interior repair and full rebuild remain available.
  The benchmark report records workloads that regress as well as improve.
- Whole-Tree serialization and distributed semantics remain optional future
  work. Path-coordinate persistence does not provide permanent item IDs. This
  release is not production-ready. The documented benchmark results include
  alternating and small-random insertion regressions against Preview.3.

## [2.0.0-preview.3] - 2026-09-29

Preview.3 is a released Preview snapshot, not a production-ready release.

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
