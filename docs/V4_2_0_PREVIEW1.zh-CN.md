# Full v4.2.0 Preview.1 候选（历史译本）

原文：[V4_2_0_PREVIEW1.md](V4_2_0_PREVIEW1.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/V4_2_0_PREVIEW1.md)。保留原件/数据，候选及待验证指当时状态；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

实现 `22544cf283694dd27cef203caba9cf1838845947`，已发布Stable v4.1.0，候选临时。Windows45轨迹消融轮换比较4.1.0、祖先compare、受影响区间位置更新、合并；中位候选/对照0.90538、0.82874、0.71422，合并compare-heavy0.53093、最差0.86120。每条件repeat-zero预热后七新正式进程。见[原始筛选](../benchmarks/research/full-v4.2.0-preview.1/mutation-screen.csv)，属于历史Windows选择，不与下述托管生产混用。

[架构](design/V4_2_0_PREVIEW1_ARCHITECTURE.zh-CN.md)和[交接](FULL_CONSUMER_HANDOFF.zh-CN.md)绑定实现。最后已接受指南PR#25（head `359924df3ea45db38661e32c7528425ce3627bd4`、merge `1a175f91e4c338c2afe6a73a9f8f9c5b149768e5`）说明已发布 `70d7f0ef5857b54381b863d292a213909d87459f`，本周期起始无临时Preview快照。各行共同旧源码70d7f0ef、新源码22544cf2，历史已审与临时章节分开。

| 指南 | 已审章节/源码 | 候选验证/限制 |
| --- | --- | --- |
| 总导读 | live块/Handle、compare、插删/移动；lks_order_compare/v4_place/lks_order_remove/v4_move | live/mutation CTest、bulk后配对；身份不变 |
| API | Explicit compare/寿命、callback/status/cursor/OOM；头/order.c | 冻结64清单、foreign/null-output、Cursor/失败；仅候选metadata |
| 内部 | AVL高/parent、后缀修复、split/merge/redistribution全修、bulk；v4_shifted_locations/v4_locations/snap_bulk_index | 模型、suffix计数、空尾、bulk precedence、snapshot/wire；块数对数比较 |
| 验证 | 来源、构建/证据、v4_mutation、分发/OrdWeave | Windows Debug/Release、后续Release回归、独立托管/固定CI；以下性能范围 |

metadata后Windows GCC Debug/Release各39/39，外部源码/安装/合并C/C++通过。资产两次候选ZIP逐字节相同，SHA-256 `5d5271fdf7610f51f6f07b1f305aeec496e5357b0b2080726bc1523f5fa7c906`。

## 补充本地Windows生产回放

GCC16.2 Release在45冻结轨迹共1,800新进程：候选22544cf2、已发布4.0.0、等价4.1.0、List、Treap。每条件一预热七正式，C检查compare/最终序。中位逐条件比对4.0为0.70148、4.1为0.72003，compare-heavy对4.1为0.54343，最差小short/seed305419896为0.97857。至少一个小update-heavy List更低，最大候选/List1.29427；候选45条件均胜归档Treap。该轮缺同时代OM，初期可能与隔离OrdWeave构建测试重叠。RSS粗进程证据，trace耗时变化非操作延迟。

CMake/Ninja Release建三个固定Full库，获取不可变归档对象；根运行 `python -B benchmarks/research/full-v4.2.0-preview.1/final_c_matrix.py <new-output-dir> <stable40-library> <stable41-library> <candidate-library>`。驱动验轨迹、记编译命令/库和程序哈希、轮换变体、归档成功预期。生产无请求分配计数，raw零表示未插桩。

v4_mutation聚焦检查bulk树重复payload的103,041有序对、foreign/null输出、suffix写、空尾、存活Handle。既有到达OOM/Cursor/结构/managed同套运行，含新case的Windows Release39/39。

隔离OrdWeave以git archive `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`，third_party/LayerKeySort替入 `22544cf283694dd27cef203caba9cf1838845947`，Ninja Release/CTest12/12含checkpoint/history/recovery。官方依赖仍 `70d7f0ef5857b54381b863d292a213909d87459f`，未由此改数据库/adapter。

较早continuation `af5e5ff25d700d38b2381ad80235c10716a55f52` 的CI45352740204通过MSVC x64/Win32、GCC、Clang及sanitizer、AppleClang；新回归是后来测试修改，最终HEAD需另查。

## Windows操作profile

既有v4_trace预期覆盖endpoint/local/hotspot/large move/churn/managed churn/random managed，10,000初始、5,000操作、seed1779033703，每实现在生产/诊断各四重复，共112成功凭据。计时/请求工作分开，复用两个既有Release配置，故只是受控C矩阵的补充。

位置赋值endpoint741,324→44,888，large945,471→276,797，managed churn729,142→226,442；七家族peak请求相同。五家族p99中位均0.2µs，managed churn0.2→0.3，random managed0.45→0.4，接近计时粒度。模型/清理均过，命令/程序哈希/stdout/stderr保留windows-operation-profiles.jsonl。

## 最终托管生产比较

运行45358159898以研究 `a6dfe8319ce98b95ba7cc7a34dec191e328032e9` 完成Windows/原Linux。原Linux C继承墙钟，修正原生Linux45359868900、研究 `42987c5eb8eaa85cfe791d8efc65865951999dbc` 在暂存harness替CLOCK_MONOTONIC，作为主要Linux，原采集另保留。Windows QPC、Rust Instant。两轮Full固定 `22544cf283694dd27cef203caba9cf1838845947`，同adapter/release配置对精确4.0/4.1。

每主要主机2,835行：2,205正式、315预热、315诊断，另五语义fixture调用通过。45哈希匹配，凭据核对stdout计时/存储、成功退出、零诊断清理、准入。全部条件中位/min/max/IQR见[永久采集](../benchmarks/research/full-v4.2.0-preview.1/final-matrix)，[复现](../benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.zh-CN.md)含干净暂存及固定。

| Control | Windows median candidate/control | Linux median candidate/control | Windows / Linux candidate faster |
|---|---:|---:|---:|
| v4.0.0 | 0.62587 | 0.66618 | 45/45 · 45/45 |
| v4.1.0 | 0.69786 | 0.70889 | 45/45 · 45/45 |
| List | 0.03084 | 0.04265 | 43/45 · 45/45 |
| Treap | 0.17246 | 0.13983 | 45/45 · 45/45 |
| Tag-range OM | 1.28146 | 1.36005 | 9/45 · 11/45 |
| List-range OM | 1.23704 | 1.16922 | 10/45 · 15/45 |

列依次对照、Windows候选/对照中位、Linux中位、两主机更快条件数。对4.1 compare-heavy0.57230/0.56894，两主机达固定整体15%/compare25%目标，最差候选/Stable0.82818/0.80700。经典adapter多数领先，尤其比较密集；最大观察优势约Windows某tag-range3.58x。N100,000、seed17、compare-heavy，候选/4.1各请求retained3,875,768、peak4,253,752，经典各13,436,784/13,437,504，是请求表示字节非RSS。

匹配原生公开API探针用同Windows C harness/外部边界，两Full在100,000 append状态跨块中位Stable94.660ns、候选38.483ns，同块3.262/3.250ns。内联祖先为诊断，固定ordinal无维护下限。未测忠实同语言经典实现，C/Rust生成、wrapper、allocator仍混在OM差异；比较不为Full可恢复保证定价，托管描述非普遍选型。

实现达范围内目标，保持待独立审核候选；独立接受另论。完整生命周期、更广数据库归因、外部验证选型不在本包。
