# Windows 祖先比较筛选（历史译本）

原文：[RESULTS.md](RESULTS.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.1/RESULTS.md)。原始结果保持不变，以下“尚未”指当时筛选；[双语导航](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

Full源码 `c108acd72b37614f4be271c9747484c2dde5eac4` 的src、include、cmake及CMakeLists.txt与已发布v4.1.0相同。detached实验翻译单元ancestor-order.c.txt只替换比较路径，不在生产清单。

既有比较计算两个绝对块rank；实验从AVL高度较低块向上走。高度沿每条父边严格增长，故一条行走不会在另一条到达共同祖先前越过该祖先。成为兄弟时按左子关系判断，提升时直接到另一当前块则按子侧判断。同块仍比local。不需额外每块/每resident存储。块标签仅作为固定状态ordinal下限，不测维护或建立可用标签设计。

100,000 resident的跨块中位探针：公开99.441ns、祖先39.365ns、固定ordinal载入2.459ns；同块分别3.131和3.112ns。原型在筛选可内联，公开API外部调用；混合回放则各实现与harness分开编译且不用LTO。

混合筛选用45份冻结轨迹、每变体/轨迹八次进程，丢弃repeat zero。保留C harness验证比较成功及最终顺序。候选/对照逐条件比率中位数为compare-heavy 0.54637、整体0.91091，最差1.02697。整体降低8.9%未达任务15%规划目标，仅为有潜力而综合证据不足的比较修改。本次交付尚未选择生产候选；不能从本筛选推导编译器/语言归因、写路径、延迟尾部或生命周期性能。

原始行为screen.csv和mixed-screen.csv。初始归档审计检查40份文件摘要和45份轨迹成员摘要，归档分支保留Windows原始换行。这是本地Windows/GCC证据，当时同时代v4.0.0/OM对照、Linux复制、churn状态剖析和候选正确性/分发门槛仍待完成。
