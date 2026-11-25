# 基准指南

[English](README.md)

当前已发布的 Full Stable 为 [v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。参见 [4.1.0 内存与性能研究及限制](../docs/V4_1_0.md)、[历史 Stable 记录](../docs/V4_STABLE.md)和[冻结契约](../docs/V4_FREEZE.md)。
[4.2 RC 证据](../docs/FULL_RC1_EVIDENCE.zh-CN.md)从归档的最终 Preview.3 运行导出图表及汇总。重新生成时保留原始数据。以下旧测量仍用当时的名称，Preview.4 为收敛证据。Stable 发布未另作调优。

## 历史 V3 基准程序（3.1.0 源码）

保留原有 V3 程序与数据。历史 V4 Preview.2 增加了区分生产和诊断的修改检查，见[方法](../docs/V4_PREVIEW2.md)。当时的本地 V4 Preview.1 分支启用基准后还构建 `layerkeysort_v4_smoke`。其有限的显式顺序检查和适配器差异见 [V4 Preview.1](../docs/V4_PREVIEW1.md)，适用于该阶段的基本检查。

3.1.0 保留 v3.0.0 的生产排序代码与基准程序。历史 Preview 和 RC 数据来自当时的版本；没有新采集 3.1.0 耗时 CSV。Stable v2.0.0 保留 V2 Preview.4/Stage 5 数据。当时的程序用 V3 比较器托管 `LksOrderedTree` 插入测量 `tree`。两个比较文件分别记录 Stable V2 与发布 Preview.1 的早期比较，以及 Stable V2、Preview.1、原 Preview.2 候选的直接比较。长时间修改测试另用于正确性验证。

`current_benchmark.c` 计时公开操作，验证和可选诊断使用私有测试接口。每次打印一行 CSV：

```text
operation,distribution,n,warmups,repetitions,median_ms,min_ms,max_ms,output_bytes
```

使用 C17 `timespec_get(TIME_UTC)` 墙钟时间。每个进程在相同确定输入上按要求预热并独立重复测量，主要报告中位数，最小最大值反映分布。输入准备、验证及 Tree/Group 结果销毁不计时。Tree 创建不计入插入；Path 解析计入逐项销毁。Path 格式化和解析每次测量执行 100 个调用，此时 `n` 是 Path 深度，`output_bytes` 不含 NUL。修改测试计时前创建初始 Tree 和候选 Path，`mut_mixed` 每次执行三项操作、迭代 `n/2` 次，其他修改执行 `n` 次。

计时之外检查有序、API 承诺的等值稳定性、Tree 大小、修改对象身份或 Path 往返。标准 C 不要求 qsort 稳定，因此只检查其有序性。另行执行正常正确性测试。

## 构建和运行

使用 `-DLKS_BUILD_BENCHMARKS=ON` 启用可选目标，优化 Release 构建后运行 `layerkeysort_benchmark`。启用测试时 CTest 执行三项小型基准自检。为复现历史 GCC 对比，对各版本既有生产 `src/*.c` 使用相同 GCC 和 `-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`。针对各自版本的 `include/` 和 `src/` 编译同一程序；仅在编译缺少后续 API 的已发布 Preview.3 时定义 `LKS_BENCH_PREVIEW3`。对照副本及构建输出放于独立目录。计时无需诊断。

`run_matrix.ps1` 接受程序、版本标签、CSV 路径和 `Common` 或 `Current` 模式。Common 仅用 Preview.3 兼容的公开操作，Current 增加更大规模、修改与 Path 文本和键案例。`hotspot` 先插入固定最小键，之后反复在其后直接插入，覆盖固定内部间隙。可用 C 程序的可选种子运行独立留出组。

```powershell
./benchmarks/run_matrix.ps1 -Executable ./layerkeysort_benchmark.exe `
  -Version current -Output ./current.csv -Profile Current `
  -Warmups 1 -Repetitions 7
```

诊断与计时分开。定义 `LKS_ENABLE_ALLOC_DIAGNOSTICS` 与 `LKS_BENCH_DIAGNOSTICS`，添加 `src/` 头路径并执行一次 `tree`。`path_max` 给出分布、数量及最终最大 Path 深度。`footprint` 给出分布、数量、存活 Path 对象及步骤字节、步骤总数、显示文本字节（逐项含 NUL）及 LK1 字节（逐项含 NUL）。后两项估算将坐标保存为 C 字符串的大小，与 Tree 驻留分配不同。`relabel.nodes_relabelled` 统计成功插入时替换的旧坐标；直接插入次数加重标成功数即新坐标数。`relabel.max_region_nodes` 为最大成功区间，而 `placement.max_attempted_region` 也包含失败规划。

`diag` 依次报告分布、数量、比较器调用、比较器搜索步数、旋转、AVL 高度、修复尝试、成功、回退、扩展、最大区间节点、重标节点、深层接受、完整重建、分配调用、累计请求字节、平均/P95/P99 Path 深度、平均/最大显示长度、平均/最大 LK1 长度、存活字节峰值、存活 Tree 节点字节、Path 对象字节、Path 步骤字节及 Tree 节点 `sizeof`。定义 `LKS_BENCH_STAGE5_DIAGNOSTICS` 后，`rebuild_reasons` 还提供间隙或深度限制尝试以及成功完整重建中的节点数。此类字段对应 Stable V2，V3 托管插入为零。

V3 的 `relabel` 另给出分布、数量、直接插入、尝试、成功、扩展、旧节点重标总数、最大区间、全区间重标次数及全区间旧节点数。私有计数无 ABI 承诺。Preview.2 的私有 `placement` 包括端点或内部直接插入、burst 插入、端点或内部重标尝试及成功、尝试区间累计和最大节点数、生成的重标 Path、候选深度和与最大值。对比还含临时 Preview.1 工作计数，改动过的对照工作树随后恢复。

以 `LKS_BENCH_COUNT_COMPARISONS` 编译 Preview.3、Stage 4 和 Stable V2，运行相同 `tree` 案例以对比比较器调用数。该构建未用于公布的计时表。实测设备、负载、结果、边界及版本差异见[基准报告](../docs/BENCHMARKS.md)。30 行 Preview.3、61 行 Stage 4/5 矩阵见 [results](results/)。各行含范围和中位数；比较分布，无需逐字节重现毫秒值。

## 发布后负载证据

[v3.1.0 负载证据](../docs/WORKLOAD_EVIDENCE.md)包含四个合成应用类负载、配对生产/诊断运行，以及逐插入延迟、重标和存储增长。精确假设与复现见[模拟方法](WORKLOADS.md)。发布算法、历史对照及数据保留。

## 历史 V4 Preview.3 导出

`layerkeysort_v4_snapshot_smoke` 分别测量捕获、键格式化、线格式、加载、恢复、导入和导出频率。独立诊断程序报告请求分配及居民字节。生产和插桩计时分开。[原始检查及方法](results/v4-preview3-smoke/README.md)属于阶段验证。

## V4 Preview.4 生产语义矩阵

`v4_trace.c` 和 `run_v4_matrix.py` 使用单调计时器、独立平面实例模型、独立链接的生产和诊断版本，以及有限的 B64/B128/B256 筛选。既有研究实现保留在归档。[原始矩阵及定义](results/v4-preview4/README.md)说明规模、种子、百分位、保留快照、频率与 RSS 限制。Group/Batch 对比在 `v4_groups.c`。

用 `-DLKS_BUILD_BENCHMARKS=ON -DLKS_BUILD_CAPACITY_SCREEN=ON` 构建 Release，再运行 `python benchmarks/run_v4_matrix.py --build build --output build/new-evidence`。使用新输出目录，保留已采集证据。V4 正常修改计时和诊断计数分开。V3 无稳定居民 Handle，删除含比较器查找；该适配成本明确计入。V3 LK1 格式化逐行构造调用方分配的临时键，因此不能直接与 LS1 大小或成本相比。
