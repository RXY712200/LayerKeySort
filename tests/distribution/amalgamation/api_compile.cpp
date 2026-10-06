#include "layerkeysort.h"
/* Typed volatile references compile and link every frozen declaration. */
static const char *(*volatile ref_lks_status_string)(LksStatus status) = lks_status_string;
static LksStatus (*volatile ref_lks_sort)(void **items, size_t count, LksCompareFn compare,
    void *context) = lks_sort;
static LksOrder *(*volatile ref_lks_order_create)(void) = lks_order_create;
static void (*volatile ref_lks_order_destroy)(LksOrder *order) = lks_order_destroy;
static size_t (*volatile ref_lks_order_size)(const LksOrder *order) = lks_order_size;
static LksStatus (*volatile ref_lks_order_insert_front)(LksOrder *order, void *item,
    const LksOrderHandle **out_handle) = lks_order_insert_front;
static LksStatus (*volatile ref_lks_order_insert_back)(LksOrder *order, void *item,
    const LksOrderHandle **out_handle) = lks_order_insert_back;
static LksStatus (*volatile ref_lks_order_insert_before)(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle) = lks_order_insert_before;
static LksStatus (*volatile ref_lks_order_insert_after)(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle) = lks_order_insert_after;
static LksStatus (*volatile ref_lks_order_remove)(LksOrder *order, const LksOrderHandle *handle,
    void **out_item) = lks_order_remove;
static const LksOrderHandle *(*volatile ref_lks_order_first)(const LksOrder *order) = lks_order_first;
static const LksOrderHandle *(*volatile ref_lks_order_last)(const LksOrder *order) = lks_order_last;
static const LksOrderHandle *(*volatile ref_lks_order_next)(const LksOrderHandle *handle) = lks_order_next;
static const LksOrderHandle *(*volatile ref_lks_order_previous)(const LksOrderHandle *handle) = lks_order_previous;
static void *(*volatile ref_lks_order_item)(const LksOrderHandle *handle) = lks_order_item;
static LksStatus (*volatile ref_lks_order_compare)(const LksOrder *order, const LksOrderHandle *left,
    const LksOrderHandle *right, int *out_order) = lks_order_compare;
static LksStatus (*volatile ref_lks_order_move_front)(LksOrder *order, const LksOrderHandle *handle) = lks_order_move_front;
static LksStatus (*volatile ref_lks_order_move_back)(LksOrder *order, const LksOrderHandle *handle) = lks_order_move_back;
static LksStatus (*volatile ref_lks_order_move_before)(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor) = lks_order_move_before;
static LksStatus (*volatile ref_lks_order_move_after)(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor) = lks_order_move_after;
static LksManagedOrder *(*volatile ref_lks_managed_order_create)(const LksComparator *comparator) = lks_managed_order_create;
static void (*volatile ref_lks_managed_order_destroy)(LksManagedOrder *order) = lks_managed_order_destroy;
static size_t (*volatile ref_lks_managed_order_size)(const LksManagedOrder *order) = lks_managed_order_size;
static LksStatus (*volatile ref_lks_managed_order_insert)(LksManagedOrder *order, void *item,
    const LksOrderHandle **out_handle) = lks_managed_order_insert;
static LksStatus (*volatile ref_lks_managed_order_locate)(LksManagedOrder *order, const void *query,
    const LksOrderHandle **out_predecessor, const LksOrderHandle **out_equal,
    const LksOrderHandle **out_successor) = lks_managed_order_locate;
static LksStatus (*volatile ref_lks_managed_order_remove)(LksManagedOrder *order,
    const LksOrderHandle *handle, void **out_item) = lks_managed_order_remove;
static const LksOrderHandle *(*volatile ref_lks_managed_order_first)(const LksManagedOrder *order) = lks_managed_order_first;
static const LksOrderHandle *(*volatile ref_lks_managed_order_last)(const LksManagedOrder *order) = lks_managed_order_last;
static LksStatus (*volatile ref_lks_managed_order_compare)(const LksManagedOrder *order,
    const LksOrderHandle *left, const LksOrderHandle *right, int *out_order) = lks_managed_order_compare;
static LksStatus (*volatile ref_lks_order_cursor_create)(const LksOrder *order, int reverse,
    LksOrderCursor **out_cursor) = lks_order_cursor_create;
static LksStatus (*volatile ref_lks_managed_order_cursor_create)(const LksManagedOrder *order, int reverse,
    LksOrderCursor **out_cursor) = lks_managed_order_cursor_create;
