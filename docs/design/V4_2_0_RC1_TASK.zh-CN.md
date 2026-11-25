# Full v4.2.0 RC.1 — 文档更新、完整源码指南信任重建及候选验证（历史中文译本）

原文：[V4_2_0_RC1_TASK.md](V4_2_0_RC1_TASK.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_RC1_TASK.md)。保原件；英文执行指令为准，本译本不扩大授权。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

Preview3 Steps6–7独立PASS后owner授权进RC，固定三步按序同包：(1)普通文档/定量展示，(2)四详细指南穷尽source-bound重建，(3)有界独立视角审查/final RC验证。普通文档先详细指南。是稳定化，非新Preview/优化campaign/论文/发布；停独立acceptance，先全文读任务。

## 0. 权威、source pins与历史保护

私下完整读owner现行协作/release policies，含RC信任重建/scope/debt/bounded review，私报告记revision/access，不公开私policy正文/URL/私前提。读README/entry/development/ownership/release/freeze/compatibility/consumer/#3/#27/task/stage-design/source-tests/research。核live非只本文件。

| 角色 | 精确revision |
| --- | --- |
| main签发/Stable仍4.1 | c108acd72b37614f4be271c9747484c2dde5eac4 |
| 已发布Stable生产 | 70d7f0ef5857b54381b863d292a213909d87459f |
| accepted Stable guide | 1a175f91e4c338c2afe6a73a9f8f9c5b149768e5 |
| Preview1 production/PR28 | 22544cf283694dd27cef203caba9cf1838845947 / 3a2b3eabd0153531e1064457519b40deb4af9842 |
| Preview1.2 production/PR29 | 61e9290ec5ba47b9092bcb8ae7d61f13104ea839 / 83367fd9b3b7297db1642c2eea6159bf395ef286 |
| Preview1.2.1 diagnostic/PR30 | c977b0fd14a501d769b9783bfdcc61266bd1b5ae / 07b739de1841256ed89236b4096d130f88206956 |
| Preview2 production/PR31 | fe00536e7bd7f4e5128432ea4a29e54158074903 / 01bc01e2d9ddbda448d9ef0807150f419203b580 |
| same-revision snapshot regression | 92fb7fad977f792a7d9e3e0e430a5d5f32e8914a |
| Preview3 offset-one wire/restored mixed tags | 754b3088c160564f6666ee520284168ee5da6ac7 / 3ac82d26301ea12876a3ebdba6319fe8233621f3 |
| accepted PR32/latest provisional guides/RC parent | 168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0 |
| OrdWeave/official Full | b12a007c50c2f9efcbee500a14d3cf09dcc74f10 / 70d7f0ef5857b54381b863d292a213909d87459f |
| OrdWeave final native evidence | ebfa5c62245f793e90fde590b577003839d3e0b1 |

七development steps独立技术通过不等于七research records全解决。Preview2 Workbench attempt3在01bc01e2成功（45406241385）。Preview3 final Full push/PR45515911042/45515918263、Mini45515918235、Workbench45515918404、measurement45515918272成功。核live exacthead，保初始runner失败/拒实验。#28–32仍open draft，非已合并/发布。

核既有 `dev/full-v4.2.0-rc.1` 精确accepted32+task-only，isolated worktree保unknown，不amend/force/rebase/retarget/delete/写旧PR branch；main不变。

ownership是机器执行边界：README.md/README.zh-CN.md/docs/FULL.md/docs/FULL.zh-CN.md/contribution/product-family navigation属Repo，多数Full docs/source/tools/evidence属Full。读ownership.json/OWNERSHIP.md/trustedbase实现，交两独立owner单位：

- Full RC branch一Full-only stacked draft，target `dev/full-v4.2.0-preview.3` 精确保#32。
- Repo branch `docs/full-v4.2.0-rc.1-navigation` 从verifiedmain c108...，一draft对main，仅必要Repo文档导航，无governance/workflow。main/publicStable4.1，RC另branch未合；main无文件用immutable candidate links，不称RCtag/merged/released。最终pin需后commit则先ordinarycontent，Step3只更新refs。留以后真实merge/publish同步note，不自动merge。
- 两owner普通文档都Step1先Step2，FullPR不混shared，不scope-exception/跨ownerPR。无Mini/Workbenchcode-doc/pins/workflows/settings/第三仓/论文变更。

