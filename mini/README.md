# LayerKeySort Mini

Current milestone: **v1.0.0-rc.1**. MIT licensed, independent C17 library
for local mutable order maintenance with stable live occurrence handles.

Use Mini when an application needs to maintain an explicit sequence, such as
an editor's ordered objects, a task queue, or a UI collection. The application
decides the order and owns its objects. Mini stores opaque `void *` payloads;
it does not sort, inspect, or free them. NULL payloads and repeated pointers
are valid. Each insertion creates a distinct occurrence.

RC.1 preserves all 18 planned functions: creation/destruction, size, endpoint
queries, neighbor traversal, payload lookup, four insertion variants, removal,
four stable-handle movement operations and live-order comparison.
This release candidate is not the final Stable release. Final validation and
publication require the separate Stable milestone.

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

Known-handle movement and basic operations take O(1) time; destruction and
arbitrary order comparison take O(n). Movement and comparison allocate/free
nothing. Traversal takes
O(n) and each occurrence needs one allocation. There is no random-access index,
persistence, stale-pointer detection or automatic synchronization.

## Project documentation

- [Contract](docs/CONTRACT.md): exact API, errors and handle lifetime.
- [Usage](docs/USAGE.md): insertion, traversal, removal and ownership.
- [Development](docs/DEVELOPMENT.md): standalone validation and scope.
- [Architecture](docs/ARCHITECTURE.md): invariants and tradeoffs.
- [Changelog](CHANGELOG.md) and [license](LICENSE).
- [简体中文](README.zh-CN.md).

Moves preserve the handle, payload, owner and size. Moving to the current
endpoint, before/after self, before the immediate next neighbor or after the
immediate previous neighbor succeeds without changing links. Comparison returns
exactly -1 (before), 0 (same occurrence), or +1 (after), independently of payload.
All required handles are validated before no-op decisions. Remaining milestones
are final Stable validation and authorized delivery.

Preview.3 adds an independent array reference model: four fixed xorshift32 seeds,
6,000 operations each, three independent orders, and a 48-occurrence per-order
bound. Default CTest also checks adversarial sequences, private invariants,
24 single-allocation failure/recovery cases and a 190-case legal-input matrix.
Local Windows GCC 16.2.0 checks include strict Debug/Release, extraction and
production static analysis. Local Windows ASan/UBSan probing failed because their
link libraries are absent. This local limitation does not describe Linux CI.
No production defect was found in these checks. See the [development guide](docs/DEVELOPMENT.md)
for reproducibility and coverage limits. Allocation accounting is not a sanitizer.

RC.1 adds strict compiler/sanitizer build options, an independent cross-platform
CI matrix, extracted-project verification and external consumers. For exact
integration commands see [Development](docs/DEVELOPMENT.md); for measured costs
see [Performance](docs/PERFORMANCE.md). CI results must be checked on the exact
candidate SHA; a workflow definition alone is not validation.

RC.1 candidate 58f19a15 passed all five Mini CI jobs, including the instrumented
Linux suite; GCC/Clang/MSVC/AppleClang Debug and Release each passed 4/4.
The exact candidate, compiler versions and run are recorded in
[Development](docs/DEVELOPMENT.md). Later changes require their own SHA checks.
