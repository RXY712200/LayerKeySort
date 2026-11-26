# LayerKeySort

## Full：Stable 与候选版

主分支和已发布 Stable 均为 **4.1.0**。需要可下载的源码或发行包时，请使用 [Stable 发行页](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。独立的 4.2 候选分支仍在审查，尚未合并、打标签或发布。Mini 使用独立的版本和发布路径。

Full 用稳定 Handle 表示实时顺序中的单次存活实例；应用持有业务对象、身份和事务。4.2 候选版在跨块比较中优先使用有界标签，必要时回退到 AVL。以下是 [固定候选证据](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/FULL_RC1_EVIDENCE.zh-CN.md) 中相对 Stable 4.1 的配对耗时比与耗时降幅：

| 负载 | Windows | Linux |
| --- | ---: | ---: |
| 全部实时顺序条件 | 0.629（降低 37.1%） | 0.640（降低 36.0%） |
| 比较密集条件 | 0.198（降低 80.2%） | 0.219（降低 78.1%） |

比值为候选耗时除以对照耗时。先分别计算同一平台每个匹配条件的耗时比，再取这些比值的中位数。初始规模为 1k、10k 和 100k 个实例。端点编辑和平面历史的生命周期对照中，平面组合更快。需要相对编辑与历史恢复时，先按目标负载比较。参见 [候选验证](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/V4_2_0_RC1.zh-CN.md) 与 [候选实现导读](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md)。另见[历史 V4 架构设计的完整中文译本](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/design/V4_ARCHITECTURE.zh-CN.md)。下文仓库内链接描述主分支的 Stable 基线。


[English](README.md) | 简体中文

**面向持续变化的集合顺序，提供多个彼此独立的 C17 动态顺序维护库。**

LayerKeySort 解决的是集合长期使用时不断插入、删除、移动实例的顺序管理问题，而不是 `qsort` 一类的一次性排序。业务对象及业务身份由应用自己管理；每个产品拥有独立的顺序管理契约。

## Full 核心实现：建议从这四份源码文档开始

**想知道 LayerKeySort Full 究竟是怎样实现的，而不只是照着 API 调用？** 这四份中文文档是理解 Full 内部机制的主要入口。它们以 **Full v4.1.0 的固定源码提交**为依据，覆盖全部 **16 个生产 C 模块**及其主要执行路径；**不适用于 Mini**，也不替代公开头文件和冻结的兼容性契约。

1. **[① 完整实现导读：从源码理解 Full](docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md)** — **建议先读。** 从整体架构和数据模型入手，解释 Block、AVL、稳定 Handle 如何维护动态顺序，并沿实际调用链梳理插入、移动、删除、ManagedOrder、Snapshot、LS1、序列化、恢复以及新旧代码的分工。
2. **[② 公开 API 索引与关键使用契约](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md)** — 列出 **64 个公开函数**及对应源码，集中说明必填参数、错误返回、对象所有权、Handle/Cursor 生命周期、回调限制、并发边界与遍历示例。适合实际调用前查阅。
3. **[③ 内部数据结构与算法实现详解](docs/IMPLEMENTATION_INTERNALS.zh-CN.md)** — 深入 **16 个生产 C 模块**的主要内部函数：分块与 AVL 维护、块拆分和修复、移动的不同分支、预分配与提交、批量恢复、稳定归并、内存诊断，以及 V3 LK1 兼容过程。适合想研究或修改算法的人。
4. **[④ 测试、验证与可复现审计指南](docs/IMPLEMENTATION_VALIDATION.zh-CN.md)** — 将关键行为对应到具体测试函数与断言，说明 CMake 构建、API 和 Wire 固定样本、CI、分发验证及如何复现；同时明确区分“读过测试”和“实际执行测试”。

**阅读顺序：** 想完整研究 Full，推荐按 **① → ② → ③ → ④** 阅读；如果主要想在自己的 C/C++ 程序里使用 Full，可以先看 **②**，结合 [Full 产品与集成指南](docs/FULL.zh-CN.md)实践。文档基于明确的源码 SHA，不宣称形式化证明了所有代码或所有环境下的正确性。

## 选择适合你的产品

| | **[Full](docs/FULL.zh-CN.md)** | **[Mini](mini/README.zh-CN.md)** |
| --- | --- | --- |
| 最新**已发布** Stable | **[v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)** | **[v1.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0)** |
| 当前 `main` 源码 | **4.1.0**（[已发布更新](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/V4_1_0.zh-CN.md)） | **1.0.0**（未改动） |
| 主要定位 | 更完整的顺序管理及历史工作流 | 轻量本地可变序列 |
| 实时顺序管理 | 相对插入/移动、稳定实例句柄、可选比较器管理 | 相对插入/移动、稳定 occurrence 句柄 |
| 任意两实例实时顺序比较 | 按 Full 契约支持 | 最坏 O(n) |
| 历史快照与持久化 | 不可变快照、LS1、独立 Wire 格式与恢复 | 不提供 |
| Group / Batch 工具 | 提供 | 不提供 |
| 核心实现 | 有界分块与平衡索引 | 独立的非侵入式双向链表 |
| 公开 API | 64 函数（Full v4） | 18 函数（Mini v1） |
| 本仓库构建入口 | 根目录 `CMakeLists.txt` | `mini/CMakeLists.txt` |

**Full 与 Mini 是同一个项目家族的独立产品，不存在父子依赖关系。** 二者版本号、公开 API、兼容性要求及发布周期分别管理，互不链接对方的生产源码。

表格比较的是**能力，不是性能排名**，也不表示任何产品在所有场景下更快。只需 Mini 的轻量功能时选择 Mini；需要更完整的排序管理、快照或批量工具时选择 Full。

## 从对应产品开始

| 产品 | 入门和使用 | API / 契约 | 正式下载 |
| --- | --- | --- | --- |
| Full | [Full 专属 README](docs/FULL.zh-CN.md) · [中文集成](docs/INTEGRATION.zh-CN.md) · [示例](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/examples/README.zh-CN.md) | [API](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/API.zh-CN.md) · [V4 冻结契约](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/V4_FREEZE.zh-CN.md) | [已发布 v4.1.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) · [4.1.0 更新](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/V4_1_0.zh-CN.md) |
| Mini | [Mini 专属 README](mini/README.zh-CN.md) · [使用指南](mini/docs/USAGE.md) · [示例](mini/examples/basic.c) | [Mini v1 公开契约](mini/docs/CONTRACT.md) | [Mini v1.0.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0) |

下载 Mini 时请使用 Release 中的 **Mini-only 源码 ZIP**；GitHub 自动生成的 Source code 压缩包包含**整个仓库**。Full 则另有两文件 amalgamation 分发包。具体集成方法以各自产品文档为准；根目录 CMake 仍默认构建 Full。

## 仓库目录说明

- `include/`、`src/`、根 `CMakeLists.txt`、`tests/`、`benchmarks/` 和大部分 `docs/`：**Full**。
- `mini/`：独立的 **Mini** 实现、文档、测试和构建系统。
- `consumers/layer-order-workbench/`：**Mini 消费者工程及验证程序**，不是 Mini 生产源码，也不是第三套排序库。
- `.github/`、部分 `tools/` 和[产品线政策](docs/PRODUCT_LINES.zh-CN.md)：仓库治理和产品专属维护工具。

未来可能研究 Embedded/MCU 版本，但**目前只是构想**，没有已发布的 Embedded 产品、API 或交付承诺。

## 版本、兼容性与维护

- Full 的已发布标签使用 `v*`（当前已发布 Stable 为 `v4.1.0`）。
- Mini 的已发布标签使用 `mini-v*`（当前 Stable：`mini-v1.0.0`）。
- GitHub 仓库级 **Latest** 不等于各产品各自的最新版本；请使用上表的独立发布页面。
- Full 早期版本和 V1–V4 研究属于保留的历史资料，不代表 Mini 的契约。
- 许可证：[MIT](LICENSE)。各产品独立维护自己的文档及验收标准。

跨产品修改前请阅读[产品线与发布管理规范](docs/PRODUCT_LINES.zh-CN.md)（[English](docs/PRODUCT_LINES.md)）及[贡献指南](CONTRIBUTING.md)。

## 路线图与验证资料

- **Full：** [Full 工程路线与三大步骤完成记录 Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)；[历史问题台账 Issue #4](https://github.com/RXY712200/LayerKeySort/issues/4)。
- **Mini 与其消费者：** [Mini 路线图 Issue #7](https://github.com/RXY712200/LayerKeySort/issues/7)。
- **Full 示例与验证：** [Full README](docs/FULL.zh-CN.md) · [验证](https://github.com/RXY712200/LayerKeySort/blob/701c1d989541928868101991861d340f44574275/docs/VALIDATION.zh-CN.md)。
- **Mini 示例与验证：** [Mini README](mini/README.zh-CN.md) · [开发与验证](mini/docs/DEVELOPMENT.md)。

## 外部项目目录

- [Quiveld — 作者的独立项目目录](https://quiveld.com/projects/independent/)。

不要从功能表推断实际采用比例、所有平台均受支持或某一产品拥有普遍性能优势。请结合实际操作负载和资源限制选型。
