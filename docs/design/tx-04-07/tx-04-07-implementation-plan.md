# tx-04-07 实现方案：故障注入、B 死亡与复活、异常 Join、跨房射击、加载查询与首轮完整场景集

> **当前状态：已完成（用户正式接受，D31，2026-10-26）**。用户将 e2e-11 整体 E2E 验收正式视为全部通过，tx-04-07 按此结论收尾；不再将原未通过/未观察项作为本轮补测要求。原始自动 FAIL、S17 停止、S18–S21 未运行及 V12 实测 PASS 仍作为实际记录保留，见[验收记录](../../artifacts/repentogon/tx-04-07-e2e-11/acceptance.md)。下方设计与历史状态按原时点阅读。

> e2e-11：V12已实测PASS，六tick恢复窗口、无提前B激光及实时松键后激光证据齐全；整体仍因S17无观测可移动净空而FAIL，S18–S21未运行。见[结果](../../artifacts/repentogon/tx-04-07-e2e-11/README.md)。本次只修移动预览/时长余量、保留既有V12修复，未自动重跑。

> 2026-10-26 V12 修复：用户已采纳六 tick Shooting 恢复例外，实时 witness/激光归属与恢复逻辑完成并统一纯构建；唯一 e2e-10 在 S9 无可观察移动方向时停止，V12 NOT_RUN，未自动重跑。见[实现与规则](tx-04-07-v12-recovery-fix.md)及[结果](../../artifacts/repentogon/tx-04-07-e2e-10/README.md)。下方早期失败与规则保持原时点。

> e2e-09（2026-10-26）已执行一次：debug 10 控制台启用，A 存活保存，S20 Continue 完成；S21 暂停等待超时，整体 FAIL。V2/V5 已 PASS，V13 加载不变量通过但自然重复覆盖未齐，V12 仍 FAIL；见[实际记录](../../artifacts/repentogon/tx-04-07-e2e-09/README.md)。没有自动重跑或测试后功能修改。

> e2e-08 已完成一次启动：S0–S19 执行，S20 因 A 死亡后无可 Continue 的 run 而失败；中继收尾及前三轮对账通过，V12 仍有实际提前发射/输入空档及快照时序判定问题，详见[本次结果](../../artifacts/repentogon/tx-04-07-e2e-08/README.md)。用户随后要求下一次开局原生控制台输入 debug 10，harness 已新增一次性准备并纯构建，尚未实测，见[记录](../../artifacts/repentogon/tx-04-07-debug10-preparation/README.md)。

> 2026-10-26 后续修复已编写并统一纯构建：中继收尾通知/排空与 V12 普通松键对照、180 tick 按住、真实消费边界及独立证据文件。未部署或执行修复后 E2E；真实重绑缺输入仍如实中性和失败，见[说明](tx-04-07-relay-v12-fix.md)。

> 最新e2e-06（2026-10-26）：**FAIL**，S6暂停等待回归修复，S0–S15已执行，V1/V7/V8/V9/V11/V15通过。S16发起Bye且主机正常退休，但UDP relay receive报10054，收尾未完成，S17–S21未执行；V12缺激光证据、NOT_OBSERVED。环境恢复，无自动重跑。见[结果](tx-04-07-results.md)与[修复](tx-04-07-pause-wait-fix.md)。旧结果保留。

> 最新 e2e-05（2026-10-26）**FAIL**：焦点维护修改已构建，但 S6 主动退出暂停的 stable 等待被新增检查过早终止，属于 harness 回归；焦点恢复尚未得到实机样本。S0–S5 完成，S6 失败，S7–S21 未执行；正常退出、无新 dump、环境已清理，无自动重跑。见[结果](tx-04-07-results.md)与[修改/回归说明](tx-04-07-focus-fix.md)。旧结果全部保留。

> 退休崩溃实质修复后第三次 E2E（2026-10-25）：**FAIL**。S13 Stop 正常退休、无新 dump，S0–S14 完成；S15 前发生 b_animation_timeout，直接原因是 Lua 夹具把实体 Index=17 与玩家 ordinal=1 比较后误标 B 不存在。S15–S21 未执行，本轮一次启动、无自动重跑，环境已清理。见[修复](tx-04-07-retirement-fix.md)与[结果](tx-04-07-results.md)。下方历次结论保留原时点。

> 实质修复后第二次 E2E（2026-10-25）：**FAIL**。启动 trace 丢失未重现，S0–S12 完成、V11 死亡/复活 PASS；S13 Stop 退休时游戏崩溃（0xC0000005），S14–S21 未执行。用户要求修复并继续测试后仅再启动一次，未自动重跑。见[修复](tx-04-07-trace-fix.md)与[完整结果](tx-04-07-results.md)。下方首次失败与编码阶段按原时点保留。

> 唯一整体 E2E 已执行（2026-10-25，America/Los_Angeles）：**FAIL**。首轮开局因启动期间已有 trace 丢失 63 条触发 trace_event_lost，K/Join 未发生；只启动一次，未重跑，已正常退出和清理。详见[结果](tx-04-07-results.md)。下方未测试与 E2E 待执行为编码时点记录，不预记后续未运行项通过。

> 2026-10-26 第 3.4 节 harness 已编写，CLI/部署、relay、远端、四轮编排、V1–V16 与持久文件证据已接入；必要的 Lua 只读观测和网络终态时刻同步补齐。3.1–3.4 已完成统一纯编译链接，见[编写记录](tx-04-07-harness.md)和[构建日志](../../artifacts/repentogon/tx-04-07-harness/README.md)。没有部署、启动游戏或运行测试，唯一整体 E2E 尚未执行。下方 3.4 尚未开始、未构建等表述保留之前阶段的原始时点。

> 2026-10-26 第 3.2 节已完成状态/trace 编写，见[接口与接线记录](tx-04-07-state-trace.md)。补齐成功退休后的 None/Join 结果、完整分类查询覆盖与场景检查点、丢弃/Ack/生命归属/加载/持久证据及日志截断处理。未构建、部署或测试；第 3.4 节及整个 tx-04-07 的统一纯构建/唯一整体 E2E 仍待完成。下方同时保留 3.1/3.3 的工程进度记录。

日期：2026-10-25；2026-10-26 同步 ST2 主审。状态：**ST1–ST7 静态取证与主审完成，保留 UNKNOWN；工程改动已细化；2026-10-26 第 3.1 节 DLL `game/` 与第 3.3 节测试 mod 已编码，尚未构建或运行**。用户已按建议确认 U1–U6（设计 D29）和起草中新增的 U7–U11（设计 D30），并确认第二个存档已解锁阿萨泻勒（见第 13 节）。[静态核验](../research/tx-04-07-static-confirmation.md)确认加载时序，并发现当前 Restore/RunExit 会退休菜单 Host 的实现冲突，见第 8 节。2026-10-25 完成[工程改动细化](tx-04-07-engineering-changes.md)，2026-10-26 追加 ST2 对普通复活复用、健康与动画边界的主审更正，见报告第 15–18 节。2026-10-26 按该细化完成第 3.1 节 `game/` 及其必要的 core/net/binding 配对改动（含 8 个 `observe` 组只读入口与 `PeerLost` 追加），**没有构建、部署或运行**；第 3.3 节的 Lua 夹具已编写（tx07 动作/生命/动画/激光观测，**未构建、未部署、未运行**），第 3.4 的 harness 与 relay 尚未开始，整体 E2E 仍留到全部实现完成后。本次 ST2 材料审核不包含上述实现代码的验收。

