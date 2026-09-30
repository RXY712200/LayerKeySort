#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "layerkeysort.h"
#include "mutation.h"
#include "../src/lks_alloc_internal.h"
#include "../src/lks_tree_internal.h"

typedef struct ModelEntry {
    LksPath *path;
    void *item;
} ModelEntry;

static uint32_t random_next(uint32_t *state)
{
    uint32_t value = *state;
    value ^= value << 13; value ^= value >> 17; value ^= value << 5;
    *state = value;
    return value;
}

static LksPath *key_path(unsigned int key)
{
    if (key == 256u) return lks_path_create_zero();
    return lks_path_create(key < 256u ? LKS_DIRECTION_NEGATIVE :
        LKS_DIRECTION_POSITIVE, key < 256u ? key : key - 257u);
}

static int same_path(const LksPath *a, const LksPath *b)
{
    int order = 1;
    return lks_path_compare(a, b, &order) == LKS_STATUS_OK && order == 0;
}

static int clean(void)
{
    LksAllocStats stats = lks_alloc_stats_get();
    return stats.live_bytes == 0 && stats.live_blocks == 0 &&
        stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes == 0;
}

static int compare_int(const void *left, const void *right, void *context)
{
    int a = *(const int *)left, b = *(const int *)right;
    (void)context;
    return (a > b) - (a < b);
}

static int verify(LksTree *tree, const ModelEntry *model, size_t count)
{
    const LksTreeNode *nodes[128];
    LksTreeInternalProfile profile;
    size_t i, j;
    if (count > 128 || lks_tree_size(tree) != count ||
        lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
        !profile.balance_valid || profile.real_node_count != count ||
        lks_tree_internal_fill_ordered(tree, nodes, count) != LKS_STATUS_OK ||
        lks_tree_root_child_count(tree) != (count != 0)) return 0;
    for (i = 0; i < count; ++i) {
        const LksTreeNode *found = NULL;
        int order = 0;
        if (lks_tree_find_path(tree, model[i].path, &found) != LKS_STATUS_OK ||
            found == NULL || lks_tree_node_item(found) != model[i].item) return 0;
        for (j = 0; j < count; ++j)
            if (nodes[j] == found) break;
        if (j == count) return 0;
        if (i != 0 && (lks_path_compare(lks_tree_node_path(nodes[i - 1]),
            lks_tree_node_path(nodes[i]), &order) != LKS_STATUS_OK ||
            order >= 0)) return 0;
    }
    return 1;
}

static void model_sort(ModelEntry *model, size_t count)
{
    size_t i;
    for (i = 1; i < count; ++i) {
        ModelEntry entry = model[i];
        size_t j = i;
        int order;
        while (j != 0 && lks_path_compare(entry.path, model[j - 1].path,
            &order) == LKS_STATUS_OK && order < 0) {
            model[j] = model[j - 1];
            --j;
        }
        model[j] = entry;
    }
}

static size_t model_find(const ModelEntry *model, size_t count,
    const LksPath *path)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (same_path(model[i].path, path)) return i;
    return count;
}

