# LayerKeySort

此分支为实验性 `v4.0.0-preview.2`：新增保留句柄的移动、比较器管理顺序和修订检查游标。
参见 [Preview.2 指南](docs/V4_PREVIEW2.md)、[显式顺序示例](examples/live_order.c)
和[管理顺序示例](examples/managed_order.c)。快照、导出和持久化尚未实现，V4 API 尚未冻结。
最新公开稳定版仍为 `v3.1.0`；下文的 Path 介绍适用于保留的 V3 实现。

[English](README.md) | 简体中文

LayerKeySort 是一个 C17 库，用来维护会不断插入、删除或调整位置的集合顺序。

例如，在图层列表中加入 Effects：

```text
调整前：Background  Player  HUD
调整后：Background  Player  Effects  HUD
```

普通排序回答“现在谁排在前面”；如果顺序经常变化，数组下标就不适合充当长期位置。LayerKeySort 为元素提供可比较的**排序坐标**，也能在现有元素之间安排新位置。

这个坐标叫 `Path`。`Path` 表示位置，**不是元素的永久 ID**。库可以根据比较器管理坐标，也可以由应用自行选择坐标来移动元素。

**当前稳定版本为 [v3.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v3.1.0)。** 此前的 [v3.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v3.0.0) 等版本仍可获取。

## 获取 LayerKeySort

| 你的需求 | 建议从这里开始 |
| --- | --- |
| 先试运行 | 构建下面的小示例 |
| 向现有 C 项目放入源文件 | 使用两文件合并版（amalgamation） |
| 在 CMake 项目中获取依赖 | 用 `FetchContent` 固定 Release tag |
| 在本机安装后复用 | 用 `cmake --install` 和 `find_package` |
| 将完整仓库放进项目 | 用 `add_subdirectory` |

