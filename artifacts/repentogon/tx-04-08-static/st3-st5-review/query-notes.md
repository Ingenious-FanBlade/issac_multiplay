# 主审静态查询说明

2026-10-26，tx-04-08 ST3–ST5。这里的 JSON 是对当前 IDB 指令、反编译、RTTI 和本地 SDK/原型源码的静态导出，不是测试输入或运行结果。Python 仅在 IDA 中读取资料并保存导出；PowerShell 仅阅读、摘录和编辑文档。没有执行 JSON 语法校验、构建、生成器、游戏、离线工具或测试，没有计算或校验哈希，没有改 IDB。

主审最初手抄 `Room::constructor` 特征时，把 `33 C5 89 45 ?? 56 57 50` 误写为 `33 C5 50`，查询得到零命中。回读 SDK `Room.zhl:1` 后按完整特征重新查询，唯一命中 VA `0x7DF8F0`，随后保存完整指令和伪代码到 `main-room-constructor.json`。初次零命中是查询转录错误，不是目标缺少该函数。

`FuncItems` 会包含远处 EH funclet；导出保留这些原始条目，但正文不把它们算作正常路径。特别是 Player 构造的远处基类析构跳转属于异常清理，不能据此宣称构造正常路径执行了析构。Hex-Rays 的整数/指针、额外寄存器参数与推测返回类型也不直接充当 ABI。

`main-sdk-source.json` 为本地 SDK 带行号快照，`main-prototype-source.json` 为原型字段、分类及来源相关摘录。读取 JSON 内容用于审阅不作为格式校验或验收。主审结论另见研究报告；没有因证据文件可读而预记功能通过。

主审向 ST3 代理转述 GridSpawn 两条写入时曾误写地址 `7E2259/7E225F`，随即重新回读原始导出并更正为 `7E2257: 89 47 08` 与 `7E225D: 89 47 18`，随后 `7E2260` 调 GridInit。原始 JSON 中地址始终正确，最终字段表采用更正地址。

代理初筛遗漏 flags 的高 dword、SubPlayer/replaced 初始化和 Familiar Player 关联写入；主审完整指令补读后要求逐行修正。Grid ctor/Factory、字段 Lua 注册、GetSubPlayer/GridPosition 注册及原生函数体都保存为独立 `main-*.json`，不以“SDK 未声明”或“过滤未命中”直接推出缺少目标语义/初始化。
