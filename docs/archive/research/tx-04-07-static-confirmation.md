# tx-04-07 静态确认：ST1–ST7

日期：2026-10-25；2026-10-26 追加 ST2 主审。状态：**ST1–ST7 静态取证与主审完成，保留 UNKNOWN**。第 1–6 节保留 ST1 研究，第 7–10 节为 ST5–ST7，第 11–14 节为 ST3、ST4；第 15–18 节追加 ST2 的重新取证、材料更正与实现影响。此前各节的待主审表述保留当时语境，以第 18 节为当前 ST2 状态。范围对应[实现方案](../design/tx-04-07-implementation-plan.md)第 10 节。本次静态审核未修改生产代码、构建、启动游戏或执行测试，也未消耗 tx-04-07 的整体 E2E；工作区已有编码的进度另见实现方案，不由本审核验收。

前序研究按用户指定由 GPT-6 Luna / max 子智能体使用 IDA MCP 逆向，主智能体另行读取 IDB 的反编译、汇编、vtable 和调用引用审阅。2026-10-26 本轮由主智能体审核既有 ST2 材料，直接回读当前 IDB 与官方源码，没有另建子智能体。下列为静态事实与有条件推导，不是运行时观察；适用[现行规范](../refactor/project-conventions.md)。

## 1. 目标与证据

- IDB：`scratchpad/v12/isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`；完整输入/数据库路径保存在[主审元数据](../../../artifacts/repentogon/tx-04-07-static/st1-review/metadata.json)。imagebase=`0x400000`。
- IDB 字符串 VA `0xB2EEA0` 为 `Binding of Isaac: Repentance+ v1.9.7.12.J273`，VA `0xB2BDB8` 的转储文件名也含该版本；见[补充元数据](../../../artifacts/repentogon/tx-04-07-static/st1-review/supplement-details.json)。没有校验哈希。
- 官方源码：`F:/projects_isaac/REPENTOGON-1.1.3`；[带原行号摘录](../../../artifacts/repentogon/tx-04-07-static/st1-review/source-excerpts.md)。`.zhl` 和既有 tx-04-03 点位仅用于定位，结论重新读取当前 IDB；官方 Hook 的行为单独标作 `SOURCE`。
- [原始导出索引](../../../artifacts/repentogon/tx-04-07-static/README.md)：子智能体与主审的材料分开保存。主审 JSON 保留反编译、逐条 VA/RVA、指令字节及 xref。自动变量名、推断原型不作为 ABI 证据。

下文地址均先列 **RVA**，必要时附 VA；换算为 `VA=0x400000+RVA`。

## 2. ST1 答案与适用条件

这里的正常路径指：A 健康，A/B 都是独立的完整 `Entity Type=1/Variant=0` Player，B 为本轮 S10 的普通 Isaac（`playerType=0`），无阻止死亡或自动复活的效果；`Game+0x25B8C<2`，不进入下文另列的间接分派；没有第三方 gameplay 回调改写对象或死亡结果。这些是静态分支的适用条件，实际场景值仍待整体 E2E 记录。

| 问题 | 静态结论 |
| --- | --- |
| B 原生死亡会进入 MorphToCoopGhost 吗 | 会。Kill 标 dead，Player 原生更新处理死亡动画，PlayerManager 在动画结束后调用 `TriggerDeath(false)`。没有效果抢先复活时，共同尾在 co-op、独立非 ghost Player 数大于 1 等条件成立后调用 Morph |
| Player 是否被移除、人数是否变化 | 该正常 ghost 路径不移除 B，不重新 Spawn/Join；原 Player 仍在 PlayerManager vector，列表人数预期保持 2。原生的“非 ghost 完整 Player 数”会变成 1，它不是 vector.size |
| dead/ghost 如何变化 | `dead=1` 是前段死亡状态；Morph 先调用 `Revive` 清 dead，后置 `ghost=1`。正常转换完成后预期 `dead=0, ghost=1`。因此 Lost 必须识别 `dead || ghost` |
| controller 与 K | 已读的普通死亡、Revive 和 Morph 局部分支不改 `Player+0x160C` controller、不重建/移除 K、不重新分配 Player index。不能把这个局部结论扩大为任意角色、任意官方/第三方回调的设备闭包 |
| 设备通知是否完全排除 | `UNKNOWN`。普通局部分支未识别到通用设备注册/移除/重分配入口；条件性间接调用及官方回调的完整通知闭包未确认，不能断言整个死亡窗口没有通知 |
| HUD | ghost Player 没有因此从列表/HUD slot 退役；`PlayerHUD::RenderHearts` 读 `+0x207D` 后直接返回，跳过常规心容器绘制。Morph 另切 ghost Sprite、更新外观与道具缓存，并清理归属实体 |
| 死亡计数 | 正常 ghost 分支绕过 `IncreaseEventCounter(10,1)`；不是“B 一被 Kill 就计死亡次数”。失败进入原生终局分支时可执行该计数并转 Game 结束状态 |
| 能否承诺无持久写入 | 不能。道具重算具有条件性 TryUnlock 路径，官方死亡/实体回调也有自己的条件；终局路径还有 run/PGD bookkeeping。ST7 仍需核对完整观测覆盖 |
| 夹具选择 | 采用 `B:Kill()`，然后等待原生动画与 PlayerManager 分派到 ghost。它使用原生致死入口，能覆盖本轮死亡→ghost→后续复活目标。致命伤害还会增加免疫、护盾、伤害回调和 TryPreventDeath 条件，不是本轮必须覆盖的入口 |

以上列表人数、controller 与状态的最终实际值仍由 tx-04-07 的一次整体 E2E 记录，静态结果不冒充实机成功。

## 3. 原生死亡链与分支

### 3.1 Kill 与致命伤害汇合

`STATIC_CONFIRMED`：原生 Lua 注册点 RVA `0x44F6D2/0x44F6D7` 将函数 VA `0x844230` 与字符串 `Kill` 关联。该函数构造空 `EntityRef`，在 RVA `0x44428D` 经实体 vtable `+0x24` 调用；这是选择 `B:Kill()` 的实际绑定依据，不只是依据 API 名称推测。

`STATIC_CONFIRMED`：Player vtable RVA `0x71BDB4`（VA `0xB1BDB4`）第 9 槽即 `+0x24` 指向 `Entity::Kill` RVA `0x2A7ED0`。Kill 的 `0x2A7EEA` 写 `Player+0x173=1`，最后于 `0x2A7FE9` 调实体死亡回调入口 `0x44CB30`。它本身不调用 Morph，也不删除 PlayerManager vector 项。

`Entity_Player::TakeDamage` RVA `0x36B900` 的致死判定可在 `0x370BB4` 调 `TryPreventDeath`，并在 `0x370C25` 经 vtable `+0x24` 调到相同 Kill。伤害成功、健康扣减、特殊效果与 `TryPreventDeath` 的条件不能由传入“大伤害”自动保证。`Kill()` 覆盖原生死亡处理，不覆盖伤害入口本身的语义。

原始证据：[Lua Kill](../../../artifacts/repentogon/tx-04-07-static/st1-review/Lua_EntityKill.json)、[Kill](../../../artifacts/repentogon/tx-04-07-static/st1-review/Entity_Kill.json)、[TakeDamage](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_TakeDamage.json)、[注册点与 vtable 实值](../../../artifacts/repentogon/tx-04-07-static/st1-review/supplement-details.json)。

### 3.2 动画结束后的 PlayerManager 分派

`STATIC_CONFIRMED`：`PlayerManager::Update` RVA `0x592650` 枚举真实 Player vector；`0x5926CC` 查 `dead +0x173`，`0x5926D5–0x5926FB` 查当前动画及关联 twin 的动画，符合条件后 `0x5926FD` 压入 `false`，`0x5926FF` 调 `TriggerDeath` RVA `0x3992A0`。

`TriggerDeath` 先处理 Variant=1、twin、自动复活/效果分支，再进入共同尾。其临时 vector 位于全局 VA `0xBEE6D0/+4/+8`，用于当前死亡组；不要将该临时 vector 当成 PlayerManager 的列表，也不要因反编译标签位置把共同尾误读为“只有消耗复活道具才能 Morph”。

关键共同尾（RVA）：

