#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <assert.h>
#include <string.h>

_Static_assert(LKS_ORDER_BLOCK_CAPACITY >= 8u &&
    LKS_ORDER_BLOCK_CAPACITY % 2u == 0u, "even half-full block policy required");

/* No V3 Path/index coupling: order is the implicit in-order block sequence. */
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
#define V4_COUNT(order, field) (++(order)->work.field)
static void v4_mark_index(LksOrder *order, const LksOrderBlock *block)
{
    size_t i;
    if (!block) return;
    for (i = 0; i < order->work.index_blocks; ++i)
        if (order->work.tracked[i] == block) return;
    if (i == LKS_ORDER_DIAG_TRACK_LIMIT) {
        order->work.tracking_overflow = 1;
        return;
    }
    order->work.tracked[i] = block;
    ++order->work.index_blocks;
}
static void v4_begin(LksOrder *order)
{ memset(&order->work, 0, sizeof(order->work)); }
#else
#define V4_COUNT(order, field) ((void)(order))
#define v4_mark_index(order, block) ((void)(order), (void)(block))
#define v4_begin(order) ((void)(order))
#endif

static int v4_height(const LksOrderBlock *block)
{ return block ? block->height : 0; }
static size_t v4_subtree(const LksOrderBlock *block)
{ return block ? block->subtree_blocks : 0; }
static void v4_refresh(LksOrder *order, LksOrderBlock *block)
{
    int left = v4_height(block->left), right = v4_height(block->right);
    v4_mark_index(order, block);
    block->height = 1 + (left > right ? left : right);
    block->subtree_blocks = 1 + v4_subtree(block->left) + v4_subtree(block->right);
}
static void v4_replace(LksOrder *order, LksOrderBlock *old,
    LksOrderBlock *replacement)
{
    LksOrderBlock *parent = old->parent;
    v4_mark_index(order, parent);
    v4_mark_index(order, replacement);
    if (!parent) order->root = replacement;
    else if (parent->left == old) parent->left = replacement;
    else parent->right = replacement;
    if (replacement) replacement->parent = parent;
}
static LksOrderBlock *v4_rotate_left(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *pivot = block->right;
    v4_replace(order, block, pivot);
    block->right = pivot->left;
    v4_mark_index(order, block->right);
    if (block->right) block->right->parent = block;
    pivot->left = block;
    block->parent = pivot;
    v4_refresh(order, block);
    v4_refresh(order, pivot);
    V4_COUNT(order, rotations); V4_COUNT(order, left_rotations);
    return pivot;
}
static LksOrderBlock *v4_rotate_right(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *pivot = block->left;
    v4_replace(order, block, pivot);
    block->left = pivot->right;
    v4_mark_index(order, block->left);
    if (block->left) block->left->parent = block;
    pivot->right = block;
    block->parent = pivot;
    v4_refresh(order, block);
    v4_refresh(order, pivot);
    V4_COUNT(order, rotations); V4_COUNT(order, right_rotations);
    return pivot;
}
static void v4_balance(LksOrder *order, LksOrderBlock *block)
{
    while (block) {
        int balance;
        v4_refresh(order, block);
        balance = v4_height(block->left) - v4_height(block->right);
        if (balance > 1) {
            if (v4_height(block->left->left) < v4_height(block->left->right)) {
                v4_rotate_left(order, block->left);
                V4_COUNT(order, left_right_rotations);
            }
            block = v4_rotate_right(order, block);
        } else if (balance < -1) {
            if (v4_height(block->right->right) < v4_height(block->right->left)) {
                v4_rotate_right(order, block->right);
                V4_COUNT(order, right_left_rotations);
            }
            block = v4_rotate_left(order, block);
        }
        block = block->parent;
    }
}
static LksOrderBlock *v4_minimum(LksOrderBlock *block)
{
    while (block->left) block = block->left;
    return block;
}
static LksOrderBlock *v4_new_block(void)
{
    LksOrderBlock *block = (LksOrderBlock *)lks_alloc(sizeof(*block));
    if (block) {
        memset(block, 0, sizeof(*block));
        block->height = 1; block->subtree_blocks = 1;
    }
    return block;
}
/* Called only with a prepared block; an implicit insertion has no search key. */
static void v4_link_after(LksOrder *order, LksOrderBlock *anchor,
    LksOrderBlock *block)
{
    LksOrderBlock *parent;
    if (!anchor) {
        assert(!order->root);
        order->root = order->first = order->last = block;
        parent = NULL;
    } else {
        block->previous = anchor; block->next = anchor->next;
        v4_mark_index(order, anchor); v4_mark_index(order, block->next);
        if (block->next) block->next->previous = block;
        else order->last = block;
        anchor->next = block;
        if (!anchor->right) { parent = anchor; parent->right = block; }
        else { parent = v4_minimum(anchor->right); parent->left = block; }
        block->parent = parent;
        v4_mark_index(order, parent);
    }
    v4_mark_index(order, block);
    ++order->blocks;
    v4_balance(order, parent);
}
/* Physically transplant the successor. Copying its array would break stable
 * record-to-block locations, so block identity and threaded order are retained. */
