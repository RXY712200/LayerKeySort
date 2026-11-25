# V4 Preview.2：实时移动及比较器管理顺序（历史译本）

原文：[V4_PREVIEW2.md](V4_PREVIEW2.md)，英文，固定于 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_PREVIEW2.md)。保留原件；下面B128、接口数及版本状态属于当时，非当前RC。[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

v4.0.0-preview.2实验prerelease，当时普通用途优先v3.1.0 Stable，API临时/最终性能后续。59 V3函数、Path/LK1、V3算法仍可用，直接基于已发布Preview.1 `21b2902555534d10d97e704161066197cebc13ae`，不合研究历史。

## 公开示例与集成

仅包含layerkeysort.h，链接LayerKeySort::layerkeysort。分支/包4.0.0-preview.2，CMake数字4.0.0，[集成](INTEGRATION.md)。

```sh
cmake -S . -B build -DLKS_BUILD_BENCHMARKS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/layerkeysort_order_example
./build/layerkeysort_managed_order_example
```

VS用--config Release、CTest -C Release、build/Release/。[Explicit示例](../examples/live_order.c)展示保Handle移动，[Managed示例](../examples/managed_order.c)展示绑定、等值插入、locate、Cursor、精确删除，CI运行它们及三旧V3例。

## 显式移动

```c
LksStatus lks_order_move_front(LksOrder *, const LksOrderHandle *handle);
LksStatus lks_order_move_back(LksOrder *, const LksOrderHandle *handle);
LksStatus lks_order_move_before(LksOrder *, const LksOrderHandle *handle,
                              const LksOrderHandle *anchor);
LksStatus lks_order_move_after(LksOrder *, const LksOrderHandle *handle,
                             const LksOrderHandle *anchor);
```

两Handle同Explicit存活，保持地址、item、count，无跨容器move。身份应用拥有，Handle非Path/序列键/持久ID/指针值label，自身删除/容器毁失效，不探测悬空。Self/已邻接/已端点返回OK、无分配/结构维护/revision，在revision满也OK；真实move恰加一，失败保留，耗尽先CAPACITY_LIMIT不wrap。

同块仅移目标区间/修local，免分配、占用不变、无AVL。跨块仅满目标先备spare再摘resident，相对稳定anchor插，最多一次目标split，再用插后邻居修来源，最多一次pair merge/redistribute，可能含新split块；提交无分配/回调地修位置/链/高/count。

B128不变：多块64..128、唯一1..128、129拆64/65；跨块最多四local数组，结构O(B+log(1+M))与介入resident数无关，allocator/callback/OS无延迟界。无全坐标重标、距离扫描、split级联、全normalize或延后队列。

## 比较器facade

LksManagedOrder表示managed而非第二索引，区别V3 LksOrderedTree。

| API | 目的 |
| --- | --- |
| lks_managed_order_create/destroy/size | 拥有facade/私有core |
| lks_managed_order_insert | 稳定upper bound |
| lks_managed_order_locate | 严格前驱/首相等/严格后继 |
| lks_managed_order_remove | 精确occurrence |
| lks_managed_order_first/last | 逻辑端点 |
| lks_managed_order_compare | 插入位置先后非值相等 |

Neighbor/item复用lks_order_next/previous/item，无重复wrapper/record。Managed不能任意移动，因为Explicit需其owner LksOrder*而facade不暴露。创建复制LksComparator描述符、借用callback/context，整个facade寿命须可调用且一致total preorder，resident比较字段不得改变。改key用remove/modify/reinsert，独立操作非原子载荷事务，库不free item。

Upper搜索AVL块max首个严格>query，再块内二分，无则append，纯相等块以平衡搜索跳过非successor扫描；相等保持插序。Lower首max≥query。Locate必填三个不同输出地址先NULL，给首equal或NULL、lower前resident、首strictly greater；重复run非第二equal，query可非resident，空OK/三NULL。

Insert比较调用至多AVL高+块内二分，locate上下界+等值验证O(log(1+M)+log B)×比较器成本，非实时/V3完整插入保证。按Handle删除免compare/分配。语义不一致/载荷变更破坏排序保证，结构仍有界数组非比较器key维护。

## 回调安全及状态

结构提交开始后无compare。同facade重入含read：Status REENTRANT、指针/size NULL/0、busy destroy忽略；共享Handle getter/Cursor也拒core比较时访问。独立容器允许。回调正常返回，不能longjmp/异常跨C；LksCompareFn只负/零/正，无error/OOM模型。

Guard非mutex，调用方同步修改/销毁及context/payload。追加INVALIDATED/REENTRANT不重排旧值。Insert必填输出NULL、remove可省NULL、compare必填0；非法INVALID_ARGUMENT、OOM OUT_OF_MEMORY。NULLdestroy安全、NULLgetter0/NULL，拒live foreign，过期访问是调用方寿命错。

## 版本Cursor

不透明LksOrderCursor一次分配调用方毁，Explicit/Managed create共实现，reverse仅0/1，输出先NULL，捕获revision及首/尾；分配失败不改source。Next依方向推进，末OK/NULL，读缓存前比revision；真实插删移INVALIDATED/NULL含缓存已删。失败/no-op/query保留。返回Handle寿命独立Cursor，destroy(NULL)安全。Cursor借source、不可活得更久，revision非毁source探测；普通邻居适用于调用方控修改、先存next再删。

## 验证与诊断

[修改测试](../tests/v4_mutation.c)独立flat模型：三种子各30000混合插删移，每步checker、频繁精确正反/关联和pair；100001居民2000远移。定向local双向/no-op/相邻满目标split+source merge/redistribute/foreign/revision。

Managed七分布ascending/descending/random/equal/低基数duplicates/alternating/hotspot，equal100001其余4096，检查精确occurrence序非仅sorted；5000删改重插测稳定/Cursor。Remove在下一分配失败下运行且compare计数不变。Insert比较界height+8，locate2*height+17；重入嵌套insert/locate/remove/compare/getters/cursors/destroy均拒。OOM扫spare、facade/core创建、空/非满/split插、Cursor。满目标move失败保持块字节/全部Handle/revision/序/live统计/旧Cursor，免分配move/remove零尝试，最终live零。

私有计数位置/local数组/不同index-thread块/split/repair/rotate/no-op/跨块，普通库无。每move≤4B位置/四数组，index有界祖先路径，无公开diagnostic ABI。

本地GCC16.2严格C17 -Wall -Wextra -Wpedantic -Werror、MSVC19.51 x64 Release/ASan各16/16含未改Preview1/V3，无ASan发现；Clang/GCC sanitizer运行库不可用，远端另门槛。五例、C/C++源码/离线FetchContent/安装/合并复现消费者过。保73签名加17共90。

Explicit观测42186 move，41113跨块、20no-op，最大319位置/四数组/12index；独立random screen20index，合并观测最大20，全部过结构界。Managed单insert最多14compare、locate37，但断言用高界非机器max，最终零。

## 初筛

Benchmarks ON的layerkeysort_v4_mutation_smoke为生产时间/compare，diagnostics另计结构/分配，含插桩耗时不可比。生产零diagnostic列为不可用。Seed17、20000居民/操作：反复local/distant/random move、交替删重插、managed ascending/random/equal/duplicate/alternating，smoke为小CI。输出操作时间和、最近索引p50/p99/max、compare、赋值、数组/index/split/repair/逐操作max/分配；含时钟/检查，Windows QPC，C17 TIME_UTC不保单调，排除应用buffer，单机不推最终优越/实时。

一次Windows x64 GCC16.2 Release[采集](../benchmarks/results/v4-preview2-smoke/)：20000操作local0.597、distant1.940、random3.362、删重插1.750ms；managed插ascending2.060、random5.082、equal2.063、duplicates3.072、alternating2.291ms，equal147351 compare。另CSV结构/分配，一次含开销，低分辨零非免费。

## 延后

Snapshot/key/domain/namespace/persistence/save/load/marker寿命/LK1迁移/V4 Group/Batch/增量export待Preview.3或后续显式授权；rank/select、锁、最终冻结也无。live Cursor/Handle不暗示这些能力。见未改[Preview.1](V4_PREVIEW1.md)、[架构](design/V4_ARCHITECTURE.md)、[决定](design/V4_DECISIONS.md)。Preview1保原发布前文字，该里程碑已以[v4.0.0-preview.1](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0-preview.1)发布。
