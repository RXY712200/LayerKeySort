# Performance and benchmark evidence

## V3 Preview.3 validation retained for RC.1

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

## Preview.3 development diagnostics

No Preview.3 timing comparison is claimed here. The current harness adds a
fixed interior `hotspot` insertion pattern and a `footprint` record for final
resident Path allocation, total steps, and display/LK1 C-string bytes.
`relabel.nodes_relabelled` counts old coordinates replaced by successful
managed insertions; `max_region_nodes` records the largest successful region.
The diagnostic records are separate from wall-clock timing and cannot prove
an asymptotic bound. Historical Preview.2/V2 measurements below retain their
original labels and values.

## Released V3 Preview.2 versus Preview.1 and stable V2

The released Preview.2 implementation carries full endpoint slots into an
available ancestor and uses a one-slot stride after a long successful endpoint
run. The original `04cdd0c` candidate used an endpoint direct-depth allowance
of eight. A valid depth-nine append candidate triggered a full-range relabel at
insertion 261,579 solely because of that policy. The final candidate allows
direct endpoint coordinates through depth 16, with adaptive relabel retained
when the candidate is deeper or cannot be generated. This is a provisional
bounded growth policy, not a Path validity limit or complexity guarantee.

The [original three-version comparison](../benchmarks/results/v3-preview2-comparison.csv)
uses clean exact stable V2 and Preview.1 baselines; a separate
[holdout CSV](../benchmarks/results/v3-preview2-holdout.csv) changes the input
seed. The [final candidate timing rows](../benchmarks/results/v3-preview2-final-comparison.csv),
[final diagnostics](../benchmarks/results/v3-preview2-final-diagnostics.txt),
[policy experiment timings](../benchmarks/results/v3-preview2-policy-audit.csv),
[policy experiment diagnostics](../benchmarks/results/v3-preview2-policy-diagnostics.txt),
and [cross-workload holdout](../benchmarks/results/v3-preview2-policy-cross.csv)
show the final decision and rejected alternatives. [Original metadata](../benchmarks/results/v3-preview2-metadata.md) states the
machine, flags, timing interval, and diagnostic limits.

The following table is the **original depth-eight candidate** at `04cdd0c`,
retained as historical comparison evidence. Its Preview.2 column is not the
final candidate.

| Managed insert | N | Stable V2 ms | Preview.1 ms | Original Preview.2 ms |
| --- | ---: | ---: | ---: | ---: |
| Ascending | 100,000 | 303.922 | 362.676 | 54.104 |
| All equal | 100,000 | 303.207 | 362.617 | 53.426 |
| 32-value duplicates | 100,000 | 424.473 | 107.261 | 109.607 |
| Random unique | 100,000 | 97.487 | 96.812 | 98.830 |
| Alternating | 10,000 | 189.293 | 75.047 | 73.228 |
| Descending | 10,000 | 2.984 | 2.982 | 3.080 |
| Ascending | 300,000 | 3343.667 | 3616.003 | 505.805 |

These are one-machine median timings with one warmup and three repetitions.
The primary duplicate, random, and descending rows regress slightly against
Preview.1; the holdout duplicate and random rows improve. Do not infer a
general speed ranking or complexity bound from either seed. Diagnostic data
show that ascending 100k Preview.1 performed five full-range relabels and
visited 719,468 region nodes; Preview.2 performed zero relabels. The
[diagnostic log](../benchmarks/results/v3-preview2-diagnostics.txt) also
records relabel work, allocations, Path depths, comparator calls, AVL
rotations, and height for both versions and stable V2. At 300k ascending, the
original depth-eight candidate made one 261,578-node full-range relabel. The
final depth-16 candidate avoids that event. At 500k it has a measurable
tradeoff: its 889.976 ms median exceeds the original candidate's 749.108 ms
in a separate matched policy experiment (where the depth-16 median was
882.634 ms). At 1,000,000 ascending insertions the original candidate made
four full-range relabels of 2,487,584 old nodes in total; depth 16 made one
of 523,722. A no-depth-trigger variant made none, but final mean/P95/P99/max
Path depth grew to 15.780/30/31/31 and peak live bytes to 288,763,760.
Depth 16 ended at 6.173/14/15/16 and 165,980,120 peak live bytes.

