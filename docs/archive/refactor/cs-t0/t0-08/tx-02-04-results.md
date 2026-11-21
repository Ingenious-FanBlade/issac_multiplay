# tx-02-04：owned A 物理输入的唯一整体 E2E 结果

2026-10-20（Asia/Shanghai）。**T0-08 操作1–9 本轮唯一整体 E2E 为 33/33 PASS，result.status=PASS，controller exit0，未自动重跑。** tx-02-03-e2e-01 的输入接力超时失败原样保留；本轮由验收器自己持有物理输入会话。

## 实际结果

- 游戏 PID 14344，creation time `134369795345707102`，唯一可见 HWND `2556746`；全部初始与连续输入样本显示该窗口属于游戏前台。
- 初始物理输入 4 步成功、13 个前台样本全部通过；controller 启动后首个真实 commit 为 tick3，连续输入在 tick3 后立即开始，64 步 W/D/S/A 共 12.879 秒，结果观察时 commit 已到 tick914。
- `L04.physical-input-initial-focus`、`L04.continuous-input-window`、`L04.A-and-clock-continue`、`L04.A-native-source-retains-game-foreground`、`L04.A-real-partial-position-change` 全部 PASS。
- Native A 共 6269 个 PhysicalSample，6269/6269 前台属于游戏 PID；其中 646 个样本有非零移动量。A 在 outage 内实际 Resolve 非中性，真实 Player 位置采样发生变化。
- 协议 trace **56,070/56,070**，`produced=consumed=56070`、`lost=0`，12,780 个发送包配对无误，`packet-pairing.json.errors=[]`。
- 6,268 个真实 commit（tick1..6268）逐 tick 唯一；ACK/STATE 双 lane、真实 receipt 字段、远端接受与 world 身份配对全部 PASS。远端接受 ACK 4,967、STATE 5,063 条有效包。
- 固定 seed 故障：random loss 616、outage loss 60；B 在 outage tick170..179 中性，之后恢复；B 输入 711/711 从 wire 进入生产准入，222 accepted、487 幂等重复；3/3 Drop 边沿 exactly-once。
- 正常 stop：helper 成功，`nativeError=0`、`nativeRestored=true`、`pinned=true`、70 个不同恢复目标 before=after；`controllerError=null`、`cleanupErrors=[]`，同一游戏进程存活。
- 离线 corpus/生产 Participant 生命周期 **19,022/19,022 PASS**，17,373 条 corpus 向量。历史 trace-overflow/缺恢复文件的只读副本由同一入口验证，报告器按预期失败并保留 `protocol trace overflow`。
- 本次保存 389,537 条真实事件、56,070 条协议轨迹、6,268 个 commit、world 记录（partial 1、gap 3,135、complete 3,135）。完整原始材料约 460 MB。

## 产物

- [统一结果 33/33 PASS](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/result.json)
- [实机逐项检查](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/live-checks.json)
- [控制器结果](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/capture/result.json)、[70槽恢复](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/capture/restoration.json)
- [初始物理输入](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/physical-input-ready.json)、[连续输入请求](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/physical-input-continuous-request.json)、[连续输入结果](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/physical-input-continuous-result.json)、[输入窗口校验](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/physical-input-continuous-validation.json)
- [包配对摘要](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/packet-pairing.json)、[真实事件](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/capture/events.jsonl)、[原始协议轨迹](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/capture/protocol.jsonl)、[操作记录](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/capture/operations.jsonl)
- [实际调用/配置/游戏身份](../../../../../artifacts/cs-t0/tx-02-04-e2e-01/invocation.json)、`sources/`、`build/`、`corpus/`和全部离线原始记录。

## 结论与边界

- T0-08 当前范围（操作1–9：独立 wire、握手/input/ACK/STATE、严格 decoder/corpus、内存 FaultTransport、真实 Authority 接入与一次统一 E2E）判定 **PASS**。
- 该 PASS 的实机拓扑仍只是一个真实游戏 Authority 加同进程自有逻辑 Remote；没有 socket/Steam transport、两个游戏进程、真实网络断线、可玩 Replica、snapshot/baseline/delta apply、rollback、late join 或跨进程世界相等证据。这些属于 T0-09/后续工作。
- 本轮不计算或校验哈希，没有新增 gate。tx-02-03-e2e-01 的 `live=NOT_RUN` 失败、tx-02-02 的 29/31 FAIL、tx-02-01/00 的历史失败均保留，不重写。
