# V3 Preview.2 最终端点策略审计（历史译本）

原文：[v3-preview2-final-metadata.md](v3-preview2-final-metadata.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v3-preview2-final-metadata.md)。原文及原始数据保持不变。[双语导航](../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

原04cdd0c候选保留于v3-preview2-comparison.csv、v3-preview2-diagnostics.txt、v3-preview2-holdout.csv及v3-preview2-metadata.md，使用端点直接深度8。最终候选只将私有允许深度改为16；新增聚焦OOM测试、诊断path_max字段和文档不影响计时库算法。

测量环境为Windows 11、AMD Ryzen 9 9955HX、MSYS2 UCRT64 GCC 16.2.0、C17，严格 `-Wall -Wextra -Wpedantic -Werror`，计时有效优化 `-O3 -DNDEBUG`。原候选A使用CMake Release二进制；实验与最终候选从各自源码以相同有效优化编译。A/B/C/D计时比较应使用匹配的v3-preview2-policy-audit.csv；v3-preview2-final-comparison.csv独立确认最终源码，不是逐字节复刻。

policy-audit比较A=原深度8、B16=深度16、C=无端点深度触发、Dlog=随规模对数增长端点上限。另测深度10和12。policy-cross比较主要种子 `0x91A30D47` 及保留种子 `0xDEADBEEF` 的A8/B16。最终计时CSV的非单调负载含两个种子。每行一次预热、三次测量，全部升序1m行例外为两次。最初三次主要交替比较波动约3%，因此每版本重复九次：A8/B16中位数72.991/73.080ms（主要）、71.409/71.845ms（保留）。这不支持实质交替回归。

final-diagnostics使用独立 `-O2` 诊断构建，定义 `LKS_ENABLE_ALLOC_DIAGNOSTICS`、`LKS_BENCH_DIAGNOSTICS`、`LKS_BENCH_STAGE5_DIAGNOSTICS`。新path_max报告最终最大Path深度。诊断时间不与计时CSV比较；relabel和placement沿用benchmarks/README.md字段定义。累计请求字节计成功库分配请求；峰值为诊断分配器峰值。policy-diagnostics保留500k和1m的A8/C/Dlog/E12及最终B16深度、分配、修复、比较器、旋转、高度记录。baseline-extra补充Stable V2与Preview.1的相等300k、升序500k及降序100k/300k；500k一次预热两次测量，其他一次预热三次。

精确深度9事件在A的插桩detached检出中观察：升序及全相等append的第261,579次插入成功分配合法Path `0RUc/RUc/RUc/RUc/RUc/RUc/RUc/RUc/DEq`。前驱存在、后继不存在，append_run=261577、prepend_run=0、端点步长1。仅深度8策略触发重标记，没有坐标间隙或分配失败；同一候选可合法直接插入。A在502,918、742,003、981,088次插入重复事件，表明约固定规模反复陡增，而非几何间隔。B16首次深度17候选接近523,723次插入。prepend采用负根level进位，至500k无重标记。

结果仅描述该机器与确定性负载。端点规则私有且临时；全范围重标记仍可触及n节点，不声称完整managed插入的最坏或正式摊销界。无触发变体1m最大深度31、峰值288,763,760存活字节，是拒绝该变体的实测原因。
