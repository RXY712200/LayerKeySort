#ifndef LKS_SNAPSHOT_INTERNAL_H
#define LKS_SNAPSHOT_INTERNAL_H
#include "lks_order_internal.h"
typedef struct LksSnapshotRow { size_t end; } LksSnapshotRow;
struct LksSnapshot {
    LksSnapshotRow *rows;
    unsigned char *namespace_data, *associations;
    size_t count, namespace_size, association_size, association_capacity;
    LksSourceMarker *marker;
    uint64_t revision;
};
/* Rows hold cumulative ends: repeated ends represent empty associations.
 * Header, rows and namespace share one allocation; association bytes stay
 * separately growable until publication. Unpublished rows need no zero fill. */
static inline size_t lks_snapshot_row_offset(const LksSnapshot *s, size_t row)
{ return row ? s->rows[row-1].end : 0; }
static inline size_t lks_snapshot_row_size(const LksSnapshot *s, size_t row)
{ return s->rows[row].end-lks_snapshot_row_offset(s, row); }
LksStatus lks_snapshot_new(size_t count, const void *domain, size_t domain_size,
    LksSnapshot **out);
/* Private traversal adapter: one next-item call per occurrence, no rank search. */
typedef void *(*LksSnapshotNextItem)(void *iterator);
LksStatus lks_snapshot_capture_sequence(size_t count, LksSnapshotNextItem next,
    void *iterator, int *busy, LksSourceMarker **marker, uint64_t revision,
    const LksSnapshotOptions *options, LksSnapshot **out);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
/* Private capacity boundary injection, only with externally serialized tests. */
void lks_source_marker_test_refs(LksSourceMarker *marker, size_t refs);
#endif
#endif
