# tx-04-07 ST3：普通换层路径静态确认
> **主审批注（2026-10-25；以下更正优先于正文中相应的原始表述，原文保留）：**
> 1. `cs-t0/native/src/owned_virtual_device.cpp:65` 是旧 T0 原型，不代表当前 REPENTOGON mod。当前实现是 `mod/src/game/device_k.cpp:103–105`：`Poll` 返回条件为 `p` 非空且 `state == Registered`，不是无条件 `true`。正文旧 `SOURCE` 段仅作为历史依据保留，不应用于描述当前实现。
> 2. 普通 A/B 的 Player 保留推断只适用于排除 `TriggerRoomExit` parent/null-effect 移除、Genesis reset 等特殊分支后的普通条件；它不能概括整个已读函数体“没有 Remove/重置”。这些特殊分支继续列为例外/UNKNOWN。
> 3. `sub_9950B0(566, ...)` 中的 `566` 仅确认是传给尚未识别业务语义 helper 的参数；不得称为已确认的事件 ID、设备通知或 callback。


本笔记只覆盖夹具 `Game():StartStageTransition(false, 0, nil)` 在官方 v1.9.7.12 的静态路径。目标二进制由当前 IDA 会话打开：`isaac-ng-v1.9.7.12.exe.unpacked.exe`，输入路径为 `C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe`；imagebase=`0x400000`，以下 RVA 均为 `VA - 0x400000`。未校验哈希。所有 IDA 证据均经只读 API 导出；未启动游戏、构建、测试或调试，也未改 IDB。

可复核材料：完整函数/指令/字节/xref/伪代码/焦点窗与元数据见 [st3-static-export.json](st3-static-export.json)；重做该只读导出的脚本见 [export_st3_readonly.py](export_st3_readonly.py)。函数名对 IDA 未命名函数使用候选标签；`PostNewRoom` / `PostNewLevel` 的标签依据 helper 内写入的 callback ID（19/18）及调用上下文，导出中仍保留其实际 IDA 名称和原始指令。

## STATIC_CONFIRMED

### Lua fixture 的 ABI 有效

官方 wrapper `REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaGame.cpp:104–115` 对参数 2 执行 `luaL_checkboolean`、参数 3 执行 `luaL_checkinteger`；参数 4 先置 `nullptr`，仅当 `lua_type(L, 4) == LUA_TUSERDATA` 时才解析 Player userdata，然后把三个值交给原生方法。注册表在同文件第 424 行。因而当 Game userdata 有效时，`false`、整数 `0`、`nil` 正好符合这个 wrapper：nil 留下空 Player 指针，不会被当作 Player userdata 解析。原生 ABI 声明为 `__thiscall void Game::StartStageTransition(bool, int, Entity_Player*)`（`libzhl/functions/Game.zhl:68–69`）；官方 API 文档也把 Player 标为可选并说明缺省为 `GetPlayer(0)`（`docs/docs/Game.md:34–37`）。

`Game::StartStageTransition` VA `0x6F9B30` / RVA `0x2F9B30` 在 `0x6F9B41` 比较 transition 参数与 6、`0x6F9B44` 对大于 6 的值才跳错误路径，因此 0 在接受范围内。`player == nullptr` 时在 VA `0x6F9D3D` 调 `Game::GetPlayer(0)`，并在 `0x6F9D4D` 保存到 `Game+0x1BB70`；`GetPlayer(0)` 对非空 Player vector 返回 ordinal 0。空 vector 行为不是本夹具所需的正常前置，保留 UNKNOWN。

### 转场状态与起止边界

在普通 `sameStage=false` 路径，`StartStageTransition` 设置 FSM 的 state 为 1、counter 为 0、sameStage 为 false、transition override 为 0、默认 duration 为 50：

