# 13：显示委托安装与原生显示边界

## 2026-11-14 中途加入与原生读回

布局版本升至 75。World 基线新增 Boss 演出代次、双 Boss ID、房间参数和当前主屏／肖像帧位。Boss 事件仍按可靠顺序消费。首次见到当前代次时构建本地原生演出，后续帧更新姿态。普通转场优先使用已有的旧画面。首次加入且无历史画面时，两张 surface 从当前已安装房间捕获，阶段和进度沿用 Host 当前值。

- 全局 ItemOverlay 保留 Game 承载。每个控制器可借用独立的本地原生承载，两份 ANM2 使用原子字段安装，Player 槽使用原生登记，绘制跟随 Game 内原生 Overlay 调用点。退休先撤 Player 登记，再释放 Sprite。
- TMTRAINER 在本地准备稳定的负 ID 配置，借给原生程序道具表；1024 和 512 两张本地图集按负 ID 槽位使用已选资源绘制。原配置的 gfx 为空时使用本地 TMTRAINER 图标，原生随机生成的像素图案留在 Host。程序配置、表、图集和 Sprite 各自持有本地资源，场景退休时归还原生图集槽。
- Manager 分别轮转当前 World／Body 写入器的 record 和根块。读回用安装时的字段类型和当前原生承载。标量、掩码及已知选择器差异按块修复，地址、附件或资源身份变化请求两流完整恢复。轮转从每流每次最多 128 条操作开始，发现差异时另核对受影响整块。修复沿用已提交版本。

运行表现及耗时留第 15 步统一验收。

## 2026-11-14 正式连接接线

布局仍为 74。正式 `StartClient` 注册二进制准备接口，与网络接收器共享 Peer 内存账户。Manager 消费资源和批次邮箱，依次准备承载、安装原生字段及成员、装配显示、回放事件和发布。网络接收器仍使用原票据决定 AppliedAck。已提交 world／body 批次保留原生租约，后继批次接管后按依赖退休。部分写入失败撤销两流的原生差分资格并请求完整恢复。

- 房间切换先保留旧 surface，再排空旧路由。新路由用已安装的场景捕获新 surface。镜像地图保存项只在原生构建栈有效。挑战 32 用两份配置各自的 Host 焦点替代指定调用点的随机选择。
- 音乐按请求和播放代次调度原生 Play／Crossfade／Fadein／Fadeout、Queue、层和 jingle。SFX 从已准备的本地资源与实际样本槽播放，直接执行已核的后端参数段。循环声按实例与样本更新和退休。相邻同局路由保留音乐及循环声。
- 全局 ItemOverlay 在新代次调用原生 Show，随后安装 Host 状态、延时和两份 ANM2。覆盖 Player 引用登记与撤销。道具横幅和 fortune 交 HUD，Boss 事件抑制原生入口内另起的选曲和环境声清除，转场当前值继续由 Q11 管理。
- 安装药丸效果／识别、水流及星象房显示值。StatHUD 消费 Host 总率、占比和资格。天使占比的两处调用保留 J273 的 XMM0 返回 ABI。

**已知范围**：全局 ItemOverlay 有实际承载，多控制器 Overlay 的本地定位仍缺证据，遇到此目标时原生预检拒绝。程序道具定义尚未接入本地负 ID 配置／图集 owner。首次加入活跃普通转场仍需可用的历史画面。Boss 迟加入仍需同目标演出资源或已接纳的开场事件。道具横幅 typed 分支用本机配置，fortune 用本机语言查询已选索引，查无译文时使用 Host 提供的行。原生背景构图、显示漂移与安装性能的运行结果仍留第 15 步。

以下保留 2026-11-13 组件实现时的边界与静态结论。

**2026-11-13 Host后续进度**：[worker／Peer二进制发布](implementation-11-worker-peer-publish.md)已接票据消费、事件队列和ACK回收，Host正式来源已替换旧DTO。本页Client接收／原子安装／回放边界仍未因此自动完成，当前Client不声明二进制能力。以下内容保留本页原实现时点。

日期：2026-11-13（Asia/Singapore）。依据：[显示委托方案](client-display-delegation.md)。布局仍为72，没有新增线上字段。产品 Release／Win32 纯编译、链接通过；没有运行游戏、E2E、校验脚本或其他测试。

## 当前交付与运行边界

本轮实现显示侧安装器、Manager调度、原生Game::Render包装、Q13镜像和音频维护。代码入口为 `mod/src/game/client/display/`，Scene／session接口为 `ClientStageDisplayWorld`、`ClientCommitDisplayWorld`。

