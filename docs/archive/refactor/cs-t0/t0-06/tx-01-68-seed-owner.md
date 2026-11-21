# tx-01-68：seed 与归属字段的最小只读登记

日期：2026-10-18。范围仅为当前 IDA 中八类对象（Player、NPC、Tear、Projectile、Pickup、Familiar、Effect、Grid）的具名 seed 候选、Entity 基类关系字段和 Familiar 专属 Player 关系。逐项核对对象相对偏移、字节宽度、访问形态及少量 Init/清理锚点；不恢复完整 RNG 消费、Morph/Restore/重用状态机，不实现采集器，不运行游戏、构建或代码测试。

遵循[现行项目规范](../../project-conventions.md)。沿用 [tx-01-66 的采样边界](tx-01-66-readonly-sampling.md)，不重新分析控制流。当前 IDA MCP health 为 `ok`；输入 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，IDB 为对应 `isaac-ng.exe.i64`，imagebase `0x400000`、x86，`0xB7E1B4` 横幅为 `Binding of Isaac: Repentance+ v1.9.7.17.J460`。以下 VA 均属本次打开的 IDB，RVA=VA−`0x400000`；不外推其他构建。

本篇中的字段绑定、raw 地址、宽度和所列指令为 `STATIC_CONFIRMED`；字段名所表示的算法语义及由多处指令合成的对象关系按 `STATIC_INFERENCE` 标注；采样规则为 `DESIGN_PROPOSAL`；未闭合项保留为 `UNKNOWN`。没有 `RUNTIME_OBSERVED` 结论。

## 适用对象与具名 seed 字段

`Entity` 根绑定位于 `0x866960`。沿用当前 RTTI/虚表登记 [tx-01-64](tx-01-64-category-coverage.md)，下列七类动态对象都以 Entity 为基类；本轮又确认每类所列派生 Init 中存在直接调用 `Entity::Init 0x6A93A0` 的指令。因而 Entity 根的 `InitSeed`、`DropSeed`、`GetDropRNG` 以及四个关系字段按 Entity 子对象偏移解释。GridEntity 是独立对象域，不套用 Entity 偏移。

| 对象域/候选 | 对象相对位置、宽度与形式 | 当前绑定及直接访问 | 初始化线索与适用范围 |
|---|---|---|---|
| `Entity.InitSeed` | `Entity+0x3EC`，4 bytes，raw32 整数 | 根注册 `0x868F61..0x868F6D`；getter `0x505B70`（RVA `0x105B70`）为 `8B 81 EC 03 00 00`，即 `mov eax,[ecx+3ECh]` | `Entity::Init 0x6A93A0` 在 `0x6A93C7`（RVA `0x2A93C7`）执行 `89 86 EC 03 00 00`，把 Init 参数写入 `+0x3EC`。Player `0x755B11`、NPC `0x6B8D0E`、Tear `0x6660D8`、Projectile `0x652BA8`、Pickup `0x6E0849/0x6E08A2/0x6E08C0`、Familiar `0x60DEE9`、Effect `0x5CDB71` 均可见直接调用该基类 Init。只据此登记七类共同继承字段，不代表所有其他 Init/恢复路径均已覆盖。 |
| `Entity.DropSeed` | `Entity+0x3DC`，4 bytes，raw32；是下方嵌入 RNG 的首 dword | 根注册 `0x868F72..0x868F7E`；getter `0x5B1590`（RVA `0x1B1590`）为 `8B 81 DC 03 00 00`，即 `mov eax,[ecx+3DCh]` | Entity::Init 在 `0x6A93CD`（RVA `0x2A93CD`）把同一 Init 参数写入 `+0x3DC`，之后还写 RNG 区域 `+0x3E0..+0x3EB`。此 getter 不是 `InitSeed` getter。 |
| `Entity.GetDropRNG()` / `RNG.GetSeed()` | `GetDropRNG` 返回对象内地址 `Entity+0x3DC`；可见连续 RNG 存储跨度为 `Entity+0x3DC..+0x3EB`（0x10 bytes）。`RNG.GetSeed` 只返回子对象 `+0` 的4-byte首字 | 根方法绑定 `0x868F83..0x868F8F`；`0x4174B0` 为 `8D 81 DC 03 00 00`，即返回 `this+0x3DC`；RNG 的 `GetSeed` 注册点 `0x867764..0x867770` 对应 `0x40C3A0`，其指令 `8B 01` 读取 RNG `+0` | Entity 构造 `0x6A8290` 在 `0x6A87C5/0x6A87CD/0x6A87D5` 写该 0x10-byte 区域，其中首 dword 初值为 `0xAA17414F`；基类 Init 再于 `0x6A93CD/0x6A93D8/0x6A93E0` 重设它。构造只说明对象初构造时的字段写入；池对象逻辑复用见 [tx-01-63](tx-01-63-entity-lifecycle.md)，不能把该构造初值当成每次 Spawn 的值。 |
| `GridEntityDesc.SpawnSeed` | `GridEntity+0x14`，4 bytes，raw32 整数 | 当前根绑定点 `0x86B5AA..0x86B5B5` 传入 Desc offset `0x10` 给 `0x88A270`；该 helper 闭合到 getter `0x8C0680` 和 setter `0x8C0620`。getter 的 `0x8C06C8` 为 `8B 14 3A`（从接收对象地址加闭包捕获 offset 取 dword）；setter 的 `0x8C0673` 为 `89 04 0B`（写入 dword） | `Grid::Init 0x709DB0` 在 `0x709DB9`（RVA `0x309DB9`）把 Init 参数写到对象 `+0x14`。Grid 构造 `0x709C40` 在 `0x709C7C` 先写常量 `0x16A9DE81`；不据此解释该常量的 gameplay 含义。GridDesc 相对 Grid 的 `+4` 映射由下文 `Grid.State` 交叉核对。 |
| `GridEntityDesc.VariableSeed` | `GridEntity+0x20`，4 bytes，raw32 整数 | 绑定点 `0x86B5DA..0x86B5E5` 传入 Desc offset `0x1C`，同用 `0x88A270` 的 raw dword getter/setter | Grid 构造 `0x709C40` 在 `0x709C8E` 写常量 `0x16A9DE81`。所检查的 Grid 基类 Init 没有直接写此字段；派生初始化、生成/恢复路径及其他 writer 为 `UNKNOWN`。 |

