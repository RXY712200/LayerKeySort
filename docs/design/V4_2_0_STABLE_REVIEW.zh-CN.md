# Stable 准备：文档审核台账

[English](V4_2_0_STABLE_REVIEW.md)

源码基线：`d7f3bdfe387ed80d2c3cff3b0413775a858afca3`。台账区分现行语义审核、历史出处保留及其他产品隔离。[机器可读清单](../verification/v4.2.0-preparation/documentation-inventory.json)逐文件列出原始哈希、完整标题、语言、归属及动作。新准备／发布记录与本台账共同检查，属于新增，未替换历史。

## 审核轮次及实质修正

A——源码／契约／证据：核对公开API及模块映射、局部标签／AVL分派、线格式函数、排序临时数组及Group/Batch同步。合并说明时保留共享载荷全部实例、句柄失效及应用恢复条款。十个SVG与JSON重生成字节一致。

B——维护者／消费者视角：发现并纠正初稿将历史V2→V3迁移误标为V4的问题，将历史契约链接固定；恢复初次替换开头时遗漏的旧基准方法。修正main版本错误、源码指南仅中文的旧说法、一次排序排斥和分散的临时候选标签。

C——修补重查：编译运行新排序片段、严格Debug/Release CTest、七示例、长修改／解析测试、C/C++分发消费者、API／线格式样例、包可复现／篡改拒绝、链接／锚点及保护树检查。最终远端检查在交付时按不可变HEAD报告；本台账不预称未来CI成功。

浏览器以1280px／390px渲染全部十个归档SVG及双语HTML。首次直接打开SVG截图超时；关闭GPU并以同样SVG内容渲染后成功。截图为本地审核材料。没有重跑计时实验。最后的证据审查发现 Git 对 Windows 日志进行了换行规范化；现以确定性 gzip 保存原始字节流，并将每个解压日志重新与记录的 SHA-256 核对；保留原生 CRLF 输出，不将其当作文档尾空格。


### A—C 轮次内的独立审查修补

审查发现四个中文语言切换链接指向自身、历史 RC 检出说明与当前适用性混淆，以及比值分子表述不明确。使用、兼容性、V3→V4 迁移及验证页现指向对应英文原文。两版验证说明区分历史 `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08` 与最终准备 HEAD，并保留两个 Preview.3 回归身份。中文证据页明确比值为 Full 耗时／对照耗时，未改动归档实现或测量值。

原有十五条[命令回执](../verification/v4.2.0-preparation/commands.json)遗漏了已报告的 Usage 片段命令；这些回执保持原样。[sort-repair-commands.json](../verification/v4.2.0-preparation/sort-repair-commands.json)记录的是**本次重新执行**，不是补造历史输出。[复现脚本](../verification/v4.2.0-preparation/reproduce-sort.py)从两版 Usage 提取完全相同的 C 片段，采用 GCC C17、警告即错误及规范清单中的全部十六个生产模块编译并运行。独立的[稳定性核验程序](../verification/v4.2.0-preparation/sort-stability-harness.c)检查成功状态、键排序、八个实例的确切地址、比较器等值项的输入先后及比较器上下文。五条新命令全部成功；编译与最小片段运行的成功输出确实为空。原十五条和新增五条日志解压后的 SHA-256 均核对通过。先前冻结前的链接回执继续作为历史保存。

定向契约复核保留所有相关实例／共享容器条款、Group/Batch 重建、Handle 失效、可能失败且非事务性的重新插入、应用恢复及同步要求。核对来源为 `src/sort.c`、`src/managed_order.c`、`src/immutable_group.c`、`src/snapshot.c`、私有 Group 状态、冻结公共头、已接受 main 及双语详细 API 指南。四指南矩阵仍适用，无须更改指南、源码或契约。当前语言切换目标按清单检查，不重写历史切换链接。推送后的交付另附按归属及版本解析的目标审计和精确 HEAD CI，不把本前序版本记录当作最终 HEAD 验证。

## 逐文件结果

