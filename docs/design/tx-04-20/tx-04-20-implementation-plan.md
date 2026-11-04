# tx-04-20：四参与者、多身体 Player 客户端应用具体实现计划

日期：2026-11-04。当前进度：**I01—I10目录、目标、来源、投影、身体计划及结构owner／关系接口已接入**，见[基础实现与主审](tx-04-20-foundation.md)、[I05—I10结构实现与必要逆向主审](tx-04-20-structure.md)、[逐签名应用目录](player-application.json)、[R01—R16研究结果](player-native-static.md)。I11—I40待推进；本轮未构建、测试或启动游戏。缺完整最终量／资源／角色adapter时写前明确拒绝，native_apply仍为false，不声明完整原生Player已应用。未闭合私有writer／角色／武器／清理分支随具名adapter采用时补证，不能由结构合同外推支持。

初版规划事实（同日，保留）：当时仅编写文档、阅读现有代码及本地资料，I01—I40均待实施，无新增逆向结果／运行代码／构建／测试。下文“应”“完成表现”继续表示完整实施要求，不自动表示已完成。

遵循[现行项目规范](../../refactor/project-conventions.md)和[完整路线图](../state-synchronization-roadmap.md)。整个 tx-04-20 是一个推进目标；I／R／P／E 编号只作任务索引，不拆成新的 tx，不设 gate、不校验哈希、不要求独立角色或子智能体。全部目标完成后才进入 I39 统一 E2E，失败在同一 tx 内修复、续测；本次文档验收仅在全部编辑结束后统一静态审阅。

配套文档：

- [玩家状态、全部角色类型、身体关系与武器应用清单](tx-04-20-player-application.md)：P01—P20 状态族、C01—C17 角色族及 W01—W15 武器类型。
- [静态确认与必要逆向计划](tx-04-20-native-research-plan.md)：R01—R16，逐项说明现有依据、具体缺口、研究方法及实现落点。

## 1. 完整目标和交付边界

在现有唯一原生 ClientScene 中应用 Authority 的完整玩家目标，支持一至四名参与者、一个输入源对应一个控制组、多于四个身体、活动／非活动／备份／替换身体、装备和完整身体表现。Player 与 Room／Grid／非 Player Entity 使用同一个目标、attempt、原生实例依赖和提交 owner；不构建第二套运行世界。

本 tx 必须完成：

1. participant—control group—身体稳定 Key 的完整关系，原生 roster 和非 roster 身体分别承载；四组 Jacob／Esau 的八活动身体以及指定切换／备份组合必须真实应用。
2. Player 公共状态、最终属性、所有心量和资源、装备／主动／口袋槽、充能、临时效果、变身、History、角色专用值及必要 HUD 数据。完整来源、已应用值和 Authority-only 事实分别标记。
3. 主体／身体／头／手／服装／武器的资源、完整姿态、组合绘制、举持和与非 Player 的关系；不能以主 Sprite 能画代替完整外观。
4. create／change／remove、同 Key 形态变化和本地实例替换、重复／旧目标、取消、部分失败、重建、正常停止与退休；保持输入、玩法、随机、Lua 和访客持久副作用隔离。
5. 正式观测到纯值 T 再到真实 Client 后态及画面的统一 E2E，包含 18／19 回归、准确能力范围和后续接口。

责任边界：

| 邻接任务 | 本 tx 要交付的接口 | 不在此提前宣称完成的内容 |
| --- | --- | --- |
| 09／10／13 | 复用控制语义、Entity 本体和 player 关系／History 来源，补本次确有消费者的缺口 | 不重做一套输入真值或通过 Client 构造伪造 Authority 出生 |
| 17—19 | 完整 Player owner、身体资源和跨域引用，消除当前单普通 Isaac 的产品限制 | 历史代表 profile 保留原范围，不改写原通过／失败 |
| 21／22 | 完整 Player 应用结果、同 attempt 原生提交、失败／恢复原因，供 Baseline／Delta 消费 | 四端实时世界下行、网络 Applied ACK、丢包恢复仍由 21／22 接线验收 |
| 23／24／26 | 身体跨 scope 的保留／撤销策略、备份来源和新实例重建接口 | 不把顺序导入跨房样本当实时切房、换层事务已完成 |
| 25／27 | 角色规则当前结果及 HUD 只读视图、独立表现的具名交接 | 不重演随机选择；完整 HUD 布局、音画事件消费归 27 |
| 28／29 | 正确死亡中存在性、幽灵／复活后的当前身体、全部原版 Player 类型及专用状态族 | 完整死亡／团灭／结局流程和所有道具组合自然游玩由 28／29 扩展，不能借此后移本 tx 必需的当前应用 |
| 30—34 | 能力、预算、生命周期和产品构建边界 | late join、合法 Continue、整局性能及发布联合验收不在本 tx 冒报 |

## 2. 当前事实与具体缺口

下表来自本轮源码／资料阅读；“已有”不外推为完整 Player 已可应用。

| 事实与位置 | 对本 tx 的直接影响 |
| --- | --- |
| [09 实现](../observation-expansion/four-participant-implementation.md)、`mod/src/game/topology.h`：四参与者；每组关系遍历容量 64；原生主 roster 容量与四个连接身份分开；Esau Jr 是全局四槽 | 不能使用 `Player[4]`；64 是现有关系预算，不是任意组合都可创建的承诺；备份槽不能按 ordinal 分配 |
| [13 实现](../observation-expansion/tx-04-13-implementation.md)、`domains/player/collect.lua`：本体引用同窗口 Entity scope；独立记录 History、control_relation／membership 和备份槽 | Player 域不能再复制另一份本体真值；备份缺少合法字段来源时必须补来源，不能补零或只保留一个关系标签 |
| `mod/src/replica/entity_target.h` 已保留 Player payload、player_keys 和正式字段；19 应用目录将 Player／Weapon／Costume 等交给 20 | 新 Player 目标组合 entity 与 player 两域，沿用 SubjectKey；目录中的“保留”不等于原生安装 |
| `client_objects.cpp::PrepareTarget` 无外部 owner 时只允许一个普通 Isaac；旧代表解析要求角色 0，普通支撑拒绝自身跨对象关系、换形和多身体 | 接入完整 owner 和全量计划，不能只删这些拒绝条件 |
| `client_objects_native.cpp` 当前构造要求空 roster，设置首 PlayerIndex；当前 ResolveEntity／Preflight 以公开 roster 成员资格证明 Player 借用 | 建立按 body role 区分的资格见证，修改非 roster 身体借用与计数合同，不能把备份硬塞入活动 roster |
| `ClientEntityOwners::{resolve,dependency,player_ready}`、`ClientObjects::ConfigureEntityOwners` 已提供扩展接口 | 新 owner 在计划开始前配置；按稳定 Key、shape、实例代次、角色及用途返回借用，禁止回退首玩家 |
| `client_scene.cpp::SetCommittedRange` 当前只报 non_player_entity_applied／player_support_applied，full_entity_applied 固定 false | Player 完成后重新定义精确提交谓词，不把一个布尔值直接改成 true |
| [19 验收](../tx-04-19/tx-04-19-acceptance.md)：单 Isaac＋BodySprite＋Effect136 持物；Player 自体动作／伸缩／冷却闪烁仍是 20 边界 | 纳入完整 Player draw，接管旧窄分支而不双画／双持有；保留 136 的 Parent、shape、资源及外层 CPU 消费合同 |
| `mod/tools/replica_projection_policy.py` 省略 GetWeapon、主动／口袋槽、CostumeNullPos、TemporaryEffects 等参数查询 | 逐个恢复真实消费者所需数据或改成等价完整紧凑结构；不能假设当前发送模型已含所有 Player 值 |

