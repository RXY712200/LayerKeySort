#ifndef HPS_GROUP_INTERNAL_H
#define HPS_GROUP_INTERNAL_H

#include "hps.h"

const HpsTree *hps_group_internal_tree(const HpsGroup *group);

/*
 * Consumes *inout_base on every runtime failure after argument validation.
 * On success, preserves the same exclusively owned HpsGroup object and adds
 * incoming items without modifying incoming. This is not for borrowed Batch
 * Groups; callers must provide exclusive ownership.
 */
HpsStatus hps_group_merge_into_owned_base(
    HpsGroup **inout_base,
    const HpsGroup *incoming,
    const HpsComparator *comparator
);

#endif /* HPS_GROUP_INTERNAL_H */
