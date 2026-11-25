# Full RC.1 bilingual documentation checklist / 双语文档清单

Working inventory for draft PRs #33 and #34. An existing counterpart is only evidence of file presence.
本清单用于草稿 PR #33 与 #34。存在译本只表示文件已创建，内容审查另行记录。

Baseline / 基线: Full `fd7e3e7d15b5b8fc35ff0bfb0ffbafe5c94b5111`; Repo `53956c2183827936f94bd8f44309f93b9b613cbe`.

Action codes / 操作代码: S1 ordinary / 普通文档; S2 implementation or archival translation / 实现指南或历史翻译; S3 semantic and navigation verification / 语义及导航验证.

| Owner / 归属 | Original / 原文 | Kind / 类型 | Other edition / 对应版本 | Required action / 待办 | Completion / 状态 | Verification / 验证 |
| --- | --- | --- | --- | --- | --- | --- |
| repo | `.github/pull_request_template.md` | current / 现行 | inline / 内嵌双语 | S3 semantic and link review / 语义与链接复核 | pair present / 已有双语 | English scopes and five checkboxes retained / 已核对五个归属选项 |
| full | `CHANGELOG.md` | current + historical / 现行及历史 | [CHANGELOG.zh-CN.md](../../CHANGELOG.zh-CN.md) | S3 semantic review / 语义复核 | pair present / 已有双语 | release headings, dates and bullet counts checked / 已核版本、日期和条目数 |
| repo | `CONTRIBUTING.md` | current / 现行 | CONTRIBUTING.md#参与-layerkeysort-开发 | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| repo | `README.md` | current / 现行 | README.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `benchmarks/README.md` | current / 现行 | benchmarks/README.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `benchmarks/WORKLOADS.md` | historical / 历史 | [benchmarks/WORKLOADS.zh-CN.md](../../benchmarks/WORKLOADS.zh-CN.md) | S3 semantic and index navigation / 语义和索引导航 | pair present / 已有双语 | command block checked / 已核命令 |
| full | `benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `benchmarks/research/full-v4.2.0-preview.1/RESULTS.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.1/RESULTS.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/research/full-v4.2.0-preview.3/REPRODUCTION.md` | historical / 历史 | benchmarks/research/full-v4.2.0-preview.3/REPRODUCTION.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; full SHA and fenced commands checked / 原件保留，已核全 SHA 与命令块 |
| full | `benchmarks/results/v3-preview1-metadata.md` | historical / 历史 | benchmarks/results/v3-preview1-metadata.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v3-preview2-final-metadata.md` | historical / 历史 | benchmarks/results/v3-preview2-final-metadata.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v3-preview2-metadata.md` | historical / 历史 | benchmarks/results/v3-preview2-metadata.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v3-preview3-validation-metadata.md` | historical / 历史 | benchmarks/results/v3-preview3-validation-metadata.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v4-preview2-smoke/README.md` | historical / 历史 | benchmarks/results/v4-preview2-smoke/README.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v4-preview3-smoke/README.md` | historical / 历史 | benchmarks/results/v4-preview3-smoke/README.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `benchmarks/results/v4-preview4/README.md` | historical / 历史 | benchmarks/results/v4-preview4/README.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `docs/API.md` | current / 现行 | docs/API.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | token subset checked / 已核部分标记 |
| full | `docs/ARCHITECTURE.md` | current / 现行 | docs/ARCHITECTURE.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/BENCHMARKS.md` | current / 现行 | [docs/BENCHMARKS.zh-CN.md](../BENCHMARKS.zh-CN.md) | S3 semantic and link review / 语义与链接复核 | pair present / 已有双语 | matrix numbers, order and links checked / 已核矩阵数字、顺序和链接 |
| full | `docs/COMPATIBILITY.md` | current / 现行 | docs/COMPATIBILITY.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/DEVELOPMENT.md` | current / 现行 | docs/DEVELOPMENT.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| repo | `docs/FULL.md` | current / 现行 | docs/FULL.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/FULL_CONSUMER_HANDOFF.md` | current + historical / 现行及历史 | [English](../FULL_CONSUMER_HANDOFF.en.md), [简体中文](../FULL_CONSUMER_HANDOFF.zh-CN.md) | S3 semantic review / 语义复核 | two language editions present / 双语全篇已建立 | exact SHAs, local links and seven checklist entries matched / 已核 SHA、链接和七项验收 |
| full | `docs/FULL_RC1_EVIDENCE.md` | current / 现行 | docs/FULL_RC1_EVIDENCE.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/FULL_SELECTION_GUIDE.md` | current / 现行 | docs/FULL_SELECTION_GUIDE.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md` | guide / 实现指南 | docs/IMPLEMENTATION_API_REFERENCE.md | S3 semantic review / 语义复核 | all 64 API entries paired / 64 项双语已建立 | header and source families reread; 64 rows and two C snippets matched; strict GCC consumer passed / 已复读头及各实现家族、核对64行与两段代码并通过严格消费者检查 |
| full | `docs/IMPLEMENTATION_INTERNALS.zh-CN.md` | guide / 实现指南 | [English](../IMPLEMENTATION_INTERNALS.md) | S3 semantic review / 语义复核 | 15 sections paired / 15 节双语已建立 | source checked for label bounds, sign, counters and bulk spacing; sections and local links checked / 已核标签分支、符号、计数、批建及链接 |
| full | `docs/IMPLEMENTATION_VALIDATION.zh-CN.md` | guide / 实现指南 | [English](../IMPLEMENTATION_VALIDATION.md) | S3 semantic review / 语义复核 | nine sections paired / 九节双语已建立 | exact command blocks, headings, local links and #24 merge timestamp checked / 已核命令、章节、链接及合并时间 |
| full | `docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md` | guide / 实现指南 | docs/IMPLEMENTATION_WALKTHROUGH.md | S3 semantic review / 语义复核 | all 15 numbered sections paired / 15 个编号章节双语已建立 | fixed missing tag member and Cursor validation order; section/command/ratio/link checks passed / 补 tag 成员和 Cursor 校验顺序，已核章节、命令、比率与链接 |
| full | `docs/INTEGRATION.md` | current / 现行 | docs/INTEGRATION.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/MIGRATION_V3_V4.md` | current / 现行 | docs/MIGRATION_V3_V4.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | token subset checked / 已核部分标记 |
| repo | `docs/PRODUCT_LINES.md` | current / 现行 | docs/PRODUCT_LINES.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/USAGE.md` | current / 现行 | docs/USAGE.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/V3_MIGRATION.md` | historical / 历史 | [docs/V3_MIGRATION.zh-CN.md](../V3_MIGRATION.zh-CN.md) | S3 semantic and index navigation / 语义和索引导航 | pair present / 已有双语 | pending / 待核 |
| full | `docs/V4_1_0.md` | historical / 历史 | docs/V4_1_0.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; exact SHA/link checks and adverse outcomes retained / 原件保留，已核 SHA 及链接，负面结论保留 |
| full | `docs/V4_2_0_PREVIEW1.md` | historical / 历史 | docs/V4_2_0_PREVIEW1.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_2_0_PREVIEW1_2.md` | historical / 历史 | docs/V4_2_0_PREVIEW1_2.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_2_0_PREVIEW1_2_1.md` | historical / 历史 | docs/V4_2_0_PREVIEW1_2_1.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; exact SHA/link checks and adverse outcomes retained / 原件保留，已核 SHA 及链接，负面结论保留 |
| full | `docs/V4_2_0_PREVIEW2.md` | historical / 历史 | docs/V4_2_0_PREVIEW2.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_2_0_PREVIEW3.md` | historical / 历史 | docs/V4_2_0_PREVIEW3.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_2_0_RC1.md` | current / 现行 | [docs/V4_2_0_RC1.zh-CN.md](../V4_2_0_RC1.zh-CN.md) | S3 semantic and links / 语义及链接 | pair present / 已有双语 | commands and hashes checked / 已核命令与哈希 |
| full | `docs/V4_FREEZE.md` | historical / 历史 | docs/V4_FREEZE.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | token subset checked / 已核部分标记 |
| full | `docs/V4_PREVIEW1.md` | historical / 历史 | docs/V4_PREVIEW1.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_PREVIEW2.md` | historical / 历史 | docs/V4_PREVIEW2.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_PREVIEW3.md` | historical / 历史 | docs/V4_PREVIEW3.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_PREVIEW4.md` | historical / 历史 | docs/V4_PREVIEW4.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/V4_RC1.md` | historical / 历史 | docs/V4_RC1.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical state labeled; original/hashes/links preserved / 历史状态已标，原件/哈希/链接保留 |
| full | `docs/V4_STABLE.md` | historical / 历史 | docs/V4_STABLE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical state labeled; original/hashes/links preserved / 历史状态已标，原件/哈希/链接保留 |
| full | `docs/VALIDATION.md` | current / 现行 | docs/VALIDATION.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |
| full | `docs/WORKLOAD_EVIDENCE.md` | historical / 历史 | [Chinese edition](../WORKLOAD_EVIDENCE.zh-CN.md) | S3 semantic and link review / 语义和链接复核 | pair present / 已有双语 | nine raw table bodies unchanged / 九个数据表正文一致 |
| full | `docs/demo/index.html` | historical interactive / 历史交互页 | [Chinese interactive edition](../demo/index.zh-CN.html), [Chinese explanations](../demo/index.zh-CN.md) | S3 final links and language review / 最终链接及语言复核 | both editions present / 双语版本已建立 | browser confirmed 16 identical paths and randomized no-Path state / 浏览器已验 16 条一致 Path 及随机缺省状态 |
| full | `docs/design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.md` | historical / 历史 | [Chinese companion](V4_2_0_LIVE_CORE_RESEARCH_HISTORY.zh-CN.md) | S3 semantic review / 语义复核 | pair present / 已有双语 | original, full SHAs, link destinations and three numerical table bodies checked / 已核原文、全 SHA、链接及三张数据表 |
| full | `docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `docs/design/V4_2_0_PREVIEW1_2_1_TASK.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_2_1_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; exact SHA/link checks and adverse outcomes retained / 原件保留，已核 SHA 及链接，负面结论保留 |
| full | `docs/design/V4_2_0_PREVIEW1_2_TASK.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_2_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original preserved; exact SHA/link checks and adverse outcomes retained / 原件保留，已核 SHA 及链接，负面结论保留 |
| full | `docs/design/V4_2_0_PREVIEW1_COMPLETION.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_COMPLETION.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW1_CONTINUATION.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_CONTINUATION.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW1_TASK.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW1_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `docs/design/V4_2_0_PREVIEW2_TASK.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW2_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | historical original preserved; SHA/local links checked / 历史原件保留，已核 SHA 及本地链接 |
| full | `docs/design/V4_2_0_PREVIEW3_TASK.md` | historical / 历史 | docs/design/V4_2_0_PREVIEW3_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_RC1_API_INDEX.md` | guide / 实现台账 | [Chinese](V4_2_0_RC1_API_INDEX.zh-CN.md) | S3 semantic review / 语义复核 | pair present / 已有双语 | 64 exact declaration rows checked / 已核 64 条精确声明 |
| full | `docs/design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md` | guide / 实现台账 | [Chinese](V4_2_0_RC1_DOCUMENTATION_AUDIT.zh-CN.md) | S3 semantic review / 语义复核 | pair present / 已有双语 | added source/test trace for labels, spacing and initial validation SHA / 已增标签、间隔和验证出处 |
| full | `docs/design/V4_2_0_RC1_REVIEW_REPAIR.md` | historical / 历史 | docs/design/V4_2_0_RC1_REVIEW_REPAIR.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_2_0_RC1_TASK.md` | historical / 历史 | docs/design/V4_2_0_RC1_TASK.zh-CN.md | S2 full counterpart / 完整译本; English task stays authoritative / 英文任务为准 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_ARCHITECTURE.md` | historical / 历史 | docs/design/V4_ARCHITECTURE.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/design/V4_DECISIONS.md` | historical / 历史 | docs/design/V4_DECISIONS.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| repo | `docs/governance/BASELINE.md` | historical / 历史 | docs/governance/BASELINE.zh-CN.md | S3 semantic and link review / 语义与链接复核 | pair present / 已有双语 | hashes and original checked / 已核哈希与原文 |
| repo | `docs/governance/OWNERSHIP.md` | historical / 历史 | docs/governance/OWNERSHIP.zh-CN.md | S3 semantic and link review / 语义与链接复核 | pair present / 已有双语 | original and policy files unchanged / 原文及配置不变 |
| repo | `docs/governance/RULESETS.md` | historical / 历史 | docs/governance/RULESETS.zh-CN.md | S3 semantic and link review / 语义与链接复核 | pair present / 已有双语 | checklist count and original checked / 已核条目数与原文 |
| full | `docs/history/BENCHMARK_HISTORY.md` | historical / 历史 | docs/history/BENCHMARK_HISTORY.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `docs/history/DESIGN_HISTORY.md` | historical / 历史 | docs/history/DESIGN_HISTORY.zh-CN.md | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| repo | `docs/index.html` | current / 现行 | `docs/index.html#zh-cn` at Repo `5a44bb8498d218c76330e71ec65b38e4f80ebfe2` | S3 semantic and final links / 语义及最终链接 | inline bilingual / 内嵌双语 | browser headings, IDs and language fragments checked / 已用浏览器核对标题、ID 及语言锚点 |
| full | `docs/v3/README.md` | historical / 历史 | docs/v3/README.zh-CN.md | S3 semantic review / 语义复核 | complete companion present / 完整译本已建立 | original and full SHA sets preserved; code blocks/local paths checked / 原件及全 SHA 保留，已核代码块与本地路径 |
| full | `docs/v3/index.html` | historical / 历史 | bilingual inline or owned companion | S2 full counterpart / 完整译本 | open / 待完成 | pending / 待核 |
| full | `examples/README.md` | current / 现行 | examples/README.zh-CN.md | S3 semantic and links / 语义及链接 | pair present / 已有双语 | pending / 待核 |

| full | `docs/design/V4_2_0_RC1_RESUME.md` | task record / 任务记录 | [inline Chinese edition](V4_2_0_RC1_RESUME.md#中文执行说明与英文对应英文指令为准) | S3 semantic review; English authoritative / 语义复核，以英文为准 | inline bilingual / 内嵌双语 | read at 540f905 / 已读 |

## Scope boundaries / 范围边界

Mini and Workbench product-owned documents are outside the two review units. Build definitions, raw results, manifests, fixture bytes, logs, generated binary receipts, and authoritative license texts are preserved. Embedded human-readable distribution instructions still need review.
Mini 与 Workbench 产品自有文档不在这两份 PR 内。构建定义、原始结果、清单、固定向量、日志、二进制凭据及正式许可文本保留。嵌入的分发说明仍须审查。
