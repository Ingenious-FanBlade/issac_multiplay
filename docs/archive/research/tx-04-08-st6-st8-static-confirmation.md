# tx-04-08 ST6–ST8：owner 完成、生命周期资格与工程边界静态核验

日期：2026-10-26，Asia/Singapore。范围来自迁移设计第 4.3 节（已删除，原文 `39ddcfd6^:docs/design/tx-04-08/tx-04-08-implementation-plan.md`）。用户明确指定 ST6、ST7、ST8 各由一个 **GPT-6 Luna / max** 子智能体逆向，主智能体逐项审阅。三个代理仅编辑自己的研究笔记和原始材料；主智能体另行读取目标 IDB、原始指令、本地 SDK 和当前工程，负责本文和入口同步。

状态：**ST6、ST7、ST8 指定问题的静态核验与主智能体审阅完成，具体 UNKNOWN 与实现缺口保留。** 只做静态读取与研究文档编辑，未修改生产代码、绑定、IDB 或运行进程；未构建、运行生成器、游戏、测试、离线 replay/compare、额外格式校验或哈希操作。整个 tx-04-08 的实现和唯一整体 E2E 尚未完成。

## 1. 证据与审阅方法

目标为当前 IDB 输入 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，J273、imagebase `0x400000`。下文为 VA，`RVA = VA - 0x400000`。本地 SDK 根为 `F:/projects_isaac/REPENTOGON-1.1.3/`。`STATIC_CONFIRMED` 是目标静态指令/数据；`CURRENT_SOURCE` 是 SDK/当前工程；`DESIGN` 是未来接线，`UNKNOWN` 不因静态工作结束而消失。

代理笔记：[ST6](tx-04-08-st6-agent-notes.md)、[ST7](tx-04-08-st7-agent-notes.md)、[ST8](tx-04-08-st8-agent-notes.md)。各自材料位于 `artifacts/repentogon/tx-04-08-static/st6-agent/`、`st7-agent/`、`st8-agent/`。

主审材料见[读取说明](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/query-notes.md)：[准确 owner 函数与原始指令](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-exact-owner-functions.json)、[ManagerRestore/Replace 完整查询](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-owner-functions-full.json)、[Shutdown](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-shutdown.json)、[main 的退出调用](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-shutdown-caller.json)、[PE 头](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-pe-context.json)及[带行号源码摘录](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-source-excerpts.txt)。初次手抄地址误选与 MCP 截断均保留说明，最终结论使用准确入口及完整原始输出。

## 2. ST6：对象完成必须归属真实外层 owner

### 2.1 Entity 创建、Morph 与恢复

| 路径 | 静态事实与完成限定 |
| --- | --- |
| Spawn `0x6FA210` | `0x6FA2E3` Factory，之后真实派生 vtable Init；`0x6FA3B3` Publish 后仍有属性/引用/回调和其它后处理，正常主路径到 `0x6FA91A ret 20h`。Factory 也可返回 null。只在外层实际返回后核对返回对象/use、初始化和 membership，不在 Publish 返回时确认出生完成 |
| Init/Publish | 基 Init 正常不等于派生 Init/外层 owner 完成；重复 Publish 仅是新的 membership 观察。七类派生、非析构池取得和 FIRST_OBSERVED 规则沿用 ST3–ST5，不补构造/出生历史 |
| NPC Morph `0x6BD0C0` | 参数与当前 type/variant/subtype/champion 相同可早退；主体修改类型和配置等状态，**没有直接执行完整 Entity/派生 Init 的常规创建链**。同物理对象的 revision/可读依据需更新，不凭 Morph 名称创建新物理身份 |
| Pickup Morph `0x6DFF30` | 可推进本对象 RNG；保存部分字段→真实派生 Init→恢复 seed/Index/位置/引用等→后处理。内层 Init 返回时仍未完成 Morph，不能提前重写出生事实 |
| ManagerRestore `0x993A90` | 聚合 owner，可恢复多名 Player、构造/Init backup 或保留 Player，并重接引用和成员。单名 PlayerRestore 的返回不等于聚合恢复结束；保留对象不自动成为 active roster |
| Replace `0x995E10` | 既有 roster 交换，也有寻找旧项并换成 replacement；双侧 `+267C` 为调用期关系，Publish 后仍有引用迁移、PlayerRestore/PostLevelInit 和 familiar 等处理，结尾清关系。要保存 old/replacement、真实返回及前后 roster，不把旧对象退出 roster 当物理销毁 |

