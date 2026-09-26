#include <stddef.h>
#include "layerkeysort.h"

static LksStatus validate_before_candidate(
    const LksPath *right,
    LksPath *candidate,
    LksPath **out_path
)
{
    int comparison;

    if (lks_path_compare(candidate, right, &comparison) != LKS_STATUS_OK ||
        comparison != -1) {
        lks_path_destroy(candidate);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return LKS_STATUS_OK;
}

static LksStatus validate_after_candidate(
    const LksPath *left,
    LksPath *candidate,
    LksPath **out_path
)
{
    int comparison;

    if (lks_path_compare(left, candidate, &comparison) != LKS_STATUS_OK ||
        comparison != -1) {
        lks_path_destroy(candidate);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return LKS_STATUS_OK;
}

LksStatus lks_path_before(const LksPath *right, LksPath **out_path)
{
    LksDirection direction;
    LksPath *candidate;
    LksPath *zero_path;
    unsigned int root_slot;
    size_t root_level;
    size_t candidate_level;
    LksStatus status;

    if (out_path == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (right == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    direction = lks_path_direction(right);
    if (direction == LKS_DIRECTION_ZERO) {
        candidate = lks_path_create_at_level(
            LKS_DIRECTION_NEGATIVE,
            LKS_PATH_SLOT_MIN,
            0
        );
        if (candidate == NULL) {
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        return validate_before_candidate(right, candidate, out_path);
    }

    if (direction == LKS_DIRECTION_POSITIVE) {
        zero_path = lks_path_create_zero();
        if (zero_path == NULL) {
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        status = lks_path_between(zero_path, right, out_path);
        lks_path_destroy(zero_path);
        return status;
    }

    if (direction != LKS_DIRECTION_NEGATIVE ||
        lks_path_get_slot(right, 0, &root_slot) != LKS_STATUS_OK ||
        lks_path_get_level(right, 0, &root_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    if (root_slot < LKS_PATH_SLOT_MAX) {
        candidate_level = root_level;
        ++root_slot;
    } else {
        if (root_level == (size_t)-1) {
            return LKS_STATUS_LEVEL_LIMIT;
        }
        candidate_level = root_level + 1;
    }

    candidate = lks_path_create_at_level(
        LKS_DIRECTION_NEGATIVE,
        root_slot,
        candidate_level
    );
    if (candidate == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    return validate_before_candidate(right, candidate, out_path);
}

LksStatus lks_path_after(const LksPath *left, LksPath **out_path)
{
    LksDirection direction;
    LksPath *candidate;
    size_t depth;
    size_t last_level;
    LksStatus status;

    if (out_path == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (left == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    direction = lks_path_direction(left);
    if (direction == LKS_DIRECTION_ZERO) {
        if (lks_path_depth(left) != 0) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        candidate = lks_path_create_at_level(
            LKS_DIRECTION_POSITIVE,
            LKS_PATH_SLOT_MIN,
            0
        );
        if (candidate == NULL) {
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        return validate_after_candidate(left, candidate, out_path);
    }

    if (direction != LKS_DIRECTION_POSITIVE &&
        direction != LKS_DIRECTION_NEGATIVE) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    depth = lks_path_depth(left);
    if (depth == 0 ||
        lks_path_get_level(left, depth - 1, &last_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    if (last_level == (size_t)-1) {
        return LKS_STATUS_LEVEL_LIMIT;
    }

    candidate = lks_path_clone(left);
    if (candidate == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    status = lks_path_append_at_level(candidate, LKS_PATH_SLOT_MIN, last_level + 1);
    if (status != LKS_STATUS_OK) {
        lks_path_destroy(candidate);
        return status;
    }
    return validate_after_candidate(left, candidate, out_path);
}

static int steps_equal(
    const LksPath *left,
    size_t left_index,
    const LksPath *right,
    size_t right_index
)
{
    unsigned int left_slot;
    unsigned int right_slot;
    size_t left_level;
    size_t right_level;

    if (lks_path_get_slot(left, left_index, &left_slot) != LKS_STATUS_OK ||
        lks_path_get_slot(right, right_index, &right_slot) != LKS_STATUS_OK ||
        lks_path_get_level(left, left_index, &left_level) != LKS_STATUS_OK ||
        lks_path_get_level(right, right_index, &right_level) != LKS_STATUS_OK) {
        return 0;
    }
    return left_slot == right_slot && left_level == right_level;
}

static LksStatus build_prefix_and_step(
    const LksPath *source,
    size_t prefix_depth,
    unsigned int slot,
    size_t level,
    LksPath **out_path
)
{
    LksPath *path;
    LksDirection direction;
    size_t index;
    size_t source_level;
    unsigned int source_slot;
    LksStatus status;

    *out_path = NULL;
    direction = lks_path_direction(source);
    if (prefix_depth == 0) {
        path = lks_path_create_at_level(direction, slot, level);
        if (path == NULL) {
            return LKS_STATUS_OUT_OF_MEMORY;
        }
        *out_path = path;
        return LKS_STATUS_OK;
    }

    if (lks_path_get_slot(source, 0, &source_slot) != LKS_STATUS_OK ||
        lks_path_get_level(source, 0, &source_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    path = lks_path_create_at_level(direction, source_slot, source_level);
    if (path == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }

    for (index = 1; index < prefix_depth; ++index) {
        if (lks_path_get_slot(source, index, &source_slot) != LKS_STATUS_OK ||
            lks_path_get_level(source, index, &source_level) != LKS_STATUS_OK) {
            lks_path_destroy(path);
            return LKS_STATUS_INTERNAL_ERROR;
        }
        status = lks_path_append_at_level(path, source_slot, source_level);
        if (status != LKS_STATUS_OK) {
            lks_path_destroy(path);
            return status == LKS_STATUS_OUT_OF_MEMORY ? status : LKS_STATUS_INTERNAL_ERROR;
        }
    }

    status = lks_path_append_at_level(path, slot, level);
    if (status != LKS_STATUS_OK) {
        lks_path_destroy(path);
        return status == LKS_STATUS_OUT_OF_MEMORY ? status : LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = path;
    return LKS_STATUS_OK;
}

static LksStatus append_below_left(
    const LksPath *left,
    LksPath **out_path
)
{
    LksPath *path;
    size_t depth;
    size_t last_level;
    LksStatus status;

    *out_path = NULL;
    depth = lks_path_depth(left);
    if (depth == 0 ||
        lks_path_get_level(left, depth - 1, &last_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    if (last_level == (size_t)-1) {
        return LKS_STATUS_LEVEL_LIMIT;
    }

    path = lks_path_clone(left);
    if (path == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    status = lks_path_append_at_level(path, LKS_PATH_SLOT_MIN, last_level + 1);
    if (status != LKS_STATUS_OK) {
        lks_path_destroy(path);
        return status == LKS_STATUS_OUT_OF_MEMORY ? status : LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = path;
    return LKS_STATUS_OK;
}

static LksStatus between_zero_and_positive(
    const LksPath *right,
    LksPath **out_path
)
{
    unsigned int first_slot;
    size_t first_level;
    size_t new_level;
    LksPath *path;

    if (lks_path_get_slot(right, 0, &first_slot) != LKS_STATUS_OK ||
        lks_path_get_level(right, 0, &first_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    if (first_slot > LKS_PATH_SLOT_MIN) {
        new_level = first_level;
    } else {
        if (first_level == (size_t)-1) {
            return LKS_STATUS_LEVEL_LIMIT;
        }
        new_level = first_level + 1;
    }

    path = lks_path_create_at_level(
        LKS_DIRECTION_POSITIVE, LKS_PATH_SLOT_MIN, new_level
    );
    if (path == NULL) {
        return LKS_STATUS_OUT_OF_MEMORY;
    }
    *out_path = path;
    return LKS_STATUS_OK;
}

static LksStatus after_negative_before_zero(
    const LksPath *left,
    LksPath **out_path
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

    depth = lks_path_depth(left);
    if (depth == 0) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    last_index = depth - 1;
    is_root = last_index == 0;
    if (lks_path_get_slot(left, last_index, &current_slot) != LKS_STATUS_OK ||
        lks_path_get_level(left, last_index, &current_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    if (is_root) {
        if (current_slot > LKS_PATH_SLOT_MIN) {
            next_slot = current_slot - 1;
            return build_prefix_and_step(
                left, last_index, next_slot, current_level, out_path
            );
        }
        minimum_level = 0;
    } else {
        if (current_slot < LKS_PATH_SLOT_MAX) {
            next_slot = current_slot + 1;
            return build_prefix_and_step(
                left, last_index, next_slot, current_level, out_path
            );
        }
        if (lks_path_get_level(left, last_index - 1, &parent_level) != LKS_STATUS_OK ||
            parent_level == (size_t)-1) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        minimum_level = parent_level + 1;
    }

    if (current_level > minimum_level) {
        next_slot = is_root ? LKS_PATH_SLOT_MAX : LKS_PATH_SLOT_MIN;
        return build_prefix_and_step(
            left, last_index, next_slot, current_level - 1, out_path
        );
    }
    return append_below_left(left, out_path);
}

static LksStatus between_same_direction(
    const LksPath *left,
    const LksPath *right,
    LksDirection direction,
    LksPath **out_path
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

    left_depth = lks_path_depth(left);
    right_depth = lks_path_depth(right);
    common_depth = 0;
    while (common_depth < left_depth && common_depth < right_depth &&
           steps_equal(left, common_depth, right, common_depth)) {
        ++common_depth;
    }

    if (common_depth == left_depth) {
        if (left_depth >= right_depth) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (lks_path_get_slot(right, left_depth, &right_slot) != LKS_STATUS_OK ||
            lks_path_get_level(right, left_depth, &right_level) != LKS_STATUS_OK) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (right_slot > LKS_PATH_SLOT_MIN) {
            candidate_level = right_level;
        } else {
            if (right_level == (size_t)-1) {
                return LKS_STATUS_LEVEL_LIMIT;
            }
            candidate_level = right_level + 1;
        }
        return build_prefix_and_step(
            left, left_depth, LKS_PATH_SLOT_MIN, candidate_level, out_path
        );
    }
    if (common_depth >= right_depth) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    index = common_depth;
    if (lks_path_get_slot(left, index, &left_slot) != LKS_STATUS_OK ||
        lks_path_get_slot(right, index, &right_slot) != LKS_STATUS_OK ||
        lks_path_get_level(left, index, &left_level) != LKS_STATUS_OK ||
        lks_path_get_level(right, index, &right_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    reverse_root_slot =
        direction == LKS_DIRECTION_NEGATIVE && index == 0;

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
        return LKS_STATUS_INTERNAL_ERROR;
    }
    if (reverse_root_slot) {
        if (left_slot > LKS_PATH_SLOT_MIN) {
            candidate_slot = left_slot - 1u;
            return build_prefix_and_step(
                left, common_depth, candidate_slot, left_level, out_path
            );
        }
    } else if (left_slot < LKS_PATH_SLOT_MAX) {
        candidate_slot = left_slot + 1u;
        return build_prefix_and_step(
            left, common_depth, candidate_slot, left_level, out_path
        );
    }

    if (left_level - right_level > 1) {
        candidate_level = left_level - 1;
        candidate_slot = reverse_root_slot ?
            LKS_PATH_SLOT_MAX : LKS_PATH_SLOT_MIN;
        return build_prefix_and_step(
            left, common_depth, candidate_slot, candidate_level, out_path
        );
    }
    return append_below_left(left, out_path);
}

static LksStatus validate_between_result(
    const LksPath *left,
    const LksPath *right,
    LksPath *candidate,
    LksPath **out_path
)
{
    int left_comparison;
    int right_comparison;

    if (lks_path_compare(left, candidate, &left_comparison) != LKS_STATUS_OK ||
        lks_path_compare(candidate, right, &right_comparison) != LKS_STATUS_OK ||
        left_comparison != -1 || right_comparison != -1) {
        lks_path_destroy(candidate);
        return LKS_STATUS_INTERNAL_ERROR;
    }
    *out_path = candidate;
    return LKS_STATUS_OK;
}

LksStatus lks_path_between(
    const LksPath *left,
    const LksPath *right,
    LksPath **out_path
)
{
    LksDirection left_direction;
    LksDirection right_direction;
    int comparison;
    LksPath *candidate;
    LksStatus status;

    if (out_path == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }
    *out_path = NULL;
    if (left == NULL || right == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    status = lks_path_compare(left, right, &comparison);
    if (status != LKS_STATUS_OK) {
        return status;
    }
    if (comparison != -1) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    left_direction = lks_path_direction(left);
    right_direction = lks_path_direction(right);
    if (left_direction == LKS_DIRECTION_NEGATIVE &&
        right_direction == LKS_DIRECTION_POSITIVE) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    candidate = NULL;
    if (left_direction == LKS_DIRECTION_ZERO &&
        right_direction == LKS_DIRECTION_POSITIVE) {
        status = between_zero_and_positive(right, &candidate);
    } else if (left_direction == LKS_DIRECTION_NEGATIVE &&
               right_direction == LKS_DIRECTION_ZERO) {
        status = after_negative_before_zero(left, &candidate);
    } else if (left_direction == right_direction &&
               (left_direction == LKS_DIRECTION_POSITIVE ||
                left_direction == LKS_DIRECTION_NEGATIVE)) {
        status = between_same_direction(
            left, right, left_direction, &candidate
        );
    } else {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    if (status != LKS_STATUS_OK) {
        lks_path_destroy(candidate);
        return status;
    }
    if (candidate == NULL) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    return validate_between_result(left, right, candidate, out_path);
}
