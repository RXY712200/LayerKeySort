# Full Preview.3 工程及采用证据（历史译本）

原文：[V4_2_0_PREVIEW3.md](V4_2_0_PREVIEW3.md)，英文，固定于 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_2_0_PREVIEW3.md)。原文/数据不变，状态指该里程碑；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

[草稿PR #32](https://github.com/RXY712200/LayerKeySort/pull/32)目标保留Preview.2 `01bc01e2d9ddbda448d9ef0807150f419203b580`，任务HEAD `326b7859d2a0325520d9b88635e400ba32a33d86`。生产仍 `fe00536e7bd7f4e5128432ea4a29e54158074903`，开发4.2.0-preview.2。见[绑定审计](design/V4_2_0_PREVIEW3_ARCHITECTURE.zh-CN.md)及[选型](FULL_SELECTION_GUIDE.zh-CN.md)。

## 验证与范围

未对齐wire/恢复混合tag回归 `754b3088c160564f6666ee520284168ee5da6ac7` / `3ac82d26301ea12876a3ebdba6319fe8233621f3`。严格本地GCC Debug/Release各39/39、API/C/C++分发/资产过，精确托管见PR。Preview.2 Workbench AppleClang两独立尝试cancelled，无runner/steps（138487623671、138579797624），重试日志HTTP404，fail-closed aggregate失败，不变macos-latest；公开incident无匹配，初因未知。一次有界[attempt3](https://github.com/RXY712200/LayerKeySort/actions/runs/45406241385/attempts/3)在原Preview.2 SHA成功，[job138872070896](https://github.com/RXY712200/LayerKeySort/actions/runs/45406241385/job/138872070896) Debug/Release各10/10，aggregate通过。

## Step 7 发现

[Live45439986382](https://github.com/RXY712200/lks0011/actions/runs/45439986382)执行6,480行、10语义门槛。Full Windows/Linux匹配中位比对4.1为0.6291/0.6400，对Preview.1.2为0.9945/0.9981，原生tag-range1.0524/1.0202、list-range0.9085/0.9257。collector旧名preview1在此指Preview.1.2诊断c977b0f。Windows两条件对Preview.1.2超过10%，最差1k short1.424，Linux无。保留离群、语言/adapter限制，live源码不变。新增配对Windows重复不利两条件为long0.994、short0.996，原减速未复现，两采集保留。

[对齐生命周期45441925131](https://github.com/RXY712200/lks0011/actions/runs/45441925131)每主机810观察。Final/flat端点墙钟Windows1.137、Linux1.136；相对threaded-Treap生命周期0.580/0.526。被测Treap逐项相对插入恢复，更好bulk替代未测。早期非对称扫描作为负面方法证据保留。最终分阶段和及含oracle/fixture墙钟，广采集的tree snapshot分配排除sequence内存，另保留诊断计入。Managed等价及任意长期零tag恢复不在范围。

首次sparse构建在诊断归档存在前启动、链接失败，保护重跑成功复现10k/100k/1m及定向回退/恢复。事务应用证据在[授权OrdWeave归档](https://github.com/RXY712200/OrdWeave/tree/research/full-preview3-20261125/benchmarks/research/full-preview3-20261125)，并保留Preview.2归档。

## 指南同步

先前临时指南01bc01e/实现fe00536为运行契约；最后接受Stable源码/文档70d7f0e/1a175f9。下表映射仅测试修正：

| 指南/已审章节 | 现源码/契约 | 断言/边界 |
| --- | --- | --- |
| 导读 §§2,5–6,12–14 | order有界tag/AVL、snapshot独立字节、wire固定无符号移位 | 混合tag/同revision/offset-one，无普遍恢复期限 |
| API §§B–E | 公开头/snapshot/restore，64/14/ownership相同 | view在Snapshot存活时有效，即使live来源已毁；新Handle，回调副作用应用 |
| 内部 §§2–3,9–11 | 跨零非零子序列、bulk新间隔、wire两pass | 恢复间隔/双向回退/literal bytes，无live拓扑持久 |
| 验证 §§1–6,9 | 当前检出/冻结fixture、v4_snapshot两新例 | 精确754b3088c160564f6666ee520284168ee5da6ac7及3ac82d26301ea12876a3ebdba6319fe8233621f3；阶段和/墙钟分开 |

## Technical Debt Impact

活跃记录为[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3)、[七步#27](https://github.com/RXY712200/LayerKeySort/issues/27)，源码决定/否决保留[清单](../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.zh-CN.md)。

| ID | 证据/处置 | 剩余及触发 |
| --- | --- | --- |
| FULL-RI-001 | 保留B64/精确位置/有界tag回退，[live历史](design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.zh-CN.md)，刻意取舍 | 外部长期trace证实持续零回退或结构写主导时重评 |
| FULL-RI-002 | 范围比较完成，6,480 live、原生OM、不利/配对重复保留 | tag-range可胜；目标失败跨匹配主机复发且同语义候选过既有门槛才重评 |
| FULL-RI-003 | 部分，fe00536e固定wire、1,620生命周期、阶段/墙钟分开 | 复制/N指针scratch保留；预算实测要求时测优化bulk树restore或capture |
| FULL-RI-004 | 702原生观察及恢复/故障支持范围内adapter兼容 | staging/WAL主导、官方pin不改；获准集成/真实trace/实测adapter来源不符时重评 |
| FULL-RI-005 | 部分，受保护10k/100k/1m、重复稀疏存活/身份 | 长历史、多百万、任意零恢复未测；有需求且安全准入才扩 |
| FULL-RI-006 | offset-one/恢复tag补范围内测试不足，无已证Full生产缺陷 | 可复现可移植/owner/wire/发布缺陷重开对应不变量/平台 |
| FULL-RI-007 | 测试相对历史/端点类的证据选型，含反例和下游归因 | 无普遍选择规则；新生产消费者、managed等价、可信bulk替代同成功语义时重评 |

建议独立审核及精确门槛后考虑RC入口，Preview.2 Workbench前置已在自身SHA通过，不暗示RC实施/发布。
