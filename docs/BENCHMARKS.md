# V3 performance evidence

Current V4 scope is **experimental Preview.3**: [immutable snapshots, LS1, persistence, restoration and V3 import](V4_PREVIEW3.md), alongside [live moves and managed order](V4_PREVIEW2.md). Stable remains v3.1.0. Earlier milestone guides are historical records; V4 API/wire freeze is deferred to Preview.5.

The local V4 Preview.1 sanity screen is described in [V4 Preview.1](V4_PREVIEW1.md).
The captured V3 measurements below remain historical evidence and are unchanged;
they do not measure the new contextual live-order core.

The 3.1.0 source retains the published `v3.0.0` production ordering code and
benchmark harness. The capture below is not a new 3.1.0 timing run.
It describes one machine and the listed workloads; benchmark timing is not a
complexity proof or a universal speed claim. The source and benchmark harness
have no changes between the captured Preview.3 commit and the Stable tag.
Historical V2 and Preview comparisons are in
[benchmark history](history/BENCHMARK_HISTORY.md).

## Comparable V3 insertion matrix

RC.1 retains Preview.3's ordering algorithm. A one-machine validation at
100k–1m managed Tree insertions found substantial workload differences. The
[timed CSV](../benchmarks/results/v3-preview3-validation-timed.csv),
[diagnostic capture](../benchmarks/results/v3-preview3-validation-diagnostics.txt),
and [methodology](../benchmarks/results/v3-preview3-validation-metadata.md)
are retained for inspection. All rows below use the same source commit and
compiler; 1m timings are single runs and should not be read as stable medians.

| Insertion pattern | 100k ms | 1m ms | 1m full-range relabels | 1m final resident Path MiB | 1m estimated LK1 MiB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ascending | 54 | 2,428 | 1 | 112.5 | 53.8 |
| Descending | 36 | 411 | 0 | 45.8 | 14.3 |
| Random unique | 99 | 2,277 | 1 | 69.1 | 29.8 |
| All equal | 54 | 2,447 | 1 | 112.5 | 53.8 |
| 32-value duplicates | 109 | 5,007 | 2 | 96.2 | 40.3 |
| Alternating low/high | 1,655 | 44,023 | 56 | 98.5 | 40.8 |
| Fixed interior hotspot | 38 | 418 | 0 | 45.8 | 19.8 |

The alternating pattern repeatedly approaches both ends of the ordered
range. At 1m it caused 353,830 successful relabels and 69.4 million
cumulative old-coordinate replacements, including 56 full-range events.
The final Path depth still had mean 4.47 and maximum 6. This is a relabel
and allocation-churn cost, not evidence that final Path depth grew without
control. A separate per-insert probe observed pauses above 700 ms in this
pattern and above 1 second in the duplicate pattern; methodology and limits
are in the linked metadata. Applications requiring bounded or consistently
low synchronous insertion latency at hundreds of thousands of items should
measure their own workload or choose an ordering design with a suitable
guarantee. No worst-case `O(log n)` complete insertion or formal amortized
bound is claimed.

## Measurement boundary

The [capture metadata](../benchmarks/results/v3-preview3-validation-metadata.md)
records the source commit, Windows host, MSYS2 UCRT64 GCC 16.2.0, build flags,
seed, repetitions, and timed/diagnostic separation. The timed harness excludes
input preparation, output validation, and destruction. Runs at 100k and 300k
used one warmup and three repetitions; 500k used one warmup and two; 1m used
one run without warmup. The diagnostic build was separate from timed runs.
Its relabel and footprint counters are not extra timing observations. A
separate per-insert probe sampled tail latency; treat observed maxima as
workload evidence, not a guaranteed ceiling. The [harness guide](../benchmarks/README.md)
explains reproduction. Current V3 benchmarks include ascending, descending,
random, equal, duplicate, alternating, and fixed-hotspot patterns.

In diagnostics, `relabel.nodes_relabelled` counts old coordinates replaced by
successful managed insertions, while `max_region_nodes` records the largest
successful region. Footprint records final resident Path allocation, total
Path steps, and display/LK1 C-string bytes. These are separate from wall-clock
timing and do not establish an asymptotic bound.

## Complexity and limits of V3 ordering

Let `n` be the Tree size, `d` a Path depth, `b` an encoded text length, and
`c` the cost of the caller's item comparator. An AVL visit may also compare
Paths at cost up to `O(d)`; node visits alone are not whole-operation cost.
The table describes the current implementation, with output size included
where relevant. It is not a formal bound on every allocation or hardware
cost.

| Operation | Current structural work / important caveat |
| --- | --- |
| Path compare | `O(min(d1,d2))` step comparisons. |
| Display/key length and format | Iterative `O(d + b)` work, no library allocation for formatting. |
| Display/key parse | Iterative `O(b)` syntax work plus decoded Path allocation/growth; malformed input and overflow fail cleanly. |
| Explicit Tree find | `O(log n)` AVL visits, each Path comparison up to shared depth. |
| Explicit Tree insert | Same search plus clone of the supplied Path and `O(log n)` AVL rebalance; no repair policy. |
| Ordered Tree locate | `O(log n)` caller comparator calls under a consistent comparator. Returned nodes are borrowed. |
| Ordered Tree insert with direct gap | Upper-bound search and gap generation, then `O(log n)` Path-index insertion; costs include `c`, Path comparisons, and allocated candidate depth. Endpoint carry can scan/copy up to Path depth. |
| Tree remove | `O(log n)` Path-index search/rebalance plus Path comparisons and destruction; no allocation. |
| Tree rekey | Two Path-index searches and `O(log n)` structural work, plus clone/new-node allocation and old-Path cleanup; unchanged-coordinate success is a no-op. |
| Adaptive relabel | Starts with eight logical neighbors and doubles the region as needed. A successful `k`-node region prepares `k + 1` Paths, replaces `k` existing coordinates, and can reach all `n` nodes. Failed attempts also cost Path generation and allocation. |
| Full-range relabel | Prepares `n + 1` fresh Paths, then swaps the `n` old coordinates and links one node without rebuilding the physical AVL index. Peak memory includes old and prepared Paths and scratch arrays. |
| Group build | Stable sort uses `O(n log n)` comparator calls, then Path generation, Tree construction, and validation; Path work adds its own cost. |
| Group merge | Stable merge visits both ordered inputs linearly in item count, then builds/validates the result's fresh Path space. |
| `lks_sort` | Stable merge sort: `O(n log n)` comparator calls, `O(n)` pointer scratch, and `O(log n)` recursion depth. |

An individual managed insertion **does not** have a guaranteed `O(log n)`
time bound because adaptive relabel can touch every resident coordinate.
No formal amortized bound or fixed Path-depth/memory ceiling is claimed.
Historical V2 full-rebuild counts above are not V3 full-range relabel counts;
the separate V3 diagnostic counters report relabel frequency and affected
coordinates.
Database collation for `LK1:` must preserve bytewise ASCII order.
