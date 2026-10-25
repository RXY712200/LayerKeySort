# LayerKeySort

[English](README.md) | 简体中文

**面向持续变化的集合顺序，提供多个彼此独立的 C17 动态顺序维护库。**

LayerKeySort 解决的是集合长期使用时不断插入、删除、移动实例的顺序管理问题，而不是 `qsort` 一类的一次性排序。业务对象及业务身份由应用自己管理；每个产品拥有独立的顺序管理契约。

## 选择适合你的产品

| | **[Full](docs/FULL.zh-CN.md)** | **[Mini](mini/README.zh-CN.md)** |
| --- | --- | --- |
| 当前 Stable | **[v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)** | **[v1.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0)** |
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
| Full | [Full 专属 README](docs/FULL.zh-CN.md) · [中文集成](docs/INTEGRATION.zh-CN.md) · [示例](examples/README.md) | [API](docs/API.md) · [V4 冻结契约](docs/V4_FREEZE.md) | [Full v4.1.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) |
| Mini | [Mini 专属 README](mini/README.zh-CN.md) · [使用指南](mini/docs/USAGE.md) · [示例](mini/examples/basic.c) | [Mini v1 公开契约](mini/docs/CONTRACT.md) | [Mini v1.0.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0) |

下载 Mini 时请使用 Release 中的 **Mini-only 源码 ZIP**；GitHub 自动生成的 Source code 压缩包包含**整个仓库**。Full 则另有两文件 amalgamation 分发包。具体集成方法以各自产品文档为准；根目录 CMake 仍默认构建 Full。

## 仓库目录说明

- `include/`、`src/`、根 `CMakeLists.txt`、`tests/`、`benchmarks/` 和大部分 `docs/`：**Full**。
- `mini/`：独立的 **Mini** 实现、文档、测试和构建系统。
- `consumers/layer-order-workbench/`：**Mini 消费者工程及验证程序**，不是 Mini 生产源码，也不是第三套排序库。
- `.github/`、部分 `tools/` 和[产品线政策](docs/PRODUCT_LINES.zh-CN.md)：仓库治理和产品专属维护工具。

未来可能研究 Embedded/MCU 版本，但**目前只是构想**，没有已发布的 Embedded 产品、API 或交付承诺。

## 版本、兼容性与维护

- Full 的已发布标签使用 `v*`（当前 Stable：`v4.1.0`）。
- Mini 的已发布标签使用 `mini-v*`（当前 Stable：`mini-v1.0.0`）。
- GitHub 仓库级 **Latest** 不等于各产品各自的最新版本；请使用上表的独立发布页面。
- Full 早期版本和 V1–V4 研究属于保留的历史资料，不代表 Mini 的契约。
- 许可证：[MIT](LICENSE)。各产品独立维护自己的文档及验收标准。

跨产品修改前请阅读[产品线与发布管理规范](docs/PRODUCT_LINES.zh-CN.md)（[English](docs/PRODUCT_LINES.md)）及[贡献指南](CONTRIBUTING.md)。

## 路线图与验证资料

- **Full：** [Full 当前路线与技术债 Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)；[历史问题台账 Issue #4](https://github.com/RXY712200/LayerKeySort/issues/4)。
- **Mini 与其消费者：** [Mini 路线图 Issue #7](https://github.com/RXY712200/LayerKeySort/issues/7)。
- **Full 示例与验证：** [Full README](docs/FULL.zh-CN.md) · [验证](docs/VALIDATION.md)。
- **Mini 示例与验证：** [Mini README](mini/README.zh-CN.md) · [开发与验证](mini/docs/DEVELOPMENT.md)。

## 外部项目目录

- [Quiveld — 作者的独立项目目录](https://quiveld.com/projects/independent/)。

不要从功能表推断实际采用比例、所有平台均受支持或某一产品拥有普遍性能优势。请结合实际操作负载和资源限制选型。
