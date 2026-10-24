> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v15

日期：2026-10-13  
设计交付推进：`tx-01-35`  
静态执行推进：`tx-01-36`（W01–W04 属于同一完整目标）  
状态：`DESIGN_ONLY / 承接 v15 复核；确认报告 v16 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-34` 对[报告 v15](virtual-device-owned-abi-static-confirmation-v15.md)、[证据 v15](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v15.md)及[工作设计 v14](virtual-device-owned-abi-static-confirmation-next-work-design-v14.md)的只读复核。v15 补上了两种 Popup 的空回调、普通 B 删除入口、TopUpdateExit 和普通 create 调用点的 no-create continuation；但退休引用责任、首次 Join 的查询接线及 A 后续实际 input reader 尚未完整。旧 v15 和审阅通过记录保留，不能据此称仅剩一条 context 条件或候选已内部一致。本轮新报告纠正和补全，不改旧交付。

## 1. 完整目标与工作边界

本轮交付 v16 报告、证据、topic README 实际结果。完整目标是给出一套有限正常路径的静态依据和工作候选：①native Manager remove 的真实引用增减与自有 coordinator 账本无重复责任；②B 尚未创建时，JoinControl 命令可经自有设备查询触发普通 native Join，并与 gameplay S2/UI 分开；③B 退出或设备 remove/reselect 后 A 的实际 key→Manager→device query 结果有明确 guard 与 receiver；④把三者接回唯一生命周期、S2 writer、Join attempt 与恢复 owner，并准确分类剩余项。

W01–W04 是本轮内容编号，和旧报告 W 编号分别解释，不增加推进或验收次数。已证 Main member erase、TopUpdateExit、Popup 空回调、no-create continuation、raw/canonical 去重、held V2 与六消费者/action 表继承使用，不全面重做。

