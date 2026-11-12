# 14：退役旧路径与产品边界

**2026-11-12 显示委托修订**：
退役清单补入[修改方案](client-display-delegation.md)第 9 节的内容：

- Client 自协调绘制：`DrawImpl`、`FrameBorrow`、`RenderPublished`、`ClientMenuRenderBoundary` 的场景绘制和 `client_resource_shaders_native.cpp`；
- Host 显示生产；
- 显示包络；
- 绘制随机观测；
- 默认删除的绘制 hook。

删除前逐项确认没有其他调用方。

本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：新布局已退出重复HUD缓存；旧可运行legacy采集／安装／漂移比较继续保留，等正常来源与原生显示实际接管后退役，不能仅因纯值模块编译成功删除唯一运行来源。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化退役交接，仅文档**：按[现行合同](hud-local-presentation.md)在新schema、真实来源及Client纯显示owner接管后，退役low_health_flash／holy_mantle_visible、普通图标／中心、逐帧Drop渐变和Host布局缓存的重复采集／发送／安装／原值比较。保留实际驱动、完整心格／三色及充能闪烁、特殊资源、有序列表、Stat三值／概率与正常本地资源生命周期；不恢复Player六项玩法属性或因本地化开启整段HUD Update。当前未删除代码、未改生成表／版本，无构建或测试；旧可运行路径仍按真正接管时序退出。

**2026-11-11 第9步收尾交接，版本65**：线格式、正文／显示／资源／完整消息、纯值接受恢复及[预算合同](9-budget-capacity.md)均完成，旧Range和JSON握手仍不能作为新状态的兼容分支。本步在10—13步完整接线后退役旧入口与重复预算常量，保留输入、隔离、Hook常驻和必要生命周期owner；第15步统一验收。

**2026-11-11 当前退役目标调整**：[本地生成合同](local-media-presentation.md)落实后，退役正式同步中的文字／切房／hallucination像素必发和音频文件传送假设；通用媒体blob当前不启用。保留本地资源加载、文字纯显示生成、hallucination本地缓存和必要音频执行／失败保护。旧schema／代码仅在替代消费者实际接通后迁移，不提前删唯一可用路径；历史来源、失败和UNKNOWN保持。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。正文编码器已编入产品状态库，但整消息新Kind／握手／资源／接受入口仍未启用。不能因此提前删除唯一运行中的旧Range／Lua／JSON路径；按完整10—13接线完成后退役，旧失败与UNKNOWN照实保留。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。以版本59公共合同为后续替换目标；新固定头和Kind数值登记不等于新编解码已可用。待完整payload、接受规则和10—13步运行接线完成，再退役旧Range／树形解析／相关旧路径，不能仅因本项定稿便先删除唯一运行实现。

**第8步版本58交接**：[正式源与后续边界](implementation-8-presentation.md)已落实，旧运行链本轮尚未删除。仅在新采集／外层封装／资源／解码／原生安装接线完成后，退出旧application_draw随机四字组、通用getter／JSON、Host地图输入镜像、单根HUD绑定及以原生两槽充当系统容量的路径；历史失败与UNKNOWN照留。

**2026-11-10 显示批次及HUD旧路径退役交接**：[方案B](presentation-result-batches.md)要求新来源／固定结果／各端主体选择落实后，退役通用HUD固定roster第0身体绑定及仅采Host两槽作为全部Peer显示的旧假设；各端History／Stat默认跟随自身控制组，公共多人HUD保持完整。原始History／getter／JSON不恢复为正式来源，不先删除唯一结果留下空HUD；旧多身体／动态HUD事实及失败记录保留。当前仅文档决定，未实施退役，版本57不变，无构建或测试。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为14.5“各域退役与保留交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

## 目标

从正式同步路径中移除 Lua getter 采集、JSON 树、逐成员目录和字符串分派。划清产品构建与 E2E 构建的边界，使产品 DLL 中不再包含旧路径的代码。

## 输入事实

### 构建边界

`mod/CMakeLists.txt:94` 的 `ISAACCS_BUILD_E2E_TOOLS` 选项区分产品构建和 E2E 构建。E2E 构建的工具、夹具和故障注入只进入 E2E DLL（`CMakeLists.txt:151-157`）。

### 旧路径主要文件

| 位置 | 内容 |
| --- | --- |
| `mod/src/game/world_sources/lua_entity.cpp`（708 行） | Lua 正式采集与 `CopyLuaSource` |
| `mod/src/game/world_sources/observation.cpp`（797 行） | 十一个域的 Lua 正式采集 |
| `mod/src/replica/stream_capture.cpp`（417 行） | 后台树构建与身份投影 |
| `stream_partition.cpp`、`stream_source_value.*`、`projection_graph.*`、`stream_materialize.*` | 分层、纯值块、Lua 图和还原 |
| `entity_plan.*`、`room_grid_plan.*`、`player_plan.*`，以及对应的 `*_target.*` | JSON 目标与 A→T 计划 |
| `stream_binary.cpp` 的字段表部分、`EncodePayload`／`DecodePayload` | 树形正文编码 |
| `mod/tools/gen_stream_catalog.py`（231 行）及其产物 `stream_policy.lua`、`field_ids` | 逐成员目录生成 |
| `client_entity_fields.inc` 的字符串分派 | Client 按名写入 |