| 指令点 | 检查/调用 |
| --- | --- |
| `0x399586` | 当前死亡组首对象 `+0x173` 仍为 dead；自动复活成功后不会满足 |
| `0x39958F` | `+0x3BC` parent 为空；附属 Baby/possessor 另有分支 |
| `0x3995A8` | `PlayerManager::IsCoopPlay` RVA `0x596990` |
| `0x3995B5–0x3995BD` | helper RVA `0x596A00` 计算独立、完整、非 ghost 的 Player 数，并要求大于 1；它没有排除尚未 Morph 的 dead B |
| `0x3995BF` | 首对象尚非 ghost（`+0x207D=0`） |
| `0x3995D0–0x3995E4` | 枚举当前死亡组，以 ECX 传每个 Player，于 `0x3995D6` 调 Morph RVA `0x3CFFA0` |

因此 B 单独死亡、A 健康时，A 与尚未 Morph 的 B 仍计为两个非 ghost 完整 Player，可以进入转换。并非“只有非 Main 能变 ghost”的索引硬判断；结论依赖当前拓扑和状态条件。

原始证据：[PlayerManager Update](../../../artifacts/repentogon/tx-04-07-static/st1-review/PlayerManager_Update.json)、[TriggerDeath](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_TriggerDeath.json)、[IsCoopPlay](../../../artifacts/repentogon/tx-04-07-static/st1-review/PlayerManager_IsCoopPlay.json)、[非 ghost 计数](../../../artifacts/repentogon/tx-04-07-static/st1-review/PlayerManager_CountNonGhost.json)。

### 3.3 Morph 对对象、controller 与实体的动作

`STATIC_CONFIRMED`：Morph RVA `0x3CFFA0` 对原 ECX 对象操作：

- `0x3D0021 → 0x39A400` 调 `Revive`，后者 `0x39A41C` 清 dead，并恢复最小健康/冷却相关状态；这不是 ST2 的 `RevivePlayerGhost`。
- `0x3D004D` 的 `RemoveNullEffect(112,1)` 是临时效果移除，不是持久 event 112。
- 保存/清理 active item 和 pocket item 描述，切 ghost 图形，播放 `GhostAppear`。
- `0x3D02C0` 写 `ghost +0x207D=1`；`0x3D02CE` 写 visible `+0x171=1`；`0x3D02D5` 调 `EvaluateItems` RVA `0x35D4B0`。
- `0x3D0322`、`0x3D0371` 对归属该 Player 的实体走虚表 `+0x28` Remove；第二组查询为 Type=3/Variant=237。被清理的是归属实体，不能将这些虚调用记成 B 被移出 PlayerManager。

在普通 Isaac 路径中，没有写 controller `+0x160C` 或 Player index `+0x1610`，也未见写 valid `+0x170`、exists `+0x172`、removed `+0x177`。这些值沿用进入前对象，不应把“未写”描述成“该函数无条件设置为健康”。

**限定。** Morph 的角色 35 分支在 `0x3CFFFC` 可移除关联 twin，角色 40 可提前返回；不能推广为所有角色都保持人数。另有 `Game+0x25B8C>=2` 条件下，`0x3D006A` 经 `[Game+0x25B98]` 的 vtable `+0x60` 调用；不能把未解析的间接边写成“从不通知任何子系统”。本次正常分支没有发现 InputManager 注册/移除、K 退役、控制器重分配或通用设备通知入口；K 的普通 Poll 仍可独立发生。

原始证据：[Morph](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_MorphToCoopGhost.json)、[Revive](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_Revive.json)、[ghost 图形 helper](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_LoadGhostSprite.json)。

## 4. HUD 与持久副作用

### 4.1 HUD

`STATIC_CONFIRMED`：`PlayerHUD::RenderHearts` RVA `0x434930` 在 `0x434BBD` 读所绑定 Player 的 `+0x207D`。为真时 `0x434BC6–0x434BDF` 返回布局结果，绕过后续常规心容器绘制。此证据说明 ghost 的常规生命 HUD 行为，不声称所有 HUD 内容均隐藏或某种最终像素画面已经观察到。

原始证据：[RenderHearts](../../../artifacts/repentogon/tx-04-07-static/st1-review/PlayerHUD_RenderHearts.json)。

### 4.2 死亡次数有明确的分支条件

`STATIC_CONFIRMED`：`TriggerDeath` 的 `0x3995ED` 再次判断 dead。正常 Morph 内 Revive 已清 dead，所以不会进入 `0x39A141 → 0x593120` 的终局死亡处理。

该 helper RVA `0x593120` 在 `0x593131` 分 Variant：完整 Variant0 分支在 `0x593144` 写 manager `+0x7C`，`0x593147` 调 `IncreaseEventCounter(10,1)`，最后 `0x5931A7` 进入 Game 状态切换；非零 Variant 则有 `0x5931FF → RemoveCoPlayer`。官方 `AchievementsStuff.h:37` 将事件 10 命名为 `DEATHS`。`IncreaseEventCounter` RVA `0x503A50` 在 readonly=false 时写计数并置 dirty。

故只能说本轮正常 ghost 路径绕过这次死亡计数，不能声称“死亡计数从不改”或“任何非 Main 死亡都不会结束 run”。全员死亡、co-op 条件不满足、对象/角色异常或回调改写时必须按实际分支处理。

原始证据：[终局死亡 helper](../../../artifacts/repentogon/tx-04-07-static/st1-review/PlayerManager_DeathHandler.json)、[PGD 计数入口](../../../artifacts/repentogon/tx-04-07-static/st1-review/Persistent_IncreaseEventCounter.json)。[补充线索导出](../../../artifacts/repentogon/tx-04-07-static/st1-review/supplement-details.json)的“直接 xref 前十二条找 push 10”只用来定位，未据此声称全映像间接调用闭包。

### 4.3 仍存在条件性写入与官方回调

- Morph 的道具重算可到 `EvaluateItems` 中的 TryUnlock：例如 RVA `0x367520`（ID 144）、`0x367637`（19）、`0x3679B6/0x3679CD`（58/370）、`0x368610`（355）、`0x368EBD`（361）。它们分别受 cache flags、对象 valid/Variant、道具/效果/阈值等条件影响，不代表本轮普通 Isaac 一定触发或成功解锁。见[EvaluateItems](../../../artifacts/repentogon/tx-04-07-static/st1-review/Player_EvaluateItems.json)。
- 原生死亡动画初始化还有“没有其他可继续的 Player”等条件下的 `0x5324A0/0x533C10`。后者有 readonly、统计与 run bookkeeping 的条件路径；正常 A 健康时不能将这些终局副作用记成已发生。见[run 状态](../../../artifacts/repentogon/tx-04-07-static/st1-review/Manager_DeathRunState.json)、[bookkeeping](../../../artifacts/repentogon/tx-04-07-static/st1-review/Manager_DeathRunBookkeeping.json)。
- `SOURCE`：官方 `CustomCallbacks.cpp:6500–6522` 包装 Entity Kill / MC_POST_ENTITY_KILL（68）；`ExtraLives.cpp:30–37,63–136,190` 包装 PlayerManager Update、TriggerDeath，并加入 1050/1051 回调和复活处理；`ItemSpoofSystem.cpp:346–354` 在 TriggerDeath 后整理角色相关 item spoof。官方 Hook 不在未加载的原生 IDB 中，不能伪装成已反编译的运行时代码。
- `SOURCE`：官方 `AchievementsStuff.cpp:248–257` 在 PGD 计数入口先运行事件 tracker；原生 PGD 入口观测并不自动等于所有官方 tracker/JSON/回调写入均已覆盖。ST7 再决定完整观测清单。

现有 ST8 的 `pgd_counter/pgd_unlock/pgd_local/pgd_cloud/button_maps` 观测是可复用入口，但当前 `PersistenceOperation` 只围住同步调用。`Kill()` 返回后若跨多个 loop 才完成 ghost，后续写入不会自动继承 `b_death` 标签；实现时需保留死亡请求至 ghost/失败的窗口关联。这里只记录需求，没有修改观测代码或翻转 readonly。

## 5. 对实现方案的落实