static void v4_detach(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *start;
    v4_mark_index(order, block);
    v4_mark_index(order, block->previous); v4_mark_index(order, block->next);
    if (block->previous) block->previous->next = block->next;
    else order->first = block->next;
    if (block->next) block->next->previous = block->previous;
    else order->last = block->previous;
    if (!block->left || !block->right) {
        start = block->parent;
        v4_replace(order, block, block->left ? block->left : block->right);
        if (!start) start = order->root;
    } else {
        LksOrderBlock *successor = v4_minimum(block->right);
        V4_COUNT(order, two_child_detaches);
        if (successor->parent == block) {
            v4_replace(order, block, successor);
            successor->left = block->left;
            v4_mark_index(order, successor->left);
            successor->left->parent = successor;
            start = successor;
        } else {
            start = successor->parent;
            v4_replace(order, successor, successor->right);
            v4_replace(order, block, successor);
            successor->left = block->left; successor->right = block->right;
            v4_mark_index(order, successor->left);
            v4_mark_index(order, successor->right);
            successor->left->parent = successor;
            successor->right->parent = successor;
        }
    }
    --order->blocks;
    v4_balance(order, start);
}
static void v4_locations(LksOrder *order, LksOrderBlock *block)
{
    size_t i;
    assert(block->count <= LKS_ORDER_BLOCK_CAPACITY);
    V4_COUNT(order, local_blocks);
    for (i = 0; i < block->count; ++i) {
        block->records[i]->block = block;
        block->records[i]->local = i;
        V4_COUNT(order, records_reassigned);
    }
    for (; i <= LKS_ORDER_BLOCK_CAPACITY; ++i) block->records[i] = NULL;
}
LksOrder *lks_order_create(void)
{
    LksOrder *order = (LksOrder *)lks_alloc(sizeof(*order));
    if (order) memset(order, 0, sizeof(*order));
    return order;
}
void lks_order_destroy(LksOrder *order)
{
    LksOrderBlock *block;
    if (!order || order->callback_active) return;
    block = order->first;
    while (block) {
        LksOrderBlock *next = block->next;
        size_t i;
        for (i = 0; i < block->count; ++i) lks_free(block->records[i]);
        lks_free(block); block = next;
    }
    lks_source_marker_release(order->source_marker);
    lks_free(order);
}
size_t lks_order_size(const LksOrder *order)
{ return order && !order->callback_active ? order->count : 0; }

/* Shared allocation-free placement for new records and existing moved records.
 * Caller prepared SPARE iff BLOCK is empty/full; no record allocation here. */
static void v4_place(LksOrder *order, LksOrderBlock *block, size_t at,
    LksOrderHandle *record, LksOrderBlock *spare)
{
    size_t i;
    if (!block) { block = spare; v4_link_after(order, NULL, block); spare = NULL; }
    assert(at <= block->count);
    for (i = block->count; i > at; --i) block->records[i] = block->records[i-1];
    block->records[at] = record; ++block->count;
    if (spare) {
        size_t left = block->count / 2;
        spare->count = block->count - left;
        memcpy(spare->records, block->records + left,
            spare->count * sizeof(*spare->records));
        block->count = left;
        assert(block->count >= LKS_ORDER_BLOCK_MIN && spare->count >= LKS_ORDER_BLOCK_MIN);
        v4_locations(order, block); v4_locations(order, spare);
        v4_link_after(order, block, spare);
        V4_COUNT(order, splits);
    } else v4_locations(order, block);
}