### Lua 观测 Mod

`mod/observation/isaac-cs-observation`，包含 11 个域。

## 设计

### 14.1 移除顺序

1. **新路径完整编译**：第 10—13 步的新路径完成后，先确认它们可以独立编译，不依赖旧路径。
2. **逐个移除**：实施时对上表每项先用静态检索辨认实际调用方／职责，再以相应纯产品构建核对依赖；本次文档整理不执行删除、构建或测试。
   - 删除前要看清目标文件的实际内容，确认它的职责已被新路径完全接管。
   - 部分接管的文件，只删除已接管的部分。例如 `stream_binary.cpp` 中仍被控制消息使用的编码保留。
3. **同步构建与生成**：同步修改 `mod/CMakeLists.txt` 的源文件列表和生成命令，移除 `gen_stream_catalog.py` 的生成步骤。

### 14.2 Lua 观测 Mod 的新角色

| 部分 | 产品构建 | E2E 构建 |
| --- | --- | --- |
| `domains/mod`（具名只读 adapter） | 当前原版产品不加载、不接入正式采集；设计与既有来源保存在[Mod 后续方案](../mod-adaptation.md)及原路径 | 当前第 15 步不加载第三方 adapter，不运行其适配用例；历史观测事实保留 |
| 其余十个域 | 不加载 | 只作为 E2E 对照（第 15 步）加载，不进入同步路径 |

第三方 Mod 支持代码与资源不因迁出方案而在本次文档修订中删除。第 14 步实现时划清当前产品加载／构建依赖，使原版正式同步不依赖 `domains/mod` 或 StageAPI；本项目自带界面和测试夹具仍按原职责管理。

### 14.3 保留不动的部分

- **Hook**：统一清单保持常驻、会话逻辑启停，常规采集复用现有窗口；具体显示结果生产点和已明确的Lock第七覆盖点按各自owner合同纳入统一清单，不能把“退役旧采集”解释成卸Hook。`world_delirium_owner`保留Init后资格职责，真实公共shape决定结构换代，Boss AI选择不触发无条件升shape。
- **身份**：`IdentityTable`、`Registry`。
- **传输**：通道、可靠发送、分片重组、合包、zstd、内存池。
- **Client 原生写入层**：原生写入函数、结构事务和退休流程。

第4步的关系职责按版本29保留，参数与容器以版本37为准，不能用旧应用目录要求恢复已排除同步：Tear保留三份辅助Sprite及StickTarget，Pickup保留两份Sprite／公开Ghost和MegaChest三关系，未来循环数组自版本35起留Host，Laser保留BounceLaser／原始有符号采样模式／optimized与unoptimized两组最终点列及实际展示标量；Knife四槽、Tear六槽、Pickup私有一槽、Laser十二槽的旧Source／安装／原值比较在接线时退役。Familiar +DAC仅variant240布局130保留展示引用，其他变体不发。被排除的目标仍是独立成员；撤边和本地空槽／正常析构保护不可随旧表删除。几何不调用CalculateSamples，FlipSaveState不复活。TearPlus／PickupPlus／LaserPlus的派生声音、重抽和重采样请求仍属Host。见[关系职责复核](internal-entity-relations-review.md)，早期实现记录保留原时点。

### 14.4 文档同步

- **索引**：更新 `docs/README.md` 和 `docs/hooks/README.md` 中受影响的入口。
- **旧方案与历史资料**：旧方案文档已在第 0 步加了顶部说明；历史记录正文不改。
- **Repo中的`stream-catalog.json`**：原目录与历史签名保留，不再由旧生成器产生运行中的逐成员热路径。版本57的唯一布局源仍引用它作为Room／Level／RoomSave／Exploration共597行回映射输入，并由普通构建追踪资料依赖；这项同源文档／目录用途不能与旧线上树编码一起误删。后续实际改变资料依赖时再同步生成器和文档，不宣称当前已无构建引用。

### 14.5 各域退役与保留交接清单

以[第9步9.10](9-wire-format.md)、[第10步采集](10-host-capture.md)、[第11步发布](11-host-publish.md)、[第12步解码](12-client-decode.md)、[第13步应用](13-client-apply.md)为替代合同。版本57的布局／生成元数据不代表实际调用链已经接管。本表说明未来切换时退出的正式职责、必须保留的行为及尚需接通的替代，不在本次删除任何代码、来源或历史材料。

