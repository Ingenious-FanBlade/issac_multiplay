# tx-04-05 实现方案：循环阶段、tick 判定、A 的统一输入与单人会话

日期：2026-10-24。状态：U1–U4 已由用户按建议确认（设计 D19）；第 10 节 T1–T8 的静态审查和主智能体审阅已完成，事实、修正和保留的 UNKNOWN 见[静态确认报告](../research/tx-04-05-static-confirmation.md)。随后完成第 3.1 节 [core 原型移植](tx-04-05-core-port.md)与第 3.2 节 [game 适配及必要 Hook/solo 接线](tx-04-05-game-adapter.md)，均纯编译链接成功。第 11、12 节的 Lua 夹具、单启动 S0–S11 harness 与 V1–V10 归约也已编写，见[实现记录](tx-04-05-test-harness.md)。第一次整体 E2E（2026-10-24）在 S0 启动阶段因 harness focus 时序 FAIL，未重跑；按 D20 修正 harness 后执行的第二次 E2E 跑完 S0–S11，V1、V3–V9 PASS，V2（S1 时游戏尚未开始模拟）与 V10（F9 触发原版截图）FAIL，未重跑；按 D21 修正后的第三次 E2E 中 V1、V3–V10 PASS，只有 V2 因 S10 向右被随机房间的岩石挡住而 FAIL，未重跑；按 D22 修正后的第四次 E2E 中 V1、V3–V10 PASS，V2 因 S10 改向左移动而判定仍要求向右（D22 修改遗漏）FAIL，未重跑。D23：用户决定将第四次 E2E 算作验收通过，tx-04-05 完成（用户接受），见[结果](tx-04-05-results.md)。依据：

- [REPENTOGON 适配重构设计](repentogon-refactor.md)（下称“设计”），尤其是第 5.4、7 节；
- [tx-04-04 实现方案](tx-04-04-implementation-plan.md)与[实现与结果](tx-04-04-results.md)（第八次 E2E 的计数与观察）；
- [v1.9.7.12 点位核对](../hooks/repentogon-binding.md)；
- [现行项目规范](../refactor/project-conventions.md)，尤其是第 6、8、9 节；
- 原型源码 `cs-t0/native/`：`authority_session`、`authority_tick`、`authority_tick_pipeline`、`input_core`、`coop_input`、`coop_active_runtime.cpp`。

本文属于 tx-04-05。按规范第 2 节，只在全部实现完成后执行一次 E2E（第 12 节），分析、逆向和实现期间不运行任何测试。2026-10-24 用户要求先完成所有静态确认，并指定 GPT-6 Luna / max 子智能体负责逆向、主智能体审阅结论；原始证据保存于 `artifacts/repentogon/tx-04-05-static/`。第 13 节列出的用户决定事项 U1–U4 已于 2026-10-24 全部按建议确认。

## 1. 目标、范围与边界

**目标。** 在 tx-04-04 已验证的常驻 Hook 框架上，让 Hook 从“只透传计数”变成“会话激活时按语义工作”：

- 用上下文栈识别循环阶段，按设计 7.3 判定 Authority tick（StandardRoom 与 OddPlayer 两种）；
- 本地玩家 A（ordinal 0）的物理输入只采样，经统一的 canonical / Submit / queue / Resolve 进入游戏，consumer 读到的是 Resolve 结果；
- 生命周期：进房、暂停、加载、退出到菜单等情况下，按规范第 6 节作废绑定、保持中性、退休 generation；
- 单人会话：在没有网络和 B 的情况下，会话真实地经过 `Warmup → Active → Retiring → Idle`。

**预期结果。** 一次 E2E 通过第 12 节的 V1–V10。

**本轮包含：**
1. 从原型移植不碰游戏对象的 core 模块（第 3.1 节）；
2. 新增 `game/` 主线程适配：上下文、tick、A 的采样与查询覆写、consumer 分派、生命周期；
3. 单人会话与对应的 `cs` 命令、状态字段和 trace；
4. 测试 mod 与 harness 的新场景。

**本轮不包含：**
- 网络、远端模拟客户端与 relay；虚拟设备 B；Join 调用点补丁（tx-04-06）；
- world_record（tx-04-07）；Replica 与状态同步；
- 界面 mod 的新页面。界面仍只显示状态，`cs host/stop` 仍是 tx-04-04 的占位语义。

## 2. 依据与已知事实

### 2.1 沿用的已采纳语义（规范第 6、9 节）

| 编号 | 语义 | 本轮落点 |
| --- | --- | --- |
| 第 6 节 | A 固定 ordinal 0；A/B 共用 canonical、sequence、target tick、queue、dedupe、Resolve；房主没有即时旁路；缺输入中性；room、death、设备重分配使临时绑定失效；teardown 退休 session 与 generation；pause、loading、menu、未知绑定下保持中性；不把顶层 loop 或渲染帧当 tick | 第 4–7 节 |
| tx-01-45 | Authority-pre 发布 current，原函数正常返回后提交 previous；Poll 只采样，不写快照；缺输入立即中性（`max_input_age_ticks=0`） | 第 5.2、6.2 节 |
| tx-01-50 | 奇数帧 helper 的 Player 更新也是真实模拟；在首个正常 Main Player 更新前执行 Authority-pre，helper 正常返回后提交；特殊暂停/FSM 更新不推进 scheduler | 第 5.3 节 |
| tx-01-51 | 启动例外：重新唯一绑定后，首个新鲜输入可用于 completed+1，同一样本的连续量另以 lead=2 排入 completed+2，edge 清零 | 第 6.3 节 |
| tx-01-52 | 射击相反方向后按者优先，按轴保存胜出方向；无法区分先后时保留旧胜出方向；没有历史时固定右/下 | 第 6.2 节 |
| tx-01-54 | 没有 Authority tick 或有 pause/loading/lifecycle 阻断时，主动暂停原生射击 consumer；正常 tick 的松键与缺输入仍执行中性消费 | 第 6.5 节 |

