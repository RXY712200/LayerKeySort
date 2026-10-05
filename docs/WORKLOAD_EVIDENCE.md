# Post-release synthetic workload evidence — v3.1.0

**Recommendation: investigate compatible tail-cost improvements in future 3.2 research.**
Recent-tail timeline runs produced rare but repeatable full-range relabel pauses. This is workload evidence, not a production correctness defect or a reason to declare a breaking V4. No production algorithm, API, version, Path/LK1 encoding, packaging, or release behavior changed.

## Scope, capture, and reproducibility

- Production baseline: `dfa9562b9471947cfbd4ee1d1750a59434be83e2` (`v3.1.0`).
- Frozen simulator commit: `640fe49e50df3aa21f50569b5ea3b2bf1d54a89f`.
- Windows 11 Pro 10.0.26300, Ryzen 9 9955HX (16 cores / 32 threads), 15,922,848 KiB visible RAM; GCC 16.2.0 UCRT64, C17 Release `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`.
- 52 scenario pairs / 104 executions; 3,809,377 measured insertions in each build. Initial setup is untimed. Production and diagnostic traces and all checkpoint coordinate digests matched.
- Primary seeds: `17`, `305419896`, `2654435769`, `826366246`, `12648430`. Reserved holdout: `3512641005` (`0xD15EA5ED`), run after primary scenarios and controls without changing definitions.
- Small: four profiles at 1k initial / 10k operations, first two primary seeds. Medium: four profiles at 10k / 100k, all five primary seeds. Large: all four at 100k / 100k, seed 17 only. Long churn: 50k / 500k, seed 17 and holdout. All seven controls at 1k / 10k and 10k / 100k, seed 17. Holdout additionally covers each medium profile.
- Each run checks operation zero, ten fixed intervals, and completion: 1,144 checkpoint rows total. Default bounded tail output has 2,960 rows; no full raw traces are committed.

Exact model assumptions, CLI, oracle, clock, sampling, and counter interpretation are in [WORKLOADS.md](../benchmarks/WORKLOADS.md). Exact binary hashes, compiler output, plan and capture time are in [metadata](../benchmarks/results/v3.1-workload-evidence-metadata.json). Captured CSVs are additive; historical benchmark files are unchanged.

## Synthetic profiles

| Profile | Model assumptions | Purpose |
| --- | --- | --- |
| Recent-tail timeline | 70% near-end new, 15% tail backfill, 5% broad interior new, 10% remove | Delayed/backfilled chronological data; growth about 0.8 items/operation |
| Priority/task | 35% new, 25% remove, 40% priority update; 32 equal-key buckets | Stable duplicate order with changing priorities; growth about 0.1 items/operation |
| Local reorder | 10% new, 10% remove, 80% update; updates 90% small numeric moves, 10% jumps | Long-lived editor/layer-like numeric-key activity; roughly steady population |
| Churn | 30% new, 30% remove, 40% update; redirected at +/-5% population band; occasional historical extremes | Mutation history much longer than population |

These are **synthetic application-like workloads**, not user research or real-user traces. Local movement is numeric distance, not exact rank distance. Queries remove the first equal item by key; they do not model an item-ID lookup facade. Comparator-visible keys change only after removal. Detailed key distributions and PRNG mapping are documented in the methodology.

## Correctness and measurement boundaries

**No correctness failures were observed.** A flat reference sorts resident identities by key then fresh insertion sequence, independently of the index and coordinates. At checkpoints it verifies size, all identities and membership, stable equality, strict Path order, and 32 first-equal locate/exact-Path find probes. All diagnostic runs returned library live bytes to zero on destroy. The oracle self-test rejects deliberately corrupted equality order and membership; Python smoke rejects incomplete/corrupted output and invalid scenario CLI inputs.

The timer surrounds managed insertion only. Reinsertion latency excludes the preceding locate/removal, key generation, reference sorting, profiling and Python orchestration. Separate diagnostic executables run identical traces and provide work deltas; their timings are instrumented, not production measurements. The wall-clock `timespec_get(TIME_UTC)` epoch-millisecond method matches the existing benchmark. Double granularity is approximately 0.244 microseconds; many central percentiles are near clock resolution. Timing noise and background scheduling were not isolated. Quantiles use nearest rank; P99.9 is withheld below 10,000 insertions. No observed maximum is a guarantee.