static int check_edges(void)
{
    LksTree *tree = NULL;
    LksPath *paths[8] = {NULL};
    int items[8] = {0,1,2,3,4,5,6,7};
    size_t i;
    int ok = 1;
    void *removed = &items[0];
    const LksTreeNode *node = (const LksTreeNode *)tree;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    paths[0] = lks_path_create_zero();
    paths[1] = lks_path_create(LKS_DIRECTION_NEGATIVE, 4);
    paths[2] = lks_path_create(LKS_DIRECTION_POSITIVE, 4);
    paths[3] = lks_path_create(LKS_DIRECTION_NEGATIVE, 2);
    paths[4] = lks_path_create(LKS_DIRECTION_NEGATIVE, 6);
    paths[5] = lks_path_create(LKS_DIRECTION_POSITIVE, 2);
    paths[6] = lks_path_create(LKS_DIRECTION_POSITIVE, 6);
    paths[7] = lks_path_create(LKS_DIRECTION_POSITIVE, 8);
    if (tree == NULL) ok = 0;
    for (i = 0; i < 8; ++i) if (paths[i] == NULL) ok = 0;
    if (!ok) goto done;
    if (lks_tree_remove_path(tree, paths[0], &removed) != LKS_STATUS_NOT_FOUND ||
        removed != NULL ||
        lks_tree_remove_path(NULL, paths[0], &removed) !=
            LKS_STATUS_INVALID_ARGUMENT || removed != NULL ||
        lks_tree_remove_path(tree, NULL, NULL) !=
            LKS_STATUS_INVALID_ARGUMENT) ok = 0;
    for (i = 0; i < 8 && ok; ++i)
        if (lks_tree_insert(tree, paths[i], &items[i], NULL) != LKS_STATUS_OK)
            ok = 0;
    if (!ok) goto done;
    if (lks_tree_rekey(NULL, paths[0], paths[1], &node) !=
            LKS_STATUS_INVALID_ARGUMENT || node != NULL ||
        lks_tree_rekey(tree, NULL, paths[1], &node) !=
            LKS_STATUS_INVALID_ARGUMENT || node != NULL ||
        lks_tree_rekey(tree, paths[0], NULL, &node) !=
            LKS_STATUS_INVALID_ARGUMENT || node != NULL ||
        lks_tree_rekey(tree, paths[0], paths[1], &node) !=
            LKS_STATUS_ALREADY_EXISTS || node != NULL ||
        lks_tree_rekey(tree, paths[0], paths[0], &node) != LKS_STATUS_OK ||
        node == NULL || lks_tree_node_item(node) != &items[0]) ok = 0;
    lks_alloc_test_fail_on_attempt(1);
    if (lks_tree_remove_path(tree, paths[0], &removed) != LKS_STATUS_OK ||
        removed != &items[0] || lks_alloc_test_get_attempt_count() != 0)
        ok = 0;
    lks_alloc_test_disable_failure();
    if (lks_tree_size(tree) != 7 ||
        lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
            LKS_STATUS_OK) ok = 0;
    /* Remove physical roots repeatedly, covering successor transplant and
     * rotations until the last node is gone. */
    while (lks_tree_size(tree) != 0 && ok) {
        const LksTreeNode *root = lks_tree_root_child_at(tree, 0);
        LksPath *key = lks_path_clone(lks_tree_node_path(root));
        void *item = lks_tree_node_item(root);
        if (key == NULL || lks_tree_remove_path(tree, key, &removed) !=
            LKS_STATUS_OK || removed != item ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK) ok = 0;
        lks_path_destroy(key);
    }
done:
    for (i = 0; i < 8; ++i) lks_path_destroy(paths[i]);
    lks_tree_destroy(tree);
    lks_alloc_test_disable_failure();
    return ok && clean();
}

