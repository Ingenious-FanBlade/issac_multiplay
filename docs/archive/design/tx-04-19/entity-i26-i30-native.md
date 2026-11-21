# I26—I30 必要原生静态补证

2026-11-04 I34更正：下文记录的基类调用意图与旧实现不一致。官方SDK的 `e.Entity::Init/Remove/ClearReferences` 包装器内部仍按虚表派发，不能通过C++基类限定跳过派生。Client07实际Familiar退休AV后，指定Luna max补查、主智能体独立复读，19 adapter已改为调用唯一清单已有的base entry RVA `2A4A20/2A8000/2A8060`，保留官方Hook／Plus／ESSM链；具体失败、ABI／callee与续测结果见[统一验收](tx-04-19-acceptance.md)。旧记录按当时采用解释保留，以本次更正为准。

目标：`isaac-ng-v1.9.7.12.J273`，IDA ImageBase=`0x400000`。本文只记录本地实体 Wiki、REPENTOGON 源码和目标 IDB 静态证据；VA/RVA 按 `RVA=VA-0x400000`。标记：`SRC`=直接源码，`J273`=二进制直接观察，`推导`=基于两者的设计结论，`UNKNOWN`=尚未闭合。复用 [I20—I25 原生合同](entity-i20-i25-native.md)、[通用原生合同](entity-native-static.md) 与[应用合同](entity-application-contracts.md)，这里只补池预算、生命周期前缀与 I27 副作用。

## I30：Factory 实际池容量与复用

`J273` `EntityFactory::EntityFactory` `VA 0x6EEE60 / RVA 0x2EEE60` 用 `eh vector constructor iterator` 一次构造池槽；以下偏移均相对 `EntityFactory*`。表中 `cursor` 是池数据末尾的 4-byte round-robin 字段；下一个池从 cursor 后再偏移 8 bytes。ctor 立即数分别是 count、stride；`lea`/cursor 清零位置用于主审复核。

| Factory 池 | 起点 | stride × count | cursor | ctor 的直接指令位置（VA） |
| --- | ---: | ---: | ---: | --- |
| NPC（其余普通类型） | `+0x45FE40` | `0xF60 × 0x200` = 512 | `+0x64BE40` | count `6EF00E`；base `6EF013`；stride `6EF01D`；clear `6EF02B` |
| Tear（Type 2） | `+0x223220` | `0x848 × 0x200` = 512 | `+0x32C220` | count `6EEF4A`；base `6EEF4F`；stride `6EEF59`；clear `6EEF67` |
| Projectile（Type 9） | `+0x194218` | `0x478 × 0x200` = 512 | `+0x223218` | count `6EEF19`；base `6EEF1E`；stride `6EEF28`；clear `6EEF36` |
| Bomb（Type 4） | `+0x16CC10` | `0x9D8 × 0x40` = 64 | `+0x194210` | count `6EEEEB`；base `6EEEED`；stride `6EEEF7`；clear `6EEF05` |
| Laser（Type 7） | `+0x32C228` | `0x568 × 0x80` = 128 | `+0x357628` | count `6EEF7B`；base `6EEF80`；stride `6EEF8A`；clear `6EEF98` |
| Knife（Type 8） | `+0x695E58` | `0x520 × 0x40` = 64 | `+0x6AA658` | count `6EF0A1`；base `6EF0A3`；stride `6EF0AD`；clear `6EF0BB` |
| Pickup（Type 5） | `+0x357630` | `0x6F0 × 0x200` = 512 | `+0x435630` | count `6EEFAC`；base `6EEFB1`；stride `6EEFBB`；clear `6EEFC9` |
| Familiar（Type 3） | `+0x130008` | `0xF30 × 0x40` = 64 | `+0x16CC08` | count `6EEEBA`；base `6EEEBC`；stride `6EEEC9`；clear `6EEED7` |
| Effect（Type 1000） | `+0x000000` | `0x4C0 × 0x400` = 1024 | `+0x130000` | count `6EEE96`；stride `6EEE9B`；clear `6EEEA6` |
| Slot（Type 6） | `+0x435638` | `0x550 × 0x80` = 128 | `+0x45FE38` | count `6EEFE7`；base `6EEFDD`；stride `6EEFEF`；clear `6EEFFA` |
| Text（Type 9001） | `+0x64BE48` | `0x530 × 0x80` = 128 | `+0x675648` | count `6EF03F`；base `6EF044`；stride `6EF04E`；clear `6EF05C` |
| Dummy（专用池） | `+0x675650` | `0x410 × 0x80` = 128 | `+0x695E50` | count `6EF070`；base `6EF075`；stride `6EF07F`；clear `6EF08D` |

