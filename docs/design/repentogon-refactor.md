# REPENTOGON 适配重构设计

> 2026-10-24 tx-04-05：T1–T8 静态审查与主审、[core 移植](tx-04-05-core-port.md)、[game 主线程适配与必要 Hook/solo 命令接线](tx-04-05-game-adapter.md)已完成相应代码与纯构建；[Lua 夹具、Rust 单启动 S0–S11 与 V1–V10 归约](tx-04-05-test-harness.md)也已编写，Rust/DLL 纯构建成功。没有实机操作或测试，唯一整体 E2E 尚未执行；历史 tx-04-04 结果保留。

> 2026-10-24 tx-04-04 后续：第七次 E2E 已验证暂停状态导航可正确返回主菜单，Game::Exit 计数 0→1，两次进程正常退出。整体仍 FAIL：Lua run 回应和 UI 渲染标记缺失；启用及关闭配置两次均发现第一、三槽存档字节变化。菜单 g_Game/MC_MAIN_MENU_RENDER 中的 Lua 命令桥继续已观察，详见结果文档，不表示整轮已完成。

> 2026-10-24 tx-04-04 实现修订：IDA 原始指令确认 Lua 捕获字段为 `Game+0x680D8`（即 `Console+0x104`），类型为 MSVC `std::string*`；此前记为 `0x68158` 的资料与实现有误。`Console::Print` 捕获时直接追加后返回，不写控制台历史，继续使用 Print。静态记录见 `artifacts/repentogon/tx-04-04-revision-static/`。

日期：2026-10-23 起草（tx-04-01），2026-10-24 修订（tx-04-02）。状态：设计文档。

- tx-04-02 吸收了用户对目标版本、输入屏蔽、会话界面和测试 mod 的决定，补充了 ImGui 界面设计和点位重新核对方法，并修订了[现行规范](../refactor/project-conventions.md)第 4、7 节，新增第 14 节。
- tx-04-01、tx-04-02 只阅读资料、编写文档：没有构建，没有运行游戏，也没有执行任何测试。
- tx-04-03（2026-10-24）在 v1.9.7.12 上静态核对了首轮点位，结果见 [点位核对](../hooks/repentogon-binding.md)。本文第 2.2、2.5、3.1、5、6、7、9、14、15 节据此做了修订。核对同样没有构建、运行或测试；目前仍没有新代码。
- tx-04-04（2026-10-24）已实现并编译 mod、UI 和 i686 harness；唯一 E2E 在注入启动阶段 Launch 等待超时失败，未进入菜单、未重跑。见[实现与结果](tx-04-04-results.md)；此前“没有代码”描述保留为对应轮次的历史事实。
- tx-04-04 E2E 之后（2026-10-24）做了一次静态审阅，并据此修订了[实现方案](tx-04-04-implementation-plan.md)。修复沿用 tx-04-04 编号（D16）。本文第 1、5.4、9.3、10、13、14、15 节同步更新。同步只改了文档，没有运行任何测试。

证据标签沿用现行规范第 5 节：

- `SOURCE`：直接阅读 REPENTOGON（`D:\projects\REPENTOGON`，本地源码为 `1.1.2-50-g421c91d08`，提交日期 2026-06-17）或本仓库源码所得；
- `STATIC_INFERENCE`：静态推导，未经 IDA 或运行时复核；
- `DESIGN`：本文提出的设计决定；
- `USER_DECISION`：用户在会话中的决定；
- `UNKNOWN`：尚未知道。

## 0. 摘要

- **目标环境。** 官方 REPENTOGON，游戏降级到它的目标版本 `Repentance+ v1.9.7.12.J273`。原型基于 v1.9.7.17.J460 的全部点位都要在新版本上重新核对。
- **产物形态。** 新产物是由 REPENTOGON 加载器加载的 `zhlIsaacCS.dll`，不再需要 Helper 注入或外部控制器。网络放在 DLL 内；Rust 只保留为 E2E 编排、远端模拟客户端和故障注入中继。
- **会话界面。** 会话通过 REPENTOGON 的 ImGui 界面发起。界面由本项目自带的纯界面 Lua mod 借助 REPENTOGON 的 Lua ImGui API 绘制，用 `cs` 控制台命令与 DLL 通信。
- **Hook 生命周期。** Hook 在启动时安装后常驻，会话只做逻辑启停：未激活时原样透传，不恢复字节。
- **Hook 接入方式。** 函数入口 Hook 接入 LibZHL 调用链（`super()` 加 priority），与 REPENTOGON 共用。原型靠 rel32 调用点区分调用方，现在改为外层 owner Hook 标记的“调用方上下文”。入口 Hook 表达不了的语义才用自有字节补丁，并推迟到 REPENTOGON 打完自身补丁后再装。
- **失败不拖垮游戏。** `ModInit` 永远返回 0。扫描失败只关闭对应能力组，C/S 不可用，游戏照常运行。
- **代码复用。** 不碰游戏对象的 `core/` 模块（Session、TickClock、TickPipeline、Participant、输入 codec/queue/Resolve、协议状态机）基本照搬原型；`game/` 适配层按新 Hook 机制重写。
- **点位核对（tx-04-03）。** 原型 39 个调用点中 37 个在 v1.9.7.12 上找到了对应。输入设备子系统变化较大，自有虚拟设备要按新版本重做。另有 4 处必须保留调用点补丁，并且要按新的寄存器分工重写。详见[点位核对](../hooks/repentogon-binding.md)。
- **推进次序。** 先重新核对点位（已完成），再迁 CoopActive 主链，然后迁 world_record，之后按[状态同步方案](state-synchronization.md)推进 Replica。
- **兼容约束。** 旧约束（ISCS protocol 5 / ABI 5、七项 `Isaac.Sync` Lua API、channel 0 / 0xFD）不再延续，只保留“不触及原版联机和 Isaac Sync”的隔离原则。

## 1. 用户决定与范围

