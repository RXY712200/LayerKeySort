# Full v4.2.0 — deferred industrial order-maintenance comparison

**Status: PLANNED / DEFERRED — NOT AUTHORIZED FOR EXPERIMENT EXECUTION.**

Recorded: 2026-11-25. Owner request: preserve a detailed research plan in the repository so later release work and conversation changes cannot erase it. English is the working specification; the Chinese edition below preserves the same substantive decisions.

## 1. Activation and boundaries

Complete RC.1 first, then let the owner decide whether RC.2 or further RCs are justified, and complete the separately authorized Stable release process. **Only after v4.2.0 Stable is published and the owner explicitly authorizes this research may experimental implementation begin.** Read the actual release tag, peeled production commit, release assets and current main; record exact identities. A provisional RC SHA is not the study baseline. Additional RCs do not cancel this plan.

This record is stored separately from the current release candidates. At recording time main was c108acd72b37614f4be271c9747484c2dde5eac4 and Full RC.1 PR #33 was at 123c1c69558309d840f68598a2f64066433c9999 (not an accepted final candidate). These are historical context only. The eventual research branch must originate from the published Stable source or a verified compatible checkout, with this plan carried forward without changing release history. The plan itself grants no future implementation, dependency, merge, tag or publication permission.

Full only. No change to production sources, public API, LS1/LKS4SNP1 formats, frozen contracts, release assets, Mini, Workbench, downstream applications, governance, or required CI. Research-only adapters, dependencies, scripts, fixtures and documentation belong in an isolated Full research tree, proposed location benchmarks/research/full-v4.2.0-industrial-comparison/. Verify ownership, dependency license and storage constraints before writing. If credible production defects emerge, record separately with reproducer and severity; do not fix or optimize the Stable implementation under this study.

## 2. Questions and prior evidence

Purpose: strengthen external validity and industrial representativeness for future international software-engineering research, potentially a high-ranking journal. This is not a promise of publication or a predetermined speedup.

- RQ1: Under equivalent dynamic-order operations, how do LKS and established libraries differ in elapsed time, tail latency and memory?
- RQ2: How do operation mix, sequence size, churn, and order-comparison frequency change performance and resource trade-offs? Where are crossover regions and disadvantages?
- RQ3: What capabilities and additional integration costs distinguish native support from application-supplied composition (stable occurrence identity, comparator management, snapshots, serialization, restoration and failure guarantees)?
- RQ4: What transferable software-engineering insights about stable identity, order labels, fallback structures, locality, memory layout and lifecycle integration are supported by the measurements, source inspection and actual use cases?

