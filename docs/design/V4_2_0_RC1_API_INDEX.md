# RC.1 public API source index

Validation checkout: d1132a26ca7b2c65bfd512fa2eb29f1b16793c08. Declarations come from the public header; see the [contract guide](../IMPLEMENTATION_API_REFERENCE.zh-CN.md) for status, ownership and failure rules. The fixture verifies the frozen surface.

| Declaration | Header line | Definition | Contract guide line |
| --- | ---: | --- | ---: |
| `LksStatus lks_group_batch_build(void *const *items, size_t count, size_t group_size, const LksComparator *comparator, LksGroupBatch **out_batch);` | 273 | `src/immutable_group.c:68` | 116 |
| `void lks_group_batch_destroy(LksGroupBatch *batch);` | 276 | `src/immutable_group.c:59` | 117 |
| `const LksGroup *lks_group_batch_group_at( const LksGroupBatch *batch, size_t index);` | 280 | `src/immutable_group.c:99` | 121 |
| `size_t lks_group_batch_group_count(const LksGroupBatch *batch);` | 278 | `src/immutable_group.c:95` | 119 |
| `size_t lks_group_batch_group_size(const LksGroupBatch *batch);` | 279 | `src/immutable_group.c:97` | 120 |
| `LksStatus lks_group_batch_merge_all(const LksGroupBatch *batch, const LksComparator *comparator, LksGroup **out_group);` | 282 | `src/immutable_group.c:101` | 122 |
| `size_t lks_group_batch_size(const LksGroupBatch *batch);` | 277 | `src/immutable_group.c:93` | 118 |
| `LksStatus lks_group_build(void *const *items, size_t count, const LksComparator *comparator, LksGroup **out_group);` | 262 | `src/immutable_group.c:24` | 111 |
| `void lks_group_destroy(LksGroup *group);` | 264 | `src/immutable_group.c:15` | 112 |
| `void *lks_group_item_at(const LksGroup *group, size_t index);` | 266 | `src/immutable_group.c:22` | 114 |
| `LksStatus lks_group_merge(const LksGroup *base, const LksGroup *incoming, const LksComparator *comparator, LksGroup **out_group);` | 267 | `src/immutable_group.c:38` | 115 |
| `size_t lks_group_size(const LksGroup *group);` | 265 | `src/immutable_group.c:20` | 113 |
| `LksStatus lks_group_snapshot_capture(const LksGroup *group, const LksSnapshotOptions *options, LksSnapshot **out_snapshot);` | 287 | `src/immutable_group.c:147` | 123 |
| `int lks_group_snapshot_is_current(const LksGroup *group, const LksSnapshot *snapshot);` | 289 | `src/immutable_group.c:157` | 124 |
| `LksStatus lks_managed_order_compare(const LksManagedOrder *order, const LksOrderHandle *left, const LksOrderHandle *right, int *out_order);` | 168 | `src/managed_order.c:108` | 81 |
| `LksManagedOrder *lks_managed_order_create(const LksComparator *comparator);` | 149 | `src/managed_order.c:6` | 73 |
| `LksStatus lks_managed_order_cursor_create(const LksManagedOrder *order, int reverse, LksOrderCursor **out_cursor);` | 181 | `src/managed_order.c:116` | 67 |
| `void lks_managed_order_destroy(LksManagedOrder *order);` | 150 | `src/managed_order.c:17` | 74 |
| `const LksOrderHandle *lks_managed_order_first(const LksManagedOrder *order);` | 166 | `src/managed_order.c:104` | 79 |
| `LksStatus lks_managed_order_insert(LksManagedOrder *order, void *item, const LksOrderHandle **out_handle);` | 152 | `src/managed_order.c:59` | 76 |
| `const LksOrderHandle *lks_managed_order_last(const LksManagedOrder *order);` | 167 | `src/managed_order.c:106` | 80 |
| `LksStatus lks_managed_order_locate(LksManagedOrder *order, const void *query, const LksOrderHandle **out_predecessor, const LksOrderHandle **out_equal, const LksOrderHandle **out_successor);` | 160 | `src/managed_order.c:75` | 77 |
| `LksStatus lks_managed_order_remove(LksManagedOrder *order, const LksOrderHandle *handle, void **out_item);` | 164 | `src/managed_order.c:96` | 78 |
| `size_t lks_managed_order_size(const LksManagedOrder *order);` | 151 | `src/managed_order.c:22` | 75 |
| `LksStatus lks_managed_order_snapshot_capture(LksManagedOrder *order, const LksSnapshotOptions *options, LksSnapshot **out_snapshot);` | 207 | `src/snapshot.c:177` | 82 |
| `int lks_managed_order_snapshot_is_current(const LksManagedOrder *order, const LksSnapshot *snapshot);` | 212 | `src/snapshot.c:190` | 88 |
| `LksStatus lks_order_compare(const LksOrder *order, const LksOrderHandle *left, const LksOrderHandle *right, int *out_order);` | 120 | `src/order.c:427` | 49 |
| `LksOrder *lks_order_create(void);` | 80 | `src/order.c:249` | 36 |
| `LksStatus lks_order_cursor_create(const LksOrder *order, int reverse, LksOrderCursor **out_cursor);` | 179 | `src/order.c:532` | 62 |
| `void lks_order_cursor_destroy(LksOrderCursor *cursor);` | 185 | `src/order.c:559` | 64 |
| `LksStatus lks_order_cursor_next(LksOrderCursor *cursor, const LksOrderHandle **out_handle);` | 183 | `src/order.c:546` | 63 |
| `void lks_order_destroy(LksOrder *order);` | 81 | `src/order.c:255` | 37 |
| `const LksOrderHandle *lks_order_first(const LksOrder *order);` | 110 | `src/order.c:404` | 44 |
| `LksStatus lks_order_import_v3_lk1(const LksV3Lk1ImportEntry *entries, size_t count, LksOrder **out_order);` | 247 | `src/snapshot_restore.c:36` | 66 |
| `LksStatus lks_order_insert_after(LksOrder *order, const LksOrderHandle *anchor, void *item, const LksOrderHandle **out_handle);` | 95 | `src/order.c:341` | 42 |
| `LksStatus lks_order_insert_back(LksOrder *order, void *item, const LksOrderHandle **out_handle);` | 91 | `src/order.c:325` | 40 |
| `LksStatus lks_order_insert_before(LksOrder *order, const LksOrderHandle *anchor, void *item, const LksOrderHandle **out_handle);` | 93 | `src/order.c:338` | 41 |
| `LksStatus lks_order_insert_front(LksOrder *order, void *item, const LksOrderHandle **out_handle);` | 89 | `src/order.c:322` | 39 |
| `void *lks_order_item(const LksOrderHandle *handle);` | 114 | `src/order.c:425` | 48 |
| `const LksOrderHandle *lks_order_last(const LksOrder *order);` | 111 | `src/order.c:406` | 45 |
| `LksStatus lks_order_move_after(LksOrder *order, const LksOrderHandle *handle, const LksOrderHandle *anchor);` | 134 | `src/order.c:528` | 53 |
| `LksStatus lks_order_move_back(LksOrder *order, const LksOrderHandle *handle);` | 131 | `src/order.c:520` | 51 |
| `LksStatus lks_order_move_before(LksOrder *order, const LksOrderHandle *handle, const LksOrderHandle *anchor);` | 132 | `src/order.c:525` | 52 |
| `LksStatus lks_order_move_front(LksOrder *order, const LksOrderHandle *handle);` | 130 | `src/order.c:515` | 50 |
| `const LksOrderHandle *lks_order_next(const LksOrderHandle *handle);` | 112 | `src/order.c:410` | 46 |
| `const LksOrderHandle *lks_order_previous(const LksOrderHandle *handle);` | 113 | `src/order.c:417` | 47 |
| `LksStatus lks_order_remove(LksOrder *order, const LksOrderHandle *handle, void **out_item);` | 102 | `src/order.c:374` | 43 |
| `size_t lks_order_size(const LksOrder *order);` | 82 | `src/order.c:269` | 38 |
| `LksStatus lks_order_snapshot_capture(LksOrder *order, const LksSnapshotOptions *options, LksSnapshot **out_snapshot);` | 205 | `src/snapshot.c:167` | 54 |
| `int lks_order_snapshot_is_current(const LksOrder *order, const LksSnapshot *snapshot);` | 211 | `src/snapshot.c:187` | 65 |
| `LksStatus lks_snapshot_association(const LksSnapshot *snapshot, size_t row, const void **out_data, size_t *out_size);` | 221 | `src/snapshot.c:200` | 97 |
| `size_t lks_snapshot_count(const LksSnapshot *snapshot);` | 218 | `src/snapshot.c:192` | 95 |
| `LksStatus lks_snapshot_deserialize(const void *buffer, size_t buffer_size, LksSnapshot **out_snapshot);` | 235 | `src/snapshot_wire.c:42` | 104 |
| `void lks_snapshot_destroy(LksSnapshot *snapshot);` | 217 | `src/snapshot.c:65` | 94 |
| `LksStatus lks_snapshot_key_compare(const char *left, const char *right, int *out_order);` | 228 | `src/snapshot_key.c:44` | 101 |
| `LksStatus lks_snapshot_key_format(const LksSnapshot *snapshot, size_t row, char *buffer, size_t buffer_size);` | 224 | `src/snapshot_key.c:8` | 99 |
| `size_t lks_snapshot_key_length(const LksSnapshot *snapshot);` | 223 | `src/snapshot_key.c:6` | 98 |
| `LksStatus lks_snapshot_key_validate(const char *key);` | 226 | `src/snapshot_key.c:42` | 100 |
| `LksStatus lks_snapshot_namespace(const LksSnapshot *snapshot, const void **out_data, size_t *out_size);` | 219 | `src/snapshot.c:193` | 96 |
| `LksStatus lks_snapshot_restore_order(const LksSnapshot *snapshot, LksSnapshotResolveFn resolve, void *context, LksOrder **out_order);` | 242 | `src/snapshot_restore.c:5` | 105 |
| `LksStatus lks_snapshot_serialize(const LksSnapshot *snapshot, void *buffer, size_t buffer_size);` | 233 | `src/snapshot_wire.c:26` | 103 |
| `size_t lks_snapshot_serialized_size(const LksSnapshot *snapshot);` | 229 | `src/snapshot_wire.c:24` | 102 |
| `LksStatus lks_sort(void **items, size_t count, LksCompareFn compare, void *context);` | 58 | `src/sort.c:56` | 30 |
| `const char *lks_status_string(LksStatus status);` | 52 | `src/lks_base.c:3` | 29 |

