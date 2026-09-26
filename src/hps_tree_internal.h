#ifndef HPS_TREE_INTERNAL_H
#define HPS_TREE_INTERNAL_H

#include <stddef.h>
#include "hps.h"

typedef struct HpsTreeInternalDegreeCapacity {
    size_t node_count;
    size_t capacity_sum;
    size_t minimum_capacity;
    size_t maximum_capacity;
    size_t unused_slots_total;
} HpsTreeInternalDegreeCapacity;

typedef struct HpsTreeInternalProfile {
    size_t real_node_count;
    size_t total_child_count;
    size_t total_child_capacity;
    /* Counts allocated ChildBlocks, including the virtual root block. */
    size_t allocated_child_array_count;
    size_t real_nodes_with_child_array_count;
    size_t leaf_count;
    size_t unary_count;
    size_t branching_count;
    size_t root_child_count;
    size_t root_child_capacity;
    size_t max_real_child_capacity;
    size_t degree_bucket_counts[11];
    HpsTreeInternalDegreeCapacity degree_capacity[8];
} HpsTreeInternalProfile;

/* Read-only implementation details for tests and diagnostics. */
HpsStatus hps_tree_internal_profile(
    const HpsTree *tree,
    HpsTreeInternalProfile *out_profile
);

/* Dense histogram indexed by the actual child_capacity value.  Query with
 * counts == NULL to obtain required_count, then pass a zeroed caller buffer. */
HpsStatus hps_tree_internal_capacity_histogram(
    const HpsTree *tree,
    size_t *counts,
    size_t counts_capacity,
    size_t *out_required_count
);

size_t hps_tree_internal_sizeof_tree(void);
size_t hps_tree_internal_alignof_tree(void);
size_t hps_tree_internal_sizeof_node(void);
size_t hps_tree_internal_alignof_node(void);
size_t hps_tree_internal_child_block_header_size(void);
size_t hps_tree_internal_alignof_child_block(void);

#endif /* HPS_TREE_INTERNAL_H */
