> **tx-04-19前5项已实现（2026-11-01）**：R19-01—05已接通正式来源、全部非Player类别的私有原生owner与状态应用、身份／Morph调和、引用及必要Player载体、原生资源与绘制排序。GPT-6 Luna／max负责缺口逆向，主智能体复读、纠错并完成实现。生产DLL已编译、链接；未部署、启动游戏或运行测试。见[实现范围、主审事实与验收边界](design/tx-04-19/tx-04-19-native-entity-plan.md#r19-first-five-progress)。R19-06—10及19统一E2E仍待完成，不代表整个19已验收；下方“仅文档”段落为此前设计时点。

> **tx-04-19实现设计、任务路线与整体验收计划（2026-11-01，仅文档）**：[非Player Entity原生应用设计](design/tx-04-19/tx-04-19-native-entity-plan.md)、[10项实现任务及实施路线](design/tx-04-19/tx-04-19-native-entity-plan.md#101-实施路线与依赖)、[统一E2E计划](design/tx-04-19/tx-04-19-e2e-plan.md)。明确来源／共享载体→原生对象与身份→引用／必要Player→资源展示→原生阻挡与Authority效果→恢复退休→全目标统一验收的依赖，列出实际改动、交付行为、场景对应和待核定原生入口。优先复用原生机制，只补实际缺失状态与接线；未改代码、构建或运行测试，19尚未实施／验收。

> **19碰撞职责与交互验收补全（同日，仅文档）**：飞行与非飞行的空间阻挡／通行均由Client原生机制判定和处理；Authority同步所需状态并裁决其他效果。[空间阻挡合同](design/tx-04-19/tx-04-19-native-entity-plan.md#72-client原生空间阻挡与authority效果裁决)及[E2E详细交互矩阵](design/tx-04-19/tx-04-19-e2e-plan.md#51-entity与grid交互的共同判据)包含27种非空Grid与适用Entity类别、阻挡／通行、破坏／连锁、地面效果／机关、正反对照与恢复。当前32个主场景组，其中E19-27—32细化为41个交互子用例，全部未执行；19仍不加入快照间自主移动。

> **tx-04-18 原生应用已完成（2026-11-01），统一E2E 29/29 PASS**：[create／remove／change原生Room／Grid应用及验收结果](design/tx-04-18/tx-04-18-native-state-plan.md#13-最终统一验收结果)。真实Authority的25个场景覆盖27类型／17类、12房形、448格、八门视图、资源／动画／FX／水色与实际墙面选择；10种无效候选、重复／过期、三阶段失败恢复、stop／重连／正常退出及访客持久隔离通过。同一18完成17的原生展示接线调整，未新增单元测试，本轮临时材料已清理。[旧专用表示10/10](design/tx-04-18/tx-04-18-implementation.md)及旧资源重放失败仍保留历史。Entity19／Player20、网络21／整世界Applied及29全内容组合均不由本次域成功代替。

> **tx-04-14 已完成（2026-10-31）**：[整局、随机选择、HUD/独立表现、Mod及真实StageAPI实现与统一验收](design/observation-expansion/tx-04-14-implementation.md)。6/6 PASS、21/21场景、84个指定四域scope；[记录/适配合同](design/observation-expansion/run-mod-schema.md)。下方早期计划状态保留原时点。

> **tx-04-13已完成（2026-10-31）**：[探索、离房保存与玩家跨房观测](design/observation-expansion/tx-04-13-implementation.md)已通过统一E2E 5/5，31/31场景、124个指定正式scope；不受影响的已完成项目复用，失败和边界如实保留，临时材料已清理。

> **tx-04-11 已完成（2026-10-30）**：[完整Grid实现与统一E2E结果](design/observation-expansion/tx-04-11-implementation.md)。27种非空类型、17个派生接口及保存／资源／变化／生命周期观测已交付；最终5/5 PASS，25条完整Grid后态，Grid Partial为0，临时材料已清理。下方历史计划保留原时点。

> **tx-04-10 已完成（2026-10-30）**：[公共接口、域拆分、完整 Entity 观测与统一 E2E 结果](design/observation-expansion/tx-04-10-implementation.md)。最终 5/5 PASS，14 条完整 Entity 后态含 4 条四组八身体后态；采集与录制独立，v1/v2 同读取投影通过，测试临时材料已清理。Grid 全量及其他域仍待 tx-04-11—14，下方历史时点不改判。

# C/S研究与状态同步文档

> **tx-04-17 已完成（2026-10-31）**：[公共展示／资源运行时实现与统一E2E](design/tx-04-17/tx-04-17-implementation.md)，最终10/10 PASS。独立Scene／Camera／HUD、多pass资源与完整姿态、候选原子发布、CPU／GPU在途退休、Client玩法／Lua／访客进度隔离和正常退出已实现；第四次仅续验Mod持久与退出，复用8项不受影响的通过结果。必要剩余逆向由一个GPT-6 Luna/max完成，主智能体独立回读并纠错，[主审补充](design/tx-04-17/tx-04-17-reverse-supplement.md)。失败事实保留，临时测试材料清理；18—21的具体全域应用与网络世界闭环不预记完成。

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
