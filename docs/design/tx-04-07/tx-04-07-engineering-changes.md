# tx-04-07 工程改动细化

> 2026-10-26 第 3.4 节已按本文接入 relay/远端/四轮场景/V1–V16 与文件通知/窗口快照。3.1–3.4 已统一纯编译链接，尚未部署、启动游戏或运行测试；见[编写记录](tx-04-07-harness.md)。下方设计时的“拟/待实现”保留历史语境，当前入口为 `harness e2e tx-04-07`，一次整体 E2E 仍未执行。

> 2026-10-26 第 3.2 节状态/trace 已编写，当前接口以[状态与 trace 编写记录](tx-04-07-state-trace.md)为准：查询覆盖按 ordinal/consumer 分行，追加场景检查点与完整键累计；不再使用下方前序记录的单条合并 coverage。未构建或测试，全目标的一次整体 E2E 尚未执行。本文末尾“本轮文档细化”的无生产代码修改描述属于 2026-10-25 的历史工作。

日期：2026-10-25。状态：**工程设计；2026-10-26 已按本文完成 game/ 层编码与第 3.3 节测试 mod，尚未构建或运行**。本文件把[实现方案](tx-04-07-implementation-plan.md)第 3 节落实到当前源码；场景编号 S0–S21、验收编号 V1–V16 和 D29/D30 的用户决定不变。

编码落点（2026-10-26，均未编译、未部署、未运行）：新增 `game/remote_life.h/.cpp` 与 `game/persistence.h`；改写 `game/` 的 state/lifecycle/topology/join/peer/query/consumers/tick/persistence/bridge，`core/session`、`net/protocol`、`net/worker` 追加 `PeerLost` 与 Ack stamp 命中标志，`ops/command.cpp` 承担 RestoreScope 与 `ClearPendingRunMutations`；`binding/hooks.json` 增加 8 个 `observe` 组只读入口，`binding/v1_9_7_12-static.json` 记录其唯一特征，`gen_binding.py` 生成对应 wrapper，`CMakeLists.txt` 接入新源文件；Rust 远端同步协商 build 标识与 Bye 合法值。测试 mod 同步完成第 3.3 节：`main.lua` 新增 tx07 配置驱动 F8 动作、B 死亡/复活/换层、生命/动画/人数/房间观测、`MC_POST_LASER_INIT` 激光出生与 A/B 归属、`harness_config.lua` 模式与超时字段，并新增事件/配置说明。构建与唯一整体 E2E 仍留到全部实现完成后执行。

2026-10-26 ST2 主审同步后的代码适配（同样未编译/未运行）：复活的 Lost→Revived 与 `BindRevivedRemote` 都以 `PersistenceWindowCallReturned(BRevive)` 为门，动作正常返回前不进入 Revived、不绑定、不推进代次（对应“ghost 先于下游清零”）；loading owner 事件补 `owner`/`owner_token`，loading episode 记录 `starting/room_transition/stage_transition` 的进入来源与 admission 前后计数；失效 trace 带 `epoch` 与 `reason_class`（room/pause/stage/revive 等），重绑事件同时保留原始 reason；查询覆盖在会话结束时分键输出一条有界 `query_coverage`，并新增 `query_keys_observed/query_keys_repeated` 摘要。

依据为[静态确认报告](../research/tx-04-07-static-confirmation.md)及当前 `mod/`、Rust harness、Lua 夹具的源码阅读。ST1–ST7 已取证并主审；ST2 于 2026-10-26 回读当前 IDB 与官方源码完成审核，见报告第 15–18 节及[主审材料](../../artifacts/repentogon/tx-04-07-static/st2-review/README.md)。2026-10-25 细化时仅有代理材料的历史保留；当前使用有条件主审结论，仍不把 UNKNOWN 写成保证。本次 ST2 材料审核不包含上述编码的验收。

本轮交付为文档细化，属于 tx-04-07 的分析/设计工作，不另起推进编号、不执行阶段测试，也不消耗功能完成后的唯一一次整体 E2E。实现时为新增功能、核心函数、数据结构写中文注释。

## 1. 文件与责任

路径相对仓库根目录；“新增”表示拟新增文件。

