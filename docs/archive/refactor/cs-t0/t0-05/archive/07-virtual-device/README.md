> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 决策

## tx-01-43 自有 ABI 正常功能路径静态确认 v18（2026-10-16）

本轮完成 W01–W04 单一目标，结果见[静态确认报告 v18](../../07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v18.md)和[IDA/source 静态证据 v18](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v18.md)。在执行设计声明的正常前提内，原生静态主干基本足够；未发现需要重开基础 ABI 或扩成完整 Init、global manager-this、全 alias/observer、全部 Mod、worker/EH、完整 05-06 网络证明的范围缺口。

本轮按实际调用区分 Join 的 19 配置、NOT_CALLED、CALLED_FALSE 和 CALLED_TRUE：已有同 K 有效 selector 时，associated 使 9BB03C 越过新 admission 的 9BB064；B 尚无 Main Player 时，Player 局部为空又使 removal 的 9BB13E 不调用。普通新 admission 可先后有 9BAF78 possessor query 和 9BB064 query；JoinControl 的 8/10 为中性。旧 v17 E02 row 2/3 与 v17 工作设计的“已有有效 row且19=false”种子已在新证据更正，旧文件原样保留。

S2 规则将所有输出所需的 CurrentEligible 与仅 level-derived Triggered 所需的 PreviousComparable 分开。合法新 tag/current/target 的 8–11 explicit edge 不依赖 previousValid；invalid/stale current 中性。当前源码仍为 A-only V1；V2 held、双 participant reader与其他候选未生产实现。四个 UI 地址已改在新报告/证据中逐项区分：95B867 context,-1；95B87F selected Player,-1,1；95BBB6 selector 字段清理；95BBCB context,detected K。

candidate 内部一致但仍 DESIGN_ONLY。Authority-pre/current、StandardRoom normal-post/previous 与 contract v2 §4 +0x78 sole-writer 冲突，未采纳且未改 contract。vector责任-1与外部ΔR分账、NEVER、participant独立queue/dedupe/Resolve、六 consumer/action 许可表沿继承证据。K、selector/context/pair/listener现场值未观察；Join create/publish仍为用户接受的规划假设，后续运行验收另行处理。

IDA MCP 本轮成功读取32-bit isaac-ng.exe / IDB，base 0x400000、image size 0x8FE000，Hex-Rays与自动分析就绪。未运行测试/构建/脚本或模型，未操作游戏、进程、debugger、注入或Hook。tx-01-43唯一整体人工静态文档审阅通过（文档准确性、限定静态完整度与候选/采纳边界一致性）；覆盖报告/证据v18、README本轮修订及对应source/contract，未发现需修正项，无自动修补或复审。

## tx-01-42 下一步自有 ABI 静态确认设计 v18（2026-10-16）

当前执行入口为[工作设计v18](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v18.md)。设计交付tx-01-42，用户指定GPT-6-Luna、max子智能体执行tx-01-43，交付报告/证据v18。目标限定为已有selector的实际query轨迹、S2 level-derived/显式edge条件分离、95B867/95BBCB调用定位勘误及统一结论。原生静态主干基本足够，基础ABI、vector/R账本、writer边界及原生query/rebind条件继承，不展开完整新一轮逆向。

候选仍DESIGN_ONLY，writer与contract v2 §4的差异保留；未采纳、production未实现和现场未观察另列。执行只读IDA/source并修订新交付，不改旧文件、source、contract/schema或IDB，不运行测试/构建/游戏；全部编辑结束后仅一次整体人工文档审阅。tx-01-42设计交付唯一整体人工文档审阅**通过（设计交付范围）**，全部编辑后一次通读设计和本README修订并对照规范、v17复核、contract/source；审阅后仅回填结果，未复审。本条不是v18执行结果。

## tx-01-41 v17只读复核补记（2026-10-16）

会话只读复核确认限定正常路径的原生静态依据基本足够，但新交付仍有有限修正：B尚未创建且已有同K有效selector时，9BAE34–9BAE74置associated，9BB03C跳过9BB064；因没有对应Main Player，9BB10E–9BB110也跳过该Player的19查询。正确轨迹是本K本pass的19 NOT_CALLED，之后仍可查20/21/14；command mask的19=0不等于实际query返回false。v17设计的同一轨迹前提也需勘误，旧设计不改写。

v17“invalid previous一律不能产生Triggered”的摘要需限定到依赖previous/current比较的level-derived分支；新tag/current/target均有效且经Resolve的8–11显式edge不应受previousValid=false抑制，invalid/stale current仍全部中性。另两处要求的地址勘误未写入：context reset call为95B867，selector-clear之后的90AF30 call为95BBCB；95B87F是Player setter，95BBB6是字段清理。三类问题沿新设计收口，未发现需重开基础ABI/全alias的新缺口。tx-01-41未改v17或重跑tx-01-40审阅；下方历史原文/通过记录保留，以本条及后续新交付为当前说明。

## tx-01-40 自有 ABI 正常功能路径静态确认 v17（2026-10-16）

当前结果见[静态确认报告 v17](design-revision/virtual-device-owned-abi-static-confirmation-v17.md)与[IDA/source 静态证据 v17](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md)。在工作设计声明的有效对象、主线程顺序操作、Offline/local-co-op、Online roster为空、B唯一正K、创建B前Main Player vector只有A等前提内，本轮 Join 跨pass、引用责任、S2返回域及writer时序的原生静态依据足够；这不表示candidate已采纳、production已实现或运行已观察。

候选继续要求 A/B 共用 canonical、sequence、target tick、queue、dedupe、Resolve/apply，并隔离原版/Isaac Sync；不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。自有virtual-device每session创建owner与B Player的`9BB415` actual-create attempt owner分开，后者不以`9BB424` bind代替成功后置。

勘误：action19=false只拒绝该key本pass的新admission；已有有效selector row仍可进入后续扫描。进入confirm后先调用action14，再检查stored byte；stored=1只允许14=false继续创建，不跳过query。B的Manager vector responsibility恰消费一次，但total strong为`ΔStrong_B,total=-1+ΔR_B`，有条件的`913760→902F10` source-pair清理可改变外部R。StandardRoom original normal return后的Authority-post提交previous；完整`g_update()`返回后的TopUpdateExit只做生命周期/pause-gap边界。case2静态意味着`n298[2]==2`，但这不定义其Offline/local-co-op语义。旧v16文件与审阅事实保留，以v17勘误为当前说明。

候选内部一致但未采纳：Authority-pre/current、Authority-post/previous与contract v2 §4 `+0x78` poll sole writer冲突。实际K、selector flags、listener是否运行、n298[2]现场值未观察；自有ABI、JoinControl、双participant S2、attempt owner与retire coordinator未生产实现。tx-01-40唯一整体人工静态文档审阅通过（文档准确性与声明边界一致性）；核对三类勘误、引用依据与事实边界，未发现需修正项，且未复审。

## tx-01-39 下一步自有 ABI 静态确认设计 v17（2026-10-16）

当前执行入口为[工作设计 v17](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v17.md)。设计交付tx-01-39；用户指定GPT-6-Luna、max子智能体执行tx-01-40，新增确认报告/证据v17。目标限定为Join跨pass/selector轨迹勘误、native remove的vector责任与外部引用变化分账、S2中性/透传运行域和Authority-post/TopUpdateExit时序收口，再统一P01–P06分类与索引。基础ABI、无B TLS JoinControl、普通B erase、原生query/rebind主干和去重/held候选继承使用，不重新展开完整逆向。

程序未实现、候选未采纳和运行值未观察分别列出，不自动算作原生静态缺口。候选保持DESIGN_ONLY，不改contract/schema/source或旧报告/证据，不运行测试/构建/游戏；tx-01-40全部分析和编辑完成后仅一次整体人工文档审阅。tx-01-39设计交付唯一整体人工文档审阅**通过（设计交付范围）**，全部编辑后一次通读本文和设计修订并对照规范、v16复核、contract/source；审阅后仅回填结果，未复审。本条不是v17执行结果。

## tx-01-38 v16 只读复核补记（2026-10-16）

会话复核确认v16原生静态主干基本补齐，但若干结论需要修正：进入confirm分支时9BB39A先查询14，stored-confirm只允许14=false仍创建，不跳过这次query；已有有效selector时，本pass action19=false仍可能继续确认/创建。vector对B净减一不等于总strong恒减一或R恒定：已知通知登记/dispatch可有条件经913760→902F10清持久source-pair引用。S2候选中的“neutral/原生passthrough”需按活动C/S gameplay与UI/恢复后域分开，Authority-post的previous提交须与TopUpdateExit退休点分开。

