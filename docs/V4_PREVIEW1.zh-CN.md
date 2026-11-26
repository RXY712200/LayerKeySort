# V4 Preview.1：显式实时顺序（历史译本）

原文：[V4_PREVIEW1.md](V4_PREVIEW1.md)，英文，固定于 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_PREVIEW1.md)。保留原文，以下为当时未发布状态/实现参数，不代表当前RC接口。[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

本地未发布4.0.0-preview.1；当时公共Stable v3.1.0。本Preview仅显式核心，V3 Tree/OrderedTree/Path/LK1/Group/Batch保持可用不变，V4是单独API而非V3内部新表示。

## 从这里开始

只包含layerkeysort.h，链接普通LayerKeySort::layerkeysort或用生产清单。在该分支：

```sh
cmake -S . -B build/preview1 -DLKS_BUILD_BENCHMARKS=ON
cmake --build build/preview1
ctest --test-dir build/preview1 --output-on-failure
./build/preview1/layerkeysort_order_example
```

VS加--config Release、CTest -C Release并运行build/preview1/Release/。公开头[示例](../examples/live_order.c)展示创建、放置、遍历、比较、删除、销毁。本地安装CMake数字4.0.0，头完整prerelease；数字匹配不代表冻结/Stable。

## 临时API：新增14函数

| 操作 | API |
| --- | --- |
| 容器 | lks_order_create、lks_order_destroy |
| 数量 | lks_order_size |
| 端点插入 | lks_order_insert_front、lks_order_insert_back |
| 相对插入 | lks_order_insert_before、lks_order_insert_after |
| 精确删除 | lks_order_remove |
| 端点 | lks_order_first、lks_order_last |
| 邻居 | lks_order_next、lks_order_previous |
| 借用item | lks_order_item |
| 同源先后 | lks_order_compare |

头共73函数，含59 V3。Order/Handle不透明，insert返回const LksOrderHandle*，必填输出失败NULL，item非NULL，相对anchor同源存活。同item可多个独立occurrence/Handle。

Create OOM NULL，destroy(NULL)无动作；size(NULL)=0，NULL端点/getter/neighbor为NULL；空/越端NULL。Compare要求同源live两Handle和输出，失败清0。Remove item输出可省，失败清NULL。Live foreign为INVALID_ARGUMENT，悬空是调用方寿命错非可检测缺失查询。追加CAPACITY_LIMIT覆盖count/revision耗尽，不重排旧enum。

## Handle与所有权

容器拥有record/block，应用拥有item，库从不free item。Handle代表一次存活插入，经过其他插删、块内移位/拆分、邻块重分配/合并及旋转保持地址；自身删除使全部副本失效，容器销毁使全部失效。重插即使复用内存也是新occurrence。

Handle非Path/持久ID/序列键/指针值标签，应由容器比较，业务身份外部；当时无V4 export/persistence。正反遍历跨隐藏块边界，插入可改邻接而不使存活Handle失效。删除遍历保存next后删current，之后不可用current；Preview.1无版本Cursor。修改/销毁需调用方同步，无内部锁。

## 实际存储与维护

order.c是隐式块AVL，含parent/children、height/subtree count、前驱／后继链接和首尾缓存。块128提交槽+1临时槽；独立稳定record含owner/block/local/item。此版本无Path/label/序列key/物化全局rank。

整体序为中序块串接局部数组；同块local，跨块用parent/subtree计算rank，无分配。空零块，唯一1..128，多块64..128；129拆64/65，删至63优先右邻否则左邻，合≤128则merge，否则均分（63+128→95/96）。单操作最多两数组，无repair级联/欠账；物理导航/局部位置不公开。

Insert先record/可选spare准备，提交仅移位、位置、链/平衡，无分配/回调。失败保留序/count/revision/旧Handle。Remove/merge/redistribution/compare免分配。私有uint64 revision真实修改加一不绕回，非公开export能力；销毁沿链迭代。固定B128/M块，插删结构O(B+log(1+M))、同块compare O(1)/跨块O(log(1+M))、邻居/count O(1)、全遍历/销毁O(N)；不约束allocator延迟，逐项分配/重复局部更新仍成本，非实时/全库摊销保证。

## 验证及初筛方法

[v4_order.c](../tests/v4_order.c)独立扁平occurrence模型，三种子各40000混合，每步私有不变量，每17步精确正反序/抽样compare；另100000端点插/排空、四个8192-item front/hotspot/alternating。模型持Handle直到自身删除，验地址/关联。

边界128/129、首次merge、63+128→95/96；单/双旋、双子root物理移植、重复收缩、同item多次、foreign/NULL、revision/count容量、非悬空故意checker损坏。Failpoint扫构造/record/首块/满split/多块split：create一分配、普通插一、空/满两，无单独AVL节点或准备临时分配。逐失败后成功；失败保留精确块字节/revision/Handle/live字节与块。Remove/drain置下分配失败仍零尝试，最终诊断live零。

诊断不进普通库，计位置重赋、局部数组、不同索引及前驱／后继链接涉及的块、split/redistribute/merge/rotate。每操作≤一split/一repair、两数组、≤2B位置、保守16(h+2)索引范围。索引写非重写所有rank变化resident；checker递归受word位和预算限，生产维护/销毁迭代。

可选[v4_core_smoke.c](../benchmarks/v4_core_smoke.c)，benchmarks ON，单机合理性非最终证据：顺序100000，V3预定义短两层坐标/V4 append，非V3 managed relabel；同邻2000及随机显式10000，同seed17邻居意图，V3生成/拥有Path再克隆插入，V4直接相对Handle，应用flat记账在区间外。两者交替首尾排空、10000compare、精确遍历；V3无公开逻辑迭代，用已备buffer私有ordered-fill，V4公开neighbor，非相同API成本。

Windows QPC，便携C17 TIME_UTC非实时保证；一次含时钟/检查开销，无百分位推断，不推allocator/RSS/普遍收益。最终性能留后续。

### 本地接受结果

Windows x64 GCC16.2/MSVC19.51.36260；严格GCC Release -Wall -Wextra -Wpedantic -Werror、MSVC Release/ASan各14/14含旧V3/新V4，无ASan发现。本地Clang/GCC ASan/UBSan运行库不可用。公开C/C++源码add_subdirectory/离线FetchContent/安装、生成合并/可复现包、四例和原V3头消费者通过，不需私有头。

结构388599操作含120000随机，最大191位置/两数组/13索引块；旋转各形、重分配/merge/双子摘除均达，失败保持提交状态，最终live零。单独GCC Release一次烟测ms，同负载两引擎全遍历checksum相同，按上述不同adapter解释，非通用对照或百分位：

| Workload | Engine | N | Insert total | Remove total | Traversal | 10000 compares |
|---|---|---:|---:|---:|---:|---:|
| Sequential explicit grid | V3 | 100000 | 59.914 | 58.478 | 4.850 | 3.177 |
| Sequential explicit grid | V4 | 100000 | 8.491 | 10.568 | 1.770 | 1.240 |
| Same neighbor | V3 | 2000 | 188.994 | 127.378 | 0.136 | 80.138 |
| Same neighbor | V4 | 2000 | 0.173 | 0.238 | 0.009 | 0.350 |
| Random explicit | V3 | 10000 | 6.908 | 3.310 | 0.258 | 0.609 |
| Random explicit | V4 | 10000 | 1.111 | 1.008 | 0.050 | 0.504 |

列依次负载、引擎、N、插入总ms、删除总ms、遍历ms、10000比较ms。历史结论为explicit范围本地PASS，非发布或最终V4接受。

## 架构来源及延后

[架构](design/V4_ARCHITECTURE.md)/[决定](design/V4_DECISIONS.md)内容自 `3c0373f4bd62287766a84e79e9ee5dbc03194e38` 导入至生产 `c33afa2ac08248bffa6dde51bd590a5d799b394a`，无研究祖先/原型/插桩/采集导入，缺研究链接用固定permalink。Stage3历史验证不是本Preview验证。

当时未实现move、comparator facade、snapshot/export、新key、持久/加载、LK1迁移、V4 Group/Batch、rank/select、锁、增量export、最终API/wire冻结。旧LksOrderedTree是V3非V4 facade；source-marker寿命在snapshot实现时落实，本Preview不加未用marker基础设施。下一Preview在核心上继续，move/facade仅待Preview.2另授权；snapshot/格式/持久/最终性能留后续，不暗分给Preview.2。
