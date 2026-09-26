#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "hps.h"
#include "hps_alloc_internal.h"
#include "hps_tree_internal.h"

typedef struct HpsTreeChildBlock HpsTreeChildBlock;

struct HpsTreeNode {
    HpsPath *path;
    void *item;
    struct HpsTreeNode *parent;
    HpsTreeChildBlock *child_block;
};

struct HpsTreeChildBlock {
    size_t count;
    size_t capacity;
    HpsTreeNode *children[];
};

struct HpsTree {
    HpsTreeNode root;
    size_t size;
};

static size_t tree_node_child_count(const HpsTreeNode *node)
{ return node == NULL || node->child_block == NULL ? 0 : node->child_block->count; }

static size_t tree_node_child_capacity(const HpsTreeNode *node)
{ return node == NULL || node->child_block == NULL ? 0 : node->child_block->capacity; }

static HpsTreeNode *const *tree_node_children(const HpsTreeNode *node)
{ return node == NULL || node->child_block == NULL ? NULL : node->child_block->children; }

static HpsTreeNode **tree_node_children_mutable(HpsTreeNode *node)
{ return node == NULL || node->child_block == NULL ? NULL : node->child_block->children; }

static int tree_child_block_bytes(size_t capacity, size_t *out_bytes)
{
    size_t header_bytes = offsetof(HpsTreeChildBlock, children);
    size_t pointer_bytes;
    if (out_bytes == NULL || capacity > (size_t)-1 / sizeof(HpsTreeNode *)) return 0;
    pointer_bytes = capacity * sizeof(HpsTreeNode *);
    if (header_bytes > (size_t)-1 - pointer_bytes) return 0;
    *out_bytes = header_bytes + pointer_bytes;
    return 1;
}

