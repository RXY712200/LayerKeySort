# Native GLib / Full v4.2.0 comparison

## Scope and environment

One native Windows 11 x64 campaign on an AMD Ryzen 9 9955HX, using actual **GLib 2.90.1** (`mingw-w64-ucrt-x86_64-glib2` 2.90.1-1) and released **Full v4.2.0**, source commit `f751e1ee2afd682874281d6dc495969f9eca49eb`. The research branch starts at `9a7f11eb204bc3cedcf9cc1904af6af0a075987a`; its production source, public headers and canonical source manifest are byte-identical to that release. This is an initial native-platform comparison, with no algorithm modifications or root-cause investigation.

GCC 16.2.0 compiles Full and both adapters together using `-std=c17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`, without LTO. GLib is the native MSYS2 release DLL, not a replacement implementation. Its archived build metadata identifies GCC 16.2.0-4, `!debug` and `!lto`. The exact matching packaging recipe selects Meson `--buildtype=release` and disables `glib_debug`. Thus both sides use release optimization; the packaged GLib compiler command line is not overridden by the harness. Full is linked into the executable and GLib is dynamically linked, as recorded in the build command. Both modes load the same executable and DLLs. Exact package, DLL, executable and harness SHA-256 values are in [environment.json](results/environment.json). The GLib recipe provenance and native dependency hashes are in [dependency-provenance.json](results/dependency-provenance.json).

## Protocol and semantic matching

Each implementation/scenario/size cell has one untimed-for-summary warmup run followed by three measured repetitions, each in a fresh process. Warmup warms the executable/OS rather than preserving one container or allocator. Implementation order alternates by repetition (Full then GLib for warmup and repetition 2; GLib then Full for repetitions 1 and 3). A deterministic xorshift64 seed `0x91e10da5c79e7b1d` generates the same trace and payloads in both modes. There is one trace seed per condition, not three independent workload seeds. The process runs serially with normal scheduling, without CPU affinity or machine isolation.

Timing uses `QueryPerformanceCounter` around the API loop only. Trace generation, oracle construction, initial container population (except insertion workloads), correctness validation and destruction are outside that interval. Payloads are identical 16-byte `{uint64_t key, id}` records borrowed through pointers. Trace lookup costs, comparator calls, native API adaptation, Full success-status checks and result writes needed by the operation are included. No comparator counters or allocation instrumentation are enabled.

| Scenario | Timed work at size N | Native APIs / matching behavior |
| --- | --- | --- |
| insert | N insertions into an initially empty sequence; insert before a uniformly selected existing occurrence | `lks_order_insert_before/back`; `g_sequence_insert_before` |
| remove | Random-permutation removal of all N initial occurrences | `lks_order_remove`; `g_sequence_get` plus `g_sequence_remove`, returning/checking the removed item in both adapters |
| move | N random source/anchor moves within N live occurrences | `lks_order_move_before`; `g_sequence_move`; identical before-anchor semantics, self-moves allowed |
| compare | N random pairs from the initial ordered sequence | `lks_order_compare`; `g_sequence_iter_compare`; signs checked against original ID order |
| mixed | N trace steps: 25% remove-and-append replacement, 50% random move-before, 25% precedence | Fixed live population N; a replacement step makes two mutation calls, so this is 1.25N logical API operations. Precedence pairs are current adjacent occurrences (or self at the end), unlike the standalone random-pair workload |
| managed_insert | N shuffled unique even-key insertions | `lks_managed_order_insert`; `g_sequence_insert_sorted` |
| managed_lookup | N random keys in 0..2N+1, approximately half present | `lks_managed_order_locate`; `g_sequence_search` plus iterator adaptation to return predecessor/equal/successor. Unique keys make the three-result semantics equivalent |
| managed_remove | Random-permutation removal of all N sorted occurrences | Managed removal vs iterator removal, both checking the borrowed removed item |
| sort_random | One stable pointer-array sort of N random 64-bit keys | `lks_sort`; native `g_sort_array` |
| sort_reverse | One sort of strictly descending keys | Same stable array APIs |
| sort_nearly | Initially ascending, then N/100 random pair swaps (positions can repeat) | Same stable array APIs |
| sort_duplicates | One sort of uniformly generated keys in 0..15 | Same stable array APIs; original ID order must survive within ties |

