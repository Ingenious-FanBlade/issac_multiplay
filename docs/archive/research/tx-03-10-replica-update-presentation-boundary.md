# tx-03-10：Replica 更新分派与展示边界（静态逆向）

日期：2026-10-22（Asia/Shanghai）。GPT-6 Luna / max子智能体研究，主智能体重新取证、纠正并审阅。证据为`STATIC_CONFIRMED`及明确标注的静态推导；运行时行为未新增观察。主审与本轮一次整体静态验收见[复核记录](tx-03-10-primary-review.md)。

## 结论

当前静态证据没有显示出一个现成的“停掉 Replica gameplay、继续绘制 HUD/动画”的单一子函数边界。普通偶数帧分支在 owner 前恢复一批 staged position；owner 里按状态选择普通 Room 更新或特殊状态/FSM 更新；奇数帧另走 Player/Entity helper；两条分支之后仍执行外层 tail。renderer 则从独立 render hook 进入，但 render 子树本身会更新 HUD/动画对象的本地展示状态，并混有 Room/EntityList 的绘制工作。

因此，拦普通 owner call、拦奇数 helper、拦 Entity slot4 或拦 renderer 都各有明显遗漏或损失。renderer 内还会条件性进入 Lua `lua_pcallk`，其 callback 的世界副作用未闭合。更上层的候选是把原版 update dispatch 与独立 render dispatch 分开控制；但要覆盖本任务可见的所有入口，至少需要处理外层 staged restore、普通 owner、奇数 helper、owner 内 FSM 分支和仍保留的外层 tail。静态证据还不能证明这些目标函数不存在其它线程或其它入口。

本报告只讨论循环分派、Room/EntityList 调用、slot4 位置写入，以及 render/HUD/动画关联；不展开 Spawn/Init/Remove、库存结构或同步 apply。

## 目标与证据范围

当前 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，输入路径为同目录 `isaac-ng.exe`，imagebase `0x400000`，IDA banner 为 `Binding of Isaac: Repentance+ v1.9.7.17.J460`。IDB imagebase 处映射 bytes 为 `FF`，未从映射头确认 PE header。另从磁盘 input path 读取到 COFF TimeDateStamp `0x69E6E3A7`、SizeOfImage `0x8FE000`；这是 **FILE_METADATA**，不证明该磁盘文件与当前 IDB 的映射内容相同。未计算或校验哈希。IDA 读取函数、指令、bytes 和 IDB metadata；未运行游戏、构建、测试、probe 或实验；未 patch、rename、保存 IDB，也未控制调试器。

VA/RVA 按 imagebase `0x400000` 换算。下表的 direct-call 目标由 IDA `get_operand_value(ea, 0)` 读取，并保留 MCP `get_bytes` 结果；没有靠手算相对位移推断目标。

IDA恢复的函数类型只作阅读提示，不直接作为未来adapter的完整ABI契约。`*(Game+offset)`表示从字段加载指针；`Game+offset`表示对象内地址，两者不能互换。

