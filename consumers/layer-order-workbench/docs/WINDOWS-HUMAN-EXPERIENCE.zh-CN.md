# Windows 11 真人操作与轨迹回放（Mini Workbench Phase 3）

本指南面向实际操作者，采用现有 **Local Layer Order Workbench**。这是交互式终端消费者测试，不是 Mini 库的新版本，也不是图形界面编辑器。三次人工会话属于尚待采集的使用证据；不能把先前 Codex 脚本、自动终端输入、重放或生成负载标记为真人操作。

## 1. 前置条件与构建

在 **Visual Studio 的 Developer PowerShell** 或具有可用 C17 编译器、CMake 和 Python 3.8+ 的 Windows PowerShell 终端运行。先进入仓库根目录（应能看到 `mini` 和 `consumers` 两个子目录）。

以下命令按顺序逐行执行；不用安装新的专用 IDE，也不需要构建 Full：

```powershell
Test-Path .\mini\CMakeLists.txt
Test-Path .\consumers\layer-order-workbench\CMakeLists.txt
$mini = (Resolve-Path .\mini).Path
cmake -S .\consumers\layer-order-workbench -B .\wb-human -DMINI_SOURCE="$mini" -DWORKBENCH_MEASUREMENTS=OFF -DWORKBENCH_TESTS=ON
cmake --build .\wb-human --config Debug
ctest --test-dir .\wb-human -C Debug --output-on-failure
```

前两个 `Test-Path` 都应输出 `True`。若 CMake 找不到编译器，请确认是在 Developer PowerShell 中启动；不要修改 Mini 源码或项目的 CMake 来绕过配置问题。

不同构建器的 EXE 路径可能不同，执行下面的 PowerShell 命令自动选择：

```powershell
$exe = if (Test-Path .\wb-human\Debug\layer_order_workbench.exe) {
    (Resolve-Path .\wb-human\Debug\layer_order_workbench.exe).Path
} else {
    (Resolve-Path .\wb-human\layer_order_workbench.exe).Path
}
& $exe --help
```

若希望用现有 MSYS2 UCRT64 GCC，请在 **Windows PowerShell** 中将已有 UCRT64 的 `bin` 目录加入当前进程的 `PATH`，确保 `gcc` 和 `mingw32-make` 可用，再用 `-G "MinGW Makefiles"` 配置到另一构建目录。该生成器使用 Windows 命令环境，不应切换到 MSYS/Bash 执行本页 PowerShell 命令；也不能复用 Visual Studio 构建缓存。无需另装 IDE 或修改源码。

## 2. 开始一场真实的人工交互

每场会话都在真实终端中运行程序，然后**由操作者亲自输入并选择命令**。不要用 `--commands`、标准输入重定向、自动填充脚本或 Codex 代输来充当人工会话。

```powershell
$traceDir = Join-Path (Get-Location).Path "human-session-traces"
New-Item -ItemType Directory -Force -Path $traceDir | Out-Null
$trace = Join-Path $traceDir ("human-1-{0}.trace" -f (Get-Date -Format "yyyyMMdd-HHmmss"))
& $exe --source human-terminal --trace $trace
```

程序应出现 `layer>` 提示符。可以先输入 `help`。每次交互选择一些实际的小图层编辑任务，包含新建对象、插入、移动、查看、比较、删除、重新插入和一个已退休 ID 的合法错误场景。

例如命令类型有 `create LayerA`、`insert-back 1`、`insert-back NULL`、`list`、`compare 1 2`、`move-front 2`、`remove 1`，但这些只是语法示例，不是一套供复制运行的“真人脚本”。业务对象 ID 与每次插入的新 occurrence ID 不是同一种编号：以程序实际输出为准；不要猜测已经删除的句柄。

三场会话分别聚焦：**初次建立图层顺序、重新排列已有图层、删除与恢复/重复对象/NULL/错误恢复**。每场至少有 **20 条被程序处理的命令**，包括最后的 `quit`；空行或注释行不计数。应由真人实际操作三次，不能将同一轨迹重复回放三次冒充独立会话。

三场合计应覆盖以下语义，由你自行选择操作顺序和程序实际返回的 ID，不要复制预编的 20 条命令脚本：

- 创建业务对象及前/后/相对插入；同一对象 ID 重复插入，另插入 `NULL` 占位实例。
- 实际交换相邻实例，再将实例跨越多个其他实例移动；用 `list`、`list-reverse` 和 `compare` 核对结果。
- 合法无操作移动，例如将首实例再次移到首部，或 `move-before OCC OCC`。
- 删除后用原 occurrence ID 请求移动，观察 `APP_RETIRED_OCCURRENCE`；再插入同一业务对象，使用新 occurrence ID 继续操作。错误后顺序应保持不变。

完成一场后输入 `quit`。正常结束应显示类似 `VERIFIED commands=25 ...`。这里的 `commands` 数量必须不少于 20，并记录是否出现应用错误或不符合预期的行为。若程序提前异常退出，应保留原始失败轨迹，不能修改尾标记假装完成。

**每场重新生成一个尚不存在的轨迹文件名。** `--trace` 拒绝覆盖旧文件，防止破坏证据。第二、三场分别把文件名前缀改为 `human-2-`、`human-3-`，再次启动程序。输入 `human-terminal` 仅是操作者声明，不构成身份鉴定。

## 3. 检查、回放和记录

每场都执行（`$trace` 指向那一场原始文件）：

```powershell
Get-Content -Path $trace -TotalCount 1
Get-Content -Path $trace -Tail 1
& $exe --replay $trace
```

应分别看到首行 `LLOW<TAB>1<TAB>human-terminal<TAB>stdin` 的实际制表符字段、末行 `LLOW-END<TAB>命令数`，以及回放的 `VERIFIED commands=...`。回放必须以退出码 0 结束；PowerShell 中可以随后用 `$LASTEXITCODE` 查看退出状态。回放的是同一轨迹，**不算另一场人工会话**。

请为每场保存简短说明：操作者是否确实为真人、日期、Windows/编译器、源码 commit（`git rev-parse HEAD`）、任务意图、实际命令数、回放结果、过程中发生的错误或不方便之处、是否为临时自选命令。若有私人名称或信息，分享前先检查内容并脱敏；未经本人许可不要公开上传原始人工轨迹。

## 4. 验收边界

- 3 场不同的真人终端操作，每场 20+ 条已处理命令且轨迹有完整 footer。
- 每场原始轨迹均能在相应源码版本的 Workbench 中成功回放。
- 错误、重复引用、NULL 和已退休 occurrence 等语义由现有独立数组 Oracle 校验。
- 不把自动脚本、合成测试数据、一次会话反复回放计作多个人工会话。
- 出现可复现异常时，先保存轨迹与运行环境，按 Workbench 问题和 Mini 公共契约问题分别审计；不要直接推断为 Mini v1.0.0 缺陷。
- 本阶段不要求开启新 Mini 版本、不要求 GUI、不进行 Full/Mini 性能排名。

相关资料：[消费者 README](../README.zh-CN.md) · [轨迹格式](TRACE.md) · [人工体验说明](MANUAL-EXPERIENCE.md) · [Mini Issue #7](https://github.com/RXY712200/LayerKeySort/issues/7)。