依据：
- [REPENTOGON 适配重构设计](repentogon-refactor.md)（下称“设计”），尤其是第 8、13、14 节；
- [tx-04-06 实现方案](tx-04-06-implementation-plan.md)与[结果](tx-04-06-results.md)：本轮直接复用的网络、K、Join、B 输入与退休；
- [tx-04-05 实现方案](tx-04-05-implementation-plan.md)：tick、A 输入链与阻断；
- [现行项目规范](../refactor/project-conventions.md)，尤其是第 2、6、8、9 节；
- 原型：`cs-t0/src/t09/relay.rs`（relay 与故障模型），以及 virtual device 系列记录 tx-01-49/50（死亡与复活）、tx-01-54（跨房射击）、tx-01-55（加载与重复查询）、tx-01-56（异常 Join），都在 `docs/refactor/cs-t0/t0-05/07-virtual-device/` 下。原型只作参考，点位须在 v1.9.7.12 上重新核对。

按规范第 2 节，本轮只在全部实现完成后执行一次 E2E（第 12 节）；分析、逆向和实现期间不运行任何测试。

## 1. 目标、范围与边界

**目标。** 在 tx-04-06 的单远端 Host 会话上，补齐设计第 13 节首轮场景集里尚未覆盖的部分：网络故障下的输入语义、B 死亡后复活重绑、实际走到原生 Baby 判断的 Join、B 按住攻击跨房、Continue 加载与换层期间的查询。然后用一次启动执行首轮完整场景集。

**预期结果。** 一次 E2E 通过第 12 节的 V1–V16。

**本轮包含：**
1. harness：Rust relay（插在远端与主机之间，按场景段切换故障档位）、四轮 Host 的单启动场景、Continue 菜单导航、控制台键入命令、新的判定项；
2. 测试 mod（D29 U2 允许它修改世界）：按顺序执行的夹具动作（跳帧、切房、令 B 死亡、复活 B、换层），以及激光与生命状态观测；
3. DLL `game/`：B 复活后的重新绑定（替换当前“死亡后永久失效”的规则）、死亡与复活期间的 Player 数规则、所有覆写动作的重复查询一致性计数；
4. 状态与 trace：B 的生命状态与重绑次数；会话结束后 `join_state` 的语义（第 13.2 节 U9）。

**不包含：** world_record（tx-04-08）；Replica 与状态同步；DLL 内的 Client 角色；断线重连与 late join；ghost 状态下由远端操控 B（规范第 6 节：死亡使绑定失效）；包内容损坏（CRC）故障注入（第 4.2 节）；多远端；第三方 gameplay mod。

## 2. 依据与已知事实

### 2.1 沿用的已采纳语义

| 来源 | 语义 | 本轮落点 |
| --- | --- | --- |
| 规范第 6 节 | room、death、设备重分配使绑定失效；缺输入中性（`max_input_age_ticks=0`）；Authority 不等待远端 | 第 4、5 节 |
| 规范第 9 节、tx-01-49/50 | 死亡与复活不是设备退休或第二次创建；复活后重新唯一绑定到同一个 K，ordinal 不变；Player 地址可以变化；旧 generation 的输入被拒绝 | 第 5 节 |
| tx-01-51 | 重新唯一绑定后的启动例外：首个新鲜命令可用于 completed+1 | 第 5、8 节 |
| tx-01-54 | 无 tick 或有阻断时主动暂停射击 consumer；有效 tick 中的真实松键和缺输入仍按中性消费 | 第 7 节 |
| tx-01-55 | 加载期间不 Submit/Resolve、不推进 Authority；加载结束后先重新唯一绑定，再接受新鲜输入；同一查询重复读取值不变，且不改变参与者状态 | 第 8 节 |
| tx-01-56 | Baby 判断为真时，只对自有 K 选择普通完整 Player 路径；原 predicate 各调用一次；普通工厂是唯一创建 owner | 第 6 节 |
| tx-04-06 第 5 节 | 乱序旧包计 stale；字节相同的副本只计数；Hello 每 250 ms 原字节重发；Bye 原字节连发 3 份；对端超时 3000 ms | 第 4 节 |
| D24 | 对端断开后整个会话结束，回到 Idle；B 的角色由房主经 `cs host <port> [character]` 指定 | 第 6、7 节 |
| D27 | 存档判定不看每日备份 | 第 12 节 V16 |

### 2.2 tx-04-06 留下的事实

- `RUNTIME_OBSERVED`（e2e-02）：
  - 第 1 轮在开局第一个房间 Join，三处 predicate 原值都是 0。
  - 第 2、3 轮在切房之后 Join，三处原值都是 1，全部被改写为 0（`join_baby_overrides=5`），B 仍是完整的 Isaac。异常 Join 路径已经实际执行过，但不是有意安排的场景。
- `RUNTIME_OBSERVED`（e2e-02）：本机回环下主机 stale=0、duplicate=0；远端每轮收到 2 份字节相同的副本。乱序和丢包规则还没有实际起作用过。
- `SOURCE`（`mod/src/game/lifecycle.cpp`）：
  - B 一旦被观察到 dead、removed 或 ghost，就置位 `remote_life_lost`；它在会话内不会复位，B 此后不再绑定。
  - Host 会话中，只要 run 内的 Player 数不等于 2（有 B 时）或 1，就以 `host_player_count_changed` 让运行时失败。
  - 加载阻断来自 `starting`、`room_transition`、`stage_transition` 三个标志。
- `SOURCE`（`mod/src/game/join.cpp`）：`cs host` 的 character 必须在 manager `+0x70/+0x74` 角色表中按值找到（`CharacterAvailable`），否则拒绝创建。阿萨泻勒（7）是否在第二个存档的表中，取决于该存档的解锁状态（第 13.3 节 R4）。

### 2.3 REPENTOGON 1.1.3 提供的接口（`SOURCE`，`F:\projects_isaac\REPENTOGON-1.1.3`）

- `EntityPlayer:ReviveCoopGhost()`：仅当 `_isCoopGhost`（`+0x207D`）为真时调用原生 `RevivePlayerGhost`，返回是否尝试了复活（`LuaEntityPlayer.cpp:2355`）。
- `EntityPlayer:MorphToCoopGhost()`：直接调用原生 `MorphToCoopGhost`。这是把玩家变成 ghost 的原生动作本身，不是死亡判定，本轮不用它来模拟死亡。
- `Game:StartStageTransition(sameStage, transition, player)`：直接调用原生 `Game::StartStageTransition`（`LuaGame.cpp:104`）。
- `Game:StartRoomTransition(...)`：tx-04-05 起由夹具用于切房。

