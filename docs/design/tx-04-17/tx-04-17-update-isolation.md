# tx-04-17 ST3：J273 更新隔离与保留点

日期：2026-10-31，Asia/Singapore。状态：**ST3指定范围的J273静态研究、与ST2两轮互核及主智能体独立审阅完成。** 本项由用户指定的GPT-6 Luna／max子智能体推进；主审接受范围和更正记录见[专项工作流程](tx-04-17-workflow.md)的“ST2／ST3交叉核对与主审记录”。本文属于 tx-04-17，不是独立验收，也不表示 Client 隔离已实现。

范围：官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。只读目标为当前共享 IDA 输入 `C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`；地址同时给出 VA/RVA。没有切换 IDB、操作调试器或写入 IDB。只做静态分析和文档；未运行构建、校验器、测试或游戏，未修改运行代码。

## 1 证据标签与结论边界

| 标签 | 含义 |
| --- | --- |
| `J273_IDA` | 本轮在上述目标文件上的反汇编、反编译或交叉引用；调用点以汇编/xref核对，不以 Hex-Rays 行尾地址单独充作调用地址。 |
| `REUSED_J273` | 复用 [04-05 静态确认](../../research/tx-04-05-static-confirmation.md)、[04-08 ST2](../../research/tx-04-08-st2-static-confirmation.md) 与 [04-08 ST6—ST8](../../research/tx-04-08-st6-st8-static-confirmation.md) 中对应范围；不扩大其覆盖。 |
| `CURRENT_SOURCE` | 当前工作树或 REPENTOGON 1.1.3 源码可读事实；不代表本次实机安装的 Hook 配置。 |
| `J460_INDEX` | 03-10 是 J460 上的风险索引。其地址、偏移、ABI与路径不迁移到 J273。 |
| `DESIGN` / `UNKNOWN` | 拟用的隔离/展示方式，或静态证据不足之处；不等同运行行为。 |

核心判断：J273 的 Client 不能只跳过 `Game::Update`、普通 `Room::Update`、Odd helper 或 Player/input consumer。Manager 在调用这些路径前执行 Start/菜单处理，普通与 Odd 是条件分支，特殊 FSM 也在 GameUpdate 内另行更新；普通 Room 返回后仍有 Game tail，Manager 外层还会恢复位置、更新相机/状态并可能消费共享 RNG。静态证据证明这些路径存在，不证明每次都可达，也不证明本文列举已闭合全映像、全部派生类或全部 Lua 副作用。

## 2 Manager 外层及加载路径

### 2.1 最外层分派与重入

| VA / RVA | 入口、ABI与调用关系 | 隔离含义 |
| --- | --- | --- |
| `0x92E3E0 / 0x52E3E0` | `Manager::Update(bool)`；REPENTOGON ZHL 声明为 static `__stdcall`，主路径 `ret 4`。J273 IDA直接代码 caller 为 `_main` 的 `0x90AF08` 和 Game 内层 `0x6F42AD`，共两个静态直接 caller。 | 这是覆盖本地 Start、菜单切换、Game/Odd 分派与 Manager 尾部的已知外围候选。Game 内 caller 令同一 Manager Update 可嵌套；不得把每次入口都当独立顶层帧。04-08 已记录当前 `frame_depth` 重入检测。 |
| `0x932A20 / 0x532A20` | `Manager::ExecuteStartGame()`，ZHL 为 `__thiscall`、Manager 在 ECX、无显式参数。Manager 在 `0x92E46E`、状态 switch 之前调用；IDA只有该直接 caller。 | 函数每个 Manager Update 都可进入；“本次没有开始请求”只会早退。若保留 Manager 主体，必须先截断或隔离这个加载 owner。 |
| `0x933060 / 0x533060` | Manager 在 `0x92E475` 及另一状态转换路径 `0x92E57C` 调用。IDA 入口是带 Manager ECX 的内部 helper；不把反编译器推定的 EBP/EDI 值升级为公开 ABI。 | helper 检查转换标志和 Manager 状态，调用 menu/state helper、分配约 `0xF938` 大小的对象并更新其状态。不是可证明无副作用的空转段。 |