| 编号 | 问题 | 决定 | 来源 |
| --- | --- | --- | --- |
| D1 | 目标游戏版本 | 使用官方 REPENTOGON，游戏降级到 v1.9.7.12.J273；原型点位全部重新核对 | `USER_DECISION`（tx-04-02） |
| D2 | Hook 字节恢复 | 不再考虑字节恢复 | `USER_DECISION`（tx-04-01） |
| D3 | 进程模型 | 网络放进 DLL；Rust 只做测试和中继 | `USER_DECISION`（tx-04-01） |
| D4 | 兼容约束 | ISCS protocol 5 / ABI 5、七项 `Isaac.Sync` Lua API、channel 0 / 0xFD 全部不延续 | `USER_DECISION`（tx-04-01） |
| D5 | 第三方 gameplay Lua mod | 首版不考虑 | `USER_DECISION`（tx-04-01） |
| D6 | 首轮迁移范围 | 先迁 CoopActive 主链（Session、Clock、Pipeline、输入、虚拟设备、Join），world_record 放第二轮 | `USER_DECISION`（tx-04-01） |
| D7 | 房主打开 REPENTOGON 控制台、菜单或 ImGui 时 A/B 同被屏蔽 | 暂按类似暂停处理 | `USER_DECISION`（tx-04-02） |
| D8 | 会话发起方式 | 设计 ImGui 图形界面（第 9 节） | `USER_DECISION`（tx-04-02） |
| D9 | E2E 能否使用测试专用 Lua mod | 可以 | `USER_DECISION`（tx-04-02） |
| D10 | 现行规范中冲突的条款 | 修订（第 16 节） | `USER_DECISION`（tx-04-02） |
| D11 | REPENTOGON 开发包 | 本地仓库已更新；开发包取 tag `1.1.3`，与安装版本一致 | `USER_DECISION`（tx-04-04） |
| D12 | 启动器自动更新 | 已关闭 | `USER_DECISION`（tx-04-04） |
| D13 | E2E 启动方式 | harness 自行注入，复现启动器的流程 | `USER_DECISION`（tx-04-04） |
| D14 | E2E 存档 | 使用第二个存档 | `USER_DECISION`（tx-04-04） |
| D15 | 推进拆分 | tx-04-04 拆为两次推进：开发包、骨架、Hook 框架、命令桥与 harness 为一次，循环、tick 判定与 A 的输入为一次；之后的编号依次顺延（第 14 节） | `USER_DECISION`（tx-04-04） |
| D16 | tx-04-04 首次 E2E 失败后的修复 | 沿用 tx-04-04 编号。修复完成后执行 tx-04-04 的第二次 E2E，仍然只执行一次、不自动重跑；第一次 E2E 的 FAIL 和 artifact 原样保留 | `USER_DECISION`（tx-04-04） |
| D17 | 第二次 E2E 失败后的启动方式与验收 | 采用方式 A：harness 继续自行注入，给游戏进程设置启动器标记 `LAUNCHED_BY_RGONLAUNCHER=1`，只在 `Launch` 返回 0 后才恢复主线程。修正后执行 tx-04-04 的第三次 E2E，仍然只执行一次、不自动重跑；前两次的 FAIL 和 artifact 原样保留 | `USER_DECISION`（tx-04-04） |
| D18 | 第七次 E2E 之后的修正 | 测试 mod 改用 `Game():GetNumPlayers()`，并在主菜单渲染时重新显示界面窗口；第二个存档的 REPENTOGON 附属文件列入允许范围；第一、三个存档按基线对比（同一文件在关闭配置启动中也变化，则记为 REPENTOGON 自身行为）；开场动画由用户自行开启 REPENTOGON 的 `SkipIntro` 选项，harness 只读检查、未开启则不注入。修正后再执行一次 E2E，不自动重跑 | `USER_DECISION`（tx-04-04） |
| D19 | tx-04-05 方案中的四项决定 | U1：单人会话用新命令 `cs solo start/stop`，`cs host` 留给 tx-04-06；U2：测试 mod 可注册 `MC_PRE_UPDATE` 制造跳帧并读取玩家状态，只作 E2E 夹具；U3：切房由测试 mod 调用 `Game():StartRoomTransition` 触发；U4：MC_PRE_PLAYER_UPDATE 跳过的帧计为 tick、边沿不补发，本轮只记录。见 [tx-04-05 实现方案](tx-04-05-implementation-plan.md)第 13 节 | `USER_DECISION`（tx-04-05） |
| D20 | tx-04-05 第一次 E2E 之后的修正 | harness 启动等待期间的 focus 只尽力激活并记录，失败不致命，发键和截图前仍必须成功；V10 对第一、三档只放行文件末尾 Counter/Checksum 两个 u32 字段的变化，且须落在历史关闭配置启动（e2e-08）改动过的字段内，长度和其余字节不得变化；第三方纯资源 mod（REPENTOGON Download Helper）每次运行前临时禁用、运行后恢复，S0 规则不变。保留 tx-04-05 编号，修正后执行第二次 E2E，不自动重跑。见 [结果](tx-04-05-results.md) | `USER_DECISION`（tx-04-05） |
| D21 | tx-04-05 第二次 E2E 之后的修正 | harness 在 S0 结束前须观测到两个不同的正 `game_frame`（游戏已开始模拟）才开始 S1，等待期间不发键；切房触发键由 F9 改为第二次 F8（F9 是原版手动截图热键），第一次 F8 仍为跳帧；第二次运行留下的 `ManualScreenshot-20.png` 先存入 e2e-02 artifact 再从用户 Documents 删除。保留 tx-04-05 编号执行第三次 E2E，不自动重跑。见 [结果](tx-04-05-results.md) | `USER_DECISION`（tx-04-05） |
| D22 | tx-04-05 第三次 E2E 之后的修正 | S10 的原生移动改为沿 S9.move 的反方向走（玩家刚走过的路径已知可通行），不再固定按 D；只改 harness。保留 tx-04-05 编号执行第四次 E2E，不自动重跑。见 [结果](tx-04-05-results.md) | `USER_DECISION`（tx-04-05） |
| D23 | tx-04-05 验收结论 | 用户决定：不修复第四次 E2E 中 V2 的判定方向问题，也不重跑，直接将第四次 E2E（`tx-04-05-e2e-04`）算作 tx-04-05 的验收通过。harness 原报告保持 FAIL，不改写；该次 V1、V3–V10 PASS，V2 的 S10 中玩家在会话 Idle、计数不变时原生左移约 198 px，失败来自判定写死向右。`moved(&obs, "x", 1.0)` 的方向缺陷保留为已知问题，未修复。见 [结果](tx-04-05-results.md) | `USER_DECISION`（tx-04-05） |
| D24 | tx-04-06 的拆分与三项决定 | tx-04-06 只做网络、握手、设备 K、Join、B 的远端输入与断开退休；relay 故障注入、B 死亡复活、异常 Join、跨房射击、loading 查询与完整场景集移到 tx-04-07；world_record 顺延到 tx-04-08。B 的角色由房主经 `cs host <port> [character]` 指定，默认 Isaac。对端断开、B 退休后整个会话结束（回到 Idle，不回到 Hosting）。实现前的静态确认沿用 tx-04-05 做法（GPT-6 Luna / max 子智能体逆向，主智能体独立审阅）。见 [tx-04-06 实现方案](tx-04-06-implementation-plan.md) | `USER_DECISION`（tx-04-06） |
| D25 | tx-04-06 方案中的四项决定 | U1：配置项 `host.bind_address` 默认 `0.0.0.0`，E2E 部署配置写 `127.0.0.1`；U2：对端超时默认 3000 ms（`host.peer_timeout_ms`）；U3：界面首版不提供角色选择，只经 `cs host` 参数指定；U4：主菜单 Hosting 由测试 mod 执行 `cs host` 发起，同时覆盖 Lua 命令桥。用户要求暂不开始正式工程任务（静态确认与实现）。见 [tx-04-06 实现方案](tx-04-06-implementation-plan.md)第 15 节 | `USER_DECISION`（tx-04-06） |
| D26 | tx-04-06 第一次 E2E 之后的修正 | K 的 vtable 表头按原型 `BuildTables` 自建 MSVC RTTI：类型 `.?AVRemoteInputDevice@IsaacCs@@`，层次为自身 + 设备基类 COL（`0x745F4C`，`InputDeviceBase`，单继承）的 `bases[0]`，基类 COL 与静态核对不符时失败关闭、不注册 K；harness 的崩溃扫描加入存档根的 `crash_dumps`，新增转储时记录 `failure_cause=game_crash`，某轮远端日志或 `counters.json` 缺失时只让相应 V 项不通过，不再中止归约；第一次运行的崩溃转储存入 e2e-01 artifact 后从用户 Documents 删除。保留 tx-04-06 编号执行第二次 E2E，不自动重跑。见 [结果](tx-04-06-results.md) | `USER_DECISION`（tx-04-06） |
| D27 | 存档判定排除每日备份 | harness 的存档判定不看每日备份 `documents/save_backups/` 中的任何存档：这些文件不进入白名单判定，也不进入 D18/D20 基线判定，不出现在 `save-changes.json`；快照仍原样复制，作为原始证据。其他规则不变，第一、三档的云端/本地主存档与附属文件仍按 D20 判定。适用于此后所有使用该 harness 的推进；已生成的报告不改写。见 [tx-04-06 结果](tx-04-06-results.md) | `USER_DECISION`（tx-04-06） |
| D28 | tx-04-06 验收结论 | 用户决定：不再执行第三次 E2E，直接将第二次 E2E（`tx-04-06-e2e-02`）算作 tx-04-06 的验收通过。harness 原报告保持 FAIL，不改写。该次 S0–S11 全部执行，V1–V11 由 harness 判为 PASS；V12 只有存档项失败，失败的是第一、三档当日备份：它们只改了 D20 规定的 Counter/Checksum 字段，因为文件名带日期而找不到历史基线；D27 起每日备份已不参与判定。D27 的新规则尚未经过实际运行。见 [tx-04-06 结果](tx-04-06-results.md) | `USER_DECISION`（tx-04-06） |

本文覆盖：
- 目标架构、启动与 Hook 机制；
- 原型点位的迁移策略与重新核对方法；
- 线程与会话、网络、ImGui 界面与控制命令；
- 失败处理、工程布局、E2E 验收方式和后续推进次序。

本文不覆盖：
- 各点位在 v1.9.7.12 上的核对结果（tx-04-03）；
- Replica 的实现细节（见状态同步方案）；
- 任何代码。

## 2. 事实依据

### 2.1 REPENTOGON 的加载与 Hook 安装（`SOURCE`）

1. **定义在所有 mod 之前解析。** 代理 DLL 加载 `libzhl.dll` 并调用 `InitZHL`。`InitZHL` 先对全部 `.zhl` 生成的定义执行 `FunctionDefinition::Init()`：只缺一个时弹框并 `ExitProcess(0)`，缺多个时弹框并返回错误（`libzhl/dllmain.cpp:110-135`）。`.zhl` 生成的 `IsaacRepentance.cpp` 编译进 libzhl（`CMakeLists.txt:67-77`），所以这些地址在任何 mod 的 `ModInit` 之前就已解析好。
2. **mod 加载顺序不确定。** `HookMain` 修改 `IsaacMain`，让它调用 `LoadMods`。`LoadMods` 用 `FindFirstFileA("zhl*.dll")` 枚举并 `LoadLibraryA`，对每个 DLL 调用 `ModInit()`；REPENTOGON 自身则调用 `InitRepentogon(poisonAddr, poisonValue)`（`loader/loader.cpp:125-190`）。枚举顺序由文件系统决定，没有保证。
3. **初始化出错直接 abort。** 任一 `ModInit` 返回非 0，或者没找到 REPENTOGON，循环结束后都会 `abort()`（`loader/loader.cpp:175-186`）。
4. **Hook 一次装完，之后注册的不会生效。** 全部 `ModInit` 执行完后，`LoadMods` 末尾调用一次 `ZHL::Init()`（`loader/loader.cpp:190`）。它由 `static bool initialized` 保护，只执行一次：按 priority 安装全部已注册的 FunctionHook，任何一个失败就弹框并 `ExitProcess(1)`（`libzhl/HookSystem.cpp:22-31, 413-420`）。之后再注册的 Hook 永远不会被安装。
5. **定义缺失即安装失败；priority 决定内外层。** `FunctionHook_private::Install` 用 `Definition::Find(name)` 查找定义，找不到就失败（`HookSystem.cpp:558-566`）。Hook 列表是 `std::multimap<int, …, std::greater<int>>`（`HookSystem.cpp:397`）：数值大的先安装，处在内层；数值小的后安装，处在外层，最先被调用。
6. **导出接口：**
   - `FunctionHook::Add(name, type, hook, outInternalSuper, priority)` 是公开的静态函数，可以在运行时调用（`include/HookSystem.h:22`，实现在 `HookSystem.cpp:1038-1041`）。
   - `FunctionDefinition` 有两个构造函数，一个接收特征串，一个直接接收地址（`HookSystem.cpp:234-264`）。用地址构造的定义，`Load()` 永远成功（`HookSystem.cpp:288-314`）。
   - `Definition::Add` 遇到重复的非空 internalName 会抛异常（`HookSystem.cpp:167-188`）。
   - `SigScan`、`ASMPatcher`、`ASMPatch` 都以 `LIBZHL_API` 导出（`libzhl/SigScan.h:39`、`libzhl/ASMPatcher.hpp:26,141`）。
   - `HOOK_*` 宏在命名空间作用域定义静态 `FunctionHook` 对象，也就是在 `LoadLibraryA` 的静态初始化阶段无条件注册（`include/HookSystem.h:45-130`）。
