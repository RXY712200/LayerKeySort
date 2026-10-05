# LayerKeySort

English | [简体中文](README.zh-CN.md)

**This branch implements experimental `v4.0.0-preview.2`.**
V4 adds handle-preserving moves, comparator-managed order and revision-checked
cursors to the contextual live-order core. Start with the [Preview.2 guide](docs/V4_PREVIEW2.md),
[explicit example](examples/live_order.c) and [managed example](examples/managed_order.c).
Snapshots/export/persistence remain unimplemented and V4 APIs are provisional.
The coordinate-based introduction and Quick Start below describe the coexisting
V3 implementation; latest published Stable remains `v3.1.0`.

LayerKeySort is a C17 library for keeping a collection in order as items are added, removed, or moved.

Imagine adding an Effects layer to a layer list:

```text
Before:  Background  Player  HUD
After:   Background  Player  Effects  HUD
```

Sorting an array can tell you the order *now*, but an array index is a poor lasting position when the order keeps changing. LayerKeySort gives items sortable **ordering coordinates** and helps you place new items between existing ones.

The library can manage those coordinates from your comparator, or your application can choose them for explicit moves. The coordinates are called **Paths**. They describe position, not item identity.

**Current Stable release: [v3.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v3.1.0).** Earlier releases, including [v3.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v3.0.0), remain available.

## Get LayerKeySort

| I want to… | Start with |
| --- | --- |
| Try the project | Build the example below. |
| Drop C files into an application | Use the two-file amalgamation. |
| Add a CMake dependency | Use `FetchContent` with a pinned release tag. |
| Reuse a local CMake installation | Install once, then use `find_package`. |
| Vendor the full repository | Use `add_subdirectory`. |