| 域与来源 | 退役与保留的特殊约束 | 身份／版本依赖 | 待落实项 |
|---|---|---|---|
| 公共热路径／构建：[本页旧文件表](14-retire-old-path.md)、[第0—2步](implementation-0-2.md) | 退出正式Lua getter／JSON树／字段名字分派、旧Range正文与分区／materialize；控制消息仍需的codec、原生事务／安全退休、身份／传输／池保留。部分接管文件只删除已替代职责 | 新source→发布→typed解码→实际writer形成同一布局／目标链，旧格式不兼容猜读；不是仅删一个入口 | 静态调用图与逐文件职责迁移、CMake源／生成命令和产品/E2E边界；实际删除／纯编译在实施阶段完成 |
| 目录／生成器：[第1步](1-layout-source.md)、[版本57](implementation-7-room-level.md) | 退出gen_stream_catalog的旧运行字段／policy产物；保留唯一state源、同源数字表、回映射与原目录历史。stream-catalog目前仍是597行构建资料输入，不能一概删引用 | 版本57／正式生成器的catalog_complete_scopes与原一基行号／签名关联，重复ID不去重 | 区分旧热路径产物与当前资料依赖，追踪实际CMake／生成器消费者，不恢复第二份运行schema |
| Entity公共／动画／Plus：[公共职责](entity-common-synchronization-review.md)、[生命周期](entity-lifecycle-synchronization-review.md)、[资源](animation-resource-synchronization-review.md) | 退出公共valid／工厂字节、已裁计时／物理历史、ColorParams和事件／loaded原值比较；保留exists/dead、最终颜色／selector／playing/speed、实际Image／路径、nullable和本地资格／退休 | Player独立激活由P7接管后再迁旧结构来源；非Null资源失败不ACK，空资源仍正常退休 | 正式投影／资源owner／读回与旧Source／writer／validator一起迁移，不删必要生命周期保护 |
| Familiar：[结果／绳索](familiar-synchronization-review.md)、[装备／Plus](player-equipment-plus-synchronization-review.md) | 退出轨道／方向等11范围与玩法Weapon补图；80保留最终amount／原动画尾部，240保留DAC／typed绳索／节点与430E40 owner，蝗虫／IsaacsBody显示结果不删 | 共享payload、资源和主体/use同目标，Familiar保留展示计时／D76关系 | 具体来源／缓存／adapter及正常候选退休接好后退出旧路径，不能用Null或空绳索遮缺口 |
| Knife／Bomb／Tear：[三类复核](knife-bomb-tear-synchronization-review.md) | 退出42个Host范围、旧攻击关系／锚点及原值相等；保留拖影／飞行、Bomb五costume与pending清除读回、Tear三Sprite／效果和真实引用 | 当前生成块与同world资源／最终姿态对应，被排除目标仍是独立正常成员 | Source／应用／漂移与E2E口径一起迁移；不以LoadCostumes／ResetSpriteScale／碰撞补旧生产行为 |
| Pickup／Slot：[职责复核](pickup-slot-synchronization-review.md) | 退出选择／循环／Flip保存体／Timeout/Wait和机器开奖字段；保留Price／DropDelay、价格／替代底座／奖品Sprite、playing与公开三引用，本地未来列表安全空owner | 计时与同world时钟／资源关联，独立奖品Sprite不由Slot State派生 | 保留输入／资源／计时新源及writer接好，旧定价／奖励／旧查询相等退役；不删除独立HUD进度来源 |
| Laser／Projectile：[Laser](laser-homing-synchronization-review.md)、[PR2补证](projectile-effect-residual-evidence.md) | 退出采样输入／indices／Bezier／Laser计时与Projectile八项运动／回调；保留两最终点列、Light unoptimized、Height及PR2开关／冻结Vector、精确偏移恢复 | 同成员use／目标／资源，独立点列正常owner与空集合清理；版本45覆盖旧仅Height设计 | 旧Source／字符串writer／原值回验迁为真实生成字段与几何owner，不让旧目录遗漏PR2成为删字段理由 |
| NPC／Delirium／Text／Dummy：[NPC纠错](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md)、[Text](text-dummy-evidence.md) | 退出ControllerId／模式1／原版Online筛选、Boss选择无条件shape、旧Text duration／KColor尾字raw比较；保留0/2、三deque／双Sprite／独立companion及Text真实writer、Dummy池／字体／历史cache缺口 | 稳定RTTI／公共shape、当前use与Text独立scope；不能混同type964或拼普通快照 | 具名UI隔离／数字writer／资源及旧绑定清理；不卸唯一常驻Text Hook、不凭缺描述删除历史显示依赖 |
| Effect：[五项](effect-next-five-evidence.md)、[版本45](projectile-effect-residual-evidence.md) | 退出旧8/21B常数、113/167旧臂、Trail16384上限、136保存描述、请求／函数地址及旧固定Ghost四槽原值；保留16/13B、127节点、有效Ghost前缀及三个typed owner | Parent、存在性／条件／资源与目标一致；Timeout负值／非线性不受旧静止不重发假定约束 | 真正当前全部臂／source／writer／读回切换；合法空payload与未知错型拒绝、正常本地构造／退休保留 |
| Player P0—P2／拓扑：[正式Player](implementation-5-player.md)、[前三块](5-player-p0-p2.md) | 退出旧final-state全量／Captured字典、Player六项模拟属性／cacheFlags与Eternal/Bone原值要求；保留五Render充能、健康／资源输入、主Sprite／P0和HUD最终缓存 | P0/P6 topology、P7真实身体关系和资源同事务；独立激活不随公共valid裁剪而删除 | body／ordinary、真实HUD源及数字应用／读回整体接管，不能只删旧getter后留下空HUD |
| Player P3—P8／组：[正式Player](implementation-5-player.md)、[中三块](5-player-p3-p5.md)、[后三块](5-player-p6-p8.md) | 退出完整innate group／timer／旧统一Weapon四值、costume生产map、sub/replaced/703四边、非704 cooldown／原History／回放全集；保留当前有效聚合、真实槽／类型／Whip、P6目录／P7激活与C/S组、残影和当前记录 | 四参与者／多身体、participant/group与当前use、统一拓扑／配置身份及资源相关联；Plus模型不作裸写依据 | 新官方容器／结构owner、动态装备窄Free与HUD／History结果就绪后迁消费，原输入发现拓扑和本地保护不删 |
| Grid公共／17派生：[第6步](6-grid.md)、[旁表](grid-plus-water-source-evidence.md)、[描述接口](grid-description-implementation.md) | 退出Legacy字符串Grid字段、collision原值／water≤255／optional镜像、默认名／待加载路径、Fire旧24B速度／4096预算、Door误用+154。保留完整u32／glitch、nested_layout、Fire≤5×16B结果、Door主色入口／extra及Rock碎块 | 正式描述→执行器／门绑定与Room上下文同world，实例use／资源一致；Lock第七点和原链once必须接通 | 对接真实writer／纹理adapter／七水面消费后退出legacy，不能将已有描述生成成功说成运行替代完成 |
| Room／背景／FX：[版本57](implementation-7-room-level.md)、[显示资源](room-display-resource-design.md) | 退出完整配置／Spawn/Merged JSON、背景RNG轨迹／Init重演、原水色时间线、相机ring／FX推进参数与函数／生成队列；保留当前几何／八门／真实纹理／water/light/wave/focus／rails／FX与环境声 | 内容、world／显示／pass与资源修订关联，旧room/use退休后不续借；flow不重复 | 完整结果来源／资源／adapter接好再迁；不靠Camera／地狱背景Update、雨／装饰生成或旧Lua补缺 |
| Level／保存探索／地图：[两域](room-save-exploration-synchronization-review.md)、[版本57](implementation-7-room-level.md) | 退出全flags／全seed／完整保存体、四揭图bool／Host按键历史／ESSM ID；保留L0—L3、HasWater五位、所有模式Boss、L2图标／楼梯／Portal、Focus／Badge／Question。旧原版地图builder只属历史窄路径 | 物理源槽／有序活动成员／双配置、地图／world／资源同目标，-19隔离，探索回退／特殊房间寿命保留 | 纯地图adapter／扫描失效／资源与门Light替代；错误Getter事实保留，不修官方SDK冒称DLL修复，不用Restore／Refresh兜底 |
| Run／HUD／UI：[Run](run-synchronization-review.md)、[Presentation](presentation-synchronization-review.md) | 退出整flags／回溯体、Stat五值全复制／颜色生产三态／Host展开输入及原值进度查询；保留132B窄Run、最终64B分数、真实身体HUD／心格／Stat三值／有序图像／消息及世界pass | 当前Game／冻结结算owner、身体归组、world／资源／显示批次分开，本地UI／Options保持自身owner | 新结果缓存／typed根与应用、分数／Online隔离完成后迁旧链；不因141行目录漏项删除Minimap／Nightmare等必要原版负载 |
| D0／Random／音频：[逐用途交接](room-random-handoff.md)、[音频边界](presentation-synchronization-review.md) | 退出application_draw全流、37/13B旧D0、背景／地图随机重放、随机Play／SetAmbientSound及仅事件恢复声音；保留5B相位／paused、具名alpha／资格、已选sample与持续voice／crossfade/jingle状态 | world/presentation、subject/use/shape/pass／调用、资源及voice代次／场景生命周期完整；状态与事件去重不混 | 正式批次／事件／非随机leaf与CPU／颜色栈恢复接通后退旧路径，流不上线不能变成结果删除 |
| typed资源／E2E边界：[本地生成合同](local-media-presentation.md)、[现行规范](../../refactor/project-conventions.md) | 新路线接通后退出正式原始媒体／Host前后surface与反馈图必发依赖；保留本地文件加载、文字缓存、本端hallucination、有限recipe、音频控制与失败／退休保护。Lua观测／故障注入仅E2E，原始证据和旧实现事实保留 | 正式资源空间／epoch／描述身份与现行必要依赖一致；保持Hook常驻、C/S隔离、输入／设备与NEVER生命周期 | 迁移旧文字图／结果引用与调用方后再删旧链；固定头存在不启用媒体正文，未知／未接线不冒称已退役，不恢复Lua补源 |