### 2.2 tx-04-04 第八次 E2E 的计数（运行时观察）

`artifacts/repentogon/tx-04-04-e2e-08/counters.json`，在退回菜单之前采集，覆盖主菜单与约 30 秒 run：

| 点位 | 次数 | 解读 |
| --- | --- | --- |
| `Manager::Update`、`Manager::ExecuteStartGame`、InputManager 更新、设备公共更新、Keyboard Poll | 1425 | 每个 Manager 帧各一次。`ExecuteStartGame` 每帧都被调用，只是外层 owner，不是“开局事件” |
| `Game::Update`、奇数帧 helper | 397 / 397 | run 内两者交替，各占一半 Manager 帧（静态推断，与原型 tx-01-50 的模型一致） |
| 控制台判断 `0x287020` | 794 | 等于 397×2：`Game::Update` 和奇数帧 helper 各调用一次 |
| `Room::Update`、`EntityList::Update`、MC_POST_UPDATE 调用函数 | 258 / 258 / 257 | 比 `Game::Update` 少 139 次，对应浮层、暂停等主体被跳过的帧 |
| `Entity_Player::Update`、六个 consumer | 各 605 | 普通 Room 更新与奇数帧 helper 中的 Player 更新之和 |

这些计数说明：只按 `Manager::Update` 或 `Game::Update` 的次数计 tick 都是错的，必须看普通 `Room::Update` 与奇数帧 helper 中的 Player 更新（设计 7.3）。

### 2.3 v1.9.7.12 上 `Room::Update` 的调用方与上下文（静态，binding 第 4 节）

| 原型编号 | 调用点 → 目标 | 所在函数（外层 owner） | 语义 |
| --- | --- | --- | --- |
| #15 | `0x2F78DF → 0x3F7F60` | `Game::Update`（owner `Game`） | 普通 Room 更新，唯一会产生 StandardRoom tick 的路径 |
| #24 | `0x509B26 → 0x3F7F60` | 房间生命周期 FSM `0x509100`（owner `Lifecycle`），由 `Game::Update` 调用 | FSM 更新，不算 tick |
| #26 | `0x2FA17C → 0x3F7F60` | `0x2FA0F0`（owner `NestedDoor`），调用方为 `RoomTransition::ChangeRoom`、`Console::RunCommand` | 门切房，不算 tick |
| #11 | `0x532EAA → 0x3F7F60` | `Manager::ExecuteStartGame`（owner `Start`） | alternate/loading 路径，不算 tick |

因此“栈顶是 `Game`”可以唯一识别已分类调用链中的普通 Room 更新，不需要返回地址。T1 已在 v1.9.7.12 上确认奇数帧 helper 的三处 Player 更新：`0x2F93F9/0x2F949C` 为特殊路径，`0x2F9559` 为普通点，采用第 3.3 节调用点 shim 分类。

### 2.4 其他相关事实

- **浮层即暂停（`SOURCE`）。** REPENTOGON 打开浮层时把 `g_Game->GetConsole()->_state` 设为 2，以“诱发游戏暂停”（`repentogon/ImGuiFeatures/ImGui.cpp:306-326`）；控制台方法 `0x287020` 据此让 `Game::Update` 和奇数帧 helper 提前返回。该方法还更新控制台动画与消息状态，必须只观察原调用的结果，不额外补调。暂停菜单和失焦是另外的路径，不能从控制台返回值推断所有暂停。
- **暂停菜单（`STATIC_CONFIRMED`，tx-04-04 第七次前）。** `Game::_pauseScreen` 位于 `+0x2363C`，`state` 在 `+0`、`selectedelement` 在 `+8`（`artifacts/repentogon/tx-04-04-pause-static/README.md`）。
- **失焦暂停。** 用户选项为 `PauseOnFocusLost=1`（tx-04-04 快照）。T5 已确认 Game/Odd 的焦点分支：Manager `+0x4AFAA` 为 0 且选项 `+0x2A1D7` 开启时，原生相应路径打开 PauseScreen 或跳过普通更新。控制台判断与该分支分开；实际事件/取值仍待唯一 E2E 观察，以普通调用点实际到达为准。
- **玩家列表（`SOURCE`）。** `PlayerManager._playerList` 是 `Game+0x1BAA8` 处对象 `+0x0` 的 `vector<Entity_Player*>`（SDK `IsaacRepentance.h:7623`）。Player 的 `_playerIndex`、`_controllerIndex` 分别在 `+0x1610`、`+0x160C`（binding 第 7 节）。
- **输入查询（`SOURCE`）。** SDK 有 `InputManager::IsActionTriggered/IsActionPressed/GetActionValue(action, controllerid, unk)`；REPENTOGON 自己在菜单相关代码里 Hook 了前两个（`LuaMenuManager.cpp:191-199`、`ModsMenuStuff.cpp:130`）。设备层查询是 Keyboard/Gamepad 第 8/9/10 槽，tx-04-04 已常驻指针补丁。

## 3. 工程改动

### 3.1 core/：从原型移植

