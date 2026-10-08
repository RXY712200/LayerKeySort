# LayerKeySort Mini

目标版本：**v1.0.0**（Stable 候选，尚未发布）。Mini 是独立的 C17 顺序维护库，采用
非侵入式双向链表，适用于编辑器对象序列、任务队列和界面集合等本地可变顺序。
应用决定元素排列并拥有业务对象；Mini 只管理容器和出现节点。

本阶段完整提供 18 个函数：创建、销毁、大小、首尾查询、前后遍历、载荷查询、
四种插入、删除、四种移动及当前顺序比较。
当前为已准备的 Stable 候选；最终提交的验证记录见完成报告。
尚未发布 GitHub Release，合并和公开发布需要另行授权。

每次插入都产生独立的出现实例及稳定句柄，同一个业务指针可以多次插入，
NULL 载荷也有效。插入或删除其他实例不改变存活句柄。
删除自身实例或销毁容器后，句柄立即失效；调用者不得继续使用。
Mini 不释放、读取或修改业务对象，也不检测悬空指针。

## 独立构建

需要 C17 编译器、CMake 3.16 以上及对应构建工具。在本 README 所在目录运行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Windows MinGW 配置时增加 `-G "MinGW Makefiles"`，并确保编译器和
`mingw32-make` 在 PATH 中。示例位于单配置构建的 `build/lks_mini_basic.exe`
或 Visual Studio 构建的 `build/Debug/lks_mini_basic.exe`，输出两行：10、30。
也可以在父目录执行 `cmake -S mini -B <独立构建目录>`。
完整复制本目录到其他位置后，仍可独立构建，无第三方运行时依赖。

集成时包含 `layerkeysort_mini.h` 并链接 `layerkeysort_mini` 目标。
检查每个返回状态；错误时有效输出位置默认写入 NULL 或 0。
已知句柄的移动及基本操作为 O(1)，任意顺序比较、销毁和完整遍历为 O(n)。
移动和比较不分配或释放内存；每个插入实例需要一次分配。
同一容器的并发访问需要应用提供同步。

## 文档

- [公开契约](docs/CONTRACT.md)：API、错误、所有权与生命周期。
- [使用指南](docs/USAGE.md)：插入、删除与遍历示例。
- [开发指南](docs/DEVELOPMENT.md)：编译要求及验证流程。
- [架构](docs/ARCHITECTURE.md)：内部结构、不变量与限制。
- [变更记录](CHANGELOG.md)、[MIT 许可证](LICENSE)、[English](README.md)。

移动保持句柄地址、业务指针、所有权和大小不变。已在首尾的对应移动、
移动到自身前后、移动到紧邻后继之前或紧邻前驱之后，均成功且不改变链接。
所有必需句柄须先通过参数和所属容器验证。比较结果严格为 -1（在前）、
0（同一实例）或 +1（在后），不比较业务值；错误时输出为 0。

## 兼容性

v1.0.0 契约包含 18 个函数、三个公开类型和五个状态值。兼容的 1.x 更新
必须保留现有签名及文档约定的所有权、生命周期、错误和顺序行为。
私有结构保持不透明，其布局不属于 ABI 保证。悬空或伪造句柄仍不属于
有效调用契约。Mini 使用独立的版本历史。

Preview.3 新增独立数组参考模型，采用四个固定 xorshift32 种子，每个执行
6,000 次操作，覆盖三个独立容器，每个容器最多 48 个实例。默认测试包括
对抗序列、结构不变量、24 个单次分配失败及恢复案例、190 个合法指针错误矩阵案例。
已执行本地 Windows GCC 16.2.0 严格警告 Debug/Release、提取构建和生产静态分析。
本地 Windows ASan/UBSan 探测因缺少链接库失败；这不代表 Linux CI 的情况。
这些检查未发现生产缺陷；分配计数不能替代 sanitizer。
复现方法及覆盖边界见[开发指南](docs/DEVELOPMENT.md)。

RC.1 增加独立跨平台 CI、严格警告与 sanitizer 选项、提取项目验证及外部消费者。
可使用公开头文件与单个 C 源文件直接编译，或使用 CMake add_subdirectory
和 target_link_libraries 集成；准确命令见[开发指南](docs/DEVELOPMENT.md)。
性能与内存口径见[测量记录](docs/PERFORMANCE.md)。CI 必须按具体候选提交检查，
存在工作流文件不等于平台测试已经通过。

RC.1 候选 58f19a15 的五个 Mini CI 作业均已通过，包括实际插桩的 Linux
ASan/UBSan 全套测试；GCC、Clang、MSVC、AppleClang 的 Debug/Release
各通过 4/4。具体提交、版本及运行链接见[开发指南](docs/DEVELOPMENT.md)。
后续提交必须重新核对对应 SHA 的结果。