## 3. 数据、owner 与执行合同

### 3.1 纯值目标和状态归属

建议新增 `mod/src/replica/player_target.*`、`player_plan.*`；确切命名可随实施调整，职责不能丢失。

| 结构／接口 | 必须包含和约束 |
| --- | --- |
| PlayerTarget | T 的 scope／revision、entity 与 player 来源 scope／token／schema、完整性与缺失原因、能力版本、身体全集、roster 顺序、控制组、备份槽、共享关系；不含地址 |
| PlayerBodyTarget | 既有 Entity SubjectKey／use／shape、角色类型、存在性和 body role、active／can_consume 的 Authority 事实、公共／专用最终值、状态有效性、资源和具名引用 |
| PlayerRelationTarget | participant／group／control_session／binding_revision；主／副／twin／sub／替换／备份边及源依据；Null、Unresolved、NotCaptured 分别表达 |
| 装备／资源子值 | 容器有稳定槽身份、顺序、完整性和显式空；Weapon／costume 是具名身体子 owner，不因文件相同合并；原生指针及 vector 头不进入模型 |
| ClientPlayers（建议） | 唯一 Player 实例／子资源 owner；向 Entity19 暴露受限借用和就绪见证；原生地址只在主线程实际使用 |
| PlayerApplyResult | 明确本体、关系、资源、原生检查及失败前缀；不是网络 ACK；最终只由 ClientScene 提升 A |

Entity 本体以 entity 规范记录为准；player 域负责关系／连续性和 History。player.canonical_state 必须匹配目标内同一提交窗口的 Entity 来源；不能拼接“最新的两个域”。独立事件保持其自身 scope，不当作普通快照补洞。缺席／读取失败不等于删除；只有完整成员集合或明确退休语义才允许计划 remove。

每个目录成员必须落到一种具名处置：原生直接最终量、受控结构／资源安装、只读派生视图、Authority-only 事实、交给具体后续消费者、或明确未支持原因。凡本 tx 原生 draw／依赖／HUD 数据接口实际读取的字段，不能以“纯值保留”代替应用。共享状态按来源显式关系表达；两个身体值相同不是共享证据。

### 3.2 同一原生场景的应用次序

1. 解析完整 T，关联两域、角色和资源，检查数值／身份／预算／依赖；此时不改原生 A。
2. 从最后有效 A 到 T 生成身体和子 owner 计划，区分可原位改值、资源重建、实例替换与全场景重建；B 仍只由协议模型管理。
3. 进入已保护的 Client 原生窗口，停止旧展示／新借用，排空已知在途使用；保留首因和实际执行前缀。
4. 按核定合同取得 Player／角色必需结构和 roster／非 roster 归属。构造自动生成的 twin／sub／附属 Entity 必须与 T 具名对应或由已解释的窄路径抑制，不能额外创建世界对象。
5. 安装公共和专用最终量、装备容器、主／辅助资源、costume／Weapon 必需结构；公开 setter 仅在已解释其副作用后采用，不运行 EvaluateItems／Update 补算。
6. 发布带用途的 Player structure-ready 见证，供非 Player 构造必要依赖；全部实例存在后绑定后置引用、持物、Weapon main entity、跨身体和 Grid 关系。结构就绪与最终 draw-ready 分开，避免循环等待。
7. 回读本次实际原生对象、容器、关系和资源，核对 roster／非 roster 集合及唯一 owner；构建 draw 的必要表现缓存，不运行玩法。
8. 18／19／20 在同 attempt 全部满足声明的应用范围后由 Scene 唯一提交，开启展示；失败不展示或确认半套状态。
9. 写前取消保持 A；写后失败撤销 A 的展示资格，按实际前缀安全退休后从完整 T 恢复。未知异常继续保持隔离、保留 owner，不盲目析构或交还 Off。

不是所有引用都能后置：Weapon／twin 初始化需要先决身体时由 R02／R08／R10—R12 明确顺序。无法成立的构造环必须补具体结构方案，不能以空指针初始化碰运气。

## 4. 全部实施任务索引

I01—I04当前交付见上方实现说明；下表保留完整任务内容。依赖是技术关系，可按不冲突部分交错阅读／编辑，不是审批或完成率门槛。

