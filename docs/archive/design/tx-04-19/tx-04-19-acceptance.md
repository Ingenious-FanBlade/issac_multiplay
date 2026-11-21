# tx-04-19 I34—I35：统一验收与后续交接

日期：2026-11-04，Asia/Singapore。状态：**I34统一验收与I35交接完成**。执行范围是[实施计划](tx-04-19-implementation-plan.md) I34／I35：产品及 E2E 构建、18 全量 Room／Grid 回归、19 全类别真实 Source／冷 Client、E01—E26 归约、实际画面审阅、成本与后续交接。前序实现存在不等于本次运行通过。

本次会话执行中用户追加授权：“之后直接连续测试即可，不需要问我”。后续同tx启动、修复／续测不再逐次问准备确认；仍在每次启动前提示只跳过开场、到标题／选档页停止操作。该会话指示优先于本次执行的逐次确认安排。

## 实际执行与修复

| 顺序 | 事实与处理 |
| --- | --- |
| 构建 01 | 沙箱内 MSBuild 停在文件扫描，已中止；没有启动游戏。改用构建所需执行权限继续同一入口。 |
| 构建 02 | 产品构建生成 Hook 清单失败：前序新增的 20 个自有 Entity draw／shadow／post／paused／owned-final／SFX Hook 缺少入口特征登记。保留生成器拒绝行为；先核对本地 Wiki／官方 ZHL，再由指定 GPT-6 Luna max 补缺口，主智能体审阅后修复。没有运行分项测试或游戏。 |
| 构建 03 | 主智能体独立回读全部20处入口／返回指令后补登记，生成器通过（173／192行）。产品编译暴露三个新来源文件缺少 `ObservationSubject` 声明；补入实际声明头 `game/world_sources/lua_entity.h`，继续本次整体构建。 |
| 构建 04 | 产品DLL成功；E2E首次完整编译发现 `client_e2e_native.cpp` 的新增inspect经资源模型头引入C++20协议／参与者类型，和C++17 SDK边界冲突。原生检查改为直接生成自有RapidJSON文本，字段／正式adapter只读检查不变，不修改SDK或降低核心标准。 |
| 构建 05 | E2E Dummy来源桥误用只有Lua公开包装的 `Room:GetCenterPos` 为C++方法，SDK无该声明。改为在已核当前Player真实位置生成并检查Player存在，不猜新原生入口；不改变正式Factory／Init／Publish责任。 |
| 构建 06 | 修正harness的int64比较时多写一个右括号，Rust编译明确拒绝，未启动游戏；删除该多余括号后继续同次构建。 |
| 18 Source 01 | 产品／E2E DLL及harness构建成功，真实启动后因无法取得游戏前台焦点停止，尚未进入场景采集；三项环境清理均无错误，`evidence_error=null`。用户说明刚才出现小问题并明确继续测试；未据此修改焦点保护或游戏逻辑，直接同tx重新启动Source。 |
| 18 Source 02 | 全量49／49场景、2／2自动检查通过，未复用历史Source；正常返回菜单／退出，三项清理无错误，`failure=null`、`evidence_error=null`。随后在本次入口继续独立冷Client；Source通过不当作Client通过。 |
| 18 Client 01 | 全量49／49原生提交／绘制、12／12自动检查通过，正常退出、三项清理无错误，`failure=null`、`evidence_error=null`；Source／Client画面随后另行主审。 |
| 19 Source 01 | baseline／npc／npc-change三份真实目标导出；关系变化动作后主线程控制口超时（`main_thread_timeout`），三项清理无错误。静态发现夹具保留旧 `n.Child=child;child.Parent=n` 时又反向设Parent／Child，会构成无根Parent链；改为先解除旧方向、再建立反向树，仍覆盖合法双向Parent／Child引用。没有调用栈见证，不将具体卡住函数写成已观察事实；不改变正式观测或应用器，不把失败样本算通过。 |
| 19 Source 02 | 修正关系夹具后完成14份来源，已越过原失败场景；laser-straight导出因 `EntityLaser.GetHitList:decimal_range_or_type` 拒绝。REP Wiki标题写int、正文写array；官方 `LuaInterfaces/Entities/LuaEntityLaser.cpp:114—127` 明确返回整数table，实际来源为合法Empty table。目录生成器采用类型纠正为int[]，保留原Wiki签名／documented_returns并附官方来源；不删字段或吞掉错误。18夹具没有Laser及该成员的消费者，本次49／49回归不受影响。 |
| 19 Source 03 | GetHitList修正后直线／圆形／曲线／sample四份Laser来源均成功，累计18份目标；laser-chain未取得新的完整同窗口token而超时，三项清理无错误。原编排忽略 `SourceSnapshotToken` 已有第二返回值，新增限频capture_pending域／字段原因及超时附带诊断；不修改Complete条件。Source日志另有Bezier少于两项断言，具体关联及链式来源首因待诊断，不能猜成已修复。 |
| 19 Source 04 | 限频诊断确认laser-chain的Entity持续Partial（`field_or_budget_gap`），主线程仍响应，三项清理无错误；该域总原因尚不足以定位成员。编排在等待10秒后复用既有只读正式出口保存Partial逐字段诊断，不把该出口结果记作19场景目标；继续保留90秒完整窗口等待上限。 |
| 19 Source 05 | 仍完成18份来源后在chain停止，三项清理无错误；只读Partial诊断定位 `EntityLaser.BounceLaser` 为Unresolved，当前库存没有第二Laser。Player `IsLocalPlayer` 的既有NotCaptured为已声明原版Online隔离字段，不是本次新增的必需gap。保留失效引用，不以Null或删除字段替代；继续修正Source的真实链构造及非退化sample。 |
| 19 Source 06 | 新增真实场景判据后，前5份来源成功，在dead场景提前发现原夹具只有活NPC／死亡Effect／掉落，没有dead-but-existing主体；未计作E02／E17通过。三项清理无错误；本次尚未抵达新激光夹具。按本地Wiki `EntityNPC:KillUnique` 的独特死亡动画用途，先在NPC场景创建真实Dip，再对同一Key调用该公开方法，运行时继续要求IsDead且Exists。 |
| 19 Source 07 | 同一Dip Key90已采到IsDead=true／Exists=true，越过原失败并完成8份来源；随后Delirium变形实际仍为BossType0、TransformationTimer1、通用NPC State1（Appear），未采到目标变形，三项清理无错误。REP Wiki及官方 `LuaDelirium.cpp` 说明Transform只设定下一AI帧的请求；夹具改为等待实际BossType20后再取新完整窗口。另外修正harness数值助手及Delirium／Slot比较，使正式int64标签按其真实值解释，保留严格的实际后态判据。 |
| 19 Source 08 | 已实际变为BossType20，Type仍412，同Key95的shape revision为2；但内部Init不在现有NpcMorph owner内，五类原生补充均NotCaptured，Entity持续Partial，三项清理无错误。定位到正式生命周期来源缺口，不能清空payload或放宽所有未知Init。必要静态研究与主审核定专属49F3D0 owner，按下列失败模型补接。 |
| 19 Source 09 | 新owner接入后Delirium变形的正式完整来源可得，越过五类NotCaptured；解析随后以Angle=130报signed_field_range，三项清理无错误。REP Wiki标题int8但正文明确0..255，官方Lua getter与SDK均为uint8，本地adapter已按uint8存储。目录采用范围修正为uint8并保留Wiki原签名／documented_returns，正式解析增加0..255边界；不改来源值或放宽成任意整数。 |
| 19 Source 10 | Angle修正后前18个真实场景全部越过；sample已实际有8个不同optimized点，变形完整来源及Dip双真判据通过。chain根束实际有bit19、MaxDistance700、终点约(600,376)，但GridHit=false、BounceLaser=Null且无第二Laser；新实际链判据明确失败，三项清理无错误。夹具显式恢复公开GRIDCOLL_WALLS，并等待真实存活子束后才冻结；这只是公开来源配置／等待修正，是否足够由后续实机决定，尚不把碰撞类说成已证根因。 |
| 19 Source 11 | 已通过真实反弹子束／引用及clear，累计20份来源；knife导出后只有Player，没有实际EntityKnife，严格类别判据失败，三项清理无错误。原夹具FireKnife返回非空不能证明后续仍存在；按公开资料／源码及必要窄研究修正生命周期，不删场景或放宽类别要求。 |
| 19 Source 12 | Knife三阶段及真实owner变化通过；Familiar换run、Effect、Slot1—18、结果变化、真实Text／专用Dummy来源均越过，累计53份。lifted仍只有Player，完整lifted描述为空，严格类别判据失败，三项清理无错误。公开PickupGridEntity只取得helper；按原版Wiki TryHoldEntity／ThrowHeldEntity接上真实Player持物owner，不把瞬时返回值当成已举起来源。 |
| 19 Source 13 | 完成28份后，familiar-owner的Entity持续Partial，三项清理无错误。控制关系有效，旧／新run的Player实际复用了地址2166591520，Familiar已创建，Player三类表现补充NotCaptured。静态发现非注册Subject的“必须born”条件会继续拒绝复用Player，即使已有当前更新见证；修复只消费项目已采纳的readiness3／current_public_ready／同一有效anchor，不给无见证Player或非Player复用放行。没有补出生、删除Partial或扩大完整Player应用范围；本次尚未到新的举起场景。 |
| 19 Source 14 | 已越过换run并完成29份来源，新Player正式Subject为readiness3。随后effects动作触发原生AV，控制口10054；三项部署清理无错误，evidence_error为save_changes_outside_D18_D20_D27，实际新增范围外文件是该次两个crash_dumps诊断文件，不是已确认的进度破坏。只读dump给实际base410000／PC5E20C0，即静态5D20C0，写null+360；该函数RTTI为MrMeData。统一配置把108.Target错误设成Player，原生cast为Effect得到null后解引用。改用公开UseActiveItem产生正确MrMe／Target关系，191由真实Pickup owner／公开回调产生，57／166单独FollowParent。原失败／附带错误保留，不修改判定或用新成功抹去。 |
| 19 Source 15 | 同址Player及五个Effect变体的实际来源通过，SpriteTrail有两条64B记录，Ghost count2／两份已加载ANM；仍累计53份后停在lifted。TryHold返回true，但正式采样Player held槽为Null，136及描述已消失，仅另一个87 Effect存在；三项清理无错误。必要研究／主审确认TryHold不设置helper Parent，而case136在 `5D548C` 对空Parent直接Remove，故同步补公开FollowParent。拒绝“立即Throw仍采136”的方案：throw会生成Tear40并消费旧helper，不能替代136判据。 |
| 19 Source 16 | 136及跨域描述／变化／落地已越过，累计62份；queries的Sprite调用误把Sprite当成Entity owner，`Subject(Entity expected, got userdata)`明确拒绝，三项清理无错误。正式Query已支持owner与第五参数object分开，夹具改为Query(npc,Sprite,...,sprite)，不修改正式观测或删除Sprite查询。另外在Source正常退出前接既有Win32进程成本读取，和CPU窗口分别记录；未完成的外部进程采样不作为正式峰值。 |
| 19 Source 17 | 两种真实独立Query越过，累计63份；密集128 Tear已产生就绪日志，但单行库存被游戏截断，JSON在第10198字符不完整，harness无法收到ready而超时，三项清理无错误。不是正式出口通过或预算失败。改为每块最多16主体、ready最后发送，harness按同action／label核对块号、唯一性／总数与16K上界，完整重组后再导出；不删实体或降低密集规模。 |
| 19 Source 18 | 65／65真实来源、E26产品／录制故障检查及故障后正式恢复目标均完成，正常退出、三项清理无错误、failure=null；最终evidence_error仍为save_changes_outside_D18_D20_D27。唯一范围外变化是新增documents/screenshots/ManualScreenshot-20.png，与本轮末尾唯一F9解冻同窗口；不修改文件规则或事后改PASS。改为全部场景后使用既有F8解冻，不增加别的按键，重跑完整Source。Source CPU／进程成本已实际取得，但本轮总结果仍失败。 |
| 19 Source 19 | 全65／65真实来源及E26产品／实际录制句柄故障／正式consumer继续发布与恢复目标均通过，pass=true、failure=null、evidence_error=null；正常退出且三项清理无错误。128 Tear密集库存完整分块／重组，没有因日志限额缩减主体。随后进入独立冷Client，Source通过不当作Client通过。 |
| 19 Client 01 | 基线Player支撑创建后后态检查失败，未提交／未展示；三项清理无错误，evidence_error=null。静态定位19描述转17支撑时未填写四个primary引用，缺席默认is_null=false／target0，而Source四项均是显式Null。补准确Null传递，不以默认空掩盖缺失；普通Isaac支撑写前明确拒绝其自身跨对象关系，交20具名owner。原四项后态判据不放宽，失败首因细分值／成员／引用／资源；Source19与18原目标不受这项Client转换修复影响，继续复用。 |
| 19 Client 02 | 原Player后态检查已通过，随后在提交前的阴影验证发生AV，控制口10054；三项清理无错误，范围外变化是该次两个crash_dumps，evidence_error如实保留。末状态grid_commit不是具体崩溃函数；只读dump／自有PDB映射到DrawImpl的RenderShadows，静态PC74C53E读取非法Src0x1C。415541先设置Image shader，415581开启跨Entity循环batch；新增逐实体析构调用9E4140，原生会清shader，后续4157C3的默认阴影quad继续依赖它。不能盲填默认shader或修改batch私有位；继续核定队列是否持有ANM选择器及正确提交边界。 |
| 19 Client 03 | 基线已真正提交／原生检查／绘制；重复T检查以native_executed_prefix增长误报owner重建，三项清理无错误。实际attempt始终1、资源候选加载始终3，prefix713→716→720→724与正常presentation_draw3→6→10→14同步增加；该prefix不是构造计数。修正为保持attempt／资源加载不变，并每次重复T重新读取原生Key／instance／shape／类别／存在性及完整字段资源后态，要求和初次实际库存一致；不放宽真正幂等要求。 |
| 19 Client 04 | 基线完整通过，NPC已实际提交／检查／绘制；导入NPC独立记录时，114条中第10索引是构造期间真实Partial，正式入口按coverage拒绝，已接纳前缀10条，三项清理无错误。原harness错误要求整批全接纳。保留正式拒绝及原始Partial，不过滤或提升覆盖；编排核对拒绝项确为Entity非Complete、A不变／presented=0，随后只复制未处理原始后缀继续经正式入口；遇背压明确记未接纳余量，不用假呈现清队列。 |
| 19 Client 05 | 完成23／65份实际提交、类别后态、重复T及draw后，在pickup被resource_animation_frame拒绝（Key136）；failure不为空、三项清理无错误、evidence_error=null。真实价格Sprite使用停止态-1全局phase及独立数字层9／9／10，原负值校验过严；采用下述有界哨兵处理，保留其他错误拒绝。 |
| 18 Client 02 | 通用资源哨兵修复后重跑49／49与12／12自动检查，pass=true、failure=null、evidence_error=null，三项清理无错误。主智能体再次审阅全部49对画面，原Room／Grid结构与既有Player／HUD／水光相位边界一致，未发现新缺失。继续复用Source02，随后启动19 Client06。 |
| 19 Client 06 | Pickup价格Sprite及27份实际后态／画面完成，familiar在写前计划以entity_class:x:missing拒绝，三项清理无错误、evidence_error=null。目录把静态GetOrbitDistance(Layer)错误规范成实例属性OrbitDistance，派生查询NotScheduled遮蔽真实Vector。先读原版Wiki73／215行，确认两种独立语义；修复前失败模型是带参或static getter不得与字段共用ID，真实属性缺失仍拒绝，静态按层查询不能当字段补零。生成器仅将无参实例getter合并属性；当前目录受此规则改变的只有这一行。独立静态查询作为无Client消费者逐行列入省略清单，真实OrbitDistance仍保留／安装。需要重采19全部Source及冷Client；18没有Familiar，此修改不影响已通过的18回归。 |
| 19 Source 20 | 新目录下完整65／65及E26均通过，pass=true、failure=null、evidence_error=null、三项清理无错误。七个Familiar真实OrbitDistance均为Vector；变体68为35／30，其余本阶段为0／0，静态按层查询不再遮蔽字段。作为后续冷Client的完整正式来源。 |
| 19 Client 07 | Familiar已真正提交、逐字段／辅助资源后态、draw和重复T通过，累计28份；familiar-owner新run重建的退休阶段发生原生AV，三项清理无错误。只读dump基址410000／PC61A0D2，对应静态60A0D2写null+1640；PDB对应Retire209。范围外新增该次dmp／txt，evidence_error如实失败；未确认持久进度破坏。主审官方SDK发现e.Entity::Remove与ClearReferences仍经naked wrapper调用虚表+28／+44，基类限定不能实现注释承诺；全图解除已清空Familiar._player，实际派生Remove仍写Player状态。Initialize中的e.Entity::Init同样走虚表，四类“只进base”承诺也未实现。继续由同一指定Luna max补查，主审回读后修复。 |
| 19 Client 08 | 真正base调用后Familiar换run退休／重建、Effect五变体、Slot1—18／结果变化、Text／专用Dummy、136描述、资源／水面／Query均越过，完成63／65；dense在写前以EntityTear.TearIndex:native_width拒绝，三项清理无错误、evidence_error=null。Wiki称int、SDK `_tearIdx +450` 也为int32，但本轮真实公开getter对128条Spawn Tear全部返回4294967295，不能清0或拒绝完整32位模式。原普通FireTear值0已通过。修复前失败模型：保留0至UINT32_MAX完整位模式，溢出／负的非公开表示仍拒绝；不把该计数当本地实体身份。字段表改为同宽uint32写／读，既不改变字节布局也不解释未知语义；Source20保留，可只重跑Client。 |
| 19 Client 09 | 全65／65原生提交／逐类后态、重复T（dense20次）、独立记录拒绝／背压完成；E26恢复T实际应用越过。负向pool-budget因pretty JSON达45,213,938B，先被E2E32MiB文件上限拒绝，没有命中所要求的类别容量，严格断言失败；三项清理无错误、evidence_error=null。改为只移除格式空白的compact序列化，保持全部目标字段、完整计数和既有上限。主智能体审图另发现familiar-owner新run的Burning Basement起只有HUD与黑红FX，房间／实体不可见，差值约50，不能把65份后态成功冒认为画面通过；该视觉问题独立修复，必要窄研究继续由同一Luna max承担并由主智能体复读。 |
| 19 Client 10 | 全65后态、16个非法目标（池用例实际命中NPC512容量）／2版本协商拒绝、实际独立Query协议与16／16故障恢复、四阶段stop均通过。暂停等待发现此前故意无用途调用native exercise已触发正式client_dispatch_unexpected_entry而停显；暂停并未实际开始，三项清理无错误、evidence_error=null。原18编排此处直接stop，19错误地沿用Failed场景去测暂停；修正前失败模型为非法调用不得穿过原链，明确检查exercise=1／原调用计数不变／进度拒绝增长后，正常stop与完整T恢复，再测暂停，不放宽产品dispatch。另一次E2E GPU诊断在负颜色场景得到floor780×468中心RGBA0/0/0/0、walls同尺寸29/17/10/255、color866×632为5/3/2/21、light433×316为214/216/223/255；getter返回FBO／viewport未变，只是一点采样，不把透明中心直接认作整个缓存为空。视觉问题仍待解决。 |
| 19 Source 21 | 新presentation v4完成65／65及E26，正常退出、三项清理无错误、failure／evidence_error均null。但随机restart开到普通Basement，换主人后只有pass4、没有原黑场的Burning云层；不能作为该修复覆盖，也不能运行要求generic层的新增负向测试。未启动对应Client；夹具按本地DebugConsole及StageType文档采用公开stage 1b，在真实新房就绪后建立Familiar，并要求实际id4／pass1／blend_mode1，然后重采。 |
| 19 Source 22 | 固定真实Burning分支后完整65／65及E26通过；familiar-owner实际presentation v4、云层id4／pass1／blend_mode1，另有pass4照明层。failure／evidence_error均null，三项清理无错误，正常退出。作为最终冷Client11的完整正式来源，没有用XML或夹具填补观测值。 |
| 19 Client 11 | 全65原生后态／重复T、18非法目标（新增FX缺值／范围拒绝）、2版本协商、实际独立协议、16故障恢复与四阶段stop通过。Burning房间／实体正常显示，主审发现E14举起岩石仍未显示，不能据descriptor／pose读回宣告画面完成；Source的136不可见、由Player持物绘制消费，继续补最窄依赖。E20已正确恢复committed但暂停等待超时，ui_blocked始终0；静态复读ClientUiBlocked只看REP Console状态，原17用CONSOLE打开／Esc关闭，19错误地用Esc打开。修正编排并增加暂停期间两次连续draw及A不变检查，产品策略不改。正常退出、三项清理无错误、evidence_error=null，未据自动前缀宣告最终通过。 |
| 19 Client 12 | baseline在plan阶段以player_body:ordinary_Isaac_resource_required写前拒绝，尚未原生应用；普通场景的公开BodySprite实际Null，强制非空是错误前提。资源模型本来支持Nullable ANM，去掉该额外强制，继续按完整来源安装／清空Body；不能让先前LiftItem资源残留到普通目标。正常清理、三项清理无错误、evidence_error=null。 |
| 19 Client 13 | 最终完整65／65、20非法目标／2版本拒绝、真实独立协议、16故障退休／恢复、4阶段stop、真正CONSOLE暂停／持续draw／Esc恢复及活跃Client退出全部通过。failure／evidence_error均null，三个cleanup均null。主智能体已审阅全部65对／17组画面，原分辨率确认136岩石、Text及Laser；Player身体／HUD差异按下文范围保留。 |

