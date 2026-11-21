# tx-01-69 整体静态文档审阅

日期：2026-10-18。目标：逐项核对参考会话列出的偏移，补全操作 3 最小原样只读静态字段登记的最后一项缺口。由主智能体读取会话、当前 IDA 和既有当前版本证据完成，不增加子智能体任务。

遵循[现行规范](../../project-conventions.md)。本轮分析只做会话/文件/源码阅读、静态反编译、指令/虚表数据读取、证据导出与文档编辑。没有游戏操作、运行时读取实验、注入、构建、代码测试、IDB 修改或哈希校验。所有修订完成后只执行本轮一次整体静态文档验收，不重跑 tx-01-63 至 tx-01-68 的验收。

审阅对象：[逐项核对表](tx-01-69-offset-reconciliation.md)、[会话快照](../../../../../artifacts/cs-t0/tx-01-69-static/reference-session.json)、[当前 IDA 证据](../../../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json)、[字段补充](../../../../../artifacts/cs-t0/tx-01-69-static/field-evidence.json)、[主脚本](export_tx_01_69.py)、[补充脚本](export_tx_01_69_fields.py)、[阶段入口](README.md)及两个总入口本轮段落。脚本以非 main 名称加载[有 guard 的格式化 helper](export_tx_01_67_review.py)，不会执行历史导出。

本轮结果的主要内容：

- 将参考最终答复中的根/容器、EL 描述、Entity 字段、玩家关系、池、具名心量/state/timer、Grid 与虚表相对槽统一登记；当前依据与旧版本来源分别标注。
- 明确 8 处旧数值更正：工厂指针、五个 Player 字段、Familiar stride 和 Pickup pool 起点。保留原始说法，避免统一平移旧结构。
- 新闭合分类/Index/EntityFlags，重新确认 bool 宽度及最小 Init/发布/变更线索；区分 64-bit EntityFlags、128-bit TearFlags 与独立 ProjectileFlags。
- 补充当前原生 Variant setter 的单字段写入，区分参考 REPENTOGON 扩展 setter 的 Effect VarData 行为；没有借此声称完整 Morph/writer 已恢复。

整体审阅项目：

1. 参考会话、旧版来源和当前目标标识明确，偏移表中的条目均有当前直接证据或明确的既有当前版本证据引用。
2. 8 处旧数值更正、448 Grid 槽、global slot 与对象偏移、内嵌对象与指针、vector 与 EL 的区别准确。
3. 分类、Index、flags 的当前 getter、存储宽度、必要初始化/变更指令相互支持；不将分类或 Room 序号作为永久身份。
4. 三个 flags 域、byte 状态字段、float 向量和逐类状态/计时字段不混用；本轮未恢复的 bit 语义保留限制。
5. PlayerType/PlayerIndex/Twin/backup/replaced 与 Entity 同名概念分开，旧值不能直接采样；Grid 不套用 Entity 布局或重复加 Desc 偏移。
6. 8 张当前虚表支持所列相对槽，目标按具体类记录；采样不调用 getter、虚函数、查询或清理入口。
7. 原始快照、历史报告和证据保留，新导出 create-new；文件引用、入口范围与当前完成情况一致。
8. 完成范围仅为本会话约定的操作 3 最小原样只读静态字段研究；未知/专用字段、全部 Morph/Restore、采集器、身份和 T0-06 整体均未冒充完成。

结果：`PASS_STATIC_DOCUMENT_REVIEW`。上述 8 项整体静态审阅通过，仅适用于会话偏移核对与操作 3 最小原样只读静态字段登记。

本次唯一整体审阅读取最终核对表、审阅记录、阶段入口及两个总入口更新，人工对照保留的 3 条参考最终答复与 6 张合计 74 行的偏移/槽位表。主导出含 31 个函数和 8 张虚表，补充含 3 个函数和 4 段初始化/发布指令，合计 34 个不同函数；函数均有伪代码及非空指令，未出现导出错误。三个审阅文档中的 56 处本地文件引用均存在，两个总入口的新引用与这些目标一致。

确认 8 处旧数值需要更正，其余所列偏移和虚槽均有当前依据；已补齐剩余分类/Index/flags 并区分宽度和对象域。本轮没有重跑历史验收、游戏、构建或代码测试。操作 3 在本会话约定的最小静态范围内完成，采集器、操作 4 及后续工作保持未完成。