Manager 的 `0x92E45A` 是位于 `0x92E46E→ExecuteStartGame` 之前的条件早退分支；走主 fallthrough 才会执行 `ExecuteStartGame`、`0x933060` menu/state helper，再读 Manager state 并分派。该后续分支另有 `0x92E80D` 条件早退与主路径结束 `0x92EC22`。因此不能把 `0x92E45A` 说成每帧 Start 前置逻辑已执行，也不能只保护会到 `0x92E80D` 的 case 分支；Client 边界要在整个 ManagerUpdate 入口外覆盖两类路径。case 2 的内部条件另决定普通 Game 或 Odd helper。

`CURRENT_SOURCE`：C/S 当前 `manager_update` Hook 清单项为 RVA `0x52E3E0`、priority `-10000`；[gen_binding.py](../../../mod/tools/gen_binding.py) 的 `manager_update` wrapper 在 `FrameScope` 后无条件 `super(value)`（497—499行）。它目前不是 Client 短路。ZHL priority 的外层关系可作未来候选，但同 priority 次序、未知更外层 Hook、实际安装和命中均未在本轮观察，不能声称已形成 Client 边界。

### 2.2 ExecuteStartGame、Game 构造、Start 与 Continue

Manager 的 `ExecuteStartGame` 位于 ordinary/odd switch 之前。REPENTOGON 快启 Hook 在调用 Manager::Update 的 `super` 前可设 `Manager::_starting` 等字段（[Quickstart.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Quickstart.cpp:22)）；`Isaac.Pause` 的 Manager::Update Hook 也会在 `super` 前写 `Manager::_state=2`（[LuaIsaac.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaIsaac.cpp:477)）。因此 Client 若需阻断 Manager，检查必须发生在能包住已知 REPENTOGON wrapper 的外层；只跳 case 2、GameUpdate 或 Start 函数体不够。实际 hook 链及同优先级行为仍 UNKNOWN。

| 路径 | J273 调用点与可确认副作用 |
| --- | --- |
| 新建 Game | `ExecuteStartGame` 在 `g_Game == nullptr` 条件下于 `0x932ACF` 调 `sub_6ED4B0`；该构造路径在 `0x6EE128`、条件分支 `0x6EE137` 调 `0x6EB4B0`。后者是全局 RNG/seed helper，已有 04-08 ST6—ST8 证据显示其会推进 RNG 状态并维护追溯数据。正常菜单已有 Manager/Game/基础 Room 根，不应为 Client 新建 Game；既有 Game ctor 也不能作为无副作用的可显示 Room 壳。 |
| 新游戏 Start | ExecuteStartGame 的真实 Game::Start 调用点为 `0x933007 → 0x6F16D0 / RVA 0x2F16D0`；不是前面的 Game 构造点 `0x932ACF`。ZHL `Game.zhl:102` 声明 `__thiscall void Game::Start(int playertype,int challenge,Seeds,unsigned int difficulty)`。Start 内有种子/ItemPool/Level/Room与实体初始化路径；普通启动中会更新 Room，并可进入 Lua/PostGameStart。不得用原生 Start 来做无权威 run 的 Replica bootstrap。 |
| Continue/Restore | ExecuteStartGame 的 `0x932E7E → Game::RestoreState 0x6F43A0 / RVA 0x2F43A0`；随后 `0x932E89` 调 controller reassign，`0x932E94/9F` 做 PlayerManager 后处理，`0x932EAA → Room::Update 0x7F7F60 / RVA 0x3F7F60`。GameRestore 继续 ManagerRestore、LevelRestore、PostLevelInit；并不只是恢复展示字段。 |
| 其他开始分支 | ExecuteStartGame 还在 `0x932DC8`、`0x932F84`、`0x932FAA`、`0x932FDD` 分派其它 start/rerun/daily/challenge helper。具体产品语义不在本轮逐一命名；其中可达 Room 调用已由下文的 Room xref 列出。 |

`REUSED_J273`：04-05 §6 已指出加载 owner 可在 NewRoom callback 之前更新 Entity；回调后也可能再定位 Player/更新 Room。故 NewRoom、PostGameStart、GameRestore 返回或单个 RoomUpdate Hook 都不是安全的隔离起点/终点。当前 Start wrapper 只建立 `RunStart/LoadingOwner` 与持久观察 scope，然后仍调用原函数（[gen_binding.py](../../../mod/tools/gen_binding.py:555)及586行）；Scope 不会阻止原生加载或 Lua。

## 3 普通 Game、Room、EntityList 与特殊 FSM

### 3.1 普通更新与跳过点

