#include "lks_snapshot_internal.h"
#include <string.h>
static const char snap_hex[] = "0123456789abcdef";
static int snap_nibble(char c)
{ return c >= '0' && c <= '9' ? c-'0' : c >= 'a' && c <= 'f' ? c-'a'+10 : -1; }
size_t lks_snapshot_key_length(const LksSnapshot *s)
{ return s ? 21+2*s->namespace_size : 0; }
LksStatus lks_snapshot_key_format(const LksSnapshot *s, size_t row,
    char *buffer, size_t buffer_size)
{
    size_t i, at = 4; uint64_t ordinal = (uint64_t)row;
    if (!s || !buffer || row >= s->count) return LKS_STATUS_INVALID_ARGUMENT;
    if (buffer_size <= lks_snapshot_key_length(s)) return LKS_STATUS_BUFFER_TOO_SMALL;
    memcpy(buffer, "LS1.", 4);
    for (i = 0; i < s->namespace_size; ++i) {
        buffer[at++] = snap_hex[s->namespace_data[i] >> 4];
        buffer[at++] = snap_hex[s->namespace_data[i] & 15];
    }
    buffer[at++] = '.';
    for (i = 0; i < 16; ++i) buffer[at++] = snap_hex[(ordinal >> (60-4*i)) & 15];
    buffer[at] = '\0'; return LKS_STATUS_OK;
}
static LksStatus snap_key_parse(const char *key, size_t *domain_size, uint64_t *ordinal)
{
    const char *p; size_t n = 0, i; uint64_t value = 0;
    if (!key || strncmp(key, "LS1.", 4)) return LKS_STATUS_INVALID_ARGUMENT;
    p = key+4;
    while (*p && *p != '.') {
        if (snap_nibble(*p) < 0 || n == SIZE_MAX) return LKS_STATUS_INVALID_ARGUMENT;
        ++p; ++n;
    }
    if (!n || n%2 || n/2 > UINT32_MAX || *p != '.') return LKS_STATUS_INVALID_ARGUMENT;
    ++p;
    for (i = 0; i < 16; ++i) {
        int digit = snap_nibble(*p);
        if (digit < 0) return LKS_STATUS_INVALID_ARGUMENT;
        value = (value << 4) | (unsigned)digit; ++p;
    }
    if (*p) return LKS_STATUS_INVALID_ARGUMENT;
    *domain_size = n; *ordinal = value; return LKS_STATUS_OK;
}
LksStatus lks_snapshot_key_validate(const char *key)
{ size_t n; uint64_t value; return snap_key_parse(key, &n, &value); }
LksStatus lks_snapshot_key_compare(const char *left, const char *right, int *out)
{
    size_t a, b; uint64_t x, y; LksStatus status;
    if (out) *out = 0;
    if (!out) return LKS_STATUS_INVALID_ARGUMENT;
    status = snap_key_parse(left, &a, &x); if (status != LKS_STATUS_OK) return status;
    status = snap_key_parse(right, &b, &y); if (status != LKS_STATUS_OK) return status;
    if (a != b || memcmp(left+4, right+4, a)) return LKS_STATUS_DOMAIN_MISMATCH;
    *out = (x > y)-(x < y); return LKS_STATUS_OK;
}