static int check_rekey_shapes(void)
{
    size_t count, i;
    int ok = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    for (count = 1; count <= 7 && ok; ++count) {
        LksTree *tree = lks_tree_create();
        LksPath *keys[7] = {NULL};
        LksPath *target = key_path(count % 2u ? 10u : 500u);
        const LksTreeNode *root, *moved = NULL, *found = NULL;
        int items[7] = {0,1,2,3,4,5,6};
        LksPath *old_copy = NULL;
        void *item = NULL;
        if (tree == NULL || target == NULL) ok = 0;
        for (i = 0; i < count && ok; ++i) {
            keys[i] = key_path((unsigned int)(260u + i));
            if (keys[i] == NULL ||
                lks_tree_insert(tree, keys[i], &items[i], NULL) !=
                    LKS_STATUS_OK) ok = 0;
        }
        root = lks_tree_root_child_at(tree, 0);
        if (root != NULL) {
            old_copy = lks_path_clone(lks_tree_node_path(root));
            item = lks_tree_node_item(root);
        }
        if (ok && (old_copy == NULL ||
            lks_tree_rekey(tree, old_copy, target, &moved) != LKS_STATUS_OK ||
            moved == NULL || lks_tree_node_item(moved) != item ||
            lks_tree_size(tree) != count ||
            lks_tree_find_path(tree, old_copy, &found) !=
                LKS_STATUS_NOT_FOUND ||
            lks_tree_find_path(tree, target, &found) != LKS_STATUS_OK ||
            found != moved ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK)) ok = 0;
        for (i = 0; i < count; ++i) lks_path_destroy(keys[i]);
        lks_path_destroy(old_copy);
        lks_path_destroy(target);
        lks_tree_destroy(tree);
    }
    /* Keep comparator order compatible across explicit rekey and subsequent
     * comparator-driven insertion. */
    if (ok) {
        LksTree *tree = lks_tree_create();
        LksPath *a = key_path(270), *b = key_path(280);
        LksPath *new_b = key_path(290);
        int low = 1, high = 3, middle = 2;
        LksComparator comparator = {compare_int, NULL};
        const LksTreeNode *node = NULL;
        if (tree == NULL || a == NULL || b == NULL || new_b == NULL ||
            lks_tree_insert(tree, a, &low, NULL) != LKS_STATUS_OK ||
            lks_tree_insert(tree, b, &high, NULL) != LKS_STATUS_OK ||
            lks_tree_rekey(tree, b, new_b, &node) != LKS_STATUS_OK ||
            node == NULL || lks_tree_node_item(node) != &high ||
            lks_tree_insert_item(tree, &middle, &comparator, &node) !=
                LKS_STATUS_OK || node == NULL ||
            lks_tree_node_item(node) != &middle ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK) ok = 0;
        lks_path_destroy(a); lks_path_destroy(b); lks_path_destroy(new_b);
        lks_tree_destroy(tree);
    }
    printf("V2 RekeyShapesAndComparator %s\n", ok ? "PASS" : "FAIL");
    return ok && clean();
}

static int check_delete_shapes(void)
{
    size_t degree;
    int ok = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    for (degree = 0; degree <= 2 && ok; ++degree) {
        size_t count = degree + 1, i, selected = count;
        LksTree *tree = lks_tree_create();
        LksPath *keys[3] = {NULL,NULL,NULL};
        const LksTreeNode *nodes[3];
        int items[3] = {1,2,3};
        void *removed = NULL;
        if (tree == NULL) ok = 0;
        for (i = 0; i < count && ok; ++i) {
            keys[i] = key_path((unsigned int)(270u + i));
            if (keys[i] == NULL ||
                lks_tree_insert(tree, keys[i], &items[i], NULL) !=
                    LKS_STATUS_OK) ok = 0;
        }
        if (ok && lks_tree_internal_fill_ordered(tree, nodes, count) !=
            LKS_STATUS_OK) ok = 0;
        for (i = 0; i < count && ok; ++i) {
            size_t j;
            if (lks_tree_node_child_count(nodes[i]) != degree) continue;
            for (j = 0; j < count; ++j)
                if (same_path(keys[j], lks_tree_node_path(nodes[i]))) break;
            selected = j;
            break;
        }
        if (ok && (selected == count ||
            lks_tree_remove_path(tree, keys[selected], &removed) !=
                LKS_STATUS_OK || removed != &items[selected] ||
            lks_tree_size(tree) != count - 1 ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK)) ok = 0;
        for (i = 0; i < count && ok; ++i) {
            const LksTreeNode *found = NULL;
            LksStatus expected = i == selected ? LKS_STATUS_NOT_FOUND :
                LKS_STATUS_OK;
            if (lks_tree_find_path(tree, keys[i], &found) != expected ||
                (i != selected && lks_tree_node_item(found) != &items[i]))
                ok = 0;
        }
        for (i = 0; i < count; ++i) lks_path_destroy(keys[i]);
        lks_tree_destroy(tree);
    }
    printf("V2 DeleteLeafUnaryBranch %s\n", ok ? "PASS" : "FAIL");
    return ok && clean();
}