`CURRENT_SOURCE`：`CustomCallbacks.cpp:1192–1317` 的 NPC/Pickup Morph Hook 支持回调改参，也支持 false 时直接返回而不调用 super；后置 callback 在 super 后。NPC 原生 AL 的 0/1 不等于官方 void API 的成功返回契约，不能从 void wrapper 读取一个“成功 bool”。观察层正常、下游返回、原生 body 到达、最终参数和最终状态分别记录。同优先或内层位置不能单独证明整链后处理完成。

短路不等于整个 Hook 链零变更：Lua 回调本身可能改变对象。未到 native body 时不能伪造原生 Morph/Init 完成或按请求认定新 revision；实际已观察的分类、字段、生命周期变化及当前采样污染仍需如实记录。NPC 原生相同参数早退也不代表外围 callback 没有行为。

主审纠正代理初稿的 Replace 条件倒置：`0x995E57 cmp byte [replacement+170h],0`，`0x995E64 jz 0x995F37`。**非零**才落入 roster/index swap；**零**才进入查找 old 并替换/Publish 的路线。只据该 byte 的实际分支，不臆定它完整等同于“登记状态”。代理已同步修订正文与动作说明，主审已回读，初稿的反向解释不被接受。

`DESIGN`：call/parent、enter/normal/exception、物理/use/revision 与 pending owner 分开；嵌套创建可独立记录，但在相应外层 owner 结束前不授予完整就绪。异常只撤销观察资格并保留失败事实，观察 wrapper 调下一层 exactly-once，原生异常传播不变；下游合法短路时不额外补调原生 body。不能因外层正常返回而恢复已被内层 Remove/Restore/复用撤销的旧依据。

### 2.2 Grid 与门

`STATIC_CONFIRMED`：`Room::SpawnGridEntity 0x7E21D0` 先检查 index `[0,447]` 和原生占用/类型条件；`0x7E2232` Factory 成功后，调用旧槽对象删除包装，`0x7E2253` 写新槽，再写 variant/VarData，`0x7E2260` GridInit，最后 bool=true。新指针已发布到槽的时刻早于初始化完成；失败返回不凭请求推断发生替换。

`Room::MakeDoor 0x7E4DE0` 的 door 槽已有值可早退；否则从映射的 Grid 槽识别既有 Door，或分配 Door 并删除/替换旧 Grid。`0x7E4F7B` 写 Grid 槽，随后初始化/设置门数据，`0x7E4FE7` 写 door 槽。该路径有内联基 Grid 初始化过程，不能假设必经 `GridEntity::Init 0x705070`。同一指针保留 Grid/door 两份 membership，只算一个对象/use；复用 Door 也不重记物理出生。

`RemoveGridEntityImmediate 0x813BD0` 对越界、空槽和 `type=1 && KeepDecoration` 可无事返回；实际删除分支先在 `0x813BFF` 清 Grid 槽，再在 `0x813C0B` 调 slot 0(flags=1)。请求不等于退休，Grid 槽退出不证明所有 door alias 均已退出。最终状态必须读实际来源，不能删除诊断里尚存的别名来制造一致性。

REPENTOGON 的 GridSpawn 回调 1100 还可改参、返回 false 或转入 `SpawnGridEntityDesc`；因此一般 GridSpawn wrapper 返回和直接原生 body 成功分列。SDK `GridEntityPlus.cpp:42–52` 在 Factory/Init 周围另有扩展数据操作，必须等相关整链返回。

### 2.3 夹具范围

