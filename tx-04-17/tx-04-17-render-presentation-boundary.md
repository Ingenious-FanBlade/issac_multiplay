# tx-04-17 ST4：render、HUD、动画事件与展示边界

**2026-11-12 结论调整**：按用户决定，Client 改为包装放行原生 `Game::Render`（显示委托），并放行 Client 路径上的 Lua 回调分派（Q6）。本文“Client 不应原样执行完整 native render callback 树”和“隔离 PostRender／HUD 1022/1024／PRE_*_RENDER”两项结论不再执行。本文的 J273 事实保留，可作为 RE-5、RE-9 的起点。见[客机显示委托与按需采集修改方案](../binary-state/client-display-delegation.md)。

**2026-11-12 RE-5 定位纠正**：下方 Tear 行的 `+420/+424` 来自伪代码十进制 420／424，实际是公共屏幕缓存 `+0x1A4/+0x1A8`，不是子类 Scale 字段 `+0x420/+0x424`。`0x675BE0 lea ebx,[esi+1A4h]` 与 `0x675C0B／0x675C0F` 的写入已复读。该函数暂存并恢复的 44B 颜色实际是 `Entity+0xF0=Entity._sprite(+0x48)._color(+0xA8)`，不是公共 `Entity.Color+0x1B8`；`0x675CA7—0x675D89` 与 SDK 已交叉确认。原行保留错误记录，后续按[RE-5 写集](../binary-state/client-display-delegation/RE-5/write-set.md)定位。本文 Room／Minimap 手工 pass 建议也按现行原生 Render 委托取代。

## 2026-11-01 当前设计与补充研究

现行合同与执行顺序见[tx-04-17主流程](tx-04-17-workflow.md)。R1／R2已形成初始化范围与对象生命周期合同，R3形成[维护／绘制分派静态合同](tx-04-17-native-maintenance-rendering.md)；本轮[R4资源与在途寿命](tx-04-17-native-resource-lifetime.md)补原生析构／Morph资源变化、容器借用、Image保留及实际CPU使用退场。custom-render延后上下文分支当前关闭，只记条件风险。R5已补[失败退休与菜单交还](tx-04-17-failure-retirement.md)，R6已形成[接线归并](tx-04-17-model-role-applier-integration.md)，原生实现／统一E2E仍未完成；本轮只改设计文档，没有修改代码、构建或运行测试。

现行方案优先复用原生Room／Entity／Player／HUD与绘制。服务器最终值通过create／remove／change写入这些Client对象；必要的本地显示维护允许执行，独立Sprite只服务UI和明确的辅助表现。

