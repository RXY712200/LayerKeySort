# 从 Stable V2.0.0 迁移到 V3 3.x

历史英文原文：[docs/V3_MIGRATION.md](https://github.com/RXY712200/LayerKeySort/blob/fd7e3e7d15b5b8fc35ff0bfb0ffbafe5c94b5111/docs/V3_MIGRATION.md)。以下适用于当时的 V3 迁移。

本指南描述从历史 Stable `v2.0.0` 迁移到已发布 `v3.0.0` 引入的 API。3.1.0 源码仍有相同的 59 个公开函数。原文链接 `COMPATIBILITY.md` 描述当时的兼容入口；这个路径在当前树上用于 V4。

V2 的 `LksTree` 同时接受调用者选择的 Path 及每次操作传入的比较器。显式插入或改键可能破坏下一次比较器插入所假定的顺序。V3 将两种模型分开：

| V2 操作 | V3 手动坐标 Tree | V3 比较器托管 Tree |
| --- | --- | --- |
| `lks_tree_create()` / destroy / size | 不变 | `lks_ordered_tree_create(&comparator)` / destroy / size |
| `lks_tree_insert(path, item)` | 不变 | 无任意 Path 插入 |
| `lks_tree_insert_item(item, &comparator)` | 移除 | `lks_ordered_tree_insert(item)` |
| `lks_tree_locate_item(item, &comparator)` | 移除 | `lks_ordered_tree_locate(item)` |
| `lks_tree_find_path(path)` | 不变 | `lks_ordered_tree_find_path(path)` |
| `lks_tree_remove_path(path)` | 不变 | `lks_ordered_tree_remove_path(path)` |
| `lks_tree_rekey(old, new)` | 不变 | 无任意改键 |
| 节点 Path、对象、物理子节点与父节点访问 | 不变，使用 `LksTreeNode` | 相同的借用 `LksTreeNode` 访问器 |
| 虚拟根导航 | `lks_tree_root_child_*()` | `lks_ordered_tree_root_child_*()` |

托管容器在创建时复制 `LksComparator` 描述符，随后描述符对象可以失效。回调代码、上下文指针及居民对象参与比较的字段仍由调用者拥有；须在容器销毁前存活并保持一致的排序语义。就地修改参与比较的字段可能破坏搜索及插入。保存确切 Path，先删除，再修改并重新插入。

两个容器均拥有 AVL 结构节点及 Path 并借用对象，按 `lks_path_compare()` 比较坐标。成功的修改使先前借用的节点、Path 与导航观察失效。手动改为同等 Path 是成功的无操作，它们仍有效。托管容器不提供任意移动，以保持绑定顺序。

Path、显示文本、LK1 v1、Group、GroupBatch 及 `lks_sort()` 语义不变。自动生成的确切 Path 和 AVL 物理形态不属于兼容性承诺。没有自动的整树或对象序列化。物理导航只提供瞬时索引视图，逻辑 Path 层次另有语义。