复核还给出有限澄清种子：954CD0 case2选路本身约束当时n298[2]==2，但不等于Offline/Online模式语义；864C30/864D80是有条件的Lua callback提前返回，全部Mod兼容不属于本轮；906D60/A1D610/A212C0可静态连接913760登记与间接dispatch，是否登记且仍在list及匹配source-key保留为条件。该次只读复核未编辑v16或重跑tx-01-36审阅；原通过记录及以下历史原文保留。下方v16摘要中“stored跳过query”“外部R不变”及“最终审阅未完成”等说法由本条和v17新交付更正，不作为当前结论。

## tx-01-36 v16 静态执行续接（2026-10-16，已完成）

替代执行者已按[续接设计 v16](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v16.md)与[完整工作设计 v15](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)继续原 tx-01-36，仍交付报告/证据 v16。旧实例中断且未见其最终报告/证据或 tx-01-36 审阅结果；不推断未落盘进度，也不改变唯一审阅次数。已读取现行规范、技能、架构/T0资料、两版工作设计、v15报告/证据、contract v2 与本 README。IDA MCP survey/health正常；v16 E01/E02已落盘的IDA直接证据沿其原标注保留，本替代执行者继续W03的IDA-DIRECT与W04的SOURCE-DIRECT。tx-01-37的A20EB0仅作初始定位种子，E01另记tx-01-36的函数/raw复读。

v16报告/证据已完成分析和编辑。tx-01-36唯一整体人工静态文档审阅**通过（文档准确性与声明边界一致性）**；正常功能静态完整度仍部分完成，Authority-pre/normal-post writer候选与contract v2 §4 `+0x78` sole-writer冲突且未采纳。W03的`n298[2]`模式值、`v91`现场归属、context/成员实值与disconnect callback运行时dispatch未观察；自有ABI、Join lane、双participant S2及B retirement未生产实现。未运行测试、构建、harness、probe、状态模型或校验脚本；未操作游戏/进程、debugger、注入或Hook。

当前交付：[正常功能路径静态确认 v16](design-revision/virtual-device-owned-abi-static-confirmation-v16.md)；[静态证据 v16](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v16.md)。两份文件均记录进行中的静态功能边界及已通过的唯一文档审阅结果。

W01 已由本轮 IDA/source 静态读取补齐：`A20390`取得 lookup-local pair；`906020`另复制 by-value 参数；`A20EB0`的 `+0x7C/+0x80` 实际 target 是 `A212C0/A213B0`，各自通知副本在 callee 归还。`+0x0C` release 独立于可选 `n79_0`；B尾删时由 `A20EB0`消费 vector strong，中删时由 `A21570`首个移位临时消费，tail release另消费旧末项；调用返回前 by-value 与 lookup-local 临时也分别消费。v16证据提供逐事件表与 exact-pair retirement候选：确认缺席后只记native已消费账本，不再调用旧`ReleaseClass→port.ReleaseStrong`；保留 root/对象/control与DLL。`RemoveAndDestroy`会继续`FinalRelease`，不符合`NEVER`。W01静态正常路径已闭合，live成员、production接入及最终唯一审阅未完成。

W02 已由本轮 IDA/source 复读闭合静态入口与候选：`6FA540`在其guards后于`6FADA7`唯一调用`9BAC40`，scope需在该call前打开、`6FADAC` normal return时关闭；无B Player/consumer TLS也可在`9BB064`读取自有receiver/K的action19 Triggered。已区分另一个有Variant/state guard的`9BB13E` action19 removal分支、action14/15确认取消、20/21导航和native `+0x54→A1EE10` action-state写回。native flags可设置stored-confirm并绕过action14 query；`9BB415`是所读Main-vector创建尝试入口，其callee`9B9CD0`另有其他callers，因此只包该精确callsite，deny补4字节栈并走`9BB561`，不伪造null。证据列了重读、flush、导航取消、no-pass、同update新pass、other K、stored confirm和发布终态手工轨迹。candidate仍为`DESIGN_ONLY`，不是生产JoinControl hook或现场功能证明。

W03 已确认A真实helper链：Player+`0x1618`→`956110/9561E0/9562D0`→Manager keyed/fallback→`A20940/A209E0/A20AB0`→receiver `+0x30/+0x34/+0x38`；`K=0`是正常非负key。`9BAAA0/90B010`只返回HUD slot/context record，不是物理query receiver。`913760`的直接函数只删secondary/context记录或清source pair association，未写Player key，也未证明必由`A20EB0`调用。`95B87F/95BCA0`的context-empty分支通过`7A6450`分别使selected Player key失效、再从pair device `+0x0C`正向写回；`context nonempty`的`90AF30`分支不直接写`Player+0x1618`。另直接读完整`954B40`，其`n298[2]==2`条件下有Player[0] setter；模式含义/值与`v91`现场归属尚未观察。详细矩阵和限度见v16 E03。

W04 已对照当前A-only source：`ReadUniquePlayer`要求单元素Main vector，`InputQueue`/`ActiveDispatcher`只有一个client/sequence/planner/pending，`AdvanceOwnerGeneration`会清该唯一participant；不能直接用于A/B双participant。candidate分别保留A/B queue/sequence/history/planner/pending、共享Authority tick按参与者各Resolve一次且不等全员，JoinControl独立于0–11 gameplay state；retire只使B generation neutral并记native已消费vector ref，不能再经旧`ReleaseClass`或`FinalRelease`。Authority-pre/current与normal-post/previous候选和contract v2 §4的`+0x78` sole-writer相冲突，仍`DESIGN_ONLY / 未采纳`；未改contract/source/schema。W01–W04分析已完成，tx-01-36唯一整体文档审阅已通过（文档准确性与声明边界一致性）；未进行修补或复审。

## tx-01-37 v16 静态确认续接设计（2026-10-15）

本轮入口为[续接工作设计 v16](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v16.md)，详细执行规格继续沿[完整设计 v15](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)。此前v16子智能体查询为interrupted；本次live列表无旧实例，未见v16报告、证据或tx-01-36最终审阅记录。保留中断事实，由新的GPT-6-Luna、max替代执行者继续未完成的tx-01-36，仍交付报告/证据v16，不新增执行编号或验收次数。用户恢复IDA MCP后已成功读取survey/health及A20EB0，status=ok、32-bit isaac-ng.exe、base400000、image8FE000、Hex-Rays ready；不将服务恢复当成功能核验完成。任务继续聚焦引用账本、无B TLS的JoinControl、A真实input query/reselect及统一S2/lifecycle候选。tx-01-37设计交付唯一整体人工文档审阅**通过（设计交付范围）**，审阅后仅回填结果；本条不是v16执行结果。

## tx-01-35 下一步自有 ABI 静态确认设计 v15（2026-10-13）

下一轮执行入口为[工作设计 v15](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)：设计交付tx-01-35，用户指定GPT-6-Luna、max子智能体执行tx-01-36，交付报告/证据v16。目标收敛为native remove的临时pair/ReleaseStrong与coordinator账本、无B Player TLS时的首次JoinControl查询、A实际key→Manager→device query及remove/reselect后置，并合并一致的S2/lifecycle/attempt候选。删除入口、TopUpdateExit、Popup空回调、普通create no-create continuation、去重/held V2继承；不改source/contract/schema，不运行测试或游戏。tx-01-35设计交付唯一整体人工文档审阅**通过（设计交付范围）**，审阅后仅回填结果；本条不是v16执行结果。

## tx-01-34 v15 只读复核（2026-10-13）

v15正常功能静态确认仍未完整，“仅剩A context一条条件”和“候选内部一致”过强。复读确认：A20EB0在A20F56/A20F82直接调用control+0x0C，902D60/A21570涉及pair取得、复制与释放；必须说明入参临时strong及native已消费vector ref后的账本责任，不能只追n79_0或再次套旧ReleaseClass。v15自有B槽要求Player/consumer TLS、scope外中性，但首次Join发生在B创建前，JoinCommandLane到action19/14返回缺独立scope。9BAAA0返回controller/HUD slot，不能直接当physical input reader；应沿956110/9561E0/9562D0→Manager keyed callback→device slots核对，重点是通知/reselect后的A key、physical pair与fallback。两个Popup空回调、普通Variant0 B删除、TopUpdateExit和9BB415 deny补栈到9BB561可继承。证据开头../docs相对链接层级错误，新交付需勘误。此复核只读，未改v15或重跑tx-01-33审阅；原交付/通过记录保留，后续工作见上方设计。