| Ascending endpoint policy | 300k full-range relabels | 500k median ms | 1m full-range relabels / old nodes | 1m cumulative requested bytes | 1m peak live bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Original depth 8 | 1 | 749.108 | 4 / 2,487,584 | 1,237,677,144 | 261,503,264 |
| Fixed depth 12 | 0 | 1,113.694 | 2 / 1,156,008 | 843,087,256 | 210,582,904 |
| Final depth 16 | 0 | 882.634 | 1 / 523,722 | 588,908,496 | 165,980,120 |
| No depth trigger | 0 | 844.408 | 0 | 496,745,496 | 288,763,760 |
| Adaptive log-size limit | 0 | 860.316 | 0 full-range; one regional relabel of 524,288 nodes | 766,487,448 | 224,656,312 |

Rows use one machine. Timings are separate optimized runs (one warmup, three
measurements at 500k); diagnostic memory counts come from instrumented builds.
At 1m ascending, a separate matched run with one warmup and two measurements
gave 3,580.605 ms (depth eight), 2,456.653 ms (depth 16), 3,189.265 ms
(no depth trigger), and 3,098.101 ms (adaptive log-size limit).
The log-size rule adds complexity and has worse 1m memory than depth 16. The
unbounded variant's 1m Path growth is not an acceptable general tradeoff.
Depth 12 still relabels repeatedly. Depth 16 avoids the specific 300k cliff
while keeping deeper growth bounded enough for this measured range. The first
depth-17 candidate causes a full-range relabel near insertion 523,723, so the
cost has been deferred and reduced, not eliminated. Equal-item append follows
the same structural pattern; descending/prepend stays at final Path depth one
through 500k and does not relabel. Comparator calls, AVL rotations and height
at a fixed scale are unchanged by these endpoint thresholds.

The following selected rows put the final candidate beside stable V2 and
released Preview.1. V2/Preview.1 100k and ascending 300k values come from the
original direct comparison above. Their additional 300k equal, 500k ascending,
and descending values are in [baseline extra rows](../benchmarks/results/v3-preview2-baseline-extra.csv).
Original Preview.2 uses the matched policy experiment where available;
final Preview.2 uses its independent final-source CMake build. These are
observations across separate runs, not controlled paired speedup ratios.

| Managed insert | N | Stable V2 ms | Preview.1 ms | Original Preview.2 ms | Final Preview.2 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ascending | 100,000 | 303.922 | 362.676 | 54.933 | 54.574 |
| Ascending | 300,000 | 3,343.667 | 3,616.003 | 510.985 | 339.946 |
| Ascending | 500,000 | 9,689.246 | 9,547.128 | 749.108 | 889.976 |
| All equal | 100,000 | 303.207 | 362.617 | 55.500 | 55.956 |
| All equal | 300,000 | 3,273.642 | 3,593.092 | 520.029 | 355.181 |
| Descending | 100,000 | 35.823 | 35.583 | 36.456 | 35.760 |
| Descending | 300,000 | 111.815 | 114.281 | 113.631 | 117.469 |
| Random unique | 100,000 | 97.487 | 96.812 | 99.179 | 101.240 |
| 32-value duplicates | 100,000 | 424.473 | 107.261 | 110.067 | 110.557 |
| Alternating | 10,000 | 189.293 | 75.047 | 72.623 | 74.479 |

The apparently slower final alternating row prompted a nine-repetition
check: original/final medians were 72.991/73.080 ms with the primary seed
and 71.409/71.845 ms with the holdout seed. No material alternating
regression was established. The 500k ascending/all-equal cost increase is
repeatable and remains a real tradeoff of the selected depth policy.

Interior insertion still uses bounded logical-range relabel with geometric
expansion and a full-range correctness fallback. Path generation and
comparison costs depend on Path depth. Complete insertion has no claimed
worst-case `O(log n)` or formal amortized bound.

## V3 Preview.1 versus stable V2.0.0

