#include <stdio.h>
#include <string.h>
#include "layerkeysort.h"

typedef struct Item { int id; const char *name; } Item;

static int sign_of(int n) { return (n > 0) - (n < 0); }

static int key_of(const LksPath *path, char key[128])
{
    size_t length = lks_path_order_key_length(path);
    return length != 0 && length < 128 &&
        lks_path_order_key_format(path, key, 128) == LKS_STATUS_OK;
}

int main(void)
{
    Item items[4] = {{101, "Background"}, {202, "Player"},
        {303, "HUD"}, {404, "Effects"}};
    LksPath *paths[4] = {NULL, NULL, NULL, NULL};
    LksTree *tree = NULL;
    const LksTreeNode *node = NULL;
    char keys[4][128] = {{0}}, old_key[128], display_a[128], display_b[128];
    int expected[3] = {404, 101, 202};
    int cmp = 0, ok = 0;
    void *removed = NULL;
    size_t i;
#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "consumer failure at line %d\n", __LINE__); goto cleanup; \
} } while (0)

    paths[0] = lks_path_create(LKS_DIRECTION_POSITIVE, 0);
    paths[2] = lks_path_create(LKS_DIRECTION_POSITIVE, 200);
    paths[3] = lks_path_create(LKS_DIRECTION_POSITIVE, 300);
    tree = lks_tree_create();
    REQUIRE(paths[0] && paths[2] && paths[3] && tree);
    REQUIRE(lks_tree_insert(tree, paths[0], &items[0], NULL) == LKS_STATUS_OK);
    REQUIRE(lks_tree_insert(tree, paths[2], &items[2], NULL) == LKS_STATUS_OK);
    REQUIRE(lks_tree_insert(tree, paths[3], &items[3], NULL) == LKS_STATUS_OK);
    REQUIRE(lks_path_between(paths[0], paths[2], &paths[1]) == LKS_STATUS_OK);
    REQUIRE(lks_tree_insert(tree, paths[1], &items[1], NULL) == LKS_STATUS_OK);
    REQUIRE(key_of(paths[3], old_key));

    {   /* Intentional move: business identity and pointer survive coordinate change. */
        LksPath *new_path = NULL;
        REQUIRE(lks_path_before(paths[0], &new_path) == LKS_STATUS_OK);
        REQUIRE(lks_tree_rekey(tree, paths[3], new_path, &node) == LKS_STATUS_OK);
        REQUIRE(node && lks_tree_node_item(node) == &items[3] && items[3].id == 404);
        lks_path_destroy(paths[3]);
        paths[3] = new_path;
        REQUIRE(key_of(paths[3], keys[3]) && strcmp(old_key, keys[3]) != 0);
    }
    REQUIRE(lks_tree_remove_path(tree, paths[2], &removed) == LKS_STATUS_OK);
    REQUIRE(removed == &items[2] && items[2].id == 303);
    REQUIRE(lks_tree_size(tree) == 3);

    /* Detectable misuse leaves the surviving Tree intact. */
    REQUIRE(lks_tree_insert(tree, paths[0], &items[2], &node) ==
        LKS_STATUS_ALREADY_EXISTS && node == NULL);
    REQUIRE(lks_tree_remove_path(tree, paths[2], &removed) ==
        LKS_STATUS_NOT_FOUND && removed == NULL);
    REQUIRE(lks_tree_find_path(tree, paths[2], &node) ==
        LKS_STATUS_NOT_FOUND && node == NULL);
    REQUIRE(lks_tree_rekey(tree, paths[0], paths[1], &node) ==
        LKS_STATUS_ALREADY_EXISTS && node == NULL);
    REQUIRE(lks_path_create(LKS_DIRECTION_ZERO, 0) == NULL);
    REQUIRE(lks_path_create(LKS_DIRECTION_POSITIVE, 65536) == NULL);
    REQUIRE(lks_tree_size(tree) == 3);

    {   /* Display lexicographic order differs at the negative/ZERO boundary. */
        LksPath *zero = lks_path_create_zero();
        LksPath *negative = lks_path_create(LKS_DIRECTION_NEGATIVE, 0);
        char ka[128], kb[128];
        REQUIRE(zero && negative);
        REQUIRE(lks_path_format(negative, display_a, sizeof(display_a)) == LKS_STATUS_OK);
        REQUIRE(lks_path_format(zero, display_b, sizeof(display_b)) == LKS_STATUS_OK);
        REQUIRE(lks_path_compare(negative, zero, &cmp) == LKS_STATUS_OK && cmp < 0);
        REQUIRE(sign_of(strcmp(display_a, display_b)) != sign_of(cmp));
        REQUIRE(key_of(negative, ka) && key_of(zero, kb));
        REQUIRE(sign_of(strcmp(ka, kb)) == sign_of(cmp));
        lks_path_destroy(negative);
        lks_path_destroy(zero);
    }

    for (i = 0; i < 4; ++i) {
        if (i == 2) continue;
        REQUIRE(key_of(paths[i], keys[i]));
    }
    lks_tree_destroy(tree); tree = NULL;
    for (i = 0; i < 4; ++i) { lks_path_destroy(paths[i]); paths[i] = NULL; }

    tree = lks_tree_create();
    REQUIRE(tree != NULL);
    for (i = 0; i < 4; ++i) {
        if (i == 2) continue;
        REQUIRE(lks_path_order_key_parse(keys[i], &paths[i]) == LKS_STATUS_OK);
        REQUIRE(lks_tree_insert(tree, paths[i], &items[i], NULL) == LKS_STATUS_OK);
        REQUIRE(lks_tree_find_path(tree, paths[i], &node) == LKS_STATUS_OK);
        REQUIRE(lks_tree_node_item(node) == &items[i]);
    }
    REQUIRE(lks_tree_size(tree) == 3);
    {   const size_t order[3] = {3, 0, 1};
        for (i = 0; i < 3; ++i) REQUIRE(items[order[i]].id == expected[i]);
        for (i = 1; i < 3; ++i) {
            REQUIRE(lks_path_compare(paths[order[i - 1]], paths[order[i]], &cmp)
                == LKS_STATUS_OK && cmp < 0);
            REQUIRE(strcmp(keys[order[i - 1]], keys[order[i]]) < 0);
        }
    }
    ok = 1;
cleanup:
    lks_tree_destroy(tree);
    for (i = 0; i < 4; ++i) lks_path_destroy(paths[i]);
    if (ok) puts("Consumer LK1 persistence/rekey/identity PASS");
    return ok ? 0 : 1;
#undef REQUIRE
}
