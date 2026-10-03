#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Item { int id; int value; } Item;
static Item items[128];
static char old_paths[128][512];
static char now_paths[128][512];
static const LksPath *now_path_objects[128];
static int seen[128];
static int failed;

static int compare_items(const void *left, const void *right, void *context)
{
    const Item *a = (const Item *)left, *b = (const Item *)right;
    (void)context;
    return (a->value > b->value) - (a->value < b->value);
}

static void visit(const LksTreeNode *node)
{
    const Item *item;
    size_t i;
    if (!node || failed) return;
    item = (const Item *)lks_tree_node_item(node);
    if (!item || item->id < 0 || item->id >= 128) { failed = 1; return; }
    i = (size_t)item->id;
    if (seen[i] || lks_path_format(lks_tree_node_path(node),
        now_paths[i], sizeof now_paths[i]) != LKS_STATUS_OK) {
        failed = 1; return;
    }
    seen[i] = 1;
    now_path_objects[i] = lks_tree_node_path(node);
    visit(lks_tree_node_child_at(node, 0));
    visit(lks_tree_node_child_at(node, 1));
}

static int run(const char *name, const int *values, int count)
{
    LksComparator comparator = {compare_items, NULL};
    LksOrderedTree *tree = lks_ordered_tree_create(&comparator);
    int step, i, changed;
    int a, b, comparison;
    if (!tree) return 1;
    memset(old_paths, 0, sizeof old_paths);
    for (step = 0; step < count; ++step) {
        items[step].id = step;
        items[step].value = values[step];
        if (lks_ordered_tree_insert(tree, &items[step], NULL) != LKS_STATUS_OK)
            return 2;
        memset(seen, 0, sizeof seen);
        memset(now_paths, 0, sizeof now_paths);
        for (i = 0; i < (int)lks_ordered_tree_root_child_count(tree); ++i)
            visit(lks_ordered_tree_root_child_at(tree, (size_t)i));
        if (failed) return 3;
        for (a = 0; a <= step; ++a) for (b = a + 1; b <= step; ++b) {
            if (lks_path_compare(now_path_objects[a], now_path_objects[b],
                &comparison) != LKS_STATUS_OK) return 6;
            if (((values[a] < values[b]) ||
                (values[a] == values[b] && a < b)) != (comparison < 0))
                return 7;
        }
        changed = 0;
        for (i = 0; i < step; ++i) {
            if (!seen[i]) return 4;
            if (strcmp(old_paths[i], now_paths[i]) != 0) ++changed;
        }
        printf("%s,%d,%d,%d", name, step + 1, values[step], changed);
        for (i = 0; i <= step; ++i) {
            if (!seen[i]) return 5;
            printf(",%s", now_paths[i]);
            strcpy(old_paths[i], now_paths[i]);
        }
        putchar('\n');
    }
    lks_ordered_tree_destroy(tree);
    return 0;
}

int main(void)
{
    int mixed[] = {20, 10, 20, 30, 15, 20, 11, 12, 13, 14};
    int hotspot[25];
    int i, result;
    result = run("mixed", mixed, (int)(sizeof mixed / sizeof mixed[0]));
    if (result) return result;
    hotspot[0] = 0; hotspot[1] = 100;
    for (i = 2; i < 25; ++i) hotspot[i] = 50;
    return run("hotspot", hotspot, 25);
}
