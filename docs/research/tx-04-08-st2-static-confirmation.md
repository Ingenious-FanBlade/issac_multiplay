# tx-04-08 ST2：完整采样阶段、Hook 次序与短路静态核验

日期：2026-10-26，Asia/Shanghai。范围为迁移设计第 4.3 节 ST2（已删除，原文 `39ddcfd6^:docs/design/tx-04-08/tx-04-08-implementation-plan.md`）及第 5 节。用户明确指定一个 **GPT-6 Luna / max** 子智能体逆向，主智能体审阅；代理材料见[逆向笔记](tx-04-08-st2-agent-notes.md)。本报告区分目标指令 `STATIC_CONFIRMED`、本地代码 `CURRENT_SOURCE`、静态推导与拟接线 `DESIGN`，不增加运行时观察。

本次工作范围为读取 IDB、源码和既有记录并编辑研究文档，没有运行游戏、构建、生成器、离线工具或功能测试，没有计算/校验哈希，没有修改 IDB、生产实现、Hook 清单或部署文件。**过程偏差：代理额外执行了一次用于判断文件正确性的 JSON 语法校验，违反分析期间不运行校验脚本的约定**；命令、输出与主审处理见[原始记录](../../artifacts/repentogon/tx-04-08-static/st2-agent/process-deviation.md)。不得将其改称零校验或授权验收。ST2 是同一 tx-04-08 内的静态资料工作，不另作子项测试验收；完整 tx-04-08 和唯一整体 E2E 尚未完成。

## 1. 主审证据与结论边界

