#include <stdint.h>
#include <string.h>
#include "layerkeysort.h"

/* LK1 payload: direction, zero or more self-delimiting steps, '!'. The
 * constant family/version prefix has no effect on same-version ordering. */
/* Numeric byte values keep the wire format ASCII even on a host whose C
 * execution character set is not ASCII. */
enum { K_END = 0x21, K_ZERO = 0x30, K_ONE = 0x31, K_TWO = 0x32 };
static const char key_prefix[] = {0x4c, 0x4b, 0x31, 0x3a, 0};
static const char hex_digits[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46
};

static size_t byte_count(size_t value)
{
    size_t count = 1;
    while (value >= 256) { value /= 256; ++count; }
    return count;
}

static int add_size(size_t *total, size_t amount)
{
    if (*total > SIZE_MAX - amount) return 0;
    *total += amount;
    return 1;
}

static LksStatus measure(const LksPath *path, size_t *out_length)
{
    LksDirection direction;
    size_t depth, length = sizeof(key_prefix) - 1 + 2, previous = 0;
    if (path == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    direction = lks_path_direction(path);
    depth = lks_path_depth(path);
    if (direction == LKS_DIRECTION_ZERO) {
        if (depth != 0) return LKS_STATUS_INTERNAL_ERROR;
        *out_length = length;
        return LKS_STATUS_OK;
    }
    if ((direction != LKS_DIRECTION_NEGATIVE &&
         direction != LKS_DIRECTION_POSITIVE) || depth == 0)
        return LKS_STATUS_INTERNAL_ERROR;
    for (size_t i = 0; i < depth; ++i) {
        size_t level, bytes;
        unsigned int slot;
        if (lks_path_get_level(path, i, &level) != LKS_STATUS_OK ||
            lks_path_get_slot(path, i, &slot) != LKS_STATUS_OK ||
            slot > LKS_PATH_SLOT_MAX || (i != 0 && level <= previous))
            return LKS_STATUS_INTERNAL_ERROR;
        bytes = byte_count(level);
        if (bytes > (SIZE_MAX - 5) / 3 || !add_size(&length, 3 * bytes + 5))
            return LKS_STATUS_INTERNAL_ERROR;
        previous = level;
    }
    *out_length = length;
    return LKS_STATUS_OK;
}

size_t lks_path_order_key_length(const LksPath *path)
{
    size_t length;
    return measure(path, &length) == LKS_STATUS_OK ? length : 0;
}

static void write_hex_byte(char *output, unsigned int byte)
{
    output[0] = hex_digits[byte >> 4];
    output[1] = hex_digits[byte & 15u];
}

LksStatus lks_path_order_key_format(const LksPath *path,
    char *buffer, size_t buffer_size)
{
    size_t length, cursor = 0, depth;
    LksDirection direction;
    LksStatus status;
    if (path == NULL || buffer == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    status = measure(path, &length);
    if (status != LKS_STATUS_OK) return status;
    if (length == SIZE_MAX || buffer_size < length + 1)
        return LKS_STATUS_BUFFER_TOO_SMALL;
    memcpy(buffer, key_prefix, sizeof(key_prefix) - 1);
    cursor = sizeof(key_prefix) - 1;
    direction = lks_path_direction(path);
    buffer[cursor++] = direction == LKS_DIRECTION_NEGATIVE ? K_ZERO :
        direction == LKS_DIRECTION_ZERO ? K_ONE : K_TWO;
    depth = lks_path_depth(path);
    for (size_t i = 0; i < depth; ++i) {
        unsigned char bytes[sizeof(size_t)];
        size_t level, count, value;
        unsigned int slot;
        /* MEASURE already checked validity, so these reads cannot fail. */
        (void)lks_path_get_level(path, i, &level);
        (void)lks_path_get_slot(path, i, &slot);
        count = byte_count(level);
        value = level;
        for (size_t j = count; j > 0; --j) {
            bytes[j - 1] = (unsigned char)(value % 256);
            value /= 256;
        }
        for (size_t j = 0; j < count; ++j) buffer[cursor++] = K_ZERO;
        buffer[cursor++] = K_ONE;
        for (size_t j = 0; j < count; ++j) {
            write_hex_byte(buffer + cursor, 255u - bytes[j]);
            cursor += 2;
        }
        if (direction == LKS_DIRECTION_NEGATIVE && i == 0)
            slot = LKS_PATH_SLOT_MAX - slot;
        for (size_t j = 4; j > 0; --j) {
            buffer[cursor + j - 1] = hex_digits[slot & 15u];
            slot >>= 4;
        }
        cursor += 4;
    }
    buffer[cursor++] = K_END;
    buffer[cursor] = '\0';
    return cursor == length ? LKS_STATUS_OK : LKS_STATUS_INTERNAL_ERROR;
}

static int hex_value(char c)
{
    if (c >= 0x30 && c <= 0x39) return c - 0x30;
    if (c >= 0x41 && c <= 0x46) return c - 0x41 + 10;
    return -1;
}

LksStatus lks_path_order_key_parse(const char *key, LksPath **out_path)
{
    LksPath *path = NULL;
    LksDirection direction;
    size_t length, cursor, previous = 0, depth = 0;
    LksStatus status = LKS_STATUS_INVALID_ARGUMENT;
    if (out_path == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    *out_path = NULL;
    if (key == NULL) return LKS_STATUS_INVALID_ARGUMENT;
    length = strlen(key);
    if (length < sizeof(key_prefix) - 1 + 2 ||
        memcmp(key, key_prefix, sizeof(key_prefix) - 1) != 0 ||
        key[length - 1] != K_END) return LKS_STATUS_INVALID_ARGUMENT;
    cursor = sizeof(key_prefix) - 1;
    if (key[cursor] == K_ONE) {
        if (length != sizeof(key_prefix) - 1 + 2) return status;
        path = lks_path_create_zero();
        if (path == NULL) return LKS_STATUS_OUT_OF_MEMORY;
        *out_path = path;
        return LKS_STATUS_OK;
    }
    if (key[cursor] != K_ZERO && key[cursor] != K_TWO) return status;
    direction = key[cursor++] == K_ZERO ? LKS_DIRECTION_NEGATIVE :
        LKS_DIRECTION_POSITIVE;
    if (key[cursor] == K_END) return status;
    while (cursor < length - 1) {
        size_t count = 0, level = 0;
        unsigned int slot = 0;
        while (cursor < length - 1 && key[cursor] == K_ZERO) {
            if (count == sizeof(size_t)) goto fail;
            ++count; ++cursor;
        }
        if (count == 0 || cursor >= length - 1 || key[cursor++] != K_ONE ||
            count > (length - 1 - cursor) / 2 ||
            length - 1 - cursor - 2 * count < 4) goto fail;
        for (size_t i = 0; i < count; ++i) {
            int high = hex_value(key[cursor++]);
            int low = hex_value(key[cursor++]);
            unsigned int byte;
            if (high < 0 || low < 0) goto fail;
            byte = 255u - (unsigned int)(high * 16 + low);
            if (i == 0 && count > 1 && byte == 0) goto fail;
            if (level > (SIZE_MAX - byte) / 256) goto fail;
            level = level * 256 + byte;
        }
        for (size_t i = 0; i < 4; ++i) {
            int digit = hex_value(key[cursor++]);
            if (digit < 0) goto fail;
            slot = slot * 16u + (unsigned int)digit;
        }
        if (direction == LKS_DIRECTION_NEGATIVE && depth == 0)
            slot = LKS_PATH_SLOT_MAX - slot;
        if (depth == 0) {
            path = lks_path_create_at_level(direction, slot, level);
            if (path == NULL) { status = LKS_STATUS_OUT_OF_MEMORY; goto fail; }
        } else {
            if (level <= previous) goto fail;
            status = lks_path_append_at_level(path, slot, level);
            if (status != LKS_STATUS_OK) goto fail;
        }
        previous = level;
        ++depth;
        status = LKS_STATUS_INVALID_ARGUMENT;
    }
    if (cursor != length - 1) goto fail;
    *out_path = path;
    return LKS_STATUS_OK;
fail:
    lks_path_destroy(path);
    return status;
}
