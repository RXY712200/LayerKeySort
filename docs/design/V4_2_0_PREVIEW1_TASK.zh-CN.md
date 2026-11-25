# Full v4.2.0 Preview.1 — 完整实现工作包（历史中文译本）

原文：[V4_2_0_PREVIEW1_TASK.md](V4_2_0_PREVIEW1_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW1_TASK.md)。原件保留，英文执行指令为准；译本仅保存历史授权，不重启任务。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

**签发2026-11-24。范围：** FULL-RI-001/002，[固定七步/三Preview计划#27](https://github.com/RXY712200/LayerKeySort/issues/27) Steps1–2，长期工程register[#3](https://github.com/RXY712200/LayerKeySort/issues/3)。一次实现任务、一份完成报告；同会话先Step1再Step2，含普通修正、测试、文档、有界自审。不得步间因例行决策停下、索第二prompt、只交架构提案或开Preview2。目标是实质可审候选，不是论文修订/空版本号。owner已要求开始；本任务仅本次有界授权记录，非未来永久权力。

## 0. 恢复精确上下文和工作状态

仓库 `RXY712200/LayerKeySort`，仅Full；短期分支 `dev/full-v4.2.0-preview.1`，生产起点 `c108acd72b37614f4be271c9747484c2dde5eac4`，task文件另提交在其上。fetch核实际remote HEAD/task commit，实作前记完整local/remote SHA，确认task commit不动生产。优先验证既有checkout/worktree，保未知变更/其他agent工作，不reset覆盖。重核main及相关refs；base实质变化需调查，不暗纳无关实现，必要时isolated worktree。

从另供私有环境/指令完整读授权owner policy，私记实际revision、完整文件、access/discovery；URL或旧报告非已读。公共任务不让私政策成为普通外部贡献门槛。完整读#27/#3、根README/contribution、Full DEVELOPMENT/V4_FREEZE/API/architecture/compatibility/integration/validation/V4_1_0/design history/decisions、四指南/FULL_CONSUMER_HANDOFF，查真实header/source manifest/实现/tests。执行Full文档与ownership gate，不止通用lifecycle；必要核live settings/checks，本地rules非实际执行证明。

### 精确对照与研究来源

| 角色 | 来源 |
| --- | --- |
| 授权时main | `c108acd72b37614f4be271c9747484c2dde5eac4` |
| Stable不变control | v4.1.0 peeled `70d7f0ef5857b54381b863d292a213909d87459f` |
| 早期Stable | v4.0.0 peeled `80004ae732daf696f7e9192e417de0edb0b2aafa` |
| Full archive comparison/locality | `f41b7be182c2ef766ee0dec1211500f7dd762f54`，[archive](https://github.com/RXY712200/LayerKeySort/tree/f41b7be182c2ef766ee0dec1211500f7dd762f54/benchmarks/research/full-ri-20261124) |
| 经典OM | j-hui/order-maintenance `ac4b902d103beb30cee3316d20cd10701f52d15a`，archive source capsule及实际license |
| OrdWeave只读参考 | `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`，实际Full pin `70d7f0ef5857b54381b863d292a213909d87459f`；[integration archive](https://github.com/RXY712200/OrdWeave/tree/aa95b240b019c518a98bfb900ec374d24a537165/benchmarks/research/full-ri-20261124) |

验证真实source/tag objects/license/main与Stable生产等价，不凭名字。前独立审已核39个Full archive原blob、40manifest entries/path lengths；trace ZIP member hashes及clean reproduction尚待。前archive-only任务没跑新profile/lifecycle；本包承接Full archive reproduction和core-profile余项，完整lifecycle/更广OrdWeave attribution/resource integration归Preview2。不重跑无关论文实验或改论文。

已知起点：Windows/Linux各45条件tag-range median均快Full；Full/tag cell-ratio中位1.97x/2.21x，compare-heavy3.87x/5.18x，最大约5.45x。list-range在43/45 Windows、44/45 Linux快。100k/seed17/compare-heavy final representation requested：Full3,875,768bytes，两个经典wrapper均13,436,784。C/Rust API/runtime/allocator/timing差异重要。4.1历史memory改善真实且部分speed回归，但不证明全部外部gap由4.1引入。

## 目标与验收边界

恢复Full core竞争力，同时针对原创性/机制协同与成熟替代的实测劣势；七RI最终需有证据的采用理由。Preview1选/集成可行live-core，不声称lifecycle/downstream/selection已解；只改善一benchmark不够，追实质speed、合理resource、有效结构组合。可接受有理memory增长，不以零增长教条或未评估就丢旧优点。

默认兼容Full4.x：保冻结64函数/14类型API、layouts/status、ownership/lifetime/callback/reentrancy/thread、cursor/revision、strong failure、LS1/LKS4SNP1 bytes/semantics、V3 LK1 import、C17、C++ consumer/distribution。isolated branch内可实质改private结构/算法，B64/AVL/index/stable-address allocation/representation/helpers不是承诺。不未经批准改public/persisted contract，不只为benchmark加runtime dependency。维护性和一致invariants重要。若需break/scope expansion，记录取舍/migration交owner另决，同时继续独立授权事项；兼容方案选择/分支集成在授权内，main adoption/publish不在。

## Step1/7 — FULL-RI-001：架构原创性与有意义机制协同

### 1.1 恢复可复用研究基础设施

核Full archive manifest、精确内容、45trace member hashes；在可用/授权处对比raw captures/metadata/adapter/source identities。核byte lengths/SHA-256/membership/ZIP路径安全/source-license/派生或repack身份。保Windows CSV原字节和历史raw，不覆盖archive/静改receipt。新Full research目录如 `benchmarks/research/full-v4.2.0-preview.1/` 恢复standalone build/dependency staging/common trace runner/analysis/reproduction。只引用/复制必需公开授权asset，附origin/hash；不公开私论文/policy/review logs/confidential source。复用旧工具，明记CMake/source pin适配，clean isolated build，真实smoke replay Full/旧comparators并用独立expected order/results。记精确commands/toolchain/dependency/archive检查/失败。普通consumer无需装研究dependency。

### 1.2 基于源码调查替代，不用口号

追当前Full order/block/index/handle、managed search、insert/move/remove/callback publication、snapshot capture/restore对live core的依赖；辨derived ranks与relative precedence。读固定V3/V4设计和拒绝实验（live coordinate relabel/tail、theory-guided、sparse-retention slab screen、4.0→4.1 memory/performance），恢复实际patch/raw；后来的SHA包含change不等于introducing fix。

研究经典OM（Dietz–Sleator、Bender-style indirection）、practical tag/list-range、balanced/blocked sequence、相关persistence/history的primary literature和可执行实现。给精确书目/source URL、假定/保证/反例；看真实实现/license，不只abstract。评估成熟替代加合理stable-identity wrapper/traversal/managed comparator/history能做什么，分操作语义重叠与完整lifecycle等价。不称block labels/indirection/persistence或已知结构组合本身新颖；找Full特定机制协同及可证伪benefit解释，记录旧方案失败和本次不同。

### 1.3 选择有界可执行候选

至多三个初始实质不同方向，非三产品：A优化blocked AVL（重复rank ascent/common ancestor/path/hot loads/local maintenance）；B hybrid precedence（block labels/OM/indirection加compact stable occurrence，保managed search等所需index）；C替换sequence core或调查所得有据更优方案，完整语义/lifecycle。相同预算内可用有据方向替换某family。按cost model/invariants/expected effects/旧失败/complexity/license筛选；有前景不同方向建最小可跑prototype。仅一个值得实现时，其他以具体证据拒，非偏好。Step2集成有前景private change。每存活方向写representation/metadata/operations/ownership/publication-cleanup次序/worst-adversarial/complexity假定/overflow-capacity/resources/compatibility，兼顾managed/snapshot restore，不只单操作。

### Step1交付与内部转段

建持久Full架构/决策文档如 `docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.md` 及research protocol/evidence。含current/candidate图或同等精确结构、算法/pseudocode、source-contract map、prior art/license、真实prototype/rejection、未决假说。是living design非最终实现声明，Step2后改准adopted/rejected。保可复现harness/核验archive报告，测前decision protocol后直接Step2，不为例行选择交owner。

## Step2/7 — FULL-RI-002：根因测量及live-core实现

### 2.1 公平同期control与因果profile

同campaign测不变4.0/4.1、实际未改固定tag/list-range adapters、现有list/treap、候选。复用45条精确20,000-operation定义（三size1k/10k/100k、三seed、五profile）。核replay/identity/traversal/duplicate payload/相同成功操作/no-op，计等价wrapper全storage。recoverable OOM/foreign-handle/callback与共同成功benchmark分开，不给abort comparator编恢复保证。评API check/dispatch和C/Rust差异；可行找忠实有license同语言OM或port，核behavior/complexity/provenance，加为额外control非静替原。不可行则matched native/API/harness microcontrol并说明未解因果。

profile/测量包含：

- 同/跨block compare、`lks_order_compare` 两rank ascents、path/load/branch/cache、API validation/dispatch。
- explicit insert/remove/move：同块、跨nonfull/full、no-op、endpoint、short/long、source/dest occupancy；managed insert/search/remove和comparator工作。
- allocation、pointer/local-index assignments、rotations/splits/merges/redistribution、label变化、requested/retained memory、latency/tails。
- 同controls下4.0/4.1，区分owner metadata layout、snapshot等实际贡献，不假定memory优化导致所有慢。

独立sequence expected结果，surviving occurrence地址稳定；分精确logical state与同block/occupancy/topology。重建/直接验起态，扩三append形至churn/hotspot/fragmented。prod/profile分开；无hardware counters如实记。monotonic clock/可比边界/release flags/rotated order/serial independent processes，记机器/compiler/allocator/source+binary hash/resource guards/timeouts/skips/failures。不同host不能旧新对比。选中候选对照每Windows/Linux trace/control至少一warmup、七formal fresh processes，diagnostic另跑；先bounded screen，仅selected/control跑full matrix，免无谓重跑。含独立预设adversarial及out-of-tuning条件。

### 2.2 结果前预注册决策标准

看候选测量前保存protocol/table：correctness/compatibility绝对gate，分负载timing目标、resource预算、baseline noise/variation、adverse/tail条件、边界及retain/reject。规划目标例如对4.1 compare-heavy和overall median cell-ratio至少降25%/15%，是工程screen非最终优越证明。量化双host及剩OM gap。系统性实质回归（例如相关组超过10%且超实测波动）、tail blowup、sparse retention、failure/capacity须调查。memory预算在测试前预期，测requested/peak/retained及可支持RSS，实质增长须解释；10–20%非自动不可或可接受。按baseline/application给每candidate具体预算；目标不适当则结果前有据替代，非看胜负后改。不得挑seed/profile、隐藏adverse或无据普遍/显著性声明。

### 2.3 实现集成兼容live core

批准候选branch内实质改Full生产，旧controls另保。如block precedence令常见cross-block常数工作，须在split/merge/redistribution/insert/delete/rebuild中一致，不能依未检查midpoint gap。stress有限label exhaustion、单gap反复插入、relabel frequency/total writes/worst tail/counters-overflow/cleanup/OOM。对比低风险rank优化及合理screen方案；不能为compare快重引暴露live持久坐标或无界历史relabel隐患。

审changed private routine所有caller，含managed和snapshot restore/bulk。保survivor handle地址、duplicate payload identity、comparator equal stability、actual/no-op revision/cursor、valid foreign、reentry、publication前prealloc、rollback/cleanup、immutable snapshot/wire。dangling handle不作有效输入测试。保owned/borrowed/status；不加推测public API/format/DB特性。清module/invariants/comments，不无谓framework或production双core。失败prototype不意外链接distributed Full。

若无可辩改善，保失败证据，报精确limitation/最小后续决策；不能伪称成功Preview1、空版本或自动开新研究阶段。普通fix/targeted remeasure仍两步内。

### 2.4 验证实现/兼容/分发

运行真实Full gates，不仅benchmark；查expected assertion，覆盖原/新路径。isolated Debug/optimized及适用sanitizer，既有Windows MSVC x64/Win32、Linux GCC/Clang ASan/UBSan、现有CI可得macOS AppleClang、exact-head public consumers/distribution/API/golden。复用既有CI，不改shared workflow/control-plane求green。

按实装generator/compiler/platform适配并记录实际命令输出：

```sh
python tools/verify_v4_api.py
cmake -S . -B build/preview1-debug -DCMAKE_BUILD_TYPE=Debug -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build/preview1-debug --parallel --config Debug
ctest --test-dir build/preview1-debug -C Debug --output-on-failure
cmake -S . -B build/preview1-release -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build/preview1-release --parallel --config Release
ctest --test-dir build/preview1-release -C Release --output-on-failure
cmake -S . -B build/preview1-san -DCMAKE_BUILD_TYPE=Debug -DLKS_ENABLE_SANITIZERS=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build/preview1-san --parallel
ctest --test-dir build/preview1-san --output-on-failure
python tools/validate_distribution.py
python tools/validate_release_assets.py
```

用适当native MSVC generator/architecture和支持Clang sanitizer，验七public examples和真实distribution consumers；保API/golden字节，不弱assertion或刷新fixture掩break。新表示补adversarial model、reached allocation failure、resource reclamation、managed/bulk/restore，精确source/assertion引用。查source list/standalone/amalgamation/C++。可行用isolated只读origin OrdWeave临时本地Full替代做smoke/compatibility，报精确RC/current source和实际dependency pin；不称native Linux OrdWeave，完整paired lifecycle/DB attribution归Step4。真实缺测试需记impact，不能当成功。

## 强制version/architecture/documentation交付

实质选中实现metadata为 `4.2.0-preview.1`，CMake numeric4.2.0，header version/prerelease依历史；查全部version source/generator/install/package/distribution，准确改Full-owned metadata/changelog/notes，不改历史tags/releases。公开Stable仍4.1.0，不能称Preview1已发布。无可行候选不把失败experiment标升级；需break则停止adoption报version/migration。

建 `docs/V4_2_0_PREVIEW1.md` 持久stage报告及Step1架构/decision，附真实protocol/profile/independent checks/adopted-rejected/sourcepins/reproduction。避免双权威：design解释invariant/rationale，四指南解释实际实现，report记milestone/limits，Issue3为债务权威。给源码技术图/pseudocode/private-public call paths/ownership-failure，非泛说快，链prior design/research。其他research形式随仓库。

Full same-PR文档gate即使仅private性能变更也强制，读四指南并更新受影响中文全文：

1. `docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md`：真实modules/data flow/operation call paths/examples。
2. `docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md`：全public surface/status/ownership/lifetime/source mapping及明确核过不变部分。
3. `docs/IMPLEMENTATION_INTERNALS.zh-CN.md`：representation/functions/invariants/update/publication/rollback/capacity/allocation/snapshot restore。
4. `docs/IMPLEMENTATION_VALIDATION.zh-CN.md`：实际assertion/source links/commands/counts/environment-CI/benchmark/limits。

同change更新 `docs/FULL_CONSUMER_HANDOFF.md` 最新ledger：old/new精确implementation revision、changed paths/functions/structures/call chains、compatibility/performance/memory/failure、分别DB-core/adapter verdict、actual tests/unverified。以证据及本授权restart修DEVELOPMENT/handoff过时planning-only DB状态/Nov15暂停，保历史决策。其他Full-owned API/Architecture/Compatibility/Usage/Integration/Development/examples/notes按需。public README/family navigation/shared workflows归Repo，本Full-only任务不改，报将来另scope需求。保published truth，不把强制Full docs拖RC。

记录独立接受baseline：4.1source `70d7f0ef5857b54381b863d292a213909d87459f`、PR25指南实际document commit须核；本cycle原provisional snapshot起点无；新candidate implementation revision。先commit生产再在docs绑exact SHA，后docs-only commit非新algorithm；final PR HEAD另记免自引用。生产再变须重绑更新。stage report/PR四行sync matrix：每guide reviewed/updated sections或有据unchanged、files/functions/source SHA/actual validation/limits。Preview docs待独立acceptance，不能已知错误。不为新奇重写manual；RC虽重建final/history信任，不豁免当下同步。

## 证据、有界自审与停止条件

raw measurements/inputs-hash manifests/code/candidates-rejections/logs/counters/models/environment-binary-source/analysis置新Full research目录或固定专用research branches，永久引用。保原captures/历史branches，不泄私。Git太大则可复现durable authorized archive+完整hash/ref，不只短期CI artifact。protocol改需前瞻记录、保旧失败。合理复用已验证infra/results，新performance须同期controls。记RI001/002发现/gaps，RI003–007附带影响仅证据更新非完成步骤。

先contract/evidence/critical assertions checklist，实作测，再读source/docs/assertions/task修真实问题；聚焦adversarial/new-maintainer审representation/fairness，核scope/provenance/contracts/docs。有界活动非无限fixed-point；普通failure两步内修。未解correctness/security/compatibility blocker或无viable architecture则有据停可审状态，不扩计划。保final covered/uncovered/repairs checklist，不自宣独立PASS。

## 当前操作授权与交付

- **读/研究：** 列出的Full/archive、合法public comparator/primary sources、OrdWeave只读reference；无关private/global config不动。
- **edit/test：** 指定Full branch private production、Full-owned build/version/tests/research/强制docs，可本地isolated prototypes/worktrees；不改未授权public contract/Mini/Workbench/governance/control-plane/Repo shared。
- **commit/push：** 指定Full branch coherent implementation/evidence/docs提交，保历史，不force/direct main。另essential research remote branch不在授权，报需求；默认本branch+temporary local worktree。
- **PR：** 一Full-only draft against main审完整两步，实际inventory/source/head/doc matrix/consumer ledger/exacthead CI；跑等既有required Full/Mini/Workbench/scope-guard适用context，分skip/missing/fail，不能弱保护/exception/admin bypass/模拟pass。Repo需要另列不改。
- **merge不授权；tag/Release/publish assets/changeLatest不授权。** 可candidate准备验证；真实preview tag/Prerelease/exact target待独立accept后owner另决，不动旧tag/assets。
- **其他repos：** 只读；本地isolated OrdWeave test/dependency替代可，不remotewrite/production pin/release/private research-manuscript。无global/credential/cloud/settings变更。
- **停点：** 两步、实际integrated候选或精确failed selection、全部证据/docs、授权commit/push/reviewPR、完整英文报告后停；不开Preview2/RC。

## 最终英文报告（两步共一份）

含精确start/end local/remote SHA/base/branch，implementation source SHA、docs SHA、final review HEAD分开，worktree、真实PR/checks/runs/headSha、全file inventory、task/compatibility scope、候选架构/理由/实测、operation rootcause/uncertainty、4.0/4.1/OM/candidate前后resource/tail/adverse、raw/protocol/repro、actual tests/platform/assertions/unexecuted、package/version、四guide matrix、OrdWeave adapter/core impact。区分历史/新执行/独立acceptance，未完成/真实blocker实报，不编clean/green/published。**Technical Debt Impact：** FULL-RI001–007各disposition/evidence、新/派生问题、真正introducing commit与later diagnostics、partial/unresolved、不假关闭。独立技术验收、main集成、release、nextstage均另决。
