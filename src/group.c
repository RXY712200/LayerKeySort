#include <stdlib.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_group_internal.h"

struct LksGroupBatch {
    LksGroup **groups;
    size_t group_count;
    size_t total_count;
    size_t group_size;
};

struct LksGroup {
    LksTree *tree;
    const LksTreeNode **ordered_nodes;
    size_t count;
};

const LksTree *lks_group_internal_tree(const LksGroup *group)
{
    return group == NULL ? NULL : group->tree;
}

static LksStatus flatten_subtree(
    const LksTreeNode *node,
    const LksTreeNode **ordered_nodes,
    size_t capacity,
    size_t *write_index
)
{
    size_t index;

    if (node == NULL || *write_index >= capacity) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    ordered_nodes[*write_index] = node;
    ++*write_index;

    for (index = 0; index < lks_tree_node_child_count(node); ++index) {
        LksStatus status;

        status = flatten_subtree(lks_tree_node_child_at(node, index),
            ordered_nodes, capacity, write_index);
        if (status != LKS_STATUS_OK) {
            return status;
        }
    }
    return LKS_STATUS_OK;
}

static LksStatus validate_ordered_nodes(
    const LksGroup *group,
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

static LksStatus group_build_ordered_nodes(const LksGroup *group,
    const LksComparator *comparator, const LksTreeNode ***out_ordered_nodes)
{
    const LksTreeNode **ordered_nodes;
    size_t index;
    size_t write_index;
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

    write_index = 0;
    for (index = 0; index < lks_tree_root_child_count(group->tree); ++index) {
        status = flatten_subtree(lks_tree_root_child_at(group->tree, index),
            ordered_nodes, group->count, &write_index);
        if (status != LKS_STATUS_OK) {
            lks_free(ordered_nodes);
            return status;
        }
    }
    if (write_index != group->count) {
        lks_free(ordered_nodes);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    status = validate_ordered_nodes(group, ordered_nodes, group->count, comparator);
    if (status != LKS_STATUS_OK) {
        lks_free(ordered_nodes);
        return status;
    }
    *out_ordered_nodes = ordered_nodes;
    return LKS_STATUS_OK;
}

static LksStatus group_build_ordered_view(LksGroup *group,
    const LksComparator *comparator)
{
    const LksTreeNode **ordered_nodes;
    LksStatus status = group_build_ordered_nodes(group, comparator, &ordered_nodes);
    if (status != LKS_STATUS_OK) return status;
    group->ordered_nodes = ordered_nodes;
    return LKS_STATUS_OK;
}

LksStatus lks_group_build(
    void *const *items,
    size_t count,
    const LksComparator *comparator,
    LksGroup **out_group
)
{
    LksGroup *group;
    size_t index;
    LksStatus status;

    if (out_group == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (comparator == NULL || comparator->compare == NULL ||
        (count > 0 && items == NULL)) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    group = (LksGroup *)lks_alloc_tagged(sizeof(*group), LKS_ALLOC_TAG_GROUP_OBJECT);
    if (group == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    group->tree = lks_tree_create();
    group->ordered_nodes = NULL;
    group->count = count;
    if (group->tree == NULL) {
        lks_free(group);
        return LKS_STATUS_OUT_OF_MEMORY;
    }

    for (index = 0; index < count; ++index) {
        status = lks_tree_insert_item(group->tree, items[index], comparator, NULL);
        if (status != LKS_STATUS_OK) {
            lks_tree_destroy(group->tree);
            lks_free(group);
            return status;
        }
    }

    status = group_build_ordered_view(group, comparator);
    if (status != LKS_STATUS_OK) {
        lks_free(group->ordered_nodes);
        lks_tree_destroy(group->tree);
        lks_free(group);
        return status;
    }

    *out_group = group;
    return LKS_STATUS_OK;
}

void lks_group_destroy(LksGroup *group)
{
    if (group == NULL) {
        return;
    }
    lks_free(group->ordered_nodes);
    lks_tree_destroy(group->tree);
    lks_free(group);
}

size_t lks_group_size(const LksGroup *group)
{
    return group == NULL ? 0 : group->count;
}

void *lks_group_item_at(const LksGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return lks_tree_node_item(group->ordered_nodes[index]);
}

const LksPath *lks_group_path_at(const LksGroup *group, size_t index)
{
    if (group == NULL || index >= group->count) {
        return NULL;
    }
    return lks_tree_node_path(group->ordered_nodes[index]);
}

static void group_destroy_planned_paths(LksPath **planned_paths, size_t count)
{
    size_t index;
    if (planned_paths == NULL) return;
    for (index = 0; index < count; ++index) lks_path_destroy(planned_paths[index]);
    lks_free(planned_paths);
}

/* Shared 6.1 monotonic Base/Incoming path planner. Incoming Paths are never read. */
static LksStatus group_plan_incoming_paths(const LksGroup *base,
    const LksGroup *incoming, const LksComparator *comparator,
    LksPath ***out_planned_paths)
{
    LksPath **planned_paths = NULL;
    size_t base_count, incoming_count, base_index, index;
    const LksPath *left_path = NULL;
    LksStatus status = LKS_STATUS_OK;
    if (out_planned_paths == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_planned_paths = NULL;
    base_count = lks_group_size(base);
    incoming_count = lks_group_size(incoming);
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
        void *incoming_item = lks_group_item_at(incoming, index);
        const LksPath *right_path;
        LksPath *new_path = NULL;
        while (base_index < base_count) {
            void *base_item = lks_group_item_at(base, base_index);
            int order = comparator->compare(base_item, incoming_item, comparator->context);
            if (order > 0) break;
            left_path = lks_group_path_at(base, base_index);
            if (left_path == NULL) { status = LKS_STATUS_INTERNAL_ERROR; goto fail; }
            ++base_index;
        }
        right_path = base_index < base_count ? lks_group_path_at(base, base_index) : NULL;
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

LksStatus lks_group_merge(
    const LksGroup *base,
    const LksGroup *incoming,
    const LksComparator *comparator,
    LksGroup **out_group
)
{
    LksPath **planned_paths = NULL;
    LksTree *tree = NULL;
    LksGroup *result = NULL;
    size_t base_count;
    size_t incoming_count;
    size_t total_count;
    size_t index;
    LksStatus status = LKS_STATUS_OK;

    if (out_group == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (base == NULL || incoming == NULL || comparator == NULL ||
        comparator->compare == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    base_count = lks_group_size(base);
    incoming_count = lks_group_size(incoming);
    if (base_count > (size_t)-1 - incoming_count) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    total_count = base_count + incoming_count;

    status = group_plan_incoming_paths(base, incoming, comparator, &planned_paths);
    if (status != LKS_STATUS_OK) return status;

    tree = lks_tree_create();
    if (tree == NULL) {
        status = LKS_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }
    for (index = 0; index < base_count; ++index) {
        status = lks_tree_insert(tree, lks_group_path_at(base, index),
            lks_group_item_at(base, index), NULL);
        if (status == LKS_STATUS_NOT_FOUND ||
            status == LKS_STATUS_ALREADY_EXISTS) {
            status = LKS_STATUS_INTERNAL_ERROR;
        }
        if (status != LKS_STATUS_OK) {
            goto cleanup;
        }
    }
    for (index = 0; index < incoming_count; ++index) {
        status = lks_tree_insert(tree, planned_paths[index],
            lks_group_item_at(incoming, index), NULL);
        if (status == LKS_STATUS_NOT_FOUND ||
            status == LKS_STATUS_ALREADY_EXISTS) {
            status = LKS_STATUS_INTERNAL_ERROR;
        }
        if (status != LKS_STATUS_OK) {
            goto cleanup;
        }
    }

    result = (LksGroup *)lks_alloc_tagged(sizeof(*result), LKS_ALLOC_TAG_GROUP_OBJECT);
    if (result == NULL) {
        status = LKS_STATUS_OUT_OF_MEMORY;
        goto cleanup;
    }
    result->tree = tree;
    result->ordered_nodes = NULL;
    result->count = total_count;
    tree = NULL;
    status = group_build_ordered_view(result, comparator);
    if (status != LKS_STATUS_OK) {
        goto cleanup;
    }

    *out_group = result;
    result = NULL;

cleanup:
    group_destroy_planned_paths(planned_paths, incoming_count);
    lks_tree_destroy(tree);
    lks_group_destroy(result);
    return status;
}

LksStatus lks_group_merge_into_owned_base(LksGroup **inout_base,
    const LksGroup *incoming, const LksComparator *comparator)
{
    LksGroup *base;
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
            lks_group_item_at(incoming, index), NULL);
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
    lks_group_destroy(base);
    *inout_base = NULL;
    return status;
}

LksStatus lks_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const LksComparator *comparator,
    LksGroupBatch **out_batch
)
{
    LksGroupBatch *batch;
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

    batch = (LksGroupBatch *)lks_alloc_tagged(sizeof(*batch), LKS_ALLOC_TAG_BATCH_OBJECT);
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
        batch->groups = (LksGroup **)lks_alloc_tagged(
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
        status = lks_group_build(items + offset, current_count, comparator,
            &batch->groups[index]);
        if (status != LKS_STATUS_OK) {
            size_t cleanup_index;

            for (cleanup_index = 0; cleanup_index < batch->group_count;
                    ++cleanup_index) {
                lks_group_destroy(batch->groups[cleanup_index]);
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

void lks_group_batch_destroy(LksGroupBatch *batch)
{
    size_t index;

    if (batch == NULL) {
        return;
    }
    for (index = 0; index < batch->group_count; ++index) {
        lks_group_destroy(batch->groups[index]);
    }
    lks_free(batch->groups);
    lks_free(batch);
}

size_t lks_group_batch_total_size(const LksGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->total_count;
}

size_t lks_group_batch_group_count(const LksGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_count;
}

size_t lks_group_batch_group_size(const LksGroupBatch *batch)
{
    return batch == NULL ? 0 : batch->group_size;
}

const LksGroup *lks_group_batch_group_at(
    const LksGroupBatch *batch,
    size_t index
)
{
    if (batch == NULL || index >= batch->group_count) {
        return NULL;
    }
    return batch->groups[index];
}

typedef struct LksMergeEntry {
    const LksGroup *group;
    int owned;
} LksMergeEntry;

static LksStatus group_clone_preserving_paths(
    const LksGroup *source,
    const LksComparator *comparator,
    LksGroup **out_group
)
{
    LksTree *tree = NULL;
    LksGroup *clone = NULL;
    size_t index;
    LksStatus status;

    if (out_group == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (source == NULL || comparator == NULL || comparator->compare == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    tree = lks_tree_create();
    if (tree == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    for (index = 0; index < source->count; ++index) {
        status = lks_tree_insert(tree,
            lks_tree_node_path(source->ordered_nodes[index]),
            lks_tree_node_item(source->ordered_nodes[index]), NULL);
        if (status == LKS_STATUS_NOT_FOUND ||
            status == LKS_STATUS_ALREADY_EXISTS) {
            status = LKS_STATUS_INTERNAL_ERROR;
        }
        if (status != LKS_STATUS_OK) {
            lks_tree_destroy(tree);
            return status;
        }
    }

    clone = (LksGroup *)lks_alloc_tagged(sizeof(*clone), LKS_ALLOC_TAG_GROUP_OBJECT);
    if (clone == NULL) {
        lks_tree_destroy(tree);
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    clone->tree = tree;
    clone->ordered_nodes = NULL;
    clone->count = source->count;
    status = group_build_ordered_view(clone, comparator);
    if (status != LKS_STATUS_OK) {
        lks_group_destroy(clone);
        return status;
    }
    *out_group = clone;
    return LKS_STATUS_OK;
}

LksStatus lks_group_batch_merge_all(
    const LksGroupBatch *batch,
    const LksComparator *comparator,
    LksGroup **out_group
)
{
    LksMergeEntry *current = NULL;
    LksMergeEntry *next = NULL;
    size_t current_count;
    size_t next_count = 0;
    size_t index;
    LksStatus status;

    if (out_group == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_group = NULL;
    if (batch == NULL || comparator == NULL || comparator->compare == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    current_count = batch->group_count;
    if (current_count == 0) {
        return lks_group_build(NULL, 0, comparator, out_group);
    }
    if (current_count == 1) {
        return group_clone_preserving_paths(batch->groups[0], comparator,
            out_group);
    }
    if (current_count > ((size_t)-1) / sizeof(*current)) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    current = (LksMergeEntry *)lks_alloc_tagged(
        current_count * sizeof(*current), LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (current == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
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
            status = LKS_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }
        next = (LksMergeEntry *)lks_alloc_tagged(
            next_count * sizeof(*next), LKS_ALLOC_TAG_MERGE_SCRATCH);
        if (next == NULL) {
            status = LKS_STATUS_OUT_OF_MEMORY;
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
                LksMergeEntry *left = &current[current_index];
                LksMergeEntry *right = &current[current_index + 1];
                LksGroup *owned_base = NULL;

                if (left->owned) {
                    owned_base = (LksGroup *)left->group;
                    left->group = NULL;
                    left->owned = 0;
                    status = lks_group_merge_into_owned_base(&owned_base,
                        right->group, comparator);
                } else {
                    status = lks_group_merge(left->group, right->group,
                        comparator, &owned_base);
                }
                if (status != LKS_STATUS_OK) {
                    goto cleanup;
                }
                next[next_index].group = owned_base;
                next[next_index].owned = 1;
                ++next_index;

                if (right->owned) {
                    lks_group_destroy((LksGroup *)right->group);
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

        lks_free(current);
        current = next;
        current_count = next_count;
        next = NULL;
    }

    if (current[0].owned) {
        *out_group = (LksGroup *)current[0].group;
        current[0].group = NULL;
        current[0].owned = 0;
        status = LKS_STATUS_OK;
    } else {
        /* A multi-group reduction must have produced an owned lineage. */
        status = LKS_STATUS_INTERNAL_ERROR;
    }

cleanup:
    if (next != NULL) {
        for (index = 0; index < next_count; ++index) {
            if (next[index].owned) {
                lks_group_destroy((LksGroup *)next[index].group);
            }
        }
        lks_free(next);
    }
    if (current != NULL) {
        for (index = 0; index < current_count; ++index) {
            if (current[index].owned) {
                lks_group_destroy((LksGroup *)current[index].group);
            }
        }
        lks_free(current);
    }
    return status;
}
