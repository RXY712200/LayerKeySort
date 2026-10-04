# Developing LayerKeySort

This is the contributor workflow for the published `v3.0.0` Stable source.
Read [architecture](ARCHITECTURE.md) for the current implementation,
[validation](VALIDATION.md) for test scope and evidence, and
[benchmark evidence](BENCHMARKS.md) for performance limits. The
[compatibility contract](COMPATIBILITY.md) governs public changes.

## Repository layout

- `include/` — public header.
- `src/` — production C implementation and private headers.
- `tests/` — property, stress, deterministic soak, and public API validation code.
- `benchmarks/` — optional current public-API benchmark harness and matrix runner.
- `examples/` — managed Tree first use, basic sort, and dynamic layer-list public API examples.
- `demo/` — executable validation/demo entry point.
- `docs/` — user documentation, current V3 visualizer, historical V1 showcase, and site assets.
- `LayerKeySort.slnx` — Visual Studio solution.
- `LayerKeySort.vcxproj` — Visual Studio project and build configurations.
- `CMakeLists.txt` — reusable production library, example, and test targets.
- `.github/workflows/ci.yml` — MSVC, GCC, Clang, and AppleClang validation.

## Toolchain

Both Visual Studio and CMake compile as C17. The CMake matrix also targets
GCC and Clang on Ubuntu and AppleClang on macOS. New production C source
belongs in both build systems. Consumer setup is in [INTEGRATION.md](INTEGRATION.md).

## CMake build and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`layerkeysort` contains production source only. All three example targets link
that library. `layerkeysort_tests` and `layerkeysort_soak` link a separately
compiled diagnostic variant so allocation counters and single-shot fault
injection remain usable without shared mutable instrumentation in ordinary
production allocations.
`-DLKS_BUILD_BENCHMARKS=ON` adds a production-linked public-API harness and
three small CTest smoke cases. See [BENCHMARKS.md](BENCHMARKS.md) for timed
results and limits; diagnostic counter builds are separate from timed builds.
For a supported GCC or Clang toolchain, `-DLKS_ENABLE_SANITIZERS=ON` enables
AddressSanitizer and UndefinedBehaviorSanitizer. Diagnostic fault injection is
process-global test state and carries no concurrent-test guarantee.

For test names, soak counts and seeds, sanitizer scope, and historical
validation evidence, see [VALIDATION.md](VALIDATION.md).

## Build configurations

The Visual Studio project defines **Debug**, **Release**, and **ASan** configurations for x64. Open `LayerKeySort.slnx` in Visual Studio and select the configuration and x64 platform before building.

## Running the validation runner

The Visual Studio executable entry point is `demo/main.c`. With no argument it
runs the V2 focused tests, public API smoke, deterministic property tests,
stress tests, and OOM tests. The CMake test executable uses `tests/v2_main.c`
for the same retained regression suite. `tests/benchmark.c` preserves the historical v1
benchmark evidence and remains available through its existing focused runner
switches; its exact v1 baseline expectations do not describe V2 Path output.

The entry point also handles the existing focused arguments below; these spellings are read from `demo/main.c`:

- `--stage14.1-only`
- `--stage14.2-only`
- `--stage14.2-oom-only`
- `--stage14.2-release-smoke`
- `--stage14.3-frozen-smoke`
- `--public-api-usage-smoke`

Pass an argument through Visual Studio’s Debugging / Command Arguments setting, or run the built executable with that argument. The project is configured as a console application and its output name follows the project configuration. These runner options are development validation hooks, not a stable public command-line interface.

## AddressSanitizer note

On Windows, the ASan runtime DLL `clang_rt.asan_dynamic-x86_64.dll` may need its MSVC runtime directory present in the current process `PATH` when launching an ASan-built executable. This is an environment/runtime lookup note, not a LayerKeySort defect. For a one-off PowerShell launch, prepend the appropriate installed MSVC runtime directory to the current process only:

```powershell
$env:PATH = "<MSVC runtime directory>;$env:PATH"
./path/to/LayerKeySort.exe
```

This does not change the persistent system PATH. Do not obtain runtime DLLs from unofficial download sites.

## Development history

Older V2 policy decisions and discarded alternatives are preserved in
[design history](history/DESIGN_HISTORY.md). Earlier measurement comparisons
and policy trials are in [benchmark history](history/BENCHMARK_HISTORY.md).
They do not define the active V3 implementation or compatibility contract.

## Core engineering constraints

- Keep implementation in portable C17 under both build systems.
- Preserve public ownership and ordering semantics. Exact generated Path
  coordinates and private placement policy are implementation details; a
  compatible future 3.x release may change them while preserving documented
  public contracts.
- Preserve stable ordering and the documented ownership rules.
- Keep relevant regression coverage passing when behavior changes.

## Release documentation and presentation review

For every Stable release, review the complete public path from first visit to
first use: README, API, usage, integration, compatibility, development and
relevant migration guides, benchmark documentation, CHANGELOG, examples,
visual diagrams, interactive demos, GitHub Pages entry and navigation, release
notes, and relevant GitHub Issues. Read each section, classify it as current,
historical, stale, or obsolete, and verify that historical material is clearly
labeled and does not become the default presentation. Verify links and visual
claims against the released implementation. A Stable release is not complete
merely because its version numbers and CHANGELOG were updated.

Apply a scope-appropriate review to other releases and code changes too. Keep
Issue updates and release notes aligned with verified evidence; local-only
validation must not be presented as CI evidence. Documentation review alone
does not publish a release.

## Line endings

Do not normalize line endings as part of unrelated work. Windows Git configurations may report LF/CRLF conversion notices; review whether a change is substantive before altering files.
