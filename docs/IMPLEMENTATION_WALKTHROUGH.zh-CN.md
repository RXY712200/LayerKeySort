# LayerKeySort Full：从源码理解完整实现（v4.1.0）

> **定位：面向新维护者、独立审计者和直接集成 Full 的 C/C++ 开发者的源码执行导读。**
>
> **核对基线**：2026-11-15，仓库 `RXY712200/LayerKeySort`、`main`、完整 SHA `70d7f0ef5857b54381b863d292a213909d87459f`，对应已发布 Full **v4.1.0 Stable**。审读范围为根目录 Full，**不是** `mini/` 产品。
>
> **证据及限制**：本次重新读取了 `cmake/ProductionSources.cmake` 指定的 **16 个生产 C 文件**，公开头 `include/layerkeysort.h`，核心私有结构/内部声明，根 `CMakeLists.txt`，既有架构与兼容文档，检索了主要 V4 测试的调用与测试入口，确认了旧版本回归源码的生产/诊断边界。以下实现结论依赖这些固定 SHA 下的真实源码，文末提供定位。**没有重新在本轮独立编译和运行全测试矩阵；历史 CI 的通过不能替代新环境复测。没有审完仓库内每一份历史研究数据及每一条测试断言，更不能保证 100% 不存在隐藏缺陷。**
>
> 如当前 `main`、版本、源清单变化，请先检查确切 Git diff；不要把本篇冻结基线导读当作未来版本的源码事实。项目历史/规范文件可能过时；生产行为必须以固定源码和可复现实验为准。

## 0. 首先读懂这个库究竟在做什么

Full 不是传统的一次性 `qsort` 替代品，也不是数据库。其职责被分为三种不同数据模型：

1. **可变实时顺序**：`LksOrder` 为一组长期存活的 *occurrence* 管理相对先后；`LksManagedOrder` 使同一实时结构按比较器自动定位。
2. **不可变历史顺序**：`LksSnapshot` 捕获一个时刻的逻辑顺序及调用者提供的关联字节，再由 LS1 和 LKS4SNP1 进行历史定位与序列化。
3. **不可变批处理顺序**：`LksGroup`、`LksGroupBatch` 和 `lks_sort` 负责稳定排序、分块排序及稳定归并，不共享实时 Order 的结构。

贯穿所有模块的边界：

- **应用拥有业务对象和业务 ID**；Full 的 Order/Group 仅借用业务对象指针，不替应用释放其内容。
- **Live Handle 只代表当前 Order 中的一次存活插入**；一个业务指针可以多次插入，形成多个 Handle。
- **实时 Handle、业务永久 ID、历史 LS1 Key 是三种不同的东西**；没有自动的“持久 Handle”。
- 生产运行时的顺序管理不持续制造 V3 的 Path/LK1 坐标；旧格式代码还在，是为了兼容历史导入。
- Full 自身没有负责磁盘文件写入、崩溃原子提交、数据库事务、SQL 或多人协同排序的模块。

建议新人按 **§1 数据结构 → §2 插入 → §3 移动/删除 → §4 Managed → §5 Snapshot → §6 恢复 → §7 批处理 → §8 兼容代码** 的顺序阅读，不必一开始通读全部历史研究分支。

## 1. 从构建入口分清“生产代码”和“历史代码”

**真实生产清单**：[`cmake/ProductionSources.cmake`](../cmake/ProductionSources.cmake)。根 `CMakeLists.txt` 用该清单生成 **STATIC** 库 `layerkeysort`，对外目标名 `LayerKeySort::layerkeysort`，C17 标准。

| 生产模块 | 源文件 | 主要职责 |
| --- | --- | --- |
| 分配与公共状态 | [`lks_alloc.c`](../src/lks_alloc.c)、[`lks_base.c`](../src/lks_base.c) | 内存分配抽象、诊断注入、Status 文案 |
| 实时核心 | [`order.c`](../src/order.c) | 分块、AVL、稳定 Handle、插入、删除、移动、比较、Cursor |
| 比较器外观层 | [`managed_order.c`](../src/managed_order.c) | 依靠同一 Order 核心进行 lower/upper bound、稳定有序插入 |
| 快照生成与读视图 | [`snapshot.c`](../src/snapshot.c) | 复制 Namespace/关联数据、来源标记、逻辑版本 |
| 历史排序 Key | [`snapshot_key.c`](../src/snapshot_key.c) | LS1 格式化、验证与同域比较 |
| 二进制编解码 | [`snapshot_wire.c`](../src/snapshot_wire.c) | LKS4SNP1 格式、验证、编码及解析 |
| 历史恢复与导入 | [`snapshot_restore.c`](../src/snapshot_restore.c)、[`order_bulk.c`](../src/order_bulk.c) | Resolver → 新 Order，按序批量建块/建 AVL，旧 LK1 导入 |
| 批处理 | [`immutable_group.c`](../src/immutable_group.c)、[`sort.c`](../src/sort.c) | Group、Batch、稳定 Merge、独立指针排序 |
| 旧数据读取兼容 | [`path.c`](../src/path.c)、[`path_text.c`](../src/path_text.c)、[`path_order_key.c`](../src/path_order_key.c)、[`slot_codec.c`](../src/slot_codec.c)、[`path_compare.c`](../src/path_compare.c) | V3 Path/LK1 的私有表示、语法与比较 |

