# V4 API 参考

[English](API.md)

Full v4.2.0 保留冻结的 V4 源码 API（64 个函数、14 个公开类型）、LS1 和 LKS4SNP1。只需包含 `<layerkeysort.h>`。[冻结契约](V4_FREEZE.zh-CN.md)定义兼容性；[完整 API 指南](IMPLEMENTATION_API_REFERENCE.zh-CN.md)逐函数映射源码与详细契约。[V3 迁移](MIGRATION_V3_V4.zh-CN.md)说明主版本源码变化。

## 状态、比较器与排序

LksStatus 使用明确的数值；未知整数返回 "Unknown status"。比较器包含 compare 和 context。ManagedOrder 保留描述符，借用上下文直到销毁。Group 和 sort 只在操作期间借用。比较语义必须保持一致。lks_sort 就地稳定排序指针，内存不足时输入保持不变，少于两个元素时不分配。

## 显式顺序、Handle 与游标

Order 拥有结构并借用非空业务对象。相对位置及端点插入创建新实例。删除仅释放结构；实例被删除或来源销毁时，其 Handle 失效。移动保留 Handle，来自其他来源的有效 Handle 会被拒绝。无操作保留游标和快照当前性。遍历使用逻辑顺序，没有秩或物理拓扑 API。游标借用来源，方向为 0 或 1；实际修改会使游标在访问缓存 Handle 之前失效。先销毁游标，再销毁来源。完整错误规则见公开头文件。

## 托管顺序

比较器驱动共享的上下文顺序核心，按稳定上界插入。定位返回前驱、第一个等值实例及后继。删除使用 Handle；不能任意移动或绕过托管核心。修改比较器可见数据前，必须先移除或妥善处理每个相关 ManagedOrder 中的所有受影响实例，并考虑借用同一对象的其他有序结构。若排序前提将失效，不可变 Group 须由应用重建或替换。删除立即使该实例的 Handle 失效；重新插入可能失败。删除／修改／重插不是库提供的原子事务，应用恢复由调用者负责。详见[完整修改契约](IMPLEMENTATION_API_REFERENCE.zh-CN.md)。

## 不可变 Group 和 Batch

Group 拥有平面数组副本并借用对象，不含索引或 Path。稳定构建和独立合并都保留等值顺序，合并时 Base 先于 Incoming。Batch 拥有已排序的分块；借用视图随 Batch 失效。合并所有分块时，较早分块优先，返回独立拥有的 Group。空构建也需要比较器，分块大小必须非零。

GroupBatch 的有序分片若因共享载荷修改而失效，应用必须重建或替换 Batch；借用的 Batch 所有 Group 视图不能单独替换或销毁，且仅在 Batch 仍存活时有效。

## 快照、LS1、线格式与恢复

捕获拥有命名空间和关联数据的副本。回调返回的字节区间必须在紧接其后的复制期间保持有效。历史行可在来源和业务对象销毁后存活。当前性要求来源标记和修改代数匹配；加载的快照没有当前性。LS1 精确的小写十六进制命名空间及序号语法，以及 LKS4SNP1 的显式大端格式，均已冻结。格式化和序列化无分配。文本长度不含 NUL，二进制长度为确切字节数，目标空间不足时保持目标不变。反序列化先完整验证借用的输入，再分配内存。恢复非空快照时必须提供解析回调，库按行序号调用它。回调成功时必须返回非空的借用对象指针，允许多行返回同一指针。解析完所有行后，库才构建使用新句柄的新顺序。失败时输出保持 NULL，临时存储随即释放。调用者继续拥有已解析对象，并负责回调的副作用。严格 LK1 导入保留旧坐标含义，旧实时 API 仍不公开。

## 完整冻结函数清单

确切签名及逐函数要求见[公开头文件](../include/layerkeysort.h)。类型化编译及链接引用覆盖所有声明，C/C++ 消费者覆盖各接口族。tools/verify_v4_api.py 拒绝未经审查的签名、布局及名称变化。

### Group / Batch

- `lks_group_batch_build`
- `lks_group_batch_destroy`
- `lks_group_batch_group_at`
- `lks_group_batch_group_count`
- `lks_group_batch_group_size`
- `lks_group_batch_merge_all`
- `lks_group_batch_size`
- `lks_group_build`
- `lks_group_destroy`
- `lks_group_item_at`
- `lks_group_merge`
- `lks_group_size`
- `lks_group_snapshot_capture`
- `lks_group_snapshot_is_current`

### 托管顺序

- `lks_managed_order_compare`
- `lks_managed_order_create`
- `lks_managed_order_cursor_create`
- `lks_managed_order_destroy`
- `lks_managed_order_first`
- `lks_managed_order_insert`
- `lks_managed_order_last`
- `lks_managed_order_locate`
- `lks_managed_order_remove`
- `lks_managed_order_size`
- `lks_managed_order_snapshot_capture`
- `lks_managed_order_snapshot_is_current`

### 显式顺序、游标与导入

- `lks_order_compare`
- `lks_order_create`
- `lks_order_cursor_create`
- `lks_order_cursor_destroy`
- `lks_order_cursor_next`
- `lks_order_destroy`
- `lks_order_first`
- `lks_order_import_v3_lk1`
- `lks_order_insert_after`
- `lks_order_insert_back`
- `lks_order_insert_before`
- `lks_order_insert_front`
- `lks_order_item`
- `lks_order_last`
- `lks_order_move_after`
- `lks_order_move_back`
- `lks_order_move_before`
- `lks_order_move_front`
- `lks_order_next`
- `lks_order_previous`
- `lks_order_remove`
- `lks_order_size`
- `lks_order_snapshot_capture`
- `lks_order_snapshot_is_current`

### 快照、LS1 与线格式

- `lks_snapshot_association`
- `lks_snapshot_count`
- `lks_snapshot_deserialize`
- `lks_snapshot_destroy`
- `lks_snapshot_key_compare`
- `lks_snapshot_key_format`
- `lks_snapshot_key_length`
- `lks_snapshot_key_validate`
- `lks_snapshot_namespace`
- `lks_snapshot_restore_order`
- `lks_snapshot_serialize`
- `lks_snapshot_serialized_size`

### 通用接口

- `lks_sort`
- `lks_status_string`
