# Full v4.2.0 RC.1 — one complete continuation objective

This is one continuation of the already-authorized RC.1 work package. It is NOT a new stage, a new Preview, or a sequence of separately delivered tasks.

## One objective

Complete **all remaining RC.1 acceptance work** and deliver the two existing draft PRs at final, independently reviewable heads. Continue in the same Codex conversation and the existing workspaces. The original three-step order in the task file remains the internal dependency order, but **step boundaries, document batches, commits, successful CI runs, and progress checkpoints are never stopping points**. Do not use `/goal` or another mechanism that turns this into repeated short execution windows. Do not send a final progress report while authorized work remains and the next action is available. Do not ask the owner to issue another “continue” between internal parts.

## Exact starting state to verify

The following GitHub state was independently checked immediately before this continuation-file update. Re-fetch both PRs, branches and check-runs at execution start. These SHAs are verified references, not permission to overwrite a divergent local workspace.

- Full PR #33: open Draft; branch `dev/full-v4.2.0-rc.1`; base `dev/full-v4.2.0-preview.3` at `168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0`; current remote HEAD `b9dc2eba0b91c05aae90f47ddfb6ff8b4de5def4`. The PR currently reports 85 commits and 123 changed files. Latest commit `b9dc2eba` has parent `359bc0285fcc955b9c881affb477363b24a1decc`, message `docs(full): reconcile initial comparison protocol and reproduction`.
- Repo PR #34: open Draft; branch `docs/full-v4.2.0-rc.1-navigation`; base `main` at `c108acd72b37614f4be271c9747484c2dde5eac4`; current remote HEAD `7114cc06bb02a14cd402c77e0bafd17ccef553f3`. The PR currently reports 8 commits and 12 changed files.
- Public `main` is still exactly `c108acd72b37614f4be271c9747484c2dde5eac4`. Published Stable remains v4.1.0: annotated tag object `bdf29c22a44001d5bf1f988bf06dc2324d5f7512`; the GitHub Release is the existing non-prerelease v4.1.0 release [423288672](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0). Both PRs remain separate, open, Draft and unmerged. No merge, tag, release or publication is authorized.
- Latest Codex report says the Full worktree is clean and all four pre-existing Repo documentation edits remain uncommitted and preserved. This is an implementation report; the current local worktrees are not directly visible to this GitHub review. At execution start, inspect the actual Full and Repo worktrees, remotes, branches, local HEADs, staged/unstaged/untracked changes and full diffs. Preserve all changes. Do not reset, clean, overwrite, recreate, or silently discard either workspace.
- Latest Full commit `b9dc2eba` modifies exactly:
  - `benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.zh-CN.md`
  - `benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.zh-CN.md`
  - `docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md`
  The protocol translation now describes append-built occurrence states, independent paired-oracle validation, resident records and block layout. The reproduction translation clarifies package-registry dependencies, measurement harness terminology, warmup exclusion, process sequencing and the native comparison scope. The checklist records a complete comparison of both paragraphs and the reproduction instructions. The English historical originals were not changed by this commit. Codex reports `git diff --check` passed before commit.
- The preceding Full commits remain:
  - `6f4989ce9b5f2989adcc1f9d8eefd58284c74c78`: Preview.2 reproduction terminology corrections and checklist update.
  - `359bc0285fcc955b9c881affb477363b24a1decc`: complete Preview.3 protocol/reproduction comparison and Chinese correctness-oracle wording.
  - `cae59fea09a9321be4325a58edde1b21b4a3e5ce`: historical Preview.1.2 / Preview.1.2.1 / Preview.2 protocol and reproduction terminology corrections.
  Preserve these completed comparisons; do not repeat them without a specific new discrepancy.
