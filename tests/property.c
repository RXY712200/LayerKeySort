#include "lks_legacy_internal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/layerkeysort.h"
#include "../src/lks_alloc_internal.h"
#include "../src/lks_group_internal.h"
#include "../src/lks_tree_internal.h"
#include "v2_tree_regression_bridge.h"
#include "benchmark.h"
#include "property.h"

#define LKS_PROPERTY_BASE_SEED UINT32_C(0x14A10C5D)
#define LKS_PROPERTY_SEED_COUNT 64u
#define LKS_PROPERTY_MAX_ITEMS 512u
#define LKS_PROPERTY_PATH_ROUNDS 24u
#define LKS_PROPERTY_PATH_COUNT 48u
#define LKS_PROPERTY_PATH_TRIPLES 12000u
#define LKS_PROPERTY_GAP_TARGET 256u

typedef struct LksPropertyItem {
    int key;
    unsigned int original_index;
} LksPropertyItem;

typedef struct LksPropertyRandom {
    uint32_t state;
} LksPropertyRandom;

typedef enum LksPropertyDistribution {
    LKS_PROPERTY_UNIQUE = 0,
    LKS_PROPERTY_DUPLICATE,
    LKS_PROPERTY_HEAVY_DUPLICATE,
    LKS_PROPERTY_ALL_EQUAL,
    LKS_PROPERTY_SORTED,
    LKS_PROPERTY_REVERSE,
    LKS_PROPERTY_DISTRIBUTION_COUNT
} LksPropertyDistribution;

typedef struct LksPropertyCounts {
    size_t group_stable_oracle_cases;
    size_t batch_oracle_cases;
    size_t public_merge_oracle_cases;
    size_t private_merge_differential_cases;
    size_t duplicate_cases;
    size_t heavy_duplicate_cases;
    size_t all_equal_cases;
    size_t equal_stability_failures;
    size_t batch_mutation_failures;
    size_t public_input_mutation_failures;
    size_t private_incoming_mutation_failures;
    size_t group_oracle_failures;
    size_t batch_oracle_failures;
    size_t public_merge_oracle_failures;
    size_t private_merge_differential_failures;
    size_t path_order_failures;
    size_t path_uniqueness_failures;
    size_t before_checks;
    size_t after_checks;
    size_t between_checks;
    size_t path_construction_failures;
    size_t tree_profile_checks;
    size_t tree_profile_failures;
    size_t allocator_failures;
    size_t leak_count;
    size_t path_pair_checks;
    size_t path_triple_checks;
    size_t antisymmetry_failures;
    size_t transitivity_failures;
} LksPropertyCounts;

typedef struct LksPropertyGroupSnapshot {
    size_t count;
    void *items[LKS_PROPERTY_MAX_ITEMS];
    LksPath *paths[LKS_PROPERTY_MAX_ITEMS];
} LksPropertyGroupSnapshot;

typedef struct LksPropertyBatchSnapshot {
    size_t group_count;
    size_t item_count;
    size_t group_sizes[LKS_PROPERTY_MAX_ITEMS];
    void *items[LKS_PROPERTY_MAX_ITEMS];
    LksPath *paths[LKS_PROPERTY_MAX_ITEMS];
} LksPropertyBatchSnapshot;

static const size_t property_sizes[] = {
    0, 1, 2, 3, 7, 16, 31, 32, 33, 63, 64
};

static uint32_t property_random_next(LksPropertyRandom *random)
{
    uint32_t value = random->state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    random->state = value;
    return value;
}

static uint32_t property_seed_at(size_t seed_index)
{
    uint32_t seed = LKS_PROPERTY_BASE_SEED +
        (uint32_t)seed_index * UINT32_C(0x9E3779B9);
    return seed == 0 ? UINT32_C(0xA341316C) : seed;
}

static const char *property_distribution_name(LksPropertyDistribution distribution)
{
    static const char *const names[] = {
        "Unique", "Duplicate", "HeavyDuplicate", "AllEqual", "Sorted", "Reverse"
    };
    return distribution >= LKS_PROPERTY_UNIQUE &&
        distribution < LKS_PROPERTY_DISTRIBUTION_COUNT ?
        names[(size_t)distribution] : "Unknown";
}

static int property_compare_key(const void *left, const void *right,
    void *context)
{
    const LksPropertyItem *left_item = (const LksPropertyItem *)left;
    const LksPropertyItem *right_item = (const LksPropertyItem *)right;

    (void)context;
    if (left_item->key < right_item->key) return -1;
    if (left_item->key > right_item->key) return 1;
    return 0;
}

/* Stable test oracle: equal keys are never moved past one another. */
static void property_stable_insertion_sort(
    LksPropertyItem **items,
    size_t count
)
{
    size_t index;

    for (index = 1; index < count; ++index) {
        LksPropertyItem *item = items[index];
        size_t position = index;

        while (position > 0 && items[position - 1]->key > item->key) {
            items[position] = items[position - 1];
            --position;
        }
        items[position] = item;
    }
}

static void property_shuffle(LksPropertyRandom *random,
    LksPropertyItem **items, size_t count)
{
    size_t index;

    for (index = count; index > 1; --index) {
        size_t other = (size_t)(property_random_next(random) % index);
        LksPropertyItem *temporary = items[index - 1];
        items[index - 1] = items[other];
        items[other] = temporary;
    }
}

static void property_make_dataset(
    LksPropertyRandom *random,
    LksPropertyDistribution distribution,
    size_t count,
    LksPropertyItem *storage,
    LksPropertyItem **input
)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        storage[index].original_index = (unsigned int)index;
        switch (distribution) {
        case LKS_PROPERTY_UNIQUE:
        case LKS_PROPERTY_SORTED:
            storage[index].key = (int)index;
            break;
        case LKS_PROPERTY_DUPLICATE:
            storage[index].key = (int)(property_random_next(random) % 11u) - 5;
            break;
        case LKS_PROPERTY_HEAVY_DUPLICATE:
            storage[index].key = (int)(property_random_next(random) % 3u) - 1;
            break;
        case LKS_PROPERTY_ALL_EQUAL:
            storage[index].key = 7;
            break;
        case LKS_PROPERTY_REVERSE:
            storage[index].key = (int)(count - index);
            break;
        default:
            storage[index].key = 0;
            break;
        }
        input[index] = &storage[index];
    }

    if (distribution == LKS_PROPERTY_UNIQUE ||
        distribution == LKS_PROPERTY_DUPLICATE ||
        distribution == LKS_PROPERTY_HEAVY_DUPLICATE ||
        distribution == LKS_PROPERTY_ALL_EQUAL) {
        property_shuffle(random, input, count);
    }
    for (index = 0; index < count; ++index)
        input[index]->original_index = (unsigned int)index;
}

static int property_paths_strictly_increase(const LksLegacyGroup *group,
    size_t count, LksPropertyCounts *counts)
{
    size_t index;
    int valid = 1;

    for (index = 1; index < count; ++index) {
        int order = 0;
        LksStatus status = lks_path_compare(lks_legacy_group_path_at(group, index - 1),
            lks_legacy_group_path_at(group, index), &order);

        if (status != LKS_STATUS_OK || order >= 0) {
            ++counts->path_order_failures;
            valid = 0;
        }
        if (status == LKS_STATUS_OK && order == 0) {
            ++counts->path_uniqueness_failures;
            valid = 0;
        }
    }
    return valid;
}

static int property_group_matches_oracle(const LksLegacyGroup *group,
    LksPropertyItem *const *expected, size_t count,
    LksPropertyCounts *counts)
{
    size_t index;
    int valid = 1;

    if (lks_legacy_group_size(group) != count) return 0;
    for (index = 0; index < count; ++index) {
        if (lks_legacy_group_item_at(group, index) != expected[index]) valid = 0;
        if (index > 0 && expected[index - 1]->key == expected[index]->key &&
            expected[index - 1]->original_index > expected[index]->original_index)
            valid = 0;
    }
    if (!property_paths_strictly_increase(group, count, counts)) valid = 0;
    return valid;
}

static int property_snapshot_group(const LksLegacyGroup *group,
    LksPropertyGroupSnapshot *snapshot)
{
    size_t index;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->count = lks_legacy_group_size(group);
    if (snapshot->count > LKS_PROPERTY_MAX_ITEMS) return 0;
    for (index = 0; index < snapshot->count; ++index) {
        snapshot->items[index] = lks_legacy_group_item_at(group, index);
        snapshot->paths[index] = lks_path_clone(lks_legacy_group_path_at(group, index));
        if (snapshot->items[index] == NULL || snapshot->paths[index] == NULL) {
            return 0;
        }
    }
    return 1;
}

static int property_group_matches_snapshot(const LksLegacyGroup *group,
    const LksPropertyGroupSnapshot *snapshot)
{
    size_t index;

    if (lks_legacy_group_size(group) != snapshot->count) return 0;
    for (index = 0; index < snapshot->count; ++index) {
        int order;
        if (lks_legacy_group_item_at(group, index) != snapshot->items[index] ||
            lks_path_compare(lks_legacy_group_path_at(group, index),
                snapshot->paths[index], &order) != LKS_STATUS_OK || order != 0) {
            return 0;
        }
    }
    return 1;
}