每项实际切换都须同时接管来源、组帧／版本、解码、应用／退休与对应读回口径，避免新旧路径重复发布或旧JSON反填掩盖缺口。源码／历史记录、正常本地初始化、必要关系撤销和资源失败保护按原职责保留；本次没有执行卸载、恢复Hook字节、删文件或产品打包。

## 改动范围

上表列出的文件、`mod/CMakeLists.txt`、观测 Mod 的加载方式，以及相关的文档入口。

## 失败方式

- **误删被依赖的代码**：删除仍被控制消息、E2E 或冷路径使用的代码，导致编译失败，或功能在运行时缺失。
- **旧路径残留**：旧路径仍被某个分支调用，产品 DLL 中保留着 Lua 采集入口。
- **产品依赖 E2E 内容**：E2E 对照 Lua 被产品构建加载，或者产品功能依赖 E2E 文件。
- **生成残留**：旧的生成产物仍然留在构建目录中，被意外包含。
- **Hook 被误删**：顺带删除或改动了 Hook，破坏“Hook 常驻”约束。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **迁移交付**：需实际完成表中各域替代与旧热路径退出，再在实施阶段完成产品／E2E相应纯编译；本次未删除或构建。
- **调用边界**：需静态核对正式热路径不再调用旧接口，同时保留控制消息／当前回映射资料／E2E及本地owner的必要依赖，不把目录历史一并删去。
- **验证安排**：功能验证在第 15 步完成。

