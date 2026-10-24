# tx-01-81：修正进程枚举并继续同一剩余测试目标

日期：2026-10-19。目标仍为原来十个未完成子项；排除 replay 与真实 Online/Sync 隔离，不拆成额外试跑。tx-01-80 原始 FAIL 保留，尚未执行本次整体验收。

后续运行：本次唯一验收已在进程识别阶段 FAIL；完整 Win32 诊断实际只扫描到 3 个进程、0 个游戏候选，未注入，见 [运行记录](tx-01-81-e2e-results.md)。本文“未执行”指修正准备时的历史状态。

## 实现前失败方式

- 零/多个候选或输出形态异常被同一短消息覆盖，缺少原始诊断：保存候选及枚举状态，再给精确错误。
- PowerShell 子进程选择/序列化或 Modules 属性读取导致丢项：改用项目现有 Win32 Toolhelp 枚举、QueryFullProcessImageName/GetProcessTimes 与 PSAPI LIST_MODULES_32BIT 路线。
- 打不开一个候选就把它忽略，从而错误宣称唯一：保留候选错误并中止，不降为单进程。
- 只按路径选一个并忽略其它同名进程，或只按 PID 接纳旧实例：仍要求 basename 候选总数恰好为一，再检查完整路径和创建时间；保留 resident module 禁止重注入判断。
- 枚举 handle 泄漏、Process32First/Next 错误当作空列表：finally 关闭自有 handle，ERROR_NO_MORE_FILES 之外保留 Win32 错误。

实现/资料分析阶段不运行程序、进程查询或试跑；准备好后才请求新的整体执行。Native、controller、Mod 不因这个枚举修正而重编译或改动。

修正已实现：`win_observer.py::discover_unique` 使用 PROCESSENTRY32W / Process32FirstW / Process32NextW；在判断唯一前保存全部同名候选，原始字段包括 PID、parent、线程数，选中后保存完整路径、创建时间与 PSAPI 模块清单。`process-queries/001/enumeration.json` 即使失败也在 finally 中输出 Win32 结束码、候选与错误；所有自有句柄均关闭。不再通过 PowerShell JSON 形态决定进程数。

`Suite.process_snapshot` 在该轮统一验收内部调用这个只读查询。新 invocation 继续读取 tx-01-79 的历史 PASS，并显式复制 tx-01-80 的 FAIL/finalization/授权范围结果。当前仅静态代码审阅完成，**未执行进程查询、Python 程序或新一轮验收**。等待用户对修正后整体验收的确认；未注入的游戏不因本次识别失败而被要求重新启动。
