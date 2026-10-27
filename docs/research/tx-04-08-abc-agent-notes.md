# tx-04-08 A/C 静态补充笔记

> 主审后记：本文引用的layout InitUnknown是补正前状态；当前已根据下方原始指令更正并接入限定当前值规则，见[ABC实现与主审](../design/tx-04-08/tx-04-08-abc-implementation.md)。候选建议与UNKNOWN保留代理原始含义；Grid有限补查之后，主审另以实际空地/自建rock/路径0等约束接入场景，不能据此声称已实机安全。

日期：2026-10-27。目标二进制为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA imagebase `0x400000`；源码使用本机 `F:/projects_isaac/REPENTOGON-1.1.3`。本文仅为静态资料补充，供主审复读；没有运行游戏、测试、构建或校验脚本，没有改 IDB，也没有修改生产代码。

先前依照 `AGENTS.md`、`docs/refactor/project-conventions.md`、[tx-04-08 当前工程任务](../design/tx-04-08/tx-04-08-current-engineering-tasks.md)及 ST3/ST6/ST7/ST8 既有报告阅读。下面把新的有限证据与历史结论区分开。

## A：`Entity.removed_by_factory +0x177`

`mod/binding/world_layout.json:301` 仍把字段记为 `InitUnknown`，ST3 的初始结论也说 base ctor/Init 没有明确初始化依据（[ST3 报告](tx-04-08-st3-agent-notes.md)，其字段表和 UNKNOWN 清单）。只读复读发现该判断漏掉了一个跨字段宽写：`Entity::Init` VA `0x6A4A20` / RVA `0x2A4A20` 的 VA `0x6A4BD6` 是 `mov word ptr [esi+176h], 0`，原始字节为 `66 C7 86 76 01 00 00 00 00`。该 16 位零写覆盖 `+0x176`、`+0x177`，因此 `_removedByFactory` 在 **Entity base Init** 中有明确的零初始化依据。相邻 VA `0x6A4BCC` 的 dword 写是 `[esi+171h]=0x101`；VA `0x6A4DAE` 另写 byte `[esi+175h]=0`。完整只读摘录见 [Entity base Init 原始证据](../../artifacts/repentogon/tx-04-08-abc-static/entity-base-init-removed-by-factory.json)。

对 base ctor `0x6A3940` 的 201 条指令做了同样的显式 `+0x174..+0x177` 成员寻址复读，没有发现该区间写，也没有发现 `memset` / `rep movs` / `rep stos` 对象块清零。这个有限检查不是全映像、指针别名或所有派生代码的写闭包。

已保存的 `EntityFactory::Create` 资料（ST3–ST5 主审 `main-core-functions.json`，函数 VA `0x6EC540`）还记录了 pool 满时覆盖旧实体的临时标志：例如 VA `0x6EC685` 写 `[object+0x177]=1`；之后调用两个虚函数（`0x6EC69B`、`0x6EC6AC`），VA `0x6EC6B9` 清回 0。此是 factory reuse/移除流程，不是 Init 默认值，也不证明它是全程序唯一的写入方。

结论限于初始化历史：base Init 写零已确认；派生 Init、Hook 或其他 owner 是否在其后再次改写该字节仍未由本次有限范围闭合。当前值可读资格与初始值不是一回事。按本次会话已采纳的 +177 规则，只允许在支持的 Entity 创建 owner 完成后，或 exact-pair A/B 的有效 native PlayerUpdate 后，取得无污染来源且前后复核稳定的当前值；owner、Init、Restore 或重绑等生命周期变化会撤销资格。这个约束不回填任何对象的构造历史。生产采集器的 `current_public_ready` 与资格检查见 `mod/src/world/identity.h:62,83-104`、`mod/src/world/collector.cpp:323-325,529-535`。

## C：Player Replace / Restore 自然 owner 路线

目标没有公开 `PlayerManager::ReplacePlayer` Lua wrapper；ST6 的有限 LuaInterface source 搜索记于 [ST6 报告](tx-04-08-st6-agent-notes.md:41-43)。目标 IDA 显示两个 native caller：`0x5B4A60` 与 `0x7A3A40`。禁止 fixture 直接传任意 `old*` / `replacement*` 给 native Replace。

