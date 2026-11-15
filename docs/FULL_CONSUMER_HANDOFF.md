# Full 外部消费者交接与变更台账

> **项目归属：LayerKeySort Full** · **首次建立：2026-11-15** · **维护者：Full 项目（LKS3）**
>
> 本文给数据库及未来其他外部消费者提供**持续、可追溯的跨项目交接**。它不是数据库的设计文档，也不是新 API 或新版本的授权。以后不用依赖 ChatGPT 跨对话查找记忆；需要的历史结论、精确变更与核对入口应维护在 GitHub。
>
> **初始 Full 代码基线：** `main` 的 `70d7f0ef5857b54381b863d292a213909d87459f`（本文创建前）。**当时已发布 Stable：** [Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)。建立本文件本身仅是文档变动，不表示 Full 生产实现升级。

## 一、项目边界：数据库是外部消费者，不是 Full 的宿主

- Full 是**可以独立直接使用的 C17 动态顺序库**。用户可以单独引用公开头文件并编译链接，也可以使用正式 Release 中经过验证的 `layerkeysort.h` + `layerkeysort.c` 两文件 amalgamation；**任何人都不必先安装或使用数据库**。
- 未来数据库是**独立 GitHub 项目**，将按「数据库核心 → 数据库自己维护的中转/适配层 → Full 公开 API」集成。**数据库核心和适配层由数据库项目的 AI 负责；Full 及本交接文档由 LKS3 负责。**
- 数据库的永久记录 ID、查询、索引、磁盘存储、事务、服务、以及 ID→Handle 转换，原则上是数据库侧职责。Full 不因数据库存在而变成数据库专用库。
- 数据库提出新 API 需求时，须提交明确的使用场景、现有接口不足及复现证据；Full 项目独立评估它是否具有**通用的顺序管理价值**。允许拒绝数据库专属功能，或建议留在适配层。
- Mini 是另外一条独立产品线；数据库未来是否接入 Mini 由数据库项目评估。本文**只维护 Full**。Full 和 Mini 版本、API、发布契约不应混同。
- 目前数据库是**计划中的外部消费者**；未经真实接入提交和验证，不宣称它已在生产使用 Full。

## 二、必须长期执行的交接流程

### 初次集成

新消费者仍须根据自己采用的 Full **固定版本 / 40 位 SHA**，阅读相关的真实公开头文件、生产源码、构建文件、测试和 API 契约；本文件只能缩小定位范围，不能替代源码审查。

### 后续每次 Full 更新

**Full 的每次相关变更（包括只改内部实现）进入主线时，必须同步更新本文变更台账。每次 Full 版本发布必须有独立、准确的升级交接。** 具体要求：

1. 写清 **旧 SHA → 新 SHA、版本/分支、PR、commit、Issue**；若一个 PR 包含多个 commit，可用一个完整条目涵盖全部，但不能漏改动。
2. 写清实际改变的 **源码文件、函数/数据结构、前后调用链、改动前后行为**。不能只写「优化」「修复」「提高兼容性」。
3. 明确是否影响 **公开 API、参数/返回码、资源所有权、Handle 生命周期、排序语义、比较器/回调、重入、OOM 失败原子性、并发边界、快照/LS1/LKS4SNP1、旧版迁移、构建与 amalgamation**。
4. **即使没有 API 改动，也要写内部变化、内存/性能/故障行为变化**；引用实际基准或回归证据，承认未知。不要把某一指标改善宣称为全面加速。
5. 单独写出「**数据库适配层是否需要修改**」及理由；可取：**无需修改 / 需要专项复测 / 需要适配层修改 / 可能涉及数据库核心 / 证据不足**。其他 Full 消费者需要什么验证也要写出。
6. 给出测试、CI、复现程序及研究报告的链接，标记哪些亲自审查过、哪些只是外部报告。**不得把 Codex 报告直接当成 PASS**。
7. 在相同 PR 中更新本文件；如涉及对外契约或发行行为，还应同步更新必要的 API、README、集成/迁移文档与 Release Notes。不能只靠聊天记忆或事后回忆补写。

**Full 内部算法或性能变化，哪怕数据库不必改一行适配代码，也必须说明「内部怎么改、为什么无需改接口、有哪些负载要复测」。** Mini 专属变动如不影响 Full 的共享构建/交付，无需写成 Full 升级。

缺失变更记录、Git SHA 无法对上、或测试证据不足时，数据库项目应视为**升级影响尚未核定**，要求补充，而不是重新把整个 LKS 当成未知项目从头分析，更不能盲目升级。

### 每次新增变更的固定模板

