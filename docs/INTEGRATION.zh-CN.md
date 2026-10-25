# LayerKeySort Full 4.x 集成指南

[LayerKeySort Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) 是当前已公开发布的 Stable 版本。
可固定到 `v4.1.0` 标签，或从该 Release 下载经过校验的 amalgamation ZIP 和 SHA-256 清单。
历史 v4.0.0 发行记录保持不变。

实现使用 C17；C++17 应用可以通过 C API 链接。已验证 Windows MSVC x64/Win32、
Linux GCC/Clang、macOS AppleClang。CMake 最低版本 3.21。

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

当前项目发布的是 static library，不承诺跨 MSVC/GCC/Clang、架构或 CRT 的 universal binary ABI。
推荐按目标工具链重新构建。

V3.1.0 仍保留给必须继续使用 Path/Tree 契约的应用。

详见：

- [英文完整集成指南](INTEGRATION.md)
- [兼容性](COMPATIBILITY.md)
- [API](API.md)
- [V3 → V4 迁移](MIGRATION_V3_V4.md)
- [4.1.0 更新记录](V4_1_0.md)
- [v4.0.0 Stable 历史记录](V4_STABLE.md)
