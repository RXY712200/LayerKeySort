#include <stddef.h>
#include <string.h>
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_tree_internal.h"
#include "lks_bulk_internal.h"
#include "lks_policy_internal.h"

struct LksTreeNode {
    LksPath *path;
    void *item;
    struct LksTreeNode *left;
    struct LksTreeNode *right;
    struct LksTreeNode *parent;
    int height;
};

struct LksTree {
    LksTreeNode *root;
    size_t size;
};

struct LksOrderedTree {
    LksTree *index;
    LksComparator comparator;
    size_t append_run;
    size_t prepend_run;
};

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
static LksTreeRepairStats repair_stats;
#define REPAIR_COUNT(member) (++repair_stats.member)
#define REPAIR_ADD(member, amount) do { \
    size_t add_ = (amount); \
    repair_stats.member = repair_stats.member > (size_t)-1 - add_ ? \
        (size_t)-1 : repair_stats.member + add_; \
} while (0)
#else
#define REPAIR_COUNT(member) ((void)0)
#define REPAIR_ADD(member, amount) ((void)0)
#endif

void lks_tree_repair_stats_reset(void)
{
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    memset(&repair_stats, 0, sizeof(repair_stats));
#endif
}

LksTreeRepairStats lks_tree_repair_stats_get(void)
{
    LksTreeRepairStats value;
    memset(&value, 0, sizeof(value));
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    value = repair_stats;
#endif
    return value;
}

static int height(const LksTreeNode *node)
{
    return node == NULL ? 0 : node->height;
}

static void update_height(LksTreeNode *node)
{
    int left = height(node->left), right = height(node->right);
    node->height = 1 + (left > right ? left : right);
}

static void replace_parent_link(LksTree *tree, LksTreeNode *old_node,
    LksTreeNode *new_node)
{
    LksTreeNode *parent = old_node->parent;
    if (parent == NULL) tree->root = new_node;
    else if (parent->left == old_node) parent->left = new_node;
    else parent->right = new_node;
    new_node->parent = parent;
}

/* Unlike rotation replacement, removal may replace a node with NULL. */
static void splice_parent_link(LksTree *tree, LksTreeNode *old_node,
    LksTreeNode *new_node)
{
    LksTreeNode *parent = old_node->parent;
    if (parent == NULL) tree->root = new_node;
    else if (parent->left == old_node) parent->left = new_node;
    else parent->right = new_node;
    if (new_node != NULL) new_node->parent = parent;
}

static LksTreeNode *rotate_left(LksTree *tree, LksTreeNode *node)
{
    LksTreeNode *pivot = node->right;
    replace_parent_link(tree, node, pivot);
    node->right = pivot->left;
    if (node->right != NULL) node->right->parent = node;
    pivot->left = node;
    node->parent = pivot;
    update_height(node);
    update_height(pivot);
    REPAIR_COUNT(rotations);
    return pivot;
}

static LksTreeNode *rotate_right(LksTree *tree, LksTreeNode *node)
{
    LksTreeNode *pivot = node->left;
    replace_parent_link(tree, node, pivot);
    node->left = pivot->right;
    if (node->left != NULL) node->left->parent = node;
    pivot->right = node;
    node->parent = pivot;
    update_height(node);
    update_height(pivot);
    REPAIR_COUNT(rotations);
    return pivot;
}

static void rebalance_up(LksTree *tree, LksTreeNode *node)
{
    while (node != NULL) {
        int balance;
        update_height(node);
        balance = height(node->left) - height(node->right);
        if (balance > 1) {
            if (height(node->left->left) < height(node->left->right))
                rotate_left(tree, node->left);
            node = rotate_right(tree, node);
        } else if (balance < -1) {
            if (height(node->right->right) < height(node->right->left))
                rotate_right(tree, node->right);
            node = rotate_left(tree, node);
        }
        node = node->parent;
    }
}

static LksStatus search_path(const LksTree *tree, const LksPath *path,
    LksTreeNode **out_found, LksTreeNode **out_parent, int *out_order)
{
    LksTreeNode *current = tree->root, *parent = NULL;
    int order = 0;
    while (current != NULL) {
        LksStatus status = lks_path_compare(path, current->path, &order);
        if (status != LKS_STATUS_OK) return status;
        if (order == 0) break;
        parent = current;
        current = order < 0 ? current->left : current->right;
    }
    *out_found = current;
    *out_parent = parent;
    *out_order = order;
    return LKS_STATUS_OK;
}

/* Once SEARCH_PATH has established a unique position, linking and AVL
 * rotations require no allocations or recoverable operations. */
static void link_prepared(LksTree *tree, LksTreeNode *node,
    LksTreeNode *parent, int order)
{
    node->parent = parent;
    if (parent == NULL) tree->root = node;
    else if (order < 0) parent->left = node;
    else parent->right = node;
    ++tree->size;
    rebalance_up(tree, parent);
}

static LksTreeNode *first_node(LksTreeNode *node)
{
    if (node != NULL) while (node->left != NULL) node = node->left;
    return node;
}

static LksTreeNode *last_node(LksTreeNode *node)
{
    if (node != NULL) while (node->right != NULL) node = node->right;
    return node;
}