**Esau Jr. (`703`) 是一个静态可追的候选 item owner 路径，但仍应默认禁用。** `Global.zhl:1495` 将 `COLLECTIBLE_ESAU_JR` 定义为 703。`sub_7A3A40` VA `0x7A3A40` 在 `0x7A3CB5-0x7A3CC1` 读取 item ID 并比较 703；函数随后检查四个关联槽（`0x7A3CDF-0x7A3CF4`）。四槽全占用时走效果/无替换分支（`0x7A3CFF-0x7A3E46`）；有空位时分配 `0x2EA0` 字节 Player 并调用 ctor `0x74DC70`、虚拟 Init 和字段/关系装配（从 `0x7A3E58` 开始）。后续仍须 manager 中选出的关联 Player 非空：`0x7A4147-0x7A4153` 取槽并检查 null，之后才在 `0x7A420A` 调 `PlayerManager::ReplacePlayer 0x995E10`。原始反编译保存于 `artifacts/repentogon/tx-04-08-static/st6-agent/ida-restore-replace-callers.json`，目标流程的旧摘要见 ST6 第 3 节。

有限复读 `sub_7A3A40` 中 703 的完整分支后，入口将 `n3` 限为 `0..3`，并以 `32*n3` 步幅读取对应 slot 的 collectible ID；没看到要求该索引必须为 0 的条件。该 703 专属段从 item-ID 比较到 `0x7A420A` 的 Replace 调用，没有看到 `PlayerType`/标准 Isaac 身份比较，也没有看到可识别的 Lua `UseFlags` 常量比较。因此，没有直接静态证据表明标准 Isaac A 或 slot 0 被此分支封禁；这不证明外层 UseActiveItem、真实持有状态或更上游 owner 会允许触发。

该段仍要求当前 703 slot 的关联指针字段（`+5520`）为空才进入新建关联 Player 的路径；管理器的四个关联槽都占用时只走效果分支。可用槽会设置 slot 关联索引，稍后从 manager 关联数组解析出 Player 并检查非空，才在 `0x7A420A` 调 Replace。此处 `a4` 是 8-bit 参数；已见它影响四槽占满时一条 item 移除/效果分支，但不能由本函数把它映射成公开 Lua `UseFlags`，也不能闭合实际 `Slot=-1` / 显式 slot 的解析及外层权限。真实 PlayerType/角色限制、标准 Isaac A 的自然持有和调用可达性、两次动作间对象代次/绑定恢复，仍为 **UNKNOWN**。

REPENTOGON 公开 `EntityPlayer:UseActiveItem(Item, UseFlags=0, Slot=-1, CustomVarData=0)`：文档见 `REPENTOGON-1.1.3/docs/docs/EntityPlayer.md:72-79`，实际 Lua wrapper 见 `REPENTOGON-1.1.3/repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp:3247-3298`，其中最终调用 `player->UseActiveItem(...)` 并返回 resultFlags。`Global.zhl:3300` 的 `USE_OWNED=1<<2`；文档说明没有 `USE_OWNED` 时 vanilla item 的 `REMOVE` result 不会返回。可读的公开调用形状是 `A:UseActiveItem(703, useFlags, actualSlot, customVarData)`，但本笔记不把 `(703,0,-1,0)` 宣称成真实物品消费的等价动作。若目标是“自然持有 item”路线，应以真实 A Player/实际拥有状态和 active slot 为前提；完整 use 参数及 703 的最终 Replace eligibility 未证实前不执行。Replace 发生后旧 A `Player*` 的关联/资格必须撤销；第二次触发前只能由新稳定 exact-pair 与当前 ordinal-0 owner 重新确认，不可沿用旧指针。

另一个直接 caller `0x5B4A60` 的 item case 711 与 `COLLECTIBLE_FLIP`（`Global.zhl:1503`）相关。有限代码读到针对 Player type `0x1D` / `0x26` 和关联指针的条件；这些类型的业务映射与完整前置条件未闭合。依照用户要求，此路线保留 **UNKNOWN**，不当作默认场景。

