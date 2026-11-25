# Full Preview.1.2.1 架构决定（历史译本）

原文：[V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md](V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md)。保留原件和历史决定；[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

## 实际基线与成本模型

主要实现为Preview.1.2 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`。稳定Handle独立分配；带双链的B64块保存resident指针，resident存block/local。普通placement和remove移位后缀并写位置，同块move修复受影响区间。跨块先准备新块再摘取，在最终目标放置后按最终邻居修复不足来源。拆分/重分配/合并可移动指针并重写位置/owner。除唯一块外最小半满。Bulk restore生成新Handle、均匀标签及平衡索引；标签不进入LS1或LKS4SNP1。

AVL在结构接入/摘除/旋转时维护高度与子树块数。Managed搜索依赖索引/比较器，便宜的标签compare不调用该比较器。跨块双方有tag则比较，否则高度引导AVL回退。先前匹配公开compare探针在Windows的100k测3.891ns。子树count消融基本持平，因此保留元数据和完整传播；无独立结构证明就删高度传播会危及不变量。

## 已测瓶颈地图

原生确定轨迹将分配、位置赋值、触及索引块、结构事件、managed比较与前景计时分开。它们是工作代理，没有硬件cache计数。10k初始/5k操作本地B64长move诊断为276,797位置赋值、460拆分、225合并、6,038索引触及；端点为44,888赋值、467拆分、4,395索引触及，含初始构建。Managed随机为190,112比较器调用和379,347赋值。端点/hotspot仍有resident分配，不能将插入耗时全归数组复制。

index_blocks保持原AVL结构定义，不含tag store。结构probe报告累计标签存储/修复、单attachment最大存储、占用、当前零标签数量、最长聚集及比较路线。逐操作尾计时含分辨率、分配器/runner干扰。Profile及重复hotspot/long回放相对只读对照揭示结构/局部性取舍。

## 筛选设计

| 方向 | 实际筛选与取舍 |
| --- | --- |
| 边界转移 | 替代两数组scratch暂存，只直接复制边界resident，必要时对右侧保留项作一次重叠移位；来源删除后local可能已旧，保留全部位置修复。保持B64 burn-in、拆/合阈值、AVL接/摘及标签策略。 |
| B32/B128 | 只改私有偶数容量，半满/临时溢出槽/bulk由其推导。B32缩短后缀但增加块/attachment；B128延长移位，以少索引节点交换更多块内工作。 |
| 标签恢复 | 保留有界64块重标记，测试100k聚集、收缩、两个故意零块的重叠恢复。跨多个零仍正确，非零子序列严格递增；成功重叠attachment可恢复，不要求每场景回收全部零标签。 |
| AVL/Handle表示 | 保留索引和稳定独立分配Handle；既有count移除及稀疏slab保留分析不支持新生产变体，本研究没有实测安全pool或分配器/cache因果隔离。 |

转移原型拼接保持先左后右；左目标数量增加则右前缀移左，减少则左后缀成为右前缀。余量与已接受算法相同。两个数组独立，边界复制不重叠。修复在免分配摘取之后运行，不引入失败点。容量变体用独立序列和既有修改模型验不变量。全部原型留研究区，未导入第三方代码。

## 采用的诊断修改

`c977b0fd14a501d769b9783bfdcc61266bd1b5ae` 的v4_check_node在已交叉检查线程顺序的AVL中序遍历中记录最后非零tag，零不覆盖前值，拒绝跨零相等/下降非零tag。仅诊断构建，checker加一个word，无新每块存储/遍历。回归恢复改过的tag，原饱和fixture继续验证AVL compare。不选择生产版本或运行算法修改。

[协议](../../benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.zh-CN.md)和[决定报告](../V4_2_0_PREVIEW1_2_1.md)将决定与测量关联。
