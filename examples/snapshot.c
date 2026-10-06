/* Public API only: historical business IDs, not serialized live pointers. */
#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Task { const char *id; const char *title; } Task;
static LksStatus association(void *item, uint64_t ordinal, void *context,
    const void **data, size_t *size)
{
    Task *task = (Task *)item; (void)ordinal; (void)context;
    *data = task->id; *size = strlen(task->id); return LKS_STATUS_OK;
}
static LksStatus resolve(const void *data, size_t size, uint64_t ordinal,
    void *context, void **item)
{
    Task *tasks = (Task *)context; size_t i; (void)ordinal;
    *item = NULL;
    for (i = 0; i < 3; ++i)
        if (strlen(tasks[i].id) == size && !memcmp(tasks[i].id, data, size)) {
            *item = &tasks[i]; return LKS_STATUS_OK;
        }
    return LKS_STATUS_NOT_FOUND;
}
int main(void)
{
    Task tasks[] = {{"a", "draft"}, {"b", "review"}, {"c", "publish"}};
    const unsigned char domain[] = {0x20, 0x26, 0x10, 0x06};
    LksSnapshotOptions options = {domain, sizeof(domain), association, NULL};
    LksOrder *order = lks_order_create(), *restored = NULL;
    LksSnapshot *snapshot = NULL, *loaded = NULL;
    const LksOrderHandle *handle = NULL, *first = NULL;
    unsigned char *blob = NULL; size_t i, size; char key[64]; int result = 1;
    if (!order) goto done;
    for (i = 0; i < 3; ++i) {
        if (lks_order_insert_back(order, &tasks[i], &handle) != LKS_STATUS_OK) goto done;
        if (!i) first = handle;
    }
    if (lks_order_snapshot_capture(order, &options, &snapshot) != LKS_STATUS_OK) goto done;
    for (i = 0; i < lks_snapshot_count(snapshot); ++i) {
        if (lks_snapshot_key_format(snapshot, i, key, sizeof(key)) != LKS_STATUS_OK) goto done;
        puts(key);
    }
    size = lks_snapshot_serialized_size(snapshot); blob = (unsigned char *)malloc(size);
    if (!blob || lks_snapshot_serialize(snapshot, blob, size) != LKS_STATUS_OK) goto done;
    if (lks_order_move_back(order, first) != LKS_STATUS_OK ||
        lks_order_snapshot_is_current(order, snapshot)) goto done;
    lks_order_destroy(order); order = NULL;
    puts("Historical snapshot survives source mutation and destruction:");
    for (i = 0; i < lks_snapshot_count(snapshot); ++i) {
        const void *data; size_t n;
        if (lks_snapshot_association(snapshot, i, &data, &n) != LKS_STATUS_OK) goto done;
        printf("row %zu: %.*s\n", i, (int)n, (const char *)data);
    }
    if (lks_snapshot_deserialize(blob, size, &loaded) != LKS_STATUS_OK ||
        lks_snapshot_restore_order(loaded, resolve, tasks, &restored) != LKS_STATUS_OK ||
        lks_order_snapshot_is_current(restored, loaded)) goto done;
    for (handle = lks_order_first(restored); handle; handle = lks_order_next(handle))
        puts(((Task *)lks_order_item(handle))->title);
    result = 0;
done:
    free(blob); lks_order_destroy(order); lks_order_destroy(restored);
    lks_snapshot_destroy(snapshot); lks_snapshot_destroy(loaded); return result;
}