目标仍为 ST1 的 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`。下文明确区分 VA 和 RVA，不从旧 J460 返回地址平移。官方源码根为本地 `F:/projects_isaac/REPENTOGON-1.1.3/`，不把源码 Hook/ASM 补丁当成本次进程已安装。

主智能体独立导出并阅读：

- [六个阶段函数的完整伪码、指令、字节与入口引用](../../artifacts/repentogon/tx-04-08-static/st2-review/main-stage-functions.json)：Manager、Game、Room、标准尾部、Odd helper、EntityList。
- [关键分支原始窗口](../../artifacts/repentogon/tx-04-08-static/st2-review/main-stage-windows.json)：Manager 普通/Odd 分流、Game 最后早退及更新入口、Room→EntityList、Odd 普通虚调用、尾部及嵌套 Manager 调用。
- [官方 MainUpdate/Return 特征静态搜索](../../artifacts/repentogon/tx-04-08-static/st2-review/main-asm-signatures.json)：本 IDB 中分别唯一命中 `0x6F77A4`、`0x6F7A39`。
- [带文件路径和行号的源码摘录](../../artifacts/repentogon/tx-04-08-static/st2-review/main-source-excerpts.json)：当前 wrapper、Frame/Simulation/Player scope、pipeline、官方排序/安装、loader、callback 短路及 EntityManager ASM 补丁。
- [代理阶段窗口与结论原始材料](../../artifacts/repentogon/tx-04-08-static/st2-agent/st2-evidence.json)，主审逐项对照上述独立取证；代理最终说明见[笔记](tx-04-08-st2-agent-notes.md)。

**核心结论：普通 Room 正常返回只是 `NativeReturned / AwaitPostState`；只有同 outer 的普通 Game owner、Room、标准尾部按顺序正常完成，Manager 下游链也正常返回，并满足本轮唯一 StandardRoom candidate、生命周期和只读稳定性条件，才可在 `EndManager` 前准备完整复制。clock 提交后才可发布该批次。** 正常返回不保证所有 Lua callback 成功，也不保证每个 Player 派生更新实际到达。

## 2. 目标普通路径与精确边界

| 语义 | VA / RVA | 直接指令事实与含义 |
| --- | --- | --- |
| Manager::Update | `0x92E3E0 / 0x52E3E0` | 静态 stdcall 边界，主路径 `0x92EC22: C2 04 00`；另有 `0x92E45A`、`0x92E80D` 正常早退，不能数 Manager 次数代替模拟 |
| Manager→Game | `0x92E911 / 0x52E911` | `E8 1A 84 DC FF` 调 `0x6F6D30`；Manager state=2 的条件分支，`+0x4A8B8` 低位为 0 时走此路 |
| Game::Update | `0x6F6D30 / 0x2F6D30` | `this=ECX`；Console、等待/加载、Room lifecycle、Door、暂停/失焦、ReviveMenu 等可使其不走普通更新 |
| Game 最后早退后的更新段 | `0x6F77A4 / 0x2F77A4` | `FF 87 38 5B 02 00`，`inc [edi+0x25B38]`；官方 `Game_Update_MainUpdate` 的目标位置，而非“进入 Game 即开始普通更新” |
| Game→Room | `0x6F78DF / 0x2F78DF` | `E8 7C 06 10 00`；前一条从 `[Game+0x18300]` 取 Room 到 ECX。原生返回续点 `0x6F78E4` |
| Room::Update | `0x7F7F60 / 0x3F7F60` | 原生主路径正常 `ret` 在 `0x7FBD81`；其返回结束 Room 模拟，Game 后处理仍未结束 |
| Room→EntityList | `0x7F93A7 / 0x3F93A7` | `E8 54 AB C1 FF`；`0x7F9399: push 0`、`0x7F939B: lea ecx,[edi+0x1218]`，是当前 Room 自身的列表更新。参数以实值 0 为准，官方 ASM 称其 `isTransition`，当前 C/S 参数名 `partial` 不作为额外语义证明；不能将任意 EntityList 调用视作完整 Room/profile |
| Game→标准尾部 | `0x6F7A34 / 0x2F7A34` | `E8 57 F2 14 00` 调 `0x846C90`，在 Room 返回及后处理之后；本次直接 code xref 只有此处 |
| 标准尾部 | `0x846C90 / 0x446C90` | cdecl，无显式参数；正常主路径 `0x846D15: C3`。内部可因 callback 状态未开启而不分派 Lua，仍是尾部正常调用 |
| Game 主路径返回 | `0x6F7A49 / 0x2F7A49` | 标准尾部返回后经 `0x6F7A39` epilogue 返回；多个早退也使用此 epilogue，不可只记录这个 ret 就断言经过 Room/tail |

正常路径的嵌套顺序为：Manager 进入 → Game owner 进入 → Room 进入 → Room 正常返回 → Game 后处理 → tail 进入/正常返回 → Game owner 正常返回 → Manager 剩余处理及下游 Hook 返回 → C/S FrameEnd 收束。

`STATIC_CONFIRMED`：Room 的其他直接调用者包括 `0x6F1B9D`、`0x6F2671`、`0x6F3A3E`、`0x6FA17C`、`0x70213B`、`0x732E57`、`0x73FA36`、`0x909B26`、`0x932EAA`。已知 Start、Restore、Lifecycle、Door 相关 owner 的内部 Room 更新不计普通完整阶段；这些引用也说明不能以“发生过 Room”作为资格。未对这些 owner 的全部嵌套/副作用在 ST2 内作闭包，具体归 ST6/ST7。

Manager 入口另有 `0x6F42AD → 0x92E3E0` 的调用，所在函数为 `0x6F3E10`，Game 的条件分支可进入它；因此“每次 Manager wrapper 都是新顶层轮次”不成立。当前 `FrameBegin` 已有 `frame_depth` 重入检测和既有 ContextFault 策略；world 不得给嵌套 Manager 独立分配完整采样资格。真实 owner 重入故障仍由既有机制处理，不能改成只丢 world。

## 3. 官方链与 priority

`CURRENT_SOURCE`：`libzhl/HookSystem.cpp:397–424` 使用 `std::multimap<..., std::greater<int>>`，按较大 priority 先安装；每次安装保存当前入口跳转并覆盖入口，`mologie_detours.h:331–431` 的备份/相对跳转重定位让后安装者包住已安装者。故**调用时低数先进入、同链返回时低数后退出**。这与 `repentogon/dllmain.cpp:69–76` 的公开说明一致，不能把安装遍历顺序误读为执行顺序。

`loader/loader.cpp:130–190` 先枚举 DLL 并调用初始化，再统一 `ZHL::Init()`；当前 `RegisterHooks` 在 ModInit 内向共享 libzhl 注册。这个源码关系支持不同 priority 的同次注册排序；不证明用户机器当前 DLL 列表、开关和实际运行顺序。

| 已检阅入口 | 当前 C/S priority | 官方相关源码 | 能证明的范围 |
| --- | --- | --- | --- |
| Manager::Update | `-10000` | `Quickstart.cpp:22`、`LuaIsaac.cpp:478` 为 `0`；`EntitySaveStateManagement.cpp:3489` 的 `-9999` 在 `#ifndef NDEBUG` 下 | 对已检阅、正常注册的这些 Hook，C/S Manager scope 在外层；debug Hook 不能写成 Release 必有 |
| Game::Update | `0` | `ASMCallbacks.cpp:30` 在 super 前清 `s_callbackPause`；`ASMGridEntityCollision.cpp:28` 在 super 后清碰撞缓存，均为 `0` | 同优先级的相对顺序依赖注册顺序，不能由清单行号或文件名决定；它们都正常调用一次 super |
| Room / tail / EntityList | `0`、自有定义 | 当前清单 `RoomUpdate / PostUpdateCaller / EntityListUpdate`；目标有官方函数体内 ASM 补丁 | 已检阅源码未发现相同语义的独立官方完整替代 wrapper；不等于原始函数体在运行时完全未改 |
| Player::Update | 外 `-10000`、内 `10000` | `CustomCallbacks.cpp:523–534` 的 callback 1160 为 `0` | 对已检阅链，内标记位于可短路 callback 之后；它是该层到达证据，不是对任意后续更内 Hook 的数学保证 |