7. **调用 ModInit 时，REPENTOGON 的补丁不一定已打上。** `InitRepentogon` 依次执行 `PerformASMPatches()`、着色器初始化、再次 `Definition::Init()`（返回值被忽略），最后恢复 poison 字节（`repentogon/dllmain.cpp:198-235`）。由于加载顺序不确定，我们的 `ModInit` 执行时，REPENTOGON 的 ASM patch 可能已经打上，也可能还没有。
8. **只改内存，不能卸载。** REPENTOGON 只修改进程内存，不改磁盘上的 exe，也没有卸载 Hook 的接口。
9. **可参照的第三方 zhl mod 写法。** 同仓库的 `zhlDelirium` 就是这种 mod：导出 `ModInit`，链接 libzhl，用 `/MD` 和 `/EHs` 编译（`CMakeLists.txt:170-180`，`delirium/Delirium.cpp:7`）。REPENTOGON 各目标都用 `/MD`（`CMakeLists.txt:80,165`）。

### 2.2 与我们相关的 REPENTOGON 已有 Hook（`SOURCE`）

| 函数 | REPENTOGON 中的位置 | 影响 |
| --- | --- | --- |
| `Game::Update` | `Patches/ASMPatches/ASMCallbacks.cpp:28`、`Patches/ASMPatches/ASMGridEntityCollision.cpp:28`（`HookTest.cpp:37` 是测试文件） | 每帧开头重置 `s_callbackPause` |
| `Game::IsPaused` | `ASMCallbacks.cpp:34`：`return s_callbackPause \|\| super();` | MC_PRE_UPDATE 返回 true 时本帧更新被跳过。所以 `Game::Update` 被调用并不等于模拟了一个 tick |
| `Manager::Update`（`__stdcall`，参数 `bool`） | `LuaInterfaces/LuaIsaac.cpp:478`、`Patches/Quickstart.cpp:22`、`SaveStateManagement/EntitySaveStateManagement.cpp:3403`（priority -9999） | 顶层更新已有多层 Hook，其中 -9999 那个要求处在最外层 |
| `InputManager::IsActionTriggered` / `IsActionPressed` | `LuaInterfaces/Menus/LuaMenuManager.cpp:191,199`、`Patches/ModsMenuStuff.cpp:130` | 菜单或 ImGui 打开时屏蔽动作。这一层在设备层之上（D7） |
| `Console::RunCommand` | `Patches/ConsoleHooks.cpp:163` 等多处 | REPENTOGON 用 ASM patch 让控制台命令在主菜单也能运行，并在 `ConsoleHooks.cpp:163-200` 中对不在 run 内时不能用的命令给出错误 |
| `Entity::Init/Remove`、`Entity_NPC::Morph`、`Entity_Pickup::Morph`、`Game::Spawn`、`Room::SpawnGridEntity`、`GridEntity::Init`、`Entity_Player::constructor/Init`、`PlayerManager::Update`、`Game::Exit` | 多处 | world_record 要与之共用调用链 |
| `Entity_Player::Update` | `LuaInterfaces/CustomCallbacks.cpp:523-534`，宏 `HOOK_PRE_UPDATE_CALLBACK(Entity_Player, …, 1160)` | MC_PRE_PLAYER_UPDATE 返回真时，跳过 Player 子类的更新，只执行 `Entity::Update`，各 consumer 都不会执行。（tx-04-01 只按字面搜索，只看到 `HookTest.cpp:60` 的注释代码，误判为没有 Hook，tx-04-03 更正） |
| 原版 Online 菜单 | `Patches/ASMPatches/ASMMenu.cpp:35`：REPENTOGON 主动禁止联机 | 和原版联机的隔离天然成立；依赖原版联机的 Isaac Sync 在 REPENTOGON 下无法运行 |

### 2.3 `.zhl` 已覆盖与未覆盖的部分（`SOURCE`）

已有定义：
- Game：`Game::Update/IsPaused/Render/Spawn/Exit`；
- Manager：`Manager::Update/Render`（`Manager.zhl:5,58`）；
- `Entity` 和 `Entity_Player` 的 `__vtable`，含 `Init/Update/Remove/ClearReferences`（`Entity.zhl:254-291`、`EntityPlayer.zhl:661-672`）；
- Player：`Entity_Player::constructor/ControlActiveItem`，`PlayerManager::SpawnCoPlayer/SpawnCoPlayer2/Update`；
- 输入：`InputManager::IsActionTriggered/IsButtonTriggered/IsActionPressed/IsButtonPressed/GetActionValue/GetDeviceType/GetDeviceNameByIdx`（`Input.zhl`）；
- 房间与实体：`Room::SpawnGridEntity/MakeDoor`，`GridEntity::Init`，`Entity_NPC::Morph`，`Entity_Pickup::Morph`；
- 控制台：`Console::RunCommand`（`Console.zhl:5`）。

未覆盖：
- 更新函数：`Room::Update`、`EntityList::Update`；
- 输入设备：Keyboard、Gamepad 等设备类及其 vtable（`InputManager` 在 `.zhl` 里是空结构体），InputManager 的完整更新、设备注册与移除；
- Join：Join 的内部函数和三处 mode predicate；
- Player：移动、炸弹、射击、卡牌药丸、丢弃这几个 consumer，以及奇数帧的 Player-only helper；
- 循环与房间：alternate/loading owner 及其状态应用，房间生命周期 FSM 和门切房 FSM；
- 回调与退出：MC_POST_UPDATE 尾部和 alternate transaction tail 所调用的回调分派函数，run teardown 函数（是否就是 `Game::Exit` 尚未确认）。

### 2.4 原型现状（`SOURCE`，本仓库）

- **进程组成。** Rust 控制器（`IsaacCsT0`、`IsaacCsT0T09`）加 x86 Helper 注入加 C++20 Native DLL，三者通过版本化的共享内存 IPC 通信。T09 的 UDP endpoint 和 relay 在 Rust 侧；Native 侧的 `AuthorityWireLink` 只读写 wire IPC。
- **补丁事务。** 补丁统一由 `hooks.cpp` 的事务处理：RelativeCallPatch、PointerPatch、EntryPatch 三类，安装时挂起线程，按顺序安装，失败回滚，结束时恢复。
- **补丁规模。** loop 基础 32 槽（27 个 rel32、2 个 Poll vtable、3 个 USER32 IAT），加 coop 19 槽，共 51 槽。可选的 world_record 再加 19 个入口补丁，共 70 槽（[Hook 总入口](../hooks/README.md)）。
- **装配中心** 是 `coop_active_runtime.cpp`：`Authority()`、`TopExit()`、`Consume()`、`Evaluate()`、`Submit()`、`RetireB()`、`LifecycleCallback()` 等。tick 边界由 `CurrentTickBoundary(owner, return_address)` 判定，它用 rel32 调用点的返回地址来区分调用方。
- **不碰游戏对象的模块：** `authority_session`、`authority_tick`、`authority_tick_pipeline`、`input_core`、`coop_input`（Participant、ShootingDirections 等）、`minimal_protocol`（ICT8 v2）、`protocol_transport`、`virtual_device_lifecycle`。
- **已采纳的语义** 见现行规范第 6、9、11、12 节，包括：A/B 统一输入；Authority-pre 发布 current，正常返回后提交 previous；缺输入中性；启动例外；射击后按者优先；跨房射击暂停；异常 Join；NEVER 回收。

### 2.5 版本（D1）

- **目标。** 官方 REPENTOGON。本地源码的 README 写的目标是 v1.9.7.12.J273（`README.md:10`）。用户会把游戏降级到这个版本。
- **原型点位。** 全部基于 `Repentance+ v1.9.7.17.J460`（PE timestamp `0x69E6E3A7`），在新版本上只作索引：RVA、原始字节、对象偏移、vtable 槽位和 COL 都要重新核对（方法见 6.0）。
- **好处。** REPENTOGON 的 `.zhl` 特征串和结构体本来就以 v1.9.7.12 为目标，可以直接作为核对锚点。
- **tx-04-03 已确认：**
  - REPENTOGON 装在游戏目录的 `Repentogon\` 子目录，里面有一份单独的 v1.9.7.12.J273 exe（PE timestamp `0x68154D8D`），以及 `zhlLoader.dll`、`libzhl.dll`、`zhlREPENTOGON.dll`；游戏根目录的 exe 仍是 v1.9.7.17。
  - 已安装的发行版是 v1.1.3。
  - 这份 exe 带 SteamStub 3.1 壳，用 Steamless 对副本脱壳后才能静态分析。

### 2.6 REPENTOGON 的 ImGui（`SOURCE`）

- **ImGui 不对外导出。** `imgui` 是静态库，只链接进 `zhlREPENTOGON.dll`（`CMakeLists.txt:150, 215`）。上下文在 `RunImGui` 中创建和绘制（`repentogon/ImGuiFeatures/ImGui.cpp:513-665`）。`RunImGui` 由 ASM patch 插在 `Manager::Render` 调用 `SwapBuffers` 之前（`ImGui.cpp:678-688` 的说明）。REPENTOGON 只导出 `InitRepentogon` 和 `__REPENTOGON_VERSION`（`repentogon/dllmain.cpp:198`、`repentogon/Version.cpp:4`），没有任何 ImGui 的 C++ 接口。
- **Lua ImGui API。** REPENTOGON 给 Lua 提供 ImGui 接口（`ImGuiFeatures/LuaImGui.cpp:32-1041`），包括：
  - 结构：`CreateMenu`、`CreateWindow`、`AddElement`、`AddTabBar`、`AddTab`；
  - 控件：`AddButton`、`AddText`、`AddInputText`、`AddInputInteger`、`AddCheckbox`、`AddCombobox`、`AddProgressBar`；
  - 回调与更新：`AddCallback`、`UpdateText`、`UpdateData`；
  - 其他：`PushNotification`、`SetVisible`、`SetTooltip` 等。
- **回调时机。** 元素的 Render 回调和点击类回调都在 ImGui 绘制过程中执行（`ImGuiFeatures/CustomImGui.h:658-676`），也就是在主线程的 render 阶段（`STATIC_INFERENCE`：由 `RunImGui` 的调用位置推出）。
- **REPENTOGON 自己的做法。** 它自带的脚本用“ImGui 回调 → `Isaac.ExecuteCommand`”执行控制台命令（`resources/scripts/repentogon_extras/onlinestub.lua:289-290`）。`Isaac.ExecuteCommand` 是否经过 `Console::RunCommand` 属于 `STATIC_INFERENCE`，REPENTOGON 的控制台扩展依赖这一点，tx-04-03 中确认。

## 3. 目标架构

### 3.1 部署形态（`DESIGN`）

```text
<游戏目录>/Repentogon/                          REPENTOGON 的运行目录（tx-04-03 观察）
  isaac-ng.exe（v1.9.7.12）、zhlLoader.dll、libzhl.dll、zhlREPENTOGON.dll …   REPENTOGON（不改动）
  zhlIsaacCS.dll                                 本项目 DLL（LoadMods 在当前目录按 zhl*.dll 枚举）
  isaac-cs/config.json                           配置，默认 enabled=false
  isaac-cs/logs/、isaac-cs/traces/               日志与 E2E trace
