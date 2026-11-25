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
