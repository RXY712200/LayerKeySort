# Full Live-Order Step 1 — compact resident candidate

This directory is a bounded implementation/evidence delivery, not an adopted
release or a replacement for independent review. See [REPORT.md](REPORT.md).
Public Full version, ABI-opaque API and wire formats are unchanged.

## Reproduce

Use Windows x64, CMake 3.21+, Python 3 and GCC/MinGW. Create an immutable
baseline checkout at `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`. The collected
baseline used the existing research checkout at `d9d3b370fb8daf134eadc26c64e3b30aae12ec75`;
its production sources/headers/manifest are byte-identical to that main commit.
Build both variants with this **same** harness and the same toolchain:

```powershell
cmake -S benchmarks/research/live-core-step1 -B ../step1-base -G "MinGW Makefiles" -DFULL_ROOT=C:/absolute/baseline -DCMAKE_BUILD_TYPE=Release
cmake -S benchmarks/research/live-core-step1 -B ../step1-candidate -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build ../step1-base --parallel
cmake --build ../step1-candidate --parallel
ctest --test-dir ../step1-candidate --output-on-failure
python -B benchmarks/research/live-core-step1/compare.py ../step1-base ../step1-candidate ../step1-capture
python -B benchmarks/research/live-core-step1/analyze.py ../step1-capture ../step1-analysis
python -B benchmarks/research/live-core-step1/compare.py ../step1-base ../step1-candidate ../step1-long --scales 100000,1000000 --long
python -B benchmarks/research/live-core-step1/analyze.py ../step1-long ../step1-long-analysis
../step1-base/full_shrink.exe
../step1-candidate/full_shrink.exe
gcc -std=c17 -O3 -Wall -Wextra -Wpedantic -Werror benchmarks/research/live-core-step1/storage_screen.c -o ../storage_screen.exe
../storage_screen.exe
```

Set `CMAKE_C_COMPILER` and `CMAKE_MAKE_PROGRAM` explicitly if not in PATH.
`FULL_ROOT` must be absolute. Do not compare results from differing toolchains
or compiler flags. `RESEARCH_ASAN=ON` supports the separate MSVC small-size
oracle campaign; run from a VS developer environment so the ASan DLL is found.

For each paired matrix case, variant order alternates deterministically. Two
fresh processes per production workload/scale are planned. Allocation wrapping
and structural instrumentation are separate runs, never production timings.
Admission uses a conservative envelope, 40% of currently available physical
memory, at most 3 GiB, while reserving 3 GiB. Skips are recorded as skips. Upper
scales and history are not permission to cause paging or to terminate unrelated
applications. History is capped at 1m to limit simultaneous retained states.

## Workload and provenance

`study.c`, the independent ID-linked `oracle.c/.h`, `oracle_probe.c`,
`oracle_test.py`, `wrap.c`, `probe.h` and `long_churn.c` are copied from the
immutable million-scale study above. `probe.c` only updates its provenance
comment. `CMakeLists.txt` adds a source-root parameter and the shrink driver.
The prior raw evidence is untouched. Workload semantics, generator, seed17,
sample strata, operation sequences, independent checkpoints and measurement
boundaries are preserved. The oracle's separate Python-list check is retained.

- append/prepend: corresponding explicit endpoint insertion;
- anchor: repeated insertion at a fixed stable anchor;
- local: recent-arrival relative insertion;
- managed ascending/descending/random/equal/duplicates: stable comparator order,
  random uses the fixed generator, duplicates use 64 keys;
- managed locate: 4096 precomputed queries, one quarter absent, up to 200k calls;
- explicit edits: local/hotspot/front-back/uniform moves; uniform and hotspot
  remove/reinsert; half deletion/regrowth, each with an independent full-order
  checkpoint;
- history: captures, wire serialization/load, resolve-first fresh restore and
  teardown. No snapshot representation or callback change;
- long:100 cycles of up to 100k remove/reinsert pairs, 100k and 1m residents,
  exact checkpoints initially, every tenth cycle and finally;
- shrink: one million residents, deterministic shuffled removal to 10% and 1%,
  regrowth and destruction, exact identities/order checked at each stage;
- storage screen: independently allocated stable-address slab records of 16/64/256
  slots; immediate empty-slab reclamation. This is a **resident-storage-only**
  rejection screen, not a fully integrated pool implementation or throughput
  comparison. It cannot establish pool insertion performance or OOM correctness.

All are generated engineering workloads, not human sessions. Driver loops,
status checks and periodic budget checks are inside phase wall time; generating
the expected model is outside. Pair samples bracket two API calls. Quantiles
are observed samples, not worst-case bounds; the Windows timer can quantize
short observations to zero. Resident arrays/payloads/oracle/trace are application
memory, excluded from Full requested bytes but included in working set.

`evidence/` preserves gzip JSON Lines raw stdout/stderr, process commands/admission
metadata and test logs. `results/` includes every completed phase, all production
comparison rows and independent checkpoint verification. Exploratory runs that
overlapped builds are explicitly separated from the final confirmation campaign.
`SHA256SUMS.txt` covers canonical Git blob bytes of the delivered assets and changed code except itself (checkout newline conversion is not part of those hashes).
Each campaign's `raw.jsonl.gz` contains one JSON record per completed process,
including its ID, stdout and stderr; stdout is CSV. The analyzer accepts both
these compact bundles and the collector's individual gzip files. For example:

```powershell
python -B benchmarks/research/live-core-step1/analyze.py benchmarks/research/live-core-step1/evidence/confirmation ../recheck
```

No network/package dependency is needed to analyze.
