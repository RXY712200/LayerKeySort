# Theory-guided ordering research: Phase 2 results

## Decision: B — Tradeoff only

The finite-family capacity model is correct and the prototype passes correctness and allocation-failure validation. It is not a production candidate. It removes the measured timeline full-range events and improves priority/duplicate tails, but replaces rare timeline cliffs with recurring regional rewrites, much worse P99.9, increased allocation traffic, and larger coordinates. Default v3.1 remains unchanged.

No second prototype was built. No Phase 3 implementation is authorized by this result.

## Frozen implementation and environment

- Implementation commit: `2ea0a37f231f8009c398d442995a1c456fde8898` on `v3-theory-guided-ordering-research`.
- Production baseline: main `c33afa2ac08248bffa6dde51bd590a5d799b394a`, stable v3.1 release commit `dfa9562b9471947cfbd4ee1d1750a59434be83e2`.
- Windows 11 Pro 10.0.26300, Ryzen 9 9955HX (16 cores / 32 logical processors), 15922848 KiB RAM; GCC 16.2.0 UCRT x64.
- Both builds: C17, Release `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`, tests/benchmarks ON; only `LKS_RESEARCH_SLACK` OFF/ON differs.
- Final capture: 20 screening scenarios, 40 timed/diagnostic pairs, 80 process runs. Baseline rerun in the same environment, sequential baseline pair then prototype pair per scenario. One pass; no affinity, power isolation, confidence intervals or universal speed claim.
- Five primary seeds `17,305419896,2654435769,826366246,12648430`; reserved holdout `3512641005`. Workload generation, RNG, seeds, comparator and reference oracle are unchanged.
- The wrapper imports `benchmarks/run_workloads.py` pairing/oracles. The simulator changes only add private counter fields to output. Metadata records exact source/binary hashes and committed implementation head.
- Production latency comes from the uninstrumented executable, surrounding managed insertion only. Diagnostic latency includes capacity-membership counting and is not used for speed conclusions. P99.9 uses the frozen nearest-rank definition.
- Initial prefix is untimed. Event counts/rewrites and attempted-node totals below cover the measured operations. Cumulative generated-Path, rewritten-byte and capacity counters include the initial prefix. Requested allocation traffic below subtracts the prefix; resident/peak bytes are library-requested bytes, not RSS or allocator overhead.
- A preliminary screen was preserved outside the repository before the final committed-source capture. Its timings are not mixed into these tables.

## Exact finite Path capacity/slack model

For each non-ZERO reference, consider D=1..min(reference depth,6). Fix direction, all D absolute levels, and the first D-min(D,2) slots. The last min(D,2) slots enumerate a big-endian base-65536 rank. For a variable negative root, use the digit `65535-slot`. Family size is 65536 (D=1) or 2^32 (D>=2). These are finite equal-depth families, not an assumption that arbitrary Path space is an integer interval.

Within the family, rank order equals `lks_path_compare()` because levels and earlier slots are fixed; the first differing variable slot controls order. The transformed negative root gives exactly its reversed order. Families copy representable levels without arithmetic.

Binary search against the actual unchanged exterior Paths gives `lower = first F(rank) > L` and `upper = first F(rank) >= R`; absent bounds clip to 0/family size. The exact usable capacity is `C=max(0,upper-lower)`. Full direction/level/slot/parent-prefix comparison is used, including bounds outside the family. No display/LK1 text comparison participates.

Count n resident nodes in the contiguous logical region. P=n+1 includes the new item. Before repair, diagnostic membership counting gives the exact number K of these nodes already occupying the chosen family; residents outside the family are also evacuated. After repair, exactly P family coordinates are occupied and C-P remain free. No exterior resident can occupy the open interval.

Require `s=floor(C/(P+1)) >=16`. Assign ranks `lower+s*(i+1)-1`, i=0..P-1. There are s-1 free ranks before the first item and in each internal gap, and at least s after the last. Thus every gap retains at least 15 enumerable free coordinates at commit. Rank/products fit uint64_t (C<=2^32), including a 32-bit size_t build. Reject impossible populations before addition/multiplication.