For the 3.1.0 Release, download `LayerKeySort-3.1.0-amalgamation.zip` from [GitHub Releases](https://github.com/RXY712200/LayerKeySort/releases). This source checkout can also generate the package with `python tools/amalgamate.py --package-parent build/amalgamation`. GitHub's automatic source archive is the full repository; the amalgamation ZIP is the smaller two-file route.

Extract the ZIP, copy those two files beside your application, include `#include "layerkeysort.h"`, and compile both C files as C17:

```sh
cc -std=c17 -I. layerkeysort.c main.c -o my_app
```

In an MSVC Developer Command Prompt, use `cl /std:c17 /I. layerkeysort.c main.c`. The ZIP also includes `example.c`, `LICENSE`, and a short `README.txt`. Python generates the package in this repository; it is not needed to consume it. The companion `LayerKeySort-3.1.0-SHA256SUMS.txt` records the ZIP digest.

For a CMake project, use the source target `layerkeysort`:

```cmake
include(FetchContent)
FetchContent_Declare(layerkeysort_source
    GIT_REPOSITORY https://github.com/RXY712200/LayerKeySort.git
    GIT_TAG v3.1.0)
FetchContent_MakeAvailable(layerkeysort_source)
target_link_libraries(my_app PRIVATE layerkeysort)
```

Pin a release tag such as `v3.1.0` or an exact commit, not a moving branch. `my_app` must already be a target in a C17 CMake project (CMake 3.21+). See [Integration](docs/INTEGRATION.md) for complete setup.

## Try LayerKeySort

From a repository checkout, build and run the [small ordered-tree example](examples/ordered_tree.c):

```sh
cmake -S . -B build/quickstart -DLKS_BUILD_TESTS=OFF
cmake --build build/quickstart --target layerkeysort_ordered_example
```

Run `./build/quickstart/layerkeysort_ordered_example` on Unix-like systems. On Windows, run `build\quickstart\layerkeysort_ordered_example.exe` for a single-configuration build, or `build\quickstart\Debug\layerkeysort_ordered_example.exe` for Visual Studio Debug. The example inserts three items, finds one, removes it, and prints:

```text
Found: Middle
After removal: Low < 20 < High
```

For explicit moves, try the [layer-list example](examples/layer_list.c). For a one-time stable pointer-array sort, see the [sort example](examples/basic.c). The [V3 visualizer](https://rxy712200.github.io/LayerKeySort/) shows recorded ordering changes without requiring a C build.

## Use LayerKeySort in your project

For vendored 3.1.0 source, `add_subdirectory(external/LayerKeySort)` and link `layerkeysort`. To install it locally, build the source, run `cmake --install build --prefix <prefix>`, then use `find_package(LayerKeySort CONFIG REQUIRED)` and link `LayerKeySort::layerkeysort`. The [integration guide](docs/INTEGRATION.md) gives complete commands and explains `CMAKE_PREFIX_PATH`. Application code includes only `layerkeysort.h`.

## Which API should I use?

| Your task | Start with |
| --- | --- |
| Keep items in comparator order as the collection changes | [`LksOrderedTree`](examples/ordered_tree.c): the library assigns Paths and keeps equal values in insertion order. |
| Choose positions yourself, including explicit moves | [`LksTree`](examples/layer_list.c): your application selects and changes Paths. |
| Sort a pointer array once, stably | [`lks_sort()`](examples/basic.c); a normal sort may be simpler if stability is unnecessary. |
| Build or merge immutable sorted batches | [Group and GroupBatch](docs/USAGE.md) are specialized batch APIs. |

`LksOrderedTree` binds one comparator when created. If an item's fields used by that comparator need to change, remove the item, update it, and reinsert it. `LksTree` does not impose a comparator order; the application manages its coordinates.

## Is it a fit?

LayerKeySort is useful when positions change repeatedly and you need to insert between existing items or keep a changing collection in comparator order. It is less compelling when:

- You only sort an array once. Use an ordinary sort, or `lks_sort()` when you specifically need stable pointer-array sorting.
- You need a short database rank string for occasional reorders and do not need this library's Tree or Path behavior. A simpler fractional-ranking scheme may have less overhead.
- You need immutable position IDs or distributed/CRDT convergence. Paths can change, and this library does not coordinate independent replicas.
- You require a proven bound on every insertion or consistently low synchronous latency. Managed insertion may relabel many Paths at once; measure your workload before using it in a latency-sensitive path.

The [benchmark report](docs/BENCHMARKS.md) includes slower workloads and large relabel events alongside faster ones. It is evidence from specific machines and inputs, not a universal speed claim.

## The model in a little more detail

A **Path** is a sortable ordering coordinate. Keep your own application ID for each item: `item identity != Path identity`. The library's Tree objects borrow your item pointers; they do not copy or free the items.

With `LksOrderedTree`, the comparator determines logical item order and the library chooses Paths. Comparator-equal items keep stable insertion order. If a gap becomes crowded, a managed insertion may **relabel** existing Paths. Reacquire borrowed Tree nodes and Paths after an actual mutation. With `LksTree`, your application chooses coordinates and can insert, remove, or rekey by Path.

`lks_path_compare()` defines Path order. The physical AVL index is an implementation detail, not a hierarchy of logical Path parents. Readable Path text is for display; the separate versioned `LK1:` key can persist and bytewise-sort one coordinate. Neither form saves an entire Tree or gives an item a permanent ID.

For an interactive explanation, open the [V3 visualizer](https://rxy712200.github.io/LayerKeySort/). Its managed replay uses Path snapshots recorded from the Stable release's unchanged RC.1 production implementation; the browser does not run the C algorithm. The [historical V1 visualizer](docs/demo/index.html) remains separate.

## Limits to plan for

- A managed insertion can relabel a large region or even the full collection, causing workload-dependent synchronous pauses. Path depth, memory use, and LK1 key size also depend on the workload.
- Complete managed insertion has no proven worst-case `O(log n)` guarantee and no formal amortized bound. The [benchmarks](docs/BENCHMARKS.md) give measured cases and methodology.
- Shared mutable objects need external synchronization. Caller-owned items and comparator context must remain valid for the documented lifetimes.
- LK1 stores one coordinate under bytewise ASCII ordering; it is not whole-Tree or item serialization. The library provides no distributed conflict resolution.

See the [API reference](docs/API.md) for exact ownership, error, and Path rules, and the [3.x compatibility contract](docs/COMPATIBILITY.md) for what a compatible update preserves.

## Documentation

| Looking for… | Read |
| --- | --- |
| How to use the main APIs | [Usage guide](docs/USAGE.md) |
| Download, amalgamation, CMake, install, or direct C17 source integration | [Integration guide](docs/INTEGRATION.md) |
| Exact functions, ownership, errors, and Path/LK1 formats | [API reference](docs/API.md) |
| How the two Trees and relabel work | [Architecture](docs/ARCHITECTURE.md) |
| Stable 3.x promises and historical 2.x promises | [Compatibility contract](docs/COMPATIBILITY.md) |
| Changes from V2 Tree code | [V2 to V3 migration guide](docs/V3_MIGRATION.md) |
| Measurements and their limits | [Benchmark report](docs/BENCHMARKS.md) |
| CI, regression tests, and soak counts | [Validation](docs/VALIDATION.md) |
| Building, testing, and contributing | [Development guide](docs/DEVELOPMENT.md) and [Contributing](CONTRIBUTING.md) |
| Earlier design and measurement decisions | [Design history](docs/history/DESIGN_HISTORY.md) and [benchmark history](docs/history/BENCHMARK_HISTORY.md) |
| See ordering changes visually | [V3 visualizer](https://rxy712200.github.io/LayerKeySort/) |
| Release-by-release history | [Changelog](CHANGELOG.md) |

The public header is [`include/layerkeysort.h`](include/layerkeysort.h). The [migration guide](docs/V3_MIGRATION.md) covers the V2-to-V3 Tree API change. Earlier release details remain in the changelog.

## License

LayerKeySort is licensed under the [MIT License](LICENSE).