**正式运行尚未切换到此链路。** 当前网络／Scene仍接旧DTO；第12步完整validator、主线程批次投递、二进制其它域原生安装和Host新采集尚未把这些入口串成完整生产路径。本轮不把旧DTO修订当二进制版本，不从旧JSON补造Q8／Q11／Q12字段，也不为显示片段单独发送AppliedAck。旧手工绘制留到第14步删除；不能据本轮组件入源宣称双端显示、切房或音频已经运行成功。

接入顺序：后台在完整候选上完成来源、资源、引用、拓扑和生命周期检查，从完整world影子与同目标Room／Level描述构造 `ClientDisplayWorld`；Manager在结构写入前Stage，准备种子树并捕获旧surface；其它域安装成功后、发布／显示消费者／事件播放之前Commit；然后由第12步owner完成原始票据、缓存提升和ACK。未知处理器不能用成功占位。新的安装入口目前尚无二进制后台投递调用者。

## 安装与帧号

生成器新增 `state_display_install.inc`：从需求计划生成45项具名纯值复制、数字字段索引、Effect到期帧处理表，并在构建期对照Q12枚举。原生偏移只来自可信生成表，网络不能提供地址或函数。Room／Level描述尾部追加RawCurses、SpawnSeed和Q8角色；旧角色编号保留，不再必读已退役的effective_curses和Error谓词。

| 项目 | 已入源行为 |
| --- | --- |
| Q8 | 安装颜色、闪电、暗度、dizzy、Bloom／Hallucination／Retrovision、fade alpha、HUD可见性、水色四端点及计时、两组shockwave、地狱累计／绘制位移；不运行生产段 |
| 阈值 | 震屏正数→0清两个本地偏移；首次绑定／F回退请求本地历史surface；周期HUD请求跨30边界合并，type2期间保留待处理请求 |
| Q11 | 九个Host转场值直接安装；Start／Update不执行；ChangeRoom常驻hook拦截；旧／新surface在安装前／后分别捕获 |
| Q12 | IsPaused返回Host位；13项分支选择U／Pα／M／Room显示／P；Pα仅在已安装primary alpha>0时运行 |
| SpawnSeed | 描述包含完整u32；当前seed为0或本地绑定不符时拒绝；Render之前写入；不恢复旧Error谓词 |
| raw诅咒／Seeds | 安装Level／Game两个raw值；从至多80个排序唯一id构造本方MSVC树并借给Seeds的head/count；不复制Host指针、不调用种子生成或效果setter |
| 方案B | Room原始基准直接安装；TimeCounter和完整影子中的Effect到期帧按目标F还原，块未变化也处理；最后写Game+25A70，Render不借换F／M |

Effect转换列表在纯值准备时生成，主线程只遍历该列表；其它Entity原始frame_base仍由其正式字段安装owner承担。正常退休先撤Seeds借用、恢复本owner改过的Game／Manager标量，再允许Game／Room生命周期继续。本方树只由本方释放。

提交后，本地Manager由该owner进入游戏显示态2，完整Manager更新仍由C/S整体跳过；音乐的dizzy／红房分支和Manager::Render读取真实本地游戏承载，退休先归还菜单态。不借state3进入演出，不执行StartGame。

## Manager与Render

Manager边界复用唯一输入泵，world安装之后运行显示根。偶数M按Q12选择30Hz序列；普通Room显示包含自有GetRenderZ全排、FXLayers(false)、floor主／overlay和地狱Sprite，26号背景Idle按7F93D8—7F9449维护。随后按本机CameraStyle二选一运行相机，保留paused／MINECART及override清理条件；再SFX dirty刷新、音乐tick、本地cursor，最后推进本机M。关闭插值的奇数快返和本机A／B保持均由唯一M owner处理，不放行Odd、Online或混合Manager尾部。

本地失焦／控制台阻断只抑制30Hz批次和相机，不改Host终结分支或同步paused；音频仍按有效尾部周期运行。没有放行完整Console／PauseScreen更新。cursor在既有输入泵之后只调用一次9F26E0（ECX窗口、EDX键0，会消费过渡态3），使用本机窗口坐标。

Game::Render包装持有真实Scene／图形借用并建立PresentationDraw用途，只允许该树内的GameRender、RoomPreRender／RoomRender及具名显示入口。主循环原有Manager::Render负责调用，不额外补画；新链启用后MenuRender不再手工绘制world。Q6回调在具名显示、捕获和音频作用域内走官方分派；保存／PGD／解锁保护保持。

