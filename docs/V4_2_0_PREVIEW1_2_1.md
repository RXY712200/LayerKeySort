# Full Preview.1.2.1 ceiling study

**Decision: keep Preview.1.2 production.** The two-host evidence supports the stronger diagnostic checker and rejects all three experimental performance variants. The [draft PR #30](https://github.com/RXY712200/LayerKeySort/pull/30) is stacked on Preview.1.2 `83367fd9b3b7297db1642c2eea6159bf395ef286`. Development started at the task-only commit `de84eb1c82c75e790ae65445a5bf5a00ed8bf103`. The accepted implementation stays `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`; diagnostic implementation and regression are `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`. The version remains `4.2.0-preview.1.2`.

## Steps 1–2 — measured decision

The [architecture report](design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md) maps resident movement, allocation, structural work, label maintenance and managed compare costs. The [prospective protocol](../benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.md) was committed at `9df156dd425f940fa25bb131b7d9993d4557a420` before new candidate timings. It screened direct source-repair transfer and the private B32/B128 capacity variants. Index and Handle rewrites had insufficient support from prior count-ablation and sparse-retention evidence. The operational comparison path was left alone.

Each host ran the same 15 frozen seed-17 traces, five mixes at 1k, 10k and 100k. It ran one warmup, seven serial formal and one diagnostic process per cell. Rows matched 746 command receipts per host, including the diagnostics, held-out profiles, guard decisions and build commands. The local Windows screen and hosted Windows/Linux screens yielded 1,620 replay rows in total. Hosted clocks were QPC and CLOCK_MONOTONIC. Absolute times and hardware are host-specific.

Median elapsed ratios relative to B64, with equal weight per cell and lower values favorable:

| Host / variant | Overall | Compare-heavy | Update-heavy | Long | Hotspot | Short |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Windows / transfer | 1.0053 | 0.9808 | 1.0111 | 1.0091 | 1.0053 | 1.0001 |
| Linux / transfer | 0.9825 | 0.9733 | 0.9845 | 0.9792 | 0.9825 | 0.9985 |
| Windows / B32 | 0.9953 | 0.9953 | 0.9123 | 0.9138 | 1.0610 | 1.0869 |
| Linux / B32 | 0.9239 | 0.9411 | 0.8472 | 0.8806 | 0.8966 | 1.0078 |
| Windows / B128 | 1.1389 | 1.1323 | 1.2836 | 1.2329 | 1.1214 | 1.0134 |
| Linux / B128 | 1.1277 | 1.1184 | 1.1720 | 1.2180 | 1.0681 | 0.9871 |

- Transfer made no allocation or location-write savings. It removed a scratch copy in one structural path, and the effect was too small to meet either performance route.
- B32 cleared the Linux timing screen but missed both Windows routes. It also exceeded the separate 5% resource limit on both hosts: maximum requested live increase **8.38%**, peak **6.43%**. The 100k short-move median was **13.93%** slower on hosted Windows and **8.73%** slower on hosted Linux. The local Windows sample corroborated the short-move penalty and missed both timing routes overall.
- B128 saved some representation bytes, but overall replay was **13.89%** slower on Windows and **12.77%** slower on Linux, and the compare-heavy regression exceeded 5% on both. Median update-heavy time increased **28.36%** on Windows and **17.20%** on Linux.

The [raw evidence and scripts](../benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.md) retain all adverse cells, medians, ranges, IQR, requested memory, working-set fields where supported, and allocations. No default-setting decision depends on a noisy threshold crossing. The nine-adapter campaign was conditional on a viable variant. Prior exact classical OM and Stable comparisons remain historical observations at their original pins.

## Mutation, resources and label health

- The local diagnostic 10k initial / 5k operation profiles expose different costs. Long moves registered 276,797 position assignments and 6,038 index touches, versus 44,888 and 4,395 at the endpoint. Managed random had 190,112 comparator calls. Counts include construction. They do not isolate hardware cache latency or allocator wall-clock share.
- The 100k focused B64 insert phase on both hosts produced 3,318 tag stores, 171 repairs, zero failed tag assignments and at most four stores on a single attachment. Of the sampled cross-block comparisons, 100,224 used both tags. Requested live storage fell from 4,289,120 bytes to 14,720 after deletion to 256 residents. Injecting two zero tags caused 96 sampled fallback comparisons; both tags recovered during the later append run. This is a reachable recovery case within the existing bounded policy.
- The independent 100k bulk-built holdout is materially different. B64 started at 1,563 blocks and ended at 1,605 after 4,096 mixed moves, including 1,306 same-block, 2,790 cross-block and 86 full cross-block destinations. It ended with zero untagged blocks. B32 reduced position writes (168,761 → 108,999) while increasing index touches (1,637 → 3,950) and produced 53 failed label assignments, finishing with 44 untagged blocks. B128 reduced index touches to 733 but increased position writes to 281,914. This probe uses the restore bulk constructor in memory, with exact persisted decoding and restore covered separately by CTest.
- Native operation profiles use an independent seed, four formal processes and separate diagnostics. Worst within-family candidate/control median p99.9 ratios on hosted Windows/Linux were 1.17×/1.08× for transfer, 1.06×/1.03× for B32, and 1.46×/1.08× for B128. Individual maxima vary with timer resolution, runner scheduling and allocation. Process working set and allocator requests are different quantities. Long-running arbitrary fallback prevalence and million-record/sparse lifecycle remain open; the bounded campaign stopped at 100k-scale populations.

## Verification and synchronization

The checker now rejects equal or descending nonzero labels across zero-tag blocks. The regression checks both invalid orders and restores original tags before the existing saturation and order checks. Ordinary production preprocessing of `order.c` was byte-identical under GCC, and the public header, version, CMake configuration and remaining production files are identical to accepted Preview.1.2. Stable Handles, managed equality and reentry, callback/OOM/revision/no-op semantics, snapshot compatibility and the 64-function/14-type public surface carry forward.

Windows strict GCC Debug and Release passed 39/39 each. Frozen API manifest, C/C++ source/install/amalgamation and local release-asset validation passed. The existing parameterized mutation suite passed for transfer, B32 and B128, with independent identity/order checks and zero live allocations. Isolated OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` passed 12/12 with the diagnostic commit substituted. Its official v4.1.0 pin remains `70d7f0ef5857b54381b863d292a213909d87459f`. See [draft PR checks](https://github.com/RXY712200/LayerKeySort/pull/30/checks) for final-head platform results. [Verification receipts](../benchmarks/research/full-v4.2.0-preview.1.2.1/verification/)

The accepted Stable implementation and guide source are `70d7f0ef5857b54381b863d292a213909d87459f` and `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`. The prior provisional guide revision is `83367fd9b3b7297db1642c2eea6159bf395ef286` at production `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`. Current diagnostic source is `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`.

| Guide | Affected explanation and actual verification |
| --- | --- |
| [Walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | Comparison flow distinguishes private diagnostics from unchanged public operation; tested tagged/fallback comparison. |
| [API reference](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | `lks_order_compare` status, ownership and execution path unchanged, with diagnostic distinction; manifest and foreign-Handle tests. |
| [Internals](IMPLEMENTATION_INTERNALS.zh-CN.md) | `v4_check_node` checks the nonzero subsequence with one checker word; preserves block fields, B64, lookup and allocation. |
| [Validation](IMPLEMENTATION_VALIDATION.zh-CN.md) | `preview12_block_tags` rejects equal and descending nonzero pairs across zero, restores tags and continues forced-saturation coverage. |

The [consumer handoff](FULL_CONSUMER_HANDOFF.md) records the diagnostic-only ledger and separate core/adapter evidence. Public API, usage, release notes and compatibility policy require no production update.

## Technical Debt Impact

| Item | Evidence and remaining boundary |
| --- | --- |
| FULL-RI-001 | Measured occupancy, location and index costs separately; no new production mechanism justified. |
| FULL-RI-002 | Rejected all bounded variants on same-language or resource evidence; existing OM comparison claims stay scoped to Preview.1.2. |
| FULL-RI-003 | Checked bulk restore and existing persistence contracts; broader lifecycle costs remain open. |
| FULL-RI-004 | Refreshed isolated OrdWeave coverage; adapter cost attribution remains open. |
| FULL-RI-005 | Measured concentration, shrink/regrow and a B32 fallback-heavy shape; broader resource envelope remains open. |
| FULL-RI-006 | Diagnostic label invariant strengthened, with failing equal and descending fixtures and recovery evidence. No regression or architecture issue is closed by timing alone. |
| FULL-RI-007 | Preserved a reusable capacity and repair comparison; general selection guidance remains open. |

Independent acceptance remains pending. This work stops after the bounded Steps 1–2 extension.
