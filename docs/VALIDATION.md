# V4 RC validation

With benchmarks/Python: 39 CTest tests, retaining all 38 Preview.5 gates plus
RC hostile-input and allocation-free checks. Existing gates include
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

RC extended runs: layerkeysort_v4_mutation --long (three 200k-step seeds),
layerkeysort_v4_rc_torture --long (three 20k-iteration seeds), large structural
traces and focused Preview.4 counter comparison. The Clang sanitizer job runs
long mutation/parser, 100k distant moves and 200k churn. See [RC record](V4_RC1.md).