The experimental V3 Preview.1 is released. The
[`v3-preview1-comparison.csv`](../benchmarks/results/v3-preview1-comparison.csv)
compares its public managed insertion API against stable `v2.0.0` in a clean
detached checkout. Both binaries used MSYS2 UCRT64 GCC 16.2.0, C17,
`-O2 -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG -std=c17` (the latter
`-O3` comes from CMake Release), on the
same Windows 11 / Ryzen 9 9955HX machine. Input seed was `0x91A30D47`;
each row used one warmup and three measured runs. Times below are medians in
milliseconds. Compiler, thermal state, and OS scheduling limit generality.
The [metadata sidecar](../benchmarks/results/v3-preview1-metadata.md) records
the source checkouts, flags, and measurement boundaries.

| Managed insert workload | N | Stable V2 | V3 development |
| --- | ---: | ---: | ---: |
| Ascending | 100,000 | 327.913 | 421.662 |
| All equal | 100,000 | 331.146 | 419.852 |
| 32-value duplicates | 100,000 | 477.263 | 126.797 |
| Random unique | 100,000 | 112.053 | 112.892 |
| Alternating | 10,000 | 216.716 | 85.698 |
| Two values | 10,000 | 16.918 | 7.562 |
| Eight values | 10,000 | 14.393 | 8.593 |
| 64 values | 10,000 | 12.501 | 9.041 |
| Descending | 10,000 | 3.528 | 3.517 |
| Middle hotspot | 10,000 | 3.816 | 3.733 |
| Ascending | 4,000 | 1.433 | 1.444 |
| Random unique | 4,000 | 4.980 | 3.609 |

Ascending and all-equal 100k regress measurably. Duplicates and alternating
improve strongly; random 100k is effectively close at this scale. The design
removes physical replacement-Tree rebuilding, but that does not make every
insertion distribution faster. The large-workload comparisons are engineering
evidence for this machine, not an asymptotic proof.

A separate diagnostic build (`LKS_ENABLE_ALLOC_DIAGNOSTICS` and
`LKS_BENCH_DIAGNOSTICS`, same GCC flags) reports the mechanism:

| Workload | V2 physical rebuilds / nodes rebuilt | V3 full-range relabels / old nodes relabelled | V3 all relabelled nodes | V2 / V3 comparator calls | V2 / V3 peak live bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ascending 100k | 9 / 522,876 | 5 / 292,764 | 293,524 | 1,639,288 / 1,568,929 | 28,341,744 / 22,181,704 |
| Duplicates 100k | 10 / 556,781 | 0 / 0 | 97,256 | 1,634,918 / 1,588,203 | 29,648,128 / 10,798,784 |
| Alternating 10k | 26 / 134,148 | 1 / 189 | 119,789 | 132,835 / 146,639 | 2,601,016 / 1,296,120 |
| All equal 100k | 9 / 522,876 | 5 / 292,764 | 293,524 | 1,639,288 / 1,568,929 | 28,341,744 / 22,181,704 |

V3 physical replacement-Tree rebuild count is zero by architecture; a
full-range relabel still allocates a new Path per rank and can be expensive.
For 100k ascending, V3's mean/P95/P99 Path depth was approximately
3.987/6/7, versus V2's 3.881/5/6. For duplicates, V3 was
2.146/4/5 versus V2's 3.796/4/4. These tradeoffs are why exact generated
Paths and private policy thresholds are not stable contracts.

AVL lookup takes index-height work, ignoring comparator cost. A direct insert
adds Path generation and AVL link/rebalance. Relabel planning touches a
geometrically expanded logical region; a successful relabel changes `k`
existing Paths and may have `k=n`. No worst-case `O(log n)` complete insertion
or formal amortized bound is claimed.

## Stable V2 historical evidence (Preview.4 and Stage 5)

Stable v2.0.0 retains the published RC.1 production code. The
Stage 4 and Stage 5
columns below are historical development snapshots. These numbers describe
one machine and the stated workloads; they are not universal performance
rankings. Generated
Path coordinates and private policy values remain implementation details.

Preview.5 adds usability and correctness-endurance coverage; it
does not rerun or replace the captured Stage 5 timing matrix. The new mutation
soak is a correctness test, not a performance ranking.

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

## Complexity and limits of V3 RC.1

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