根构建另定义 **`LKS_LEGACY_REGRESSION_SOURCES`**：`src/gap.c`、`src/tree.c`、`src/group.c`、`src/bulk.c`。这些是**历史测试/基准目标额外链接的 V1–V3 机制**，不属于常规 Full 生产库。不要因为它们与 `order.c` 同处 `src/` 就认为 V4 实时排序调用旧 Tree/Gap/Group。

`LKS_ENABLE_ALLOC_DIAGNOSTICS` 使测试/测量版本添加诊断计数与确定性内存分配故障注入；普通生产版本不含这些共享可变诊断状态。`LKS_ORDER_BLOCK_CAPACITY` 可以在研究构建中屏选不同容量，但生产默认 **64**，不是冻结 ABI 常量。

## 2. 实时 Order 的数据结构：为什么能移动而不全局改 Key？

实现在 [`src/lks_order_internal.h`](../src/lks_order_internal.h) 和 `order.c`。

### 2.1 三层对象

**`LksOrderHandle`（一次存活 occurrence）** 有三个成员：

- `block`：目前所属的块；
- `local`：块内数组下标；
- `item`：应用传入的非 NULL 业务对象指针。

**`LksOrderBlock`（相邻记录的一组容器）** 有：

- `records[CAPACITY + 1]`：存放 Handle 指针的连续数组；额外一槽供插入时暂用；
- `count`：当前居民数量；
- `previous/next`：整条块序列上的前后邻居；
- `left/right/parent`、`height`、`subtree_blocks`：对**块**建立的 AVL 平衡索引；
- `owner`：指向当前 `LksOrder`，这是 v4.1.0 相比 v4.0.0 下沉到 Block 级的元数据。

**`LksOrder`** 有 AVL `root`、顺序链首尾 `first/last`、记录数 `count`、块数 `blocks`、逻辑修改版本 `revision`、回调占用标记 `callback_active` 和快照来源标记 `source_marker`。

示意（B=块，H=独立分配的 Handle）：

~~~text
Order
  AVL root ── B2 (block index, not user-visible Path)
              /    \
            B1      B3
  linked order: B1 <=> B2 <=> B3
  each block: [ H* H* H* ... ]  (up to 64 resident Handles)
  each H: { block, local, caller-owned item* }
~~~

这是一个**按块顺序隐式组织的 AVL**，不是“以业务 ID 或排序字符串为键的 AVL”。AVL 节点包含块，不包含每一条记录；块内数组保持连续逻辑顺序，块间用 `previous/next` 直接遍历。

### 2.2 关键不变量

- 默认每块提交后的容量上限为 **64**。多块场景下每块至少 **32** 条；全局唯一块允许低于 32，甚至删除成空后移除。
- 每个存活 Handle 分开分配，移动记录时可以调整其 `block/local`，但**不会改变 Handle 自身的地址**。
- 每个块的 `height` 与 `subtree_blocks` 必须匹配 AVL 实际子树；`subtree_blocks` 数的是**块数**，不是记录数。
- AVL 中序遍历与块双向链顺序必须相同；块的 `records` 顺序与真实逻辑顺序相同。
- `v4_locations` 在块内插入、分裂或合并后重写受影响 Handle 的 `block/local`，并清掉不用的数组槽位。

调试构建的 `lks_order_internal_valid` 检查高度、父子指针、占用范围、块双链、Handle 所在位置、总量和正反遍历；**这是私有检查器，不是公开 API，也不能检测已经释放的悬空指针**。

### 2.3 为什么能快速比较位置？

`lks_order_compare`：若两个 Handle 在同块，直接比较 `local`；否则用 `v4_rank` 结合每个 AVL 节点的 `subtree_blocks` 算出其块序号，再比较块序号。

