# tx-04-08 ST2 主智能体静态取证

2026-10-26，Asia/Shanghai。主审结论见 [ST2 报告](../../../../docs/research/tx-04-08-st2-static-confirmation.md)。这些文件由主智能体独立读取当前目标 IDB 和本地源码产生；代理材料另外放在相邻 `st2-agent/`。不是构建或运行验收产物。

| 文件 | 内容与复查方式 |
| --- | --- |
| `main-stage-functions.json` | 保存 IDB/input 路径、imagebase、六个入口的 VA/RVA、伪码、全部 FuncItems 指令/字节与入口 xref。在同版本 IDB 回读所列地址即可复查；自动类型/函数块归属不作 ABI 或正常返回的独立证明 |
| `main-stage-windows.json` | 六个带标签的关键汇编窗口，起止地址均记录在文件中；包含 Manager 分流、Game 更新段、Room 参数、Odd 普通点、tail 错误处理及嵌套 Manager 调用 |
| `main-asm-signatures.json` | IDA 只读字节搜索的参数与结果；定位官方 MainUpdate/Return，未计算任何哈希，未运行目标程序 |
| `main-source-excerpts.json` | 本仓库和官方 REPENTOGON-1.1.3 的路径、起止行及带行号原文，包括 Hook 排序、detour 链、loader、callback 与当前接线 |

取证使用 IDA MCP 的只读 Python API（`get_input_file_path/get_imagebase/get_func/decompile/FuncItems/XrefsTo/get_bytes/generate_disasm_line`）和 `find_bytes`；静态导出只写本目录，不改 IDB。源码摘录由 PowerShell `Get-Content` 读取并写 JSON。未执行生成器、构建、游戏、离线工具、测试、校验脚本、Hook 安装或 DLL 部署。

重点复查地址：`0x92E911 → 0x6F6D30`，`0x6F78DF → 0x7F7F60`，`0x7F93A7 → 0x413F00`，`0x6F7A34 → 0x846C90`；官方整轮短路从 `0x6F77A4` 跳 `0x6F7A39`。原始地址均是 VA，RVA 减 `0x400000`。生成桩/Hook 的实际运行地址、启用状态及同优先级次序仍未观察。