<mods 目录>/isaac-cs-ui/                          本项目自带的纯界面 Lua mod（metadata.xml、main.lua）
```

- 玩家只需放入 DLL、`isaac-cs/` 目录和界面 mod。没有 Helper，也没有外部控制器。
- 配置为 `enabled=false`、配置缺失或能力解析失败时，DLL 不注册任何 Hook，游戏按 REPENTOGON 原样运行。界面 mod 此时只显示不可用的原因。

### 3.2 模块分层

```text
zhlIsaacCS.dll
├─ bootstrap/  ModInit：读配置 → 解析绑定 → 判定能力组 → 注册 Hook；永远返回 0
├─ binding/    与版本相关的全部内容：自有特征串与锚点规则、对象偏移、vtable 槽号、
│              自有 FunctionDefinition（地址构造）、对 REPENTOGON 定义的引用
├─ hooks/      Hook 清单与安装：ZHL Hook（REPENTOGON 定义 / 自有定义），
│              延后安装的指针补丁与字节补丁；全部常驻，逻辑启停
├─ game/       主线程适配：循环阶段与调用方上下文、tick 观测、输入采样与注入、
│              虚拟设备、Join、Player consumer；第二轮加入世界采集
├─ core/       不碰游戏对象：Session、TickClock、TickPipeline、Participant、
│              输入 codec / queue / Resolve、协议状态机
├─ net/        Winsock UDP 工作线程：收发、解析、校验、有界队列；不碰游戏对象
└─ ops/        配置、日志、trace；唯一的命令处理器（控制台、界面、测试控制口共用）
isaac-cs-ui（Lua mod，DLL 之外）
               用 REPENTOGON Lua ImGui API 绘制界面，只通过 `cs` 命令与 DLL 交互
```

依赖方向：
- 主链：`bootstrap → hooks → game → core`。
- 网络：`net → core`，只经有界队列。
- `binding` 只被 `bootstrap`、`hooks`、`game` 引用。
- `core` 和 `net` 不包含任何 REPENTOGON 或游戏头文件，可以单独编译。
- 界面 mod 不直接读写游戏状态或 DLL 内存，只发命令、读命令输出。

Rust 端（新目录 `tools/harness/`）负责：
- E2E 编排：准备游戏目录、启动游戏、驱动场景；
- 远端模拟客户端：按脚本发送输入；
- UDP 中继与故障注入；
- 汇总 artifact 并生成报告。

正常游玩不需要 Rust 端。

### 3.3 线程规则

- **主线程** 是唯一可以读写游戏对象、调用原生函数、触碰 Lua 的线程。每个 Hook 先判断当前线程：不是主线程就原样透传并计数，不改动任何状态。
- **网络线程** 只有一个，负责 socket 收发、解析和校验，把结果放进有界 SPSC 队列；主线程在固定阶段（Authority-pre 之前）取出。主线程要发送的内容同样经有界队列交给网络线程。
- **测试控制口**（只在配置开启时存在）的收发也放在网络线程。收到的命令行同样只进入主线程命令队列。
- **日志线程**（可选）：主线程只写有界环形缓冲，后台线程落盘；缓冲溢出时计数，不阻塞。
- **线程生命周期。** `DllMain` 里不创建线程，也不等待线程。网络线程在第一次需要时创建，会话结束时显式停止。进程退出时不在 `DllMain` 里 join 线程（加载器锁）。

## 4. 启动与能力解析

### 4.1 时序（`DESIGN`）

```text
LoadLibraryA(zhlIsaacCS.dll)    静态初始化：不放 HOOK_* 宏对象，什么都不做
ModInit()                       读配置 → 解析绑定 → 判定能力组 → 构造自有 FunctionDefinition（地址）
                                → 只为通过的能力组调用 FunctionHook::Add → return 0