可从 [GitHub Releases](https://github.com/RXY712200/LayerKeySort/releases) 下载 `LayerKeySort-3.1.0-amalgamation.zip`。也可在当前源码检出中运行 `python tools/amalgamate.py --package-parent build/amalgamation` 生成它。GitHub 自动提供的源码压缩包包含完整仓库；合并版 ZIP 是更轻量的两文件接入方式。

ZIP 内文件如下；大多数不使用 CMake 的项目只需要复制其中的 `layerkeysort.h` 和 `layerkeysort.c`：

```text
LayerKeySort-3.1.0-amalgamation/
├── layerkeysort.h
├── layerkeysort.c
├── example.c
├── LICENSE
└── README.txt
```

在应用中写 `#include "layerkeysort.h"`，并把实现文件与应用一起按 C17 编译：

```sh
cc -std=c17 -I. layerkeysort.c main.c -o my_app
```

在 MSVC Developer Command Prompt 中使用 `cl /std:c17 /I. layerkeysort.c main.c`。下载并使用生成好的包**不需要 Python**；仓库维护者生成包时才需要。配套的 `LayerKeySort-3.1.0-SHA256SUMS.txt` 记录 ZIP 的校验值。

如果项目已有 CMake，可以链接源码构建目标 `layerkeysort`：

```cmake
include(FetchContent)
FetchContent_Declare(layerkeysort_source
    GIT_REPOSITORY https://github.com/RXY712200/LayerKeySort.git
    GIT_TAG v3.1.0)
FetchContent_MakeAvailable(layerkeysort_source)
target_link_libraries(my_app PRIVATE layerkeysort)
```

应固定 `v3.1.0` 等 Release tag 或具体提交，不要依赖会移动的开发分支。`my_app` 应已是 C17 CMake 项目中的目标，CMake 版本需至少 3.21。完整配置、安装和直接编译方式见[中文集成指南](docs/INTEGRATION.zh-CN.md)。

## 先运行一个示例

在仓库检出目录中构建[比较器管理的 Tree 示例](examples/ordered_tree.c)：

```sh
cmake -S . -B build/quickstart -DLKS_BUILD_TESTS=OFF
cmake --build build/quickstart --target layerkeysort_ordered_example
```

类 Unix 系统运行 `./build/quickstart/layerkeysort_ordered_example`。Windows 单配置构建运行 `build\quickstart\layerkeysort_ordered_example.exe`；Visual Studio Debug 构建运行 `build\quickstart\Debug\layerkeysort_ordered_example.exe`。示例插入三个元素，查找其中一个并将其删除，输出：

```text
Found: Middle
After removal: Low < 20 < High
```

显式调整图层位置见[图层列表示例](examples/layer_list.c)；只需对指针数组做一次稳定排序，见[排序示例](examples/basic.c)。[V3 可视化工具](https://rxy712200.github.io/LayerKeySort/)可查看记录的顺序变化，无需先编译 C 程序。

## 选哪个 API？

| 任务 | 从这里开始 |
| --- | --- |
| 集合变化时仍保持比较器顺序 | [`LksOrderedTree`](examples/ordered_tree.c)：库分配 `Path`；比较结果相等的元素保留插入顺序 |
| 应用自己选择位置、显式移动 | [`LksTree`](examples/layer_list.c)：应用负责选择和更换 `Path` |
| 只做一次稳定的指针数组排序 | [`lks_sort()`](examples/basic.c)；不需要稳定性时，普通排序可能更简单 |
| 构建或合并不可变排序批次 | [Group / GroupBatch](docs/USAGE.zh-CN.md)：面向批处理的进阶 API |

`LksOrderedTree` 创建时绑定比较器。元素中影响比较结果的字段要改变时，应先移除、修改，再重新插入。`LksTree` 不强制比较器顺序，由应用管理坐标。

## 适合什么场景？

当元素需要反复插入到现有元素之间，或变化中的集合需要维持比较器顺序时，LayerKeySort 值得考虑。以下情况往往有更简单的选择：

- 只排序一次数组：使用普通排序；确实需要稳定的指针数组排序时可用 `lks_sort()`。
- 偶尔调整数据库排名，只需要简短排名字符串：更简单的分数排名方案可能开销更低。
- 需要不可变位置 ID 或分布式／CRDT 收敛：`Path` 会变化，库不协调独立副本。
- 每次插入都需要已证明的时间上界或稳定的同步低延迟：托管插入可能一次重标记大量 `Path`，应先测自己的负载。

[英文基准报告](docs/BENCHMARKS.md)同时记录较慢负载和大范围重标记；它不是普遍速度保证。

## 使用前记住的模型与限制

`Path` 是排序坐标。应用应另存元素 ID：`item identity != Path identity`。Tree **借用**调用方持有的元素指针，不复制、也不释放元素。

`LksOrderedTree` 按比较器决定逻辑顺序并分配 `Path`；空间拥挤时，插入可能重标记（relabel）已有坐标。实际成功修改 Tree 后，应重新获取借用的节点、`Path` 和导航结果。`LksTree` 则由应用选择坐标，按坐标插入、删除或 rekey。

`lks_path_compare()` 是 `Path` 顺序的依据。AVL 的物理父子关系只是内部索引结构。可读的 `Path` 文本用于展示；单个坐标也可用独立版本的 `LK1:` 键持久化并按 ASCII 字节序排序。它们都不保存整个 Tree，也不赋予元素永久身份。

- 托管插入可能重标记很大范围，甚至整个集合，造成与负载有关的同步尾延迟。`Path` 深度、内存占用和 LK1 长度也取决于负载。
- 完整托管插入**没有**已证明的最坏情况 `O(log n)` 保证，也没有形式化摊还界。
- 多线程共享可变对象需要外部同步。调用方持有的元素和比较器上下文必须在约定的生命周期内有效。
- 库不提供整个 Tree／元素数据序列化或分布式冲突解决。

## 继续阅读

| 想了解 | 文档 |
| --- | --- |
| 如何获取和接入库 | [中文集成指南](docs/INTEGRATION.zh-CN.md) |
| 如何使用主要 API | [中文使用指南](docs/USAGE.zh-CN.md) |
| 精确函数签名、所有权、错误与 LK1 规则 | [英文 API 参考](docs/API.md) |
| 两种 Tree 与重标记的实现 | [英文架构说明](docs/ARCHITECTURE.md) |
| 3.x 兼容性边界 | [英文兼容性约定](docs/COMPATIBILITY.md) |
| V2 到 V3 的变化 | [英文迁移指南](docs/V3_MIGRATION.md) |
| 性能证据与局限 | [英文基准报告](docs/BENCHMARKS.md) |
| 测试、开发和版本历史 | [英文验证](docs/VALIDATION.md) · [开发](docs/DEVELOPMENT.md) · [更新记录](CHANGELOG.md) |
| 早期设计与测量 | [英文设计历史](docs/history/DESIGN_HISTORY.md) · [基准历史](docs/history/BENCHMARK_HISTORY.md) |

公共头文件是 [`include/layerkeysort.h`](include/layerkeysort.h)。3.1.0 延续稳定 V3 的源码、API 和语义兼容边界；涉及精确技术约定时，以公共头文件及英文 API／兼容性文档为准。

## 许可证

LayerKeySort 使用 [MIT License](LICENSE)。
