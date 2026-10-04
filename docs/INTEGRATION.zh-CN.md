# 接入 LayerKeySort

[English](INTEGRATION.md) | 简体中文

本文介绍 LayerKeySort 3.1.0 的接入方式。可使用 `v3.1.0` Release tag、发布资源或固定的具体提交。升级自 V2 时另见[英文 V3 迁移指南](V3_MIGRATION.md)和[英文兼容性约定](COMPATIBILITY.md)。

| 方式 | 适用情况 | 接入时需要 CMake？ | 放入应用的文件 |
| --- | --- | --- | --- |
| 两文件合并版 | 小型源码直接接入 | 否 | `layerkeysort.h` 和 `layerkeysort.c` |
| `FetchContent` | CMake 源码依赖 | 是 | 下载的源码 |
| `add_subdirectory` | 将完整仓库放进项目 | 是 | 完整源码检出 |
| 安装后 `find_package` | 多个项目复用本机安装 | 是 | 本机构建的包 |
| 直接编译模块源码 | 特殊的手动构建 | 否 | 12 个生产 `.c` 文件及其私有头文件 |

库要求 C17；CMake 接入要求 CMake 3.21 或更新版本。CI 覆盖 Windows/MSVC、Ubuntu/GCC 与 Clang，以及 macOS/AppleClang；详见[英文验证记录](VALIDATION.md)。

## 两文件合并版

从 [GitHub Releases](https://github.com/RXY712200/LayerKeySort/releases) 下载 `LayerKeySort-3.1.0-amalgamation.zip`。GitHub 自动提供的源码 ZIP／tar.gz 包含完整仓库；合并版 ZIP 是更小的直接接入包，内含 `layerkeysort.h`、`layerkeysort.c`、`example.c`、`LICENSE` 和 `README.txt`。

解压后，将头文件和实现文件复制到应用中。使用 `#include "layerkeysort.h"`，按 C17 与应用一起编译：

```sh
cc -std=c17 -I. layerkeysort.c main.c -o my_app
```

`cc` 可以是 GCC 或 Clang。在 MSVC Developer Command Prompt 中：

```bat
cl /std:c17 /I. layerkeysort.c main.c
```

将 `main.c` 换成 `example.c` 即可编译包内示例。使用已生成的包**不需要 Python**。维护者才使用 Python 从规范的 `include/` 和 `src/` 源码生成它；不要编辑生成的 `layerkeysort.c`。

如需校验下载文件，可在 GNU/Linux 运行 `sha256sum LayerKeySort-3.1.0-amalgamation.zip`，或在 PowerShell 运行 `Get-FileHash .\LayerKeySort-3.1.0-amalgamation.zip -Algorithm SHA256`，并与同一 Release 的 `LayerKeySort-3.1.0-SHA256SUMS.txt` 比对。

## CMake FetchContent

下面是面向 `v3.1.0` Release tag 的完整消费者 `CMakeLists.txt`：

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

运行 `cmake -S . -B build`，再运行 `cmake --build build --target my_app`。作为 3.1.0 源码依赖时，测试、示例和安装规则默认关闭。应固定 Release tag 或具体提交。离线测试本地检出可配置 `-DFETCHCONTENT_SOURCE_DIR_LAYERKEYSORT_SOURCE=<checkout>`。

## 将完整仓库放入 CMake 项目

将 3.1.0 源码放在 `external/LayerKeySort`：

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_subdirectory(external/LayerKeySort)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE layerkeysort)
```

`layerkeysort` 目标会提供公共头文件目录。作为子项目时，测试、示例和安装规则默认关闭，无需另加 OFF 参数。构建树中也提供带命名空间的别名。历史 `v3.0.0` 的子项目默认值不同，且没有安装或合并版支持；接入该版本时请查看对应 tag 的 CMake 文件。

## 本机安装与 find_package

用 C17 工具链构建并安装 3.1.0 源码：

```sh
cmake -S . -B build/install -DLKS_BUILD_TESTS=OFF -DLKS_BUILD_EXAMPLES=OFF
cmake --build build/install
cmake --install build/install --prefix /some/prefix
```

多配置生成器应在构建和安装命令中使用相同的 `--config Release`。另一个 CMake 项目可写：

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES C)
find_package(LayerKeySort CONFIG REQUIRED)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE LayerKeySort::layerkeysort)
```

配置时运行 `cmake -S . -B build -DCMAKE_PREFIX_PATH=/some/prefix`，然后运行 `cmake --build build`。`CMAKE_PREFIX_PATH` 应指向安装前缀，而不是其中的 `lib/cmake` 目录。已安装目标为 `LayerKeySort::layerkeysort`；源码树示例链接 `layerkeysort`。这是本机构建的静态包，不是跨工具链的二进制 SDK。

## 直接编译模块化 C17 源码

若不能使用 CMake，且合并版也不合适，请将 `include/` 加入编译器头文件搜索路径，并编译全部 12 个生产源码：

```text
src/lks_alloc.c       src/lks_base.c
src/path.c            src/path_text.c
src/path_order_key.c  src/slot_codec.c
src/path_compare.c    src/gap.c
src/tree.c            src/group.c
src/sort.c            src/bulk.c
```

权威清单见[生产源码清单](../cmake/ProductionSources.cmake)。`src/*.h` 是生产源码依赖的私有头文件，不是应用头文件。生产构建不要定义 `LKS_ENABLE_ALLOC_DIAGNOSTICS` 或 `LKS_ENABLE_V1_REGRESSION_HELPERS`。历史 Visual Studio 工程是组合式诊断运行器，不是供复用的库目标。不用 CMake 时，两文件合并版通常更简单。

## C++ 消费者

公共头文件含 `extern "C"` 保护。C++ 应用可以包含该头文件并链接 C17 库，已安装消费者测试也覆盖这一用法。实现源码仍应按 C17 编译，不应按 C++ 编译。

## 构建选项与排错

以下是 3.1.0 的默认值：

| 选项 | 顶层构建默认值 | 作为子项目默认值 | 用途 |
| --- | --- | --- | --- |
| `LKS_BUILD_TESTS` | ON | OFF | 诊断测试及 soak |
| `LKS_BUILD_EXAMPLES` | ON | OFF | 公共 API 示例 |
| `LKS_INSTALL` | ON | OFF | 安装与导出规则 |
| `LKS_BUILD_BENCHMARKS` | OFF | OFF | 公共 API 基准程序 |
| `LKS_ENABLE_SANITIZERS` | OFF | OFF | 支持环境中的 ASan/UBSan |

找不到 `layerkeysort.h` 时，应链接上述目标，而不是将私有 `src/` 头文件作为应用接口。`find_package` 失败时检查安装前缀和 `CMAKE_PREFIX_PATH`。C++ 项目应链接按 C17 构建的库。LayerKeySort 借用调用方拥有的元素对象；其生命周期要求见[中文使用指南](USAGE.zh-CN.md)。持久化单个 Path 的规则见[英文 LK1 规范](API.md)。