`J273` `EntityFactory::Create` `VA 0x6EC540 / RVA 0x2EC540`：Type 1 Player 被拒（`6EC575`）；Type 2/3/4/5/6/7/8/9 依次取 Tear/Familiar/Bomb/Pickup/Slot/Laser/Knife/Projectile；1000 取 Effect；9001 取 Text；未匹配类型取 NPC。Dummy 无专用分支，`Create(964,true)` 是 NPC 槽。`Create` 的池可用性测试读 `_valid` byte `+0x170`（Effect 例 `6ED079`，NPC 例 `6ED1E9`），不读 `_exists +0x172`。

`J273` force/覆盖：`Create(type,true)` 扫完池后返回 null（Effect `6ED09E`，NPC `6ED20E`）；不会覆盖活槽。`force=false` 满池时调用 `CanOverwrite` vtable `+0x38`，但若找不到允许项仍选游标槽，写 `removedByFactory +0x177`，虚调 `Remove +0x28`、`ClearReferences +0x44`，清标志后返回同一地址（NPC 样例 `6ED221–315`；Effect `6ED0B1–1A5`）。因此可覆写调用有活对象复用风险；本用途固定使用 `true`。静态容量超限在纯值阶段拒绝；实际可用量在取得Entity之前只读盘点，不能把Factory扫描改称“写前预检”（它本身推进cursor）。

`推导` I30 不得把“512 个对象”当成池预算。NativePreflight 应按以上池分别统计目标需求；空槽至少满足 `_valid==false && _exists==false`，若两标志矛盾则视为不可用（`Create` 自身只看 valid）。只有本 Owner 内已确认、且本次计划保证先完成引用撤销与安全退休的对应池 use，才能计入可回收量。不可把地址属于其它 owner、部分 Init、在途 draw 或未闭合删除前缀计作空槽。availability 只读检查池，不调用 `Create` 探测，因为它推进 cursor 且成功后会触发 ESSM New。Dummy 需另按 vtable `B181B8` 与 `valid/exists` 双条件扫描其 128 专用槽。Player 不占 EntityFactory 池。

## I28：Acquire、Init、ESSM 与安全退休

`SRC` REPENTOGON `repentogon/Patches/EntityManager.cpp`：`HOOK_METHOD(EntityFactory, Create)` 先调原 Factory，非空时同步 `NotifyNewEntity`；该 closure 经 `lua_pcall` 调官方 `NewEntity`。`resources/scripts/main_ex.lua` 的 New/Delete 仅转发至 ESSM；`entity_save_state_manager.lua::_OnNewEntity` 建立 `s_validEntities`，相同地址重复 New 会先删旧 ESSM entity data，`_OnDeleteEntity` 清 valid 与各 mod data 表。它不是原生内存构造/析构。`EntityFactory` 池槽由 Factory 构造预建；`Create(type,true)` 只选择池 use、推进 cursor 并走 New 通知，不为该 use 分配/释放 heap。`EntityPlus.cpp::Entity::Init` 才重置 Plus holder 状态。

