# Single ordered production-source manifest. CMake and tools/amalgamate.py read
# this file; keep diagnostics and examples out of this list.
set(LKS_SOURCES
    src/lks_alloc.c
    src/lks_base.c
    src/path.c
    src/path_text.c
    src/path_order_key.c
    src/slot_codec.c
    src/path_compare.c
    src/gap.c
    src/tree.c
    src/order.c
    src/managed_order.c
    src/snapshot.c
    src/snapshot_key.c
    src/snapshot_wire.c
    src/snapshot_restore.c
    src/order_bulk.c
    src/group.c
    src/sort.c
    src/bulk.c
)
