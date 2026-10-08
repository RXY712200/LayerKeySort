# LayerKeySort

[English](README.md) | 简体中文

## 产品线选择

本仓库包含独立产品线：

- **Full**：[v4.0.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0)，完整顺序管理、历史快照及进阶工作流。
- **[Mini](mini/README.zh-CN.md)**：[v1.0.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0)，独立的轻量 C17 顺序维护库。

本仓库采用**多产品独立版本管理**。Full 保留历史 `v*` 标签；Mini 采用 `mini-v*` 标签，首个 Stable 为 `mini-v1.0.0`。GitHub 仓库级 `Latest` 不等于两个产品各自的最新版本。详见[产品线与发布管理规范](docs/PRODUCT_LINES.zh-CN.md)（[English](docs/PRODUCT_LINES.md)）。

本页其余内容介绍 Full。

LayerKeySort 是一个 **C17 动态顺序管理库**。它解决的不是“把一组元素一次排好”，
而是“一个集合长期存在，并且持续发生插入、删除、移动、遍历、保存历史顺序和恢复”。

**实时顺序使用稳定的容器内句柄；历史/持久化顺序使用不可变快照和 LS1 键。**

当前推荐 Stable 为 **v4.0.0**。V4 的 Preview.1–5 → RC.1 → Stable 开发周期已经完成。
V3.1.0 仍作为历史 Stable 保留给依赖 Path/Tree 契约的应用；V4 是源码级大版本变化，
不是可以直接替换头文件的兼容升级。

当前 post-v4 roadmap 与技术债统一记录在
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。
历史 Issue #2 已关闭但完整保留。

## 它适合什么？

LayerKeySort 最适合长期维护复杂本地 mutable collection 的软件，例如：

- 图层编辑器；
- 视频 / 音频时间线；
- CAD、游戏编辑器；
- 节点 / 工作流编辑器；
- 播放列表编辑器；
- 本地文档编辑系统；
- 大型 UI / 树结构中的某一层扁平顺序。

典型操作不是“排序一次”，而是：

```text
把 A 插到 B 前面
把 C 移到 D 后面
删除 E
加入 F
读取第一个 / 下一个
捕获当前历史顺序
保存后恢复并继续编辑
```

如果你只需要一次性排序、简单 SQL `position` 字段、CRDT/分布式协同顺序，
或者跨进程长期业务 ID，LayerKeySort 通常不是最合适的工具。

## 四个主要入口

- `LksOrder`：显式相对顺序，支持前/后插入、移动、遍历和稳定 resident handle。
- `LksManagedOrder`：由比较器维护顺序，保证相等元素的稳定插入语义。
- `LksGroup` / `LksGroupBatch`：不可变平面序列、稳定构建和合并。
- `LksSnapshot`：历史快照、LS1、LKS4SNP1、序列化和恢复。

业务对象与业务 ID 始终由应用拥有。

**Handle 不是持久化 ID。LS1 也不是 live position。**

## 最小示例

```c
#include <layerkeysort.h>

int main(void) {
    int item = 42;
    const LksOrderHandle *handle = NULL;
    LksOrder *order = lks_order_create();
    if (!order) return 1;

    if (lks_order_insert_back(order, &item, &handle) != LKS_STATUS_OK) {
        lks_order_destroy(order);
        return 2;
    }

    lks_order_destroy(order); /* item 仍由应用拥有 */
    return 0;
}
```

完整公开示例见 [examples/README.md](examples/README.md)。

## 集成

支持 CMake `add_subdirectory`、离线 FetchContent、本地安装 +
`find_package(LayerKeySort CONFIG REQUIRED)`，也提供确定性的两文件 amalgamation。

见：

- [中文集成指南](docs/INTEGRATION.zh-CN.md)
- [英文完整集成指南](docs/INTEGRATION.md)
- [API 参考](docs/API.md)
- [使用指南](docs/USAGE.zh-CN.md)
- [V3 → V4 迁移](docs/MIGRATION_V3_V4.md)

## 已知成本与边界

V4 不再为 live resident 维护 Path/LK1，因此避免了 V3 的全范围 live-coordinate relabel
和 live Path 增长；但它仍然有真实成本：

- 每 resident allocation；
- block slack 和局部指针移动；
- comparator / callback / allocator / OS 成本；
- 完整 snapshot/export 是显式 `O(N+A)`；
- 保留大量历史 snapshot 会占用相应历史内存。

Mutable source 仍由调用方负责串行化；不承诺所有操作的统一硬延迟上界，
也不承诺 universal binary ABI、CRDT、持久化 live handle 或数据库职责。

主要性能收敛证据见
[Preview.4 evidence](benchmarks/results/v4-preview4/README.md)；
Stable 结论见 [V4_STABLE](docs/V4_STABLE.md)。

## 长期方向

LayerKeySort 的长期定位是 **dynamic ordering library**，而不是传统 sorting library。
核心边界保持在：

1. Mutable Order Engine；
2. Immutable Historical Representation；
3. Immutable Batch Utilities。

短期内不应为了版本号立刻开始 V5。下一阶段最重要的是进入真实编辑器类应用，
收集 implementation-independent 的真实 workload traces。

Mini 已在独立的 `mini/` 目录发布 v1.0.0 Stable，但不属于 Full V4 的 API 契约；Embedded 等其他产品线仍只是未来设想，不代表已经实现。
详见 [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)。

## 文档

- [架构](docs/ARCHITECTURE.md)
- [API](docs/API.md)
- [兼容性](docs/COMPATIBILITY.md)
- [验证](docs/VALIDATION.md)
- [开发维护](docs/DEVELOPMENT.md)
- [V4 Stable 记录](docs/V4_STABLE.md)
- [Changelog](CHANGELOG.md)

License: [MIT](LICENSE)。