| VA / RVA | 机器指令 | 含义 |
| --- | --- | --- |
| `0x6F9D62` / `0x2F9D62` | `C7 83 78 BA 01 00 01 00 00 00` — `mov dword ptr [ebx+1BA78h],1` | 打开 FSM，state=1 |
| `0x6F9D6C` / `0x2F9D6C` | `C7 83 7C BA 01 00 00 00 00 00` — `mov dword ptr [ebx+1BA7Ch],0` | counter=0 |
| `0x6F9D76` / `0x2F9D76` | `88 83 84 BA 01 00` — `mov [ebx+1BA84h],al` | sameStage=false |
| `0x6F9D7C` / `0x2F9D7C` | `89 BB 88 BA 01 00` — `mov [ebx+1BA88h],edi` | transition override=0 |
| `0x6F9D8F` / `0x2F9D8F` | `C7 83 80 BA 01 00 32 00 00 00` — `mov dword ptr [ebx+1BA80h],32h` | 默认 duration=50 |

该 routine 在 VA `0x6F9E8B` / RVA `0x2F9E8B` 有条件把 duration 改为 8：原生条件要求当前 stage 在 1..6、Game 标志 `+0x25AC4` 所在位 `0x10000` 为 1，并经过 stage 4/5 调整后不等于 1；另有 stage=5 的专门分支。故不能把 50 或 8 概括成所有运行状态下固定的转场帧数。FSM 还依赖状态、计数、动画/玩家字段等分支，本文只确认存储值与已读分支，不将 counter 等同于墙钟或固定帧时长。

常规 FSM 在 VA `0x909830` / RVA `0x509830` 由 state 1 写 state 2；其他特定分支可提前写 state 4。state 2 达到其分支阈值时，在 `0x9098CE` 写 state 3。该分支之后先遍历当前 Player vector，通过 vtable `+0xC` 更新 Player，并写 Player `+0x171=1`；接着按下列顺序调用：

1. `Level::Init`：VA `0x909B15` / RVA `0x509B15`。
2. FSM 明确调用 `Room::Update`：VA `0x909B26` / RVA `0x509B26`。
3. `Manager::SaveGameState`：VA `0x909B2B` / RVA `0x509B2B`。
4. `NewRoomTempEffects`：VA `0x909B3C` / RVA `0x509B3C`。

state 3 的一个正常完成分支在 `0x909D68` 写 state 4。state 4 会先对现有 Player vector 执行 vtable `+0xC` 更新（`0x909DC0`），再检查 Player `+0x1398/+0x139A` 条件；满足完成条件时，VA `0x909DF8` / RVA `0x509DF8` 写 state 0，随后仍调用 `ItemOverlay::Update`（`0x909E06`），在 `0x909E0B` 继续 counter 收尾并从 FSM 返回。因而“state=0”只是 active-state 字段清零点，不等于此调用栈及其后续 work 已结束。

owner 顺序同样重要：`Game::Update` VA `0x6F6D30` / RVA `0x2F6D30` 在 `0x6F73E5` 检查 `Game+0x1BA78` 非零后，于 `0x6F73F4` 调 `StageTransition::Update`，随后在 `0x6F7410` 直接 `ret`。因此即便 FSM 在本次调用中写了 state 0，本次 `Game::Update` 也不会继续落入其后普通更新/Room 分支；可把 active-field 窗口写作 state 1 至 state 0，把完整转场 owner 的本次尾声写到 ItemOverlay/FSM 返回和 `Game::Update` 返回。下一个普通更新 pass 才能作为后续稳定点候选；静态资料不证明其具体时刻。

### LevelInit 与房间/关卡 callback 顺序

`Level::Init` VA `0x73EB10` / RVA `0x33EB10` 有特殊分支：当 `Game+0x25AFC == 31 && stage == 11`（条件注释位于 VA `0x73FA13`）时，先 `RoomLoad`（`0x73FA2B`），紧接 `Room::Update`（`0x73FA36`）；否则仅调用 `RoomLoad`（`0x73FA3F`）。随后共同执行 `PlayerManager::TriggerNewStageAll`（`0x73FBFE`），再到 PostNewLevel dispatch helper（`0x73FC43`）。

