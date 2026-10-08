# LayerKeySort Mini

Current milestone: **v1.0.0-preview.1**. MIT licensed, independent C17 library
for local mutable order maintenance with stable live occurrence handles.

Use Mini when an application needs to maintain an explicit sequence, such as
an editor's ordered objects, a task queue, or a UI collection. The application
decides the order and owns its objects. Mini stores opaque `void *` payloads;
it does not sort, inspect, or free them. NULL payloads and repeated pointers
are valid. Each insertion creates a distinct occurrence.

Preview.1 provides 13 functions: creation/destruction, size, endpoint queries,
neighbor traversal, payload lookup, four insertion variants and removal.
Movement and order comparison are future Preview.2 work and are not available.
This preview is a foundation milestone, not the final stable release.

## Build and run

A C17 compiler, CMake 3.16 or later, and a build tool are required. From the
directory containing this README (including an extracted standalone copy):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

For a Windows MinGW installation select `-G "MinGW Makefiles"` when configuring,
with the compiler and `mingw32-make` on PATH. Run `build/lks_mini_basic` on a
single-configuration build, or `build/Debug/lks_mini_basic.exe` for Visual Studio.
The example prints `10` and `30` on separate lines. From a parent checkout,
`cmake -S mini -B <separate-build-directory>` also works.

## Use

Link the `layerkeysort_mini` CMake target and include `layerkeysort_mini.h`.
An embedding CMake project can use `add_subdirectory(path/to/mini)` and
`target_link_libraries(your_target PRIVATE layerkeysort_mini)`.

```c
LksMiniOrder *order = NULL;
LksMiniHandle *handle = NULL;
int value = 42;
if (lks_mini_create(&order) == LKS_MINI_OK) {
    if (lks_mini_insert_back(order, &value, &handle) == LKS_MINI_OK) {
        /* handle identifies this occurrence; value still belongs to us. */
    }
    lks_mini_destroy(order);
}
```

All basic operations take O(1) time; destruction takes O(n). Traversal takes
O(n) and each occurrence needs one allocation. There is no random-access index,
persistence, stale-pointer detection or automatic synchronization.

## Project documentation

- [Contract](docs/CONTRACT.md): exact API, errors and handle lifetime.
- [Usage](docs/USAGE.md): insertion, traversal, removal and ownership.
- [Development](docs/DEVELOPMENT.md): standalone validation and scope.
- [Architecture](docs/ARCHITECTURE.md): invariants and tradeoffs.
- [Changelog](CHANGELOG.md) and [license](LICENSE).
- [简体中文](README.zh-CN.md).
