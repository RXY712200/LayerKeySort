# V4.0 冻结：最终 Preview.5

[英文原文](V4_FREEZE.md)。这是历史冻结记录，英文原文保留。以下版本状态均指当时。

`4.0.0-preview.5` 是最后一个实验 Preview。Stable 当时仍为 v3.1.0。接下来进入阶段 5 Release Candidate；没有 Preview.6。冻结提交 `5e4fbbf6d02bff6cba1f049b357ddb8bd329e0aa`。后续发布标签指向最终文档和打包候选，其 API 必须符合这一冻结版本。[清单](../tests/fixtures/v4_public_api.json)包含 64 个函数、14 个类型、显式状态值、回调签名和公开字段。tools/verify_v4_api.py 检查注释和发布文本以外的头文件标记，RC 不得静默更新。

## 源码、ABI 与 V3 策略

安装版 layerkeysort.h 仅提供 V4 的上下文实时顺序、托管顺序、平面 Group、快照、排序与状态，以及严格旧格式导入。公开 Path/Tree/OrderedTree、物理导航及 LK1 编解码已移除；V3.1.0 继续独立提供。私有迁移头不安装。旧实时 Tree/Group/gap/bulk 仅用于历史测试及基准，不参与生产库或合并源码。导入保留已发布的严格 LK1 解码。

4.x 保留名称、签名和声明布局，包括 LksComparator、LksSnapshotOptions 与 LksV3Lk1ImportEntry 的字段类型与顺序。相同架构、工具链及 ABI 设置保存这些布局。通常重新编译静态源码；不承诺 MSVC/GCC/Clang、CRT、结构体打包设置或跨架构的通用二进制 ABI。不得持久化原生结构；无预构建共享 ABI。V5 可以实施明确记录的主版本源码更改。新增 4.x API 需要在 4.0 以后有意修订，RC 冻结中不得未经审核添加。

| Preview.4 | 最终 Preview.5 | 原因 |
| --- | --- | --- |
| LksImmutableGroup | LksGroup | 统一平面不可变 V4 模型 |
| LksImmutableGroupBatch | LksGroupBatch | 统一 V4 Batch 模型 |
| 14 个 lks_immutable_group_* 调用 | lks_group_* / lks_group_batch_* | 移除过渡前缀 |
| V3 实时、Path 和旧 Group API | 从主头文件移除 | 主版本语义变化，V3 仍独立提供 |
| LEVEL_LIMIT / NOT_IMPLEMENTED | 移除，4/6 留空 | V3 专用状态 |

不提供源码别名。其他名称区分实际流程。分配的游标用于检测修改。容量、秩、拓扑、标记、修订号、诊断及故障注入点都是私有接口。

## 状态冻结

| 名称 | 数值 |
|---|---:|
| LKS_STATUS_OK | 0 |
| LKS_STATUS_INVALID_ARGUMENT | 1 |
| LKS_STATUS_OUT_OF_MEMORY | 2 |
| LKS_STATUS_BUFFER_TOO_SMALL | 3 |
| LKS_STATUS_NOT_FOUND | 5 |
| LKS_STATUS_INTERNAL_ERROR | 7 |
| LKS_STATUS_ALREADY_EXISTS | 8 |
| LKS_STATUS_CAPACITY_LIMIT | 9 |
| LKS_STATUS_INVALIDATED | 10 |
| LKS_STATUS_REENTRANT | 11 |
| LKS_STATUS_DOMAIN_MISMATCH | 12 |


状态说明为静态借用字符串。未知整数（包括 4/6）返回 "Unknown status"。NOT_FOUND 允许解析回调报告找不到应用对象；ALREADY_EXISTS 表示导入坐标重复。INTERNAL_ERROR 为防御性检查，无法检测未定义输入。若不重新开启冻结，RC 不增添、重编号或改释状态。

## 所有权与生命周期

| 数据 | 所有权和失效 |
| --- | --- |
| Order/ManagedOrder | 调用者拥有结果，库拥有结构并借用对象，销毁不释放对象 |
| 对象 | 调用者拥有，驻留或回调读取时须存活 |
| Handle | 借用的驻留实例；删除其自身或销毁来源使所有副本失效 |
| 游标 | 调用者拥有且借用来源；先于来源销毁；销毁不改变来源 |
| Group | 调用者拥有平面指针副本，借用对象 |
| Batch/分块 | 调用者拥有 Batch，Batch 拥有分块；不得销毁借用 group_at 视图 |
| 合并结果 | 独立的新 Group，不依赖来源寿命 |
| 快照 | 调用者拥有不可变复制行与字节；来源和对象销毁后仍有效 |
| 比较上下文 | 调用者拥有；Group/sort 按操作借用；ManagedOrder 保存描述符并借用上下文直到销毁 |
| 关联回调字节范围 | 借用直至返回后立即复制；须保持有效 |
| Resolver 结果 | 调用者拥有非空对象，恢复后借用，回滚不释放 |
| 命名空间输入 | 复制应用选择的非空域 |
| 快照命名空间/关联视图 | 借用直到快照销毁 |
| 序列化缓冲区 | 调用者拥有，库写入但不保存缓冲区 |
| 旧键及输入数组 | 仅在导入调用期间借用，调用结束后不保留 |

