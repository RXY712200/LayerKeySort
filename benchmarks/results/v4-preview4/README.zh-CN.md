# V4 Preview.4 工作负载采集证据（历史译本）

原文：[README.md](README.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v4-preview4/README.md)。原件/原始测量不变；[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。以下版本状态属于历史采集：实验性证据，当时v3.1.0仍为Stable。本目录为完整未调B128，tuned-b64/为完整最终B64，未以有利子集替换任一采集。

## 环境与来源

Windows build26300、AMD Ryzen 9 9955HX（16核/32逻辑处理器）、GCC16.2.0、严格C17、Release -O3 -DNDEBUG、QueryPerformanceCounter。短预热后三次顺序生产运行，另一次插桩提供分配/维护。种子7/19、8字节不可变occurrence关联。行是单次运行非预聚合。元数据保留采集时HEAD（Preview.3）、实际工作树SHA-256及后来提交源码的单独标识。原始私有B128，最终默认B64。

轨迹从归档研究词汇独立重建，为确定性合成工作负载，不是实际应用会话或研究代码逐字节复刻。扁平指针数组预期使用不可变应用occurrence ID/位置，独立于块rank、Path文本、快照ordinal，验证精确正反序、resident Handle、稳定相等、revision。前64操作、每1,024及最终全查；每个诊断mutation都断言结构工作界。逐行检查导出关联，首次导出另加载/恢复并对模型检查。

## 矩阵与负载

每集合456工作负载行、264 Group行、456 phase/reference行。全部已记录预期通过，最终诊断清理live字节/块为零。以下19家族各在1,024、10,000、100,000操作执行三次计时和一次诊断：

- ascending/descending：单调比较器键；random伪随机；equal同键；duplicates为16键。
- alternating低/高到达；priority为优先家族名下随机比较器到达，不是另采调度器。
- hotspot显式中部插入。
- timeline/large_timeline为显式插/删混合，每五次一次删除；large仅同合成形状的规模名称。
- local在末尾32位置内插入，删除比例相同。
- churn交替删/插；delete_reinsert复用应用item/ID但取得新resident Handle。
- managed_churn为managed 16键churn，移除首个等值项。
- local_drag移动1–7邻位；front_back交替首尾移动。
- distant_drag/large_moves为远距Handle移动，large为规模标签非独立实采轨迹。
- hotspot_drag持续移入小中部邻域。

纯插家族从空开始，其他普通家族初始N再执行N操作。额外包括1M端点、1M等值、初始10k的1M长churn、100,001居民的100k远移、100k来源在10k修改中保留八Snapshot。timeline/churn/distant_drag在10k初始/10k操作下对完全相同回放使用none/10k/1k/100导出节奏。

八个公平比较器家族含10k/100k V3参考，键生成、稳定上界、首等值删除相同；V3删除含locate，因为没有resident-handle move对应。不虚构V3 Handle移动对照。V3 LK1与V4 LS1契约不同；V3逐行格式化还含临时分配，不是直接数据库基准。

Group：random/all-equal，N=1,024/10k/100k，三生产一诊断，稳定build/两源merge/batch（chunk1,024）/merge-all/V4 capture/cleanup。独立key+input-ID排序预期验证等值优先。groups-corrected.csv是规范最终采集：早期harness cleanup行携带前次比较计数，原groups.csv保留，但其cleanup comparison列无效；修正仅重置报告计数、不改生产，全部264行重新采集验证。

## 解释边界

Mutation计时排除模型维护/验证，但其cache/调度影响仍在。百分位/总量含初始构建，phase/reference分开。RSS为全进程工作集，不是隔离库内存或碎片。rss_final在释放fixture item/model/handle数组之后、延迟采样buffer释放之前；分配器保留、时钟样本、OS使RSS不必归零。请求库字节门槛在容器及全部保留Snapshot销毁后测；空live对象和有意保留历史不是泄漏。生产零诊断列表示未插桩。最大暂停是样本非最坏界，单机不作通用速度声称。

## 最终B64总量

三生产运行中位ms，存在初始构建时计入；原始行另给尾、phase、分配、存储及导出。

| Family | 100k operations (ms) |
|---|---:|
| ascending | 9.554 |
| descending | 17.530 |
| random | 39.681 |
| equal | 9.714 |
| duplicates | 17.545 |
| alternating | 17.021 |
| hotspot | 11.286 |
| timeline | 33.532 |
| priority | 38.204 |
| local | 20.398 |
| churn | 24.944 |
| large_timeline | 28.912 |
| local_drag | 14.320 |
| front_back | 24.398 |
| distant_drag | 28.617 |
| hotspot_drag | 26.493 |
| large_moves | 26.371 |
| delete_reinsert | 25.635 |
| managed_churn | 39.166 |

Family为上述家族标识，右列为100k操作毫秒。Large timeline（100k初始+100k操作）28.912ms，最多95 local赋值/2 arrays/15 index更新，请求resident 7,032,864字节。长churn（10k初始+1M操作）113.451ms，当前461,584/峰值918,240字节，最多95赋值/16索引更新。全部默认B64诊断最大160赋值、4数组、29索引更新，单mutation最多一次split及一次来源对repair。均为观测计数及独立逐操作结构断言，不是墙钟上界。

生产1M端点工作集中位111,550,464字节；诊断库峰值约50.25MB。诊断对象比正常core多4,280字节；应看分配/存储CSV，不混同插桩RSS与正常生产。

100k存储拆分3,200,000 record字节、912,792 block-index（1,563块）、4,344插桩容器字节。平面不可变Group为800,000指针数组+32对象字节；其Snapshot为1,600,000 row、800,000关联（容量1,048,576）、8 marker、1,600,026 wire、2,500,000聚合LS1字节。Group/live wire逐字节相同。八保留Snapshot有意使用25,306,928请求字节，最终全释放。另一个修改保留轨迹保留13,520,000 row和8,388,608关联容量，清理前总27,258,552字节。

## 最终B64导出节奏

中位ms、10k初始+10k操作；修改/capture/format/serialize分开，load/restore/query/cleanup另记。

| Family | Cadence | Mutation | Capture | Format | Serialize |
|---|---:|---:|---:|---:|---:|
| timeline | none | 2.082 | 0.000 | 0.000 | 0.000 |
| timeline | 10000 | 2.020 | 0.205 | 0.059 | 0.070 |
| timeline | 1000 | 2.031 | 1.381 | 0.491 | 0.572 |
| timeline | 100 | 2.186 | 13.138 | 5.082 | 5.656 |
| churn | none | 1.741 | 0.000 | 0.000 | 0.000 |
| churn | 10000 | 1.871 | 0.141 | 0.037 | 0.047 |
| churn | 1000 | 1.778 | 1.168 | 0.367 | 0.429 |
| churn | 100 | 1.981 | 10.258 | 3.834 | 4.613 |
| distant_drag | none | 1.847 | 0.000 | 0.000 | 0.000 |
| distant_drag | 10000 | 1.833 | 0.144 | 0.037 | 0.048 |
| distant_drag | 1000 | 1.890 | 1.334 | 0.411 | 0.458 |
| distant_drag | 100 | 2.228 | 10.822 | 3.933 | 4.997 |

列依次为家族、导出间隔（none不导出）、修改、捕获、格式化、序列化。频繁O(N+A)全导出可主导修改，有界局部mutation不使Snapshot增量或任意频率都便宜。

## 有限容量筛选及唯一调优

B64/B128/B256在10k/100k的八种相同形状；较大筛选三重复轮换进程。capacity-large.csv含全部96行。100k的B64在八条件均比B128低约7–27%，local赋值最大约减半，而请求字节/分配流量升约1–3%。结果B：唯一修改选B64/min32，bulk恢复采用同一策略。不做第二轮一般调优、pool或重设计，完整调优前/后均保留。

## 本地发布门槛

严格GCC C17 Release、MSVC x64 Debug、MSVC x64带符号Release、MSVC x64 ASan RelWithDebInfo、MSVC Win32 Debug各34/34。七示例在GCC Release、MSVC带符号Release及MSVC ASan通过。GCC/MSVC分发检查通过C/C++源码、离线FetchContent、安装及独立合并消费者和可复现ZIP。本地Clang/GCC ASan/UBSan运行库不可用；远端是独立门槛，不能由本地推出。

默认MSVC Release调查：某既有修改诊断exe链接后存在，两秒后仍在，稍后未经执行已消失；未发现仓库/生成后构建删除步骤。检测到安全产品，但无可访问事件证明哪个外部主体删除，未关闭保护。受影响配置是Not Run，不是断言失败；干净Release加 `/DEBUG /INCREMENTAL:NO` 为34/34。保留该环境限制。

## 复现与剩余问题

见benchmarks/run_v4_matrix.py、run_v4_capacity.py、公开API v4_trace.c/v4_groups.c及独立存储拆分v4_storage.c。collector拒绝替换既有采集。元数据保留原物理摘要（Windows换行），另记生产commit避免自引用哈希。

剩余成本为逐record分配、块头/slack、局部写、回调、全导出、历史、scratch及进程分配器保留。没有引入resident Path/LK1增长、全坐标改写或延后维护队列。当时Preview.5需明确复核命名/借用、LS1/wire冻结、集成及证据接受。本里程碑不加入Stable/RC、rank/select、持久Handle、增量导出、锁、数据库适配器或CRDT。
