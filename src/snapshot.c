#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h>
struct LksSourceMarker { volatile long refs; };
#define SNAP_REF_MAX ((size_t)LONG_MAX)
#else
#include <stdatomic.h>
struct LksSourceMarker { atomic_size_t refs; };
#define SNAP_REF_MAX SIZE_MAX
#endif

static LksSourceMarker *snap_marker_new(void)
{
    LksSourceMarker *m = (LksSourceMarker *)lks_alloc(sizeof(*m));
    if (m) {
#if defined(_MSC_VER)
        m->refs = 2;
#else
        atomic_init(&m->refs, 2);
#endif
    }
    return m;
}
static int snap_marker_retain(LksSourceMarker *m)
{
#if defined(_MSC_VER)
    long old = _InterlockedCompareExchange(&m->refs, 0, 0);
    for (;;) {
        long found;
        if ((size_t)old == SNAP_REF_MAX) return 0;
        found = _InterlockedCompareExchange(&m->refs, old + 1, old);
        if (found == old) return 1;
        old = found;
    }
#else
    size_t old = atomic_load_explicit(&m->refs, memory_order_relaxed);
    do { if (old == SNAP_REF_MAX) return 0; }
    while (!atomic_compare_exchange_weak_explicit(&m->refs, &old, old + 1,
        memory_order_relaxed, memory_order_relaxed));
    return 1;
#endif
}
void lks_source_marker_release(LksSourceMarker *m)
{
    if (!m) return;
#if defined(_MSC_VER)
    if (_InterlockedDecrement(&m->refs) == 0) lks_free(m);
#else
    if (atomic_fetch_sub_explicit(&m->refs, 1, memory_order_acq_rel) == 1) lks_free(m);
#endif
}
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
void lks_source_marker_test_refs(LksSourceMarker *m, size_t refs)
{
#if defined(_MSC_VER)
    (void)_InterlockedExchange(&m->refs, (long)refs);
#else
    atomic_store_explicit(&m->refs, refs, memory_order_relaxed);
#endif
}
#endif

