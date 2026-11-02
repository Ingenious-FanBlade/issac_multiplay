# tx-04-18 I26—I30：工程接线与统一验收

2026-11-02—03，Asia/Singapore。I26—I30已完成。最终Source 49/49、独立冷Client 49/49及12/12检查通过，代表Room/Grid画面已主审；本次不是实时四端或完整联机验收。本文保留实际失败与修复。

## I26：预算、缓存与诊断

`replica/room_grid_budget.*` 在不可变目标复制和任何原生写入前，限制完整目标为 32 MiB、67,108,864 节点、64 层深度；资源 4,096、资源图层 65,536、FX 成员 32,768、Grid 主槽/门别名合计 456。字段/Key/引用、descriptor、计划边和步骤继续使用领域解析器原有上限，未知或超限明确拒绝，不截断必需成员。

历史上1M节点在完整2×2来源中失败，先后提高为2M、3M；2026-11-03按用户要求，当前统一为64M，保留32MiB聚合字节约束。容量只由代码/声明决定，报文不能扩大。`ClientScene` 保持一份 A 与一个候选 T，不建立无界目标队列；解析与计划的 CPU 工作受完整目标及边数上限约束，不承诺硬实时期限。Native Grid 同时至多 456 个，删除/替换先退休再创建；借用和 retained owner 各 16，旧 scope 至多 128，满后显式拒绝并要求结束连接。双配置、DOM、命令和原生资源副本随各自目标/实例 owner 释放；没有按每个 tx 归档副本。

`ClientGrids` 缓存候选已解析的描述，安装后交给同一 T 的最终检查；每次 Manager 步骤至多安装/检查/退休一格。新目标、取消和退休使相应缓存失效。最终检查不再重新解析全图，也不在安装及最终提交阶段重复全图检查。`ClientPresentation::Step` 对同一 T 只准备一次，避免分步 Grid 终检期间反复分配 FX/Image。`ClientApplyResourcePose` 对同一实例完全相同的原生资源/姿态直接复用；任何不匹配继续原有候选预检/安装路径。

`cs status` 仅增加目标字节、节点、资源、图层、别名和 FX 聚合计数。正式采集各域节点统一64M；Grid保留524K查询及32MiB字节上限，其余域原有字节/查询限制不变；Lua 回传计数按本次真实请求预算核对。逐对象、逐 tick、查询、画面对照仍由显式 E2E diagnostics 开启。应用不依赖录制或导出成功。

## I27：产品和测试边界

新增预算模块接入 `isaac_cs_replica`；未新增 Hook owner 或修改常驻 Hook 启停方式。`ISAACCS_BUILD_E2E_TOOLS` 默认 OFF，`scripts/build.ps1 -E2E` 才构建独立 fixture/contract 程序和 Rust harness。`scripts/package.ps1` 仅按清单复制 DLL、UI、正式观察 Lua 和功能目录，采用默认 Off、关闭录制/诊断/控制口且无 token 的配置；不会覆盖已有安装树。玩法夹具不进入产品安装树。

`ops/client_fixture.*` 仅在显式 E2E token、Manager 主线程下导出正式 consumer 自有值。完整来源保存为临时 `source-full-<label>.json`；只投影17已声明的 Entity 字段，Room/Grid 不删除字段或成员。EL 内尚未摘表但 `Entity.Exists=false` 的记录在投影说明的 `retired_entity_prefixes` 中显式列出，不作为可见 Entity 构建；完整源保留原记录，未支持的存活 Entity 仍明确失败。导出与 world recorder 独立。同步测试文件出口不属于生产网络路径；21仍负责实际下行和 ACK。

`cs client_test` 输入经15正式二进制 codec。`client_profile` 是本地投影元数据，编码前取出、解码后原样恢复，没有修改 Baseline wire schema。Grid 首次 `shape_revision=0` 沿用正式身份服务语义，字段缺失仍拒绝；Key和本地 native instance token仍独立。

## I28：完整场景入口

