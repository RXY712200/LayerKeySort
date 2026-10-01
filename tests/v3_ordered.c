#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "layerkeysort.h"
#include "../src/lks_alloc_internal.h"
#include "../src/lks_tree_internal.h"

typedef struct OrderedItem {
    int64_t key;
    size_t serial;
} OrderedItem;

static int compare_ordered(const void *a, const void *b, void *context)
{
    const OrderedItem *left = (const OrderedItem *)a;
    const OrderedItem *right = (const OrderedItem *)b;
    int sign = context == NULL ? 1 : *(const int *)context;
    return sign * ((left->key > right->key) - (left->key < right->key));
}

static int verify_ordered(const LksOrderedTree *tree, int sign)
{
    const LksTree *index = lks_ordered_tree_internal_index(tree);
    LksTreeInternalProfile profile;
    const LksTreeNode **nodes;
    size_t count = lks_ordered_tree_size(tree), i;
    int valid = 1;
    if (lks_tree_internal_profile(index, &profile) != LKS_STATUS_OK ||
        !profile.balance_valid || profile.real_node_count != count)
        return 0;
    if (count > SIZE_MAX / sizeof(*nodes)) return 0;
    nodes = (const LksTreeNode **)malloc((count ? count : 1) * sizeof(*nodes));
    if (nodes == NULL) return 0;
    if (lks_tree_internal_fill_ordered(index, nodes, count) != LKS_STATUS_OK)
        valid = 0;
    for (i = 0; i < count && valid; ++i) {
        const LksTreeNode *found = NULL;
        const LksPath *path = lks_tree_node_path(nodes[i]);
        if (lks_ordered_tree_find_path(tree, path, &found) != LKS_STATUS_OK ||
            found != nodes[i]) valid = 0;
        if (i != 0) {
            const OrderedItem *left = (const OrderedItem *)
                lks_tree_node_item(nodes[i - 1]);
            const OrderedItem *right = (const OrderedItem *)
                lks_tree_node_item(nodes[i]);
            int path_cmp = 0;
            if (lks_path_compare(lks_tree_node_path(nodes[i - 1]), path,
                    &path_cmp) != LKS_STATUS_OK || path_cmp >= 0 ||
                sign * ((left->key > right->key) -
                    (left->key < right->key)) > 0 ||
                (left->key == right->key && left->serial >= right->serial))
                valid = 0;
        }
    }
    free(nodes);
    return valid;
}

static int check_public_order(void)
{
    enum { COUNT = 4096, REMOVALS = 128 };
    OrderedItem *items = (OrderedItem *)malloc(COUNT * sizeof(*items));
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    uint32_t random = UINT32_C(0x8B7A91C3);
    size_t i;
    int valid = items != NULL && tree != NULL;
    for (i = 0; i < COUNT && valid; ++i) {
        const LksTreeNode *inserted = NULL;
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        items[i].key = (int64_t)(random % 64u);
        items[i].serial = i;
        if (lks_ordered_tree_insert(tree, &items[i], &inserted) != LKS_STATUS_OK ||
            inserted == NULL || lks_tree_node_item(inserted) != &items[i])
            valid = 0;
    }
    if (valid) valid = verify_ordered(tree, 1);
    for (i = 0; i < REMOVALS && valid; ++i) {
        const LksTreeNode *root = lks_ordered_tree_root_child_at(tree, 0);
        void *removed = NULL;
        if (root == NULL || lks_ordered_tree_remove_path(tree,
                lks_tree_node_path(root), &removed) != LKS_STATUS_OK ||
            removed == NULL) valid = 0;
    }
    if (valid) valid = verify_ordered(tree, 1);
    printf("V3 ordered public random=%u removed=%u %s\n",
        COUNT, REMOVALS, valid ? "PASS" : "FAIL");
    lks_ordered_tree_destroy(tree);
    free(items);
    return valid;
}

static int check_bound_context(void)
{
    OrderedItem items[6] = {{2,0},{1,1},{2,2},{3,3},{1,4},{3,5}};
    int descending = -1;
    LksComparator comparator = { compare_ordered, &descending };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    const LksTreeNode *left = NULL, *equal = NULL, *right = NULL;
    OrderedItem query = { 2, 99 };
    size_t i;
    int valid = tree != NULL;
    for (i = 0; i < 6 && valid; ++i)
        valid = lks_ordered_tree_insert(tree, &items[i], NULL) == LKS_STATUS_OK;
    if (valid) valid = verify_ordered(tree, -1) &&
        lks_ordered_tree_locate(tree, &query, &left, &equal, &right) ==
            LKS_STATUS_OK && equal != NULL &&
        ((OrderedItem *)lks_tree_node_item(equal))->key == 2;
    printf("V3 ordered bound context/equals %s\n", valid ? "PASS" : "FAIL");
    lks_ordered_tree_destroy(tree);
    return valid;
}