## 未决问题

- 冷路径（场景准备）中是否还有依赖旧 JSON 的部分没有被第 11 步的 Baseline 完全取代，需要在实现第 11 步时逐项核对。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步显示／资源退役合同（待实施）**：按[补齐设计](room-display-resource-design.md)在新路径接通后退出完整RoomConfig JSON／Spawn与Merged递归采集、背景RNG轨迹／Init纹理重放、原始水色时间线、Host地图state／holdTime镜像，以及依赖伪造保存体／混合Refresh／Update／分数重算的正式显示补丁。保留实际图像／Sprite／世界结果、各端本地UI与正常资源owner；旧观测及原始失败不改写。不是现在删掉尚无替代的运行功能；新typed布局／source／adapter仍待实施，无测试。

**2026-11-10 RoomSave／Exploration退役边界（待实施）**：[197行目录核对](room-save-exploration-synchronization-review.md)不再把continuity全保存体、四揭图bool、Host地图按键历史、保存项ID／ESSM ID作为正式同步负载；复用L0/L1/L2、当前Entity/Grid和Presentation资源／最终震动／有序徽标。旧Lua／JSON正式下行调用只在替代来源与安装完整后退出，历史完整观测及原始事实按原职责保留，不为退役删除未知或宣称已接线。不得把原Save/Restore／Clear／随机Refresh换成Client补状态的备用链。本轮未退休运行代码或测试。

**2026-11-10 RoomDescriptor退役补充（待实施）**：按[专门核对](room-descriptor-synchronization-review.md)移出旧“四位flags／非Greed Boss清零”假设，保留第五HasWater与普通路线TrapDoor查询；旧24B UNKNOWN结论作为历史，现为Host商店类别／补货表，不恢复原表传输。Error图像接收Host实际结果，颜色adapter仅消费D1的1B是否ModelingClay及实际Player上下文；完整效果ID不发、不伪造Getter，不留本地SpawnSeed旁路。旧全保存vector镜像改为最终图标mask／帧，保留Slot default／Portal／楼梯，完整RoomSave仍Host。配置／reader／显示adapter接通前不假称已退役；本轮仅设计，版本56，无构建或测试。

**2026-11-10 Level退役交接（待实施）**：按[Level静态核对](level-synchronization-review.md)退役正式Level／描述JSON、旧全flags／全seed／完整保存体镜像，以及只靠访问／清理count决定地图重读的假设。旧`ClientMinimapBuild`／原生HUD仍依赖原始ListIndex／Greed宝物房索引／保存投影，不能直接套用L0/L1设计后声称已接通；须换为具名影子显示adapter，保留实际地图实体图标／Grid楼梯、challenge32已选标记、双配置与门Light。官方错误Greed Getter/Setter的18204读写事实及旧路径记录保留，不把它作为真实18374计数来源或对照相等要求；不修改官方SDK后假称目标DLL已修复。本轮只核文档与设计数据，版本56，无构建或测试。

**2026-11-10 Room退役交接（续核修订2）**：[Room职责](room-synchronization-review.md)及[剩余字节补证](room-residual-evidence.md)要求待正式结果来源接通后，退役旧Room JSON中的背景RNG／construction seed／墙纹随机轨迹、四水色端点＋计时、整套颜色生产态、Camera数值／focus ring镜像、FX pose／ray推进速度及雨／补货／效果生产列表。SDK456-DWORD path、BackdropConfig+74/+138整DWORD及+7220 slowdown声明不能继续作为正确字段依据；真实两张448项表及+722C减速均Host。输出函数表、死亡历史、EntityList碰撞／查询缓存和原版Online环不进入新同步。历史静态／运行记录保留；实际背景纹理、水面反向方向、water/light/shockwave结果、环境voice、真实门槽／轨道与FX不能随旧源一起删掉。尚未实施运行退役，生产版本56。

