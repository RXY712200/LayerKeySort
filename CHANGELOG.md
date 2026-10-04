# Changelog

Notable changes to LayerKeySort are documented here.

## [3.1.0] - Unreleased

This compatible V3 feature release improves how applications obtain and build
LayerKeySort. It does not change the public C function set or production
ordering implementation.

- Add CMake install/export support for a locally built static library. Consumers
  can use `find_package(LayerKeySort CONFIG REQUIRED)` and link
  `LayerKeySort::layerkeysort`. Source subprojects now default tests, examples,
  and install rules OFF.
- Add a generated two-file C17 amalgamation and deterministic candidate Release
  assets: `LayerKeySort-3.1.0-amalgamation.zip` plus a SHA-256 manifest.
  GitHub's automatic tagged source archives remain the full repository.
- Extend regression validation to source-tree and offline FetchContent builds,
  installed C and C++ consumers, and amalgamation consumers. CI also checks
  asset reproducibility, exact membership, checksum integrity, and tamper
  rejection. The read-only candidate workflow produces an Actions artifact;
  public Release publication remains separate.
- Rework first-use documentation around integration choices and API selection.
  Separate current architecture, validation, and usage from archived design
  decisions and benchmark history.
- Preserve Path comparison, display grammar, LK1 v1 bytes, Tree ownership,
  comparator behavior, relabel semantics, and the core production C source.

## [3.0.0] - 2026-10-03

This is the published V3 Stable release. `v2.0.0` remains the historical
stable 2.x release; `v3.0.0-rc.1` remains a historical prerelease. The 3.x
compatibility contract is active.

- Separate manual-coordinate `LksTree` from comparator-bound `LksOrderedTree`.
  Managed insertion uses logical Path relabeling rather than the V2 managed
  full-Tree replacement fallback; a relabel may still cover the full range.
- Retain V2 Path comparison, canonical display parsing, LK1 v1 bytes, Group
  and GroupBatch semantics, and stable `lks_sort()` behavior.
- Clarify comparator, caller-owned item/context, borrowed-view lifetime, and
  remove/update/reinsert contracts. Improve Quick Start, examples, and C17
  source/CMake consumer guidance across the V3 Preview series.
- Retain the RC.1 documented-input, mutation, parser, and allocation-failure
  regressions. The exact RC.1 commit passed five GitHub CI configurations,
  each with CTest 8/8. A separate local post-release campaign used clean
  consumers, ten 500,000-operation seeds per Tree type at a 12,000-item peak,
  adversarial insertion, parser/LK1 torture, OOM sweeps, and 5,000 lifecycle
  cycles. These extended local runs were not reproduced by GitHub CI and
  their raw outputs are not part of this repository.
- Keep full-range relabel, synchronous insertion tail latency, and
  workload-dependent Path depth/storage explicit. Complete managed insertion
  has no proven worst-case `O(log n)` guarantee or formal amortized bound.
- Replace the default historical V1 web and README visuals with a V3
  architecture explanation and production-recorded managed-ordering replay.
  Keep the V1 showcase clearly marked as history. Extend the release review
  to all user-facing documentation, examples, assets, and navigation.

The final Stable preparation changes version metadata, documentation, and static presentation only. It does not
change public function signatures, the 59-function API count, production
ordering, Path semantics, parser behavior, or LK1 v1 encoding.

## [3.0.0-rc.1] - 2026-10-02

This published Release Candidate freezes the existing V3 feature set for final
stabilization. It does not add an ordering model, change Path/LK1 encoding, or
claim a bound for complete managed insertion.

- Extend documented-input and lifecycle regression checks, including NULL
  arguments, duplicate/missing Paths, aliased locate outputs, undersized
  formatting, and constructor allocation failures.
- Revalidate deterministic mixed mutations, adversarial insertion patterns,
  parser inputs, existing OOM sweeps, supported compilers, and consumer builds.
- Pass strict local GCC C17, MSVC Debug/Release and MSVC AddressSanitizer
  validation. The reviewed candidate passed Windows MSVC, Ubuntu GCC/Clang,
  Ubuntu Clang sanitizer, and macOS AppleClang CI with CTest 8/8 in each job.
- Verify `add_subdirectory`, SHA-pinned remote `FetchContent`, direct C17
  source integration, and all three public examples.
- Keep possible full-range relabel, workload-dependent Path growth, and
  insertion tail latency visible as known limitations; no proven worst-case
  or formal amortized bound is claimed for complete managed insertion.

## [3.0.0-preview.5] - 2026-10-02

This released experimental Preview prepares the existing V3 implementation for
release-candidate consideration. It does not change the ordering algorithm,
Path model, LK1 format, or public function signatures.

- Recheck public ownership, comparator, borrow, and error contracts against
  implementation and consumer tests.
- Align current-version documentation and distinguish historical V2 Preview
  features from the released V3 Preview milestones.
