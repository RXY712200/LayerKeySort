# Preview.1：候选验证与完成（历史任务译本）

原文：[V4_2_0_PREVIEW1_COMPLETION.md](V4_2_0_PREVIEW1_COMPLETION.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_2_0_PREVIEW1_COMPLETION.md)。原文及历史授权保留，以英文为准；本译本不授权现在实施/实验/发布。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

同原工作包、Issue#27 Steps1–2、draft#28、dev/full-v4.2.0-preview.1。读原任务/前续作，此指令仅收敛候选验证/完成，不开新阶段、不放宽门槛。保先前采集及历史。已审起点 `f0340a3c7b96ba6227b93c89a1256c4f92c4bae3`，实现 `22544cf283694dd27cef203caba9cf1838845947`。Fetch最新续作，核实际base/head。

## 独立检查点

审者确认PR head/main未变、31路径、实现至审head的src/include/cmake/CMakeLists全部36blob精确一致。真实order.c补丁为高度引导祖先比较及变更区local更新，同块/repair/预分配/签名保留，无新resident/block字段。此聚焦源码审无实质缺陷，不是穷举执行/批准。重算全部1,440消融CSV：45轨迹×四变体各七正式。对Stable中位ancestor .9053786、mutation .8287413、combined .7142152；combined compare .5309342、update .6920400、long .6725386、short .8014858、hotspot .7404580。胜45/45、最差.8612037，两个Windowsscreen目标达。仅适用分离编译实验非最终生产binary。仅中点/fallback原型一次记录1057耗尽，不证明全部动态OM差。保combined验证目标，除实质必要不重开无界探索/删subtree。

检查点f0340a3的Trusted Scope Guard/Policy Tests/Mini/Workbench过，Full C17及可选Measurement仍进行；查新finalhead，不能借中途状态。本地GCC Debug/Release/包为实施者报告，保真实命令凭据。

## 一次连续完成剩余证据

1. 研究harness真可复现：隔离stage固定4.0/4.1/真实production candidate/List/Treap/归档实际tag/list-range，核source/trace/binary hash、compiler/link flags/许可。Variant生成用不可变旧source，mutation_variants.py读现order.c已非旧源；不覆盖历史或静默重生。给便携clean-checkout build/run、manifest及成功smoke；旧screen仅历史选择。
2. 原最终同时代Windows/原生Linux、真实production、45轨迹/所有controls、轮换、预热后七fresh formal、精确边界/独立正确。需既有授权基础设施则用，不改公共workflow/control-plane/私有内容。逐host/cell绝对时间/比、对4.0/4.1/外部、波动/反例/请求保留/合适RSS峰值/tail。共同成功语义与额外保证分开。同语言可行做，否则原匹配native/API-overhead并实录残余未知，不用旧IST代当前control。保25%compare/15%overall，不事后改。
3. 补mutation/managed/churn/演进split-merge-redistribute/restore拓扑profile。新聚焦回归含ancestor/sibling/root/equal/reverse、结构变更后pairs、shifted insert/remove/crossmove、未变prefix、stale tail、membership/local、存活地址、no-op/revision/cursor、callback/foreign/out、到达OOM。旧39有价值但PR当时无新定向生产测试；检查真实assert/diagnostics非假旧suite证明。仅原acceptance profile，不做Preview2的完整lifecycle/DB归因。隔离OrdWeave原生Windows实际固定替换、独立状态/身份/checkpoint/recovery，不写official库/pin；记前后source和结果。
4. 补平台/API/wire/sanitizer/consumer/distribution/examples/精确CI证据。后续source变则重测受影响/candidate，纯doc保implementation SHA并核finalgate不冒改旧测量。报缺/失败/pending，不改governance/sharedworkflow。

## 独立审核指出的具体文档修正

阶段及四指南存在但同步不足，须正文修非再intro。Walkthrough约106/112仍称普通插用v4_locations、跨compare读subtree：换真实suffix及parent/height/child行为。Internals约101分普通placement与结构repair，查全部retired absolute rank；subtree_blocks仍生产维护/诊断，但compare不读。四指南历史4.1已审基线与临时candidate分清，真实status/sectionprovenance。Validation仍只pre-metadata Windows Release，须核后来的Debug/Release凭据，不推测。

交接约20仍“planned external consumer”，实际已有checkpoint/WAL；按OrdWeave真实source/revision修当前句，分official依赖/候选测试。用真实完整change模板替短ledger：函数/结构/前后调用、API/owner/Handle/callback/OOM/wire/build是否变、性能资源、独立DB core/adapter判断及retests、CI/限制。历史保留。Preview1四行同步矩阵给真实节/函数/旧新source/tests/限制。架构补精确invariant/proof/复杂度（块数最坏仍log）、替代证据/限制、理由/primary refs，不凭两优化称原创/普遍选型。报告最终productionbinary对照/命令/处置，metadata与source/doc SHA分开。

## 停止与报告

完成以上及原Step1/2，范围内普通修复/重测获准。无新步骤/探索周期/分支/PR/merge/tag/release/Preview2/越Full生产。真阻断给实际命令/证据/范围非模糊待办。候选critical失败在原步修或有据不采用，不结果后降门槛。一份英文报告原字段、final remote HEAD/source/test/CI/证据/Technical Debt Impact，独立审者/owner保接受与发布决定。
