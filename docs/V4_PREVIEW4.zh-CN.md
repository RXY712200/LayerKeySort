# V4 Preview.4：扁平不可变集成及工作负载收敛（历史译本）

原文：[V4_PREVIEW4.md](V4_PREVIEW4.md)，英文，固定于 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_PREVIEW4.md)。原文保留，临时名字/版本状态为历史，不是当前公开API。[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

v4.0.0-preview.4实验，推荐Stable当时v3.1.0。将Group接入live/历史架构，不冻结API名/LS1/wire，Preview5需显式决定。

## 表示及所有权

LksImmutableGroup拥有对象、借用非NULLitem扁平数组、count、callback guard、可选惰性marker，无Path/Tree/live record/block/AVL/mutable order。本x64 GCC对象32字节，非空N指针8N，应用payload/allocator另计，重复指针合法，毁不毁item。

V3 Group/Batch含Path保持不变，新类型为独立临时新增，非alias/转换。121函数=59V3+62临时V4；不改V3签名/status/Path/LK1字节。

## Group契约

```c
LksStatus lks_immutable_group_build(void *const *items, size_t count,
    const LksComparator *comparator, LksImmutableGroup **out_group);
void lks_immutable_group_destroy(LksImmutableGroup *group);
size_t lks_immutable_group_size(const LksImmutableGroup *group);
void *lks_immutable_group_item_at(const LksImmutableGroup *group, size_t index);
LksStatus lks_immutable_group_merge(const LksImmutableGroup *base,
    const LksImmutableGroup *incoming, const LksComparator *comparator,
    LksImmutableGroup **out_group);
```

Build复制指针稳定O(N log N)merge sort，原数组/items不变，equal保输入序。空合法items=NULL/count0，仍必填compare/output。Merge新数组O(Nbase+Nincoming)，源不变，equal base优先、源内序保；自merge合法，按同规则每项两occurrence。

Comparator描述符操作期间复制、不保留，代码/context/payload应用拥有。Merge须两源语义兼容；read不compare或隐藏兼容扫描。改比较字段不重排，后续merge兼容由应用保证。Size/item O(1)，NULL/invalid/busy零/NULL，越界NULL；item指针应用寿命，索引读位置非永久ID。

## Batch契约

```c
LksStatus lks_immutable_group_batch_build(void *const *items, size_t count,
    size_t group_size, const LksComparator *comparator,
    LksImmutableGroupBatch **out_batch);
void lks_immutable_group_batch_destroy(LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_size(const LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_group_count(const LksImmutableGroupBatch *batch);
size_t lks_immutable_group_batch_group_size(const LksImmutableGroupBatch *batch);
const LksImmutableGroup *lks_immutable_group_batch_group_at(
    const LksImmutableGroupBatch *batch, size_t index);
LksStatus lks_immutable_group_batch_merge_all(const LksImmutableGroupBatch *batch,
    const LksComparator *comparator, LksImmutableGroup **out_group);
```

Group_size非零，连续chunk各稳定排序。Batch拥有指针表/Groups，借chunk至Batch毁，禁止自行毁借chunk。总item数、配置大小、chunk数不同getter；NULL/busy零/NULL，空无chunk合法。Merge-all串sorted run，O(N log K)稳定指针pairwise，K>1一N-pointer scratch，早chunk优先，无中间Path/index，新结果独立可长于Batch，源不变。

## 共用历史导出

```c
LksStatus lks_immutable_group_snapshot_capture(const LksImmutableGroup *group,
    const LksSnapshotOptions *options, LksSnapshot **out_snapshot);
int lks_immutable_group_snapshot_is_current(const LksImmutableGroup *group,
    const LksSnapshot *snapshot);
```

单次flat遍历无compare，复用live capture事务O(N+关联+callback)。Namespace/关联/LS1/wire恰[Preview.3格式](V4_PREVIEW3.zh-CN.md)，非第二协议；相同序/items/域/关联产生跨live/Group相同wire/LS1。

Group revision0，首成功全部行就绪后才惰性原子marker，后续retain，失败首capture不留marker、retain失败不发布。捕获对存活原Group current；load无marker不current。Group毁后历史关联仍可读，不保item/source地址，禁止对已毁Group查。Capture逻辑只读但改marker/guard，外部串行同Group；无mutable线程保证；Snapshot并发读保护寿命，独立快照可并发释放共享marker。

## 失败与回调安全

所有拥有输出调用在校验/失败前将可用out清NULL。非法/不可表示INVALID_ARGUMENT，OOM OUT_OF_MEMORY，marker耗尽CAPACITY_LIMIT。发布前准备全部存储，失败源可用。注入覆盖空/非空build、sort scratch、对象/数组、各部分Batch chunk、merge scratch、capture行/关联/惰性marker/retain容量；失败恢复live bytes，最终成功清理bytes/blocks零。

Merge比较时guard两源，merge-all guard Batch/每chunk，capture guard自身；同源递归capture/merge REENTRANT，busy read零/NULL，busy destroy忽略，借chunk忙时Batch毁也忽略。其他独立操作正常，无global锁/异常longjmp恢复/悬空探测。Callback span至返回后即时复制仍活，自动局部数组不合法。

## 生产证据与方法

[采集](../benchmarks/results/v4-preview4/README.zh-CN.md)含完整未调矩阵、分开计时/诊断。严格Release、benchmarks ON、可选私有capacity screen，然后：

```sh
python benchmarks/run_v4_matrix.py --build build --output build/new-v4-evidence
```

输出不可已有runs.jsonl，无研究祖先合入。新实现确定性应用形状合成轨迹，用归档[Phase1/2词汇](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ARCHITECTURE_PHASE2.md)，不是人类会话或旧研究逐字节复刻，定义见证据指南。

独立flat模型用不可变occurrence ID、upper-bound预期或应用位置，查count/精确正反/等值/Handle/revision。私有不变量早期、每1,024、最终查，每mutation断言结构界；每export全关联，首次load/restore序。Oracle失败终止，不记录为计时证据。

Windows QPC/POSIX CLOCK_MONOTONIC，短预热后三次，排模型/验证但cache/调度仍受影响，诊断不预测生产延迟。百分位含初始构建，另build/op总；接近量化不可作微百分比结论，max观测非硬界。RSS全进程含应用数组/allocator，非隔离碎片，final RSS仍含延迟sample buffer，精确时点见指南。

## 唯一调优：B128→B64

预定B64/B128/B256八相同负载10k/100k，大筛轮换；100k B64八条件中位均低约7–27%，local赋值最大约半，live请求/分配增约1–3%，足够广取舍才改策略。Outcome B唯一有界调优：B64/min32，不加pool/结构/comparator/Group算法/export调优。Bulk同政策65→33/32，LS1/wire不编码块。完整B128及调后独立保留，私有占用/OOM fixture改按capacity/min表达，公开/golden不变，无第二轮。

## 适用及Preview.5决定

Live无Path/LK1增长、全坐标改、tombstone registry、延后维护。普通mutation有界local数组+对数块index，compare/record allocation/local写仍成本；远move用Handle不扫距离，非constant墙钟或独立callback/allocator/平台的完整O(log N)API界。

Snapshot O(N+A)，频繁全导出可主导，八历史有意放大存储；请求排item/allocator。空对象仍占对象及捕获后marker；zero live是容器及显式保留历史销毁后，不能把有意保留/存活空对象当泄漏。

成本含逐record分配、块头/slack、local写、compare、row元数据/容量、外部LS1、Group scratch、进程保留/清理。V3仅公平稳定comparator/Group/Batch对照，无Handle move对应，LK1/LS1契约不同，单编码size非数据库端到端。

Preview5须决定临时Group/Batch名/V3展示、callback/borrow兼容、LS1/wire冻结/迁移、集成配置、可复现证据和阻断。本轮无rank/select/锁/身份所有权/持久Handle/增量export/DB/CRDT/事务/自动namespace/Stable/RC。

## 已验本地门槛

GCC严格Release、MSVC x64 Debug/带符号Release/ASan、Win32 Debug各34/34，七例GCC/带符号MSVC/ASan过。C/C++源码/离线FetchContent/安装/合并及可复现包GCC/MSVC过，本地Clang/GCC sanitizer不可用。默认MSVC Release一链接exe延后未经执行消失，外部主体不明，无仓库删除步骤/可取安全事件，带debug链接metadata全测过、保护不变。限制及修正Group cleanup计数详证据指南。