| 文件 | 工程改动 | 对应验收 |
| --- | --- | --- |
| `mod/src/game/state.h`、新增 `game/remote_life.h/.cpp` | 替换 `remote_life_lost`；保存生命状态、死亡窗口、当前归属与计数 | V6、V11、V14 |
| `mod/src/game/lifecycle.h/.cpp`、`topology.h/.cpp` | B 单独失效/重绑；先识别加载窗口，再作稳定拓扑判断；细分可读性与身份错误 | V3、V11、V13 |
| `mod/src/game/bridge.h`、`mod/tools/gen_binding.py`、`mod/src/ops/command.cpp` | Restore 的统一决策、正常返回/异常观察；保留首次菜单 Host；继续清理旧排队变更 | V2、V3、V13 |
| `mod/src/game/peer.h/.cpp`、`tick.cpp`、`mod/src/net/worker.h/.cpp` | 按原因统计远端丢弃；Ack 发布代次/记录命中标志；生命、查询、加载窗口的状态/trace | V4、V6、V11、V15 |
| `mod/src/game/join.h/.cpp` | `join_state=None`、`join_last`；退休使用当前已确认的 A/B 关联，处理 B 已被原生移除 | V5、V14 |
| `mod/src/game/consumers.cpp`、`query.cpp`、`state.h` | 按完整查询键比较自然重复读取；无 tick 中性查询汇总；保留现有 Shooting 暂停机制 | V11–V13 |
| `mod/src/game/persistence.cpp`、`bridge.h`、`mod/binding/hooks.json` | 跨 loop 操作窗口、保存/读取/删除观察、原返回值记录；统一清单注册 | V1、V16 |
| `mod/src/core/session.h/.cpp`、`mod/src/net/protocol.h/.cpp`、`game/peer.cpp` | U10 防御路径使用 `PeerLost`；补全命名、合法值和 Bye 映射 | V3、V14 |
| 新增 `tools/harness/src/relay/mod.rs` | 双 socket 转发、确定的随机决定、定时发送队列、档位切换、逐包记录与收尾 | V4、V10 |
| `tools/harness/src/remote/mod.rs`、`remote/protocol.rs` | 启动样本、重绑后清历史、受控旧 Ack 样本、预期超时收尾；识别 PeerLost | V4、V6、V10、V11 |
| 新增 `tools/harness/src/tx07.rs`、`tx07/evidence.rs`、`scenarios/tx-04-07.json` | 四轮单启动编排、Continue 导航、S0–S21、V1–V16 | 全部 |
| `tests/e2e-mods/isaac-cs-test/main.lua`、`harness_config.lua` | 配置驱动 F8 动作、生命/激光/加载观测，保留 tx05/tx06 模式 | V5–V13 |
| `tools/harness/src/main.rs`、`saves.rs`、`win32.rs`、`mod/CMakeLists.txt` | 新入口/文件接入、附加文件证据、所需菜单按键；沿用现有构建与快照工具 | V1、V16 |

core 的 canonical codec、Participant 队列、dedupe、Resolve、时钟与 tick 流水线继续复用。此次不为 B 另造输入通道，不更改网络身份或把 Player/device 指针放入报文。

拟采用的内部接口如下，命名可随实现作局部调整，职责不变：

| 接口/数据 | 输入、输出与调用位置 |
| --- | --- |
| `ObserveRemoteLife(SessionRuntime&, const Topology&)` | 主线程已读取的 B 拓扑；产生迁移/撤销资格/退休请求，不直接 Bind；在 RefreshLifecycle 的非加载观察段调用 |
| `BindRevivedRemote(SessionRuntime&, const Topology&)` | 只在稳定 Authority 入口接受重新读取的唯一关联；使用原 Participant::Bind 并记录新当前归属 |
| `RestoreScope(game, state, startGame)` | 捕获一次首次菜单加载/已有 run 决策；构造时处理停止与排队清理，析构观察正常/异常；整个 original 处于 Restore owner |
| `ClearPendingRunMutations()` | 从现有 RunExit 拆出的清队列辅助函数；不改变已生效 session，用于两种 Restore 决策与实际退出 |
| `RetirementPlayerSnapshot` | 在移出 K 前保存当前 Game/manager/A/B/controller 的只读关联；移出后须重新从列表确认，才能调用原生移除 |
| `QueryRepeatBucket` | 首次值、读取/重复/不一致计数；按第 4 节的完整键固定容量存储，只作诊断 |
| `PersistenceWindow` / `PersistenceCallScope` | 主线程窗口 id 与同步 call/parent id；scope 显式接收正常返回值，异常仅记录未正常返回 |
| `Relay::set_profile / snapshot / finish` | 有界消息、档位确认 epoch、值快照与收尾结果；finish 归还线程/socket 并保留未发包计数 |

这些接口不从网络线程访问 Game/Player/Lua；主线程把值复制给网络、trace 或 harness 后，后台只处理值。

## 2. B 的生命状态与当前归属

### 2.1 数据与转换

拟新增 `RemoteLifeState { Alive, Lost, Revived }` 与 `RemoteLifeData`。Join 前或会话结束后，status 显示 `b_life=None`，不把尚不存在的 B 记成 Alive。数据保存状态、死亡事件编号、首次观测 loop/tick、是否等待复活重绑、`deaths/revives/rebinds`，以及当前确认的 Player/K/controller 关联；所有数据由主线程修改。

`Alive` 表示生命可用；是否已经绑定由 `b_bound` 单独表达。切房/暂停使绑定失效，不自动增加死亡/复活计数。转换如下：