基类入口修复前失败模型：不能保留／伪造Player指针来迁就派生玩法Remove；不能改vptr或绕过已安装Hook直接调用原始跳板；不得跳过官方Plus reset或ESSM配平。19完整adapter应直达清单已登记的base入口，原有Hook／官方链仍常驻执行；Init中的LoadEntityConfig／ResetColor仍按其实际虚调用解释。Familiar先撤引用和自有附属owner，再调用真正base Remove／Clear，其他未知前缀继续保留隔离。修改只在19完整类别adapter，18代表路径不改；Source20无Client调用，可继续复用。

Luna补查与主审一致：不是Player先销毁；全图Unlink先清 `_player +410`，Familiar派生Remove60A040对变体68／78／243于60A0D2／DA写Player+1640／1644。主智能体独立复读SDK wrapper、dump栈与自身PDB、原始指令／ABI及vtable。真正base Init为6A4A20（ret10h在6A4F25），Remove6A8000（ret在6A8057），Clear6A8060（ret在6A8209）。Remove不向本体虚派发；Clear仅可能虚Remove+310附件，该槽已先撤。四类vtable的+60均为6A7CB0（只复制颜色，零callee），+64均为6A4F30（配置装载／字段与资源处理，无Entity虚派发），不会回到派生Init／Remove。采用 `Base+RVA` 调用当前清单已有entry；没有增加Hook、改变vptr或直调原始跳板。此前文档“基类限定即可跳过派生”的采用解释是错误的，作为失败历史保留，以本次修正覆盖；静态核定不当作后续运行成功。

