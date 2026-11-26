# V4 Release Candidate 1 验证（历史译本）

原文：[V4_RC1.md](V4_RC1.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/V4_RC1.md)。原件保留；本文为4.0.0历史RC，不是4.2.0当前RC。[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

当时RC.1为Stage5非Stable，Stable仍v3.1.0，无Preview.6；Stage6发布前需独立RC审核。

## 起点与冻结

Preview.5精确发布 `f413382e96175b79c117956fe4a57212785fb104`，API冻结 `5e4fbbf6d02bff6cba1f049b357ddb8bd329e0aa`。最终注释RC tag绑定验证候选，必须是Preview.5未重写后代，main仍 `c33afa2ac08248bffa6dde51bd590a5d799b394a`。

API/wire/migration清单、fixture哈希及V4_FREEZE.md不变，头仅版本4.0.0-rc.1。64函数/14类型、回调/字段/状态不变，空洞4/6 unknown。私有B64/min32不变，未改生产算法/源码、未加功能家族。规范LF SHA-256：

| Fixture | SHA-256 |
|---|---|
| v4_public_api.json | 57f3ef027b458632c80858d84275217cb1c2ad427aeaedbd27721e95f81708b2 |
| v4_wire_golden.json | 6be573624930b232c237dbd320318fc953cbfe0ee815de9f91ae1c805ee2e7a0 |
| v3_lk1_golden.json | 2ddbc8562ad453aeec869ad36b322794e74d055acae27b79dacab58148735c2e |

## 对抗源码复核

检查插入准备、免分配placement、同/跨块move、满目标、来源修复/移除、邻接链、AVL移植、rank/height/count及revision。不可逆live提交后无分配/比较回调，目标最终安置后最多一次来源repair。旋转/双子摘除/重分配/占用/外来Handle/边界测试保留。

Managed先二分块max再一局部数组，稳定upper-bound和首等值/严格邻居，无等值扫描；比较在结构提交前完成。描述符复制、借用context、重入、删/改/重插测试保留。

Handle在保留resident操作中存活；Cursor先比代数再碰缓存，失败/no-op/查询不失效。测试不传已释放Handle。悬空/伪造指针、禁用的source/output别名仍调用方错；locate重复输出可检出并测，但不保证通用alias探测。

Snapshot仅保留复制关联/namespace/rows及分离marker，不含items/Handle/blocks/context/旧source指针。测试currentness、空/二进制、回调错误/重入、新restore、来源/item到期。原子marker测试为独立拥有Snapshot和保护共享读寿命，不支持可变竞争。

复核x64/x86分配乘积、块/count算术、uint64 revision、marker饱和、namespace扩张、序列偏移；构造边界预留export算术，wire先查可用字节再分配，不序列化native struct/size_t。当时无未解决整数、寿命、提交原子或parser发现。

## 编译分析及分类

GCC16.2 -fanalyzer与MSVC19.51 /analyze编译全部普通生产模块；调查四warning，不能称无warning，未加suppression：

- GCC path.c:163 NULL level：构造已查相同确定capacity-1布局和非空storage。
- GCC order.c:224 v4_place NULL块：报告move假设伪造NULL block resident；合法Handle有block，空插入已准备spare。
- MSVC order.c:310 scratch读：两memcpy初始化total，redistribute两半和为total，界已断言/测试，分析丢关系。
- MSVC immutable_group.c:138 NULL scratch：merge pass需正count多chunk，两buffer已成功；空/单chunk不swap或copy。

分类无A生产修复、无C破冻结阻断；B接受限制如下。扩测试/CI非掩盖不兼容修正，普通GCC严格C17警告仍独立干净门槛。

## 符号与集成

普通CMake显式STATIC。GCC nm见64支持函数及43私有lks_* text（分配stub、迁移Path codec），私有符号非API，无旧live Tree/OrderedTree/Gap/旧Group符号。诊断/历史库分开，无DLL/SO ABI。

C17/C++17 typed编译链接覆盖64，源码/离线FetchContent/install/find_package/合并消费者在GCC、MSVC x64/Win32通过；普通依赖无需Python/私有头/测试/示例/基准。CMake数字4.0.0，头完整RC；静态重建兼容非通用跨工具链二进制兼容。

## 本地矩阵

| 配置 | 结果 |
| --- | --- |
| GCC16.2严格C17 Release | CTest39/39 |
| MSVC x64 Debug | CTest39/39 |
| MSVC x64带符号Release | CTest39/39 |
| MSVC x64 ASan RelWithDebInfo | CTest39/39及长mutation/parser |
| MSVC Win32 Debug | CTest39/39 |

各配置运行七公开示例，源码/FetchContent/安装/合并C/C++17为独立干净构建。本地Clang/GCC ASan/UBSan不可用，不声称覆盖。默认优化MSVC exe消失仍未确认外部异常，未关保护，采用已知通过带符号Release。

确定OOM扫create/insert/split/spare move、managed创建/插入、Cursor、snapshot/namespace/rows/association/marker、deserialize/restore、旧import、Group/Batch build/merge/capture。适用失败保留旧状态/Handle/revision/currentness，清输出无泄漏。免分配remove、无需spare move、locate/compare/key-format/serialize保留failpoint断言。

七LS1、四LKS4SNP1、17已发布LK1向量精确，Win32/x64同fixture。迁移含打乱10k坐标、10k深度加SIZE_MAX终端、重复/畸形/溢出拒绝。Group/Batch等值来源/chunk优先、重复occurrence、snapshot、重入、回滚通过。

## 扩展实验

- Explicit三种子各200,000随机插/删/移，每操作invariant、定期全序/Handle。
- Managed七形状含100,001等值、重复/稳定locate/remove及5,000删改重插；另有managed churn。
- Parser三种子×20,000：180,471 wire、180,000 LS1；GCC/本地ASan接受13,262规范wire、拒绝167,209，接受均精确重编码，无部分结果/崩溃/最终分配。覆盖截断/字节变异/随机长/巨字段/分隔/NUL。
- Marker两来源家族、八线程、每家族64独立Snapshot、来源销毁后160,000受保护共享读，不测未支持可变竞争。

大诊断seed7/cadence0，独立序列和定期checker，每轨迹排空至零：

| 轨迹 | 初始居民 | 操作 |
| --- | ---: | ---: |
| endpoint | 0 | 1,000,000 |
| equal managed insertion | 0 | 300,000 |
| timeline | 100,000 | 100,000 |
| grow/shrink long churn | 4,096 | 1,000,000 |
| distant drag | 100,001 | 100,000 |
| managed churn | 4,096 | 200,000 |
| retained historical snapshots | 4,096 | 100,000 |

元数据跟踪当前resident，历史单独所有。无深度增长/历史尺寸live registry/延后repair队列；清理全部resident/block再source/history至零，RSS保留另论。

冻结聚焦timeline/churn/distant_drag/equal、10k、seed7，每三普通一诊断；equal空开始其余10k。16回放的16维护/比较/清理计数同Preview.5/4。未调时/改历史，Preview.4仍主要性能。

## 远端及发布门槛

Tag前精确最终候选需六job：MSVC x64/Win32、Ubuntu GCC/Clang/Clang ASan+UBSan、macOS AppleClang，各39测试、七例、API、干净消费者。Clang sanitizer另跑长mutation/parser、100k远移、200k churn，GCC验资产。

两个RC源码资产采用冻结五成员ZIP：layerkeysort.h/.c、example.c、LICENSE、README.txt；独立生成逐字节一致，metadata/成员/checksum/篡改/额外文件门槛，无二进制。发布需green SHA注释tag、prerelease true/draft false、GitHub size/digest及下载字节校验。Preview1–5、Stable3.1.0、main身份受保护。

## 接受限制与判断

B：逐record分配、块slack/局部写、回调、全O(N+A)导出/关联增长/历史、Group scratch、RSS保留、调用方寿命/同步、非通用ABI。无硬全库延迟或正式全库定理。真实trace、pool、旧Handle registry、增量export、持久Handle、更多嵌入目标/adapter延后。

当时本地RC入口通过，无已知正确性/OOM/格式/冻结阻断；独立审核、精确远端与资产校验决定发布接受，RC非Stable。晋级另需授权/审核，RC任务不允许新架构/功能。
