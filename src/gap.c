#include <stddef.h>
#include "hps.h"

static HpsStatus validate_before_candidate(
    const HpsPath *right,
    HpsPath *candidate,
    HpsPath **out_path
)
{
    int comparison;

    if (hps_path_compare(candidate, right, &comparison) != HPS_STATUS_OK ||
        comparison != -1) {
        hps_path_destroy(candidate);
        return HPS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return HPS_STATUS_OK;
}

static HpsStatus validate_after_candidate(
    const HpsPath *left,
    HpsPath *candidate,
    HpsPath **out_path
)
{
    int comparison;

    if (hps_path_compare(left, candidate, &comparison) != HPS_STATUS_OK ||
        comparison != -1) {
        hps_path_destroy(candidate);
        return HPS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return HPS_STATUS_OK;
}

HpsStatus hps_path_before(const HpsPath *right, HpsPath **out_path)
{
    HpsDirection direction;
    HpsPath *candidate;
    HpsPath *zero_path;
    unsigned int root_slot;
    size_t root_level;
    size_t candidate_level;
    HpsStatus status;

    if (out_path == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (right == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    direction = hps_path_direction(right);
    if (direction == HPS_DIRECTION_ZERO) {
        candidate = hps_path_create_at_level(
            HPS_DIRECTION_NEGATIVE,
            HPS_PATH_SLOT_MIN,
            0
        );
        if (candidate == NULL) {
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        return validate_before_candidate(right, candidate, out_path);
    }

    if (direction == HPS_DIRECTION_POSITIVE) {
        zero_path = hps_path_create_zero();
        if (zero_path == NULL) {
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        status = hps_path_between(zero_path, right, out_path);
        hps_path_destroy(zero_path);
        return status;
    }

    if (direction != HPS_DIRECTION_NEGATIVE ||
        hps_path_get_slot(right, 0, &root_slot) != HPS_STATUS_OK ||
        hps_path_get_level(right, 0, &root_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    if (root_slot < HPS_PATH_SLOT_MAX) {
        candidate_level = root_level;
        ++root_slot;
    } else {
        if (root_level == (size_t)-1) {
            return HPS_STATUS_LEVEL_LIMIT;
        }
        candidate_level = root_level + 1;
    }

    candidate = hps_path_create_at_level(
        HPS_DIRECTION_NEGATIVE,
        root_slot,
        candidate_level
    );
    if (candidate == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    return validate_before_candidate(right, candidate, out_path);
}

HpsStatus hps_path_after(const HpsPath *left, HpsPath **out_path)
{
    HpsDirection direction;
    HpsPath *candidate;
    size_t depth;
    size_t last_level;
    HpsStatus status;

    if (out_path == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (left == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    direction = hps_path_direction(left);
    if (direction == HPS_DIRECTION_ZERO) {
        if (hps_path_depth(left) != 0) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        candidate = hps_path_create_at_level(
            HPS_DIRECTION_POSITIVE,
            HPS_PATH_SLOT_MIN,
            0
        );
        if (candidate == NULL) {
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        return validate_after_candidate(left, candidate, out_path);
    }

    if (direction != HPS_DIRECTION_POSITIVE &&
        direction != HPS_DIRECTION_NEGATIVE) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    depth = hps_path_depth(left);
    if (depth == 0 ||
        hps_path_get_level(left, depth - 1, &last_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    if (last_level == (size_t)-1) {
        return HPS_STATUS_LEVEL_LIMIT;
    }

    candidate = hps_path_clone(left);
    if (candidate == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    status = hps_path_append_at_level(candidate, HPS_PATH_SLOT_MIN, last_level + 1);
    if (status != HPS_STATUS_OK) {
        hps_path_destroy(candidate);
        return status;
    }
    return validate_after_candidate(left, candidate, out_path);
}

static int steps_equal(
    const HpsPath *left,
    size_t left_index,
    const HpsPath *right,
    size_t right_index
)
{
    unsigned int left_slot;
    unsigned int right_slot;
    size_t left_level;
    size_t right_level;

    if (hps_path_get_slot(left, left_index, &left_slot) != HPS_STATUS_OK ||
        hps_path_get_slot(right, right_index, &right_slot) != HPS_STATUS_OK ||
        hps_path_get_level(left, left_index, &left_level) != HPS_STATUS_OK ||
        hps_path_get_level(right, right_index, &right_level) != HPS_STATUS_OK) {
        return 0;
    }
    return left_slot == right_slot && left_level == right_level;
}

static HpsStatus build_prefix_and_step(
    const HpsPath *source,
    size_t prefix_depth,
    unsigned int slot,
    size_t level,
    HpsPath **out_path
)
{
    HpsPath *path;
    HpsDirection direction;
    size_t index;
    size_t source_level;
    unsigned int source_slot;
    HpsStatus status;

    *out_path = NULL;
    direction = hps_path_direction(source);
    if (prefix_depth == 0) {
        path = hps_path_create_at_level(direction, slot, level);
        if (path == NULL) {
            return HPS_STATUS_OUT_OF_MEMORY;
        }
        *out_path = path;
        return HPS_STATUS_OK;
    }

    if (hps_path_get_slot(source, 0, &source_slot) != HPS_STATUS_OK ||
        hps_path_get_level(source, 0, &source_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    path = hps_path_create_at_level(direction, source_slot, source_level);
    if (path == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }

    for (index = 1; index < prefix_depth; ++index) {
        if (hps_path_get_slot(source, index, &source_slot) != HPS_STATUS_OK ||
            hps_path_get_level(source, index, &source_level) != HPS_STATUS_OK) {
            hps_path_destroy(path);
            return HPS_STATUS_INTERNAL_ERROR;
        }
        status = hps_path_append_at_level(path, source_slot, source_level);
        if (status != HPS_STATUS_OK) {
            hps_path_destroy(path);
            return status == HPS_STATUS_OUT_OF_MEMORY ? status : HPS_STATUS_INTERNAL_ERROR;
        }
    }

    status = hps_path_append_at_level(path, slot, level);
    if (status != HPS_STATUS_OK) {
        hps_path_destroy(path);
        return status == HPS_STATUS_OUT_OF_MEMORY ? status : HPS_STATUS_INTERNAL_ERROR;
    }
    *out_path = path;
    return HPS_STATUS_OK;
}

static HpsStatus append_below_left(
    const HpsPath *left,
    HpsPath **out_path
)
{
    HpsPath *path;
    size_t depth;
    size_t last_level;
    HpsStatus status;

    *out_path = NULL;
    depth = hps_path_depth(left);
    if (depth == 0 ||
        hps_path_get_level(left, depth - 1, &last_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    if (last_level == (size_t)-1) {
        return HPS_STATUS_LEVEL_LIMIT;
    }

    path = hps_path_clone(left);
    if (path == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    status = hps_path_append_at_level(path, HPS_PATH_SLOT_MIN, last_level + 1);
    if (status != HPS_STATUS_OK) {
        hps_path_destroy(path);
        return status == HPS_STATUS_OUT_OF_MEMORY ? status : HPS_STATUS_INTERNAL_ERROR;
    }
    *out_path = path;
    return HPS_STATUS_OK;
}

static HpsStatus between_zero_and_positive(
    const HpsPath *right,
    HpsPath **out_path
)
{
    unsigned int first_slot;
    size_t first_level;
    size_t new_level;
    HpsPath *path;

    if (hps_path_get_slot(right, 0, &first_slot) != HPS_STATUS_OK ||
        hps_path_get_level(right, 0, &first_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    if (first_slot > HPS_PATH_SLOT_MIN) {
        new_level = first_level;
    } else {
        if (first_level == (size_t)-1) {
            return HPS_STATUS_LEVEL_LIMIT;
        }
        new_level = first_level + 1;
    }

    path = hps_path_create_at_level(
        HPS_DIRECTION_POSITIVE, HPS_PATH_SLOT_MIN, new_level
    );
    if (path == NULL) {
        return HPS_STATUS_OUT_OF_MEMORY;
    }
    *out_path = path;
    return HPS_STATUS_OK;
}

static HpsStatus after_negative_before_zero(
    const HpsPath *left,
    HpsPath **out_path
)
{
    size_t depth;
    size_t last_index;
    size_t current_level;
    size_t minimum_level;
    size_t parent_level;
    unsigned int current_slot;
    unsigned int next_slot;
    int is_root;

    depth = hps_path_depth(left);
    if (depth == 0) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    last_index = depth - 1;
    is_root = last_index == 0;
    if (hps_path_get_slot(left, last_index, &current_slot) != HPS_STATUS_OK ||
        hps_path_get_level(left, last_index, &current_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    if (is_root) {
        if (current_slot > HPS_PATH_SLOT_MIN) {
            next_slot = current_slot - 1;
            return build_prefix_and_step(
                left, last_index, next_slot, current_level, out_path
            );
        }
        minimum_level = 0;
    } else {
        if (current_slot < HPS_PATH_SLOT_MAX) {
            next_slot = current_slot + 1;
            return build_prefix_and_step(
                left, last_index, next_slot, current_level, out_path
            );
        }
        if (hps_path_get_level(left, last_index - 1, &parent_level) != HPS_STATUS_OK ||
            parent_level == (size_t)-1) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        minimum_level = parent_level + 1;
    }

    if (current_level > minimum_level) {
        next_slot = is_root ? HPS_PATH_SLOT_MAX : HPS_PATH_SLOT_MIN;
        return build_prefix_and_step(
            left, last_index, next_slot, current_level - 1, out_path
        );
    }
    return append_below_left(left, out_path);
}

static HpsStatus between_same_direction(
    const HpsPath *left,
    const HpsPath *right,
    HpsDirection direction,
    HpsPath **out_path
)
{
    size_t left_depth;
    size_t right_depth;
    size_t common_depth;
    size_t index;
    size_t left_level;
    size_t right_level;
    size_t candidate_level;
    unsigned int left_slot;
    unsigned int right_slot;
    unsigned int candidate_slot;
    int reverse_root_slot;

    left_depth = hps_path_depth(left);
    right_depth = hps_path_depth(right);
    common_depth = 0;
    while (common_depth < left_depth && common_depth < right_depth &&
           steps_equal(left, common_depth, right, common_depth)) {
        ++common_depth;
    }

    if (common_depth == left_depth) {
        if (left_depth >= right_depth) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (hps_path_get_slot(right, left_depth, &right_slot) != HPS_STATUS_OK ||
            hps_path_get_level(right, left_depth, &right_level) != HPS_STATUS_OK) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (right_slot > HPS_PATH_SLOT_MIN) {
            candidate_level = right_level;
        } else {
            if (right_level == (size_t)-1) {
                return HPS_STATUS_LEVEL_LIMIT;
            }
            candidate_level = right_level + 1;
        }
        return build_prefix_and_step(
            left, left_depth, HPS_PATH_SLOT_MIN, candidate_level, out_path
        );
    }
    if (common_depth >= right_depth) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    index = common_depth;
    if (hps_path_get_slot(left, index, &left_slot) != HPS_STATUS_OK ||
        hps_path_get_slot(right, index, &right_slot) != HPS_STATUS_OK ||
        hps_path_get_level(left, index, &left_level) != HPS_STATUS_OK ||
        hps_path_get_level(right, index, &right_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    reverse_root_slot =
        direction == HPS_DIRECTION_NEGATIVE && index == 0;

    if (left_level == right_level) {
        if (reverse_root_slot) {
            if (left_slot > right_slot && left_slot - right_slot > 1u) {
                candidate_slot = left_slot - 1u;
                return build_prefix_and_step(
                    left, common_depth, candidate_slot, left_level, out_path
                );
            }
        } else if (left_slot < right_slot && right_slot - left_slot > 1u) {
            candidate_slot = left_slot + 1u;
            return build_prefix_and_step(
                left, common_depth, candidate_slot, left_level, out_path
            );
        }
        return append_below_left(left, out_path);
    }

    if (left_level < right_level) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    if (reverse_root_slot) {
        if (left_slot > HPS_PATH_SLOT_MIN) {
            candidate_slot = left_slot - 1u;
            return build_prefix_and_step(
                left, common_depth, candidate_slot, left_level, out_path
            );
        }
    } else if (left_slot < HPS_PATH_SLOT_MAX) {
        candidate_slot = left_slot + 1u;
        return build_prefix_and_step(
            left, common_depth, candidate_slot, left_level, out_path
        );
    }

    if (left_level - right_level > 1) {
        candidate_level = left_level - 1;
        candidate_slot = reverse_root_slot ?
            HPS_PATH_SLOT_MAX : HPS_PATH_SLOT_MIN;
        return build_prefix_and_step(
            left, common_depth, candidate_slot, candidate_level, out_path
        );
    }
    return append_below_left(left, out_path);
}

static HpsStatus validate_between_result(
    const HpsPath *left,
    const HpsPath *right,
    HpsPath *candidate,
    HpsPath **out_path
)
{
    int left_comparison;
    int right_comparison;

    if (hps_path_compare(left, candidate, &left_comparison) != HPS_STATUS_OK ||
        hps_path_compare(candidate, right, &right_comparison) != HPS_STATUS_OK ||
        left_comparison != -1 || right_comparison != -1) {
        hps_path_destroy(candidate);
        return HPS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return HPS_STATUS_OK;
}

HpsStatus hps_path_between(
    const HpsPath *left,
    const HpsPath *right,
    HpsPath **out_path
)
{
    HpsDirection left_direction;
    HpsDirection right_direction;
    int comparison;
    HpsPath *candidate;
    HpsStatus status;

    if (out_path == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (left == NULL || right == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    status = hps_path_compare(left, right, &comparison);
    if (status != HPS_STATUS_OK) {
        return status;
    }
    if (comparison != -1) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    left_direction = hps_path_direction(left);
    right_direction = hps_path_direction(right);
    if (left_direction == HPS_DIRECTION_NEGATIVE &&
        right_direction == HPS_DIRECTION_POSITIVE) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    candidate = NULL;
    if (left_direction == HPS_DIRECTION_ZERO &&
        right_direction == HPS_DIRECTION_POSITIVE) {
        status = between_zero_and_positive(right, &candidate);
    } else if (left_direction == HPS_DIRECTION_NEGATIVE &&
               right_direction == HPS_DIRECTION_ZERO) {
        status = after_negative_before_zero(left, &candidate);
    } else if (left_direction == right_direction &&
               (left_direction == HPS_DIRECTION_POSITIVE ||
                left_direction == HPS_DIRECTION_NEGATIVE)) {
        status = between_same_direction(
            left, right, left_direction, &candidate
        );
    } else {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    if (status != HPS_STATUS_OK) {
        hps_path_destroy(candidate);
        return status;
    }
    if (candidate == NULL) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    return validate_between_result(left, right, candidate, out_path);
}