| Callsite VA / RVA | 原始机器码 | IDA 解析目标 VA / RVA | 入口 ABI（IDA 恢复）与直接作用 |
|---|---|---|---|
| `0x955231 / 0x555231` | `E8 8A 5B DA FF` | `0x6FADC0 / 0x2FADC0` | owner，IDA: `int __thiscall(char *this)`；ECX 为 Game 对象。 |
| `0x955241 / 0x555241` | `E8 AA 81 DA FF` | `0x6FD3F0 / 0x2FD3F0` | odd helper，IDA: `unsigned int(void)`，从 Game singleton 取数据。 |
| `0x6FBA4D / 0x2FBA4D` | `E8 2E 6F 10 00` | `0x802980 / 0x402980` | 普通 Room 更新入口，ECX加载`*(Game+0x18300)`得到Room指针。 |
| `0x803E70 / 0x403E70` | `E8 4B 48 C1 FF` | `0x4186C0 / 0x186C0` | EntityList 更新；前置指令在 `0x803E62` push 0、在 `0x803E65` 设 ECX 为 Room+0x1218。 |
| `0x6FB51C / 0x2FB51C` | `E8 7F 63 13 00` | `0x8318A0 / 0x4318A0` | `Game+0x1B83C` 状态对象非零时进入特殊状态/FSM 子路径，ECX 为 `Game+0x1B83C`。 |
| `0x831941 / 0x431941` | `E8 7A 6D BE FF` | `0x4186C0 / 0x186C0` | 特殊状态/FSM 子路径也进入 EntityList 更新。 |
| `0x6FBBA5 / 0x2FBBA5` | `E8 F6 4B 16 00` | `0x8607A0 / 0x4607A0` | owner 标准尾部的条件 Lua 分派/引用释放 helper；可能调用 `sub_872980`。 |
| `0x9559C1 / 0x5559C1` | `E8 4A 62 DA FF` | `0x6FBC10 / 0x2FBC10` | render child；由 render owner 调入。 |
| `0x6FC094 / 0x2FC094` | IDA call operand = `sub_80F200` | `0x80F200 / 0x40F200` | renderer 在普通显示分支调用 Room/Entity 渲染路径。 |
| `0x6FC0A1 / 0x2FC0A1` | `E8 AA 67 13 00` | `0x832850 / 0x432850` | renderer 在 Game+0x1B83C 为 2/3 时调用状态/FSM 展示分支。 |
| `0x6FC0A6 / 0x2FC0A6` | `E8 D5 75 16 00` | `0x863680 / 0x463680` | render 内的条件 Lua 分派/引用释放 helper；可能调用 `sub_872980`。 |
| `0x6FC39F / 0x2FC39F` | IDA call operand = `sub_9A3EB0` | `0x9A3EB0 / 0x5A3EB0` | HUD/Player overlay 样式的展示对象更新。 |
| `0x6FC5B7 / 0x2FC5B7` | `E8 D4 AB 2B 00` | `0x9B7190 / 0x5B7190` | 标题、SeedPaper、完成面板等 HUD 绘制。 |

原始 bytes、IDA operand 查询和新增函数反编译输出见 [followup-mcp-evidence.json](../../../artifacts/cs-t0/tx-03-10-static/followup-mcp-evidence.json)；父级汇总的主入口导出见同目录的 `primary-evidence.json`、`primary-call-targets.json`、`primary-leaf-decompilations.json`、`primary-followup-evidence.json` 与 [primary-lua-evidence.json](../../../artifacts/cs-t0/tx-03-10-static/primary-lua-evidence.json)。可重复只读导出的脚本为 [export_tx0310_loop_render.py](../../../artifacts/cs-t0/tx-03-10-static/export_tx0310_loop_render.py)。

## 更新分派：偶数、奇数和外层 tail

主智能体收尾时已执行修订后的只读导出脚本，汇总为[loop-render-export.json](../../../artifacts/cs-t0/tx-03-10-static/loop-render-export.json)；脚本从自身目录选择输出位置，以create-new写入。另见[动画参数原始指令](../../../artifacts/cs-t0/tx-03-10-static/primary-animation-call-context.json)与[EntityList绘制调用参数](../../../artifacts/cs-t0/tx-03-10-static/primary-render-call-context.json)。重复提取可在IDA Python执行namespace中指定`TX0310_EXPORT_OUTPUT`为新文件；这是静态资料提取，不执行目标游戏。

`sub_954CD0`（VA `0x954CD0`, RVA `0x554CD0`, IDA: `int __stdcall(char a1)`）的 EDI 是 Manager（IDA 全局 `0xC7169C`），不是 Game（IDA 全局 `0xC71678`）。它在 `0x9551CF` 检查 `Manager+0x4ABBC` 的奇偶，在 `0x955238` 检查 `Manager+0x2A3C0`；对应十进制偏移为 306108 与 172992。奇数且 Manager+`0x2A3C0` 为 0 时走早退分支；case 2 里，奇数且该字段非零会在 `0x955241` 调 `sub_6FD3F0`。偶数分支则先遍历 Game+`0x18300` 对应列表：当对象 byte +373 非零时，在 `0x955208` 和 `0x955214` 把 +836/+840 staged 值写回 +828/+832，然后在 `0x95521A` 清 +373，之后才于 `0x955231` 调普通 owner。这是 owner 和 odd helper 之外的真实位置写入，不能把拦 owner 等同于不写 position。

