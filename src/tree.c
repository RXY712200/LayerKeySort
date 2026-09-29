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

#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
static LksTreeRepairStats repair_stats;
#define REPAIR_COUNT(member) (++repair_stats.member)
#else
#define REPAIR_COUNT(member) ((void)0)
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

LksStatus lks_tree_locate_item(const LksTree *tree, const void *item,
    const LksComparator *comparator, const LksTreeNode **out_left,
    const LksTreeNode **out_equal, const LksTreeNode **out_right)
{
    LksTreeNode *before, *equal, *after;
    if (tree == NULL || comparator == NULL || comparator->compare == NULL ||
        out_left == NULL || out_equal == NULL || out_right == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    *out_left = NULL; *out_equal = NULL; *out_right = NULL;
    item_bounds(tree, item, comparator, &before, &equal, &after);
    if (equal != NULL) *out_equal = equal;
    else { *out_left = before; *out_right = after; }
    return LKS_STATUS_OK;
}

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

/* Existing nodes stay in exactly the same in-order ranks. Replacing their
 * keys with a strictly increasing sequence inside the unchanged exterior
 * bounds therefore preserves every BST relation in the physical AVL shape.
 * Only the newly linked node can require rotations. */
static LksStatus try_range_repair(LksTree *tree, LksTreeNode *before,
    LksTreeNode *after, void *item, size_t candidate_depth,
    const LksTreeNode **out_node, int *out_repaired)
{
    size_t window = LKS_POLICY_LOCAL_INITIAL_NODES;
    *out_repaired = 0;
    REPAIR_COUNT(attempts);
    while (window <= LKS_POLICY_LOCAL_MAX_NODES) {
        LksTreeNode *nodes[LKS_POLICY_LOCAL_MAX_NODES];
        LksPath *paths[LKS_POLICY_LOCAL_MAX_NODES + 1];
        LksPath *old_paths[LKS_POLICY_LOCAL_MAX_NODES];
        LksTreeNode *start = before != NULL ? before : after;
        LksTreeNode *cursor, *parent, *found, *new_node;
        const LksPath *outer_left, *outer_right;
        size_t count = 0, position = 0, i, left_steps;
        int order;
        LksStatus status;
        if (start == NULL) return LKS_STATUS_INTERNAL_ERROR;
        left_steps = window / 2;
        while (left_steps-- > 0 && predecessor(start) != NULL)
            start = predecessor(start);
        cursor = start;
        while (cursor != NULL && count < window) {
            nodes[count++] = cursor;
            cursor = successor(cursor);
        }
        if (before != NULL) {
            while (position < count && nodes[position] != before) ++position;
            if (position == count) return LKS_STATUS_INTERNAL_ERROR;
            ++position;
        }
        outer_left = predecessor(start) == NULL ? NULL : predecessor(start)->path;
        outer_right = cursor == NULL ? NULL : cursor->path;
        for (i = 0; i <= count; ++i) paths[i] = NULL;
        status = generate_range(paths, 0, count + 1, outer_left, outer_right);
        if (status == LKS_STATUS_OK) {
            size_t max_depth = 0;
            for (i = 0; i <= count; ++i) {
                int comparison;
                size_t depth;
                if (paths[i] == NULL ||
                    (i != 0 &&
                     (lks_path_compare(paths[i - 1], paths[i], &comparison) !=
                          LKS_STATUS_OK || comparison >= 0)) ||
                    (i == 0 && outer_left != NULL &&
                     (lks_path_compare(outer_left, paths[i], &comparison) !=
                          LKS_STATUS_OK || comparison >= 0)) ||
                    (i == count && outer_right != NULL &&
                     (lks_path_compare(paths[i], outer_right, &comparison) !=
                          LKS_STATUS_OK || comparison >= 0))) {
                    status = LKS_STATUS_INTERNAL_ERROR;
                    break;
                }
                depth = lks_path_depth(paths[i]);
                if (depth > max_depth) max_depth = depth;
            }
            if (status == LKS_STATUS_OK &&
                (max_depth > LKS_POLICY_HARD_ONLINE_DEPTH ||
                (candidate_depth < max_depth + LKS_POLICY_LOCAL_MIN_DEPTH_GAIN &&
                 candidate_depth <= LKS_POLICY_HARD_ONLINE_DEPTH)))
                status = LKS_STATUS_LEVEL_LIMIT;
        }
        if (status == LKS_STATUS_OK) {
            new_node = (LksTreeNode *)lks_alloc_tagged(sizeof(*new_node),
                LKS_ALLOC_TAG_TREE_NODE);
            if (new_node == NULL) status = LKS_STATUS_OUT_OF_MEMORY;
        } else new_node = NULL;
        if (status == LKS_STATUS_OK) {
            for (i = 0; i < count; ++i) {
                old_paths[i] = nodes[i]->path;
                nodes[i]->path = paths[i < position ? i : i + 1];
            }
            new_node->path = paths[position]; new_node->item = item;
            new_node->left = NULL; new_node->right = NULL;
            new_node->parent = NULL; new_node->height = 1;
            /* All key and boundary checks completed before this commit.
             * Searching cannot fail for the prepared valid Path sequence. */
            status = search_path(tree, new_node->path, &found, &parent, &order);
            if (status != LKS_STATUS_OK || found != NULL) {
                /* An internal invariant violation is not a recoverable
                 * allocation failure; restore the original keys. */
                for (i = 0; i < count; ++i) nodes[i]->path = old_paths[i];
                lks_free(new_node);
                destroy_path_array(paths, count + 1);
                return LKS_STATUS_INTERNAL_ERROR;
            }
            link_prepared(tree, new_node, parent, order);
            destroy_path_array(old_paths, count);
            REPAIR_COUNT(successes);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
            repair_stats.nodes_relabelled += count;
            if (count > repair_stats.max_region_nodes)
                repair_stats.max_region_nodes = count;
#endif
            if (out_node != NULL) *out_node = new_node;
            *out_repaired = 1;
            return LKS_STATUS_OK;
        }
        destroy_path_array(paths, count + 1);
        if (status != LKS_STATUS_LEVEL_LIMIT) return status;
        if (window == LKS_POLICY_LOCAL_MAX_NODES) break;
        window *= LKS_POLICY_LOCAL_EXPANSION_FACTOR;
        if (window > LKS_POLICY_LOCAL_MAX_NODES)
            window = LKS_POLICY_LOCAL_MAX_NODES;
        REPAIR_COUNT(region_expansions);
    }
    REPAIR_COUNT(fallbacks);
    return LKS_STATUS_OK;
}

static LksStatus rebuild_with_item(LksTree *tree, void *item,
    LksTreeNode *after, const LksTreeNode **out_node)
{
    void **items;
    LksTree *replacement;
    LksTreeNode *cursor;
    LksTreeInternalProfile profile;
    size_t i, position = 0;
    LksStatus status;
    if (tree->size == (size_t)-1 ||
        tree->size + 1 > (size_t)-1 / sizeof(*items))
        return LKS_STATUS_OUT_OF_MEMORY;
    items = (void **)lks_alloc_tagged((tree->size + 1) * sizeof(*items),
        LKS_ALLOC_TAG_MERGE_SCRATCH);
    if (items == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    cursor = first_node(tree->root);
    for (i = 0; i < tree->size; ++i) {
        if (cursor == after) position = i;
        items[i] = cursor->item;
        cursor = successor(cursor);
    }
    if (after == NULL) position = tree->size;
    memmove(items + position + 1, items + position,
        (tree->size - position) * sizeof(*items));
    items[position] = item;
    status = lks_bulk_build_tree(items, tree->size + 1, &replacement);
    lks_free(items);
    if (status != LKS_STATUS_OK) return status;
    if (lks_tree_internal_profile(replacement, &profile) != LKS_STATUS_OK ||
        profile.real_node_count != tree->size + 1 || !profile.balance_valid) {
        lks_tree_destroy(replacement);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    cursor = first_node(replacement->root);
    for (i = 0; i < position; ++i) cursor = successor(cursor);
    if (cursor == NULL) {
        lks_tree_destroy(replacement);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    /* Swap independently prepared representations, then destroy the old one. */
    { LksTreeNode *old_root = tree->root; size_t old_size = tree->size;
      tree->root = replacement->root; tree->size = replacement->size;
      replacement->root = old_root; replacement->size = old_size; }
    lks_tree_destroy(replacement);
    REPAIR_COUNT(full_rebuilds);
    if (out_node != NULL) *out_node = cursor;
    return LKS_STATUS_OK;
}

LksStatus lks_tree_insert_item(LksTree *tree, void *item,
    const LksComparator *comparator, const LksTreeNode **out_node)
{
    LksTreeNode *before = NULL, *after = NULL;
    LksPath *candidate = NULL;
    LksStatus status;
    size_t depth;
    int repaired = 0;
    if (out_node != NULL) *out_node = NULL;
    if (tree == NULL || comparator == NULL || comparator->compare == NULL)
        return LKS_STATUS_INVALID_ARGUMENT;
    if (tree->size == 0) {
        candidate = lks_path_create_zero();
        if (candidate == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    } else {
        item_upper_bound(tree, item, comparator, &before, &after);
        if (before != NULL && after != NULL)
            status = lks_path_between(before->path, after->path, &candidate);
        else if (before != NULL) status = lks_path_after(before->path, &candidate);
        else status = lks_path_before(after->path, &candidate);
        if (status == LKS_STATUS_LEVEL_LIMIT)
            return rebuild_with_item(tree, item, after, out_node);
        if (status != LKS_STATUS_OK) return status;
    }
    depth = lks_path_depth(candidate);
    if (depth <= LKS_POLICY_PREFERRED_ONLINE_DEPTH) {
        status = lks_tree_insert(tree, candidate, item, out_node);
        lks_path_destroy(candidate);
        return status;
    }
    if (tree->size != 0) {
        status = try_range_repair(tree, before, after, item, depth,
            out_node, &repaired);
        if (status != LKS_STATUS_OK || repaired) {
            lks_path_destroy(candidate);
            return status;
        }
    }
    if (depth > LKS_POLICY_HARD_ONLINE_DEPTH) {
        lks_path_destroy(candidate);
        return rebuild_with_item(tree, item, after, out_node);
    }
    status = lks_tree_insert(tree, candidate, item, out_node);
    lks_path_destroy(candidate);
    if (status == LKS_STATUS_OK) REPAIR_COUNT(deeper_accepts);
    return status;
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
