# Overall native GLib versus Full v4.2.0

This report executes the [pinned plan](https://github.com/RXY712200/LayerKeySort/blob/2949501e9523643d5ff5831d74dc124ce17fc5ad/benchmarks/research/full-v420-academic-evidence/PLAN.md). Production baseline: released Full v4.2.0, `f751e1ee2afd682874281d6dc495969f9eca49eb`. No product changes or performance targets are part of this experiment.

## Hosts, versions and provenance

- **Windows, prior**: GLib **2.90.1**, 384 fresh-process samples. Exact compiler, hardware, build commands and source/binary identities: [windows-prior/environment.json](results/windows-prior/environment.json).
- **Windows, new**: GLib **2.90.1**, 512 fresh-process samples. Exact compiler, hardware, build commands and source/binary identities: [windows-new/environment.json](results/windows-new/environment.json).
- **Linux, new**: GLib **2.90.1**, 896 fresh-process samples. Exact compiler, hardware, build commands and source/binary identities: [linux-new/environment.json](results/linux-new/environment.json).

Windows runs use a Ryzen 9 9955HX and GCC 16.2.0; GLib is the actual native MSYS2 UCRT64 2.90.1-1 release DLL. Linux runs use a native Ubuntu 24.04 GitHub-hosted runner; its CPU/compiler are recorded in the Linux environment file. GLib 2.90.1 is built from the SHA-256-pinned GNOME source archive, in Meson release mode with `b_ndebug=true`, `glib_debug=disabled`, no LTO. Full and the adapters use C17 `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror` on each host. The MSYS2 package has downstream packaging patches; Linux uses upstream source. Both are actual native GLib 2.90.1, not emulations. Linux build options and Meson log are archived. Absolute speed comparisons between these different machines would be misleading; all ratios below compare implementations within one host.

The original Windows data are preserved byte-for-byte from the Git blobs at [`2980792`](https://github.com/RXY712200/LayerKeySort/tree/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420). They were not rerun for this report. New Windows measurements cover only the added mixed traces and lifecycle cells. Linux executes the original twelve scenarios and the new cells. [Prior-data provenance](results/windows-prior/provenance.json) records source paths and hashes.

## Protocol and matching

All sizes are 1k, 10k, 100k and 1m. Each implementation/size/scenario/seed has one warmup and three measured repetitions in separate processes, with alternating implementation order. The four new stochastic mixed workloads and shuffled managed lifecycle population each use three fixed seeds (`0x91e10da5c79e7b1d`, `0x243f6a8885a308d3`, `0x13198a2e03707344`). Deterministic order lifecycle and the retained legacy scenarios use the original single seed. Tables first take each seed's median of three repetitions, then the median across seeds. Their ranges span all measured observations; [seed-summary.csv](results/seed-summary.csv) retains per-seed medians and ranges. The seeds are generated traces, not collected human interactions. Repetitions are not treated as independent application workloads or used to claim statistical significance.

| Added workload | Nominal precedence / move / replace mix | Locality |
| --- | --- | --- |
| mix_query | 90% / 5% / 5% | Global random live handles |
| mix_balanced | 50% / 25% / 25% | Global random live handles |
| mix_mutation | 10% / 45% / 45% | Global random live handles |
| mix_hotspot | 50% / 25% / 25% | Mutations select from 64 evolving hot occurrences; queries remain global |

Each trace has N steps at fixed live population N. A replacement removes one occurrence and appends a new borrowed payload, so it makes two mutation calls. Moves retain occurrence identity and use before-anchor semantics. Every query selects arbitrary currently live pairs after preceding mutations, rather than inferring precedence from initial IDs. Payloads are identical 16-byte key/ID records. An untimed array-based implicit-treap rank oracle supplies expected query signs; independent ID-linked reference updates and complete final traversal validate its resulting sequence. Every actual comparison/removal answer and final occurrence pointer is checked. The oracle is research correctness machinery, not another performance control. Trace hashes and answer/final-order fingerprints match across implementations and hosts.

The legacy scenarios retain the original definitions: random-position insertion into empty order, random-permutation removal, random move-before, random-pair precedence on initially unmutated order, the original adjacent-query mixed trace, shuffled unique-key sorted insertion, predecessor/equal/successor lookup with approximately half absent keys, sorted-container removal, and four stable pointer-array sorts. GLib lookup adapts native `g_sequence_search` to the same three-result semantics. Managed keys are unique: duplicate-equal lookup policy is not forced into equivalence. Stable sort checks use independent `(key, original occurrence ID)` total ordering. Sort distributions are random 64-bit keys, strictly reverse keys, ascending keys with N/100 random swaps, and 16 duplicate values. [Original definitions](https://github.com/RXY712200/LayerKeySort/blob/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420/REPORT.md) remain available.

Timing covers the public operation loop, including adapters, result writes and Full success-status checks. Generation, payload/oracle preparation, correctness checking and destruction are excluded from operation time. New measurements separately time container creation, initial population and destruction. Lifecycle-only cells time construction/population/cleanup without a second operation campaign. `lifecycle_order` appends N payloads; `managed_lifecycle` populates N shuffled unique keys. Payload allocation and reference-model work remain outside those costs. Sorting has no comparable container lifecycle. Original Windows lifecycle costs were not captured and remain unavailable; unchanged successful-operation observations are reused rather than recreated.

## Added mixed workload results

Median ms; brackets are observed min–max across measured repetitions/seeds. F/G is Full time divided by GLib time; below 1 favors Full.

| Host | N | Workload | Full ms [range] | GLib ms [range] | F/G |
| --- | ---: | --- | ---: | ---: | ---: |
| Windows | 1,000 | mix_query | 0.016 [0.015–0.017] | 0.133 [0.123–0.148] | 0.121 |
| Windows | 1,000 | mix_balanced | 0.040 [0.039–0.048] | 0.148 [0.139–0.158] | 0.272 |
| Windows | 1,000 | mix_mutation | 0.059 [0.058–0.128] | 0.142 [0.138–0.148] | 0.413 |
| Windows | 1,000 | mix_hotspot | 0.032 [0.029–0.033] | 0.133 [0.127–0.141] | 0.243 |
| Windows | 10,000 | mix_query | 0.116 [0.112–0.123] | 1.736 [1.610–2.300] | 0.067 |
| Windows | 10,000 | mix_balanced | 0.356 [0.349–0.470] | 1.773 [1.723–2.370] | 0.201 |
| Windows | 10,000 | mix_mutation | 0.592 [0.578–0.735] | 1.648 [1.600–2.074] | 0.359 |
| Windows | 10,000 | mix_hotspot | 0.242 [0.239–0.404] | 1.542 [1.453–1.697] | 0.157 |
| Windows | 100,000 | mix_query | 2.181 [2.095–3.076] | 30.194 [28.595–31.534] | 0.072 |
| Windows | 100,000 | mix_balanced | 6.039 [5.844–6.619] | 28.416 [28.184–30.362] | 0.213 |
| Windows | 100,000 | mix_mutation | 10.717 [10.152–11.092] | 29.157 [27.727–31.097] | 0.368 |
| Windows | 100,000 | mix_hotspot | 5.089 [4.285–5.814] | 24.835 [22.430–25.238] | 0.205 |
| Windows | 1,000,000 | mix_query | 76.065 [74.438–83.709] | 905.456 [868.941–1011.679] | 0.084 |
| Windows | 1,000,000 | mix_balanced | 244.973 [236.511–246.572] | 949.431 [908.751–1038.498] | 0.258 |
| Windows | 1,000,000 | mix_mutation | 426.513 [402.635–447.191] | 959.813 [908.082–992.979] | 0.444 |
| Windows | 1,000,000 | mix_hotspot | 78.281 [74.959–80.328] | 581.794 [560.419–608.280] | 0.135 |
| Linux | 1,000 | mix_query | 0.014 [0.012–0.014] | 0.180 [0.170–0.201] | 0.076 |
| Linux | 1,000 | mix_balanced | 0.041 [0.038–0.043] | 0.186 [0.178–0.199] | 0.222 |
| Linux | 1,000 | mix_mutation | 0.065 [0.062–0.066] | 0.183 [0.175–0.204] | 0.356 |
| Linux | 1,000 | mix_hotspot | 0.033 [0.031–0.046] | 0.180 [0.169–0.210] | 0.181 |
| Linux | 10,000 | mix_query | 0.206 [0.199–0.223] | 2.609 [2.536–2.772] | 0.079 |
| Linux | 10,000 | mix_balanced | 0.507 [0.492–0.527] | 2.546 [2.488–2.634] | 0.199 |
| Linux | 10,000 | mix_mutation | 0.817 [0.772–0.824] | 2.345 [2.304–2.383] | 0.348 |
| Linux | 10,000 | mix_hotspot | 0.342 [0.337–0.368] | 2.291 [2.173–2.494] | 0.149 |
| Linux | 100,000 | mix_query | 3.775 [3.766–3.842] | 47.488 [45.764–48.467] | 0.079 |
| Linux | 100,000 | mix_balanced | 10.370 [10.336–10.542] | 49.336 [46.568–50.848] | 0.210 |
| Linux | 100,000 | mix_mutation | 17.941 [17.873–18.577] | 48.375 [46.990–48.896] | 0.371 |
| Linux | 100,000 | mix_hotspot | 4.198 [4.188–5.115] | 35.685 [33.772–45.047] | 0.118 |
| Linux | 1,000,000 | mix_query | 73.469 [73.132–74.552] | 996.707 [958.883–1044.663] | 0.074 |
| Linux | 1,000,000 | mix_balanced | 171.158 [169.513–173.474] | 992.762 [973.742–1026.892] | 0.172 |
| Linux | 1,000,000 | mix_mutation | 290.845 [289.893–292.875] | 1001.053 [998.840–1018.878] | 0.291 |
| Linux | 1,000,000 | mix_hotspot | 90.158 [89.231–90.878] | 653.544 [611.732–688.295] | 0.138 |

## Original operations and secondary stable sorting

Windows rows below reuse the original observations; Linux rows are new. All four scales and ranges are in [summary.csv](results/summary.csv). The million-element rows are shown here to keep this report concise.

| Host | N | Workload | Full ms [range] | GLib ms [range] | F/G |
| --- | ---: | --- | ---: | ---: | ---: |
| Windows | 1,000,000 | insert | 166.035 [158.227–168.176] | 400.044 [394.469–426.537] | 0.415 |
| Windows | 1,000,000 | remove | 261.720 [257.240–280.495] | 584.510 [541.913–604.681] | 0.448 |
| Windows | 1,000,000 | move | 367.384 [364.112–367.760] | 1045.369 [1025.966–1097.963] | 0.351 |
| Windows | 1,000,000 | compare | 32.354 [30.172–34.215] | 903.995 [891.865–1045.532] | 0.036 |
| Windows | 1,000,000 | mixed | 327.466 [321.964–347.747] | 898.361 [891.533–917.525] | 0.365 |
| Windows | 1,000,000 | managed_insert | 636.996 [611.999–653.056] | 789.086 [774.140–913.487] | 0.807 |
| Windows | 1,000,000 | managed_lookup | 990.110 [962.325–1001.862] | 1133.410 [1108.717–1214.530] | 0.874 |
| Windows | 1,000,000 | managed_remove | 267.307 [262.445–275.939] | 574.118 [567.051–587.251] | 0.466 |
| Windows | 1,000,000 | sort_random | 121.663 [119.814–122.922] | 138.624 [124.061–169.425] | 0.878 |
| Windows | 1,000,000 | sort_reverse | 26.695 [26.240–27.409] | 22.563 [22.223–23.964] | 1.183 |
| Windows | 1,000,000 | sort_nearly | 36.989 [35.780–37.483] | 30.093 [29.611–31.909] | 1.229 |
| Windows | 1,000,000 | sort_duplicates | 61.057 [60.649–61.725] | 55.712 [54.836–56.000] | 1.096 |
| Linux | 1,000,000 | insert | 212.833 [212.033–214.750] | 586.363 [582.018–587.343] | 0.363 |
| Linux | 1,000,000 | remove | 242.896 [232.907–244.264] | 555.504 [547.873–561.350] | 0.437 |
| Linux | 1,000,000 | move | 307.047 [306.776–310.793] | 1108.526 [1097.632–1109.153] | 0.277 |
| Linux | 1,000,000 | compare | 40.078 [39.976–40.108] | 975.017 [961.158–987.341] | 0.041 |
| Linux | 1,000,000 | mixed | 241.033 [236.637–241.667] | 979.298 [975.541–981.214] | 0.246 |
| Linux | 1,000,000 | managed_insert | 914.088 [913.810–917.184] | 1066.583 [1064.087–1071.037] | 0.857 |
| Linux | 1,000,000 | managed_lookup | 1162.519 [1158.748–1163.857] | 1185.740 [1177.490–1190.258] | 0.980 |
| Linux | 1,000,000 | managed_remove | 254.102 [252.834–254.656] | 670.507 [663.089–674.260] | 0.379 |
| Linux | 1,000,000 | sort_random | 164.453 [164.331–164.954] | 132.004 [131.740–132.020] | 1.246 |
| Linux | 1,000,000 | sort_reverse | 35.968 [35.878–36.088] | 28.964 [28.908–29.537] | 1.242 |
| Linux | 1,000,000 | sort_nearly | 54.425 [54.109–54.592] | 39.650 [39.587–39.667] | 1.373 |
| Linux | 1,000,000 | sort_duplicates | 97.071 [97.051–97.568] | 75.420 [75.379–75.531] | 1.287 |

## Create, populate and destroy

Dedicated million-element lifecycle cells below report medians (ms). Full/GLib cells use the same borrowed payloads and no item destructor. Complete phase observations for every new cell/size/seed are in raw CSVs. The lifecycle sum in summary.csv is a median of per-run phase sums (create + populate + operation loop + destroy); it is not whole-process wall time and excludes reference preparation and validation.

| Host | Container | Full create | GLib create | Full populate | GLib populate | Full destroy | GLib destroy |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Windows | lifecycle_order | 0.0057 | 0.0067 | 34.4519 | 55.5283 | 21.9967 | 44.0518 |
| Windows | managed_lifecycle | 0.0123 | 0.0110 | 616.7996 | 755.2180 | 54.4978 | 89.2984 |
| Linux | lifecycle_order | 0.0006 | 0.0009 | 30.6932 | 65.6664 | 7.6569 | 18.6672 |
| Linux | managed_lifecycle | 0.0006 | 0.0008 | 921.5976 | 1064.8669 | 15.5241 | 40.7342 |

## Memory

Resident memory is process peak working set on Windows and `getrusage(RUSAGE_SELF).ru_maxrss * 1024` on Linux. Windows also records peak private process commit (`PeakPagefileUsage`); this is not paging I/O. Linux private commit is unavailable and encoded as 0, never interpreted as zero memory consumption. Baseline resident memory is sampled after trace/reference preparation but before container creation. High-water marks include setup and validation, and are sampled before destruction. They include shared libraries, common payload/trace/answer arrays and the oracle. In particular, the new mixed oracle adds common reference memory; do not compare its peaks to the smaller legacy harness as isolated component storage. These are comparable implementation totals within each cell, not requested allocation bytes, portable allocator counters, or exact container-only footprints. A same-harness comparison avoids mixing Full diagnostic allocation bytes with unrelated GLib process metrics.

Million-element representative peak resident values (MiB, 1,048,576 bytes); all cells and baselines remain in summary.csv.

| Host | Workload | Full peak resident MiB | GLib peak resident MiB |
| --- | --- | ---: | ---: |
| Windows | insert | 197.04 | 199.07 |
| Windows | remove | 209.75 | 206.69 |
| Windows | move | 209.77 | 206.70 |
| Windows | compare | 209.79 | 206.71 |
| Windows | mixed | 217.38 | 214.31 |
| Windows | managed_insert | 204.69 | 206.71 |
| Windows | managed_lookup | 204.69 | 206.72 |
| Windows | managed_remove | 204.59 | 206.70 |
| Windows | sort_random | 145.32 | 145.32 |
| Windows | sort_reverse | 145.33 | 145.33 |
| Windows | sort_nearly | 145.32 | 145.33 |
| Windows | sort_duplicates | 145.33 | 145.32 |
| Windows | mix_query | 297.51 | 294.45 |
| Windows | mix_balanced | 297.52 | 294.44 |
| Windows | mix_mutation | 297.51 | 294.44 |
| Windows | mix_hotspot | 297.51 | 294.44 |
| Windows | lifecycle_order | 209.81 | 206.71 |
| Windows | managed_lifecycle | 204.71 | 206.73 |
| Linux | insert | 168.34 | 170.46 |
| Linux | remove | 203.84 | 200.96 |
| Linux | move | 180.97 | 178.05 |
| Linux | compare | 203.97 | 200.96 |
| Linux | mixed | 211.59 | 208.59 |
| Linux | managed_insert | 175.92 | 178.17 |
| Linux | managed_lookup | 198.80 | 200.80 |
| Linux | managed_remove | 198.80 | 200.99 |
| Linux | sort_random | 109.54 | 101.92 |
| Linux | sort_reverse | 109.55 | 101.92 |
| Linux | sort_nearly | 109.54 | 101.92 |
| Linux | sort_duplicates | 109.55 | 101.61 |
| Linux | mix_query | 291.59 | 288.71 |
| Linux | mix_balanced | 291.59 | 288.69 |
| Linux | mix_mutation | 291.57 | 288.71 |
| Linux | mix_hotspot | 291.59 | 288.69 |
| Linux | lifecycle_order | 158.09 | 155.04 |
| Linux | managed_lifecycle | 175.83 | 178.12 |

## Observations

- Windows: Full has a lower median operation-loop time in 16/16 added mixed scenario/size cells. At one million elements, F/G ratios are mix_query 0.084, mix_balanced 0.258, mix_mutation 0.444, mix_hotspot 0.135.
- Windows: GLib has the lower median stable-sort time in 13/16 distribution/size cells. All outcomes are preserved, rather than only favorable Full cells.
- Linux: Full has a lower median operation-loop time in 16/16 added mixed scenario/size cells. At one million elements, F/G ratios are mix_query 0.074, mix_balanced 0.172, mix_mutation 0.291, mix_hotspot 0.138.
- Linux: GLib has the lower median stable-sort time in 16/16 distribution/size cells. All outcomes are preserved, rather than only favorable Full cells.

These observations describe the declared public-API traces on each host. They do not attribute a cause, establish a complexity bound, or support a universal component ranking. Near-equal values and small-N creation timings should be read with their observed ranges, not as robust wins.

## Completion, limitations and reproduction

The integrated dataset contains 1,792 successful fresh-process samples: 384 retained Windows samples, 512 new Windows samples and 896 new Linux samples. Of these, 1,344 are measured and 448 are warmups. There are no skipped cells, timeouts or failed correctness checks. The analyzer verifies every paired cell and every common trace/result identity across hosts. An additional 224 successful small-scale Windows smoke runs are archived separately and excluded from the comparison. The [execution record](results/execution.json) gives the actual Linux workflow source revision and run/artifact evidence.

This is a bounded comparison, not a stress campaign: one physical Windows host and one native Linux hosted-runner environment, three workload seeds for additions, three measured repeats after warmup, successful allocations only, ordinary OS scheduling and no CPU isolation. The two machines differ. Full is linked into the test executable; GLib is dynamically linked. Windows uses the previously verified release package; Linux builds the pinned upstream release. Managed duplicate policy and failure semantics differ (Full returns statuses/failure-atomic contracts; GLib allocation failure can terminate). Snapshots, history, Group/Batch, other controls and downstream applications are outside the equivalent-semantics scope. No per-operation latency distribution or isolated container allocator instrumentation was added: the reused harness measures aggregate loops, and GLib lacks a directly matching public per-container allocation counter. Previous Full diagnostics retain their original scope and are not presented as GLib allocation measurements.

See [README.md](README.md) for exact commands and provenance. Production code, APIs, formats, versions, main and releases are unchanged. Technical Debt Impact: **No change**; this adds comparative evidence and does not claim an algorithmic repair or independent acceptance.
