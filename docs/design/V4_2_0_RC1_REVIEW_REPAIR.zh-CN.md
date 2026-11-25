# RC.1 独立审查：修复现有三步工作包（历史任务中文译本）

原文：[V4_2_0_RC1_REVIEW_REPAIR.md](V4_2_0_RC1_REVIEW_REPAIR.md)，英文，固定于 [`ffe38170220192886faa4a0665ba7b7540d9a90c`](https://github.com/RXY712200/LayerKeySort/blob/ffe38170220192886faa4a0665ba7b7540d9a90c/docs/design/V4_2_0_RC1_REVIEW_REPAIR.md)。历史原文保留；本译本为完整说明性对应版，英文执行指令仍为权威，不新增或重启授权。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

对 Full PR #33（`643d8f79631ef8f13a6318afb8ddc02d858bf4e7`）和 Repo PR #34（`1c823fe7aac9c1e6098e4fc7adc83873561caeb6`）的独立审查，整体为 FAIL，原因是文档不完整，并非生产回归。保留已接受的运行源码 `fe00536e7bd7f4e5128432ea4a29e54158074903`、RC 元数据/测试基线 `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`、历史数据和 fixtures。上述受审 HEAD 的实际托管检查已通过，包括 #34 真正面向 main 的 Scope Guard。完整阅读原 V4_2_0_RC1_TASK.md 和现行私有 owner 政策。本文件是原任务的审查修正指令，不是新开发阶段。一次连续会话内，先完成 Step 1 修正，再 Step 2，最后 Step 3；分别提交，最终只交一份英文报告，不作例行交接。

## Step 1 — 普通文档及定量展示修复

受审 #34 的 diff 每文件仅新增三行，基本重复候选通知，没有实质替换入门章节。owner 明确要求替换过时说明并呈现有意义的性能证据。保持 main/公开 Stable 信息准确，不把未合并、未打标签的候选提升为正式版本。在两种根 README 和两个 Full 入口页既有结构内改进：简洁区分版本、当前 Stable 与单独固定的 RC 机制/选择/验证、可读的匹配比较表（总体及 compare-heavy，含百分比和范围）、Full 入口页上的实际图表及不可变候选链接、标明分支适用性的当前详细指南导航、实际约束、稳定下载路径。保留 Mini 的独立事实；根 README 不扩张成 benchmark 报告。移除重复或过时的候选说明，不移除有效 Stable 事实。指向新增 RC 文件的链接不得假装文件已在 main；Full 工作结束后重新固定到最终 Full commit，区分公开 Stable 和候选指南链接。不修改 Mini 或共享治理。

在 Full 分支审查实际正文和章节衔接，不能只追加注释。受审 docs/BENCHMARKS.md 第9–13行的具体问题：原句 “historical V4-convergence evidence. The latest independently accepted three-step” 被插入的候选段打断，后面才接 “Full 4.1.0 research covers ...”。修复段落，明确所有 V3 材料是历史；V3 复杂度章节的 “current implementation” 和 “Current structural work” 不得与当前 Full 混淆。WORKLOAD_EVIDENCE.md 中旧3.2研究建议和 “No breaking V4 is justified” 是历史结论，应标明语境，不改原观察。检查所有现行普通文档，记录实际审过的章节和保留不变的理由，保持正式 Stable 链接。扩展 CHANGELOG.md 当前候选条目，记录累计实际采用的生产改进和兼容性，而非仅链接继承实现；保留旧条目，不编新功能。

受审 generate.py 的散点循环限制为 ('full','tag','list_range')，但所选两主机100k、seed-17、compare-heavy/update-heavy 四组中，八种变体均有匹配正式计时和诊断 requested peak 行。审查者已独立重算。例如 compare-heavy 的 peak bytes / ms：

| 变体 | Windows | Linux |
| --- | ---: | ---: |
| full | 4278776 / 0.6021 | 4278776 / 1.127721 |
| stable40 | 5028760 / 3.2931 | 5028760 / 6.663075 |
| stable41 | 4253752 / 3.2323 | 4253752 / 5.580504 |
| preview1（Preview.1.2） | 4278776 / 0.6401 | 4278776 / 1.158808 |
| list | 2400168 / 1664.2743 | 2400168 / 1094.379312 |
| tree | 6400424 / 7.846 | 6400424 / 14.941655 |
| tag | 13437504 / 0.5721 | 13437504 / 1.370782 |
| list_range | 13437504 / 0.7881 | 13437504 / 1.763961 |

必须从原始数据重算，不硬编码这些示例。核算范围可比时，派生散点数据和清晰图表纳入八种变体；否则明确区别核算范围或遗漏。用对数耗时轴，或完整范围概览加放大图；诚实处理重叠点，图例/单位/标记清楚不同，给精确数值表和可访问数据。明确正式计时与诊断分配是独立样本、wrapper/编译器/核算差异，不作通用 Pareto 声明。确定性重现并目视检查全部图；无需新 benchmark 活动。核验历史演进表，给两主机同期 Preview.1 对 Stable 的结果；历史活动与本地 screen 分开，报告可比历史阶段及最终直接比较，不把独立中位数相乘。保留不利情况；全部数字表对照归档、源码固定和计时范围。

## Step 2 — 重建详细指南的可信性

四指南相对 Preview.3 总共仅31行新增、43行删除（不计外部索引）。改动量不是验收标准；原任务要求全文完整、章节内整合正确，审查发现具体反例。阅读全部指南、全部有关 Preview commit diff/report，包括删除或变更的诊断说明；对照完整最终源码和相关测试。不能靠表面补充或列文件解决。有效内容可留；替换过时现行描述，将散落笔记放入逻辑章节，移除重复开篇，保留证据区分。核对精确函数名、源引用和实际分支/控制流。

以下为最低修正，非穷尽清单：

- IMPLEMENTATION_INTERNALS.zh-CN.md §3：helper 表缺 v4_tag_attach，v4_link_after 行未说它在 v4_balance 前调用。补足外界 sentinel/非零检查、width=1 与 width>1 的 stride 接受条件、确定性左右扩张规则、64-block 上限及失败行为。将从 Preview.3 开篇删除的 tag_writes、tag_repairs、tag_fallbacks、max_tag_writes 定义、更新方式及范围放回正确正文。解释实际单调性诊断检查，以及 parent/height fallback 分支和符号。§15 比较错误排查错误优先 subtree_blocks，当前 compare 从不读取它：先查 owner/slot/tag 顺序/零标签路径/parent/height，并区别结构验证。按 order_bulk.c 更新 bulk spacing 公式；区分实测主机字节宽度/布局与公共保证。给源码支持的序列/分支解释，非改写旧摘要。
- IMPLEMENTATION_VALIDATION.zh-CN.md §3.1受审第85行称本轮未运行 CTest，§9仍只说 Preview.3 证据；Walkthrough §12/13保留“本轮没有完成...重新运行 CTest”，§14受审第471行称PR #24未合并，但 GitHub 实际合并于2026-11-23 06:02:54 UTC。修复过时现行断言。保留当时阅读断言与执行套件的有效区分，但明确 RC 实际含测试 checkout、本地/托管执行、已有精确 run links，以及未独立重现部分。当前验证指南应独立可用，不要求从别的阶段推测结果；将每个优化对应实际测试断言，包括同 revision 快照独立、offset-one wire、tag zero/exhaustion、restored mixed state、实际到达 OOM 和旧冻结 fixtures。
- Walkthrough §10目前给4.1.0历史回归，却缺4.2实测和选择边界；融入当前结构成本、compare dispatch 和链接定量页的证据。解释有界优化为何非普遍改善；Full 总结须含标签/fallback，而非仅4.2前结构。
- API reference：保留实际64函数/14类型冻结和源码索引，对照源码核验 compare 验证次序、精确声明、每函数参数/错误/输出/所有权/生命周期。移除重复/过时阶段措辞，审全文而非仅改动行。
- 重建 ledger 超出通用模块分组：逐义务对应 source/function/section/commit/test/assertion/evidence/limit，按时间覆盖全部有关 Preview 和修正 commits，标明删除/替换/从未采用及理由，明确检查过而不改的章节。不虚构覆盖；协调普通文档、消费者交接、source manifest 和四指南。未读/未测不得声称已做。修复审计发现的其他差异，不限以上例子。

## Step 3 — 复核及精确 HEAD 验证

重复原A/B/C审查循环，记录真实发现、修正提交和最终复核。重新检查全文及跨文档链接，发现矛盾、断句、过时状态、遗漏技术义务、范围误导和图表可读性问题；发现实际问题就修复并复查，不能只因达到固定轮次而停。记录实际审过的历史、章节、证据、剩余限制；不扩研究或重设计生产，不作无依据的形式化完整性声明。

只保留现有两份 draft PR #33/#34，base 不变。Full 分支包含本审查任务提交，获取精确 SHA；Repo 分支起于 `1c823fe7aac9c1e6098e4fc7adc83873561caeb6`。只提交推送各自归属变更；保留历史、main、旧PR、dependency pins、assets、evidence。检查新精确 HEAD 的 Full push/PR、Mini、Workbench、measurement，#34真正面向main的 Scope Guard，以及其他适用检查。按实际结果重跑适当本地 API/links/examples/figure reproduction/asset verification；保留既有严格本地测试和资产收据的精确 SHA 来源，未重跑不能称重跑。最终共享页指向最终 Full head。不得 merge/rebase/retarget/tag/release/RC.2/new Preview/new benchmark campaign，不改workflow/ownership exception。停于独立审核；一份英文总报告，含精确本地远端HEAD、三步修复证据、Technical Debt Impact、未验证边界、候选与集成区分。

## owner 强制补充 — 完整中英文文档对等

owner 要求所有文档同时有简体中文和英文。这是现有 RC.1 三步包的强制验收，非第四步或新RC；覆盖先前只需部分入口双语或称四详细指南仅中文的说法。

- **范围：** 盘点 Full 全部人类可读文档和两种授权归属内共享文档，包括普通指南、READMEs、开发/API/架构/使用/集成/兼容/迁移/冻结/验证/benchmark/workload/选择、交接、changelog、示例/分发说明、四份完整实现指南、源码审计/API索引、候选记录、设计/历史阶段/研究报告，以及研究归档中的人类可读文档；非Markdown（如共享HTML导航）也纳入，不因原先单语而遗漏。Mini、Workbench自身产品文档和其他仓库不在本次授权，共享页保留独立产品事实。raw data/source code/machine manifests/test fixtures/binary receipts/third-party licenses/精确原始证据不当解释性正文翻译改动；保留法律权威license，必要译释标非权威，原件完整。
- **文件/权威：** 允许时用既有英文名加 .zh-CN.md；中文详细指南建完整英文对应版，不重命名旧中文文件，保既有名字和入链。共享路径新增对应版若不在trusted classifier，使用既有文件内清楚组织的双语或归属正确的链接同伴，不改ownership或混归属。历史文档/原研究收据必须字节不变，新增明确标记的忠实译本，给精确原路径/revision/原语言/历史状态，而非重写原件。历史人类可读任务译本为说明性，Playbook 下原英文执行指令仍权威。私有协作政策保持私有；Codex完成报告和可执行任务指令依政策保持英文。
- **Step 1：** 在详细指南重建前，两个归属内普通文档更新并完整双语，包括证据表/图注/图解释/导航。可共享底层图数据，但两种语言读者都能理解轴/单位/图例/范围/限制；必要时给语言特定图，不复制改动原测量。
- **Step 2：** 依据实际历史/源码重建四指南，四份全部完整双语，非英文摘要、部分翻译或以另一语言链接替代。翻译相关技术正文/标题/表/图/适用的解释性代码注释/排障/引用/限制/证据边界；保留代码、标识符、精确数值、单位、SHA、命令和契约语义。两种读者有同等技术深度与复现能力。双语 source audit/coverage ledger/API index；完成剩余历史译本，保原件。
- **Step 3：** 在授权分支发布覆盖清单，每原文列 owner/status/中英文路径或双语章节/source-evidence baseline/翻译完整性/双向导航/有据非文档排除。检查全部当前正文及译文的事实语义/术语/代码数字/版本源码测试pins/链接锚点/章节覆盖/图可读性；两语实际差异均修复并复查，直至无已知实质差异。缺语言或实质缩写译本是 FAIL，非可选任务。保历史；真正未完成范围须明确，不宣称全双语。仅现有#33/#34、原三步、一连续会话、一英文报告；核验两个新精确HEAD gates。不合并/发布/新RC，不改未授权其他产品。
