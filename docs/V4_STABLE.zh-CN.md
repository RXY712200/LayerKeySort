# LayerKeySort V4.0.0 Stable 发布记录（历史译本）

原文：[V4_STABLE.md](V4_STABLE.md)，英文，固定于 [`c51e987aa1fda0b16bd61bae3f34ca1b6d091929`](https://github.com/RXY712200/LayerKeySort/blob/c51e987aa1fda0b16bd61bae3f34ca1b6d091929/docs/V4_STABLE.md)。原始记录不变，以下“推荐Stable”指4.0.0晋级时状态，并非替代当前4.1.0事实。[双语清单](design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md)。

## 决定与谱系

Stage6完成固定V4 Preview.1–5/RC.1周期。RC.1通过独立审核，当时无已知正确性/OOM/冻结格式发布阻断。Stable仅改版本和当前展示，生产源码/算法/测试/fixture/CI不变。

- 晋级前main：`c33afa2ac08248bffa6dde51bd590a5d799b394a`。
- Preview.5冻结基线：`f413382e96175b79c117956fe4a57212785fb104`。
- 起始RC.1：`9ad636fb2e292070505a86f59011d433296651b1`。
- [历史冻结](V4_FREEZE.md)、[RC验证](V4_RC1.md)保持不变。
- 最终revision见注释[v4.0.0标签](https://github.com/RXY712200/LayerKeySort/tree/v4.0.0)的peeled commit及[Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0)。候选和独立main CI成功后记录40字符SHA。提交内文件不能包含自身最终哈希，固定标签/Release而非移动分支是权威最终记录。

当时推荐V4.0.0 Stable；需Path/Tree的调用方仍可用历史V3.1.0。V4为大版本源码破坏，见[迁移](MIGRATION_V3_V4.md)。

## 冻结兼容

64公开函数/14类型，回调/字段/签名不变，无公开Path/Tree/OrderedTree/过渡immutable_group API。状态OK0、INVALID_ARGUMENT1、OUT_OF_MEMORY2、BUFFER_TOO_SMALL3、NOT_FOUND5、INTERNAL_ERROR7、ALREADY_EXISTS8、CAPACITY_LIMIT9、INVALIDATED10、REENTRANT11、DOMAIN_MISMATCH12，空洞4/6仍unknown。

LS1 v1保留七向量、小写namespace/ordinal hex、同域词典序及跨域拒绝。LKS4SNP1保留四二进制向量、flags0、显式大端、计数/溢出/尾拒绝；17条严格V3 LK1迁移向量/已发布语义不变，不重新解释格式。Win32/x64同fixture。4.x源码/布局兼容要求重建，不保证跨工具链/CRT/架构二进制ABI。普通库静态，私有归档符号不变公共承诺。

## 验证与发布条件

本地GCC16.2严格C17 Release（-Wall -Wextra -Wpedantic -Werror）、MSVC x64 Debug/带符号Release/ASan、Win32 Debug，各需39/39及七示例。本地Clang/GCC sanitizer运行库不可用，由精确SHA远端覆盖。默认优化MSVC exe消失未无据归因，带符号Release为已验配置。

修改oracle、managed、Group/Batch、parser torture、marker并发、OOM/failpoint、免分配断言不变。Preview.4聚焦回放timeline/churn/distant_drag/equal，原初始人数、seed7、10k操作，16次回放/16维护比较清理计数须匹配，无计时/调优。干净GCC/MSVC x64/Win32 C17/C++17消费者通过公开头引用全部64函数：add_subdirectory、离线FetchContent、install/export/find_package、LayerKeySort::layerkeysort、合并版；普通消费者不需Python。

精确v4-stable候选及快进main各需六配置：Windows x64/Win32、Ubuntu GCC/Clang/Clang ASan+UBSan、macOS AppleClang，各39测试、七示例、清单、干净消费者，GCC验资产。Clang sanitizer保留600k修改、长parser、100k远移、200k churn。只有green main可注释tag及非draft/非prerelease latest Stable。历史Preview/RC分支/tag/Release/v3.1.0保留。

## 源码资产

LayerKeySort-4.0.0-amalgamation.zip及LayerKeySort-4.0.0-SHA256SUMS.txt无二进制。ZIP仅layerkeysort.h、layerkeysort.c、example.c、LICENSE、README.txt。独立生成逐字节相同，规范ZIP元数据、成员、checksum及篡改/多文件拒绝；包示例编译运行，上传size/digest和下载字节匹配。最终size/SHA-256见Release。

## 成本与技术债

[Preview.4证据](V4_PREVIEW4.md)和[采集负载](../benchmarks/results/v4-preview4/README.zh-CN.md)为主要测量，不改历史计时。

| 分类 | 项目/边界 |
| --- | --- |
| 已解决/替代 | V3大型同步live坐标重标记架构 |
| 已解决/替代 | live Path/LK1增长为主表示 |
| 已解决/替代 | 不可变Group的Path/Tree开销 |
| 已解决/替代 | live坐标与持久耦合，改为显式快照 |
| 部分 | 结构操作界，无全库延迟定理 |
| 部分 | 分配器，逐resident分配/RSS保留 |
| 部分 | 桌面x86/x64已测，非全部平台 |
| 延后 | 真实用户trace、更强全库形式界 |
| 延后 | pool/slab实验、悬空Handle防御注册 |
| 延后 | 增量导出、持久live Handle |
| 延后 | 更多平台、Mini、嵌入式/MCU版本 |

私有B64/min32不变但非公开承诺。局部写、slack、平衡索引、回调/分配器/OS成本保留；选定表示的move结构工作不随逻辑距离，但不保证通用O(1)或整操作O(log N)延迟。全capture/export O(N+A)，历史/关联/Group scratch可主导内存。item/context寿命和可变访问同步由调用方；自身删除/来源销毁使Handle失效。无悬空探测、分布式/CRDT或持久Handle。本记录不安排新研究、V5或性能政策。
