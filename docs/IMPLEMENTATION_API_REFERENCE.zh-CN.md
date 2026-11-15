# Full v4.1.0：全部 64 个公开 API 的源码映射与使用契约

> 审查基线：`main` `70d7f0ef5857b54381b863d292a213909d87459f`；接口最高事实来源为 [`include/layerkeysort.h`](../include/layerkeysort.h) 与其生产实现。本文与 [完整执行导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md)、[内部算法详解](IMPLEMENTATION_INTERNALS.zh-CN.md)、[测试与证据地图](IMPLEMENTATION_VALIDATION.zh-CN.md) 配套。**本文不是可编译的替代头文件；签名、参数顺序、准确返回值以该固定 SHA 的公开头为准**。4.x 公开契约冻结，不在本文发明额外 API。

## A. 如何读取本手册

- **一次 occurrence** 指向一个非 NULL、由应用拥有的 `void *item`；同一 `item` 指针可被插入多次，但每次有独立 `LksOrderHandle`。
- **生命周期**：Order/ManagedOrder/Group/Batch/Snapshot/Cursor 结构由 Full 分配，创建者负责销毁。业务对象、比较器上下文、关联字节来源与外部文件始终由应用负责。
- **结果返回**：`LksStatus` 的枚举值被冻结；非 Status 的函数在无效输入时可能返回 `NULL`、0 或无动作，不要把这种行为误当作能够检查悬空指针。
- **来源指针**：Handle 是借用视图，在其 occurrence 被删或 Order 销毁时失效；Cursor 必须先于 Order 销毁；Snapshot 视图在 Snapshot 销毁时失效。
- **回调**：同源回调期间不能重新访问同一可变容器；比较器必须满足一致的序关系；回调必须正常返回，不能跨 C 调用 `longjmp` 或从 C++ 抛异常。
- **并发**：使用者对可变容器的全部冲突访问提供同步。SourceMarker 的原子引用计数不是容器锁。
- **精确定位**：下文每个函数标出主要生产文件。完整调用路径见内部详解；测试证据见验证地图。对未特别列出的通用非法输入处理和输出清零行为，仍以头文件的具体条款为准。

## B. 64 个函数完整索引（按使用场景）

下列每行代表**一个已在公开头声明的实际函数**。标注的实现路径均在当前生产清单中。

### B1. 状态与一次性排序（2）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_status_string` | 将状态值映射为 Full 拥有的静态文字，未知整数返回 `Unknown status`，不分配、不可释放返回的字符串。 | `src/lks_base.c` |
| `lks_sort` | 在原调用者指针数组上进行稳定排序：先分配复制与临时 Merge scratch，排序成功后才覆盖原数组；分配失败原数组不变；`count<2` 不分配。 | `src/sort.c` |

### B2. Explicit Order：构造、插入、删除、查询、移动（19）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_order_create` | 新建空 Order；分配失败返回 NULL；不创建业务对象。 | `src/order.c` |
| `lks_order_destroy` | 释放全部块与 Handle 和标记引用，不释放任何业务 item；`NULL` 可传入。 | `src/order.c` |
| `lks_order_size` | 返回 occurrence 数；NULL/回调忙碌时返回 0。 | `src/order.c` |
| `lks_order_insert_front` | 在最前创建新的 occurrence；返回独立 Handle；新块或满块拆分预分配后再提交。 | `src/order.c` |
| `lks_order_insert_back` | 在最后创建新的 occurrence；其余性质同上。 | `src/order.c` |
| `lks_order_insert_before` | 在同一 Order 的存活 anchor 之前插入新 occurrence，不能传其他 Order 的有效 Handle。 | `src/order.c` |
| `lks_order_insert_after` | 在同一 Order 的存活 anchor 之后插入新 occurrence。 | `src/order.c` |
| `lks_order_remove` | 删除**指定 occurrence**；可取回借用 item；其他 occurrence/Handle 继续有效；已删除 Handle 失效。 | `src/order.c` |
| `lks_order_first` | 获取首个 Handle 或空集 NULL；结果为借用值。 | `src/order.c` |
| `lks_order_last` | 获取末尾 Handle 或空集 NULL。 | `src/order.c` |
| `lks_order_next` | 同块访问下一个下标，跨块跳至下一块首项，尾部返回 NULL。 | `src/order.c` |
| `lks_order_previous` | 同块向前一项，跨块跳至上一块末项，首部返回 NULL。 | `src/order.c` |
| `lks_order_item` | 从存活 Handle 取出调用者提供的原始 item 指针；不复制、不转移所有权。 | `src/order.c` |
| `lks_order_compare` | 比较同一 Order 内两个存活 Handle 的先后；同块比较局部下标，跨块比较隐式 AVL 块 rank；输出负/零/正。 | `src/order.c` |
| `lks_order_move_front` | 将现有 occurrence 移动到开头；Handle 本身保持有效；本来就在首部则是 noop。 | `src/order.c` |
| `lks_order_move_back` | 将现有 occurrence 移动到尾部；本来就在尾部则 noop。 | `src/order.c` |
| `lks_order_move_before` | 将存活 occurrence 移到同一 Order anchor 之前；自己/已邻接位置 noop，不递增 revision。 | `src/order.c` |
| `lks_order_move_after` | 将存活 occurrence 移到同一 Order anchor 之后；自己/已邻接位置 noop。 | `src/order.c` |
| `lks_order_snapshot_capture` | 对当前 Explicit Order 生成独立、不可变历史 Snapshot；不会写磁盘。 | `src/snapshot.c` |