其他 zhl*.dll 的 ModInit / InitRepentogon（与我们的先后顺序不定）
ZHL::Init()                     一次性安装全部 ZHL Hook（包括我们的）
首次主线程 Manager::Update        延后安装：指针补丁与字节补丁（安装前核对期望字节）
界面或命令发起会话                逻辑激活：创建会话对象，打开输入与发布
```

### 4.2 ModInit 规则

- **永远返回 0。** 整个函数包在异常捕获里，任何异常都只记日志，并把状态置为 `Disabled`。
- **不写代码字节，不改指针表。** 这时 REPENTOGON 的 ASM patch 可能还没执行，我们改过的字节可能让它的扫描失败。
- **不碰游戏。** 不读写游戏对象，不调用原生函数，因为游戏尚未初始化。
- **不用 `HOOK_*` 宏。** 宏在静态初始化时无条件注册；如果对应定义缺失，`ZHL::Init` 会直接 `ExitProcess(1)`。改用自有宏 `CS_HOOK_METHOD` / `CS_HOOK_STATIC`：生成的 wrapper 结构与 `HOOK_*` 相同，但注册调用只放进本 DLL 的待注册表，由 ModInit 按能力组逐个调用 `FunctionHook::Add`。挂在 REPENTOGON 已有定义上的 Hook 也走这条路，这样配置关闭时注册数为零。
- **自有定义的构造方式。** 一律用地址构造函数，地址由 `binding/` 在 ModInit 中解析；不用特征串构造函数，以免进入 REPENTOGON 的 `Definition::Init` 缺失列表。名称加 `IsaacCS::` 前缀，internalName 传空串，避免与 REPENTOGON 或其他 mod 冲突。
- **ArgData 的来源。** 自有定义的 ArgData 与调用约定标志按已理解的 ABI 手写；首轮约二十个，数量可控。能否复用 REPENTOGON 的 zhlparser 生成这些描述，留待实现时决定（`UNKNOWN`）。

### 4.3 能力组

| 组 | 内容 | 依赖 | 失败时 |
| --- | --- | --- | --- |
| `loop` | 顶层更新与渲染 owner、普通 gameplay owner、pause 判断、alternate/loading owner、teardown、Room/EntityList 更新、回调尾部、NewRoom、两个 FSM owner | — | 本进程不提供 C/S |
| `input` | InputManager 完整更新、Keyboard/Gamepad 查询与 Poll | `loop` | 本进程不提供 C/S |
| `device` | 自有虚拟设备的原生依赖。v1.9.7.12 上包括：基类构造函数 `0x5EC570`、Initialize（基类第 1 槽）、InputManager 第 45/46 槽的注册与移除、基类与 Keyboard/Gamepad vtable，以及 COL | `input` | 本进程不提供 C/S（首版没有 B 就没有会话） |
| `player` | Player::Update、六个 consumer、奇数帧 helper | `loop`、`input` | 本进程不提供 C/S |
| `join` | Join pass、普通 Player 创建、三处 mode predicate | `device`、`player` | 本进程不提供 C/S |
| `control` | `Console::RunCommand`（REPENTOGON 已有定义） | — | 界面和控制台都无法发起会话，只剩测试控制口；对玩家而言等于不可用 |
| `world`（第二轮） | world_record 的 19 个入口 | `loop` | 只关闭采集，不影响会话 |

`net` 只依赖 Winsock，不需要游戏绑定，因此不在表中。

- **组内全有或全无。** 只要有一个点位没解析出来，整组都不注册。
- **判定结果可查。** 能力组判定在 ModInit 内完成，结果写入日志，并通过 `cs status` 提供给界面。

### 4.4 自有地址解析（`binding/`）

每个自有点位登记以下内容：名称、所属能力组、解析方式、约束、期望 ABI、v1.9.7.12 的核对结果，以及对应的原型 v1.9.7.17 RVA（只作索引）。

解析方式有两种：

1. **自有特征串。** 必须在 `.text` 中唯一命中，并避开 REPENTOGON ASM patch 会改写的区域（以 `libzhl/functions/ASM.zhl` 和 `repentogon/Patches/ASMPatches.cpp:166` 起的 `PerformASMPatches` 为准）。这样无论 ModInit 在 REPENTOGON 之前还是之后执行，看到的字节都一样。
2. **锚点规则。** 从 REPENTOGON 已解析的定义出发，例如“`Game::Update` 内第 N 条 `E8` 的目标”，再核对目标入口的若干指令。适合 REPENTOGON 已定义函数的直接调用方或被调方。

核对的只是功能所需的指令形状和 ABI（现行规范第 1 节允许），不计算或校验任何哈希。对象偏移、vtable 槽号和 RTTI 名称也放在 `binding/` 中，尽量取自 REPENTOGON 生成的 `IsaacRepentance.h` 结构体，不足的部分自行登记。

## 5. Hook 机制

### 5.1 四种安装方式及选择顺序（`DESIGN`）

1. **ZHL Hook，挂在 REPENTOGON 已有定义上。** 首选。通过 `super()` 调用原函数，与 REPENTOGON 共用调用链。
2. **ZHL Hook，挂在自有定义上。** 用于函数入口可以 Hook、但 `.zhl` 没有定义的情况，例如 `Room::Update`、`EntityList::Update`、Join 和各个 consumer。
3. **自有指针补丁。** 用于 vtable 槽。Keyboard 和 Gamepad 的查询槽共用基类实现，改单个类的槽位可以保持“只影响这两类设备”的语义。REPENTOGON 不 Hook 这些设备函数，所以不存在冲突。在 v1.9.7.12 上，查询槽是 8/9/10，Poll 是 35（原型是 12/13/14 和 30），见[点位核对](../hooks/repentogon-binding.md)第 5、6 节。
4. **自有字节补丁或 ASMPatch。** 只有当“入口 Hook + 调用方上下文”仍表达不了语义时才使用，例如必须区分同一函数里的两处调用，而外层 owner 又分不出来。使用 REPENTOGON 导出的 `ASMPatcher`，在 4.1 的延后阶段安装，安装前核对期望字节。如果与 REPENTOGON 的 ASM patch 重叠，或者字节与预期不符，就关闭所属能力组。

### 5.2 常驻与逻辑启停（D2）

- 所有 Hook 从 `ZHL::Init`（或延后安装点）起常驻到进程结束，不恢复字节。
- 每个 Hook 第一步检查 `Armed`：会话已激活、在主线程上、当前 generation 有效，三者同时满足。未 armed 时，只调用一次 `super()` 或原函数，原样返回结果，不读写任何游戏对象。
- 观测类 Hook 保证原调用恰好一次。唯一的主动抑制是跨房射击时暂停 Shooting consumer，沿用现行规范第 9 节 tx-01-54 的明确例外，并且只在 armed 时生效。
- 停止会话的步骤是：关闭输入与发布，处理在途回调，退休 B 和当前 generation，然后回到透传。虚拟设备与 DLL 按 NEVER 策略常驻，这一点和规范第 9 节一致。

### 5.3 优先级

- 默认 priority 为 0。
- **需要看到 REPENTOGON 合成结果的 Hook 放外层（负值）。** 原先举的例子是 `Game::IsPaused`，但 tx-04-03 核对后发现 v1.9.7.12 的 `Game::Update` 并不调用它：MC_PRE_UPDATE 的跳过是在函数内部直接跳到尾声，`IsPaused` 只影响其它调用者。所以 tick 判定不依赖 `IsPaused`（见 7.3）。
- **上下文标记 Hook 放最外层。** 例如顶层 `Manager::Update` 要放在 REPENTOGON 的 -9999 之外（如 -10000），使 REPENTOGON 外层 Hook 内发生的原生调用也落在我们的上下文里。
- **`Console::RunCommand` 放外层。** 这样 `cs` 命令由我们独占处理，不进入其他处理者。
- 具体数值在点位核对时逐个确定，写进 Hook 清单。

### 5.4 调用方上下文：替代 rel32 返回地址

原型靠调用点的返回地址判断“谁调用了 `Room::Update`”。入口 Hook 中间隔着 ZHL 的 thunk，拿不到可靠的原调用点，所以改为下面的做法：

- **上下文栈。** 主线程维护一个小的上下文栈。外层 owner Hook 进入时压入自己的种类和 token，退出时弹出。外层 owner 包括：普通 gameplay owner、alternate/loading owner、房间生命周期 FSM、门切房 FSM、奇数帧 helper、render owner、Join pass。
- **内层读栈顶。** 内层 Hook 读取栈顶来判定种类。内层包括 `Room::Update`、`EntityList::Update`、`Entity_Player::Update`、各个 consumer、回调分派和 Join mode predicate。
- **保守默认值。** 栈为空或种类未知时：不算 Authority tick，输入保持中性，并记录原因。这与现行规范“未知绑定下保持中性”一致。
- **异常与重入。** 重入，或异常越过 Hook 边界时，由 RAII 弹栈，并让当前 token 作废。
  - wrapper 对原函数抛出的异常保持中立：不声明 `noexcept`，异常照常交给上层；只有我们自己的代码保证不抛出。
  - 依据（tx-04-04 审阅）：REPENTOGON 自己会在游戏调用链中抛 C++ 异常，见 `repentogon/SaveSyncing/SaveSyncing.cpp:204`。

原型中 4 处 `Room::Update` 调用点、5 处 alternate completion、2 处 teardown 调用点，都改由各自的外层 owner 标记。若某个外层函数本身被多种语义共用，再退回方式 4。

### 5.5 Hook 清单

代码中维护唯一一份清单（同时生成文档），它就是“单一协调 owner”，任何 Hook 只能经它注册。每一项包含：
- `id`、所属能力组、安装方式；
- 绑定对象：REPENTOGON 定义名、自有定义名或槽位；
- priority 和语义；
- 未 armed 时的行为、armed 时的行为；
- 对应的原型项（family、index、RVA）；
- 证据等级。

## 6. 原型点位迁移映射（tx-04-03 已核对）

### 6.0 方法与产物

按原计划（锚点 → 逐项核对 → 偏移 → 冲突检查 → 定方案），在 v1.9.7.12 上完成了静态核对：

- 结论文档：[点位核对](../hooks/repentogon-binding.md)；
- 机器可读清单：[repentogon-binding.json](../hooks/repentogon-binding.json)，共 43 项；
- 脚本与中间数据：[artifacts/repentogon/tx-04-03-static](../../artifacts/repentogon/tx-04-03-static/README.md)。

过程中只用了 IDA 静态分析，以及 REPENTOGON 的源码和运行日志；没有计算或校验哈希。`binding/` 的实现以 JSON 清单为准，原型 v1.9.7.17 的文档不改写。

### 6.1 安装方式分布

| 方式 | 点位 |
| --- | --- |
| ZHL Hook，REPENTOGON 已有定义 | `Manager::Update`、`Game::Update`、`Game::Exit`、`Manager::Render`、`Manager::ExecuteStartGame`、`LuaEngine::PostGameStart`、`Game::RestoreState`、`RoomTransition::Update`、`Entity_Player::Update`、`Entity_Player::ControlActiveItem` |
| ZHL Hook，自有地址定义 | MC_POST_UPDATE 调用函数、MC_POST_NEW_ROOM 调用函数、InputManager 完整更新、设备公共更新、控制台判断 `0x287020`、`Room::Update`、`EntityList::Update`、房间生命周期 FSM、门切房内层所在函数 `0x2FA0F0`、移动/炸弹/卡牌药丸/射击/丢弃 consumer、Join pass、奇数帧 helper |
| 指针补丁 | Keyboard/Gamepad vtable 的查询槽 8/9/10 和 Poll 槽 35 |
| 调用点补丁 | 三处 `PlayerManager::CoopBabiesOnly` 调用、`SpawnCoPlayer2` 创建点（拒绝时跳到 `0x5925E4`） |
| 不安装 | 主循环时钟（XMM0 返回）、三个 USER32 IAT |
| v1.9.7.12 不存在 | standard body anchor（原型 `0x2FAE09→0x308630`） |

### 6.2 对设计有影响的差异

- **REPENTOGON 名称对应（Q2）。**
  - 原型的 pause 判断不是 `Game::IsPaused`，而是 `Game::_console`（`+0x67FD4`）上的控制台方法，返回真时跳过整个 `Game::Update` 主体。
  - 原型的顶层包装 `0x5AB6D0` 在 v1.9.7.12 里没有对应；`IsaacMain` 直接调用 `Manager::Update`。
- **调用点补丁的寄存器。** 4 处调用点补丁读取的调用方寄存器在 v1.9.7.12 上不同：manager 在 EBX，key 在 ESI 或 ECX，行指针在 EDI。原型的汇编 thunk 要重写。
- **回调调用函数（Q3）。** 三个都是单一回调专用的，没有参数需要过滤，而且调用方集合和原型 Hook 的调用点完全一致。
- **输入设备子系统。** 查询槽、Poll 槽、对象大小、注册/移除方式和所有权都变了，原型的 owned device ABI 不能照搬（见 7.4）。
- **字段偏移。** Player、Game、g_Manager 有多个字段平移，已用 REPENTOGON `.zhl` 的字段名逐项印证。
- **与 REPENTOGON 补丁不重叠。** 343 个 REPENTOGON 补丁特征串中，没有一个落在我们的字节级补丁点或入口 Hook 函数的开头附近。

## 7. 会话与运行时

### 7.1 会话状态

```text
Disabled（配置关闭或能力不全，永久）
Idle（Hook 透传）→ Hosting（等对端握手）→ Warmup（已握手，等 run 与 B 绑定）
→ Active → Retiring → Idle
```

- `Idle` 和 `Hosting` 承接原型中由 Rust 控制器负责的部分；`Warmup`、`Active`、`Retiring` 沿用 `AuthoritySession` 的状态与原因码。
- REPENTOGON 让控制台命令在主菜单也能运行，所以可以在主菜单进入 `Hosting`，进入 run 后再转 `Warmup`。
- run teardown 或回到菜单时，退休 session 和 generation（规范第 6 节）。
- 对端断开时，B 输出中性，然后直接退休 B。late join 和 reconnect 是后续能力。

### 7.2 主线程每帧的顺序（Authority）

1. 进入顶层更新 owner：先执行命令队列中的会话命令，再从网络队列取出远端输入，Submit 到统一队列。
2. InputManager 更新：物理设备只做采样。
3. `Game::Update` 先调用控制台判断 `0x287020`（`Game::_console`），返回真就跳过整个主体；REPENTOGON 的 MC_PRE_UPDATE 也可能在函数中段直接跳到尾声。两种情况下本帧都不执行普通 `Room::Update`。
4. 在首个正常的 Main Player 更新之前执行 Authority-pre：发布 current，A/B 使用同一个 Resolve。
5. Player::Update 和各 consumer 读取 Resolve 的结果；射击暂停按 tx-01-54 处理。
6. 原函数正常返回后，提交 previous。
7. 标准尾部完成后，在 TopExit 发布 completed，生成 Ack 和观测，放入发送队列，并更新状态快照。
8. render owner 不推进 tick。ImGui 界面在这一阶段绘制，只读状态快照。

奇数帧 helper、FSM 和 alternate 路径按规范第 9 节 tx-01-50 处理。以上都是已采纳的语义，本设计只改变“如何检测这些阶段”。

### 7.3 tick 判定

既不把 `Game::Update` 的进入当成 tick，也不按 `Manager::Update` 的次数计 tick，也不以 `Game::IsPaused` 作为本轮判据。Authority 有两种已采纳的边界：

- **StandardRoom。** 只有普通 `Game` owner 内的 `Room::Update` 实际执行并正常返回，才开始/提交该种 tick；Lifecycle、Door、Start 等非普通路径的 Room 更新不计。控制台阻断、PauseScreen 阻断和 MC_PRE_UPDATE 跳帧不会进入这个普通调用点。
- **OddPlayer。** 奇数帧 helper 中 `0x2F9559` 的普通 Player 虚调用可以开始 tick，helper 正常返回后提交；`0x2F93F9`、`0x2F949C` 两处特殊更新不能开始。T1 确认须在普通调用点建立标记，不能只凭 `Odd` owner 或 ZHL wrapper 的返回地址分类。

按 D19/U4，MC_PRE_PLAYER_UPDATE 跳过 Player 子类更新时，所在的普通 Room tick 仍计数，其 edge 不补发、不重放。原先将它列在“自然不计 tick”之后的文字有歧义，此处明确修正。Player Hook 的进入、原生 body 到达和 consumer 查询分别记录，不能从 consumer 缺席推断 Lua 返回真。具体状态机与一次 E2E 见 tx-04-05 实现方案。

### 7.4 输入、虚拟设备、Join

语义全部沿用现行规范第 6、9 节，不重新设计。差异如下：

- **远端输入来源。** 改由 DLL 内的网络线程送入。原型的路径是 Rust → wire IPC → Native。
- **REPENTOGON 屏蔽输入（D7，规范第 14 节）。** 房主打开 REPENTOGON 控制台、菜单或 ImGui（包括本项目的联机界面）时，REPENTOGON 在 `InputManager::IsAction*` 层屏蔽动作，A 和 B 同时受影响。首版按类似暂停处理：
  - 屏蔽期间的输入不生效；
  - 我们不补偿、不改写 REPENTOGON 的屏蔽；
  - 事后不补发、不重放边沿；
  - Authority 照常按 7.3 判定 tick。

  tx-04-05/T4 修订：菜单 mask 和搜索 Hook 可在 `super()` 前短路，内层查询可能根本未执行，无法保证同次调用的设备值与最终值差值。首版记录现有控制台原调用的结果区间，并分别记录 pause/loading 原因，不额外补调原生查询。ImGui 浮层通过设置 Console state=2 诱发暂停；该来源与主菜单 mask 分开说明。用户采纳的屏蔽行为不变。
- **自有虚拟设备要按 v1.9.7.12 重做**（[点位核对](../hooks/repentogon-binding.md)第 6 节）：
  - 基类大小 `0x3F8`，构造函数 `0x5EC570`，Initialize 是第 1 槽；
  - 注册和移除用 InputManager vtable 的第 45/46 槽，只传设备裸指针，没有 control block；
  - 覆写第 0/3/8/9/10/35 槽。InputManager 移除设备时会调用第 0 槽删除它，所以第 0 槽必须遵守 NEVER 策略：不释放，只标记退休；
  - Poll（第 35 槽）返回假会让 InputManager 移除该设备，可以作为退休路径之一。
  NEVER 策略、只采样不写快照、退休时输出中性这些原则不变；规范第 9 节里 control block 6 槽的部分在这个版本不适用。
- **Join 的调用点补丁。** 三处 `CoopBabiesOnly` 和创建点的汇编 thunk，要按 v1.9.7.12 的寄存器分工重写：manager 在 EBX，key 在 ESI 或 ECX，行指针在 EDI。创建被拒绝时的落点是 `0x5925E4`。

## 8. 网络（D3、D4）

- **新协议。** 从 ICT8 v2 的 Hello、Input、Ack、观测语义出发，换用新的 magic 和版本号，不承担 ISCS 兼容。沿用以下规则：
  - 显式小端；
  - 严格检查长度、kind、flags 和 reserved 字段；
  - 用 CRC32 检查包是否损坏（这不是哈希校验，也不是认证；与[状态同步方案](state-synchronization.md)第 13 节一致）。
- **lane。** 首轮只有 Input、Control（握手与会话）和 Ack/观测。Recovery、State、Presentation 三条 lane 在状态同步阶段按该方案第 13 节加入。
- **角色。** Host 是房主游戏内的 DLL（Authority）。首轮的 Remote 是 Rust 测试客户端，按脚本发送输入。DLL 内的 Client 角色（采样本地输入、显示 Replica）随状态同步阶段加入，届时界面增加“加入”页。
- **连接。** 直连 UDP（IP 加端口）。NAT 穿透、Steam 网络和大厅不在首轮范围。E2E 时由 Rust relay 插在中间，注入丢包、乱序和延迟。
- **安全。** 没有认证（与原型相同），只适用于可信网络，并在支持声明和界面中写明。

## 9. 会话界面、控制命令与可观测性（D8）

### 9.1 方案选择

REPENTOGON 的 ImGui 上下文在 `zhlREPENTOGON.dll` 内部，没有 C++ 导出（2.6），所以我们的 DLL 不能直接往里加窗口。比较过的三条路线：

| 路线 | 做法 | 结论 |
| --- | --- | --- |
| A | 纯界面 Lua mod 调用 REPENTOGON 的 Lua ImGui API 绘制界面，经 `Isaac.ExecuteCommand("cs …")` 与 DLL 通信 | **采用。** 与 REPENTOGON 风格、开关和输入处理一致；这也是 REPENTOGON 自带脚本已在用的路径 |
| B | DLL 内另编一份 Dear ImGui，自建上下文，在 `SwapBuffers` 前自行绘制 | 不采用。同一窗口和 GL 上下文里会有两个 ImGui：要和 REPENTOGON 的 WndProc 子类化链接；鼠标、键盘捕获互相冲突；字体和样式各一套；还要额外找绘制时机 |
| C | 按特征定位 REPENTOGON 内部的 `GImGui`，借用它的上下文 | 不采用。依赖 REPENTOGON 的具体构建，发行版一更新就可能失效 |

### 9.2 界面布局

- **菜单入口。** REPENTOGON ImGui 菜单栏中的“Isaac CS”菜单（`ImGui.CreateMenu`），其中的“联机面板”打开窗口（`ImGui.CreateWindow`）。
- **状态区**（始终显示）：
  - 会话状态：`Disabled`、`Idle`、`Hosting`、`Warmup`、`Active` 或 `Retiring`；
  - DLL 版本与能力组结果；
  - 对端地址、往返时延和丢包计数；
  - 当前 Authority tick、B 的绑定状态；
  - 最近一次错误。
- **“主机”页：**
  - 监听端口（整数输入，默认取配置）；
  - 【开始主机】、【停止】两个按钮，可用性随状态变化，例如不在 `Idle` 时禁用【开始主机】；
  - “无认证，仅限可信网络”的提示。
- **“诊断”折叠区：** Hook 透传与异常计数、队列丢弃计数、trace 文件路径。
- **状态变化通知。** 用 `ImGui.PushNotification` 提示，例如对端已连接、会话结束及其原因。
- **`Disabled` 时** 窗口只显示原因（配置关闭，或哪个能力组的哪些点位失败），不显示操作按钮。
- **“加入”页** 首轮不显示，Client 角色出现后再加。

### 9.3 命令桥

- **命令集合。** 命令前缀暂定 `cs`，实现时检查不与原版和 REPENTOGON 的命令冲突。
  - `cs version`：返回 DLL 版本和命令协议版本；
  - `cs status`：每行一个 `key=value`，首行为 `status_version=1`；
  - `cs host <port>`；
  - `cs stop`。
- **处理位置。** 我们在 `Console::RunCommand` 上挂外层 Hook，独占处理以 `cs` 开头的命令，不调用 `super()`；其他命令原样传下去。REPENTOGON 自己在这个函数上有 7 个 Hook，所以我们的 priority 要排在它们之外。
- **输出方式（tx-04-04 审阅修订）。**
  - `Isaac.ExecuteCommand` 调用 RunCommand 时 `out` 为空，输出靠 `g_Game+0x680D8` 指向的字符串捕获（tx-04-03）。所以 `out` 为空时，处理器用 `Console::Print` 写出整段响应。
  - `out` 不为空时只写入 `out`，不再 Print。这与 REPENTOGON 自己处理命令的写法一致（`repentogon/Patches/ConsoleHooks.cpp:306-309`）。
  - 原写法是“`out` 不为空时也写入 `out`”，会让自行显示 `out` 的调用方看到两份输出。
  - 捕获时 Print 是否仍写入控制台历史，见[实现方案](tx-04-04-implementation-plan.md) 4.7。
- **处理器不直接改状态。** ImGui 回调发生在 render 阶段，所以处理器只校验参数，把命令放进主线程命令队列，在下一帧顶层更新 owner 入口执行（7.2 第 1 步）。`cs status` 只读取上一帧末尾更新的状态快照。
- **单一实现。** 游戏内控制台手输、界面 mod 和测试控制口共用这一个处理器和同一套命令文本。
- **界面 mod 的职责。**
  - 用 Render 回调每 60 帧（约 1 秒）调用一次 `cs status` 并解析。REPENTOGON 只在浮层打开或窗口被固定时渲染窗口（`repentogon/ImGuiFeatures/CustomImGui.h:571`），所以浮层关闭时不会轮询。tx-04-04 审阅修订：原为“每隔若干帧（例如 15 帧）”，而每次查询都经 `Console::Print`，有刷屏的风险；
  - 按钮回调只调用 `cs host` 或 `cs stop`；
  - 启动时调用 `cs version`，DLL 不存在或版本不兼容时只显示说明；
  - 不注册任何 gameplay 回调（规范第 14 节）。
- **Q10（tx-04-03 已核对）：**
  - `Isaac.ExecuteCommand` 的实现在 `0x4567F0`，在 `LuaEngine::RegisterClasses` 中登记。它只设置输出捕获指针，然后调用 `Console::RunCommand(in, nullptr, nullptr)`。
  - 控制台输入路径（`0x282240`）只写栈上的局部变量。
  - 因此 `cs` 命令在不调用原函数体的前提下，不会设置任何全局或 Game 标志（`STATIC_CONFIRMED`）。
- **限制（Q14）。** 这个绑定在 `g_Game` 为空时直接返回空串，根本不会调用 RunCommand。主菜单时 `g_Game` 是否为空，静态分析定不下来。如果为空，界面在主菜单就无法发起会话。可选的处理办法：
  1. 首版只在进入 run 之后允许发起会话；
  2. 给 `0x4567F0` 建自有定义并加入口 Hook，对 `cs` 命令绕过这项检查。
  
  到 tx-04-04 E2E 实际观察后再定。

  静态旁证（tx-04-04 审阅）：
  - REPENTOGON 的窗口过程每处理一条消息，都会读 `g_Game->GetConsole()->_state`（`repentogon/ImGuiFeatures/ImGui.cpp:306`、`331`）。
  - 官方启动的游戏能在菜单里正常运行，所以推断：至少在这个窗口过程装上之后，主菜单时 `g_Game` 不为空。
  - 这只是推断，仍以 E2E 观察为准。E2E 的测试 mod 改在 `MC_MAIN_MENU_RENDER` 中观察（[实现方案](tx-04-04-implementation-plan.md)第 6 节）。

### 9.4 配置

`isaac-cs/config.json` 由 ModInit 读取，读取失败即进入 `Disabled`。字段包括：
- `enabled`；
- 默认监听端口；
- 输入参数（lead、max_input_age 等沿用原型）；
- 日志等级、trace 目录；
- 测试控制口开关与端口。

界面中修改的端口只对当次会话生效，首轮不回写配置。

### 9.5 测试控制口

- 只在配置开启时存在，是本机 loopback TCP，不监听其他地址。
- 每行一条命令，与 9.3 共用同一个处理器，返回同样的文本。
- 供 Rust harness 启停会话和轮询状态。

### 9.6 日志与 trace

普通日志之外，另写一份 JSONL trace，记录以下内容，作为 E2E artifact 的主要来源：
- 能力组判定、Hook 清单，以及透传和异常的计数；
- 命令的来源与结果；
- 会话状态迁移；
- tick 边界与拒绝原因；
- 输入的 Submit 与 Resolve；
- 屏蔽区间；
- 收发包摘要。

## 10. 隔离与副作用（D4、D5）

- **原版联机。** REPENTOGON 已禁用。我们也不引用 Online Manager、`NetInputDevice` 或 `FrameStore`；规范第 4 节的隔离原则保留。
- **Isaac Sync。** 在 REPENTOGON 下无法运行。不再提供共存保证，也不再保持它的 API。
- **Lua mod。**
  - 首版不支持第三方 gameplay mod。支持声明写为“只有 REPENTOGON 本体和本项目界面 mod”。
  - trace 会记录实际启用的 mod 列表，但不做拦截。
  - 界面 mod 只调用 ImGui 接口和 `Isaac.ExecuteCommand("cs …")`。
  - 测试专用 Lua mod 只在 E2E 中使用（D9）。
- **持久副作用。** 不造成未经任务允许的 save、achievement、unlock 或统计等持久副作用（规范第 7 节最后一条）。
  - B 走原生 Join 时涉及的路径沿用原型已有的理解，但要在 v1.9.7.12 上重新核对；
  - 控制台命令路径的影响见 Q10；
  - REPENTOGON 自身的存档行为不受我们控制，在支持声明中写明。tx-04-04 审阅补充了以下静态事实和观察：
    - REPENTOGON 使用自己的存档文件，并与原版存档自动同步（`repentogon/SaveSyncing/SaveSyncing.cpp`）。开着 Steam Cloud 时，槽 N 写在 Steam 云存储里：`rgon_steam_persistentgamedataN.dat`、`rep+persistentgamedataN.dat`、`rep+gamestateN.dat`。不开云时，写在存档目录的 `Repentogon\rgon_persistentgamedataN.dat` 等文件里（52-71 行）。
    - 本机是云模式。官方启动器那次运行期间，三个槽位的云端文件修改时间都落在运行期间（直接观察，归因 `UNKNOWN`）。
    - 因此 E2E 判断副作用时，要同时快照存档目录和 Steam 云存储目录，并按每次启动分开记录（[实现方案](tx-04-04-implementation-plan.md)第 7 节第 2 步）。

## 11. 失败处理

| 情况 | 处理 |
| --- | --- |
| 配置缺失或非法 | 进入 `Disabled`，不注册任何 Hook，游戏原样运行 |
| 某个能力组解析失败 | 该组及依赖它的组都不注册；首轮核心组失败则进入 `Disabled`；记录未命中的点位，并在界面上显示 |
| ModInit 内发生异常 | 捕获并记录，进入 `Disabled`，返回 0 |
| 延后安装时字节与预期不符，或与 REPENTOGON 补丁重叠 | 不写入该补丁，所属能力组在本进程内关闭；已注册的 ZHL Hook 保持透传 |
| 非主线程进入 Hook | 透传并计数 |
| 上下文未知、重入或异常越过边界 | 当前 token 作废，本帧不算 tick，输入中性 |
| 命令参数非法，或在不允许的状态下发出 | 返回 `error=<原因>`，状态不变 |
| 网络错误或对端超时 | B 输出中性，退休 B；Authority 不等待 |
| 队列已满 | 丢弃并计数（输入带冗余），不阻塞主线程 |
| 绑定冲突、未知 ABI | 停止相关修改并退休会话（规范第 7 节） |

## 12. 工程布局与构建

```text
mod/                 C++ DLL（新建）
  CMakeLists.txt     MSVC x86、/MD、/EHs；链接 REPENTOGON 的 libzhl.lib，包含其 include/
                     和生成的 IsaacRepentance.h
  src/bootstrap/ src/binding/ src/hooks/ src/game/ src/core/ src/net/ src/ops/
