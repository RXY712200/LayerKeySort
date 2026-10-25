# LayerKeySort V4 使用指南

**版本状态：** `main` 当前 Full 源码为 **4.1.0**（[合并提交](https://github.com/RXY712200/LayerKeySort/commit/7fef6396cba4f5f8d19516b9ae2af132ca3cd529)）；最新**已公开发布**的 Stable 仍为 [v4.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0)，4.1.0 的新 Tag 和 Release 尚未发布。公开 API 和持久化格式仍兼容，详见 [4.1.0 说明](V4_1_0.md)。

只包含 `<layerkeysort.h>` 即可。

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
remove → 修改 → reinsert。

### `LksGroup` / `LksGroupBatch`

用于不可变平面序列、稳定构建、merge 和 batch ordering。

Group 复制指针数组，但不拥有 item payload。

### `LksSnapshot`

需要历史/持久化顺序时显式 capture。

Snapshot 复制 namespace 和 association bytes；可以生成 LS1、序列化成
LKS4SNP1，并通过 resolver 恢复成新的 live order 和新的 handle。

## 身份与持久化

业务 ID 由应用维护。

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
