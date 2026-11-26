# Full Preview.1.2：有界块先后扩展（历史译本）

原文：[V4_2_0_PREVIEW1_2.md](V4_2_0_PREVIEW1_2.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/V4_2_0_PREVIEW1_2.md)。原件/数据保留，候选及待审状态指当时里程碑；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

[叠加草稿PR #29](https://github.com/RXY712200/LayerKeySort/pull/29)仅Steps1–2，对Preview.1 `3a2b3eabd0153531e1064457519b40deb4af9842`。Preview.1为独立接受回退，已发布Stable v4.1.0。集成候选源码 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`，精确已接受实现对照 `22544cf283694dd27cef203caba9cf1838845947`。

## 决定与机制

[有界标签架构](design/V4_2_0_PREVIEW1_2_ARCHITECTURE.zh-CN.md)、[预设接受条件](../benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.zh-CN.md)。有界C标签以既有索引块为间接层，AVL保持层次及managed搜索。有界重标记/安全零回退处理集中间隙耗尽，保留分配/失败发布/Handle语义。经典OM语义不涵盖Full managed搜索、快照、导入、历史、失败契约。

初步Windows筛选十个预选条件，各七正式、预热/诊断分开。相对Preview.1，tag整体中位0.8871、compare-heavy 0.3227，无中位更慢条件。仅元数据消融1.0009/0.9892故放弃；完整测试构建显示仍检查被删count，失败构建及只库测量归档，历史仅中点候选保留否决。

10,000聚焦插入324 tag store、八repair、零fallback，单attachment最多两store。中等状态请求live/peak由434,536到437,096。生产QPC p95/p99/p99.9/max为100/200/5,100/20,400ns，对照100/200/5,300/21,200ns。快速事件量化，零中位低于有效单操作分辨率；该hotspot不证明所有负载罕达64块上限，回归另强制耗尽。

独立ordinal预期/相同构建的C17外部调用探针：1,024跨块4.365→1.222ns，100,000为37.262→3.891ns；同块1.219→1.232、3.098→3.139ns。内部祖先和固定ordinal可内联，为诊断非等价动态API。经典对照为锁定Rust原生构建；本有界实验未要求忠实C移植，跨语言仍混合语言/编译器/wrapper成本。

## 同时代矩阵

[研究运行](https://github.com/RXY712200/lks0011/actions/runs/45387318084)使用公开Full及精确锁定OM。45份冻结20,000操作轨迹交叉三规模1k/10k/100k、三种子、五混合。每主机45×8×9观察，含一次预热、七新正式生产进程、一次单独诊断。独立序列预期、QPC/CLOCK_MONOTONIC/Rust Instant，setup/诊断分开，轮换且512 MiB准入、120秒超时。Runtime为含分派/检查回放耗时，非孤立update延迟。

扩展两主机过预设门槛，保留候选供独立审核。相对Preview.1整体匹配中位比降低Windows10.2%/Linux11.4%，compare-heavy降低62.9%/65.7%；保留两项Stable4.1参考目标，两个job成功。归档验证核对6,480行与6,490成功调用凭据。比率<1利于候选，wins为严格更快中位：

| Control | Windows overall / compare-heavy / wins | Linux overall / compare-heavy / wins |
| --- | ---: | ---: |
| Preview.1 | 0.8980 / 0.3707 / 44/45 | 0.8859 / 0.3434 / 45/45 |
| v4.1.0 | 0.6417 / 0.2185 / 45/45 | 0.5720 / 0.2061 / 45/45 |
| v4.0.0 | 0.5963 / 0.2122 / 45/45 | 0.5391 / 0.1965 / 45/45 |
| Tag-range OM | 0.9793 / 0.9108 / 23/45 | 0.9173 / 0.6523 / 28/45 |
| List-range OM | 0.9837 / 0.7502 / 23/45 | 0.8804 / 0.5182 / 33/45 |
| List | 0.0349 / 0.0032 / 45/45 | 0.0460 / 0.0045 / 44/45 |
| Treap | 0.1461 / 0.0833 / 45/45 | 0.1365 / 0.0800 / 45/45 |

列依次是对照、Windows整体/比较密集/胜场、Linux同指标。唯一慢于Preview.1的条件为Windows1,000初始、seed2654435769 hotspot：2.1064对1.8227ms（1.156×）；候选范围1.7106–2.5839、IQR0.7388ms，对照1.7794–2.5822、IQR0.0579ms，重叠，两主机无该profile反复减速。最大OM落后为Windows1k/seed305419896 update-heavy（tag1.591×、list1.584×）及Linux同规模/种子long（1.630×/1.568×）。Linux List也胜一条件，无普遍优势。

两主机相对Preview.1请求live最多升0.545%、peak0.600%。本地留出验证种子1779033703，10k初始/5k修改，端点/hotspot/local/random/churn/long/managed churn，112生产/诊断行全部预期/清理通过。最差家族中位p99.9比约1.21×（random6.00对4.95µs）、1.16×（hotspot5.85对5.05µs），无近反复2×。补充Windows受量化/本地负载限制，托管未测Linux逐操作tail。

[复现/证据](../benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.zh-CN.md)含输入/对照/凭据/采集/资源/不利条件。OM固定j-hui，v4.1.0/v4.0.0/Preview.1/List/Treap新测。请求字节与工作集不同，缺RSS不等于零。

## 契约与验证

64函数/14类型、稳定Handle、owner/callback、同步、revision/no-op、OOM发布、LS1/wire/LK1不变。Restore新Handle并初始私有tag；CMake4.2.0，头标临时扩展。

Windows严格GCC Debug/Release各39/39。聚焦测试内部重复插入、共享payload身份、全序、强制耗尽双向跨块比较；既有混合/OOM/bulk/restore/managed等值/wire/生命周期。清单、C/C++源码/安装/合并、资产本地通过，仅生成本地测试资产。

隔离Windows OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` 替入候选12/12，含持久/checkpoint/history/外部消费者，官方仍 `70d7f0ef5857b54381b863d292a213909d87459f`。集成信号不同于核心正确和升级决定；PR checks为最终HEAD及叠加base遗漏权威。

## 源码绑定指南同步

Stable实现 `70d7f0ef5857b54381b863d292a213909d87459f`，已接受文档 `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`。各行将Preview.1实现 `22544cf283694dd27cef203caba9cf1838845947`、修正文档 `3a2b3eabd0153531e1064457519b40deb4af9842` 映射至候选 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`。

| 指南 | 同步章节/函数 | 证据/范围 |
| --- | --- | --- |
| [总导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | 表示/比较/compare、接块/restore | 动态compare/耗尽/恢复，所有权/操作不变 |
| [API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | compare路径/寿命/错误 | 冻结64清单、状态/身份/foreign，LS1/wire/callback/owner不变 |
| [内部](IMPLEMENTATION_INTERNALS.zh-CN.md) | 块/有界计数/回退、tag_attach/bulk | 不变量/混合/恢复/诊断，有限标签样本外恢复频率未知 |
| [验证](IMPLEMENTATION_VALIDATION.zh-CN.md) | 回归/精确pin/限制/链接 | Debug/Release39/39、隔离12/12、分发/资产、PR实时检查 |

[交接](FULL_CONSUMER_HANDOFF.zh-CN.md)含精确SHA台账/adapter动作，冻结API/集成/迁移/外部所有权/wire无需改变。

## Technical Debt Impact

| 调查 | 结果 |
| --- | --- |
| FULL-RI-001 | 同时代双主机/同语言公开调用隔离推进经典比较；抽象跨语言归因开放 |
| FULL-RI-002 | 有界动态维护/count消融、聚集/回退，本次Steps1–2范围 |
| FULL-RI-003 | 快照兼容已查，节奏/成本优化留后续研究 |
| FULL-RI-004 | 隔离OrdWeave更新，数据库组件成本/官方采用开放 |
| FULL-RI-005 | 请求存储/有界修复已测，大状态保留/生命周期后续 |
| FULL-RI-006 | 结构/耗尽/恢复身份/失败原子已查，更广维护/工程验证开放 |
| FULL-RI-007 | 发布对照/复现保留，更广采用/工程证据开放 |

当时独立接受待定，本扩展不含集成或发布。