| 当前状态/事件 | 动作 | 后续资格 |
| --- | --- | --- |
| Join 正常返回并确认完整 B | 建立首次生命记录与当前归属 | 按原稳定入口激活 |
| Alive，首次读到 `dead || ghost` | 进入 Lost；撤销 B 绑定，清队列/current/previous/edge；记录死亡事件一次 | A 与 session 仍 Active，B 中性 |
| Lost，仍 dead/ghost | 更新观测，丢弃 B 新输入并计数 | 不再次 Invalidate，不再次计死亡 |
| Lost，健康与身份全部满足 | 进入 Revived，记录一次复活观察 | 此时仍不接收 B 输入 |
| Revived，稳定且未阻断的 Authority 入口 | 重新读取唯一 exact pair，Bind；更新当前归属，记录重绑；进入 Alive | TopExit 可公布新代次 |
| Revived，再次 dead/ghost，尚未重绑 | 回 Lost，保留同一死亡事件 | 不重复计数或产生额外输入窗口 |
| removed/稳定身份矛盾 | 关闭 B 输入，按第 2.4 节退休 | 不以“复活”接纳错误 Player |

健康条件仍为 Type=1、Variant=0、index=1、valid、exists、非 dead/removed/ghost、controller=K.id、设备列表中恰好同一 Registered K，且 Player 拓扑唯一；确认角色与本轮指定角色一致。地址变化允许，但新地址必须从当前原生列表重新读取，通过完整条件后才能成为当前关联。旧地址只用于已知受控 consumer 的中性拦截，不作为可解引用对象或身份依据。

ST1 正常死亡→ghost 的 vector 人数保持 2，dead 可能已清零，故人数为 2 与 ghost=true 同时成立是正常情况。Player 列表人数、非 ghost 完整 Player 数、健康人数分开记录，不替换现有列表人数语义。

### 2.2 失效、代次与 admission 的准确顺序

当前 `Participant::Invalidate()` 已经递增 generation；`Bind()` 对未绑定对象不再递增。实现沿用此语义：**死亡时撤销并递增，复活绑定后才对远端公布这个代次**。不在 Revived/Bind 再手动加一，不在每次 Lost 轮询中加一。无其它失效事件的单个死亡窗口，新旧 B generation 相差 1；期间另有暂停/加载时，分别记录其独立失效原因。

`InvalidateRemote` 对已绑定的 B 只调用一次 Participant Invalidate，并保留一次输入丢弃边界。Lost 的持续轮询依赖状态拒绝输入，无需反复推进边界。`DrainPeer` 增加按原因累计的命令条数：`life_lost`、`loading_or_blocked`、`unbound`、`old_generation`、`old_window`、`sequence_floor`、`inactive`；冗余中的每条也计数。计数与 Accepted 分开，现有 `admitted` 是尝试数，不能当作接受数。

Ack 维持已有 `advertised_generation` 规则：Lost/Revived 或阻断时保留最后公布值；稳定 Bind 且可接收输入后才公布。旧输入的代次由 `Input.last_ack` 查 worker 的成功发送 Ack 记录得出，不因包较晚到达就贴上当前 generation；记录缺失时仍拒绝。`ReceivedInput` 补 `ack_stamp_known` 值标志，区分未命中记录与合法 Warmup generation=0，不读取游戏对象。绑定失效后，不得把丢弃的命令或历史边沿重新编码为新代次输入。

如果死亡在一个已开始的原生 tick 中被观察到，立即撤销 B 查询资格，但不撤销之前已经合法发生的动作，也不抹掉该 tick 的 `tick_input`。将失效记在具体观测点；该 tick 按真实正常返回完成。V11 的连续中性区间从失效后第一个完整 Authority tick 开始，不追溯宣称死亡前半个 tick 已经中性。

### 2.3 A 继续推进、稳定窗口与 ST2 边界

`ReadyForAuthority` 在 Warmup 首次激活时仍要求 A/B 都绑定；Active 时只要求 A 与会话/上下文可用，允许 B 为 Lost/未绑定。共用流水线对 B 产生中性结果，Authority 不等待 B；不要把 B 的 Lost 加入使整个 session 停 tick 的全局 Lifecycle block。

`RefreshLifecycle` 先读取 loading/owner 状态并撤销旧绑定；加载中临时列表不完整只意味着不可绑定，不立即按人数变化或 PeerLost 处理。owner 正常返回后的稳定入口，再执行人数、身份与 exact pair 检查。非加载时仍严格要求已 Join 的普通 A/B 列表人数为 2。

ST2 主审确认普通受限 Isaac 路径在原对象上清 ghost、恢复生命值，本体不重建/Init 或重分配 controller；dead/valid/exists/removed 并非被无条件重置，仍有 `Appear`、backup 与条件道具分支。选择 `ReviveCoopGhost()`，调用正常返回后重读实际健康与归属，不以 Lua true 视为绑定成功，也不在下游重入回调中仅凭已清 ghost 提前公布代次。夹具记录 `IsExtraAnimationFinished()`；重绑后先观察移动，两个移动 consumer 未读动画标志不构成立即位移保证。主要射击块在 Appear 期间会被跳过，射击断言在实际动画完成后开始，不主动终止动画或写控制字段；其他射击分支、设备通知与回调完整闭包保留 UNKNOWN。

### 2.4 PeerLost 与安全退休

U10 已允许 PeerLost。ST1/ST2 排除普通受限死亡/复活局部路径主动移除 B 的需求，但完整窗口与特殊分支闭包仍未证明，因此本方案保留 **可达的防御处理与原因码**，不把四轮正常结束方式改成 PeerLost，也不增加第五轮测试。

