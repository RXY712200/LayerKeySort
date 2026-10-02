#include <stdio.h>
#include "layerkeysort.h"

typedef struct Item {
    int key;
    const char *name;
} Item;

static int compare_items(const void *left, const void *right, void *context)
{
    const Item *a = (const Item *)left;
    const Item *b = (const Item *)right;
    (void)context;
    return (a->key > b->key) - (a->key < b->key);
}

int main(void)
{
    Item items[] = {
        {20, "Middle"},
        {10, "Low"},
        {30, "High"}
    };
    Item query = {20, "query"};
    LksComparator comparator = {compare_items, NULL};
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    const LksTreeNode *left = NULL;
    const LksTreeNode *equal = NULL;
    const LksTreeNode *right = NULL;
    void *removed = NULL;
    LksStatus status = LKS_STATUS_OK;
    size_t i;
    int ok = 0;

    if (tree == NULL) {
        status = LKS_STATUS_OUT_OF_MEMORY;
        goto done;
    }
    for (i = 0; i < sizeof items / sizeof items[0]; ++i) {
        status = lks_ordered_tree_insert(tree, &items[i], NULL);
        if (status != LKS_STATUS_OK) {
            goto done;
        }
    }
    status = lks_ordered_tree_locate(tree, &query, &left, &equal, &right);
    if (status != LKS_STATUS_OK) {
        goto done;
    }
    if (equal == NULL || lks_tree_node_item(equal) != &items[0]) {
        status = LKS_STATUS_INTERNAL_ERROR;
        goto done;
    }
    printf("Found: %s\n", ((const Item *)lks_tree_node_item(equal))->name);

    /* The node and its Path are borrowed. Do not use them after removal. */
    status = lks_ordered_tree_remove_path(tree, lks_tree_node_path(equal),
        &removed);
    if (status != LKS_STATUS_OK) {
        goto done;
    }
    if (removed != &items[0] || lks_ordered_tree_size(tree) != 2) {
        status = LKS_STATUS_INTERNAL_ERROR;
        goto done;
    }
    status = lks_ordered_tree_locate(tree, &query, &left, &equal, &right);
    if (status != LKS_STATUS_OK) {
        goto done;
    }
    if (equal != NULL || left == NULL || right == NULL ||
        lks_tree_node_item(left) != &items[1] ||
        lks_tree_node_item(right) != &items[2]) {
        status = LKS_STATUS_INTERNAL_ERROR;
        goto done;
    }
    printf("After removal: %s < 20 < %s\n",
        ((const Item *)lks_tree_node_item(left))->name,
        ((const Item *)lks_tree_node_item(right))->name);
    ok = 1;

done:
    lks_ordered_tree_destroy(tree); /* The caller still owns items. */
    if (!ok) {
        fprintf(stderr, "Ordered-tree example failed: %s\n",
            lks_status_string(status));
    }
    return ok ? 0 : 1;
}
