#ifndef LKS_V2_TREE_REGRESSION_BRIDGE_H
#define LKS_V2_TREE_REGRESSION_BRIDGE_H

/* Frozen V2 regression inputs still exercise shared index mechanics. These
 * names exist only in tests; V3 callers cannot mix manual and managed APIs. */
#include "../src/lks_tree_internal.h"
#define lks_tree_insert_item lks_tree_internal_insert_item
#define lks_tree_locate_item lks_tree_internal_locate_item

#endif
