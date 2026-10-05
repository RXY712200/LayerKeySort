/* Research-only public-semantic and allocation-failure checks. Deliberately
 * congested depth-six coordinates exercise depth escape, equal-depth repair,
 * asymmetric selection and open-range preparation. No policy counts asserted. */
#include <stdint.h>
#include <stdio.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"

typedef struct Item { int key; unsigned int serial; } Item;
static int compare(const void *a, const void *b, void *context)
{
    const Item *x = (const Item *)a, *y = (const Item *)b;
    (void)context;
    return (x->key > y->key) - (x->key < y->key);
}
static int check_order(LksOrderedTree *tree)
{
    const LksTreeNode *nodes[21];
    size_t i, n = lks_ordered_tree_size(tree);
    LksTreeInternalProfile profile;
    if (n > 21 || lks_tree_internal_fill_ordered(lks_ordered_tree_internal_index(tree), nodes, n) != LKS_STATUS_OK ||
        lks_tree_internal_profile(lks_ordered_tree_internal_index(tree), &profile) != LKS_STATUS_OK ||
        !profile.balance_valid) return 0;
    for (i = 0; i < n; ++i) {
        const Item *x = (const Item *)lks_tree_node_item(nodes[i]);
        const LksTreeNode *left, *equal, *right, *found;
        size_t first = i;
        int cmp;
        if (i) {
            const Item *prev = (const Item *)lks_tree_node_item(nodes[i-1]);
            if (prev->key > x->key || (prev->key == x->key && prev->serial >= x->serial) ||
                lks_path_compare(lks_tree_node_path(nodes[i-1]), lks_tree_node_path(nodes[i]), &cmp) != LKS_STATUS_OK || cmp >= 0)
                return 0;
        }
        while (first && ((const Item *)lks_tree_node_item(nodes[first-1]))->key == x->key) --first;
        if (lks_ordered_tree_locate(tree,x,&left,&equal,&right) != LKS_STATUS_OK || equal != nodes[first] ||
            lks_ordered_tree_find_path(tree,lks_tree_node_path(nodes[i]),&found) != LKS_STATUS_OK || found != nodes[i]) return 0;
    }
    return 1;
}
static int sweep(int key)
{
    LksComparator cmp = {compare, NULL};
    LksOrderedTree *tree = lks_ordered_tree_create(&cmp);
    LksPath *original[20] = {0};
    Item items[21];
    size_t i, j, failure, failures = 0;
    int valid = tree != NULL, success = 0;
    for (i = 0; valid && i < 20; ++i) {
        original[i] = lks_path_create(LKS_DIRECTION_POSITIVE,32768);
        if (!original[i]) { valid = 0; break; }
        for (j = 1; j < 6; ++j)
            if (lks_path_append(original[i], j == 5 ? 200 + (unsigned int)i : 32768) != LKS_STATUS_OK) valid = 0;
        items[i].key = (int)(i*2);items[i].serial = (unsigned int)i;
        if (valid && lks_ordered_tree_test_seed_path(tree,original[i],&items[i]) != LKS_STATUS_OK) valid = 0;
    }
    items[20].key = key;items[20].serial = 20;
    for (failure = 1; valid && failure < 10000; ++failure) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *node = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(failure);
        status = lks_ordered_tree_insert(tree,&items[20],&node);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = node != NULL && lks_tree_node_item(node) == &items[20] &&
                lks_ordered_tree_size(tree)==21 && check_order(tree);break;
        }
        ++failures;
        if (status != LKS_STATUS_OUT_OF_MEMORY || node != NULL || lks_ordered_tree_size(tree) != 20 ||
            before.live_bytes != lks_alloc_stats_get().live_bytes || before.live_blocks != lks_alloc_stats_get().live_blocks ||
            !check_order(tree)) valid = 0;
        for (i = 0; valid && i < 20; ++i) {
            const LksTreeNode *found;
            int order;
            if (lks_ordered_tree_find_path(tree,original[i],&found)!=LKS_STATUS_OK ||
                found == NULL || lks_tree_node_item(found)!=&items[i] ||
                lks_path_compare(original[i],lks_tree_node_path(found),&order)!=LKS_STATUS_OK || order!=0) valid=0;
        }
    }
    printf("research OOM key=%d failures=%zu success=%d valid=%d\n",key,failures,success,valid);
    for (i=0;i<20;++i) lks_path_destroy(original[i]);
    lks_ordered_tree_destroy(tree);
    return valid && success && failures && lks_alloc_stats_get().live_bytes==0;
}
int main(void)
{
    if (!sweep(19) || !sweep(20) || !sweep(37)) return 1;
    return 0;
}
