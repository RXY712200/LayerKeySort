#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"
#include "../src/lks_alloc_internal.h"
#include "../src/lks_policy_internal.h"

typedef struct V2Item { int key; size_t order; } V2Item;

static int compare_item(const void *left, const void *right, void *context)
{
    const V2Item *a = (const V2Item *)left;
    const V2Item *b = (const V2Item *)right;
    (void)context;
    return (a->key > b->key) - (a->key < b->key);
}

static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *state = x;
    return x;
}

static int clean_allocator(void)
{
    LksAllocStats stats = lks_alloc_stats_get();
    return stats.live_bytes == 0 && stats.live_blocks == 0 &&
        stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes == 0;
}

static int check_sort(void)
{
    V2Item values[] = {{2,0},{1,1},{2,2},{1,3},{2,4},{0,5}};
    void *items[] = {&values[0],&values[1],&values[2],&values[3],&values[4],&values[5]};
    void *original[6];
    static const size_t expected[] = {5,1,3,0,2,4};
    size_t i;
    memcpy(original, items, sizeof(items));
    if (lks_alloc_stats_reset() != 0) return 0;
    lks_alloc_test_fail_on_attempt(1);
    if (lks_sort(items, 6, compare_item, NULL) != LKS_STATUS_OUT_OF_MEMORY ||
        memcmp(items, original, sizeof(items)) != 0) return 0;
    lks_alloc_test_fail_on_attempt(2);
    if (lks_sort(items, 6, compare_item, NULL) != LKS_STATUS_OUT_OF_MEMORY ||
        memcmp(items, original, sizeof(items)) != 0) return 0;
    lks_alloc_test_disable_failure();
    if (lks_sort(items, 6, compare_item, NULL) != LKS_STATUS_OK) return 0;
    for (i = 0; i < 6; ++i) if (items[i] != &values[expected[i]]) return 0;
    return clean_allocator() && lks_sort(NULL, 0, compare_item, NULL) == LKS_STATUS_OK;
}

static int check_group_case(const char *name, size_t count, int distribution,
    size_t depth_limit)
{
    V2Item *values;
    void **items;
    LksGroup *group = NULL;
    LksComparator comparator = {compare_item, NULL};
    LksAllocStats stats;
    uint32_t random = UINT32_C(0xC0FFEE);
    size_t i, max_depth = 0, max_text = 0;
    int valid = 1;
    values = (V2Item *)malloc(count * sizeof(*values));
    items = (void **)malloc(count * sizeof(*items));
    if (values == NULL || items == NULL) { free(values); free(items); return 0; }
    if (lks_alloc_stats_reset() != 0) { free(values); free(items); return 0; }
    for (i = 0; i < count; ++i) {
        values[i].key = distribution == 0 ? (int)i :
            distribution == 1 ? (int)(count - i) :
            distribution == 2 ? 7 :
            distribution == 3 ? (int)(next_random(&random) % 13u) :
            distribution == 5 ? (int)(next_random(&random) % 257u) : (int)i;
        values[i].order = i;
        items[i] = &values[i];
    }
    if (distribution == 4) {
        for (i = count - 1; i > 0; --i) {
            size_t j = next_random(&random) % (i + 1);
            int tmp = values[i].key; values[i].key = values[j].key; values[j].key = tmp;
        }
    }
    if (lks_group_build(items, count, &comparator, &group) != LKS_STATUS_OK ||
        group == NULL || lks_group_size(group) != count) valid = 0;
    for (i = 0; valid && i < count; ++i) {
        const V2Item *item = (const V2Item *)lks_group_item_at(group, i);
        const LksPath *path = lks_group_path_at(group, i);
        size_t depth = lks_path_depth(path), text = lks_path_text_length(path);
        if (item == NULL || path == NULL || items[item->order] != item || text == 0)
            valid = 0;
        if (depth > max_depth) max_depth = depth;
        if (text > max_text) max_text = text;
        if (i != 0) {
            const V2Item *previous = (const V2Item *)lks_group_item_at(group, i - 1);
            int path_order = 0;
            if (previous->key > item->key ||
                (previous->key == item->key && previous->order > item->order) ||
                lks_path_compare(lks_group_path_at(group, i - 1), path,
                    &path_order) != LKS_STATUS_OK || path_order >= 0) valid = 0;
        }
    }
    stats = lks_alloc_stats_get();
    printf("V2 Group %s N=%zu MaxDepth=%zu MaxText=%zu PeakBytes=%zu\n",
        name, count, max_depth, max_text, stats.peak_live_bytes);
    if (max_depth > depth_limit) valid = 0;
    lks_group_destroy(group);
    free(items);
    free(values);
    return valid && clean_allocator();
}

