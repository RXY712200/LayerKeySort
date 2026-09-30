#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include "lks_bulk_internal.h"
#include "lks_tree_internal.h"
#include "lks_policy_internal.h"

/* Generate the Preview.3 sparse Path sequence independently of Tree shape.
 * Its recursive blocks are logical Path subdivisions, never physical nodes. */
static LksStatus generate_coordinate_blocks(LksPath **paths, size_t count,
    const LksPath *parent, size_t level)
{
    size_t blocks, common, extra, block, offset = 0;
    if (count == 0) return LKS_STATUS_OK;
    blocks = count < LKS_POLICY_BULK_BLOCK_LIMIT ? count :
        LKS_POLICY_BULK_BLOCK_LIMIT;
    common = count / blocks;
    extra = count % blocks;
    for (block = 0; block < blocks; ++block) {
        size_t block_size = common + (block < extra ? 1u : 0u);
        unsigned int slot = lks_policy_bulk_slot(block, blocks);
        LksStatus status;
        if (parent == NULL)
            paths[offset] = lks_path_create_at_level(
                LKS_DIRECTION_POSITIVE, slot, level);
        else {
            paths[offset] = lks_path_clone(parent);
            if (paths[offset] != NULL) {
                status = lks_path_append_at_level(paths[offset], slot, level);
                if (status != LKS_STATUS_OK) return status;
            }
        }
        if (paths[offset] == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        if (block_size > 1) {
            if (level == (size_t)-1) return LKS_STATUS_LEVEL_LIMIT;
            status = generate_coordinate_blocks(paths + offset + 1, block_size - 1,
                paths[offset], level + 1);
            if (status != LKS_STATUS_OK) return status;
        }
        offset += block_size;
    }
    return LKS_STATUS_OK;
}

LksStatus lks_bulk_build_tree(void *const *sorted, size_t count,
    LksTree **out_tree)
{
    LksPath **paths = NULL;
    LksStatus status;
    size_t i;
    if (out_tree == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_tree = NULL;
    if (count != 0 && sorted == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    if (count > (size_t)-1 / sizeof(*paths)) return LKS_STATUS_OUT_OF_MEMORY;
    if (count != 0) {
        paths = (LksPath **)lks_alloc_tagged(count * sizeof(*paths),
            LKS_ALLOC_TAG_MERGE_SCRATCH);
        if (paths == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        for (i = 0; i < count; ++i) paths[i] = NULL;
        paths[0] = lks_path_create_zero();
        status = paths[0] == NULL ? LKS_STATUS_OUT_OF_MEMORY :
            generate_coordinate_blocks(paths + 1, count - 1, NULL, 0);
        if (status != LKS_STATUS_OK) goto cleanup;
    }
    status = lks_tree_internal_build_ordered(paths, sorted, count, out_tree);
    if (status == LKS_STATUS_OK) {
        lks_free(paths);
        return status;
    }
cleanup:
    for (i = 0; i < count; ++i) lks_path_destroy(paths[i]);
    lks_free(paths);
    return status;
}
