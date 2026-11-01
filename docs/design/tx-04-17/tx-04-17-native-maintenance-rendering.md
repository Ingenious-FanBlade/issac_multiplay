# tx-04-17 R3：停止玩法后的原生维护与绘制

当前记录：2026-11-02，Asia/Singapore。目标为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。**I4代表路径已实现，并由根智能体在同一tx的第21次U统一实机验收通过。** 根亲自对照Source P与Client U三图：baseline除FPS外无可见差异，Player身体完整、Lost小图问号／curse图标正确、多余黑椭圆消失；pose目标光束正确。14个自动用例全部通过，正常退出码0、严格存档比对通过。这里是人工视觉对照结果，不声明逐像素相等或全内容支持；I4未自行构建、运行测试或控制游戏。

第20次T已证实Player body实际提交透明顶点、小图原生Frame也已执行；阴影位置／顶点与viewport均正确，故不是缺资源、缺draw调用或实体offset。根与I4复核确认：graphics+0xD4只有一个共享depth renderbuffer，切RT仅更换color texture；`PresentWithoutSwap`消费队列后把`BA4714`深度序列重置为-1，却不清depth。原版`RoomPreRender 803E6C`调用`Present 9E4170`，在非空RT上于`9E41D0`清共享depth。旧组合错用WithoutSwap留下半尺寸shadow深度，挡住主floor形成左上黑点；主画面中途多次flush重置z，又挡住已提交的body／minimap。第19次S的14个自动用例虽已通过，仍有这些视觉缺口，未据此提前接受。

最终顺序为shadow／light全部先预绘；只在本方刚设置、非空且指针匹配的offscreen目标上调用原生Present结束预pass，再恢复外层RT。主画面floor→Z分层实体／walls→shading／shadow→主层Player／Effect→light合成→ray→HUD保持连续队列，最后一次WithoutSwap消费并由RAII恢复RT／blend／shader。没有直接清depth、改z游标或补实体坐标；未知外层main目标不调用可能Swap的Present。上述修正经U实际消除了三处视觉缺口。

当前边界仍是所选普通Basement、稳定房间、空装备普通Isaac和Effect57、来源明确零水量、id7 lighting及已选ray、Lost小图与关闭合作选择面板。非零水面／poop、其它必要FX、普通地图拓扑或未闭合角色／装备明确拒绝，不扩大为任意房间／完整HUD／所有内容Applied。既有原生玩法更新和通用Lua隔离、资源／context借用及正常退休约束继续生效；下文待验收标记与旧pass顺序保留其当时事实，以本段最终实现和U结果为当前记录。

2026-11-01原始研究状态（历史）：目标为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。状态：**本轮公开接口／源码阅读、必要静态逆向与主审形成维护分派合同；尚未实现或运行原生 Client 世界。** 同属 tx-04-17，不另拆研究 tx。本轮只更新设计文档，没有修改代码、绑定、游戏文件或 IDB，没有构建、测试或实机操作。

