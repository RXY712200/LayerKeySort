# Full source-bound inventory

Pin all current production paths below to [fe00536e](https://github.com/RXY712200/LayerKeySort/tree/fe00536e7bd7f4e5128432ea4a29e54158074903). Stable control is 70d7f0ef5857b54381b863d292a213909d87459f; accepted Stable guides are 1a175f91e4c338c2afe6a73a9f8f9c5b149768e5. Prior guide checkpoint is 01bc01e2d9ddbda448d9ef0807150f419203b580. Preview.3 adds two test revisions without a production representation change.

| File or module | Inspected obligation | Oracle and residual boundary |
| --- | --- | --- |
| order.c, order_bulk.c, lks_order_internal.h | B64 occupancy, stable resident allocation, owner and slot positions. AVL rotations, block links and counts, split/repair/merge, vacated slots. v4_tag_attach bounded writes/probes, strict nonzero subsequence; lks_order_compare uses AVL if either tag is zero. | v4_order, independent mutation model and concentrated diagnostics, reached OOM. Local success does not guarantee arbitrary recovery. |
| managed_order.c, lks_order_internal.h | Comparator bound search, stable ties, busy guards, staged mutation, revision/cursor semantics | v4_order and v4_mutation. Mutable comparison state must preserve ordering; synchronization is external. |
| snapshot.c, snapshot_key.c, lks_snapshot_internal.h | Immediate independent copies and cumulative ends, marker lifetime/revision, LS1 same-domain ASCII order | v4_snapshot, v4_marker_threads, freeze fixtures. Application payload identity and effects remain external. |
| snapshot_wire.c, snapshot_restore.c, order_bulk.c | Four/eight byte unsigned operations, validates entire input before allocating, ordinal resolve first, bulk creation and rollback | v4_snapshot, v4_wire_probe, v4_freeze, malformed fixtures and offset-one regression at 754b3088. Mixed-tag restore coverage at 3ac82d26. Scratch remains N pointers. |
| immutable_group.c, lks_immutable_group_internal.h, sort.c, lks_sort_internal.h | Stable duplicates and left-biased merge, independent capture, overflow checks and no failed publication | v4_immutable_group and public consumer tests. Comparator and items remain borrowed. |
| path.c, path_text.c, path_compare.c, path_order_key.c, slot_codec.c and path/slot/legacy headers | Compatibility parsing, ordering and checked arithmetic for imported LK1. | Parser torture, frozen fixtures and v4_snapshot restore/import. No public V3 mutable Tree surface. |
| gap.c, tree.c, group.c, bulk.c and tree/group/bulk/policy headers | Legacy diagnostic object set with historical contracts | V2/V3 regression tests; excluded from the production source manifest. |
| lks_alloc.c, lks_alloc_internal.h, lks_base.c, include/layerkeysort.h | Fixed public statuses, checked allocation accounting, reached injection, 64 functions/14 types | API manifest, Win32 and sanitizer gates, teardown counters. No dangling Handle validity promise. |
| ProductionSources.cmake, top-level CMake, amalgamation and distribution tooling | Production manifest, installation/export, single-file consumer, C/C++ link surface | Independent consumer builds, examples, release asset checks. No release generated. |

The [architecture audit](../../../docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md) records preparation/publication boundaries. The [permanent live-core history](../../../docs/design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.md) retains midpoint exhaustion, metadata experiments, B32/B128 and transfer rejection. Treat offset-one and restored-tag additions as test deficiencies addressed, fixed-width wire as a retained optimization, snapshot copies and AVL fallback as deliberate resource tradeoffs, arbitrary zero-tag recovery as an unproved guarantee, and Workbench attempts 1–2 as an environment problem. Whole-database durability remains external. See the [seven-ID disposition](../../../docs/V4_2_0_PREVIEW3.md#technical-debt-impact).

## Historical debt lineage

| Original Issue #3 family | Disposition and related investigations |
| --- | --- |
| A. Live search, managed insertion and broad moves | Partially improved. Exact shifted locations from 22544cf2, block precedence from 61e9290e. RI-001/002/007 establish live explicit comparisons; managed scaling and cache attribution remain open. |
| B. Stable resident allocation | Retained one allocation per Handle. Measured sparse survival through 1m is evidence for reclamation in these states (RI-005). A slab or pool would need equivalent failure, stability and reclamation results. |
| C. Occupancy and maintenance | B64 retained. B32, B128 and pointer transfer rejected on measured time/resource gates. Diagnostic nonzero ordering across zero blocks added at c977b0fd (RI-001/002/006). |
| D. Snapshot/history and scratch | Wire conversion improved at fe00536e; 92fb7fad covers same-revision independence. Copies, retention and resolve-first scratch remain (RI-003/005). |
| E. Consumer lifetime and recovery | Native dependency-only replay and consumer ledger clarify the interface. Application-owned persistence and side effects remain unchanged (RI-004/007). |
| F. Verification and optional integration | Added offset-one and restored fallback tests. Checkpoints and platform runs cover bounded inputs. Long-duration fragmentation, 10m sanitizer and optional bindings remain outside this verification (RI-005/006). |

The issue retains the [historical families and active records](https://github.com/RXY712200/LayerKeySort/issues/3); [plan #27](https://github.com/RXY712200/LayerKeySort/issues/27) defines the current numbered scope.
