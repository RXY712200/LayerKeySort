# Validation commands and scope

Executed on the candidate in a separate worktree. Build/capture directories are
outside the checkout. `evidence/validation/` preserves the resulting test logs;
absolute paths identify the actual Windows environment, not required install paths.

```powershell
cmake -S . -B ../live-core-step1-gcc -G "MinGW Makefiles" -DCMAKE_C_COMPILER=D:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=D:/msys64/ucrt64/bin/mingw32-make.exe -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON "-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror"
cmake --build ../live-core-step1-gcc -j 8
ctest --test-dir ../live-core-step1-gcc --output-on-failure -j 4
```

The same GCC configuration was repeated in `../live-core-step1-gcc-debug` with
`CMAKE_BUILD_TYPE=Debug`. Both configurations ran39/39 tests. The focused
`layerkeysort_v4_tests.exe` output records the new133-point bulk OOM sweep.

```powershell
cmake -S . -B ../live-core-step1-msvc -G "Visual Studio 18 2026" -A x64 -DLKS_BUILD_BENCHMARKS=ON
cmake --build ../live-core-step1-msvc --config Debug -j 8
ctest --test-dir ../live-core-step1-msvc -C Debug --output-on-failure -j 4
cmake --build ../live-core-step1-msvc --config Release -j 8
ctest --test-dir ../live-core-step1-msvc -C Release --output-on-failure -j 4
cmake -S . -B ../live-core-step1-asan -G "Visual Studio 18 2026" -A x64 -DLKS_BUILD_BENCHMARKS=ON "-DCMAKE_C_FLAGS=/fsanitize=address /Zi" -DCMAKE_EXE_LINKER_FLAGS=/INCREMENTAL:NO
```

For ASan build `RelWithDebInfo`, then invoke:

```powershell
cmd /c '"D:\vs\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && ctest --test-dir ..\live-core-step1-asan -C RelWithDebInfo --output-on-failure -j 3'
```

The initial ASan run from an ordinary shell failed to load the runtime DLL
(`0xc0000135`); it was not a passed test. The developer-environment rerun passed
39/39. The research harness additionally used `RESEARCH_ASAN=ON`, MSVC
RelWithDebInfo, and the same environment, passing 12/12 including the separate
Python-list oracle check. Large-scale timing runs were unsanitized.

```powershell
python -B tools/verify_v4_api.py
python -B tools/verify_v4_wire.py ../live-core-step1-gcc/layerkeysort_v4_wire_probe.exe
python -B -X utf8 tools/validate_distribution.py --generator "MinGW Makefiles" --c-compiler D:/msys64/ucrt64/bin/gcc.exe --cxx-compiler D:/msys64/ucrt64/bin/g++.exe --make-program D:/msys64/ucrt64/bin/mingw32-make.exe
Get-ChildItem ../live-core-step1-gcc/layerkeysort*example.exe | ForEach-Object { & $_.FullName; if ($LASTEXITCODE -ne 0) { throw $_.Name } }
git diff --check
```

Distribution validation builds C17/C++ source consumers with add_subdirectory
and FetchContent, installed-package consumers, deterministic twice-generated
amalgamation packages and their consumers/examples. Production source membership
is unchanged; the generated source incorporates both modified C modules.

GCC 16.2 sanitizer linking was probed and failed because `-lasan` and `-lubsan`
are unavailable. No local Clang compiler or installed WSL distribution was found.
VS's LLVM directory contains formatting/analysis tools, not a compiler. No new
platform installation was attempted. Remote CI evidence, if obtained after
delivery, is reported separately for the exact pushed commit; it is not implied
by local MSVC validation. Win32/macOS/Linux are not local results.

No public manifest/header, LS1/LKS4SNP1 fixture, V3 import grammar or callback
algorithm was edited. Existing tests cover Groups/Batch, marker lifetime,
callbacks/reentry, cursors/revisions, migration, restore resolution, and mutation
OOM. The new test strengthens shared block ownership across bulk construction,
cross-block moves, merges and foreign **live** handle rejection. There is no new
fallible preparation path and no stale-pointer registry.
