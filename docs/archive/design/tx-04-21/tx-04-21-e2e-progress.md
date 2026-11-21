# tx-04-21 双人 E2E 当前进度

> 2026-11-08 起按[原生二进制状态同步重构](../binary-state/README.md)推进：本文的暂停／Resume 方式不再执行，E01—E26 修订后并入[第 15 步统一整体验收](../binary-state/15-e2e-acceptance.md)；本文事实与失败记录保留，正文不改。

## 2026-11-08 第74—75轮构建及界面停止

第74轮仅构建失败：新查询块reader把Lua长度和unsigned参数个数放在同一个auto声明，MSVC报告C3538；已拆开声明。第75轮产品／E2E／harness构建和部署成功，包含下节GetTearsCap类型修正、直接查询块及类别计时。计划补测`E01.stream-schema`、`E01.compact-schema`、`E01.valid-menu`后继续E02；其余原E01结果仅按未触及的拒绝路径复用。

第75轮在`E01.stream-schema`启动Host PID69540之后、尚未操作开场时，computer-use收到物理Esc停止信号。已停止界面输入，并向现有编排入口提交stop，未启动本轮Client或新run。此轮没有新世界性能结果，不能称查询块优化已实测有效。必要恢复材料仍位于`out/tx-04-21-p2-20261106-01`，本轮控制台`out/tx-04-21-ui/memory-fix-matrix75-console.log`；默认临时材料在整个验收完成后清理。

编排器随后正常完成收尾并以未完成状态退出；两端部署restored=true。连接级设置仍保留，不能把节点部署还原等同整个网络环境已最终还原。

继续时使用同tx Resume并保留上述三个E01补测，再覆盖E02及未完成项。静态复核另发现待处理项：worker在冷构建完成后因水位变化撤销输出时，须重新请求被撤销的冷结果，避免内部baseline指针在后续ordinary中重新取得发布资格；该问题尚未修改或实机验证。当前E02仍未通过、Host帧耗时仍明显超标、E03—E26及P4未执行。

## 2026-11-08 第73轮：基线到达Client，类型校验失败；帧率仍不达标

第73轮构建部署成功，Host PID38240、VM Client PID7084，第二测试槽Isaac／Normal／Basement新局。165个内存样本的Host private峰值889,147,392字节（约848MiB）；Values／Wire／Workspace峰值225,840,246／4,709,812／3,448,497字节，未出现capture-failure。Client实际收到基线并进入admit，首因为`entity:objects/0/body/EntityPlayer.GetTearsCap:decimal_range_or_type`，随后Recovering，90秒内world_applied仍为0。E02继续失败；两端已正常收尾、restored=true。

本轮596个stage12主线程采集样本wall平均184,082微秒、最大460,268微秒；596个stage13帧间隔平均194,618微秒、最大1,155,832微秒。558个Lua帧聚合平均179,009微秒；259个后台构建平均230,895微秒。260次ordinary域读取中Entity平均113.50ms、RoomSave32.49ms、Grid24.31ms、Room18.77ms、Presentation18.70ms、Level11.65ms。Random仅有4次冷读取，平均47.40ms。性能合同仍未达到，不能将本轮越过OOM和收到基线当作E02或50FPS通过。

续修：官方1.1.3的`Lua_PlayerGetTearsCap`用lua_pushnumber返回double的GetCustomCacheValue，修正生成目录的采用类型并保留Wiki int原签名；不放宽通用整数校验。两类Player有限标量查询在正式路径直接写自有值块，保留公开绑定逐次调用、四种bool组合、图定义／引用和失败语义，消除中间Lua包装树。增加同次来源的Sprite／Player／配置／动画累计计时，父子计时不相加充当整帧成本。尚未取得该修改后的运行结果；目录类型变化需复验E01及E02和后续受影响用例。

## 2026-11-08 第72轮：内存下降、基线已发布，E02仍未通过