`RoomLoad` VA `0x7385F0` / RVA `0x3385F0` 尾部在 VA `0x739719` / RVA `0x339719` 调 helper `0x84B930`；该 helper 设置 callback ID 19 并在其开关位允许时经 `0x8591D0` 分派，按当前回调映射标为 PostNewRoom。`Level::Init` 在 `0x73FBFE` 之后于 `0x73FC43` 调 helper `0x84BA10`；该 helper设置 callback ID 18、经同一 dispatch helper 分派，标为 PostNewLevel。callback listener 是否存在、listener 的间接调用闭包不由这个 callsite 保证。普通分支的顺序为：

`RoomLoad → PostNewRoom dispatch → TriggerNewStageAll / 每个现有 Player::TriggerNewStage → PostNewLevel dispatch → 返回 StageTransition::Update → 显式 Room::Update → SaveGameState → TempEffects`

特殊 stage-11 分支在 RoomLoad / PostNewRoom 后多出一次 `Room::Update`，之后仍走共同的 TriggerNewStageAll 与 PostNewLevel。state 4 的 Player update、state 清零、ItemOverlay 和 Game::Update 返回发生在上述 state-2/LevelInit 路径之后。不能以 RoomLoad 已返回、某 callback 已发起或 state 已写 0 单独标作“稳定房间 tick”。

### Manager 输入顺序、Join 与三处 predicate

`Manager::Update` VA `0x92E3E0` / RVA `0x52E3E0` 的 state-2 分支中，前置 `sub_999E10(manager+0x4CD50)==false` 且 `sub_929E50(...)==false` 时，于 VA `0x92E89F` / RVA `0x52E89F` 调 `Game::ProcessInput`。同一 state-2 分支之后只有 `[manager+0x4AD10]==0` 才继续到普通 `Game::Update` 调用 VA `0x92E911` / RVA `0x52E911`。顺序是 ProcessInput 在前、Game::Update 在后；`0x92E911` 是 Game::Update，不能误标为 ProcessInput。

`Game::ProcessInput` VA `0x6F66E0` / RVA `0x2F66E0` 的关键字节为：

- `0x6F6CD2` / RVA `0x2F6CD2`：`83 BF 78 BA 01 00 00` — `cmp dword ptr [edi+1BA78h],0`。
- `0x6F6CD9` / RVA `0x2F6CD9`：`75 40` — `jnz 0x6F6D1B`。
- `0x6F6D21` / RVA `0x2F6D21`：`E8 3A A6 29 00` — `call sub_991360`（JoinPass）。

所以 transition state 非零时，分支跳到 JoinPass 前的块并继续调用 JoinPass；这是跳过 state=0 的中间 pause/输入分支，不是阻止 Join。零状态下普通路径也能到 JoinPass，但该函数仍有其它条件/early return。换层自身的 `StartStageTransition` 与 FSM 没有 JoinPass 直接调用；不能把它们与 Manager::Update 的常规输入 pass 混为一谈。

`PlayerManager::CoopBabiesOnly` 的三个 code xref 都落在 JoinPass `0x991360` 内：VA `0x9915AE`、`0x99174C`、`0x991E67`（RVA 分别 `0x5915AE`、`0x59174C`、`0x591E67`）。这些是 Join 算法中的条件调用点，不代表每次 ProcessInput/JoinPass 必然执行全部三次。准确结论是：Manager guards 通过且 ProcessInput 未提前返回时，转场进行中仍可进入 JoinPass，并可按 JoinPass 自己的分支到达三处 predicate；换层 FSM 本身不触发 Join。

### Player 保留与 controller

`StartStageTransition` 遍历当前 Player vector，并在 VA `0x6F9F36` 调每个 Player 的 `TriggerRoomExit(false)`。FSM state 2/4 的 Player 更新通过 vtable `+0xC`；当前 vtable VA `0xB1BDB4` 的 `+0xC` 槽值为 VA `0x77BD40`（RVA `0x37BD40`，Player::Update）。`Level::Init` 的 TriggerNewStageAll `0x9929B0` 同样遍历传入的当前 vector 并调用 `Player::TriggerNewStage`（`0x9929FF`）；此函数只在现有 vector 上触发，不在该函数体直接创建 Player 或调用 `SetControllerIndex`。

