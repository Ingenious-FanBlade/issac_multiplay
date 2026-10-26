# tx-04-08 ST7：加载、复活、Continue 与当前 Player 读取依据

范围：只做静态源码阅读和 J273 IDB 只读反编译，核对 tx-04-07 的加载、B 死亡/复活、首次菜单 Continue、run teardown 与当前 Player 读取/撤销行为。未改生产代码、公共设计或 IDB；未运行测试、构建、生成器、语法/格式校验、游戏或哈希操作。

## 结论

- STATIC_CONFIRMED：J273 Continue 路径先调用 Game Restore，再调用 PlayerManager Restore、Level Restore、PostLevelInit Restore。PlayerManager Restore 会匹配已有 Player、恢复槽位、处理未匹配对象，并可为缺失槽位分配和构造 Player。Player Restore 也能在特定条件下进入 RevivePlayerGhost。因此这些层次不能合并成“同一 Player 原地回填”。
- STATIC_CONFIRMED：RevivePlayerGhost 在原生函数前段就清 ghost 标志，之后仍重置状态、重算物品、生成 Effect、处理特定角色的关联对象，并沿 backup Player 循环。ghost=false 不是复活完成、身份连续或 world 读取资格。
- CURRENT_SOURCE：tx-04-07 当前会话已延后首次菜单加载退休、在 B 死亡时撤销绑定、待复活动作正常返回与健康 topology 后再稳定重绑；这些 C/S 会话事实不是 world identity 通知，也没有实现读取依据 3 的发放。
- CURRENT_SOURCE：PlayerScope 在完整 topology/lifecycle 刷新前读取当前 Player metrics；其后置条件只比较 BoundValid、generation 和 Player 指针。它不做完整 exact-pair、room/run、epoch 的后置复读。现有 player_fields_known 是这些 metrics 的状态标志，不能作为 world witness 或完整样本授权。
- DESIGN：读取依据 3 只能在有效 Authority 窗口内 PlayerUpdate 正常返回、目标原生 body 到达，随后完成当前完整 exact-pair/代次及生命周期复核后建立；任何 Restore、Replace、加载/转场、生命变化、绑定或 run 退休均要即时撤销，稳定复核后再发放。当前源码尚未实现这个 world bridge。

## J273 IDB 静态证据

证据目标为 isaac-ng-v1.9.7.12.exe.unpacked.exe.i64，IDA imagebase 0x400000。原始反编译和所选地址字节保存在 [st7-agent 原始材料](../../artifacts/repentogon/tx-04-08-static/st7-agent/README.md)；这里只记录可复核观察，不把伪代码当作运行时验证。

- STATIC_CONFIRMED：ExecuteStartGame VA 0x932A20 / RVA 0x532A20 的 Continue 分支在 VA 0x932E7E 调 Game Restore 0x6F43A0（RVA 0x2F43A0），紧接 VA 0x932E89 调 AutoReassignControllers 0x9964A0，之后还有房间更新和 PostGameStart。最终 Player/controller 应以后续稳定复读为准，不能以 Restore 前或 Restore 中的字段作 current 依据。
- STATIC_CONFIRMED：Game Restore 的 VA 0x6F4A58 调 PlayerManager Restore 0x993A90；0x6F4A60 调 Level Restore；0x6F4A6C 调 PlayerManager PostLevelInit Restore 0x993FA0。Game Restore 与 PlayerManager Restore 是不同 owner/事件。
- STATIC_CONFIRMED：PlayerManager Restore 的 VA 0x993AC9 调 helper 0x993780（RVA 0x593780），随后多处调用 Player Restore 0x7A5660（RVA 0x3A5660）。它还在 VA 0x993B8E/0x993B97 使用 operator new(0x2EA0) 和 Player 构造函数 0x74DC70。辅助函数按状态记录槽位及保存的 Player 标识匹配 manager 中已有 Player；未匹配对象交给另一 helper，缺失而需恢复的槽位通过创建 helper 产生 Player。PostLevelInit Restore 又调用同一 helper 的不创建分支，并对存在槽位应用 PlayerRestore。故 Restore 过程允许旧对象保留、对象退出以及新对象创建等不同结果。
- STATIC_CONFIRMED：Player Restore 0x7A5660 在 0x7A56BE 根据保存控制器值调用控制器设置函数；在 VA 0x7A5753 的特定恢复/ghost 条件下调用 RevivePlayerGhost 0x7CFC60。它有大量状态字段读写，不能当作单纯复活 helper。
- STATIC_CONFIRMED：RevivePlayerGhost 0x7CFC60 在 VA 0x7CFC88 清 Player +0x207D（ghost），在 0x7CFC8F 清 +0x3B0，并在 0x7CFC96 写 collision class +0x188=4。后续仍做图形/动画、伤害冷却、物品与生命字段处理；VA 0x7CFEF1 调 Spawn effect。playerType=35 分支会处理关联对象；VA 0x7CFF79 取 backup Player +0x1E50 并在它仍 ghost 时继续循环。普通角色的一对象观察不能外推到所有角色/回调。
- UNKNOWN：0x990250 创建和 0x990560 未匹配对象处理 helper 的完整语义、所有官方 Hook/Lua/mod 回调及其对字段的再写入没有在本 ST7 扩展；见原始材料保留的直接调用位置。结论仅限静态直接调用和反编译范围。