- 下文“仅自有呈现对象可写、不得写Game／Room／Entity／Player”的设计限制已被替代。客户端原生对象是服务器状态的承载者；限制落在自主玩法裁决、重复事件、未知回调及访客进度，而非禁止一切原生对象写入。
- 依[R3维护与绘制](tx-04-17-workflow.md#r3)为实际保留的Game／Room／派生Entity／HUD链列出输入、必要缓存、允许写入、阶段和副作用分派；既不整树无条件透传，也不因发现局部写入便退回逐个独立Sprite重做世界。
- 动画姿态与开火／掉落消费者分开；原生显示时钟不推进Authority tick，声音、闪光等按唯一触发者与事件水位处理。HUD所需权威状态由应用器供应，必要布局／显示维护按实际合同运行。
- [R4资源与借用](tx-04-17-workflow.md#r4)承接原生实例与内嵌Sprite寿命；具体Player组合、Laser等全内容适配仍由18—20／27／29实现。渲染字段暂存的正常恢复不扩展为异常／重入保证。

**R3采用范围（2026-11-01）**：优先复用原生对象、排序与绘制pass，拆开目标提交、移除、显示缓存和draw。下文“一律不保留HUD PostUpdate”是旧选择；本轮改为按PlayerHUD、HistoryHUD、StatHUD的输入与消费者选择受控维护，不能整体放行。`render_sort`非空／scratch／渐进排序限制、Entity原生Interpolate与碰撞、Camera窄setter及DoUpdate混合链已补证。Room PreRender包含RenderShadows、light／pass维护，不能删掉或当成Interpolate；通用render Lua及共享动画帧暂存仍需隔离。详细阶段、时间／音画去重、R1／R2反馈和未决项以R3正文为准；静态范围不是逐类renderer已完成。

## 2026-10-31 历史研究正文

以下保留当时事实、纠错、UNKNOWN和设计推导；冲突设计按上文及主流程执行，静态结论不升级为运行结果，历史分工不成为新要求。


日期：2026-10-31，Asia/Singapore。状态：**ST4指定静态研究及主智能体审阅完成**。由本任务指定的 GPT-6 Luna/max 子智能体研究，主审独立复核关键来源与 J273 地址并补证；子智能体唯一新建文件为本文。未运行游戏、未构建、未测试。本报告是 ST4 设计输入，不代表 Client 场景或隔离已实现。目标版本为 Repentance+ v1.9.7.12.J273；只读目标与[ST2](tx-04-17-client-scene-lifecycle.md)相同，为该文记录的 v12 目录中 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，未切换或写入IDB。imagebase 为 0x400000；文内二进制点位记 VA，RVA=VA-0x400000。

证据标签：SOURCE 表示本地 REPENTOGON 源码；J273-STATIC 表示目标二进制中的静态指令、伪码或已匹配 ZHL 签名；DERIVED 表示据静态调用关系提出的设计约束；UNKNOWN 表示静态材料不能回答。03-11／03-12 的 J460 仅作为查找动画事件和派生 render 的索引，本文地址只记录 J273。

## 1 结论与 Client 边界

Client 可以执行自有界面的有限绘制、已加载资源的本地姿态设置、按权威状态驱动的展示适配，以及去重后的权威音画事件。允许写入的对象应属于 Client 的呈现层：自有 Sprite／LayerState、呈现缓存、屏幕或世界绘制变换、图形 pass 状态、动画相位和本地音频队列；不得写入本地权威 Game、Room、Entity、Player、道具、血量、掉落、进度或共享玩法 RNG。

不能把 Game::Render、HUD 更新链、派生 Entity::Render、Lua dispatcher 或动画事件消费者整体视作“纯显示”。J273 中能确认的路径含有逐类 Lua 回调、HUD 音频与内部更新、派生 Entity 字段暂存、动画事件驱动 Projectile Spawn；本轮也没有闭合全部 Render helper、Entity 子类和动态 Mod callback。因此 Client 不应原样执行原生 Game／Room／实体 Update 或完整 native render callback 树。后续可保留已经逐项审定的 draw helper；其数据来源和可写对象限定为呈现副本，callback 和玩法消费者由边界显式隔离。

一个自有 Sprite 的菜单展示只适用于连接／等待等屏幕 UI，不能代表 Room、Grid、Camera、玩家组合、实体、HUD 或原版所有特殊 pass。ST2 的菜单入口、ST3 的整段 Manager Update 短路与生命周期结论见[场景根与生命周期](tx-04-17-client-scene-lifecycle.md)和[更新隔离](tx-04-17-update-isolation.md)。

## 2 J273 绘制路径：绘制调用不等于无副作用

### 2.1 Game 与 Lua 的顺序

| J273 地址／入口 | 静态可确认事实 | Client 处理 |
| --- | --- | --- |
| Game::Render 0x6F7A80；0x6F7AE1 调 0x803E20 | 在 Room 绘制前执行的 helper；该 helper 的全部 side effect 本轮未闭合。 | 不以进入 Game::Render 为“已经只读”的证明。 |
| 0x6F7DF4 调 0x804500 | Room render 候选调用。Game::Render 还有后续 Lua、HUD 等路径。 | Room／Grid 绘制由 tx-18 提供快照驱动的适配；不得调用模拟更新来准备 draw state。 |
| 0x6F7E02 调 LuaEngine::PostRender 0x849DD0 | J273 唯一完整 ZHL signature 匹配到 LuaEngine::PostRender。该 dispatcher 依条件 flags&4 选择性分派 callback 参数 2；0x849E2B→0x8591D0，0x8591EC rawgeti、0x8591FF／0x85920B push integer、0x85921B pcallk，之后 0x849E3B unref。 | 这是 Game render 主体与 HUD Render 之间的 Lua 分派；不是 HUD 绘完后的回调。Client 不得无条件调用原生 registry callback。 |
| 0x6F8108 调 HUD::Render 0x97C300 | ECX 指向 Game+0x1D9AC 的 HUD。HUD Render 前后分别分派 callback 1022、1024，源码仍无条件调用 super。 | 仅在 HUD 数据已改由权威状态供应、callback 已受限时保留绘制；HUD::Render 不能替代 HUD 状态应用。 |
| 0x6F9267 调 0x8587A0；随后 0x6F9272 调 0x687730 | 0x8587A0 是条件 lua_gc(L,3,0) 与内存统计／绘制 helper，不是 PostRender callback。0x687730 是 console 绘制候选。 | 不把 GC 统计 helper 错标为 presentation callback；console 绘制另归本地 UI。 |

Game::Render 在 0x6F7D38 写 Game+0x25AB8=1；另有从 Room 描述+0x5C 读取 seed 并做 xorshift-like 混合计算的路径，本轮没有看到 seed 写回或共享 RNG 推进证据。该写入／计算不构成“Client 应推进玩法时钟或 RNG”的依据；具体字段语义和全函数其它副作用仍应保持克制表述。

### 2.2 HUD 更新、绘制与事件回调

| 路径 | 源码／目标静态事实 | 决定 |
| --- | --- | --- |
| HUD::Update 0x97ACF0 | SlideIn／SlideOut 条件路径构造初始 Sound 252，经 0x92FAD0 转换，在 0x97AE75 调 SFXManager::Play 0x907C60；该 Play 具有独立静态声音样本选择状态 dword_BD0F40。 | 不将原生 HUD::Update 整链作为无副作用本地动画时钟。服务器世界/HUD结果音效带权威事件身份并由客户端去重；Client 自有连接、设置等产品 UI 可由本地 UI owner 播放不触及玩法的反馈音效。静态证据只证明这条条件声音路径，不证明每帧都播放；不全局禁音。 |
| HUD::PostUpdate 0x97AE90 | 静态调用逐 PlayerHUD helper 0x832C10、0x8304B0、0x82FF40、0x83C860（this+22964）；不只是绘图。 | Client 不保留原生 PostUpdate 更新链；HUD adapter 将权威血量、资源、物品、地图等字段转为绘制态。需用 tx-27 明确实际支持的全部 HUD 项。 |
| HUD::Render 0x97C300 | SOURCE：CustomCallbacks.cpp 在 super 前 dispatch 1022、super 后 dispatch 1024；源码没有据 callback 返回值取消 super 的逻辑。 | callback dispatcher 本身仍执行任意已注册 Lua；不能以“render callback”命名推定脚本只读。 |
| PlayerHUD::RenderHearts | SOURCE：PRE hearts 1118 若 Lua 返回 true 则直接 return、取消原生 hearts draw；POST hearts 1091 在 super 后调用。 | 取消极性是该 wrapper 的具体语义。不要把 hearts 的 true=取消外推到其它 render callback。 |
| Inventory 等特殊 HUD | SOURCE：RunRenderSpecialHudElementCallback 的 pre 路径返回 bool；RenderInventory 等只有 helper 返回 true 才执行 super 与 post，因此 pre 返回 false 可取消 draw。回调参数还包含 Player userdata。 | 与 hearts 极性不同。每个 wrapper 必须按源码逐一解释；没有明确白名单时，Client 隔离该 Lua 分派。 |

SOURCE 行号：HUD Render 在 CustomCallbacks.cpp 220—242；hearts 在 2827 起；Inventory 及其它特殊元素在 2913 起。具体调用和返回值见[REPENTOGON CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp)。

### 2.3 Entity render 与 callback 是有对象能力的 Lua 调用

REPENTOGON 的 Entity_NPC／Player／Pickup／Tear／Projectile／Familiar／Knife／Effect／Bomb 等 Render wrapper 会在 super 前把 Entity userdata 和 offset Vector userdata 交给 Lua。SOURCE 中 NPC、Player、Pickup、Tear、Projectile 等通用 pre-render wrapper：Lua 返回 boolean false 时直接 return、跳过 super；返回 Vector userdata 时可替换传入 offset；无此返回则调用 super。它们把原生游戏对象暴露给 Lua，不能形成写权限沙箱。源码位置：CustomCallbacks.cpp 1742、1771、1800、1829、1858、1887、1916、1945、1974 起。

J273 的实际派生路径还含额外 Lua helper。IDA 静态调用点／参数核对结果如下；参数常量仅作为 helper 实参记录，不按 rawgeti／pushinteger／pcallk 序列推断成公开 callback 名或公开 API ID：

| 派生 render | J273 静态链与对象写入 | 设计限制 |
| --- | --- | --- |
| Entity_Player::Render 0x784A60 | 大型组合绘制函数，完整附加对象／资源／Lua子树未闭合。 | tx-20 必须适配玩家身体、角色组合及相关附加显示；不能以单 Sprite 取代。 |
| Entity_NPC::Render 0x6CEC90 | 大型多 helper 绘制函数；0x6CFE4C→0x8470C0（分派上下文编号 28、条件 flags&0x10000000），0x847129 调 0x859360，后者于 0x859452 pcall。J273 参数、NPC 类路径与本地 ModCallbacks Wiki 对应 MC_POST_NPC_RENDER(28)。 | callback 的实际注册函数与全部 NPC 派生分支未闭合；不直接作为纯展示树。 |
| Entity_Tear::Render 0x675AC0 | 写本体+420/+424显示坐标并暂时更改颜色；0x675DE7→0x849210，关联 helper 0x842560 push 分派上下文编号 41，条件检查 flags 字段偏移+4中的 0x200 位→0x84931A pcall。J273 参数、Tear 类路径与本地 ModCallbacks Wiki 对应 MC_POST_TEAR_RENDER(41)；路径含 base/shadow helper 0x6AC4E0。 | 位置／颜色写入需在呈现副本或可证明恢复的窗口进行；callback 与阴影层不能静默丢弃。 |
| Entity_Projectile::Render 0x659460 | 0x659484 暂时从 Entity+0x350 减 Entity+0x410 的分量，0x6597D1 正常尾部再加回；局部 frame/entity 数值参与抖动计算，本函数未见共享 RNG helper 调用或 seed 回写。0x6597DB→0x848CF0（分派上下文编号 45，条件检查 flags 字段偏移+4中的 0x2000 位）→0x848D5A→0x859E30→0x859F22 pcall。J273 参数、Projectile 类路径与本地 ModCallbacks Wiki 对应 MC_POST_PROJECTILE_RENDER(45)；draw 路径还调用 base renderer 0x6AC4E0。 | +0x350 暂存不是权威位置；该代码没有保存旧值作为恢复源，不能据正常尾部声称异常／重入也安全。局部数值计算不等同 Authority RNG。 |
| 其它 Entity 类 | 当前证据未逐类闭合 Bomb／Laser／Knife／Familiar／Pickup／Effect／Slot 及附件树。 | tx-19 负责完整非 Player 类；未知类别不能伪装成 NPC 或丢掉。 |

上述分派上下文编号和 callsite 经目标 IDB、对应类路径与本地 ModCallbacks Wiki 交叉核对；不表示所有条件都在实际游戏中命中，也没有证明全映像只在这些 callsite 分派 Lua或覆盖实际注册函数及其子树。REPENTOGON 明确 PRE_*_RENDER callback 编号（1081—1088）与原版 MC_POST_*_RENDER 编号（如 NPC 28、Tear 41、Projectile 45）是不同回调，不能互换。

## 3 动画缓存与权威玩法事件分开

### 3.1 帧状态本身不等于安全事件

J273 静态点位；公开事件查询绑定可对照 [LuaSprite.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaSprite.cpp:90)：

- ANM2::SetFrame 的 ZHL 匹配地址为 0x406010；该函数将 Sprite+0x30 对应的 AnimationState 设为 ECX 后调用 0x4042A0。该 helper 清理当前事件位 state+0x18、写新帧状态，然后进入 0x404130；0x404181 对当前事件位 state+0x18 置位、0x404190 对历史位 state+0x1C 置位。
- ANM2::Update 为 0x404550，条件更新内嵌两套 AnimationState（偏移+0x30、+0x50）；helper 0x404480 可调用前进 0x4041A0 或回退 0x405B30。
- AnimationState::IsEventTriggered 为 0x406710，按动画事件名查找位并读取 state+0x18；WasEventTriggered 为 0x4067B0，读取 state+0x1C。读取接口不会替代事件消费者隔离。

因此，在 Client 侧设置姿态、推进自有装饰 Sprite 或读取事件位只是本地动画状态操作；一旦原生 Entity／Player gameplay consumer 仍运行，相同事件位就可能变成战斗、掉落、物品或生成副作用。不能依赖“这只是 SetFrame”保障玩法隔离，也不能让客户端本地事件位驱动权威动画消费者。

### 3.2 J273 Shoot → RNG → Projectile Spawn 闭环

J273 中读取字符串 Shoot 的 Entity 动作函数为 0x555E10（在 Shoot consumer 分支）。其在 0x555EED 将 Entity+0x78 的 AnimationState 交给 0x406710 检查 Shoot；false 条件分支从 0x555EF7 跳至函数出口 0x5560DD。true 分支有声音与瞄准计算；call 前 0x5560B0 将 ECX 设为 EDI（Entity 本体），并在 0x5560BD 调 0x6C74B0，mode 为 0。

0x6C74B0 的 mode 0 分支于 0x6C7513 调共享随机 helper 0x6EB4B0，在 0x6C755D 准备 projectile type 9，并于 0x6C7564 调 wrapper 0x424340。wrapper 内实际 call 指令是 0x42435D→Game::Spawn 0x6FA210；0x424364 是伪码显示的返回地址，不是 callsite。Spawn 结果经 RTTI 转为 Entity_Projectile，后续仍有对象后处理。该链证明动画事件消费者属于 Authority；Client 应应用 Authority 给出的 Projectile 和带唯一事件身份的音画表现，不得本地重跑 Spawn／RNG。动画触发／消费次数、帧间调度和所有其它动画消费者未由静态证据完整证明。

同一范围内还存在明确的权威物品／掉落 consumer 候选：Player::ControlActiveItem 0x789E50、ConsumerDrop 0x79C300、Card/Pill consumer 0x79BDD0、Bomb consumer 0x77A210；Bomb 路径在 0x77A8A5 调 Game::Spawn 0x6FA210（0x77A8A3 先 push type 4；0x77A8AF 是 mov esi,eax）。该清单不是全量闭包；拾取、商店战利品、道具效果、其它类型 Spawn、Mod callback 及其随机源仍需按 tx-19／20 和后续整局范围补齐。Client 不运行 Player/NPC 的输入、Update、事件消费者或玩法 Lua；Authority 的事件/快照是唯一玩法结果来源。

## 4 可保留路径与禁止的隐式写入

| 功能 | Client 可保留的呈现操作 | 禁止原样保留／需要的隔离 |
| --- | --- | --- |
| 连接等待／本地界面 | MC_MAIN_MENU_RENDER(1023) 中自有屏幕 Sprite 与连接状态 UI 是窄候选；公开 Sprite API 可用于显式 Load、姿态、图层与绘制。REPENTOGON 的 callback 源码在 ASMCallbacks.cpp:1528、1555。 | 不把菜单回调当 Room renderer、完整 Client 场景或 HUD 生命周期；UI Lua 必须属于已声明的自有呈现逻辑。 |
| Room、Grid、Camera | 依据已接收 Authority 值更新 Client 呈现数据、相机 transform 和本地绘制队列。Camera 不推进 Room/Game simulation。 | 不调用原生 Room／Game／Manager update、RoomTransition 或 Manager camera tail 来“刷新镜头”。ST3 已发现 Manager camera helper 条件调用共享 RNG。 |
| Player 与实体 | 消费 18—20 提供的不可变 Authority 状态，写呈现副本中的位置、姿态、图层、叠加、附件、阴影／特效和绘制参数。原生 draw helper 仅逐项审查后调用。 | 不让 live gameplay Entity/Player 或未知 userdata 作为自由 Lua 的可写上下文；不把复杂 Player/NPC/Projectile/Tear render 化约成一个 Sprite；不执行原生行为 Update 或 Spawn／Drop／Pickup／damage consumer。 |
| HUD 与音画 | 使用 Authority 的 HUD 快照生成绘制态；音效／一次性动画从有事件 ID、来源 tick 与目标的权威表现事件去重播放。 | 禁止本地 HUD::Update／PostUpdate 驱动权威状态；不重放 baseline 中的历史一次性事件；不因 HUD 更新检测本地状态变化而重复播音；不全局静音。 |
| 动画与时间 | 自有 UI／装饰 Sprite 可用公开姿态 setter；世界实体姿态从 Authority 快照提供。纯展示 shader 使用明确定义的 Authority presentation time 或不依赖模拟的本地 presentation clock。 | SetFrame／Update 生成的事件位不得被本地 gameplay consumer 消费；Client 停止 Authority Game tick 时不得改写全局 Game／Manager frame count 伪造 tick。 |
| Lua 回调 | C/S 自有 presentation 回调可作为单独、具名白名单：输入只含有限纯值／只读快照，结果仅影响本地绘制参数。 | 隔离通用 PostRender(id 2)、HUD 1022/1024、PRE_*_RENDER 与其它动态注册玩法 callback；它们可以调用 Lua API 并读写传入 Entity／Player userdata。不能用“PreRender”或“Render”为写权限白名单。 |
| Render pass／shader | Adapter 明确设置并恢复当前 pass 所有需要的图形状态、时间和几何输入；独立处理 Room 图层、实体层、shadow、beam／laser、效果、UI/HUD、特殊 shader。 | 不依赖前一个 Sprite 偶然遗留 shader、blend mode 或全局 frame clock；不把一个屏幕 Sprite 的成功显示当成 world renderer 已完成。 |

Sprite／LayerState 的公开语义及限制以[接口目录](tx-04-17-public-interface-catalog.md)为准。REPENTOGON 的 MainMenu callback 源码及两套公开 Wiki 见[ASMCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp)、[原版 Sprite Wiki](F:/projects_isaac/IsaacDocs/docs/Sprite.md)、[REPENTOGON Sprite Wiki](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)；回调名称和编号对照[原版 ModCallbacks Wiki](F:/projects_isaac/IsaacDocs/docs/enums/ModCallbacks.md)。Wiki 公共接口优先；setter 存在不代表缺失资源、全部层图像、GL context 或特殊几何已正确。

## 5 跨18—20／27的适配责任与字段反馈

事件 wire 和稳定对象身份归 tx-15；连接与 Client 主线程入口归 tx-16；本节列出本 ST 需要反馈的呈现消费接口，不宣称协议字段已经定稿。

| 后续任务 | 必须承担的适配 | 提供给15／16的接口反馈 |
| --- | --- | --- |
| tx-18 Room／Grid | 从权威 Room/Grid 状态构造完整环境绘制层：房间背景与布局、各类 Grid／门、场景装饰、RoomTransition 的呈现态、灯光／可见性与 Camera。适配读取 Authority transform/transition 值，不调用本地 room generation、相机 RNG 或实体更新。 | Room/Transition 应有可表达的布局与相机目标／过渡状态；房间切换期间需要把展示换代与资源准备失败单独表示。具体 field 定义由15决定。 |
| tx-19 非 Player Entity | 对 NPC、Tear、Projectile、Bomb、Laser、Knife、Pickup、Familiar、Effect、Slot 等完整必要类别保留原版外观与附属渲染：类资源、animation main/overlay、分层姿态、拥有者／父子引用、shadow、轨迹／beam、发光／颜色／缩放／渲染 offset。各类只适配 draw state，不运行本地 update/collision/damage/pickup/drop/spawn/RNG consumer。 | 需表达每类需要的资源 variant、可见性、层姿态、render offset 与关联引用；伤害／拾取／掉落／射击产生的实体由 Authority 应用，表现事件单独传递且客户端去重。 |
| tx-20 Player／多身体 | 保留完整角色外观组合与实体关联：本体、形态／服装、头手／武器、多人及一人多身体、鬼魂／特殊体、显示 offset、复合 Sprite／layer 与角色 shader；显示健康、被动／主动装备不执行本地消费。 | player/body身份稳定映射；需要拆分 world pose 与外观组合、HUD health/equipment presentation、一次性动画/音效事件。不要让原生 Player Render 从本地 Player simulation 反推这些值。 |
| tx-27 HUD／音画／声明 Mod | 从 Authority 状态绘制全套必要 HUD：生命、币钥弹、物品／卡牌、地图、状态图标、玩家布局与模式特有 UI；为 Authority 的一次性音画提供事件身份、生命周期和 baseline watermark。允许本地连接设置/纯 UI。 | 需区分持续 HUD state 与一次性/可重播事件，事件至少按 session、Authority tick／序号与稳定 event id 去重；baseline 不重播旧声效。声明 Mod 的 presentation adapter 必须独立登记，不支持任意 gameplay Mod。 |
| tx-17资源/context/lifecycle公共边界 | 各上列 adapter 经候选准备／失败隔离／提交／撤销，依已确认主线程和 graphics context 使用资源；逐绘制窗口保证借用层与在途资源有效。 | 不把资源 load 返回、Sprite 构造或单帧无错误等同完整 world 可发布；加载失败仍保持 Client gameplay 隔离并走正常退休。 |

这些是必要完整范围，不应以“后续可扩展”为由省略原版主游戏已需要的实体类别、角色组合、Room 及 HUD；tx-29 可承接进一步内容差异和明确声明 Mod 的额外适配，不能接走本表里原版完整可玩流程的已知展示责任。

## 6 REPENTOGON 插值、特殊 shader 与 renderer state

- SOURCE：[InterpolV2.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/InterpolV2.cpp) 以全局 _interpol_isgamerender、_interpol_state 控制是否插值。在 AnimationLayer::RenderFrame 中它会临时改写共享 AnimationFrame 的 color、scale、pos、rotation，调用 super 后再正常路径恢复；这不是 RAII/scope guard。共享帧重入、callback 异常、非本地帧和不正常退出时的恢复均 UNKNOWN。Client adapter 应使用私有插值副本或呈现缓冲，并确保完整恢复、重入隔离；不能默认共享动画缓存只读。
- SOURCE：[Anm2Extras.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Anm2Extras.cpp) 的 AnimationLayer::RenderFrame priority 9999 重写路径使用 g_ANM2_LightRendering。Dogma／Gold shader 用 IGNORE_GAME_TIME 选择 g_Game->_frameCount 或 g_Manager->_framecount/2，再取 time%256；因此冻结 Authority 更新时钟会让部分 shader 外观停住。Adapter 为视觉时间单独提供确定输入，不伪造 Game/Manager 模拟帧。
- 同一特殊绘制修改 graphics manager._blendMode 后调用 SubmitQuadForShader，正常返回时恢复 blend mode。任何保留的原生 helper 都须在静态分析／实现前明确其异常、早退与重入恢复策略；无法给出策略时应停用该路径或改用隔离副本。E2E 只观察选定场景，不能证明未审阅的路径也安全。
- SOURCE：[MinimapPatches.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/MinimapPatches.cpp:9) 说明 vanilla 某些 minimap image render 未自行设置 shader、依赖之前 AnimationLayer::RenderFrame 遗留 shader；REPENTOGON 另行补丁设置 ColorOffset。它是明确的 pass/state 耦合样例。Room／Minimap／HUD adapter 应在自己的 pass 设置所需 shader、blend 与裁剪状态；详细 context 与 batch 生命周期归 ST6，不在本报告推导 GL ABI。

## 7 未决事实、运行时留项与审阅重点

1. **回调全集未闭合。** J273 只核了主 Render 调用路径及上述关键派生点；真实动态 callback 注册、实际 Mod 集合、其它 native caller、全类／全状态／异常分支均 UNKNOWN。静态发现的 callsite 不能证明每帧可达；没有发现 Spawn 也不能证明一个 render helper 永远纯展示。
2. **原生 renderer 安全恢复未闭合。** Tear 颜色与 Projectile +0x350 的正常路径写回不证明异常、重入或回调返回早退时状态恢复；InterpolV2 显式共享 cache 临时写回更说明不能默认安全。需决定采用只在私有呈现对象绘制，还是为保留的 native helper 增加隔离的可写副本／保护窗口；由18—20／ST5—7将对象、资源、owner和退休逐项落地。
3. **动画事件次数和玩法消费者未全量闭合。** IsEventTriggered 读取 current event mask，本轮未验证实际引擎调度如何清／保持／跨帧消费；Shoot 的本地连发、其它事件名、Boss/NPC行为、所有道具/掉落/奖励与Mod写入来源仍不完整。设计按 Authority 唯一模拟器处理，不要求假设它们每帧或仅一次。
4. **HUD语义与像素结果分属静态和运行事实。** HUD helper 的完整字段语义须在 tx-27 adapter 实现前继续通过源码／必要静态分析逐项归属；全玩家布局、主菜单 callback 在当前运行安装中的 context/lifetime、字体与 shader 资源可用性及实际像素结果由完整 tx-17 E2E观察。本次不运行或称为已验证。
5. **来源与接口限制。** F:/projects_isaac/IsaacDocs 与 F:/projects_isaac/REPENTOGON-1.1.3/docs/docs 用于公开 API 语义；当前 REPENTOGON 源码用于验证 dispatcher 和适配行为；J273 IDB 静态地址不可由旧 J460 地址平移。源码 wrapper 保留 super、条件跳过和回调的行为必须按每条 wrapper 的真实 return 语义描述。

主审已独立回读上述核心链，接受本文静态事实、隔离决定和适配责任。已修正初稿将 ANM2::SetFrame 写成内部状态方法、Shoot helper 的 ECX 写成内部成员、Bomb 的 0x77A8AF 写成 callsite 等问题；明确原版 POST 编号与 REPENTOGON PRE 编号、声音样本随机与玩法随机、普通返回恢复与异常保证的区别。Game/HUD时序、帧事件与Spawn链、派生Lua、声音及shader源码的主审补证见[工作流程](tx-04-17-workflow.md#st4主审补证与采用限制2026-10-31)。未引述任何 J460 地址或实机事实，也不把 E2E 当作未解释 helper 安全性的替代证明。

本文向 ST5—ST8 和 tx-18—20／27交付静态边界；完整 Renderer、资源准备和 Client 世界壳仍由相邻任务实现与统一验收。本报告没有建立分项验收、gate或artifact。