static LksTreeNode *successor(const LksTreeNode *node)
{
    LksTreeNode *current;
    if (node == NULL) return NULL;
    if (node->right != NULL) return first_node(node->right);
    current = node->parent;
    while (current != NULL && current->right == node) {
        node = current;
        current = current->parent;
    }
    return current;
}

static LksTreeNode *predecessor(const LksTreeNode *node)
{
    LksTreeNode *current;
    if (node == NULL) return NULL;
    if (node->left != NULL) return last_node(node->left);
    current = node->parent;
    while (current != NULL && current->left == node) {
        node = current;
        current = current->parent;
    }
    return current;
}

/* Detach exactly NODE, preserving every other node's Path/item association.
 * The successor is transplanted structurally; no allocation or Path relabel
 * occurs. Rebalance from the lowest changed ancestor to the new root. */
static void detach_node(LksTree *tree, LksTreeNode *node)
{
    LksTreeNode *start;
    if (node->left == NULL || node->right == NULL) {
        start = node->parent;
        splice_parent_link(tree, node,
            node->left != NULL ? node->left : node->right);
        if (start == NULL) start = tree->root;
    } else {
        LksTreeNode *replacement = first_node(node->right);
        if (replacement->parent == node) {
            splice_parent_link(tree, node, replacement);
            replacement->left = node->left;
            replacement->left->parent = replacement;
            start = replacement;
        } else {
            start = replacement->parent;
            splice_parent_link(tree, replacement, replacement->right);
            splice_parent_link(tree, node, replacement);
            replacement->left = node->left;
            replacement->right = node->right;
            replacement->left->parent = replacement;
            replacement->right->parent = replacement;
        }
        replacement->height = node->height;
    }
    --tree->size;
    node->left = NULL; node->right = NULL; node->parent = NULL;
    node->height = 1;
    rebalance_up(tree, start);
}

LksTree *lks_tree_create(void)
{
    LksTree *tree = (LksTree *)lks_alloc_tagged(sizeof(*tree),
        LKS_ALLOC_TAG_TREE_OBJECT);
    if (tree != NULL) { tree->root = NULL; tree->size = 0; }
    return tree;
}

static void destroy_nodes(LksTreeNode *node)
{
    /* AVL height depends on node count, never on user-supplied Path depth. */
    if (node == NULL) return;
    destroy_nodes(node->left);
    destroy_nodes(node->right);
    lks_path_destroy(node->path);
    lks_free(node);
}

void lks_tree_destroy(LksTree *tree)
{
    if (tree == NULL) return;
    destroy_nodes(tree->root);
    lks_free(tree);
}

size_t lks_tree_size(const LksTree *tree)
{
    return tree == NULL ? 0 : tree->size;
}