所以不同块时访问的是 AVL 高度上的父指针与子树**块数量**，不是遍历两个元素之间的全部记录。公开复杂度为同块 O(1)、跨块 O(log(1+M))，其中 M 为块数量。

`lks_order_next/previous` 直接在块内邻居或相邻块首尾之间跳转；不通过 AVL 逐项查找。调用者不能凭 C 指针的数值大小判断顺序。

## 3. Explicit Order 的完整执行过程

核心入口在 [`src/order.c`](../src/order.c)。

### 3.1 创建和析构

- `lks_order_create`：分配并清零一个 Order；一开始没有块与 Handle。
- `lks_order_destroy`：沿 `first → next` 的块链，依次释放该块所有独立 Handle、块，再释放来源标记引用和 Order 自身；**业务 item 始终由应用释放**。
- 销毁前要确保 Cursor 不再使用 Order、没有并发修改或非法回调调用。

### 3.2 插入：先准备，再不可失败地提交

公开入口为 `lks_order_insert_front/back/before/after`，最后进入私有 `v4_insert(order, block, at, item, out_handle)`：

1. 清零输出；验证 Order、item、输出参数及相对操作的同源存活 anchor。**已释放 Handle 是未定义的 C 指针生命周期误用，不能指望库安全识别。**
2. 检查回调重入，记录总量和 `revision` 的容量限制。
3. 独立分配新的 Handle；如果当前没有块，或者目标块已满（64），预先分配一个 **spare Block**。
4. 所有可能失败的分配都完成后，才进入 `v4_place` 的提交区；因此 OOM 不会使原有顺序和旧 Handle 半更新。
5. 在目标块下标 `at` 处右移数组、插入新 Handle；如超过容量，则按大致一半拆成两块，调用 `v4_locations` 更新全部受影响 Handle，再 `v4_link_after` 把新块接入链表和 AVL。
6. `v4_balance` 沿父链刷新高度/子树块数，并进行必要的左右单旋/双旋。
7. 最后 `count++`、`revision++`，返回新 Handle。

**注意**：拆块移动的是 Handle *指针*，不是业务对象，也不是 Handle 本身；不会重建全部记录的动态 Path。结构性目标成本 `O(B + log(1+M))`，B 为私有固定块容量。操作系统分配器、内存页错误等完整延迟不包含在这个结构公式里。

### 3.3 AVL 维护不是额外的排序规则

- `v4_link_after` 在顺序上连接块，并在 AVL 中隐式追加到相邻块之后；找到右子树的最左位置即可确定连接点，没有业务 Key 搜索。
- `v4_rotate_left/right` 保持 AVL **中序块顺序不变**，更新高度、`subtree_blocks` 和父子链接。
- `v4_detach` 从双链及 AVL 移除块。两子节点时**物理移植后继块对象**，而不是复制另一个块的数组来替代，以保护 Handle→Block 位置关系。
- `v4_balance` 自变动节点向根恢复 AVL 不变量。

### 3.4 删除：移除 occurrence，不删除业务对象

`lks_order_remove`：

1. 验证同 Order 的存活 Handle、重入和 revision 上限。
2. 从目标块数组中左移，`block->count--`、`order->count--`。
3. 如果唯一块被删空，直接从链和 AVL 摘除该块。
4. 如果多块场景出现低于半满的块，调用 `v4_repair`：将不足块与其相邻块的记录暂放在栈上数组；总数量不超过 64 就合并并移除右块，否则重新分配相邻两块的记录，使它们恢复合理占用。
5. 更新存活 Handle 的新 `block/local`，`revision++`，释放被删除的 Handle；如果调用者要求，则返回原始业务 item 指针。

该操作不调用业务析构函数，不移动存活 Handle 的地址，也不分配新的堆内存。被删 Handle 从此立即失效。

### 3.5 Move：距离远不意味着改写中间所有记录

`lks_order_move_front/back/before/after` 进入 `v4_move`：

- **无操作情形**：移动到自己前/后、已经紧邻目标、已经在端点等，直接 OK；不分配、不修改 `revision`，原 Cursor 仍有效。
- **同块移动**：计算源下标、目标下标，处理取出后的下标偏移，仅位移目标区间内的 Handle 指针并修正相应 `local`；无需分配。
- **跨块移动**：若目标块满，先分配 spare；在任何可失败环节前准备好。然后从源块摘除 Handle 指针，插入目标块（必要时拆块），最后对源块执行低占用修复。
- 真正成功修改后 `revision++`；记录数不变，业务 item 和同一 Handle 地址不变。

