# V3 Preview.2 直接比较元数据（历史译本）

原文：[v3-preview2-metadata.md](v3-preview2-metadata.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v3-preview2-metadata.md)。原始记录保持不变；这是历史测量状态。[双语导航](../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

- 日期：2026-10-01（Asia/Shanghai）。机器：Windows 11、AMD Ryzen 9 9955HX、32个逻辑处理器。
- 编译器：MSYS2 UCRT64 GCC 16.2.0，C17，经CMake Release使用 `-O2 -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG`。
- Stable源码：detached的v2.0.0，`9fb7a9b0ae8d71cbd7410822e37702f6a6dc4d88`。
- Preview.1源码：干净、detached的已发布标签，`6fd34ca709ded89e0784e7c33cf08a6603ac9df1`。
- Preview.2源码：v3-preview2-relabel-optimization工作候选，之后在不改变已测算法的情况下提交。
- 主要种子 `0x91A30D47`，留出验证种子 `0xDEADBEEF`；每行一次预热和三次测量，报告中位/最小/最大ms。
- tree仅测插入；输入生成、Tree创建、验证、销毁均在区间外。Stable V2使用 `lks_tree_insert_item`，V3使用 `lks_ordered_tree_insert`。
- 诊断二进制为单独的 `-O2 -DNDEBUG` 构建，使用 `LKS_ENABLE_ALLOC_DIAGNOSTICS` 和 `LKS_BENCH_DIAGNOSTICS`；其耗时不与计时CSV比较。Preview.1临时诊断构建只增加累计尝试区域及生成Path计数，detached源码worktree在编译后恢复干净。
- CSV和诊断日志均为单机观察，不建立最坏或摊销复杂度保证。

## 直接结果摘要

| 工作负载 | N | Stable V2中位ms | Preview.1中位ms | Preview.2中位ms |
| --- | ---: | ---: | ---: | ---: |
| 升序 | 100,000 | 303.922 | 362.676 | 54.104 |
| 全相等 | 100,000 | 303.207 | 362.617 | 53.426 |
| 32值重复 | 100,000 | 424.473 | 107.261 | 109.607 |
| 随机唯一 | 100,000 | 97.487 | 96.812 | 98.830 |
| 交替 | 10,000 | 189.293 | 75.047 | 73.228 |
| 降序 | 10,000 | 2.984 | 2.982 | 3.080 |
| 升序 | 300,000 | 3343.667 | 3616.003 | 505.805 |

主要重复值和随机中位数相对Preview.1回归约2.2%和2.1%；独立留出种子下Preview.2则改善两者。短降序测试约慢3.3%，差值约0.1ms，不能声称普遍改善。

升序100,000诊断：Preview.1尝试100次重标记，其中五次全范围，共涉及292,764个旧节点；全部尝试访问719,468个区域节点、生成719,634条候选Path。Preview.2没有尝试重标记、访问区域节点或生成重标记Path。其99,935次突发模式端点插入使用300,001次库分配，Preview.1为1,739,366；累计请求字节12,628,392对155,027,832；峰值存活字节10,702,928对22,181,704。平均/P95/P99深度由3.987/6/7变为2.051/3/4。比较器调用1,568,929、AVL旋转99,983及最终高度17不变。Stable V2该负载的九次物理重建共重建522,876个节点。

随机100,000在两个V3版本中仍有昂贵重标记：一次涉及4,230个旧节点的全范围操作仍存在。Preview.2重标记旧节点总数由8,606降为6,302；两者比较器调用1,544,756、旋转69,754及最终高度20。交替10,000的119,789个旧节点工作量不变，包含一次189节点全范围重标记。

端点收益不保证重标记消失。单独升序300,000诊断中，Preview.2触发一次261,578节点全范围重标记，Preview.1为20次、涉及3,319,695旧节点。Preview.2仍访问523,714个尝试区域节点，重标记前短暂达到候选深度9。这是真实的单操作成本陡增，仍是当时核心性能问题。

完整分配、深度、失败尝试和其他分布见v3-preview2-diagnostics.txt。先前Preview.1发布测量保留在原文件，未被覆盖。
