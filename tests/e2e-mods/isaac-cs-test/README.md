# isaac-cs-test（临时 E2E 夹具）

tx-04-08 扩展及后续[ABC实现](../../../docs/design/tx-04-08/tx-04-08-abc-implementation.md)：复用同一F8队列，world_fixture已接具体七类Entity/Grid参数和原始episode索引。特殊Player Replace资格仍UNKNOWN/默认禁用，具体参数与原生覆盖均未实机验收。tx08关闭自动菜单Host，world真正Recording后才由harness Host/Join创建B；两次启动使用不同fixture token。下文tx07约定保持原范围。

本目录是 tx-04-05/06/07 的临时测试夹具，不是产品 gameplay mod。tx-04-07 的公共入口、屏幕日志和事件只由
[`tools/harness`](../../../tools/harness) 的部署流程启用；harness 每次运行都会复制本目录到游戏 `mods/isaac-cs-test`，
写入本次 `harness_config.lua`，结束后删除。

## 配置

`harness_config.lua` 由 harness 生成。tx-04-07 使用以下字段（示例见源文件的注释）：

| 字段 | 含义 |
| --- | --- |
| `tx` | 必须为 `"tx-04-07"` 才启用本节动作与激光观测；`tx-04-05/06` 保持旧行为。 |
| `token` | 本次运行标识；所有 tx07 记录都带该 token，harness 只归约同一次部署的记录。 |
| `host_port` | 首轮主菜单自动 `cs host` 的端口。 |
| `menu_host` | 默认 `true`。进程内只成功自动 Host 一次；S20 仍由控制口 Host。 |
| `expected_b_player_type` | 期望的 B 角色，默认 `PlayerType.PLAYER_ISAAC`（0）。 |
| `actions` | 动作表；字符串项或 `{ name, timeout_ms }` 表项均可。 |
| `observation_heartbeat` | 静止时每 N 次渲染写一条 `observation`。 |
| `laser_event_cap` | `laser-spawn` 事件上限；达到后写一次 `laser-overflow`。 |
| `animation_timeout_ms` | 复活后等待 `IsExtraAnimationFinished()` 的观察上限。 |

默认动作序列与场景一致：`skip → room → kill_b → revive_b → stage → room`（S6/S9/S10/S11/S12/S15）。
每个动作按 F8 执行下一项；同一时间只允许一个 pending。每次 F8 请求（包括被拒绝的非法请求）都分配
`action_id`/`request_id`，正常执行的动作另有 `step`（配置表 1 基下标）。动作完成前再次按 F8 会写一条
`phase=rejected, reason=pending` 的独立请求，不推进动作序号。动作失败不重试；`rejected`/`timeout` 之后
夹具停止接受后续动作。

超时以 `Game:GetFrameCount()` 计（Isaac 文档为 30 帧/秒，不含暂停），`timeout_ms` 在启动动作时换算成帧。
`skip` 由 30 次 `MC_PRE_UPDATE` 返回 `true` 推进；渲染层只做兜底超时。

## 动作

| 动作 | 行为 | 成功证据 |
| --- | --- | --- |
| `skip` | 连续 30 次 `MC_PRE_UPDATE` 跳过更新，下一次允许更新前记录结束。 | `action phase=arrived`，带 `skipped=30`。 |
| `room` | 按 DoorSlot 顺序选择已开启、未上锁的普通相邻门，调用 `Game:StartRoomTransition`。 | 房间身份变化后的 `action phase=arrived`，带 `source/target/actual/target_matched`。 |
| `kill_b` | 先确认 A 健康、B 为 index=1 的完整普通角色，再调用一次 `B:Kill()`。 | 原生 `dead`（若可观察）与 `ghost` 各自记录，随后 `action phase=arrived`；不补造未观察到的 dead。 |
| `revive_b` | 只对已确认的 B ghost 调用一次 `ReviveCoopGhost()`。 | `b-revive phase=return` 记录返回值；`healthy` 记录实际健康/身份；随后独立记录 Appear 动画完成。 |
| `stage` | 调用 `Game():StartStageTransition(false, 0, nil)`。 | `stage phase=call_return`，随后 stage/room 身份变化的 `action phase=arrived`，并记录 new_level/new_room 回调次数。 |