## tx-04-07 加载与 B 生命状态对照

- CURRENT_SOURCE：RestoreScope 在 Game Restore 入口分类首次菜单 Preserve。须 Host、start_game、外层 Start 已确认、当前干净（无 Join/K/绑定/retire/failure）、尚未观察过有效 run，并处于未建立 peer 或 Warmup；符合时清 start_qualified、调用 InvalidateBinding("restore_preserve") 并清待处理 run mutations，但不调用 RunTeardownBeginning。正常 Restore 返回后只延迟后续 loop 的稳定观察。否则把既有 run restore 按 run teardown 处理。见 [command.cpp](../../mod/src/ops/command.cpp#L369)。
- CURRENT_SOURCE：首次 Host 菜单下发现 !run 且 !run_observed 时，RefreshLifecycle 保留 warmup，会丢弃输入边沿并更新观察状态，不退休会话；已观察过 run 后 run 消失才 StopSession(RunTeardown)。这与 tx-04-07 历史静态报告第 13.3 节发现的“Restore 一律提前退休”冲突已由当前源码修复。
- CURRENT_SOURCE：B 死亡/ghost 被观察到时 ObserveRemoteLife 进入 Lost、记录 death event 并 InvalidateRemoteBinding，增加 participant generation、清输入队列并保留旧指针仅作中性归因。ghost=true 时还会按 Kill 调用返回情况关 BDeath 窗口。BRevive 期间在原生调用未正常返回前不因 ghost=false 变成 Revived；之后要求 RemoteHealthEligible，再在 stable Authority 重新 ReadTopology(1,K) 和绑定，不 Join、不新建 K。
- CURRENT_SOURCE：Persistence 的 ObserveEntityKill/ObserveReviveGhost 只对当前 B 打开窗口和记请求；Revive 入口观察在原生函数之前。窗口不是对象身份/出生/连续性账本，未通知 future world revoke/rebind。
- CURRENT_SOURCE：tx-04-07 E2E-09 实际选择 Continue 并回到 Active、players=2，恢复后的 B 创建/绑定编排完成；V13 的 invariants 与 save/load topology 字段为 true，但 coverage_complete=false，整项 NOT_OBSERVED。S21 的暂停菜单选择超时，未观察计划内 RunTeardown 终态，不能说 run teardown 已经 E2E 验证。见 [tx-04-07 结果](../design/tx-04-07/tx-04-07-results.md) 与 [e2e-09 报告](../../artifacts/repentogon/tx-04-07-e2e-09/report.md)。

## 当前 Player 读取依据与撤销通知

- DESIGN：规范第 11 节已接受 USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS（basis 3）：有效 Authority 模拟窗口、真实 PlayerUpdate 正常返回、目标原生 body 到达、generation 与已绑定 exact pair 仍一致后，当前专用 Player 字段才可读；绑定失效立即撤销，再次满足后才恢复。它不给出生或 Init 历史，Player-only 更新也不授予完整 world 样本资格。见 [现行规范 §11](../refactor/project-conventions.md#11-已采纳的-t0-06-当前-player-字段读取规则) 与 [tx-04-08 设计 §6.2](../design/tx-04-08/tx-04-08-implementation-plan.md#62-对象身份与出生)。
- CURRENT_SOURCE：hooks.json 把 Player Update 同地址 0x37BD40 注册为 player_update（priority -10000）和 player_native（priority +10000）。生成器进入前者建立 PlayerScope；后者在调用 super 前执行 PlayerNativeReached。若进入后者，它记录的是 Hook 到达该转接点；链下游/其他 Hook 全部执行的覆盖仍不能仅由此标志证明。
- CURRENT_SOURCE：PlayerScope 构造时按 ControlledOrdinal 归类并保存当时 participant generation。析构时仅要求本线程、无异常、同一 simulation tick、native_reached，再比较 BoundValid、generation 和当前 bound Player 指针后读取 MoveDirection/velocity/shoot fields。随后才调用 RefreshLifecycle(*r,false)。具体顺序见 [consumers.cpp](../../mod/src/game/consumers.cpp#L218)。
- CURRENT_SOURCE：这个后置比较没有重读 PlayerManager list/ordinal、controller key/K 所有权、完整 Topology::pair、run/room/epoch 或 Authority window 的结束状态。且 ControlledOrdinal 可使用缓存 controlled_player/旧 topology 关联给调用中的 Player 归 ordinal。因此该 metrics 采样不是 basis 3 的完整授权。即使后续输出 player_fields_known=true，仍不能视为 world 读取资格。
- CURRENT_SOURCE：当前 ReadTopology 能静态核对 Game/Manager、当前 Player vector、ordinal/PlayerIndex、Type=1、Variant=0、PlayerType、ControllerIndex、valid/exists/dead/removed/ghost、列表指针/index 唯一性及当前设备唯一所有权（B 按 K）；SameLife 只比较指针和生命字段。应复用“完整绑定核对”的约束，不能把 SameLife 或单独 pointer/gen 当 exact pair。
- CURRENT_SOURCE：B 的撤销覆盖已连同消费者复核：ObserveRemoteLife 在 B 暂不可读/不在 run（remote_life.cpp:199–202）及 Alive 状态下 B 非 unique（245–246）返回 Invalidate；RefreshLifecycle 的唯一调用点（lifecycle.cpp:314–320）只对 PeerLost 显式调用 InvalidateRemoteBinding，且 blocks 按 A topology 计算（297）。这几条路径没有共同的立即 B generation 失效动作。
- CURRENT_SOURCE：ConsumerScope 虽在消费者入口调用 RefreshLifecycle，但之后按 ControlledOrdinal 和缓存 participant binding 选择资格；例如 Shooting 的原调用条件只检查 blocks、BoundValid、绑定 Player 指针与 Context 状态（consumers.cpp:38–62）。OverrideQuery 再检查缓存 BoundValid、Player/device 指针及当前 device ID/controller 是否等于缓存 key（query.cpp:13–47），并不另读 B 的 PlayerManager topology；RuntimeHealthy 只查主线程、foreign/trace 故障与运行时 context/failure/finished（tick.cpp:180–189），也不核对 B topology。因此这些消费者复核不能证明上述 Invalidate 分支已撤销 B generation 或 world 依据，也不能仅凭源码注释“只保持中性”宣称这些分支已动态保持中性。
- UNKNOWN：在上述 B 暂不可读/非 unique 分支，缓存 binding 及 query 条件仍可能可用，但这里只能确认缺少 topology 复核和共同失效动作；没有通过实验判定实际 consumer/query 结果或动态输出，不能写成已证实输入泄漏。
- DESIGN：未来 world witness 应在本轮生命周期刷新后，重新读取并比较完整 current Binding（Game/PlayerManager/player/controller K）、participant generation、run/room/recording epoch 和 scope/tick；只有全部一致才授权本轮字段读取或在读后发布 witness。撤销要由所有当前态失效入口触发，不能依赖晚到的 Manager retirement 或只监听 Player ghost/dead。
- DESIGN：至少分开观察 Game Restore、PlayerManager Restore、Player Restore/Replace；Restore entry 先撤销连续性与当前读取依据，正常返回只记录 owner 完成，不能直接确认出生/保留/同一身份。首次菜单 Continue 仍是 Preserve：记录发生过的 Restore，但不要误发上一个 run retired；继续等待当前 run 与 exact pair 稳定建立。run teardown 入口应即时退休该 run 的 world 关联且可去重：可能来自 Restore、Game::Exit 原生入口、run-disappeared 生命周期路径；录制 context 可继续采后续场景，不能复活旧 run 引用。
- DESIGN：B death/revive 应沿用 K、ordinal 与已有 life/generation 事实；死亡/生命/identity 变化立即撤销当前 witness，RevivePlayerGhost 正常返回并稳定健康 exact pair 后才可恢复依据。world 观察不得另 Join 或创建 K。加载 begin/end 污染当前 sample token，稳定后重新见证。

## UNKNOWN 与主审关注

1. UNKNOWN：Player Update hook chain 内第三方/其他 Hook 的短路和具体叠序，故 PlayerNativeReached 是否覆盖目标原生 body 实际执行。
2. UNKNOWN：PlayerManager Restore 两个内部 helper 对所有旧对象、旁路列表、特殊保存状态的精确移除/复用副作用；这里只据可见比较、调用和分配判断“可保留/可删除/可新建”，没有推断完整生命周期。
3. UNKNOWN：B unreadable/non-unique 的 Invalidate 返回值在调用点被忽略；ConsumerScope、OverrideQuery、RuntimeHealthy 使用的缓存绑定/上下文条件没有独立 B topology 复核。实际调用结果和设计意图未动态验证，所以既不能保证已中性，也不能断言已发生输入泄漏。
4. UNKNOWN：tx-04-07 的 E2E-09 没有完整覆盖 V13，S21 未跑通；当前没有 world recorder 实际 Revive/Continue/run-teardown 通知或 basis 撤销/恢复的运行时证据。

主审复读结论：consumers.cpp:263–281 的 Player metrics 读取在 RefreshLifecycle 前，后置比较不是完整 exact pair/epoch；未来 world witness 不可复用 player_fields_known。另，RefreshLifecycle 对 B 的 Invalidate action 目前没有调用点失效；ConsumerScope 即使刷新生命周期，query 仍按旧 BoundValid/Player/device/key 和 tick/context 条件工作，RuntimeHealthy 不额外查 B topology。静态资料不足以保证这些路径已撤销 B generation/world 依据，注释中的“中性”不是动态验证。IDB 恢复序列与 RevivePlayerGhost 的 ghost=0 早于后续完整动作，仍支持分离 Restore 事件并即时撤销读依据。

