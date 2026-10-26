# tx-04-07 修复后的唯一整体 E2E

2026-10-25，America/Los_Angeles。用户要求修复 e2e-01 后继续测试，完成 trace/空启动 scope 等实质修改与纯构建，再经用户本次准备确认后启动。仅运行一次统一入口、一次游戏启动，无自动重跑。

结果 **FAIL**：S0–S12 COMPLETE，S13 FAILED，S14–S21 NOT_RUN。S13 执行 `cs stop` 后，游戏在退休/原生 B 移除流程崩溃（0xC0000005），控制口随之报 10054。准确故障指令与根因仍 UNKNOWN。最后 checkpoint 的 trace 丢失/队列/写入/唤醒/截断均为 0；首轮启动丢失未重现。V1/V7/V11/V15 PASS，其余 FAIL 或 NOT_RUN，详见原始 report.json；场景 COMPLETE 不等于功能 PASS。

复现入口（会再次接管输入和改动已授权第二档，须另行安排实机启动）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

- `report.json` / `report.md`：原始自动报告，不重新归约或覆盖。
- `steps.jsonl` / `control.jsonl` / `enabled/trace.jsonl`：场景、命令与原始事件。
- `sources/` / `scenario.json`：本次部署源与场景。
- `remote-1.jsonl` / `relay-1.jsonl` / `network-ledger.json`：本轮网络记录，四轮未完成。
- `file-*.json*` / `persistent-evidence.json` / `save-changes.json`：监听、窗口、保存结果；崩溃文件仍标为 allowed=false，不新增白名单。
- `crash/`：新增转储与同名游戏日志的副本；minidump 含进程内存，只留本地，原 Documents 文件未删除。
- `preparation.json`：本次准备确认与 Download Helper 恢复时间。

测试 mod 已移除，配置 enabled/control_port.enabled=false，Download Helper 原状态恢复，无残留游戏进程。存档不回滚，无哈希计算。正常退休/Bye/线程停止因崩溃未获完整证据。

构建见 [build-01.log](../tx-04-07-trace-fix/build-01.log)，改动见[修复说明](../../../docs/design/tx-04-07-trace-fix.md)，完整解释见[结果](../../../docs/design/tx-04-07-results.md)。
