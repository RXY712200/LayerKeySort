#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"

LksStatus lks_snapshot_restore_order(const LksSnapshot *s,
    LksSnapshotResolveFn resolve, void *context, LksOrder **out)
{
    void **items = NULL; size_t i; LksStatus status;
    if (out) *out = NULL;
    if (!s || !out || (s->count && !resolve) || s->count > SIZE_MAX/sizeof(*items))
        return LKS_STATUS_INVALID_ARGUMENT;
    if (s->count) {
        items = (void **)lks_alloc(s->count*sizeof(*items));
        if (!items) return LKS_STATUS_OUT_OF_MEMORY;
    }
    for (i = 0; i < s->count; ++i) {
        const void *data = s->rows[i].size ? s->associations+s->rows[i].offset : NULL;
        items[i] = NULL;
        status = resolve(data, s->rows[i].size, (uint64_t)i, context, &items[i]);
        if (status != LKS_STATUS_OK) goto done;
        if (!items[i]) { status = LKS_STATUS_INVALID_ARGUMENT; goto done; }
    }
    status = lks_order_bulk_build(items, s->count, out);
done:
    lks_free(items); return status;
}
typedef struct SnapLegacyRow { LksPath *path; void *item; } SnapLegacyRow;
static int snap_legacy_compare(const void *left, const void *right, void *context)
{
    const SnapLegacyRow *a = (const SnapLegacyRow *)left, *b = (const SnapLegacyRow *)right;
    int order = 0;
    (void)context; (void)lks_path_compare(a->path, b->path, &order); return order;
}
LksStatus lks_order_import_v3_lk1(const LksV3Lk1ImportEntry *entries,
    size_t count, LksOrder **out)
{
    SnapLegacyRow *rows = NULL; void **sorted = NULL; size_t i, parsed = 0;
    LksStatus status = LKS_STATUS_OUT_OF_MEMORY;
    if (out) *out = NULL;
    if (!out || (count && !entries) || count > SIZE_MAX/sizeof(*rows) ||
        count > SIZE_MAX/sizeof(*sorted)) return LKS_STATUS_INVALID_ARGUMENT;
    if (!count) return lks_order_bulk_build(NULL, 0, out);
    rows = (SnapLegacyRow *)lks_alloc(count*sizeof(*rows));
    sorted = (void **)lks_alloc(count*sizeof(*sorted));
    if (!rows || !sorted) goto done;
    for (i = 0; i < count; ++i) {
        if (!entries[i].key || !entries[i].item) { status = LKS_STATUS_INVALID_ARGUMENT; goto done; }
        status = lks_path_order_key_parse(entries[i].key, &rows[i].path);
        if (status != LKS_STATUS_OK) goto done;
        ++parsed; rows[i].item = entries[i].item; sorted[i] = &rows[i];
    }
    status = lks_sort(sorted, count, snap_legacy_compare, NULL);
    if (status != LKS_STATUS_OK) goto done;
    for (i = 1; i < count; ++i) {
        if (!snap_legacy_compare(sorted[i-1], sorted[i], NULL)) {
            status = LKS_STATUS_ALREADY_EXISTS; goto done;
        }
    }
    for (i = 0; i < count; ++i) sorted[i] = ((SnapLegacyRow *)sorted[i])->item;
    status = lks_order_bulk_build(sorted, count, out);
done:
    for (i = 0; i < parsed; ++i) lks_path_destroy(rows[i].path);
    lks_free(sorted); lks_free(rows); return status;
}