The sequence oracle is an independent flat ID-linked model built before timing, not another ordered-tree implementation. Every final occurrence pointer and traversal endpoint is checked. Removed-item results and every precedence/lookup answer are checked. Sort output is compared pointer-for-pointer against C `qsort` using the total `(key, original ID)` ordering, independently encoding stable ties. Raw trace/result fingerprints match across implementations and repetitions. This validates the exercised successful operations, not allocation-failure equivalence.

The APIs have broader differences: Full supplies failure statuses and failure-atomic contracts; GLib allocation failure commonly terminates the process. This campaign stays within successful memory admission. It does not compare those failure policies, Full snapshots/history/persistence, GLib rank/index APIs, or duplicate-equal managed lookup semantics. No snapshots or OrdWeave are involved. Stable occurrence handles/iterators and borrowed payloads are retained consistently across the selected operations; removed occurrences are never reused.

## Timing results

All **384 runs completed**, including **288 measured samples** and 96 warmups. No failed oracle, timeout or memory-admission skip occurred. Each number is median milliseconds, with the observed minimum–maximum of the three measured repetitions. Ratio is Full / GLib; below 1 favors Full. Small samples have substantial timer/scheduler sensitivity. No significance or asymptotic claim is inferred from three repetitions.

| N | Workload | Full ms [min–max] | GLib ms [min–max] | Full / GLib |
| ---: | --- | ---: | ---: | ---: |
| 1,000 | insert | 0.077 [0.074–0.082] | 0.119 [0.118–0.121] | 0.644 |
| 1,000 | remove | 0.043 [0.043–0.043] | 0.094 [0.092–0.095] | 0.459 |
| 1,000 | move | 0.059 [0.059–0.059] | 0.120 [0.118–0.121] | 0.492 |
| 1,000 | compare | 0.004 [0.004–0.004] | 0.117 [0.113–0.118] | 0.031 |
| 1,000 | mixed | 0.052 [0.051–0.052] | 0.142 [0.142–0.143] | 0.364 |
| 1,000 | managed_insert | 0.130 [0.129–0.135] | 0.218 [0.206–0.373] | 0.595 |
| 1,000 | managed_lookup | 0.127 [0.127–0.129] | 0.150 [0.150–0.158] | 0.848 |
| 1,000 | managed_remove | 0.040 [0.040–0.040] | 0.089 [0.088–0.095] | 0.454 |
| 1,000 | sort_random | 0.059 [0.058–0.229] | 0.078 [0.077–0.078] | 0.760 |
| 1,000 | sort_reverse | 0.020 [0.019–0.021] | 0.017 [0.017–0.018] | 1.161 |
| 1,000 | sort_nearly | 0.025 [0.025–0.032] | 0.019 [0.018–0.019] | 1.312 |
| 1,000 | sort_duplicates | 0.049 [0.049–0.049] | 0.044 [0.044–0.045] | 1.103 |
| 10,000 | insert | 0.559 [0.541–0.563] | 1.174 [1.129–1.182] | 0.476 |
| 10,000 | remove | 0.437 [0.436–0.437] | 1.020 [1.007–1.066] | 0.428 |
| 10,000 | move | 0.555 [0.553–0.716] | 1.374 [1.343–1.411] | 0.404 |
| 10,000 | compare | 0.030 [0.030–0.034] | 1.539 [1.515–1.726] | 0.020 |
| 10,000 | mixed | 1.058 [0.654–1.133] | 1.845 [1.611–2.166] | 0.574 |
| 10,000 | managed_insert | 1.449 [1.442–1.468] | 2.320 [2.256–3.272] | 0.625 |
| 10,000 | managed_lookup | 1.861 [1.827–3.040] | 1.998 [1.978–2.412] | 0.932 |
| 10,000 | managed_remove | 0.439 [0.436–0.443] | 1.103 [1.083–1.287] | 0.398 |
| 10,000 | sort_random | 0.705 [0.703–0.715] | 0.682 [0.673–0.834] | 1.034 |
| 10,000 | sort_reverse | 0.197 [0.194–0.203] | 0.170 [0.170–0.243] | 1.159 |
| 10,000 | sort_nearly | 0.255 [0.254–0.258] | 0.196 [0.188–0.205] | 1.304 |
| 10,000 | sort_duplicates | 0.488 [0.479–0.501] | 0.439 [0.437–0.440] | 1.112 |
| 100,000 | insert | 7.590 [7.073–7.948] | 18.697 [17.474–18.903] | 0.406 |
| 100,000 | remove | 7.636 [6.965–8.819] | 16.560 [16.408–16.658] | 0.461 |
| 100,000 | move | 8.073 [7.788–8.129] | 26.030 [24.974–26.261] | 0.310 |
| 100,000 | compare | 0.980 [0.969–0.984] | 27.674 [26.988–28.164] | 0.035 |
| 100,000 | mixed | 8.616 [8.557–9.206] | 28.088 [26.967–28.434] | 0.307 |
| 100,000 | managed_insert | 24.751 [24.369–25.557] | 36.066 [35.026–36.339] | 0.686 |
| 100,000 | managed_lookup | 33.604 [32.108–33.644] | 33.683 [33.597–34.547] | 0.998 |
| 100,000 | managed_remove | 7.060 [6.848–7.144] | 17.304 [17.231–17.896] | 0.408 |
| 100,000 | sort_random | 8.986 [8.934–9.185] | 13.183 [13.122–13.231] | 0.682 |
| 100,000 | sort_reverse | 2.218 [1.986–2.243] | 1.834 [1.834–2.002] | 1.209 |
| 100,000 | sort_nearly | 2.720 [2.713–2.979] | 2.188 [2.162–2.221] | 1.244 |
| 100,000 | sort_duplicates | 5.246 [5.175–5.337] | 4.864 [4.694–4.868] | 1.079 |
| 1,000,000 | insert | 166.035 [158.227–168.176] | 400.044 [394.469–426.537] | 0.415 |
| 1,000,000 | remove | 261.720 [257.240–280.495] | 584.510 [541.913–604.681] | 0.448 |
| 1,000,000 | move | 367.384 [364.112–367.760] | 1045.369 [1025.966–1097.963] | 0.351 |
| 1,000,000 | compare | 32.354 [30.172–34.215] | 903.995 [891.865–1045.532] | 0.036 |
| 1,000,000 | mixed | 327.466 [321.964–347.747] | 898.361 [891.533–917.525] | 0.365 |
| 1,000,000 | managed_insert | 636.996 [611.999–653.056] | 789.086 [774.140–913.487] | 0.807 |
| 1,000,000 | managed_lookup | 990.110 [962.325–1001.862] | 1133.410 [1108.717–1214.530] | 0.874 |
| 1,000,000 | managed_remove | 267.307 [262.445–275.939] | 574.118 [567.051–587.251] | 0.466 |
| 1,000,000 | sort_random | 121.663 [119.814–122.922] | 138.624 [124.061–169.425] | 0.878 |
| 1,000,000 | sort_reverse | 26.695 [26.240–27.409] | 22.563 [22.223–23.964] | 1.183 |
| 1,000,000 | sort_nearly | 36.989 [35.780–37.483] | 30.093 [29.611–31.909] | 1.229 |
| 1,000,000 | sort_duplicates | 61.057 [60.649–61.725] | 55.712 [54.836–56.000] | 1.096 |