入口为 `harness e2e tx-04-18 --game <根目录> --artifact-dir <临时新目录> --node-role Source|Client`。Client 加 `--resume-from <正常完成的 Source 目录>`。场景配置在 `tools/harness/scenarios/tx-04-18.json`，Source 夹具在 `tests/e2e-mods/isaac-cs-test/tx18.lua`，编排在 `tools/harness/src/tx18.rs`。

Source/Client 是同一次整体 E2E 的两个顺序阶段，不把各场景或重跑另立 tx。Source 在录制关闭下取得真实正式观测，正常退出后才运行冷 Client。Client 的构建、提交、Grid owner、重复目标、原生 draw、非法值、资源部分失败、阶段取消、实际线程/context、玩法/随机/回调/持久隔离及窗口关闭走公共实现。实际截图另外审阅；自动检查完成仍标记 `visual_review_pending`。

Source 使用本地 Wiki 明确的 `TryPlaceRoom`、`RemoveBlueFly/Spider`、Grid pre-update 回调和既有12特殊场景准备器；没有采用新未知原生调用，爬行空间的未公开 FX 适用性另由 GPT-6 Luna max 只读逆向，主智能体复核后采用，见下文。

## I29：实际执行与连续修复

| 顺序 | 实际结果与修复 |
| --- | --- |
| Source 01 | baseline 真实 ready 已产生，harness 共用解析器漏接 tx18 前缀而超时。补上前缀。 |
| Source 02 | baseline、shape1 导出后，shape2 延迟生成钥匙，代表 Entity 出口明确拒绝；改为持续清理 Source，导出未就绪允许有界重试。 |
| Source 03 | 通过 baseline 及 shape1—7；shape8 触及旧 1M 聚合节点预算。提高为有 32 MiB 字节限制的 2M。回读真实源同时修正首次 Grid revision 0 的误拒绝。 |
| Source 04 | 蓝苍蝇由玩家计数持续重生，仅 Remove 实体不够。按本地公开接口清理计数并清理代表角色装备。 |
| Source 05 | 25个目标导出后，水场景出现 `invalid_u64`；修正测试Entity variant对正式typed int64的漏解包。 |
| Source 06 | 水花99已在Source的Init回调退休，但EL普通后态仍枚举 `Exists=false` 记录；代表投影误当待构建主体而拒绝。改为显式保留墓碑说明，完整源和Room/Grid不变。 |
| Source 07 | 水/颜色/地图及32个目标已导出；密集Grid正式观测Partial，未提交残缺目标。补齐Grid专用采集预算及Partial域/字段诊断。 |
| Source 08 | 32个目标完成；密集源仍超出16 MiB单域旧上限，扩大Grid专用预算至32 MiB。产品包默认Off及无测试工具检查通过。 |
| Source 09 | 密集源Complete，但完整模型仍因重复Sprite展开而触及解析预算。SDK确认Rock使用基类Sprite；正式Grid来源v4采用显式同实例alias，旧v3按局部图节点映射比较。 |
| Source 10 | 34个目标完成，包括密集2×2与Devil离图；爬行空间Room Partial，具名诊断定位为FX选择索引。 |
| Source 11 | 确认失败为 `fx_ray_selection`。原版Reset关闭loaded、清空容器但不初始化停用选择索引；按原生消费者条件引入显式适用性，未把未知值补0/-1。 |
| Source 12 | 爬行空间成功，35个目标完成；Downpour采到清理后新生的存活RAIN_DROP135，正式出口正确拒绝。Source对非代表Effect改为Init时退休，不修改Room/Grid环境来源。 |
| Source 13 | 41个来源目标完成并正常退出，Source阶段2/2通过；含12房形、27类Grid、水/地图/门/替换、密集2×2及特殊场景。 |
| Source 14 | 扩为43目标，补齐非空Fire余烬、Rock/Poop/TNT真实破坏后态及Busted门；2/2通过并正常退出。 |
| Source 15 | TimerPlate存在和公开setter换图成功，45目标完成；普通构造仍可保留TimerPlate，测试对隐式清空的假设失败。 |
| Source 16 | 公开setter明确安装未加载Sprite；TimerPlate退休后态成功，46目标完成，2/2通过并正常退出。取得稳定Minimap shader/pose诊断。 |
| Source 17 | 新增正式Minimap边框pose，32目标完成；dense完整各域均Complete，但合并模型触及2M节点限制。旧dense样本已有2,069,106节点，接近2,097,152上限；合并上限提高到固定3M，32MiB总开销限制不变，领域采集预算不扩大。 |
| Source 18 | 3M上限下完成新map pose的全部46目标，2/2通过、正常退出。随后用户要求统一提高节点容量至64M。 |
| Source 19 | 复用46份来源并重放准备，新增真实Frame条件/深度只读诊断；2/2通过、正常退出。Frame入栈z=-1.01。 |
| Source 20 | 46项机械采集与退出检查通过，但用gfx是否驻留误判背景轨迹适用性；该数据未采用为最终Client来源。改为实际Init/墙面正常返回见证。 |
| Source 21 | 新见证下46项完整来源有效；第47项夹具误把RoomType5当宝箱、从错误配置集取Boss而失败。正常退出与恢复。 |
| Source 22 | 复用46项有效来源；追加Boss仍沿用Greed默认查询而无配置，正常失败退出。 |
| Source 23 | 显式Normal配置集仍无所需Boss。补充场景改为Normal阶段直接进入Level已生成的真实Boss descriptor，再做献祭背景。正常退出与恢复。 |
| Source 24 | 将Boss/献祭背景前移Normal阶段并采用真实生成descriptor后，49项采集完成、2/2通过并正常退出。主审REP公开setter确认重复Init不重置RNG，随后补真实Init输入前态，旧数据不作为最终构建输入。 |
| Source 25 | 49项机械采集通过，但主审发现Room.Init直接调用917910而绕过917870，正常房间的initial未取得真实构建前态；不作为最终Client来源。 |
| Source 26 | 将构建见证移至实际917910入口，保存正常返回及真实initial，49项完整来源、2/2自动检查通过并正常退出，作为该阶段Client输入；后有v5成员姿态修正。 |
| Source 27 | 保留43目标前缀，Source夹具以公开PlaybackSpeed=0固定门的两份Sprite，重采后6目标，49项/2项检查通过。但旧ExtraSprite属性的值拷贝缺失仍在，不作为成员姿态修复后的最终来源。 |
| Source 28 | 全量重采49个v5目标，2/2检查通过并正常退出。实际Door附属Sprite基线Offset约(0,15)、Greed右门约(-15,0)，以及夹具显式(3,-2)均被正式采集；旧属性副本对应值为0。Fire真实余烬为2，TimerPlate经历Null→实际Sprite→换图→Null。 |
| Source 29 | 新增公开原始Grid槽集合对照；Crawlspace本次135个成员一致，但Greed换波快照比ready多47/192两格，揭示采样与拍照时刻未固定。加入公开MC_PRE_UPDATE暂停与同批正式快照字面标识后再导出、拍照。 |
| Source 30 | 新的固定采样时刻全量执行到3项，baseline-return未取得完整快照而超时；当时尚无具体域原因诊断，不推断已修复。正常清理。 |
| Source 31 | 复用3项后完成36项；Devil的Grid/Room明确Partial，仍在原生槽中的3个对象不可用。停止把退休身份静默丢弃后，旧漏项被正确暴露为失败。 |
| Source 32 | 复用36项；Devil的5个当前Wall槽对应旧destroyed=1身份，严格来源拒绝。加入有界创建/退休事件诊断。 |
| Source 33 | Devil和Crawlspace本次完整，矿井入口slot227再次命中已退休Wall地址；事件链显示最后一次GridFree已清slot，此后没有工厂取得，当前槽却有新Wall。Luna/max定位两条直接new墙体路径，主审确认后补基物理构造观察与工厂去重。 |
| Source 34 | 修复后全量重采49/49项，每项正式Grid库存与同一保持窗口的公开槽集合一致，2/2自动检查通过；Crawlspace为135个Grid（90 Wall、33 Gravity、12 Decoration），正常退出、三项清理及保存/崩溃检查无错误。 |