static LksStatus v4_insert(LksOrder *order, LksOrderBlock *block, size_t at,
    void *item, const LksOrderHandle **out_handle)
{
    LksOrderHandle *record;
    LksOrderBlock *spare = NULL;
    if (out_handle) *out_handle = NULL;
    if (!order || !item || !out_handle) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    v4_begin(order);
    if (order->count == SIZE_MAX || (uintmax_t)order->count >= UINT64_MAX ||
        order->revision == UINT64_MAX) return LKS_STATUS_CAPACITY_LIMIT;
    record = (LksOrderHandle *)lks_alloc(sizeof(*record));
    if (!record) return LKS_STATUS_OUT_OF_MEMORY;
    if (!block || block->count == LKS_ORDER_BLOCK_CAPACITY) {
        spare = v4_new_block();
        if (!spare) { lks_free(record); return LKS_STATUS_OUT_OF_MEMORY; }
    }
    record->owner = order; record->item = item;
    /* Preparation has finished. No recoverable work/callback/allocation follows. */
    v4_place(order, block, at, record, spare);
    ++order->count; ++order->revision;
    *out_handle = record;
    return LKS_STATUS_OK;
}
LksStatus lks_order_insert_front(LksOrder *order, void *item,
    const LksOrderHandle **out_handle)
{ return v4_insert(order, order ? order->first : NULL, 0, item, out_handle); }
LksStatus lks_order_insert_back(LksOrder *order, void *item,
    const LksOrderHandle **out_handle)
{
    LksOrderBlock *last = order ? order->last : NULL;
    return v4_insert(order, last, last ? last->count : 0, item, out_handle);
}
static LksStatus v4_insert_relative(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle, int after)
{
    if (out_handle) *out_handle = NULL;
    if (!order || !anchor || anchor->owner != order) return LKS_STATUS_INVALID_ARGUMENT;
    return v4_insert(order, anchor->block, anchor->local + (size_t)after, item, out_handle);
}
LksStatus lks_order_insert_before(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle)
{ return v4_insert_relative(order, anchor, item, out_handle, 0); }
LksStatus lks_order_insert_after(LksOrder *order, const LksOrderHandle *anchor,
    void *item, const LksOrderHandle **out_handle)
{ return v4_insert_relative(order, anchor, item, out_handle, 1); }

/* Only the just-deleted source can underflow. A pair total <= B merges; otherwise
 * balanced halves each contain >= B/2. No second repair/maintenance queue exists. */
static void v4_repair(LksOrder *order, LksOrderBlock *block)
{
    LksOrderBlock *left = block->next ? block : block->previous;
    LksOrderBlock *right = left->next;
    LksOrderHandle *records[2u * LKS_ORDER_BLOCK_CAPACITY];
    size_t total = left->count + right->count;
    assert(block->count == LKS_ORDER_BLOCK_MIN - 1u &&
        total >= 2u * LKS_ORDER_BLOCK_MIN - 1u &&
        total <= LKS_ORDER_BLOCK_CAPACITY + LKS_ORDER_BLOCK_MIN - 1u);
    memcpy(records, left->records, left->count * sizeof(*records));
    memcpy(records + left->count, right->records, right->count * sizeof(*records));
    if (total <= LKS_ORDER_BLOCK_CAPACITY) {
        memcpy(left->records, records, total * sizeof(*records));
        left->count = total;
        v4_locations(order, left);
        /* Count both participating arrays even though the right is retired. */
        V4_COUNT(order, local_blocks); V4_COUNT(order, merges);
        v4_detach(order, right);
        lks_free(right);
    } else {
        left->count = total / 2; right->count = total - left->count;
        memcpy(left->records, records, left->count * sizeof(*records));
        memcpy(right->records, records + left->count, right->count * sizeof(*records));
        v4_locations(order, left); v4_locations(order, right);
        V4_COUNT(order, redistributions);
    }
}
LksStatus lks_order_remove(LksOrder *order, const LksOrderHandle *handle,
    void **out_item)
{
    LksOrderBlock *block;
    LksOrderHandle *record;
    size_t i;
    void *item;
    if (out_item) *out_item = NULL;
    if (!order || !handle || handle->owner != order) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    v4_begin(order);
    if (order->revision == UINT64_MAX) return LKS_STATUS_CAPACITY_LIMIT;
    block = handle->block; record = block->records[handle->local]; item = record->item;
    for (i = handle->local; i + 1 < block->count; ++i)
        block->records[i] = block->records[i+1];
    --block->count; --order->count;
    if (!block->count && order->blocks == 1) {
        V4_COUNT(order, local_blocks);
        v4_detach(order, block); lks_free(block);
    } else if (order->blocks > 1 && block->count < LKS_ORDER_BLOCK_MIN)
        v4_repair(order, block);
    else v4_locations(order, block);
    ++order->revision;
    lks_free(record);
    if (out_item) *out_item = item;
    return LKS_STATUS_OK;
}
const LksOrderHandle *lks_order_first(const LksOrder *order)
{ return order && !order->callback_active && order->first ? order->first->records[0] : NULL; }
const LksOrderHandle *lks_order_last(const LksOrder *order)
{
    return order && !order->callback_active && order->last ? order->last->records[order->last->count-1] : NULL;
}
const LksOrderHandle *lks_order_next(const LksOrderHandle *handle)
{
    if (!handle || handle->owner->callback_active) return NULL;
    if (handle->local + 1 < handle->block->count)
        return handle->block->records[handle->local+1];
    return handle->block->next ? handle->block->next->records[0] : NULL;
}
const LksOrderHandle *lks_order_previous(const LksOrderHandle *handle)
{
    const LksOrderBlock *previous;
    if (!handle || handle->owner->callback_active) return NULL;
    if (handle->local) return handle->block->records[handle->local-1];
    previous = handle->block->previous;
    return previous ? previous->records[previous->count-1] : NULL;
}
void *lks_order_item(const LksOrderHandle *handle)
{ return handle && !handle->owner->callback_active ? handle->item : NULL; }
static size_t v4_rank(const LksOrderBlock *block)
{
    size_t rank = v4_subtree(block->left);
    while (block->parent) {
        if (block->parent->right == block)
            rank += 1 + v4_subtree(block->parent->left);
        block = block->parent;
    }
    return rank;
}
LksStatus lks_order_compare(const LksOrder *order, const LksOrderHandle *left,
    const LksOrderHandle *right, int *out_order)
{
    size_t a, b;
    if (out_order) *out_order = 0;
    if (!order || !left || !right || !out_order ||
        left->owner != order || right->owner != order) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    if (left->block == right->block) { a = left->local; b = right->local; }
    else { a = v4_rank(left->block); b = v4_rank(right->block); }
    *out_order = (a > b) - (a < b);
    return LKS_STATUS_OK;
}