static int check_rekey_transitions(void)
{
    static const unsigned int positions[] = {1u, 2u, 256u, 300u, 3u, 301u};
    LksPath *paths[sizeof(positions) / sizeof(positions[0])] = {NULL};
    LksTree *tree;
    const LksTreeNode *node = NULL;
    int item = 17, ok = 1;
    size_t i;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        paths[i] = key_path(positions[i]);
        if (paths[i] == NULL) ok = 0;
    }
    if (tree == NULL) ok = 0;
    if (ok && lks_tree_insert(tree, paths[0], &item, NULL) != LKS_STATUS_OK)
        ok = 0;
    for (i = 1; i < sizeof(paths) / sizeof(paths[0]) && ok; ++i) {
        const LksTreeNode *found = NULL;
        if (lks_tree_rekey(tree, paths[i - 1], paths[i], &node) !=
            LKS_STATUS_OK || node == NULL ||
            lks_tree_node_item(node) != &item || lks_tree_size(tree) != 1 ||
            lks_tree_find_path(tree, paths[i - 1], &found) !=
                LKS_STATUS_NOT_FOUND ||
            lks_tree_find_path(tree, paths[i], &found) != LKS_STATUS_OK ||
            found != node ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK) ok = 0;
    }
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
        lks_path_destroy(paths[i]);
    lks_tree_destroy(tree);
    printf("V2 RekeyTransitions %s\n", ok ? "PASS" : "FAIL");
    return ok && clean();
}

static int check_random(uint32_t seed)
{
    enum { STEPS = 2400, CAP = 96 };
    ModelEntry model[CAP];
    int items[STEPS];
    LksTree *tree;
    size_t count = 0, step, i;
    uint32_t original_seed = seed;
    int ok = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    for (step = 0; step < STEPS && ok; ++step) {
        unsigned int a = random_next(&seed) % 512u;
        unsigned int b = random_next(&seed) % 512u;
        unsigned int operation = random_next(&seed) % 3u;
        LksPath *old = key_path(a), *new_key = key_path(b);
        size_t old_index, new_index;
        if (old == NULL || new_key == NULL) { ok = 0; lks_path_destroy(old);
            lks_path_destroy(new_key); break; }
        old_index = model_find(model, count, old);
        new_index = model_find(model, count, new_key);
        if (operation == 0 || count == CAP) {
            void *removed = &items[step];
            LksStatus expected = old_index == count ? LKS_STATUS_NOT_FOUND :
                LKS_STATUS_OK;
            LksStatus actual = lks_tree_remove_path(tree, old, &removed);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                removed != model[old_index].item) ||
                (actual != LKS_STATUS_OK && removed != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK) {
                lks_path_destroy(model[old_index].path);
                model[old_index] = model[--count];
            }
        } else if (operation == 1) {
            const LksTreeNode *node = NULL;
            LksStatus actual = lks_tree_insert(tree, old, &items[step], &node);
            LksStatus expected = old_index == count ? LKS_STATUS_OK :
                LKS_STATUS_ALREADY_EXISTS;
            if (actual != expected || (actual == LKS_STATUS_OK &&
                (node == NULL || lks_tree_node_item(node) != &items[step])) ||
                (actual != LKS_STATUS_OK && node != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK) {
                model[count].path = lks_path_clone(old);
                model[count++].item = &items[step];
                if (model[count - 1].path == NULL) ok = 0;
            }
        } else {
            const LksTreeNode *node = NULL;
            LksStatus expected = old_index == count ? LKS_STATUS_NOT_FOUND :
                (new_index != count && new_index != old_index ?
                LKS_STATUS_ALREADY_EXISTS : LKS_STATUS_OK);
            LksStatus actual = lks_tree_rekey(tree, old, new_key, &node);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                (node == NULL || lks_tree_node_item(node) !=
                model[old_index].item)) ||
                (actual != LKS_STATUS_OK && node != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK && old_index != new_index) {
                LksPath *copy = lks_path_clone(new_key);
                if (copy == NULL) ok = 0;
                else { lks_path_destroy(model[old_index].path);
                    model[old_index].path = copy; }
            }
        }
        lks_path_destroy(old); lks_path_destroy(new_key);
        model_sort(model, count);
        if (ok && !verify(tree, model, count)) ok = 0;
    }
    /* Drain in pseudo-random order, checking AVL invariants after every
     * deletion including the final root replacement. */
    while (count != 0 && ok) {
        size_t selected = random_next(&seed) % count;
        void *removed = NULL;
        if (lks_tree_remove_path(tree, model[selected].path, &removed) !=
            LKS_STATUS_OK || removed != model[selected].item) ok = 0;
        lks_path_destroy(model[selected].path);
        model[selected] = model[--count];
        model_sort(model, count);
        if (ok && !verify(tree, model, count)) ok = 0;
    }
    for (i = 0; i < count; ++i) lks_path_destroy(model[i].path);
    lks_tree_destroy(tree);
    printf("V2 MutationDifferential Seed=%08X Steps=%zu %s\n",
        (unsigned int)original_seed, step, ok ? "PASS" : "FAIL");
    return ok && clean();
}