1. `kill_b` 选 `B:Kill()`，对正确 ordinal/Type/Variant 的 B 发起一次请求；等待自然原生 ghost，而非直接 Morph/写字段。动作结果必须区分已请求、dead、ghost 到达、意外自动复活、移除、超时，不因 Kill 已返回就记死亡场景成功。
2. 正常死亡路径保留当前 Player 列表人数为 2 的规则；Lost 不能要求列表人数变成 1，也不能只检查 dead。B 的失效、中性与新输入丢弃继续按方案第 5 节。
3. 死亡不应触发第二次 K 创建、Join 或 RemoveCoPlayer。普通 B 死亡保持 controller 的静态证据不能代替 ST2 对 `RevivePlayerGhost` 的重新核对。
4. 本次只排除 R1 在普通 Isaac 死亡→ghost 路径中的原生移除；R2 和复活后的身份由 ST2 确认。因此不独自决定 U10/PeerLost 的最终实现。
5. ST7 保留死亡窗口的持久入口/条件核对；本报告没有将“正常分支绕过死亡计数”提升为“死亡窗口无持久副作用”。

## 6. 主智能体审阅与未决项

主审已独立读取第 3、4 节的核心函数、Player vtable 第 9 槽、Morph 分支原始汇编及计数 helper，并保留[可重复导出的脚本](../../../artifacts/repentogon/tx-04-07-static/st1-review/export_review.py)与[补充脚本](../../../artifacts/repentogon/tx-04-07-static/st1-review/export_supplement.py)。子智能体提交最终结论后，主审逐项对照：

| 子智能体结论 | 主审处理与证据 |
| --- | --- |
| Kill 进入原生死亡链，夹具优先采用 Kill | 采纳；另外核对 Lua 注册点、空 EntityRef 和 Player vtable 实值，闭合第 3.1 节 |
| 由 Manager Update 后续处理 | 采纳并收紧时序：须等死亡动画与关联 twin 动画条件满足，**不保证 Kill 后下一次 Manager Update 即完成分派**。第 3.2 节给逐条指令 |
| 普通 B 原对象变 ghost、名单人数不变、controller 局部不变 | 有条件采纳；区分临时死亡组、真实 Player vector 与非 ghost 计数；以第 3.3 节限定普通 Isaac 和回调边界 |
| 最后回传中的“playerType=1/variant=0” | 更正字段混用：完整 Player 是 `Entity Type=1/Variant=0`；普通 Isaac 的专用角色 `playerType=0`。不把实体 Type 和角色字段混为一谈 |
| 存在“Type=35”特殊清理例外 | 文字更正为 **playerType=35**，不是 Entity Type=35；IDB 实际读取 `+0x13BC` 角色字段，Entity Type 仍为 1 |
| 落盘笔记称 `0x7995C8–0x7995CE`“排除 Main” | **不采纳，明确纠正**：`cmp esi,dword_BEE6D4; jz loc_7995E6` 比较临时死亡组的迭代指针与 end。它不是 Player/ordinal 比较，也不排除 Main；主审原始汇编见 TriggerDeath JSON。笔记中的 dead 检查地址也更正为 VA `0x9926CC`，不是指令中间的 `0x9926D0` |
| HUD 不绘普通心；普通 ghost 不必增加 DEATHS | 采纳；主审另读 RenderHearts 分支、终局 handler 与 PGD 写入指令，见第 4 节 |
| 无零持久副作用证明；设备通知总体 UNKNOWN | 采纳；保留条件性 TryUnlock、官方 Hook、跨 loop 窗口关联及间接调用边界，不伪造完整闭包 |

[子智能体笔记](tx-04-07-st1-agent-notes.md)及其[六个核心函数原始导出](../../../artifacts/repentogon/tx-04-07-static/st1-agent/ida-core-functions.json)已逐项阅读；主审另取的第 3、4 节函数与上述控制流相符。原笔记保留其证据与措辞，顶部注明实际发现的误读；结论以本报告审阅后的条件和地址为准。

保留 `UNKNOWN/未观察`：官方 Hook 实际排列、回调实际启用状态、实际死亡动画耗时和观测时序、所有特殊角色/自定义复活效果、间接调用/道具重算的完整副作用闭包，以及复活端 ST2。原 ST1 完成时 ST3–ST7 尚未开始；本轮 ST5–ST7 的追加结果见下文。没有把原型点位或历史成功样本当成本版本运行结论。

## 7. ST5–ST7 的分工与主审证据

用户要求每项各用一个 GPT-6 Luna / max 子智能体逆向，主智能体审阅。上一轮分别使用 `st5_reverse`、`st6_reverse`、`st7_reverse`，每项的原始材料分别保存于 [st5-agent](../../../artifacts/repentogon/tx-04-07-static/st5-agent/notes.md)、[st6-agent](../../../artifacts/repentogon/tx-04-07-static/st6-agent/notes.md)、[st7-agent](../../../artifacts/repentogon/tx-04-07-static/st7-agent/notes.md)。它们没有修改公共报告或生产代码。

主智能体另经 IDA MCP 采集 [38 个完整函数、逐条指令与 xref](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/metadata.json)、[Player vtable 实值](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/player-vtable.json)、[候选角色表摘录](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/stock-role-candidates.json)，并阅读[本工程与官方源码摘录](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/source-excerpts.md)。[导出脚本与边界说明](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/README.md)可重复使用。目标仍是 v1.9.7.12、base=`0x400000`；以下地址先列 RVA。

静态资料读取不是测试；本轮未建立新的推进编号、未运行验收入口，也没有哈希、gate 或动态调试。

## 8. ST5：Baby 位、换层与 Continue

### 8.1 三处 predicate 与置位点

`STATIC_CONFIRMED`：`CoopBabiesOnly` RVA `0x5910A0` 从全局 VA `0xBE49F4` 的 Game 指针读取 `Game+0x25AC0`，返回低 dword 的 `bit 4 / mask 0x10`。入口代码 xref 正好是 Join pass 的 `0x5915AE`、`0x59174C`、`0x591E67`。三个调用读取的是同一位；K 的三个 override 继续只改当前调用返回值，不写 Game flags。

`Level::ChangeRoom 0x339DE0` 在 `0x339E60` 执行 `or dword ptr [ecx+25AC0h],10h`。这是函数前段顺序执行的置位点。`Level::Init 0x33EB10` 在 `0x33EB5E/0x33EB6E` 原样读回/写回这个低 dword，`0x33EB64` 清的是相邻 `+0x25AC4` 的 `0x1000`；`Level::SetNextStage` 的 `0x3410A2` 清的是低 dword `0x80000`，都不是 Baby 位。原始证据见主审 [predicate](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/PlayerManager_CoopBabiesOnly.json)、[ChangeRoom](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Level_ChangeRoom.json)、[Init](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Level_Init.json)、[SetNextStage](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Level_SetNextStage.json)。

`STATIC_INFERRED`：S9 已正常切房置位、S12 采用普通换层且没有特殊 reset/回调改写时，后续 Join 的原值预期仍为 1。有限函数与直接偏移访问扫描不证明任意别名/间接回调绝不会清位；实际三处原值仍由整体 E2E 记录。

### 8.2 Continue 恢复保存值，不无条件清零

`STATIC_CONFIRMED`：`ExecuteStartGame` 的 `0x532AE0 → 0x2F0AF0 → 0x2F0B36 → 0x2F0770 → 0x2F07A1 → ResetState 0x2F08D0` 启动准备链会在 `0x2F0977` 清零 `Game+0x25AC0`。这里 `0x2F0AF0` 使用中性描述“启动准备”，不是官方构造函数；完整构造签名匹配的是 `0x2ED4B0`。

随后 Continue 分支于 `0x532E7E` 调 `Game::RestoreState(startGame=true)`。`SaveState` 在 `0x2F5286` 设置 53 个 flag 字节，`0x2F52B4/0x2F52D0` 逐位存入 `GameState+0xE+index`；Baby 位是 index 4，即 `GameState+0x12`。Restore 的 `0x2F4477` 读取保存字节，经 `0x2F44AE–0x2F44D4` 对相应位 OR 或 AND 后写回。**构造/准备阶段的清零会被恢复覆盖；最终 Baby 值由存档中的 bit 4 决定。** 见主审 [启动准备](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Game_ExecuteStartPreparation.json)、[ExecuteStartGame](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Manager_ExecuteStartGame.json)、[SaveState](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Game_SaveState.json)、[RestoreState](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Game_RestoreState.json)。

