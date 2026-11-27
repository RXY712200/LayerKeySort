# LayerKeySort Full 4.x 集成指南

Full v4.2.0 支持源码、已安装 CMake 包及两文件集成。固定到经过审查的源码版本，并按编译器、架构与 CRT 重新构建。

已发布 Stable/Latest：[Full v4.2.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.2.0)，发布日期 2026-11-27。[C17 源码 ZIP](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-amalgamation.zip) · [SHA-256 清单](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-SHA256SUMS.txt)。发行源码：`v4.2.0`／`f751e1ee2afd682874281d6dc495969f9eca49eb`。[历史 4.1 发行](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。

实现使用 C17；C++17 应用可以通过 C API 链接。已验证 Windows MSVC x64/Win32、
Linux GCC/Clang、macOS AppleClang。CMake 最低版本 3.21；支持的平台使用八位字节。

## add_subdirectory

```cmake
add_subdirectory(path/to/LayerKeySort lks)
target_link_libraries(app PRIVATE LayerKeySort::layerkeysort)
```

作为子项目时，测试、示例和 install 默认不会强制打开。

## 本地安装 + find_package

```sh
cmake -S . -B build -DLKS_BUILD_TESTS=OFF -DLKS_BUILD_EXAMPLES=OFF -DLKS_INSTALL=ON
cmake --build build --config Debug
cmake --install build --config Debug --prefix install
```

应用侧：

```cmake
find_package(LayerKeySort 4 CONFIG REQUIRED)
target_link_libraries(app PRIVATE LayerKeySort::layerkeysort)
```

## FetchContent

支持离线 FetchContent。可以把
`FETCHCONTENT_SOURCE_DIR_LAYERKEYSORT_SOURCE`
指向本地 checkout，再执行 `FetchContent_MakeAvailable`。

正式项目建议固定到明确 release/tag/commit，而不是无约束跟随 main。

## 两文件 amalgamation

Stable Release 提供：

- `layerkeysort.h`
- `layerkeysort.c`

以及最小示例、LICENSE 和 README。

把 `layerkeysort.c` 按 C17 编译，再让 C/C++ 应用链接即可。

普通 consumer 不需要 Python 或 private headers；Python 只用于仓库开发/验证工具。

## 兼容性

v4.0.0 冻结的 4.x 源码契约、LS1 和 LKS4SNP1 在 4.1.0 中仍然兼容。
4.1.0 改进 Full 私有内存布局，不能理解成所有负载必然加速。

生产源码清单 `cmake/ProductionSources.cmake` 包含16个模块；四个旧实时模块仅用于回归。已发布的 Full v4.2.0 的 CMake 与公共头均报告 **4.2.0**。安装包采用 `SameMajorVersion` 表达从源码重建的同主版本兼容性。ZIP 包含 `layerkeysort.h`、`layerkeysort.c`、`example.c`、`LICENSE` 和 `README.txt`。

当前项目发布的是 static library，不承诺跨 MSVC/GCC/Clang、架构或 CRT 的 universal binary ABI。
推荐按目标工具链重新构建。

V3.1.0 仍保留给必须继续使用 Path/Tree 契约的应用。

详见：

- [英文完整集成指南](INTEGRATION.md)
- [兼容性](COMPATIBILITY.zh-CN.md)
- [API](API.zh-CN.md)
- [V3 → V4 迁移](MIGRATION_V3_V4.zh-CN.md)
- [4.1.0 更新记录](V4_1_0.zh-CN.md)
- [v4.0.0 Stable 历史记录](V4_STABLE.zh-CN.md)