这解决 V3 时代大量 live Path 全局重标记的负担，但**不是“任何操作 O(1)”**：仍有局部块数组搬移、AVL 更新、分配器开销。

### 3.6 读取、Cursor 和逻辑修改版本

- `first/last/next/previous/item/size` 提供普通借用式读取；在外部保证无冲突修改的前提下使用。
- `lks_order_cursor_create` 记录当前 `revision` 和下一 Handle；`lks_order_cursor_next` **首先比对版本**，若自创建后发生真实修改就返回 `INVALIDATED`，在读取可能已被释放的缓存 Handle 之前终止。
- 不改变顺序的 move、失败操作和纯查询不增加版本，所以不使 Cursor 失效。
- Cursor 自身借用源容器；**Cursor 必须先于源容器销毁**。

## 4. ManagedOrder：不是另一个排序树

实现：[`src/managed_order.c`](../src/managed_order.c)，结构 `LksManagedOrder { core, comparator, active }`。

它内部创建一个**私有 `LksOrder`**，把调用者的比较器描述符复制进来（比较器 context 仍然借用），没有额外的排序树、业务 Key 注册表或全新 Handle 类型，也没有把 core 作为公共可变接口暴露。

**插入路径**：

1. `lks_managed_order_insert` 进入 `active=1`。
2. `v4_managed_bound(..., upper=1)` 沿 AVL 寻找“块内最大元素严格大于新元素”的第一个块。
3. 再在该块内二分查找 **upper bound**。
4. 如果找到严格大于的 Handle，调用底层 `lks_order_insert_before`；否则插到末尾。
5. 重置 `active`。结果是**比较器相等的新记录排在已有相等记录之后**，保持稳定插入次序。

**定位路径**：`lks_managed_order_locate` 求 lower bound（首个 >= query）和 upper bound（首个 > query），由此确定 predecessor、first equal、first strictly greater；三个输出必须是互不别名的不同输出地址。比较器可返回相等，但这不表示同一 occurrence。

**防回调重入**：每次比较器调用期间设置 `core->callback_active`；同时 facade 使用 `active` 阻止同一 ManagedOrder 的再次访问。独立的其他容器不受这个标记影响。它是重入保护**不是多线程锁**。

**修改比较相关的业务字段**：不能原地改后假设顺序自动更新；应该从 ManagedOrder 删除、修改字段、再插入（新的 Handle）。不提供任意拖动式的 ManagedOrder move。

## 5. Snapshot：独立于实时结构的历史视图

生产实现：[snapshot.c](../src/snapshot.c)、[lks_snapshot_internal.h](../src/lks_snapshot_internal.h)。

Snapshot 不持有 live Handle、AVL/块布局或旧业务指针；其数据包含：

- `namespace_data`：快照命名空间的**字节副本**（1..UINT32_MAX 字节）。
- `count`：本次顺序的 occurrence 数量。
- `rows[i].end`：第 i 行关联字节的**累计结束偏移**；相等的相邻 end 代表空关联。
- `associations`：逐行拼接、由回调返回后立即复制的应用关联字节。
- `marker` + `revision`：仅用于判断 Snapshot 对原来源是否仍是当前版本；不是持久业务 ID。

### 5.1 捕获调用链

`lks_order_snapshot_capture` / `lks_managed_order_snapshot_capture` / `lks_group_snapshot_capture` → 共用 `lks_snapshot_capture_sequence`：

1. 验证选项与来源重入状态，复制 Options 描述符（避免回调篡改原描述符造成过程不一致）。
2. `lks_snapshot_new` 为 Snapshot Header + row 数组 + namespace **一次性合并分配**，并拷贝 namespace。
3. 按逻辑顺序遍历全部 occurrence；对每行最多调用一次 Association 回调，获得业务方提供的字节段；`snap_append` 将其复制到增长缓冲区，写入累计 end。
4. 成功收集所有行后，将 associations 容量裁剪至准确数据长度；该缩容也可能失败，失败则整个捕获不发布。
5. **最后才**建立/复用 SourceMarker 的引用，并存 `revision`；返回完整 Snapshot。捕获过程中任意 OOM、回调错误均销毁临时 Snapshot，不交出部分结果。
6. 用户回调自身引发的业务副作用**不会由 Full 自动回滚**。

关联字节是可由业务侧选择的“恢复所需线索”，常见做法是 occurrence ID 或业务关联信息，而不是偷偷将内存指针地址当作文件中的永久身份。没有 Association 回调时，关联数据全为空，Resolver 仍可以按 ordinal 自行恢复。

### 5.2 为什么旧 Snapshot 可以安全存活？

