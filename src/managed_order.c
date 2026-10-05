#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"

/* Composition only: no second index, resident wrapper or mutable core escape. */
LksManagedOrder *lks_managed_order_create(const LksComparator *comparator)
{
    LksManagedOrder *order;
    if (!comparator || !comparator->compare) return NULL;
    order = (LksManagedOrder *)lks_alloc(sizeof(*order));
    if (!order) return NULL;
    order->core = lks_order_create();
    if (!order->core) { lks_free(order); return NULL; }
    order->comparator = *comparator; order->active = 0;
    return order;
}
void lks_managed_order_destroy(LksManagedOrder *order)
{
    if (!order || order->active) return;
    lks_order_destroy(order->core); lks_free(order);
}
size_t lks_managed_order_size(const LksManagedOrder *order)
{ return order && !order->active ? lks_order_size(order->core) : 0; }

static int v4_managed_compare_item(LksManagedOrder *order, const void *item,
    const void *query)
{
    int result;
    /* Reentry into this facade/core is rejected even for reads. Other independent
     * containers are allowed. Caller callbacks must return normally (no longjmp). */
    order->core->callback_active = 1;
    result = order->comparator.compare(item, query, order->comparator.context);
    order->core->callback_active = 0;
    return result;
}

/* Find the first block whose maximum exceeds query (upper) or is >= query
 * (lower), then binary-search only that block. No successor/equal-run scan. */
static const LksOrderHandle *v4_managed_bound(LksManagedOrder *order,
    const void *query, int upper)
{
    LksOrderBlock *node = order->core->root, *candidate = NULL;
    size_t low, high;
    while (node) {
        int c = v4_managed_compare_item(order, node->records[node->count-1]->item, query);
        if (c > 0 || (!upper && c == 0)) { candidate = node; node = node->left; }
        else node = node->right;
    }
    if (!candidate) return NULL;
    low = 0; high = candidate->count;
    while (low < high) {
        size_t middle = low + (high-low)/2;
        int c = v4_managed_compare_item(order, candidate->records[middle]->item, query);
        if (c > 0 || (!upper && c == 0)) high = middle;
        else low = middle + 1;
    }
    return low < candidate->count ? candidate->records[low] : NULL;
}
LksStatus lks_managed_order_insert(LksManagedOrder *order, void *item,
    const LksOrderHandle **out_handle)
{
    const LksOrderHandle *upper;
    LksStatus status;
    if (out_handle) *out_handle = NULL;
    if (!order || !item || !out_handle) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->active) return LKS_STATUS_REENTRANT;
    order->active = 1;
    upper = v4_managed_bound(order, item, 1);
    /* All comparisons finish before the explicit core begins preparation/commit. */
    status = upper ? lks_order_insert_before(order->core, upper, item, out_handle) :
        lks_order_insert_back(order->core, item, out_handle);
    order->active = 0;
    return status;
}
LksStatus lks_managed_order_locate(LksManagedOrder *order, const void *query,
    const LksOrderHandle **out_predecessor, const LksOrderHandle **out_equal,
    const LksOrderHandle **out_successor)
{
    const LksOrderHandle *lower, *upper;
    if (out_predecessor) *out_predecessor = NULL;
    if (out_equal) *out_equal = NULL;
    if (out_successor) *out_successor = NULL;
    if (!order || !query || !out_predecessor || !out_equal || !out_successor ||
        out_predecessor == out_equal || out_predecessor == out_successor ||
        out_equal == out_successor) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->active) return LKS_STATUS_REENTRANT;
    order->active = 1;
    lower = v4_managed_bound(order, query, 0);
    upper = v4_managed_bound(order, query, 1);
    *out_predecessor = lower ? lks_order_previous(lower) : lks_order_last(order->core);
    if (lower && v4_managed_compare_item(order, lower->item, query) == 0) *out_equal = lower;
    *out_successor = upper;
    order->active = 0;
    return LKS_STATUS_OK;
}
LksStatus lks_managed_order_remove(LksManagedOrder *order,
    const LksOrderHandle *handle, void **out_item)
{
    if (out_item) *out_item = NULL;
    if (!order) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->active) return LKS_STATUS_REENTRANT;
    return lks_order_remove(order->core, handle, out_item);
}
const LksOrderHandle *lks_managed_order_first(const LksManagedOrder *order)
{ return order && !order->active ? lks_order_first(order->core) : NULL; }
const LksOrderHandle *lks_managed_order_last(const LksManagedOrder *order)
{ return order && !order->active ? lks_order_last(order->core) : NULL; }
LksStatus lks_managed_order_compare(const LksManagedOrder *order,
    const LksOrderHandle *left, const LksOrderHandle *right, int *out_order)
{
    if (out_order) *out_order = 0;
    if (!order) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->active) return LKS_STATUS_REENTRANT;
    return lks_order_compare(order->core, left, right, out_order);
}
LksStatus lks_managed_order_cursor_create(const LksManagedOrder *order, int reverse,
    LksOrderCursor **out_cursor)
{
    if (out_cursor) *out_cursor = NULL;
    if (!order) return LKS_STATUS_INVALID_ARGUMENT;
    if (order->active) return LKS_STATUS_REENTRANT;
    return lks_order_cursor_create(order->core, reverse, out_cursor);
}
