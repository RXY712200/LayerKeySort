# V4 Preview.3：不可变历史导出（历史译本）

原文：[V4_PREVIEW3.md](V4_PREVIEW3.md)，英文，固定于 [`41132918fed7a82355dcf3db4a7b03c68ea37df5`](https://github.com/RXY712200/LayerKeySort/blob/41132918fed7a82355dcf3db4a7b03c68ea37df5/docs/V4_PREVIEW3.md)。原件保留，下面版本/容量/临时契约属于历史阶段；[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

4.0.0-preview.3实验，推荐Stable当时v3.1.0；Preview.2冻结生产祖先，临时API/LS1/wire待Preview.5决定。本轮加export非新live算法。

## 两个世界

| 实时 | 历史 |
| --- | --- |
| LksOrder/LksManagedOrder | LksSnapshot |
| 同源resident Handle | ordinal行/复制字节 |
| 可变序列/稳定Handle | 成功捕获后不可变 |
| 无独立position key | 调用方buffer格式化LS1 |
| Handle不可序列化作数据库key | 规范可序列化历史域 |

Snapshot无item/Handle/block/context/旧source/Path，拥有namespace、row offset/length及紧凑关联buffer，ordinal隐式row下标，不存N份namespace文本/物化key。应用选1..UINT32_MAX任意namespace字节，库复制不生成/保证唯一。不同状态不要复用namespace；相同状态复用按应用政策。关联可重复，需区分occurrence则自编码，内存marker非export namespace。

## 捕获与关联寿命

Order/Managed capture必填输出先NULL，成功调用方拥有。操作复制options不留descriptor/context，namespace在回调前复制；NULL/空INVALID_ARGUMENT。可选LksSnapshotAssociationFn(item,uint64_t ordinal,context,&data,&size)按逻辑序每行恰一次，无callback空关联，非零size须非NULL，错误透传无部分对象。返回span须至回调后立即复制结束仍有效，局部自动数组无效；可复用应用scratch因下次前已复制。item/context仍应用。

回调正常返回，不能跨C longjmp/抛异常。同源getter NULL/0、Status REENTRANT、busy destroy忽略、递归capture拒绝，独立容器允许。Managed capture无compare。沿前驱／后继链接遍历一次，无逐行rank/Path/revision变化，失败也不使Handle/Cursor失效。成本O(N+A+callback)，关联几何增长可留slack而非每行分配，失败清临时。

## 来源与当前性

首成功捕获最终步骤才给live core单独私有marker，失败首捕获不留marker；core/Snapshot共享身份引用，复制revision。两种is_current须同marker同revision；query/capture/失败/no-op保留，真实插删移使旧快照stale，不同源永不current。加载快照无marker/revision provenance，即使序/域相同也非current；restore/import新core捕获时建立自身惰性身份。

引用增查capacity。GCC/Clang C17 atomic_size_t CAS retain/acquire-release decrement，MSVC Interlocked long，末引用恰free一次。来源可先销毁，各快照仅释放自身引用。原子不使容器线程安全。完成Snapshot允许并发读，应用保护寿命/销毁不竞争；可变来源和诊断分配器外部串行。

## 公开只读及输出

| 函数 | 结果 |
| --- | --- |
| lks_snapshot_destroy | free字节/marker，NULL安全 |
| lks_snapshot_count | size_t行数，NULL0 |
| lks_snapshot_namespace | 借用字节/size |
| lks_snapshot_association | size_t row关联，空NULL |
| lks_snapshot_key_length | 不含NUL字符数，NULL0 |
| lks_snapshot_key_format | 现有row至调用buffer |
| lks_snapshot_key_validate | 严格LS1 |
| lks_snapshot_key_compare | 严格同域ordinal |
| lks_snapshot_serialized_size | 精确blob字节，NULL0 |
| lks_snapshot_serialize | 写caller blob |
| lks_snapshot_deserialize | 严格借用input至拥有Snapshot |

View至销毁，无item accessor；必填view输出失败NULL/0，无效指针/下标INVALID_ARGUMENT。format/serialize不分配，短buffer不变BUFFER_TOO_SMALL，NULLbuffer INVALID_ARGUMENT。parse/restore/capture失败清必填对象输出。

## LS1 v1规范

```text
key = "LS1." namespace_hex "." ordinal_hex
namespace_hex = 2*K lowercase hexadecimal characters, 1 <= K <= UINT32_MAX
ordinal_hex = exactly 16 lowercase hexadecimal characters
hex alphabet = "0123456789abcdef"
```

namespace按输入字节高nibble先，ordinal无符号64位高nibble先。无空白/异大小写/多分隔/后缀/嵌入NUL，C终止NUL非key。长度21+2*K。

```text
LS1.00112233445566778899aabbccddeeff.0000000000000000
LS1.00112233445566778899aabbccddeeff.0000000000000001
```

同版本同namespace规范key共享前缀，固定ordinal首不同位按无符号数序，有序小写hex保ASCII strcmp符号相同。数据库需字节保持collation，非忽略大小写/语言排序。语义compare验证两者，不同域追加DOMAIN_MISMATCH=12、输出0；跨域字节序无live意义。validator允许全uint64，formatter仅实际row。

## Snapshot blob v1规范

| 偏移 | 字段 |
| --- | --- |
| 0 | 八ASCII LKS4SNP1 |
| 8 | 大端u32 flags恰0 |
| 12 | 大端u32 namespace长K非零 |
| 16 | 大端u64 row数N |
| 24 | K字节namespace |
| 24+K起 | N次：大端u64关联长及精确字节 |

无padding/尾字节，空仍需namespace，ordinal按wire出现序；不序列化revision/marker/items/Handle/拓扑/LS1字符串。大小24+K+8*N+A，移位/逐字节，与native endian/struct无关。

Parser先完整验证magic/flags/域/可用字节/count/逐长/受检总量/精确end才分配。Count须uint64及本地SIZE_MAX，维度/关联长须SIZE_MAX，不靠巨大声明推测分配；小地址空间拒绝不截断。Capture也查key/wire及NUL可放size_t。畸形/不可表示INVALID_ARGUMENT，OOM为OUT_OF_MEMORY/NULL；解后重编码逐字节同。Blob只给结构非checksum/认证/保密/任意关联位翻检测，应用外包，无filesystem/db依赖。

## 恢复与V3迁移

restore(snapshot,resolve,context,&order)逐行调用LksSnapshotResolveFn(association,size,uint64_t ordinal,context,&item)，非空需resolver，成功item非NULL借用，可重复指针独立occurrence。失败透传，已解析items仍应用，先全解析至临时指针再未发布构建。

当时bulk O(N)：ceil(N/128)块，均匀，多块64..128/唯一1..128，独立record，沿块的前驱／后继链直接构建平衡 AVL parent/height/count；无逐项search/rotate/relabel。129→65/64，索引递归O(log blocks)，遍历/清理/parser迭代；失败毁部分结构不发布。

LksV3Lk1ImportEntry{key,item}的import非空key/item，严格原LK1解析、实际Path排序、重复ALREADY_EXISTS、bulk新Order；无序输入/不同坐标同item合法，零count可NULL entries，不保留Path/string。成本含解码/稳定O(N log N)非仅bulk；V3实现/golden不变。

## 示例、集成与证据

运行layerkeysort_snapshot_example，[snapshot.c](../examples/snapshot.c)捕获业务ID、打印LS1、序列化、改/毁source、读历史、load/resolve新Order，仅公开API。源码/离线FetchContent/安装C/C++/合并消费者测17新函数，头107=59V3+48临时V4。

layerkeysort_v4_snapshot测LS1/wire固定golden/损坏/截断、当前性、Managed无compare、回调/错误、marker/capacity、OOM、restore边界/100k、10kimport及10,001-step LK1含SIZE_MAX终端。smoke分离export频率/存储和mutation，见[说明](../benchmarks/results/v4-preview3-smoke/README.zh-CN.md)。证据依平台负载非实时/整体复杂度证明；存储O(N+A+K)含关联几何容量。

## 延后

当时Preview.4 Group/Batch/capture，Preview.5最终冻结。未实现增量export、DB/文件adapter、应用身份所有权、持久Handle、rank/select、容器锁、crypto、CRDT、最终全V4性能。快照导出历史序不使其免费，也不使ordinal成为永久身份。