| VA / RVA | 直接调用/ABI | 影响 |
| --- | --- | --- |
| `0x92E911 / 0x52E911` | Manager case 2 的 GameUpdate call，目标 `0x6F6D30 / 0x2F6D30`；`Game::Update` 为 thiscall、ECX=Game、无显式参数。 | 普通主更新不是 Manager 全部工作；其前已有 Start/menu/input处理，其后仍有 Manager case tail。 |
| `0x6F78DF / 0x2F78DF` | Game 从 `[Game+0x18300]` 取 Room 作 ECX，call `0x7F7F60 / 0x3F7F60`。Room::Update 为 thiscall、ECX=Room、无显式参数。 | Room 正常返回不表示 GameUpdate/ManagerUpdate 已结束。Game 还做后处理，并可在 `0x6F7A34 / 0x2F7A34` 调标准 tail。 |
| `0x7F93A7 / 0x3F93A7` | Room 内 `push 0`，ECX 指向 `Room+0x1218` 的 EntityList，调用 `0x413F00 / 0x13F00`。实参 0 是静态指令事实；不要把 wrapper 参数名 `partial` 当成额外语义证明。 | EntityList 动态遍历实体并通过虚调用进入 Update 等实体方法。J273 EntityList 的直接指令可见 slot `+0x0C` 调用（如 VA `0x4141A9`、`0x4145F7`、`0x4146AB`、`0x41472F`），不是纯渲染列表读取。 |
| `0x6F7A34 / 0x2F7A34` → `0x846C90 / 0x446C90` | Game 普通主路径的 tail；`PostUpdateCaller` 是 cdecl、无显式参数。 | 位于 Room 和 Game 后处理之后；只跳 Room 或提前跳 Game 某段，不会自动阻断此 tail。 |

REPENTOGON `MC_PRE_UPDATE(1026)` 安装在 GameUpdate 内 `0x2F77A4`。true 分支运行 CommitRemovedEntities、render_sort 和 HUD::PostUpdate，再跳到 `0x2F7A39`；它会越过普通 Room 与标准 PostUpdate tail，但保留上述三项和之前已执行的 Game 工作。源码证据为 [ASMCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp:45)。因此 1026 是局部 skip，不是 Client 全隔离开关。

GameUpdate 还有多处早退：Console gate（`0x2F6D69`）、PauseScreen（`0x2F775B`）、loading/lifecycle 与其它恢复状态条件可让它在 Room 之前返回；使用同一 epilogue 不代表经过 Room/tail。具体 Console、暂停菜单和失焦区分见 04-05 §5。静态入口出现不能推定某个运行帧确实抵达普通 Room。

### 3.2 Room、碰撞、伤害、拾取/物品与随机

- **AI/实体更新**：EntityList 的 slot `+0x0C` 按动态类型调 Entity/Player/NPC/Tear/Familiar/Bomb/Pickup/Projectile 等更新；REPENTOGON 的 1160—1168 per-entity pre-update callback 若返回 true，只跳过派生 Update，仍调用 `Entity::Original_Update()`（[CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp:497)）。因此这些回调不代替整个实体模拟隔离。动态类全集与派生调用副作用未闭合。
- **碰撞与伤害**：J273 EntityList VA `0x4146B2` 直接调用碰撞 helper `0x6A6A40 / RVA 0x2A6A40`；Odd helper 也从 `0x6F9560`、`0x6F9650` 调同一函数。IDA 字符串和函数体可见 Player/Grid collision、GridEntity Factory/Init/槽位写路径（包括 `0x705070`）；这是有状态碰撞，不是仅触发显示。当前 `world_layout.json:134` 给出 base Entity vtable RVA `0x718224`；imagebase `0x400000` 后，slot `+0x0C` 的 VA `0xB18230` 有指向 J273 `sub_6A9CF0 / RVA 0x2A9CF0` 的 data xref，与 ZHL `Entity::Update()` 声明（`Entity.zhl:264`）相符；这是 `__thiscall void`、ECX=Entity 的虚分派。该函数在 VA `0x6AAB6F/0x6AAB77/0x6AAB7E` 对 `[Entity+0x380]` 做条件减值并写回；按 Entity 字段布局把该值解释为 HP，但指令本身仅证明减值和写回。条件路径在 `0x6AAD8A` 通过 base vtable slot `+0x24` 虚调 `Entity::Kill`（ZHL `Entity.zhl:275`）。这确认基 Entity Update 有状态伤害/死亡处理，但不闭合所有派生更新、HP变化来源或碰撞伤害链。旧索引地址 `0x6AE820 / RVA 0x2AE820` 不作为本目标证据。`Entity::TakeDamage` 的 ZHL ABI 为 `__thiscall bool TakeDamage(float damage, uint64 flags, EntityRef* source, int countdown)`（隐式 `this` 不计入显式参数）；REPENTOGON 的 Player TakeDamage 还可进入 1008 Lua callback（`CustomCallbacks.cpp:584`）。当前 C/S 清单没有统一客户端伤害否决边界；回调否决也不能回滚此前的 RNG、碰撞或直接字段写。
- **Pickup/Use/Drop**：Player 的普通更新与 Odd 普通分支会进入 Player 派生更新；对应可达接口包括 Entity_Pickup::HandleCollision(Entity*, bool)（`EntityPickup.zhl:23`）、GetLootList(bool, Entity_Player*)（`EntityPickup.zhl:56`）、Player ControlActiveItem、drop/card/bomb/shoot consumers。当前 Hook 表中确有 `actual_pickup_loot`、`actual_shopkeeper_loot`、`active_item`、`drop` 等点，但注册/局部 consumer Hook 不覆盖所有掉落、拾取、道具效果或 Lua 路径；`active_item` 当前行仍为 disabled。C/S 行为还没有 Client 分支。
- **RNG**：Game 构造/Start 已有全局 seed/ItemPool副作用。Manager case 2 的外层 camera/position helpers 也有条件 RNG：`sub_91E5E0` 内 VA `0x91E65D`、`0x91E6C1`，`sub_91D840` 内 VA `0x91E25B`、`0x91E2CA` 调全局 helper `0x6EB4B0`。这些点只在相应分支条件满足时运行，不表示每帧都会消耗；仍说明“仅为相机保留原 helper”可能推进共享 RNG。适配应从 Authority 状态导出 camera 值并在 Client 不推进原生 RNG。所有 Entity/Mod/Item 的随机消费者没有全量闭包。

