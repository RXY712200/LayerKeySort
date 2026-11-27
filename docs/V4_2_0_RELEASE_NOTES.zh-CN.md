# LayerKeySort v4.2.0

已发布 Stable/Latest：[Full v4.2.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.2.0)，发布日期 2026-11-27。[C17 源码 ZIP](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-amalgamation.zip) · [SHA-256 清单](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-SHA256SUMS.txt)。发行源码：`v4.2.0`／`f751e1ee2afd682874281d6dc495969f9eca49eb`。[历史 4.1 发行](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。

Full v4.2.0 是兼容的 C17 更新，面向动态顺序、稳定指针数组排序、不可变有序批次及历史快照。保留 64 函数／14 类型源码 API、LS1 和 LKS4SNP1、稳定实例 Handle 及调用者拥有的载荷。

## 改进

- 普通修改只更新受影响的记录位置。
- 跨块先后使用有界标签，无标签时按高度／父指针进行 AVL 回退；局部结构工作仍由块策略及平衡索引维护约束。
- 快照线格式采用可移植固定宽度转换，保持精确字节和先验证后分配。
- 偏移一字节线格式和恢复后混合标签回归增强保留实现的覆盖。
- 完整双语使用及实现指南说明实际工作流、共享对象修改、并发、失败恢复、分发及兼容。

## 证据

未变运行实现的 Windows/Linux 配对归档测得 Full/4.1 总体耗时比中位数为 0.629106/0.640047，比较密集为 0.197718/0.219173；每平台胜出 45/45。这些结论适用于具体对照与负载。原生标签区间 OM 在若干更新密集条件胜出；端点平面历史组合在对应生命周期比较更快，Treap 批量恢复未测。没有新的 Stable 基准。见[完整证据](https://github.com/RXY712200/LayerKeySort/blob/f751e1ee2afd682874281d6dc495969f9eca49eb/docs/FULL_RC1_EVIDENCE.zh-CN.md)。

验证覆盖严格 C17、API／线格式向量、故障注入、七个示例及 C/C++ 源码／安装／合并版消费者。最终集成源码和重建资产绑定下方确切提交；CI 运行链接列于发布清单。

## 源码及资产

从已发布标签 `v4.2.0` 或下方确切发行提交取得源码。准备好的资产为 `LayerKeySort-4.2.0-amalgamation.zip` 和 `LayerKeySort-4.2.0-SHA256SUMS.txt`。ZIP 含 `layerkeysort.h`、`layerkeysort.c`、`example.c`、`LICENSE`、`README.txt`。实现按 C17 编译，再链接 C/C++ 调用方。审查用准备 ZIP 的 SHA-256 为 `1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb`。本校验值已从下方确切集成提交重新构建并验证。

## 实际边界

逐实例分配、块空余、回调、O(N+A) 历史导出、保留快照及恢复临时数组仍有成本。应用管理身份、事务、回调副作用与同步。不提供普遍速度优势、端到端硬延迟、通用二进制 ABI、数据库或 CRDT。Mini 与 Workbench 不变，下游依赖采用另行决定。

## 确切源码及资产校验值

源码提交：[`f751e1ee2afd682874281d6dc495969f9eca49eb`](https://github.com/RXY712200/LayerKeySort/commit/f751e1ee2afd682874281d6dc495969f9eca49eb)。

```text
1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb  LayerKeySort-4.2.0-amalgamation.zip
36e06a53ad41f0a01ee0435e5c81ec81b8611b61df2351f5cf322ea1acd6f68e  LayerKeySort-4.2.0-SHA256SUMS.txt
```

## 最终目标验证

确切发行目标的六个 C17 配置均通过 CTest 39/39：MSVC x64／Win32、Linux GCC、Linux Clang、Linux Clang ASan/UBSan 及 macOS AppleClang。Mini、Workbench、测量证据检查及 Pages 也全部成功。

- [45832456582](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456582)
- [45832456787](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456787)
- [45832455183](https://github.com/RXY712200/LayerKeySort/actions/runs/45832455183)
- [45832456709](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456709)
- [45832456797](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456797)