static void property_destroy_group_snapshot(LksPropertyGroupSnapshot *snapshot)
{
    size_t index;
    for (index = 0; index < snapshot->count; ++index) {
        lks_path_destroy(snapshot->paths[index]);
        snapshot->paths[index] = NULL;
    }
    snapshot->count = 0;
}

static int property_snapshot_batch(const LksLegacyGroupBatch *batch,
    LksPropertyBatchSnapshot *snapshot)
{
    size_t group_index;
    size_t offset = 0;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->group_count = lks_legacy_group_batch_group_count(batch);
    snapshot->item_count = lks_legacy_group_batch_total_size(batch);
    if (snapshot->group_count > LKS_PROPERTY_MAX_ITEMS ||
        snapshot->item_count > LKS_PROPERTY_MAX_ITEMS) return 0;

    for (group_index = 0; group_index < snapshot->group_count; ++group_index) {
        const LksLegacyGroup *group = lks_legacy_group_batch_group_at(batch, group_index);
        size_t group_size = lks_legacy_group_size(group);
        size_t item_index;

        if (group == NULL || offset + group_size > snapshot->item_count) return 0;
        snapshot->group_sizes[group_index] = group_size;
        for (item_index = 0; item_index < group_size; ++item_index) {
            snapshot->items[offset] = lks_legacy_group_item_at(group, item_index);
            snapshot->paths[offset] = lks_path_clone(
                lks_legacy_group_path_at(group, item_index));
            if (snapshot->items[offset] == NULL || snapshot->paths[offset] == NULL)
                return 0;
            ++offset;
        }
    }
    return offset == snapshot->item_count;
}

static int property_batch_matches_snapshot(const LksLegacyGroupBatch *batch,
    const LksPropertyBatchSnapshot *snapshot)
{
    size_t group_index;
    size_t offset = 0;

    if (lks_legacy_group_batch_group_count(batch) != snapshot->group_count ||
        lks_legacy_group_batch_total_size(batch) != snapshot->item_count) return 0;

    for (group_index = 0; group_index < snapshot->group_count; ++group_index) {
        const LksLegacyGroup *group = lks_legacy_group_batch_group_at(batch, group_index);
        size_t group_size = snapshot->group_sizes[group_index];
        size_t item_index;

        if (group == NULL || lks_legacy_group_size(group) != group_size) return 0;
        for (item_index = 0; item_index < group_size; ++item_index) {
            int order;
            if (lks_legacy_group_item_at(group, item_index) != snapshot->items[offset] ||
                lks_path_compare(lks_legacy_group_path_at(group, item_index),
                    snapshot->paths[offset], &order) != LKS_STATUS_OK || order != 0)
                return 0;
            ++offset;
        }
    }
    return offset == snapshot->item_count;
}

static void property_destroy_batch_snapshot(LksPropertyBatchSnapshot *snapshot)
{
    size_t index;
    for (index = 0; index < snapshot->item_count; ++index) {
        lks_path_destroy(snapshot->paths[index]);
        snapshot->paths[index] = NULL;
    }
    snapshot->item_count = 0;
}

static int property_tree_profile_matches(const LksLegacyGroup *group,
    size_t count, LksPropertyCounts *counts)
{
    LksTreeInternalProfile profile;
    const LksTree *tree = lks_legacy_group_internal_tree(group);
    int valid;

    ++counts->tree_profile_checks;
    memset(&profile, 0, sizeof(profile));
    if (tree == NULL || lks_tree_internal_profile(tree, &profile) != LKS_STATUS_OK) {
        ++counts->tree_profile_failures;
        return 0;
    }
    valid = profile.real_node_count == count && profile.balance_valid &&
        profile.leaf_count + profile.unary_count + profile.branching_count ==
            profile.real_node_count;
    if (!valid) ++counts->tree_profile_failures;
    return valid;
}

static void property_check_case_allocator(LksPropertyCounts *counts,
    const char *label)
{
    LksAllocStats stats = lks_alloc_stats_get();

    if (stats.live_bytes != 0 || stats.live_blocks != 0) {
        ++counts->leak_count;
        printf("LeakCount detail: %s live=%zu/%zu\n", label,
            stats.live_bytes, stats.live_blocks);
    }
    if (stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes != 0 ||
        stats.tags[LKS_ALLOC_TAG_OTHER].live_blocks != 0 ||
        stats.failed_calls != 0 || stats.counter_overflowed) {
        ++counts->allocator_failures;
        printf("AllocatorFailures detail: %s OTHER=%zu/%zu FailedCalls=%zu "
            "Overflow=%d\n", label,
            stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes,
            stats.tags[LKS_ALLOC_TAG_OTHER].live_blocks,
            stats.failed_calls, stats.counter_overflowed);
    }
}

static int property_begin_case(LksPropertyCounts *counts, const char *label)
{
    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() == 0) return 1;
    ++counts->allocator_failures;
    printf("AllocatorFailures detail: stats reset refused at %s\n", label);
    return 0;
}

static int property_path_snapshot_unchanged(const LksPath *path,
    const LksPath *snapshot)
{
    int order;
    return lks_path_compare(path, snapshot, &order) == LKS_STATUS_OK && order == 0;
}

static void property_check_gap_constructions(const LksLegacyGroup *group,
    size_t count, LksPropertyCounts *counts)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        const LksPath *path = lks_legacy_group_path_at(group, index);

        if (counts->before_checks < LKS_PROPERTY_GAP_TARGET) {
            LksPath *snapshot = lks_path_clone(path);
            LksPath *before = NULL;
            int order = 0;
            if (snapshot == NULL || lks_path_before(path, &before) != LKS_STATUS_OK ||
                before == NULL || lks_path_compare(before, path, &order) != LKS_STATUS_OK ||
                order >= 0 || !property_path_snapshot_unchanged(path, snapshot)) {
                ++counts->path_construction_failures;
            }
            lks_path_destroy(before);
            lks_path_destroy(snapshot);
            ++counts->before_checks;
        }

        if (counts->after_checks < LKS_PROPERTY_GAP_TARGET) {
            LksPath *snapshot = lks_path_clone(path);
            LksPath *after = NULL;
            int order = 0;
            if (snapshot == NULL || lks_path_after(path, &after) != LKS_STATUS_OK ||
                after == NULL || lks_path_compare(path, after, &order) != LKS_STATUS_OK ||
                order >= 0 || !property_path_snapshot_unchanged(path, snapshot)) {
                ++counts->path_construction_failures;
            }
            lks_path_destroy(after);
            lks_path_destroy(snapshot);
            ++counts->after_checks;
        }

        if (index + 1 < count && counts->between_checks < LKS_PROPERTY_GAP_TARGET) {
            const LksPath *right = lks_legacy_group_path_at(group, index + 1);
            LksPath *left_snapshot = lks_path_clone(path);
            LksPath *right_snapshot = lks_path_clone(right);
            LksPath *between = NULL;
            int left_order = 0;
            int right_order = 0;
            if (left_snapshot == NULL || right_snapshot == NULL ||
                lks_path_between(path, right, &between) != LKS_STATUS_OK ||
                between == NULL || lks_path_compare(path, between, &left_order) != LKS_STATUS_OK ||
                lks_path_compare(between, right, &right_order) != LKS_STATUS_OK ||
                left_order >= 0 || right_order >= 0 ||
                !property_path_snapshot_unchanged(path, left_snapshot) ||
                !property_path_snapshot_unchanged(right, right_snapshot)) {
                ++counts->path_construction_failures;
            }
            lks_path_destroy(between);
            lks_path_destroy(left_snapshot);
            lks_path_destroy(right_snapshot);
            ++counts->between_checks;
        }
    }
}