`STATIC_CONFIRMED`（[ST1](../research/tx-04-07-static-confirmation.md)）：A 健康、独立完整 B 为普通 Isaac 且无自动复活/回调改写时，B 经原生死亡动画与 `TriggerDeath(false)` 在原对象上转为 ghost，真实 Player vector 人数保持 2；Morph 先清 dead、再置 ghost，不改普通局部分支的 controller。正常 ghost 分支绕过 `DEATHS+1`，仍有条件性道具重算/回调副作用；设备通知的完整调用闭包为 UNKNOWN。复活端仍由 ST2 确认。

### 2.4 原型 relay（`SOURCE`，`cs-t0/src/t09/relay.rs`）

- 两个方向各有一个由种子派生的随机源。逐包决定：丢包（ppm）、断网窗口内丢弃、重复（ppm，第二份晚 1 ms）、乱序（ppm，额外延迟）、基础延迟加抖动。
- 每包写一条决定记录。参数都有上限（延迟分量 ≤ 5000 ms，概率 ≤ 1 000 000 ppm），种子不能为 0。

## 3. 工程改动（3.1–3.4 已编码并统一纯构建，尚未部署或测试）

文件与接口、状态迁移和验收接线以[工程改动细化](tx-04-07-engineering-changes.md)为具体实施依据。本节保留总览；第 10 节的 ST2 主审于 2026-10-26 经重新取证完成，不以设计细化或实现编码替代静态证据。

### 3.1 DLL `game/`

- **B 的生命状态与当前归属。** 新增 `remote_life.h/.cpp`，把 `remote_life_lost` 改成 `Alive/Lost/Revived`，生命状态与是否绑定分开。死亡只使 B 失效，Active 的 Authority 继续；复活后稳定唯一绑定才恢复输入。首次 Join 历史与重绑后的当前 A/B 归属分开保存，退休时重新读取当前关联，不依赖最初的 Player 地址。详细转换与 PeerLost 防御见工程细化第 2 节。
- **Player 数规则。** ST1 已确认普通 Isaac 死亡→ghost 仍在真实 Player vector 中，该正常路径继续要求人数为 2；原生“非 ghost 完整 Player 数”不用于替代 vector 人数。异常移除/身份变化仍按第 5.3 节保留防御，复活端由 ST2 确认。
- **重复查询一致性。** 将目前只覆盖炸弹的比较扩展到全部覆写动作；按 ordinal、tick、participant generation、资格变化段、consumer、QueryKind、action 分键。加载时另汇总无 tick 查询是否中性；未自然重复的动作如实记 NOT_OBSERVED。只读观测，不增加为测试而调用的查询，不将不同查询或合法失效前后的值混为不一致。
- **首次菜单 Continue 的生命周期。** 用同一次 Restore scope 决策替代两个无条件退休请求。仅对源自菜单、尚未观察稳定 run、没有 K/Join/旧绑定、符合首次 Start/Restore 路径的本轮 Host 保留会话；清排队变更与 StopSession 分开。已有 run 恢复/实际 Game::Exit 继续退休；所有相关 owner 正常返回后才稳定重绑。加载状态先于稳定人数/身份判断，避免内部临时拓扑提前触发失败。详见工程细化第 3 节，本轮未实施修复。
- **持久副作用观测。** 跨 loop 操作窗口与同步调用标签并存，按 call_id/parent/window ids 关联；增加原生 Kill/Revive/Stage 与官方 ABI 的 GameState 保存/读取/删除观察，补 TryUnlock 的 bool 返回。文件证据采用 harness 有界变更监听、窗口前后快照和官方日志，不做全局文件 API Hook；未确认结果逐项 UNKNOWN。归约拆成入口完整性、窗口关联与文件许可，保留 D18/D20/D27，不套旧零变化断言、不扩大白名单、不翻 readonly、不回滚。详见工程细化第 5 节。

### 3.2 状态与 trace

2026-10-26 已编写；具体字段、事件、场景检查点命令与证据边界见[编写记录](tx-04-07-state-trace.md)，尚未经过构建或运行验证。

- `cs status` 增加 `b_life`（Alive/Lost/Revived，无 B 或已退休显示 None）、`b_deaths`、`b_revives`、`b_rebinds`；U9 已确认：退休完成后 `join_state=None`，`join_last` 保留最近结果；清理失败不提前显示 None/Idle。
- trace 增加生命迁移/当前归属、按原因输入丢弃、loading owner/episode、持久窗口/调用结果；重复查询按 tick 汇总与场景累计分键覆盖输出，控制日志队列负荷。session generation 与 A/B participant generation 分字段。

### 3.3 测试 mod（`tests/e2e-mods/isaac-cs-test`）

- **夹具动作序列（U8）。** 动作序列写在 harness 生成的 `harness_config.lua` 中，每按一次 F8 执行下一项。动作有 `skip`（30 次 MC_PRE_UPDATE 跳过）、`room`（相邻普通房间）、`kill_b`、`revive_b`、`stage`。每个动作写 request/arrived/rejected/timeout 事件；不重试，失败记为夹具错误。F9 仍不使用。
- **令 B 死亡。** ST1 确定采用 ordinal 1 完整普通 B 的 `Kill()`；它通过原生 Entity Kill、死亡动画及 PlayerManager 分派进入 ghost，不直接写 dead/ghost 字段，也不调用 `MorphToCoopGhost`。只发起一次请求，等待实际 dead/ghost 观测；不把 Kill 返回或下一次 Manager Update 当作 ghost 已到达。意外自动复活、移除或超时分别记录。
- **复活 B。** 仅对已确认的完整 B ghost 调一次 `ReviveCoopGhost()`，分别记录返回值、实际健康、动画和 DLL 稳定重绑；不把 true 当作重绑成功。ST2 主审已确认 Appear 会跳过主要射击输入块；按工程细化第 2.3 节先观察移动，动画实际结束后再作射击断言，不承诺立即位移。
- **换层。** 调用 `Game():StartStageTransition(false, 0, nil)`；ST3 已确认官方 Lua 参数匹配、原生 transition=0 有效、nil 回退主 Player。通过方法发起，不按官方 StageTransition 结构中与原生指令不一致的 sameStage 偏移写字段。
- **观测扩展。** 每个 Player 的 dead/ghost（已有）、动画完成状态、关卡/房间身份、每次动作前后人数；在实体初始化回调中只读记录 Laser 出生及可确认的 spawner/parent 归属，不仅用渲染帧存量判断瞬时激光。

实现状态（2026-10-26）：`tests/e2e-mods/isaac-cs-test/main.lua` 已加入 tx07 配置驱动动作、生命/动画/人数/房间/激光观测和 `tx07` 日志；`harness_config.lua` 记录配置模式与超时字段；同目录 `README.md` 记录动作、事件和配置契约。**未构建、未部署、未运行**，事件字段供 3.4 harness 接线。

### 3.4 harness（`tools/harness`）

实现状态（2026-10-26）：本节已编写并完成统一纯编译链接，详细接口、artifact 与未观察结果的处理见[编写记录](tx-04-07-harness.md)。尚未实机验收，不预记 V1–V16 通过。

