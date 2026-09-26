#include "layerkeysort.h"

static int lks_slot_to_chars(unsigned int slot, char *letter, char *digit)
{
    unsigned int letter_index;
    unsigned int digit_index;

    if (slot > LKS_PATH_SLOT_MAX) {
        return 0;
    }

    letter_index = slot / 10u;
    digit_index = slot % 10u;
    *letter = (char)('A' + letter_index);
    *digit = (char)('0' + digit_index);
    return 1;
}

static LksStatus lks_path_measure_text(
    const LksPath *path,
    size_t *out_length
)
{
    LksDirection direction;
    size_t depth;
    size_t index;
    size_t level;
    size_t previous_level;
    size_t first_level;
    size_t length;
    unsigned int slot;

    direction = lks_path_direction(path);
    depth = lks_path_depth(path);

    if (direction == LKS_DIRECTION_ZERO) {
        if (depth != 0) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        *out_length = 3;
        return LKS_STATUS_OK;
    }
    if ((direction != LKS_DIRECTION_POSITIVE &&
         direction != LKS_DIRECTION_NEGATIVE) || depth == 0) {
        return LKS_STATUS_INTERNAL_ERROR;
    }
    if (lks_path_get_level(path, 0, &first_level) != LKS_STATUS_OK) {
        return LKS_STATUS_INTERNAL_ERROR;
    }

    length = (direction == LKS_DIRECTION_POSITIVE && first_level > 0) ? 3 : 1;
    previous_level = first_level;
    for (index = 0; index < depth; ++index) {
        if (lks_path_get_slot(path, index, &slot) != LKS_STATUS_OK ||
            slot > LKS_PATH_SLOT_MAX ||
            lks_path_get_level(path, index, &level) != LKS_STATUS_OK) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (index == 0 && level != first_level) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (index > 0 && level <= previous_level) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        if (length > (size_t)-1 - 2) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        length += 2;
        if (length > (size_t)-1 - level) {
            return LKS_STATUS_INTERNAL_ERROR;
        }
        length += level;
        previous_level = level;
    }

    *out_length = length;
    return LKS_STATUS_OK;
}

size_t lks_path_text_length(const LksPath *path)
{
    size_t length;

    if (path == NULL ||
        lks_path_measure_text(path, &length) != LKS_STATUS_OK) {
        return 0;
    }
    return length;
}

LksStatus lks_path_format(
    const LksPath *path,
    char *buffer,
    size_t buffer_size
)
{
    LksDirection direction;
    size_t depth;
    size_t required_length;
    size_t output_index;
    size_t slot_index;
    size_t slash_index;
    size_t level;
    unsigned int slot;
    char letter;
    char digit;
    LksStatus status;

    if (path == NULL || buffer == NULL) {
        return LKS_STATUS_INVALID_ARGUMENT;
    }

    status = lks_path_measure_text(path, &required_length);
    if (status != LKS_STATUS_OK) {
        return status;
    }
    if (required_length == (size_t)-1 ||
        buffer_size < required_length + 1) {
        return LKS_STATUS_BUFFER_TOO_SMALL;
    }

    direction = lks_path_direction(path);
    depth = lks_path_depth(path);
    if (direction == LKS_DIRECTION_ZERO) {
        buffer[0] = '0';
        buffer[1] = '0';
        buffer[2] = '0';
        buffer[3] = '\0';
        return LKS_STATUS_OK;
    }

    output_index = 0;
    if (direction == LKS_DIRECTION_POSITIVE &&
        lks_path_get_level(path, 0, &level) == LKS_STATUS_OK && level > 0) {
        buffer[output_index++] = '0';
        buffer[output_index++] = '0';
        buffer[output_index++] = '0';
    } else {
        buffer[output_index++] =
            (direction == LKS_DIRECTION_POSITIVE) ? '0' : '1';
    }

    for (slot_index = 0; slot_index < depth; ++slot_index) {
        if (lks_path_get_slot(path, slot_index, &slot) != LKS_STATUS_OK ||
            lks_path_get_level(path, slot_index, &level) != LKS_STATUS_OK ||
            !lks_slot_to_chars(slot, &letter, &digit)) {
            return LKS_STATUS_INTERNAL_ERROR;
        }

        for (slash_index = 0; slash_index < level; ++slash_index) {
            buffer[output_index++] = '/';
        }
        buffer[output_index++] = letter;
        buffer[output_index++] = digit;
    }

    buffer[output_index] = '\0';
    return LKS_STATUS_OK;
}
