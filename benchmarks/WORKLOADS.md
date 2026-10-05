# Workload simulator methodology

This is additive **synthetic application-like workload** evidence for released
v3.1.0. It is not a real-user trace and changes no production algorithm.
The source is `workload_simulator.c`; `run_workloads.py` uses Python's standard
library. CMake builds it only when `LKS_BUILD_BENCHMARKS=ON`.

## Profiles (frozen before evidence capture)

Each resident item has a signed 64-bit synthetic key, a unique item ID, and a
fresh insertion sequence. The public comparator compares only the key. Equal
keys must retain sequence order. A key-query removal selects the first/oldest
resident item with that key. Thus selection by a randomly chosen active key is
bucket-weighted, not uniform over equal item identities. Updates remove that
item, change its key while absent, and reinsert it at the end of its new equal
run. No resident comparator-visible field is modified.

| Family | Operation model | Key model | Population |
|---|---|---|---|
| `timeline` | 70% near-end new, 15% tail backfill, 5% broad new, 10% remove | Start at `i*1024`; near-end is historical high-water + uniform integer 1..1024; backfill subtracts 1..65536 from that high-water; broad spans 0..high-water | Grows about 0.8 items/operation |
| `priority` | 35% new, 25% remove, 40% remove/change/reinsert | Start and new keys in 0..31; new priority also in 0..31 | Grows about 0.1 items/operation |
| `local` | 10% new, 10% remove, 80% remove/change/reinsert | Start at `i*1048576`; new key near a random resident by signed 1..8 initial spacings; 90% updates move the removed key by signed 1..8 spacings; 10% jump near another resident by signed 128..1024 | Roughly steady |
| `churn` | 30% new, 30% remove, 40% remove/change/reinsert | Start at `i*1048576`; normal new keys span 0..initial*1048576; 5% new / 10% update proposals move one spacing beyond a historical extreme | Steady target with +/-5% band |

These percentages are modeling assumptions, not application statistics.
Timeline resembles delayed event arrival; priority exercises stable duplicate
buckets with changes; local models numeric-distance reordering, **not exact rank
moves** or every editor; churn exercises long mutation history. The churn band
redirects a new/removal proposal at the upper/lower boundary; all families force
an insert below two residents. Actual operation counts are recorded.

Every operation consumes one deterministic xorshift32 trace. Even unused key
proposals consume PRNG draws; exact reproduction uses the implementation and
scenario arguments, not percentages alone. Modulo mapping has small bias and
32-bit resolution; these are synthetic distributions. Key arithmetic and CLI
sizes are checked. Seeds must be nonzero.

The seven controls mirror `current_benchmark.c` without modifying that file:
ascending `i`, descending `N-i`, random Fisher-Yates of `i`, equal `0`, duplicates
`xorshift32%32`, alternating `i` even: `i/2`, odd: `N-1-i/2`, hotspot first `0`
then `N-i`. Here `N=initial+operations`; initial is an untimed prefix. They are
insertion-only; mixed application cases are therefore not identical operation
mixes even when the initial size and measured history length match.

## Timing and correctness

Initial construction is untimed warmup; no measured mutations are discarded.
Only `lks_ordered_tree_insert()` is inside the per-insert timer. Lookup, removal,
key generation, oracle sorting, profiling, sample processing, and orchestration
are excluded. Update latency means **reinsertion latency**, not total update
latency. Whole-process wall time is retained separately, including all overhead.

The C17 `timespec_get(TIME_UTC)` epoch-millisecond method is identical to the
existing benchmark. It is a wall clock, not monotonic; negative intervals abort.
Double precision near this epoch has about 0.000244 ms granularity; actual clock
resolution and scheduler effects can be larger. Zeros/submicrosecond percentiles
must not be interpreted as reliable differences. Quantiles use nearest rank.
P99.9 is emitted only for >=10,000 insertions (at least ten upper-tail samples).
Maxima are observations, never latency guarantees.

Production and diagnostic executables run identical traces separately and
sequentially (production first). Trace and checkpoint coordinate digests must
match. Diagnostic latency includes allocation/counter instrumentation; it is
**not production performance**. Operation indices allow cross-build event joins,
but timings are separate executions and not simultaneous measurements.

At operation zero, fixed operation intervals, and the end, a flat reference
array sorts by key then insertion sequence independently of Path encoding and
AVL shape. Checks cover size, every identity, dense membership of every created
item, stable equal order, strict Path order, and 32 first-equal locate/exact-Path
find probes. This is periodic validation, not an invariant check after every
operation. The CI oracle self-test deliberately corrupts equality order and
membership and verifies rejection. Failure prints complete scenario arguments
and the checkpoint operation; a fault between checkpoints may require finer
checkpoint spacing to locate its first occurrence.

## Work deltas, sampling, and memory

For each diagnostic insertion, subtract cumulative private repair counters
before/after. `nodes_relabelled` is the number of **existing** nodes whose
coordinates were replaced. Full-range is a separate flag; the relabel histogram
uses disjoint bins: direct, 1..8, 9..32, 33..128, 129..512, 513..4096, 4097+,
full-range. Local means <=128 existing nodes; larger means >128, unless full.
Attempted nodes is a **sum over attempted windows**, not the largest attempted
window. Running maxima are never subtracted as if they were per-event counts.
Inserted depth is the committed new Path depth, not rejected-candidate depth.

Normal output retains the union of worst 25 latencies, worst 25 relabel sizes,
and first 5 full-range events (<=55 diagnostic / <=25 production rows/run).
`sample_flags` bits 1, 2, 4 identify these selections. C `--trace` emits all
inserts for investigations; it is excluded from normal evidence files.

Checkpoint profiling visits every resident Path. Depth mean/P95/P99/max,
display/LK1 mean/max, resident Path bytes, and total LK1 payload bytes are
recorded. Lengths exclude NUL; C-string storage needs one extra byte/item.
Resident Path bytes include objects plus allocated step capacity (not only used
steps). Diagnostic allocation totals cover **library allocations only**, not
simulator arrays, Python memory, allocator headers, or process RSS. Requested
bytes are cumulative traffic; live/peak bytes are simultaneous requested library
memory. Setup is included in cumulative totals; measured-history deltas exclude
it. Destroy must return library live bytes to zero. A high traffic total is not
a leak.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --parallel
python benchmarks/run_workloads.py --timed build/layerkeysort_workload \
  --diagnostic build/layerkeysort_workload_diagnostics --smoke
python benchmarks/run_workloads.py --timed build/layerkeysort_workload \
  --diagnostic build/layerkeysort_workload_diagnostics --campaign \
  --output /path/to/new-results --compiler gcc --machine "describe machine"
```

Windows uses `.exe`, and MSVC multi-config binaries use `build/Release/`.
Diagnostic targets require `LKS_BUILD_TESTS=ON` (the top-level default).
The driver refuses to overwrite captured files and requires a committed
simulator. Full campaign definitions are in `campaign_plan()`: two small seeds,
five primary medium seeds, one large seed, selected long churn, all seven
controls at small/medium, and reserved holdout medium/long cases last. CI uses
only 128 initial items and 512 operations, plus malformed CLI/output checks.

See [the captured evidence report](../docs/WORKLOAD_EVIDENCE.md) for results and
limitations. No performance conclusion should be drawn from smoke cases.