## Comparable process-memory results

`GetProcessMemoryInfo` records **PeakWorkingSetSize** (peak resident working set) and **PeakPagefileUsage** (peak private process commit charge, not observed paging I/O). The baseline working set is sampled after common trace/payload/oracle preparation and before container creation. Peaks are lifetime process high-water marks sampled after validation, including container setup and validation. They cannot isolate the timed operation or establish requested allocation bytes. The common executable/DLLs, payloads, handles, trace, answers and independent oracle arrays are included; this deliberately uses an identical harness in both modes rather than presenting unequal allocator counters as comparable memory. Common harness storage is large and can dominate array-sort measurements. Subtracting baseline working set from a process high-water mark does not recover exact component allocation.

All memory values below are medians of three runs, in MiB (1,048,576 bytes), for N=1,000,000. Raw bytes and every other scale are retained in [raw.csv](results/raw.csv), [summary.csv](results/summary.csv) and [comparisons.csv](results/comparisons.csv).

| Workload | Full peak resident MiB | GLib peak resident MiB | Full peak commit MiB | GLib peak commit MiB | Full baseline resident MiB | GLib baseline resident MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| insert | 197.04 | 199.07 | 208.53 | 211.42 | 122.43 | 122.43 |
| remove | 209.75 | 206.69 | 213.74 | 211.43 | 130.05 | 130.07 |
| move | 209.77 | 206.70 | 213.71 | 211.42 | 130.06 | 130.06 |
| compare | 209.79 | 206.71 | 213.71 | 211.41 | 130.05 | 130.05 |
| mixed | 217.38 | 214.31 | 213.72 | 211.41 | 130.06 | 130.06 |
| managed_insert | 204.69 | 206.71 | 208.54 | 211.40 | 130.07 | 130.06 |
| managed_lookup | 204.69 | 206.72 | 208.54 | 211.41 | 130.07 | 130.06 |
| managed_remove | 204.59 | 206.70 | 208.54 | 211.42 | 130.06 | 130.07 |
| sort_random | 145.32 | 145.32 | 177.54 | 169.86 | 122.43 | 122.43 |
| sort_reverse | 145.33 | 145.33 | 177.56 | 169.88 | 122.44 | 122.44 |
| sort_nearly | 145.32 | 145.33 | 177.52 | 169.86 | 122.43 | 122.44 |
| sort_duplicates | 145.33 | 145.32 | 177.54 | 169.88 | 122.44 | 122.43 |

