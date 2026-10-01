#ifndef LKS_POLICY_INTERNAL_H
#define LKS_POLICY_INTERNAL_H

#include "layerkeysort.h"
#include <stdint.h>

/* Provisional coordinate-placement policy, separate from Path storage and
 * text encoding. Bulk blocks are logical Path subdivisions, not Tree child
 * arrays. V3 relabel windows grow geometrically to the full logical range.
 * Depth 6 is preferred. At open ends, allow direct keys through depth 16
 * before trying adaptive relabel. This bounds ordinary endpoint Path growth
 * while deferring the measured depth-9 full-range relabel cliff. Neither
 * value limits Path validity. */
enum {
    LKS_POLICY_INITIAL_SLOT = 32768,
    LKS_POLICY_TARGET_SPACING = 10,
    LKS_POLICY_MIN_MIDPOINT_SPAN = 2,
    LKS_POLICY_BULK_BLOCK_LIMIT = 26,
    LKS_POLICY_ORDERED_BURST_THRESHOLD = 64,
    LKS_POLICY_ORDERED_BURST_STEP = 1,
    LKS_POLICY_PREFERRED_ONLINE_DEPTH = 6,
    LKS_POLICY_OPEN_END_DIRECT_DEPTH = 16,
    LKS_POLICY_LOCAL_INITIAL_NODES = 8,
    LKS_POLICY_LOCAL_EXPANSION_FACTOR = 2,
    LKS_POLICY_STACK_REGION_NODES = 64
};

/* Spread roots across the full 16-bit domain, leaving space at both ends;
 * the old low-end first/stride layout would waste the newly available gaps.
 * With at most 26 blocks the numerator fits uint_least32_t. */
_Static_assert(LKS_POLICY_BULK_BLOCK_LIMIT <=
    UINT_LEAST32_MAX / ((uint_least32_t)LKS_PATH_SLOT_MAX + UINT32_C(1)),
    "bulk slot numerator must fit the intermediate type");
static inline unsigned int lks_policy_bulk_slot(size_t block_index, size_t blocks)
{
    uint_least32_t slot_domain_size =
        (uint_least32_t)LKS_PATH_SLOT_MAX + UINT32_C(1);
    return (unsigned int)(((uint_least32_t)(block_index + 1) * slot_domain_size) /
        (uint_least32_t)(blocks + 1));
}

static inline unsigned int lks_policy_midpoint(unsigned int low, unsigned int high)
{
    return low + (high - low) / 2u;
}

static inline unsigned int lks_policy_endpoint_step(unsigned int available)
{
    return available >= LKS_POLICY_TARGET_SPACING ?
        LKS_POLICY_TARGET_SPACING : available;
}

#endif