static int check_external_round_trip(const LksPath *path)
{
    size_t display_length = lks_path_text_length(path);
    size_t key_length = lks_path_order_key_length(path);
    char *display = NULL, *key = NULL;
    LksPath *from_display = NULL, *from_key = NULL;
    int good = 0;
    if (display_length == 0 || key_length == 0 ||
        display_length == SIZE_MAX || key_length == SIZE_MAX) return 0;
    display = (char *)malloc(display_length + 1);
    key = (char *)malloc(key_length + 1);
    if (display != NULL && key != NULL &&
        lks_path_format(path, display, display_length + 1) == LKS_STATUS_OK &&
        lks_path_order_key_format(path, key, key_length + 1) == LKS_STATUS_OK &&
        lks_path_parse(display, &from_display) == LKS_STATUS_OK &&
        lks_path_order_key_parse(key, &from_key) == LKS_STATUS_OK &&
        same_path(path, from_display) && same_path(path, from_key)) good = 1;
    lks_path_destroy(from_display); lks_path_destroy(from_key);
    free(display); free(key);
    return good;
}

/* A flat Path-sorted model deliberately ignores physical AVL topology. Item
 * values are reset to sorted ranks after arbitrary explicit insert/rekey, so
 * each later comparator insertion starts with a compatible Tree ordering. */