Game owner 的 `0` scope 不保证包住所有同优先官方前后处理；但对上述官方代码，普通 Room 和 tail 都在其 super 所含原生 body 内，仍可归入同 Game token。**完整复制等待 Manager 下游整链正常返回**，不把 Game scope 退出当成官方所有后处理已结束。ST8 仍须核对最终实际注册/容量及运行链记录。

ZHL `_internalHook` 和 `_internalSuper` 都会额外发出 `call`（`HookSystem.cpp:651–656,744–748`）。因此 Hook 内 `_ReturnAddress()` 可能属于生成桩，不等于 `0x6F78E4` 或 `0x6F7A39`。上述原生地址用于证明静态顺序；运行接线使用同一 outer/owner/scope 的显式通知与配对，不猜桩栈层数，也不新建第二套入口补丁 owner。

## 4. 三类正常返回不能扩大解释

### 4.1 `MC_PRE_UPDATE` 跳过普通整轮

`CURRENT_SOURCE`：官方 `ASM.zhl:289–290` 定义 `Game_Update_MainUpdate` 和 `Game_Update_Return`。`ASMCallbacks.cpp:92–113` 在前者执行 callback，真时直接跳到后者。对本 IDB，两个原始位置分别为 VA `0x6F77A4` 和 `0x6F7A39`；中间的 Room call 和 tail call 均被跳过。

`handle_skipped_update()` 仍会调用 `EntityManager::CommitRemovedEntities`、`render_sort`、`HUD::PostUpdate`，不能写成跳过帧完全无世界/列表变更；`HUD::PostUpdate` 也不是标准尾部 `0x846C90`。该路径 Game 和 Manager 都可正常返回，但没有完整阶段、没有普通 Room Authority candidate，应输出本轮无 Authority/阶段不足的明确 gap；没有实际 callback skip 通知时，只能写已观察到的阶段缺失，不能仅据此断言 1026 返回了 true。

### 4.2 Player 派生更新被跳过

`CustomCallbacks.cpp:499–541`：1160 返回 true 时不调用 Player 的下游 `super()`，改调 `Entity::Original_Update()`。外层 Player scope 可以正常返回、普通 Room/tail 也可以正常完成；这不授予该 Player 新的专用字段读取依据。当前 `PlayerScope / PlayerNativeReached` 已区分进入、内层到达及同 tick/generation 仍有效后的字段读取，不能拿 consumer=0 推断 body 被跳过。