Source 01—04 的游戏均正常关闭，部署/环境恢复三项无错，保存/新增崩溃检查无错；没有 Client 像素通过结论。临时文件只用于本轮继续，正常整体验收完成后清理。

| 顺序 | 实际结果与修复 |
| --- | --- |
| Client 01 | baseline写前失败：停用门槽仍保留大房模板坐标。按房形仅核验采用门槽的活动extent，其余仍限制在448容量内。 |
| Client 02 | baseline写前失败：正式空typed table的entries={}未被资源序列解析接纳。补齐精确空编码，保留缺项/非空对象/表洞拒绝。 |
| Client 03 | baseline及四个大房入口完成原生提交/绘制；shape1被旧ReducedSpawnRate的Null误拒绝。Wiki明确该字段在Repentance移除，接纳Null/Unavailable而不创建setter。 |
| Client 04 | 12种房形、四入口及双配置共18目标已提交/绘制；all-grids被普通板的空NextGreedAnimation误拒绝。保留实际空字符串，与nil分开，仅有名字时验证动画目录。 |
| Client 05 | all-grids提交/绘制，19目标完成；锁门使同一Door的Variant从0变1，旧计划误要求换代。Door Variant改为同实例可变状态，仍核验类型/实例代次/构造seed。 |
| Client 06 | 46目标完整正式纯值预检，集中找到4项特殊场景问题：镜门/矿车路由不以普通descriptor启用位裁决；Mineshaft保留未占图descriptor；镜像dim1原始GridIndex异常保留诊断，native几何独立取真实offset。 |
| Client 07 | 46份目标均通过写前预检；实际37项成功，矿井背景Init产生未列入目标的1000:140，提交正确失败并退休。新map pose未修复缺框。 |
| Client 08 | 复用未受影响37项前缀，冷构建baseline及mirror_inside；确认额外实体为BACKDROP_DECORATION，地图几何值与Source一致。 |
| Client 09 | 同样正常退休；只读诊断确认Frame可见/资源/全局颜色条件一致，但Client入栈z=-1，与Source的-1.01不同。未将这三次局部续测计作完整Client通过。 |
| Client 10 | Source26全部49目标写前预检通过，实际43目标提交/绘制成功；背景与地图框修复后的彩色截图经主智能体核对。Greed换波中同Key Rock Variant1000→0被旧构造冲突逻辑拒绝。 |
| Client 11 | 将Variant/VariableSeed作为同实例可变状态后，续测其余6目标，全部49目标及12/12自动检查通过，正常关闭与访客持久隔离通过。但本次新截图连菜单均为空白，不能认定视觉通过。 |
| Client 12 | 新增原生renderer HWND同一性核对、窗口DC/桌面客户区双采集和近全白/全黑拒绝；baseline实际提交后仍采到空白，正确判失败并正常清理。用户随后告知电脑已息屏，待点亮后核查显示采集。 |
| Client 13 | 亮屏后全部49目标及12项自动检查通过，原窗口DC截图恢复彩色。桌面区域采集被其它应用遮挡，移除该不可靠入口，保留renderer HWND核对及窗口DC空白拒绝。实际窗口图显示Greed门附属Sprite差异，未据自动通过结束视觉验收。 |
| Client 14 | Source27的全部49目标在新冷进程完整重放，12/12检查通过并正常关闭；固定播放速度仍不能消除门附属Sprite位置差异。静态主审定位到旧值属性清偏移/flags，改正式来源而不修改原生Render或放宽像素判定。 |
| Client 15 | 全部49个v5目标和12/12自动检查通过、正常关闭；Greed附属Sprite与TimerPlate实际像素已匹配。Crawlspace上下边缘仍有缺砖，主审追到Source28正式库存仅122个成员、漏掉仍在绘制的Wall；本次未判视觉通过。 |
| Client 16 | 使用全新Source34，不复用旧Client前缀，完整49/49及12/12自动检查通过，正常退出且三项清理无错；主智能体完成49对画面与Crawlspace原分辨率复核，Room/Grid代表验收通过，边界见最终结果。 |

