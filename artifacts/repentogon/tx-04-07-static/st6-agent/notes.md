# tx-04-07 ST6 静态逆向记录（子智能体）

目标仅为 ST6 三问：角色 7（Azazel）进入 PlayerManager 角色表时是否经过解锁判定；`Player::Init` 对角色 7 是否有专用分支或附属实体；Door owner 在切房时直接虚调 `Player::Update` 的路径上，ordinal 1 的 Shooting consumer 是否会被暂停。

## 范围、方法与证据口径

- `STATIC_CONFIRMED` 依据当前 IDA MCP 中的 `isaac-ng-v1.9.7.12.exe.unpacked.exe` 指令、交叉引用和只读内存读取；imagebase 为 `0x400000`，文中 RVA=`VA-0x400000`。没有改动 IDB 名称、注释、类型、字节或分析状态。
- 原始 IDA 导出：[ida-evidence.json](ida-evidence.json)；可重复的只读导出脚本：[ida-readonly-export.py](ida-readonly-export.py)。脚本只调用 IDA getter/枚举 API，输出范围仅含本报告点位。
- 已阅读现行规范 `docs/refactor/project-conventions.md`、`docs/design/tx-04-07-implementation-plan.md` 第 7、10、13.3 节及 R4、`docs/research/tx-04-06-static-remaining.md`、`docs/research/tx-04-05-static-confirmation.md`，并核对本地官方 REPENTOGON 1.1.3 源码及 `mod/src/game/` 当前射击上下文实现。
- 本轮未启动游戏、构建、测试、调试或校验哈希；不设 gate。有限函数/字段筛选不等于任意角色、配置、回调或实体的完整调用闭包。

## 1. 角色 7 是否受解锁限制进入 manager 角色表

**结论：STATIC_CONFIRMED——角色 7 是固定原生候选项，并会经过统一的 achievement 判定后才追加到角色表；这不是无例外的严格锁定。** 具体存档中的角色 7 配置 achievement 数值未从本次静态资料确定，因此该存档是否走“已解锁”还是某个例外分支为 **UNKNOWN**。

IDA 中 `sub_98F5C0`（VA `0x98F5C0`，RVA `0x58F5C0`）在 VA `0x98F76A`（RVA `0x58F76A`）调用 `sub_994CD0`。官方 `ASMPlayerManager.cpp:20–39` 将该目标函数签名标作 `PlayerManager::init_special_baby_selection`。函数在 VA `0x994E85`（RVA `0x594E85`）以 `this+0x70` 访问 vector；`[this+0x70]`、`[this+0x74]`、`[this+0x78]` 分别作为 begin/end/capacity 使用。`dword_B2F8F0` 的有限静态表在 RVA `0x72F8F0` 起包含 `0,1,2,3,4,5,6,7,8,9,10,13,...`，其中下标 7 的值确为 7（VA `0xB2F90C`，RVA `0x72F90C`）。

原生候选循环从 VA `0x994EC0`（RVA `0x594EC0`）读取该表项到 EBX。角色 7 经普通非负类型路径取得 Player config，并在 VA `0x994EFB`（RVA `0x594EFB`）读取 `[config+0xE8]`；官方 `EntityConfig_Player.zhl:31` 将该字段定义为 `_achievement`。在候选 7 的配置满足有效索引路径时，汇编条件如下：

- achievement `<0`：`0x994F01` 的 `test` 后在 `0x994F03` 跳到追加块。
- achievement `==0`：在 `0x994F0C–0x994F0E` 直接跳到接受路径。
- achievement `1..640`：`0x994F10` 检查 `[manager+achievement+0x4C]`。非零表示已解锁并接受；零则只有在 `[manager+8]==2`、Game 指针非空，且 `[game+0x25BA8]>0` 或 `[game+0x25B01]!=0` 时才由例外分支接受，否则跳过该候选。achievement `>=641` 在 `0x994F05/0x994F0A` 跳过。
- 接受路径还检查角色 ID 9 或 30 的额外条件 `[manager+0x324]>0`（`0x994F38–0x994F49`）；角色 7 不满足这两个 ID 比较，因此不受该附加条件影响。成功追加发生于 `0x994F4B–0x994F65`。

故对常规保存路径，若角色 7 的 `_achievement` 是正值且小于 641，角色 7 受已解锁位限制；如果字段为负数或零则会通过，若 manager state2 的 daily/debug 例外成立也会通过。不能概括为“角色 7 总是锁定”或“必须永久解锁才能进表”。角色 7 的静态配置值及 R4 所述第二存档实际解锁状态不由这些机器码单独决定。