At logical scales 8,16,32,...,whole collection, try increasingly deep families anchored first at the predecessor then the successor. Only a reserve-qualified family prepares replacement Paths; otherwise expand without speculative Path generation. Scratch allocation and range inspection still occur. Keep the ordinary v3.1 repair trigger/endpoint policy. If no family qualifies at full range, retain the original sparse bulk fallback and its depth-improvement condition.

Prepare all replacement Paths, validate strict order and exterior bounds, allocate the new node, then perform the existing allocation-free key replacement/link commit. Resident rank order and AVL shape invariants remain intact. There is no persistent slack metadata to roll back: accounting is recomputed. Exact removal is unchanged, allocation-free, and does not relabel survivors.

This reserve is an immediate, stateless invariant. It does not guarantee how long a hotspot retains reserve, assign durable ownership to logical scale boundaries, predict insertion demand, or provide an amortized bound. Direct gap candidates may use coordinates outside the selected family. These limits matter in the measured result. See [implementation specification](../research/README.md).

## Same-environment screening results

Medium = 10,000 initial / 100,000 operations; large timeline = 100,000 / 100,000; long churn = 50,000 / 500,000. All values are baseline → slack. Timing is milliseconds.

| Scenario / seed | Full events | Maximum ms | P99.9 ms | Relabel events | Resident coordinates rewritten |
|---|---:|---:|---:|---:|---:|
| timeline medium / 17 | 3 → 0 | 80.886 → 11.110 | 0.006836 → 0.495605 | 3 → 481 | 156296 → 759664 |
| timeline medium / 305419896 | 3 → 0 | 86.373 → 11.128 | 0.015625 → 0.533936 | 4 → 433 | 156561 → 761320 |
| timeline medium / 2654435769 | 3 → 0 | 85.087 → 27.113 | 0.008545 → 0.510254 | 5 → 287 | 156708 → 869024 |
| timeline medium / 826366246 | 3 → 0 | 86.237 → 14.963 | 0.016602 → 0.557373 | 4 → 422 | 156700 → 841744 |
| timeline medium / 12648430 | 3 → 0 | 87.333 → 26.844 | 0.006592 → 0.520264 | 3 → 351 | 155902 → 896288 |
| timeline medium / 3512641005 | 3 → 0 | 80.639 → 10.281 | 0.006104 → 0.501953 | 6 → 554 | 156421 → 749416 |
| timeline large / 17 | 4 → 0 | 211.177 → 25.931 | 0.007080 → 0.497803 | 5 → 382 | 568153 → 910744 |
| priority medium / 17 | 1 → 0 | 8.714 → 0.033 | 0.100342 → 0.005615 | 2469 → 1905 | 81127 → 15240 |
| priority medium / 305419896 | 2 → 0 | 16.093 → 0.133 | 0.052734 → 0.004395 | 1806 → 1617 | 67001 → 12936 |
| priority medium / 2654435769 | 1 → 0 | 7.837 → 0.034 | 0.066895 → 0.004883 | 2360 → 1793 | 74755 → 14344 |
| priority medium / 826366246 | 0 → 0 | 1.909 → 0.038 | 0.037109 → 0.005371 | 3409 → 1756 | 52536 → 14048 |
| priority medium / 12648430 | 0 → 0 | 1.523 → 0.033 | 0.024902 → 0.005371 | 2687 → 1702 | 37216 → 13616 |
| priority medium / 3512641005 | 1 → 0 | 7.206 → 0.033 | 0.068359 → 0.005615 | 2354 → 1897 | 74743 → 15176 |
| random medium / 17 | 0 → 0 | 0.054 → 0.084 | 0.020996 → 0.019531 | 8 → 450 | 72 → 3776 |
| duplicates medium / 17 | 0 → 0 | 3.949 → 0.126 | 0.034424 → 0.028809 | 6915 → 10553 | 106224 → 85888 |
| alternating medium / 17 | 1 → 1 | 27.603 → 6.349 | 3.132568 → 1.857910 | 35664 → 19812 | 4280851 → 9241859 |
| local medium / 17 | 0 → 0 | 0.121 → 0.036 | 0.005859 → 0.002930 | 666 → 643 | 5520 → 5144 |
| churn medium / 17 | 0 → 0 | 0.132 → 0.017 | 0.003174 → 0.002930 | 88 → 88 | 704 → 704 |
| churn long / 17 | 0 → 0 | 0.038 → 0.214 | 0.003906 → 0.005615 | 777 → 765 | 6296 → 6120 |
| churn long / 3512641005 | 0 → 0 | 0.224 → 0.131 | 0.004883 → 0.005615 | 817 → 792 | 6616 → 6336 |