`kill_b` 的意外自动复活、B 移除、身份变化、调用异常都写 `action phase=rejected` 并单列 `reason`。
`revive_b` 返回 `true` 只代表 Lua 包装调用返回；`dll_rebind` 固定为 `pending`，稳定重绑由 DLL 状态/trace 判定。
动作事件里的 `before/after` 是只读 Player 快照，包含人数、房间身份、坐标、生命字段、controller、
`dead/ghost`、`IsExtraAnimationFinished()`；`GetBlackHearts()` 按位掩码记录为 `black_heart_mask`，不当作黑心数量。

3.4 配对新增 `movement_options`：对四个轴向的 10–80 像素路径逐点读取房间边界与格子碰撞，harness 从当前可通行方向选取移动并保存同一方向作判定。`run-exit` 分别记录 `should_save` 与退出请求时的 `snapshot`；`run-started` 记录实际 `continued` 与 Join 前恢复人数/controller，不将回调等同于 DLL 稳定绑定或磁盘写入成功。

## 激光观测

- `MC_POST_LASER_INIT` 记录每次新激光出生；这是主要证据，不依赖渲染帧存量。
- 每次进入新房间/新层/新 run，`laserEpoch` 递增并清空 init 去重表；首次扫描房间内已有激光单列为
  `laser-baseline`，不与出生事件混同。
- 每个出生事件带 `laser_id`（`epoch:serial`）、`ptr_hash`、`InitSeed`、frame/loop、`SpawnerEntity`/`Parent`
  的可读引用与 `attribution`（`A`/`B`/`UNKNOWN`），以及当时 A/B 的生命身份。归属链不明时保持
  `UNKNOWN`，不强行归 B。
- 超过 `laser_event_cap` 时写一次 `laser-overflow`；不静默丢弃。

## 日志

每行格式为 `[isaac-cs-test] tx07 {json}`。主要 `event`：

| `event` | `phase` / 字段 | 用途 |
| --- | --- | --- |
| `action` | `request` / `arrived` / `rejected` / `timeout` | 动作生命周期；每个动作只产生一个终态；`rejected`/`timeout` 带 `fixture_error=true`。 |
| `b-kill` | `request` / `dead` / `ghost` | B 死亡窗口的独立观察。 |
| `b-revive` | `return` / `healthy` / `animation_finished` / `animation_timeout` / `reghost` / `animation_lost` | 复活、健康、动画与回退观察。 |
| `stage` | `call_return` | `StartStageTransition` 已调用。 |
| `level-callback` | `new_level` / `new_room` | 每次加载边界；若正有 stage/room 动作，附带 `action/action_id` 并计入窗口计数。 |
| `laser-baseline` | `epoch`, `room`, `players`, `count`, `lasers` | 房间内已有激光快照。 |
| `laser-spawn` | `laser_id`, `laser`, `players` | 新激光出生与归属。 |
| `laser-overflow` | `cap`, `epoch` | 事件上限。 |
| `observation` | `player_count`, `players`, `room` | 周期/变化时的只读 Player 快照。 |
| `menu-host` | `response` | 首轮主菜单自动 Host。 |
| `fixture-error` | `reason` | 夹具自身错误；出现后不再执行动作。 |

所有记录都是只读观测加显式测试动作（Kill/Revive/Stage/Room）；夹具不写 Player 字段、不修改 Door、
不终止或跳过 Appear、不调用 `MorphToCoopGhost`。
## tx-04-08 ABC 后续实现

world_fixture 的具体参数、位置/空格/标签保护、PathTrail作用域、真实MC_POST_RENDER视图和原生episode归约见[ABC实现说明](../../../docs/design/tx-04-08/tx-04-08-abc-implementation.md)。默认scenario已启用七类Entity（Player仍由原生新局/Host/Join）和Grid覆盖/移除；特殊Player Replace仍UNKNOWN/禁用。只编译，未运行Lua或游戏；上方早先通用夹具记录按原时点保留。