续测前静态审阅还发现原pool-budget负向夹具增加513个NPC后未更新完整成员计数，会先命中成员不一致。修正前失败模型：该用例必须具有合法完整计数及唯一Key、实际触达类别容量拒绝、不得开始原生attempt或破坏A。编排同步count／native_count，明确要求拒绝首因为 `entity_budget:native_pool_category=1:capacity=512`；不改变产品预算或降低负向要求。Source20运行中只纯构建新Rust编排器，Source正常退出后才替换下一次启动使用的exe；产品DLL及Source本身未改动。

Client02的必要研究／主审纠正了I25的逐实体flush假设：官方ANM2.zhl指向 `405A50 RenderLayer`，它同步按selector选帧并调用 `404570`；`67E833—67E856/67E8ED—67E8F3` 把4份顶点字段复制到目标缓冲，正式KAGE队列保存Image／Batch／vertex／index数据，不保存ANM选择器或Entity标量地址。故原调用返回后可恢复选择器，Image及shader资源owner仍须活到外层drain。`9E4140`会清shader且 `9EFA70`会清活动batch标志／缓存，不能仅保存shader修补中途Present。已移除逐实体Present，沿用外层RenderState排空；整帧scope退出若仍有CPU队列则保留owner／隔离，不能提前释放或假交还Off。未知原生异常的原保留规则不变。该scope只在19 full-Entity表现时生效，18代表路径无此逐实体flush，已通过回归不受该修复影响。