| ID | 任务 | 主要依赖／研究 | 统一验收覆盖 |
| --- | --- | --- | --- |
| I01 | 已接入逐签名应用目录与具名责任映射 | 10／13／19，P01—P20 | E01、E22；运行归I39 |
| I02 | 已接入PlayerTarget、版本、两域／owner token关联及Scene／计划共享 | I01，R01 | E01、E22；原生owner归I07以后 |
| I03 | 已接入正式只读来源扩展、完整有限查询及12份新增原生ANM；具体私有writer缺口不补值 | I01，R01、R04—R13 | E02—E20；投影复核归I05 |
| I04 | 已形成R01—R16采用／否决合同、必要逆向与主审；未闭合具体分支保留 | R01—R16 | 支撑所有运行项，不单独运行测试 |
| I05 | 已复核P1—P6并恢复Player／装备／History按参查询发送 | I01—I03，R01 | E27；I39未运行 |
| I06 | 已接入A→T身体／子owner计划、变化分类及Scene重建选择 | I02，R02、R15 | E01、E21、E23；I39未运行 |
| I07 | 已接入稳定Key／shape／实例／owner epoch与四用途资格 | I02、I06，R03 | E02—E08、E21；I39未运行 |
| I08 | 已接入受控base结构构造、roster／Room分开发布及逐前缀清理；完整状态adapter归I11以后 | I04、I07，R02 | E02—E08、E23；I39未运行 |
| I09 | 已接入正式集合来源、四备份关联及非roster owner借用 | I07、I08，R03、R11 | E04—E08、E21；I39未运行 |
| I10 | 已接入具名槽安装／拆边、跨域依赖和Weapon后置关系接口；具体动态实例归I20 | I07—I09，R03、R08、R09 | E05—E08、E18、E22；I39未运行 |
| I11 | 公共状态、运动和最终属性 | I03、I08，R04 | E09、E17、E20 |
| I12 | 全部心量与当前生命状态 | I11，R04 | E09、E21 |
| I13 | 资源及角色计数 | I11，R04、R12 | E09、E16 |
| I14 | 道具／饰品／固有效果库存 | I03、I08，R05 | E10、E16 |
| I15 | 主动／口袋／卡药槽与充能 | I14，R05 | E11、E16 |
| I16 | TemporaryEffects 与变身最终值 | I14，R06 | E12、E16 |
| I17 | History、共享和每身体独立状态 | I09—I16，R03、R06 | E05—E08、E12 |
| I18 | 主／身体／头／手等完整资源姿态 | I03、I08，R07 | E13、E20 |
| I19 | costume 容器与动态换装 | I14、I16、I18，R07 | E13、E16 |
| I20 | 全武器类型与关联 Entity | I10、I15、I18，R08 | E14、E15、E18 |
| I21 | 举持、投掷结果和复合依赖 | I10、I18、I20，R09 | E18 |
| I22 | 普通角色和最终形态注册 | I11—I20，C01—C05 | E03、E16 |
| I23 | Jacob／Esau 多组双身体 | I09—I20，R10 | E04、E05 |
| I24 | Forgotten／Soul 两套身体语义 | I09—I21，R10 | E06 |
| I25 | Tainted Lazarus／Flip／Birthright | I09—I20，R11 | E07 |
| I26 | Esau Jr 四槽与备份恢复 | I09—I20，R11 | E08 |
| I27 | 其他角色专用状态族 | I13—I22，R12，C02—C05／C11—C17；与I23—I26共用状态适配 | E16、E18 |
| I28 | 死亡中存在性、换形、替换和删除 | I06—I27，R15 | E21、E23 |
| I29 | 全 Player draw 和必要维护 | I18—I28，R13 | E13—E20 |
| I30 | 控制组／输入代次与 Client 隔离接线 | I07、I09、I28，R14 | E17、E24 |
| I31 | HUD／角色展示只读数据接口 | I12—I20，R14 | E19 |
| I32 | 玩法、Lua、RNG 与持久全窗口隔离 | I04，R16；贯穿所有原生调用 | E24、E25 |
| I33 | 逐前缀退休、在途资源和恢复 | I08—I32，R15 | E23、E25 |
| I34 | Scene 唯一提交与准确能力 | I06—I33 | E01、E22—E25 |
| I35 | 有界容量、缓存和成本 | I02、I05、I29、I33 | E28 |
| I36 | 状态／错误观测和故障注入边界 | I34、I35 | E22—E25、E28 |
| I37 | 构建接线与产品边界 | 所有新增模块／Hook | E29 |
| I38 | 统一 E2E Source／冷 Client 编排 | 第 6—7 节失败模型／矩阵 | E01—E30 |
| I39 | 完整统一验收与连续修复 | 完整实现目标 | E01—E30 |
| I40 | 结果、能力目录、入口与后续交接 | I39 实际结果 | 如实报告覆盖和未决项 |

## 5. 每项任务的实施细节

### 5.1 工程落点与职责分配

下表区分当前文件和建议新增文件；建议名称不是已存在实现。所有模块仍属于同一 tx、同一 Scene，避免把每种角色写成一套独立世界。

| 任务 | 当前需要接线的文件／目录 | 建议新增职责 |
| --- | --- | --- |
| I01—I05 | `mod/tools/gen_entity_catalog.py`、`gen_continuity_catalog.py`、`gen_entity_application_catalog.py`、`gen_replica_catalog.py`、`replica_projection_policy.py`；正式 `mod/observation/isaac-cs-observation/domains/entity/`、`domains/player/`；`mod/src/game/world_sources/` | `mod/tools/gen_player_application_catalog.py`、本目录 `player-application.json`；Player正式补充来源及独立版本 |
| I02／I06 | `mod/src/replica/entity_target.*`、`entity_plan.*`、`model.*`、`codec.*` | `mod/src/replica/player_target.*`、`player_plan.*`：只处理纯值／依赖，不包含SDK对象 |
| I07—I10／I28 | `mod/src/game/client_objects.*`、`client_objects_native.*`、`client_entity_model.*`、`client_entity_adapters.h` | `mod/src/game/client_players.*`、`client_players_native.*`：身体注册、PlayerOwner、实际构造／列表／引用和生命周期 |
| I11—I17／I22—I27 | 正式Entity／player来源、现有资源／对象接口 | `client_player_model.*`负责不可变状态解码；`client_player_state_native.*`负责字段／容器安装；角色差异按具名策略分组，避免巨型角色switch同时包办采集／安装／绘制 |
| I18—I21 | `client_resources*`、`client_resource_model.*`、`client_entity_class_model.cpp`、`client_entity_links.inc`、`client_entity_owned_native.cpp` | `client_player_resources_native.*`、`client_player_weapons_native.*`：costume／Weapon子owner及资源寿命；held跨域引用仍由统一对象服务解析 |
| I29 | `client_presentation*`、`client_entity_draw_native.*`、`client_native_world.*` | `client_player_draw_native.*`：Player pass／临时借用，接既有唯一分层draw和外层队列，不自建renderer |
| I30／I31 | `mod/src/game/topology.*`、`player_api.cpp`、`mod/src/core/participant.*`和现有Client输入会话；`client_scene.*`已提交模型 | 只读Player控制／HUD视图；按实际需要新增接口，不让HUD或网络线程持有native Player |
| I04／I32／I33 | `mod/binding/hooks.json`、`world_layout.json`、`mod/src/hooks/registry.cpp`、`client_lifetime.*`、`client_native_world.*` | 具名用途分派与Player逐前缀退休，沿唯一Hook／借用owner |
| I34—I36 | `client_scene.*`、`client_entity_budget.h`、`mod/src/ops/client_fixture.*`及现有状态发布 | Player预算、准确提交范围和只读诊断；生产字段与E2E故障接口分开 |
| I37—I39 | `mod/CMakeLists.txt`、`mod/src/game/client_e2e_native.cpp`、`mod/src/game/client_entity_e2e.*`、`scripts/run-tx-04-19.ps1`、`tools/harness/scenarios/tx-04-19.json` | Player E2E场景／真实后态桥、20统一入口与场景文件；产品构建不包含测试实现 |