现行目标与顺序见[主流程](tx-04-17-workflow.md)，前置事实见[R1 原生初始化](tx-04-17-client-scene-lifecycle.md#r1-results)和[R2 对象生命周期](tx-04-17-native-object-lifecycle.md)。ST3／ST4 的普通、odd、FSM、loading、动画事件与派生绘制证据继续有效；旧“原生对象一律不可写”及“一律禁止 HUD 后处理”的设计按本文细分，历史事实不改写。

## 1 采用范围

**R6接口归并（2026-11-01）**：[模型／角色／应用器](tx-04-17-model-role-applier-integration.md)将应用结果、已发布显示状态及独立展示时间分开：维护／draw只消费一致的已提交实例和资源代次，不回写A、不推进Authority。跨帧应用只在完整子步骤间让出，影响旧owner后停相关展示；独立Component和音画事件不升级成完整世界，事件接纳与实际效果完成另行协调。未解释的实际pass／官方Hook层次仍需补齐。

**R5回补（2026-11-01）**：[失败退休／菜单交还](tx-04-17-failure-retirement.md)将当前在途栈返回、采用pass正常消费／已解释取消、原生owner与HUD资源清理分段管理。已停显不等于资源可析构，旧context不符时不为排空而恢复未知GameRender；Room重建／Player清理后仍需HUD与菜单实际后态。普通退出与进程Shutdown有不同终点，后者不能等下一轮Manager更新或依赖录制开关才接线。

**R4回补（2026-11-01）**：[资源与在途合同](tx-04-17-native-resource-lifetime.md)分开当前draw活跃栈、Image raw提交与owner析构。官方buffer复制Entity／Animation上下文的分支被EnableCustomRendering=false及注释patch入口关闭，只记未来启用风险。资源重建／Morph／析构排在采用pass的实际CPU消费结束与下一轮提交之间；Image pins只保图像。全部采用的shadow／light／HUD／loading pass纳入使用范围，未知取消路径停新发布并交R5收口，不为关闭的扩展分支增加清理前置。

Client 继续使用已有 Game 下的原生 Room／Grid／Entity／Player／Camera／HUD。服务器提供最终玩法状态；客户端可写这些对象的目标值和必要显示缓存，复用已解释的原生排序、资源与 draw helper。不运行整条 Manager／Game／Room／Entity 更新来补显示，也不另外建立一个运行中的 Game。

### 2026-11-02 I4实际接线（等待整个tx统一验收）

第12轮统一实机的完整截图显示：Backdrop原点修正后几何已对齐，但墙边暗渐变、Lost minimap、HUD run徽章与合作面板仍有差异。以下为该轮后的静态修正，尚待根智能体继续同一整体验收。正式`RoomPresentation`现采实际`shading` Image身份及采样参数、`hud_context`的最终Manager成就禁用查询和seed／rerun／debug／challenge／difficulty与4槽合作选择面板门值，以及`minimap_runtime`的最终curse、小图／大图alpha、state、shake及实际Question Image。Source不会因Client只支持代表路径而隐去其它正式域；Client明确只接关闭合作面板、普通空装备Isaac、普通Basement无特殊darkness／lightning混色、Lost小图state0且大图alpha0。普通地图依赖楼层拓扑，仍明确拒绝，不将缺失静默当空地图。

原生`80513E`绘Z<5000，`8051A0`绘墙，`80548B`继续Z<10000，`805954`按Backdrop offset画实际shading Image，`805980`合成实体shadow，然后才画主层实体。`7E814F`的普通Basement shading来自真实`gfx/backdrop/shading.png`；Client使用来源实际资源名，不重跑含FX随机选择的`7E7670`。shadow维持原版`803E39`的`usesScreenSize=false`。surface改用真实Image vtable+24位置入口／light的+1C目标quad入口生成实际像素source UV，补齐shader3附加属性；不再把整张padded纹理UV=1铺到屏幕。`67E7B0 __thiscall(ECX=ColorMod, stack=vertexBuffer,Image,counter)`、`ret12`；调用前显式增加临时strong，callee在尾部Decr，原surface owner不被每帧减引用。light surface总是先按`803FC6`用basic-color shader和solid-image四角色入口填shadowColor，再用`803FFB`实际premultiplied-alpha blend和普通分支的lightColor调用layer4，最后按原生blend合成。

右下GET／Space为`HUD::Render 97DC46`的合作面板，并非MenuManager漏画；其PlayerManager+0C／+14索引门值按stride20逐槽在HUD draw作用域供应来源值，只有同槽两个索引均非负时才打开，正常首槽`(0,-1)`必须接受。左红徽章包括`Manager::AchievementUnlocksDisallowed 9328C0`的最终查询：它及官方patch可读取访客PGD、mod环境、console、online、Manager状态与Game daily／debug；仅改Manager状态不能复制Authority结果。根统一Hook在`ClientPresentationHudAchievementQuery`确认有效主线程、Render owner、HUD同步栈时返回来源最终bool；其它调用原样super，持久mutator隔离不变。第15轮又纠正了`unk=false`过滤：`97D001`实际`push ecx`，native `9328C0`完全不读栈参数、官方Hook只透传，因此其参数值不能充显示查询资格。临时seed／curse／challenge／difficulty／rerun／debug和面板门值正常／异常离栈恢复，Manager状态不改写。

`Minimap::Render 966680`已由HUD原生调用，采用Lost分支无需Refresh／楼层拓扑。`965740`已完整静态核定只加载内嵌ANM／Question Image、创建地图surface并设置显示数值，不调用玩法／Refresh／Update／RNG；随后每次显式核Question Image的实际格式。Minimap长寿命资源继续由Game owner持有；首次准备保存header0x18、两个Config+11C..15F纯数值段和shake12字节，明确排除Config+114的Image strong，正常退休恢复这些显示缓存。首draw诊断另记录实体shadow-layer ID与body可见性在shadow／light前后实际值，仅收集因果，不擅改I2／I5姿态或隐藏必要层。

第15轮O实际Effect已持续可见、shading已出现且GET面板消失；shadow-layer／body LayerState在前后均未变化，撤回“shadow pass隐藏Effect”的因果假说。Minimap还依赖`966802→827350`的RoomTransition fade；既有正式room.transition已采mode0，Client预检只接此稳定模式，并在HUD draw临时供应、离栈恢复，避免菜单mode2/3进度使小图alpha归零。Backdrop装饰的正式`backdrop_runtime`补实际construction_seed、rng_seed及三个shift，均用Lua整数精确复制uint32；I1仅以构造seed和真实shift恢复该独立装饰流，不能把当前后态rng_seed当初始种子重播。上述后续修正继续由根统一验收，尚未用未跑的截图结论替代实际结果。

第17轮Q在draw前因本地`Room._waterAmount`非零拒绝，而P正式来源值明确为0。原实现虽采水量却没有应用，错误地让本地初始化结果决定目标支持范围。SDK `Room.zhl:161`直接返回`+7240`地址，官方`LuaRoom.cpp:102—109 SetWaterAmount`也只写该float、无额外helper或owner副作用；现写前只拒绝目标非零，在已登记written的展示值应用段复制Authority水量0，draw继续确认本地仍为0。非零目标仍要求真实水面／反射适配，不能以删除必要视觉flag冒充支持。

第18轮R继续在draw前因`presentation_foreign_fx_owner`拒绝。补核确认Room ctor为FX构造真实空vector并初始化loaded／avg／bounds，但从未写FX+30 selectedRayGroup和+34 poop倒计时；`96BD30`也不写这两个scalar。Install原已写selected，却漏写正式poop值，并错误把未初始化倒计时混入owner检查。现owner只比较FX／ray完整vector三指针与本方已安装记录数；Install显式安装来源poop倒计时（代表0或-1），draw再逐项核loaded／selected／poop／avg／bounds最终值，正倒计时仍拒绝其必要cloud消费者。退休只核真实vector归属，不因未安装的heap显示值阻止候选清理；真正vector不符时只记录一次精确begin／end／capacity、安装数与显示scalar诊断。

**同日统一E2E中的必要修正（由主智能体实际执行，I4未控制游戏／运行测试）**：首次实际来源为Burning Basement并带Effect87余烬，之后普通Basement来源仍在`presentation_room_fx_requires_native_pass_adapter`拒绝。不能把这些实际来源改成“无FX”来满足初稿范围。主智能体读取真实安装的`extracted_resources/resources/fxlayers.xml`确认普通Basement必有id7、stage1／altStages7、layer=lighting的overlay；本节以下初稿的“所有非空房间FX均拒绝”范围已由本段细化替代，实际失败事实保留。

修正后的正式`presentation_runtime.room_fx`复制Authority真实FX向量中已经选定的Image资源名、过滤／wrap／shader状态、全部20字节pose中的位置／速度／翻转、FX平均位置与bounds，以及真实ray group数量／poop倒计时。Client支持代表id7／layer4；不按seed或XML重选覆盖图，不调用含共享随机的`FXLayers::Load 968E10`／`Update 96BF40`。第7次统一E2E实际又在`presentation_fx_rays_or_poop_adapter_required`拒绝：普通Basement有两组ray容器。单有group数量不能证明当前是否需要光线pass；正式来源现补每组7项几何、全部ray的Image与过滤／wrap／shader、opacity／speed／phase、真实当前组索引`ray_group`和最终`ray_lighting`／`ray_darkness`条件。来源有界采集最多256组、每组64条（与Client代表预算分开），应用器只接最多4组、每组16条；不把不支持的真实成员从来源隐藏。Lua正式编码器将空table写成`{}`，仅该精确空值按空成员接受。其它FX、poop和特殊背景pass仍明确拒绝。`Game._screenShakeOffset(+25A84/88)`与`Room._renderScrollOffset(+1204/08)`也由正式来源复制并应用，不继承菜单／之前Host的值；首次写前保存Game旧shake并在正常退休恢复，Room继续由I1的真实owner退休。

额外静态依据：`96BD30`确认FXLayers`+18/+1C/+20`是FX vector三指针，stride`0x4C`；`+24/+28/+2C`是RayGroup vector，stride`0x2C`。FX`+4`为Image strong pair，`+0C/+10/+14`是pose vector，stride`0x14`，`+18`为采用pose数量，`+2C`是pass分类。普通Basement必要调用是`96DB20`（原生caller `803E20:8041B3`），不是处理layer2的`96CD10`。本方在原生light surface中调用`96DB20 __thiscall(ECX=真实FXLayers, 栈=KColor* shadowColor)`，raw尾部`96DF72 retn 4`；它仅读取已复制pose／bounds／滚动并提交原生Image draw，不运行FX随机更新。

FX候选记录与pose都经目标exe `A86DE5 operator new`分配，预算最多8条FX、每条16个pose，每块均小于4096字节，不混用本方STL allocator。资源候选的Image strong pair由`Manager::LoadImage`实际返回，完整准备后才转交Room实际FX vector；未发布候选仍由本方native owner承担。换代／退休先停止新draw、退尽活跃栈及普通／透明CPU队列，再用原生`96FFD0 __fastcall(ECX=begin,EDX=end)`析构pose vector及Image strong，raw尾部`970054 retn`无栈参数；pose析构`41EAB0`使用同一原生free。最后由`A8717B free(void*)`释放FX记录块并将Room vector归零，避免随后Room析构重复释放。缺图／预算失败保留实际候选前缀用于退休，原生异常仍标不确定并保持隔离，不声称任意异常可回滚。此修正由主智能体继续同一tx的统一E2E验证，本文不提前宣称通过。

Ray补核纠正：`FXLayers+30`是**有符号当前组索引**，不是时间float；`96D26E`读取后`test/js`拒绝负数，`96D4F3`按44字节选择组，`96B680/96B72C`写`-1`。RayGroup stride44，前7个float依次为xMin／xMax／topParallax／bottomParallax／rayLength／raySpacing／perspective，`+1C/+20/+24`为Ray vector三指针；Ray stride20，`+0/+4/+8`为opacity／speed／phase，`+C/+10`为Image strong pair。不能因为普通XML有两组就同时画两组，也不能把空选中组交给原生：`96D58A`在循环判断前先除以实际ray数，外层对空组直接跳过，保留目标的空成员证明。

采用光线pass为`96CE20 __thiscall(ECX=Room._fxLayers)`，无栈参数，`96DB1A ret`；真实caller为`RoomRender 806859/80685F`。它在当前代表的仅layer4／poop<=0条件下跳过foreground FX与poop分支。入口会在Backdrop57绘制特殊Effect166/sub951，故选中非空ray时实际Backdrop只接普通Basement的1；不让该特殊分支进入代表路径。`96D238—264`还读Manager lighting和`(Game._curses | Game+25AC8 | 6F55C0()) & ~6F5750()`的darkness位；两个helper只读真实run-effect map，不是随机生产者。来源采集真实最终条件，准备与draw分别核对本地采用原生上下文一致；不一致明确拒绝`presentation_ray_context_requires_adapter`／`presentation_ray_context_changed`，不让菜单或旧Host遗留条件静默改变服务器必要光线。索引`-1`、真实空选中组、来源关闭lighting或最终darkness成立时不调用ray pass；没有删除视觉flag来凑代表场景。

`96DAAA`在每条ray绘制后执行`phase+=speed`；本方`RayPhases`栈owner保存全部原位phase并在正常／C++异常离栈恢复，重复draw不推进服务器相位。`412AC0`为sin helper，不是RNG。复读纠正：`67E7B0`实际为ColorMod顶点补全，写4个stride88顶点的colorize／offset／textureSize等附加属性，并消费按值Image临时strong；它不是CPU入队入口。原生Image Render建立batch，本方在同一借用内`PresentWithoutSwap`消费，最终检查普通／透明队列实际为空。ray候选组与成员均用同一目标exe allocator；每个placement完成即记录constructed group数／ray end，缺图等已知失败只析构真实前缀。候选及已安装的组由`96F760 __thiscall(ECX=&group.rays)`析构（内部`970060 __fastcall(begin,end)`对成员Image strong逐一Decr并释放ray块），随后目标exe free外层group块；Room vector清零后才交原生Room析构，避免重复释放。异常前缀不确定时继续保持隔离，未声称通用恢复。

实际来源`Image._name`为`resources/gfx/Overlays/Basement/1x1_overlay_3.png`。`Manager::LoadImage 931B30`经`8D2AB0`处理逻辑资源名，再交Image manager；本方只将已核定挂载根`resources/`剥去，传`gfx/...`，随后核对返回Image._name与来源完整挂载名一致，避免重复添加根或加载其它资源。ray图片走相同具名路径和实际strong owner；没有重读XML或按seed选择。其它挂载根当前明确不采用，不能把这个归一化外推成任意mod路径处理。

第8次统一E2E由主智能体采到真实ray成员3／5条、当前组索引`-1`、lighting=true、darkness=false，故该实际目标保留完整ray资源但当前不需要ray draw；不能把该来源解释为没有ray。第9次实际创建Player(instance1)和Effect(instance2)、通过两者原生状态／资源检查后，在`presentation_ray_resource_candidate`拒绝，随后安全退休到native_safe_menu且pending_owners=0。主审据来源ray shader3／filter0／wrap1，与overlay shader1／filter1／wrap0的差异要求补核，未把候选失败或安全退休记为展示成功。

静态修正：`96A223`的overlay与`96B3D2`的ray确实均先用Position descriptor `BA47B0`调用`Manager::LoadImage`，但ray随后`96B488 push [AACB00]`、`96B48E call vtable+3C`执行`Image::SetVertexFormat`。`AACB00`存真实descriptor地址`B88760`，准确SDK参数类型为`KAGE_Graphics_ImageBase_VertexAttributeDescriptor const&`；该terminated descriptor八项format为`5,6,7,4,3,2,1,3`，终止format0，stride88。`9E2210`先按descriptor指针身份将Image shader设为3，再复制format字节、属性数与stride；Position descriptor对应三属性`5,6,7`、stride36、shader1。故shader3不能只等待Host缓存偶然保留，也不能直接改shader标量冒充完整格式。本方每次加载ray都按原生caller显式调用该虚setter，overlay也显式设置Position；再核验实际shader／属性数／stride／format字节与正式filter／wrap。该修正只保证每次正式准备执行同一具名资源子段，不主动释放全局cache或更改其归属；实机结果继续由同一tx统一验收提供。I4未运行构建、测试或游戏。

第10次统一E2E已完成baseline原生提交及重复／change／back，后停在I2 remove／replace；主智能体实际查看Client截图发现右侧裁切与Player偏右，最初把保留菜单projection列为推断。第11次同条件Host-source对照同样右裁且Player／HUD坐标相同，故撤回“仅Client投影导致”的因果解释，截图DPI由主智能体另修。静态链也显示`92D946/94E`、`92D969/971`及`92DBD5/DD`、`92DC14/1C`成对维护`g_WIDTH/HEIGHT`与`g_OrthographicWidth/Height`；`9E85B0`据实际target／usesScreenSize重建投影，`67EEB0`恢复target时重走该链。没有据截图比例盲写640／512或另造Camera矩阵。

Host／Client对照同时确认Backdrop相对实体有垂直偏差，来源`Room.GetRenderSurfaceTopLeft=(-4.5,15)`、scroll约`(1.5e-5,0)`、shake0。已核`703F05—46`的独立纯显示标量是`Room+11FC/1200=((g_WIDTH-338)/2-52,(g_HEIGHT-182)/2-52)`；RoomRender`804E81—EBE`再合成该origin+scroll+shake，分别在`80501D`／`8051A0`传给Backdrop floor／walls。本方补上该本地视口派生缓存，在Prepare及draw维护，只向Backdrop传完整offset；Entity已自行进行世界→屏幕变换，继续使用其原生scroll合同，不能把surface origin重复加到Player。I1的Backdrop.Init仍由Finalize在I2真实Player／EL有效后执行，I4随后维护，不重新加载另一套Backdrop。

Effect57 baseline不见而pose后出现的现象尚由主智能体核I5资源／层姿态缓存。静态`600ECC—601005`每次读取`Entity._pos(+33C/340)+_posOffset(+34C/350)`并重写screenPos`+1A4/+1A8`，variant57走默认`603121—12F`直接把该位置交内嵌ANM2 renderer，未发现需额外复制的独立Effect位置缓存；因此本轮没有为现象猜写Effect字段。修正后的Backdrop像素仍由后续同tx统一实机验收确认，I4仅进行图片阅读、源码／IDA静态分析及窄实现编辑。

`mod/src/game/client_presentation.h/.cpp`负责完整纯值候选，`client_presentation_native.h/.cpp`负责C++17原生窗口。正式Room域的`presentation_runtime`由`ClientPresentationCaptureLua`复制真实floor／wall／FXParams／water／HUD可见性、24个心形描述及10项StatHUD显示缓存；概率值来自服务器已经存在的缓存，不运行Client概率生产者。缺描述、非有限值或不支持的必要pass在写前拒绝，保留旧完整展示；解析成功后才替换候选，原生写入后的失败不声称回滚。I1在I2完整成员／资源提交后执行维护，完成后才发布；I3在Manager仍负责clear／flush／swap的窗口替代`MenuManager::Render`，实际像素来自原生世界。

当前代表范围为单普通Isaac、空装备和Effect57子型0／1、无Grid／水面的普通Basement，保留id7 layer4 overlay、该背景的已选真实光线组与普通Player必要光照。正式采集直接检查真实FX layer／ray group容器及poop FX计数；存在时`room_fx_present=true`，并由成员描述预检采用范围。不清空这些容器、改flag或改采样值来放行。其它角色／Grid／水面反射／其它房间FX仍是明确未采用范围，不能把本节称为全类别或完整可玩世界。

实际pass为：原生GetRenderZ全排 → native RenderShadows到原生Shadow Surface → Backdrop floor → native阴影合成 → floor层实体 → Backdrop walls →其余原生Player／Effect组合draw → native实体light与layer4 overlay合成 → 已选native ray pass及phase恢复 → HUD::Render → PresentWithoutSwap消费CPU raw提交。逐pass显式保存目标／blend／shader；不调用GameRender、RoomRender、RoomPreRender、Entity::Interpolate、PlayerHUD::Update、HUD::Update／PostUpdate或StatHUD::RecomputeStats。一般原版／官方Lua回调由I3隔离，官方插值不经GameRender启用；实体与HUD的真实draw仍保留。该静态组合是否产生正确像素尚待统一实机验收，不把void返回或CPU队列清空记为视觉PASS。

`render_sort`的空表／共享scratch／渐进全排问题由固定256成员预算的本方全排处理，比较键仍来自实际native vtable+0x34，tie用原生index；不写Game帧奇偶。超预算、无效成员、重复成员、Player关联不唯一及未采用类别均停显。维护不推进Authority tick，不调用动画Update，不消费动画事件；HUD心形字符串由本方owner持有，借用退出及实际普通／透明CPU队列退尽后，先撤HUD字符串／Player关联，再交I1原生Room／PM／HUD单一owner退休。

本次额外只读J273补核：`7E08E0`全部分支仅创建／加载具名surface和图像、设置vertex format／filter／wrap，可作为资源保证子段；不采用其caller `703E40`的其它全局操作。EntityList ctor `413270→413662`已加载`gfx/shadow.png`到`+0xD8`。`Room::Init`的`7E8DB7—7E911A`分别Load真实内嵌`Room+0x6E40`的`gfx/LightGradient.anm2`和`+0x6F54`的`gfx/SpotLight.anm2`，layer0 min／mag为1，默认blend为`0,1,7,1,7`。I4仅复用该资源子段的具名Load／姿态，不执行完整Room Init。补读`7FCA50→7FD03D→7FD0F8`确认普通空装备Player仍需LightGradient，已保留真实`RenderEntityLight`，不能以空装备推导灯光no-op。`805980`阴影合成常量为`(1,1,1,0.24)`；`806699—8066C8`light overlay blend为`0,4,7,8,7`。StatHUD记录按`83CFD0`／`83DC60`的当前值／差值／前值／阈值／倒计时复制，不调用其概率分支。所有点位只形成采用依据，尚无本轮运行结论。

仍需统一验收确认当前安装的官方Hook、真实context、像素、HUD布局／Minimap及所选Player组合；原生异常／context丢失不具备通用安全恢复保证，相关owner不确定时停显并保持隔离。全局render surfaces仍归真实Manager，未转成网络对象或任意线程释放；本方只借用并在有效context消费，CPU退尽不表示GPU完成。此实现阶段未由I4执行构建、游戏、校验器、单元测试或E2E。

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