## Frozen type definitions

| Type | Header declaration |
| --- | --- |
| `LksComparator` | `typedef struct LksComparator { LksCompareFn compare; void *context; } LksComparator;` |
| `LksCompareFn` | `typedef int (*LksCompareFn)( const void *left, const void *right, void *context );` |
| `LksGroup` | `typedef struct LksGroup LksGroup;` |
| `LksGroupBatch` | `typedef struct LksGroupBatch LksGroupBatch;` |
| `LksManagedOrder` | `typedef struct LksManagedOrder LksManagedOrder;` |
| `LksOrder` | `typedef struct LksOrder LksOrder;` |
| `LksOrderCursor` | `typedef struct LksOrderCursor LksOrderCursor;` |
| `LksOrderHandle` | `typedef struct LksOrderHandle LksOrderHandle;` |
| `LksSnapshot` | `typedef struct LksSnapshot LksSnapshot;` |
| `LksSnapshotAssociationFn` | `typedef LksStatus (*LksSnapshotAssociationFn)(void *item, uint64_t ordinal, void *context, const void **out_data, size_t *out_size);` |
| `LksSnapshotOptions` | `typedef struct LksSnapshotOptions { const void *namespace_data; size_t namespace_size; LksSnapshotAssociationFn association; void *context; } LksSnapshotOptions;` |
| `LksSnapshotResolveFn` | `typedef LksStatus (*LksSnapshotResolveFn)(const void *association, size_t association_size, uint64_t ordinal, void *context, void **out_item);` |
| `LksStatus` | `typedef enum LksStatus { LKS_STATUS_OK = 0, LKS_STATUS_INVALID_ARGUMENT = 1, LKS_STATUS_OUT_OF_MEMORY = 2, LKS_STATUS_BUFFER_TOO_SMALL = 3, LKS_STATUS_NOT_FOUND = 5, LKS_STATUS_INTERNAL_ERROR = 7, LKS_STATUS_ALREADY_EXISTS = 8, LKS_STATUS_CAPACITY_LIMIT = 9, LKS_STATUS_INVALIDATED = 10, LKS_STATUS_REENTRANT = 11, LKS_STATUS_DOMAIN_MISMATCH = 12 } LksStatus;` |
| `LksV3Lk1ImportEntry` | `typedef struct LksV3Lk1ImportEntry { const char *key; void *item; } LksV3Lk1ImportEntry;` |
