# RE-P6：时序对应与端到端计时

前半部分是代码核对（本项目 `mod/src` 与 J273 调度），后半部分是阶段 0 新增的计时。

## 1. Host 的 Authority tick

**Host 开插值**（Manager `+0x2A1FB` 为真）：

| Manager 帧 | Authority tick | 内容 |
|---|---|---|
| 偶帧 | StandardRoom | Game::Update → Room::Update → EntityList::Update：玩家半步 A，随后碰撞阶段 |
| 奇帧 | OddPlayer | Odd helper：玩家半步 B ＋ `PlayerCollideWithGrid(p, 0)` |

- tick 约 60Hz，每个输入命令（target_tick）只驱动一个玩家半步。
- `input_lead_ticks=2` 约等于 33ms，不是设计 1.1 原写的 67ms。

**Host 关插值**：没有奇帧 tick；一个 StandardRoom tick 内玩家用同一输入走两个半步，约 30Hz，提前量 2 tick 约 67ms。

代码位置：`mod/src/game/authority/tick.cpp` SimulationScope（StandardRoom）、BeginOddPlayer（OddPlayer），Manager 返回处累加 `ticks_standard`／`ticks_odd` 并 FinishPostState → PublishCompleted → PublishTick。`mod/src/core/participant.cpp` Begin(tick) 取 target_tick == tick 的命令，End(normal) 写 `last_processed_tick = visible_tick`、`last_processed_sequence`。

## 2. Body 采样时刻

- Body 在 Manager 返回处的 WorldPreparePostState（`world_bridge.cpp:661`，由 `ops/command.cpp:700` 调用）中采集，晚于 NativeReturned 与参与者 End()。所以采集时参与者的 `last_processed_sequence`／tick 正好对应这份 Body；偶帧样本已包含碰撞阶段。
- ACK（`peer.cpp:240—260`：server_tick、server_sample_ms、authority_kind、processed、processed_tick）出自同一个 Manager 返回窗口，但是另一条消息，到达顺序不保证。
- 段头（`wire_protocol.cpp:305—311`）有 stream、member_epoch、resource_epoch、base／target_version、source_revision、presentation_batch、game_frame、event_watermark 和计数，**没有** Authority tick、半帧相位和输入序号。game_frame（`Game+0x25A70`）在 Game::Update 中递增，同一游戏帧的偶帧与奇帧样本 game_frame 相同（读码推断），只能靠 source_revision 的先后区分。

结论：设计 5.7 的“每身体 `input_processed_sequence`＋半帧相位”确有必要。它是线格式改动，属 P3，不在阶段 0。

## 3. 客机输入目标

- `client.cpp` InputTiming：以 server_sample_ms／server_tick 为锚估计 rate_ticks／rate_ms；目标 = server_tick + ceil((本地经过 + RTT + publication_age + 不确定度) × 速率) + 2（`NextInputTarget` 加 `input_lead_ticks`）。
- 每观察到一次 server_tick 变化最多发一条意图。TickProgress 在 `server_tick % (60/timing_hz) == 0` 时发（timing_hz=60 即每 tick 一次），`publication_age_ms = send − server_sample_ms`（`live.cpp:162`）。

## 4. 阶段 0 新增计时（已实现，未运行）

不改线格式、输入目标、发送节奏或显示。Host 的 World／Body 不带发送时刻，所以“生效”以 ACK 覆盖该序号为准，“首次可见”以随后第一次 Body 发布进入原生为准。

**客机**（`mod/src/game/client/session/client.cpp` `InputLatencyProbe`），在流诊断第 0 页输出 `stream.latency.*`：

| 字段 | 含义 |
|---|---|
| `input_ack_us_{count,p50,p95,max}` | 意图发出到 ACK 的 processed 覆盖该序号（只计每次 ACK 的最新序号） |
| `lead_ticks_*`、`lead_ms_*` | 发送时 target − server_tick，以及按观测节拍折算的毫秒 |
| `onset_ack_us_*` | 移动起步（发出的意图由中性变为非零移动）到 ACK 覆盖起步序号 |
| `onset_body_us_*` | 移动起步到 ACK 之后第一次 Body 发布进入原生（同轮随后 Render）；上界近似 |
| `processed_early`／`processed_on_time`／`processed_late`／`processed_late_max` | ACK 的 processed_tick 相对该序号 target 的早、准、晚次数与最大晚到 tick 数 |
| `ack_ring_miss` | ACK 序号已被 128 项环覆盖或代次变化后清掉，未计时 |
| `onsets`／`onsets_dropped`／`onsets_expired` | 起步数；未完成即被新起步顶替；2 秒内没等到 ACK＋Body |

`onset_body_us` 的局限：Body 发布计数是全体身体共用的，ACK 之后的第一次 Body 未必含本机身体那一步；同一 Manager 窗口的 Body 若先于 ACK 到达，会多算一个 Body 间隔。计数只用于看量级，阶段 2 改线格式后可精确到序号。

**Host**（`tick.cpp` StatusFields）：新增 `native_interpolation`（0 关、1 开、2 不可读），与已有 `ticks_standard`／`ticks_odd` 一起确认 tick 频率。读取函数 `NativeInterpolationState()` 在 `game/common/player_api.cpp`。

构建：2026-11-19 仅编译产品 DLL `zhlIsaacCS`（`build/repentogon`，Release）通过；未运行任何测试。
