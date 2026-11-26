# Full RC.1 source and documentation reconstruction

[简体中文](V4_2_0_RC1_DOCUMENTATION_AUDIT.zh-CN.md)

| Role | Exact revision |
| --- | --- |
| Published Stable source | 70d7f0ef5857b54381b863d292a213909d87459f |
| Independently accepted Stable guides | 1a175f91e4c338c2afe6a73a9f8f9c5b149768e5 |
| Preview.3 provisional guides | 168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0 |
| Operational source | fe00536e7bd7f4e5128432ea4a29e54158074903 |
| RC metadata and validation checkout | d1132a26ca7b2c65bfd512fa2eb29f1b16793c08 |

Inspect the complete [Stable-to-RC reference diff](https://github.com/RXY712200/LayerKeySort/compare/70d7f0ef5857b54381b863d292a213909d87459f...d1132a26ca7b2c65bfd512fa2eb29f1b16793c08), the [public declarations](V4_2_0_RC1_API_INDEX.md), and the [Preview.3 inventory and A–F lineage](../../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md). All current paths in the matrix refer to the validation checkout; unlisted experimental branches do not define current behavior.

## Source evolution and guide impacts

| Change / milestone | Replaced or added | Synchronized sections and checks |
| --- | --- | --- |
| 22544cf283694dd27cef203caba9cf1838845947 and 6d4099d | Replaced rank comparison with parent/height precedence; suffix location updates on ordinary shifts | Walkthrough §§2–4; Internals §§2–7; v4_mutation topology and failure oracle |
| 61e9290ec5ba47b9092bcb8ae7d61f13104ea839 and 21f6fc4 | Nonzero tag fast path, bounded redistribution, zero fallback and bulk tag spacing | Internals §§2–6,11; Walkthrough §§2–3; concentrated insertion, exhaustion, full order and symbol checks |
| c977b0fd14a501d769b9783bfdcc61266bd1b5ae | Diagnostic last nonzero tag across zero gaps; retained B64 | Internals §§3,14–15; Validation §§3–4; diagnostic equal and descending tag rejection. Rejected B32/B128/direct transfer remain evidence. |
| fe00536e7bd7f4e5128432ea4a29e54158074903 and 92fb7fad977f792a7d9e3e0e430a5d5f32e8914a | Fixed 4/8-byte wire access and same-revision independent snapshots | Walkthrough §§5–6; API §§B,D,E; Internals §§9–11; Validation §§3–4. Frozen bytes preserved. |
| 754b3088c160564f6666ee520284168ee5da6ac7 and 3ac82d26301ea12876a3ebdba6319fe8233621f3 | Offset-one writer/reader and restored mixed-tag fallback | Validation §§1,3,4,7 and Walkthrough §§6,13. Both in v4_snapshot.c and both reachable in RC checkout. |
| 8e85584, 4b66ab0, 168debf | Preview.3 evidence, section synchronization and precise test path/timing correction | Keeps application operation latency, process wall and phase sum separate. Replaces operational-only checkout in Walkthrough and Validation. |

## Module obligations

| Files | Current contract or invariant and guide location | Evidence and exclusions |
| --- | --- | --- |
| order.c, order_bulk.c, lks_order_internal.h | Walkthrough §§2–3, Internals §§2–6,11. Stable independently allocated occurrence, B64/min32, AVL heights and exact chain order; prepares optional spare before releasing a moved occurrence. | v4_order and v4_mutation sequence/identity, probe bounds, zero fallback, revision and OOM. No arbitrary recovery guarantee. |
| managed_order.c | Walkthrough §4 and Internals §7. Copy comparator descriptor, borrow context, stable equal upper bound, reject same-source reentry. | v4_order / v4_mutation: stable oracle, reentry and remove/edit/reinsert churn. Application compare consistency required. |
| snapshot.c, lks_snapshot_internal.h | Walkthrough §5, API §§B,D,E, Internals §§8–9. Cumulative ends include empty rows; marker independent from source address; acquire/release before publication, borrow callback bytes only through immediate copy. | v4_snapshot and v4_marker_threads. Source destruction/revision and same-revision data independence; concurrency of immutable views does not make live mutation internally synchronized. |
| snapshot_key.c, snapshot_wire.c, snapshot_restore.c | Walkthrough §§6,8,11; Internals §§10–11. Same-domain ordinal order, full parse before allocation, resolve-first immutable history and fresh Handles, strict V3 import. | Frozen literal wire, malformed/truncated input, offset-one access, old LK1, restored mixed tags, reached OOM. No built-in durable commit or application rollback. |
| immutable_group.c, lks_immutable_group_internal.h, sort.c, lks_sort_internal.h | Walkthrough §7 and Internals §12. Stable left-biased merges, borrowed items, owned arrays, no partial output. | v4_immutable_group and API consumer assertions. Comparator state remains external. |
| path.c, path_text.c, path_order_key.c, path_compare.c, slot_codec.c and path/slot/legacy headers | Walkthrough §8 and Internals §13. Private legacy decoding, checked arithmetic and semantic key validation. | LK1 fixtures and parser torture. V3 live tree, gap, mutable group and bulk and their private policy headers are historical regression-only paths. |
| lks_base.c, lks_alloc.c, lks_alloc_internal.h | Walkthrough §9 and Internals §14. Frozen statuses, checked allocation accounting and ownership. | Public manifest and diagnostic allocation injection. Requested bytes exclude allocator internals. |
| include/layerkeysort.h, cmake/ProductionSources.cmake, CMakeLists.txt, tools/amalgamate.py and validation tools | API §§A–F and Validation §§2,4–5. 64 functions and 14 types, 16 production modules, stable fixtures, source/install/single-file C and C++ interfaces. | API manifest, CTest, distribution consumers and release asset wrapper. Mini and Workbench are independent products. |

## Four-guide synchronization

| Guide | Current section changes | Confirmed retained content and limits |
| --- | --- | --- |
| Walkthrough | §§3–4 prepare/publish and comparator, §11 restore, §13 exact validation checkout | §§7–9 stable sort and V3 import remain; application transaction responsibility retained. |
| API reference | §A frozen signature/definition index and compare validation order; §F view/owner boundaries | All 64 entries and type/status tables retained; no new ABI or public contract. |
| Internals | §§3,6,11 retired rank, arithmetic, spare and bulk restore | §§8–10 compact rows and fixed width wire remain; allocator and layout conclusions remain platform-qualified. |
| Validation | §§1,3,7 exact checkout, actual assertions and failure triage | §§4–6 fixtures/consumer checks remain; archived timings are inherited and separate from RC CI. |

Actual run links and qualification live in the [RC stage record](../V4_2_0_RC1.md). The complete [FULL-RI records](../V4_2_0_PREVIEW3.md#technical-debt-impact) are retained.

## Repair-level traceability

| Implementation obligation | Exact introducing change | Current explanation | Observed evidence and boundary |
| --- | --- | --- | --- |
| Compare dispatch and signed fallback | 22544cf2, then 61e9290e | Internals §3 explains same-block index, tagged comparison, each ancestor/sibling sign, height-equal direction, and unused subtree count | Source `lks_order_compare`; `preview1_precedence_and_shifts` in v4_mutation and restored mixed-tag tests. No constant-time worst-case promise. |
| Bounded assignment and counts | 61e9290e; exhaustion test 21f6fc4; diagnostic monotonicity c977b0fd | Internals §3 includes both sentinel values, stride and minimum, deterministic growth, width cap and counter scopes | Source `v4_tag_attach/v4_check_node` plus `preview12_block_tags`. Historical midpoint exhaustion, B32/B128 and direct transfer remain distinct rejected designs. No arbitrary recovery guarantee. |
| Bulk spacing | 61e9290e | Internals §11 gives the spacing formula and resolve-first behavior | `src/order_bulk.c`; `capture_restore` checks ordered restored traversal, tags and both comparison routes at 3ac82d26. New Handles do not restore earlier addresses. |
| Validation provenance | d1132a26; initial candidate 643d8f79 | Validation §§1–5 and Walkthrough §13 distinguish frozen operational source, test checkout and actual run | Initial RC CTest and sanitizer [45518848531](https://github.com/RXY712200/LayerKeySort/actions/runs/45518848531). Later heads require their own evidence. |

## API and walkthrough reconstruction review

At `0701951` the API guide acquired a complete English edition, with all 64 table entries in the same order as the Chinese edition and the frozen header. The review reread the public header and current sort, managed, snapshot, key, wire, restore and immutable-group implementations, plus explicit compare/move/remove/Cursor entry points. API §§B8,D,E retain status precedence, output initialization, optional outputs, callback byte lifetimes, shared-item mutation and nontransactional reinsertion. Both C snippets were extracted from the English guide, compared to the Chinese code with comments removed, compiled/linked against the existing Release library under GCC C17 `-Wall -Wextra -Wpedantic -Werror`, and executed successfully. That is a documentation-consumer check, not a new production change.

At `364b98b` all numbered Walkthrough sections 0–14 gained an English counterpart. A source-first review found and corrected two additional body defects in both languages: §2.1 omitted the actual block `tag` member, and §3.6 called revision comparison the first Cursor check when the implementation first validates arguments and rejects callback reentry. The summary now includes bounded labels and AVL fallback. Section numbering, exact build-command block, six contemporary elapsed ratios and local link destinations were checked between editions. These checks establish those concrete properties; full cross-document semantic review remains open in the durable checklist.

After these documentation changes, the existing local strict GCC Release and Debug builds each ran CTest 39/39 successfully, including API freeze, wire golden, snapshot and mutation tests. The production source is unchanged. Historical timing campaigns were not repeated. A Workbench hosted package-download failure at run [45535306231](https://github.com/RXY712200/LayerKeySort/actions/runs/45535306231), job 138940331524, was HTTP 503 in `extract_release.py` before testing; one failed-job retry was requested without changing enforcement. Its result must be checked separately; neither this receipt nor initial RC CI substitutes for final-head verification.

A later bilingual identifier pass compared the actual guide bodies, not only file presence. It restored exact public entry names in the English walkthrough, private codec/helper names in Internals, and source filenames plus sequence_check in Validation. The behavior had been translated, but the abbreviated names impaired source navigation. The API namespace_size member is now explicit in both languages. Numeric-literal screening of the new historical companions also restored the explicit downstream v4.1.0 release qualifier. These are explanation corrections only.

## Architecture companion language repair after 8441232

The historical English V4_ARCHITECTURE.md remains the same Git blob as at 84412320fffecdd60da2b89aef17adb6da4adfab. The Chinese companion was rewritten in full, retaining the existing local repairs. Sections 1–2 retain selection/evidence qualifications and the complete ownership table; §§3–4 retain concatenation, rank comparison, B128/H occupancy, one-split and pair-repair termination arguments; §§5–6 retain handle/cursor lifetimes, no-wrap revision, destination preparation and stable upper/lower bounds. Sections 7–9 retain immediate association copies, source-marker lifetime, order-only currentness, export-domain restrictions, resolve-first restoration and allocation-free commit. Sections 10–12 retain all V3 dispositions, external synchronization, structural complexity and storage targets. Sections 13–15 retain ten acceptance gates, thirteen stress walkthroughs and unresolved historical implementation work.

Concrete corrections: threaded predecessor/successor links now mean 前驱／后继链接 rather than concurrent threads; ordinary concatenated English explanations were replaced with Chinese sentences; source marker/revision/export domain terminology was normalized; split partitioning explicitly means dividing pointers, not allocating during commit. A subsequent terminology pass repaired the glossary's English export-domain term. All English inline expressions and full SHA literals remain represented, all local link targets exist, and all fifteen numbered sections remain. These checks support this translation repair; they do not establish semantic acceptance of the other 88 inventory items, which remains outstanding. No production, frozen-contract, raw-evidence or historical-English change was made.

## Ordinary-document semantic comparison after 375aa7b

Read both complete editions of examples/README, COMPATIBILITY, INTEGRATION and ARCHITECTURE. The examples preserve all seven targets, public-header-only use, borrowed payload ownership and Stable applicability. Compatibility preserves all five prohibited silent changes, 64/14 surface, static-only ABI, historical 64/43 symbol observation, private B64 policy and V3 boundary; no substantive change required. Integration comparison exposed missing Chinese detail: eight-bit bytes, sixteen production modules/four regression-only modules, main version qualification, SameMajorVersion and exact ZIP filenames. These are now explicit; the English edition gains reciprocal Chinese navigation. ProductionSources.cmake and CMakeLists.txt were read to check the module list, STATIC target, CMake minimum, install default and package compatibility mode. Architecture's English entry links now lead to all four complete English guides rather than describing only Chinese guides; Chinese resident terminology is corrected. Other checklist items remain pending; this receipt does not claim whole-corpus acceptance.

## Usage and development semantic review after 706842d

Read both complete Usage and Development editions. Usage Chinese omitted Batch borrowed-view lifetime, independently owned merge results, deserialized snapshot provenance, and explicit item/context/Cursor lifetimes. These are now included, with reciprocal language links and the Chinese selection destination. Header GroupBatch ownership comments and immutable_group.c group_at/merge_all implementation confirm the ownership distinction; header currentness comments explicitly exclude loaded snapshots. Development's six synchronization obligations, frozen-contract restrictions, same-PR ledger, candidate/Stable distinction and historical evidence priority match across languages. Its English guide table now uses the existing English editions. These findings concern documentation parity only; remaining checklist entries are not implicitly accepted.

## Local CTest rerun on 6f5f2df

Debug and Release each passed 39/39 using the existing strict GCC build trees. The local CTest logs contain 39 passing records each. The tracked `src`, `tests`, `include`, `cmake` and root `CMakeLists.txt` trees match the `d1132a26` test baseline. This receipt records local test execution on the unchanged implementation and does not substitute for hosted checks on the final PR heads.

The Preview.3 test patches were checked directly. `754b3088` uses aligned backing arrays but passes offset-one pointers for both deserialize and serialize, then compares the serialized bytes to the original fixture. `3ac82d26` deliberately clears two intermediate tags when a restored order has at least four blocks, checks invariant validity and both directions of mixed-tag comparison, then restores the tags and rechecks validity. This establishes fallback coverage in a constructed state. It does not establish that ordinary restore produces zero tags.

Rechecked the actual Preview.1.2 patch at `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` against Internals §3 in both languages: the minimum stride, open-right cap, deterministic expansion, 64-block limit and zero-tag AVL fallback agree with `v4_tag_attach` and `lks_order_compare`. Rechecked Preview.2 at `fe00536e7bd7f4e5128432ea4a29e54158074903` against Internals §§10–11. The fixed-width helpers only receive widths 4 and 8. The guide now distinguishes full input validation, allocation, and the second pass that fills row endpoints and copies associations. The Chinese guide now specifies one N-entry temporary pointer array held through bulk construction, as implemented in `lks_snapshot_restore_order`.

Compared the pending ordinary VALIDATION additions with the actual snapshot test bodies. Offset-one buffers are checked against literal fixture bytes; mixed tags are deliberately introduced after restore, verified in both comparison directions, then restored. The ordinary guides now identify these exact regression commits in both languages. On local documentation checkout 950bdf2 with these additions, the existing Release tree passed wire golden, snapshot and snapshot smoke (3/3), and verify_v4_api.py reported 64 functions. This targeted run does not settle final hosted validation or the outstanding corpus review.

## Architecture and historical decision semantic reconciliation after d6b16d1

Compared all fifteen architecture sections and all thirteen stress walkthroughs in both editions. Sections 1–2 preserve the evidence erratum and ownership matrix; 3–6 preserve concatenation, historical rank/B128 policy, occupancy repair, lifetimes and prepared moves; 7–9 preserve immediate copies, marker/currentness, domains, migration and allocation-free publication; 10–12 preserve V3 dispositions, synchronization, structural/storage targets; 13–15 preserve ten gates, every stress case and historical unresolved work. The English blob is unchanged. These historical targets are not current implementation claims: `lks_order_internal.h`, `order.c` and `order_bulk.c` implement B64/min32, tags plus ancestry fallback, and evenly spaced bulk tags as documented in current Internals §§2–3,11.

The companion decision record still mixed English fragments with Chinese, and its use of “threads” obscured predecessor/successor links. Rewrote it against all nine English decisions, the eighteen adversarial cases, eight residual risks and historical test receipts. Preserved rejected F/B128-era decisions as historical, the five/four event erratum, all adverse export measurements, source links and limitations. The historical English remains unchanged.

Read both complete Validation guide editions and the complete API guide families and ownership/error matrices against the public header. Corrected English navigation that still led to Chinese-only guide destinations. The accepted ordinary VALIDATION regression additions were preserved. Checked the legacy measurement metadata pairs for seeds, warmups, diagnostic/timing separation, cumulative replacement counts, single-sample 1m limits and adverse endpoint costs; these remain historical observations, not current Full performance claims.

Embedded `README.txt` distribution prose was still English-only. Added a complete Chinese edition inside the existing generated README, preserving both compiler commands and caller ownership. This changes documentation bytes in the ZIP, not generated C/header or production behavior; the earlier ZIP digest remains a historical receipt. Fresh packaging/consumer verification must use the new README and record its new digest.

The bilingual README package passed `validate_distribution.py --generator Ninja --c-compiler gcc --cxx-compiler g++`: source, installed and amalgamation C/C++ consumers executed successfully. `verify_v4_api.py` passed 64 functions. `package_release.py --output-dir build/rc-assets-bilingual` and `validate_release_assets.py --asset-dir build/rc-assets-bilingual` passed, including two deterministic builds and checksum rejection. New documentation-package ZIP SHA-256: `b28e33b0351664ad03e6b55535934d28a477c9ebe0eb2398bf765168491d63ea`. The older digest above remains valid for the earlier English-only package.

## Final corpus reconciliation after a13bab6

This review reused the earlier source/guide comparisons recorded above rather than repeat accepted regression work. It completed the remaining whole-body bilingual comparisons recorded per row in the checklist. The historical architecture's fifteen sections and thirteen stress cases, the decision record's nine decisions/eighteen adversarial cases/eight risks, the four complete guides, current ordinary documents, historical stage/release/research records and task companions were compared by meaning. The historical English originals remain unchanged. The final delivery still requires exact-head hosted verification and independent review; an inventory count is not acceptance.

Concrete findings and corrections:

- V4 Preview.1/3 companions used threaded terminology that could imply concurrent threads. They now describe predecessor/successor links. Stable's symbol and model/population wording was repaired. Holdout means independently held-out validation, not retained history.
- BENCHMARKS had one remaining unqualified RC.1/Preview.3 reference inside its V3 history. It now explicitly names V3. Evidence text now says median of matched-cell elapsed ratios; it does not divide independent pooled medians. Both language editions retain contrary OM/list cases, all eight variants and all 32 scatter coordinates.
- Historical task companions still contained concatenated ordinary English descriptions. Rewrote Preview.1, its continuation/completion, 1.2, 1.2.1, 2, 3 and the RC task against their complete English requirements: state/authority, technical numbered duties, protocol thresholds, comparison and failure boundaries, source-bound guide/consumer obligations and Git stopping permissions. Exact previous full SHA sets, link destinations and inline literals were preserved. The source task remains authoritative; translations do not reopen historical authorization. Supplemental comparisons checked actual text as well as pins.
- Historical design/benchmark navigation still used relative paths for the then-current V3 API. The companions now explain that temporal boundary and provide immutable v3.1.0 API/compatibility/architecture links. The original English text and historical results were preserved.
- Compared every release body in CHANGELOG, the V3 measurement metadata and V4 smoke/capacity records, the live-core selection history, A–F inventory and all seven RI dispositions. Preserved historical API counts, B128-to-B64 evidence, rejected midpoint/count/transfer/capacity designs, timing/compiler scopes, adverse observations, single-sample limits and unmeasured alternatives. No new benchmark was run.
- Repo governance companions were compared to their original observations: 403 does not prove absent protection, a Git reference is not an offline backup, the exact-head exception is target-controlled, and scope-guard-selftest is not the trusted scope-guard. These readings change no policy, workflow or repository setting. Repo entry prose was compared in both languages; final candidate links are a separate Repo-owned delivery after Full is frozen.

### Source and consumer checks

The entire tracked src/tests/include/cmake/CMakeLists.txt tree still equals d1132a26ca7b2c65bfd512fa2eb29f1b16793c08. Rechecked the actual order_bulk.c construction and formula against Internals §11. All 64 API index rows were checked by function name at their exact header, implementation and Chinese contract line: no mismatches. The 14 frozen type declarations remain unchanged. The accepted ordinary VALIDATION and test-patch audit additions were preserved.

The consumer handoff's three editions now record the README-only bilingual distribution change from a13bab620ea289a64188c58af1b916f11cfb7d2b. The generated C/header, example, license and five-member layout are unchanged. The new ZIP digest above applies; the older initial-candidate checksum remains historical.

### Executed local verification

- Existing strict Ninja/GCC Debug and Release builds were rebuilt successfully; both CTest runs passed 39/39 (build/rc-review-debug-ctest.log and build/rc-review-release-ctest.log). No warning or assertion was weakened.
- All seven Release examples executed successfully: example, ordered_example, layer_list_example, order_example, managed_order_example, snapshot_example and group_snapshot_example.
- verify_v4_api.py passed 64 functions. Fresh distribution C/C++ source/install/amalgamation consumers, deterministic package regeneration and checksum-tamper rejection passed as recorded above. These local checks are not the hosted platform matrix.
- RC generate.py reproduced derived JSON and all ten SVGs without a diff. Existing complete bilingual visual inspection remains applicable because no figure coordinate, style or generated byte changed in this review.
- Local Markdown validation examined 169 in-scope tracked Markdown files and 1,099 local links/anchors with zero missing destinations at this review. Recheck after audit edits and final Repo pins. UTF-8 decoding succeeded. Historical raw data, fixtures and private policy material were not edited.

### Remaining engineering limits and Technical Debt Impact

FULL-RI-001/002 retain the accepted B64/tag/AVL/range-update implementation and scoped native comparison evidence. FULL-RI-003 remains partial (independent copies, N-pointer restore scratch and unmeasured bulk Treap). FULL-RI-004 retains scoped adapter compatibility with an unchanged official dependency and staging/WAL-dominated application cost. FULL-RI-005 retains guarded scale/sparse-survival evidence with longer history, multi-million and arbitrary tag-recovery limits. FULL-RI-006 gains completed documentation-to-source/test reconciliation; FULL-RI-007 gains precise bilingual selection/accounting/navigation. No production debt is closed by translation. Existing evidence, classifications and reopening triggers in the linked Preview.3 seven-ID table and Issues #3/#27 remain effective. Final-head hosted checks and later real main-target integration are separate gates; no merge, tag, release or dependency promotion occurs here.