因此本方案 S19 保存发生在切房/换层之后且该位仍为 1 时，S20 Continue 后的 Join 也预期为 1；如果保存位为 0，则预期为 0。不能按“Continue 后第一个房间”直接预期 0。

特殊 helper `0x303460` 也在 `0x303473` 调 ResetState；代理另将其 thunk 的登记关联到 `case 636 / COLLECTIBLE_R_KEY`。这是特殊重开线索，不是普通换层的清位依据；其间接调用消费者闭包仍为 `UNKNOWN`，见代理笔记。主审另取的 ResetState/特殊 helper 与其指令相符。

## 9. ST6：角色 7、初始化与门切换射击

### 9.1 角色表存在条件化解锁筛选

`STATIC_CONFIRMED`：`PlayerManager` 角色表构建函数 `0x594CD0` 在 `0x594E85` 选取 `this+0x70` 的 int32 vector，先追加 `-1`，再遍历原生候选表 VA `0xB2F8F0`；第 7 项为 PlayerType 7。每项从对应 `EntityConfig_Player+0xE8` 读取 achievement，在 `0x594F01–0x594F49` 决定是否 append：负值/0、1..640 已解锁可通过，另有 Manager state=2 且 daily/debug 的例外；stock 循环对 achievement≥641 跳过；9/30 还有 token 条件，不能泛化给 7。最终实际加入仍由现有 `CharacterAvailable` 按值检查。

`SOURCE`：官方结构确认 `+0xE8` 为 achievement、Game `+0x25BA8` 为 daily、`+0x25B01` 为 debug；`ASMPlayerManager.cpp:20–40` 的额外成就筛选补丁作用于后面的 modded-ID 循环，不是 stock 7 的循环。当前静态资料没有读取本次 XML 配置中 role 7 的具体 achievement 值及当前存档表内容，因此不声称“所有锁定存档都必然排除 7”，也不把用户已确认解锁变成静态运行观察。见[构建函数](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/PlayerManager_BuildRoleTable.json)与源码摘录。

### 9.2 Init 的条件与附属 Player 边界

`STATIC_CONFIRMED`：`Player::Init 0x34F6A0` 的 `0x350EBC cmp eax,7` 来自 `Player+0x1AD8`（Baby skin），外层还要求 Variant=1，**不是角色 7 分支**。专用 PlayerType 字段为 `+0x13BC`。Init 的显式专用角色分支、`InitPreLevelInitStats 0x3B2C90` 与 `InitPostLevelInitStats 0x3B3980` 的已读角色分支中，没有识别到 role 7 专用的附属 Player 创建；其他角色确有不同路径，不能外推“所有角色 Init 都不创建子 Player”。

`STATIC_INFERRED`：本轮角色 7 沿普通完整 Player 和配置驱动的初始化路径，可继续要求 Join 后恰好两个完整 A/B。角色配置、初始物品、通用 helper 与官方回调仍可能影响属性和实体；“未见 role 7 专用子 Player 分支”不等于完整实体/回调闭包无创建、无副作用。`UNKNOWN` 保留通用 helper 闭包、运行时配置和 Hook 次序。见主审 [Init](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Player_Init.json)、[InitPre](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Player_InitPreLevelInitStats.json)、[InitPost](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Player_InitPostLevelInitStats.json)与代理笔记。

### 9.3 门 owner 的 ordinal 1 暂停已有静态路径

`STATIC_CONFIRMED`：`RoomTransition::Update 0x426430` 的 `0x426557/0x4265A2/0x4265C5` 等位置对 Player 作 vtable `+0xC` 调用；Player vtable VA `0xB1BDB4` 的该槽静态实值为 `0x77BD40`（Player::Update）。Player 原生 `0x37DE1D` 直接调用 Shooting consumer `0x373990`。见主审 [Door](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/RoomTransition_Update.json)、[Player](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Player_Update.json)和 vtable 证据。

`SOURCE`：`hooks.json` 给 Door/NestedDoor 建 owner；`CallScope` 跨 original 保持该 owner，`InBlockedContext` 包括两者。`ControlledOrdinal` 对 A/B 对称识别，`ConsumerScope` 的 Shooting 分支要求有效 tick、未阻断、绑定一致和健康上下文；生成的 shoot wrapper 在 `CallOriginal=false` 时直接返回。因此在相关 Hook 安装、Armed 且 B 被识别、调用到达该 consumer 的前提下，门过渡中的 ordinal 1 也会被主动暂停。它不要求 Player::Update 必须从普通 Room owner 进入。

这只确认静态调用和暂停逻辑，不是 S15/V12 实机通过。官方回调跳过 Player 子类 body 时可能根本不到达 consumer；实际 Hook 链、蓄力、松键后的激光归属和窗口原调用次数仍需整体 E2E。继续保留原生 Player/其他 consumer 调用，仅暂停 Shooting，不改蓄力字段。

## 10. ST7：持久观测覆盖、缺口与主审结论

### 10.1 五个现有入口只提供局部观测

`SOURCE`：现有清单仍是 `pgd_unlock 0x503930`、`pgd_counter 0x503A50`、`pgd_local 0x503460`、`pgd_cloud 0x502F00`、`button_maps 0x42AC90`。`PersistenceScope` 记录字段前后值、dirty/readonly/loaded 和正常返回标记；**不记录 TryUnlock 的 bool 返回值，也不直接记录文件写入成功**。合法范围外的 achievement/counter 为 `known=0`，并不因此证明相应官方扩展没有写入。

| 窗口 | 已定位的入口与条件 | 当前覆盖结论 |
| --- | --- | --- |
| B 死亡 | ST1 正常 ghost 绕过终局 `DEATHS+1`；终局 helper 和 Morph 道具重算仍有条件 PGD 调用 | 原生 counter/unlock 入口可记录；没有 `b_death` 窗口归属，不覆盖完整回调/文件闭包 |
| B 复活 | `RevivePlayerGhost 0x3CFC60` 的 `0x3CFDB4` 调 EvaluateItems，后者有条件 TryUnlock | 原生 unlock 入口可记录；没有 `b_revive` 窗口归属，不证明所有通用 helper 无写入 |
| 普通换层 | `StageTransition::Update 0x509B2B → Manager_SaveGameState 0x532160`；保存有 run、daily、容器等条件，另有条件解锁 | PGD local/cloud/unlock 可记录；Manager/GameState 写入、删除、备份入口和官方附加文件不在五入口内，缺 `stage_change` |
| Continue | `0x532E7E → RestoreState`；已读 Continue 分支与 Restore 本体没有直接 Manager_SaveGameState 调用 | 缺 `continue` 归属和恢复/读取事件；不能把 Game::Start 尾或之后换层、退出的保存说成 Continue 必经写入 |

`STATIC_CONFIRMED`：StageTransition 还在 `0x5099FF/0x509A39` 按 Game `+0x10/+0x1839C` 与累计字段的条件调用 TryUnlock（42/67）。这些偏移相对 Game，不是 Manager。保存函数的 `0x532300/0x5323F0` 调 `GameState::write 0x59F7A0`；`0x53223D/0x532346` 条件调用 DeleteRerun；loaded 条件后的 `0x53244F` 调 PGD 备份入口 `0x5032A0 → 0x503380`，随后根据 Steam/选项走 local/cloud。主审已独立读取这些函数、分支和原始写模式。见 [StageTransition](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/StageTransition_Update.json)、[Manager 保存](../../../artifacts/repentogon/tx-04-07-static/st5-st7-review/Manager_SaveGameState.json)和代理原始指令窗。

备份入口的存在作为静态事实保留。**D27 不以每日备份判定存档变化**，本轮不因发现该入口新增验收限制。

### 10.2 官方附加文件不经过五个入口

`SOURCE`：官方 `GameStateManagement.cpp:261–286` 在 info known 且原 write 成功后执行 ESSM Serialize、VirtualRoom WriteSave、ItemPool SaveToDisk；VirtualRoom 失败还会影响后续调用。已定位文件包括 `EntitySaveStateManagement/<name>.dat`、`EntitySaveStateManagement/ex_<name>.json`、`VirtualRoomSetManager/<name>.json`、`ItemPoolManager/<name>.json`。普通 VirtualRoom 写入即使 Rooms 为空也会尝试打开/写 JSON，不能用“无虚拟房间”推论无文件写。