## FX适用性：逆向与主审

按本地RoomType文档确认16为Crawlspace；REPENTOGON SDK提供FXLayers布局，公开接口未提供本次缺失语义后，Luna max再静态核查J273。主智能体独立回读 `7E7670`：`7E7C76` 对RoomType16/背景18跳过Load，`7E7D6C`调用 `96BD30`；Reset清空层/射线并置loaded=false，清空十个vein根的child，不写选择索引及poop计数。主审 `96CE20` 与其它绘制pass均先检查loaded；`96EA50` 在 `96EA5C` 只以首根child判定vein是否消费。

Room来源v5 / presentation v3明确标记未加载FX的NotApplicable，不读取其停用索引/计数；poop仅在loaded且倒计时>0时采集，vein仅在实际首child激活时读图。Client对停用部分构造自有关闭状态，不声称取得Source标量。继续接受已有Room v4/presentation v2与Grid v3/v4，该阶段能力为room-grid-target-v6。此为静态消费条件，特殊场景运行结果在I29另列。

主审随后复读 `HUD::Render 97C300`，确认 `97D0BB/97D370` 对difficulty 2/3直接采用已安装Greed波数；压力板 `95E140/95E2D9` 消费Room+723C真实倒计时。移除旧代表的difficulty>1拒绝，challenge/seed边界保留。

