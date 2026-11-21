# tx-03-11：实体绘制与动画事件的逻辑／展示分离

日期：2026-10-22（Asia/Shanghai）。性质：进一步静态逆向；GPT-6 Luna / max 子智能体研究，主智能体独立取证并审阅。延续 [tx-03-10 分派研究](tx-03-10-replica-update-presentation-boundary.md)，本轮目标是拆解 Room／Entity 绘制、状态展示及动画事件依赖，交付可定位的调用与副作用边界，不实现 Replica。

## 结论与工程含义

本轮将原先笼统的 render 子树拆成了不同 Entity 绘制 pass、Sprite 绘制、动画目标帧设置、动画自主推进和事件消费。它们不能用一个“渲染函数”或一个虚槽统一替代。

1. EntityList 的 `+0x18` 与 `+0x14` 是不同的绘制分派，均有坐标参数；`+0x0C` 的 Entity Update 以及 Grid 的同偏移调用不因此改变语义。基类两个绘制入口可直接看到 Sprite／绘制坐标写入，但未证明全部派生实现及下层调用安全。
2. 设置目标动画帧也会设置当前／历史事件位。已追出 `"Shoot"` 事件读取后到 `Type=9`、`Game::Spawn` 的静态路径，因此“客户端只动动画”仍必须与原生 gameplay 消费者隔离。
3. 保留整条原生 render 仍保留条件 Lua 执行；Room／状态展示还会维护遍历游标、Sprite 和图形状态。无直接 gameplay 写入的叶函数可以作为进一步适配候选，不能据此宣称整个 renderer 已成为安全展示层。

由此形成的设计建议是：Authority 复制已裁决的实体与动画展示结果；Replica 的展示时钟及事件播放独立于原生 gameplay 消费者。优先研究支持集合内的 Sprite 绘制或受约束的实体绘制入口，不允许通过 `"Shoot"` 等原生动画事件重做发射结果。这是设计推导，尚无 adapter 实现或运行时安全性结论。

## 证据范围与地址