| 前缀状态 | 证据与允许的清理 |
| --- | --- |
| 尚未 Acquire | 没有池 use/New，释放候选模型即可。 |
| Acquire 正常返回、Init 尚未进入 | 要求 `acquire_returned`、无 `Init` 位、未 valid/exists、地址及池 use 归属明确。可预检 ESSM Delete binding 后只发**一次** Delete 配平 New；不调 Remove/ClearReferences/Free/析构，预构造池槽仍由 Factory 持有。 |
| Create 返回后地址/所有权冲突，或通知/回调前缀未知 | 保留原生指针与 owner、锁定 cleanup；不能把 New 遗忘，也不能 Delete 可能属于另一个活 use 的 ESSM。当前冲突分支保留候选指针并置 `cleanup_uncertain`。 |
| Init 尝试/Init 中断 | 保留 owner，不重入 Init，不自动 Remove/Free。`FormalExecute::Initialize` 在 `before_init` 前已标 `Init`，仅 Init 完整返回后才置 `initialized=true`；所以失败发生在 before_init 还是派生 Init 中无法由现有位区分，应按较危险前缀保留。 |
| Init 完成但未 Publish | 仍归对象 Owner，不可交给 Room 自行清。通过正式逐类 Retire 处理已核 owner、ESSM 与关系；异常保留完整前缀。 |
| 已 Publish | 先撤所有入/出关系，等待在途展示结束，再按实例取得序逆序、每泵退休一项；完整执行类别 Remove/ClearReferences、摘七个 EL、DeleteEntity 与类型缓存清理。 |

`SRC` `client_objects_native.cpp::EssmDelete`（约 168–185）：`execute=false` 只验证已封 ESSM 所有者/闭包身份，不执行 callback；`true` 才在 `lua_pcall` 中调用真实 Delete。正式 Remove 先验证 ESSM，再记 `Removed`，运行类别 `Retire`，检查 exists 和列表，记 `Detached` 并摘七个列表，真实 Delete 后写 `_valid=false` 与类型缓存清理。`Removed` 在进入类别删除之前记位，Delete 错误后不得自动重复 Remove/Delete；捕获异常或未知前缀时 owner 必须留存。此顺序与原版 REPENTOGON `CommitRemovedEntities`（逻辑 Remove 后 Delete callback，再 ClearReferences、valid=false、移出 owner list）不完全相同；当前适配器有意以类 owner 单独完成 Remove/Clear，再由公共 owner摘表并通知 ESSM，任何阶段失败均不得靠重试越过一次性位。

`SRC` `ClientNativeObjectsAbandon`：先逐项清 relation，确认全体 unlink 后才按最高 instance 逆序每泵退休一项。仅完整 Acquire、尚未 Init 的使用走一次 Delete 平衡；Init 后的正式实体走正式 Remove。失败后保持 owner/Room，不直接清 map。`ClientScene::Pump` 退休顺序为 Presentation → Objects → Grids → `ClientNativeWorldRetire`；后者才调用 `Game` room 重建 `VA 0x732D20 / RVA 0x332D20`，随后清 PlayerManager 与 HUD。`推导` 整个世界退休不能只清 Entity map 后把活池 use 交给 Room。Room 重建不是 ESSM、反向引用或池 use 的替代清理。

## I27：已采用路径中的副作用隔离边界

