# Preview.1 实时顺序架构决定（历史译本）

原文：[V4_2_0_PREVIEW1_ARCHITECTURE.md](V4_2_0_PREVIEW1_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.md)。原件/决定保留，以下候选为当时状态；[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

Stable从AVL子树count导出两个绝对块rank；候选保持线程块、稳定resident、AVL，跨块提升较低高度侧，直到兄弟或直接祖先。高度向上严格增加，避免一侧在另一侧到达前越过共同祖先；按子方向决定先后。同块用local，子树count继续维护/诊断；managed搜索和bulk恢复拓扑/契约不变。

插入设新resident成员，从插入点更新local；普通删除清尾并更新后缀；跨块move更新目标及无需repair时的来源后缀。split/merge/redistribution改变成员，因此保留完整位置修复。可失败分配仍在发布前，对象大小不变。

研究专用动态64位中点块标签原型保留AVL回退。端点固定步长、内部split取中点；持续在内部anchor前插入至1,057次耗尽间隙。原型禁用该Order标签，并通过10,000插入比较验证，以放弃快速路径避免全局改标签。退化与额外块存储使工程价值仍未决，排除候选构建。

生产候选合并祖先比较和变更区间更新，共享紧凑稳定resident。经典标签可常数工作查询，因此本比较优化不建立对等或创新性。Snapshot/wire保持所有权和时序，接受前仍需跨主机等价语义测量。

## 先后不变量及先前设计

不同块最低共同祖先为C。任一到C前高度严格低于C，故提升较低侧不能在另一侧仍低于C时越过C。父提升直接到对方为祖先条件，否则到C的不同孩子，左先右后。旋转保持中序并重算高度/parent，bulk同样建立。总父步数≤两条根路径长度和，为M块O(log(1+M))；同块常数。

Dietz–Sleator、Bender等研究插/删下precedence，给机制基准；组件比较还依赖身份wrapper、遍历、可恢复失败、保留存储及负载。实测j-hui tag-range/list-range固定 `ac4b902d103beb30cee3316d20cd10701f52d15a`，保留MIT notice，为实用标签变体；不代表全部最优文献算法，也不为全部Full保证定价。

Full分离块顺序、resident身份、历史导出。普通移位只影响local且可不变成员，位置优化利用这一点。标签探针揭示集中间隙耗尽，故本候选保留AVL precedence，不加新重标记政策。历史V3尾成本说明需看全负载；该原型不排除未来有界标签维护。

参考：[Dietz–Sleator报告](https://www.cs.cmu.edu/~sleator/papers/maintaining-order.html)、[Bender等ESA2002](https://doi.org/10.1007/3-540-45749-6_17)、[经典适配器归档](https://github.com/RXY712200/LayerKeySort/tree/f41b7be182c2ef766ee0dec1211500f7dd762f54/benchmarks/research/full-ri-20261124/archive/tooling)、[V3尾成本历史](https://github.com/RXY712200/LayerKeySort/blob/ce812dda83bdc7f9581cbb2761921aa3c7ba75cd/docs/TAIL_COST_RESEARCH.md)。