`GridEntityDesc` 的对象相对换算为 `STATIC_CONFIRMED`：根注册中的 Desc `State` offset 为 `+0x8`（`0x86B595`），而 Grid 的直接 `State` getter `0x4176F0` 在 `0x4176F0` 读取 `[ecx+0xC]`（`8B 41 0C`）。主智能体进一步确认根注册 `Desc` 显式传入 offset 4；helper `0x88ADA0` 在 `0x88AE03` 以 `C7 00 04 00 00 00` 捕获该偏移，绑定 getter `0x8C07B0`。后者从 Grid 加捕获偏移处读取两个连续 16-byte 块，在 `0x8C0835/0x8C083E` 复制到新建的 Lua userdata。因此描述结构是 Grid 内嵌的 `+0x4..+0x23`，不是需要额外解引用的 Desc 指针；Lua `Desc` 属性返回的是一份值拷贝。Desc `SpawnSeed +0x10`、`VariableSeed +0x1C` 分别换算为 Grid `+0x14`、`+0x20`。只读采样应直接复制 native 对象内已登记的字段，不调用会分配 userdata 的 Lua getter。

主智能体补充的 `Grid.GetRNG` 绑定指向 `0x85DDA0`（RVA `0x45DDA0`）：`8D 41 2C` / `C3` 返回 **Grid 内嵌 `+0x2C` 地址**。结合构造对 `+0x2C/+0x30/+0x34/+0x38` 的四个 dword 写入，可登记为独立的 0x10-byte RNG 原始存储候选；`RNG.GetSeed` 只对应其首4 bytes。它与 Desc `SpawnSeed(+0x14)`、`VariableSeed(+0x20)` 均不是同一存储。构造首字为 `0xAA17414F`，其余字的默认值及派生 Init 之后的当前值不作为本轮 gameplay 契约；本轮不穷尽该 RNG 的消费者或重置。

`STATIC_INFERENCE`：`DropSeed` 与 `RNG.GetSeed` 当前指向同一内存 dword；结合 `GetDropRNG` 返回内嵌子对象地址，可以把 `Entity+0x3DC` 解释成 Drop RNG 暴露的当前 seed/state 首字。`InitSeed` 则另存于 `+0x3EC`。基类 Init 确实将同一个参数同时写入这两个不同位置，但运行中的 RNG 消费可能改变 RNG 状态；本轮未追完消费者，因此不把二者视作可互换字段。需要保存完整 Drop RNG 原始状态时，候选为复制 `+0x3DC` 起的 0x10 bytes；`GetSeed`/`DropSeed` 单独只对应首4 bytes。

## Entity 归属与引用字段

