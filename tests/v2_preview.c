#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"
#include "../src/lks_alloc_internal.h"
#include "../src/lks_bulk_internal.h"
#include "../src/lks_group_internal.h"
#include "../src/lks_policy_internal.h"
#include "../src/lks_slot_codec_internal.h"
#include "../src/lks_tree_internal.h"

typedef struct V2Item { int key; size_t order; } V2Item;

typedef struct WideItem { int64_t key; size_t order; } WideItem;

static int compare_wide(const void *left, const void *right, void *context)
{
    const WideItem *a = (const WideItem *)left;
    const WideItem *b = (const WideItem *)right;
    (void)context;
    return a->key < b->key ? -1 : a->key > b->key ? 1 : 0;
}

static int compare_item(const void *left, const void *right, void *context)
{
    const V2Item *a = (const V2Item *)left;
    const V2Item *b = (const V2Item *)right;
    (void)context;
    return (a->key > b->key) - (a->key < b->key);
}

static int compare_item_with_direction(const void *left, const void *right,
    void *context)
{
    int order = compare_item(left, right, NULL);
    return (*(const int *)context) * order;
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
    if (lks_sort(NULL, 0, compare_item, NULL) != LKS_STATUS_OK ||
        lks_sort(NULL, 1, compare_item, NULL) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_sort(items, 6, NULL, NULL) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_sort(NULL, 0, NULL, NULL) != LKS_STATUS_INVALID_ARGUMENT ||
        lks_sort(items, SIZE_MAX, compare_item, NULL) != LKS_STATUS_OUT_OF_MEMORY)
        return 0;
    lks_alloc_test_fail_on_attempt(1);
    if (lks_sort(items, 1, compare_item, NULL) != LKS_STATUS_OK ||
        lks_alloc_test_get_attempt_count() != 0) return 0;
    lks_alloc_test_disable_failure();
    {
        int descending = -1;
        if (lks_sort(items, 6, compare_item_with_direction, &descending) != LKS_STATUS_OK)
            return 0;
        for (i = 1; i < 6; ++i)
            if (((V2Item *)items[i - 1])->key < ((V2Item *)items[i])->key)
                return 0;
    }
    return clean_allocator();
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
    /* text_length is one Path's formatted character count, not storage. */
    printf("V2 Group %s N=%zu MaxDepth=%zu MaxSinglePathFormattedChars=%zu PeakRequestedBytes=%zu\n",
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

static int check_many_equal_merges(void)
{
    V2Item values[17];
    void *items[17];
    LksComparator comparator = {compare_item, NULL};
    LksGroup *base = NULL, *incoming = NULL, *merged = NULL, *batch_result = NULL;
    LksGroupBatch *batch = NULL;
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    for (i = 0; i < 17; ++i) {
        values[i].key = 7;
        values[i].order = i;
        items[i] = &values[i];
    }
    if (lks_group_build(items, 7, &comparator, &base) != LKS_STATUS_OK ||
        lks_group_build(items + 7, 10, &comparator, &incoming) != LKS_STATUS_OK ||
        lks_group_merge(base, incoming, &comparator, &merged) != LKS_STATUS_OK ||
        lks_group_batch_build(items, 17, 3, &comparator, &batch) != LKS_STATUS_OK ||
        lks_group_batch_merge_all(batch, &comparator, &batch_result) != LKS_STATUS_OK)
        valid = 0;
    if (valid && (lks_group_batch_group_count(batch) != 6 ||
        lks_group_size(merged) != 17 || lks_group_size(batch_result) != 17))
        valid = 0;
    for (i = 0; valid && i < 17; ++i)
        if (lks_group_item_at(merged, i) != &values[i] ||
            lks_group_item_at(batch_result, i) != &values[i]) valid = 0;
    for (i = 0; valid && i < 7; ++i)
        if (lks_group_item_at(base, i) != &values[i]) valid = 0;
    for (i = 0; valid && i < 10; ++i)
        if (lks_group_item_at(incoming, i) != &values[i + 7]) valid = 0;
    lks_group_destroy(base); lks_group_destroy(incoming);
    lks_group_destroy(merged); lks_group_destroy(batch_result);
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

static int check_zero_boundaries(void)
{
    LksPath *zero = lks_path_create_zero();
    LksPath *negative = NULL, *positive = NULL;
    LksPath *negative_to_zero = NULL, *zero_to_positive = NULL;
    LksPath *negative_to_positive = NULL;
    int valid = zero != NULL, order = 0;
    if (valid && lks_path_before(zero, &negative) != LKS_STATUS_OK) valid = 0;
    if (valid && lks_path_after(zero, &positive) != LKS_STATUS_OK) valid = 0;
    if (valid && lks_path_between(negative, zero, &negative_to_zero) != LKS_STATUS_OK)
        valid = 0;
    if (valid && lks_path_between(zero, positive, &zero_to_positive) != LKS_STATUS_OK)
        valid = 0;
    if (valid && lks_path_between(negative, positive,
            &negative_to_positive) != LKS_STATUS_OK) valid = 0;
    if (valid && (lks_path_direction(negative) != LKS_DIRECTION_NEGATIVE ||
        lks_path_direction(positive) != LKS_DIRECTION_POSITIVE ||
        lks_path_direction(negative_to_positive) != LKS_DIRECTION_ZERO ||
        lks_path_compare(negative, negative_to_zero, &order) != LKS_STATUS_OK ||
        order >= 0 ||
        lks_path_compare(negative_to_zero, zero, &order) != LKS_STATUS_OK ||
        order >= 0 ||
        lks_path_compare(zero, zero_to_positive, &order) != LKS_STATUS_OK ||
        order >= 0 ||
        lks_path_compare(zero_to_positive, positive, &order) != LKS_STATUS_OK ||
        order >= 0)) valid = 0;
    lks_path_destroy(zero); lks_path_destroy(negative);
    lks_path_destroy(positive); lks_path_destroy(negative_to_zero);
    lks_path_destroy(zero_to_positive); lks_path_destroy(negative_to_positive);
    return valid && clean_allocator();
}

static int check_explicit_tree_paths(void)
{
    V2Item values[2] = {{5,0},{15,1}};
    LksTree *tree = lks_tree_create();
    LksPath *first = lks_path_create(LKS_DIRECTION_POSITIVE, 5);
    LksPath *second = lks_path_create(LKS_DIRECTION_POSITIVE, 15);
    const LksTreeNode *first_node = NULL, *second_node = NULL, *found = NULL;
    int order = 0;
    int valid = tree != NULL && first != NULL && second != NULL;
    if (valid && lks_tree_insert(tree, first, &values[0], &first_node) != LKS_STATUS_OK)
        valid = 0;
    if (valid && (first_node == NULL || lks_tree_node_item(first_node) != &values[0]))
        valid = 0;
    if (valid && lks_tree_insert(tree, second, &values[1], &second_node) != LKS_STATUS_OK)
        valid = 0;
    if (valid && (lks_tree_size(tree) != 2 ||
        lks_tree_find_path(tree, first, &found) != LKS_STATUS_OK ||
        found == NULL || lks_tree_node_item(found) != &values[0] ||
        lks_path_compare(first, lks_tree_node_path(found), &order) != LKS_STATUS_OK ||
        order != 0 ||
        lks_tree_find_path(tree, second, &found) != LKS_STATUS_OK ||
        found != second_node ||
        lks_path_compare(second, lks_tree_node_path(second_node), &order) != LKS_STATUS_OK ||
        order != 0)) valid = 0;
    lks_tree_destroy(tree); lks_path_destroy(first); lks_path_destroy(second);
    return valid && clean_allocator();
}

static int parent_link_consistent(const LksTreeNode *node)
{
    const LksTreeNode *parent = lks_tree_node_parent(node);
    size_t i;
    if (parent == NULL) return 1;
    for (i = 0; i < lks_tree_node_child_count(parent); ++i)
        if (lks_tree_node_child_at(parent, i) == node) return 1;
    return 0;
}

typedef struct BulkAudit {
    const LksTree *tree;
    void **items;
    size_t count;
    size_t seen;
    size_t max_depth;
    const LksPath *previous;
    int valid;
} BulkAudit;

static void audit_bulk_node(BulkAudit *audit, const LksTreeNode *node,
    const LksTreeNode *expected_parent)
{
    const LksPath *path;
    const LksTreeNode *found = NULL;
    char formatted[64];
    const LksTreeNode *lower = NULL, *higher = NULL;
    size_t child, depth;
    int comparison = 0;
    if (!audit->valid || node == NULL || audit->seen >= audit->count) {
        audit->valid = 0;
        return;
    }
    path = lks_tree_node_path(node);
    depth = lks_path_depth(path);
    if (depth > audit->max_depth) audit->max_depth = depth;
    for (child = 0; child < lks_tree_node_child_count(node); ++child) {
        const LksTreeNode *candidate = lks_tree_node_child_at(node, child);
        if (candidate == NULL ||
            lks_path_compare(lks_tree_node_path(candidate), path,
                &comparison) != LKS_STATUS_OK || comparison == 0) {
            audit->valid = 0;
            return;
        }
        if (comparison < 0) {
            if (lower != NULL) { audit->valid = 0; return; }
            lower = candidate;
        } else {
            if (higher != NULL) { audit->valid = 0; return; }
            higher = candidate;
        }
    }
    if (lower != NULL) audit_bulk_node(audit, lower, node);
    if (!audit->valid) return;
    if (path == NULL || lks_path_text_length(path) >= sizeof(formatted) ||
        lks_path_format(path, formatted, sizeof(formatted)) != LKS_STATUS_OK ||
        strlen(formatted) != lks_path_text_length(path) ||
        lks_tree_node_item(node) != audit->items[audit->seen] ||
        lks_tree_node_parent(node) != expected_parent ||
        !parent_link_consistent(node) ||
        lks_tree_find_path(audit->tree, path, &found) != LKS_STATUS_OK ||
        found != node ||
        (audit->previous != NULL &&
         (lks_path_compare(audit->previous, path, &comparison) != LKS_STATUS_OK ||
          comparison >= 0))) {
        audit->valid = 0;
        return;
    }
    audit->previous = path;
    ++audit->seen;
    if (higher != NULL) audit_bulk_node(audit, higher, node);
}

static int audit_tree_order(const LksTree *tree, void **items, size_t count,
    size_t *out_max_depth)
{
    BulkAudit audit;
    size_t root;
    audit.tree = tree; audit.items = items; audit.count = count;
    audit.seen = 0; audit.max_depth = 0; audit.previous = NULL;
    audit.valid = lks_tree_size(tree) == count;
    for (root = 0; audit.valid && root < lks_tree_root_child_count(tree); ++root)
        audit_bulk_node(&audit, lks_tree_root_child_at(tree, root), NULL);
    if (out_max_depth != NULL) *out_max_depth = audit.max_depth;
    return audit.valid && audit.seen == count;
}

typedef struct TreeSnapshot {
    size_t count;
    void *items[256];
    const LksTreeNode *nodes[256];
    size_t subtree_end[256];
    char paths[256][96];
} TreeSnapshot;

static int snapshot_node(TreeSnapshot *snapshot, const LksTreeNode *node)
{
    size_t i, position = snapshot->count;
    if (position >= 256 || lks_path_format(lks_tree_node_path(node),
            snapshot->paths[position], sizeof(snapshot->paths[position])) !=
            LKS_STATUS_OK) return 0;
    snapshot->items[position] = lks_tree_node_item(node);
    snapshot->nodes[position] = node;
    ++snapshot->count;
    for (i = 0; i < lks_tree_node_child_count(node); ++i)
        if (!snapshot_node(snapshot, lks_tree_node_child_at(node, i))) return 0;
    snapshot->subtree_end[position] = snapshot->count;
    return 1;
}

static int snapshot_tree(const LksTree *tree, TreeSnapshot *snapshot)
{
    size_t i;
    memset(snapshot, 0, sizeof(*snapshot));
    for (i = 0; i < lks_tree_root_child_count(tree); ++i)
        if (!snapshot_node(snapshot, lks_tree_root_child_at(tree, i))) return 0;
    return snapshot->count == lks_tree_size(tree);
}

static int snapshots_equal(const TreeSnapshot *a, const TreeSnapshot *b)
{
    size_t i;
    if (a->count != b->count) return 0;
    for (i = 0; i < a->count; ++i)
        if (a->items[i] != b->items[i] || a->nodes[i] != b->nodes[i] ||
            a->subtree_end[i] != b->subtree_end[i] ||
            strcmp(a->paths[i], b->paths[i]) != 0) return 0;
    return 1;
}

static size_t snapshot_find_node(const TreeSnapshot *snapshot,
    const LksTreeNode *node)
{
    size_t i;
    for (i = 0; i < snapshot->count; ++i)
        if (snapshot->nodes[i] == node) return i;
    return snapshot->count;
}

/* Logical-range repair retains old nodes; AVL rotations may change their
 * physical preorder and parents. Relabeling is bounded to the chosen range. */
static int snapshot_local_repair_preserves_outside(
    const TreeSnapshot *before, const TreeSnapshot *after, size_t old_region_nodes)
{
    size_t i, changed = 0;
    if (after->count != before->count + 1 || old_region_nodes == 0) return 0;
    for (i = 0; i < before->count; ++i) {
        size_t found = snapshot_find_node(after, before->nodes[i]);
        if (found == after->count || before->items[i] != after->items[found])
            return 0;
        if (strcmp(before->paths[i], after->paths[found]) != 0) ++changed;
    }
    return changed <= old_region_nodes;
}

static int check_bulk_boundaries(void)
{
    static const size_t sizes[] = {0, 1, 2, 25, 26, 27, 675, 676, 677, 1024, 10000};
    size_t case_index;
    for (case_index = 0; case_index < sizeof(sizes) / sizeof(sizes[0]); ++case_index) {
        size_t count = sizes[case_index], i;
        V2Item *values = (V2Item *)malloc((count == 0 ? 1 : count) * sizeof(*values));
        void **items = (void **)malloc((count == 0 ? 1 : count) * sizeof(*items));
        LksTree *tree = NULL;
        BulkAudit audit;
        if (values == NULL || items == NULL || lks_alloc_stats_reset() != 0) {
            free(values); free(items); return 0;
        }
        for (i = 0; i < count; ++i) {
            values[i].key = (int)i;
            values[i].order = i;
            items[i] = &values[i];
        }
        if (lks_bulk_build_tree(count == 0 ? NULL : items, count, &tree) != LKS_STATUS_OK ||
            tree == NULL || lks_tree_size(tree) != count) {
            lks_tree_destroy(tree); free(values); free(items); return 0;
        }
        audit.tree = tree; audit.items = items; audit.count = count;
        audit.seen = 0; audit.max_depth = 0; audit.previous = NULL; audit.valid = 1;
        for (i = 0; i < lks_tree_root_child_count(tree); ++i)
            audit_bulk_node(&audit, lks_tree_root_child_at(tree, i), NULL);
        printf("V2 BulkBoundary N=%zu Seen=%zu MaxDepth=%zu Status=%s\n",
            count, audit.seen, audit.max_depth,
            audit.valid && audit.seen == count && audit.max_depth <= 4 ? "PASS" : "FAIL");
        lks_tree_destroy(tree);
        free(values); free(items);
        if (!audit.valid || audit.seen != count || audit.max_depth > 4 ||
            !clean_allocator()) return 0;
    }
    return 1;
}

static int check_bulk_to_prepend(void)
{
    V2Item values[45];
    void *initial[5], *expected[45];
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree = NULL;
    BulkAudit audit;
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    for (i = 0; i < 5; ++i) {
        values[i].key = (int)i;
        values[i].order = i;
        initial[i] = &values[i];
    }
    if (lks_bulk_build_tree(initial, 5, &tree) != LKS_STATUS_OK || tree == NULL)
        return 0;
    for (i = 0; i < 40; ++i) {
        const LksTreeNode *inserted = NULL;
        values[i + 5].key = -(int)(i + 1);
        values[i + 5].order = i + 5;
        if (lks_tree_insert_item(tree, &values[i + 5], &comparator,
                &inserted) != LKS_STATUS_OK || inserted == NULL ||
            lks_tree_node_item(inserted) != &values[i + 5]) {
            valid = 0;
            break;
        }
    }
    for (i = 0; i < 40; ++i) expected[i] = &values[44 - i];
    for (i = 0; i < 5; ++i) expected[40 + i] = &values[i];
    audit.tree = tree; audit.items = expected; audit.count = 45;
    audit.seen = 0; audit.max_depth = 0; audit.previous = NULL; audit.valid = valid;
    if (valid && lks_tree_size(tree) != 45) audit.valid = 0;
    for (i = 0; audit.valid && i < lks_tree_root_child_count(tree); ++i)
        audit_bulk_node(&audit, lks_tree_root_child_at(tree, i), NULL);
    printf("V2 BulkToPrepend N=45 Seen=%zu MaxDepth=%zu Status=%s\n",
        audit.seen, audit.max_depth,
        audit.valid && audit.seen == 45 ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return audit.valid && audit.seen == 45 && clean_allocator();
}

static int check_tree_sequence(size_t count, int descending)
{
    LksTree *tree = NULL;
    V2Item *values = NULL;
    LksComparator comparator = {compare_item, NULL};
    size_t index, maximum_depth = 0, max_text = 0, depth_sum = 0;
    size_t depth_histogram[129] = {0}, p95 = 0, p99 = 0, cumulative = 0;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    lks_tree_repair_stats_reset();
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
        const LksTreeNode **ordered = (const LksTreeNode **)malloc(
            count * sizeof(*ordered));
        const LksPath *previous = NULL;
        LksTreeInternalProfile profile;
        if (ordered == NULL ||
            lks_tree_internal_fill_ordered(tree, ordered, count) != LKS_STATUS_OK ||
            lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
            !profile.balance_valid) valid = 0;
        for (index = 0; index < count && valid; ++index) {
                const LksTreeNode *node = ordered[index];
                    const LksPath *path = lks_tree_node_path(node);
                    const LksTreeNode *found = NULL;
                    size_t path_depth = lks_path_depth(path);
                    size_t text_length = lks_path_text_length(path);
                    int order = 0;
                    if (path_depth > maximum_depth) maximum_depth = path_depth;
                    if (text_length > max_text) max_text = text_length;
                    if (path_depth < 129) {
                        ++depth_histogram[path_depth];
                        depth_sum += path_depth;
                    } else valid = 0;
                    if (previous != NULL &&
                        (lks_path_compare(previous, path, &order) != LKS_STATUS_OK || order >= 0))
                        valid = 0;
                    if (lks_tree_find_path(tree, path, &found) != LKS_STATUS_OK || found != node)
                        valid = 0;
                    if (!parent_link_consistent(node)) valid = 0;
                    previous = path;
        }
        free(ordered);
        if (maximum_depth > LKS_POLICY_HARD_ONLINE_DEPTH) valid = 0;
    }
    for (index = 0; index < 129; ++index) {
        cumulative += depth_histogram[index];
        if (p95 == 0 && cumulative >= (count * 95 + 99) / 100) p95 = index;
        if (p99 == 0 && cumulative >= (count * 99 + 99) / 100) p99 = index;
    }
    { LksTreeRepairStats stats = lks_tree_repair_stats_get();
      LksAllocStats allocations = lks_alloc_stats_get();
      printf("V2 Tree %s N=%zu MaxDepth=%zu AvgDepth=%.3f P95=%zu P99=%zu MaxText=%zu Attempts=%zu Local=%zu Fallbacks=%zu Nodes=%zu Expanded=%zu MaxRegion=%zu Full=%zu Deeper=%zu PeakBytes=%zu RequestedBytes=%zu\n",
        descending ? "descending" : "ascending", count, maximum_depth,
        count == 0 ? 0.0 : (double)depth_sum / (double)count,
        p95, p99, max_text, stats.attempts, stats.successes, stats.fallbacks,
        stats.nodes_relabelled, stats.region_expansions, stats.max_region_nodes,
        stats.full_rebuilds, stats.deeper_accepts,
        allocations.peak_live_bytes,
        allocations.total_successful_requested_bytes);
      if (stats.max_region_nodes > LKS_POLICY_LOCAL_MAX_NODES) valid = 0; }
cleanup:
    lks_tree_destroy(tree);
    free(values);
    return valid && clean_allocator();
}

static int check_narrow_gap(void)
{
    WideItem values[62];
    void *expected[62];
    LksTree *tree;
    LksComparator comparator = {compare_wide, NULL};
    LksTreeRepairStats stats;
    size_t i, max_depth = 0, max_text = 0;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    lks_tree_repair_stats_reset();
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    values[0].key = 0;
    values[1].key = INT64_C(1) << 60;
    for (i = 0; i < 62; ++i) {
        if (i >= 2) values[i].key = INT64_C(1) << (61 - i);
        values[i].order = i;
        if (lks_tree_insert_item(tree, &values[i], &comparator, NULL) != LKS_STATUS_OK)
            { valid = 0; break; }
    }
    expected[0] = &values[0];
    for (i = 1; i < 62; ++i) expected[i] = &values[62 - i];
    if (valid) valid = audit_tree_order(tree, expected, 62, &max_depth);
    if (valid) {
        TreeSnapshot snapshot;
        if (!snapshot_tree(tree, &snapshot)) valid = 0;
        else for (i = 0; i < snapshot.count; ++i) {
            size_t length = strlen(snapshot.paths[i]);
            if (length > max_text) max_text = length;
        }
    }
    stats = lks_tree_repair_stats_get();
    printf("V2 NarrowGap N=62 Attempts=%zu Local=%zu Fallbacks=%zu Expanded=%zu Full=%zu Deeper=%zu MaxDepth=%zu MaxText=%zu PeakBytes=%zu RequestedBytes=%zu Status=%s\n",
        stats.attempts, stats.successes, stats.fallbacks,
        stats.region_expansions, stats.full_rebuilds,
        stats.deeper_accepts, max_depth, max_text,
        lks_alloc_stats_get().peak_live_bytes,
        lks_alloc_stats_get().total_successful_requested_bytes,
        valid ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return valid && max_depth <=
        LKS_POLICY_HARD_ONLINE_DEPTH && clean_allocator();
}

/* Explicit tight boundary: a depth-5 branch has 12 depth-6 children, with
 * the final child at MAX. Appending must consider a depth-7 candidate and
 * rebuild a region larger than the initial eight-node window. */
static int build_congested_branch(LksTree *tree, V2Item *values,
    const LksTreeNode **out_last)
{
    LksPath *chain = lks_path_create(LKS_DIRECTION_POSITIVE,
        LKS_POLICY_INITIAL_SLOT);
    size_t i;
    int valid = chain != NULL;
    *out_last = NULL;
    for (i = 0; i < 5 && valid; ++i) {
        if (i != 0 && lks_path_append(chain, LKS_POLICY_INITIAL_SLOT) !=
                LKS_STATUS_OK) valid = 0;
        values[i].key = (int)i; values[i].order = i;
        if (valid && lks_tree_insert(tree, chain, &values[i], NULL) !=
                LKS_STATUS_OK) valid = 0;
    }
    for (i = 0; i < 12 && valid; ++i) {
        LksPath *child = lks_path_clone(chain);
        values[i + 5].key = (int)(i + 5); values[i + 5].order = i + 5;
        if (child == NULL || lks_path_append(child,
                LKS_PATH_SLOT_MAX - 11u + (unsigned int)i) != LKS_STATUS_OK ||
            lks_tree_insert(tree, child, &values[i + 5],
                i == 11 ? out_last : NULL) != LKS_STATUS_OK) valid = 0;
        lks_path_destroy(child);
    }
    lks_path_destroy(chain);
    return valid && *out_last != NULL && lks_tree_size(tree) == 17;
}

static int check_slot_codec(void)
{
    /* Independent expected alphabet: do not ask the codec for its own oracle. */
    static const char expected_alphabet[] =
        "23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz";
    static const struct { unsigned int slot; const char *text; } examples[] = {
        {0u,"222"},{1u,"223"},{53u,"22z"},{54u,"232"},{130u,"24R"},
        {259u,"26p"},{32767u,"DEp"},{32768u,"DEq"},{65535u,"RUc"}
    };
    char previous[4] = {0,0,0,0}, current[4];
    uint_least32_t slot = 0;
    size_t index;
    if (strlen(expected_alphabet) != 54u ||
        LKS_PATH_SLOT_MIN != 0u || LKS_PATH_SLOT_MAX != 65535u ||
        LKS_SLOT_TEXT_RADIX != 54 || LKS_SLOT_TEXT_WIDTH != 3) goto fail;
#if UINT_MAX > LKS_PATH_SLOT_MAX
    if (lks_slot_encode(LKS_PATH_SLOT_MAX + 1u, current)) goto fail;
#endif
    for (index = 0; index < sizeof(examples)/sizeof(examples[0]); ++index) {
        slot = examples[index].slot;
        if (!lks_slot_encode(examples[index].slot, current)) goto fail;
        current[3] = '\0';
        if (strcmp(current, examples[index].text) != 0) goto fail;
    }
    for (slot = 0; slot <= 65535u; ++slot) {
        size_t digit;
        if (!lks_slot_encode((unsigned int)slot, current)) goto fail;
        current[3] = '\0';
        if (slot < 54u && (current[0] != '2' || current[1] != '2' ||
                current[2] != expected_alphabet[slot])) goto fail;
        for (digit = 0; digit < LKS_SLOT_TEXT_WIDTH; ++digit) {
            if (strchr(expected_alphabet, current[digit]) == NULL ||
                strchr("0Oo1IiLl", current[digit]) != NULL ||
                !((current[digit] >= '2' && current[digit] <= '9') ||
                  (current[digit] >= 'A' && current[digit] <= 'Z') ||
                  (current[digit] >= 'a' && current[digit] <= 'z')))
                goto fail;
        }
        if (slot != 0 && strcmp(previous, current) >= 0) goto fail;
        memcpy(previous, current, sizeof(previous));
    }
    printf("V2 SlotCodec Exhaustive=65536 Examples=9 LexicalOrder=PASS Status=PASS\n");
    return 1;
fail:
    printf("V2 SlotCodec FailedSlot=%u Status=FAIL\n", (unsigned int)slot);
    return 0;
}

static int text_equals(const LksPath *path, const char *expected)
{
    char buffer[128], short_buffer[128] = "unchanged";
    size_t length = lks_path_text_length(path);
    return length == strlen(expected) && length < sizeof(buffer) &&
        lks_path_format(path, short_buffer, length) == LKS_STATUS_BUFFER_TOO_SMALL &&
        strcmp(short_buffer, "unchanged") == 0 &&
        lks_path_format(path, buffer, length + 1) == LKS_STATUS_OK &&
        buffer[length] == '\0' && strcmp(buffer, expected) == 0;
}

static int check_slot_domain(void)
{
    LksPath *low = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    LksPath *high = lks_path_create(LKS_DIRECTION_POSITIVE, 65535u);
    LksPath *copy = NULL, *middle = NULL, *before = NULL, *after = NULL;
    unsigned int slot = 0;
    int order = 0, valid = low != NULL && high != NULL;
    if (valid) valid = lks_path_get_slot(high, 0, &slot) == LKS_STATUS_OK &&
        slot == 65535u &&
        lks_path_compare(low, high, &order) == LKS_STATUS_OK && order < 0;
#if UINT_MAX > LKS_PATH_SLOT_MAX
    if (valid) valid = lks_path_create(LKS_DIRECTION_POSITIVE, 65536u) == NULL &&
        lks_path_create_at_level(LKS_DIRECTION_NEGATIVE, 65536u, 0) == NULL;
#endif
    if (valid) copy = lks_path_clone(high);
    if (valid) valid = copy != NULL &&
        lks_path_append(copy, 65535u) == LKS_STATUS_OK &&
#if UINT_MAX > LKS_PATH_SLOT_MAX
        lks_path_append(copy, 65536u) == LKS_STATUS_INVALID_ARGUMENT &&
#endif
        lks_path_depth(copy) == 2 &&
        lks_path_get_slot(copy, 1, &slot) == LKS_STATUS_OK && slot == 65535u;
    if (valid) valid = lks_path_between(low, high, &middle) == LKS_STATUS_OK &&
        lks_path_before(low, &before) == LKS_STATUS_OK &&
        lks_path_after(high, &after) == LKS_STATUS_OK &&
        lks_path_compare(low, middle, &order) == LKS_STATUS_OK && order < 0 &&
        lks_path_compare(middle, high, &order) == LKS_STATUS_OK && order < 0 &&
        lks_path_compare(before, low, &order) == LKS_STATUS_OK && order < 0 &&
        lks_path_compare(high, after, &order) == LKS_STATUS_OK && order < 0;
    lks_path_destroy(low); lks_path_destroy(high); lks_path_destroy(copy);
    lks_path_destroy(middle); lks_path_destroy(before); lks_path_destroy(after);
    valid = valid && clean_allocator();
    printf("V2 SlotDomain Boundaries=0,65535,65536 CloneAndGaps=%s\n",
        valid ? "PASS" : "FAIL");
    return valid;
}

static int check_level_text(void)
{
    static const size_t depths[] = {1,2,4,8,16,32,64};
    LksPath *zero = lks_path_create_zero();
    LksPath *low = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    LksPath *high = lks_path_create(LKS_DIRECTION_POSITIVE, 65535u);
    LksPath *skip = NULL, *large = NULL;
    size_t i;
    int valid = zero != NULL && low != NULL && high != NULL;
    if (valid) valid = text_equals(zero, "000") &&
        text_equals(low, "0222") && text_equals(high, "0RUc");
    if (valid) skip = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 0, 0);
    if (valid) valid = skip != NULL &&
        lks_path_append_at_level(skip, 1, 5) == LKS_STATUS_OK &&
        lks_path_append_at_level(skip, 53, 6) == LKS_STATUS_OK &&
        text_equals(skip, "0222/5223/22z");
    if (valid) large = lks_path_create_at_level(LKS_DIRECTION_NEGATIVE, 54, 12);
    if (valid) valid = large != NULL && text_equals(large, "112232");
    lks_path_destroy(large);
    large = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 0, SIZE_MAX);
    if (valid) {
        char expected[128];
        snprintf(expected, sizeof(expected), "0%zu222", (size_t)SIZE_MAX);
        valid = large != NULL && text_equals(large, expected);
    }
    for (i = 0; valid && i < sizeof(depths)/sizeof(depths[0]); ++i) {
        LksPath *path = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
        size_t step;
        if (path == NULL) { valid = 0; break; }
        for (step = 1; step < depths[i]; ++step)
            if (lks_path_append(path, 0) != LKS_STATUS_OK) { valid = 0; break; }
        if (valid) {
            char buffer[257];
            size_t length = lks_path_text_length(path);
            valid = length == 4 * depths[i] &&
                lks_path_format(path, buffer, length + 1) == LKS_STATUS_OK &&
                strlen(buffer) == length && strstr(buffer, "//") == NULL;
        }
        lks_path_destroy(path);
    }
    lks_path_destroy(zero); lks_path_destroy(low);
    lks_path_destroy(high); lks_path_destroy(skip); lks_path_destroy(large);
    valid = valid && clean_allocator();
    printf("V2 LevelText GrammarAndBuffer=%s Depths=1,2,4,8,16,32,64\n",
        valid ? "PASS" : "FAIL");
    return valid;
}

static int check_bulk_slot_policy(void)
{
    static const size_t counts[] = {1,2,25,26};
    size_t case_index, failed_blocks = 0;
    for (case_index = 0; case_index < sizeof(counts)/sizeof(counts[0]); ++case_index) {
        size_t blocks = counts[case_index], index;
        unsigned int previous = 0;
        V2Item values[27];
        void *items[27];
        const LksTreeNode *ordered[27];
        LksTree *tree = NULL;
        int valid;
        failed_blocks = blocks;
        for (index = 0; index <= blocks; ++index) {
            values[index].key = (int)index;
            values[index].order = index;
            items[index] = &values[index];
        }
        if (lks_bulk_build_tree(items, blocks + 1, &tree) != LKS_STATUS_OK ||
            tree == NULL || lks_tree_root_child_count(tree) != 1 ||
            lks_tree_internal_fill_ordered(tree, ordered, blocks + 1) !=
                LKS_STATUS_OK) {
            lks_tree_destroy(tree); goto fail;
        }
        valid = lks_path_direction(lks_tree_node_path(
            ordered[0])) == LKS_DIRECTION_ZERO;
        for (index = 0; index < blocks; ++index) {
            unsigned int slot = lks_policy_bulk_slot(index, blocks);
            unsigned int actual = 0;
            unsigned int expected_slot = (unsigned int)(
                ((uint_least32_t)(index + 1) * UINT32_C(65536)) / (blocks + 1));
            const LksPath *path = lks_tree_node_path(
                ordered[index + 1]);
            if (slot != expected_slot || slot <= previous || slot >= 65535u ||
                lks_path_get_slot(path, 0, &actual) != LKS_STATUS_OK ||
                actual != slot) valid = 0;
            previous = slot;
        }
        lks_tree_destroy(tree);
        if (!valid || !clean_allocator()) goto fail;
        if (blocks == 1 && (previous != 32768u ||
            LKS_POLICY_INITIAL_SLOT != 32768)) goto fail;
    }
    printf("V2 BulkSlotPolicy Blocks=1,2,25,26 Status=PASS\n");
    return 1;
fail:
    printf("V2 BulkSlotPolicy Blocks=%zu Status=FAIL\n", failed_blocks);
    return 0;
}

static int check_local_relabel(void)
{
    V2Item values[18];
    void *expected[18];
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    LksTreeRepairStats stats;
    TreeSnapshot before, after;
    size_t i, count = 18, max_depth = 0;
    const LksTreeNode *last = NULL;
    int valid;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    valid = tree != NULL;
    if (!valid) return 0;
    lks_tree_repair_stats_reset();
    if (valid) valid = build_congested_branch(tree, values, &last);
    for (i = 0; i < 18; ++i) expected[i] = &values[i];
    values[17].key = 17; values[17].order = 17;
    if (valid && !snapshot_tree(tree, &before)) valid = 0;
    if (valid && lks_tree_insert_item(tree, &values[17], &comparator, NULL) !=
            LKS_STATUS_OK) valid = 0;
    stats = lks_tree_repair_stats_get();
    if (valid && !snapshot_tree(tree, &after)) valid = 0;
    if (valid) valid = snapshot_local_repair_preserves_outside(&before, &after,
        stats.nodes_relabelled);
    if (valid) valid = audit_tree_order(tree, expected, count, &max_depth);
    valid = valid && stats.successes == 1 && stats.region_expansions > 0 &&
        stats.full_rebuilds == 0 && stats.max_region_nodes <=
            LKS_POLICY_LOCAL_MAX_NODES && max_depth <= LKS_POLICY_HARD_ONLINE_DEPTH;
    printf("V2 LocalRelabel N=%zu Attempts=%zu Success=%zu Expanded=%zu Nodes=%zu MaxRegion=%zu Full=%zu MaxDepth=%zu Status=%s\n",
        count, stats.attempts, stats.successes, stats.region_expansions,
        stats.nodes_relabelled, stats.max_region_nodes, stats.full_rebuilds,
        max_depth, valid ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_parent_first_descendant_repair(void)
{
    V2Item values[6];
    void *expected[6];
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    LksPath *path;
    TreeSnapshot before, after;
    LksTreeRepairStats stats;
    const LksTreeNode *inserted = NULL;
    size_t i, max_depth = 0;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    path = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
        LKS_POLICY_INITIAL_SLOT, 0);
    if (tree == NULL || path == NULL) valid = 0;
    for (i = 0; i < 5 && valid; ++i) {
        if (i > 0 && lks_path_append_at_level(path,
                LKS_POLICY_INITIAL_SLOT, i) != LKS_STATUS_OK) valid = 0;
        values[i].key = i == 4 ? 5 : (int)i;
        values[i].order = i;
        if (valid && lks_tree_insert(tree, path, &values[i], NULL) !=
                LKS_STATUS_OK) valid = 0;
    }
    values[5].key = 4; values[5].order = 5;
    for (i = 0; i < 4; ++i) expected[i] = &values[i];
    expected[4] = &values[5]; expected[5] = &values[4];
    if (valid && !snapshot_tree(tree, &before)) valid = 0;
    lks_tree_repair_stats_reset();
    if (valid && lks_tree_insert_item(tree, &values[5], &comparator,
            &inserted) != LKS_STATUS_OK) valid = 0;
    stats = lks_tree_repair_stats_get();
    if (valid && !snapshot_tree(tree, &after)) valid = 0;
    if (valid) valid = inserted != NULL &&
        lks_tree_node_item(inserted) == &values[5] &&
        snapshot_local_repair_preserves_outside(&before, &after,
            stats.nodes_relabelled) &&
        audit_tree_order(tree, expected, 6, &max_depth);
    valid = valid && stats.successes == 1 && stats.full_rebuilds == 0;
    printf("V2 ParentFirstDescendant Local=%zu Region=%zu MaxDepth=%zu Status=%s\n",
        stats.successes, stats.nodes_relabelled, max_depth,
        valid ? "PASS" : "FAIL");
    lks_path_destroy(path);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_positive_boundary_repair(int between_siblings)
{
    V2Item values[7];
    void *expected[7];
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    LksPath *base, *left = NULL, *first = NULL, *right = NULL;
    TreeSnapshot before, after;
    LksTreeRepairStats stats;
    size_t i, max_depth = 0;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    base = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
        LKS_POLICY_INITIAL_SLOT, 0);
    if (tree == NULL || base == NULL) valid = 0;
    for (i = 0; i < 3 && valid; ++i) {
        if (i > 0 && lks_path_append_at_level(base,
                LKS_POLICY_INITIAL_SLOT, i) != LKS_STATUS_OK) valid = 0;
        values[i].key = (int)i; values[i].order = i;
        if (valid && lks_tree_insert(tree, base, &values[i], NULL) !=
                LKS_STATUS_OK) valid = 0;
    }
    if (valid) left = lks_path_clone(base);
    if (left == NULL || lks_path_append_at_level(left,
            LKS_POLICY_INITIAL_SLOT, 3) != LKS_STATUS_OK) valid = 0;
    values[3].key = 3; values[3].order = 3;
    if (valid && lks_tree_insert(tree, left, &values[3], NULL) != LKS_STATUS_OK)
        valid = 0;
    if (valid) first = lks_path_clone(left);
    if (first == NULL || lks_path_append_at_level(first,
            LKS_POLICY_INITIAL_SLOT, 4) != LKS_STATUS_OK) valid = 0;
    values[4].key = 4; values[4].order = 4;
    if (valid && lks_tree_insert(tree, first, &values[4], NULL) != LKS_STATUS_OK)
        valid = 0;
    if (valid) right = lks_path_clone(between_siblings ? base : left);
    if (right == NULL || lks_path_append_at_level(right,
            LKS_POLICY_INITIAL_SLOT + 1u,
            between_siblings ? 3u : 4u) != LKS_STATUS_OK) valid = 0;
    values[5].key = 6; values[5].order = 5;
    if (valid && lks_tree_insert(tree, right, &values[5], NULL) != LKS_STATUS_OK)
        valid = 0;
    values[6].key = 5; values[6].order = 6;
    for (i = 0; i < 5; ++i) expected[i] = &values[i];
    expected[5] = &values[6]; expected[6] = &values[5];
    if (valid && !snapshot_tree(tree, &before)) valid = 0;
    lks_tree_repair_stats_reset();
    if (valid && lks_tree_insert_item(tree, &values[6], &comparator, NULL) !=
            LKS_STATUS_OK) valid = 0;
    stats = lks_tree_repair_stats_get();
    if (valid && !snapshot_tree(tree, &after)) valid = 0;
    if (valid) valid = snapshot_local_repair_preserves_outside(&before, &after,
            stats.nodes_relabelled) &&
        audit_tree_order(tree, expected, 7, &max_depth);
    valid = valid && stats.successes == 1 && stats.full_rebuilds == 0;
    printf("V2 PositiveBoundary Kind=%s Local=%zu Region=%zu MaxDepth=%zu Status=%s\n",
        between_siblings ? "sibling-subtrees" : "descendants",
        stats.successes, stats.nodes_relabelled, max_depth,
        valid ? "PASS" : "FAIL");
    lks_path_destroy(base); lks_path_destroy(left);
    lks_path_destroy(first); lks_path_destroy(right);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_online_pattern(size_t count, int mode)
{
    V2Item values[160];
    void *expected[160];
    LksTree *tree;
    LksComparator comparator = {compare_item, NULL};
    LksTreeRepairStats stats;
    TreeSnapshot before, after;
    size_t i, j, max_depth = 0, max_text = 0;
    int valid;
    if (count > 160 || lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    valid = tree != NULL;
    if (!valid) return 0;
    lks_tree_repair_stats_reset();
    for (i = 0; i < count && valid; ++i) {
        LksTreeRepairStats prior = lks_tree_repair_stats_get();
        if (!snapshot_tree(tree, &before)) { valid = 0; break; }
        if (mode == 0) values[i].key = (i & 1u) == 0 ?
            -(int)(i / 2) : (int)(i / 2 + 1);
        else if (mode == 1) values[i].key = (int)(i % 9);
        else if (mode == 2) values[i].key = 7;
        else values[i].key = (int)((i * 73u) % count) - (int)(count / 2);
        values[i].order = i;
        if (lks_tree_insert_item(tree, &values[i], &comparator, NULL) !=
                LKS_STATUS_OK) valid = 0;
        if (valid && lks_tree_repair_stats_get().successes > prior.successes) {
            LksTreeRepairStats now = lks_tree_repair_stats_get();
            if (!snapshot_tree(tree, &after) ||
                !snapshot_local_repair_preserves_outside(&before, &after,
                    now.nodes_relabelled - prior.nodes_relabelled)) valid = 0;
        }
        j = i;
        while (j > 0 && compare_item(expected[j - 1], &values[i], NULL) > 0) {
            expected[j] = expected[j - 1]; --j;
        }
        expected[j] = &values[i];
    }
    if (valid) valid = audit_tree_order(tree, expected, count, &max_depth);
    if (valid) {
        TreeSnapshot snapshot;
        if (!snapshot_tree(tree, &snapshot)) valid = 0;
        else for (i = 0; i < snapshot.count; ++i) {
            size_t length = strlen(snapshot.paths[i]);
            if (length > max_text) max_text = length;
        }
    }
    stats = lks_tree_repair_stats_get();
    valid = valid && max_depth <= LKS_POLICY_HARD_ONLINE_DEPTH;
    printf("V2 OnlinePattern Mode=%d N=%zu Attempts=%zu Local=%zu Fallbacks=%zu Expanded=%zu Full=%zu Deeper=%zu MaxDepth=%zu MaxText=%zu PeakBytes=%zu RequestedBytes=%zu Status=%s\n",
        mode, count, stats.attempts, stats.successes, stats.fallbacks,
        stats.region_expansions, stats.full_rebuilds, stats.deeper_accepts,
        max_depth, max_text, lks_alloc_stats_get().peak_live_bytes,
        lks_alloc_stats_get().total_successful_requested_bytes,
        valid ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_local_repair_oom(void)
{
    V2Item values[18];
    LksTree *tree;
    LksComparator comparator = {compare_item, NULL};
    const LksTreeNode *last = NULL;
    TreeSnapshot before_snapshot, after_snapshot;
    size_t i = 17, fail_index, failure_points = 0;
    int success = 0, valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    lks_tree_repair_stats_reset();
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    valid = build_congested_branch(tree, values, &last);
    values[i].key = (int)i; values[i].order = i;
    lks_tree_repair_stats_reset();
    if (valid && !snapshot_tree(tree, &before_snapshot)) valid = 0;
    if (valid) {
        LksPath *snapshot = lks_path_clone(lks_tree_node_path(last));
        if (snapshot == NULL) valid = 0;
        for (fail_index = 1; valid && fail_index <= 10000; ++fail_index) {
            LksAllocStats before = lks_alloc_stats_get();
            const LksTreeNode *inserted = NULL, *found = NULL;
            LksStatus status;
            lks_alloc_test_fail_on_attempt(fail_index);
            status = lks_tree_insert_item(tree, &values[i], &comparator, &inserted);
            lks_alloc_test_disable_failure();
            if (status == LKS_STATUS_OK) {
                success = inserted != NULL &&
                    lks_tree_node_item(inserted) == &values[i] &&
                    lks_tree_find_path(tree, lks_tree_node_path(inserted),
                        &found) == LKS_STATUS_OK && found == inserted &&
                    lks_tree_size(tree) == i + 1;
                break;
            }
            if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
                lks_tree_size(tree) != i ||
                !snapshot_tree(tree, &after_snapshot) ||
                !snapshots_equal(&before_snapshot, &after_snapshot) ||
                lks_tree_find_path(tree, snapshot, &found) != LKS_STATUS_OK ||
                found != last || lks_alloc_stats_get().live_bytes != before.live_bytes ||
                lks_alloc_stats_get().live_blocks != before.live_blocks) valid = 0;
            ++failure_points;
        }
        lks_alloc_test_disable_failure();
        lks_path_destroy(snapshot);
    }
    { LksTreeRepairStats stats = lks_tree_repair_stats_get();
      printf("V2 Tree congestion OOM N=%zu FailPoints=%zu Success=%d Local=%zu Expanded=%zu Nodes=%zu Full=%zu\n",
          i, failure_points, success, stats.successes,
          stats.region_expansions, stats.nodes_relabelled, stats.full_rebuilds); }
    { LksTreeRepairStats stats = lks_tree_repair_stats_get();
      valid = valid && stats.successes == 1 && stats.full_rebuilds == 0; }
    lks_tree_destroy(tree);
    return valid && success && failure_points > 0 && clean_allocator();
}

static int check_full_fallback_oom(void)
{
    V2Item values[8];
    void *expected[8];
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    LksPath *path;
    TreeSnapshot before, after;
    LksTreeRepairStats stats;
    size_t i, fail_index, failures = 0, max_depth = 0;
    int valid = 1, success = 0;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    path = lks_path_create_at_level(LKS_DIRECTION_NEGATIVE,
        LKS_POLICY_INITIAL_SLOT, 0);
    if (tree == NULL || path == NULL) valid = 0;
    for (i = 0; i < 7 && valid; ++i) {
        if (i > 0 && lks_path_append_at_level(path,
                LKS_POLICY_INITIAL_SLOT, i) != LKS_STATUS_OK) valid = 0;
        values[i].key = (int)i; values[i].order = i;
        expected[i] = &values[i];
        if (valid && lks_tree_insert(tree, path, &values[i], NULL) !=
                LKS_STATUS_OK) valid = 0;
    }
    values[7].key = 7; values[7].order = 7;
    expected[7] = &values[7];
    if (valid && !snapshot_tree(tree, &before)) valid = 0;
    lks_tree_repair_stats_reset();
    for (fail_index = 1; valid && fail_index <= 10000; ++fail_index) {
        LksAllocStats live = lks_alloc_stats_get();
        const LksTreeNode *inserted = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_tree_insert_item(tree, &values[7], &comparator, &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = inserted != NULL &&
                lks_tree_node_item(inserted) == &values[7] &&
                audit_tree_order(tree, expected, 8, &max_depth);
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            !snapshot_tree(tree, &after) || !snapshots_equal(&before, &after) ||
            lks_alloc_stats_get().live_bytes != live.live_bytes ||
            lks_alloc_stats_get().live_blocks != live.live_blocks) valid = 0;
        ++failures;
    }
    lks_alloc_test_disable_failure();
    stats = lks_tree_repair_stats_get();
    valid = valid && success && failures > 0 &&
        stats.full_rebuilds + stats.successes == 1;
    printf("V2 RangeOrFallbackOOM FailPoints=%zu Local=%zu Full=%zu MaxDepth=%zu Status=%s\n",
        failures, stats.successes, stats.full_rebuilds, max_depth,
        valid ? "PASS" : "FAIL");
    lks_path_destroy(path);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_level_limit_full_fallback(void)
{
    V2Item values[2] = {{0, 0}, {1, 1}};
    void *expected[2] = {&values[0], &values[1]};
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    LksPath *path;
    LksTreeRepairStats stats;
    const LksTreeNode *inserted = NULL;
    size_t max_depth = 0;
    int valid;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    path = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
        LKS_PATH_SLOT_MAX, (size_t)-1);
    valid = tree != NULL && path != NULL;
    if (valid && lks_tree_insert(tree, path, &values[0], NULL) != LKS_STATUS_OK)
        valid = 0;
    lks_tree_repair_stats_reset();
    if (valid && lks_tree_insert_item(tree, &values[1], &comparator,
            &inserted) != LKS_STATUS_OK) valid = 0;
    stats = lks_tree_repair_stats_get();
    if (valid) valid = inserted != NULL &&
        lks_tree_node_item(inserted) == &values[1] &&
        audit_tree_order(tree, expected, 2, &max_depth);
    valid = valid && stats.full_rebuilds == 1;
    printf("V2 LevelLimitFallback Full=%zu Status=%s\n",
        stats.full_rebuilds, valid ? "PASS" : "FAIL");
    lks_path_destroy(path);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_stage12_explicit_and_deep(void)
{
    static const size_t depths[] = { 1000u, 10000u, 100000u };
    LksTree *tree;
    LksPath *zero = NULL, *negative = NULL, *positive = NULL;
    const LksTreeNode *node = NULL;
    LksTreeInternalProfile profile;
    int values[6] = {0, 1, 2, 3, 4, 5};
    size_t i, d;
    int valid = 1, order;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    zero = lks_path_create_zero();
    negative = lks_path_create_at_level(LKS_DIRECTION_NEGATIVE, 65535u,
        (size_t)-1);
    positive = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 0u,
        (size_t)-1);
    if (tree == NULL || zero == NULL || negative == NULL || positive == NULL)
        valid = 0;
    if (valid && (lks_tree_insert(tree, positive, &values[2], &node) !=
            LKS_STATUS_OK || node == NULL ||
        lks_tree_insert(tree, negative, &values[0], NULL) != LKS_STATUS_OK ||
        lks_tree_insert(tree, zero, &values[1], NULL) != LKS_STATUS_OK ||
        lks_tree_insert(tree, positive, &values[5], &node) !=
            LKS_STATUS_ALREADY_EXISTS || node != NULL ||
        lks_tree_root_child_count(tree) != 1 ||
        lks_tree_find_path(tree, negative, &node) != LKS_STATUS_OK ||
        lks_tree_node_item(node) != &values[0] ||
        lks_path_compare(negative, zero, &order) != LKS_STATUS_OK ||
        order >= 0 ||
        lks_path_compare(zero, positive, &order) != LKS_STATUS_OK ||
        order >= 0)) valid = 0;
    for (d = 0; valid && d < sizeof(depths)/sizeof(depths[0]); ++d) {
        LksPath *deep = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
            (unsigned int)(d + 1u), 0);
        if (deep == NULL) { valid = 0; break; }
        for (i = 1; i < depths[d]; ++i)
            if (lks_path_append_at_level(deep, 1u, i) != LKS_STATUS_OK)
                { valid = 0; break; }
        if (valid && (lks_tree_insert(tree, deep, &values[d + 3u], NULL) !=
                LKS_STATUS_OK ||
            lks_tree_find_path(tree, deep, &node) != LKS_STATUS_OK ||
            lks_tree_node_item(node) != &values[d + 3u] ||
            lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
            !profile.balance_valid || profile.real_node_count != d + 4u))
            valid = 0;
        lks_path_destroy(deep);
    }
    printf("V2 AVL ExplicitNoPrefix Deep=1000,10000,100000 Status=%s\n",
        valid ? "PASS" : "FAIL");
    lks_path_destroy(positive);
    lks_path_destroy(negative);
    lks_path_destroy(zero);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_stage12_equal_upper_bound(void)
{
    enum { COUNT = 4096 };
    V2Item *items;
    const LksTreeNode **nodes;
    LksTree *tree;
    LksComparator comparator = {compare_item, NULL};
    LksTreeRepairStats stats;
    LksTreeInternalProfile profile;
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    items = (V2Item *)malloc(COUNT * sizeof(*items));
    nodes = (const LksTreeNode **)malloc(COUNT * sizeof(*nodes));
    tree = lks_tree_create();
    if (items == NULL || nodes == NULL || tree == NULL) valid = 0;
    lks_tree_repair_stats_reset();
    for (i = 0; i < COUNT && valid; ++i) {
        items[i].key = 7; items[i].order = i;
        if (lks_tree_insert_item(tree, &items[i], &comparator, NULL) !=
            LKS_STATUS_OK) valid = 0;
        if (valid && (i % 64u == 63u) &&
            (lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
             !profile.balance_valid || profile.real_node_count != i + 1u))
            valid = 0;
    }
    stats = lks_tree_repair_stats_get();
    if (valid && (lks_tree_internal_fill_ordered(tree, nodes, COUNT) !=
        LKS_STATUS_OK ||
        lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
        !profile.balance_valid || profile.real_node_count != COUNT ||
        stats.comparator_search_steps > 32u * COUNT)) valid = 0;
    for (i = 0; i < COUNT && valid; ++i)
        if (lks_tree_node_item(nodes[i]) != &items[i]) valid = 0;
    printf("V2 AVL EqualUpperBound N=%u SearchSteps=%zu Rotations=%zu Status=%s\n",
        COUNT, stats.comparator_search_steps, stats.rotations,
        valid ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    free(nodes);
    free(items);
    return valid && clean_allocator();
}

static int check_stage12_explicit_rotation_oom(void)
{
    LksTree *tree;
    LksPath *first = NULL, *third = NULL, *second = NULL;
    TreeSnapshot before, after;
    LksTreeInternalProfile profile;
    LksAllocStats live;
    const LksTreeNode *inserted = NULL;
    size_t fail_index, failures = 0;
    int values[3] = {1, 2, 3};
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    first = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 1u, 0);
    third = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 3u, 0);
    second = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 2u, 0);
    if (tree == NULL || first == NULL || second == NULL || third == NULL ||
        lks_tree_insert(tree, first, &values[0], NULL) != LKS_STATUS_OK ||
        lks_tree_insert(tree, third, &values[2], NULL) != LKS_STATUS_OK ||
        !snapshot_tree(tree, &before)) valid = 0;
    if (valid) live = lks_alloc_stats_get();
    for (fail_index = 1; valid && fail_index <= 16; ++fail_index) {
        LksStatus status;
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_tree_insert(tree, second, &values[1], &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            valid = inserted != NULL && lks_tree_size(tree) == 3 &&
                lks_tree_internal_profile(tree, &profile) == LKS_STATUS_OK &&
                profile.balance_valid && profile.avl_height == 2 &&
                lks_tree_node_item(lks_tree_root_child_at(tree, 0)) == &values[1];
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            !snapshot_tree(tree, &after) || !snapshots_equal(&before, &after) ||
            lks_alloc_stats_get().live_bytes != live.live_bytes ||
            lks_alloc_stats_get().live_blocks != live.live_blocks) valid = 0;
        ++failures;
    }
    printf("V2 AVL ExplicitRotationOOM FailPoints=%zu Status=%s\n",
        failures, valid && failures > 0 ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    lks_path_destroy(first); lks_path_destroy(second); lks_path_destroy(third);
    return valid && failures > 0 && clean_allocator();
}

static int check_stage12_locate(void)
{
    V2Item values[3] = {{10, 0}, {20, 1}, {30, 2}};
    V2Item query = {25, 0};
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree;
    const LksTreeNode *left = NULL, *equal = NULL, *right = NULL;
    size_t i;
    int valid = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    for (i = 0; i < 3 && valid; ++i) {
        LksPath *path = lks_path_create_at_level(
            LKS_DIRECTION_POSITIVE, (unsigned int)(i + 1), 0);
        if (path == NULL || lks_tree_insert(tree, path, &values[i], NULL) !=
            LKS_STATUS_OK) valid = 0;
        lks_path_destroy(path);
    }
    if (valid && (lks_tree_locate_item(tree, &query, &comparator,
            &left, &equal, &right) != LKS_STATUS_OK ||
        lks_tree_node_item(left) != &values[1] || equal != NULL ||
        lks_tree_node_item(right) != &values[2])) valid = 0;
    query.key = 20;
    if (valid && (lks_tree_locate_item(tree, &query, &comparator,
            &left, &equal, &right) != LKS_STATUS_OK ||
        left != NULL || lks_tree_node_item(equal) != &values[1] ||
        right != NULL)) valid = 0;
    printf("V2 AVL ComparatorLocate Status=%s\n", valid ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_stage12_full_fallback_oom(void)
{
    LksTree *tree;
    LksPath *edge;
    LksComparator comparator = {compare_item, NULL};
    V2Item values[2] = {{0, 0}, {1, 1}};
    TreeSnapshot before, after;
    LksAllocStats live;
    LksTreeRepairStats stats;
    const LksTreeNode *inserted = NULL;
    size_t fail_index, failures = 0;
    int valid = 1, success = 0;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    edge = lks_path_create_at_level(LKS_DIRECTION_POSITIVE,
        LKS_PATH_SLOT_MAX, (size_t)-1);
    if (tree == NULL || edge == NULL ||
        lks_tree_insert(tree, edge, &values[0], NULL) != LKS_STATUS_OK ||
        !snapshot_tree(tree, &before)) valid = 0;
    if (valid) live = lks_alloc_stats_get();
    lks_tree_repair_stats_reset();
    for (fail_index = 1; valid && fail_index <= 100; ++fail_index) {
        LksStatus status;
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_tree_insert_item(tree, &values[1], &comparator, &inserted);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = inserted != NULL && lks_tree_size(tree) == 2 &&
                lks_tree_node_item(inserted) == &values[1];
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || inserted != NULL ||
            !snapshot_tree(tree, &after) || !snapshots_equal(&before, &after) ||
            lks_alloc_stats_get().live_bytes != live.live_bytes ||
            lks_alloc_stats_get().live_blocks != live.live_blocks) valid = 0;
        ++failures;
    }
    stats = lks_tree_repair_stats_get();
    valid = valid && success && failures > 0 && stats.full_rebuilds == 1;
    printf("V2 AVL FullFallbackOOM FailPoints=%zu Status=%s\n",
        failures, valid ? "PASS" : "FAIL");
    lks_path_destroy(edge);
    lks_tree_destroy(tree);
    return valid && clean_allocator();
}

static int check_stage12_bulk_oom(void)
{
    V2Item values[6];
    void *items[6];
    LksTree *tree = NULL;
    size_t i, fail_index, failures = 0;
    int valid = 1, success = 0;
    if (lks_alloc_stats_reset() != 0) return 0;
    for (i = 0; i < 6; ++i) {
        values[i].key = (int)i; values[i].order = i; items[i] = &values[i];
    }
    for (fail_index = 1; fail_index <= 100; ++fail_index) {
        LksStatus status;
        LksAllocStats before = lks_alloc_stats_get();
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_bulk_build_tree(items, 6, &tree);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = tree != NULL && lks_tree_size(tree) == 6;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || tree != NULL ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks)
            valid = 0;
        ++failures;
    }
    printf("V2 AVL BulkOOM FailPoints=%zu Status=%s\n",
        failures, valid && success && failures > 0 ? "PASS" : "FAIL");
    lks_tree_destroy(tree);
    return valid && success && failures > 0 && clean_allocator();
}

/* Endpoint insertion within the hard depth allowance should not repeatedly
 * prepare almost identical repair windows. This is a private policy guard,
 * while the Tree profile and stable item order remain semantic checks. */
static int check_endpoint_repair_pressure(void)
{
    enum { COUNT = 20000 };
    V2Item *values = (V2Item *)malloc(COUNT * sizeof(*values));
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree = NULL;
    LksTreeRepairStats stats;
    LksTreeInternalProfile profile;
    size_t i;
    int valid = values != NULL && lks_alloc_stats_reset() == 0;
    if (valid) tree = lks_tree_create();
    valid = valid && tree != NULL;
    if (valid) lks_tree_repair_stats_reset();
    for (i = 0; valid && i < COUNT; ++i) {
        values[i].key = (int)i;
        values[i].order = i;
        valid = lks_tree_insert_item(tree, &values[i], &comparator, NULL) ==
            LKS_STATUS_OK;
    }
    if (valid) {
        stats = lks_tree_repair_stats_get();
        valid = lks_tree_internal_profile(tree, &profile) == LKS_STATUS_OK &&
            profile.balance_valid && profile.real_node_count == COUNT &&
            profile.max_path_depth <= LKS_POLICY_HARD_ONLINE_DEPTH &&
            stats.attempts < COUNT / 50 && stats.full_rebuilds <= 2;
        printf("V2 EndpointPressure N=%u Attempts=%zu Full=%zu MaxDepth=%zu Status=%s\n",
            COUNT, stats.attempts, stats.full_rebuilds, profile.max_path_depth,
            valid ? "PASS" : "FAIL");
    }
    lks_tree_destroy(tree);
    free(values);
    return valid && clean_allocator();
}

int lks_run_v2_preview_tests(void)
{
    if (!check_slot_codec() || !check_slot_domain() || !check_level_text() ||
        !check_bulk_slot_policy() || !check_sort() || !check_gap_contract() ||
        !check_zero_boundaries() ||
        !check_explicit_tree_paths() || !check_merge_and_batch() ||
        !check_many_equal_merges() || !check_bulk_boundaries() ||
        !check_bulk_to_prepend() ||
        !check_group_case("ascending", 1024, 0, 4) ||
        !check_group_case("descending", 1024, 1, 4) ||
        !check_group_case("all-equal", 10000, 2, 4) ||
        !check_group_case("heavy-duplicates", 1024, 3, 4) ||
        !check_group_case("random-duplicates", 1024, 5, 4) ||
        !check_group_case("random-unique", 1024, 4, 4) ||
        !check_tree_sequence(1024, 0) || !check_tree_sequence(256, 1) ||
        !check_narrow_gap() || !check_local_relabel() ||
        !check_parent_first_descendant_repair() ||
        !check_positive_boundary_repair(0) ||
        !check_positive_boundary_repair(1) ||
        !check_online_pattern(128, 0) || !check_online_pattern(128, 1) ||
        !check_online_pattern(128, 2) || !check_online_pattern(160, 3) ||
        !check_endpoint_repair_pressure() ||
        !check_local_repair_oom() || !check_full_fallback_oom() ||
        !check_level_limit_full_fallback() ||
        !check_stage12_explicit_and_deep() ||
        !check_stage12_equal_upper_bound() ||
        !check_stage12_explicit_rotation_oom() ||
        !check_stage12_locate() ||
        !check_stage12_full_fallback_oom() ||
        !check_stage12_bulk_oom()) {
        puts("LayerKeySort v2 preview test FAILED");
        return 1;
    }
    puts("LayerKeySort v2 preview focused tests PASS");
    return 0;
}