`LksSourceMarker` 引用计数在适用平台上用原子操作维护**标记的生命周期**。当原 Order/Group 已销毁，旧 Snapshot 仍可存储 Namespace、关联字节和读取历史 Key。

`snapshot_is_current` 要求来源仍是同一存活对象标记、revision 相同；已经通过 `deserialize` 载入的 Snapshot 没有原 live provenance，不能声称“还是当前内存 Order 的快照”。

标记引用的原子操作**不表示**整个 Order、ManagedOrder 或 Snapshot 捕获过程拥有通用并发修改安全性。

### 5.3 LS1 历史 Key 精确是什么？

格式在 [snapshot_key.c](../src/snapshot_key.c)：

~~~text
LS1.<namespace lower-case hex>.<16位小写十六进制行序号>
例：LS1.64656d6f.0000000000000002
~~~

- `key_length = 21 + 2 * namespace_size`，不含终止 NUL。
- 格式化对 Namespace 逐字节编码，再把本次 Snapshot 的 `row` 转换成 16 个小写 hex 字符。
- 验证器拒绝非法字符/语法；比较器要求**相同 namespace/domain**，否则返回 `DOMAIN_MISMATCH`，同域按 ordinal 比较。
- **LS1 不能作为不同历史时刻记录身份**；相同 namespace 的不同快照可能给不同业务对象产生相同 ordinal Key，故上层须单独标识版本/快照。

## 6. LKS4SNP1：真正保存的二进制是什么？

实现：[snapshot_wire.c](../src/snapshot_wire.c)。**由调用者提供缓冲区**，Full 负责字节格式，不直接创建文件。

### 6.1 二进制布局（Big Endian）

~~~text
offset  width  contents
0       8      ASCII "LKS4SNP1"
8       4      reserved flags，必须为 0
12      4      namespace 字节长度
16      8      occurrence 数量 N
24      D      namespace 原始字节
...     8+Ai   第 i 行 association 的 64 位长度 Ai + Ai 字节数据
重复 N 次；不得有多余尾部字节
~~~

完整大小是 `24 + D + 8*N + sum(Ai)`。长度字段均按源码 `snap_be_write/read` 使用显式大端，不是把 C 结构直接 `fwrite` 到文件，因此不依赖本机结构体对齐布局。

`lks_snapshot_serialize` 检查调用方缓冲区长度；不足则返回 `BUFFER_TOO_SMALL` 且不修改目标缓冲区，成功后按固定格式写入，不进行内部堆分配。

`lks_snapshot_deserialize` 首先**完整扫描输入字节并验证** magic、flags、域长度、数量、每行长度、算术溢出和尾部长度，确认结构有效之后才分配 Snapshot/associations 并拷贝。完成后得到独立持有字节的历史 Snapshot。

**不提供**磁盘事务、完整性校验和抗静默位翻转 checksum；这些必须由上层持久化协议处理，不能因为文件格式有版本号就等同于可靠数据库。

### 6.2 如何恢复可编辑的 Order？

`lks_snapshot_restore_order(snapshot, resolve, context, out_order)` 位于 [snapshot_restore.c](../src/snapshot_restore.c)：

1. 分配暂用的 N 个 `void*` 槽位（空 Snapshot 则免此分配）。
2. 按快照行序逐项取 association 和 ordinal，调用应用提供的 **Resolver**，要求恢复每一行的**非 NULL、调用者拥有的业务对象指针**；同一业务指针可重复出现。
3. 解析成功后调用 `lks_order_bulk_build`，一次构建新的实时 Order。
4. `order_bulk.c` 按默认块容量计算块数，均衡填充各块，独立分配每个新 Handle，链接块前后关系；随后 `snap_bulk_index` 递归按顺序取中间块构建平衡 AVL，并计算高度和 `subtree_blocks`。
5. 成功返回一个**新的** Order 和新的 Handle；旧快照只保留历史数据，不能恢复旧内存指针的身份。失败则输出 NULL，清理本库临时内存；应用 Resolver 的外部副作用/已创建业务对象不由 Full 回滚或释放。

**关键成本**：构建 Order 需要逐 occurrence 的 Handle 分配，恢复过程还需要 N 指针临时数组。它不是 O(1) 地“重新映射磁盘里的 AVL”。

## 7. Group、Batch 与一次性排序：为何要单列一组？

实现：[immutable_group.c](../src/immutable_group.c)、[sort.c](../src/sort.c)、内部结构 [lks_immutable_group_internal.h](../src/lks_immutable_group_internal.h)。