第72轮产品／E2E／harness构建部署成功。`p2-E02.cold-55`实际Host PID23964、VM Client PID11568，第二测试槽Isaac／Normal／Basement新局。276个内存样本的Host private峰值887,975,936字节（约847MiB），Values账本峰值197,282,893字节，Wire／Workspace为13,630／18,416字节；本轮没有`capture-failure`分配异常。11域均完成分层，实际冷基线已发布。与第71轮的2,195,279,872字节和200次分层异常分开记录，不能外推为所有场景内存达标。

Client仍在90秒内未取得world_applied，E02继续失败。此轮已越过OOM，暴露出正常Grid UPDATE被归入Lifecycle事件、反复撤新来源，网络把“暂缺新候选”当作“来源退休”取消冷传输；不能以Host本地发布计为Client应用通过。两个节点已正常收尾且restored=true，同tx重新构建后继续。

用户另指出Host仍低帧率。此轮495个stage12主线程采集样本wall平均224,856微秒、最大493,547微秒；495个stage13帧间隔平均235,968微秒、最大500,766微秒。458个Lua帧聚合平均218,328微秒，209个后台构建平均258,269微秒。它们属于失败run全程实际记录，远未达到50FPS／30和60Hz合同；Lua帧聚合不是“一个域一次调用”的耗时。

正在同tx修复：Grid独立UPDATE只按显式诊断需求采集且不撤销Live；真实生命周期仍撤新目标资格。发布信封增加本地采集session／epoch／run／room连续性键，同场景在途历史基线继续按原sample发送，明确退休／换场景仍取消路由；暂缺来源仍受原陈旧恢复期限约束。冷请求完成与dirty水位分开，避免每次dirty重做Random等全域冷读取。小对象容量原型、Lua getter调用函数／目录选择复用及对象一次raw复制减少临时分配，未缓存动态结果、删除字段或改变频率。节点另读取现有累计域计时以保留失败时的来源成本。上述后续修改尚未构建／复验。

## 2026-11-07 第70轮之后：后台构建内存修复与续验

用户本次要求定位并修复内存峰值问题，随后明确“改动完直接跑e2e测试，虚拟机已经准备好”。因此恢复同一个tx、原P2账本及统一入口；不另建tx、不增单元测试、不改变未通过项的结果。此前暂停说明保留为历史时点。

