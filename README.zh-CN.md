# LayerKeySort

[English](README.md) | 简体中文

C17 动态集合排序：实时位置使用容器内句柄，历史与持久化顺序使用不可变快照键。
当前为 v4.0.0-preview.5，最后一个 V4 Preview；不是 RC 或 Stable。
正常使用仍推荐 v3.1.0。下一阶段是 Release Candidate，没有 Preview.6。

LksOrder 用于相对排序/移动，LksManagedOrder 用于比较器排序，LksGroup/Batch
用于不可变平面序列，LksSnapshot 提供 LS1/LKS4SNP1 持久化。
V4 主头文件移除 V3 Path/Tree API；业务身份仍由应用维护，句柄不可持久化。
完整导出 O(N+A)，保留快照增加内存，不承诺所有操作的硬延迟上界。

见 [英文快速开始](README.md)、[冻结契约](docs/V4_FREEZE.md)、
[迁移指南](docs/MIGRATION_V3_V4.md)、[七个公开示例](examples/README.md)。