## Insertion latency

Representative seed 17, including all three initial-population scales. Central statistics are in **microseconds**; maximum is in **milliseconds**. Rounded digits preserve auditability, not measurement precision. Full per-seed data is in [summary.csv](../benchmarks/results/v3.1-workload-evidence-summary.csv).

| Profile | Initial | Operations | Inserts | Mean us | P50 us | P95 us | P99 us | P99.9 us | Max ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| timeline | 1000 | 10000 | 9010 | 0.524 | 0.488 | 0.732 | 1.465 | n/a | 0.038 |
| priority | 1000 | 10000 | 7490 | 0.707 | 0.488 | 0.977 | 3.174 | n/a | 0.173 |
| local | 1000 | 10000 | 9024 | 0.579 | 0.488 | 0.732 | 1.221 | n/a | 0.292 |
| churn | 1000 | 10000 | 7043 | 0.511 | 0.488 | 0.732 | 0.977 | n/a | 0.182 |
| timeline | 10000 | 100000 | 89943 | 2.769 | 0.977 | 1.709 | 2.686 | 9.033 | 91.089 |
| priority | 10000 | 100000 | 75234 | 1.115 | 0.488 | 1.221 | 3.418 | 102.295 | 9.754 |
| local | 10000 | 100000 | 89947 | 0.704 | 0.732 | 0.977 | 1.465 | 3.418 | 0.228 |
| churn | 10000 | 100000 | 70171 | 0.659 | 0.732 | 0.977 | 1.221 | 3.174 | 0.136 |
| timeline | 100000 | 100000 | 89943 | 9.105 | 0.977 | 2.197 | 4.150 | 9.521 | 225.062 |
| priority | 100000 | 100000 | 75063 | 1.843 | 0.732 | 1.953 | 4.395 | 149.414 | 3.094 |
| local | 100000 | 100000 | 89947 | 1.835 | 1.709 | 3.174 | 4.883 | 9.277 | 0.139 |
| churn | 100000 | 100000 | 70171 | 1.616 | 1.465 | 2.686 | 3.418 | 5.615 | 0.021 |

### Five-seed medium comparison

Counts below pool the five primary medium traces; latency ranges are **per-run** ranges, not pooled quantiles. Full-range is a disjoint histogram bin. Direct/relabel percentages use insertion/reinsertion count, not all actions.

| Profile | Inserts | Relabel rate | Full events | Existing nodes replaced | Largest region | P99.9 range us | Max range ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| timeline | 449785 | 0.00422% | 15 | 782167 | 74360 | 8.301–17.090 | 86.469–92.264 |
| priority | 375133 | 3.39373% | 4 | 312635 | 19589 | 21.729–102.295 | 1.577–17.435 |
| local | 449615 | 0.72907% | 0 | 27120 | 32 | 3.418–5.127 | 0.009–0.228 |
| churn | 350306 | 0.12760% | 0 | 3584 | 16 | 2.930–3.174 | 0.013–0.255 |

The timeline had three full-range events in **every** primary medium seed. The priority profile had four full events across five primary traces; one seed avoided them. Local/churn had no full events. Thus rarity alone does not make tail cost irrelevant, and a percentile such as P99.9 misses events below its rank cutoff.

### Relabel-size distribution — five primary medium seeds

| Profile | Direct | 1–8 | 9–32 | 33–128 | 129–512 | 513–4096 | 4097+ | Full |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| timeline | 449766 | 1 | 2 | 1 | 0 | 0 | 0 | 15 |
| priority | 362402 | 10854 | 1155 | 495 | 213 | 10 | 0 | 4 |
| local | 446337 | 3174 | 104 | 0 | 0 | 0 | 0 | 0 |
| churn | 349859 | 446 | 1 | 0 | 0 | 0 | 0 | 0 |

A relabel count is existing nodes whose coordinates were replaced; it excludes the new item. Each operation derives cumulative-counter deltas. Full-range events are separated from numeric bins. Local is <=128 existing nodes; larger is >128. Attempted node count sums all attempted windows and can exceed the population. It is not a per-operation maximum, and running maxima were not subtracted.