正常前提：有效分配、对象和缓冲、主线程顺序操作、Offline/local-co-op、**Online roster 为空**、B 使用唯一正 K、创建 B 前 Main Player vector 恰为 A。Online roster、context vector、Main Player vector、InputManager pair vector 独立判断。沿[原生 Join 可靠创建并发布 B 的用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，标 USER-ASSUMPTION / DESIGN_ONLY；不重做 full Player Init、global manager-this 或 05-06 全网络证明。这个假设不代替 JoinCommand 到查询返回的接线，也不预设 A 的所有 key/context 分支均相同。

保持 device 38 槽、六覆写、control 六槽、S2 与 NEVER。首次 base ctor 前固定 DLL，ctor 后写自有 vptr；BASE_CONSTRUCTED 之后对象/control/有效基类状态不析构或释放，DLL 常驻。每 session 至多一次创建设备，失败不自动 retry；B 实际创建尝试独立 owner。退休输出中性，B 解绑或退出，自有设备从 Manager vector 移除到 UNREGISTERED；不能保留 B REGISTERED 来替代退休。

A ordinal0/B ordinal1，ClientId→ordinal session 内冻结，Player*/device*/K/平台 handle 仅为当前 generation 临时关联。A/B 同 canonical、sequence、target tick、queue、dedupe、Resolve/apply，无房主或 B 即时旁路；通用 InputManager 可用，C/S 不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。A 物理设备与 B 自有设备不因为使用同一 Manager 就变成同一对象。

正常 pause/loading/noRoom、重复 poll/query/pass、首次 Join、B 退休和 A 后续输入在范围内；排除 OOM/EH、worker 并发、溢出/ABA、全映像 alias、持久化、异常恢复、全部 Mod/Lua 行为和完整网络证明。保留线程、生命周期及 Hook 恢复约束。本轮不操作游戏，不扩大实机授权。

tx-01-35 与 tx-01-36 分别在完整交付结束后只做一次人工整体静态文档审阅；过程中零测试、构建、实验、可执行状态模型或校验脚本。不设 gate，不校验任何哈希，旧 gate/解锁字段不作为推进条件。

## 2. v15 复核结论与定位种子

以下为 tx-01-34 主智能体直接读取或资料对读所得，执行者继承时注明来源；自己的 IDA-DIRECT 只标本轮实际成功读取。关键 ABI/条件应复读 raw，不能用 decompiler 的猜测参数或工具 done 替代实际区间。

| 项 | v15 问题与复核事实 | 本轮责任 |
| --- | --- | --- |
| Q01 | A20EB0 在 A20F56 对 vector 尾 pair control 调 +0x0C，在 A20F82 对入参 control 再调 +0x0C；n79_0 是之后由 AL/非空条件触发的回调，不是这两个 ReleaseStrong 的唯一入口。 | W01 给实际 release/临时 pair/账本责任，不能只追 n79_0 判断是否减过引用。 |
| Q02 | 902D60 用 control+0x04 取得 pair 引用；A21570 移位通过复制、交换、ReleaseStrong 处理对象。906020 的 A20390 lookup→902D60 参数复制→A20EB0→局部 pair release 是正常 caller 形状。 | W01 准确说明 by-value 入参所有权与 vector 删除增减；区分账本记录和再次执行 ReleaseStrong。 |
| Q03 | v15 的 B owned query 要求 Player/consumer TLS，scope 外中性；首次 Join 在 B 创建及 gameplay TLS 之前，未给 JoinCommandLane→action19/14 查询返回的独立分支。 | W02 建立不依赖 B Player* 的 JoinControlScope/read owner，明确查询优先级和 pass 寿命。 |
| Q04 | 9A8620/9BAAA0 属于 HUD/controller slot 分配；9BAAA0 返回 slot 编号或 -1，不能当作返回 physical query receiver 的函数。 | W03 将 HUD 与实际 input query 两条链分别记录。 |
| Q05 | 实际 gameplay helpers 956110/9561E0/9562D0 读 Player+0x1618，进入 Manager keyed callback 或相应 fallback；956110/9562D0 的已读非负 key 路径不因 context 非空就必须经 90B010。 | W03 沿真实 query，核对 remove通知/reselect 是否改变 A key、physical pair 或 fallback，现场值单列。 |
| Q06 | v15 证据开头用 ../docs/...，从 out/cs-t0 不能到仓库 docs；“两个 Popup 没有 callback”是已证事实，不是待读取缺边。候选未采纳、现场未观察、source 未实现不能代替 native 缺边。 | W04 修正新交付的引用、类别及完成结论，保留旧原文。 |

已证可继承：两个 95B310 Popup callback/payload 为0并复制到对象；普通 Variant0 B 可直接到 9BA000 共享 Main erase、ECX receiver/两栈 dword/ret8；+0x28→6ACB00 为 cleanup；下一 Main vector dispatch 只见 `[A]`；TopUpdateExit 在完整原生 update 返回之后；9BB415 deny 时补 selected-type 栈 dword 后到 9BB561→9BB567 是所读调用点的明确 no-create 候选。6FADAC 是 9BAC40 返回后续，6FA540 RET 在6FADB2；Join 早于独立 Room guards。

## 3. 资料、工具和有限读取范围

先读 AGENTS.md、现行规范、项目技能 `C:/Users/114514/.codex/skills/isaac-cs-refactor/SKILL.md`、架构计划、T0 任务计划与 cs-t0 README；再读本文、v15 报告/证据、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)及本轮实际 source。旧流程由现行规范覆盖；读历史事实不执行旧检查入口。

