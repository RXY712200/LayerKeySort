#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"

enum { MODEL_CAPACITY = 48 };

typedef struct SoakItem {
    size_t id;
    int value;
} SoakItem;

typedef struct SoakEntry {
    LksPath *path;
    SoakItem *item;
} SoakEntry;

static uint32_t next_random(uint32_t *state)
{
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static int path_order(const LksPath *left, const LksPath *right)
{
    int result = 0;
    if (lks_path_compare(left, right, &result) != LKS_STATUS_OK) return 2;
    return (result > 0) - (result < 0);
}

static int allocations_clean(void)
{
    LksAllocStats stats = lks_alloc_stats_get();
    return stats.live_bytes == 0 && stats.live_blocks == 0 &&
        !stats.counter_overflowed;
}

static int round_trip(const LksPath *path)
{
    size_t text_size = lks_path_text_length(path);
    size_t key_size = lks_path_order_key_length(path);
    char *text = NULL, *key = NULL;
    LksPath *from_text = NULL, *from_key = NULL;
    int good = 0;
    if (text_size == 0 || key_size == 0 || text_size == SIZE_MAX ||
        key_size == SIZE_MAX) return 0;
    text = (char *)malloc(text_size + 1);
    key = (char *)malloc(key_size + 1);
    if (text != NULL && key != NULL &&
        lks_path_format(path, text, text_size + 1) == LKS_STATUS_OK &&
        lks_path_order_key_format(path, key, key_size + 1) == LKS_STATUS_OK &&
        lks_path_parse(text, &from_text) == LKS_STATUS_OK &&
        lks_path_order_key_parse(key, &from_key) == LKS_STATUS_OK &&
        path_order(path, from_text) == 0 &&
        path_order(path, from_key) == 0 &&
        lks_path_depth(path) == lks_path_depth(from_text) &&
        lks_path_depth(path) == lks_path_depth(from_key)) good = 1;
    lks_path_destroy(from_text);
    lks_path_destroy(from_key);
    free(text);
    free(key);
    return good;
}

static int compare_item(const void *left, const void *right, void *context)
{
    const SoakItem *a = (const SoakItem *)left;
    const SoakItem *b = (const SoakItem *)right;
    (void)context;
    return (a->value > b->value) - (a->value < b->value);
}

static int verify_tree(const LksTree *tree, const SoakEntry *model,
    size_t count, int comparator_order)
{
    const LksTreeNode *nodes[MODEL_CAPACITY];
    LksTreeInternalProfile profile;
    size_t i;
    if (count > MODEL_CAPACITY || lks_tree_size(tree) != count ||
        lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK ||
        !profile.balance_valid || profile.real_node_count != count ||
        lks_tree_internal_fill_ordered(tree, nodes, count) != LKS_STATUS_OK)
        return 0;
    for (i = 0; i < count; ++i) {
        const LksTreeNode *found = NULL;
        if (model[i].path == NULL || model[i].item == NULL ||
            lks_tree_node_item(nodes[i]) != model[i].item ||
            path_order(lks_tree_node_path(nodes[i]), model[i].path) != 0 ||
            lks_tree_find_path(tree, model[i].path, &found) != LKS_STATUS_OK ||
            found != nodes[i]) return 0;
        if (i != 0 && (path_order(model[i - 1].path, model[i].path) >= 0 ||
            (comparator_order && model[i - 1].item->value >
                model[i].item->value))) return 0;
    }
    return 1;
}

static void remove_model(SoakEntry *model, size_t *count, size_t index)
{
    size_t i;
    lks_path_destroy(model[index].path);
    for (i = index + 1; i < *count; ++i) model[i - 1] = model[i];
    --*count;
}

static void insert_model(SoakEntry *model, size_t *count,
    LksPath *path, SoakItem *item)
{
    size_t i = *count;
    while (i != 0 && path_order(path, model[i - 1].path) < 0) {
        model[i] = model[i - 1];
        --i;
    }
    model[i].path = path;
    model[i].item = item;
    ++*count;
}

static LksStatus gap_path(const SoakEntry *model, size_t count,
    uint32_t *seed, LksPath **out)
{
    size_t gap;
    if (count == 0) {
        *out = lks_path_create(LKS_DIRECTION_POSITIVE, 32768u);
        return *out == NULL ? LKS_STATUS_OUT_OF_MEMORY : LKS_STATUS_OK;
    }
    gap = next_random(seed) % 4u == 0 ?
        (size_t)(next_random(seed) % (count + 1)) : count / 2;
    if (gap == 0) return lks_path_before(model[0].path, out);
    if (gap == count) return lks_path_after(model[count - 1].path, out);
    return lks_path_between(model[gap - 1].path, model[gap].path, out);
}

static int run_explicit(size_t steps, uint32_t initial_seed)
{
    SoakEntry model[MODEL_CAPACITY] = {{0}};
    SoakItem *items = NULL;
    LksTree *tree = NULL;
    uint32_t seed = initial_seed;
    size_t count = 0, next_item = 0, step = 0, i;
    int ok = 1;
    if (steps > SIZE_MAX / sizeof *items || lks_alloc_stats_reset() != 0)
        return 0;
    items = (SoakItem *)calloc(steps ? steps : 1, sizeof *items);
    tree = lks_tree_create();
    if (items == NULL || tree == NULL) ok = 0;
    for (step = 0; step < steps && ok; ++step) {
        unsigned int action = next_random(&seed) % 100u;
        if (count == 0) action = 0;
        else if (count == MODEL_CAPACITY || step % 4096u < MODEL_CAPACITY)
            action = 40;
        if (action < 35) {
            LksPath *candidate = NULL;
            const LksTreeNode *node = NULL;
            SoakItem *item = &items[next_item];
            item->id = next_item + 1;
            item->value = (int)(item->id % 11u);
            if (gap_path(model, count, &seed, &candidate) != LKS_STATUS_OK ||
                candidate == NULL ||
                lks_tree_insert(tree, candidate, item, &node) != LKS_STATUS_OK ||
                node == NULL || lks_tree_node_item(node) != item) {
                lks_path_destroy(candidate);
                ok = 0;
            } else {
                insert_model(model, &count, candidate, item);
                ++next_item;
            }
        } else if (action < 60) {
            size_t index = next_random(&seed) % count;
            void *removed = NULL;
            if (lks_tree_remove_path(tree, model[index].path, &removed) !=
                LKS_STATUS_OK || removed != model[index].item) ok = 0;
            else remove_model(model, &count, index);
        } else if (action < 85) {
            size_t index = next_random(&seed) % count;
            const LksTreeNode *before = NULL, *after = NULL;
            if (lks_tree_find_path(tree, model[index].path, &before) !=
                LKS_STATUS_OK) ok = 0;
            if (ok && next_random(&seed) % 5u == 0) {
                if (lks_tree_rekey(tree, model[index].path, model[index].path,
                    &after) != LKS_STATUS_OK || after != before ||
                    lks_tree_node_item(after) != model[index].item) ok = 0;
            } else if (ok) {
                SoakEntry others[MODEL_CAPACITY];
                LksPath *target = NULL;
                size_t j, n = 0;
                for (j = 0; j < count; ++j)
                    if (j != index) others[n++] = model[j];
                if (gap_path(others, n, &seed, &target) != LKS_STATUS_OK ||
                    target == NULL) ok = 0;
                if (ok && path_order(target, model[index].path) == 0) {
                    lks_path_destroy(target);
                    target = NULL;
                    if (n != 0 && index == count - 1)
                        ok = lks_path_before(others[0].path, &target) ==
                            LKS_STATUS_OK;
                    else if (n != 0)
                        ok = lks_path_after(others[n - 1].path, &target) ==
                            LKS_STATUS_OK;
                    else
                        ok = lks_path_before(model[index].path, &target) ==
                            LKS_STATUS_OK;
                }
                if (ok && (target == NULL ||
                    lks_tree_rekey(tree, model[index].path, target, &after) !=
                        LKS_STATUS_OK || after == NULL ||
                    lks_tree_node_item(after) != model[index].item)) ok = 0;
                if (ok) {
                    SoakItem *item = model[index].item;
                    remove_model(model, &count, index);
                    insert_model(model, &count, target, item);
                } else lks_path_destroy(target);
            }
        } else {
            size_t index = next_random(&seed) % count;
            const LksTreeNode *node = NULL;
            if (lks_tree_find_path(tree, model[index].path, &node) !=
                LKS_STATUS_OK || node == NULL ||
                lks_tree_node_item(node) != model[index].item) ok = 0;
        }
        if (ok && step % 41u == 0) {
            LksPath *missing = lks_path_create_at_level(
                LKS_DIRECTION_NEGATIVE, 65535u, SIZE_MAX);
            const LksTreeNode *node = NULL;
            void *removed = (void *)(uintptr_t)1;
            if (missing == NULL ||
                lks_tree_find_path(tree, missing, &node) !=
                    LKS_STATUS_NOT_FOUND || node != NULL ||
                lks_tree_remove_path(tree, missing, &removed) !=
                    LKS_STATUS_NOT_FOUND || removed != NULL) ok = 0;
            lks_path_destroy(missing);
        }
        if (ok && (step % 29u == 0 || step + 1 == steps) &&
            !verify_tree(tree, model, count, 0)) ok = 0;
        if (ok && count != 0 && step % 53u == 0 &&
            !round_trip(model[next_random(&seed) % count].path)) ok = 0;
    }
    if (!ok) fprintf(stderr, "Soak explicit failed step=%zu seed=%08X count=%zu\n",
        step, (unsigned int)initial_seed, count);
    for (i = 0; i < count; ++i) lks_path_destroy(model[i].path);
    lks_tree_destroy(tree);
    free(items);
    printf("Soak explicit Seed=%08X Steps=%zu %s\n",
        (unsigned int)initial_seed, step, ok ? "PASS" : "FAIL");
    return ok && allocations_clean();
}

static int refresh_comparator_paths(const LksTree *tree, SoakEntry *model,
    size_t count)
{
    const LksTreeNode *nodes[MODEL_CAPACITY];
    LksPath *replacement[MODEL_CAPACITY] = {NULL};
    size_t i, j;
    if (lks_tree_internal_fill_ordered(tree, nodes, count) != LKS_STATUS_OK)
        return 0;
    for (i = 0; i < count; ++i) {
        if (lks_tree_node_item(nodes[i]) != model[i].item) break;
        replacement[i] = lks_path_clone(lks_tree_node_path(nodes[i]));
        if (replacement[i] == NULL) break;
    }
    if (i != count) {
        for (j = 0; j < count; ++j) lks_path_destroy(replacement[j]);
        return 0;
    }
    for (i = 0; i < count; ++i) {
        lks_path_destroy(model[i].path);
        model[i].path = replacement[i];
    }
    return 1;
}

static int run_comparator(size_t steps, uint32_t initial_seed)
{
    SoakEntry model[MODEL_CAPACITY] = {{0}};
    SoakItem *items = NULL;
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree = NULL;
    uint32_t seed = initial_seed;
    size_t count = 0, next_item = 0, step = 0, i;
    int ok = 1;
    if (steps > SIZE_MAX / sizeof *items || lks_alloc_stats_reset() != 0)
        return 0;
    items = (SoakItem *)calloc(steps ? steps : 1, sizeof *items);
    tree = lks_tree_create();
    if (items == NULL || tree == NULL) ok = 0;
    for (step = 0; step < steps && ok; ++step) {
        unsigned int action = next_random(&seed) % 100u;
        if (count == 0) action = 0;
        else if (count == MODEL_CAPACITY || step % 4096u < MODEL_CAPACITY)
            action = 60;
        if (action < 50) {
            SoakItem *item = &items[next_item];
            const LksTreeNode *node = NULL;
            size_t position = 0, j;
            item->id = next_item + 1;
            item->value = (int)(next_random(&seed) % 8u);
            while (position < count && model[position].item->value <=
                item->value) ++position;
            if (lks_tree_insert_item(tree, item, &comparator, &node) !=
                LKS_STATUS_OK || node == NULL ||
                lks_tree_node_item(node) != item) ok = 0;
            if (ok) {
                for (j = count; j > position; --j) model[j] = model[j - 1];
                model[position].item = item;
                model[position].path = NULL;
                ++count;
                ++next_item;
                if (!refresh_comparator_paths(tree, model, count)) ok = 0;
            }
        } else if (action < 85) {
            size_t index = next_random(&seed) % count;
            void *removed = NULL;
            if (lks_tree_remove_path(tree, model[index].path, &removed) !=
                LKS_STATUS_OK || removed != model[index].item) ok = 0;
            else remove_model(model, &count, index);
        } else {
            size_t index = next_random(&seed) % count;
            const LksTreeNode *before = NULL, *after = NULL;
            if (lks_tree_find_path(tree, model[index].path, &before) !=
                LKS_STATUS_OK || before == NULL ||
                lks_tree_rekey(tree, model[index].path, model[index].path,
                    &after) != LKS_STATUS_OK || after != before ||
                lks_tree_node_item(after) != model[index].item) ok = 0;
        }
        if (ok && (step % 97u == 0 || step + 1 == steps) &&
            !verify_tree(tree, model, count, 1)) ok = 0;
        if (ok && count != 0 && step % 113u == 0 &&
            !round_trip(model[next_random(&seed) % count].path)) ok = 0;
    }
    if (!ok) fprintf(stderr, "Soak comparator failed step=%zu seed=%08X count=%zu\n",
        step, (unsigned int)initial_seed, count);
    for (i = 0; i < count; ++i) lks_path_destroy(model[i].path);
    lks_tree_destroy(tree);
    free(items);
    printf("Soak comparator Seed=%08X Steps=%zu %s\n",
        (unsigned int)initial_seed, step, ok ? "PASS" : "FAIL");
    return ok && allocations_clean();
}

int main(int argc, char **argv)
{
    size_t total = 30000, explicit_steps;
    if (argc == 3 && strcmp(argv[1], "--operations") == 0) {
        char *end = NULL;
        unsigned long long parsed;
        errno = 0;
        parsed = strtoull(argv[2], &end, 10);
        if (errno == ERANGE || argv[2][0] == '-' || end == argv[2] ||
            *end != '\0' || parsed < 4 || parsed > SIZE_MAX) {
            fputs("Invalid operation count\n", stderr);
            return 2;
        }
        total = (size_t)parsed;
    } else if (argc != 1) {
        fputs("Usage: layerkeysort_soak [--operations N]\n", stderr);
        return 2;
    }
    explicit_steps = total - total / 4u;
    if (total > SIZE_MAX / sizeof(SoakItem)) {
        fputs("Operation count exceeds allocation range\n", stderr);
        return 2;
    }
    if (!run_explicit(explicit_steps, UINT32_C(0x6B47C291)) ||
        !run_comparator(total - explicit_steps, UINT32_C(0xA9172E63)))
        return 1;
    printf("Mutation soak total operations=%zu PASS\n", total);
    return 0;
}
