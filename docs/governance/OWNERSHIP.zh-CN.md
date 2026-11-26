# LayerKeySort 仓库归属与范围执行

[英文原文](https://github.com/RXY712200/LayerKeySort/blob/c108acd72b37614f4be271c9747484c2dde5eac4/docs/governance/OWNERSHIP.md)。此译文仅说明原政策。机器判定依据为 [ownership.json](ownership.json)；[scope-exceptions.json](scope-exceptions.json) 初始为空。本政策覆盖仓库，不改变任何产品 API。

## 产品区域

- `full`：根目录 Full C17 源码、公开头、旧版与回归测试、基准、示例、根 CMake 和 Full 专有文档。Full 在技术上维护其 CI 和发布工作流，但工作流路径归受保护的 `repo` 控制面。
- `mini`：`mini/**`、两个列明的 Mini 打包脚本。Mini CI 和候选发布工作流的技术归属为 Mini，路径归属为 `repo`。
- `consumer`：`consumers/layer-order-workbench/**`。Workbench CI 与证据工作流由 Mini/Workbench 技术维护，路径归 `repo`，防止产品 PR 静默弱化验证。Workbench 缺陷不自动等于 Mini 库缺陷。
- `repo`：产品线 README、导航、共享政策、贡献和许可证元数据、CODEOWNERS、PR 模板、治理政策及所有明确列出的 Actions 工作流。初次多产品导航迁移中，`docs/FULL.*` 和 `docs/index.html` 也归 `repo`；其中 Full 技术陈述仍由 Full 审查。

默认拒绝未知、歧义或畸形路径、缺失的文件列表页、不安全的重命名分类，以及没有预先提交的精确 HEAD SHA 授权的跨归属集合。按精确路径优先、否则最长声明前缀分类。PR 标题、标签和正文不影响分类。

通常每份 PR 只改一个类别。同时修改产品源码与其 CI 工作流属于跨类，需预先获准的精确文件例外或拆成两份 PR。必要的跨产品变更须先由真实所有者批准独立 `repo` 治理 PR，在 `scope-exceptions.json` 绑定 PR 编号、HEAD SHA 及排序后的变更清单。例外不得记录 `base_sha`，因为授权先合入 main，最终基线 SHA 会产生自引用。例外只从受保护目标分支读取。每次运行的 `read_pr_inventory()` 独立将事件的 PR 编号、HEAD、基线分支和 SHA、完整变更清单与实时 API 对照。基线移动本身不撤销未变 HEAD 的许可；HEAD 或清单变化需重审。

### 跨产品的精确授权顺序

1. 创建跨产品 PR 后固定其 HEAD，记录编号、40 位 HEAD SHA 及排序后的 `changed_inventory()`，包括重命名来源与状态。
2. 用独立 `repo` PR 添加并审查精确 `{"pr_number": ..., "head_sha": ..., "files": [...]}`，不允许 `base_sha`。
3. 通过正常获准路径合并治理许可到 main，使许可独立于产品 PR。
4. 在当前 main 上重跑可信门槛，动态验证基线身份，核对未变 HEAD 与文件清单。
5. 产品 PR 变基、合入 main 更新或改变清单时，HEAD 或授权条件不再匹配，门槛拒绝通过。从新 HEAD 重办许可。

此机制不得启用严格的 “Require branches to be up to date”。授权推进 main 后再更新产品 HEAD 会形成第二轮授权循环。先用宽松必需状态检查与独立合并提交回归。若严格新鲜度为必需条件，应重新设计例外机制。宽松检查仍要求准确 HEAD、实时基线及完整 CI。

## 信任边界

可信 Scope Guard 由 `pull_request_target` 启动，只读仓库和 PR 权限，只检出可信 main 基线 SHA。它不检出或执行 PR HEAD；GitHub API 文件清单视为不可信且须完整。

修改门槛或策略仍由基线门槛验证。治理 PR 合并后，新程序才成为未来策略，因此须真实所有者人工审查并有测试证据。CODEOWNERS 不提供第二个 GitHub 身份或独立强制审核。使用同一真实身份的管理员与 Codex 共用权限。

GitHub 公开仓库事件政策可能从 2026-11-02 阻止 `pull_request_target`。必须确认该政策允许工作流且真实 PR 在要求 SHA 上运行，方可启用要求 `scope-guard` 的 Ruleset。跳过、缺失、中立或本地模拟不足以确认实际执行。

## 检查与发布约束

拟要求 `full-gate`、`mini-gate`、`workbench-gate`、`scope-guard`。上游矩阵须成功完成。初始聚合门槛核对矩阵结果，无另行冻结的成员数量。矩阵定义变更归 `repo` 并独立审查。普通 PR 的 Workbench Measurement Evidence 初始仅为辅助；对以测量为目标的 #9 是特定验收条件，与常规 `workbench-gate` 不同。见 [RULESETS.md](RULESETS.md)。Rulesets 是额外的平台设置；源码本身不强制仅由 PR 合入或标签防护。

- 治理不改 Full 4.x 冻结 API、LS1 或 LKS4SNP1。
- 不改 Mini 1.x 的 18 个 API、源码或发布契约。
- 保留历史标签、发行、基准、Issue 历史与资产校验和。
- Workbench #9 保持独立；须核验过渡再启用新 CI 要求。
- `repo` 路径类别不授予修改 Full、Mini 或 Workbench 生产代码的权限。

CODEOWNERS 映射实际账户 @RXY712200。Full 和 Mini 的 ChatGPT 对话是独立架构建议渠道，不是两个 GitHub 审核者。