### Delirium来源owner修改前的失败模型

- 普通AI返回而未发生Init：不得产生Morph／换代，或给未捕获字段补资格。
- 同址新use／新Key、退休、跨run／outer、重入或错误线程：不得把旧scope授予新对象。
- Init只有外层Hook返回而没有原生body完成、异常、helper未正常返回：保持不可读，不补出生。
- 观察停止／更换context时仍有在途owner：保留原context至scope收口，不能恢复已停止来源。
- 该helper之外的未知Init继续采用原失效规则；新owner只覆盖已观察出生的Type412同use。所有新调用仍由唯一Hook清单注册、启动常驻，原调用恰一次。

## E01—E26最终归约与画面范围

本表是Source22／Client13的原生后态、编排检查及主智能体画面审阅的联合结果；18回归采用本tx内的Source02／Client02。通过的是下列具体场景，不是每个成员所有取值或全部内容组合。

| 项 | 实际覆盖及结果 |
| --- | --- |
| E01 | 65个连续完整T均原生提交，普通目标重复2次、dense重复20次不重建owner／资源；旧revision／同revision冲突写前保A，final-clear清空非Player。仅提交非Player与普通Isaac支撑。 |
| E02 | npc、change、relations、clear-relations、dead-existing、morph：冠军色、部件Parent／Child回链、姿态与真实Dip死亡仍存在；原生后态与画面通过，没有Client AI／伤害／奖励原调用。 |
| E03 | Delirium及真实BossType20变形，同Key／shape换代、专用字段／资源和画面通过；不在Client执行变形AI或重新抽Boss。 |
| E04 | Tear／Projectile及变化：高度、flags、轨迹值、资源与姿态通过；128条Spawn Tear的UINT32_MAX TearIndex按原位模式保留。 |
| E05 | Bomb、change、remove：真实倒计时／颜色／资源／服装与删除通过，Client不引爆／破Grid／生成掉落。 |
| E06 | 直线／圆／曲线／非退化sample／真实TEAR_BOUNCE墙面链／clear：几何容器、活子束引用、端点与画面通过，清空后无旧束残影。 |
| E07 | Knife挂载／出刀／负速度返回及NPC owner变化通过；不使用普通无Knife武器Isaac作会被原版立即删除的父级。 |
| E08 | Pickup、cycle、Morph、remove：价格99、停止-1 phase及独立层、底座／物品／循环最终量通过；无本地支付、重抽或购买。 |
| E09 | Familiar1／8／48／68／78／231／243、真实新run Player Key变化与退休重建通过；Body／复合draw及OrbitDistance真实Vector通过，231当前附属组为空。 |
| E10 | Effect22／57／108／166／191及变化通过；108由真实道具owner产生，191真实count2／两Sprite，166真实记录序列，地面／透明与子资源层次可对应。 |
| E11 | Slot1—18每种至少一份真实适用后态／资源／画面通过，没有捐款／支付／开奖。 |
| E12 | 多Slot结果／奖品字段与destroyed目标通过，最终对象唯一，Client不继续奖品裁决。 |
| E13 | 真实damage writer产生NativeText，Font6的dmg1.00可对应；专用池的真实NativeDummy分类／实例／=964.0显示／退休通过。Dummy自然gameplay caller仍UNKNOWN。 |
| E14 | 真实136举起、描述变化、删除及最终活动Grid通过；同Key／shape／Parent、完整32B描述、BodySprite/null-frame与持物画面已补齐，无第二活动Grid或虚构source_grid。最终落地Grid由显式E2E生成，不称自然抛物落地全流程。 |
| E15 | 合法Parent／Child双向引用图（Parent链保持有根）、换向／Null、Knife前置与后置关系、held稳定Key通过；缺依赖／跨类型／held错误类别写前拒绝，补完整T正常。构造前置按现有拓扑计划，不把普通后置回链当必须拒绝的构造环。 |
| E16 | NPC同Key Morph、Pickup换资源、Player owner新run、原生实例替换／删除／重复T通过；Source15同地址新Player当前资格失败与修复保留。身份／实例／资源不因池复用复活旧use。 |
| E17 | 同采样短命create→remove、真实dead-existing及独立旧phase记录通过；真实Query重复／scope冲突／Partial拒绝不改普通A，背压保留未接纳后缀，presented始终0。 |
| E18 | 65对Entity加49对Room/Grid画面主审完成；主／overlay／层／辅助ANM、shader查询保留，阴影／light／水反射／柱深度／Burning云层通过。镜像由18清单回归覆盖；非空自定义shader包及全部类别镜像组合未宣称逐项运行。 |
| E19 | Acquire／Init／Fields／Resources／Publish／References／Check／Draw前后共16个解释过的正常返回边界注入，逐次命中一次、停显／准确前缀／安全退休／完整T重建通过；不把注入说成真实原生异常恢复。 |
| E20 | 各场景原Gameplay计数不增长；故意非法native exercise令正式dispatch停显，6次native／6次Lua持久入口被拒绝，正常退休／重建后真实CONSOLE打开、至少两次持续draw、Esc恢复通过。Source Host／Client／Off原链与普通／odd／FSM等保护按现有隔离入口核对，没有四端实时游玩声明。 |
| E21 | prepare／native／grid／display四阶段stop及活跃Client窗口退出通过；借用／线程／部署正常收口，最终三个cleanup均null。Client stop只新增日志；shutdown配置仅同内容时间戳、日志变化，未恢复用户存档来制造通过。 |
| E22 | 20非法目标均写前保A／native_attempt不变，包括真实NPC512池容量、字段失败／截断、shader失败、Player超范围、FX缺混合模式／范围3、held缺owner／错误类别；另2项正式model／catalog版本协商明确拒绝。 |
| E23 | 本tx实际重跑18 Source49／49、冷Client49／49及12检查，通用-1资源修复后又完整冷Client回归，49对画面再次主审。随后仅19 full-Entity／generic FX／Player窄支撑改动不进入18旧代表路径，按实际影响复用。 |
| E24 | 65份同token原始观测／当前投影、组件内图闭合、真实独立Query及四个shader查询保留通过；每份复核190个省略成员ID，动态层／pose不因路径相同被合并。原始观测与当前投影差不冒称P1—P6实施前后净收益。 |
| E25 | dense128 Tear、复杂Laser、多Sprite及稳定重复20次通过，真实NPC类别容量拒绝；预算／应用墙钟／代码窗口／进程峰值见下节。未将所有池容量当满池实测，未宣称10Mbps实时下行。 |
| E26 | 产品／E2E分离及产品默认录制／详细诊断关闭通过；65份不依赖录制，实际FILE_ERROR后正式consumer继续、恢复T在Client应用；正常退出与临时部署恢复通过。 |

