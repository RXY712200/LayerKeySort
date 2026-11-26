# Contributing to LayerKeySort

[简体中文](#参与-layerkeysort-开发)

## Before opening a change

- Understand the affected public semantics before changing implementation.
- Keep the scope focused.
- Avoid unrelated formatting changes.

## Products, building and versions

The root build and [development guide](docs/DEVELOPMENT.md) describe **Full**.
For **Mini**, use [mini/README.md](mini/README.md) and
[mini/docs/DEVELOPMENT.md](mini/docs/DEVELOPMENT.md). See the
[product-line and release policy](docs/PRODUCT_LINES.md) before changing tags,
release packaging or shared workflows. Full and Mini have independent version
and API contracts. Review and run checks for both when a change crosses lines.
For Full, consult the [validation guide](docs/VALIDATION.md).

## Testing

Add or update relevant regression tests and run them before opening a change. Report which tests and configurations you ran.

## Public API changes

Public API changes should be deliberate and documented. Describe behavior and compatibility impact in the change. For Full, review the active V4 contract in [docs/V4_FREEZE.md](docs/V4_FREEZE.md)
and [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md). For Mini, review
[mini/docs/CONTRACT.md](mini/docs/CONTRACT.md). Do not apply one product's
compatibility claims to the other.

## Pull requests

Include a concise description, the reason for the change, tests performed, and behavior or compatibility impact.

Release-facing changes also need a complete documentation and presentation
review, including examples, diagrams, and GitHub Pages navigation. The
[development guide](docs/DEVELOPMENT.md) describes the Stable-release check.


[Governance baseline](docs/governance/BASELINE.md) · [Ownership](docs/governance/OWNERSHIP.md) · [Rulesets](docs/governance/RULESETS.md)

## 参与 LayerKeySort 开发

[English](#contributing-to-layerkeysort)

### 提交改动前

- 修改实现前理解受影响的公开语义。
- 将改动限定在实际所需范围。
- 避免无关格式变动。

文档版本：以下固定候选提交中的中文指南属于已集成至 main、尚未发布的 Full 4.2.0-rc.1 文档；历史记录译本只解释其原版本，不表示候选实现已发布。已发布 Stable 4.1.0 的权威文档仍见 [Stable API（英文）](https://github.com/RXY712200/LayerKeySort/blob/70d7f0ef5857b54381b863d292a213909d87459f/docs/API.md)、[Stable 使用说明（英文）](https://github.com/RXY712200/LayerKeySort/blob/70d7f0ef5857b54381b863d292a213909d87459f/docs/USAGE.md)及[原始变更日志](https://github.com/RXY712200/LayerKeySort/blob/70d7f0ef5857b54381b863d292a213909d87459f/CHANGELOG.md)。

### 产品、构建及版本

根目录构建和[开发指南；已集成但未发布 RC.1 文档](https://github.com/RXY712200/LayerKeySort/blob/16bdde588b1bbf996bf5b6392a5fb91fb45d3576/docs/DEVELOPMENT.zh-CN.md)对应 **Full**。使用 **Mini** 时，请参见 [mini/README.md](mini/README.md)和 [mini/docs/DEVELOPMENT.md](mini/docs/DEVELOPMENT.md)。改变标签、发行打包或共享工作流前，请查阅[产品线和发布策略](docs/PRODUCT_LINES.zh-CN.md)。Full 与 Mini 有独立的版本及 API 契约；跨产品改动须审查并执行两边的检查。Full 的验证见[验证指南；已集成但未发布 RC.1 文档](https://github.com/RXY712200/LayerKeySort/blob/16bdde588b1bbf996bf5b6392a5fb91fb45d3576/docs/VALIDATION.zh-CN.md)。

### 测试

提交改动前新增或更新相关回归测试并实际运行。说明执行的测试和构建配置。

### 公共 API 变更

公共 API 变更应是有意的决定，并写入文档。说明行为和兼容性影响。Full 请核对 [V4 契约；已集成但未发布 RC.1 文档](https://github.com/RXY712200/LayerKeySort/blob/16bdde588b1bbf996bf5b6392a5fb91fb45d3576/docs/V4_FREEZE.zh-CN.md)及[兼容性说明；已集成但未发布 RC.1 文档](https://github.com/RXY712200/LayerKeySort/blob/16bdde588b1bbf996bf5b6392a5fb91fb45d3576/docs/COMPATIBILITY.zh-CN.md)。Mini 请核对 [Mini 契约](mini/docs/CONTRACT.md)。两条产品线的兼容声明各自适用。

### Pull Request

提供简明的改动内容、理由、已执行测试及行为或兼容影响。

影响发布的改动还须完整检查文档与公开呈现，包括示例、图表和 GitHub Pages 导航。[开发指南；已集成但未发布 RC.1 文档](https://github.com/RXY712200/LayerKeySort/blob/16bdde588b1bbf996bf5b6392a5fb91fb45d3576/docs/DEVELOPMENT.zh-CN.md)描述 Stable 发布检查。

[治理基线](docs/governance/BASELINE.zh-CN.md) · [归属](docs/governance/OWNERSHIP.zh-CN.md) · [规则部署](docs/governance/RULESETS.zh-CN.md)
