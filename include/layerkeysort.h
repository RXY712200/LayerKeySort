#ifndef LAYERKEYSORT_H
#define LAYERKEYSORT_H

#include <stddef.h>

/* LayerKeySort public API. LKS_VERSION_* describes the current API generation. */
#ifdef __cplusplus
extern "C" {
#endif

#define LKS_VERSION_MAJOR 2
#define LKS_VERSION_MINOR 0
#define LKS_VERSION_PATCH 0
#define LKS_VERSION_PRERELEASE "preview.3"
#define LKS_VERSION_STRING "2.0.0-preview.3"

/* Public Path slot range. Slots are ordering coordinates, not durable IDs. */
#define LKS_PATH_SLOT_MIN 0u
#define LKS_PATH_SLOT_MAX 65535u

/* Operation results. Constructors returning pointers use NULL on failure. */
typedef enum LksStatus {
    LKS_STATUS_OK = 0,
    LKS_STATUS_INVALID_ARGUMENT,
    LKS_STATUS_OUT_OF_MEMORY,
    LKS_STATUS_BUFFER_TOO_SMALL,
    LKS_STATUS_LEVEL_LIMIT,
    LKS_STATUS_NOT_FOUND,
    LKS_STATUS_NOT_IMPLEMENTED,
    LKS_STATUS_INTERNAL_ERROR,
    LKS_STATUS_ALREADY_EXISTS
} LksStatus;

/* Generic item comparison: negative / zero / positive means before / equal /
 * after. LayerKeySort passes context through without interpreting it. Group and
 * merge operations are stable for comparator-equal items. */
typedef int (*LksCompareFn)(
    const void *left,
    const void *right,
    void *context
);

/* Comparator callback and caller-owned context used for one operation. */
typedef struct LksComparator {
    LksCompareFn compare;
    void *context;
} LksComparator;

/* Position direction around ZERO. */
typedef enum LksDirection {
    LKS_DIRECTION_ZERO = 0,
    LKS_DIRECTION_POSITIVE = 1,
    LKS_DIRECTION_NEGATIVE = -1
} LksDirection;

/* Opaque, re-encodable ordering coordinate, not a persistent item identity. */
typedef struct LksPath LksPath;

/* Return a static, library-owned description of a status value. */
const char *lks_status_string(LksStatus status);

/* Stable-sort the caller's pointer array in place. Items remain caller-owned.
 * Equal items retain input order. On allocation failure the array is unchanged.
 * COUNT may be zero with ITEMS == NULL; COMPARE must still be non-NULL.
 * COUNT below two needs no allocation. */
LksStatus lks_sort(void **items, size_t count, LksCompareFn compare,
    void *context);

/* Create a ZERO Path. Caller owns the returned Path and destroys it with
 * lks_path_destroy; returns NULL if allocation fails. */
LksPath *lks_path_create_zero(void);
/* Create a one-step Path at level zero. Caller owns the result; NULL indicates
 * an invalid direction/slot or allocation failure. */
LksPath *lks_path_create(
    LksDirection direction,
    unsigned int first_slot
);
/* Create a one-step Path at an explicit level. Same ownership and failure
 * behavior as lks_path_create. */
LksPath *lks_path_create_at_level(
    LksDirection direction,
    unsigned int first_slot,
    size_t level
);
/* Deep-copy a Path. Caller owns the independent copy; NULL means invalid input
 * or allocation failure. Source is unchanged. */
LksPath *lks_path_clone(
    const LksPath *source
);
/* Append a step at the next level. On failure the Path remains unchanged. */
LksStatus lks_path_append(
    LksPath *path,
    unsigned int slot
);
/* Append a step at a strictly greater explicit level. On failure the Path is
 * unchanged; may return INVALID_ARGUMENT, LEVEL_LIMIT, or OUT_OF_MEMORY. */
LksStatus lks_path_append_at_level(
    LksPath *path,
    unsigned int slot,
    size_t level
);
/* Read direction; NULL is treated as ZERO. */
LksDirection lks_path_direction(
    const LksPath *path
);
/* Read step count; NULL is treated as an empty Path. */
size_t lks_path_depth(
    const LksPath *path
);
/* Read a step slot into caller storage; invalid input/index returns
 * INVALID_ARGUMENT and leaves the output unchanged. */
LksStatus lks_path_get_slot(
    const LksPath *path,
    size_t index,
    unsigned int *out_slot
);
/* Read a step level into caller storage; invalid input/index returns
 * INVALID_ARGUMENT and leaves the output unchanged. */
LksStatus lks_path_get_level(
    const LksPath *path,
    size_t index,
    size_t *out_level
);
/* Destroy a caller-owned Path; NULL is accepted. */
void lks_path_destroy(
    LksPath *path
);
/* Return formatted character count excluding the terminator; zero means NULL
 * input or invalid internal Path state. */
size_t lks_path_text_length(
    const LksPath *path
);
/* Format into caller-owned storage without allocating. BUFFER_TOO_SMALL reports
 * insufficient capacity; the Path is never modified. Text is for display, not
 * persistent serialization or lexicographic replacement for Path comparison. */
LksStatus lks_path_format(
    const LksPath *path,
    char *buffer,
    size_t buffer_size
);
/* Compare two positions and write a negative/zero/positive result on success.
 * Inputs are borrowed; output is unchanged on failure. */
LksStatus lks_path_compare(
    const LksPath *left,
    const LksPath *right,
    int *out_result
);
/* Allocate a valid position ordered before RIGHT. Caller owns *out_path on
 * success; output is set to NULL on failure. RIGHT is unchanged. */
LksStatus lks_path_before(
    const LksPath *right,
    LksPath **out_path
);
/* Allocate a valid position ordered after LEFT. Caller owns *out_path on
 * success; output is set to NULL on failure. LEFT is unchanged. */
LksStatus lks_path_after(
    const LksPath *left,
    LksPath **out_path
);

/* Opaque public Tree and read-only node view. Tree owns its nodes and cloned
 * Paths; item pointers remain borrowed. All borrowed Tree nodes, Paths, and
 * navigation results expire on any successful Tree mutation. In particular,
 * comparator-driven insertion may replace every internal node. */
typedef struct LksTree LksTree;
typedef struct LksTreeNode LksTreeNode;
/* Opaque sorted collection. Group owns its structural storage, not its items. */
typedef struct LksGroup LksGroup;
/* Opaque collection of Groups. Batch owns its Groups, not the input items. */
typedef struct LksGroupBatch LksGroupBatch;

/* Build an independent sorted Group from borrowed item pointers. On success
 * caller owns *out_group and destroys it with lks_group_destroy. On failure
 * output is NULL and the input array/items are unchanged. */
LksStatus lks_group_build(
    void *const *items,
    size_t count,
    const LksComparator *comparator,
    LksGroup **out_group
);
/* Merge two sorted Groups into a new independent Group. Both inputs must have
 * been ordered under comparison semantics compatible with COMPARATOR,
 * including relevant context. Result Paths may be
 * reassigned, including Base Paths. Equal items from BASE
 * precede equal items from INCOMING; order within each source is preserved.
 * Inputs/items are borrowed and unchanged; caller owns the result. On failure
 * output is NULL and both inputs remain usable. */
LksStatus lks_group_merge(
    const LksGroup *base,
    const LksGroup *incoming,
    const LksComparator *comparator,
    LksGroup **out_group
);
/* Destroy a caller-owned Group; NULL is accepted. Does not destroy its items. */
void lks_group_destroy(LksGroup *group);
/* Return item count; NULL is treated as empty. */
size_t lks_group_size(const LksGroup *group);
/* Borrow an item pointer by sorted index; returns NULL for invalid input/index. */
void *lks_group_item_at(const LksGroup *group, size_t index);
/* Borrow a Group-owned Path, stable until the Group is destroyed.
 * Paths are local order coordinates, not persistent item identities. */
const LksPath *lks_group_path_at(const LksGroup *group, size_t index);

/* Build a Batch by splitting borrowed items into consecutive input chunks of
 * GROUP_SIZE and sorting each chunk. On success caller owns *out_batch; on
 * failure output is NULL and inputs/items are unchanged. */
LksStatus lks_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const LksComparator *comparator,
    LksGroupBatch **out_batch
);
/* Destroy a caller-owned Batch and its Groups; item pointers are not freed. */
void lks_group_batch_destroy(LksGroupBatch *batch);
/* Return total item count; NULL is treated as empty. */
size_t lks_group_batch_total_size(const LksGroupBatch *batch);
/* Return number of constituent Groups; NULL is treated as empty. */
size_t lks_group_batch_group_count(const LksGroupBatch *batch);
/* Return configured target size for each input chunk; NULL is treated as zero. */
size_t lks_group_batch_group_size(const LksGroupBatch *batch);
/* Borrow a Batch-owned Group, valid until the Batch is destroyed. */
const LksGroup *lks_group_batch_group_at(
    const LksGroupBatch *batch,
    size_t index
);
/* Merge all Batch Groups into a new independent Group. COMPARATOR must order
 * the constituent Groups compatibly with their build comparison semantics.
 * Caller owns the result;
 * Batch and its Groups remain unchanged. Equal items preserve source order.
 * On failure output is NULL and the Batch remains usable. */
