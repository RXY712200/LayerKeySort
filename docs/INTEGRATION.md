# Integrating LayerKeySort

Current V4 scope is **experimental Preview.4**: [flat immutable Groups/Batch and full workload evidence](V4_PREVIEW4.md), plus [immutable snapshots, LS1, persistence, restoration and V3 import](V4_PREVIEW3.md), alongside [live moves and managed order](V4_PREVIEW2.md). Stable remains v3.1.0. Earlier milestone guides are historical records; V4 API/wire freeze is deferred to Preview.5.

English | [简体中文](INTEGRATION.zh-CN.md)

For this experimental V4 branch use the same source/library integration
mechanisms, with [V4 Preview.3](V4_PREVIEW3.md) and its snapshot example as the export guide. The current source manifest has 20 production modules; use it rather than copying the historical Stable module count below. Installed
current-branch packages have numeric version 4.0.0; request 4.0 or omit a version
in `find_package`. The pinned v3.1.0 examples/assets below refer to the published
Stable and do not contain the new V4 API. V4 prereleases use GitHub source archives; no new binary asset policy is introduced.

Choose a method for your build. These are the LayerKeySort 3.1.0 integration
routes. Use the `v3.1.0` release tag, its published assets, or an exact commit.
See [V3 migration](V3_MIGRATION.md) and [compatibility](COMPATIBILITY.md).

| Method | Best for | CMake to consume? | Files in application |
| --- | --- | --- | --- |
| Two-file amalgamation | Small source drop-in | No | `layerkeysort.h` + `layerkeysort.c` |
| `FetchContent` | CMake source dependency | Yes | Fetched source |
| `add_subdirectory` | Vendored full repository | Yes | Full checkout |
| Install + `find_package` | Reusable local installation | Yes | Locally built package |
| Direct modular sources | Unusual/manual builds | No | 12 production `.c` files and private headers |

The library requires C17. CMake routes require CMake 3.21+. CI tests Windows/MSVC, Ubuntu/GCC and Clang, and macOS/AppleClang; see [Validation](VALIDATION.md).

## Two-file amalgamation

Download `LayerKeySort-3.1.0-amalgamation.zip` from
[GitHub Releases](https://github.com/RXY712200/LayerKeySort/releases). GitHub's
automatic source ZIP/tar.gz is the full repository; the amalgamation ZIP is
the smaller drop-in. It contains `layerkeysort.h` (public API),
`layerkeysort.c` (implementation), `example.c`, `LICENSE`, and `README.txt`.

Extract it and copy `layerkeysort.h` and `layerkeysort.c` into the application. Include `#include "layerkeysort.h"`. Compile as C17:

```sh
cc -std=c17 -I. layerkeysort.c main.c -o my_app
```

`cc` may be GCC or Clang. From an MSVC Developer Command Prompt:

```bat
cl /std:c17 /I. layerkeysort.c main.c
```

Replace `main.c` with `example.c` to compile the packaged example. Users of the generated package need no Python. Maintainers use Python to generate it from canonical `include/` and `src/` sources; do not edit generated `layerkeysort.c`.

For optional checksum verification, compare
`sha256sum LayerKeySort-3.1.0-amalgamation.zip` on GNU/Linux or
`Get-FileHash .\LayerKeySort-3.1.0-amalgamation.zip -Algorithm SHA256` in
PowerShell against `LayerKeySort-3.1.0-SHA256SUMS.txt` from that Release.

## CMake FetchContent

A complete consumer `CMakeLists.txt` for the `v3.1.0` release tag:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(layerkeysort_source
    GIT_REPOSITORY https://github.com/RXY712200/LayerKeySort.git
    GIT_TAG v3.1.0)
FetchContent_MakeAvailable(layerkeysort_source)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE layerkeysort)
```

Build with `cmake -S . -B build` and `cmake --build build --target my_app`.
For 3.1.0 source dependencies, tests, examples, and install rules default OFF.
Pin a release tag or exact commit. To test a local checkout offline, configure with
`-DFETCHCONTENT_SOURCE_DIR_LAYERKEYSORT_SOURCE=<checkout>`.

## Vendored CMake add_subdirectory

Place 3.1.0 source at `external/LayerKeySort` and use:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_subdirectory(external/LayerKeySort)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE layerkeysort)
```

The source target `layerkeysort` supplies the public include directory. Tests,
examples, and install rules default OFF for a subproject; no explicit OFF
flags are needed. A namespaced alias is also available in the build tree.
Historical `v3.0.0` had different subproject defaults and did not include
installation or amalgamation; consult that tag's CMake file when vendoring it.

## Local install and find_package

Build and install 3.1.0 source with your C17 toolchain:

```sh
cmake -S . -B build/install -DLKS_BUILD_TESTS=OFF -DLKS_BUILD_EXAMPLES=OFF
cmake --build build/install
cmake --install build/install --prefix /some/prefix
```

For a multi-configuration generator, pass the same `--config Release` to build and install. In another CMake project:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
find_package(LayerKeySort CONFIG REQUIRED)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE LayerKeySort::layerkeysort)
```

Configure with `cmake -S . -B build -DCMAKE_PREFIX_PATH=/some/prefix`, then `cmake --build build`. Point `CMAKE_PREFIX_PATH` to the install prefix, not its `lib/cmake` directory. The installed target is `LayerKeySort::layerkeysort`; source-tree examples use `layerkeysort`. This is a locally built static package, not a cross-toolchain binary SDK.

## Direct modular C17 sources

For a custom build that cannot use CMake and where amalgamation is unsuitable, add `include/` to your compiler search path and compile all 12 production sources:

```text
src/lks_alloc.c       src/lks_base.c
src/path.c            src/path_text.c
src/path_order_key.c  src/slot_codec.c
src/path_compare.c    src/gap.c
src/tree.c            src/group.c
src/sort.c            src/bulk.c
```

The authoritative source list is [the production manifest](../cmake/ProductionSources.cmake). The `src/*.h` files are private dependencies of those sources, not application headers. Do not define `LKS_ENABLE_ALLOC_DIAGNOSTICS` or `LKS_ENABLE_V1_REGRESSION_HELPERS` in a production build. The historical Visual Studio project is a combined diagnostic runner, not the reusable library target. Amalgamation is usually simpler without CMake.

## C++ consumers

The public header has `extern "C"` guards. C++ applications may include it and link the C17 library, as the installed consumer test does. Compile the implementation sources as C17, not C++.

## Build options and troubleshooting

These are the 3.1.0 build defaults.

| Option | Top-level default | Subproject default | Purpose |
| --- | --- | --- | --- |
| `LKS_BUILD_TESTS` | ON | OFF | Diagnostic tests and soak |
| `LKS_BUILD_EXAMPLES` | ON | OFF | Public API examples |
| `LKS_INSTALL` | ON | OFF | Install/export rules |
| `LKS_BUILD_BENCHMARKS` | OFF | OFF | Public API benchmark harness |
| `LKS_ENABLE_SANITIZERS` | OFF | OFF | ASan/UBSan where supported |

If a consumer cannot find `layerkeysort.h`, link the documented target instead of adding private `src/` headers. If `find_package` fails, check the installation prefix and `CMAKE_PREFIX_PATH`. A C++ link should use the C17-built library. LayerKeySort borrows caller-owned item objects; keep them alive as required by [Usage](USAGE.md). For Path persistence, see the [LK1 specification](API.md).
