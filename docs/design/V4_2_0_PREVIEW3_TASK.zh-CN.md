# Full v4.2.0 Preview.3 — 综合工程健全性与有据采用（历史中文译本）

原文：[V4_2_0_PREVIEW3_TASK.md](V4_2_0_PREVIEW3_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW3_TASK.md)。保留原件；英文执行权威，本译本不重启历史授权。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

**签发2026-11-25，owner当地日期。** [计划#27](https://github.com/RXY712200/LayerKeySort/issues/27)最终主开发包：Step6/7 FULL-RI-006、Step7/7 FULL-RI-007。一连续Codex任务含范围内必要fix、实际comparisons、source-bound docs、一英文报告。非新core探索/论文/RC/发布；保累计成功设计，测真实限制，建立应用特定采用依据。不制造优越/删adverse/只metadata造版本；已证限制或有理保留tradeoff有效。下一点是独立review及owner RC决定，不自动新Preview。

## 0. 精确权威、来源与安全起点

完整读现行Development-Playbook WORKFLOW.md/RELEASE_POLICY.md、templates、本地AGENTS/repo instructions；核policy SHA（签发 `ba0c6fb6b35650608b857adba4ad0f672d65cac7`），私记read/access。读#27/#3及amendments/comments、原tasks、永久live-core history、stage/design/evidence/protocol、四指南/ledger、API/freeze/compatibility/architecture/development/validation/ownership/release rules。看真实source/tests非summary，不公开policy/论文/credentials。

| 角色 | 精确pin |
| --- | --- |
| main签发 | c108acd72b37614f4be271c9747484c2dde5eac4 |
| v4.0.0 | 80004ae732daf696f7e9192e417de0edb0b2aafa |
| v4.1.0 | 70d7f0ef5857b54381b863d292a213909d87459f |
| Preview1 production/保PR28 | 22544cf283694dd27cef203caba9cf1838845947 / 3a2b3eabd0153531e1064457519b40deb4af9842 |
| Preview1.2 production/保PR29 | 61e9290ec5ba47b9092bcb8ae7d61f13104ea839 / 83367fd9b3b7297db1642c2eea6159bf395ef286 |
| tag diagnostic/保PR30 | c977b0fd14a501d769b9783bfdcc61266bd1b5ae / 07b739de1841256ed89236b4096d130f88206956 |
| permanent live-core | 779157f5604480b5f14c866e736567d7a0d9811c |
| Preview2 production/isolation regression | fe00536e7bd7f4e5128432ea4a29e54158074903 / 92fb7fad977f792a7d9e3e0e430a5d5f32e8914a |
| 保PR31/新branch base | 01bc01e2d9ddbda448d9ef0807150f419203b580 |
| last hosted research | lks0011 research/preview2-lifecycle-matrix，f720828df2e9f7c12eab01289479d108174f18fd |
| OrdWeave original/official Full | b12a007c50c2f9efcbee500a14d3cf09dcc74f10 / 70d7f0ef5857b54381b863d292a213909d87459f |
| OrdWeave Preview2 archive | 1e40955a783f91ec835170bea664bfa8e324b465 |
| original Full/OrdWeave archive | f41b7be182c2ef766ee0dec1211500f7dd762f54 / aa95b240b019c518a98bfb900ec374d24a537165 |
| native practical OM | j-hui/order-maintenance ac4b902d103beb30cee3316d20cd10701f52d15a |

仅 `dev/full-v4.2.0-preview.3`，精确PR31加task起；fetch核actual base/start/local-remote/source/task-only diff/clean-isolated/PR。#28–31保独立open draft，不amend/merge/retarget/rebase/delete/force/publish。PR31仍open时一draft stacked against `dev/full-v4.2.0-preview.2`；若另授权集成改base，报并取精确决定。Stable4.1；1.2.1为negative selection非release，无1.2.2。保B64/bounded tags/AVL fallback/exact range-location updates/monotonic diagnostic/fixed-width wire/same-revision snapshot independence。仅修confirmed defect或实质有据工程issue，不任加API/wire/architecture/break。综合交付有理可metadata4.2.0-preview.3，分source fix/docs-validation convergence/released；无tag/release。

### 当前CI blocker及诊断

签发Preview2 technical acceptance有独立核source、1,872lifecycle rows/208cells、2,430live、720native app、73fault、精确Full六配置logs；reviewer未重建全binary/解全compressed receipts。Full CI45406241272/45406230389 pass。Workbench45406241385（01bc01e...）首AppleClang job138487623671约15分钟取消，runner_name空/无steps-logblob，aggregate正确fail。attempt2始2026-11-24 16:00:24 UTC，job138579797624最后query仍queued无steps，其余五jobs成功。未改 `.github/workflows/workbench-ci.yml` 为macos-latest/fail-fast false/fail-closed；无source failure证据，runner原因未定。开始前/最终都记current run/attempt/job。仍长queued如30分钟后，查assignment/annotations/logs/workflow/run-cancel-concurrency history/其他macOS/GitHub Status/可访问limits-permissions，分事实假说。仅terminal transient infrastructure failure且授权有action才bounded retry；不反复cancel queue/造committrigger/换label/删AppleClang/改gate/bypass/别SHA冒green。未分runner报精确elapsed/attempt/inaccessible/pending，继续其他工作。真compile/test fail修Full范围或报cross-product/governance blocker。旧Preview2 gate为后集成独立前提，即便新Preview3 pass。保旧成功。签发文件broad diff继承trailing space，如修仅新branch最小whitespace并记，不重写旧record。

## Step6/7 — FULL-RI-006：全产品工程健全性

### 6.1 绑定源码综合审计

建 docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md 及 benchmarks/research/full-v4.2.0-preview.3/ 持久inventory，定accepted Stable/provisional/current source。逐全部production translation units/public-private headers/build-export-amalgamation/validation/tests，不只newdiff；file/function或coherent module、SHA/invariant-contract/test-oracle-inspection/limit。协调原A–F debt和FULL-RI001–007/current docs，保stable IDs/已知first repair/rejected。区别confirmed bug/test deficiency/resource tradeoff/unproven risk/environment/out-of-scope，非每改进都是bug。

实际审：

- **Explicit Order：** block list/AVL order-parent-height-subtree、B64/min occupancy、stable独立Resident ownership、block/local updates、split/merge/redistribute/repair/stale slot cleanup/no-op revision-cursor/cross-owner-error-output/compare sign/callback guard。非零tags跨zero严格单调；fixed-width arithmetic/capped repair-probe-write/zero fallback真实AVL/bulk spacing-overflow。无任意恢复保证须记录；核无hidden allocation/revision/contract改。
- **Managed：** comparator life/stable duplicates/upper-lower bounds/精确compare次序/reentry-external sync/AVL-private API/move-insert-remove-search失败/ownership-identity；同payload非合并occurrence。
- **Sort/Group/Batch/legacy Path-Tree：** stability/comparator-ordinal-tie/merge-overflow/callback-cleanup/LK1 import/status-compatibility；实核64函数14type及opaque-private边界，不因聚焦Order删supported code。
- **snapshot：** 每occurrence callback一次/立即copy/same revision mutable payload/namespace-cumulative ends/独立retained bytes-borrowed views/marker ref-capacity-life-currentness/release/source destroy/C17-MSVC atomic/external sync。LS1 ASCII order-domain/LKS4SNP1 framing，固定4/8byte unsigned shifts/unaligned/width、两pass完整验证才allocation、length-overflow-reserved-trailing/oldgolden/no partial。resolve-first ordinal/Npointer scratch/bulk cleanup-fresh handles/oldformat-V3 import，不删validation/callback求快。
- **alloc/status/platform：** publication/cleanup/reachedfail/counter-revision overflow/transfer/stale cursor与invalid-dangling合同/caller bounds/32bit size_t/MSVC-Clang-GCC/无UB shift-alignment-overflow/deterministic wire/API-export-link-C17-C++。不故意dangling作supported；untestable前提实记。
- **source-doc-consumer：** 四完整中文指南、API/architecture/usage/integration/compatibility/migration/development/validation/changelog/examples/install-amalgamation/ledger；搜oldrank/layout/metadata-callback/source links/published-candidate。1.2保B64-label，negative study无production bump。历史context保，不留错误present instructions。

### 6.2 合并对抗验证gates

复用独立models/tests，不为形式重复。audit找gap后针对assertions。mixed states、zero/nonzero/fallback after restore、duplicate identity、managed equality/reentry/cursor-revision-noop/foreign status、snapshot-Group isolation/release、wire malformed-truncated-oversized-legacy、reachedOOM/strong publish。独立invalid boundaries和支持的source mutation；long/churn/holdout对逻辑sequence+全live/snapshot assoc+cleanup。新path全exercise，保failed/min reproducer/fix。strict GCC C17 Debug/Release `-Wall -Wextra -Wpedantic -Werror`、MSVC x64/Win32、Linux GCC/Clang ASan/UBSan/extended、AppleClang existingCI；实际CTest（历史39可增长）、`tools/verify_v4_api.py`、oldgolden、七examples、source/install/amalgamation C/C++、`tools/validate_distribution.py`/`tools/validate_release_assets.py`，`LKS_BUILD_BENCHMARKS=ON`、适用 `LKS_ENABLE_SANITIZERS=ON`。static analyzer/fuzz/extra model有用可用就跑，记config/falsepositive/limits，不以报告替测试。finalHEAD Full/Mini/Workbench/measurement，trustedbase全file分类现在必需，真实scope-guard仅main target且未来集成前必pass；不改shared workflow/governance/其他产品production。

Step6验收：约定Full范围无未解confirmed release blocker，source-bound maintainability/correctness/compatibility-failure-resource证据、必要fix、精确source-test-doc和实质uncertainty。runner问题非codedefect亦非gatepass；同task直接Step7。

## Step7/7 — FULL-RI-007：比较采用依据

### 7.1 前瞻计划、保留与语义

新candidate计时前commit `benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.md`：可检验应用假说/边界/exactcontrols/factors/selection-holdout/hostsample/time-memory admission/non-target regression/uncertainty-adverse。保source-data/archive/negative（B32/B128/direct transfer/midpoint exhaustion/flat metadata removal/V3 relabel-retention），核permanenthistory links/hash/facts，真错显记修，不静改旧result；无论文。旧review重算全primary但未重建每binary/hash每compressed receipt；新repro查source-trace hashes/membership/raw receipt/commands-flags-compiler-binary/license/timers。新outputdir，QPC/CLOCK_MONOTONIC/Rust Instant、native optimized Release、rotated serial fresh、一warmup七formal critical component、diagnostic另；slow app/scale anchor可前瞻减samples但报dispersion。无unmatched subtraction/虚构zeroRSS；skip/fail/timeout/memory-disk receipt保，requested与processpeak/RSS/traffic分；每cell min/max/IQR和timer-resolution下tails。数字仅精确成功操作且说明guarantee差，非各library failure等价。

复用public replay/hosted，授权private lks0011 `research/preview3-final-matrix` 从f720828...仅comparison harness/workflow/evidence；OrdWeave `research/full-preview3-20261125` 从实际b12a007，旧archive另读。无officialdep/productionapp/论文/main/unrelatedworkflow；licensedpublic归Full、app归OrdWeave，保原receipt/pin，不只expiringartifact。核runner，不为history无谓重跑。

### 7.2 最终同期live与独立validation

精确final operational对4.0/4.1/accepted1.2（记diagnostic pin）、List/Treap/tag-range/list-range；final与accepted异则八variant。旧Preview1/V0.5分chronological/cause，不pool。每host45冻结20,000-op（1k/10k/100k×三seed×五mix）、model/semantic gates、native exactsource；七formal+一warmup+一diagnostic=每host3,240rows/八variant。预选未调参seed/structural holdout如三sizes不同population/mix、concentrated move/split、restore、duplicate refs。共同public成功/setup齐，wrapper/language/compiler/nativeRust边界说明。有界可行licensed faithful same-language OM或matched C probes，剩归因明确，不把跨语言rank称理论优越。报counts/wins/adverse、matchedcell-ratio中位/resource按host，非pooledmean/无关medians相乘。保live优势；反复>10%non-target且超dispersion按预设rule调查/修/披露保留，不为少数noise重开架构。

### 7.3 等价lifecycle与整体工程

延Preview2 C17 linked-order/flat-history（endpoint-only曾赢且guarantee轻），加一能arbitrary relative query、mutable、independent retained history、restore/continued的成熟OM或tree/index+stablewrapper+ownedhistory合理composition。复用pinnedlicensed adapter/source/notice/高效layout。纯endpointlist为合法contrary但非唯一。共同成功语义匹配arbitrary insert/move/remove/query/traversal、association/namespace、duplicate identity、snapshot跨mutation/source-payloaddestroy/keys-wire-deserialize-resolve-restore-edit；若比managed promise纳comparator，未实现guarantee明分，非称equivalence。不给alternative多余负担/foreknowledge，exactwire合理可就核，否则格式/validation/size差披露。真oracle非人工latency。分timed phases和wall lifecycle、之前未计source destroy/allocation、计时外oracle、serializebuffers、externalpayload，不称选phase之和就是wall。

用原13cells（1k/10k/100k、retention0/1/2/8/16、variable/empty/wide assoc、namespace8/4096），加预设arbitrary precedence/managed-history、多mutationrates/seeds，超endpoint rotate。same revision callback bytes变/capture afteredit/restore future；compare exactfinal/4.0/4.1/适用prior/endpointflat/matureindexed，bounded anchors非Cartesian。测phase-total dispersion/historyresource/fullpeak-temp-wire-alloc/callback-resolver/guarantees。derive真实crossover和simplewins，不给只需mutable用户强配history。不称universal/theoreticalnovelty/measured developer effort；integration burden据真实adapter操作/components/guarantees/tests，非编hours/score。

### 7.4 native应用与规模

Preview2有486 original rows（27traces Stable+1.2）、234final（九seed17×三dep）。本次全27原trace memory/persistent，samepinnedapp、exact4.1/1.2/final，核旧ZIP/member/hash/initial state；slowapp三formal+warmup，representative独立matchedattribution。4,096-occurrence persistent holdout或独立安全larger/crosscollection，checkpoint/history/reopen/modelidentity、officialpin不改、FullreachedOOM/upstreamsuite（原12非固定未来）。是generated native非field。旧1.2/4.1 memorymedian .655503/persistent .980250；wirechange九trace .996311/.989207；stage/publish memoryshare .945519，persistence WAL .975710/nestedflush .951181。不可加nested spans或假exclusive comparator/allocator。旧largestcheckpoint adverse1.102359 ranges重叠。TD001globalrebuild、TD002semanticreplay、checkpointbound/IO归DB，无独立DB优化/pinpromotion。有益可行加stage comparator/alloccounts或第二真实use非新产品/合成宣传；不可行实记。adapter/core分verdict/actions/pin；确认真实total益或缺乏。

复用1m→1%sparse42,753,776→431,576 requested、cluster/scatter两cycles，重复critical exactfinal及独立concentration/shrink-regrow/restore/retainedholdout。10k/100k/1m、capturecadence/sparse/occupancy-tag/request-retention-peak-traffic/可支持working set/cleanup。3m/10m/longhistory仅guard准且改selection；否则unmeasured。无耗尽/为新颖造allocator，历史证据重验no-pool/B64，不称B8/B12已测或最优。按真实budget评alternative，arbitrary longzero恢复未保证。

### 7.5 采用指南及七RI处置

清楚source-evidence guide：何时Full、plain list/treap/nativeOM/composedhistory，scale/mix/locality/retention/assocsize/format、identity/ownership/failure/provenance/managed需要，tradeoff/contrary。每advantage连mechanism和同期evidence，component/lifecycle/downstream/resource分开；不虚构weightedscore。论实际prior art Dietz–Sleator/Bender et al./pinnednativeOM/AVL/blocks/snapshots，工程整合/实测协同非新理论；替代composition据真实实现非strawman。有据adoption须meaningful target measuredbenefit，或性能相当且实质resource/contract/integration益，独立holdout/contrary/uncertainty；某class无益明说。重评FULL-RI001–007及适用A–F，每subproblem证据/classification/disposition（scopedresolved/retainedtradeoff/partialopen/blocker/unmeasured）/source-fixSHA/risks/triggers。步骤完非研究全关，不添steps/自动Preview；精确RCentry/gateblocker，非论文/publish。

## 2. 必需artifacts/技术文档/最终验证

`docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md` audit/invariants/rationale，`docs/V4_2_0_PREVIEW3.md` decisions/evidence/RCrecommendation，Full-owned selection如 `docs/FULL_SELECTION_GUIDE.md` 相关Full docs入链，七IDmatrix连#3/#27，researchdir含protocol/source-license/repro/raw/scripts/完整analysis-adverse/holdouts/source-build-toolchain-tracemanifests。沿既有结构，无假据或仅expiring；finalsource与evidence/PRHEAD分。fix需focused regression/firstfix/scope/重affected比较测试。CMake4.2.0，适当headerpreview.3，仅candidate无assetpublish；64/14全contract及oldLS1/LKS4SNP1/LK1goldens保，不refresh掩回归。

Full samePR严格gate：全文同步四IMPLEMENTATION_WALKTHROUGH/API_REFERENCE/INTERNALS/VALIDATION.zh-CN.md。四行matrix真实sections、changed/unchangedfiles-functions-contract、old-currentSHA/tests/evidence/unverified。分acceptedStable70d7f0...、auditedguide `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`、全部provisional、currentimplementation/test/docs。真false修，history保，不能只开篇note。FULL_CONSUMER_HANDOFF模板source/revision/PR/Issue/before-after/API-ownership-Handle-reentry-callback-concurrency-failure-revision-LS1-wire-restore-distribution/time-resource-adverse/OrdWeavecore-adapter分判/retest-action。按需Full API/Architecture/Usage/Integration/Compatibility/Migration/Development/Validation/CHANGELOG/examples/version，无rootshared。research-only准确未变source/实际reviewsections，不编代码变化。RC另重建freeze，非豁免当前docs。

有界自审：实际inventory/tests/independent-style adversarial/raw-analysis-receipt-provenance-CI，修具体并重测doc；原两步和evidence完后停，非初screen/无限review。适用exacthead Fullpush/PR/extendedsan/Mini/Workbench/measurement；最后复查PR31runner，不把queued/cancelled/absent称PASS；technicalreadiness/integrationgate分，latermain scope必需，不改rule。externalblocker给logs/attempttime/actualscope，继续其他。

## 3. 授权与报告

指定Full branch source/tests/Fulldocs/research/有理metadata/localworktrees；仅上述lks0011/OrdWeave各researchbranch，benchmark/instrumentation/verification/repro/evidence、适用hosted/researchcommits；一stackedFull draft。其他source可读，无officialpin/productionwrites/privateleak/其他app-branch-governance。无directmain/rebase-retargetolder/force/merge/tag/Release/Latest/assets/RC/newPreview/论文。集成RC另owner授权，保history/negative。

一英文报告branch/base/start/final localremotePR/implementation-test-docSHA、researchworkflow/unchangedmain-pins/fileownership；Step6modules/defects-fixes-tests-boundaries；Step7finaldata/cellcounts/semantics/holdouts/phases-resources-tails/contrary/selection；OrdWeavepairedattribution/scale-unmeasured/source-bounddoc-consumer/exactCIjobslogs含旧Workbench/repro/**Technical Debt ImpactFULL-RI001–007**及RCrecommendation。独立acceptance非自己，未完成不宣称；真environmentblocker示实际commands/scope非空泛。止约定checkpoint，不自动RC实现。
