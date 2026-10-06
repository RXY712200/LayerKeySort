# LayerKeySort

English | [简体中文](README.zh-CN.md)

Dynamic ordering for changing collections in C17, with stable live handles and
sortable immutable snapshot keys.

**Live order: contextual handles. Historical/persistent order: immutable snapshot keys.**

**v4.0.0 is the current recommended Stable release.**
[V4_FREEZE](docs/V4_FREEZE.md) freezes the V4.0 API, LS1 and LKS4SNP1 contracts.
V4 completes the fixed Preview.1–5 / RC.1 development cycle.
[Stable release record](docs/V4_STABLE.md), [RC validation record](docs/V4_RC1.md).
V3.1.0 remains a historical Stable release for applications requiring Path/Tree contracts;
V4 is a major source break, not a drop-in upgrade.

## Choose a workflow

- LksOrder: editable relative order, stable resident handles, before/after moves.
- LksManagedOrder: comparator-managed order, stable equal insertion.
- LksGroup/LksGroupBatch: flat immutable sequences and stable merge.
- LksSnapshot: copied historical associations, LS1 and explicit wire persistence;
  restoration gives fresh live handles.

Items/business identity remain application-owned. Handle is not persistent;
snapshot key is not live position. V3 Path/Tree APIs are absent from the V4 header;
see [migration](docs/MIGRATION_V3_V4.md).

## First successful run

```sh
cmake -S . -B build -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Run build/layerkeysort_example (MSVC: build/Debug/layerkeysort_example.exe).
[Minimal example](examples/basic.c): create, insert, query, remove and destroy.
[Seven examples](examples/README.md) use only public headers.

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
[Integration guide](docs/INTEGRATION.md).

## Limits and evidence

V4 avoids resident Path growth/global relabel. Structural work O(B+log M),
private B64; allocator/comparator/callback/OS costs remain. No whole-library hard
latency or universal speed guarantee. Full snapshot export O(N+A); frequent
exports/retained history can dominate time/memory. Mutable sources require caller
serialization; items/contexts stay alive, ordering semantics stay consistent.
Dangling handles cannot safely be tested after removal.

[Preview.4 evidence](benchmarks/results/v4-preview4/README.md) remains the primary
convergence record; [Stable record](docs/V4_STABLE.md) classifies costs/future debt.

## Documentation

- [Usage](docs/USAGE.md), [API](docs/API.md), [Examples](examples/README.md)
- [Compatibility](docs/COMPATIBILITY.md), [Migration](docs/MIGRATION_V3_V4.md)
- [Validation](docs/VALIDATION.md), [Development](docs/DEVELOPMENT.md)
- [Architecture](docs/ARCHITECTURE.md), [Changelog](CHANGELOG.md)

Earlier Preview guides and captured V3 benchmarks are historical, not the current
primary API. License: [MIT](LICENSE).