官方 Delete/DeleteRerun 在原调用后、info known 时删除 ItemPool、VirtualRoom、ESSM 及 ex 文件；当前五入口不记录这些删除。成就 tracker/自定义 achievement 还可走 `achievements<slot>.json`，并受 readonly、ID、当前状态等条件控制；原生 PGD 入口观测不能代替这些文件结果或回调体观测。ESSM 内部 binary serialize 失败返回后，外层仍可尝试 ex JSON，二者也不能只用一个成功标记概括。详见主审源码摘录与 [ST7 笔记](../../../artifacts/repentogon/tx-04-07-static/st7-agent/notes.md)。

### 10.3 后续实现需同时补窗口、入口和归约语义

1. `b_death`、`stage_change` 跨 loop 延续到结果/加载稳定；只在 Kill/StartStageTransition 同步函数外包 TLS scope 不足。复活、Continue 分别关联其原生调用/恢复与结果，后续退出保存独立归属。未归属事件显式记录。
2. 在统一 Hook 清单/已理解 ABI 下补保存、删除与读取观察，以及官方附加文件/成就 JSON 的观测证据；具体可用入口或文件 API/官方日志方案在实现时选择。本报告没有实现这些观察，也不要求穷尽第三方 gameplay 回调。
3. `tx06/evidence.rs:611–654` 的旧归约只把 `operation!=none` 的 return 纳入写入检查；仅补标签又会因旧零变化/禁止保存断言误拒本轮允许的原生写。tx-04-07 需按窗口记录入口、条件、结果，结合 D18/D20/D27 区分允许的原生行为与 C/S 新增行为，不能机械复用旧 `no_added_writes`。

### 10.4 主审采纳与保留项

主审已逐项阅读三份代理笔记及关键导出，另取上述函数、vtable、表值和源码验证核心结论。审阅过程中纠正并保留了：Restore 的 indexed flag 访问被漏搜；启动准备函数 `0x2F0AF0` 被误标为官方 constructor（实际签名在 `0x2ED4B0`）；Init 的 Baby skin 7 与 PlayerType 7 混淆风险；ST7 把字段前后观测说成 API 返回值，以及把 Game 相对偏移写成 Manager。更正记录见代理笔记与主审材料说明。

采纳 ST5 的置位/保存恢复与有限普通换层结论、ST6 的条件化角色表与 B 射击暂停静态路径、ST7 的“现有观测覆盖不足”结论。`UNKNOWN` 保留运行时 XML/存档表、通用 helper/间接调用和回调闭包、官方 Hook 实际顺序、各文件写入结果，以及 S15 激光实际行为。上一轮没有补齐 ST2–ST4 的整项主审；本轮追加的 ST3/ST4 见下文，ST2 仍待主审。tx-04-07 实现/构建/整体 E2E 尚未执行，静态核验完成不等于这些功能或 V1–V16 已通过。

## 11. 本轮 ST3/ST4 的范围与证据

本轮按用户要求，为 ST3、ST4 分别新开 `st3_reverse`、`st4_reverse` 两个 GPT-6 Luna / max 子智能体，只读 IDA MCP 逆向；主智能体另行取证审阅。[ST3 新笔记](../../../artifacts/repentogon/tx-04-07-static/st3-agent-current/notes.md)、[ST4 新笔记](../../../artifacts/repentogon/tx-04-07-static/st4-agent-current/notes.md)与各自可重复导出保存在 `st3-agent-current/`、`st4-agent-current/`。此前已存在的 `st3-agent/`、`st4-agent/` 原始导出原样保留，不以旧导出存在代替本轮审阅。

主审独立保存 [31 个完整函数及元数据](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/metadata.json)、[本工程与官方源码及原行号](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/source-excerpts.md)、[只读导出脚本与边界说明](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/README.md)。目标仍为 `v1.9.7.12.J273`、imagebase=`0x400000`；以下先写 RVA。所有调用链结论受所列分支条件限制，未闭合的间接调用、物品效果、官方 Hook/Lua 回调和设备通知不升级为已知。

## 12. ST3：普通换层、Player 与 Join/设备边界

### 12.1 夹具参数与原生 ABI

`SOURCE`：官方 `LuaGame.cpp:104–115` 要求第二参数为 boolean、第三参数为整数；第四参数只有 userdata 才转为 Player，否则传 nullptr。因此 `Game():StartStageTransition(false, 0, nil)` 与该 wrapper 匹配，无需虚构 Player 参数。注册见同文件 424 行。

`STATIC_CONFIRMED`：原生 `Game::StartStageTransition 0x2F9B30` 用 ECX=this，`0x2F9B41/0x2F9B44` 对 transition 做 unsigned `>6` 的拒绝检查，0 是有效值；尾部 `retn 0xC` 与三栈参数的 thiscall 一致。Player 参数为空时经 `Game::GetPlayer(0)` 选择当前主 Player。`0x2F9D62` 写 `Game+0x1BA78=1`，同时 counter `+0x1BA7C=0`、duration `+0x1BA80=50`；`0x2F9D76` 将 sameStage 实参写 `Game+0x1BA84`，本场景为 false，transition 写 `+0x1BA88=0`。本段只是函数行为和合法实参的确认，没有执行请求。见[入口完整指令](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Game_StartStageTransition.json)。

字段更正：当前原生指令的 sameStage 是 StageTransition `+0xC`，官方 `StageTransition.zhl:2` 却声明 `_sameStage:0x10`；后者与本版本实参写入不一致，不能用于直接字段写入。本夹具经方法传参，不按该结构声明改字段。选定 transition Player 的原生指针写点为 `Game+0x1BB70`，不得把旧误读 `+0x1BAF0` 当作布局事实。

### 12.2 窗口起止与回调次序

`STATIC_CONFIRMED`：普通 transition=0 的主线是 `1→2→3→4→0`。字段 `Game+0x1BA78` 的置 1 是请求已进入转场的起点；后续按实际 FSM 与动画/状态条件推进，不能将 duration=50 解释成固定 50 个 Authority tick 或固定墙钟时长。

| 原生阶段 | 本版本直接证据与顺序 |
| --- | --- |
| 1→2 | 满足相关 Player 状态与计数条件后，`0x509829/0x509830` 清 counter、写 state=2。其它动画类型有终局/短路分支，本轮不外推 |
| state 2 | `counter==duration-2` 的普通分支调用 `Level::SetNextStage`；达到 duration 后先在 `0x5098CE` 写 state=3，再更新相关 Player、执行 Level 初始化和房间操作 |
| 新层/新房间 | `Level::Init` 内普通路径 `0x33FA3F→RoomLoad`，后者 `0x339719→MC_POST_NEW_ROOM`；特殊 stage 另一分支 `0x33FA2B` 后还有条件 Room::Update。随后 `0x33FBFE→TriggerNewStageAll`、`0x33FC43→MC_POST_NEW_LEVEL`。这些回调都早于转场最终结束 |
| 回到 state 2→3 的调用体 | `0x509B15→Level::Init` 返回后，`0x509B26→Room::Update`、`0x509B2B→Manager_SaveGameState`，再处理临时效果。持久副作用按 ST7，第 10 节，不因处于加载状态而忽略 |
| 3→4 | `0x509D61/0x509D68` 清 counter、写 state=4，并调用层名显示 helper |
| 4→0 | `0x509DC0` 仍会虚调 Player::Update；根据 `Player+0x1398/+0x139A` 等条件，在 `0x509DF8` 清 state。随后 `0x509E06` 仍执行 ItemOverlay 更新，最后 FSM 返回 |

`Game::Update` 在 `0x2F73E5` 检查非零 state，`0x2F73F4` 调 FSM 后直接于 `0x2F7410` 返回；即使该次 FSM 内已经清零，也不在同次 Game::Update 落入普通 Room 分支。奇数帧 helper 对转场另有特殊 Player 更新；它们不是普通 odd shim `0x2F9559` 的 Authority 起点。见主审 [FSM](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/StageTransition_Update.json)、[Game::Update](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Game_Update.json)、[OddHelper](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Game_OddHelper.json)。

