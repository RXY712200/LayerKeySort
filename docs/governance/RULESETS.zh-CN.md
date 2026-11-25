# 治理部署：GitHub 管理设置与回滚

[历史英文原文](https://github.com/RXY712200/LayerKeySort/blob/c108acd72b37614f4be271c9747484c2dde5eac4/docs/governance/RULESETS.md)。本说明性译文保留当时的检查清单；它不证明规则已启用，也不授予绕过 PR 测试或改动既有 Stable 标签和发行的权限。

## 启用前的已核验条件

- [ ] 在 GitHub 之外独立保存可恢复镜像或 bundle，以及现有 Full/Mini Stable 资产并校验。PR #10 的 Actions 资产是临时的，仍存于 GitHub。
- [ ] 审查并合入产品中立的 README/Full 入口 PR #10，包括 Mini 描述和 Pages 链接，生产不变。
- [ ] Mini Workbench 草稿 #9 正常合入并重验，或获得独立迁移/例外许可。它同时改变消费者代码和 Workbench 工作流，二者刻意属于不同受保护类别。
- [ ] 可信 Scope Guard 和三个固定矩阵门槛已安装在候选受保护 main。
- [ ] 真实 PR 在此基线上证明 `scope-guard` 启动并报告正确来源、SHA 和成败。新增 `pull_request_target` 工作流须先安装在目标分支才能验证。
- [ ] 适用事件政策明确允许 `pull_request_target`。受影响的公开仓库从 2026-11-02 开始默认限制。
- [ ] 同一准确且最新的 PR 显示四个独立、已完成的检查：`scope-guard`、`full-gate`、`mini-gate`、`workbench-gate`。`scope-guard-selftest` 仅为信息检查，不能替代。
- [ ] 在 GitHub 管理界面直接核对经典分支保护。GitHub App API 的 403 不能证明无保护。
- [ ] 实际所有者确认单人正常合并及发行不会因设置互锁。

## main 分支规则

进入 GitHub 仓库 Settings → Rules → Rulesets → New branch ruleset，仅针对 main。

1. 合并前要求 Pull Request。
2. 要求上述四个固定检查，来源为预定的 GitHub Actions app。用真实 PR 核对，检查实际运行前不得将它设为必需。
3. 禁止强推及删除；支持时要求解决讨论。
4. 保留 Merge Commit，不启用 Require linear history。
5. 单人维护不得要求不存在的第二审批者或无法提供的 CODEOWNER 审批。
6. 常规 Codex 工作无自动绕过。实际所有者须有紧急恢复计划，记录前后设置和原因。
7. 必需工作流无路径过滤。末尾任务用 `always()`，任何列出的上游 ID 未报告 `success` 都应失败。
8. 支持精确 HEAD SHA 例外期间，初始关闭 Require branches to be up to date before merging。授权推进 main；若要求更新到最新，产品 HEAD 会变化，原许可失效并再进入循环。须用 GitHub 实际归属验证这一取舍；以后需要严格新鲜度时重新设计授权方式。
9. 用真实 PR 验证基线推进和 SHA 新鲜度。过时的合并提交或检查归属歧义应阻止启用。

平台不能运行可信事件或准确归属结果时暂停执行。不得静默替换为更弱或自声明的检查。

## 标签规则

在同一设置中创建 tag ruleset，覆盖 Full `v*` 和 Mini `mini-v*` 历史名称。

- 限制已发布匹配标签的更新和删除。
- 未获准并验证正常、无绕过的发布路径前，勿全面禁止创建。创建和更新是不同规则。
- 在独立测试仓库或受控命名空间用一次性标签验证，禁止重写已有产品发布标签。
- 记录附注标签对象及解引用提交的 SHA，二者不可混淆。

## Workbench Phase 2 / PR #9 的过渡

以下为合并顺序，不授予 Full 负责人修改或合并 Mini 消费者工作的权限。

1. Mini 独立审查 #9 期间，治理 #11 保持草稿。#9 含 `consumers/layer-order-workbench/**` 与 `workbench-ci.yml`、新增 `workbench-evidence.yml`，属于不同类别；无例外时门槛启用后会拒绝。
2. 优先在 #11 合入或启用门槛前完成 #9。要求常规 Workbench 测试及双平台 Workbench Measurement Evidence 成功，并检查上传的测量资产。
3. #9 无法先完成时，Mini 可将仅工作流改动拆为独立审查的治理 PR，保留仅消费者代码的 PR。不得为全部 `consumers/**` 静默豁免。
4. #9 和中立主页 #10 安全合并后，将 #11 更新至新 main 并重跑相应测试，维持 `repo` 单一归属。
5. 合入经审核验证的门槛，但暂不设为强制。用后续真实测试 PR 验证要求的 SHA 上确有 `scope-guard`，改变测试 HEAD 后再验证；涵盖未经授权的跨归属。
6. 测试成功才启用。实际所有者另行完成外部备份，并检查已存在的经典保护。

当时选择将 Workbench Measurement Evidence 作为未来普通 PR 的补充，它不在四个固定常规门槛内。对 #9 性能证据任务，它是人工验收条件。若以后要常规强制双平台证据，须经 Mini 批准先新增独立稳定门槛；不能称当前 `workbench-gate` 已执行该要求。

## GitHub Release 完整性

采集时 Full 4.0.0 与 Mini 1.0.0 Release 均报告 `immutable=false`。新的发行不可变设置仅适用未来发行。保留 [BASELINE.md](BASELINE.md) 中四个已发布 SHA-256，勿用历史资产测试。平台支持且获准后，创建 Draft Release、上传和核验准确 SHA 的资产和清单，再以不可变方式发布。本清单不授予任何发布权。

## 恢复与负向测试

启用前在测试仓库或测试 PR 演示：

- Mini 单独变更通过；Mini 与 Full 生产混合拒绝。
- 单独 Full 或 Workbench 消费者通过。
- Repo PR 不能通过标签授权产品生产改动。
- 独立批准的 `repo` 例外含 PR 编号、HEAD SHA 和确切排序清单；许可先合入 main 后才适用未变 PR。无 `base_sha`。HEAD 或清单变化拒绝；基线变化单独按实时 API 验证。
- 例外可执行，无自引用基线 SHA，也无需进入严格最新的重复变基循环。
- 弱化 Scope Guard 的 PR 不能用自己的验证器放行；实际门槛由可信基线提供。
- 缺失、取消、失败、跳过或中立的上游要求由相应固定门槛拒绝。
- 同一 PR 中工作流与产品源码为跨归属，需独立获准的精确文件例外。
- 正常 Full/Mini 发布可创建新标签；已发布标签不能经常规路径删除或重指。

若强制执行导致维护互锁，实际所有者应记录原因与检查状态，通过管理界面暂时调整阻塞规则，经 PR 恢复已知正常工作流，再启用并记录。禁止强推 main、改写发行历史或永久禁用规则来隐藏失败。
