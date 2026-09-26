#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"

typedef struct LksTreeChildBlock LksTreeChildBlock;

struct LksTreeNode {
    LksPath *path;
    void *item;
    struct LksTreeNode *parent;
    LksTreeChildBlock *child_block;
};

struct LksTreeChildBlock {
    size_t count;
    size_t capacity;
    LksTreeNode *children[];
};

struct LksTree {
    LksTreeNode root;
    size_t size;
};

static size_t tree_node_child_count(const LksTreeNode *node)
{ return node == NULL || node->child_block == NULL ? 0 : node->child_block->count; }

static size_t tree_node_child_capacity(const LksTreeNode *node)
{ return node == NULL || node->child_block == NULL ? 0 : node->child_block->capacity; }

static LksTreeNode *const *tree_node_children(const LksTreeNode *node)
{ return node == NULL || node->child_block == NULL ? NULL : node->child_block->children; }

static LksTreeNode **tree_node_children_mutable(LksTreeNode *node)
{ return node == NULL || node->child_block == NULL ? NULL : node->child_block->children; }

static int tree_child_block_bytes(size_t capacity, size_t *out_bytes)
{
    size_t header_bytes = offsetof(LksTreeChildBlock, children);
    size_t pointer_bytes;
    if (out_bytes == NULL || capacity > (size_t)-1 / sizeof(LksTreeNode *)) return 0;
    pointer_bytes = capacity * sizeof(LksTreeNode *);
    if (header_bytes > (size_t)-1 - pointer_bytes) return 0;
    *out_bytes = header_bytes + pointer_bytes;
    return 1;
}

static LksStatus child_lower_bound(
    const LksTreeNode *parent,
    const LksPath *path,
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
        LksStatus status;

        middle = low + (high - low) / 2;
        status = lks_path_compare(tree_node_children(parent)[middle]->path, path,
            &comparison);
        if (status != LKS_STATUS_OK) {
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
        LksStatus status;

        status = lks_path_compare(tree_node_children(parent)[low]->path, path,
            &comparison);
        if (status != LKS_STATUS_OK) {
            return status;
        }
        *out_found = comparison == 0;
    }
    return LKS_STATUS_OK;
}