static HpsStatus child_lower_bound(
    const HpsTreeNode *parent,
    const HpsPath *path,
    size_t *out_index,
    int *out_found
)
{
    size_t low;
    size_t high;

    low = 0;
    high = tree_node_child_count(parent);
    *out_found = 0;

    while (low < high) {
        size_t middle;
        int comparison;
        HpsStatus status;

        middle = low + (high - low) / 2;
        status = hps_path_compare(tree_node_children(parent)[middle]->path, path,
            &comparison);
        if (status != HPS_STATUS_OK) {
            return status;
        }
        if (comparison < 0) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    *out_index = low;
    if (low < tree_node_child_count(parent)) {
        int comparison;
        HpsStatus status;

        status = hps_path_compare(tree_node_children(parent)[low]->path, path,
            &comparison);
        if (status != HPS_STATUS_OK) {
            return status;
        }
        *out_found = comparison == 0;
    }
    return HPS_STATUS_OK;
}

static HpsStatus find_prefix_node(
    const HpsTree *tree,
    const HpsPath *target,
    size_t prefix_depth,
    const HpsTreeNode **out_node
)
{
    HpsPath *prefix;
    const HpsTreeNode *parent;
    HpsDirection direction;
    size_t index;
    HpsStatus status;

    *out_node = NULL;
    if (prefix_depth == 0 || prefix_depth > hps_path_depth(target)) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    direction = hps_path_direction(target);
    if (direction != HPS_DIRECTION_POSITIVE &&
        direction != HPS_DIRECTION_NEGATIVE) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    prefix = NULL;
    parent = &tree->root;
    for (index = 0; index < prefix_depth; ++index) {
        unsigned int slot;
        size_t level;
        size_t child_index;
        int found;
        const HpsTreeNode *child;

        if (hps_path_get_slot(target, index, &slot) != HPS_STATUS_OK ||
            hps_path_get_level(target, index, &level) != HPS_STATUS_OK) {
            hps_path_destroy(prefix);
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (index == 0) {
            prefix = hps_path_create_at_level(direction, slot, level);
            if (prefix == NULL) {
                return HPS_STATUS_OUT_OF_MEMORY;
            }
        } else {
            status = hps_path_append_at_level(prefix, slot, level);
            if (status != HPS_STATUS_OK) {
                hps_path_destroy(prefix);
                return status;
            }
        }

        status = child_lower_bound(parent, prefix, &child_index, &found);
        if (status != HPS_STATUS_OK) {
            hps_path_destroy(prefix);
            return status;
        }
        if (!found) {
            hps_path_destroy(prefix);
            return HPS_STATUS_NOT_FOUND;
        }
        child = tree_node_children(parent)[child_index];
        parent = child;
    }

    hps_path_destroy(prefix);
    *out_node = parent;
    return HPS_STATUS_OK;
}

static HpsStatus reserve_child(HpsTreeNode *parent)
{
    size_t capacity;
    size_t bytes;
    HpsTreeChildBlock *block;
    size_t current_capacity = tree_node_child_capacity(parent);
    size_t current_count = tree_node_child_count(parent);

    if (current_count < current_capacity) {
        return HPS_STATUS_OK;
    }
    if (current_count == (size_t)-1) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }

    capacity = current_capacity == 0 ? 1 : current_capacity;
    while (capacity <= current_count) {
        if (capacity > ((size_t)-1) / 2) {
            capacity = current_count + 1;
            break;
        }
        capacity *= 2;
    }
    if (!tree_child_block_bytes(capacity, &bytes)) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }

    if (parent->child_block == NULL) {
        block = (HpsTreeChildBlock *)hps_alloc_tagged(bytes, HPS_ALLOC_TAG_TREE_CHILDREN);
        if (block != NULL) {
            block->count = 0;
            block->capacity = capacity;
        }
    } else {
        block = (HpsTreeChildBlock *)hps_realloc(parent->child_block, bytes);
        if (block != NULL) block->capacity = capacity;
    }
    if (block == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    parent->child_block = block;
    return HPS_STATUS_OK;
}

HpsTree *hps_tree_create(void)
{
    HpsTree *tree;

    tree = (HpsTree *)hps_alloc_tagged(sizeof(*tree), HPS_ALLOC_TAG_TREE_OBJECT);
    if (tree == NULL) {
        return NULL;
    }
    tree->root.path = NULL;
    tree->root.item = NULL;
    tree->root.parent = NULL;
    tree->root.child_block = NULL;
    tree->size = 0;
    return tree;
}

static void destroy_node(HpsTreeNode *node)
{
    size_t index;

    for (index = 0; index < tree_node_child_count(node); ++index) {
        destroy_node(tree_node_children(node)[index]);
    }
    hps_free(node->child_block);
    hps_path_destroy(node->path);
    hps_free(node);
}

void hps_tree_destroy(HpsTree *tree)
{
    size_t index;

    if (tree == NULL) {
        return;
    }
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        destroy_node(tree_node_children(&tree->root)[index]);
    }
    hps_free(tree->root.child_block);
    hps_free(tree);
}

size_t hps_tree_size(const HpsTree *tree)
{
    return tree == NULL ? 0 : tree->size;
}

