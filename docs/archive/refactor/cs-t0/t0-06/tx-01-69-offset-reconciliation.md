# tx-01-69：参考会话偏移逐项核对

日期：2026-10-18。用户要求补全操作 3 最后一项缺口：核对会话 `01a149e2-a48c-7cc2-883e-fd04d14e964d` 列出的偏移。本次读取该会话全部四轮的当前记录，保留其中类别、偏移和字段稳定性三条最终答复的[原始快照](../../../../../artifacts/cs-t0/tx-01-69-static/reference-session.json)。参考内容来自旧版 REPENTOGON，标为 `REFERENCE_VERSION_SOURCE`；本表核对其最终答复明确列出的结构偏移、池布局与虚槽，不把旧源码行为当作当前原生行为。

遵循[现行规范](../../project-conventions.md)。本轮由主智能体完成静态分析；既有 tx-01-63 至 tx-01-68 的当前版本证据按字段引用，新补分类、Index、flags、Grid 访问及当前虚表。没有重跑历史验收，没有运行游戏、构建或代码测试，没有修改 IDB 或计算/校验哈希。全部修订完成后仅一次整体静态文档验收。

当前 IDA MCP：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`；input 为同目录 `isaac-ng.exe`，x86，imagebase `0x400000`；`0xB7E1B4` 为 `Binding of Isaac: Repentance+ v1.9.7.17.J460`。本篇所有“当前”限于该数据库；VA−`0x400000` 为 RVA。下表“同值”只表示当前布局核对相同，不承诺跨版本或生命周期不变。整数按 raw 位宽登记，不由单个 `mov` 推断完整 C++ signedness。

## 1. 枚举根、容器与池

这些项目已由 [tx-01-63](tx-01-63-entity-lifecycle.md) 和 [tx-01-64](tx-01-64-category-coverage.md) 的当前指令闭合。本轮保留逐项对照，不重复把旧报告当成新运行时结果。

| 基址/成员 | 会话旧偏移 | 当前值 | 形式与证据 |
|---|---:|---:|---|
| Game.Room | `+0x18300` | 同值 | 4-byte 指针；Spawn `0x6FE410` |
| Game.PlayerManager | `+0x1BAA8` | 同值 | 内嵌对象；`0x9B9CD0/0x9BEE80` |
| Game.EntityFactory | `+0x25AF8` | **`+0x26580`** | 4-byte 指针；`0x6FE4F4/0x6F3060` |
| Room.EntityList | `+0x1218` | 同值 | 内嵌对象；`0x802980/0x86FB20` |
| Room.Grid slots | `+0x24` | 同值 | 连续 Grid 指针槽；当前上界为 448 项 |
| Room.Door view | `+0x724` | 同值 | 8 个 Grid 门指针；与 Grid 槽可别名 |
| Room.width | `+0x0C` | 同值 | 4 bytes；当前坐标/边界路径 |
| Room.height | `+0x10` | 同值 | 4 bytes；当前边界路径 |
| EntityList.main | `+0x20` | 同值 | 内嵌 EL 描述符 |
| EntityList.persistent | `+0x30` | 同值 | 内嵌 EL 描述符 |
| EntityList.update | `+0x40` | 同值 | 内嵌 EL 描述符 |
| EntityList.render | `+0x50` | 同值 | 内嵌 EL 描述符 |
| EntityList.effect | `+0x60` | 同值 | 内嵌 EL 描述符 |
| EntityList.temp | `+0x70` | 同值 | 内嵌 EL 描述符 |
| EntityList.bufferTemplate | `+0x80` | 同值 | 内嵌 EL 描述符 |
| EntityList.wisp | `+0x90` | 同值 | 内嵌 EL 描述符 |
| EL.sublist/flag | `+0x00` | 同值 | byte；描述符总长 16 bytes |
| EL.data | `+0x04` | 同值 | 4-byte `Entity**` |
| EL.capacity | `+0x08` | 同值 | raw32 |
| EL.size | `+0x0C` | 同值 | raw32；不与 Player vector 混用 |
| PlayerManager.playerList | `+0x00` | 同值 | 内嵌 12-byte vector，begin/end/capacity=`+0/+4/+8` |
| PlayerManager.EsauJr slots | `+0x80` | 同值 | 4 个 Player 指针；`0x9BCAA0` |
| Familiar pool | `+0x130008`，64 × `0xF30` | **`+0x130008`，64 × `0xF48`** | 工厂构造 `0x6F2A10`、销毁 `0x6F3060` |
| Pickup pool | `+0x357630`，512 × `0x6F0` | **`+0x357C30`，512 × `0x6F0`** | 同上；不能据此把全部池槽当活实体 |

参考会话末尾提出 Grid 声明长度 447 与接口 `0..447` 的矛盾。当前本体已由 tx-01-64 确认 **448 槽，索引 0..447**；因此当前直接复制应使用已确认的原生边界，并另记 8 门槽来源。无需调用查询或 Lua getter 来补齐枚举。七类动态对象也不能只看 updateEL 就宣称全部覆盖，沿用 tx-01-64 的来源规格。

旧会话只给 `g_Game` 签名，没有可直接移植的全局 RVA。当前已确认保存 Game 指针的全局槽为 **VA `0xC71678` / RVA `0x871678`**，需读取该槽得到 Game；它与 `Game+offset` 是两层地址，不是把全局槽本身当 Game 对象。

## 2. Entity 共同字段与剩余分类/flags 依据

| 字段 | 会话旧偏移 | 当前偏移/宽度 | 当前证据 |
|---|---:|---|---|
| vptr | `+0x00` | 同值，4 bytes | 当前 8 张 Entity 系虚表记录；不把 vptr 当网络状态 |
| Index | `+0x20` | 同值，4 bytes | getter `0x456370`：`8B 41 20` |
| Type | `+0x28` | 同值，4 bytes | getter `0x417260`：`8B 41 28` |
| Variant | `+0x2C` | 同值，4 bytes | getter `0x417270`：`8B 41 2C` |
| SubType | `+0x30` | 同值，4 bytes | getter `0x417280`：`8B 41 30` |
| EntityFlags | `+0x168` | 同值，**8 bytes** | `0x417420/0x417426` 分别读 `+168/+16C`，返回 EDX:EAX |
| valid | `+0x170` | 同值，1 byte | 发布 `0x41859C` 写 1，工厂按 byte 检查 |
| visible | `+0x171` | 同值，1 byte | getter `0x417480`：`8A 81 71 01 00 00` |
| exists | `+0x172` | 同值，1 byte | Remove `0x6ACB0C`：`C6 86 72 01 00 00 00` |
| dead | `+0x173` | 同值，1 byte | getter `0x417470`；Remove `0x6ACB15` 写 1 |
| removedByFactory | `+0x177` | 同值，1 byte | 工厂覆盖如 `0x6F01D8/0x6F020C` 置 1/0 |
| Position | `+0x33C` | 同值，2 × f32，共 8 bytes | [tx-01-66](tx-01-66-readonly-sampling.md)，getter `0x417290` |
| Velocity | `+0x360` | 同值，2 × f32，共 8 bytes | tx-01-66，getter `0x4172D0` |
| InitSeed | `+0x3EC` | 同值，4 bytes | [tx-01-68](tx-01-68-seed-owner.md)，getter `0x505B70` |

Type 数值 Player=1、Tear=2、Familiar=3、Pickup=5、Projectile=9、Effect=1000 沿用 tx-01-64 的当前工厂/类型依据；NPC 仍需结合已确认的类别/RTTI，不能按 `Type>9` 全收为 NPC。布局相同不代表 Type、Variant、SubType 在一次逻辑使用中恒定。

本轮补充的 `STATIC_CONFIRMED` 最小写入线索：

- `Entity::Init` 的 `0x6A93B5/0x6A93BB/0x6A93C1` 写 Type/Variant/SubType；当前原生 Variant setter `0x6A8180` 在 `0x6A8186` 单写 `+0x2C`，SubType setter `0x435CD0` 在 `0x435CD6` 单写 `+0x30`。它们是可变分类字段，不是实体永久身份。
- Index 在 Spawn 的 `0x6FE613..0x6FE63B` 由当前 Room 序号赋值后发布；不是跨房、跨 run 或跨进程唯一 ID。初始化和发布窗口不当作最终采样阶段。
- EntityFlags getter 对低/高两格读指令为 `8B 81 68 01 00 00`、`8B 91 6C 01 00 00`；Add `0x4173E6/0x4173EF` 分别 OR 两格，Clear `0x41740B/0x417413` 分别 AND 反掩码。`HasEntityFlags 0x417430` 只用于解释代码，不在采样时调用。
- `Entity::Init` 在 `0x6A96FB/0x6A9705` 将两格 EntityFlags 写零；`0x6A954C` 的 4-byte 存储从 `+0x171` 开始，按小端拆为 visible=1、exists=1、dead=0 等相邻字节，不能把每个 bool 都误作 4-byte 字段。派生路径/回调后续仍可改写。

参考会话称 Effect Variant setter 会清 `_varData`，对应的是 REPENTOGON 的扩展 Lua setter。本轮当前原生绑定 `0x6A8180` 只写 Variant，不据此加入旧 Mod 的附加行为。Type/Morph、全部 flags writer 与全部 bit 语义不在这次偏移核对范围。

flags 域额外消歧：当前 `Tear.TearFlags` getter `0x4177C0` 在 `0x4177C6` 以 `movups` 读 **Tear `+0x428` 的 16 bytes**；setter `0x676B9F` 写同一宽度，并包含其它状态处理。Tear Init `0x6661E2..0x666200` 的四次 dword 写入提供零初值锚点。当前 `Projectile.ProjectileFlags` getter `0x417830` 读 **Projectile `+0x438/+0x43C` 的 8 bytes**，其 Init `0x652BF2/0x652C0B` 分别写零。它们与 EntityFlags 是独立字段，不能因名称含 flags 就共用偏移或宽度。旧会话关于 EntityFlags bit 19 的类别语义仅作旧版本参考，本轮不声称已确认当前全部位含义；原样采样保留完整位串即可。

## 3. 玩家/Familiar 补充字段

| 基址/字段 | 会话旧偏移 | 当前值 | 当前依据 |
|---|---:|---:|---|
| Player.PlayerType | `+0x13BC` | **`+0x13C0`** | `0x4253B0`：`8B 81 C0 13 00 00`；旧位置当前为 GetDamageCooldown 字段 |
| Player.PlayerIndex | `+0x1610` | **`+0x161C`** | tx-01-64 的创建、替换及置 -1 指令 |
| Player.Twin | `+0x1E4C` | **`+0x1E68`** | `0x417820`：`8B 81 68 1E 00 00` |
| Player.backup | `+0x1E50` | **`+0x1E6C`** | `0x9BCBC4`：`89 88 6C 1E 00 00` |
| Player.replaced | `+0x267C` | **`+0x26C8`** | `0x9BEEC1/0x9BEECE`，正常替换窗口写入/清零 |
| Familiar.Player | `+0x410` | 同值 | 4-byte 独立指针槽；tx-01-68 `0x4361F0/0x60DF0B` |

PlayerIndex 不等同 Entity.Index；PlayerType 不等同 Entity.Type。Twin、backup、replaced 也不是同一个关系。引用目标不因保存值非零就被证明有效，replaced 不能当持续 roster。

## 4. 会话后来列出的 HP、state 与 timer

以下逐项同值，详细访问器、初始化及限制分别在 [HP 研究](tx-01-65-hp-analysis.md)和 [state/timer 研究](tx-01-67-state-timer.md)，未重新运行旧验收。

| 类别/字段 | 会话旧偏移 | 当前偏移/宽度 | 当前依据 |
|---|---:|---|---|
| Player.MaxHearts | `+0x1340` | 同值，4 bytes | tx-01-65 具名 getter/写入指令 |
| Player.RedHearts | `+0x1344` | 同值，4 bytes | 同上 |
| Player.SoulHearts | `+0x134C` | 同值，4 bytes | 同上 |
| NPC.StateFrame | `+0x410` | 同值，4 bytes | `0x8877E0` 注册/`0x6B8D4F` 初值 |
| NPC.State | `+0xB64` | 同值，4 bytes | `0x887AD0` 注册/`0x6B8D31` 初值 |
| NPC.ProjectileCooldown | `+0xBA0` | 同值，4 bytes | `0x8877E0` 注册/`0x6B8E35` 初值 |
| Familiar.State | `+0xD88` | 同值，4 bytes | `0x885F80` 注册/`0x60E2AD` 初值 |
| Effect.State | `+0x450` | 同值，4 bytes | `0x8854A0` 注册/`0x5CDBB8` 初值 |
| Effect.Timeout | `+0x45C` | 同值，4 bytes | 同一注册/`0x5CDBC2` 初值 |
| Effect.LifeSpan | `+0x460` | 同值，4 bytes | 同一注册/`0x5CDBCC` 初值 |
| Pickup.Timeout | `+0x540` | 同值，4 bytes | `0x5B1940` getter/`0x6E0359` 初值 |
| Grid.State | `+0x0C` | 同值，4 bytes | 下节独立 Grid 布局 |

旧会话没有给通用 HP 的具体偏移；该缺失已由 tx-01-65 补全为 Entity `+0x380/+0x384`，各 4-byte f32。沿用用户接受的“各 Entity 类统一使用这两项表示生命”假设，并保留 Player hearts 独立机制；这不是本轮新证明的各类实际生命算法。Tear/Projectile 尚未确认的专用 State/Timeout 也不通过套用 NPC/Effect 偏移补齐。

## 5. Grid 的独立布局

以下均直接相对 **GridEntity**，会话已加过 Desc 内嵌的 `+4`，不能再加一次。直接 Desc 绑定、seed 与独立 RNG 的证明在 tx-01-68；本轮另读 Grid type/variant/index/VarData 及 CollisionClass closure。

| 字段 | 会话旧偏移 | 当前值/宽度 | 当前依据 |
|---|---:|---|---|
| Grid.Type | `+0x04` | 同值，4 bytes | getter `0x4073C0` 读 `[ecx+4]` |
| Grid.Variant | `+0x08` | 同值，4 bytes | getter `0x43EEC0` 读 `[ecx+8]` |
| Grid.State | `+0x0C` | 同值，4 bytes | `0x4176F0` / tx-01-67 |
| Grid.SpawnCount | `+0x10` | 同值，4 bytes | Init `0x709DCD`：`FF 46 10` |
| Grid.SpawnSeed | `+0x14` | 同值，4 bytes | Init `0x709DB9` / tx-01-68 |
| Grid.VarData | `+0x18` | 同值，4 bytes | getter `0x67EFC0`：`8B 41 18` |
| Grid.Initialized | `+0x1C` | 同值，1 byte | Init `0x709DD0`：`C6 46 1C 01` |
| Grid.GridIndex | `+0x24` | 同值，4 bytes | getter `0x4E4830`：`8B 41 24` |
| Grid.SpawnFrame | `+0x28` | 同值，4 bytes | Init `0x709DDA`：`89 46 28`，来源当前 Game frame |
| Grid.CollisionClass | `+0x3C` | 同值，4 bytes | root offset 60→`0x88AF40`→getter `0x8C08F0` dword load |

Grid 初始化 byte 和 SpawnFrame 不能混淆。Grid 不继承 Entity，不读取 Entity 的 HP/flags/Parent 偏移；来源槽号与对象保存的 GridIndex 分开登记。Initialized 只是元数据，不能单独证明完整初始化、生命周期或引用有效。

## 6. 会话列出的虚表相对槽

本轮重新读取 Entity 基类及七类动态对象的 8 张当前虚表。下列槽相对位置与旧表一致；当前不同类的目标地址保存在原始 JSON 中，不将基类目标当所有派生类目标，也不在采样中调用这些虚函数。

| 旧方法名 | 旧/当前 vtable 偏移 | 当前 Entity 基类目标 VA |
|---|---:|---:|
| Free / deleting destructor | `+0x00` | `0x6A8820` |
| Init | `+0x04` | `0x6A93A0` |
| PreUpdate | `+0x08` | `0x6AE7D0` |
| Update | `+0x0C` | `0x6AE820` |
| TakeDamage | `+0x20` | `0x6ACE60` |
| Kill | `+0x24` | `0x6AC920` |
| Remove | `+0x28` | `0x6ACB00` |
| ClearReferences | `+0x44` | `0x6ACB60` |

这些是当前槽位/目标映射，不是所有调用约定、异常、释放算法或完整帧阶段的证明。比如 Free 参数决定是否释放，Remove 与清引用、摘表、析构仍是不同阶段。

## 7. 完成范围与证据

会话表中的 **8 处旧数值需要更正**：Game.EntityFactory、PlayerType、PlayerIndex、Twin、backup、replaced、Familiar stride、Pickup pool 起点。其余所列结构偏移/虚槽均已在当前版本证据中找到对应依据；Grid 数组长度争议按当前 448 槽处理。地址/偏移一致不等于字段恒定或业务语义完整。

操作 3 在本会话收窄后的**最小原样只读静态字段范围**至此补齐：分类、位置/速度、HP 假设、state/timer、flags、seed/归属及统一采样候选阶段都有登记。未实现或运行 capture；未建立操作 4 的临时身份；未完成状态比较、字段全集、全部 Morph/Restore/writer、运行时覆盖或整个 T0-06。

本轮[当前 IDA 证据](../../../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json)由[主导出脚本](export_tx_01_69.py)保存；[字段补充](../../../../../artifacts/cs-t0/tx-01-69-static/field-evidence.json)由[补充脚本](export_tx_01_69_fields.py)保存。脚本以非 main 名称读取有 main guard 的旧 helper，均 create-new，不修改或覆盖既有证据。历史材料分别见 tx-01-63/64/65/66/67/68；本轮新函数/指令用于补齐剩余项，不重复运行历史验收。唯一一次整体静态文档审阅见 [tx-01-69-review.md](tx-01-69-review.md)。
