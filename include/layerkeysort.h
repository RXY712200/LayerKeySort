#ifndef LAYERKEYSORT_H
#define LAYERKEYSORT_H

#include <stddef.h>
#include <stdint.h>

/* LayerKeySort public API. LKS_VERSION_* describes the current API generation. */
#ifdef __cplusplus
extern "C" {
#endif

#define LKS_VERSION_MAJOR 4
#define LKS_VERSION_MINOR 0
#define LKS_VERSION_PATCH 0
#define LKS_VERSION_PRERELEASE ""
#define LKS_VERSION_STRING "4.0.0"

/* Operation results. Constructors returning pointers use NULL on failure. */
typedef enum LksStatus {
    LKS_STATUS_OK = 0,
    LKS_STATUS_INVALID_ARGUMENT = 1,
    LKS_STATUS_OUT_OF_MEMORY = 2,
    LKS_STATUS_BUFFER_TOO_SMALL = 3,
    LKS_STATUS_NOT_FOUND = 5,
    LKS_STATUS_INTERNAL_ERROR = 7,
    LKS_STATUS_ALREADY_EXISTS = 8,
    LKS_STATUS_CAPACITY_LIMIT = 9,
    LKS_STATUS_INVALIDATED = 10,
    LKS_STATUS_REENTRANT = 11,
    LKS_STATUS_DOMAIN_MISMATCH = 12
} LksStatus;

/* Generic item comparison: negative / zero / positive means before / equal /
 * after. LayerKeySort passes context through without interpreting it. Group and
 * merge operations are stable for comparator-equal items. */
typedef int (*LksCompareFn)(
    const void *left,
    const void *right,
    void *context
);

/* Comparator callback and caller-owned context. Managed order copies this
 * descriptor at creation, not the context it points to. Its callback/context
 * must stay callable with unchanged ordering semantics while the managed order lives.
 * Comparisons must give a consistent order for all resident items. */
typedef struct LksComparator {
    LksCompareFn compare;
    void *context;
} LksComparator;

/* Return a static, library-owned description of a status value. */
const char *lks_status_string(LksStatus status);

/* Stable-sort the caller's pointer array in place. Items remain caller-owned.
 * Equal items retain input order. On allocation failure the array is unchanged.
 * COUNT may be zero with ITEMS == NULL; COMPARE must still be non-NULL.
 * COUNT below two needs no allocation. */
LksStatus lks_sort(void **items, size_t count, LksCompareFn compare,
    void *context);

/* V4 explicit live order. Frozen V4.0 public source API. Private V3 LK1 migration preserves published
 * legacy coordinate semantics. Immutable snapshots are described
 * below and capture explicit historical order.
 *
 * ORDER owns structural storage and borrows non-NULL application item pointers.
 * Each insertion creates an occurrence; the same item may reside more than once.
 * A handle stays valid through unrelated insert/remove, split, merge, neighbor
 * redistribution and rotations. Its own removal or container destruction expires
 * ALL copies. Never pass a dangling handle: detection is not promised. Handles
 * are not Paths, IDs, serialized keys or pointer-value ordering coordinates.
 *
 * Caller serializes access against mutation/destruction. No internal locking.
 * Counts/revisions never wrap: mutation can return CAPACITY_LIMIT. Items remain
 * caller-owned and are never freed by LayerKeySort. */
typedef struct LksOrder LksOrder;
typedef struct LksOrderHandle LksOrderHandle;

/* Caller owns result; NULL on allocation failure. Destroy(NULL) is harmless.
 * Size(NULL) is zero; first/last(NULL) and getters(NULL) return NULL. */
LksOrder *lks_order_create(void);
void lks_order_destroy(LksOrder *order);
size_t lks_order_size(const LksOrder *order);

/* OUT_HANDLE required and set NULL on failure. ORDER and ITEM required; relative
 * insertion requires a live same-order anchor. Strong failure atomicity: order,
 * count and every prior handle are unchanged on any failure. Target structural
 * cost O(B + log(1+M)), B private fixed block capacity, M current block count.
 * Record and optional spare block allocation precede an allocation-free commit. */
LksStatus lks_order_insert_front(LksOrder *order, void *item,
    const LksOrderHandle **out_handle);
LksStatus lks_order_insert_back(LksOrder *order, void *item,
    const LksOrderHandle **out_handle);
LksStatus lks_order_insert_before(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle);
LksStatus lks_order_insert_after(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle);

/* Exact occurrence removal, no allocation. Optional OUT_ITEM is initialized NULL
 * on failure, receives the borrowed item on success. Only removed handle expires.
 * Surviving relative order/handles remain valid. O(B + log(1+M)) structural work.
 * Live foreign handles rejected; freed handles are invalid C lifetime usage. */
LksStatus lks_order_remove(LksOrder *order, const LksOrderHandle *handle,
    void **out_item);

/* Logical O(1) traversal, independent of physical block/index boundaries.
 * Empty order and neighbor past an endpoint return NULL. Returned handles borrow
 * the residence. Save next before removing current if iterating with removals;
 * later insertion may change a previously observed adjacency, not handle identity.
 * Use a revision-checked cursor below when mutation detection is required. */
const LksOrderHandle *lks_order_first(const LksOrder *order);
const LksOrderHandle *lks_order_last(const LksOrder *order);
const LksOrderHandle *lks_order_next(const LksOrderHandle *handle);
const LksOrderHandle *lks_order_previous(const LksOrderHandle *handle);
void *lks_order_item(const LksOrderHandle *handle);

/* OUT_ORDER required, initialized zero on failure; negative/zero/positive gives
 * residence order. All handles required, live and in ORDER. Same-block O(1),
 * cross-block O(log(1+M)); allocation-free, read-only. Never compare raw pointer
 * values to determine order. Comparator equality is not residence equality. */
LksStatus lks_order_compare(const LksOrder *order, const LksOrderHandle *left,
    const LksOrderHandle *right, int *out_order);

/* Move one live same-order residence without changing handle, item or size.
 * Self, already-adjacent and already-at-endpoint moves succeed without allocation,
 * structural work or revision increment (even at revision capacity). Actual moves
 * increment revision once. O(B + log(1+M)) structural work independent of distance;
 * same-block moves allocate nothing and shift only the affected interval.
 * Cross-block move prepares a spare only for a full destination. Strong failure
 * atomicity applies. No cross-container transfer or managed-order moves exist. */
LksStatus lks_order_move_front(LksOrder *order, const LksOrderHandle *handle);
LksStatus lks_order_move_back(LksOrder *order, const LksOrderHandle *handle);
LksStatus lks_order_move_before(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor);
LksStatus lks_order_move_after(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor);

/* Comparator-managed V4 facade; replacement for the V3 OrderedTree workflow.
 * Owns one private LksOrder exclusively, copies descriptor and borrows callback/
 * context. Comparator must define a consistent total preorder, remain callable
 * and have stable semantics throughout the facade lifetime. Resident comparator-
 * visible data must not change: remove, modify, reinsert instead.
 * Insert uses stable upper bound. Handles use the same occurrence/lifetime rules
 * as explicit order; logical next/previous/item use lks_order_* accessors above.
 * There is no public mutable core escape. Same-facade callback reentry is rejected:
 * status APIs return REENTRANT; pointer/size getters return NULL/zero; destroy is
 * ignored while busy. This guard is not synchronization. Serialize access. */
typedef struct LksManagedOrder LksManagedOrder;
/* Comparator and callback required; NULL for invalid descriptor or OOM. */
LksManagedOrder *lks_managed_order_create(const LksComparator *comparator);
void lks_managed_order_destroy(LksManagedOrder *order);
size_t lks_managed_order_size(const LksManagedOrder *order);
LksStatus lks_managed_order_insert(LksManagedOrder *order, void *item,
    const LksOrderHandle **out_handle);
/* Required, distinct outputs are set NULL before validation/search. EQUAL is the first
 * equal residence; PREDECESSOR immediately precedes that first equal (or query's
 * missing insertion position); SUCCESSOR is the first strictly greater residence.
 * Thus duplicate runs are bounded by strictly lower/greater neighbors. Missing
 * equality still returns OK with EQUAL NULL. QUERY required, need not be resident.
 * O(log(1+M)+log B) comparator calls; no allocation. */
LksStatus lks_managed_order_locate(LksManagedOrder *order, const void *query,
    const LksOrderHandle **out_predecessor, const LksOrderHandle **out_equal,
    const LksOrderHandle **out_successor);
/* Exact handle removal: no comparator calls/allocation; optional output as above. */
LksStatus lks_managed_order_remove(LksManagedOrder *order,
    const LksOrderHandle *handle, void **out_item);
const LksOrderHandle *lks_managed_order_first(const LksManagedOrder *order);
const LksOrderHandle *lks_managed_order_last(const LksManagedOrder *order);
LksStatus lks_managed_order_compare(const LksManagedOrder *order,
    const LksOrderHandle *left, const LksOrderHandle *right, int *out_order);

/* One allocated cursor shared by explicit/managed containers. Creation borrows
 * container, captures revision and starts at first (reverse==0) or last (reverse==1).
 * OUT_CURSOR required, initialized NULL on failure. Reverse must be 0 or 1.
 * Next returns successive handles, OK/NULL at end, INVALIDATED/NULL after actual
 * mutation, before touching cached handles. Failures/no-op moves/queries preserve
 * it. It MUST NOT outlive its container; destroy(NULL) is harmless. Output handle
 * follows ordinary residence lifetime, independently of cursor lifetime. */
typedef struct LksOrderCursor LksOrderCursor;
LksStatus lks_order_cursor_create(const LksOrder *order, int reverse,
    LksOrderCursor **out_cursor);
LksStatus lks_managed_order_cursor_create(const LksManagedOrder *order, int reverse,
    LksOrderCursor **out_cursor);
LksStatus lks_order_cursor_next(LksOrderCursor *cursor,
    const LksOrderHandle **out_handle);
void lks_order_cursor_destroy(LksOrderCursor *cursor);

/* Immutable historical export, separate from contextual live handles.
 * See docs/V4_FREEZE.md for frozen LS1/wire contracts.
 * Completed snapshots own copied bytes, never items or live handles. */
typedef struct LksSnapshot LksSnapshot;
typedef LksStatus (*LksSnapshotAssociationFn)(void *item, uint64_t ordinal,
    void *context, const void **out_data, size_t *out_size);
typedef struct LksSnapshotOptions {
    const void *namespace_data;
    size_t namespace_size;
    LksSnapshotAssociationFn association;
    void *context;
} LksSnapshotOptions;
/* Namespace: 1..UINT32_MAX bytes, copied. Callback runs once per occurrence;
 * returned span must remain valid through the immediate copy after return.
 * Nonzero size requires data. No callback means empty associations. Same-source
 * access is rejected while callback runs; callbacks must return normally.
 * Capture/failure do not change revision or invalidate handles/cursors.
 * Required outputs are cleared before work; caller destroys successful result. */
LksStatus lks_order_snapshot_capture(LksOrder *order,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot);
LksStatus lks_managed_order_snapshot_capture(LksManagedOrder *order,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot);
/* False on NULL, callback reentry, loaded snapshots or different source identity.
 * Same source/revision only: successful no-ops and failures remain current. */
int lks_order_snapshot_is_current(const LksOrder *order, const LksSnapshot *snapshot);
int lks_managed_order_snapshot_is_current(const LksManagedOrder *order,
    const LksSnapshot *snapshot);
/* NULL destroy is harmless; NULL count/length/size queries return zero.
 * Views are borrowed until destruction; required view outputs cleared on error.
 * Snapshot reads may run concurrently if its lifetime is externally protected. */
void lks_snapshot_destroy(LksSnapshot *snapshot);
size_t lks_snapshot_count(const LksSnapshot *snapshot);
LksStatus lks_snapshot_namespace(const LksSnapshot *snapshot,
    const void **out_data, size_t *out_size);
LksStatus lks_snapshot_association(const LksSnapshot *snapshot, size_t row,
    const void **out_data, size_t *out_size);
size_t lks_snapshot_key_length(const LksSnapshot *snapshot);
LksStatus lks_snapshot_key_format(const LksSnapshot *snapshot, size_t row,
    char *buffer, size_t buffer_size);
LksStatus lks_snapshot_key_validate(const char *key);
/* Validates both keys; domain mismatch is an error. Required output cleared. */
LksStatus lks_snapshot_key_compare(const char *left, const char *right, int *out_order);
size_t lks_snapshot_serialized_size(const LksSnapshot *snapshot);
/* Caller-buffer serialization allocates nothing; undersized output unchanged.
 * Deserialize strictly validates the whole borrowed blob before allocation.
 * Loaded snapshot has no live provenance; output NULL on any failure. */
LksStatus lks_snapshot_serialize(const LksSnapshot *snapshot,
    void *buffer, size_t buffer_size);
LksStatus lks_snapshot_deserialize(const void *buffer, size_t buffer_size,
    LksSnapshot **out_snapshot);
typedef LksStatus (*LksSnapshotResolveFn)(const void *association,
    size_t association_size, uint64_t ordinal, void *context, void **out_item);
/* Resolver required for nonempty input; OK requires non-NULL borrowed item.
 * Duplicate items allowed. Resolve first, then unpublished O(N) bulk build.
 * Caller owns successful fresh order; failure never frees resolved items. */
LksStatus lks_snapshot_restore_order(const LksSnapshot *snapshot,
    LksSnapshotResolveFn resolve, void *context, LksOrder **out_order);
typedef struct LksV3Lk1ImportEntry { const char *key; void *item; } LksV3Lk1ImportEntry;
/* Strict published LK1 parsing and Path sorting; duplicate coordinates rejected.
 * Empty import permits NULL entries. Output NULL on failure; no Paths retained. */
LksStatus lks_order_import_v3_lk1(const LksV3Lk1ImportEntry *entries,
    size_t count, LksOrder **out_order);

/* Immutable flat sequences. Their V4 semantics replace the V3 Path-based Groups.
 * Items remain borrowed; no Paths, handles or comparator/context are retained.
 * Comparator is borrowed for each operation and must order existing sources
 * compatibly on merge. Comparator-relevant item changes can violate that rule.
 * Required output cleared on failure. Empty input allows NULL items, but always
 * requires a callable comparator. All nonempty entries must be non-NULL.
 * Build preserves input array and stable equality. Merge takes Base on equality.
 * Same-source callback reentry returns REENTRANT; getters return zero/NULL and
 * destroy is ignored during callbacks. Sources remain caller-serialized during
 * merge/capture. No callback may throw/longjmp across the C operation. */
typedef struct LksGroup LksGroup;
typedef struct LksGroupBatch LksGroupBatch;
LksStatus lks_group_build(void *const *items, size_t count,
    const LksComparator *comparator, LksGroup **out_group);
void lks_group_destroy(LksGroup *group);
size_t lks_group_size(const LksGroup *group);
void *lks_group_item_at(const LksGroup *group, size_t index);
LksStatus lks_group_merge(const LksGroup *base,
    const LksGroup *incoming, const LksComparator *comparator,
    LksGroup **out_group);
/* Independently sorted chunks, preserving original chunk precedence on equality.
 * Group size > 0. Batch owns its groups; group_at borrows until batch destruction.
 * Do not destroy a borrowed chunk. merge_all creates a separately owned result. */
LksStatus lks_group_batch_build(void *const *items, size_t count,
    size_t group_size, const LksComparator *comparator,
    LksGroupBatch **out_batch);
void lks_group_batch_destroy(LksGroupBatch *batch);
size_t lks_group_batch_size(const LksGroupBatch *batch);
size_t lks_group_batch_group_count(const LksGroupBatch *batch);
size_t lks_group_batch_group_size(const LksGroupBatch *batch);
const LksGroup *lks_group_batch_group_at(
    const LksGroupBatch *batch, size_t index);
LksStatus lks_group_batch_merge_all(const LksGroupBatch *batch,
    const LksComparator *comparator, LksGroup **out_group);
/* Same immutable Snapshot/LS1/wire semantics. Logical revision always zero.
 * Capture changes only a private lazy provenance cache; caller serializes it.
 * Historical snapshots survive Group destruction; loaded snapshots never current. */
LksStatus lks_group_snapshot_capture(const LksGroup *group,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot);
int lks_group_snapshot_is_current(const LksGroup *group,
    const LksSnapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* LAYERKEYSORT_H */
