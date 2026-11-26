# V3 Preview.3 验证采集（历史译本）

原文：[v3-preview3-validation-metadata.md](v3-preview3-validation-metadata.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v3-preview3-validation-metadata.md)。保留原始记录。[双语导航](../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

源码提交 `8db22a831506f70163c419bc6d931063df884631`。本采集支持Preview.4面向用户的性能指导；Preview.4不改变被测排序算法。

机器为Windows 11 Pro 10.0.26300、AMD Ryzen 9 9955HX（16核/32逻辑处理器）、约16GB内存。MSYS2 UCRT64 GCC 16.2.0，C17、`-Wall -Wextra -Wpedantic -Werror`。计时使用 `-O3 -DNDEBUG`；独立插桩诊断使用 `-O2 -DNDEBUG -DLKS_ENABLE_ALLOC_DIAGNOSTICS -DLKS_BENCH_DIAGNOSTICS`。生产src/*.c与benchmarks/current_benchmark.c一起编译。除另述外种子为 `0x91A30D47`。

[计时CSV](v3-preview3-validation-timed.csv)覆盖七种Tree负载的100k、300k、500k、1m。前两规模一次预热三次测量，500k一次预热两次测量，1m不预热仅一次。CSV记录中位与范围，因此1m时间仅为观察值，不是稳定中位估计。定义见current_benchmark.c：alternating交错低/高键向中心靠近；hotspot持续在一个固定最低键项之后插入。计时排除输入准备、输出验证和销毁。

[诊断日志](v3-preview3-validation-diagnostics.txt)每条件单独运行一次。relabel计成功重标记及累计旧坐标替换，同一旧节点可被重复计数。footprint给最终存活Path字节以及display/LK1 C字符串大小，每条含一个NUL。diag含深度平均/P95/P99、分配器峰值等私有计数；顺序见[benchmarks/README.md](../README.md)。诊断时间不可与计时CSV比较。

1m下第二种子 `0xDEADBEEF` 对随机唯一产生一次全范围重标记，对32值重复产生两次，与主要种子次数相同。其他计数变化：随机成功1,208次、影响25,383旧坐标；重复成功70,204次、影响3,049,306。这是留出样本观察，不是穷举种子研究。

额外逐插入probe在计时harness之外用C17 timespec_get包围每次 `lks_ordered_tree_insert()`，使用同一优化源码/负载。交替100k/300k/500k/1m的最大单插入延迟约24/240/308/723ms；升序1m约654ms，重复1m约1,078ms。交替1m有55次超过100ms，单独插桩运行将56次全范围重标记关联到长暂停。逐调用计时含测量开销和调度噪声，仅描述一个本地环境，不定义公开延迟保证。

该采集显示真实大规模取舍，不建立正式摊销/最坏界、通用速度排名或固定Path深度/内存上限。