官方 REPENTOGON 在 `repentogon/Patches/ASMPatches/ASMPlayerManager.cpp:20–39` 的补丁命中特征落在 VA `0x994FEA`（RVA `0x594FEA`），即从候选 ID `0x29`（41）开始的 modded-character 循环，而不是上述 stock 候选循环。`XMLPlayerExtras.cpp:67–74` 的 `IsCharacterUnlockedRgon` 是自定义角色 XML 检查；不能把它当成 stock role 7 的入口判定。IDA 有限代码范围与官方补丁源码见 `ida-evidence.json` 的 `role_table_stock_candidates_and_unlock_branch`、`role_table_modded_character_loop`。

**实现影响：** 主机指定角色 7 的 `CharacterAvailable` 应依据本次初始化后 manager 角色表实际包含值 7 与否决定。用户确认第二存档已解锁只能作为该存档事实输入；不要改存档或解锁状态来满足表项。角色 7 的 config achievement 具体值和实际运行时表项仍应由实现所用 manager 表检查处理，而非依据常识常量猜测。

## 2. `Player::Init` 对角色 7 的专用分支或附属实体

**结论：STATIC_CONFIRMED（有限直接路径）——在直接 `Player::Init`、InitPre 和 InitPost 中，没有识别到以 `_playerType==7` 为条件的专用分支，也没有识别到角色 7 专用的附属 Player 创建。** 这不等同于“角色 7 与其他角色在初始化结果上完全相同”。通用 helper、配置驱动初始 stats/items、REPENTOGON/Lua 回调及间接实体创建闭包保留 **UNKNOWN**。

当前 IDB 中 Player Init 函数为 VA `0x74F6A0`（RVA `0x34F6A0`）；Init 在 VA `0x7518B5`（RVA `0x3518B5`）直接调用 InitPre VA `0x7B2C90`（RVA `0x3B2C90`）。官方声明见 `EntityPlayer.zhl:89`（InitPostLevelInitStats）、`:297`（InitPreLevelInitStats）、`:747`（`_playerType+0x13BC`）及 `:815`（`_babySkin+0x1AD8`）。主审以官方签名独立确认了 InitPre/InitPost 地址。

直接 `Player::Init` 对 `_playerType+0x13BC` 的直接引用扫描仅见初始化写入 `0x74FC16/0x74FCA2` 和尾部 `cmp [esi+0x13BC],0x12`（VA `0x751A7D`，RVA `0x351A7D`）。角色 18 分支调用 `sub_75D4B0` 与 `sub_7899A0`；其余路径去通用 helper `sub_84A7E0`。在 InitPre 的 `_playerType` 直接引用中，专用比较包括 type 22（`0x7B3176`）和 type 25（`0x7B31C5`），没有 type 7。InitPost 的直接比较包括 type 1、0、14、5（`0x7B3B90/0x7B3C94/0x7B3CF6/0x7B3D5F`），没有 type 7。完整有限引用列表保存在 JSON 的 `player_type_field_references`。

InitPre 的已读特殊附属 Player 分支比较 type `0x1D`（29）于 VA `0x7B3086`（RVA `0x3B3086`）；条件满足后于 `0x7B30C8` 分配 `0x2EA0` 字节，`0x7B30E1` 调构造函数 `sub_74DC70`，`0x7B3105` 虚调新 Player vtable `+4`（传入 type `0x26`），再于 `0x7B310E` 写 child 的 parent 指针。此分支角色比较不是 7。

需要排除一个容易误判的 `cmp eax,7`：Init 在 VA `0x750EAC`（RVA `0x350EAC`）先要求 `[edi+0x2C]==1`，再于 `0x750EB6` 从 `[edi+0x1AD8]` 读 EAX；官方将 `+0x1AD8` 定义为 `_babySkin`（`EntityPlayer.zhl:815`）。因此 VA `0x750EBC`（RVA `0x350EBC`）的 `cmp eax,7` 是 skin 值分支，不是 `_playerType==7`。该分支后续调用 `sub_7D3A30`，不可据它宣称 Azazel 的角色初始化分支。

**UNKNOWN 边界：** 没有穷举 `sub_84A7E0`、InitPre/InitPost 的所有通用 helper、配置读取造成的角色差异、间接函数指针/虚调用、官方或 Lua 回调，以及它们的所有可能实体创建。没有识别到 role7 专用 Player 附属实体创建，不足以证明任意上下文中绝无其他附属实体或配置差异。

## 3. Door owner 直接更新路径上的 ordinal 1 Shooting 暂停