最终65对已按全部17组逐一审阅，并原分辨率核对Text／136。Burning黑场、持物不显示、旧价格Sprite拒绝均已在受影响场景得到修复后的实机覆盖。仍有明确跨域差异：Source无敌冷却的身体闪烁、Player自体伸缩／动作细节及完整形态归20，active-item／Boss条／初始楼层标题等HUD归27，FPS及水／光／小地图时钟相位不作像素相等判据。136岩石本体、最终Grid、非Player资源／几何／层次已对应，不能据这些画面声称完整Player／HUD已交付。

## 必要静态逆向与主审

一个指定的 GPT-6 Luna max 子智能体补查本次缺失入口。先读本地原版 `Entity.md:518—541`、`SFXManager.md:51—54` 及 REPENTOGON Wiki，再对照官方 `libzhl/functions/Entity*.zhl`、`SFXManager.zhl`，最后读取 J273 静态指令。没有修改 IDB、运行游戏或将静态事实写成运行观察。

主智能体独立批量回读全部20处函数起点、入口32字节与返回指令，并审阅子智能体结论。11个派生 Render 和4个 Shadow 均为 ECX=this、一个指针栈word、`ret4`；两个 PostRender 无显式栈参；`6F92A0` 为无显式栈参的暂停查询；`647310` 为 this+两个指针栈参、`ret8`；`907C60` 的 SFX Play 为官方具名顺序的六个栈word、`ret18h`。短入口特征登记在 `mod/binding/v1_9_7_12-static.json`，SEH／cookie地址及call相对位移通配，仍由唯一Hook清单生成器使用。

主审明确排除两类误判：NPC／Dummy 的共享EH尾 `412B0E ret` 不是普通ABI返回；IDA推断的 userpurge／fastcall 或SFX int返回不能覆盖官方声明与实际调用／返回约定。Dummy单指针的具体语义继续复用既有I20—I25绘制合同，本轮入口登记不扩大其支持范围。运行安装、原调用次数及画面仍由本次统一E2E判定。

## 最终验收状态

早期Client05资源修复事实：真实价格Sprite为NumbersWhite、停止播放、全局phase/frame=-1、独立层selector=9/9/10。原校验一律拒绝负phase；修复只允许停止态精确-1，保留其他负值／播放负值／资源及selector错误拒绝。依据官方AnimationState.zhl及已审Render：先选动画有效0帧，再复原精确哨兵及已验证selector，不向原生SetFrame传-1，不用Update制造姿态。该通用修复已补做18全量冷Client回归，49／49与12检查及画面通过。

本次最终采用18 Source02／Client02、19 Source22／Client13：18的49／49来源、49／49原生应用与12检查；19的65／65完整来源、65／65原生应用／后态、20非法目标／2版本协商拒绝、16正常返回故障注入／安全退休／重建、4阶段stop、真正控制台暂停／持续展示／恢复及活跃Client退出全部通过。最终两端failure／evidence_error均null、三项清理无错误。主智能体已逐组审阅49＋65对画面，另原分辨率复读Text、Laser及136；E01—E26按下表归约。自动报告的visual_pending只是编排边界，最终人工审阅在本文明确记录，未篡改原始自动结果。