- **`lks_sort`**：对应用指针数组稳定排序；先将原数组复制到临时空间，用递归稳定 Merge Sort 在副本排序；只有分配/比较阶段完成后才 `memcpy` 回原数组，因此分配失败时输入数组保持不变。
- **`lks_group_build`**：复制输入指针并稳定排序到独立的 `LksGroup` 中；Group 持有自己的指针数组，却**不持有业务 item 内容**。
- **`lks_group_merge`**：归并两个已按兼容比较器排序的 Group；同值时优先取 base（左侧），使跨来源相等项保留稳定先后。
- **`lks_group_batch_build`**：按调用者给定 `group_size` 切分输入，每段分别建立 Group，Batch 持有各 Group。
- **`lks_group_batch_merge_all`**：按 `group_size` 起步，逐轮翻倍进行宽度归并，使用独立 scratch，最后生成**独立拥有指针数组**的新 Group；每一层在比较器相等时仍按早期来源优先。
- **`lks_group_snapshot_capture`**：将已有 Group 的有序指针序列转入与 Order 相同的 Snapshot/LS1/wire 逻辑；Group 的逻辑 revision 为 0，来源标记用于本 Group 的当前性判断。
- **防重入**：Group/Batch 在调用比较器/关联回调期间设置 `busy`；避免在来源尚被调用时被访问/销毁。与多线程互斥不同。

它们不维护动态 Handle，也不维护实时 AVL；不能把 `Group` 理解成另一个 Mutable Order。

## 8. V3 旧 Path/LK1 为什么还在？现在真正使用哪些部分？

历史身份与现行生产职责需要严格区分：

**当前生产会用到的旧格式路径**：

`lks_order_import_v3_lk1` → `lks_path_order_key_parse` → 临时 `LksPath` → `lks_path_compare` 排序 → 拒绝重复坐标 → `lks_order_bulk_build` 得到**不保留 Path 的 V4 新 Order**。临时 Path 在成功/失败退出前释放。

相关生产文件：

- [path.c](../src/path.c)：方向、slot/level 数组、append/clone/destroy 和内存布局；slot 为 16 bit，level 使用 `size_t`，共享按容量布置的一块 steps 存储。
- [slot_codec.c](../src/slot_codec.c)：私有 54 字符基数、每 slot 三字符的人类可读编码；规避易混淆字符。
- [path_text.c](../src/path_text.c)：历史 Path 的显示文本格式/严格解析；显示文本**不能替代**有顺序语义的 LK1 Key。
- [path_order_key.c](../src/path_order_key.c)：严格的 `LK1:` 版本化 ASCII 字节顺序编码与解析；含方向、level 和 slot；只为旧格式导入。
- [path_compare.c](../src/path_compare.c)：按方向顺序负/零/正、层级与 slot 规则比较历史 Path。

**只在历史回归/基准目标额外参与链接**：`gap.c` 的 Path 前后/中间坐标生成，`tree.c` 的旧 Path-keyed Tree 与 OrderedTree，`group.c` 的旧 Group/Batch，`bulk.c` 的旧批量生成。它们是 V1–V3 行为研究和回归对照的一部分，不表示 Full V4 公共头又重新开放了 Tree、Path 或旧有序坐标接口。

**重要结论**：`src/path*.c` 出现在当前生产链接里，是为了严格兼容旧 LK1；不是 V4 live mutation 仍在增量重写 Path，也不是在私有文件里偷留了另一套实时主引擎。

## 9. 分配、错误处理、重入、线程和生命周期

实现：[lks_alloc.c](../src/lks_alloc.c)、[lks_alloc_internal.h](../src/lks_alloc_internal.h)、各公开头注释。

### 9.1 分配器分层

- 生产配置的 `lks_alloc/lks_realloc/lks_free` 基于标准 `malloc/realloc/free`，不会增加生产全局统计状态；不依赖 SQLite、LMDB 等第三方存储组件。
- 测试诊断配置记录分配调用、实时/峰值请求字节、分类统计，能够指定第几次分配故障；部分旧式 tag 是历史兼容的诊断分类。
- 记录、块、快照、临时数组等分别分配；已发布 v4.1.0 主要降低单 Resident 与 Snapshot 元数据请求字节，但**仍然每 occurrence 分配一个 Resident**。
- 库中关于强失败原子性的承诺指**指定 API 面对可恢复错误时的自身状态**；它不负责业务系统写盘、外部回调副作用或操作系统突然断电的事务回滚。

### 9.2 常见错误语义