## tx-01-33 自有 ABI 正常功能路径静态确认 v15（2026-10-13）

当前结果见[确认报告 v15](design-revision/virtual-device-owned-abi-static-confirmation-v15.md)和[IDA/source 证据 v15](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v15.md)，执行入口为[工作设计 v14](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v14.md)。两个Popup的callback/payload实际写入均为null；普通Variant0 B可经9BA000共享Main erase直接处理。B自有device的native remove另走A20EB0 shared InputManager exact-pair remove；DESIGN_ONLY顺序候选是在TopUpdateExit移除此B pair，复扫确认exact pointer缺席后只释放一次ManagerVectorStrong accounting并到UNREGISTERED，保留root/object/control/base和pinned DLL，再做B Player/context/selector清理。下一Main reader只枚举A。A键盘/手柄及B自有query接入一个per-participant S2、current/previous唯一writer和retire generation为DESIGN_ONLY候选，与contract v2 +0x78 writer不一致，未采纳。9BB415前拒绝第二actual create须补偿栈上的4字节并跳至9BB561合法no-create continuation，不伪返回null给无guard的setter。保留A的K经context keyed lookup回到下一物理query的具体条件边及现场值未观察；生产coordinator、two-participant S2与V2未实现。

tx-01-33唯一整体人工静态文档审阅：**通过（文档准确性与声明边界一致性）**；工作设计v14、v15报告/证据、相关source/contract及本README的事实来源与分类口径一致。正常功能静态完整程度仍为部分完成；P01/P04/P06有限边、S2候选未采纳、现场未观察和production未实现仍分别列明。审阅后仅回填结果，没有修补或复审。未运行测试、构建、harness/probe/smoke/canary/verifier、hash/link-check或状态模型；未改旧记录、source、contract/schema或IDB。

## tx-01-32 下一步自有 ABI 静态确认设计 v14（2026-10-13）

当前下一轮执行入口为[工作设计 v14](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v14.md)：设计交付 tx-01-32，用户指定 GPT-6-Luna、max 子智能体执行 tx-01-33，交付报告/证据 v15。聚焦两个具体 Popup 与 A 的五个后续场景、普通 B 精确删除及 native Manager remove/通知/A 继续、A 物理与 B 自有查询共用 S2、Join 第二次实际创建的合法阻止与控制流；继承 v14 已证 raw/canonical 去重与 held V2，只核实衔接。候选仍未采纳，不改 source/contract/schema，不运行测试或游戏。tx-01-32 设计交付唯一整体人工静态文档审阅**通过（设计交付范围）**，审阅后仅回填结果；本条不是 v15 执行结果。

## tx-01-31 v14 只读复核（2026-10-13）

v14 正常功能静态确认未完整，“只剩普通 B Leave 入口一条缺边”不准确。复读 source/IDA 确认：9BA000 的 Main erase 不只接受 Player+0x2C==1，其余分支也能到共享尾部；ECX receiver、两个栈 dword、ret8，当前函数不读取第二栈参数，不能只据反编译四参数继续列未知。Player vtable+0x28→6ACB00 为清理路径而非 deleting dtor。v14 保留 B 自有设备 REGISTERED 的候选未交付现行 remove/UNREGISTERED 顺序，A 物理设备与 B 自有对象不能混作 shared device。QueryS2 只检查 own receiver，A 键盘/手柄 S2 接入及退休 current/previous writer 仍未接完。Join 独立于其后的 Room guards；6FADAC 是 9BAC40 call 后续，6FA540 RET 在6FADB2。第二create“拒绝”未给合法原生 continuation。Popup n15 是 menu-state holder，具体 callback/五场景 A 结果仍缺；生产未实现、运行未观察与全映像 alias 不应并入真正 native 缺边。已证 Main vector 下一次枚举不再见被删 B，基础 ABI 和去重可继承。此次只读复核未改 v14 原文或重跑 tx-01-30 审阅；原交付和通过记录保留，后续实质工作见上方设计。

## tx-01-30 自有 ABI 正常功能路径静态确认 v14（2026-10-13）

本轮实际结果见[确认报告 v14](design-revision/virtual-device-owned-abi-static-confirmation-v14.md)和[IDA/source 证据 v14](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v14.md)。W01 更正 case2 Room bit0/raw 地址，并接到 Popup state3/关闭以及 context-empty 时对目标 Player 的 SetControllerId(-1,1) 条件；不能把 Online roster empty 当成 context empty，也未证明 A 已绑定新 K。W02 找到 9BC120→9BA000 的精确 Main Player 单成员删除和 9BA8F0 的条件context清理，证明 [A,B] 下可选择移除 B、重排后 A ordinal0保持；普通用户 Leave 到 caller guard 的入口仍未证实，query层 stale B neutral仍是 DESIGN_ONLY。W03 提供冻结 ClientId→ordinal、participant独立队列、Authority-pre current/neutral、normal-post previous/close的一套S2候选；与 contract v2 +0x78 sole latch writer冲突，未采纳。W04 给出raw→canonical→sequence精确同tick去重轨迹和V2 held mask候选：Shoot bit0、Item9 bit1、Card/Pill10 bit2、Drop11 bit3；edge bit0–3仍映射8–11。W05区分自有设备创建owner与B一次actual create attempt，追到9BB415、9BB421、9BB424/9BB429和6FADAC；完整attempt/publish terminal仅为DESIGN_ONLY。

总体静态文档目标已完成交付；native正常功能仍有上述有限缺边，C/S路由、V2、S2 publisher、JoinAttempt与B retirement没有生产实现。此次未运行测试、构建、harness、probe、smoke、canary、verifier、hash/链接检查或状态模型；未修改旧v13、工作设计、source、contract/schema或IDB。tx-01-30唯一整体人工静态文档审阅通过：报告、证据、工作设计要求、source、contract v2 与 README 口径一致；B Leave caller guard 缺边及 DESIGN_ONLY/未实现状态保留。审阅后只回填结果，未修补或复审。

## tx-01-29 下一步自有 ABI 静态确认设计 v13（2026-10-13）

当前下一轮执行入口为[工作设计 v13](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v13.md)：设计交付tx-01-29，用户指定GPT-6-Luna、max子智能体执行tx-01-30，交付报告/证据v14。W01准确勘误及Popup→A，W02 B专属解绑/退出和native清理，W03已有ClientId分流/参与者分区与Authority-pre唯一publisher工作候选，W04 raw/canonical持续量、边沿和去重，W05 JoinCommand/pass及一次B创建尝试owner。候选未采纳，不改contract/schema/source；本轮要求给完整候选接口/时序，而非继续并列未设计的选择。IDA只读服务和32位isaac-ng.exe数据库已确认正常。tx-01-29设计交付唯一整体文档审阅**通过（设计交付范围）**；本条不是v14执行结果。

## tx-01-28 v13只读复核（2026-10-13）

v13正常功能静态未完整，原“审阅通过”记录保留。复读source及IDA raw确认：E02的Room bit0条件写反，实际bit0非零跳955238替代分支，正常6FADC0要求bit0为零；显式Join/Room为case2，不可合并case1/2，6FA540实际call在9551BD。E06字节摘录漏9BB421的89 45 F0。E05把raw状态改变写成必产生新persistent state/sequence过宽，v1不编码action8–11 falling，量化也可能保持同canonical。S2候选混用了consumer original执行与query返回层，必须在consumer内部查询时生效。B-only退休/下一reader、A的具体Popup场景、参与者队列路由/唯一writer、Join一次attempt/发布/关闭仍需实质补全。此次只读复核未改v13原文或重跑tx-01-27验收；后续新工作见上方设计。

## tx-01-27 自有 ABI 正常功能路径静态确认 v13（历史交付；后续复核见上）

执行结果入口：[v13静态确认报告](design-revision/virtual-device-owned-abi-static-confirmation-v13.md)及[IDA/source静态证据v13](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v13.md)。本轮完成W01–W04实质静态研究：纠正Popup ID/K/v91/ordinal/result身份，确认Popup只按不同Join/Room条件影响后续路径；确认Main Player创建、全员erase/all-unbind和有限keyed reader；从当前单A Source推导per-slot A/B witness、publication、S2及generation候选；复读native poll current/previous、BuildRawSample edge和SubmitPhysical去重，并具体化9BB415 create、9BB41C selector K、9BB424绑定与6FADAC pass close。

