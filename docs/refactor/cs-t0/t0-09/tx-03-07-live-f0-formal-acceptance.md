# tx-03-07：T0-09-B 实机 P1/F0 正式验收（用户裁定）

日期：2026-10-21（Asia/Shanghai）。状态：**HOST+VM 双真实游戏 P1/F0 FORMAL PASS（用户裁定）；原始统一入口因 staging 清理返回 FAIL，该事实保留；T0-09 整体仍 IN_PROGRESS。**

## 1. 完整目标与执行边界

用户在本会话明确要求“现在开始实机测试”，并确认 Host 与 VM 两端游戏已准备到测试基线：offline/local、seed `PY1JDB20`、角色以撒、初始房间，RDP 保持连接。授权范围包括 Native/Helper 注入、owned virtual B、Host UDP relay、T9C1 control、只读采样和正常 stop/restore。

本轮仅执行一次现有唯一实机入口，未自动重跑，未启动或终止游戏，未进入原版 Online/Isaac Sync，也未执行 F1–F5、observer 状态比较、C0/C1/C3 或完整 T0-09 矩阵。

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-07-live-01 `
  --authorization "..." `
  --vm-share "D:\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --vm-guest-share "\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --await-b-seconds 900
```

运行标识：`tx-03-02-F0-live-20261021T100942Z`。该内部标签沿用实现身份，实际验收归属 tx-03-07 及产物目录 `tx-03-07-live-01`。

## 2. RUNTIME_OBSERVED：实机功能结果

| 项目 | 实际观察 |
| --- | --- |
| 拓扑 | Host Authority + VM Remote，两个真实 `isaac-ng.exe`；Host PID `46608`、VM PID `10348` |
| Orchestrator | `status=PASS`、`liveHostVmP1=true`、`realGame=ATTEMPTED`、`error=null`、退出码 0 |
| 实机 Analyzer | `LIVE_F0_PASS`，**38/38 PASS**，`failures=[]` |
| Authority 窗口 | `runStartTick=4553`、目标 `endTick=14553`；ACK 与 gap STATE 各 10,000 条，覆盖 `4554..14553` |
| heartbeat | A/B 最终 heartbeat 均为 `14553=true`，与 Authority 终点一致 |
| B 输入 | 发送 9,989 条唯一递增 INPUT；Authority byte admission 9,981 条；edge 1 次 |
| 最终进度 | `server_tick=14553`、`accepted=processed=9981`、`generation=7`、`rejected_count=0` |
| Relay | A→B 20,002、B→A 9,990；drop/duplicate/rejected/sendErrors 均为 0 |
| 双端停止 | 两端均 STOP/RETIRE，endpoint 已退休、driver 退出；Wire overwritten 均为 `[0,0]` |
| 进程存活 | Host 和 VM 游戏均存活；没有强制终止游戏或卸载常驻 DLL |
| Host 恢复 | `native_error=0`、`native_restored=true`、实际 catalog **51/51**、`same_process=true`、`stop_success=true` |
| VM 常驻 | `dllResident=true`、`gameProcessAlive=true`、`endpointRetired=true` |

## 3. 原始统一入口 FAIL 与用户裁定

统一入口写出的 `acceptance-result.json` 机械状态为 **FAIL / `t0_09_b=IN_PROGRESS`**。唯一错误为：

```text
WorkCleanup
[WinError 5] 拒绝访问。:
out\cs-t0\tx-03-07-live-01-work\build\Native\bin\IsaacCsT0.Native.dll
```

直接原因是 Host 游戏仍运行并映射该 staging 中的 DLL；项目采用 DLL 常驻（NEVER）策略，正常 stop/restore 不卸载 DLL。因此这是验收入口在功能成功后尝试删除仍被游戏映射的临时构建文件时产生的清理错误，不是 F0 功能失败、Hook 恢复失败或 DLL 异常卸载失败。

用户在审阅该结果与原因后明确表示：**“算正式通过，构建产物我自行删除”**。据此：

- 本次 host+VM 双真实游戏 **P1/F0 记为正式通过**，依据是 Orchestrator PASS、Analyzer 38/38 PASS 和用户对清理问题的明确裁定；
- 原始 `acceptance-result.json` 的 FAIL 保持原样，不篡改、不重跑，也不把清理错误伪写成功能通过；
- staging/构建产物由用户自行删除，本轮代理未删除；
- 该裁定只覆盖 P1/F0，不扩展为 F1–F5、状态相等、Replica 或完整 T0-09 PASS。

## 4. 阶段结论

- T0-09-B host+VM P1/F0：**FORMAL PASS（用户裁定）**。
- 原始验收脚本成功路径的 staging 清理语义仍是已知实现问题，待后续维护处理；它不是本轮功能结论的依据。
- T0-09 整体：**IN_PROGRESS**，仍需 T0-09-C/D 的 observer-simulator、F1–F5、C0/C1/C3、多 seed 矩阵和最终报告。

## 5. 证据入口

- [原始统一结果](../../../../artifacts/cs-t0/tx-03-07-live-01/acceptance-result.json)
- [用户裁定记录](../../../../artifacts/cs-t0/tx-03-07-live-01/user-adjudication.json)
- [Analyzer 38/38 结果](../../../../artifacts/cs-t0/tx-03-07-live-01/analyzer/result.json)
- [全部 38 项检查](../../../../artifacts/cs-t0/tx-03-07-live-01/analyzer/checks.jsonl)
- [Orchestrator 结果](../../../../artifacts/cs-t0/tx-03-07-live-01/orchestrator-result.json)
- [Host 结果](../../../../artifacts/cs-t0/tx-03-07-live-01/host/result.json)
- [VM 结果](../../../../artifacts/cs-t0/tx-03-07-live-01/vm/result.json)
- [Relay 停止摘要](../../../../artifacts/cs-t0/tx-03-07-live-01/relay/stopped.json)