`SOURCE`：现有 `room_lifecycle` 为 Lifecycle owner，Room scope 只接受普通 Game owner；Odd tick 还要求普通 shim 标记。`RefreshLifecycle` 检查 starting/room/stage loading 并撤销绑定，稳定 Authority 才允许重新 Bind；`NewRoomReturned` 还设置本 loop 延后。因此“早期新房间/新关卡回调”“最后一次 Player 更新”“state 刚清零”均不能单独授权 Submit/Resolve。应等当前 Lifecycle/Start/Restore 等 owner 正常返回，之后在未阻断的稳定 Authority 入口重新读取 A/B exact pair；保留无新鲜输入中性、不重放旧边沿的要求。

### 12.3 普通 Player 保留与特殊例外

`STATIC_INFERRED`：本轮 S12 的普通换层，A/B 为独立完整 Player、B 为正常已复活的 Isaac，且没有会引发替身/移除/重置的效果或回调时，已读主线遍历原 Player vector，使用原对象 TriggerRoomExit/TriggerNewStage 和更新，没有无条件重新 Spawn/Init、重建 K 或改 controller 的步骤；可保留对象和 controller 的正常路径预期。它不取消加载后的唯一重绑。

以下直接反例使“任意换层都绝不重建/改 controller”不成立：`TriggerRoomExit` 的 `0x39EA0F` 在 parent/相应 null effect 等条件满足时可 RemoveCoPlayer；RoomLoad 在 room index=-12 且指定房间条件成立时，于 `0x339536` 调特殊 reset helper，后者可重新 Init/设置 controller。道具重算和官方回调还提供间接分支。见 [TriggerRoomExit](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Player_TriggerRoomExit.json)、[RoomLoad](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/RoomLoad.json)与 ST3 笔记。特殊房间、角色、物品及通知完整闭包仍为 `UNKNOWN`，本轮没有通过宽松身份检查接纳它们。

另一个可达性边界是 FSM 经 vtable `+0xC` 调 Player::Update，后者的特殊角色分支可在 `0x37BFCA` 调 SetControllerId。主审已回读前置：`0x37BEC5–0x37BED3` 只接纳 PlayerType 29/38，随后还检查关联子 Player、valid 和相应道具条件；`0x37BF58` 保存当前 controller，再将它作为参数传给 setter。这是特殊关联恢复/传播路径，不能解释为普通 Isaac 必然重新分配 controller，也不能声称整个转场调用图没有 setter。见主审 [Player::Update](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Player_Update.json)及 [vtable 实值](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/player-vtable.json)。

### 12.4 转场自己的 Join 与外层 Join pass 必须分开

`STATIC_CONFIRMED`：已读换层请求/FSM 主体不直接调用 Join pass 或三处 predicate。但 Manager 的普通 state=2 分派在 `0x52E89F` 调 `Game::ProcessInput`，随后才在偶数分支 `0x52E911` 调 Game::Update。ProcessInput 的 `0x2F6CD2/0x2F6CD9` 对 stage 非零跳到 `0x2F6D1B`，随后 `0x2F6D21` **仍调用 Join pass**；该跳转绕过的是一段暂停处理，不是屏蔽 Join。三处 predicate 仍各有其分支前提，不因一次 Join pass 就保证全部调用。见主审 [Manager](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Manager_Update.json)、[ProcessInput](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Game_ProcessInput.json)。

`SOURCE`：C/S `JoinScope` 只在 Warmup、K Registered、未 Join complete、无 starting/transition/block 且 A 已绑定等条件下合成 Join 动作。S12 已加入 B 后不会因此再次开启 Join scope；`JoinMode` 不在 scope 时保留原 predicate 值。K 非受控 gameplay/非 Join 查询输出中性。不能将原生 Join pass 的发生记录为 C/S 的第二次 Join。

K 的注册/移除不是本轮换层逻辑的步骤；已读普通块没有识别到这两类 K 专用操作。K 保持 Registered 时，正常 InputManager poll 仍可经过已有 K 兼容分派，它不是“换层触发 K 退休/重建”。当前 `mod/src/game/device_k.cpp:103–105` 的 Poll 仅在记录存在且 Registered 时返回真；旧 cs-t0 原型的无条件 true 不作为当前实现依据。所有间接设备通知、真实调用次数与 Hook 顺序仍为 `UNKNOWN`，不以有限直接调用图宣布无通知。

主审另纠正代理笔记中一个通知线索：`0x2F9D82` 的目标 `0x5950B0` 由官方 `FirstCollectibleOwner` 特征唯一匹配，实参 `0x236=566` 对应 Dream Catcher，道具查询结果用于后续转场条件；它不能被称为“事件 566/设备通知”。原生函数体遍历 Player 并查询持有情况，完整官方查询 Hook/间接效果仍不由这一定位证明。见 [FirstCollectibleOwner](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/PlayerManager_FirstCollectibleOwner.json)和源码摘录；代理笔记的未识别状态按当时取证范围保留，以本段主审定位为准。

## 13. ST4：Continue、starting 与菜单 Host 冲突

### 13.1 读取存档与应用存档是两个阶段

`STATIC_CONFIRMED`：菜单状态切换 helper `0x562E20` 的条件路径在 `0x562FCC` 调 `Manager_LoadGameState 0x5332F0`，读取当前 slot 的 GameState。它与之后 ExecuteStartGame 调用 RestoreState 不是同一操作，不把这个预读点当作 starting 窗口起点。

菜单淡出完成等条件满足时，`Menu_Game_Update` 在 `0x5596C6` 写 `Manager+0x4AD10=1`，相邻另一选择分支在 `0x559716` 也写此位。`ExecuteStartGame 0x532A20` 的 `0x532A69` 检查该 starting 位，正常尾 `0x53302E` 清零。Continue 分支用 `Manager+0x4AD11`，相邻 `+0x4AD12` 属于另一恢复路线，不混为同一分支。见 [菜单](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Menu_Game_Update.json)、[ExecuteStartGame](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Manager_ExecuteStartGame.json)与 ST4 新导出。

Continue 顺序为 `0x532E7E→Game::RestoreState(state,true)`，随后 `0x532E89→AutoReassignControllers`、临时效果、`0x532EAA→Room::Update`、`0x532EDF→PostGameStart(1)`，最后才到 starting 清零。Restore 本体依次在 `0x2F4A58/0x2F4A60/0x2F4A6C` 调 PlayerManager Restore、Level Restore、PostLevelInit Restore。因此内部新房间/启动回调与 Room/Player 更新仍发生在 starting/Start/Restore 窗口内，不能据回调提前绑定、创建 K 或推进 Authority。

### 13.2 保存一名 A 的含义

`STATIC_CONFIRMED`：PlayerManager 保存函数 `0x5940E0` 从零统计符合条件的保存记录，count 位于 `GameState+0x19D70`（105840），首记录 `+0x19D74`（105844），stride=`0x5BC`（1468）。`Game::SaveState` 的 `0x2F54C3` 调用该保存函数。主 Restore 经 `0x593AC9` 调 SyncPlayersFromGameState，`a4=1` 时匹配已有 Player，移除未匹配者，并通过 `0x593A1C→SpawnCoPlayer2` 补建缺失项；PostLevelInit 再以 a4=0 映射已有项。这些是原生恢复工厂调用，不是三处 predicate 控制的 Join 创建。

`STATIC_INFERRED`：实际存档只有一条有效普通 A 主记录、无附属 Player/替身和回调改写时，普通恢复预期为一个完整 Player；不是“原样复用加载前 A 地址”。`Player::RestoreGameState 0x3A56BE` 先按记录设置 controller；随后 AutoReassign 在 `0x596511` 先设 -1、在 `0x5967C7` 按可用设备重分配。官方 `ASMPlayerManager.cpp:42–64` 也明确 Continue 会覆盖先前恢复的 controller，并处理关联 Player 的共同分配。加载后的最终 controller 必须重新读取，不能承诺保存前后数值相同。实际 saved count、对象、角色、设备集合与完整回调效果仍待整体 E2E，不能由未读取的真实存档推定成功。

