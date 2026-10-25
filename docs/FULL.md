# LayerKeySort Full — v4.1.0 Stable

[← LayerKeySort product family](../README.md) | English | [简体中文](FULL.zh-CN.md)

This page documents **Full**, the original library at the repository root. It is not the Mini API. For authoritative compatibility and format contracts, consult [V4_FREEZE](V4_FREEZE.md), not this onboarding page.

Dynamic ordering for changing collections in C17, with stable live handles and
sortable immutable snapshot keys.

**Live order: contextual handles. Historical/persistent order: immutable snapshot keys.**

**v4.1.0 is the current recommended Stable release.**
[Full v4.1.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) includes compatible Live Order and Snapshot memory-layout optimizations.
[V4_FREEZE](V4_FREEZE.md) freezes the V4.0 API, LS1 and LKS4SNP1 contracts.
V4 completes the fixed Preview.1–5 / RC.1 development cycle.
[v4.1.0 update record](V4_1_0.md), [historical v4.0.0 Stable record](V4_STABLE.md), [RC validation record](V4_RC1.md).
V3.1.0 remains a historical Stable release for applications requiring Path/Tree contracts;
V4 is a major source break, not a drop-in upgrade.

The active post-v4 roadmap and current technical debt are tracked in
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3). Historical Issue #2
is closed and preserved as the V2/V3/V4-development record.

## What problem does LayerKeySort solve?

LayerKeySort is **not a replacement for one-shot array sorting** such as `qsort`
or `std::sort`. It is for collections that stay alive and keep changing order:

- layer stacks and editor object lists,
- video/audio timelines,
- CAD/game/node/workflow editors,
- playlists and local document editors,
- other long-lived collections with repeated insert/remove/move operations.

The core question is not “sort these N items once”, but “keep this collection in
order while it is edited for minutes or hours, then capture or persist a historical
ordering when needed”.

LayerKeySort is usually the wrong tool if all you need is a one-shot sort, a simple
database `position` column, distributed/CRDT ordering, or persistent business identity.
Applications own identity and storage; LayerKeySort owns ordering semantics.

## Choose a workflow

- LksOrder: editable relative order, stable resident handles, before/after moves.
- LksManagedOrder: comparator-managed order, stable equal insertion.
- LksGroup/LksGroupBatch: flat immutable sequences and stable merge.
- LksSnapshot: copied historical associations, LS1 and explicit wire persistence;
  restoration gives fresh live handles.

Items/business identity remain application-owned. Handle is not persistent;
snapshot key is not live position. V3 Path/Tree APIs are absent from the V4 header;
see [migration](MIGRATION_V3_V4.md).

## First successful run

```sh
cmake -S . -B build -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Run build/layerkeysort_example (MSVC: build/Debug/layerkeysort_example.exe).
[Minimal example](../examples/basic.c): create, insert, query, remove and destroy.
[Seven examples](../examples/README.md) use only public headers.

```c
#include <layerkeysort.h>
int main(void) {
    int item = 42;
    const LksOrderHandle *handle = NULL;
    LksOrder *order = lks_order_create();
    if (!order) return 1;
    if (lks_order_insert_back(order, &item, &handle) != LKS_STATUS_OK) {
        lks_order_destroy(order); return 2;
    }
    lks_order_destroy(order); /* item remains application-owned */
    return 0;
}
```

## Integrate

CMake >=3.21: add_subdirectory, offline FetchContent or installed
find_package(LayerKeySort CONFIG REQUIRED), target LayerKeySort::layerkeysort.
No Python/private headers for ordinary consumers. Deterministic two-file C17
source ZIP plus SHA256SUMS; C++17 clients link the C library.
[Integration guide](INTEGRATION.md).

## Limits and evidence

V4 avoids resident Path growth/global relabel. Structural work O(B+log M),
private B64; allocator/comparator/callback/OS costs remain. No whole-library hard
latency or universal speed guarantee. Full snapshot export O(N+A); frequent
exports/retained history can dominate time/memory. Mutable sources require caller
serialization; items/contexts stay alive, ordering semantics stay consistent.
Dangling handles cannot safely be tested after removal.

The [v4.1.0 update record](V4_1_0.md) documents later internal 100k–10m workload studies, accepted storage optimizations and workload-specific timing regressions. Earlier [Preview.4 evidence](../benchmarks/results/v4-preview4/README.md) and [v4.0.0 Stable record](V4_STABLE.md) remain available as history.
No universal speedup is claimed. External adoption is not a prerequisite for future internal research.

## Documentation

- [Usage](USAGE.md), [API](API.md), [Examples](../examples/README.md)
- [Compatibility](COMPATIBILITY.md), [Migration](MIGRATION_V3_V4.md)
- [Validation](VALIDATION.md), [Development](DEVELOPMENT.md)
- [Architecture](ARCHITECTURE.md), [Changelog](../CHANGELOG.md)

Earlier Preview guides and captured V3 benchmarks are historical, not the current
primary API. License: [MIT](../LICENSE).