另在Source发现SDK `_slowdownDuration` 的真实word为 `0x72617000`。Luna复核后，主智能体直接回读 `807146—8071AC`，只核定Room+7220低字节控制坐标调整。停止写int高三字节，不将7224的boss/deathspawn计数猜作slowdown。新Room来源v6/runtime v3只采真实byte `render_flag_7220`；旧runtime v2只解释原始word的低字节，不采用其高位。能力同步v7，接受既有Room v4—6/runtime v2—3。

小地图视觉：Client05的世界、Grid与地图格子可对应Source，但缺黑色外框。Luna核查原版资源和REP `Anm2Extras.cpp`，普通层显式选择ColorOffset，故Source入口Colormod/Client入口ColorOffset差异不能直接归因；主智能体复读 `966915 -> ANM2::Render`，补充真实config `_mapSprite`语义pose来源，Refresh后安装。新Room来源v7要求 `map-frame-pose-v1`，能力v8；不是复制ANM指针或用IconsSprite替代。视觉结果仍以续测为准。

镜像GridIndex：原版Wiki、REP SDK及主智能体回读确认字段为+0的int32。Source dim1 1x1样本raw为 `0x00600060`，实际Safe/offset/current为68，raw含义仍UNKNOWN。完整模型不改raw，`raw_grid_index_adopted=false`，`client.unadopted_descriptor_grid_indices`显式计数；本方native原点仅用真实唯一occupied格与Safe交叉确定，绝不把raw当数组索引，也不声称raw字段等价应用。多格或缺占用的异常不套用此分支。Mineshaft已不占图的descriptor仍保留Data/存储身份，不补旧offset。

## I30：后续消费接口

- 19：`ClientGrids::ResolveReferences`、`VisitLifted` 与稳定 Key/实例代次；Effect136 专用 Entity 应用仍归19，不能创建第二份活动 Grid。
- 20：`RoomGridTarget::room.layout`、最终地形/path/门与环境及已有真实 Player 前置；身体 roster 和装备归20。
- 21：同一 `ClientScene::AdmitTarget`、纯值 parser/plan 和 `ClientApplyResult`；只有 Room/Grid 范围提交，当前不发送完整世界 Applied ACK。
- 23—29：门路由、特殊 Room、地图及独立事件接口沿用[场景接线说明](tx-04-18-scene-integration.md)；不从本次顺序 Source/Client 推导实时四端或整局可玩。