owner/odd helper 返回后，外层 case 还继续读取 Room+4600，调用 `sub_6FD350` 做状态判断；根据分支调用 `sub_9446E0` 等路径，并继续完成 case 尾工作。故只拦 `0x955231` 或 `0x955241` 时，外层尾部仍在执行。早退条件、状态标志的精确产品语义仍未知，不据字段偏移猜名字。

## 普通 owner、Room 更新和 FSM 子路径

`sub_6FADC0`（VA `0x6FADC0`, RVA `0x2FADC0`）在函数开始对 Game+429432 调 `sub_68BBA0`；若返回真即早退。否则在 `0x6FAE09` 先调 `sub_708630`，随后进入大段 Game 状态、计时、屏幕/状态处理。IDB 显示 Game+`0x1B83C`（112700）非零时，在 `0x6FB51C` 调 `sub_8318A0(Game+0x1B83C)`，随后调 `sub_98DBA0`、`sub_9A2B30`、`sub_6F0070` 并返回；这个分支不再到普通 Room call。

普通路径在 `0x6FBA4D` 对`*(Game+0x18300)`中的Room调用 `sub_802980`。其直接 callsite `0x803E70` 指向 `sub_4186C0`，即 EntityList 更新函数。该函数会按条件逐实体发出 vtable `+0x0C` 调用（例如 `0x41895C` 处的 Entity 实例调用）；同一 EntityList 更新函数在特殊路径 `sub_8318A0` 的 `0x831941` 再次被调用。即使跳过普通 Room call，特殊 owner 分支仍可运行 EntityList 更新。IDA 的 xrefs 还显示 `sub_8318A0` 有 `0x4212C0/0x429570/0x7475B0/0x92F1C0` 等其他caller函数（均为VA），不能把在 `sub_6FADC0` 单一 callsite 拦截外推为该状态机不再执行。

`sub_6FADC0` 在 Room call 之后还直接调 `sub_9BB5D0`、`sub_4257B0` 并维护 Game timer/计数状态，标准尾部在 `0x6FBBA5` 调 `sub_8607A0`。`sub_8607A0` 在标志满足时于 `0x8607FB` 调 `sub_872980`，随后才调用 `luaL_unref`。render 子树的 `sub_863680` 也在标志满足时于 `0x8636DB` 调同一个 helper，然后释放 registry reference。进一步反编译的 `sub_872980` 在 `0x87299C` 调 `lua_rawgeti` 取 registry 函数，调用两次 `lua_pushinteger`，于 `0x8729CB` 调 `lua_pcallk(v4, 2, 1, 0, 0, 0)`，之后取回结果并用 `luaL_ref` 保存、`lua_settop` 恢复栈。故这两个 helper 不只是 unref：owner tail 与 render child 都有条件 Lua callback 分派。callback 函数本身及其世界状态副作用仍 UNKNOWN；这是真实调用路径，不证明每次调用都进入 Lua，也不证明 callback 必然修改 gameplay。原始函数与三段指令记录见 `primary-lua-evidence.json`。

特殊暂停/FSM owner 路径 `sub_8318A0`（VA `0x8318A0`, RVA `0x4318A0`）不只是状态展示：在 `0x831941` 先调 EntityList 更新 `sub_4186C0`。随后它从 Game+`0x1BAA8` 到 Game+`0x1BAAC` 遍历 Player 指针，当前 Player 在寄存器 EDI；多个状态分支以 `mov ecx, edi` 后调用 `[eax+0x0C]`。其中指令 VA `0x8319C9`、`0x831A11`、`0x831A31` 的 bytes 都是 `FF 50 0C`，即 Player 的 slot3 调用；对应的返回地址 VA 是 `0x8319CC`、`0x831A14`、`0x831A34`，RVA 分别为 `0x4319CC`、`0x431A14`、`0x431A34`。旧文档给的是返回 RVA，不是 VA；这些地址是实际静态 Player 更新线索，不能排除。此路径受 FSM 与 Player 状态条件控制，调用是否每帧发生及派生 slot3 的完整副作用仍未由静态代码证明。

