#include "layerkeysort.h"
#include "lks_bulk_internal.h"
#include "lks_policy_internal.h"

static LksStatus bulk_subtree(LksTree *tree, void *const *items, size_t count,
    const LksPath *parent, size_t level)
{
    size_t blocks, common_size, extra, block, offset = 0;
    if (count == 0) return LKS_STATUS_OK;
    blocks = count < LKS_POLICY_BULK_CHILDREN ? count : LKS_POLICY_BULK_CHILDREN;
    /* Contiguous nearly equal blocks preserve sorted preorder: each block's
     * prefix node is visited before that block's recursive remainder. Sparse
     * canonical slots leave room for later online insertions. */
    common_size = count / blocks;
    extra = count % blocks;
    for (block = 0; block < blocks; ++block) {
        size_t block_size = common_size + (block < extra ? 1u : 0u);
        unsigned int slot = LKS_POLICY_BULK_FIRST_SLOT +
            (unsigned int)block * LKS_POLICY_BULK_STRIDE;
        LksPath *path;
        LksStatus status;
        if (parent == NULL) {
            path = lks_path_create_at_level(LKS_DIRECTION_POSITIVE, slot, level);
        } else {
            path = lks_path_clone(parent);
            if (path != NULL) {
                status = lks_path_append_at_level(path, slot, level);
                if (status != LKS_STATUS_OK) {
                    lks_path_destroy(path);
                    return status;
                }
            }
        }
        if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        /* A block's first item is its prefix node. Insert it before descendants. */
        status = lks_tree_insert(tree, path, items[offset], NULL);
        if (status == LKS_STATUS_OK && block_size > 1) {
            if (level == (size_t)-1) status = LKS_STATUS_LEVEL_LIMIT;
            else status = bulk_subtree(tree, items + offset + 1,
                block_size - 1, path, level + 1);
        }
        lks_path_destroy(path);
        if (status != LKS_STATUS_OK) return status;
        offset += block_size;
    }
    return LKS_STATUS_OK;
}

LksStatus lks_bulk_build_tree(void *const *sorted, size_t count,
    LksTree **out_tree)
{
    LksTree *tree;
    LksPath *zero;
    LksStatus status;
    if (out_tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_tree = NULL;
    if (count != 0 && sorted == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    tree = lks_tree_create();
    if (tree == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    if (count != 0) {
        zero = lks_path_create_zero();
        if (zero == NULL) { lks_tree_destroy(tree); return LKS_STATUS_OUT_OF_MEMORY; }
        status = lks_tree_insert(tree, zero, sorted[0], NULL);
        lks_path_destroy(zero);
        if (status == LKS_STATUS_OK)
            status = bulk_subtree(tree, sorted + 1, count - 1, NULL, 0);
        if (status != LKS_STATUS_OK) { lks_tree_destroy(tree); return status; }
    }
    *out_tree = tree;
    return LKS_STATUS_OK;
}

LksStatus lks_bulk_build_branch(const LksPath *anchor_path,
    void *const *sorted, size_t count, LksTree **out_tree)
{
    LksTree *tree;
    LksPath *prefix = NULL;
    size_t depth, step, level = 0;
    unsigned int slot;
    LksStatus status = LKS_STATUS_OK;
    if (out_tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_tree = NULL;
    if (anchor_path == NULL || sorted == NULL || count == 0 ||
        lks_path_direction(anchor_path) != LKS_DIRECTION_POSITIVE ||
        (depth = lks_path_depth(anchor_path)) == 0)
        return LKS_STATUS_INVALID_ARGUMENT;
    tree = lks_tree_create();
    if (tree == NULL) return LKS_STATUS_OUT_OF_MEMORY;
    for (step = 0; step < depth; ++step) {
        if (lks_path_get_slot(anchor_path, step, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(anchor_path, step, &level) != LKS_STATUS_OK) {
            status = LKS_STATUS_INTERNAL_ERROR;
            break;
        }
        if (step == 0) prefix = lks_path_create_at_level(
            LKS_DIRECTION_POSITIVE, slot, level);
        else status = lks_path_append_at_level(prefix, slot, level);
        if (prefix == NULL) status = LKS_STATUS_OUT_OF_MEMORY;
        if (status != LKS_STATUS_OK) break;
        status = lks_tree_insert(tree, prefix,
            step + 1 == depth ? sorted[0] : NULL, NULL);
        if (status != LKS_STATUS_OK) break;
    }
    if (status == LKS_STATUS_OK && count > 1) {
        if (level == (size_t)-1) status = LKS_STATUS_LEVEL_LIMIT;
        else status = bulk_subtree(tree, sorted + 1, count - 1,
            anchor_path, level + 1);
    }
    lks_path_destroy(prefix);
    if (status != LKS_STATUS_OK) { lks_tree_destroy(tree); return status; }
    *out_tree = tree;
    return LKS_STATUS_OK;
}
