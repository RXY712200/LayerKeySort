# LayerKeySort 仓库治理：源码基线

历史英文原文：[BASELINE.md](https://github.com/RXY712200/LayerKeySort/blob/c108acd72b37614f4be271c9747484c2dde5eac4/docs/governance/BASELINE.md)。本译文仅解释观察记录，不发布版本、不授予权限，也不表示离线备份已存在。

## 采集状态（2026-10-11，GitHub API）

- 仓库 `RXY712200/LayerKeySort`，默认分支 `main`。
- `main` 提交及树为 `7f17331297e88618a71257ebc65b04cb6b4b8c25`，递归树未截断（277 个跟踪 blob）。
- Full Stable 标签 `v4.0.0`，发行目标 `80004ae732daf696f7e9192e417de0edb0b2aafa`。
- Mini Stable 附注标签 `mini-v1.0.0` 解引用为 `3f52798b83b6580aa4b89846b6403b0bd7b15f0a`。
- 当时标为 Latest 的发行是 Full `v4.0.0`。
- 开放的 [Workbench Phase 2 #9](https://github.com/RXY712200/LayerKeySort/pull/9) 为草稿，采集时 HEAD 为 `6af36e0f60d93d7893f73223ce9f0845fe37a93e`。
- Rulesets API 返回空列表。所用集成以 403 拒绝读取经典分支保护，不能据此推断保护全部缺失。
- 成功的 main CI：[Full](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087857)、[Mini](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087854)、[Workbench](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087860)。

### 已发布 Stable 资产（GitHub API SHA-256）

| 产品 | 资产 | SHA-256 |
| --- | --- | --- |
| Full 4.0.0 | `LayerKeySort-4.0.0-amalgamation.zip` | `ed828c547cba05e39d05e28c9f6eaec7ffd325839d291713b7cfe0e47bef1a1d` |
| Full 4.0.0 | `LayerKeySort-4.0.0-SHA256SUMS.txt` | `0ad427855cde57af7c426dba6168830838b0e7932bfa77c8c5efce84cdbcfee2` |
| Mini 1.0.0 | `LayerKeySort-Mini-v1.0.0-source.zip` | `96b21a4d9177a98991e27712063dab7ce4d91a4adc0a76e420e8ab74263089c4` |
| Mini 1.0.0 | `LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt` | `722c2d9aa5bace6ce7d15cdb5f876c270d9a0c3105b882f8e1d041db9743141a` |

采集时四项资产所属的 Release 均报告 `immutable=false`。标签防护和未来发行不可变设置是不同的控制。

## 恢复与独立备份

不可变 SHA 及校验清单提供恢复引用。启用严格 Rulesets 或合并治理 PR 之前，归档仓库并独立验证恢复：

```sh
git clone --mirror https://github.com/RXY712200/LayerKeySort.git LayerKeySort.git
git -C LayerKeySort.git bundle create ../LayerKeySort-governance-baseline.bundle --all
git -C LayerKeySort.git bundle verify ../LayerKeySort-governance-baseline.bundle
```

将 Git bundle 保存于 GitHub 仓库之外。另行下载 Full/Mini Stable 附件并核对上表 SHA-256。bundle 不含 GitHub Issues、PR 讨论、Actions 设置或 Release 资产；需要完整灾难恢复时另行导出。仅有 GitHub Actions 资产不算离线备份。

**首次采集状态：** 已记录基线；离线镜像、bundle、外部保存及恢复验证在当时环境均待完成，不能写为通过。不得创建虚假备份标签或重写既有发行。实际启用生产规则前重新核对 main 和 PR #9。