```md
### Full <旧版本/旧 ref> → <新版本/新 ref> — <日期> — <Stable/RC/Preview/开发中>
- 变更前完整 SHA：
- 变更后完整 SHA：
- PR / commits / Issues / compare 链接：
- 真实改动文件（生产源码 / 公开接口 / 测试 / 工具 / 文档）：
- 具体变化（函数、结构、调用链；改动前 → 改动后）：
- API、参数、返回码与排序语义：改变 / 明确无变化
- 所有权、Handle、回调、OOM、并发边界：改变 / 明确无变化
- Snapshot、LS1、wire、历史恢复：改变 / 明确无变化
- 构建、安装、amalgamation：改变 / 明确无变化
- 性能、内存与已知回归（负载、数据、报告链接）：
- 数据库适配层：无需修改 / 专项复测 / 必须修改 / 可能改核心 / 证据不足；理由：
- 其他 Full 消费者应采取的操作：
- 测试、CI、基准及精确测试 SHA：
- 审核状态与未验证事项：
```

最新条目写在最上面，既保留真实历史，也不篡改此前版本、测试数据、报告或已发布 Tag。一个 PR 内的多次代码改动可归并到该 PR 的完整条目；新的独立 Full 变更或版本升级则新增条目。

## 三、Full v4.1.0 的已知真实能力和数据库相关边界

以下是初始源代码基线的导航，**新版本请以实际代码核对**：

| 主题 | 基线行为与外部注意事项 | 核对源码/契约 |
| --- | --- | --- |
| API | V4 冻结的 **64 个函数、14 个公开类型**，C17；Full 是通用库，不是数据库 | [公开头文件](../include/layerkeysort.h)、[API](API.md)、[冻结契约](V4_FREEZE.md) |
| 实时显式顺序 | `LksOrder` 支持相对/首尾插入、删除、移动、遍历和顺序比较；Handle 在自身删除或容器销毁后失效 | [order.c](../src/order.c)、[内部结构](../src/lks_order_internal.h) |
| 自动规则排序 | `LksManagedOrder` 由用户比较器维护顺序，提供定位；修改比较相关字段不能绕过排序结构更新 | [managed_order.c](../src/managed_order.c) |
| 永久 ID | **Full 不自动分配可持久化的业务 ID**；Live Handle 不是稳定磁盘 ID，也不是字符串排序 Key | [头文件](../include/layerkeysort.h) |
| Snapshot | `LksSnapshot` 捕获顺序与调用者回调提供的关联字节；**LS1 = namespace + 快照行 ordinal**，不表示不同时刻同一记录的永久身份 | [snapshot.c](../src/snapshot.c)、[snapshot_key.c](../src/snapshot_key.c) |
| 历史编码与恢复 | `LKS4SNP1` 格式；恢复通过应用 Resolver 映射到业务对象并建立新 Handle；Full 不承担完整数据库事务 | [snapshot_wire.c](../src/snapshot_wire.c)、[snapshot_restore.c](../src/snapshot_restore.c)、[order_bulk.c](../src/order_bulk.c) |
| V3 Path/LK1 | 旧 Path 代码仍可能因历史迁移而在生产源中，但 **V4 Live Order 不持续维护 V3 Path** | [迁移指南](MIGRATION_V3_V4.md)、[源清单](../cmake/ProductionSources.cmake) |
| 当前不提供 | 永久业务主键、通用字段索引/SQL、事务性磁盘存储、网络服务、多人协作排序、持久 Live Handle、公开的数字 rank/随机第 N 项接口 | [头文件](../include/layerkeysort.h)、[API](API.md) |
| 集成 | 可以正常通过 CMake/源码构建或官方发布两文件 amalgamation 集成；不承诺任意平台的通用动态二进制 ABI | [集成指南](INTEGRATION.md)、[中文指南](INTEGRATION.zh-CN.md) |

**切勿混淆：** （1）数据库永久记录 ID；（2）Full Live Handle；（3）Full LS1 历史排序 Key；（4）数据库版本或事务 ID。这些职责可组合，但不能互相当作同一身份。数据库未来是否真的需要「持续变化的持久排序 Key」、数字排名或新查询 API，须由实际需求证明，而不是 LKS 先行猜测。

## 四、已有发布版本的对外变化基线

### 初次建档 — 2026-11-15 — 仅新增规范文件

- **Full 源码基线：** `70d7f0ef5857b54381b863d292a213909d87459f`；公开 Stable 为 `v4.1.0`。
- **影响：** 新增外部交接规范与变更台账；不修改 Full/Mini 生产代码、头文件、历史 LS1/wire 格式或任何已发布发行物。
- **数据库操作：** 首次接入必须自行选定依赖 SHA 并审核真正用到的源码；以后通过本文的增量变更记录 + Git 差异复核来评估升级。
- **验证：** 文档及受审源码基线交叉核对；本条**不是**新增一轮编译、CI 或性能测试的报告。