### Timeline: large-event elimination does not pass adoption

Across six medium seeds, full-range events fall 18→0. Maximum insertion latency ranges 80.639–87.333→10.281–27.113 ms. But P99.9 worsens 0.006104–0.016602→0.495605–0.557373 ms. Relabel events increase 25→2,528; existing coordinate rewrites 938,588→4,877,456 (5.20x); requested traffic 477,573,752→1,207,361,984 bytes (2.53x); old coordinate bytes rewritten 76,478,672→546,342,544 (7.14x).

The large seed-17 trace removes all four full events and reduces maximum 211.177→25.931 ms, but P99.9 increases 0.007080→0.497803 ms and relabel events 5→382. Traffic falls 273.83→224.23 MB in this one trace; this does not offset the multi-seed medium regressions.

For timeline prefixes, no initial repair occurred, so failed preparation is exactly identifiable: medium baseline generated 2,315,079 Paths, of which 1,376,466 were rejected rather than committed. The prototype generated 4,879,984 Paths and rejected none after preparation: it avoids speculative allocation, but generates more accepted replacement Paths overall. Large baseline discarded 917,590 prepared Paths; prototype discarded none. Capacity-rejected regions still cost scanning/scratch, visible in attempted-node totals.

The meaningful structural failure is that immediate spare capacity is insufficient to amortize repeated focused demand. Exhaustion of the two-slot family in a bounded depth budget forces repeated enlarged-region repacking. The observed rewrites show this cost; this is not a correctness failure or proof that every slack-aware approach is infeasible.

### Other workloads

- Priority: no new latency cliff. Six medium seeds remove five total full events; regional maximum drops to 8 nodes. Recurring rewrite/traffic falls, but some final mean coordinate/key sizes increase substantially (e.g. seed17 LK1 17.10→27.56 characters).
- Random: maximum region 16→128 and events 8→450, but no millisecond-scale cliff appeared in this screen. The worst measured maximum is 0.054→0.084 ms. This is extra recurring work, not the rejected open_dense large random cliff.
- Duplicates: maximum region 8192→16 and maximum latency improves; events rise 6915→10553 and requested traffic 34.15→35.80 MB. Mean depth 2.346→4.193 and LK1 24.77→39.54 characters make the storage tradeoff material.
- Alternating: lower maximum/P99.9, but rewrites increase 4,280,851→9,241,859 and requested traffic 971.65→2049.90 MB. One full-range event remains in each variant.
- Local/churn: no structural cliff. Local rewrites fall slightly; ordinary churn counts are unchanged. Sub-microsecond central timing differences are not interpreted as improvements.

## Preparation/work/traffic accounting

This table separates cumulative prepared Paths and old coordinate bytes from measured attempted-node sums and requested traffic. Capacity rejection means no replacement Path was generated for that scale. Baseline generated counts include accepted and rejected preparations; no separate historical failure counter is invented for priority/controls with repaired initial prefixes.

