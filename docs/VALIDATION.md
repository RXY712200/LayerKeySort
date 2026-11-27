# V4 Stable validation

Released source: [`v4.2.0`](https://github.com/RXY712200/LayerKeySort/tree/v4.2.0), commit `f751e1ee2afd682874281d6dc495969f9eca49eb`. RC/Preview test pins below are retained historical verification baselines.

[简体中文](VALIDATION.zh-CN.md)

Full v4.2.0 retains all 64 functions, 14 public types, LS1/LKS4SNP1 fixtures and strict V3 migration. Stable promotion changed release metadata, not operational semantics. See the [4.2 verification record](V4_2_0.md).
To reproduce the Full v4.2.0 operational source and test baseline, use checkout `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08` for the historical RC metadata and both Preview.3 regressions; this historical checkout is not the released v4.2.0 commit. The earlier operational source pin alone lacks these tests. See the [4.2 quantitative evidence](FULL_RC1_EVIDENCE.md). For final PR validation, run the required checks at the actual PR head.


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

## Retained 4.2 regression evidence

Full v4.2.0 retains the unaligned snapshot-buffer regression from `754b3088c160564f6666ee520284168ee5da6ac7` and the restored mixed-tag comparison regression from `3ac82d26301ea12876a3ebdba6319fe8233621f3`. Both are in `tests/v4_snapshot.c`. The former checks decoding and encoding at byte offset one against literal wire bytes. The latter deliberately clears two restored block tags, verifies the internal invariant and both comparison directions through the fallback, then restores the tags. These tests accompany the retained production implementation at `fe00536e7bd7f4e5128432ea4a29e54158074903`. Final hosted results must identify the actual documentation PR commit.

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

Historical v4.0.0 RC extended campaigns include:

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