现有 C/S native 局部 wrapper 没有把这些 Player 更新路径整体关闭：`coop_active_runtime.cpp` 中 Player update hook 仍调用 `g_player_update(self)`，`PlayerOnlyUpdate` 仍调用其原函数一次；只有 `Shooting` consumer 在有效 hook owner 范围内且模拟/暂停/生命周期/loading 条件阻断时才跳过原调用，离开 owner 范围仍执行 consumer。这个局部 active-input 分派规则不能外推为 Replica gameplay 已停止，也不切断 Room、EntityList、FSM 或外层 tail。

## 奇数帧 helper、Entity slot4 与位置副作用

`sub_6FD3F0`（VA `0x6FD3F0`, RVA `0x2FD3F0`）也先以 Game+429432 调 pause/state helper。它随后按 Game+0x1B83C、Game+0x1BA78 与对象状态对 EntityList 发出多个动态调用，并在常规 helper 路径再检查 `sub_6FD350`。

此函数中 `Type==1` 的 Entity 在 `0x6FD609` 调 vtable `+0x0C`；`Type!=1` 的分支才在 `0x6FD62D` 调 vtable `+0x10`。这两处是 if/else，不能写成每个 PlayerUpdate 之后都再执行 slot4。前面另有非 Type==1 对象在 `0x6FD4DA`、`0x6FD586` 调 vtable `+0x10`。`0x6FD6D9` 之后的位置直写循环受 Room+`0x7884` 条件控制，内层对象集合来自 Room+`0x776C`，具体集合语义未闭合；它会直接写 Entity +828/+832。最后遍历 Game+`0x1BB78`..Game+`0x1BB7C` 的额外对象向量，在 `0x6FD77D` 调每个对象的 vtable `+0x0C`；该循环没有 Type 检查，也没有独立证据表明所有对象都是 Player。这里不是一个纯 Player-only helper。

Entity 基类 slot4 对应 vtable byte offset `+0x10`，目标 `sub_6B0EB0`（VA `0x6B0EB0`, RVA `0x2B0EB0`, IDA: `void __thiscall(this)`）。当`EntityFlags & 0x2 != 0`时，它清除此位并返回，不执行位置步进。只有未走该早返时，代码才把 Entity+0x33C/+0x340 复制到 staged +0x344/+0x348，置`+0x175`，再读 movement 参数并写回 +0x33C/+0x340。Type==1 路径调用 `sub_6AB440(1)` 并可进入 Player/Grid 碰撞处理；非 Player 路径调用 `sub_6A9C90(1)`。`sub_6AB440` 的直接代码包含 `PlayerCollideWithGrid` 检查、位置校正，也有 `SpawnGridEntity: Grid index invalid` 路径和 GridEntity 构造/Room 槽写入。这里仅把它作为 slot4 的 gameplay 风险证据，不提出改写 vptr 或复刻该逻辑的方案。

**直接观察**：outer staged restore、odd helper 中的 +828 写入、Entity slot4 内 +33C/+340 写入、EntityList update vtable dispatch、Player/Grid 碰撞调用均在本任务范围内可见。

**推导**：slot4 不是“只推位置或只更新动画”的安全展示入口；即使拦掉 helper 中的 slot4 分支，普通 Room/EntityList 和其它状态机 caller 仍可能更新实体，slot4 的派生实现也未全部闭包。

## Render、HUD 和动画调用链

当前 native loop 代码将 `RenderHook` 与 `UpdateHook` 分开：RenderHook 只调用保存的 `g_render`，UpdateHook 仍调用 `g_update` 原函数。render owner `sub_9555C0` 在 `0x9559C1` 调 `sub_6FBC10`。所以 hook 层已经有“渲染入口与原 update 入口分离”的结构，但尚未对原 update 入口实施 Replica-specific 的分派切断。