正常功能静态目标仍部分完成。W02没有闭合B-only local-co-op解绑/退出及context/Manager清理和下一reader；W03候选未采纳，command到ordinal路由和contract v2 poll writer选择未定；W04的静态重复poll边沿结论已限定，但Join attempt owner/成功校验未实现。P01–P06分别列出IDA-DIRECT事实、真正native缺边、DESIGN_ONLY差异、现场未观察、生产未实现和未来整体验收事项。本轮未修改source/contract/schema/旧报告/IDB，未运行测试、构建、游戏或hash/链接检查。旧v12唯一审阅失败和tx-01-25复核记录保留在下文。

本轮唯一整体人工静态文档审阅结果：通过。要求、证据引用、source/contract 与 README 表述一致；W02 的 B-only 正常退休及 W03 的正式路由/契约接入仍未闭合，文档没有将它们表述为已完成。审阅后未自动修补或复审。

## tx-01-26 下一步自有 ABI 静态确认设计 v12（2026-10-13）

当前下一轮执行入口为[工作设计 v12](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v12.md)：设计交付tx-01-26，由用户指定GPT-6-Luna、max子智能体执行tx-01-27，交付报告/证据v13。本轮重点为Popup ID/K身份及UI→A后置、B正常解绑或退出与context/Manager清理、单A source到A/B witness和每人S2快照的具体接入、正常重复poll的去重条件/edge可达性、Join创建尝试的接入/终结时点。IDA只读服务及32位isaac-ng.exe数据库已实际确认正常；不修改contract/schema/生产source，不运行测试、构建或游戏。tx-01-26设计交付的一次整体人工静态文档审阅**通过（设计交付范围）**；本条不是v13执行结果。

## tx-01-25 v12 只读复核（2026-10-13）

v12正常功能静态确认未完整，原审阅失败记录保留。进一步复核发现9C01D0将递增Popup ID写入首dword，9C39E0返回该ID，95B310随后据ID查Popup；n298+4ABCC不能写成设备选择结果/K。Popup/UI对A后续Room/Player的条件仍需接完，B真正解绑或退出及清理次序未证。当前ReadUniquePlayer限一个Player/ordinal0，正常A↔B切换会触及owner generation，不能直接支持v12要求的B ordinal1 witness。SubmitPhysical仅抑制同completed tick/source、无edge且持续状态相同的level-only重复输入，不能外推所有重复poll。实际keyed call在95B6B3，95B6B6读返回pair；生产未实现和现场未观察应移出native静态缺边。Join的9BB41C取selector K，attempt成功接入点还需具体化。此次只读复核未修补或重跑tx-01-24审阅；后续实质工作见上方设计。

## tx-01-24 自有 ABI 正常功能静态确认 v12（历史交付；后续复核见上）

本轮结果入口：[v12 静态确认报告](design-revision/virtual-device-owned-abi-static-confirmation-v12.md)和[IDA/source 只读证据 v12](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v12.md)。已从当前 IDA 只读服务完整读取 95B310（反编译全文、747 条指令直至函数尾）及相关 UI/menu/context/Player/Join caller 与 callee。纠正 Mutex 与 indexed/keyed lookup 混淆，确认 95B395 是参数准备、95B39B 才调用 A20240；确认 Base cast-null 会进入 generic fallthrough，且 menu/context/Main Player vector 是不同对象。

W01–W04 的规定研究均已执行。W02 的实际结论是 95B87F 的 K=-1 setter、95BCA0 重绑、UI 提示及菜单转换均没有证明 B 从 Main Player vector 移除；A20EB0 移除 Manager pair 也不清 Player key。因而 B-only 正常退休和 A 的最终继续/暂停仍是有限且具体的静态开放边。W03 增补合法 native 跨类别 query 许可表及 poll→Room Authority→TopUpdateExit 的分段时序；Authority-pre S2 与 JoinCommand/attempt owner 仍是未采纳 DESIGN_ONLY 候选，未改 contract、schema 或生产 source。自有 ABI 注册、S2、Join lane 与 B-only retirement 均未生产实现。

过程未运行测试、构建、harness、probe、smoke、canary、哈希/链接检查或可执行状态模型；未改旧报告、生产 source/contract/schema 或 IDB，未访问游戏进程。tx-01-23 的设计审阅记录、v11 原始审阅失败及后续复核继续按历史保留。v12 唯一整体人工静态文档审阅**未通过（文档准确性/完整性）**：报告未明确列出 Online roster 为空这一正常前提，证据 E08 有 DESGIN_ONLY 拼写错误。按约定只回填结果，没有修补或重审。

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

## tx-01-23 下一步自有 ABI 静态确认设计 v11（2026-10-13）

当前下一轮执行入口为[工作设计 v11](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v11.md)：设计交付tx-01-23，用户指定GPT-6-Luna、max子智能体执行tx-01-24，交付报告/证据v12。IDA只读服务已实际返回正常，32位isaac-ng.exe数据库和95B310完整伪代码可读。本轮优先核对95B310完整分支、B/A后置和正常退休序列，纠正Mutex/callsite/无Room错误，补S2跨类别查询与时序、Join K/pass/创建尝试规则。tx-01-23设计交付的一次整体人工静态文档审阅**通过（设计交付范围）**；本条不是v12执行结果。未运行测试、构建、hash校验或游戏操作。

## tx-01-22 v11 只读复核（2026-10-13）

v11正常功能静态确认仍未完成，其原审阅失败记录保留。进一步复核发现：证据E05误把C57B2C的Mutex锁/解锁写成key=-1设备查询，并将95B395参数准备标成A20240调用（实际call在95B39B）；W03首次无Room时queue/planner/pending可先被PhysicalExit提交改变；S2候选没有规定原生跨类别consumer/action查询如何通过。v10已证的限定poll→Join→Room→Player链应继承，不重新列为整体未知。真正需补的是95B310后半、UI/目标Player/A影响/退休顺序及候选新增窗口；未实现、待采纳和现场未观察分别列明。此次只读复核未修补或重跑tx-01-21审阅。

## tx-01-21 自有 ABI 正常功能静态确认 v11（历史交付；后续复核见上）

当前执行结果入口：[v11 静态确认报告](design-revision/virtual-device-owned-abi-static-confirmation-v11.md)及[只读静态证据 v11](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v11.md)。W01 勘误与分类、W03 S2/pause候选、W04 Join K/pass/导航分析已完成；W02 已定位95B310本地解绑/reselect摘要并读取保存导出至95B4AB，但断线UI后续、实际Player removal、A影响和B-only退休次序未闭合，故正常功能静态完整性仍未完成。

本轮工具列表没有IDA Pro MCP或x32dbg，没有新增IDA-DIRECT。报告区分ARTIFACT-READ、SOURCE-DIRECT、STATIC-INHERITED、DESIGN_ONLY、运行值未观察和生产未实现。Authority-pre S2与独立JoinCommand是未采纳候选；contract/schema和生产源码未修改。tx-01-21唯一整体人工静态文档审阅**未通过（文档准确性/完整性）**：报告Q05/W02和证据E05的inline-code标记未闭合；报告W03对第一次无Room时queue/planner/pending的表述过宽，因为同一update的PhysicalExit可先提交输入。按约定只回填结果，没有修补或重审。未运行测试、构建、哈希/链接检查脚本或游戏操作。

## tx-01-20 下一步自有 ABI 静态确认设计 v10（2026-10-13）

本节保留当时的[工作设计 v10](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v10.md)执行入口记录：设计交付 `tx-01-20` 时计划由用户指定的GPT-6-Luna、max子智能体执行 `tx-01-21` 并交付v11；该执行结果现见上节与v11报告/证据。其目标包括纠正v10四项事实误述、追95B310本地入口、补全S2有效期和Join K/pass；当时未发现IDA工具，执行者应继续读取保存导出和源码。

`tx-01-20` 设计交付的一次整体人工静态文档审阅**通过（设计交付范围）**；本条为执行设计入口，不是v11核验结果。未运行构建、测试、哈希脚本或游戏操作。

## tx-01-19 v10 只读复核与后续本地设备线索

