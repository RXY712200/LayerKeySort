# V3 / Preview.4 to V4 migration

V4 is a source-breaking major release. Keep v3.1.0 if hierarchical Path/Tree
contracts are required. Rebuild against the final V4 header; no alias layer.

| Old workflow | V4 replacement |
|---|---|
| Manual Path + Tree insertion | LksOrder relative/end insertion with resident handles |
| Physical node navigation | Logical traversal / same-source lks_order_compare |
| OrderedTree | LksManagedOrder with stable comparator-visible item data |
| Path-based Group/Batch | Flat LksGroup/LksGroupBatch; no group_path_at |
| Persist live Path/LK1 | Capture Snapshot, persist LS1 and LKS4SNP1 |
| Load published V3 LK1 | Strict lks_order_import_v3_lk1 plus caller-owned items |

Before V3: `lks_tree_insert_path(tree, path, item, &node)`.
After V4: `lks_order_insert_before(order, anchor, item, &handle)`.
Handles survive unrelated mutations and moves; own removal/source destruction
expires them. Keep business IDs independently; numerical pointer values are not order.
Managed order has no arbitrary move facade: remove/edit/reinsert to change keys.

Preview.4 LksImmutableGroup/Batch becomes LksGroup/Batch; all 14
lks_immutable_group_* names become lks_group_* / lks_group_batch_*.
Same-named V3 Groups have different semantics; migrate use rather than blindly
replace headers. Input pointer arrays are copied; payload ownership unchanged.

Strict import sorts by published LK1 semantics, rejects malformed/noncanonical/
overflowing/duplicate coordinates, and creates fresh live handles. Levels beyond
local SIZE_MAX are rejected. Keys are not retained, no Path exposed publicly.

Snapshot captures copy application-supplied namespace and identity associations.
Loaded snapshots have no source provenance/currentness. Resolver maps associations
to caller-owned objects; restore borrows those objects and gives fresh handles.
Historical snapshot remains after source/item destruction. LS1 is an ordinal
within a namespace, not live position. Namespace uniqueness belongs to application.
Full capture/export costs O(N+A). See [freeze](V4_FREEZE.md), [API](API.md) and
[public snapshot example](../examples/snapshot.c). Stable remains v3.1.0;
Release Candidate is next, not another Preview.

## Complete Preview.4 rename table

| Preview.4 function | Preview.5 function | Reason |
|---|---|---|
| `lks_immutable_group_batch_build` | `lks_group_batch_build` | Sole flat V4 Group model |
| `lks_immutable_group_batch_destroy` | `lks_group_batch_destroy` | Sole flat V4 Group model |
| `lks_immutable_group_batch_group_at` | `lks_group_batch_group_at` | Sole flat V4 Group model |
| `lks_immutable_group_batch_group_count` | `lks_group_batch_group_count` | Sole flat V4 Group model |
| `lks_immutable_group_batch_group_size` | `lks_group_batch_group_size` | Sole flat V4 Group model |
| `lks_immutable_group_batch_merge_all` | `lks_group_batch_merge_all` | Sole flat V4 Group model |
| `lks_immutable_group_batch_size` | `lks_group_batch_size` | Sole flat V4 Group model |
| `lks_immutable_group_build` | `lks_group_build` | Sole flat V4 Group model |
| `lks_immutable_group_destroy` | `lks_group_destroy` | Sole flat V4 Group model |
| `lks_immutable_group_item_at` | `lks_group_item_at` | Sole flat V4 Group model |
| `lks_immutable_group_merge` | `lks_group_merge` | Sole flat V4 Group model |
| `lks_immutable_group_size` | `lks_group_size` | Sole flat V4 Group model |
| `lks_immutable_group_snapshot_capture` | `lks_group_snapshot_capture` | Sole flat V4 Group model |
| `lks_immutable_group_snapshot_is_current` | `lks_group_snapshot_is_current` | Sole flat V4 Group model |

## Removed legacy names

The following V3 names are absent from the primary V4 header:

- `lks_group_batch_total_size`
- `lks_group_path_at`
- `lks_ordered_tree_create`
- `lks_ordered_tree_destroy`
- `lks_ordered_tree_find_path`
- `lks_ordered_tree_insert`
- `lks_ordered_tree_locate`
- `lks_ordered_tree_remove_path`
- `lks_ordered_tree_root_child_at`
- `lks_ordered_tree_root_child_count`
- `lks_ordered_tree_size`
- `lks_path_after`
- `lks_path_append`
- `lks_path_append_at_level`
- `lks_path_before`
- `lks_path_between`
- `lks_path_clone`
- `lks_path_compare`
- `lks_path_create`
- `lks_path_create_at_level`
- `lks_path_create_zero`
- `lks_path_depth`
- `lks_path_destroy`
- `lks_path_direction`
- `lks_path_format`
- `lks_path_get_level`
- `lks_path_get_slot`
- `lks_path_order_key_format`
- `lks_path_order_key_length`
- `lks_path_order_key_parse`
- `lks_path_parse`
- `lks_path_text_length`
- `lks_tree_create`
- `lks_tree_destroy`
- `lks_tree_find_path`
- `lks_tree_insert`
- `lks_tree_node_child_at`
- `lks_tree_node_child_count`
- `lks_tree_node_item`
- `lks_tree_node_parent`
- `lks_tree_node_path`
- `lks_tree_rekey`
- `lks_tree_remove_path`
- `lks_tree_root_child_at`
- `lks_tree_root_child_count`
- `lks_tree_size`

Old V3 `lks_group_*` names that are reused now implement flat V4 semantics.
All 57 V3-specific contracts are removed/replaced; only generic sort/status remain.

RC.1 uses the unchanged Preview.5 frozen declarations and formats; see
[RC validation](V4_RC1.md). Recommended Stable remains v3.1.0.
