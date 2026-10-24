# tx-03-05：Hook 校正后的双机 F0 实机验收

日期：2026-10-21（Asia/Shanghai）。状态：**唯一双机 F0 已执行，整体 FAIL（33/38）**。真实初始化与基础 51 槽恢复通过；终点多发布 24 tick。两端正常退休、游戏存活，未重跑。

## 完整目标与授权

用户在 tx-03-04 固定 Hook 校正后明确要求“现在开始实机测试”，并在本次会话确认“两端已就绪，继续 F0”：Host/VM 已重启游戏、各处于 offline/local，以撒、种子 PY1JDB20、初始房间，RDP 保持连接。

本轮整体目标：通过现有 `accept-t09-b-live.py` 一次入口执行 Host Authority + VM Remote 的 T0-09-B P1/F0，使用最新校正源码纯编译产物，检查真实安装、10,000 Authority tick、Wire 输入/ACK/STATE、双端停止与实际 catalog 恢复，并保存失败诊断或通过证据。不进入原版 Online/Isaac Sync，不自动重试、不启动/终止游戏或强制卸载驻留模块。

此入口默认运行基础 51 槽 C/S catalog，不配置可选 world observer 的 19 槽；因此该次实机结果不覆盖 70 槽 world catalog。VM 当前 Remote 实现不应用世界 snapshot，本次仍不证明 Replica 或双端世界相等。F1–F5、完整 T0-09 矩阵不纳入本次 F0。

## 执行方式

统一入口只调用一次，过程中不另行运行 inspect/probe/smoke/自测或离线回归。编译是该入口内的纯编译。用户确认已收到开始通知，VM 脚本就绪后在相同 RDP 会话执行一次，保持游戏环境不被手动操作。

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-05-live-01 `
  --authorization '2026-10-21 user requested live testing after hook calibration and confirmed both restarted games ready offline, Isaac, seed PY1JDB20, initial room, RDP connected; proceed host+VM F0 once with Native/Helper, owned B, relay and normal stop/restore' `
  --vm-share 'D:\projects\isaac_hook_reserach\cs-t0\vm_share' `
  --vm-guest-share '\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share' `
  --await-b-seconds 900
```

现有入口中的 run-id/内部 tx 标签沿用实现身份 tx-03-02；本次实际执行归属 tx-03-05，由本记录、独立产物目录和上述用户授权说明对应，不改写旧产物。

## 结果

### 实际执行

上述统一入口只执行一次。用户在脚本生成后确认已于 VM 执行一次：

`\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share\tx-03-02-F0-live-20261021T073209Z\start-vm-endpoint.ps1`

运行标识 `tx-03-02-F0-live-20261021T073209Z`，实际归属本 tx-03-05。纯编译成功；orchestrator 退出 **0**、编排结果 PASS，但最终 analyzer 为 **33/38 FAIL**，统一入口退出 **1**。没有第二次验收或分析器重跑，没有通过修改规则把失败改判。

### RUNTIME_OBSERVED：本轮已通过的实际行为

- Host PID **42924**、VM PID **3372**，双端 READY/ARMED 后 RUN_START，STOP_COMPLETE/RETIRE_COMPLETE 均完成。Host 初始化不再停留在旧 `Error::Hook`。
- Host authority `runStartTick=4795`，目标终点 `14795`。实际连续 ACK 与对应 gap STATE 各 **10,024** 条，覆盖 `4796..14819`，两个序列逐条一致。
- VM 发出 **10,017** 条唯一递增 B INPUT，sequence `1..10017`。属于目标窗口的 **9,986** 条 byte admission 与分析器预期 9,986 逐条一致，edge admission 1；处理边沿 exactly-once 检查通过。不能因最终 tick 不符而声称准入数量本身不符。
- Host 保存 **957,236** 条 native event、**640,685** 条 loop event，`event_overwritten=0`；双端 Wire overwritten 均 `[0,0]`，bridge rejected=0。
- relay 正常 STOPPED，A→B 转发 **20,050** 包、B→A **10,018** 包；丢弃/重复/拒绝/sendErrors 均为 0。
- Host `native_error=0`、`native_restored=true`，实际 catalog **51/51** 逐槽恢复、`same_process=true`、`stop_success=true`。本轮未启用 world 19 槽，不声称 70 槽实测。
- 双端 driver 退出、endpoint retired，游戏均存活；Native DLL 按 NEVER/常驻设计保留。orchestrator 记录 Host endpoint 退出 0、`forcedTermination=false`，relay 已退出。未强制终止游戏或卸载 DLL。

### 五个失败项

| 原检查名称 | 实际证据 |
| --- | --- |
| `wire.10000-authority-acks` | 实际 10,024 条，要求 10,000 条。 |
| `input.b-end-at-authority` | 期望末 tick 14795，实际发布到 14819。 |
| `input.final-progress-complete` | accepted=processed=9986 与预期数量一致，但最终 ACK 的 server_tick=14819，不是 14795。 |
| `control.heartbeat-final` | A/B 最后心跳均为 `(14819,true)`，要求 `(14795,true)`。 |
| `process.identities` | 此检查同时要求 Host 最终 heartbeatTick==endTick；实际 14819，故失败。两端 gameProcessAlive、VM dllResident 和 Host same_process 均为 true，不把该检查名解释为已观察到 PID 改变。 |

### 源码阅读推导与未完成

`AuthorityWireLink::Publish` 在 tick 达到终点后设置 TargetReached/ScriptComplete，但只要 Wire RunState 仍 Running，就继续发送 ACK/STATE；控制器 STOP 到达前的后续模拟仍可继续发布。这条源码路径与实测“终点后多 24 tick”相符。下一次实质修正应处理目标 tick 后的 publication/heartbeat 边界及正常停止顺序，保持游戏 Authority 日常模拟不等待控制器；本轮未修改该行为、未重测，也未放宽分析器终点要求。

本次只证明 P1/F0 的部分链路及 51 槽安装/恢复，没有完整 F0 PASS；T0-09-B 与 T0-09 均保持 IN_PROGRESS。完整 world observer、Replica/世界相等、F1–F5 和动态 Steam vtable ABI 不在本次实测范围。

### 原始产物与状态

- `artifacts/cs-t0/tx-03-05-live-01/acceptance-result.json`：统一入口退出及总状态。
- `analyzer/result.json`、`analyzer/checks.jsonl`、`analyzer/first-failure.json`：33/38 和五个原始失败。
- `orchestrator-result.json`：双端编排、停止、退休、游戏 PID、relay 退出记录。
- `host/result.json`、`vm/result.json`：双端实际状态与 Host 51 槽恢复。
- `host/events.jsonl`、`host/loop-events.jsonl`、双端 `wire.jsonl/control.jsonl/endpoint.jsonl`：原始时序和包。
- `relay/stopped.json`、双端 `bridge.json`：零丢失及正常关闭证据。
- `build/`、`source/`、`provenance.json`：本次构建与源码/文件长度记录；不含哈希校验。

VM 本次 bridge 实际 UDP 本地端点为 `192.168.137.9:59006`；旧入口环境记录仍填旧 VM IP `192.168.137.148`，网络实际事实以本次原始 bridge/control 为准，不改写其生成记录。

失败工作目录与共享产物按入口策略保留，未删除诊断或重新注入。后续测试前需要正常重启两端游戏以清除本轮驻留 Native/session；这不是让当前已恢复 DLL 强制卸载。
