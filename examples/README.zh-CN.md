# V4 公共 API 示例

[English](README.md)

这些公共 API 示例适用于当前已发布的 [Full v4.1.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。私有内存布局优化保留了公共函数和使用语义。

所有示例只包含 layerkeysort.h，检查错误，并保持业务对象由调用者拥有。

| 源文件 / 目标 | 用途 |
| --- | --- |
| basic.c / layerkeysort_example | 最小创建、插入、查询、删除及销毁流程 |
| ordered_tree.c / layerkeysort_ordered_example | 托管比较排序；文件名沿用历史命名 |
| layer_list.c / layerkeysort_layer_list_example | 可相对编辑的图层 |
| live_order.c / layerkeysort_order_example | Handle 和遍历 |
| managed_order.c / layerkeysort_managed_order_example | 托管顺序中的等值与删除 |
| immutable_group.c / layerkeysort_group_snapshot_example | 平面 Group/Batch 的快照 |
| snapshot.c / layerkeysort_snapshot_example | LS1、线格式、恢复和迁移 |

CMake 构建完成后运行对应程序，MSVC Debug 构建的程序位于 Debug 目录。ZIP 内含 basic.c，文件名为 example.c。demo/main.c 是私有历史回归入口。参见[冻结契约](../docs/V4_FREEZE.md)。
