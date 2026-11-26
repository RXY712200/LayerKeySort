# V3 Preview.1 对稳定V2直接比较的元数据（历史译本）

原文：[v3-preview1-metadata.md](v3-preview1-metadata.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v3-preview1-metadata.md)。原始记录保持不变；以下陈述对应采集时状态。[双语导航](../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

- 日期：2026-10-01（Asia/Shanghai）。
- 机器：Windows 11，AMD Ryzen 9 9955HX，32个逻辑处理器。
- 编译器：两个检出版本均为MSYS2 UCRT64 GCC 16.2.0。
- 构建：CMake Release，`LKS_BUILD_TESTS=OFF`、`LKS_BUILD_BENCHMARKS=ON`；两构建实际生产标志均为 `-O2 -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG -std=c17`。
- Stable源码：干净、detached的v2.0.0，提交 `9fb7a9b0ae8d71cbd7410822e37702f6a6dc4d88`。
- V3源码：v3-preview1-core-architecture开发分支，首选直接深度6、开放端直接允许深度8。
- 每行输入种子 `0x91A30D47`，一次预热、三次测量；中位数/最小/最大为墙钟毫秒。
- tree仅测比较器管理插入；输入创建、结果验证、销毁不在计时区间。V2调用 `lks_tree_insert_item`，V3调用 `lks_ordered_tree_insert`。
- 诊断构建与计时构建分开，启用 `LKS_ENABLE_ALLOC_DIAGNOSTICS`、`LKS_BENCH_DIAGNOSTICS`，标志为 `-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`。诊断耗时不得与CSV计时行比较。

结果是单机工作负载测量，不是普遍速度或复杂度保证。记录时V3 Preview.1尚未发布。