在 `SessionCause` 与 `ByeReason` 末尾追加 PeerLost，保留现有数值；同步 C++/Rust codec 的合法值、名称、主线程映射与协商 build 标识。沿用当前协议布局；这是本次 C/S 成对更新，旧客户端通过既有协商拒绝。网络 worker 本身不读取 Player，也不新增“探测 B 死亡”的逻辑。

区分三种情况：

1. **暂时不可读/加载中。** B 中性并保持无绑定；读取失败不能冒充已确认移除。持续异常按现有 RuntimeFailure 记录并停止修改。
2. **稳定且确认被移除或归属矛盾。** 请求 PeerLost，停 admission/publication，按已有次序中性 K、移出设备 vector、停止网络；不重试 Join，不创建 K。
3. **Player 清理。** 只对当前列表中再次确认归属本 session/K 的 B 调一次 RemoveCoPlayer。已确认 B 被原生移除时跳过此调用；身份矛盾时不移除未知 Player。无法安全确认清理完成则保留 `cleanup_failed`/Retiring 并报告残留，不伪造 Idle；网络和可完成的 K 退休仍执行。

`RetireJoinedPlayer` 当前比较最初 `join.joined_player`、`join.host`。改为区分“首次 Join 历史”和“当前已确认归属”：后者在每次稳定 A/B 重绑后更新；退休时再读当前 Game/manager/列表与 A 的 exact pair，防止跨房/复活更换地址导致误用旧指针。不得用 `run_torn_down=true` 掩盖 B 单独丢失；它仍只表示原生整体 run 清理。

退休顺序仍先移出 K、再移除 B：在 K 尚 Registered 时取得当前归属的只读退休快照；RemoveOnce 正常返回后，重新读取 Player 列表/身份并与该快照比对，再调用一次 RemoveCoPlayer。此时 K 已离开设备 vector，不得要求 `ReadTopology(1)` 再返回 Registered/unique 来证明 B 可移除；使用独立的受限读取确认当前 Player/controller 与已确认的自有 K 归属。快照只含诊断值和关联，不跨原生调用盲目解引用旧地址。

## 3. Continue 与加载边界

### 3.1 一次 Restore 决策处理两个停止点

把生成 wrapper 中相邻的 `RestoreBeginning(); RunExit();` 替换成一个主线程 Restore scope，统一得出 `PreserveInitialHost` 或 `RetireExistingRun`。保留条件同时满足：Host、Hosting 或握手后的 Warmup、会话未退休/失败、`run_observed=false`、从主菜单等待首次 run、K 为 Empty、未 Join、没有旧有效绑定、原生 `startGame=true` 且处于已识别的 Start/starting 路径。不能只凭 Warmup 或 `startGame` 豁免已有 run。

`run_observed` 改为“观察过非加载的有效 run”，而不是在 starting/Restore 内首次读到 run=true 就置位。首次加载资格在外层 Start scope 进入时固定，同一加载窗口内不因半成品拓扑反复变化。Host 若在已有 run 中创建则不得获得首次菜单加载资格。

对 Preserve 分支：保留当前 worker、握手/session identity 和菜单 Host；清空输入样本、旧关联及所有排队变更，保持 loading，中性；不调用两处 RunTeardown。将 `RunExit` 的“清排队变更”拆为无会话退休副作用的辅助函数，两个分支都复用。保留的是已经生效的本轮 Host，不保留排队的旧 host/solo 请求。

对 Retire 分支：沿用已有 run 的提前退休与原生清理语义，两个停止点合并为一次请求。`Game::Exit` 仍是实际退出路径，不能随 Restore 豁免。scope 观察原调用 exactly-once、正常返回与异常；异常/上下文不一致时停输入并记录原始失败，不继续 Join。

### 3.2 何时可以创建 K 和重绑

为 Start/Restore/Lifecycle owner 增加有界的 enter/return 事件与 loading episode，记录 `normal`、loop、owner token、starting/stage 值。owner 正常返回后延后至后续稳定 Authority 入口；FSM 内 state 已清 0、MC_POST_NEW_ROOM/LEVEL、PostGameStart 或内部 Room/Player Update 均不提前授予资格。

S20 的具体顺序：

```text
菜单 Host → 握手 Warmup（K=Empty）
→ 菜单预读存档（独立 read 记录，可能早于 starting）
→ starting/Start → Restore（保留首次菜单 Host）
→ AutoReassignControllers → 内部 Room 更新/PostGameStart
→ starting 清零、相关 owner 正常返回
→ 后续稳定入口读取当前 A 唯一关联 → 首次 CreateOnce K
→ 合格的原生 Join pass → 确认完整 B → 稳定入口 Active
```

预读若发生在 Host 之前，证据按真实时序保存，不能为配合此图而移动原生预读。保存记录 count=1、恢复后当前列表 count=1、A 的最终 controller 分别记录；不要求 A 地址或 controller 等于保存前值。K 在本轮恢复期间不存在；进程中前几轮已 UNREGISTERED 的常驻 K 不重新注册。

