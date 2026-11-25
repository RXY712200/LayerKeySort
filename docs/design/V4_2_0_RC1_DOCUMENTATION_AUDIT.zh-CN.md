# Full RC.1 源码与文档重建

[English](V4_2_0_RC1_DOCUMENTATION_AUDIT.md)

| 角色 | 准确版本 |
| --- | --- |
| 已发布 Stable 源码 | 70d7f0ef5857b54381b863d292a213909d87459f |
| 独立接受的 Stable 指南 | 1a175f91e4c338c2afe6a73a9f8f9c5b149768e5 |
| Preview.3 临时指南 | 168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0 |
| 运行源码 | fe00536e7bd7f4e5128432ea4a29e54158074903 |
| RC 元数据及验证检出点 | d1132a26ca7b2c65bfd512fa2eb29f1b16793c08 |

参见[完整 Stable 到 RC 差异](https://github.com/RXY712200/LayerKeySort/compare/70d7f0ef5857b54381b863d292a213909d87459f...d1132a26ca7b2c65bfd512fa2eb29f1b16793c08)、[公开声明](V4_2_0_RC1_API_INDEX.zh-CN.md)及[Preview.3 清单及 A–F 谱系](../../benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md)。矩阵现行路径均针对验证检出点，其他实验分支不定义行为。

## 源码演进及指南影响

| 改动 | 新增或替换 | 同步及检查 |
| --- | --- | --- |
| 22544cf283694dd27cef203caba9cf1838845947、6d4099d | 父节点/高度取代秩比较，普通位移更新受影响后缀 | 导读 §§2–4；内部 §§2–7；v4_mutation 拓扑及失败参考 |
| 61e9290ec5ba47b9092bcb8ae7d61f13104ea839、21f6fc4 | 非零快路、有界再分配、零回退、批间隔 | 内部 §§2–6,11；导读 §§2–3；集中插入、耗尽、全顺序和符号 |
| c977b0fd14a501d769b9783bfdcc61266bd1b5ae | 诊断跨零保留最后非零，保留 B64 | 内部 §§3,14–15；验证 §§3–4；拒绝相同或下降标签，B32/B128/转移否决证据仍在 |
| fe00536e7bd7f4e5128432ea4a29e54158074903、92fb7fad977f792a7d9e3e0e430a5d5f32e8914a | 固定 4/8 字节访问及同版本独立快照 | 导读 §§5–6；API §§B,D,E；内部 §§9–11；验证 §§3–4。冻结字节不变 |
| 754b3088c160564f6666ee520284168ee5da6ac7、3ac82d26301ea12876a3ebdba6319fe8233621f3 | 偏移一字节读写及恢复混合标签回退 | 验证 §§1,3,4,7；导读 §§6,13。两项均在 v4_snapshot.c 及 RC 检出点 |
| 8e85584、4b66ab0、168debf | Preview.3 证据、章节同步、测试路径和计时修正 | 分开应用延迟、进程墙钟和阶段和；导读和验证用含测试检出点 |

## 模块义务

| 源码 | 契约、内部不变量及指南 | 证据与边界 |
| --- | --- | --- |
| order.c、order_bulk.c、lks_order_internal.h | 导读 §§2–3、内部 §§2–6,11；单独分配并保持实例、B64/最小 32、AVL 高度和链序，移动释放原位置前准备备用块 | v4_order、v4_mutation 顺序身份、有界探测、零回退、版本和 OOM；无任意恢复保证 |
| managed_order.c | 导读 §4、内部 §7；复制比较描述符，借用上下文，稳定上界，拒绝同源重入 | v4_order/v4_mutation 的参考、重入及删除/修改/重插；应用须维持一致比较 |
| snapshot.c、lks_snapshot_internal.h | 导读 §5、API §§B,D,E、内部 §§8–9；累计结束位置含空行，标记独立于来源地址，发布前取得引用，回调字节仅借用到立即复制 | v4_snapshot、v4_marker_threads；来源销毁、版本与同版本独立性；不可变视图并发不推出实时修改同步 |
| snapshot_key.c、snapshot_wire.c、snapshot_restore.c | 导读 §§6,8,11、内部 §§10–11；域内序号、先完整解析再分配、先 Resolver 后恢复及新 Handle、严格 V3 导入 | 固定向量、畸形/截断、偏移访问、旧 LK1、混合标签和已达到的 OOM；无持久提交或回调回滚 |
| immutable_group.c、lks_immutable_group_internal.h、sort.c、lks_sort_internal.h | 导读 §7、内部 §12；稳定左优先、借用对象、拥有数组、无部分输出 | v4_immutable_group 与 API 消费者；比较状态仍在应用 |
| path.c、path_text.c、path_order_key.c、path_compare.c、slot_codec.c 及相关头 | 导读 §8、内部 §13；私有旧解码、受检算术和键语义 | LK1 固定向量与对抗解析；旧树、间隙、可变组、批建及私有策略头仅供历史回归 |
| lks_base.c、lks_alloc.c、lks_alloc_internal.h | 导读 §9、内部 §14；冻结状态、受检分配统计及所有权 | 公开清单及诊断注入；请求字节不含分配器内部 |
| include/layerkeysort.h、cmake/ProductionSources.cmake、CMakeLists.txt、tools/amalgamate.py 及验证工具 | API §§A–F、验证 §§2,4–5；64 函数、14 类型、16 生产模块、冻结向量、源码/安装/合并的 C/C++ 接口 | API 清单、CTest、分发消费者及资产包装验证；Mini 和 Workbench 独立 |

## 四指南同步

| 指南 | 当前章节变化 | 保留及边界 |
| --- | --- | --- |
| 导读 | §§3–4 准备与发布、比较器；§11 恢复；§13 验证检出点 | §§7–9 稳定排序和 V3 导入保留；应用负责事务 |
| API | §A 冻结签名和定义索引、比较校验顺序；§F 视图和所有者 | 保留 64 条目、类型及状态；无新 ABI 或契约 |
| 内部 | §§3,6,11 旧秩移除、算术、备用及批建 | §§8–10 紧凑行与固定宽度线格式保持；布局和分配数据有平台限制 |
| 验证 | §§1,3,7 确切检出、断言、故障定位 | §§4–6 向量及消费者检查保持；归档计时与 RC CI 分开 |

正式运行及边界见[候选记录](../V4_2_0_RC1.zh-CN.md)。保留完整 [FULL-RI 处置](../V4_2_0_PREVIEW3.md#technical-debt-impact)。

## 修复追踪

| 实现义务 |引入版本 | 当前说明 | 实际证据及边界 |
| --- | --- | --- | --- |
| 比较分派与回退符号 | 22544cf2、61e9290e | 内部 §3 说明同块下标、标签、祖先和同父符号、等高方向，以及未使用的子树计数 | 源码 `lks_order_compare`、v4_mutation 的 `preview1_precedence_and_shifts` 及恢复混合标签。无最坏常数时间承诺。 |
| 有界赋值及计数 | 61e9290e；耗尽 21f6fc4；单调诊断 c977b0fd | 内部 §3 明确两哨兵、步长与最小值、确定扩张、宽度上限及计数范围 | `v4_tag_attach/v4_check_node` 及 `preview12_block_tags`。中点耗尽、B32/B128、直接转移为分别否决的方案。无任意恢复保证。 |
| 批建间隔 | 61e9290e | 内部 §11 给出公式和先解析顺序 | `src/order_bulk.c`；3ac82d26 的 `capture_restore` 验证恢复遍历、标签及两种比较路径。新 Handle 不保留旧地址。 |
| 验证出处 | d1132a26；初次候选 643d8f79 | 验证 §§1–5 与导读 §13 区分运行源码、测试检出点及实际运行 | 初次 RC CTest 和 sanitizer [45518848531](https://github.com/RXY712200/LayerKeySort/actions/runs/45518848531)。后续 HEAD 需自身证据。 |
