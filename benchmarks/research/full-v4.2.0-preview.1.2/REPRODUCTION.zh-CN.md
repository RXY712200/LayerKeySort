# 复现与证据索引（历史译本）

原文：[REPRODUCTION.md](REPRODUCTION.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.md)。原始记录保留；[双语导航](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

主要采集运行于隔离研究分支research/preview1-2-matrix，revision `638535b76e5609104723b8c80b5b190bf06332a6`、workflow运行45387318084。公开回放采用公开许可输入，无需私有研究库访问权。

从Full根目录在原生Windows/Linux运行 `python -B benchmarks/research/full-v4.2.0-preview.1.2/replay/replay.py <new-output-dir>`，需要Python 3.11+、Git、GCC、CMake、Ninja及Rust/Cargo。`--c-smoke`无需Rust，检查六个C适配器构建和语义fixture。源码使用脚本中四个精确固定的 Full 提交。不可变归档 `f41b7be182c2ef766ee0dec1211500f7dd762f54` 提供45份SHA-256检查轨迹、C harness/对照、固定j-hui源码及MIT notice `ac4b902d103beb30cee3316d20cd10701f52d15a`、Cargo lockfile。Release采用C17或Cargo锁定release profile，诊断分开；各C构建目录生成编译命令。采集保存时间戳、环境、可执行SHA-256，原生程序分别用QPC、Linux单调时钟和Rust Instant。

final-matrix/{Linux,Windows}保存元数据、原始行、逐调用凭据、压缩stdout/stderr、重算条件。`archive_matrix.py <capture-parent>`在建新归档前检查成员、子进程状态、内存准入、独立参考模型校验完成、分配清理和原始到行的核对。summarize_matrix.py计算七进程中位数、极值、按包含端点法计算的四分位距（IQR）、诊断存储、候选/对照比。比率对应同一主机的同一冻结轨迹，越低越利于候选。Windows工作集不同于分配器请求表示字节；Linux不支持的工作集观察缺省。

初始筛选：`python -B <research-dir>/stage.py <new-dir> control`，然后tags和no-count使用不同目录。各树Ninja Release、严格GCC C17构建，再运行 `screen.py --control <control-dir> --tags <tags-dir> --no-count <no-count-dir> <new-output-dir>`，使用各暂存目录中的build。保留移除count的失败。计时消融只构建库目标、不构建测试。screen-metadata.json、screen-raw.csv、screen-invocations.jsonl.gz保存实际进程输入/输出。小型本地筛选施加子进程超时；托管准入保护用于最终实验。

公开compare微探针将既有Preview.1 compare_screen.c以各精确C库自己的私有/公开头、GCC `-std=c17 -O3 -DNDEBUG` 链接对应生产归档。四次串行顺序Preview.1、candidate、candidate、Preview.1；排除编号为零的预热重复。单独归档操作profile用既有layerkeysort_v4_trace及诊断版，JSON每行嵌入命令、二进制哈希及stdout。本地原始凭据旧标签stable41实际上指preview12-control中的已接受Preview.1 `22544cf283694dd27cef203caba9cf1838845947`，其头为4.2.0-preview.1，不是已发布v4.1.0。托管矩阵对两者使用明确不同固定。本地补充profile采集时开发主机也在运行其他验证，因此用于筛查大回归。

Windows日志涵盖四个独立范围：核心CTest、分发、资产、隔离OrdWeave。核心分别建Debug/Release，配置 `-DLKS_BUILD_BENCHMARKS=ON -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"`，构建并运行 `ctest --test-dir <build-dir> --output-on-failure`。根目录运行 `python -B tools/verify_v4_api.py`、`python -B tools/validate_distribution.py`、`python -B tools/validate_release_assets.py`。下游在隔离目录解压固定OrdWeave及候选，在OrdWeave外部Full源码位置替换候选，原生Windows Ninja Release配置、构建并运行十二项原生测试。上游依赖固定保持不变。
