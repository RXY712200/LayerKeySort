# Public examples

All examples include only `layerkeysort.h`; caller owns item payloads. Build with
`LKS_BUILD_EXAMPLES=ON` (default for a standalone checkout).

| Source | Target | Purpose |
|---|---|---|
| basic.c | layerkeysort_example | Stable V3 sort/Group basics |
| ordered_tree.c | layerkeysort_ordered_example | Stable V3 comparator-managed coordinate Tree |
| layer_list.c | layerkeysort_layer_list_example | Stable V3 manual layer coordinates |
| live_order.c | layerkeysort_order_example | Experimental V4 explicit residence order |
| managed_order.c | layerkeysort_managed_order_example | Experimental V4 comparator-managed residence order |
| snapshot.c | layerkeysort_snapshot_example | Experimental V4 historical IDs, LS1, blob and fresh restore |

The snapshot example captures business IDs, formats keys, serializes, changes and
destroys its source, reads historical associations, loads and resolves a fresh
order. Its four-byte namespace is illustrative, not a global uniqueness scheme.
Read [Preview.3 contracts](../docs/V4_PREVIEW3.md) before choosing namespaces or
persistence associations. Stable recommendation remains v3.1.0.