- **`relay/`（新增）。** 远端只与 relay 通信，relay 用自己的 socket 转发给主机，因此主机锁定的对端是 relay。故障档位由场景在段边界切换（第 4 节）。
- **远端。** 经 relay 连接；支持主动 Bye（已有）；新增“对端已因超时结束”的正常收尾，用于第 3 轮。
- **启动与旧代次证据。** 当前远端固定 lead=2，补新代次首个新鲜样本的 +1 启动与 edge=0 连续量补偿；S11 用新包/新命令序号携带旧 generation Ack 引用，验证主线程旧代次拒绝，不能用重复老包的 stale 代替。详见工程细化第 6.3 节。
- **场景。** 单启动四轮 Host（第 11.3 节）；Continue 菜单导航；在 REPENTOGON 控制台中键入 `cs echo <token>` 并回车。
- **归约。** V1–V16；relay 日志参与三方计数对齐。

## 4. relay 与故障注入（D29 U1：故障只放在专门的场景段）

### 4.1 拓扑

```text
Rust 远端 ──UDP──> relay[R 口] ··· relay[H 口] ──UDP──> 主机 DLL（127.0.0.1:27090）
        <────────            ···             <────────
```

relay 在四轮中都在线。除故障段外使用 `clean` 档：不丢包、不重复、不乱序，固定延迟 0。主机侧的 bind address 与端口不变（U1/D25）。

### 4.2 故障模型

沿用原型 relay 的模型，两个方向独立、可分别配置：丢包、重复、乱序加额外延迟、基础延迟加抖动、断网窗口。随机源由场景中的固定种子按方向派生，同一种子得到同一决定序列；但包的到达时刻受游戏帧时序影响，所以逐包结果不承诺可复现，只承诺决定序列可复现。

不注入内容损坏：按 tx-04-06 第 5.1 节，握手后的 CRC 错误会以 `PeerProtocol` 结束会话，混入正常故障段会让段内判定失去意义。`PeerProtocol` 结束路径本轮不覆盖（第 13.3 节 R6）。

### 4.3 档位

| 档位 | 内容（两个方向相同） | 使用位置 |
| --- | --- | --- |
| `clean` | 无故障 | 故障段以外的全部场景 |
| `moderate` | 丢包 5%，重复 2%，乱序 2%（额外延迟 60 ms），延迟 20±10 ms | S7，约 10 秒 |
| `outage_short` | 1000 ms 双向全部丢弃，短于 `peer_timeout_ms`；前后为 `clean` | S8 |
| `handshake_loss` | 丢包 30%，其余为 `clean` | S17 握手段（U7），握手完成后切回 `clean` |
| `outage_long` | 双向全部丢弃，直到主机以 PeerTimeout 结束会话、远端也自行判超时，再切回 `clean` | S18 |

### 4.4 记录

每轮一份 `relay-N.jsonl`。逐包记录：方向、到达时刻、长度、用 harness 自己的 codec 解出的 kind 与 packet sequence（解不出也照常转发并记录）、决定（forward/drop/outage/duplicate/reorder）、计划发送时刻、实际发送时刻、份数。每次切换档位记一条，收尾写各方向计数摘要。

### 4.5 故障段的判定（不变量）

故障会让位置等确定值失去意义，因此故障段只按不变量判定：
- 段内不出现协议错误：CRC、格式、身份类错误为 0，没有 `PeerProtocol`；会话保持 Active。
- `moderate` 段必须实际发生丢包、重复、乱序各至少一次，并且主机侧的 duplicate 与 stale 都 > 0；任何一项没出现都记 NOT_OBSERVED，判为不通过。
- 三方计数对齐：按每轮双方成功 tx、relay 收包/成功发送的份数与双方 rx 对账；等双方停止发送、relay 有界 drain 后比较最终计数。副本、策略丢弃、发送失败、待发/收尾丢弃分别记账；Hello/Bye 同序号重发按 occurrence 计数，详见工程细化第 6.2 节。
- 每个非中性的 B Resolve 都能对上一条被接受的命令（沿用 tx-04-06 的 `matches_command`）；边沿至多生效一次；缺输入的 tick 中 B 为中性。
- Authority 不等待：S7 与段前同长度 clean 有效 Active 窗口各约 10 秒，以 DLL trace 时间轴计算 completed 推进率；比值预设为 0.8–1.2，并检查无等待 B，阈值写 scenario/artifact，不按结果调整。明确 pause/loading 段不纳入两个窗口。
- `outage_short` 后会话仍在，B 不需要重绑，也不切换 generation，恢复后接受新输入。
- `outage_long`：主机在最后一个合法包之后 ≥ `peer_timeout_ms` 时以 PeerTimeout 结束会话。主机发出的 3 份 Bye 被 relay 丢弃，远端由自身超时结束；两边的结束原因都如实记录。

## 5. B 的死亡与复活

### 5.1 语义

```text
Alive(bound) --观察到 dead 或 ghost--> Lost --重新满足健康条件--> Revived --稳定 Authority 入口--> Alive(bound, 新 generation)
```

- **Lost。**
  - 在观察到的那一帧使 B 失效，B 输出中性；A 不受影响，Authority 照常推进。
  - Lost 期间 B 的命令全部丢弃并计数，不缓存到复活之后。
  - Ack 保留上次公布的 generation（tx-04-06 第 5.3 节）；远端继续发送，主机不会接受。
- **Revived 的条件。** 同一会话内，ordinal 1 的 Player 同时满足：
  - Type=1、Variant=0；
  - controller 仍是 K 的 id，K 仍处于 Registered；
  - valid、exists，且不是 dead、removed、ghost；
  - 拓扑唯一。

  Player 地址可以变化（tx-01-50）。
- **原生复活的静态边界（ST2）。** 普通受限 Isaac 路径复用原对象，本体清 ghost、不重新 Init 或重分配 controller；valid/exists/dead/removed 等并非被无条件重置。下游重算、特殊角色、backup 和官方回调不作完整闭包保证。ghost 在下游调用前已清零，动作正常返回前不得仅凭 ghost=false 公布新代次。健康/归属重读、稳定绑定与 Appear 完成分别记录。
- **重绑。** 在之后第一个稳定、未阻断的 Authority 入口重新唯一绑定。现有 Participant 在死亡失效的 `Invalidate()` 时已加一，Bind 不再加一；只有绑定可接收输入后，TopExit 才公布新 generation。远端清空旧样本、边沿和冗余，保留 session 级 sequence，再发送新输入；旧 generation 的 Ack 引用输入被拒绝。启动例外按 tx-01-51 适用，详见工程细化第 2.2、6.3 节。
- **不允许的事。** 复活时不重新创建 K，不再走 Join，不调用 RemoveCoPlayer，也不改变 ClientId→ordinal 的映射。

### 5.2 死亡窗口内的观察要求

至少持续 3 个失效之后的完整 Authority tick（tx-01-49）。若在已开始 tick 中途观察到死亡，保留该 tick 前半段已合法发生的事实，不追溯改写为全 tick 中性。在完整 Lost 区间内：
- B 没有非中性 Resolve，也没有被接受的输入；
- A 有有效 Resolve，Authority 正常提交；
- 原生的 ghost 行为（ghost 的移动、射击）不由远端控制。

### 5.3 不能复活的情况

在非加载的稳定观察中确认 B 被原生移除（removed）或归属矛盾（例如 controller 不再是 K）时，B 在本会话内永久失效，以 `PeerLost` 请求退休。暂时不可读、加载中的不完整拓扑不冒充已移除；保持中性，按实际错误处理。

