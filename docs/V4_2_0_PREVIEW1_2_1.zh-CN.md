# Full Preview.1.2.1 上限研究（历史译本）

原文：[V4_2_0_PREVIEW1_2_1.md](V4_2_0_PREVIEW1_2_1.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/V4_2_0_PREVIEW1_2_1.md)。保留历史原文、结果及当时状态；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

决定保留Preview.1.2生产。双主机支持加强诊断checker，否决三个性能变体。[草稿PR #30](https://github.com/RXY712200/LayerKeySort/pull/30)叠在Preview.1.2 `83367fd9b3b7297db1642c2eea6159bf395ef286`，起点纯任务 `de84eb1c82c75e790ae65445a5bf5a00ed8bf103`。已接受实现仍 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`，诊断/回归 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`，版本仍4.2.0-preview.1.2。

## Steps 1–2：实测决定

[架构](design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.zh-CN.md)映射resident移动、分配、结构、标签、managed比较成本。[前瞻协议](../benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.zh-CN.md)先于新候选计时提交 `9df156dd425f940fa25bb131b7d9993d4557a420`，筛选来源repair直接转移及私有B32/B128；先前count消融/稀疏保留不足支持索引/Handle重写，运行比较路径不动。

每主机同15份冻结种子17轨迹，五混合×1k/10k/100k；每条件一次预热、七正式串行、一次诊断。每主机746命令凭据含诊断、保留profile、guard及构建。Windows本地及托管Windows/Linux共1,620回放行，托管QPC/CLOCK_MONOTONIC。绝对时间与硬件主机特定。

相对B64耗时比，条件等权，低为优：

| Host / variant | Overall | Compare-heavy | Update-heavy | Long | Hotspot | Short |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Windows / transfer | 1.0053 | 0.9808 | 1.0111 | 1.0091 | 1.0053 | 1.0001 |
| Linux / transfer | 0.9825 | 0.9733 | 0.9845 | 0.9792 | 0.9825 | 0.9985 |
| Windows / B32 | 0.9953 | 0.9953 | 0.9123 | 0.9138 | 1.0610 | 1.0869 |
| Linux / B32 | 0.9239 | 0.9411 | 0.8472 | 0.8806 | 0.8966 | 1.0078 |
| Windows / B128 | 1.1389 | 1.1323 | 1.2836 | 1.2329 | 1.1214 | 1.0134 |
| Linux / B128 | 1.1277 | 1.1184 | 1.1720 | 1.2180 | 1.0681 | 0.9871 |

列依次为主机/变体、整体、比较密集、修改密集、长距、热点、短距。

- Transfer不省分配/位置写，只去一结构路径scratch复制，效应不足两个路线。
- B32过Linux计时但未过Windows两个路线；两主机亦超过独立5%资源限：最大请求live +8.38%、peak +6.43%。100k短move托管Windows慢13.93%、Linux慢8.73%，本地也确认短move代价且未过整体两路线。
- B128省部分表示字节，但整体Windows慢13.89%、Linux慢12.77%，两者compare-heavy回归>5%；update-heavy中位增28.36%/17.20%。

[原始证据/脚本](../benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.zh-CN.md)留全部不利条件、中位/范围/IQR、请求内存、支持的工作集及分配。默认决定不依赖噪声阈值跨越；九适配器以可行候选为前提。先前OM/Stable比较仍为原固定历史观察。

## 修改、资源和标签健康

- 本地10k初始/5k操作诊断：长move位置276,797、索引6,038；端点44,888/4,395；managed随机190,112比较，含构建，不隔离cache延迟或分配器墙钟份额。
- 两主机100k B64聚焦insert为3,318 tag store、171 repair、零失败，单attachment最多四store；跨块样本100,224用双tag。删至256 resident后请求live由4,289,120降14,720字节。注入两个零产生96回退样本，后来append两者恢复，是既有有界策略内可达例子。
- 独立100k bulk保留状态不同：B64从1,563块至4,096混合move后1,605，含1,306同块、2,790跨块、86满目标，最终零无tag。B32位置168,761→108,999，索引1,637→3,950，53赋标签失败、最终44零块；B128索引733、位置281,914。此probe在内存用restore bulk，完整持久解码/恢复另由CTest覆盖。
- 原生操作profile独立种子、四正式进程、单独诊断；家族内最差候选/对照中位p99.9 Windows/Linux为transfer 1.17×/1.08×、B32 1.06×/1.03×、B128 1.46×/1.08×。max受时钟/调度/分配影响；工作集区别请求。任意长期回退及百万/稀疏生命周期当时仍开放，本轮止100k。

## 验证与同步

Checker拒绝跨零相等/下降非零标签；回归检查两种错误后恢复tag，再继续饱和/顺序。GCC普通生产order.c预处理逐字节相同，公开头/版本/CMake/其他生产文件同已接受Preview.1.2。稳定Handle、managed等值/重入、callback/OOM/revision/no-op、快照及64函数/14类型继承。

Windows严格GCC Debug/Release各39/39，冻结清单、C/C++源码/安装/合并、本地资产通过。transfer/B32/B128参数化修改模型通过，独立身份/顺序和最终零分配。隔离OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` 替入诊断提交为12/12，官方仍 `70d7f0ef5857b54381b863d292a213909d87459f`。见[PR最终平台检查](https://github.com/RXY712200/LayerKeySort/pull/30/checks)和[凭据](../benchmarks/research/full-v4.2.0-preview.1.2.1/verification/)。

Stable实现/指南 `70d7f0ef5857b54381b863d292a213909d87459f` / `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`；先前临时指南 `83367fd9b3b7297db1642c2eea6159bf395ef286` 对应生产 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`；本轮诊断 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`。

| 指南 | 受影响解释与验证 |
| --- | --- |
| [总导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | 分开诊断与不变公开比较；测标签/回退 |
| [API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | compare状态/owner/路径不变，区分诊断；清单和foreign-Handle测试 |
| [内部](IMPLEMENTATION_INTERNALS.zh-CN.md) | checker一word验非零子序列；块字段/B64/查找/分配保留 |
| [验证](IMPLEMENTATION_VALIDATION.zh-CN.md) | preview12_block_tags跨零拒绝相等/下降，恢复后继续强制饱和 |

[消费者交接](FULL_CONSUMER_HANDOFF.zh-CN.md)记录诊断台账与独立核心/adapter证据；公开API、用法、发布及兼容不需生产更新。

## Technical Debt Impact

| 项目 | 证据和剩余边界 |
| --- | --- |
| FULL-RI-001 | 分测占用/位置/索引，不支持新生产机制 |
| FULL-RI-002 | 同语言或资源否决全部有界变体；OM结论仅Preview.1.2范围 |
| FULL-RI-003 | 检查bulk/持久契约，广泛生命周期仍开放 |
| FULL-RI-004 | 更新隔离OrdWeave覆盖，adapter归因仍开放 |
| FULL-RI-005 | 测聚集/收缩再生/B32重回退，广资源仍开放 |
| FULL-RI-006 | 标签不变量加强，错误fixture和恢复证据；计时不能关闭回归/架构问题 |
| FULL-RI-007 | 复用容量/repair对照，一般选型仍开放 |

当时独立接受待定，工作止于有界Steps1–2扩展。
