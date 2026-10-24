> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v18

日期：2026-10-16  
设计交付推进：`tx-01-42`  
静态执行推进：`tx-01-43`（W01–W04 是同一完整目标）  
预期交付：正常功能路径静态确认报告、证据 **v18**  
状态：`DESIGN_ONLY / 待执行`  
执行者：用户指定 **GPT-6-Luna，max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接[报告 v17](virtual-device-owned-abi-static-confirmation-v17.md)、[证据 v17](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md)及会话 `tx-01-41` 只读复核。v17 的 tx-01-40 已有一次整体人工文档审阅记录，本轮修正具体轨迹、候选规则和遗漏勘误，不重判或重跑原审阅。旧报告、证据、设计和历史结果保留。

## 1. 目标、当前完成情况和范围

本轮交付一份准确区分“未调用/调用返回false”的 Join 轨迹、一套不误抑制合法显式edge的S2候选规则，以及两处已定位的调用地址勘误，再同步结论和索引。**既定正常范围内原生静态主干已基本足够**；本轮主要是有限定点补读与说明修正，不重新展开基础 ABI 逆向，不以新增版本数代替实质进展。

v17 可继承：先query14再判断stored、vector责任与外部R分账、无B TLS JoinControl独立scope、普通Main B erase、A真实query/rebind条件、Authority-post与TopUpdateExit分开、S2中性/原生UI透传分域、raw/canonical去重和held V2候选、六consumer/action许可表。已有基础布局、38槽/六覆写/6槽control、构造、register/keyed lookup、pair临时/移位责任无需全面重读。

