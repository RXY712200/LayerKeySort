# 使用 LayerKeySort

[English](USAGE.md) | 简体中文

本文介绍 LayerKeySort 3.1.0 的主要 API 用法。公共函数集合与稳定版 `v3.0.0` 兼容。获取库请看[中文集成指南](INTEGRATION.zh-CN.md)；从 V2 升级请看[英文 V3 迁移指南](V3_MIGRATION.md)。精确函数签名、错误和兼容性约定，以[公共头文件](../include/layerkeysort.h)、[英文 API 参考](API.md)及[英文兼容性约定](COMPATIBILITY.md)为准。

## 开始之前

可用两文件合并版、CMake 源码依赖、本机安装包，或直接编译模块源码接入。应用代码只需包含 `layerkeysort.h`，不应依赖 `src/` 中的私有头文件。本文从已经接入库后的使用方式开始。

## 第一步：比较器管理的 Tree

[`examples/ordered_tree.c`](../examples/ordered_tree.c) 是完整且只使用公共头文件的示例：创建绑定比较器的 Tree，插入三个调用方拥有的元素，按查询值定位其中一个，借用其 `Path` 执行删除，再查询新的相邻节点，最后销毁 Tree。删除后，它重新获取查询结果，不继续使用已失效的节点指针。

```sh
cmake -S . -B build -DLKS_BUILD_TESTS=OFF
cmake --build build --target layerkeysort_ordered_example
./build/layerkeysort_ordered_example
```

使用 Visual Studio 等多配置生成器时，Debug 构建的可执行文件位于 `build/Debug/` 下。Tree 会复制比较器描述符，但**借用**比较器上下文和已插入的元素。两者须保持有效；元素留在 Tree 内期间，影响比较结果的字段须保持不变。若在其他项目中使用，请先按[集成指南](INTEGRATION.zh-CN.md)接入。

## 排序指针数组

为指针所指的元素定义比较器，然后调用：

```c
LksStatus status = lks_sort(items, item_count, compare_items, NULL);
```

`lks_sort` 改变指针数组，不修改被指向的对象。比较结果相等的元素保持原输入顺序；分配失败时输入数组不变。[`examples/basic.c`](../examples/basic.c) 是更小的单次排序示例：

```sh
cmake -S . -B build
cmake --build build
./build/layerkeysort_example
```

Visual Studio 等多配置生成器的可执行文件路径可能不同，例如 `build/Debug/layerkeysort_example.exe`。

## 动态图层列表示例

[`examples/layer_list.c`](../examples/layer_list.c) 用具有稳定应用 ID 的调用方对象表示 Background、Player、HUD 和 Effects。它使用显式 `Path` 在 Background 与 Player 之间插入 Effects，将 HUD rekey 到 Effects 与 Player 之间，删除 Effects，再格式化 HUD 的 `LK1:` 坐标并进行往返解析。

构建目标为 `layerkeysort_layer_list_example`，正常 CMake 构建后运行。Tree 借用元素指针；应用拥有自己复制的 Path，并在销毁 Tree 后释放这些副本。示例使用 `lks_path_compare()` 对应用侧的 Path 关联排序；物理 AVL 导航不等于逻辑元素顺序。

## 比较器约定

`LksComparator` 的 `compare` 回调应遵守：

- 负数：左侧元素排在右侧之前。
- 零：按当前比较规则两者相等。
- 正数：左侧元素排在右侧之后。

`context` 可以为 `NULL`。库只把它原样传给回调，应用可用它配置比较规则。整数比较可能溢出时，不要用相减法；分别测试小于和大于。

## 构建 Group

Group 对借用的元素指针排序并分配 Path。它是进阶 API；普通指针数组排序只需 `lks_sort`。

```c
LksComparator comparator = { compare_item, NULL };
LksGroup *group = NULL;
LksStatus status;

status = lks_group_build(items, item_count, &comparator, &group);
if (status != LKS_STATUS_OK) {
    /* Handle the error. */
}

/* Read the ordered Group, then release its structure. */
lks_group_destroy(group);
```

`items` 是指向调用方对象的指针数组。比较器遵守与 `lks_sort` 相同的约定。

## 构建与合并 GroupBatch

`lks_group_batch_build` 将输入序列分成连续的块，每块至多 `group_size` 个元素，并将各块排序为 Group。它不会在构建前随机化或重排块。`lks_group_batch_merge_all` 将所有组成 Group 合并为新的独立结果 Group；Batch 在销毁前仍可读取。

传给 `lks_group_merge` 的 Group，以及 Batch 中将由 `lks_group_batch_merge_all` 合并的 Group，必须按与合并比较器及其上下文**兼容的比较语义**排序。库无法从函数指针或上下文地址推断语义等价。

```c
LksGroupBatch *batch = NULL;
LksGroup *result = NULL;

status = lks_group_batch_build(items, item_count, group_size,
    &comparator, &batch);
if (status == LKS_STATUS_OK) {
    status = lks_group_batch_merge_all(batch, &comparator, &result);
}
/* Check status, read result, then destroy result and batch. */
```

合并成功后，Batch 及其组成 Group 仍可读取。

## 读取与保存 Path 坐标

