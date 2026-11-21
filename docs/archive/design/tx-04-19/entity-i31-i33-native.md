# I31—I33：E13 NativeText／NativeDummy 来源与窄入口研究

日期：2026-11-03。范围：为 E13 查找 Source 中真实 NativeText 写入场景与 NativeDummy 创建场景，并给无公开创建 API 的 Dummy 明确 E2E 窄桥合同。证据来自本地原版 Wiki、REPENTOGON 1.1.3 文档／ZHL／源码，以及 IDB `isaac-ng-v1.9.7.12.exe.unpacked.exe`（ImageBase `0x400000`）静态分析。没有运行游戏、构建、测试或探针，也没有修改运行代码或 IDB。本文的“可达”只指静态调用链可达，不代表运行时已实测。

## NativeText：真实写入 caller 与触发场景

原版 Wiki 将 `ENTITY_TEXT` 记为 Type 9001，并公开通用 `Isaac.Spawn`、`Game:Spawn`；这些接口按类型走工厂创建，但不提供文本 buffer 写入 API。REPENTOGON Wiki／ZHL 没有发现专门的 Text 创建／写入包装器。现有字段、writer witness 与 Render phase 合同见[Entity Source 研究](entity-source-research.md)及[C12 应用合同](tx-04-19-entity-application.md)。

静态二进制中已定位两个真实游戏 caller，都调用同一个内部 writer `sub_6FF270`：

| caller | 调用点 VA / RVA | 分支输入 | 触发条件 |
|---|---:|---|---|
| Entity 公共更新 `sub_6A9CF0` | `0x6AAAF0 / 0x2AAAF0` | 格式 `"dmg %.2f"`、本次伤害数值、Entity 位置、绿色 KColor | `Game+0x25ABC` 的 bit 6 为 1，且该更新正在处理正值伤害记录 |
| Player 虚调函数 `sub_76B900` | `0x76D2BC / 0x36D2BC` | 格式 `"hurt %d"`、伤害值转整数、Player 位置、KColor `0xFFFF4500` | `Game::GetDebugFlag(6)` 返回 true，并进入对应 Player 伤害处理路径 |

REP 官方 [`DebugFlag`](../../../../REPENTOGON-1.1.3/docs/docs/enums/DebugFlag.md) 将 `DAMAGE_VALUES` 定义为 `1 << 6`；[`Game`](../../../../REPENTOGON-1.1.3/docs/docs/Game.md) 暴露 Get/AddDebugFlags。原生 Console 处理函数 VA `0x688100` 包含 `debug` 命令：VA `0x689635` 起读取数字，`atoi` 后减一，再调用 `Game::ToggleDebugFlag`。REP ZHL 声明 ABI 为 `__thiscall void Game::ToggleDebugFlag(unsigned int flag)`；IDB 对应 VA `0x681540 / RVA 0x281540`，Console 调用点将 Game 放入 ECX、将零基 flag index 压栈，函数在 `Game+0x25ABC` 对应字中异或 `1 << index`。因此 Console 输入 `debug 7` 切换 bit 6；重复一次可切回。E2E 若经 Lua `AddDebugFlags` 打开该位，需先读取并保留原 mask，再只恢复本次改动的位；不要清掉此前已存在的其它 debug 位。当前 REP Lua 文档公开的是 Get/Add，未发现 RemoveDebugFlags API。

原版自然场景是让真实 Player 或受支持实体承受一次伤害，同时启用 `DAMAGE_VALUES`。这会经过游戏自身 writer，而不是测试代码直接伪造文本。Entity caller 的原始指令在 `0x6AAAB2–0x6AAAF0`：读取 Game debug mask 并测试 bit 6，随后传入 `"dmg %.2f"`、位置和颜色后调用 writer。Player caller 在 `0x76D27B–0x76D2BC`：查询 flag 6，构造 `"hurt %d"` 和橙红色后调用 writer。要取得 E13 的真实文本来源，应在 writer 完成后按现有 Entity identity/use/revision 记录见证；仅仅有 Type 9001 对象或普通 `Render` 不足以证明 buffer 属于本代写入。

