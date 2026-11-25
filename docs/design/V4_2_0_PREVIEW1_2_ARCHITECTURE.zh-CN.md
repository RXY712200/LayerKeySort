# Preview.1.2 有界块标签（历史译本）

原文：[V4_2_0_PREVIEW1_2_ARCHITECTURE.md](V4_2_0_PREVIEW1_2_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.md)。原件保留；[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

已接受 `22544cf283694dd27cef203caba9cf1838845947` 用高度引导AVL祖先找块相对顺序。候选 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` 保留它、每块加私有无符号64位tag。两tag块比无符号值，同块比resident位置，任一无tag用旧AVL；AVL继续managed搜索及结构顺序职责。

## 维护与不变量

非零tag沿链严格增加，零为无标签。v4_tag_attach围绕新块逐渐扩宽连续区域，最多64，仅在外部有tag界或端点哨兵、无符号间隔充足时选区。重赋均严格处于界内，保持外部标签有效。端点步长封顶，为后续增长留空间。先减后除算术，每赋值受上界限制。

失败则新块留零，后续成功重叠修复可能赋值，但不保证每个零回收。一次attachment最多检查64种宽度、写64tag。删除保留余序，旋转保持链序，记录调整沿用位置维护。lks_order_bulk_build初始广间隔；来源、快照、持久格式不带tag。

有tag跨块常数工作，B块回退O(log B)。因此每块接入增加有界成本，有限标签饱和由回退处理；每块多一word，repair无分配/回调。准备、失败发布、revision、no-op、地址/所有权不变。

## 候选选择

Dietz/Sleator1988区分维护标签与比较；Bender等2002用间接/密度维护。归档MIT j-hui tag/list-range在自身条目维护标签；独立C候选在Full既有resident块作有界重分布并保留结构回退，不能继承经典完整理论保证。

先前仅中点原型集中间隙约1,057次耗尽，本扩展测真实重分布。删subtree count消融Release基本持平，invariant测试仍读该字段，故保留。[预定协议](../../benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.zh-CN.md)及[阶段报告](../V4_2_0_PREVIEW1_2.md)保留证据/门槛。