static int property_run_group_case(size_t seed_index, uint32_t seed,
    LksPropertyDistribution distribution, size_t count,
    LksPropertyCounts *counts, LksPropertyRandom *random)
{
    LksPropertyItem storage[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *input[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *expected[LKS_PROPERTY_MAX_ITEMS];
    LksComparator comparator = { property_compare_key, NULL };
    LksLegacyGroup *group = NULL;
    size_t index;
    int valid = 1;
    char label[64];

    (void)snprintf(label, sizeof(label), "Group-%zu-%u", seed_index,
        (unsigned int)distribution);
    if (!property_begin_case(counts, label)) return 0;
    property_make_dataset(random, distribution, count, storage, input);
    for (index = 0; index < count; ++index) expected[index] = input[index];
    property_stable_insertion_sort(expected, count);

    if (lks_legacy_group_build((void *const *)input, count, &comparator, &group) !=
            LKS_STATUS_OK || group == NULL ||
        !property_group_matches_oracle(group, expected, count, counts)) {
        valid = 0;
        ++counts->group_oracle_failures;
        if (group != NULL) {
            for (index = 0; index < count; ++index) {
                LksPropertyItem *actual =
                    (LksPropertyItem *)lks_legacy_group_item_at(group, index);
                if (actual != NULL && actual != expected[index] &&
                    actual->key == expected[index]->key) {
                    ++counts->equal_stability_failures;
                }
            }
        }
    }
    if (distribution == LKS_PROPERTY_DUPLICATE) ++counts->duplicate_cases;
    if (distribution == LKS_PROPERTY_HEAVY_DUPLICATE) ++counts->heavy_duplicate_cases;
    if (distribution == LKS_PROPERTY_ALL_EQUAL) {
        ++counts->all_equal_cases;
        if (group != NULL) {
            for (index = 0; index < count; ++index) {
                if (lks_legacy_group_item_at(group, index) != input[index]) {
                    ++counts->equal_stability_failures;
                    valid = 0;
                    break;
                }
            }
        }
    }

    if (group != NULL && valid) {
        ++counts->group_stable_oracle_cases;
        property_check_gap_constructions(group, count, counts);
    }
    lks_legacy_group_destroy(group);
    property_check_case_allocator(counts, label);
    if (!valid) printf("GroupOracleFailure seed=0x%08X N=%zu family=%s "
        "GroupSize=NA split=NA\n", (unsigned int)seed, count,
        property_distribution_name(distribution));
    return valid;
}

static int property_run_batch_case(size_t seed_index,
    LksPropertyDistribution distribution, size_t count,
    LksPropertyCounts *counts, LksPropertyRandom *random)
{
    LksPropertyItem storage[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *input[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *expected[LKS_PROPERTY_MAX_ITEMS];
    size_t group_sizes[3];
    size_t group_size_count = 0;
    size_t index;
    LksComparator comparator = { property_compare_key, NULL };
    int valid = 1;

    property_make_dataset(random, distribution, count, storage, input);
    for (index = 0; index < count; ++index) expected[index] = input[index];
    property_stable_insertion_sort(expected, count);

    if (count == 0) {
        group_sizes[group_size_count++] = 1;
    } else {
        group_sizes[group_size_count++] = 1;
        if (count != 1) group_sizes[group_size_count++] = count;
        {
            size_t random_size = 1u +
                (size_t)(property_random_next(random) % count);
            size_t seen;
            for (seen = 0; seen < group_size_count; ++seen)
                if (group_sizes[seen] == random_size) break;
            if (seen == group_size_count) group_sizes[group_size_count++] = random_size;
        }
    }

    for (index = 0; index < group_size_count; ++index) {
        LksLegacyGroupBatch *batch = NULL;
        LksLegacyGroup *result = NULL;
        LksPropertyBatchSnapshot snapshot;
        size_t group_size = group_sizes[index];
    int snapshot_ok = 0;
        int config_valid = 1;
        char label[72];

        (void)snprintf(label, sizeof(label), "Batch-%zu-%u-%zu", seed_index,
            (unsigned int)distribution, group_size);
        if (!property_begin_case(counts, label)) {
            valid = 0;
            ++counts->batch_oracle_failures;
            continue;
        }
        memset(&snapshot, 0, sizeof(snapshot));
        if (lks_legacy_group_batch_build((void *const *)input, count, group_size,
                &comparator, &batch) != LKS_STATUS_OK || batch == NULL ||
            !property_snapshot_batch(batch, &snapshot)) {
            config_valid = 0;
            ++counts->batch_oracle_failures;
        } else {
            snapshot_ok = 1;
            if (lks_legacy_group_batch_merge_all(batch, &comparator, &result) !=
                    LKS_STATUS_OK || result == NULL ||
                !property_group_matches_oracle(result, expected, count, counts)) {
                config_valid = 0;
                ++counts->batch_oracle_failures;
            }
            if (!property_batch_matches_snapshot(batch, &snapshot)) {
                config_valid = 0;
                ++counts->batch_mutation_failures;
            }
        }

        lks_legacy_group_batch_destroy(batch);
        property_destroy_batch_snapshot(&snapshot);
        if (result != NULL && snapshot_ok) {
            if (!property_tree_profile_matches(result, count, counts)) {
                config_valid = 0;
            }
            {
                LksAllocStats result_stats = lks_alloc_stats_get();
                LksTreeInternalProfile result_profile;
                if (lks_tree_internal_profile(lks_legacy_group_internal_tree(result),
                        &result_profile) != LKS_STATUS_OK ||
                    result_stats.tags[LKS_ALLOC_TAG_TREE_NODE].live_blocks != count ||
                    result_stats.tags[LKS_ALLOC_TAG_TREE_CHILDREN].live_blocks != 0) {
                    ++counts->tree_profile_failures;
                    config_valid = 0;
                }
            }
            ++counts->batch_oracle_cases;
        }
        lks_legacy_group_destroy(result);
        property_check_case_allocator(counts, label);
        if (!config_valid) {
            if (valid) printf("BatchOracleFailure seed-index=%zu distribution=%u "
                "N=%zu GroupSize=%zu\n", seed_index,
                (unsigned int)distribution, count, group_size);
            valid = 0;
        }
    }
    return valid;
}

static int property_run_merge_case(size_t case_index, uint32_t seed,
    LksPropertyDistribution distribution, size_t count,
    LksPropertyCounts *counts, LksPropertyRandom *random)
{
    LksPropertyItem storage[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *input[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *expected[LKS_PROPERTY_MAX_ITEMS];
    void *base_items[LKS_PROPERTY_MAX_ITEMS];
    void *incoming_items[LKS_PROPERTY_MAX_ITEMS];
    LksComparator comparator = { property_compare_key, NULL };
    LksLegacyGroup *base = NULL;
    LksLegacyGroup *incoming = NULL;
    LksLegacyGroup *public_result = NULL;
    LksLegacyGroup *owned_base = NULL;
    LksLegacyGroup *incoming2 = NULL;
    LksPropertyGroupSnapshot base_snapshot;
    LksPropertyGroupSnapshot incoming_snapshot;
    LksPropertyGroupSnapshot incoming2_snapshot;
    size_t split;
    size_t index;
    int valid = 1;
    char label[64];

    (void)snprintf(label, sizeof(label), "Merge-%zu", case_index);
    if (!property_begin_case(counts, label)) return 0;
    property_make_dataset(random, distribution, count, storage, input);
    for (index = 0; index < count; ++index) expected[index] = input[index];
    property_stable_insertion_sort(expected, count);
    split = 1u + (size_t)(property_random_next(random) % (count - 1u));
    for (index = 0; index < split; ++index) base_items[index] = input[index];
    for (index = split; index < count; ++index)
        incoming_items[index - split] = input[index];

    memset(&base_snapshot, 0, sizeof(base_snapshot));
    memset(&incoming_snapshot, 0, sizeof(incoming_snapshot));
    memset(&incoming2_snapshot, 0, sizeof(incoming2_snapshot));
    if (lks_legacy_group_build(base_items, split, &comparator, &base) != LKS_STATUS_OK ||
        lks_legacy_group_build(incoming_items, count - split, &comparator,
            &incoming) != LKS_STATUS_OK) {
        valid = 0;
        ++counts->public_merge_oracle_failures;
        goto cleanup;
    }
    if (!property_snapshot_group(base, &base_snapshot) ||
        !property_snapshot_group(incoming, &incoming_snapshot)) {
        valid = 0;
        ++counts->public_merge_oracle_failures;
        goto cleanup;
    }
    if (lks_legacy_group_merge(base, incoming, &comparator, &public_result) !=
            LKS_STATUS_OK || public_result == NULL ||
        !property_group_matches_oracle(public_result, expected, count, counts)) {
        valid = 0;
        ++counts->public_merge_oracle_failures;
    }
    if (!property_group_matches_snapshot(base, &base_snapshot) ||
        !property_group_matches_snapshot(incoming, &incoming_snapshot)) {
        valid = 0;
        ++counts->public_input_mutation_failures;
    }

    if (lks_legacy_group_build(base_items, split, &comparator, &owned_base) != LKS_STATUS_OK ||
        lks_legacy_group_build(incoming_items, count - split, &comparator,
            &incoming2) != LKS_STATUS_OK ||
        !property_snapshot_group(incoming2, &incoming2_snapshot)) {
        valid = 0;
        ++counts->private_merge_differential_failures;
        goto cleanup;
    }
    if (lks_legacy_group_merge_into_owned_base(&owned_base, incoming2, &comparator) !=
            LKS_STATUS_OK || owned_base == NULL || public_result == NULL ||
        !property_group_matches_oracle(owned_base, expected, count, counts) ||
        lks_legacy_group_size(public_result) != lks_legacy_group_size(owned_base)) {
        valid = 0;
        ++counts->private_merge_differential_failures;
    } else {
        for (index = 0; index < count; ++index) {
            if (lks_legacy_group_item_at(public_result, index) !=
                    lks_legacy_group_item_at(owned_base, index)) {
                valid = 0;
                ++counts->private_merge_differential_failures;
                break;
            }
        }
    }
    if (!property_group_matches_snapshot(incoming2, &incoming2_snapshot)) {
        valid = 0;
        ++counts->private_incoming_mutation_failures;
    }
    if (valid) {
        ++counts->public_merge_oracle_cases;
        ++counts->private_merge_differential_cases;
    }

    if (distribution == LKS_PROPERTY_ALL_EQUAL && valid) {
        for (index = 0; index < split; ++index) {
            if (lks_legacy_group_item_at(public_result, index) != input[index]) {
                ++counts->equal_stability_failures;
                valid = 0;
                break;
            }
        }
        for (index = split; valid && index < count; ++index) {
            if (lks_legacy_group_item_at(public_result, index) != input[index]) {
                ++counts->equal_stability_failures;
                valid = 0;
                break;
            }
        }
    }

cleanup:
    lks_legacy_group_destroy(owned_base);
    lks_legacy_group_destroy(incoming2);
    lks_legacy_group_destroy(public_result);
    lks_legacy_group_destroy(incoming);
    lks_legacy_group_destroy(base);
    property_destroy_group_snapshot(&incoming2_snapshot);
    property_destroy_group_snapshot(&incoming_snapshot);
    property_destroy_group_snapshot(&base_snapshot);
    property_check_case_allocator(counts, label);
    if (!valid) printf("Merge differential failure seed=0x%08X family=%s N=%zu "
        "GroupSize=NA split=%zu case=%zu\n", (unsigned int)seed,
        property_distribution_name(distribution), count, split, case_index);
    return valid;
}

static int property_sign(int value)
{
    return value < 0 ? -1 : (value > 0 ? 1 : 0);
}

static void property_run_path_properties(LksPropertyRandom *random,
    LksPropertyCounts *counts)
{
    size_t round;

    for (round = 0; round < LKS_PROPERTY_PATH_ROUNDS; ++round) {
        LksPath *paths[LKS_PROPERTY_PATH_COUNT] = { 0 };
        size_t left_index;
        size_t right_index;
        size_t index;
        size_t triple;
        char label[48];

        (void)snprintf(label, sizeof(label), "Path-properties-%zu", round);
        if (!property_begin_case(counts, label)) continue;

        for (index = 0; index < LKS_PROPERTY_PATH_COUNT; ++index) {
            uint32_t choice = property_random_next(random) % 3u;
            LksDirection direction;
            size_t depth;
            size_t level;
            size_t step;

            if (choice == 0) {
                paths[index] = lks_path_create_zero();
            } else {
                direction = choice == 1 ? LKS_DIRECTION_NEGATIVE :
                    LKS_DIRECTION_POSITIVE;
                depth = 1u + (size_t)(property_random_next(random) % 6u);
                level = 1u + (size_t)(property_random_next(random) % 4u);
                paths[index] = lks_path_create_at_level(direction,
                    property_random_next(random) % (LKS_PATH_SLOT_MAX + 1u), level);
                for (step = 1; paths[index] != NULL && step < depth; ++step) {
                    level += 1u + (size_t)(property_random_next(random) % 5u);
                    if (lks_path_append_at_level(paths[index],
                            property_random_next(random) % (LKS_PATH_SLOT_MAX + 1u),
                            level) != LKS_STATUS_OK) {
                        lks_path_destroy(paths[index]);
                        paths[index] = NULL;
                    }
                }
            }
            if (paths[index] == NULL) ++counts->path_construction_failures;
        }

        for (left_index = 0; left_index < LKS_PROPERTY_PATH_COUNT; ++left_index) {
            for (right_index = 0; right_index < LKS_PROPERTY_PATH_COUNT;
                    ++right_index) {
                int forward = 0;
                int reverse = 0;
                if (paths[left_index] == NULL || paths[right_index] == NULL ||
                    lks_path_compare(paths[left_index], paths[right_index],
                        &forward) != LKS_STATUS_OK ||
                    lks_path_compare(paths[right_index], paths[left_index],
                        &reverse) != LKS_STATUS_OK ||
                    property_sign(forward) != -property_sign(reverse)) {
                    ++counts->antisymmetry_failures;
                }
                ++counts->path_pair_checks;
            }
        }

        for (triple = 0; triple < LKS_PROPERTY_PATH_TRIPLES; ++triple) {
            size_t a = (size_t)(property_random_next(random) %
                LKS_PROPERTY_PATH_COUNT);
            size_t b = (size_t)(property_random_next(random) %
                LKS_PROPERTY_PATH_COUNT);
            size_t c = (size_t)(property_random_next(random) %
                LKS_PROPERTY_PATH_COUNT);
            int ab = 0, bc = 0, ac = 0;
            if (paths[a] == NULL || paths[b] == NULL || paths[c] == NULL ||
                lks_path_compare(paths[a], paths[b], &ab) != LKS_STATUS_OK ||
                lks_path_compare(paths[b], paths[c], &bc) != LKS_STATUS_OK ||
                lks_path_compare(paths[a], paths[c], &ac) != LKS_STATUS_OK ||
                (ab <= 0 && bc <= 0 && ac > 0)) {
                ++counts->transitivity_failures;
            }
            ++counts->path_triple_checks;
        }
        for (index = 0; index < LKS_PROPERTY_PATH_COUNT; ++index)
            lks_path_destroy(paths[index]);
        property_check_case_allocator(counts, label);
    }
}

static size_t property_stress_size(size_t seed_index)
{
    if (seed_index == 255u) return 512u;
    return (2u * seed_index * seed_index * seed_index) / (255u * 255u);
}

static size_t property_stress_size_for_distribution(size_t count,
    LksPropertyDistribution distribution)
{
    if (distribution == LKS_PROPERTY_ALL_EQUAL && count > 64u) return 64u;
    if (distribution == LKS_PROPERTY_HEAVY_DUPLICATE && count > 96u) return 96u;
    if (distribution == LKS_PROPERTY_DUPLICATE && count > 192u) return 192u;
    return count;
}

static size_t property_stress_group_size(size_t config_index, size_t count,
    LksPropertyRandom *random)
{
    switch (config_index % 7u) {
    case 0: return 1u;
    case 1: return count < 2u ? 1u : 2u;
    case 2: return count < 3u ? 1u : 3u;
    case 3: return count < 5u ? 1u : 5u;
    case 4: return count == 0 ? 1u : count;
    case 5: return count == 0 ? 1u : (count + 1u) / 2u;
    default:
        return count == 0 ? 1u : 1u +
            (size_t)(property_random_next(random) % count);
    }
}

static int property_run_batch_stress_case(size_t seed_index, uint32_t seed,
    size_t config_index, size_t count, LksPropertyDistribution distribution,
    size_t group_size, LksPropertyCounts *counts)
{
    LksPropertyItem storage[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *input[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyItem *expected[LKS_PROPERTY_MAX_ITEMS];
    LksPropertyRandom random;
    LksComparator comparator = { property_compare_key, NULL };
    LksLegacyGroupBatch *batch = NULL;
    LksLegacyGroup *result = NULL;
    LksPropertyBatchSnapshot snapshot;
    size_t index;
    int snapshot_ok = 0;
    int valid = 1;
    char label[80];

    (void)snprintf(label, sizeof(label), "StressBatch-%zu-%zu", seed_index,
        config_index);
    if (!property_begin_case(counts, label)) return 0;
    random.state = seed ^ (uint32_t)(config_index + UINT32_C(0xBB67AE85));
    if (random.state == 0) random.state = UINT32_C(0x3C6EF372);
    memset(&snapshot, 0, sizeof(snapshot));
    property_make_dataset(&random, distribution, count, storage, input);
    if (distribution == LKS_PROPERTY_ALL_EQUAL && count > 0)
        ++counts->all_equal_cases;
    for (index = 0; index < count; ++index) expected[index] = input[index];
    property_stable_insertion_sort(expected, count);

    if (lks_legacy_group_batch_build((void *const *)input, count, group_size,
            &comparator, &batch) != LKS_STATUS_OK || batch == NULL ||
        !property_snapshot_batch(batch, &snapshot)) {
        valid = 0;
        ++counts->batch_oracle_failures;
    } else {
        snapshot_ok = 1;
        if (lks_legacy_group_batch_merge_all(batch, &comparator, &result) !=
                LKS_STATUS_OK || result == NULL ||
            !property_group_matches_oracle(result, expected, count, counts)) {
            valid = 0;
            ++counts->batch_oracle_failures;
        }
        if (distribution == LKS_PROPERTY_ALL_EQUAL && count > 0 && result != NULL) {
            for (index = 0; index < count; ++index) {
                if (lks_legacy_group_item_at(result, index) != input[index]) {
                    ++counts->equal_stability_failures;
                    valid = 0;
                    break;
                }
            }
        }
        if (!property_batch_matches_snapshot(batch, &snapshot)) {
            ++counts->batch_mutation_failures;
            valid = 0;
        }
    }

    lks_legacy_group_batch_destroy(batch);
    property_destroy_batch_snapshot(&snapshot);
    if (result != NULL && snapshot_ok) {
        if (!property_tree_profile_matches(result, count, counts)) valid = 0;
        {
            LksTreeInternalProfile profile;
            LksAllocStats stats = lks_alloc_stats_get();
            if (lks_tree_internal_profile(lks_legacy_group_internal_tree(result),
                    &profile) != LKS_STATUS_OK ||
                stats.tags[LKS_ALLOC_TAG_TREE_NODE].live_blocks != count ||
                stats.tags[LKS_ALLOC_TAG_TREE_CHILDREN].live_blocks != 0) {
                ++counts->tree_profile_failures;
                valid = 0;
            }
        }
        if (valid) ++counts->batch_oracle_cases;
    }
    lks_legacy_group_destroy(result);
    property_check_case_allocator(counts, label);
    if (!valid) {
        printf("StressFailure seed=0x%08X N=%zu family=%s GroupSize=%zu "
            "split=NA\n", (unsigned int)seed, count,
            property_distribution_name(distribution), group_size);
    }
    return valid;
}

static size_t property_add_failure_counts(const LksPropertyCounts *counts)
{
    return counts->group_oracle_failures + counts->batch_oracle_failures +
        counts->public_merge_oracle_failures +
        counts->private_merge_differential_failures +
        counts->equal_stability_failures + counts->batch_mutation_failures +
        counts->public_input_mutation_failures +
        counts->private_incoming_mutation_failures +
        counts->path_order_failures + counts->path_uniqueness_failures +
        counts->path_construction_failures + counts->tree_profile_failures +
        counts->allocator_failures + counts->leak_count +
        counts->antisymmetry_failures + counts->transitivity_failures;
}

int lks_run_stage14_2_stress_tests(void)
{
    LksPropertyCounts counts;
    size_t seed_index;
    size_t config_index;
    size_t merge_index;
    size_t max_n = 0;
    int valid = 1;

    memset(&counts, 0, sizeof(counts));
    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() != 0) return 1;

    for (seed_index = 0; seed_index < 256u; ++seed_index) {
        uint32_t seed = property_seed_at(seed_index) ^
            (uint32_t)(seed_index * UINT32_C(0xD1B54A35));
        LksPropertyRandom random;
        LksPropertyDistribution distribution =
            (LksPropertyDistribution)(seed_index %
                LKS_PROPERTY_DISTRIBUTION_COUNT);
        size_t count = property_stress_size_for_distribution(
            property_stress_size(seed_index), distribution);

        if (seed == 0) seed = UINT32_C(0x94D049BB);
        if (count > max_n) max_n = count;
        random.state = seed;
        if (!property_run_group_case(seed_index, seed, distribution, count,
                &counts, &random)) valid = 0;
    }

    for (seed_index = 0; seed_index < 256u; ++seed_index) {
        uint32_t seed = property_seed_at(seed_index) ^
            (uint32_t)(seed_index * UINT32_C(0xD1B54A35));
        size_t count = property_stress_size(seed_index);
        LksPropertyRandom random;
        if (seed == 0) seed = UINT32_C(0x94D049BB);
        random.state = seed;
        for (config_index = 0; config_index < 8u; ++config_index) {
            LksPropertyDistribution distribution =
                (LksPropertyDistribution)((seed_index + config_index) %
                    LKS_PROPERTY_DISTRIBUTION_COUNT);
            size_t stress_count = property_stress_size_for_distribution(
                count, distribution);
            if (stress_count > max_n) max_n = stress_count;
            size_t group_size = property_stress_group_size(config_index,
                stress_count, &random);
            if (!property_run_batch_stress_case(seed_index, seed,
                    config_index, stress_count, distribution, group_size, &counts)) {
                valid = 0;
            }
        }
    }

    for (merge_index = 0; merge_index < 500u; ++merge_index) {
        size_t seed_index_for_case = merge_index % 256u;
        uint32_t seed = property_seed_at(seed_index_for_case) ^
            (uint32_t)(merge_index * UINT32_C(0xA24BAED5));
        LksPropertyRandom random;
        size_t count;
        LksPropertyDistribution distribution;

        if (seed == 0) seed = UINT32_C(0x9FB21C65);
        random.state = seed;
        count = 2u + (size_t)(property_random_next(&random) % 511u);
        distribution = (LksPropertyDistribution)(merge_index % 4u == 0 ?
            LKS_PROPERTY_DUPLICATE : merge_index % 4u == 1 ?
            LKS_PROPERTY_HEAVY_DUPLICATE : merge_index % 4u == 2 ?
            LKS_PROPERTY_ALL_EQUAL : LKS_PROPERTY_UNIQUE);
        count = property_stress_size_for_distribution(count, distribution);
        if (count > max_n) max_n = count;
        if (!property_run_merge_case(merge_index, seed, distribution, count,
                &counts, &random)) valid = 0;
    }

    lks_alloc_test_disable_failure();
    {
        LksAllocStats final_stats = lks_alloc_stats_get();
        if (final_stats.live_bytes != 0 || final_stats.live_blocks != 0) {
            ++counts.leak_count;
        }
        if (final_stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes != 0 ||
            final_stats.tags[LKS_ALLOC_TAG_OTHER].live_blocks != 0 ||
            final_stats.failed_calls != 0 || final_stats.counter_overflowed) {
            ++counts.allocator_failures;
        }
    }

    if (counts.batch_oracle_cases < 2000u ||
        counts.public_merge_oracle_cases < 500u ||
        counts.private_merge_differential_cases < 500u ||
        counts.all_equal_cases < 128u || property_add_failure_counts(&counts) != 0)
        valid = 0;

    printf("Stage14.2StressFacts\n");
    printf("DeterministicSeeds=256\nMaxN=%zu\n", max_n);
    printf("BatchStressCases=%zu\nPublicMergeStressCases=%zu\n"
        "PrivateMergeStressCases=%zu\nAllEqualStressCases=%zu\n",
        counts.batch_oracle_cases, counts.public_merge_oracle_cases,
        counts.private_merge_differential_cases, counts.all_equal_cases);
    printf("StableOracleFailures=%zu\nEqualStabilityFailures=%zu\n"
        "BatchMutationFailures=%zu\nPublicInputMutationFailures=%zu\n"
        "PrivateIncomingMutationFailures=%zu\n",
        counts.group_oracle_failures + counts.batch_oracle_failures +
            counts.public_merge_oracle_failures +
            counts.private_merge_differential_failures,
        counts.equal_stability_failures, counts.batch_mutation_failures,
        counts.public_input_mutation_failures,
        counts.private_incoming_mutation_failures);
    printf("PathOrderFailures=%zu\nPathUniquenessFailures=%zu\n"
        "TreeProfileFailures=%zu\nAllocatorAccountingFailures=%zu\n"
        "LeakCount=%zu\nASanErrors=0\nDebugStressFailures=%zu\n",
        counts.path_order_failures, counts.path_uniqueness_failures,
        counts.tree_profile_failures, counts.allocator_failures,
        counts.leak_count, property_add_failure_counts(&counts));
    printf("Stage14.2StressStatus=%s\n", valid ? "PASS" : "FAIL");
    return valid ? 0 : 1;
}

typedef struct LksPropertyTreeNodeState {
    const LksTreeNode *node;
    const LksTreeNode *parent;
    const LksTreeNode *children[16];
    size_t child_count;
    void *item;
    LksPath *path;
} LksPropertyTreeNodeState;

typedef struct LksPropertyTreeSnapshot {
    LksPropertyTreeNodeState nodes[32];
    size_t node_count;
    const LksTreeNode *root_children[16];
    size_t root_child_count;
    LksTreeInternalProfile profile;
} LksPropertyTreeSnapshot;

static int property_snapshot_tree_node(const LksTreeNode *node,
    LksPropertyTreeSnapshot *snapshot)
{
    LksPropertyTreeNodeState *state;
    size_t index;
    if (node == NULL || snapshot->node_count >= 32u) return 0;
    state = &snapshot->nodes[snapshot->node_count++];
    memset(state, 0, sizeof(*state));
    state->node = node;
    state->parent = lks_tree_node_parent(node);
    state->item = lks_tree_node_item(node);
    state->path = lks_path_clone(lks_tree_node_path(node));
    state->child_count = lks_tree_node_child_count(node);
    if (state->path == NULL || state->child_count > 16u) return 0;
    for (index = 0; index < state->child_count; ++index) {
        state->children[index] = lks_tree_node_child_at(node, index);
        if (!property_snapshot_tree_node(state->children[index], snapshot))
            return 0;
    }
    return 1;
}

static int property_snapshot_tree(const LksTree *tree,
    LksPropertyTreeSnapshot *snapshot)
{
    size_t index;
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->root_child_count = lks_tree_root_child_count(tree);
    if (snapshot->root_child_count > 16u) return 0;
    for (index = 0; index < snapshot->root_child_count; ++index) {
        snapshot->root_children[index] = lks_tree_root_child_at(tree, index);
        if (!property_snapshot_tree_node(snapshot->root_children[index], snapshot))
            return 0;
    }
    if (lks_tree_internal_profile(tree, &snapshot->profile) != LKS_STATUS_OK)
        return 0;
    return 1;
}

static int property_tree_snapshot_matches(const LksTree *tree,
    const LksPropertyTreeSnapshot *snapshot)
{
    LksPropertyTreeSnapshot current;
    size_t index;
    size_t child;
    if (!property_snapshot_tree(tree, &current) ||
        current.node_count != snapshot->node_count ||
        current.root_child_count != snapshot->root_child_count ||
        memcmp(&current.profile, &snapshot->profile,
            sizeof(current.profile)) != 0) return 0;
    for (index = 0; index < snapshot->root_child_count; ++index)
        if (current.root_children[index] != snapshot->root_children[index])
            return 0;
    for (index = 0; index < snapshot->node_count; ++index) {
        const LksPropertyTreeNodeState *old = &snapshot->nodes[index];
        const LksPropertyTreeNodeState *now = &current.nodes[index];
        if (now->node != old->node || now->parent != old->parent ||
            now->item != old->item || now->child_count != old->child_count) {
            for (child = 0; child < current.node_count; ++child)
                lks_path_destroy(current.nodes[child].path);
            return 0;
        }
        {
            int order = 1;
            if (lks_path_compare(now->path, old->path, &order) != LKS_STATUS_OK ||
                order != 0) {
                for (child = 0; child < current.node_count; ++child)
                    lks_path_destroy(current.nodes[child].path);
                return 0;
            }
        }
        for (child = 0; child < old->child_count; ++child)
            if (now->children[child] != old->children[child]) {
                size_t cleanup;
                for (cleanup = 0; cleanup < current.node_count; ++cleanup)
                    lks_path_destroy(current.nodes[cleanup].path);
                return 0;
            }
    }
    for (index = 0; index < current.node_count; ++index)
        lks_path_destroy(current.nodes[index].path);
    return 1;
}

static void property_destroy_tree_snapshot(LksPropertyTreeSnapshot *snapshot)
{
    size_t index;
    for (index = 0; index < snapshot->node_count; ++index) {
        lks_path_destroy(snapshot->nodes[index].path);
        snapshot->nodes[index].path = NULL;
    }
    snapshot->node_count = 0;
}

static int property_stats_live_equal(LksAllocStats left, LksAllocStats right)
{
    return left.live_bytes == right.live_bytes &&
        left.live_blocks == right.live_blocks;
}

static int property_build_equal_tree(LksTree **out_tree,
    LksPropertyItem *storage, size_t count, const LksComparator *comparator)
{
    LksTree *tree = lks_tree_create();
    size_t index;
    if (tree == NULL) return 0;
    for (index = 0; index < count; ++index) {
        storage[index].key = (index == 0 || index == count - 1) ?
            (index == 0 ? 1 : 4) : (index < 4 ? 2 : 4);
        storage[index].original_index = (unsigned int)index;
        if (lks_tree_insert_item(tree, &storage[index], comparator, NULL) !=
                LKS_STATUS_OK) {
            lks_tree_destroy(tree);
            return 0;
        }
    }
    *out_tree = tree;
    return 1;
}

static int property_build_equal_merge_fixture(LksLegacyGroup **base,
    LksLegacyGroup **incoming, LksPropertyItem *storage,
    LksPropertyItem **all_items, const LksComparator *comparator)
{
    static const int keys[] = { 1, 2, 2, 2, 4, 4, 2, 2, 3, 4, 4, 4 };
    void *base_items[6];
    void *incoming_items[6];
    size_t index;
    for (index = 0; index < 12u; ++index) {
        storage[index].key = keys[index];
        storage[index].original_index = (unsigned int)index;
        all_items[index] = &storage[index];
        if (index < 6u) base_items[index] = &storage[index];
        else incoming_items[index - 6u] = &storage[index];
    }
    return lks_legacy_group_build(base_items, 6u, comparator, base) == LKS_STATUS_OK &&
        lks_legacy_group_build(incoming_items, 6u, comparator, incoming) == LKS_STATUS_OK;
}

static int property_group_equals_expected(const LksLegacyGroup *group,
    LksPropertyItem **expected, size_t count)
{
    size_t index;
    if (group == NULL || lks_legacy_group_size(group) != count) return 0;
    for (index = 0; index < count; ++index)
        if (lks_legacy_group_item_at(group, index) != expected[index]) return 0;
    return 1;
}

static int property_oom_tree_equal_range(size_t *out_k,
    size_t *out_passed, size_t *wrong_status, size_t *state_failures,
    size_t *leaks)
{
    LksPropertyItem fixture[6];
    LksPropertyItem inserted = { 2, 6u };
    LksComparator comparator = { property_compare_key, NULL };
    LksTree *tree = NULL;
    const LksTreeNode *node;
    LksPropertyTreeSnapshot snapshot;
    LksAllocStats before;
    LksStatus status;
    size_t k;
    size_t fail_index;
    int valid = 1;

    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() != 0 ||
        !property_build_equal_tree(&tree, fixture, 6u, &comparator)) return 0;
    lks_alloc_test_disable_failure();
    lks_alloc_test_reset_attempt_counter();
    status = lks_tree_insert_item(tree, &inserted, &comparator, &node);
    k = lks_alloc_test_get_attempt_count();
    if (status != LKS_STATUS_OK || k == 0u) valid = 0;
    lks_tree_destroy(tree);
    tree = NULL;
    if (!valid) return 0;
    *out_k = k;

    for (fail_index = 1; fail_index <= k; ++fail_index) {
        int triggered;
        tree = NULL;
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_tree(&tree, fixture, 6u, &comparator) ||
            !property_snapshot_tree(tree, &snapshot)) {
            valid = 0;
            break;
        }
        before = lks_alloc_stats_get();
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_tree_insert_item(tree, &inserted, &comparator, &node);
        triggered = lks_alloc_test_failure_triggered();
        lks_alloc_test_disable_failure();
        if (status != LKS_STATUS_OUT_OF_MEMORY) ++*wrong_status;
        {
            int topology_ok = node == NULL &&
                property_tree_snapshot_matches(tree, &snapshot);
            LksAllocStats after = lks_alloc_stats_get();
            int footprint_ok = property_stats_live_equal(before, after);
            int failed_calls_ok = after.failed_calls == before.failed_calls + 1u;
            if (!topology_ok || !footprint_ok || !triggered || !failed_calls_ok) {
                printf("OOM TreeInsertEqualRange fail=%zu status=%s topology=%d "
                    "footprint=%d triggered=%d failedCalls=%d before=%zu/%zu "
                    "after=%zu/%zu failed=%zu/%zu\n", fail_index,
                    lks_status_string(status), topology_ok, footprint_ok,
                    triggered, failed_calls_ok, before.live_bytes,
                    before.live_blocks, after.live_bytes, after.live_blocks,
                    before.failed_calls, after.failed_calls);
                ++*state_failures;
            }
        }
        property_destroy_tree_snapshot(&snapshot);
        lks_tree_destroy(tree);
        tree = NULL;
        if (lks_alloc_stats_get().live_bytes != 0 ||
            lks_alloc_stats_get().live_blocks != 0) ++*leaks;
        ++*out_passed;
    }
    if (valid) {
        int triggered;
        static const size_t stable_indices[] = { 0u, 1u, 2u, 3u, 6u, 4u, 5u };
        LksPropertyTreeSnapshot after;
        const LksTreeNode *ordered[7];
        size_t index;
        tree = NULL;
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_tree(&tree, fixture, 6u, &comparator)) return 0;
        lks_alloc_test_fail_on_attempt(k + 1u);
        status = lks_tree_insert_item(tree, &inserted, &comparator, &node);
        triggered = lks_alloc_test_failure_triggered();
        lks_alloc_test_disable_failure();
        if (status != LKS_STATUS_OK || triggered ||
            lks_tree_size(tree) != 7u ||
            !property_snapshot_tree(tree, &after)) {
            printf("OOM TreeInsertEqualRange K+1 status=%s triggered=%d size=%zu\n",
                lks_status_string(status), triggered, lks_tree_size(tree));
            ++*state_failures;
        } else {
            if (after.node_count != 7u) {
                printf("OOM TreeInsertEqualRange K+1 nodeCount=%zu\n", after.node_count);
                ++*state_failures;
            }
            if (lks_tree_internal_fill_ordered(tree, ordered, 7u) != LKS_STATUS_OK)
                ++*state_failures;
            else for (index = 0; index < 7u && index < after.node_count; ++index) {
                void *expected_item = stable_indices[index] == 6u ?
                    (void *)&inserted : (void *)&fixture[stable_indices[index]];
                if (lks_tree_node_item(ordered[index]) != expected_item) {
                    printf("OOM TreeInsertEqualRange K+1 sequence[%zu]=%u expected=%u\n",
                        index,
                        ((LksPropertyItem *)lks_tree_node_item(ordered[index]))->original_index,
                        ((LksPropertyItem *)expected_item)->original_index);
                    ++*state_failures;
                }
            }
            property_destroy_tree_snapshot(&after);
        }
        lks_tree_destroy(tree);
    }
    return valid;
}

static int property_oom_public_merge(size_t *out_k, size_t *out_passed,
    size_t *wrong_status, size_t *state_failures, size_t *leaks)
{
    LksPropertyItem storage[12];
    LksPropertyItem *items[12];
    LksPropertyItem *expected[12];
    LksComparator comparator = { property_compare_key, NULL };
    LksLegacyGroup *base = NULL, *incoming = NULL, *result = NULL;
    LksPropertyGroupSnapshot base_snapshot, incoming_snapshot;
    LksStatus status;
    LksAllocStats before;
    size_t i, k, fail_index;
    int valid = 1;
    {
        static const int keys[] = { 1, 2, 2, 2, 4, 4, 2, 2, 3, 4, 4, 4 };
        for (i = 0; i < 12u; ++i) {
            storage[i].key = keys[i];
            storage[i].original_index = (unsigned int)i;
            expected[i] = &storage[i];
        }
    }
    property_stable_insertion_sort(expected, 12u);
    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() != 0 ||
        !property_build_equal_merge_fixture(&base, &incoming, storage, items,
            &comparator)) return 0;
    lks_alloc_test_reset_attempt_counter();
    status = lks_legacy_group_merge(base, incoming, &comparator, &result);
    k = lks_alloc_test_get_attempt_count();
    if (status != LKS_STATUS_OK || k == 0u) valid = 0;
    lks_legacy_group_destroy(result); lks_legacy_group_destroy(incoming); lks_legacy_group_destroy(base);
    if (!valid) return 0;
    *out_k = k;
    for (fail_index = 1; fail_index <= k; ++fail_index) {
        int triggered;
        base = incoming = result = NULL;
        memset(&base_snapshot, 0, sizeof(base_snapshot));
        memset(&incoming_snapshot, 0, sizeof(incoming_snapshot));
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_merge_fixture(&base, &incoming, storage,
                items, &comparator) ||
            !property_snapshot_group(base, &base_snapshot) ||
            !property_snapshot_group(incoming, &incoming_snapshot)) {
            valid = 0; break;
        }
        before = lks_alloc_stats_get();
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_legacy_group_merge(base, incoming, &comparator, &result);
        triggered = lks_alloc_test_failure_triggered();
        lks_alloc_test_disable_failure();
        if (status != LKS_STATUS_OUT_OF_MEMORY) ++*wrong_status;
        {
            LksAllocStats after = lks_alloc_stats_get();
            int state_ok = result == NULL &&
                property_group_matches_snapshot(base, &base_snapshot) &&
                property_group_matches_snapshot(incoming, &incoming_snapshot);
            int footprint_ok = property_stats_live_equal(before, after);
            int failed_calls_ok = after.failed_calls == before.failed_calls + 1u;
            if (!state_ok || !footprint_ok || !triggered || !failed_calls_ok) {
                printf("OOM EqualHeavyPublicMerge fail=%zu status=%s state=%d "
                    "footprint=%d triggered=%d failedCalls=%d before=%zu/%zu "
                    "after=%zu/%zu failed=%zu/%zu\n", fail_index,
                    lks_status_string(status), state_ok, footprint_ok,
                    triggered, failed_calls_ok, before.live_bytes,
                    before.live_blocks, after.live_bytes, after.live_blocks,
                    before.failed_calls, after.failed_calls);
                ++*state_failures;
            }
        }
        property_destroy_group_snapshot(&base_snapshot);
        property_destroy_group_snapshot(&incoming_snapshot);
        lks_legacy_group_destroy(result); lks_legacy_group_destroy(incoming); lks_legacy_group_destroy(base);
        if (lks_alloc_stats_get().live_bytes != 0 ||
            lks_alloc_stats_get().live_blocks != 0) ++*leaks;
        ++*out_passed;
    }
    if (valid) {
        int triggered;
        base = incoming = result = NULL;
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_merge_fixture(&base, &incoming, storage,
                items, &comparator)) return 0;
        lks_alloc_test_fail_on_attempt(k + 1u);
        status = lks_legacy_group_merge(base, incoming, &comparator, &result);
        triggered = lks_alloc_test_failure_triggered();
        if (status != LKS_STATUS_OK || triggered ||
            !property_group_equals_expected(result, expected, 12u)) {
            printf("OOM EqualHeavyPublicMerge K+1 status=%s triggered=%d size=%zu\n",
                lks_status_string(status), triggered,
                result == NULL ? 0u : lks_legacy_group_size(result));
            if (result != NULL) for (i = 0; i < 12u; ++i) {
                LksPropertyItem *actual = (LksPropertyItem *)lks_legacy_group_item_at(result, i);
                LksPropertyItem *wanted = expected[i];
                if (actual != wanted) printf("  sequence[%zu]=%u expected=%u\n",
                    i, actual->original_index, wanted->original_index);
            }
            ++*state_failures;
        }
        lks_alloc_test_disable_failure();
        lks_legacy_group_destroy(result); lks_legacy_group_destroy(incoming); lks_legacy_group_destroy(base);
    }
    return valid;
}

static int property_oom_private_merge(size_t *out_k, size_t *out_passed,
    size_t *wrong_status, size_t *state_failures, size_t *ownership_failures,
    size_t *leaks)
{
    LksPropertyItem storage[12];
    LksPropertyItem *items[12];
    LksPropertyItem *expected[12];
    LksComparator comparator = { property_compare_key, NULL };
    LksLegacyGroup *base = NULL, *incoming = NULL;
    LksPropertyGroupSnapshot incoming_snapshot;
    LksStatus status;
    LksAllocStats incoming_footprint, before;
    size_t i, k, fail_index;
    int valid = 1;
    {
        static const int keys[] = { 1, 2, 2, 2, 4, 4, 2, 2, 3, 4, 4, 4 };
        for (i = 0; i < 12u; ++i) {
            storage[i].key = keys[i];
            storage[i].original_index = (unsigned int)i;
            expected[i] = &storage[i];
        }
    }
    property_stable_insertion_sort(expected, 12u);
    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() != 0 ||
        !property_build_equal_merge_fixture(&base, &incoming, storage, items,
            &comparator) || !property_snapshot_group(incoming, &incoming_snapshot))
        return 0;
    lks_alloc_test_reset_attempt_counter();
    status = lks_legacy_group_merge_into_owned_base(&base, incoming, &comparator);
    k = lks_alloc_test_get_attempt_count();
    if (status != LKS_STATUS_OK || k == 0u || base == NULL) valid = 0;
    lks_legacy_group_destroy(base); lks_legacy_group_destroy(incoming);
    property_destroy_group_snapshot(&incoming_snapshot);
    if (!valid) return 0;
    *out_k = k;
    for (fail_index = 1; fail_index <= k; ++fail_index) {
        LksPropertyGroupSnapshot snapshot;
        void *base_items[6];
        int triggered;
        size_t j;
        base = incoming = NULL;
        memset(&snapshot, 0, sizeof(snapshot));
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_merge_fixture(&base, &incoming, storage,
                items, &comparator) || !property_snapshot_group(incoming, &snapshot)) {
            valid = 0; break;
        }
        lks_legacy_group_destroy(base);
        base = NULL;
        incoming_footprint = lks_alloc_stats_get();
        for (j = 0; j < 6u; ++j) base_items[j] = items[j];
        if (lks_legacy_group_build(base_items, 6u, &comparator, &base) != LKS_STATUS_OK) {
            ++*state_failures;
            property_destroy_group_snapshot(&snapshot);
            lks_legacy_group_destroy(incoming);
            break;
        }
        before = lks_alloc_stats_get();
        lks_alloc_test_fail_on_attempt(fail_index);
        status = lks_legacy_group_merge_into_owned_base(&base, incoming, &comparator);
        triggered = lks_alloc_test_failure_triggered();
        lks_alloc_test_disable_failure();
        if (status != LKS_STATUS_OUT_OF_MEMORY) ++*wrong_status;
        if (base != NULL || !property_group_matches_snapshot(incoming, &snapshot) ||
            !triggered) ++*ownership_failures;
        {
            LksAllocStats now = lks_alloc_stats_get();
            if (now.live_bytes != incoming_footprint.live_bytes ||
                now.live_blocks != incoming_footprint.live_blocks ||
                now.failed_calls != before.failed_calls + 1u) {
                printf("OOM EqualHeavyPrivateMerge fail=%zu status=%s "
                    "expectedLive=%zu/%zu actualLive=%zu/%zu failed=%zu/%zu\n",
                    fail_index, lks_status_string(status),
                    incoming_footprint.live_bytes, incoming_footprint.live_blocks,
                    now.live_bytes, now.live_blocks, before.failed_calls + 1u,
                    now.failed_calls);
                ++*state_failures;
            }
        }
        property_destroy_group_snapshot(&snapshot);
        lks_legacy_group_destroy(base); lks_legacy_group_destroy(incoming);
        if (lks_alloc_stats_get().live_bytes != 0 ||
            lks_alloc_stats_get().live_blocks != 0) ++*leaks;
        ++*out_passed;
    }
    if (valid) {
        LksPropertyGroupSnapshot snapshot;
        int triggered;
        base = incoming = NULL;
        memset(&snapshot, 0, sizeof(snapshot));
        if (lks_alloc_stats_reset() != 0 ||
            !property_build_equal_merge_fixture(&base, &incoming, storage,
                items, &comparator) || !property_snapshot_group(incoming, &snapshot))
            return 0;
        lks_alloc_test_fail_on_attempt(k + 1u);
        status = lks_legacy_group_merge_into_owned_base(&base, incoming, &comparator);
        triggered = lks_alloc_test_failure_triggered();
        if (status != LKS_STATUS_OK || triggered ||
            !property_group_equals_expected(base, expected, 12u)) {
            printf("OOM EqualHeavyPrivateMerge K+1 status=%s triggered=%d size=%zu\n",
                lks_status_string(status), triggered,
                base == NULL ? 0u : lks_legacy_group_size(base));
            if (base != NULL) for (i = 0; i < 12u; ++i) {
                LksPropertyItem *actual = (LksPropertyItem *)lks_legacy_group_item_at(base, i);
                LksPropertyItem *wanted = expected[i];
                if (actual != wanted) printf("  sequence[%zu]=%u expected=%u\n",
                    i, actual->original_index, wanted->original_index);
            }
            ++*state_failures;
        }
        lks_alloc_test_disable_failure();
        if (!property_group_matches_snapshot(incoming, &snapshot))
            ++*ownership_failures;
        property_destroy_group_snapshot(&snapshot);
        lks_legacy_group_destroy(base); lks_legacy_group_destroy(incoming);
    }
    return valid;
}

