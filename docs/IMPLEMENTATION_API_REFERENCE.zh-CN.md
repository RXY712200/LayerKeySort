# Full 4.2.0-rc.1：完整公开 API 索引与关键契约指南

[English](IMPLEMENTATION_API_REFERENCE.md) · [精确声明索引](design/V4_2_0_RC1_API_INDEX.zh-CN.md) · [源码重建](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.zh-CN.md)

> 本版适用 Full 4.2.0-rc.1 验证树 `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`，包含两个 Preview.3 回归。运行行为源自 `fe00536e7bd7f4e5128432ea4a29e54158074903`；已发布 Stable 仍为 4.1.0。完整谱系见[源码审计](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.zh-CN.md)。

已发布 Stable 对照为 `70d7f0ef5857b54381b863d292a213909d87459f`。签名、参数顺序及准确返回值以对应 SHA 的 [公开头](../include/layerkeysort.h) 为准。本手册覆盖根目录 Full，4.x 的接口保持冻结。serialize/deserialize 的字节、状态、所有权与借用规则保持不变，偏移一字节的缓冲区由回归测试覆盖。参见[实现审计与证据](V4_2_0_PREVIEW3.zh-CN.md)。

## A. 如何读取本手册

以下 64 项与公开头逐项对照；实际原型、实现位置及说明行见[冻结 API 索引](design/V4_2_0_RC1_API_INDEX.zh-CN.md)。错误优先级以各入口校验顺序为准。例如 compare 先将输出归零，再查空参数和 owner，之后才检测回调重入。Handle 已失效时不得再传入。

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
| `lks_order_compare` | 比较同一 Order 内两个存活 Handle 的先后；同块比较局部下标，跨块优先比较内部标签，任一块无标签时沿 AVL 父链判断；跨块最坏结构复杂度仍为 O(log(1+M))；输出负/零/正。 | `src/order.c` |
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
| `lks_managed_order_remove` | 按独立 Handle 删除，不要求再次比较数据；比较字段变更必须处理共享对象的全部相关 occurrence；删除后重插不是原子事务（见 E）。 | `src/managed_order.c` |
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
| `lks_snapshot_deserialize` | 用固定宽度大端读取先完整验证 magic/flags/行长度/溢出/尾部，再分配、复制完整 Snapshot 字节；输出和错误契约不变。 | `src/snapshot_wire.c` |
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

### B8. 按家族核对输入、失败和所有权

本表是头文件与实现的关键摘要；错误检查有先后顺序，不承诺多个非法条件同时出现时返回某个统一状态。所有借用对象必须仍然存活，NULL 处理不能检测悬空指针。