具体动作、参数与可达入口以[ST6 笔记](tx-04-08-st6-agent-notes.md)为索引。主审直接确认 `LuaRoom.cpp:33–64`：`SpawnGridEntity(index,type,variant=0,seed=index+frame+1,varData=0)` 返回 bool，Lua 层拒绝 index 不在 `[0,447]`；受控场景应显式传 seed。`RemoveGridEntityImmediate(index,pathTrail,keepDecoration)` 的 PathTrail 为必填整数，不能误套默认参数。删除/替换会改变碰撞、路径、房间状态和 RNG/回调，不是只读观察。

代理定位 Lua 注册后，主审另读 [Spawn 公开桥](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-lua-spawn.json)：`Game:Spawn(type,variant,position,velocity,spawner,subtype,seed)` 对应 `0x424340`，把原生第八参固定为 0 后进入 `0x6FA210`；`Isaac.Spawn(type,variant,subtype,position,velocity,spawner)` 的 `0x8559D0` 先调用 `0x6EB4B0` 取得 seed，再走前者。显式 seeded 场景应使用已核对的 Game 公开桥，不把两种 API 的参数顺序或 RNG 副作用混用。当前 F8 夹具没有这些 Spawn/Morph/Grid 动作，尚需后续实现，不能把 S4/S5 设计表当作已有执行能力。

代理初稿未找到 Morph Lua 入口，主审补齐[注册](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-morph-registration.json)、[注册 adapter](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-morph-adapters.json)和[Lua closure](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-morph-closures.json)：NPC 的 `0x85180E/0x851813` 将 `0x6BD0C0` 与字符串 Morph 送入 `0x86D190`，注册 closure `0x897480`，从 Lua slot 2–5 读四个整数；Pickup 的 `0x850FB3/0x850FB8` 经 `0x86B040` 注册 closure `0x896C30`，slot 2–4 为整数、5–7 为 bool。两 closure 都返回零个 Lua 值。因此 `npc:Morph(type,variant,subtype,champion)` 和 `pickup:Morph(type,variant,subtype,keepPrice,keepSeed,ignoreModifiers)` 的**公开路由已静态确认**；具体合法对象/类型和受控场景仍 UNKNOWN，不能说已有安全夹具或 Lua 调用返回成功 bool。

主审再读 [seed helper](../../../artifacts/repentogon/tx-04-08-static/st6-st8-review/main-spawn-seed-helper.json)：`0x6EB4B0` 通常路径读写全局 RNG 数组/索引并递增，还会分配/维护调试追溯列表及采栈；另有 flag 分支调用 rand。故 Isaac.Spawn 的自动 seed 不是只读取值。这不证明未来运行时选择哪个分支，也不保证显式 Game seed 能消除 Spawn 内其它全部 RNG/回调副作用。

只允许将已核对、来源明确的临时实体/场景动作列为未来候选。Player Replace/Restore 要通过真实游戏 owner 和有效保存状态取得参数，不能伪造 GameStatePlayer、任意地址或绕过 Join 再创建 B。门别名优先用自然切房观察；强迫池饱和、直接析构、任意类型 Morph 与所有角色/变体覆盖仍不能从本次静态资料外推。静态可达不是新的实机授权；未来整体 E2E 启动说明必须列出实际动作和持久影响，按既有范围确认。

## 3. ST7：读取依据与 tx-04-07 生命周期对齐

### 3.1 当前 Player 依据尚未实现为 world 契约

`CURRENT_SOURCE`：`consumers.cpp:218–281` 已有外层 PlayerScope、`PlayerNativeReached` 和正常返回分支。`player_update=-10000` 与 `player_native=10000` 包住官方 Player 更新 Hook；后者是已检阅官方链下游的到达证据，不能对未知第三方链宣称无条件等于原生已运行。回调短路无 body 到达不授予读取依据；OddPlayer 的真实 Player 更新可以满足当前 Player 层的依据，但仍只有 gap，不能取得完整世界样本。

主审特别区分：现有 `player_fields_known` 是少量输入诊断字段读取结果，**不是 world 的 78 项字段初始化或当前 Player 读取依据**。其条件只检查 BoundValid、generation、player 相同，且读取发生在 `RefreshLifecycle(*r,false)` 之前；它没有保存并复核整个 exact pair、room/run、query epoch 与 recording/use。