S12 保持已有 K 与 Join 记录；stage=1–4 及 owner 尾部均禁止 tick/Submit/Resolve，稳定后 A/B 重绑。原生 ProcessInput 在 stage 非零时可能执行 Join pass，其自然发生不等于 C/S 再次 Join；`join.complete` 时不合成 Join 输入。

## 4. 查询观测与跨房射击

### 4.1 扩展现有炸弹观测

当前 `RecordQuery` 只比较 Bomb/Triggered/action=8。扩展为固定容量数组，每个 ordinal 按 `(tick, participant_generation, eligibility_epoch, consumer, QueryKind, action)` 分桶，桶内保存首次有限结果、读取数、重复数、不一致数。每次合法性变化递增观测 epoch 或结束当前比较段，记录原因；不拿失效前非中性和失效后中性作“同一快照不一致”。

移动/射击 action 0–7、炸弹 8、主动道具/卡牌/丢弃 9–11 均纳入；Pressed/Triggered/Scalar、不同 consumer 分开。每 tick 只汇总总数与掩码，首个 mismatch 额外给出准确键和值；按场景/结束输出分键累计覆盖，避免逐查询 trace 撑满现有 512 项队列。原 bomb 字段可作为派生字段保留。查询观测只修改诊断计数，不 Submit、不 Resolve、不消费 edge，不额外调用原查询。

无 tick 的加载查询另按 loading episode 汇总 `queries/nonzero/unknown_context`，比较调用前后 admission/Resolve 计数与 participant generation/queue 变化的归属。加载进入时的合法 Invalidate 单独记录；不把“加载造成一次失效”误判成“查询修改了参与者”。已有 RecordQuery 的无 tick 早退不能继续作为 V13 的唯一证据。

V13 要求不一致=0，同时报告每个键的实际读取/重复覆盖；某动作没有自然重复读取则记 NOT_OBSERVED，不伪造该动作已经验证。原生加载窗口无自然查询同理；窗口内没有 tick/Submit/Resolve仍可分别判断，不能从零查询推出查询返回中性已获观察。生产 DLL 不为补覆盖主动增加查询。

### 4.2 Shooting 与激光证据

ST6 已确认 Door/NestedDoor→Player::Update→Shooting 的静态链，现有 `ConsumerScope` 对 ordinal 1 的主动暂停沿用。仅补 `shooting_dispatch` 的 session/participant generation、owner token、room/loading episode，并配对正常原调用记录；非 Shooting wrapper 仍调用 original exactly-once。

Lua 在实体初始化回调中观察新 Laser，记录实体本次生命周期标识（如 pointer 标识与 InitSeed 的组合）、frame/loop、spawner/parent 可确认归属、当前 A/B 生命身份；首次扫描已有实体单列为 baseline，实体移除/换房后重新编号，不以易复用地址单独去重。归属链不明记 UNKNOWN，不强行归 B；回调只观察，不改变实体。

S15 的充能与跨房使用 clean relay，先在新绑定后安装新鲜 held 脚本；F8 切房前，harness 另行预置一次性的下一代次源状态（见第 6.3 节）。跨房使代次变化，远端丢弃旧命令历史后立即对该源重新采样，生成新的 edge=0 held 命令，不等待 harness 再发一次脚本消息。这是持续按住的源状态重新采集，不复活旧命令。松开前不得出现 B 新激光，松开后有限窗口内至少一次，窗口上限在 scenario 中预先固定；若网络时序仍导致有效 tick 缺输入，按真实中性消费及可能的发射记录失败，不保持旧攻击、不暂停正常 tick 来迁就夹具。A 的身份/推进独立检查。

## 5. 持久化窗口、入口与文件结果

### 5.1 操作窗口的生命周期

新增主线程 `PersistenceWindow` 记录，固定容量且允许嵌套，包含 id、kind、起止 loop、session generation、相关 loading/death episode、触发来源、结束原因。TLS `PersistenceOperation` 保留，用于 join/device_register/player_remove 等同步调用；异步窗口不依赖它跨帧存活。

| 窗口 | 开始 | 正常结束/异常结束 |
| --- | --- | --- |
| b_death | 受控 B 的原生 Kill wrapper，original 前 | 原生 ghost 结果已观察且相关调用正常返回；意外自动复活/移除/超时单列 |
| b_revive | B 的 RevivePlayerGhost wrapper，original 前 | 调用正常返回并观察健康；保持与随后 stable rebind 的关联；不以 bool 代替结果 |
| stage_change | StartStageTransition wrapper，original 前 | stage 清零、owner 正常返回、后续稳定观察；未进入转场记 rejected |
| continue | 首次菜单加载的 Start/Restore episode | owner 正常返回后的稳定 A；预读用独立 read call id 关联，不提前伪造此窗口 |
| run_exit | 实际 Game::Exit 与保存/清理 | 原调用返回；与此前 continue/stage 分开 |

每条持久入口记录包含 call_id、parent_call_id、同步 operation 与有效 window ids，而不是只保留一个覆盖其它窗口的标签。无法关联的写入仍输出 `operation=none/windows=[]` 并参加报告；事件丢失/容量不足有明确错误，不丢弃后给出“无副作用”。来源由真实 native entry 标记，夹具动作编号只作关联，不由 DLL 硬编码 S10/S12。