Reuse the independently accepted Preview.3 evidence at 168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0 (benchmarks/research/full-v4.2.0-preview.3/), including matched Windows/Linux live-order and lifecycle matrices, native tag-range and list-range OM, linked list and threaded Treap, rejected alternatives, application measurements and adverse cases. See the pinned [audit inventory](https://github.com/RXY712200/LayerKeySort/blob/168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0/benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md). These are historical measurements, not automatic proof of the eventual Stable version's performance. Check equivalence of source, flags, workload, environment and accounting; reuse compatible evidence as history, and rerun matched controls only where needed for a valid new comparison. Preserve existing raw evidence and never silently pool incompatible measurements.

## 3. Industrial controls and priorities

**P0 — GLib GSequence (mandatory primary industrial control).** Start with the native C sequence and inspect current stable source and iterator contract. Evaluate stable iterators for surviving elements, insert-before, removal, move, position/iterator order comparison, traversal, sorted insertion and sort where semantically equivalent. Verify how iterator comparison is implemented, actual balance/complexity guarantees, end-iterator behavior, ownership/destruction callbacks and allocation policy. Use native functions rather than intentionally slow emulation. Cross-sequence movement is a distinct capability, not a required common operation.

**P1 — Boost.MultiIndex.** Inspect sequenced, random_access, ordered (including ordered_non_unique where appropriate) indexes and meaningful combinations. Compare separately: a minimal sequence configuration, a native random-access configuration, and a justified multi-index composition. Use relocate/rearrange or equivalent native operations where supported. An ordered key index is not the same as mutable positional precedence; do not conflate the two or require unnecessary secondary indexes. For comparator-managed workloads, preserve equal-key stability/duplicate and rekey semantics with explicit adapter cost.

**P1 — Boost.Container stable_vector.** Verify stable references/iterators for surviving elements, random-access position and order, native insert/erase, and practical relocation/composition strategies. An erase/reinsert that invalidates the moved occurrence's identity is not an equivalent stable move; either use a valid native/composed strategy with measured cost or mark that operation non-equivalent. Do not compare unsupported semantics as a performance loss.

**P2 — LLVM Instruction Ordering.** Mechanism and targeted experiments only, after the general-purpose comparisons. Verify the actual stable LLVM source for Instruction::comesBefore, BasicBlock ordering, invalidation, numbering/recomputation, insertion/movement, and same-basic-block preconditions. Compare only equivalent instruction-order scenarios, including valid versus invalidated ordering and clustered edits where meaningful. Distinguish native IR object/allocator cost from ordering-specific incremental cost. Do not represent LLVM as a general sequence/snapshot library or compare whole IR memory directly against a lightweight LKS container as if they were equivalent.

**Other controls:** admit at most one or two additional mature implementations only if source, license, semantics and a distinct research question justify them. No endless search or baseline expansion. Document rejected candidates and why. No obligation to add an object merely because it is popular.

Upstream starting points (verify current links, releases and exact source at activation): [GLib GSequence](https://docs.gtk.org/glib/struct.Sequence.html), [Boost.Container](https://www.boost.org/libs/container/), [Boost.MultiIndex](https://www.boost.org/libs/multi_index/), [LLVM](https://github.com/llvm/llvm-project). Use latest suitable released stable versions available at study start; record full commits/tags, package versions, patches, checksums, license, compiler, language standard, build flags and dependency configuration. If a platform cannot support that release, document the reason and report the comparison scope rather than selecting an older unfavorable version silently.

## 4. Semantic equivalence and experimental design

Freeze protocol and primary endpoints before inspecting comparative results. Establish a per-operation matrix: native equivalent / composed equivalent / different semantics / unavailable / not evaluated. Include reference stability, duplicate occurrences, same-container comparison, insertion by existing handle, deletion, relocation with identity retention, traversal, comparator management and stable equal-key ordering, bulk creation/sort, independent historical snapshots, byte serialization, restore with fresh handles, ownership, invalidation, concurrency and failure guarantees. State LKS limitations too; snapshots are not a durable transaction engine, borrowed payload identity and application persistence remain application responsibilities.

Use identical seeded traces, live occurrence IDs, operation counts, initial/target sizes and expected state. Maintain an independent sequence oracle; check full order, compare sign, surviving identity, stable duplicates, moves, deletion and callback/restore behavior as applicable. Hash traces and outputs; reject or classify semantic mismatches before performance interpretation. Preallocate adapter bookkeeping and payloads consistently when measuring container operations; measure any necessary index/handle mapping separately and in end-to-end costs. Preserve actual native ownership/allocation policy; record differences rather than imposing artificially inefficient common internals.

Three distinct comparison layers:
1. Native common operations: compare insert, remove, move, compare, iterate and applicable managed insertion under exactly shared semantics. No charge for unused optional features.
2. Equivalent feature composition: separately add only the missing capabilities required by a defined application task; include adapter/index/snapshot/serialization/restore and integration costs. Specify bytes, identity, ordering, failure semantics and format compatibility. Do not require competitors to implement the LKS-specific wire format unless the research question truly requires it.
3. Domain/application experiments: actual realistic operation traces or small representative applications (including existing downstream evidence where permission and provenance permit). Separate application overhead from container kernels and respect LLVM domain restrictions.

Proposed bounded design: pilot sizes 1k, 10k, 100k; extend to 1M and long churn only for justified scaling questions and feasible controls. Match existing compare-heavy/update-heavy/mixed traces, endpoint-heavy and arbitrary-relative edits, traversal, managed duplicate-key operations, growth/shrink and concentrated insertions. Include low/high comparison ratios, predeclared seeds, fixed workload lengths and held-out scenarios if used to test generality. Include invalidation/relabel bursts and restore/lifecycle separately, not as forced common operations. Pilot runtime and memory feasibility; predeclare any timeouts, exclusions, size limits and stopping rules. Preserve failed/timeout observations. Avoid quadratic full matrices for expensive controls when a documented matched subset answers the question. Final sample sizes and resource budget require owner approval in the activation task.

Run matched experiments on the same hardware and OS for each comparison; preferably Windows and Linux, distinguish native toolchains/allocators and C versus C++. Record CPU, RAM, OS/kernel, compiler, optimization, build type, architecture, allocator, thread/affinity policy, frequency/thermal effects, background load, warmup and run order. Randomize or interleave paired runs. Use identical timer boundaries and separate setup, oracle, fixture preparation, allocation instrumentation, I/O and application work; report wall and operation time as different estimands. Record warm/cold state; never subtract an unmeasured cost. Start with existing seven formal repeats where appropriate, adjust via pilot precision/variance and predeclared budget; no cherry-picking or stopping because LKS wins.

## 5. Metrics, analysis and reproducibility

Time: per-run elapsed, throughput, mean, median, min/max, variability (SD/IQR or CV as appropriate), paired ratios, effect sizes and uncertainty intervals; retain individual runs. For operation latency, use a calibrated sampling scheme and sufficient event count; report p50/p95/p99 and p99.9 only where supported, max and sampled-event counts, including relabel/invalidation tails. Separate latency instrumentation from production timing if intrusive. Use paired run/trace-level resampling or justified alternatives; do not treat dependent individual operations as independent replicates. Report host/profile/size strata, uncertainty and crossover regions. Do not average ratios misleadingly; specify weighting and geometric means where suitable. No universal superiority claim from a single aggregate.

Memory: define baseline and observation window, logical live bytes, time-weighted or explicitly sampled mean/median, peak requested allocation bytes, retained/live bytes, per-live-element incremental cost, allocation/free counts, and process RSS/peak RSS (or documented platform equivalent). Distinguish requested bytes, allocator overhead, reserved capacity, process footprint, payload and adapter structures; do not equate requested allocation with RSS. Use comparable instrumentation where feasible and label unavailable metrics with reasons. Consider steady-state, grow/shrink and churn. Record frequency of sampling and any instrumentation overhead.

Feature matrix and integration cost: cite released docs/source and concrete tested behavior; distinguish native, composed, unavailable and untested, not pass/fail by feature count. Report adapter LOC/complexity only as descriptive, dependency/build and ownership burden, error/cleanup behavior, test burden and application-defined persistence guarantees; avoid unsupported productivity claims. Compare equivalent end-to-end workloads only. Retain all favorable and unfavorable results, assumptions, outliers, timeouts, contradictory cases and threats to validity. If a metric cannot be measured fairly, explain and omit from cross-control ranking.

Archive: protocol and amendment log; source/version/license/patch manifest; workload generator, trace seeds/hashes, oracle, adapters and tests; build and reproduction commands; exact source and environment manifests; raw per-run and diagnostic data, measurement definitions, failure/timeout logs; analysis scripts; derived tables/figures (including memory-time scatter, crossover and tail views as justified), bilingual feature matrix, bilingual findings and limitations, and acceptance report with immutable evidence references. Keep heavy binaries and dependencies out of normal Git history; preserve reproducible retrieval/checksums and obtain permission before external hosting. Follow project ownership and third-party license rules. Existing evidence is immutable.

## 6. Fixed proposed execution sequence (four stages, two steps each)

This is a deferred plan, not current authorization. On activation confirm the fixed scope, version pins, feasible platform coverage and compute/usage budget once. Normal corrections remain in their original step; substantial scope expansion needs owner approval.

| Stage | Step 1 | Step 2 | Acceptance gate |
| --- | --- | --- | --- |
| A — protocol and feasibility | A1: pin Stable and upstream releases; source/contract/license survey, shortlist and equivalence matrix | A2: freeze workloads, measurement definitions, oracle, pilot limits and statistical plan | Reviewable protocol, source pins, resource envelope and semantics; no measured superiority claim |
| B — primary industrial evidence | B1: implement and validate GSequence and matched LKS adapters with oracle and identity/failure checks | B2: run predeclared feasible GSequence matrix; record raw results and initial analysis | Reproducible matched measurements, correctness first, adverse cases preserved |
| C — broader evidence | C1: validate and measure justified Boost configurations | C2: LLVM targeted mechanism comparison and selected lifecycle/application composition; document any scope exclusion | Comparable native/composed/domain evidence separated; additional controls justified; no production change |
| D — synthesis and archive | D1: consolidate data, uncertainty, crossover/resource and capability analysis, threats to validity and reproducibility | D2: independent evidence audit and bilingual final archive; owner reviews research acceptance | All agreed deliverables and exact evidence links complete; no unsupported conclusion or implied publication |

Do not start Stage B before Stage A acceptance or expand later stages autonomously. If Boost/LLVM build or semantic feasibility is genuinely blocked, record exact evidence and proposed narrowing for owner decision; do not substitute a fake comparison. Finish authorized independent work within the same scope. No automatic product optimization or version bump after research.

## 7. Execution efficiency and handoff

Use ordinary Codex instructions, not /goal (owner explicitly rejected it after excessive quota use). Divide approved stages into finite coherent execution batches with objective stop points; do not stop after arbitrary subsections inside a batch. Keep a single durable checklist with scope, exact revisions, evidence, outstanding work and stop reason. Reuse unchanged valid source inspections, prior calculations, fixtures, builds and test results. Recompute or rerun only where evidence is invalidated, the matched new experiment requires it, or a concrete discrepancy warrants it. Final required validation applies to the actual deliverable head; distinguish new measurements from historical receipts. Avoid commit-per-paragraph, needless CI triggers, repeated corpus-wide review and repeated planning. Batch related changes. Report actual completed work, genuine blockers and remaining work; no self-awarded independent PASS. ChatGPT independently checks actual remote diff, raw evidence, methodology and results; the owner decides acceptance and any next authorization.

When a new conversation takes over: read this record, current release/tag/branch state, current authoritative project documentation and applicable collaboration policy. Do not infer a future research authorization from this plan. During RC.1/RC.2/Stable work, leave this plan deferred. After Stable, surface it to the owner before forming the activation task.

---

# 中文版：Full v4.2.0 正式版发布后的成熟工业级顺序维护横向研究

**状态：已规划、延期执行；尚未授权开展实验。** 记录于 2026-11-25。此文档用于防止后续 RC、Stable 工作及会话迁移造成研究意图丢失。

## 1. 启动条件与边界

先完成 RC.1，再由用户决定是否需要 RC.2 或进一步 RC，按单独授权完成 Stable 正式发布。**必须等 v4.2.0 Stable 已实际发布且用户明确授权启动本研究后，才实施实验。** 启动时核实正式 tag 解引用后的生产提交、发布资产及 main；不得以临时 RC 作为最终实验基线。增加 RC 不取消本计划。

本记录独立于候选版分支。记录时 main 为 c108acd72b37614f4be271c9747484c2dde5eac4，Full RC.1 PR #33 为 123c1c69558309d840f68598a2f64066433c9999，尚未最终验收；仅是历史背景。正式研究从已发布 Stable 或经验证兼容的检出建立独立研究分支，携带此计划，不改发布历史。本文件不授予未来实施、依赖变更、合并、打标签或发布权限。

只研究 Full；不改生产源码、公有 API、LS1/LKS4SNP1、冻结契约、发布资产、Mini、Workbench、下游应用、治理及必需 CI。实验适配器、依赖、脚本、测试数据与文档放在独立 Full 研究目录，建议 benchmarks/research/full-v4.2.0-industrial-comparison/，写入前核归属、许可和存储约束。若发现可信生产缺陷，单独记录复现、严重性与证据，不在本研究中擅自优化或修复正式版。

## 2. 研究问题与既有证据

目的为增强未来国际软件工程论文的外部有效性与工业代表性，可作为冲击高水平期刊的研究基础，不承诺录用，不预设 LKS 必胜。

- RQ1：等价动态顺序操作下，LKS 与成熟库在时间、尾延迟和内存方面有何差异？
- RQ2：操作比例、规模、长期增删及比较频率如何影响时空取舍？何时出现交叉点及劣势？
- RQ3：稳定实例身份、比较器管理、快照、序列化、恢复、失败保证等原生能力与应用组合能力的综合成本分别是什么？
- RQ4：从稳定身份、顺序标签、回退结构、局部性、内存布局及生命周期集成中，可以提炼哪些有证据支持、可供其他软件工程借鉴的机制与规律？

继承已独立验收的 Preview.3 证据（168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0，benchmarks/research/full-v4.2.0-preview.3/），包括 Windows/Linux 配对实时顺序与生命周期矩阵、Native Tag-range OM、Native List-range OM、传统链表、Threaded Treap、未采用方案、应用测量及负面结果。参见[固定审计清单](https://github.com/RXY712200/LayerKeySort/blob/168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0/benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md)。历史证据不自动证明最终 Stable 性能；核对源码、编译参数、负载、环境及统计口径，仅在新公平配对所需时重跑对照，不混合不可比数据，不改历史原始记录。

## 3. 对照优先级

**P0 — GLib GSequence，必须优先。** 同为 C，核心场景高度重合。核查最新适用正式版源码、存活元素的迭代器稳定性、相对插入、删除、移动、位置/顺序比较、遍历、排序插入及排序；核查实际比较实现、平衡和复杂度边界、尾迭代器、销毁回调与分配策略。使用原生高效操作；跨序列移动单独列能力，不强制纳入公共操作。

**P1 — Boost.MultiIndex。** 研究 sequenced、random_access、ordered（适用时 ordered_non_unique）及有理由的组合。分别比较最小序列配置、原生随机访问配置与必要多索引组合；使用原生 relocate/rearrange 等适用接口。有序键索引不等于可变位置先后关系，不强加无用索引。比较器管理负载需保留重复键、相等键稳定性、改键语义并计入适配成本。

**P1 — Boost.Container stable_vector。** 核查存活元素的引用/迭代器稳定性、随机访问位置和比较、插入删除及移动方案。若 erase/reinsert 使被移动实例原身份失效，不能算等价稳定移动；须用有效原生/组合方案并计成本，或列为不等价。

**P2 — LLVM Instruction Ordering。** 通用库对照完成后再做机制及针对性实验。核查实际正式版 Instruction::comesBefore、BasicBlock 顺序、失效、编号/重新编号、插入移动及同基本块前提。分别观察有效与失效编号、适用的集中编辑等场景。区分 IR 对象/分配器完整成本与顺序维护增量成本；不把 LLVM 当通用序列或快照库，不用整个 IR 对象内存直接压比 LKS 轻量容器。

其他实现最多额外筛选一至两个，必须有不同研究问题、源码、许可与语义依据；记录淘汰理由，不无限扩张。上游入口：[GLib](https://docs.gtk.org/glib/struct.Sequence.html)、[Boost.Container](https://www.boost.org/libs/container/)、[Boost.MultiIndex](https://www.boost.org/libs/multi_index/)、[LLVM](https://github.com/llvm/llvm-project)。启动时重新确认链接与最新适用正式版，固定完整源码提交/tag、包版本、补丁、校验值、许可、编译器、语言标准、构建参数和依赖；平台不支持时说明原因与比较边界，不暗中选用落后版本。

## 4. 等价语义与实验协议

先冻结协议、主要指标再看对比结果。逐操作分类：原生等价、组合等价、语义不同、不支持、未测。覆盖稳定引用、重复实例、同容器比较、句柄相对插入、删除、保身份移动、遍历、比较器管理与相等键稳定性、批建/排序、独立历史快照、字节序列化、恢复的新句柄、所有权、失效、并发及失败保证。也记录 LKS 限制：快照不等于持久事务，业务对象身份、借用对象生命周期与持久化仍由应用负责。

统一随机种子、操作序列、实例 ID、操作量、初始/目标规模及预期状态。独立序列 oracle 校验完整顺序、比较符号、存活身份、重复键稳定性、移动、删除及适用回调/恢复；保存 trace 与结果哈希。语义不匹配先分类或修适配，不直接解释性能。容器测量中一致处理 payload 与适配映射的预分配，必要索引/映射既单独测也计入端到端；保留各库真实所有权/分配方式并披露差异。

分三层比较：①原生公共操作（插删移比遍历、等价管理插入），不计未使用的可选功能；②等价功能组合，在明确应用任务下补足必要能力，计入适配、索引、快照、序列化恢复及集成成本，说明身份、顺序、字节、错误语义；不无理由强制其他库实现 LKS 专有线格式；③真实或代表性应用负载，利用许可和来源允许的已有下游证据，隔离应用开销，LLVM 严守适用域。

拟定规模 1k/10k/100k；仅在扩展性问题需要且对照可行时加入 1M、长期 churn。配对既有比较密集、更新密集、混合负载及端点/任意相对编辑、遍历、重复键管理、增缩、集中插入；预设低高比较比例、种子、长度、留出场景（如需检验泛化）。失效/重标突发及恢复生命周期单独研究。先做资源可行性试验，预设超时、排除、规模上限与停止条件；保留失败超时结果。对昂贵对照用有依据的匹配子集，避免无意义的全笛卡尔积。正式样本规模与资源预算在启动任务由用户确认。

每项对比使用相同硬件与 OS，优先 Windows/Linux 两平台分别分析，记录 CPU、RAM、OS/内核、编译器、优化、构建、架构、分配器、线程/亲和性、频率温度、后台负载、预热与执行顺序；随机化或交错配对。计时边界一致，区分初始化、oracle、fixture、分配诊断、I/O、应用开销、操作耗时与墙钟；记录冷热状态，不扣除未测成本。可从既有七次正式重复设计出发，根据先导方差、精度及预算预设调整，不按 LKS 胜负挑样本或结束。

## 5. 指标、分析与归档

时间：逐次原始耗时、吞吐、平均、中位、最小最大、适用的标准差/IQR/CV、配对比值、效应量与不确定性区间。操作尾延迟采用经校准抽样并保证事件量，报告 p50/p95/p99，有充分数据才报 p99.9，另报最大值与抽样数，包含重标/失效突发。侵入式延迟诊断与正式计时分开。以配对运行/trace 层级重抽样或适当方法估计不确定性，不把相关操作当独立样本。按平台/负载/规模报告交叉点，不误平均比率，说明权重与几何均值适用条件，不凭汇总量声称普遍优势。

内存：明确基线与观测区间，逻辑存活字节、时间加权或注明抽样方法的平均/中位、请求分配峰值、保留/存活字节、每存活元素增量成本、分配释放次数、进程 RSS/峰值 RSS 或注明平台等价量。区分请求字节、分配器开销、预留容量、进程占用、payload 和适配器；不把请求字节等同 RSS。可比诊断无法获取时注明原因；涵盖稳态、增缩、churn；注明采样频率和诊断开销。

功能矩阵必须有发布文档/源码/测试依据，原生、组合、不支持、未测区分，不把能力数量当性能胜负。集成成本描述适配代码量/复杂度、依赖/构建、所有权、错误清理、测试负担及应用定义的持久化保证，不做无依据的生产率结论。保留正负结果、假设、异常点、超时、反例、效度威胁；不可公平测量的指标说明并不用于排名。

GitHub 归档包括：冻结协议及修订记录；源码版本、许可、补丁清单；负载生成、种子、哈希、oracle、适配器、测试；构建复现命令、环境与提交清单；原始运行和诊断数据、统计定义、失败超时日志；分析脚本、派生表图（适当时内存-时间散点、交叉点、尾延迟图）、双语功能矩阵、双语结论与限制、含不可变证据链接的验收记录。大型二进制及依赖不塞普通 Git 历史，保存可复现获取方式/校验值，外部托管另行授权。遵守归属与第三方许可，既有历史证据不改。

## 6. 固定拟议阶段：四阶段，每阶段两步

本表只是延期计划，不是现时授权。启动时一次性确认版本、范围、平台可行性、算力和额度预算；常规修复留原步骤，重大扩张须经用户批准。

| 阶段 | 第一步 | 第二步 | 验收门槛 |
| --- | --- | --- | --- |
| A 协议与可行性 | A1 固定 Stable 和上游版本，源码/契约/许可调查及等价矩阵 | A2 冻结负载、指标、oracle、先导限制及统计计划 | 可审查协议、来源与资源边界，未预判胜负 |
| B 首要工业对照 | B1 实现并用 oracle 验证 GSequence/LKS 适配与身份/失败语义 | B2 按预设可行矩阵测量并保存原始数据和初步分析 | 匹配、正确、可复现，保留负面结果 |
| C 扩展对照 | C1 验证、测量有理由的 Boost 配置 | C2 LLVM 专用机制与选定生命周期/应用组合，记录排除 | 原生/组合/专用证据分离，新增对象有依据，不改生产 |
| D 综合归档 | D1 综合数据、不确定性、交叉点、时空/能力及效度分析 | D2 独立证据审核与双语归档，由用户验收研究 | 约定产物和不可变证据链接齐全，不虚构结论或暗示已发表 |

A 验收前不开展 B，不自主扩张后续阶段。Boost/LLVM 若确因构建或语义不可行而受阻，记录具体证据与拟缩减范围供用户决定；不造假对照。同一授权内完成仍可开展的独立工作。研究结束不自动优化产品、升级版本。

## 7. 执行效率与长期交接

普通 Codex 指令、有限连贯批次，**不使用 /goal**（用户因额度过度消耗明确拒绝）。批次内不因任意小节停工；设置清晰可验证停止点，单一持久清单记录范围、精确版本、证据、剩余工作与停止原因。继承有效且未失效的源码核查、计算、fixture、构建和测试结果；只有变化使证据失效、新匹配实验必需或具体矛盾时才重算重验。最终必要验证针对实际交付 HEAD，区分历史凭据与新结果。避免逐段提交、无谓 CI、反复全仓库审查与重新规划；相关修改成组提交。报告真实完成情况、具体阻断和剩余事项，不自授独立 PASS。ChatGPT 实查远端 diff、原始证据、方法与结论；用户决定验收及后续授权。

新会话接手先读本文件、实际 release/tag/branch、当前项目权威文档和有效协作政策，不从本计划推导未来实施权限。RC.1、RC.2 及 Stable 收尾期间保持延期；正式发布后主动向用户恢复这项研究计划，再制定并取得启动授权。
