# V4 Stage 3 决策与对抗审查（历史中文译本）

原文：[V4_DECISIONS.md](V4_DECISIONS.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/design/V4_DECISIONS.md)。原件保留；当时临时命名/策略为历史，最终API/wire/policy见[V4_FREEZE](../V4_FREEZE.md)。[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

内容由 `3c0373f4bd62287766a84e79e9ee5dbc03194e38` 导入生产基线 `c33afa2ac08248bffa6dde51bd590a5d799b394a`，没有merge/cherry-pick研究commits。下列Stage3状态/证据为历史，后续实现范围见[V4 Preview.1](../V4_PREVIEW1.md)。状态：选定架构，**并非生产实现/采用**；规范[V4_ARCHITECTURE.md](V4_ARCHITECTURE.md)。基线 `cbf1862c63fa4e23bef4f1383df7e2b464051ae0`，本地分支 `v4-architecture-definition`，当时生产仍v3.1.0。

## 证据来源与分类

- **实验：** Phase1/2报告、实际F/I/S代码、语义trace生成/回放、不可变captures及已有测试。
- **选定：** block core/facade、occupancy、pointer handles、lifetime marker、snapshot domain分离及下列V3概念处置。
- **假定/前提：** caller遵C生命周期、稳定比较器、串行mutable-core访问，提供持久occurrence映射/唯一export namespace，并接受显式全量snapshot成本。
- **未决实现细节：** 签名/status映射、struct packing、wire grammar、portable atomic backend、精确field-write envelope、目标平台空间/性能；主架构已定。

已审 `include/layerkeysort.h`、`src/tree.c` Tree/OrderedTree、私有tree/group/path headers、Path构造/比较/文本、`src/path_order_key.c`、Group/Batch、`src/sort.c`、allocator ownership/failure、API/compatibility/migration/development/design-history。研究审查含 `models.h`、`models.c`、`flat_attempt.py`、`campaign.py`、`replay.c`及prototype tests，非仅摘要。

历史内永久证据：

- [Phase1报告](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ROOT_CAUSE_PHASE1.md)、[capture](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-root-cause-phase1/)。
- [Phase2报告](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ARCHITECTURE_PHASE2.md)、[metadata](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/metadata.json)、[comparison](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/comparison.csv)、[maintenance](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/maintenance.csv)、[validation](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/validation.json)。
- 冻结Phase2测量实现 `1b8e9f39f5d13983690daba493ff3d8900c93c5d`；最终受审研究基线 `cbf1862c63fa4e23bef4f1383df7e2b464051ae0` 含后续验证/文档。

### 证据勘误

Phase2正文称大型timeline五次full events，但maintenance.csv的 `run 12 / timeline-17-100000-100000 / V / capacity 128` 行为 **5 relabel events、4 full relabels**，568153次坐标替换，最大172867区域。以机器记录为准；旧正文作归档保留，不静改。此差异不改outcome D，不代表不同mutation trace。

### 实验建立的内容及边界

Phase1支持generation/validation/cleanup混合尾成本，非仅AVL；不改契约减少relabel会换来Path/LK1增长，未建立下界或普遍不可能性定理。

Phase2验证25scenarios、81paired comparisons/162replays、2.2million语义操作、13.56million重复engine操作，144000prototype differential steps以独立flat order参考。failpoint覆盖prototype insert/split/move/snapshot preparation。这些是历史计数，非Stage3重跑计时。

大timeline（100000初始/100000操作/179886最终resident）：V3 observed max273.2439 ms、I128为0.3208、S为0.0807；requested resident metadata25.190/6.536/11.513 MB。prototype不含共享caller registry或allocator headers，非RSS或最终V4对象大小。

I反复有界reindex writes：long churn143993638 item-field assignments，不是等量不同item；S该处更少index assignment，但每resident index record64-byte。I capacity32/128/256呈不同取舍，单机无通用最佳capacity。

export另计。long-project同mutation history每100操作export，V/I/S build+cleanup共252.73/531.25/453.41 ms；每1000则26.75/49.62/43.28。不能称contextual order让export-heavy应用一律更快。

prototype I的AI1编码epoch/block rank/local ordinal，S的AS1编码epoch/rank，都是实验fixture非最终持久设计。prototype owner-counter/wrap与source destroy后snapshot未形成完整公共生命周期，Stage3明确解决而非意外继承。

## 决策1 — 上下文live order，显式历史export

**选定：** 顺序属于container，live handle在该context查询；snapshot拥有历史row/key关联，无需后来重写。**理由：** Phase1揭generation/repair及增长取舍；Phase2 I/S去掉全部resident坐标替换并验证精确逻辑顺序，而全量export仍实测有成本，应分离工作域，非暗中发布变化live keys。**拒绝：** 每个独立live Path仍作V4规范位置、块标签作持续持久全局序、未验owner/revision就称历史key为current。**成本：** live查询需context，当前storage key需显式capture；export-heavy可选V3，是major契约变更。

## 决策2 — 单block core及独占comparator facade

**选定：** 显式neighbor顺序为存储core，comparator facade拥有私有实例，保稳定等值/不可变binding。**理由：** V3已分manual Path和comparator invariant；Phase2共享identity/neighbor trace支持共同mutation engine，无据维护两算法。**拒绝：** I/S双生产engine、facade暴露可变core、仅为source兼容保两旧type。

托管search找首block maximum严格大于query，再做local upper bound。全局sorted concatenation使maxima单调；duplicates跨块不会漏掉更早greater item或变线性equal-run扫描。resident期间比较器可见数据不可变；任意move仅explicit core。

## 决策3 — 有界pointer arrays及implicit block AVL

**选定：** 初始B128、B+1scratch、独立稳定records、含subtree block count和threaded neighbors的implicit AVL；非单块时occupancy H..B，split一次，underflow merge/redistribute一次。**理由：** Phase2 I有紧凑contextual frontier，Phase1非指AVL为主因；计算block rank不新增top-level relabel；threads让export线性，避免prototype I每row查rank。

**新设计与证据区别：** half-full occupancy/redistribution非prototype原策略，旨在删除churn时按当前population约束metadata。规范代数证明count终止，array/index实现仍需differential/OOM。**拒绝：** 全resident growable数组、全局block-rank标签、级联repair窗、任意块搬迁、推测性B-tree替换。B128可调，非最优证明或兼容承诺。

## 决策4 — 稳定pointer handle及显式C生命周期

**选定：** opaque resident-record pointers，地址跨shift/block变更/rotation稳定；remove终止本次residence，business ID应用拥有；still-live foreign owner拒绝，已free stale handle无效。**理由：** I/S验证稳定resident identity，避免V3 unrelated mutation后重取；强stale检测需带独立lifetime/high-water/generation的registry，当前无实测必要。**拒绝：** handle即Path、序列化pointer、默按pointer数值排、跨进程持久handle、宣称检测dangling。**成本：** caller清除removed handle并跟踪container生命；防御性stale-token查询由business-ID map提供，不是free pointer保证。

## 决策5 — 历史snapshot所有权与source身份

**选定：** snapshot复制association/key，保小型不可变source marker，不保source container/items；marker引用生命周期防source-address ABA。currentness须显式live source+revision；revision不wrap，counter满时actual mutation先失败不改状态。**理由：** prototype证capture/immutable但未完整测source destroy；历史archive须存活且不把复用core地址误认source。**拒绝：** durable rows借item、悬挂旧core指针、未同步全局owner counter、revision wrap。marker refcount须portable atomic lifetime，独立对象destroy不共享隐蔽race；不增加container锁，backend仍实现gate。

## 决策6 — 新snapshot key家族，退出主Path/LK1

**选定：** 新版本家族，显式export domain+flat ordinal，仅同domain有residence-order意义；wire规范/golden review前不冻结字节。**理由：** AI1 block ranks暴露偶然拓扑，复用LK1让历史rank看似live层次坐标；V3 golden保护已发布数据，不能改义。**拒绝：** 挪用LK1v1、export block/local index、用epoch比较跨snapshot item先后、无应用契约宣称namespace全局唯一。caller提供/copied namespace，不同状态需不同namespace，唯一性在core外。**成本：** 应用跟踪domain/occurrence；新core load保order不保source marker/旧handle，保旧bytes需留旧artifact；仅LK1用户经已发布codec/tooling迁移。

## 决策7 — Flat不可变Group/Batch及显式持久化

**选定：** 保稳定immutable item序列、chunk/equality语义；V4移除Group-owned Path/index；通过完成snapshot/occurrence mapping保存/恢复逻辑序，load建新优化blocks。**理由：** Group本已发布不可变序，export显式后无需生成mutable-core topology/层次坐标；持久化无需恢复私有blocks/AVL形。**拒绝：** 假Path兼容带回旧生成成本、存raw struct/pointer、隐式存caller payload、保证resolver/I/O副作用回滚。

## 决策8 — 无分配commit，外部串行化

**选定：** mutation前准备records/spare blocks；pointer移动、pair repair、rotations、revision publish不分配；snapshot/load独立prepare；remove不分配。callback guard拒同core reentry，不可逆commit内无callback。**理由：** V2/V3强原子性及I/S failpoint支持保prepare/commit，不为contextual move放宽。**拒绝：** unlink后分配、部分split状态调用comparator、任意外部payload/I/O事务承诺、hidden locks。counter/count/reference-retention overflow为prepare失败，不wrap。

## 决策9 — F迁移非选定路径

实际 `flat_attempt.py` 的mixed-epoch反例颠倒两item：earlier `(1,0)` 对later `(0,1)`，budgets1/8/32/128均不满足invariant；无已接受F performance frontier。拒该实现，不拒数学上所有独立key OM。contextual blocks避免mixed live key generation和deferred completion义务。新定理/encoding可启未来独立研究，Stage3不因推测重开竞赛。

## 对抗内部审查

| 攻击 | 解决/剩余gate |
| --- | --- |
| 标签隐藏O(N) mutation | 无物化global ranks/Paths；只bounded arrays和log index ancestors。 |
| delete变稀疏high-water blocks | half-full加一次merge/redistribution；无history-sized handle registry。 |
| 级联split/merge | one-record mutation、B+1 split、H-1 pair证明；须实现/测精确过渡。 |
| 跨move源/目标重叠 | 同block特殊路径；跨block先split，source repair用最终neighbors/locations。 |
| 再造top-level relabel | AVL in-order定block序，count算rank，无数值block label。 |
| maintenance使handle失效 | 独立稳定record分配，仅自身remove/destroy失效。 |
| 过度承诺stale检测 | 明确freed-pointer前提，不把不安全owner dereference当验证。 |
| source/items毁后snapshot | 全association复制、marker保留，不读旧core/item pointer。 |
| source地址/counter复用 | 保留identity分配+revision不wrap，历史marker不复用。 |
| 跨snapshot strcmp混淆 | 语义比较查domain，任意raw排序无live意义。 |
| 持久化偶然block拓扑 | flat ordinal snapshot，新load布局，key独立B/AVL形。 |
| 相等项跨块 | 单调maxima/upper bound跳过等值，保已有等值序。 |
| iterator读retired block | 先验revision再读cached state，core须live。 |
| 跨move unlink后OOM | 先备spare，无可失败commit，须完整failpoint。 |
| callback reentry/data变化 | guard同core callback、稳定比较数据契约，无外部事务承诺。 |
| marker无container锁而race | atomic lifetime；同core不许并发mutation/read。 |
| 高频snapshot耗时/内存 | 显式O(N+A)，应用负责retention，多frequency测联合成本。 |
| fragmentation破payload frontier | 测真实allocator headers/RSS/drain，不承诺prototype payload节省。 |

### 剩余风险，非架构矛盾

1. bounded local writes反复发生；更强occupancy repair可增churn，测/调B不改语义。
2. per-record allocation/spare准备可有allocator暂停/碎片，结构界非realtime保证。
3. portable atomic marker/ref/overflow需toolchain/concurrency测试，不能用process-global mutable ID绕过。
4. save/load及domain/occurrence mapping为caller义务，高频full snapshots可能不适合持续索引数据库。
5. raw-handle生命周期误用是无效C行为，不自动检测stale。
6. AVL transplant/threads/cross-move repair顺序需invariant/OOM，prototype测试不自动证明新maintenance。
7. 新wire grammar/namespace width/API/error names当时未freeze，以上语义约束之，不存在第二未决主设计。
8. 外部trace/平台/更长history可能暴露不适应用，不声称普遍性能或library-wide形式界。

## Stage3 验证与退出

本阶段只新增两架构文档；生产/研究source、public header/version/LK1 vectors/captures/build不改。无须新prototype消歧已选count策略，规范含local occupancy论证，但不证明未来pointer/index实现正确。

本次历史run：Windows，重建既有configured目录。

| 配置 | 结果 |
| --- | --- |
| GCC C17、-Wall -Wextra -Wpedantic -Werror，research OFF | CTest13/13 |
| GCC C17 strict warnings，Phase1/2 ON | CTest18/18 |
| MSVC x64 Release，Phase1/2 ON | CTest18/18 |
| MSVC x64 RelWithDebInfo AddressSanitizer，Phase1/2 ON | CTest18/18；无sanitizer findings |
| 三GCC生产examples | 全执行，exit0 |
| 本地链接/diff whitespace/source isolation | links全resolve；diffcheck通过；仅两文档变更 |

GCC16.2.0；MSVC用既有VS x64配置。本地Clang不可用，不声称新Clang/UBSan。目录 `build/v4-root-normal`、`build/v4-architecture`、`build/v4-architecture-msvc`（Release）、`build/v4-architecture-asan`（RelWithDebInfo，`/fsanitize=address`）。CMake重建再CTest `--output-on-failure`，GCC caches保 `-Wall -Wextra -Wpedantic -Werror`。examples：`layerkeysort_example`、`layerkeysort_ordered_example`、`layerkeysort_layer_list_example`。

测试含V3 core/mutation soak/consumer roundtrip/parser torture/RC contracts/workload-benchmark smoke、三个Phase1 strategy contracts、Phase2 I/S differential/OOM、F反例。通过保baseline，不测试未实现V4 API或重测全campaign；未声称新million-insert或外部benchmark。

退出：**PASS，仅架构定义**。live representation/invariant、block/index、handles/moves、export/domain、Path/LK1、persistence/atomicity/ownership/threading/complexity targets/migration/non-goals均单一答案。更强occupancy、marker lifetime、wire/API为明确实现gates，未发现需重开方向的矛盾。后续可在另授权Preview实现，本commit不是Preview.1。
