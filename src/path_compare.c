#include "hps.h"

static int direction_rank(HpsDirection direction)
{
    switch (direction) {
    case HPS_DIRECTION_NEGATIVE:
        return 0;
    case HPS_DIRECTION_ZERO:
        return 1;
    case HPS_DIRECTION_POSITIVE:
        return 2;
    default:
        return -1;
    }
}

static int path_is_consistent(
    const HpsPath *path,
    HpsDirection direction,
    size_t depth
)
{
    size_t index;
    size_t level;
    size_t previous_level;
    unsigned int slot;

    if (direction == HPS_DIRECTION_ZERO) {
        return depth == 0;
    }
    if ((direction != HPS_DIRECTION_POSITIVE &&
         direction != HPS_DIRECTION_NEGATIVE) || depth == 0) {
        return 0;
    }

    previous_level = 0;
    for (index = 0; index < depth; ++index) {
        if (hps_path_get_slot(path, index, &slot) != HPS_STATUS_OK ||
            slot > HPS_PATH_SLOT_MAX ||
            hps_path_get_level(path, index, &level) != HPS_STATUS_OK) {
            return 0;
        }
        if (index > 0 && level <= previous_level) {
            return 0;
        }
        previous_level = level;
    }
    return 1;
}

HpsStatus hps_path_compare(
    const HpsPath *left,
    const HpsPath *right,
    int *out_result
)
{
    HpsDirection left_direction;
    HpsDirection right_direction;
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
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    left_direction = hps_path_direction(left);
    right_direction = hps_path_direction(right);
    left_depth = hps_path_depth(left);
    right_depth = hps_path_depth(right);

    left_rank = direction_rank(left_direction);
    right_rank = direction_rank(right_direction);
    if (left_rank < 0 || right_rank < 0 ||
        !path_is_consistent(left, left_direction, left_depth) ||
        !path_is_consistent(right, right_direction, right_depth)) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    if (left_direction != right_direction) {
        *out_result = (left_rank < right_rank) ? -1 : 1;
        return HPS_STATUS_OK;
    }

    if (left_direction == HPS_DIRECTION_ZERO) {
        *out_result = 0;
        return HPS_STATUS_OK;
    }

    common_depth = (left_depth < right_depth) ? left_depth : right_depth;
    for (index = 0; index < common_depth; ++index) {
        if (hps_path_get_slot(left, index, &left_slot) != HPS_STATUS_OK ||
            hps_path_get_slot(right, index, &right_slot) != HPS_STATUS_OK ||
            hps_path_get_level(left, index, &left_level) != HPS_STATUS_OK ||
            hps_path_get_level(right, index, &right_level) != HPS_STATUS_OK) {
            return HPS_STATUS_INTERNAL_ERROR;
        }

        if (left_level != right_level) {
            *out_result = (left_level > right_level) ? -1 : 1;
            return HPS_STATUS_OK;
        }

        if (left_slot != right_slot) {
            if (left_direction == HPS_DIRECTION_NEGATIVE && index == 0) {
                result = (left_slot > right_slot) ? -1 : 1;
            } else {
                result = (left_slot < right_slot) ? -1 : 1;
            }
            *out_result = result;
            return HPS_STATUS_OK;
        }
    }

    if (left_depth < right_depth) {
        *out_result = -1;
    } else if (left_depth > right_depth) {
        *out_result = 1;
    } else {
        *out_result = 0;
    }
    return HPS_STATUS_OK;
}