LksStatus lks_tree_insert(LksTree *tree, const LksPath *path, void *item,
    const LksTreeNode **out_node)
{
    LksTreeNode *found, *parent, *node;
    LksPath *copy;
    int order;
    LksStatus status;
    if (out_node != NULL) *out_node = NULL;
    if (tree == NULL || path == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    if (tree->size == (size_t)-1) return LKS_STATUS_OUT_OF_MEMORY;
    status = search_path(tree, path, &found, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (found != NULL) return LKS_STATUS_ALREADY_EXISTS;
    copy = lks_path_clone(path);
    if (copy == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    node = (LksTreeNode *)lks_alloc_tagged(sizeof(*node), LKS_ALLOC_TAG_TREE_NODE);
    if (node == NULL) { lks_path_destroy(copy); return LKS_STATUS_OUT_OF_MEMORY; }
    node->path = copy; node->item = item;
    node->left = NULL; node->right = NULL; node->parent = NULL; node->height = 1;
    link_prepared(tree, node, parent, order);
    if (out_node != NULL) *out_node = node;
    return LKS_STATUS_OK;
}

/* Comparator insertion already owns its generated candidate. On success the
 * node takes it; on failure the caller keeps and destroys it. This avoids a
 * short-lived Path clone while retaining the public explicit-insert contract. */
static LksStatus insert_owned_path(LksTree *tree, LksPath *path, void *item,
    const LksTreeNode **out_node)
{
    LksTreeNode *found, *parent, *node;
    int order;
    LksStatus status;
    if (tree->size == (size_t)-1) return LKS_STATUS_OUT_OF_MEMORY;
    status = search_path(tree, path, &found, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (found != NULL) return LKS_STATUS_ALREADY_EXISTS;
    node = (LksTreeNode *)lks_alloc_tagged(sizeof(*node), LKS_ALLOC_TAG_TREE_NODE);
    if (node == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    node->path = path; node->item = item;
    node->left = NULL; node->right = NULL; node->parent = NULL; node->height = 1;
    link_prepared(tree, node, parent, order);
    if (out_node != NULL) *out_node = node;
    return LKS_STATUS_OK;
}

LksStatus lks_tree_find_path(const LksTree *tree, const LksPath *path,
    const LksTreeNode **out_node)
{
    LksTreeNode *found, *parent;
    int order;
    LksStatus status;
    if (out_node == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_node = NULL;
    if (tree == NULL || path == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    status = search_path(tree, path, &found, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (found == NULL) return LKS_STATUS_NOT_FOUND;
    *out_node = found;
    return LKS_STATUS_OK;
}

LksStatus lks_tree_remove_path(LksTree *tree, const LksPath *path,
    void **out_item)
{
    LksTreeNode *found, *parent;
    LksStatus status;
    int order;
    if (out_item != NULL) *out_item = NULL;
    if (tree == NULL || path == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    status = search_path(tree, path, &found, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (found == NULL) return LKS_STATUS_NOT_FOUND;
    if (out_item != NULL) *out_item = found->item;
    detach_node(tree, found);
    lks_path_destroy(found->path);
    lks_free(found);
    return LKS_STATUS_OK;
}

LksStatus lks_tree_rekey(LksTree *tree, const LksPath *old_path,
    const LksPath *new_path, const LksTreeNode **out_node)
{
    LksTreeNode *old_node, *new_found, *parent, *prepared;
    LksPath *new_copy;
    LksStatus status;
    int order;
    if (out_node != NULL) *out_node = NULL;
    if (tree == NULL || old_path == NULL || new_path == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    status = search_path(tree, old_path, &old_node, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (old_node == NULL) return LKS_STATUS_NOT_FOUND;
    status = search_path(tree, new_path, &new_found, &parent, &order);
    if (status != LKS_STATUS_OK) return status;
    if (new_found == old_node) {
        /* Same coordinate: no structural mutation or allocation. */
        if (out_node != NULL) *out_node = old_node;
        return LKS_STATUS_OK;
    }
    if (new_found != NULL) return LKS_STATUS_ALREADY_EXISTS;
    new_copy = lks_path_clone(new_path);
    if (new_copy == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    prepared = (LksTreeNode *)lks_alloc_tagged(sizeof(*prepared),
        LKS_ALLOC_TAG_TREE_NODE);
    if (prepared == NULL) {
        lks_path_destroy(new_copy);
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    prepared->path = new_copy; prepared->item = old_node->item;
    prepared->left = NULL; prepared->right = NULL;
    prepared->parent = NULL; prepared->height = 1;
    /* Both positions are already validated. From here through removal every
     * operation is allocation-free and cannot return a recoverable failure. */
    link_prepared(tree, prepared, parent, order);
    detach_node(tree, old_node);
    lks_path_destroy(old_node->path);
    lks_free(old_node);
    if (out_node != NULL) *out_node = prepared;
    return LKS_STATUS_OK;
}

/* Comparator order must be monotone in Path in-order traversal. */
static void item_bounds(const LksTree *tree, const void *item,
    const LksComparator *comparator, LksTreeNode **out_before,
    LksTreeNode **out_first_equal, LksTreeNode **out_after)
{
    LksTreeNode *node = tree->root;
    LksTreeNode *before = NULL, *first_equal = NULL, *after = NULL;
    while (node != NULL) {
        int order = comparator->compare(item, node->item, comparator->context);
        REPAIR_COUNT(comparator_search_steps);
        if (order <= 0) {
            after = node;
            if (order == 0) first_equal = node;
            node = node->left;
        } else {
            before = node;
            node = node->right;
        }
    }
    *out_before = before;
    *out_first_equal = first_equal;
    *out_after = after;
}

static void item_upper_bound(const LksTree *tree, const void *item,
    const LksComparator *comparator, LksTreeNode **out_before,
    LksTreeNode **out_after)
{
    LksTreeNode *node = tree->root;
    LksTreeNode *before = NULL, *after = NULL;
    while (node != NULL) {
        int order = comparator->compare(item, node->item, comparator->context);
        REPAIR_COUNT(comparator_search_steps);
        if (order < 0) { after = node; node = node->left; }
        else { before = node; node = node->right; }
    }
    *out_before = before;
    *out_after = after;
}

static LksStatus locate_by_comparator(const LksTree *tree,
    const void *item, const LksComparator *comparator,
    const LksTreeNode **out_left,
    const LksTreeNode **out_equal, const LksTreeNode **out_right)
{
    LksTreeNode *before, *equal, *after;
    if (out_left == NULL || out_equal == NULL || out_right == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    *out_left = NULL; *out_equal = NULL; *out_right = NULL;
    if (tree == NULL || comparator == NULL || comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    item_bounds(tree, item, comparator, &before, &equal, &after);
    if (equal != NULL) *out_equal = equal;
    else { *out_left = before; *out_right = after; }
    return LKS_STATUS_OK;
}

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
LksStatus lks_tree_internal_locate_item(const LksTree *tree,
    const void *item, const LksComparator *comparator,
    const LksTreeNode **out_left, const LksTreeNode **out_equal,
    const LksTreeNode **out_right)
{
    return locate_by_comparator(tree, item, comparator,
        out_left, out_equal, out_right);
}
#endif

LksStatus lks_ordered_tree_locate(const LksOrderedTree *ordered,
    const void *item, const LksTreeNode **out_left,
    const LksTreeNode **out_equal, const LksTreeNode **out_right)
{
    return locate_by_comparator(
        ordered == NULL ? NULL : ordered->index, item,
        ordered == NULL ? NULL : &ordered->comparator,
        out_left, out_equal, out_right);
}

LksOrderedTree *lks_ordered_tree_create(const LksComparator *comparator)
{
    LksOrderedTree *ordered;
    if (comparator == NULL || comparator->compare == NULL) return NULL;
    ordered = (LksOrderedTree *)lks_alloc_tagged(sizeof(*ordered),
        LKS_ALLOC_TAG_TREE_OBJECT);
    if (ordered == NULL) return NULL;
    ordered->index = lks_tree_create();
    if (ordered->index == NULL) { lks_free(ordered); return NULL; }
    ordered->comparator = *comparator;
    ordered->append_run = 0;
    ordered->prepend_run = 0;
    return ordered;
}

void lks_ordered_tree_destroy(LksOrderedTree *tree)
{
    if (tree == NULL) return;
    lks_tree_destroy(tree->index);
    lks_free(tree);
}

size_t lks_ordered_tree_size(const LksOrderedTree *tree)
{ return tree == NULL ? 0 : lks_tree_size(tree->index); }

const LksTree *lks_ordered_tree_internal_index(const LksOrderedTree *tree)
{ return tree == NULL ? NULL : tree->index; }

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
LksStatus lks_ordered_tree_test_seed_path(LksOrderedTree *tree,
    const LksPath *path, void *item)
{
    LksStatus status = lks_tree_insert(tree == NULL ? NULL : tree->index,
        path, item, NULL);
    if (status == LKS_STATUS_OK) {
        tree->append_run = 0;
        tree->prepend_run = 0;
    }
    return status;
}
#endif

LksStatus lks_ordered_tree_find_path(const LksOrderedTree *tree,
    const LksPath *path, const LksTreeNode **out_node)
{ return lks_tree_find_path(tree == NULL ? NULL : tree->index, path, out_node); }

LksStatus lks_ordered_tree_remove_path(LksOrderedTree *tree,
    const LksPath *path, void **out_item)
{
    LksStatus status = lks_tree_remove_path(tree == NULL ? NULL : tree->index,
        path, out_item);
    if (status == LKS_STATUS_OK) {
        tree->append_run = 0;
        tree->prepend_run = 0;
    }
    return status;
}

size_t lks_ordered_tree_root_child_count(const LksOrderedTree *tree)
{ return lks_tree_root_child_count(tree == NULL ? NULL : tree->index); }

const LksTreeNode *lks_ordered_tree_root_child_at(
    const LksOrderedTree *tree, size_t index)
{ return lks_tree_root_child_at(tree == NULL ? NULL : tree->index, index); }

/* Prepare COUNT keys in (LEFT, RIGHT) by splitting logical rank intervals.
 * Each returned key is validated by the gap API. Recursion depth is log COUNT. */
static LksStatus generate_range(LksPath **paths, size_t first, size_t count,
    const LksPath *left, const LksPath *right)
{
    size_t middle;
    LksStatus status;
    if (count == 0) return LKS_STATUS_OK;
    middle = count / 2;
    if (left != NULL && right != NULL)
        status = lks_path_between(left, right, &paths[first + middle]);
    else if (left != NULL)
        status = lks_path_after(left, &paths[first + middle]);
    else if (right != NULL)
        status = lks_path_before(right, &paths[first + middle]);
    else {
        paths[first + middle] = lks_path_create_zero();
        status = paths[first + middle] == NULL ?
            LKS_STATUS_OUT_OF_MEMORY : LKS_STATUS_OK;
    }
    if (status != LKS_STATUS_OK) return status;
    status = generate_range(paths, first, middle, left, paths[first + middle]);
    if (status != LKS_STATUS_OK) return status;
    return generate_range(paths, first + middle + 1, count - middle - 1,
        paths[first + middle], right);
}

static void destroy_path_array(LksPath **paths, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) lks_path_destroy(paths[i]);
}

/* At an open end there is no exterior Path to collide with. Carrying a full
 * slot into an ancestor keeps a long append run shallow; a small stride
 * reserves interior gaps while using most of the 16-bit endpoint capacity.
 * This policy is private to comparator-managed insertion. */
static LksStatus ordered_after(const LksPath *left, unsigned int step,
    LksPath **out_path)
{
    LksDirection direction = lks_path_direction(left);
    size_t depth = lks_path_depth(left), index, j;
    LksPath *path;
    LksStatus status;
    int order;
    if (direction == LKS_DIRECTION_ZERO)
        return lks_path_after(left, out_path);
    for (index = depth; index != 0; --index) {
        unsigned int slot;
        size_t level;
        if (lks_path_get_slot(left, index - 1, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(left, index - 1, &level) != LKS_STATUS_OK)
            return LKS_STATUS_INTERNAL_ERROR;
        if (index == 1 && direction == LKS_DIRECTION_NEGATIVE) {
            if (slot == 0) continue;
            slot -= slot < step ? slot : step;
        } else {
            if (slot == LKS_PATH_SLOT_MAX) continue;
            slot += LKS_PATH_SLOT_MAX - slot < step ?
                LKS_PATH_SLOT_MAX - slot : step;
        }
        if (index == 1) path = lks_path_create_at_level(direction, slot, level);
        else {
            unsigned int root_slot;
            size_t root_level;
            if (lks_path_get_slot(left, 0, &root_slot) != LKS_STATUS_OK ||
                lks_path_get_level(left, 0, &root_level) != LKS_STATUS_OK)
                return LKS_STATUS_INTERNAL_ERROR;
            path = lks_path_create_at_level(direction, root_slot, root_level);
            if (path != NULL) {
                for (j = 1; j < index; ++j) {
                    unsigned int next_slot = slot;
                    size_t next_level = level;
                    if (j != index - 1 &&
                        (lks_path_get_slot(left, j, &next_slot) != LKS_STATUS_OK ||
                         lks_path_get_level(left, j, &next_level) != LKS_STATUS_OK)) {
                        lks_path_destroy(path);
                        return LKS_STATUS_INTERNAL_ERROR;
                    }
                    status = lks_path_append_at_level(path, next_slot, next_level);
                    if (status != LKS_STATUS_OK) {
                        lks_path_destroy(path);
                        return status;
                    }
                }
            }
        }
        if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        if (lks_path_compare(left, path, &order) != LKS_STATUS_OK || order >= 0) {
            lks_path_destroy(path);
            return LKS_STATUS_INTERNAL_ERROR;
        }
        *out_path = path;
        return LKS_STATUS_OK;
    }
    if (direction == LKS_DIRECTION_NEGATIVE) {
        path = lks_path_create_zero();
        if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        *out_path = path;
        return LKS_STATUS_OK;
    }
    return lks_path_after(left, out_path);
}

/* A new minimum can always use a fresh negative root coordinate. Negative
 * root slots run in reverse order; after slot 65535 the next level sorts
 * earlier. The public gap generator remains unchanged. */
static LksStatus ordered_before(const LksPath *right, unsigned int step,
    LksPath **out_path)
{
    LksDirection direction = lks_path_direction(right);
    unsigned int slot = 0;
    size_t level = 0;
    LksPath *path;
    int order;
    if (direction == LKS_DIRECTION_NEGATIVE) {
        if (lks_path_get_slot(right, 0, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(right, 0, &level) != LKS_STATUS_OK)
            return LKS_STATUS_INTERNAL_ERROR;
        if (slot < LKS_PATH_SLOT_MAX)
            slot += LKS_PATH_SLOT_MAX - slot < step ?
                LKS_PATH_SLOT_MAX - slot : step;
        else {
            if (level == (size_t)-1) return LKS_STATUS_LEVEL_LIMIT;
            ++level;
            slot = 0;
        }
    }
    path = lks_path_create_at_level(LKS_DIRECTION_NEGATIVE, slot, level);
    if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    if (lks_path_compare(path, right, &order) != LKS_STATUS_OK || order >= 0) {
        lks_path_destroy(path);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = path;
    return LKS_STATUS_OK;
}

/* The affected nodes are a contiguous in-order range. Replacing their Paths
 * by a strictly increasing sequence inside unchanged exterior bounds keeps
 * every physical AVL edge ordered. The insertion gap determines a vacant
 * child link before any key is changed; commit therefore cannot fail. */
static LksStatus try_adaptive_relabel(LksTree *tree, LksTreeNode *before,
    LksTreeNode *after, void *item, size_t candidate_depth,
    const LksTreeNode **out_node, int *out_relabelled)
{
    size_t window = tree->size < LKS_POLICY_LOCAL_INITIAL_NODES ?
        tree->size : LKS_POLICY_LOCAL_INITIAL_NODES;
    LksTreeNode *parent;
    int order;
    int endpoint = before == NULL || after == NULL;
    *out_relabelled = 0;
    if (before != NULL && before->right == NULL) {
        parent = before; order = 1;
    } else if (after != NULL && after->left == NULL) {
        parent = after; order = -1;
    } else return LKS_STATUS_INTERNAL_ERROR;
    REPAIR_COUNT(attempts);
    if (endpoint) REPAIR_COUNT(endpoint_attempts);
    else REPAIR_COUNT(interior_attempts);
    for (;;) {
        LksTreeNode *local_nodes[LKS_POLICY_STACK_REGION_NODES];
        LksPath *local_paths[LKS_POLICY_STACK_REGION_NODES + 1];
        LksTreeNode **nodes = local_nodes;
        LksPath **paths = local_paths;
        int heap_scratch = window > LKS_POLICY_STACK_REGION_NODES;
        LksTreeNode *left = before, *right = after, *start = NULL;
        LksTreeNode *cursor, *new_node;
        const LksPath *outer_left, *outer_right;
        size_t count = 0, position = 0, i, max_depth = 0;
        LksStatus status;
        int take_left = 1;
        if (heap_scratch) {
            if (window > (size_t)-1 / sizeof(*nodes) ||
                window == (size_t)-1 ||
                window + 1 > (size_t)-1 / sizeof(*paths))
                return LKS_STATUS_OUT_OF_MEMORY;
            nodes = (LksTreeNode **)lks_alloc_tagged(window * sizeof(*nodes),
                LKS_ALLOC_TAG_MERGE_SCRATCH);
            if (nodes == NULL) return LKS_STATUS_OUT_OF_MEMORY;
            paths = (LksPath **)lks_alloc_tagged((window + 1) * sizeof(*paths),
                LKS_ALLOC_TAG_MERGE_SCRATCH);
            if (paths == NULL) {
                lks_free(nodes); return LKS_STATUS_OUT_OF_MEMORY;
            }
        }
        for (i = 0; i <= window; ++i) paths[i] = NULL;
        /* Alternate around the insertion gap, then use whichever side remains.
         * At window == size this necessarily includes every existing node. */
        while (count < window && (left != NULL || right != NULL)) {
            if (left != NULL && (take_left || right == NULL)) {
                start = left;
                left = predecessor(left);
            } else {
                if (start == NULL) start = right;
                right = successor(right);
            }
            ++count;
            take_left = !take_left;
        }
        if (count != window || start == NULL) {
            if (heap_scratch) { lks_free(paths); lks_free(nodes); }
            return LKS_STATUS_INTERNAL_ERROR;
        }
        REPAIR_ADD(attempted_region_nodes, count);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
        if (count > repair_stats.max_attempted_region)
            repair_stats.max_attempted_region = count;
#endif
        cursor = start;
        for (i = 0; i < count; ++i) {
            nodes[i] = cursor;
            if (cursor == after) position = i;
            cursor = successor(cursor);
        }
        if (after == NULL) position = count;
        if ((after != NULL && (position == count || nodes[position] != after)) ||
            (before != NULL && (position == 0 ||
                nodes[position - 1] != before))) {
            if (heap_scratch) { lks_free(paths); lks_free(nodes); }
            return LKS_STATUS_INTERNAL_ERROR;
        }
        outer_left = predecessor(start) == NULL ? NULL : predecessor(start)->path;
        outer_right = cursor == NULL ? NULL : cursor->path;
        /* With no exterior bounds, the sparse bulk coordinate generator is
         * faster and shallower than recursively splitting every gap. It
         * prepares Paths only: existing physical nodes stay in place. */
        status = window == tree->size ?
            lks_bulk_generate_paths(paths, count + 1) :
            generate_range(paths, 0, count + 1, outer_left, outer_right);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
        for (i = 0; i <= count; ++i)
            if (paths[i] != NULL) REPAIR_ADD(generated_relabel_paths, 1);
#endif
        if (status == LKS_STATUS_OK) {
            for (i = 0; i <= count; ++i) {
                int cmp;
                size_t depth;
                if (paths[i] == NULL ||
                    (i != 0 && (lks_path_compare(paths[i - 1], paths[i], &cmp)
                        != LKS_STATUS_OK || cmp >= 0)) ||
                    (i == 0 && outer_left != NULL &&
                     (lks_path_compare(outer_left, paths[i], &cmp)
                        != LKS_STATUS_OK || cmp >= 0)) ||
                    (i == count && outer_right != NULL &&
                     (lks_path_compare(paths[i], outer_right, &cmp)
                        != LKS_STATUS_OK || cmp >= 0))) {
                    status = LKS_STATUS_INTERNAL_ERROR;
                    break;
                }
                depth = lks_path_depth(paths[i]);
                if (depth > max_depth) max_depth = depth;
            }
        }
        if (status == LKS_STATUS_OK && max_depth < candidate_depth) {
            new_node = (LksTreeNode *)lks_alloc_tagged(sizeof(*new_node),
                LKS_ALLOC_TAG_TREE_NODE);
            if (new_node == NULL) status = LKS_STATUS_OUT_OF_MEMORY;
        } else new_node = NULL;
        if (new_node != NULL) {
            /* No allocation or recoverable error occurs from here onward. */
            for (i = 0; i < count; ++i) {
                LksPath *old = nodes[i]->path;
                nodes[i]->path = paths[i < position ? i : i + 1];
                lks_path_destroy(old);
            }
            new_node->path = paths[position]; new_node->item = item;
            new_node->left = NULL; new_node->right = NULL;
            new_node->parent = NULL; new_node->height = 1;
            link_prepared(tree, new_node, parent, order);
            REPAIR_COUNT(successes);
            if (endpoint) REPAIR_COUNT(endpoint_successes);
            else REPAIR_COUNT(interior_successes);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            repair_stats.nodes_relabelled += count;
            if (count > repair_stats.max_region_nodes)
                repair_stats.max_region_nodes = count;
            if (count == tree->size - 1) {
                ++repair_stats.full_range_relabels;
                repair_stats.full_range_relabelled_nodes += count;
            }
#endif
            if (out_node != NULL) *out_node = new_node;
            *out_relabelled = 1;
            if (heap_scratch) { lks_free(paths); lks_free(nodes); }
            return LKS_STATUS_OK;
        }
        destroy_path_array(paths, count + 1);
        if (heap_scratch) { lks_free(paths); lks_free(nodes); }
        if (status != LKS_STATUS_OK && status != LKS_STATUS_LEVEL_LIMIT)
            return status;
        if (window == tree->size) return LKS_STATUS_OK;
        window = window > tree->size / 2 ? tree->size : window * 2;
        REPAIR_COUNT(region_expansions);
    }
}

/* Only a successful mutation updates the placement hint. Remove resets it,
 * so a failed insertion or changed endpoint never predicts a false run. */
static void record_ordered_position(LksOrderedTree *ordered, int position)
{
    if (ordered == NULL) return;
    if (position > 0) {
        if (ordered->append_run != (size_t)-1) ++ordered->append_run;
        ordered->prepend_run = 0;
    } else if (position < 0) {
        if (ordered->prepend_run != (size_t)-1) ++ordered->prepend_run;
        ordered->append_run = 0;
    } else {
        ordered->append_run = 0;
        ordered->prepend_run = 0;
    }
}

static LksStatus insert_by_comparator(LksTree *tree, void *item,
    const LksComparator *comparator, const LksTreeNode **out_node,
    LksOrderedTree *ordered)
{
    LksTreeNode *before = NULL, *after = NULL;
    LksPath *candidate = NULL;
    LksStatus status = LKS_STATUS_OK;
    size_t depth = (size_t)-1;
    unsigned int end_step = LKS_POLICY_TARGET_SPACING;
    int position = 0;
    int relabelled = 0;
    if (out_node != NULL) *out_node = NULL;
    if (tree == NULL || comparator == NULL || comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    if (tree->size == (size_t)-1) return LKS_STATUS_OUT_OF_MEMORY;
    if (tree->size == 0) {
        candidate = lks_path_create_zero();
        if (candidate == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    } else {
        item_upper_bound(tree, item, comparator, &before, &after);
        position = before == NULL ? -1 : after == NULL ? 1 : 0;
        if (ordered != NULL &&
            (position > 0 ? ordered->append_run : ordered->prepend_run) >=
                LKS_POLICY_ORDERED_BURST_THRESHOLD)
            end_step = LKS_POLICY_ORDERED_BURST_STEP;
        if (before != NULL && after != NULL)
            status = lks_path_between(before->path, after->path, &candidate);
        else if (before != NULL)
            status = ordered_after(before->path, end_step, &candidate);
        else status = ordered_before(after->path, end_step, &candidate);
        if (status != LKS_STATUS_OK && status != LKS_STATUS_LEVEL_LIMIT)
            return status;
    }
    if (candidate != NULL) depth = lks_path_depth(candidate);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
    if (candidate != NULL) {
        REPAIR_ADD(candidate_depth_sum, depth);
        if (depth > repair_stats.candidate_depth_max)
            repair_stats.candidate_depth_max = depth;
    }
#endif
    if (tree->size != 0 && (candidate == NULL ||
        (depth > LKS_POLICY_PREFERRED_ONLINE_DEPTH &&
         ((before != NULL && after != NULL) ||
          depth > LKS_POLICY_OPEN_END_DIRECT_DEPTH)))) {
        status = try_adaptive_relabel(tree, before, after, item, depth,
            out_node, &relabelled);
        if (status != LKS_STATUS_OK || relabelled) {
            lks_path_destroy(candidate);
            if (status == LKS_STATUS_OK && relabelled)
                record_ordered_position(ordered, position);
            return status;
        }
    }
    if (candidate == NULL) return LKS_STATUS_LEVEL_LIMIT;
    status = insert_owned_path(tree, candidate, item, out_node);
    if (status != LKS_STATUS_OK) lks_path_destroy(candidate);
    if (status == LKS_STATUS_OK) {
        record_ordered_position(ordered, position);
        REPAIR_COUNT(direct_inserts);
        if (position == 0) REPAIR_COUNT(direct_interior_inserts);
        else {
            REPAIR_COUNT(direct_endpoint_inserts);
            if (end_step == LKS_POLICY_ORDERED_BURST_STEP)
                REPAIR_COUNT(burst_endpoint_inserts);
        }
        if (depth > LKS_POLICY_PREFERRED_ONLINE_DEPTH)
            REPAIR_COUNT(deeper_accepts);
    }
    return status;
}

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
LksStatus lks_tree_internal_insert_item(LksTree *tree, void *item,
    const LksComparator *comparator, const LksTreeNode **out_node)
{ return insert_by_comparator(tree, item, comparator, out_node, NULL); }
#endif

LksStatus lks_ordered_tree_insert(LksOrderedTree *tree, void *item,
    const LksTreeNode **out_node)
{
    return insert_by_comparator(tree == NULL ? NULL : tree->index,
        item, tree == NULL ? NULL : &tree->comparator, out_node, tree);
}

const LksPath *lks_tree_node_path(const LksTreeNode *node)
{ return node == NULL ? NULL : node->path; }
void *lks_tree_node_item(const LksTreeNode *node)
{ return node == NULL ? NULL : node->item; }
const LksTreeNode *lks_tree_node_parent(const LksTreeNode *node)
{ return node == NULL ? NULL : node->parent; }
size_t lks_tree_node_child_count(const LksTreeNode *node)
{ return node == NULL ? 0 : (node->left != NULL) + (node->right != NULL); }
const LksTreeNode *lks_tree_node_child_at(const LksTreeNode *node, size_t index)
{
    if (node == NULL) return NULL;
    if (node->left != NULL) {
        if (index == 0) return node->left;
        if (index == 1) return node->right;
    } else if (index == 0) return node->right;
    return NULL;
}
size_t lks_tree_root_child_count(const LksTree *tree)
{ return tree != NULL && tree->root != NULL ? 1u : 0u; }
const LksTreeNode *lks_tree_root_child_at(const LksTree *tree, size_t index)
{ return tree == NULL || index != 0 ? NULL : tree->root; }

static LksTreeNode *build_balanced(LksTreeNode **nodes, size_t count,
    LksTreeNode *parent)
{
    size_t middle;
    LksTreeNode *node;
    if (count == 0) return NULL;
    middle = count / 2;
    node = nodes[middle];
    node->parent = parent;
    node->left = build_balanced(nodes, middle, node);
    node->right = build_balanced(nodes + middle + 1, count - middle - 1, node);
    update_height(node);
    return node;
}

LksStatus lks_tree_internal_build_ordered(LksPath **paths,
    void *const *items, size_t count, LksTree **out_tree)
{
    LksTree *tree;
    LksTreeNode **nodes = NULL;
    size_t i;
    if (out_tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_tree = NULL;
    if (count != 0 && (paths == NULL || items == NULL))
        return LKS_STATUS_INVALID_ARGUMENT;
    if (count > (size_t)-1 / sizeof(*nodes)) return LKS_STATUS_OUT_OF_MEMORY;
    for (i = 0; i < count; ++i) {
        int order;
        if (paths[i] == NULL || (i != 0 &&
            (lks_path_compare(paths[i - 1], paths[i], &order) != LKS_STATUS_OK ||
             order >= 0))) return LKS_STATUS_INTERNAL_ERROR;
    }
    tree = lks_tree_create();
    if (tree == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    if (count != 0) {
        nodes = (LksTreeNode **)lks_alloc_tagged(count * sizeof(*nodes),
            LKS_ALLOC_TAG_MERGE_SCRATCH);
        if (nodes == NULL) { lks_tree_destroy(tree); return LKS_STATUS_OUT_OF_MEMORY; }
        for (i = 0; i < count; ++i) {
            nodes[i] = (LksTreeNode *)lks_alloc_tagged(sizeof(*nodes[i]),
                LKS_ALLOC_TAG_TREE_NODE);
            if (nodes[i] == NULL) {
                while (i != 0) lks_free(nodes[--i]);
                lks_free(nodes);
                lks_tree_destroy(tree);
                return LKS_STATUS_OUT_OF_MEMORY;
            }
            nodes[i]->path = paths[i];
            nodes[i]->item = items[i];
            nodes[i]->left = NULL; nodes[i]->right = NULL;
            nodes[i]->parent = NULL; nodes[i]->height = 1;
        }
        tree->root = build_balanced(nodes, count, NULL);
        tree->size = count;
        lks_free(nodes);
    }
    *out_tree = tree;
    return LKS_STATUS_OK;
}

LksStatus lks_tree_internal_fill_ordered(const LksTree *tree,
    const LksTreeNode **nodes, size_t capacity)
{
    LksTreeNode *cursor;
    size_t i = 0;
    if (tree == NULL || (capacity != 0 && nodes == NULL) || capacity != tree->size)
        return LKS_STATUS_INVALID_ARGUMENT;
    for (cursor = first_node(tree->root); cursor != NULL; cursor = successor(cursor))
        nodes[i++] = cursor;
    return i == capacity ? LKS_STATUS_OK : LKS_STATUS_INTERNAL_ERROR;
}

static LksStatus profile_node(const LksTreeNode *node,
    const LksTreeNode *parent, const LksPath *lower, const LksPath *upper,
    LksTreeInternalProfile *profile, int *out_height)
{
    int left_height, right_height, order;
    size_t degree;
    LksStatus status;
    if (node == NULL) { *out_height = 0; return LKS_STATUS_OK; }
    if (node->parent != parent || node->path == NULL) return LKS_STATUS_INTERNAL_ERROR;
    if (lower != NULL &&
        (lks_path_compare(lower, node->path, &order) != LKS_STATUS_OK ||
         order >= 0)) return LKS_STATUS_INTERNAL_ERROR;
    if (upper != NULL &&
        (lks_path_compare(node->path, upper, &order) != LKS_STATUS_OK ||
         order >= 0)) return LKS_STATUS_INTERNAL_ERROR;
    status = profile_node(node->left, node, lower, node->path, profile, &left_height);
    if (status != LKS_STATUS_OK) return status;
    status = profile_node(node->right, node, node->path, upper, profile, &right_height);
    if (status != LKS_STATUS_OK) return status;
    if (left_height - right_height > 1 || right_height - left_height > 1 ||
        node->height != 1 + (left_height > right_height ? left_height : right_height))
        return LKS_STATUS_INTERNAL_ERROR;
    *out_height = node->height;
    if (profile->real_node_count == (size_t)-1) return LKS_STATUS_INTERNAL_ERROR;
    ++profile->real_node_count;
    degree = (node->left != NULL) + (node->right != NULL);
    if (degree == 0) ++profile->leaf_count;
    else if (degree == 1) ++profile->unary_count;
    else ++profile->branching_count;
    if (lks_path_depth(node->path) > profile->max_path_depth)
        profile->max_path_depth = lks_path_depth(node->path);
    return LKS_STATUS_OK;
}

LksStatus lks_tree_internal_profile(const LksTree *tree,
    LksTreeInternalProfile *out_profile)
{
    LksStatus status;
    int root_height;
    if (out_profile == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    memset(out_profile, 0, sizeof(*out_profile));
    if (tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    status = profile_node(tree->root, NULL, NULL, NULL, out_profile, &root_height);
    if (status != LKS_STATUS_OK || out_profile->real_node_count != tree->size)
        return LKS_STATUS_INTERNAL_ERROR;
    out_profile->avl_height = (size_t)root_height;
    out_profile->balance_valid = 1;
    return LKS_STATUS_OK;
}

size_t lks_tree_internal_sizeof_tree(void) { return sizeof(LksTree); }
size_t lks_tree_internal_alignof_tree(void) { return _Alignof(LksTree); }
size_t lks_tree_internal_sizeof_node(void) { return sizeof(LksTreeNode); }
size_t lks_tree_internal_alignof_node(void) { return _Alignof(LksTreeNode); }