**结论：STATIC_CONFIRMED + SOURCE——当 Door owner 下的虚调用沿当前 Player vtable 进入正常 Player::Update 子类体，且运行时 Hook 已 armed、在主线程上将该 Player 识别为有效 ordinal 1 绑定时，Shooting consumer 的 original 会被暂停。** 这是实现逻辑的静态结论，不是本轮运行时观测。

IDA 中 Door 更新函数 VA `0x826430`（RVA `0x426430`）在至少三个分支通过 Player vtable `+0x0C` 虚调用：VA `0x826557`、`0x8265A2`、`0x8265C5`（RVA 分别 `0x426557`、`0x4265A2`、`0x4265C5`），对应汇编 `mov eax,[edi] ; mov ecx,edi ; call dword ptr [eax+0Ch]`。当前 Player vftable address point VA `0xB1BDB4`（RVA `0x71BDB4`）的 `+0x0C` 槽位 VA `0xB1BDC0` 存放目标 VA `0x77BD40`（RVA `0x37BD40`），即已配置的 Player::Update Hook 目标。Player::Update 在 VA `0x77DE1D`（RVA `0x37DE1D`）以直接 `call sub_773990` 到射击 consumer VA `0x773990`（RVA `0x373990`）。JSON 同时保存 vtable DWORD、调用指令和 xref。

Owner 与抑制实现链：

1. `mod/binding/hooks.json:17` 将 Door 的 `RoomTransition::Update` RVA `0x426430` 标为 `Owner=Door`；`:22–23` 注册 Player Update Hook；`:28` 注册 Shoot consumer RVA `0x373990`。
2. `mod/src/hooks/context.cpp:10–22,25–36` 在主线程 Hook 入口压入配置 owner，并在同步原函数返回后弹出。Door 的 original 在调用 Player::Update 期间，当前 owner 因而仍为 Door。
3. `mod/src/game/context.cpp:36–40` 将 `Owner::Door` 与 `Owner::NestedDoor` 列为阻断上下文。
4. `mod/src/game/consumers.cpp:18–33` 按冻结 ordinal 和 topology pair 识别 A/B；Host 模式检查两个参与者。`:59–73` 的 Shooting 决策要求 `tick && !r.blocks && BoundValid && 同一 player && ContextHealthy && !InBlockedContext && !r.failure` 才调用原函数。Door 上下文令 `InBlockedContext()==true`，因此此分派 `CallOriginal()==false`；若没有 Authority tick 也同样为 false。
5. `mod/tools/gen_binding.py:75–80` 生成 Shoot wrapper 时先构造 `ConsumerScope`；若 `CallOriginal()` 为 false 则直接 return，不执行 `super()`。`mod/src/game/bridge.h:73–78` 注明只有 Shooting 可主动抑制，且 `CallOriginal()` 返回该决定。

**前置条件及边界：** Hook 在当前 session 已安装并 armed；调用在主线程；Player::Update 路径实际抵达 `ConsumerShoot`；ordinal 1 被 `ControlledOrdinal` 认作当前 pair。`ConsumerScope` 非主线程早退且 `call_original_` 默认 true，因此不能外推到任意线程。若 REPENTOGON 的 Player pre-update callback 返回“跳过”分支，官方 `CustomCallbacks.cpp:523–533` 会改调 `Entity::Original_Update()` 而不继续 `super()` 的 Player 子类更新链；这种路径可能根本不抵达射击 consumer，属于绕过该分派点，不应记作该 consumer 未暂停。除上述调用路径外的角色回调/武器自定义发射路径没有在本范围闭合。

**实现影响：** 切房直接 Door→Player::Update 虚调无需单独识别其调用点；当前 Door owner 继承关系足以让 ordinal 1 的原生 Shooting wrapper 在阻断窗口返回前短路。实现或验收报告应按 shooting dispatch/original 决策判断，不以 Door wrapper 次数或虚调次数替代实际 consumer 到达。运行时 callback 是否跳过、特定帧 tick/blocks 值及真实激光结果仍由 tx-04-07 唯一整体 E2E 判定。

## 未决项

- 角色 7 当前官方配置的 `_achievement` 数值以及实际 manager 表中的值：UNKNOWN。
- Init 的通用 helper、配置与官方/Lua 回调的传递影响和任意间接实体创建：UNKNOWN。
- Door 路径中运行时 Player pre-update callback 是否绕过子类更新、consumer 每次实际到达情况与射击效果：UNKNOWN；本报告只确认静态调用链与当前逻辑判定。
- 没有做任意角色、任意道具、任意 callback 或任意实体的完整闭包扫描。