产品继续使用唯一ClientScene／parser／plan／结果与退休路径，最终non_player_entity_applied=1、player_support_applied=1、full_entity_applied=0。普通Isaac支撑和非Player应用范围分别声明；完整Player归20，实际网络下行及Applied ACK归21／22，独立音画消费归27，整局覆盖归后续任务。

18本次回归已完成Source49／49、冷Client49／49及12／12自动检查；主智能体已逐组审阅全部49对画面，并额外原分辨率复读Crawlspace。Room／Grid几何、完整成员／资源及必要层次可对应；Crawlspace上下边缘砖、Greed附属门与TimerPlate无旧缺失。没有逐像素相等声明：Source无敌冷却导致身体闪烁、初始楼层标题、FPS、水／光／地图时钟相位的跨域差异沿用18已明确的Player20／HUD27边界，不能据此宣称完整世界画面通过。

Source05的Partial逐字段诊断已定位：`EntityLaser.BounceLaser` 为Unresolved，而第二Laser未在当前正式库存。不能把该引用清成Null或删字段取得成功，须修正Source链的真实生命周期。另逐值检查发现sample夹具的51个点全部重合、Bezier仅1项；它不能作为非退化sample覆盖。已续派同一GPT-6 Luna max补查公开资料／源码仍未解释的mode2及链生产条件，主智能体随后审阅，不把先前成功导出当作该功能通过。

主智能体独立回读 `950B90 → 94C940`：`BAB120` 的128位原值为 `0x80000`（与本地TearFlags Wiki的bit19 `TEAR_BOUNCE`相符），未满足该位时94C940会对既有 `+4E8` 子束执行虚Remove。原生创建分支会登记Bounce引用、子束 `+4E4=1`、`+52C` 反向关系及Parent；因此不采用手拼裸Bounce指针。新Source只设置公开flag与足够MaxDistance，让实际击墙创建该链。

同函数 `94D1CD/94D211` 按 `+4D4` 分派：HomingType1／2对应 `974C60` 的内部模式4／5，TEAR_HOMING位另有优先分支。主审 `974C60` 的候选逻辑确认mode5额外纳入 `Type==1 && entity!=parent` 的Player；旧单Player-Parent夹具没有这一目标。新Source用真实NPC作Parent、mode2、CurveStrength0，保留当前Player作为目标；不增加新身体、改原生模式或补造采样值。运行判据要求至少2个不同采样点及至少2项Bezier；chain须引用另一条实际存活Laser。

同一Luna max子智能体随后独立确认上述分支，并给出 `94D1A5` 的TEAR_HOMING优先路径、`94D1D3—94D1E7/94D1FF` 的模式4／5及调用、`974EC1` 附近的Player排除Parent条件；主智能体已与自身读取相互核对。采用的是这些具体生产条件：CurveStrength0只是本夹具配置，不冒称必要条件；MaxDistance700只是促成真实碰撞，不能替代运行中已生成Bounce的判据。NPC失效／死亡时的候选不由该静态结论保证。

链生成的进一步窄研究与主审：`94E764—94E7AD` 每6度查Grid，再用Entity `+184` 碰撞类经 `7068A0` 筛选；class0始终不接受，class3接受Grid碰撞类4／5。没有候选偏移时 `94EC64` 跳过创建，非零时 `94EC8F` 调用真实创建owner并在 `94ECD8` 登记子束。旧通用keep把Laser碰撞类也清0，阻止了这条路径；Source11恢复公开GRIDCOLL_WALLS后已实际采到存活链，不能将GridHit作为该路径的必要判据。

Knife先查原版Wiki的FireKnife／Shoot／Reset、REP公开距离／速度接口和官方包装，未找到生存条件；同一Luna max补窄研究，主智能体独立复读：`7A5080`只创建及配置，CantOverwrite仅设FLAG_DONT_OVERWRITE。`93AAF0` Update经 `93F650` 沿Knife父链找master；Variant0且master.Parent为Player时，在 `93ADE7` 检查 `5B3500(parent,4)`，失败于 `93AF08` 调虚Remove。该helper读 `+13EC+index`，不把匿名字节冒称官方成员。有效Type10 NPC Parent跳过Player／Familiar特定检查。夹具采用真实冻结Gaper Parent，另一个Gaper覆盖owner变化；出刀用公开Shoot，返回等待真实负速度／非零距离，不再以Shoot(0,0)代替返回。静态结论不保证NPC父级下自然返回，仍由后续实际阶段判据验收。

Source12已实际取得同一Knife Key137的挂载／伸出／返回：距离0／53.1608／139.804，速度0／17.6643／−5.33568，IsFlying为false／true／true；Parent由Key136变为139。故该NPC父级样本下自然返回已实际覆盖，不外推所有父级／变体。

Source13的Player复用修复前失败模型：无有效anchor、未正常完成当前PlayerUpdate、Init／撤绑／退休后的旧资格、非Player未观察创建及跨scope旧登记，均不能成为KnownCurrent；只在本scope已有主体且有效当前身体仍持readiness3时接受。该依据来自现行规范第11节，原出生／origin／use／revision不改。修复不改变已经可读的18回归样本，只恢复新run同址Player当前值，不把它说成完整Player装备应用。

逐值复读Source12还发现effects标签未保证每个请求变体仍存在：实际只有22／108／191，57／166已消失，不能将该标签当作五变体覆盖。夹具为需要Parent的Effect使用公开FollowParent，并保留真实target；harness新增22／57／108／166／191逐个存活判据。已有三类nullable owned资源仍按实际Null／空集合报告，不冒称已覆盖非空形态。

后续Effect窄研究与主审纠正统一配置：case108在 `5FCAB0` 调 `5D1EB0`，其 `5D2060` 将Target RTTI转换为Effect，`5D206D`置ESI后无空值检查，`5D20C0`写其Velocity。Source14把Target设成Player是实际AV原因，不是泛称父级或“缺hand”。case57在 `5F0005` 要求Parent，空时Remove；case166在 `5DA281—5DA29C` 对已有帧且无Parent置State99。新夹具57只跟随真实冻结NPC，166只跟随Player；108走公开UseActiveItem，191走REP公开1335／LootList回调及Pickup.UpdatePickupGhosts。所有对象／资源仍由真实原生owner创建，不填正式观测值、不写裸布局。

Source15已直接验证新旧run的Player同址235106320仍能取得完整目标，覆盖了Source13实际失败分支；不再只是不同地址下的成功。effects实际同时存在22／57／108／166／191，191有count2及两个已加载辅助Sprite；MrMe本阶段辅助Sprite仍为Null，按真实状态保留，不冒称全部活动形态已覆盖。

死亡来源的必要窄研究仍由同一Luna max执行，主智能体复读 `6BA4A0`：公开KillUnique经vtable+24调用后，将NPC `+B64` 设17（UniqueDeath）并清 `+410` stateFrame；不直接写 `+172/+173` 或执行Remove。故不能将该入口说成直接保证dead／exists或固定两tick寿命；Dip的真实双真状态由Source07记录确认，静态合同只解释公开动作。

