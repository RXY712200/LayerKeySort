#include <stdlib.h>
#include <string.h>
#include "hps.h"
#include "hps_alloc_internal.h"
#include "hps_path_internal.h"

struct HpsPath {
    HpsDirection direction;
    unsigned char *storage;
    size_t depth;
    size_t capacity;
};

static HpsStatus path_storage_layout(size_t capacity, size_t *levels_offset,
    size_t *total_bytes)
{
    size_t slot_bytes, level_bytes, alignment, remainder, padding, offset;
    if (levels_offset == NULL || total_bytes == NULL) return HPS_STATUS_INVALID_ARGUMENT;
    *levels_offset = 0;
    *total_bytes = 0;
    if (capacity == 0) return HPS_STATUS_OK;
    if (capacity > (size_t)-1 / sizeof(unsigned short) ||
        capacity > (size_t)-1 / sizeof(size_t)) return HPS_STATUS_OUT_OF_MEMORY;
    slot_bytes = capacity * sizeof(unsigned short);
    level_bytes = capacity * sizeof(size_t);
    alignment = _Alignof(size_t);
    if (alignment == 0) return HPS_STATUS_OUT_OF_MEMORY;
    remainder = slot_bytes % alignment;
    padding = remainder == 0 ? 0 : alignment - remainder;
    if (slot_bytes > (size_t)-1 - padding) return HPS_STATUS_OUT_OF_MEMORY;
    offset = slot_bytes + padding;
    if (offset > (size_t)-1 - level_bytes) return HPS_STATUS_OUT_OF_MEMORY;
    *levels_offset = offset;
    *total_bytes = offset + level_bytes;
    return HPS_STATUS_OK;
}

static unsigned short *path_slots(HpsPath *path)
{
    return path == NULL || path->storage == NULL ? NULL : (unsigned short *)(void *)path->storage;
}

static const unsigned short *path_slots_const(const HpsPath *path)
{
    return path == NULL || path->storage == NULL ? NULL : (const unsigned short *)(const void *)path->storage;
}

static size_t *path_levels(HpsPath *path)
{
    size_t offset, total;
    if (path == NULL || path->storage == NULL ||
        path_storage_layout(path->capacity, &offset, &total) != HPS_STATUS_OK) return NULL;
    (void)total;
    return (size_t *)(void *)(path->storage + offset);
}

static const size_t *path_levels_const(const HpsPath *path)
{
    size_t offset, total;
    if (path == NULL || path->storage == NULL ||
        path_storage_layout(path->capacity, &offset, &total) != HPS_STATUS_OK) return NULL;
    (void)total;
    return (const size_t *)(const void *)(path->storage + offset);
}

size_t hps_path_internal_capacity(const HpsPath *path)
{
    return path == NULL ? 0 : path->capacity;
}

size_t hps_path_internal_sizeof_path(void) { return sizeof(HpsPath); }
size_t hps_path_internal_alignof_path(void) { return _Alignof(HpsPath); }

size_t hps_path_internal_storage_bytes_for_capacity(size_t capacity)
{
    size_t offset, total;
    return path_storage_layout(capacity, &offset, &total) == HPS_STATUS_OK ? total : (size_t)-1;
}

size_t hps_path_internal_levels_offset_for_capacity(size_t capacity)
{
    size_t offset, total;
    return path_storage_layout(capacity, &offset, &total) == HPS_STATUS_OK ? offset : (size_t)-1;
}

size_t hps_path_internal_storage_bytes(const HpsPath *path)
{
    return path == NULL ? 0 : hps_path_internal_storage_bytes_for_capacity(path->capacity);
}

static HpsStatus hps_path_reserve_one(HpsPath *path)
{
    size_t required_capacity, new_capacity, old_offset, old_total, new_offset, new_total;
    unsigned char *new_storage;
    HpsStatus status;
    if (path->depth == (size_t)-1) return HPS_STATUS_OUT_OF_MEMORY;
    required_capacity = path->depth + 1;
    if (required_capacity <= path->capacity) return HPS_STATUS_OK;
    new_capacity = path->capacity == 0 ? 1 : path->capacity;
    while (new_capacity < required_capacity) {
        if (new_capacity > (size_t)-1 / 2) { new_capacity = required_capacity; break; }
        new_capacity *= 2;
    }
    status = path_storage_layout(path->capacity, &old_offset, &old_total);
    if (status != HPS_STATUS_OK) return status;
    status = path_storage_layout(new_capacity, &new_offset, &new_total);
    if (status != HPS_STATUS_OK) return status;
    new_storage = (unsigned char *)hps_realloc(path->storage, new_total);
    if (new_storage == NULL) return HPS_STATUS_OUT_OF_MEMORY;
    if (path->depth > 0 && old_offset != new_offset) {
        memmove(new_storage + new_offset, new_storage + old_offset,
            path->depth * sizeof(size_t));
    }
    path->storage = new_storage;
    path->capacity = new_capacity;
    (void)old_total;
    return HPS_STATUS_OK;
}