tx-01-35 当前元数据实际返回：IDA MCP status=ok，auto-analysis/Hex-Rays ready，32-bit isaac-ng.exe，base 0x400000、image size 0x8FE000，输入 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe`，同目录 `.i64`。未计算或校验哈希。执行者发现实际工具/schema，首个分析调用 survey_binary minimal，只使用非哈希定位信息，再读服务状态；工具错误保留已读终点，继续独立 source/候选工作。

| 主题 | 种子和读取用途 |
| --- | --- |
| pair 参数/引用 | A20EB0 全正常体及 A20F4A–A20F9A；902D60；A21570；906020 的 906054/906069/906073/90608B；A20390；Manager +0x7C/+0x80→实际通知目标。追本轮临时 pair acquire/release，不做全引用 universe。 |
| 自有 control/source | contract v2 §5 的 TryAddStrong/AddStrong/ReleaseStrong；`virtual_device_lifecycle.cpp:96–176/360–406`、对应 header EngineLifecyclePort。已存 source 是旧 port 抽象，不把它的 ManagerRemove/ReleaseClass 责任原样套到 native。 |
| Join/control | 6FA540→9BAC40 entry/return；9BB05D/9BB064 action19、9BB0B8 native +0x54 flush；9BB393/9BB39A action14、9BB3C2/9BB3C9 action15、9BB2B0/9BB2FE 导航；9BB40E–9BB429、9BB561–9BB56E。Manager +0x74/A20FB0→A209E0→owned +0x34。 |
| A input helpers | 956110 的 95614E–9561CA；9561E0；9562D0 的 956305–956394；A20FB0、A20390、A20940/A209E0/A20AB0；A physical +0x30/+0x34/+0x38。特殊前置 guard 和 fallback 按实际函数分开。 |
| remove/reselect | A20EB0 通知、913760、90AF30/90AC70；95B310 的 selected-device lookup、95B867/95B87F、95BC4D/95BCA0、95BBB6；7A6450。沿 v10/v15 已证数据流只补 A key/pair/reader 条件。 |
| HUD/阶段/当前接入 | 9A8620、9BAAA0、90B010 仅为 HUD/context 关联；9BA000/4186C0/TopUpdateExit 继承。`active_runtime.cpp:489/584/653/709/839/908/937/986/1036/1354/1394`、`loop_runtime.cpp:1002–1018`、`active_apply.cpp:71/129`、input_core/active_dispatcher 的 client_id/queue/Resolve。 |

以三条缺链需要的函数为限，每条可沿结果有限深读4–8条相关 caller/callee 路径；记录选择依据、guard与准确终点。继承事实不需要重新逐函数出证，不要求所有未来 writer、所有 native alias 或原版 Leave 按钮可达。

## 4. 同一目标内的执行内容

### W01 — Native remove 引用增减与 coordinator 账本

复读 A20EB0 raw，把 +0x0C 的调用与后置 n79_0 条件分开。沿 906020、A20390、902D60 的正常参数复制，确定传给 A20EB0 的 pair 是否持有独立临时 strong、谁取得、谁消耗、返回后谁清局部 pair。不能把原生 by-value pair ABI 写成任意借用的两指针。

沿 A21570 准确说明 erase 后移位如何复制/交换/释放，区分“目标 B pair 在尾部”和“目标后仍有其他 pairs”。移位时 +0x0C 的 receiver 不必总是 B；给整体净增减。两次通知各自复制的 pair 和 callee release 一并对账。已有其他正常引用可用符号 R 表示，不要求外部引用全为零，不恢复旧 strong 上限/全 alias 证明。

交付一张逐事件手工账本：root、vector、lookup-local、by-value argument、notification temporary、shift temporary、其他 R 的 acquire/release owner、control slot、净增减及返回后责任。明确 n79_0 调用有/无为何不能决定前面的 ReleaseStrong 是否执行。若回调目标会改变本轮计数，限定读取对应 writer/目标和副作用；否则说明为什么不需要扩大追踪。

给可实现的 native bridge/coordinator 候选：临时入参以已证方式取得/复制引用；native erase 已消费 vector ref 后，自有 coordinator **只记录已经完成的责任**，不能再通过旧 ReleaseClass→port.ReleaseStrong 重复减同一引用。需要额外释放的只能是表中明确独立持有的临时引用。分别命名“记录 vector ref 已由 native 消费”和“实际调用 ReleaseStrong”，给最小接口/状态更新和 exactly-once 责任。若选择另一 wrapper 所有权，须给同样完整账本。

在已证 TopUpdateExit 串行点接回：关闭 B admission/visible并失效 generation → native notify/remove 与引用责任 → exact B pair absence → UNREGISTERED → native B member cleanup/ordinal与关系后置 → A 下一 reader。排序调整需说明正常数据依赖；保持对象/control/base/DLL常驻和规定 root。不能调用旧 RemoveAndDestroy/FinalRelease，也不能把“NEVER 不释放内存”当作可以任意多减计数、清掉 root 或 control+0x14 的理由。

手工覆盖 pair 位于尾部/中部、重复 retire 请求、已缺席时不再次消费同一 vector责任、A pair retained。异常/并发不扩展。交付 raw/ABI、账本、bridge API、退休顺序和 source 修改责任；不是仅给“n79_0待研究”结论。

### W02 — B 创建前的 JoinControl 查询路由

给独立的 `JoinControlScope/JoinControlReader` 候选，不依赖尚不存在的 B Player* 或 gameplay consumer TLS。pass scope 必须在 native action19查询之前建立，不能把“action19查询为真才能开scope”作为循环前提。明确 entry/exit Hook、main-thread/Offline条件、session、预留B ClientId、B own device/K、updateEpoch、passToken、commandSeq、action和命令有效期；updateEpoch不是Authority tick。

列 owned查询的明确优先级：①有效 JoinControlScope 下，exact B own receiver/K对应控制 lane 的允许动作；②有效 Player/consumer gameplay scope 下读该participant S2；③其他UI/outside/retired/stale情况中性。A physical UI查询保留自己的native行为；硬件采样不读合成控制值。初次Join不进入gameplay InputQueue/Resolve，也不把19/14/15/20/21强塞进只有0–11的gameplay state。

逐项定义控制动作：action19开/选、14确认、15取消、20/21选择导航，Pressed/Triggered/Scalar各返回什么、是否只读、重复query如何可重复读而不重放到第二pass。native +0x54 flush 是继承槽对 C0/C4 的动作，不等于消费私有 JoinCommand；说明它与控制输出的关系，保持六覆写约定，不静默增加第七覆写。避免额外输出8/10组合触发其他创建路径；只对本轮普通B路径作有界结论，发现同一控制输出可走另一已读create时必须指出实际guard/call，而非把局部callpoint泛化成全session所有原生入口。

继承 v15 的9BB415 Admission：pass/client/K/token匹配且未尝试才BeginAttempt；允许时原callee恰一次，9BB429观察绑定；deny时补栈到9BB561 no-create。具体绑定前/后、pass close、用户给定publish后置、Succeeded/FailedFinal/RetiredFinal，均与JoinControlReader可见期衔接。原生query只读；一次副作用由 actual create owner处理，不靠query计数。无需重新研究fullInit或伪返回null。

交付 scope API/字段/reader优先级/动作表、Hook和恢复owner表及手工轨迹：无B TLS的19+14首次Join、重复query、+0x54 flush、导航/取消、同update第二pass、no-pass update、另K、native stored confirm、成功后再次confirm、retire遇publish pending。每步列控制scope/token、query返回、create call/绑定/发布/终态；不能只重复状态机而不说明 action19/14怎样实际返回。

### W03 — A 的实际 key→Manager→device query 与重选后置

明确HUD/controller slot链与gameplay输入链分别出表。9BAAA0返回slot编号/-1，90B010是它使用的context lookup；二者不直接返回 A 的 physical query receiver。实际输入依六消费者的 Player+0x1618→956110/9561E0/9562D0→A20FB0/+0x74 keyed callback→A20390 active pair→A20940/A209E0/A20AB0→device slots。复读实际关键 guard 和 receiver，不用 HUD mapping 代替此链。

给有界场景矩阵：A key非负（包括0）/key=-1；context空/非空；B own pair仍登记/已remove；A retained pair/重绑未完成。非负key的常规路径和-1 fallback分别说明；helper前置特殊分支若能绕过device query，记录准确guard和候选如何限定/处理，不追全部Mod/Lua。不能默默新增“A全部场景都是正K/context空”的用户假设；需要唯一有效A绑定时列为candidate的明确输入状态，未重绑前中性。

沿 native remove通知/913760 和95B310的实际 selected-device lookup、95B87F、95BC4D/95BCA0、95BBB6，说明B own remove是否导致A key改变、A source pair reset、selector重选或旧B fallback。两Popup callback为null可以继承；A正向重绑应列真正setter caller、K来源、Player选择及完成后置，不能写成“另一路可能会重绑”。受context条件控制的分支保留条件，不把当前live成员未知等同于静态未读。

给下一 poll→reselect→Join/独立Room→Main `[A]` update→consumer/query的限定表：哪些字段保持、哪些改变、何时A按唯一绑定重新Submit/Resolve、哪个receiver读A S2或neutral；B迟到查询不读A/旧B。当前单A全局owner source作为差异，A/B正常遍历只能换/恢复TLS，不能清另一个人的queue。保持A物理采样读原硬件值，防止合成S2回流为新命令。

交付真实query链、scene/guard/receiver表、reselect writer后置及有限剩余。已证明的条件性映射应标STATIC-DERIVED并说明条件；条件所需运行值放到现场验收，不为了补现场值开启运行工具。Main vector删除和下一轮只见A不重新归UNKNOWN。

### W04 — 合并候选、继承项与新交付勘误

三条链合并成一套接口/状态/owner责任：Session冻结identity，participant独立queue/sequence/replay/dedupe/planner/pending，共享Authority tick各Resolve至多一次；JoinControl独立pass输出；gameplay在consumer original内部query应用S2；TopUpdateExit处理normal retire及引用账本。给每个关键API最小参数、调用阶段、唯一writer、有效条件及终态，不仅列概念名。

继续沿单一候选Authority-pre写current/state或neutral、normal-post提交previous/关visible、+0x78仅提供native活跃poll条件；retire以generation/visible失效，不额外pre写previous。若pause/loading/binding generation变化后的previous需要逻辑中性或reset，准确说明它的tag检查/唯一owner及与normal-post的互斥，避免跨generation触发。与contract v2 §4 poll latch writer差异标DESIGN_ONLY/未采纳，不改contract；设计采纳和生产接入另记，不作为本轮读取资料的前提。

继承v15的raw/canonical去重、held V2、12个gameplay action和native cross-family表，不重新推完整press/release批次。只核对W02控制动作不混入V2，W03 A/binding重置不跨participant造edge，W01 retire不提交旧publication，重复query/+0x78不Resolve或推进S2。normal noRoom首轮可能已Submit但未Resolve，第二个missingTopUpdate按source阈值2处理pause；恢复只给新generation/epoch的结果，候选与当前source差异准确列明。

新证据的报告/设计链接从 `out/cs-t0` 使用 `../../docs/...`，报告到证据从本design-revision目录使用 `../../../../../../out/...`；人工核对，禁止链接检查脚本。不改v15旧链接。报告开头回答Q01–Q06及W01–W04目标满足/部分完成/排除方法/未读终点，纠正“仅剩一条context边”和“已内部一致”过强表述；不能因为artifact写完就宣布正常功能完整。

P01–P06仍按注册枚举、type/name/binding、Offline Join/Popup、B key/ordinal/退休、snapshot/query、pause/context/lifecycle六个功能主题，各列native已证/真正未读边、候选及未采纳差异、现场未观察、生产未实现、后续整体验收事项。Popup无callback是已证事实，新的JoinControl API不存在是实现事项，运行K/context未知是现场事项，全alias不是本轮剩余。按事实收敛有限静态任务，不重复把已证基础降回UNKNOWN。

## 5. 产物、分工与唯一整体验收

- 报告：`docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v16.md`。
- 证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v16.md`；必要分册同v16前缀并全部引用。
- topic README：新增tx-01-36实际结果，保留tx-01-34复核、本文和所有旧记录。