当前 IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`；输入路径同目录 `isaac-ng.exe`，imagebase `0x00400000`。本轮直接读取当前 IDB；版本横幅及磁盘 PE 元数据的历史限定见 tx-03-10，本轮不把同名输入文件与 IDB 声明为内容相同。未计算或校验哈希。以下均为 x86 静态事实 `STATIC_CONFIRMED`，控制流可达不等于本次已实际发生；未知运行时对象、派生目标与线程保留 `UNKNOWN`。

`RVA = VA - 0x00400000`。对象偏移、虚槽 byte offset、call 指令与返回地址分别表述。IDA 伪代码签名仅辅助阅读，尤其 `__usercall`、被遗漏的 ECX／XMM 参数及优化后的栈帧，不能直接转成可调用 ABI。

## Entity 两条绘制分派

主智能体原始证据：[虚表](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-vtable.json)、[EntityList 常规绘制](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-main-render.json)、[基类叶函数](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-draw-leaves.json)、[另一绘制 pass 与状态展示](../../../artifacts/cs-t0/tx-03-11-static/primary-render-review.json)。子智能体的发现及原始返回见 [Luna 报告](tx-03-11-luna-render-findings.md)。

| 调用链／表项 | 实际 receiver、参数与证据 | 直接可见的状态影响 |
| --- | --- | --- |
| `0x80EAF3 → 0x419B50`，RVA `0x40EAF3 → 0x19B50` | 调用前 ECX=`Room+0x1218`，push `Room+0x1204` 坐标地址。`0x419C61..67` 为当前 Entity 加载 vptr、取 `+0x18`、call EAX。 | EntityList 的该 pass 调度。不能代称所有 Entity 绘制。 |
| Entity vtable VA `0xB6730C`，RVA `0x76730C`，`+0x18 → 0x6B2000` | 表项 bytes `00 20 6B 00`，基类目标 RVA `0x2B2000`；ECX=Entity，栈上传坐标 `float*`。 | 读位置与 Sprite，临时改变选中 layer 的 active byte `+0x74`、Sprite 颜色等字段，经 `0x40A140` 绘制后正常路径恢复；active 最后明确写 0，不是保存任意旧值。未直接写 Entity Position／HP／Room／Grid。 |
| `0x80F200 → 0x4199D0`，RVA `0x40F200 → 0x199D0` | 后者 ECX=EntityList，另有排序阈值及坐标指针；先调 Entity `+0x34`，再在 `0x419A2E`／`0x419B11` 发出 `FF 50 14`。 | 写 EntityList 遍历游标 `+0xA0`。case 5 临时翻转 `Entity+0x350`、写 `+0x34C`，设置全局绘制标志，调用派生 `+0x14` 后正常路径恢复。这里是 PositionOffset，不是 Position `+0x33C/+0x340`。异常或重入恢复未证明。 |
| 基类 `+0x14 → 0x6B10F0`，RVA `0x2B10F0` | 表项 bytes `F0 10 6B 00`；ECX=Entity，栈传坐标指针。先检查 exists `+0x172`、visible `+0x171`。 | 根据 Position、PositionOffset 和屏幕参数计算，直接写 `Entity+0x1A4/+0x1A8` 的绘制坐标，再调 `0x40A0D0(Entity+0x48, …)`。不是只读函数。 |

`0x6B2000` 以及基类 `0x6B10F0` 的直接函数体未出现 Lua、Spawn 或 Entity Position 写入；这只限定这些函数体，不能代替其资源／图形子调用和所有派生 override 的分析。`0x4199D0` 的 `+0x34` 也会动态分派；基类 `0x6B1000` 根据实体层级／索引／Room 条件计算排序值，派生实现仍须分别解释。

主审额外展开 [Sprite helpers](../../../artifacts/cs-t0/tx-03-11-static/primary-sprite-helpers.json)：`0x40B220` 在合法 layer ID 下返回 `Sprite+0x7C` 字段指向数组中的 `160*id` 项；非法 ID 会记录诊断，并可能经线程局部初始化标记构造全局后备对象、注册 `atexit` 后返回后备地址。`0x407170` 直接初始化 11 个 DWORD 的颜色／附加参数结构。因此 layer 查询不能无条件称作只读；副本必须具有有效 layer／资源，不能靠非法 ID 的后备路径假装资源准备成功。该初始化分支是静态可达事实，不是本轮运行观察。

## 动画目标帧、推进与事件位

原始证据：[动画状态叶函数](../../../artifacts/cs-t0/tx-03-11-static/primary-animation.json)、[实际推进及 Entity 调用](../../../artifacts/cs-t0/tx-03-11-static/primary-animation-consumers.json)、[具名事件读取与消费者](../../../artifacts/cs-t0/tx-03-11-static/primary-event-gameplay.json)。下列 offset 以 animation state 为基址，不以 Sprite 或 Entity 为基址。

| VA / RVA | 直接操作和实际参数 | 对分离方案的意义 |
| --- | --- | --- |
| `0x408E00 / 0x8E00` | ECX=state，XMM1=目标帧；写 `+0x10` 为上限裁剪后的帧，清 `+0x18`，调 `0x408C90`，重置／重算两个 layer 索引数组。后两个 helper 的 frame 参数实际在 XMM2。 | 是目标帧设置，不是自主 tick；但也不是无事件状态写入。 |
| `0x408C90 / 0x8C90` | 遍历 descriptor 的 `(event_id, frame)` 项；frame 与 state 当前帧相等时，对 `state+0x18` 与 `state+0x1C` 分别 OR 对应 bit。原指令写点为 `0x408CE1`／`0x408CF0`。 | 本次可见当前事件位与历史累计位。这里只设置状态，未直接执行事件 callback。范围／非法 event_id 的处理不扩充成新契约。 |
| `0x408D00 / 0x8D00` | ECX=state，栈参为目标整数帧；清当前事件位，逐个跨过的整数帧调 `0x408C90`，随后更新 layer 索引。带 `Animation::AdvancePosition cannot go backward` 诊断。 | 前进经过多个事件帧会累计本次推进的事件，不等于直接设定单一目标帧。 |
| `0x409030 / 0x9030` | ECX=state，检查 descriptor 和 playing byte `+0x14`；在 `0x40905E` 将 `state+0x10` 加上 `*(state+0)+0x104` 的 speed，再按长度／循环调用 `0x408D00` 或重置 `0x40A1B0`。 | 这是实际动画推进器；其直接体处理动画状态，不能推导任意调用者的完整纯展示性。 |
| `0x40ADD0 / 0xADD0`、`0x40AE70 / 0xAE70` | ECX=state，栈参为事件名；查名字取得 bit，再于 `0x40AE61` 的 `85 41 18`／`0x40AF01` 的 `85 41 1C` 读取对应 mask。 | 确认这两组位会按名字被原生消费者读取，并非无用途缓存。 |

Entity 基类 Update `0x6AE820` 在条件满足时，`0x6AE8DE`（`E8 4D A7 D5 FF`）以 ECX=`Entity+0x78`、`0x6AE8E9`（`E8 42 A7 D5 FF`）以 ECX=`Entity+0x98` 调 `0x409030`。这也对应 Sprite 位于 `Entity+0x48`、两组 animation state 位于其 `+0x30/+0x50` 的本次可见布局。不能调用整段 Entity Update 来替代单独动画推进；它还含大量世界行为。

`0x40A1B0` 的重置会同时清 `state+0x18/+0x1C` 再计算初帧事件；`0x408E00` 仅清当前位而保留历史位后 OR。原生 Sprite 状态不是“动画名字＋整数帧”两字段就完整，原样保留或恢复它们须解释 playing、分数帧、两个状态、event masks 及 layer 索引。协议是否直接复制这些原生位尚未决定，不把本布局直接升格为 wire schema。

`0x408E00` 在 `0x408E81/0x408EC4` 给 layer helpers 的 XMM2 重新加载原始请求帧，未改成写入 `state+0x10` 的裁剪后值；也未在该体中证明负值／非有限值全部得到拒绝。因此不能从“存在上限裁剪”推导任意网络参数都可安全直接调用。

### 已闭合的动画事件到世界创建路径

```mermaid
flowchart LR
    A[设置或推进动画帧] --> B[408C90 写事件位]
    B --> C[5583D0 读取 Shoot]
    C --> D[6CB2E0 case 0]
    D --> E[428B20 Type 9]
    E --> F[6FE410 Game Spawn]
