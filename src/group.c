#include <stdlib.h>
#include "hps.h"
#include "hps_alloc_internal.h"
#include "hps_group_internal.h"

struct HpsGroupBatch {
    HpsGroup **groups;
    size_t group_count;
    size_t total_count;
    size_t group_size;
};

struct HpsGroup {
    HpsTree *tree;
    const HpsTreeNode **ordered_nodes;
    size_t count;
};

const HpsTree *hps_group_internal_tree(const HpsGroup *group)
{
    return group == NULL ? NULL : group->tree;
}

static HpsStatus flatten_subtree(
    const HpsTreeNode *node,
    const HpsTreeNode **ordered_nodes,
    size_t capacity,
    size_t *write_index
)
{
    size_t index;

    if (node == NULL || *write_index >= capacity) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    ordered_nodes[*write_index] = node;
    ++*write_index;

    for (index = 0; index < hps_tree_node_child_count(node); ++index) {
        HpsStatus status;

        status = flatten_subtree(hps_tree_node_child_at(node, index),
            ordered_nodes, capacity, write_index);
        if (status != HPS_STATUS_OK) {
            return status;
        }
    }
    return HPS_STATUS_OK;
}

static HpsStatus validate_ordered_nodes(
    const HpsGroup *group,
    const HpsTreeNode **ordered_nodes,
    size_t count,
    const HpsComparator *comparator
)
{
    size_t index;

    for (index = 1; index < count; ++index) {
        const HpsPath *previous_path;
        const HpsPath *current_path;
        int path_order;
        int item_order;
        HpsStatus status;

        previous_path = hps_tree_node_path(ordered_nodes[index - 1]);
        current_path = hps_tree_node_path(ordered_nodes[index]);
        if (previous_path == NULL || current_path == NULL) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        status = hps_path_compare(previous_path, current_path, &path_order);
        if (status != HPS_STATUS_OK || path_order != -1) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        item_order = comparator->compare(
            hps_tree_node_item(ordered_nodes[index - 1]),
            hps_tree_node_item(ordered_nodes[index]),
            comparator->context);
        if (item_order > 0) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
    }
    return HPS_STATUS_OK;
}