surface捕获作用域固定自己的显示来源：旧画面读旧目标，新画面在publication前读同批已安装目标及paused，不回落旧DTO绘制输入。房间身份变化或同房转场从mode0启动时先捕获旧画面。原生历史surface尚无本地资源owner时返回`display_history_surface_pending`，不让6F7D28读取空对象。

Projectile hook保留Q10：保存PositionOffset.z的4字节，返回时先核同Room、实际对象映射、shape、index和birth再精确归还。旧绘制输入注入和时间借换在新链中旁路；旧文件不在本轮删除。Render结束纠正已核Q8写集；F意外改变则停止显示，不盲目写回旧对象。Manager也检查已提交显示目标的F与根绑定，丢失时进入恢复，不静默回落旧DTO绘制。

## Q13镜像与本次静态核定

新增常驻 `client_map_cache`：J273 VA9635B0／RVA5635B0，ECX为MinimapConfig，无显式栈参；三个原生caller共用此包装。反编译的bad-sp不作为额外参数依据。原函数在镜像中恰好执行一次，返回或栈展开时归还两个vector头（含capacity）。本次IDA确认：**descriptor+3C非零才扫描保存项／保存Grid**；摘要presence须与已安装DisplayFlags一致，不强制扩大可见性。本地可见ordinary描述必须在本目标中。输入仅为0x78／0x20步长的本方POD，不移交析构权。

代表项每类别一条；Pickup的intStorage3=0、位置=(1,1)，避开原生排除条件：

| bit／帧 | type／variant／subtype |
| --- | --- |
| 0，0—9 | 5／10；subtype依次2、1、12、8、10、3、6、11、7、4 |
| 1，0—3 | 5／20；subtype1、2、3、7 |
| 2 | 5／42 |
| 3，0—2 | 5／30；subtype1、2、4 |
| 4，0—1 | 5／40；subtype1、4 |
| 5／12 | 5／300；从本机已加载card配置选type!=2／type==2，不执行随机 |
| 6 | 5／100 |
| 7，0—1 | 5／70；subtype1、14 |
| 8，0—1 | 5／90；subtype1、4 |
| 9，0—9 | type5；variant52、51、69、69、50、360、56、53、60、57；两项69分别subtype1／2 |
| 10 | 5／350 |
| 11，0—8 | type6；variant12、8、1、2、3、16、10、17、19 |
| 13，0—5 | type6；variant4、18、5、7、9、13 |
| 14 | 保存Grid type18 |
| 15 | 1000／161／980 |

地图输入保存为明确值向量比较，仅输入／绑定改变标脏，不按每个world重建FBO，没有哈希。挑战32仍缺第二批Host MapFocusResult，安装前明确pending，不让本地随机代替Host选择。

另核定RoomTransition::ChangeRoom完整入口为**8254B0**，825700是函数内调用点，Hook不挂中段。纯几何／scroll补偿按旧／新descriptor、维度、房间尺寸、镜像和本地scroll计算；没有搬运其玩家、随机、选曲、保存或换房分支。

## RE-10、失败与未完成串联

SFX乘数只拒非有限值，保留负值；max(m,0)或本机音量改变才dirty。Clear正常返回的Hook通知合并官方setter已完成的刷新。音乐VA7D7DA0每有效尾部周期一次，不因相机paused停下，也不按到包／ΔF补跑；读取本地种子树、dizzy、descriptor红房位和本机Options。

`ClientDisplayReplaySfx`给第二批已接纳事件owner提供回放leaf：核合法id／样本，只把该sound+4旧截止暂置-1，以FrameDelay=0调用原Play，退出归还旧截止；普通本地播放保留节流。**事件协议与顺序／去重队列、音乐请求、loop集合、BossReplay仍待第二批／Host接线**，不以旧voice正文冒充新事件队列。

没有历史world的mode2／3初次baseline、未就绪的Boss资源均返回pending。活跃转场resize或旧surface对象／尺寸失效停止该显示事务，交完整恢复，不消费空surface。准备分配失败且未进入捕获时保留旧目标；一旦可能写入／捕获异常则带written失败，停止发布并正常退休，不能继续把污染目标当差分base。

首轮纯编译暴露C++20影子类型进入C++17 SDK单元导致的C4596及旧state()名称冲突，已拆成纯值桥。最终产品纯编译、链接通过；上述不是运行测试结果。功能、隔离、恢复及性能统一留第15步。