### Full v4.0.0 Stable → v4.1.0 Stable — 历史发布补录

- **From：** `v4.0.0` (`80004ae732daf696f7e9192e417de0edb0b2aafa`)。
- **To：** `v4.1.0`（最终源码/文档基线 `70d7f0ef5857b54381b863d292a213909d87459f`）。
- **证据：** [Full v4.1.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)、[实现 PR #19](https://github.com/RXY712200/LayerKeySort/pull/19)、[完整变化与测量](V4_1_0.md)、[研究原始报告](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md)。
- **源代码变化：** `src/lks_order_internal.h`、`src/order.c`、`src/order_bulk.c` 将重复 owner 元数据从各 Resident Handle 迁移到块；Handle 仍分别分配且地址稳定，B64 与内部块索引仍保留。`src/lks_snapshot_internal.h`、`src/snapshot.c`、`src/snapshot_restore.c`、`src/snapshot_wire.c` 改为累计 association 结束偏移、同块分配快照 header/rows/namespace、完成捕获时裁剪 associations 至实际容量，调整恢复和 wire 访问。`tools/validate_distribution.py` 修正 Windows 隐式单配置生成器的 Debug 分发验证。
- **公开契约：** 64 函数/14 类型 V4 API、Handle 生命周期、OOM 原子性、LS1、LKS4SNP1、V3 LK1 导入及回调语义均保持兼容；**没有**给 Full 新增数据库 ID、动态持久 Key 或通用查询 API。
- **内存/性能：** 发布报告所测 x64 Full 请求分配字节：100 万 append 从 50,249,480 到 42,499,472（约下降 15.4%）；100 万/16 retained snapshots 请求字节峰值从 513,593,776 到 355,751,040（约下降 30.7%）。这些**不是进程 RSS**；部分 Linux 随机查找、较大 namespace key formatting、序列化和 churn 场景有回归，不能宣称全部性能提升。
- **对数据库适配层：** 采用已冻结 V4 API 的调用逻辑**无需因为此版本重命名函数或转换 Key 语义**；仍应针对真实负载复测、重新编译并验证 Snapshot 与恢复。**未见数据库真实集成测试，不能宣称已验证数据库兼容。**
- **测试事实：** 测试详情与准确 CI 链接见上列发布文档。此处是可溯源的历史摘要，未在本次建档重新执行测试。

## 五、跨项目消费者的升级步骤

1. 确定数据库当前固定的 Full **旧版本及完整 SHA**，目标 **新版本及完整 SHA**（`v*` 是 Full；`mini-v*` 是另一产品）。
2. 阅读本台账中覆盖该区间的全部 Full 变更记录；如缺少准确区间或信息不全，要求 Full 项目补充。
3. 使用 Git 实际比较旧、新源码，例如 `git diff --stat <old-sha> <new-sha>`、`git diff <old-sha> <new-sha> -- include/ src/ cmake/ tools/ tests/`，只对**确实变化且消费者用到的部分**深入读取相关源码与调用链；初次接入不能省略源码审查。
4. 对数据库自身做适配评估与集成测试：ID↔Handle、生存期、插入/移动/比较、ManagedOrder、Snapshot/关联字节/恢复、错误/OOM、编译链接，以及有实际需求的性能负载。
5. **数据库项目自主决定**是否升级依赖、更新自己的中转层/核心与发布；Full 不能代替它接受变更，也不要求用户每次通读整个 Full 仓库。
6. 如果发现通用 API 不足或缺陷，数据库向 Full 提交清晰复现与使用情景；Full 独立评估和维护。

## 六、Full 维护者验收清单

- [ ] 检查真实远端 HEAD、精确 diff、生产源码及相关测试；不能只依据 ChatGPT 旧总结、Issue 或 Codex 报告。
- [ ] 涉及 Full 的改动，在同一 PR/发布准备中更新**本文件的新增变更记录**；不篡改已发布历史。
- [ ] 说明函数/结构实际变化，明确公开契约改变或不变，并写出对数据库适配层及其他消费者的影响。
- [ ] 对性能声明提供公平可复现证据，明确回归和证据不足处。
- [ ] 维护公开 API、兼容性、集成文档与必要测试；遵守不同产品线隔离规则与现有保护分支的 PR/CI 流程。
- [ ] 未获授权不额外推进下一阶段、不擅自合并、打 Tag 或创建 Release。

**事实优先级：指定 SHA 的真实生产源码、构建和可复现测试 > 公开契约 > README/开发/集成文档与 Issue > 历史交接/聊天记忆。** 本文是跨版本变更索引和消费者交接入口，绝不以文档取代必要的源码验证。
