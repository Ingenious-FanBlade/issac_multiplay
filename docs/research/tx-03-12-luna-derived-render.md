# tx-03-12 Luna 子研究：NPC、Tear、Projectile 派生绘制入口

日期：2026-10-22（Asia/Shanghai）。性质：只读 IDA 静态分析；GPT-6 Luna / max 子智能体研究，主智能体独立取证并修订字段表述，见 [主审记录](tx-03-12-primary-review.md)。本文件是派生绘制范围的证据记录，不取代主审报告。

## 范围与结论

本轮核对 `Entity_NPC`、`Entity_Tear`、`Entity_Projectile` 当前 RTTI 虚表的 `+0x14`、`+0x18`、`+0x34`，反编译对应派生目标及必要的直接展示／几何／Lua wrapper。静态目标与指令不等于本次运行实际发生，也不覆盖所有子类、Mod、版本或资源状态。

结论是三类的 `+0x14` 均不是可直接归约为基类单 Sprite 叶函数的同义入口。NPC `+0x14` 有派生坐标与多种条件绘制，并经类型分支到额外实体／图形 helper；其 `+0x18` 对特定类型还构造并提交几何批次。Tear `+0x14` 写派生绘制坐标缓存，临时改 Sprite 相关状态并按条件绘制额外 Sprite 路径。Projectile `+0x14` 暂时调整 `PositionOffsetY`、计算派生绘制坐标，并有局部帧派生附加向量参数的 Sprite 路径。三者的 `+0x14` 还都能到达条件 Lua `pcall`；回调身份及脚本效果未知。

## 地址与证据约定

本次读取的活动 IDB 路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，imagebase `0x00400000`。这里只记录 IDA 当前 IDB 元数据，不声明它与同名磁盘 PE 内容相同。`RVA = VA - 0x00400000`；虚表槽偏移是 vptr 的字节偏移，对象字段偏移也不是 RVA。伪代码签名仅用于辅助阅读；特别是 `__usercall`、XMM 与优化栈参数不能未经机器码验证当作 ABI。call 指令地址记作 callsite，不是返回地址。

静态原始依据：[只读导出脚本](../../artifacts/cs-t0/tx-03-12-static/luna-export-static.py)、[虚表／派生函数完整导出](../../artifacts/cs-t0/tx-03-12-static/luna-derived-render.json)、[第一次 py_exec_file MCP 返回](../../artifacts/cs-t0/tx-03-12-static/luna-py-exec-file-return.json)。展开依赖所用的[第二只读导出脚本](../../artifacts/cs-t0/tx-03-12-static/luna-export-dependencies.py)与[派生依赖完整导出](../../artifacts/cs-t0/tx-03-12-static/luna-derived-dependencies.json)、[MCP 返回](../../artifacts/cs-t0/tx-03-12-static/luna-dependency-export-return.json)一并保留。MCP 原始返回另见[派生目标](../../artifacts/cs-t0/tx-03-12-static/luna-direct-callees-mcp.json)、[Lua wrappers](../../artifacts/cs-t0/tx-03-12-static/luna-lua-wrapper-callees-mcp.json)、[NPC 几何 helper](../../artifacts/cs-t0/tx-03-12-static/luna-npc-geometry-mcp.json)、[原始调用字节与 Lua 反汇编](../../artifacts/cs-t0/tx-03-12-static/luna-call-bytes-and-lua-disasm-mcp.json)及[完整 pcall／几何调用字节](../../artifacts/cs-t0/tx-03-12-static/luna-pcall-geometry-bytes-mcp.json)。后一个文件保存了完整 6 字节间接 `lua_pcallk` 调用；不以早期 5 字节读取推断其完整编码。

## 虚表目标与 EntityList 调用形态

活动 IDB 中 `Entity` 基类表 VA `0xB6730C`（RVA `0x76730C`）相同槽目标为 `+0x14 → 0x6B10F0`、`+0x18 → 0x6B2000`、`+0x34 → 0x6B1000`。所选派生表为：

| RTTI 类 | vtable VA / RVA | `+0x14` 表项字节 → 目标 VA / RVA | `+0x18` 目标 VA / RVA | `+0x34` 目标 VA / RVA |
| --- | --- | --- | --- | --- |
| `Entity_NPC` | `0xB67468 / 0x767468` | `50 2E 6D 00` → `0x6D2E50 / 0x2D2E50` | `0x6D4840 / 0x2D4840` | `0x6B1000 / 0x2B1000` |
| `Entity_Tear` | `0xB64EAC / 0x764EAC` | `C0 90 67 00` → `0x6790C0 / 0x2790C0` | `0x6B2000 / 0x2B2000` | `0x6B1000 / 0x2B1000` |
| `Entity_Projectile` | `0xB64990 / 0x764990` | `60 CB 65 00` → `0x65CB60 / 0x25CB60` | `0x6B2000 / 0x2B2000` | `0x6B1000 / 0x2B1000` |

