#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <string.h>

/* Consume the threaded sequence in order to construct a balanced index without
 * arrays of blocks, per-resident searches or rotations. Stack is O(log blocks). */
static LksOrderBlock *snap_bulk_index(LksOrderBlock **cursor, size_t count)
{
    LksOrderBlock *left, *root, *right; int a, b;
    if (!count) return NULL;
    left = snap_bulk_index(cursor, count/2);
    root = *cursor; *cursor = root->next;
    right = snap_bulk_index(cursor, count-count/2-1);
    root->left = left; root->right = right;
    if (left) left->parent = root;
    if (right) right->parent = root;
    a = left ? left->height : 0; b = right ? right->height : 0;
    root->height = 1+(a > b ? a : b); root->subtree_blocks = count;
    return root;
}
LksStatus lks_order_bulk_build(void *const *items, size_t count, LksOrder **out)
{
    LksOrder *o; LksOrderBlock *cursor; size_t blocks, base, extra, i, row = 0;
    *out = NULL;
    if ((count && !items) || count > UINT64_MAX) return LKS_STATUS_INVALID_ARGUMENT;
    o = lks_order_create(); if (!o) return LKS_STATUS_OUT_OF_MEMORY;
    blocks = count/128+(count%128 != 0);
    base = blocks ? count/blocks : 0; extra = blocks ? count%blocks : 0;
    for (i = 0; i < blocks; ++i) {
        LksOrderBlock *b = (LksOrderBlock *)lks_alloc(sizeof(*b));
        size_t j, n = base+(i < extra);
        if (!b) goto oom;
        memset(b, 0, sizeof(*b)); b->previous = o->last;
        if (o->last) o->last->next = b; else o->first = b;
        o->last = b; ++o->blocks;
        for (j = 0; j < n; ++j, ++row) {
            LksOrderHandle *h;
            if (!items[row]) { lks_order_destroy(o); return LKS_STATUS_INVALID_ARGUMENT; }
            h = (LksOrderHandle *)lks_alloc(sizeof(*h));
            if (!h) goto oom;
            h->owner = o; h->block = b; h->local = j; h->item = items[row];
            b->records[b->count++] = h; ++o->count;
        }
    }
    cursor = o->first; o->root = snap_bulk_index(&cursor, blocks);
    *out = o; return LKS_STATUS_OK;
oom:
    lks_order_destroy(o); return LKS_STATUS_OUT_OF_MEMORY;
}