`DESIGN`：world 在有效 Authority 内保存 `{recording/run/room, outer, tick, session_generation, participant_generation, exact_pair, object/use, epoch}`，确认真实 Player body 正常返回，再收束本次生命周期刷新、复核这些值仍有效后，才能标 `USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS`。后续同 outer 或采集前发生任何失效须撤销，不凭 metrics、绑定成功、回调正常或此前完成的 tick 续用。当前事实只提供接线位置，没有实现这些 world 通知。

### 3.2 撤销边与恢复顺序

| 触发 | 已有源码/静态事实 | world 接线要求 |
| --- | --- | --- |
| pause/console/loading enter/leave | `RefreshLifecycle` 调 InvalidateBinding，清快照/edge 和 peer window，并推进 query epoch；owner 返回还 defer 当前 loop | 在资格变更处同步撤销；离开阻断后需新的唯一绑定及 Player 正常更新，不延用旧依据 |
| 房间/生命/exact pair 变化及 NewRoom 返回 | `lifecycle.cpp` 对应失效，room 身份变化和延后重绑已有处理 | 撤销同指针也可能失效的依据，记录 room epoch/mutation；不能只比较 Player 地址 |
| B dead/ghost/lost | `InvalidateRemoteBinding` 只撤销 B，保留受控地址诊断，正常 A Authority 可继续 | 不把 B 死亡泛化为整个 C/S 停止，也不继续读取旧 B 字段；完整 profile 不足仍明确 incomplete |
| B revive | `0x7CFC88/0x7CFC8F` 一开始清 ghost/dead byte，随后大量状态恢复、Spawn/effect、关联 Player 路径；源码等待 revive 调用返回、稳定拓扑和唯一 exact pair | 原生 ghost=false 不能提前授予恢复完成；重绑保留 ordinal/K，读取依据仍需新代次 Player 更新 |
| Replace/Restore | 可原址改字段、关系、roster/附属 Player；不一定换 Player 地址 | owner 进入就污染/撤销，外层正常结束仅解除待定；不能补造 Init/出生或直接恢复旧 witness |
| Stop/fault/peer lost/run teardown | StopSession 进入 retiring，EndManager 随后中性化/移出 K、确认 B、退休 participant；Game::Exit 先关闭会话再原生清 Player | 在 stop 请求即关闭有效 Authority 资格；退休前不再复制完整样本，仍按 recording 独立生命周期观察退休事件 |

`core::Participant::Invalidate`（355–405）递增 generation 并清绑定/队列/current/previous资格；`Bind` 对已失效后的新绑定并不再递增一次。因此不能用“出现 Bind”作为物理新对象出生，也不能假设每次 rebind 连增两代。保留代次与事件序号的真实值。

代理另外提出的 B 不可读/不唯一分支，主审已核对并保留为**具体源码缺口**：`ObserveRemoteLife:200–202,245–246` 仅返回 `Invalidate`，唯一调用点 `RefreshLifecycle:314–320` 只显式处理 `PeerLost`，本轮 blocks 又主要按 A 计算。这些分支没有共同的 B generation 失效动作；不能把“返回 Invalidate”写成“已撤销绑定”。ConsumerScope 虽会刷新生命周期，`OverrideQuery` 仍主要核对已保存绑定、device/key、context/tick，`RuntimeHealthy` 不另复读 B topology。因此 future world 必须独立拒绝不可读/不唯一的 B，并立即撤销自身 witness，不能只监听现有 generation 变化。这里确认的是源码控制流及 world 接线风险，**没有实机复现或证明输入泄漏，也未在本轮修改既有输入实现**。

### 3.3 首次菜单 Continue 与 run teardown

首次菜单 Host 资格由 `NoteLoadingOwnerEntered(StartOwner)` 的 Host/Warmup、未见 run、未 Join、K=Empty、双方未绑定及无 retiring/failure 等共同决定。当前 `RunActive=false` 可以保留初次 Host；经历有效 run 后再离开才是 teardown。不能将所有菜单观察都视为新 run 或一概退休。

