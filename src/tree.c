#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"
#include "lks_bulk_internal.h"
#include "lks_policy_internal.h"

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

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
static LksTreeRepairStats repair_stats;
#define REPAIR_COUNT(field) (++repair_stats.field)
#else
#define REPAIR_COUNT(field) ((void)0)
#endif

void lks_tree_repair_stats_reset(void)
{
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    memset(&repair_stats, 0, sizeof(repair_stats));
#endif
}

LksTreeRepairStats lks_tree_repair_stats_get(void)
{
    LksTreeRepairStats result;
    memset(&result, 0, sizeof(result));
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    result = repair_stats;
#endif
    return result;
}

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

static void collect_subtree_items(const LksTreeNode *node, void **items,
    size_t *write_index)
{
    size_t child;
    items[(*write_index)++] = node->item;
    for (child = 0; child < tree_node_child_count(node); ++child)
        collect_subtree_items(tree_node_children(node)[child], items, write_index);
}

static const LksTreeNode *node_at_preorder(const LksTreeNode *node,
    size_t target, size_t *cursor)
{
    size_t child;
    const LksTreeNode *found;
    if ((*cursor)++ == target) return node;
    for (child = 0; child < tree_node_child_count(node); ++child) {
        found = node_at_preorder(tree_node_children(node)[child], target, cursor);
        if (found != NULL) return found;
    }
    return NULL;
}

static size_t bounded_subtree_size(const LksTreeNode *node, size_t limit)
{
    size_t count = 1, child;
    for (child = 0; child < tree_node_child_count(node) && count <= limit; ++child) {
        size_t remaining = limit - count;
        size_t descendants = bounded_subtree_size(tree_node_children(node)[child],
            remaining);
        count += descendants;
    }
    return count;
}

static size_t subtree_max_depth(const LksTreeNode *node)
{
    size_t depth = lks_path_depth(node->path), child;
    for (child = 0; child < tree_node_child_count(node); ++child) {
        size_t child_depth = subtree_max_depth(tree_node_children(node)[child]);
        if (child_depth > depth) depth = child_depth;
    }
    return depth;
}

/* A complete positive subtree is closed under Path prefixes. Its root Path
 * stays fixed, so every external sibling and ancestor keeps its coordinate;
 * replacing the one parent link leaves all external preorder relations intact. */
