# tx-04-21 I36—I38：全窗口隔离、接线审阅与统一编排

日期：2026-11-05，Asia/Singapore。I36—I38的源码、部署恢复、回归目录和唯一入口已接入，并由主智能体静态复读。**本轮没有构建、运行测试、部署到游戏目录或启动游戏。I39、I40仍未执行，tx-04-21整体验收仍未完成。** 本文是功能合同，不是测试artifact或运行通过报告。

入口：[实施清单](tx-04-21-implementation-plan.md)、[E2E组件](tx-04-21-e2e-components.md)、[验收矩阵](tx-04-21-e2e-plan.md)、[现行规范](../../refactor/project-conventions.md)。保持同一tx，不设gate、不校验哈希、不补写unit test。

## I36：从网络准备到最后退出writer

新链路继续使用唯一`ClientScene`及既有17—20用途/借用合同。没有新增游戏原生函数、Hook点位或原生对象布局。入口对应如下：

| 窗口 | 实际入口和owner | 继续约束 |
| --- | --- | --- |
| 菜单连接/准备 | `StartClient`、`BeginStream`、`AdmitStream`，Manager主线程 | 准备前Lua/回调/持久/Shutdown保护齐备；网络只传自有值，不进入本地run/Continue |
| 冷构建 | `Pump`→NativeBuild/NativeApply、18—20原有owner | Manager、用途帧、前缀登记、Graphics/Native票据；不整体放行GameStart/RoomInit/Update |
| 普通动态/身体快源 | `PumpStream`→CompileStream/CompileFast→InstallStream/InstallFast | 同Scene、实例/资源代次、完整成员和精确依赖；不执行AI/碰撞/伤害/拾取/库存玩法补算 |
| 结构/资源替换 | `StructuralModel`、原有Player/Entity/Grid资源owner及关系检查 | 先撤旧展示/借用，再按真实前缀替换；未知构造/析构不猜测放行 |
| 原生维护/绘制 | PresentationMaintenance、PresentationDraw、Player Draw用途 | Manager/Render分别限定；真实context、ANM/Image在途使用及已有随机/SFX/Lua分派不扩大 |
| 恢复 | `Recover`→旧Scene退休→新Scope/route完整Baseline | 关闭输入/旧候选；安全退休后才恢复，首因和失败owner仍由连接级账本保留 |
| 停止/关闭 | `StopClient`、`ClientProcessExitDecision`、Shutdown保护 | worker结束不等于native已退休；Off只在借用/owner退尽后交还，退出保护延续到最后持久writer |

`client_isolation.*`提供固定纯值累计，不依赖Recorder。`client.isolation.*`分开原生调用/正常返回、预期跳过/拒绝、Lua玩法跳过/自有菜单UI、native/Lua持久拒绝、随机拒绝、故障、恢复、停止和退休。真实网络Client现在与旧fixture一样，在连接开始冻结九个原玩法原调用计数，不能只检查最终`run_active=0`。

E2E显式启用后，这些事件带本地连接、wire连接、route、Scene/实例/资源/Lua/context代次、Scene阶段、主线程与owner纯值。外线程只记未知身份与故障，不读取Client控制块或游戏对象。ring丢失/诊断分配失败仍是缺测，不影响正式提交/ACK。

持久入口继续覆盖原生save/achievement/unlock/计数、Shutdown直接写入、REPENTOGON附加保存，以及Lua的`Isaac.SaveModData/RemoveModData`、PGD写方法和ESSM路径。ModReference的SaveData/RemoveData经既有早期别名保护；玩法Lua不因新增传输或高频安装而开放。调用见证与Documents/Steam remote/官方ModData逐字节末态比较分别记录。任意未声明Mod的外部I/O不被本文冒称已隔离；原有声明范围保持不变。

E2E的物理来源、canonical接纳/过期、真实普通/odd consumer正常返回、实际Triggered边沿及状态年龄另有纯值记录。一次consumer内重复读取Triggered只记一次查询见证；它与实际玩法结果的语义审阅分开。产品不编译这些逐项明细。`stream_test ... observation-loss`实际丢弃一条诊断并通知缺测；既有既有真实secondary VM/释放借用/错误context演练均要求token及Manager边界；实际同步不依赖通知成功。

## I37：逐成员与逐消息接线审阅

逐成员语义仍以`stream-catalog.json`和`CaptureLayouts`为唯一目录，没有删掉未支持成员来形成“完整”目标。静态主审按以下链路复读，而不是只检查接口声明：

