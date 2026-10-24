# tx-01-98：T0-07操作9结果与复现

日期：2026-10-20（Asia/Shanghai）。**操作9已完成一次真实游戏 Authority 的 B 输入停发实验。最终有效 invocation 为 `tx-01-98-source-stop-e2e-04`：统一报告 24/24 PASS，退出码 0。用户随后基于操作1–8已有记录与本操作9结果确认 T0-07 整体 PASS。** B 在真实已绑定 session 中停发 1/5/30/120 Authority tick；A/B 绑定、Authority tick、双玩家 resolve/receipt、标准/odd Native 边界、B 中性、edge exactly-once、120-tick timeout/recovery、A 物理动作、世界字段和 70 槽恢复均由同一 run 的实际事件/世界记录检查。脚本停发不冒充真实网络断线。

## 1. 最终运行

- 目标：批准的 unpacked `isaac-ng.exe`（PE timestamp `0x69E6E3A7`，5 sections，SizeOfImage `0x8FE000`，EntryPoint `0x6EFC46`）。实际运行是全新 New Run、seed `NJ30 ZKHY (1843139929)`、Start Room `1.2`，无 Room/Pause/Loading/Death/Rebind lifecycle 事件进入测试窗口。
- 配置：显式 IPC v5；`input_lead_ticks=2`、`future_window_ticks=64`、`max_input_age_ticks=0`、`script_drain_budget=64`、`admission_attempt_budget=258`。EventKind36 显示 Native 实际冻结同一组值。
- A：使用授权物理键盘工具，在延迟 controller 启动前已开始的 bounded `W/D/S/A` 中心方形往返。事件中 A 有四个方向状态，四段停发窗口都有非零 A resolve；同一 run 的 world record 记录 A Player 位置变化。
- B：原生选择角色，`Joined` 于 tick0，generation 1；脚本完成 tick355，最后 accepted target 357；最终 target 实际 effective 后，统一入口再等一个 committed boundary（最后轮询 commit 366）才发送一次正常 `stop`。B 在 tick436 正常退休。
- controller `CAPTURE_CLOSED`，`restoredTargets=70`、`nativeRestored=true`、`transportFailure=null`、`nativeError=0`，controller exit0；PASS 时游戏进程仍存活，随后正常关闭。controller 运行约 8.15 秒。

最终机器结果：

```text
artifact: artifacts/cs-t0/tx-01-98-source-stop-e2e-04
status: PASS
checks: 24/24
exit: 0
automaticRetry: false
hashes: NOT_USED
```

## 2. 源停发与 effective 缺口

提交事件的 target 相对源事件 tick 的稳定差值为 **2**，与配置 lead=2 相同。每一段 source stop 都比对应 effective missing 提前 2 tick；开始前最后一个 accepted target 为 effective 缺口前一 tick，并在该 tick 实际 available，随后才进入中性。停止期间没有新 sequence，恢复的第一个命令为前一 sequence +1。

| 段 | source 停发 tick | 源长度 | effective neutral tick | 有效长度 | 缺失原因形状 | 恢复 |
| --- | --- | ---: | --- | ---: | --- | --- |
| 1 | 40 | 1 | 42 | 1 | FutureOnly ×1 | 新 target/sequence |
| 2 | 81..85 | 5 | 83..87 | 5 | NoTargetCommand ×4 + FutureOnly ×1 | 新 target/sequence |
| 3 | 126..155 | 30 | 128..157 | 30 | NoTargetCommand ×29 + FutureOnly ×1 | 新 target/sequence |
| 4 | 196..315 | 120 | 198..317 | 120 | NoTargetCommand ×119 + FutureOnly ×1 | 新 target/sequence |

B 共 201 个 accepted source commands（200 个脚本 step + startup 的 completed+1 命令），sequence 1..201 连续、generation 1、rejected B submits 0。分析窗口 tick1..357 中 **178 standard + 179 odd** 个已提交 Authority boundary；每个 tick 都有 A/B local receipt 和 commit/post-state 行。B 在所有停发 tick 的 `Resolve.state` held/edge/move/aim 全零，availability `effective_available=false`、`neutral_applied=true`。

120-tick 窗口实际达到 `silent_ticks=120`：1 次 `TimedOut`、1 次 timeout recovery、4 次 silence recovery；B 未因 timeout 退休，session 保持 Active。5 个 drop edge 被 accepted 且 processed_count 恰为 5；停发 tick 没有 edge，恢复首 tick 使用新命令的 edge。

## 3. A 动作与世界证据

| effective 窗口 | 长度 | A 非零 resolve tick | 窗口内 Player 位置样本 | 相对停发前位置最大变化 |
| --- | ---: | ---: | ---: | ---: |
| 42 | 1 | 1/1 | 1 | 6.51 |
| 83..87 | 5 | 3/5 | 2 | 13.94 |
| 128..157 | 30 | 28/30 | 15 | 63.94 |
| 198..317 | 120 | 107/120 | 60 | 69.70 |

