# tx-04-17 ST2：Client 场景根与生命周期

研究日期：2026-10-31，Asia/Singapore。状态：**ST2指定静态研究、与ST3两轮互核及主智能体独立审阅完成**。按任务指定 GPT-6 Luna/max 做静态阅读与必要目标二进制查询；Client世界壳仍未实现。

互核状态：已完成与 ST3 的两轮交流和交叉阅读。最终更正记录：将 `0x92E45A` 标为 `0x92E41D` guard 分支的 prelude early return，将 `0x92E80D` 标为 Start/menu 前置处理后的较晚退出；Camera 构造从误指的 `0x91EC20` 更正为 `0x91C730`，并核实 Room setup 的分配、传参及 `+0x11F8` 回存。主审独立回读后提出的角色输入语义与细节修订已合并。关联记录：[ST1 公共接口目录](tx-04-17-public-interface-catalog.md)、[ST3 更新隔离研究](tx-04-17-update-isolation.md)。

## 范围与证据标签

本报告只研究 Client 展示场景所需的根、初始化、准备、取消与退休。依据是仓库源码／本地 Wiki 与目标二进制的静态阅读；没有运行游戏、构建、测试、哈希或校验脚本，也没有修改运行代码或 IDB。

- `SOURCE`：可读的仓库源码、ZHL 声明或本地 Wiki 文档事实。
- `BINARY_STATIC`：当前 IDA 输入中的指令／反编译静态事实；不表示运行时命中或副作用必然发生。
- `DERIVED`：基于上述事实的控制流或风险推导。
- `DESIGN`：给 tx-04-16 的生命周期建议，不代表已有实现。
- `UNKNOWN`：当前证据未闭合的问题。

IDA 当前输入为 `C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe`，image base `0x400000`；IDA 数据库为同路径加 `.i64`。本次仅使用只读反汇编／反编译查询，未切换输入或控制调试器。

## 结论

当前证据没有证明存在可通过原生 `Game::Start`、Continue/Restore 或 Room 更新建立的“无本地玩法副作用 Client 世界壳”。正常启动已经有一个可复用的 `Manager`、`Game` 和空 `Room` 根，但 Game 构造本身会调用全局 RNG helper；这个 Room 的 Camera 指针为空，且没有证据表明它是已填充楼层、实体、HUD 与 Camera 的可显示世界。不能为了 Client 再 `new Game`，也不能把菜单 Sprite 展示称作 Room/HUD 世界。

目前源码和目标二进制能具体描述的最小候选限于菜单 UI/资源显示：复用正常启动的 `Manager`/`MenuManager`/Lua/图形路径，在 `MC_MAIN_MENU_RENDER` 中绘制 Client 自有屏幕 Sprite。它可承载加载状态、连接状态或调试性资源展示，但当前安装上的可用性与安全退休仍未知，不能满足 16 的 Room/Camera/HUD/Entity 世界展示目标。完整世界需要独立的 Replica 场景根与逐项封闭过的构造／更新／销毁适配；这些尚未证明可安全执行。

## 根对象与初始化顺序

### 正常启动已有根

`SOURCE`：REPENTOGON ZHL 声明 `Manager::Init` 为静态函数（`F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Manager.zhl:2`），`Game::constructor` 为 `__thiscall void`（`Game.zhl:2`），`Room::constructor` 为 `__thiscall Room*`（`Room.zhl:2`），`Camera::constructor(Room*)` 为 `__thiscall void`（`Camera.zhl:2`）。`Game::_room` 位于 `Game+0x18300`（`Game.zhl:272`）；`Room::_Camera` 位于 `Room+0x11F8`，`GetCamera()` 只返回该指针（`Room.zhl:157,189`）。

`BINARY_STATIC`：`Manager::Init` VA `0x92BA20` / RVA `0x52BA20` 在 VA `0x92BED0` 调 Game ctor `0x6ED4B0`，VA `0x92BEDC` 发布 `g_Game`，随后 VA `0x92BEE1` 调初始化 helper `0x6F0AF0`。因此正常菜单启动已持有 Manager 与 Game 根；新 Client 不需要、也不应额外创建 Game。

