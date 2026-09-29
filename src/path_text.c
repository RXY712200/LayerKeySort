#include "layerkeysort.h"
#include "lks_slot_codec_internal.h"
#include <string.h>

static const char zero_path_text[] = "000";

static size_t decimal_digits(size_t value)
{
    size_t digits = 1;
    while (value >= 10) { value /= 10; ++digits; }
    return digits;
}

static int add_length(size_t *length, size_t addition)
{
    if (*length > (size_t)-1 - addition) return 0;
    *length += addition;
    return 1;
}

/* The final three characters of each segment are always the slot, so a
 * preceding decimal field is unambiguous. The first level is absolute; later
 * levels are deltas from the previous step. Omit +1, the common case. '/' is
 * only a step separator, never a unary level count. */
static LksStatus measure_path_text(const LksPath *path, size_t *out_length)
{
    LksDirection direction = lks_path_direction(path);
    size_t depth = lks_path_depth(path), length = 1, previous = 0, index;
    if (direction == LKS_DIRECTION_ZERO) {
        if (depth != 0) return LKS_STATUS_INTERNAL_ERROR;
        *out_length = sizeof(zero_path_text) - 1;
        return LKS_STATUS_OK;
    }
    if ((direction != LKS_DIRECTION_POSITIVE &&
         direction != LKS_DIRECTION_NEGATIVE) || depth == 0)
        return LKS_STATUS_INTERNAL_ERROR;
    for (index = 0; index < depth; ++index) {
        unsigned int slot;
        size_t level, level_delta, decimal_prefix_length = 0;
        if (lks_path_get_slot(path, index, &slot) != LKS_STATUS_OK ||
            slot > LKS_PATH_SLOT_MAX ||
            lks_path_get_level(path, index, &level) != LKS_STATUS_OK ||
            (index != 0 && level <= previous))
            return LKS_STATUS_INTERNAL_ERROR;
        level_delta = index == 0 ? level : level - previous;
        if ((index == 0 && level_delta != 0) ||
            (index != 0 && level_delta > 1))
            decimal_prefix_length = decimal_digits(level_delta);
        if (!add_length(&length, LKS_SLOT_TEXT_WIDTH + (index != 0)) ||
            !add_length(&length, decimal_prefix_length))
            return LKS_STATUS_INTERNAL_ERROR;
        previous = level;
    }
    *out_length = length;
    return LKS_STATUS_OK;
}

size_t lks_path_text_length(const LksPath *path)
{
    size_t length;
    return path != NULL && measure_path_text(path, &length) == LKS_STATUS_OK ?
        length : 0;
}

static size_t write_decimal_level(char *buffer, size_t index, size_t level_value)
{
    size_t digits = decimal_digits(level_value), cursor = index + digits;
    do {
        buffer[--cursor] = (char)('0' + level_value % 10);
        level_value /= 10;
    } while (level_value != 0);
    return index + digits;
}

LksStatus lks_path_format(const LksPath *path, char *buffer, size_t buffer_size)
{
    LksDirection direction;
    size_t required_length, depth, index, output_index = 0, previous_level = 0;
    LksStatus status;
    if (path == NULL || buffer == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    /* Validate and bound the entire no-allocation output before writing. */
    status = measure_path_text(path, &required_length);
    if (status != LKS_STATUS_OK) return status;
    if (required_length == (size_t)-1 || buffer_size < required_length + 1)
        return LKS_STATUS_BUFFER_TOO_SMALL;
    direction = lks_path_direction(path);
    if (direction == LKS_DIRECTION_ZERO) {
        memcpy(buffer, zero_path_text, sizeof(zero_path_text));
        return LKS_STATUS_OK;
    }
    buffer[output_index++] = direction == LKS_DIRECTION_POSITIVE ? '0' : '1';
    depth = lks_path_depth(path);
    for (index = 0; index < depth; ++index) {
        size_t level, level_delta, digit_index;
        unsigned int slot;
        char token[LKS_SLOT_TEXT_WIDTH];
        if (lks_path_get_level(path, index, &level) != LKS_STATUS_OK ||
            lks_path_get_slot(path, index, &slot) != LKS_STATUS_OK ||
            !lks_slot_encode(slot, token))
            return LKS_STATUS_INTERNAL_ERROR;
        if (index != 0) buffer[output_index++] = '/';
        level_delta = index == 0 ? level : level - previous_level;
        if ((index == 0 && level_delta != 0) ||
            (index != 0 && level_delta > 1))
            output_index = write_decimal_level(buffer, output_index, level_delta);
        for (digit_index = 0; digit_index < LKS_SLOT_TEXT_WIDTH; ++digit_index)
            buffer[output_index++] = token[digit_index];
        previous_level = level;
    }
    buffer[output_index] = '\0';
    return output_index == required_length ? LKS_STATUS_OK :
        LKS_STATUS_INTERNAL_ERROR;
}
