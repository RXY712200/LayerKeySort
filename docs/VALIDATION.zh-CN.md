# V4 Stable 验证

[English](VALIDATION.md)

4.2 候选版应检出 `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`，其中包含 RC 元数据和两个 Preview.3 回归。较早的运行源码提交缺少这些测试。参见[候选定量证据](FULL_RC1_EVIDENCE.zh-CN.md)。

当前已发布 Stable 为 [LayerKeySort Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。4.x API 和线格式保持冻结，见 [4.1.0 说明](V4_1_0.md)。V4.0.0 Stable 在 Preview.5 冻结及独立审查 RC.1 后发布。Stable 发布保留冻结的生产实现和兼容契约，只改发布元数据与呈现。

## 当前自动验证门槛

启用基准和 Python 后，基线为 39 项 CTest。覆盖范围：

- 冻结的 64 函数、14 类型 API 清单；
- 显式顺序和托管实时顺序；
- Handle 与游标语义；
- Group 与 Batch 行为；
- 快照寿命和当前性；
- LS1 精确向量及错误输入；
- LKS4SNP1 精确字节及解析拒绝；
- 严格 V3 LK1 迁移；
- 确定性的内存不足及失败原子性；
- 文档规定的不分配路径；
- 契约范围内的标记寿命与并发；
- C/C++ 公开消费者；
- 分发、合并源码与打包验证。

CI 执行全部七个公开示例。

## 平台矩阵

已验证的发布配置包括 Windows MSVC x64、Windows MSVC Win32、Linux GCC、Linux Clang、Linux Clang ASan/UBSan、macOS AppleClang；实现采用 C17，消费者覆盖 C++17。未承诺支持每种 C17 编译器或平台，也未承诺通用二进制 ABI。

## 发布证据

RC 扩展测试包括三个种子、每个 200k 步的显式顺序修改、长时间对抗性解析、大规模远距移动和周转、来源标记寿命压力，以及针对性的 Preview.4 计数回放。

Preview.4 记录 V4 的历史收敛过程。后续 4.1.0 三步整合 [A/B/C 研究](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md)记录 1,548 个完成的原生进程、36 项安全跳过及 11,802 个确切顺序指纹，保留收益与回退。参见 [RC 记录](V4_RC1.md)及 [Stable 记录](V4_STABLE.md)。

## 验证范围

通过当前矩阵后，仍不应据此推导通用硬延迟上限、通用性能优势、任意分配器和操作系统行为、全部真实编辑器负载、可变容器的通用线程安全、分布式或 CRDT 正确性，以及整个库的形式化复杂度定理。

应用自己的延迟预算、任意运行时长下的分配器行为、更大的 sanitizer 或高保留量场景仍待验证。针对明确的工程问题重新验证。实际轨迹有帮助；继续内部研究无需外部采用。当前路线见 [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。

Stable 4.x 中冻结清单与固定样例不得静默修改。