A 的 217 个 partial Player 位置样本有 216 个相邻有效变化；四段停发均观察到实际位置变化，不是仅计数/query。`stable_run_and_room` 为 PASS，测试窗口内完全没有 A/B lifecycle 失效事件。

world record 在本次短窗口中产生 **218 个 capture records，全部为 partial `NOT_READY`，0 个 complete world snapshot**。Player 的 `position.x/y` 字段实际为 `VALUE`，A 和 B 各有 217 个位置样本；报告将 complete/partial/reason 分离记录，只把可用 Player 字段作为明确 partial 证据，不宣称完整世界相等。Native post-state 因该 partial 状态全部记录为 explicit `Gap`，没有被改写成 Complete。

## 4. 恢复

- 70 个 Native 目标从事件明细恢复：70 个唯一 `(action,query)`、`before==after`，另有 1 个 detail=70 的 catalog 行。
- `restoration.json`：`nativeRestored=true`、`pinned=true`、`dllResidentExpected=true`；DLL 按 NEVER 策略常驻，没有强制卸载或重注入。
- world consumer：`CAPTURE_CLOSED`、producer_closed=true、reader_done=true、consumed=produced=846、backpressure=0、failure=null。
- 进程：controller 正常退出 0；游戏 PID 在恢复后仍存活，随后正常关闭。options 在测试前备份并在测试后恢复。

## 5. 历史失败（原样保留，不改判）

- `tx-01-98-source-stop-e2e-01`：**FAIL，未开始 Active 测试**。误用 D-drive 的非批准 PE（timestamp `0x63914F0A`），Native 在 target image 检查返回 `Error::Image=2`；controller 在 helper initialize 阶段退出 1，0 events、0 captures、0 restoration。修正：统一入口新增批准 canonical/unpacked PE header preflight。
- `tx-01-98-source-stop-e2e-02`：**FAIL，20/22 PASS**。使用正确目标且完成 1/5/30/120 停发、timeout、恢复和 70 槽恢复，但 A 物理输入在约 tick200 才进入事件窗口，前三个 B stop 无 A 动作；同一 run 的 231 个 world capture 全为 partial `NOT_READY`，当时报告尚未把可用 Player 字段作为明确 partial 证据。修正：可选 `--start-delay-seconds`，先让外部 A 动作运行再启动 controller；world complete/partial/reason 分离。
- `tx-01-98-source-stop-e2e-03`：**FAIL，21/23 PASS**（该 artifact 的早期报告版本）。延迟启动已让 A 从 tick1 有输入，但持续 A/D 动作在注入前穿过门进入 room `1.1100`，A 于 tick185 发生 `Lifecycle::Death`，120-tick 窗口失去 A 绑定动作。修正：改用 bounded W/D/S/A 中心方形动作，增加 `stable_run_and_room` 检查；使用全新进程和 New Run。
- 最终 `-04` 使用同一生产实现和修正后的单一统一入口，重新执行一次完整实机验收；失败目录、事件、报告和旧检查结果均未覆盖或改判。

## 6. 未验证范围与限制

- 这是脚本源停发，不是真实网络断线、收包/ACK、重连或 wire timeout 测试。
- 只覆盖一次 New Run、start room、Isaac A、native B，以及当前配置/角色/房间下的标准与 odd Authority tick；不外推到所有角色、武器、敌人、死亡、换房、loading、Restore/Replace、OOM/EH/worker 并发。
- world record 本次无 complete world sample；完整世界比较、canonical snapshot equality、snapshot apply、rollback、late join 和 Dedicated Server 未验证。
- 未计算或校验任何哈希；未进入或复用原版 Online/Isaac Sync。
- controller 延迟只用于让授权外部 A 动作先于注入/脚本开始，不改变生产 Authority/lead/timeout 语义；实际配置、source gap 与 effective gap 均由生产事件验证。

## 7. 产物与复现

- [最终结果](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/report/result.json)
- [缺口/对齐证据](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/report/gaps.json)
- [A/世界证据](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/report/a-world-observation.json)
- [运行观测](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/report/runtime-observations.json)
- [controller/恢复结果](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/capture/result.json)、[restoration](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/capture/restoration.json)
- [原始 Native 事件](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/capture/events.jsonl)、[world captures](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/capture/world/captures.jsonl)
- [前三次失败及修正摘要](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/prior-attempts.json)
- [环境与游戏日志](../../../../artifacts/cs-t0/tx-01-98-source-stop-e2e-04/environment/)
- 工具和复现入口：[设计/失败方式](tx-01-98-source-stop-e2e.md)、[静态审阅](tx-01-98-static-review.md)、`cs-t0/build-authority-source-stop.ps1`、`cs-t0/accept-authority-source-stop.ps1`。

最终流程先创建新 build/artifact，纯编译 Rust Controller、Native 和 Helper，随后只执行一次统一实机入口。最终构建目录为 `out/cs-t0/tx-01-98-build-01`；复现时必须使用新 build/artifact 目录，不覆盖本次成功或历史失败产物。
