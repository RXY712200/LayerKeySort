# 设计历史（中文历史译本）

原文：[DESIGN_HISTORY.md](DESIGN_HISTORY.md)，英文，固定于 [`5b59afff3bf3c8c59cb6c490059280bda195f55d`](https://github.com/RXY712200/LayerKeySort/blob/5b59afff3bf3c8c59cb6c490059280bda195f55d/docs/history/DESIGN_HISTORY.md)。原件保留；本译本保留原阶段事实，不把旧架构当当前 Full。[双语清单](../design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

本文归档 V2 和 Preview 时代的工程决策、表示测量及范围处置，并非当前 V3 公共契约或托管插入架构；这些内容见 [API](../API.md)、[兼容性](../COMPATIBILITY.md)、[架构](../ARCHITECTURE.md)。归档章节中的“当前”指其描述的开发阶段。

V3设计审查也比较了有界块/间接标签与几何原地重标记。块可能减少大规模重标记，却增加第二套顺序表示、分块不变量与查找同步。V3保留几何重标记，后来加入端点进位。这里记录当时决策，不约束未来架构。

## 保留的 Path 契约与稳定 V2 设计说明

### 编码约束

V2 Preview.3 使用完整公共 slot 域 `0..65535`（65,536值），仍以紧凑 `uint16_t` slot 数组及对齐 `size_t` level 数组存于一次 Path steps 分配。公共API以 `unsigned int` 接收/返回slot；构造/append先验证范围再窄化。`uint16_t` 覆盖完整公共域。Path存储随step count及capacity增长；格式化文本按需生成，不逐Path保存。

`src/slot_codec.c` 拥有固定三字符 radix-54 转换。按rank排序的精确字母表为：
`23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz`：先`2..9`，再允许的大写，再允许的小写。字符是 **codec rank**，因此`'2'` rank为零；ASCII字节序等于rank序，仅slot token可按字典序单调比较，无需locale规则。排除易混淆 `0/O/o` 和 `1/I/i/L/l`；不用标点，便于日志、URL、配置与shell复制，`/`保留结构分隔。完整字符顺序及公共文本契约见[API.md](../API.md)。

仅 `src/path_text.c` 格式化Path文本：一个方向字符，每step三个codec字符，step间一个`/`；首绝对level非零或后续level delta大于一时，输出最小十进制metadata。`/`为分隔符，不再是一元level计数。每有效Path只有一份规范输出。V2 Preview.4 的 `lks_path_parse()` 恰好接受该输出；`src/path_order_key.c` 单独拥有版本化持久键。历史Preview间未承诺显示文本兼容；已发布稳定2.x与当时拟定稳定3.x边界保留文档规范语法。level从零连续增长的链，文本恰为 `4 * depth` 字符；一般情况再加实际输出level metadata十进制位数。此项与内部数值Path存储分开。

| 深度 | Preview.2文本长度 | Preview.3文本长度 |
| ---: | ---: | ---: |
| 1 | 3 | 4 |
| 2 | 6 | 8 |
| 4 | 15 | 16 |
| 8 | 45 | 32 |
| 16 | 153 | 64 |
| 32 | 561 | 128 |
| 64 | 2145 | 256 |

这些已核连续level样例显示浅层文本有小幅代价，深层更短。跳级只增加十进制metadata位数，不再随level增加斜杠。针对性测试也格式化首level为 `SIZE_MAX` 的十进制metadata。level为 `size_t`；后续严格递增且delta正，因此level已为 `SIZE_MAX` 时不能再有下一step。

`src/lks_policy_internal.h` 拥有策略值，与Path存储/编码分离。稀疏bulk规则为整数除法 `((index + 1) * 65536) / (blocks + 1)`，block index为`0..blocks-1`。根坐标分布全域，两端留空；单块slot为32768。bulk递归最多26个逻辑Path blocks，与物理索引形状和codec radix54无关，目的是留插入空间。初始online slot为32768。端点目标间距10、preferred/hard depths4/6、local windows8/16/32/64是暂定布局启发式，非正确性上限。两个已有整数slot之间要有严格中间slot，midpoint最小跨度2是算术要求。Stage4删冗余repair gain阈值，并重命名逻辑块/midpoint概念，不改生成布局。Stage5评估depth/window变体，经跨负载与内存检查保留这些值；它们仍为暂定，未形式证明最优。

### 观察到的 Preview.3 效果

| 升序online，N=1024 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| 最大深度 | 4 | 1 |
| repair尝试 | 563 | 0 |
| 局部成功 | 38 | 0 |
| 放弃repair | 525 | 0 |
| 接受更深Path | 520 | 0 |
| full rebuild | 5 | 0 |
| 峰值live bytes | 210176 | 90200 |

| 全等online，N=128 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| 最大深度 | 5 | 1 |
| 局部repair | 5 | 0 |
| 峰值live bytes | 22760 | 11352 |

更宽slot域在这些实测负载显著减少拥塞；强制局部repair与fallback回归仍重要。样例不证明每负载恒深度一、repair过时或最坏复杂度已解决。

### 历史merge代码

V2建立、V3保留的公共 `lks_group_merge` 稳定合并有序item序列：比较器相等时Base先于Incoming，然后以全新Path建新Group，源Group均不变。私有历史v1 owned-base helper则保留独占Base对象并规划Incoming坐标，只用于v1回归证据比较。`tests/property.c` 在 `LKS_ENABLE_V1_REGRESSION_HELPERS` 下测试它；普通CMake生产库不启用，独立诊断target和Visual Studio测试配置启用。冻结 `tests/benchmark.c` 为历史tag分析保留，但当前Visual Studio构建排除，因为ChildBlock测量不适用于新表示。旧 `--stage14.3-frozen-smoke` 属该历史benchmark；当时现行验证使用保留V2 fixtures、新V3 ordered-container测试与 `--public-api-usage-smoke`。该helper非公共API。

以下online repair说明记录 **稳定 V2.0.0 基线**，非当前V3托管插入。稳定V2 online策略在 `src/lks_policy_internal.h`；Tree为单个Path-keyed AVL，物理形状由实现定义，与Path层次无关。bulk先分配稀疏有序Path，再建平衡索引；Group顺序来自in-order。比较器upper-bound按索引高度直接找最后等值item，无需等值run扫描。

online局部repair准备至多64个连续逻辑节点，在不变外界bounds间创建Path，提交前核验depth/order。已有节点保in-order rank，严格递增替换键序列保持当前形状的BST关系；随后新节点链接/rebalance不分配。full rebuild为正确性fallback，交换独立构建Tree。可变索引不再用ChildBlock；物理高度取决节点数，不取决用户Path深度。repair counters仅诊断构建且进程级。在V2 Preview.4中，open-end候选深度不超过六时直接插入：重复生成重叠端点repair窗成本高，却未改善接受坐标。内部gap仍repair；open-end超过六仍先repair再fallback。比较器插入在分配成功后将已拥有候选Path转交node，去掉临时clone，但可能保留Path备用capacity，形成小幅实测peak-memory代价。详计数/候选试验/限制见[BENCHMARKS.md](../BENCHMARKS.md)。

V2 Preview.4支持精确Path删除，使用无分配AVL successor transplant。rekey先分配替换Path和node，再无后续分配地链接替换节点/摘旧节点。delete不压缩坐标；AVL rotations只改物理链接。item仍由caller拥有，已发布Groups仍不可变。

V2 Preview.4含Stage3严格规范显示parser和可移植字节可排序 `LK1:` 持久键。level以最小base-256大端字节表示，降序补码和一元字节数前缀；不序列化 `size_t` 或struct。`src/slot_codec.c` 仍为唯一生产显示slot alphabet所有者，并增加decode。接收平台level超过 `SIZE_MAX` 即拒绝。两parser迭代step，失败释放部分Path。精确v1规范语法和排序证明见[API.md](../API.md)。golden vectors于发布前锁定v1字节；发布后不兼容表示须新key version，key版本独立库Preview版本。

### Stage4范围处置

该阶段core支持单个Path坐标序列化和字节排序，不序列化整Tree、物理AVL、caller payload或应用ID。Tree mutation可改坐标，持久坐标不等于永久item identity。full rebuild保失败原子正确性fallback；Stage5测量并减少重复端点repair，对抗负载仍会触发高成本rebuild，不声称摊还界。

V2 core面向单进程排序。并发独立gap插入、replica convergence和CRDT为可选未来架构，非core前提。公共custom allocator同样可选：当时公共正确性契约无需它，诊断fault injection仍私有。支持CMake、Visual Studio、直接C17源码集成；package recipes和whole-Tree序列化另列可选。`lks_sort()` 为稳定排序便利操作；Stage5与不承诺稳定性的平台 `qsort` 比较后保留。

### Stage5后的本地稳定化处置

本表描述已发布V2 Preview.4 core，供后续roadmap审查，不声称Preview.3已含这些变化。

| 范围 | 处置 |
| --- | --- |
| 物理Tree/Path耦合、ChildBlock移动、等值run扫描、物理subtree repair限制、深物理Tree递归 | Path-keyed AVL和逻辑range repair解决。 |
| 精确remove、rekey、显示parser、可移植可排序Path key | 已在各自文档公共契约范围解决。 |
| online启发式 | 多负载评估，暂留当时值并针对端点repair改进。 |
| full-tree rebuild | 保失败原子正确性fallback；减少端点repair浪费，但duplicate-heavy/alternating仍有实测rebuild成本。 |
| online插入形式化摊还界及固定内存上限 | 未建立，不作公共保证。 |
| 公共custom allocator、package recipes、whole-Tree序列化、distributed/CRDT | 可选集成或不同架构。 |
| 专门 `lks_sort()` 优化 | 延后；稳定便利实现通过正确性和有语境的 `qsort` 比较。 |

benchmark证据与复杂度边界见[BENCHMARKS.md](../BENCHMARKS.md)。未来展示图可用已测全等比较器调用减少及精确负载标签；混合结果不支持宽泛速度声明或胜者图。