运行生产冻结acceptedPreview2。metadata适当4.2.0-rc.1（header prerelease/string/affectedmetadata，CMake4.2.0），先独立metadata/source baselinecommit再docs绑定；验行为不变，operational与exactcandidate build/test分开。validation checkout必须含两Preview3 regressions，不能仅fe00536e。confirmed correctness/portability/compatibility blocker精确报，不借稳定化重实现；仅trivial metadata/build/docs及targetedverification授权。substantive defect/contract change停止affected、独立决定。保64functions/14types/status/LS1/LKS4SNP1/LK1migration/Handle/callback/ownership/failure。

## Step1/3 — 普通文档及定量更新

盘点全部current public docs/entry，分current/candidate/historical/shared，读正文/source dependencies；原位重写/替换obsolete，不只disclaimer，去retired现行指令、改实现、补概念。历史文档/changelog/release/observations/负面保，不倒改历史，分publishedStable/main/RC，两语一致。

Repo核双rootREADME、双FULL、真正受影响product/contribution/nav/docs/index.html，保Mini独立version/download。Full核API/ARCHITECTURE/USAGE/INTEGRATION双语、DEVELOPMENT/COMPATIBILITY/VALIDATION/MIGRATION_V3_V4/V3_MIGRATION/V4_FREEZE/BENCHMARKS/WORKLOAD_EVIDENCE/FULL_SELECTION_GUIDE/FULL_CONSUMER_HANDOFF/CHANGELOG/example-benchmark README及inventory其他现行。改过时activePreview1 priority/rank-label/performance-structure/test checkout/version-install；历史无关不凑改量。改runnableexample需复现。清晰Full能力/target/limits/stableoccurrence/explicit-managed/snapshots/keys/wire/restore及不拥有businessID/transactions/persistence/consumer policies。入门简洁，详据Full supportingpages，链四guides/selection/RC/archive，不编untaggedRC下载URL；用户所需4.1instructions保。

### 1A. 累计性能/资源/图表（必需）

owner要更新tables/charts，非仅text；Full简洁evidence section及detail page、machine data/deterministicgenerator，Full-owned docs/benchmarks SVG/PNG，caption/alt/units/exactsources。科学清晰非营销。README/Fullentry紧凑比较及详情链；main无figure可immutablecandidate URL，核access/status。

独立重算rawCSV/protocol/pins/tracehash/samplemode/oldanalysis，分chronological mechanism-evidence与final apples-to-apples。finalPreview3同host matchedcell matrix比4.0/4.1/accepted1.2/List/ThreadedTreap/native tag-list OM，不能独立median或百分比相乘。Preview1/1.2按原checkpoint/host；不同complete trace-control-host-protocol分series或标discontinuity，不假连续。1.2.1/3为research-validation非额外production加速；V1–V3仅证可比才数值并列，否则qualitative/分scope，不称API代际互换。RC继承运行行为，实测源是acceptedoperational非编RC新bench。

至少：

1. **version evolution table：** release/checkpoint/change/baseline/workload-host/有据elapsed ratio-reduction/memory-resource/source-test-evidence pins/scope-unknown。
2. **同期live：** 分Windows/Linux，4.1参考及4.0/1.2/alternatives，overall/compare-heavy matched medians、wins/total/adverse。ratio=candidate/control，reduction=(1-ratio)*100，speedup=1/ratio，分清不混指标不称全快。已独立重算final/4.1 overall0.6291057068/0.6400470680，compare-heavy0.1977184064/0.2191729090，各45/45；final/4.0为0.5939734423/0.5402864517，/1.2为0.9945242198/0.9981393365。仍从raw重算，保OM wins/worst。
3. **memory-time scatter：** x=requested memory（如peak MiB），y=ms，panel精确同workload/size/seed/host。优先100k seed17 compare-heavy Full/OM（peak4,278,776/13,437,504bytes）及有据update-heavy/mixed。七formal时间配独立diagnostic memory，不拿diagnostic time代prod。明sample/accounting差；仅可比control，missing非零。retained/peak/traffic/RSS-working set/externalpayload分、不混unit/host；pointlabels/必要logaxis，无空泛Pareto。
4. **lifecycle：** wallratio与phase sum分。endpointflat1.137284/1.136323、arbitraryrelative threadedTreap0.580369/0.526053属不同class，13vs5cells/host、retention/association/namespace/holdouts/incrementalTreaprestore局限/缺bulkalt/不同callback-OOM-provenance。wire decode对前implementation另，不doublecount。
5. **OrdWeave/scale：** 27generated×memory/persistent、702observations含warmups/profiles、officialpin不变。对1.2 latency1.001530/0.997323，staging/WAL主导，nestedflush不加，不无据全DBspeedup。sparse shrink-regrow/retained/1midentity/更大长history未知，generated非field。分table/figure，不混scope。
6. **可复算：** raw/source/compiler/host/hash或Gitblob/grouping/sample/timing-memory/formula-unit/generatecommands/exclusions/adverse，保B32/B128/directtransfer/midpointlabels等拒方案。目视axes/points/legend/caption与data。优先不新campaign，缺可比measure记gap非未经授权实验/插值。raw/source immutable，RCderived新文件。