`EntityList` 的 `0x419A2E` (`RVA 0x19A2E`) 有 `FF 50 14`：机器码在调用前将 Entity 放入 ECX，并 push 一个坐标指针，然后经 `[vptr+0x14]` 调用。另一个实际绘制 pass 在 `0x419C67` (`RVA 0x19C67`) 以 `FF D0` 经 `[vptr+0x18]` 调用，ECX 为 Entity、栈上有坐标指针。`0x4199F5` (`RVA 0x199F5`) 的 `FF D0` 调用 `[vptr+0x34]`，返回 EAX 后与排序阈值比较。故 `+0x34` 是参与排序的目标，不应当作绘制入口；三个派生类在该槽都共用基类 `0x6B1000`。

## NPC：派生绘制与类型化几何

### `+0x14 → 0x6D2E50`

此函数有 1,344 条指令，明显超过基类叶绘制。可直接确认：

- 以 `0x6D3217` 的 `E8 24 6F D3 FF` 调 `0x40A140`，在 `0x6D3C74` 的 `E8 77 D4 FD FF` 调基类 `0x6B10F0`，随后由基类到 Sprite 绘制 helper `0x40A0D0`。函数其他分支也直接调 `0x40A0D0`，其中 `0x6D3F72` 的字节为 `E8 59 61 D3 FF`。
- 该派生函数直接向本 Entity 写绘制缓存 `+0x1A4`、`+0x1A8`：`0x6D3E61` `F3 0F 11 97 A4 01 00 00`，`0x6D3E69` `F3 0F 11 87 A8 01 00 00`。还直接写 `+0xE4/+0xE8`（`0x6D3353` `F3 0F 11 8F E4 00 00 00`；`0x6D335B` `F3 0F 11 87 E8 00 00 00`），并有针对 `EDI`／`EAX` 接收对象的后续同偏移写入。完整写点与上下文保留于 JSON；字段含义不能仅凭偏移扩张。
- 对 Type 411/816 的 `"Item"` layer 分支，函数查找 layer 后暂时写关联 Sprite 的位置／比例、调用 `0x40A0D0`，再恢复这些字段；还有 OverlayEffect 与其它条件绘制路径。这些是条件派生显示工作，不是一次基类 Sprite 调用。
- 在 `0x6D4006` 以 `E8 95 CB 18 00` 到 `0x860BA0`。该 wrapper 只有当 Lua 标志字 `word0 & 0x10000000` 非零才走回调；传入整数 discriminator `28`、实体类型、Entity userdata 与独立 x/y 组成的 `Vector2`，经 `0x872B10` 在 `0x872C02` 调用 `lua_pcallk`（4 个参数、1 个结果，完整 bytes `FF 15 D8 83 B1 00`）。绑定的 Lua 函数及其效果未知。

### `+0x18 → 0x6D4840`

该 override 先按 `Entity+0x28`（伪代码显示类型字段）分支，再总会调用共享基类 `0x6B2000`。Type `886` 时先调 `0x4E9B10`，调用 bytes 为 `0x6D485A: E8 B1 52 E1 FF`；Type `19` 且 `this[11] == 3` 时先调 `0x59BED0`，bytes 为 `0x6D487D: E8 4E 76 EC FF`。随后两分支都调用 `0x6B2000`（`0x6D4862: E8 99 D7 FD FF`、`0x6D4885: E8 76 D7 FD FF`）。

两个前置 helper 都会把点／插值结果送入几何批次：`0x4E9B10` 多次调用 `0x41E720` 并调用 `0x41E960`（callsite `0x4EA2E7: E8 34 44 F3 FF`、`0x4EA335: E8 26 46 F3 FF`）；`0x59BED0` 亦如此（例如 `0x59C35A: E8 C1 23 E8 FF`、`0x59C495: E8 C6 24 E8 FF`）。后端 `0x41E960` 的函数体包含间接调用 `0x41F5DB: FF D6`，其目标未知。该分支明确不是只有 `0x6B2000` 的 Sprite layer 绘制；直接替换为基类 `+0x18` 会略去所选类型条件下提交的图形几何。更深的资源生命周期、后端目标及全部 helper 副作用不在本轮闭合范围。

## Tear：位置缓存、附加 Sprite 与 Lua 条件入口

`+0x14 → 0x6790C0` 在 exists/visible 字节条件满足时计算屏幕坐标，直接写 Entity `+0x1A4/+0x1A8`（`0x67920B: F3 0F 11 0B`、`0x67920F: F3 0F 11 43 04`）。`0x679356` 以 `E8 95 7D 03 00` 调基类 `0x6B10F0`。派生函数还直接使用 `0x40A0D0` 绘制关联对象：调用 receiver 包含 Entity `+0x4AC`、`+0x5C0`、`+0x6D4` 等条件路径；这些对象字段的产品语义不在此按偏移命名。

