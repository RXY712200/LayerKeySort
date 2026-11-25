# Full v4.2.0 Preview.1.2：有界经典顺序维护竞争力扩展（历史任务译本）

原文：[V4_2_0_PREVIEW1_2_TASK.md](V4_2_0_PREVIEW1_2_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW1_2_TASK.md)。原件保留，英文执行权威；本译本不重启历史授权。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

签发2026-11-24，owner批准[计划#27](https://github.com/RXY712200/LayerKeySort/issues/27)Steps1–2/FULL-RI-001/002一次扩展，七项在[#3](https://github.com/RXY712200/LayerKeySort/issues/3)。一次连续Codex/英文报告，非Step8/重复Preview1/Preview2，无自动1.3。调查且证据支持时缩小经典OM优势、保Full工程价值，不要求各算法/负载/指标都胜；有据拒激进方案是完成研究，不为版本制造发布/弱正确性。

## 0. 权威、状态、保护

授权私下完整读现行WORKFLOW/RELEASE_POLICY、模板及本地约束，私记policy revision/可用性；读#27/#3、原Preview1任务/最终报告架构交接、当前API/freeze/development/ownership/validation/四指南和实际source/tests，遵Full同PR文档更严格gate。不公开政策/凭据/论文/无关评审。

| 角色 | 精确revision |
| --- | --- |
| 签发main | c108acd72b37614f4be271c9747484c2dde5eac4 |
| 发布4.0.0 | 80004ae732daf696f7e9192e417de0edb0b2aafa |
| Stable4.1.0 | 70d7f0ef5857b54381b863d292a213909d87459f |
| 已接受Preview1生产 | 22544cf283694dd27cef203caba9cf1838845947 |
| Preview1主要证据/CI | 2d14510a8aae131bcd35679b940ab3d3b36fb8fa |
| 文档修正checkpoint/扩展base | 3a2b3eabd0153531e1064457519b40deb4af9842 |
| 单调托管研究源 | lks0011 42987c5eb8eaa85cfe791d8efc65865951999dbc，research/preview1-final-matrix |
| OrdWeave/官方Full | b12a007c50c2f9efcbee500a14d3cf09dcc74f10 / 70d7f0ef5857b54381b863d292a213909d87459f |
| 实用经典OM | j-hui/order-maintenance ac4b902d103beb30cee3316d20cd10701f52d15a |

dev/full-v4.2.0-preview.1.2从文档修正点创建再加任务commit。Fetch记录initial SHA/local/remote/worktree，核refs/生产/task-only diff。原draft#28独立保留，不改其branch/rebase/delete/merge/publish。扩展隔离，owner另决Preview1集成；commit非Release，签发两拟议4.2 tag不存在，Stable4.1，tag查询非授权。Preview1为已验fallback，结果/接受不依赖本实验。

用短期叠加Full candidate及一个draft对dev/full-v4.2.0-preview.1；#28合后retarget由owner/独立决定，不能静默rebase。叠base不触checks则如实报并准备精确head验证，不改governance。无main/publication，不reset未知，用隔离worktree/输出。

## 1. 起点证据和决定

Preview1将高度祖先compare（不用两绝对rank）与普通插删移suffix local合并，结构成员变化保全修，无新字段，跨块最坏log。90primary均胜Stable，请求live/peak90/90相同。托管45×7×9双主机共5,670（4,410正式/630warm/630diag），独立630cells核中位/极值/IQR/存储/比。Linux主要CLOCK_MONOTONIC，旧wall保历史。Full45360683557六配置各39/39含扩san/distribution，OrdWeave12/12。CI后仅两说明修正，finalhead另验。

| Preview1/control中位 | Windows | Linux |
| --- | ---: | ---: |
| Stable4.1 | .69786 | .70889 |
| Stable4.1 compare-heavy | .57230 | .56894 |
| tag-range OM | 1.28146 | 1.36005 |
| list-range OM | 1.23704 | 1.16922 |
| tag-range OM compare-heavy | 3.07133 | 2.62921 |

对tag胜9/45及11/45，list10/45及15/45；最差tag3.576x。100k/seed17/compare-heavy Preview1 retained3,875,768对OM13,436,784，peak4,253,752对13,437,504，是请求非RSS/普遍内存优。比较剩余gap最佳目标，非组件整体不竞争证明；C/Rust native/语言/编译器/allocator/wrapper仍混合。旧中点原型1,057耗尽后AVL，非成熟动态OM实质评估，保否决历史。[阶段报告](https://github.com/RXY712200/LayerKeySort/blob/2d14510a8aae131bcd35679b940ab3d3b36fb8fa/docs/V4_2_0_PREVIEW1.md)及final-matrix。

## Step1/7扩展：机制研究与有界prototype

### A. 复用已验环境

复用公开final-matrix、j-hui/notice、45不可变轨迹、native/API probe、diagnostics、独立oracle、prod/diag编译及旧授权lks0011。核pins/归档/构建/trace hash/边界，重读实际码/V3 tail/V4选择/4.1内存/1标签原型及proof。不为历史重跑无关。新capture cleanstage/锁deps/不可变control。Windows/nativeLinux已具Rust/GCC，用既有路径，本地无Rust非阻断，不全局装/改FullCI/论文。必要仅一个lks0011 research/preview1-2-matrix，从保留head，限captureworkflow/code，本包授权；保原研究/论文，不复制私内容公开。

### B. 成熟OM最高优先

读Dietz–Sleator、Bender2002、锁定tag/list代码及需时可信现代/间接实现，比较最坏/摊销、relabel流量、update限制、word/overflow、稳定身份/query/任意相对insert/delete/merge/split；核实代码/许可/假设非仅渐进描述。分析仅维持block先后、既有AVL供managed/topology/bulk的协同，非新OM理论宣称。研究既有tag/range只施块层、分组/间接/受控localrelabel，及哪些AVL metadata不再要；对合理独立index/更好机制，不为跑分引大runtime。衍生代码许可/归属兼容，适宜优先独立实现并说明。

### C. 范围与可证伪方案

最多三实质家族含旧AVLcontrol，至多两新：真实有限label且集中插删持续维护的block OM，不是永久禁用唯一fast路；有据替代如去unused-rank降低index/mutation或不同廉价precedence，可与选定动态合并、消融归因，非三完整库；Preview1不变fallback。

不为新颖强造算法，拒家族需primary/invariant-cost论证或可执行失败。可信动态维护需原型，除可证明契约/不变量不可行。各方案明确块/间接/tag对象、界/capacity、分配/relabel/清理/发布、init/rotate/split/merge/redistribute/crossmove/remove/bulk/managed/noop/revision/身份/owner/snapshot/wire。Fastpath有效及fallback/rebuild转换不能无界不安全/永久静默退化。坐标非ID，不改LS1/wire。subtree_blocks全读者/checker/restore搜后实测才删。保持C17/冻结。

建架构文档V4_2_0_PREVIEW1_2_ARCHITECTURE.md，精确diagram/pseudocode、invariant/proof、call/op、复杂度/实证限制、选择/反例、priorart/历史；另新研究目录前瞻协议。直接Step2，无普通选择中途owner交接。

## Step2/7扩展：实现、测量、决定

### A. 测前规则

新计时前commit协议：固定trace/priority/threshold/resource-tail/选择/stop。对已接受Preview1非仅弱4.1。默认双主机compare-heavy至少20%elapsed下降、overall8%，或前瞻有理强取舍如目标明显改善、其他持平/额外内存tail有界。目标不强采用，精确exception先定，不事后改。量对两OM gap/wins，不要求普遍win/等memory，用baseline波动及真实目标优先。

拟议目标cell最多约15%额外请求live/peak可为显著收益合理，更大需结果前资源论证及owner采用前审，不自动拒研究。单设tail/最坏工作预算，保反例、系统回归、tail大增；不隐>10%非目标系统慢（超噪声）、无界pause或不可接受稀疏保留，除明确有据决定。正确/兼容/失败绝对；保先前对4.1整体15%/compare25%参考，勿丢已接受收益。

### B. 筛选、消融、对抗

先有界子集/预定独立状态，仅可行final及controls全matrix。公平外部API边界、diag/time分开，测fast/maintenance/mix，拒者亦留。窄gap超过1,057、持续hotspot/首尾、fragment/churn、变占用split/merge/redistribute、逆/long、层次compare、restore/bulk、shrink/regrow。记耗尽/relabel/rebuild/fallback/影响块字节、写max/分布、分辨支持p50/p95/p99/p99.9/max，计时floor非精度。比较-only、update/move、managed/search、分配/cache/index/资源，勿仅转移巨成本。实际演进状态独立序/全部live身份，非只append；新可失败preparation到达注入，saturation/overflow/failsafe不破Handle/revision/history；消融分metadata/update。

### C. 最终可行候选campaign

双原生host同时代：candidate、原Preview1、4.1、4.0、List/Treap、锁tag/list，distinct共八（七或八描述按实际）。45冻结20,000 op、七formal fresh串行+warm+diag，45×8×9=3,240每host；另预定独立对抗/holdout。轮换、匹配优化、QPC/CLOCK_MONOTONIC/RustInstant，source/compile/link/binary/env凭据、guard/admission/timeout/stdout/stderr/failed/skipped全留，旧timing不能作同时代。live/peak/retained、alloc/free/cleanup、真实可用RSS，不从0placeholder推RSS。按实际风险大/churnscreen，广lifecycle/DB留Preview2。五C语义fixture及Rustgate，独立order/identity/traversal/equal/no-op；区分failure/comparator/wrapper/memory/native。

可行时忠实licensed C/C17经典或严格匹配C-facing归因，不开无界port；预算不能合理移植则匹配C API/precedence/mutation并留语言/wrapper未知，仍比锁定原OM。C胜Rust是组件非抽象更优证明，无cherrypick，保全部源/好坏证据。

无viable：足够同时代pairedscreen定原因、完整负选择/design/protocol，生产版本保Preview1、建议下一Preview2，不为绝对契约失败者跑巨final或空prerelease。Promising但gate未决精确报非默采用，不再扩展。

### D. 真实兼容候选与验证

Viable者清洁Full-owned生产，保64函数/14类型、C17/C++、Handle/callback/reentry/sync/status/Cursor/revision/原子、LS1/wire/LK1。私有可大变，公共/持久须另授权。原型不进生产；仅采用分支改4.2.0-preview.1.2、CMake4.2.0、按惯例头并验包/version/currenttruth，未采用不升。合法SemVer名非tag/release授权，不为宣传速度改码。

新定向regression维护/relabel/repair、身份/等值、oracle/output/failure/bulk/restore/cleanup，查真实assert非计数。严格Debug/Release C17、MSVC x64/Win32、GCC/Clang、ASan/UBSan/扩san、AppleClang原CI、API/wire/golden/olddata/例/C/C++源码安装合并/assets。用benchmarks ON、适配generator/config、verify_v4_api.py/validate_distribution.py/validate_release_assets.py、适用Clang sanitizers ON，不弱fixture/tests。精确Full/Mini/Workbench/scope/governance。原生Windows隔离OrdWeave固定/候选，十二上游或实际更新suite、身份/checkpoint/history/recovery，officialpin不变，精确命令/未执行范围；无广下游优化。

## 强制产物、文档及有界审查

阶段V4_2_0_PREVIEW1_2.md给前后SHA/候选或负决定/理由/全部测量条件/CI/限制/七RI；对应研究目录含protocol/runnable或固定recipe/raw/source/hash/compile/scripts/分析/失败/反例、source/diag区别。Full耐久归档，私host仅公开许可授权技术，不带私物；复用旧结构不无故重建。记control及researchworkflow，旧证据不改。

生产/契约变同PR审四中文指南：Walkthrough真实表示/调用；API compare/identity/owner/failure/callback/source/明确不变；Internals label/index/invariant/维护/复杂度/内存/发布/bulk/tests；Validation真实assert/命令/CI/source/limits。四行旧新revision/节/函数/tests/不变未知。交接按模板SHA/PR/Issue/文件函数/前后、全部错误owner寿命callbackOOMsyncLS1wire恢复build、性能内存反例、core/adapter各判断、下游测试/CI/动作。其他受影响Full-owned API/架构/用法/集成/兼容/开发/changelog/version，不改root/navigation/sharedworkflow/ownership/governance。

不复制全手册，区分last accepted Stable、修正Preview1、当前source及临时快照，不能明知冲突。跨doc/example搜旧rank/label/maintenance，非仅intro。先实现commit，文档绑定真source另finalPR；research-only拒则保正确生产文档、记不变，不造source基线。

同session有界自审：清单/实际实现tests/改函数callercontractdocs/维护复杂度对抗/范围许可provenance/raw/文档/精确CI，原两步修；达门槛或有据拒完即停，不无限loop/自PASS，一份报告后独立审。

## 授权与停止

仅dev/full-v4.2.0-preview.1.2 Full-owned源码/tests/版本/docs/新research、本地隔离OrdWeave替换；一个上述lks0011隔离branch窄workflow/code/run/archive；Full commit/push一个draftstackPR记base/head，普通修复复用。无其他生产remote/官方pin/论文/私内容公开/任意workflow-settings/global/FullCI/Mini/Workbench改。无merge28或新PR、mainpush/force/tag/Release/资产/旧tag-Latest/Preview2。准备非发布，owner精确目标另授权。拒也保Preview1，完成有界调查和实跑证据/已验candidate或负决定，不停在未探原计划的初筛。

一英文双步报告：branch/base/start/final local/remote/source、PR/CI/research/run/main/pins、清单/范围、hypothesis/proof/limits/前瞻gate、逐host对Preview1/OM中位/wins/反例/memory/tail/relabel、同语言状态、实际命令/oracle/regression/platform/OrdWeave、source/docs/version/四指南交接、采用/拒理由、真阻断和每FULL-RI-001–007影响。范围进展非广研究关闭，不造执行/发布；返owner/独立，下一Preview2 Steps3–5。
