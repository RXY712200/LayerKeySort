#ifndef LKS_ORDER_INTERNAL_H
#define LKS_ORDER_INTERNAL_H

#include "layerkeysort.h"
#include <stdint.h>
#include <limits.h>

/* Private policy, not a coordinate format or ABI guarantee. The extra pointer
 * slot is insertion scratch; committed occupancy never exceeds capacity. */
#define LKS_ORDER_BLOCK_CAPACITY 128u
#define LKS_ORDER_BLOCK_MIN (LKS_ORDER_BLOCK_CAPACITY / 2u)
typedef struct LksOrderBlock LksOrderBlock;

struct LksOrderHandle {
    LksOrder *owner;
    LksOrderBlock *block;
    size_t local;
    void *item;
};

struct LksOrderBlock {
    LksOrderBlock *left, *right, *parent;
    LksOrderBlock *previous, *next;
    size_t subtree_blocks, count;
    int height;
    LksOrderHandle *records[LKS_ORDER_BLOCK_CAPACITY + 1u];
};

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
/* Per-operation diagnostics, deliberately absent from normal production.
 * 'index_blocks' counts distinct blocks whose index metadata/links are written,
 * not residents whose derived ranks changed. Tracking is diagnostic work only. */
#define LKS_ORDER_DIAG_TRACK_LIMIT (8u * (sizeof(size_t) * CHAR_BIT + 1u))
typedef struct LksOrderWork {
    size_t records_reassigned, local_blocks, index_blocks;
    size_t splits, redistributions, merges, rotations;
    size_t left_rotations, right_rotations;
    size_t left_right_rotations, right_left_rotations, two_child_detaches;
    size_t noop_moves, cross_block_moves;
    int tracking_overflow;
    const LksOrderBlock *tracked[LKS_ORDER_DIAG_TRACK_LIMIT];
} LksOrderWork;
#endif

struct LksOrder {
    LksOrderBlock *root, *first, *last;
    size_t count, blocks;
    uint64_t revision;
    int callback_active;
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    LksOrderWork work;
#endif
};

struct LksManagedOrder {
    LksOrder *core;
    LksComparator comparator;
    int active;
};

struct LksOrderCursor {
    const LksOrder *owner;
    const LksOrderHandle *next;
    uint64_t revision;
    int reverse;
};

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
/* No allocation. Bounded recursion rejects cycles/excessive height before deep
 * descent. Checks index in-order against threads, record uniqueness/location,
 * occupancy and both public traversal directions. Not a dangling-pointer probe. */
int lks_order_internal_valid(const LksOrder *order);
#endif
#endif