## Findings and limits

At one million elements Full has lower median elapsed time in all five explicit-order scenarios and all three managed-order scenarios. The measured random-pair precedence loop takes 32.354 ms for Full and 903.995 ms for GLib. These are one million public comparisons on the stated initial sequence, not a throughput claim for arbitrary application activity.

Sorting gives a mixed result. At one million elements Full takes 121.663 ms versus GLib's 138.624 ms on random input, but GLib is faster on reverse (22.563 vs 26.695 ms), nearly sorted (30.093 vs 36.989 ms), and duplicate-heavy input (55.712 vs 61.057 ms). All smaller-scale results, including unfavorable cells, remain in the table. The report does not diagnose causes or propose algorithm changes.

Process memory does not show a universal winner: for the million-item move/compare cases Full's resident peak is about 209.8 MiB versus GLib's 206.7 MiB, while insertion has a slightly lower Full peak. Array-sort resident peaks are nearly identical because the common harness dominates that metric; Full's peak process commit is about 177.5 MiB versus GLib's 169.9 MiB. These process totals must not be relabeled as per-container or requested-byte measurements.

This bounded experiment covers native Windows, one machine, one seed, unique-key managed operations and successful allocations. It supplies reproducible evidence for these workload definitions, rather than a general ranking. A 96 MiB + 256*N estimate had to fit within half available physical memory before each child run; none was skipped. Each child had a 180-second timeout. Normal background scheduling remains an uncontrolled influence, visible in the reported ranges. Full production, public contracts, version metadata, main and releases are unchanged. Technical Debt Impact: additional comparative evidence only; no existing debt is claimed resolved.

## Evidence and reproduction

See [README.md](README.md) for commands, [benchmark.c](benchmark.c) for the native adapters/oracles, [run.py](run.py) for acquisition/build/admission/execution and [analyze.py](analyze.py) for the paired-data checks and tables. [exceptions.json](results/exceptions.json) preserves the empty failure/skip lists. `results/glib.BUILDINFO.txt` and `results/glib.PKGINFO.txt` preserve the upstream package's compiler, dependencies and LGPL-2.1-or-later declaration. GLib binaries are not redistributed in this branch.