### I01：全成员应用目录与责任映射

以现有 entity／continuity 目录和 19 的 `PLAYER_RELATED` 为输入，逐成员、重载及参数范围登记 Player、继承 Entity、Weapon、TemporaryEffects／TemporaryEffect、CostumeSpriteDesc、ActiveItemDesc、PocketItem、QueueItemData、History／HistoryItem、PlayerManager 和配置／资源依赖。补独立原生表现成员，不以公开 Getter 数量当作完整绘制覆盖。

建议新增 `mod/tools/gen_player_application_catalog.py` 及本目录 `player-application.json`，字段至少包括来源、有效期、角色适用性、同窗口要求、共享归属、安装方式、真实消费者、清理责任、I／R／E 索引、实现／运行状态。将 19 的具名交接链接到新目录；不要将所有未分类字段默认标成“已处理”或自动纯值保留。

### I02：纯值目标、版本与两域关联

实现 PlayerTarget 及两域关联解析，复用 `replica::SubjectKey`、Scope 和既有图解析；深复制自有值，保留 int64／位集／有符号字段与显式空，严格区分缺失、不可用和零。明确必需字段、可空成员、各身体的完整性，不将一个完整普通身体扩大成整个 Player 范围完整。

更新正式模型和能力协商：旧单 Isaac profile 仍可按原合同识别，新目标缺必需字段／来源版本时写前明确拒绝。能力版本按新增语义决定，不在文档预猜现有全局版本号。完成表现是 parser、计划和 Scene 都消费同一 PlayerTarget，不靠夹具元数据决定角色能力。

### I03：正式只读来源补齐

对照 P01—P20 的真实消费者，复查 `mod/observation/isaac-cs-observation/domains/entity/`、`domains/player/collect.lua`、`mod/src/game/world_sources/`。先公开只读来源，补具体缺口才使用核定原生读取；不为 Get 前缀接口假定无副作用，不轮询 GetBombFlags、攻击参数或随机选择方法。

库存／costume／Weapon／非活动身体缺少完整来源时补正式有界结构、读取依据和来源 owner；当前 PlayerUpdate 见证仅适用于项目已接受的当前身体，不能覆盖从未正常更新的备份。资源必须是真实成员的最终状态，必要 draw scope 独立保存。来源缺口引起 Partial 时保持真实原因，不由 E2E 填正式值。

### I04：静态确认和必要逆向

按 R01—R16 实施，公开 Wiki→官方源码／SDK→已有 J273 研究→具体未暴露调用的必要逆向。每个采用调用形成 ABI、前置、线程／用途、读写、副作用、返回／失败前缀、owner／资源／清理说明；只需要源码确认的成员不强制逐项反编译。

研究结果写回功能目录及对应实现说明，保持直接事实、推导和 UNKNOWN 区分；不得把普通 Isaac 已采用 Init／Pre 路径直接套到所有角色。未闭合项保留具体工作，不以删除拒绝分支代替研究。此项不运行探针、游戏或分项测试。

### I05：投影与参数查询

逐项复查 `replica_projection_policy.py` 被省略的 Player／Weapon／TemporaryEffects／History 参数查询，以及 P1 资源别名、P2 静态动画表、P4 占位、P5 图编号、P6 独立查询。有限槽完整采样；物品 ID 使用当前配置／实际容器的明确范围，不能只问默认槽或无限枚举。

消费者需要的值可用完整紧凑库存／槽数组／资源 null-frame 代替冗余查询，但须登记等价语义、失效条件及参数范围。未知或不可读不被投影消掉；同文件的主／Body／costume 不构成别名。更新生成器及正式能力，完整观测不随发送优化缩减。

### I06：A→T 计划

生成身体和子 owner 的 create／change／remove／replace 计划，分别比较成员完整性、角色／shape、资源、关系和状态。重复 T 不重复构造，旧 revision 不写世界，同 revision 内容冲突报告错误；同 Key 换实例与同地址新 Key 分开。

计划明确操作先决和成本、写前取消及写后恢复范围；某角色必须重建整个场景时显式选择安全重建，不把它伪装成无副作用原位 setter。只有最后有效原生 A 可作差分依据，半应用状态不成为下一轮 A。

### I07：稳定身份和实例 owner

建议新增 `mod/src/game/client_players.*`、`client_players_native.*`，保存稳定 Key→本地实例／owner epoch／shape／role，沿用 Scene 的线程与借用合同。Participant、control group 与 Entity 身份各自独立，PlayerIndex／ControllerIndex 是当前本地关联值。

实现正式 PlayerOwner 接口，借用核对作用域、原生有效性、实例代次和用途；区分构造依赖、渲染依赖、只读非活动状态，不赋予 Client 模拟资格。关系冲突、跨组复用、旧 use 或旧 scope 必须给明确首因，不能按索引／种子／地址猜身份。

### I08：构造、roster 和发布

沿 R02 的实际前缀实现分配、构造、角色必要结构、PlayerManager 归属、Init、官方扩展、资源与 Room 成员发布；替换当前“空 roster＋唯一 Isaac”的产品路径。明确原生数组／vector 所有者和 allocator，不能复制容器头。

按角色处理自动生成的第二身体／Familiar／Weapon：T 中已有独立 Key 的对象只创建一次。未发布、半 Init、已入 roster 分别记前缀，预设无 owner 的失败不得丢 raw 指针。PlayerIndex 本地分配不改变 participant ordinal；不走原版 Online／Isaac Sync Join。

### I09：活动、非 roster、非活动和备份集合

分开全部身体集合、公开 roster 顺序、Room 成员、可见成员、控制组成员和四个备份槽。读取资格与存在性分开，非活动仍可能需要真实对象／资源以支持关系与切换；是否延迟物化必须逐角色有消费者和生命周期依据。

修改 Entity19 的 Player 借用／Preflight／roster 计数，使其按用途接受核定的非 roster 身体，而不是去掉存活检查。备份、全体幽灵或切换中的暂不可控组不误当全局错误；零活动输入身体不等于所有 Player 已退休。

### I10：跨身体、Entity 和 Grid 关系

安装 parent／child／target／spawner、main／other twin、sub-player、替换／备份、Familiar owner、Weapon owner／main entity、held Entity 和 minecart 等具名槽。EntityPtr、裸借用、拥有型指针分别使用实际合同；Null 要清旧引用，未解析不能偷换成 Null。

分开结构就绪、资源就绪、关系完成和 draw-ready，解决 Player↔Knife／Familiar／Effect 的构造环。owner 消失或换实例时使反向关系、Lua／Sprite 借用和缓存一并失效；Effect136 与 18 lifted descriptor 保持同一 T，不多建活动 Grid。