| 原型文件 | 移植后 | 调整 |
| --- | --- | --- |
| `authority_session.*` | `core/session.*` | 命名空间改为 `cs::core`；状态与原因码保留，增加 `Solo` 模式标记（第 4 节）；去掉 Rust 控制器相关原因的生效路径但保留枚举值，便于对照历史 |
| `authority_tick.*` | `core/tick_clock.*` | `TickBoundary` 去掉 `return_rva`/`player_return_rva`，改为 `owner_kind` 与 `scope_token`（上下文栈提供）；其余判定（Reentry、DuplicateLoop、WrongReturn 等）保留 |
| `authority_tick_pipeline.*` | `core/tick_pipeline.*` | 去掉 post-state/world 相关字段的生效路径（world_record 在 tx-04-07）；Drain/Simulating/Committed/Published 流程保留 |
| `input_core.*`、`coop_input.*` | `core/input_core.*`、`core/participant.*` | Participant、canonical codec、ShootingDirections、Resolve 原样移植；去掉对 `raw_apply_runtime` 的依赖（第 6.2 节） |

core 只用 C++20 标准库，不包含 REPENTOGON 或游戏头文件（设计 3.2）。原型中依赖共享内存 IPC、Rust 控制器、world 采集、raw apply 的部分不移植。

### 3.2 game/：新增主线程适配

| 文件 | 职责 |
| --- | --- |
| `game/context.*` | 扩展 tx-04-04 的上下文栈：每个外层 owner 压栈时分配 `scope_token`；提供“栈顶是否为 `Game`”“是否位于奇数帧 helper 内”等查询 |
| `game/tick.*` | 把 Hook 事件映射为 TickClock / TickPipeline 调用（第 5 节） |
| `game/topology.*` | 读取 PlayerManager 玩家列表、A 的 Player 与设备的配对（第 6.1 节） |
| `game/input_a.*` | A 的物理采样与 Submit（第 6.2、6.3 节） |
| `game/query.*` | 第 8/9/10 槽 wrapper 的覆写逻辑（第 6.4 节） |
| `game/consumers.*` | consumer 上下文与射击分派（第 6.5 节） |
| `game/lifecycle.*` | 阻断状态与作废（第 7 节） |

真实 Solo 已接入 `game` 持有的 `core::Session`；`ops/command.cpp` 调用主线程适配并发布状态。`cs host/stop` 保留单独的占位对象与原有语义，和 Solo 互斥。七个模块及必要接线已编写并纯构建成功，见[实现记录](tx-04-05-game-adapter.md)，尚未实机验收。

### 3.3 Hook 清单的变化

- 现有入口绑定继续使用；新增点仍只经统一 Hook 清单注册，常驻并按会话逻辑启停。静态阶段没有安装或修改 Hook；随后已完成本节代码接线并构建，但没有在游戏进程中安装或观察。
- `Entity_Player::Update` 的 priority 从 0 改为 -10000，排在 REPENTOGON 的 MC_PRE_PLAYER_UPDATE Hook 之外；另在同一已有定义上加 priority=+10000 的只观测内层 Hook，记录是否到达原生 Player 子类更新。外层正常返回但内层未到达时记录链路跳过，不能由“consumer 次数为零”推断 Lua 返回了真（第 5.5 节）。
- T1 确认奇数帧 helper 中有三处 Player 虚调用。采用方式 4：只在普通调用点 `0x2F9559` 建立 `OddNormal` 标记；两处特殊调用不产生 tick。ZHL 在 wrapper 前另发出 `call`，不在 C++ wrapper 中用 `_ReturnAddress()` 判断原生调用点，也不依赖未约定的桩帧布局。指令搬移与异常清理约定见静态报告 T1。
- T4 确认 REPENTOGON 会在调用 `super()` 前短路返回。首版采用已观察的控制台与暂停区间，不新增用于“设备为真、最终为假”差值比较的两个 InputManager Hook；内层未执行不能被记成设备返回假。

### 3.4 注释

按 AGENTS.md，每个功能模块、核心函数和数据结构写中文注释；从原型移植的英文注释改写为中文，保留原有的不变式说明。

## 4. 单人会话

```text
Idle --cs solo start--> Warmup --A 唯一绑定且处于 run--> Active
Active --cs solo stop / run teardown / 失败--> Retiring --在途 tick 收尾、退休 generation--> Idle
```

- **只有 A。** Participant 0 绑定 A；Participant 1 不初始化，视为不存在（不是“缺输入的 B”）。
- **generation。** 每次 `cs solo start` 生成新的 session id 与 generation；A 的每次重新绑定递增 participant generation。
- **启动时机。** 只在 run 内（`run_active=1`）接受 `cs solo start`；主菜单发出时返回 `error=not_in_run`。
- **Warmup。** 每个 Authority 入口刷新拓扑；A 唯一绑定后转 Active（原型 `Activate(host_bound, …)`，单人时只要求 A）。Warmup 期间不分配 sequence、不 Resolve、不推进 scheduler。
- **退休。** stop 请求不再开始新的 tick；已经进入的 tick 走完正常返回与发布；之后关闭输入、作废绑定、退休 generation，回到透传。
- **与 `cs host` 的关系。** `cs host/stop` 仍只切换 tx-04-04 的占位状态；两者互斥，同时请求时返回 `error=busy`。tx-04-06 引入网络后，`Hosting` 才接到真实握手。命令名 `cs solo start/stop` 已由用户确认（U1）。

## 5. 上下文与 tick 判定

### 5.1 外层 owner