**2026-11-10 Random旧链退役合同（交接修订2，待实施）**：按[核对与历史替代](room-random-handoff.md)在正式结果缓存／资源／adapter接通后退出application_draw的两份四u32、两seed D0 13B草稿及全局游标借用重放；当前D0为5B，随机alpha／资格按具体用途消费。退役Client背景RNG轨迹、随机地图Refresh、随机Play／SetAmbientSound和池／保存RNG镜像的正式同步用途。必要程序资源、地图选择窗口／标记、Error颜色、药丸／预览及实际音频结果保留；不把“流不发”变成“结果删除”。原有诊断按显式启用合同、历史PASS和失败均保留。当前仅静态修订，生产版本56，未退役运行代码或测试。

**2026-11-10 Presentation退役补充（待实施）**：按[字节与职责核对](presentation-synchronization-review.md)退役StatHUD五值全复制、三份ColorModState全同步、SFX全靠事件、Font唯一单例及Host地图展开/本地后端音量相等要求。保留最终心格/实际图像/动态HUD/History顺序、官方星象房行、current六值及持续voice状态。不能因旧141行未列Minimap/HUDMessage/Nightmare而删负载，不能据SDK标签调用4ECA30；历史失败和UNKNOWN保持。本轮未退役运行代码、未测试。

**版本54 Grid水面退役（待实施）**：按[旁表核定及Lock纠错](grid-plus-water-source-evidence.md)把正式来源／u32 typed view／每实例应用和Lock第七覆盖点一起接通，再退出旧JSON的water<=255限制及getter全量回验。不能把尚未接线的高位u32或Lock记成已支持；不引入optional／填充／共享表复制，不要求Client与Host的旁表原值／重置历史相等，也不通过公开Set／Reset补齐画面。

**版本53 两类旧链退役（待实施）**：按[新字节职责](grid-pressure-door-evidence.md)退出旧Door previous／四动画名／current类型／Slot／Direction／Busted原值同步与查询相等，保留真实Room槽映射／双Sprite最终旋转与姿态；删除误把Door+154当bool的door_animation_mode，+28C完整flags改1B Light位投影。NextGreedAnimation内容／借用、GreedModeRNG及两类SpawnSeed不回流新路径；Door原颜色相等改入口布尔＋墙色，TimerPlate／extra保持真实颜色及资源。旧文档与失败历史保留，实际owner接通后退役，不用旧JSON写入或Lua getter给typed路径补缺源。

**版本51 八类Grid旧链退役补充（待实施）**：按[新字节合同](grid-eight-subclasses-evidence.md)移除旧fire_timer／fire_style、ember vx/vy的写入／原值相等要求；五类必覆写主Sprite颜色和Fire第三Sprite残留主帧／选择器也不再比较。保留真实粒子位置／age／maxLife、模板主资源／is_playing、完整overlay、三份Sprite独立资源／图层／glitch及正常本地生命周期。不能把旧JSON的4096粒子预算或完整24B元素直接沿用为新线格式（最多5条、每项16B），也不能让旧通用Sprite反向恢复被裁字段。历史记录不改写，实际来源／安装／绘制接通后才退役旧链。

**版本50 Grid退役补充（待实施）**：按[职责复核](grid-common-synchronization-review.md)在来源与应用owner接好后，退出CollisionClass原值写入／相等、Grid默认动画名和图层spritesheet_path同步／回读，以及旧八项Desc／SpawnFrame要求。保留实际Image、显式动画／selector、原链各一次与安全构造／退休；不能为旧JSON回验再复制Host碰撞、加载请求或旁表optional。最终water_clip_flags和glitch_draw_bits进入正式typed view，不回退到Lua／通用字典，后者必须接通纹理adapter后才能退出旧本地地址随机依赖。 Grid冠军色载体后七值28B也退出写入／比较；保留前四GPU参数和普通颜色11值。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步逐行回映射退役补充（2026-11-09，未实施）**：[1265行新归属](player-member-map.md)保留旧目录身份供追溯，正式路径不再建立ClientPlayerDisplayView的泛化queries_／Captured表，也不按公开getter全量回读相等。P0—P8及第8步实际HUD／具名显示决定owner就绪后一起迁移消费，避免先删必要显示来源。原版通信域和待移除效果队列不进入新状态；不得用旧JSON、原版设备／Online或全量Plus缓存补齐。

**第5步P6—P8退役合同（2026-11-09，未实现）**：[同步职责定稿](5-player-p6-p8.md)退役costume生产元数据／map priority的旧结构比较、native sub／replaced／703四槽的Client写入与全量getter回读、非704 cooldown、原始History／16索引／form字典及完整回放队列下行。保留C/S发现／输入拓扑、真实twin／backup／held和独立激活资格，不能因裁native sub就删SubPlayer组成员。实际HistoryHUD／库存图像列表／分数新owner与Render重算隔离就绪后再切旧消费，不留下空HUD，也不回退Lua／旧查询字典补缺。P8当前记录用真实0／1本地list及ANM，残影完整保留；版本仍46，本轮没有切换运行路径或测试。

