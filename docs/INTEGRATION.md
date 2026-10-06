# V4 integration

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
Numeric CMake version 4.0.0 uses SameMajorVersion source rebuild compatibility;
header includes prerelease text. No shared ABI shipped.

ZIP contains layerkeysort.h, layerkeysort.c, example.c, LICENSE and README.txt.
Compile implementation as C17, link C/C++ application. No Python/private header
requirement for installed/amalgamated consumers; Python only development tools.
[Freeze](V4_FREEZE.md) defines exact compatibility and package gates.

RC.1 uses the unchanged Preview.5 frozen declarations and formats; see
[RC validation](V4_RC1.md). Recommended Stable remains v3.1.0.