Controller 的直接路径需分普通 Isaac 与特殊角色：`Player::Update` 在 VA `0x77BEC5` 读取 `Player+0x13BC`，比较 `0x1D`/`0x26`（29/38）；其它值在 `0x77BED3` 跳往 `0x77C2AC`。只有这两个 PlayerType，且 `Player+0x1E50` 子对象非空、子对象 `+0x170==0`，再满足主/子 Player 的 collectible `0x26B` 检查后，才会到 `0x77BF58` 读取当前 `Player+0x160C`，在 `0x77BF68` 保存该值，并于 `0x77BFCA` 调 `Player::SetControllerIndex(savedValue, 1)`。这是特殊关联角色路径；不能写成所有转场 Player::Update 都重绑 controller。普通独立 Isaac 的 PlayerType=0 在该处直接跳过 setter。其余已读 `StartStageTransition`、`TriggerRoomExit`、`TriggerNewStageAll` 和 `TriggerNewStage` 函数体没有直接 setter 调用。

## STATIC_INFERRED

在普通独立完整 Isaac A/B、Player vector 初始为两名现存 Player、没有自定义角色/回调移除或重建对象的前提下，转场开始、FSM Player 更新、LevelInit 的 TriggerNewStageAll 都是对已有 vector 的迭代；已读直接路径没有 PlayerManager 新增/删除或 Player constructor 调用。加上普通 Isaac 的 Player::Update 跳过上述特殊 controller setter，可静态推导普通 A/B 沿这条路径保留原 Player 对象与 controller index。它不是动态观察或对所有 stage/helper/callback 的保证。

## SOURCE

- wrapper/原生 ABI 与可选参数来源：`F:\projects_isaac\REPENTOGON-1.1.3\repentogon\LuaInterfaces\LuaGame.cpp:104–115,424`；`libzhl/functions/Game.zhl:68–69`；`docs/docs/Game.md:34–37`。
- 当前工程的 `OwnedVirtualDevice::Poll` 源码在 `cs-t0/native/src/owned_virtual_device.cpp:65` 返回 `true`。它只说明如果引擎在该窗口调用 K 的 Poll，该实现不会用 false 请求移除设备；不说明本窗口一定发生 Poll 或收到某个 transition 专项通知。

## UNKNOWN

- `StartStageTransition` 在 VA `0x6F9D82` 还直接调用 `sub_9950B0(566, ..., 1)`，其确切事件语义与是否触及 K 未由本轮关闭；Room/Level dispatch、Player::TriggerRoomExit/Update 的 helper 和模组 callback 也可能有间接影响。因此只能说已检查的有限直接路径没有识别出 K 专项注册/移除通知，不能说“整个转场没有设备通知”。日常 InputManager Poll/查询若继续发生，K 可沿兼容查询分派；该时序与分派细节仍 UNKNOWN。
- 特殊 `Game+0x25AFC==31 && stage==11` 的额外 Room::Update 分支、Genesis/null-effect 等特殊 Room/Player 路径，可能采用不同移除/重置行为；不外推普通 A/B 结论。
- 任意 helper 的间接调用闭包、真实 Lua callback listener 顺序/行为、Player vector 在动态运行中是否被外部 callback 修改、空 Player vector、duration 条件在具体存档中的取值，以及 state0 之后首个稳定普通 Room tick 的实际时间，均未由静态样本证明。
- 官方 `libzhl/functions/StageTransition.zhl:1–3` 声明 `_sameStage:0x10`，但当前机器码在 Game+`0x1BA84`（StageTransition base+`0x0C`）写 sameStage，在 Game+`0x1BA88`（base+`0x10`）写 transition int。该声明与实际写入偏移不一致。本报告遵循 v1.9.7.12 原生指令及实参数据流，不用该结构声明覆盖机器码。

## 取证更正记录

早期草稿曾把 sameStage 和 selected Player 的 Game-relative 偏移误写为 `+0x1BAC4`、`+0x1BAF0`。复核原生伪代码与字节后更正为 `Game+0x1BA84` 和 `Game+0x1BB70`；最终结论、脚本和 JSON 都采用更正值。另保留上文 `StageTransition.zhl` 声明与原生指令冲突，避免用旧结构布局误读运行时写入。