- `LKS_STATUS_INVALID_ARGUMENT`：空指针、不合法参数、跨 Order Handle 等。
- `LKS_STATUS_OUT_OF_MEMORY`：可失败分配过程无可用内存。
- `LKS_STATUS_BUFFER_TOO_SMALL`：序列化或格式化缓冲区不够。
- `LKS_STATUS_CAPACITY_LIMIT`：计数或版本不能再增长等受保护边界。
- `LKS_STATUS_INVALIDATED`：Cursor 看到来源真实修改。
- `LKS_STATUS_REENTRANT`：同源回调期间的非法再次访问。
- `LKS_STATUS_DOMAIN_MISMATCH`：比较两个不同 LS1 Namespace 的历史 Key。
- `LKS_STATUS_ALREADY_EXISTS`：例如 V3 导入重复历史坐标。

某些指针型 getter 因返回类型约束，在无效/忙碌时返回 NULL；这不等同于能安全读取已经释放的 Handle。

### 9.3 线程安全不是“一点原子操作就全线程安全”

Snapshot SourceMarker 的引用计数存在原子操作，但 **Order、ManagedOrder、Group 的可变访问仍要求调用者自己同步**。回调活跃标记只防同源回调重入，不是互斥锁；调用者应保证业务 item、比较器 context 在要求的生命周期内有效。

## 10. 性能、算法成本及其适用边界

- Live 插入/删除/移动的结构成本与**局部块容量 B 和 AVL 块数 M**相关，而不是改变两条记录之间所有元素的外部排序 Key。
- 读写整个 Snapshot 要遍历 N 个 occurrence，拷贝关联字节总量 A：这项成本是显式、线性的，**不会因为实时修改变快而自动消失**。
- 并非所有操作都比旧版快：v4.1.0 的研究保留了随机查找、Namespace Key 格式化、宽关联序列化、部分 churn/移动/析构的回归证据。
- 实时内存使用还包含应用自己的业务 item、系统分配器元数据与碎片、页缓存；测试记录的 requested bytes **不等于 RSS**。
- 对数据库等外部项目，应先建立真实使用行为的 trace，再决定有无理由增加增量持久化或新的 rank/定位 API；**Full 现在没有磁盘原生排序文件和通用数据库事务能力**。

