# Tail-Cost Research

## Recommendation

**B — Tradeoff only. Do not adopt any tested strategy.** Dense open-range preparation reduces timeline maximum/full-range tails, including the holdout and large case, but substantially worsens very-high-percentile latency, allocation traffic, and duplicate/random controls. Keep the exact v3.1 algorithm as the production default. No v3.2.0 scope or V4 conclusion is assigned.

## Baseline problem

Base main: `c33afa2ac08248bffa6dde51bd590a5d799b394a`. Stable remains `v3.1.0` at `dfa9562b9471947cfbd4ee1d1750a59434be83e2`. Candidate implementation/campaign source is `705db25` (full SHA in capture metadata).

The baseline prepares a valid direct coordinate, prefers depth at most six for interior insertion, and otherwise starts an eight-node contiguous logical relabel. Prepared coordinates must be ordered inside exterior bounds and have maximum depth **strictly less** than the original candidate. Rejected preparations are destroyed and the window doubles. Full range uses sparse bulk generation; resident Paths are untouched until preparation and validation succeed.

Default GCC Release `tree.c.obj` SHA-256 is `c7ac50043693fb3708620ae28deab75432875741a3c094825b19563b4894c5fc`: byte-identical to the saved pre-research object with the same compiler/options. Strategy zero is compile-time default; candidate branches and trace state are excluded from ordinary production builds.

## Method and provenance