| owner | 压栈的 Hook | 说明 |
| --- | --- | --- |
| `Manager` | `Manager::Update` | 顶层；入口执行命令队列，出口为 TopExit |
| `Input`、`Device` | InputManager 完整更新 `0x5EDCC0`、设备公共更新 `0x5EC900` | 标识采样来源与同一设备的 Poll 结果；只作线程局部上下文，不进入 canonical 身份 |
| `Start` | `Manager::ExecuteStartGame` | 每帧都会进入；其中的 `Room::Update` 是 alternate/loading 路径 |
| `Game` | `Game::Update` | 普通 gameplay owner |
| `Odd` | 奇数帧 helper `0x2F9340` | Player-only 模拟 |
| `Lifecycle` | 房间生命周期 FSM `0x509100` | |
| `Door`、`NestedDoor` | `RoomTransition::Update`、`0x2FA0F0` | |
| `Restore` | `Game::RestoreState` | |
| `Render` | `Manager::Render` | 不推进 tick |
| `Join`、`Console` | Join pass、`Console::RunCommand` | 本轮只用于分类 |

每次压栈分配递增的 `scope_token`；出栈时校验 token。异常越过边界时由 RAII 出栈，并让当前 tick 作废（设计 5.4）。

### 5.2 StandardRoom tick

1. `Room::Update` 入口，且栈顶为 `Game`、会话为 Active、主线程、没有阻断：Authority-pre。依次 BeginDrain → A 的 Submit（第 6.3 节）→ BeginSimulation，发布 current；TickClock 进入 `Current=candidate`。
2. 其中的 Player 更新与 consumer 读取 Resolve 结果。
3. `Room::Update` 正常返回：`NativeReturned`，提交 previous（tx-01-45）。
4. `Manager::Update` 出口（TopExit）：先沿用 pipeline 的 `FinishPostState` 收束步骤（本轮 post-state 为 Disabled，不采集世界），由该步骤提交 completed，再 `PublishCompleted`，写 `tick` trace 和状态快照。不能直接调用只发布 receipt、并不推进 clock 的 `PublishCompleted`。

同一个 `Manager::Update` 内出现第二次 StandardRoom 入口，按原型记为 `DuplicateLoop`。栈顶不是 `Game` 的 `Room::Update` 一律不开始 tick。

### 5.3 OddPlayer tick

1. 奇数帧 helper 入口压栈 `Odd`，记 `player_started=false`。
2. helper 内普通调用点 `0x2F9559` 的 shim 建立 `OddNormal` 标记；只在该标记对应的首个 A Player 更新入口执行 Authority-pre，kind 为 `OddPlayer`。特殊调用点 `0x2F93F9`、`0x2F949C` 即使进入同一个 Player Hook，也不开始 tick。
3. helper 正常返回：若已开始，`NativeReturned` 提交；TopExit 发布。

### 5.4 不计 tick 的情况

不需要额外判断，以下情况都不会满足第 5.2/5.3 节的入口条件：
- 控制台判断为真，包括 REPENTOGON 浮层打开（第 2.4 节）；
- REPENTOGON MC_PRE_UPDATE 跳帧（`Game::Update` 内 `0x2F77A4` 直接跳到尾声）；
- 暂停菜单、失焦暂停，以实际未执行普通 `Room::Update` 为准；
- 加载与切房：`Room::Update` 位于 `Lifecycle`、`Door`、`NestedDoor`、`Start` 之下；
- 会话不是 Active，或不在主线程。

奇数帧 helper 在 MC_PRE_UPDATE 跳帧期间是否仍执行 Player 更新，是运行时问题：本轮如实记录 OddPlayer tick，不把它当作失败条件。

### 5.5 MC_PRE_PLAYER_UPDATE 跳过

REPENTOGON 的 Lua 回调返回真时，Player 子类更新被跳过，仍调用 `Entity::Original_Update`（binding 第 3 节第 9 条）。按 U4：所在的普通 `Room::Update` 仍算 tick，这一 tick 的 Resolve 结果不被读取，其中的 edge 不补发、不重放。外层与内层 Player Hook 分别记录 `player_update_entered`、`player_native_reached`；在本轮已知官方链路下，外层正常返回且内层未到达可记 `player_skipped_by_chain=1`。异常、未知链路或只有 consumer 缺席时不填写确定的 Lua 跳过原因。本轮只记录，不单独验收。

## 6. A 的输入链路

### 6.1 绑定

- A = `PlayerManager._playerList[0]`，且 `_playerIndex==0`；设备 = InputManager 设备 vector 内唯一满足 `device+0x08 == Player::_controllerIndex` 的对象。T2 已确认原生 lookup `0x5EEC40` 逐项比较设备 id；controllerIndex 不是 vector 下标，`-1` 的“任意设备”分支不用于 exact pair。无匹配、多匹配或未知设备类型时不绑定。
- 绑定是 (Player*, device*, controllerIndex) 的 exact pair，只在当前 generation 内有效，不进入 canonical 或 trace 中的身份字段（规范第 6 节）。
- 作废时机：新房间（MC_POST_NEW_ROOM 调用函数返回后）、死亡/复活状态改变、设备重分配（配对变化）、从实际暂停或加载恢复、run teardown。死亡/复活须观察 Player 的生命周期状态，不能只比较指针；同一 Player 对象也可能改变状态。新房间回调只使旧绑定失效，其所在 Manager 轮次继续中性；至少等该轮 owner 退出，后续 Manager 轮次的无阻断 Authority 入口再以房间身份和唯一 exact pair 重绑，绑定前不 apply。T6 不把回调当成所有路径最终位置稳定的证明。

### 6.2 采样（只采样，不写快照）

