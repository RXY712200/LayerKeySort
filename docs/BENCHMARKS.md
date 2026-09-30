# V2 performance and benchmark evidence (Preview.4 release candidate)

The published release is **v2.0.0-preview.3**; **v2.0.0-preview.4** is a
release candidate on the development branch. The Stage 4 and Stage 5 columns
below are historical development snapshots. These numbers describe one machine and
the stated workloads; they are not universal performance rankings. Generated
Path coordinates and private policy values remain implementation details.

## Reproduction and measurement boundary

The public-API harness is in [`benchmarks/current_benchmark.c`](../benchmarks/current_benchmark.c),
with a matrix runner in [`benchmarks/run_matrix.ps1`](../benchmarks/run_matrix.ps1).
Its [README](../benchmarks/README.md) defines each interval and its correctness
checks. The optional CMake target uses `-DLKS_BUILD_BENCHMARKS=ON`; three small
CTest cases exercise the harness. The timing binaries omit diagnostic counters.

Measurements used Windows 11 Pro `10.0.26200`, an AMD Ryzen 9 9955HX
(16 cores, 32 logical processors), approximately 15.2 GiB RAM, AC power,
and MSYS2 UCRT64 GCC 16.2.0. All three versions used the same compiler and
`-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`. Each CSV row has one
warmup, seven measured repetitions, and the median/minimum/maximum in
milliseconds. Input generation and output validation occur outside the timed
interval; Tree creation and final destruction are outside Tree insertion
timings. Inputs use deterministic seed `0x91A30D47`. Repeat runs used
`0xDEADBEEF` as a holdout. The comparison checkouts were detached copies of:

| Label | Commit |
| --- | --- |
| Preview.3 | `9fa2eadcca422be9483a050505f0c332558c5908` |
| Stage 4 | `4ef9e240ab08edbb7875c0b2934313c126307e73` |
| Stage 5 | `99f84ae5228ea67ade74eaf206b17dc5e6304ffe` |

The `Common` matrix has 30 rows usable by Preview.3; `Current` has 61 rows.
The exact captured rows are committed as
[`preview3.csv`](../benchmarks/results/preview3.csv),
[`stage4.csv`](../benchmarks/results/stage4.csv), and
[`stage5.csv`](../benchmarks/results/stage5.csv); each row includes its
minimum and maximum as well as its median. The harness and runner can regenerate
them, but ordinary machine variation prevents byte-identical timing values.
Preview.3 has no remove, rekey, display parser, or durable-key API, so those
cells have no historical comparison. Millisecond-scale differences near the
clock/noise floor should not be treated as improvements. Scheduling, cache,
power behavior, and allocation state were not controlled as in a laboratory.

## Results

Median milliseconds, lower is better. `equal` means every item compares
equal; `duplicates` draws from a small value set. `alternating` repeatedly
inserts around two ends of the value range.

| Public operation / distribution | N | Preview.3 | Stage 4 | Stage 5 |
| --- | ---: | ---: | ---: | ---: |
| Tree insert / ascending | 10,000 | 8.122 | 5.635 | 5.245 |
| Tree insert / descending | 10,000 | 7.680 | 3.754 | 3.359 |
| Tree insert / random | 10,000 | 11.743 | 10.197 | 9.806 |
| Tree insert / equal | 4,000 | 937.480 | 1.517 | 1.354 |
| Tree insert / duplicates | 4,000 | 81.190 | 6.822 | 6.572 |
| Tree insert / alternating | 4,000 | **65.951** | 75.359 | 77.934 |
| Tree insert / two distinct values | 4,000 | 279.084 | 4.804 | 4.611 |
| Tree insert / 64 distinct values | 4,000 | 45.347 | 4.468 | 4.395 |
| Group build / random | 10,000 | 6.353 | 2.550 | 2.617 |
| Group merge / duplicates | 10,000 | 5.827 | 2.001 | 2.000 |
| GroupBatch build / duplicates | 10,000 | 3.674 | 1.921 | 1.929 |
| GroupBatch merge / duplicates | 10,000 | 5.814 | 2.151 | 2.152 |
| `lks_sort` / random | 10,000 | 0.679 | 0.666 | 0.661 |
| C `qsort` / random | 10,000 | 0.645 | 0.642 | 0.654 |

At 100,000 ascending Tree insertions, Stage 4 took 2,120.870 ms and Stage 5
took 319.905 ms; all-equal took 2,137.550 and 340.463 ms respectively.
Descending took 43.546 and 38.759 ms. Duplicate-key insertion took 472.106
and 466.507 ms, so the Stage 5 policy does not solve every congested case.
At 1,000 random insertions, Preview.3 took 0.607 ms and Stage 5 took 1.343 ms.
The new AVL representation is not faster at every scale or distribution.