| 路径 | 归属／语言 | 分类 | 章节／源码依据／动作 |
| --- | --- | --- | --- |
| `.github/pull_request_template.md` | repo / en+zh-CN | current-policy | 产品范围与真实必需门槛; 可信归属清单与实际活动规则; 保留政策，不更改治理 |
| `CHANGELOG.md` | full / en | mixed-current/historical | 新条目；旧条目字节保留; 已接受4.2谱系及历史条目; 添加无日期4.2累计准备条目 |
| `CONTRIBUTING.md` | repo / en+zh-CN | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `README.md` | repo / en | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `README.zh-CN.md` | repo / zh-CN | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `benchmarks/README.md` | full / en | current/mixed-method | 完整示例／程序指南; 公开示例、构建目标及历史协议; 现行4.2入口；保留旧方法 |
| `benchmarks/README.zh-CN.md` | full / zh-CN | current/mixed-method | 完整示例／程序指南; 公开示例、构建目标及历史协议; 现行4.2入口；保留旧方法 |
| `benchmarks/WORKLOADS.md` | full / en | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `benchmarks/WORKLOADS.zh-CN.md` | full / zh-CN | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2.1/REPRODUCTION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1.2/REPRODUCTION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/PROTOCOL.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/REPRODUCTION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/RESULTS.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.1/RESULTS.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.2/PROTOCOL.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.2/REPRODUCTION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/AUDIT_INVENTORY.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/PROTOCOL.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/REPRODUCTION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/research/full-v4.2.0-preview.3/REPRODUCTION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview1-metadata.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview1-metadata.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview2-final-metadata.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview2-final-metadata.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview2-metadata.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview2-metadata.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview3-validation-metadata.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v3-preview3-validation-metadata.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview2-smoke/README.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview2-smoke/README.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview3-smoke/README.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview3-smoke/README.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview4/README.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `benchmarks/results/v4-preview4/README.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `consumers/layer-order-workbench/README.md` | consumer / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/README.zh-CN.md` | consumer / zh-CN | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/docs/MANUAL-EXPERIENCE.md` | consumer / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/docs/MEASUREMENT.md` | consumer / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/docs/TRACE.md` | consumer / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/docs/VALIDATION.md` | consumer / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `consumers/layer-order-workbench/docs/WINDOWS-HUMAN-EXPERIENCE.zh-CN.md` | consumer / zh-CN | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `docs/API.md` | full / en | current | JSON列出的全部标题; 公开头、托管／不可变组源码；全部符号与寿命章节; 保留 64 项清单；Batch 修改并入组章节 |
| `docs/API.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 公开头、托管／不可变组源码；全部符号与寿命章节; 保留 64 项清单；Batch 修改并入组章节 |
| `docs/ARCHITECTURE.md` | full / en | current | JSON列出的全部标题; 块标签／AVL、快照与 Group busy/marker；第1–7节; 说明4.2模型；已接受 Batch／并发条款并入第3／6节 |
| `docs/ARCHITECTURE.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 块标签／AVL、快照与 Group busy/marker；第1–7节; 说明4.2模型；已接受 Batch／并发条款并入第3／6节 |
| `docs/BENCHMARKS.md` | full / en | current | JSON列出的全部标题; 最终矩阵原始数据与派生比；完整现行节；历史表保留; 以累计4.2比值、生命周期反例及统计口径开篇 |
| `docs/BENCHMARKS.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 最终矩阵原始数据与派生比；完整现行节；历史表保留; 以累计4.2比值、生命周期反例及统计口径开篇 |
| `docs/CHANGELOG.zh-CN.md` | full / zh-CN | mixed-current/historical | 新条目；旧条目字节保留; 已接受4.2谱系及历史条目; 添加无日期4.2累计准备条目 |
| `docs/COMPATIBILITY.md` | full / en | current | JSON列出的全部标题; 冻结公开头／API／线格式样例；全部章节; 更新4.2适用性；保留源码重建与私有符号边界 |
| `docs/COMPATIBILITY.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 冻结公开头／API／线格式样例；全部章节; 更新4.2适用性；保留源码重建与私有符号边界 |
| `docs/DEVELOPMENT.md` | full / en | current | JSON列出的全部标题; 四指南门槛、消费者台账、4.2源码与证据；全部章节; 以4.2证据开篇；保留强制同步／适配器门槛 |
| `docs/DEVELOPMENT.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 四指南门槛、消费者台账、4.2源码与证据；全部章节; 以4.2证据开篇；保留强制同步／适配器门槛 |
| `docs/FULL.md` | repo / en | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `docs/FULL.zh-CN.md` | repo / zh-CN | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `docs/FULL_CONSUMER_HANDOFF.en.md` | full / en | mixed-current/historical | 新现行条目及历史谱系; 仅元数据差异及应用适配／依赖边界; 添加准备交接；保留历史条目 |
| `docs/FULL_CONSUMER_HANDOFF.md` | full / en+zh-CN | mixed-current/historical | 新现行条目及历史谱系; 仅元数据差异及应用适配／依赖边界; 添加准备交接；保留历史条目 |
| `docs/FULL_CONSUMER_HANDOFF.zh-CN.md` | full / zh-CN | mixed-current/historical | 新现行条目及历史谱系; 仅元数据差异及应用适配／依赖边界; 添加准备交接；保留历史条目 |
| `docs/FULL_RC1_EVIDENCE.md` | full / en | current | JSON列出的全部标题; 生成脚本、原始CSV及元数据、派生JSON；全部章节表格; 现行4.2汇总；重算图／JSON字节一致；保留归档 |
| `docs/FULL_RC1_EVIDENCE.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 生成脚本、原始CSV及元数据、派生JSON；全部章节表格; 现行4.2汇总；重算图／JSON字节一致；保留归档 |
| `docs/FULL_SELECTION_GUIDE.md` | full / en | current | JSON列出的全部标题; 实时／生命周期协议及源码；全部章节; 各API族独立选用；保留先行技术及实测反例 |
| `docs/FULL_SELECTION_GUIDE.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 实时／生命周期协议及源码；全部章节; 各API族独立选用；保留先行技术及实测反例 |
| `docs/IMPLEMENTATION_API_REFERENCE.md` | full / en | current | JSON列出的全部标题; 头与固定清单64函数／14类型；A–F节; 保留每项；附加已接受条款并入B7／E1 |
| `docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 头与固定清单64函数／14类型；A–F节; 保留每项；附加已接受条款并入B7／E1 |
| `docs/IMPLEMENTATION_INTERNALS.md` | full / en | current | JSON列出的全部标题; 16生产与4旧模块；第1–15节及私有调用; 保留算法；区分确切snap_be_read／snap_be_write标识 |
| `docs/IMPLEMENTATION_INTERNALS.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 16生产与4旧模块；第1–15节及私有调用; 保留算法；区分确切snap_be_read／snap_be_write标识 |
| `docs/IMPLEMENTATION_VALIDATION.md` | full / en | current | JSON列出的全部标题; 实际测试函数、API／线格式样例和打包工具；第1–9节; 历史检出点复现RC；最终元数据使用准备HEAD |
| `docs/IMPLEMENTATION_VALIDATION.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 实际测试函数、API／线格式样例和打包工具；第1–9节; 历史检出点复现RC；最终元数据使用准备HEAD |
| `docs/IMPLEMENTATION_WALKTHROUGH.md` | full / en | current | JSON列出的全部标题; 16模块清单及Order／托管／快照／Group／sort调用；第0–14节; 目标适用性、正面排序定位、确切线格式函数及源码pin |
| `docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 16模块清单及Order／托管／快照／Group／sort调用；第0–14节; 目标适用性、正面排序定位、确切线格式函数及源码pin |
| `docs/INTEGRATION.md` | full / en | current | JSON列出的全部标题; CMake、生产清单、分发校验；全部章节; 纠正main版本错误；区分目标源码与已发布下载 |
| `docs/INTEGRATION.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; CMake、生产清单、分发校验；全部章节; 纠正main版本错误；区分目标源码与已发布下载 |
| `docs/MIGRATION_V3_V4.md` | full / en | current | JSON列出的全部标题; 现行公开头及严格导入；全部章节; 保留迁移表；统一全部实例修改及4.2适用性 |
| `docs/MIGRATION_V3_V4.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 现行公开头及严格导入；全部章节; 保留迁移表；统一全部实例修改及4.2适用性 |
| `docs/PRODUCT_LINES.md` | repo / en | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `docs/PRODUCT_LINES.zh-CN.md` | repo / zh-CN | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `docs/USAGE.md` | full / en | current | JSON列出的全部标题; 公开头、sort.c及七示例；全部工作流与失败章节; 移除一次排序排斥；实际编译稳定排序例；保留全部实例恢复 |
| `docs/USAGE.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 公开头、sort.c及七示例；全部工作流与失败章节; 移除一次排序排斥；实际编译稳定排序例；保留全部实例恢复 |
| `docs/V3_MIGRATION.md` | full / en | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/V3_MIGRATION.zh-CN.md` | full / zh-CN | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/V4_1_0.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_1_0.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1_2.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1_2.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1_2_1.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW1_2_1.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW2.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW2.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW3.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_PREVIEW3.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_RC1.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_2_0_RC1.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_FREEZE.md` | full / en | frozen-contract | 全部所有权、回调、状态、API及格式节; 头清单与线格式样例; 保留规范字节 |
| `docs/V4_FREEZE.zh-CN.md` | full / zh-CN | frozen-contract | 全部所有权、回调、状态、API及格式节; 头清单与线格式样例; 保留规范字节 |
| `docs/V4_PREVIEW1.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW1.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW2.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW2.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW3.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW3.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW4.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_PREVIEW4.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_RC1.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_RC1.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_STABLE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/V4_STABLE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/VALIDATION.md` | full / en | current | JSON列出的全部标题; 39项CTest、修改／OOM／线格式和消费者；全部章节; 区分历史回归SHA与最终准备执行 |
| `docs/VALIDATION.zh-CN.md` | full / zh-CN | current | JSON列出的全部标题; 39项CTest、修改／OOM／线格式和消费者；全部章节; 区分历史回归SHA与最终准备执行 |
| `docs/WORKLOAD_EVIDENCE.md` | full / en | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/WORKLOAD_EVIDENCE.zh-CN.md` | full / zh-CN | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/demo/index.html` | full / en | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/demo/index.zh-CN.html` | full / zh-CN | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/demo/index.zh-CN.md` | full / zh-CN | historical/method | 出处与现行跳转；历史观测不改; 明确历史版本及出处; 保留正文；必要时修正现行跳转 |
| `docs/design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_LIVE_CORE_RESEARCH_HISTORY.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_1_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_1_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_1_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_2_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_COMPLETION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_COMPLETION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_CONTINUATION.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_CONTINUATION.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW1_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW2_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW2_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW2_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW3_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW3_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_PREVIEW3_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_API_INDEX.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_API_INDEX.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_BILINGUAL_CHECKLIST.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_DOCUMENTATION_AUDIT.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_RESUME.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_REVIEW_REPAIR.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_REVIEW_REPAIR.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_TASK.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_2_0_RC1_TASK.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_ARCHITECTURE.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_ARCHITECTURE.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_DECISIONS.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/design/V4_DECISIONS.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/governance/BASELINE.md` | repo / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/governance/BASELINE.zh-CN.md` | repo / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/governance/OWNERSHIP.md` | repo / en | current-policy | 产品范围与真实必需门槛; 可信归属清单与实际活动规则; 保留政策，不更改治理 |
| `docs/governance/OWNERSHIP.zh-CN.md` | repo / zh-CN | current-policy | 产品范围与真实必需门槛; 可信归属清单与实际活动规则; 保留政策，不更改治理 |
| `docs/governance/RULESETS.md` | repo / en | current-policy | 产品范围与真实必需门槛; 可信归属清单与实际活动规则; 保留政策，不更改治理 |
| `docs/governance/RULESETS.zh-CN.md` | repo / zh-CN | current-policy | 产品范围与真实必需门槛; 可信归属清单与实际活动规则; 保留政策，不更改治理 |
| `docs/history/BENCHMARK_HISTORY.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/history/BENCHMARK_HISTORY.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/history/DESIGN_HISTORY.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/history/DESIGN_HISTORY.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/index.html` | repo / en+zh-CN | current | 全文; Full接口族及固定证据；Mini／Workbench契约不变; Repo内改写呈现导航；保留已发布入口 |
| `docs/v3/README.md` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/v3/README.zh-CN.md` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/v3/index.html` | full / en | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `docs/v3/index.zh-CN.html` | full / zh-CN | historical/evidence | 出处及入链，非重做实验; 固定记录；核对归档字节及现行入链; 保留原文 |
| `examples/README.md` | full / en | current/mixed-method | 完整示例／程序指南; 公开示例、构建目标及历史协议; 现行4.2入口；保留旧方法 |
| `examples/README.zh-CN.md` | full / zh-CN | current/mixed-method | 完整示例／程序指南; 公开示例、构建目标及历史协议; 现行4.2入口；保留旧方法 |
| `mini/CHANGELOG.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/README.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/README.zh-CN.md` | mini / zh-CN | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/ARCHITECTURE.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/CONTRACT.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/DEVELOPMENT.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/PERFORMANCE.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/RELEASE_v1.0.0.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |
| `mini/docs/USAGE.md` | mini / en | other-owner | 归属边界; 独立产品；字节未改; 保留 |

## 剩余边界及技术债影响

未发现需要重开RC的生产缺陷。不提议算法、API、格式、依赖或研究处置变化。FULL-RI-001至007的接受处置与触发条件保留在历史RC／Preview记录及Issue #3／#27。文档与元数据澄清采用及证据，不将未测替代方案标为解决。最终集成验证与发布授权分别进行。