这些事实足以说明保留 Room/EntityList/Player/输入或 camera 原生后处理存在玩法写入；不能据此声称已找完全部伤害、拾取、掉落、道具和随机来源。

### 3.3 RoomTransition/FSM 与奇数 helper

普通 Manager case 2 根据 frame parity/条件在 GameUpdate 与 Odd helper 之间分流：`0x92E911` 调 GameUpdate，`0x92E921` 调 Odd `0x6F9340 / RVA 0x2F9340`。两条不能互相替代；Odd 路径不调用上述普通 Room 或 Game PostUpdate tail。

Odd helper 的三个 Player slot `+0x0C` call 分别是：

| VA / RVA | 分支 |
| --- | --- |
| `0x6F93F9 / 0x2F93F9` | `Game+0x1B83C` 对应特殊状态。 |
| `0x6F949C / 0x2F949C` | `Game+0x1BA78` 对应另一特殊状态。 |
| `0x6F9559 / 0x2F9559` | 普通 Odd Player 循环；helper 中存在 Room/EntityList 对象检查、Player 更新及碰撞相关处理。 |

这些地址、条件和虚调用依据为 04-05 §2/J273；slot `+0x0C` 为实体更新，不代表 Authority 普通 Player tick。前两个特殊 Player 调用位于普通 Console/Pause/Focus 条件之外的路径；正常 Odd 分支仍有 Console、PauseScreen 与失焦早退，不能把 helper 入口记成一次完整模拟。

GameUpdate 的 RoomTransition FSM 另有独立内层分支：当 `Game+0x1B83C` 状态对象存在，J273 call `VA 0x6F7420 / RVA 0x2F7420 → RoomTransition::Update VA 0x826430 / RVA 0x426430`；ZHL 为 `__thiscall void RoomTransition::Update()`，ECX 为 transition 对象。此更新体内部另有条件 EntityList 更新：VA `0x8264D3`、`0x826EAB`、`0x826F1C`、`0x826FD9` 调 `0x413F00 / RVA 0x13F00`；条件 Player 虚 `+0x0C` 更新位于 VA `0x826557`、`0x8265A2`、`0x8265C5`、`0x826EE4`、`0x826F43`、`0x827036`。因此仅屏蔽普通 RoomUpdate 仍不能隔离 RoomTransition 中的实体/Player 更新。当前目标对 `0x826430` 有五个静态代码 xref：`0x6F7420`（GameUpdate）、`0x41CC53`、`0x425346`、`0x741A73`、`0x909BC8`；其它 caller 的完整状态/生命周期语义未在本轮展开。返回后还有后处理和条件早退，正常 Room 不保证执行。它与 Odd helper 的两个特殊 Player 分支是不同路径；此处使用 J273 点位，不采用 J460 上相似的地址叙述。