- Validate strict C17 builds, examples, external CMake consumers, and the
  complete test suite across available local configurations.

## [3.0.0-preview.4] - 2026-10-02

This released experimental Preview prepares the existing V3 architecture for first use.
It changes examples, documentation, and integration guidance without changing
the ordering algorithm, public function set, Path semantics, or LK1 bytes.

- Add a short comparator-managed Tree example covering create, insert, locate,
  exact removal, and cleanup; build and run it in CI.
- Put a verified Quick Start and CMake consumer paths near the start of the
  documentation, and distinguish the manual and managed Tree models.
- Publish measured large-relabel and Path-storage limitations without claiming
  a complete-insertion bound or a universal performance result.
- Keep stable V2 as the recommended release for normal use while V3 remains
  experimental. Preview.3 is also published as a Prerelease.

## [3.0.0-preview.3] - 2026-10-02

This released experimental Preview stabilizes the current V3 core contract. It does
not redesign the ordering algorithm or change Path/LK1 bytes.

- Clarify borrowed item/context lifetime, comparator stability, safe item-key
  update by remove/reinsert, and mutation invalidation for both Tree models.
- Keep physical navigation as an ephemeral implementation-defined view rather
  than making AVL shape a logical ordering contract.
- Correct V3 relabel and complexity documentation, including the depth-16
  endpoint allowance and potentially full-range Path replacement.
- Extend diagnostic benchmarking with a pinned-item interior hotspot and
  final resident Path/LK1 footprint records. Historical timing data are unchanged.

## [3.0.0-preview.2] - 2026-10-01

This released experimental V3 Preview optimizes comparator-managed endpoint
insertion after Preview.1. Stable `v2.0.0` remains the recommended version
for normal use.

- Carry a saturated endpoint slot into the nearest available ancestor instead
  of repeatedly extending the Path. Prepend uses negative-root slot and level
  carry. Long successful endpoint runs switch to a one-slot stride; mixed
  insertion retains ten-slot spacing.
- Keep manual Path gap APIs, AVL topology, Path comparison, display text, and
  LK1 v1 bytes unchanged. Existing logical-window relabel remains the bounded
  interior strategy and full-range correctness fallback.
- Add endpoint/OOM and long-run mixed-operation coverage, placement/relabel
  diagnostics, and direct stable V2 / Preview.1 / Preview.2 benchmark evidence.
- Defer endpoint compaction until a valid candidate exceeds depth 16, avoiding
  the measured depth-nine full-range relabel while bounding Path growth.
- Adopt real-use clarity and first-use experience as V3 design principles;
  Preview.2 does not complete the planned integration and distribution work.
- Complete insertion can still relabel all nodes. No worst-case `O(log n)`
  complete insertion or formal amortized bound is claimed.

## [3.0.0-preview.1] - 2026-10-01

This released experimental V3 Preview is based on stable `v2.0.0`. For normal
use, prefer stable V2; its published history is unchanged.

- Split manual coordinate `LksTree` from comparator-bound `LksOrderedTree`.
  The V2 per-operation-comparator Tree insert/locate functions are removed from
  the V3 public API. Managed ordering binds one comparator and borrowed context
  for the container lifetime; arbitrary rekey remains manual-only.
- Kept the Path-keyed AVL index. Managed insertion now prepares and commits
  adaptive logical-range Path relabeling, expanding geometrically to a
  full-range relabel if necessary. It does not replace physical Tree nodes via
  `rebuild_with_item()`. Bulk Tree construction remains available internally.
- Preserved Path order, canonical display text, LK1 v1, item ownership,
  immutable Group/Batch behavior, and stable `lks_sort()` semantics.
- Added managed-container invariant/OOM tests and a direct stable V2 benchmark
  comparison. The current data includes slower 100k ascending/all-equal cases;
  no complete-insertion worst-case or formal amortized bound is claimed.

## [2.0.0] - 2026-10-01

The first stable V2 release establishes the documented 2.x source/API and
semantic compatibility contract. It retains `v2.0.0-rc.1`'s production implementation:

- Hierarchical Path coordinates, a mutable Path-keyed Tree, stable ordering of
  comparator-equal items, and before/after/between coordinate generation.
- Exact-Path removal and failure-atomic rekey; canonical Path display and
  parsing; versioned LK1 sortable, durable coordinate keys.
- Immutable Group and Batch results, stable merge behavior, and `lks_sort()`.
- C17 source integration through CMake or direct sources, validated on Windows,
  Linux, and macOS.
- Permanent regression coverage from RC observation for business ID versus
  LK1 persistence, parser torture, multi-seed soak, and Path-growth OOM rollback.

Full rebuild can still be costly; complete comparator-driven insertion has no
formal worst-case or amortized bound. Comparator compatibility remains the
caller's responsibility. Paths and LK1 keys are ordering coordinates, not
permanent application item identities.

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