`sub_6FBC10` 的直接序列先在 `0x6FBC71` 调 `sub_80EA80(Room)`，之后根据 Game+0x1B83C 调 `sub_80F200(Room)` 或 `sub_832850(Game+0x1B83C)`，随后在 `0x6FC0A6` 调 `sub_863680`。后续在 `0x6FC39F` 调 `sub_9A3EB0`，在 `0x6FC5B7` 到 `0x6FC5EE` 调 HUD/展示组件，包括 `sub_9B7190`、`sub_8D2450`、`sub_8EFE10`、`sub_998880`、`sub_858050`、`sub_8CAA30`。`sub_8EFE10` 直接引用 “REVIVE PLAYER” 文本；这些入口支持其 HUD/overlay 用途，但不证明所有深层 callees 纯展示。

`sub_80EA80`（VA `0x80EA80`, RVA `0x40EA80`）是 Room 绘制路线。调用 `sub_419B50` 的指令位于 VA `0x80EAF3`（RVA `0x40EAF3`）；调用前代码将 Room+`0x1204` 的坐标结构地址作为栈参数，并令 ECX=Room+`0x1218`（EntityList）。因此 `sub_419B50`（VA `0x419B50`, RVA `0x19B50`）的 this 是 EntityList，另有坐标栈参数；它遍历该列表中的 Entity，在可见性/阴影条件下对同一 Entity receiver 调 vtable `+0x18` 并传位置参数，然后走 graphics draw call。该 `+0x18` 是 Entity 的另一个方法/ABI，不能按相同 receiver 就混同于 slot3 `+0x0C` 的更新，也不能因它发生在 renderer 下就假定纯展示。随后 `sub_80EA80` 逐项调 `sub_806C20`、`sub_80C810`，处理 Entity/网格的绘制对象。

`sub_9A3EB0` 是 render child 内的 HUD/Player overlay 更新入口。其直接代码会初始化/写入多组 HUD Sprite 状态字段，并在 `0x9A44F4`、`0x9A54C2` 等位置按名字调用 `sub_40A5D0("Idle"/"Destination", 1)`；动画选择 helper 再按名字调用 `sub_408830` 或 `sub_408970`。`sub_408E00` 的 ABI 为 ECX=animation state、XMM1=目标帧；它直接写 `state+0x10 = min(nextafterf(descriptor frame count, 0), target frame)`，并重置 `state+0x18`、调用 `sub_408C90`，再清理/重建 per-layer arrays（`sub_408B90`、`sub_408C10`）。函数内没有以旧帧加 delta 推进的计算，因此只能称为“设定目标动画帧/重算层状态”的候选，不能由此称作逐帧推进器。`sub_40A1B0` 清理 Sprite 本地字段与 layer arrays。这些直接写入不是 Entity world position 的证据；`sub_9A3EB0` 的深层 callees仍有未展开范围，不能将它声明为全闭包纯函数。

`sub_9B7190` 在标题显示状态下按条件写 “Menu Title” 和 “SeedPaper” Sprite 的 active byte `+116`，并走标题/完成面板绘制。它同样会改展示状态。当前 render child 也直接改动若干 Game/render 参数字段（如标记与 post-process 选择）；这些属于可见的可变状态，具体哪些字段必须在 C/S 中复制仍不在本任务范围。

**范围限制**：对 `sub_80F200`、`sub_832850` 及 GPU/效果叶函数只闭合到直接入口和已确认行为。80F200 中看到的 renderable/grid 对象 `+0x0C` 带绘制参数的调用不等于 Entity slot3 Update。未以函数名或旧摘要断言这些路径纯展示；也未完整扫描全部派生类或所有深层调用。

## 切断候选与确定损失