```

调用者 `0x5583D0` 先检查其 animation descriptor／playing 状态；在 `0x5584A8` 压入 `"Shoot"`，`0x5584B0` 调 `0x40ADD0(Entity+0x78, "Shoot")`，`0x5584B7` 在返回 false 时跳过后续攻击。满足条件后，`0x55867D` 的 `E8 5E 2C 17 00` 调 `0x6CB2E0`；调用前栈传位置、速度、模式 0 及参数对象，ECX 为当前实体。

`0x6CB2E0` 的 mode 0 在 `0x6CB38D` 压 `9`，`0x6CB394`（`E8 87 D7 D5 FF`）调 `0x428B20`，再对结果做 Entity_Projectile RTTI 转换。`0x428B20` 是给末尾参数补 0 的 wrapper，在 `0x428B3D`（`E8 CE 58 2D 00`）实际调 `0x6FE410`。该链的指令与反编译见 [攻击 helper](../../../artifacts/cs-t0/tx-03-11-static/primary-shoot-callee.json)及 [Spawn wrapper](../../../artifacts/cs-t0/tx-03-11-static/primary-spawn-wrapper.json)。

这是有条件的静态可达路径，不声称单独调用帧设置函数必然生成弹体；它要求原生消费者继续执行且其对象／条件有效。足以排除“本地推进共用动画，保留攻击消费者也无影响”的假设。另有 `0x4212C0` 的 `"End"` 分支到 `0x8318A0`，会写 Manager 状态；这些写入的产品语义未全命名，不称作已证实存档副作用。

## Room、状态展示与 Lua 的保留范围

主智能体已独立重读 [Room render](../../../artifacts/cs-t0/tx-03-11-static/primary-room-render.json)与 [FSM／Lua](../../../artifacts/cs-t0/tx-03-11-static/primary-render-review.json)。

`0x80F200` 的直接体包含 Grid 对象带坐标参数的虚调用、EntityList 分 pass 绘制、Room Sprite 的名字选择和目标帧设置、图形状态操作。它会为 EntityList 设置 pass／游标，因此不是无状态 draw；Grid 的 `+0x0C` receiver 与 Entity Update 不同，不能仅按偏移判成 AI。`0x832850` 受状态值 2／3 限制，含画面混合、资源虚槽 `+0x1C` 以及 Sprite 图层的变换／颜色写入，具体外部动态目标和完整下层影响仍 UNKNOWN。两者的下层没有全量闭包。

Luna 另指出 `0x80F200` 的 `+0x14` 调用含多种 receiver，主智能体已独立核对两个例子：`0x810496`（`FF 50 14`）的 ECX 从全局 `0xC7984C` 图形对象加载，不能计入 Entity render；`0x810D0D`（同 bytes）的 ECX 则从 `Game+0x1BB78..+0x1BB7C` 的额外对象向量加载，并 push 坐标参数。后者实际动态类型和 target UNKNOWN，不能因普通 EntityList 的基类目标已知就用同一个函数替换。

已重新确认 `0x6FC0A6 → 0x863680 → 0x872980` 条件链。`0x863680` 检查标志 `&4`，`0x872980` 先 `lua_rawgeti` 获取 registry 函数，压两个 integer 后在 `0x8729CB` 调 `lua_pcallk`，随后 ref／unref／settop。它不是仅释放引用，也不是每次 render 都执行 Lua。registry 中实际 dispatcher／callback、用户 Mod 的绑定、回调世界副作用以及重入行为仍 UNKNOWN；不能将其写成已证实“任意／无限制用户 Lua 执行”。仅屏蔽这一处也不证明不存在各派生 render 内的其它脚本入口。

## 可供后续实现使用的边界表

| 候选操作 | 本轮确认 | 实际损失／剩余工作 |
| --- | --- | --- |
| 继续 tx-03-10 的 Replica-specific update 分派隔离候选 | 必须同时处理 driver、odd／FSM、tail 及其它入口；没有新增可直接启用的完整切点。 | 本轮不实现更新阻断，也不依赖原版 online／pause flag 表示 C/S 角色。 |
| 对支持的展示对象设置指定动画／目标帧 | 帧设置、两组 state、事件位与 layer 重算边界更明确。 | 必须避免残存 gameplay 消费者读取这些位；资源生命周期、有限值／范围、异常、复用与主线程上下文未形成 apply 契约。 |
| 本地调用 `0x409030` 自主推进 | 有真实 speed／playing／loop 证据。 | 会改变事件状态；默认不据此允许客户端驱动武器、AI、Grid 或回调。展示时间及丢包恢复需另设计。 |
| 直接 Sprite 绘制 `0x40A0D0/0x40A140` | 基类 `+0x14/+0x18` 最终可达这些入口；本轮也取得直接包装层代码。 | 可能损失派生类自定义绘制、Player 身体／装备、特效、阴影等；资源、图形后端和生命周期未闭合，尚不能直接实现通用替代器。 |
| 保留完整 Entity／Room／FSM render | 可复用更多原版画面。 | 派生虚函数、Lua、临时对象写入及 pass 时序仍需处理；不能标记为已证纯展示。 |

## 交付与未决项

本轮完成的是有限目标的静态拆解与主审，未完成的是完整安全展示闭包：派生 `+0x14/+0x18/+0x34`、Grid／资源动态目标、脚本绑定与重入、Sprite 构造／资源加载／释放、Player 组合外观、停止 update 后 render 所需缓存与 HUD 的完整依赖。下一项具体研究宜选首版支持类的派生 render 和 Sprite 资源生命周期，建立可被单独维护的最小展示对象集合。

可重复只读提取脚本为 [export_primary_static.py](../../../artifacts/cs-t0/tx-03-11-static/export_primary_static.py)；通过 IDA MCP `py_exec_file` 执行默认导出。重提取须以 `runpy.run_path(..., init_globals=...)` 指定新的 `TX0311_OUTPUT`，并用相应 JSON 的 `requested_va`、ranges 和 tables 设置 `TX0311_FUNCTIONS/TX0311_RANGES/TX0311_TABLES`；采用 create-new，不覆盖原始观察。主智能体两次大响应发生工具传输截断，后改为文件导出完整伪代码／逐指令数据；截断未作为全量分析依据。一次 `runpy` 调用还返回了过大的 namespace，后改为赋值避免打印，与目标执行无关。

本轮不修改生产代码和 IDB，不启动／控制游戏或调试器，不构建或运行代码测试。主智能体对 Luna 结论的接受、限定与纠正以及唯一一次整体静态文档验收，见 [主审记录](tx-03-11-primary-review.md)。