最终支持范围和后续责任见文末；不以任务行数或纯构建替代功能通过。

2026-11-03会话修订：用户要求“先给节点预算尽可能拉高保证不因为这个失败，后续会进行压缩优化”。聚合、正式consumer各域、默认Lua与独立回传验证的节点上限统一提高到64M；既有32MiB聚合、各域字节/深度/对象、引用/资源限制继续独立生效。不以已通过旧上限的完整来源重新扩充实际内容；当前低于旧上限的采集值可继续用于本轮Client验收。

## 后续主审：真实队列深度与背景构建副作用

主智能体回读9E4140：PresentWithoutSwap消费CPU队列并把BA4714重置为-1，不清当前FBO的depth。Client内层RenderState在世界合成后执行该操作，破坏Game.Render合成→HUD的连续z。新增FlushSameTarget只保留该全局递减游标，仍消费队列；真正offscreen Present继续原生清depth/重置。Source19与Client09的Frame条件相同而入栈深度分别-1.01/-1.00；修复后的像素仍待续测。未采用此前被主审否定的DebugRenderer=队列消费点解释，也未加入猜测性的glClear。

Luna核查917870→917910→911310后，主智能体独立复读915890完整callee与915660/915750/917420。915890会生成背景Effect并推进Backdrop独立RNG；直接跳过会改变后续墙纹，故未将该临时方案运行或认定等价。新增正式Room来源v8/能力v9：每次原生墙面构建保存起始/结束独立seed及每次915890的参数、DestinationQuad与真实RNG前后态；Client逐项核对后重放后态，保留原生墙面绘制，不调用被省去的实体生成和全局Random。旧来源可解析，但缺少构建轨迹不得进入该原生构建。轨迹改变会使背景缓存失效。

全局Random维护MT状态和跟踪链表；本方案不复制或推进Client全局MT，不声称完整游戏RNG等价。Source的完整玩法与Entity状态仍由Authority负责。917420绘制后的*null标记生成121与911310内两处直接生成7的分支，另按实际指令补齐隔离；不以最终EL为空代替过程中未生成的判断。

背景主审补充：917420在917518/9175CA/9175DF调用405990（AnimationState::Render），不能据子智能体一度的“无Sprite draw”说法跳过整个helper。主审保留前半h/v随机选择与绘制，仅在Client同步构建栈内借用 `_wallDetailANM2._numNullLayers=0`；91761E/91772B仅读该计数枚举*null生成121，离栈恢复。两处直接variant7分支的原call点91168F/9119E9及landing911778/911AEE均由主智能体回读；常驻shim保持GP/flags/x87/MXCSR/XMM和原栈，Client只补生成块用于scale的一次独立xorshift，再略过共享Random、Spawn/配置/Entity.Update；Source/Off重放原call恰一次。原字节、RVA与恢复地址都来自统一hooks.json并由生成器供给，不增加第二Hook owner。

REPENTOGON `LuaInterfaces/Room/LuaRoom.cpp:75–84` 的SetBackdropType直接调用Backdrop.Init；不重置RNG。来源轨迹因此另保存真实Init前态`initial`，Client先用它重建地面，再进入墙面轨迹。Source25开始采集该完整格式，Source24前的来源不拼造补值。

## 后续主审：Sprite值副本与真实成员

Luna再次只读研究门绘制，主智能体复读`707F90`、`4059E0`、`405990`及`805886—80592F`，确认Render不消费playing状态、不推进动画；原版顺序为逐格Begin/Render后第二轮End，与Client一致，没有按假设修改产品绘制。Source27固定播放速度并未消除该成员差异。