- **位置。** 第 35 槽 wrapper 只记录本次 Poll 是否成功；在同一设备的公共更新 `0x5EC900` 正常返回 true、且本层 Poll 成功后完成采样。只处理拓扑确认的 A 唯一候选物理设备，要求调用栈处于 `Manager → InputManager 更新 → 设备公共更新` 内。候选采样与可 apply 的绑定分开：Warmup/重绑前也能取得本轮新鲜物理样本，但须在后续 Authority 入口重新确认 exact pair 和 generation 后才允许 Submit；已失效设备的旧样本不能因此恢复资格。T3 的主审确认 Poll 之后公共更新还生成方向 mask，不能把 Poll 刚返回时的状态当成最终 action 状态。采样始终不写 Authority 的 current/previous。
- **读取方式。** 调用该设备第 8/9/10 槽保存的原函数（不经我们的 wrapper），读出 12 个 gameplay action。T3 已核对这些函数及 Keyboard/Gamepad 下层 button 查询：不消费边沿，不改设备状态；下层状态描述函数只写调用方的输出缓冲。`+0xC8/+0xCC` 只是 action float 状态的一部分，查询还合并映射按钮；不能只读取两个数组替代完整查询。原型依赖 v1.9.7.17 设备布局的 `sampler` 不移植。
- **canonical。** 移动轴 = 右减左、下减上；射击方向经 `ShootingDirections` 按 tx-01-52 投影；炸弹、主动道具、卡牌药丸、丢弃取上升沿；按住的射击为 held。
- **边沿历史。** 跟随实际设备而不是 generation；即使绑定暂时失效，也继续维护已知物理设备的 held 历史。屏蔽期间只更新历史，不缓存待提交 edge；恢复时丢弃旧样本与旧 edge，要求同一 Manager 轮次的新鲜采样。暂停或切房期间一直按住的键不因重绑生成新边沿。设备移除或身份不明时先中性，首次重见只建立 held 基线。

### 6.3 Submit

在 Authority-pre 的 drain 阶段（第 5.2 步骤 1）：
- 每次有本轮新鲜采样的 Authority-pre 都生成 InputCmd，target = completed + lead（默认 2），sequence 递增，经 Participant 的统一 admission 入队；持续 held 即使数值不变也按目标 tick 发送，因为 `max_input_age_ticks=0` 不会沿用上一个 target 的输入；
- 只对同一 completed tick、同一设备/participant generation、同一采样序号的重复发布去重；`sample` trace 可以按数值变化降采样，不能据此省掉后续 target tick 的提交；
- 重新唯一绑定后的首个新鲜输入按 tx-01-51 启动例外处理；
- 边沿 exactly-once：同一边沿只进入一个 sequence；dedupe、过期、冲突的结果写入 trace。

### 6.4 Resolve 与查询覆写

第 8/9/10 槽 wrapper 的规则：

| 条件 | 返回 |
| --- | --- |
| 会话未 armed，或不在主线程 | 原值（tx-04-04 行为） |
| 处于 consumer 上下文，且该 consumer 的 Player 是 A、设备是 A 的设备、当前有 SimulationTick | `Participant::Read(action, query, consumer, exact, tick)`，即 Resolve 结果 |
| armed 的 A gameplay consumer 内，但没有 SimulationTick、绑定无效、设备不匹配或生命周期阻断 | 0（中性）；不能回退到物理输入 |
| 不在 consumer 上下文（菜单、暂停菜单导航、UI 等） | 原值 |

只覆写 12 个 gameplay action（原型 `action < 12`）；其他 action 一律原值。consumer 上下文由 consumer wrapper 写入线程局部状态（Player、ordinal、consumer 种类）。

上表只接管本轮 A 的 gameplay consumer；其他 Player 的非目标查询原样透传。A 的临时绑定失效时仍保留本轮受控对象的失效标记以输出中性，撤销 apply 资格不等于给 A 开启原生输入旁路。候选样本只有在重绑条件满足后才能进入第 6.3 节。

T8 已确认三类 consumer 查询经 Manager 包装层到 InputManager，再进入设备槽；设备层 Scalar 为 x87 返回，高层 Manager Scalar 为 XMM0，不能混用 ABI。Manager 的 Lua 输入回调替代分支可能不进入设备层；本轮支持范围没有 gameplay 输入 Lua mod，自带 UI 与测试夹具也不注册输入重写回调。继续使用设备槽覆写，不额外 Hook ABI 不同的 Manager Scalar，不把设备计数外推为任意 Lua 输入覆写均被接管。

### 6.5 consumer 分派

- 移动、炸弹、主动道具、卡牌药丸、丢弃：设置 consumer 上下文后恰好调用一次原函数。
- 射击（tx-01-54）：armed 且该 Player 是 A 时，如果没有 SimulationTick，或有暂停/加载/生命周期阻断，就不调用原函数，并写 `shooting_dispatch` trace；否则调用一次。未 armed 时始终调用一次。

### 6.6 REPENTOGON 的屏蔽（D7）

浮层打开使控制台判断阻断 Game/Odd 主体；屏蔽期间不 Submit，物理 held 历史继续维护，期间产生的 edge 不进入恢复后的输入。T4 确认菜单 mask 和搜索状态会在 `super()` 前短路，不能稳定采集同次调用的“设备层真、最终假”对照。首版记录原有控制台方法的返回区间，并分别记录 PauseScreen 与加载原因；不新增差值观测 Hook，不补调原生输入或控制台方法。控制台为真不等于所有暂停原因，控制台为假也不证明 gameplay 已恢复。

## 7. 生命周期与阻断

