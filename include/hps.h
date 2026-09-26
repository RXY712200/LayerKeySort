#ifndef HPS_H
#define HPS_H

#include <stddef.h>

/* HPSort public API. HPS_VERSION_* describes the current API generation. */
#ifdef __cplusplus
extern "C" {
#endif

#define HPS_VERSION_MAJOR 1
#define HPS_VERSION_MINOR 0

/* Public Path slot range: A0..Z9 maps to values 0..259. */
#define HPS_PATH_SLOT_MIN 0u
#define HPS_PATH_SLOT_MAX 259u

/* Operation results. Constructors returning pointers use NULL on failure. */
typedef enum HpsStatus {
    HPS_STATUS_OK = 0,
    HPS_STATUS_INVALID_ARGUMENT,
    HPS_STATUS_OUT_OF_MEMORY,
    HPS_STATUS_BUFFER_TOO_SMALL,
    HPS_STATUS_LEVEL_LIMIT,
    HPS_STATUS_NOT_FOUND,
    HPS_STATUS_NOT_IMPLEMENTED,
    HPS_STATUS_INTERNAL_ERROR,
    HPS_STATUS_ALREADY_EXISTS
} HpsStatus;

/* Generic item comparison: negative / zero / positive means before / equal /
 * after. HPSort passes context through without interpreting it. Group and
 * merge operations are stable for comparator-equal items. */
typedef int (*HpsCompareFn)(
    const void *left,
    const void *right,
    void *context
);

/* Comparator callback and caller-owned context used for one operation. */
typedef struct HpsComparator {
    HpsCompareFn compare;
    void *context;
} HpsComparator;

/* Position direction around ZERO. */
typedef enum HpsDirection {
    HPS_DIRECTION_ZERO = 0,
    HPS_DIRECTION_POSITIVE = 1,
    HPS_DIRECTION_NEGATIVE = -1
} HpsDirection;

/* Opaque position value. */
typedef struct HpsPath HpsPath;

/* Return a static, library-owned description of a status value. */
const char *hps_status_string(HpsStatus status);

/* Create a ZERO Path. Caller owns the returned Path and destroys it with
 * hps_path_destroy; returns NULL if allocation fails. */
HpsPath *hps_path_create_zero(void);
/* Create a one-step Path at level zero. Caller owns the result; NULL indicates
 * an invalid direction/slot or allocation failure. */
HpsPath *hps_path_create(
    HpsDirection direction,
    unsigned int first_slot
);
/* Create a one-step Path at an explicit level. Same ownership and failure
 * behavior as hps_path_create. */
HpsPath *hps_path_create_at_level(
    HpsDirection direction,
    unsigned int first_slot,
    size_t level
);
/* Deep-copy a Path. Caller owns the independent copy; NULL means invalid input
 * or allocation failure. Source is unchanged. */
HpsPath *hps_path_clone(
    const HpsPath *source
);
/* Append a step at the next level. On failure the Path remains unchanged. */
HpsStatus hps_path_append(
    HpsPath *path,
    unsigned int slot
);
/* Append a step at a strictly greater explicit level. On failure the Path is
 * unchanged; may return INVALID_ARGUMENT, LEVEL_LIMIT, or OUT_OF_MEMORY. */
HpsStatus hps_path_append_at_level(
    HpsPath *path,
    unsigned int slot,
    size_t level
);
/* Read direction; NULL is treated as ZERO. */
HpsDirection hps_path_direction(
    const HpsPath *path
);
/* Read step count; NULL is treated as an empty Path. */
size_t hps_path_depth(
    const HpsPath *path
);
/* Read a step slot into caller storage; invalid input/index returns
 * INVALID_ARGUMENT and leaves the output unchanged. */
HpsStatus hps_path_get_slot(
    const HpsPath *path,
    size_t index,
    unsigned int *out_slot
);
/* Read a step level into caller storage; invalid input/index returns
 * INVALID_ARGUMENT and leaves the output unchanged. */
HpsStatus hps_path_get_level(
    const HpsPath *path,
    size_t index,
    size_t *out_level
);
/* Destroy a caller-owned Path; NULL is accepted. */
void hps_path_destroy(
    HpsPath *path
);
/* Return formatted character count excluding the terminator; zero means NULL
 * input or invalid internal Path state. */
size_t hps_path_text_length(
    const HpsPath *path
);
/* Format into caller-owned storage. BUFFER_TOO_SMALL reports insufficient
 * capacity; the Path is never modified. */
HpsStatus hps_path_format(
    const HpsPath *path,
    char *buffer,
    size_t buffer_size
);
/* Compare two positions and write a negative/zero/positive result on success.
 * Inputs are borrowed; output is unchanged on failure. */
HpsStatus hps_path_compare(
    const HpsPath *left,
    const HpsPath *right,
    int *out_result
);
/* Allocate a position immediately before RIGHT. Caller owns *out_path on
 * success; output is set to NULL on failure. RIGHT is unchanged. */
HpsStatus hps_path_before(
    const HpsPath *right,
    HpsPath **out_path
);
/* Allocate a position immediately after LEFT. Caller owns *out_path on
 * success; output is set to NULL on failure. LEFT is unchanged. */
HpsStatus hps_path_after(
    const HpsPath *left,
    HpsPath **out_path
);

/* Opaque public Tree and read-only node view. Tree owns its nodes and cloned
 * Paths; item pointers remain borrowed. */
typedef struct HpsTree HpsTree;
typedef struct HpsTreeNode HpsTreeNode;
/* Opaque sorted collection. Group owns its structural storage, not its items. */
typedef struct HpsGroup HpsGroup;
/* Opaque collection of Groups. Batch owns its Groups, not the input items. */
typedef struct HpsGroupBatch HpsGroupBatch;

/* Build an independent sorted Group from borrowed item pointers. On success
 * caller owns *out_group and destroys it with hps_group_destroy. On failure
 * output is NULL and the input array/items are unchanged. */
HpsStatus hps_group_build(
    void *const *items,
    size_t count,
    const HpsComparator *comparator,
    HpsGroup **out_group
);
/* Merge two sorted Groups into a new independent Group. Equal items from BASE
 * precede equal items from INCOMING; order within each source is preserved.
 * Inputs/items are borrowed and unchanged; caller owns the result. On failure
 * output is NULL and both inputs remain usable. */
HpsStatus hps_group_merge(
    const HpsGroup *base,
    const HpsGroup *incoming,
    const HpsComparator *comparator,
    HpsGroup **out_group
);
/* Destroy a caller-owned Group; NULL is accepted. Does not destroy its items. */
void hps_group_destroy(HpsGroup *group);
/* Return item count; NULL is treated as empty. */
size_t hps_group_size(const HpsGroup *group);
/* Borrow an item pointer by sorted index; returns NULL for invalid input/index. */
void *hps_group_item_at(const HpsGroup *group, size_t index);
/* Borrow a Group-owned Path, valid until the Group is destroyed. */
const HpsPath *hps_group_path_at(const HpsGroup *group, size_t index);

/* Build a Batch by splitting borrowed items into consecutive input chunks of
 * GROUP_SIZE and sorting each chunk. On success caller owns *out_batch; on
 * failure output is NULL and inputs/items are unchanged. */
HpsStatus hps_group_batch_build(
    void *const *items,
    size_t count,
    size_t group_size,
    const HpsComparator *comparator,
    HpsGroupBatch **out_batch
);
/* Destroy a caller-owned Batch and its Groups; item pointers are not freed. */
void hps_group_batch_destroy(HpsGroupBatch *batch);
/* Return total item count; NULL is treated as empty. */
size_t hps_group_batch_total_size(const HpsGroupBatch *batch);
/* Return number of constituent Groups; NULL is treated as empty. */
size_t hps_group_batch_group_count(const HpsGroupBatch *batch);
/* Return configured target size for each input chunk; NULL is treated as zero. */
size_t hps_group_batch_group_size(const HpsGroupBatch *batch);
/* Borrow a Batch-owned Group, valid until the Batch is destroyed. */
const HpsGroup *hps_group_batch_group_at(
    const HpsGroupBatch *batch,
    size_t index
);
/* Merge all Batch Groups into a new independent Group. Caller owns the result;
 * Batch and its Groups remain unchanged. Equal items preserve source order.
 * On failure output is NULL and the Batch remains usable. */
HpsStatus hps_group_batch_merge_all(
    const HpsGroupBatch *batch,
    const HpsComparator *comparator,
    HpsGroup **out_group
);

/* Create an empty Tree. Caller owns it and destroys it with hps_tree_destroy;
 * returns NULL on allocation failure. */
HpsTree *hps_tree_create(void);
/* Destroy a caller-owned Tree and its nodes/Paths; item pointers are not freed. */
void hps_tree_destroy(HpsTree *tree);
/* Return node count; NULL is treated as empty. */
size_t hps_tree_size(const HpsTree *tree);
/* Insert a unique explicit Path and borrowed item pointer. Tree clones PATH.
 * On success optional OUT_NODE borrows the inserted node. Failure leaves the
 * logical Tree unchanged and OUT_NODE NULL. */
HpsStatus hps_tree_insert(
    HpsTree *tree,
    const HpsPath *path,
    void *item,
    const HpsTreeNode **out_node
);
/* Insert ITEM at a position determined by COMPARATOR. Equal items are inserted
 * stably after existing comparator-equal items. Item remains borrowed; failure
 * leaves the logical Tree unchanged and OUT_NODE NULL. */
HpsStatus hps_tree_insert_item(
    HpsTree *tree,
    void *item,
    const HpsComparator *comparator,
    const HpsTreeNode **out_node
);
/* Locate the neighboring/equal nodes for ITEM under COMPARATOR. Returned nodes
 * are borrowed from TREE and remain valid until mutation/destruction. */
HpsStatus hps_tree_locate_item(
    const HpsTree *tree,
    const void *item,
    const HpsComparator *comparator,
    const HpsTreeNode **out_left,
    const HpsTreeNode **out_equal,
    const HpsTreeNode **out_right
);
/* Find a node by Path; OUT_NODE borrows from TREE and is NULL on failure. */
HpsStatus hps_tree_find_path(
    const HpsTree *tree,
    const HpsPath *path,
    const HpsTreeNode **out_node
);
/* Return a node-owned Path borrowed until the Tree changes or is destroyed. */
const HpsPath *hps_tree_node_path(const HpsTreeNode *node);
/* Return the borrowed item pointer stored in a node; NULL node returns NULL. */
void *hps_tree_node_item(const HpsTreeNode *node);
/* Return borrowed parent node; the virtual root is reported as NULL. */
const HpsTreeNode *hps_tree_node_parent(const HpsTreeNode *node);
/* Return child count; NULL is treated as zero. */
size_t hps_tree_node_child_count(const HpsTreeNode *node);
/* Return borrowed child node or NULL for an invalid index. */
const HpsTreeNode *hps_tree_node_child_at(
    const HpsTreeNode *node,
    size_t index
);
/* Return virtual-root child count; NULL is treated as zero. */
size_t hps_tree_root_child_count(const HpsTree *tree);
/* Return borrowed virtual-root child node or NULL for an invalid index. */
const HpsTreeNode *hps_tree_root_child_at(
    const HpsTree *tree,
    size_t index
);
/* Allocate a Path strictly between ordered LEFT and RIGHT. Caller owns the
 * result; output is NULL on failure and both inputs remain unchanged. */
HpsStatus hps_path_between(
    const HpsPath *left,
    const HpsPath *right,
    HpsPath **out_path
);

#ifdef __cplusplus
}
#endif

#endif /* HPS_H */