LksStatus lks_group_batch_merge_all(
    const LksGroupBatch *batch,
    const LksComparator *comparator,
    LksGroup **out_group
);

/* Create an empty Tree. Caller owns it and destroys it with lks_tree_destroy;
 * returns NULL on allocation failure. */
LksTree *lks_tree_create(void);
/* Destroy a caller-owned Tree and its nodes/Paths; item pointers are not freed. */
void lks_tree_destroy(LksTree *tree);
/* Return node count; NULL is treated as empty. */
size_t lks_tree_size(const LksTree *tree);
/* Insert a unique explicit Path and borrowed item pointer. Tree clones PATH
 * without re-encoding it. On success optional OUT_NODE borrows the inserted
 * node; earlier borrowed Tree observations must be reacquired. Failure leaves
 * the logical Tree unchanged and OUT_NODE NULL. */
LksStatus lks_tree_insert(
    LksTree *tree,
    const LksPath *path,
    void *item,
    const LksTreeNode **out_node
);
/* Insert ITEM at a position determined by COMPARATOR. Equal items are inserted
 * stably after existing comparator-equal items. Item remains borrowed; failure
 * leaves the logical Tree unchanged and OUT_NODE NULL. Success may replace a
 * local subtree or the entire Tree; discard every borrowed Tree node/Path/navigation
 * result and use OUT_NODE as the new inserted node if requested. */