### 5.2 明确采用的入口

继续保留 pgd_unlock/counter/local/cloud/button_maps 五入口；TryUnlock wrapper 补正常返回的 bool，区分“调用过”“返回 true”“观测字段变化”。RAII 记录正常/异常；不在异常路径虚构返回值，保留 original 的返回与异常传播。

统一清单新增候选入口如下，均为只读观察、original exactly-once：

| 入口 | 本次责任与依据 |
| --- | --- |
| Entity::Kill `0x2A7ED0` | ST1；仅识别当前归属 B，开始跨 loop 死亡窗口，不更改死亡结果 |
| Entity_Player::RevivePlayerGhost `0x3CFC60` | 官方 thiscall void() 声明与 ST2 主审；原对象同步清 ghost，实际健康/归属/动画分别观察，不外推回调完整闭包 |
| Game::StartStageTransition `0x2F9B30` | ST3 已确认参数 ABI；请求前开始观察，返回后记录是否真正进入 FSM |
| Manager::LoadGameState `0x5332F0` | 官方 `void(int saveslot)`；记录预读 slot、调用结果边界，不称为必经保存 |
| GameState::write/read | 官方 `bool(GameStateIO**)` / `bool(GameStateIO**, bool)`；记录原始调用与 Hook 链结果的可见范围、io/slot 可知性 |
| GameState::Delete/DeleteRerun | 官方 void()；记录删除请求与正常返回，文件删除成功另由文件证据判断 |

`Manager_SaveGameState 0x532160` 的调用链已经定位，但没有因已有反编译就选用推断原型注册；此轮优先在上述官方 ABI 的 GameState 入口捕获实际保存。后续如需 Manager 层请求事件，再依据寄存器/栈与源码完成其 ABI 描述。write_rerun/read_rerun 不在首轮目标中，仅在结果列出覆盖边界，不为它们增加调用或测试。

所有新入口及特征加入 `hooks.json` 与对应静态 binding 数据，由 `gen_binding.py` 生成 wrapper/manifest；清单标明用途与透传行为。持久观察范围尽量包住官方 Hook 链，优先级与实际链在 artifact 记录；未能证明官方链整体被包住时，只声称已观测层的结果。

### 5.3 文件证据的选型与许可边界

本轮采用 **GameState/PGD 入口事件 + harness 有界文件变更监听 + 窗口前后快照 + 官方日志**，不对整个 Win32 文件 API 加全局 Hook。harness 监听实际 Documents 存档目录、Steam cloud 目录及已发现的 REPENTOGON sidecar 目录，记录原始变更通知的时刻/路径/类型；溢出、文件仍在写、无法读取均显式报告。

在 S10/S11/S12/S19/S20/S21 的窗口前后由 harness 只读复制相关存档与 sidecar，记录存在/缺失、长度、字节变化、slot、采样开始/结束时刻；运行首尾仍保留完整 SaveRoots 快照。重点包括 ESSM binary/ex JSON、VirtualRoom、ItemPool、achievements<slot>.json。发现路径后按官方实际命名建清单，不用泛化的 `*.json` 或宽泛目录放行。

文件通知只能说明变化，不证明调用者或写入成功；前后相同不能排除中间写后恢复。多个官方子写操作的独立结果（尤其 ESSM binary 与 ex JSON）只有日志/文件证据充分时才标已知，否则逐项 UNKNOWN。云端远程提交结果也不能用本地 cache 文件变化替代。如此明确观测范围，不声称 ST7 的全部间接闭包已闭合。

新 `tx07/evidence.rs` 取代 tx06 的 `no_added_writes`：分别归约入口完整性、窗口关联、实际文件变化的许可。第二档主存档与 D18 已允许的附属文件按既有规则；第一/三档仍用 D20 的末尾 Counter/Checksum 基线范围；每日备份按 D27 原样快照但排除判定。新增发现的 achievements JSON 等文件先记录与归类，不因需要观察就扩大修改白名单；超出既有授权的变化仍报告未允许/待确认。不翻 readonly、不回滚存档，也不强加旧“operation 内必须零变化/不得保存”的断言。

## 6. Rust relay 与远端

### 6.1 relay 的线程与有界队列

每轮 relay 使用独立 R/H 两个 socket、两个方向随机源和一个发送时间排序的有界队列。harness 通过有界控制消息切换档位/请求快照/收尾；relay 线程不访问游戏、Lua 或控制口。地址锁定本轮 remote 与 authority，外来包记录并丢弃。

参数沿用总方案第 4 节，固定种子、延迟分量≤5000 ms、概率≤1000000 ppm，预先校验队列容量和档位；决定日志含 PRNG 版本/种子、方向内到达序号与所用随机值。一次决定后不因发包失败重新抽签。codec 只作日志解码；无法解码的原始包仍照既定故障决定转发。

档位切换的生效点是 relay 已确认的队列消息序号与其单调时钟，不用 harness 发消息的时间冒充。已经排定的包保留原决定与 profile_epoch，切回 clean 不改写其历史；新场景开始前等待旧排队包完成或明确记录清理丢弃，防止故障尾包混入死亡/跨房场景。长断网一直保持到双方实际超时；主机三次 Bye 也属于丢弃，不提前切 clean。