void lks_snapshot_destroy(LksSnapshot *s)
{
    if (!s) return;
    lks_source_marker_release(s->marker);
    lks_free(s->associations); lks_free(s->namespace_data);
    lks_free(s->rows); lks_free(s);
}
LksStatus lks_snapshot_new(size_t count, const void *domain, size_t domain_size,
    LksSnapshot **out)
{
    LksSnapshot *s;
    *out = NULL;
    /* Reserve arithmetic for caller-buffer wire/key queries as well. */
    if (!domain || !domain_size || domain_size > UINT32_MAX ||
        domain_size > (SIZE_MAX-22)/2 || count > UINT64_MAX ||
        count > SIZE_MAX/sizeof(LksSnapshotRow) ||
        domain_size > SIZE_MAX-24 || count > (SIZE_MAX-24-domain_size)/8)
        return LKS_STATUS_INVALID_ARGUMENT;
    s = (LksSnapshot *)lks_alloc(sizeof(*s));
    if (!s) return LKS_STATUS_OUT_OF_MEMORY;
    memset(s, 0, sizeof(*s)); s->count = count; s->namespace_size = domain_size;
    s->namespace_data = (unsigned char *)lks_alloc(domain_size);
    if (!s->namespace_data) goto oom;
    memcpy(s->namespace_data, domain, domain_size);
    if (count) {
        s->rows = (LksSnapshotRow *)lks_alloc(count*sizeof(*s->rows));
        if (!s->rows) goto oom;
        memset(s->rows, 0, count*sizeof(*s->rows));
    }
    *out = s; return LKS_STATUS_OK;
oom:
    lks_snapshot_destroy(s); return LKS_STATUS_OUT_OF_MEMORY;
}
static LksStatus snap_append(LksSnapshot *s, size_t row, const void *data, size_t size)
{
    size_t need, limit = SIZE_MAX-24-s->namespace_size-8*s->count;
    if ((size && !data) || size > UINT64_MAX || size > limit-s->association_size)
        return LKS_STATUS_INVALID_ARGUMENT;
    need = s->association_size + size;
    if (need > s->association_capacity) {
        size_t cap = s->association_capacity ? s->association_capacity : 64;
        unsigned char *p;
        if (cap > limit) cap = need;
        while (cap < need) {
            if (cap > limit/2) { cap = need; break; }
            cap *= 2;
        }
        p = (unsigned char *)lks_realloc(s->associations, cap);
        if (!p) return LKS_STATUS_OUT_OF_MEMORY;
        s->associations = p; s->association_capacity = cap;
    }
    s->rows[row].offset = s->association_size; s->rows[row].size = size;
    if (size) memcpy(s->associations+s->association_size, data, size);
    s->association_size = need; return LKS_STATUS_OK;
}
LksStatus lks_snapshot_capture_sequence(size_t count, LksSnapshotNextItem next,
    void *iterator, int *busy, LksSourceMarker **marker, uint64_t revision,
    const LksSnapshotOptions *options, LksSnapshot **out)
{
    LksSnapshot *s; LksStatus status; size_t row;
    LksSnapshotOptions descriptor;
    if (out) *out = NULL;
    if (!options || !out) return LKS_STATUS_INVALID_ARGUMENT;
    if (*busy) return LKS_STATUS_REENTRANT;
    descriptor = *options; options = &descriptor;
    status = lks_snapshot_new(count, options->namespace_data, options->namespace_size, &s);
    if (status != LKS_STATUS_OK) return status;
    for (row = 0; row < count; ++row) {
        void *item = next(iterator); const void *data = NULL; size_t size = 0;
        if (options->association) {
            *busy = 1;
            status = options->association(item, (uint64_t)row, options->context, &data, &size);
            *busy = 0;
            if (status != LKS_STATUS_OK) goto fail;
        }
        status = snap_append(s, row, data, size);
        if (status != LKS_STATUS_OK) goto fail;
    }
    /* Commit provenance only after every row is ready. */
    if (*marker) {
        if (!snap_marker_retain(*marker)) { status = LKS_STATUS_CAPACITY_LIMIT; goto fail; }
        s->marker = *marker;
    } else {
        s->marker = snap_marker_new();
        if (!s->marker) { status = LKS_STATUS_OUT_OF_MEMORY; goto fail; }
        *marker = s->marker;
    }
    s->revision = revision; *out = s; return LKS_STATUS_OK;
fail:
    lks_snapshot_destroy(s); return status;
}
typedef struct SnapLiveIterator { LksOrderBlock *block; size_t local; } SnapLiveIterator;
static void *snap_live_next(void *context)
{
    SnapLiveIterator *it = (SnapLiveIterator *)context;
    void *item = it->block->records[it->local++]->item;
    if (it->local == it->block->count) { it->block = it->block->next; it->local = 0; }
    return item;
}
LksStatus lks_order_snapshot_capture(LksOrder *o, const LksSnapshotOptions *options,
    LksSnapshot **out)
{
    SnapLiveIterator it;
    if (out) *out = NULL;
    if (!o || !out) return LKS_STATUS_INVALID_ARGUMENT;
    it.block = o->first; it.local = 0;
    return lks_snapshot_capture_sequence(o->count, snap_live_next, &it,
        &o->callback_active, &o->source_marker, o->revision, options, out);
}
LksStatus lks_managed_order_snapshot_capture(LksManagedOrder *o,
    const LksSnapshotOptions *options, LksSnapshot **out)
{
    LksStatus status;
    if (out) *out = NULL;
    if (!o || !out) return LKS_STATUS_INVALID_ARGUMENT;
    if (o->active) return LKS_STATUS_REENTRANT;
    o->active = 1; status = lks_order_snapshot_capture(o->core, options, out);
    o->active = 0; return status;
}
int lks_order_snapshot_is_current(const LksOrder *o, const LksSnapshot *s)
{ return o && s && !o->callback_active && s->marker &&
    o->source_marker == s->marker && o->revision == s->revision; }
int lks_managed_order_snapshot_is_current(const LksManagedOrder *o, const LksSnapshot *s)
{ return o && !o->active && lks_order_snapshot_is_current(o->core, s); }
size_t lks_snapshot_count(const LksSnapshot *s) { return s ? s->count : 0; }
LksStatus lks_snapshot_namespace(const LksSnapshot *s, const void **data, size_t *size)
{
    if (data) *data = NULL;
    if (size) *size = 0;
    if (!s || !data || !size) return LKS_STATUS_INVALID_ARGUMENT;
    *data = s->namespace_data; *size = s->namespace_size; return LKS_STATUS_OK;
}
LksStatus lks_snapshot_association(const LksSnapshot *s, size_t row,
    const void **data, size_t *size)
{
    if (data) *data = NULL;
    if (size) *size = 0;
    if (!s || !data || !size || row >= s->count) return LKS_STATUS_INVALID_ARGUMENT;
    *size = s->rows[row].size;
    *data = *size ? s->associations+s->rows[row].offset : NULL;
    return LKS_STATUS_OK;
}