正常前提保持：有效分配、对象与缓冲，主线程顺序操作，Offline/local-co-op，**Online roster为空**，B唯一正K，创建B前Main Player vector仅A。Online roster、context、Main Player和InputManager pair vectors分别判断；进入9BAC40不等于其普通扫描所有guard已满足，轨迹逐项列条件。沿[原生Join可靠创建并发布B的用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，标 `USER-ASSUMPTION / DESIGN_ONLY`，不重做full Player Init/global manager-this。

保持A0/B1与session内冻结ClientId→ordinal；指针/K/handle只作当前generation关联。A/B同canonical、sequence、target tick、queue、dedupe、Resolve/apply，无即时旁路；C/S隔离原版/Isaac Sync，不依赖Online Manager、NetInputDevice、FrameStore或全员ready，通用InputManager可用。

保持自有38槽device、六覆写、六槽control、S2、NEVER；base ctor前pin DLL、ctor后自有vptr。BASE_CONSTRUCTED后对象/control/有效base不析构不释放，DLL常驻；每session至多一次设备创建，失败不自动重试。设备创建owner和B actual-create attempt owner分开。退休中性、Manager pair移除到UNREGISTERED、B解绑或退出，root常驻不授权重复扣引用。

正常pause/loading/noRoom、重复poll/query/pass、selector跨pass、新generation首个有效tick和B-only retire在范围。排除OOM/EH、worker并发、溢出/ABA、全映像alias/全部observer闭包、持久化、异常恢复、全部Mod/Lua及完整05-06网络证明。未实现、候选未采纳、现场值未观察不自动变成原生静态缺口。

## 2. tx-01-41复核交接与资料

以下为主智能体前次只读复核种子，标 `STATIC-INHERITED`，不是本次设计或执行者的新IDA-DIRECT：

| 项目 | 复核事实与本轮修正 |
| --- | --- |
| 已有selector的19调用 | 9BAE34–9BAE74检查四条同K且selected index非负的row并置associated标志；9BB03C据此跳到9BB10B，跳过9BB064。B尚无Main Player时对应Player局部为空，9BB10E–9BB110继续跳过受Player guard的19查询。后续selector仍可20/21→14/stored→actual create。v17 E02第2、3行把19=false列为实际query步骤不准确。 |
| 上轮设计前提 | 工作设计v17也把“既存有效row且19=false”列作轨迹。这可指命令mask配置为false，但不能冒充原生实际调用并返回false。新设计及报告明确更正该设计种子，保留旧文不改。 |
| S2 previous | v17报告称invalid previous一律不能产生Triggered，而E04又允许新tag命令的显式edge；当前active_apply.cpp::ActionTriggered对0–7比较previous/current，对8–11直接读current.edge_flags。需分开level-derived与explicit-edge，不把前者条件套给后者。 |
| 两处调用定位 | 95B867才是context-nonempty reset的90AF30 call；95B87F是context-empty的7A6450 call。95BBB6是selector字段清理，95BBCB才是其后条件90AF30 call。v17设计要求了勘误，但新交付未写入。 |

开工先读根AGENTS.md、现行规范、项目技能 `C:/Users/114514/.codex/skills/isaac-cs-refactor/SKILL.md`，再读架构计划、T0 task plan、cs-t0 README的现行要求及相关内容。随后完整读本文、topic README当前入口、v17报告/证据、[工作设计v17](virtual-device-owned-abi-static-confirmation-next-work-design-v17.md)和[contract v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)的相关ABI、输入、writer与NEVER条款。held V2和query-family表按[报告v16](virtual-device-owned-abi-static-confirmation-v16.md)相应段落继承，不能把当前V1源码当成V2已实现。

## 3. IDA和source的有限读取

用户已要求继续用IDA MCP。设计本轮只发现当前schema、未新调用IDB；tx-01-41实读曾为32-bit isaac-ng.exe，base `0x400000`、image size `0x8FE000`，输入 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe`，IDB同目录`.i64`，分析/Hex-Rays ready。执行者发现实际工具/schema，首个分析调用survey_binary minimal，仅使用非哈希元数据，再读server_health；以自己成功返回记录连接与读取，不能继承成新DIRECT。

| 定点窗口/source | 用途 |
| --- | --- |
| 9BAC40：9BAD31–9BAD3E、9BAE2F–9BAE7E、9BAED4–9BAEDC、9BB015–9BB06E、9BB103–9BB145、9BB264–9BB28F；必要时最短连接指令 | 追associated标志、Main Player局部、已有row何时跳过19，以及普通扫描guard。 |
| 9BB376–9BB3DF、9BB40E–9BB429、9BB561–9BB56E | 继承14/stored/15、actual call、bind与no-create；仅复读轨迹所依赖区间。 |
| 95B854–95B884、95BBB0–95BBD5 | 直接核对95B867/95B87F、95BBB6/95BBCB各自对象/条件/效果。 |
| active_apply.cpp::ActionMagnitude/ActionTriggered/BeginAuthority/EndAuthority/Evaluate；input_core.hpp的InputStateDraftV1；v16 held V2段落 | 区分现有source事实、同tag/previous候选约束、V2 held与explicit edge，列最小reader规则。 |
| 按需读取active_runtime.cpp的Authority/query wrappers、loop_runtime.cpp的StandardRoomHook/UpdateHook，以及queue/dispatcher相关局部 | 仅核对current/previous writer、original调用和Resolve/dedupe责任，不重做现有完整调用链。 |

每项关键结论附VA/RVA、实际原始指令/字节、guard、数据来源、控制转移、效果及限度，RVA=VA−`0x400000`。decompile不能代替关键branch/ABI指令；输出截断或cursor.done不能冒充区间已读。可用get_bytes和纯静态IDA Heads补齐，只读IDB，不patch/rename/set_type/define_func/save，不附着或读取进程。工具异常记录已读终点并保存草稿，继续独立source/文档，不循环重试或重做全部基础。

## 4. 同一执行目标内的具体工作

### W01 — Join：把command配置、未调用和返回false分开

重建已有有效B selector的最短路径：同K且selected index>=0使associated为真 → 新admission的19 call被跳过 → B尚无Main Player使removal查询guard也不成立 → key scan结束后进入四row扫描 → row guard、20/21、14/stored/15 → 精确9BB415或no-create。不能仅从9BB064的false分支接回row scan，就声称该入口在已有有效row的起态可达。

交付明确的三值事件列：`NOT_CALLED`、`CALLED_FALSE`、`CALLED_TRUE`；另列JoinControl command/mask配置。mask中19=0只能说明若被读会返回false，不说明实际发生调用。缺少实际native调用时不记query结果或query次数。逐K说明，不把其他K的19查询算到B。ordinary新admission可继承已读possessor检查曾查19、之后再次查19的分支；8/10在JoinControl域必须中性，不能把9BB064说成整个函数唯一19查询。

手工轨迹至少覆盖：

1. 本K无既存有效row、普通新admission guard满足，19实际false，无该K确认/创建；按实际分支列可能重复的19查询，不虚构一次固定调用。
2. B尚未创建、本K已有有效row：19配置为0，但实际19为NOT_CALLED；20/21=false、14=true，到actual call。
3. 同②，14=false、stored=1：先发生14，再检查stored，不查15，仍可actual call。
4. 同②，14=false/stored=0，15=true与false分开；前者清selected index，后者保留row。
5. row key/index或9BA980 guard拒绝、20/21导航：哪些query未发生、是否清row、是否仅导航，不写成全pass结束。
6. 新19 admission→本pass首次确认，以及下一pass直接使用旧row；分别列scope、row和command终点。
7. 同pass重复query/native+0x54、同update第二pass、owner已Succeeded时再到9BB415：query只读、旧edge不复活、deny补4字节栈到9BB561→9BB567。
8. no-pass/expired命令、retire遇pending publish；不制造scope/query/attempt，或按已有attempt进入RetiredFinal，不能重试。

保持JoinControl scope在9BAC40/query之前打开、6FADAC关闭，不依赖19成功或B Player TLS；query不consume/Resolve，+0x54不确认私有sequence，控制动作不混进gameplay 0–11。actual attempt owner仅管精确B K/row的9BB415，不拦其他creator；9BB424只bind，Succeeded仍沿用户接受发布后置。若引用循环索引，零化在9BAC85，9BAC87是循环头，不写成每次回跳都重新初始化。

### W02 — S2：level-derived与显式edge分别判断

先以SOURCE-DIRECT记录当前V1：ActionTriggered对0–7比较previous/current的量，8–11读current.edge_flags；这些是既有source行为，不能把新generation候选约束或V2 held写成已有实现。

本轮候选明确两个predicate：

- `CurrentEligible`：活动C/S受控gameplay域、正确consumer/action、session/ClientId/冻结ordinal、当前participant/binding generation、exact pair/K、visible Authority tick及已Resolve的current均匹配。任一不满足，所有受控输出中性；不能因explicit bit置位绕过这些条件。
- `PreviousComparable`：previousValid且与current同一身份/session/generation/binding，来自正常已提交的previous。该条件只用于需要比较previous/current的level-derived Triggered，不是全部Triggered的共同前提。

| 类型 | 候选Triggered规则 | previous无效但current有效时 |
| --- | --- | --- |
| 依赖连续量前后比较的0–7 | CurrentEligible且PreviousComparable后，按既有零→非零比较。 | 此类Triggered中性，不拿旧generation previous制造edge；Pressed/Scalar仍按当前有效量。该首tick策略标DESIGN_ONLY。 |
| 8–11显式edge，含Bomb8 | CurrentEligible后只读当前target已Resolve命令的对应edge bit；不从previous/held重新推导。 | 合法新tag edge仍可返回true，不能被previousValid=false整体抑制。 |
| invalid/stale current、旧tag edge、expired/错误target、退休participant | 不具备CurrentEligible。 | 全部中性，previous是否有效不改变结果。 |

继承V2 held mask `0x0F` 对应Shoot、Item9、Pill/Card10、Drop11；edge mask `0x0F`仍对应8–11，Bomb8 edge-only。held只负责相应持续量/Pressed/Scalar语义，不代替explicit edge。不得把“按held上升沿重造9–11事件”作为修复。V2尚未改codec/queue/planner/adapter/schema，本轮不改。

给最小候选reader的条件表/声明式步骤及手工状态表，不执行状态模型。覆盖新generation首个current有Bomb edge但无previous、首tick连续量非零、同generation无新edge、old-generation edge到达、同tick重复query、下一tick没有新edge、pause/loading/rebind、B-only retire但A继续。每行列current/tag/tick、previousValid/tag、edge来源与target、Pressed/Triggered/Scalar及post后字段。

exactly-once责任在sequence/dedupe/target/queue/Resolve和已证world/consumer调用边界；query对同一snapshot重复读等值，不靠查询时清edge。不能把重复true query自动称为任意consumer副作用恰一次，也不能把explicit edge复制到下一Authority tick。显式neutral marker与无效current分开；normal Authority-post只提交仍属于当前generation的合法current或合法neutral snapshot，退休/失效的旧tag不得在post复活。

保持Authority-pre发布current、StandardRoom original正常返回后Authority-post提交previous/关visible、完整g_update返回后的TopUpdateExit处理退休和pause-gap。generation transition只失效相应tag/visible，不另写previous。活动受控gameplay身份不确定时neutral；UI/非受控/恢复后物理域按原生；A wrapper的original恰一次与最终返回值分开，B owned reader无虚构Base original。硬件sampler不读synthetic，不回投B输出。

### W03 — 调用地址、对象与后置勘误落盘

必须在v18证据、主报告摘要和README当前记录中写出以下区分，不能只引用旧v17/v16错误表：

| VA | 应核对的事实 | 不应混同 |
| --- | --- | --- |
| 95B867 | context nonempty分支，ECX指向context集合，调用90AF30(context,-1)。 | 不是95B87F；不直接写Main Player key。 |
| 95B87F | context empty分支，先按selected index取得Player，调用7A6450(Player,-1,1)。 | 是Player key invalidation；只有selected identity是A才称A失效。 |
| 95BBB6 | 当前selector字段写-1，随后还有selector/global flag与context guard。 | 不是90AF30 call，字段清理不等于Player rebind完成。 |
| 95BBCB | context非空条件下，调用90AF30(context,detectedK)。 | 是context source关联，不是Main Player正向key setter。 |

给调用前寄存器/栈实参来源、实际call及后续的最小raw摘录；K、selected Player/context的运行值保留为条件。95BCA0正向Player setter等沿继承条件，不重开完整95B310/UI/Popup研究。

### W04 — 合并候选与准确交付

v18报告开头分别回答：①限定正常路径的原生静态依据是否足够；②W01实际query事件是否准确；③W02候选内部是否一致；④W03两处勘误是否真正落盘；⑤真正仍需补读的有限边（若有）；⑥未采纳/未实现/现场未观察各是什么。

继承v17正确的vector账本：ΔV_B=-1、root由既定owner保持、总量 `ΔStrong_B,total=-1+ΔR_B`；有条件listener可改变R，native消费V后coordinator只记责任、不二次ReleaseClass，不FinalRelease。只引用，不要求重读全部listener/alias。

保持writer候选与contract v2 §4 `+0x78` poll唯一writer的差异为 `DESIGN_ONLY / 未采纳`，不改contract、不把采纳或production编码当成本轮静态完成条件。六consumer/action许可表与participant独立queue/sequence/history/planner/pending继承，不恢复strict单family拒绝，也不改协议。

新报告/证据集中列旧说法→新说法→来源，包括旧设计v17自身的Join种子。P01–P06仅作索引，原生静态依据、候选一致性/采纳、现场值和production分别列。未知live K、v91、mode、pair/listener状态不自动成为静态缺口；全部Mod/alias等排除项不得恢复为先行证明。

如果没有新反例，允许明确结论“在声明前提内静态依据足够；剩余是契约选择、实现和最终运行验收”。若仍有影响目标的真实静态未知，列准确函数/字段/分支、已读终点及最小下一读取；不使用“所有生命周期未闭合”等泛化结论，不自动开启下一轮。

## 5. 文件、证据标签和操作范围

执行者仅新增/编辑：

1. `docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v18.md`。
2. `out/cs-t0/virtual-device-owned-abi-static-evidence-v18.md`，必要分册同v18前缀并被引用。
3. `docs/refactor/cs-t0/t0-05/07-virtual-device/README.md` 的当前入口、tx-01-43结果及必要勘误，保留tx-01-42及全部历史事实。

不改旧报告/证据/工作设计、规范、contract/schema、production source或IDB，不还原/清理已有仓库改动，不提交/推送Git。子智能体不再委派、不创建侧栏聊天。主智能体交出上述文件和本轮IDA只读使用权，执行期间不并发编辑或操作IDA。

区分IDA-DIRECT、SOURCE-DIRECT、STATIC-INHERITED、STATIC-DERIVED、CONTRACT、USER-ASSUMPTION、DESIGN_ONLY。本次设计的source复读不等于执行者亲读；逐项说明来源，不将旧证据改标DIRECT。报告到out用 `../../../../../../out/...`，证据到docs用 `../../docs/...`，topic README到out用 `../../../../../out/...`；人工核对引用，不跑链接检查脚本。

永不设gate、永不计算或校验hash。过程中不运行测试/构建/harness/probe/smoke/canary/verifier/link-check或可执行状态模型；不操作游戏、进程、debugger、注入或Hook。文件/source/IDB静态读取、rg、反编译/raw和手工表推理允许。PowerShell可用login:false。

## 6. 唯一整体人工文档审阅

tx-01-43全部分析与编辑结束后，只做一次覆盖本文、v18报告/证据/分册、README修订及相关source/contract的整体人工静态文档审阅。审阅三类修正是否实际完成、NOT_CALLED与false是否分开、explicit edge不依赖previous的条件是否一致、失效current仍中性、地址/对象/参数是否准确、writer与引用责任是否继承正确、标签和历史保留是否清楚。

不得预填PASS。结束后只回填实际结果；失败记录未完成项，不自动修补、复审或改编号重跑。分别给文档准确性、限定静态完整程度、候选一致性与采纳状态、现场未观察和production未实现结论。不要求现场观测或任何测试。工具中断也落盘证据/草稿和准确终点，不只留下“正在执行”。

## 7. tx-01-42设计交付记录

本次完整目标为写出上述可执行收尾设计并按用户要求启动GPT-6-Luna max子智能体。新增本文、README补记tx-01-41复核与tx-01-42执行入口；依据为现行规范、v17产物、前次只读复核、contract及本次复读source。未新调用IDA，未测试/构建或操作游戏，未改旧交付、source、contract/schema或IDB。

设计交付唯一一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文与README修订，并对照现行规范、v17产物/tx-01-41复核、contract和已读source；核对Join未调用/false分离、level-derived/显式edge与current有效条件、两处地址、继承边界、来源标签、引用及执行约束。审阅后仅回填结果，未复审；本条不是tx-01-43执行结果。
