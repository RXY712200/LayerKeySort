# Preview.2 复现与证据（历史译本）

原文：[REPRODUCTION.md](REPRODUCTION.md)，英文，固定版本 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.md)。原件保持不变；[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)提供成对导航。

从精确 Full 检出版本运行，需要 Python 3.12+、GCC C17、Git、CMake 和 Ninja。确保 run.py 列出的固定版本及冻结公开归档 `f41b7be182c2ef766ee0dec1211500f7dd762f54` 存在于本地对象库；按精确revision获取缺失SHA。随后执行：

```text
python -B benchmarks/research/full-v4.2.0-preview.2/run.py <new-output> fe00536e7bd7f4e5128432ea4a29e54158074903
python -B benchmarks/research/full-v4.2.0-preview.2/analyse.py <new-output>
```

省略第二个参数会暂存 wire.inc 所述仅源码wire原型；归档的 `Linux` 和 `Windows` 对应该筛选。`final-Linux` 和 `final-Windows` 使用精确生产二进制。托管筛选使用私有研究revision `a0c0dc9784d55e143d5aa9a4f406f2b993c58389` 和[运行45403456609](https://github.com/RXY712200/lks0011/actions/runs/45403456609)。精确候选及45条件实时回放使用 `f720828df2e9f7c12eab01289479d108174f18fd` 和[运行45404122363](https://github.com/RXY712200/lks0011/actions/runs/45404122363)。公开复现步骤不要求访问私有仓库。

run.py 暂存归档并以严格标志构建 Release 库，计时与诊断二进制分开。准入使用512 MiB可用内存加前瞻逐条件边界，超时120秒。原始gzip凭据保存准入、命令、输出及stderr。分析将记录与子进程输出核对，报告逐条件中位数/范围/IQR。早期final归档有两条名为 `sparse-diag-link` 的链接凭据，目标可执行文件不同；稀疏计时/诊断测量标明精确源码变体。元数据中重复的稀疏二进制键指向后来的候选稀疏程序；凭据保留两个显式构建路径。计时阶段总和不含驱动验证、外部缓冲区准备及部分阶段间销毁；它是统一的已观测生命周期边界，不是整个进程墙钟时间的估计。请求字节不包含fixture和调用方wire缓冲区。

补充QPC/RSS剖析需要以 `-DLKS_BUILD_BENCHMARKS=ON` 构建两个精确 Full 对照，然后调用 `scale_windows.py <baseline-build> <candidate-build> <new-output>`。四份固定轨迹使用独立种子和既有原生序列/身份/currentness预期。诊断样本报告请求内存和分配流量，RSS在原生Windows单独采样。百分位样本包含初始构建，阶段行区分构建和修改墙钟时间。

原生 OrdWeave 源码、复现命令、构建/测试结果、原始及最终凭据、大规模/故障保留样本位于[研究归档](https://github.com/RXY712200/OrdWeave/tree/1e40955a783f91ec835170bea664bfa8e324b465/benchmarks/research/full-preview2-20261124)。先前生成fixture与原生归档保留在原始提交。当前应用轨迹均不代表观测到的人类操作。

本地核心验证使用严格GCC Debug和Release，配置 `-DLKS_BUILD_BENCHMARKS=ON`，运行 `cmake --build <build> --parallel 3`、`ctest --test-dir <build> --output-on-failure`。从根目录用Python运行 tools/verify_v4_api.py、tools/validate_distribution.py、tools/validate_release_assets.py。verification保存真实本地日志和筛选前的严格编译失败：原型专用helper在Full适配器中编译后未使用，已在计时前修正。
