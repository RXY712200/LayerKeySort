# Full v4.2.0 Preview.2 — lifecycle, downstream integration and scale

[Draft PR #31](https://github.com/RXY712200/LayerKeySort/pull/31) targets `dev/full-v4.2.0-preview.1.2.1` at `07b739de1841256ed89236b4096d130f88206956`. The clean starting branch was task commit `5e74ae71139e518f3c8d5f45261d91236e786255`. Select fixed-width LKS4SNP1 conversion at `fe00536e7bd7f4e5128432ea4a29e54158074903`. It improves decode workload costs without adding allocation, changing data format, or reopening the live core. The development header is 4.2.0-preview.2 and published Stable stays v4.1.0.

## Step 3. Equivalent lifecycle boundary

The [protocol](../benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.md) was committed before new candidate timing. The [architecture decision](design/V4_2_0_PREVIEW2_ARCHITECTURE.md) traces capture, callback copies, ownership/currentness, validation/checking, exact bytes and resolve-first restoration.

Both hosted screen and final run covered 13 prespecified cells per host, four alternatives, warmup plus seven fresh formal runs and separate diagnostics (468 rows/host/campaign). Workloads cover 1k/10k/100k occurrences, 0/1/2/8/16 retained histories, empty/short/32/256/variable-wide association widths and 8/4096-byte namespaces. The cases rotate live order between captures, destroy the source and payload before historical consumption, verify independent LS1 and exact wire contents, decode, resolve into a fresh order and continue editing. The comparison uses a C17 linked sequence and independently copied flat history for the common successful semantics, with lighter alloc-failure, validation and provenance obligations. It lacks the full comparator-managed index, arbitrary precedence surface and publication guarantees. Results describe these workload compositions.

Median ratios of cell medians against accepted Preview.1.2, lower is favorable:

| Outcome | Windows | Linux |
| --- | ---: | ---: |
| Screen decode, wire prototype | 0.6330 | 0.3456 |
| Final decode | 0.6712 | 0.4940 |
| Final encode | 0.9979 | 1.0030 |
| Final sum of timed lifecycle phases | 0.9667 | 0.9439 |
| Independent flat alternative sum of timed phases | 0.8160 | 0.8447 |

At 100k with eight retained snapshots and 32-byte associations, timed phase sums fell from **153.858 to 135.795 ms** on Windows and **88.121 to 83.173 ms** on Linux. Decode alone fell **15.287 → 9.416 ms** and **11.460 → 6.574 ms**. A 256-byte fixed association cell diluted the decode speed benefit in the initial screen. The flat composition remained faster overall in the representative anchor (123.120 and 75.557 ms), with lower requested peak (37,600,600 against 40,142,904 bytes). That is a real tradeoff for this endpoint-only operation mix. It cannot be extrapolated to arbitrary precedence or failure-equivalent managed orders.

No extra requested peak or retained bytes were seen for the wire candidate in any matched lifecycle cell. In the 100k × eight anchor both exact Full versions requested 36,279,424 bytes live with original order and history, and 40,142,904 bytes at peak. The deferred alternatives are snapshot-result reuse by source revision, which is unsound with mutable payloads, and interleaving restore preparation with callbacks, which would change the resolver prefix under allocation failure. No sharing, allocator or restore scratch prototype was adopted.

## Step 4. Exact native OrdWeave integration

The [OrdWeave archive](https://github.com/RXY712200/OrdWeave/tree/1e40955a783f91ec835170bea664bfa8e324b465/benchmarks/research/full-preview2-20261124) preserves the standalone 27-trace ZIP membership and SHA verification, replay and metrics. The original campaign is 486 rows against v4.1.0 and accepted Preview.1.2. The final campaign is 234 rows, with all nine seed-17 traces in both memory and persistent modes and all three dependency builds. The exact final native substitution, upstream pin and pinned application source are in the manifests. All three native builds passed the 12 upstream tests. A generated 4,096-occurrence persistent holdout passed independent logical sequence, checkpoint, history and reopen checks across all three dependency pins. The original Full allocation-fault sweep on the final candidate reached 37 insert and 34 remove failures, with unchanged order/revision and zero residual requested bytes after teardown.

Preview.1.2/v4.1.0 operation medians were 0.6555 in memory and 0.9803 persistent over 27 counterparts. The exact new candidate/Preview.1.2 medians were **0.9963** memory and **0.9892** persistent. Checkpoint, reopen and history ratios were **1.0057**, **0.9841** and **1.0001**. The worst checkpoint ratio was 1.1024 at 2,048 hotspot, with overlapping sample ranges (0.203–0.240 vs 0.196–0.219 seconds). The Full decode microbenchmark win does not establish a database-wide win.

In matched instrumented operation loops, stage plus publish consumed a median **94.55%** in memory. That stage contains Full-managed inserts under OrdWeave's global index rebuild policy. Direct collection edit spans were about 1.04%. Persistent WAL commit accounted for **97.57%** and its nested main flush accounted for **95.12%**. These are Windows generated-trace estimates, not additive unrelated timers or field telemetry. Checkpoint/history/reopen are separately timed. The instrumentation does not separately resolve comparator cost inside the stage or attribute whole-process temporary allocations. There is no adapter or database core change required for compatibility. TD-001 rebuild and TD-002 replay remain application-owned issues. No downstream production files or official dependency changed.

## Step 5. Resource envelope and live guard

At 1m occurrences every admitted clustered and scattered survivor trace retained the same 10,000 Handle addresses across two shrink/regrowth cycles. Accepted and candidate builds on both hosts went from 42,753,776 requested bytes to **431,576** after shrink, with 312 blocks and no untagged blocks in the final sparse state. All exited with zero residual library allocations. The guarded campaign also covered 10k and 100k and had zero memory skips. It did not attempt 3m/10m. Cumulative second-cycle requested traffic reached 85,075,976 bytes at the 1m/1% anchor, a different quantity from retained memory.

The separate 40-observation native Windows holdout includes 100k churn with four captures, distant and hotspot drag, and retention-heavy 10k. Timed median churn mutation p99.9 was 1.20→1.25 microseconds, and decode for the first churn snapshot was 0.821→0.426 ms. The separate diagnostic churn sample had a slower p99.9 (1.4→2.7 microseconds); the single diagnostic timings do not establish a recurring production tail. Diagnostic peak remained 11,492,708 bytes. QPC and supported process working-set samples are in the archived trace, separate from requested bytes. Large lifecycle traffic beyond this envelope remains open.

Fresh 45-trace native live safeguards produced 1,215 rows per host. The candidate/Preview.1.2 median was 1.0189 Windows and 0.9985 Linux, with worst individual ratios 1.0585 and 1.0282. Candidate/Stable 4.1.0 medians were 0.6461 and 0.6107, with **45/45** faster cells on each host. The fresh live traces and production diff preserve the chosen live architecture. The historical OM and 4.0 comparisons remain distinct and were not repeated for a wire-only change. [Reproduction, receipts and all cells](../benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.md)

## Compatibility, verification and documentation synchronization

The 64 public functions, 14 types, status, ownership, immutable views, callback/reentry, cursor/revision, LS1, LKS4SNP1 and LK1 remain frozen. Local strict GCC Debug and Release each passed 39/39, API, source/install/amalgamation distribution, local assets and external consumer validation. Wire golden bytes and malformed cases are unchanged. The same-revision independent snapshot test verifies two copied namespaces and duplicate associations after caller storage and source destruction. Reached historical capture/trim/marker/deserialize/restore failure paths remain covered. [Exact-head PR checks](https://github.com/RXY712200/LayerKeySort/pull/31/checks)

Accepted Stable source and guide baseline are `70d7f0ef5857b54381b863d292a213909d87459f` / `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`. Prior operational source `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`, diagnostic source `c977b0fd14a501d769b9783bfdcc61266bd1b5ae` and guides `07b739de1841256ed89236b4096d130f88206956` are preserved. This provisional documentation maps implementation `fe00536e7bd7f4e5128432ea4a29e54158074903` and added isolation test `92fb7fad977f792a7d9e3e0e430a5d5f32e8914a`.

| Full guide | Affected explanation and evidence |
| --- | --- |
| [Walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | Wire chapter describes fixed four/eight-byte fields and validated decoding; independent wire byte oracle passed. |
| [API reference](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | `lks_snapshot_deserialize` validation/copy route revised. Status, borrowed views and ownership unchanged. |
| [Internals](IMPLEMENTATION_INTERNALS.zh-CN.md) | `snap_be_read/write` unsigned shifts, byte access and intact two-pass publication. No new allocation. |
| [Validation](IMPLEMENTATION_VALIDATION.zh-CN.md) | Wire goldens/failures retained, same-revision namespace and duplicate payload isolation regression added. Exact native and CI evidence linked. |

[Full consumer handoff](FULL_CONSUMER_HANDOFF.md) records the version, before/after source paths and separate OrdWeave core/adapter outcomes. Changelog marks the development candidate. Public build and migration instructions remain applicable.

## Technical Debt Impact

| Item | Impact |
| --- | --- |
| FULL-RI-001 | Accepted B64/AVL/tag design and diagnostic checker retained. |
| FULL-RI-002 | 90 fresh live cells preserve the earlier stable comparison, within the non-target guard. |
| FULL-RI-003 | Quantified lifecycle phase displacement and improved exact wire decode. Capture sharing and resolver scratch remain deliberate retained costs. |
| FULL-RI-004 | Verified actual generated native workloads and separated global index, collection and WAL/flush spans. Exact component suballocation and external workloads remain open. |
| FULL-RI-005 | Verified sparse 1m-to-1% survival through regrowth, retained snapshots and known scale tradeoffs. Larger envelopes remain open. |
| FULL-RI-006 | Added source destruction and equal-revision snapshot independence coverage. Broader engineering convergence remains open. |
| FULL-RI-007 | Added equivalent-success comparison and measured limits. No universal selection recommendation. |

Independent acceptance is pending. The scope ends with Steps 3–5.
