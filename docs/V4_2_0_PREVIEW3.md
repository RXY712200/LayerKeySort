# Full Preview.3 engineering and adoption evidence

[Draft PR #32](https://github.com/RXY712200/LayerKeySort/pull/32) targets preserved Preview.2 at 01bc01e2d9ddbda448d9ef0807150f419203b580. The issued HEAD is 326b7859d2a0325520d9b88635e400ba32a33d86. Production remains fe00536e7bd7f4e5128432ea4a29e54158074903 and the development version remains 4.2.0-preview.2. See the [source-bound audit](design/V4_2_0_PREVIEW3_ARCHITECTURE.md) and [selection guide](FULL_SELECTION_GUIDE.md).

## Verification and scope

Unaligned wire and restored mixed-tag precedence regressions are at 754b3088c160564f6666ee520284168ee5da6ac7 and 3ac82d26301ea12876a3ebdba6319fe8233621f3. Strict local GCC Debug and Release passed 39/39. API, C/C++ distribution and local asset checks passed. Exact hosted validation is attached to the PR. Two independent Preview.2 Workbench AppleClang attempts ended cancelled with empty runner assignment and no steps (jobs 138487623671 and 138579797624). The log endpoint returned HTTP 404 for the retry. The fail-closed aggregate gate failed. The unchanged workflow uses macos-latest. The public incident query identified no matching incident; the initial cause remains unknown. One bounded retry [attempt 3](https://github.com/RXY712200/LayerKeySort/actions/runs/45406241385/attempts/3) succeeded at the original Preview.2 SHA. [AppleClang job 138872070896](https://github.com/RXY712200/LayerKeySort/actions/runs/45406241385/job/138872070896) passed 10/10 in Debug and 10/10 in Release. The aggregate gate passed.

## Step 7 findings

[Live run 45439986382](https://github.com/RXY712200/lks0011/actions/runs/45439986382) executed 6,480 rows and 10 semantic gates. Full's matched median ratio on Windows/Linux was 0.6291/0.6400 versus Stable 4.1, 0.9945/0.9981 versus Preview.1.2, 1.0524/1.0202 versus native tag-range and 0.9085/0.9257 versus native list-range. The preserved collector label preview1 denotes Preview.1.2 diagnostic pin c977b0f in this archive. Two Windows cells exceeded 10% against Preview.1.2, the worst 1.424 on a short 1k trace. Linux had none. Keep the outliers and the native language/adapter limitations. The operational live sources are unchanged. A new paired native Windows repeat of both adverse cells gave 0.994 (long) and 0.996 (short), so the original slowdown did not recur. Both captures are retained.

[Aligned lifecycle run 45441925131](https://github.com/RXY712200/lks0011/actions/runs/45441925131) executed 810 observations on each host. Final/flat endpoint wall-time ratios were 1.137 Windows and 1.136 Linux. Final/threaded-Treap relative lifecycle wall-time ratios were 0.580 and 0.526. The chosen composition restores via incremental relative insertion. An improved bulk restore alternative remains unmeasured. The earlier asymmetric scan is preserved as a negative methodology result. The final capture distinguishes sum of phases from wall time including oracle and fixture work. Tree snapshot allocator figures exclude sequence memory in the broad capture; separate held-out diagnostics account for retained sequence memory. Managed equivalence and arbitrary long-run label recovery remain outside this comparison.

The first sparse build was launched before its diagnostic archive existed and failed to link. A successful guarded rerun then reproduced 10k/100k/1m anchors and targeted fallback/recovery. The transactional application evidence is tracked in the [authorized OrdWeave archive](https://github.com/RXY712200/OrdWeave/tree/research/full-preview3-20261125/benchmarks/research/full-preview3-20261125), alongside the preserved Preview.2 archive.

## Guide synchronization

Prior provisional guides 01bc01e and implementation fe00536 remain the current operational contract. Last accepted Stable source and documentation are 70d7f0e and 1a175f9. The matrix below maps test-only corrections to the operational source.

| Guide / reviewed sections | Current source and contract | Assertion and boundary |
| --- | --- | --- |
| Walkthrough §§2, 5–6, 12–14 | order.c bounded tags/AVL, snapshot.c independent bytes, snapshot_wire.c fixed unsigned shifts | Mixed tags, same-revision isolation and offset-one golden. No general label recovery deadline. |
| API reference §§B–E | Public header and snapshot.c/snapshot_restore.c, same 64 entries/14 types and ownership | Borrowed views outlive only the live source, remaining valid until Snapshot destruction. Restore uses fresh Handles. Callback effects remain application-owned. |
| Internals §§2–3, 9–11 | order.c nonzero subsequence across zero blocks; order_bulk.c fresh spacing; snapshot_wire.c two-pass decode | Restored tag spacing and bidirectional fallback, literal wire bytes. No live topology serialized. |
| Validation §§1–6, 9 | Current checkout recipe, frozen fixtures, v4_snapshot.c and v4_mutation.c | New assertions at 754b3088c160564f6666ee520284168ee5da6ac7 and 3ac82d26301ea12876a3ebdba6319fe8233621f3. Archived phase sum and wall analyses are separate. |

## Technical Debt Impact

The active records are [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3) and the [seven-step plan #27](https://github.com/RXY712200/LayerKeySort/issues/27). The source-bound decisions and rejected candidates remain in the [inventory](../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md).

| ID | Evidence and disposition | Remaining limit and trigger |
| --- | --- | --- |
| FULL-RI-001 | Retained B64, exact location updates and bounded tags with fallback. [Live-core history](design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.md). Deliberate tradeoff. | Revisit if external long-lived traces show sustained zero-tag traversal or structural write costs materially dominate. |
| FULL-RI-002 | Scoped comparison completed, 6,480 live rows, contemporary native OM controls, adverse cells and paired repeats preserved. | Native tag-range can win. Revisit if a failing target workload recurs across matched hosts and a same-semantics candidate clears the existing gates. |
| FULL-RI-003 | Partial. Fixed-width wire at fe00536e, 1,620 lifecycle observations, separate phase and wall evidence. | Retained copies and N-pointer restore scratch cost remain. Test optimized bulk tree restore or alternative capture when measured lifecycle budgets warrant it. |
| FULL-RI-004 | Scoped adapter compatibility supported by 702 native observations and retained recovery/fault evidence. | OrdWeave staging and WAL dominate, official pin unchanged. Revisit with owner-approved integration, actual traces, or a measured adapter/provenance mismatch. |
| FULL-RI-005 | Partial. Guarded 10k/100k/1m and repeated sparse survival with unchanged Handles. | Longer retained histories, multi-million states and arbitrary zero-tag recovery remain unmeasured. Extend only on evidenced demand and safe admission. |
| FULL-RI-006 | Scoped test gaps addressed by the offset-one and restored-tag regressions. No checked Full production defect established. | A reproducible portability, ownership, wire or publication defect reopens the corresponding invariant and platform gate. |
| FULL-RI-007 | Evidence-backed selection for tested relative-history and endpoint classes, with contrary evidence and real downstream attribution. | No universal choice rule. Revisit with another production consumer, managed equivalence or a credible bulk alternative under matched success semantics. |

Recommend RC-entry consideration after independent review and exact-head gates. The earlier Preview.2 Workbench prerequisite now passes at its own SHA. No RC implementation or publication is implied.
