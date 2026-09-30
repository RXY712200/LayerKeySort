#include <stdio.h>
#include "layerkeysort.h"

typedef struct Layer {
    unsigned int id;       /* Application identity survives Path changes. */
    const char *name;
    LksPath *path;         /* Application-owned ordering coordinate. */
    int active;
} Layer;

static int show_order(const char *label, const LksTree *tree,
    const Layer *layers, size_t layer_count)
{
    const Layer *ordered[4];
    size_t count = 0, i, j;
    for (i = 0; i < layer_count; ++i) {
        const LksTreeNode *found = NULL;
        if (!layers[i].active) continue;
        if (lks_tree_find_path(tree, layers[i].path, &found) != LKS_STATUS_OK ||
            found == NULL || lks_tree_node_item(found) != &layers[i]) return 0;
        j = count;
        while (j != 0) {
            int comparison = 0;
            if (lks_path_compare(layers[i].path, ordered[j - 1]->path,
                &comparison) != LKS_STATUS_OK) return 0;
            if (comparison >= 0) break;
            ordered[j] = ordered[j - 1];
            --j;
        }
        ordered[j] = &layers[i];
        ++count;
    }
    if (count != lks_tree_size(tree)) return 0;
    printf("%s:", label);
    for (i = 0; i < count; ++i)
        printf(" %s(id=%u)", ordered[i]->name, ordered[i]->id);
    putchar('\n');
    return 1;
}

int main(void)
{
    Layer layers[4] = {
        {101u, "Background", NULL, 0},
        {202u, "Player", NULL, 0},
        {303u, "HUD", NULL, 0},
        {404u, "Effects", NULL, 0}
    };
    LksTree *tree = lks_tree_create();
    LksPath *moved = NULL, *decoded = NULL;
    char display[64], key[128];
    void *removed = NULL;
    int comparison = 1, ok = 0;
    size_t i;
    if (tree == NULL) goto done;

    layers[0].path = lks_path_create(LKS_DIRECTION_POSITIVE, 1000u);
    layers[1].path = lks_path_create(LKS_DIRECTION_POSITIVE, 2000u);
    layers[2].path = lks_path_create(LKS_DIRECTION_POSITIVE, 3000u);
    for (i = 0; i < 3; ++i) {
        if (layers[i].path == NULL || lks_tree_insert(tree, layers[i].path,
            &layers[i], NULL) != LKS_STATUS_OK) goto done;
        layers[i].active = 1;
    }
    if (!show_order("Initial", tree, layers, 4)) goto done;

    /* Generate a coordinate between two existing application layers. */
    if (lks_path_between(layers[0].path, layers[1].path,
        &layers[3].path) != LKS_STATUS_OK ||
        lks_tree_insert(tree, layers[3].path, &layers[3], NULL) !=
            LKS_STATUS_OK) goto done;
    layers[3].active = 1;
    if (!show_order("Insert Effects", tree, layers, 4)) goto done;

    /* HUD retains id 303 while its Path coordinate changes. */
    if (lks_path_between(layers[3].path, layers[1].path, &moved) !=
            LKS_STATUS_OK ||
        lks_tree_rekey(tree, layers[2].path, moved, NULL) != LKS_STATUS_OK)
        goto done;
    lks_path_destroy(layers[2].path);
    layers[2].path = moved;
    moved = NULL;
    if (!show_order("Move HUD", tree, layers, 4)) goto done;

    if (lks_tree_remove_path(tree, layers[3].path, &removed) != LKS_STATUS_OK ||
        removed != &layers[3]) goto done;
    layers[3].active = 0;
    if (!show_order("Remove Effects", tree, layers, 4)) goto done;

    if (lks_path_format(layers[2].path, display, sizeof display) !=
            LKS_STATUS_OK ||
        lks_path_order_key_format(layers[2].path, key, sizeof key) !=
            LKS_STATUS_OK ||
        lks_path_order_key_parse(key, &decoded) != LKS_STATUS_OK ||
        lks_path_compare(layers[2].path, decoded, &comparison) !=
            LKS_STATUS_OK || comparison != 0) goto done;
    printf("Persist HUD id=%u display=%s key=%s roundtrip=OK\n",
        layers[2].id, display, key);
    ok = 1;

done:
    lks_path_destroy(decoded);
    lks_path_destroy(moved);
    for (i = 0; i < 4; ++i) lks_path_destroy(layers[i].path);
    lks_tree_destroy(tree); /* Does not free the caller-owned Layer objects. */
    if (!ok) fputs("Layer-list example failed\n", stderr);
    return ok ? 0 : 1;
}