> **注意**：上表包括 `lks_order_snapshot_capture`，它既是 Order 家族的函数，也是快照入口；在文档中只计数一次。基础 Explicit API（不含 Snapshot）为 18 个。

### B3. Cursor、来源当前性与旧版导入（6）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_order_cursor_create` | 按指定方向（0 正向，1 反向）创建捕获 Order revision 的 Cursor；初始当前位置为首/尾。 | `src/order.c` |
| `lks_order_cursor_next` | 每次返回下一个 Handle；结束返回 `OK + NULL`；发生真实变动时先返回 `INVALIDATED + NULL`，不碰过期缓存。 | `src/order.c` |
| `lks_order_cursor_destroy` | 释放 Cursor 自身，不释放或修改它借用的 Order。 | `src/order.c` |
| `lks_order_snapshot_is_current` | 来源对象的标记相同、revision 相同才为真；历史加载 Snapshot 没有活来源标记，返回假。 | `src/snapshot.c` |
| `lks_order_import_v3_lk1` | 对严格旧格式 LK1 坐标解析、按 Path 顺序排序并拒绝坐标重复，最后批量建立无 Path 的新 V4 Order。 | `src/snapshot_restore.c` |
| `lks_managed_order_cursor_create` | ManagedOrder 的 Cursor，通过其私有 Explicit 核心创建；应用不能直接取得可变 core。 | `src/managed_order.c` |

### B4. ManagedOrder：比较器驱动（10）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_managed_order_create` | 验证比较器，复制描述符，借用 callback/context，并创建私有 `LksOrder`。 | `src/managed_order.c` |
| `lks_managed_order_destroy` | 销毁 facade 和核心顺序结构，不销毁业务 item、context。 | `src/managed_order.c` |
| `lks_managed_order_size` | 查询 occurrence 总数，忙碌/NULL 返回 0。 | `src/managed_order.c` |
| `lks_managed_order_insert` | 搜索 upper bound 并用 `lks_order_insert_before/back` 插入；比较器相等的新项排在旧项后。 | `src/managed_order.c` |
| `lks_managed_order_locate` | 分别查 lower/upper bound，返回严格左邻、首个等值、首个严格大值；三个输出地址须不同。 | `src/managed_order.c` |
| `lks_managed_order_remove` | 按独立 Handle 删除，不要求再次比较数据；修改比较器相关字段需要先删再改再插。 | `src/managed_order.c` |
| `lks_managed_order_first` | 取得比较器顺序中的首个 occurrence。 | `src/managed_order.c` |
| `lks_managed_order_last` | 取得比较器顺序中的末项。 | `src/managed_order.c` |
| `lks_managed_order_compare` | 复用 core 的 Handle 顺序比较；不再根据当前字段即时重新排序。 | `src/managed_order.c` |
| `lks_managed_order_snapshot_capture` | 经过 ManagedOrder reentry guard 后对其 core 捕获 Snapshot。 | `src/snapshot.c` |