static LksStatus try_local_subtree(LksTree *tree, const LksTreeNode *predecessor,
    void *item, const LksComparator *comparator, size_t candidate_depth,
    int topology_problem, const LksTreeNode **out_node, int *out_repaired)
{
    const LksTreeNode *anchor = predecessor;
    size_t window = LKS_POLICY_LOCAL_INITIAL_NODES;
    void *items[LKS_POLICY_LOCAL_MAX_NODES + 1];
    *out_repaired = 0;
    REPAIR_COUNT(attempts);
    while (anchor != NULL && anchor->path != NULL &&
        lks_path_direction(anchor->path) == LKS_DIRECTION_POSITIVE) {
        /* Even a one-item branch must place the new item below its root.
         * Skip ancestors that cannot meet the depth-gain policy. */
        if (!topology_problem &&
            (candidate_depth <= LKS_POLICY_LOCAL_MIN_DEPTH_GAIN ||
             lks_path_depth(anchor->path) >=
                candidate_depth - LKS_POLICY_LOCAL_MIN_DEPTH_GAIN)) {
            anchor = anchor->parent;
            continue;
        }
        size_t count = bounded_subtree_size(anchor, LKS_POLICY_LOCAL_MAX_NODES);
        while (count > window && window < LKS_POLICY_LOCAL_MAX_NODES) {
            size_t next = window * LKS_POLICY_LOCAL_EXPANSION_FACTOR;
            window = next > LKS_POLICY_LOCAL_MAX_NODES ?
                LKS_POLICY_LOCAL_MAX_NODES : next;
            REPAIR_COUNT(region_expansions);
        }
        if (count <= window) {
            size_t write_index = 0, position = 0, cursor = 0, new_depth;
            const LksTreeNode *new_anchor = NULL, *inserted;
            LksTree *temporary = NULL;
            LksTreeNode *parent;
            size_t child_index;
            int found;
            LksStatus status;
            collect_subtree_items(anchor, items, &write_index);
            if (write_index != count) return LKS_STATUS_INTERNAL_ERROR;
            while (position < count && comparator->compare(items[position], item,
                comparator->context) <= 0) ++position;
            if (position == 0) return LKS_STATUS_INTERNAL_ERROR;
            memmove(items + position + 1, items + position,
                (count - position) * sizeof(*items));
            items[position] = item;
            status = lks_bulk_build_branch(anchor->path, items, count + 1,
                &temporary);
            if (status == LKS_STATUS_LEVEL_LIMIT) {
                anchor = anchor->parent;
                continue;
            }
            if (status != LKS_STATUS_OK) return status;
            status = lks_tree_find_path(temporary, anchor->path, &new_anchor);
            if (status != LKS_STATUS_OK || new_anchor == NULL) {
                lks_tree_destroy(temporary);
                return status == LKS_STATUS_OK ? LKS_STATUS_INTERNAL_ERROR : status;
            }
            inserted = node_at_preorder(new_anchor, position, &cursor);
            if (inserted == NULL) {
                lks_tree_destroy(temporary);
                return LKS_STATUS_INTERNAL_ERROR;
            }
            new_depth = subtree_max_depth(new_anchor);
            if (new_depth <= LKS_POLICY_HARD_ONLINE_DEPTH &&
                (topology_problem ||
                 (candidate_depth >= new_depth &&
                  candidate_depth - new_depth >= LKS_POLICY_LOCAL_MIN_DEPTH_GAIN))) {
                parent = (LksTreeNode *)anchor->parent;
                status = child_lower_bound(parent, anchor->path,
                    &child_index, &found);
                if (status != LKS_STATUS_OK || !found ||
                    tree_node_children(parent)[child_index] != anchor ||
                    tree_node_child_count(new_anchor->parent) != 1) {
                    lks_tree_destroy(temporary);
                    return LKS_STATUS_INTERNAL_ERROR;
                }
                /* All validation and allocation precede this pointer swap.
                 * Detach the prepared branch so its temporary ancestors can
                 * be destroyed without touching the committed replacement. */
                ((LksTreeNode *)new_anchor->parent)->child_block->count = 0;
                ((LksTreeNode *)new_anchor)->parent = parent;
                tree_node_children_mutable(parent)[child_index] = (LksTreeNode *)new_anchor;
                ++tree->size;
                destroy_node((LksTreeNode *)anchor);
                lks_tree_destroy(temporary);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
                repair_stats.nodes_relabelled += count;
                if (count > repair_stats.max_region_nodes)
                    repair_stats.max_region_nodes = count;
#endif
                REPAIR_COUNT(successes);
                if (out_node != NULL) *out_node = inserted;
                *out_repaired = 1;
                return LKS_STATUS_OK;
            }
            lks_tree_destroy(temporary);
        }
        anchor = anchor->parent;
    }
    REPAIR_COUNT(fallbacks);
    return LKS_STATUS_OK;
}

