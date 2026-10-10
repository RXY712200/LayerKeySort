# 本地图层顺序工作台 — Phase 1

Phase 2 独立工具提供[性能/内存测量方法](docs/MEASUREMENT.md)，不改变交互程序或 Mini。
[人工体验指南](docs/MANUAL-EXPERIENCE.md)供真实操作者后续采集轨迹，自动化不计为用户采用证据。

独立 C17 终端应用，通过 Mini v1.0.0 **公共头文件和 CMake target** 管理图层
出现实例。支持交互、脚本、语义轨迹与确定性回放，每条命令后使用独立数组
Oracle 核验。它是消费者验证项目，不是 Mini 新版本或真实用户采用率证据。

[English README](README.md)｜[轨迹格式](docs/TRACE.md)｜[验证边界](docs/VALIDATION.md)

## 构建与运行

需要 CMake 3.21+、C17 编译器和对应构建工具；CTest 另需 Python 3.8+，仅使用
标准库，应用运行不需要 Python。不依赖 Full，不改变根目录默认构建。

```sh
cmake -S consumers/layer-order-workbench -B wb-build -DMINI_SOURCE=/绝对路径/mini -DCMAKE_BUILD_TYPE=Debug
cmake --build wb-build --config Debug
ctest --test-dir wb-build -C Debug --output-on-failure
```

`MINI_SOURCE` 可以指向仓库的 `mini/`，也可以指向从[正式发行附件](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0)
下载并校验后解压的 `LayerKeySort-Mini-v1.0.0`。独立目录中只需要工作台和 Mini
包，不需要 Full。GitHub 自动生成的全仓库 source archive 不是 Mini-only 包。

Windows MSVC 在 Developer PowerShell 中构建，运行
`wb-build/Debug/layer_order_workbench.exe`；MinGW 添加
`-G "MinGW Makefiles"`，运行 `wb-build/layer_order_workbench.exe`。
Unix 运行 `wb-build/layer_order_workbench`。Release 使用独立构建目录和
`-DCMAKE_BUILD_TYPE=Release`、`--config Release`。严格警告均视为错误。

## 命令与生命周期

- `create NAME`：创建业务对象，返回 object ID。
- `insert-front PAYLOAD`、`insert-back PAYLOAD`：插入，返回新的 occurrence ID。
- `insert-before PAYLOAD ANCHOR`、`insert-after PAYLOAD ANCHOR`：相对插入。
- `remove OCC`：删除实例，立即清除活动 Handle 映射。
- `move-front OCC`、`move-back OCC`、`move-before OCC ANCHOR`、`move-after OCC ANCHOR`：移动。
- `list`、`list-reverse`、`compare OCC OCC`、`size`、`help`、`quit`：查看、比较与退出。

`PAYLOAD` 为已经创建的业务对象 ID 或字面量 `NULL`；`OCC` 和 `ANCHOR` 为实例 ID。
同一对象可以重复插入，每个实例 ID 唯一，Handle 不作为业务身份输出。
删除后的 ID 被应用拒绝，不会访问已释放 Handle；重新插入分配新 ID。
对象由应用拥有，即使引用计数降为零也保留到退出；退出先销毁 Mini，再释放
应用对象存储，不会因删除一个重复实例而释放另一个实例仍引用的对象。

```text
create Ink
insert-back 1
insert-back 1
insert-before NULL 2
move-front 2
list
remove 1
move-back 1
quit
```

输入限制：每行至多 255 字节、4 个 token；名称为 1–47 个 ASCII 字母、数字、
下划线或连字符；最多 256 个对象、2,048 个存活实例、单进程累计 16,384 次成功
插入。这些是应用限制，不是 Mini 的容量保证。非法命令/参数/退休 ID 输出
`APP_*`；真实库分配失败输出 `MINI_OUT_OF_MEMORY`，两类错误不混淆。错误命令
继续会话，空行及首个非空白字符为 `#` 的注释行不计为命令。

## 轨迹与测试

```sh
wb-build/layer_order_workbench --commands consumers/layer-order-workbench/scenarios/layer-composition.commands --source script --trace /新的路径/session.trace
wb-build/layer_order_workbench --replay /新的路径/session.trace
```

输出轨迹路径必须不存在，防止覆盖历史证据或输入文件。轨迹包括版本、来源与
执行方式、命令序号、业务/实例/锚点 ID、操作前后数量、状态及完整预期顺序。
回放重新执行 Mini 并逐项匹配，损坏、截断、结果改变或额外数据均失败退出。
正常错误场景可以记录和回放，所以含错误恢复的脚本不一定返回非零；Oracle、
I/O 或回放不一致必须返回非零。

来源标签为 `script`、`generated`、`codex-interactive`、`human-terminal` 或
`unspecified`；终端标签必须确实使用终端 stdin，不允许给文件/管道冒充终端来源。
三个已保存场景是 Codex/CTest 执行的预编脚本，分别 32、31、30 条命令，不能当作
外部真实用户行为。16、128、2,048 实例场景是生成的工程测试数据。

Oracle 不读取 Mini 私有状态、不复制链表算法、不以实际遍历结果生成预期答案。
每步检查完整正反序列、首尾、数量、句柄和业务指针、引用计数、错误/no-op 状态。
小于等于 32 项完整两两比较；大规模固定抽样、端点和全部自比较，策略见英文说明。

Linux GNU 兼容链接器/MinGW 另有隔离 OOM 测试：仅测试可执行文件包装分配符号，
验证创建和四种插入失败/恢复。正式工作台不链接包装器，不修改 Mini 源码或公共
接口。MSVC/macOS 不运行不支持的链接器包装，仍运行其他测试。适用环境使用
`-DWORKBENCH_SANITIZERS=ON` 同时插桩 Mini 和消费者；实际平台 CI 结果见 Draft PR
的准确 SHA 运行记录，不能凭工作流文件宣称通过。

本阶段不测量性能优势、不实现 Full 适配层。真实下游轨迹、代表性统计计时和
分配器/RSS 内存证据留待后续决定；不会因这个工作台启动 Mini v1.1.0。