### Holdout

| Profile | Inserts | P50 us | P95 us | P99 us | P99.9 us | Max ms | Relabel events | Full | Largest region |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| timeline | 89891 | 0.977 | 1.465 | 2.197 | 10.010 | 84.952 | 6 | 3 | 74198 |
| priority | 74968 | 0.488 | 0.977 | 3.174 | 69.092 | 7.234 | 2354 | 1 | 16199 |
| local | 89973 | 0.732 | 0.977 | 1.221 | 3.418 | 0.176 | 670 | 0 | 16 |
| churn | 69896 | 0.732 | 0.977 | 1.221 | 2.686 | 0.190 | 89 | 0 | 8 |

The holdout reproduced three timeline full events and one priority full event. It did not reveal a new correctness or storage failure. It is one independent deterministic trace per scenario, not statistical validation of the model assumptions.

## Worst events and work correlation

The most expensive production events join to the same operation indices in the separate diagnostic run:

| Run | Profile | Operation | Resident before insert | Production ms | Diagnostic ms | Class | Replaced | Attempted sum | Expansions |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 29 | timeline | 91178 | 172867 | 225.062 | 281.805 | full | 172867 | 435003 | 15 |
| 29 | timeline | 65621 | 152358 | 200.161 | 243.761 | full | 152358 | 414494 | 15 |
| 29 | timeline | 39851 | 131844 | 195.480 | 243.283 | full | 131844 | 393980 | 15 |
| 29 | timeline | 13783 | 111020 | 95.512 | 123.623 | full | 111020 | 242084 | 14 |
| 13 | timeline | 80333 | 74022 | 92.264 | 124.235 | full | 74022 | 205086 | 14 |

For large timeline run 29, all four full-range operations replaced 111,020–172,867 existing coordinates. Instrumented full-event mean was 223.118 ms; direct-insert mean was 0.001311 ms, with direct max 0.188477 ms. The full sample is small (four events) and is not a distribution estimate. Its cost and corresponding production pauses nevertheless align strongly with region size and expansion/generation work. Medium timeline tail events repeat across all seeds. Not every smaller spike was a relabel: local/churn also had isolated direct-insert outliers; allocator/scheduling causes are not isolated by this harness.

The [tail CSV](../benchmarks/results/v3.1-workload-evidence-tail.csv) retains bounded worst latency/region/first-full samples, candidate **committed** depth, comparisons, search steps, rotations, attempts, expansions and generated Paths. Production event work fields are stub zeros marked `category=unavailable`, not measurements; summary work fields are blank. Join by run/operation to instrumented evidence. This bounded sample is selection-biased and must not be used to estimate whole-run event frequency; use the histogram instead. [Latency class rows](../benchmarks/results/v3.1-workload-evidence-latency-classes.csv) contain whole-run diagnostic class summaries.

## Path, LK1, and memory

Final seed-17 checkpoints (including setup/history), bytes are requested library allocation bytes. Path bytes include object and step capacity. Peak is peak **library live allocation**, not process RSS or allocator-resident memory. LK1 payload excludes NUL and does not include database overhead.

| Profile | Initial | Final population | Depth mean | P95 | P99 | Max | LK1 mean bytes | Max | Path MiB | LK1 payload MiB | Peak library MiB | History traffic MiB |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| timeline | 10000 | 89886 | 3.662 | 4 | 5 | 5 | 35.296 | 46 | 7.411 | 3.026 | 16.897 | 73.547 |
| priority | 10000 | 20539 | 1.388 | 3 | 5 | 6 | 17.104 | 54 | 1.024 | 0.335 | 2.910 | 25.041 |
| local | 10000 | 9923 | 2.170 | 5 | 6 | 6 | 23.361 | 54 | 0.589 | 0.221 | 1.050 | 11.304 |
| churn | 10000 | 10099 | 1.925 | 4 | 5 | 6 | 21.396 | 54 | 0.565 | 0.206 | 1.029 | 7.923 |
| timeline | 100000 | 179886 | 3.831 | 4 | 4 | 4 | 36.646 | 38 | 15.788 | 6.287 | 41.060 | 261.145 |
| priority | 100000 | 110183 | 2.068 | 4 | 5 | 6 | 22.542 | 54 | 6.310 | 2.369 | 11.650 | 33.988 |
| local | 100000 | 99923 | 2.672 | 5 | 6 | 6 | 27.373 | 54 | 6.618 | 2.608 | 11.192 | 12.849 |
| churn | 100000 | 100099 | 2.427 | 4 | 5 | 6 | 25.417 | 54 | 6.187 | 2.426 | 10.770 | 9.564 |

