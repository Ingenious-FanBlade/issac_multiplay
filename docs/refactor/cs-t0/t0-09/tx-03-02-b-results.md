# tx-03-02：T0-09-B 离线传输/控制基础结果

日期：2026-10-20（Asia/Shanghai）。结论：**OFFLINE F0 FOUNDATION PASS / 41/41**；
**host+VM 双真实游戏 P1 = NOT_RUN**；T0-09-B 整体仍为 **IN_PROGRESS**。本轮没有启动游戏、
注入 Native/Helper、操作 Original Online/Isaac Sync，也没有计算或校验任何哈希。

## 1. 唯一有效整体验收

入口：

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-09 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

实际执行方式：受控后台 job，一次调用完成纯编译、一次离线 F0 runner、一次 analyzer。
无自动重跑。

结果：

- build exit **0**；F0 runner exit **0**；controller A/B exit **0**；relay exit **0**。
- analyzer **41/41 PASS**，status `OFFLINE_PASS`。
- 顶层明确记录 `t0_09_b=IN_PROGRESS`、`realHostVmP1=NOT_RUN`。
- artifact：`artifacts/cs-t0/tx-03-02-e2e-09/`。

## 2. 实际观察

| 项 | 实际值 |
| --- | --- |
| Authority gameplay tick | 1..10,000 连续；progress ACK=10,000、STATE=10,000、首 ACK=1 |
| A→B relay | received 20,002、forwarded 20,002、drop 0、duplicate 0 |
| B→A relay | received 9,853、forwarded 9,853、drop 0、duplicate 0 |
| relay | rejected 0、sendErrors 0、bootstrapCached 1、status STOPPED |
| B canonical INPUT | 9,852 records；sequence 1..9,852；target ≤10,000；edge 1 次 |
| A admission | 9,852/9,852 Accepted；edge processed 1 次；最后 accepted/processed=9,852 |
| Wire IPC A | produced/consumed `[9853, 20002]`；overwritten `[0, 0]`；closed 1 |
| Wire IPC B | produced/consumed `[9853, 20002]`；controllerHeartbeatTick 10,000；overwritten `[0, 0]`；closed 1 |
| control heartbeat | A 端最终 tick 10,000，target_reached；转发给 B 的最终 tick 10,000 |
| retirement | A/B STOP_COMPLETE + RETIRE_COMPLETE；driverExited/endpointRetired true |
| raw evidence | relay 25,379,350 B；host wire 18,903,913 B；vm wire 18,864,429 B |
| runtime | relay summary runtimeMs 154,099 |

A/B endpoint result 均为 PASS；staging work 在成功后清理。以上只证明离线两进程
Wire IPC/UDP/T9C1/Endpoint/F0 基础，不证明真实游戏、VM 网络、Native v7、Hook/恢复或
host+VM P1。

## 3. 41 项检查

analyzer 保存逐项 JSONL：`artifacts/cs-t0/tx-03-02-e2e-09/analyzer/checks.jsonl`。覆盖：

- manifest offline/F0/10,000 tick/lead；
- orchestrator 双 ARMED 后 RUN_START、target 后 STOP、双端 stop/retire；
- host/vm endpoint result、Wire overflow/overwrite；
- relay raw byte Counter 与 endpoint outbound 完全对应、F0 零 drop/duplicate/reject/error；
- 10,000 progress ACK 和 10,000 STATE tick 连续、sequence/session/role；
- B INPUT 单 record、sequence/target、edge 唯一；
- A Participant byte admission 全部 Accepted、edge exactly-once；
- B control heartbeat→canonical input 的 target/lead 对应；
- T9C1 每 role/方向的 seq/ack、双 READY/ARMED 屏障、heartbeat 转发与 stop/retire；
- 双端正常退出和 artifact 完整性。

## 4. 失败与修复历史

所有失败均保留原始 artifact，不把后续成功回填为旧 PASS：

- [启动失败 01](tx-03-02-b-launcher-failure-01.md)：`sys.path` 缺失，未 build/run。
- [启动失败 02](tx-03-02-b-launcher-failure-02.md)：分析器模块文件名不能用短横线 import，
  未 build/run。
- [验收失败 03](tx-03-02-b-acceptance-failure-03.md)：T09 binary 参数 `skip(2)` 错误；
  analyzer 残留 `separate_ascii`。未启动运行进程。
- [验收失败 04](tx-03-02-b-acceptance-failure-04.md)：manifest B endpoint `os` 超过 16 字节。
  未启动运行进程。
- [前台中断 05](tx-03-02-b-acceptance-interruption-05.md)：前台工具 120 秒窗口中断，实际到达
  A tick 7412；无完整结果。
- [验收失败 06](tx-03-02-b-acceptance-failure-06.md)：完整运行；orchestrator 将 Retired
  endpoint 的连接 reset 当失败，analyzer 误读 B `acks` 字段。39/41。
- [验收失败 07](tx-03-02-b-acceptance-failure-07.md)：A controller 快速退出导致最后
  STOP/RETIRE 未到达；analyzer 把 closed 审计行当 envelope。38/41。
- [验收失败 08](tx-03-02-b-acceptance-failure-08.md)：完整运行与 orchestrator 均 PASS，
  analyzer 把 `rolesRetired=true` 与整数 2 比较。40/41。
- e2e-09：修复后唯一 `OFFLINE_PASS` 结果，41/41。

## 5. 明确未完成

- 真实 host Windows + Windows VM 双游戏环境未启动或验证。
- 真实 `isaac-ng.exe`、Native v7 endpoint bridge、Helper 注入/停止、exact pair、observer
  local replay、hook 恢复、DLL 常驻和游戏进程存活均未接入本轮离线 runner。
- VM 网络 baseline delay/jitter/NAT 未测量；F1–F5 未执行；C0/C1/C3 未执行；完整多 seed
  矩阵与双端状态比较属于后续 T0-09-C/D。
- 因此不能宣称 T0-09 PASS，不能把本结果外推为玩家可玩同步或 Replica World。
