/* Historical V3 regression using private declarations; not the V4 public consumer. */
#include "lks_legacy_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "RC contract failure at line %d: %s\n", \
        __LINE__, #condition); return 0; \
} } while (0)

static int compare_int(const void *left, const void *right, void *context)
{
    int a = *(const int *)left, b = *(const int *)right;
    (void)context;
    return (a > b) - (a < b);
}

/* These calls have documented error results. A dangling pointer, a
 * comparator that dereferences invalid data, and double destruction remain
 * caller errors; this test deliberately does not invoke undefined behavior. */
static int check_invalid_arguments(void)
{
    LksComparator bad = { NULL, NULL }, good = { compare_int, NULL };
    LksOrderedTree *ordered;
    LksTree *manual;
    LksPath *path = lks_path_create(LKS_DIRECTION_POSITIVE, 42);
    const LksTreeNode *node = (const LksTreeNode *)path;
    const LksTreeNode *left = node, *equal = node, *right = node;
    LksPath *parsed = path;
    void *removed = path;
    int item = 7;
    char buffer[16] = "untouched";
    CHECK(path != NULL);
    CHECK(lks_ordered_tree_create(NULL) == NULL);
    CHECK(lks_ordered_tree_create(&bad) == NULL);
    ordered = lks_ordered_tree_create(&good);
    manual = lks_tree_create();
    CHECK(ordered != NULL && manual != NULL);
    CHECK(lks_ordered_tree_locate(ordered, &item, &left, &equal,
        &right) == LKS_STATUS_OK && !left && !equal && !right);
    CHECK(lks_ordered_tree_locate(ordered, &item, NULL, &equal,
        &right) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_ordered_tree_locate(ordered, &item, &left, &left,
        &right) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_ordered_tree_insert(NULL, &item, &node) ==
        LKS_STATUS_INVALID_ARGUMENT && node == NULL);
    CHECK(lks_tree_insert(NULL, path, &item, &node) ==
        LKS_STATUS_INVALID_ARGUMENT && node == NULL);
    CHECK(lks_tree_insert(manual, NULL, &item, &node) ==
        LKS_STATUS_INVALID_ARGUMENT && node == NULL);
    CHECK(lks_tree_find_path(manual, path, &node) ==
        LKS_STATUS_NOT_FOUND && node == NULL);
    CHECK(lks_tree_find_path(manual, path, NULL) ==
        LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_ordered_tree_find_path(ordered, path, &node) ==
        LKS_STATUS_NOT_FOUND && node == NULL);
    CHECK(lks_tree_remove_path(manual, path, &removed) ==
        LKS_STATUS_NOT_FOUND && removed == NULL);
    CHECK(lks_ordered_tree_remove_path(ordered, path, &removed) ==
        LKS_STATUS_NOT_FOUND && removed == NULL);
    CHECK(lks_tree_insert(manual, path, &item, &node) ==
        LKS_STATUS_OK && node != NULL);
    CHECK(lks_tree_insert(manual, path, &item, &node) ==
        LKS_STATUS_ALREADY_EXISTS && node == NULL);
    CHECK(lks_tree_size(manual) == 1);
    CHECK(lks_path_format(path, buffer, 2) ==
        LKS_STATUS_BUFFER_TOO_SMALL && strcmp(buffer, "untouched") == 0);
    CHECK(lks_path_parse(NULL, &parsed) ==
        LKS_STATUS_INVALID_ARGUMENT && parsed == NULL);
    parsed = path;
    CHECK(lks_path_parse("000/", &parsed) ==
        LKS_STATUS_INVALID_ARGUMENT && parsed == NULL);
    parsed = path;
    CHECK(lks_path_order_key_parse("LK1:1!!", &parsed) ==
        LKS_STATUS_INVALID_ARGUMENT && parsed == NULL);
    CHECK(lks_path_order_key_parse(NULL, NULL) ==
        LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_path_before(path, NULL) == LKS_STATUS_INVALID_ARGUMENT);
    CHECK(lks_sort(NULL, 0, NULL, NULL) == LKS_STATUS_INVALID_ARGUMENT);
    lks_path_destroy(path);
    lks_tree_destroy(manual);
    lks_ordered_tree_destroy(ordered);
    return 1;
}

static int check_create_oom_and_lifecycle(void)
{
    LksComparator comparator = { compare_int, NULL };
    size_t attempt;
    CHECK(lks_alloc_stats_reset() == 0);
    for (attempt = 1; attempt <= 2; ++attempt) {
        LksOrderedTree *tree;
        lks_alloc_test_fail_on_attempt(attempt);
        tree = lks_ordered_tree_create(&comparator);
        CHECK(tree == NULL && lks_alloc_test_failure_triggered());
        lks_alloc_test_disable_failure();
        CHECK(lks_alloc_stats_get().live_blocks == 0);
    }
    for (attempt = 0; attempt < 100; ++attempt) {
        LksTree *manual = lks_tree_create();
        LksOrderedTree *ordered = lks_ordered_tree_create(&comparator);
        CHECK(manual != NULL && ordered != NULL);
        lks_tree_destroy(manual);
        lks_ordered_tree_destroy(ordered);
    }
    CHECK(lks_alloc_stats_get().live_blocks == 0);
    return 1;
}

int main(void)
{
    if (!check_invalid_arguments() || !check_create_oom_and_lifecycle())
        return 1;
    puts("RC documented-input/lifecycle checks PASS");
    return 0;
}