static int check_endpoint_runs(void)
{
    enum { COUNT = 512 };
    OrderedItem items[COUNT];
    LksComparator comparator = { compare_ordered, NULL };
    int mode, valid = 1;
    for (mode = 0; mode < 3 && valid; ++mode) {
        LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
        size_t i;
        if (tree == NULL) return 0;
        lks_tree_repair_stats_reset();
        for (i = 0; i < COUNT && valid; ++i) {
            items[i].key = mode == 0 ? (int64_t)i :
                mode == 1 ? 0 : (int64_t)(COUNT - i);
            items[i].serial = i;
            if (lks_ordered_tree_insert(tree, &items[i], NULL) != LKS_STATUS_OK)
                valid = 0;
        }
        if (valid) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            valid = verify_ordered(tree, 1) && stats.attempts == 0 &&
                stats.burst_endpoint_inserts >= COUNT - 65 &&
                stats.direct_endpoint_inserts >= COUNT - 1;
        }
        lks_ordered_tree_destroy(tree);
    }
    printf("V3 ordered append/equal/prepend runs=%u each %s\n", COUNT,
        valid ? "PASS" : "FAIL");
    return valid;
}

static int check_endpoint_carry_oom(void)
{
    OrderedItem items[2] = {{0, 0}, {1, 1}};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *original = lks_path_create(LKS_DIRECTION_POSITIVE, 1234u);
    size_t failure = 1, failures = 0;
    int valid = tree != NULL && original != NULL, success = 0;
    if (valid && lks_path_append(original, LKS_PATH_SLOT_MAX) != LKS_STATUS_OK)
        valid = 0;
    if (valid && lks_ordered_tree_test_seed_path(tree, original, &items[0]) !=
            LKS_STATUS_OK) valid = 0;
    while (valid && failure < 100) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL, *old = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[1], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            unsigned int slot = 0;
            success = inserted != NULL && verify_ordered(tree, 1) &&
                lks_path_depth(lks_tree_node_path(inserted)) == 1 &&
                lks_path_get_slot(lks_tree_node_path(inserted), 0, &slot) ==
                    LKS_STATUS_OK && slot == 1244u;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != 1 ||
            lks_ordered_tree_find_path(tree, original, &old) != LKS_STATUS_OK ||
            old == NULL || lks_tree_node_item(old) != &items[0] ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failure; ++failures;
    }
    printf("V3 ordered endpoint carry OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_path_destroy(original);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

/* A valid depth-nine append coordinate must not trigger a full-range relabel.
 * Fault injection also checks that failure preserves the seeded coordinate. */
static int check_deep_endpoint_direct_oom(void)
{
    OrderedItem items[2] = {{0, 0}, {1, 1}};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *original = lks_path_create(LKS_DIRECTION_POSITIVE,
        LKS_PATH_SLOT_MAX);
    size_t i, failure = 1, failures = 0;
    int valid = tree != NULL && original != NULL, success = 0;
    for (i = 1; i < 8 && valid; ++i)
        valid = lks_path_append(original, LKS_PATH_SLOT_MAX) == LKS_STATUS_OK;
    if (valid && lks_ordered_tree_test_seed_path(tree, original, &items[0]) !=
            LKS_STATUS_OK) valid = 0;
    lks_tree_repair_stats_reset();
    while (valid && failure < 100) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL, *old = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[1], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            success = inserted != NULL && verify_ordered(tree, 1) &&
                lks_path_depth(lks_tree_node_path(inserted)) == 9 &&
                stats.attempts == 0 && stats.direct_endpoint_inserts == 1;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != 1 ||
            lks_ordered_tree_find_path(tree, original, &old) != LKS_STATUS_OK ||
            old == NULL || lks_tree_node_item(old) != &items[0] ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failure; ++failures;
    }
    printf("V3 ordered depth-nine direct OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_path_destroy(original);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

static int check_endpoint_depth_limit_oom(void)
{
    OrderedItem items[2] = {{0, 0}, {1, 1}};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *original = lks_path_create(LKS_DIRECTION_POSITIVE,
        LKS_PATH_SLOT_MAX);
    size_t i, failure = 1, failures = 0;
    int valid = tree != NULL && original != NULL, success = 0;
    for (i = 1; i < 16 && valid; ++i)
        valid = lks_path_append(original, LKS_PATH_SLOT_MAX) == LKS_STATUS_OK;
    if (valid && lks_ordered_tree_test_seed_path(tree, original, &items[0]) !=
            LKS_STATUS_OK) valid = 0;
    lks_tree_repair_stats_reset();
    while (valid && failure < 100) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL, *old = NULL;
        LksStatus status;
        lks_tree_repair_stats_reset();
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[1], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            success = inserted != NULL && verify_ordered(tree, 1) &&
                stats.attempts == 1 && stats.successes == 1 &&
                stats.full_range_relabels == 1 &&
                stats.full_range_relabelled_nodes == 1;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != 1 ||
            lks_ordered_tree_find_path(tree, original, &old) != LKS_STATUS_OK ||
            old == NULL || lks_tree_node_item(old) != &items[0] ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failure; ++failures;
    }
    printf("V3 ordered depth-seventeen relabel OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_path_destroy(original);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

static int check_prepend_level_carry(void)
{
    OrderedItem items[2] = {{1, 0}, {0, 1}};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *original = lks_path_create(LKS_DIRECTION_NEGATIVE,
        LKS_PATH_SLOT_MAX);
    const LksTreeNode *inserted = NULL;
    size_t level = SIZE_MAX;
    unsigned int slot = LKS_PATH_SLOT_MAX;
    size_t failure = 1, failures = 0;
    int valid = tree != NULL && original != NULL, success = 0;
    if (valid && lks_ordered_tree_test_seed_path(tree, original, &items[0]) !=
            LKS_STATUS_OK) valid = 0;
    while (valid && failure < 100) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *old = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[1], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = inserted != NULL && verify_ordered(tree, 1) &&
                lks_path_depth(lks_tree_node_path(inserted)) == 1 &&
                lks_path_get_level(lks_tree_node_path(inserted), 0, &level) ==
                    LKS_STATUS_OK && level == 1 &&
                lks_path_get_slot(lks_tree_node_path(inserted), 0, &slot) ==
                    LKS_STATUS_OK && slot == 0;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != 1 ||
            lks_ordered_tree_find_path(tree, original, &old) != LKS_STATUS_OK ||
            old == NULL || lks_tree_node_item(old) != &items[0] ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failure; ++failures;
    }
    printf("V3 ordered prepend level carry OOM failures=%zu %s\n",
        failures, valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_path_destroy(original);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

static int check_burst_transition_oom(void)
{
    enum { EXISTING = 65 };
    OrderedItem items[EXISTING + 1];
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *last_path = NULL;
    size_t i, failure = 1, failures = 0;
    unsigned int old_slot = 0, new_slot = 0;
    int valid = tree != NULL, success = 0;
    for (i = 0; i < EXISTING && valid; ++i) {
        const LksTreeNode *node = NULL;
        items[i].key = (int64_t)i; items[i].serial = i;
        if (lks_ordered_tree_insert(tree, &items[i], &node) != LKS_STATUS_OK ||
            node == NULL) valid = 0;
        if (valid && i == EXISTING - 1)
            last_path = lks_path_clone(lks_tree_node_path(node));
    }
    items[EXISTING].key = EXISTING;
    items[EXISTING].serial = EXISTING;
    if (valid && (last_path == NULL ||
        lks_path_get_slot(last_path, 0, &old_slot) != LKS_STATUS_OK))
        valid = 0;
    lks_tree_repair_stats_reset();
    while (valid && failure < 100) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL, *old = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[EXISTING], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            success = inserted != NULL && verify_ordered(tree, 1) &&
                lks_path_get_slot(lks_tree_node_path(inserted), 0,
                    &new_slot) == LKS_STATUS_OK &&
                new_slot == old_slot + 1 &&
                stats.burst_endpoint_inserts == 1;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != EXISTING ||
            lks_ordered_tree_find_path(tree, last_path, &old) != LKS_STATUS_OK ||
            old == NULL || lks_tree_node_item(old) != &items[EXISTING - 1] ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failure; ++failures;
    }
    printf("V3 ordered burst transition OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_path_destroy(last_path);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

static int check_local_relabel_oom(void)
{
    enum { CHAIN = 7, CHILDREN = 12, SENTINEL = CHAIN + CHILDREN,
        EXISTING = SENTINEL + 1 };
    OrderedItem items[EXISTING + 1];
    LksPath *original[EXISTING] = {NULL};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    LksPath *chain = lks_path_create(LKS_DIRECTION_POSITIVE, 32768u);
    size_t i, failure = 1, failures = 0;
    int valid = tree != NULL && chain != NULL, success = 0;
    for (i = 0; i < CHAIN && valid; ++i) {
        if (i != 0 && lks_path_append(chain, 32768u) != LKS_STATUS_OK)
            valid = 0;
        items[i].key = (int64_t)i; items[i].serial = i;
        original[i] = lks_path_clone(chain);
        if (original[i] == NULL ||
            lks_ordered_tree_test_seed_path(tree, original[i], &items[i]) !=
                LKS_STATUS_OK) valid = 0;
    }
    for (i = 0; i < CHILDREN && valid; ++i) {
        size_t index = CHAIN + i;
        items[index].key = (int64_t)index; items[index].serial = index;
        original[index] = lks_path_clone(chain);
        if (original[index] == NULL ||
            lks_path_append(original[index], 65524u + (unsigned int)i) !=
                LKS_STATUS_OK ||
            lks_ordered_tree_test_seed_path(tree, original[index],
                &items[index]) != LKS_STATUS_OK) valid = 0;
    }
    items[SENTINEL].key = EXISTING;
    items[SENTINEL].serial = SENTINEL;
    original[SENTINEL] = lks_path_create(LKS_DIRECTION_POSITIVE, 32769u);
    if (valid && (original[SENTINEL] == NULL ||
        lks_ordered_tree_test_seed_path(tree, original[SENTINEL],
            &items[SENTINEL]) != LKS_STATUS_OK)) valid = 0;
    items[EXISTING].key = SENTINEL;
    items[EXISTING].serial = EXISTING;
    lks_tree_repair_stats_reset();
    while (valid && failure < 10000) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[EXISTING], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            success = inserted != NULL && verify_ordered(tree, 1) &&
                stats.successes == 1 && stats.nodes_relabelled > 0 &&
                stats.max_region_nodes > 8 && stats.full_rebuilds == 0;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != EXISTING ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        for (i = 0; i < EXISTING && valid; ++i) {
            const LksTreeNode *found = NULL;
            if (lks_ordered_tree_find_path(tree, original[i], &found) !=
                    LKS_STATUS_OK || found == NULL ||
                lks_tree_node_item(found) != &items[i]) valid = 0;
        }
        ++failures; ++failure;
    }
    printf("V3 ordered local/expanded OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    for (i = 0; i < EXISTING; ++i) lks_path_destroy(original[i]);
    lks_path_destroy(chain);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

static int check_full_range_oom(void)
{
    enum { EXISTING = 130 };
    OrderedItem items[EXISTING + 1];
    LksPath *original[EXISTING] = {NULL};
    LksComparator comparator = { compare_ordered, NULL };
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    size_t i, failure = 1, failures = 0;
    int valid = tree != NULL, success = 0;
    /* Consecutive slots at SIZE_MAX have no representable interior Path.
     * Every bounded range fails, forcing expansion past 64 and then a
     * full-range Path relabel while retaining all physical AVL nodes. */
    for (i = 0; i < EXISTING && valid; ++i) {
        items[i].key = (int64_t)(i * 2);
        items[i].serial = i;
        original[i] = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
            (unsigned int)i, SIZE_MAX);
        if (original[i] == NULL ||
            lks_ordered_tree_test_seed_path(tree, original[i], &items[i]) !=
                LKS_STATUS_OK) valid = 0;
    }
    items[EXISTING].key = 127;
    items[EXISTING].serial = EXISTING;
    if (valid) valid = verify_ordered(tree, 1);
    lks_tree_repair_stats_reset();
    while (valid && failure < 20000) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree, &items[EXISTING], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            LksTreeRepairStats stats = lks_tree_repair_stats_get();
            success = inserted != NULL && verify_ordered(tree, 1) &&
                stats.full_range_relabels == 1 &&
                stats.full_range_relabelled_nodes == EXISTING &&
                stats.max_region_nodes == EXISTING &&
                stats.region_expansions >= 5 && stats.full_rebuilds == 0;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            lks_ordered_tree_size(tree) != EXISTING ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        for (i = 0; i < EXISTING && valid; ++i) {
            const LksTreeNode *found = NULL;
            if (lks_ordered_tree_find_path(tree, original[i], &found) !=
                    LKS_STATUS_OK || found == NULL ||
                lks_tree_node_item(found) != &items[i]) valid = 0;
        }
        ++failures;
        ++failure;
    }
    printf("V3 ordered full-range/expanded OOM failures=%zu %s\n", failures,
        valid && success && failures > 0 ? "PASS" : "FAIL");
    for (i = 0; i < EXISTING; ++i) lks_path_destroy(original[i]);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures > 0;
}

int lks_run_v3_ordered_tests(void)
{
    if (lks_ordered_tree_create(NULL) != NULL) return 1;
    if (!check_public_order() || !check_bound_context() ||
        !check_endpoint_runs() || !check_endpoint_carry_oom() ||
        !check_deep_endpoint_direct_oom() ||
        !check_endpoint_depth_limit_oom() ||
        !check_prepend_level_carry() || !check_burst_transition_oom() ||
        !check_local_relabel_oom() ||
        !check_full_range_oom()) return 1;
    return 0;
}
