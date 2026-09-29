#ifndef LKS_POLICY_INTERNAL_H
#define LKS_POLICY_INTERNAL_H

#include "layerkeysort.h"

/* Preview policy lives here. These are deterministic, bounded tuning choices,
 * not Path encoding rules. Local windows expand 8 -> 16 -> 32 -> 64 nodes;
 * depth 4 is preferred, while depth 6 is the last direct-Path allowance. */
enum {
    LKS_POLICY_INITIAL_SLOT = 130,
    LKS_POLICY_TARGET_SPACING = 10,
    LKS_POLICY_MIN_USEFUL_SPACING = 2,
    LKS_POLICY_BULK_CHILDREN = 26,
    LKS_POLICY_BULK_FIRST_SLOT = 5,
    LKS_POLICY_BULK_STRIDE = 10,
    LKS_POLICY_PREFERRED_ONLINE_DEPTH = 4,
    LKS_POLICY_HARD_ONLINE_DEPTH = 6,
    LKS_POLICY_LOCAL_INITIAL_NODES = 8,
    LKS_POLICY_LOCAL_EXPANSION_FACTOR = 2,
    LKS_POLICY_LOCAL_MAX_NODES = 64,
    LKS_POLICY_LOCAL_MIN_DEPTH_GAIN = 1
};

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