`BINARY_STATIC`：Game ctor `0x6ED4B0` 在 VA `0x6ED5FE` 调 `0x732D20`。该 helper 检查并可能销毁旧 `Game+0x18300` Room，在 VA `0x732D6D` 调分配器取得 `0x7898` 字节，并于 VA `0x732D81` 调 Room ctor `0x7DF8F0`；VA `0x732D88` 写回 Game 的 Room 指针，随后设置房间索引。Room ctor 的反编译在 VA `0x7E00E8` 将 `Room+0x11F8` 设为零。因此，Game ctor 确实创建一个基础 Room 对象，但没有 Camera 指针，也没有证据证明该 Room 已经承载楼层内容。

`BINARY_STATIC`：Game ctor 还在 VA `0x6EE128` 调全局 RNG helper `0x6EB4B0`，条件分支在 VA `0x6EE137` 再调一次，随后 VA `0x6EE14B` 调 `0x4257A0`。tx-04-08 ST6 对 `0x6EB4B0` 的既有研究将其识别为全局 RNG 状态读写／调用追溯相关 helper。这里能证明的是构造路径存在 RNG helper 调用点；是否进入条件分支取决于状态，不能据此声称每次构造都推进相同次数。

`BINARY_STATIC`：`0x6F0AF0` 在 ctor 后由 `Manager::Init` 调用；另一个调用点是 `ExecuteStartGame` 创建 Game 时。已有主审静态阅读发现该 helper 还装载 stages、curses、ambush、preload 并重置若干状态。其确切 ABI、每项副作用的条件和完整清单未由当前 ZHL 签名闭合，不能把它视作空的字段初始化。

`BINARY_STATIC`：Camera.zhl:2 的完整 SEH/prologue signature 在本目标中定位到 `Camera::constructor(Room*)` VA `0x91C730` / RVA `0x51C730`（完整签名定位由主审核定，本轮复核调用与回存），不是 `0x91EC20`。其唯一已见代码 xref 是 VA `0x7ED7E5`，位于 Room setup owner `0x7E8AD0`：VA `0x7ED7C8` 分配 `0x94` 字节，VA `0x7ED7DB` 压入 Room 指针 EDI，VA `0x7ED7DC` 把新对象设为 ECX，随后调用构造函数；VA `0x7ED7F7` 把返回值写到 `Room+0x11F8`。`0x7E8AD0` 的已见调用者为 `0x7385F0`；后者分别从 `0x739DE0`、`0x73EB10`、`0x7415E0` 的房间/楼层处理路径到达。故“基础 Room 的 Camera 为空”只描述 Game ctor 后的初始状态，正常 Room setup 之后会另行构造 Camera。

`BINARY_STATIC`：Camera ctor 初始化六组环形缓冲，保存传入的 Room 指针，并读取全局 Game 的当前房间/进入门及 Room 几何相关字段，最后写入 `Room+0x1204/+0x1208`。这是有效 Room setup 的一部分，不能对 Game ctor 留下的空 Room 直接调用；这条原生路径能构造 Camera，不等于可安全复用它来构造 Client 场景。

`BINARY_STATIC`：Room setup owner `0x7E8AD0` 自身在 VA `0x7ECA34`、条件 VA `0x7ECAA2` 与 `0x7ECAC7` 调用全局 RNG helper `0x6EB4B0`，且有实体创建调用路径。因此连同 Camera ctor 在内的原生 Room setup 不是“只补镜头字段”的安全准备函数。

### 最小依赖与准备顺序

| 顺序 | 依赖 | 当前证据与边界 |
| --- | --- | --- |
| 0 | 窗口、renderer 与当前图形 context | `UNKNOWN`：正常菜单已能进入 `Manager::Render`，但本报告没有证明目标 J273 下 context 的线程归属、加载旁路及在途提交完成点。资源服务必须在已确认的主线程/context 窗口调用。 |
| 1 | 既有 `Manager`、`Game`、`LuaEngine`、`MenuManager` | `SOURCE/BINARY_STATIC`：正常启动 `Manager::Init` 创建/发布 Game；菜单 callback 经现有 LuaEngine 与 MenuManager render owner。Client 应复用这些根，不再构造第二个 Game。 |
| 2 | 菜单 UI 候选：自有 Sprite、资源与 callback | `SOURCE`：`MC_MAIN_MENU_RENDER` 可由 MenuManager render 调用并显示 Sprite。候选只需要菜单屏幕坐标与资源；不需要 Room、实体或 HUD。Lua/GL 资源的最终退休仍 `UNKNOWN`。 |
| 3 | 世界 Room 的布局与成员 | `SOURCE`：Game 布局中嵌入 `PlayerManager +0x1BAA8`、HUD `+0x1D9AC`，Room 指针在 `+0x18300`（`Game.zhl:272,290,295`）。Room 内含 grid width/height `+0xC/+0x10`、grid entities `+0x24`、doors `+0x724`、Camera 指针 `+0x11F8`、EntityList `+0x1218`（`Room.zhl:155,182-195`）。这些是内存布局/成员位置，不证明有有效玩家、房间几何、门、实体、HUD 可绘状态或镜头。 |
| 4 | 有效当前 Room、Camera 与展示成员 | `BINARY_STATIC/DESIGN`：Camera 仅由 Room setup 路径在 Room 指针和布局/几何有效后创建；Start 则创建 Player 并生成楼层/房间。Client 世界若不能调用原生玩法 setup，就需要数据驱动的独立 Room/Camera/render adapter，并逐个证明成员和 draw-state 来源。不能把 `Game` 内嵌字段或基础 Room 分配等同于世界场景准备完成。 |

