# REPENTOGON 目标版本点位核对（tx-04-03）

> 2026-10-25 tx-04-06 3.4/3.5 编写补充：Join/K 兼容分派和命令/界面/远端已接入，统一清单新增 5 个 ST8 持久写入入口的只读观测，当前共 33 入口、8 设备槽与 12 字节补丁；纯编译链接成功，见[编写记录](../design/tx-04-06-ops-harness.md)。未部署、安装或运行测试，下方静态阶段的“尚未实现”保留原时点。

> 2026-10-25 tx-04-06 静态补充：[ST1–ST9 与主审](../research/tx-04-06-static-confirmation.md)已完成指定静态研究。[ST2–ST9](../research/tx-04-06-static-remaining.md)确认 Join 的唯一角色参数、CALL shim 清栈来源、RemoveCoPlayer ABI 与初始化时序，并发现输入设备通知触及原版联机、UI 直接读派生尾部、退出保存 dirty PGD 的风险。兼容分派尚未实现，未运行测试。

> 2026-10-24 tx-04-05 静态补充：[T1–T8 与主审](../research/tx-04-05-static-confirmation.md)确认 Odd 普通调用点、设备 id/查询/最终采样时点、PauseScreen/失焦分支、切房 animation 参数链和字段宽度。本文及 JSON 的 tx-04-03 主表保留历史快照，后续更正单独标注；新增候选均未安装。

> 2026-10-24 tx-04-04 实现修订：IDA 原始指令确认 Lua 捕获字段为 `Game+0x680D8`（即 `Console+0x104`），类型为 MSVC `std::string*`；此前记为 `0x68158` 的资料与实现有误。`Console::Print` 捕获时直接追加后返回，不写控制台历史，继续使用 Print。静态记录见 `artifacts/repentogon/tx-04-04-revision-static/`。

日期：2026-10-24。范围：[REPENTOGON 适配重构设计](../design/repentogon-refactor.md) 第 6.0 节规定的首轮点位，包括原型 loop 32 槽、coop 19 槽、自有虚拟设备依赖、硬编码地址和字段偏移。本次全部是静态分析：用 IDA 读两个版本的映像，读 REPENTOGON 源码和它的运行日志。没有构建，没有运行游戏，也没有执行测试；没有计算或校验任何哈希。

机器可读清单：[repentogon-binding.json](repentogon-binding.json)（43 项）。脚本和中间数据：[artifacts/repentogon/tx-04-03-static](../../artifacts/repentogon/tx-04-03-static/README.md)。

## 1. 两个映像

| | 原型 | 目标 |
| --- | --- | --- |
| 游戏版本 | Repentance+ v1.9.7.17.J460 | Repentance+ v1.9.7.12.J273 |
| PE timestamp | `0x69E6E3A7` | `0x68154D8D` |
| 文件 | 游戏根目录 `isaac-ng.exe`（IDB 为当时已有的脱壳映像） | `<游戏目录>\Repentogon\isaac-ng.exe`，SteamStub 3.1，用桌面上的 Steamless 3.1.0.5 对副本脱壳 |
| 脱壳后 SizeOfImage / 入口 | `0x8FE000` / `0x6EFC46` | `0x85F000` / `0x687722` |
| REPENTOGON | — | 已安装 v1.1.3（`rgon_changelog.txt`）；本地源码为 `1.1.2-50-g421c91d08` |

几点说明：