来源：[v4.1.0 变更与限制](V4_1_0.md)、[当前路线与剩余成本（Issue #3）](https://github.com/RXY712200/LayerKeySort/issues/3)、[历史问题（Issue #4）](https://github.com/RXY712200/LayerKeySort/issues/4)。这些资料解释为什么做出取舍，**具体行为仍以生产源码为准**。

## 11. 实际上从一次“编辑 → 保存 → 重新打开”会经过哪些函数？

假设外部程序维护有序图层列表 A/B/C：

~~~text
外部应用创建记录 A/B/C（数据和业务 ID 属于外部）
    └─ lks_order_create()
        ├─ lks_order_insert_back(A) -> Handle HA
        ├─ lks_order_insert_back(B) -> Handle HB
        └─ lks_order_insert_back(C) -> Handle HC

用户将 C 移动到 A 之前：
    └─ lks_order_move_before(HC, HA)
        ├─ 校验、识别是否 noop
        ├─ 同块：局部指针移动；跨块：先准备再摘取/安置
        ├─ 必要时维护块/AVL
        └─ revision++

外部应用希望保存：
    └─ lks_order_snapshot_capture(...)
        ├─ 顺序遍历并复制应用提供的 association
        └─ lks_snapshot_serialize(...) -> 调用方缓冲区
            └─ 外部程序按自己制定的文件事务方式保存字节

进程关闭后，外部程序从自己的存储读取：
    └─ lks_snapshot_deserialize(bytes)
        └─ lks_snapshot_restore_order(snapshot, resolver)
            ├─ resolver 将每行 association 映射到新的业务对象指针
            ├─ bulk_build 均衡创建块、新 Handle
            └─ 新 Order 可以继续移动/删除/插入
~~~

上述流程的**磁盘读写、文件提交、业务 ID、故障恢复**都由调用方实现，不能理解成 Full 自动拥有这些能力。

如果业务对象同一指针有多个 occurrence，而外部程序要精确恢复多次插入的身份，association 应当区分这些 occurrence；否则仅靠业务对象 ID 可能不足以还原一对一映射。

## 12. 要修改实现时，先从哪里查？（源码入口 → 测试）

| 修改/审计目标 | 首要源码入口 | 相关已有测试入口 |
| --- | --- | --- |
| Handle、块、AVL 不变量 | `src/lks_order_internal.h`、`src/order.c` | `tests/v4_order.c`、`tests/v4_mutation.c` |
| 插入/移动/删除失败原子性 | `v4_insert/v4_place/v4_move/v4_repair` | `tests/v4_order.c`、`tests/v4_mutation.c` |
| Managed 比较器排序和相等元素 | `src/managed_order.c` | `tests/v4_mutation.c` |
| Cursor 与 revision | `src/order.c` 的 Cursor 函数 | `tests/v4_mutation.c` |
| Snapshot/SourceMarker/回调 | `src/snapshot.c`、`src/lks_snapshot_internal.h` | `tests/v4_snapshot.c`、`tests/v4_marker_threads.c` |
| LS1 Key | `src/snapshot_key.c` | `tests/v4_snapshot.c`、`tests/v4_rc_torture.c` |
| LKS4SNP1 数据与破损拒绝 | `src/snapshot_wire.c` | `tests/v4_snapshot.c`、`tests/v4_wire_probe.c`、`tools/verify_v4_wire.py` |
| 恢复批量构建 | `src/snapshot_restore.c`、`src/order_bulk.c` | `tests/v4_snapshot.c` |
| Group/Batch 与 stable sort | `src/immutable_group.c`、`src/sort.c` | `tests/v4_immutable_group.c` |
| 旧 LK1 导入 | `src/path_order_key.c`、`src/path_compare.c`、`src/snapshot_restore.c` | `tests/v4_snapshot.c`、历史 parser/regression 测试 |
| 公开 API 冻结 | `include/layerkeysort.h` | `tests/v4_api_compile.c`、`tests/v4_freeze.c`、`tools/verify_v4_api.py` |
| 生产源/两文件发行包 | `cmake/ProductionSources.cmake`、根 `CMakeLists.txt`、`tools/amalgamate.py` | [验证指南](VALIDATION.md)、发行包验证脚本 |

**注意**：上表只标识源码已核对的相关测试入口，**不等于本轮逐条执行并复验每个测试断言**。历史发布记录称正常配置可运行 39 项 CTest，具体数量受构建选项、Python 等条件影响；应使用实际构建的 `ctest -N` 和结果确认。

## 13. 可复现源码审查与构建步骤

取得准确提交并锁定，而不是未经约束地跟随不断更新的 `main`：

~~~sh
git clone https://github.com/RXY712200/LayerKeySort.git
cd LayerKeySort
git checkout 70d7f0ef5857b54381b863d292a213909d87459f
git rev-parse HEAD
cat cmake/ProductionSources.cmake

cmake -S . -B build -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
~~~

另按 [验证指南](VALIDATION.md)运行 API manifest 校验、分发校验、C/C++ 客户端和适用平台/sanitizer；测试命令应在具体系统上采用正确的编译器与生成器配置。

**本轮审读实际已完成**：从 GitHub 固定 SHA 读取全部 16 个生产 C 文件及主要头文件，核对生产源清单、根 CMake、相关源码调用链、代表性 V4 测试入口和历史兼容/回归源码归属。

**本轮没有完成、不能假装完成**：在可执行环境重新编译全部配置、重新运行 CTest、逐条独立复核所有历史基准 CSV/研究分支、覆盖全部操作系统/故障注入组合、证明整个库形式化无缺陷。若进行版本升级、修改算法或声称完整审计 PASS，仍需独立执行相应验证。

## 14. 源码真相优先于本文；今后怎样防止这份说明再过时？

- **新实现影响本篇描述时，同 PR 更新相关章节**，标明精确 SHA、旧→新调用变化，以及用户可观察语义；勿用改写旧数据的方式制造“从未发生过”的历史。
- Full 对外消费者的升级影响另应记录在独立的 [Full Consumer Handoff PR #24](https://github.com/RXY712200/LayerKeySort/pull/24) 引入的交接规范中（**该 PR 未合并之前，请不要把其文档当成已进入 main 的文件**）。
- 本文用于**先了解完整工作流程并快速定位源码**，不是把源码藏在文档后面。实质改动时必须按该提交及现行代码直接复核，并执行适当测试。
- 后续如发现源码与本文冲突，标出函数、真实 diff 与复现证据并修正文档；不凭聊天记忆猜测、不将文档描述反过来当作源码行为。

**一句话理解 Full：它把长期变化的逻辑顺序放在“稳定 occurrence Handle + 有序块链 + 块级 AVL”中；把需要离开运行时的历史顺序放在“独立 Snapshot + LS1 + 显式二进制格式”中；把一次性排序放在“稳定复制/归并”中。三者边界清晰，业务身份与物理持久化始终属于上层应用。**
