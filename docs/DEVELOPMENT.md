# Developing LayerKeySort

## Repository layout

- `include/` — public header.
- `src/` — production C implementation and private headers.
- `tests/` — property, stress, benchmark, and public API validation code.
- `examples/` — standalone public API example.
- `demo/` — executable validation/demo entry point.
- `docs/` — user documentation, visualizer, and site assets.
- `LayerKeySort.slnx` — Visual Studio solution.
- `LayerKeySort.vcxproj` — Visual Studio project and build configurations.
- `CMakeLists.txt` — reusable production library, example, and test targets.
- `.github/workflows/ci.yml` — MSVC, GCC, and Clang validation.

## Toolchain

Both Visual Studio and CMake compile as C17. The CMake matrix also targets
GCC and Clang on Ubuntu. New production C source belongs in both build systems.

## CMake build and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`layerkeysort` contains production source only. `layerkeysort_example` links
that library. `layerkeysort_tests` links a separately compiled diagnostic
variant so allocation counters and single-shot fault injection remain usable
without shared mutable instrumentation in ordinary production allocations.
For a supported GCC or Clang toolchain, `-DLKS_ENABLE_SANITIZERS=ON` enables
AddressSanitizer and UndefinedBehaviorSanitizer. Diagnostic fault injection is
process-global test state and carries no concurrent-test guarantee.

## Build configurations

The Visual Studio project defines **Debug**, **Release**, and **ASan** configurations for x64. Open `LayerKeySort.slnx` in Visual Studio and select the configuration and x64 platform before building.

## Running the validation runner

The Visual Studio executable entry point is `demo/main.c`. With no argument it
runs the V2 focused tests, public API smoke, deterministic property tests,
stress tests, and OOM tests. The CMake test executable uses `tests/v2_main.c`
for the same preview suite. `tests/benchmark.c` preserves the historical v1
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

## Validation coverage

The repository contains public API smoke coverage, deterministic property tests, stress tests, allocation-failure and out-of-memory tests, and frozen regression checks. The Visual Studio project provides Debug, Release, and AddressSanitizer builds. The committed project also contains older development entry points and benchmark code; their presence does not make them public API.

## Coding constraints

Preview.2 online repair policy lives in `src/lks_policy_internal.h`. The
smallest eligible complete positive subtree is rebuilt off-Tree with the same
sparse bulk layout; enclosing ancestors are considered within a fixed node
limit. The root Path of the selected subtree remains fixed. All new Paths,
nodes, and child storage are ready before an allocation-free pointer splice.
The full rebuild remains the final fallback. Repair counters are process-wide
only in diagnostic builds; normal production builds do not update them.

- Keep implementation in portable C17 under both build systems.
- Preserve public ownership and ordering semantics; generated Path coordinates
  can change when the preview allocator policy changes.
- Preserve stable ordering and the documented ownership rules.
- Keep relevant regression coverage passing when behavior changes.

## Line endings

Do not normalize line endings as part of unrelated work. Windows Git configurations may report LF/CRLF conversion notices; review whether a change is substantive before altering files.
