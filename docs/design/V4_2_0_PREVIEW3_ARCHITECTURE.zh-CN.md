# Full Preview.3 综合工程审计（历史译本）

原文：[V4_2_0_PREVIEW3_ARCHITECTURE.md](V4_2_0_PREVIEW3_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md)。原件保留；以下为该里程碑状态。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

该工程里程碑保留Preview.2生产 `fe00536e7bd7f4e5128432ea4a29e54158074903` 及开发版本。诊断基线 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`，加快照独立测试 `92fb7fad977f792a7d9e3e0e430a5d5f32e8914a`。源码绑定文档为Preview.2 `01bc01e2d9ddbda448d9ef0807150f419203b580`；Stable源码/已接受指南仍为 `70d7f0ef5857b54381b863d292a213909d87459f` / `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`。

| 检查源码及私有头 | 不变量/契约 | 证据与限制 |
| --- | --- | --- |
| order.c、order_bulk.c、lks_order_internal.h | B64占用、稳定独立resident、物理slot/owner、AVL高/count/parent、标签间隔与赋值/回退边界 | 独立顺序/Handle模型、内部checker、回退及聚集probe；零tag用AVL，无普遍恢复期限 |
| managed_order.c、lks_order_internal.h | 稳定等值插入、locate边界、外部同步、回调忙标志、失败revision保留 | 既有managed修改、callback/reentry、foreign-cursor；调用方比较语义一致 |
| immutable_group.c、lks_immutable_group_internal.h、sort.c、lks_sort_internal.h | 稳定顺序/等值、独立历史关联、发布前分配/清理 | Group/Batch/sort冒烟及分配检查；旧可变group/tree为单独回归对象 |
| lks_alloc.c、lks_alloc_internal.h、lks_base.c、公开头 | 状态/哨兵、故障注入与生产请求区别、受检大小、不透明接口稳定 | 冻结manifest、到达故障测试、MSVC Win32及sanitizer CI；悬空Handle不在契约 |
| path.c、path_text.c、path_order_key.c、slot_codec.c、path_compare.c、相关私有头及lks_legacy_internal.h | LK1解析/顺序、长度、稳定旧导入/清理 | 冻结LK1/大迁移fixture；兼容helper生产保留，gap/tree/group/bulk及相关旧头供诊断/回归源码集 |
| snapshot.c、snapshot_key.c、lks_snapshot_internal.h | 即时复制、独立namespace/关联所有权、累计end、引用来源/不可变视图、LS1域 | 同revision修改、保留/销毁、marker/key向量；currentness不比较应用字节 |
| snapshot_wire.c、snapshot_restore.c、order_bulk.c | 固定字节移位、分配前完整验证、先ordinal解析再bulk、新Handle/无部分结果 | 既有golden/legacy、截断/长度/到达OOM；扩展以offset-one及literal golden测reader/writer |
| cmake/ProductionSources.cmake、CMake安装及tools/amalgamate.py | 共享实际清单、旧诊断分离、C17/C++链接、确定分发 | API清单、示例、外部源码/安装/合并及本地资产检查 |

## 结构与失败结论

跨块双方非零直接比较tag，任一为零使用权威AVL。诊断检查整个非零子序列严格单调。Attachment仅探测/写入有界区域，大量聚集编辑仍可能耗尽局部空隙。Bulk重建产生新tag/Handle，间隔在无符号域内。历史不含live拓扑。

保留实时修改先spare、wire先完整验证、回调立即复制、先resolve后建造。这些边界避免失败发布部分状态，或交错分配失败改变resolver前缀。请求内存与进程内存分开，有界结构工作不等于延迟保证。

本扩展只改测试、研究、文档。未对齐缓冲是覆盖不足，字节实现已支持；有界审计未证实生产语义缺陷。B32/B128/transfer、中点耗尽、元数据消融、保留研究均在永久版本保留。

[模块清单及A–F谱系](../../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.zh-CN.md) · [阶段证据与剩余检查](../V4_2_0_PREVIEW3.md)