U10 已授权此防御处理。现有结论未证明所有相关分支都不会发生，工程方案保留原因码、C++/Rust codec 与 Bye 映射，但不增加第五轮场景。已确认原生移除时不重复 RemoveCoPlayer；未知归属不删 Player，无法安全完成清理时保留 cleanup_failed/Retiring，网络与可完成的 K 退休继续执行，不伪造 Idle。细节见工程细化第 2.4 节。

## 6. 异常 Join

- **场景安排。** 第 1 轮在开局第一个房间 Join，且没有发生切房，预期原值为 0。第 2、3 轮发生在切房和普通换层之后，没有特殊 reset/回调改写时预期至少有一处原值为 1。ST5 已确认 `Game+0x25AC0 & 0x10` 在 ChangeRoom 置位；已读 Init/SetNextStage 不清该位，特殊 ResetState 不作普通换层的推论。第 4 轮 Continue 按保存值恢复该位；S19 保存位为 1 时预期原值为 1，不能因“Continue 后第一个房间”假定为 0。实际三处值仍逐次记录，不将有限静态闭包当作实机结果。
- **判定。** 在原值为 1 的轮次中：
  - 三处 predicate 每次实际到达的 shim 均调用原函数恰好一次，返回给原生的结果为 0；按 pass/site 记录次数，不能要求每个 site 在整轮中只调用一次（既有两 pass 路径可合计 5 次）；
  - 创建点恰好调用一次，产生完整的 Type 1 / Variant 0 B，角色等于房主指定的角色；
  - A 的身份与健康不变；
  - 没有出现 Baby 或 possessor。

  如果四轮中都没有观察到原值为 1，异常 Join 记 NOT_OBSERVED，不能判通过。
- tx-04-06 的 Join override 预计不需要修改。ST5 的 Baby 位及其置位/保存恢复结论只用于解释和预期，不作为 DLL 的判断条件，不直接改写该位。

## 7. B 的跨房射击（D29 U3：B 为阿萨泻勒）

- **会话。** 第 2 轮用 `cs host 27090 7` 发起，B 为阿萨泻勒，A 不变。ST6 确认 7 进入原生角色表的配置成就筛选，包含 daily/debug 例外；具体 XML achievement 值及本次角色表未静态读取，继续由 `CharacterAvailable` 按值检查。未识别到 role 7 专用的附属 Player 创建分支，但配置、通用 helper 与回调闭包保留 UNKNOWN；仍要求 Join 后恰好两个完整 A/B。
- **操作。**
  1. B 的脚本持续按住一个攻击方向，直到蓄力完成；
  2. 夹具执行 `room`，A、B 一起切到相邻普通房间；
  3. 到达后 B 继续按住至少 2 秒；
  4. B 在有效 tick 中松开。
- **期望。**
  - 从切房开始到松开之前，没有归属 B 的新激光实体；
  - 松开之后的有限 tick 内，出现至少一次归属 B 的激光；
  - 切房的非 tick 窗口内，ordinal 1 的射击 consumer 原调用为 0，并有主动暂停的记录（tx-01-54）；
  - A 不受影响。
- 激光实体只作为“发射”的世界证据；蓄力进度不读取，也不作判定（tx-01-54 的限定）。ST6 已连通本版本 Door→Player vtable `+0xC`→Update→Shooting consumer，以及现有 ordinal 1 暂停逻辑；以 Hook 已安装、Armed、B 被识别且 consumer 被调用为条件。实际松键、激光归属与原调用次数仍由 V12 记录，不以静态路径代替实机通过。

跨房前显式预置一次性的下一代次源状态；新 Ack 到达时先清旧命令历史，再立即重新采集 held、生成 edge=0 新命令，不等待 harness 后续装脚本、不重放旧命令。若仍有有效 tick 缺输入，照实中性消费与记录，不保持旧攻击来迁就场景。Laser 出生与归属观测见工程细化第 4.2 节。

2026-10-26 用户新增决定：重绑后的首条可消费新命令前，Shooting 可按现行规范第9节最多暂停六个 Authority tick；Authority 不等待输入，查询仍中性，不复用旧 held。首条新 current 命令（含真实松键）或上限到期立即结束窗口，其他缺输入仍中性消费。V12 保留原始空档，只有逐 tick/实际分派证明的恢复暂停才计入有效射击连续性；采用实时 witness，不能用帧尾快照错置激光时刻。详见[本次修复](tx-04-07-v12-recovery-fix.md)。

2026-10-26 采证修复：S15 在跨房操作前增加同角色普通蓄力/松键正对照，前后按住默认 180 Authority tick，实际消费样本和 DLL 时钟仍须证明至少两秒。真实松键以消费到匹配命令的中性 tick 定界；独立 `laser-evidence.json` 列出对照、时长、激光归属和真实连续性空档。对照不代替跨房观察，缺输入不排除，修复后的实机结果尚未知，见[修复说明](tx-04-07-relay-v12-fix.md)。

## 8. 加载期间的查询（D29 U4：Continue 与换层都做）

- **换层（第 1 轮 S12）。** B 已加入时，夹具执行 `stage`。
- **Continue（第 4 轮 S20）。** 先在只有 A、没有会话的 run 中退回主菜单，原生会保存该 run。然后在主菜单 `cs host`、完成握手（Hosting→Warmup），再由 harness 在菜单中选择 Continue。ST4 已确认实际保存为一条有效普通 A 主记录时，原生按该记录恢复；地址可变化，controller 还会被 AutoReassignControllers 重分配。加载完成、A 重新唯一绑定后，Warmup 才创建 K 并 Join。这是既定目标时序；当前源码在 Restore 前提前退休该 Host，后续按第 3.1 节修正，不把静态场景设计写成已可运行结果。
- **两种加载的共同要求：**
  - 加载窗口内（`starting` 或 `stage_transition`，并覆盖其 Start/Restore/Lifecycle owner）没有 StandardRoom/OddPlayer tick，没有 Submit 或 Resolve；原生内部 Room/Player 更新与回调可以发生，不用它们冒充 Authority；
  - 被覆写的查询返回中性，参与者状态不变；
  - 加载标志清零且相关 owner 正常返回后，A、B 都要在稳定 Authority 入口先重新唯一绑定，才接受新鲜输入；不以早期 MC_POST_NEW_ROOM/MC_POST_NEW_LEVEL/PostGameStart 或最后一次 Player 更新替代该边界，不重放加载前的边沿。
