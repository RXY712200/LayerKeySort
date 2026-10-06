# Validation

Current V4 scope is **experimental Preview.4**: [flat immutable Groups/Batch and full workload evidence](V4_PREVIEW4.md), plus [immutable snapshots, LS1, persistence, restoration and V3 import](V4_PREVIEW3.md), alongside [live moves and managed order](V4_PREVIEW2.md). Stable remains v3.1.0. Earlier milestone guides are historical records; V4 API/wire freeze is deferred to Preview.5.

Local V4 Preview.1 adds `layerkeysort_v4_live_order` to the preserved V3 suite.
See [V4 Preview.1](V4_PREVIEW1.md) for oracle/failpoint/structural scope. The V3
coverage and historical validation evidence below remain applicable to the
coexisting V3 implementation, with the separate Preview.3 snapshot suite covering the currently implemented export features.

This page describes how the current V3 source is checked. Test coverage and
past passing runs are evidence, not a formal proof of all workloads.

Preview.4 adds Group/Batch stability/OOM/reentry tests, concurrent marker release/read tests, production semantic-trace smoke campaigns and Group comparison smoke. CI explicitly runs all seven examples. Public C/C++/amalgamation consumers cover all 14 new Group APIs alongside the 17 snapshot/import functions. See the Preview.4 report for the verified test count and local/remote matrix.

## Local CMake run

```sh
cmake -S . -B build -DLKS_BUILD_BENCHMARKS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The historical V3-only configuration registered eight tests: `layerkeysort_v3_core`,
`layerkeysort_mutation_soak`, `layerkeysort_consumer_roundtrip`,
`layerkeysort_parser_torture`, `layerkeysort_rc_contract`, and three benchmark
smoke tests. The smoke tests check harness execution; they are not timing
thresholds. The three examples use only the public header and can be built
and run separately. [INTEGRATION.md](INTEGRATION.md) documents external
consumers. Diagnostic builds and timed benchmark builds are separate.

## CI and sanitizers

The repository workflow checks Windows MSVC, Ubuntu GCC, Ubuntu Clang,
Ubuntu Clang with ASan/UBSan, and macOS AppleClang. Each configuration runs
the registered CTest suite and examples. A passing run covers its exact
compiler, runner, and revision, not every supported machine. For compatible
GCC/Clang installations, configure `-DLKS_ENABLE_SANITIZERS=ON` to enable
the sanitizer build. The Visual Studio project also provides x64 Debug,
Release, and ASan configurations. On Windows the ASan runtime DLL may need
its installed MSVC runtime directory in the process `PATH`.

## Regression and fault injection

The core runner covers public API use, Path and LK1 parsing, deterministic
properties, stress and mutation cases, historical fixtures, and allocation
failure. Diagnostic allocation/fault-injection state is private and
process-global; concurrent use of that test instrumentation is not promised.
The production CMake target omits it. The parser torture and public consumer
tests exercise external-input and header boundaries independently.

`layerkeysort_mutation_soak` defaults to 30,000 requested operations. Its
manual phase uses 22,500 steps with seed `0x6B47C291`; its managed phase
uses **30,000** steps with seed `0xA9172E63`. Endpoint phases run separately
after those two phases. Thus “30,000” is the argument to the soak, not the
sum of both phases. The flat model checks Path/item associations, order,
size, returned pointers, AVL invariants, and comparator compatibility.
The active set is drained periodically; dedicated tests cover deep Paths.
For a larger run, invoke `layerkeysort_soak --operations 100000` or
`--operations 500000` (include the configuration directory for a
multi-configuration generator).

The exact RC.1 tag passed all five CI configurations with CTest 8/8 each.
An additional post-RC local campaign covered deterministic 500,000-operation
seeds, adversarial insertion, parser/LK1 and OOM cases, and lifecycle cycles.
Its raw outputs are outside this repository and were not reproduced by CI.
The published `v3.0.0` production source retains the RC.1 implementation;
the 3.1.0 source keeps that ordering implementation unchanged. See
[BENCHMARKS.md](BENCHMARKS.md) for measured limits and provenance.

## Distribution validation

For 3.1.0 source, `python tools/validate_distribution.py` exercises
external source-tree and offline FetchContent consumers, local install plus
`find_package` for C and C++, strict-warning amalgamation consumers,
the generated package example,
and repeatable package generation. It checks that 3.1.0 source dependencies do
not enable tests, examples, or install rules by default. The historical
`v3.0.0` tag had different subproject defaults.

`python tools/validate_release_assets.py` builds the candidate asset set twice
and checks exact ZIP/checksum membership, reproducibility, checksum matching,
and tamper rejection. Normal CI runs these checks as regression protection.
The separate `Build release assets` workflow is a read-only, manually
initiated candidate build that uploads a temporary GitHub Actions artifact.
It does not publish a GitHub Release or attach public Release assets. Manual
dispatch in the GitHub UI requires the workflow to reach the default branch.