HpsPath *hps_path_create_zero(void)
{
    HpsPath *path = (HpsPath *)hps_alloc_tagged(sizeof(*path), HPS_ALLOC_TAG_PATH_OBJECT);
    if (path == NULL) return NULL;
    path->direction = HPS_DIRECTION_ZERO;
    path->storage = NULL;
    path->depth = 0;
    path->capacity = 0;
    return path;
}

HpsPath *hps_path_create(HpsDirection direction, unsigned int first_slot)
{
    return hps_path_create_at_level(direction, first_slot, 0);
}

HpsPath *hps_path_create_at_level(HpsDirection direction, unsigned int first_slot, size_t level)
{
    HpsPath *path;
    size_t offset, bytes;
    if ((direction != HPS_DIRECTION_POSITIVE && direction != HPS_DIRECTION_NEGATIVE) ||
        first_slot < HPS_PATH_SLOT_MIN || first_slot > HPS_PATH_SLOT_MAX ||
        path_storage_layout(1, &offset, &bytes) != HPS_STATUS_OK) return NULL;
    path = (HpsPath *)hps_alloc_tagged(sizeof(*path), HPS_ALLOC_TAG_PATH_OBJECT);
    if (path == NULL) return NULL;
    path->storage = (unsigned char *)hps_alloc_tagged(bytes, HPS_ALLOC_TAG_PATH_STEPS);
    if (path->storage == NULL) { hps_free(path); return NULL; }
    path->direction = direction;
    path->depth = 1;
    path->capacity = 1;
    path_slots(path)[0] = (unsigned short)first_slot;
    path_levels(path)[0] = level;
    return path;
}

HpsPath *hps_path_clone(const HpsPath *source)
{
    HpsPath *copy;
    size_t offset, bytes;
    if (source == NULL) return NULL;
    copy = (HpsPath *)hps_alloc_tagged(sizeof(*copy), HPS_ALLOC_TAG_PATH_OBJECT);
    if (copy == NULL) return NULL;
    copy->direction = source->direction;
    copy->storage = NULL;
    copy->depth = source->depth;
    copy->capacity = source->depth;
    if (source->depth > 0) {
        if (path_storage_layout(source->depth, &offset, &bytes) != HPS_STATUS_OK) {
            hps_free(copy); return NULL;
        }
        copy->storage = (unsigned char *)hps_alloc_tagged(bytes, HPS_ALLOC_TAG_PATH_STEPS);
        if (copy->storage == NULL) { hps_free(copy); return NULL; }
        memcpy(path_slots(copy), path_slots_const(source), source->depth * sizeof(unsigned short));
        memcpy(path_levels(copy), path_levels_const(source), source->depth * sizeof(size_t));
    }
    (void)offset;
    return copy;
}

HpsStatus hps_path_append_at_level(HpsPath *path, unsigned int slot, size_t level)
{
    HpsStatus status;
    size_t depth;
    if (path == NULL || path->direction == HPS_DIRECTION_ZERO ||
        slot < HPS_PATH_SLOT_MIN || slot > HPS_PATH_SLOT_MAX || path->depth == 0 ||
        level <= path_levels(path)[path->depth - 1]) return HPS_STATUS_INVALID_ARGUMENT;
    status = hps_path_reserve_one(path);
    if (status != HPS_STATUS_OK) return status;
    depth = path->depth;
    path_slots(path)[depth] = (unsigned short)slot;
    path_levels(path)[depth] = level;
    ++path->depth;
    return HPS_STATUS_OK;
}

HpsStatus hps_path_append(HpsPath *path, unsigned int slot)
{
    size_t last_level;
    if (path == NULL || path->direction == HPS_DIRECTION_ZERO ||
        slot < HPS_PATH_SLOT_MIN || slot > HPS_PATH_SLOT_MAX || path->depth == 0)
        return HPS_STATUS_INVALID_ARGUMENT;
    last_level = path_levels(path)[path->depth - 1];
    if (last_level == (size_t)-1) return HPS_STATUS_INVALID_ARGUMENT;
    return hps_path_append_at_level(path, slot, last_level + 1);
}

HpsDirection hps_path_direction(const HpsPath *path) { return path == NULL ? HPS_DIRECTION_ZERO : path->direction; }
size_t hps_path_depth(const HpsPath *path) { return path == NULL ? 0 : path->depth; }

HpsStatus hps_path_get_slot(const HpsPath *path, size_t index, unsigned int *out_slot)
{
    if (path == NULL || out_slot == NULL || index >= path->depth) return HPS_STATUS_INVALID_ARGUMENT;
    *out_slot = (unsigned int)path_slots_const(path)[index];
    return HPS_STATUS_OK;
}

HpsStatus hps_path_get_level(const HpsPath *path, size_t index, size_t *out_level)
{
    if (path == NULL || out_level == NULL || index >= path->depth) return HPS_STATUS_INVALID_ARGUMENT;
    *out_level = path_levels_const(path)[index];
    return HPS_STATUS_OK;
}

void hps_path_destroy(HpsPath *path)
{
    if (path == NULL) return;
    hps_free(path->storage);
    hps_free(path);
}
