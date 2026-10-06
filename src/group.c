#include <stdlib.h>
#include "lks_legacy_internal.h"
#include "lks_alloc_internal.h"
#include "lks_group_internal.h"
#include "lks_bulk_internal.h"
#include "lks_sort_internal.h"
#include "lks_tree_internal.h"

struct LksLegacyGroupBatch {
    LksLegacyGroup **groups;
    size_t group_count;
    size_t total_count;
    size_t group_size;
};

struct LksLegacyGroup {
    LksTree *tree;
    const LksTreeNode **ordered_nodes;
    size_t count;
};

const LksTree *lks_legacy_group_internal_tree(const LksLegacyGroup *group)
{
    return group == NULL ? NULL : group->tree;
}

static LksStatus validate_ordered_nodes(
    const LksTreeNode **ordered_nodes,
    size_t count,
    const LksComparator *comparator
)
{
    size_t index;

    for (index = 1; index < count; ++index) {
        const LksPath *previous_path;
        const LksPath *current_path;
        int path_order;
        int item_order;
        LksStatus status;

        previous_path = lks_tree_node_path(ordered_nodes[index - 1]);
        current_path = lks_tree_node_path(ordered_nodes[index]);
        if (previous_path == NULL || current_path == NULL) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        status = lks_path_compare(previous_path, current_path, &path_order);
        if (status != LKS_STATUS_OK || path_order != -1) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        item_order = comparator->compare(
            lks_tree_node_item(ordered_nodes[index - 1]),
            lks_tree_node_item(ordered_nodes[index]),
            comparator->context);
        if (item_order > 0) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
    }
    return LKS_STATUS_OK;
}