static LksStatus (*volatile ref_lks_order_cursor_next)(LksOrderCursor *cursor,
    const LksOrderHandle **out_handle) = lks_order_cursor_next;
static void (*volatile ref_lks_order_cursor_destroy)(LksOrderCursor *cursor) = lks_order_cursor_destroy;
static LksStatus (*volatile ref_lks_order_snapshot_capture)(LksOrder *order,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot) = lks_order_snapshot_capture;
static LksStatus (*volatile ref_lks_managed_order_snapshot_capture)(LksManagedOrder *order,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot) = lks_managed_order_snapshot_capture;
static int (*volatile ref_lks_order_snapshot_is_current)(const LksOrder *order, const LksSnapshot *snapshot) = lks_order_snapshot_is_current;
static int (*volatile ref_lks_managed_order_snapshot_is_current)(const LksManagedOrder *order,
    const LksSnapshot *snapshot) = lks_managed_order_snapshot_is_current;
static void (*volatile ref_lks_snapshot_destroy)(LksSnapshot *snapshot) = lks_snapshot_destroy;
static size_t (*volatile ref_lks_snapshot_count)(const LksSnapshot *snapshot) = lks_snapshot_count;
static LksStatus (*volatile ref_lks_snapshot_namespace)(const LksSnapshot *snapshot,
    const void **out_data, size_t *out_size) = lks_snapshot_namespace;
static LksStatus (*volatile ref_lks_snapshot_association)(const LksSnapshot *snapshot, size_t row,
    const void **out_data, size_t *out_size) = lks_snapshot_association;
static size_t (*volatile ref_lks_snapshot_key_length)(const LksSnapshot *snapshot) = lks_snapshot_key_length;
static LksStatus (*volatile ref_lks_snapshot_key_format)(const LksSnapshot *snapshot, size_t row,
    char *buffer, size_t buffer_size) = lks_snapshot_key_format;
static LksStatus (*volatile ref_lks_snapshot_key_validate)(const char *key) = lks_snapshot_key_validate;
static LksStatus (*volatile ref_lks_snapshot_key_compare)(const char *left, const char *right, int *out_order) = lks_snapshot_key_compare;
static size_t (*volatile ref_lks_snapshot_serialized_size)(const LksSnapshot *snapshot) = lks_snapshot_serialized_size;
static LksStatus (*volatile ref_lks_snapshot_serialize)(const LksSnapshot *snapshot,
    void *buffer, size_t buffer_size) = lks_snapshot_serialize;
static LksStatus (*volatile ref_lks_snapshot_deserialize)(const void *buffer, size_t buffer_size,
    LksSnapshot **out_snapshot) = lks_snapshot_deserialize;
static LksStatus (*volatile ref_lks_snapshot_restore_order)(const LksSnapshot *snapshot,
    LksSnapshotResolveFn resolve, void *context, LksOrder **out_order) = lks_snapshot_restore_order;
static LksStatus (*volatile ref_lks_order_import_v3_lk1)(const LksV3Lk1ImportEntry *entries,
    size_t count, LksOrder **out_order) = lks_order_import_v3_lk1;
static LksStatus (*volatile ref_lks_group_build)(void *const *items, size_t count,
    const LksComparator *comparator, LksGroup **out_group) = lks_group_build;
static void (*volatile ref_lks_group_destroy)(LksGroup *group) = lks_group_destroy;
static size_t (*volatile ref_lks_group_size)(const LksGroup *group) = lks_group_size;
static void *(*volatile ref_lks_group_item_at)(const LksGroup *group, size_t index) = lks_group_item_at;
static LksStatus (*volatile ref_lks_group_merge)(const LksGroup *base,
    const LksGroup *incoming, const LksComparator *comparator,
    LksGroup **out_group) = lks_group_merge;
static LksStatus (*volatile ref_lks_group_batch_build)(void *const *items, size_t count,
    size_t group_size, const LksComparator *comparator,
    LksGroupBatch **out_batch) = lks_group_batch_build;
static void (*volatile ref_lks_group_batch_destroy)(LksGroupBatch *batch) = lks_group_batch_destroy;
static size_t (*volatile ref_lks_group_batch_size)(const LksGroupBatch *batch) = lks_group_batch_size;
static size_t (*volatile ref_lks_group_batch_group_count)(const LksGroupBatch *batch) = lks_group_batch_group_count;
static size_t (*volatile ref_lks_group_batch_group_size)(const LksGroupBatch *batch) = lks_group_batch_group_size;
static const LksGroup *(*volatile ref_lks_group_batch_group_at)(
    const LksGroupBatch *batch, size_t index) = lks_group_batch_group_at;