v10正常功能静态确认仍未完成，原文准确性也需勘误：device slot接收action值而非callback层的action指针；action8–11对应edge bits0–3；正常recovery不阻断时第二次连续无Authority会触发PauseEnter及generation clear；action20导航后继续其他selector，不能称整个扫描终止。Authority-pre发布候选仍缺post后的查询有效期与旧edge关闭说明。生产注册尚未实现、候选待采纳、现场值未观察应与实际静态缺边分开。

随后重读v2 W06/W07资料，已定位95B310本地断线/重新分配候选：主更新poll后954F84调用，95B87F可设key=-1，95BCA0可重分配key。lookup miss之后还有guard、断线UI及可能的Player移除分支；它对自有Base/B独立退休的适用性与完整后置状态待本轮追清，不能再笼统认为没有本地解绑入口。v10原交付与审阅记录以下按历史保留，不重跑旧审阅。

## tx-01-18 自有 ABI 正常功能静态确认 v10（历史交付；后续复核见上）

执行入口：[v10 静态确认报告](design-revision/virtual-device-owned-abi-static-confirmation-v10.md)与[只读静态证据 v10](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v10.md)，依据[工作设计 v9](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v9.md)。本轮纠正内嵌 pair 与 callback receiver，追到可见 context-list element ctor/insert/notification cleanup 和 Offline empty-context guard；从 Player COL/RTTI、address point、ctor vptr store 串到两条 virtual update 路径及六消费者；复读 queue/dispatcher/Authority/Room lifecycle 与 Join selector/key/confirm guards。

正常功能静态完整性仍未完成：自有 device 的生产 keyed registration 和 B-only normal Player exit/unbind/remove 顺序未闭合。Authority-pre-only 快照发布与独立 JoinCommand/pass/update-epoch 是内部一致但需改 contract/schema 的 DESIGN_ONLY 候选，未采纳。当前 roster/context/listener/key/selector/tick 等现场值未观察；C/S 自有 ABI、Join lane 和 B retirement 尚未生产实现。v9及更早报告、审阅失败/异常记录继续保留为历史。

本轮唯一整体人工静态文档审阅结果：**通过（文档准确性与所声明范围的一致性）**。审阅覆盖工作设计 v9、v10报告/证据、相关 SOURCE/CONTRACT 与本入口，未发现事实误述或文档间矛盾；正常功能静态完整性仍未完成，B-only exit/unbind 与生产 keyed registration 等缺边继续保留。审阅后仅回填结果，没有修补或重审；未运行测试、构建、哈希脚本或游戏操作。

## tx-01-17 下一步自有 ABI 静态确认设计 v9（2026-10-13）

当前下一轮执行入口为[工作设计 v9](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v9.md)：设计交付 `tx-01-17`，用户指定 GPT-6-Luna、max 子智能体执行 `tx-01-18`，交付确认报告/证据 v10。目标是纠正 pair/receiver 表述、追完关联列表的正常创建清理、连接 Player 虚分派、确定 B 正常退休路径、修订 Resolve 与未提交 epoch 接续，并核对 Join 重复查询的 selector/创建效果。继续只做正常功能静态分析。

`tx-01-16` 对 v9 的只读复核未认定验证完整，并发现事实勘误：element+0x378 是嵌入 device/control pair 的地址，source key 从其首项 device+0x0C 读取；A20940/A209E0/A20AB0 是 cdecl callback，Manager this 属于 A20FB0 dispatcher。列表 factory/Offline 条件、Player vtable到消费者、B解绑/退出、提前Resolve后的skip/pause/后续输入与Join过期、重复selector/创建条件仍需完成。v9原作者的文档准确性PASS记录如下保留，不作为这些缺项已解决的结论，也不重跑旧验收。

`tx-01-17` 设计交付的一次整体人工静态文档审阅**通过（设计交付范围）**；该设计不是 v10 执行结果，不表示正常功能已完成。报告/证据 v10 由执行者实际交付后更新入口。

## tx-01-15 自有 ABI 正常功能静态确认 v9（历史交付；后续勘误见上）

当前结果入口：[v9 静态确认报告](design-revision/virtual-device-owned-abi-static-confirmation-v9.md)及[只读证据 v9](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v9.md)。本轮对 Join action 19/20/21/14/15 的五个 immediate store 均重新直读；更正 +0x378 的对象身份：A22190 Gamepad 的 hardware descriptor 与 913760 list element 的 source association pair 是不同对象；callback owner 与 90AC70 lookup 使用同一 n298+0x4B3D8 vector，primary key 是 entry+0x0C 对 Player+0x1618，source pair key 是独立关联数据。

限定正常 state2 更新可由 IDA 静态串为 poll → 条件 Join → 条件 6FADC0/6FBA4D Room → Authority pre/original/post → entity/Player virtual dispatch；六类 consumer 到具体 Player vtable target 的关联仍 UNKNOWN。A20EB0 remove 与 Player key/unbind 分离；908070 是 Online lost-connection 的 all-Player reset，9BF530 是 state transition 的 all-Player remap；均不是已证 B exit。建议的 pre-poll publication epoch、同 epoch poll 幂等、独立 Join event/pass 与版本化 held 8–11 语义均为 DESIGN_ONLY。

结果分别判定：静态分析完成了范围内可确认的链，但 B 专属退出、consumer vtable 目标关联、运行时 roster/context/listener/key、真实自有 ABI/Join/event/held 接入仍未闭合或未实现。没有观察运行值、编辑 contract/schema/生产源码、运行构建或测试。tx-01-15 唯一人工整体静态文档审阅：**通过（文档准确性与声明范围）**；正常功能完整性仍未完成。此前 v8 来源标注审阅失败、启动异常及旧文档事实仍按历史保留，不重写。

## 当前自有 ABI 正常功能静态修订工作（2026-10-12）

下一轮执行入口为[工作设计 v8](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v8.md)：设计交付 `tx-01-14`，用户指定GPT-6-Luna、max执行 `tx-01-15`，交付确认报告/证据v9。目标是修正v8字段对象身份与证据来源，追通n298+0x4B3D8通知context/查询列表关系及限定正常poll→Join→Authority发布→room阶段，给出晚发布的快照/held/edge/Join接续候选并补正常B退休链；继续只做正常功能静态分析。

`tx-01-14`设计的一次整体人工静态文档审阅**通过（设计交付范围）**；该结果不表示tx-01-15执行或正常功能已完成。

`tx-01-13`只读复核未认定v8完整，新增定位：A22190的Gamepad +0x378是硬件描述块，不是913760列表对象的关联pair；90AC70在Item/PillCard路径的ECX同为n298+0x4B3D8，主要按entry自身key匹配Player key，诊断source pair须分开；既有loop锚点与IDA可给限定分支poll→Join→StandardRoom前Authority发布→room更新的静态顺序。v8的原来源审阅失败、启动异常和交付事实如下保留，不以其部分结论替代完整性。

### tx-01-12 v8 交付与重启记录（历史原文；审阅未通过，后续复核新增问题）

当前结果入口为[正常功能静态确认 v8](design-revision/virtual-device-owned-abi-static-confirmation-v8.md)、[只读静态证据 v8](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)和[完整执行设计 v7](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v7.md)。`tx-01-12` 接续用户指定的 GPT-6-Luna、max 目标，纠正 Join action编号、scalar三层ABI与固定action表，并逐项确认六类gameplay consumer、正常暂停/退休、cache关联、native update/Join及C/S Authority阶段边界。静态结果不表示正常功能已接入生产或游戏。

`tx-01-11`设计的一次整体人工静态文档审阅**通过（设计交付范围）**；该结果不表示tx-01-12静态执行或正常功能已经完成。

2026-10-12 用户报告v8子智能体异常并要求重新启动。主智能体读取到原实例 `/root/owned_abi_functional_static_v8` 状态为 `pending_init`，文件检索尚未找到本任务的v8报告/证据；已中止原实例，由同模型GPT-6-Luna、max替代实例继续原 `tx-01-12` 与工作设计v7。替代实例再次核对未发现v8草稿或已完成审阅记录，保留此次异常事实；重启没有另起目标或增加审阅次数。

#### tx-01-12 v8 交付状态

已完成设计v7的S01–S06静态分析和v8报告/证据编辑。IDA MCP只读可用，input/IDB为 `isaac-ng.exe[.i64]`，imagebase `0x400000`。本轮直接复核了action21/15调用点、device ST(0)→A20AB0输出指针/AL→9562D0 XMM0三层scalar ABI、BAB240/BAB370固定动作表、Manager+0x14 embedded Mutex receiver、poll=0 auto-remove及六类consumer调用点。v7的有效32项binding/构造/key事实标为继承证据，不冒称本轮读取。没有运行游戏/进程操作、测试、构建、模拟、verifier或链接校验脚本。