| 副作用 | 静态证据与本地实际隔离 | 缺口/限制 |
| --- | --- | --- |
| Global RNG | `J273` Init 使用的 `sub_6EB4B0` 对应 `native_global_random RVA 0x2EB4B0`。生成 wrapper 在 `initializing_class && ClientBusy` 下返回 `initializing_seed`，不调用原 RNG。Pickup/Familiar draw 的不同 inline streams 则由 `ClientEntityFrameScope` 对 `Base+0x7D0CA4 / +0x7D0674` 做 Source 安装及整帧恢复。 | 声音的 `dword_BD0F40` 是独立 RNG，不被 GlobalRandom hook 保护，见下。Init 安装 Source 最终量仍需覆盖类别实际随机结果。 |
| Sound | `SRC` `libzhl/functions/SFXManager.zhl` 与 `repentogon/LuaInterfaces/CustomCallbacks.cpp:288` 给 `SFXManager::Play` ABI：`__thiscall void(int ID,float Volume,int FrameDelay,bool Loop,float Pitch,float Pan)`，该目标 `VA 0x907C60 / RVA 0x507C60`。`J273` Play 在 `0x907D5D/9D/A7` 读写 `dword_BD0F40`。Laser `PlayInitSound VA 0x9483D0 / RVA 0x5483D0` 唯一 caller `0x949001` 属 `0x948FF0`；该 helper 被 Laser Update `0x950B90` 的 `0x951107/1298/141D` 调，非 Laser Init。当前 `client_entity_local_sound`（`CsSoundEffects::ClientLocalPlay`，RVA `0x507C60`）在 ClientBusy 时直接 return，不调用原 SFX 或 Post 回调；Host/Off 透传。退休的 `client_entity_retire_sound RVA 0x508140` 是另一条窄 stop helper。 | 当前 Initialize 中 category 7—10 走 `Entity::Init`（base-qualified）；故 Pickup `Init 0x6DD7C0`、Familiar `0x607AA0`、Effect `0x5CD070` 派生 Init 的音效分支被跳过，Slot 派生 Init 亦跳过。实际 category 1—6 的派生 Init 直接 callee 静态表未见 SFX Play。Tear 的 ZHL `EntityTear_InitTear_PrePlaySound` pattern 位于 `0x663BF0` 的 `sub_663BE0`；该函数虽调 Play，但唯一 caller 是 `0x663FDF`（`sub_663F90`），不是 Tear Init `0x662F50` 的直接调用证据；其具体产品语义仍 UNKNOWN。当前 ClientBusy 声音 hook 用于全窗口隔离；I26/27 若消费 Authority 瞬时声音，需另行具名允许入口，不能本地重演 SFX 裁决。 |
| Lua gameplay | `client_native_bridge.cpp::GuardedClientCallback` 在 ClientBusy 跳过一般 ModCallbacks；仅安全菜单回调 1023 有精确白名单。 | 官方 EntityManager New/Delete 的 ESSM closure 不是该通用 ModCallback 路径，仍需在准确时机调用以平衡 ESSM；其源码只维护 ESSM data，不运行第三方 ModCallback。Laser PostLaserInit 如走该 dispatcher则被 ClientBusy guard 跳过。 |
| NPC bestiary | `J273` NPC Morph `VA 0x6BD0C0 / RVA 0x2BD0C0` 在 `0x6BD24F` 调 `0x904DE0 / RVA 0x504DE0` encounter/bestiary writer。现有 `pgd_encounter` client hook 在 ClientPersistenceBlocked 时跳原链；AddBestiaryKill 亦有对应 hook。 | 当前形态替换/字段安装不调 Morph；若未来采用 Morph，仍须确保所有 PGD、Lua post/pre callback 在写入前被隔离。 |
| Slot/Pickup 持久写 | `J273` `Slot::Update VA 0x65A210` 对 `PersistentGameData::IncreaseEventCounter VA 0x903A50 / RVA 0x503A50` 的调用点为 `0x65D0C6/0x65E2BA/0x65E812/0x65E821`；现有 `pgd_counter` hook 在 ClientPersistenceBlocked 时跳原链。Pickup 集藏 writer `PersistentGameData::AddToCollection VA 0x9041E0 / RVA 0x5041E0` 有拾取链 xref `0x6E7EF3`，现有 `pgd_add_collection` hook 同样 ClientBusy 前置拒绝。Pickup `InitFlipState` 的 save-state/ItemPool/ANM Update 路径不属于当前 base Init。 | 当前 adapter 不调用 Slot/Pickup Update、碰撞、收集、领奖或 `InitFlipState`；直接写字段、消费已给资源、Render 不等于这些玩法动作。PGD hook只覆盖具名函数，不能代替不调用玩法 Update。Slot Update 还有声音、RNG、Spawn 等副作用，不允许作为补画面入口。尚未穷尽所有未来产品调用链上的原始持久 writer。 |

