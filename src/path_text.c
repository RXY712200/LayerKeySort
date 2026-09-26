#include "hps.h"

static int hps_slot_to_chars(unsigned int slot, char *letter, char *digit)
{
    unsigned int letter_index;
    unsigned int digit_index;

    if (slot > HPS_PATH_SLOT_MAX) {
        return 0;
    }

    letter_index = slot / 10u;
    digit_index = slot % 10u;
    *letter = (char)('A' + letter_index);
    *digit = (char)('0' + digit_index);
    return 1;
}

static HpsStatus hps_path_measure_text(
    const HpsPath *path,
    size_t *out_length
)
{
    HpsDirection direction;
    size_t depth;
    size_t index;
    size_t level;
    size_t previous_level;
    size_t first_level;
    size_t length;
    unsigned int slot;

    direction = hps_path_direction(path);
    depth = hps_path_depth(path);

    if (direction == HPS_DIRECTION_ZERO) {
        if (depth != 0) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        *out_length = 3;
        return HPS_STATUS_OK;
    }
    if ((direction != HPS_DIRECTION_POSITIVE &&
         direction != HPS_DIRECTION_NEGATIVE) || depth == 0) {
        return HPS_STATUS_INTERNAL_ERROR;
    }
    if (hps_path_get_level(path, 0, &first_level) != HPS_STATUS_OK) {
        return HPS_STATUS_INTERNAL_ERROR;
    }

    length = (direction == HPS_DIRECTION_POSITIVE && first_level > 0) ? 3 : 1;
    previous_level = first_level;
    for (index = 0; index < depth; ++index) {
        if (hps_path_get_slot(path, index, &slot) != HPS_STATUS_OK ||
            slot > HPS_PATH_SLOT_MAX ||
            hps_path_get_level(path, index, &level) != HPS_STATUS_OK) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (index == 0 && level != first_level) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (index > 0 && level <= previous_level) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        if (length > (size_t)-1 - 2) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        length += 2;
        if (length > (size_t)-1 - level) {
            return HPS_STATUS_INTERNAL_ERROR;
        }
        length += level;
        previous_level = level;
    }

    *out_length = length;
    return HPS_STATUS_OK;
}

size_t hps_path_text_length(const HpsPath *path)
{
    size_t length;

    if (path == NULL ||
        hps_path_measure_text(path, &length) != HPS_STATUS_OK) {
        return 0;
    }
    return length;
}

HpsStatus hps_path_format(
    const HpsPath *path,
    char *buffer,
    size_t buffer_size
)
{
    HpsDirection direction;
    size_t depth;
    size_t required_length;
    size_t output_index;
    size_t slot_index;
    size_t slash_index;
    size_t level;
    unsigned int slot;
    char letter;
    char digit;
    HpsStatus status;

    if (path == NULL || buffer == NULL) {
        return HPS_STATUS_INVALID_ARGUMENT;
    }

    status = hps_path_measure_text(path, &required_length);
    if (status != HPS_STATUS_OK) {
        return status;
    }
    if (required_length == (size_t)-1 ||
        buffer_size < required_length + 1) {
        return HPS_STATUS_BUFFER_TOO_SMALL;
    }

    direction = hps_path_direction(path);
    depth = hps_path_depth(path);
    if (direction == HPS_DIRECTION_ZERO) {
        buffer[0] = '0';
        buffer[1] = '0';
        buffer[2] = '0';
        buffer[3] = '\0';
        return HPS_STATUS_OK;
    }

    output_index = 0;
    if (direction == HPS_DIRECTION_POSITIVE &&
        hps_path_get_level(path, 0, &level) == HPS_STATUS_OK && level > 0) {
        buffer[output_index++] = '0';
        buffer[output_index++] = '0';
        buffer[output_index++] = '0';
    } else {
        buffer[output_index++] =
            (direction == HPS_DIRECTION_POSITIVE) ? '0' : '1';
    }

    for (slot_index = 0; slot_index < depth; ++slot_index) {
        if (hps_path_get_slot(path, slot_index, &slot) != HPS_STATUS_OK ||
            hps_path_get_level(path, slot_index, &level) != HPS_STATUS_OK ||
            !hps_slot_to_chars(slot, &letter, &digit)) {
            return HPS_STATUS_INTERNAL_ERROR;
        }

        for (slash_index = 0; slash_index < level; ++slash_index) {
            buffer[output_index++] = '/';
        }
        buffer[output_index++] = letter;
        buffer[output_index++] = digit;
    }

    buffer[output_index] = '\0';
    return HPS_STATUS_OK;
}