| Scenario / seed | Generated Paths (cumulative) | Attempted nodes (measured sum) | Capacity rejections (slack, cumulative) | Old coordinate bytes rewritten (cumulative, MB) | Requested traffic (measured, MB) |
|---|---:|---:|---:|---:|---:|
| timeline medium / 17 | 385690 → 760145 | 385648 → 1515480 | 2365 | 12.73 → 85.10 | 77.12 → 189.24 |
| timeline medium / 305419896 | 386015 → 761753 | 385969 → 1519176 | 2535 | 12.76 → 85.28 | 80.09 → 189.57 |
| timeline medium / 2654435769 | 386139 → 869311 | 386092 → 1735752 | 1964 | 12.77 → 97.33 | 81.59 → 213.87 |
| timeline medium / 826366246 | 386095 → 842166 | 386052 → 1680112 | 2343 | 12.77 → 94.29 | 78.69 → 207.70 |
| timeline medium / 12648430 | 385296 → 896639 | 385254 → 1789768 | 2014 | 12.70 → 100.40 | 81.49 → 219.99 |
| timeline medium / 3512641005 | 385844 → 749970 | 385797 → 1494400 | 2248 | 12.75 → 83.95 | 78.59 → 186.99 |
| timeline large / 17 | 1485748 → 911126 | 1485681 → 1818432 | 2527 | 48.67 → 101.82 | 273.83 → 224.23 |
| priority medium / 17 | 157350 → 24849 | 142919 → 15240 | 0 | 6.07 → 1.96 | 26.26 → 13.22 |
| priority medium / 305419896 | 151510 → 21717 | 138945 → 12936 | 0 | 4.93 → 1.64 | 27.33 → 11.61 |
| priority medium / 2654435769 | 154345 → 23697 | 132955 → 14344 | 0 | 5.84 → 1.82 | 24.98 → 12.36 |
| priority medium / 826366246 | 90903 → 23292 | 77800 → 14048 | 0 | 4.53 → 1.80 | 18.97 → 12.61 |
| priority medium / 12648430 | 77749 → 22707 | 52936 → 13616 | 0 | 3.80 → 1.74 | 15.56 → 12.17 |
| priority medium / 3512641005 | 148349 → 24696 | 130839 → 15176 | 0 | 5.74 → 1.93 | 24.14 → 12.98 |
| random medium / 17 | 7091 → 4496 | 80 → 3952 | 7 | 0.17 → 0.38 | 14.86 → 15.73 |
| duplicates medium / 17 | 176077 → 104145 | 157128 → 87352 | 183 | 8.87 → 9.37 | 34.15 → 35.80 |
| alternating medium / 17 | 8576411 → 9690837 | 8291595 → 18329275 | 97472 | 317.24 → 1082.47 | 971.65 → 2049.90 |
| local medium / 17 | 6401 → 5787 | 5712 → 5144 | 0 | 0.44 → 0.41 | 11.85 → 11.89 |
| churn medium / 17 | 792 → 792 | 704 → 704 | 0 | 0.05 → 0.05 | 8.31 → 8.32 |
| churn long / 17 | 7162 → 6885 | 6376 → 6120 | 0 | 0.51 → 0.50 | 46.00 → 46.18 |
| churn long / 3512641005 | 7523 → 7128 | 6696 → 6336 | 0 | 0.54 → 0.51 | 46.00 → 46.17 |

## Path / storage profile

Final resident metrics, baseline → slack; MB = 1,000,000 bytes. Path bytes include object and allocated slot/level capacity. LK1 excludes NUL. Peak is library-requested total storage high-water over the whole process, including initial prefix and temporary preparations. No fixed depth/storage guarantee is implied.