| 链路 | 复读位置 | 结论/边界 |
| --- | --- | --- |
| 来源→范围/版本 | `world_bridge.cpp`、`stream_capture.*`、`stream_model.*` | 统一Registry；ordinary与每身体时间线分开；29/38私有联合来源限制保留，不虚报所有身体60 Hz |
| 范围→线上 | `stream_binary.cpp`、`stream_codec.cpp`、`stream_transport.cpp` | 固定完整布局、显式依赖、wire/framing/schema版本、解压/节点/候选预算；不引入22的字段继承Delta |
| 每Peer发送/取消/确认 | `net/stream_session.*`、`net/live.*`、`net/worker.*` | route与连接复核、可靠补片/进度超时、已发/已收/已应用分开、总桶/单Peer及lane；共享值不跨线程借用native |
| 主线程目标/实际消费 | `client_stream.cpp`、`client_scene.cpp`及18—20 adapter | 冷闭包、结构重建、普通安装、逐身体安装仍由一个Scene负责；实际native提交后形成精确范围ACK |
| 停止/失败owner | `client.cpp`、Scene/retained owner及持久保护 | 控制终态、输入封存、真实退休、恢复上限和首因分别保留；不以socket停止代替世界退出 |
| 产品/测试边界 | CMake、build/package/deploy、`ISAACCS_E2E` | 产品只增固定隔离聚合与必要检查；故障/inspect/负向/视觉暂留/逐项明细只随E2E编译 |

本轮静态发现和修正包括：网络Client缺原调用起点、旧Source夹具取得后态后暂停Host、P4扩展断言只有声明没有执行、E25仅五个概括项、WinRM服务桌面不能提供交互游戏窗口、产品配置不能含空E2E token、helper异常不能经Game兜底Drop强杀游戏、跨allocator临时JSON必须深复制、跨设备资格分母不能直接用另一台机器的绝对时钟、部署目录与恢复路径的边界和逐次记录，以及精确截图期间提交向量仍会变化。这些是静态发现，不是运行失败或通过事实。

`tx21/regression.rs::inventory`逐个读18/19/20现有`scenes`，保存原场景定义、所属tx、正负向和受影响来源/adapter/资源/退休责任。正向用依赖前缀依次布置，避免把`npc-change`等有前置的步骤孤立运行；每个最终条目单独比较真实网络后态/画面。Entity全部16个和Player全部24个具名故障前缀逐个展开。旧19独立事件文件导入器的四个拒绝场景未进入21的完整范围传输，不把未改动的旧导入入口重命名为21网络回归；其历史事实保留，完整最终成员/字段/引用仍由21范围回归覆盖。

菜单容量另用最多三个无世界握手探针：由正式codec生成当前能力的Hello，不发送Ready/Input/Range；填槽须实际收到Welcome，满槽拒绝须观察Host真实foreign计数，不能靠接收超时通过。这些探针不生成身体、不计入P2/P4的真实游戏Client数量。

负向分层明确：`stream_test ... wire <name>`在真实Hello/Welcome形成后仅改一次发送正文，接收端仍实际解码/协商；代理负责片/route/ACK等真实报文损伤；`stream_test ... reject <18|19|20> <name>`从真实已提交网络闭包派生恶意模型，在同一Scene检验原adapter的写前拒绝与A保持，不用本地文件，也不冒充wire错误。前置对象/FX/twin/backup不存在时明确报缺前置。具体拒绝层、首因和原生A必须对应具名场景，不能用任一错误兜底。

21到22的边界没有变化：本tx的完整范围、可靠分片、每Peer确认记录、Scope路由和同连接完整恢复可复用；多基线保留/轮换、对象/字段Delta及`Delta(B,T)`重建未被声明完成。23+的自然切房/整局/重连/Continue仍保留原责任。

## I38：唯一入口与真实节点

唯一入口为`scripts/run-tx-04-21.ps1 -Topology <实际拓扑.json>`。拓扑示例在`tools/harness/scenarios/tx-04-21-topology.example.json`，必须用实际IP、安装/存档/临时目录、已登录的交互用户、物理设备key和真实键位填写，不能把示例路径当本机事实。本轮没有访问远端设备或更改这些设置。

入口仅在I39调用时才构建产品/E2E、打包、备份部署并启动节点helper。远端helper使用当前已登录用户的Interactive任务启动隐藏helper，真实游戏留在该用户桌面；不是在WinRM服务桌面生成一个不可操作的“游戏进程”。任务仅用于本次启动，正常停止后注销。默认不更改全局网络/防火墙，也不发送邮件或其他通知。

