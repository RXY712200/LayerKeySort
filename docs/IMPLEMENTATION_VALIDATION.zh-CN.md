# Full v4.1.0：可复现的源码、测试、发布与审计证据地图

> 当前临时候选源码 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`。此前 Preview.1 文档修正点为 `3a2b3eabd0153531e1064457519b40deb4af9842`；下文历史 Stable 证据仍指 v4.1.0。

tests/v4_mutation.c::preview12_block_tags 检查 10,000 次集中插入后完整 Handle 顺序和标签单调性；再构造有效但饱和的标签，验证有界回退、双向先后与 AVL 不变量。Windows GCC Release 39/39，独立 OrdWeave 候选测试 12/12。 [候选验证](V4_2_0_PREVIEW1_2.md)。

> Preview.1 增量源码 `22544cf283694dd27cef203caba9cf1838845947`。v4.1.0 发布记录作为历史基线保留。

候选版本元数据完成后，本地 Windows GCC Debug 和 Release 分别通过 39/39 CTest；独立 C/C++ 分发消费者及发行资产验证通过。Windows 轨迹消融的原始结果见候选报告。 见 [候选证据](V4_2_0_PREVIEW1.md)。

> 固定基线：`70d7f0ef5857b54381b863d292a213909d87459f`。依据该 SHA 的根 `CMakeLists.txt`、真实测试入口、CI/构建脚本、API/Golden 文件整理。**本轮建立文档没有自行执行完整本地 CTest 和跨平台矩阵**；“测试覆盖什么”是静态阅读相关测试源码后的范围说明，不能冒充新的独立运行 PASS。
>
> 文档体系：[总体实现导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [64 API 逐项索引](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [私有机制详解](IMPLEMENTATION_INTERNALS.zh-CN.md)。

## 1. 先确认你审查的是哪份源码

~~~sh
git clone https://github.com/RXY712200/LayerKeySort.git
cd LayerKeySort
git checkout 70d7f0ef5857b54381b863d292a213909d87459f
git rev-parse HEAD
git status --short
~~~

Full 4.1.0、Mini、Workbench 和 V3 历史回归是不同对象。不要因为它们在一个仓库，就把某个产品或实验目标的通过等同于 Full 正常生产库的行为。

## 2. 真正的 CMake 目标和选项

根 `CMakeLists.txt` 通过 `include(cmake/ProductionSources.cmake)` 获取 `LKS_SOURCES` 列表，声明 `STATIC` 目标 `layerkeysort` 和别名 `LayerKeySort::layerkeysort`；C17 编译、关闭编译器 C 扩展。

| 选项 | 用途 |
| --- | --- |
| `LKS_BUILD_TESTS` | 测试开关；仓库根构建默认 ON，被其他项目作为子目录使用默认 OFF |
| `LKS_BUILD_EXAMPLES` | 公开例子；根构建默认 ON，嵌入默认 OFF |
| `LKS_BUILD_BENCHMARKS` | 基准目标与附加测试，默认 OFF |
| `LKS_ENABLE_SANITIZERS` | GNU/Clang 可用时启用 ASan/UBSan |
| `LKS_BUILD_CAPACITY_SCREEN` | B64/B128/B256 私有容量屏选目标，非产品 ABI |
| `LKS_INSTALL` | 安装静态库、公开头与 CMake package |

`layerkeysort_diagnostics` 和 `layerkeysort_legacy_bench` 额外使用诊断定义与 V3 旧回归模块。普通 Full 生产目标**不包含** `gap.c/tree.c/group.c/bulk.c` 这四个旧 live 实现。

按所选平台运行：

~~~sh
cmake -S . -B build -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug -N
ctest --test-dir build -C Debug --output-on-failure
~~~

`docs/VALIDATION.md` 的“39 个 CTest”明确有 benchmark/Python 的配置前提。具体次数以本机 `ctest -N` 和实际日志为准。

## 3. 核心源码 → 测试源码 → 真实保障

| 修改或研究的能力 | 测试入口 | 实际测试意图与范围 |
| --- | --- | --- |
| `order.c` 块索引、Handle 生命周期 | [`tests/v4_order.c`](../tests/v4_order.c) | 参考序列比对、边界模式、内部 invariant 校验、两子删除、Shared Block Owner、插入 OOM |
| `order.c` Move/Cursor/revision | [`tests/v4_mutation.c`](../tests/v4_mutation.c) | 随机/敌对移动、长距离操作、no-op、Cursor 失效、跨块 Move OOM |
| `managed_order.c` | [`tests/v4_mutation.c`](../tests/v4_mutation.c) | 重复值、有序插入、邻居定位、比较器调用、回调交互与 OOM |
| `snapshot.c`/`snapshot_restore.c` | [`tests/v4_snapshot.c`](../tests/v4_snapshot.c) | 零行、历史保留、回调生命周期、关联数据、Resolver 失败、容量/分配错误 |
| `snapshot_key.c`/`snapshot_wire.c` | [`tests/v4_snapshot.c`](../tests/v4_snapshot.c)、[`tests/v4_rc_torture.c`](../tests/v4_rc_torture.c) | LS1 规范、wire 向量、非法输入、畸形数据解析 |
| `order_bulk.c` | [`tests/v4_snapshot.c`](../tests/v4_snapshot.c) | 大规模恢复、空恢复、新 Order 重建、失败后资源清理 |
| `immutable_group.c`/`sort.c` | [`tests/v4_immutable_group.c`](../tests/v4_immutable_group.c) | 稳定排序、Group/Batch、相等源顺序、合并、多点 OOM、Group Snapshot |
| SourceMarker 原子引用 | [`tests/v4_marker_threads.c`](../tests/v4_marker_threads.c) | 受限线程场景中的标记引用生命周期；**不是**可变 Order 的并发写安全性证明 |
| 公开 API 冻结 | [`tests/v4_api_compile.c`](../tests/v4_api_compile.c)、[`tests/v4_freeze.c`](../tests/v4_freeze.c) | 64 个函数的类型/编译链接与冻结语义的示例核查 |
| 严格 LK1 导入与旧代码 | [`tests/v4_snapshot.c`](../tests/v4_snapshot.c)、[`tests/parser_torture.c`](../tests/parser_torture.c) | V3 Path/LK1 格式和历史解析边界 |
| V3 旧 Tree/Path 回归 | [`tests/consumer_roundtrip.c`](../tests/consumer_roundtrip.c)、`v2_main.c` 等 | 历史私有声明与旧 API 行为；不是 Full V4 公开 Tree 消费者示例 |
| 长负载与跟踪 | `tests/soak.c`、`benchmarks/v4_trace.c`、`benchmarks/workload_simulator.c` | 反复变更与 workload 族；不代表全部真实应用 |

### 3.1 关键契约对应的具体函数与断言

以下函数的相关断言已在本次文档修订中静态检查；“检查断言”不等于执行测试，更不证明任意输入。生产源码仍是行为依据。

| 契约 | 测试函数及已检查场景 | 覆盖边界 |
| --- | --- | --- |
| 删除只使目标 Handle 失效，其他 Handle 保持身份 | `tests/v4_order.c` 的 `remove_model` 清除模型中的被删 Handle，检查返回 item；`random_campaign` 定期调用 `verify`，逐项核对剩余 Handle 地址、item、正反遍历和 compare | 检查存活者身份；不尝试读取已释放 Handle 来证明失效，这种使用本身非法；不是每一步都执行完整 verify |
| no-op、revision、容量 | `tests/v4_mutation.c::cursor_tests` 在失败注入下执行端点/self/邻接 no-op，断言 revision 不变且无分配，失败插入后 Cursor 可用；UINT64_MAX 时首项 move_front OK、实际 move_back CAPACITY_LIMIT | 测试直接覆盖这些具体分支，不声称穷举全部 API 容量组合 |
| 跨块 Move OOM 不改顺序 | `tests/v4_mutation.c::move_oom` 对满目的块 spare 分配注入失败，比较两块副本和 revision、live bytes/blocks，`sequence_check` 检查 Handle 序列，Cursor 仍可读取原首项 | 直接覆盖 spare 分配失败；同函数还检查非满目标/同块 move 不分配 |
| 同 item 多 occurrence | `tests/v4_order.c::boundaries_and_checker` 多次插入同一个 `&item`，通过保存的不同位置 Handle 删除并检查合并/重分布、不变量 | 覆盖重复业务指针的结构行为；跨多个 ManagedOrder 共享对象后修改字段的应用恢复流程**未由此测试直接验证** |
| Resolver 中途失败 | `tests/v4_snapshot.c::large_and_edges` 配合 `resolve_status_fail`，在第 0、中间、末行返回 NOT_FOUND，断言输出 NULL、Full live bytes 不增加；`resolve_fail` 的首行 OK/NULL 导致 INVALID_ARGUMENT | 首次错误停止由 `snapshot_restore.c` 的立即跳转确认；这些断言没有独立记录并验证失败后的每行调用次数，也不证明业务副作用已回滚 |
| busy getter 与 destroy | `tests/v4_mutation.c::reentry_compare`（由 `managed_interactions` 驱动）检查 Managed 状态函数 REENTRANT、getter NULL/0、共享 Handle getter NULL，并调用 busy destroy；`tests/v4_snapshot.c::associate`（由 `callbacks_lifetime` 驱动）检查来源 getter/嵌套 capture；外层操作随后成功 | 检查列出的调用，不能扩展为所有 getter 的全部组合 |
| Group/Batch busy destroy | `tests/v4_immutable_group.c::compare`、`association`（由 `campaign` 驱动）检查 Group getter NULL/0、嵌套 capture/merge_all REENTRANT，调用 Group/Batch destroy，外层继续使用来源并最终释放 | busy destroy 不破坏来源由后续使用/清理场景支持；未由这里直接验证并发 merge/capture 安全性 |
| 空恢复、回调 buffer | `tests/v4_snapshot.c::capture_restore` 包含 count=0 场景；`associate` 使用文件作用域 scratch 返回字节 | 不能据此声称测试过返回自动局部数组（非法程序）的行为；空恢复 NULL Resolver 的允许性按公开头和 restore 实现确认 |

证据分层：①这些测试代码存在；②本次读过表中相关断言；③本次执行的是文档结构/链接/API 清单与示例编译检查，**没有运行上述 CTest**；④历史 CI 和发布验证仍是历史证据，不转记为本次运行结果。本次检查命令和结果见 PR 修订报告，后续复测应另记 SHA、环境和日志。

### 3.2 测试文件名容易误导的地方

`tests/consumer_roundtrip.c` 顶部明确写有 “Historical V3 regression using private declarations”。请不要因为叫 “consumer” 就宣称它验证了一个使用 V4 稳定 API 的数据库消费者。

测试 `v4_marker_threads.c` 证明的是有专门同步条件的 SourceMarker 生命周期，**不证明**用户能无锁多线程访问/移动一个 Order。

## 4. API、持久格式和 Frozen Fixtures 怎么核对？

**公开头/API**

~~~sh
python tools/verify_v4_api.py
~~~

此脚本对照 [`tests/fixtures/v4_public_api.json`](../tests/fixtures/v4_public_api.json) 检查公开头的符号/类型，并核对引用当前接口的消费者编译测试。64 个函数和 14 个公开类型是冻结基线，新增函数需要按版本治理方案正式讨论，不能只为更新文档调整 Manifest。

**LS1、LKS4SNP1 和 V3 LK1**

`tests/v4_wire_probe.c` 是独立的 wire 测试可执行文件；`tools/verify_v4_wire.py` 与 [`tests/fixtures/v4_wire_golden.json`](../tests/fixtures/v4_wire_golden.json)、V3 LK1 golden 对照；测试框架通过 `layerkeysort_v4_wire_golden` 检验已编译的 probe。

**Golden 常量不能由当前编码器重新生成后又拿来给当前编码器打分**。历史冻结字节变化需要显式版本政策和来源审查。

**模拟 OOM**

诊断版 `lks_alloc_test_fail_on_attempt(k)` 指定可失败分配位置；`v4_order.c`、`v4_mutation.c`、`v4_snapshot.c` 和 `v4_immutable_group.c` 在相关功能中使用该能力。验证点是失败时不泄漏本库资源、不部分提交被承诺原子性的源结构、必要时返回 NULL 输出；应用回调已产生的副作用仍由应用负责。

**Fixture 哈希**

[`tests/fixtures/v4_fixture_sha256.json`](../tests/fixtures/v4_fixture_sha256.json) 用于防止 Golden/Manifest 被无审查地更改。形式、宽度、字节序和兼容规则应以 [V4 冻结契约](V4_FREEZE.md) 为准。

## 5. 两文件发行、安装模式和实际外部调用

Full 有三类正式集成路径：

1. **源码集成**：`add_subdirectory` 或离线 `FetchContent`；
2. **安装后集成**：`cmake --install`、`find_package(LayerKeySort 4 CONFIG REQUIRED)`，链接 `LayerKeySort::layerkeysort`；
3. **官方 amalgamation**：`layerkeysort.h` + `layerkeysort.c`，必须由发行流程生成，用户编译 C17 后由 C/C++ 应用调用。

`tools/amalgamate.py` 严格以 `cmake/ProductionSources.cmake` 为依据收集生产 .c 和所需私有头，不允许人工长期维护另一套会漂移的合并 C 文件。它还有与 MSVC 项目/源清单一致性的校验，避免发行源码漏模块。

`tools/validate_distribution.py` 针对源码消费者、安装消费者和两文件消费者分别编译验证，包含 C/C++ 包装工程；`tools/validate_release_assets.py` 独立生成两份发行物，对照字节一致性、SHA256 和故意篡改/额外文件的拒绝行为。

在适当 Python/CMake/工具链环境：

~~~sh
python tools/validate_distribution.py
python tools/validate_release_assets.py
~~~

这些是**可执行命令建议**，不是本次对它们实际运行的记录。

## 6. 性能与内存报告怎样才能可信？

需要固定来源 SHA、平台、编译器和优化参数；公布 record 数量、操作分布、Comparator/Allocator 代价、块策略、是否含 Snapshot/保留历史等约束。不能只挑一个最漂亮的结果。

- requested live bytes、peak requested bytes 和进程 RSS 不同；
- 同块 Move 和跨块 Move 不同；远距离 Drag 与热点局部修改要分开；
- Snapshot 捕获/序列化是完整 O(N+A)，不要用实时 Move 的局部成本替代其实际耗时；
- 4.1.0 的历史报告保留了空间改善和部分 CPU/延迟回归，不应改写结果成“全面更快”；
- 实际应用 trace 有价值，但没有 trace 不等于所有技术审计必须停止。

## 7. 哪些事实不能靠测试绿色直接推出？

不能推断：任意用户传入已释放 Handle 的操作可以安全检测；所有用户 Comparator 都定义总序；任何调用方的 C++ 回调可以抛异常穿越 C 库；一个 Order 可以并发读写；所有情况都有硬实时最坏延迟；库自动创建磁盘文件、支持 WAL、可崩溃恢复；动态顺序结构能在不载入 RAM 的情况下直接改磁盘页。

如果外部数据库需要这些能力，应由数据库建立自己的存储和事务测试；不能把调用 Full 的 Snapshot 序列化直接宣传为数据库崩溃安全提交。

## 8. 修改源码后必须同步哪里？

| 变更类型 | 需要更新的文档和测试 |
| --- | --- |
| 块/AVL/Handle/Move | [内部详解](IMPLEMENTATION_INTERNALS.zh-CN.md) §2–§8、[总体导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) §2–§4、相关 Order/Mutation 测试 |
| ManagedOrder | 内部详解 §7、API 索引与 Managed Mutation 测试 |
| Snapshot/LS1/wire/Restore | 内部详解 §9–§11、公开 API/兼容/迁移说明、对应 Golden、Snapshot 测试 |
| Group/Batch/sort | 内部详解 §12、API 索引、Group 与稳定排序测试 |
| 旧 LK1 导入 | 内部详解 §13、V3 fixture 和迁移说明，不倒改旧发行版 |
| 构建/安装/两文件发行 | 本篇 §2 / §5、Integration 文档、发行验证脚本 |
| 新公开 API | 先正式审查 4.x 冻结契约和版本，再修改 Manifest、API 文档、消费者测试 |
| Full 对外版本升级 | 在 [尚未合并的 PR #24](https://github.com/RXY712200/LayerKeySort/pull/24) 所拟定的交接台账中记录精确前后 SHA、API/内部差异、消费者是否需改适配层 |

**如果 PR #24 尚未合并，则它的交接文件还不存在于 main**；不能在发行文档里假装已生效。

## 9. 本轮文档化的验收范围与不足

- **已完成：** 固定 SHA 源码核对、全部 16 个生产 C 翻阅、主要私有头、64 个公开 API 枚举、关键内部函数/分支说明、测试入口/构建/分发工具对照、源码导航。
- **未完成：** 在本轮独立运行完整 39 项 CTest、所有 CI 平台、Sanitizer、全部历史实验数据以及对任意输入的形式化正确性证明。
- **未授权：** 修改 Full 生产代码、修改 stable API/文件格式、直接合并 PR、打 tag 或发布新版。

这份文件是一张**可复现审查地图**。将来要把“静态源码确认”升级为“实际测试 PASS”，必须记录实际运行的测试 SHA、命令、平台与日志，不得只复制上次 Codex 或 ChatGPT 的结论。
