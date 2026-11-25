# Preview.1 最终比较复现（历史译本）

原文：[REPRODUCTION.md](REPRODUCTION.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.md)。保留原始证据；[双语导航](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

从含本目录的Full检出运行，需要Windows或原生Linux、Python 3.12+、GCC、CMake、Ninja、Git、Rust/Cargo；Windows的C使用MinGW。托管采集直接解析三个Full revision，j-hui git依赖锁在 `ac4b902d103beb30cee3316d20cd10701f52d15a`。公开Full研究归档含其源码/MIT许可及C基线源码/notice。registry依赖需要联网。

    python -B benchmarks/research/full-v4.2.0-preview.1/replay/replay.py <new-workspace>

回放包装器暂存已发布Full归档并检查各轨迹摘要，检出两个已发布源码及生产候选，构建七个Release适配器。输出写入指定空路径，原始测量harness与分析分开保存。托管workflow运行同一collector和CMake方法、原始输入准备。Windows C用QPC，两平台Rust用Instant。首次Linux采集用归档C墙钟；单独修正Linux运行用CLOCK_MONOTONIC，是主要Linux结果。公开包装器仅对暂存Linux C harness作同一时钟替换。每轨迹20,000操作，时间覆盖同一回放范围及分派/检查，排除初始化和回放后验证。

collector轮换七实现、子进程串行，诊断请求与生产计时分开，正式分析丢弃repeat zero，保留stdout/stderr，执行原512 MiB可用内存保护和120秒子进程超时。请求存储排除共用harness缓冲区；该harness无Linux进程内存及Rust RSS字段。

重算采集比较：

    python -B benchmarks/research/full-v4.2.0-preview.1/summarize_matrix.py

同语言微对照将compare_screen.c分别链接v4.1.0及候选生产归档，GCC C17 -O3 -DNDEBUG，使用Full公开/私有include。运行顺序Stable、candidate、candidate、Stable，原始对照保存头与采集，各丢弃repeat zero。内部祖先循环可内联，属诊断比较；两公开路径为匹配外部调用。ordinal探针仅固定状态下限，不含动态维护。跨语言排名包含原生实现及包装差异。

公开包装器--c-smoke从Windows空暂存路径实际执行，获取并构建三个固定Full源码，通过五个独立C语义fixture。Rust适配器以--locked构建，在两个托管job通过语义fixture。完整含Rust公开包装器未在本地执行。