| 事件 | 识别 | 处理 |
| --- | --- | --- |
| 进入新房间 | MC_POST_NEW_ROOM 调用函数返回 | 作废 A 的绑定；本 Manager 轮次中性，后续轮次的稳定 Authority 入口再重绑（第 6.1 节） |
| 暂停菜单、失焦、浮层 | 普通 `Room::Update` 不执行；`pause_state` 只用于 trace | 无 tick；恢复后作废绑定（held 键不产生新边沿） |
| 加载与切房 | 非普通 Room/Player 调用、房间身份或原生加载状态改变，结合 `Lifecycle`、`Door`、`NestedDoor`、`Start` 上下文 | 特殊调用不推进 tick；发生真实生命周期变化后作废，稳定且无阻断的 Authority 入口才重绑 |
| 死亡/复活 | 拓扑、Player 配对或死亡/ghost 等生命周期状态变化 | 作废并重新绑定，覆盖同一指针状态变化 |
| 退出到菜单 | `Game::Exit` 返回 | run teardown：会话进入 Retiring → Idle，退休 session 与 generation；清空命令队列中的旧变更 |
| 继续游戏 | `Game::RestoreState` | 本轮只在新开 run 中启动会话；RestoreState 期间若会话已激活，按 teardown 处理并记录 |

阻断标志（pause、loading、lifecycle）只用于 trace 与射击分派，tick 的有无仍以第 5 节的入口条件为准。

**周期性 owner 与生命周期事件分开。** `ExecuteStartGame` 每 Manager 帧都会调用，Lifecycle/Door owner 的进入本身也不证明发生了加载。上下文栈只区分其内部调用；不能因这些 wrapper 进入/退出就持久阻断或每帧递增 A generation，否则会清空正常的 lead=2 队列。只有实际状态、房间身份或非普通更新事件构成对应的失效依据。

## 8. Hook 行为表

| Hook | 未 armed | armed（会话 Warmup/Active，主线程） |
| --- | --- | --- |
| `Manager::Update` | tx-04-04 行为 | 入口：命令队列；出口：TopExit 发布 completed、快照 |
| `Manager::ExecuteStartGame`、生命周期 FSM、门切房、`RestoreState` | 计数、压栈 | 同左，另作为阻断上下文 |
| `Game::Update` | 计数、压栈 | 同左 |
| 控制台判断 | 计数 | 记录返回值，形成 `paused` 区间 |
| `Room::Update` | 计数 | 栈顶为 `Game` 时开始/提交 StandardRoom tick |
| 奇数帧 helper | 计数、压栈 | OddPlayer 作用域 |
| `Entity_Player::Update` 外层（priority -10000） | 计数、原样传递 | 设置 Player/ordinal 上下文；OddPlayer 的 Authority-pre；记录外层到达与正常返回 |
| `Entity_Player::Update` 内层（priority +10000） | 只观测、原样传递 | 记录原生 body 到达，结合外层结果区分已知链路跳过 |
| Odd 普通调用点 shim `0x2F9559` | 搬移原虚调用与 push 0 | 设置 `OddNormal` 标记；虚调用仍恰好一次，不直接 Submit 或改快照 |
| 六个 consumer | 计数、恰好一次原调用 | consumer 上下文；射击按第 6.5 节分派 |
| 第 8/9/10 槽 | 原值 | 第 6.4 节 |
| 第 35 槽（Poll）、设备公共更新 | 原值 | Poll 记本层成功；公共更新正常返回后采样最终 action 状态 |
| MC_POST_NEW_ROOM 调用函数、`Game::Exit` | 计数 | 生命周期事件 |
| `Console::RunCommand` | 处理 `cs` | 同左，增加第 9 节命令 |
| Join pass、MC_POST_UPDATE、PostGameStart | 计数 | 只计数（Join 在 tx-04-06） |

所有 wrapper 对原函数异常保持中立，原调用恰好一次；唯一的主动抑制是第 6.5 节的射击。

## 9. 命令、状态与 trace

- **命令。** `cs solo start`、`cs solo stop`（U1，已确认）。仍是“校验后入队，下一次 `Manager::Update` 入口执行”。
- **`cs status` 增加的字段：** `session_state`、`session_mode`、`generation`、`completed_tick`、`current_tick`、`ticks_standard`、`ticks_odd`、`a_bound`、`a_generation`、`blocks`（pause/loading/lifecycle 位）、`last_tick_decision`、`queue_depth_a`。
- **trace（每 tick 汇总，避免逐次查询刷满环形缓冲）：**

| `ev` | 内容 |
| --- | --- |
| `session` | 状态迁移、原因、generation |
| `binding` | A 的绑定、作废与原因 |
| `sample` | A 的 canonical 样本发生变化时一条（轴、held、edge） |
| `submit` | sequence、target、decision |
| `tick` | tick 编号、kind、owner token、A 的 current（轴/held/edge）、本 tick 各 consumer 的读次数与非零读数、射击是否被抑制、Player 外层进入/原生 body 到达/链路跳过 |
| `tick_reject` | 拒绝或中止原因（Reentry、WrongReturn 等） |
| `paused` | 控制台判断为真的区间起止 |
| `block_interval` | 第 6.6 节：记录已观察的 console/pause/loading 原因；不伪造同次查询的设备差值 |

## 10. 静态确认结果