逐包日志保存原始 bytes（不算哈希）、方向、occurrence_id、解码 kind/packet sequence、决定、计划/实际发送时间、每份 copy_id 与 send 结果。Hello/Bye 原字节重发可有相同 packet sequence，须按 occurrence 计数，不能只按 sequence 合并。

### 6.2 三方计数与性能不变量

只在一轮收尾、两端停止发送且 relay 完成有界 drain 后比较最终计数，不比较运行中的非原子快照。分别核对：双方成功 tx 与 relay 收包、relay 成功 send 的份数与双方 rx；转发记录同时给出 `received / policy_drop / copies_scheduled / send_ok / send_error / pending / shutdown_drop`，副本不算源端多发。若本地 UDP 丢失或尾包未能核对，明确列出差额使 V4 不通过，不用调整计数补平。

S7 前预留 clean 的 10 秒有效 Active 基线；S7 采用相同长度，排除明确 pause/loading 段但不排除网络缺输入 tick。以 DLL trace 自身时间轴计算 completed 的推进率，`fault_rate / clean_rate` 预设容差为 0.8–1.2，要求有效样本窗口充足且无等待 B 的记录；阈值写入 scenario/artifact，不根据结果调宽。丢包/重复/乱序必须实际发生，主机 duplicate/stale 也必须>0；未发生记 NOT_OBSERVED，不追加时长。

### 6.3 远端补齐项

1. 当前所有命令均 `target=Ack.tick+lead`，需实现每个新可用绑定代次的启动采样：首个新鲜状态用新的 sequence 发往 `Ack.tick+1`，同样连续量再用新 sequence 发往 `Ack.tick+lead` 且 edge=0（相同 target 时不发两份不同命令）。后续 lead=2。两条均走既有 codec/Endpoint/冗余规则，主机不改 target。记录 Ack 序号、sample id 与两条命令；过期照实拒绝，不延迟 Authority 配合启动样本。
2. 更高 generation 的 Ack 先清脚本/desired/edge/history/last_sample，保留 session 级 command/packet sequence。通常由 harness 等远端确认新代次后再安装脚本。另提供 `ArmNextGeneration` 有界消息：在 S15 的切房前（也可用于 S11 的复活前）预置本 session、旧代次、场景标签及一次性新源脚本；收到更高且 Active 的新代次 Ack 后，清旧历史、消费这份显式预置的源并立即重新采样，避免先发送中性占位再等待控制线程。预置项不可携带旧命令/旧 edge/旧 target，超时或会话结束即作废，不自动继承旧脚本。
3. S11 安排一份 **新 packet sequence、新 command sequence、但引用已知旧 generation 的 last_ack** 的合法 Input，记录它只用于确认旧代次拒绝；不复用老包触发 stale 来冒充 WrongGeneration。保留复活前最近的 Lost Ack，而非很早的死亡前 Ack，以尽量处于既有有界 stamp 历史中。此接口限 Rust 测试客户端，约束为当前 session/合法 CRC，远端正常脚本随后继续。旧 Ack stamp 已过期时分别记 `ack_unknown`，不能声称已证明旧代次分支。
4. 将“期望的 PeerTimeout 终止”保存为 terminal cause，仍保留实际 error，不与协议/IO 错误混淆。只有 S18 且已 Established、实际 elapsed 满足超时的 peer_timeout 可作为预期收尾；握手超时仍失败，不再发送正常 Bye 来伪造主机结束。

## 7. Lua 动作与四轮编排

生成配置包含 `tx/token`、host 端口、动作表及每项超时。F8 顺序固定为 `skip → room → kill_b → revive_b → stage → room`，对应 S6/S9/S10/S11/S12/S15。每个 request 分配 action_id，只允许一个 pending；完成前多按 F8 作为独立非法请求记 rejected，不推进动作序号。正常动作各记录一次 request，之后在 arrived/rejected/timeout 中只选一个终态，无重试。F9 继续不用。

| 动作 | 执行与成功证据 |
| --- | --- |
| skip | 30 次 MC_PRE_UPDATE true，下一允许更新前记录结束 |
| room | 选择已打开的普通相邻门，记录来源/目标；到达目标与 DLL 稳定重绑分开 |
| kill_b | 确认 index=1、完整普通 Isaac、A 健康；一次 B:Kill()，等待原生 ghost。先观察 dead、后观察 ghost，各自记录；未观测到短暂 dead 不补造 |
| revive_b | 仅对已确认 B ghost 调一次 ReviveCoopGhost；记录 bool、实际生命/身份与动画；之后等待 DLL 重绑 |
| stage | 调 Game():StartStageTransition(false,0,nil)，记录进入转场与目标层/房变化，稳定完成由 DLL 窗口与绑定证据决定 |

kill 的意外自动复活、B 移除、错误角色/人数、调用异常均单列原因，不将其作为本轮正常 ghost 用例成功。死亡窗口保持≥3个完整 Authority tick，A 安排新鲜移动输入，远端继续发送以取得 `life_lost` 丢弃证据。

