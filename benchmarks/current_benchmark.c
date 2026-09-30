/* Reproducible public-API timing harness. Build against either the current
 * library or Preview.3 (-DLKS_BENCH_PREVIEW3 for unavailable newer APIs). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "layerkeysort.h"
#ifdef LKS_BENCH_DIAGNOSTICS
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"
#endif

typedef struct Item { int key; size_t serial; } Item;
typedef struct NodeRef { const LksTreeNode *node; } NodeRef;
static volatile size_t sink;
#if defined(LKS_BENCH_DIAGNOSTICS) || defined(LKS_BENCH_COUNT_COMPARISONS)
static size_t comparator_calls;
#endif

static uint32_t random_next(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *state = x;
}

static int compare_item(const void *a, const void *b, void *context)
{
    const Item *x = (const Item *)a, *y = (const Item *)b;
    (void)context;
#if defined(LKS_BENCH_DIAGNOSTICS) || defined(LKS_BENCH_COUNT_COMPARISONS)
    ++comparator_calls;
#endif
    return (x->key > y->key) - (x->key < y->key);
}

static int compare_pointer(const void *a, const void *b)
{
    return compare_item(*(void *const *)a, *(void *const *)b, NULL);
}

static int compare_node_path(const void *a, const void *b)
{
    const NodeRef *x = (const NodeRef *)a, *y = (const NodeRef *)b;
    int result = 0;
    if (lks_path_compare(lks_tree_node_path(x->node),
            lks_tree_node_path(y->node), &result) != LKS_STATUS_OK) abort();
    return result;
}

static double now_ms(void)
{
    struct timespec t;
    if (timespec_get(&t, TIME_UTC) != TIME_UTC) abort();
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}

static int time_order(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static int fill_items(Item *items, void **pointers, size_t count,
    const char *distribution, uint32_t seed)
{
    size_t i;
    if (count > (size_t)INT32_MAX) return 0;
    for (i = 0; i < count; ++i) {
        int value;
        if (strcmp(distribution, "ascending") == 0) value = (int)i;
        else if (strcmp(distribution, "descending") == 0) value = (int)(count - i);
        else if (strcmp(distribution, "equal") == 0) value = 0;
        else if (strcmp(distribution, "two") == 0) value = (int)(random_next(&seed) % 2);
        else if (strcmp(distribution, "eight") == 0) value = (int)(random_next(&seed) % 8);
        else if (strcmp(distribution, "sixtyfour") == 0) value = (int)(random_next(&seed) % 64);
        else if (strcmp(distribution, "duplicates") == 0) value = (int)(random_next(&seed) % 32);
        else if (strcmp(distribution, "alternating") == 0)
            value = (int)((i & 1u) ? count - 1 - i / 2 : i / 2);
        else if (strcmp(distribution, "middle") == 0)
            value = (int)((i & 1u) ? count / 2 + i / 2 : count / 2 - i / 2);
        else if (strcmp(distribution, "random") == 0) value = (int)i;
        else return 0;
        items[i].key = value;
        items[i].serial = i;
        pointers[i] = &items[i];
    }
    if (strcmp(distribution, "random") == 0)
        for (i = count; i > 1; --i) {
            size_t j = random_next(&seed) % i;
            int value = items[i - 1].key;
            items[i - 1].key = items[j].key;
            items[j].key = value;
        }
    return 1;
}

static int sorted_items(void *const *items, size_t count, int stable)
{
    size_t i;
    for (i = 1; i < count; ++i) {
        const Item *before = (const Item *)items[i - 1];
        const Item *after = (const Item *)items[i];
        if (before->key > after->key || (stable && before->key == after->key &&
                before->serial > after->serial)) return 0;
    }
    return 1;
}

static int validate_tree(const LksTree *tree, size_t count)
{
    const LksTreeNode **stack = NULL;
    NodeRef *refs = NULL;
    size_t top = 0, used = 0, i, children;
    int good = 1;
    if (lks_tree_size(tree) != count) return 0;
    stack = (const LksTreeNode **)malloc((count + 1) * sizeof(*stack));
    refs = (NodeRef *)malloc((count + 1) * sizeof(*refs));
    if (stack == NULL || refs == NULL) { good = 0; goto done; }
    children = lks_tree_root_child_count(tree);
    for (i = 0; i < children; ++i)
        stack[top++] = lks_tree_root_child_at(tree, i);
    while (top != 0 && good) {
        const LksTreeNode *node = stack[--top];
        size_t j, child_count;
        if (node == NULL || used >= count) { good = 0; break; }
        refs[used++].node = node;
        child_count = lks_tree_node_child_count(node);
        for (j = 0; j < child_count; ++j) {
            if (top >= count) { good = 0; break; }
            stack[top++] = lks_tree_node_child_at(node, j);
        }
    }
    if (used != count) good = 0;
    if (good) {
        qsort(refs, count, sizeof(*refs), compare_node_path);
        for (i = 1; i < count; ++i) {
            const Item *a = (const Item *)lks_tree_node_item(refs[i - 1].node);
            const Item *b = (const Item *)lks_tree_node_item(refs[i].node);
            if (a->key > b->key || (a->key == b->key && a->serial > b->serial)) {
                good = 0; break;
            }
        }
    }
done:
    free(stack); free(refs);
    return good;
}

#ifdef LKS_BENCH_DIAGNOSTICS
static int compare_size(const void *a, const void *b)
{
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

static void diagnose_tree(const LksTree *tree, const char *distribution,
    size_t count, LksAllocStats allocations, LksTreeRepairStats repair)
{
    LksTreeInternalProfile profile;
    const LksTreeNode **stack = NULL;
    size_t *depths = NULL;
    size_t top = 0, used = 0, i, depth_sum = 0, display_sum = 0, key_sum = 0;
    size_t display_max = 0, key_max = 0;
    if (lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK) abort();
    stack = (const LksTreeNode **)malloc((count + 1) * sizeof(*stack));
    depths = (size_t *)malloc((count + 1) * sizeof(*depths));
    if (stack == NULL || depths == NULL) abort();
    for (i = 0; i < lks_tree_root_child_count(tree); ++i)
        stack[top++] = lks_tree_root_child_at(tree, i);
    while (top != 0) {
        const LksTreeNode *node = stack[--top];
        const LksPath *path = lks_tree_node_path(node);
        size_t children = lks_tree_node_child_count(node);
        size_t display = lks_path_text_length(path);
        size_t key = lks_path_order_key_length(path);
        depths[used++] = lks_path_depth(path);
        depth_sum += lks_path_depth(path);
        display_sum += display; key_sum += key;
        if (display > display_max) display_max = display;
        if (key > key_max) key_max = key;
        for (i = 0; i < children; ++i)
            stack[top++] = lks_tree_node_child_at(node, i);
    }
    if (used != count) abort();
    qsort(depths, count, sizeof(*depths), compare_size);
    printf("diag,%s,%zu", distribution, count);
    printf(",%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu",
        comparator_calls, repair.comparator_search_steps,
        repair.rotations, profile.avl_height, repair.attempts, repair.successes,
        repair.fallbacks, repair.region_expansions, repair.max_region_nodes,
        repair.nodes_relabelled, repair.deeper_accepts, repair.full_rebuilds,
        allocations.alloc_calls, allocations.total_successful_requested_bytes);
    printf(",%.3f,%zu,%zu,%.3f,%zu,%.3f,%zu,%zu,%zu,%zu,%zu,%zu\n",
        (double)depth_sum / count, depths[(95 * count - 1) / 100],
        depths[(99 * count - 1) / 100], (double)display_sum / count,
        display_max, (double)key_sum / count, key_max,
        allocations.peak_live_bytes,
        allocations.tags[LKS_ALLOC_TAG_TREE_NODE].live_bytes,
        allocations.tags[LKS_ALLOC_TAG_PATH_OBJECT].live_bytes,
        allocations.tags[LKS_ALLOC_TAG_PATH_STEPS].live_bytes,
        lks_tree_internal_sizeof_node());
#ifdef LKS_BENCH_STAGE5_DIAGNOSTICS
    printf("rebuild_reasons,%s,%zu,%zu,%zu,%zu\n", distribution, count,
        repair.gap_limit_rebuild_attempts,
        repair.depth_limit_rebuild_attempts, repair.full_rebuilt_nodes);
#endif
    free(depths); free(stack);
}
#endif

#ifndef LKS_BENCH_PREVIEW3
static int run_mutation(const char *operation, Item *items, size_t count,
    double *out_ms)
{
    LksTree *tree = NULL;
    LksPath **original = NULL, **target = NULL;
    size_t i, active = strcmp(operation, "mut_mixed") == 0 ? count / 2 : count;
    uint32_t state = UINT32_C(0x54B7D091);
    size_t *order = NULL;
    double start, end;
    int good = 1;
    if (count > SIZE_MAX / sizeof(*original)) return 0;
    original = (LksPath **)calloc(count, sizeof(*original));
    target = (LksPath **)calloc(count, sizeof(*target));
    order = (size_t *)malloc(count * sizeof(*order));
    tree = lks_tree_create();
    if (original == NULL || target == NULL || order == NULL || tree == NULL)
        { good = 0; goto done; }
    for (i = 0; i < count; ++i) {
        order[i] = i;
        original[i] = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 0, i + 1);
        target[i] = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, 1,
            count + i + 1);
        if (original[i] == NULL || target[i] == NULL) { good = 0; goto done; }
    }
    for (i = 0; i < active; ++i)
        if (lks_tree_insert(tree, original[i], &items[i], NULL) != LKS_STATUS_OK)
            { good = 0; goto done; }
    if (strcmp(operation, "mut_remove_random") == 0 ||
        strcmp(operation, "mut_rekey_random") == 0)
        for (i = active; i > 1; --i) {
            size_t j = random_next(&state) % i, temp = order[i - 1];
            order[i - 1] = order[j]; order[j] = temp;
        }
    start = now_ms();
    if (strcmp(operation, "mut_remove_random") == 0 ||
        strcmp(operation, "mut_remove_all") == 0) {
        for (i = 0; i < active; ++i) {
            size_t index = strcmp(operation, "mut_remove_all") == 0 ?
                active - 1 - i : order[i];
            void *removed = NULL;
            if (lks_tree_remove_path(tree, original[index], &removed) != LKS_STATUS_OK ||
                removed != &items[index]) { good = 0; break; }
        }
    } else if (strcmp(operation, "mut_rekey_random") == 0) {
        for (i = 0; i < active; ++i) {
            size_t index = order[i];
            if (lks_tree_rekey(tree, original[index], target[index], NULL) !=
                LKS_STATUS_OK) { good = 0; break; }
        }
    } else if (strcmp(operation, "mut_rekey_hotspot") == 0) {
        for (i = 0; i < count; ++i) {
            const LksPath *old_path = (i & 1u) ? target[0] : original[0];
            const LksPath *new_path = (i & 1u) ? original[0] : target[0];
            if (lks_tree_rekey(tree, old_path, new_path, NULL) != LKS_STATUS_OK)
                { good = 0; break; }
        }
    } else if (strcmp(operation, "mut_mixed") == 0) {
        for (i = 0; i < active; ++i) {
            void *removed = NULL;
            LksPath *new_path = original[active + i];
            LksPath *move_path = target[active + i];
            if (lks_tree_remove_path(tree, original[i], &removed) != LKS_STATUS_OK ||
                removed != &items[i] ||
                lks_tree_insert(tree, new_path, &items[active + i], NULL) !=
                    LKS_STATUS_OK ||
                lks_tree_rekey(tree, new_path, move_path, NULL) != LKS_STATUS_OK)
                { good = 0; break; }
        }
    } else good = 0;
    end = now_ms();
    if (good) good = lks_tree_size(tree) ==
        ((strcmp(operation, "mut_remove_random") == 0 ||
          strcmp(operation, "mut_remove_all") == 0) ? 0 : active);
    if (good && strcmp(operation, "mut_rekey_random") == 0)
        for (i = 0; i < active; ++i) {
            const LksTreeNode *node = NULL;
            if (lks_tree_find_path(tree, target[i], &node) != LKS_STATUS_OK ||
                lks_tree_node_item(node) != &items[i]) { good = 0; break; }
        }
    if (good && strcmp(operation, "mut_rekey_hotspot") == 0) {
        const LksTreeNode *node = NULL;
        const LksPath *final_path = (count & 1u) ? target[0] : original[0];
        good = lks_tree_find_path(tree, final_path, &node) == LKS_STATUS_OK &&
            lks_tree_node_item(node) == &items[0];
    }
    if (good && strcmp(operation, "mut_mixed") == 0)
        for (i = 0; i < active; ++i) {
            const LksTreeNode *node = NULL;
            if (lks_tree_find_path(tree, target[active + i], &node) != LKS_STATUS_OK ||
                lks_tree_node_item(node) != &items[active + i]) { good = 0; break; }
        }
    if (good) *out_ms = end - start;
done:
    lks_tree_destroy(tree);
    if (original != NULL) for (i = 0; i < count; ++i) lks_path_destroy(original[i]);
    if (target != NULL) for (i = 0; i < count; ++i) lks_path_destroy(target[i]);
    free(original); free(target); free(order);
    return good;
}
#endif

static int run_once(const char *operation, const char *distribution,
    size_t count, uint32_t seed, double *out_ms, size_t *out_bytes)
{
    Item *items = NULL;
    void **pointers = NULL;
    LksComparator comparator = {compare_item, NULL};
    LksTree *tree = NULL;
    LksGroup *group = NULL, *result = NULL;
    LksGroupBatch *batch = NULL;
    size_t i;
    double start, end;
    int good = 1;
#ifdef LKS_BENCH_PREVIEW3
    (void)out_bytes;
#endif
    if (count > SIZE_MAX / sizeof(*items) ||
        count > SIZE_MAX / sizeof(*pointers)) return 0;
    items = (Item *)malloc(count * sizeof(*items));
    pointers = (void **)malloc(count * sizeof(*pointers));
    if (items == NULL || pointers == NULL ||
        !fill_items(items, pointers, count, distribution, seed)) { good = 0; goto done; }
#ifndef LKS_BENCH_PREVIEW3
    if (strncmp(operation, "mut_", 4) == 0) {
        good = run_mutation(operation, items, count, out_ms);
        if (good) sink += count;
        goto done;
    }
#endif
    if (strcmp(operation, "tree") == 0) {
#if defined(LKS_BENCH_DIAGNOSTICS) || defined(LKS_BENCH_COUNT_COMPARISONS)
        comparator_calls = 0;
#endif
#ifdef LKS_BENCH_DIAGNOSTICS
        if (lks_alloc_stats_reset() != 0) { good = 0; goto done; }
        lks_tree_repair_stats_reset();
#endif
        tree = lks_tree_create();
        if (tree == NULL) { good = 0; goto done; }
        start = now_ms();
        for (i = 0; i < count; ++i) {
            if (lks_tree_insert_item(tree, pointers[i], &comparator, NULL) != LKS_STATUS_OK) {
                good = 0; break;
            }
        }
        end = now_ms();
#ifdef LKS_BENCH_COUNT_COMPARISONS
        if (good) printf("comparisons,%s,%zu,%zu\n", distribution,
            count, comparator_calls);
#endif
#ifdef LKS_BENCH_DIAGNOSTICS
        if (good) diagnose_tree(tree, distribution, count,
            lks_alloc_stats_get(), lks_tree_repair_stats_get());
#endif
        if (good) good = validate_tree(tree, count);
    } else if (strcmp(operation, "group") == 0) {
        start = now_ms();
        good = lks_group_build(pointers, count, &comparator, &group) == LKS_STATUS_OK;
        end = now_ms();
        if (good) {
            for (i = 0; i < count; ++i) pointers[i] = lks_group_item_at(group, i);
            good = sorted_items(pointers, count, 1);
        }
    } else if (strcmp(operation, "batch") == 0) {
        start = now_ms();
        good = lks_group_batch_build(pointers, count, 128, &comparator, &batch) == LKS_STATUS_OK;
        end = now_ms();
        if (good) {
            size_t j, groups = lks_group_batch_group_count(batch);
            good = lks_group_batch_total_size(batch) == count;
            for (i = 0; good && i < groups; ++i) {
                const LksGroup *part = lks_group_batch_group_at(batch, i);
                size_t size = lks_group_size(part);
                for (j = 1; j < size; ++j) {
                    const Item *a = (const Item *)lks_group_item_at(part, j - 1);
                    const Item *b = (const Item *)lks_group_item_at(part, j);
                    if (a->key > b->key || (a->key == b->key &&
                            a->serial > b->serial)) { good = 0; break; }
                }
            }
        }
    } else if (strcmp(operation, "batch_merge") == 0) {
        if (lks_group_batch_build(pointers, count, 128, &comparator, &batch) != LKS_STATUS_OK) {
            good = 0; goto done;
        }
        start = now_ms();
        good = lks_group_batch_merge_all(batch, &comparator, &result) == LKS_STATUS_OK;
        end = now_ms();
        if (good) {
            for (i = 0; i < count; ++i) pointers[i] = lks_group_item_at(result, i);
            good = sorted_items(pointers, count, 1);
        }
    } else if (strcmp(operation, "group_merge") == 0) {
        size_t half = count / 2;
        if (lks_group_build(pointers, half, &comparator, &group) != LKS_STATUS_OK ||
            lks_group_build(pointers + half, count - half, &comparator, &result) != LKS_STATUS_OK) {
            good = 0; goto done;
        }
        { LksGroup *merged = NULL;
            start = now_ms();
            good = lks_group_merge(group, result, &comparator, &merged) == LKS_STATUS_OK;
            end = now_ms();
            if (good) {
                for (i = 0; i < count; ++i) pointers[i] = lks_group_item_at(merged, i);
                good = sorted_items(pointers, count, 1);
            }
            lks_group_destroy(merged);
        }
    } else if (strcmp(operation, "sort") == 0) {
        start = now_ms();
        good = lks_sort(pointers, count, compare_item, NULL) == LKS_STATUS_OK;
        end = now_ms();
        if (good) good = sorted_items(pointers, count, 1);
    } else if (strcmp(operation, "qsort") == 0) {
        start = now_ms();
        qsort(pointers, count, sizeof(*pointers), compare_pointer);
        end = now_ms();
        good = sorted_items(pointers, count, 0);
#ifndef LKS_BENCH_PREVIEW3
    } else if (strcmp(operation, "path_format") == 0 ||
               strcmp(operation, "path_parse") == 0 ||
               strcmp(operation, "key_format") == 0 ||
               strcmp(operation, "key_parse") == 0) {
        LksPath *path = lks_path_create(LKS_DIRECTION_POSITIVE, 32768);
        char *buffer = NULL;
        size_t length, depth = count;
        int key = operation[0] == 'k', parse = strstr(operation, "parse") != NULL;
        if (path == NULL) { good = 0; goto done; }
        for (i = 1; i < depth; ++i)
            if (lks_path_append(path, (unsigned int)(i % 65536u)) != LKS_STATUS_OK) {
                good = 0; break;
            }
        length = key ? lks_path_order_key_length(path) : lks_path_text_length(path);
        if (!good || length == 0 || length == SIZE_MAX) {
            lks_path_destroy(path); good = 0; goto done;
        }
        buffer = (char *)malloc(length + 1);
        if (buffer == NULL || (key ?
            lks_path_order_key_format(path, buffer, length + 1) :
            lks_path_format(path, buffer, length + 1)) != LKS_STATUS_OK) {
            free(buffer); lks_path_destroy(path); good = 0; goto done;
        }
        start = now_ms();
        for (i = 0; i < 100; ++i) {
            if (parse) {
                LksPath *decoded = NULL;
                LksStatus status = key ? lks_path_order_key_parse(buffer, &decoded) :
                    lks_path_parse(buffer, &decoded);
                if (status != LKS_STATUS_OK || decoded == NULL) good = 0;
                lks_path_destroy(decoded);
            } else {
                LksStatus status = key ?
                    lks_path_order_key_format(path, buffer, length + 1) :
                    lks_path_format(path, buffer, length + 1);
                if (status != LKS_STATUS_OK) good = 0;
            }
            if (!good) break;
        }
        end = now_ms();
        if (good && parse) {
            LksPath *decoded = NULL;
            int order = 1;
            LksStatus status = key ? lks_path_order_key_parse(buffer, &decoded) :
                lks_path_parse(buffer, &decoded);
            good = status == LKS_STATUS_OK && decoded != NULL &&
                lks_path_compare(path, decoded, &order) == LKS_STATUS_OK &&
                order == 0;
            lks_path_destroy(decoded);
        }
        *out_bytes = length;
        lks_path_destroy(path); free(buffer);
#endif
    } else { good = 0; goto done; }
    if (good) { *out_ms = end - start; sink += count; }
done:
    lks_group_destroy(group); lks_group_destroy(result);
    lks_group_batch_destroy(batch); lks_tree_destroy(tree);
    free(pointers); free(items);
    return good;
}

int main(int argc, char **argv)
{
    size_t n, i, repetitions, warmups, bytes = 0;
    double *samples, elapsed, low, high, median;
    char *end;
    uint32_t seed = UINT32_C(0x91A30D47);
    if (argc != 6 && argc != 7) {
        fputs("usage: benchmark OPERATION DISTRIBUTION N WARMUPS REPETITIONS [SEED]\n", stderr);
        return 2;
    }
    n = (size_t)strtoull(argv[3], &end, 10);
    if (*end || n == 0 || n > 1000000) return 2;
    warmups = (size_t)strtoull(argv[4], &end, 10);
    if (*end || warmups > 10) return 2;
    repetitions = (size_t)strtoull(argv[5], &end, 10);
    if (*end || repetitions == 0 || repetitions > 30) return 2;
    if (argc == 7) {
        seed = (uint32_t)strtoul(argv[6], &end, 0);
        if (*end || seed == 0) return 2;
    }
    samples = (double *)malloc(repetitions * sizeof(*samples));
    if (samples == NULL) return 2;
    for (i = 0; i < warmups + repetitions; ++i) {
        if (!run_once(argv[1], argv[2], n, seed, &elapsed, &bytes)) {
            fprintf(stderr, "benchmark self-check failed: %s/%s/%zu run %zu\n",
                argv[1], argv[2], n, i);
            free(samples); return 1;
        }
        if (i >= warmups) samples[i - warmups] = elapsed;
    }
    qsort(samples, repetitions, sizeof(*samples), time_order);
    low = samples[0]; high = samples[repetitions - 1];
    median = samples[repetitions / 2];
    if (repetitions % 2 == 0)
        median = (samples[repetitions / 2 - 1] + median) / 2.0;
    printf("%s,%s,%zu,%zu,%zu,%.3f,%.3f,%.3f,%zu\n", argv[1], argv[2], n,
        warmups, repetitions, median, low, high, bytes);
    free(samples);
    return 0;
}