必要输出须为有效且独立的存储，不得与仍在使用的输入数组、对象存储或描述符重叠。复制源和目标不得重叠。悬空、重复销毁、来源无效、读/销毁竞争及异常或 longjmp 逃逸均由调用者负责，非可检测状态。

### Handle 与游标

无关插入删除、自身移动、分裂合并再分配和旋转均保持 Handle。删除自身或销毁来源使其失效；禁止用已失效 Handle 测试错误处理。没有失效注册表、持久 Handle、业务身份或跨容器移动。有效外来 Handle 返回 INVALID_ARGUMENT。指针数值不表示顺序。游标借用来源并记录修订号。实际修改使其失效，失败、查询和无操作保持有效。在访问缓存的句柄前先核对修订号。Reverse 只能为 0/1。到末尾返回 OK+NULL，修改后返回 INVALIDATED+NULL。游标不具备线程安全性，不得晚于来源销毁。

### 回调与线程统一契约

比较器和上下文保持可调用、一致；驻留对象参与比较的字段保持稳定。编辑数据后 ManagedOrder/Group 不会自动重排，应先删除、编辑、重新插入或重建。合并要求兼容的有序来源。回调在受保护的完整状态下执行，不会观察到尚未完成的提交。回调重入同一来源时，返回状态的 API 返回 REENTRANT、getter 返回零或 NULL、销毁被忽略。仅 Handle 访问仍受其生命周期约束。无关容器可继续使用。回调须正常返回，不得使 C++ 异常或 longjmp 逃逸。排序和构建期间尚无已发布的来源，输入数组与描述符须保持有效。恢复读取存活的不可变快照，未完成的输出不公开。应用回调的副作用不回滚。

调用者同步可变来源的访问，包括对修改或回调活动并发的读取。保护寿命后，Group 可并行读取。合并和捕获的私有标记或守卫活动须与读取、销毁串行。快照在生命周期受保护时可并行读取；禁止并发销毁和读取。来源标记的原子操作仅管理生命周期，不提供容器锁。

## Group 与 Batch

构建稳定排序复制的非空对象指针，输入保持不变；空数组有效但仍需比较器。无 Path/索引，item_at 为 O(1)。合并创建新 Group，等值时 Base 在 Incoming 前，自合并允许。Batch 拥有稳定排序的输入分块，分块大小大于零；合并保持较早分块优先，结果可独立于 Batch 存活。Group 捕获使用修订号 0，延迟标记仅成功时提交。历史快照独立存活；加载的快照没有当前性；LS1 和线格式不能重建来源。

## LS1 版本 1（冻结）

```text
LS1.<nonempty even-length lowercase namespace hex>.<16 lowercase ordinal hex digits>
```

命名空间表示 1..UINT32_MAX 个任意字节，字符为 0123456789abcdef。序号为无符号 uint64，含前导零共 16 位。拒绝空格、大写、缺失字段或尾随数据。校验接受完整 uint64 范围，不依赖本机地址能力；格式化要求实际存在的快照行。命名空间唯一性由应用维护；相同字节代表同域。LS1 表示历史序号。按 ASCII 字节排序时，同域规范字符串的 sign(strcmp) 等于序号顺序。跨域语义比较返回 DOMAIN_MISMATCH 并清零结果；跨域 strcmp 没有业务含义。不支持语言或忽略大小写的排序规则。4.x 保留含义、精确输出、规范接收、域和序号语义。以后不兼容改动需独立家族或版本。

## LKS4SNP1（冻结）

八位字节，CHAR_BIT==8。无填充序列如下。

```text
8 ASCII bytes LKS4SNP1
u32 BE flags=0
u32 BE namespace_length (>0)
u64 BE row_count
namespace bytes
repeat row_count:
    u64 BE association_length
    association bytes
```

任意二进制有效载荷可含 NUL，允许空关联。空快照为零行和非空命名空间。未知 magic/flags、截断、计数/长度溢出、地址空间溢出或尾随字节均返回 INVALID_ARGUMENT。完整语法和计数校验先于分配。较小 SIZE_MAX 主机拒绝不可寻址输入，不截断或部分发布。OOM 返回空输出。解码后拥有不可变历史数据，无标记或当前性。重新序列化重现原始字节，LS1 从命名空间及序号重建。4.x 冻结宽度、端序、顺序、flags=0、禁止尾随及重建规则。未来不兼容格式须新增版本，解码器不得猜测未知 flags 的含义。格式不内置完整性、校验和或压缩；应用负责传输完整性。