`tx07.rs` 复用 tx06 的启动、部署、第二档、临时 Download Helper 禁用、控制口和失败收集；另写本轮场景/归约，不修改旧 artifact 的结论。Continue 导航先观测菜单，再选择 Continue，不复用新局确认的固定按键链。S19 明确没有会话且当前只有 A；S20 直到正常恢复的稳定 A 后才能创建 K。S21 真实 Game::Exit 才触发 RunTeardown。

`main.rs` 同步接入 CLI 的 tx 名称、`Run::execute/finish` 分派、部署时生成测试 Lua 配置的 tx 分支，以及源码/场景收集。Lua 同步新增 tx07 日志前缀与首轮主菜单自动 Host 分支；第 4 轮仍由控制口 Host，不再次自动创建。拟统一入口沿用现有参数形态：`harness e2e tx-04-07 --game <game-root> --out <new-artifact-directory> [--scenario <json>]`。这是待实现入口，本轮没有执行。

跨 generation 的脚本安装使用明确场景标签与完成确认；控制台 echo 的文本/Enter/关闭顺序有实际命令结果佐证。所有移动方向来自当前夹具可通行观察，操作与判定共享同一方向，避免复用 tx05 历史方向缺陷。前段失败时保留原始失败，后续不能安全执行则 NOT_RUN；可依据已有证据的 V 项继续归约。

## 8. 状态、验收证据与实施顺序

status 增加 `b_life/b_deaths/b_revives/b_rebinds`、按原因 peer discard、loading episode 与 query repeat 摘要。`b_rebinds` 统计首次 Join 绑定以后的成功重绑总数，事件带 reason 区分 room/pause/stage/revive。`join_state` 在退休完成后为 None，`join_last` 保留 Joined/Failed 等最终业务结果；清理失败时继续显示 Retiring 和实际 Join 状态，不先伪造 None/Idle。新 Host 以 Waiting 开始，不继承旧 K 或 life 状态。

| 验收 | 必须新增的证据 |
| --- | --- |
| V1/V5 | 由新清单生成的入口/槽/补丁 manifest；四轮实际安装与 Join 原 predicate/创建次数；不沿用旧硬编码数量。predicate 按每次 shim 到达核对 original exactly-once，按 pass/site 保留次数；两 pass 可合计5次，不断言整轮每处只到达一次。创建 original 每轮一次 |
| V3 | session generation 的四轮递增与独立 b_generation；不得拿复活的 participant generation 当作第五轮 |
| V6/V11 | 原生死亡→ghost、Lost 的≥3完整 tick、A有效输入、B零 Accepted/中性；Invalidate→Bind→Ack→fresh input 全链；旧 Ack代次输入拒绝；K/Join/Remove次数不增加 |
| V12 | clean 输入连续性、B自然激光出生事件、放开时刻、ordinal 1 blocked shooting 的原调用=0 |
| V13 | starting/stage 与 owner 正常返回/稳定绑定时序；非 tick查询中性与状态不变；逐键重复覆盖；Continue saved/loaded count及实际controller；首次菜单Host保留且本轮K只创建一次 |
| V4/V10 | 三方逐包/份数账本、故障实际命中、超时点、推进率基线/容差与缺输入中性 |
| V14/V16 | 当前关联的安全退休、join_last、线程停止；跨loop持久窗口/入口/返回值/文件变化及 UNKNOWN；D18/D20/D27许可判断 |

结果区分 PASS、FAIL、NOT_OBSERVED、NOT_RUN。没有自然查询/激光/乱序等所需观察时不能给该子项 PASS；未知因果/回调闭包按观测边界列出，不把所有 UNKNOWN 一律当成新增前置流程。完整验收仍由同一入口一次执行，失败不自动重跑。

实施按依赖顺序完成以下工作，不把它们拆成新 tx 或阶段验收：

1. 落实 B 生命/当前归属、generation 与 PeerLost 的接口；修复首次菜单 Restore 决策及 loading 稳定边界。
2. 接上查询计数、窗口 trace、持久入口与 status/join_last；保持现有 K、Join 和 Shooting 行为合同。
3. 编写 relay、远端启动/换代/超时/旧 Ack 测试动作及 Lua 配置序列；完成 tx07 单启动编排、文件证据与 V1–V16 归约。
4. 整个实现完成后统一纯构建（不得夹带测试）；静态阅读构建产物清单和最终源码。执行唯一整体 E2E 前单独告知接管操作与停止手动操作的时机，需配合时确认用户准备好。

当前可继续落实上述设计；ST1–ST7 静态主审均已完成，官方 Hook 实际排列/设备通知闭包、动画帧数、真实存档/角色表、文件操作实际结果等仍按真实未知状态保留。后续编码或验收若提供新证据，再更新对应结论，不以本轮静态审核预记功能完成。

本轮文档细化已完成一次统一静态审阅：对照现行规范、静态证据、当前源码与总方案，核对状态/代次、退休与加载时序、场景/验收映射和引用范围。未运行构建、程序、校验脚本或游戏，生产代码与原始静态证据未修改；tx-04-07 功能整体 E2E 尚未执行。