`tx21/node.rs`的有界token RPC只提供真实子进程启动/结束、固定控制口、已核窗口物理键、截图和本次文件分页。没有任意shell、任意函数/地址或任意文件路径RPC。节点连续泵显式E2E trace和真实状态/内存，保存缺页/溢出；不从DTO填native事实。进程、Lua和原生对象不进入Rust网络worker。控制连接断开先释放本方按键，不能据此强杀或假称原生已退休。

`tx21/runner.rs::run`按完整`phase_suite`执行，P2所有E01—E26和展开子用例实际完成、清理及总审之后才启动P4游戏。P4保留同一功能子项，扩到四组输入和三Peer，并实际执行扩展断言。没有单项预跑入口或人数SKIP/N/A。产品E24另换产品DLL，禁用fixture/测试token/控制口，走真实UI和外部物理操作。

编排是**需要人工配合的完整入口**：每次启动独立告知节点/目的，只跳开场，到标题/选档页停手；确认ready后才启动/发键。Host选定测试存档并开始新run、VM窗口选择、自然互动和产品UI均有明确开始/结束信号。harness不自动抢焦点；SendInput只发到已核实的当前前台游戏。`physical_script`维护跨步骤held集合及异常松键。按拓扑显式选择临时`PauseOnFocusLost=0`时，保存两个选项文件原字节，正常还原。

具名观察器分开自动事实和人工语义：实际负载、精确版本/成员/字段、原调用隔离、已命中的sender故障、物理/consumer事实及现有归约器数值不能靠输入“通过”覆写；画面、复杂玩法后态、指定原始报文注入和具名退休阶段须审阅对应原始事实，填写具体`observed`说明。未完成注入、未命中边界、缺前置、缺对应版本、未审画面或指标失败仍为未完成。该入口不声称无人值守，也不以一个通用`ok`完成整项。

精确画面段可显式启用`stream_test ... visual on/off`：选中native快照提交后，最多120秒暂留该Client提交向量，Host/来源/网络继续，期间候选仍有界；真实Render完成后再截图/审阅。暂留期间仍推进确认和纯值Collect；Host显式进入同一有时限的视觉中性段，单列收敛计时。退出暂留恢复陈旧年龄起点，避免把人工审阅时间当实际网络故障。此段明确属于视觉中性，不计算30/60 Hz或CPU指标；产品没有该能力。

代理另外有最多20次真实完整动态transfer重送。缓存只保留每Peer一份在收集、一份已完整的至多64片动态消息；延迟/发送队列收紧到7MiB，为这些有界副本留空间，全部仍在原8MiB缓存范围内。重送保持原报文身份，按实际socket发送计费，排队次数不等于已送达或已应用次数。

## 同tx修复、还原与未执行项

### I38补充：从游戏内ImGui操作统一入口

2026-11-05按用户要求增加E2E专用面板，入口为REPENTOGON ImGui菜单的 **Isaac CS → tx-04-21 acceptance (E2E)**。这是上述同一套I39编排的前端，P2/P4次序、完整子用例、实际失败、恢复及人工说明要求都由原runner执行。**补充实现同样只有源码和静态审阅，本轮仍没有构建、部署或实机测试。**

第一次使用需要在进入I39后构建并安装包含本次修改的E2E DLL和UI；已经运行的旧DLL不会自动获得新接口。引导安装配置沿用完整schema=1配置，将`enabled`显式设为`true`，并保留E2E包附带的`"e2e_ui":{"enabled":true}`。产品包没有该配置、原生入口或`e2e.lua`，不会显示测试按钮。REPENTOGON Wiki `changes/Debugging.md`说明ImGui沿用原调试控制台打开键；以用户当前REPENTOGON按键设置为准。

操作顺序：

