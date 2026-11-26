# Full Preview.3 综合工程审计（历史译本）

原文：[V4_2_0_PREVIEW3_ARCHITECTURE.md](V4_2_0_PREVIEW3_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md)。原件保留；以下为该里程碑状态。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

该工程里程碑保留Preview.2生产 `fe00536e7bd7f4e5128432ea4a29e54158074903` 及开发版本。诊断基线 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`，加快照独立测试 `92fb7fad977f792a7d9e3e0e430a5d5f32e8914a`。源码绑定文档为Preview.2 `01bc01e2d9ddbda448d9ef0807150f419203b580`；Stable源码/已接受指南仍为 `70d7f0ef5857b54381b863d292a213909d87459f` / `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`。

| 检查的源码及私有头文件 | 不变量或契约 | 证据与限制 |
| --- | --- | --- |
| `order.c`、`order_bulk.c`、`lks_order_internal.h` | B64 占用下限、驻留记录独立分配且地址稳定、实际槽位和所属块、AVL 高度／计数／父链接、标签间距与赋值或回退边界 | 独立的顺序与句柄模型、内部不变量检查、回退及集中插入探针。零标签使用 AVL 比较，恢复没有统一期限。 |
| `managed_order.c`、`lks_order_internal.h` | 比较器等值项的稳定插入、定位边界、外部同步、回调期间的忙标志、失败时保留修订号 | 既有托管修改、回调重入及外部游标测试。调用方须维持比较器与当前顺序的一致性。 |
| `immutable_group.c`、`lks_immutable_group_internal.h`、`sort.c`、`lks_sort_internal.h` | 稳定顺序与等值处理、独立捕获历史关联数据、发布前分配及清理 | Group、Batch 与排序的冒烟测试及分配检查。旧版可变 Group 和 Tree 源码单独用于回归测试。 |
| `lks_alloc.c`、`lks_alloc_internal.h`、`lks_base.c`、公开头文件 | 状态及哨兵处理、故障注入与生产分配请求的区别、大小检查、稳定的不透明接口 | 冻结接口清单、实际触达的故障点测试、MSVC Win32 及 Sanitizer CI。传入悬空句柄超出契约。 |
| `path.c`、`path_text.c`、`path_order_key.c`、`slot_codec.c`、`path_compare.c`、相应私有头文件及 `lks_legacy_internal.h` | LK1 解析及顺序、长度检查、稳定的旧格式导入及清理 | 冻结的 LK1 用例和大规模迁移用例。生产构建保留兼容助手；`gap.c`、`tree.c`、`group.c`、`bulk.c` 及相关旧版头文件用于明确划分的诊断与回归源码集。 |
| `snapshot.c`、`snapshot_key.c`、`lks_snapshot_internal.h` | 即时复制、独立拥有命名空间及关联数据、累计结束偏移量、引用计数管理的来源标记、不可变视图、LS1 域规则 | 同修订号下的载荷修改、保留及销毁、来源标记及键的用例。是否仍为当前状态取决于来源和修订号，判断时不比较应用数据字节。 |
| `snapshot_wire.c`、`snapshot_restore.c`、`order_bulk.c` | 定宽逐字节移位、分配前完整校验、按序号解析后才批建、恢复后使用新句柄、失败时无部分输出 | 既有黄金字节及旧格式用例，以及截断、长度、实际触达的内存分配失败测试。本扩展使用偏移一字节的缓冲区及字面黄金字节分别检查读写。 |
| `cmake/ProductionSources.cmake`、CMake 安装及 `tools/amalgamate.py` | 共用真实源码清单、旧版诊断源码分离、C17/C++ 链接、确定性分发 | API 清单、示例、外部源码／安装／合并版消费者及本地分发产物验证。 |

## 结构与失败结论

跨块双方均有非零标签时直接比较标签，任一为零时由 AVL 路径确定顺序。诊断检查整个非零标签子序列严格递增。块接入仅在有界窗口内探测和写入；大量集中编辑仍可能耗尽局部间隔。批量重建创建新标签及句柄，标签间隔处于无符号数值范围内。历史快照不存储实时块拓扑。

本次审计确认仍保留这些边界：实时修改先准备备用块，解码分配前校验完整线格式，捕获时立即复制回调借出的字节，恢复先解析全部行再批建。这样可以避免失败时发布部分状态，也避免交错分配失败改变应用可观察到的解析回调前缀。分配请求的内存与进程内存分别统计；结构工作量有界不能直接推出时延上限。

本扩展只改测试、研究和文档。原测试缺少未对齐缓冲区覆盖，逐字节实现已经支持该输入。本次有界审计未证实生产语义缺陷。B32、B128、边界转移、中点耗尽、元数据消融及保留成本研究均可在各自的历史提交中查阅。

[模块清单及A–F谱系](../../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.zh-CN.md) · [阶段证据与剩余检查](../V4_2_0_PREVIEW3.zh-CN.md)