**第5步P3—P5后续裁剪（2026-11-09，未实施）**：[新合同](5-player-p3-p5.md)取消旧I14完整innate group／临时list／timer与voided复制要求，改blocked＋有效计数投影；queued charge／VarData／无关flags、crafting池／重复输出原值不再回填，实际HUD结果owner仍须完成。P4空槽不用旧payload做相等条件。P5退出统一四值假定，五种恒零类型不传充能、其它类型只发公式mask，旧MaxCharge／axisAligned／两份chargebar／Host owner及所有攻击关系不恢复，Whip主引用例外保持。原运行来源／writer／预检／漂移对照须一起迁移，不能由旧Captured或JSON反填掩盖缺口；当前版本46通用13／135源未被本次设计改动，旧事实保留。

**第5步P0—P2裁剪合同（2026-11-09，待实现）**：[具体字段职责](5-player-p0-p2.md)取代旧Player final-state表的“全部字段必需”假定：六项属性栏由Presentation缓存承接，cacheFlags／攻击模板／头部锁时等留Host，EternalHearts／BoneHeartsMask原值不传；不得由旧Captured或Lua全树回填。保留五项758850直接Render充能及P2健康／资源原值，不能把展示缓存方案扩大为删除所有原生输入。迁移同时落实P0投影与P6拓扑owner、八类后续Player块及未知；历史测试与失败保留，本轮布局版本仍46，没有退役运行代码。

版本45[Projectile／Effect残余](projectile-effect-residual-evidence.md)切换时，Projectile三个新原生展示字段直接来自生成布局，不能继续依赖旧Lua目录没有这三项而宣称展示齐备；不恢复430／444／468等Host参数。Effect167移148、19／71／194选149，需同步退役旧102含167的分派，保持公开API资格范围；119 callback特例不升级为通用字段或调用入口。UNKNOWN与旧静态失败记录保留，已完成的五项不重做。

版本44 Text／Dummy按[字段复核](text-dummy-evidence.md)退役旧正式Lua／JSON字段链，但保留与Recorder无关的本use/revision真实writer资格；不能把Text Update／Render唯一常驻Hook误删或重复安装。独立`native-text-render-v1`仍为独立观测／对照scope，不能作为普通状态拼接来源。旧duration／KColor末字raw安装与全量漂移比较在切换时撤销，通用KColor20B合同不变；Dummy真实池／Plus／New／Delete／退休owner保留，E2E桥不进入产品。Dummy历史屏幕cache的已知依赖未闭合前保持具体Gap，不以旧完整目录或本地缓存掩盖。

**原正文分域补充**

版本30另按[Entity公共职责](entity-common-synchronization-review.md)退役旧公共计时、伤害、物理输入及历史位置的Source／安装／原值比较。版本31再移除公共valid字段／handler／热比较，由本地构造与发布owner及Player P7结构接管；P7尚未完成前保留旧Player结构源的独立激活资格信息，不得把“移出E2”误作删除该信息。exists／dead继续同步，不调用Kill／Remove替代最终状态。见[生命周期再复核](entity-lifecycle-synchronization-review.md)。

版本32另按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)接管资源存在性、路径／实际Image及本地准备owner后，退役旧事件位原值比较、loaded直接写入／跨端原值相等要求，以及ColorParams队列与颜色锚点的Source／安装／漂移比较。保留事件位本地清理、空↔非空资源正常退休、加载失败不ACK、完整selector及最终11分量颜色；保留is_playing／playback_speed的官方绘制依赖。新具名资源handler未接通前不能删去旧资源owner或声称替代完成；历史记录不改判。

版本33按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)退役Familiar11个参数范围的旧Source／安装／原值比较。保留FireCooldown／State／Coins等真实展示源；80接入最终amount缓存／展示adapter，240接入typed payload、逻辑deque和430E40 owner后，才算接管相应缺口。新节点／payload未知仍不写入；不得因新表存在就删除旧生命周期保护或宣称可玩完整。

版本34按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)退役42个参数范围的旧Source／写入／原值相等要求，撤销Bomb ExplosionCountdown与Tear WaitFrames／StickTimer锚点及漂移比较。保留Knife拖影输入、Tear效果位／DeadEye和Room Light、全部主／辅助Sprite及公共最终绘制偏移；Bomb pending清除和读回继续保留。不得通过调用LoadCostumes、ResetSpriteScale、Height／Scale setter、Update或Interpolate重建已排除输入的效果。通用entity_counters仍服务其他线上计数，不连带删除。