1. 在房主本机打开面板，填入**实际四机拓扑JSON路径**；先按现有示例填写真实节点、路径、IP、交互用户和键位。面板不将示例自动当作真实环境。
2. 新运行的目录可留空；同tx续跑则填写原运行目录，勾选`Resume this tx`，填写受影响用例列表和未受影响结果复用原因。列表以逗号分隔，条目仍是原runner的精确`p2/<id>`、`p4/<id>`或整个`p2`、`p4`。
3. 回到没有活动run或C/S会话的菜单，明确勾选I39准备项，点击开始。按提示**从游戏菜单正常退出当前引导游戏**；后台等待请求进程实际退出后才调用统一脚本构建/部署。不会强杀或替换仍在运行的DLL。
4. 房主E2E游戏启动后可重新打开相同菜单，查看阶段、用例、步骤和当前提示。每次启动/人工窗口仍分别点击`Ready`；完成指定操作并停手后点击`Done`。具名观察、总审和修复续测分别提交`observed`、`reviewed`、`retry`的具体说明，数值失败不能通过按钮覆写。
5. 游戏关闭期间、未打开面板时和产品版E24期间，使用独立确认窗口接续同一条提示。正向回答提交后自动收起ImGui；备用窗口如检测到本机游戏仍在运行，会暂存已明确点击的回答，等待用户手动将游戏恢复前台再发送。没有自动抢焦点或按超时推定准备完成；具体目标节点的窗口/物理输入检查仍由原helper执行。

`scripts/start-tx-04-21-ui.ps1`提供持续存在的备用窗口和隐藏编排子进程；隐藏子进程仍调用`run-tx-04-21.ps1`，不是第二套测试引擎。当前GUI只支持**协调器就是本机Host**；远端Client保留各自真实窗口和人工配合，房主面板显示具名节点提示。Windows需安装标准位置的64位PowerShell 7；原终端入口保留兼容。

本机`out/tx-04-21-ui`保存界面profile和信箱。请求经`cs e2e_ui`的有界分块入口进入专用worker；游戏主线程只交换自有字符串，worker只处理文件/进程句柄，不跨线程持有游戏/Lua对象。脚本地址固定在编译时工程的`scripts`目录，不提供任意shell。启动重复检查、唯一窗口互斥、显式会话及逐提示ID防止旧回答或双窗口点击串入下一条提示。两种界面按first-writer规则提交，runner独立验证回答内容；截断、半包、缺观察或错误回答都不形成通过事实。

自有UI仍只有原来的一个`MC_MAIN_MENU_RENDER`回调，没有扩展Client玩法Lua白名单，也没有新增玩法回调。面板构建发生在UI初始化或该菜单回调，避免在ImGui遍历窗口集合时新增窗口。产品切换除不加载界面外，也移除先前E2E部署的`e2e.lua`；最终部署备份继续负责还原用户原文件。正常完成清理GUI本次输出/提示，仅保留界面配置和完成末态；必要中断保留运行位置、输出和恢复材料。ImGui桥、备用窗口、焦点交接和所有既有合同均归I39统一实机覆盖。

`progress.json`记录每个P2/P4子项、尝试目录、首因与修复说明。失败后继续同tx定位/修复/重试；只有缺配合、需要重构建部署或实际无法自行收口时中断。`-Resume -ReuseReason <未受影响原因> -Rerun p2/E...,...`显式复用未受影响成功，公共修改要列出P2影响，再继续P4。没有哈希、旧解锁字段或独立作者要求。

部署逐文件备份并在覆盖前写记录；额外选项/Mod启停标记另存原字节。恢复只复制原文件/删除本次新增单文件，创建的目录仅在仍为空时逐个删除。关闭失败不强杀，保留实际owner/诊断。正常全部完成后，脚本还原全部当前及历史尝试节点，注销helper任务，核对绝对临时目录和本次owner记录后清理。中断仅保留继续所需的进度、失败、节点/恢复路径和必要采样；不新增每tx归档artifact。

资料顺序：先查本地原版Wiki `ModReference.md`、`enums/ButtonAction.md`，以及REPENTOGON Wiki `PersistentGameData.md`/回调文档，再查本地官方持久/附加保存/OptionsSyncing源码和项目17—21已核owner。本次只复用已核原生调用/借用及纯值协议，没有新增未知ABI/点位需求，**无需新增逆向，未启动逆向子智能体**。若I39出现必须逆向的具体缺口，仍按本次会话要求启动一个GPT-6 Luna max并由主智能体审阅。

目前只有静态实现/复读事实。两种配置能否构建、远端Interactive任务和运行库场景、P2/P4全部原始注入/负载/画面/隔离/恢复、采样吞吐、频率/预算/延迟/CPU/内存均待I39实际执行。诊断ring/文件预算耗尽会明确缺测，不能截尾或缩小负载判通过；需要分段时在执行前按场景边界安排并保留完整分母。I39失败必须在同tx修复续验，不能用本文或历史18—20结果替代。
