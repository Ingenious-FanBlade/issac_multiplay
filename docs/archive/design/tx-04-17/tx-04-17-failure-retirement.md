# tx-04-17 R5：部分失败、原生退休、菜单交还与持久隔离

日期：2026-11-01，Asia/Singapore。目标为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。同属 tx-04-17，汇总 [R1 初始化](tx-04-17-client-scene-lifecycle.md#r1-results)、[R2 对象生命周期](tx-04-17-native-object-lifecycle.md)、[R3 维护／绘制](tx-04-17-native-maintenance-rendering.md)、[R4 资源寿命](tx-04-17-native-resource-lifetime.md)和 [ST8 持久归属](tx-04-17-persistence-exit-isolation.md)。本轮只读公开接口、源码与必要静态逆向后更新设计文档，不修改代码、绑定、游戏文件或 IDB，不构建、不运行游戏或测试，不另留 artifact。

必要逆向沿用户指定的一个 GPT-6 Luna／max 子智能体执行，由主智能体独立复读审阅。`PUBLIC_DOC`、`SOURCE`、`J273_STATIC`、`REUSED_J273`、`DERIVED`、`DESIGN`和`UNKNOWN`分别表示公开语义、源码、当轮／复用的目标静态事实、推导、设计及未闭合内容。地址默认 VA，`RVA=VA-0x400000`；静态研究不表示安装命中、异常恢复或无访客副作用已经运行验证。

## 1 本轮采用的失败与退休原则

**后续R6归并（2026-11-01）**：[模型／角色／应用器合同](tx-04-17-model-role-applier-integration.md)明确A在部分写入后只作历史完整值，当前原生差分资格失效；取消、较新T、旧Scope迟到和worker结果各按实际attempt处理。Client场景状态不与连接／Host SessionMode混用，16的Input／Scope／退出接线缺口已登记，提交结果由17产生、Applied ACK由21—22消费；本项具体菜单／取消／异常UNKNOWN继续保留。

**失败不改变世界归属。** 断连、stop、超时、apply错误、资源失败与准备取消均先关闭新输入、目标应用和受影响展示；只要服务器场景及其后续消费者尚未安全退尽，仍保持Client隔离。网络线程结束、收到disconnect、两轮松键、画面已停或Manager看似菜单，单独都不足以转Off。

本轮选择自有退休编排，调用已解释且满足前置的原生完整helper；不跳到Exit／Room Init中部执行，不调用Exit(false)取消，不先开一局再清空，不使用访客Continue恢复“原状”。按实际owner撤销关联、清理或保留可复用的已构造根；保留原生Game／Room不等于保留服务器run状态供Off接管。

失败后区分三种结果：未改原生世界时丢弃候选；已部分改动但owner有效时停显并按实际前缀清理／重建；对象有效性或清理阶段不确定时停止相关修改、保持隔离并报告不可恢复位置。最后一种不伪造退休成功，不重复运行未知helper，不承诺任意SEH、access violation、损坏指针或强制终止都能安全回菜单。

## 2 公开接口、官方包装和现有工程

本地依据为 `F:/projects_isaac/IsaacDocs/docs/` 与 `F:/projects_isaac/REPENTOGON-1.1.3/`。公开 [MC_PRE_GAME_EXIT](https://wofsauge.github.io/IsaacDocs/rep/enums/ModCallbacks.html#mc_pre_game_exit)说明ShouldSave语义、结局可能重复调用及返回值不能否决；REPENTOGON本地 `docs/docs/enums/ModCallbacks.md:138—145`说明early MC_PRE_MOD_UNLOAD增加ShuttingDown参数。公开名称不提供通用“取消并安全回菜单”接口；native Exit／ResetState／cleanup_current_state仍须按源码及目标ABI解释。

| 入口／实际来源 | 已读事实 | R5采用限制（`DESIGN`／`DERIVED`） |
| --- | --- | --- |
| Game::Exit、Game::End及退出回调 | ST8已核Exit的PGD flush早于ShouldSave；false可条件删除run。公开[Game:End](https://wofsauge.github.io/IsaacDocs/rep/Game.html#end)还涉及结局／胜场状态，不是纯菜单转换。MC_PRE_GAME_EXIT返回void且可重复 | 不用这些入口把取消包装成无保存退出；不通过伪造false、结局或重放回调来恢复菜单 |
| Game::ResetState | `Game.zhl:16—17`给native声明；`SaveStateManagement/EntitySaveStateManagement.cpp:2587—2605`在super之前收集backwards stages保存项、清ESSM、ResetBackwardsStage，再调用原生函数 | 只有相关Game／descriptor／backwards保存容器都有效时才是候选；原生函数无call也不证明官方整链无Lua／无分配。不能用于修复未构造或已损坏容器 |
| ESSM ClearStates | 同文件`:1092—1103,1181—1207,897—922`维护保存项并经registry调用内部Lua；`resources/scripts/repentogon_extras/entity_save_state_manager.lua`的 `_OnClearSaveStates`仅移除对应保存ID的内存数据 | 保留这些具名内部结构维护；不等于允许任意gameplay／Mod持久callback。Lua错误只记录不自动把整个外层清理标成功；也不假设该函数删除访客文件 |
| 官方Exit后处理 | `VirtualRoomConfig/VirtualRoomSetManager.cpp:1265—1269`在super后ResetAllRoomWeights，`:360`遍历已配置room chunk恢复InitialWeight；`Patches/ItemPoolManager.cpp:2073—2077`置扩展pool未初始化；`Patches/LagMetric.cpp:85`复位局部初始化标志 | 跳过整条Exit不会自动做这些工作；按实际采用的配置／pool域安排等价维护，不手改匿名静态变量。ResetAllRoomWeights为源码内static函数，不假定本方DLL已有可调用导出；保留官方Hook后处理或提供具体桥接的范围／层次仍须落实，不绕过整链后宣称官方状态已复原 |
| 原生对象／资源退休 | R2／R4分开逻辑remove、摘表、池use、物理Free、内嵌ANM、Lua包装、Image强引用与raw队列。Room dtor还会先Free其Grid | 按owner和依赖清理，不能对池项delete，也不能给同一Player／Grid重复Free；外层析构包含内层资源的责任，不再逐成员手动析构一遍 |
| 当前Client停止 | `mod/src/game/client.cpp:130—178`停止worker、进入Retiring，依赖worker finished、两轮真实中性和MenuReady转Off；源码明确假定没有自有图形／Lua引用 | 16的无世界实现事实保留；17新增原生场景后必须增加本合同的owner／资源／持久结果，不能复用旧判断宣称世界退休完成 |
| 当前进程退出 | `client.cpp:259`的ClientProcessExitDecision目前只等待worker；`hooks/exit_patch.cpp`合并Client与world决定，但安装仍依赖WorldObservationArmed及World capability | 17的正常退出协调不能依赖录制打开或仅worker finished，须覆盖原生资源与持久隔离；仍走统一常驻Hook清单，不另装可卸载退出补丁 |
| 当前原生Exit wrapper | `mod/tools/gen_binding.py`的game_exit通知RunTeardownBeginning、执行原链，正常返回后RunExit；后者清排队run变更并关观察窗口 | 这些是现有观察／会话收束，不是Client持久否决或资源清理。RunTeardownBeginning／RunExit不是新的Client安全退出API，不因窗口关闭就撤销保护 |

### 2.1 J273正常清理链与菜单后态的主审事实

子智能体定位后，主智能体独立回读下面的原始指令／函数和官方源码。当前IDB仍为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`；所有结论限正常前置成立的路径，不把伪码的残余寄存器参数当可调用ABI。

| native入口（VA；RVA） | 已复读的调用／字段与有限后态 |
| --- | --- |
| `Game::ResetState 0x6F08D0`；`0x2F08D0` | 对Game标量／计数直接写入，含7组BackwardsStageDesc（基址`Game+0x669F0`、stride`0x320`）计数／标志，函数内无call；它不释放`Game+0x18300` Room或`+0x1BAA8` PlayerManager。官方ESSM prehook仍先执行，不能把原生无call外推整链 |
| Game状态helper `0x6F0770`；`0x2F0770` | Exit于`0x6F6454`调用；helper于`0x6F07A1`调ResetState，此外清Game标量并对`+0x25BB8/+0x25C5C`调用容器／ScoreSheet helper。不是全场景析构，也不是公开轻量取消接口 |
| Room重建 `0x732D20`；`0x332D20` | `0x732D55`对旧`Game+0x18300` Room调dtor `0x7E0190`，`0x732D60`free旧对象；`0x732D6D`才new(`0x7898`)，`0x732D81→0x7DF8F0`构造，`0x732D88`回存新Room。期间旧字段未清，失败并非保留旧Room；调用是normal reset/rebuild，不是无分配teardown或通用回滚 |
| PlayerManager清理 `0x98F7C0`；`0x58F7C0` | Exit于`0x6F65D7`以`Game+0x1BAA8`调用；helper从当前`Game+0x18300` Room取EntityList，对主roster逐项撤销若干成员／类型缓存、置Player valid=false，在`0x98F850`调slot0 Free(1)，之后搬移／缩短roster；还清辅助指针vector及四个保存对象指针并复位颜色分配。它需要有效的当前Room和一致roster，不能在旧Room已free、新Room未发布时调用，也不以它修复重复／未知归属Player |
| Room／EntityList析构 | Room `0x7E0190`在`0x7E01D4`读Camera `+0x11F8`并释放六组数组和对象，`0x7E03C5`调内嵌EntityList `0x413740`。该EL析构的全部派生虚调用／池项回收不能由“析构”名字猜成逐实体Free；Player另由上项owner处理 |
| Exit正常后段 `0x6F6270`；`0x2F6270` | 入口以`Game+0x25B02!=0`限定active分支；成立才执行本表清理。`0x6F65CC`先Room重建，再`0x6F65D7`清PlayerManager，`0x6F65E2/5ED`调HUD `0x97AB10/0x979D80`，`0x6F66B9`清暂停界面。`0x6F66C8`将`Game+0x25B02`两字节写0，`0x6F66D7 ret 4`。本工程RunActive指向`+0x25B02`；该位只在尾部清，不能提前清它冒充整个清理完成 |
| HUD两段 | `0x97AB10`含多处ANM Reset、图像引用清理、8个PlayerHUD helper以及History／Stat helper；`0x979D80`重设显示字段、8槽owner／index并调用缓存重置。它们不是单纯取消关联，可能进入资源释放；尚未核定全部callee，不整体标成纯函数 |
| `Manager::cleanup_current_state 0x932970`；`0x532970` | state=2且Game存在才于`0x932996`传`Manager+0x4AE64`给Game Exit，返回后清`+0x4AE65`；state=1另有旧菜单对象销毁分支。调用它仍可能进入ST8保存／删除，不是无保存清理别名 |
| 普通菜单请求helper `0x933060`；`0x533060` | `+0x4AE68`请求成立后，在`0x933119`调cleanup_current_state，`0x93311E`调另一helper，`0x933129`写Manager state=1；必要时new(`0xF938`)／构造菜单对象，`0x933173→0x962E20`准备选定页，`0x93319E`清请求。state1在Manager Render `0x92EE1C→0x9623C0`走菜单。helper前段另可能进入`Manager+0x4AFB8`的原版Online相关链（`Manager.zhl`将此字段声明为NetplayManager），因此C/S不整体执行它、不触及该状态，未命名callee和菜单参数仍有UNKNOWN |

另一路Manager Update在`0x92E495`调cleanup_current_state，随后`0x92E4B6`写state=3；Render的state3在`0x92EE29`进入Cutscene，与上表普通请求写state1是不同路径。不能据其中一处call就说所有Exit都直接回菜单，也不能将请求helper比较的`Manager+0x4AFB8/+0x4AFBC`误称Game EntityList。

`DESIGN`：采用完整有效世界时的**分职责候选**是Game状态收束、Room／Player owner清理、HUD／界面资源与官方配置状态维护，再完成菜单对象／选择页及Manager请求状态。若选择Room重建与PlayerManager helper，首先沿已核正常依赖保持“有效新Room发布后再清roster”，不盲目倒序。每个候选都要求其资源／容器有效，且所有Gameplay／持久／Lua消费者已按用途分派；它们不是已经闭合的通用teardown函数。由于Room重建本身有分配和失效窗口，17实际实现仍须解释该前缀失败的责任；不能把这项风险推给Exit(false)或未来统一catch。

部分准备时RunActive可能尚未成立，Exit可跳过整段清理，即便caller随后写state1也不代表本方已创建的对象被回收。不能为了借用清理先伪造active=true，也不能先清active来绕过保存然后假定后段仍执行；两种做法都破坏本表实际条件。自有退休按实际前缀和有效owner处理，而不是通过run位操纵大函数控制流。

菜单请求`+0x4AE68`及`+0x4AE64/+0x4AE65`不等于ZHL的`_starting +0x4AD10`、rerun／debugStart和seed请求字段。上述函数未直接清所有启动字段，不据此声称它们一定残留或一定已被其它helper清除；恢复Manager原链之前，R6按实际来源取消旧请求并保证不会重新进入ExecuteStartGame。`0x933060`整体不作为Client调用入口；不能跳过它的前半段后从中部进入来绕开Online。

## 3 记录实际前缀，不以布尔成功猜清理责任

以下为待实现的内存状态合同，不是新协议字段、独立artifact或当前已存在的类型。记录随生命周期变化更新，稳定期间复用，不逐consumer重复全图扫描。

| 必需信息 | 用途 |
| --- | --- |
| session／scope／目标版本、最后完整A与本次T | 取消旧目标的新工作；部分原生写入后A仍是历史完整值，却不再描述当前可安全显示的原生世界，不以A回滚成功或T已应用发ACK |
| Game／Room／Camera／Player／Grid／pool-use及资源代次 | 区分仍存活owner、地址复用、成员失效和最后释放；网络key不提供原生强持有 |
| 已尝试／已正常返回／已发布成员／待退休的阶段 | 进入可能破坏性调用前就登记已尝试；不能仅在helper返回后登记，否则异常会漏掉真实前缀。异常展开后只接受有直接依据的正常后态 |
| 成员、反向引用、官方副表、HUD／roster／door／Camera关联 | 依据R2／R4实际依赖撤销，避免仅清EL后Player仍被其他owner引用；不将所有清理当作分配列表倒序free |
| 实际在途栈、采用pass的raw队列／batch与自有Image pins／Lua refs | 在正确主线程／phase／context下停止新使用、消费或取消、释放；pin只保Image，不保证draw记录仍在队列 |
| 每项owner的退休责任及是否已经被原生EH／外层dtor清理 | 防止重复调用Remove、析构、unref或free；不据“进了catch”推定所有构造都完成或所有析构都执行 |

R1已证Camera替换是先释放旧对象、后分配／构造、最后回存Room字段，中途字段还可能指向已释放地址。自有编排应分别登记旧对象已退休、新对象分配／构造／发布的状态，并让绘制与其它读取方看不到失效关联；不重放原Room Init，不再次free字段中的旧地址。是否能安全改字段／清理新分配依赖具体构造和owner合同，未知处不靠清零、memset、直接跳EH handler或手调派生dtor补救。

## 4 各阶段失败、取消与退休表

以下为`DESIGN`；“完成”均要求相应原生及官方owner正常后态有据，单个void返回／非空指针不代替整体判断。

| 停止／失败位置 | 立即停止与允许保留 | 清理、恢复与结果 |
| --- | --- | --- |
| 原生写入前、只有DTO／连接 | 清输入／边沿、目标publication与新候选；旧scope不再产生事件 | 丢弃有界自有值，按各自owner退休资源候选，worker异步停止；未触世界且菜单仍安全才按输入收束后交还Off |
| 原生资源候选已准备、尚无世界变化 | 候选不发布；先前完整展示仅在其scene／资源仍有效时可保留 | 候选即使没draw也可能有Image／ANM／Lua资源，按R4有效context释放；不把“没发布”视作可在worker随意析构 |
| 根／descriptor／结构部分准备 | 停相关世界draw与下一批原生应用；已构造根可继续保留，未完成对象不冒充有效 | 按实际前缀与构造责任撤销关联。只有依赖完整有效的已构造owner才可用具名reset／清理；不能调用访客Restore或完整Exit试着恢复 |
| create中工厂槽已取得／Init未完整返回／已进原生成员 | 禁止半成品绘制，标实际pool-use和已发布成员；不确认create | 经对应owner撤销成员与关系；池项与直接分配的Player／Grid不同。官方New／Delete与ESSM维护按实际前缀配对，不补造网络出生或重复执行整个Spawn |
| 已应用部分字段／引用／Morph | 停受影响scope展示和后续变化；未影响scope仅在依赖完整隔离时继续 | A不能代表当前原生后态；不ACK T、不重复整批。保留前缀，按有据owner恢复有效空承载，再从完整目标／新Baseline重建；不能重放随机／奖励／玩法以补缺项 |
| remove已逻辑完成、尚待摘表／Free | 不恢复旧存在性、新借用或旧scope输入；服务器目标已删不因此立即本地free | 清实际引用／EL／roster／HUD／door等后，在途退尽才物理退休；重复请求依据已执行阶段幂等，不重复Remove／ClearReferences／Free |
| 已展示后断连／超时／stop | 立即停止输入与新目标使用；冻结完整画面仅在资源和owner有效时允许，断连画面不继续玩法 | 按同一退休流程结束，不等新网络包、不自动Off；恢复必须新scope／完整目标，不恢复旧edge／事件水位 |
| 资源换代／Load／Morph后部分失败 | 旧层／动画借用立即失效，半套姿态不可发布；Image pin不恢复旧资源语义 | 正常draw消费后换资源；若写入已发生，停显并按新实际前缀退休／重建，不假定原子回滚。LuaImage finalizer与最后native strong分别处理 |
| 错误线程／context不符／意外重入 | 不进行新原生／Lua／GL操作，主线程记录真实原因；不强制MakeCurrent | 若未写可取消候选；已写则保持隔离并交有效owner边界续清。当前callback不等待自己的reader归零；不得重复调用未知外层helper |
| OOM、Lua error、正常C++异常 | 捕获层只能依据已知展开合同停止新工作；必要的最小错误状态不得依赖再次成功分配大容器 | 已正常构造且owner有效的部分按责任清理；构造未完成、原生EH是否清理不明时停止相关释放。不承诺catch(...)或Lua错误日志能把场景还原 |
| 进程正常关闭请求 | 从首次请求起关闭新输入／应用／展示，开始当前主线程有效阶段的资源退休 | 不假设还有下一次Manager Update；必须在Shutdown直接进度写入及各资源截止点之前处理。进程退出不必先转Off或回菜单，保护覆盖剩余关闭整链 |

正常退休顺序为：关闭新输入／publication／目标 → 当前在途栈返回 → 采用pass的正常CPU消费或已解释取消 → 撤销旧借用／关系与成员 → 对应owner清理／保留有效空根 → Image／Lua等按各自截止点退休 → 原生菜单后态与输入／网络收束 → 最后解除本次场景的Client隔离。实际依赖可使“清关系”和“退场”分多步，不能强套全域先create后remove或单一逆序free。

## 5 在途取消、LuaImage及无法恢复的处理

`REUSED_J273`：正常Image提交／reset `0x9EFA70→0x9EFA10`、按图像cache／raw remove `0x9EFE60→0x9EF7C0`、最后strong析构至GL删除是不同责任。正常CPU reset不证明所有pass清理或GPU idle，cache callback也不是通用取消全部场景的API。R4已确认custom-render buffer context副本分支在1.1.3关闭，不因这段未启用代码为R5新增虚假reader。

`DESIGN`：能正常消费且context／pass仍合法时，先让实际owner完成旧提交／清理，再做破坏性退休。不能继续消费时，只采用已解释的取消操作及其caller后态；缺少这条路径就停新发布、维持有界保留和隔离，不运行整条Gameplay／GameRender排空，也不根据“延迟两帧”猜释放。预算到限不能驱逐仍在途owner或无限堆积新revision。

本轮选择的native资源服务优先用类型化Image smart-pointer值保存本方pins，避免为C/S退休表新增需要等待GC的LuaImage。它不减少原生世界视觉内容，不要求所有公开Lua查询改为native，也不声称已经清除观测模块、官方或外部Lua已有的Image引用。若采用路径确实生成／持有LuaImage，其finalizer可控窗口或真实renderer保留责任必须单独落实；unref、全局collectgarbage、手调__gc和Lua state仍活均不能替代。

已知owner或context损坏时，不继续执行可能解引用它的普通reset／destructor，不把该场景交还Off，也不自动重新准备第二套世界。保持能够安全运行的消息／错误界面并说明剩余对象和停止位置；若连安全owner边界也不存在，明确为本进程不可恢复，不声称正常Shutdown必然能跳过损坏资源。强制终止及其后果不作为自动恢复实现，不从“退出请求”推导额外持久或实机授权。

## 6 持久归属与保护的结束位置

| 数据／调用来源 | Client准备至安全退休 | 退休后与进程关闭 |
| --- | --- | --- |
| 访客已有Continue／rerun／PGD／解锁／统计／成就 | 不读取Continue建世界，不保存／删除／覆盖；禁止服务器场景在内存进度、平台通知和官方扩展JSON之前产生副作用 | 安全菜单后仍保留访客原有归属，允许其以后自行开始／Continue。不得把“回菜单”实现成恢复或重写访客档 |
| 服务器run／奖励／完成与HUD展示值 | 只进本方复制／原生场景中已声明的非PGD状态；展示成就／奖励不再调用本地裁决 | 清场后不可被Shutdown／Manager版本同步当成本地进度输入；不存在自动合并访客档或回滚文件的步骤 |
| GameState及ESSM／VirtualRoom／ItemPool附加文件 | 在上游run保存／删除请求与官方整链副作用之前处理；原生write=false不能回滚已发生的sidecar或阻止caller继续 | 清理运行时容器与删除持久文件分开；为清内存不调用GameState Delete／DeleteRerun |
| shutdown直接PGD修改及flush | ST8的 `0x982D44`直接清counter[192]、随后522计数与本地保存，以及独立dirty／loaded分支，必须在进入之前隔离服务器来源 | 不把early unload当第一保护点；不整体return Shutdown阻断对象／Lua／renderer正常销毁 |
| Manager析构的SaveSyncing | 启动／选档／结束的合法本地同步有自身来源；不因Client连接去调用SetSaveSlot导入进度 | 保留源码条件下的正常本地同步，前提是服务器从未污染其输入。保护／归属协调随常驻模块存在到最后相关writer，不能随网络对象析构或early unload释放 |
| 本地Options／输入映射／明确UI偏好 | 只允许具名本地来源和字段；Load也可能复制／写INI，不按函数名判只读 | 正常保存设置可继续；不把Mod SaveData、未知JSON或run进度一概放为设置 |

**保护的常驻寿命与Client场景身份分开。** 正常菜单交还满足第7节后可以转Off，不能因为Manager要到进程末尾才析构就永久卡在Client；但归属策略及残留来源检查不能随场景对象被销毁。若还有服务器来源的待发callback、资源finalizer、run字段或持久请求没有解释，就不满足交还条件，保持Client。真正安全交还后，Host／Off及访客以后合法本地run沿原链透传，不全局禁用保存／同步。

这依靠构建前已生效的消费者分派、实际owner与按变化维护的来源记录；不能以dirty=0、最终文件未变、存档快照恢复、readonly翻回原值或任何哈希充当全过程无污染证明。R5不新增文件rollback，也不为存档归属建立另一套场景或并行PGD。

## 7 菜单交还和进程退出是两个终点

### 7.1 回到可交还的菜单

`DESIGN`：下面描述同一退休过程应达成的实际运行状态；达不到就说明缺什么，不通过伪造Manager字段强行推进。

- 服务器场景已停止玩法、输入、目标和事件publication，旧scope／start／restore／host排队请求不可能在恢复原生Manager后复活；正常物理松键缓存已按统一输入语义收束。
- 全部本次采用的原生成员、池use、Player roster／HUD、Grid／door、Camera／Room关联已清理或成为明确有效的可复用空承载；没有已释放指针、半初始化对象、未分类连带实体或仍会被Off路径消费的服务器run状态。
- 新Lua／draw使用已关闭，实际在途栈、raw队列和资源／借用按R4结束，所保留原生根满足后续原生菜单／启动／Shutdown的读取合同；没有依赖未来未知GC才安全的C/S持有资源。
- 必需的官方结构状态（ESSM、pool标记、room权重等实际改动域）得到受控收束，服务器来源的延后持久消费者已消失；访客Continue与允许设置保持归属。
- 真实Manager状态、MenuManager对象／选择页和请求字段处于已解释的菜单后态；worker不再发布旧任务，身份映射与输入已退场。满足后才转Off并恢复原生路径，而非仅把selectedMenuID改成已知数字。

现有MenuReady是连接入口及无世界Client的退休检查，不证明第二／三项。完整可调用菜单转换仍须依据本轮核定的native helper范围和明确UNKNOWN实现，不把这些语义条件说成已经有一个可靠native一键返回API。

### 7.2 正常进程退出

窗口关闭请求走相同停新工作与owner退休，但不要求先恢复菜单／Off。退出协调在正常主线程阶段处理本方线程与资源，不能在DllMain join、等待当前callback自身退出或等待永不到来的新网络包。允许原生Shutdown继续时，必要的服务器持久隔离仍有效，Game／Manager／ANM-cache、Lua state和ImageManager／context各自截止点按ST6／ST7满足；不把RunPreModUnloadCallbacks返回、worker finished或录制已flush当整体完成。

正常Shutdown若发生在准备／apply中，不能依赖下一轮Manager泵；必须有独立于录制的关闭接线及清理前缀。当前代码尚无原生Client场景，不能把已有退出shim升级为已解决本合同；R6负责统一清单与资源服务接线，17完整实现后的统一E2E再观察实际正常关闭与无法恢复行为。

## 8 未决项与后续责任

| 未决问题 | 具体范围／归属 |
| --- | --- |
| 可调用的完整菜单转换 | 已定位正常Room重建→roster／HUD清理及`0x933060`的state1后态；后者混入原版Online而不整体采用。菜单准备callee／参数、保留的原生空根及所有启动请求字段仍需落实成自有编排；17实际代表路径与R6接线不能全部推给31 |
| 部分构造与异常展开 | Camera替换、派生Init和已发布成员的EH／OOM／Lua error实际清理责任仍需逐采用路径解释；未知不调用通用destructor碰碰运气 |
| 官方后处理调用层 | ESSM具名内部Lua维护与Gameplay／Mod持久callback分开；Exit外层room权重／pool／LagMetric重置的实际Hook层次、可调用桥接及失败后态仍需落实，不能直接调用源码static或清全局表替代 |
| 全采用pass取消与LuaImage | 普通队列reset与图像remove已定位，但不是全renderer取消；任何新增LuaImage持有都须有真实退休窗口。R4的关闭custom-render分支不是额外阻塞 |
| 真实持久整链 | 当前官方Hook顺序、Startup／SetSaveSlot／Manager destructor同步、Shutdown直接写与所有保留callback来源需实现后统一E2E观察；静态合同不冒称无副作用 |
| 后续完整产品 | 31承接Host合法保存／Continue与各端进度归属；23—29扩展整局场景和所有必要类别，新增路径继承本合同，不能缩小原版完整内容来规避已知必要清理 |

## 9 主智能体审阅与收束

主智能体复读了ResetState与其外壳、Exit后段及RunActive条件、Room重建的旧对象析构／free与新对象发布、PlayerManager清理的当前Room读取及Free调用、HUD两段、Manager cleanup／普通菜单请求及state1／state3 Render分派；复读官方ESSM prehook／ClearStates Lua、Exit后处理、SaveSyncing与现有Client／退出shim源码。接受范围仅为上述正常路径、分阶段设计和明确列出的UNKNOWN。

主审纠正：子智能体阶段摘要曾把`0x933060`前段条件误称为Game EntityList非空，实际是`Manager+0x4AFB8/+0x4AFBC`且ZHL归属NetplayManager；Room dtor真正call在`0x732D55`，`0x732D53`是准备ECX。state3来自另一处cutscene请求，不能据此推定普通菜单请求的最终state；完整普通请求另在`0x933129`写state1，但整体不作为C/S入口。主智能体另补核Exit只在RunActive为真时清理，并在末尾才清该位；部分准备不能用假active或提前清位操纵退出。原生helper的伪码返回寄存器和残余参数不升级为公开ABI。

完成本轮统一人工文档审阅，核对事实／推导／设计口径、交叉引用、历史记录保留、前缀责任与保护结束位置；没有运行格式／链接校验器、程序测试、构建、游戏或图形实验，没有改IDB或留artifact包。研究不恢复旧gate、哈希或强制角色流程。

本轮交付失败方式、可采用边界、退休与保护结束合同；原生Client运行时和安全返回菜单仍未实现，不记功能PASS，不进入测试。
