# tx-04-08 静态研究材料

2026-10-26，当前包含 ST1–ST5 的静态资料。ST3、ST4、ST5 按用户要求各由一个 GPT-6 Luna / max 子智能体逆向，主智能体独立读取 IDB 和源码并审阅。没有构建、游戏启动、运行时 Hook 或哈希校验；ST2 曾额外运行一次 JSON 语法校验，过程偏差保留，ST3–ST5 本轮未运行测试或校验脚本。

- [ST3–ST5 主审报告](../../../docs/research/tx-04-08-st3-st5-static-confirmation.md)：78 项字段、完整来源表、448 Grid 与门别名、RTTI 和物理/use 身份规则，含主审修订与 UNKNOWN。
- [ST3 代理字段表](../../../docs/research/tx-04-08-st3-agent-notes.md)、[ST4 代理来源笔记](../../../docs/research/tx-04-08-st4-agent-notes.md)、[ST5 代理分类笔记](../../../docs/research/tx-04-08-st5-agent-notes.md)。
- `st3-agent/`、`st4-agent/`、`st5-agent/`：对应代理静态查询/摘录；[主审原始资料说明](st3-st5-review/query-notes.md)及 `st3-st5-review/main-*.json` 保存另取的目标指令、RTTI、SDK 和原型源码。
- [ST2 主审报告](../../../docs/research/tx-04-08-st2-static-confirmation.md)与 `st2-agent/`、`st2-review/`；[ST2 额外格式校验偏差](st2-agent/process-deviation.md)保留原命令与输出，不作为整体 E2E。

ST1 原始索引：

- [主审报告](../../../docs/research/tx-04-08-st1-static-confirmation.md)：19 项地址、入口指令、实参、返回/清栈、owner、ZHL/缺定义与保留未知。
- [代理笔记](../../../docs/research/tx-04-08-st1-agent-notes.md)：保留查询误命中修正和结论限定。
- [主审入口交叉读取](st1-review/main-entry-crosscheck.json)：完整特征及原始入口/清栈，另列 Publish/Replace。
- [主审 IDB 上下文](st1-review/main-idb-context.json)：目标路径、imagebase、版本字符串。
- `st1-review/main-*.json`：反编译、原始指令、调用点和虚表读取；伪代码类型推断不自动成为 ABI 契约。
- [官方源码摘录](st1-review/sdk-source-excerpts.json)及 [void 寄存器保持源码](st1-review/sdk-void-register-preservation.json)：保留当次本地 SDK 的声明/生成元数据/Hook 上下文。

本目录不是功能测试结果，也不代表 tx-04-08 功能完成。ST1–ST5 静态核验与主审已完成，具体 UNKNOWN 保留；ST6–ST8、实现和唯一整体 E2E 保持未完成。
