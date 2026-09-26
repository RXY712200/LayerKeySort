#ifndef LKS_GROUP_INTERNAL_H
#define LKS_GROUP_INTERNAL_H

#include "layerkeysort.h"

const LksTree *lks_group_internal_tree(const LksGroup *group);

/*
 * Consumes *inout_base on every runtime failure after argument validation.
 * On success, preserves the same exclusively owned LksGroup object and adds
 * incoming items without modifying incoming. This is not for borrowed Batch
 * Groups; callers must provide exclusive ownership.
 */
LksStatus lks_group_merge_into_owned_base(
    LksGroup **inout_base,
    const LksGroup *incoming,
    const LksComparator *comparator
);

#endif /* LKS_GROUP_INTERNAL_H */
