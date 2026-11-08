> **原生二进制状态同步重构第 4 步：Tear／Pickup／Laser（2026-11-08）**：[实现说明](design/binary-state/implementation-4-tear-pickup-laser.md)、[必要逆向与主审](design/binary-state/tear-pickup-laser-evidence.md)和[生成布局表](design/binary-state/layouts/README.md)。三类布局、派生 Plus、独立辅助 Sprite、全部已核引用槽、Pickup 循环道具与 Laser 四份几何容器已接入生成链，布局版本 20。未知字节与条件计时缺口照实保留；普通 Release 纯编译／链接通过，没有测试或启动游戏，第 15 步统一验收。

> **原生二进制状态同步重构第 4 步：Familiar／Knife／Bomb（2026-11-08）**：[实现说明](design/binary-state/implementation-4-familiar-knife-bomb.md)、[必要逆向与主审](design/binary-state/familiar-knife-bomb-evidence.md)和[生成布局表](design/binary-state/layouts/README.md)。三类布局、派生 Plus、辅助 Sprite、Familiar 两种专用 payload 与公开成员回映射已接入生成链，布局版本 16。未知字节与未闭合的计时条件明确保留，不能宣称类型完整；Release 纯编译／链接通过，没有测试或启动游戏，第 15 步统一验收。

> **原生二进制状态同步重构第 3 步（2026-11-08）**：[实现说明](design/binary-state/implementation-3.md)、[Entity 公共证据](design/binary-state/entity-common-evidence.md)与[生成布局](design/binary-state/layouts/Entity.md)。Entity `[0,0x410)` 全部字节已有归属，生成 E0—E9 共 77 个字段／投影和 23 个直接复制区间，布局版本 12；另有 EntityPlus 只读来源、条件计时锚点和 Client 数字字段查询。Entity 仍有 335B 未知，不宣称类型完整。基础库和完整 DLL 的 Release 纯编译／链接通过；没有测试、部署或启动游戏，第 15 步统一整体验收。

> **原生二进制状态同步重构第 0—2 步（2026-11-08）**：[实现说明](design/binary-state/implementation-0-2.md)、[基础类型证据](design/binary-state/base-types-evidence.md)与[生成布局表](design/binary-state/layouts/README.md)。唯一 JSON 布局源、四份生成表、14 种基础类型及有界容器／引用／资源共用代码已落地，基础库和完整 DLL 的 Release 纯编译／链接通过。必要逆向由 GPT-6 Luna（max）执行，主智能体复读并纠正了字段类型、选择器、颜色空间及 RTTI 偏移等结论；未知和跨域未决照实保留。没有测试、部署或启动游戏，第 15 步统一整体验收。

> **原生二进制状态同步重构（2026-11-08，仅计划文档）**：[总计划与 0—15 步设计](design/binary-state/README.md)。按用户决定，Host 改为 C++ 原生采集，状态按类型的二进制布局连续排布，以 memcpy 复制、memcmp 判断变化；完整性按字节归属判定，Baseline 也统一使用二进制格式。步骤直接用数字编号，第 15 步是唯一的统一整体验收（[规范第 17 节](refactor/project-conventions.md)）。tx-04-21 中与之冲突的采集和同步设计以新目录为准，事实与失败记录保留。本轮没有修改代码，也没有构建、测试或启动游戏。

> **tx-04-21 当前暂停进度（2026-11-07）**：[双人E2E进度、修复与续测位置](design/tx-04-21/tx-04-21-e2e-progress.md)。用户恢复后又要求“先不跑02测试”，现已正常收尾。第65轮已完成产品／E2E／harness构建部署，包含两项分层及[Host完整采集性能修复](design/tx-04-21/tx-04-21-capture-performance-plan.md)；新版本E01.valid-menu第8次通过，两端实际三池512／128／128 MiB、正常退出0且无访客进度差异。E02第47次仅启动Host、尚无新run即中断；E01仍共九项通过，E02未通过，E03—E26／P4未执行。未取得新采集性能数据，不宣称50帧或30／60 Hz达标。两端部署已还原，连接级设置保留；以下条目保留各自历史时点事实。

> **tx-04-21 I36—I38（2026-11-05）**：[全窗口隔离、全链静态主审、逐项18—20回归及P2→P4唯一入口](design/tx-04-21/tx-04-21-isolation-orchestration.md)已接入。真实网络Client隔离起点、纯值调用见证、交互桌面远端节点、精确画面中性段与同tx恢复／环境还原已编写；无需新增逆向。未构建／测试／部署／启动游戏；I39全P2→全P4和I40仍待执行，未声明实时指标或网络验收通过。

> **tx-04-21 I31—I35（2026-11-05）**：[双配置构建部署、完整E2E子用例、真实UDP损伤、同版本原生对照与性能归约](design/tx-04-21/tx-04-21-e2e-components.md)已编写并完成主智能体静态复读。本轮复用既有已核原生检查，无新增逆向需求；未构建／测试／部署／启动游戏。I36—I38继续集成，I39仍须先P2全E01—E26及子用例通过后再P4全套，未声明实时指标或网络验收通过。

> **tx-04-21 I26—I30（2026-11-05）**：[同连接恢复、慢端收敛、完整停止、聚合统计与冻结配置](design/tx-04-21/tx-04-21-recovery-operations.md)已实现并完成静态主审。安全退休后请求新Baseline，连接级原生自动重建最多一次；终值100ms补发、500ms陈旧／2s恢复、控制候选独立保留。没有新增游戏原生入口，复用已核退休合同，本轮无需新增逆向；未构建／测试／启动游戏。I31—I40及I39的P2全E01—E26及子用例→P4全套待继续；下方旧记录保留当时事实。

> **tx-04-21 I21—I25（2026-11-05）**：[正式路由准备、网络冷构建、同Scene动态／结构安装和精确原生Applied ACK](design/tx-04-21/tx-04-21-installation.md)已实现，必要逆向由GPT-6 Luna max研究并经主智能体复读。Release纯编译／链接通过；未运行测试或启动游戏。29/38单侧身体快源因跨身体消费而报告gap，沿合格ordinary组合；频率与性能未实测。I26—I40及I39的P2全E01—E26及子用例→P4全套待继续；以下旧记录保留原时点事实。

> **tx-04-21 I16—I20（2026-11-05）**：[可靠操作、三Peer预算、高频输入时序、Host发布与有界线程交接](design/tx-04-21/tx-04-21-publication.md)已实现并经主智能体静态复读。本轮无需新增游戏逆向；未构建／测试／启动游戏。I21—I40及I39统一P2全套→P4全套待继续，未声明原生网络应用闭环或实时性能通过。

> **tx-04-21 I01—I15（2026-11-05）**：[紧凑编码、zstd、路由预算与选择性补片](design/tx-04-21/tx-04-21-transport.md)已实现并完成静态主审。I01—I10既有基础／采集保留；本轮无新增游戏逆向，未构建／测试／启动游戏。I16以后及I39统一P2全套→P4全套待继续，未声明实时性能或世界网络应用闭环通过。

> **tx-04-21 I06—I10（2026-11-05）**：[同窗目标、动态来源、逐身体见证与身份](design/tx-04-21/tx-04-21-capture.md)已实现并经静态主审。包含共享 Baseline/范围版本、编码前分层、ordinary/odd 逐身体来源、九个资源观察入口和统一 Registry/退休事实；GPT-6 Luna max 的必要逆向已由主智能体复读。未构建/测试/启动游戏；I11以后及I39统一P2全套→P4全套仍待推进，不宣称频率、性能或网络世界闭环已通过。

> **tx-04-21 I01—I05（2026-11-05）**：[基础实现与边界](design/tx-04-21/tx-04-21-foundation.md)、[逐成员实时目录](design/tx-04-21/stream-catalog.json)、[Luna max必要逆向及主审](design/tx-04-21/tx-04-21-native-contracts.md)。已接分层不可变范围／精确依赖、v2严格schema与Live小消息、正式consumer3及Host生命周期。未构建、测试或启动游戏；I06—I40与I39两人全套后四人整体验收待推进，未声明实时频率／带宽或网络世界应用完成。下方初版规划记录保持原事实。

> **tx-04-21详细规划（2026-11-05，仅文档）**：[I01—I40实施计划](design/tx-04-21/tx-04-21-implementation-plan.md)、[频率／10 Mbps预算／分层完整状态与21—22边界](design/tx-04-21/tx-04-21-network-contract.md)、[R01—R09静态与必要逆向任务](design/tx-04-21/tx-04-21-native-research-plan.md)、[E01—E26统一E2E](design/tx-04-21/tx-04-21-e2e-plan.md)。按用户澄清，先在两人中跑通全部E01—E26及子用例／指标／回归，全部通过后再转四人整套重跑和扩展；不能只跑“两人适用项”。目标Player调度60 Hz、普通动态30 Hz、roster20 Hz，Host三端总上行10 Mbps、模组持续预算7.5 Mbps。目标不是实测值；未写运行代码、构建或测试。

> **tx-04-20 I38—I40完成（2026-11-05）**：[统一编排与实际验收结果](design/tx-04-20/tx-04-20-e2e.md)覆盖核心114逻辑场景、18全49及5完整Player环境、19全65与全部负向／故障／退出；双配置构建和产品边界通过。[必要逆向与主审](design/tx-04-20/player-i39-source.md)、[能力目录与后续交接](design/tx-04-20/tx-04-20-player-application.md)保留真实失败和有限范围。21／22网络闭环、27完整HUD／声明Mod、28／29流程组合及32性能继续按路线图推进。下方保留此前阶段事实。

> **tx-04-20 I36—I37（2026-11-04）**：[纯值状态／首因、真实原生 inspect、E2E 故障边界及构建发布接线](design/tx-04-20/tx-04-20-observability-build.md)已实现。产品与 E2E 的 DLL／工具／目录分开输出，Player 应用目录纳入发布清单。未构建、测试或启动游戏，按要求直接提交；I38—I40 及 I39 统一验收待推进。下方保留此前阶段事实。

> **tx-04-20 I31—I35（2026-11-04）**：[同A只读展示、动态HUD、隔离退休、Scene提交与预算及必要逆向主审](design/tx-04-20/tx-04-20-commit-isolation-budget.md)。已接入完整只读接口、原生HUD/control pair、Player用途随机拒绝、全窗口SFX保护、提前撤边／ANM异常owner登记、安全退休后完整T恢复一次及准确Player／Entity域位。按用户要求直接提交，未构建、测试或启动游戏；I36—I40及I39统一验收待继续。下方保留此前阶段事实。

