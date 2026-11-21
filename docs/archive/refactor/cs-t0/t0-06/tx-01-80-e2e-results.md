# tx-01-80 统一验收：进程识别阶段 FAIL

日期：2026-10-19。统一入口仅执行一次，原始产物为 `artifacts/cs-t0/tx-01-80-instrumented-e2e-01/`；finalization=COMPLETED，整体 FAIL。

用户确认已重启、启用 fixture 并准备全新 seeded run 后，执行器收到 `start primary`。首次 `Suite.process_snapshot()` 抛出 `Exactly one user-started isaac-ng.exe is required`；异常位置只说明解码结果不是单元素 list，不能区分零进程、多进程或格式异常。旧查询没有持久化 stdout/stderr/候选项，故本次根因仍 UNKNOWN，不声称用户未启动游戏。

没有创建 captures 目录，没有 controller Popen、helper、注入、目标补丁、场景操作或故障触发。Native 测试专用探针没有运行。fixture 已由用户启用；后续只读日志观察到 seed=1998345781、room=84 的 A 单玩家 raw_input 记录，这不能倒推出首次查询的结果或替代进程身份。没有重新查询进程、重跑验收或操作游戏。

历史 21 PASS 保留原来源，不作为本次新二进制 PASS。本轮新增 0 PASS，十项授权范围全部仍 NOT_COVERED；W08 全量 replay、W01 真实 Online/Isaac Sync 隔离仍 USER_EXCLUDED。完整授权范围在原始 `authorized-scope-result.json`，历史 FAIL/INTERRUPTED 均保留。

后续修正应先补齐原始进程枚举/身份/错误诊断，并用现有 Win32 Toolhelp 进程枚举与 PSAPI 32 位模块路线直接读取；不静默丢弃不可读候选，不因文件路径匹配便忽略另一 isaac-ng.exe。修正后如继续，须作为实质修改后的新整体验收，不自动重跑本次。
