#ifndef LKS_POLICY_INTERNAL_H
#define LKS_POLICY_INTERNAL_H

#include "layerkeysort.h"
#include <stdint.h>

/* Preview policy lives here, separate from Path storage and text encoding.
 * Local windows expand 8 -> 16 -> 32 -> 64 nodes; depth 4 is preferred and
 * depth 6 is the last direct-Path allowance. Endpoint spacing remains 10 so
 * Preview.3 measures the wider slot domain before any heuristic retuning. */
enum {
    LKS_POLICY_INITIAL_SLOT = 32768,
    LKS_POLICY_TARGET_SPACING = 10,
    LKS_POLICY_MIN_USEFUL_SPACING = 2,
    LKS_POLICY_BULK_CHILDREN = 26,
    LKS_POLICY_PREFERRED_ONLINE_DEPTH = 4,
    LKS_POLICY_HARD_ONLINE_DEPTH = 6,
    LKS_POLICY_LOCAL_INITIAL_NODES = 8,
    LKS_POLICY_LOCAL_EXPANSION_FACTOR = 2,
    LKS_POLICY_LOCAL_MAX_NODES = 64,
    LKS_POLICY_LOCAL_MIN_DEPTH_GAIN = 1
};

/* Spread roots across the full 16-bit domain, leaving space at both ends;
 * the old low-end first/stride layout would waste the newly available gaps.
 * With at most 26 blocks the numerator fits uint_least32_t. */
_Static_assert(LKS_POLICY_BULK_CHILDREN <=
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