文档准确性结论与正常功能完整性分别见v8报告。P01–P06的系统链状态均逐项报告；正常功能仍有自有ABI/registration、Join事件、B退出/解绑、跨family/held模型、poll epoch和cache owner关联等未实现或未闭合项。

tx-01-12依据工作设计v7 §6执行的唯一一次人工整体静态文档审阅结果：**未通过（证据来源标注）**。E02将9BB05D action19标作“v7/W09继承并bytes复读”，将9BB2B0 action20标作IDA-DIRECT有限复读；本轮实际直接复读的是9BB2FE action21、9BB393 action14、9BB3C2 action15，19/20仅沿用v7/W09。事实值仍来自旧静态依据，但本轮direct/inherited标注夸大；审阅按设计未修正、未重跑。报告和证据保留了完整检查结果及功能不完整状态。

`tx-01-10` 后续只读复核未认定 v7 完整，并发现事实错误：9BB2FE 的 `0x15` 是 action21导航，action15取消在9BB3C2；A20AB0通过输出指针交付scalar、AL返回bool，上层9562D0返回XMM0；BAB240固定为0/1/2/3，BAB370固定为4/6/5/7，不能统称动态未知或直接判为cross-family。设备保留登记并中性只支持暂停输入，尚未证明正常退休。以下tx-01-09原审阅和交付文字保留为历史，不能据其限定范围的通过推出准确且完整。

### tx-01-09 v7 交付记录（历史原文；后续复核发现错误及缺口）

当前入口为[正常功能静态确认 v7](design-revision/virtual-device-owned-abi-static-confirmation-v7.md)、[只读静态证据 v7](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md)及[执行设计 v6](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v6.md)。`tx-01-09`已完成R01–R06；唯一一次人工整体静态文档审阅**通过（文档准确性与限定静态结论范围）**。没有运行测试、构建或链接校验器。

v7本轮直接修正：A20DF0在A20E55读取Manager vptr、A20E5C调用Manager slot30通知且ECX=Manager；A1E050从C7362C读key counter、以立即数B82758写Base vptr、再写device+0x0C key。正常`_main`初始化由sub_4307C0构造32项默认binding模板，C799F4单独是值为4的update辅助count；当前运行值不声称已观察。六类主要consumer逐项连到Player key、Manager keyed lookup和device query；动态side actions、helper bypass与cross-family覆盖仍有限。`SetControllerId(-1)`不等于B中性，优先候选为保留B正key/device并令自有查询中性。Join command/pass completion只提出DESIGN_ONLY候选，未实现。审阅通过只表示本轮静态文档一致，不代表生产实现或端到端游戏功能完成。

tx-01-09仅做限定正常功能的静态资料核验和新报告编辑；未编辑contract、生产源码、测试、verifier、decision或权限。v6唯一审阅未通过和更早历史失败原文保留为历史，不重写、不将其作为当前 gate；本轮不计算/校验哈希。整个目标的唯一审阅应同时核对v7报告、证据、执行设计和本目录入口，结果回填后才形成最终状态。

### tx-01-06 v6 交付记录（历史原文；审阅未通过）

`tx-01-06` 已交付[正常功能路径静态确认 v6](design-revision/virtual-device-owned-abi-static-confirmation-v6.md)与[只读证据 v6](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v6.md)，依据[执行设计 v5](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v5.md)。唯一整体静态文档审阅结果：**未通过**。发现 A20DF0 通知 receiver 写成 device vptr（指令显示 this=Manager）、P05 缺少逐项 consumer/action 地址对应、P02 C799F4 与 binding template 来源表达不清，以及两次工具输出意外包含历史哈希行；未修复或重跑。此状态不表示六条链已在生产代码或游戏中实现。

已确认普通 Join 直接枚举 InputManager 主 active vector；Base slot7 控制 update 辅助表/bit0，不是 Join 前置覆写。已有六槽 S2 状态目前只有 gameplay action 0–11，不能提供 native Join/confirm action 19/14；需要单独的 Join event source/epoch，query 仍保持只读。通用注册通知要求有效 name 槽及至少32项 native binding vector；模板运行时长度未知。remove 本身也不清 Player+0x1618，正常停用需要明确 B exit 或 unbind。

范围按用户要求限于正常功能路径；OOM、EH、锁/worker 并发、计数原子/溢出、全映像 alias/points-to、持久化及异常恢复不展开、不改判。本轮没有修改生产源码或 contract，也没有运行游戏、构建或测试。执行过程中曾有两次工具输出意外包含旧文档哈希行，未计算/校验/使用，已记录在 v6 证据并报告父智能体。

### tx-01-03 v5 交付记录（历史原文；实际审阅未通过）

上一子智能体的最终回报明确记载：唯一整体静态文档审阅未通过，原因含README标题重复及v5预填通过与实际结果矛盾。以下原交付摘要及v5正文的通过文字保留为历史；不作为当前完整性结论。本次更新删除目录重复标题并记录实际失败，不重跑tx-01-03审阅。

当前结果入口为[补充静态确认 v5](design-revision/virtual-device-owned-abi-static-confirmation-v5.md)和[证据 v5](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md)。tx-01-03 已完成 S01–S07 的只读静态取证与文档整理，并在全部编辑后完成唯一一次人工整体静态文档审阅。审阅通过的是证据边界、文档一致性和导航；这不表示全部 ABI 已验证完整、计数候选完整或 T0-05-05 已完成。

本轮直接修正：A1E760 两个调用 receiver 是栈上临时输出记录；ManagerBase / Manager 表身份与槽号统一，FindByKey 为 slot10/+0x28，slot30 notify 为 sub_A211D0；common 在 poll 前已执行 C4←C0、清 C0，返回0会在 manager lock 内触发通知、vector erase、control release；A1FC00 state1 unwind 存在 manager Mutex unlock action，state2–5另有 terminate action；C0=current、C4=previous，slot21 方向为 C4←C0；I5/I7/I9 计数候选矛盾已纠正，候选仍不完整且未采纳为契约。

仍未闭合或未实现：默认 binding 运行值及后续长度变化、native inline reader 闭包、callback/n79_0 目标与实际线程、RETIRING poll 返回策略、B/generation/Join 接续、计数 mirror/flags/diagnostic 的原子观察协议、真实自有设备的生产实现。v1–v4 与历史失败记录保留；v4 的旧审阅结论不作为当前完整性结论。本静态目标未运行 tests、构建或候选模拟；没有修改契约 v2、生产源码、测试、verifier 或机器决策。

### tx-00-09 v4 交付记录（历史原文；结论需按上述核查修正）