| 编号 | 已完成的静态结论 | 实现落点与限制 |
| --- | --- | --- |
| T1 | 三处 Player 虚调用；普通点 `0x2F9559`，特殊点 `0x2F93F9/0x2F949C` | 第 3.3、5.3 节，选择普通调用点 shim；不读 ZHL 桩帧 |
| T2 | `0x5EEC40` 枚举 vector 并比较设备 `+8` 的 id | 第 6.1 节，唯一匹配；controllerIndex 非 vector 下标 |
| T3 | 三槽查询及已知下层不消费状态；数组只是一部分；键盘源有指令实证 | 第 6.2 节，公共更新成功返回后调用保存的原 query |
| T4 | 菜单 mask/搜索 Hook 在 super 前短路，查询链已确认 | 第 6.6 节，选择控制台区间；不新增差值观测 Hook |
| T5 | Console、PauseScreen、失焦分支及普通 Room/Odd 的跳过关系 | 第 5.4、7 节，分开记原因；实际运行值留 E2E |
| T6 | 回调前有条件定位，部分 animation 分支有回调后 Player 定位；已对齐 Lua animation→过渡对象 `+0x1C` | 第 6.1、7 节，回调失效、后续 Manager 轮次再稳定重绑；S9 显式 animation=0；所有路径最终位置写入次序仍 UNKNOWN |
| T7 | `+0x162C` 为 byte，`+0x1618` 为 int，`+0x360/+0x364` 为速度 float | Player 观测按实际宽度；不把 byte 当方向 float 或蓄力值 |
| T8 | 移动 Scalar 0–3/辅助查询、射击 Pressed 0–7、炸弹 Triggered 8；其余 consumer 9–11 交叉查询 | 第 6.4 节与静态报告；同 tick 重复查询幂等，不以固定查询次数定义 exactly-once |

原始反编译/反汇编、官方源码片段、子智能体结论及主审证据已保存在 [artifact](../../artifacts/repentogon/tx-04-05-static/README.md)，详细证据和候选更正见[静态报告](../research/tx-04-05-static-confirmation.md)。这里的“完成”指静态审查完成，不把 UNKNOWN 改为已观察，也不代替 tx-04-05 的实现或唯一 E2E。

## 11. 测试 mod 与 harness

### 11.1 测试 mod（只在 E2E 期间部署，D9）

第 11、12 节已编码，尚未运行。具体实现、少量只读诊断和单启动下 D18 历史基线的严格字节比较规则见[测试代码记录](tx-04-05-test-harness.md)。本节场景与验收目标保持不变。

在 tx-04-04 夹具基础上增加（U2，已确认）：
- **跳帧触发。** 在 `MC_POST_RENDER` 中检测 F8 键（`Input.IsButtonTriggered`），之后让 `MC_PRE_UPDATE` 连续返回真 30 次，起止各写一条 `[isaac-cs-test] skip-update` 日志。这是 gameplay 回调，只作测试夹具。
- **只读观测。** 在 `MC_POST_RENDER` 中，当 A 的位置、炸弹数、泪弹实体数变化时写日志（`Isaac.GetPlayer(0).Position`、`GetNumBombs()`、`Isaac.CountEntities(nil, EntityType.ENTITY_TEAR)`），作为输入生效的游戏侧证据。
- **切房触发（U3，已确认）。** 在 `MC_POST_RENDER` 中检测 F9 键（D21 起改为第二次 F8，F9 是原版手动截图热键），以全参数 `Game():StartRoomTransition(targetRoom, direction, 0, nil, -1)` 发起一次相邻房间过渡，显式使用 animation=0、当前维度。T6 已静态对齐 animation 参数到 RoomTransition `+0x1C` 的写入链；该字段不是 `_mode`。目标取当前房间的可通行门对应房间，具体 Lua 选门读取在实现时完成。记录请求参数和实际房间身份；原生可能拒绝或改写目标，不能用“调用返回”代替切房成功。前后各写一条 `[isaac-cs-test] room-transition` 日志，之后按生命周期重新绑定。

### 11.2 harness 场景（一次执行，单次启动）

开始前按规范第 8 节告知用户：harness 会接管键盘几分钟；`SkipIntro` 已开启，不需要手动跳过动画；之后不要操作电脑。

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S0 启动 | 环境检查（含 `SkipIntro=1`）、部署、注入、选第二个存档、开新局 | 与 tx-04-04 相同 |
| S1 未激活透传 | 会话未启动时按住 D 1 秒 | 玩家移动（Lua 位置日志）；`ticks_standard=0`；查询覆写次数为 0 |
| S2 启动单人会话 | 控制口 `cs solo start` | `Warmup → Active`；`a_bound=1` |
| S3 移动 | 按住 D 1 秒再松开；按住 W 1 秒再松开 | A 的 sample/submit/tick 中 move 轴非零，松开后在 lead 内回到中性；位置变化方向正确 |
| S4 射击优先 | 按住 → 1 秒，再加按 ← 0.5 秒，松开 ←，再松开 → | aim_x 依次为正、负、正、0（tx-01-52）；泪弹数增加 |
| S5 炸弹 exactly-once | 轻按 E 一次 | 一个 bomb edge 仅归属一个被接受/Resolve 的目标 tick；允许同 tick 的只读重复查询，炸弹数减一 |
| S6 浮层 | 打开浮层 1 秒再关闭 | 浮层期间 completed 不变；关闭后 tick 恢复；物理历史可继续更新，期间输入不进入 Submit/Resolve，edge 不跨屏蔽区间重放 |
| S7 暂停菜单 | ESC 打开暂停 1 秒，期间按住 D，再 ESC 恢复 | 暂停期间无 tick；恢复后 held 的 D 不产生新边沿；A 重新绑定 |
| S8 跳帧 | 按 F8 | 跳帧窗口内 `ticks_standard` 不增加；OddPlayer tick 如实记录 |
| S9 切房 | 按 F9（D21 起为第二次 F8），由测试 mod 切换到相邻房间（U3） | 加载期间无 StandardRoom tick；A 作废并重新绑定；之后移动仍生效 |
| S10 停止 | `cs solo stop`，再按住 D | `Retiring → Idle`；查询回到原值；玩家仍能原生移动 |
| S11 teardown | 再次 `cs solo start`，然后退回主菜单 | `Game::Exit` 使会话退休；状态为 Idle；旧输入不复活 |

## 12. E2E 验收（本次推进唯一的一次测试）

一次执行，失败只记录，不自动重跑。