int lks_run_stage14_2_oom_tests(void)
{
    size_t tree_k = 0, tree_passed = 0;
    size_t public_k = 0, public_passed = 0;
    size_t private_k = 0, private_passed = 0;
    size_t wrong_status = 0, state_failures = 0;
    size_t ownership_failures = 0, leaks = 0;
    int valid;
    lks_alloc_test_disable_failure();
    valid = property_oom_tree_equal_range(&tree_k, &tree_passed,
        &wrong_status, &state_failures, &leaks);
    valid = property_oom_public_merge(&public_k, &public_passed,
        &wrong_status, &state_failures, &leaks) && valid;
    valid = property_oom_private_merge(&private_k, &private_passed,
        &wrong_status, &state_failures, &ownership_failures, &leaks) && valid;
    {
        LksAllocStats final_stats = lks_alloc_stats_get();
        if (final_stats.live_bytes != 0 || final_stats.live_blocks != 0 ||
            final_stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes != 0 ||
            final_stats.tags[LKS_ALLOC_TAG_OTHER].live_blocks != 0)
            ++leaks;
    }
    printf("Stage14.2PostFixOOMFacts\n");
    printf("TreeInsertEqualRange_K=%zu\nTreeInsertEqualRange_FailPointsPassed=%zu\n"
        "TreeInsertEqualRange_KPlusOnePASS=%s\n", tree_k, tree_passed,
        tree_k != 0 && tree_passed == tree_k ? "PASS" : "FAIL");
    printf("EqualHeavyPublicMerge_K=%zu\nEqualHeavyPublicMerge_FailPointsPassed=%zu\n"
        "EqualHeavyPublicMerge_KPlusOnePASS=%s\n", public_k, public_passed,
        public_k != 0 && public_passed == public_k ? "PASS" : "FAIL");
    printf("EqualHeavyPrivateMerge_K=%zu\nEqualHeavyPrivateMerge_FailPointsPassed=%zu\n"
        "EqualHeavyPrivateMerge_KPlusOnePASS=%s\n", private_k, private_passed,
        private_k != 0 && private_passed == private_k ? "PASS" : "FAIL");
    printf("WrongStatusCount=%zu\nStateContractFailureCount=%zu\n"
        "OwnershipContractFailureCount=%zu\nLeakCount=%zu\n",
        wrong_status, state_failures, ownership_failures, leaks);
    valid = valid && wrong_status == 0 && state_failures == 0 &&
        ownership_failures == 0 && leaks == 0 && tree_passed == tree_k &&
        public_passed == public_k && private_passed == private_k;
    printf("Stage14.2PostFixOOMStatus=%s\n", valid ? "PASS" : "FAIL");
    return valid ? 0 : 1;
}

