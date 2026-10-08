# 产品线与发布管理规范

LayerKeySort 采用**单仓库、多独立产品、独立版本号**。这不是将同一套源码用开关裁剪成
“完整版”和“精简版”。各产品的公开 API、兼容性和生命周期，以各自文档为准。

## 产品划分

| 产品 | 所在目录与构建入口 | 名称 | 当前状态（2026-10-08） |
| --- | --- | --- | --- |
| Full | 仓库根目录：`include/`、`src/`、根 `CMakeLists.txt`、`docs/` | `LayerKeySort` / `layerkeysort` | v4.0.0 Stable，已经发布 |
| Mini | 自包含的 `mini/` 子目录 | `LayerKeySort Mini` / `layerkeysort_mini` | v1.0.0 Stable 候选，尚未发布 |

Full 与 Mini 是针对不同需求的**并行产品**，不是父子依赖关系。Full 面向更完整的
顺序管理和快照工作流；Mini 使用独立的简单双向链表实现，冻结 18 个公开函数，不依赖
Full 的生产源码。只复制 `mini/` 就能单独构建，两个产品的数字版本之间没有兼容关系。

不为了目录对称而将已有 Full 从仓库根目录迁移到 `full/`。默认根构建面向 Full；
`cmake -S mini` 面向 Mini。两个库可以由外部消费者显式链接，但任何一个都不应
隐式依赖另一方。Windows、Linux、macOS 等平台支持是产品的构建/兼容性维度，
通常**不是单独的产品版本**。

## 独立版本与全仓库 Tag

每个产品独立遵循自己的版本与兼容契约；已经发布的标签不得复用、改指向或重写。

| 产品 | 对用户展示的版本 | 全仓库 Git Tag 示例 |
| --- | --- | --- |
| Full | v4.0.0 | `v4.0.0`（保留原先所有 `v*` 历史标签） |
| Mini | v1.0.0 | `mini-v1.0.0` |

仓库已有的 `v1.0.0` 属于**历史 Full**，不是 Mini，绝对不能复用。Mini 新 Tag
必须采用 `mini-v` 前缀。将来若批准新的独立产品线，应有自己的前缀；同一个产品的
平台编译产物不必新建产品级 Tag。

Git Tag 指向的是**整个仓库的某个提交**，而非某个文件夹。发布时须记录完整 40 位
提交 SHA。一个 Mini Tag 指向的提交包含 Full 文件，不代表 Mini 运行时需要 Full；
其他产品的后续发展也不应改变已经发布的 Tag。

## Releases 与下载文件

GitHub 的 Releases 页面是**整个仓库共用的**，其中 `Latest` 也不能同时分别给
Full 和 Mini 设立一套。根 README 和 Mini README 分别维护自身的当前版本与入口。
Release 标题分别以 `LayerKeySort`（Full）和 `LayerKeySort Mini`（Mini）开头，
链接准确 Tag，并在发布时检查 `Latest` 的实际表现。不要因为发布 Mini 就让用户
误认为已发布的 Full Stable 被替换。

Full 继续保留现有发行工具和历史 Releases。Mini 采用独立的候选打包流程：从指定
Git 提交中，只提取受到版本控制的 `mini/` 内容，生成以
`LayerKeySort-Mini-vX.Y.Z/` 为顶层目录的源码 ZIP 和独立 SHA-256 校验文件，
并逐文件对照该提交验证。这个 ZIP 是跨平台的 C17 源码，不是 Mac/Windows 专属二进制。

GitHub 自动附带的 Source code ZIP / tar.gz 会包含**整个仓库**，不能将它宣传为
Mini 独立包。Mini 应以额外上传的“仅 Mini”源码包作为主要下载内容。

候选工作流只构建、验证、上传 Actions 临时工件供审查，**不会自行创建 Tag、
GitHub Release 或公开发行**。正式发布必须另行明确授权，核查准确 SHA 对应的 CI、
源码包校验值、Release 文案与链接。

## 日常开发与验证

只使用一个长期集成主分支 `main`，按需使用短期功能/发布准备分支，不采用永久的
`full`、`mini` 分支作为两套真相。Mini 的业务代码、测试和自身文档留在
`mini/`；Full 对应内容保持根目录。跨产品导航、发行脚本和 CI 可以留在共享目录，
但必须说明影响范围。

Full 原有 CI 继续测试 Full；Mini 独立 CI 覆盖 GCC、Clang、MSVC、AppleClang、
Linux ASan/UBSan，以及独立提取、外部消费者和可选共存。PR 的集成检查应同时
核查两个产品；不要随意用路径过滤跳过关键检查。修改打包脚本后必须执行对应
发行包验证。CI 通过是可复核的证据，并不等于已经得到发布授权。

PR / Issue 描述建议明确写 `Full:`、`Mini:` 或 `Repo:`，并注明受影响接口。
保留历史 Full 的 Release 与 Issues，不追溯改名，不凭设想建立尚不存在的产品。

## Mini v1.0.0 发布核对表（尚未执行）

1. 将审查过的候选与拟集成的提交核对，锁定最终 SHA，确认 Full 生产文件
   和冻结公开 API 没有被意外修改。
2. 检查最终 SHA 上 Full / Mini CI 均已完成且成功，Mini-only ZIP 经验证。
3. 确认 Mini 内版本为 v1.0.0，且 `mini-v1.0.0` 未被占用；
   不使用历史 Full 的 `v1.0.0`。
4. **只有取得明确发布授权以后**才合并、对批准的提交创建 Mini Tag、
   编写 `LayerKeySort Mini v1.0.0` Release，上传经验证的 Mini ZIP
   及 SHA256SUMS，并检查链接。
5. 另经审查更新 README 中的“候选/未发布”措辞和正式下载链接；
   检查仓库级 `Latest` 标记的结果，准确披露局限。

本文不构成合并、打 Tag、发布 Release 或修改 Issue 的授权。