证据见主审 [保存记录](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/PlayerManager_SaveGameState.json)、[同步](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/PlayerManager_SyncPlayersFromGameState.json)、[Restore](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/Game_RestoreState.json)、[AutoReassign](../../../artifacts/repentogon/tx-04-07-static/st3-st4-review/PlayerManager_AutoReassignControllers.json)。Continue 的 flag 恢复与持久副作用边界仍遵循 ST5/ST7，不把 Restore 等同于必经保存。

### 13.3 当前源码会在恢复前退休菜单 Host

`SOURCE`：方案 S20 要求主菜单 `cs host`、握手到 Warmup、Continue、稳定后才创建 K/Join。`StartHost` 确实允许主菜单；`PollPeer` 握手后创建 Host Warmup。`RefreshLifecycle` 在尚未 `run_observed` 且没有 run 时保留该会话。但当前 restore wrapper 是另一条直接路径：

```text
Game::RestoreState wrapper（gen_binding.py:73–80，super 之前）
  RestoreBeginning() → RunTeardownBeginning() → StopSession(RunTeardown)
  RunExit() → StopSession(RunTeardown)，并清旧排队变更
  super(state, start)
```

`core::Session::BeginRetirement` 接受 RunTeardown，也接受 Warmup 状态；外层 `StopSession` 设置 retiring。尚未握手 Hosting 虽跳过 core 退休调用，也会被外层置 retiring。`!run_observed` 的轮询特例拦不住这两次直接停止请求。故相关 Hook 正常安装且原生 Continue 抵达 Restore 时，**当前源码会在原生恢复前退休菜单 Host，会话无法沿方案继续到 A Bind→CreateOnce K→Join**。这是静态实现冲突，不是已运行的失败样本。

后续实现需要在整个 restore/RunExit 调用链区分“首次菜单 Host 加载尚未接管的目标 run”与“已有 run 的恢复/teardown”，同时处理两个停止点；只豁免 RestoreBeginning 仍会被 RunExit 停止。已有 run 的退休和旧对象/输入不复活约束保持。这里记录修正要求，没有修改生产代码或选择未经核验的绕行手段。

### 13.4 K 兼容分派的条件答案

当前冲突路径在创建本轮 K 之前退休，不应说已确认 S20 成功触发 K 创建/Join。按方案修正后的时序，K 应在 starting/Restore/Start 窗口结束、稳定 A 唯一绑定后首次创建；原生 Continue 的恢复工厂与 AutoReassign 本身不等于这次 K Join。

所有兼容 Hook 常驻，不能把“没有本轮 K”说成 wrapper 都不执行。K 专用分支按自有对象识别；本轮 K 尚未创建/注册时，没有该对象参与本轮 Continue 的设备遍历。进程内旧 K 按 NEVER 保留内存，但已退休并离开设备 vector，不因此恢复身份。之后 CreateOnce 注册、InputManager poll、稳定 Join 才可按实际对象命中相应分派。完整间接通知与实际次数仍 `UNKNOWN`；本轮没有通过提前注册 K、临时移除全局设备或放宽绑定条件规避冲突。

## 14. 本轮主审结论与实现影响

主审逐项对照两份新代理材料、独立函数与源码，采纳以上有条件结论。审阅过程保留更正：Manager `0x52E911` 是 Game::Update，ProcessInput 实际调用点为 `0x52E89F`；sameStage 原生字段为 `Game+0x1BA84`，与官方结构声明不一致；选定转场 Player 指针为 `+0x1BB70`；旧原型 Poll 不代表当前 mod；FirstCollectibleOwner 的道具 566 参数不是设备事件；普通 Player 保留只在排除特殊 Remove/reset 分支后推导。没有用反编译自动名、固定偏移搜不到或有限图无边来证明完整闭包。

实现方案同步三点：S12/加载阻断的窗口需覆盖内部更新与回调，到 owner 正常返回后再稳定重绑；恢复 count=1 与 controller/对象身份必须分别判断；S20 的首次菜单 Host restore/RunExit 退休冲突需要在后续实现中修正。ST2 整项主审、功能实现、构建和 tx-04-07 整体 E2E 均未由本轮补齐。未运行游戏、构建、测试或任何动态 probe；所有实际字段、回调顺序/效果、通知次数与文件结果仍以其真实 UNKNOWN/未观察状态保留。

## 15. ST2 主审范围、结论与材料更正（2026-10-26）

**ST2 的指定问题已完成静态主审。** 普通 Isaac、独立完整 B、无备用/关联 Player、无相关附属 Player 效果且下游不改写身份时，`RevivePlayerGhost` 复用原对象，不需要第二次 K 创建或 Join；这一结论是有条件静态推导，不是整个复活窗口的无副作用证明。健康、稳定绑定与动画结束仍需分别观察。

主审直接回读当前 v1.9.7.12.J273 IDB，保存 [20 个完整函数与元数据](../../../artifacts/repentogon/tx-04-07-static/st2-review/metadata.json)、[调用方/注册片段与 vtable 实值](../../../artifacts/repentogon/tx-04-07-static/st2-review/details.json)、[官方源码摘录](../../../artifacts/repentogon/tx-04-07-static/st2-review/source-excerpts.md)及[只读导出脚本](../../../artifacts/repentogon/tx-04-07-static/st2-review/export_review.py)。imagebase=0x400000，内嵌版本字符串与前序一致；没有计算或校验哈希。代理[原笔记](../../../artifacts/repentogon/tx-04-07-static/st2-agent/notes.md)、原始导出和有界扫描不覆盖。

| 代理材料中的说法 | 主审处理 |
| --- | --- |
| 普通复活复用 this、不重新 Init、局部 controller/index 不变 | 有条件采纳；本体没有 Player 分配/Init/注册设备/Join，Effect Spawn 不是 Player Spawn。EvaluateItems、特殊角色和 backup 链另列，不把被跳过的调用图当作已排除 |
| 复活返回后身份与健康字段“立即满足” | **收紧**：本体只清 ghost，不重置 valid/exists/dead/removed/controller/index。必须以 ST1 的正常前态为前提，并重新读取实际健康与唯一归属；Lua true 不能证明这些条件 |
| extra animation 本身不阻止移动，移动“不受影响” | 仅采纳两个移动 consumer 没有读 extra animation 标志这一局部事实；其余控制条件、Player::Update 分派、碰撞和世界状态仍存在，不能承诺立即位移。主要射击输入块会被跳过，但并非全部射击函数都不执行 |
| InputManager vtable 有“60 个槽” | **更正为 47 个函数槽（0–46）**；VA 0xB32F00 已是相邻表的 RTTI COL 0xB45F4C。原“图像加载器不在 60 槽内”的检查不构成设备闭包证据 |
| 所有写入都针对入口 this | 只适用于无 backup/特殊分支的本轮对象；本体尾部会沿 `+0x1E50` 改换 edi，playerType=35 还创建并初始化关联 Player |
| InitTwin 调用点 `0x3CFF08`；maxHearts 读取点 `0x3CFDFD` | 分别更正为 `call` 位于 **0x3CFF03**、读取位于 **0x3CFDB9**；0x3CFF08 是保存返回对象，0x3CFDFD 是 test eax,eax。函数定位和心值分支结论不变 |
| 无设备通知/持久写入、普通 B 不会触发 PeerLost | 不作无条件采纳；原笔记已保留完整窗口 UNKNOWN，本轮维持。扫描排除了 EvaluateItems/Spawn 等且只看有限直接边，不能据此删除 PeerLost 或 V16 观测 |

## 16. ST2 原生复活、字段与条件调用链

### 16.1 同步调用与对象范围

`STATIC_CONFIRMED`：[RevivePlayerGhost](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_RevivePlayerGhost.json) RVA **0x3CFC60–0x3CFF97**，ECX=this（0x3CFC6C 保存到 edi），尾部 0x3CFF96 为无栈实参的 retn；入口字节对应官方 EntityPlayer.zhl:515–516 的 thiscall void() 声明。首轮不自行检查 ghost。`SOURCE`：LuaEntityPlayer.cpp:2355–2368 仅在 `_isCoopGhost` 为真时同步调用，原生返回后才返回 true；false 表示未发起调用，不是异步请求状态。