`DERIVED`：目前能从源码与二进制具体描述的最小 prepare 顺序是“复用正常启动根 → 确认菜单 callback 的主线程图形窗口 → 准备自有 Sprite → 仅在 MenuManager callback 绘制”。世界场景还需要有效布局、Room 成员、Camera 几何以及实体/HUD 的显示输入；现有原生 Start 会提供其中一部分，但同时进入 RNG/玩法链，不能直接拿来填壳。

### 原生启动与恢复不是场景构造接口

`SOURCE`：`Manager::Update(bool)` 是 `Manager.zhl:5` 声明；当前 `hooks.json:18` 将目标 RVA `0x52E3E0` 配为 stdcall hook。`Manager::ExecuteStartGame()` 是 `Manager.zhl:46` 的 `__thiscall`。`Game::Start` 是 `Game.zhl:102` 的 `__thiscall void(int playerType,int challenge,Seeds seeds,unsigned int difficulty)`；`RestoreState` 是 `Game.zhl:11` 的 `__thiscall void(GameState*,bool)`。

`BINARY_STATIC`：`Manager::Update` VA `0x92E3E0` / RVA `0x52E3E0` 在状态 switch 前于 VA `0x92E46E` 调 `ExecuteStartGame` VA `0x932A20` / RVA `0x532A20`。新局分支仅在全局 Game 指针为空时，才于 VA `0x932ACF` 调 Game ctor `0x6ED4B0`，随后 VA `0x932AE0` 调 `0x6F0AF0`；已有 Game 时复用已有对象。VA `0x933007` 调 `Game::Start` VA `0x6F16D0` / RVA `0x2F16D0`。Continue 分支在 VA `0x932E7E` 调 `Game::RestoreState` VA `0x6F43A0` / RVA `0x2F43A0`。

`BINARY_STATIC`：Start 路径 VA `0x6F1877` 与 `0x6F19FE` 调 seed helper `0x6EBE10`，VA `0x6F1A0D` 初始化 ItemPool，VA `0x6F1AE6` 创建 Player，VA `0x6F1B07` 调楼层／房间生成 helper `0x73EB10`，VA `0x6F1B9D` 更新 Room `0x7F7F60`，VA `0x6F1BAE` 调 `PostGameStart` `0x84B660`（state=0）。另在 VA `0x6F1B43` 调 `0x932160`；该 helper 的正常条件分支包含 GameState/PGD 保存 helper 调用，早退条件存在，不能说每次都写盘。对 `0x6EBE10` 的本轮反汇编可见它在 VA `0x6EBE61` 写入 `this+8` 的 RNG 状态，因此该 seed helper 会推进相关 RNG 状态。

`BINARY_STATIC`：Continue 在 Restore 后还执行 PlayerManager／PostLevelInit 后处理，VA `0x932EAA` 更新 Room，并在 VA `0x932EDF` 调 `PostGameStart`（state=1）。既有 `docs/research/tx-04-08-st6-st8-static-confirmation.md:85-89` 将 Restore 继续链定位为 GameRestore `0x6F43A0` → ManagerRestore `0x993A90` → LevelRestore → PostLevelInit `0x993FA0`。它是恢复玩法状态的路径，不是空显示壳。

