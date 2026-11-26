# V4 Preview.3 导出冒烟，2026-10-06（历史译本）

原文：[README.md](README.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/results/v4-preview3-smoke/README.md)。原件/测量不变；[双语导航](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

源码为基于冻结Preview.2提交 `2967cc6429c6af7a0c5a8d9fabdeb31ed2bf0642` 的Preview.3快照实现；随附发布提交包含harness和这些行。AMD Ryzen 9 9955HX（16核/32逻辑处理器）、Windows x64、GCC 16.2.0 UCRT64、CMake Release、严格C17警告视为错误。每程序仅一次进程/运行，无重复中位或外部比较；属于工程冒烟，不是V4最终性能验收。

启用benchmarks复现：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON
cmake --build build
./build/layerkeysort_v4_snapshot_smoke
./build/layerkeysort_v4_snapshot_diagnostics
```

production.csv为非插桩计时；allocation/resident-delta为NA而非零，生产没有分配计数。diagnostics.csv为请求分配及存活字节；耗时含插桩，不作生产时间。两者均未测RSS、分配头、碎片或应用峰值内存。

## 条件与字段解释

捕获行采用CSV头。N为10k/100k/1M，namespace为16个零字节，关联载荷0/16/64字节来自同一个持续存活scratch span。已知有序输入由私有bulk builder建立，以分离导出和live插入；每次捕获仍遍历全部行并复制载荷。分别计时捕获、格式化全部N键、序列化、反序列化、恢复（resolver映射到同一重复调用方指针）。键循环复用输出buffer，blob分配在serialize计时外。

诊断捕获分配包括namespace、row元数据、几何增长字节buffer、Snapshot对象及首次惰性marker。resident delta包含Snapshot和新共享marker，不含既存来源记录。row元数据是两个size_t（该环境16字节）。关联逻辑字节与容量分开，namespace只存一次。all_key_bytes是假想物化全部键、不含NUL，并非实际Snapshot存储。百万行0/16/64字节载荷的delta为16,000,096 / 32,777,312 / 83,108,960字节；wire为8,000,040 / 24,000,040 / 72,000,040字节；物化键为53,000,000字节。

频率行采用独立schema：`frequency,mutations,cadence,export_count,total_ms,capture_and_destroy_ms`。20k append、空关联，每10000/1000/100次修改捕获/销毁，或从不捕获。集合增长导致累计导出行随cadence不同，展示显式成本位置。生产总耗时1.3344 / 1.3214 / 1.8027 / 5.2702ms。小逆转是计时噪声；200次导出比2次拷贝更多。Snapshot/export为O(N+A)，并非免费地把工作隐藏到修改中。

导入行 `import,N,elapsed_ms`：10k/100k无序LK1，level N..1，严格解码/排序/bulk build含临时Path清理。生产2.2324 / 27.8017ms包含O(N log N)排序，仅最终bulk为O(N)。Restore先解析所有occurrence，再直接建块与平衡索引，没有逐项AVL搜索/旋转。

1M生产capture为16.1759 / 19.8548 / 34.0987ms（0/16/64字节），restore为37.6800 / 35.3811 / 32.4922ms。单机数据不是实时、上界、通用速度或并发保证。

CTest以smoke参数运行同一harness（512行、1000修改、512/1024导入）检测功能回归，无计时门槛。历史Preview.1/2采集不变。Preview.4保留最终Group/Batch范围，全V4最终性能验收仍分开。
