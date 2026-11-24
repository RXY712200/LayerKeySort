# Full Preview.3 engineering and adoption evidence

[Draft PR #32](https://github.com/RXY712200/LayerKeySort/pull/32) targets preserved Preview.2 at 01bc01e2d9ddbda448d9ef0807150f419203b580. The issued HEAD is 326b7859d2a0325520d9b88635e400ba32a33d86. Production remains fe00536e7bd7f4e5128432ea4a29e54158074903 and the development version remains 4.2.0-preview.2. See the [source-bound audit](design/V4_2_0_PREVIEW3_ARCHITECTURE.md) and [selection guide](FULL_SELECTION_GUIDE.md).

## Verification and scope

Unaligned wire and restored mixed-tag precedence regressions are at 754b3088c160564f6666ee520284168ee5da6ac7 and 3ac82d26301ea12876a3ebdba6319fe8233621f3. Strict local GCC Debug and Release passed 39/39. API, C/C++ distribution and local asset checks passed. Exact hosted validation is attached to the PR. Two independent Preview.2 Workbench AppleClang attempts ended cancelled with empty runner assignment and no steps (jobs 138487623671 and 138579797624). The log endpoint returned HTTP 404 for the retry. The fail-closed aggregate gate failed. The unchanged workflow uses macos-latest. The public incident query identified no matching incident; the cause remains unknown.

## Step 7 findings

[Live run 45439986382](https://github.com/RXY712200/lks0011/actions/runs/45439986382) executed 6,480 rows and 10 semantic gates. Full's matched median ratio on Windows/Linux was 0.6291/0.6400 versus Stable 4.1, 0.9945/0.9981 versus Preview.1.2, 1.0524/1.0202 versus native tag-range and 0.9085/0.9257 versus native list-range. The preserved collector label preview1 denotes Preview.1.2 diagnostic pin c977b0f in this archive. Two Windows cells exceeded 10% against Preview.1.2, the worst 1.424 on a short 1k trace. Linux had none. Keep the outliers and the native language/adapter limitations. The operational live sources are unchanged.

[Aligned lifecycle run 45441925131](https://github.com/RXY712200/lks0011/actions/runs/45441925131) executed 810 observations on each host. Final/flat endpoint wall-time ratios were 1.137 Windows and 1.136 Linux. Final/threaded-Treap relative lifecycle wall-time ratios were 0.580 and 0.526. The chosen composition restores via incremental relative insertion. An improved bulk restore alternative remains unmeasured. The earlier asymmetric scan is preserved as a negative methodology result. The final capture distinguishes sum of phases from wall time including oracle and fixture work. Tree snapshot allocator figures exclude sequence memory in the broad capture; separate held-out diagnostics account for retained sequence memory. Managed equivalence and arbitrary long-run label recovery remain outside this comparison.

Fresh Windows sparse and label probes reproduce 10k/100k/1m anchors and targeted fallback/recovery. The main transactional application evidence is delivered in the separately authorized OrdWeave research branch, alongside the preserved Preview.2 archive.

## Guide synchronization

Prior provisional guides 01bc01e and implementation fe00536 remain the current operational contract. Last accepted Stable source and documentation are 70d7f0e and 1a175f9. The matrix below maps test-only corrections to the operational source.

| Guide | Reviewed path and bound evidence |
| --- | --- |
| Walkthrough | Wire decode and restore tutorial, unaligned buffer regression and fresh Handle identity. |
| API reference | Snapshot serialize/deserialize, exact bytes and unchanged error and borrowed lifetime. |
| Internals | Wire fixed-width fields and restored block label/fallback invariant checked. |
| Validation | New exact offset-one and restored-tag test assertions and final CI evidence. |

## Technical Debt Impact

| ID | Disposition |
| --- | --- |
| FULL-RI-001 | Retain the live design; no theoretical novelty claim. |
| FULL-RI-002 | Contemporary live data, bounded gains, retained OM counterexamples. |
| FULL-RI-003 | Equivalent-success lifecycle and restore comparison; scratch and copy costs retained. |
| FULL-RI-004 | Native application compatibility and stage attribution, no official pin or core/adapter change. |
| FULL-RI-005 | Revalidated sparse resource envelope through 1m; larger and long-retention cases remain open. |
| FULL-RI-006 | Tightened wire and restore coverage, no confirmed Full production blocker found in checked paths. |
| FULL-RI-007 | Evidence-linked selection with contrary conditions; no universal rule. |

RC recommendation depends on independent review and completion of the real gates, including the earlier Workbench prerequisite. No RC implementation or publication is implied.