There is no fixed depth/storage bound inferred here. Complete checkpoint display-length, LK1-length, allocation calls, Path-object/step bytes, Tree-node bytes and live/peak bytes are in [growth.csv](../benchmarks/results/v3.1-workload-evidence-growth.csv). Length inspection does not serialize every resident LK1 string; exact sizes are obtained with the public length API. Core tests separately cover formatting/parsing.

### Long-lived churn: 50k initial / 500k operations

| Seed | Inserts | Mean us | P50 us | P95 us | P99 us | P99.9 us | Max ms | Relabel events | Largest region | Full |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 17 | 349902 | 1.001 | 0.977 | 1.465 | 1.953 | 5.127 | 0.190 | 777 | 32 | 0 |
| 3512641005 | 349756 | 0.961 | 0.977 | 1.221 | 1.709 | 4.395 | 0.188 | 817 | 16 | 0 |

| Seed | Operation | Population | Mean depth | P95 | P99 | Max | Mean LK1 | Max LK1 | Path MiB | Live library MiB | Cumulative requested MiB |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 17 | 0 | 50000 | 1.356 | 2 | 2 | 2 | 16.848 | 22 | 2.425 | 4.713 | 4.985 |
| 17 | 100000 | 50099 | 1.902 | 3 | 4 | 6 | 21.214 | 54 | 2.755 | 5.049 | 13.235 |
| 17 | 250000 | 50017 | 2.203 | 4 | 5 | 6 | 23.621 | 54 | 2.947 | 5.237 | 26.069 |
| 17 | 500000 | 50188 | 2.387 | 5 | 6 | 6 | 25.100 | 54 | 3.104 | 5.401 | 48.853 |
| 3512641005 | 0 | 50000 | 1.356 | 2 | 2 | 2 | 16.848 | 22 | 2.425 | 4.713 | 4.985 |
| 3512641005 | 100000 | 49844 | 1.903 | 3 | 4 | 6 | 21.221 | 54 | 2.741 | 5.022 | 13.197 |
| 3512641005 | 250000 | 49856 | 2.197 | 4 | 5 | 6 | 23.577 | 54 | 2.933 | 5.216 | 26.050 |
| 3512641005 | 500000 | 49528 | 2.373 | 5 | 6 | 6 | 24.988 | 54 | 3.052 | 5.319 | 48.856 |

**Observed:** Both seeds stayed near 50k residents; mean depth rose from 1.356 to approximately 2.38, maximum reached 6, LK1 mean rose from 16.848 to about 25 bytes. Resident Path bytes rose about 26–28%; total live library bytes rose about 13–15%. Each had about 48.85 MiB cumulative requested traffic including setup, versus about 5.3–5.4 MiB final live library bytes. Both returned live bytes to zero on destruction. There were 777 / 817 relabel events, no full events, largest regions 32 / 16.

**Interpretation:** A bounded-population history has measurable coordinate/storage drift; it cannot be described as unchanged storage. Its rate slowed over the measured checkpoints and did not explode within 500k operations. Allocation traffic and live residency are different quantities; these data do not indicate a leak.

**Unknown:** Whether storage reaches a steady distribution after millions of mutations, different update locality/extreme rates, and actual allocator/process/database overhead. Two long seeds and one large seed/profile are limited coverage; checkpoint maxima can miss transient depths between checkpoints.

## Comparable adversarial controls

These current-build reruns use the same compiler, clock, initial population, operation count and inspection schedule. They remain insertion-only, with different population growth and key distributions. They do not reproduce the original million-item runs and must not be presented as a speedup against historical timing.