Step1验收两owner实质renew、共享中立准确main/published、presentation可追数学正确、changeddocs/figuresinventory；记Step1commit/evidence后Step2，无另例行owner交接。

## Step2/3 — 完整详细文档信任重建

四中文指南合起来让有能力新maintainer理解整个Full实现/contract/mechanics/boundaries/validation，不靠散Preview补当前行为。补source非取代source/formalproof。全文重建非openingnote，逐全文读，留准有用，重写/删stale-contradictory-abandoned-redundant，补functions/diagram/examples/callchains；历史清晰分/归因。

### 2A. 时间顺序恢复及独立finalsource检查

三baseline：acceptedStable guides/source、lastprovisional3 guides/source、exactRC source/build/tests。逐Preview1/1.2/1.2.1/2/3 task/report/design/validation/consumer/真实history含docs corrections。看git log/show/patch，added/removed/replaced/reverted/neveradopted，含Stableguidemerge至candidate。不假stage穷尽/继承guide真实。每semantic/diagnostic/test change多对多map所有docs sections，introducing/fix/later test/historical snapshot/evidence分。非diff-only，完整finalinventory/callers/callees/private/publicheaders/build-export-amalgamation/examples/tests。

持久Fullledger如 `docs/design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md`，可机器inventory：ProductionSources.cmake16modules、相关privateheaders、64functions/14types真实签名-contract、legacyhelpers/diagnostic-only/builddistribution/history/tests-oracles。按module/function/coherent obligation：old-current/source/history-patch/behavior/guide sections/changed-confirmedunchanged-deleted/tests-assertions-execution/limits-covered rationale。无假coverage百分比，nonFull排除解释，查实际function非name，不把缺core解释当style。

### 2B. 四完整指南（必须全文）

- **IMPLEMENTATION_WALKTHROUGH.zh-CN.md：** build/publicuse至explicit insert/remove/move/compare、managed/search、sort/Group/Batch、capture/key/wire/restore/import/consumer/ownership。真实allocation/publish/noop-revision-cursor/callbackorder-reentry/error-cleanup，非“validate then save”；有用图，分current/history/version-source-test。
- **IMPLEMENTATION_API_REFERENCE.zh-CN.md：** 每function/type/parameters/exactreturn-status-output/valid-invalid/owned-borrowed/errorprecedence（已规定处）/comparator-association-resolver/stableduplicates/sync/cursor/examples。核header+implementation，分contract/private，不编API/guarantee。含same-revision isolation/markeridentity/serializedviews/freshrestored Handles/LS1domain/LK1boundary/no danglingvalidation，全examples/tables一致。
- **IMPLEMENTATION_INTERNALS.zh-CN.md：** modules/layout/B64-min32/implicitAVL/threads/stableResidents-Blockowner/height-subtreecount/suffixupdatesvsfullrepair/split-merge-redistribute/spare-publish/bounded uint64 tag spacing-repair-probewrite/非零subsequence跨zero/actualAVLfallback/bulktags/overflow/complexity/no arbitraryrecovery deadline；managedbounds/reentry、stablemerges、snapshot row-namespace-assoc allocation/compactlayout、marker-ref-atomic、fixedwidth bytewisebigendian、completevalidationthenalloc、resolverfirstscratch/bulkrollback、legacycodec/LK1、allocdiag-prodseparation。明removedv4_rank、abandoned/current-old，bytes平台资格/无未测声明，每path资源失败cleanup。
- **IMPLEMENTATION_VALIDATION.zh-CN.md：** guarantee→实际testfile/function/assertion/fixturebytes/source-test-build-CI SHA。model/boundary/adversarial/failpoint/identity/noop-revision/tags-restoredmixed/isolation/offsetone/malformed/migration/API/32bit-san/examples/source-install-amalgamation/distribution-assets。修早checkout错，精确testtree含两新regression，operationalSHA另。可复现commands，分historical/current、method/raw/limits；解释failure诊断非只CTestname。先realbaseline再docs，finalheadCI放PR/independentreview避免自引用。