static int check_merge_and_batch(void)
{
    V2Item values[] = {{0,0},{2,1},{1,2},{2,3},{0,4},{2,5}};
    void *base_items[] = {&values[0],&values[1]};
    void *incoming_items[] = {&values[2],&values[3]};
    void *batch_items[] = {&values[0],&values[1],&values[2],&values[3],&values[4],&values[5]};
    LksComparator comparator = {compare_item, NULL};
    LksGroup *base = NULL, *incoming = NULL, *result = NULL, *batch_result = NULL;
    LksGroupBatch *batch = NULL;
    LksPath *base_snapshot[2] = {NULL,NULL}, *incoming_snapshot[2] = {NULL,NULL};
    static const size_t merged_expected[] = {0,2,1,3};
    static const size_t batch_expected[] = {0,4,2,1,3,5};
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    if (lks_group_build(base_items, 2, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(incoming_items, 2, &comparator, &incoming) != LKS_STATUS_OK)
        { valid = 0; goto cleanup; }
    for (i = 0; i < 2; ++i) {
        base_snapshot[i] = lks_path_clone(lks_group_path_at(base, i));
        incoming_snapshot[i] = lks_path_clone(lks_group_path_at(incoming, i));
        if (base_snapshot[i] == NULL || incoming_snapshot[i] == NULL)
            { valid = 0; goto cleanup; }
    }
    if (lks_group_merge(base, incoming, &comparator, &result) != LKS_STATUS_OK ||
        lks_group_size(result) != 4) { valid = 0; goto cleanup; }
    for (i = 0; i < 4; ++i)
        if (lks_group_item_at(result, i) != &values[merged_expected[i]]) valid = 0;
    for (i = 0; i < 2; ++i) {
        int order = 0;
        if (lks_path_compare(base_snapshot[i], lks_group_path_at(base, i), &order)
                != LKS_STATUS_OK || order != 0 ||
            lks_path_compare(incoming_snapshot[i], lks_group_path_at(incoming, i),
                &order) != LKS_STATUS_OK || order != 0) valid = 0;
    }
    if (lks_group_batch_build(batch_items, 6, 2, &comparator, &batch) != LKS_STATUS_OK ||
        lks_group_batch_merge_all(batch, &comparator, &batch_result) != LKS_STATUS_OK ||
        lks_group_size(batch_result) != 6) { valid = 0; goto cleanup; }
    for (i = 0; i < 6; ++i)
        if (lks_group_item_at(batch_result, i) != &values[batch_expected[i]]) valid = 0;
cleanup:
    for (i = 0; i < 2; ++i) {
        lks_path_destroy(base_snapshot[i]);
        lks_path_destroy(incoming_snapshot[i]);
    }
    lks_group_destroy(base); lks_group_destroy(incoming);
    lks_group_destroy(result); lks_group_destroy(batch_result);
    lks_group_batch_destroy(batch);
    return valid && clean_allocator();
}

static int check_gap_contract(void)
{
    LksPath *negative = lks_path_create(LKS_DIRECTION_NEGATIVE, 130);
    LksPath *positive = lks_path_create(LKS_DIRECTION_POSITIVE, 130);
    LksPath *middle = NULL;
    int a = 0, b = 0;
    int valid = negative != NULL && positive != NULL &&
        lks_path_between(negative, positive, &middle) == LKS_STATUS_OK &&
        lks_path_direction(middle) == LKS_DIRECTION_ZERO &&
        lks_path_compare(negative, middle, &a) == LKS_STATUS_OK && a < 0 &&
        lks_path_compare(middle, positive, &b) == LKS_STATUS_OK && b < 0;
    lks_path_destroy(negative); lks_path_destroy(positive); lks_path_destroy(middle);
    return valid && clean_allocator();
}

static int prefix_matches_parent(const LksTreeNode *node)
{
    const LksPath *path = lks_tree_node_path(node);
    const LksTreeNode *parent = lks_tree_node_parent(node);
    size_t depth = lks_path_depth(path), i, level;
    unsigned int slot;
    LksPath *prefix;
    int comparison = 0;
    int valid;
    if (depth < 2) return parent == NULL;
    if (parent == NULL ||
        lks_path_get_slot(path, 0, &slot) != LKS_STATUS_OK ||
        lks_path_get_level(path, 0, &level) != LKS_STATUS_OK) return 0;
    prefix = lks_path_create_at_level(lks_path_direction(path), slot, level);
    if (prefix == NULL) return 0;
    for (i = 1; i + 1 < depth; ++i) {
        if (lks_path_get_slot(path, i, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(path, i, &level) != LKS_STATUS_OK ||
            lks_path_append_at_level(prefix, slot, level) != LKS_STATUS_OK)
            { lks_path_destroy(prefix); return 0; }
    }
    valid = lks_path_compare(prefix, lks_tree_node_path(parent),
        &comparison) == LKS_STATUS_OK && comparison == 0;
    lks_path_destroy(prefix);
    return valid;
}

static int check_tree_sequence(size_t count, int descending)
{
    LksTree *tree = NULL;
    V2Item *values = NULL;
    LksComparator comparator = {compare_item, NULL};
    size_t index, maximum_depth = 0;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    values = (V2Item *)malloc(count * sizeof(*values));
    if (tree == NULL || values == NULL) { valid = 0; goto cleanup; }
    for (index = 0; index < count; ++index) {
        const LksTreeNode *node = NULL;
        values[index].key = descending ? (int)(count - index) : (int)index;
        values[index].order = index;
        if (lks_tree_insert_item(tree, &values[index], &comparator, &node) !=
                LKS_STATUS_OK || node == NULL || lks_tree_node_item(node) != &values[index])
            { valid = 0; break; }
    }
    if (valid && lks_tree_size(tree) != count) valid = 0;
    if (valid) {
        /* Tree lookup and child ordering must hold after any atomic rebuild. */
        const LksTreeNode *stack[128];
        size_t child_index[128], depth = 0, visited = 0;
        const LksPath *previous = NULL;
        for (index = 0; index < lks_tree_root_child_count(tree) && valid; ++index) {
            stack[0] = lks_tree_root_child_at(tree, index);
            child_index[0] = 0;
            depth = 1;
            while (depth > 0 && valid) {
                const LksTreeNode *node = stack[depth - 1];
                if (child_index[depth - 1] == 0) {
                    const LksPath *path = lks_tree_node_path(node);
                    const LksTreeNode *found = NULL;
                    size_t path_depth = lks_path_depth(path);
                    int order = 0;
                    if (path_depth > maximum_depth) maximum_depth = path_depth;
                    if (previous != NULL &&
                        (lks_path_compare(previous, path, &order) != LKS_STATUS_OK || order >= 0))
                        valid = 0;
                    if (lks_tree_find_path(tree, path, &found) != LKS_STATUS_OK || found != node)
                        valid = 0;
                    if (!prefix_matches_parent(node) ||
                        (path_depth > 1 &&
                         (depth < 2 || lks_tree_node_parent(node) != stack[depth - 2])))
                        valid = 0;
                    previous = path;
                    ++visited;
                }
                if (child_index[depth - 1] < lks_tree_node_child_count(node)) {
                    const LksTreeNode *child = lks_tree_node_child_at(node,
                        child_index[depth - 1]++);
                    if (depth >= 128) { valid = 0; break; }
                    stack[depth] = child; child_index[depth] = 0; ++depth;
                } else --depth;
            }
        }
        if (visited != count || maximum_depth > LKS_POLICY_MAX_ONLINE_DEPTH) valid = 0;
    }
    printf("V2 Tree %s N=%zu MaxDepth=%zu\n",
        descending ? "descending" : "ascending", count, maximum_depth);
cleanup:
    lks_tree_destroy(tree);
    free(values);
    return valid && clean_allocator();
}

static int check_narrow_gap(void)
{
    V2Item values[22];
    LksTree *tree;
    LksComparator comparator = {compare_item, NULL};
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    values[0].key = 0;
    values[1].key = 1 << 20;
    for (i = 0; i < 22; ++i) {
        if (i >= 2) values[i].key = 1 << (21 - i);
        values[i].order = i;
        if (lks_tree_insert_item(tree, &values[i], &comparator, NULL) != LKS_STATUS_OK)
            { valid = 0; break; }
    }
    if (lks_tree_size(tree) != 22) valid = 0;
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_tree_rebuild_oom(void)
{
    V2Item values[256];
    LksTree *tree;
    LksComparator comparator = {compare_item, NULL};
    const LksTreeNode *last = NULL;
    size_t i, fail_index, failure_points = 0;
    int found_congestion = 0, success = 0, valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    for (i = 0; i < 256; ++i) {
        LksPath *candidate = NULL;
        values[i].key = (int)i;
        values[i].order = i;
        if (last != NULL) {
            if (lks_path_after(lks_tree_node_path(last), &candidate) != LKS_STATUS_OK)
                { valid = 0; break; }
            found_congestion = lks_path_depth(candidate) > LKS_POLICY_MAX_ONLINE_DEPTH;
            lks_path_destroy(candidate);
        }
        if (found_congestion) break;
        if (lks_tree_insert_item(tree, &values[i], &comparator, &last) != LKS_STATUS_OK)
            { valid = 0; break; }
    }
    if (!found_congestion) valid = 0;
    if (valid) {
        LksPath *snapshot = lks_path_clone(lks_tree_node_path(last));
        if (snapshot == NULL) valid = 0;
        for (fail_index = 1; valid && fail_index <= 10000; ++fail_index) {
            LksAllocStats before = lks_alloc_stats_get();
            const LksTreeNode *inserted = NULL, *found = NULL;
            LksStatus status;
            lks_alloc_test_fail_on_attempt(fail_index);
            status = lks_tree_insert_item(tree, &values[i], &comparator, &inserted);
            if (status == LKS_STATUS_OK) {
                success = inserted != NULL && lks_tree_size(tree) == i + 1;
                break;
            }
            if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
                lks_tree_size(tree) != i ||
                lks_tree_find_path(tree, snapshot, &found) != LKS_STATUS_OK ||
                found != last || lks_alloc_stats_get().live_bytes != before.live_bytes ||
                lks_alloc_stats_get().live_blocks != before.live_blocks) valid = 0;
            ++failure_points;
        }
        lks_alloc_test_disable_failure();
        lks_path_destroy(snapshot);
    }
    printf("V2 Tree rebuild OOM N=%zu FailPoints=%zu Success=%d\n",
        i, failure_points, success);
    lks_tree_destroy(tree);
    return valid && success && failure_points > 0 && clean_allocator();
}

int lks_run_v2_preview_tests(void)
{
    if (!check_sort() || !check_gap_contract() || !check_merge_and_batch() ||
        !check_group_case("ascending", 1024, 0, 4) ||
        !check_group_case("descending", 1024, 1, 4) ||
        !check_group_case("all-equal", 10000, 2, 4) ||
        !check_group_case("heavy-duplicates", 1024, 3, 4) ||
        !check_group_case("random-duplicates", 1024, 5, 4) ||
        !check_group_case("random-unique", 1024, 4, 4) ||
        !check_tree_sequence(1024, 0) || !check_tree_sequence(256, 1) ||
        !check_narrow_gap() || !check_tree_rebuild_oom()) {
        puts("LayerKeySort v2 preview test FAILED");
        return 1;
    }
    puts("LayerKeySort v2 preview focused tests PASS");
    return 0;
}