绘制前有针对 Sprite 状态字节／向量的条件路径：先保存 Sprite 状态的一组字段，经 `0x4073D0` 处理临时结构，调基类绘制后恢复该组字段；另有颜色／alpha 相关的条件分支和附加 Sprite 绘制。恢复证据限于正常静态路径，不能外推到异常、重入或所有早退。

函数末尾附近在 `0x6793E7` 以 `E8 94 97 1E 00` 调 `0x862B80`。只有 Lua 全局第二标志字 `word1 & 0x200` 非零才调用；wrapper 压入 discriminator `41`、实体 Variant（`+0x2C`）、Entity userdata 与 `Vector2`，在 `0x862CA5` 直接 `lua_pcallk(4,1,...)`，完整 call bytes 为 `FF 15 D8 83 B1 00`。初稿误写为实体类型，现按主审取证修正；当前静态导出没有把该 callback 注册点／Lua 侧效果闭合。

因此 Tear 的该入口包含派生坐标缓存与额外受条件控制的 Sprite 状态／子对象绘制，以及独立 Lua 条件入口。用基类 `0x6B10F0` 会丢失这些 Tear 侧写入和绘制；即便保留基类，也不能据“Sprite API”断定下层资源 helper 与 Lua 的副作用边界。

## Projectile：临时 PositionOffset、派生坐标与局部向量参数

`+0x14 → 0x65CB60` 的普通分支调用基类 `0x6B10F0`（`0x65CEB3: E8 38 42 05 00`），特殊分支直接调用 Sprite `0x40A0D0`，并有以下派生行为：

- 若 `Entity+0x440` 的字节条件成立，先把 `Entity+0x350`（`PositionOffsetY`）减去 `Entity+0x410`，于 `0x65CB84` 写回（`F3 0F 11 86 50 03 00 00`）；特殊分支计算后写绘制坐标 `+0x1A8/+0x1A4`（`0x65CD2B: F3 0F 11 86 A8 01 00 00`、`0x65CD37: F3 0F 11 8E A4 01 00 00`）；两条绘制分支会合后于 `0x65CED1` 加回 `+0x410`，用相同编码写回 `+0x350`。这是正常路径的计算式恢复，不是保存原位值后回写，不证明舍入后逐位相同、EH 或重入恢复；也不是 Position `+0x33C/+0x340`。
- 当 EntityFlags `+0x168 & 0x20` 且 `+0x460/+0x464` 的非零条件成立时，函数从 Game 计数派生局部 xorshift 式数值，组成两组额外向量传给 `0x40A0D0`；零种子分支会记录诊断并 `__debugbreak`。初稿把伪代码十进制 360 错写成十六进制 `0x360`，已修正为 `0x168`。向量是主坐标参数之外的绘制参数，完整下层语义未闭合，不宣称已经证明位置／纹理抖动，也不声称推进了游戏 RNG 或异常已发生。
- 函数随后在 `0x65CEDB` 以 `E8 90 57 20 00` 调 `0x862670`。当 Lua 第二标志字 `word1 & 0x2000` 非零，wrapper 传 discriminator `45`、实体 Variant（`+0x2C`）、Entity userdata 与 `Vector2`，转 `0x873740` 并在 `0x873832` `lua_pcallk(4,1,...)`，完整 bytes `FF 15 D8 83 B1 00`。初稿误写 filter 为实体类型，已按主审取证修正；Lua 回调身份和效果未知。

在本轮已导出的直接函数体里，Projectile 派生 draw 的可见图形输出走 `0x40A0D0`／基类 `0x6B10F0` Sprite 路径；这仅是当前入口及直接闭包的观察，不证明间接图形目标、变体或任意资源 helper 没有其它效果。直接用基类 `+0x14` 会丢失 PositionOffset 的临时变换／恢复、派生绘制坐标和附加向量分支，也会避开这个条件 Lua 入口。

## 边界与交付状态

以上仅覆盖所选三张虚表的三个槽与为说明绘制所需的直接 helper。NPC 完整 type／Mod 闭包、Tear／Projectile 全部继承与变体、Player、Sprite 生命周期与全局 Lua 注册生命周期由主审或其他范围负责。所有未追出 callback 注册／副作用、动态目标、图形资源状态、异常及重入路径保持 `UNKNOWN`；没有用缺少具名 Lua call 反推不存在脚本，也没有把静态分支当成运行观察。

本轮只读 IDB、保存了 `luna-*` 新证据与本子报告；未 patch、改名、`set_type`、保存 IDB 或改生产文件。未运行测试、构建、游戏、probe、实验或调试器。