| 家族 | 必填、NULL 与空输入 | 输出、主要错误 | 重入、分配和生命周期 |
| --- | --- | --- | --- |
| Status / sort | status 接受未知枚举；sort 的 compare 必填，count=0 允许 items=NULL | 未知 status 返回静态文字；sort 非法参数 INVALID_ARGUMENT、数组分配失败/过大 OUT_OF_MEMORY，原数组不变 | count<2 不分配；否则 copied/scratch 两数组；借用 item/context，调用方保证比较器和数组不被回调破坏；无容器 busy guard |
| Explicit 创建/读取 | create 无参数；destroy(NULL) 无动作；size(NULL)=0，指针 getter(NULL)=NULL | create 分配失败 NULL；空 first/last、越端 next/previous 返回 NULL；compare 必须有同源 live Handles 和输出，失败输出 0 | create 分配；读取/compare 不分配；busy 时 getter 为 NULL/0、compare 为 REENTRANT、destroy 忽略；Order 拥有结构、借用 item |
| Explicit insert/remove/move | Order/item/out_handle 为插入必填；相对 anchor 必须同源 live；remove 的 out_item 可省略；空 Order 可插入，不能删除/移动不存在的 occurrence | 插入失败 out_handle=NULL；删除失败 out_item=NULL；INVALID_ARGUMENT、REENTRANT、CAPACITY_LIMIT；insert/需要 spare 的 move 可 OUT_OF_MEMORY | 插入先分配 Handle/可选块；remove 不分配；实际修改增长 revision；no-op move 即使 revision 上限也可 OK；存活 Handle 不变，删除只使目标失效 |
| Managed | create 要求 callable comparator；复制描述符、借用 context；插入 item/out_handle 必填；locate query 与三个互异输出必填，可查询非 resident | create 非法/OOM 为 NULL；Status API 可 INVALID_ARGUMENT/REENTRANT；insert 继承核心容量/OOM；locate 输出先清 NULL，未匹配仍 OK；compare 输出先置 0 | bound/locate/remove/compare 不分配，remove 不调用比较器；busy getter 为 NULL/0、destroy 忽略；不得改变 resident 比较语义，无公开 move/core |
| Cursor | source/out_cursor 必填，reverse 只能 0/1；空来源可创建 | 创建失败输出 NULL：INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY；next 输出先清 NULL，结束 OK/NULL，真实修改 INVALIDATED/NULL | 创建分配，next/destroy 不分配；destroy(NULL) 安全；next 在源回调期间 REENTRANT；Cursor 必须先于源销毁 |
| Snapshot capture/currentness | source/options/out 必填；namespace 为有效的 1..UINT32_MAX 字节；association 可 NULL，空来源可捕获 | capture 输出先清 NULL：INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY/CAPACITY_LIMIT，或回调错误透传；currentness 在 NULL/busy/加载快照/不同来源时假 | 捕获分配 rows/域/关联数据和可选 marker；容量/算术边界在发布前检查；失败不改来源 revision；Snapshot 拥有副本，回调 span 见 E |
| Snapshot views / LS1 / wire | view 的两个输出均必填；association/key_format 要求合法行；format/serialize 的目标必填；NULL count/length/serialized_size 为 0 | view 错误输出 NULL/0；key_compare 输出先为 0；INVALID_ARGUMENT、BUFFER_TOO_SMALL；跨 LS1 域 DOMAIN_MISMATCH；deserialize 输出 NULL，可 INVALID_ARGUMENT/OUT_OF_MEMORY | 只读 view/编码不分配；不足 buffer 不写；deserialize 完整验证后分配；views 借用到 Snapshot 销毁，destroy(NULL) 安全 |
| Restore / LK1 import | restore 的 Snapshot/out 必填，import 的 out 必填；非空 restore 要求 Resolver，空允许 NULL；空 import 允许 entries=NULL，非空 key/item 必须有效非 NULL | 输出先清 NULL；INVALID_ARGUMENT/OUT_OF_MEMORY，回调错误透传；LK1 重复坐标 ALREADY_EXISTS；超出目标平台表示范围拒绝、不截断 | restore 逐行短路后 bulk build，import 先解析排序；空结果也可能分配失败；新 Order 归调用者，items 始终借用；回调副作用不回滚 |
| Group / Batch | build/merge 要 callable comparator/out；空 build 允许 items=NULL；非空条目非 NULL；Batch group_size>0；merge 来源必须排序兼容 | 输出先清 NULL；INVALID_ARGUMENT/REENTRANT/OUT_OF_MEMORY；size 类 NULL/busy 为 0，item/group_at 越界或 busy 为 NULL | 构建/归并分配；build/merge 的计数/数组算术检查可 INVALID_ARGUMENT，实际分配失败 OUT_OF_MEMORY，勿统一写成 CAPACITY_LIMIT；busy destroy 忽略；Batch 拥有 chunk，借用 chunk 不可单独销毁；merge_all 仅多 chunk 分配 scratch |

### B9. ManagedOrder 的共享 Handle 访问器

以下函数片段可放在包含 `layerkeysort.h` 的 C 文件中。调用期间应用保证来源和 item 存活且无冲突修改；`visit` 不修改或销毁该来源。

```c
static void visit_managed(const LksManagedOrder *order, void (*visit)(void *))
{
    const LksOrderHandle *h;
    if (!order || !visit) return;
    for (h = lks_managed_order_first(order); h; h = lks_order_next(h))
        visit(lks_order_item(h));
    for (h = lks_managed_order_last(order); h; h = lks_order_previous(h))
        visit(lks_order_item(h)); /* 第二轮为反向遍历，重复访问是本例有意行为。 */
}
```

这是受支持的共享 Handle 访问方式，无需也不能取得私有 core。

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

同 revision 的两次捕获也各自拥有历史字节。对于借用的快照行及命名空间，生命周期随快照；对于恢复，Resolver 按行顺序返回的 item 仍由调用者管理，新 Order 为每个 occurrence 建立新 Handle。

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

### D1. Handle 身份不冻结相邻关系

存活 Handle 继续有效，不代表之前观察到的 next/previous 邻居或 compare 结果固定不变。插入和移动可以改变这些关系。普通遍历没有自动 revision 检测；需要检测修改时使用 Cursor，真实修改后返回 INVALIDATED，no-op/失败不使它失效。

删除式遍历先保存下一个 live Handle，再删除当前 occurrence；保存不是延长生命周期，若保存的 Handle 自身随后被删除也立即失效：

```c
static LksStatus remove_all(LksOrder *order)
{
    const LksOrderHandle *h, *next;
    if (!order) return LKS_STATUS_INVALID_ARGUMENT;
    for (h = lks_order_first(order); h; h = next) {
        LksStatus status;
        next = lks_order_next(h);
        status = lks_order_remove(order, h, NULL);
        if (status != LKS_STATUS_OK) return status;
        /* h 已失效；next 只在其自身未被删除且来源存活时有效。 */
    }
    return LKS_STATUS_OK;
}
```

