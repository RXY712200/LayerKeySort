# Reproduction and evidence verification

All commands below are for independent reproduction, not requests for the owner
to type commands. The completed execution logs are in `evidence/validation/`.
Use separate build/source directories; do not overlap local timing with builds.

## Full tests and examples

From the integrated checkout, the actual strict GCC builds used:

```powershell
cmake -S . -B ../step3-gcc-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON "-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror"
cmake --build ../step3-gcc-release --parallel 4
ctest --test-dir ../step3-gcc-release --output-on-failure
```

Repeat in a different directory with Debug. MSVC used `Visual Studio 18 2026`,
`-A x64`, and separate Debug/Release builds with39/39 each. The ASan configuration
uses `/fsanitize=address /Zi` and `/INCREMENTAL:NO`, RelWithDebInfo. Run its CTest
from the Visual Studio developer environment. The first failed launcher quoting
is preserved separately from the successful39/39 LastTest log. Research ASan
uses the wrapper CMake project below with `-DRESEARCH_ASAN=ON`:21/21.

```powershell
python -B -X utf8 tools/verify_v4_api.py
python -B -X utf8 tools/validate_distribution.py
python -B -X utf8 tools/validate_distribution.py --generator "MinGW Makefiles" --c-compiler D:/msys64/ucrt64/bin/gcc.exe --cxx-compiler D:/msys64/ucrt64/bin/g++.exe --make-program D:/msys64/ucrt64/bin/mingw32-make.exe
python -B -X utf8 tools/validate_distribution.py --generator "Visual Studio 18 2026" --architecture x64
```

The initial default implicit-Ninja failure and both corrected paths are archived.
The correction always supplies Debug to match the tool's build/install config.
Seven executable examples were run: basic, ordered tree, layer list, live order,
managed order, snapshot and immutable Group snapshot. Full root CI additionally
runs every example and all public/distribution consumers on six configurations.

No local Clang or GCC sanitizer runtime pass is claimed. See the archived fresh
GCC link failure. Real remote Clang ASan/UBSan, extended campaigns and native
Win32/macOS checks are in inspected CI run40170420306;39/39 per configuration.

## Fresh A/B/C research

A root is exact043872e; B is exact19ca51b; C production is exact291305d. Preserve
original worktrees. Create detached read-only source checkouts as needed. From
this integrated checkout build the SAME wrapper against each absolute root:

```sh
cmake -S benchmarks/research/final-step3 -B ../step3-a -DFULL_ROOT=/absolute/root-a -DCMAKE_BUILD_TYPE=Release '-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror'
cmake --build ../step3-a --parallel 4
ctest --test-dir ../step3-a --output-on-failure
../step3-a/full_layout
```

Repeat B/C before timing. Windows used explicit MinGW generator and .exe suffix;
measured executables are under each build's `history-harness/`. Compile flags,
source hashes and measured Windows executable/object-text hashes are archived.

```sh
python benchmarks/research/final-step3/collect.py ../step3-a ../step3-b ../step3-c ../step3-results
```

Optional `--suites live,history,long,shrink,upper` selects a subset. This does not
change operation definitions. Logs never overwrite prior attempts; use fresh
output directories for a genuinely new round. The fixed plan has792 attempts
per host. `plans()` defines repetitions, scales, payloads and workload names.
Every run records command, UTC, memory admission/readings, status and raw output.
All A/B/C runs use seed17 where the unchanged driver takes a seed.

Admission: min(3 GiB,40% available),3 GiB reserve, existing driver2 GiB guards,
360-second child timeout (the scale driver additionally has a240-second guard).
Do not lower these or kill unrelated processes. A skip/abort is not successful
coverage. Production timings, allocator-wrapper accounting and private structural
observation are separate fresh processes. No10m sanitizer/high-retention demand.

## Analyze and independently check data

Repeat for each suite using fresh output directories:

```sh
python benchmarks/research/final-step3/analyze.py benchmarks/research/final-step3/evidence/windows/capture-live ../recomputed/analysis-live
python benchmarks/research/final-step3/verify/audit.py benchmarks/research/final-step3/evidence/windows ../windows-audit.json
python benchmarks/research/final-step3/verify/audit.py benchmarks/research/final-step3/evidence/linux ../linux-audit.json
python benchmarks/research/final-step3/verify/tables.py benchmarks/research/final-step3/evidence ../TABLES.md
```

Compare TABLES.md and ADVERSE.csv after text-newline normalization. Raw capsule
hashes must instead match exact bytes. The analyzer reads compact raw.jsonl.gz
or individual raw files; it verifies cross-source oracle fingerprints and final
cleanup before producing phase distributions. `verify/audit.py` independently
checks work counts, x64 resident formulas, capture metadata/capacity and shrink
reclamation. It deliberately distinguishes the model count retained after
teardown from live resident count, and skips nonexistent legacy storage columns.

The Windows and Linux raw capsules contain all attempts, not just favorable
medians. All22 downloaded Linux analysis outputs were regenerated successfully;
JSON newline differences across operating systems were normalized only for
comparison. Timed-phase totals include calibration/source preparation; do not
rename them pure history cost. No allocator copy/cache attribution is supplied.

## Checksums

Each capsule retains its original SHA256SUMS.txt. Its local .gitattributes preserves
native output bytes (including CSV CRLF). The additional attributes file is
transport metadata, not an original runner output. Verify any inner manifest
using bytes from `git show HEAD:<path>` to avoid user Git checkout conversions.
The downloaded Linux artifact outer SHA256/id/run are in artifact-provenance.json;
all its permanent inner evidence is committed, so the evidence does not depend
on90-day artifact retention.

The top-level SHA256SUMS.txt is broader: it hashes canonical committed Git blobs
for the entire original-main..delivery changed-file inventory, excluding itself.
From the repository root:

```python
import hashlib, subprocess
from pathlib import Path
base = Path('benchmarks/research/final-step3')
for line in (base/'SHA256SUMS.txt').read_text().splitlines():
    expected, path = line.split('  ', 1)
    data = subprocess.check_output(['git', 'show', 'HEAD:'+path])
    assert hashlib.sha256(data).hexdigest() == expected, path
```

FILES.txt covers only Step 3 since accepted291305d. INTEGRATED_FILES.txt covers
original043872e..delivery, including both earlier accepted steps. No history was
rewritten. Captured code/CI tests are engineering evidence, not human acceptance.