static LksStatus group_build_ordered_nodes(const LksLegacyGroup *group,
    const LksComparator *comparator, const LksTreeNode ***out_ordered_nodes)
{
    const LksTreeNode **ordered_nodes;
    LksStatus status;

    if (out_ordered_nodes == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_ordered_nodes = NULL;
    if (group->count == 0) return LKS_STATUS_OK;
    if (group->count > ((size_t)-1) / sizeof(*group->ordered_nodes)) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    ordered_nodes = (const LksTreeNode **)lks_alloc_tagged(
        group->count * sizeof(*ordered_nodes), LKS_ALLOC_TAG_GROUP_ORDERED);
    if (ordered_nodes == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }

    status = lks_tree_internal_fill_ordered(group->tree,
        ordered_nodes, group->count);
    if (status != LKS_STATUS_OK) {
        lks_free(ordered_nodes);
        return status;
    }
    status = validate_ordered_nodes(ordered_nodes, group->count, comparator);
    if (status != LKS_STATUS_OK) {
        lks_free(ordered_nodes);
        return status;
    }
    *out_ordered_nodes = ordered_nodes;
    return LKS_STATUS_OK;
}

static LksStatus group_build_ordered_view(LksLegacyGroup *group,
    const LksComparator *comparator)
{
    const LksTreeNode **ordered_nodes;
    LksStatus status = group_build_ordered_nodes(group, comparator, &ordered_nodes);
    if (status != LKS_STATUS_OK) return status;
    group->ordered_nodes = ordered_nodes;
    return LKS_STATUS_OK;
}

static LksStatus group_from_sorted(void *const *items, size_t count,
    const LksComparator *comparator, LksLegacyGroup **out_group)
{
    LksLegacyGroup *group;
    LksStatus status;
    LksTree *tree = NULL;
    status = lks_bulk_build_tree(items, count, &tree);
    if (status != LKS_STATUS_OK) return status;
    group = (LksLegacyGroup *)lks_alloc_tagged(sizeof(*group), LKS_ALLOC_TAG_GROUP_OBJECT);
    if (group == NULL) {
        lks_tree_destroy(tree);
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    group->tree = tree;
    group->ordered_nodes = NULL;
    group->count = count;
    status = group_build_ordered_view(group, comparator);
    if (status != LKS_STATUS_OK) {
        lks_legacy_group_destroy(group);
        return status;
    }
    *out_group = group;
    return LKS_STATUS_OK;
}

LksStatus lks_legacy_group_build(void *const *items, size_t count,
    const LksComparator *comparator, LksLegacyGroup **out_group)
{
    void **sorted = NULL;
    LksStatus status;
    if (out_group == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_group = NULL;
    status = lks_stable_sort_copy(items, count, comparator, &sorted);
    if (status != LKS_STATUS_OK) return status;
    /* Nothing borrowed from the caller is ever rewritten. */
    status = group_from_sorted(sorted, count, comparator, out_group);
    lks_free(sorted);
    return status;
}

void lks_legacy_group_destroy(LksLegacyGroup *group)
{
    if (group == NULL) {
        return;
    }
    lks_free(group->ordered_nodes);
    lks_tree_destroy(group->tree);
    lks_free(group);
}

size_t lks_legacy_group_size(const LksLegacyGroup *group)
{
    return group == NULL ? 0 : group->count;
}

void *lks_legacy_group_item_at(const LksLegacyGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return lks_tree_node_item(group->ordered_nodes[index]);
}

const LksPath *lks_legacy_group_path_at(const LksLegacyGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return lks_tree_node_path(group->ordered_nodes[index]);
}

/* Historical v1 owned-base machinery for regression comparisons only. The
 * public V2 merge below builds a fresh result and never mutates either source. */
#ifdef LKS_ENABLE_V1_REGRESSION_HELPERS
static void group_destroy_planned_paths(LksPath **planned_paths, size_t count)
{
    size_t index;
    if (planned_paths == NULL) return;
    for (index = 0; index < count; ++index) lks_path_destroy(planned_paths[index]);
    lks_free(planned_paths);
}

/* Shared 6.1 monotonic Base/Incoming path planner. Incoming Paths are never read. */
static LksStatus group_plan_incoming_paths(const LksLegacyGroup *base,
    const LksLegacyGroup *incoming, const LksComparator *comparator,
    LksPath ***out_planned_paths)
{
    LksPath **planned_paths = NULL;
    size_t base_count, incoming_count, base_index, index;
    const LksPath *left_path = NULL;
    LksStatus status = LKS_STATUS_OK;
    if (out_planned_paths == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_planned_paths = NULL;
    base_count = lks_legacy_group_size(base);
    incoming_count = lks_legacy_group_size(incoming);
    if (base_count > (size_t)-1 - incoming_count) return LKS_STATUS_OUT_OF_MEMORY;
    if (incoming_count == 0) return LKS_STATUS_OK;
    if (incoming_count > (size_t)-1 / sizeof(*planned_paths))
        return LKS_STATUS_OUT_OF_MEMORY;
    planned_paths = (LksPath **)lks_alloc_tagged(
        incoming_count * sizeof(*planned_paths), LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (planned_paths == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    for (index = 0; index < incoming_count; ++index) planned_paths[index] = NULL;

    base_index = 0;
    for (index = 0; index < incoming_count; ++index) {
        void *incoming_item = lks_legacy_group_item_at(incoming, index);
        const LksPath *right_path;
        LksPath *new_path = NULL;
        while (base_index < base_count) {
            void *base_item = lks_legacy_group_item_at(base, base_index);
            int order = comparator->compare(base_item, incoming_item, comparator->context);
            if (order > 0) break;
            left_path = lks_legacy_group_path_at(base, base_index);
            if (left_path == NULL) { status = LKS_STATUS_INTERNAL_ERROR; goto fail; }
            ++base_index;
        }
        right_path = base_index < base_count ? lks_legacy_group_path_at(base, base_index) : NULL;
        if (left_path == NULL && right_path != NULL) status = lks_path_before(right_path, &new_path);
        else if (left_path != NULL && right_path != NULL)
            status = lks_path_between(left_path, right_path, &new_path);
        else if (left_path != NULL) status = lks_path_after(left_path, &new_path);
        else {
            new_path = lks_path_create_zero();
            status = new_path == NULL ? LKS_STATUS_OUT_OF_MEMORY : LKS_STATUS_OK;
        }
        if (status != LKS_STATUS_OK) goto fail;
        if (new_path == NULL) { status = LKS_STATUS_INTERNAL_ERROR; goto fail; }
        planned_paths[index] = new_path;
        left_path = new_path;
    }
    *out_planned_paths = planned_paths;
    return LKS_STATUS_OK;
fail:
    group_destroy_planned_paths(planned_paths, incoming_count);
    return status;
}

#endif

LksStatus lks_legacy_group_merge(
    const LksLegacyGroup *base,
    const LksLegacyGroup *incoming,
    const LksComparator *comparator,
    LksLegacyGroup **out_group
)
{
    void **merged;
    size_t base_count, incoming_count, total_count;
    LksStatus status;
    if (out_group == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_group = NULL;
    if (base == NULL || incoming == NULL || comparator == NULL ||
        comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    base_count = lks_legacy_group_size(base);
    incoming_count = lks_legacy_group_size(incoming);
    if (base_count > (size_t)-1 - incoming_count ||
        base_count + incoming_count > (size_t)-1 / sizeof(*merged))
        return LKS_STATUS_OUT_OF_MEMORY;
    total_count = base_count + incoming_count;
    merged = NULL;
    if (total_count != 0) {
        merged = (void **)lks_alloc_tagged(total_count * sizeof(*merged),
            LKS_ALLOC_TAG_MERGE_SCRATCH);
        if (merged == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        /* The result has its own coordinate space. No source Path is consulted. */
        {
            size_t left = 0, right = 0, output = 0;
            while (left < base_count && right < incoming_count) {
                void *base_item = lks_legacy_group_item_at(base, left);
                void *incoming_item = lks_legacy_group_item_at(incoming, right);
                if (comparator->compare(base_item, incoming_item,
                        comparator->context) <= 0) {
                    merged[output++] = base_item;
                    ++left;
                } else {
                    merged[output++] = incoming_item;
                    ++right;
                }
            }
            while (left < base_count) merged[output++] = lks_legacy_group_item_at(base, left++);
            while (right < incoming_count)
                merged[output++] = lks_legacy_group_item_at(incoming, right++);
        }
    }
    status = group_from_sorted(merged, total_count, comparator, out_group);
    lks_free(merged);
    return status;
}

#ifdef LKS_ENABLE_V1_REGRESSION_HELPERS
LksStatus lks_legacy_group_merge_into_owned_base(LksLegacyGroup **inout_base,
    const LksLegacyGroup *incoming, const LksComparator *comparator)
{
    LksLegacyGroup *base;
    LksPath **planned_paths = NULL;
    const LksTreeNode **new_ordered_nodes = NULL;
    const LksTreeNode **old_ordered_nodes;
    size_t base_count, incoming_count, total_count, index;
    LksStatus status;

    if (inout_base == NULL || *inout_base == NULL || incoming == NULL ||
        comparator == NULL || comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;

    base = *inout_base;
    base_count = base->count;
    incoming_count = incoming->count;
    if (base_count > (size_t)-1 - incoming_count) {
        status = LKS_STATUS_OUT_OF_MEMORY;
        goto consume_base;
    }
    total_count = base_count + incoming_count;
    status = group_plan_incoming_paths(base, incoming, comparator, &planned_paths);
    if (status != LKS_STATUS_OK) goto consume_base;

    /* Empty incoming is a true in-place no-op: no copy and no Tree mutation. */
    if (incoming_count == 0) return LKS_STATUS_OK;

    for (index = 0; index < incoming_count; ++index) {
        status = lks_tree_insert(base->tree, planned_paths[index],
            lks_legacy_group_item_at(incoming, index), NULL);
        if (status == LKS_STATUS_NOT_FOUND || status == LKS_STATUS_ALREADY_EXISTS)
            status = LKS_STATUS_INTERNAL_ERROR;
        if (status != LKS_STATUS_OK) goto consume_base;
    }

    /* Keep the old ordered view alive until a complete replacement validates. */
    base->count = total_count;
    status = group_build_ordered_nodes(base, comparator, &new_ordered_nodes);
    if (status != LKS_STATUS_OK) goto consume_base;
    old_ordered_nodes = base->ordered_nodes;
    base->ordered_nodes = new_ordered_nodes;
    new_ordered_nodes = NULL;
    lks_free(old_ordered_nodes);
    group_destroy_planned_paths(planned_paths, incoming_count);
    return LKS_STATUS_OK;

consume_base:
    lks_free(new_ordered_nodes);
    group_destroy_planned_paths(planned_paths, incoming_count);
    lks_legacy_group_destroy(base);
    *inout_base = NULL;
    return status;
}
#endif

LksStatus lks_legacy_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const LksComparator *comparator,
    LksLegacyGroupBatch **out_batch
)
{
    LksLegacyGroupBatch *batch;
    size_t index;
    size_t offset;

    if (out_batch == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_batch = NULL;
    if (group_size == 0 || comparator == NULL || comparator->compare == NULL ||
        (count > 0 && items == NULL)) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    batch = (LksLegacyGroupBatch *)lks_alloc_tagged(sizeof(*batch), LKS_ALLOC_TAG_BATCH_OBJECT);
    if (batch == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    batch->groups = NULL;
    batch->total_count = count;
    batch->group_size = group_size;
    batch->group_count = count / group_size;
    if (count % group_size != 0) {
        ++batch->group_count;
    }

    if (batch->group_count > 0) {
        if (batch->group_count > ((size_t)-1) / sizeof(*batch->groups)) {
            lks_free(batch);
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        batch->groups = (LksLegacyGroup **)lks_alloc_tagged(
            batch->group_count * sizeof(*batch->groups), LKS_ALLOC_TAG_BATCH_GROUP_ARRAY);
        if (batch->groups == NULL) {
            lks_free(batch);
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        for (index = 0; index < batch->group_count; ++index) {
            batch->groups[index] = NULL;
        }
    }

    offset = 0;
    for (index = 0; index < batch->group_count; ++index) {
        size_t current_count;
        size_t remaining;
        LksStatus status;

        remaining = count - offset;
        current_count = remaining < group_size ? remaining : group_size;
        status = lks_legacy_group_build(items + offset, current_count, comparator,
            &batch->groups[index]);
        if (status != LKS_STATUS_OK) {
            size_t cleanup_index;

            for (cleanup_index = 0; cleanup_index < batch->group_count;
                    ++cleanup_index) {
                lks_legacy_group_destroy(batch->groups[cleanup_index]);
            }
            lks_free(batch->groups);
            lks_free(batch);
            return status;
        }
        offset += current_count;
    }

    *out_batch = batch;
    return LKS_STATUS_OK;
}

void lks_legacy_group_batch_destroy(LksLegacyGroupBatch *batch)
{
    size_t index;

    if (batch == NULL) {
        return;
    }
    for (index = 0; index < batch->group_count; ++index) {
        lks_legacy_group_destroy(batch->groups[index]);
    }
    lks_free(batch->groups);
    lks_free(batch);
}

size_t lks_legacy_group_batch_total_size(const LksLegacyGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->total_count;
}

size_t lks_legacy_group_batch_group_count(const LksLegacyGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_count;
}

size_t lks_legacy_group_batch_group_size(const LksLegacyGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_size;
}

const LksLegacyGroup *lks_legacy_group_batch_group_at(
    const LksLegacyGroupBatch *batch,
    size_t index
)
{
    if (batch == NULL || index >= batch->group_count) {
        return NULL;
    }
    return batch->groups[index];
}

LksStatus lks_legacy_group_batch_merge_all(
    const LksLegacyGroupBatch *batch,
    const LksComparator *comparator,
    LksLegacyGroup **out_group
)
{
    void **source = NULL;
    void **scratch = NULL;
    size_t *run_sizes = NULL;
    size_t total, run_count, index, offset;
    LksStatus status;
    if (out_group == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_group = NULL;
    if (batch == NULL || comparator == NULL || comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    total = batch->total_count;
    run_count = batch->group_count;
    if (total == 0) return group_from_sorted(NULL, 0, comparator, out_group);
    if (total > (size_t)-1 / sizeof(*source) ||
        run_count > (size_t)-1 / sizeof(*run_sizes))
        return LKS_STATUS_OUT_OF_MEMORY;
    source = (void **)lks_alloc_tagged(total * sizeof(*source), LKS_ALLOC_TAG_MERGE_SCRATCH);
    scratch = (void **)lks_alloc_tagged(total * sizeof(*scratch), LKS_ALLOC_TAG_MERGE_SCRATCH);
    run_sizes = (size_t *)lks_alloc_tagged(run_count * sizeof(*run_sizes), LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (source == NULL || scratch == NULL || run_sizes == NULL) {
        status = LKS_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }
    offset = 0;
    for (index = 0; index < run_count; ++index) {
        const LksLegacyGroup *group = batch->groups[index];
        size_t item_index;
        run_sizes[index] = group->count;
        for (item_index = 0; item_index < group->count; ++item_index)
            source[offset++] = lks_legacy_group_item_at(group, item_index);
    }
    if (offset != total) { status = LKS_STATUS_INTERNAL_ERROR; goto cleanup; }
    while (run_count > 1) {
        size_t next_count = 0;
        offset = 0;
        for (index = 0; index < run_count; index += 2) {
            size_t left = run_sizes[index];
            size_t right = index + 1 < run_count ? run_sizes[index + 1] : 0;
            /* Earlier chunks are on the left, so equality keeps chunk order. */
            lks_merge_sorted_pointers(source + offset, left,
                source + offset + left, right, comparator, scratch + offset);
            run_sizes[next_count++] = left + right;
            offset += left + right;
        }
        { void **temporary = source; source = scratch; scratch = temporary; }
        run_count = next_count;
    }
    /* Build one final Tree; intermediate merge passes contain pointers only. */
    status = group_from_sorted(source, total, comparator, out_group);
cleanup:
    lks_free(run_sizes);
    lks_free(scratch);
    lks_free(source);
    return status;
}
