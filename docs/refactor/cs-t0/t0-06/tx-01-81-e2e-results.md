# tx-01-81：有限进程可见范围下识别 FAIL

日期：2026-10-19。用户确认修正后继续，统一入口仅运行一次；原始目录 `artifacts/cs-t0/tx-01-81-instrumented-e2e-01/`，finalization=COMPLETED，整体 FAIL。

直接 Win32 Toolhelp 枚举实际完成：PROCESSENTRY32W=568 bytes；扫描 3 个进程；ERROR_NO_MORE_FILES=18；isaac-ng.exe 候选为 0。原始诊断保存在 `process-queries/001/enumeration.json`。不再是无法区分输出形态的泛化消息。

运行环境推断：正常 Windows 桌面通常具有远多于 3 个进程，当前极小的枚举视图很可能来自工作区执行沙箱的进程隔离；本次并未在沙箱外查询，故不把这一推断当作独立验证后的事实。用户确认游戏仍准备好，不能将零候选误写成用户未启动游戏。

没有 captures 目录、controller Popen、helper、注入或游戏 Hook，Native/故障/场景均未执行；没有额外进程探测或自动重跑。十个授权测试仍未覆盖，历史 21 PASS 不变；全量 replay 与真实 Online/Sync 实机隔离继续排除。

后续应仅在获得执行环境范围确认后，将完整同一 E2E 放到工作区沙箱外运行；不要先做独立查询/试跑，不更改原始计数或放松唯一进程/身份要求。tx-01-80/81 FAIL 均保留。