### I11：公共状态、运动与最终属性

复用已核定 Entity 公共成员合同，由 Player owner 安装实际位置／速度／偏移／碰撞与显示属性、FrameCount、方向、最终伤害／射速／射程／移速／幸运等及实际需要的冷却／渲染状态。Source Index／seed 不能覆盖本地分配身份。

不运行 Player Update、EvaluateItems、输入 consumer 或碰撞以重新算出属性；需要只读派生字段时指定纯值来源或核定 leaf。安装后从真实身体回读，单位、符号、浮点及位宽依字段合同判定，不给所有字段套同一容差。

### I12：心量及生命状态

按 P03 完整处理红／最大／魂／黑／永恒／骨／金／腐／破碎心、心量排列／掩码和角色特有容器、无敌／闪烁／幽灵相关显示值。保留角色合法范围，不能沿用普通 Isaac 的 max_hearts>=hearts 判据覆盖所有角色。

伤害、治疗、死亡与复活是 Authority 裁决；Client 安装当前结果，不调用 TakeDamage／AddHearts／Revive 来达到目标。IsDead、Exists、IsCoopGhost 与 roster／输入资格分别判断，不能把死亡直接转成 remove。

### I13：资源和角色计数

安装硬币／钥匙／炸弹、金钥匙／金炸弹等最终标志、魂／血充能及角色专用计数，保留每身体／共享的来源含义。增减 API 只有已证明为合适 leaf 才采用，否则走核定最终量适配。

角色计数变化不能触发本地掉落、支付、奖励或物品获取。所有清空／回退须清旧值；角色不适用成员保留 NotApplicable，不用零伪装读取成功。

### I14：库存、饰品和固有物品

构建完整有序或具名计数库存，区别普通持有、临时／固有、熔炼／金饰品、阻止／隐藏／效果乘数以及真实共享关系。配置 ID 与本地资源可用性先核对，未知配置不能回退成空道具。

不循环 AddCollectible／RemoveCollectible／AddTrinket 重放装备变化，不依赖 EvaluateItems 创建 Familiar／costume。装备的最终容器、最终属性、表现资源和非 Player 成员分别应用，再核对一致性；History 不是库存替代来源。

### I15：主动、口袋、卡药和充能

逐槽表达 slot kind／index／item、ActiveItemDesc 全字段、普通／电池／小数／子充能、冷却、卡／药和 PocketItem 状态、排队物品及需要的可用性结果。槽容量以目标版本合同为准，不假定只有主副两个槽。

清空、交换、学校书包、口袋主动和共享充能都安装 Source 后态；不调用 UseActiveItem／UseCard／UsePill、Discharge 或触发动画来模拟变化。主动槽 VarData 若编码备份槽／关系，应由对应专用合同解释，不能作为地址转发。

### I16：TemporaryEffects 和变身

分别安装 collectible／trinket／null effect 的真实列表、计数／持续状态以及变身最终标志，处理相同效果重复、清零和角色切换失效。服装和属性变更由 I19／I11 使用最终源接入。

不得通过添加临时效果链重复触发玩法；公开查询与底层列表不一一对应时列明映射。只能保留 Authority 事实的派生内容说明消费者，任何 draw 所需状态仍要补齐原生输入。

### I17：History 与共享关系

消费 player 域同窗口 History／HistoryItem 的获得顺序、时间、房间／楼层和物品类型；与当前持有、先天物品、临时效果分别保存。为 27 提供稳定只读视图，不调用取得道具路径重造历史。

共享关系来源明确到角色／槽／字段，不能整个 Player memcpy 到 twin。角色切换、备份恢复或成员退出后，shared owner 和各身体独立值一起更新；来源冲突报告具体字段／身体，不按第一个身体覆盖其余对象。

### I18：基础资源和完整姿态

复用 `client_resources*` 服务，为真实主／Body／head／手／辅助成员建立资源 slot；哪些部分是同 ANM 的层、哪些是独立 ANM，由 R07 确认。保留 main／overlay、精确 phase、层状态、颜色、shader、缩放、null-frame 和换图结果。

换资源前撤销内嵌 Sprite／Lua 借用，CPU／GPU 使用结束后才释放旧 owner。不要通过 Update／SetFrame 后再推进动画获取姿态。19 的 BodySprite 窄支撑迁入同一 owner，不能同时安装两份。

### I19：costume 完整容器

按真实 CostumeSpriteDesc 顺序、优先级、配置、可见性和资源姿态重建容器，涵盖头／身／手、飞行、颜色、覆盖／隐藏及动态替换。相同资源路径的两个 costume 实例各自保存状态。

采用已核定的原生容器分配／释放方式；不通过收集物品刷新外观。容器缩小、空集合、角色重配和取消均退休旧资源；缺图／层／配置写前或准确前缀失败，不静默去掉该服装。

### I20：Weapon owner 与所有武器类型

按 W01—W15 建立武器 slot／动态类型、owner、main entity、charge／delay／方向／修饰和必要资源／私有表现的合同。Weapon 不是 Entity Key，使用身体 Key＋稳定 slot／代次描述，实际 Entity 引用仍走统一 Key。

核定 weapon 工厂／初始化的连带创建和退休，采用最终状态安装；不调用 FireTear／FireKnife／Shoot／Weapon Update 补本地攻击。Familiar、Laser、Knife、骨棒／剑／鞭的真实 owner 必须满足 19 的依赖；旧武器与新武器不能共用失效指针。

### I21：举持和其他复合依赖

统一 held Entity、ItemState／SuplexState、Body null-frame、持物偏移／缩放及相关表现。复用 19 已核 136 路径，扩到本 tx 的其他原生持物类型；物品动画、举持真实 Entity 和独立事件分开。

投掷后若变成新 Tear／Effect，只按 T 的新旧成员处理；不调用 TryHold／Throw／Suplex 或 PickupGridEntity 重演。持物自身 Visible=false 的合法父级代画与普通可见成员不得双画；临时 draw 状态要保留到最外层已知 CPU 消费结束再恢复。

### I22：普通角色与最终形态注册

按配套 C 表注册全部 PlayerType，明确普通外观／状态族与专用族，不仅允许角色 0。常规复活形态、黑 Judas、Lost、Keeper、Lilith、Apollyon、Bethany 等都选择对应心量、资源和依赖规则。

Source 最终角色和 seed 是结果，Client 不运行角色开局配装／随机 Eden／复活奖励链。0—40 每个真实类型都有应用责任；-1 possessor 与非完整 Baby 另作真实内部形态分类，不伪装普通角色，也不从枚举存在推导新参与者功能。

