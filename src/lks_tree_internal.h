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
    size_t attempts;
    size_t successes;
    size_t fallbacks;
    size_t region_expansions;
    size_t nodes_relabelled;
    size_t max_region_nodes;
    size_t full_rebuilds;
    size_t deeper_accepts;
    size_t comparator_search_steps;
    size_t rotations;
} LksTreeRepairStats;

void lks_tree_repair_stats_reset(void);
LksTreeRepairStats lks_tree_repair_stats_get(void);
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
