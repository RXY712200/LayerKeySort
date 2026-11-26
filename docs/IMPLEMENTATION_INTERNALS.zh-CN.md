# Full v4.1.0：内部结构、函数执行路径和算法分支详解

恢复后同时构造非零标签和零标签块，分别核对标签路径与 AVL 回退；验证后恢复标签，不改生产算法。 [验证与源码基线](V4_2_0_PREVIEW3.md)。

> Preview.3 核验的生产源码为 [fe00536](https://github.com/RXY712200/LayerKeySort/tree/fe00536e7bd7f4e5128432ea4a29e54158074903)，沿用 Preview.2 开发版。Preview.1 与 Preview.1.2 的记录保留为历史。

标签维护只在接入新块后执行。搜索区间不超过 64 个块，边界必须有效，写入严格递增且不越过边界。失败时只保留新块的零标签；后续成功的区间维护可重新标记它。删除释放块，旋转不重标；快照恢复批量赋等距标签。参见候选架构。诊断 tag_writes 统计赋值，tag_repairs 统计宽度大于一的维护，tag_fallbacks 统计无标签接入，max_tag_writes 统计单次最大赋值数。 [候选验证](V4_2_0_PREVIEW1_2.md)。

> Preview.1 增量源码 `22544cf283694dd27cef203caba9cf1838845947`。v4.1.0 发布记录作为历史基线保留。

诊断构建中，`v4_check_node` 沿已验证的中序拓扑记录最后一个非零标签，跨越任意零标签也拒绝相等或递减标签；额外空间为验证器状态中的一个整数，不增加每块空间。详见 [本轮架构](design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md)。

Preview.1 的高度引导父链是当前无标签块的回退机制。两块均有非零标签时直接比较标签。生产 AVL 更新及批量构建继续维护 subtree_blocks，诊断检查其值；比较不读取它。records_reassigned 统计实际回填的位置条目。 见 [候选证据](V4_2_0_PREVIEW1.md)。

> 当前实现以 `fe00536e7bd7f4e5128432ea4a29e54158074903` 为准，已发布 Stable 对照为 `70d7f0ef5857b54381b863d292a213909d87459f`。本手册覆盖根目录 Full。参见 [实现审计与证据](V4_2_0_PREVIEW3.md)。

## 1. 所有 16 个生产源文件，内部机制各是什么？

| 文件 | 关键内部对象、函数和真实职责 |
| --- | --- |
| `lks_alloc.c` | `lks_alloc / lks_realloc / lks_free` 封装内存；诊断构建追加带对齐的统计 Header、按 Tag 记录请求字节并可确定性注入分配失败 |
| `lks_base.c` | `lks_status_string` 返回冻结的状态文字，不分配 |
| `order.c` | `v4_refresh/replace/rotate_left/rotate_right/balance` 维护块级 AVL；`v4_link_after/detach` 维护块链和树节点；`v4_place/insert/repair/move/locations/shifted_locations` 实现实时顺序 |
| `managed_order.c` | `v4_managed_compare_item` 包装用户比较回调；`v4_managed_bound` 做块 AVL + 块内二分，复用 Explicit 核心 |
| `snapshot.c` | `snap_marker_new/retain` 原子生命期计数；`lks_snapshot_new`、`snap_append`、`lks_snapshot_capture_sequence` 完成 Snapshot 的建造；`snap_live_next` 顺序遍历 |
| `snapshot_key.c` | `snap_nibble`、`snap_key_parse` 及 LS1 格式化/验证/同域比较 |
| `snapshot_wire.c` | `snap_be_read/write`、`lks_snapshot_serialize/deserialize` 编解码不可变快照的精确二进制格式 |
| `snapshot_restore.c` | `lks_snapshot_restore_order` 通过 Resolver 还原 item；`snap_legacy_compare` 与 `lks_order_import_v3_lk1` 做 V3 旧格式导入 |
| `order_bulk.c` | `lks_order_bulk_build` 均匀建块/新 Handle；`snap_bulk_index` 直接把块双链建成平衡 AVL |
| `immutable_group.c` | `ig_new/ig_next`，Group、Batch 的复制/稳定归并/快照管理 |
| `sort.c` | `lks_stable_sort_copy`、`stable_sort_range`、`lks_merge_sorted_pointers` 实现稳定归并 |
| `path.c` | V3 Path 结构、对齐的 slot/level 内存布局、增长/复制/销毁 |
| `path_text.c` | V3 Path **显示文本**长度、格式化和解析；虽被编译进生产源清单，**不是** LK1 导入必经函数 |
| `path_order_key.c` | V3 LK1 严格的字节顺序 Key 格式化和解析，导入时使用 `parse` |
| `slot_codec.c` | V3 Path 显示文本中的 54 进制、每槽 3 字符编码；LK1 的 slot 用的是另种十六进制布局 |
| `path_compare.c` | V3 Path 方向、层级、slot 和前缀深度比较，供 LK1 导入的排序与查重 |

**不在常规生产清单里的历史实现**：`src/gap.c`、`src/tree.c`、`src/group.c`、`src/bulk.c` 由 `LKS_LEGACY_REGRESSION_SOURCES` 添加到历史测试或基准目标。看到这些文件不代表当前生产 Full 仍保留 V3 live Tree 的公开能力。

内部头：`lks_order_internal.h`、`lks_snapshot_internal.h`、`lks_immutable_group_internal.h`、`lks_sort_internal.h`、`lks_alloc_internal.h`、`lks_legacy_internal.h`、`lks_path_internal.h`、`lks_slot_codec_internal.h`。普通应用只应包含 `layerkeysort.h`。

## 2. 实时排序核心的三层数据结构

`LksOrderHandle` 是一次独立插入的 occurrence，存三个字段：`block`、`local`、`item`。Handle 由堆单独分配，指针地址在自身仍存活时稳定。多个 Handle 可以引用同一个业务 `item`。

`LksOrderBlock` 持有：`owner`；AVL 左/右/父指针；按顺序排列的 `previous/next`；`subtree_blocks` 和 `height`；`count`；私有 `uint64_t tag`（0 表示回退）；`records[CAPACITY+1]` 这个 Handle 指针数组。默认 CAPACITY=64，额外一个槽位用于插入中的临时状态，正常提交时不超过 64。

`LksOrder` 持有 AVL 根 `root`、块链首尾 `first/last`、`count/blocks`、`revision`、`callback_active` 和 `source_marker`。

其真实结构可表示为：

~~~text
Item A (caller owned) ← HA { block=Block1, local=0, item=&A }
Item B (caller owned) ← HB { block=Block1, local=1, item=&B }
Item C (caller owned) ← HC { block=Block2, local=0, item=&C }

Block1.records = [HA*, HB*]
Block2.records = [HC*, ...]
threaded: Block1 <--> Block2

AVL: indexes Block1/Block2 by implicit in-order sequence
Order: root + first/last + logical count + revision
~~~

`subtree_blocks` 数的是**块**，不等于子树内的 item 数。候选的比较不读取该字段，仍由诊断核对其值。Full 当前也没有公开的随机第 N 项访问 API。

多块情形每块正常至少半满（当前最小为 32）；唯一块可以低于半满。AVL 的中序顺序、块链顺序、块内 Handle 指针顺序应当一致。

## 3. AVL 与块链接口：每个私有函数的职责

v4_tag_attach 的分配标签工作有固定探测和写入界限；耗尽局部空间时保留零标签。lks_order_compare 在任一标签为零时使用 AVL。诊断验证整个非零子序列严格递增，跨零标签块也要检查。

| 函数 | 实际逻辑 |
| --- | --- |
| `v4_height` | NULL 高度为 0，其他读 block->height。 |
| `v4_subtree` | NULL 的块数为 0，其他读 subtree_blocks。 |
| `v4_refresh` | 根据左右孩子重新计算高度和子树块数，并在诊断构建计数。 |
| `v4_replace` | 修改父节点指向 old 的那条链接或 root，替换为 replacement 并重设 parent。 |
| `v4_rotate_left` | 用右孩子做 pivot 左旋，保持中序顺序不变，刷新旧根和新根的高度/块数。 |
| `v4_rotate_right` | 对称右旋。 |
| `v4_balance` | 从改变的节点沿 parent 向上，每节点刷新并根据高度差选择单旋或双旋。 |
| `v4_minimum` | 沿左链接找到子树最小 Block。 |
| `v4_new_block` | 堆分配、清零并给新 Block 设置 owner、height=1、subtree_blocks=1。 |
| `v4_link_after` | 更新双向块链，并在 anchor 的 AVL 后继位置接入新块；空 Order 时直接建立 root/first/last；再平衡。 |
| `v4_detach` | 从双链及 AVL 删除一个块；两子节点时物理移植后继 Block，而不是复制其 resident 数组；从必要的位置再平衡。 |
| `v4_locations` | 遍历一块现存居民，更新各 Handle 的 block/local 并清理剩余数组槽。 |
| `v4_shifted_locations` | 普通操作仅回填受影响 local 后缀。 |
| `v4_mark_index / v4_begin` | `v4_begin` 清零已有的每操作计数；`v4_mark_index` 在已有固定长度数组中去重并记录 Block 指针，满时置 tracking_overflow；二者均不分配堆内存，正常生产构建无动作。 |

当 AVL 旋转时，块在逻辑顺序中的位置不变。因此 AVL 只是**顺序索引**，不是将用户元素按比较器键重新排序。

两子删除不拷贝整个后继块的记录，这是为了保留 Handle 指向具体 Block 的关系。AVL 节点可以移动树位置，但不能随意把另外一个 Block 的 resident 内容偷换成它的内容。

## 4. 插入的“准备 → 提交”完整路径

四个公开入口（front/back/before/after）调用 `v4_insert`；相对操作先通过 `v4_insert_relative` 验证同 Order 的 live anchor，再转为目标 Block 与下标。

~~~text
检查 order/item/out/anchor → 回调重入？ → count/revision 溢出？
                 ↓
          分配一个新 Handle
                 ↓
    目标没有块，或目标已满 64？
           是 → 先分配 spare Block
           否 → 无需 spare
                 ↓  以下不再做可恢复分配
             v4_place
          → 块内 Handle* 右移
          → 插入新 Handle 指针
          → 如满块溢出，把最多 65 个 Handle* 分到旧块/spare
          → 普通路径更新新成员 block 和位移后缀 local；分裂走 v4_locations
          → v4_link_after 与 AVL 平衡
                 ↓
        count++ / revision++ / 输出 Handle
~~~

【边界】如果 Handle 或 spare 分配失败，旧 Order 的结构、旧 Handle 和 revision 保持原状；成功后增加**一个** occurrence，业务 item 没有被复制。把同一个业务指针连续插两次会得到两个不同的 Handle。

## 5. 删除和块的欠占修复

`lks_order_remove` 使用 Handle 的 block/local 精确命中 occurrence，数组左移，减少块与 Order 的 count。

- **唯一块被删空**：调用 `v4_detach`，释放 Block，Order 变空。
- **多块且目标块少于半满**：`v4_repair` 选择目标及相邻 Block，先用**栈上的 Handle* 数组**串接两块记录。如果总数 <=64，就合并到左块、摘除右块；否则将其重新均分到左右两个块。
- **不欠占**：清除腾出的尾槽，并回填受影响的 local 后缀。

最后 revision++，释放被删 Handle；可选择输出原 item 指针。**没有业务 item 析构、没有释放未被删除的 Handle，也没有堆内存分配。**

注意：旧 Handle 释放后是悬空指针，不能继续传给任何函数做“活性检测”。

## 6. 移动：三个容易弄混的分支

1. **No-op**：移动到自己前/后、已经紧邻目标或已经在正确端点。立即 OK；无分配、不改变 revision、不使已有 Cursor 失效。
2. **同块 Move**：从 `from` 到相对 anchor 的 `to`；如果 source 在目标前，取出 source 后需要对目标下标减一。只移动目标区间内的 Handle*，更新对应 local；不分配。
3. **跨块 Move**：若目标块已满，先准备 spare；从源块摘去 Handle*，将**同一个 Handle**放进目标块；必要时触发目标拆块，之后对源块执行欠占修复。整个过程不会为被移动 occurrence 分配第二个 Handle。

真实 move 结束后 revision++。结构变动集中在源与目的相邻块和 AVL 局部路径，**不依赖拖动距离逐个重写中间全部元素的排序 Key**。

## 7. ManagedOrder：为何同一个 AVL 能做比较器检索？

`LksManagedOrder` 只是 facade：`{ core: LksOrder*, comparator, active }`，没有另建一棵 comparator tree。

`v4_managed_compare_item` 在实际执行用户 compare 时设置 core->callback_active，保证回调不能偷偷对同一 core 重入；facade 自身以 active 防同一 ManagedOrder 进入新调用。

`v4_managed_bound` 接收布尔 upper：upper=0 查首个 >= query，upper=1 查首个 > query。先根据每个 Block 的**最后一条 item**沿 AVL 选择可能包含边界的最左块，再对该块做二分，最终返回边界 Handle（可能 NULL）。

- 插入查 upper bound：新的等值 occurrence 放在所有已有等值 occurrence 后面。
- Locate 运行 lower 和 upper 两次查找；predecessor 为 lower 前一个（若 lower 不存在，则最后一个），equal 仅在 lower 比较结果为零时才返回，successor 为 upper。
- 修改比较器可见字段前，应用必须处理每一个相关 ManagedOrder 中引用该对象的**全部 occurrence**，以及依赖该对象排序的 Group 等其他结构。仅删除一个 occurrence 不够；Full 不会自动追踪跨容器共享对象。Group 不可原地删除记录，应由应用安排重建或替换。
- “remove → 修改 → insert”是多个独立操作，**不是 Full 事务**：成功 remove 立即使原 Handle 的全部副本失效；后续 insert 可能因 OOM 等错误失败，Full 不会撤销先前删除，也不保证恢复原 Handle。应用应保留业务对象及足够的顺序/身份状态，在失败后重建相关容器或按应用策略恢复；恢复插入本身也可能失败，必须继续处理错误。
- ManagedOrder 没有任意相对 Move，也没有供调用者修改其 core 的逃逸接口。

例：`[2a,2b,3,7a,7b,9]`，查询 7 得 predecessor=3、equal=7a、successor=9；新插入 7c 会位于 7b 和 9 之间。

## 8. Cursor 与 Snapshot currentness 的版本机制

`LksOrderCursor` 缓存 owner、下一 Handle、revision、方向。每次 next 先对比 revision，若实际修改过，返回 INVALIDATED 而**不会碰缓存中可能已释放的 Handle**。Cursor 自己没有持久化含义，也不能比 owner 活得更久。

Snapshot currentness 需要 **同一 SourceMarker 指针**和 **相同 revision**。它和 Cursor 的 revision 检查类似但多了来源身份。序列化再载入的 Snapshot 没有 live provenance，不能变回旧进程里的 current Snapshot。

SourceMarker 的原子操作只是管理标记最后释放的时机，**不代表 Order 或 Group 已经能被任意线程并发修改**。

## 9. Snapshot 的分配布局及主要内部 helper

`LksSnapshotRow { size_t end; }` 保存关联数据的累计结束偏移，空行也占一条 end 但 end 与上行相同。

`lks_snapshot_new` 检查 Namespace 非空、长度限制、行数和长度算术不溢出，一次性申请 `Snapshot 头 + Row 数组 + Namespace 原始字节`；association 字节另有独立扩容缓冲。

`lks_snapshot_capture_sequence`：

1. 验证来源的 busy 状态、options/out；复制 options 描述符，防止回调意外改原描述符。
2. 通过来源专用的 next 逐 occurrence 得到 item；若配置 Association 回调，设置忙碌标记、运行回调、恢复标记，要求回调正常返回。
3. `snap_append` 对每行关联字节执行几何增长、完整复制和累计 end 记录；每次长度有显式溢出保护。
4. 全部行成功后将 associations 容量缩到实际有效长度；缩容如果失败则整个 Snapshot 捕获失败，不公开部分数据。
5. 最后才经 `snap_marker_new` 或 `snap_marker_retain` 为此次成功快照建立来源记录，保存 revision，交出完整 Snapshot。

`snap_live_next` 在当前 Block 的 local 下标读取 item，到块尾转向 block->next；不用为每一行重新查 AVL rank。Group 则用 `ig_next` 直接按数组下标取 item。

`lks_snapshot_row_offset/size` 是内部 inline：第 i 行从上一行 end 开始，长度为此行 end−上一行 end。

`snap_marker_new` 以引用数 2 创建一个来源和首张 Snapshot 共用的 Marker。`retain/release` 原子增减并在引用数到 0 时释放 Marker；在引用数上限不能增加时报告容量错误。来源 Order/Group 可以比它的历史快照更早销毁。

**Association 返回 span 的生命周期**：数据必须在回调返回之后仍然有效，直到 Full 完成紧接着的复制。不能返回自动局部数组的地址，因为其生命周期在回调返回时结束；可使用仍存活的业务对象字段或由调用方管理的缓冲区。非零长度要求非 NULL 数据。

**失败原子性仅覆盖 Full 自有 Snapshot**：已执行的应用 Association 回调副作用不由 Full 回滚。

## 10. LS1 与 LKS4SNP1 的主要编码与校验分支

### 10.1 LS1：历史行序号，不是永久记录 ID

`snapshot_key.c` 里 `snap_nibble` 只识别数字及 a..f 小写；`snap_key_parse` 验证 `LS1.` 前缀、namespace 非空偶数位 hex、点号、正好 16 位 ordinal hex、无尾随字符。

`key_length` 返回 `21+2*namespace_size`（不含 NUL）；`key_format` 写入 Namespace hex 和固定 16 位 ordinal；`key_compare` 先验证两方，Namespace 不同则 DOMAIN_MISMATCH，同域比较 ordinal。

同一 namespace 的规范 LS1 键使用固定 16 位小写十六进制 ordinal，故其**字节级 ASCII 词典序与 ordinal 顺序一致**。外部数据库依赖该性质时须采用保持字节顺序的 collation，不能假定语言或忽略大小写的排序规则适用。不同 namespace 的 `lks_snapshot_key_compare` 返回 `DOMAIN_MISMATCH`；对这些字符串直接排序不建立有意义的业务记录先后。LS1 是历史快照坐标，不是跨快照版本的持久记录 ID；这与 V3 LK1 的 Path 顺序编码是不同契约。

### 10.2 Wire：明确字节序，不保存原生 C 指针

`snapshot_wire.c` 的 `snap_be_read/write` 仅处理四字节与八字节字段，使用无符号移位、组合和单字节访问；不要求对齐或本机端序。仍先完整验证后分配，无新增分配点，布局：

~~~text
magic = ASCII LKS4SNP1 (8)
flags = u32 BE 0
namespace_size = u32 BE, >= 1
row_count = u64 BE
namespace bytes
for each row: association_length u64 BE + raw bytes
end-of-input，不能有尾随垃圾
~~~

`serialize` 在检查容量之后将整个 Snapshot 编码进**调用方提供**的 buffer；`deserialize` 第一遍完整扫描所有行并做边界检查，第二遍才分配和拷贝。失败不返回半解码状态；加载结果没有 live SourceMarker。它不是磁盘 I/O，格式本身也没有校验和、WAL、文件写入提交或断电恢复协议。

## 11. Snapshot Restore 与 Bulk Build：为何不做 N 次公开 insert？

恢复先解析全部行，再批量构建；旧快照不携带实时标签、块或 Handle 地址。新增测试检查恢复后的标签间隔和 Handle 顺序，并对人为构造的有效混合标签状态验证双向 AVL 回退。

`lks_snapshot_restore_order` 对非空输入先分配 N 个临时 item 指针槽。按行序调用 Resolver，首次非 OK 或 OK 但 item 为 NULL 时立即停止，后续行不调用；仅全部必需行成功后才进入未发布的 bulk Order 构建。空 Snapshot 不调用 Resolver，允许 NULL Resolver，仍需分配一个新的空 Order，因此可能 OOM。失败清理 Full 自有临时分配、输出为 NULL，但不回滚应用回调副作用、不释放已经解析出的调用方 item。全部通过才调用 `lks_order_bulk_build`，最后释放临时指针槽。

`lks_order_bulk_build`：

- 根据 N、内部块容量计算块数，平均分配每块居民数；
- 从左到右逐块分配 Block、每次 occurrence 的 Handle，建立块双链；
- `snap_bulk_index` 以**中序从链表消耗块**的递归方法，先建左半子树、取中间块、再建右半子树，回填 parent/height/subtree_blocks。块按初始顺序以等距标签初始化；
- 建立全部成功后才交出新 Order，失败清理全部内部资源。

恢复的 Handle 全是**新分配的**，不能跨进程恢复旧 Handle 地址；内部私有函数不会通过公共接口暴露。

## 12. 一次性 Sort、Group 与 Batch 的所有核心算法

`lks_stable_sort_copy` 申请 sorted 和 scratch 两个指针数组，复制输入、调用 `stable_sort_range` 递归稳定归并，最后只将 sorted 返回。`lks_sort` 在这一步全部成功之后才覆盖调用者的原指针数组。

`lks_merge_sorted_pointers` 两路归并，比较器结果 <=0 时取左侧，保证等值时左侧来源优先。`stable_sort_range` 递归划分左右段、归并到 scratch、复制回当前区间。

`LksGroup { items,count,marker,busy }` 拥有已排序的指针数组但不拥有每个 item。`ig_new` 分配 Group Header；`group_build` 调 stable_sort_copy；`group_merge` 分配新 Group/目标数组并做一次归并，旧 Group 不变。即使两个输入是同一 Group，逻辑上仍是两条借用序列。

`LksGroupBatch { groups,count,group_count,group_size,busy }` 持有分片 Group。`batch_build` 逐片调用 group_build，失败释放已建片；`batch_group_at` 只返回 Batch 持有的借用 Group，不可自行销毁。`batch_merge_all` 将已排序片拷贝进新数组，仅当 `group_count > 1` 则额外申请 scratch（空和单 chunk 不分配此数组）；按 group_size 为初始宽度，依次翻倍，反复归并并交换输入/输出数组；结果是独立 Group，早片等值先于晚片。

Group 所谓不可变，指 **指针顺序和容器结构**不经 API 变化；外部修改 comparator 参与的业务字段会使其“当前数组已正确排序”的前置条件失效。

## 13. V3 Path/LK1 的每个生产模块怎样相互配合？

- `path.c`：Path 的方向、depth、capacity 与单块 steps storage。`path_storage_layout` 计算 uint16_t slots 和对齐后 size_t levels 的字节偏移，防溢出。`lks_path_reserve_one` 通常倍增容量；因 levels 起点随容量变化，可能需要 `memmove` 到新对齐位置。`path_slots/levels` 给出各数组视图。create/clone/append/destroy 负责结构生命周期。
- `slot_codec.c`：从专门选定的 54 字符表把 uint16 slot 编成 3 个可打印字符，解码时每位查字母表并检查结果 <=65535。它给旧版 Path **展示文法**使用。
- `path_text.c`：`measure_path_text` 计算可打印 Path 所需长度，含方向、递增 level 以差分或前缀表示、slash 和每段 3 字符 slot；`format/parse` 严格处理显示文本。**它不是 LK1 字节 Key**。
- `path_order_key.c`：`byte_count` 计算 level 编码长度；`measure` 防止长度溢出；`write_hex_byte` 写大写十六进制；`format` 生成以 `LK1:` 起头、方向码、层级/slot 编码、`!` 结尾的规范排序 Key；`parse` 拒绝不规范或不可表示的文本，重建 Path。
- `path_compare.c`：`direction_rank` 建立 NEGATIVE < ZERO < POSITIVE；`path_is_consistent` 验证 Path 状态；`lks_path_compare` 按方向、level、slot、前缀深度比较，负向首层 slot 方向倒转。
- `snapshot_restore.c` 中的 `lks_order_import_v3_lk1` 使用 `lks_path_order_key_parse` 解析键→建立临时 Path→按 `lks_path_compare` 对键排序→相同坐标返回 ALREADY_EXISTS→抽出业务 item→`lks_order_bulk_build` 建新 V4 Order→释放临时 Path。不保存 V3 Path 或旧坐标到新 Order。
- `gap.c/tree.c/group.c/bulk.c` 是独立的历史回归/基准源码，**不属于普通 Full 生产库**。不能把这些历史方法误认为当前 live engine 的主算法。

## 14. 诊断构建、OOM 与实际内存开销

`lks_alloc.c` 的普通生产分支基于 C 标准 `malloc/realloc/free`，不引入第三方数据库。诊断分支通过对齐 Header 附加申请大小和 Tag，记录 requested live/peak bytes、block 数、失败调用次数和各类别统计；提供按第 k 次分配故障注入。诊断全局计数不应被当作生产线程安全保证。

一次插入先分配新 Handle，必要时提前分配 spare Block，再进入不会可恢复分配失败的提交阶段；跨块 Move 对满目标也是先分配 spare 后变更。Snapshot 的 Association 回调、缓冲扩容、最终 shrink 和 Marker retain 都在输出正式发布之前；原生 sort 先复制排序后再写回输入。这些支撑了各自限定范围内的强失败原子性。

requested memory 不等于进程 RSS；即使用户 item 由应用持有、Full 统计释放为零，分配器缓存/碎片/系统内存也可能仍然存在。B64 是实现参数，4.1.0 减小 Resident 请求大小不代表消除了每 occurrence 分配成本。

## 15. 症状 → 应该检查的源码不变量

| 现象 | 优先检查 |
| --- | --- |
| 次序不对但数量正确 | `records[]`、before/after 的目标下标修正、`block->previous/next` |
| 跨块邻居错误 | 块链的首尾和 next/previous、各块的 count |
| compare 结果有误 | `lks_order_compare`、`subtree_blocks`、AVL parent 链 |
| 删除之后损坏 | `v4_repair` 的合并/重分布、`v4_detach` 的两子物理移植 |
| OOM 后原顺序改变 | `v4_insert/v4_move` 是否越过预分配边界 |
| Managed 重复值定位错误 | upper/lower bound 的严格与非严格比较、比较器语义是否稳定 |
| Cursor 碰到已删缓存 | revision 检查是否发生在读取缓存 Handle 前；源是否仍存活 |
| Snapshot currentness 意外为真 | SourceMarker 是否属于同一来源、revision 是否一致 |
| LS1 或 wire 不符合 golden | 大端长度、namespace 编码、累积 end 与 association slice、解析尾部检查 |
| 旧 LK1 导入顺序不对 | 旧方向/level/slot 比较规则、规范解析、重复坐标检测 |
| 峰值内存高 | 每居民分配、Block slack、Snapshot 关联增长和快照保留 |

这里是**从源码推导的定位地图**，不是宣称存在这些 Bug。相关的测试、构建命令和仍未验证的范围见 [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md)。

**本章限于固定 SHA 的实现细节，不承诺未来版本继续使用同样私有函数。对外调用应当只依赖 `include/layerkeysort.h`。**