### I23：Jacob／Esau

支持四组双身体八活动对象，各组主／副、roster 顺序、同控制意图、独立库存／心量／资源／姿态分别安装。检查身体在两个组重叠、缺副体、反向关系冲突和局部退出。

不得按 participant ordinal 直接索引 Player roster；边沿只经一次 canonical 处理，各身体原生规则仍由 Authority 执行。Client 的双体只是显示同一权威结果；一组退休不删除其他组身体。

### I24：Forgotten／Soul 与 Tainted 组合

普通 16／17 复用 09 已知同对象状态／类型交换事实，但 Client 原位采用路径须重新核定；主 roster 与 sub-player、可见／可控和两套心量分开。35／40 的携带实体／灵魂与实际活动资格按独立合同实现。

切换、抛掷／落地结果、骨棒和持有关系使用 Source 最终值；不在 Client 执行 SwapForgottenForm 或模拟投掷。非 roster 身体要能被正确借用和退休，不为满足旧检查改成额外参与者。

### I25：Tainted Lazarus、Flip 与 Birthright

明确 29／38 两个身体和 Birthright hologram 的角色、装备、活动／可见资格及关系；状态交换与对象替换按实际 owner 区分，不按名称推导 pointer swap。保留每身体装备／costume／Weapon 和共享字段。

Flip 只应用结果；原生不允许 hologram 执行的动作不要求 Client 或 Authority 补做。旧研究中递归清理导致重复移除的失败纳入 I33，不能按稳定旧 roster 顺序逐个盲删。

### I26：Esau Jr 备份

按真实四个全局槽和主动槽 703 的关联表达备份，不按参与者／PlayerIndex 猜槽；活动、备份、替换和退休后保留的原生槽分别管理。备份读取资格、对象创建和资源寿命由 R11 补齐。

切换及恢复要正确映射逻辑 Key 和本地实例；旧设备／输入不能随备份恢复复活。保留 09 的“未证明退出后回收四槽”边界，不擅自清 Authority 槽；Client 自有镜像的退休按已核定所有权处理，而非继承一个永久泄漏假设。

### I27：其他角色专用状态

落实 C02—C05／C11—C17 的专用库存／显示结果：Tainted Isaac 库存次序、Cain crafting 内容、Blue Baby poop spell、Eve clot、Samson 狂暴、Lilith 胎儿／鞭、Apollyon 蝗虫、Bethany 魂／血充能与 wisps、Jacob／Dark Esau 关联等；C06—C10的身体转换分别由I23—I26负责，共用本项状态／资源适配。

逐族列本体值、表现资源和非 Player owner；对未公开且确被 draw 使用的缓存补来源。Dark Esau／clot／wisp 不因与玩家关联而重复创建成 Player。完整组合归 29，但这些族的基本应用及必要依赖必须在本 tx 完成。

### I28：当前生命、形态变化和成员退休

支持死亡动画中仍存在、合作幽灵、复活后当前结果、同 Key shape 改变、角色交换及最终删除。关系来源暂缺／未完整采样只撤销相关可用性，不按缺席推销毁；已退休 Key 不从旧 T 复活。

replace 时保留逻辑身份而换本地实例代次，撤旧资源／引用／缓存再重建；remove 不调用 Die 或战斗奖励路径。完整复活／团灭的 Authority 触发流程仍归 28，这里必须真实承载已发生结果。

### I29：原生 Player 绘制和必要维护

核定并接入 Player 的主体／Body／Head／手／服装／武器、阴影、光照、post、透明、飞行、闪烁及水面／镜像／minecart／Grid 柱遮挡。复用 18／19 的 FBO、depth、跨实体 shadow buffer 和外层 CPU 队列合同，不能直接孤立调用需要 caller 初始化的虚函数。

把 Player 需要的真实姿态／缓存接入唯一分层分派；不运行 Pre／Update／动画事件补算，不依赖本地按键决定脸朝向或蓄力。每条允许 draw 的读集合和临时写恢复明确到 owner，性能缓存按实例／资源／shape 失效。

### I30：控制关系与输入接线

将正式 player 关系接入现有 Client session／输入代次，仍是一组一次 Submit／去重／Resolve。远端 Client 发送意图，不消费本地 Player 玩法；Authority 上已有 09 的多身体消费及中性／重绑例外保持原义。

participant 连接代次与目标身体 shape 不是同一概念；纯换图不能无故重建连接，实际撤绑则清旧边沿。pause／loading／stop／退出及迟到旧输入必须正确失效；不因某身体准备中让 Authority 等全员。

### I31：HUD 和角色展示数据接口

定义按 participant／body／slot 访问的已提交只读视图，含所有心量、资源、主动／口袋槽及充能、History、角色专用条和共享关系。数据来自同一 A，未提交／失效目标不能混进 HUD。

如现有原生 HUD 构造或 Player draw 要求最小 PlayerHUD 绑定，由 R14 核定结构并在本 tx 实现必要安全依赖；完整 HUD 排版／动态表现仍交 27。接口不能以界面 getter 触发属性重算或生成状态。

### I32：玩法、随机、Lua 和持久隔离

对每个新构造／setter／draw／retire 建用途规则：必要结构维护、最终量安装、明确阻止的玩法／随机／音画／Lua 及持久 writer。进入受保护 Client 准备之前生效，持续到安全退休与最后相关 writer；沿唯一 Hook 清单注册并常驻。

Host／Off 原样透传，观测 wrapper original exactly-once；明确主动跳过的 gameplay 分支不冒称调用了 original。不能靠静音、readonly、输入归零或最后存档未变代替隔离；不进入原版 Online／Isaac Sync 做测试。

### I33：失败、在途寿命和恢复

为 raw allocation、ctor-entered、roster-owned、Init、资源／costume／Weapon、引用、发布和 draw 在途逐前缀定义清理。PlayerManager、Room、sub／twin／backup、Weapon、EntityPlus 和 Lua userdata 谁释放谁必须唯一，不能把 09 的正常递归 Remove 当成 Client 半构造的通用清理。

复用 Scene retained／borrow 服务，停止新借用后清依赖、排空在途并按已知 owner 退休；未知 SEH／allocator／context 状态保持隔离和首因，不盲目重试或卸载。安全退休后同 tx 继续完整目标恢复；自有 K 的 NEVER 策略不改。

### I34：Scene 接线和完整范围提交

将 Player 计划、结构见证、非 Player／Grid 构造、后置引用和检查接入 `ClientScene::Pump` 同 attempt，取消／重建／Retire 使用同一收口。不要新增独立 Player Scene 或独立提升 A 的提交入口。

