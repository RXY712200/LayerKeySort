# LayerKeySort 历史 v1 Path 演示

历史英文原页：[index.html](https://github.com/RXY712200/LayerKeySort/blob/540f905316749732be5818e9168fe0a2f2dc3bb9/docs/demo/index.html) · [打开原始交互演示](index.html) · [中文交互版](index.zh-CN.html) · [转到 V3 可视化](../v3/)

此译文解释历史演示的界面、显示和限制。它保留原始英文页面和执行代码，适用于原页所示的历史 v1。

## 观察对象进入顺序位置

柱高表示比较器值。默认 16 项运行显示从历史 v1 Group 构建记录的确切 Path 字符串。它们不是 V3 输出。

### 增量 Group 构建

已放置的值形成升序阶梯，标签保留原输入顺序。图例含排队项（Queued）、当前项（Current item）、已定位的边界（Located boundary）、已放置项（Placed）。Path 条带按当前柱的有序顺序列出已分配 Path。

| 原控件 | 用途 |
| --- | --- |
| Randomize | 改用随机输入 |
| Step | 插入下一项 |
| Play / Pause | 播放或暂停 |
| Reset | 重置 |
| Speed：Slow / Normal / Fast | 慢速、正常、快速 |

当前步骤面板展示当前值、原输入下标、定位位置及边界、分配的 Path、方向及深度、放置关系、既有 Path 改动数。未开始时还没有 Path。只有经核验的默认运行显示历史 v1 的确切 Path。随机运行展示次序并隐藏未经核验的 Path。Path 形态标记仅在本次运行已放置验证 Path 后启用。

## 与历史 v1 算法的对应

v1 Group 构建器按输入顺序处理对象。Tree 在有序子列表中二分查找，并沿前驱子节点下探，返回确定的邻居。动画展示已采集的 v1 位置和 Path，不虚构内部比较探针。V2 与 V3 的 Tree/Group 构建机制不同，本动画只展示 v1。默认值各不相同，因此本次运行不能演示等值稳定性。LayerKeySort 保留比较器等值对象的输入顺序。

这是历史说明性可视化，浏览器不运行当前生产 C。随机运行不展示确切 Path 文本，因为没有使用 JavaScript Path 生成器。
