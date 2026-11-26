# Full v4.2.0 Preview.2：生命周期、下游集成与规模（历史译本）

原文：[V4_2_0_PREVIEW2.md](V4_2_0_PREVIEW2.md)，英文，固定版本 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_2_0_PREVIEW2.md)。原文和数据不变，候选、待审及版本状态均指该里程碑；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

[草稿PR #31](https://github.com/RXY712200/LayerKeySort/pull/31)目标是 `dev/full-v4.2.0-preview.1.2.1` 的 `07b739de1841256ed89236b4096d130f88206956`，干净起点为任务提交 `5e74ae71139e518f3c8d5f45261d91236e786255`。选择 `fe00536e7bd7f4e5128432ea4a29e54158074903` 的固定宽度LKS4SNP1转换，改善解码成本，不加分配、不改格式、不重开live核心。开发头为4.2.0-preview.2，已发布Stable仍v4.1.0。

## Step 3：等价生命周期边界

[协议](../benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.zh-CN.md)在新计时前提交，[架构决定](design/V4_2_0_PREVIEW2_ARCHITECTURE.zh-CN.md)追踪捕获、回调复制、所有权/当前性、验证、精确字节和先解析后恢复。

托管筛选及最终运行每主机覆盖13个预设条件、四替代实现，一次预热、七新正式运行、单独诊断，每主机/轮468行。覆盖1k/10k/100k occurrence、0/1/2/8/16保留历史、空/短/32/256/可变宽关联、8/4096字节namespace。捕获间轮换顺序，历史消费前销毁来源及载荷，独立验证LS1/精确wire，解码、解析进新Order后继续修改。C17链序及独立扁平历史仅比较共同成功语义，其分配失败、验证、来源义务较轻，没有完整managed索引、任意先后查询及发布保证。结果描述这些组合。

相对已接受Preview.1.2逐条件中位数比的中位数，越低越好：

| 指标 | Windows | Linux |
| --- | ---: | ---: |
| 筛选decode，wire原型 | 0.6330 | 0.3456 |
| 最终decode | 0.6712 | 0.4940 |
| 最终encode | 0.9979 | 1.0030 |
| 最终计时生命周期阶段总和 | 0.9667 | 0.9439 |
| 独立flat替代的计时阶段总和 | 0.8160 | 0.8447 |

100k、八快照、32字节关联锚点：Windows阶段总和153.858→135.795ms，Linux88.121→83.173ms；单decode15.287→9.416ms和11.460→6.574ms。初筛256字节固定关联稀释decode收益。代表锚点flat整体仍快（123.120/75.557ms），peak请求37,600,600对40,142,904字节。这是真实端点混合取舍，不能外推任意precedence或失败等价managed。

全部匹配生命周期条件wire候选未增请求peak或retained。100k×八锚点两精确Full的来源+历史live均36,279,424、peak40,142,904字节。按source revision复用快照因payload可变不可靠；restore准备与回调交错会改变OOM时resolver调用前缀。未采用共享、allocator或restore scratch原型。

## Step 4：精确原生OrdWeave集成

[OrdWeave归档](https://github.com/RXY712200/OrdWeave/tree/1e40955a783f91ec835170bea664bfa8e324b465/benchmarks/research/full-preview2-20261124)保留独立27-trace ZIP成员/SHA验证、回放及指标。原轮486行，对4.1.0和Preview.1.2；最终234行，为九条seed17轨迹×内存/持久两模式×三依赖构建。manifest给精确替换、上游pin、应用源码；三构建各12上游测试过。生成4096 occurrence持久化留出样本在三pin均过独立序列/checkpoint/history/reopen。最终候选原Full故障扫描到达37次insert、34次remove失败，顺序/revision不变，清理请求零。

27对照的Preview.1.2/4.1.0操作中位比内存0.6555、持久0.9803。新候选/Preview.1.2为0.9963/0.9892；checkpoint/reopen/history为1.0057/0.9841/1.0001。最差checkpoint 2,048 hotspot为1.1024，范围0.203–0.240对0.196–0.219秒重叠。Full微基准收益不建立数据库整体收益。

匹配插桩操作循环，stage+publish内存中位94.55%，stage含OrdWeave全局索引重建策略中的Full managed插入；直接collection edit约1.04%。持久WAL commit97.57%，其嵌套主flush95.12%，不是可相加无关计时或现场遥测。checkpoint/history/reopen单计，未独立分离stage比较器或全进程临时分配。兼容不需adapter/core修改，TD-001重建/TD-002回放仍属应用，未改下游生产文件/官方依赖。

## Step 5：资源范围与live保护

1m下所有准入聚集/散布存活轨迹经两收缩/再增长周期保持同10,000个Handle地址。两主机已接受/候选从42,753,776请求字节降431,576，最终稀疏312块、零无tag块，退出零残留。另覆盖10k/100k，零内存跳过，未尝试3m/10m。1m/1%锚点第二周期累计请求85,075,976，不等于retained。

独立Windows 的40项留出观察含100k churn/四capture、远距/热点drag、重保留10k。生产churn p99.9中位1.20→1.25µs，首Snapshot decode0.821→0.426ms；单独诊断p99.9为1.4→2.7µs，单诊断时间不证明反复生产tail。诊断peak11,492,708字节不变，归档QPC/支持工作集不同于请求。范围外大生命周期流量开放。

新45轨迹原生live safeguard每主机1,215行，候选/Preview.1.2中位Windows1.0189、Linux0.9985，最差1.0585/1.0282；对Stable4.1中位0.6461/0.6107，两主机45/45更快。新轨迹及生产diff保留live架构；wire-only改动未重复历史OM/4.0。见[复现/全部凭据](../benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.zh-CN.md)。

## 兼容、验证与文档同步

64函数/14类型、状态、所有权、不可变视图、callback/reentry、cursor/revision、LS1/wire/LK1冻结。本地严格GCC Debug/Release各39/39，API、C/C++源码/安装/合并、资产及外部消费者过；golden/畸形不变。同revision测试在来源/调用者存储销毁后验独立namespace及重复关联，既有capture/trim/marker/deserialize/restore到达失败路径仍覆盖。[精确HEAD检查](https://github.com/RXY712200/LayerKeySort/pull/31/checks)。

Stable实现/指南 `70d7f0ef5857b54381b863d292a213909d87459f` / `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`；原运行 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`、诊断 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`、指南 `07b739de1841256ed89236b4096d130f88206956`保留。本临时文档映射 `fe00536e7bd7f4e5128432ea4a29e54158074903` 及独立测试 `92fb7fad977f792a7d9e3e0e430a5d5f32e8914a`。

| 指南 | 受影响解释/证据 |
| --- | --- |
| [导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | wire四/八字节及已验decode，独立字节预期过 |
| [API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | deserialize验证/复制路径修订，status/view/ownership不变 |
| [内部](IMPLEMENTATION_INTERNALS.zh-CN.md) | snap_be_read/write无符号逐字节、两pass发布、无新分配 |
| [验证](IMPLEMENTATION_VALIDATION.zh-CN.md) | golden/失败保留，加同revision namespace/重复载荷独立性，原生/CI链接 |

[交接](FULL_CONSUMER_HANDOFF.zh-CN.md)记录版本、前后源码及OrdWeave core/adapter独立结果。Changelog标开发候选，公开构建/迁移仍适用。

## Technical Debt Impact

| 项目 | 影响 |
| --- | --- |
| FULL-RI-001 | 保留接受B64/AVL/tag及诊断 |
| FULL-RI-002 | 90新live条件在非目标guard内保留Stable比较 |
| FULL-RI-003 | 量化阶段迁移、改善精确decode；复制及resolver scratch刻意保留 |
| FULL-RI-004 | 原生生成负载并分全局索引/collection/WAL；精确子分配/外部负载开放 |
| FULL-RI-005 | 验1m→1%再生、保留历史、已知规模取舍；更大范围开放 |
| FULL-RI-006 | 加来源销毁/同revision独立覆盖，更广工程收敛开放 |
| FULL-RI-007 | 加共同成功比较及实测限制，无普遍选型推荐 |

当时独立接受待定，范围止Steps3–5。