static int check_mixed_interaction(uint32_t seed)
{
    enum { STEPS = 600, CAP = 96 };
    ModelEntry model[CAP];
    int items[STEPS];
    LksComparator comparator = {compare_int, NULL};
    LksTree *tree;
    size_t count = 0, step, i;
    uint32_t original_seed = seed;
    int ok = 1;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    if (tree == NULL) return 0;
    for (step = 0; step < STEPS && ok; ++step) {
        unsigned int operation = random_next(&seed) % 5u;
        unsigned int code = random_next(&seed) % 513u;
        LksPath *candidate;
        size_t index;
        if (count == CAP) operation = 0;
        if ((operation == 0 || operation == 2) && count != 0 &&
            random_next(&seed) % 4u != 0)
            code = 513u;
        candidate = code == 513u ? NULL : key_path(code);
        if (code == 513u) {
            index = random_next(&seed) % count;
            candidate = lks_path_clone(model[index].path);
        }
        if (candidate == NULL) { ok = 0; break; }
        index = model_find(model, count, candidate);
        if (operation == 0) {
            void *removed = (void *)(uintptr_t)1;
            LksStatus expected = index == count ? LKS_STATUS_NOT_FOUND : LKS_STATUS_OK;
            LksStatus actual = lks_tree_remove_path(tree, candidate, &removed);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                removed != model[index].item) ||
                (actual != LKS_STATUS_OK && removed != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK) {
                lks_path_destroy(model[index].path);
                model[index] = model[--count];
            }
        } else if (operation == 1) {
            const LksTreeNode *node = NULL;
            LksStatus expected = index == count ? LKS_STATUS_OK :
                LKS_STATUS_ALREADY_EXISTS;
            LksStatus actual = lks_tree_insert(tree, candidate, &items[step], &node);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                (node == NULL || lks_tree_node_item(node) != &items[step])) ||
                (actual != LKS_STATUS_OK && node != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK) {
                model[count].path = lks_path_clone(candidate);
                model[count++].item = &items[step];
                if (model[count - 1].path == NULL) ok = 0;
            }
        } else if (operation == 2) {
            LksPath *target = key_path(random_next(&seed) % 513u);
            const LksTreeNode *node = NULL;
            size_t target_index;
            LksStatus expected, actual;
            if (target == NULL) { ok = 0; lks_path_destroy(candidate); break; }
            target_index = model_find(model, count, target);
            expected = index == count ? LKS_STATUS_NOT_FOUND :
                (target_index != count && target_index != index ?
                LKS_STATUS_ALREADY_EXISTS : LKS_STATUS_OK);
            actual = lks_tree_rekey(tree, candidate, target, &node);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                (node == NULL || lks_tree_node_item(node) != model[index].item)) ||
                (actual != LKS_STATUS_OK && node != NULL)) ok = 0;
            if (actual == LKS_STATUS_OK && index != target_index) {
                LksPath *copy = lks_path_clone(target);
                if (copy == NULL) ok = 0;
                else { lks_path_destroy(model[index].path); model[index].path = copy; }
            }
            lks_path_destroy(target);
        } else if (operation == 3) {
            const LksTreeNode *node = NULL;
            const LksTreeNode *ordered_nodes[CAP];
            LksPath *replacement_paths[CAP] = {0};
            void *expected_items[CAP];
            size_t expected_position = 0;
            LksStatus actual;
            items[step] = (int)(random_next(&seed) % (count / 3u + 2u));
            while (expected_position < count &&
                *(int *)model[expected_position].item <= items[step])
                ++expected_position;
            actual = lks_tree_insert_item(tree, &items[step], &comparator, &node);
            if (actual != LKS_STATUS_OK || node == NULL ||
                lks_tree_node_item(node) != &items[step] ||
                lks_tree_internal_fill_ordered(tree, ordered_nodes, count + 1) !=
                    LKS_STATUS_OK) ok = 0;
            if (ok) {
                for (i = 0; i <= count; ++i) {
                    expected_items[i] = i < expected_position ? model[i].item :
                        i == expected_position ? &items[step] : model[i - 1].item;
                    if (lks_tree_node_item(ordered_nodes[i]) != expected_items[i])
                        ok = 0;
                }
                for (i = 0; i <= count && ok; ++i) {
                    replacement_paths[i] = lks_path_clone(
                        lks_tree_node_path(ordered_nodes[i]));
                    if (replacement_paths[i] == NULL) ok = 0;
                }
                if (ok) {
                    for (i = 0; i < count; ++i) lks_path_destroy(model[i].path);
                    for (i = 0; i <= count; ++i) {
                        model[i].path = replacement_paths[i];
                        model[i].item = expected_items[i];
                    }
                    ++count;
                } else for (i = 0; i <= count; ++i)
                    lks_path_destroy(replacement_paths[i]);
            }
        } else {
            const LksTreeNode *node = NULL;
            LksStatus expected = index == count ? LKS_STATUS_NOT_FOUND : LKS_STATUS_OK;
            LksStatus actual = lks_tree_find_path(tree, candidate, &node);
            if (actual != expected || (actual == LKS_STATUS_OK &&
                (node == NULL || lks_tree_node_item(node) != model[index].item)) ||
                (actual != LKS_STATUS_OK && node != NULL)) ok = 0;
        }
        lks_path_destroy(candidate);
        model_sort(model, count);
        for (i = 0; i < count; ++i) *(int *)model[i].item = (int)(i / 3u);
        if (ok && !verify(tree, model, count)) ok = 0;
        if (ok && count != 0 &&
            !check_external_round_trip(model[step % count].path)) ok = 0;
        if (!ok) fprintf(stderr, "MixedInteraction failure step=%zu operation=%u count=%zu seed=%08X\n",
            step, operation, count, (unsigned int)original_seed);
    }
    for (i = 0; i < count; ++i) lks_path_destroy(model[i].path);
    lks_tree_destroy(tree);
    printf("V2 MixedInteraction Seed=%08X Steps=%zu %s\n",
        (unsigned int)original_seed, step, ok ? "PASS" : "FAIL");
    return ok && clean();
}

