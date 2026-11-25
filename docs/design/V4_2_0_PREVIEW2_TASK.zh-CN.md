# Full v4.2.0 Preview.2 — 完整生命周期、下游集成与资源工程（历史中文译本）

原文：[V4_2_0_PREVIEW2_TASK.md](V4_2_0_PREVIEW2_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW2_TASK.md)。原件保留；译本为历史说明，英文执行指令权威，不重启授权。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

**签发2026-11-24。** owner授权[计划#27](https://github.com/RXY712200/LayerKeySort/issues/27)下一主包：Step3/7 FULL-RI-003、Step4/7 FULL-RI-004、Step5/7 FULL-RI-005。同一连续Codex任务执行三步，含普通修正/证据/测试/文档，一份英文报告。不得步间为例行决定停、建新规划阶段或开始Preview3。目标是超越快速mutable order的完整lifecycle、真实integration、resource价值，非论文表面/功能数量/预定算法。保selected live core，不开Preview1.2.2或发布1.2.1；运行基线为Preview1.2，加PR30已核diagnostic-only提升及研究证据。

## 0. 权威、状态与保留

完整读现行授权Development-Playbook WORKFLOW.md/RELEASE_POLICY.md、英文task/completion模板、repo instructions及更严Full要求。签发policy main为ba0c6fb6b35650608b857adba4ad0f672d65cac7，核实时ref并私记read status。读#27、[#3](https://github.com/RXY712200/LayerKeySort/issues/3)、[永久live-core决策索引](V4_2_0_LIVE_CORE_RESEARCH_HISTORY.md)、前stage/design/consumer、header/development/freeze/ownership/validation/API/architecture/compatibility/四指南；OrdWeave读真实pin/source/HANDOFF/DEVELOPMENT/debt/API/persistence/checkpoint-history，勿把其TD当Full defect。设计前读source/tests；私policy/论文/reviewer材料不入public。

| 角色 | 精确起始revision |
| --- | --- |
| main签发未变 | c108acd72b37614f4be271c9747484c2dde5eac4 |
| Stable4.0 | 80004ae732daf696f7e9192e417de0edb0b2aafa |
| Stable4.1 | 70d7f0ef5857b54381b863d292a213909d87459f |
| Preview1运行source | 22544cf283694dd27cef203caba9cf1838845947 |
| accepted Preview1.2，primary control | 61e9290ec5ba47b9092bcb8ae7d61f13104ea839 |
| 保PR28/29 HEAD | 3a2b3eabd0153531e1064457519b40deb4af9842 / 83367fd9b3b7297db1642c2eea6159bf395ef286 |
| diagnostic增强 | c977b0fd14a501d769b9783bfdcc61266bd1b5ae |
| 保PR30/本branch base | 07b739de1841256ed89236b4096d130f88206956 |
| 永久决策记录 | 779157f5604480b5f14c866e736567d7a0d9811c |
| OrdWeave参考/官方Full pin | b12a007c50c2f9efcbee500a14d3cf09dcc74f10 / 70d7f0ef5857b54381b863d292a213909d87459f |
| 原Full/DB archive | f41b7be182c2ef766ee0dec1211500f7dd762f54 / aa95b240b019c518a98bfb900ec374d24a537165 |
| 上次private research source | lks0011 research/preview1-2-1-matrix，5c5540406af5a465ceaf12f8ea3b410f5566d81c |
| classical OM | j-hui/order-maintenance ac4b902d103beb30cee3316d20cd10701f52d15a |

`dev/full-v4.2.0-preview.2` 起于精确PR30 HEAD，上有永久研究决策及本task两docs-only签发commit。fetch核真实local/remote/clean或刻意isolated worktree、source、branch/PR和两文件。stacked base有意，#28–30保持open及research/tests，非release；不改/rebase/merge/retarget/delete/force旧分支PR。PR30未合时建一draft Preview2 targeting `dev/full-v4.2.0-preview.1.2.1`；若前集成另授权已完成，报状态并取精确base决定，非静改history。Stable4.1，不授权main merge/tag/release；有实质可行production改善才metadata4.2.0-preview.2，签发时tag不存在。子问题有实测等价证据的no-change有效；主包应有meaningful integrated progress或有据negative，非编version。

继承证据：精确Preview1.2在90primary live cells全胜4.1，median ratios Windows .641694/Linux .572017；tag-range .979252/.917306，list-range .983687/.880425，较Preview1最大requested peak增.60%。仅native成功component非全lifecycle优越。保bounded64-bit tags/B64/AVL fallback/suffix updates/nonzero monotonic checker。B32/B128/direct-transfer已前瞻ceiling拒，不重探索/为新颖改capacity；真新scale/lifecycle bottleneck才可有据限定复议。不设零memory增长教条，保stable identity/immutable association-history/wire-restore-error。

## 1. 共用protocol与效率

复用精确public archives、trace/replay builder、strict C17/model/failpoint、4.0/4.1 scale/capture、成功双host recipe。新比较不得复制旧测量，changed behavior用新精确control及同logical inputs。核source/input/license/toolchain-flags/executable/raw hashes；clean staging和one-command reproduction。Windows QPC、Linux CLOCK_MONOTONIC、Rust Instant（适用）边界齐；serial/rotated、warmup分、fresh process，critical cells七formal samples；slow large end-to-end/scale可前瞻有据减样并报uncertainty；diagnostic别于timing，保每raw/dispersion/adverse。分setup/callback/resolver/build/operations/cleanup，不减unmatched totals作精确因果。RSS/working set仅支持且sampled时给，区分requested live/peak/retained、allocator traffic/process memory，unsupported记missing。prospective每case memory/time estimate、guard/timeout/skip reason，不耗尽机器或把skip当pass。

结果前commit `benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.md`：questions/semantics/baseline pins/factors-cells/generator/oracles/screen/未用validation/timing/budgets/pass-fail-no-adoption/priorities/contribution。bounded fractional/factorial+anchor+one-factor，非Cartesian爆炸。含sparse/retention-heavy/真实application、多初态/独立seeds。对baseline noise量化实益：默认受影响lifecycle/resource/application phase elapsed降>=15%或material workload requested peak/retained降>=15%，至少一个预设target class整体end-to-end/resource有益；非各phase必胜或可忽正确性。依baseline sensitivity预设分负载预算，重大总benefit可有理memory增；默认调查反复>10%non-target end-to-end/live slowdown或超variance的tail/retention回归，保live收益，结果后不放gate。预设exceptions/真negative，保4.1/精确1.2，历史memory归因需时加4.0。unchanged facts复用旧据。共runner/manifests，先screen再integrated final，不每小改无谓full rerun。

授权隔离环境：Full指定branch；private lks0011 `research/preview2-lifecycle-matrix` 从上述last research ref，仅窄workflow/benchmark/source/results；OrdWeave `research/full-preview2-20261124` 从精确native pin（必要另读旧archive），仅可复现instrumentation/prototype/results。真实OrdWeave native Windows，不暗示Linux。无production remote/pin promotion/manuscript。research branches复用hosted，不改Full official CI/governance。Full证据归Full，application归OrdWeave，原件适当保private；不copy confidential public。仅这branches和一Full draft，不其他PR/release。disposable DB、分staging、精确local substitute，不production data/global config。

## Step3/7 — FULL-RI-003：live/history全生命周期与成本转移

### 3.1 实际语义与等价组合baseline

追selected source：`src/snapshot.c` 的 lks_snapshot_new/snap_append/lks_snapshot_capture_sequence、marker retain/capture publication；`src/lks_snapshot_internal.h` 的累计association ends/header-rows-namespace coallocation/独立growable associations；`src/snapshot_key.c`、`src/snapshot_wire.c`、`src/snapshot_restore.c` 全resolve到临时items后bulk、`src/order_bulk.c`、Group/Batch capture、managed callback、revision/currentness。明确copy associations/namespace before publish，caller payload可变/毁，source毁后snapshot独立，duplicate occurrence/ordinal/namespace-domain keys、callback invocation/failure、marker ref lifetime、immutable borrowed views、caller buffer/old format。resolver序列/副作用/发布顺序重要，不能假定可重排或插入新的allocation failure，查真实契约测试。private storage与persisted format分开，当前capture O(N+A)另namespace/callback，完整storage/restore前提不能只渐进缩写。

至少一实际合理成熟**组合替代**有共同成功lifecycle语义，非raw OM对Full snapshot。优先固定practical OM adapter+stable occurrence wrapper/traversal+独立owned flat rows/association bytes/namespace-ordinal key/encode-decode/resolver/fresh restoration。可行C ordered tree/list/blocked sequence+flat history作matched language，按credible use/license选。baseline可合理高效layout/preallocate，但同输入信息；不故意加其不承诺Full checks，亦不给callback data先验。不用sleep/虚构overhead。实际实现source/license/domain/association/traversal/stable occurrences/edit/query/capture/retain/export/deserialize/restore/continued edit，以及场景所需managed。exact wire可合理实现就核，否则logical payload对齐、disclose format/size/guarantee。common-success与替代缺OOM/marker-currentness/foreign-handle/failure分开，不无据称安全。双方同inputs和独立flat sequence+historical association oracle。

### 3.2 全lifecycle，非capture孤立

预设trace含mutable/query、namespace-association capture、可选key format/compare、多snapshot跨mutation保留、caller-buffer serialize（I/O另）、deserialize/resolve-restore/continued edit、不同release序、source先destroy再history用。N1k/10k/100k及guarded larger anchor；低/高/no-capture cadence；retained1/2/8/16可行时；association empty/small/variable/wide如0/8/32/256/4096 bytes真实mix；namespace small/history-wide包括guard允许4096；mutation fraction/locality/churn、重复payload、same item/revision不同association。无需全交叉，明controlled comparison/exclusions。适用加Group/Batch/managed capture及真正history use，不纯复制。可instrument时callback/resolver另计；生成bytes和borrowed lifetime双方同。记count/size distribution、per-phase、association realloc calls/bytes、transient peak/retained/storage-sharing/wire/restore scratch/total-crossover/cleanup/tails。适用对4.1 history变化，新matched controls和旧数据保留；判断成本是否转到capture/query/serialize/recovery。

### 3.3 筛实改进，有据集成或保留

依profile至多三机制family，非必须改：

- **直接write/serialize/format和buffer管理：** 保caller-buffer及exact bytes，减少冗余walk/copy/metadata/allocation-realloc/capacity-slack，研究immutable layout。4.1 trim为取舍，wide/variable/retention-heavy先测；不能假定callback可size两次或borrowed data复用，invocation count/order/content会变。
- **sharing/chunks/dedup/incremental-COW-persistent：** 仅实测重复copy主导且独立性可证时。source revision不足保证association/namespace不变，caller callback/payload可同revision变。不借mutable caller memory，不全局cache破snapshot views；需独立byte/content/ownership等价或证明、release序/ref-capacity/concurrency、无hidden unbounded cache/mandatory dependency。测locality/serialization及unique-unshareable内容。不加推测public delta API/format/history特性。
- **resolve/restore scratch/bulk：** 评可免temp/allocation phase，但不变callback顺序/副作用/strong failure/output publication。当前resolve-first N-pointer scratch，不可交错而改failure语义；证明等价，否则保成本。历史snapshot独立live topology，tags不persist。

每promising family实际prototype和before/after ablation，记拒因/结果；非每phase必赢，要整合收益、清成本、无break。独立tests：capture后source item变/毁、无revision变更却namespace/association变化、duplicate/empty bytes、多destroy序、deserialize/restore继续、marker/currentness；malformed/old wire/golden、callback/resolver errors/reached OOM/newpoints、overflow-size/ownership-ref/cleanup。审external sync，不加unsolicited locks。Step3架构含before-after/pseudocode/invariant-ownership-failure/source/experiment。无安全子路优化则记实测保留。验收实际等价lifecycle/phase-resource-total、验证改进或有据保留、不弱format/lifetime/callback/failure；直接Step4。

## Step4/7 — FULL-RI-004：真实下游价值

### 4.1 native应用证据及配对

读OrdWeave `src/lks_adapter.c`、`src/ordweave.c` stage_occurrences/publish_occurrences、checkpoint/WAL/history/tests。补archive standalone reproduction：授权文件/trace SHA/member、精确native source/Full pin、build/runner、独立fixture/result。V0.5已有27 generated DB traces（3N×3seeds×3profiles，1,000ops，memory/persistent，含warmup324executions；Full insert37/remove34 reached OOM）仅起点非field telemetry/穷尽failure。用archive真实source。A精确未改OrdWeave b12a007+官方Full4.1；B同OrdWeave+accepted1.2；C同OrdWeave+Step3/5集成。disposable native Windows DB，source/pin/ABI/build hashes，官方dep与substitute分录。原27trace memory/persistent，匹配status/identity/order/revision/checkpoint/history/reopen/sampling，新增guard允许大状态。保旧behavior/generated/native data并诚实标generated。只为成本隔离按需typed records/repeated refs/multi collections-IDs/其他真实ops；独立model、persistent reopen/history/recovery/source/fault。精确timing scope/workload/IO/disk/cache/environment/uncertainty，不称human trace。

### 4.2 真bottleneck归因及可复用API gap

隔离OrdWeave variant instrument，非official：分Full操作/temp allocation、DB全局occurrence-index rebuild、adapter/comparator、WAL/FlushFileBuffers/IO、checkpoint/history、其他index/core/maintenance。不能减无关总耗时；matched timers/counters另于prod，防nested/doublecount，报residual。TD001 stage_occurrences建survivor alternate ManagedOrder再collection变后publish，O(N log N)/O(N)temp和prepare-before-publish属DB非Full defect。TD002 repeated semantic replay/retained WAL scan为保正确性debt（TD101历史security/correctness fix），不可删validation求快。TD003 whole-checkpoint temp/256MiB bound、TD004 copy backup、TD005 per-hash provider、TD006 interruption cleanup在到达时归因，别混DB/Full limit。保source/debt classification，confirmed/new issues另报，不无关DB优化让Full好看。

有TD001实测支持时查library facilities/internal可降integration负担：managed init、既有API、batch/prepare-commit/staged publication。先查现API足否、跨OrdWeave复用语义、revision exhaustion/multi-structure atomicity、transaction/persistent IDs/recovery归属；有据才research prototype。**不加未授权public API/break/Full DB-WAL engine，不改official OrdWeave dep/source。** 真general gap记精确proposal/evidence，非必须未经决策实现。adapter/core局部prototype为research，独立于same DB+dep swap比较，别把DB改进归Full；测failure/capacity/rollback/model，不假等价。可行且有益找另一真实ordering use或独立realistic semantic trace，不造新产品/field observation；不可得明scope/why。验收native真实paired end-to-end、defensible attribution、benefit或bounded lack、source integration建议、core/adapter分别影响，无未授权production。证据分归对应branch/pins，继续Step5。

## Step5/7 — FULL-RI-005：规模/资源与集成候选

### 5.1 实际接受表示的规模

复用V4 scale/4.1 memory/保core research，测accepted1.2与integrated在10k/100k/1m guard可行处；3m/10m仅decision-relevant且安全admitted，不承诺全尺度。append/build/lookup/traversal、long/hotspot moves、sustained churn+capture、restore shapes、多cadence/association retained snapshots、destroy-release-shrink-regrow。**million-to-1% sparse survival**（1m不可则明最大admitted），cluster/scatter多arrangements、多cycle、全部survivor stable地址、shrink/release requested-retained。历史64-slot slab retention为具体拒绝约束，不等于pooling不可能；不为小bench引dead slots/cache bloat。计allocation traffic/count、built/live/peak/retained/occupancy-slack/actual block-handle、tag fallback/repairwrites/long zero clusters、scratch/association/wire、可支持RSS/peak、CPU/tails/cleanup。heldout independent skew/concentrated tag repair/delete-regrowth；B64/checker保留，长程tag recovery是经验非通用保证。按需对精确control/历史pin。

### 5.2 优化实测集成资源，非废方案

依Step3/4择metadata/capacity/association/ownership-lifetime/allocation/restore scratch/transient overlap，量化跨phase。resident allocator仅真实profile和lifetime-safe sparse reclaim支持才复议；无新机制不重B32/B128/direct-transfer/rank ablation。新allocator/representation须stable Handles/recoverable OOM/sparse survivor/旧语义/sync/cleanup证明。snapshot独立source/payload，不未经决策改format/API。量化time-memory非零增长教条，affected path独立oracle/regression/fault，对peak/retained/total。每material concern分类improved/justified retained/regressed/unresolved，guard skip未验证。

### 5.3 最终集成验证和决定

重跑affected Step3/4对精确最终production，非早variant拼表。至少representative full lifecycle target/retention-heavy/callback-variable/no-capture live/high-scale-churn/persistent recovery。完整45冻结live replay对精确1.2及4.1、双host、七formal fresh+warmup+diagnostic、同source/input。classical adapters纳相关当前lifecycle，unchanged历史OM明pin复用，无需无关大矩阵；live core变或作general comparison则补相关full同期controls。历史time不冒充新测；新diagnostics/旧API-format-correctness/adverse保。报live gains保否、lifecycle/application总益是否抵成本、target/crossover。Step7最终selection非推迟Step3–5借口。验收有据兼容integrated product、真实total/resource/scale、bounded attribution、保留成本、source/compatibility/consumer/guide matrix。子路保旧明说；全stage无viable production改善交negative，不造prerelease；证据支持时应有实质实现/version。

## 2. 强制工程、架构与验证

建 `docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.md`：starting source/lifecycle-downstream cost map/机制/design-invariants-ownership-operations/complexity/memory-callback-failure/experiments/rejected/source mapping；`docs/V4_2_0_PREVIEW2.md`：Steps3–5决定/evidence/measurements/limits/七RI。`benchmarks/research/full-v4.2.0-preview.2/` 含protocol/pins-license/clean staging-replay/raw receipts-analysis/models/phase-profile-allocation-tail/reject-fail-skip/commands-env-manifests。OrdWeave对应exact revisions链接，不能仅expiring CI artifact。保旧archive/design/adverse，不重写旧research支持新结论。implementation introducing commit与test/docs/evidence/finalHEAD分开，无自指SHA。version有理才4.2.0-preview.2，header prerelease/CMake4.2.0/真实package readers/local validators，无tag/release，Stable不变。

兼容冻结64函数/14类型-layout-status、caller payload、stable occurrence、equal stability/no-op-revision-cursor/callback-reentry-external sync/recoverable atomicity/snapshot ownership-view/marker/currentness/LS1-LKS4SNP1/V3 LK1-old fixtures/restore fresh handles/C17-C++ source-install-amalgamation。break想法只experiment，明migration/version/owner决定。本包不实施breaking product。贯穿测试非仅结尾：独立oracle、reachedalloc failure含capture/marker/serialize/restore/source destruction、callback/resolver、malformed-legacy、retained data/order、tag/invariants/version-overflow-capacity；golden不静刷新，不dangling作有效。核assertion/cleanup和不变条件。strict GCC Debug/Release `-Wall -Wextra -Wpedantic -Werror`，native MSVC x64/Win32、Linux GCC/Clang ASan/UBSan、AppleClang/extended sanitizer、实际Full CTest（基线39非未来固定）、七examples、API/oldwire、C/C++ source/install/amalgamation/distribution/assets。`tools/verify_v4_api.py`、`tools/validate_distribution.py`、`tools/validate_release_assets.py`、CMake `LKS_BUILD_BENCHMARKS=ON`、适用 `LKS_ENABLE_SANITIZERS=ON` 按平台适配。exact final source/head Full/Mini/Workbench适用checks，不weak。native Windows isolated OrdWeave final candidate suite（当时12）+paired，不official pin改。local非independent acceptance，精确logs/source归档。

**同PR强制文档：** 全读四中文指南并更新所有受影响解释/来源：IMPLEMENTATION_WALKTHROUGH.zh-CN.md、IMPLEMENTATION_API_REFERENCE.zh-CN.md、IMPLEMENTATION_INTERNALS.zh-CN.md、IMPLEMENTATION_VALIDATION.zh-CN.md。区分accepted Stable source70d7f0.../guide merge `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`，prior provisional1.2 source61e9290...+diagnosticc977b0f.../guidehead07b739d...及当前精确新source。四行matrix sections/justified unchanged/functions-paths/SHA/assertions-tests-platform-evidence-limits。全文查row layout/association ownership/marker/callback count-order/scratch/allocation/serialization-restore/version，不只append开篇。consumer handoff同PR：old/new精确SHA、paths/functions/call chains、contract/ownership/errors/OOM/Handle/LS1/wire/restore/build、time-resource-adverse、OrdWeave真实结果，DB core/adapter分别none/retest/adapter change/possible core change/unknown及why/actions。按需Full API/Architecture/Compatibility/Usage/Integration/Development/examples/CHANGELOG/version-distribution，不shared root/workflows/Mini/Workbench/Repo policy。历史保，candidate/published准确；source再修重绑docs并重affected gates，不拖RC。

有界自审：pre-edit evidence/contract checklist，实际implementation/verification，重读callers/tests/contracts/四指南，聚焦adversarial ownership/failure/resource，核source-data hashes/trace-receipt membership/permissions，修material并重测。不无限循环/人工拆task。真blocker需实际attempted commands/evidence，非泛partial。三步全部review-ready后停，不预授PASS或早关七RI；记phase/introduced defect/repair SHA/history。

## 3. 精确授权与一份英文报告

仅指定Full Preview2 branch、Full-owned source/tests/docs/research/version/local worktrees；精确上述lks0011/OrdWeave各一research branch做benchmark/repro/instrumentation/prototype/evidence，允许commit/push/窄hosted research，保application/pins/disposable。一draft stacked Full PR对PR30 branch，精确scope/base。无OrdWeave main/dep/其他production、global config/credentials/private paper/Full CI-controls-rulesets-ownership/Mini/Workbench实施。stacked不跑真实main Scope Guard，trustedbase核inventory，报absence、later main gate仍必需。无directmain/merge/retarget/rebase/force/tag/release/assets/Latest/Preview3。#28–30及旧据保，research非发布/采用。

一英文报告Steps3/4/5：source/base/start/final local-remote-PR/implementation-doc SHA、research branch/workflow/OrdWeave pins、file inventory、step findings/integrated choices、等价lifecycle semantics/phases-total-memory-crossover、paireddownstream/attribution/core-adapter、scale-sparse-tail-skips、prospective criteria/adopt-reject、repro/raw、correctness-failure-compatibility-CI、version、guidematrix/ledger、limitations/true blockers、Technical Debt Impact FULL-RI001–007。区分historical/new、verifiedimplementation/proposedcontract。无benefit诚实no-adoption不编release；独立accept/integration-publication/Preview3由owner，IST仍暂停。