`DERIVED`：即便只需要画面，调用 Start/Restore 也会带入 seed/RNG、ItemPool、Player、楼层/房间构造、Room 更新及 Lua 回调；Continue 还会进入恢复与 PostLevelInit。`Manager::Update` 的 Client 逻辑必须在 `ExecuteStartGame` 之前截断，不能只屏蔽后来某个普通 Player tick。

## 渲染与可行候选

`BINARY_STATIC`：`Manager::Render` VA `0x92EC40` / RVA `0x52EC40` 是独立于 Update 的路径。其状态分派在 VA `0x92EE08`：菜单态走菜单 render helper `0x9623C0`，Game 态走 `Game::Render` VA `0x6F7A80` / RVA `0x2F7A80`。Game::Render 内 VA `0x6F7E02` 调 `MC_POST_RENDER` callback `0x849DD0`。因此 `MC_POST_RENDER` 不等于菜单展示钩子；保留 Game Render 会进入 Game/Room/Entity/HUD 与 Lua 消费链，不能作为无玩法的通用展示边界。

`SOURCE`：官方 REPENTOGON 将 `MC_MAIN_MENU_RENDER` 定义为 callback id 1023（`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/ModCallbacks.md:1298-1303`）。`F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp:1528-1558` 的 `RunMainMenuRenderCallback(MenuManager*)` 从 MenuManager render 路径触发该 callback。官方脚本 `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/resources/scripts/repentogon_extras/bestiary_menu.lua:49-117` 在此回调中加载 ANM2 并通过 Sprite 绘制菜单内容。原版本地 Wiki 的 `Sprite:Load`/`Sprite:Render` 例子见 `F:/projects_isaac/IsaacDocs/docs/Sprite.md:157-184,219-229`。

`DESIGN`：第一阶段只把菜单态屏幕 Sprite 作为连接／准备状态的可见 UI 候选：复用现存 Lua 与菜单 render owner，自有 Sprite 与回调按 Client scene generation 绑定；不创建 Game、Room、Player 或 Camera，不调用游戏更新。若需要等待期持续显示，可把状态画在菜单 callback 中。它不是世界 replica，不能据此宣称 Client 已有可玩/可见房间。

`UNKNOWN`：当前加载安装是否启用对应 hook、菜单回调在目标 J273 构建的命中与排序、Lua 与图形线程/context 是否满足资源加载和绘制要求、所有屏幕 Sprite 所需资源缓存生命周期，均未由静态源码证明为运行时事实。

## 取消与退休边界

`BINARY_STATIC`：`Game::Exit(bool)` 的 ZHL ABI 是 `__stdcall`（`Game.zhl:135`），目标 VA `0x6F6270` / RVA `0x2F6270`。在 active 分支，VA `0x6F6363` 检查 PGD `+0` / `+0xF8C` 标志，VA `0x6F639A` 可调 cloud save `0x902F00`，或 VA `0x6F63A3` 调 local save `0x903460`；这些分支先于 VA `0x6F63B5` 的 `ShouldSave` 检查。`ShouldSave=false` 走 VA `0x6F63BE` → `0x9324A0`，而 true 走 `0x6F63B7` → `0x932160`。VA `0x6F647B` 还清 Manager `+0x15`（即 PGD `+1`）read-only 标志。保存分支依赖 active/PGD flags，不能声称每次 Exit 都会写入；但 `Exit(false)` 也不能用来保证不写访客进度。

`DERIVED`：不应通过 `Game::Exit(false)` 来取消 Client 场景或建立隔离；它属于 Game/PGD 生命周期，某些条件下在 ShouldSave 分支判定前已进入云/本地保存路径，并且会改只读标志。

`DESIGN`：准备状态先留在 Client 隔离态；候选资源未发布时可直接作废候选；已发布时先停止输入与状态发布，再在主线程阻止新 render callback、等待已进入 callback 的工作退出，最后释放自有 Lua/图形引用。只有自有场景没有残留引用后才能转 `Off`。Client update 的早退必须覆盖 `Preparing`、`Displaying` 和 `Retiring`；不能先切 `Off` 再期待原生菜单/Start 行为安全接管。

`UNKNOWN`：本次没有证明 Sprite Reset、Lua nil/GC 或 callback remove 与原生／GL cache 最终释放的时序，也没有证明在途批次结束点及 Shutdown 的最后可控 owner。`docs/design/tx-04-17/tx-04-17-public-interface-catalog.md` 的 ST6/ST7 项目已把线程/context、资源引用和 lua_close 生命周期列为补证项。若进程即将走 Main/Shutdown，退休动作必须在 Game/Manager/图形 owner 被销毁前完成；Shutdown 不再保证存在下一次 Manager Update。