新增定位：除第70轮已修的来源小对象和装备整树复制，后台展开、分层、图闭合、身份投影与冷模型也重复放大容器；分层数组曾为随后被覆盖的每个槽分配占位对象；冷模型还同时保留十一域展开封套并深复制完整components。修复为SAX一次展开、已知容量的小对象、仅缺席槽分配占位、原地闭图、逐域投影后释放中间arena、逐域冷封套及最终components所有权移交。失败重试收集Store旧候选，并补实际arena失败请求大小。完整字段/参数、图语义、身份登记顺序、线程边界、三池及超时不变。实现与剩余临时内存见[采集说明](tx-04-21-capture.md#2026-11-07同tx的构建内存修复)。

第71轮产品／E2E／harness均构建部署成功。E02.cold-54实际Host PID45764、VM Client PID11624，第二测试存档、Isaac／Normal／Basement新run；Client仍在90秒内未取得world_applied。Host新增诊断记录200次`stream_partition_allocation:split:domain=0:object=0:arena=4096:bad allocation`，说明失败发生在首对象且分层arena尚未增长，不能将这次分配直接归于分层大树。175个memory样本的private峰值2,195,279,872字节，Values账本峰值178,669,192字节，Wire／Workspace为13,630／18,416字节；不得因账本峰值下降就宣布故障已解决。该轮已按原入口正常收尾，两个节点均restored=true；重新构建需要结束本次编排后用同tx Resume继续，并非撤销用户的连续续测指示。

据此定位到另一处更大的静态放大：`gen_stream_catalog.py`把9,884,272字节正文生成1168段临时`std::string`的连续`+`表达式。本机MSVC14.44的`xstring`双右值拼接在容量不足时新建正文而保留左右输入；完整表达式结束前前缀临时值仍活着。全部前缀正文的静态长度和为5,788,167,066字节，这是代码路径容量推导，不是成功分配或实测private。首次FieldLayer查目录会触发这段初始化；这解释了小分层arena与进程大峰值并存、且未经过自有arena失败诊断的现象。

本次把五个目录生成器统一改为一次reserve、逐段append；追加语句不产生前缀正文副本，UTF-8字节和原目录内容保持。C++分层改查与Lua同规则生成的静态有序表，不再为查scope惰性解析完整研究目录。第72轮正在同一入口重新构建部署并续测。原E02.cold-51与54失败均保留；本次未运行额外smoke、单元测试或离线试跑。必要续测控制台分别为`out/tx-04-21-ui/memory-fix-console.log`与`memory-fix-catalog-console.log`。

## 2026-11-07 第69—70轮续验及界面停止

本次用户确认VM准备好后继续原P2账本。接手时第69轮仍停在Client启动准备，Host PID60768已正常退出0；以实际缺少Host收尾E02第50次，再在同一入口重试，没有启动重复编排器。

- `p2-E02.cold-51`实际运行Host PID3484、VM Client PID4500。Host第二测试存档槽、Isaac／Normal新run，Client保持菜单。90秒未取得`client_world_applied=1`，E02仍失败；E03—E26及P4未执行。
- 真实后台首因为`capture-failure:stream_capture_exception:partition:0:bad allocation`，记录中共202次。Host三池拒绝序号均0，不能把本轮故障称为三池预算耗尽。已读取的内存记录中private峰值2,133,397,504字节。后续`live_source_changed:1:2:isaac.cs.grid.lua-event`会遮住具体构建失败，但尚未证明它是本轮无基线的唯一原因。
- 本轮已取得483个stage12逐帧主线程采集记录，wall平均254,627微秒、最大599,428微秒；483个stage13整帧间隔平均265,901微秒、最大606,361微秒。均是失败run的实际样本，不是已完成的稳定性能窗口，明确未达到实时目标。
- 第70轮修改：装备拆分只复制待修改装备结构，避免先复制完整Player查询树；`CopyLuaSource`用Document SAX构建栈准确分配容器，避免每个小对象默认预留16个成员。分层分配失败额外保留stage、domain、object及scratch／final arena容量。字段、查询矩阵、块／图、协议与三池额度未放宽；尚未取得修改后的新run结果。
- 产品／E2E／harness均已构建打包部署，最新包为`attempt-70/product`与`attempt-70/e2e`。E02第52次启动Host后收到computer-use物理Esc停止信号；用户再次要求继续后，第53次仍收到该停止信号。两次均在开场准备阶段正常收尾，无新run；不计为新世界功能通过或失败。此后未继续界面输入。
- 第51次两端退出0、无不允许的存档差异；第52—53次Host退出0，Client启动次数0。最终两端节点`restored=true`，第70轮编排已退出；连接级设置仍未最终还原。

最新继续位置仍为E02，使用原`out/tx-04-21-p2-20261106-01`加`-Resume`。本次必要恢复材料保留于该目录，控制台为`out/tx-04-21-ui/attempt70-console.log`；第51次失败与第52—53次中断记录原样保留。下文此前暂停、构建及性能未知的记载保留各自原时点事实。

## 2026-11-07 本次恢复及再次停止

**当前状态：用户恢复测试后，又明确要求“先不跑02测试”；现已停止，不继续E02或后续用例。** 同一入口、同一tx和原P2账本继续使用；本次没有新run或世界采集性能结果，不宣称50 FPS、30／60 Hz或E02已通过。

- 第64轮在产品构建时发现新C++分层枚举`World`与同命名空间`World(scope)`函数歧义，未部署或启动游戏。已将枚举项改为`WorldLayer`，分层含义不变。
- 第65轮产品／E2E／harness均构建、打包并部署完成，包含第63轮之后两项分层修复和第三项Host完整采集性能修复。构建通过只证明编译／链接，不能替代完整采集的实机验收。
- `p2-E01.valid-menu-8`实际PASS：Host PID36844、VM Client PID13740；两端菜单连接，Host的participant1为Established，Client为AwaitingWorld且world_applied=0。两次事实中的Client participant=1、connection和wire_connection一致；两端Identity错误为0。实际冻结60／30／20／60 Hz、每Peer312500／总937500 byte/s、不借空Peer份额，三池实际512／128／128 MiB。两端正常退出0，cleanup.pass=true，save_failures为空。其余八项E01结果按未触及协议／拒绝路径显式复用，不增加已通过子项数。
- `p2-E02.cold-47`仅启动Host PID15436并到开场处理提示，用户即要求不跑02；尚未开始新run，E02的Client未再次启动（节点finish中的launch=1来自前一E01）。在该提示提交stop并经统一入口正常收尾。账本原样记录“用户请求必要中断”且pass=false，不能记为新世界功能失败或通过。computer-use收到物理Esc停止信号，此后没有再发界面输入。
- 本轮两端节点均已restored=true；Host正常退出0，Client保持前一E01已正常退出0，节点helper已收尾。本机未发现isaac-ng／harness进程。连接级设置仍按下文账本保留，没有执行WinRM／防火墙的最终还原，不能宣称整个网络环境已还原。

必要恢复材料仍在`out/tx-04-21-p2-20261106-01`：最新构建为`attempt-65/product`和`attempt-65/e2e`，日志为`out/tx-04-21-ui/attempt65-console.log`，最新菜单通过为`p2-E01.valid-menu-8`，最新停止位置为`p2-E02.cold-47`。恢复后仍从E02开始，随后E03—E26；P4未执行。E01九项通过、E02历史失败、E03—E26未执行的总进度不变。

## 第63轮后实施阶段记录（保留原时点事实）

以下“未构建／测试继续暂停”等描述属于本次恢复之前的实施阶段，最新构建、菜单复验及停止状态以上节为准；历史失败和性能未知不变。

更新时间：2026-11-07，Asia/Singapore；在原14:44暂停记录后补充分层修复及Host采集性能方案的实施结果。

**状态：测试继续暂停。第63轮采集优化已构建但未实机复验；随后第一、二项分层修复，以及本次按用户指示实施的[第三项Host采集性能修复](tx-04-21-capture-performance-plan.md#10-2026-11-07实施结果与尚未验证的成本)，均已修改代码、尚未构建／复验。E02仍未通过，不能宣称50帧、实时频率或完整联机验收已经达标。**

原暂停指示是“修复采集超时问题后先不继续进行测试，将当前进度写到文档内”。之后用户要求“先修复前三个问题。第三个问题是重点问题，需要你先给修复方案然后我再审阅”，前轮据此实现前两项并提交方案。本轮用户明确指示“按照tx-04-21：Host 采集性能修复方案的要求，开始修复工作”，据此实施第三项；该指示未恢复测试。后续收到恢复测试的指示再构建、部署与启动游戏，历史失败不因代码修改改判。

## 已实际完成的验收

环境是同一台电脑上的宿主机 Host＋Hyper-V 虚拟机 Client，目标为官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273。当前只执行P2；P4未执行，也不在这次双人实机请求内自动启动。

P2已通过 **9个子项，均属于E01**；按E01—E26的大项计算，只有E01完整通过。E02未通过，E03—E26未执行。以下为暂停时进度账本实际保存的最新通过记录：

| E01子项 | 最近通过的子项尝试号 | 结果 |
| --- | --- | --- |
| wire | 13 | PASS |
| stream-schema | 4 | PASS |
| compact-schema | 4 | PASS |
| resource-declaration | 4 | PASS |
| session-identity | 4 | PASS |
| framing | 4 | PASS |
| capability-missing | 4 | PASS |
| full-room-capacity | 8 | PASS |
| valid-menu | 7 | PASS；第60轮重新实测翻倍后的三池 |

原八项协议／身份／容量结果来自第44轮；第60轮valid-menu用于重新确认两端实际冻结配置。重复运行同一子项不增加已完成项数。

## 内存修复与新额度

- 按用户决定，Values **256→512 MiB**、Wire **64→128 MiB**、Workspace **64→128 MiB**。默认值、可配置硬上限、实际账本、值展开／Store／Live限制与配置样例已同步。
- 保留构建阶段原有保守预付上界；安装后按仍持有的DTO容器capacity、字符串及共享纯值分配计量。实际保留分配从已付租约转交，临时副本退尽后才归还余量；异常路径的候选／部分原生所有者不能提前退费。
- 旧场景实际退休且纯值／原生所有者被替换后，释放已不存在的旧计划／DTO租约。共享纯值用有界同步索引计一次，计量工作区先付费。
- 第60轮valid-menu第7次已确认两端实际512／128／128 MiB，正常退出0，cleanup通过、save_failures为空。第60轮E02第43次Client Values峰值 **292,111,239字节**，三池拒绝序号均0；第62轮E02第45次Client峰值 **265,625,087字节**，拒绝仍为0。

以上证明新额度实际生效，并在这些样本中越过原256 MiB限制；不证明全部场景不再OOM，也不把额度翻倍当作同步超时已解决。单条消息、MTU、节点上限、60／30／20／60 Hz配置、带宽、重传及超时参数未随内存额度放宽。

## 最近实机失败与采集超时定位

第62轮E02第45次：Host PID13852、VM Client PID7660，Host第二测试存档槽，Isaac／Normal、种子H6GR NTNY。实际提交包括冷19与普通37、61、71、105，共5次；仍未形成E02通过。

Client首因为 `client_state_stale`，首个等待原因为 `no_pending`，后续 `operation_no_progress_or_deadline` 后安全退休至Off。两端退出0、cleanup通过、save_failures为空。该轮三池没有预算拒绝，不能继续归因为内存不足。

新增的真实分域计时取得：

| Host同轮真实调用 | 次数 | 总耗时 | 单次平均 |
| --- | --- | --- | --- |
| Entity Lua采集 | 19 | 19.682秒 | 约1.036秒 |
| RoomSave Lua采集 | 19 | 7.743秒 | 约0.408秒 |
| 全部Lua域调用合计 | 192 | 35.971秒 | 各域调用合计，不能当成192个完整世界窗 |

Host同机普通源前18个间隔大多为2.17—2.76秒，另有一次3.63秒。完整源产生本身会超过2秒恢复阈值；Client没有新待处理目标也可能陈旧。上述时间只在各自设备的单调时钟内比较，没有跨设备时钟相减。

## 当前代码修复及验证边界

### 第63轮之后：第一、二项分层修复（未构建／未测试）

1. **修复域编号错位。** `CaptureLuaEntity` 传入模块的零基下标；Lua 曾按一基编号判断原生应用域。现在具名映射为 Entity=0、Grid=1、Room=2、Player=6，其余域保留原 Context 语义；同时修正 Grid 默认范围并拒绝非法下标。Entity／Player 的 Structure／Resource／动态字段恢复按既有目录分层，避免主 Sprite 资源落进 Context 后无法满足 PlayerPost 依赖。
2. **分离稳定内容与逐窗采样关联。** 保留 `location` 的真实身份，把 `scope`／`entity_scope`、Player 的来源和 canonical scope、continuity、RoomSave 集合的 `source_scope`／`temporal_relation`、Presentation 的 canonical scope 放入同目标 WorldDynamic。RoomSave 的房间／集合状态不是失败叶子，允许拆出内部采样关联；实际保存数据仍留在 Context。没有删除 Player 安装器要求的同窗字段，也没有把旧字段改成新 scope。

静态复核沿来源生成→分层→Context／Structure／Resource／WorldDynamic合并→Player目标校验检查了实际字段路径，复用现有等长数组占位与合并规则。未知值、真实空集合和原始冷来源仍保留。原有工作区修复未回退；本轮没有执行构建、校验脚本、测试或启动游戏。

**边界：**真实 Run／Presentation 动态字段和 Player 普通联合值仍可能推动范围更新；此修复只消除已确认的错误路由与采样编号造成的版本变化，不宣称所有 Context 已稳定，也不宣称采集成本已达标。动态范围字节、同窗关联、PlayerPost及稳定版本行为还须在恢复后的统一 E2E 中覆盖。

### 第三项：完整采集链路性能修复（本轮已改代码，未构建／未测试）

1. 主线程仍在合法ordinary／PlayerPost窗口取得必要值，正式路径用自有类型化值和64项块替代Lua JSON；大查询／关联数组不再全部滞留Lua结果树，显式诊断从同份值编码。
2. Player参数集合仅在同窗共享，批量调用仍走原公开绑定；RoomSave Grid向量批量复制公开列，有类型比较替代JSON判等。资源／保存／配置writer未形成完整闭包，因此未启用跨窗稳定缓存；目录逐字段标明FreshRead和剩余成本。
3. 单一后台owner执行分层、闭图、身份投影、范围冻结／Store维护；普通最新槽、独立冷槽、八身体槽和256项有序控制环保持有界。构建结果核对generation／水位；PlayerPost只接受交接时冻结的ordinary依赖，冷请求不能由更早的在途来源满足。
4. 来源、批次和发布元数据纳入既有三池；封闭树转移所有权及租约，最后大对象释放交给有界回收owner。主线程不等待worker或join；未提高池限、报文／带宽或超时。
5. stage11单列后台构建，stage12统计每帧全部采集窗口的排他wall／CPU，stage13记录整帧入口间隔，14—18给出来源分项。归约器检查主线程2／4ms及20ms整帧目标，补充逐秒FPS和长停顿统计。

实现、静态依据、失败处理和明确未缓存字段见[修复方案第10节](tx-04-21-capture-performance-plan.md#10-2026-11-07实施结果与尚未验证的成本)；后续覆盖见[E2E续验补充](tx-04-21-e2e-plan.md#host采集性能修复的续验补充2026-11-07)。本轮只进行源码阅读与编辑，未运行构建、测试或游戏；当前部署仍不能代表这批修改。

本次继续使用既有失败材料即可；实施后要证明50帧仍需统一 E2E 的真实采集。Client 侧“无结构事务的依赖等待达到2秒”及同步原生准备长窗为独立剩余风险，本轮未修复，不能因 Host 优化而宣布超时已彻底消除。

### 第61—63轮历史修复

第61轮已经编译、第62轮第45次实际运行的修复：

- 每Peer固定一个正在准备的普通目标，直至真实Applied ACK；最新候选独立合并，避免报文发完但依赖未齐时反复切换目标。
- 结构依赖取得绑定已经完整收到的精确World版本。只有新增依赖接纳或原生步骤变化才推进；仍使用2秒无进度／120秒绝对期限，陈旧提示仍按最后真实提交计算。
- E2E源快照选择发送器接管的不可变闭包，通过单个有界诊断worker组合；不再用未发送的Capture候选要求Client对齐，也不在网络线程同步组合大快照。该轮尚未到精确对照步骤，不能将此修复记为已实机通过。

第63轮新增并编译通过、**尚未实机复验**的采集修复：

- JSON编码在同次调用内复用短字符串完整转义结果，缓存上限64 KiB，结束即释放；没有复用旧采样值。
- 图闭包复制中的纯连续数组按实际槽序复制，避免为大量保存项逐次创建字符串比较排序；混合表仍做确定性排序，字段、数组槽和图关系完整保留。
- `Sprite.GetCurrentAnimationData`／`GetOverlayAnimationData` 的完整描述跟随当前动画选择所属动态分区，避免内部 `AnimationData.GetLength` 单独被提升为Resource换代。真正ANM路径／资源代次仍进入Resource；没有删除描述字段或停止重读。
- E2E新增每域collect／partition／encode三阶段计时，仅记录真实原调用包围窗口，不重复采集，不混入同步值。

主要位置：[采集与JSON编码](../../../../mod/observation/isaac-cs-observation/domains/common/read.lua)、[分层与图闭包](../../../../mod/observation/isaac-cs-observation/domains/common/stream.lua)、[只读计时桥](../../../../mod/src/game/world_sources/lua_entity.cpp)、[实际DTO计费](../../../../mod/src/game/client_dto_memory.h)、[Client范围应用](../../../../mod/src/game/client_stream.cpp)、[Host发送owner](../../../../mod/src/net/stream_session.cpp)。

仍未证明：新采集耗时低于2秒、Host 50帧底线、60／30 Hz实时目标、稳定同房更新、当前完整E02、精确源／Client原生／Render一致、全部热DTO与异常保留路径。仅低于2秒也不满足实时目标；后续统一验收须结合三阶段计时、完整主线程成本和整帧表现，不能通过扩大超时、减少必需字段或把旧值当新采样来改判。

## 暂停位置与必要恢复材料

- 唯一入口：`scripts/run-tx-04-21.ps1`，继续使用同一个tx和 `-Resume`。
- 同tx目录：`F:\projects_isaac\issac_multiplay\out\tx-04-21-p2-20261106-01`；`progress.json`是实际通过／失败账本，不手工改判。
- 最后实际游戏样本：`p2-E02.cold-45`，保留必要facts、两端事件与失败／退出事实，供后续分析。
- 最新构建：`attempt-63/product`、`attempt-63/e2e`；控制台为 `out/tx-04-21-ui/attempt63-console.log`。它们不包含随后第一、二项分层修复，也不包含本轮第三项Host采集性能代码。
- 最新停止记录：`p2-E02.cold-46`。它是用户要求暂停后在Host启动准备处停止，**两端launch均0**，不是新增游戏失败，不计完成项。
- 恢复时从 `p2/E02.cold` 重跑，再继续E03—E26；八项E01拒绝结果和第60轮valid-menu可按未触及路径显式复用。后续若修改协议、协商或默认额度，须覆盖其影响，不能机械复用。

原14:44暂停收尾时已核对：宿主与VM没有测试游戏或harness进程，VM没有本tx测试计划任务；当时节点部署及游戏选项已还原。临时快照用后清理，必要失败事实和恢复材料因暂停保留。本次代码实施未重新检查节点、部署或启动游戏，以上是最后确认的环境状态。

连接级设置尚保留供续测：宿主WinRM、只含VM地址的TrustedHosts、具名窄防火墙规则及本次DPAPI凭据；`out/tx-04-21-connection/state.json` 的 `restored=false`，**不能宣称整个网络环境已还原**。原管理员还原helper已超过12小时退出；当前会话非管理员，本机WinRM身份检查拒绝访问。没有为本次文档暂停再弹管理员窗口。若后续结束整个测试环境，使用 `scripts/prepare-tx-04-21-winrm.ps1 -Action Restore` 按同一连接账本执行管理员还原，不删除未恢复账本或先删凭据。

更早的失败、修复假设及纠正见[原编排记录](tx-04-21-isolation-orchestration.md)，不以本页覆盖历史事实。
