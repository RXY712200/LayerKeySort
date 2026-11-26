# LayerKeySort V4 使用指南

[English](USAGE.zh-CN.md)

Full v4.2.0 提供五类可独立使用或组合使用的工作流；对象和业务身份始终由应用拥有。

| 工作 | API | 示例 |
| --- | --- | --- |
| 显式相对编辑与先后查询 | `LksOrder` | [basic.c](../examples/basic.c) |
| 比较器维护的稳定顺序与查询 | `LksManagedOrder` | [managed_order.c](../examples/managed_order.c) |
| 一次性稳定指针数组排序 | `lks_sort` | 下方最小示例 |
| 不可变排序分片与稳定归并 | `LksGroup`、`LksGroupBatch` | [immutable_group.c](../examples/immutable_group.c) |
| 历史关联、LS1、线格式及恢复 | `LksSnapshot` | [snapshot.c](../examples/snapshot.c) |

显式顺序适用于编辑器、时间线和播放列表；托管顺序适用于持续维护的优先级或比较器集合；Group 适用于排序分片与批量聚合；一次性排序无需实时容器。数据库可采用这些顺序组件，同时自行管理事务、身份和存储。分布式冲突解决需要独立机制。

## 一次性稳定排序

```c
#include <layerkeysort.h>
static int compare_int(const void *a, const void *b, void *context) {
    const int x = *(const int *)a, y = *(const int *)b;
    (void)context;
    return (x > y) - (x < y);
}
int main(void) {
    int values[] = {3, 1, 1};
    void *items[] = {&values[0], &values[1], &values[2]};
    return lks_sort(items, 3, compare_int, NULL) == LKS_STATUS_OK ? 0 : 1;
}
```

比较等值的对象保持输入先后。`lks_sort` 在提交调用者数组前，使用指针副本和同样大小的临时数组。内存不足时输入不变；少于两项不分配，但仍须提供比较器。该稳定排序接口有复制和临时存储成本；实时顺序测量未与 `qsort` 或 `std::sort` 比较。

## 选择 API 家族

### `LksOrder`

应用明确决定相对位置时使用。

支持前后插入、相对插入、remove、move、遍历和同源顺序比较。

Handle 表示一个 live occurrence。它可以跨无关 mutation 和 move 保持稳定，
但自己的 removal 或 source destruction 会使它失效。

### `LksManagedOrder`

顺序由 comparator 决定时使用。

resident 期间 comparator 可见的数据必须保持一致。要修改排序 key：
修改比较器可见数据前，必须先移除或妥善处理每个相关 ManagedOrder 中的所有受影响实例，并考虑借用同一对象的其他有序结构。若排序前提将失效，不可变 Group 须由应用重建或替换。删除立即使该实例的 Handle 失效；重新插入可能失败。删除／修改／重插不是库提供的原子事务，应用恢复由调用者负责。详见[完整修改契约](IMPLEMENTATION_API_REFERENCE.zh-CN.md)。

### `LksGroup` / `LksGroupBatch`

用于不可变平面序列、稳定构建、merge 和 batch ordering。

Group 复制指针数组，但借用应用对象载荷。Batch 返回的借用 Group 视图随 Batch 销毁而失效；合并结果则独立拥有。

GroupBatch 的有序分片若因共享载荷修改而失效，应用必须重建或替换 Batch；借用的 Batch 所有 Group 视图不能单独替换或销毁，且仅在 Batch 仍存活时有效。

### `LksSnapshot`

需要历史/持久化顺序时显式 capture。

Snapshot 复制 namespace 和 association bytes；可以生成 LS1、序列化成
LKS4SNP1，严格反序列化后通过解析回调恢复成新的实时顺序和新句柄。加载的快照没有旧来源身份信息，不能判为旧来源的当前快照。

## 身份与持久化

业务 ID 和应用对象载荷由应用拥有。对象驻留期间必须保持存活；比较器上下文必须在容器生命期内存活；游标须先于来源销毁。

- Handle 不是业务 ID；
- Handle 不能跨保存/恢复继续使用；
- LS1 不是 live position；
- Snapshot key 只属于其历史 snapshot/domain。

## 成本

V4 避免 live Path 增长和全范围 live-coordinate relabel，但仍存在：

- per-resident allocation；
- block slack 和局部指针更新；
- comparator/callback 成本；
- 完整 export 的 `O(N+A)` 成本；
- retained snapshot 的历史内存。

不承诺所有操作的统一硬延迟上界。

[4.2 证据](FULL_RC1_EVIDENCE.zh-CN.md)包含 Windows/Linux 实时、生命周期和生成应用负载。使用[选型指南](FULL_SELECTION_GUIDE.zh-CN.md)了解资源取舍及反例；[4.1 记录](V4_1_0.zh-CN.md)保留独立的十万至千万级存储研究。

当前 roadmap：
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。

详见 [API](API.zh-CN.md)、[兼容性](COMPATIBILITY.zh-CN.md)、[冻结契约](V4_FREEZE.zh-CN.md)、
[迁移指南](MIGRATION_V3_V4.zh-CN.md)。