| Scenario / seed | Depth mean | Depth P95/P99/max | LK1 mean/max chars | Resident Path MB | Peak library MB |
|---|---:|---|---|---:|---:|
| timeline medium / 17 | 3.662 → 4.976 | 4/5/5 → 6/6/6 | 35.30/46 → 45.80/54 | 7.771 → 8.846 | 17.717 → 16.821 |
| timeline medium / 305419896 | 3.659 → 4.969 | 4/5/5 → 6/6/6 | 35.27/46 → 45.75/54 | 7.755 → 8.832 | 17.755 → 15.060 |
| timeline medium / 2654435769 | 3.659 → 4.264 | 4/5/5 → 5/5/6 | 35.27/46 → 40.12/54 | 7.764 → 8.853 | 18.634 → 21.198 |
| timeline medium / 826366246 | 3.667 → 4.970 | 4/5/5 → 6/6/6 | 35.34/46 → 45.76/54 | 7.813 → 8.881 | 17.784 → 17.280 |
| timeline medium / 12648430 | 3.664 → 5.013 | 4/5/5 → 6/6/6 | 35.31/46 → 46.10/54 | 7.775 → 8.853 | 18.590 → 21.270 |
| timeline medium / 3512641005 | 3.659 → 4.982 | 4/5/5 → 6/6/6 | 35.27/46 → 45.85/54 | 7.760 → 8.845 | 17.742 → 15.247 |
| timeline large / 17 | 3.831 → 3.523 | 4/4/4 → 6/6/6 | 36.65/38 → 34.18/54 | 16.555 → 15.124 | 43.054 → 28.995 |
| priority medium / 17 | 1.388 → 2.695 | 3/5/6 → 4/5/6 | 17.10/54 → 27.56/54 | 1.073 → 1.364 | 3.052 → 2.351 |
| priority medium / 305419896 | 2.933 → 2.172 | 4/4/6 → 3/5/6 | 29.46/54 → 23.38/54 | 1.426 → 1.216 | 4.225 → 2.176 |
| priority medium / 2654435769 | 1.622 → 2.404 | 4/5/6 → 4/5/6 | 18.97/54 → 25.23/54 | 1.098 → 1.252 | 2.681 → 2.205 |
| priority medium / 826366246 | 1.866 → 2.528 | 4/6/6 → 4/5/6 | 20.93/54 → 26.23/54 | 1.177 → 1.295 | 2.283 → 2.259 |
| priority medium / 12648430 | 1.921 → 2.356 | 4/6/6 → 4/5/6 | 21.37/54 → 24.85/54 | 1.190 → 1.262 | 2.154 → 2.225 |
| priority medium / 3512641005 | 1.431 → 2.614 | 3/4/6 → 4/5/6 | 17.45/54 → 26.91/54 | 1.043 → 1.301 | 3.102 → 2.256 |
| random medium / 17 | 2.717 → 2.938 | 3/3/6 → 5/6/6 | 27.73/54 → 29.51/54 | 7.434 → 7.666 | 12.714 → 12.946 |
| duplicates medium / 17 | 2.346 → 4.193 | 4/5/6 → 6/6/6 | 24.77/54 → 39.54/54 | 6.906 → 9.640 | 12.188 → 14.921 |
| alternating medium / 17 | 2.369 → 2.864 | 4/4/6 → 6/6/6 | 24.95/54 → 28.91/54 | 7.185 → 7.497 | 17.162 → 14.121 |
| local medium / 17 | 2.170 → 2.242 | 5/6/6 → 5/6/6 | 23.36/54 → 23.94/54 | 0.618 → 0.626 | 1.101 → 1.105 |
| churn medium / 17 | 1.925 → 1.944 | 4/5/6 → 4/5/6 | 21.40/54 → 21.55/54 | 0.593 → 0.595 | 1.079 → 1.081 |
| churn long / 17 | 2.387 → 2.425 | 5/6/6 → 5/6/6 | 25.10/54 → 25.40/54 | 3.255 → 3.281 | 5.676 → 5.702 |
| churn long / 3512641005 | 2.373 → 2.406 | 5/6/6 → 5/6/6 | 24.99/54 → 25.25/54 | 3.200 → 3.223 | 5.608 → 5.629 |

### Long-churn drift

Both 50k/500k seeds passed all 11 checkpoint reference-model/identity/order checks. Neither variant has a full-range event. Depth P95/P99/max ends at 5/6/6 in both variants.

- Primary17: initial resident Path bytes 2,542,368 in both; final 3,254,776→3,281,472 (+0.82% vs baseline). Final LK1 mean 25.099705→25.396987; depth mean 2.387463→2.424623. Rewrites 6296→6120.
- Holdout3512641005: same initial bytes; final 3,199,816→3,222,688 (+0.71%). Final LK1 mean 24.987563→25.249556; depth mean 2.373445→2.406194. Rewrites 6616→6336.
- Requested measured traffic stays near 46 MB per run. Both variants grow storage over time; the prototype is slightly higher, without a runaway divergence in this finite observation. No unbounded-duration conclusion follows.

## Correctness, OOM and production isolation