| 候选切点 | 能直接抑制什么 | 已知遗漏/损失 |
|---|---|---|
| UpdateHook 上对原 `g_update` 的整入口分派 | 最强的主循环原版 update 候选；render hook 仍可单独调用 `g_render`。 | 会跳过完整原 update 的其它 manager、case tail、timers、Room/Entity/Player 更新和相关事件处理；其他线程/入口是否存在未知。需要显式 C/S 模式语义，不能借原版 online/pause flags 作身份标志。 |
| `sub_954CD0` case 2 调度块（恢复 staged + 偶数 owner + 奇数 helper） | 可避免当前 outer case 2 的可见 pre-restore、owner、odd helper。 | case2 的外层 tail 是否全部位于块内须按控制流重新定位；当前伪代码显示在 owner/helper 后的 tail 仍执行。其它 Game case/其它 `sub_8318A0` callers 不受影响。 |
| 只跳过 `0x955231 → sub_6FADC0` | 停止该普通 owner 主路径。 | 偶数分支 staged→current 写仍先发生；奇数 helper 与 case tail 仍运行；owner 的其它 callers 仍可到达。丢掉普通 Room/EntityList、Game/timer/常规状态更新和尾部处理。 |
| 只跳过 `0x955241 → sub_6FD3F0` | 停止此 outer odd helper 路径。 | 普通偶数 owner、staged restore 和 outer tail仍运行；丢失 Player slot3 与 helper 内非 Player slot4/位置/碰撞相关逻辑。 |
| 在 helper/EntityList 中拦 Entity slot4 | 局部跳过 slot4 movement/collision calls。 | slot3、Room/FSM 更新、+828 direct writer、其它 caller 与 slot4 派生方法仍可能运行；会丢失与位置、Player/Grid 碰撞和 GridEntity 状态有关的 gameplay。 |
| 停用 `sub_6FBC10` 或整个 `sub_9555C0` | 停止这一条完整绘制链。 | 同时丢失 Room/Entity/HUD/Player overlay、HUD Sprite 动画、标题、后处理与状态转场画面；不适合保留显示。 |

native hook 已有 update/render 分入口，因此可将分入口控制列为后续候选；保留原 render 仍会进入上述 Lua callback，不能据此认定对 Replica 安全。整入口 update 分派、指定分支或独立展示链的候选损失见表。本轮只建立静态分派图，不宣称已有完整、安全的 Replica gameplay/presentation 边界，也不实施修改。

## 未闭合项

- Lua 分派的 wrapper 与 `sub_872980` 已闭合到 `lua_pcallk` 调用点，但 registry 中实际 callback、触发条件在运行时的满足情况及 callback 的 gameplay/world side effects 均 UNKNOWN；其他 `sub_9BB5D0`、`sub_9446E0`、`sub_6FA540` 和 owner/outer-tail 深层调用也未全部追完。
- `sub_80F200`、`sub_832850` 与其 GPU/状态转场叶函数没有全量闭包；已区分直接绘制工作与未知间接工作。
- `sub_6B0EB0` 的 slot4 派生类闭包、所有 EntityList 的其它 caller、线程/异步系统中的 gameplay 入口均未查完。不得把静态未见当成没有。
- **地址纠错记录**：初次分析时把 `docs/archive/hooks/player.md` 中的返回 RVA `0x4319CC/0x431A14/0x431A34` 当成 VA，误读了不相关的 VA `0x431830` 资源函数，并错误排除了 Player 更新路径。该结论已撤回；正确返回 VA 为 `0x8319CC/0x831A14/0x831A34`，其前的 call 指令 VA 为 `0x8319C9/0x831A11/0x831A31`，均为 `FF 50 0C` 且 ECX=Player。先前保存在 `followup-mcp-evidence.json` 中的 `sub_431830` 输出是这次错误地址读取的真实输出，但与本任务的 FSM Player 调用无关，不应用作反证。准确原始指令范围见 `primary-followup-evidence.json`；分析修正也见 [主智能体复核记录](tx-03-10-primary-review.md)。
- 动画边界仍部分 UNKNOWN：`0x408E00` 可见地接受目标帧并重算/重置 layer 状态；从它的子调用是否会进一步读资源或产生其它副作用未闭合。HUD/Sprite 本地可变状态已直接观察，不能因为写入发生在 render 中就把所有 renderer callees都认定为纯展示。
- 静态代码不提供运行时是否实际触发这些分支、是否修改 online/lobby 同步语义的证据。本报告结论不是运行时验证。