> **tx-04-20 I26—I30（2026-11-04）**：[四槽／专用角色状态／生命周期／原生绘制和控制关系实现](design/tx-04-20/tx-04-20-special-draw-control.md)、[Luna max必要逆向及主审纠错](design/tx-04-20/player-i26-i30-native.md)。同窗口有限槽、Samson／IBS最终充能、残影／可选动画序列、真实Image及ANM整帧强借用、Scene寿命与已提交控制组视图已接入。未构建、测试或启动游戏，按会话要求直接提交；I31—I40及I39统一验收继续，完整native_apply仍false。

> **tx-04-20 I21—I25（2026-11-04）**：[通用持物、原版角色注册及双身体／sub／Flip结果安装](design/tx-04-20/tx-04-20-role-installation.md)、[Luna max必要逆向及主审纠错](design/tx-04-20/player-i21-i25-native.md)。增加同窗口role来源、41类型具名责任、全图关系校验及全体Link后全体Check；直接复读IsSub／main／other／hologram、Swap和通用held Render。未构建、测试或启动游戏；I26—I40与I39统一E2E待推进，native_apply仍false。

> **tx-04-20 I11—I15（2026-11-04）**：[公共／生命／资源／完整库存／主动口袋槽最终量及必要逆向主审](design/tx-04-20/tx-04-20-state-installation.md)。正式同窗口补源、不可变解码、原生安装／回读和owner接线已接入；一个GPT-6 Luna max研究，主智能体复读并纠正PartialCharge／effective命名，补明Active+4为item_pool。未构建、测试或启动游戏；I16—I40及I39统一E2E待推进，完整native_apply仍false。

> **tx-04-20 I05—I10（2026-11-04）**：[投影、A→T身体／子owner计划、结构owner／集合／关系与必要逆向主审](design/tx-04-20/tx-04-20-structure.md)。正式合同升级player-target/source-v2，恢复装备与History按参查询；Key／实例／用途借用、base结构前缀、roster与Room发布、非roster／备份及跨域后置关系已接线。一个GPT-6 Luna max子智能体研究，主智能体独立复读；完整最终态／资源／角色adapter归I11以后，缺席写前拒绝，native_apply仍false。按用户要求直接提交；未构建、测试或启动游戏，I11—I40及I39统一E2E待推进。

> **tx-04-20 I01—I04（2026-11-04，阶段记录）**：[目录、PlayerTarget、正式只读来源及主审](design/tx-04-20/tx-04-20-foundation.md)、[逐签名应用目录](design/tx-04-20/player-application.json)、[R01—R16静态采用／否决合同](design/tx-04-20/player-native-static.md)。已接入两域同窗口／owner关联、roster／四备份槽／控制组、完整有限查询与12份新增内嵌ANM来源；指定GPT-6 Luna max研究，主智能体独立复读。当时I05—I40待推进；未构建、测试或启动游戏，完整原生Player支持尚未完成。后续见上项，下方保留初版规划事实。

> **tx-04-20 具体实现规划（2026-11-04）**：[I01—I40 全部任务、失败模型与 E01—E30 统一验收安排](design/tx-04-20/tx-04-20-implementation-plan.md)、[P01—P20 玩家状态、全部角色／身体族及15种武器](design/tx-04-20/tx-04-20-player-application.md)、[R01—R16 静态确认／必要逆向计划](design/tx-04-20/tx-04-20-native-research-plan.md)。覆盖四参与者、多身体／非活动／备份、装备／资源／武器、原生绘制、跨域依赖、唯一提交和失败退休；明确正式来源／投影与19单Isaac支撑的差额。本轮仅编写文档并统一静态审阅，全部实施和研究任务待执行，没有新增逆向结果、运行代码修改、构建或测试。

> **tx-04-19 I34—I35（2026-11-04，完成）**：[统一验收、实际失败／修复、成本与后续交接](design/tx-04-19/tx-04-19-acceptance.md)。18 Source49／49、冷Client49／49及12／12检查；19 Source65／65、冷Client65／65、20非法目标／2版本拒绝、16故障恢复、暂停／stop／退出均通过，主智能体已审阅114对画面。能力与应用目录记录准确非Player范围；完整Player20、网络21／22、独立音画27及整局内容仍按具名交接推进。下方保留前序阶段事实。

> **tx-04-19 I26—I30（2026-11-03）**：[独立结果、隔离、退休与唯一提交](design/tx-04-19/tx-04-19-scene-commit-retirement.md)、[Luna max必要静态研究／主审](design/tx-04-19/entity-i26-i30-native.md)。Entity专用独立scope、全窗口SFX隔离、逐前缀逐类退休、Scene唯一提交及准确非Player／Player支撑范围、真实逐池预算和稳定缓存已接入。按本轮会话要求未构建／测试而直接提交；I31—I35与I34统一E2E待推进，不置完整Entity域位或网络ACK。下方记录保留各阶段事实。

> **tx-04-19 I15—I19（2026-11-03）**：[Laser／Knife／Pickup／Familiar／Effect类别应用](design/tx-04-19/tx-04-19-laser-effect-application.md)、[Luna max静态研究与主审](design/tx-04-19/entity-i15-i19-native.md)。五类最终量、完整激光几何／cycle、具名引用／辅助资源及五种typed ExtraData已接入；Player复合draw、Effect136交接、完整分层绘制和唯一Scene提交继续按I24／I25／I29推进。按会话要求直接提交，没有构建或运行测试；I34待统一验收。

2026-11-03 tx-04-19：[I20—I25剩余类别／形态／Effect136／分层绘制实现](design/tx-04-19/tx-04-19-slot-text-draw-application.md)，[指定Luna研究及主审](design/tx-04-19/entity-i20-i25-native.md)。未构建或运行测试，整体验收归I34。

> **tx-04-19 I10—I14（2026-11-03，阶段记录）**：[资源与四类应用实现／明确边界](design/tx-04-19/tx-04-19-class-application.md)、[Luna max静态研究与主审](design/tx-04-19/entity-i10-i14-native.md)。主／辅助资源完整姿态、NPC／Delirium、Tear、Projectile、Bomb字段和具名关系已接入；特殊NPC私有draw目标写前明确拒绝并交I25，不冒称全类别画面已完成。Win32 Release纯编译链接成功，未运行分项测试、游戏或E2E；后续进度见上项。

> **tx-04-19 I05—I09（2026-11-03）**：[公共应用实现与必要逆向主审](design/tx-04-19/tx-04-19-common-application.md)。稳定实例／Player接口、A→T阶段计划、构造／发布、公共最终值与真实后态、分类型依赖已接线；一个GPT-6 Luna max子智能体逆向，主智能体独立复读并纠正SplatColor／Color别名及DepthOffset映射。Win32 Release纯构建成功，没有测试或游戏启动；类别adapter、唯一Scene提交与I34统一E2E仍归I10—I35。

> **tx-04-19 I04（2026-11-03，阶段记录）**：R01—R13的[静态采用合同](design/tx-04-19/entity-application-contracts.md)、[J273必要逆向与主审](design/tx-04-19/entity-native-static.md)、[目录研究索引](design/tx-04-19/entity-native-contracts.json)已写回功能目录。一个GPT-6 Luna max子智能体执行必要逆向，主智能体独立复读并纠错；明确池大小／工厂选择、字段宽度、真实辅助资源、draw随机及跨对象退休副作用，不放行未知分支。此阶段I05—I35与统一E2E待实施，无构建、测试或游戏启动；后续进度见上项。

> **tx-04-19 I01—I03（2026-11-03）**：[目录／纯值目标／正式来源实现](design/tx-04-19/tx-04-19-foundation.md)、[全成员应用目录](design/tx-04-19/entity-application.json)、[Text／Laser 来源研究与主审](design/tx-04-19/entity-source-research.md)。已接入 `entity-target-v1`／`entity-source-v1`、分类与 Player 边界、完整资源姿态和必要原生只读补充；本段为当轮实现记录，新发现的来源／owner缺口按I04回补。逆向由 GPT-6 Luna Max 完成，主智能体独立审阅并纠正 Text writer 推断；没有启动游戏或执行提前分项测试。

> **tx-04-19 初版具体实现计划（2026-11-03，历史文档阶段）**：[35项完整任务、失败模型与26项统一E2E安排](design/tx-04-19/tx-04-19-implementation-plan.md)、[13项类别适配与Slot 18种变体](design/tx-04-19/tx-04-19-entity-application.md)、[13项静态确认／必要逆向](design/tx-04-19/tx-04-19-native-research-plan.md)。初版仅规划，不代表当时已改代码、逆向或运行验收；后续 I01—I03 进度见上项。