Windows 11 Pro 10.0.26300, Ryzen 9 9955HX (16 cores/32 threads), visible RAM 15,922,848 KiB. GCC 16.2.0, C17 Release, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`. No affinity/power isolation. Metadata records compiler, CMake options, source commit, binaries' SHA-256, exact plan and capture time.

The original simulator workload/RNG/oracle definitions and `run_workloads.py` remain unchanged. Previously captured `v3.1-workload-evidence-*` files remain historical and are not overwritten or mixed into new timing columns. The research reruns the baseline. Each scenario executes baseline production/diagnostic followed by candidate production/diagnostic, not randomized. One timing capture per seed is evidence, not a statistical confidence interval. Tiny central-timing changes and isolated scheduling outliers are not optimization claims.

The complete frozen plan has 52 scenarios: eight small application cases; twenty medium application cases (five primary seeds); four large application cases; one long churn; fourteen small/medium controls; four medium holdouts; one long holdout churn. Primary seeds: 17, 305419896, 2654435769, 826366246, 12648430; reserved holdout: 3512641005. Medium is 10k initial/100k operations; large 100k/100k; long churn 50k/500k. Existing profile definitions are specified in [the historical evidence report](WORKLOAD_EVIDENCE.md) and the frozen simulator.

104 production/diagnostic pairs (208 runs) completed. Independent membership/equality/order validation, cleanup, within-strategy coordinate digests, and cross-strategy logical action digests all passed. Production timing surrounds managed insertion only, including relabel when triggered. Diagnostic runs supply counts/storage, **not production timing**. P99.9 is nearest rank and reported only with at least 10k insertions. Reported relabel counts exclude untimed initialization. The diagnostic `attempts` counter counts insertion operations entering relabel, not individual doubling windows; `attempted_nodes_sum` counts nodes in all attempted windows, including rejected preparations. Resident byte statistics are requested library storage, not process RSS. LK1 sizes exclude NUL/database overhead. Requested allocation traffic counts successful requested bytes, not allocator physical overhead.

## Full-range root cause

Targeted tracing is opt-in on research diagnostic executables: set `LKS_RESEARCH_TRACE_OP=91178` for a specific operation or `full` for the first four full events. Each insertion buffers at most 128 windows without allocation; exterior display strings are bounded (256 bytes, `too-long` fallback). Ordinary A/B runs disable tracing. Traces are process-local, private and unsuitable as a public concurrent diagnostics interface.

The retained [159-window trace CSV](../benchmarks/results/tail-cost-research-root-traces.csv) covers 11 events: medium timeline seed 17 (26702/51148/80269), timeline holdout (26789/51354/80487), large timeline seed 17 (13783/39851/65621/91178), priority seed 17 (54515).

These diagnostic replay timings were captured before the candidate freeze using the traced baseline implementation. The replay binary was subsequently rebuilt, so its exact original binary hash is not retained; replay numbers are explanatory phase observations, not part of the frozen A/B timing comparison.

Every recorded window generated successfully (status zero). Nonfinal windows rejected because generated maximum depth was equal to or greater than candidate depth seven; final bulk windows accepted at depth four (priority three). There was no level-limit failure in these replays.

### Large timeline operation 91,178

| Window | Left/right selected | Generated max depth | Outcome |
|---|---|---|---|
| 8 | 4/4 | 7 | depth-not-better |
| 16 | 11/5 | 7 | depth-not-better |
| 32 | 27/5 | 7 | depth-not-better |
| 64 | 59/5 | 8 | depth-not-better |
| 128 | 123/5 | 9 | depth-not-better |
| 256 | 251/5 | 10 | depth-not-better |
| 512 | 507/5 | 11 | depth-not-better |
| 1024 | 1019/5 | 12 | depth-not-better |
| 2048 | 2043/5 | 13 | depth-not-better |
| 4096 | 4091/5 | 13 | depth-not-better |
| 8192 | 8187/5 | 13 | depth-not-better |
| 16384 | 16379/5 | 12 | depth-not-better |
| 32768 | 32763/5 | 15 | depth-not-better |
| 65536 | 65531/5 | 16 | depth-not-better |
| 131072 | 131067/5 | 17 | depth-not-better |
| 172867 | 172862/5 | 4 | accepted |

The first eight-node window has two bounds. At sixteen nodes, all five nodes on the right have been selected and the right exterior bound is absent. Windows 8/16/32 nevertheless produce valid depth-seven coordinates: equality alone rejects them. From 64 upward, balanced gap generation creates greater depth; it is not running out of valid coordinates. It finally bulk-generates the entire 172,867-node index.

Thus a congested near-end gap triggers the event, but it is not an intrinsically unrepresentable gap. A direct depth-seven coordinate already exists. Alternating selection may affect the first window, but cannot alone explain expansion after the right end is reached. Generator layout and strict depth-improvement acceptance jointly cause the cliff. The experiment does not prove that accepting equality universally is safe for future storage/workload cost.

### Other representative chains

- `timeline-17-10000`, op 26702: 8(7) → 16(7) → 32(7) → 64(8) → 128(9) → 256(10) → 512(11) → 1024(12) → 2048(13) → 4096(13) → 8192(13) → 16384(12) → 31367(4).
- `timeline-3512641005-10000`, op 26789: 8(7) → 16(7) → 32(7) → 64(8) → 128(9) → 256(10) → 512(11) → 1024(12) → 2048(13) → 4096(13) → 8192(13) → 16384(12) → 31306(4).
- `priority-17-10000`, op 54515: 8(8) → 16(8) → 32(8) → 64(7) → 128(8) → 256(9) → 512(9) → 1024(8) → 2048(8) → 4096(9) → 8192(13) → 15967(3).

Numbers in parentheses are generated maximum depth. Every nonfinal window is valid but fails strict shallowness. Priority starts above depth seven, reaches equal depth at window 64, and later reaches the open end at window 1024; full range 15,967 is the first accepted preparation. The mechanism is shared, though congestion geometry differs.

### Cost decomposition

The following milliseconds are **targeted diagnostic replay phases**, not comparable to production max columns. Scratch includes allocation/initialization; selection includes in-order materialization; generation includes all Path allocations; validation includes new-node preparation; finish includes rejected Path destruction, or final replacement/old-Path destruction/link/scratch cleanup. Allocator time cannot be isolated from these phases. Comparator upper-bound search and direct candidate generation before relabel are outside phase sums.

| Case/op | Scratch | Selection | Generation | Validation | Finish | Alloc calls | Requested MiB |
|---|---|---|---|---|---|---|---|
| timeline-17-10000/26702 | 0.144 | 3.704 | 17.625 | 10.107 | 17.179 | 128298 | 9.552 |
| timeline-3512641005-10000/26789 | 0.115 | 1.226 | 15.815 | 2.791 | 3.939 | 128176 | 9.545 |
| timeline-17-100000/91178 | 0.707 | 41.770 | 141.304 | 51.479 | 50.372 | 870062 | 76.599 |
| priority-17-10000/54515 | 0.093 | 2.104 | 7.973 | 1.539 | 3.204 | 64726 | 4.300 |

For operation 91178, failed windows consume 117.614 ms of generation and 18.857 ms of finish work before full range. Full-range generation is 23.690 ms and final finish 31.515 ms. Repeated failed preparation is a substantial cost, not just the final linear replacement. Scratch-array preparation alone is small; changing scratch allocation alone has no demonstrated route to remove the dominant cost. Final replacement/destruction still touches every affected node. No unsafe incremental in-place mutation or allocation-after-commit approach was attempted.

The seven diagnosis questions therefore resolve as: (1) a local near-end trigger, not coordinate impossibility; (2) selection alone insufficient; (3) viable depth-seven direct escape exists; (4) strict equality rejection forces expansion; (5) failed windows are material; (6) Path generation dominates this traced case with selection/validation/destruction also material, allocator cost inseparable; (7) priority shares the acceptance/generator mechanism. Causation beyond these deterministic replays remains a hypothesis.

## Candidate strategies

One `tree.c`, private `LKS_RESEARCH_RELABEL_STRATEGY` compile-time IDs, no public selection API. `-DLKS_BUILD_RESEARCH=ON -DLKS_BUILD_BENCHMARKS=ON` builds every variant and the default baseline; nothing experimental is installed/exported.

| ID/name | Exact difference | Rationale |
|---|---|---|
| 0 baseline | None | Released control |
| 1 direct7 | Accept valid interior direct depth ≤7 before relabel | Avoid a cliff with one additional level |
| 2 direct8 | Same, depth ≤8 | Wider depth/storage tradeoff |
| 3 depth_bias | Include gap neighbors, then 3:1 contiguous expansion toward initially shallower neighbor (ties left) | Reach shallower exterior space sooner |
| 4 equal8 | Also accept generated maximum equal to candidate depth when ≤8 | Test bounded equal-depth redistribution |
| 5 open_range | For candidate depth ≤8 and one open exterior bound, sequential managed endpoint carry with stride 10; full range unchanged | Avoid balanced endpoint generation's depth growth |
| 6 open_dense | Same as 5, stride 1 | Spend dense endpoint space to prepare a shallower region |

All variants retain strict Path ordering/exterior checks, stable comparator rank, full-range bulk fallback, and allocation before commit. Strategy 4 alone changes the depth acceptance predicate. Automatic coordinates may differ; logical action traces must not.

## Rejected candidates

Screen captures precede the final implementation freeze, with dirty state, binary hashes and source commit recorded explicitly. They are negative engineering screens, not final campaign comparisons. See `tail-cost-research-screen-original-*` and `tail-cost-research-screen-dense-*` in results.

- direct7: medium timeline still three full events, no material maximum benefit. Priority improves, but target objective unmet.
- direct8: timeline still two full events and maximum roughly unchanged; duplicate control introduces full range and larger coordinates.
- depth_bias: timeline still three full events; duplicates introduce full range and more relabel work. Choosing a shallower neighbor does not reliably predict usable exterior space.
- equal8: timeline still two full events; 2,865 relabels instead of three and roughly doubled alternating relabel count. It trades strict rejection for frequent repairs without eliminating the target cliff.
- open_range (stride 10): timeline still three full events with identical final coordinates; random repairs rise from eight to 868. Weak target benefit.
- An initial unrestricted open-range prototype violated the unchanged regression expecting expanded local repair. Tests were not patched. It was restricted to candidate depths ≤8, targeting the measured seven/eight-depth problem; the revised variant passes all gates. This was a private policy regression, not evidence of a proven public ordering violation.
- open_dense survived screening for full evaluation only. The completed campaign rejects production adoption because it moves serious cost into high percentiles, random and duplicates.

## Surviving candidates

No candidate survives the final adoption criteria. `open_dense` is the sole serious final comparison, not a production recommendation. Families A/B/C were killed early. Family D diagnosis favored avoiding wasted preparatory generation rather than weakening atomicity or adding scratch-buffer complexity; open-range preparation is the tested compatible alternative. No separate full-range commit optimization is justified by this evidence.

## Workload comparison

### Five primary medium seeds

Max/P99.9 columns are ranges of per-seed production values, not pooled percentiles. Full counts/relabel totals/allocation traffic are summed across the five diagnostic runs. Relabel rate uses total measured insertions, including update reinsertion.

| Profile | Strategy | Max ms range | P99.9 ms range | Full | Relabel rate | Coordinates | Max region | Traffic MiB |
|---|---|---|---|---|---|---|---|---|
| timeline | baseline | 72.614–101.714 | 0.009–0.020 | 15 | 0.004% | 782167 | 74360 | 380.496 |
| timeline | open_dense | 21.954–58.744 | 0.375–6.343 | 0 | 4.852% | 9851528 | 65536 | 3946.773 |
| priority | baseline | 1.651–17.388 | 0.027–0.100 | 4 | 3.394% | 312635 | 19589 | 107.856 |
| priority | open_dense | 0.885–9.697 | 0.017–0.123 | 1 | 4.304% | 292478 | 16384 | 98.402 |
| local | baseline | 0.094–0.295 | 0.004–0.007 | 0 | 0.729% | 27120 | 32 | 56.505 |
| local | open_dense | 0.018–0.321 | 0.003–0.006 | 0 | 0.729% | 27120 | 32 | 56.505 |
| churn | baseline | 0.025–0.337 | 0.003–0.010 | 0 | 0.128% | 3584 | 16 | 39.598 |
| churn | open_dense | 0.014–0.293 | 0.003–0.005 | 0 | 0.128% | 3584 | 16 | 39.598 |

Timeline total attempted-window nodes grow from 1,929,015 to 19,528,472; relabel events 19 to 21,823. Removing full range does not mean removing large-region work. Priority improves in several seeds but not uniformly: seed 826366246 maximum rises 1.738→9.697 ms and P99.9 0.036→0.123 ms. Local/churn coordinate/work/allocation metrics remain identical.

### Large, controls, holdout and long churn

Holdout rows are reserved seed 3512641005, evaluated after primary cases. All timings below are production; counts/bytes come from paired diagnostics.

| Case | Strategy | Max ms | P99.9 ms | Full | Max region | Traffic MiB |
|---|---|---|---|---|---|---|
| timeline 100000/100000 | baseline | 218.851 | 0.010 | 4 | 172867 | 261.145 |
| timeline 100000/100000 | open_dense | 47.931 | 1.112 | 0 | 65536 | 1055.728 |
| priority 100000/100000 | baseline | 2.407 | 0.131 | 0 | 4096 | 33.988 |
| priority 100000/100000 | open_dense | 2.822 | 0.110 | 0 | 4096 | 37.093 |
| local 100000/100000 | baseline | 0.113 | 0.006 | 0 | 32 | 12.849 |
| local 100000/100000 | open_dense | 0.111 | 0.008 | 0 | 32 | 12.849 |
| churn 100000/100000 | baseline | 0.039 | 0.010 | 0 | 8 | 9.564 |
| churn 100000/100000 | open_dense | 0.056 | 0.007 | 0 | 8 | 9.564 |
| churn 50000/500000 | baseline | 0.113 | 0.004 | 0 | 32 | 43.868 |
| churn 50000/500000 | open_dense | 0.130 | 0.004 | 0 | 32 | 43.868 |
| ascending 10000/100000 | baseline | 0.104 | 0.022 | 0 | 0 | 12.654 |
| ascending 10000/100000 | open_dense | 0.199 | 0.025 | 0 | 0 | 12.654 |
| descending 10000/100000 | baseline | 0.066 | 0.022 | 0 | 0 | 9.155 |
| descending 10000/100000 | open_dense | 1.033 | 0.022 | 0 | 0 | 9.155 |
| random 10000/100000 | baseline | 0.114 | 0.029 | 0 | 16 | 14.176 |
| random 10000/100000 | open_dense | 70.751 | 0.041 | 0 | 65536 | 65.803 |
| equal 10000/100000 | baseline | 0.092 | 0.022 | 0 | 0 | 12.654 |
| equal 10000/100000 | open_dense | 0.054 | 0.020 | 0 | 0 | 12.654 |
| duplicates 10000/100000 | baseline | 4.594 | 0.035 | 0 | 8192 | 32.571 |
| duplicates 10000/100000 | open_dense | 38.061 | 0.154 | 1 | 64508 | 67.811 |
| alternating 10000/100000 | baseline | 30.688 | 3.172 | 1 | 65536 | 926.641 |
| alternating 10000/100000 | open_dense | 38.500 | 3.234 | 1 | 65536 | 926.641 |
| hotspot 10000/100000 | baseline | 0.252 | 0.023 | 0 | 0 | 9.155 |
| hotspot 10000/100000 | open_dense | 0.228 | 0.026 | 0 | 0 | 9.155 |
| holdout timeline 10000/100000 | baseline | 92.361 | 0.011 | 3 | 74198 | 74.953 |
| holdout timeline 10000/100000 | open_dense | 28.362 | 0.366 | 0 | 32768 | 482.995 |
| holdout priority 10000/100000 | baseline | 7.480 | 0.066 | 1 | 16199 | 23.023 |
| holdout priority 10000/100000 | open_dense | 3.603 | 0.018 | 0 | 8192 | 20.229 |
| holdout local 10000/100000 | baseline | 0.012 | 0.003 | 0 | 16 | 11.346 |
| holdout local 10000/100000 | open_dense | 0.018 | 0.003 | 0 | 16 | 11.346 |
| holdout churn 10000/100000 | baseline | 0.108 | 0.003 | 0 | 8 | 7.892 |
| holdout churn 10000/100000 | open_dense | 0.095 | 0.004 | 0 | 8 | 7.892 |
| holdout churn 50000/500000 | baseline | 1.689 | 0.005 | 0 | 16 | 43.871 |
| holdout churn 50000/500000 | open_dense | 0.271 | 0.008 | 0 | 16 | 43.871 |

Large timeline removes four full events and lowers max 218.851→47.931 ms, but P99.9 rises 0.010→1.112 ms; relabels grow 5→6,199. Holdout timeline similarly removes three full events and lowers max 92.361→28.362 ms while P99.9 rises 0.011→0.366 ms and allocation traffic rises about 6.4×.

Medium random creates 1,101 repairs and region 65,536 instead of eight repairs/region 16; max 0.114→70.751 ms. Medium duplicates introduce one full event/region 64,508 versus none/8,192, max 4.594→38.061 ms. These deterministic work increases are material regressions, not timer noise. Alternating coordinate/work totals remain identical, with its existing full event and high traffic; measured maximum variation is not attributed to a changed algorithm path. Ascending/descending/equal/hotspot retain coordinate/work metrics.

## Storage/depth tradeoffs

Final-coordinate snapshots are not peak or lifetime ceilings. All required per-scenario P50/P95/P99/P99.9/max, work, depth mean/P95/P99/max, LK1 mean/max, resident bytes, peak library allocation and cumulative traffic are in [the complete summary CSV](../benchmarks/results/tail-cost-research-full-summary.csv). Growth CSV retains eleven checkpoints per run. No metric is inferred from diagnostic timing.

Representative final storage follows; MiB = 2^20 bytes, peak covers initialization plus measured operations.

| Case | Strategy | Depth mean/P95/P99/max | LK1 mean/max | Resident Paths MiB | LK1 payload MiB | Peak library MiB |
|---|---|---|---|---|---|---|
| timeline 10000/100000 s17 | baseline | 3.662/4/5/5 | 35.296/46 | 7.411 | 3.026 | 16.897 |
| timeline 10000/100000 s17 | open_dense | 3.816/6/6/6 | 36.529/54 | 7.352 | 3.131 | 12.360 |
| priority 10000/100000 s17 | baseline | 1.388/3/5/6 | 17.104/54 | 1.024 | 0.335 | 2.910 |
| priority 10000/100000 s17 | open_dense | 1.361/3/5/6 | 16.890/54 | 1.018 | 0.331 | 3.230 |
| timeline 100000/100000 s17 | baseline | 3.831/4/4/4 | 36.646/38 | 15.788 | 6.287 | 41.060 |
| timeline 100000/100000 s17 | open_dense | 3.371/6/6/6 | 32.965/54 | 13.215 | 5.655 | 25.342 |
| churn 50000/500000 s17 | baseline | 2.387/5/6/6 | 25.100/54 | 3.104 | 1.201 | 5.413 |
| churn 50000/500000 s17 | open_dense | 2.387/5/6/6 | 25.100/54 | 3.104 | 1.201 | 5.413 |
| random 10000/100000 s17 | baseline | 2.717/3/3/6 | 27.733/54 | 7.089 | 2.909 | 12.125 |
| random 10000/100000 s17 | open_dense | 2.220/4/5/6 | 23.761/54 | 6.369 | 2.493 | 14.585 |
| duplicates 10000/100000 s17 | baseline | 2.346/4/5/6 | 24.767/54 | 6.586 | 2.598 | 11.623 |
| duplicates 10000/100000 s17 | open_dense | 3.641/4/5/6 | 35.130/54 | 8.739 | 3.685 | 13.774 |
| timeline 10000/100000 s3512641005 | baseline | 3.659/4/5/5 | 35.273/46 | 7.401 | 3.020 | 16.920 |
| timeline 10000/100000 s3512641005 | open_dense | 3.605/5/5/6 | 34.837/54 | 7.017 | 2.983 | 14.854 |
| priority 10000/100000 s3512641005 | baseline | 1.431/3/4/6 | 17.446/54 | 0.994 | 0.331 | 2.959 |
| priority 10000/100000 s3512641005 | open_dense | 2.778/5/6/6 | 28.228/54 | 1.343 | 0.535 | 2.376 |
| churn 50000/500000 s3512641005 | baseline | 2.373/5/6/6 | 24.988/54 | 3.052 | 1.180 | 5.349 |
| churn 50000/500000 s3512641005 | open_dense | 2.373/5/6/6 | 24.988/54 | 3.052 | 1.180 | 5.349 |

Timeline final mean LK1 is not uniformly larger: dense packing may reduce average encoding while increasing endpoint congestion/work. It is not a compactness improvement claim. Duplicate LK1 mean grows 24.767→35.130 and holdout priority 17.446→28.228. Direct escape screens reach depth seven/eight and LK1 maxima 62/70 instead of 54.

### Long-lived churn

Both 50k/500k seeds retain identical coordinates, repair counts, resident storage and every checkpoint growth value between baseline/dense. No candidate-specific storage drift is observed in these frozen traces. This is not proof for all long-lived applications.

| Seed | Strategy | Operation | Resident | Depth mean | Max depth | LK1 mean | Path MiB |
|---|---|---|---|---|---|---|---|
| 17 | baseline | 0 | 50000 | 1.356 | 2 | 16.848 | 2.425 |
| 17 | baseline | 250000 | 50017 | 2.203 | 6 | 23.621 | 2.947 |
| 17 | baseline | 500000 | 50188 | 2.387 | 6 | 25.100 | 3.104 |
| 17 | open_dense | 0 | 50000 | 1.356 | 2 | 16.848 | 2.425 |
| 17 | open_dense | 250000 | 50017 | 2.203 | 6 | 23.621 | 2.947 |
| 17 | open_dense | 500000 | 50188 | 2.387 | 6 | 25.100 | 3.104 |
| 3512641005 | baseline | 0 | 50000 | 1.356 | 2 | 16.848 | 2.425 |
| 3512641005 | baseline | 250000 | 49856 | 2.197 | 6 | 23.577 | 2.933 |
| 3512641005 | baseline | 500000 | 49528 | 2.373 | 6 | 24.988 | 3.052 |
| 3512641005 | open_dense | 0 | 50000 | 1.356 | 2 | 16.848 | 2.425 |
| 3512641005 | open_dense | 250000 | 49856 | 2.197 | 6 | 23.577 | 2.933 |
| 3512641005 | open_dense | 500000 | 49528 | 2.373 | 6 | 24.988 | 3.052 |

## Correctness/OOM validation

Existing test sources unchanged. Every compiled strategy executes the entire original 13-test suite, including mixed update/remove/reinsert, equal-value stability, locate/find, deep Paths, parser/LK1 golden/order equivalence, soak, contract/OOM regressions, benchmark and frozen workload smoke. Seven additional OOM tests produce 98 CTest tests.

New congested depth-six fixtures sweep every allocation failure before successful insertion for an interior item, comparator-equal item and near-end item. Every failure requires OOM, NULL node output, original size/items/Paths unchanged, strict Path and independent key/serial order, locate/find consistency, AVL validity, unchanged live blocks/bytes, and zero library live bytes after destruction.

| Strategy | Interior failures | Equal failures | Near-end failures |
|---|---|---|---|
| baseline |167|167|49|
| direct7 |4|4|4|
| direct8 |4|4|4|
| depth_bias |84|84|49|
| equal8 |45|45|49|
| open_range |167|167|49|
| open_dense |167|167|49|

The original broader expanded repair/full-range OOM regressions also run for each variant. The new near-end fixture does not force every large candidate window seen in performance cases; tests plus explicit prepare/validate/no-allocation-commit inspection support atomicity, not exhaustive proof for all resource states.

Local GCC 16.2.0 C17 Release: 98/98, warnings as errors. Local MSVC 19.51 x64 Debug: 98/98; Release: 98/98; RelWithDebInfo with AddressSanitizer: 98/98. All three baseline examples also pass. The five CI configurations cap build parallelism at four to avoid unbounded simultaneous candidate compilation, enable research targets and run all tests, examples and distribution/amalgamation consumers; complete performance capture remains local, outside CI. Delivery reports the exact final CI run and sanitizer result. No ordinary public API, parser, Path or LK1 implementation was modified.

## Observed

- All traced cliffs have valid direct candidates and successful range generation; nonfinal depth acceptance fails.
- Failed preparation/destruction is a material part of synchronous cost.
- Dense open-range preparation removes primary/holdout/large timeline full events in this campaign.
- It raises frequent repair work, high-percentile cost and allocation traffic; random/duplicates regress seriously.
- Local/churn structural/storage measurements remain unchanged.

## Interpretation

The generator and depth-improvement rule jointly amplify a near-end congestion trigger. Avoiding a full-range label is not sufficient: dense coordinate redistribution spends spacing and creates recurring congestion. The tested strategies are compatible in public semantics but fail the multi-workload acceptance criteria. Promoting one would exchange one latency hazard for others.

## Unknown

- Repeated timing captures, randomized execution order, other hardware, real application concurrency/load, different lifespan/distributions.
- A defensible amortized bound or universally useful spacing/acceptance policy.
- Whether a narrower, independently justified open-range condition can preserve tail benefits without recurring repair; not tested or recommended here.
- Allocation-internal timing and peak process RSS; phase boundaries do not provide them.

## Complexity and next decision

AVL comparator search is unchanged. Candidate preparation remains proportional to affected nodes and generated Path sizes; repeated rejected windows and full-range replacement can still be linear or worse in total Path work. No worst-case or formal amortized insertion bound is added.

Direct escape is a small predicate but spends depth; bias adds selection branches; equal acceptance changes a policy predicate; open-range adds a bounded sequential preparation helper using existing managed endpoint generators. Research infrastructure is opt-in and private. Its complexity is justified as evidence tooling, not as production behavior. No production promotion, release or architecture redesign is justified by this branch. Keep v3.1 default and preserve these negative findings for an independent review.

## Reproduction and scope

Configure research with CMake tests/benchmarks enabled; build; run CTest. From repository root, a clean tracked source state and a fresh output directory:

```text
python benchmarks/run_tail_research.py --build build/tail-research --phase full --strategies baseline open_dense --output <fresh-directory>
```

The driver refuses output overwrite and tracing during A/B capture. Full metadata identifies the frozen source and binaries. Screen captures explicitly retain earlier dirty state. No original evidence files, README, public API, version, Path comparison/display/LK1, ownership, comparator binding, installed targets or packaging format changed. Main, Stable tag/release, Issue #2 and repository metadata are outside delivery scope. Only the research branch is pushed for review.