原版`ExtraSprite`属性getter `8A6F50`创建`UserdataValue<ANM2>`并调用`402CD0`。子智能体最初把深拷贝phase/selectors扩大为全pose等价，主审逐指令发现其在`402DA7/402DB1`清ANM2+94/+98的Offset，`402E97`清+110 render flags；完整尾部没有恢复。子智能体复核并撤回原判断。REP `LuaGridEntityDoor.cpp` 的`GetExtraSprite()`返回真实内嵌成员指针；正式采集改用该公开Getter。TimerPlate同样只有值属性，使用已有+154原生只读成员接口，未加载保持Null。两者均不复制指针到纯值目标。

Grid来源v5要求`grid-member-sprite-pose-v1`，能力v10仍接受旧来源纯值解析，但原生Grid构建至少需要v5，不把旧值副本缺少的成员偏移/flags补0或默认为正确。实际成员原值及像素结果由后续完整来源/Client续测判断，静态副本缺陷本身不冒认本场景根因。

## 后续主审：Grid直接物理构造与完整库存

Source/Client的Crawlspace画面对照发现缺砖后，主审复读Room.Render `804500` 的RoomType16分支：它仍只按当前Grid槽逐格绘制；Wall.Render `71B4E0`没有私有墙面缓存或邻居补画。不能通过Client额外画墙掩盖来源漏项。`ObservationGrids`现在将本批真实非空槽对应的缺失/退休身份计为不可用，使Grid/Room明确Partial；同一就绪时刻独立核对公开Grid槽集合和正式成员集合。

REP公开回调文档明确`MC_PRE_UPDATE`返回true取消本次玩法更新。E2E在同owner、Authority tick、run/room且11域全部Complete/NotApplicable的快照发布后使用它保持Source；字面标识只由上下文编号串接，不是哈希。拍照、原始槽集合和导出使用该稳定窗口。停止观察使标识失效并释放暂停，不影响正常退出。仅显式E2E可调用此诊断入口。

Luna/max只读定位两条绕过`CreateGridEntity 706100`的直接墙体构造，主审独立反编译`7E4B40`与`80B950`确认：前者`7E4D80`调用基构造后直接设置Wall/slot并调用专属初始化；后者`80BA0E`基构造、安装Wall后才调用Grid.Init。REP的inline spawn patches没有覆盖普通Wall分支，官方GridEntityPlus也只包工厂。Source33矿井入口的有界真实事件链最后是GridFree退休，之后无Factory，当前却为Wall，与该漏记机制吻合；未将此前未证实的延迟旧call退休猜想当作原因。

统一Hook清单追加`GridCtor=19`，原有0—18编号不变、当前共20项观察语义。主审`704FB0`确认ECX=this、无栈参、`705019`设置EAX=this、`70501F ret`。仅正常返回后以物理构造换新local/use，工厂包含该构造时去重；不由地址相等恢复旧身份，也不把基构造当作派生Init或完整外层出生。直接墙体的最终公开读取继续要求真实initialized/index/membership。诊断事件仅显式E2E保留至多4096条，不进入正式目标，生产默认不分配。

构建中一次清单校验拒绝方法/definition名称不一致，统一名称后重新编译成功；这不是实机结果。息屏期间的空白截图保留失败记录；harness使用线程级SetThreadExecutionState临时保持屏幕/系统唤醒，离栈释放，不修改用户电源方案。

收尾主审把背景分支shim改为先选择路径、再分别恢复全部寄存器/flags；不再借保存的ZF携带选择结果，避免覆盖原始flags。Client16使用该修正执行完整背景及场景续测。

## 最终统一验收结果与边界

2026-11-03，I26—I30完成。Source34全量49/49，2/2检查通过；随后独立冷Client16全量49/49，12/12自动检查通过。两进程均正常退出，三项部署/环境清理错误均为null，保存检查与新增崩溃检查无错误。Client每个目标实际经过15 Baseline codec、正式接纳、原生安装/检查/提交及draw，不使用旧Client前缀复用。