| 编号 | 判定 |
| --- | --- |
| V1 加载 | 能力组全部通过；入口 Hook 注册数（含 Player 内外层）与清单一致；设备槽补丁 8 个及 Odd 普通调用点 shim 已安装 |
| V2 透传 | S1 与 S10 期间查询覆写次数为 0，tick 计数不变，玩家原生可控 |
| V3 会话 | 状态迁移顺序为 `Idle → Warmup → Active → Retiring → Idle`（两次：stop 与 teardown），generation 递增，无 `tick_reject` |
| V4 tick | 每个 StandardRoom/OddPlayer tick 恰好一次 Authority-pre、一次正常提交和一次发布；StandardRoom 栈顶为 `Game`，OddPlayer 来源为 `OddNormal`，稳定未阻断的输入场景须实际观察到两种 tick；completed 单调递增，同一 Manager 轮次没有重复 tick；非普通 Room 与两处特殊 Odd Player 点不开始 tick，shim 的原虚调用恰好一次 |
| V5 移动 | S3 中 Resolve 的移动轴、移动 consumer 的非零读数、Lua 位置变化三者一致；松开后不超过 lead+1 个 tick 回到中性 |
| V6 射击 | S4 的 aim 序列符合 tx-01-52；无 tick 时射击 consumer 原调用为 0（tx-01-54） |
| V7 炸弹 | S5 中 bomb edge 恰好一次被接受，只在一个目标 tick 中 Resolve，不跨 tick 重放；consumer 同 tick 的重复查询结果幂等，炸弹数恰好减一 |
| V8 阻断 | S6、S7、S8 期间没有 StandardRoom tick；恢复后 held 键没有新边沿；跳帧起止与 tick 空档对齐 |
| V9 切房 | S9 中加载期间没有 StandardRoom tick；A 至少一次作废并重新绑定；之后 S3 式移动仍生效 |
| V10 退出与副作用 | 正常退出；没有崩溃转储；存档变化按 tx-04-04 的规则（含 D18 基线）；`trace_dropped=0`、`context_faults=0`、非主线程调用为 0 |

**预先列出的失败情况：**

| 编号 | 情况 |
| --- | --- |
| F1 | 启动或会话无法进入 Active |
| F2 | 栈顶判断错误：非普通 Room 更新开始了 tick，或普通 Room 更新没有开始 tick |
| F3 | 同一帧两次 Authority-pre、提交缺失或 `tick_reject` |
| F4 | A 的样本未进入统一队列，或 consumer 读到的不是 Resolve 结果 |
| F5 | 边沿重复或丢失（炸弹数变化不是一） |
| F6 | 暂停、浮层、跳帧、切房期间出现 StandardRoom tick，或恢复后出现新边沿 |
| F7 | 停止或 teardown 后仍覆写查询，或旧输入复活 |
| F8 | 射击在无 tick 时被调用，或正常 tick 中被错误抑制 |
| F9 | trace 丢弃、上下文错误或非主线程调用 |
| F10 | 崩溃、卡死或存档出现不允许的变化 |

**artifact：** 与 tx-04-04 相同的环境、部署、存档快照与报告，另加每个场景的步骤记录、`tick`/`submit`/`sample` trace、Lua 观测日志和关键截图。

## 13. 风险与需要用户决定的事项

**用户决定（2026-10-24 全部按下表建议确认，记为设计 D19）：**

| 编号 | 事项 | 建议（已确认） |
| --- | --- | --- |
| U1 | 单人会话的命令 | 新增 `cs solo start/stop`，`cs host` 保留给 tx-04-06 的真实网络会话 |
| U2 | 测试 mod 是否可以注册 `MC_PRE_UPDATE`（跳帧）并读取玩家状态 | 可以，只作 E2E 夹具（D9；设计第 13 节已把跳帧列为场景） |
| U3 | 切房场景的触发方式 | 由测试 mod 调用 `Game():StartRoomTransition` 切到相邻房间：确定、可重复，不用调试控制台命令；备选是让 harness 控制 A 走过门，但起始房间的门位置不固定 |
| U4 | MC_PRE_PLAYER_UPDATE 跳过的帧是否计 tick | 按设计 7.3 计 tick、edge 不补发（第 5.5 节），本轮只记录不单独验收 |

**风险：**

| 编号 | 风险 | 处理 |
| --- | --- | --- |
| R1 | 奇数帧 helper 的普通调用点只靠 Odd 上下文分不出 | T1 已选方式 4；shim 与异常清理的实际实现仍待整体验收，不读 ZHL 桩帧 |
| R2 | Poll 后 action 状态仍有派生阶段 | T3 已确认已知 query 不消费状态，采样改到公共更新成功返回后；不只读数组 |
| R3 | 用户自定义了键位 | 采样按 action 读取，不依赖键位；harness 按默认键位发送，环境中记录 `inputconfigs` 与 options，键位非默认时在 S0 结束并说明 |
| R4 | trace 量过大 | 每 tick 一条汇总，`sample` 只在变化时写；环形缓冲容量按 30Hz×场景时长估算 |
| R5 | REPENTOGON 的 MC_PRE_PLAYER_UPDATE Hook 与我们的 Player 更新 Hook 嵌套顺序 | 外层 -10000、只观测内层 +10000（第 3.3 节）；按原生 body 到达证据记录，不从 consumer 缺席倒推 |
| R6 | 失焦暂停使场景时序不稳定 | harness 每次发键前确认焦点（tx-04-04 已有）；以 tick 计数而不是墙钟判定 |
| R7 | 炸弹在起始房间造成持久副作用 | 第二个存档的变化在允许范围内（D14）；起始房间通常没有可破坏物，结果中如实记录 |
