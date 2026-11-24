# Full Preview.1.2 — bounded block-precedence extension

[Stacked draft PR #29](https://github.com/RXY712200/LayerKeySort/pull/29) covers Steps 1–2 only, against the Preview.1 branch at `3a2b3eabd0153531e1064457519b40deb4af9842`. Preview.1 remains the independently accepted fallback. Published Stable is v4.1.0. The integrated research candidate source is `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`; its exact accepted implementation control is `22544cf283694dd27cef203caba9cf1838845947`.

## Decision and mechanism

See the [bounded tag architecture](design/V4_2_0_PREVIEW1_2_ARCHITECTURE.md) and [prespecified acceptance criteria](../benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.md). The bounded C block labels reuse Full's indexed blocks as the level of indirection. The existing AVL maintains hierarchy and managed search. Bounded relabel and safe zero-tag fallback address focused-gap exhaustion while preserving allocation, failure publication and Handle semantics. The order-maintenance semantics of the classical controls do not subsume Full's managed search, snapshots, import, history and failure contracts.

The preliminary Windows screen used ten preselected cells, seven formal repeats each with warmup and diagnostic samples separate. Relative to Preview.1 the tags had median ratios 0.8871 overall and 0.3227 comparison-heavy, with no slower cell at its median. The metadata-only ablation was 1.0009 overall and 0.9892 comparison-heavy, so it was dropped. The complete test build also demonstrated that the tests still inspect the removed count. The failed build and library-only measurements are archived. The historical midpoint-only candidate remains rejected.

In 10,000 focused inserts, the candidate recorded 324 tag stores, eight repairs, no fallback and at most two stores per attachment. Requested medium-state live and peak bytes rose from 434,536 to 437,096. Production QPC p95/p99/p99.9/max were 100/200/5,100/20,400 ns versus 100/200/5,300/21,200 ns. The timer quantizes fast events; a zero median is below useful single-operation resolution. This sampled hotspot does not prove that the 64-block cap is rarely reached in every workload. A regression test forces exhaustion separately.

The C17 external-call comparison probe, with independent ordinal oracle and the same order construction, gave median public cross-block costs of 4.365 → 1.222 ns at 1,024 occurrences and 37.262 → 3.891 ns at 100,000. Same-block costs were 1.219 → 1.232 and 3.098 → 3.139 ns. Internal ancestry and fixed-ordinal rows are diagnostic comparisons, with inlining possible, and do not represent equivalent dynamic APIs. The classical controls remain their native pinned Rust builds. No faithful C port was required for this bounded experiment; language, compiler and wrapper costs remain confounded across languages.

## Contemporaneous matrix

The [research run](https://github.com/RXY712200/lks0011/actions/runs/45387318084) uses public Full source and exact locked OM controls. Forty-five frozen 20,000-operation traces cross three populations (1k, 10k, 100k), three seeds and five mixes. Each host runs 45 × 8 × 9 observations, including one warmup, seven fresh formal production processes and one separate diagnostic process per cell. Traces use an independent sequence oracle. QPC, CLOCK_MONOTONIC and Rust Instant bound replay; setup and diagnostics are separate. The collector rotates execution and enforces 512 MiB admission and a 120-second child timeout. Runtime is replay elapsed time, not isolated update latency.

The extension meets the prespecified adoption gates on both hosts. Retain the integrated candidate for independent review. It reduces the median matched-cell ratio versus Preview.1 by 10.2% on Windows and 11.4% on Linux overall, and by 62.9% and 65.7% on comparison-heavy traces. Both Stable 4.1 reference targets are retained. Both benchmark jobs succeeded. The archive validator reconciled all 6,480 rows with 6,490 successful invocation receipts.

Ratios below 1 favor the candidate; wins count strictly faster medians.

| Control | Windows overall / compare-heavy / wins | Linux overall / compare-heavy / wins |
| --- | ---: | ---: |
| Preview.1 | 0.8980 / 0.3707 / 44/45 | 0.8859 / 0.3434 / 45/45 |
| v4.1.0 | 0.6417 / 0.2185 / 45/45 | 0.5720 / 0.2061 / 45/45 |
| v4.0.0 | 0.5963 / 0.2122 / 45/45 | 0.5391 / 0.1965 / 45/45 |
| Tag-range OM | 0.9793 / 0.9108 / 23/45 | 0.9173 / 0.6523 / 28/45 |
| List-range OM | 0.9837 / 0.7502 / 23/45 | 0.8804 / 0.5182 / 33/45 |
| List | 0.0349 / 0.0032 / 45/45 | 0.0460 / 0.0045 / 44/45 |
| Treap | 0.1461 / 0.0833 / 45/45 | 0.1365 / 0.0800 / 45/45 |

The only slower Preview.1 comparison was Windows, 1,000 initial occurrences, seed 2654435769, hotspot: 2.1064 ms versus 1.8227 ms (1.156×). Candidate range 1.7106–2.5839 ms and IQR 0.7388 ms overlapped the control range 1.7794–2.5822 ms and IQR 0.0579 ms. Neither host showed a recurring slowdown across that profile. The largest native OM deficits were Windows update-heavy at 1k/seed 305419896 (1.591× tag-range, 1.584× list-range), and Linux long-move at the same size and seed (1.630×/1.568×). Linux List also won one cell. The candidate does not give a universal performance advantage.

Requested live bytes rose by at most 0.545%, and peak by at most 0.600% versus Preview.1 on either host. The local held-out seed 1779033703 exercised endpoint, hotspot, local, random, churn, long moves and managed churn at 10k initial occurrences and 5k mutations. All 112 production and diagnostic rows passed their oracle and cleanup check. Worst family median production p99.9 ratios were about 1.21× (random: 6.00 versus 4.95 µs) and 1.16× (hotspot: 5.85 versus 5.05 µs); no family approached the repeated 2× criterion. These supplemental Windows measurements have timer quantization and local load limitations. Hosted replay does not measure per-operation Linux tails.

See [reproduction and evidence index](../benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.md) for exact inputs, controls, receipts, raw captures, resource metrics, full adverse cells and scope. OM comparisons use pinned j-hui tag-range and list-range; published v4.1.0 and v4.0.0, accepted Preview.1, List and Treap were freshly measured. Requested bytes and working-set samples are different measurements; missing process RSS has no semantic zero.

## Contract and verification

The public surface remains 64 functions and 14 public types. Stable live Handle identity, owner and callback boundaries, external synchronization, revision and no-op results, OOM failure publication, LS1 and LKS4SNP1, and V3 LK1 import remain as before. Restore creates fresh Handles and initializes private block tags. Numeric CMake version remains 4.2.0; the candidate header identifies the provisional extension.

Strict GCC C17 Debug/Release passed 39/39 each on Windows. The targeted test checks repeated interior insertions, shared-payload Handle identity, the complete sequence and forced exhaustion with cross-block comparison checked in both directions. Existing tests cover mixed mutation, allocation faults, bulk/restore, managed stable duplicates, wire and lifecycle. Frozen API verification, C/C++ source/install/amalgamation distribution and release-asset validation passed locally. Asset verification generated local test output only.

Isolated native Windows OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` with the candidate substituted passed 12/12 tests, including persistence, checkpoints, history and external consumer. Its official v4.1.0 dependency remains `70d7f0ef5857b54381b863d292a213909d87459f`. This test is an integration signal, separate from core correctness and a downstream upgrade decision. The PR checks tab is authoritative for actual final-head push/PR checks and any stacked-base-only omissions.

## Source-bound guide synchronization

The last accepted Stable implementation is `70d7f0ef5857b54381b863d292a213909d87459f`, with accepted documentation on main at `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`. Each row maps accepted Preview.1 implementation `22544cf283694dd27cef203caba9cf1838845947` and corrected documentation `3a2b3eabd0153531e1064457519b40deb4af9842` to candidate implementation `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`.

| Guide | Synchronized sections and functions | Evidence and scope |
| --- | --- | --- |
| [Walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | Representation and comparison flow, `lks_order_compare`, block attachment and restore | Dynamic compare/exhaustion regression and restore tests; existing ownership and operation contracts retained |
| [API reference](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | Comparison route and lifetime/errors, `lks_order_compare` | Frozen 64-function manifest, status/identity/foreign-handle tests; LS1, wire, callbacks and ownership retained |
| [Internals](IMPLEMENTATION_INTERNALS.zh-CN.md) | Block representation, bounded relabel metrics, fallback, `v4_tag_attach`, `lks_order_bulk_build` | Invariant/mixed-mutation/restore tests plus diagnostic screen; finite-label recovery frequency beyond these samples unknown |
| [Validation](IMPLEMENTATION_VALIDATION.zh-CN.md) | Regression and exact source pins, limits and evidence links | 39/39 Debug and Release, 12/12 isolated consumer, distribution/assets; live hosted checks linked from PR |

[Consumer handoff](FULL_CONSUMER_HANDOFF.md) carries the exact-SHA ledger and adapter actions. The frozen API, integration, migration, external ownership and wire contracts require no change.

## Technical Debt Impact

| Investigation | Outcome |
| --- | --- |
| FULL-RI-001 | Advances classical comparison investigation with current two-host controls and same-language public-call isolation. Abstract cross-language attribution remains open. |
| FULL-RI-002 | Tests bounded dynamic block maintenance and metadata ablation, including concentration and fallback; scope remains this Step 1–2 decision. |
| FULL-RI-003 | Existing snapshot behavior checked for compatibility. Snapshot cadence and cost optimization remain later research. |
| FULL-RI-004 | Isolated OrdWeave compatibility evidence refreshed. Component-level database costs and official dependency adoption remain open. |
| FULL-RI-005 | Requested storage and bounded repair were measured. Larger-state retention and lifecycle costs remain later research. |
| FULL-RI-006 | Structural invariants, exhaustion, restored identity and failure atomicity checked. Wider maintenance and engineering validation remain open. |
| FULL-RI-007 | Released controls and reproducibility preserved. Broader adoption/engineering evidence remains open. |

Independent acceptance is pending. No integration or publication is part of this extension.