### B5. Managed 当前性（1）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_managed_order_snapshot_is_current` | 判断该 ManagedOrder 的 core 与 Snapshot 的来源标记和修订代次是否一致。 | `src/snapshot.c` |

### B6. Snapshot、LS1、wire、恢复（12）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_snapshot_destroy` | 释放 copied bytes、Snapshot、来源标记引用；允许 NULL。 | `src/snapshot.c` |
| `lks_snapshot_count` | 返回 Snapshot 行数；NULL 为 0。 | `src/snapshot.c` |
| `lks_snapshot_namespace` | 输出 borrowed Namespace 字节及长度；借用到 Snapshot 销毁。 | `src/snapshot.c` |
| `lks_snapshot_association` | 通过行序号输出 borrowed 关联数据和长度；空关联返回 `NULL,0`。 | `src/snapshot.c` |
| `lks_snapshot_key_length` | 给出 LS1 文本字符数，不含末尾 NUL；供调用者预备缓冲区。 | `src/snapshot_key.c` |
| `lks_snapshot_key_format` | 格式化指定 Snapshot 行的 `LS1.<ns-hex>.<ordinal-hex>`；空间不足不写入。 | `src/snapshot_key.c` |
| `lks_snapshot_key_validate` | 严格检查 LS1 的规范字节语法，不要求来源 Snapshot 存在。 | `src/snapshot_key.c` |
| `lks_snapshot_key_compare` | 验证双方 LS1，同 Namespace 比较 ordinal；不同 Namespace 返回 `DOMAIN_MISMATCH`。 | `src/snapshot_key.c` |
| `lks_snapshot_serialized_size` | 计算此 Snapshot 的 **LKS4SNP1** 二进制字节总数；NULL 为 0。 | `src/snapshot_wire.c` |
| `lks_snapshot_serialize` | 通过调用者提供的二进制缓冲区生成精确 wire；**不创建文件**，空间不足目标不变。 | `src/snapshot_wire.c` |
| `lks_snapshot_deserialize` | 先完整验证 magic/flags/行长度/溢出/尾部，再分配、复制完整 Snapshot 字节。 | `src/snapshot_wire.c` |
| `lks_snapshot_restore_order` | 逐行 Resolver 取得非 NULL item，批量建一个新 Order；新的 Handle 不继承老进程地址。 | `src/snapshot_restore.c` |

### B7. Group/Batch：不可变数组与分组稳定排序（14）

| API | 做什么、核心语义 | 实现 |
| --- | --- | --- |
| `lks_group_build` | 复制并稳定排序输入指针，返回独立 Group；可以为空，但必须有有效比较器。 | `src/immutable_group.c` |
| `lks_group_destroy` | 释放 Group 指针数组、标记及结构，不释放 item。 | `src/immutable_group.c` |
| `lks_group_size` | 读取 Group 数量；NULL 或忙碌返回 0。 | `src/immutable_group.c` |
| `lks_group_item_at` | 按 Group 的排序位置取原始借用 item 指针；越界 NULL。 | `src/immutable_group.c` |
| `lks_group_merge` | 归并两个有序 Group，生成新 Group；比较相等时优先原 base，允许输入相同组。 | `src/immutable_group.c` |
| `lks_group_batch_build` | 把原输入切为多个 group_size 的 chunk，每块独立排序；Batch 持有这些组。 | `src/immutable_group.c` |
| `lks_group_batch_destroy` | 销毁 Batch 及持有的所有 chunk；不可单独销毁借用的 chunk view。 | `src/immutable_group.c` |
| `lks_group_batch_size` | 返回 Batch 全部元素数。 | `src/immutable_group.c` |
| `lks_group_batch_group_count` | 返回 Batch 内实际 chunk 数。 | `src/immutable_group.c` |
| `lks_group_batch_group_size` | 返回构建时指定的目标 chunk 大小；最后一块可能更少。 | `src/immutable_group.c` |
| `lks_group_batch_group_at` | 借用 Batch 持有的 Group 视图；随 Batch 销毁失效。 | `src/immutable_group.c` |
| `lks_group_batch_merge_all` | 将已排序 chunk 按宽度翻倍的稳定归并合成一个新 Group；早期 chunk 相等项优先。 | `src/immutable_group.c` |
| `lks_group_snapshot_capture` | 将不可变 Group 的当前顺序捕获到通用 Snapshot 格式；使用与 Order 相同的导出逻辑。 | `src/immutable_group.c` |
| `lks_group_snapshot_is_current` | 同一 Group 标记且 Snapshot revision 为 0 才为真；不代表 Group 的业务字段仍未变化。 | `src/immutable_group.c` |

