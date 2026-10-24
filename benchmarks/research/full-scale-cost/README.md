# Full large-scale cost research

Full **v4.0.0 Stable**, internal engineering research, 2026-10-24.
No production algorithm, public header, existing test, fixture, root build/CI,
Issue, tag, Release or other product line is changed. No optimization candidate
is implemented. This subtree is not registered in root CMake.

Read [REPORT.md](REPORT.md), [RESULTS.md](RESULTS.md),
[METHOD.md](METHOD.md), and [CANDIDATES.md](CANDIDATES.md).

## Reproduce on Windows

From the repository root, GCC/CMake/Python on PATH:

```powershell
cmake -S benchmarks/research/full-scale-cost -B ../full-scale-build-release `
  -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build ../full-scale-build-release -j 8
ctest --test-dir ../full-scale-build-release -V
python -B -X utf8 benchmarks/research/full-scale-cost/collect.py `
  ../full-scale-build-release ../full-scale-evidence/final-capture
python -B -X utf8 benchmarks/research/full-scale-cost/collect_long.py `
  ../full-scale-build-release ../full-scale-evidence/long-capture
python -B -X utf8 benchmarks/research/full-scale-cost/analyze.py `
  ../full-scale-evidence/final-capture ../full-scale-evidence/long-capture `
  benchmarks/research/full-scale-cost
```

**Resource gate:** inspect the recorded planning envelope before rerunning.
The collector requires a 3 GiB available-memory reserve, limits its estimate to
40% of currently available RAM and at most 3 GiB, and runs one child at a time.
The child aborts on observed free memory below 2 GiB or matrix process time over
240 seconds; the collector kills a child after 270 seconds. Nothing disables OS
protections or deliberately exhausts memory. Those are research safety policies,
not Full capacity/latency guarantees. Other workloads on the machine can change
whether a run is admitted.

`collect.py --scales 100000` starts the pilot/warmups; later invocations can
resume with `--scales 1000000`, then `3000000`, then `10000000`. A completed
smaller append pilot is required before each step. Existing run IDs are retained,
not overwritten. A failed/budget-aborted run stops collection for inspection;
policy skips are recorded explicitly. Use a new output directory to recollect
with a changed binary/source. Build outputs stay outside the checkout.

Direct single case:

```powershell
../full-scale-build-release/full_scale.exe random 1000000 17
../full-scale-build-release/full_scale_alloc.exe append 1000000 17
../full-scale-build-release/full_scale_structural.exe anchor 1000000 17
../full-scale-build-release/full_long.exe 1000000 100
```

## Executables and isolation

| Executable | Library and observation path |
|---|---|
| full_scale | Unchanged ordinary production library; public APIs only |
| full_scale_alloc | Same production library, external GNU malloc/calloc/realloc/free interception |
| full_scale_structural | Unchanged sources with existing LKS_ENABLE_ALLOC_DIAGNOSTICS macro; private probe isolated in probe.c |
| full_long / full_long_alloc / full_long_structural | Separate 100-cycle lifetime campaign using the same respective isolation |
| oracle_probe | Independent ID sequence only; does not link Full |

Production timing never links the diagnostic library or allocator wrapper.
Private measurements are not ordinary consumer evidence or a new public ABI.
`oracle.c/.h` has no Full dependency. The new array-linked oracle is independent
of Full's bounded blocks and AVL index; Python lists test 20k C-oracle
transitions, including an explicit checker-corruption rejection.

The main driver has a POSIX clock branch but fresh measured execution is
Windows only. The safety collectors and focused long driver are Windows-only.
No unsupported platform result is implied. MSVC can build the public/diagnostic
drivers with NMake Makefiles in its Developer Command Prompt; GNU wrapper
targets are deliberately absent there. `-DRESEARCH_ASAN=ON` instruments the
library and research source on supported compiler/runtime combinations; use
only small test sizes under sanitizers.

## Workload vocabulary

All workloads are deterministic synthetic profiles, not human traces.

- `append`: insert every new occurrence at the back.
- `prepend`: insert before the most recently inserted occurrence, hence front.
- `anchor`: insert every new occurrence before occurrence 0 after the first.
  This is a fixed-anchor region, not a claim of an interior rank hotspot.
- `local`: relative insertion before one of the 32 most recent arrival IDs.
  Arrival recency is **not** a strict logical-rank-distance guarantee.
- `ascending` / `descending`: managed arrivals with monotone integer keys.
- `random`: managed arrivals with deterministic 64-bit pseudorandom keys.
- `duplicates`: managed arrivals mapped to 64 key values.
- `equal`: every managed arrival has the same comparison key.
- `history`: append build, two retained captures, serialization, load, restore
  into a second live source, then cleanup; only <=1m residents.

Each explicit-order growth profile is followed by the same editing phases:

1. Local move before the current second successor (one neighbor crossed);
   missing successor means move-back, including legal no-ops.
2. Move random residents before a fixed resident `N/2`.
3. Alternate move-front/move-back of random residents.
4. Move a random resident before another independently selected resident.
5. Uniform remove/reinsert trace; reinsert before a selected anchor.
6. Hotspot churn: remove current first, insert before fixed resident `N/2`.
7. Replay the uniform churn trace twice more on its evolving order.
8. Remove every even occurrence ID, verify N/2 survivors, append those IDs again.

Each move phase has min(N, 200k) calls; each churn phase has that many pairs,
twice as many API calls. Expected commands and oracle transitions are prepared
before each timed phase. Reinserted occurrences get fresh handles; surviving
ones keep their original pointer values. Retired handles are never probed.

Managed locate prepares 4,096 queries: three quarters drawn from resident keys,
one quarter UINT64_MAX, absent from these generated corpora. An independent
sorted ID sequence computes predecessor/first-equal/first-greater outputs.
The timed loop replays min(N,200k) lookups and checks all three results.

The focused long campaign performs 100 x 100k remove/reinsert pairs per process
at 100k and 1m residents: **20 million public mutation calls per process**. The
PRNG continues across cycles. The same ID may be removed/reinserted repeatedly;
payload ownership stays with the driver. Counts stay fixed.

## Raw evidence

`evidence/observations.csv` retains all final matrix phase rows, including
warmups and holdouts identified explicitly. `summary.csv` excludes both from
primary aggregates. `runs.json` retains pre/post memory, estimates, commands,
status, time and repetition. `checkpoints.csv` records complete sequence
digests and exact verification status. `long-churn.csv` retains all 100 cycles
from every long process. `SHA256.json` hashes committed evidence bytes.

Individual stdout/stderr, pilot captures, binaries and caches remain outside
the repository; [PROVENANCE.md](PROVENANCE.md) documents paths and hashes.
Do not read instrumented timings as production timing or zero unavailable
counters as measured zero. The CSV uses `NA` for unavailable observations.