static int check_rekey_oom(void)
{
    LksTree *tree;
    LksPath *old, *target;
    int item = 42, ok = 1, success = 0;
    size_t index, failures = 0;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    old = key_path(250); target = key_path(300);
    if (tree == NULL || old == NULL || target == NULL ||
        lks_tree_insert(tree, old, &item, NULL) != LKS_STATUS_OK) ok = 0;
    for (index = 1; index < 20 && ok; ++index) {
        LksAllocStats before = lks_alloc_stats_get();
        const LksTreeNode *node = (const LksTreeNode *)tree, *found = NULL;
        LksStatus status;
        lks_alloc_test_fail_on_attempt(index);
        status = lks_tree_rekey(tree, old, target, &node);
        lks_alloc_test_disable_failure();
        if (status == LKS_STATUS_OK) {
            success = node != NULL && lks_tree_node_item(node) == &item &&
                lks_tree_find_path(tree, old, &found) == LKS_STATUS_NOT_FOUND &&
                lks_tree_find_path(tree, target, &found) == LKS_STATUS_OK &&
                found == node;
            break;
        }
        if (status != LKS_STATUS_OUT_OF_MEMORY || node != NULL ||
            lks_tree_size(tree) != 1 ||
            lks_tree_find_path(tree, old, &found) != LKS_STATUS_OK ||
            lks_tree_node_item(found) != &item ||
            lks_tree_find_path(tree, target, &found) != LKS_STATUS_NOT_FOUND ||
            lks_alloc_stats_get().live_bytes != before.live_bytes ||
            lks_alloc_stats_get().live_blocks != before.live_blocks ||
            lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
                LKS_STATUS_OK) ok = 0;
        ++failures;
    }
    printf("V2 RekeyOOM FailPoints=%zu %s\n", failures,
        ok && success && failures >= 2 ? "PASS" : "FAIL");
    lks_path_destroy(old); lks_path_destroy(target);
    lks_tree_destroy(tree);
    return ok && success && failures >= 2 && clean();
}

static int check_deep(size_t depth)
{
    LksTree *tree;
    LksPath *shallow, *deep;
    const LksTreeNode *node = NULL;
    int item = 1, ok = 1;
    size_t i;
    if (lks_alloc_stats_reset() != 0) return 0;
    tree = lks_tree_create();
    shallow = key_path(300);
    deep = key_path(301);
    if (tree == NULL || shallow == NULL || deep == NULL) ok = 0;
    for (i = 1; i < depth && ok; ++i)
        if (lks_path_append(deep, 1) != LKS_STATUS_OK) ok = 0;
    if (ok && (lks_tree_insert(tree, shallow, &item, NULL) != LKS_STATUS_OK ||
        lks_tree_rekey(tree, shallow, deep, &node) != LKS_STATUS_OK ||
        node == NULL || lks_tree_node_item(node) != &item ||
        lks_tree_internal_profile(tree, &(LksTreeInternalProfile){0}) !=
            LKS_STATUS_OK ||
        lks_tree_rekey(tree, deep, shallow, &node) != LKS_STATUS_OK ||
        lks_tree_rekey(tree, shallow, deep, &node) != LKS_STATUS_OK ||
        lks_tree_remove_path(tree, deep, NULL) != LKS_STATUS_OK ||
        lks_tree_size(tree) != 0)) ok = 0;
    printf("V2 MutationDeep Depth=%zu %s\n", depth, ok ? "PASS" : "FAIL");
    lks_path_destroy(shallow); lks_path_destroy(deep);
    lks_tree_destroy(tree);
    return ok && clean();
}

int lks_run_mutation_tests(void)
{
    if (!check_edges() || !check_delete_shapes() || !check_rekey_shapes() ||
        !check_rekey_transitions() || !check_rekey_oom() ||
        !check_random(UINT32_C(0xC0FFEE12)) ||
        !check_random(UINT32_C(0x12345678)) ||
        !check_random(UINT32_C(0xDEADBEEF)) ||
        !check_mixed_interaction(UINT32_C(0x41786321)) ||
        !check_mixed_interaction(UINT32_C(0xA76509DE)) ||
        !check_deep(1000) || !check_deep(10000) || !check_deep(100000)) {
        puts("V2 mutation tests FAILED");
        return 1;
    }
    puts("V2 mutation tests PASS");
    return 0;
}
