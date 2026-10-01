#ifndef LKS_TREE_INTERNAL_H
#define LKS_TREE_INTERNAL_H

#include <stddef.h>
#include "layerkeysort.h"

typedef struct LksTreeInternalProfile {
    size_t real_node_count;
    size_t leaf_count;
    size_t unary_count;
    size_t branching_count;
    size_t avl_height;
    size_t max_path_depth;
    int balance_valid;
} LksTreeInternalProfile;

typedef struct LksTreeRepairStats {
    size_t direct_inserts;
    size_t direct_endpoint_inserts;
    size_t direct_interior_inserts;
    size_t burst_endpoint_inserts;
    size_t attempts;
    size_t endpoint_attempts;
    size_t interior_attempts;
    size_t successes;
    size_t endpoint_successes;
    size_t interior_successes;
    size_t fallbacks; /* legacy V2 diagnostic: zero in V3 */
    size_t region_expansions;
    size_t attempted_region_nodes;
    size_t generated_relabel_paths;
    size_t max_attempted_region;
    size_t candidate_depth_sum;
    size_t candidate_depth_max;
    size_t nodes_relabelled;
    size_t max_region_nodes;
    size_t full_range_relabels;
    size_t full_range_relabelled_nodes;
    /* Retained only for frozen V2 regression/benchmark readers. Managed V3
     * insertion does no physical replacement-Tree rebuild, so these are zero. */
    size_t full_rebuilds;
    size_t full_rebuilt_nodes;
    size_t gap_limit_rebuild_attempts;
    size_t depth_limit_rebuild_attempts;
    size_t deeper_accepts;
    size_t comparator_search_steps;
    size_t rotations;
} LksTreeRepairStats;

void lks_tree_repair_stats_reset(void);
LksTreeRepairStats lks_tree_repair_stats_get(void);
/* Test-only bridge for frozen V2 regression cases. Not a V3 public operation. */
LksStatus lks_tree_internal_insert_item(LksTree *tree, void *item,
    const LksComparator *comparator, const LksTreeNode **out_node);
LksStatus lks_tree_internal_locate_item(const LksTree *tree,
    const void *item, const LksComparator *comparator,
    const LksTreeNode **out_left, const LksTreeNode **out_equal,
    const LksTreeNode **out_right);
/* Read-only diagnostic view; never hand it to a public manual mutator. */
const LksTree *lks_ordered_tree_internal_index(const LksOrderedTree *tree);
/* Diagnostic fixture only: seed a coordinate while the test itself proves
 * the bound comparator order remains valid. Absent from the public API. */
LksStatus lks_ordered_tree_test_seed_path(LksOrderedTree *tree,
    const LksPath *path, void *item);
LksStatus lks_tree_internal_profile(const LksTree *tree,
    LksTreeInternalProfile *out_profile);
/* Both arrays have COUNT entries. Successful build transfers ownership of Paths.
 * Failure leaves ownership with the caller. Items remain borrowed. */
LksStatus lks_tree_internal_build_ordered(LksPath **paths,
    void *const *items, size_t count, LksTree **out_tree);
LksStatus lks_tree_internal_fill_ordered(const LksTree *tree,
    const LksTreeNode **nodes, size_t capacity);
size_t lks_tree_internal_sizeof_tree(void);
size_t lks_tree_internal_alignof_tree(void);
size_t lks_tree_internal_sizeof_node(void);
size_t lks_tree_internal_alignof_node(void);

#endif