本体先在 0x3CFC88 清 `ghost +0x207D`、0x3CFC8F 清 `+0x3B0`、0x3CFC96 恢复 collision class=4，随后重载图形、条件播放 Appear、设置受伤冷却 `+0x13B8=60`、还原物品与饰品、以 CACHE_ALL 重算并恢复生命值。**ghost 在下游调用之前已清零**，因此不能在某个重入回调中只看 ghost=false 就公布新代次；仍须等动作正常返回及稳定 Authority 入口。

0x3CFEF1 的 Spawn 实参为 Type=1000/Variant=15 Effect；不代表新建 B。0x3CFEF6 的 playerType=35 分支才于 **0x3CFF03** 调 [InitTwin](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_InitTwin.json)，其 0x3C838D SpawnCoPlayer2(40)、0x3C840A 给新关联对象设置原 controller。尾部 0x3CFF79 取 `+0x1E50` backup，非空且也是 ghost 时回到 0x3CFC80；不能把普通 Isaac 的单对象推导推广至这些分支。

### 16.2 字段值与生命值

| 字段 | 函数本体行为与可采纳结论 |
| --- | --- |
| ghost `+0x207D` | 明确写 0；仍须考虑之后的下游/回调改写 |
| valid/visible/exists/dead/removed `+0x170/+0x171/+0x172/+0x173/+0x177` | 本体未写。正常 ST1 Morph 已调用 Revive 清 dead 并设置 visible，因此预期沿用；本函数不会修复异常前态 |
| Type/Variant/playerType `+0x28/+0x2C/+0x13BC` | 普通对象本体未改；不能用此声明下游不存在身份变更 |
| controller/index `+0x160C/+0x1610` | 普通对象本体没有写或 setter 调用；普通受限路径预期保持 K/index=1，完整窗口必须重读 |
| ControlsEnabled/ControlsCooldown `+0x410/+0x414` | 本体未写；受伤冷却 60 不能当作控制冷却，也不能据此跳过实际控制状态 |

0x3CFDB9 在 EvaluateItems 返回后读取 maxHearts；若正数，红心先取 min(maxHearts,3)，HealthType=3 时改成 2。非正数时按 HealthType=4/3/其他分别处理骨心、金币心、魂心，见原始指令。`SOURCE`：官方 GetHealthType 不只是纯查询，CustomCallbacks.cpp:1574 起会读取 XML/custom cache、运行 1067，并在类型变化时转换心值及运行 1128。因此这些数值是**原生分支局部写入公式**，不是所有官方 Hook 下的最终状态承诺；记录实际心值，不固定普通 Isaac 必得某个值。

### 16.3 原生调用方、附属 Player 与副作用

`STATIC_CONFIRMED`：当前入口共 6 条直接 call 引用，来自 AddEternalHearts、Player::RestoreGameState、PlayerManager 的 revive-all 与 revive-selected 四个函数；三次 selected 内调用仍属于同一函数。原生引用没有包含官方 DLL 的 Lua 包装，不把这 6 条当成运行时全部入口。

- [AddEternalHearts](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_AddEternalHearts.json) 入口检查 ghost；为真时复活并直接返回，amount=0 也能触发。
- [RestoreGameState](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_RestoreGameState.json) 的 0x3A5753 受沙漏恢复、当前 ghost、保存 ghost=false 等条件约束；它还另有 controller setter 与保存 ghost 的直接复制，不能把整个 Restore 当作单纯复活。
- [revive-all](../../../artifacts/repentogon/tx-04-07-static/st2-review/PlayerManager_ReviveAllCoopGhosts.json) 枚举当前列表且先检查 ghost；[revive-selected](../../../artifacts/repentogon/tx-04-07-static/st2-review/PlayerManager_ReviveCoopPlayer.json) 检查索引与 ghost，并处理关联 Player。局部调用方片段重新确认 boss 清理、Greed 条件、bed 与 Slot 分支可进入复活，换层的 Maggy's Faith 路径可经 AddEternalHearts 进入；S10 Lost 窗口必须记录是否先发生自然复活，不能仅按 F8 动作次数归因。未扩展或调用原版 Netplay。
- [EvaluateItems](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_EvaluateItems.json) 在 cache 含 0x200、Variant=0、helper 及重入条件满足时维护 Found Soul/667 相关附属 Player。0x3682FC/0x36843C 可 RemoveCoPlayer，0x36833F/0x36847F 可 Spawn，0x368354/0x368494 可给新对象设置 owner 的 controller。没有相关效果且没有既存附属对象时才可作“不增删”的受限推导，不能推广成整函数无 PlayerManager 写入。
- 复活本体无直接 PGD 计数/解锁调用，但 EvaluateItems 的条件 TryUnlock、HealthType=4 下 AddBoneHearts、特殊角色初始化、Effect 初始化和官方回调仍有条件副作用。官方 CACHE_ALL Hook 在原生 EvaluateItems 之前运行已知 custom cache，音效有 pre/post callback。V16 沿用 ST7 的跨 loop 窗口与文件观测要求，不声称零持久写入或设备通知闭包完成。

## 17. ST2 Appear、输入消费与重绑时机

`STATIC_CONFIRMED`：Game+0x25F63=0 时才在 0x3CFD1A 调 [PlayExtraAnimation](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_PlayExtraAnimation.json)("Appear")。它清 `+0x139A`、设置 `+0x1398=1`；[IsExtraAnimationFinished](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_IsExtraAnimationFinished.json) 仅在这两个字段都为 0 时返回真。Player::Update 的 0x37D234–0x37D24E 在动画完成判断成立时清 1398；没有读取动画资源时长，不能预设固定等待帧数。

主审保存 [Movement](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_Movement.json)、[MovementAux](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_MovementAux.json) 与 [Player::Update](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_Update.json) 的完整导出，并核对相关控制/分派片段：两个移动 consumer 没有直接读 1398/139A，仍检查 ControlsEnabled/Cooldown，外层也有其分派条件。这只说明 extra animation 不是它们本体的直接输入查询条件；S11 先验证移动仍须观察实际位移。

[Shooting](../../../artifacts/repentogon/tx-04-07-static/st2-review/Player_Shooting.json) 的 0x373ACC–0x373B37 在 extra animation 标志非零时，还要求 body sprite 的动画状态及名称满足 LiftItem；不满足则跳 0x373F62，绕过 0x373C2D–0x373D16 的主要射击查询。后续函数仍继续执行，其他查询/武器分支完整可达性保留 UNKNOWN。故“接受了射击命令”“恢复普通射击查询”“生成归属 B 的攻击实体”是不同观察，不能用其中一个替代全部。

工程规则：复活调用正常返回后，只在稳定且未阻断的 Authority 入口重新读取健康与 exact pair；仍按现有失效代次绑定同一个 K/ordinal，不再次 Invalidate/Join。动画未完成不作为整个 Authority 停 tick 的理由；A 继续推进。夹具记录 true/false、实际状态、稳定重绑与动画完成各自时点，先观察移动，实际动画结束后再作射击断言，不 StopExtraAnimation、不改 ControlsEnabled/Cooldown。

## 18. ST2 审核结果与未决项

采纳普通受限路径“原对象清 ghost、恢复生命状态、不直接重建/Init 或重分配 controller”的核心结论；修正第 15 节列出的健康保证、移动措辞、槽数与指令地址问题。原材料保留，在笔记顶部追加主审批注；本报告与实现方案/工程细化同步为 **ST1–ST7 静态取证与主审完成**。

U10 保留可达的 PeerLost 防御与既定清理规则：静态已排除本轮普通局部路径主动移除 B 的需求，但尚不能排除整个 Lost→Revived 窗口的身份矛盾或回调改写。不增加新场景轮次，也不放宽健康/归属条件。

`UNKNOWN/未观察` 仍包括：实际物品、backup 与心值；官方 Hook 实际排列及 callback 状态；资源/日志/Effect 初始化与间接调用的完整闭包；设备通知与持久副作用实际结果；Appear 时长和后续射击分支；实际重新绑定、代次拒绝与输入效果。本轮只读静态取证和文档修订，没有功能实现、构建、游戏启动、动态 probe 或测试，不预记 S10/S11 或 V1–V16 通过。
