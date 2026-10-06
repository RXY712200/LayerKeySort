#include "lks_snapshot_internal.h"
#include "lks_alloc_internal.h"
#include <string.h>
_Static_assert(CHAR_BIT == 8, "snapshot wire requires eight-bit bytes");
static uint64_t snap_be_read(const unsigned char *p, size_t width)
{ uint64_t v = 0; size_t i; for (i = 0; i < width; ++i) v = (v << 8) | p[i]; return v; }
static void snap_be_write(unsigned char *p, uint64_t v, size_t width)
{ while (width) { p[--width] = (unsigned char)(v & 255); v >>= 8; } }
size_t lks_snapshot_serialized_size(const LksSnapshot *s)
{ return s ? 24+s->namespace_size+8*s->count+s->association_size : 0; }
LksStatus lks_snapshot_serialize(const LksSnapshot *s, void *buffer, size_t size)
{
    unsigned char *p = (unsigned char *)buffer; size_t i;
    if (!s || !buffer) return LKS_STATUS_INVALID_ARGUMENT;
    if (size < lks_snapshot_serialized_size(s)) return LKS_STATUS_BUFFER_TOO_SMALL;
    memcpy(p, "LKS4SNP1", 8); snap_be_write(p+8, 0, 4);
    snap_be_write(p+12, s->namespace_size, 4); snap_be_write(p+16, s->count, 8);
    p += 24; memcpy(p, s->namespace_data, s->namespace_size); p += s->namespace_size;
    for (i = 0; i < s->count; ++i) {
        size_t n = s->rows[i].size;
        snap_be_write(p, n, 8); p += 8;
        if (n) memcpy(p, s->associations+s->rows[i].offset, n);
        p += n;
    }
    return LKS_STATUS_OK;
}
LksStatus lks_snapshot_deserialize(const void *buffer, size_t size, LksSnapshot **out)
{
    const unsigned char *p = (const unsigned char *)buffer;
    size_t domain, count, at, i, total = 0; uint64_t rows;
    LksSnapshot *s; LksStatus status;
    if (out) *out = NULL;
    if (!out || !p || size < 24) return LKS_STATUS_INVALID_ARGUMENT;
    if (memcmp(p, "LKS4SNP1", 8) || snap_be_read(p+8, 4)) return LKS_STATUS_INVALID_ARGUMENT;
    domain = (size_t)snap_be_read(p+12, 4); rows = snap_be_read(p+16, 8);
    if (!domain || domain > size-24 || rows > SIZE_MAX) return LKS_STATUS_INVALID_ARGUMENT;
    at = 24+domain; count = (size_t)rows;
    if (count > (size-at)/8 || count > SIZE_MAX/sizeof(LksSnapshotRow))
        return LKS_STATUS_INVALID_ARGUMENT;
    /* Complete counting/validation pass before any speculative allocation. */
    for (i = 0; i < count; ++i) {
        uint64_t length;
        if (size-at < 8) return LKS_STATUS_INVALID_ARGUMENT;
        length = snap_be_read(p+at, 8); at += 8;
        if (length > SIZE_MAX || length > size-at || length > SIZE_MAX-total)
            return LKS_STATUS_INVALID_ARGUMENT;
        at += (size_t)length; total += (size_t)length;
    }
    if (at != size) return LKS_STATUS_INVALID_ARGUMENT;
    status = lks_snapshot_new(count, p+24, domain, &s);
    if (status != LKS_STATUS_OK) return status;
    if (total) {
        s->associations = (unsigned char *)lks_alloc(total);
        if (!s->associations) { lks_snapshot_destroy(s); return LKS_STATUS_OUT_OF_MEMORY; }
    }
    s->association_capacity = total; s->association_size = total;
    at = 24+domain; total = 0;
    for (i = 0; i < count; ++i) {
        size_t n = (size_t)snap_be_read(p+at, 8); at += 8;
        s->rows[i].offset = total; s->rows[i].size = n;
        if (n) memcpy(s->associations+total, p+at, n);
        total += n; at += n;
    }
    *out = s; return LKS_STATUS_OK;
}