## 4 Manager 外层位置恢复、相机与尾部

Manager 的偶数/普通分支在调用 GameUpdate 之前，直接遍历 Game Room EntityList：对标记实体将 staged `+0x344/+0x348` 写回当前位置 `+0x33C/+0x340`，再清标记 `+0x175`（J273 VA `0x92E8E8–0x92E8FA` 周围）。这是 Game/Room/Player hook 之前的原生位置写入；只跳过 GameUpdate 或 Room 无法取消它。是否每个 Manager 帧都满足标记条件是运行时未知。

分支完成后，Manager 还可能执行 camera/position postprocessing（`0x91E5E0`、`0x91D840`）、状态过渡和计时等。前节列出的四个 `0x6EB4B0` callsite 是实际汇编点，条件调用共享 RNG；即使最终把逻辑分类为相机，这些 helper 也不能原样当作无玩法展示更新。后续相机适配使用 Authority 已同步的位置/目标/过渡状态，在本地写显示状态；不要为得到相机结果调用原生 GameUpdate、RoomTransition 或 RNG helper。

Manager/Room 的外层后处理不能以“GameUpdate 被跳过”推定停止。若 Client 仍执行原 `Manager::Update`，`ExecuteStartGame`、菜单转换、预恢复位置和 case tail 均仍然可达。若未来短路整个 ManagerUpdate，则 Client 的连接消息、资源生命周期、停止/退休与渲染调度必须在 wrapper 短路前由独立主线程路径完成；不能靠 Authority 的 `BeginManager/EndManager` 给 Client 伪造 tick。当前主线程 owner/context 可复用，但没有 Client 入口或本地展示壳。

## 5 当前 Hook 与建议的保留/跳过/展示适配

此表为 `DESIGN`，是后续 16/17 的接线输入，不是已实现能力。

| 路径 | Host/Off | Client 建议 | 限制 |
| --- | --- | --- | --- |
| `Manager::Update 0x52E3E0` | 原链 exactly-once，保留现有 Host/Off 语义。 | 在可证明包住已知 REPENTOGON wrapper 的最外层先处理 Client 消息/生命周期，然后不进入原生玩法 Manager 主体。 | 当前 C/S wrapper仍无条件 super；priority/安装/未知 hook链须在后续实现和整体验收验证。嵌套 Manager 不开第二套 outer tick。 |
| `ExecuteStartGame`、Game Start/Restore、`Room::Update`、`EntityList::Update`、Odd、RoomTransition FSM | 原版链正常运行。 | 直接跳过原生本地模拟路径；由18—20状态应用和明确的展示 adapter 驱动。 | 不以 1026、1160—1168、输入中性、Room或Player单点作为隔离措施。Start/Restore不能用于客户端建壳。 |
| Manager staged-position restore / camera tail | Host/Off 正常。 | 不执行位置恢复和 RNG camera helper；用 Authority 快照更新本地相机/位置展示。 | 具体外层字段/动画过渡继续由 18—20/表现模型定义；未知Lua写路径仍须限制。 |
| Game Render/PostUpdate、HUD、动画/普通Lua | 按原语义。 | 只保留有明确展示 owner 的回调/绘制；隔离任意 gameplay callback。 | Game::Render 内含 Room/Entity/HUD 绘制工作，不由单 Sprite 或 `MC_POST_RENDER` 自动变纯展示；ST4继续闭合。 |
| 菜单展示候选 | 原菜单链正常。 | ST2 提出的 `MC_MAIN_MENU_RENDER(1023)` 与自有屏幕 Sprite 可作为不新建、也不依赖有效 Game/Room world 根的窄资源展示候选；不把它称作完整 Replica World。 | 正常菜单已存在 Manager/Game/基础 Room 根；MenuManager、context和启停的实机行为仍 UNKNOWN；Game::Render 与菜单 renderer 是不同入口。 |

`CURRENT_SOURCE`：`mod/tools/gen_binding.py:518–538` 为 Game/Room/tail 建阶段 scope，Room/Odd 另建 SimulationScope；随后仍调用 `super`。Player Shoot consumer 有单独跳过条件，不能推出其他 consumer 或世界停止。观察 Hook 的 C++ 异常 RAII见 `mod/src/hooks/context.cpp:10–37`；它记录展开并撤销 owner token，不提供 SEH/access violation/强制退出恢复保证。若实现 Client skip，应显式保证所有退出路径仍收回 Client 生命周期；不能把原生异常包装误报为“已安全退休”。

