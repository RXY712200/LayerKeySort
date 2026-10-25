# Public V4 examples

These public-API examples apply to the current published [Full v4.1.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0). No new public functions or
usage semantics were added by the private memory-layout improvements.

All include only layerkeysort.h, check errors and preserve item ownership.

| Source / target | Purpose |
|---|---|
| basic.c / layerkeysort_example | Minimal create/insert/query/remove/destroy |
| ordered_tree.c / layerkeysort_ordered_example | Managed comparison; filename historical |
| layer_list.c / layerkeysort_layer_list_example | Editable relative layers |
| live_order.c / layerkeysort_order_example | Handles / traversal |
| managed_order.c / layerkeysort_managed_order_example | Managed equality/removal |
| immutable_group.c / layerkeysort_group_snapshot_example | Final flat Group/Batch snapshot |
| snapshot.c / layerkeysort_snapshot_example | LS1/wire/restore/migration |

Run named executable after CMake build (Debug directory for MSVC).
ZIP includes basic.c as example.c. demo/main.c is a private historical regression
runner, not V4 first-use surface. [Freeze](../docs/V4_FREEZE.md).