static LksStatus (*volatile ref_lks_group_batch_merge_all)(const LksGroupBatch *batch,
    const LksComparator *comparator, LksGroup **out_group) = lks_group_batch_merge_all;
static LksStatus (*volatile ref_lks_group_snapshot_capture)(const LksGroup *group,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot) = lks_group_snapshot_capture;
static int (*volatile ref_lks_group_snapshot_is_current)(const LksGroup *group,
    const LksSnapshot *snapshot) = lks_group_snapshot_is_current;
int main(void)
{
    if (!ref_lks_status_string) return 1;
    if (!ref_lks_sort) return 1;
    if (!ref_lks_order_create) return 1;
    if (!ref_lks_order_destroy) return 1;
    if (!ref_lks_order_size) return 1;
    if (!ref_lks_order_insert_front) return 1;
    if (!ref_lks_order_insert_back) return 1;
    if (!ref_lks_order_insert_before) return 1;
    if (!ref_lks_order_insert_after) return 1;
    if (!ref_lks_order_remove) return 1;
    if (!ref_lks_order_first) return 1;
    if (!ref_lks_order_last) return 1;
    if (!ref_lks_order_next) return 1;
    if (!ref_lks_order_previous) return 1;
    if (!ref_lks_order_item) return 1;
    if (!ref_lks_order_compare) return 1;
    if (!ref_lks_order_move_front) return 1;
    if (!ref_lks_order_move_back) return 1;
    if (!ref_lks_order_move_before) return 1;
    if (!ref_lks_order_move_after) return 1;
    if (!ref_lks_managed_order_create) return 1;
    if (!ref_lks_managed_order_destroy) return 1;
    if (!ref_lks_managed_order_size) return 1;
    if (!ref_lks_managed_order_insert) return 1;
    if (!ref_lks_managed_order_locate) return 1;
    if (!ref_lks_managed_order_remove) return 1;
    if (!ref_lks_managed_order_first) return 1;
    if (!ref_lks_managed_order_last) return 1;
    if (!ref_lks_managed_order_compare) return 1;
    if (!ref_lks_order_cursor_create) return 1;
    if (!ref_lks_managed_order_cursor_create) return 1;
    if (!ref_lks_order_cursor_next) return 1;
    if (!ref_lks_order_cursor_destroy) return 1;
    if (!ref_lks_order_snapshot_capture) return 1;
    if (!ref_lks_managed_order_snapshot_capture) return 1;
    if (!ref_lks_order_snapshot_is_current) return 1;
    if (!ref_lks_managed_order_snapshot_is_current) return 1;
    if (!ref_lks_snapshot_destroy) return 1;
    if (!ref_lks_snapshot_count) return 1;
    if (!ref_lks_snapshot_namespace) return 1;
    if (!ref_lks_snapshot_association) return 1;
    if (!ref_lks_snapshot_key_length) return 1;
    if (!ref_lks_snapshot_key_format) return 1;
    if (!ref_lks_snapshot_key_validate) return 1;
    if (!ref_lks_snapshot_key_compare) return 1;
    if (!ref_lks_snapshot_serialized_size) return 1;
    if (!ref_lks_snapshot_serialize) return 1;
    if (!ref_lks_snapshot_deserialize) return 1;
    if (!ref_lks_snapshot_restore_order) return 1;
    if (!ref_lks_order_import_v3_lk1) return 1;
    if (!ref_lks_group_build) return 1;
    if (!ref_lks_group_destroy) return 1;
    if (!ref_lks_group_size) return 1;
    if (!ref_lks_group_item_at) return 1;
    if (!ref_lks_group_merge) return 1;
    if (!ref_lks_group_batch_build) return 1;
    if (!ref_lks_group_batch_destroy) return 1;
    if (!ref_lks_group_batch_size) return 1;
    if (!ref_lks_group_batch_group_count) return 1;
    if (!ref_lks_group_batch_group_size) return 1;
    if (!ref_lks_group_batch_group_at) return 1;
    if (!ref_lks_group_batch_merge_all) return 1;
    if (!ref_lks_group_snapshot_capture) return 1;
    if (!ref_lks_group_snapshot_is_current) return 1;
    return 0;
}
