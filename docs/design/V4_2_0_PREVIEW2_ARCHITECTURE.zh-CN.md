# Full Preview.2 架构决定（历史译本）

原文：[V4_2_0_PREVIEW2_ARCHITECTURE.md](V4_2_0_PREVIEW2_ARCHITECTURE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.md)。原始历史记录保留；[双语清单](V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

## 运行源码与兼容

已接受Preview.1.2 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` 提供B64、后缀更新、有界64位标签/AVL回退。本扩展保持这些文件及 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae` 的单调性诊断。实际实现修改 `fe00536e7bd7f4e5128432ea4a29e54158074903` 限snapshot_wire.c及候选版本元数据；`92fb7fad977f792a7d9e3e0e430a5d5f32e8914a` 另测同revision不可变独立性。

## Step 3：完整生命周期

捕获过程以 O(N) 遍历显式顺序、托管顺序或不可变 Group，每个出现项调用一次关联数据回调。它复制命名空间，并在进入下一项之前复制当前项借出的关联数据。各行的累计结束偏移量可表示空行和非空行。快照头、命名空间及行索引共用一次分配，关联数据则另行扩容，并在发布前裁剪。调用方可以复用回调的临时缓冲区，之后也可销毁或修改来源。字节捕获成功后才分配或保留来源标记。同一来源的相同修订号只说明序列仍与来源一致，不能据此判定关联载荷或命名空间相同，也就不能仅凭修订号共享捕获结果。

LKS4SNP1写既有magic、保留字段、namespace长/row数、namespace及64位长+字节各行。Reader先完整框架/计数/溢出扫描，再分配未发布Snapshot及精确关联buffer并复制。恢复时分配一个含 N 个指针的临时数组，按序号调用解析回调，全部成功后才批量建立平衡的实时顺序结构。原有临时数组及复制语义保持不变；如果把解析与批建分配交织起来，内存分配失败时，调用方看到的已执行回调前缀可能不同。捕获、编码和解码的代价为 O(N + 命名空间字节数 + 关联数据字节数)。恢复包含一次实时结构的批建和 O(N) 的临时指针存储。临时存储不改变快照所有权或成功恢复后输出的句柄。

固定宽度原型/采用实现用无符号32位移位/OR读字段，八字节合成64位；写对应无符号移位至单字节。调用宽度只四/八，读取前已有范围边界。支持未对齐，无本机表示/字节序依赖，保持两遍验证、分配/失败顺序、resolver顺序及精确输出，不增分配/字段。解码先校验所有长度及输入末尾，拒绝尾随字节；第二遍才填写行结束偏移量并复制关联数据。保留原有黄金字节、畸形输入和故障注入用例。

独立对照为逐occurrence分配链序、扁平复制快照，测试共同成功wire/ordinal/resolve和独立字节/ordinal预期。特定负载含端点重排、重复捕获和全行key/编解码/恢复。List无任意precedence索引、托管顺序的比较器不变量、来源标记或Full式可恢复分配/畸形策略；其decoder按可信输入长先分配再扫描，resolve时逐节点恢复。差异应表述为被测组合/保证的成本。更宽保留和可变载荷条件约束固定解码可能收益；无依据做去重/cache/共享/restore重排。

## Step 4：OrdWeave

应用 `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` 拥有持久ID、类型字段、集合成员、事务发布、WAL持久与恢复。适配器用Full管理live相对顺序/managed查找。TD-001插/删在发布前构建替代全局occurrence索引，属于OrdWeave O(N log N)构建和O(N)临时resident分配，含真实Full managed插入、遍历、分配和应用比较器，区别于一次Full集合编辑。发布后销毁旧索引并回填live索引Handle；仅替换依赖不会改变该策略。

结构化采集分stage、publish、选定集合adapter调用、WAL commit；主flush嵌在commit。精确条件checkpoint/reopen/历史重建为独立driver阶段。剩余循环含区间外查找、比较器/layer成本、检查和未插桩工作。单Order契约不能推导通用原地多结构事务。Full替换不改adapter API、应用源码、恢复保证或持久格式。本包不选替代修改策略/公开API，保留validated-WAL回放及I/O语义。

## Step 5：规模

两种移除分布均从一百万项收缩至百分之一，经过两次收缩和重新增长，检查内存回收和存活项的句柄地址。保留快照的成本单独测量，以实际复制字节和诊断计数为依据。收缩后稀疏存储应随剩余resident和占用，而不是pool历史高水位。原no-pool决定及容量研究保留。聚集/散布删除中的重复存活者主要测试块/resident回收。RSS区别于请求live/peak，采样生命周期不约束未来分配器复用、碎片或标签健康。

[阶段报告](../V4_2_0_PREVIEW2.zh-CN.md)提供证据/限制，[前瞻协议](../../benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.zh-CN.md)定义门槛和范围。
