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

## Toolchain

The checked-in project configures Microsoft Visual Studio / MSVC, x64, and C17. This is the toolchain currently validated by the repository. Do not infer support for other compiler/platform combinations from the C source alone.

## Build configurations

The Visual Studio project defines **Debug**, **Release**, and **ASan** configurations for x64. Open `LayerKeySort.slnx` in Visual Studio and select the configuration and x64 platform before building.

## Running the validation runner

The current executable entry point is `demo/main.c`. With no argument, Debug runs the final Debug validation path and then the deterministic property tests. Non-Debug builds run the Release smoke path and then the deterministic property tests.

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

- Keep implementation in C17 under the existing project configuration.
- Preserve public API and Path semantics.
- Preserve stable ordering and the documented ownership rules.
- Keep relevant regression coverage passing when behavior changes.

## Line endings

Do not normalize line endings as part of unrelated work. Windows Git configurations may report LF/CRLF conversion notices; review whether a change is substantive before altering files.