`sub_6FF270`（VA `0x6FF270 / RVA 0x2FF270`）静态路径如下：先格式化共享 `ErrMsg`，调用 `Game::Spawn(Type=9001, Variant=0, ...)`（经 `sub_424340`）；RTTI cast 至 `Entity_Text`；调用 vtable `+0x3C` 设置 KColor；在 `Text+0x418` 以容量 `0x100`、格式 `"%s"` 写入终止字符串；最后虚调 Update。helper 返回的是 Update 的整数返回值，不是 Text 指针。该 Update 调用后的原生返回地址为 VA `0x6FF344`；hook adapter 内应按现有文档从原生调用者返回槽确认此 writer，不能直接把 adapter 的 `_ReturnAddress()` 当成 caller。

通用 `Isaac.Spawn(9001, ...)`、`Game:Spawn(9001, ...)` 或 Console `spawn` 可以走 Type 9001 的专用 Text 池，但它们本身不写 `+0x418`。Text Init `0x5CC260` 不清 buffer；故这些入口只证明创建，不证明本代文本内容。Variant 1/2 的 `Entity_Text::Update` 在 counter 为 0 时分别于 `0x5CC38E`（`%d`）和 `0x5CC420`（`%g`）格式化 `+0x334` 到 `+0x418`，但当前资料没有证明普通 Spawn 为 `+0x334` 提供了合法、可复现的业务值来源；不应以未初始化／池残留数值补出 Source。若 E2E 不能触发真实 damage caller，应把 Text 字符串覆盖明确列为未完成，不能直接调用 writer 或手填 Source buffer 冒充自然事件。

相关静态点位：Text ctor / Init / Update / Render 为 `0x5CC1F0 / 0x5CC260 / 0x5CC340 / 0x5CC580`；Type 9001 的池来自 `EntityFactory` ctor `0x6EEE60`，池为 `0x530 × 0x80`；工厂 `Create` 的 Text 特支位于 `0x6ED32B` 附近。

## NativeDummy：无自然 acquisition caller 的结论

原版 Wiki `EntityType.md` 将 `ENTITY_DUMMY` 记为 Type 964；REP `Global.zhl` 同样只提供枚举。没有找到官方 Lua／REP 创建包装器或业务 caller。IDB 中 `EntityFactory` ctor `0x6EEE60` 于 `0x6EF088` 预构造 128 个 Dummy 对象，池位于 `EntityFactory+0x675650`，stride `0x410`；构造／析构入口为 `0x6A3570 / 0x6A35C0`。Dummy vtable VA 为 `0xB181B8`，Init / Update / Render 分别为 `0x6A35D0 / 0x6A35E0 / 0x6A3630`。对 Dummy 构造函数的静态代码交叉引用只有工厂构造路径；没有识别到专用空槽 acquisition 函数或自然场景 caller。

`EntityFactory::Create` VA `0x6EC540` 对 Type 9001 有独立 Text 池分支；Type 964 未命中任何专用分支，落入默认 NPC 池（`Entity_NPC`、stride `0xF60`、0x200（512）项）。因此通用 `Game:Spawn(964, ...)`、`Isaac.Spawn(964, ...)` 和 Console `spawn 964` 都不是 NativeDummy 来源。原生 Console handler `sub_688100` 经 `sub_424340` 调通用 `Game::Spawn`，不会自行选择 Dummy 池；Type 数值或公开枚举名称不能替代真实 vtable owner。

## 建议的 E2E-only NativeDummy 窄桥

当前没有公开 Dummy acquisition 入口。E13 若要覆盖 NativeDummy，应把测试来源标注为 E2E-only bridge，而非自然游戏场景。建议桥接现有统一 Hook owner，限定 Source 主线程、有效 E2E token 和 `type == 964`：