HpsStatus hps_tree_insert(
    HpsTree *tree,
    const HpsPath *path,
    void *item,
    const HpsTreeNode **out_node
)
{
    HpsTreeNode *parent;
    HpsPath *path_copy;
    HpsTreeNode *node;
    size_t depth;
    size_t index;
    int found;
    HpsStatus status;

    if (out_node != NULL) {
        *out_node = NULL;
    }
    if (tree == NULL || path == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    depth = hps_path_depth(path);
    if (hps_path_direction(path) == HPS_DIRECTION_ZERO) {
        if (depth != 0) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        parent = &tree->root;
    } else {
        if (depth == 0 ||
            (hps_path_direction(path) != HPS_DIRECTION_POSITIVE &&
             hps_path_direction(path) != HPS_DIRECTION_NEGATIVE)) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (depth == 1) {
            parent = &tree->root;
        } else {
            const HpsTreeNode *found_parent;

            status = find_prefix_node(tree, path, depth - 1, &found_parent);
            if (status != HPS_STATUS_OK) {
                return status;
            }
            parent = (HpsTreeNode *)found_parent;
        }
    }

    status = child_lower_bound(parent, path, &index, &found);
    if (status != HPS_STATUS_OK) {
        return status;
    }
    if (found) {
        return HPS_STATUS_ALREADY_EXISTS;
    }

    path_copy = hps_path_clone(path);
    if (path_copy == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    node = (HpsTreeNode *)hps_alloc_tagged(sizeof(*node), HPS_ALLOC_TAG_TREE_NODE);
    if (node == NULL) {
        hps_path_destroy(path_copy);
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    node->path = path_copy;
    node->item = item;
    node->parent = parent;
    node->child_block = NULL;

    status = reserve_child(parent);
    if (status != HPS_STATUS_OK) {
        hps_path_destroy(node->path);
        hps_free(node);
        return status;
    }
    memmove(&tree_node_children_mutable(parent)[index + 1],
        &tree_node_children_mutable(parent)[index],
        (tree_node_child_count(parent) - index) * sizeof(HpsTreeNode *));
    tree_node_children_mutable(parent)[index] = node;
    ++parent->child_block->count;
    ++tree->size;
    if (out_node != NULL) {
        *out_node = node;
    }
    return HPS_STATUS_OK;
}

static HpsStatus locate_in_children(
    const HpsTreeNode *parent,
    const void *item,
    const HpsComparator *comparator,
    const HpsTreeNode *upper_bound,
    const HpsTreeNode **out_left,
    const HpsTreeNode **out_equal,
    const HpsTreeNode **out_right
)
{
    const HpsTreeNode *current;
    const HpsTreeNode *next_upper_bound;

    current = parent;
    next_upper_bound = upper_bound;
    for (;;) {
        size_t low;
        size_t high;

        if (tree_node_child_count(current) == 0) {
            if (current->path != NULL) {
                *out_left = current;
                *out_right = next_upper_bound;
            }
            return HPS_STATUS_OK;
        }

        low = 0;
        high = tree_node_child_count(current);
        while (low < high) {
            size_t middle;
            const HpsTreeNode *child;
            int comparison;

            middle = low + (high - low) / 2;
            child = tree_node_children(current)[middle];
            comparison = comparator->compare(
                item, child->item, comparator->context);
            if (comparison == 0) {
                *out_equal = child;
                return HPS_STATUS_OK;
            }
            if (comparison > 0) {
                low = middle + 1;
            } else {
                high = middle;
            }
        }

        if (low == 0) {
            if (current->path != NULL) {
                *out_left = current;
            }
            *out_right = tree_node_children(current)[0];
            return HPS_STATUS_OK;
        }

        if (low < tree_node_child_count(current)) {
            next_upper_bound = tree_node_children(current)[low];
        }
        current = tree_node_children(current)[low - 1];
    }
}

HpsStatus hps_tree_locate_item(
    const HpsTree *tree,
    const void *item,
    const HpsComparator *comparator,
    const HpsTreeNode **out_left,
    const HpsTreeNode **out_equal,
    const HpsTreeNode **out_right
)
{
    if (tree == NULL || comparator == NULL || comparator->compare == NULL ||
        out_left == NULL || out_equal == NULL || out_right == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    *out_left = NULL;
    *out_equal = NULL;
    *out_right = NULL;
    if (tree->size == 0) {
        return HPS_STATUS_OK;
    }

    return locate_in_children(&tree->root, item, comparator, NULL,
        out_left, out_equal, out_right);
}
static const HpsTreeNode *tree_node_successor(
    const HpsTree *tree,
    const HpsTreeNode *node
)
{
    const HpsTreeNode *current;

    if (tree == NULL || node == NULL) {
        return NULL;
    }
    if (tree_node_child_count(node) > 0) {
        return tree_node_children(node)[0];
    }

    current = node;
    while (current->parent != NULL && current->parent->path != NULL) {
        const HpsTreeNode *parent;
        size_t index;
        int found;

        parent = current->parent;
        if (child_lower_bound(parent, current->path, &index, &found) !=
                HPS_STATUS_OK || !found) {
            return NULL;
        }
        if (index + 1 < tree_node_child_count(parent)) {
            return tree_node_children(parent)[index + 1];
        }
        current = parent;
    }

    /* The current node was the last child all the way to the virtual root. */
    if (current->parent == &tree->root) {
        size_t index;
        int found;

        if (child_lower_bound(&tree->root, current->path, &index, &found) !=
                HPS_STATUS_OK || !found || index + 1 >= tree_node_child_count(&tree->root)) {
            return NULL;
        }
        return tree_node_children(&tree->root)[index + 1];
    }
    return NULL;
}

HpsStatus hps_tree_insert_item(
    HpsTree *tree,
    void *item,
    const HpsComparator *comparator,
    const HpsTreeNode **out_node
)
{
    const HpsTreeNode *left;
    const HpsTreeNode *equal;
    const HpsTreeNode *right;
    const HpsTreeNode *successor;
    HpsPath *new_path;
    HpsStatus status;

    if (out_node != NULL) {
        *out_node = NULL;
    }
    if (tree == NULL || comparator == NULL || comparator->compare == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    new_path = NULL;
    if (hps_tree_size(tree) == 0) {
        new_path = hps_path_create_zero();
        if (new_path == NULL) {
            return HPS_STATUS_OUT_OF_MEMORY;
        }
    } else {
        left = NULL;
        equal = NULL;
        right = NULL;
        status = hps_tree_locate_item(tree, item, comparator,
            &left, &equal, &right);
        if (status != HPS_STATUS_OK) {
            return status;
        }

        if (equal != NULL) {
            /* Insert after the complete comparator-equal run.  This keeps
             * item insertion stable when several items compare as equal. */
            successor = tree_node_successor(tree, equal);
            while (successor != NULL &&
                comparator->compare(item, hps_tree_node_item(successor),
                    comparator->context) == 0) {
                equal = successor;
                successor = tree_node_successor(tree, equal);
            }
            if (successor != NULL) {
                status = hps_path_between(hps_tree_node_path(equal),
                    hps_tree_node_path(successor), &new_path);
            } else {
                status = hps_path_after(hps_tree_node_path(equal), &new_path);
            }
        } else if (left == NULL && right != NULL) {
            status = hps_path_before(hps_tree_node_path(right), &new_path);
        } else if (left != NULL && right == NULL) {
            status = hps_path_after(hps_tree_node_path(left), &new_path);
        } else if (left != NULL && right != NULL) {
            status = hps_path_between(hps_tree_node_path(left),
                hps_tree_node_path(right), &new_path);
        } else {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (status != HPS_STATUS_OK) {
            hps_path_destroy(new_path);
            return status;
        }
    }

    status = hps_tree_insert(tree, new_path, item, out_node);
    hps_path_destroy(new_path);
    if (status == HPS_STATUS_ALREADY_EXISTS || status == HPS_STATUS_NOT_FOUND) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    return status;
}
HpsStatus hps_tree_find_path(
    const HpsTree *tree,
    const HpsPath *path,
    const HpsTreeNode **out_node
)
{
    size_t depth;

    if (out_node == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_node = NULL;
    if (tree == NULL || path == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    depth = hps_path_depth(path);
    if (hps_path_direction(path) == HPS_DIRECTION_ZERO) {
        size_t index;
        int found;
        HpsStatus status;

        if (depth != 0) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        status = child_lower_bound(&tree->root, path, &index, &found);
        if (status != HPS_STATUS_OK) {
            return status;
        }
        if (!found) {
            return HPS_STATUS_NOT_FOUND;
        }
        *out_node = tree_node_children(&tree->root)[index];
        return HPS_STATUS_OK;
    }
    if (depth == 0) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    return find_prefix_node(tree, path, depth, out_node);
}

const HpsPath *hps_tree_node_path(const HpsTreeNode *node)
{
    return node == NULL ? NULL : node->path;
}

void *hps_tree_node_item(const HpsTreeNode *node)
{
    return node == NULL ? NULL : node->item;
}

const HpsTreeNode *hps_tree_node_parent(const HpsTreeNode *node)
{
    if (node == NULL || node->parent == NULL || node->parent->path == NULL) {
        return NULL;
    }
    return node->parent;
}

size_t hps_tree_node_child_count(const HpsTreeNode *node)
{
    return tree_node_child_count(node);
}

const HpsTreeNode *hps_tree_node_child_at(
    const HpsTreeNode *node,
    size_t index
)
{
    if (node == NULL || index >= tree_node_child_count(node)) {
        return NULL;
    }
    return tree_node_children(node)[index];
}

size_t hps_tree_root_child_count(const HpsTree *tree)
{
    return tree == NULL ? 0 : tree_node_child_count(&tree->root);
}

const HpsTreeNode *hps_tree_root_child_at(const HpsTree *tree, size_t index)
{
    if (tree == NULL || index >= tree_node_child_count(&tree->root)) {
        return NULL;
    }
    return tree_node_children(&tree->root)[index];
}

static int tree_profile_add(size_t *target, size_t value)
{
    if (*target > (size_t)-1 - value) return 0;
    *target += value;
    return 1;
}

static size_t tree_profile_degree_bucket(size_t degree)
{
    if (degree <= 4) return degree;
    if (degree <= 8) return 5;
    if (degree <= 16) return 6;
    if (degree <= 32) return 7;
    if (degree <= 64) return 8;
    if (degree <= 128) return 9;
    return 10;
}

static size_t tree_profile_joint_bucket(size_t degree)
{
    if (degree <= 4) return degree;
    if (degree <= 8) return 5;
    if (degree <= 16) return 6;
    return 7;
}

static HpsStatus tree_profile_node(const HpsTreeNode *node,
    const HpsTreeNode *expected_parent, HpsTreeInternalProfile *profile)
{
    size_t bucket, joint, index, unused;
    HpsTreeInternalDegreeCapacity *group;
    if (node == NULL || node->parent != expected_parent ||
        tree_node_child_count(node) > tree_node_child_capacity(node) ||
        ((tree_node_child_capacity(node) == 0) != (node->child_block == NULL)))
        return HPS_STATUS_INTERNAL_ERROR;
    if (!tree_profile_add(&profile->real_node_count, 1) ||
        !tree_profile_add(&profile->total_child_count, tree_node_child_count(node)) ||
        !tree_profile_add(&profile->total_child_capacity, tree_node_child_capacity(node)))
        return HPS_STATUS_INTERNAL_ERROR;
    if (node->child_block != NULL) {
        if (!tree_profile_add(&profile->allocated_child_array_count, 1) ||
            !tree_profile_add(&profile->real_nodes_with_child_array_count, 1))
            return HPS_STATUS_INTERNAL_ERROR;
    }
    if (tree_node_child_capacity(node) > profile->max_real_child_capacity)
        profile->max_real_child_capacity = tree_node_child_capacity(node);
    bucket = tree_profile_degree_bucket(tree_node_child_count(node));
    if (!tree_profile_add(&profile->degree_bucket_counts[bucket], 1))
        return HPS_STATUS_INTERNAL_ERROR;
    if (tree_node_child_count(node) == 0) {
        if (!tree_profile_add(&profile->leaf_count, 1)) return HPS_STATUS_INTERNAL_ERROR;
    } else if (tree_node_child_count(node) == 1) {
        if (!tree_profile_add(&profile->unary_count, 1)) return HPS_STATUS_INTERNAL_ERROR;
    } else if (!tree_profile_add(&profile->branching_count, 1)) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    joint = tree_profile_joint_bucket(tree_node_child_count(node));
    group = &profile->degree_capacity[joint];
    unused = tree_node_child_capacity(node) - tree_node_child_count(node);
    if (group->node_count == 0) group->minimum_capacity = tree_node_child_capacity(node);
    if (tree_node_child_capacity(node) < group->minimum_capacity)
        group->minimum_capacity = tree_node_child_capacity(node);
    if (tree_node_child_capacity(node) > group->maximum_capacity)
        group->maximum_capacity = tree_node_child_capacity(node);
    if (!tree_profile_add(&group->node_count, 1) ||
        !tree_profile_add(&group->capacity_sum, tree_node_child_capacity(node)) ||
        !tree_profile_add(&group->unused_slots_total, unused))
        return HPS_STATUS_INTERNAL_ERROR;
    for (index = 0; index < tree_node_child_count(node); ++index) {
        HpsStatus status = tree_profile_node(tree_node_children(node)[index], node, profile);
        if (status != HPS_STATUS_OK) return status;
    }
    return HPS_STATUS_OK;
}

HpsStatus hps_tree_internal_profile(const HpsTree *tree,
    HpsTreeInternalProfile *out_profile)
{
    size_t index;
    HpsTreeInternalProfile profile;
    if (out_profile == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    memset(out_profile, 0, sizeof(*out_profile));
    if (tree == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    if (tree->root.parent != NULL || tree->root.path != NULL || tree->root.item != NULL ||
        tree_node_child_count(&tree->root) > tree_node_child_capacity(&tree->root) ||
        ((tree_node_child_capacity(&tree->root) == 0) != (tree->root.child_block == NULL)))
        return HPS_STATUS_INTERNAL_ERROR;
    memset(&profile, 0, sizeof(profile));
    profile.root_child_count = tree_node_child_count(&tree->root);
    profile.root_child_capacity = tree_node_child_capacity(&tree->root);
    if (!tree_profile_add(&profile.total_child_count, tree_node_child_count(&tree->root)) ||
        !tree_profile_add(&profile.total_child_capacity, tree_node_child_capacity(&tree->root)))
        return HPS_STATUS_INTERNAL_ERROR;
    if (tree->root.child_block != NULL &&
        !tree_profile_add(&profile.allocated_child_array_count, 1))
        return HPS_STATUS_INTERNAL_ERROR;
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        HpsStatus status = tree_profile_node(tree_node_children(&tree->root)[index], &tree->root, &profile);
        if (status != HPS_STATUS_OK) return status;
    }
    if (profile.real_node_count != tree->size ||
        profile.total_child_count != profile.real_node_count ||
        profile.leaf_count + profile.unary_count + profile.branching_count != profile.real_node_count)
        return HPS_STATUS_INTERNAL_ERROR;
    *out_profile = profile;
    return HPS_STATUS_OK;
}

static HpsStatus tree_capacity_histogram_node(const HpsTreeNode *node,
    size_t *counts, size_t required_count)
{
    size_t index;
    if (tree_node_child_capacity(node) >= required_count ||
        counts[tree_node_child_capacity(node)] == (size_t)-1)
        return HPS_STATUS_INTERNAL_ERROR;
    ++counts[tree_node_child_capacity(node)];
    for (index = 0; index < tree_node_child_count(node); ++index) {
        HpsStatus status = tree_capacity_histogram_node(tree_node_children(node)[index], counts,
            required_count);
        if (status != HPS_STATUS_OK) return status;
    }
    return HPS_STATUS_OK;
}

HpsStatus hps_tree_internal_capacity_histogram(const HpsTree *tree,
    size_t *counts, size_t counts_capacity, size_t *out_required_count)
{
    HpsTreeInternalProfile profile;
    size_t index, required;
    HpsStatus status;
    if (out_required_count == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    *out_required_count = 0;
    status = hps_tree_internal_profile(tree, &profile);
    if (status != HPS_STATUS_OK) return status;
    if (profile.max_real_child_capacity == (size_t)-1)
        return HPS_STATUS_OUT_OF_MEMORY;
    required = profile.max_real_child_capacity + 1;
    *out_required_count = required;
    if (counts == NULL) return HPS_STATUS_OK;
    if (counts_capacity < required) return HPS_STATUS_BUFFER_TOO_SMALL;
    for (index = 0; index < required; ++index) counts[index] = 0;
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        status = tree_capacity_histogram_node(tree_node_children(&tree->root)[index], counts, required);
        if (status != HPS_STATUS_OK) return status;
    }
    return HPS_STATUS_OK;
}

size_t hps_tree_internal_sizeof_tree(void) { return sizeof(HpsTree); }
size_t hps_tree_internal_alignof_tree(void) { return _Alignof(HpsTree); }
size_t hps_tree_internal_sizeof_node(void) { return sizeof(HpsTreeNode); }
size_t hps_tree_internal_alignof_node(void) { return _Alignof(HpsTreeNode); }
size_t hps_tree_internal_child_block_header_size(void)
{ return offsetof(HpsTreeChildBlock, children); }
size_t hps_tree_internal_alignof_child_block(void)
{ return _Alignof(HpsTreeChildBlock); }