LksStatus lks_tree_insert_item(
    LksTree *tree,
    void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_node
);
/* Locate the neighboring/equal nodes for ITEM under COMPARATOR. Returned nodes
 * are borrowed from TREE and remain valid until mutation/destruction. */
LksStatus lks_tree_locate_item(
    const LksTree *tree,
    const void *item,
    const LksComparator *comparator,
    const LksTreeNode **out_left,
    const LksTreeNode **out_equal,
    const LksTreeNode **out_right
);
/* Find a node by Path; OUT_NODE borrows from TREE and is NULL on failure. */
LksStatus lks_tree_find_path(
    const LksTree *tree,
    const LksPath *path,
    const LksTreeNode **out_node
);
/* Return a node-owned Path borrowed until the Tree changes or is destroyed.
 * A successful Tree mutation may re-encode existing Paths. */
const LksPath *lks_tree_node_path(const LksTreeNode *node);
/* Return the borrowed item pointer stored in a node; NULL node returns NULL. */
void *lks_tree_node_item(const LksTreeNode *node);
/* Return borrowed parent node; the virtual root is reported as NULL. */
const LksTreeNode *lks_tree_node_parent(const LksTreeNode *node);
/* Return child count; NULL is treated as zero. */
size_t lks_tree_node_child_count(const LksTreeNode *node);
/* Return borrowed child node or NULL for an invalid index. */
const LksTreeNode *lks_tree_node_child_at(
    const LksTreeNode *node,
    size_t index
);
/* Return virtual-root child count; NULL is treated as zero. */
size_t lks_tree_root_child_count(const LksTree *tree);
/* Return borrowed virtual-root child node or NULL for an invalid index. */
const LksTreeNode *lks_tree_root_child_at(
    const LksTree *tree,
    size_t index
);
/* Allocate a Path strictly between ordered LEFT and RIGHT. Caller owns the
 * result; output is NULL on failure and both inputs remain unchanged. */
LksStatus lks_path_between(
    const LksPath *left,
    const LksPath *right,
    LksPath **out_path
);

#ifdef __cplusplus
}
#endif

#endif /* LAYERKEYSORT_H */