static LksStatus rebuild_with_item(LksTree *tree, void *item,
    const LksComparator *comparator, const LksTreeNode **out_node)
{
    void **items;
    LksTree *replacement = NULL;
    size_t index = 0, position = 0, child, cursor = 0;
    const LksTreeNode *inserted = NULL;
    LksTreeChildBlock *old_block;
    size_t old_size;
    LksStatus status;
    if (tree->size == (size_t)-1 || tree->size + 1 > (size_t)-1 / sizeof(*items))
        return LKS_STATUS_OUT_OF_MEMORY;
    items = (void **)lks_alloc_tagged((tree->size + 1) * sizeof(*items),
        LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (items == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    for (child = 0; child < tree_node_child_count(&tree->root); ++child)
        collect_subtree_items(tree_node_children(&tree->root)[child], items, &index);
    if (index != tree->size) { lks_free(items); return LKS_STATUS_INTERNAL_ERROR; }
    /* Equality goes after the existing run. The old Tree is untouched. */
    while (position < tree->size &&
        comparator->compare(items[position], item, comparator->context) <= 0)
        ++position;
    memmove(items + position + 1, items + position,
        (tree->size - position) * sizeof(*items));
    items[position] = item;
    status = lks_bulk_build_tree(items, tree->size + 1, &replacement);
    lks_free(items);
    if (status != LKS_STATUS_OK) return status;
    for (child = 0; child < tree_node_child_count(&replacement->root); ++child) {
        inserted = node_at_preorder(tree_node_children(&replacement->root)[child],
            position, &cursor);
        if (inserted != NULL) break;
    }
    if (inserted == NULL) { lks_tree_destroy(replacement); return LKS_STATUS_INTERNAL_ERROR; }
    /* Commit is allocation-free. Reparent only direct root children because
     * every deeper parent pointer already points inside the new Tree. */
    old_block = tree->root.child_block;
    old_size = tree->size;
    tree->root.child_block = replacement->root.child_block;
    tree->size = replacement->size;
    replacement->root.child_block = old_block;
    replacement->size = old_size;
    for (child = 0; child < tree_node_child_count(&tree->root); ++child)
        tree_node_children_mutable(&tree->root)[child]->parent = &tree->root;
    lks_tree_destroy(replacement);
    REPAIR_COUNT(full_rebuilds);
    if (out_node != NULL) *out_node = inserted;
    return LKS_STATUS_OK;
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
    const LksTreeNode *predecessor = NULL;
    LksPath *new_path;
    LksStatus status;
    size_t candidate_depth;
    int repaired = 0, topology_problem = 0;

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
            predecessor = equal;
            if (successor != NULL) {
                status = lks_path_between(lks_tree_node_path(equal),
                    lks_tree_node_path(successor), &new_path);
            } else {
                status = lks_path_after(lks_tree_node_path(equal), &new_path);
            }
        } else if (left == NULL && right != NULL) {
            status = lks_path_before(lks_tree_node_path(right), &new_path);
        } else if (left != NULL && right == NULL) {
            predecessor = left;
            status = lks_path_after(lks_tree_node_path(left), &new_path);
        } else if (left != NULL && right != NULL) {
            predecessor = left;
            status = lks_path_between(lks_tree_node_path(left),
                lks_tree_node_path(right), &new_path);
        } else {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (status != LKS_STATUS_OK) {
            lks_path_destroy(new_path);
            if (status == LKS_STATUS_LEVEL_LIMIT)
                return rebuild_with_item(tree, item, comparator, out_node);
            return status;
        }
    }

    candidate_depth = lks_path_depth(new_path);
    if (candidate_depth <= LKS_POLICY_PREFERRED_ONLINE_DEPTH) {
        status = lks_tree_insert(tree, new_path, item, out_node);
        if (status == LKS_STATUS_OK ||
            (status != LKS_STATUS_ALREADY_EXISTS && status != LKS_STATUS_NOT_FOUND)) {
            lks_path_destroy(new_path);
            return status;
        }
        topology_problem = 1;
    }
    if (predecessor != NULL) {
        status = try_local_subtree(tree, predecessor, item, comparator,
            candidate_depth, topology_problem, out_node, &repaired);
        if (status != LKS_STATUS_OK || repaired) {
            lks_path_destroy(new_path);
            return status;
        }
    }
    if (candidate_depth > LKS_POLICY_HARD_ONLINE_DEPTH) {
        lks_path_destroy(new_path);
        return rebuild_with_item(tree, item, comparator, out_node);
    }
    if (topology_problem) {
        /* The same candidate already failed without mutating the Tree. */
        lks_path_destroy(new_path);
        return rebuild_with_item(tree, item, comparator, out_node);
    }
    status = lks_tree_insert(tree, new_path, item, out_node);
    lks_path_destroy(new_path);
    if (status == LKS_STATUS_ALREADY_EXISTS || status == LKS_STATUS_NOT_FOUND) {
        /* A standalone gap candidate need not have an existing Tree prefix.
         * Both statuses leave the Tree untouched, so rebuild from item order. */
        return rebuild_with_item(tree, item, comparator, out_node);
    }
    if (status == LKS_STATUS_OK && candidate_depth > LKS_POLICY_PREFERRED_ONLINE_DEPTH)
        REPAIR_COUNT(deeper_accepts);
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