## 给 tx-04-16 的角色与切换反馈

`SOURCE`：当前 `mod/src/core/session.h:49` 的 `SessionMode` 仅有 `None/Solo/Host`，没有 Client role。`mod/src/game/lifecycle.cpp:186-205` 的 `NoteLoadingOwnerEntered` 只记录 Host/Warmup 首次菜单启动资格；`:252` 的 `StartGameRequested` 读取 Manager `_starting` 请求。`mod/src/ops/command.cpp:559-606` 的 `RestoreScope` 处理现有 Host Restore 观察/退休范围，没有 Client 模式。

`SOURCE`：当前 hook 清单 `mod/binding/hooks.json:18` 把 `manager_update` 的优先级设为 `-10000`。结合当前 hook 安装顺序研究，低优先级 wrapper 位于已知链外层；同优先级顺序与未知更外层 wrapper 仍不是稳定契约。

`BINARY_STATIC`：Manager::Update VA `0x92E3E0` 的 `0x92E41D` 条件分支可直接走 `0x92E45A retn 4`，它位于主 fallthrough 的 `0x92E46E call ExecuteStartGame` 之前。主 fallthrough 会执行 Start/menu 前置处理，之后的其它分支可在 `0x92E80D retn 4` 退出。不能把某个正常 early-return 当成 Client 隔离入口；外层必须先判定 Client，避免有的路径早退、有的路径已进入 Start/menu 后再返回。

`DESIGN`：新增 Client 生命周期应在 Manager 消费 `_starting` 前完成角色切换。建议最少区分 `Connecting/AwaitingScene/Preparing/Displaying/Retiring`，网络 worker 只向有界不可变队列交付数据，游戏对象工作由主线程 client adapter 消费。Client update 边界处理消息、代次/取消与退休后直接返回，不调用原生 `Manager::Update`；Client 可上行本地用户的 input intent，但不执行本地 Authority tick 或原生 input apply，也不把资源 ready 记为 world/applied ACK。Host/Solo/Off 保持各自正常的本地 Start 语义；Client 才禁止原生 Start/Restore 与本地 Authority 模拟。

`DERIVED`：因为 `ExecuteStartGame` 在 Manager 状态 switch 前被调用，若 Client 等到 `Game::Start` hook 或 Manager state 改变后才切角色，已经过晚。禁用整个原生 Manager Update 可能让菜单动态状态冻结；可保留独立菜单渲染，但菜单 Update 的完整玩法／状态副作用尚未证明安全。

`UNKNOWN`：Client prepare 的正式触发来源与跨线程队列契约、同址/同优先级 Hook 实际顺序、Client 模式与 Host loading ownership 的互斥状态字段，需 tx-04-16 在既有 Session/Hook 架构中明确。世界场景需独立限定每项构造和展示调用的写入闭包；ST3 指出的外层 Camera 路径 `0x91E5E0`、`0x91D840` 内部条件调用 `0x6EB4B0`（callsite VA `0x91E65D`、`0x91E6C1`、`0x91E25B`、`0x91E2CA`）也说明 Camera 更新不能原样保留为无 RNG 的 presentation tick。只有条件成立时才发生对应 helper 调用。

## 尚未闭合的问题清单

- `UNKNOWN`：J273 下有无可安全创建、更新与退休的独立 Replica Root；当前已证实路径不够支持房间世界。
- `UNKNOWN`：MenuManager/Sprite 的资源加载、取消、最终缓存释放与 Shutdown 顺序；菜单显示只证明静态候选，不证明目标运行时可用。
- `UNKNOWN`：Room 填充、Camera 所有权、HUD、EntityList、动画与渲染闭包的最小安全子集；Game ctor 的空 Room 不是充分条件。
- `UNKNOWN`：Client 接管时实际 hook 链、主线程队列与正在运行的原生启动请求能否原子隔离；需要在实现设计中保证在 `ExecuteStartGame` 入口前切 Client 并维持至完全退休。

这些 UNKNOWN 是实际缺口；本报告给出的菜单 Sprite 是有限展示候选，不把它包装成已经证明安全的世界展示实现。
