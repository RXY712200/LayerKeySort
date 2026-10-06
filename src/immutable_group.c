#include "lks_immutable_group_internal.h"
#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include "lks_sort_internal.h"
#include <string.h>

static LksImmutableGroup *ig_new(size_t count)
{
    LksImmutableGroup *g;
    if (count > SIZE_MAX/sizeof(void *)) return NULL;
    g = (LksImmutableGroup *)lks_alloc(sizeof(*g));
    if (g) { memset(g, 0, sizeof(*g)); g->count = count; }
    return g;
}
void lks_immutable_group_destroy(LksImmutableGroup *g)
{
    if (!g || g->busy) return;
    lks_source_marker_release(g->marker); lks_free(g->items); lks_free(g);
}
size_t lks_immutable_group_size(const LksImmutableGroup *g)
{ return g && !g->busy ? g->count : 0; }
void *lks_immutable_group_item_at(const LksImmutableGroup *g, size_t at)
{ return g && !g->busy && at < g->count ? g->items[at] : NULL; }
LksStatus lks_immutable_group_build(void *const *items, size_t count,
    const LksComparator *comparator, LksImmutableGroup **out)
{
    LksImmutableGroup *g; LksStatus status; size_t i; LksComparator descriptor;
    if (out) *out = NULL;
    if (!out || !comparator || !comparator->compare || (count && !items) ||
        count > SIZE_MAX/sizeof(void *)) return LKS_STATUS_INVALID_ARGUMENT;
    for (i = 0; i < count; ++i) if (!items[i]) return LKS_STATUS_INVALID_ARGUMENT;
    descriptor = *comparator;
    g = ig_new(count); if (!g) return LKS_STATUS_OUT_OF_MEMORY;
    status = lks_stable_sort_copy(items, count, &descriptor, &g->items);
    if (status != LKS_STATUS_OK) { lks_immutable_group_destroy(g); return status; }
    *out = g; return LKS_STATUS_OK;
}
LksStatus lks_immutable_group_merge(const LksImmutableGroup *base,
    const LksImmutableGroup *incoming, const LksComparator *comparator,
    LksImmutableGroup **out)
{
    LksImmutableGroup *g, *a = (LksImmutableGroup *)base, *b = (LksImmutableGroup *)incoming;
    size_t count; LksComparator descriptor;
    if (out) *out = NULL;
    if (!a || !b || !out || !comparator || !comparator->compare) return LKS_STATUS_INVALID_ARGUMENT;
    if (a->busy || b->busy) return LKS_STATUS_REENTRANT;
    if (a->count > SIZE_MAX-b->count || a->count+b->count > SIZE_MAX/sizeof(void *))
        return LKS_STATUS_INVALID_ARGUMENT;
    count = a->count+b->count; descriptor = *comparator;
    g = ig_new(count); if (!g) return LKS_STATUS_OUT_OF_MEMORY;
    if (count) {
        g->items = (void **)lks_alloc(count*sizeof(*g->items));
        if (!g->items) { lks_immutable_group_destroy(g); return LKS_STATUS_OUT_OF_MEMORY; }
    }
    a->busy = b->busy = 1;
    lks_merge_sorted_pointers(a->items, a->count, b->items, b->count, &descriptor, g->items);
    a->busy = b->busy = 0; *out = g; return LKS_STATUS_OK;
}
void lks_immutable_group_batch_destroy(LksImmutableGroupBatch *b)
{
    size_t i;
    if (!b || b->busy) return;
    /* A borrowed chunk may currently be executing an association callback. */
    for (i = 0; i < b->group_count; ++i) if (b->groups[i]->busy) return;
    for (i = 0; i < b->group_count; ++i) lks_immutable_group_destroy(b->groups[i]);
    lks_free(b->groups); lks_free(b);
}
LksStatus lks_immutable_group_batch_build(void *const *items, size_t count,
    size_t group_size, const LksComparator *comparator, LksImmutableGroupBatch **out)
{
    LksImmutableGroupBatch *b; size_t groups, i, at = 0; LksStatus status;
    LksComparator descriptor;
    if (out) *out = NULL;
    if (!out || !group_size || !comparator || !comparator->compare || (count && !items) ||
        count > SIZE_MAX/sizeof(void *)) return LKS_STATUS_INVALID_ARGUMENT;
    descriptor = *comparator; groups = count/group_size+(count%group_size != 0);
    if (groups > SIZE_MAX/sizeof(LksImmutableGroup *)) return LKS_STATUS_INVALID_ARGUMENT;
    b = (LksImmutableGroupBatch *)lks_alloc(sizeof(*b));
    if (!b) return LKS_STATUS_OUT_OF_MEMORY;
    memset(b, 0, sizeof(*b)); b->count = count; b->group_size = group_size;
    if (groups) {
        b->groups = (LksImmutableGroup **)lks_alloc(groups*sizeof(*b->groups));
        if (!b->groups) { lks_free(b); return LKS_STATUS_OUT_OF_MEMORY; }
    }
    for (i = 0; i < groups; ++i) {
        size_t n = count-at < group_size ? count-at : group_size;
        status = lks_immutable_group_build(items+at, n, &descriptor, &b->groups[i]);
        if (status != LKS_STATUS_OK) { lks_immutable_group_batch_destroy(b); return status; }
        ++b->group_count; at += n;
    }
    *out = b; return LKS_STATUS_OK;
}
size_t lks_immutable_group_batch_size(const LksImmutableGroupBatch *b)
{ return b && !b->busy ? b->count : 0; }
size_t lks_immutable_group_batch_group_count(const LksImmutableGroupBatch *b)
{ return b && !b->busy ? b->group_count : 0; }
size_t lks_immutable_group_batch_group_size(const LksImmutableGroupBatch *b)
{ return b && !b->busy ? b->group_size : 0; }
const LksImmutableGroup *lks_immutable_group_batch_group_at(const LksImmutableGroupBatch *b, size_t i)
{ return b && !b->busy && i < b->group_count ? b->groups[i] : NULL; }
LksStatus lks_immutable_group_batch_merge_all(const LksImmutableGroupBatch *batch,
    const LksComparator *comparator, LksImmutableGroup **out)
{
    LksImmutableGroupBatch *b = (LksImmutableGroupBatch *)batch;
    LksImmutableGroup *g; void **scratch = NULL, **from, **to;
    size_t i, at = 0, width; LksComparator descriptor;
    if (out) *out = NULL;
    if (!b || !out || !comparator || !comparator->compare) return LKS_STATUS_INVALID_ARGUMENT;
    if (b->busy) return LKS_STATUS_REENTRANT;
    for (i = 0; i < b->group_count; ++i) if (b->groups[i]->busy) return LKS_STATUS_REENTRANT;
    descriptor = *comparator; g = ig_new(b->count); if (!g) return LKS_STATUS_OUT_OF_MEMORY;
    if (b->count) {
        g->items = (void **)lks_alloc(b->count*sizeof(*g->items));
        if (!g->items) goto oom;
    }
    if (b->group_count > 1) {
        scratch = (void **)lks_alloc(b->count*sizeof(*scratch));
        if (!scratch) goto oom;
    }
    for (i = 0; i < b->group_count; ++i) {
        memcpy(g->items+at, b->groups[i]->items, b->groups[i]->count*sizeof(*g->items));
        at += b->groups[i]->count;
    }
    b->busy = 1;
    for (i = 0; i < b->group_count; ++i) b->groups[i]->busy = 1;
    from = g->items; to = scratch; width = b->group_size;
    while (width < b->count) {
        for (at = 0; at < b->count;) {
            size_t left = b->count-at < width ? b->count-at : width;
            size_t remain = b->count-at-left, right = remain < width ? remain : width;
            lks_merge_sorted_pointers(from+at, left, from+at+left, right, &descriptor, to+at);
            at += left+right;
        }
        { void **swap = from; from = to; to = swap; }
        if (width >= b->count-width) break;
        width *= 2;
    }
    if (from != g->items) memcpy(g->items, from, b->count*sizeof(*g->items));
    for (i = 0; i < b->group_count; ++i) b->groups[i]->busy = 0;
    b->busy = 0; lks_free(scratch); *out = g; return LKS_STATUS_OK;
oom:
    lks_free(scratch); lks_immutable_group_destroy(g); return LKS_STATUS_OUT_OF_MEMORY;
}
typedef struct IgIterator { const LksImmutableGroup *group; size_t index; } IgIterator;
static void *ig_next(void *context)
{ IgIterator *it = (IgIterator *)context; return it->group->items[it->index++]; }
LksStatus lks_immutable_group_snapshot_capture(const LksImmutableGroup *group,
    const LksSnapshotOptions *options, LksSnapshot **out)
{
    LksImmutableGroup *g = (LksImmutableGroup *)group; IgIterator it;
    if (out) *out = NULL;
    if (!g || !out) return LKS_STATUS_INVALID_ARGUMENT;
    it.group = g; it.index = 0;
    return lks_snapshot_capture_sequence(g->count, ig_next, &it, &g->busy,
        &g->marker, 0, options, out);
}
int lks_immutable_group_snapshot_is_current(const LksImmutableGroup *g, const LksSnapshot *s)
{ return g && s && !g->busy && s->marker && g->marker == s->marker && s->revision == 0; }