The first 100,000 random Tree matrix run gave Stage 4 112.967 ms and Stage 5
124.081 ms, with a broad Stage 5 range of 108.651–139.708 ms. Two interleaved
repeat pairs gave Stage 5 108.834/106.017 ms and Stage 4 113.406/110.530 ms.
The initial apparent regression is not repeatable. The Group random 100,000
matrix gave 51.000/35.448 ms (Stage 4/5), but interleaved pairs gave
37.923/34.554 and 37.040/35.333 ms, with overlapping order; no Stage 5 Group
improvement is claimed. Batch results likewise overlap. `lks_sort` remains a
stable convenience API; C `qsort` need not be stable and is not a replacement
where stable equality is required.

## Why the Tree changed

The Path-keyed AVL and comparator upper-bound search were added before Stage 5.
A separately instrumented comparison-count build found:

| Tree distribution / N | Preview.3 calls | Stage 4 calls | Stage 5 calls |
| --- | ---: | ---: | ---: |
| Ascending / 10,000 | 210,463 | 123,617 | 123,617 |
| Random / 10,000 | 135,697 | 120,151 | 120,151 |
| Equal / 4,000 | 4,129,960 | 43,905 | 43,905 |
| Duplicates / 4,000 | 302,023 | 45,148 | 45,148 |

Stage 5 does not reduce comparator calls. It removes a short-lived clone of
the already-owned candidate Path during comparator insertion. On an ascending
10,000 diagnostic run, allocation calls dropped from 49,999 to 30,000 and
cumulative requested bytes from 1,821,640 to 1,259,024. This retains the
candidate's spare Path capacity, increasing peak live bytes from 1,042,704 to
1,068,880 in that run; random 10,000 rose from 1,132,528 to 1,171,584.
The current 64-bit Tree node is 48 bytes. These are private allocator
statistics, not process RSS or portable byte commitments.

The larger Stage 5 gain comes from avoiding repeated open-end repair while a
candidate Path is still within the hard depth allowance. Interior gaps retain
their repair path, and open ends still attempt repair beyond the allowance
before full rebuild. At 100,000 ascending insertions, Stage 4 attempted
64,262 repairs, abandoning 64,007; Stage 5 attempted 90 (81 successful).
Both performed nine full rebuilds. Diagnostic cumulative requested bytes fell
from 970,265,160 to 120,173,304 bytes. The Stage 5 diagnostic counted 522,876 nodes
processed by successful full rebuilds, zero gap-limit rebuild attempts, and
nine depth-limit rebuild attempts. This explains the timing gain without a
change to the AVL search or comparator count.

| Diagnostic Tree case | AVL height | Rotations | Mean/P95/P99 Path depth | Peak live bytes | Live nodes / Path objects / Path steps |
| --- | ---: | ---: | --- | ---: | --- |
| Stage 4 ascending, 100,000 | 18 | 99,958 | 3.874 / 4 / 6 | 28,298,704 | 4,800,000 / 3,200,000 / 5,892,128 |
| Stage 5 ascending, 100,000 | 18 | 99,958 | 3.881 / 5 / 6 | 28,341,744 | 4,800,000 / 3,200,000 / 6,001,184 |
| Stage 5 random, 100,000 | 19 | 69,256 | 2.757 / 3 / 3 | 11,619,984 | 4,800,000 / 3,200,000 / 3,619,968 |
| Stage 5 duplicates, 100,000 | 18 | 125,629 | 3.796 / 4 / 4 | 29,648,128 | 4,800,000 / 3,200,000 / 5,902,816 |

Peak includes transient repair/rebuild allocations, so it is larger than the
sum of the three listed live storage categories. The adopted candidate Path
and endpoint rule leave Stage 5 ascending peak slightly higher than Stage 4,
despite the much lower cumulative allocation volume. P95 can cross a discrete
depth boundary from a small layout change; P99 remained six here.

This policy is workload-sensitive. At 100,000 random insertions Stage 5
recorded 208 repair attempts, 141 successes, 67 fallbacks, and one full
rebuild affecting 2,799 nodes. At 100,000 duplicate-key insertions it
recorded 30,798 attempts, 30,534 successes, and ten full rebuilds affecting
556,781 nodes. An alternating 10,000 run still performed 26 full rebuilds
and processed 134,148 nodes through them. Full rebuild remains a necessary
atomic fallback and a measurable worst-case cost.

## Policy trials and retained values

These were local prototype checkouts, each tested with the same harness before
being rejected or incorporated. A policy change was retained only when it
preserved correctness/OOM behavior and helped representative workloads without
a material countervailing cost.

