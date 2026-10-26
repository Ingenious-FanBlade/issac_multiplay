# tx-04-07 ST5–ST7 主智能体审阅材料

2026-10-25。使用 IDA MCP 读取 v1.9.7.12 IDB，imagebase=`0x400000`。本目录是主智能体独立于三个逆向子智能体采集的静态资料；没有调用游戏函数、修改数据库、调试、构建、测试或校验哈希。

- `export_review.py`：只读导出脚本，供 IDA MCP `py_exec_file` 调用。各 JSON 的 `instructions` 保留 VA/RVA、字节和汇编，`decompile` 为原始反编译；标签是人工描述，不写回数据库。
- `metadata.json`：输入路径、数据库路径、基址和所导出的完整函数范围。
- 更正：早期 `Game_Constructor.json` 将 `0x6F0AF0` 误标为构造函数；该原始文件保留，但最终解释使用 `Game_ExecuteStartPreparation.json`。官方完整构造签名对应 `0x6ED4B0`，另见 `Game_Constructor_OfficialSignature.json`；启动准备→初始化→ResetState 的实际调用链没有因此改变。
- `player-vtable.json`：Player vtable 第 3 槽（`+0xC`）的静态实值，用于连接门切换的 Player 更新虚调用。
- `stock-role-candidates.json`：原生候选表开头 32 个 int32，只是有界数据摘录，不声称这是数组全长或当次已解锁角色表。
- `export_sources.ps1`、`source-excerpts.md`：当前工程与官方源码摘录及原行号，只读取文件，不运行其逻辑。首次摘录因 PowerShell 嵌套数组展开导致范围类型错误，未执行任何目标代码；改为显式字符串范围后完成摘录。

自动类型、变量名、反编译参数不单独作为 ABI 或对象身份依据。有限函数/引用读取不证明任意 Lua、间接调用、官方 Hook 的完整闭包；最终审阅结论见 [静态确认报告](../../../../docs/research/tx-04-07-static-confirmation.md)。