## C. 类型、枚举、回调（公开头中的“非函数接口”）

**14 个公开类型**以 [`tests/fixtures/v4_public_api.json`](../tests/fixtures/v4_public_api.json) 的冻结清单为准。最重要的对象/回调包括：

| 类型 | 用途 | 持有规则 |
| --- | --- | --- |
| `LksStatus` | 显式整数状态枚举 | 值冻结，不可擅自重排 |
| `LksComparator` / `LksCompareFn` | 比较器与 context | Managed 复制描述符、借用 context；sort/group 只在当前调用借用 |
| `LksOrder` / `LksOrderHandle` | 实时容器和单次插入的借用标识 | Order 释放结构；Handle 删除即失效 |
| `LksManagedOrder` | 比较器维护的 facade | 持有私有 Order 核心 |
| `LksOrderCursor` | 版本检查式遍历 | 借用其来源 |
| `LksSnapshotOptions` / `LksSnapshotAssociationFn` | 命名空间、回调及被复制的关联字节 | 回调数据由 Full 当次复制 |
| `LksSnapshot` | 独立不可变历史顺序 | Snapshot 自有字节，用户销毁 |
| `LksSnapshotResolveFn` | 从 Association 或 ordinal 恢复应用 item | 恢复结果仍属于应用 |
| `LksV3Lk1ImportEntry` | 一次性旧 LK1 迁移入口 | key/item 都是调用方借用 |
| `LksGroup` / `LksGroupBatch` | 不可变有序组及其 Batch | Group 只持有指针数组，Batch 拥有其中 Group |

**状态数值（不要自行按连续数字重排）**：

| 数值 | 名称 | 常见触发 |
| ---: | --- | --- |
| 0 | `OK` | 成功 |
| 1 | `INVALID_ARGUMENT` | 空参数/输出地址重叠/非法标识等 |
| 2 | `OUT_OF_MEMORY` | 分配失败 |
| 3 | `BUFFER_TOO_SMALL` | 格式化/序列化缓冲区不足 |
| 5 | `NOT_FOUND` | 某些调用方回调返回未找到 |
| 7 | `INTERNAL_ERROR` | 内部防御性失败 |
| 8 | `ALREADY_EXISTS` | 旧 LK1 相同坐标重复 |
| 9 | `CAPACITY_LIMIT` | 计数、revision 或引用数边界 |
| 10 | `INVALIDATED` | Cursor 看到来源真实变更 |
| 11 | `REENTRANT` | 同源回调忙碌期间的非法重新访问 |
| 12 | `DOMAIN_MISMATCH` | LS1 两个 namespace 不一致 |

保留空洞 4、6 是有意行为；不要为对齐编号修改 ABI 或持久格式。

## D. 一次实际使用中的**所有权/失效**判断矩阵

| 事件 | Order | Handle | Cursor | Snapshot | 外部 item |
| --- | --- | --- | --- | --- | --- |
| 普通插入 | 不变为同一对象，版本增长 | 旧 Handle 地址不变，新 Handle 独立创建 | 已有 Cursor 失效 | 旧历史仍可读，`is_current` 变假 | 不由 Full 释放 |
| 删除一个 occurrence | 容器仍在，count−1 | 被删 Handle 立即失效，其余有效 | 已有 Cursor 失效 | 旧历史仍可读 | 不由 Full 释放 |
| 实际 move | 容器不变，revision +1 | 全部存活 Handle 地址不变 | 已有 Cursor 失效 | 旧历史仍可读 | 不由 Full 释放 |
| noop move | 逻辑和 revision 不变 | 全部有效 | 仍有效 | 已有快照仍可 current | 不变 |
| 普通失败（如 OOM） | 公开强失败原子性承诺覆盖的原状态不变 | 原有存活 Handle 有效 | 不被失败操作无故失效 | 不受影响 | 回调副作用另论 |
| 销毁 Order | Order 失效 | 全部 Handle 失效 | 必须已销毁，否则使用非法 | Snapshot 数据继续有效，来源 current 变假 | 仍归应用 |
| 反序列化并恢复 | 新建不同 Order | **全部是新 Handle** | 需要新建 Cursor | 加载 Snapshot 无 live provenance | 由 Resolver 返回有效对象 |

