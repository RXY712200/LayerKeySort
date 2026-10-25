# V4 Stable validation

**Version status:** [LayerKeySort Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) is the current published Stable release. The 4.x API and wire contract stay frozen; see [4.1.0 notes](V4_1_0.md).

V4.0.0 Stable was promoted only after Preview.5 freeze and independently reviewed RC.1.

The Stable commit preserves the frozen production implementation and compatibility
contracts; Stable promotion itself changed release metadata/presentation only.

## Current automated gates

With benchmarks/Python enabled, the current baseline is 39 CTest tests.

The suite covers:

- frozen 64-function / 14-type API manifest,
- explicit and managed live order,
- handle/cursor semantics,
- Group/Batch behavior,
- snapshot lifetime/currentness,
- LS1 exact vectors and malformed inputs,
- LKS4SNP1 exact bytes and parser rejection,
- strict V3 LK1 migration,
- deterministic OOM/failure atomicity,
- allocation-free documented paths,
- marker lifetime/concurrency within promised contracts,
- C/C++ public consumers,
- distribution/amalgamation/package validation.

All seven public examples execute in CI.

## Platform matrix

Validated release configurations include:

- Windows MSVC x64,
- Windows MSVC Win32,
- Linux GCC,
- Linux Clang,
- Linux Clang ASan/UBSan,
- macOS AppleClang,
- C17 implementation,
- C++17 consumer.

The project does not claim every C17 compiler/platform or a universal binary ABI.

## Release evidence

RC extended campaigns include:

- three 200k-step explicit mutation seeds,
- long hostile parser campaigns,
- large distant-move/churn traces,
- marker lifetime stress,
- focused Preview.4 counter replay.

Preview.4 documents historical V4 convergence. The later three-step 4.1.0
integrated [A/B/C research](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md)
records 1,548 completed native processes, 36 safe skips and 11,802 recorded
exact-order fingerprints; benefits and regressions are both retained.

See [RC record](V4_RC1.md) and [Stable record](V4_STABLE.md).

## What validation does not prove

Passing the current matrix does not establish:

- a universal hard latency bound,
- universal performance superiority,
- every allocator/OS behavior,
- every real editor workload,
- general mutable-container thread safety,
- distributed/CRDT correctness,
- a whole-library formal complexity theorem.

Remaining gaps include application-specific latency budgets, arbitrary-duration
allocator behavior, and larger sanitizer/high-retention scenarios. Revisit
validation for concrete engineering questions. Real traces can help, but
external adoption is not required to continue internal study.

Current roadmap:
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).

Frozen manifests/goldens cannot silently change inside Stable 4.x.