Delirium重配研究／主审：官方 `ASMDelirium.cpp` 的字段写入sig命中 `49FC20`、完成sig命中 `4A03C2`。二者在专属 `49F3D0..4A12B7` 内；`6BF295/6BF297` 是NPC dispatcher Type412唯一直接caller，ECX=this、无栈参，返回值被忽略。两支虚Init位于 `49F61B/49FDAB`，后者使用当前BossType／Variant；配置继续在同helper完成，`4A03C2` 跳至epilogue，唯一正常ret为 `4A11F8`。主智能体独立读入口／caller／返回点，与Luna结论相符。

新 `world_delirium_owner` 为唯一清单中的常驻只读Hook，保存原EAX32且原调用一次。`WorldEntityReconfigurationScope` 强持有采集context；Collector有界记录同一已观察Type412出生的local/use/birth/run/outer。entry仅登记候选，实际BaseInit body首次进入才推进一次shape；Init外层、原生body及整个helper均正常收口后恢复readiness2与当前字节资格。原出生／origin不重建，helper之外未知Init保持原规则；没有Init的AI返回不恢复资格。停止／异常／换use不恢复，owner未退尽不能提交完整采集、打开新outer或退休context。18场景没有该Type412 helper，owner栈为空时原Init／提交／退休条件不变，本次18全量前缀继续复用。

本次按正常完成流程清理临时Source／Client目标、日志、截图、审图材料及仅由本轮生成的崩溃／手动截图文件，不保留tx artifact包。历史失败、修复、实际结果与成本保留在本文；没有用存档恢复伪造Client隔离通过。

I35更新应用目录、能力说明和入口后，统一入口再次完成产品／E2E／辅助工具纯构建，全部已完成游戏阶段直接跳过，没有额外启动游戏或追加分项测试。

## 体积、容量与成本口径

### E14持物显示的必要Player窄依赖

先查REP Wiki EntityPlayer.GetHeldEntity/GetBodySprite/GetSuplexState及原版GetItemState，再查官方SDK／LuaEntityPlayer.cpp。同一Luna max补查，主智能体独立复读：Player Render为784A60、ECX=Player、一个Vector*、78629A ret4；784F6F读取+140C。78558D要求held.Visible=false，785651—7856D1临时置true后调用held的虚Render，7856E2—78572B恢复Visible及临时scale／rotation／alpha。不能把136设为可见来掩盖缺失的持物关系，也不能调用TryHold／Update重演玩法。

主审核正中间位移解释：Player+44C是BodySprite+41C内部的AnimationState+30，不是独立HeldSprite+FF8。4065B0(...,0)从当前Body动画取null-frame；缺失时才按SuplexState尝试主Sprite，均无记录才在78540B关闭持物绘制。Source22已有公开GetBodySprite=LiftItem frame2、null selector0=2，GetItemState=604、GetSuplexState=0、GetHeldEntity=同Key136，所有必要来源已完整。431860公开ItemState getter直接读+15F4，setter在75883E写同槽；Suplex公开getter直接读SDK+1F08。公开枚举只列0—4，但主审78CE75确认原生过渡写5，因此保留0—5已知真实状态而不钳值。不新增私有字段、player_draw v2或Source补采。7C5980是控制器映射只读查询，空表／未匹配返回true；本轮普通Isaac不据此宣称全部控制器／角色显示。

写前核对同T稳定Key、shape、完整lifted与Parent当前Player，当前136采用ItemState604；缺owner／错误类别分别纳入E22并要求具体拒绝首因。普通Isaac支撑经原资源服务安装／核对独立BodySprite的完整pose，预算也计入该资源；主／Body同路径仍是两个真实成员，不能合并。17／18旧代表没有derived，不进入该新增支撑。Render Frame只临时借held裸指针及ItemState／SuplexState，保护Body及136主ANM工作值至外层CPU队列消费完毕后还原；unknown在途仍保留owner／隔离。完整Player角色／装备／其它持物类别仍归20，实际136岩石与描述变化画面已在Client13确认，Player自体动作／伸缩仍按20边界保留。

### Burning Basement前景混合模式缺口

主审先复读本地REP Wiki的Backdrop／Image公开能力及官方FXLayers声明，再核对本地原版`resources/fxlayers.xml`：id4是Burning Basement前景云，配置具有blendMode语义。原生`96CE20:96CF10`在绘制generic层之前读取FX记录`+44`，交给`9E17B0`构造混合模式；后者0为`(0,1,0,1,0)`覆盖，1为`(0,6/1,7,1,7)`，2为`(0,6/1,1,1,1)`，斜线分支来自原生目标模式。原安装只补`+3c/+40/+48`，遗漏`+44`并零初始化，因此不能宣称已复制完整绘制输入。最终ColorModifier shader／48B顶点与原生一致，不能以负颜色值为由清白来源或删掉云层。

修复前失败模型：generic pass1／2／3／6缺混合模式、越界或非整数必须写前拒绝，不能默认覆盖或透明；pass4／5不读取该字段，不采未初始化word。正式presentation来源升级v4，复制实际generic `+44`、解析0—2、原样安装并在draw前核对。旧v2／v3仅无generic层或已显式带该字段时可用。本次18的49份来源经逐值复读均只有pass4，因此复用已通过的18 Source／Client及画面；19须重采65份并重跑Client，不能由XML补填旧目标。E22增加缺字段／范围3两种保A拒绝。二次floor helper `919030`的实际生产条件仍未证明与本黑场有关，没有据waterGfx存在猜标志或补调。Source22／Client13的实际Burning房间／实体及云层画面已通过主审。

同一Luna max独立补核后，主智能体复读四处指令：pass1／2／3／6分别在`96CF08/96CD78/96CC68/96E318`压入record+44；`967DA1`原生初始化明确写1，`969B75`匹配XML属性，`969BB7`调用atoi覆盖。故“未写XML属性”不代表0。本地`IsaacDocs/docs/xml/fxlayers.md`及官方`BlendMode.zhl::GetMode`的premultiplied-alpha说明与原生分支一致。采用真实已构建record的1，未在Client解析XML或将旧来源猜填为1；Backdrop+20继续UNKNOWN。

Source20的65份同token资料：原始观测字节最小4,781,102、中位4,895,931、最大8,899,581；当前replica投影字节最小1,983,370、中位2,044,665、最大3,871,597，最大均为128 Tear的dense场景。这里比较同一来源的原始观测与当前投影，**不是P1—P6实施前后的净收益**；两端顺序运行、离线完整T导入也不能证明10Mbps实时下行。

Source20的 `formal_lua_capture` 共2,134次，总122,097,371,900ns，单次最大503,767,000ns。计数包围单个域的Lua采集（11个域），不是2,134个完整模拟帧，也不能把单域最大值称作完整帧最大值。Source侧draw_prepare为空Client分支，不作为类别绘制成本。Source正常退出前进程工作集830,799,872B、private_bytes809,431,040B；OS `peak_working_set`1,345,945,600B、`peak_pagefile`1,318,604,800B。后两项保留API原字段口径，不能说成Entity独占内存或瞬时private_bytes峰值。另一次外部监视未完成，未采用其输出。