`lks_path_format()` 生成规范的可读文本，`lks_path_parse()` 解析并返回调用方拥有的 Path。逻辑排序必须使用 `lks_path_compare()`；完整展示文本**不是**字典序排序键。独立版本化的 `lks_path_order_key_*()` API 生成规范的 `LK1:` 键：同一版本内，按 ASCII 字节序比较键，与 Path 顺序一致。例如，正向、零层级、零槽位 Path 的展示文本是 `0222`，键是 `LK1:201FF0000!`。数据库必须使用保留 ASCII 字节顺序的排序规则。完整语法、非法输入及溢出规则见[英文 API 参考](API.md)。

持久化键只保存某一时刻的**排序坐标**，不保存永久元素 ID、调用方数据或整个 Tree。应用须自行关联元素身份，并处理托管修改后 Path 的变化。解析得到的 Path 用 `lks_path_destroy()` 释放。

## 读取结果

Group 访问器可按比较器顺序读取元素及其坐标：

```c
size_t count = lks_group_size(result);
size_t index;
for (index = 0; index < count; ++index) {
    void *item = lks_group_item_at(result, index);
    const LksPath *path = lks_group_path_at(result, index);
    /* Use the borrowed item and Path while result remains alive. */
}
```

`lks_group_item_at` 返回排序位置上的元素指针；`lks_group_path_at` 返回 Group 拥有的借用 Path。发布后的 Group 不可变，Path 在该 Group 销毁前保持稳定。

## 所有权与稳定性

- 元素指针由库**借用**；库不复制、也不释放调用方业务对象。
- 调用方必须在元素可能被比较或读取的整个期间保持其有效。
- 调用方拥有的 Path、Group、Tree 和 Batch 应通过各自的公共 destroy 函数释放。访问器返回的 `const` Path 或节点指针是借用视图，不应单独销毁。
- 比较结果相等的输入元素保持输入顺序。公开的两个 Group 合并时，Base Group 的相等元素排在 Incoming Group 之前，各来源内部顺序不变。GroupBatch 合并保持相等元素的来源块顺序。

## 选择 Tree 模型

应用要自行决定坐标时，使用 `LksTree`。显式插入、精确查找／删除和 rekey 接受调用方选定的 Path。解析后的 LK1 坐标可以通过显式插入恢复。`LksTree` 不检查比较器不变量，也没有基于比较器的插入或定位操作。

在线维护比较器顺序时，使用 `LksOrderedTree`。创建时提供一个比较器和调用方拥有的上下文。比较结果相等的元素按插入顺序稳定排列。插入可能重标记连续的逻辑范围，甚至改变所有已有 Path；实现见[英文架构说明](ARCHITECTURE.md)，实测成本见[英文基准报告](BENCHMARKS.md)。

库复制比较器描述符，因此创建时位于栈上的描述符之后可以失效；回调代码和调用方上下文仍被借用，须在容器生命周期内保持有效，比较语义也须稳定。每个已有元素都必须保持有效，影响比较结果的字段在其驻留期间不得改变。若要修改排序键，先克隆或以其他方式保存其精确 Path，按该 Path 移除元素，修改元素，再插入。在容器内直接修改排序键可能使查询漏掉元素或选错插入间隙。当前没有任意托管 rekey 或逐次调用替换比较器的功能。

```c
LksComparator order = { compare_items, context };
LksOrderedTree *ordered = lks_ordered_tree_create(&order);
if (ordered == NULL) { /* handle invalid comparator or OOM */ }
LksStatus status = lks_ordered_tree_insert(ordered, item, NULL);
/* Check status, use ordered, then release it. Caller still owns item. */
lks_ordered_tree_destroy(ordered);
```

两个容器均借用元素，拥有节点和 Path。**实际成功修改**后，应重新获取借用的节点、Path 和导航结果。物理导航只展示实现定义的 AVL 链接，不是 Path 层级；逻辑顺序请比较 Path。失败的操作不使已有借用视图失效。手动 Tree 对相同 Path 的 rekey 是已约定的成功空操作。

Path 是排序坐标，而非稳定的应用身份。可读展示文本和版本化 LK1 键分别用于阅读与持久化一个坐标，都不保存调用方数据或整个 Tree。生成 Path 的精确文本不是稳定身份。私有重标记阈值不属于公共兼容性承诺；见[英文 3.x 约定](COMPATIBILITY.md)。完整的托管插入可能重标记全部 `n` 个节点；不承诺最坏情况 `O(log n)` 或形式化摊还界。

## 错误处理与限制

返回 `LksStatus` 的函数会用 `LKS_STATUS_OK`、`LKS_STATUS_INVALID_ARGUMENT`、`LKS_STATUS_OUT_OF_MEMORY` 等值报告结果；`lks_status_string(status)` 提供静态英文描述。返回指针的构造函数在失败时返回 `NULL`。完整状态及函数约定见[英文 API 参考](API.md)。没有公共分配器故障注入 API。

- 共享的可变对象不保证线程安全；跨线程使用时应由应用同步。
- 不同 Group 的 Path 是各自坐标空间中的值，直到合并建立结果的坐标空间。
- 库不提供整个 Tree 序列化、固定内存上限、公共自定义分配器或故障注入 API。