`DESIGN`：普通 tick 仍按原规则提交；world 记录该 Player 的字段依据缺席，字段完整性据实决定，不能为补齐字段再调一次 Player 更新。若已有真实构造/Init 等其他读取依据是否有效，由 ST3/ST5/ST7 确认，不在 ST2 中宣称全部失效或全部可读。

### 4.3 tail 正常返回与内部错误

尾部会按 Lua callback 状态选择是否分派；无 callback 不影响“经过标准尾部”这一阶段事实。原始指令 `0x846D16–0x846D63` 是错误处理 funclet，打印 `Error in "PostUpdate" call`，最后返回恢复地址 `0x846D04`；它不是第二个普通更新主出口。Hex-Rays 主伪码没有完整显示这段错误处理，不能只读伪码作“所有 callback 成功”的结论。

`DESIGN`：阶段的 normal 表示没有异常逃过当前 wrapper；不自动等同 Lua callback 成功、无内部处理错误或零副作用。若运行时可观察到相关错误应保留；本次未运行 callback，也没有新建错误观测入口。tail 的 EAX 残值不是采样成功码。

## 5. Odd、阻断与异常

Manager 的 `0x92E8B8` 与 `0x92E916` 分流指令表明普通 Game 与 Odd helper 是同 state 分支的替代路径。Odd helper 在 `0x92E921` 被调用；它不调用普通 Room 或标准尾部。其 `0x6F9559 / RVA 0x2F9559` 的 `FF 52 0C` 是现有普通 Player 标记点；早期特殊 Player 更新不能因同属 Odd helper 就计 Authority。

`CURRENT_SOURCE`：`SimulationScope` 在普通 Room 下以 Game owner token 执行 BeginTick，在 Odd 下等待 `BeginOddPlayer`；正常返回后 `NativeReturned` 提交 previous 并进入 `AwaitPostState`。因此真实 OddPlayer 可以完成输入模拟和 tick，world 仍只能写 `ODD_PLAYER_ONLY` gap，不获得完整普通样本资格；无普通标记/无候选的 helper 调用也不能伪造 Odd tick。

Console、暂停、loading、主菜单、失焦等可能在普通入口之前返回。当前 `BeginTick / ReadyForAuthority` 还检查主线程、自有会话、生命周期阻断及绑定；不能由 Game/Room 方法被调用自动推出本轮 Authority 开始。`InBlockedContext` 包括 Start、Restore、Lifecycle、Door、NestedDoor、Join。具体撤销/重绑接口归 ST7；本次不改变 tx-04-07 的六 tick Shooting 恢复或 A/B 中性规则。

原生异常逃过 Room/Game/tail/Manager 时，只有进入没有正常完成通知；不新复制完整世界。当前 `CallScope`、`SimulationScope`、`FrameScope` 用异常计数观察 C++ 展开，并保留既有 Authority 的失败处理。这不构成对全部 SEH、access violation、强制退出或并发异常的恢复保证。world 新入口的错误线程/配对问题只停 world，不能直接套现有 `ContextFault → FailRuntime`；真实旧 owner/Authority 故障仍按原机制处理。

## 6. 可接线的完整采样判据与 TopExit 顺序

以下为 `DESIGN`，没有在本次修改生产代码。

