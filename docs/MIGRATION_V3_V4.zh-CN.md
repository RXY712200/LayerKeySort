# V3 到 V4 的迁移

适用于 Full v4.2.0；V3 至 V4 的源码变化与严格旧 LK1 导入语义不变。见[当前集成说明](INTEGRATION.zh-CN.md)。
[English](MIGRATION_V3_V4.zh-CN.md)

本指南描述 v4.0.0 Stable 引入且Full v4.2.0 延续的冻结迁移契约。V4 主版本打破源码兼容。需要分层 Path/Tree 契约时可继续使用 v3.1.0。迁移需针对 V4 最终公开头重建，库没有别名层。应用改用取决于当前容器的实时 Handle，继续拥有业务身份，并通过严格导入保留已发布 V3 LK1 数据。

| 原流程 | V4 对应流程 |
| --- | --- |
| 以手动 Path 插入 Tree | 使用 LksOrder 相对位置或端点插入，获得驻留句柄 |
| 物理节点导航 | 逻辑遍历及同来源 lks_order_compare |
| OrderedTree | 使用 LksManagedOrder，并保持对象参与比较的数据稳定 |
| 基于 Path 的 Group/Batch | 平面的 LksGroup/LksGroupBatch，无 group_path_at |
| 持久化实时 Path/LK1 | 捕获 Snapshot，持久化 LS1 与 LKS4SNP1 |
| 加载已发布 V3 LK1 | 严格 lks_order_import_v3_lk1，业务对象仍由调用者拥有 |

旧调用：`lks_tree_insert(tree, path, item, &node)`。V4 调用：`lks_order_insert_before(order, anchor, item, &handle)`。无关修改和移动保留 Handle，删除其所属实例或销毁来源时失效。业务身份独立保存；指针数值不表示顺序。托管顺序不提供任意移动。修改键前须处理全部相关容器中的受影响实例，并重建排序前提失效的 Group/Batch；重插可能失败，恢复由应用负责。见[完整契约](IMPLEMENTATION_API_REFERENCE.zh-CN.md)。

Preview.4 的 LksImmutableGroup/Batch 改为 LksGroup/Batch。14 个 lks_immutable_group_* 名称改为 lks_group_* 或 lks_group_batch_*。V3 中的同名 Group 语义不同，应迁移调用方式。指针数组输入会被复制，有效载荷所有权保持不变。

严格导入按已发布的 LK1 语义排序，拒绝畸形、非规范、溢出或重复的坐标，创建新的实时 Handle。超出本机 SIZE_MAX 的层级被拒绝。键不会被保留；Path 不公开。

快照捕获复制应用提供的命名空间和身份关联。加载的快照没有来源当前性。Resolver 将关联映射到调用者拥有的对象；恢复借用这些对象并创建新 Handle。历史快照在来源及业务对象销毁后仍可用。LS1 是命名空间内的序号；实时位置有独立机制。命名空间唯一性由应用负责。Full 捕获与导出成本为 O(N+A)。参见[冻结契约](V4_FREEZE.zh-CN.md)、[API](API.zh-CN.md)及[公开快照示例](../examples/snapshot.c)。Full v4.2.0 保留此迁移行为。V3.1.0 仍作为历史 Stable 提供。

## 历史 Preview 专用改名表

| Preview.4 函数 | Preview.5 函数 | 原因 |
| --- | --- | --- |
| `lks_immutable_group_batch_build` | `lks_group_batch_build` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_destroy` | `lks_group_batch_destroy` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_group_at` | `lks_group_batch_group_at` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_group_count` | `lks_group_batch_group_count` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_group_size` | `lks_group_batch_group_size` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_merge_all` | `lks_group_batch_merge_all` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_batch_size` | `lks_group_batch_size` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_build` | `lks_group_build` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_destroy` | `lks_group_destroy` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_item_at` | `lks_group_item_at` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_merge` | `lks_group_merge` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_size` | `lks_group_size` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_snapshot_capture` | `lks_group_snapshot_capture` | 统一为平面 V4 Group 模型 |
| `lks_immutable_group_snapshot_is_current` | `lks_group_snapshot_is_current` | 统一为平面 V4 Group 模型 |

## 已移除的旧名称

V4 主公开头不含下列 V3 名称。

- `lks_group_batch_total_size`
- `lks_group_path_at`
- `lks_ordered_tree_create`
- `lks_ordered_tree_destroy`
- `lks_ordered_tree_find_path`
- `lks_ordered_tree_insert`
- `lks_ordered_tree_locate`
- `lks_ordered_tree_remove_path`
- `lks_ordered_tree_root_child_at`
- `lks_ordered_tree_root_child_count`
- `lks_ordered_tree_size`
- `lks_path_after`
- `lks_path_append`
- `lks_path_append_at_level`
- `lks_path_before`
- `lks_path_between`
- `lks_path_clone`
- `lks_path_compare`
- `lks_path_create`
- `lks_path_create_at_level`
- `lks_path_create_zero`
- `lks_path_depth`
- `lks_path_destroy`
- `lks_path_direction`
- `lks_path_format`
- `lks_path_get_level`
- `lks_path_get_slot`
- `lks_path_order_key_format`
- `lks_path_order_key_length`
- `lks_path_order_key_parse`
- `lks_path_parse`
- `lks_path_text_length`
- `lks_tree_create`
- `lks_tree_destroy`
- `lks_tree_find_path`
- `lks_tree_insert`
- `lks_tree_node_child_at`
- `lks_tree_node_child_count`
- `lks_tree_node_item`
- `lks_tree_node_parent`
- `lks_tree_node_path`
- `lks_tree_rekey`
- `lks_tree_remove_path`
- `lks_tree_root_child_at`
- `lks_tree_root_child_count`
- `lks_tree_size`

V3 中被复用的旧 `lks_group_*` 名称现在使用平面 V4 语义。57 个 V3 专用契约均被移除或替换，保留的是通用排序及状态接口。

Full v4.2.0 保留 Preview.5 冻结的声明及格式。参见 [4.2.0 源码记录](V4_2_0.zh-CN.md)和[历史 v4.0.0 Stable 记录](V4_STABLE.zh-CN.md)。保留旧 Path/Tree 的应用可继续使用 V3.1.0。