增加准确的 Player 本体／关系应用结果；完整 Entity 域完成须包含当前目标内全部必需 Player 与非 Player 成员、值／关系／资源，player 域完成还须覆盖其规范关系／History 责任。只有满足对应版本和字段处置才置域位；旧代表、部分数据和独立事件都不能冒报完整 11 域，也不在此发送网络 Applied ACK。

### I35：预算和缓存

分别计 participant 数、控制组关系节点、roster／非 roster／备份身体、每身体装备／效果／History／costume／Weapon、资源层、引用边、计划步骤及总字节／节点。至少容纳四组八活动身体及所需非活动对象；现有单组 64 和全局四备份槽按真实语义使用，超限显式失败而非截断。

沿用当前 64M 节点上限口径，不擅自改小为隐藏缺口；字节、关系、容器和在途 owner 各自仍需有界。稳定 T 复用解析／拓扑／资源，生命周期变化立即撤销。记录实际采集／解析／应用／draw 成本及进程内存，不将顺序导入耗时或投影压缩率说成 10 Mbps 实时能力。

### I36：状态和错误可观测性

在 `cs status`／现有 ops 发布纯值摘要：目标／已提交 revision、参与者／身体／备份计数、role、当前阶段、预算、缺依赖 Key／字段、执行前缀、退休首因和恢复状态。生产默认关闭逐 tick／query 明细和录制，必要生命周期／线程检查保留。

E2E inspect 从真实原生对象只读回取，不只打印目标缓存；故障注入只能在 E2E 构建并标明真实前缀。Rust 不持有／解引用 Player 地址；诊断失败不改变同步结果或权威输入。

### I37：工程和发布边界

接入 `mod/CMakeLists.txt`、生成目录、统一 Hook registry、正式 observation 和产品能力；SDK 原生边界与模型语言标准沿现有工程分隔。核心功能／函数／结构写中文注释，说明 owner、阶段和失效条件。

产品与 `ISAACCS_E2E` 编译输出分离，测试桥／fixture／Rust harness／故障开关不进入发布包。纯构建如单独安排不能夹带测试；全部运行检查仍只在 I39。新模块不依赖录制文件或测试客户端导出路径。

### I38：统一 E2E 编排实现

复用 `scripts/run-tx-04-19.ps1`、`tools/harness/scenarios/tx-04-19.json` 和已有 Source／冷 Client 导入流程，新增建议的 `scripts/run-tx-04-20.ps1`／`tools/harness/scenarios/tx-04-20.json`。以第 6 节已列失败模型编写场景和判据，随完整生产接口接线；不在写完代码后补写单元测试，不提前运行编排。若确需隔离研究某个系统，先补列其全部失败方式再写代码，运行仍归全目标整体验收。

Source 的玩法动作仅在测试夹具执行；正式采集由产品 observer 产生。冷 Client 必须在独立启动、没有同一源世界遗留对象的条件下消费正式 T；对每个场景记录来源、后态、视觉和隔离判据。合法／非法目标、取消和各前缀故障走同一个 Scene，不另做假对象 harness 代替原生应用。

### I39：统一整体验收

完整实现后，从一个入口执行构建、Source、冷 Client、E01—E30、必要 18／19 回归与画面审阅。每次实机启动前说明用户操作（如跳过开场到标题／选档后停止操作），需要手动配合时等用户明确准备好再启动；多次启动分别提示，不从本文取得额外操作授权。

失败先定位、修复并在同 tx 内续测，覆盖修复影响和未完成项；可复用确实不受影响的结果并说明依据。E2E 默认不留 artifact，正常临时材料用后清理；必要中断才保留恢复所需材料并说明位置。不得以 Source 操作返回成功、DTO 相等、包数或输入 ACK 代替原生后态／画面。

### I40：结果和交接

按实际情况更新目录中的来源／实现／原生支持／运行覆盖、结果说明、`docs/README.md` 和路线图；保留全部实际失败、修复和未决项，不倒改 09／13／19 历史事实。不为每个 I 单独归档 artifact。

交接 21／22 的目标／版本／完整范围谓词／恢复原因，23—26 的身体生命周期，27 的 HUD／表现视图，28／29 的角色／武器矩阵与尚未覆盖组合，32 的实际成本。若原版必要状态族还缺失，明确 tx-04-20 未全部完成，不能将其自动改成后续可选扩展。

## 6. 编码前明确的失败模型

这些是设计输入，后续统一 E2E 验证，不在实现前运行测试。

| 失败类别 | 必须避免的行为／要求的处理 |
| --- | --- |
| 两域不同窗口、body 缺失、旧关系 | 不拼接或用旧字段补全；写前拒绝／等待具名依赖，保留当前 A 的真实可用性 |
| 四人等同四身体、roster 等同全部身体 | 不截断、不丢 sub／backup；冲突定位到组与 Key |
| 相同 ControllerIndex／地址／seed | 不能据此归属、共享、继承旧实例或退休资格 |
| 非活动身体从未满足字段可读依据 | 补明确来源资格；不能套用当前 PlayerUpdate 见证／把缺值填零 |
| 自动生成 twin／Familiar／Weapon | 与目标具名对应或按核定路径隔离，不能双生成 |
| 库存已写但 stats／costume／Weapon 仍旧 | 不提交；补完整依赖和最终安装顺序 |
| 缺资源、空列表、未知字段／角色／武器 | 明确缺失与合法空；不忽略或回退普通 Isaac／Tears |
| 19 依赖先读 Player、Player 又先等全部 Entity | 区分 structure-ready 与 draw-ready，明确必要前置与后置引用 |
| 死亡／切换／备份不在 roster | 不立即 remove、不恢复旧输入，不把不可控等同不存在 |
| draw 依赖本地时钟／RNG／临时动画状态 | 消费 Authority 表现输入或核定纯表现合同；不补 Update |
| draw 参数临时恢复过早、borrow 跨实例 | 保留至真实消费结束；失效阻止使用，不能靠地址没变复用 |
| 部分 Init／资源／引用／draw 失败 | 首因和前缀真实，写后停显／安全退休再恢复；未知保留 owner |
| 递归 twin／backup cleanup＋逐个移除 | 唯一清理 owner、实时复读归属，避免重复释放和遗漏备份 |
| PrepareTarget 成功就报 Applied | 只有实际全部原生检查后由 Scene 提升 A，独立事件另算 |
| 构造／换装／退出触发进度 writer | 全窗口隔离，不以最后文件未变代替内存／调用链归属 |
| 录制关闭、队列背压、低预算／超限 | 正式状态不依赖日志；有界失败／继续推进，不阻塞 Authority 或静默删身体 |

