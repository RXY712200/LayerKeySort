# LayerKeySort Mini

当前里程碑：**v1.0.0-preview.1**。Mini 是独立的 C17 顺序维护库，采用
非侵入式双向链表，适用于编辑器对象序列、任务队列和界面集合等本地可变顺序。
应用决定元素排列并拥有业务对象；Mini 只管理容器和出现节点。

本阶段提供 13 个函数：创建、销毁、大小、首尾查询、前后遍历、载荷查询、
四种插入及删除。移动和顺序比较属于后续 Preview.2，当前没有实现或声明。
这是预览基础阶段，尚非稳定版。

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
所有基本操作为 O(1)，销毁和完整遍历为 O(n)，每个实例需要一次分配。
同一容器的并发访问需要应用提供同步。

## 文档

- [公开契约](docs/CONTRACT.md)：API、错误、所有权与生命周期。
- [使用指南](docs/USAGE.md)：插入、删除与遍历示例。
- [开发指南](docs/DEVELOPMENT.md)：编译要求及验证流程。
- [架构](docs/ARCHITECTURE.md)：内部结构、不变量与限制。
- [变更记录](CHANGELOG.md)、[MIT 许可证](LICENSE)、[English](README.md)。