**不能这么做**：销毁业务 item 但 Handle 仍指向它；用旧 Handle 代表新恢复的记录；复制 raw Handle pointer 到磁盘；把 LS1 键用作跨 Snapshot 的业务主键；未经同步跨线程修改和遍历；在回调中修改正在捕获的同源容器。

## E. 错误处理中的几个“不直观”边界

1. `before/after` 的 anchor 必须是**当前活着且属于同一 Order** 的 Handle。代码可拒绝另一个活 Order 的 Handle，但不会安全检测已经 `free` 的悬空 Handle。
2. 新插入的 `out_handle` 是调用者必须提供的有效输出地址；失败后为 NULL。删除的 `out_item` 可省略，存在时先清 NULL。
3. Managed Locate 的 `out_predecessor`、`out_equal`、`out_successor` **要求三个不同的输出指针地址**。即使查询结果是 NULL，也不允许输出槽位别名。
4. Snapshot 的 `namespace_size` 必须非零，Namespace 由应用选择；同一 Namespace 不自动保证两个不同时刻的历史 Key 唯一。
5. `lks_snapshot_key_length` 不含 `\0`，所以 `key_format` 至少准备 `length+1` 字节。二进制 `serialized_size` 是精确字节数，不包含文本终止符。
6. `lks_snapshot_restore_order` 先完成全部 Resolver 调用，后创建新的 Order；Resolver 失败导致输出 NULL，但它产生的外部副作用、对象分配须由调用方处理。
7. Group 与 Batch 在比较器语义保持一致时才可安全归并；改变参与排序的字段会破坏它们“已排序”的前置条件。
8. 满目标块的跨块 move 必须预先分配 spare；OOM 返回前**不发生逻辑顺序改变**。同块 move 不需要分配。
9. 由于 V4 已冻结，任何拟新增 `rank`、`at` 或 `persistent ID` 都不是本文现成 API；新接口需要独立方案和版本契约审查。

## F. 低风险入门流程与进一步阅读

新读者可以依次检查：

1. [`examples/live_order.c`](../examples/live_order.c) 或 [`examples/basic.c`](../examples/basic.c) 对应公开 API。
2. [`src/lks_order_internal.h`](../src/lks_order_internal.h)：先认识 Handle、Block、Order、Cursor。
3. [`src/order.c`](../src/order.c)：从 `v4_insert`、`v4_place`、`v4_move`、`v4_repair` 到 `v4_balance`。
4. [`src/managed_order.c`](../src/managed_order.c)：看看如何借助同一个 Order 完成比较器定位。
5. [`src/snapshot.c`](../src/snapshot.c)、[`src/snapshot_wire.c`](../src/snapshot_wire.c)、[`src/snapshot_restore.c`](../src/snapshot_restore.c)：如何把对象身份交给用户、将历史顺序独立保存及恢复。
6. [`src/immutable_group.c`](../src/immutable_group.c)、[`src/sort.c`](../src/sort.c)。
7. [`docs/IMPLEMENTATION_INTERNALS.zh-CN.md`](IMPLEMENTATION_INTERNALS.zh-CN.md)：每个关键私有函数、执行变形与边界。
8. [`docs/IMPLEMENTATION_VALIDATION.zh-CN.md`](IMPLEMENTATION_VALIDATION.zh-CN.md)：对应测试/证据与可复现命令。

本手册解释了所有公开函数的**意图、真实实现入口及关键限制**，但不替换源码逐行检视，也不承诺 100% 的调用方行为均可被一份文字形式化证明。
