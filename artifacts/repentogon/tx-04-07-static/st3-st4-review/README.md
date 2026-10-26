# tx-04-07 ST3/ST4 主智能体审阅证据

2026-10-25。主智能体使用当前 IDA MCP，独立采集 v1.9.7.12、imagebase=`0x400000` 的静态资料；没有启动目标、运行测试/构建、修改 IDB 或校验哈希。

- `export_review.py`：IDA MCP `py_exec_file` 可重复执行的只读导出脚本；`metadata.json` 保存 IDB/input 路径与完整函数边界。各函数 JSON 的 `instructions` 保存逐条 VA/RVA、字节、汇编，`decompile` 为原始输出，`xrefs_to` 是当前数据库可见引用。
- 本轮包含 31 个完整函数及 `player-vtable.json`；版本字符串仅作可读标识，无摘要校验。主审另按官方唯一签名匹配 `FirstCollectibleOwner 0x5950B0`，避免将道具 566 查询误称为设备事件。
- `export_sources.ps1`、`source-excerpts.md`：只读摘录官方 Lua 参数桥接、controller 重分配、本工程 owner/tick/Join、K 创建和 Restore/RunExit 生命周期的源码及原行号。
- 函数标签仅作人工解释，不写回数据库。`RoomLoad`、`SyncPlayersFromGameState` 等描述性名称不代替 ABI 证据。自动变量名、类型和伪代码参数可能有误，以原始指令为准。
- `st3-agent/`、`st4-agent/` 是本轮之前已存在的原始导出，原样保留；本轮新子智能体材料在同级的 `st3-agent-current/`、`st4-agent-current/`。

本目录覆盖有限函数和源码；不构成全部虚调用、Lua/官方 Hook、物品效果、特殊房间或设备通知闭包证明。主审最终结论与更正记录写入 [静态报告](../../../../docs/research/tx-04-07-static-confirmation.md)。