1. ABI 采用官方 ZHL 声明：`Entity* __thiscall EntityFactory::Create(uint32_t type, bool force)`；owner 为当前真实 `EntityFactory`。仅对本次显式测试 token 的 Type 964 改走专用池扫描，其它输入原样调用原 Create。
2. 在 `EntityFactory+0x675650` 的 128 个预构造槽中查找空 use；核对精确 vtable `0xB181B8`、`valid == false`、`exists == false`。不改 vptr／Type，不覆盖活槽，不分配新对象，也不移动游戏内部 pool cursor。此处沿用 I22 已有的专用空池扫描合同。
3. 返回该预构造对象给原 `Game::Spawn` 后续顺序，让游戏完成 spawner／位置／速度设置、虚调 Dummy Init、Index 与 `EntityList::Add`。不要手动把 NPC 改成 Dummy，也不要旁路掉 EntityPlus 的 holder 与 EntityManager 生命周期。
4. 让现有 `EntityManager::Create` wrapper 对该返回值执行一次 NewEntity 通知；若实现位置绕过 wrapper，须在同一 owner 显式补一次。退休走正常 Remove 与 EntityList commit：DeleteEntity、ClearReferences、从列表移除、`valid=false`。工厂拥有的预构造槽不调用析构、不释放。

REP `EntityManager.cpp` 的 `HOOK_METHOD(EntityFactory, Create, ...)` 在 `super` 返回非空对象后通知 NewEntity；正常 `Remove`／commit 路径负责 DeleteEntity 与 ClearReferences。REP `EntityPlus.cpp` 在 Entity 构造时创建 holder、在基类 Init 时按 Type 重置 Plus，因此窄桥必须保留这条原生链，且不能重复发通知。上述只是给 E2E Source fixture 的静态调用合同；没有因此声称 Dummy 的自然业务来源或运行时可达性已验收。

## 来源与边界

- 原版 Wiki：`IsaacDocs/docs/enums/EntityType.md`、`Isaac.md`、`Game.md`。通用 Spawn API 支持 Type 参数，但文档没有为 Dummy 提供专用 allocator。
- REPENTOGON 1.1.3：`docs/docs/enums/DebugFlag.md`、`docs/docs/Game.md`、`libzhl/functions/EntityFactory.zhl`、`Game.zhl`、`Global.zhl`、`repentogon/Patches/EntityManager.cpp`、`EntityPlus.cpp`。未检出 Entity_Text／Entity_Dummy 的专用 Lua 创建 wrapper。
- 既有研究：[Entity Source 研究](entity-source-research.md)、[Entity 原生静态合同](entity-native-static.md)、[C12/C13 应用合同](tx-04-19-entity-application.md)、[I22 Slot/Text/Dummy 应用约定](tx-04-19-slot-text-draw-application.md)。

本文结论均为静态推导；I34 仍需分别核实 damage Text 的实际 Source writer witness、NativeDummy 窄桥的创建／发布／绘制／退休，以及画面和连续状态。没有实机运行、测试或 E2E PASS 结论。


## 主智能体审阅与实际采用

主智能体独立回读了 `0x6FF270` writer、`0x681540` 原始指令、`0x6EC540` Factory 全部分支及 `0x6A35D0` 的基 Init 跳转；并读 REP `EntityManager.cpp` 与 `HookSystem.cpp` 的降序优先级安装。纠正上文初稿的 NPC 容量笔误为 **0x200=512**。Text 夹具采用公开 `AddDebugFlags(DAMAGE_VALUES)` 与真实 NPC `TakeDamage`，由正常下一帧产生 writer；仅恢复本次添加的 debug 位。Dummy 的统一清单新增具名 `e2e_dummy_acquire`，优先级20000在官方0及两层观察之前安装，因此官方New通知仍包裹返回对象；观察wrapper的super保持一次。Source桥只在一次显式token Lua调用栈内激活，正式构建内联恒透传。生产Client I22的获取策略不改成测试入口。

这里说明采用代码与静态依据；实际Hook顺序、New/Delete配平、writer witness与画面仍由I34观测，不表示已测试。
