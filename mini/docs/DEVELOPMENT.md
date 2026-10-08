# Development — v1.0.0-preview.1

Requirements: C17 compiler, CMake 3.16 or later and a matching build tool. No
third-party runtime libraries or sibling project sources are needed. The static
library target is `layerkeysort_mini`. CMake adds conventional compiler warnings;
the public test links that target, and the example includes only its public header.

From the Mini directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Windows MinGW PowerShell, when the toolchain is on PATH:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\lks_mini_basic.exe
```

Visual Studio uses its default generator and `--config Debug`; CMAKE_BUILD_TYPE
is for single-configuration generators. An optimized check may use a separate
Release build directory. Sanitizers can be enabled via compiler/linker flags on
platforms whose toolchains support them; do not assume sanitizer availability.

## Tests

- `mini_public`: lifetime, queries, all insertions/removals, forward/reverse order,
  size, stable live handles, duplicate/NULL payloads, NULL arguments and outputs,
  defaults, boundaries and valid foreign handles.
- `mini_internal`: private invariants after legal mutations, creation and basic
  insertion OOM, synthetic capacity limit, unchanged links on failure, no
  allocation on removal, allocation balance and caller-owned stack payloads.
- `mini_example`: build and execute the public API example.

CHECK macros stay active in Release, independent of NDEBUG. Internal source
inclusion and allocation substitution occur only in the isolated test target.
Do not introduce public diagnostic hooks. Never dereference a freed handle in
tests. This milestone does not include a systematic fault-injection campaign,
randomized reference model or expanded sanitizer campaign.

## Standalone extraction check

Copy the whole Mini directory, including LICENSE, to a new location outside the
parent repository, then configure/build/test the copy with a separate build
directory. Example PowerShell from a parent checkout (use new destination paths):

```powershell
Copy-Item -LiteralPath .\mini -Destination C:\Temp\lks-mini-preview1 -Recurse
cmake -S C:\Temp\lks-mini-preview1 -B C:\Temp\lks-mini-preview1-build -DCMAKE_BUILD_TYPE=Debug
cmake --build C:\Temp\lks-mini-preview1-build --config Debug
ctest --test-dir C:\Temp\lks-mini-preview1-build -C Debug --output-on-failure
```

All relative links and source references resolve inside the copied directory.
The test-only `../src/lks_mini.c` reference stays inside Mini.

## Contributions

Keep production simple, preserve the [contract](CONTRACT.md), and inspect the
complete diff before committing. Preview.1 changes are confined to this project
directory. Run ordinary and extracted builds/tests; record actual toolchain and
results. Do not advertise unavailable APIs. The fixed next milestone is Preview.2
(movement and comparison), followed by Preview.3 reliability, RC.1 integration
and Stable validation. Each requires separate authorization.
