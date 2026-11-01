# tx-04-17 R3：停止玩法后的原生维护与绘制

日期：2026-11-01，Asia/Singapore。目标为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。状态：**本轮公开接口／源码阅读、必要静态逆向与主审形成维护分派合同；尚未实现或运行原生 Client 世界。** 同属 tx-04-17，不另拆研究 tx。本轮只更新设计文档，没有修改代码、绑定、游戏文件或 IDB，没有构建、测试或实机操作。

现行目标与顺序见[主流程](tx-04-17-workflow.md)，前置事实见[R1 原生初始化](tx-04-17-client-scene-lifecycle.md#r1-results)和[R2 对象生命周期](tx-04-17-native-object-lifecycle.md)。ST3／ST4 的普通、odd、FSM、loading、动画事件与派生绘制证据继续有效；旧“原生对象一律不可写”及“一律禁止 HUD 后处理”的设计按本文细分，历史事实不改写。

## 1 采用范围

**R6接口归并（2026-11-01）**：[模型／角色／应用器](tx-04-17-model-role-applier-integration.md)将应用结果、已发布显示状态及独立展示时间分开：维护／draw只消费一致的已提交实例和资源代次，不回写A、不推进Authority。跨帧应用只在完整子步骤间让出，影响旧owner后停相关展示；独立Component和音画事件不升级成完整世界，事件接纳与实际效果完成另行协调。未解释的实际pass／官方Hook层次仍需补齐。

**R5回补（2026-11-01）**：[失败退休／菜单交还](tx-04-17-failure-retirement.md)将当前在途栈返回、采用pass正常消费／已解释取消、原生owner与HUD资源清理分段管理。已停显不等于资源可析构，旧context不符时不为排空而恢复未知GameRender；Room重建／Player清理后仍需HUD与菜单实际后态。普通退出与进程Shutdown有不同终点，后者不能等下一轮Manager更新或依赖录制开关才接线。

**R4回补（2026-11-01）**：[资源与在途合同](tx-04-17-native-resource-lifetime.md)分开当前draw活跃栈、Image raw提交与owner析构。官方buffer复制Entity／Animation上下文的分支被EnableCustomRendering=false及注释patch入口关闭，只记未来启用风险。资源重建／Morph／析构排在采用pass的实际CPU消费结束与下一轮提交之间；Image pins只保图像。全部采用的shadow／light／HUD／loading pass纳入使用范围，未知取消路径停新发布并交R5收口，不为关闭的扩展分支增加清理前置。

Client 继续使用已有 Game 下的原生 Room／Grid／Entity／Player／Camera／HUD。服务器提供最终玩法状态；客户端可写这些对象的目标值和必要显示缓存，复用已解释的原生排序、资源与 draw helper。不运行整条 Manager／Game／Room／Entity 更新来补显示，也不另外建立一个运行中的 Game。

本轮确定的分派是：**按R2依赖执行生命周期／目标应用（含必要移除） → 成员与清理收口 → 排序／显示缓存准备 → 原生 pass 与 draw → 在途使用结束**。移除／替换与新建的内部次序由实际owner、引用和池预算决定，不要求先创建所有对象再移除旧对象。这些阶段有各自的输入与完成含义。绘制不是新的 Authority tick；排序成功不是 create／remove 成功；提交了目标状态也不等于资源或像素已经可用。

不能把 REPENTOGON 的 `MC_PRE_UPDATE=true` 当 Client 维护总入口。官方跳过分支确实执行 `CommitRemovedEntities → render_sort → HUD::PostUpdate`，但该位置之前已有 Game 工作，外层 Manager／odd／FSM／loading 仍有独立路径，HUD 后处理又包含重算与 Lua。采用各段职责，不整体借用原版“暂停一轮”的调度。

## 2 公开接口与官方源码

| 能力／来源 | 已读事实与本轮决定 |
| --- | --- |
| `Camera:SetFocusPosition`／`SnapToPosition`／`Update` | [本地 Camera 文档](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md)及[官方在线接口](https://repentogon.com/Camera.html)说明房间尺寸、调用阶段和 Active Camera 限制；Update 的 bool 无足够语义说明。`LuaCamera.cpp:15—40`直接调用 native，Snap 之后另设 `ShouldOverride=true`。公开 setter 存在不证明 native Update 只做显示 |
| `render_sort`／移除提交 | `libzhl/functions/EntityList.zhl:16—17`只给 native 声明，没有 Lua 的完整维护接口。`ASMCallbacks.cpp:70—76`给出跳过主更新时的三段顺序；`EntityManager.cpp:94—145`仅处理五类 EL，含 ClearReferences 和 Player Free。按 R2 保留 owner／连带行为，不能当纯 draw 缓存 |
| HUD 与 PlayerHUD | `HUD.zhl`分开 LoadGraphics／Update／PostUpdate／Render；`PlayerHUD.zhl`分开 Update／UpdateHearts／RenderHearts。`LuaInterfaces/HUD/LuaPlayerHUD.cpp`的 GetLayout／RenderActiveItem 读取关联 Player 与原生布局，GetHearts／GetHeartByIndex 返回心形描述的值副本，不是可写回缓存接口。资源、关联、HUD 值与绘制四者分开 |
| HUD／光照 Lua | `CustomCallbacks.cpp:188—241`在 HUD Update、PostUpdate、Render 分派 1020／1021／1022／1024，仍调用 super；`:2962—3033`的 Room 光照包装会把 Grid／Entity userdata 交给 1151／1152，false 可取消，Vector 是加到 offset。不能把这些回调当只读或与其它 PRE render 返回极性混用 |
| Sprite 姿态与动画事件 | 原版 `Sprite.md`、官方 `LuaSprite.cpp`优先提供帧／overlay／layer setter。复用 ST4 的 J273 SetFrame／Update 会改变事件位、Shoot 消费者可 Spawn／推进 RNG 的事实。允许维护姿态，不允许本地玩法消费者消费这些位 |
| 官方插值 | `InterpolV2.cpp:9—19,33—102`依赖 Game render 标志、当前 AnimationState、Manager 帧奇偶及播放速度，临时写共享 AnimationFrame 的颜色／缩放／位置／旋转后正常返回恢复；不是原生 Entity::Interpolate，也不是异常／重入安全的通用插值器 |
| shader 时间与 pass | `Anm2Extras.cpp:310—325`的 Dogma／Gold shader 依 IGNORE_GAME_TIME 读取 Game frame 或 Manager frame/2，再以 time%256 生成颜色参数；末尾恢复 blend。复用 ST4 的 minimap shader 耦合事实。展示时钟／pass 必须显式适配，不清除 visual flag、不推进全局玩法帧来凑外观 |

上述本地来源均在 `F:/projects_isaac/REPENTOGON-1.1.3/` 下；原版文档在 `F:/projects_isaac/IsaacDocs/docs/`。在线文档只补公开语义，不替代本地 1.1.3 源码／J273 版本事实。[官方 MC_PRE_UPDATE 文档](https://repentogon.com/enums/ModCallbacks.html#mc_pre_update)承诺的是取消主更新，不是 C/S 的完整隔离。

## 3 本轮 J273 静态事实

证据标签：`SOURCE`为上述源码／声明；`J273_STATIC`为当前共享 IDA 的只读原始指令、签名和伪码；`REUSED`为 ST3／ST4 已审事实；`DESIGN`为本轮选型；`UNKNOWN`不升级为运行保证。IDA 目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，imagebase `0x400000`，与 R1／R2 相同；没有切换数据库或操作调试器。以下地址为 VA，括号给 RVA。

### 3.1 成员排序是可拆出的维护，仍有前置条件

`J273_STATIC`：官方 `EntityList::render_sort`签名唯一匹配 `0x4150C0`（`0x0150C0`）。其原生直接 caller 位于 EntityList Update 的 `0x415023`，官方跳过更新分支则直接调用同一绑定。它读取 renderEL 的 data／capacity／size（EntityList `+0x54/+0x58/+0x5C`），对指针数组原地重排，不创建或补入成员。

- `0x4150E8`首次按 capacity 分配全局 scratch `0xBEBDB0`；本函数没有容量记录或后续扩容检查。不能从列表合法直接推导 scratch 足够，不能并发或嵌套使用它。
- 每个对象通过 vtable `+0x34`读取 GetRenderZ，例如 raw call `0x41510A/0x41517E`。先按 `<10000`、再对前段按 `<5000`分组，尾部进行相邻 key 比较与交换。
- **size=0 没有早退**：`0x415179`仍读 data[0]，`0x41517E`虚调用。Client 空表须由外层跳过，而不是把空表直接交给它。
- `0x41522A—238`以 `(capacity>>4)+1`初始化工作预算，并读取 Game `+0x25A70`奇偶选择遍历方向；交换后递减、在外层检查预算。内层可继续交换，不能把预算说成精确交换次数上限，也不能保证一次调用得到任意新基线的完全排序。

主审补证：base Entity vtable `0xB18224`与 Player vtable `0xB1BDB4`的 `+0x34`都指向 `0x6AC3F0`（`0x2AC3F0`）。这个已读实现从 Entity `+0x20/+0x354/+0x358/+0x35C`、当前 Room type 和位置 Y 生成分组／深度 key；`0x6AC498`调用纯数值 helper `0x6A37D0`，输入为两个字段 `+0x358`与`+0x340`之和，做 floor／下限计算。未把三个未命名字段直接称为公开 DepthOffset；派生 GetRenderZ 尚无全类闭包。原生Index参与分组内次序，不等于服务器逻辑key；相同key／属性也不自动保证两端同深度对象的tie次序一致。

`DESIGN`：保留该 helper 为非空、已提交且实例有效的 renderEL 的维护候选；保留真实 Engine frame 值，不为其奇偶另推 Game 时钟。新成员／替换／remove／深度或显示位置变化使排序变脏；每个展示维护批次最多从同一 owner 调用一次，绘制中不改表。需要立即确定基线完整次序时，仍须补明原生渐进次序与全排序策略，不能用一次 void 返回伪造“已完全排好”。

### 3.2 原生位置插值属于混合链

`J273_STATIC/SOURCE`：`Entity.zhl`把 base vtable `+0x10`命名为 Interpolate；J273 对应 `0x6AC2A0`（`0x2AC2A0`）。它在特定 flag 分支会清 flag 并返回；正常段保存位置到 `+0x344/+0x348`，于 `0x6AC320`置 `+0x175`，`0x6AC348/34C`将速度／乘数推算的半步位置写入 `+0x33C/+0x340`。Type=1 在 `0x6AC357`调用碰撞 helper `0x6A6A40`，其它类在 `0x6AC3E1`调用 `0x6A5310`；不是仅写屏幕坐标的函数。

Odd 普通循环 `0x6F957D`对非 Type1 调 slot `+0x10`；Type1 在 `0x6F9559`调 Update，`0x6F9560`另调碰撞。特殊分支另有条件 slot `+0x10` call `0x6F942A/0x6F94D8`。复用 ST3 的 Manager `0x92E8E8—8FA`恢复 staged 位置并清 `+0x175`事实。故“只跳普通 Update，保留插值／外层恢复”仍会改服务器最终位置。

`DESIGN`：Client 不执行原生 Interpolate／odd 来补平滑，不靠改 Entity flags 跳过其副作用。按目标姿态写入实际采用 draw 所需的位置／offset／暂存合同，世界最终值与本地显示值分开归属；原生 Manager 的位置恢复也继续被 ClientManagerBoundary 包住。没有维护 staged 字段的隐式历史，不让旧 `+0x175`在交还或其它入口复活。

### 3.3 Camera 的窄 setter 与完整 Update 不同

`J273_STATIC`：官方 Camera 签名分别唯一命中 DoUpdate `0x91D650`（`0x51D650`）、SetFocusPosition `0x91EBF0`（`0x51EBF0`）、SnapToPosition `0x91EC20`（`0x51EC20`）。native ABI 以 `Camera.zhl`为准，不能用反编译器的 userpurge 临时寄存器当公开参数。

| 路径 | 核定行为与决定 |
| --- | --- |
| DoUpdate | raw `0x91D66F`／`0x91D68B`分别进入 `0x91E5E0`／`0x91D840`，还按分支清 Camera `+0x8C`。这两条正是 ST3 已定位四处条件共享 RNG call 的相机路径，本轮复读 callsite 保持。Client 不把公开 Camera:Update 或原生 DoUpdate 当无副作用时钟 |
| SetFocusPosition | `0x91EC05`在未 override 时，把输入位置加入 Camera `+0x50`所指历史缓冲，并于 `0x91EC0A`置 `+0x8C`。它不单独完成整个镜头位置计算；不调用 DoUpdate“补完” |
| SnapToPosition | 重填六组原生环形历史，前五写全局 `a7`向量、最后一组写全局向量的相反值；不是把输入目标原样填满六组。`0x91EF21`经 `0x91D2E0`算目标的屏幕／clamp 偏移，写 Camera `+0x6C/+0x70`及 Room `+0x1204/+0x1208`（raw `0x91EF40/4A`）；native 清 override，Lua 包装随后另设 true |
| 几何依赖 | `0x91D2E0→0x91D140`读取 Camera 关联 Room、descriptor/config、Room边界与显示余量／屏幕尺寸，含特殊 Room 条件。Snap 不是只需一个 Vector 的无根函数；构造完成的六缓冲、当前 Room/config 与分辨率映射必须有效 |

`DESIGN`：以服务器相机目标／过渡／震动值和本地视口参数供应 Camera／Room 显示态；窄 Snap 是首次定位或明确重置历史时的候选，不按每帧反复 reset 冒充原生平滑。正常展示由显式缓存适配保持完整相机外观。Active Camera 的平滑、clamp、特殊房及换视口仍有具体缺口，未声称 Snap 覆盖全部模式。

### 3.4 原生 draw 的必要维护不等于整树无副作用

`J273_STATIC/REUSED`：Game Render `0x6F7A80`（`0x2F7A80`）在 `0x6F7AE1`调用 Room PreRender `0x803E20`，随后普通分支 `0x6F7DF4`调用 Room render `0x804500`；过渡状态另走 `0x8273C0`。Lua PostRender 在 `0x6F7E02`，HUD Render 在 `0x6F8108`，不是同一个“绘制后回调”。本轮还读到开头 pending 分支 `0x6F7ACF→0x703E40`会调用多个资源／全局 helper 并重设 Room 显示字段，故不能绕过准备合同后无条件进入整条 Game Render。

Room PreRender 的 `0x803E62→0x4154D0`匹配 `EntityList::RenderShadows`：使用当前 renderEL 与 Room `+0x1204`显示 offset，在 `0x4155E9`对实体调 vtable `+0x18` RenderShadowLayer，再按返回值走默认阴影 quad，`0x4157C3`提交绘制。**它不是 slot `+0x10` Interpolate。** 阴影路径自己有 image batch／shader state，不能取消本体 render 后就认为整个对象没有原生绘制，也不能简单省掉它。

Room PreRender 还会清理／重建 Room 的一组光照集合，对已有实体在 raw `0x804200`调 `0x7FC110`、对几何范围中的非空Grid在`0x80422E`调`0x801BE0`；两入口分别匹配官方 RenderEntityLight／RenderGridLight 完整签名。特殊分支写 Room `+0x6EDC/+0x6EE0`；尾部`0x8044DA`清`+0x11F6`后`0x8044E1→0x81BB30`，最后`0x8044E8→0x816250`。实际采用pass所需的collection／状态／缓存须明确准备，但两个尾部helper与各类光照／虚调用闭包仍为UNKNOWN，不整体标成纯函数。

复用 ST4 并由本轮复读的 base draw `0x6AC4E0`：它按 exists／visible 条件，把世界位置加 sprite offset 转为屏幕坐标，写 Entity `+0x1A4/+0x1A8`，再调内嵌 Sprite renderer；该窄函数没有运行 Entity Update。Tear、Projectile 另有位置／颜色暂存，Player／NPC／其它类有组合绘制、附件与 Lua，正常返回恢复不等于异常／重入恢复。光照、Room FX、shadow、beam、HUD 仍属必须保留的原生表现范围，不能清标志或统一替换为单 Sprite。

### 3.5 HUD 后处理分成四类职责

`J273_STATIC/SOURCE`：HUD::PostUpdate `0x97AE90`（`0x57AE90`）是公开 native 维护入口，但其四段并非全是布局。官方外围还在进入原生体前分派1021，不能只审原生体。

| 子段与实际调用 | 已证写入／依赖与采用范围 |
| --- | --- |
| PlayerHUD Update `0x832C10`（`0x432C10`），raw `0x97AEAA` | 遍历8个有Player关联的槽，每槽stride`0x6E0`。完整官方ZHL模式唯一命中该入口，含`mov eax,0x10D0`／alloca。它读Player健康、装备、角色及显示条件，更新heart／active等缓存与图像引用，含图像路径选择／加载、flash倒计时及HUD refresh请求；单独调用不必跑Player Update。但包含未知helper／虚调用，未证明整链纯显示 |
| PlayerHUD 的输入读取（主审补证） | raw `0x834734→0x92F610`传 action=11、关联Player的`+0x160C`和Player指针；本地ButtonAction文档11为ACTION_DROP。helper在条件下raw`0x92F639→0x84B3A0`，后者设置callback13／InputHook0并调用Lua。因此它可运行MC_INPUT_ACTION，不是只读缓存。尾部还按查询结果改`PlayerHUD+0x6BC`的显示比例，并递减flash计时。展示输入单独按控制组／本地UI归属供应，不能让远端Player的controller index读取访客物理输入、执行Drop或重跑通用Lua |
| HistoryHUD RecomputeAll `0x8304B0`（`0x4304B0`）／Recompute `0x82FF40`（`0x42FF40`） | Manager`+0x2A1C0>0`时，PostUpdate先按HUD`+0x5C8C`one-shot在raw`0x97AEDE`调All(false)，再对两槽待更新记录于`0x97AEFE`调Recompute(idx,true)。它维护Player关联、历史item列表、图标和资源引用，非只读；不能通过调用拾取／添加物品生成这些输入 |
| StatHUD维护 `0x83C860`（`0x43C860`），raw `0x97AF12` | guard为Manager`+0x2A1B8`；遍历roster，`0x83C8EE`取PlayerId、`0x83C8F6`调RecomputeStats `0x83CFD0`，随后递减两组显示计数。raw`0x83C8E5/E6`分别push roster数量word和flags`0xFFFFFEFF`；不把反编译残余参数扩成新的公开bool业务语义。它是显示缓存维护，不是最终玩法stat的生产者 |

主审补证的官方链：`ASMCallbacks.cpp:2207—2246`的ReplaceGFXPath从ItemConfigEx选择custom active图像；必要资源选择需保留，不能只保留原版硬编码图标。`ASMHud.cpp:57—85`的History图标选择读取Modeling Clay对应Player effect与Error对应current Room descriptor effect；这些必须来自目标状态或明确本地资源配置，不能重新抽道具效果。该文件`:108—129`在Recompute immediate=true且callback启用时**于super后分派1029**，因此子智能体“未见直接玩法mutation”的观察不能推出整个官方链没有Lua。History Render还分派1027／1028，和HUD总回调分别处理。

Stat默认flags`0xFFFFFEFF`包含`0xC0`但排除`0x100`：raw`0x83D11D/145/156`调用`0x7EF200/0x743800/0x7EF590`后更新概率显示平滑缓存；这些间接依赖尚未全闭合。默认PostUpdate**不执行**`0x100`的planetarium分支；官方`VanillaTweaks.cpp:60—69`另在Level／特定变化时显式刷新该项。不能笼统写“HUD每帧都算planetarium”，也不能为显示概率而允许Client本地重算服务器世界规则。目标提供概率／统计显示值后，受控写缓存和倒计时；确需复用原生重算则先解释采用flags及对应callee，不能默认整段放行。

`DESIGN`：保留PlayerHUD／History／Stat三类原生缓存与资源的职责，采用所需目标值和受控helper；HUD::Update的slide音效、PlayerHUD输入／Lua、History即时callback与Stat规则计算逐项分派。roster／资源／目标值变脏时刷新缓存，flash、平滑和消息动画由展示时间推进，一帧重复draw不重复推进。完整HUD外观继续由27承担，17代表路径所需缓存先落实；本轮没有形成“任意PlayerHUD／整条PostUpdate都可原样调用”的保证。

## 4 维护阶段与允许写入合同

下表为 `DESIGN`，是未来运行时职责，不表示新 enum、Hook 或 API 已实现。主线程、scope、实例／资源代次和 context 使用沿 ST6／ST7；纯值候选可后台准备，原生对象／Lua／renderer 不交给网络线程。

| 阶段／频率 | 输入与允许的本地写入 | 禁止消费者／错误处理 |
| --- | --- | --- |
| 目标应用：每个 A→T 批次 | 由15纯值目标、R2计划供应成员、最终属性、关系、完整姿态和变化集；按实际owner完成构建／换代／成员提交 | 不借 Update／EvaluateItems 等重算服务器结果；不以绘制维护替代逻辑完成。写入后失败记录实际前缀，停相关展示并保持隔离 |
| 移除：实际产生 pending retire 后 | R2已标记对象失效、处理借用后，在受控生命周期上下文提交适用五类EL；清引用／官方登记的连带行为一并归属 | 不每次 draw 重跑 Remove／CommitRemoved，不把 staged flag 清零当所有表或GPU已清空；更改成员时无并行／嵌套 draw |
| 排序：成员／深度／显示位置变化及必要渐进维护 | 已提交的非空 renderEL、有效 GetRenderZ 输入和足够 scratch；允许数组重排和排名 scratch 写入 | 不创建／删除实体或推进 Game frame；空表跳过。未知派生 key、容量或顺序需求未解释时停止采用该 helper，回补具体合同 |
| Camera／空间缓存：每个展示维护批次，视口变化另刷新 | 目标 Room、相机／震动／过渡描述、显示位置、视口与实际派生 draw 参数；允许 Camera 历史／Room显示offset和屏幕缓存写入 | 不保留 Manager camera tail／DoUpdate／Interpolate／碰撞。缺目标值不以本地玩法补算；换Room或Camera代次撤销旧缓存 |
| HUD 关联与值缓存：roster／身体／装备等变化时 | R1资源、R2实例／身体归属、服务器 HUD 值和本地展示席位；允许布局、heart／active／history／stat显示缓存和已去重的闪烁状态 | 不用 controller index 充网络身份，不调用本地战利品／进度／概率裁决来生成缺失 HUD 值；缓存不全不宣称 HUD ready |
| 动画／短时表现：独立展示时间变化时 | 服务器main／overlay／layer姿态及可选本地非玩法相位、已去重的音画事件；允许内嵌 Sprite 和显示倒计时维护 | 不消费本地 Shoot 等玩法事件，不让重复render／baseline重放声音。跳过时间不补跑实体tick，不回写 Game／Manager frame |
| 原生 pass／draw：每个允许的 render 窗口 | 固定同一已提交实例与资源代次，使用本轮准备的 Room／Grid／Entity／Player／HUD 数据，允许屏幕坐标、绘制暂存和图形状态写入 | 玩法Lua／生成／持久消费者不透传；display callback只有具名自有白名单。重入不重复整条helper；暂存恢复不明时停该窗口并撤销显示资格 |
| draw 结束／退休 | 停止新借用，按R4等待实际在途使用结束，再由R5处理内嵌资源和原生owner | 原生Render返回、EL摘除和CPU队列清理均不等于资源最终可释放。失败／断连不自动切Off；shutdown不能假设还有下一帧补清理 |

Host／Off 对同一常驻 Hook 保持原链透传，Client 才按用途分派。观测 wrapper 的 exactly-once 与主动分派分开：被 Client 明确跳过的原生玩法 original 调用次数为零；被保留 helper 的官方结构维护按其合同运行，不能把全部 Lua 一刀切掉或把 generic render callback 当沙箱。

显示时间与权威时间分开：`serverTick`只说明来源和排序，展示批次／render次数只是本地调度。原生 Game／Manager 帧保持其真实归属，原生shader／插值读取全局时间的点需要适配参数或隔离的显示计算。仅有主／overlay整数帧不足以表达全部分数相位，按15／18—20实际缺口补描述，不伪造既有协议已经覆盖。

一次性音画以服务器事件身份、session／world／目标代次和水位消费；baseline不重播历史事件，重复包／render不重播。HUD本地观察“值变了”不能再产生同一奖励／受伤音效；连接／设置等本地UI可有自己的唯一触发者。事件身份或所需参数缺失是表达缺口，不退回原生Gameplay消费者或全局禁音。

### 4.1 普通、odd、FSM、加载与重入的角色分派

`SOURCE/REUSED`：当前`mod/tools/gen_binding.py:534—536`在FrameScope之后、完整Manager原链之前调用`ClientManagerBoundary()`；`mod/src/game/client.cpp:136`已有独立ClientRuntime与重入失败处理。R3沿用该入口，扩展原生场景用途，不把SessionMode或网络connected一个bool当完整生命周期。

| 实际入口 | Client采用方式 | Host／Off与未解释调用 |
| --- | --- | --- |
| Manager普通／odd分流及外层 | 在完整original之前由Client主线程泵承接输入／网络／生命周期，原生Manager original不运行；因此涵盖ExecuteStartGame、位置恢复、普通Game与odd以及相机tail | Host／Off走原链；不将Manager本地loop数计为Authority tick |
| Game／Room／Entity普通模拟 | Client不进入整条Update；只从已归属的准备／应用／维护owner调用表中具体helper | 若某采用helper间接请求Update，须解释该consumer的主动跳过及caller后态；不能放行补缓存，亦不能跳过后直接认定外层成功 |
| RoomTransition FSM／特殊odd | 不运行`0x826430`的FSM模拟或特殊Player更新；复用ST3独立分支事实。服务器过渡状态供应原生绘制所需展示态，过渡draw仍按实际pass适配 | 不因普通Room没有执行就宣称这些路径也停了；未解释过渡不得在Client恢复本地FSM |
| Start／Restore／Room setup及加载旁路 | 按R1／R2的受控编排执行结构／资源工作；其嵌套更新、Lua、移除与资源请求分别归属。loading标志不能临时恢复玩法或放宽PGD隔离 | 引擎直接caller仍需相应用途检查；禁止的原调用次数为零，但每个被跳过consumer的返回合同需单独说明 |
| Game／Room／Entity／HUD render | 仅在目标与采用缓存已提交、实例／资源和context有效的展示窗口执行已解释pass／helper；准备中、部分应用或退休中不进入旧世界draw | 未经分派的GameRender／loading render caller不因函数名获得默认透传；无场景时由已有本地等待UI显示状态 |
| 嵌套Manager、维护或draw | 同一批次只有一个主线程owner；维护不嵌套修改同一EL／scratch，draw不嵌套覆盖共享插值state或暂存。已知合法资源调用可继承受限用途，不能继承“Client全部原调用可用” | 意外重入／外国线程停止相关新工作并标外层失败，不重复整个原helper；已经发生的原生前缀按R4／R5处理，不声称回滚、成功Applied或已安全Off |

这是待实现的用途分派合同，不是当前Hook清单已覆盖所有入口的声明。观测包装继续按真实原调用记录；主动跳过和调用受控helper由统一清单接线，禁止把临时卸Hook／换字节作为玩法开关。实际安装次序、更多直接caller和异常收口仍在未决表中。

## 5 对 R1／R2 与后续工作的回补

| 接收项 | 本轮具体反馈 |
| --- | --- |
| R1 Room／Camera 准备 | 有容器、config、六 Camera 缓冲和资源还不等于可绘；需要有效Room边界／视口映射、阴影／光照／FX pass准备与关联。首次Snap和后续平滑职责分开，不能用DoUpdate补初始化 |
| R1 Player／HUD | LoadGraphics、AssignPlayerHUDs、PlayerHUD／History／Stat缓存与HUD Render分开；所选普通Player也要供应实际 draw所读属性，不靠Pre／Post／Update重跑。完整初始化仍按R1未决项继续，不把本轮新增缓存名当已实现 |
| R2 create／change | 成员发布后才有排序输入；排序不补roster／引用／资源。形态／实例替换使空间、排序、HUD、动画借用和派生draw缓存按各自代次失效；网络key稳定不保留旧缓存资格 |
| R2 remove | 提交移除在下一次排序／shadow／entity draw之前；所有EL与官方副表的实际owner继续按R2。禁止旧图形窗口再进入待Free Player或已换池use；在途raw引用归R4，排序不承担析构 |
| R4／R5 | 共享 AnimationFrame、Shadow image batch、Room光照集合、HUD内嵌资源及Camera历史都进入实际使用链。异常／重入恢复、借用、最后资源释放与部分失败退休仍需逐owner收束 |
| R6／15／18—20／27 | 将维护输入、变脏原因、正常返回后的状态、显示资格／错误归并到公共应用结果；具体Room／Grid、各实体、Player身体和HUD实现仍按原任务承担。Applied ACK依21／22的逻辑提交合同，不由render次数／一次sort返回代发，也不宣称像素验收 |

## 6 未决问题与本轮边界

| 未决项 | 已缩小的范围与继续方向 |
| --- | --- |
| 原生排序容量与完整次序 | 非空限制、共享scratch、GetRenderZ入口与渐进排序已核定；仍需落实所选实际EL的容量寿命、派生key与大批次立即完整次序策略。不能默认为全类可调用／一次全排 |
| Camera 完整模式 | 窄setter与混合Update已分开；Active Camera平滑、特殊Room clamp、多人焦点、震动／过渡和视口变化的全部目标字段与缓存仍需18的适配，17代表场景所用部分不能全部推迟 |
| Room pass与派生绘制闭包 | shadow链和官方光照Lua已落实；Room／特殊过渡／FX、复杂Player及各派生Render仍有未审间接节点。按实际采用路径继续补，不声称整个GameRender只读，不重开全引擎类枚举 |
| HUD 所选helper闭包 | PlayerHUD Update的缓存／资源／输入Lua，History目标效果／即时1029，Stat默认flags和C0概率依赖已定位；剩余为实际角色／装备所需间接helper、原生显示缓存可写合同及目标缺值。不能只禁HUD总回调或改controller index宣称全部解决 |
| 内嵌资源／异常／重入 | 已知普通返回的暂存恢复不覆盖异常；共享插值state／帧、Lua、batch、队列raw引用和GPU使用由R4／R5接续。无法恢复时停显并保持Client隔离，不能恢复旧A的假象 |
| 表达与事件 | 需逐实际draw核对缺失HUD值、层／相位、特殊几何、相机和音画事件；现有观测字段不自动成为完整复制schema。缺数据不本地模拟补出 |
| 实际运行 | Hook次序／安装、真实线程与context、视觉完整性、音画去重、整个过程无访客持久副作用仍未运行观察；留全目标实现后的同tx统一E2E，本轮不执行分项试验 |

## 7 主审与文档收束

必要逆向按用户要求由一个 GPT-6 Luna／max 子智能体执行。主智能体独立回读官方源码、sort／GetRenderZ、Entity Interpolate／odd调用、Camera三入口与原始写入、Room阴影链及HUD相关callsite后决定采用范围；没有把子智能体结论直接升级为运行保证。

主审特别区分了原生位置插值与官方AnimationFrame插值、shadow slot`+0x18`与Interpolate slot`+0x10`、sort key getter与成员发布；纠正HUD PostUpdate尾部call为`0x97AF12`（`0x97AF17`是pop），并补明空renderEL和预算排序限制。子智能体误把相似签名的`0x834930`归为PlayerHUD Update，主审用完整ZHL／生成源码模式与raw指令核定实际为`0x832C10`，子智能体复核撤回误报；Stat guard也按raw改为Manager`+0x2A1B8`。主审另补ACTION_DROP→MC_INPUT_ACTION、History即时1029与目标效果来源；未采纳“默认Stat分支必须原样运行”的推导，保留原生外观责任而将未解释规则链列作目标值／受控adapter缺口。已记录的SOURCE／静态事实、设计与UNKNOWN分别保留。

本轮以统一人工文本回读收束；没有运行格式／链接校验器、程序、构建、测试或游戏，不另留artifact。R3静态合同不表示R1／R2完整可调用初始化／退休已闭合，也不表示tx-04-17已实现或验收。
