# Preview.3 前瞻实验协议（历史译本）

原文：[PROTOCOL.md](PROTOCOL.md)，英文，固定版本 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.md)。保留原文；此译本解释历史实验，不扩展当前执行授权。[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)提供两种语言的导航。

使用完全不变的 Preview.2 运行源码 `fe00536e7bd7f4e5128432ea4a29e54158074903`。在原生 Windows/Linux 上，对45份不可变回放轨迹与 v4.0.0、v4.1.0、已接受的 Preview.1.2、List、Treap，以及锁定版本的原生 tag/list-range OM 进行比较。每项包括预热、七次串行正式样本和诊断运行，轮换执行顺序，使用512 MiB准入保护和120s子进程超时。在各主机内部按条件比较中位数，报告离散程度、不利条件及非目标工作负载中反复出现的超过10%减速。保留全部原始凭据。

复用13个生命周期条件，然后增加任意相对位置的混合编辑及索引化历史，使用种子1779033703和17、1k/10k/100k的独立状态。比较共同成功行为、精确编码、关联字节/ordinal预期、来源销毁后的不可变历史及新建恢复。单独报告 Full 更强的保证。保留包含各条件估计值的内存保护。原生 OrdWeave：原始27条轨迹、两种模式、三个精确依赖；为约束I/O成本，使用预热及三个正式进程；单独执行剖析、规模为 4096 的留出验证及故障扫描。保留先前源码和全部负面结果。不自动替换生产实现，不进入RC。
