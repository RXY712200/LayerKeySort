# V3 Preview.3 validation capture

Source commit: `8db22a831506f70163c419bc6d931063df884631`.
This capture supports Preview.4 user-facing performance guidance; Preview.4
does not change the measured ordering algorithm.

Machine: Windows 11 Pro 10.0.26300, AMD Ryzen 9 9955HX (16 cores/32 logical
processors), approximately 16 GB RAM. Compiler: MSYS2 UCRT64 GCC 16.2.0,
C17 with `-Wall -Wextra -Wpedantic -Werror`. Timed binaries used
`-O3 -DNDEBUG`; the separately instrumented diagnostic binary used
`-O2 -DNDEBUG -DLKS_ENABLE_ALLOC_DIAGNOSTICS -DLKS_BENCH_DIAGNOSTICS`.
Production `src/*.c` and `benchmarks/current_benchmark.c` were compiled
together. Seed was `0x91A30D47` unless stated otherwise.

[`v3-preview3-validation-timed.csv`](v3-preview3-validation-timed.csv)
contains all seven Tree workloads at 100k, 300k, 500k, and 1m. The first two
scales used one warmup and three measured runs; 500k used one warmup and two
runs; 1m used no warmup and one run. The CSV records the median and range.
Consequently, 1m timings are observations, not stable medians. Workload
definitions are in `current_benchmark.c`: `alternating` interleaves low/high
keys toward the center, and `hotspot` repeatedly inserts immediately after
one pinned lowest-key item. The timed harness excludes input preparation,
output validation, and destruction from its interval.

[`v3-preview3-validation-diagnostics.txt`](v3-preview3-validation-diagnostics.txt)
contains one separate diagnostic run for each case. `relabel` counts successful
relabels and cumulative **old-coordinate replacements**; the same old node can
be counted more than once. `footprint` gives final resident Path bytes and
the display/LK1 C-string sizes, including one NUL per Path. `diag` includes
Path-depth average/P95/P99, allocator peak, and other private counters; see
[`benchmarks/README.md`](../README.md) for its field order. Diagnostic run
times are not comparable with timed CSV values.

At 1m, a second diagnostic seed `0xDEADBEEF` produced one full-range relabel
for random unique keys and two for 32-value duplicates, matching the primary
seed's full-range counts. Other counts varied: random had 1,208 successful
relabels affecting 25,383 old coordinates; duplicates had 70,204 affecting
3,049,306. This is a holdout observation, not an exhaustive seed study.

An additional per-insert probe outside the timed harness wrapped every public
`lks_ordered_tree_insert()` in C17 `timespec_get()` calls using the same
optimized sources and workloads. It observed maximum single-insert delays of
about 24/240/308/723 ms for alternating 100k/300k/500k/1m, about 654 ms
for ascending 1m, and about 1,078 ms for duplicates 1m. Alternating 1m had
55 inserts above 100 ms; a separate instrumented run tied its 56 full-range
relabels to the long pauses. Per-call timing has measurement overhead and
machine scheduling noise; these figures describe one local environment and
do not define a public latency guarantee.

The capture shows a real large-scale tradeoff. It does not establish a formal
amortized or worst-case bound, a universal speed ranking, or a fixed Path
depth/memory ceiling.