Continue 的 GameRestore 是 `0x6F43A0`；内部 `0x6F4A58` 调 ManagerRestore `0x993A90`，之后 LevelRestore，再 `0x6F4A6C` 调 Manager PostLevelInit `0x993FA0`。单次 ManagerRestore 或 PlayerRestore 返回都早于整个 Continue owner 完成。`0x6F65D7` 的 Game::Exit 调 PlayerManager 清理；run 域退休与独立 recording 域关闭分开。

tx-04-07 的 D31 正式接受保持原样，不能转写为所有 Continue/teardown 场景本轮重新实测。e2e-11 的 S18–S21 未运行、此前各轮 Continue 的限定证据和原始 FAIL 也保留。六 tick Shooting 恢复仅影响 consumer，在新 world 接线中不延迟 Authority、不复用旧 held、不增加当前 Player 的读取特权。

## 4. ST8：Hook、预算与退出

### 4.1 定义、注册、安装与共享

`CURRENT_SOURCE`：`HookSystem.cpp:167–171,234–275` 用函数名与 C++ type 组成键登记/查找定义，没有按目标地址自动去重；定义存在本身不安装 detour。`Isaac::CreateGridEntity` 和全局 `CreateGridEntity` 虽同址，但各有独立元数据。world 应复用前者 ECX/EDX 的正确 ArgData，不能为后者注册额外入口 owner。其误元数据的风险保留，未运行验证。

Hook 按较大 priority 先安装，后安装入口包住已安装链，因此调用时低数外层；相同 priority 的相对顺序不作为稳定契约。`Install:563` 按定义键取元数据，`:570–571` 的 CanHook=false 还会返回成功但不安装。当前 registry 的 `registered=true` 只表示已构造 FunctionHook 进入待安装表，不能当已安装/已调用成功；能力检查、注册统计、目标入口与实际运行证据分列。

当前 hooks.json 为 41 行、40 个不同 RVA，PlayerUpdate 两层同址是有意设计；另有 12 处 byte patch 和 8 个 pointer hook，native_dependencies 不算 Hook。pending 固定 64 项且超限静默忽略。19 项是观察语义，不是最终新增数；`41+19=60` 只是候选算术，若新增 Shutdown 或补派生覆盖也要登记。最终实现须在生成和注册阶段显式报告容量不足，统一共享 wrapper 与清单，不能用旧 70 槽或预定总数声称闭包。

### 4.2 32 位资源预算

旧头文件的静态布局声明为 Field=48、Object=2592、Batch=43,516,368、Shared=136,316,688 字节；这只是源码声明，本轮没有运行 sizeof 或构建。三槽与事件之外再照搬旧外进程队列会违反新 256 MiB 总预算，不能据功能上限相同就保留两套膨胀 DTO。

新方案 64 MiB/64 项队列必须同时计入 reader 预留、队列、writer 在途与事件；JSON 编码 scratch、captures/events I/O、identity/radix、控制记录、线程/分配管理开销都在动态预算账本中。decoder 的 128 MiB 单条限制属于离线工具限制，不是游戏进程预留第二份 128 MiB。实现前按实际纯值布局列上界，检查乘加溢出和全部在途 ownership；256 MiB 是上限，不是保证能分配成功的证明。

主审读取当前 unpacked 输入 PE：Machine=`0x14C`、Magic=`0x10B`、Characteristics=`0x122`（LAA=true）、SizeOfImage=`0x85F000`。它证明此输入为 PE32，不证明部署 PE、实际 OS 地址空间、堆碎片或某块连续分配可用。实际峰值/吞吐/准备失败和 2,000 次结果留整体 E2E。

### 4.3 正常退出窗口的实际限制

主审按 SDK `Isaac.zhl:46–47` 完整特征唯一定位 `Isaac::Shutdown VA 0x982C90 / RVA 0x582C90`。其 caller `main 0x90AC60` 先循环 ManagerUpdate/Render；退出循环后 `0x90AFCE` 直接调用 Shutdown，再平台清理/CoUninitialize/return。**之后没有新的 Manager 边界**。

