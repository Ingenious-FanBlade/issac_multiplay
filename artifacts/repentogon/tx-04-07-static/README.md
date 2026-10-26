# tx-04-07 静态证据：ST1–ST7

2026-10-25；2026-10-26 追加 ST2 主审。只读 IDA MCP 静态分析；目标版本由 IDB 路径及内嵌 `v1.9.7.12.J273` 字符串确认，没有校验哈希，没有启动游戏、构建或测试。

- [主报告](../../../docs/research/tx-04-07-static-confirmation.md)：ST1–ST7 的结论、条件、主审与后续实现影响。
- [子智能体笔记](../../../docs/research/tx-04-07-st1-agent-notes.md)、[六个核心函数导出](st1-agent/ida-core-functions.json)：GPT-6 Luna / max 使用 IDA MCP 读取的材料。主审已纠正“临时死亡组 end 检查排除 Main”等误读，原文保留并在顶部标注。
- `st1-review/`：主智能体独立采集的函数 JSON；`instructions` 含 VA/RVA、原始字节与汇编，`decompile` 为 IDA 原始输出。函数标签只作人工解释，不写回 IDB。
- [主审导出脚本](st1-review/export_review.py)、[补充导出脚本](st1-review/export_supplement.py)：仅适用于同版本、base=0x400000 且已分析的 IDB；通过 IDA MCP `py_exec_file` 执行，仅导出静态资料。脚本不是测试或正确性校验入口。
- [元数据](st1-review/metadata.json)、[版本/vtable/死亡计数定位线索](st1-review/supplement-details.json)。`CountNonGhost` 和 `DeathHandler` 等为报告中的描述性标签，不声称是官方符号。
- [官方源码摘录](st1-review/source-excerpts.md)：保留原文件行号，覆盖角色枚举、死亡回调、官方复活处理与事件 tracker 等引用。

JSON 中自动识别的类型、调用参数和局部变量可能有误；分支、寄存器、虚调用和字段写入以所附汇编为准。没有调用目标游戏函数、调试附着、断点、字节修改或 IDB 重命名。

ST1 子智能体材料与最终审阅状态见主报告第 6 节。上一轮 ST5–ST7 各使用一个 GPT-6 Luna / max 子智能体，主智能体另行取证并完成审阅；追加结论见主报告第 7–10 节：

- [ST5 笔记](st5-agent/notes.md)、`st5-agent/export_st5.py`：Baby 位、切房置位、保存/恢复、普通换层与特殊 Reset 的边界，保留启动准备函数误标 constructor 的更正记录。
- [ST6 笔记](st6-agent/notes.md)、`st6-agent/ida-readonly-export.py`、`st6-agent/ida-evidence.json`：角色表配置成就筛选、Init/Baby skin 区分、门 owner 到 B 射击暂停。
- [ST7 笔记](st7-agent/notes.md)、`st7-agent/export_st7.py`、`st7-agent/ida-st7-target-windows.json`：五入口覆盖范围、GameState/官方附加文件/删除缺口、跨 loop 窗口与旧归约问题，顶部保留主审批注。
- [主审材料](st5-st7-review/README.md)：38 个完整函数、vtable/候选表、源码摘录与只读导出脚本；不把有限读取当作间接调用闭包证明。

`st2-agent/`、`st3-agent/`、`st4-agent/` 是此前已存在的代理材料；上一轮 ST5–ST7 只将它们作线索，没有补记其整项主审完成。ST2 于 2026-10-26 完成主审，原始材料保留，见下文。

本轮新增 ST3、ST4，各用一个 GPT-6 Luna / max 子智能体，并由主智能体独立取证审阅；结论见主报告第 11–14 节：

- [ST3 新笔记](st3-agent-current/notes.md)、`st3-agent-current/export_st3_readonly.py`、`st3-agent-current/st3-static-export.json`：Lua 参数/FSM、内部更新与回调、Player/controller 条件、外层 Join pass 和设备通知边界。
- [ST4 新笔记](st4-agent-current/notes.md)、`st4-agent-current/export_st4_readonly.py`、`st4-agent-current/ida-evidence.json`、`st4-agent-current/metadata.json`：Continue/starting、保存记录同步、controller 重分配及 Restore/RunExit 退休菜单 Host 的源码冲突。
- [ST3/ST4 主审](st3-st4-review/README.md)：31 个完整函数、vtable、当前版本字符串、源码摘录和只读可重复脚本。旧 `st3-agent/`、`st4-agent/` 导出保留，不覆盖；发现的地址/字段误读记在新笔记和主报告。

2026-10-26 新增 [ST2 主审材料](st2-review/README.md)：主智能体直接回读 20 个完整函数、调用方/注册片段、vtable 尾部和官方源码；主报告第 15–18 节记录普通复用路径、健康/动画边界、47 槽与地址更正。没有另建子智能体或覆盖原始扫描；ST1–ST7 静态取证与主审现均完成，UNKNOWN 保留。

整个目录没有测试或构建产物，tx-04-07 的唯一整体 E2E 尚未执行；本轮未修改生产代码。