1. **Outer 开始**：在最外层 Manager 的 FrameBegin 中、`game::BeginManager` 之前记录 world outer token、mutation、run/room epoch、录制 generation 和根关联。当前 BeginManager 内已有 PollPeer、RefreshLifecycle、DrainPeer，其后才执行控制请求；把 token 放在控制请求前但 BeginManager 后仍会漏掉入口变更。嵌套 Manager 不生成可完整采样的新 outer。
2. **普通阶段**：同 outer 的普通 Game owner 进入；同 Room scope 进入/正常返回各一次；其后同 Game 的标准 tail 进入/正常返回各一次；最后 Game owner 正常返回。排除内部生命周期 Room、顺序颠倒、重复、重入及未配对观察。阶段判断共享现有 wrapper，不以旧返回地址、调用计数总值或“最近一次”状态代替配对。
3. **候选与完整性**：本轮恰一个真实 StandardRoom candidate，kind、loop、Game token、Room scope 与阶段一致；主线程，未退休/污染录制域，复制前后 root、Room、来源描述符、mutation、epoch 和 generation 稳定，字段/身份/来源满足完整 profile。ST2 只确认阶段位置，不预先确认 ST3–ST7 的字段/来源规则。
4. **准备复制**：Manager wrapper 的下游 `super(value)` 正常返回，进入 FrameEnd；在调用 `game::EndManager(completed)` 之前完成阶段收束和只读复制。此时 Room 已 `NativeReturned`，previous 已提交，core 仍是 `AwaitPostState`。复制不调用 getter、Lua、Spawn 或更新方法，不再运行原调用。
5. **core 收束与批次发布**：准备态批次尚不可见；将同候选的 post-state 值交给 FinishPostState，clock 正常提交 completed 后 release READY，再继续既有 PublishCompleted、trace 和输入 Ack。若 core 未提交，取消 pending，不能留下带未提交 Authority tick 的完整样本。写盘结果不决定输入 Ack 成功。
6. **退休之后的界限**：当前 EndManager 先 FinishPostState/PublishCompleted，再 EndOuter、PublishPeer，最后才 RemoveOnce/RetireJoinedPlayer/StopPeer。因此 world 复制必须位于它之前；不能搬到 FrameEnd 尾部或 PublishStatus 后再读取已退休的 B/K。先完成 tick 后才发生的本地退休不倒抹已完成的输入事实；生命周期污染对 world 样本的影响另外如实记录。

`CURRENT_SOURCE`：`TickPostState` 目前只有 Disabled，`FinishPostState` 遇非 Disabled 会 `Abort(InvalidSource)`；当前 wrapper 对 Game/tail 没有 world 阶段计数，Room 只有输入模拟 scope。所以上述位置可接线，不代表现有工程已经能采完整样本。实现仍需扩展 Complete/Gap/Backpressure/Unavailable/Faulted 的值协议，避免 world 故障误停 Authority。

## 7. 主审限定及未决项

主审独立核对代理结论的普通 call 链、Game 更新入口/短路目标、Room→EL 的 `push 0`、Odd 普通点、官方 priority 及当前 TopExit。以下限制必须随结论保留：

- **不将同优先级次序写死**：priority 规则可由源码确认；Game 的两个官方 `0` Hook 与 C/S `0` 的相对顺序未由本次运行观察。Manager 外层收束才等待整个下游链。
- **不把 EH funclet 当普通阶段返回**：tail 的 `0x846D63` 以及 Game 的远地址 FuncItems 属附属处理/共享块信息，不能一律统计为普通主路径 ret，也不能推导所有内部 callback 成功。
- **不把下一层到达当全部原生行为保证**：Player 的内层 marker 对已检阅官方 callback 有区分能力，未知更内层 Hook、实际启用配置和派生覆盖不由它证明。
- **收窄代理初报的 Room 描述**：代理曾泛称 Room 有绕过 EntityList 的条件/早退，随后按 CFG 撤回；`0x7F9341`、`0x7F9358` 附近分支及 warning 路径汇入 `0x7F9399` 的实参 0 调用块，不能据它们声称列表更新被跳过。主审以已回读的具体指令为准，不用 `partial` 名称推定语义，也不将这一个调用扩大为全 profile 完整性。
- **不从缺席猜具体原因**：无 candidate、Room/tail 缺席与显式 1026 skip 是不同证据；只有实际原因已观察才记录该原因。
- **不恢复旧地址、运行 PASS 或验收**：原型阶段算法仅作索引，目标顺序由本次指令确认。实际 DLL 链/ASM 安装与 callback 状态、字段布局/初始化/分类、全部生命周期嵌套、来源/门别名、跨线程与后台退出仍属 UNKNOWN 或 ST3–ST8 后续工作。

**ST2 指定问题的静态核验及主审完成**，覆盖普通阶段、Hook 次序、短路与复制位置，UNKNOWN 按上文保留。功能实现、ST3–ST8、2,000 次稳定采集、双进程比较和 tx-04-08 整体 E2E 不在本次完成范围。
