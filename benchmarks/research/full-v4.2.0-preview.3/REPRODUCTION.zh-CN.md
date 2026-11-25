# 复现（历史译本）

原文：[REPRODUCTION.md](REPRODUCTION.md)，英文，固定版本 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.3/REPRODUCTION.md)。原始记录保持不变。[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)集中提供两种语言导航。

从仓库根目录运行 replay_live.py，指定新的输出目录。脚本使用固定的公开归档、GCC/CMake/Ninja 和 Cargo --locked，并核验冻结的轨迹哈希。用 archive_matrix.py 归档，再运行 summarize_matrix.py 重新计算。final-matrix 包含两台主机的完整原始凭据及工具链元数据。

在原生 Windows 或 Linux 上运行 run_lifecycle.py，指定新的输出目录，并事先取得脚本指定的精确 Git 对象。analyse_lifecycle.py 将原始 stdout 与记录相互核对，重新生成逐条件统计。历史 probe 文件夹保留非对称 Treap 扫描，不纳入最终估计。Linux 抓取失败凭据记录了测量前的源码暂存失败；之后的非浅层源码暂存已完成。scale.py 与 holdouts.py 使用实际严格 Windows Release 构建。留出验证的诊断运行和计时运行分开。

### 重新计算墙钟与阶段比率

对每台归档主机运行 `python analyse_lifecycle.py <copied receipt directory>`。脚本先核对进程 stdout 与原始行，再写入 analysis.json。使用副本以保持归档快照不变。原有 `ratios` 键概括匹配条件的阶段时间总和中位数比率；`wall_ratios` 独立概括匹配条件的进程墙钟中位数比率，只有输入记录了墙钟时间时才出现。两种比率均排除预热及诊断条件。