| 具名关系字段 | Entity 相对偏移 / 宽度 | 当前 getter / setter 及存储形态 |
|---|---:|---|
| `Parent` | `+0x3BC` / 4 bytes | getter `0x42A260` 直接读 `[ecx+3BCh]`；setter `0x6A92A0` 对 `this+0x3BC` 调用引用 bookkeeping helper 后写入 dword |
| `Child` | `+0x3C0` / 4 bytes | getter `0x431560` 直接读 `[ecx+3C0h]`；setter `0x6A92E0` 对 `this+0x3C0` 走同一引用 bookkeeping 模式 |
| `Target` | `+0x3C4` / 4 bytes | getter `0x4B7080` 直接读 `[ecx+3C4h]`；setter `0x6A9320` 对 `this+0x3C4` 走同一引用 bookkeeping 模式 |
| `SpawnerEntity` | `+0x3C8` / 4 bytes | getter `0x4174A0` 直接读 `[ecx+3C8h]`；setter `0x6A9360` 对 `this+0x3C8` 走同一引用 bookkeeping 模式 |

四者是彼此分开的 4-byte Entity 指针槽，并通过 setter 把槽地址交给引用 bookkeeping（旧类型资料称 EntityPtr；本篇只断言当前可见存储和调用关系）。例如 Parent setter：`0x6A92A5` 形成 `this+0x3BC`，若旧值非空则 `0x6A92BE` 调 `0x4CF210`，在 `0x6A92C6` 执行 `mov [esi],ecx`，新值非空时再于 `0x6A92CD` 调 `0x4CDCF0`；其它三项以相同模式操作各自不同偏移。getter 只返回字段值；没有理由为采样跟随该地址解引用。

主智能体同时确认 `Entity.SpawnerType`、`SpawnerVariant` getter 为 `0x431430/0x431440`，分别以 `8B 41 34`、`8B 41 38` 读取对象 `+0x34/+0x38` 的 raw32 分类值。当前 Spawn 在 `0x6FE592/0x6FE5A9` 写入这两格，生成者为空的已检查分支写零。它们可按需单独复制，不通过解引用已复制的 SpawnerEntity 来重新计算；分类值不代替引用目标身份，也不据此推断所有 Morph/Restore 行为。

当前直接初始化/清理线索需窄化解释：

- `Entity` 构造 `0x6A8290` 在 `0x6A8752/0x6A875C/0x6A8766/0x6A8770` 分别把四个槽写零。这只说明对象初构造，不表示池中同地址每次逻辑复用都会再次构造。
- `Entity::Init 0x6A93A0` 在 `0x6A9753/0x6A975D/0x6A9767` 对 Parent、Child、Target 有直接写零；该函数早段包含虚调用。这里仅记录所见写入，不由“没有直接写 SpawnerEntity”推导所有派生 Init、回调或重用路径都会保留它，也不把这些写入当作所有引用 bookkeeping 都已更新。
- 本体 `Game::Spawn 0x6FE410` 在虚 Init 调用 `0x6FE610` 之前处理 SpawnerEntity：`0x6FE5A3` 取 `Entity+0x3C8` 槽地址，旧值非空时先调用 `0x4CF210`；`0x6FE5C6` 写入生成者指针，非空时再走添加引用 helper。此为该 Spawn 路径的顺序锚点，不外推 Morph/Restore 或其他生成路径。
- `Entity::Remove 0x6ACB00` 与 `Entity::ClearReferences 0x6ACB60` 是不同入口。后者逐项处理 `+0x3BC/+0x3C0/+0x3C4/+0x3C8` 的引用槽及 bookkeeping；不把 Remove 返回当作引用都已清零。详细阶段关系仍以 [tx-01-63](tx-01-63-entity-lifecycle.md) 为准。

`GetLastParent`（根绑定 `0x868F0C..0x868F18`，实现 `0x6A8990`）不是额外 raw 字段：它循环读取当前对象和后继对象的 `+0x3BC`，直到下一 Parent 为空，再返回链尾对象指针。为取得本对象 Parent，只复制 `+0x3BC`；不要调用该方法或逐层跟随目标指针。

## Familiar 专属 `Player` 关系

本轮确认 Familiar 具名注册了单独的 `Player` 关系（根注册上下文 `0x86A821..0x86A832`）。getter `0x4361F0` 的原始指令 `8B 81 10 04 00 00` 读取 `Familiar+0x410`，宽度4 bytes；setter `0x62E020` 在 `0x62E09B` 执行 `89 86 10 04 00 00` 写入该槽。该 setter 还处理其它 Familiar/Player 侧行为；本轮只把对象内保存位置作为只读候选，不调用 setter。