static LksStatus v4_move(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor, int after)
{
    LksOrderBlock *source, *destination, *spare = NULL;
    LksOrderHandle *record;
    size_t from, to, i;
    if (!order || !handle || !anchor || handle->owner != order || anchor->owner != order)
        return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    v4_begin(order);
    if (handle == anchor || (after ? lks_order_next(anchor) == handle : lks_order_next(handle) == anchor)) {
        V4_COUNT(order, noop_moves);
        return LKS_STATUS_OK;
    }
    if (order->revision == UINT64_MAX) return LKS_STATUS_CAPACITY_LIMIT;
    source = handle->block; destination = anchor->block;
    from = handle->local; record = source->records[from];
    if (source == destination) {
        to = anchor->local + (size_t)after;
        if (from < to) --to;
        if (from < to) {
            for (i = from; i < to; ++i) source->records[i] = source->records[i+1];
        } else {
            for (i = from; i > to; --i) source->records[i] = source->records[i-1];
        }
        source->records[to] = record;
        V4_COUNT(order, local_blocks);
        for (i = from < to ? from : to; i <= (from > to ? from : to); ++i) {
            source->records[i]->local = i;
            V4_COUNT(order, records_reassigned);
        }
    } else {
        /* The only fallible work precedes unlinking. Source repair deliberately
         * follows destination placement, using its final neighboring blocks. */
        if (destination->count == LKS_ORDER_BLOCK_CAPACITY) {
            spare = v4_new_block();
            if (!spare) return LKS_STATUS_OUT_OF_MEMORY;
        }
        for (i = from; i + 1 < source->count; ++i) source->records[i] = source->records[i+1];
        --source->count;
        source->records[source->count] = NULL;
        to = anchor->local + (size_t)after;
        v4_place(order, anchor->block, to, record, spare);
        if (source->count < LKS_ORDER_BLOCK_MIN) v4_repair(order, source);
        else v4_locations(order, source);
        V4_COUNT(order, cross_block_moves);
    }
    ++order->revision;
    return LKS_STATUS_OK;
}
LksStatus lks_order_move_front(LksOrder *order, const LksOrderHandle *handle)
{
    if (order && order->callback_active) return LKS_STATUS_REENTRANT;
    return v4_move(order, handle, lks_order_first(order), 0);
}
LksStatus lks_order_move_back(LksOrder *order, const LksOrderHandle *handle)
{
    if (order && order->callback_active) return LKS_STATUS_REENTRANT;
    return v4_move(order, handle, lks_order_last(order), 1);
}
LksStatus lks_order_move_before(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor)
{ return v4_move(order, handle, anchor, 0); }
LksStatus lks_order_move_after(LksOrder *order, const LksOrderHandle *handle,
    const LksOrderHandle *anchor)
{ return v4_move(order, handle, anchor, 1); }

