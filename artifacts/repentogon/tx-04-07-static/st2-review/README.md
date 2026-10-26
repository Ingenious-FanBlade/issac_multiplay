# tx-04-07 ST2 主审证据

2026-10-26。当前 v1.9.7.12.J273 IDB 的只读静态导出，未启动游戏、构建、测试或校验哈希，未修改/保存 IDB。

- [主报告第 15–18 节](../../../../docs/research/tx-04-07-static-confirmation.md#15-st2-主审范围结论与材料更正2026-10-26)：条件化结论、代理材料更正、输入/动画与实现影响。
- [metadata.json](metadata.json)：IDB/输入路径、imagebase、内嵌版本字符串及 20 个完整函数索引。函数 JSON 保存逐条 VA/RVA、原始字节、反编译、直接/间接调用与入口引用。
- [details.json](details.json)：原生调用方、Lua 动画注册片段、InputManager vtable 尾部实值及复活入口字节；片段不构成调用图完整闭包。
- [source-excerpts.md](source-excerpts.md)：官方 zhl、Lua 包装、CACHE_ALL、HealthType、音效与相关 Hook 的带行号源码摘录；官方代码不冒充原生 IDB 内容。
- [export_review.py](export_review.py)：通过 IDA MCP `py_eval` 读取脚本执行，重新导出同范围静态材料；只读目标，仅写本目录文件，不是测试或正确性校验入口。

原始材料在 [st2-agent](../st2-agent/notes.md) 保留；本轮没有重跑其有界扫描或把扫描中的跳过集合视为已排除。自动类型与局部变量不是 ABI 证据，正文结论以指令和明确条件为准。设备通知、回调完整闭包、实际健康与输入效果等 UNKNOWN 见主报告。