Shutdown 的 `0x982DD8` 可调 Game::Exit(1)，`0x982E1C/0x982E45` 随后析构 Game/Manager 并释放/清根。SDK ImGui.cpp:777 的共享 Hook 只设置 shutdownInitiated 然后 super；当前 C/S 清单没有 Shutdown Hook。Game::Exit 是 run 退出，菜单退出进程也不保证再次经过同一个 Game::Exit。

因此，现有源码没有实现“退出自动获得十秒 drain 窗口”。主线程等待=0、后台默认 10s 和进程正常退出前完全 drain，不能仅靠 Shutdown 通知同时自动保证。已能明确的健康整体验收路径是在原生退出前显式 stop，真实 Closed/flush 后才退出；Shutdown 可作为最终关闭新 publication、保存已有终态/未交付范围的常驻共享观察点。未及时完成则报告 incomplete，不能伪造 Closed。**所有直接窗口关闭都保证十秒排空的退出协调仍未解决**，不能擅自阻塞主线程、修改原生退出语义，或以显式 stop 的覆盖代替所有正常退出路径已实现。这是原设计的具体实现缺口，不是降低 V9 要求或新的用户决定。

DllMain/loader lock 不 join、不大文件写入、不释放在途上下文；现有 `FinishTraceOnProcessExit` 只是 trace 最后尽力写，不是 future world drain。Hook/DLL 常驻、K NEVER、崩溃或强制退出 incomplete 的规则不变。

### 4.4 纯构建入口

主审静态读取 `scripts/build.ps1`、`mod/CMakeLists.txt`、`tools/harness/Cargo.toml`：入口执行 CMake configure/build、可选 SDK libzhl build、cargo build 和拷贝产物；mod custom command 运行绑定生成器，没有 ctest/cargo test/harness E2E。正常必要代码生成/编译不等于执行功能测试；本轮没有执行这个入口，也不把静态阅读当作构建成功。

## 5. 主审结论与具体未完成项

**接受 ST6–ST8 在本文限定范围内的静态结论；主审已逐份回读代理笔记和原始材料，并独立取得关键 IDB/源码证据。**

| 子项 | 主审处理与接受范围 |
| --- | --- |
| ST6 | 回读 Spawn/Init/Publish、两 Morph、ManagerRestore/Replace、Grid/门的原始函数；纠正代理 Replace 分支方向、区分 MakeDoor 直接早退与复用；将原先 UNKNOWN 的两 Morph Lua 路由补为静态确认，并补齐 seed helper 副作用。不接受任意 Player 指针夹具；合法对象/参数和 Replace 自然动作仍 UNKNOWN |
| ST7 | 接受 Restore 分层、Revive 提前清 ghost、首次菜单 Preserve、run 退休及新 witness 接线要求；主审复核并扩展 B Invalidate 返回未统一处理的源码证据。明确 metrics 读取早于生命周期复核，不能拿它代替完整依据。既有输入动态影响没有实机结论 |
| ST8 | 回读 HookSystem/loader、registry、双 GridFactory 元数据、构建与旧布局声明；独立定位 Shutdown/main 和 PE 头。要求纠正 source/IDB 证据标签、ImGui Hook 名称、离线128MiB归属和预算参照来源。不把 registered、名义容量或算术余量当运行成功 |

入口、主方案与 E2E 设计已同步，旧设计审阅、ST1–ST5 及 tx-04-07 的历史结果保留其原时点。本次通过资料阅读、检索和文档差异回读收束，没有对每个 ST 另运行测试，也没有消费或替代功能完成后的整体 E2E。当前工程未实现 world 观察、读取依据、容量报告、存储或退出协调；本报告的设计约束不能标作运行 PASS。

保留 UNKNOWN：所有原生/第三方派生及 EH/SEH 路径的完全覆盖、原生 body/整条 Hook 链的实际到达、所有 Replace/Restore 角色夹具和参数来源、未自然观察到的池复用/短命对象、实际线程与内存可用量、异步收口/文件失败/退出时序、完整稳定段和双进程比较。它们直接进入后续实现说明和唯一整体 E2E 的实际覆盖账本，不设新 gate、不追加子项测试。
