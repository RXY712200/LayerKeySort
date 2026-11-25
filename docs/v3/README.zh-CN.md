# V3 可视化fixture来源（历史译本）

原文：[README.md](README.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/v3/README.md)。保留原始说明和fixture；[双语清单](../design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)提供成对导航。本译本不意味着交互HTML已完成中文化。

默认Pages目标为[index.html](index.html)。浏览器不运行C或实现Path分配；fixtures.js含两条确定性LksOrderedTree插入序列的完整Path快照：混合值20, 10, 20, 30, 15, 20, 11, 12, 13, 14；内部热点0, 100之后插入23个50。

快照由[record_fixtures.c](record_fixtures.c)链接精确已发布RC.1生产库 `e7508550533f7c5f7f74b6c873fd8800576db954` 记录。随后发布的Stable未修改src/*.c。记录器仅借公开物理导航收集每次插入后的item/Path关联，另对每一对按lks_path_compare和数字比较器稳定等值序验证，并比较连续快照统计既有坐标变化。该记录热点的第23次插入重标记八个已有Path。

浏览器按数字比较器及原插入序排序记录项，并显示精确记录Path，运行时断言每个快照的变化计数。这是有限回放，不声称JavaScript实现匹配未来私有Path策略。生成坐标不是稳定业务身份或3.x兼容承诺。manual模式的顺序及最终HUD display/LK1对来自公开[layer-list示例](../../examples/layer_list.c)，刻意不虚构中间Path输出。

[历史V1展示](../demo/index.html)独立保留，不描述V3 Path分配。

`node test_replay.cjs` 无需浏览器/网络，检查Step、Reset、Play、Pause、模式切换及八个坐标变化高亮。脚本以小型DOM替身执行页面逻辑；视觉布局及浏览器特定运行行为需要另行审查。
