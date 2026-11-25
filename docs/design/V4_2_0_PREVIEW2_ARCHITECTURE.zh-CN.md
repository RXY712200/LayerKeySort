# Full Preview.2 架构决定（历史译本）

原文：[V4_2_0_PREVIEW2_ARCHITECTURE.md](V4_2_0_PREVIEW2_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.md)。原始历史记录保留；[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

## 运行源码与兼容

已接受Preview.1.2 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` 提供B64、后缀更新、有界64位标签/AVL回退。本扩展保持这些文件及 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae` 的单调性诊断。实际实现修改 `fe00536e7bd7f4e5128432ea4a29e54158074903` 限snapshot_wire.c及候选版本元数据；`92fb7fad977f792a7d9e3e0e430a5d5f32e8914a` 另测同revision不可变独立性。

## Step 3：完整生命周期

Capture按O(N)遍历Explicit/Managed Order或Group，每occurrence调用一次关联回调，复制namespace并在前进前复制借用关联。累计end表示空/非空行；namespace、头、rows共分配，关联单独增长并在发布前裁剪。调用方可复用scratch，随后销毁/修改来源。成功字节捕获后才分配/保留marker；同revision只说明序列当前性，不保证相同载荷/namespace，因此不可仅按revision共享捕获。

LKS4SNP1写既有magic、保留字段、namespace长/row数、namespace及64位长+字节各行。Reader先完整框架/计数/溢出扫描，再分配未发布Snapshot及精确关联buffer并复制。Restore分配N指针，按ordinal解析，全部成功才bulk平衡建造。保留旧scratch和复制；交错解析/分配会改变OOM时可观察回调前缀。Capture/encode/decode为O(N+namespace+关联字节)，restore另含一次live bulk及O(N)指针scratch，不改快照所有权或成功恢复Handle。

固定宽度原型/采用实现用无符号32位移位/OR读字段，八字节合成64位；写对应无符号移位至单字节。调用宽度只四/八，读取前已有范围边界。支持未对齐，无本机表示/字节序依赖，保持两遍验证、分配/失败顺序、resolver顺序及精确输出，不增分配/字段。每验证pass仍查所有长度并拒绝尾随字节；旧golden/畸形/故障fixture不变。

独立对照为逐occurrence分配链序、扁平复制快照，测试共同成功wire/ordinal/resolve和独立字节/ordinal预期。特定负载含端点重排、重复捕获和全行key/编解码/恢复。List无任意precedence索引、managed比较器、来源marker或Full式可恢复分配/畸形策略；其decoder按可信输入长先分配再扫描，resolve时逐节点恢复。差异应表述为被测组合/保证的成本。更宽保留和可变载荷条件约束固定解码可能收益；无依据做去重/cache/共享/restore重排。

## Step 4：OrdWeave

应用 `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` 拥有持久ID、类型字段、集合成员、事务发布、WAL持久与恢复。适配器用Full管理live相对顺序/managed查找。TD-001插/删在发布前构建替代全局occurrence索引，属于OrdWeave O(N log N)构建和O(N)临时resident分配，含真实Full managed插入、遍历、分配和应用比较器，区别于一次Full集合编辑。发布后销毁旧索引并回填live索引Handle；仅替换依赖不会改变该策略。

结构化采集分stage、publish、选定集合adapter调用、WAL commit；主flush嵌在commit。精确条件checkpoint/reopen/历史重建为独立driver阶段。剩余循环含区间外查找、比较器/layer成本、检查和未插桩工作。单Order契约不能推导通用原地多结构事务。Full替换不改adapter API、应用源码、恢复保证或持久格式。本包不选替代修改策略/公开API，保留validated-WAL回放及I/O语义。

## Step 5：规模

两种百万收缩至百分之一存活形状以两次收缩/再增长检查回收及Handle地址。保留快照按实际复制字节/诊断单测。收缩后稀疏存储应随剩余resident和占用，而不是pool历史高水位。原no-pool决定及容量研究保留。聚集/散布删除中的重复存活者主要测试块/resident回收。RSS区别于请求live/peak，采样生命周期不约束未来分配器复用、碎片或标签健康。

[阶段报告](../V4_2_0_PREVIEW2.md)提供证据/限制，[前瞻协议](../../benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.zh-CN.md)定义门槛和范围。
