# Preview.2 初步修改筛选（历史译本）

原文：[README.md](README.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v4-preview2-smoke/README.md)。保留原始采集及文字；[双语导航](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

一次Windows x64 GCC 16.2 Release采集，2026-10-06（Asia/Singapore）。源码4.0.0-preview.2、B128、确定种子17、20000居民及测量操作。这是合理性筛选，不是最终性能验收。

- production.csv为普通库的计时和比较器调用；零诊断列表示不可用，不表示没有结构工作或分配。
- diagnostics.csv为单独插桩库，包含分配与局部/索引工作；其时间含插桩，不与生产混用。

启用benchmarks，运行layerkeysort_v4_mutation_smoke及layerkeysort_v4_mutation_diagnostics，不传参数可复现完整负载；参数smoke使用固定的小型CI条件。QPC包含逐操作时钟/检查开销；最近索引p50/p99和max是一次运行的样本，不是实时上界，极短样本可舍入为零。

harness验证居民数量/遍历和managed已排序/稳定相等；独立精确move/managed参考在单独回归套件。操作定义、分配/计数解释及限制见[完整方法](../../../docs/V4_PREVIEW2.md)。硬件/OS调度及分配器影响未控制，不能视为通用排名。
