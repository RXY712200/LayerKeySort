# LayerKeySort V4 使用指南

[English](USAGE.md)

4.2 候选版沿用已发布 4.1.0 的调用与所有权契约。先查[选型指南](FULL_SELECTION_GUIDE.zh-CN.md)，据此选择显式顺序、比较器托管顺序或更简单的组合。

**版本状态：** [LayerKeySort Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) 是当前已发布的 Stable 版本。公开 API 和持久化格式保持兼容，详见 [4.1.0 说明](V4_1_0.md)。

只包含 `<layerkeysort.h>` 即可。先运行 [basic.c](../examples/basic.c)，再参考 [managed_order.c](../examples/managed_order.c)、[immutable_group.c](../examples/immutable_group.c) 和 [snapshot.c](../examples/snapshot.c)。

LayerKeySort 面向的是**长期存在并持续变化顺序的集合**，不是一次性排序。

## 适合的场景

典型场景包括：

- 图层列表；
- 视频 / 音频时间线；
- CAD、游戏编辑器对象顺序；
- 节点 / 工作流编辑器；
- 播放列表；
- 本地文档编辑系统。

如果你的主要操作长期反复出现 insert / remove / move，那么它比“每次重新排序”
更接近 LayerKeySort 的目标问题。

## 不适合的场景

如果你只需要：

- 一次数组排序；
- 一个简单 SQL `position` 字段；
- CRDT / 分布式冲突合并；
- 跨进程持久化业务身份；
- 数据库、WAL、事务系统；

通常应该使用更简单或更专门的工具。

## 四组主要 API

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

4.1.0 已完成包含 10 万至千万级负载的内部研究，既记录了内存收益，
也公开记录特定负载下的耗时回退。未来以明确的正确性、性能、内存和兼容性
问题驱动研究；真实编辑器数据有价值，但不是继续研发的前提。
也不为了版本号仓促启动 V5。

当前 roadmap：
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。

详见 [API](API.md)、[兼容性](COMPATIBILITY.md)、[冻结契约](V4_FREEZE.md)、
[迁移指南](MIGRATION_V3_V4.md)。

GroupBatch 的有序分片若因共享载荷修改而失效，应用必须重建或替换 Batch；借用的 Batch 所有 Group 视图不能单独替换或销毁，且仅在 Batch 仍存活时有效。
