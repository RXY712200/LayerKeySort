# Full v4.2.0 Preview.1.2.1：有界性能上限探索（历史任务译本）

原文：[V4_2_0_PREVIEW1_2_1_TASK.md](V4_2_0_PREVIEW1_2_1_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW1_2_1_TASK.md)。保留原文；英文执行指令为准。本文完整解释当时授权，不授予现在重跑实验、改实现或发布的权限。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

发布于2026-11-24，所有者批准[计划#27](https://github.com/RXY712200/LayerKeySort/issues/27)/[登记#3](https://github.com/RXY712200/LayerKeySort/issues/3)既有Step1/7、2/7的一次额外有界扩展，连续实施/研究、一份英文报告。仅本包显式覆盖先前不再扩展默认，不加第八步或自动新Preview。趁成本结构已理解，识别消除可避免成本，判断额外收益是否值得采用；不承诺升版本、不求统治所有算法、不因可无限优化重开完成架构。完整证据支持的不采用合法。下一计划仍Preview.2 Steps3–5。

## 0. 精确起点、权威与隔离

在授权私有环境完整读现行WORKFLOW/RELEASE_POLICY、模板、仓库及Full更严格要求；私下记政策revision/访问，签发时为ba0c6fb6b35650608b857adba4ad0f672d65cac7。读#27/#3、完整Preview.1.2任务/报告/架构/复现、真实源/测试/契约/冻结/开发/归属/四指南/交接。私有政策、论文、凭据不得复制公开研究。

| 角色 | 精确版本 |
| --- | --- |
| 签发时生产main | c108acd72b37614f4be271c9747484c2dde5eac4 |
| Stable4.0.0 | 80004ae732daf696f7e9192e417de0edb0b2aafa |
| Stable4.1.0 | 70d7f0ef5857b54381b863d292a213909d87459f |
| 已接受Preview.1实现 | 22544cf283694dd27cef203caba9cf1838845947 |
| 保留#28 HEAD | 3a2b3eabd0153531e1064457519b40deb4af9842 |
| 已接受Preview.1.2主要对照 | 61e9290ec5ba47b9092bcb8ae7d61f13104ea839 |
| #29 HEAD/扩展基线 | 83367fd9b3b7297db1642c2eea6159bf395ef286 |
| 既有托管研究 | lks0011 research/preview1-2-matrix，638535b76e5609104723b8c80b5b190bf06332a6 |
| 实用OM对照 | j-hui/order-maintenance ac4b902d103beb30cee3316d20cd10701f52d15a |
| 下游应用/官方Full | OrdWeave b12a007c50c2f9efcbee500a14d3cf09dcc74f10 / 70d7f0ef5857b54381b863d292a213909d87459f |

在由精确#29加单独任务提交建立的dev/full-v4.2.0-preview.1.2.1工作，fetch验证实际HEAD/local/remote/upstream/源码身份/仅任务差异；保未知改动、隔离worktree。#28/#29开放独立保留，非发布版本，不改它们分支/PR/目标、不rebase/merge/publish/delete。主要对照Preview.1.2，非Stable/Preview.1。新draft叠加目标dev/full-v4.2.0-preview.1.2，记录真实base；若旧PR另集成，报告、不自行retarget。禁止重写历史/绕范围，计划字符串4.2.0-preview.1.2.1非Release，签发时tag不存在。

## 1. 证据与剩余机会

先前已解决大量问题，无实质源/测量理由不重做。已独立重算6,480行、720cells、每主机45冻结轨迹：Preview.1.2/Preview.1整体.898020 Windows/.885862 Linux；比较密集.370654/.343438；修改密集.966991/.935483；short .844812/.835955；long .912882/.894704；hotspot .973471/.920835。对4.1整体.641694/.572017、比较.218504/.206118；对tag OM .979252/.917306，list OM .983687/.880425，胜场分别23/45与28/45、23/45与33/45。匹配Windows C公开跨块100k从37.262至3.891ns；无实证不优先削已便宜compare。成功操作边界内OM整体差距基本缩小，剩余主要mutation、资源及聚集/churn耐久行为。

90主机/负载条件对Preview.1 peak最多+.599623%、live+.545010%；100k seed17比较密集候选3,895,688请求字节对任一OM13,436,784，非RSS。聚焦10k插324store/8repair/无fallback/单次最多2写，但无任意长期集中负载无回退保证。强饱和测零回退正确。补充Windows家族中位p99.9最差1.21x，Linux tail未测。通用诊断checker尚不显式查非零子序列严格单调，仅专测部分状态，是覆盖机会非已识别生产错。旧profile stable41指Preview.1非Stable。读[报告](https://github.com/RXY712200/LayerKeySort/blob/83367fd9b3b7297db1642c2eea6159bf395ef286/docs/V4_2_0_PREVIEW1_2.md)、[架构](https://github.com/RXY712200/LayerKeySort/blob/83367fd9b3b7297db1642c2eea6159bf395ef286/docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.md)、[回放证据](https://github.com/RXY712200/LayerKeySort/tree/83367fd9b3b7297db1642c2eea6159bf395ef286/benchmarks/research/full-v4.2.0-preview.1.2)。独立审核核36生产/头/构建blob及精确六平台39/39、隔离OrdWeave12/12；新改需新验证。

## Step 1/7扩展：系统瓶颈扫描与有界选择

### 1.1 复用并预定协议

复用公开回放/暂存、45轨迹/哈希、Windows QPC/Linux CLOCK_MONOTONIC/Rust Instant、锁定OM、诊断/独立模型/精确pins，不从零搭建。核source/input/env/executable身份；必要复用私有托管，不求全局Rust/Linux安装。仅授权从先前研究建立隔离lks0011 research/preview1-2-1-matrix，限本包harness/workflow/证据，不改论文/其他私事/任何main。优化生产计时与诊断分开，保原数据/失败，只新目录/manifest。

先短源码成本模型及小/中/大、混合/mutation热点实测，compare作control而非预设瓶颈；记mix/样本/噪声/平台。分开分配、指针复制/位置写、块占用/变化、index、tag、managed compare、harness；硬件cache确可用才用，否则明确proxy。原生匹配C/标志，不能总耗时作因果。Baseline/candidate串行轮换。新计时前提交benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.md，定候选预算、screen/holdout、取舍、资源准入并记commit。

### 1.2 调查五方向

不是五项强制生产修改。看真实caller/invariant/旧否定证据，按已测成本、增益、语义风险、维护/独立评估筛选，低值尽早给理由否决。

1. **动态块维护最高优先**：检查已优化suffix、普通插删/跨块/split/merge/redistribute/来源不足的指针与loop，调查可省写/复制、边界transfer、按占用split/repair、保块免新分配；不弱化提交占用/身份，不让原免分配操作新增分配。测局部性/热点/交替move/churn非仅append。
2. **AVL更新/index**：profile高/parent/balance/count/rotate/managed/bulk，找多余传播/元数据读写，保root/parent准确。旧count-only整体平/测试读取，无实质新假说不机械重删，不推测重写已快compare。
3. **长期标签健康**：64位非零沿链增，零对数fallback；attachment至多探64宽、写64tag，成功重叠可恢复但不保证。量fallback频率/累积成本、重分布、零持留/聚集、删后恢复/快路。仅实测有值才研究有界恢复/间隙密度/自适应；无全局stop-world/new失败点/无界retry。标签不公开作持久ID、不入LS1/wire，保正确fallback。
4. **容量/fill/cache**：B64原成本不同，小范围如32/64/128，16/96须理由。用真实私有偶数策略、适配默认假设测试不跳；测占用/块/shift/结构/tag/locality/内存/tail，含稀疏/churn/restore/scale/managed非仅append。容量私有保格式语义，一条快不改默认，综合明确才替B64，不加公共调参。
5. **Handle分配/访问/表示**：profile逐resident及cache，低风险重复load/元数据/call；pool/slab仅分配成本和旧稀疏保留失败有可信解法才重访。稳定地址、部分OOM原子、mass-delete回收、callback寿命绝对。旧64槽slab须1m→1%对抗存活/retained/peak/清理，禁止仅为跑分替allocator或新公共依赖。

### 1.3 约束并选择实验

扫描后最多三新实现家族/knob，小消融非新产品，定假说/拒绝标准。Preview.1.2常为control，优先一两有望而非强纳全部。独立变体与base/individual/有理combination消融。前瞻固定45子集含多规模update/hotspot/long及compare control，另确定结构/对抗/holdout；关键计时预热后七正式或有理便宜初筛。不对每拒原型跑全跨主机。可信方向需实际代码/测量非泛文献。无机会则负决定，仍完成Step2证据不制造代码。

架构文档docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md涵起点/实测瓶颈、假说/路径、选/拒原因、不变量/证明、复杂度/资源、失败/ownership/兼容/限制，区分现码/提案，选择后更新；相关第一方/实现在案，许可归属，不造新颖性。直接同任务Step2。

## Step 2/7扩展：集成、验证、测上限及选择

### 2.1 预设目标和停止

目标是对已接受1.2增量，非弱baseline。新测前用默认或在协议中前瞻论证替代，结果后不可改：

- 两原生主机路线A：45条件整体中位耗时至少降低5%。
- 路线B：update/hotspot/long中同两家族（各九条件）每主机至少10%，整体至少2%。
- 任一路compare-heavy不比1.2恶化>5%，除非另授权前瞻取舍；调查超波动反复>10%非目标，孤立保留；不采噪声跨阈，关键样本前瞻配对重复/噪声评估。给算术/权重/逐主机波动，不pool时间。
- 维护/资源纯收益可为研究结果；不达A/B则单列建议、先所有者决定，不追认PASS或版本。

默认primary请求live/peak增≤5%，协议可有理提至15%以换显著收益，超限需owner。大/churn保留、流量、支持RSS另测，勿边际速度牺牲4.1内存。无零增长保证。计时前tail guard默认家族中位p99.9不得超control1.5x反复且超分辨/噪声，不许未知反复长暂停或真实有界维护回归；不同取舍须前瞻理由。与精确1.2比较fallback，单trace零不代表普遍。正确/可恢复失败/冻结绝对，禁止事后fixture改、强采用或周期后再探索。

### 2.2 动态、规模及诊断

实跑普通插删移、端点/长/热点/random/managed/churn，含同/跨块满/非满、不同占用/restore状态。诊断分calls、位置、块/占用、split/merge/redistribute、rotate/index、tag宽/写/repair/fallback（单次max/分布）、零数/快回退比例，区分累计tag与per-op，说明index_blocks是否另计tag，勿破历史含义。

聚焦远超10k（可行100k）、偏斜split/churn、交替hotspot move、删除恢复/收缩再增、强饱和/多零区。保资源准入、duration/repeats、昂贵上限和真实skip非零成功。1k/10k/100k及有用安全大状态，1m+仅准入，针对决策风险非无差别巨矩阵。独立holdout种子/结构不用于选择。

扩checker按全序查非零严格增、忽略零，清晰拒跨零相等/下降，诊断only、兼容exhaustion并恢复测试tag。可达时恢复原零、多零、非邻fast/fallback、混合mutation。验模型/身份/清理、block/AVL/local、bulk算术/容量、独立旧历史恢复/继续；到达preparation失败、地址/revision/Cursor/no-op、duplicates/managed等值、foreign live/out/reentry、malformed/legacy/旧fixture。无悬空有效输入测试。真实tail/QPC/单调、足够样本/分辨，不支持平台记录缺口。结构界与统计分开，保无回收期限事实。仅诊断加强可独立交付，不能称它足以新性能Preview。

### 2.3 最终比较（仅可行候选）

原生Windows/Linux复用托管，九control：candidate/1.2/1/4.1/4.0/List/Treap/锁tag/list-range；45×9×9=3,645每主机、7,290总，含预热/七正式/诊断。保45哈希、串行轮换、优化build/source/binary/toolchain、独立语义、单调边界、诊断分开/准入/超时。不同pins清楚，不用旧计时对新control。给全条件/反例/中位/IQR/live/peak/cleanup/OM差距胜数/profile目标/资源。跨语言是组件非隔离因果，无忠实匹配C或有效control就留未知，不为标签再开port。

更新公开复现pins/metadata/原始stdout及invocation核对，不改Full CI。隔离lks0011托管，证据回Full，不带私有论文/作者。拒绝者充分预设paired screen/对抗/正确性即可，无需失败gate再全矩阵；记准确原因/反例/回退。

### 2.4 集成与正确性门槛

仅兼容且达门槛候选入Full生产，干净可读、不夹scope，研究/生产分离。冻结64/14、status/owner/Handle/callback/reentry/sync/revision/no-op/LS1/wire/LK1/recovery/C17/C++。私有capacity/index/tag可变，无额外授权不改公共/持久。仅有实质生产采用才header4.2.0-preview.1.2.1、CMake4.2.0并验读者/包/安装/合并；无tag/release。未采用保1.2源码/版本，负证据与诊断only分开。

真实严格GCC Debug/Release C17/warnings、新回归/旧CTest/API/golden/legacy/restore/例/C/C++源码安装合并/资产/MSVC x64/Win32/Linux GCC/Clang ASan+UBSan/扩san/AppleClang/适用CI，使用benchmarks ON、既有san option及verify_v4_api/validate_distribution/validate_release_assets；看真实断言/log不只数。固定原生Windows OrdWeave替candidate，十二tests、持久/checkpoint/history/adversarial recovery/external consumer，官方pin不变。更广lifecycle/DB归因属Preview2；critical失败原步修或拒，保反例不假绿。

## 3. 强制文档、证据与审查

交阶段报告、上述架构、前瞻协议、身份/raw/profile/counter/tail/saturation、独立model/失败/复现/分析至对应benchmarks/research目录。不以过期artifact唯一证据，公开许可资产/精确链接、无私文件。先实现commit，再文档绑定source和另finalPR。保旧历史。

Full实现变更同PR审改四中文指南：具体章节/调用/invariant/复杂度/tag/资源/status/分配失败/真实tests/链接/限制，四行矩阵含前1.2源/文档、新源、具体函数/节、未改及证据，Stable已接受与临时快照分开。搜修旧描述不只banner，关注capacity/tag单调/fallback/分配。交接按真实模板给前后SHA/PR/Issue/路径/结构/调用、冻结全部契约、性能资源反例、core/adapter独立判断/重测/CI/限制。按需改Full-owned changelog/API/架构/兼容/用法/集成/开发/版本/例，排根README/shared workflow/governance/Mini/Workbench。未采用仅研究准确状态，不无故重写未变生产指南。

有界自审：预编辑清单、真实源/tests、caller/contract/fixture影响、新维护者对抗不变量/复杂度、预设标准/data/source/provenance/permission/link，原Steps1–2修不加阶段。独立另验不自PASS。完整筛选后不采用不是不执行。

## 4. 权限与停止

授权仅该Full分支及其owner源码/tests/docs/research/metadata、本地worktree/固定OrdWeave替换；一个隔离lks0011 research/preview1-2-1-matrix仅benchmark workflow/evidence commit/push/run；一Full-only叠draft对1.2。报实际base/HEAD/CI。真正scope-guard仅main-target，此时用可信base查diff，不能称缺检查已过，未来集成仍需。查final Full/Mini/Workbench/其他门槛，不改workflow/protection；真阻断给命令/限制。禁止main/旧PR写、强推/rebase/retarget/merge/tag/release/资产/Latest、其他生产库/官方依赖/论文/全局/凭据、新Preview2/自动延伸。连续完成有界包不再partial screen，除真实阻断。集成/发布由owner另授权。

一份英文报告含精确起终local/remote/implementation/PR/research/run、改动清单/control、瓶颈/选择/消融/否决/反例、逐host门槛/资源/tail/零恢复/scale、真实tests/CI/log、API/format/OrdWeave及文档矩阵、限制、有据采用/不采用。每FULL-RI-001–007给Technical Debt Impact，进展非关闭，保main/pins；独立与owner后下一包仍Preview2 Steps3–5。
