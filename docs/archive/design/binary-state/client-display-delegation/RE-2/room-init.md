# RE-2：Room::Init 的显示拆分与冷构建

**2026-11-14 实施补充**：[第13步冷构建设计](../../13-client-cold-build.md)第11、12节已记录布局78的受控冷构建、首次捕获及审阅修复；无关目录追加不重建房间，原生 CPU 分配在同一 Peer 账户中保留至真实释放。下文保留2026-11-12静态事实和当时接线记录；旧手工世界文件与 replay 调用不代表当前运行结论。完整 Room::Init 继续不放行，本轮未构建、未测试。

返回：[成果总览](README.md)。根 VA `0x7E8AD0`，RVA `0x3E8AD0`。本页的地址／调用／字段为 `J273_STATIC`；重组顺序为 `DESIGN`，现有工程接线为 `REUSED`。未执行原生 Init 或其子函数。

## 1. 原生阶段与处置

SDK 的两个参数是 `int param_1, RoomDescriptor* descriptor`，但第一个 word 实际指向 `RoomConfig_Room`，详见[入口证据](entrypoints-and-sources.md#1-根入口与参数纠正)。以下顺序说明为什么不能仅屏蔽生成后调用完整 Init。

| 原生阶段／调用点 | 已核职责 | Client 处置 |
| --- | --- | --- |
| `0x7E8B2A→0x90A560` 起 | 清 Room TemporaryEffects；清理死亡名单及多组容器，重置关联状态 | 不用于显示初始化；旧对象／规则状态归既有 owner 退休或安装 |
| `0x7E8D90→0x731740`、`0x7E8D97→0x7E1550` | ItemPool 关联处理，以及实际 Room 清理／重置 | 拦截；后者包含 EntityList 清理、Grid 析构及全局／Room 重置，见下文 |
| `0x7E8D9C—0x7E8DB4` | 从 config 的 Width／Height 两个 byte 加 2 得到 Room 网格宽高 | 安装经 Host 描述确定的几何；这是 Camera／背景资源的前置 |
| `0x7E8DB7—0x7E911A` | LightGradient／SpotLight ANM2 资源、过滤、默认动画／混合设置 | 在已构造的本地 Sprite 上按职责复用，不跳进 Init 中段 |
| `0x7E9147/0x7E9156`、`0x7E916D→0x9B7570`、`0x7E9178→0x82E960` | 安装 descriptor／房型、rail manager 关联初始化、特殊地狱背景资源初始化 | descriptor 先接本地对象；rail 不借此生成 minecart；地狱背景可单独冷初始化 |
| `0x7E92F2→0x7E86C0(true)` 及配置扫描 | bounds、房间 spawn 配置／随机 Grid 条目、描述标志和水流等初值 | Host 的已选结果／当前值直接安装；不能重新解释房间配置产生一份世界 |
| `0x7E9602→0x7E7670` | 选背景、设置 Backdrop seed／独立 RNG、FXLayers 加载、shading 图形资源 | 混合选择与资源准备；拆成输入结果和本地资源职责，见第 3 节 |
| `0x7E97B8` 及后续生成段 | 初始化 waterAmount；生成／恢复 Grid、Door、Entity，执行其 Init／Update，修改规则与进度 | 权威 waterAmount 直接安装；其余由 world owner 工厂与状态应用处理 |
| `0x7EA0FF→0x817F30`、`0x7EABDF→0x819130` 等 | 房间输出组映射、轨道／矿车等房内对象准备 | 不为背景显示运行；需要的实体／Grid／轨道显示按实际输入安装 |
| `0x7EAC89→0x7E6250` 等恢复段 | Door／既存房间状态恢复和关联动作 | 安装 Host 当前门状态；不触发一次原版房间恢复流程 |
| Boss／特殊房、文本／音乐段 | 房型与难度／进度判断、实体生产、Boss 演出文字／字体资源等，夹带规则与音频请求 | 非纯显示根；按状态／事件／资源需求承接，转场与演出分别归 RE-8／RE-9 |
| `0x7ED3A4→0x819530` | 计算雨生成区域；此 helper 本身不直接 Spawn，后续 Room 更新会据此生成雨实体 | 不是恢复雨纹理的必要更新入口；不启动原生雨实体生产 |
| `0x7ED3A4` 之后至 `0x7ED7B3` 之前 | floor／wall ColorMod、特定 ray image；`0x7ED6F9→0x818740` 提交房间颜色 | 采用已选资源和必要颜色输入；颜色 API 见 Update 报告，不能重跑规则选择 |
| `0x7ED7B3→0x7E86C0(false)` | 最终 bounds 处理 | 完成 Camera 所需几何；优先安装 Host 已给 bounds，避免覆盖后再读取 |
| `0x7ED7C3→0x7E03F0`，`0x7ED7CD` 分配，`0x7ED7E5→0x91C730` | 旧 Camera 清理、新 Camera 构造，写 Room `+0x11F8` | 由既有 owner 管理本地 Camera；冷构建一次，热更新不重建 |
| `0x7ED7FD—0x7ED80F` | 将两份 RoomShockwave 的 max 复制到 count，使旧波纹停止 | 房间生命周期重置值；不与每帧 radius 推进混为一项 |
| `0x7ED815→0x917910` | Backdrop surface／资源构建，包含装饰实体生成路径 | 拆分构图与生成，见第 2 节 |
| `0x7ED81A→0x40A140`，随后 `lua_gc`，特殊条件 `0x7ED888→0x815D40` | 未命名收尾、Lua GC／用量日志、特定 Boss 房 helper | 不按“收尾”推定纯显示；未分类者不进入 Client 白名单 |

这里只记录会影响拆分的原生阶段，不把所有配置分支都重命名。原生还在前段写 `Room+0x11F0` 帧锚点／初始化标志；其帧语义交 RE-3，不可把重新 Init 当作帧号对齐手段。

### 不能复用完整清理根

`0x7E1550` 调 `EntityList` 清理 `0x413A30`，重置／重排列表，遍历 **448 个 Grid 槽**并调用虚析构（例如 `0x7E15EA`），随后清空 Grid 指针表和 Door 状态，还改 Game 标志／计数、Room 类型／滚动／颜色等。它既不是纯值 reset，也不是 ClientNativeWorldOwner 的替代退休入口。进入它后再屏蔽 Spawn，已经无法避免旧世界被清掉。

中段存在实际 `0x424340` Entity 工厂及其立即 Update，Grid 创建／虚 Init（vtable `+0x28`），普通门创建 `0x7E4DE0`、特殊门 `0x81C970`；并可进入 PGD helper（例如 `0x7E99EC→0x903930`、`0x7EBBB7→0x903A50`）。这不仅是“生成数量重复”的问题。既有保存／PGD／成就防护继续常驻，不能因显示委托退役。

官方 REPENTOGON 也在 Init 外围执行 PRE_NEW_ROOM（回调 1200）和存档实体稳定性维护；这些 wrapper 没有把原函数改成纯显示。Q6 允许的回调在实际放行路径正常分派，不恢复旧的全面禁 Lua 规则；也不为触发回调而调用整条 Init。

## 2. Backdrop 的窄初始化与混合构图

### 2.1 `Backdrop::Init(false)` 的实际写集

`0x917870` 接收 backdrop id／loadGraphics；id 超过 `0x3E` 时回退为 1。ECX 为 `Room+0x1BC8`，写入：

| 相对 Backdrop | 内容 |
| --- | --- |
| `+0x150` | 当前背景 id |
| `+0x4F18/+0x4F1C` | surface 最小范围，均为 52 |
| `+0x4F20` | `(Room.gridWidth - 2) * 26 + 52` |
| `+0x4F24` | `(Room.gridHeight - 2) * 26 + 52` |

false 到此返回；true 在 `0x917900` 调图形构建 `0x917910`。因此 false 可表达背景 ID／范围初始化，却不会加载完整图像、创建 surface，也**不会重设 seed／RNG**。范围读取全局当前 Room 的几何，必须先接好 currentRoom，不能只传一个孤立 Backdrop。

原生 `0x7E7ADF—0x7E7B0F` 在 Init 之外另写 Room `+0x6B00` 的 Backdrop seed，以及 `+0x6AF0` 的独立 RNG／shift，来源为 descriptor DecorSeed（`+0x58`）；之后 `0x7E7B15` 才调用 Init(false)。不应从 SDK 的 Init 名称推定它会负责完整播种。

### 2.2 图形构建中的三类生成点

静态链：`0x917910` → floor 构图 `0x90EF10`／walls 构图 `0x911310`。资源／surface 操作会调用图形状态保存 `0x67EDD0`、恢复 `0x67EEB0` 等；教程路径还读取真实 Player／控制信息并加载 `gfx/backdrop/controls.anm2`、`controls_buttons.anm2`。

| 位置 | 副作用 | 隔离要求 |
| --- | --- | --- |
| walls → `0x915890`，例如 call 点 `0x912A85/0x912DDE/0x912EF1` | 多处 Spawn 后调用实体方法；`0x915D39` Spawn 后 `0x915D70` 实体 Update | 保留 `client_backdrop_effects` 具名拦截；不能把它误作 FX 动画更新 |
| walls `0x91168F` 全局 Random，`0x9116C0→0x424340` | 不经过 `0x915890` 的直接生成／Update 分支 | 保留直接分支隔离；仅拦前一 helper 不够 |
| walls `0x9119E9` 全局 Random，`0x911A0C→0x424340` | 第二处直接生成／Update 分支 | 同上 |

当前 `hooks.json` 两个 byte patch 正对应后两处：RVA `0x51168F`，正常续点 `0x511694`／跳过落点 `0x511778`；RVA `0x5119E9`，正常续点 `0x5119EE`／跳过落点 `0x511AEE`。现有实现还推进各自一次 Backdrop 独立外观 RNG，然后跳过世界生成。它们与旧手工 Render 不是同一职责，不能随手工绘制路径一起删除。

**设计**：图形资源与 surface 归本地，Host 实际生成的装饰 Entity 由 world 安装；Q4 允许纯外观构图随机不同。今后可通过已有具名 scope／常驻防护复用构图，或把资源职责进一步抽出，但必须覆盖这三类生成点。不要伪造 Player／空 roster 来满足教程路径；真实 roster、RoomConfig、图形 context 及构图所读输入应先就绪。

## 3. 背景／FX／光照资源的最小职责

`0x7E7670` 不是单纯的 LoadGraphics。它先结合 stage／stageType、房型、描述与挑战等选背景，处理 Backdrop seed，再加载 FX／shading；不能让 Client 通过它重新选择 Host 已裁决的主题或规则结果。

| 资源职责 | 原生依据 | Client 需要的输入／处置 |
| --- | --- | --- |
| LightGradient | Room `+0x6E40`，`gfx/LightGradient.anm2`；`0x7E8E3D→ANM2 Load 0x407AF0`，layer0 min／mag filter 设置在 `0x7E8EA4/0x7E8EB4` | 构造好的 ANM2、有效图像资源；按原设置加载／配置，用 SDK API，不复制 Host 图像指针 |
| SpotLight | Room `+0x6F54`，`gfx/SpotLight.anm2`；Load `0x7E9012`，过滤 `0x7E9079/0x7E9089`，后续默认动画／混合设置 | 同上；不需要为初始化灯光运行 Entity／Grid Update |
| FXLayers | Room `+0x1618`；`0x7E7CDC→0x968E10`，读取 FX XML、stage 输入并选择层／ray／候选；其内 `0x968E91` 有全局 Random | 同步已选资源／影响显示的输入，加载图像／本地容器；Q4 的纯外观选择可归本地，不能把整块 FX 内存或候选重新裁决当载荷 |
| 不使用 FX 的清理分支 | `0x7E7D6C→0x96BD30` | 本地 FX 图／资源的生命周期操作，由 owner 承担；不伪造 `_loaded` |
| Satan radial／shading | `gfx/Effect_SatanRadial.anm2`、特定 `gfx/satan_radial_ultragreed.png`；按形状选择 shading image，`0x7E814F` 加载，`0x7E81F5—0x7E82CD` 准备 shading target surface | 需要房间形状／主题、特殊路径和有效本地图形后端；按选中的资源构建，不复制 surface／设备句柄 |
| 特定 ray image | Init 尾的 `gfx/backdrop/fx_ray_3.png`／`fx_ray_4.png`，全局图像槽 `0xBE7358` | 已选资源引用与当前房间适用条件；不能把 Host 图像地址传到 Client |
| 地狱横移背景 | `0x82E960`，Room `+0x776C` 视图；加载 `gfx/backdrop/hell/background.anm2` 并设层／混合、活动与位移初值 | 视图 Room 回指、descriptor／stage／房型有效时可单独初始化；逐帧混合更新 `0x82EE90` 仍不能整条调用 |
| Camera | `0x91C730` 原生构造，Room `+0x11F8` 持有 `0x94` 对象 | 必须先有最终 bounds／door positions；本地原生分配器与 owner 管理 ring 等内部缓冲，不传 Host 指针 |
| 水面与颜色 | waterAmount `+0x7240`，四个 KColor 端点 `+0x7244/+0x7258/+0x726C/+0x7280`，duration／countdown `+0x7294/+0x7298`；floor／wall ColorMod `+0x72A4/+0x72D0` | 安装原生消费者需要的值，KColor 为 `0x14` 而非只四个 float；最终 water shader packet 由原生 Render 计算 |

原生 `0x82E960` 只是特殊背景资源准备，本次未发现其中 Entity 工厂；其启用判断含 descriptor 配置房型 16 与 stage 35 等条件。需要初始化时调用这个完整小入口，不能从 `0x82EE90` 中间抽一个地址当 Init。

FX 的有效 pose、vein、图像／ANM2 资源和平均 Player 位置会被 `0x96BF40` 消费，见[更新报告](room-update.md#2-fxlayers-的显示更新)。FX resource 选择与本地外观状态的最终字段裁剪仍要落到第 7／8 步需求清单；这里不据一次静态阅读宣称所有候选选择都能本地替代。

## 4. 建议的 Client 冷构建顺序

以下是显示委托下的职责顺序，不是已实现的新函数：

1. **结构准备**：在既有主线程 NativeBuild／NativeApply 事务内建立本地 Game／Room、descriptor→RoomConfig、几何、bounds／8 个 door positions、stage／stageType／难度等查询输入。水量、已选主题、必要 flags／颜色也安装当前值；不运行 Room::Init 重置旧世界。
2. **本地资源**：在有效图形 context 中准备 LightGradient／SpotLight、FX／shading、特殊背景等；对象、容器、资源引用必须真实有效。为 Camera 提供最终几何，冷构造或按已有生命周期复用，热应用只写值。
3. **实际世界**：按已有原生工厂 owner 建立实际 Player／Entity／Grid 与关系，提交实际 roster 及显示所需 NPC／门／水流等当前值；不用玩法 Update 把对象“热起来”。
4. **依赖 Player 的构图收尾**：在实际 roster、资源、selected BackdropConfig 就绪后，以具名构图作用域运行背景资源职责，并隔离上述实体生成路径。图形资源／surface／显示缓存归本地，Host 选中结果仍来自 Host。
5. **显示就绪与提交**：应用必要 Sprite 姿态、颜色目标和初始化锚点，完成原子 world 提交后才允许原生 Game::Render；随后按 RE-1／本任务的独立白名单推进。资源有缺失或生命周期不确定时，停止相关构建／显示操作，不进入完整 Init 补救。

这里不规定每次网络到包都做一次资源重建。稳定结构复用现有 owner；房间／主题／资源变化按其生命周期更新。实时转场中的目标替换点和等待方式仍由 RE-9 决定，本任务不提前把完整 Level::ChangeRoom 放行。

## 5. 现有实现交接

- [client_native_world.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/scene/client_native_world.cpp:250) 的 PrepareImpl 已按阶段建立 descriptor／config、本地 Room 几何并在 Camera 构造前覆盖真实 bounds。第 3 阶段以游戏分配器构造 Camera，第 4 阶段加载 HUD 资源，第 5 阶段把根交给实际 Player 安装。这些生命周期安排可以继续复用。
- 同文件 [FinalizeImpl](F:/projects_isaac/issac_multiplay/mod/src/game/client/scene/client_native_world.cpp:373) 明确等待实际 Player roster、有效 context 和 CPU 队列，再配置 Backdrop 资源。它目前仍使用旧场景描述／seed／wall trace／ClientBackdropReplay／Init(true)，不是本次新需求裁剪已经完成的证明。后续退役旧构图 trace／旧结果字段时，应分别处理本地构图选择和防止重复生成的职责。
- [client_backdrop_native.cpp](F:/projects_isaac/issac_multiplay/mod/src/game/client/room/client_backdrop_native.cpp) 与 [hooks.json](F:/projects_isaac/issac_multiplay/mod/binding/hooks.json:132) 已承接 Backdrop helper／两处直接生成分支隔离；Hook 仍启动安装并常驻，只在 Client 逻辑作用域启用，不临时卸载或恢复字节。
- 显示推进新增 FXLayers、背景动画、波纹／水色等时，继续使用统一 `ClientEntryDecision`／CountDispatch／预期嵌套登记。未分类入口默认拒绝，不能扩大一个 NativeApply scope 到容许任意 Room／Entity 更新。
- Render 会自行构建阴影／光照／坑纹理／水面所需缓存；原生树及漂移处理交 RE-5。旧手工 Room 绘制、最终 shader packet、相机结果同步按新方案退役，不作为调用完整 Init 的理由。

本次没有修改这些产品文件。Init／构图的异常、资源分配失败与多次换房生命周期仍需在实现时沿用真实 owner 的处理，不能从本次静态分类外推完整回滚安全性；实际覆盖在第 15 步统一验收。