consumer ledger对整个RC delta，operational无变/metadataonly/exactsource-test-doc baseline/contracts、adapter与DBcore分判、adoptionretest/pin不变。普通docs与四guide协调正确link。无需保obsolete组织，清准优先；重组仍保四入口及internal links。Step2验收全文读/finalsource+完整history协调、changedremovedadded记录、critical obligations全map、准确pins/解释、四行真实sections及unchangedmatrix；实质差异必须修或标blocking。无新algorithm/unapprovedproduct。

## Step3/3 — 反复复核、final验证、止独立审核

Step1/2后实质有界，非spellcheck/反复grep。finiteinventory来自source/API/history/sections。

- **A：** 各ordinary/detailed对finalsource/history，找omitted/wrong/stale/duplicate/contradiction/examples-links，修confirmed。
- **B：** 新maintainer/adversarial、不依旧draft，独立沿代表use/failure/memory/comparator/snapshot/restore/compatibility/distribution，双向source-doccoverage/table/diagram/math，修。
- **C：** 修后复扫affected及dependencies，对audit inventory，无旧现行claim/已知omission，links/examples。有substantive就修并repeat相关full/targeted至clean substantive且criticalcovered；不满三轮有错停，也不无尽造issue。risk-based，范围资源内真不可解给exactblocker/gap，非宣完；记每pass/finding/actualrepair/recheck，不合成correctnessscore，仍ChatGPT独立。

### 最终验证

- exactsource/metadata/tests、verify_v4_api.py、旧LS1/LKS4SNP1/LK1fixtures、无behavior/golden/ABI-format破。strictGCC C17 Debug/Release `-Wall -Wextra -Wpedantic -Werror`，实际CTest（baseline39）、七examples、C/C++ source/install/amalgamation、validate_distribution.py/validate_release_assets.py/benchmark/sanitizers-extended、existingplatformCI MSVCx64-Win32/GCC-Clang/AppleClang。local生成候选assets审（source/manifest/checksum/name-version/repro），非publicupload/tag，查generator/prereq，无假发布，日志commands/missing层实报，保旧release。
- changedcurrentdocs UTF8/links-anchor/figures/pins/nav/examplecompile/chartrepro/inventory，detailed后再核ordinary修传播错误非新step。trustedbase两owner精确fileinventory。finalhead Fullpush/stackedPR Full-Mini-Workbench-measurement及旧gates复核；Repo maintarget需真实trustedScopeGuard及适用产品/measurement，核runidentity非local模拟。absent/blocked/runnerfail精确报不bypass；仅terminaltransient boundedretry，不改matrix-label-governance。stackedFull无真实mainScope，futureintegration仍须，不假集成acceptance。
- 每branch实际localremoteHEAD/base/files/source-metadata-test-doc/CI/status/cleanunknown精确。mainbase共享页准确Stable/unmergedRC。Fullordinary/figurecommit已存在后pin并最终核access/status。finalhead验证后再写需重affectedgate。

## 交付与授权

Full RC report `docs/V4_2_0_RC1.md`、reconstructionledger、四guides、ordinarydocs、可复现tablefigures-data-script、consumerledger/versionmetadata/candidatereceipts；另Repoordinarydraft，均exactevidence/owner。保七FULLRI和A–Flineage，仅真实changedstatus，无blanketclosure。含historic-currentbaselines/sections/findings-repairs/nonblockinguncertainty/evidencelimits/release-integrationgates。

授权相关repos/branches/archive读取分析、允许docs/derivedscripts-data-figures/Fullmetadata/窄verification、isolatedbuild-test/localasset、仅指定FullRC及Repo导航branchcommitpush、每branch一指定base draft。旧branches/releases不动；无mainwrites/merge-retarget-rebaseolder/exception/rules-workflow/tag/Release/Latest/publish/productiondep/无关repo/newPreview-Stable。集成/RC发布owner独立决定；livebaseline异先保work、报mismatch停dependentwrites。

一英文总报告：私policy/access、start/final localremote/baseHEAD、operational-source-test-metadata-doc、三fixedstep commits、ordinaryinventory/beforeafter、quantitativerecompute、完整Previewhistorycoverage、四guide真实matrix、reviewfinding-repair-gaps、commands/logs/exactCI/assets/scope/unchangedcontracts-pins、**Technical Debt Impact**（IDs/evidence/triggers或Nochange）。candidate readiness非publish/integration，不自授independentPASS，停审核。