static LksStatus find_prefix_node(
    const LksTree *tree,
    const LksPath *target,
    size_t prefix_depth,
    const LksTreeNode **out_node
)
{
    LksPath *prefix;
    const LksTreeNode *parent;
    LksDirection direction;
    size_t index;
    LksStatus status;

    *out_node = NULL;
    if (prefix_depth == 0 || prefix_depth > lks_path_depth(target)) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    direction = lks_path_direction(target);
    if (direction != LKS_DIRECTION_POSITIVE &&
        direction != LKS_DIRECTION_NEGATIVE) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    prefix = NULL;
    parent = &tree->root;
    for (index = 0; index < prefix_depth; ++index) {
        unsigned int slot;
        size_t level;
        size_t child_index;
        int found;
        const LksTreeNode *child;

        if (lks_path_get_slot(target, index, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(target, index, &level) != LKS_STATUS_OK) {
            lks_path_destroy(prefix);
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (index == 0) {
            prefix = lks_path_create_at_level(direction, slot, level);
            if (prefix == NULL) {
                return LKS_STATUS_OUT_OF_MEMORY;
            }
        } else {
            status = lks_path_append_at_level(prefix, slot, level);
            if (status != LKS_STATUS_OK) {
                lks_path_destroy(prefix);
                return status;
            }
        }

        status = child_lower_bound(parent, prefix, &child_index, &found);
        if (status != LKS_STATUS_OK) {
            lks_path_destroy(prefix);
            return status;
        }
        if (!found) {
            lks_path_destroy(prefix);
            return LKS_STATUS_NOT_FOUND;
        }
        child = tree_node_children(parent)[child_index];
        parent = child;
    }

    lks_path_destroy(prefix);
    *out_node = parent;
    return LKS_STATUS_OK;
}

static LksStatus reserve_child(LksTreeNode *parent)
{
    size_t capacity;
    size_t bytes;
    LksTreeChildBlock *block;
    size_t current_capacity = tree_node_child_capacity(parent);
    size_t current_count = tree_node_child_count(parent);

    if (current_count < current_capacity) {
        return LKS_STATUS_OK;
    }
    if (current_count == (size_t)-1) {
        return LKS_STATUS_OUT_OF_MEMORY;
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
        return LKS_STATUS_OUT_OF_MEMORY;
    }

    if (parent->child_block == NULL) {
        block = (LksTreeChildBlock *)lks_alloc_tagged(bytes, LKS_ALLOC_TAG_TREE_CHILDREN);
        if (block != NULL) {
            block->count = 0;
            block->capacity = capacity;
        }
    } else {
        block = (LksTreeChildBlock *)lks_realloc(parent->child_block, bytes);
        if (block != NULL) block->capacity = capacity;
    }
    if (block == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    parent->child_block = block;
    return LKS_STATUS_OK;
}

LksTree *lks_tree_create(void)
{
    LksTree *tree;

    tree = (LksTree *)lks_alloc_tagged(sizeof(*tree), LKS_ALLOC_TAG_TREE_OBJECT);
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

static void destroy_node(LksTreeNode *node)
{
    size_t index;

    for (index = 0; index < tree_node_child_count(node); ++index) {
        destroy_node(tree_node_children(node)[index]);
    }
    lks_free(node->child_block);
    lks_path_destroy(node->path);
    lks_free(node);
}

void lks_tree_destroy(LksTree *tree)
{
    size_t index;

    if (tree == NULL) {
        return;
    }
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        destroy_node(tree_node_children(&tree->root)[index]);
    }
    lks_free(tree->root.child_block);
    lks_free(tree);
}

size_t lks_tree_size(const LksTree *tree)
{
    return tree == NULL ? 0 : tree->size;
}

LksStatus lks_tree_insert(
    LksTree *tree,
    const LksPath *path,
    void *item,
    const LksTreeNode **out_node
)
{
    LksTreeNode *parent;
    LksPath *path_copy;
    LksTreeNode *node;
    size_t depth;
    size_t index;
    int found;
    LksStatus status;

    if (out_node != NULL) {
        *out_node = NULL;
    }
    if (tree == NULL || path == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    depth = lks_path_depth(path);
    if (lks_path_direction(path) == LKS_DIRECTION_ZERO) {
        if (depth != 0) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        parent = &tree->root;
    } else {
        if (depth == 0 ||
            (lks_path_direction(path) != LKS_DIRECTION_POSITIVE &&
             lks_path_direction(path) != LKS_DIRECTION_NEGATIVE)) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (depth == 1) {
            parent = &tree->root;
        } else {
            const LksTreeNode *found_parent;

            status = find_prefix_node(tree, path, depth - 1, &found_parent);
            if (status != LKS_STATUS_OK) {
                return status;
            }
            parent = (LksTreeNode *)found_parent;
        }
    }

    status = child_lower_bound(parent, path, &index, &found);
    if (status != LKS_STATUS_OK) {
        return status;
    }
    if (found) {
        return LKS_STATUS_ALREADY_EXISTS;
    }

    path_copy = lks_path_clone(path);
    if (path_copy == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    node = (LksTreeNode *)lks_alloc_tagged(sizeof(*node), LKS_ALLOC_TAG_TREE_NODE);
    if (node == NULL) {
        lks_path_destroy(path_copy);
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    node->path = path_copy;
    node->item = item;
    node->parent = parent;
    node->child_block = NULL;

    status = reserve_child(parent);
    if (status != LKS_STATUS_OK) {
        lks_path_destroy(node->path);
        lks_free(node);
        return status;
    }
    memmove(&tree_node_children_mutable(parent)[index + 1],
        &tree_node_children_mutable(parent)[index],
        (tree_node_child_count(parent) - index) * sizeof(LksTreeNode *));
    tree_node_children_mutable(parent)[index] = node;
    ++parent->child_block->count;
    ++tree->size;
    if (out_node != NULL) {
        *out_node = node;
    }
    return LKS_STATUS_OK;
}

static LksStatus locate_in_children(
    const LksTreeNode *parent,
    const void *item,
    const LksComparator *comparator,
    const LksTreeNode *upper_bound,
    const LksTreeNode **out_left,
    const LksTreeNode **out_equal,
    const LksTreeNode **out_right
)
{
    const LksTreeNode *current;
    const LksTreeNode *next_upper_bound;

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
            return LKS_STATUS_OK;
        }

        low = 0;
        high = tree_node_child_count(current);
        while (low < high) {
            size_t middle;
            const LksTreeNode *child;
            int comparison;

            middle = low + (high - low) / 2;
            child = tree_node_children(current)[middle];
            comparison = comparator->compare(
                item, child->item, comparator->context);
            if (comparison == 0) {
                *out_equal = child;
                return LKS_STATUS_OK;
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
            return LKS_STATUS_OK;
        }

        if (low < tree_node_child_count(current)) {
            next_upper_bound = tree_node_children(current)[low];
        }
        current = tree_node_children(current)[low - 1];
    }
}

LksStatus lks_tree_locate_item(
    const LksTree *tree,
    const void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_left,
    const LksTreeNode **out_equal,
    const LksTreeNode **out_right
)
{
    if (tree == NULL || comparator == NULL || comparator->compare == NULL ||
        out_left == NULL || out_equal == NULL || out_right == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    *out_left = NULL;
    *out_equal = NULL;
    *out_right = NULL;
    if (tree->size == 0) {
        return LKS_STATUS_OK;
    }

    return locate_in_children(&tree->root, item, comparator, NULL,
        out_left, out_equal, out_right);
}
static const LksTreeNode *tree_node_successor(
    const LksTree *tree,
    const LksTreeNode *node
)
{
    const LksTreeNode *current;

    if (tree == NULL || node == NULL) {
        return NULL;
    }
    if (tree_node_child_count(node) > 0) {
        return tree_node_children(node)[0];
    }

    current = node;
    while (current->parent != NULL && current->parent->path != NULL) {
        const LksTreeNode *parent;
        size_t index;
        int found;

        parent = current->parent;
        if (child_lower_bound(parent, current->path, &index, &found) !=
                LKS_STATUS_OK || !found) {
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
                LKS_STATUS_OK || !found || index + 1 >= tree_node_child_count(&tree->root)) {
            return NULL;
        }
        return tree_node_children(&tree->root)[index + 1];
    }
    return NULL;
}

LksStatus lks_tree_insert_item(
    LksTree *tree,
    void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_node
)
{
    const LksTreeNode *left;
    const LksTreeNode *equal;
    const LksTreeNode *right;
    const LksTreeNode *successor;
    LksPath *new_path;
    LksStatus status;

    if (out_node != NULL) {
        *out_node = NULL;
    }
    if (tree == NULL || comparator == NULL || comparator->compare == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    new_path = NULL;
    if (lks_tree_size(tree) == 0) {
        new_path = lks_path_create_zero();
        if (new_path == NULL) {
            return LKS_STATUS_OUT_OF_MEMORY;
        }
    } else {
        left = NULL;
        equal = NULL;
        right = NULL;
        status = lks_tree_locate_item(tree, item, comparator,
            &left, &equal, &right);
        if (status != LKS_STATUS_OK) {
            return status;
        }

        if (equal != NULL) {
            /* Insert after the complete comparator-equal run.  This keeps
             * item insertion stable when several items compare as equal. */
            successor = tree_node_successor(tree, equal);
            while (successor != NULL &&
                comparator->compare(item, lks_tree_node_item(successor),
                    comparator->context) == 0) {
                equal = successor;
                successor = tree_node_successor(tree, equal);
            }
            if (successor != NULL) {
                status = lks_path_between(lks_tree_node_path(equal),
                    lks_tree_node_path(successor), &new_path);
            } else {
                status = lks_path_after(lks_tree_node_path(equal), &new_path);
            }
        } else if (left == NULL && right != NULL) {
            status = lks_path_before(lks_tree_node_path(right), &new_path);
        } else if (left != NULL && right == NULL) {
            status = lks_path_after(lks_tree_node_path(left), &new_path);
        } else if (left != NULL && right != NULL) {
            status = lks_path_between(lks_tree_node_path(left),
                lks_tree_node_path(right), &new_path);
        } else {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (status != LKS_STATUS_OK) {
            lks_path_destroy(new_path);
            return status;
        }
    }

    status = lks_tree_insert(tree, new_path, item, out_node);
    lks_path_destroy(new_path);
    if (status == LKS_STATUS_ALREADY_EXISTS || status == LKS_STATUS_NOT_FOUND) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    return status;
}
LksStatus lks_tree_find_path(
    const LksTree *tree,
    const LksPath *path,
    const LksTreeNode **out_node
)
{
    size_t depth;

    if (out_node == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_node = NULL;
    if (tree == NULL || path == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    depth = lks_path_depth(path);
    if (lks_path_direction(path) == LKS_DIRECTION_ZERO) {
        size_t index;
        int found;
        LksStatus status;

        if (depth != 0) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        status = child_lower_bound(&tree->root, path, &index, &found);
        if (status != LKS_STATUS_OK) {
            return status;
        }
        if (!found) {
            return LKS_STATUS_NOT_FOUND;
        }
        *out_node = tree_node_children(&tree->root)[index];
        return LKS_STATUS_OK;
    }
    if (depth == 0) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    return find_prefix_node(tree, path, depth, out_node);
}

const LksPath *lks_tree_node_path(const LksTreeNode *node)
{
    return node == NULL ? NULL : node->path;
}

void *lks_tree_node_item(const LksTreeNode *node)
{
    return node == NULL ? NULL : node->item;
}

const LksTreeNode *lks_tree_node_parent(const LksTreeNode *node)
{
    if (node == NULL || node->parent == NULL || node->parent->path == NULL) {
        return NULL;
    }
    return node->parent;
}

size_t lks_tree_node_child_count(const LksTreeNode *node)
{
    return tree_node_child_count(node);
}

const LksTreeNode *lks_tree_node_child_at(
    const LksTreeNode *node,
    size_t index
)
{
    if (node == NULL || index >= tree_node_child_count(node)) {
        return NULL;
    }
    return tree_node_children(node)[index];
}

size_t lks_tree_root_child_count(const LksTree *tree)
{
    return tree == NULL ? 0 : tree_node_child_count(&tree->root);
}

const LksTreeNode *lks_tree_root_child_at(const LksTree *tree, size_t index)
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

static LksStatus tree_profile_node(const LksTreeNode *node,
    const LksTreeNode *expected_parent, LksTreeInternalProfile *profile)
{
    size_t bucket, joint, index, unused;
    LksTreeInternalDegreeCapacity *group;
    if (node == NULL || node->parent != expected_parent ||
        tree_node_child_count(node) > tree_node_child_capacity(node) ||
        ((tree_node_child_capacity(node) == 0) != (node->child_block == NULL)))
        return LKS_STATUS_INTERNAL_ERROR;
    if (!tree_profile_add(&profile->real_node_count, 1) ||
        !tree_profile_add(&profile->total_child_count, tree_node_child_count(node)) ||
        !tree_profile_add(&profile->total_child_capacity, tree_node_child_capacity(node)))
        return LKS_STATUS_INTERNAL_ERROR;
    if (node->child_block != NULL) {
        if (!tree_profile_add(&profile->allocated_child_array_count, 1) ||
            !tree_profile_add(&profile->real_nodes_with_child_array_count, 1))
            return LKS_STATUS_INTERNAL_ERROR;
    }
    if (tree_node_child_capacity(node) > profile->max_real_child_capacity)
        profile->max_real_child_capacity = tree_node_child_capacity(node);
    bucket = tree_profile_degree_bucket(tree_node_child_count(node));
    if (!tree_profile_add(&profile->degree_bucket_counts[bucket], 1))
        return LKS_STATUS_INTERNAL_ERROR;
    if (tree_node_child_count(node) == 0) {
        if (!tree_profile_add(&profile->leaf_count, 1)) return LKS_STATUS_INTERNAL_ERROR;
    } else if (tree_node_child_count(node) == 1) {
        if (!tree_profile_add(&profile->unary_count, 1)) return LKS_STATUS_INTERNAL_ERROR;
    } else if (!tree_profile_add(&profile->branching_count, 1)) {
        return LKS_STATUS_INTERNAL_ERROR;
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
        return LKS_STATUS_INTERNAL_ERROR;
    for (index = 0; index < tree_node_child_count(node); ++index) {
        LksStatus status = tree_profile_node(tree_node_children(node)[index], node, profile);
        if (status != LKS_STATUS_OK) return status;
    }
    return LKS_STATUS_OK;
}

LksStatus lks_tree_internal_profile(const LksTree *tree,
    LksTreeInternalProfile *out_profile)
{
    size_t index;
    LksTreeInternalProfile profile;
    if (out_profile == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    memset(out_profile, 0, sizeof(*out_profile));
    if (tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    if (tree->root.parent != NULL || tree->root.path != NULL || tree->root.item != NULL ||
        tree_node_child_count(&tree->root) > tree_node_child_capacity(&tree->root) ||
        ((tree_node_child_capacity(&tree->root) == 0) != (tree->root.child_block == NULL)))
        return LKS_STATUS_INTERNAL_ERROR;
    memset(&profile, 0, sizeof(profile));
    profile.root_child_count = tree_node_child_count(&tree->root);
    profile.root_child_capacity = tree_node_child_capacity(&tree->root);
    if (!tree_profile_add(&profile.total_child_count, tree_node_child_count(&tree->root)) ||
        !tree_profile_add(&profile.total_child_capacity, tree_node_child_capacity(&tree->root)))
        return LKS_STATUS_INTERNAL_ERROR;
    if (tree->root.child_block != NULL &&
        !tree_profile_add(&profile.allocated_child_array_count, 1))
        return LKS_STATUS_INTERNAL_ERROR;
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        LksStatus status = tree_profile_node(tree_node_children(&tree->root)[index], &tree->root, &profile);
        if (status != LKS_STATUS_OK) return status;
    }
    if (profile.real_node_count != tree->size ||
        profile.total_child_count != profile.real_node_count ||
        profile.leaf_count + profile.unary_count + profile.branching_count != profile.real_node_count)
        return LKS_STATUS_INTERNAL_ERROR;
    *out_profile = profile;
    return LKS_STATUS_OK;
}

static LksStatus tree_capacity_histogram_node(const LksTreeNode *node,
    size_t *counts, size_t required_count)
{
    size_t index;
    if (tree_node_child_capacity(node) >= required_count ||
        counts[tree_node_child_capacity(node)] == (size_t)-1)
        return LKS_STATUS_INTERNAL_ERROR;
    ++counts[tree_node_child_capacity(node)];
    for (index = 0; index < tree_node_child_count(node); ++index) {
        LksStatus status = tree_capacity_histogram_node(tree_node_children(node)[index], counts,
            required_count);
        if (status != LKS_STATUS_OK) return status;
    }
    return LKS_STATUS_OK;
}

LksStatus lks_tree_internal_capacity_histogram(const LksTree *tree,
    size_t *counts, size_t counts_capacity, size_t *out_required_count)
{
    LksTreeInternalProfile profile;
    size_t index, required;
    LksStatus status;
    if (out_required_count == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_required_count = 0;
    status = lks_tree_internal_profile(tree, &profile);
    if (status != LKS_STATUS_OK) return status;
    if (profile.max_real_child_capacity == (size_t)-1)
        return LKS_STATUS_OUT_OF_MEMORY;
    required = profile.max_real_child_capacity + 1;
    *out_required_count = required;
    if (counts == NULL) return LKS_STATUS_OK;
    if (counts_capacity < required) return LKS_STATUS_BUFFER_TOO_SMALL;
    for (index = 0; index < required; ++index) counts[index] = 0;
    for (index = 0; index < tree_node_child_count(&tree->root); ++index) {
        status = tree_capacity_histogram_node(tree_node_children(&tree->root)[index], counts, required);
        if (status != LKS_STATUS_OK) return status;
    }
    return LKS_STATUS_OK;
}

size_t lks_tree_internal_sizeof_tree(void) { return sizeof(LksTree); }
size_t lks_tree_internal_alignof_tree(void) { return _Alignof(LksTree); }
size_t lks_tree_internal_sizeof_node(void) { return sizeof(LksTreeNode); }
size_t lks_tree_internal_alignof_node(void) { return _Alignof(LksTreeNode); }
size_t lks_tree_internal_child_block_header_size(void)
{ return offsetof(LksTreeChildBlock, children); }
size_t lks_tree_internal_alignof_child_block(void)
{ return _Alignof(LksTreeChildBlock); }



