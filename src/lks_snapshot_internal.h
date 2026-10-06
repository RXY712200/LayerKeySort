#ifndef LKS_SNAPSHOT_INTERNAL_H
#define LKS_SNAPSHOT_INTERNAL_H
#include "lks_order_internal.h"
typedef struct LksSnapshotRow { size_t offset, size; } LksSnapshotRow;
struct LksSnapshot {
    LksSnapshotRow *rows;
    unsigned char *namespace_data, *associations;
    size_t count, namespace_size, association_size, association_capacity;
    LksSourceMarker *marker;
    uint64_t revision;
};
/* Used after validated counts/lengths only; all fields initially zero. */
LksStatus lks_snapshot_new(size_t count, const void *domain, size_t domain_size,
    LksSnapshot **out);
#ifdef LKS_ENABLE_ALLOC_DIAGNOSTICS
/* Private capacity boundary injection, only with externally serialized tests. */
void lks_source_marker_test_refs(LksSourceMarker *marker, size_t refs);
#endif
#endif