- **重复查询。** 两次加载之后的稳定 tick 中，A、B 各覆写动作按相同 tick/代次/资格段/consumer/查询类型/action 的原生重复读取结果必须相同，不一致=0，且没有由查询产生的额外 Resolve/Submit。逐键报告自然读取/重复覆盖；没有自然重复不补调用、不记该子项 PASS。加载中的无 tick 查询另汇总中性与状态不变证据，见工程细化第 4.1 节。
- **已确认的窗口。** ST3：`Game+0x1BA78` 进入普通 `1→2→3→4→0`；FSM 内清零后仍有 ItemOverlay/尾部，所在 Game::Update 直接返回。普通独立完整 A/B 预期保留对象/controller，但特殊角色、物品与 Genesis reset 分支保留 UNKNOWN。转场自己的主线不直接 Join；外层 ProcessInput 在 stage 非零时仍可能调原生 Join pass，C/S 不因此再次合成 Join。K 的日常 poll 与换层专用通知分开记录，完整通知闭包未知。
- **Continue 的 starting。** ST4：菜单淡出完成的条件路径设置 `Manager+0x4AD10`，ExecuteStartGame 正常尾才清零；Restore、AutoReassignControllers、内部 Room 更新与 PostGameStart(1) 均在此前。K 兼容 Hook 常驻，但本轮 K 只在稳定 A 绑定后创建；当前提前退休冲突未修复前，不声称已确认 S20 可走到 K/Join。

## 9. 首轮完整场景集的映射（设计第 13 节）

| 设计第 13 节条目 | 本轮场景 | 判定 |
| --- | --- | --- |
| 启动输入（tx-01-51） | S3、S11、S12、S20 每次绑定或重绑之后 | V6 |
| 普通移动与射击 | S4 | V6、V7 |
| 跨房射击 | S15（B，阿萨泻勒）；A 的跨房沿用 tx-04-05 规则，在 S9 中观察 | V12 |
| 射击优先（tx-01-52） | S4 中 A 的物理键序列。B 的输入在远端已是 canonical 方向，不经过物理源转换 | V7 |
| B 死亡与复活 | S10、S11 | V11 |
| 异常 Join | S14、S17、S20 | V5 |
| loading 查询 | S12、S20 | V13 |
| 对端断开与回到透传 | S13（Stop）、S16（PeerBye）、S18（PeerTimeout）、S21（RunTeardown）；S13 之后 Idle 下 A 原生移动 | V2、V3、V14 |
| 命令桥三种来源 | Lua（S1 `cs host`）、控制台（S6 键入 `cs echo`）、控制口（S13、S14、S17、S20） | V9 |
| 主菜单 Hosting | S1、S20 | V2、V3 |
| REPENTOGON 菜单或控制台屏蔽（D7） | S6 | V9 |
| MC_PRE_UPDATE 跳帧（D9） | S6 | V9 |
| 网络故障（本轮新增） | S7、S8、S17、S18 | V4、V10 |

## 10. 实现前的静态确认（ST1–ST7，D29 U6）

D29 U6 指定的静态工作做法沿用 tx-04-05/06：子智能体用 IDA MCP 逆向，主智能体另行取证审阅。结论写入 `docs/research/tx-04-07-static-confirmation.md`，原始导出放在 `artifacts/repentogon/tx-04-07-static/`。只回答下列问题，不扩大范围；未知的如实记为 UNKNOWN。本轮工程设计不新增逆向分工或以角色身份作为推进前置条件。

