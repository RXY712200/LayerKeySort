# V4 integration

**Version status:** [LayerKeySort Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) is the current published Stable release. The 4.x API and wire contract stay frozen; see [4.1.0 notes](V4_1_0.md).

Tested: MSVC Windows x64/x86, Linux GCC/Clang, macOS AppleClang; C17 implementation,
C++17 consumer, eight-bit bytes. CMake >=3.21.

```cmake
add_subdirectory(path/to/LayerKeySort lks)
target_link_libraries(app PRIVATE LayerKeySort::layerkeysort)
```

Offline FetchContent: set FETCHCONTENT_SOURCE_DIR_LAYERKEYSORT_SOURCE to local
checkout before FetchContent_MakeAvailable(layerkeysort_source). Embedded tests,
examples/install default OFF. Installed consumption:

```sh
cmake -S . -B build -DLKS_BUILD_TESTS=OFF -DLKS_BUILD_EXAMPLES=OFF -DLKS_INSTALL=ON
cmake --build build --config Debug
cmake --install build --config Debug --prefix install
```

```cmake
find_package(LayerKeySort 4 CONFIG REQUIRED)
target_link_libraries(app PRIVATE LayerKeySort::layerkeysort)
```

16 production modules in cmake/ProductionSources.cmake; four legacy live modules
regression-only. Rebuild static package for compiler/architecture/CRT settings.
Current `main` CMake and public header report version **4.1.0**. The package
uses `SameMajorVersion` for rebuild-from-source compatibility; no shared ABI
is shipped. Pin the published `v4.1.0` tag for source integration, or download
the verified [Full v4.1.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) assets.

ZIP contains layerkeysort.h, layerkeysort.c, example.c, LICENSE and README.txt.
Compile implementation as C17, link C/C++ application. No Python/private header
requirement for installed/amalgamated consumers; Python only development tools.
[Freeze](V4_FREEZE.md) defines exact compatibility and package gates.

Full 4.1.0 retains Preview.5's frozen public declarations and LS1/LKS4SNP1
formats. See [4.1.0 record](V4_1_0.md) and the historical
[v4.0.0 Stable record](V4_STABLE.md). V3.1.0 remains available for applications
using legacy Path/Tree contracts.
