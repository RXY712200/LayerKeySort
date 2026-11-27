# V4 Stable 维护

发行源码：[`v4.2.0`](https://github.com/RXY712200/LayerKeySort/tree/v4.2.0)，提交 `f751e1ee2afd682874281d6dc495969f9eca49eb`。下方 RC／Preview 测试 pin 为明确保留的历史验证基线。

[English](DEVELOPMENT.md)

Full v4.2.0 保持冻结的 V4 契约，沿用已接受的 4.2 实现：受影响槽位更新、有界块标签及 AVL 回退、固定宽度线格式转换。先读 [4.2 记录](V4_2_0.zh-CN.md)、[量化证据](FULL_RC1_EVIDENCE.zh-CN.md)及[冻结契约](V4_FREEZE.zh-CN.md)。[4.1 记录](V4_1_0.zh-CN.md)及原始 V4 Preview/RC 记录属于历史证据。当前工程问题仍见 [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。

## 4.x 维护规则

保留冻结的公开名称、签名及布局，显式状态数值，所有权及生命周期，LS1 v1 字节及域语义，LKS4SNP1 字节及解析语义。只要这些契约保持不变，可以进行兼容的正确性、可移植性、文档和内部性能修复。不兼容的修复须明确论证后续主版本决策。

## 私有实现可演进

B64/最小 32 块策略、AVL/块拓扑、分配策略、诊断计数及内部帮助函数划分属于实现细节。4.0.0 含有某项内部优化，本身不使其成为公开兼容契约。

## 必要验证

变更代码时按适用性执行 `tools/verify_v4_api.py`、严格 C17 构建和 CTest、全部七个公开示例、`tools/validate_distribution.py`、`tools/validate_release_assets.py`，以及相关 sanitizer 和平台检查。旧 V3 回归只用私有声明作为迁移和回归证据。

## 强制实现文档门槛

**Full 实现或契约的文档须与目标源码 SHA 一致，才可视为完成。** 同样适用于公开 API 不变的私有优化。不要推迟到后续 PR。只要变更 Full 生产代码、公开头、排序/所有权/错误语义、私有算法或结构、分配、线格式或恢复、构建或分发，就在同一 PR 审核全部四份源码指南：

| 指南 | 审查与同步 |
| --- | --- |
| [实现导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | 架构、执行步骤、调用链、相关模块、前后行为 |
| [公开 API 索引和契约](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | 全部符号与签名、行为、返回码、所有权、寿命、回调及源码映射；明确不受影响的 API |
| [内部算法和数据结构](IMPLEMENTATION_INTERNALS.zh-CN.md) | 私有帮助函数、不变量、块/AVL、内存、提交、OOM 和热路径 |
| [验证和源码证据地图](IMPLEMENTATION_VALIDATION.zh-CN.md) | 实际测试函数及断言、增减覆盖、执行命令、CI 链接、未验证行为和资源证据 |

1. 固定旧 Full 源码 SHA，核对目标 PR HEAD，将精确差异与受影响源码和测试对应。记录证据和未验证行为。
2. 逐份审核四份指南，更新受影响的解释和示例。确保适用源码 SHA、发布/开发状态和独立验证范围准确。指南行为或 API 映射未变时在矩阵明确记录，同时核对当前适用性和审核状态。
3. 同一 PR 更新 [Full 消费者交接台账](FULL_CONSUMER_HANDOFF.md) ([简体中文版](FULL_CONSUMER_HANDOFF.zh-CN.md))：新旧准确 SHA、函数/结构/调用路径、实际测试及数据库核心或中间适配器影响（无、重验、适配器改动、潜在核心改动或未知，写明原因）。数据库拥有存储、事务、持久身份和采用决定；Full 保持独立。
4. 按实际影响更新 API、架构、兼容、用法、集成、开发、README、示例、迁移或发布文档。保留历史 Stable/Preview/RC 基准、冻结向量和已发布事实。
5. PR 证据含四行同步矩阵，列指南、审核/更新或确认无变化的章节、受影响路径及函数、适用 SHA、测试和边界。实施后复核远端最终 HEAD。
6. 任一指南未审查或有误导、台账未匹配、数据库或适配器影响缺失、源码与文档 SHA 不一致，均不得合并。在同一步和同一 PR 内修复；绿色 CI 不解除文档门槛。

仅文档且无实现或契约变更时，按实际影响编辑并保留源码出处。正常 Full 开发仍需独立审核测试、兼容性与 GitHub 分支保护。

Full v4.2.0 运行源码仍为 `fe00536e7bd7f4e5128432ea4a29e54158074903`，RC 测试／头文件基线为 `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`。Stable 准备只更改版本宏和文档。OrdWeave 官方依赖保持 4.1.0。

## 文档真实性

V4 Preview 报告、V4 RC.1 报告、归档 V3 研究、已采集基准目录及旧 CHANGELOG 条目应保留当时的真实信息。当前 README、API、用法、架构、兼容、集成、验证和开发文档应区分已合并源码版本与最新已发布 Stable。更正当前陈述，保留历史事实。

## V4 之后的证据优先级

[4.2 配对证据](FULL_RC1_EVIDENCE.zh-CN.md)是当前性能入口。Preview.4 是 V4 历史开发证据。[Full 4.1.0 记录](V4_1_0.md)和[最终 A/B/C 研究](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md)为更新的内部验证，覆盖 100k–10m 及回退场景。固定三步优化周期已结束。后续内部研究可针对实际成本，无需等待外部采用；主版本仍需证明确有不兼容需求。

只有实际正确性、兼容性、用户或工作负载证据表明应该打破兼容，才考虑 V5/V6。Full、Mini、Embedded 长期构想不属于当前实现范围。
