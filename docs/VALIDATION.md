# V4 final freeze validation

With benchmarks/Python: 38 CTest tests, retaining all 34 Preview.4 regressions plus
typed all-API compile/link, public freeze behavior, manifest and exact wire goldens.
Old V3 tests use private declarations; remain migration/regression evidence.

Required: strict GCC C17 Release, MSVC x64 Debug/Release with symbols/ASan/Win32;
Linux GCC/Clang/Clang ASan+UBSan; macOS AppleClang. CI runs seven public examples,
C/C++ source/offline FetchContent/install/amalgamation consumers. Packaging job
verifies deterministic assets, hashes/membership/corruption rejection.
Manifest/goldens cannot silently change during RC. OOM strong guarantees remain.
Concurrent snapshot/marker tests use protected lifetime, not mutable thread safety.
Local default MSVC Release disappearance is unconfirmed external behavior;
known-good Release with symbols validates packaging. Local unavailable Clang/GCC
sanitizer libraries are reported separately from verified remote safety gates.