- Baseline GCC CTest 13/13; prototype GCC CTest 14/14. Prototype MSVC x64 Release and AddressSanitizer RelWithDebInfo each 14/14. All three examples run for each GCC variant.
- Model tests compare 24,000 virtual-family projections against public Path comparison, plus exhaustive 65,536-slot boundary enumeration for qualifying positive/negative fixtures and explicit parent/two-slot/empty interval fixtures.
- Focused OOM sweeps cover 264 positive/interior, 264 negative/interior and 270 positive/equal failing allocation points, then a successful insertion. Every failure returns OOM/NULL and preserves membership, identity/Path lookup, AVL balance, count, live blocks/bytes. Successful exact removal consumes zero allocation attempts and preserves survivor coordinates.
- Existing rollback tests continue to run. One private baseline policy assertion expected a region larger than eight; the prototype legitimately succeeds at eight. Only that policy assertion selects `slack_preparations>0` under the research macro. Its unchanged per-failure rollback checks remain active; baseline still requires expansion.
- Every captured pair passed the frozen comparator/order/identity/membership oracle and zero-live-allocation cleanup. Timed/diagnostic coordinate digests match within each strategy. Logical trace digest matches across strategies. Coordinate digests across different algorithms need not match.
- Source-level default algorithm branches are preserved under `#ifndef LKS_RESEARCH_SLACK`. GCC `.text` from default tree.c equals original main tree.c compiled with the same current private diagnostic declarations/options: SHA256 `04b6cb4cd21ead377d64fb58beb3178bf55b4cdd6dba9bdcdf6344696a544aa1`. Private diagnostic return-layout extension is not a public ABI change; this is not a claim that all old binary files have identical hashes.
- Five-platform CI runs normal baseline tests plus bounded prototype 14-test validation under MSVC, GCC, Clang, Clang ASan/UBSan and AppleClang. It does not run the 20/52-scenario campaign. The delivery report supplies the final run URL and actual conclusions.
- The first delivery CI passed both test suites but caught an amalgamation include-layout error. Private research declarations were moved into `src/lks_slack_research_internal.h`, which the unchanged bundler recognizes. This is an include-layout fix only; captured implementation/measurements remain pinned to the original implementation commit above. No algorithm or workload was retuned.

## Adoption disposition and Phase 3 recommendation

| Criterion | Result |
|---|---|
| Reduce timeline full-range/max tails | Pass for all screened timeline seeds |
| Avoid moving a major cliff into random/duplicates/priority | Pass in this finite screen |
| Avoid exploding recurring work / allocation traffic | **Fail**: timeline event/rewrites and alternating traffic |
| Keep Path/LK1 growth controlled | Bounded in this screen, but material priority/duplicate storage regressions |
| Correctness and failure atomicity | Pass |

Outcome is **B — Tradeoff only**, not A or model-infeasible C. The prototype changes a real capacity/reserve decision rather than merely a depth/stride/equal-acceptance threshold, but the reserve alone is not sufficient for production adoption.

The optional chunk-prefix/suffix prototype was not built: it does not inherently supply a demonstrated reserve-lifetime invariant, and flattened prefix rewrites would still need to be counted. This screen does not justify a second speculative implementation. The full 52-scenario campaign was not run because the required screening already fails the recurring-work adoption gate.

For Phase 3, recommend retaining v3.1 and recording this rejected tradeoff. Any narrowly scoped follow-up should first explain how scale-owned reserve survives repeated focused demand without transferring prefix rewrites/allocations elsewhere. Do not tune reserve/depth/window constants solely to fit these seeds, declare 3.2.0, or begin a V4 redesign from this result. No Phase 3 work is started.

## Evidence and reproduction

- [Capacity/transaction specification and commands](../research/README.md)
- [Metadata and immutable source/binary hashes](../benchmarks/results/theory-slack-screen-metadata.json)
- [Per-scenario summary](../benchmarks/results/theory-slack-screen-summary.csv)
- [All growth checkpoints](../benchmarks/results/theory-slack-screen-growth.csv)
- [Sampled worst events](../benchmarks/results/theory-slack-screen-tail.csv)
- [Latency classes](../benchmarks/results/theory-slack-screen-latency-classes.csv)

Main, stable v3.1, public APIs, Path/LK1 semantics and Issue #2 are outside this research delivery. No merge, version bump, tag, release or issue edit is included.