[线格式向量](../tests/fixtures/v4_wire_golden.json)包含七个 LS1、四个二进制向量、严格 V3 导入及 17 个已发布 LK1 冻结向量。快照压力及 OOM 仍为门槛。tests/path_external.c 的 V3 LK1 固定向量保留为私有证据。哈希见 tests/fixtures/v4_fixture_sha256.json。期望字节采用审查常量，独立于编码器。32/64 位任务共用，RC 不得在未经解冻时修改。

## 构建与发行包

验证 Windows MSVC x64/x86、Linux GCC/Clang、macOS AppleClang，C17 实现和 C++17 消费者。Linux Clang ASan/UBSan 与本地 MSVC ASan 用于验证。实现按 C++ 编译及其他 C17 平台无普遍支持承诺。CMake>=3.21 支持 add_subdirectory、离线 FetchContent、安装/导出、find_package(LayerKeySort CONFIG REQUIRED) 和 LayerKeySort::layerkeysort。嵌入时示例、测试和安装默认关闭。普通库、安装版和合并版消费无需 Python。当时的包版本 4.0.0/SameMajorVersion 表示源码重建兼容，当时的头文件声明 4.0.0-preview.5，包版本号不表示已经发布 Stable。ZIP 使用固定时间戳并含五项：公开头、C 实现、V4 示例、LICENSE、README.txt。SHA256SUMS 覆盖 ZIP。须验证复现、散列、成员与篡改拒绝。安装版和合并版 API 符合清单，不引用私有头。

## 已接受的成本和债务

B64 / 最小 32 属于私有策略。未来改容量须保持冻结语义及已发布结构与适用性说明。这不授权 RC 调优。普通局部和索引结构目标为 O(B+log M)。托管搜索另有比较调用；分配、回调、平台和 O(N+A) 导出是独立成本。结构目标不提供全库定理或最坏延迟保证。

| 项目 | 处置 |
| --- | --- |
| 每记录分配、B64 空余与局部写入 | 接受成本 |
| 比较器及回调 | 应用负责的适用边界 |
| 完整 O(N+A) 导出、关联增长、历史保留、外部 LS1 文本 | 接受成本，限制导出密集用途 |
| Group/Batch 排序与合并临时内存 | 接受 |
| 分配器及 RSS 保留 | 接受平台行为；清理后请求量归零仍为门槛 |
| V3 大规模同步重标和实时 Path 增长 | 被 V4 实时模型替代 |
| 全库形式复杂度 | 部分结构断言覆盖，其他延后 |
| 真实用户轨迹 | 后续验证；合成数据不代表现场 |
| 检测失效 Handle | 延后；悬空输入是调用方错误 |
| 增量导出、持久 Handle、更广平台 | 当前不支持，未来主版本可能研究 |

Preview.4 为收敛证据。汇总的远距拖动不是单一条件中位数：100k 实例 29.053 ms，100001 实例 28.182 ms。保存原始行。冻结要求针对性回归，无需复跑优化。本地 MSVC 默认 Release 程序消失为未确认的外部现象；仓库无删除步骤，也未关闭保护。已知可用的带符号 Release、Debug、ASan 与 GitHub Windows 验证构建和打包。

## RC 政策与入场

RC 可修复正确性、sanitizer、可移植性、文档、测试、打包和性能缺陷，同时保持源码、语义及线格式冻结。名称、签名、布局、状态、格式、架构与特性均不可随意改变。必要的不兼容修复必须明确解除冻结。入场需要验证祖先、清单/固定向量/OOM/消费测试、全平台 CI、可复现资产、审查认可证据及无已知正确性阻塞。测试属于工程证据。

## API 最小性处置

| 接口族 | 保留原因 |
| --- | --- |
| 排序与状态 | 无容器的稳定指针排序和确定状态说明 |
| 显式插入、删除、移动 | 应用相对次序及居民实例寿命 |
| Handle 遍历、对象、比较 | 逻辑迭代和同源顺序，不暴露拓扑 |
| 托管封装、读取、定位 | 绑定比较语义，不公开可变核心 |
| 游标 | 解引用缓存 Handle 之前显式检测修改 |
| Group/Batch 读取、构建、合并 | 平面不可变处理与稳定等值分块及来源 |
| 捕获、当前性与快照视图 | 复制历史身份并区分来源当前性 |
| LS1 格式化、校验、比较 | 规范同域外部顺序，区别于实时位置 |
| 序列化、加载和恢复 | 可移植历史持久化及重建实时顺序 |
| 严格 V3 导入 | 单向迁移，不公开并行 V3 实时 API |

公开函数不暴露块、版本、标记、AVL 导航、分配器计数或批量构建内部实现。相似的封装 getter 保持来源所有权边界。