static HpsStatus group_build_ordered_nodes(const HpsGroup *group,
    const HpsComparator *comparator, const HpsTreeNode ***out_ordered_nodes)
{
    const HpsTreeNode **ordered_nodes;
    size_t index;
    size_t write_index;
    HpsStatus status;

    if (out_ordered_nodes == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    *out_ordered_nodes = NULL;
    if (group->count == 0) return HPS_STATUS_OK;
    if (group->count > ((size_t)-1) / sizeof(*group->ordered_nodes)) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    ordered_nodes = (const HpsTreeNode **)hps_alloc_tagged(
        group->count * sizeof(*ordered_nodes), HPS_ALLOC_TAG_GROUP_ORDERED);
    if (ordered_nodes == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }

    write_index = 0;
    for (index = 0; index < hps_tree_root_child_count(group->tree); ++index) {
        status = flatten_subtree(hps_tree_root_child_at(group->tree, index),
            ordered_nodes, group->count, &write_index);
        if (status != HPS_STATUS_OK) {
            hps_free(ordered_nodes);
            return status;
        }
    }
    if (write_index != group->count) {
        hps_free(ordered_nodes);
        return HPS_STATUS_INTERNAL_ERROR;
    }
    status = validate_ordered_nodes(group, ordered_nodes, group->count, comparator);
    if (status != HPS_STATUS_OK) {
        hps_free(ordered_nodes);
        return status;
    }
    *out_ordered_nodes = ordered_nodes;
    return HPS_STATUS_OK;
}

static HpsStatus group_build_ordered_view(HpsGroup *group,
    const HpsComparator *comparator)
{
    const HpsTreeNode **ordered_nodes;
    HpsStatus status = group_build_ordered_nodes(group, comparator, &ordered_nodes);
    if (status != HPS_STATUS_OK) return status;
    group->ordered_nodes = ordered_nodes;
    return HPS_STATUS_OK;
}

HpsStatus hps_group_build(
    void *const *items,
    size_t count,
    const HpsComparator *comparator,
    HpsGroup **out_group
)
{
    HpsGroup *group;
    size_t index;
    HpsStatus status;

    if (out_group == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (comparator == NULL || comparator->compare == NULL ||
        (count > 0 && items == NULL)) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    group = (HpsGroup *)hps_alloc_tagged(sizeof(*group), HPS_ALLOC_TAG_GROUP_OBJECT);
    if (group == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    group->tree = hps_tree_create();
    group->ordered_nodes = NULL;
    group->count = count;
    if (group->tree == NULL) {
        hps_free(group);
        return HPS_STATUS_OUT_OF_MEMORY;
    }

    for (index = 0; index < count; ++index) {
        status = hps_tree_insert_item(group->tree, items[index], comparator, NULL);
        if (status != HPS_STATUS_OK) {
            hps_tree_destroy(group->tree);
            hps_free(group);
            return status;
        }
    }

    status = group_build_ordered_view(group, comparator);
    if (status != HPS_STATUS_OK) {
        hps_free(group->ordered_nodes);
        hps_tree_destroy(group->tree);
        hps_free(group);
        return status;
    }

    *out_group = group;
    return HPS_STATUS_OK;
}

void hps_group_destroy(HpsGroup *group)
{
    if (group == NULL) {
        return;
    }
    hps_free(group->ordered_nodes);
    hps_tree_destroy(group->tree);
    hps_free(group);
}

size_t hps_group_size(const HpsGroup *group)
{
    return group == NULL ? 0 : group->count;
}

void *hps_group_item_at(const HpsGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return hps_tree_node_item(group->ordered_nodes[index]);
}

const HpsPath *hps_group_path_at(const HpsGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return hps_tree_node_path(group->ordered_nodes[index]);
}

static void group_destroy_planned_paths(HpsPath **planned_paths, size_t count)
{
    size_t index;
    if (planned_paths == NULL) return;
    for (index = 0; index < count; ++index) hps_path_destroy(planned_paths[index]);
    hps_free(planned_paths);
}

/* Shared 6.1 monotonic Base/Incoming path planner. Incoming Paths are never read. */
static HpsStatus group_plan_incoming_paths(const HpsGroup *base,
    const HpsGroup *incoming, const HpsComparator *comparator,
    HpsPath ***out_planned_paths)
{
    HpsPath **planned_paths = NULL;
    size_t base_count, incoming_count, base_index, index;
    const HpsPath *left_path = NULL;
    HpsStatus status = HPS_STATUS_OK;
    if (out_planned_paths == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    *out_planned_paths = NULL;
    base_count = hps_group_size(base);
    incoming_count = hps_group_size(incoming);
    if (base_count > (size_t)-1 - incoming_count) return HPS_STATUS_OUT_OF_MEMORY;
    if (incoming_count == 0) return HPS_STATUS_OK;
    if (incoming_count > (size_t)-1 / sizeof(*planned_paths))
        return HPS_STATUS_OUT_OF_MEMORY;
    planned_paths = (HpsPath **)hps_alloc_tagged(
        incoming_count * sizeof(*planned_paths), HPS_ALLOC_TAG_MERGE_SCRATCH);
    if (planned_paths == NULL) return HPS_STATUS_OUT_OF_MEMORY;
    for (index = 0; index < incoming_count; ++index) planned_paths[index] = NULL;

    base_index = 0;
    for (index = 0; index < incoming_count; ++index) {
        void *incoming_item = hps_group_item_at(incoming, index);
        const HpsPath *right_path;
        HpsPath *new_path = NULL;
        while (base_index < base_count) {
            void *base_item = hps_group_item_at(base, base_index);
            int order = comparator->compare(base_item, incoming_item, comparator->context);
            if (order > 0) break;
            left_path = hps_group_path_at(base, base_index);
            if (left_path == NULL) { status = HPS_STATUS_INTERNAL_ERROR; goto fail; }
            ++base_index;
        }
        right_path = base_index < base_count ? hps_group_path_at(base, base_index) : NULL;
        if (left_path == NULL && right_path != NULL) status = hps_path_before(right_path, &new_path);
        else if (left_path != NULL && right_path != NULL)
            status = hps_path_between(left_path, right_path, &new_path);
        else if (left_path != NULL) status = hps_path_after(left_path, &new_path);
        else {
            new_path = hps_path_create_zero();
            status = new_path == NULL ? HPS_STATUS_OUT_OF_MEMORY : HPS_STATUS_OK;
        }
        if (status != HPS_STATUS_OK) goto fail;
        if (new_path == NULL) { status = HPS_STATUS_INTERNAL_ERROR; goto fail; }
        planned_paths[index] = new_path;
        left_path = new_path;
    }
    *out_planned_paths = planned_paths;
    return HPS_STATUS_OK;
fail:
    group_destroy_planned_paths(planned_paths, incoming_count);
    return status;
}

HpsStatus hps_group_merge(
    const HpsGroup *base,
    const HpsGroup *incoming,
    const HpsComparator *comparator,
    HpsGroup **out_group
)
{
    HpsPath **planned_paths = NULL;
    HpsTree *tree = NULL;
    HpsGroup *result = NULL;
    size_t base_count;
    size_t incoming_count;
    size_t total_count;
    size_t index;
    HpsStatus status = HPS_STATUS_OK;

    if (out_group == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (base == NULL || incoming == NULL || comparator == NULL ||
        comparator->compare == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    base_count = hps_group_size(base);
    incoming_count = hps_group_size(incoming);
    if (base_count > (size_t)-1 - incoming_count) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    total_count = base_count + incoming_count;

    status = group_plan_incoming_paths(base, incoming, comparator, &planned_paths);
    if (status != HPS_STATUS_OK) return status;

    tree = hps_tree_create();
    if (tree == NULL) {
        status = HPS_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }
    for (index = 0; index < base_count; ++index) {
        status = hps_tree_insert(tree, hps_group_path_at(base, index),
            hps_group_item_at(base, index), NULL);
        if (status == HPS_STATUS_NOT_FOUND ||
            status == HPS_STATUS_ALREADY_EXISTS) {
            status = HPS_STATUS_INTERNAL_ERROR;
        }
        if (status != HPS_STATUS_OK) {
            goto cleanup;
        }
    }
    for (index = 0; index < incoming_count; ++index) {
        status = hps_tree_insert(tree, planned_paths[index],
            hps_group_item_at(incoming, index), NULL);
        if (status == HPS_STATUS_NOT_FOUND ||
            status == HPS_STATUS_ALREADY_EXISTS) {
            status = HPS_STATUS_INTERNAL_ERROR;
        }
        if (status != HPS_STATUS_OK) {
            goto cleanup;
        }
    }

    result = (HpsGroup *)hps_alloc_tagged(sizeof(*result), HPS_ALLOC_TAG_GROUP_OBJECT);
    if (result == NULL) {
        status = HPS_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }
    result->tree = tree;
    result->ordered_nodes = NULL;
    result->count = total_count;
    tree = NULL;
    status = group_build_ordered_view(result, comparator);
    if (status != HPS_STATUS_OK) {
        goto cleanup;
    }

    *out_group = result;
    result = NULL;

cleanup:
    group_destroy_planned_paths(planned_paths, incoming_count);
    hps_tree_destroy(tree);
    hps_group_destroy(result);
    return status;
}

HpsStatus hps_group_merge_into_owned_base(HpsGroup **inout_base,
    const HpsGroup *incoming, const HpsComparator *comparator)
{
    HpsGroup *base;
    HpsPath **planned_paths = NULL;
    const HpsTreeNode **new_ordered_nodes = NULL;
    const HpsTreeNode **old_ordered_nodes;
    size_t base_count, incoming_count, total_count, index;
    HpsStatus status;

    if (inout_base == NULL || *inout_base == NULL || incoming == NULL ||
        comparator == NULL || comparator->compare == NULL)
        return HPS_STATUS_INVALID_ARGUMENT;

    base = *inout_base;
    base_count = base->count;
    incoming_count = incoming->count;
    if (base_count > (size_t)-1 - incoming_count) {
        status = HPS_STATUS_OUT_OF_MEMORY;
        goto consume_base;
    }
    total_count = base_count + incoming_count;
    status = group_plan_incoming_paths(base, incoming, comparator, &planned_paths);
    if (status != HPS_STATUS_OK) goto consume_base;

    /* Empty incoming is a true in-place no-op: no copy and no Tree mutation. */
    if (incoming_count == 0) return HPS_STATUS_OK;

    for (index = 0; index < incoming_count; ++index) {
        status = hps_tree_insert(base->tree, planned_paths[index],
            hps_group_item_at(incoming, index), NULL);
        if (status == HPS_STATUS_NOT_FOUND || status == HPS_STATUS_ALREADY_EXISTS)
            status = HPS_STATUS_INTERNAL_ERROR;
        if (status != HPS_STATUS_OK) goto consume_base;
    }

    /* Keep the old ordered view alive until a complete replacement validates. */
    base->count = total_count;
    status = group_build_ordered_nodes(base, comparator, &new_ordered_nodes);
    if (status != HPS_STATUS_OK) goto consume_base;
    old_ordered_nodes = base->ordered_nodes;
    base->ordered_nodes = new_ordered_nodes;
    new_ordered_nodes = NULL;
    hps_free(old_ordered_nodes);
    group_destroy_planned_paths(planned_paths, incoming_count);
    return HPS_STATUS_OK;

consume_base:
    hps_free(new_ordered_nodes);
    group_destroy_planned_paths(planned_paths, incoming_count);
    hps_group_destroy(base);
    *inout_base = NULL;
    return status;
}

HpsStatus hps_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const HpsComparator *comparator,
    HpsGroupBatch **out_batch
)
{
    HpsGroupBatch *batch;
    size_t index;
    size_t offset;

    if (out_batch == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_batch = NULL;
    if (group_size == 0 || comparator == NULL || comparator->compare == NULL ||
        (count > 0 && items == NULL)) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    batch = (HpsGroupBatch *)hps_alloc_tagged(sizeof(*batch), HPS_ALLOC_TAG_BATCH_OBJECT);
    if (batch == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
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
            hps_free(batch);
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        batch->groups = (HpsGroup **)hps_alloc_tagged(
            batch->group_count * sizeof(*batch->groups), HPS_ALLOC_TAG_BATCH_GROUP_ARRAY);
        if (batch->groups == NULL) {
            hps_free(batch);
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        for (index = 0; index < batch->group_count; ++index) {
            batch->groups[index] = NULL;
        }
    }

    offset = 0;
    for (index = 0; index < batch->group_count; ++index) {
        size_t current_count;
        size_t remaining;
        HpsStatus status;

        remaining = count - offset;
        current_count = remaining < group_size ? remaining : group_size;
        status = hps_group_build(items + offset, current_count, comparator,
            &batch->groups[index]);
        if (status != HPS_STATUS_OK) {
            size_t cleanup_index;

            for (cleanup_index = 0; cleanup_index < batch->group_count;
                    ++cleanup_index) {
                hps_group_destroy(batch->groups[cleanup_index]);
            }
            hps_free(batch->groups);
            hps_free(batch);
            return status;
        }
        offset += current_count;
    }

    *out_batch = batch;
    return HPS_STATUS_OK;
}

void hps_group_batch_destroy(HpsGroupBatch *batch)
{
    size_t index;

    if (batch == NULL) {
        return;
    }
    for (index = 0; index < batch->group_count; ++index) {
        hps_group_destroy(batch->groups[index]);
    }
    hps_free(batch->groups);
    hps_free(batch);
}

size_t hps_group_batch_total_size(const HpsGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->total_count;
}

size_t hps_group_batch_group_count(const HpsGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_count;
}

size_t hps_group_batch_group_size(const HpsGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_size;
}

const HpsGroup *hps_group_batch_group_at(
    const HpsGroupBatch *batch,
    size_t index
)
{
    if (batch == NULL || index >= batch->group_count) {
        return NULL;
    }
    return batch->groups[index];
}

typedef struct HpsMergeEntry {
    const HpsGroup *group;
    int owned;
} HpsMergeEntry;

static HpsStatus group_clone_preserving_paths(
    const HpsGroup *source,
    const HpsComparator *comparator,
    HpsGroup **out_group
)
{
    HpsTree *tree = NULL;
    HpsGroup *clone = NULL;
    size_t index;
    HpsStatus status;

    if (out_group == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (source == NULL || comparator == NULL || comparator->compare == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    tree = hps_tree_create();
    if (tree == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    for (index = 0; index < source->count; ++index) {
        status = hps_tree_insert(tree,
            hps_tree_node_path(source->ordered_nodes[index]),
            hps_tree_node_item(source->ordered_nodes[index]), NULL);
        if (status == HPS_STATUS_NOT_FOUND ||
            status == HPS_STATUS_ALREADY_EXISTS) {
            status = HPS_STATUS_INTERNAL_ERROR;
        }
        if (status != HPS_STATUS_OK) {
            hps_tree_destroy(tree);
            return status;
        }
    }

    clone = (HpsGroup *)hps_alloc_tagged(sizeof(*clone), HPS_ALLOC_TAG_GROUP_OBJECT);
    if (clone == NULL) {
        hps_tree_destroy(tree);
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    clone->tree = tree;
    clone->ordered_nodes = NULL;
    clone->count = source->count;
    status = group_build_ordered_view(clone, comparator);
    if (status != HPS_STATUS_OK) {
        hps_group_destroy(clone);
        return status;
    }
    *out_group = clone;
    return HPS_STATUS_OK;
}

HpsStatus hps_group_batch_merge_all(
    const HpsGroupBatch *batch,
    const HpsComparator *comparator,
    HpsGroup **out_group
)
{
    HpsMergeEntry *current = NULL;
    HpsMergeEntry *next = NULL;
    size_t current_count;
    size_t next_count = 0;
    size_t index;
    HpsStatus status;

    if (out_group == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (batch == NULL || comparator == NULL || comparator->compare == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    current_count = batch->group_count;
    if (current_count == 0) {
        return hps_group_build(NULL, 0, comparator, out_group);
    }
    if (current_count == 1) {
        return group_clone_preserving_paths(batch->groups[0], comparator,
            out_group);
    }
    if (current_count > ((size_t)-1) / sizeof(*current)) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    current = (HpsMergeEntry *)hps_alloc_tagged(
        current_count * sizeof(*current), HPS_ALLOC_TAG_MERGE_SCRATCH);
    if (current == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    for (index = 0; index < current_count; ++index) {
        current[index].group = batch->groups[index];
        current[index].owned = 0;
    }

    while (current_count > 1) {
        size_t next_index;
        size_t current_index;

        next_count = current_count / 2;
        if (current_count % 2 != 0) {
            ++next_count;
        }
        if (next_count > ((size_t)-1) / sizeof(*next)) {
            status = HPS_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }
        next = (HpsMergeEntry *)hps_alloc_tagged(
            next_count * sizeof(*next), HPS_ALLOC_TAG_MERGE_SCRATCH);
        if (next == NULL) {
            status = HPS_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }
        for (next_index = 0; next_index < next_count; ++next_index) {
            next[next_index].group = NULL;
            next[next_index].owned = 0;
        }

        next_index = 0;
        current_index = 0;
        while (current_index < current_count) {
            if (current_index < current_count - 1) {
                HpsMergeEntry *left = &current[current_index];
                HpsMergeEntry *right = &current[current_index + 1];
                HpsGroup *owned_base = NULL;

                if (left->owned) {
                    owned_base = (HpsGroup *)left->group;
                    left->group = NULL;
                    left->owned = 0;
                    status = hps_group_merge_into_owned_base(&owned_base,
                        right->group, comparator);
                } else {
                    status = hps_group_merge(left->group, right->group,
                        comparator, &owned_base);
                }
                if (status != HPS_STATUS_OK) {
                    goto cleanup;
                }
                next[next_index].group = owned_base;
                next[next_index].owned = 1;
                ++next_index;

                if (right->owned) {
                    hps_group_destroy((HpsGroup *)right->group);
                    right->group = NULL;
                    right->owned = 0;
                }
                left->group = NULL;
                current_index += 2;
            } else {
                next[next_index] = current[current_index];
                ++next_index;
                current[current_index].group = NULL;
                current[current_index].owned = 0;
                ++current_index;
            }
        }

        hps_free(current);
        current = next;
        current_count = next_count;
        next = NULL;
    }

    if (current[0].owned) {
        *out_group = (HpsGroup *)current[0].group;
        current[0].group = NULL;
        current[0].owned = 0;
        status = HPS_STATUS_OK;
    } else {
        /* A multi-group reduction must have produced an owned lineage. */
        status = HPS_STATUS_INTERNAL_ERROR;
    }

cleanup:
    if (next != NULL) {
        for (index = 0; index < next_count; ++index) {
            if (next[index].owned) {
                hps_group_destroy((HpsGroup *)next[index].group);
            }
        }
        hps_free(next);
    }
    if (current != NULL) {
        for (index = 0; index < current_count; ++index) {
            if (current[index].owned) {
                hps_group_destroy((HpsGroup *)current[index].group);
            }
        }
        hps_free(current);
    }
    return status;
}