> **复制模型冗余发送优化（2026-11-03，19前置实现已完成，运行验收待19）**：[P1—P6实现与消费者复核、模型v2及统一验收安排](design/replica-redundancy-reduction.md#10-2026-11-03-前置优化实现)。完整观测不变，发送投影省略静态动画表／无消费者查询、精简占位和无引用图编号；19的Entity主体尚未开始。10 Mbps预算及21／22／32后续网络优化仍见同文第8—9节，原样本统计不作为本次收益。

> **tx-04-18 I26—I30（2026-11-03，已完成）**：预算、资源复用、产品/测试构建边界及完整Source/冷Client编排已接入；节点上限按用户要求统一64M。实际覆盖、连续失败/修复、背景构建轨迹、地图深度和后续接口见[统一验收记录](design/tx-04-18/tx-04-18-acceptance.md)。最终Source 49/49、冷Client 49/49及12/12检查通过，代表画面已主审；下方历史阶段状态保留原时点。

> **tx-04-18 I01—I18（2026-11-02）**：[基础实现](design/tx-04-18/tx-04-18-foundation.md)、[Room应用](design/tx-04-18/tx-04-18-room-application.md)及[Grid/地图owner](design/tx-04-18/tx-04-18-grid-owner.md)已接入领域adapter。I13—I18覆盖真实地图/Camera、Grid单owner、27类构造/字段/资源及Door别名；GPT-6 Luna/max逆向已由主智能体复读。I19—I25仍负责跨域引用、缓存、分层draw及整场景提交，I29统一实机未执行；本轮不运行游戏或分项测试。

> **tx-04-18 I09—I25续作（2026-11-02）**：引用、最终path/碰撞与邻接结果、Grid原生分层绘制、独立记录、失败退休和唯一Scene提交已接线，GPT-6 Luna/max研究经主智能体直接复读审阅，完整纯构建成功。见[场景集成与实际边界](design/tx-04-18/tx-04-18-scene-integration.md)。I26—I30待推进，I29统一实机验收未执行；下方此前待集成状态保留原阶段语境。


> **tx-04-14 已完成（2026-10-31）**：[整局、随机选择、HUD/独立表现、Mod及真实StageAPI实现与统一验收](design/observation-expansion/tx-04-14-implementation.md)。6/6 PASS、21/21场景、84个指定四域scope；[记录/适配合同](design/observation-expansion/run-mod-schema.md)。下方早期计划状态保留原时点。

> **tx-04-13已完成（2026-10-31）**：[探索、离房保存与玩家跨房观测](design/observation-expansion/tx-04-13-implementation.md)已通过统一E2E 5/5，31/31场景、124个指定正式scope；不受影响的已完成项目复用，失败和边界如实保留，临时材料已清理。

> **tx-04-11 已完成（2026-10-30）**：[完整Grid实现与统一E2E结果](design/observation-expansion/tx-04-11-implementation.md)。27种非空类型、17个派生接口及保存／资源／变化／生命周期观测已交付；最终5/5 PASS，25条完整Grid后态，Grid Partial为0，临时材料已清理。下方历史计划保留原时点。

> **tx-04-10 已完成（2026-10-30）**：[公共接口、域拆分、完整 Entity 观测与统一 E2E 结果](design/observation-expansion/tx-04-10-implementation.md)。最终 5/5 PASS，14 条完整 Entity 后态含 4 条四组八身体后态；采集与录制独立，v1/v2 同读取投影通过，测试临时材料已清理。Grid 全量及其他域仍待 tx-04-11—14，下方历史时点不改判。

# C/S研究与状态同步文档


> **tx-04-19 I31—I33（2026-11-03）**：[投影、构建边界与统一编排](design/tx-04-19/tx-04-19-e2e-integration.md)、[Luna max来源研究／主审](design/tx-04-19/entity-i31-i33-native.md)。P1—P6新消费者复核、正式全非Player导出、产品／E2E分离、全类别Source／冷Client、故障退休及18完整回归统一入口已接线。按会话要求直接提交，未构建／测试／启动游戏；I34及I35待推进，不冒称运行验收或完整Player／Entity域提交。

> **tx-04-17 R6模型、角色与应用器归并（2026-11-01）**：[R1—R5与15／16源码接线合同](design/tx-04-17/tx-04-17-model-role-applier-integration.md)已更新。分开B／完整T／最后原生A，明确三类操作、依赖、取消／恢复、角色与场景、正式Scope及输入换代、原生结果与ACK／事件完成的责任。登记16等待Scope／小预算／固定队列和事件窗口的真实接线缺口；R1—R6指定静态文档结果均已形成，具体原生前置与实现仍未完成。本轮只改文档，无新增逆向、代码、构建或测试。

> **tx-04-17 R5部分失败、退休与菜单交还（2026-11-01）**：[失败分阶段处理、原生清理与持久保护结束范围](design/tx-04-17/tx-04-17-failure-retirement.md)已更新。核定ResetState限制、正常Room重建→Player／HUD清理及菜单state1路径，选择自有退休编排；含原版Online的菜单helper不整体采用。保护覆盖准备至安全退休和最后相关持久writer，进程退出与回菜单分开；完整可调用菜单编排、全pass取消和异常仍有UNKNOWN。必要逆向由指定GPT-6 Luna／max子智能体执行并经主审复读。R6及实现／统一E2E待推进，本轮只改设计文档，无代码、构建或测试。

> **tx-04-17 R4原生内嵌资源与在途寿命（2026-11-01）**：[资源owner、借用失效、在途持有及未决问题](design/tx-04-17/tx-04-17-native-resource-lifetime.md)已更新。分开原生实例、服装／portrait容器、ANM、Image、Lua与context寿命，核定代表对象析构、Morph资源重载及本方native Image保留合同；官方custom-render deferred context分支当前关闭，只记条件风险。正常消费与取消、LuaImage GC、异常退休仍有未决项。必要逆向由一个指定GPT-6 Luna／max子智能体执行，主智能体复读审阅。R5—R6、实现与统一E2E待推进；本轮只更新设计文档，无代码、构建或测试。

> **tx-04-17 R3维护／绘制与R1／R2回补（2026-11-01）**：[停玩法后的原生维护范围、分派合同与未决项](design/tx-04-17/tx-04-17-native-maintenance-rendering.md)已更新。明确目标提交、移除／排序、Camera／HUD缓存与draw的阶段，保留原生表现并隔离玩法／Lua／动画事件；核定sort空表／scratch／渐进限制、位置插值碰撞与Camera Update随机路径。必要逆向由一个指定GPT-6 Luna／max子智能体执行，主智能体独立复读审阅。完整逐类draw、资源／借用、异常退休仍有UNKNOWN，R4—R6与实现／统一E2E待推进。本轮只改设计文档，无代码、构建或测试。

> **tx-04-17 R2对象生命周期与R1回补（2026-11-01）**：[create／remove／change公共合同、静态事实及未决项](design/tx-04-17/tx-04-17-native-object-lifecycle.md)已更新。明确工厂池use、派生Init、成员发布、外层完成、摘表和回收的区别，以及不覆盖存活池项、key／实例换代、Morph和官方Lua／扩展维护的处理；NPC Morph的PGD bestiary写入点已反馈持久隔离。必要逆向由一个指定GPT-6 Luna／max子智能体执行，主智能体独立复读审阅并回补R1的Player／Room归属。逐类Init闭包和完整失败退休仍有UNKNOWN，R3—R6及实现／整体验收待推进。本轮只更新设计文档，无构建或测试。

> **tx-04-17 R1原生初始化范围（2026-11-01）**：[范围选型、静态事实、主审及未决项](design/tx-04-17/tx-04-17-client-scene-lifecycle.md#r1-results)已更新。先查公开接口／官方源码，必要逆向由指定GPT-6 Luna／max子智能体执行，主智能体复读指令审阅。选择已有根上的自有准备编排，复用具名原生helper；Start／Level Init／Room Init不能整体放行，Room几何前的对象清理、config指针参数及Camera非原子替换已明确。完整可调用结构初始化、Player与失败恢复仍有具体UNKNOWN；当轮R2—R6未开展，用户随后启动的R2与回补见上方新记录。没有代码实现、构建或整体验收。

> **tx-04-17 原生世界方案修订（2026-11-01）**：[主设计与补充研究顺序](design/tx-04-17/tx-04-17-workflow.md)已按用户要求改为原生Game／Room／Entity／Player承载Client，保留create／remove／change、稳定身份与Baseline／Delta语义。后续依次研究R1原生初始化、R2操作生命周期、R3维护绘制、R4资源借用、R5失败退休与持久隔离、R6模型／角色接线；优先公开接口／源码，仅补具体缺口的J273逆向。ST1—ST8历史事实与纠错保留，冲突设计已注明替代。该方案修订时只改文档、R任务未开展；用户随后启动的R1进度见上方新记录。未改变16已有双机结果或四端暂缓状态。

> **tx-04-17 ST1—ST8静态工作完成（2026-10-31）**：[专项工作流程](design/tx-04-17/tx-04-17-workflow.md)及[接口目录](design/tx-04-17/tx-04-17-public-interface-catalog.md)已汇总场景、更新、展示、资源、线程／context、Lua寿命与持久归属合同。本轮[ST8访客持久副作用与正常退出](design/tx-04-17/tx-04-17-persistence-exit-isolation.md)的必要逆向由一个GPT-6 Luna／max子智能体执行，主智能体独立回读指令／源码并审阅、纠错和补证。确认Exit(false)、Shutdown直接计数写入与保存、官方进度JSON、GameState／Mod附加保存以及Manager析构PGD版本同步的边界；Client准备至安全退休全程保持必要隔离。本轮仅文档编辑及统一人工静态审阅，无构建或测试；Client世界壳、隔离运行实现及统一E2E仍未完成，完整接线依赖15／16，持久归属反馈31。

> **按观测域拆分设计（2026-10-30）**：[详细设计与迁移边界](design/observation-expansion/domain-observation-architecture.md)确定公共接口、各域职责、scope／采样时序、跨域引用、采集与录制分离及 Entity/Grid 迁移。已同步 tx-04-10—14 观测计划；公共拆分纳入 10 的完整实现，其他域依次接入。本次只修改设计和计划，未修改运行代码、构建或测试。

> **tx-04-09 功能整体验收通过（2026-10-30）**：[四参与者与多身体实现及结果](design/observation-expansion/four-participant-implementation.md)。实际四参与者／八身体、指定形态、独立退出与停止均已覆盖；录制开启的性能开销与原生Esau Jr四槽限制如实另列，未宣称性能达标。测试临时材料已清理。下方此前“只编写文档”“当前双人”保留其当时语境。

> **整局可玩目标与完整流程（2026-10-30）**：[状态同步设计](design/state-synchronization.md)已扩展到[tx-04-09—tx-04-34完整推进流程](design/state-synchronization-roadmap.md)。09—14仍先做四人／多身体与完整观测，15—22实现真实游戏Client及状态闭环，23—29完成整局世界／原版内容，30—34交付重连、保存Continue、平滑性能、UI发布及整局联合验收。单房间是中间成果，最终必须获得完整可玩联机模组；当前只编写文档，不启动实现或测试。

> **现行执行规则修订（2026-10-30）**：见[项目规范第 2 节](refactor/project-conventions.md#2-每次推进统一整体验收e2e-连续修复与续测)。每个 tx 本身不再保留 artifact；E2E 默认也不保留，仅在中途停下且需要记录信息时保留必要材料。E2E 无特殊说明时连续完成，遇到问题修复后在同一 tx 内继续必要测试。下方历史 artifact 与“不自动重跑”等记录保留其当时事实，不作为当前流程。

> **当前连续计划（2026-10-30，tx-04-09—tx-04-14）**：[四人、多身体与完整世界观测](design/observation-expansion/README.md)。用户确认顺延重号：09恢复四参与者及一人多EntityPlayer，10补全Entity，11补全Grid，12补全房间／楼层，13补全探索／离房保存／跨房玩家，14补全整局／随机选择／HUD／Mod状态。优先两套本地Wiki与公开Lua接口，不要求逐项逆向比对；暂不进入状态同步实现。六项均是待实施计划，本轮只编写文档、一次静态文档审阅。历史同号ASM patch记录保留且与新09分开标识。

> **tx-04-14 Mod验收补充（2026-10-31）**：StageAPI自定义房间类型、带原生载体及无原生载体的CustomGrid、Lua附加状态与生命周期，纳入[具名Mod适配及同tx统一E2E](design/observation-expansion/run-presentation-mod-observation.md#51-stageapi自定义房间与customgrid必测适配)。当前仅补充计划，待tx-04-14实施后统一验收。

> **状态同步前范围澄清（2026-10-30）**：[四人、世界覆盖与多Player归属](design/four-participant-state-sync-scope.md)。当前DLL仍为双人Authority/输入远端，无可玩Replica；min-read-v1不是整个可重建世界，Slot/Machine尚无专用schema，关系字段可读不等于双子支持。用户要求后续恢复四名参与者，本轮选择先明确现状和设计；后续按参与者/控制组/Player实体分离建模，四名参与者不等于四个Player实体。本轮无运行代码修改、构建或测试。

> **tx-04-08 V12完成（2026-10-30）**：[结果、失败历史与同步提示](../artifacts/repentogon/tx-04-08-v12-e2e-03/README.md)。第三轮V12 PASS：3710条A/B tick、104条Ack、四个Recording/Closed输入窗口、5283组持久调用与9个原生窗口通过；338条加载期未知上下文查询保留UNKNOWN，逐条实际透传值均有限为零。两进程正常退出、文件变化符合原D18/D20/D27、环境恢复无误。前两轮同步提示/收尾目录与未知值见证失败、一次旧基线未覆盖的第三档尾字段变更保留FAIL；未改写存档或放宽白名单。其它V/L本次NOT_RUN。

> **tx-04-08 V1/V10 完成（2026-10-30）**：[联合专项结果](../artifacts/repentogon/tx-04-08-v1-v10-e2e-01/README.md)。单一入口、两次顺序启动均正常退出，V1与V10 PASS，L1–L5全通过；482组原生调用四边界配对无误，五份离线源文件逐字节未改，清理无报错。按用户本次授权直接启动，最终统一资料落盘；其余V项/L6/L7本次NOT_RUN，整个tx-04-08不据此宣称完成。

> **tx-04-08 输入边沿与线程分类修复（2026-10-29）**：[实现与范围](design/tx-04-08/tx-04-08-edge-thread-fix.md)。e2e-12原始sample3757有SPACE edge2，下一采样覆盖后提交edge0；A输入源现用16项有界FIFO保存未提交真实边沿，成功统一admission才消费，换代/屏蔽清空。world桥按分类型通知区分WrongThread与异常展开，两个收束位置共用分类。原703/故障验收条件保持，纯编译后执行一次整体E2E，原失败保留。

> **tx-04-08 e2e-12（2026-10-29）**：[执行记录](../artifacts/repentogon/tx-04-08-e2e-12/execution-notes.md)。录制身份等待修复实测覆盖14次start，各跳过1次旧快照并成功进入新Recording。主稳定段253/250通过；11类故障执行，10类归约通过，thread因实际UNPAIRED_SCOPE与预期WRONG_THREAD不符而FAIL。Continue后703准备及一次SPACE已发出，第33项等待新Player Init超时，整体FAIL；第二进程未启动。完整报告生成、环境恢复无报错，游戏正常清理退出0，无自动重跑。

> **tx-04-08 录制等待修复（2026-10-29）**：[修改与边界](design/tx-04-08/tx-04-08-recording-wait-fix.md)。所有录制等待关联 start 回复的 request/generation/recording，停止也按录制身份等待，避免旧 Closed 误判新请求。e2e-11 的28次 UnknownEntity 已追到同一 Type6/Variant19 Laser，保留不支持类别及不完整资格。用户本会话要求修复后直接整体测试、无需准备询问；逐启动仍提示停止操作，结果另记新 artifact，原失败保留。

> **tx-04-08 e2e-11（2026-10-29）**：[执行记录](../artifacts/repentogon/tx-04-08-e2e-11/execution-notes.md)。主稳定段254/250通过；Grid variant仅观察生效，岩石创建、删除后同格生成便便及移除到达，原生旧退休→新出生关联通过。切房/B死亡复活/换层动作已到达。随后启动故障录制时harness误将旧primary/generation1的Closed作为新generation2终态而FAIL；新录制实际稍后Recording并被正常清理。仍有28次UNKNOWN_CLASS未分类。最终报告生成、环境恢复，未启动第二进程、未改代码或自动重跑。

> **tx-04-08 Grid variant策略调整（2026-10-29）**：[实现与边界](design/tx-04-08/tx-04-08-grid-recreation-fix.md)。按用户要求，Grid创建/到达/替换/删除与最终重建归约不再严格比对variant；同时保存请求、实际及先前观察值。位置、类型、seed、varData和原生身份/生命周期要求不变。e2e-10失败保留，新策略只修改与纯编译，未自动测试。

> **tx-04-08 e2e-10（2026-10-29）**：[执行记录](../artifacts/repentogon/tx-04-08-e2e-10/execution-notes.md)。主稳定段254/250通过；首次Grid创建的空地检查通过且API返回true，但实际岩石variant2与请求0不符，后态检查失败。删除后生成便便场景未执行，变体变化原因未核定。整体FAIL；全量报告正常生成，游戏正常退出0、环境恢复，未启动第二进程、未改代码或自动重跑。

> **tx-04-08 Grid夹具修正（2026-10-29）**：[本地Wiki核对与实现](design/tx-04-08/tx-04-08-grid-recreation-fix.md)。按用户指出的公开API约束，将完整岩石上直接重复Spawn改为明确删除、自有同格跨帧空地复核、固定seed创建POOP；ToPoop只取派生接口。原生归约要求旧身份退休→后续新owner出生，不声称单次原生覆盖。e2e-09失败保留，本次修改与纯编译，不自动测试。

> **tx-04-08 e2e-09（2026-10-29）**：[执行记录](../artifacts/repentogon/tx-04-08-e2e-09/execution-notes.md)。30秒/主250目标已完成254份连续完整普通样本，253条odd-only gap按规则保留。随后第25项Grid直接替换返回false，夹具grid_spawn_false，整体FAIL；第二进程及后续场景未执行。全量报告已生成，L1/L3/L4/L5/L7通过，游戏正常退出0、环境恢复，无自动重跑或本轮实现修改。

> **tx-04-08 e2e-08 与规模调整（2026-10-29）**：[实机记录](../artifacts/repentogon/tx-04-08-e2e-08/execution-notes.md)。提前录制生效，首份完整样本23个Effect均有初始化资格；30秒取得797份完整样本，因未达该次2,000目标而FAIL。全量报告已生成，游戏正常退出、环境恢复，未启动第二进程。用户随后要求目标随时间调整，当前主250/对照32份，实时与最终归约统一读scenario；原失败保留，新规模仅修改与纯编译，未重跑。

> **tx-04-08 开局记录时机修复（2026-10-29）**：[实现与限制](design/tx-04-08/tx-04-08-early-recording-fix.md)。两次初始录制均前移到第二档角色页，等真正 Recording 再 New Run 和 seed 重开，补存时序状态与 checkpoint。初始化见证/字段资格、2,000 次稳定段和完整报告条件不变；Rust 最终纯编译成功、harness 产物已更新。本轮未部署或测试，e2e-07 失败保持原样。

> **tx-04-08 e2e-07（2026-10-29）**：[结果](design/tx-04-08/tx-04-08-e2e-results.md)。Grid分类扩展已实测有效，无UNKNOWN_CLASS；Effect缺读取资格导致240秒完整采集仍0。全量索引正常核对28,672 tick/480Ack，匹配错误均0，最终自动报告完整生成（最后save_after后约5.05秒）；L1/L7通过。游戏正常清理、环境恢复无报错，第二进程未启动，整体仍FAIL，未重跑。

> **tx-04-08 Grid/全量索引修复（2026-10-29）**：[实现与边界](design/tx-04-08/tx-04-08-grid-and-fullscan-fix.md)、[静态主审](research/tx-04-08-grid-kinds-static-confirmation.md)。指定GPT-6 Luna/max补核Grid根+17派生类，主智能体独立复读RTTI/Factory/Init，补入16个遗漏类，核定41个未知对象为Wall。全量输入/Ack/持久调用/采集阶段/L6对齐改为分组索引查询，保留重复与原判据，不采样。DLL与Rust纯编译成功，未部署或运行验收；e2e-06失败及人工停止记录保留。

> **tx-04-08 e2e-06（2026-10-29）**：[记录](design/tx-04-08/tx-04-08-e2e-results.md)。Host监听等待已修复并实测握手/A/B Active成功；240秒稳定窗口完整采集为0，主要因Grid域UNKNOWN_CLASS。14,430次attempt、7,199不完整capture、117事件与2checkpoint正常收口，环境恢复无报错。最终trace归约重复全表扫描而长耗时，按用户明确要求停止，原始证据全部保留；最终自动report未生成，整体仍未通过，未重跑。

> **tx-04-08 e2e-05（2026-10-29）**：[记录](design/tx-04-08/tx-04-08-e2e-results.md)。按用户要求修复writer扩展盘符路径处理并纯编译，整体入口实际进入Recording，13条gap与1checkpoint正常drain/flush；原路径问题获得限定实机覆盖。随后远端首包报10054、未握手，发现Host排队回复后未等监听就绪即启动远端的时序缺口。第一进程正常退出、环境恢复无报错、无新增crash；L1/L7通过，稳定段/第二进程未完成，整体仍FAIL，未自动重跑。

> **tx-04-08 e2e-04（2026-10-29）**：[记录](design/tx-04-08/tx-04-08-e2e-results.md)。用户要求开始并确认第一次启动，S0与setup通过；world在根目录准备阶段`PREPARE_FAILED / root_directory`，attempt=0，Host与第二进程未启动。静态定位writer未处理`\\?\F:\`路径根前缀；仅记录，未改实现或重跑。第一进程正常清理退出0、环境恢复无报错、无新增crash；L7完整导入通过，整体仍FAIL。

> **tx-04-08 e2e-03（2026-10-28）**：[记录](design/tx-04-08/tx-04-08-e2e-results.md)。用户许可修正后再执行；已进入第二档单A新局，S0因harness遗漏tx08结构化players分支而误拒绝合法观测。L7实际完整导入通过（15820条记录/6335完整capture/9485gap/93事件）。第一进程正常失败清理，第二进程/world未启动，环境恢复；已修正schema分支，仅纯编译未再运行。

> **tx-04-08 用户许可重执行结果（2026-10-28）**：[e2e-02](design/tx-04-08/tx-04-08-e2e-results.md)在第一进程启动后、最初counters上因主线程尚未就绪而超时；L7实际导入另拒绝旧gap的15个零来源头。第一进程正常失败清理，第二进程/world未启动，指定mod与环境已恢复。已修正菜单等待顺序与显式legacy空头兼容，Rust纯编译成功；未再运行游戏或CLI，原FAIL与冻结材料保留。

> **tx-04-08 首次整体入口结果（2026-10-28）**：[结果记录](design/tx-04-08/tx-04-08-e2e-results.md)。e2e-01 在启动前发现 `arknights_terra_a_journey_3631023221` 仍启用而失败，实际游戏启动0次，未部署/采集；Helper已恢复，未自动重跑。原自动FAIL与缺材料状态保留，完整功能仍未验收。

> **tx-04-08 剩余工程续作（2026-10-28，D32）**：[实现与主审](design/tx-04-08/tx-04-08-defg-implementation.md)。已接入 Entity/Grid 完整复制窗口的 RNG 前后四字、main 原生关闭判断的非阻塞收口、原始证据归约与索引、精确离线差异/输入关联及旧仓库六文件原始录制。特殊 Player Replace 已按 Type1/Variant0、703/slot0/charge1、公开 GetItemState()==0 和一次自然 SPACE 接线，安排在 Continue 后，实际成功与旧对象保留由 DLL 证据判定。退出/Player 逆向由 GPT-6 Luna/max 执行，主智能体独立复读原始指令并纠正返回变量与角色类型假设。完整 DLL/Rust 纯编译成功，产物已更新；未部署/测试，唯一整体 E2E 等待逐次启动准备确认。下方历史记录保留原时点。

> **tx-04-08 A/B/C 工程续作（2026-10-27，D32）**：[实现、静态主审与未覆盖项](design/tx-04-08/tx-04-08-abc-implementation.md)。A 修正 +177 跨字段初始化漏项并接入本轮用户采纳的当前值/撤销规则；B 接通受token/generation限定的11类故障、独立录制编排和实际证据归约；C 加入七类Entity/Grid具体参数、真实覆盖/移除与episode索引。特殊Player Replace资格仍UNKNOWN/默认禁用，复用与重复发布须实际观察。Rust及完整DLL纯编译成功，未部署、启动游戏或测试。D–G与唯一整体E2E仍未完成。

> **tx-04-08 工程任务索引（2026-10-27，D32）**：[A–G任务清单](design/tx-04-08/tx-04-08-current-engineering-tasks.md)保留本轮前缺口与交付要求，以上方ABC续作更新为当前状态；仍不是“只剩运行验收”。下方代码交付/纯编译记录保留原时点。

> **tx-04-08 Lua 夹具与统一 E2E harness（2026-10-27，D32）**：已编写 F8 世界动作扩展、两次启动独立确认、2,000/256 次文件归约、真实关闭/Continue、L1–L7 CLI 编排及环境恢复，见[实现说明与未覆盖项](design/tx-04-08/tx-04-08-harness.md)。仅纯编译，未部署、启动游戏或测试；具体世界动作参数、故障注入入口、RNG 前后见证和直接退出等限制如实保留，整个 tx-04-08 未验收。下方历史进度保留其原时点。

> **tx-04-08 离线 replay／compare（2026-10-27，D32）**：已接入 harness 的两个独立文件命令，完成严格模型、全量 canonical 重编码、生命周期/checkpoint 读取、显式 legacy 导入、构建/布局兼容检查、对齐证据和分层比较，见[实现、命令与 alignment 格式](design/tx-04-08/tx-04-08-world-offline.md)。Rust Release 纯编译成功；未运行 replay/compare、测试或游戏，L1–L7 与唯一整体 E2E 仍未执行。下方历史未实现记录保留原时点。

> **tx-04-08 cs world 命令（2026-10-27，D32）**：已编写 `start/stop/status/checkpoint`、异步 Preparing/关闭、Manager 边界接收与独立 checkpoint 产物。见[命令、线程和范围说明](design/tx-04-08/tx-04-08-world-commands.md)及[纯编译链接记录](../artifacts/repentogon/tx-04-08-world-commands/README.md)。完整 DLL 编译链接成功；未测试、未启动游戏、未部署，完整 tx-04-08 的唯一整体 E2E、2,000 次及双进程比较仍未执行。下方历史“命令未实现”与构建失败记录保留原时点。

> **tx-04-08 SDK 构建兼容修复（2026-10-27，D32）**：已修复完整 DLL 的 `friend static` 编译阻断。`JoinMode/JoinCreate` 声明移到 C++17 `bridge.h`，Host 补丁移除 C++20 状态头依赖，Host/Odd 两个 SDK 适配单元单独用 C++17；核心继续 C++20，SDK、汇编与补丁行为未修改。在全新构建目录完成全部 DLL 单元及 core/net/world 的编译和链接，产物 `out/repentogon/zhlIsaacCS.dll`。见[修改与完整构建日志](../artifacts/repentogon/tx-04-08-sdk-build-fix/README.md)。仅纯构建，未测试、未启动游戏；下方构建失败记录保留其原时点。

> **tx-04-08 collector、主线程桥与 post-state（2026-10-27，D32）**：已完成本项代码编写，见[实现说明与保守范围](design/tx-04-08/tx-04-08-collector-bridge.md)。新增主线程只读 collector、Manager/阶段/生命周期与 A/B witness 桥、六种本地 post-state、clock 提交后匹配批次发布；19 项观察由同一清单派生内层 native witness，待注册 **79/96**（distinct RVA 59）。world 能力由独立配置允许注册，录制仍需后续命令/后台准备编排。`InitUnknown` 字段保留 UNCONFIRMED，含该字段的批次不冒称完整，2,000 次仍未达成。仅代码生成与纯编译，未测试、未启动游戏；整 DLL 仍有既有 SDK `friend static` 编译错误。命令/离线工具/夹具/统一整体 E2E 待继续，下方记录保留原时点。

> **tx-04-08 layout/Hook 清单与生成器（2026-10-27，D32）**：按用户指示完成“实际工程改动的 layout/Hook 清单与生成器”，见[布局与 Hook 清单实现说明](design/tx-04-08/tx-04-08-layout-hook-manifest.md)。唯一清单 `mod/binding/hooks.json` 升级为 schema 2：41 条既有行不变，新增 19 项 `world_observations`（逐参数 ABI、完成含义、SDK `flags/argdata` 交叉校验）；新增 `mod/binding/world_layout.json` 登记 78 项字段偏移/证据、15 个来源、10 个 RTTI 类与目标版本常量。`mod/tools/gen_binding.py` 生成 wrapper、容量常量、布局表与 machine-readable 清单，生成期拒绝 ABI/容量/字段/几何关系不一致；`binding/world_layout.*` 提供只读访问，`world/observe.*` 提供不进入会话失败路径的 world 观察策略。待注册 **60/64**（既有 41 + world 19，distinct RVA 59）；world 组当前不安装（`WorldObservationArmed()` 固定 false），桥接项接入配置后启用。本轮只做一次临时目录生成与纯编译，未运行测试、未启动游戏；DLL 整体链接仍被既有 `ASMPatch` 头的 `friend static` 与 C++20 组合挡住，且确认 `host_patch.cpp` 不能整体降级 C++17，留给“必要纯编译链接”项。下方历史记录保留当时状态。

> **tx-04-08 world 核心代码（2026-10-27，D32）**：按用户指示完成“实际工程改动的第一项”——纯值模型、身份表、槽/队列/事件环缓冲与 writer 的代码编写，见[world 核心实现说明](design/tx-04-08/tx-04-08-world-core.md)。新增独立静态库 `isaac_cs_world`（不依赖 SDK/游戏头，`/W4` 无警告），包含 78 项 `min-read-v1` 字段声明、33 位 radix 本地身份、三槽 `FREE→WRITING→PREPARED→READY→READING→FREE` 状态机、64 MiB/64 项双上限值队列、gap 控制环、`isaac.cs.world-record` v1 的 manifest/captures/lifecycle-events/diagnostics/result 编码与两个后台线程的有界收口。本轮只做纯编译，未运行测试、未启动游戏；DLL 整体链接被既有 `ASMPatch` 头的 `friend static` 与当前 MSVC C++20 组合挡住（非本次改动引入，留给“必要纯编译链接”项）。主线程桥、命令、离线工具、harness 与统一 E2E 仍待继续，下方历史记录保留当时状态。

> **tx-04-08 ST6–ST8 静态工作（2026-10-26，D32）**：每项各由一个 GPT-6 Luna / max 子智能体逆向，主智能体独立取证并逐项审阅，完成[owner/生命周期/工程边界报告](research/tx-04-08-st6-st8-static-confirmation.md)。核对 Spawn/Morph/Restore/Replace/Grid 嵌套完成、公开夹具参数、Player 读取依据与撤销、41 行/40 RVA 的注册与容量、内存预算和 Shutdown 窗口。主审纠正 Replace 分支倒置、补齐 Morph Lua 注册，并确认 B 部分 Invalidate 返回未统一撤销及退出后无 Manager drain 窗口。ST1–ST8 静态工作完成，具体 UNKNOWN 和实现缺口保留；功能实现与整体 E2E 待继续。未构建、测试、启动游戏或修改 IDB。

> **tx-04-08 ST3–ST5 静态工作（2026-10-26，D32）**：每项分别由一个 GPT-6 Luna / max 子智能体逆向，主智能体独立取证、逐项审阅，完成[字段/来源/分类身份报告](research/tx-04-08-st3-st5-static-confirmation.md)与[78 项字段对照](research/tx-04-08-st3-agent-notes.md)。确认 Player 多项偏移重排、Game frame `+25A70`、448 Grid 槽与门别名、四类保留 Player 关系和只读 RTTI/池复用规则；主审补齐漏命中与原生字段/坐标注册证据。ST1–ST5 静态工作完成，具体 UNKNOWN 保留；ST6–ST8、实现与整体 E2E 待继续。本轮未构建、测试、运行游戏或修改 IDB；下方旧记录保留当时状态。

> **tx-04-08 ST2 静态工作（2026-10-26，D32）**：按用户要求由一个 GPT-6 Luna / max 子智能体逆向，主智能体独立读取 IDB/源码并审阅，完成[完整采样阶段、Hook 次序与短路核验](research/tx-04-08-st2-static-confirmation.md)。普通 Room 返回后还有 Game 后处理和标准尾部；1026 可跳过整轮，1160 可跳过 Player 派生更新；复制应在 Manager 下游链正常返回后、EndManager 发布/退休前准备。ST1/ST2 已完成，ST3–ST8、实现和整体 E2E 待继续；未构建或执行 E2E。代理额外 JSON 语法校验已如实记录为[过程偏差](../artifacts/repentogon/tx-04-08-static/st2-agent/process-deviation.md)。下方 ST1 记录保留其当时状态。

> **tx-04-08 ST1 静态工作（2026-10-26，D32）**：按用户要求由 GPT-6 Luna / max 子智能体逆向、主智能体独立审阅，已完成[19 项入口与 ABI 登记](research/tx-04-08-st1-static-confirmation.md)。已区分 GridFactory 的 ECX/EDX 与官方同址定义差异、Replace 两 Player 参数/AL 返回、基类析构与删除包装。ST2–ST8、实现及整体 E2E 尚未完成，没有构建或测试。完整迁移范围、DLL 本地采集、2,000 次稳定段与两个顺序进程安排见[迁移设计](design/tx-04-08/tx-04-08-implementation-plan.md)和[E2E 方案](design/tx-04-08/tx-04-08-e2e-plan.md)；历史记录保持原样。

> **tx-04-07 已完成（用户正式接受，D31，2026-10-26）**：用户明确将 e2e-11 的整体 E2E 视为全部通过，整体验收正式通过。原始自动 FAIL、S17 停止与未执行项保持原样，不再安排本轮补测。见[验收记录](../artifacts/repentogon/tx-04-07-e2e-11/acceptance.md)与[历次结果](design/tx-04-07-results.md)。下方均为历史时点记录。

> tx-04-07 e2e-11（2026-10-26）：**V12 PASS**，六tick恢复窗口与实时激光时序获得实际证据：提前B激光0，计划松键tick同tick激光1，两个真实中性tick有Shooting暂停证明。整体仍FAIL，S0–S16完成、S17无足够观测净空、后续未运行。环境恢复、无自动重跑，见[本次结果](../artifacts/repentogon/tx-04-07-e2e-11/README.md)及[净空编排修正](../artifacts/repentogon/tx-04-07-movement-clearance-fix/README.md)。

> tx-04-07 V12 修复与 e2e-10（2026-10-26）：用户采纳六 tick Shooting 恢复窗口；已修复握手空档的射击消费，新增实时 cs witness 并调整激光归属，统一纯构建成功。唯一 E2E 在 S9 因 B 无观测可移动方向停止，V12 NOT_RUN，完整修复尚未实测通过。游戏正常退出、环境恢复，无自动重跑。见[修复](design/tx-04-07-v12-recovery-fix.md)与[结果](../artifacts/repentogon/tx-04-07-e2e-10/README.md)。

> tx-04-07 e2e-09（2026-10-26）：开局 debug 10 控制台确认启用，A 存活保存，S20 Continue 已完成；S0–S20 COMPLETE，S21 因 ESC 后未观察到暂停而 FAILED，整体仍 FAIL。前三轮网络对账通过，V12 仍有实际重绑空档/早期激光；V13 加载不变量通过但自然重复查询覆盖未齐。正常退出、环境恢复、无自动重跑，见[本次记录](../artifacts/repentogon/tx-04-07-e2e-09/README.md)。

> tx-04-07 e2e-08（2026-10-26）：**FAIL**，单启动完成 S0–S19，S20 因 A 死亡、退出 should_save=false、Continue 灰色而超时，S21 未运行。前三轮严格网络对账通过，中继收尾中断未重现；V12 对照激光已观察，但存在重绑后中性空档和提前发射，另有 Lua loop 快照偏移。游戏正常退出、环境恢复，无自动重跑。见[本次结果](../artifacts/repentogon/tx-04-07-e2e-08/README.md)。用户要求下次开局输入 debug 10，已接入并纯构建，尚未实测，见[准备记录](../artifacts/repentogon/tx-04-07-debug10-preparation/README.md)。

> tx-04-07 e2e-07 入口（2026-10-26）：启动前因遗漏 Download Helper 临时禁用步骤而中止，未部署、未启动游戏，全部用例 NOT_RUN；中继/V12 修复尚未实机验证。原报告把缺少 crash_before 基线下的历史 dump 误报为本轮崩溃，实际说明见[本次记录](../artifacts/repentogon/tx-04-07-e2e-07/README.md)。未自动重跑，下方历史结果保留。

> tx-04-07 修复编写（2026-10-26）：中继将 UDP 10054/10052 单独记录，主机退休后有界收包；V12 增加普通松键对照、延长时长并按实际消费命令归约，真实重绑输入空档不再被缺激光掩盖。统一纯构建成功，尚未进行修复后 E2E。见[说明与限制](design/tx-04-07-relay-v12-fix.md)及[构建产物](../artifacts/repentogon/tx-04-07-relay-v12-fix/README.md)。下方历史结果保留。

> tx-04-07 最新 e2e-06（2026-10-26）：**FAIL**，S6暂停等待回归已修复，S0–S15已执行，V1/V7/V8/V9/V11/V15通过；第二轮Bye后主机正常退休，但UDP中继receive报10054，第三轮未启动。V12仍缺激光证据、NOT_OBSERVED，S17–S21未执行。正常退出、无新dump、已清理，无自动重跑。见[结果](design/tx-04-07-results.md)、[修复](design/tx-04-07-pause-wait-fix.md)与[产物](../artifacts/repentogon/tx-04-07-e2e-06/README.md)。下方历次记录保持原样。

> tx-04-07 最新 E2E（2026-10-26）：e2e-05 **FAIL**。焦点维护修改已构建；S6 主动关闭计划内暂停后的稳定等待被新增检查过早终止，属于 harness 回归。无实际失焦/恢复样本，恢复功能尚未验证。S4 移动、skip、V8 投弹已通过对应检查；游戏正常退出、无新 dump、环境已清理，未自动重跑。见[结果](design/tx-04-07-results.md)、[修改/回归说明](design/tx-04-07-focus-fix.md)及[产物](../artifacts/repentogon/tx-04-07-e2e-05/README.md)。下方历次记录保持原样。

> tx-04-07 退休崩溃修复后第三次 E2E（2026-10-25）：**FAIL**。S13 Stop 已正常退休，S0–S14 完成；随后夹具混淆实体 Index=17 与玩家 ordinal=1，缺少 B 动画字段，触发 b_animation_timeout。游戏正常退出、无新 dump、shutdown trace 丢失为 0，环境已清理；本轮仅一次启动、未自动重跑。见[修复](design/tx-04-07-retirement-fix.md)、[结果](design/tx-04-07-results.md)及[产物](../artifacts/repentogon/tx-04-07-e2e-03/README.md)。下方前次失败保持原样。

> tx-04-07 实质修复后第二次 E2E（2026-10-25）：**FAIL**。启动 trace 丢失已未重现，S0–S12 完成、V11 死亡/复活 PASS；S13 Stop 退休时游戏崩溃（0xC0000005），S14–S21 未执行。本轮仅启动一次、未自动重跑，文件清理与 Download Helper 恢复已完成。见[修复](design/tx-04-07-trace-fix.md)、[两次结果](design/tx-04-07-results.md)和[本轮产物](../artifacts/repentogon/tx-04-07-e2e-02/README.md)。下方首次失败与编码时点均保留。

> tx-04-07 唯一整体 E2E（2026-10-25，America/Los_Angeles）：**FAIL**，只启动一次，未重跑。菜单 Host 与 relay 握手完成；启动阶段 trace 丢失 63 条，首轮开局因 trace_event_lost 退休，尚未创建 K/Join B，tick=0。游戏正常退出，测试 mod/C/S 配置已清理、Download Helper 已恢复。见[结果](design/tx-04-07-results.md)与[原始报告](../artifacts/repentogon/tx-04-07-e2e-01/report.md)。下方“未测试”保留编码阶段时点。

> tx-04-07 3.4 编写（2026-10-26）：relay、远端启动/换代/旧 Ack/超时、单启动四轮 S0–S21、V1–V16 归约、文件通知与窗口快照已编写，CLI/部署/夹具配对完成。Win32 DLL 与 i686 Rust harness 已统一纯编译链接；见[编写记录](design/tx-04-07-harness.md)和[原始构建日志](../artifacts/repentogon/tx-04-07-harness/README.md)。没有部署、启动游戏或运行测试，唯一整体 E2E 尚未执行。下方阶段记录保留原时点。

> tx-04-07 3.2 编写（2026-10-26）：已补齐状态与 trace，包含退休后的 b_life/join_state 语义、会话与参与者代次、生命归属、按原因丢弃/Ack、加载/持久证据、完整分类查询覆盖与场景检查点，以及日志分片/截断记录。见[编写与接线记录](design/tx-04-07-state-trace.md)。未构建、部署或测试，整个 tx-04-07 的统一纯构建/唯一整体 E2E 尚未执行；其余工程进度见实现方案。

> tx-04-07 ST2 主审（2026-10-26）：已回读当前 IDB 与官方源码，完成[ST2 材料审核](research/tx-04-07-static-confirmation.md#15-st2-主审范围结论与材料更正2026-10-26)，ST1–ST7 静态取证与主审均完成，UNKNOWN 保留。普通受限复活复用原 Player；健康/归属仍须实际重读，Appear 会跳过主要射击块。已更正代理笔记的健康保证、移动措辞、InputManager 47 槽与指令地址，并同步[实现方案](design/tx-04-07-implementation-plan.md)及[工程细化](design/tx-04-07-engineering-changes.md)。本次只做静态取证和文档修订，未构建或测试，也未验收工作区已有的 game/ 编码；下方 2026-10-25 的待主审与未编码表述保留当时状态，当前工程进度见实现方案。

> tx-04-07 工程细化（2026-10-25）：已根据现有静态结论补充[工程改动细化](design/tx-04-07-engineering-changes.md)，列明 B 生命/代次/当前归属、首次菜单 Continue 的统一 Restore 决策、重复查询观测、跨 loop 持久窗口及文件证据、relay/远端与四轮 E2E 的文件和接口；[总方案](design/tx-04-07-implementation-plan.md)已同步。用户确认没有额外 ST2 结论，ST2 整项主审仍待完成。仅修改设计文档，未改生产代码、构建、部署或测试；不预记 V1–V16 通过。

> tx-04-07 静态工作（2026-10-25）：ST1、ST3–ST7 已完成取证和主审；本轮 ST3、ST4 各由一个 GPT-6 Luna / max 子智能体逆向，主智能体另取 IDA 与源码审阅。已定位换层/Continue 的加载窗口、内部回调与稳定重绑边界；Continue 按保存记录恢复 Player，随后重分配 controller。发现当前 Restore/RunExit 会在恢复前退休菜单 Host，S20 需在后续实现中修正；此前 ST5–ST7 的 Baby 位、B 射击暂停及持久观测缺口结论保留。见[静态报告](research/tx-04-07-static-confirmation.md)与[实现方案](design/tx-04-07-implementation-plan.md)。UNKNOWN 保留；ST2 已有代理材料、整项主审待完成，实现、构建和测试未开始。U1–U11 已确认（D29、D30），第二档已解锁阿萨泻勒。

> **tx-04-06 完成（用户接受，D28，2026-10-25）**：用户决定不再执行第三次 E2E，将第二次 E2E 算作验收通过（V1–V11 PASS；V12 只因第一、三档当日备份找不到历史基线而失败，D27 起每日备份不参与判定；原报告保持 FAIL）。见[结果](design/tx-04-06-results.md)。

> tx-04-06 第二次 E2E（2026-10-25）：**FAIL**，S0–S11 全部执行，V1–V11 PASS。V12 的唯一失败来自 harness：当日备份文件名带日期，按名字找不到 e2e-08 的 D20 基线；这两份文件只改了 Counter/Checksum 字段。未重跑。用户随后决定 D27：存档判定不看每日备份，harness 已修改并纯构建。见[结果](design/tx-04-06-results.md)。

> tx-04-06 第一次 E2E（2026-10-25）：**FAIL**。S0–S3 已执行：握手、K 注册、Join 产生 B（Isaac）、Active，并完成第一个 tick；随后游戏在原生 `dynamic_cast<NetInputDevice*>`（`0x52F880`）处因 K 的 vtable 前没有 RTTI COL 而抛出 `__non_rtti_object` 崩溃，S4–S11 未执行，未重跑。用户决定 D26：按原型自建 COL、修正 harness 崩溃记录、删除转储原件、保留编号；修复已纯构建，第二次 E2E 待执行。见[结果](design/tx-04-06-results.md)。

> tx-04-06 3.4/3.5 编写（2026-10-25）：真实 Host/stop、配置/状态/trace、主机 UI、Rust 独立协议/脚本远端与 S0–S11 / V1–V12 harness 已编写，V2 方向判定已修正；Hook 清单增加 ST8 只读持久入口观测。Win32 DLL 与 i686 Rust harness 纯编译链接成功，见[实现记录](design/tx-04-06-ops-harness.md)、[构建日志](../artifacts/repentogon/tx-04-06-ops/README.md)。尚未部署、启动游戏或执行测试。下方各阶段记录保留其原始时点。

> tx-04-06 game 编写（2026-10-25）：K、Join、主线程 peer、双 ordinal 拓扑/consumer/lifecycle 与必要的 11 处常驻补丁已编写，Win32 Release 纯编译链接成功。见[实现记录](design/tx-04-06-game.md)、[构建记录](../artifacts/repentogon/tx-04-06-game/README.md)。Host API 已有，ops Host/stop 仍占位，配置/UI/Rust/harness 待接入；没有部署、运行游戏或测试，整体 E2E 待 tx-04-06 全目标完成。以下保留阶段原始记录。

> tx-04-06 core 编写（2026-10-25）：第 3.2 节 Host 双参与者会话、双绑定激活与 PeerBye/PeerTimeout/PeerProtocol 有序退休已编写，A/B 共用输入与 tick 流水线，Win32 Release 纯编译链接成功。见[实现与接线约定](design/tx-04-06-core.md)、[构建记录](../artifacts/repentogon/tx-04-06-core/README.md)。尚未接入 game/K/Join、Host 命令与 Rust 远端，没有部署或运行测试；整体 E2E 留待 tx-04-06 全目标完成。下方阶段记录保留原始时点。

> tx-04-06 网络规则同步（2026-10-25）：Authority 已修改为旧包 stale 丢弃、独立重复/迟到计数、Bye 原字节三次发送及接收端有界副本计数，Win32 Release 纯编译链接成功。见[同步说明](design/tx-04-06-net.md)、[构建记录](../artifacts/repentogon/tx-04-06-net/build-04-network-rules.log)。未部署或运行测试；Rust 远端握手重发和 First Ack 专用副本缓存仍待第 13.2 节实现。

> tx-04-06 net 编写（2026-10-25）：第 3.1 节 `mod/src/net/` 的协议、Endpoint、UDP worker 与 SPSC 队列已编写，独立静态库及 DLL 纯编译链接成功；[实现与线格式](design/tx-04-06-net.md)、[构建记录](../artifacts/repentogon/tx-04-06-net/README.md)。未接入 Host/core/game/命令/trace，没有部署或运行测试，整体 E2E 留待 tx-04-06 全目标完成。下段静态阶段记录保留原始时点。

> tx-04-06 静态工作（2026-10-25）：按用户指示，GPT-6 Luna / max 子智能体使用 IDA MCP 完成指定逆向，主智能体独立取证审阅；[ST1](research/tx-04-06-static-confirmation.md) 与 [ST2–ST9](research/tx-04-06-static-remaining.md) 均已研究并审阅。已明确唯一角色参数/CALL shim 栈、Join 后绑定及移除 ABI，发现设备通知隔离、UI 直接尾部读取与持久副作用风险；[实现方案](design/tx-04-06-implementation-plan.md)已同步，兼容处理与功能尚未实现，没有构建或测试。D24/D25 既有决定保留。
>
> **tx-04-05 完成（用户接受，D23，2026-10-24）**：用户决定将第四次 E2E 算作验收通过（V1、V3–V10 PASS；V2 的判定方向缺陷不修复，原报告保持 FAIL）。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第四次 E2E（2026-10-24）：**FAIL**，V1、V3–V10 PASS；V2 的 S10 已按 D22 向左原生移动约 198 px，但判定仍要求向右（D22 修改遗漏），未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第三次 E2E（2026-10-24）：**FAIL**，V1、V3–V10 PASS；只有 V2 的 S10 因 harness 固定向右移动、被随机房间的岩石挡住而失败，DLL 没有接管输入，未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第二次 E2E（2026-10-24）：**FAIL**，S0–S11 全部执行，V1、V3–V9 PASS；V2 因 S1 落在开局约 3 秒的未模拟区间、V10 因 F9 触发原版手动截图而失败，均为 harness/夹具问题，未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第一次 E2E（2026-10-24）：**FAIL**，S0 启动加载超过 5 s 时 harness 强制 focus 失败，未进入 run，未重跑；DLL 侧能力、28 个 Hook、8 个槽补丁与 Odd shim 安装正常。另发现 V10 的 D18 历史基线规则无法匹配。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 测试代码（2026-10-24）：剩余 Lua F8/F9/只读夹具、Rust 单启动 S0–S11 编排与 V1–V10 归约已编写，见[实现记录](design/tx-04-05-test-harness.md)。未部署或启动游戏，没有执行测试，唯一整体 E2E 尚未执行。下方各阶段记录保留其当时状态。

> tx-04-05 game 适配（2026-10-24）：第 3.2 节七个主线程适配模块、必要的常驻 Hook/Odd shim 接线、solo 命令与状态/trace 已编写并纯构建成功，见[实现记录](design/tx-04-05-game-adapter.md)。测试 mod/harness 场景尚待实现，没有部署或运行测试，唯一整体 E2E 尚未执行。

> tx-04-05 core 阶段记录（2026-10-24）：第 3.1 节的 Session、TickClock、TickPipeline、input_core、Participant 已编写，Win32 Release 纯构建成功。代码、Solo 适配和后续接线见[移植记录](design/tx-04-05-core-port.md)。该阶段尚未接入 game/命令；后续实现见上方最新记录。没有运行测试，唯一整体 E2E 尚未执行。下方静态报告与 tx-04-04 内容保留历史语境。

> tx-04-05 静态工作完成（2026-10-24）：T1–T8 已由 GPT-6 Luna / max 逆向并经主智能体独立取证审阅；修正了 Odd 分类、设备 id 映射、采样时点、屏蔽观察、持续 held 提交和字段宽度等方案。T6 不把 new-room 回调当作所有路径最终位置稳定的证明。见[静态报告](research/tx-04-05-static-confirmation.md)；功能尚未实现，没有构建、实机操作或测试。下列 tx-04-04 记录保留历史语境，其第八次整体 E2E 已 PASS。

> tx-04-04 最新（2026-10-24）：退菜单已改为观测暂停状态/退出选项后确认，第七次 E2E 已正确返回主菜单、Game::Exit 计数增长，两次启动正常退出。整体仍 FAIL：Lua run 回应、UI 渲染标记缺失，以及启用/关闭配置两次都有第一、三槽存档字节变化；详见[结果](design/tx-04-04-results.md)。

2026-10-22，tx-03-09；本工程从原研究仓库独立复制，原目录保留。

1. [现行项目规范](refactor/project-conventions.md)：唯一当前流程要求；用户会话指示优先。
2. [按功能模块查Hook与逆向点位](hooks/README.md)：全部当前安装家族、调用依赖、候选函数、字段及虚表，明确证据等级。
3. [状态同步方案](design/state-synchronization.md)：完整基线＋增量状态＋表现事件，首版单房间、无预测；列出实际缺失能力。
4. [迁移说明](migration/README.md)：复制范围、保留的历史产物位置、路径差异与使用限制。
5. [历史T0任务书](refactor/cs-t0/isaac-cs-t0-task-plan.md)、[旧总体规划](refactor/isaac-cs-architecture-plan.md)、[T0代码入口](../cs-t0/README.md)：保留原始时点与失败，不覆盖为新状态。
6. [tx-03-10：Replica更新与展示分派研究](research/tx-03-10-replica-update-presentation-boundary.md)：状态同步迁移的首项实际静态逆向，GPT-6 Luna / max子智能体研究、主智能体复核；没有实现或运行Replica。
7. [tx-03-11：实体绘制与动画事件的逻辑／展示分离](research/tx-03-11-logic-presentation-separation.md)：继续区分 Entity 绘制 pass、Sprite 动画与事件消费者，追到动画事件的弹体创建路径；含 GPT-6 Luna / max 结论及主审。
8. [tx-03-12：派生绘制与 Sprite 资源生命周期](research/tx-03-12-derived-render-sprite-lifecycle.md)：三类实体独立 Lua render 入口、额外图形与独立 Sprite 构造／资源引用／释放；Luna 研究及主审，尚未实现展示 adapter。
9. [tx-03-13：Sprite 图像资源、引用与图形后端](research/tx-03-13-sprite-image-resources.md)：ImageBase control 六槽、两层图像缓存、最后释放与 PNG 纹理上传／删除；Luna / max 结论已由主智能体独立取证复核，实际线程／context 仍未确认。
10. [tx-03-14：renderer 线程来源与 GL context 生命周期](research/tx-03-14-renderer-context-lifecycle.md)：连接正常主入口初始化、TLS current、帧内呈现和资源先于 context 的关闭顺序；补充绕过现有 RenderHook 的加载绘制入口。仍无运行时 thread／context 观察或 Replica 实现。
11. [tx-03-15：Sprite 换图、Reload 与 ANM 引用](research/tx-03-15-sprite-replacement-reload.md)：区分路径更新、逐层加载与 layer 重建；记录正常失败不回滚、限定条件的 Reload 额外引用及 Load 失败借用风险。GPT-6 Luna / max 子结论已由主智能体独立复核并修正 Lua wrapper 描述；下一项为最小展示字段与特殊标志支持边界。
12. [tx-03-16：独立 Sprite 最小展示字段](research/tx-03-16-independent-sprite-display-fields.md)：区分 main／overlay、layer ordinal／track／关键帧索引，解释 setter 与特殊绘制依赖，提出固定姿态的有限支持描述。Luna / max 结论已由主审独立取证复核；下一项为资源准备、姿态应用和失败退休的 owner 顺序，尚无 adapter 实现。
13. [tx-03-17：独立 Sprite owner 顺序](research/tx-03-17-independent-sprite-owner-lifecycle.md)：构造、未发布资源准备、固定姿态应用与正常退休；区分 copy／swap 的字段合同及 native／Lua 外层 owner。Luna / max 结论已由主审复核；新审阅器修正地址分类，保留 tx-03-16 原 FAIL。adapter／资源就绪 predicate 尚未实现。
14. [tx-03-18：最小单 Sprite 适配合同](research/tx-03-18-single-sprite-adapter-contract.md)：最终 image/control pair 就绪语义、64 项 layer 映射边界、reset 后逐 track 固定姿态与自有实例发布／退休接口。Luna / max 结论由主审独立取证复核；原生 loaded 不证明 GPU／像素成功，合同与资源清单尚未实现。下一项为实际绘制接入窗口及 thread/context guard。
15. [tx-03-19：独立 Sprite 绘制接入窗口](research/tx-03-19-sprite-render-entry-window.md)：定位最终 pre-present callsite 的屏幕叠加候选，解释 Hook／TLS／WGL guard、Sprite 参数 ABI、batch 独立强引用与状态版本。Luna / max 结论经主审独立取证并修正初始化状态及地址分类；未安装 Hook／实现 adapter，下一项为具体资源与连续姿态闭环。
16. [tx-03-20：具体资源与连续姿态闭环](research/tx-03-20-sprite-resource-pose-closure.md)：从现成 extracted_resources 收敛 Poof 单层 ANM／RGBA8 PNG 样本，明确 Authority 同窗口采集、同 descriptor 原地更新和变更候选替换。Luna / max 结论经主审复核；支持声明未实现、未加载或绘制，建议下一轮进入最小独立 Sprite adapter／本地 fixture 实现。
17. [tx-04-01／02：REPENTOGON 适配重构设计](design/repentogon-refactor.md)：把原型改为由 REPENTOGON 加载的 `zhlIsaacCS.dll`。Hook 常驻、会话逻辑启停；网络放入 DLL，Rust 只做测试与中继；旧 ISCS／Isaac Sync 兼容约束不延续；会话通过 REPENTOGON ImGui 界面发起。tx-04-02 定下目标为官方 REPENTOGON 与 v1.9.7.12.J273，并修订了现行规范。目前只有设计，没有代码。
18. [tx-04-03：REPENTOGON 目标版本点位核对](hooks/repentogon-binding.md)：在 v1.9.7.12.J273（从 `Repentogon\` 子目录的 exe 复制一份，用 Steamless 脱壳）上静态核对原型首轮点位，产出 43 项 [binding 清单](hooks/repentogon-binding.json)。37/39 个调用点有对应；输入设备子系统变化较大，自有虚拟设备要重做；4 处调用点补丁要按新寄存器分工重写；字段偏移已用 REPENTOGON `.zhl` 印证。只做了静态分析，脚本和中间数据见 `artifacts/repentogon/tx-04-03-static/`。
19. [tx-04-04：实现与结果](design/tx-04-04-results.md)：开发包与[项目代码](../mod/README.md)已实现并编译；27 个入口 Hook、8 个设备槽补丁、`cs` 命令桥、有界命令队列与测试控制口、纯界面 mod 和 Rust i686 harness。唯一 E2E 在注入启动阶段因 Launch 等待 60 秒超时失败，未进入菜单，未重跑。存档无变化，临时测试 mod 已移除、配置已关闭。tick、输入接管和网络不在本轮范围。之后做了静态审阅，并修订了[实现方案](design/tx-04-04-implementation-plan.md)和设计文档；修复沿用 tx-04-04（D16），已实现并编译；第二次 E2E 仍启动超时 FAIL，新增诊断捕获 Use The Launcher! 对话框。两类存档均无变化，未自动重跑；Lua 捕获偏移经 IDA 更正为 Game+0x680D8。 按 D18 修正后，第八次 E2E E1–E7 全部通过。
20. [tx-04-05：实现方案](design/tx-04-05-implementation-plan.md)：循环阶段与 tick 判定、本地玩家 A 经统一队列输入、生命周期和单人会话。U1–U4 已确认（D19）；T1–T8 的[静态确认与主审](research/tx-04-05-static-confirmation.md)完成，原始资料见[artifact](../artifacts/repentogon/tx-04-05-static/README.md)。第 3.1 节 [core 移植](design/tx-04-05-core-port.md)已编写并纯构建成功，[game 主线程适配](design/tx-04-05-game-adapter.md)及必要 Hook/solo 命令也已编写并纯构建成功；[Lua 夹具与 Rust 单启动 S0–S11 / V1–V10](design/tx-04-05-test-harness.md)也已编写并纯构建成功。共执行四次整体 E2E（D20–D22 修正 harness），第四次 V1、V3–V10 PASS；按 D23 由用户接受为验收通过，tx-04-05 完成，见[结果](design/tx-04-05-results.md)。
21. [tx-04-06：实现方案](design/tx-04-06-implementation-plan.md)：DLL 内网络与新协议、按 v1.9.7.12 重做的虚拟设备 K、原生 Join、B 的远端输入与断开退休、Rust 远端客户端。D24 已确认拆分（relay、B 死亡复活、异常 Join 等移到 tx-04-07，world_record 顺延到 tx-04-08）；U1–U4 已确认（D25）。2026-10-25 完成 [ST1](research/tx-04-06-static-confirmation.md) 和 [ST2–ST9](research/tx-04-06-static-remaining.md) 静态研究与主审。Join 四处调用点之外还需 K 通知/UI 兼容处理。后续第 3.1 节 [net 代码](design/tx-04-06-net.md)已编写并纯构建成功，尚未接入主线程 Host 会话；其余工程及整体 E2E 待完成，没有执行测试。

22. [tx-04-07：实现方案](design/tx-04-07/tx-04-07-implementation-plan.md)与[结果](design/tx-04-07/tx-04-07-results.md)：按 D31 用户正式接受 e2e-11 整体通过，tx-04-07 完成；原自动失败和未执行项保留，不再补测。
23. [tx-04-08：world_record 迁移设计](design/tx-04-08/tx-04-08-implementation-plan.md)：D32 的完整迁移范围、本地 artifact、2,000 次稳定段、只读时序/身份/有界缓冲；[ST1 入口/ABI](research/tx-04-08-st1-static-confirmation.md)、[ST2 采样阶段/短路](research/tx-04-08-st2-static-confirmation.md)、[ST3–ST5 字段/来源/分类身份](research/tx-04-08-st3-st5-static-confirmation.md)及[ST6–ST8 owner/生命周期/工程边界](research/tx-04-08-st6-st8-static-confirmation.md)静态核验和主审已完成，具体 UNKNOWN 与实现缺口保留，功能实现待继续。[E2E 方案](design/tx-04-08/tx-04-08-e2e-plan.md)覆盖两次顺序启动和离线比较，尚未执行；没有构建或执行功能测试，ST2 的额外 JSON 校验偏差见报告。

24. [tx-04-08：world 核心实现说明](design/tx-04-08/tx-04-08-world-core.md)：第一项代码改动（`mod/src/world/`）的值布局与预算、线程与槽状态机、身份规则、`isaac.cs.world-record` v1 五个文件的格式、失败归属、故障开关、构建状态与给后续项的接口约定。新增 `isaac_cs_world` 静态库完成一次纯编译，未运行测试、未启动游戏，DLL 整体链接受既有 SDK 头与当前工具链组合影响，留给“必要纯编译链接”项。

25. [tx-04-08：布局与 Hook 清单实现说明](design/tx-04-08/tx-04-08-layout-hook-manifest.md)：Hook 清单 schema 2（41 条既有行 + 19 项 `world_observations`，逐参数 ABI 与 SDK 元数据交叉校验）、容量 60/64 与生成/注册两端的显式报告、world 观察策略层、`world_layout.json` 的 78 项字段/15 个来源/10 个 RTTI 类与编译期几何校验、生成产物与校验清单、world 组延后安装状态、构建验证与保留 UNKNOWN。一次临时目录生成与纯编译，未运行测试、未启动游戏，DLL 整体链接仍未完成。

`refactor/`与`isaac-sync/`为迁入的历史资料；旧gate、哈希、独立角色与重复验收条款已由现行规范覆盖。新增`hooks/`与`design/`用于后续状态同步研究，不表示对应Replica生产功能已经实现。

- tx-04-10 当前实现与统一 E2E：[公共接口、Entity／Grid 拆分及完整 Entity 来源](design/observation-expansion/tx-04-10-implementation.md)。

- tx-04-12 已完成：[房间身份/构建、进度、环境及楼层结构观测](design/observation-expansion/tx-04-12-implementation.md)及[206项特殊房间/Greed补全](design/observation-expansion/tx-04-12-supplement.md)，同tx统一E2E 7/7 PASS，实际覆盖与边界见结果。

- tx-04-20 I16—I20：[效果、History、完整身体资源、costume与五槽武器安装及必要逆向主审](design/tx-04-20/tx-04-20-equipment-installation.md)。I21—I40继续推进；本轮未构建或测试，完整native_apply仍为false。


