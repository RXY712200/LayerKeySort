# 复现与凭据（历史译本）

原文：[REPRODUCTION.md](REPRODUCTION.md)，英文，固定版本 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.md)。保留原件；[双语清单](../../../docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)提供成对导航。

从Full仓库根目录运行，需要Python 3.12+、Git、GCC、CMake和Ninja：

```text
python -B benchmarks/research/full-v4.2.0-preview.1.2.1/screen.py <new-output-dir>
python -B benchmarks/research/full-v4.2.0-preview.1.2.1/analyse.py <new-output-dir>
```

归档运行使用 `6883b96b3bb6c58462fc2b5a4cccdcf2a4d5e933` 的筛选和probe、精确Preview.1.2源码 `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`、公开冻结轨迹及独立回放归档 `f41b7be182c2ef766ee0dec1211500f7dd762f54`。浅克隆需先显式获取这些对象。程序验证全部45份冻结输入哈希并运行15个种子17条件，归档精确命令、输入/二进制哈希、平台/编译器、原始stdout/stderr和进程状态。使用C17优化生产产物、单独诊断、逐子进程512 MiB准入及120秒超时。除单独支持的Windows 工作集字段外，占用字段均为请求字节。回放采用QPC或Linux CLOCK_MONOTONIC。诊断计数与逐操作时钟是独立于决策计时的观察。

私有隔离托管revision `5c5540406af5a465ceaf12f8ea3b410f5566d81c` 在[Actions运行45397684899](https://github.com/RXY712200/lks0011/actions/runs/45397684899)中运行两台主机。公开记录可在无该仓库访问权时复现。hosted-windows、hosted-linux、local-windows各含60个条件的540行回放、746条总命令凭据，以及可重算的中位数、极值、按包含端点法计算的四分位距（IQR）和内存比率。对任意捕获目录（含gzip凭据归档）运行analyse.py。精确经典OM固定与先前比较结果保存在[Preview.1.2归档](../full-v4.2.0-preview.1.2/REPRODUCTION.md)。本地筛选阻止了晋级，因此无需九适配器实验。

新增批建留出验证样本：为B64、transfer/B64、B32、B128分别将bulk_shapes.c链接至暂存诊断归档。使用GCC标志 `-std=c17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -DLKS_ENABLE_ALLOC_DIAGNOSTICS -DLKS_ORDER_BLOCK_CAPACITY=<capacity>`，以及各暂存源码自己的include/src目录。回放和留出验证样本使用各变体头，避免块布局不匹配。诊断批建构造器也用于Snapshot恢复；完整持久化解码/恢复测试位于核心套件。

精确分支检查：严格GCC Debug和Release，配置 `-DLKS_BUILD_BENCHMARKS=ON`，运行 `ctest --test-dir <build> --output-on-failure`。运行 `python -B tools/verify_v4_api.py`、`python -B tools/validate_distribution.py`、`python -B tools/validate_release_assets.py`。verification保存运行日志、三个原型的原生修改模型日志及生产预处理一致性检查。OrdWeave使用 `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` 的隔离源码归档，在third_party/LayerKeySort替换Full为 `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`，运行原生Ninja C17 Release和CTest。上游gitlink仍为 `70d7f0ef5857b54381b863d292a213909d87459f`。