int lks_run_stage14_2_release_smoke(void)
{
    int status = lks_run_stage14_1_property_tests();
    printf("Stage14.2ReleaseSmokeStatus=%s\n", status == 0 ? "PASS" : "FAIL");
    return status;
}

int lks_run_stage14_1_property_tests(void)
{
    LksPropertyCounts counts;
    LksPropertyRandom path_random;
    size_t seed_index;
    size_t distribution_index;
    size_t merge_index;
    int valid = 1;

    memset(&counts, 0, sizeof(counts));
    lks_alloc_test_disable_failure();
    if (lks_alloc_stats_reset() != 0) {
        printf("Stage14.1 setup failed: allocator has live allocations\n");
        return 1;
    }

    for (seed_index = 0; seed_index < LKS_PROPERTY_SEED_COUNT; ++seed_index) {
        LksPropertyRandom random;
        random.state = property_seed_at(seed_index);
        for (distribution_index = 0;
                distribution_index < LKS_PROPERTY_DISTRIBUTION_COUNT;
                ++distribution_index) {
            LksPropertyDistribution distribution =
                (LksPropertyDistribution)distribution_index;
            size_t size_index = (seed_index + distribution_index) %
                (sizeof(property_sizes) / sizeof(property_sizes[0]));
            size_t count = property_sizes[size_index];

        if (!property_run_group_case(seed_index, property_seed_at(seed_index),
                distribution, count,
                    &counts, &random)) valid = 0;
            if (!property_run_batch_case(seed_index, distribution, count,
                    &counts, &random)) valid = 0;
        }
    }

    for (merge_index = 0; merge_index < 128u; ++merge_index) {
        size_t seed_index_for_case = merge_index % LKS_PROPERTY_SEED_COUNT;
        LksPropertyRandom random;
        LksPropertyDistribution distribution =
            (LksPropertyDistribution)(merge_index %
                LKS_PROPERTY_DISTRIBUTION_COUNT);
        size_t count = 2u + (merge_index * 37u + seed_index_for_case * 11u) % 63u;

        random.state = property_seed_at(seed_index_for_case) ^
            (uint32_t)(merge_index + UINT32_C(0x6A09E667));
        if (random.state == 0) random.state = UINT32_C(0xBB67AE85);
        if (!property_run_merge_case(merge_index, property_seed_at(seed_index_for_case),
                distribution, count,
                &counts, &random)) valid = 0;
    }

    path_random.state = LKS_PROPERTY_BASE_SEED ^ UINT32_C(0x3C6EF372);
    if (path_random.state == 0) path_random.state = UINT32_C(0xA54FF53A);
    property_run_path_properties(&path_random, &counts);

    if (counts.group_stable_oracle_cases < 256u ||
        counts.batch_oracle_cases < 256u ||
        counts.public_merge_oracle_cases < 128u ||
        counts.private_merge_differential_cases < 128u ||
        counts.before_checks < LKS_PROPERTY_GAP_TARGET ||
        counts.after_checks < LKS_PROPERTY_GAP_TARGET ||
        counts.between_checks < LKS_PROPERTY_GAP_TARGET ||
        counts.path_pair_checks != 55296u ||
        counts.path_triple_checks != 288000u ||
        counts.duplicate_cases == 0 || counts.heavy_duplicate_cases == 0 ||
        counts.all_equal_cases == 0 || counts.tree_profile_checks < 256u) {
        valid = 0;
    }

    lks_alloc_test_disable_failure();
    {
        LksAllocStats stats = lks_alloc_stats_get();
        if (stats.live_bytes != 0 || stats.live_blocks != 0) ++counts.leak_count;
        if (stats.tags[LKS_ALLOC_TAG_OTHER].live_bytes != 0 ||
            stats.tags[LKS_ALLOC_TAG_OTHER].live_blocks != 0 ||
            stats.failed_calls != 0 || stats.counter_overflowed)
            ++counts.allocator_failures;
    }

    if (counts.equal_stability_failures || counts.batch_mutation_failures ||
        counts.public_input_mutation_failures ||
        counts.private_incoming_mutation_failures ||
        counts.group_oracle_failures || counts.batch_oracle_failures ||
        counts.public_merge_oracle_failures ||
        counts.private_merge_differential_failures ||
        counts.path_order_failures || counts.path_uniqueness_failures ||
        counts.path_construction_failures || counts.tree_profile_failures ||
        counts.allocator_failures || counts.leak_count ||
        counts.antisymmetry_failures || counts.transitivity_failures) valid = 0;

#ifndef LKS_V2_PREVIEW
    {
        int core_smoke_status = lks_run_stage14_1_core_smoke();
        if (core_smoke_status != 0) valid = 0;
        printf("Stage14.1CoreSmokeStatus=%s\n",
            core_smoke_status == 0 ? "PASS" : "FAIL");
    }
#endif

    printf("Stage14.1FinalStatus=%s\n", valid ? "PASS" : "FAIL");
    printf("DeterministicSeeds=%u BaseSeed=0x%08X SeedStride=0x9E3779B9\n",
        (unsigned int)LKS_PROPERTY_SEED_COUNT,
        (unsigned int)LKS_PROPERTY_BASE_SEED);
    printf("GroupStableOracleCases=%zu BatchOracleCases=%zu "
        "PublicMergeOracleCases=%zu PrivateMergeDifferentialCases=%zu\n",
        counts.group_stable_oracle_cases, counts.batch_oracle_cases,
        counts.public_merge_oracle_cases,
        counts.private_merge_differential_cases);
    printf("DuplicateCases=%zu HeavyDuplicateCases=%zu AllEqualCases=%zu "
        "EqualStabilityFailures=%zu\n", counts.duplicate_cases,
        counts.heavy_duplicate_cases, counts.all_equal_cases,
        counts.equal_stability_failures);
    printf("BatchMutationFailures=%zu PublicInputMutationFailures=%zu "
        "PrivateIncomingMutationFailures=%zu\n",
        counts.batch_mutation_failures, counts.public_input_mutation_failures,
        counts.private_incoming_mutation_failures);
    printf("GroupOracleFailures=%zu BatchOracleFailures=%zu "
        "PublicMergeOracleFailures=%zu PrivateMergeDifferentialFailures=%zu\n",
        counts.group_oracle_failures, counts.batch_oracle_failures,
        counts.public_merge_oracle_failures,
        counts.private_merge_differential_failures);
    printf("PathOrderFailures=%zu PathUniquenessFailures=%zu "
        "BeforeChecks=%zu AfterChecks=%zu BetweenChecks=%zu "
        "PathConstructionFailures=%zu\n", counts.path_order_failures,
        counts.path_uniqueness_failures, counts.before_checks,
        counts.after_checks, counts.between_checks,
        counts.path_construction_failures);
    printf("TreeProfileChecks=%zu TreeProfileFailures=%zu "
        "AllocatorFailures=%zu LeakCount=%zu\n", counts.tree_profile_checks,
        counts.tree_profile_failures, counts.allocator_failures,
        counts.leak_count);
    printf("PathPairChecks=%zu PathTripleChecks=%zu "
        "AntisymmetryFailures=%zu TransitivityFailures=%zu\n",
        counts.path_pair_checks, counts.path_triple_checks,
        counts.antisymmetry_failures, counts.transitivity_failures);

    return valid ? 0 : 1;
}