`SRC` 当前 Init 的明确选择见 `client_entity_adapters_native.cpp::Initialize`：Pickup/Familiar/Effect/Slot 走 `e.Entity::Init`，避免派生玩法 Init；其他类别走动态 `e.Init`。`J273` 直接 callee 清单中 NPC `6B4A50`、Tear `662F50`、Projectile `64F4B0`、Bomb `69D6C0`、Laser `9480E0`、Knife `939C70` 未见直接 `0x907C60`/sound wrapper；Laser 仍有 PostLaserInit 回调与类 RNG/资源副作用，不能因此把 Init 当纯操作。Retire 不调用 Kill/爆炸/掉落；Familiar 特别使用 base Remove/ClearReferences，避免改 Player 队形/装备/Grid。

## 明确未决

- 上表容量是静态上限，当前每个池实际空闲数、外部游戏 use 与本 Owner 可安全退休数必须在主线程 Preflight 读取；本文没有做运行时探针，也不能证明异常时 `valid`/`exists` 两位永远一致。
- `Create(type,true)` 返回前的异常与地址冲突不能假设 New 是否完整；冲突地址不得被另一个 entry 的 Delete 清除。仅对 `acquire_returned` 且 pool-use 身份唯一的 use 走未 Init 配平路径。
- 完整 SFX 转调图、所有逐类 Remove 的间接持久副作用和 Pickup/Slot 全产品 writer 闭包没有穷尽；本次只确认实际采用入口、具名 writer 和已接入隔离，不把静态直接 callee 阴性外推为全程序无副作用。
- `EntityManager::CommitRemovedEntities` 对 render/update/wisp 等辅助列表与 main/persistent owner 的处理不同；适配器当前自行检查并摘七个列表。外部原生列表别名、未知 EH 与 VM 丢失时仍需留存 owner。

## 主审收口

2026-11-03，主智能体先读两份本地Entity Wiki、官方`EntityManager.cpp`、`SFXManager.zhl`和`CustomCallbacks.cpp:288`，再独立复读`6EEE60`完整构造伪码，以及`6EEE96..6EEF67`、`6EF070..6EF0BB`的count／stride／base原始指令；复读`6ED079`的`valid +170`、`6ED09A/9E`的force拒绝、`6ED1C6..20E`的NPC池选择和`Create`覆盖分支。按原始指令纠正上表Bomb count为`6EEEEB`、Familiar count为`6EEEBA`，没有改变真实容量。

主审还独立读取`6A357A`调用基ctor、`6A39D4/DB`将valid／exists清零的原始指令，采用空槽双false；发现`!valid && exists`的未归类半活槽时，记录其指针并保留Factory／Scene隔离，不能让Factory按valid单条件选中它。

声音方面独立复读`907C60`伪码及`907D50..907DA7`原始指令，确认状态写和独立`BD0F40`流；采用官方ZHL的六参`__thiscall void` ABI，不以IDA推断返回int代替官方调用声明。主审逐项读生成器的`ClientPersistenceBlocked`前置分支和清单中的`pgd_counter`／`pgd_add_collection`／`pgd_encounter`地址，确认它们仍在一般观察original之前隔离；不据此许可Slot Update、拾取或Morph。

采用正常完整Acquire未Init只配平ESSM、未知Init／Remove保留owner、全场景先全图撤边再逐类逆序退休的合同。子智能体指出Factory返回冲突地址后置null会遗忘New责任，主智能体已保留该指针并锁住`cleanup_uncertain`。另将未知draw owner的真实退尽条件接到Scene，防止仅凭CPU队列空就继续销毁。原始UNKNOWN与未覆盖保持上文事实；本轮没有构建、测试或运行时结论。