证据给对象、VA/RVA、实际raw区间、call/return、ECX/栈/返回通道、guard/writer/reader/后置及限度；IDA-DIRECT只限本轮实际读取，继承事实带来源，source当前文件/位置、CONTRACT、用户假设和DESIGN_ONLY区分。输出截断、cursor未到目标、错误调用保留终点；可用get_bytes和IDA Heads纯静态读取补足，不伪称全函数读完。

本文交付后启动一个新上下文 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行tx-01-36，不恢复已完成v15、不再委派、不创建侧栏聊天。主智能体交出v16文件、topic README编辑权与同一IDA只读会话使用权，执行期间不并发编辑/操作IDA。

仅允许静态IDB/file/source读取及v16新文档/README编辑；不改旧交付、工作设计、project conventions、source、contract/schema，不patch/rename/set_type/define_func/save IDB，不用debugger、进程/游戏内存、游戏输入、注入或安装Hook。pure py_eval只读指令/refs，不能运行状态模型。不得运行测试、构建、harness/probe/smoke/canary/verifier、hash/link-check脚本。PowerShell可用login:false。

tx-01-36全部分析与编辑结束后一次人工整体通读本文、v16报告/证据/分册、相关source/contract和README。统一审阅引用账本/入参所有权/无双减、JoinControl在无B TLS下的返回与pass寿命、真实A input链和reselect后置、三者与S2/lifecycle/writer/恢复owner一致性、继承边界、事实分类和引用。审阅前可修正文稿；唯一最终审阅后仅回填结果，失败列具体项，不自动修补、复审或换编号重跑。

分别报告文档准确性、正常功能静态完整程度、候选内部一致性及未采纳状态、现场未观察、生产未实现。不得预填PASS。结尾列基础可开始的实现、候选需变更的具体source/API、真正还需读取的有限行为；剩余项给函数/字段/分支/地址，不以“需要现场观察”代替静态任务。工具中断继续独立source/候选工作并提交准确已完成部分，不伪造读取。

## 6. tx-01-35 设计交付记录

本次新增本文并在topic README记录tx-01-34复核和新执行入口；本轮静态种子沿前次直接读取，当前IDA元数据已确认。未运行测试、构建、哈希校验或游戏操作，未修改旧报告/证据、source、contract/schema或IDB。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文与README修订范围，对照现行规范、v15复核、contract及相关source，核对Q/W对应、native pair入参/ReleaseStrong与账本责任、无B TLS的JoinControl接线和pass寿命、HUD与实际A input链分离、remove/reselect后置、S2/lifecycle/writer/恢复owner、继承项与事实分类、引用和授权范围。审阅后仅回填结果，无第二次审阅；不代表tx-01-36执行或正常功能已确认完整。