主智能体逐组查看49对窗口截图，并以原分辨率复核Crawlspace；全部房形、大房入口、Grid绘制家族、背景/水/地图、特殊房间、Greed附属门及TimerPlate状态符合本tx的Room/Grid展示范围。Crawlspace的135个真实Grid与完整边缘对应，缺砖已消除；Greed侧门的实际成员Offset和破坏门状态、Fire真实2个余烬、TimerPlate的Null→存在→换图→Null均按真实来源应用。

| 验收范围 | 最终实际覆盖 |
| --- | --- |
| E01/E18 | 独立Source退出后菜单冷Client；正式录制关闭；149个统一注册Hook；完整来源保持正常Host/Off调用；普通产品构建与发布边界 |
| E02/E03 | 全12房形、四个大房入口、双配置、普通/Greed配置、离图Devil/Crawlspace、Mirror两维、Mineshaft入口/内部；非法路由写前拒绝 |
| E04/E07 | 1—27全部非空Grid类别及17公开派生接口；组合岩石、坑桥/梯子、尖刺/机关、TNT/Poop/Rock破坏后态、Fire余烬及真实原生资源 |
| E05/E06/E12 | 同Key可变Desc、同格替换、删除/重建、短命Grid、门锁/破坏/删除及附属Sprite；重复目标不重复原生构造/资源加载 |
| E08/E09/E10 | 背景真实输入前态/墙纹RNG轨迹、水面裁切、光色/FX、轨道/特殊Room、Room进度、真实Lost与正常地图恢复、Camera/scroll上下文 |
| E11 | 实际Pit最终桥/梯子状态、缺必需引用拒绝、TimerPlate真实成员存在/换图/退休、借用失效；Effect136所有的lifted描述保留19接口，不制造虚构的Grid parent或声称完整Entity应用 |
| E13/E14 | 路由/预算/路径/引用/同版本冲突写前保留A；缺必需资源触发真实部分写入、撤A并退休/重建；四个采用阶段取消；显示期正常窗口关闭 |
| E15/E16 | 实际worker/context/已释放借用拒绝，真实临时Lua VM代次；每个显示窗口核对9类原玩法original计数不变，Native/Lua持久前置拦截及退出后访客进度无变化 |
| E17 | 密集2×2及批量Grid；稳定目标重复20次不重建/重载；单A/单T和有界逐格步骤，超限明确拒绝 |

本次49目标的最大聚合计数分别为21,913,703预算字节、1,951,128节点、453资源、739图层、403槽别名、11个FX成员；这些是实际样本最大值，不代表测到所有上限或性能极限。用户要求的64M节点上限保留，32MiB及其它独立限制不变；压缩/完整性能工作仍归32。

最终普通`build.ps1`（没有-E2E）成功，CMake测试工具开关OFF；`package.ps1`生成44个正式文件，清单无exe/玩法测试mod，默认enabled、recording、diagnostics和control_port均false且无E2E token。功能目录由同一生成输入更新并同步到本目录，能力room-grid-target-v10标注本次代表验收范围，full_world_commit仍false。

本次没有逐像素相等结论：Source夹具持续设置无敌冷却，身体会按原版规则闪烁；Client仅采用17代表Player字段，完整身体/装备与闪烁状态归20。初始楼层标题等完整HUD归27，动态水/光/地图指示的时钟相位与FPS也不等同于同步目标。Room/Grid几何、成员、状态、资源和必要层次已实际对照；不把这些已声明的跨域差异混成完整世界画面通过。

旧Mirror样本的dim1原始GridIndex=6291552仍是UNKNOWN；当时保留raw并从唯一实际occupied格确定native原点，早期冷Client已覆盖该分支。最终Source34该字段为合法68，不再出现旧异常；不据新值改写旧事实。全内容组合/路线归26/29，Entity及lifted Effect136归19、Player归20、真实网络/ACK归21、完整HUD/独立音画归27；本次同机顺序两进程不代表四端实时或整局可玩。

最终复核后已清理本tx全部临时来源模型、窗口截图、诊断/构建日志、测试安装树及专用harness副本；没有留下E2E artifact。普通产品输出out/product保留为构建产物。
