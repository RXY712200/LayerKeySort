# V4 API reference

Preview.5 froze the V4.0 source contract: 64 functions, 14 public types.
RC.1 preserves it unchanged. See [RC record](V4_RC1.md).
Include `<layerkeysort.h>` only. [Freeze](V4_FREEZE.md) specifies ownership,
statuses and persistent compatibility; [migration](MIGRATION_V3_V4.md) covers the
major source break. v3.1.0 remains recommended Stable.

## Status / comparator / sort

LksStatus uses explicit numbers; unknown integers return "Unknown status".
Comparator contains compare/context: ManagedOrder retains descriptor, borrows
context until destruction; Group/sort borrow per operation. Comparison semantics
must remain consistent. lks_sort stable-sorts pointers in place; OOM preserves
input, fewer than two items allocates nothing.

## Explicit order / handles / cursors

Order owns structure, borrows non-NULL items. Relative/end insertion creates
occurrences. Remove frees structure only; own removal/source destruction expires
handle. Moves preserve handles; foreign valid handles rejected. No-op preserves
cursors/currentness. Traversal is logical order; no rank/physical topology API.
Cursor borrows source, direction0/1; actual mutation invalidates before cached
handle access. Destroy cursor before source. See complete header error comments.

## Managed order

Comparator-managed contextual core, stable upper-bound insertion. Locate gives
predecessor/first equal/successor. Remove uses handle; no arbitrary move/core escape.
Remove before changing comparison-relevant data then reinsert, or reconstruct.

## Immutable Group / Batch

Group owns flat copied array, borrows items, no index/Path. Stable build/fresh merge
with Base-before-Incoming equality. Batch owns sorted chunks; borrowed views expire
with Batch, merge-all favors earlier chunks and returns independently owned Group.
Empty build still requires comparator; chunk size must be nonzero.

## Snapshots / LS1 / wire / restoration

Capture owns copied namespace/associations; callback span must remain valid through
immediate copying after return. Historical rows survive source/items. Currentness
requires existing source marker/mutation generation; loaded snapshots never current.
LS1 exact lower-hex namespace/ordinal grammar and LKS4SNP1 explicit BE format are
frozen. Format/serialize allocate nothing; text length excludes NUL, binary length
counts exact bytes; undersized destination remains untouched.
Restore resolves non-NULL caller-owned objects into fresh live handles; failures
publish NULL and free temporary state. Callback side effects are not rolled back.
Strict LK1 import preserves legacy coordinates without public legacy live APIs.

## Complete frozen function list

Exact signatures/per-function requirements: [public header](../include/layerkeysort.h).
Typed compile/link references cover every declaration; C/C++ consumers exercise
all families. tools/verify_v4_api.py rejects unreviewed signature/layout/name changes.

### Group / Batch

- `lks_group_batch_build`
- `lks_group_batch_destroy`
- `lks_group_batch_group_at`
- `lks_group_batch_group_count`
- `lks_group_batch_group_size`
- `lks_group_batch_merge_all`
- `lks_group_batch_size`
- `lks_group_build`
- `lks_group_destroy`
- `lks_group_item_at`
- `lks_group_merge`
- `lks_group_size`
- `lks_group_snapshot_capture`
- `lks_group_snapshot_is_current`

### Managed order

- `lks_managed_order_compare`
- `lks_managed_order_create`
- `lks_managed_order_cursor_create`
- `lks_managed_order_destroy`
- `lks_managed_order_first`
- `lks_managed_order_insert`
- `lks_managed_order_last`
- `lks_managed_order_locate`
- `lks_managed_order_remove`
- `lks_managed_order_size`
- `lks_managed_order_snapshot_capture`
- `lks_managed_order_snapshot_is_current`

### Explicit order / cursor / import

- `lks_order_compare`
- `lks_order_create`
- `lks_order_cursor_create`
- `lks_order_cursor_destroy`
- `lks_order_cursor_next`
- `lks_order_destroy`
- `lks_order_first`
- `lks_order_import_v3_lk1`
- `lks_order_insert_after`
- `lks_order_insert_back`
- `lks_order_insert_before`
- `lks_order_insert_front`
- `lks_order_item`
- `lks_order_last`
- `lks_order_move_after`
- `lks_order_move_back`
- `lks_order_move_before`
- `lks_order_move_front`
- `lks_order_next`
- `lks_order_previous`
- `lks_order_remove`
- `lks_order_size`
- `lks_order_snapshot_capture`
- `lks_order_snapshot_is_current`

### Snapshots / LS1 / wire

- `lks_snapshot_association`
- `lks_snapshot_count`
- `lks_snapshot_deserialize`
- `lks_snapshot_destroy`
- `lks_snapshot_key_compare`
- `lks_snapshot_key_format`
- `lks_snapshot_key_length`
- `lks_snapshot_key_validate`
- `lks_snapshot_namespace`
- `lks_snapshot_restore_order`
- `lks_snapshot_serialize`
- `lks_snapshot_serialized_size`

### Generic

- `lks_sort`
- `lks_status_string`
