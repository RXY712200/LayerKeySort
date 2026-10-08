#include "layerkeysort_mini.h"
#include <stdint.h>
#include <stdlib.h>

struct LksMiniOrder {
    struct LksMiniHandle *head;
    struct LksMiniHandle *tail;
    size_t size;
};

struct LksMiniHandle {
    struct LksMiniHandle *prev;
    struct LksMiniHandle *next;
    struct LksMiniOrder *owner;
    void *item;
};

LksMiniStatus lks_mini_create(LksMiniOrder **out_order)
{
    LksMiniOrder *order;
    if (!out_order) return LKS_MINI_INVALID_ARGUMENT;
    *out_order = NULL;
    order = malloc(sizeof *order);
    if (!order) return LKS_MINI_OUT_OF_MEMORY;
    order->head = NULL;
    order->tail = NULL;
    order->size = 0;
    *out_order = order;
    return LKS_MINI_OK;
}

void lks_mini_destroy(LksMiniOrder *order)
{
    LksMiniHandle *node;
    if (!order) return;
    node = order->head;
    while (node) {
        LksMiniHandle *next = node->next;
        free(node);
        node = next;
    }
    free(order);
}

LksMiniStatus lks_mini_size(const LksMiniOrder *order, size_t *out_size)
{
    if (out_size) *out_size = 0;
    if (!order || !out_size) return LKS_MINI_INVALID_ARGUMENT;
    *out_size = order->size;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_first(const LksMiniOrder *order, LksMiniHandle **out_handle)
{
    if (out_handle) *out_handle = NULL;
    if (!order || !out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = order->head;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_last(const LksMiniOrder *order, LksMiniHandle **out_handle)
{
    if (out_handle) *out_handle = NULL;
    if (!order || !out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = order->tail;
    return LKS_MINI_OK;
}

static LksMiniStatus check_handle(const LksMiniOrder *order, const LksMiniHandle *handle)
{
    if (!order || !handle) return LKS_MINI_INVALID_ARGUMENT;
    if (handle->owner != order) return LKS_MINI_WRONG_ORDER;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_next(const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle)
{
    LksMiniStatus status;
    if (!out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = NULL;
    status = check_handle(order, handle);
    if (status != LKS_MINI_OK) return status;
    *out_handle = handle->next;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_prev(const LksMiniOrder *order, const LksMiniHandle *handle, LksMiniHandle **out_handle)
{
    LksMiniStatus status;
    if (!out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = NULL;
    status = check_handle(order, handle);
    if (status != LKS_MINI_OK) return status;
    *out_handle = handle->prev;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_item(const LksMiniOrder *order, const LksMiniHandle *handle, void **out_item)
{
    LksMiniStatus status;
    if (!out_item) return LKS_MINI_INVALID_ARGUMENT;
    *out_item = NULL;
    status = check_handle(order, handle);
    if (status != LKS_MINI_OK) return status;
    *out_item = handle->item;
    return LKS_MINI_OK;
}

/* All validation precedes this helper. No fallible work follows allocation. */
static LksMiniStatus insert_between(LksMiniOrder *order, LksMiniHandle *prev,
                                   LksMiniHandle *next, void *item,
                                   LksMiniHandle **out_handle)
{
    LksMiniHandle *node;
    if (order->size == SIZE_MAX) return LKS_MINI_CAPACITY_LIMIT;
    node = malloc(sizeof *node);
    if (!node) return LKS_MINI_OUT_OF_MEMORY;
    node->prev = prev;
    node->next = next;
    node->owner = order;
    node->item = item;
    if (prev) prev->next = node;
    else order->head = node;
    if (next) next->prev = node;
    else order->tail = node;
    ++order->size;
    *out_handle = node;
    return LKS_MINI_OK;
}

LksMiniStatus lks_mini_insert_front(LksMiniOrder *order, void *item, LksMiniHandle **out_handle)
{
    if (out_handle) *out_handle = NULL;
    if (!order || !out_handle) return LKS_MINI_INVALID_ARGUMENT;
    return insert_between(order, NULL, order->head, item, out_handle);
}

LksMiniStatus lks_mini_insert_back(LksMiniOrder *order, void *item, LksMiniHandle **out_handle)
{
    if (out_handle) *out_handle = NULL;
    if (!order || !out_handle) return LKS_MINI_INVALID_ARGUMENT;
    return insert_between(order, order->tail, NULL, item, out_handle);
}

LksMiniStatus lks_mini_insert_before(LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle)
{
    LksMiniStatus status;
    if (!out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = NULL;
    status = check_handle(order, anchor);
    if (status != LKS_MINI_OK) return status;
    return insert_between(order, anchor->prev, (LksMiniHandle *)anchor, item, out_handle);
}

LksMiniStatus lks_mini_insert_after(LksMiniOrder *order, const LksMiniHandle *anchor, void *item, LksMiniHandle **out_handle)
{
    LksMiniStatus status;
    if (!out_handle) return LKS_MINI_INVALID_ARGUMENT;
    *out_handle = NULL;
    status = check_handle(order, anchor);
    if (status != LKS_MINI_OK) return status;
    return insert_between(order, (LksMiniHandle *)anchor, anchor->next, item, out_handle);
}

LksMiniStatus lks_mini_remove(LksMiniOrder *order, LksMiniHandle *handle)
{
    LksMiniStatus status = check_handle(order, handle);
    if (status != LKS_MINI_OK) return status;
    if (handle->prev) handle->prev->next = handle->next;
    else order->head = handle->next;
    if (handle->next) handle->next->prev = handle->prev;
    else order->tail = handle->prev;
    --order->size;
    free(handle);
    return LKS_MINI_OK;
}