| 编号 | 问题 | 影响 |
| --- | --- | --- |
| ST1 | 非 Main 的 co-op Player 被致死后，原生走哪条路径（是否进入 `MorphToCoopGhost`）？Player 是否留在列表中，Player 数是否变化？controller、K、HUD 和设备通知是否有动作？有没有持久写入（死亡计数等）？夹具用 `Kill()` 还是致命伤害才算原生致死路径？ | 第 3.1、5 节；[静态取证与主审完成](../research/tx-04-07-static-confirmation.md)，保留设备通知闭包等 UNKNOWN |
| ST2 | `RevivePlayerGhost` 是复用原 Player 还是新建？是否重新 Init？controller 是否改变？复活后 valid/exists/dead/ghost 各字段的值是什么？ | 第 5.1 节；[静态取证与主审完成](../research/tx-04-07-static-confirmation.md#15-st2-主审范围结论与材料更正2026-10-26)：普通受限路径复用、清 ghost，健康/归属仍须重读；Appear 跳过主要射击块。设备/回调与副作用完整闭包 UNKNOWN |
| ST3 | `Game::StartStageTransition(false, …)` 在 v1.9.7.12 上的执行路径：`stage_transition` 窗口从哪里开始、到哪里结束？换层后 Player 是保留还是重建，controller 是否保持？是否经过 Join 与三处 predicate？K 是否收到设备通知？ | 第 3.3、8 节；已静态取证并主审：参数/FSM/内部更新回调与返回边界已定位；普通 A/B 保留为有条件推导；外层 Join pass 可发生，通知闭包 UNKNOWN |
| ST4 | 主菜单 Continue 的路径与 `starting` 窗口；保存的 run 中只有 A 时，加载是否原样恢复 1 个 Player；Hosting 状态下的 Continue 会不会触发 tx-04-06 的 K 兼容分派 | 第 8 节；已静态取证并主审：按有效保存记录恢复、controller 重分配、K 稳定后创建；当前 Restore/RunExit 提前退休菜单 Host 的冲突待实现修正 |
| ST5 | 三处 predicate 读取的 Baby 标志在 v1.9.7.12 上的位置：切房在哪里置位，换层与 Continue 是否复位 | 第 6 节；已静态取证并主审：切房置位，已读普通换层 helper 不清该位，Continue 按存档恢复；间接闭包 UNKNOWN |
| ST6 | 角色 7 是否受解锁限制进入 manager 角色表；Init 对角色 7 是否有附属实体或特殊分支；切房时门切换 owner 直接调用 Player::Update 的路径上，ordinal 1 的射击暂停是否生效 | 第 7 节、R4；已静态取证并主审：条件化配置筛选、未见专用附属 Player 分支、B 暂停静态链成立；配置/通用 helper 与实际运行 UNKNOWN |
| ST7 | ST1–ST4 各路径上新的持久写入入口（沿用 tx-04-06 的 ST8 观测清单）是否已被现有只读观测覆盖 | 第 3.1 节、V16；已静态取证并主审：现有五入口覆盖不足，需补窗口、保存/删除/附加文件证据及归约语义；未实现 |

## 11. 测试 mod 与 harness

### 11.1 测试 mod

见第 3.3 节。harness 每次运行都生成临时配置，包含 token、端口和夹具动作序列，运行结束后连同测试 mod 一起删除（tx-04-05 起的做法）。

### 11.2 远端与 relay

- 远端沿用 tx-04-06 的 Rust 实现：Hello 重发、First Ack 副本缓存、stale 计数、Bye 连发 3 份、按 generation 清空。每轮先启动 relay，再启动远端，远端目标为 relay 的 R 口。
- 夹具执行 `revive_b` 后，第一份带新 generation 的 Ack 到达时，远端安装新脚本；旧 generation 下排定的命令全部作废。S15 跨房的连续 held 用显式 `ArmNextGeneration` 预置源并在新 Ack 后立即新采样；不是保留旧命令。详见工程细化第 6.3 节。

### 11.3 场景（一次启动，四轮 Host；D29 U5）

开始前按规范第 8 节告知用户：harness 接管键盘约十分钟，同时在本机运行远端客户端和 relay；`SkipIntro=1`，不需要手动跳过动画；Download Helper 按 D20 临时禁用；启动后不要操作电脑。第二个存档已解锁阿萨泻勒，用户已于 2026-10-25 确认（R4）。

**第 1 轮（B=Isaac，以 Stop 结束）**

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S0 | 环境检查、部署、启动、到达主菜单 | 与 tx-04-06 相同 |
| S1 主菜单 Hosting | 测试 mod 在主菜单执行 `cs host`（Lua 来源） | `Idle → Hosting`；未 armed，透传 |
| S2 握手 | 经 relay（`clean`）握手 | `Hosting → Warmup`；只锁定一个对端（relay） |
| S3 开局与普通 Join | harness 开新局 | Join 原值为 0；B 为 Isaac；Active；B 首个新鲜输入用于 completed+1 |
| S4 输入 | B 右移、上移后松开；A 按住 D 的同时 B 射击；A 按射击优先序列 → 1 秒、加按 ← 0.5 秒、松开 ←、松开 → | 与 tx-04-06 S4/S5 相同；A 的 aim 序列为正、负、正、0 |
| S5 炸弹 | A 轻按 E 一次；采样 A 的资源窗口后，B 发一次 bomb edge 并单独采样 | 总区间各恰好一次，各次动作窗口内资源减一；避免两次输入的共同库存消耗混在同一资源差值中（2026-10-26 夹具修复） |
| S6 阻断 | F8 动作 `skip`；打开 REPENTOGON 控制台，键入 `cs echo <token>` 回车，停留 1 秒后关闭；ESC 暂停 1 秒，期间按住 D | 无 tick、不重放；trace 中出现 `source=console` 的命令；暂停恢复后 held 的 D 不产生新边沿 |
| S7 常规故障 | relay 切到 `moderate` 约 10 秒，B 持续执行移动、射击脚本并发一次 bomb edge；随后切回 `clean` | 第 4.5 节的不变量 |
| S8 短断网 | `outage_short` | 会话保持；B 在断网期间为中性；恢复后接受新输入 |
| S9 切房 | F8 动作 `room` | 加载期间无 tick；A、B 都重新绑定；B 仍能移动 |
| S10 B 死亡 | F8 动作 `kill_b`，保持 ≥ 3 个 tick | 第 5.2 节 |
| S11 B 复活 | F8 动作 `revive_b`；远端按新 generation 发送新脚本 | 第 5.1 节：新 generation，同一个 K、ordinal 1，接受新输入，拒绝旧 generation |
| S12 换层 | F8 动作 `stage` | 第 8 节；之后 B 移动 |
| S13 stop | 控制口 `cs stop`；之后 A 原生按住 A 键移动 | Stop 退休，远端收到 Bye；Idle 下透传 |

**第 2 轮（B=阿萨泻勒，以 PeerBye 结束）**

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S14 异常 Join | 控制口 `cs host 27090 7`；握手前 A 原生按住 D | Hosting 下透传；沿第 6 节条件预期 Join 原值为 1、被改写为 0，记录实际值；B 为完整的阿萨泻勒 |
| S15 跨房射击 | 第 7 节的操作；切房使用 F8 动作 `room` | 第 7 节 |
| S16 PeerBye | 远端主动发 Bye | 主机以 PeerBye 退休，不回 Bye |

**第 3 轮（B=Isaac，以 PeerTimeout 结束）**

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S17 丢包握手 | 控制口 `cs host`；握手期间 relay 为 `handshake_loss`，完成后切回 `clean`；B 短暂移动 | 握手只建立一次；Hello 重发与 First Ack 副本只计数，不报错 |
| S18 长断网 | `outage_long` | PeerTimeout 退休；K、B 按 tx-04-06 第 9.4 节退休 |

**第 4 轮（B=Isaac，以 RunTeardown 结束）**

| 场景 | 操作 | 主要观察 |
| --- | --- | --- |
| S19 保存 run | 会话 Idle 时退回主菜单 | 原生保存只有 A 的 run；不是会话结束事件 |
| S20 Continue | 主菜单控制口 `cs host`，握手，选择 Continue；加载后 Join；B 移动 | 第 8 节；实现需修正 ST4 确认的 Restore/RunExit 提前退休，当前源码尚不能完成此顺序 |
| S21 teardown | 退回主菜单 | RunTeardown 退休；`cs status` 与 counters 落盘；正常退出游戏 |

## 12. E2E 验收（本次推进唯一的一次测试）

一次执行，失败只记录，不自动重跑。

| 编号 | 判定 |
| --- | --- |
| V1 加载 | 能力组全部通过；入口 Hook、设备槽补丁与字节补丁的数量与清单一致，各恰好安装一次 |
| V2 透传 | 主菜单 Hosting（S1、S20 握手前）时会话未 armed，查询覆写为 0；S13 之后的 Idle 和 S14 握手前的 Hosting 中，查询覆写为 0、tick 计数不变，A 原生可控 |
| V3 会话 | 四轮状态迁移完整，结束原因依次为 Stop、PeerBye、PeerTimeout、RunTeardown；session generation 1–4 递增（与 A/B participant generation 分开）；没有 `tick_reject` |
| V4 协议与 relay | 每轮握手只建立一次；只锁定一个对端；远端、relay、主机三方计数对齐；`clean` 段没有协议错误；主机主动结束的轮次 Bye 尝试 3 次；S16 主机接受远端的 Bye |
| V5 Join | 每轮 K 恰好创建一次，Join 恰好产生一个完整的 B，角色等于指定角色；第 1 轮原值为 0；至少一轮原值为 1 且被改写为 0；每次实际 predicate 调用 original exactly-once，按 pass/site 计数；创建 original 每轮一次，不把三处 predicate 理解为整轮合计三次 |
| V6 B 输入 | 无故障段中，B 的 Resolve、consumer 读数与 Lua 位置一致；松开后 lead+1 内回到中性；每次绑定或重绑后，首个新鲜输入用于 completed+1；B 的输入只经 Participant[1] |
| V7 A 输入与同 tick | A 的射击优先序列符合 tx-01-52；S4 同一 tick 内 A、B 都有 Resolve，互不串扰；Authority 不等待 B |
| V8 边沿 | S5 中 A、B 的 bomb edge 各恰好一次被接受、在一个目标 tick Resolve，炸弹数各减一；S7 的 bomb edge 至多一次 |
| V9 阻断与命令桥 | S6 的跳帧、浮层、暂停期间，以及 S9 切房期间，都没有 StandardRoom tick，事后不重放；三种命令来源都在 trace 中出现，并返回正确结果 |
| V10 故障段 | 第 4.5 节全部不变量，分别对 S7、S8、S17、S18 判定 |
| V11 死亡与复活 | 第 5.1、5.2 节：观察到原生死亡→ghost；失效后 ≥3 个完整 tick 中 B 中性、无被接受输入，A 有有效输入并继续推进；Invalidate 加一次 B generation，稳定重绑后 Ack 公布，同一个 K、ordinal 1，接受新输入，旧 Ack 代次样本明确被拒绝；没有第二次 K 创建、Join 或 RemoveCoPlayer |
| V12 跨房射击 | 第 7 节：松开前没有归属 B 的新激光；松开后有；非 tick 窗口内 ordinal 1 的射击原调用为 0 |
| V13 加载查询 | 第 8 节：S12、S20 加载窗口内没有 tick、Submit、Resolve；自然查询中性、不改变参与者；相关 owner 正常返回后稳定重绑才接收输入；区分内部更新/回调与 Authority；S20 首次菜单 Host 保持到加载后，K 只创建一次；重复比较不一致=0，并报告逐键自然覆盖，未观察子项不冒记 PASS |
| V14 退休 | 每轮结束时 B 中性；K 的第 0 槽被调用一次且不释放；Player 数回到 1（RunTeardown 由原生清理）；A 不受影响；网络线程已停止 |
| V15 tick | 与 tx-04-05 V4、tx-04-06 V11 相同的 tick 完整性要求，覆盖 A/B 两个参与者 |
| V16 退出与副作用 | 正常退出；没有崩溃转储；存档变化按 D18/D20/D27 判定；`trace_dropped=0`、`context_faults=0`、非主线程调用为 0；按 ST7 补齐 tx-04-06 ST8 的窗口归属、保存/删除及官方附加文件证据，记录入口、条件和可确认结果；未归属/未知显式列出，不将允许的原生写入套用旧零变化断言；每日备份按 D27 排除 |

**预先列出的失败情况：**

| 编号 | 情况 |
| --- | --- |
| F1 | 故障段中出现协议错误、会话意外结束，或三方计数对不上 |
| F2 | Authority 因 B 缺输入而等待；故障段中边沿被重复应用 |
| F3 | B 死亡后仍有非中性 Resolve 或被接受的输入；A 被影响 |
| F4 | 复活后没有重绑、generation 没增加、绑定到别的设备，或旧 generation 输入被接受 |
| F5 | 复活或换层时第二次创建 K、再次 Join 或调用 RemoveCoPlayer |
| F6 | 原值为 1 的 Join 产生了 Baby，或 B 角色错误 |
| F7 | B 未松键却在跨房后发射 |
| F8 | 加载窗口中出现 tick、Submit 或 Resolve，或加载后重放旧边沿 |
| F9 | 重复查询结果不一致 |
| F10 | 退休后 B 或 K 残留，查询仍被覆写，网络线程未停止 |
| F11 | trace 丢弃、上下文错误或非主线程访问游戏对象 |
| F12 | 崩溃、卡死，或出现不允许的存档变化 |

**artifact：** 与 tx-04-06 相同，另加每轮 `relay-N.jsonl` 原始包/决定/收尾账本、夹具动作与激光出生事件、加载 owner/生命/代次链、逐键重复查询覆盖、跨 loop 持久窗口与入口结果、文件变更通知和窗口快照、推进率基线/预设容差，以及死亡、复活、换层、Continue 截图。保存完整 scenario/随机种子/源码与执行命令，失败保留已收集证据；不计算哈希。

## 13. 用户决定与风险

### 13.1 已确认（2026-10-25，D29）

| 编号 | 事项 | 决定 |
| --- | --- | --- |
| U1 | 故障注入的安排 | 故障只放在专门的场景段，按不变量判定；强度按第 4.3 节的建议 |
| U2 | 测试 mod 能否修改世界 | 允许（令 B 死亡、复活、换层） |
| U3 | 跨房射击时 B 的角色 | 阿萨泻勒（`cs host <port> 7`） |
| U4 | 加载场景 | Continue 与 B 已加入后的换层都做 |
| U5 | E2E 形态 | 一次启动，多轮 Host |
| U6 | 静态确认的做法 | 沿用 tx-04-05/06：子智能体用 IDA 逆向，主智能体独立审阅 |

### 13.2 起草中新增的事项（2026-10-25 用户按建议确认，D30）

| 编号 | 事项 | 决定 |
| --- | --- | --- |
| U7 | 第 3 轮握手期间施加 30% 丢包 | 做。Hello 重发与 First Ack 副本缓存规则目前从没实际起作用过；12 次重发全部失败的概率约 3×10⁻⁴ |
| U8 | 夹具动作的触发方式 | 沿用 F8，按配置中的动作序列依次执行；不新增按键，避开 F9 和其他可能冲突的热键 |
| U9 | 会话结束后 `join_state` 的取值 | 退休时复位为 `None`；最近一次 Join 的结果另用 `join_last` 字段保留 |
| U10 | B 被原生移除或身份不再匹配时的处理 | 新增原因码 `PeerLost`，会话有序退休；如果 ST1/ST2 确认本轮场景中不会出现，就只保留防御分支 |
| U11 | 四轮的结束方式 | Stop、PeerBye、PeerTimeout、RunTeardown 各一轮。PeerBye 在 tx-04-06 中没有覆盖过 |

### 13.3 风险

| 编号 | 风险 | 处理 |
| --- | --- | --- |
| R1 | 原生致死会把 B 移出 Player 列表，或使 controller 失去 K | ST1/ST2 已确认普通受限死亡→ghost→复活路径保留原对象与局部 controller；特殊角色/回调/间接通知不作普遍保证。第 5.3 节保留防御，不靠猜测放宽绑定条件 |
| R2 | `ReviveCoopGhost` 重新 Init 或换了 controller，或 Appear 影响射击消费 | ST2 主审排除普通受限路径直接重新 Init/重分配 controller，确认 Appear 跳过主要射击块；健康/稳定重绑与动画分开观察，先观察移动，动画结束后再断言射击；身份矛盾按第 5.3 节处理 |
| R3 | 故障段中的计时与游戏帧耦合，导致段内样本过少（例如没有发生乱序） | 段长约 10 秒；没观察到的项记 NOT_OBSERVED，不自动延长或重跑 |
| R4 | 第二个存档没有解锁阿萨泻勒，`CharacterAvailable` 拒绝创建，导致第 2 轮失败 | 用户已确认第二个存档解锁了阿萨泻勒（2026-10-25）；ST6 确认原生配置成就筛选及 daily/debug 例外，实际 XML 值与表内容保留未知，继续按值检查；不修改解锁或存档 |
| R5 | 死亡、复活、换层、Continue 引入新的持久写入（统计、解锁） | 第二档的写入按既有规则允许；ST7 已识别现有观测漏项，按第 3.1 节补齐 V16 入口/窗口证据和归约，不翻转 readonly、不回滚；不把 Continue 恢复等同于必经保存 |
| R6 | `PeerProtocol` 结束路径没有被任何场景覆盖 | 本轮不注入内容损坏（第 4.2 节）；在结果中列为未覆盖项 |
| R7 | 单次启动场景很长，前段失败会让后续场景无法执行 | 失败只记录；未执行的场景记 NOT_RUN，各 V 项能用已有证据判定的照常归约（D26 的 harness 修正） |
| R8 | Continue 恢复的 run 状态与新局不同（例如房间已清理），影响 B 的移动场景 | S20 只要求 B 产生移动，不要求固定方向；方向按观测选择（沿用 tx-04-06 S8.move 的做法） |
| R9 | 当前 Restore/RunExit 在原生 Continue 前退休菜单 Host，S20 无法到 K 创建/Join | ST4 已静态确认；按第 3.1、8 节修正首次菜单加载与已有 run 恢复的生命周期区分，同时处理两个停止点；本轮没有修复或运行场景，不放宽旧 run 退休/唯一绑定要求 |