- 游戏根目录的 exe 仍是 v1.9.7.17。REPENTOGON 安装在 `Repentogon\` 子目录，里面有自己的 v1.9.7.12 exe、`zhlLoader.dll`、`libzhl.dll` 和 `zhlREPENTOGON.dll`，并注明不要直接运行 exe。所以 `zhlIsaacCS.dll` 应放进这个子目录。
- 地址一律写成 RVA，即 IDA VA 减去 `0x400000`。
- REPENTOGON 的运行日志（`Repentogon\repentogon.log`，2026-10-24 00:40）记下了 1267 个定义的运行时地址，以及 58 个全局变量的值，当时的模块基址是 `0x00BA0000`。下文标注为 `RUNTIME_OBSERVED (REPENTOGON log)` 的条目就来自这里。

## 2. 方法

1. **原型侧特征。** 在 v1.9.7.17 上导出每个原型点位的特征：所在函数、去掉重定位和 rel32 后的开头字节、指令序列、调用与被调、字符串、`[reg+disp]` 偏移，以及 vtable 的 RTTI。
2. **函数匹配。** 在 v1.9.7.12 上，以 REPENTOGON 锚点（`IsaacMain`、`Game::Update`、`Manager::Update`、`Game::Exit` 等）和 RTTI vtable 为种子，按指令序列对齐沿调用图下推。可信的字节模式命中也作为补充。相似度用的是指令助记符序列，没有用哈希。
3. **点位映射。** 调用点、返回地址和全局变量都通过对齐后的指令映射得到；全局变量再和 REPENTOGON 日志里的值逐一核对。
4. **字段偏移。** 对齐后的指令投票得出新偏移，再对照 REPENTOGON `.zhl` 结构体的字段名和偏移。
5. **人工复核。** 输入设备、Join、控制台和回调这几部分逐一反编译确认；`CoopNoCreate` 用 IDA 的栈深度和寄存器分析确认。
6. **重叠检查。** 把 REPENTOGON 的 343 个 ASM/内联特征串扫描到 v1.9.7.12 上（命中 338 个），看是否落在我们的补丁点附近。

## 3. 主要结论

1. **函数结构基本未变，地址和部分结构有变化。** 39 个原型调用点中，37 个在 v1.9.7.12 上找到了对应点，调用目标与函数匹配结果全部一致。剩下两个：standard body anchor 在 v1.9.7.12 上不存在；时钟点找到了，但不再 Hook。
2. **Q2（与 REPENTOGON 名称的对应）：**
   - 普通 gameplay owner 是 `Game::Update`；teardown 函数是 `Game::Exit`；render owner 是 `Manager::Render`；主动道具 consumer 是 `Entity_Player::ControlActiveItem`。
   - v1.9.7.17 的 `0x5AB6D0` 只是一层包装，真正的 `Manager::Update` 是它内部调用的 `0x554CD0`。
   - **pause 判断不是 `Game::IsPaused`**，而是对 `Game::_console`（`+0x67FD4`）调用的控制台方法 `0x287020`。v1.9.7.12 的 `Game::Update` 根本不直接调用 `Game::IsPaused`。
3. **Q3（回调分派）：** 三个回调调用函数各自只发一个回调：MC_POST_UPDATE（id 1）、MC_POST_GAME_STARTED（id 15，REPENTOGON 名 `LuaEngine::PostGameStart`）、MC_POST_NEW_ROOM（id 19）。它们的调用方正好就是原型 Hook 过的那些点，所以用入口 Hook 加上下文即可，不需要按参数过滤。
4. **Q4（时钟）：** 时钟函数从 XMM0 返回 double，而 ZHL 的定义标志只有 THISCALL、CLEANUP、VOID、LONGLONG，表达不了这种返回值。首轮不 Hook 时钟。
5. **standard body anchor 不存在。** v1.9.7.17 的 `Game::Update` 在 pause 判断之后调用 `0x308630`；v1.9.7.12 在判断之后直接调用 `HUD::Update`，没有这个调用。改用 `Game::Update` 上下文加控制台判断的结果来代替。
6. **输入设备子系统变化最大**（见第 6 节）：
   - 查询槽从 12/13/14 变成 8/9/10；
   - Poll 从第 30 槽移到第 35 槽；
   - 设备注册只传裸指针，没有 control block，InputManager 会自己删除设备；
   - 基类大小从 `0x370` 变成 `0x3F8`；
   - RTTI 类名是 `DeviceKeyboard` / `DeviceGamepad`，不是 `InputDevice*`。
7. **字段偏移。** Player、Game、g_Manager 有多个字段平移，REPENTOGON 的 `.zhl` 可逐项印证（见第 7 节）。
8. **保留调用点补丁（方式 4）的点位共 4 处：** 三处 Join mode predicate 和创建点。它们都需要读调用方寄存器，创建点被拒绝时还要跳回 Join 循环。v1.9.7.12 里这几处的寄存器分工和 v1.9.7.17 不同。
9. **REPENTOGON 有两处会影响我们的执行路径：**
   - **更正 tx-04-01：** `Entity_Player::Update` 实际被 REPENTOGON 用宏 `HOOK_PRE_UPDATE_CALLBACK` Hook 了（MC_PRE_PLAYER_UPDATE，1160），回调返回真时 Player 子类的更新被跳过。
   - MC_PRE_UPDATE 的跳帧补丁打在 `Game::Update` 内部 `0x2F77A4`，跳过时直接到函数尾声 `0x2F7A39`。
10. **没有重叠。** REPENTOGON 的补丁都不在我们的字节级补丁点 ±16 字节内，也不在任何入口 Hook 函数的前 16 字节内。
11. **Q10（命令桥）：**
    - `Isaac.ExecuteCommand` 的实现是 `0x4567F0`，它调用 `Console::RunCommand(in, nullptr, nullptr)`，输出通过 `g_Game+0x680D8` 捕获。
    - 这条路径和控制台输入路径都不写任何全局标志。
    - 限制：`g_Game` 为空时，这个绑定直接返回空串，不会调用 RunCommand。

## 4. loop（原型 32 槽）

方式对应设计文档 5.1：`R` 表示 REPENTOGON 已有定义上的 ZHL Hook，`O` 表示自有定义上的 ZHL Hook，`P` 表示指针补丁，`C` 表示调用点补丁，`—` 表示不安装。

| # | 原型 site→target（v1.9.7.17） | v1.9.7.12 site→target | 名称 | 方式 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 0 | `0x531231→0x6266A0` | `0x50AE44→0x5F9690` | 时钟 | — | XMM0 返回；7 个调用方 |
| 1 | `0x5312EC→0x5AB6D0` | `0x50AF08→0x52E3E0` | `Manager::Update` | R | v1.9.7.12 由 IsaacMain 直接调用；另一调用方为 `Game::NextVictoryLap`；REPENTOGON 3 个 Hook（含 -9999） |
| 2 | `0x555231→0x2FADC0` | `0x52E911→0x2F6D30` | `Game::Update` | R | 1 个调用方；内含 MC_PRE_UPDATE 补丁 |
| 3 | `0x2FBBA5→0x4607A0` | `0x2F7A34→0x446C90` | MC_POST_UPDATE 调用函数 | O | 1 个调用方 |
| 4 | `0x559696→0x2FA0C0` | `0x532996→0x2F6270` | `Game::Exit` | R | 位于 `Manager::cleanup_current_state` |
| 5 | `0x5AB84A→0x2FA0C0` | `0x582DD8→0x2F6270` | `Game::Exit` | R | 位于 `Isaac::Shutdown`；`Game::Exit` 只有这两个调用方 |
| 6 | `0x33F5B9→0x465180` | `0x339719→0x44B930` | MC_POST_NEW_ROOM 调用函数 | O | 所在函数 `0x3385F0`；1 个调用方 |
| 7 | `0x5312F1→0x5555C0` | `0x50AF0D→0x52EC40` | `Manager::Render` | R | 另一调用方为 `ModManager::UpdateWorkshopMods`（加载路径） |
| 8 | `0x554F7D→0x61FC00` | `0x52E68B→0x5EDCC0` | InputManager 完整更新 | O | 4 个调用方，只在 `Manager::Update` 上下文内处理 |
| 9 | `0x61FC97→0x61F280` | `0x5EDD22→0x5EC900` | 设备公共更新 | O | 1 个调用方 |
| 10 | `0x554D5E→0x559720` | `0x52E46E→0x532A20` | `Manager::ExecuteStartGame` | R | REPENTOGON 未 Hook |
| 11 | `0x559B4D→0x402980` | `0x532EAA→0x3F7F60` | `Room::Update` | O | `Room::Update` 共 10 个调用方 |
| 12 | `0x559B82→0x464EE0` | `0x532EDF→0x44B660` | `LuaEngine::PostGameStart` | R | 6 个调用方 = #12、#18–22 |
| 13 | `0x2FADFA→0x28BBA0` | `0x2F6D69→0x287020` | 控制台判断（`Game::_console`） | O | 不是 `IsPaused`；3 个调用方 |
| 14 | `0x2FAE09→0x308630` | 无 | standard body anchor | — | v1.9.7.12 中不存在 |
| 15 | `0x2FBA4D→0x402980` | `0x2F78DF→0x3F7F60` | `Room::Update` | O | `Game::Update` 上下文 |
| 16 | `0x403E70→0x186C0` | `0x3F93A7→0x13F00` | `EntityList::Update` | O | 9 个调用方；内含 REPENTOGON 补丁 |
| 17 | `0x559B21→0x2F8140` | `0x532E7E→0x2F43A0` | `Game::RestoreState` | R | REPENTOGON 4 个 Hook（含 -100） |
| 18 | `0x2F580B→0x464EE0` | `0x2F1BAE→0x44B660` | 位于 `Game::Start` | R | 同 #12 |
| 19 | `0x2F61FF→0x464EE0` | `0x2F26A0→0x44B660` | 位于 `Game::NetStart` | R | 同 #12 |
| 20 | `0x2F72C9→0x464EE0` | `0x2F362C→0x44B660` | 位于 `Game::StartDailyChallenge` | R | 同 #12 |
| 21 | `0x2F7721→0x464EE0` | `0x2F3A70→0x44B660` | 位于 `Game::StartFromRerunState` | R | 同 #12 |
| 22 | `0x2F7A10→0x464EE0` | `0x2F3D63→0x44B660` | 位于 `Game::StartDebug` | R | 同 #12 |
| 23 | `0x2FB4F0→0x52F1C0` | `0x2F73F4→0x509100` | 房间生命周期 FSM | O | 1 个调用方 |
| 24 | `0x52FC77→0x402980` | `0x509B26→0x3F7F60` | `Room::Update` | O | FSM 上下文 |
| 25 | `0x2FB51C→0x4318A0` | `0x2F7420→0x426430` | `RoomTransition::Update` | R | 5 个调用方 |
| 26 | `0x2FE37C→0x402980` | `0x2FA17C→0x3F7F60` | `Room::Update` | O | 所在函数 `0x2FA0F0` 也需要一个上下文 Hook |
| Poll | Keyboard/Gamepad 第 30 槽（`+0x78`） | 第 35 槽（`+0x8C`）：Keyboard `0x73B490`→`0x633DD0`，Gamepad `0x73B3F0`→`0x6331B0` | Poll | P | Poll 之后的返回地址 `0x61F30C`→`0x5EC98C` |
| IAT×3 | Peek/Translate/Dispatch | — | 消息泵诊断 | — | 首轮去掉 |

## 5. coop（原型 19 槽）

| 项 | v1.9.7.17 | v1.9.7.12 | 方式 | 备注 |
| --- | --- | --- | --- | --- |
| 查询槽 ×6 | Keyboard/Gamepad 第 12/13/14 槽→`0x61E8D0/0x61E970/0x61EA20` | 第 8/9/10 槽（`+0x20/+0x24/+0x28`）→`0x5ECD80/0x5ECDE0/0x5ECE50` | P | 都是 `thiscall(action)`、`retn 4`，第 10 槽的浮点返回走 x87。v1.9.7.12 的第 12–15 槽是设置器 |
| Player::Update | vtable `0x76BDD0`+0x0C→`0x382AF0` | vtable `0x71BDB4` 第 3 槽→`0x37BD40` | R | REPENTOGON 用 MC_PRE_PLAYER_UPDATE 宏 Hook 了它 |
| 移动 | `0x385BC2→0x379B40` | `0x37DDFF→0x372540` | O | 函数内有 REPENTOGON 补丁 |
| 炸弹 | `0x385BC9→0x380F40` | `0x37DE06→0x37A210` | O | 函数内有 REPENTOGON 补丁 |
| 主动道具 | `0x385BD2→0x391A40` | `0x37DE0F→0x389E50` | R | `ControlActiveItem`；3 个调用方（含卡牌药丸 consumer 两处） |
| 卡牌/药丸 | `0x385BD9→0x3A3D30` | `0x37DE16→0x39BDD0` | O | |
| 射击 | `0x385BE0→0x37B020` | `0x37DE1D→0x373990` | O | |
| 丢弃 | `0x385BE7→0x3A42B0` | `0x37DE24→0x39C300` | O | 函数内有 REPENTOGON 补丁 |
| Join pass | `0x2FADA7→0x5BAC40` | `0x2F6D21→0x591360` | O | 位于 `Game::ProcessInput`；`this` = PlayerManager（`Game+0x1BAA8`）；Netplay 非空时提前返回 |
| 创建点 | `0x5BB415→0x5B9CD0` | `0x591FE6→0x590250` | C | `SpawnCoPlayer2` 有 10 个调用方。拒绝落点 `0x5BB561`→`0x5925E4`；selector 寄存器从 EBX 改为 EDI |
| 奇数帧 helper | `0x555241→0x2FD3F0` | `0x52E921→0x2F9340` | O | 返回地址 `0x555246`→`0x52E926`；其中 Player 更新的返回地址 `0x2FD60C`→`0x2F955C` |
| mode 1 | `0x5BAE7E→0x5BA980` | `0x5915AE→0x5910A0` | C | `PlayerManager::CoopBabiesOnly`；manager=EBX，key=ESI |
| mode 2 | `0x5BB081→0x5BA980` | `0x59174C→0x5910A0` | C | manager=EBX，key=ESI |
| mode 3 | `0x5BB288→0x5BA980` | `0x591E67→0x5910A0` | C | manager=EBX，key=ECX（`[edi-8]`），行指针=EDI |

**创建点拒绝路径的复核（ST5 补全解释）。** 从循环头 `0x591E53` 到创建调用 `0x591FE6`，EDI 一直没被改写。push 参数之前栈深度是 -268，和原生跳往 `0x5925E4` 的两处来源（`0x591E6E`、`0x591FA4`）相同。实际只有 `0x591FDF push eax` 一个 int32 role，工厂 `retn 4`；不能把 8 字节解释为两个工厂参数。沿用原型 E8 CALL shim 时，shim 入口 `[esp]=0x591FEB`、`[esp+4]=role`；恢复私有保存区后拒绝 `add esp,8` 清这两项再跳 `0x5925E4`，与原生放弃路径栈深度一致。若改成 E9/JMP shim，入口没有额外返回地址，只能清 role 的 4 字节。

**ST3/4/6/7 补充。** 普通 K 的设计序列为两个合格 pass（19 Triggered，再 14 Triggered），每个 pass 内查询结果一致；不是固定两个渲染帧，特殊配置可能自动确认。三处 mode E8 字节分别是 `E8 ED FA FF FF`、`E8 4F F9 FF FF`、`E8 34 F2 FF FF`，每处完整 5 字节；第三处后续继续用 ECX 作 key，shim 须保留。`RemoveCoPlayer=0x590560`，thiscall，`Player*` 与 unused bool，`retn 8`，有掉落/实体清理/重编号等副作用。工厂在 append 前按现有 Player 数写 index，Init 内写类型/角色及初始 controller，Join 在工厂返回后 `0x591FFB` 才写 K 的 controller id；exact pair 放到整个 pass 返回后确认。

## 6. 自有虚拟设备（v1.9.7.12 需要重新设计）

| 项 | v1.9.7.17（原型） | v1.9.7.12 |
| --- | --- | --- |
| RTTI 类名 | `InputDeviceKeyboard` / `InputDeviceGamepad` | `DeviceKeyboard` / `DeviceGamepad` |
| 基类 vtable | `0x782758`（38 槽，纯虚槽为 3、30） | `0x732F04`（COL `0x745F4C`，38 槽，纯虚槽为 3、35） |
| Keyboard / Gamepad vtable | `0x7A1678` / `0x782C80` | `0x73B490` / `0x73B3F0`（Gamepad 多一个第 38 槽） |
| 查询槽 | 12/13/14 | 8/9/10 |
| Poll | 第 30 槽 | 第 35 槽；返回假时 InputManager 会移除并删除该设备 |
| 基类构造函数 | `0x61E050` | `0x5EC570`（thiscall，无参数） |
| Initialize | `0x61E210` | `0x5EC6A0`（`InputDeviceBase::Initialize`，`retn 4`） |
| 注册 | `0x620DF0(manager, object, control)` | InputManager vtable `0x732E44` 第 45 槽（`+0xB4`）`0x5EEB40`，参数为 `device*` |
| 移除 | `0x620EB0` | 第 46 槽（`+0xB8`）`0x5EEBB0`；它会调用设备第 2 槽，再调第 0 槽（参数 1）删除设备 |
| 所有权 | object/control 对，6 槽 control 表 | 裸指针，存放在 InputManager `+4/+8/+0xC` 的 vector 里，没有 control block |
| 分配器 | `0x60F4C0` | 游戏自己用 `operator new` `0x686DE5`；因为只经第 0 槽释放，自有设备用哪种分配器都行 |
| 基类大小 | `0x370` | `0x3F8`（Keyboard `0x2F98`，Gamepad `0x568`） |
| 设备 id | `+0x0C` | `+0x08`（来自计数器 `0x7E65EC`） |
| 模板表 | 全局 vector `0x8799E8`，32 项 × 16 字节 | 构造函数把 `+0x10` 设为 `0x7888C0`、`+0x14` 设为 40，表项 8 字节 |
| 状态数组 | `+0xC0/+0xC4` | `+0xC8/+0xCC`；`+0x18` 是数量，`+0x1C` 是映射表 |
| 键盘状态源 | `0x878C10` | `0x7E6B08`（推断；还有 `0x7E6C68` 原始数据、`0x7E6DC8` 前一帧，各 `0x15C` 字节） |
| 自有设备覆写候选（tx-04-03 当时记录） | 0/3/12/13/14/30 | 当时为 0/3/8/9/10/35；当前修订见下方 ST1 补充 |

结论：原型的 owned device ABI（对象布局、control block、注册/移除方式）不能照搬，要在实现推进中按 v1.9.7.12 重新实现。第 0 槽必须遵守 NEVER 策略：被调用时不释放内存，只标记退休。退休可以走“Poll 返回假”或“调用 RemoveDevice”两条路，两条最终都会调到我们的第 0 槽。

**tx-04-06 ST1 补充（2026-10-25）：** [静态确认与主审](../research/tx-04-06-static-confirmation.md)确认第 2 槽会释放映射表与 current/previous，所以 NEVER 也必须覆写该槽保留有效基类状态；第 3 槽返回 `const char*` 名称，不能当整数类型码。K 当前设计覆写集合为 **0/2/3/5/6/7/8/9/10/11/26/27/35**，新增 raw/触发键/活动中性。主审另发现官方 1.1.3 `MiscFixes.cpp:268` 确实 Hook 了 `InputDeviceBase::Initialize`，读取 `+0x3FC` 名称；K 布局须预留并在 Initialize 前初始化。第 8 节旧“设备函数没有 Hook”的概括不适用于该入口，原调查遗漏在此更正。以上仅静态事实与设计修订，尚无 K 实现或运行时观察。

**ST2/9 补充。** InputManager 正好 47 个函数槽（0–46），后续项是 RTTI COL；注册会调用映射 profile 回调，主动移除/Poll=false 会调用已由 Manager 初始化安装的原版 Lobby/Netplay 断连回调。因此 K 不能无条件透传这些通知。物理确认在 `0x4AF870` 可以选中高亮 K，`0x4AFAED` 将 id 写全局 `0x788A14`；KeyConfig `0x4B8C70` 和暂停断连处理 `0x5346B0` 可直接读 `+0x48C/+0x498`，物理断连还读 `+0x3F8`。13 槽中性和 `0x400` 名称预留区均不足以证明安全。K 需通用输入通知隔离、本地 UI 列表排除和读取前分支，尚未实现；不通过进入原版联机模块修复。

## 7. 全局变量与字段

| 项 | v1.9.7.17 | v1.9.7.12 | REPENTOGON 名称 |
| --- | --- | --- | --- |
| Game 槽 | `0x871678` | `0x7E49F4` | `g_Game` |
| Manager 槽（原型叫 online） | `0x87169C` | `0x7E4A1C` | `g_Manager` |
| Lua 槽 | `0x871690` | `0x7E4A0C` | `g_LuaEngine` |
| InputManager 对象 | `0x857B18` | `0x7CE438` | `g_InputManagerBase` |
| Game run-active 字节 | `+0x2658A` | `+0x25B02` | （紧挨 `_isDebug` `+0x25B01`） |
| Game flags | `+0x26548` | `+0x25AC0` | `_gameStateFlags` |
| PlayerManager | `+0x1BAA8` | `+0x1BAA8` | `_playerManager` |
| Game 控制台 | — | `+0x67FD4` | `_console` |
| Manager Netplay vector | `+0x4B3D8/+0x4B3DC` | `+0x4AFB8/+0x4AFBC` | `_netplayManager` |
| Manager starting | `+0x4B130` | `+0x4AD10` | `_starting` |
| Player playerIndex（原型 ordinal） | `+0x161C` | `+0x1610` | `_playerIndex` |
| Player controllerIndex（原型 key） | `+0x1618` | `+0x160C` | `_controllerIndex` |
| Player 移动方向 | `+0x1624` | `+0x1618` | `_moveDirection` |
| Player 射击相关 | `+0x162C/+0x1630/+0x1634/+0x1638` | `+0x1620/+0x1624/+0x1628/(+0x162C 推断)` | |
| Player coop ghost | `+0x20A9` | `+0x207D` | `_isCoopGhost` |
| Player 骨心 | `+0x1D88` | `+0x1D74` | `_boneHearts` |
| Player 主动道具 | `+0x1580/+0x1588` | `+0x1574/+0x157C` | `_activeDesc` |
| Player 其它 | `+0x17A0`、`+0x17C0`、`+0x13BC` | `+0x1790`、`+0x17B0`、`+0x13B8` | `_actionHoldDrop`、`_damageCooldown` |
| 未变 | `0x28/0x2C/0x30`、`0x173`、`0x360`、`0x1340/0x1344/0x134C`、`0x1378`、Join 行（`+0x0C`/`+0x14`，步长 0x14）、`+0x70/+0x74` | 同左 | `_type/_variant/_subtype`、`_dead`、`_velocity`、hearts |

## 8. 与 REPENTOGON 的关系

- **共用调用链。** 我们要做入口 Hook 的这些函数，REPENTOGON 也挂了 Hook：

  | 函数 | REPENTOGON Hook 数 |
  |---|---|
  | `Console::RunCommand` | 7 |
  | `Game::RestoreState` | 4 |
  | `Manager::Update` | 3 |
  | `Game::Exit` | 3 |
  | `Manager::Render` | 2 |
  | `Game::Update` | 2 |
  | `RoomTransition::Update` | 1 |
  | `LuaEngine::PostGameStart` | 1 |
  | `ControlActiveItem` | 1 |
  | `Entity_Player::Update` | 1（宏生成） |

  tx-04-03 当时未找到入口 Hook 的项目包括 `CoopBabiesOnly`、`SpawnCoPlayer2`、`RemoveCoPlayer`、`ExecuteStartGame`、`Room::Update`、`EntityList::Update`；这不代表它们的下游没有 Hook/回调。旧“各个回调调用函数和设备函数也都没有 Hook”的概括已被 ST1/ST8 更正：官方 Hook InputDeviceBase::Initialize、Player Init/构造、PostPlayerInit、Entity Remove 及初始化 stats helper，并改断连重分配内部路径。详见静态报告，不能将原调查空结果当完整兼容证明。
- **函数内部的补丁**（不影响入口 Hook，但会改变行为）：
  - `Game::Update`：MC_PRE_UPDATE 跳帧；
  - `EntityList::Update`：3 处；
  - `Manager::Render`：光标、ImGui；
  - `Game::RestoreState`、`Console::RunCommand`、`Room::Update`：各若干；
  - 移动、炸弹、丢弃 consumer 和 Player 更新：若干。
- **字节级补丁点没有重叠。** 详见 `data/v12_overlap.json`。

## 9. 留给实现推进的事项

- **自有虚拟设备**：按第 6 节在 v1.9.7.12 上重新实现，包括对象布局、所有权和退休路径。原型里 control block 那部分在这个版本不适用。
- **Poll 返回地址检查**：改用 `0x5EC98C`。
- **门切房内层 `Room::Update` 的所在函数 `0x2FA0F0`**：需要一个上下文 Hook。它的调用方只有 `RoomTransition::ChangeRoom` 和 `Console::RunCommand`（tx-04-04 方案编写时补查，ABI 为 thiscall、`retn 8`）。
- **调用点补丁的寄存器**：mode predicate 和创建点都要按 v1.9.7.12 的寄存器分工重写。
- **控制台判断 `0x287020`**：需要建自有定义，ABI 为 `thiscall(Console*)`，返回 bool/char。
- **命令桥**：主菜单时 `g_Game` 是否为空，静态分析定不下来（`UNKNOWN`）。`cs` 命令的输出要通过 `Console::Print` 写出，才能被 `g_Game+0x680D8` 捕获。
- **射击字段（tx-04-05 补充）**：上表的 `+0x162C 推断` 是 tx-04-03 当时记录；本轮已在射击 consumer 的 `0x3794C0 mov [edi+162Ch],al` 确认一字节写入，`0x3794B0/0x3794B8` 另写 `+0x1624/+0x1628` 两个 float。业务含义仅按已追到的标志/分量描述，不外推完整武器状态。见静态报告 T7。
- **构建依赖**：REPENTOGON 本地源码（1.1.2 之后第 50 个提交）要对齐到已安装的 v1.1.3。
