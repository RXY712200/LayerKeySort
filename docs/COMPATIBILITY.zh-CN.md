# LayerKeySort V4 兼容性

[English](COMPATIBILITY.md)

[V4 冻结契约](V4_FREEZE.md)定义 4.x 的源码接口、布局和持久格式。V4.0.0 Stable 在 RC.1 独立审核后沿用了 Preview.5 的冻结接口。[Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) 是当前已发布的 Stable，属于兼容的小版本存储优化。它保留冻结 API、LS1/LKS4SNP1 字节及严格的 V3 LK1 迁移。

## 源码兼容

受支持的接口由 V4 公开头中的 64 个函数和 14 个类型界定。4.x 不得静默重命名或移除公共函数、修改公共结构字段或布局契约、重编号状态、改变文档规定的 Handle 或 Cursor 生命周期，或改变 Group/Batch 的排序语义。应用应按编译器、架构和 CRT 组合重新编译源码。

## 持久格式兼容

LS1 版本 1 和 LKS4SNP1 的持久契约强于库的二进制 ABI。4.x 中既有有效字节保留文档规定的含义。未来不兼容的格式需要新家族或新版本。严格的已发布 V3 LK1 解释仅通过 `lks_order_import_v3_lk1` 用于迁移。

## 二进制 ABI

CMake 显式构建 STATIC 库，即使设置 `BUILD_SHARED_LIBS` 也不提供共享 DLL/SO ABI。在 MSVC 与 GCC/Clang、不同架构、CRT 或互不相关的编译器 ABI 之间均无通用二进制兼容承诺。源码重建是正常集成方式。

## 符号模型

受支持的 API 由公开头和清单定义。归档导出的符号还可能包括私有 allocator、sort、snapshot 帮助函数以及用于严格 LK1 迁移的历史 Path 编解码。经核查的 GCC V4.0.0 构建中，`nm` 列出 64 个公开及 43 个私有 `lks_*` 文本符号。私有对象不恢复已退役的实时 Tree/OrderedTree、Path-gap 或历史 Group 公共契约。安装版的调用者不得声明或调用私有符号。

## 4.x 可以改变的部分

冻结语义保持不变的前提下，块容量及策略、索引拓扑、分配器实现、内部函数划分和诊断均可演进。B64/最小 32 因此属于实现策略。

## V3 兼容边界

V4 相对 V3 存在主版本源码不兼容。需要实时 Path/Tree 契约的应用仍可单独使用 V3.1.0。参见[迁移指南](MIGRATION_V3_V4.md)。

## 未来主版本

按周期安排 V5/V6 没有必要。只有冻结的 4.x 契约内无法妥善处理的重要架构或兼容问题才足以推动主版本。当前路线见 [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。另见 [Stable 记录](V4_STABLE.md)与 [RC 验证记录](V4_RC1.md)。
