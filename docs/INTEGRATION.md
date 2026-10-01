# Integrating LayerKeySort

The v2.0.0 stable candidate retains RC.1's 52-function public C API and
production implementation. Until the stable tag is published, pin the exact
candidate commit for validation. Generated Path layouts and performance
policy remain implementation details.

## Requirements and tested configurations

- C17 for the library and application code using its C header.
- CMake 3.21 or newer for the CMake build.
- CI configurations: Windows/MSVC, Ubuntu/GCC, Ubuntu/Clang, Ubuntu/Clang
  with sanitizers, and macOS/AppleClang. A passing CI run validates its runner
  and compiler versions, not every version of those operating systems or
  compilers. The RC.1 macOS job passed for its recorded CI runner.

The public header has `extern "C"` guards for inclusion by C++ applications;
the library implementation remains C17. Callers own item objects. Tree and
Group objects borrow item pointers.

## CMake `add_subdirectory`

Place or vendor a checked-out LayerKeySort release alongside the application.
The normal CMake target is the production-only static library `layerkeysort`.
Disable repository validation targets in the consuming project before adding
the subdirectory:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LKS_BUILD_TESTS OFF)
set(LKS_BUILD_BENCHMARKS OFF)
add_subdirectory(external/LayerKeySort EXCLUDE_FROM_ALL)

add_executable(my_app src/main.c)
target_link_libraries(my_app PRIVATE layerkeysort)
```

The `layerkeysort` target supplies `include/` transitively. `EXCLUDE_FROM_ALL`
keeps the repository's example targets out of a normal consumer-wide build;
linking `layerkeysort` still builds the library. CMake 3.21 honors
these normal variables for the project's `option()` calls. If several
subprojects use the same option names, configure them deliberately in the
parent project.

## CMake `FetchContent`

Pin the exact stable candidate commit for pre-publication validation; the
public example will use `v2.0.0` once that tag exists:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
include(FetchContent)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LKS_BUILD_TESTS OFF)
set(LKS_BUILD_BENCHMARKS OFF)
FetchContent_Declare(layerkeysort_source
    GIT_REPOSITORY https://github.com/RXY712200/LayerKeySort.git
    GIT_TAG <exact-stable-candidate-commit>)
FetchContent_MakeAvailable(layerkeysort_source)

add_executable(my_app src/main.c)
target_link_libraries(my_app PRIVATE layerkeysort)
```

`FetchContent_MakeAvailable` also adds the repository's example
targets; `cmake --build build --target my_app` builds only the application and
its library dependency when that distinction matters.

## Direct C17 source integration

Add `include/` to the compiler's include search path and compile these
production sources into the application or a static library:

```text
src/lks_alloc.c
src/lks_base.c
src/path.c
src/path_text.c
src/path_order_key.c
src/slot_codec.c
src/path_compare.c
src/gap.c
src/tree.c
src/group.c
src/sort.c
src/bulk.c
```

Include `layerkeysort.h`. The `src/*.h` files are private implementation
headers required by those `.c` files, not application headers or public ABI.
Do not define `LKS_ENABLE_ALLOC_DIAGNOSTICS` or
`LKS_ENABLE_V1_REGRESSION_HELPERS` in a normal application build; the CMake
production target does not define them. The historical Visual Studio project
is a combined diagnostic runner, not the reusable library target.

MSVC uses `/std:c17`; GCC and Clang use `-std=c17`. The public C header may be
included from C++ code, but compile the production `.c` files as C17.

## Build options

| Option | Default | Use |
| --- | --- | --- |
| `LKS_BUILD_TESTS` | `ON` | Repository diagnostic test runner and mutation soak. Set `OFF` for a normal consumer build. |
| `LKS_BUILD_BENCHMARKS` | `OFF` | Optional public-API benchmark harness and three CTest correctness smoke cases when tests are enabled. Set `OFF` for a normal consumer build. |
| `LKS_ENABLE_SANITIZERS` | `OFF` | ASan/UBSan flags for supported GCC/Clang validation builds. Use only with a compatible compiler/runtime. |

The CMake targets `layerkeysort_example` and
`layerkeysort_layer_list_example` show basic stable sorting and dynamic layer
ordering. The latter covers between placement, rekey, remove, and LK1
round trip without adding convenience functions to the public API.

For Path persistence, store canonical versioned `LK1:` keys under a database
collation preserving bytewise ASCII order. A persisted Path is a mutable
ordering coordinate, not a permanent item ID or whole-Tree snapshot.