- Exact-head hosted checks for Full `b9dc2eba0b91c05aae90f47ddfb6ff8b4de5def4` were queried:
  - Push C17 [45633063490](https://github.com/RXY712200/LayerKeySort/actions/runs/45633063490): **in progress**; the Clang extended sanitizer job is still running.
  - PR C17 [45633078532](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078532): **in progress**; MSVC x64/Win32, AppleClang, GCC and Clang-OFF jobs have succeeded; the Clang-ON extended sanitizer campaign is still running.
  - Mini [45633078599](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078599): success.
  - Layer Order Workbench CI [45633078538](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078538): success.
  - Workbench Measurement Evidence [45633078600](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078600): workflow still in progress at the last query; both Linux-GCC-Release and Windows-MSVC-Release evidence jobs had succeeded.
  At that query, 24 of 27 check-runs were completed successfully and 3 remained in progress; no completed failing check-run was reported. Recheck the actual final state rather than carrying this pending status forward.
- Repo PR #34 exact HEAD `7114cc06bb02a14cd402c77e0bafd17ccef553f3` remains unchanged. The visible required checks at that exact SHA are successful, including the actual main-target `pull_request_target` Trusted Scope Guard [45587380487](https://github.com/RXY712200/LayerKeySort/actions/runs/45587380487), Full C17 push/PR [45587374419](https://github.com/RXY712200/LayerKeySort/actions/runs/45587374419) / [45587384140](https://github.com/RXY712200/LayerKeySort/actions/runs/45587384140), Mini [45587383871](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383871), Workbench [45587383968](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383968), and Measurement Evidence [45587383818](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383818). The Trusted Scope Guard is distinct from its self-test. Recheck after any Repo-head change.
- The bilingual inventory still has 89/89 language counterparts or inline bilingual editions. This is presence only, not semantic acceptance. Explicit checklist items still awaiting substantive closure include:
  - `docs/design/V4_ARCHITECTURE.md`: final semantic acceptance of all 15 sections and 13 stress walkthroughs against the historical English source and relevant implementation/history.
  - `docs/VALIDATION.md` and its Chinese edition: final RC validation context and final-PR evidence.
  - Repo `README.md` / `README.zh-CN.md` and `docs/FULL.md` / `docs/FULL.zh-CN.md`: remaining entry prose review and final locale-specific immutable pins to the selected Full HEAD.
  - Embedded human-facing distribution instructions and any remaining in-scope prose not yet semantically accepted.
  The complete corpus-wide review, audit updates, final Repo navigation pins and final exact-head verification remain mandatory. Do not mark a row accepted merely because its counterpart exists or because an earlier targeted comparison passed.

## Read the actual authorities before proceeding

Read the current private collaboration and release policy authorities in full through the already-authorized private access path. Keep private policy text, repository identity, URL, and private prerequisites out of this public repository and public report.

Read these project files in full at the exact current candidate revision, not from an old chat excerpt:

- \`docs/design/V4_2_0_RC1_TASK.md\`
- \`docs/design/V4_2_0_RC1_REVIEW_REPAIR.md\`
- \`docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md\`
- this continuation file

The task and review-repair documents contain the full technical scope, Preview history pins, detailed guide obligations, quantitative evidence requirements, owner amendment, exact validation requirements and authorization boundaries. They remain authoritative. Do not replace them with a shortened interpretation of this file. Reconcile their instructions with the latest owner messages: the complete package is the single goal; the existing three steps are internal ordering only, not separate execution stops.

## Required work to reach the single completion objective

Complete the full Chinese architecture rewrite already started in the local workspace. Compare every paragraph and every one of its 15 sections with the exact English historical source, including subsections, tables, formulas, diagrams, code examples and all 13 stress walkthroughs. Preserve the English historical original byte-for-byte. Keep the Chinese natural, precise and technically complete; do not leave ordinary explanatory prose as concatenated English fragments. Maintain a consistent glossary for residence/resident records, source markers, revisions, export domains, AVL blocks, snapshots, ownership, and other core concepts. In particular, predecessor/successor links must be translated as 前驱／后继链接 and must not be confused with concurrent threads. Preserve exact identifiers, API names, hashes, formulas, values, units, historical status and evidence qualifications. The recent local terminology corrections are only a starting point: retain them, finish the whole document, then perform a paragraph-level semantic comparison and record actual discrepancies and corrections.

Then finish the complete RC.1 documentation trust reconstruction and cross-document review required by the original task. This includes every in-scope ordinary document, all four complete implementation guides in both languages, the source/API/reconstruction ledgers, the 89-row bilingual inventory, all historical translation companions, candidate and consumer records, performance/evidence pages, charts, captions, examples, and both owner classes. The current checklist says 89/89 have a language counterpart or inline bilingual edition; that proves presence only. Verify the actual meaning, technical depth, reciprocal navigation, numerical/code/command parity, source and test pins, historical attribution, and current-versus-historical wording of every in-scope item. A materially abbreviated, unreadable, inaccurate, or misleading translation is not accepted merely because its file exists.

Use the original task and review-repair requirements to trace the final implementation and all relevant Preview.1, Preview.1.2, Preview.1.2.1, Preview.2 and Preview.3 documentation and Git history. Read actual patches and correction commits, including added, removed, replaced, reverted and rejected behavior. Check the complete final source, relevant callers/callees, public header, private headers, build/export/amalgamation, examples and actual test assertions wherever required by the task. Keep production facts separate from historical proposals and diagnostic-only evidence. Correct every material inconsistency found in the guides, ordinary docs, historical companions, audit ledger, API index, consumer handoff and evidence pages. Preserve historical originals and adverse results.

Preserve the already-verified quantitative evidence and chart design unless a concrete defect is found. The required scatter presentation uses solid circles, squares, triangles and five-pointed stars, with clearly separated colors (especially different colors for the same shape), consistent assignments across languages and panels, readable legends, accurate coordinates, and no hollow markers or numeric marker labels. Preserve all eight variants and all 32 scatter coordinates. Recompute any questioned number from immutable raw evidence; do not launch a new benchmark campaign, alter raw data, invent missing values, combine incomparable accounting scopes, or claim universal superiority. Retain the original limitations and counterexamples.

Complete the durable bilingual checklist and reconstruction audit with actual evidence, not status inflation. Mark a document semantically verified only after its content has been compared with the appropriate source, implementation, history and counterpart. Record the exact reviewed sections, relevant SHAs, discrepancies, repairs, link/anchor results and remaining limitations. Reconcile every finding against both languages and all documents it affects. Repeat targeted or broader review after corrections until no known material mismatch remains. Do not invent endless issues, but do not stop with known defects.

After all Full-owned content is final and the Full SHA is fixed, update the four Repo-owned entry documents on the existing Repo branch: \`README.md\`, \`README.zh-CN.md\`, \`docs/FULL.md\`, and \`docs/FULL.zh-CN.md\`. Replace stale immutable candidate references with the actual final Full revision where appropriate; verify each target exists at that exact revision. Keep main and published Stable correctly described as v4.1.0, and keep RC.1 clearly unmerged, untagged and unpublished. Preserve Mini's independent product/version/download facts. Do not place Full-owned content in the Repo PR or change ownership rules.

Perform the final exact-head verification on both actual draft PR heads after the last relevant change. This includes the required Full push/PR C17 checks, Mini, Workbench and Measurement Evidence, plus Repo C17, Mini, Workbench, Measurement Evidence and the actual main-target Trusted Scope Guard for PR #34. Verify the real event, target and head SHA; a self-test is not a substitute for the trusted Scope Guard. Earlier successful runs apply only to their recorded SHAs. Re-run only checks invalidated by changes, but every required check must apply to the actual final head. Also complete the task-required local strict builds/tests, frozen 64-function API verification, examples, source/install/amalgamation consumers, distribution/release-asset validation, applicable sanitizer and benchmark-build checks, documentation/link/anchor checks, figure reproduction/visual inspection and ownership inventories. Use actual outputs and logs; do not claim a command ran when it did not.

## Git delivery and boundaries

The existing RC.1 task authorizes the necessary commits and pushes to the two existing owner-specific branches only. Preserve both PRs, their bases, all earlier PRs and history, main, Stable tags/releases, source/test baselines, raw evidence, fixtures, frozen contracts, dependency pins, ownership policy, trusted workflows and exception configuration. Make coherent commits and push them to the existing designated branches while continuing the same overall task; checkpoints are recovery points, not completion points.

Do not modify production behavior, public API or persistent formats; do not change Mini/Workbench product-owned files, external repositories, dependencies, governance, rulesets or workflows. No merge, rebase, force-push, retarget, tag, GitHub Release, Latest change, publication, RC.2, new Preview, unrelated work, or unnecessary benchmark campaign. Do not self-award independent PASS. Stop only when the package is ready for independent review, or when a concrete inability to proceed is actually observed. If a real runtime interruption occurs, preserve all work, record the exact state and continue in this same conversation; do not mislabel an ordinary checkpoint as a blocker.

## Final report

Deliver one consolidated English completion report only after the entire package reaches the independent-review checkpoint. Include exact local and remote SHAs, both PR bases/heads, full changed-file inventory, architecture section-by-section semantic review, cross-document findings and repairs, bilingual coverage and verification evidence, final immutable Repo-to-Full targets, actual commands/results, exact-head CI runs, ownership checks, remaining limitations, publication/integration status, workspace state and Technical Debt Impact. Distinguish completed implementation evidence from independent acceptance. Do not claim PASS on behalf of the independent reviewer. After the owner completes the personal review, conduct the separate ChatGPT-plus-owner review. Once both reviews have accepted RC.1, the owner decides whether a successor RC (for example RC.2) is warranted under RELEASE_POLICY §4. If required, execute that authorized candid### 当前准确起点

以下 GitHub 状态是在本次更新续接文件前独立核实的。Codex 开始执行时仍须重新查询两个 PR、分支和 check-runs。下列 SHA 是已核实参考，不代表可以覆盖发生分歧的本地工作区。

- Full PR #33：开放 Draft；分支 `dev/full-v4.2.0-rc.1`；目标 `dev/full-v4.2.0-preview.3`，基线 `168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0`；当前远端 HEAD 为 `b9dc2eba0b91c05aae90f47ddfb6ff8b4de5def4`。PR 当前显示 85 个提交、123 个变更文件。最新提交 `b9dc2eba` 的父提交为 `359bc0285fcc955b9c881affb477363b24a1decc`，说明为 `docs(full): reconcile initial comparison protocol and reproduction`。
- Repo PR #34：开放 Draft；分支 `docs/full-v4.2.0-rc.1-navigation`；目标 `main`，基线 `c108acd72b37614f4be271c9747484c2dde5eac4`；当前远端 HEAD 为 `7114cc06bb02a14cd402c77e0bafd17ccef553f3`。PR 当前显示 8 个提交、12 个变更文件。
- 公开 `main` 仍精确位于 `c108acd72b37614f4be271c9747484c2dde5eac4`。正式 Stable 仍为 v4.1.0：带注释 tag 对象 `bdf29c22a44001d5bf1f988bf06dc2324d5f7512`；现有 GitHub Release 是正式、非预发布的 v4.1.0 [423288672](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。两个 PR 仍分别开放、处于 Draft、没有合并。未授权合并、tag、Release 或发布。
- Codex 最新报告称 Full 工作区干净，且四处原有 Repo 文档修改仍未提交并得到保留。这是实施者报告；本次 GitHub 审查无法直接查看本地工作区。执行开始时必须检查 Full 和 Repo 的真实工作区、remote、分支、本地 HEAD、已暂存／未暂存／未跟踪修改及完整 diff。保留全部修改；不得 reset、clean、覆盖、重建工作区或静默丢弃内容。
- Full 最新提交 `b9dc2eba` 精确修改以下文件：
  - `benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.zh-CN.md`
  - `benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.zh-CN.md`
  - `docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md`
  协议译本修订了追加建立的出现项状态、独立配对参考模型验证、驻留记录和块内存布局等表述；复现译本明确了包注册表依赖、测量测试程序术语、预热轮次排除、进程执行顺序和原生对照范围；清单记录了两段协议及完整复现说明的对照。该提交没有修改历史英文原件。Codex 报告提交前 `git diff --check` 通过。
- 此前 Full 提交：
  - `6f4989ce9b5f2989adcc1f9d8eefd58284c74c78`：Preview.2 reproduction 术语修订及清单更新。
  - `359bc0285fcc955b9c881affb477363b24a1decc`：Preview.3 protocol/reproduction 完整对照及中文 correctness-oracle 表述修正。
  - `cae59fea09a9321be4325a58edde1b21b4a3e5ce`：历史 Preview.1.2、Preview.1.2.1 和 Preview.2 的 protocol/reproduction 术语修订。
  保留这些已完成的对照；没有发现具体新差异时不要重复做。
- Full 精确 HEAD `b9dc2eba0b91c05aae90f47ddfb6ff8b4de5def4` 的托管检查查询结果：
  - Push C17 [45633063490](https://github.com/RXY712200/LayerKeySort/actions/runs/45633063490)：**进行中**；Clang 扩展 sanitizer job 尚在运行。
  - PR C17 [45633078532](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078532)：**进行中**；MSVC x64/Win32、AppleClang、GCC 和 Clang-OFF job 已成功；Clang-ON 扩展 sanitizer campaign 仍在运行。
  - Mini [45633078599](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078599)：成功。
  - Layer Order Workbench CI [45633078538](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078538)：成功。
  - Workbench Measurement Evidence [45633078600](https://github.com/RXY712200/LayerKeySort/actions/runs/45633078600)：上次查询时 workflow 仍在进行；Linux-GCC-Release 和 Windows-MSVC-Release 两个 evidence job 均已成功。
  当时 27 项 check-run 中 24 项完成且成功，另有 3 项仍在进行；没有报告已完成的失败项。后续必须重新检查最终状态，不能沿用此 pending 状态。
- Repo PR #34 的精确 HEAD `7114cc06bb02a14cd402c77e0bafd17ccef553f3` 尚未变化。该 SHA 上可见的必需检查均成功，包括真正针对 main 的 `pull_request_target` Trusted Scope Guard [45587380487](https://github.com/RXY712200/LayerKeySort/actions/runs/45587380487)、Full C17 push/PR [45587374419](https://github.com/RXY712200/LayerKeySort/actions/runs/45587374419) / [45587384140](https://github.com/RXY712200/LayerKeySort/actions/runs/45587384140)、Mini [45587383871](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383871)、Workbench [45587383968](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383968) 和 Measurement Evidence [45587383818](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383818)。Trusted Scope Guard 与 self-test 不同。Repo HEAD 变化后必须重新核验。
- 双语清单仍为 89/89 项已有对应译本或内嵌双语版本。这只证明语言版本存在，不代表语义验收。清单中明确等待实质收尾的项目包括：
  - `docs/design/V4_ARCHITECTURE.md`：对照历史英文原文及相关实现／历史，最终语义验收全部 15 节和 13 个 stress walkthrough。
  - `docs/VALIDATION.md` 及中文版本：补齐最终 RC 验证语境和最终 PR 证据。
  - Repo `README.md` / `README.zh-CN.md` 与 `docs/FULL.md` / `docs/FULL.zh-CN.md`：完成入口页剩余正文审查，并固定指向选定 Full HEAD 的最终语言专属不可变链接。
  - 内嵌的人类可读分发说明，以及任何尚未完成语义接受的范围内正文。
  全语料语义审查、审计记录更新、最终 Repo 导航固定链接及最终精确 HEAD 验证仍是强制要求。不得仅因译本存在或之前通过定向对照就将条目标记为验收通过。

380487](https://github.com/RXY712200/LayerKeySort/actions/runs/45587380487)、C17 push/PR [45587374419](https://github.com/RXY712200/LayerKeySort/actions/runs/45587374419) / [45587384140](https://github.com/RXY712200/LayerKeySort/actions/runs/45587384140)、Mini [45587383871](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383871)、Workbench [45587383968](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383968) 和 Measurement Evidence [45587383818](https://github.com/RXY712200/LayerKeySort/actions/runs/45587383818)。Repo HEAD 发生变化后必须重新核验；policy self-test 不能代替真实 trusted gate。
- 双语清单有 89 行，目前 89 项均已有对应译本或内嵌双语版本。这只证明文件存在，不代表语义验收。剩余语义审查、完整指南与源码／历史核对、Repo 入口页最终导航固定链接，以及最终精确 HEAD 验证仍是必需工作。不得仅凭文件存在或之前检查过哈希／链接就标记验收通过。

开始修改前检查两个 remote、分支、本地 HEAD、状态和完整 diff。保留所有既有本地修改；任何快进前都应安全保存并协调既有修改。绝不 force、reset、clean 或覆盖。上述远端状态不能证明本地工作区干净。

### 必须先读的权威资料

通过已授权的私有访问方式完整阅读当前协作与发布政策权威文件，尤其是完整工作流程和发布政策。不得把私有政策正文、仓库身份、URL 或私有前置条件复制到公开仓库或公开报告。

在当前候选精确版本上完整阅读以下项目文件，而不是依赖旧聊天摘要：

- \`docs/design/V4_2_0_RC1_TASK.md\`
- \`docs/design/V4_2_0_RC1_REVIEW_REPAIR.md\`
- \`docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md\`
- 本续接文件

这些文件包含完整技术范围、Preview 历史基线、详细指南义务、定量证据要求、所有者双语补充和精确验证要求，仍是权威依据。不要用本文件的简写替代它们。结合用户最新要求：完整任务是唯一目标，三个步骤只是内部顺序，不能分别作为结束点。

### 必须完成的全部工作

完成本地工作区已经开始的架构中文全文修订。对照精确英文历史原文逐段检查全部15节，包括小节、表格、公式、图示、代码示例和13个压力场景。英文历史原件必须逐字节保留。中文必须自然、准确、技术内容完整，不能把普通解释写成拼接的英文短语。统一 residence/resident record、source marker、revision、export domain、AVL block、snapshot、ownership 等术语。前驱／后继链接必须明确翻译为“前驱／后继链接”，不得误译为并发线程。保留准确标识符、API名、哈希、公式、数值、单位、历史状态和证据限定。最近的局部术语修正只是起点：保留它们，完成全文后进行逐段语义对照，并记录真实差异及修正。

随后完成原任务要求的全部 RC.1 文档信任重建和跨文档审查，覆盖所有范围内普通文档、四份完整中英双语实现指南、源码/API/重建台账、89项双语清单、历史译本、候选及消费者记录、性能证据页、图表、图注、示例和两个归属范围。当前清单的89/89只证明译本或内嵌双语存在，不代表语义验收。逐项核验实际含义、技术深度、双向导航、数字/代码/命令一致性、源码和测试版本、历史归因，以及现行与历史表述。内容被实质性缩写、难以阅读、错误或误导，即使文件存在也不能验收。

依据原任务和独立修正要求，追溯实际实现及 Preview.1、Preview.1.2、Preview.1.2.1、Preview.2、Preview.3 的相关文档与 Git 历史。阅读实际补丁和更正提交，覆盖新增、删除、替代、回滚及未采用行为。按任务要求检查完整最终源码、相关调用者/被调用者、公有头、私有头、构建/导出/amalgamation、示例和真实测试断言。区分生产事实、历史提案和仅用于诊断的证据。修复指南、普通文档、历史译本、审计台账、API索引、消费者交接和证据页中发现的所有实质矛盾。保留历史原件和负面结果。

保留已经验证的定量证据和图表设计，除非发现具体缺陷。散点图必须使用实心圆、方形、三角形和五角星；颜色明显区分，尤其同一形状必须有不同颜色；中英文及各图的映射一致；图例清晰、坐标准确；不使用空心标记或数字编号。保留八种实现对照及32个坐标。需要复核数字时从不可变原始证据重新计算；不得启动新基准测试活动、修改原始数据、编造缺失值、混合不可比的统计口径或声称普遍领先。保留原有局限与反例。

完善双语清单和重建审计，必须记录真实证据，不得虚增状态。只有在实际对照对应源码、实现、历史和另一语言版本后，才能标记语义验收。记录精确审查章节、相关SHA、差异、修复、链接/锚点结果和剩余限制。将每项发现同步核对其影响到的两种语言和所有相关文档。修复后重复定向或扩大审查，直至没有已知实质不一致。不得制造无限问题，也不得带着已知缺陷停止。

只有在 Full 全部内容完成并固定最终 SHA 后，才更新现有 Repo 分支中的四份 Repo 归属入口文档：\`README.md\`、\`README.zh-CN.md\`、\`docs/FULL.md\`、\`docs/FULL.zh-CN.md\`。把过时的不可变候选引用更新为实际最终 Full revision，并验证该 revision 下的每个目标都存在。准确说明 main 和正式 Stable 仍为 v4.1.0，RC.1 尚未合并、打标签或发布。保留 Mini 独立的产品/版本/下载事实。不得把 Full 文档放入 Repo PR，也不得修改归属规则。

所有最后修改完成后，针对两个 PR 的真实最终 HEAD 执行精确验证。包括 Full push/PR C17、Mini、Workbench、Measurement Evidence；以及 Repo C17、Mini、Workbench、Measurement Evidence 和 PR #34 的真实 main-target Trusted Scope Guard。核对真实事件、目标和 HEAD SHA；自测不能替代 Trusted Scope Guard。旧 SHA 的成功检查只对旧 SHA 有效。只重跑因变更而失效的检查，但所有必需检查必须覆盖最终 HEAD。还须完成原任务要求的本地严格构建/测试、冻结64函数 API 验证、示例、source/install/amalgamation 消费者、distribution/release-asset 验证、适用 sanitizer 与 benchmark build、文档链接/锚点、图表重现及目视检查、归属清单。只报告实际命令和结果，不得声称未执行的检查已经运行。

### Git交付与边界

原 RC.1 任务已经授权仅向两个现有归属分支执行必要的 commit 和 push。保留两个 PR、它们的基分支、所有早期 PR 和历史、main、Stable tag/release、源码/测试基线、原始证据、fixtures、冻结契约、依赖 pin、归属策略、可信工作流和例外配置。按实际需要做成组提交并推送到指定的现有分支；这些检查点只是恢复点，不代表任务完成。

不得修改生产行为、公有 API 或持久格式；不得修改 Mini/Workbench 产品自有文件、外部仓库、依赖、治理、ruleset 或工作流。不合并、不变基、不强推、不改 PR 目标、不打 tag、不建 GitHub Release、不改 Latest、不发布、不创建 RC.2 或新 Preview、不做无关工作或无必要基准测试。不得自行宣布独立 PASS。只有在整个包达到可独立审查状态，或确实观察到无法继续的具体障碍时才停止。若运行环境真实中断，保存全部工作，记录精确状态并在同一对话继续；不得把普通检查点冒充阻断。

### 最终报告

只有全部包达到独立审查检查点后，才提交一份完整英文报告。包括准确本地/远端 SHA、两个 PR 的基线/HEAD、完整变更文件清单、架构逐节语义审查、跨文档发现与修复、双语覆盖及验证证据、最终 Repo→Full 不可变链接、真实命令/结果、精确 HEAD 的 CI、归属检查、剩余限制、发布/集成状态、工作区状态及 Technical Debt Impact。区分实施证据和独立接受，不得代替独立审查者宣布 PASS。用户完成个人审核后，再由 ChatGPT 与用户进行单独的联合审核。两轮审核均接受 RC.1 后，由用户依据 RELEASE_POLICY §4 决定是否需要后继 RC（例如 RC.2）。若需要，则执行获准的后继候选，并重复用户审核、联合审核和后继候选判断，直至 Stable；若不需要，也必须由用户明确授权后才能走向 Stable。Stable 完成并获得用户授权后，再准备 demo 版本。只有 demo 检查点完成且用户再次授权，才能启动工业级横向对照研究。已有持久计划为 `POST_V4_2_0_STABLE_INDUSTRIAL_COMPARISON.md`（记录提交 `9f0c6cd`）；届时必须读取该文件的准确当前版本，并使用独立研究分支。RC 收尾期间及 Stable 之前不得启动 GLib/Boost/LLVM 对照。
