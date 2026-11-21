# Isaac C/S T0-04 逆向交接

> 2026-10-12 执行约定更新：先读[现行项目规范](../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

更新时间：2026-08-27  
适用目标：Win32/x86 `isaac-ng.exe`  
交接状态：T0-01、T0-02、T0-03 scoped gate、T0-03R V5 loop gate 与 T0-04 active gate 均已完成；T0-04 正式 `PASS`

本文是截至 T0-04 的逆向信息单一入口，供 T0-05“本地多人创建和虚拟输入设备”继续使用。它整合目标构建、模式隔离、loop/Authority、输入采样、Player ownership、自然 consumer、canonical queue/apply、生命周期和安全恢复结论，并明确列出已推翻假设与后续未知项。

本文不覆盖历史 artifact、原始 analysis 或机器可读 catalog。发生冲突时，证据优先级固定为：正式 gate decision 与其 hash-bound source → 当前版本机器可读 catalog/plan → 本文 → 历史过程文档。历史文档中的旧停点只解释当时状态，不能把已经关闭的缺口重新变成当前阻塞，也不能把后续结论追写回旧 artifact。

## 1. 交接结论

- T0-04 final decision 为 schema 4 `PASS`，`t004Unlocked=true`、`activeInputAllowed=true`、`semanticContractComplete=true`、`runtimeContractComplete=true`，missing case 为 0。
- 正式 active matrix 为 primary `18/18`、extra repeatability `2/2`；20/20 根 analysis 均 `PASS` 且 `eligibleForActiveGate=true`。loopback comparison 为 4/4 `PASS`。
- T0-05 可以开始，不需要补做 T0-04 实机矩阵，也不能复用任何 T0-04 arm token、claim、mapping、session、PID/creation 或 `Player*`。
- T0-05 必须复用 T0-04 的 canonical、queue、Authority tick、owner generation、original-once、fail-closed 和恢复契约；它的新逆向范围是 local co-op 加入/退出、第二玩家创建、HUD/index、controller 分配和 virtual device 生命周期。
- 原版 OriginalOnline 只可作为隔离对照。`NetInputDevice`、原版 ready gate、Online Manager、channel `0` 和 Isaac Sync channel `0xFD` 都不能代替 local co-op 结论。

## 2. 证据标签和固定目标

本文使用以下标签：

| 标签 | 含义 |
| --- | --- |
| `RUNTIME_CONFIRMED` | 固定构建上已有可重放实机证据 |
| `STATIC_CONFIRMED` | 已核对控制流、机器码、目标和 ABI，但不得外推为未运行场景 |
| `EXPERIMENT_CONTRACT` | T0 为验证而冻结的 canonical/queue/runtime 规则，不是产品 ABI |
| `REJECTED_HYPOTHESIS` | 已被静态或实机证据推翻，禁止复用 |
| `UNKNOWN` | 尚无足够证据，后续必须建立新 ledger/source |

当前正式目标链：

| 项 | 值 |
| --- | --- |
| canonical target SHA-256 | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| observed approved-unpacked target SHA-256 | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| target equivalence document SHA-256 | `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138` |
| T0-02 mode gate SHA-256 | `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185` |
| T0-03 scoped gate SHA-256 | `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB` |
| T0-03R V5 loop gate SHA-256 | `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA` |
| T0-04 raw prerequisite SHA-256 | `99BA518991FEA36FE78D91AEA6D91D80634D4FD3B314399C4C4203C1BA27D5AC` |
| T0-04 active catalog v2 SHA-256 | `A89A74A251EEAA2510FC0C681247ABD6B0A31A7CC3ED60833C78EF36C9B0DC77` |
| T0-04 active plan v2 SHA-256 | `FE4FC7ABDE09CBB280DA342BD384AB50107CA948A052AC02BA6ADE37BB1DF3BA` |
| T0-04 formal matrix SHA-256 | `54BDD4F7E0070D14DA913C6993C97272D97A91B193BDA5C04E2FCAE3E9C6889D` |
| T0-04 repeatability SHA-256 | `AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC` |
| T0-04 verification v2 SHA-256 | `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317` |
| T0-04 final decision SHA-256 | `927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C` |

所有 RVA 都相对 canonical image；加载地址必须使用 `moduleBase + RVA`。T0-05 若目标、expected bytes、call target、vtable、COL、ABI 或 equivalence 任一变化，必须建立新 catalog，不得沿用本表。

`input-active-catalog-v2.json` / plan v2 中的 `productionActiveInputAllowed=false`，以及 verification v2 中的 `activeInputAllowed=false`，都是“单个 prerequisite/计划/自动门不能自行授权”的职责约束，不是最终项目状态。当前授权状态只读取 hash-bound schema-4 final decision，其值为 `activeInputAllowed=true`。

## 3. 模式隔离与 OriginalOnline 边界

### 3.1 ModeLatch

`RUNTIME_CONFIRMED`：

- `OriginalOnline` 只在三类信号至少两类连续成立时冻结：frame wrapper 的 `self` 与 manager/sync-object 身份匹配、2–4 人匿名 roster 稳定、SteamNetworking006 channel `0` 调用。
- `Offline` 只在精确 simulation `lua_pcallk` callsite、主线程连续 60 callback，且 manager/roster/channel `0` 在线信号全零时冻结。
- scenario 名称只是预期，不是证据。期望 Offline 却出现在线信号，或期望 OriginalOnline 却出现完整 offline window，都必须退休而不是自动换模式。
- manager 变化、online frame 回退、非零原版 channel、simulation thread 变化、绑定 `lua_close`、run teardown 或菜单退役都会结束当前 generation；旧 token 只能保留 tombstone。
- OriginalOnline C2 证明原版 Send/Available/Read 保持 channel `0`、payload 不改、原函数恰好一次、ready stall 不推进 frame、成功帧恰好一次 dispatch。C/S 事件始终为零。

### 3.2 T0-05 不得复用的在线对象

- `original-online-disarmed live-46` 只读确认了 active Run、`OnlineManager + 0x4B3D8`、Steam backend vtable `moduleBase + 0x7A2440`、2 人 distinct roster 和不回退 online frame；它没有创建 arm/token、IPC、Native、Helper、suppression 或 synthetic input。
- peer identity 只以 nonce 盐化摘要进入 artifact；原始地址和 peer identity 不持久化。
- 以上证据只证明模式检测与零干扰，不证明 local co-op 第二玩家的创建、controller 分配或 `Isaac.GetPlayer(index)` 顺序。
- T0-05 必须从 Offline/local co-op 建立对象图。若第二玩家只能通过 OriginalOnline session 或未理解的 `NetInputDevice` 创建，应立即停止。

## 4. Loop、更新与 Authority 模型

### 4.1 顶层 loop 不是 Authority tick

`RUNTIME_CONFIRMED`：

| 角色 | callsite → target | 结论 |
| --- | --- | --- |
| loop clock/header | `0x531231 → 0x6266A0` | top-level iteration 锚 |
| top-level update | `0x5312EC → 0x5AB6D0` | 每轮 update owner |
| render owner | `0x5312F1 → 0x5555C0` | 每轮 render owner |
| conditional gameplay attempt | `0x555231 → 0x2FADC0` | 约隔一个 top loop 执行一次，不等于 loop clock |

实机样本中 top-level update/render 约 60 Hz，而 broad gameplay/Authority attempt 约隔轮一次。`loopIteration`、render frame、`onlineFrame` 和 `authorityTick` 是不同身份，禁止互换。

### 4.2 Steady-standard Authority transaction

T0-04 active apply 只支持以下 `STEADY_STANDARD_ONLY` 路径：

```text
0x555231 conditional attempt
  -> 0x2FADFA predicate=false
  -> 0x2FAE09 body anchor
  -> 0x2FBA4D Room::Update (target 0x402980)
  -> 0x403E70 EntityList
  -> 0x2FBBA5 MC_POST_UPDATE tail
```

- `authorityTick` 的 pre candidate 为 `lastCompleted + 1`，只有 `Room::Update` 正常返回后的 authority post 才提交；异常、overflow 或 lifecycle 变化都不提交。
- synthetic snapshot 只在 authority pre 发布，在 post 清除；每个 target tick 最多一次 adapter apply。
- alternate、loading、room lifecycle、run teardown、menu 和无法唯一绑定 owner/device generation 的路径在 T0-04 active adapter 中全部 fail closed。
- T0-05 可以复用 steady-standard tick/apply，不得把 local co-op 创建路径直接命名为 Authority tick。新玩家加入/退出若发生在其它 owner 下，必须单独归属。

### 4.3 已知但未纳入 active apply 的事务

| 路径 | 已知锚 | 当前边界 |
| --- | --- | --- |
| alternate/run | owner `0x554D5E`、state apply `0x559B21`、room `0x559B4D`、completion `0x559B82` | 控制流存在；完整 active transaction 仍非 T0-04 apply 路径 |
| traditional room lifecycle | owner `0x2FB4F0`、nested simulation `0x52FC77` | 静态/部分 runtime 归属，不得泛化为普通门 |
| ordinary-door room change | owner `0x2FB51C`、nested simulation `0x2FE37C` | V5 实机 transaction 已确认 |

普通门的固定外层链为：

```text
0x2FB51C -> 0x4318A0 room-change owner
  -> 0x430950 -> 0x430BBF -> 0x2FE2F0
  -> 0x2FE371 -> 0x33FC80 -> 0x33E0A0
  -> 0x33F5B9 NewRoom
  -> 0x2FE37C -> 0x402980 Room::Update
  -> 0x403E70 -> 0x4186C0 EntityList
```

`0x2FB51C` 可多次出现 owner-only attempt；只有包含唯一 nested NewRoom/simulation 的事务构成 room-change proof。first-visit、revisit、clear、door type 的字段语义仍为 `UNKNOWN`。

## 5. Message pump 与设备刷新

`RUNTIME_CONFIRMED`：Win32 message pump 属于 top-level update，但 message 数不能替代 Authority tick。

| 事件 | return RVA |
| --- | ---: |
| `PeekMessageW` | `0x65E68C` |
| `TranslateMessage` | `0x65E6D6` |
| `DispatchMessage` | `0x65E6E1` |
| follow-up `PeekMessageW` | `0x65E6F0` |

设置内切换 controller 时存在两种穷举的合法 loop-local 形状：

1. 四个 message 事件后直接执行 Keyboard refresh → Gamepad refresh，共连续 12 个事件。
2. 四个 message 事件后执行完整 InputManager/Keyboard/Gamepad sample，manager exit 后再次 Peek，再执行 Keyboard/Gamepad direct refresh，共连续 23 个事件。

两种形状都要求同一 main thread、update、loop 和稳定 device identity。仅在同一 trace 看见 Keyboard/Gamepad poll，不足以证明 ownership switch。

## 6. InputDevice ABI、采样与 Player ownership

### 6.1 对象关系

当前已确认的只读对象链：

```text
module+0x857B18 input manager
  -> active shared_ptr vector
  -> InputDeviceBase+0x0C deviceKey

module+0x871678 game root
  -> game root +0x1BAA8/+0x1BAAC/+0x1BAB0 Player* vector
  -> Player+0x1618 assignmentKey

deviceKey == assignmentKey
  -> current-run unique owner join
```

- Keyboard 已实机稳定观察到 key `0`；Gamepad 已稳定观察到 key `1`。这些是当前 run 的 assignment identity，不是跨 run、跨 generation 或网络身份。
- RPM reader 采用 vector header、active span、device、binding、game root、Player vector 和 assignment key 双采；不稳定快照显式 drop。
- `Player*`、device pointer、platform handle 和 primitive code 都不进入 artifact 或 canonical bytes。
- T0-05 必须重新确认 local co-op 双 Player vector、两份 assignment 和 index 顺序，不能假设单人链可自然扩展。

### 6.2 Physical poll envelope

```text
0x554F7D -> 0x61FC00 InputManager update
0x61FC97 -> 0x61F280 per-device common update
0x61F307 -> vtable +0x78 concrete physical poll
return 0x61F30C
```

| 设备 | vtable / slot | concrete poll target | 当前 raw projection |
| --- | --- | --- | --- |
| Keyboard | vtable `0x7A1678`；slot RVA `0x7A16F0` | `0x66C650` | 对稳定 action `0..11` binding，从 current `0x878C10` / previous `0x878950` concrete state 投影 digital value |
| Gamepad | vtable `0x782C80`；slot RVA `0x782CF8` | `0x622940` | 从 button offsets `+32/+36`、directional axes `+64`、deadzone `+208`、mapping `+904` 投影 action `0..11` scalar |

采样只在 main-thread `InputQuery` phase、physical-poll original return `0x61F30C` 后成立。snapshot 不稳定、非法 deadzone、非有限 axis、structured special `0xCDCC/0xCDCD`、跨 gameplay action alias 或 valid mask 不完整都 fail closed。

### 6.3 Logical query ABI

Keyboard 与 Gamepad concrete vtable 均使用：

| 槽 | ABI | 返回 |
| --- | --- | --- |
| `+0x30` | `bool __thiscall(InputDeviceBase*, uint32 action)` | `AL`，`ret 4` |
| `+0x34` | triggered query，同上参数 | `AL`，`ret 4` |
| `+0x38` | `float __thiscall(InputDeviceBase*, uint32 action)` | x87 `ST0`，`ret 4` |

这些是 demand-driven logical consumer，不是 physical sample。Active wrapper 必须 original exactly-once；只有完整 main-thread/steady/owner/device/family/generation eligibility 才允许返回 synthetic snapshot，其余全部 passthrough。

## 7. Player owner 与自然 consumer

Player update owner：image pointer slot `0x76BDDC → 0x382AF0`，ABI 为 `void __thiscall(Entity_Player*)`。

| consumer | callsite → target | ABI | actions |
| --- | --- | --- | --- |
| movement | `0x385BC2 → 0x379B40` | `void __thiscall(Entity_Player*)` | `0..3` |
| bomb | `0x385BC9 → 0x380F40` | `void __thiscall(Entity_Player*)` | `8` |
| item slot | `0x385BD2 → 0x391A40` | `void __thiscall(Entity_Player*, int slot)` | `9` |
| pill/card | `0x385BD9 → 0x3A3D30` | `void __thiscall(Entity_Player*)` | `10` |
| shooting intent | `0x385BE0 → 0x37B020` | `void __thiscall(Entity_Player*)` | `4..7` |
| drop | `0x385BE7 → 0x3A42B0` | `void __thiscall(Entity_Player*)` | `11` |

结论边界：

- owner wrapper 必须先绑定唯一 Player 与 selected device key，之后 physical projection 才可发布。未绑定首帧只能跳过，不能读取 device、更新 edge history 或创建 command。
- 每个 proof-bearing action 要求同 Player/device/sample generation 的 finite nonzero raw、nested concrete logical true 与 exactly-one-candidate natural consumer transaction。
- 同 family 多候选 transaction 整笔只作 ambiguity，不能把 witness 分配给任一 action；cross-family 或错误 consumer site 使整轮失败。
- movement witness 为 Player `+0x1624/+0x0360/+0x0364`；shooting intent 为 `+0x162C/+0x1630/+0x1634/+0x1638`；bomb intent 为 `+0x1378 bit0`；drop counter 为 `+0x17C0`。这些是批准 witness，不等于通用 world apply。
- action 9/10 只有 consumer proof，没有冻结的通用 world field。shooting intent 也不等于 projectile spawn proof。

## 8. Canonical input 与 scheduler 实验契约

这一节是 `EXPERIMENT_CONTRACT`，可由 T0-05 复用，但不是正式产品 wire ABI。

### 8.1 Canonical bytes

- `InputStateDraftV1`：24B little-endian。
  - `u16 version=1`
  - `u16 length=24`
  - `u32 heldFlags`：当前仅 bit 0 `shootHeld`
  - `u32 edgeFlags`：bit 0 bomb、bit 1 item、bit 2 card/pill、bit 3 drop
  - 四个 `i16` Q15 轴：movement 与 aim
  - `u32 reserved=0`
- `InputCmdDraftV1`：64B，为 16B session、`u64 clientId`、`u64 sequence`、`u64 targetTick`、24B state。
- raw axis clamp 到 `[-1,1]` 后向零截断为 Q15；整数 radial dead-zone 为 `4096`，死区外线性满幅重标，再执行单位圆压缩。
- 非有限值、unknown bits、`-32768`、reserved 非零和 shoot/aim 冲突拒绝。设备类型、device key、Player pointer 和进程地址不进入 canonical bytes。

### 8.2 Queue、resolve 与 apply

| 常量 | T0-04 值 |
| --- | ---: |
| lead | `2` ticks |
| future window | `64` ticks |
| transport/input queue capacity | `256` |
| drain budget | `64/tick` |
| dedupe history | `4096` |

- 同 identity 同内容幂等；同 sequence 不同内容为 identity conflict，并安全退休。
- queue-full、expired、too-far、stale、identity-conflict 是五个公开 reject；其它协议错误为 terminal Failure/ClientRetired。
- 同 target 的 continuous state 取最高 sequence；不同 edge 合并；相同 edge collision 拒绝。
- resolve 只在目标 Authority tick；每个 command 最多 apply 一次；reject 必须 world unchanged。
- Local Client loopback 与未来 Endpoint B 必须走相同 canonical/sequence/target/dedupe/resolve/apply 路径，不能为房主保留即时旁路。

## 9. 生命周期和场景结论

| 场景 | 已确认行为 | T0-05 约束 |
| --- | --- | --- |
| pause | top loop/update/render/physical poll 持续；steady-standard transaction 形成前后夹闭缺口 | 不把 poll 停止当 pause；暂停期间不得继续 Authority apply |
| loading | 主菜单 Continue 可见 NewRoom/alternate 边界；active steady adapter fail closed | 第二玩家创建若落在 loading/alternate，必须建立新 owner 证据 |
| room | ordinary-door transaction 为 owner → NewRoom → nested Room::Update → EntityList；owner-only attempt 合法 | room change 清 owner generation；重新绑定 Player/device，不假设 pointer 稳定 |
| menu/run teardown | `RUN_TEARDOWN` 后客户端退休，旧 token 不复活 | 清空 `ClientId -> LocalPlayerOrdinal -> Player*` 当前 generation 映射 |
| device switch | assignment 必须由前后唯一 join 与 loop-local 12/23-event refresh 共同证明 | 双玩家不得以“两个 poll 都存在”冒充两份 ownership |
| original online disarmed | 两次只读在线观察、零 hook/IPC/input apply，45 槽不变 | C/S local co-op 测试不得激活 Online Manager 或 ready gate |

`ownerGeneration` 在 Player/device/lifecycle epoch 变化时递增，并同步清 transport、queue、planner 与 snapshot。room owner/lifecycle generic clear 可以推进 generation；只有真实 NewRoom 发布场景级 room 标签。

## 10. Hook 与恢复安全边界

T0-04 组合事务共 45 槽：V5 32 槽 + Active 13 槽。

| 类别 | 数量 | 内容 |
| --- | ---: | --- |
| V5 REL32 | 27 | loop/update/Authority/lifecycle/room/message owners |
| V5 image pointers | 2 | Keyboard/Gamepad physical poll |
| V5 USER32 IAT | 3 | Peek/Translate/Dispatch |
| Active image pointers | 7 | Keyboard/Gamepad 三个 logical query + Player owner |
| Active REL32 | 6 | 六个 Player natural consumer |

完整 target/bytes/ABI 以 `cs-t0/input-active-catalog-v2.json` 为准。安装和停止契约：

1. 安装前核对 target profile、process creation、全部 hash-bound prerequisite/catalog/plan 和 45 槽 expected bytes/targets。
2. suspended-thread transaction 按 V5 REL32 → Active REL32 → V5 pointer → Active pointer 安装；中途失败逆序 rollback。
3. 正常停止：关闭 publication/override → drain 已准入 embedded callback → retire/neutralize → 恢复 Active 13 → 二次 drain/target verify → 恢复 V5 32 → drain → 45 槽逐字节相等 → ProducerClosed/Stopped → unload。
4. 任何 restore、drain、thread、owner、ring、companion health 或 target 复核不确定都进入 Unsafe；排空超时不得卸载或自动重试。

T0-05 如果新增 local co-op/virtual-device hook，必须加入新的单一事务和新 slot ledger；不得在 45 槽之外另起第二个不协调的 Hook owner。

## 11. 已推翻或退休的假设

以下结论不得进入 T0-05 设计：

| 假设 | 状态 | 正确边界 |
| --- | --- | --- |
| 宽泛 OriginalOnline input window 是通用 gameplay tick | `REJECTED_HYPOTHESIS` | online dispatch 只作模式/零干扰证据 |
| top-level loop iteration 就是 Authority tick | `REJECTED_HYPOTHESIS` | steady-standard Room::Update 正常完成才提交 authorityTick |
| `0x554F84 → 0x55B310` 是 physical sample | `REJECTED_HYPOTHESIS` | 它降级为 controller-assignment candidate |
| logical vtable `+0x30/+0x34/+0x38` 是每帧 physical sample | `REJECTED_HYPOTHESIS` | 它们是 demand-driven pressed/triggered/scalar query |
| device `+0xC0/+0xC4` 是 raw physical source | `REJECTED_HYPOTHESIS` | physical poll return 后从 concrete Keyboard/Gamepad state 投影 |
| `0x5551D8` 是 input apply | `UNKNOWN / forbidden claim` | 只确认条件字段复制 |
| `0x2FB226/0x34584D` 是普通门 owner/simulation | `REJECTED_HYPOTHESIS` | 普通门使用 `0x2FB51C/0x2FE37C`；旧槽只在特殊条件路径出现 |
| `0x345842/0x345856` 是怪物/clear/visited 分流 | `REJECTED_HYPOTHESIS` | 两个条件字段语义仍 `UNKNOWN` |
| pause 会停止 physical poll | `REJECTED_HYPOTHESIS` | poll 持续，缺失的是 steady-standard transaction |
| 同一 artifact 看见 Keyboard 与 Gamepad poll 即证明 device switch | `REJECTED_HYPOTHESIS` | 需要前后唯一 assignment join 和精确 direct-refresh 事务 |
| same-family diagonal 可给多个 action 共用 witness | `REJECTED_HYPOTHESIS` | 整笔 ambiguity-only，零计分 |
| Player pointer/device key 可作为网络身份 | `REJECTED_HYPOTHESIS` | 只作当前 generation 关联；网络身份使用 ClientId/ordinal |
| 每次 Offline 新局都会重建 manager/Lua root | `REJECTED_HYPOTHESIS` | menu-return 可复用 root；以显式 lifecycle/generation 退休 |
| 每个成功不可靠 Send 都必须被对端 Read | `REJECTED_HYPOTHESIS` | mode 0/1 未送达只作不可靠丢包诊断 |

## 12. T0-04 后仍开放的逆向

这些开放项不反转 T0-04 `PASS`。它们按后续任务归属，不再写成“T0-04 尚未完成”。

### 12.1 T0-05 必须新建证据

1. local co-op 玩家加入/退出入口、controller assignment owner 和调用线程。
2. 第二个 `Entity_Player` 的创建、Player vector 插入、HUD 注册、player index 更新和 Lua `Isaac.GetPlayer(index)` 对应关系。
3. Offline 单人、local co-op 双人、OriginalOnline 双人的 manager/Player/InputDevice 对象差异；OriginalOnline 只能作反例对照。
4. `ClientId -> LocalPlayerOrdinal -> Player*` 的冻结、重绑定和退休规则；`Player*` 只在当前 run/room/owner generation 内使用。
5. 两名玩家各自的 assignment key、selected device、consumer owner 和 input isolation。
6. 游戏已有 local co-op join 路径是否足够；只有其不足且 ABI/owner/lifecycle 全部理解后，才研究 virtual controller adapter。
7. 玩家死亡/复活、换房、返回菜单时 Player pointer、ordinal、HUD 和 controller ownership 是否变化。

### 12.2 主要归属 T0-06 或更晚

- `0x33E0A0` 五个 direct caller与 `0x402980` 十个 direct caller的完整 ownership 分类。
- first-visit/revisit/clear/door-type 字段真实语义。
- 完整 active alternate transaction 与 traditional lifecycle FSM。
- 全部 spawn/remove/entity mutation 路径与稳定无指针实体身份。
- 其它 message-pump candidate 的分类。
- 客户端抑制 gameplay mutation 后是否能继续 render/network/UI。
- framestate/checksum apply、完整 RNG 对象集合、state baseline/delta 与 Replica World。

## 13. T0-05 启动规则

建议按以下顺序开始：

1. 从当前 target/equivalence 和 T0-04 final decision 建立新的 T0-05 ledger；引用旧 hash，不修改旧文件。
2. 先做离线静态分析和只读对象图：local co-op join/exit、Player vector、assignment、HUD、index、Lua order。
3. 建立单人 C0、local co-op 双人 C0 和 OriginalOnline 双人只读对照；OriginalOnline 路径保持 disarmed。
4. 只在用户新的独立授权下运行 local co-op passive observer；首次 active 前必须再次完成 target/owner/lifecycle/restore 自动门。
5. 先调用游戏已有 local co-op 加入路径创建第二玩家，再判断是否需要 virtual adapter。
6. Endpoint A/B 都使用 T0-04 canonical scheduler；Endpoint B 只绑定第二 `LocalPlayerOrdinal`，不得直接写 Player/device/cache/world。
7. 覆盖分别移动、同时移动、同时射击、edge collision、死亡/复活、房间变化和菜单退出；至少 1,000 authority tick 无串线、重复、旧 generation 或 pointer 失效。

立即停止条件：只能通过 OriginalOnline 创建第二玩家；需要伪造未理解的 owner/vtable/controller lifecycle；出现 save/achievement/run 不可逆污染；Player/HUD/index/assignment 无法唯一绑定；或新增 hook 无法纳入单一事务并完整恢复。

## 14. 文档与机器证据入口

| 入口 | 用途 | 当前地位 |
| --- | --- | --- |
| `out/cs-t0/formal-active-gate-20260827-r1/active-gate-decision.json` | T0-04 最终门禁 | 正式决策 |
| `out/cs-t0/formal-active-gate-20260827-r1/PROVENANCE.md` | 20 份 replay 与 detail-only migration | 正式审计说明 |
| `cs-t0/input-active-catalog-v2.json` | 45 槽 target/bytes/ABI | 当前机器契约 |
| `cs-t0/input-active-plan-v2.json` | 场景、Authority path、fail-closed scope | 当前机器计划 |
| `cs-t0/raw-apply-catalog-v4.json` | raw projection、Player owner、consumer ABI | 当前 passive 机器契约 |
| `cs-t0/archive/t0-04-history/loop-envelope-research.md` | T0-03R 逐轮逆向过程和历史反证 | 历史过程证据；以本文结论边界解释 |
| `cs-t0/archive/t0-04-history/input-passive-validation.md` | 早期 witness/RPM 证据 | 历史 passive 证据 |
| `cs-t0/schema4-native-runtime.md` | Schema-4 模块和恢复不变量 | 当前实现说明 |
| `cs-t0/archive/README.md` | 已归档文档与旧机器契约清单 | 历史证据索引 |
| `docs/archive/refactor/cs-t0/isaac-cs-t0-03r-04-status.md` | 时间线、artifact 与失败停点 | 历史/状态索引 |
| `docs/archive/refactor/cs-t0/t0-04/isaac-cs-t0-task-plan.md` | T0-05 高层目标、产物和验收 | T0 全局任务规范 |
| `docs/archive/refactor/cs-t0/t0-05/isaac-cs-t0-05-research-plan.md` | T0-05 阶段任务、证据门和退出条件 | T0-05 当前执行计划 |

本文只收敛已经取得的逆向结论。机器 target 或新实机证据变化时，应先创建新 ledger/catalog/artifact，再更新本文；禁止先改叙述后补证据。