ui/isaac-cs-ui/      纯界面 Lua mod：metadata.xml、main.lua
tools/harness/       Rust：E2E 编排、远端模拟、relay、报告；从 cs-t0 的 t09 endpoint/relay 迁移
cs-t0/               冻结，只作历史资料和代码来源
```

- **REPENTOGON 依赖。** 构建所用的源码（`.zhl`、头文件、导入库）固定到用户安装的官方发行版的同一 tag。本地源码目前是 `1.1.2-50-g421c91d08`，已安装的发行版是 v1.1.3（tx-04-03 观察），构建前要对齐到 v1.1.3。运行时安装的 `zhlREPENTOGON.dll` 导出 `__REPENTOGON_VERSION`，我们读取它写入日志和 trace，只作记录，不作为启用条件。
- **工具链与 CRT。** 与 REPENTOGON 保持一致（`/MD`，参照 `zhlDelirium` 的构建配置），因为 Hook 签名里会出现 `std_string` 这类跨模块对象。具体的 MSVC 工具集版本还要确认（Q8）。
- **从原型搬什么。** `core/`、`net/` 从原型复制后按新命名整理，保留原有的不变式和注释。sha256、mode gate、offline attestation、raw apply、boundary、passive 等历史研究模块不带回。

## 13. 测试与验收

- 每次推进只在目标完成后执行一次统一的 E2E（规范第 2 节），不在实现后补写单元测试。
- **E2E 入口** 是 Rust harness 的一条命令，依次完成：
  1. 准备游戏目录（REPENTOGON、本 DLL、界面 mod、测试 Lua mod、配置）；
  2. 启动游戏，并用原型的物理输入脚本方式进入 run；
  3. 开启会话；
  4. 远端模拟客户端按场景发送输入，relay 注入故障；
  5. 收集 trace 和包记录，生成报告。

  开启会话有两条路径：一是测试控制口；二是测试 Lua mod 调用 `Isaac.ExecuteCommand("cs host …")`，后者覆盖 Lua 到命令桥的路径。界面按钮本身的点击可以用物理输入脚本覆盖，作为可选场景。
- **artifact 内容：**
  - 游戏、REPENTOGON（`__REPENTOGON_VERSION`）和 DLL 的版本与构建信息；
  - 能力组判定结果和 Hook 清单；
  - 全部 trace 和双方的原始包；
  - 存档目录和 Steam 云存储目录的前后快照与变化列表，按每次启动分开；只复制，不恢复（第 10 节）；
  - 各场景的期望与实际对照；
  - 失败项与未覆盖项。

  不计算任何哈希。
- **首轮场景。**（D24：由 tx-04-06、tx-04-07 两次推进分别覆盖，见第 14 节。）沿用原型已有 E2E 的覆盖面：启动输入、普通移动与射击、跨房射击、射击优先、B 死亡与复活、异常 Join、loading 查询、对端断开与回到透传。另外新增：
  - 命令桥（控制台、Lua、测试控制口三种来源）；
  - 主菜单进入 `Hosting`；
  - 房主打开 REPENTOGON 菜单或控制台时的屏蔽行为（D7）；
  - 由测试 Lua mod 触发 MC_PRE_UPDATE 跳帧时的 tick 判定（D9）。

## 14. 推进计划

| 编号 | 目标 | 验收 | 状态 |
| --- | --- | --- | --- |
| tx-04-01 | 设计文档初稿 | 一次静态文档审阅 | 完成 |
| tx-04-02 | 吸收用户决定；ImGui 界面与命令桥设计；重新核对方法；修订现行规范与入口文档 | 一次静态文档审阅 | 完成 |
| tx-04-03 | 在 v1.9.7.12 上静态核对首轮点位、虚拟设备依赖、硬编码地址、字段偏移和命令路径（Q10）；产出 binding 清单 | 一次静态文档审阅 | 完成，见[点位核对](../hooks/repentogon-binding.md) |
| tx-04-04 | REPENTOGON 1.1.3 开发包；工程骨架；bootstrap、binding、Hook 框架（首轮 Hook 全部注册，只透传和计数）；命令桥、测试控制口和界面 mod；harness 自行注入和使用第二个存档。实现方案见 [tx-04-04-implementation-plan.md](tx-04-04-implementation-plan.md) | 一次 E2E：加载、Hook、命令桥（包括 Q14）、命令队列、界面、关闭配置、副作用。D16：首次失败后，修复完成再执行一次 | 实现与编译完成；第一次 E2E 启动超时 FAIL，未重跑，见[结果](tx-04-04-results.md)。静态审阅修订已实现并编译（D16）；第二次 E2E 仍启动超时 FAIL，诊断捕获 Use The Launcher! 错误框，未再重跑。根因静态确认为 REPENTOGON 的启动器环境变量检查，按 D17 补上标记后执行第三次 E2E。D18 修正后第八次 E2E E1–E7 全部 PASS（`artifacts/repentogon/tx-04-04-e2e-08/`），tx-04-04 完成 |
| tx-04-05 | 循环阶段与 tick 判定；本地 A 经统一队列输入；生命周期；会话真实状态（单人）。实现方案见 [tx-04-05-implementation-plan.md](tx-04-05-implementation-plan.md) | 一次 E2E：A 的输入；tick、暂停、控制台屏蔽、加载、跳帧、teardown 的判定 | U1–U4 已确认（D19）；T1–T8 静态审查与主审完成；全路径最终位置次序仍 UNKNOWN，采用后续稳定边界重绑；core/game、必要 Hook/solo、Lua 夹具与 Rust 单启动 S0–S11 / V1–V10 均已编写并完成相应纯构建；第一次整体 E2E 在 S0 启动阶段因 harness focus 时序 FAIL，未进入 run，未重跑，另发现 V10 历史基线规则无法匹配；按 D20 修正后第二次 E2E 跑完 S0–S11，V1、V3–V9 PASS，V2、V10 因 harness 时序与 F9 截图热键 FAIL；按 D21 修正后第三次 E2E 中 V1、V3–V10 PASS，V2 因 S10 移动被随机房间的岩石挡住 FAIL；按 D22 修正后第四次 E2E 中 V1、V3–V10 PASS，V2 因判定方向未随 S10 反向同步 FAIL，未重跑。D23：用户决定将第四次 E2E 算作验收通过，**tx-04-05 完成（用户接受）**，见[结果](tx-04-05-results.md) |
| tx-04-06 | net（DLL 内 UDP 线程与新协议）；按 v1.9.7.12 重做的虚拟设备 K；Join（含调用点补丁）；B 的远端输入；断开与退休；Rust 远端模拟客户端（D24）。实现方案见 [tx-04-06-implementation-plan.md](tx-04-06-implementation-plan.md) | 一次 E2E：主菜单 Hosting、握手、Join、B 输入、A/B 同 tick、炸弹、浮层、切房、stop/超时/teardown 三种结束 | 方案已起草，U1–U4 已确认（D25）；按用户要求暂不开始静态确认 ST1–ST9 与实现 |
| tx-04-07 | relay 与故障注入；B 死亡与复活；异常 Join；B 的跨房射击；loading 查询；补齐首轮完整场景集（D24） | 一次 E2E：首轮完整场景集 | |
| tx-04-08 | 迁移 world_record（D24 起由 tx-04-07 顺延） | 一次 E2E | |
| 之后 | 按状态同步方案第 17 节的次序，包括 Client 角色与界面“加入”页 | 每次各一次 E2E | |

## 15. 未决问题

| 编号 | 问题 | 当前处理 |
| --- | --- | --- |
| Q1 | 目标版本 | 已决定（D1） |
| Q2 | 原型函数与 REPENTOGON 名称的对应 | 已核对（tx-04-03）：`Game::Update`、`Game::Exit`、`Manager::Render`、`ControlActiveItem` 都对上了；`0x554CD0` 是 `Manager::Update`，`0x5AB6D0` 只是一层包装；pause 判断不是 `Game::IsPaused`，而是控制台方法 `0x287020` |
| Q3 | 回调分派是否需要方式 4 | 已核对：不需要。三个都是单一回调专用的调用函数，调用方集合和原型一致 |
| Q4 | 主循环时钟的 XMM0 返回 | 已核对：ZHL 表达不了；首轮不 Hook |
| Q5 | REPENTOGON 屏蔽输入 | 已决定：类似暂停（D7） |
| Q6 | 会话发起方式 | 已决定：ImGui 界面（D8，第 9 节） |
| Q7 | 测试专用 Lua mod | 已决定：可以（D9） |
| Q8 | REPENTOGON 开发包构建所用的 MSVC 工具集版本 | tx-04-04 已确认本机构建使用 VS2022 v143、MSVC 19.44.35217、Win32、/MD；不据此推断安装 DLL 的编译器版本 |
| Q9 | ida-mcp 连接 | 已恢复（tx-04-03） |
| Q10 | `Isaac.ExecuteCommand` 的路径与标志 | 已核对：经过 `Console::RunCommand`，不设置任何标志（9.3） |
| Q11 | REPENTOGON 发行版版本 | 已安装 v1.1.3；构建要对齐（第 12 节） |
| Q12 | 能否直接静态分析 | 不能，要先脱壳：exe 带 SteamStub 3.1，tx-04-03 用 Steamless 对副本脱壳 |
| Q13 | 屏蔽区间在 trace 中的识别方式 | tx-04-05/T4 静态确认菜单 Hook 会短路，采用控制台原调用的返回区间并分开记录其他阻断原因；不补调查询，不把内层未执行记成设备返回假，见 7.4 |
| Q14 | 主菜单时 `g_Game` 是否为空（这决定界面能否在主菜单发起会话） | `RUNTIME_OBSERVED`（tx-04-04 第六次）：主菜单 status 的 g_game=1，MC_MAIN_MENU_RENDER 中 lua-menu 返回 ok=1/echo=lua-menu；仅覆盖该目标环境与样本，整轮 E2E 仍 FAIL，见结果文档。第八次 E2E（整轮 PASS）再次观察到主菜单 `g_game=1`，`MC_MAIN_MENU_RENDER` 中 `lua-menu` 返回 `ok=1`：主菜单时 `g_Game` 不为空，界面可以在主菜单经命令桥发起会话（本环境样本） |
| Q15 | 门切房内层 `Room::Update` 的所在函数 `0x2FA0F0` 有哪些调用方，上下文怎么标 | 已解决：调用方只有 `RoomTransition::ChangeRoom` 和 `Console::RunCommand`（[tx-04-04 实现方案](tx-04-04-implementation-plan.md) 4.3） |
| Q16 | 射击字段 `+0x1638` 对应 v1.9.7.12 的 `+0x162C` | tx-04-05/T7 已由原始指令确认：`0x3794C0 mov [edi+162Ch],al`，是一字节射击标志；`+0x1624/+0x1628` 是另行写入的 float 分量。不能把 `+0x162C` 当成 float 方向或蓄力值；上层语义只按已追到的写入链描述 |

## 16. 现行规范的修订（tx-04-02 已应用）

D2、D4 以及 tx-04-02 的决定已经写入[现行规范](../refactor/project-conventions.md)：

- **文首。** 增加 tx-04-02 修订说明。
- **第 4 节第 3 条。** 不再延续 ISCS protocol 5 / ABI 5、七项 `Isaac.Sync` Lua API、channel 0 / 0xFD 的兼容约束；C/S 使用独立协议，不进入、不触及原版 Online 与 Isaac Sync。
- **第 7 节第 2 条。** Hook 只经统一 Hook 清单注册，启动时常驻，会话逻辑启停，不做字节恢复。
- **第 7 节第 4 条。** 正常停止时关闭输入和 publication、处理在途回调、退休对象，然后回到透传，不恢复目标字节。
- **新增第 14 节“已采纳的 REPENTOGON 适配决定”**，内容包括：
  - 官方 REPENTOGON 与 v1.9.7.12.J273，原型点位只作索引、须重新核对；
  - zhl DLL、网络在 DLL 内、Rust 只作测试与中继；
  - Hook 常驻，第 9 节“51 槽事务”等表述改作历史记录；
  - 兼容约束不延续；
  - Lua mod 边界（gameplay mod、界面 mod、测试 mod）；
  - REPENTOGON 屏蔽输入按类似暂停处理。

同步修改的入口：
- `AGENTS.md`：“Hook 恢复”改为“Hook 常驻与逻辑启停”，并加入本设计入口；
- 根目录 `README.md`、`docs/README.md`、`docs/refactor/README.md`：加入本设计入口；
- [状态同步方案](state-synchronization.md)与 [Hook 总入口](../hooks/README.md)：各加一段 tx-04-02 补充说明，原文保留。

Codex 技能 `isaac-cs-refactor`（`~/.codex/skills/`）没有包含上述冲突条款，冲突以现行规范为准，所以没有修改。

## 17. 资料

- **REPENTOGON 源码**（`D:\projects\REPENTOGON`）：`loader/loader.cpp`、`libzhl/dllmain.cpp`、`libzhl/HookSystem.cpp`、`include/HookSystem.h`、`repentogon/dllmain.cpp`、`repentogon/Patches/ASMPatches/`、`repentogon/Patches/ConsoleHooks.cpp`、`repentogon/ImGuiFeatures/`、`repentogon/resources/scripts/repentogon_extras/onlinestub.lua`、`libzhl/functions/*.zhl`、`delirium/`。
- **本仓库：** [现行规范](../refactor/project-conventions.md)、[Hook 总入口](../hooks/README.md)、[安装声明索引](../hooks/installed-hook-inventory.json)、[状态同步方案](state-synchronization.md)、[T0 代码入口](../../cs-t0/README.md)。
- **讨论来源：** 2026-10-23 的两份 Codex 会话记录（REPENTOGON 流程研究、原型设计梳理），以及 2026-10-23、10-24 两次会话中的用户决定。