| Control | P50 us | P95 us | P99 us | P99.9 us | Max ms | Relabel rate | Full | Largest region |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ascending | 0.732 | 0.977 | 1.221 | 19.531 | 0.064 | 0.00000% | 0 | 0 |
| descending | 0.244 | 0.488 | 0.732 | 20.752 | 0.182 | 0.00000% | 0 | 0 |
| random | 0.977 | 2.197 | 3.174 | 25.879 | 0.205 | 0.00800% | 0 | 16 |
| equal | 0.488 | 0.977 | 0.977 | 19.287 | 0.079 | 0.00000% | 0 | 0 |
| duplicates | 0.977 | 2.441 | 7.080 | 32.471 | 4.368 | 6.91500% | 0 | 8192 |
| alternating | 1.221 | 25.146 | 53.467 | 3324.951 | 29.159 | 35.66400% | 1 | 65536 |
| hotspot | 0.488 | 0.488 | 0.732 | 18.311 | 0.170 | 0.00000% | 0 | 0 |

Alternating had 35,664 relabels, 4,280,851 existing-coordinate replacements and a 65,536-node maximum region. Its relabel frequency and P99.9 were much higher than these application profiles. Yet its observed maximum (29.159 ms) was below the medium timeline maxima (~86–92 ms): average/higher-percentile cost and rare full-range pause are distinct concerns. Duplicates had 6,915 relabels and an 8,192-node maximum region. Priority changes caused different equal-bucket pressure and occasional full relabels. Pure equal and open-end controls did not relabel during the measured prefix/history here. Fixed hotspot likewise did not relabel at this scale; this does not imply all hotspot patterns are harmless.

## Validation and scope

- Local clean GCC C17 warning-as-error Release build: CTest 13/13 passed.
- Local MSVC 19.51 x64 Debug and Release: CTest 13/13 each passed.
- Local MSVC x64 AddressSanitizer RelWithDebInfo: CTest 13/13 passed after adding the compiler runtime directory to the process PATH. First launch lacked the ASan runtime DLL (`0xc0000135`); it was an environment issue, not a test result. No project fix was needed.
- Five existing CI jobs must validate the delivered commit: Windows MSVC; Ubuntu GCC; Ubuntu Clang; Ubuntu Clang ASan/UBSan; macOS AppleClang. The delivery report records the exact run SHA/URL and outcomes.
- Existing benchmark controls, historical captures, `src/*`, public header/version, production source manifest, install/package behavior, amalgamation and release asset workflow are unchanged. CMake changes only add evidence targets and smoke tests; Python is optional for local registration, available in CI.
- No branch merge, tag/release, Issue edit, repository metadata change, or future release assignment belongs to this evidence branch.

## Decision: observed / interpretation / unknown

### Observed

The modeled timeline exhibits rare, repeated full-range relabel events with approximately 85–225 ms production pauses at medium/large scales. Priority has smaller but nontrivial rare tails. Local reorder and steady churn retain small relabel regions in this campaign. Long churn shows moderate Path/LK1 residency growth without a cleanup leak. No model divergence was observed.

### Interpretation

Choose **B: investigate a compatible 3.2 research direction**. Focus a separate study on repeated timeline-tail backfill/interior-gap exhaustion: identify triggering placement and rejected candidate/expansion work, test whether coordinate-space reuse or bounded incremental relabel preparation can reduce the full-range pause while preserving current API/LK1 and strong failure behavior. These are research questions, not accepted changes or a release commitment. Priority bucket changes deserve a second study. Keep the released implementation unchanged while reproducing the findings and collecting external application traces.

No 3.1.x correctness fix is indicated by this campaign. No breaking V4 is justified by these data. The current contract permits relabel costs; whether an observed pause is acceptable depends on an application latency budget that these synthetic scenarios do not supply.

### Unknown

Real workload frequencies, user-visible latency budgets, larger/more complex item comparators, concurrent/background load, other allocator/machine behavior, exact-rank editor traces, histories much longer than ten times population, and tails below the bounded sample cutoff remain unknown. Five-seed deterministic agreement supports reproducibility of these profiles, not representativeness of users. Observed maxima provide no worst-case/amortized bound. Further data should guide any optimization; no algorithm tuning was performed in this task.