Familiar Init `0x60DE70` 在 `0x60DEEE` 读取继承的 `SpawnerEntity+0x3C8`，检查空值和 Entity Type。若其类型为 Player（Type=1），该值到 `0x60DF0B` 写入 `Familiar+0x410`；若为空或不是该类型，则经 `0x417870(0)` 的另一分支后仍在 `0x60DF0B` 写回返回值。`0x417870` 的完整语义本轮未闭合。由此只可说两字段在特定分支可能相等，不能将 Familiar.Player、SpawnerEntity、Parent、Child 或 Target 合并为一个概念。该字段不按四个 Entity tracked-reference 槽的清理机制推断目标有效性。

Player、NPC、Tear、Projectile、Pickup、Effect 没有在本轮确认另一个统一名为 `Owner` 的 Entity 原始字段；这只表示限定范围内未登记到该具名绑定，不表示它们不存在其他未命名或私有关系。

## 最小采样建议与未决项

`DESIGN_PROPOSAL`：采样沿用 [tx-01-66](tx-01-66-readonly-sampling.md) 所选同一主线程标准完整 Room 更新后、`g_update()` 正常返回且 `coop::TopExit` 前的只读边界。动态 Entity 字段样本限定于已完成相应 Init、属于当前来源容器的对象；可复制 `+0x3EC`（InitSeed）、按需复制 `+0x3DC..+0x3EB`（Drop RNG），并分别复制四个引用槽及可选的生成者分类；Familiar 单独复制 `+0x410`。Grid 保留 Grid/door 槽来源及 `+0x1C` Initialized byte，按需复制 `+0x14/+0x20`，若需其 RNG 则另记 `+0x2C..+0x3B`。Initialized 非零仅是辅助线索，不单独证明生命周期、全部初始化或引用目标有效；初始化完成未能确认时保留对象枚举记录并将字段可用性标为未确认，不静默丢掉对象或把该批宣称完整。以上都是字段值复制，不调用 Lua、查询、getter、虚函数或目标对象解引用。

地址值只允许用于同批别名/关系诊断，不构成跨 tick、跨 run 或跨进程身份。引用槽为零可按 NULL 原样记录，不等同于漏采；即使被复制的 dword 非零，也不据此宣称目标此刻存活、有效或同代。保存 `InitSeed` 不等于保存当前 RNG 状态；保存 `DropSeed` 首 dword也不等于完整复制 16-byte RNG 区域。

`UNKNOWN`：未穷尽 RNG 结构之外的全部消费者和状态变更；未追 Grid `VariableSeed` 的派生 Init、载入和其他 writer；未穷尽 Familiar.Player 清理/恢复路径、所有类别专用 owner 字段、Entity tracked references 的全部重用与回调交错。根注册中另一处 `SpawnSeed`（offset `0x5C`，helper `0x88F720`）的宿主已由主智能体从同一根注册上下文确认为 **RoomDescriptor**；它位于 `sub_88F2A0(...,"RoomDescriptor")` 与后续 `Room` 注册之间，不归到 Grid 或七类动态 Entity，本轮不继续研究该描述的 seed。未从 Lua 未绑定推断字段不存在。

本轮唯一原始 IDA 输出由 [export_tx_01_68.py](export_tx_01_68.py) create-new 写入 [ida-evidence.json](../../../../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json)，保留完整重点函数伪代码、decoded code-item 的 VA/RVA/bytes/instruction，以及绑定/初始化上下文；脚本拒绝覆盖已存在 JSON。本轮没有修改 IDB、运行时代码或旧证据，也没有运行游戏、构建或测试。主智能体可优先复核 `0x866960` 的绑定片段、Entity seed/RNG 函数 `0x505B70/0x5B1590/0x4174B0/0x40C3A0/0x6A93A0`、引用 getter/setter `0x42A260/0x431560/0x4B7080/0x4174A0` 与 `0x6A92A0..0x6A9360`、`0x6ACB60`、Spawn `0x6FE410`、Familiar `0x4361F0/0x62E020/0x60DE70`，以及 Grid `0x88A270/0x8C0680/0x8C0620/0x709C40/0x709DB0`。子智能体交稿后已停止 IDA 访问；主智能体复核见下文。

主智能体交稿后重新读取的访问器、引用 setter/清理、Spawn 与 Familiar Init 指令、完整根注册见 [primary-evidence.json](../../../../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) 和[复核脚本](export_tx_01_68_review.py)。直接 Grid.Desc 绑定与值复制的补充见 [grid-evidence.json](../../../../../artifacts/cs-t0/tx-01-68-static/grid-evidence.json) 和[补充脚本](export_tx_01_68_grid_evidence.py)。原始 JSON 保留，未重跑覆盖；最终一次整体静态文档审阅见 [tx-01-68-review.md](tx-01-68-review.md)。