| Trial | Evidence | Decision |
| --- | --- | --- |
| Preferred depth 3 instead of 4 | Duplicate 4,000 improved (6.724→4.386 ms), random 10,000 worsened (9.838→21.444 ms). | Reject. |
| Preferred depth 5 instead of 4 | Duplicate 4,000 improved (6.724→5.333 ms), but duplicate peak live bytes rose 806,712→940,472 and average Path depth rose 2.732→2.806. | Reject pending a broader memory objective. |
| Maximum repair window 32 instead of 64 | Alternating 10,000 improved (214.2→167.2 ms), but full rebuilds rose 26→40 and holdout eight-value 8,000 slowed 13.407→14.612 ms. | Reject. |
| Maximum repair window 128 instead of 64 | Random 10,000 worsened 9.838→16.164 ms; alternating 10,000 worsened 214→245 ms. | Reject. |
| Disable all open-end repair | Broke the focused N=18 local-repair contract and forced a full rebuild. | Reject; retain repair beyond hard depth. |
| Skip open-end repair within hard depth | Large ascending/all-equal runs improved substantially, with no changed comparator count. Holdout random 20,000 changed 18.615→18.518 ms and duplicates 8,000 changed 11.489→10.998 ms. | Retain. |

The initial slot 32768, endpoint target spacing 10, preferred/hard depths 4/6,
bulk block limit 26, and repair windows 8/16/32/64 remain provisional.
The minimum midpoint span of 2 is required for an integer gap. The unchanged
values did not have enough cross-workload evidence to justify changing them.
Exact generated Paths are not a persistence compatibility contract.

## Mutations, representation size, and external context

Current-only medians at 10,000 operations were 3.177 ms for random Path
removal, 2.042 ms to remove all nodes, 6.425 ms for random rekey, 5.245 ms
for hotspot rekey, and 4.778 ms for the mixed mutation workload. The mixed
case performs three operations per `n/2` iterations. These APIs were absent
from Preview.3.

For a 10,000-step Path, the chosen fixture formats to 40,000 display bytes
or 109,238 `LK1:` key bytes. Per 100 calls, Stage 5 medians were 7.480 ms
for display format, 20.807 ms for display parse, 7.760 ms for key format,
and 8.775 ms for key parse. The `LK1:` format is designed for portable,
canonical sorting, not smallest size. Stage 5 did not modify either parser
or formatter. See [API.md](API.md) for the representation contract.

An exploratory comparison used the separate public C
[`fractional-indexing`](https://github.com/sqliteai/fractional-indexing)
`generate_key_between` implementation at commit
`ddaf4147101462b7062c549f3fad82fe9775e645`. Its source was kept outside
this repository. The adapter measured **coordinate generation only**:
`lks_path_after`/`lks_path_between` against the other library's key generator,
with ordering checks. It did not compare Tree insertion, equal-item stability,
Groups, mutation, persistence contracts, or ownership models. Both builds
used GCC `-O2`; the external source required GNU C extensions, while the
LayerKeySort side stayed strict C17. Append 10,000 took 1.288 ms for
LayerKeySort and 0.439 ms for the external generator; hotspot 4,000 took
0.361 and 1.394 ms. Final coordinate lengths differed (append: 16 display/
38 durable-key bytes versus 4 external bytes; hotspot: 8 display/17 durable
versus 669 external bytes). These results cannot establish a general winner.

## Complexity and limits

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
| Comparator Tree locate | `O(log n)` caller comparator calls plus neighboring Path work where requested. |
| Comparator Tree insert with direct gap | Upper-bound search and gap generation, then `O(log n)` Path-index insertion; costs include `c`, Path comparisons, and allocated candidate depth. |
| Tree remove | `O(log n)` Path-index search/rebalance plus Path comparisons and destruction; no allocation. |
| Tree rekey | Two Path-index searches and `O(log n)` structural work, plus clone/new-node allocation and old-Path cleanup; unchanged-coordinate success is a no-op. |
| Local repair | Up to 64 nodes in the current logical window, with possible expansions and Path regeneration; exterior lookup remains AVL-based. This is a policy bound, not a stable public API promise. |
| Full rebuild | At least linear collection/construction, with stable ordering and Path generation; it can dominate an individual insertion. |
| Group build | Stable sort uses `O(n log n)` comparator calls, then Path generation, Tree construction, and validation; Path work adds its own cost. |
| Group merge | Stable merge visits both ordered inputs linearly in item count, then builds/validates the result's fresh Path space. |
| `lks_sort` | Stable merge sort: `O(n log n)` comparator calls, `O(n)` pointer scratch, and `O(log n)` recursion depth. |

An individual comparator-driven insertion **does not** have a guaranteed
`O(log n)` time bound because it may fall back to a full rebuild. No formal
amortized bound or fixed Path-depth/memory ceiling is claimed. Rebuild timing
was not isolated from the surrounding insertion in the reported timed rows;
the diagnostic counters show frequency and cumulative affected nodes instead.
Database collation for `LK1:` must preserve bytewise ASCII order.
