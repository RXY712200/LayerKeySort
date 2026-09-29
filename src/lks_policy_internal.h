#ifndef LKS_POLICY_INTERNAL_H
#define LKS_POLICY_INTERNAL_H

#include "layerkeysort.h"

/* Preview policy lives here. These are tuning choices, not Path encoding rules. */
enum {
    LKS_POLICY_INITIAL_SLOT = 130,
    LKS_POLICY_TARGET_SPACING = 10,
    LKS_POLICY_MIN_USEFUL_SPACING = 2,
    LKS_POLICY_BULK_CHILDREN = 26,
    LKS_POLICY_BULK_FIRST_SLOT = 5,
    LKS_POLICY_BULK_STRIDE = 10,
    LKS_POLICY_MAX_ONLINE_DEPTH = 4
};

static unsigned int lks_policy_midpoint(unsigned int low, unsigned int high)
{
    return low + (high - low) / 2u;
}

static unsigned int lks_policy_endpoint_step(unsigned int available)
{
    return available >= LKS_POLICY_TARGET_SPACING ?
        LKS_POLICY_TARGET_SPACING : available;
}

#endif