## 6 早退、重入、异常及未闭合范围

- Manager::Update 有多处早退，且 Start/menu 前置逻辑位于它们之前；外层 scope 应按调用栈配对。静态 caller 显示 `_main` 与 Game 内 `0x6F42AD` 都会调用它，04-08 当前 frame_depth 将后者视作重入，不给嵌套调用完整新样本。
- GameUpdate 可因 Console、暂停、loading/lifecycle、Door/FSM和其他状态条件早退。标准 tail `0x846C90` 正常主路径在 `0x846D15` 返回，另有内部错误处理 funclet `0x846D16–0x846D63`；tail 返回不证明 Lua callback 成功。
- 该 tail 的 PostUpdate callback 位置已在 J273 静态追到 Lua：`0x846CEB → 0x8591D0`，后者经 `lua_rawgeti`、两次 `lua_pushinteger` 在 `0x85921B` 调 `lua_pcallk`，再管理引用并由 tail `luaL_unref`。它是条件路径，不证明每次进入或必然改世界；但保留 tail 不能称作“无 Lua”。
- 普通 RoomUpdate 有 10 个静态代码调用点：GameUpdate `0x6F78DF`、ExecuteStartGame `0x932EAA`，以及 `0x6F1B9D`、`0x6F2671`、`0x6F3A3E`、`0x6FA17C`、`0x70213B`、`0x732E57`、`0x73FA36`、`0x909B26`；另有一个 VA `0x853101` 数据 xref，可能关联回调/注册路径，具体运行 owner 未闭合。部分代码 owner 属 Start/Restore/transition；不以单一 GameUpdate 上下文覆盖所有 Room 调用。
- Lua callback 本身可改实体、生成物品或调用持久 API；J273 的实际 Mod 列表、动态 callback、Entity 派生、异步和跨入口覆盖未穷尽。这里的 `Lua pcall`、碰撞/RNG 与 update paths 是已确认的可达风险，不是全部清单。
- `J460_INDEX`：03-10 的 outer restore/odd/FSM/renderer风险只用于解释为什么选择这些 J273 核查项；没有将 J460 地址、寄存器或 ABI搬入本报告。J273 仍未证明所有线程、所有 native caller、所有 SEH、所有实体类及所有 Mod Lua callback 都被同一主循环支配。
- 没有静态或运行证据证明当前安装 Hook 全部注册、命中、按预期优先级包裹或对 Host/Off/Client 已隔离。执行完成后的统一 E2E 才观察这些运行行为；本ST不分项测试，也不宣称功能 PASS。

## 7 向 ST2、ST4 和 tx-04-15/16 的反馈

- ST2：原生 Game 构造、Start、Restore和RoomUpdate可能触及 seed、Player、Level/Room、实体、回调或后处理；不能以这些入口建立“零玩法”客户端场景。依托现有菜单 renderer、且不新建或依赖有效 world 根的菜单 Sprite 仅是资源展示候选，不能满足房间、相机、HUD或世界实体语义。
- ST4：更新路径与 Render 分离不等于 Render 子树无副作用。Game tail PostUpdate Lua、Entity/Room绘制与 HUD/动画状态需按明确 presentation owner 适配；不能用 1026 来推定可以任意执行 Lua。
- tx-04-16：候选边界是完整 ManagerUpdate 的 Client 外层短路，而不是普通 Game/Room/Odd 局部点。Client 消息泵、等待、失败和正常退休需在短路原链前有独立主线程路径；退出/资源清理未完成前不得转 Off 让剩余原生链重新模拟。
- `DESIGN`：Client 屏蔽本地 Authority tick 不应丢弃正式上行 input intent；通过独立网络输入路径保留与 Solo/Host 相同的用户操作语义，不让本地原生 Player/input tick 再应用这些意图。
- tx-04-15：把 Camera 值、RoomTransition 显示状态、Entity 位置/姿态和资源错误分别表达。相机本地适配不得调用会上行或消费 Authority RNG 的本地原生 FSM。

本文是静态路径和设计输入。没有实施隔离、适配、运行时记录或整体验收；保留上述 UNKNOWN，不把 1026、Manager wrapper、一个 Room/Player consumer、资源画面或历史 J460研究称为“完整隔离”。