## 7. 全目标完成后的统一 E2E 矩阵

E 编号是同一入口的覆盖项，不是分项验收。每个角色／武器族需要实际适用后态，测试场景数量由真实需要确定；不得用一个静止样本冒报全部枚举／全部组合通过。

| ID | 场景／输入 | 必须判定的结果 |
| --- | --- | --- |
| E01 | 冷 Client、连续完整 T、重复／旧／冲突、恢复基线 | 正确原生提交；重复无副作用；范围准确，不 ACK 半套世界 |
| E02 | 一至四参与者、各身体不同位置／装备 | participant／group／身体唯一映射、计数分离，无首玩家回退 |
| E03 | 本地 PlayerType 0—40，按 C 表安排实际形态 | 每一类型的基本最终状态／外观正确；内部类型和未知值有明确分类 |
| E04 | 四组 Jacob／Esau，八活动身体 | 不截断；所有身体有真实 owner、状态、资源及独立后态 |
| E05 | 普通与双身体混合，一组退出／缺依赖 | 各组不串人、其他组保持正确；冲突时不乱绑 |
| E06 | Forgotten 16↔17、Tainted 35／40、举持／落地 | roster／sub／活动资格、心量、武器和关系正确 |
| E07 | Tainted Lazarus 29／38、Flip、Birthright hologram | 装备／资源／可见性正确；不本地 Flip 或重复退休 |
| E08 | Esau Jr 真切换／恢复、四槽、持备份组退出 | 槽与组不混淆、非活动来源有效、旧输入不复活，容量限制明确 |
| E09 | 所有心量类型、资源、最终 stats、幽灵相关值 | 各角色合法语义与 Source 一致，不本地伤害／治疗／属性重算 |
| E10 | 库存增加／删除／重排、金／熔炼饰品、固有效果 | 容器完整、清空正确、不同身体独立；无二次掉落／Familiar 生成 |
| E11 | 全主动／口袋／卡药槽、换槽、满／空／分数充能 | 槽身份及全部字段准确，不执行使用／放电 |
| E12 | 临时效果重复／清除、变身、History／共享状态 | 持有／临时／历史不混用，空后态清旧值、共享依据可解释 |
| E13 | 主／Body／头手／costume 多资源，换图、层／overlay／shader | 真正完整姿态和画面对照正确，旧资源无残影／错误合并 |
| E14 | W01—W15 全武器类型、不同阶段、切换和空槽 | owner／charge／姿态／动态结构正确，不在 Client 开火 |
| E15 | Laser／Knife／骨棒／剑／鞭等与 Player 循环依赖 | 结构／最终就绪分离，不死等；引用正确且不会双创建／双释放 |
| E16 | C 表专用状态族，含 crafting／poop／clot／wisp／蝗虫／Dark Esau | 各族最终数据、资源及非 Player 依赖实际存在，不以普通 Isaac 代测 |
| E17 | 同组多身体不同原生消费、换形／撤绑／重绑 | Authority 统一输入未退化；Client 只发送意图，不产生第二份模拟 |
| E18 | Effect136、其他持物、minecart、Familiar 复合绘制 | 精确具名 owner／shape／资源，父级代画无重复，投掷结果不重演 |
| E19 | 四人／多身体 HUD 数据视图与提交变更 | 所有槽／资源／History 对应正确身体，只读同一 A；不冒报完整 HUD27 |
| E20 | 阴影／光照／透明／闪烁／飞行、水／镜像／柱遮挡 | Source／Client 实际画面对照；不跳原生 caller 前置或靠 Update 修画面 |
| E21 | 死亡仍存在、ghost、复活当前结果、同 Key 换形、同址新 Key | 存在性、实例换代、关系与资源失效正确，无旧对象复活 |
| E22 | 跨 scope、缺字段、错误角色／引用／位宽、版本不兼容 | 写前具体拒绝；Partial 不删身体，不声明完整域 |
| E23 | 分配／ctor／roster／Init／资源／Weapon／关系／draw 故障与取消 | 每个采用前缀有实际覆盖；写前保 A、写后停显、唯一退休、完整恢复 |
| E24 | 普通／odd／FSM／暂停／loading／Host／Client／Off | Client 无 AI／玩法／Lua／随机重复裁决，Host／Off 原义及重绑语义保持 |
| E25 | 准备、应用、展示中 stop／断连／菜单交还／正常退出 | owner／借用／备份收口、访客持久全窗口隔离，未知失败不伪报 Off |
| E26 | 18、19 完整已验收清单回归 | 对应18的49 Source／49冷Client／12检查与19的65 Source／65冷Client、20非法目标／2版本拒绝／16恢复及正常收尾；新能力取代旧限制时更新预期并保留原场景语义，不能删除覆盖 |
| E27 | 同源完整观测／投影、所有新增参数消费者和图闭合 | 投影恢复的字段完整、失败状态仍可见、别名正确；计量口径一致 |
| E28 | 八身体＋备份＋复杂 costume／Weapon，多目标稳定段和上限／超限 | 无静默截断；真实成本、缓存失效、内存／owner 有界，预算不足不拖停 Authority |
| E29 | 正式／E2E 构建、无录制／录制故障、部署清理 | 正式应用不依赖 fixture／日志；发布无测试玩法／故障桥，临时材料清理 |
| E30 | 同一完整目标的 Room＋Grid＋非 Player＋Player 联合变化 | 唯一 attempt 提交全部声明范围；子域失败不局部提升 A，不伪报网络闭环 |

画面比较须覆盖身体组合、装备／武器和特殊状态，不要求跨进程不同独立时钟逐像素相等；任何差异需解释到 Source scope、独立表现或真实缺陷，不能把 Player 自身必要差异继续推给 27。源端制造场景必须符合实际操作授权；依赖新授权时先完成可审阅方案并只询问缺少的部分。

## 8. 推进顺序与当前结论

先做 I01— I03 的字段／来源梳理及 R01—R03，再由 I04 的各研究支撑 I05—I10；I11—I21 实现状态／资源／武器，各 C 族 I22—I27 接入；I28—I34 完成变化、绘制、隔离、退休和唯一提交。I32／I33 从首个原生调用开始同步设计，不能到最后才查副作用和清理；I35—I38 随完整接口接线，I39 在整个目标实现后统一执行，I40 如实收束。

当前没有需要用户猜测的原生实现问题；R 表中的技术未知由静态工作解决。仍未知的容量／清理／私有 draw 合同不能预记完成。如果研究后出现必须改变产品范围的选择，或实机需要新的具体操作授权，再提供可比较的方案向用户询问。本轮没有开始实施 I01— I40，也没有从历史授权推导新的游戏操作授权。