最终Source22的65份：原始观测最小4,769,700B、中位4,955,164B、最大8,857,072B；当前投影最小1,946,562B、中位2,072,820B、最大3,834,637B，最大仍为dense128 Tear。familiar为6,426,782→2,486,403B，实际Burning换主人为6,281,920→2,368,728B。`formal_lua_capture`共2,068次，总116,387,282,700ns，单域最大435,567,400ns。正常退出前工作集830,337,024B、private_bytes810,131,456B；进程`peak_working_set`1,361,481,728B、`peak_pagefile`1,341,054,976B。与Source20保留同一测量口径，不把随机房间、不同tick或版本差额解释成优化净收益。

产品预算由 `EntityCapabilities` 与同一实例池执行器声明：对象／draw成员各16,384、命令131,073、引用及曲线点各262,144、资源4,096／slot16,384、目标32MiB、节点67,108,864、深度64；独立记录128条、合计2MiB、每条256KiB。原生池容量分别为NPC512、Tear512、Projectile512、Bomb64、Laser128、Knife64、Pickup512、Familiar64、Effect1024、Slot128、Text128、Dummy128。容量是明确上限，运行矩阵只覆盖所列实际数量与超限拒绝，不将所有池的容量声明说成满池压力测试。

最终Client13到final-clear时的累计代码窗口如下，只取最后一份累计值，不跨65个检查点求和。它包含本进程内该具名wrapper／代码窗口的全部调用及其中不采用分支，不是逐类独占成本或完整渲染帧时长。

| 窗口 | 次数 | 总经过时间 ms | 单次最大 ms |
| --- | ---: | ---: | ---: |
| codec_baseline | 213 | 29,359.2749 | 331.7832 |
| target_parse_plan | 213 | 10,865.0757 | 274.8992 |
| native_object_step | 2,552 | 301.6901 | 15.5553 |
| draw_prepare | 120,775 | 2,718.3239 | 0.4823 |
| draw_scope_inclusive | 120,775 | 2,865.7361 | 0.4993 |

65次首目标应用含IPC墙钟最小1,161.420ms、中位1,671.855ms、最大16,784.714ms（dense）；两个draw窗口的编排墙钟最小598.023ms、中位671.008ms、最大11,538.023ms（dense，含20次重复T、后态与独立记录控制操作），不能把它称作两帧渲染耗时。65个检查点工作集最大670,281,728B、private_bytes最大651,456,512B；最后检查点分别627,294,208B／605,102,080B，进程OS peak_working_set为1,181,544,448B、peak_pagefile为1,155,313,664B。

cs status的模型预算target_bytes最小2,806,264、中位2,989,700、最大5,430,843；target_nodes最小269,318、中位286,559、最大490,727，最大均dense。此bytes按room_grid_budget的值／字符串预算规则计量，不是JSON文件长度。entity_resource_slots实测2—515、references2—1,930、curve_points最大314（laser-chain）、commands最大914（dense），无静默截断。

formal_lua_capture、codec_baseline、target_parse_plan、native_object_step、draw_prepare、draw_scope_inclusive均用steady_clock记录主线程代码包围窗口经过时间，并非线程CPU计时，可能包含调度停顿；前序“CPU窗口”标签据此更正。draw_scope_inclusive包含draw_prepare；原始观测字节、当前投影字节、模型预算、含IPC墙钟、局部代码窗口与全进程峰值各自报告。

## 具体覆盖限制

- 普通Isaac支撑仅一个身体、四个primary引用明确Null；完整roster、装备、角色／多身体及具名Player自身关系由20负责，19始终不置完整Entity域提交位或发送网络Applied ACK。
- Delirium重配owner限定已观察出生Type412、同use／scope的真实专属helper；835、906／920／921、951:3、965:10等NPC复合draw实现有静态合同，但本轮65份不等于逐个特殊组合的运行覆盖，交29补内容矩阵。Deathmatch状态>=2不采用当前普通布局。
- Familiar已实际包含1／8／48／68／78／231／243；231当前附属集合为空。MrMe108本阶段辅助Sprite为Null；Ghost191实际count2、两份已加载Sprite；SpriteTrail166有两条64B记录。保留这些真实阶段，不把空／Null阶段冒称所有非空形态。
- 四种HasCustomShader／HasCustomChampionShader查询及失败状态保留在投影；本轮没有真实非空自定义shader资源包的正向画面覆盖。未知第三方Mod私有状态不在声明范围。
- Text来源是真实伤害writer；Dummy来源是显式E2E专用池桥，分类／承载／退休需要真实执行，但自然gameplay caller仍UNKNOWN，不用NPC替代Dummy。
- 举起使用公开持物／throw路径，最终落地Grid由E2E明确生成并消费旧helper；验证最终目标及136描述handoff，不称为自然抛物线落地全流程。独立事件只接纳／拒绝／背压与保留，未实际消费音画，不假增加presented或watermark，消费归27。

## 后续消费接口

以下是基于本次实际运行及明确覆盖限制交付的接口与后续责任。

| 后续任务 | 交付接口与仍需负责的内容 |
| --- | --- |
| 20 Player | `ClientEntityOwners::{resolve,dependency,player_ready}` 按稳定Key／shape／实例代次取得真实身体；`ClientObjectDescription` 的普通Isaac支撑不是完整roster／角色／装备应用。补全四参与者、多身体和所有具名Player引用；禁止回退首玩家。 |
| 21／22 状态闭环 | 复用 `ParseEntityTarget`、`PlanEntityTarget`、`ClientScene::AdmitTarget`／`Result`、`ClientObjects::PrepareTarget`／`Commit`／`CancelPlanned` 及正式退休。B／T／最后原生A各自独立；只有Scene提升A。网络Applied ACK仍未由19发送，不能以非Player范围置完整Entity域位。 |
| 23／24 切房换层 | 同scope的Room／Grid／Player先决、Effect136描述及后置引用采用同attempt编排；写后取消先安全退休，再提交完整T。不能把旧房的borrow／cache／独立事件用于新scope。 |
| 27 独立表现 | `ClientEntityEvents` 与 `AdmitEntityIndependent`／`VisitEntityIndependent`／`CompleteEntityIndependent` 保持真正接纳／呈现语义；128条／2MiB未消费记录背压，不伪确认腾队列。消费Authority音画及短命结果，沿用类别资源／draw owner；构造和退休期仍阻止本地SFX重复裁决。 |
| 29 内容组合 | 依据本次实际逐类／Slot1—18／形态／分层画面清单补原版特殊变体及组合；保留Dummy自然caller、Deathmatch的835 Player分支、未知第三方Mod及未知原生异常的具名限制。 |
| 32 性能 | 消费完整目标字节／节点、逐池容量、资源／引用／曲线／命令计数、稳定重复T及本次实测成本。采集／codec／parse-plan／native-step／draw窗口与含IPC墙钟分别报告；原始观测到当前投影之差不冒称P1—P6实施前后净收益，也不据顺序进程验收宣称10Mbps实时能力。 |
