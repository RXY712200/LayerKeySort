#include "layerkeysort.h"

static int direction_rank(LksDirection direction)
{
    switch (direction) {
    case LKS_DIRECTION_NEGATIVE:
        return 0;
    case LKS_DIRECTION_ZERO:
        return 1;
    case LKS_DIRECTION_POSITIVE:
        return 2;
    default:
        return -1;
    }
}

static int path_is_consistent(
    const LksPath *path,
    LksDirection direction,
    size_t depth
)
{
    size_t index;
    size_t level;
    size_t previous_level;
    unsigned int slot;

    if (direction == LKS_DIRECTION_ZERO) {
        return depth == 0;
    }
    if ((direction != LKS_DIRECTION_POSITIVE &&
         direction != LKS_DIRECTION_NEGATIVE) || depth == 0) {
        return 0;
    }

    previous_level = 0;
    for (index = 0; index < depth; ++index) {
        if (lks_path_get_slot(path, index, &slot) != LKS_STATUS_OK ||
            slot > LKS_PATH_SLOT_MAX ||
            lks_path_get_level(path, index, &level) != LKS_STATUS_OK) {
            return 0;
        }
        if (index > 0 && level <= previous_level) {
            return 0;
        }
        previous_level = level;
    }
    return 1;
}

LksStatus lks_path_compare(
    const LksPath *left,
    const LksPath *right,
    int *out_result
)
{
    LksDirection left_direction;
    LksDirection right_direction;
    size_t left_depth;
    size_t right_depth;
    size_t common_depth;
    size_t index;
    size_t left_level;
    size_t right_level;
    unsigned int left_slot;
    unsigned int right_slot;
    int left_rank;
    int right_rank;
    int result;

    if (left == NULL || right == NULL || out_result == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    left_direction = lks_path_direction(left);
    right_direction = lks_path_direction(right);
    left_depth = lks_path_depth(left);
    right_depth = lks_path_depth(right);

    left_rank = direction_rank(left_direction);
    right_rank = direction_rank(right_direction);
    if (left_rank < 0 || right_rank < 0 ||
        !path_is_consistent(left, left_direction, left_depth) ||
        !path_is_consistent(right, right_direction, right_depth)) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    if (left_direction != right_direction) {
        *out_result = (left_rank < right_rank) ? -1 : 1;
        return LKS_STATUS_OK;
    }

    if (left_direction == LKS_DIRECTION_ZERO) {
        *out_result = 0;
        return LKS_STATUS_OK;
    }

    common_depth = (left_depth < right_depth) ? left_depth : right_depth;
    for (index = 0; index < common_depth; ++index) {
        if (lks_path_get_slot(left, index, &left_slot) != LKS_STATUS_OK ||
            lks_path_get_slot(right, index, &right_slot) != LKS_STATUS_OK ||
            lks_path_get_level(left, index, &left_level) != LKS_STATUS_OK ||
            lks_path_get_level(right, index, &right_level) != LKS_STATUS_OK) {
            return LKS_STATUS_INTERNAL_ERROR;
        }

        if (left_level != right_level) {
            *out_result = (left_level > right_level) ? -1 : 1;
            return LKS_STATUS_OK;
        }

        if (left_slot != right_slot) {
            if (left_direction == LKS_DIRECTION_NEGATIVE && index == 0) {
                result = (left_slot > right_slot) ? -1 : 1;
            } else {
                result = (left_slot < right_slot) ? -1 : 1;
            }
            *out_result = result;
            return LKS_STATUS_OK;
        }
    }

    if (left_depth < right_depth) {
        *out_result = -1;
    } else if (left_depth > right_depth) {
        *out_result = 1;
    } else {
        *out_result = 0;
    }
    return LKS_STATUS_OK;
}
