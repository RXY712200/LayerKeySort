# Benchmark历史（中文历史译本）

原文：[BENCHMARK_HISTORY.md](BENCHMARK_HISTORY.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/history/BENCHMARK_HISTORY.md)。原件和数据保留；[双语清单](../design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

归档Preview及V2/Stage比较、策略试验和外部生成器语境。数字只对应原commit、机器、harness和范围；归档“当前”指历史阶段，非V3.0.0 Stable。当时Stable代入口为[V3性能证据](../BENCHMARKS.md)；该链接随当前文档演进，历史数字以下列固定证据为准。

## V3 Preview.3 开发诊断

Preview.3稳定化未新增受控跨版本计时；当时报告的同源码验证自行记录单版本耗时。harness新增固定内部 `hotspot` 插入，以及最终resident Path分配、total steps、display/LK1 C-string bytes的 `footprint` 记录。`relabel.nodes_relabelled` 计成功托管插入替换的旧坐标；`max_region_nodes` 计最大成功区域。诊断与wall-clock分开，不证明渐进界。以下Preview.2/V2测量保留原标签/数值。

## 已发布 V3 Preview.2 对 Preview.1 和稳定 V2

已发布Preview.2在端点slot满时向可用祖先进位，长成功端点run后用一slot stride。原 `04cdd0c` 候选直接端点深度上限八，合法depth-nine append仅因该策略在第261,579次插入触发full-range relabel。最终候选允许端点直接坐标深至16，更深或不能生成时保adaptive relabel。这是暂定有界增长策略，不是Path合法深度限制或复杂度保证。

[原三版本比较](../../benchmarks/results/v3-preview2-comparison.csv)使用干净精确stable V2/Preview.1；独立[holdout CSV](../../benchmarks/results/v3-preview2-holdout.csv)变更输入seed。[最终候选计时](../../benchmarks/results/v3-preview2-final-comparison.csv)、[最终诊断](../../benchmarks/results/v3-preview2-final-diagnostics.txt)、[策略试验计时](../../benchmarks/results/v3-preview2-policy-audit.csv)、[策略诊断](../../benchmarks/results/v3-preview2-policy-diagnostics.txt)、[跨负载holdout](../../benchmarks/results/v3-preview2-policy-cross.csv)记录最终选择/拒绝替代。[原metadata](../../benchmarks/results/v3-preview2-metadata.md)给机器/flags/区间/诊断边界。

下表为 `04cdd0c` **原depth-eight候选**，Preview.2列不是最终候选。

| 托管插入 | N | Stable V2 毫秒 | Preview.1 毫秒 | 原Preview.2 毫秒 |
| --- | ---: | ---: | ---: | ---: |
| 升序 | 100,000 | 303.922 | 362.676 | 54.104 |
| 全等 | 100,000 | 303.207 | 362.617 | 53.426 |
| 32值重复 | 100,000 | 424.473 | 107.261 | 109.607 |
| 随机唯一 | 100,000 | 97.487 | 96.812 | 98.830 |
| 交替 | 10,000 | 189.293 | 75.047 | 73.228 |
| 降序 | 10,000 | 2.984 | 2.982 | 3.080 |
| 升序 | 300,000 | 3343.667 | 3616.003 | 505.805 |

单机中位数，一warmup三重复。primary的duplicate/random/descending略差于Preview.1；holdout duplicate/random改善，任一seed均不能推出通用排名/复杂度。ascending100k中Preview.1五次full-range relabel，访问719,468 region nodes，Preview.2零次。[诊断日志](../../benchmarks/results/v3-preview2-diagnostics.txt)记录三版relabel、allocation、Path depth、comparator calls、AVL rotations/height。ascending300k原depth-eight候选一次261,578节点full-range，最终depth16避开。500k有实际代价：最终独立run中位889.976 ms，高于原候选749.108 ms；另一匹配策略试验depth16为882.634 ms。1,000,000 ascending原候选四次full-range共2,487,584旧节点，depth16一次523,722。无深度触发候选零次，但最终mean/P95/P99/max深度15.780/30/31/31、peak288,763,760 bytes；depth16为6.173/14/15/16、peak165,980,120。

| 升序端点策略 | 300k全范围重标记次数 | 500k中位毫秒 | 1m全范围重标记/旧节点 | 1m累计请求bytes | 1m峰值live bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| 原depth8 | 1 | 749.108 | 4 / 2,487,584 | 1,237,677,144 | 261,503,264 |
| 固定depth12 | 0 | 1,113.694 | 2 / 1,156,008 | 843,087,256 | 210,582,904 |
| 最终depth16 | 0 | 882.634 | 1 / 523,722 | 588,908,496 | 165,980,120 |
| 无深度触发 | 0 | 844.408 | 0 | 496,745,496 | 288,763,760 |
| 自适应log-size上限 | 0 | 860.316 | 0次全范围；一次524,288节点区域重标记 | 766,487,448 | 224,656,312 |

同机；500k计时为独立优化run，一warmup三测量，诊断内存来自instrumented构建。1m另一次匹配run一warmup两测量：depth8为3,580.605 ms，depth16为2,456.653，no-depth为3,189.265，adaptive log-size为3,098.101。log-size复杂且1m内存差于depth16；无界方案1m深度增长不适合作通用取舍；depth12仍反复relabel。depth16避开具体300k cliff，并在实测范围约束增长。首depth17候选在约第523,723次插入触发full-range，因此成本延后并减少，未消除。equal append结构相同；descending/prepend至500k最终深度一、无relabel。同规模comparator calls/AVL rotations/height不随这些阈值改变。

下表把最终候选与stable V2/已发布Preview.1并列。V2/Preview.1的100k和ascending300k来自上面直接比较；另300k equal、500k ascending、descending来自[补充baseline](../../benchmarks/results/v3-preview2-baseline-extra.csv)。原Preview.2尽可能取匹配策略试验，最终版来自独立最终源码CMake构建；这是分开run观察，不是受控配对speedup。

| 托管插入 | N | Stable V2 毫秒 | Preview.1 毫秒 | 原Preview.2 毫秒 | 最终Preview.2 毫秒 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 升序 | 100,000 | 303.922 | 362.676 | 54.933 | 54.574 |
| 升序 | 300,000 | 3,343.667 | 3,616.003 | 510.985 | 339.946 |
| 升序 | 500,000 | 9,689.246 | 9,547.128 | 749.108 | 889.976 |
| 全等 | 100,000 | 303.207 | 362.617 | 55.500 | 55.956 |
| 全等 | 300,000 | 3,273.642 | 3,593.092 | 520.029 | 355.181 |
| 降序 | 100,000 | 35.823 | 35.583 | 36.456 | 35.760 |
| 降序 | 300,000 | 111.815 | 114.281 | 113.631 | 117.469 |
| 随机唯一 | 100,000 | 97.487 | 96.812 | 99.179 | 101.240 |
| 32值重复 | 100,000 | 424.473 | 107.261 | 110.067 | 110.557 |
| 交替 | 10,000 | 189.293 | 75.047 | 72.623 | 74.479 |

最终alternating看似更慢促使九重复复查：primary原/最终中位72.991/73.080 ms，holdout71.409/71.845 ms，未建立实质alternating回归。500k ascending/all-equal代价上升可重复，是选定深度策略真实取舍。内部插入仍有界逻辑range relabel、几何扩张和full-range正确性fallback；生成/比较随Path深度，完整插入不声称最坏 `O(log n)` 或形式化摊还界。

## V3 Preview.1 对 stable V2.0.0

实验V3 Preview.1已发布。[v3-preview1-comparison.csv](../../benchmarks/results/v3-preview1-comparison.csv)比较公开managed insertion与干净detached stable `v2.0.0`。均MSYS2 UCRT64 GCC16.2.0、C17、`-O2 -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG -std=c17`（后置`-O3`来自CMake Release），同Windows11/Ryzen9 9955HX。seed `0x91A30D47`，每行一warmup三run，以下中位毫秒。编译器/热状态/调度限制外推。[metadata](../../benchmarks/results/v3-preview1-metadata.md)给checkout/flags/测量边界。

| 托管插入负载 | N | Stable V2 | V3 Preview.1 |
| --- | ---: | ---: | ---: |
| 升序 | 100,000 | 327.913 | 421.662 |
| 全等 | 100,000 | 331.146 | 419.852 |
| 32值重复 | 100,000 | 477.263 | 126.797 |
| 随机唯一 | 100,000 | 112.053 | 112.892 |
| 交替 | 10,000 | 216.716 | 85.698 |
| 两值 | 10,000 | 16.918 | 7.562 |
| 八值 | 10,000 | 14.393 | 8.593 |
| 64值 | 10,000 | 12.501 | 9.041 |
| 降序 | 10,000 | 3.528 | 3.517 |
| 中部热点 | 10,000 | 3.816 | 3.733 |
| 升序 | 4,000 | 1.433 | 1.444 |
| 随机唯一 | 4,000 | 4.980 | 3.609 |

ascending/all-equal100k实质回归，duplicates/alternating明显改善，random100k该尺度相近。去掉物理replacement-Tree rebuild不代表各分布都快；大型比较是该机工程证据，非渐进证明。

独立诊断构建（`LKS_ENABLE_ALLOC_DIAGNOSTICS`、`LKS_BENCH_DIAGNOSTICS`，同GCC flags）显示机制：

| 负载 | V2物理rebuild/重建节点 | V3全范围重标记/旧节点 | V3全部重标记节点 | V2/V3比较调用 | V2/V3峰值live bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| 升序 100k | 9 / 522,876 | 5 / 292,764 | 293,524 | 1,639,288 / 1,568,929 | 28,341,744 / 22,181,704 |
| 重复 100k | 10 / 556,781 | 0 / 0 | 97,256 | 1,634,918 / 1,588,203 | 29,648,128 / 10,798,784 |
| 交替 10k | 26 / 134,148 | 1 / 189 | 119,789 | 132,835 / 146,639 | 2,601,016 / 1,296,120 |
| 全等 100k | 9 / 522,876 | 5 / 292,764 | 293,524 | 1,639,288 / 1,568,929 | 28,341,744 / 22,181,704 |

V3架构下物理replacement-Tree rebuild为零，但full-range relabel仍每rank分配新Path，成本可高。ascending100k V3 mean/P95/P99深度约3.987/6/7，V2为3.881/5/6；duplicates V3为2.146/4/5，V2为3.796/4/4。因此精确生成Path和私有阈值非稳定契约。

不计comparator成本，AVL查找为索引高度工作；direct insert另含Path生成、链接/rebalance。relabel planning访问几何扩大的逻辑区，成功时改k个已有Path，可能k=n。不声称完整插入最坏 `O(log n)` 或形式化摊还界。

## 稳定 V2 历史证据（Preview.4及Stage5）

Stable v2.0.0保留发布 `v2.0.0-rc.1` 生产代码。下列Stage4/5是历史开发快照，数字仅单机/所述负载，非普遍排名。生成Path与私有policy仍实现细节。Preview.5补使用体验/正确性耐久覆盖，未重跑替换Stage5计时；mutation soak是正确性测试非性能排名。

## 复现与测量边界

公开API harness为[benchmarks/current_benchmark.c](../../benchmarks/current_benchmark.c)，矩阵runner为[run_matrix.ps1](../../benchmarks/run_matrix.ps1)；[README](../../benchmarks/README.md)给区间和正确性核验。可选CMake `-DLKS_BUILD_BENCHMARKS=ON`，三个小CTest case检查harness；计时binary不含诊断counter。

Windows11 Pro `10.0.26200`，AMD Ryzen9 9955HX（16cores/32logical），约15.2 GiB RAM、AC供电、MSYS2 UCRT64 GCC16.2.0。三版同 `-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`。每CSV一warmup七测量，毫秒median/min/max。输入生成/输出验证在计时外；Tree insert不含Tree create/最终destroy。seed `0x91A30D47`，holdout `0xDEADBEEF`，detached checkouts：

| 标签 | 提交 |
| --- | --- |
| Preview.3 | `9fa2eadcca422be9483a050505f0c332558c5908` |
| Stage 4 | `4ef9e240ab08edbb7875c0b2934313c126307e73` |
| Stage 5 | `99f84ae5228ea67ade74eaf206b17dc5e6304ffe` |

`Common`矩阵30行适用Preview.3；`Current`61行。精确归档为[preview3.csv](../../benchmarks/results/preview3.csv)、[stage4.csv](../../benchmarks/results/stage4.csv)、[stage5.csv](../../benchmarks/results/stage5.csv)，每行含min/max/median。runner可重跑，普通机器波动不允许字节一致时间。Preview.3无remove/rekey/display parser/durable-key，故这些格无历史比较。接近clock/noise floor的毫秒差不当改善；调度/cache/power/allocation未作实验室控制。

## 结果

中位毫秒，低更好。equal指全比较相等；duplicates来自小值集；alternating在值域两端反复插入。

| 公共操作/分布 | N | Preview.3 | Stage 4 | Stage 5 |
| --- | ---: | ---: | ---: | ---: |
| Tree插入 / 升序 | 10,000 | 8.122 | 5.635 | 5.245 |
| Tree插入 / 降序 | 10,000 | 7.680 | 3.754 | 3.359 |
| Tree插入 / 随机 | 10,000 | 11.743 | 10.197 | 9.806 |
| Tree插入 / 全等 | 4,000 | 937.480 | 1.517 | 1.354 |
| Tree插入 / 重复 | 4,000 | 81.190 | 6.822 | 6.572 |
| Tree插入 / 交替 | 4,000 | **65.951** | 75.359 | 77.934 |
| Tree插入 / 两个不同值 | 4,000 | 279.084 | 4.804 | 4.611 |
| Tree插入 / 64个不同值 | 4,000 | 45.347 | 4.468 | 4.395 |
| Group构建 / 随机 | 10,000 | 6.353 | 2.550 | 2.617 |
| Group合并 / 重复 | 10,000 | 5.827 | 2.001 | 2.000 |
| GroupBatch构建 / 重复 | 10,000 | 3.674 | 1.921 | 1.929 |
| GroupBatch合并 / 重复 | 10,000 | 5.814 | 2.151 | 2.152 |
| `lks_sort` / 随机 | 10,000 | 0.679 | 0.666 | 0.661 |
| C `qsort` / 随机 | 10,000 | 0.645 | 0.642 | 0.654 |

100,000 ascending Stage4为2,120.870 ms、Stage5为319.905；equal为2,137.550/340.463；descending43.546/38.759；duplicates472.106/466.507。因此Stage5未解决每类拥塞。random1,000 Preview.3为0.607 ms、Stage5为1.343，新AVL非各尺度/分布更快。

首100,000 random矩阵Stage4为112.967、Stage5为124.081 ms，Stage5跨度108.651–139.708。两组交错复跑Stage5为108.834/106.017，Stage4为113.406/110.530，初始回归未重复。Group random100,000矩阵Stage4/5为51.000/35.448，交错为37.923/34.554及37.040/35.333，顺序重叠，不声称Stage5 Group改善；Batch同样重叠。`lks_sort`仍稳定排序便利API，C `qsort`无需稳定，相等稳定有要求时不能替代。

## Tree 为何改变

Path-keyed AVL和comparator upper-bound早于Stage5；独立instrumented比较器计数：

| Tree分布/N | Preview.3调用 | Stage4调用 | Stage5调用 |
| --- | ---: | ---: | ---: |
| 升序 / 10,000 | 210,463 | 123,617 | 123,617 |
| Random / 10,000 | 135,697 | 120,151 | 120,151 |
| Equal / 4,000 | 4,129,960 | 43,905 | 43,905 |
| 重复 / 4,000 | 302,023 | 45,148 | 45,148 |

Stage5不减比较次数，而去掉comparator insertion已有候选Path的短命clone。ascending10,000诊断中allocation calls49,999降30,000，cumulative requested bytes1,821,640降1,259,024；保留候选备用capacity使peak1,042,704升1,068,880，random10,000为1,132,528升1,171,584。当时64-bit Tree node48bytes。这是私有allocator统计，非RSS或可移植字节承诺。

更大改善来自hard depth内open-end不重复repair；interior保repair，open-end超过allowance仍先repair再full rebuild。ascending100,000 Stage4尝试64,262、放弃64,007；Stage5尝试90、成功81，两者九次full rebuild。诊断cumulative requested970,265,160降120,173,304bytes。Stage5成功full rebuild处理522,876节点，gap-limit rebuild尝试零、depth-limit九。这解释计时改善，不改变AVL search/comparator count。

| 诊断Tree条件 | AVL高度 | 旋转 | 均值/P95/P99 Path深度 | 峰值live bytes | live节点/Path对象/Path steps |
| --- | ---: | ---: | --- | ---: | --- |
| Stage 4 升序, 100,000 | 18 | 99,958 | 3.874 / 4 / 6 | 28,298,704 | 4,800,000 / 3,200,000 / 5,892,128 |
| Stage 5 升序, 100,000 | 18 | 99,958 | 3.881 / 5 / 6 | 28,341,744 | 4,800,000 / 3,200,000 / 6,001,184 |
| Stage 5 随机, 100,000 | 19 | 69,256 | 2.757 / 3 / 3 | 11,619,984 | 4,800,000 / 3,200,000 / 3,619,968 |
| Stage 5 重复, 100,000 | 18 | 125,629 | 3.796 / 4 / 4 | 29,648,128 | 4,800,000 / 3,200,000 / 5,902,816 |

peak含临时repair/rebuild，大于表中三类live总和；采用候选Path/端点规则使ascending Stage5 peak略高于Stage4，虽累计分配显著低。微小布局可使P95跨离散深度边界；此处P99仍六。

策略依赖负载。random100,000 Stage5：208attempt/141success/67fallback，一次full rebuild影响2,799。duplicates100,000：30,798attempt/30,534success，十次full rebuild影响556,781。alternating10,000仍26次full rebuild处理134,148。full rebuild为必要原子fallback和实测最坏成本。

## 策略试验与保留值

本地原型checkout均先同harness测试再拒绝/纳入；只有正确性/OOM不变且代表负载改善无实质反向代价才保留。

| 试验 | 证据 | 决策 |
| --- | --- | --- |
| preferred depth由4改3 | duplicate4,000改善6.724→4.386 ms，random10,000恶化9.838→21.444 ms。 | 拒绝。 |
| preferred depth由4改5 | duplicate4,000改善6.724→5.333 ms，但peak806,712→940,472、平均depth2.732→2.806。 | 等更广内存目标前拒绝。 |
| 最大repair窗64改32 | alternating10,000改善214.2→167.2 ms，但full rebuild26→40，holdout八值8,000恶化13.407→14.612 ms。 | 拒绝。 |
| 最大repair窗64改128 | random10,000恶化9.838→16.164 ms；alternating10,000恶化214→245 ms。 | 拒绝。 |
| 禁全部open-end repair | 破坏针对性N=18 local-repair契约并强制full rebuild。 | 拒绝，hard depth外保repair。 |
| hard depth内跳过open-end repair | 大ascending/equal明显改善，comparator count不变；holdout random20,000为18.615→18.518 ms，duplicates8,000为11.489→10.998 ms。 | 保留。 |

初slot32768、端点spacing10、preferred/hard4/6、bulk block26、repair windows8/16/32/64仍暂定；整数gap所需最小midpoint span2。无足够跨负载证据改变其余值；精确生成Path非持久兼容契约。

## Mutation、表示大小与外部语境

当时新API10,000操作中位：random Path removal3.177 ms，remove-all2.042，random rekey6.425，hotspot rekey5.245，mixed4.778。mixed每n/2轮做三操作；Preview.3无这些API。

10,000-step fixture为40,000 display bytes或109,238 LK1 bytes。每100call Stage5中位display format7.480 ms、parse20.807、key format7.760、parse8.775。LK1为可移植规范排序，非最小体积。Stage5未改parser/formatter；表示契约见[API.md](../API.md)。

探索比较用了公开C [fractional-indexing](https://github.com/sqliteai/fractional-indexing) 的 `generate_key_between`，commit `ddaf4147101462b7062c549f3fad82fe9775e645`，源码在本仓库外。adapter仅测**坐标生成**：`lks_path_after`/`lks_path_between`对其key generator并核排序；未比较Tree insertion、等值稳定性、Group、mutation、持久契约、ownership。均GCC `-O2`，外库需GNU C extensions，LKS保持strict C17。append10,000 LKS1.288 ms/外库0.439；hotspot4,000为0.361/1.394。最终长度append16display/38durable bytes对外库4；hotspot8display/17durable对669。不能据此判通用胜者。