LksStatus lks_order_cursor_create(const LksOrder *order, int reverse,
    LksOrderCursor **out_cursor)
{
    LksOrderCursor *cursor;
    if (out_cursor) *out_cursor = NULL;
    if (!order || !out_cursor || (reverse != 0 && reverse != 1)) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->callback_active) return LKS_STATUS_REENTRANT;
    cursor = (LksOrderCursor *)lks_alloc(sizeof(*cursor));
    if (!cursor) return LKS_STATUS_OUT_OF_MEMORY;
    cursor->owner = order; cursor->revision = order->revision; cursor->reverse = reverse;
    cursor->next = reverse ? lks_order_last(order) : lks_order_first(order);
    *out_cursor = cursor;
    return LKS_STATUS_OK;
}
LksStatus lks_order_cursor_next(LksOrderCursor *cursor, const LksOrderHandle **out_handle)
{
    const LksOrderHandle *next;
    if (out_handle) *out_handle = NULL;
    if (!cursor || !out_handle) return LKS_STATUS_INVALID_ARGUMENT;
    if (cursor->owner->callback_active) return LKS_STATUS_REENTRANT;
    /* Check before even inspecting a cached handle that removal may have freed. */
    if (cursor->revision != cursor->owner->revision) return LKS_STATUS_INVALIDATED;
    next = cursor->next;
    if (next) cursor->next = cursor->reverse ? lks_order_previous(next) : lks_order_next(next);
    *out_handle = next;
    return LKS_STATUS_OK;
}
void lks_order_cursor_destroy(LksOrderCursor *cursor)
{ lks_free(cursor); }

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
typedef struct V4Check {
    const LksOrder *order;
    const LksOrderBlock *previous;
    size_t budget, residents, blocks;
} V4Check;
static int v4_check_node(V4Check *check, const LksOrderBlock *block,
    const LksOrderBlock *parent, size_t depth, int *out_height, size_t *out_blocks)
{
    int lh, rh;
    size_t lb, rb, i;
    if (!block) { *out_height = 0; *out_blocks = 0; return 1; }
    if (!check->budget || depth > 2u * sizeof(size_t) * CHAR_BIT ||
        block->parent != parent) return 0;
    --check->budget;
    if (!v4_check_node(check, block->left, block, depth+1, &lh, &lb)) return 0;
    if (!block->count || block->count > LKS_ORDER_BLOCK_CAPACITY ||
        (check->order->blocks > 1 && block->count < LKS_ORDER_BLOCK_MIN) ||
        block->previous != check->previous ||
        (check->previous ? check->previous->next != block : check->order->first != block))
        return 0;
    for (i = 0; i < block->count; ++i) {
        const LksOrderHandle *record = block->records[i];
        /* A duplicated record cannot simultaneously match two locations. */
        if (!record || !record->item || record->owner != check->order ||
            record->block != block || record->local != i) return 0;
    }
    for (; i <= LKS_ORDER_BLOCK_CAPACITY; ++i) if (block->records[i]) return 0;
    if (check->residents > SIZE_MAX - block->count) return 0;
    check->residents += block->count; ++check->blocks; check->previous = block;
    if (!v4_check_node(check, block->right, block, depth+1, &rh, &rb) ||
        lh-rh > 1 || rh-lh > 1 || lb > SIZE_MAX-1 || rb > SIZE_MAX-1-lb) return 0;
    *out_height = 1 + (lh > rh ? lh : rh); *out_blocks = 1 + lb + rb;
    return block->height == *out_height && block->subtree_blocks == *out_blocks;
}
int lks_order_internal_valid(const LksOrder *order)
{
    V4Check check;
    int height;
    size_t blocks, count = 0;
    const LksOrderHandle *handle, *previous = NULL;
    if (!order) return 0;
    check.order = order; check.previous = NULL; check.budget = order->blocks;
    check.residents = check.blocks = 0;
    if (!v4_check_node(&check, order->root, NULL, 0, &height, &blocks) ||
        check.budget || blocks != order->blocks || check.blocks != order->blocks ||
        check.residents != order->count || check.previous != order->last ||
        (order->last && order->last->next) ||
        (!order->count && (order->root || order->first || order->last))) return 0;
    for (handle = lks_order_first(order); handle; handle = lks_order_next(handle)) {
        if (count >= order->count || lks_order_previous(handle) != previous) return 0;
        previous = handle; ++count;
    }
    if (count != order->count || previous != lks_order_last(order)) return 0;
    previous = NULL; count = 0;
    for (handle = lks_order_last(order); handle; handle = lks_order_previous(handle)) {
        if (count >= order->count || lks_order_next(handle) != previous) return 0;
        previous = handle; ++count;
    }
    return count == order->count && previous == lks_order_first(order);
}
#endif