**Restore** 的公开自然路线是实际菜单 Continue。`ExecuteStartGame` VA `0x932A20` 的 Continue 分支在 `0x932E7E` 调 `Game::RestoreState 0x6F43A0`，紧接 `0x932E89` 调 `AutoReassignControllers 0x9964A0`，随后还有房间更新和 PostGameStart；静态依据见 [ST7 报告](tx-04-08-st7-agent-notes.md:17-20)。Game Restore 还包括 Room、PlayerManager、Level 及 PostLevelInit owner，不可在单个 Player restore 或 owner 中途记完成。自然观察需等外层 Continue 完成和稳定复读；不向 Lua/native 传伪造 `GameState*`。真实 Continue 会恢复持久 run/player/room 状态，场景与保存影响需另行安排。

B 继续只从 C/S Host/Join 真实创建，不能以公开 `PlayerManager.SpawnCoPlayer2(playerType)` 造 B；本地公开 wrapper 为 `LuaPlayerManager.cpp:162-169,393-399`，但它会自行改变 roster/RNG 并绕开 Join 绑定。C/S Join 原型入口见 `mod/src/game/join.cpp:60-112,114-182,197-245`。这项约束与 Replace 候选是两条独立路线。

## 七类 Entity 与 Grid 参数核对

本机没有名为 `entities.xml` 的文件；使用可读的游戏资源 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/extracted_resources/resources/entities2.xml` 和 `REPENTOGON-1.1.3/libzhl/functions/Global.zhl`。下面数值只确认该枚举/资源行存在，不证明 J273 工厂、当前房间格子、运行时回调、无副作用或稳定可观测。候选列表按旧 min-read 七类：Player、Tear、Familiar、Pickup、Projectile、NPC、Effect。

`Game():Spawn` 的可读公开形状是七个显式参数 `(type, variant, position, velocity, spawner, subtype, seed)`；本地 API 文档 `IsaacDocs/docs/Game.md:518-522`，目标静态调用顺序及完成边界见 ST6 报告 `:11-17,64`。优先显式固定 seed；候选非零 seed 可先用 1；候选 velocity 可用 `Vector(0,0)`。位置必须来自实际 room 的已知合法空地，精确数值不能脱离房间布局猜；`spawner=nil` 的 Lua 转换/合法性未证实，若需引用应传经当前身份复核的 A，但这本身仍会建立 spawner 关系并带来状态影响。下表是数据源中存在的候选，尚无可被静态证明对七类都安全的统一位置/spawner/速度组合。

| 类别 | 候选 `type/variant/subtype` | 本地枚举/资源依据 | 主要风险与限制 |
|---|---:|---|---|
| Player | `1/0/0` | `Global.zhl:191`；`entities2.xml:2` | Game:Spawn 的 factory 路线不支持 Player；不用于 B，也不能替代 Host/Join。 |
| Tear | `2/0/0` | `Global.zhl:192`；`entities2.xml:8` | 会参加原生碰撞/运动逻辑；即使零速度也不等于无副作用。 |
| Familiar | `3/1/0`（Brother Bobby） | `Global.zhl:193`；`entities2.xml:125`，配置碰撞伤害为 0 | 仍可能触发 AI/发射 tears；spawner 与玩家归属关系需确定。 |
| Pickup | `5/10/1`（Heart，全红心 subtype） | `Global.zhl:195`；`PICKUP_HEART=10` (`Global.zhl:2584-2587`)；`entities2.xml:545` | 可被玩家拾取并改变生命/物品状态。 |
| Projectile | `9/0/0` | `Global.zhl:199`；`entities2.xml:898` | 发射物碰撞/寿命/目标行为未静态排除；不要因为 XML 初始 collisionDamage=0 认定无影响。 |
| NPC | `13/0/0`（Fly） | `Global.zhl:203`；`entities2.xml:960`（collisionDamage=0） | Fly 仍是会移动/参与 AI 的 NPC；备选 `10/0/0` 为实体 XML 的 Frowning Gaper 且 collisionDamage=1（`entities2.xml:926`），风险更高。 |
| Effect | `1000/15/0`（Poof01） | `Global.zhl:530`；`entities2.xml:3018` | 数据是零碰撞伤害/半径的 effect 候选，但生命周期短、回调及寿命未验证，可能错过观察。 |

不要把这张表当作可直接开启的 scenario。完整出生仍以 Game::Spawn outer owner 成功返回非空为界，且要记录 Factory、派生 Init、Publish 和后处理；Publish 本身不是完成。

Grid 的公开整数 wrapper 为 `room:SpawnGridEntity(index, type, variant=0, seed=..., varData=0)`。`LuaRoom.cpp:33-53` 对 index 限 `0..447` 并返回 bool。枚举见 `Global.zhl:643-672`：`GRID_DECORATION=1`、`GRID_ROCK=2`、`GRID_PIT=7`、`GRID_DOOR=0x10`。低风险候选是 `type=GRID_ROCK(2), variant=0, seed=1, varData=0`，但仅可针对静态/运行观察确定为普通空地且与 door alias 无关的合法 index；目标房间具体 index、格子类型规则及可否覆盖皆未验证，不能称为已安全值。排除已占格会减少覆盖旧对象风险，但不保证 factory/native 接受。

`RemoveGridEntityImmediate(index, pathTrail, keepDecoration)` 的 `PathTrail` 有直接数据流证据。REPENTOGON wrapper `LuaRoom.cpp:56-62` 将第三个 Lua 参数作为整数直接传入；目标函数 `0x813BD0`（IDA）先验证 index `<=0x1BF` 和 Grid 槽非空，对 `type==1 && KeepDecoration` 可早退；实际删除分支在 `0x813BF5` 原值写入 `Room+0x76C+4*index`，`0x813BFF` 清 `Room+0x24+4*index` 的 Grid 槽，`0x813C0B` 调被删对象 vtable slot 0、参数 1。没有对 PathTrail 作范围或枚举校验，传 `0` 会明确把该格对应表项覆盖为零；同一 `Room+0x76C` 表被 `0x706900/0x706950` 读取并按 `>0`、`>=1000` 参与 Grid 可通行判定，因此这是会改变路径/阻挡状态的真实写入。静态可给 PathTrail=0，但不能把它当成无副作用。

若只移除本夹具创建的普通 ROCK，限制必须由夹具侧在调用前核对：同一合法 index 当前槽仍是该夹具记录的实体，当前 type/variant 是 ROCK/0，且不属于 door alias；不匹配就跳过。`0x813BD0` 自身只按 index 找任意非空 Grid 槽，不验证创建来源或对象身份，wrapper 返回 0 个值，故调用请求后要独立观察实际槽变化。`KeepDecoration=false` 对普通 ROCK 不触发 decoration 保留早退，但不会增加身份保护。`Room::MakeDoor` 是目标 native owner `0x7E4DE0`，LuaRoom 注册中没有对应公开 wrapper；Door 在 Grid/door 槽中可能是同一对象的 alias，禁止用 `GRID_DOOR` 猜测生成或把 alias 记为两个出生。

## 保留的 UNKNOWN

- `+0x177` 之后是否有未复读的派生/回调写入；全映像所有原生写者与每个生命周期窗口的闭合。
- Esau Jr. 标准 Isaac A 的真实可达性/角色前置条件，公开 Lua UseActiveItem 的 flags 与 native `a4` 的对应、默认/显式 Slot 的解析，以及关联槽如何自然可用；第一次替换后 A 的重新绑定时机。
- Flip `711` 的全前置条件；特定 PlayerType 到角色名的完整静态映射。
- Continue 的持久副作用、各分支最终 Player roster，以及本次实际房间的 Entity 位置/速度/spawner 和 Grid index/type 可用性；PathTrail 任意非零值的完整路径语义。
- 无运行态证据证明上述任何候选参数会成功、无害或完成全生命周期观察。本笔记不改 scenario 的 enable 状态；Player Replace/Restore 动作应保持禁用，直到主审确认具体 route/授权。
