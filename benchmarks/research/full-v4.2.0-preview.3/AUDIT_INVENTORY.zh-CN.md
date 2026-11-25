# Full 源码绑定清单（历史译本）

原文：[AUDIT_INVENTORY.md](AUDIT_INVENTORY.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md)。原件保留；[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

以下生产路径固定于[fe00536e](https://github.com/RXY712200/LayerKeySort/tree/fe00536e7bd7f4e5128432ea4a29e54158074903)。Stable对照 `70d7f0ef5857b54381b863d292a213909d87459f`、已接受指南 `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`、先前指南 `01bc01e2d9ddbda448d9ef0807150f419203b580`。Preview.3加两个测试revision，不改生产表示。

| 文件/模块 | 已查义务 | 预期与剩余边界 |
| --- | --- | --- |
| order.c、order_bulk.c、lks_order_internal.h | B64占用、独立稳定resident、owner/slot、AVL旋转/链/count、split/repair/merge、空槽；v4_tag_attach有界写/探测、非零子序列；compare任一零用AVL | v4_order、独立修改模型、聚集诊断、到达OOM；局部成功不保证任意恢复 |
| managed_order.c、lks_order_internal.h | 比较器bound、稳定等值、busy、暂存修改、revision/cursor | v4_order/v4_mutation；比较状态须保持顺序，外部同步 |
| snapshot.c、snapshot_key.c、lks_snapshot_internal.h | 即时独立复制/累计end、marker寿命/revision、LS1同域ASCII顺序 | v4_snapshot、v4_marker_threads、freeze；应用载荷身份/副作用外部 |
| snapshot_wire.c、snapshot_restore.c、order_bulk.c | 4/8字节无符号操作、完整验证后分配、先ordinal解析、bulk/回滚 | v4_snapshot、v4_wire_probe、v4_freeze、畸形及754b3088偏移回归、3ac82d26混合tag恢复；N指针scratch仍在 |
| immutable_group.c、lks_immutable_group_internal.h、sort.c、lks_sort_internal.h | 稳定重复/左优先merge、独立capture、溢出、失败不发布 | v4_immutable_group/公开消费者；比较器/items借用 |
| path.c、path_text.c、path_compare.c、path_order_key.c、slot_codec.c及path/slot/legacy头 | LK1兼容解析/顺序/受检算术 | 对抗parser、冻结向量、v4_snapshot恢复/导入；无公开V3 mutable Tree |
| gap.c、tree.c、group.c、bulk.c及tree/group/bulk/policy头 | 旧诊断对象集及历史契约 | V2/V3回归，排除生产清单 |
| lks_alloc.c、lks_alloc_internal.h、lks_base.c、include/layerkeysort.h | 固定状态、受检统计、到达注入、64函数/14类型 | API清单、Win32/sanitizer、清理计数；无悬空Handle验证承诺 |
| ProductionSources.cmake、根CMake、合并/分发工具 | 生产清单、安装/导出、单文件、C/C++链接 | 独立消费者、示例、资产检查；未产生Release |

[架构审计](../../../docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.zh-CN.md)记录准备/发布边界；[永久live-core历史](../../../docs/design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.zh-CN.md)保留中点耗尽、元数据、B32/B128、transfer否决。offset-one及恢复tag属于补测试不足；固定wire为保留优化；独立Snapshot和AVL回退为资源取舍；任意零tag恢复未被证明；Workbench尝试1–2是环境问题。整库持久仍外部。见[七ID处置](../../../docs/V4_2_0_PREVIEW3.md#technical-debt-impact)。

## 历史债谱系

| Issue #3原家族 | 处置与关联研究 |
| --- | --- |
| A. Live搜索、managed插入、广泛移动 | 部分改善；22544cf2精确位移更新、61e9290e块先后；RI-001/002/007建立live explicit比较，managed规模/cache归因仍开放 |
| B. 稳定resident分配 | 每Handle一分配保留；至1m稀疏存活为这些状态回收证据RI-005，slab/pool需等价失败/稳定/回收证据 |
| C. 占用/维护 | 保留B64；B32/B128/指针transfer未过时间/资源门槛；c977b0fd加跨零非零顺序诊断RI-001/002/006 |
| D. Snapshot/history/scratch | fe00536e改wire，92fb7fad测同revision独立；复制/保留/先resolve scratch仍在RI-003/005 |
| E. 消费者寿命/恢复 | 原生仅依赖替换回放及handoff澄清接口；应用持久/副作用不变RI-004/007 |
| F. 验证/可选集成 | 加offset-one和恢复回退；检查点/平台只覆盖有界输入，长时碎片、10m sanitizer和可选绑定不在验证RI-005/006 |

[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)保留历史家族/活跃记录，[计划#27](https://github.com/RXY712200/LayerKeySort/issues/27)定义当前编号范围。