当前入口为[补充静态确认 v4](design-revision/virtual-device-owned-abi-static-confirmation-v4.md)与[证据 v4](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md)（整体推进 `tx-00-09`，依据[下一步工作设计 v3](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v3.md)）。本轮以**只读 IDA Pro MCP**（`ida-pro-mcp` 1.0.0，input/IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe[.i64]`，imagebase `0x400000`，PE32 i386 `SizeOfImage 0x8FE000`）完成 R01–R07，并在全部编辑完成后做**一次**整体静态文档审阅（v4 §10）。未 patch / rename / set_type / define_func / 保存 IDB；未运行游戏、调试器、构建或测试。

本轮对 v3 的修正（Q1–Q8 逐条见 v4 §2，新增 Q9）：

- **Q1 分配器栈位移**：证据 v3 的 `[ebp+0x10]/[ebp+0x14]` 是 IDA **frame 偏移**；原始字节 `0xA64910: 8B 5D 0C`（`mov ebx,[ebp+0xC]`，高）与 `0xA64917: 8B 7D 08`（`mov edi,[ebp+8]`，低）支持实际位移 `[ebp+8]/[ebp+0xC]`。OOM 出口是「诊断/回调 + **返回 NULL**」（`push 0x7FCB9DD6; call sub_A23200`），不是抛出。
- **Q2 容量不闭合**：`common sub_A1F280` 由 manager update 在 `0xA1FC97` **直接调用**（不经 vtable），并按**当时的** `n_bind=(end-begin)>>4` 现算 `4*n_bind` 访问 `+0xC0/+0xC4`。因此「两份缓冲各覆盖 `n_bind`」是注册进 manager pair vector 的**前提**，不是可选优化；`n_bind=33` 反例成立，槽覆写无法拦截这条路径。`n_bind` 惯用式在 `.text` 全段命中 13 处 / 9 个函数。
- **Q3–Q5**：EH 逐 state 表、cleanup/Init/slot5/Base dtor 的 receiver 与资源表、control block 六槽逐槽操作与有限交错，见 v4 §5/§6/§7 与证据 v4 §R03/§R04/§R05。
- **Q6 状态与审阅矛盾**：证据 v3 开头「进行中（K03–K07 待补）」与主报告 §12.7「状态一致 … 通过」互相矛盾；v4 §10 明确旧「通过」不支持完整结论，并保留历史原文不改写。
- **Q7 字段/边界笔误**：对象总大小只有 `0x370`（Base deleting dtor `0xA1E1CF: 68 70 03 00 00` + `operator delete`），故 `+0xB00` 不可能是对象字段，应为 `+0xC0/+0xC4`；UI 条件为 `0<=action<n_bind`；**device 每 action 4 个 primitive**、**record 每 action 2 个 primitive** 必须分列。
- **Q8 生产接入（不再混用）**：生产工程 `IsaacCsT0.Native.vcxproj` 的 `ClCompile`/`ClInclude` **不含** `src\virtual_device_lifecycle.cpp` / `include\isaac_cs_t0\virtual_device_lifecycle.hpp`，只在 `IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj`（第 51–55 行）编译；原型 `FinalRelease` 要求 `objectDeleted && implicitWeakReleased && controlBlockFreed` 并进入 `LifecycleState::Destroyed`，与 `NEVER`/契约 v2 §3 直接冲突。
- **Q9（本轮自纠）**：`insert sub_A20DF0` 的通知调用点是 `[manager vtable+0x78]`（`0xA20E5C: 8B 70 78`）。vtable 起点是 `0xB827F4`（`0xB827F0` 处的 dword 是 COL `0xBB033C`，TypeDescriptor `.?AVManagerBase@Input@KAGE@@`），故 `+0x78` = slot30 = `0xA211D0`，**v3 原结论成立**；本轮一度按 `0xB827F0` 对齐得出的 `0xA20FB0` 已由 RTTI 复核撤回。另发现 insert 末尾经全局回调指针 `n79_0 @ 0xC7E814` 用按值 pair 调用的第二条通知路径。

仍未解决 / 未实现：默认 binding 的运行期 count 与内容、`malloc(0)`/handler 现场行为、worker/main 实际交错与现场 COL/RTTI/UI/持久化；`sub_A211D0`(+0x7C) 与 `sub_A212C0`(+0x80) 的直接调用者、vtable slot1/2/5 的全部间接 caller、原生 inline 计数旁路；契约 v2 是否按 v4 §9 的 D1–D8 修订；真实自有设备生产实现与最小 Join/selected-device、退休恢复接续仍属 T0-05-05 剩余功能（v4 §8.5 四类划分）。`NEVER` 只保证自有对象/control 内存不释放，**不**自动维持 binding vector、双缓冲或 vptr 有效（v4 §8.4 的 L2 附加条件）。

历史：[补充静态确认 v3](design-revision/virtual-device-owned-abi-static-confirmation-v3.md)与[证据 v3](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md) 保留原文；其 §12「全部通过」已被本轮核查纠正，但支持其中的注册方向、复制宽度、分配回绕、cleanup 缓冲失效、EH 元数据与 mirror 永久过时反例。[静态确认 v2](design-revision/virtual-device-owned-abi-static-confirmation-v2.md)、[工作设计 v1/v2](design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v1.md)、[静态确认 v1](design-revision/virtual-device-owned-abi-static-confirmation-v1.md) 与原证据均保留；[设计修订稿 v1](../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)和[协调器契约 v2](../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)继续提供功能约定。历史别名批次保留为背景资料，不以其闭包、decision 或解锁字段作为推进前置条件。

## 历史决策与批次记录

以下保留各记录原有的“当前”“正式入口”和资格/权限表述，按当时语境阅读；冲突流程以现行规范为准。

本目录是 `CS-T0-05-05` 的只读入口。当前结论仅确认：若 Endpoint B 必须在 Authority 没有第二个实体手柄时持续工作，则 virtual device 是必要路径；它的实现资格尚未得到证明。

- Batch 2 已 create-new 生成 [native alias universe v4](../../../../../../../cs-t0/native-alias-universe-v4.json)、[ownership/lifecycle decision v7](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v7.json)及 [v7 只读候选入口](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v7.ps1)，候选集合差分为 register-IAT `2,333 -> 46 resolved + 2,287 retained`、switch-tail `763 -> 3 proven + 760 retained`，无新增 import escape/exact frontier；receiver 候选由父 row identity 推导为 `20,695`，graph `41 nodes / 44 edges`，首边 `0x00007455`，pair 仍为 `34`。但 [Batch 2 独立 TEST 结果](virtual-device-native-alias-fixed-point-clearing-batch2-test-results-v1.md)在唯一 Group 1 中因 oracle 把两个“有 source、无父 residual sink”的 source-only 点错误计入 source/function census 而 `REJECTED`；按 fail-stop，Group 2–4 未运行，公开 verification 输出未创建。因此 v4/v7 仅为保留的失败候选，不是正式入口，也不得据此提升 owner、权限或解锁。
- [ownership/lifecycle decision v6](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json)、[native alias universe v3](../../../../../../../cs-t0/native-alias-universe-v3.json)与[只读验证入口 v6](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1)现为正式入口。Batch 1 在固定映像闭世界内逐行清除了 18,107 个 IAT `FF 15` call、133 个 IAT `FF 25` tail transfer 和唯一 Manager service escape；callback slot `0x0087E814` 的 101 个引用全部为读取，零 writer、零 address escape、零 loader mutation，因此 3 个条件 callback frontier 及其重叠 escape 描述被清除。`_beginthreadex` 是唯一明确的 Manager nested-context import transfer，但已由 `0x0065A570 -> 0x0067F130 -> 0x0065A760 -> 0x006220C0` 固定链闭合，没有新增 `IMPORT_ALIAS_ESCAPE`。
- [独立 REVERSE Batch 1](virtual-device-native-alias-fixed-point-clearing-batch1-reverse-v1.md)确认 service vptr 只有 `0x00B9EE5C/0x00B9FB88`；derived 分支闭合到不读取传入参数的 callback，base 分支 `0x00662830 -> 0x006F05D3` 在 Manager context store 前进入 `_purecall`。v3 receiver graph 为 20 nodes / 26 edges，frontier 从 38,985 降为 20,744：474 base-only、270 split-register、16,323 residual、3,677 indirect-tail。pair graph 保持 10 nodes / 8 edges，frontier 从 37 降为 34：33 liveouts 加 RVA `0x00023D12` 的 exception/nonlocal edge。新的 universe 首边由集合排序得到 RVA `0x00006A75`，raw `FF D7`。
- [Batch 1 独立 TEST 结果](virtual-device-native-alias-fixed-point-clearing-batch1-test-results-v1.md)严格执行四组各一次并全部 PASS：internal oracle、public create-new、public verify-existing（72-file byte snapshot 不变）以及仅含两个 test methods 的参数化负向批次。[v6 verification](../../../../../../../out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch1-verification-20260909-v6.json) SHA-256 为 `301D91F8812A82B5F64052C3DAD27872E270F26199015123B4CCC6B19F68915D`。通过仅说明 Batch 1 的缩小和阻断决策可复核；两个 fixed point 仍非零，故 verdict 仍为 `UNKNOWN`、对象 owner 不提升、`formalDecision=null`、全部权限和解锁关闭。
- [ownership/lifecycle decision v5](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json)与[只读验证入口 v5](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v5.ps1)保留为历史入口。本轮仅执行 `NATIVE_ALIAS_CLOSURE`：固定 Win32/x86 PE 与 `objdump`，从 28 个 direct seeds、2 个 vtable cells 和 whole-image 35,189 个 indirect calls 建立 receiver census；489 个 base-only、270 个 split-register 与 34,430 个 residual 行精确分区，另保留 3,810 个 indirect-tail frontiers。receiver graph 为 18 nodes / 24 edges，返回 pair graph 为 10 nodes / 8 edges；35 个 pair seeds 中 2 个已展开、33 个保留 liveout。
- [native alias universe v2](../../../../../../../cs-t0/native-alias-universe-v2.json)与[独立 REVERSE v2 交接](virtual-device-native-alias-closure-reverse-v2.md)给出合法的保守结论：21 个 IDA noncontiguous tail/chunk provenance 全部改为显式 `UNRESOLVED_IDA_CODE_REGION`，8,941 个 function ranges 中 residual provenance 为 33,960 identified / 470 unresolved。`receiverDispatchFixedPointClosed=false`、`pairAliasFixedPointClosed=false`、`dynamicAliasUniverseClosed=false`；首个不可排除边仍是 RVA `0x00024037` 的 `FF D0`，即 control release 后通过零填充 callback slot `0x0087E814` 调用未知目标并可能保留 pair alias。
- [最终独立 TEST v12](virtual-device-native-alias-closure-test-results-v12.md)严格只执行四组各一次：internal oracle、public create-new、public verify-existing 和一个仅含两个 test methods 的参数化负向批次，全部 PASS。最终输出为 [verification retry-v11](../../../../../../../out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json)，SHA-256 `51191D57704C25E643688BB8698309D250E2B16A4717F9275CD275D4D25EB360`；39 项 verify-existing 快照前后相同。v1–v11 的失败计划/报告均保留，未删除或伪装成首次通过。
- [ownership/lifecycle 决策 v4](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json)及[只读验证入口](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1)保留为历史基线：离线生命周期原型已在独立 `Release|Win32` Application 中通过 16 cases / 332 assertions / 13 rollback；49 个固定 PE 锚点和 18 个 artifact binding 已通过 create-new 与 verify-existing。原型未接入生产 DLL、完整 `build.ps1` 或游戏加载路径。
- [最终独立 TEST 验收](virtual-device-ownership-lifecycle-v4-test-results-v1.md)记录压缩四组的最终通过链：group 1、create-new、verify-existing 和单个参数化负向批次均 PASS；负向批次 10/10 test methods 通过。此前 SDK 权限、陈旧断言、候选 fixed-goal、deny-list 误报和首轮负向批次失败均由各自 create-new 报告保留，未改写或伪装成首次通过。
- [REVERSE v2 交接](virtual-device-ownership-lifecycle-joint-reverse-v2.md)仍将对象 owner、创建/析构和引用计数三项 `nativeEvidenceStatus` 判为 `UNKNOWN`。v4 只把对应 `offlineImplementationStatus` 提升为 `OFFLINE_IMPLEMENTATION_CONFIRMED`；三项 `formalStatus` 仍为 `UNKNOWN`，`promotionCount=0`。
- [下一步原生证据闭合 v1](virtual-device-native-evidence-closure-next-step-v1.md)固定 alias universe、真实 factory/rollback、last-release caller/thread 三条只读工作及压缩验收；本阶段不进行注入或游戏测试。[PowerShell/JSON 清理记录](cs-t0-powershell-json-cleanup-v1.md)列出删除判据、唯一删除项和 JSON 保留原因。
- [v4 首次独立 TEST 结果](virtual-device-lifecycle-offline-test-results-v1.md)是历史失败快照：当次 group 1 在 MSBuild 读取本机 Windows SDK 路径时因访问被拒绝而停止；harness 为 `NOT_EXECUTED`，create-new report 为 `ABSENT`，group 2–4 未运行，当次不得填写 `OFFLINE_IMPLEMENTATION_CONFIRMED`。
- 用户授权继续后，[retry preseal v3](virtual-device-lifecycle-offline-retry-test-plan-v3.md)封印了新的 v2 输出和运行前 SDK 可读性门；[独立 probe 结果](virtual-device-lifecycle-offline-retry-probe-results-v1.md)因嵌套 PowerShell 变量被外层提前展开而以 ParserError 停止。权限提升已放行，但 SDK 可读性仍为 `NOT_OBSERVED`；retry/MSBuild/harness 均未运行，v2 输出仍不存在。
- 第二次明确授权后，[retry preseal v4](virtual-device-lifecycle-offline-retry-test-plan-v4.md)以参数安全脚本完成 SDK 探针 PASS，并消费一次新的 group 1；[独立 TEST retry 结果](virtual-device-lifecycle-offline-retry-test-results-v2.md)记录专用 Win32/x86 harness 实际执行 16 cases / 333 assertions，其中 13 个 rollback case 全部 PASS，但 callback move case 的一条旧子序列断言仍按修正前顺序检查，最终 15 PASS / 1 FAIL。该次 create-new report 因失败未创建，v4 candidate 与 group 2–4 当时继续锁定。
- v4 文件包括 lifecycle header/source/harness、独立测试工程、离线入口与只读 verifier；它们均不代表真实 virtual device。最终 PASS 验证输出为 `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json`，历史首次 PASS 输出与所有失败记录均保留不变。
- [ownership/lifecycle 联合决策 v3](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json)及[只读验证入口](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1)：哈希绑定 11 个前序产物与 36 个固定 PE 锚点；五项联合证明均为 `UNKNOWN`，`promotionCount=0`，工作包保持 `BLOCKED_BY_EVIDENCE`。
- [生命周期 coordinator 契约 v1](virtual-device-lifecycle-coordinator-contract-v1.md)：定义七态状态机、允许的 strong-ref 类别、失败 rollback、reader drain 与 owner-thread 约束；其状态为 `DEFINED_NOT_IMPLEMENTED`，规范文字不计作静态事实。
- [ownership/lifecycle 联合逆向交接 v1](virtual-device-ownership-lifecycle-joint-reverse-v1.md)：覆盖对象所有权、创建/析构、引用计数、调用线程和 manager vector 插入/移除所有权；首个反例包括 `{object,null-control}` 发布、callback 引用逃逸、remove/reader 无 drain、worker-thread 调用和 `uint16` 计数边界。
- [v3 独立测试计划](virtual-device-ownership-lifecycle-test-plan-v2.md)及[验收报告](virtual-device-ownership-lifecycle-test-results-v2.md)：候选前封印四组压缩验收；测试套件 4/4、生产 create-new 与 verify-existing 均一次通过且未重跑。
- [ownership/lifecycle 逆向交接 v1](virtual-device-ownership-lifecycle-reverse-v1.md)：对象 owner、创建/析构、引用计数三项的已闭合子链与最小反例；三项均保留 `UNKNOWN`。
- [ownership/lifecycle 独立测试计划](virtual-device-ownership-lifecycle-test-plan-v1.md)及[验收报告](virtual-device-ownership-lifecycle-test-results-v1.md)：历史 v2 候选前封印的全路径证明门；v2 结果为 `promotionCount=0`。
- [ownership/lifecycle 决策 v2](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v2.json)及[只读验证入口](../../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1)：历史入口，8 个前序绑定、17 个 PE 锚点，保持 6 项静态确认和 10 项未知。
- [静态逆向交接 v1](virtual-device-static-reverse-v1.md)：固定 Win32/x86 目标上的 6 项 `STATIC_CONFIRMED` 与 10 项 `UNKNOWN`。
- [独立验收报告 v1](virtual-device-decision-test-results-v1.md)：封印后的四项压缩测试、真实磁盘 PE create-new 和 verify-existing 结果。
- [机器决策 v1](../../../../../../../cs-t0/coop-virtual-device-decision-v1.json)：`necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`、`formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE`。
- [只读验证入口](../../../../../../../cs-t0/verify-coop-virtual-device-decision.ps1)：只读取磁盘 PE、机器决策和固定前序证据。

当前正式入口仍是 v6：Batch 2 v4/v7 候选已保留，但 Group 1 失败且没有正式 verification，不能替换 v6。正式可接受 frontier 仍按 v3 为 474 个 base-only、270 个 split-register、16,323 个 residual indirect calls、3,677 个 indirect tails、33 个 pair liveouts和 1 个 exception/nonlocal edge；Batch 2 的 `20,695` 只能视为等待新 create-new retry 链独立验收的候选结果。`offlineLifecyclePrototypeImplemented=true`、`implementationScope=OFFLINE_NOT_LINKED_NOT_LOADED`，但 `virtualDeviceImplemented=false`、`formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，`VIRTUAL_DEVICE_OBJECT_OWNER` 不提升。该结论不授权进程访问、游戏启动、IPC、passive/active canary、输入或世界修改；18 项权限及 `csT00506Unlocked`、`t005Unlocked`、`t006Unlocked` 均保持 `false`。alias universe 完整闭合后仍须另行执行真实 virtual factory/rollback 图与全部 reference-count/final-release caller/thread 工作；本批未提前开展。
