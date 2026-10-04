#include "layerkeysort.h"

static int compare_int(const void *left, const void *right, void *context)
{
    const int a = *(const int *)left;
    const int b = *(const int *)right;
    (void)context;
    return (a > b) - (a < b);
}

int main(void)
{
    int values[] = {3, 1, 2};
    void *items[] = {&values[0], &values[1], &values[2]};
    LksComparator comparator = {compare_int, 0};
    LksOrderedTree *tree;
    LksPath *parsed = 0;
    const LksTreeNode *left = 0;
    const LksTreeNode *node = 0;
    const LksTreeNode *right = 0;
    int query = 2;
    void *removed = 0;

    if (lks_sort(items, 3, compare_int, 0) != LKS_STATUS_OK ||
        items[0] != &values[1] || items[2] != &values[0]) return 1;
    tree = lks_ordered_tree_create(&comparator);
    if (!tree) return 2;
    if (lks_ordered_tree_insert(tree, &values[0], 0) != LKS_STATUS_OK ||
        lks_ordered_tree_insert(tree, &values[1], 0) != LKS_STATUS_OK ||
        lks_ordered_tree_insert(tree, &values[2], 0) != LKS_STATUS_OK ||
        lks_ordered_tree_locate(tree, &query, &left, &node, &right)
            != LKS_STATUS_OK ||
        !node || lks_tree_node_item(node) != &values[2]) {
        lks_ordered_tree_destroy(tree);
        return 3;
    }
    if (lks_path_order_key_parse("LK1:201FF0000!", &parsed) != LKS_STATUS_OK ||
        !parsed || lks_path_direction(parsed) != LKS_DIRECTION_POSITIVE) {
        lks_path_destroy(parsed);
        lks_ordered_tree_destroy(tree);
        return 4;
    }
    lks_path_destroy(parsed);
    if (lks_ordered_tree_remove_path(tree, lks_tree_node_path(node), &removed)
            != LKS_STATUS_OK || removed != &values[2]) {
        lks_ordered_tree_destroy(tree);
        return 5;
    }
    lks_ordered_tree_destroy(tree);
    return 0;
}