版本35按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)退役Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait锚点及未来循环数组的Source／安装／原值比较，以及Slot八项交互／状态／奖励字段。保留Price与WaterClip、DropDelay展示时钟、所有主／辅助Sprite和真实引用；本地循环安全空初始化、奖品动画／is_playing、价格Sprite资源owner不能随旧字段一起删除。不得用交互、定价、开奖或SetPrizeCollectible重算最终画面；整局HUD／进度域自己的捐款总额等来源独立保留。

版本36按[Projectile运动与展示参数复核](projectile-synchronization-review.md)退役Projectile八项运动／效果输入的旧Source／安装／原值相等要求；Damage／Change保持原排除，PR1预留。保留Height、公共EntityFlags／最终偏移／尺寸／Sprite和DepthOffset单份来源，以及Game帧展示借用；Height临时改写须精确恢复。不能通过Interpolate、Update或本地+470回调恢复曲线／摆动。私有展示UNKNOWN及其未完成接线仍如实保留，不因旧字段减少而称完整。

版本37按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)退役十九项Laser更新／运动／计时输入、六项Homing标量、indices中间缓存／Bezier与六f32系数的旧Source、writer、validator及原值／漂移比较；两个Laser锚点和旧indices i32／4B步长假设一并退役。原生indices已纠正为8B点对，旧SDK／执行器错误保留在历史说明，不能将旧路径误写成已修复。L2／L4／L7／L8及Homing子块0／3／4空预留不要求接收或安装。只安装两组最终点列与真实展示输入，保持本地self／空缓存owner、失败候选清理和正常析构；不得恢复采样、更新、半帧、接触或生成模拟。

**版本38 Effect退役**：按[Effect各臂与payload复核](effect-synchronization-review.md)撤销六项公共更新参数、四变体TearFlags完整128位、136描述八成员的旧Source／writer／validator／原值与漂移比较；136旧room-grid `lifted_descriptors`同样退役，不转移到另一同步域。Ghost旧四份固定Sprite、独立i32 count和Offset／Scale原值路径退役，改有效前缀三投影与专用视图134，保留无效槽正常清理。Trail旧base_prefix／producer未定位读取限制由606190正面证据解决，不能继续报已解决的布局Gap；节点仍完整64B且不连续。MrMe四项Host内部范围不新增同步。保留State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale、三种真实payload、引用与资源及本地owner，禁止通过旧路径重演玩法。本轮只改声明和生成链，未声称旧运行路径已经完成退役。

**版本39装备／Plus退役**：按[完整复核](player-equipment-plus-synchronization-review.md)退役ActiveItemDesc来源池／定时充能冷却、Weapon owner／FireDelay／Direction／axis_parameters／NumFired及Knife hitboxSource的旧Source／writer／validator／原值与漂移比较；正式ReadKnifeExtension接口已删除，旧Lua／JSON运行链的实际切换仍归本步。旧MaxCharge／两份chargebar重复负载改由保留字段及已核动态公式重建，不以旧比较要求恢复多余字段。保留Whip真实主引用、五槽类型与存在性、主动槽六项输入、TemporaryEffect三值及外层disabled；外层mask／evaluate请求和本地Player地址不再复制。Plus公共两nullable与同类holder／optional owner保持，已排除的各派生Plus项不恢复；所有旧本地边／容器经正常owner清理，不能memcpy零或恢复Update／Fire／HUD Update／采样／重抽。
**版本40 Effect后续五项退役合同**：见[证据与落地](effect-next-five-evidence.md)。清理旧FX0=8B／FX1=21B常数、102接收113的旧映射、Trail通用16384上限，以及“静止Effect所有锚点必不重发”的判断；统一改用版本40生成表和完整尾窗127上限。旧119回调／120生成／传播请求及113两标量不得作为新同步字段恢复，VarData残余UNKNOWN仍不上线。未知非空／错variant holder须报告，不借旧Source兜底或按空继续。保留三种表现payload及其正常资源／容器／共享owner生命周期，不增加Hook或重演玩法。本轮布局及合同已更新，正式旧运行链的退役仍由本步实施，未运行测试。

**版本43 NPC现行合同**：旧0/1/2本地设备谓词方案已撤销：删除其来源/回退及特殊未决资格，不读取或Hook原版NetplayManager；C/S只保留ReadNpcControlledUi的0/2。raw ControllerId不传不写，旧模式1不接受。正式迁移仍覆盖N0—N6、完整三deque、真实引用和双Sprite，17/18空预留，其他Host/UNKNOWN/生命周期边界保持。见[来源闭合](npc-control-ui-evidence.md)。

**原正文其余修订提示**

**版本46 Delirium退役合同**：按[静态处理](delirium-residual-evidence.md)切断旧EntityDelirium字符串字段的Client原值安装与无条件shape逻辑；新增两个u64只是原生字节归属，不得接回旧AI writer。BE8不上线、不预约、不连带Remove，companion仍按正常成员生命周期保留／退休，不因删掉旧专用来源而漏掉独立NPC。world_delirium_owner继续常驻，仅在合法重配收口恢复读取资格；143稳定type选臂及BD4未知保持，记录到的原生池复用机制不作为可发送旧值的理由。实际切换仍属本步。
