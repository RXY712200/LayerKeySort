# Full selection guide

Select Full when the application needs relative occurrence edits and precedence alongside independently owned history, a defined wire format, or comparator-managed order. The measured arbitrary-relative lifecycle advantage applies to the tested threaded Treap plus copied history implementation, whose restoration inserts nodes individually. An optimized bulk Treap remains a relevant unmeasured alternative. Full's integrated failure, provenance and callback contracts represent separate engineering requirements.

For endpoint-only workloads, a linked sequence plus flat immutable history can be simpler and faster. In the measured 13 cells Full's native lifecycle wall-time ratios against that alternative were 1.137 on Windows and 1.136 on Linux. For ordered edits without history, compare native OM libraries under your actual allocation and payload requirements. The measured tag-range OM often won small update-heavy conditions. These comparisons include adapter and toolchain effects.

Full's snapshot copy and resolve-first scratch remain proportional to the retained state. Database-backed deployments should measure durability separately. OrdWeave evidence shows that its global occurrence-index publication policy and disk flush dominate distinct application modes. A dependency substitution leaves those responsibilities with OrdWeave.

See the [architecture audit](design/V4_2_0_PREVIEW3_ARCHITECTURE.md) and [stage evidence](V4_2_0_PREVIEW3.md).

## Selection boundaries and established mechanisms

| Condition | Evidence and cost to budget |
| --- | --- |
| Arbitrary relative edits with indexed history | Across the aligned 1k/10k/100k study, final/Threaded Treap process-wall ratios were 0.580 Windows and 0.526 Linux. The tested Treap restores by incremental insertion. Bulk-balanced restore remains unmeasured. |
| Endpoint edits with flat history | Over 13 cells, Full / flat was 1.137 Windows and 1.136 Linux. Prefer testing the flat composition for this workload. Its arbitrary precedence would have different costs. |
| Precedence and mutation without history | Full / native tag-range was 1.052/1.020 and Full / native list-range was 0.909/0.926. Small update-heavy cases favor native OM. Toolchain, allocator and adapter differences apply. |
| Comparator-bound edits | ManagedOrder provides stable equal occurrences and bound lookup. The comparator and context remain borrowed and must preserve the ordering invariant. The Explicit numbers do not measure managed equivalence. |
| Durable application | OrdWeave replay supports adapter compatibility. Final / Preview.1.2 whole-process ratios were 1.002 memory and 0.997 persistent. WAL and global publication deserve separate budgets. |

The [protocol](../benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.md) varies cadence, retention, association width and namespace, with separate held-out histories. The reported ratios summarize per-cell medians from seven formal runs. These are measured workload classes without an inferred universal crossover. Snapshot retention adds copied bytes and row ends. Restore adds N temporary pointers alongside the fresh Order. Measure requested retained and peak bytes separately from resident memory, wire size and allocation traffic. Broad Treap capture diagnostics omit sequence bytes. Held-out diagnostics account for those bytes separately.

Dietz and Sleator's order maintenance addresses dynamic insertion/deletion and precedence. Bender, Cole, Demaine, Farach-Colton and Zito described simpler labeling approaches. Full combines established labels, bounded blocks, and an AVL fallback with independently allocated Handles and an immutable history lifecycle. The engineering benefit is shared occurrence, provenance and failure semantics across operations. The measurements test the integrated design and its costs. See the [native OM implementation](https://github.com/j-hui/order-maintenance/tree/ac4b902d103beb30cee3316d20cd10701f52d15a).

An alternative composition needs its own stable identity, revision and provenance, capture copies, wire validation, ordered resolution and publication rules where required by the application. The baselines align successful behavior. Recoverable failure and callbacks differ. Durable IDs, schema, WAL, resolver effects and external synchronization remain the application's responsibility. Database-native indexing, bulk Treap restore, and sustained multi-million retained histories require separate measurements.