## E. 错误处理中的几个“不直观”边界

序列化与反序列化接受满足长度条件的逐字节缓冲区，包括偏移一字节的地址。反序列化先完整检查长度、标志和尾随数据；失败时没有部分 Snapshot 输出。回调已产生的应用副作用由应用处理。

修改比较器可见字段前，应用必须处理每一个相关 ManagedOrder 中引用该对象的**全部 occurrence**，以及依赖该对象排序的 Group 等其他结构。仅删除一个 occurrence 不够；Full 不会自动追踪跨容器共享对象。Group 不可原地删除记录，应由应用安排重建或替换。

“remove → 修改 → insert”是多个独立操作，**不是 Full 事务**：成功 remove 立即使原 Handle 的全部副本失效；后续 insert 可能因 OOM 等错误失败，Full 不会撤销先前删除，也不保证恢复原 Handle。应用应保留业务对象及足够的顺序/身份状态，在失败后重建相关容器或按应用策略恢复；恢复插入本身也可能失败，必须继续处理错误。

**Association 返回 span 的生命周期**：数据必须在回调返回之后仍然有效，直到 Full 完成紧接着的复制。不能返回自动局部数组的地址，因为其生命周期在回调返回时结束；可使用仍存活的业务对象字段或由调用方管理的缓冲区。非零长度要求非 NULL 数据。

1. `before/after` 的 anchor 必须是**当前活着且属于同一 Order** 的 Handle。代码可拒绝另一个活 Order 的 Handle，但不会安全检测已经 `free` 的悬空 Handle。
2. 新插入的 `out_handle` 是调用者必须提供的有效输出地址；失败后为 NULL。删除的 `out_item` 可省略，存在时先清 NULL。
3. Managed Locate 的 `out_predecessor`、`out_equal`、`out_successor` **要求三个不同的输出指针地址**。即使查询结果是 NULL，也不允许输出槽位别名。
4. Snapshot 的 `namespace_size` 必须非零，Namespace 由应用选择；同一 Namespace 不自动保证两个不同时刻的历史 Key 唯一。
5. `lks_snapshot_key_length` 不含 `\0`，所以 `key_format` 至少准备 `length+1` 字节。二进制 `serialized_size` 是精确字节数，不包含文本终止符。
6. `lks_snapshot_restore_order` 按行序调用 Resolver，首次非 OK 或 OK 但 item 为 NULL 时立即停止，后续行不调用；仅全部必需行成功后才进入未发布的 bulk Order 构建。空 Snapshot 不调用 Resolver，允许 NULL Resolver，仍需分配一个新的空 Order，因此可能 OOM。失败清理 Full 自有临时分配、输出为 NULL，但不回滚应用回调副作用、不释放已经解析出的调用方 item。
7. Group 与 Batch 在比较器语义保持一致时才可安全归并；改变参与排序的字段会破坏它们“已排序”的前置条件。
8. 满目标块的跨块 move 必须预先分配 spare；OOM 返回前**不发生逻辑顺序改变**。同块 move 不需要分配。
9. 由于 V4 已冻结，任何拟新增 `rank`、`at` 或 `persistent ID` 都不是本文现成 API；新接口需要独立方案和版本契约审查。

### E1. 并发读取与 LS1 排序

完成后的不可变 Snapshot 可以并发读取，前提是应用从外部保护其整个读取期间的生命周期，不与销毁冲突。Group 的纯读取可在无并发销毁、merge/capture 等状态访问且业务对象读取安全时进行；merge/capture 会读写内部 busy/marker 状态，不能仅因 Group 逻辑不可变就与其他访问任意并发。Order/ManagedOrder 的访问若涉及修改或销毁，必须由调用者同步。回调重入 guard 与 SourceMarker 原子引用计数都不是容器锁。

同一 namespace 的规范 LS1 键使用固定 16 位小写十六进制 ordinal，故其**字节级 ASCII 词典序与 ordinal 顺序一致**。外部数据库依赖该性质时须采用保持字节顺序的 collation，不能假定语言或忽略大小写的排序规则适用。不同 namespace 的 `lks_snapshot_key_compare` 返回 `DOMAIN_MISMATCH`；对这些字符串直接排序不建立有意义的业务记录先后。LS1 是历史快照坐标，不是跨快照版本的持久记录 ID；这与 V3 LK1 的 Path 顺序编码是不同契约。

## F. 低风险入门流程与进一步阅读

C/C++ 仅使用公开头与目标库。快照借用的字节视图在快照销毁前有效；读写同一 Order 与销毁操作由调用者同步。解码得到的 Snapshot 可再恢复，但不带原 live marker。

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
