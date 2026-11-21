> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v12

日期：2026-10-13  
设计交付推进：`tx-01-26`  
静态执行推进：`tx-01-27`（W01–W04 是同一完整目标的内容编号）  
状态：`DESIGN_ONLY / 承接 v12 只读复核；确认报告 v13 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文依据 `tx-01-25` 对[报告 v12](virtual-device-owned-abi-static-confirmation-v12.md)、[证据 v12](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v12.md)及[执行设计 v11](virtual-device-owned-abi-static-confirmation-next-work-design-v11.md)的只读复核。v12 已读完 95B310，但正常功能静态确认仍未完整，且存在 Popup 身份、witness 接入和重复 poll 的实质问题。保留 v12 原文及其唯一审阅失败记录；本轮新增证据和候选接入说明，交付 v13，不修补或重审旧报告。

## 1. 完整目标、范围与结果要求

本轮完成四项工作：纠正 Popup ID 与设备 K 的混淆并追完其对 A 后续更新的实际条件；有界定位 B 正常解绑或退出及 context/Manager 清理次序；把单 A witness/source 与自有 B S2 的接入差异写成可执行的候选接口和事件顺序；核实正常重复 poll 的提交行为及 Join 创建尝试的实际接入窗口。输出逐条已证、推导、候选和真实缺边，明确哪些实现可以开始、哪些行为仍依赖未证的具体入口。

正常前提明确为：分配成功、对象和缓冲有效、主线程顺序操作、Offline/local-co-op、**Online roster 为空**、唯一正设备 key K、创建 B 前 Main Player vector 恰有 A。Online roster 为空、n298+0x4B3D8 context vector 为空、Main Player vector 的人数和成员是三个独立条件；不能相互代替。原生 Join 能可靠创建/发布 B 继续沿用[用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的 `DESIGN_ONLY` 前提，不重做 full Player Init 或全局 manager-this，不升级成运行证明。

继承 38 槽 device、六覆写槽、六槽 control、S2、NEVER；首次原生 base ctor 前固定 DLL，ctor 后写自有 vptr，BASE_CONSTRUCTED 后不析构或释放对象/control/有效基类状态，恢复后 DLL 常驻。每 session 至多一次自有设备创建，失败不自动重试；B 创建尝试 owner 另列，不能混淆两者。A ordinal0、B ordinal1，ClientId→ordinal 在 session 内冻结；指针/K/平台 handle 仅作 generation 内关联。A/B 使用相同 canonical、sequence、target tick、queue、dedupe、Resolve/apply 策略，B 没有即时旁路。可复用通用 InputManager；C/S 不依赖 Online Manager、NetInputDevice 或 FrameStore。

本轮关注正常功能路径，保留线程、生命周期和 Hook 恢复约束。正常 pause/loading/noRoom、重复 poll/query/pass、B 退休及 A 后续读者属于范围；不展开 OOM/EH、worker 并发、计数溢出、ABA、全映像 alias、持久化、异常恢复或 05-06 全网络证明。

`tx-01-26` 在设计及 README 完成后只做一次人工整体静态文档审阅；`tx-01-27` 在全部分析和 v13 产物完成后只做一次人工整体静态文档审阅。过程中零测试，不设 gate、不校验任何哈希，不运行构建或游戏。W 编号不是额外验收次数。

## 2. v12 复核问题与继承边界

| 项 | 当前问题 | 本轮交付 |
| --- | --- | --- |
| Q01 Popup 身份误读 | v12 把 9C39E0 返回值、n298+0x4ABCC 称选择结果/候选 key。tx-01-25 已见 Popup ctor 首 dword 是递增 Popup ID，随后以该 ID 查 Popup 列表。 | W01 给 Popup ID、实际 K、局部候选索引、Player ordinal、Popup state/result 的各自 writer/reader；纠正新报告，不覆盖旧文。 |
| Q02 UI→A 后置未闭合 | v12 已读 UI，但仍将 A 继续/暂停整体列为未知；954CD0 的 Join guard 不能代替 Room guard。 | W01 追实际 UI update、flags/state、Room/Player guard，独立于 B erase 判定 A 是否继续及在哪些条件下等待。 |
| Q03 B 退休入口未证 | SetControllerId(-1)、菜单转换、Manager pair remove 均不等于 Main Player B exit；-1 fallback 可能读 A。 | W02 从 Main Player vector writer/erase 或真正解绑入口查正常 caller，给可用次序或具体不满足原因。 |
| Q04 单 A witness 与 B 冲突 | ReadUniquePlayer 要求 vector 恰有一个 Player、ordinal0；现有 Player switch 会推进 owner generation。v12 却要求 B ordinal1 使用“现有六类 reader”。 | W03 给 A/B 身份和 witness 生命周期、每人快照路由、正常 A↔B 不清共享输入的候选接入方案。不能只放宽人数判断。 |
| Q05 重复 poll 结论过宽 | 证据 v12 E10 称同 source 重复 poll 不产生第二条 command；SubmitPhysical 仅抑制同 completed tick/source、无 edge 且持续状态相同的输入。 | W04 按 native poll→raw→canonical→SubmitPhysical 查正常 edge 是否会重现，区分采样、提交、命令去重和只读 query。 |
| Q06 Join 尝试窗口不具体 | v12 标出 9BB415/9BB424，但没有完整给开始/成功关闭的接入点；9BB41C 取 selector K，不是读取新 Player key。 | W04 给 receiver/参数来源、interception/observer 责任和正常后置确认时点；不把 null 返回候选写成已安全处理的原生路径。 |
| Q07 分类与小勘误 | P01/P02/P05 把生产未实现或现场未观察写成 native 静态缺边；keyed call 实为 95B6B3，95B6B6 读结果。v12 还遗漏 Online roster 前提、写成 DESGIN_ONLY。 | 全文纠正分类和拼写；raw 区间起点与 instruction/call VA 分列，保留旧审阅失败。 |

继承有效事实：Base/Control 布局与 ABI、source pair 与 entry own key 分层、callback receiver、scalar 返回约定、context factory/通知清理、native insert/indexed/keyed lookup/remove、poll-zero 自动移除、Player vtable→六消费者、95B310 完整 generic fallthrough 和 menu/context/Main Player 对象区别、带 guard 的 poll→reselect→Join→Room→Player 链。无需再逐项重做 38/6 表或重新读取 747 条指令以证明函数“读完”；只复读本轮所需分支与新 caller。

schema v1 的 Shoot held bit0 和 action8–11 edge bits0–3 继承；9–11 level 版本化、Authority-pre publisher、JoinCommand/pass/attempt owner 均是未采纳候选。现行[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)仍写 poll 唯一 latch writer；本轮只列差异，不修改 contract/schema/source。

## 3. 工具、定位种子与资料入口

tx-01-26 设计准备实际调用 survey_binary(minimal)、server_health：IDA 服务正常、Hex-Rays 和 auto analysis ready。输入为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，IDB 为同路径 `isaac-ng.exe.i64`，32 位，imagebase=0x400000，image size=0x8FE000。仅读取必要元数据；未输出、比较或校验哈希值。

下表包含 tx-01-25 主智能体直接读取所得定位种子和当前 source 位置，**不是 tx-01-27 执行者的新 IDA-DIRECT**。新报告若继承，应标来源及推导限度；涉及关键对象/ABI/guard/接入点应复读 raw。先发现实际工具及 schema，再调用；decompile 截断和 disasm 分页须补完相关分支。纯静态 py_eval 只读 IDB，不能修改数据库或执行状态模型。

| 种子 | 已见事实或问题 | 下一读取方向 |
| --- | --- | --- |
| 9C01D0；9C020E/9C0214/9C022E | Popup ctor 从 dword_C72A24 取 ID、存首 dword、递增全局计数。 | 与 9C39E0 返回及列表比较连起来；此 ID 不是 device key。 |
| 9C39E0；9C3A2E、9C3A8B/9C3A8E | 构造 Popup、插入列表，返回对象首 dword。 | 核对 receiver/list/返回地址；构造和返回 ID 不等于用户已做选择。 |
| 95B4C0、95B806/95B811、95BAC2/95BACC、95BBB6 | 两处 Popup ID 写入 n298+4ABC8/+4ABCC；后半按 ID 查 Popup，再清 ID。 | 分开两个 Popup 的建构、查询、callback/result、关闭和跨 update 寿命。 |
| 9C3AB0、9C3990、9C34F0、9C1260、9C07F0 | Popup ID 查询/关闭、列表 flag 检查、UI 更新、状态转换/计时。 | 参数以 raw 为准；追实际影响 Room 的 flag writer/reader，不按函数名推暂停。 |
| 954F7D/954F84/954F8F、9551BD、955231 | native poll→reselect→UI update；state2 下 9C3990/9505E0 guard Join；Room 另有 guard。 | 区分 Join 受阻、Room 受阻、A Player consumer 受阻；查实际 Popup flag 到相应 guard 的边。 |
| 95B6B3/95B6B6 | 前者是 FF 50 28 keyed call，后者 mov esi,[eax] 读返回 pair。 | 按实际 call VA 和 byte-region 起点重写证据，不混用准备/返回地址。 |
| n60+1BAA8/+1BAAC、9B9CD0、7A6450、Player vtable B6BDD0 | Main Player vector、创建/插入、setter、Player 虚分派已定位。 | 从真实 vector end/begin writer、erase/disposal 和正常 local-co-op caller 找 B 解绑/退出，而非只搜 setter xrefs。 |
| A20EB0、A1FC00、A212C0、913760、906020、90A8A0、A21F40 | Manager remove、context notify/cleanup 及已读 remove callers。 | 与真正 B 处理入口接次序；批量 teardown 不自动证明 B-only。 |
| active_runtime.cpp:653、685、699、923、1036、1394；active_witness.cpp:29/51 | 单 Player 身份、ordinal0 eligibility、owner switch、consumer TLS。 | 形成具体 A/B 接入差异表及候选 API/责任，不直接复用单 A 路径。 |
| active_dispatcher.cpp:265/300；input_core.cpp:531 | level-only 同 tick/source 去重；edge planner 可推迟 target 并生成新 sequence。 | 与 BuildRawSample/PhysicalExit/native current/previous writer 连起来，判断正常重复采样是否重现 edge。 |
| 9BB415、9BB41C、9BB424、6FADA7/6FADAC | create receiver 是 Main Player vector；9BB41C 取 selector K；7A6450 绑定后 Join 返回。 | 给开始/成功关闭的实际窗口及调用约束；区分设备创建与 B 创建 owner。 |

主要资料：[报告 v10](virtual-device-owned-abi-static-confirmation-v10.md)、v12 报告/证据、执行设计 v11、[v8 六消费者证据](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)、[v2 W06/W07](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w0607-draft.md)、[v2 W09](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)、contract v2。源码包括 [active_runtime.cpp](../../../../../../../../cs-t0/native/src/active_runtime.cpp)、[active_witness.cpp](../../../../../../../../cs-t0/native/src/active_witness.cpp)、[active_dispatcher.cpp](../../../../../../../../cs-t0/native/src/active_dispatcher.cpp)、[input_core.cpp](../../../../../../../../cs-t0/native/src/input_core.cpp)、active_apply 和 loop_runtime。阅读源码中的旧 hash/测试入口仅作为历史实现资料，不执行或要求该流程。

## 4. 执行工作

### W01：Popup 对象身份、正常 UI 寿命与 A 后续更新

先复读 Popup ctor、9C39E0 和 95B310 关键 raw，形成身份表：字段位置、值域/来源、writer、reader、比较对象、清除时点。分别列出 +4ABC8 初始 warning Popup ID、+4ABCC disconnect Popup ID、device+0C 的 K、候选 vector 下标 v91、Main Player ordinal、Popup state、callback 参数/结果。不用“当前选择值”同时代表多个量；`9C39E0 返回 ID` 与用户随后选择结果分开。

有限追两个 Popup 的 create→UI update→状态/flag变化→callback/result→close，核对 9C3AB0 receiver/参数、9C3990 所检查的 Popup+36 bit2 及其实际 writer。tx-01-25 的 ctor 片段存在后续 descriptor/normalize 写入，不能只凭初始 flags 宣称 Popup 全程阻断 A。95B310 内根据 Popup ID 找到对象后读 state/计时/调用 callback 的分支，必须与实际选中 device K 的来源分开。

沿 954CD0 及实际 Room/Player callee 输出带条件的 A 更新链：Popup 存在但未完成、完成/取消、断线后一更新、K 重绑后、设备 remove 后，各自 Join、Room、A Player/consumer 是否到达。明确哪个条件阻止哪个阶段；若只有 Join 受阻，不写成整个 run 暂停。若某 UI flag 只在 Room callee 判定，则补该边。此工作不等待 W02 找到 B vector erase；已有 UI→A 链可独立确认。

交付身份表、有限状态/调用链、各正常场景的 A 后置及具体未证边。同时在新材料纠正 call VA、raw region 标签、Online roster 前提、DESIGN_ONLY 拼写和来源分类。旧失败原文保留。

### W02：B 正常解绑/退出、A 输入与 native 清理次序

从 Main Player vector 的真实 writer/erase/disposal 查起，沿创建/人数变化与正常 local-co-op 生命周期筛选 B-specific 入口。可以采用对已定位函数的 xrefs、指令/字段访问静态查询和原生 erase callee 反向 caller；仅有 setter 的 31 个 xref 或 Manager remove 的三个 caller 不足以判定找不到 Player exit。先列相关 writer 候选并选择与正常 B 处理最相关的有限路径，优先深读不超过八条 caller/callee 路径；仍有候选未读则列出其 VA、相关性及具体缺边，不声称全映像不存在退出。

每个可用入口记录实际 receiver、参数、Player 身份选择、vector begin/end变化、关联/子 Player传播、是否只作用 B、运行阶段和调用后下一 reader。区分：B Player erase/退出、保留 Player 的真正解绑、device key=-1、context key清理、Manager pair移除、菜单/selector转换、whole-run teardown。

退休目标依现行规范是 **B 解绑或退出**，不把 vector erase 改成唯一允许方案。若方案保留 B 在 Main vector，必须证明其后正常 consumer 不经 -1 all-active fallback 读 A、不读取旧 B publication、不再产生 B 输入副作用，且 A 的绑定/ordinal 不变；单独 setter(-1) 与自有槽 RETIRING 返回中性不能证明这一点，因为 B 可能已经查询别的设备。无法证明则该方案不满足功能目标。若方案退出 B，则实际 erase/member移除和后续正常遍历是必要证据。

形成正常次序表：RETIRING 首步关闭 admission/Join/current publication并失效 generation、查询中性；随后 B-specific 解绑或退出；context/listener/selector 处理；最后 native Manager remove及下一 poll/reselect/Room/consumer。根据实际原生入口决定可调用阶段及顺序，不预设 setter、erase、notify 的内部顺序。比较先处理 B 后 remove与先 remove后靠95B310的结果，写明每一步之后 K lookup、B member/key、A输入、UI状态和可能fallback。登记期间 poll 非零、Pause保留登记并中性、NEVER常驻继续成立。

分别给 B-only 退休且 A 继续、whole-run teardown 的结论；若 B-only 原生入口没有闭合，给准确已读终点、不可用依据、尚需读取的入口或需要新增的明确实现行为。不得借用 Online 全 Player reset、裸写字段或发明 sentinel 来凑退休路径，也不得把永久中性且仍登记称完成退休。

交付一条有据可用的正常序列，或逐步骤说明为何当前候选不能完成目标及剩余工作。负面结论可以完成某条候选评估，不能标成完整正常功能已证。

### W03：A/B witness、每人快照与 S2 查询的具体接入设计

复读 ReadUniquePlayer/CurrentOwnerStable/CurrentPlayerBootstrapStable/Eligibility、ActivePlayerUpdateHook/InvokeConsumer、active_witness TLS 和 publication/apply 路径。现行 vector==1/ordinal0 和单 g_current_player/authority command 是单 A 实现；正常 A→B 切换如果触发 AdvanceOwnerGeneration/ClearForGeneration，会使同 tick 其它参与者输入失效。列真实函数/字段、当前作用和候选修改责任，不只把 count1 改成 count2。

候选接口至少明确：session 冻结的 ClientId→ordinal；在受限 Main Player vector 中定位唯一 A/B 并建立 generation 内 Player*/K关联；A physical source 与 B own-device source的读取边界；区分 session/apply/binding generation和临时 Player/consumer scope；每 ordinal 的 resolved command/current/previous/publication；Authority pre对所有有效参与者依相同scheduler规则Resolve并发布，normal post统一commit/close；Enter/LeavePlayer与Enter/LeaveConsumer的TLS保存/恢复；自有槽读取当前 B witness和对应publication。可以新增候选API名称，但必须标 DESIGN_ONLY，不冒充已有函数或已安装Hook。

列 candidate API 的输入、返回、调用者/阶段、字段writer、失败中性行为和原始 ABI 保持方式。正常 A↔B遍历仅切换临时scope，不推进session/apply/binding generation，不清共享queue/planner/pending，不把A命令用于B；真正重绑、room/lifecycle/retire才按对应事件失效。各身份字段明确比较哪一种generation，避免用“generation都匹配”掩盖责任。真实 native Player/consumer调用已证，不等于现有单A wrapper在有B时能建立witness。

继承v12跨类别许可表并核对其证据：Movement→0–3/11，Shooting→4–7/0–3/11，Bomb→8，Item→9/10/11，Card/Pill→10/9/11，Drop→11。自有query同时约束实际B Player、K、Authority、publication和consumer/action；query只读，scope外/normal post后/无Authority/retire为中性。不能让physical原函数fallback承担未证的B输入。

Authority-pre publication候选须写清 current/previous、显式neutral、post close和无输入释放held；poll不得另推进S2或第二次Resolve。列出与contract v2 poll唯一writer的具体差异；若采用保留pollwriter的其它方案，必须另给不破坏destructive Resolve的stage/reserve/commit责任。本轮只推荐，不修改或宣称采纳契约。held bit1/2/3用于9/10/11仍是schema版本候选，与continuous axes/edge区分。

纸面推导至少覆盖：同一Authority A后B和B后A；跨类别重复query；同update重复自有poll；无输入Authority；first noRoom的先Submit后无Resolve；second普通miss在TopExit清理；loading recovery；pause恢复首Authority；B Join前无B witness、成功发布后进入统一scheduler；B退休后A下一次更新。表列每人关联、generation、tick/epoch、queue/pending、current/previous、scope与关闭事件。不得执行模型或用手推代替未证native边。

交付明确 source接入差异表、候选接口/时序、正常轨迹及契约差异。只有相关内部问题已解才标候选一致；缺API或身份恢复规则不能写“现有reader已支持B”。

### W04：正常重复 poll 与 Join 创建尝试的实际窗口

先读 SubmitPhysical完整条件、SamePersistentState、EdgeTargetPlanner、BuildRawSample及PhysicalExit调用顺序；按需要有限复读native current/previous/trigger writer，判断第二次正常poll能否再次产生同edge。区分五件事：native硬件poll采样、自有device poll、canonical构造/sequence分配、command接收去重、同publication重复query。现行同completed tick/source且edge_flags==0、持续状态相同的level-only抑制不能外推到带edge或状态改变的输入。

给三类纸面轨迹：同tick同source相同level无edge；同tick连续样本具有edge；同tick状态改变。带edge轨迹分别说明“API可接收”与“正常native采样可到达”：若第二poll使previous/current推进而edge消失，须给writer/caller依据；若会重复edge，说明现有source为何产生第二sequence/target和哪个候选责任需要抑制同一物理事件。若正常可达性仍未证，列具体缺边，不凭Submit函数容许输入就声称已发现运行反例。保持跨tick新的press/repress可提交，禁止靠一概去重吞掉正常变化。

有限核对Join create路径：9BB415的ECX为Main Player vector，selected type作参数；9BB41C来自selector K；9BB424给new Player绑定K，native片段中create返回与setter之间没有null guard。可靠正常创建/发布依用户前提；null/OOM/EH不属于本轮正常前提，不能把原生继续call描述成已安全失败处理，也不为此展开异常恢复。

给B-create attempt owner的具体接入方案：哪个wrapper/interception在首次实际create前标AttemptStarted，如何保持original exactly-once；成功确认必须位于7A6450绑定及用户规划发布后置之后，由谁读取唯一B ordinal1/K并终结owner；6FADAC normal pass结束是JoinCommand关闭点，是否足够用于确认成功须说明。9BB415是原生指令位置，不是已经存在的可直接调用candidate API；若需新增Hook或callee wrapper，列实际Hook对象/ABI、最小安装范围与单协调owner/恢复事务差异，标未实现，不在本轮修改字节。

设备一次创建owner、B一次实际create attempt owner、JoinCommand admission/open/navigation、pass绑定/到期各自分列。guard未过或未抵达create不是实际尝试；到达create后失败终结仅作为候选要求，不混入正常原生已证。重复query、同update第二pass、19+14同pass、导航/取消、另一K、无pass Exit、retire关闭需与W03 scope/epoch一致，不把update epoch当Authority tick。

交付精确去重条件/可达性表、正常重复采样轨迹、Join receiver/K来源和attempt接入/终结方案。只读query不等于任意原生副作用恰好一次，本轮不展开05-06网络证明。

## 5. v13 交付和状态分类

- 报告：`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v13.md`。
- 证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v13.md`；必要分册同v13前缀，报告引用全部分册。
- 更新topic README的tx-01-27实际结果入口，保留tx-01-24/v12失败、tx-01-25复核及本设计。

证据记录对象、VA/RVA、raw区间与实际instruction/call地址、caller/ECX/参数/返回、guard、writer/reader、正常后置及限度。IDA-DIRECT仅用于本轮实际成功读取；SOURCE-DIRECT附当前source位置；ARTIFACT-READ附导出路径和终点；STATIC-INHERITED附旧报告/节；CONTRACT、用户假设和DESIGN_ONLY分开。只有xrefs、读取失败或相关分页未完成不能算整条行为已证。

报告开头逐项回答Q01–Q07和W01–W04的具体要求。W状态至少区分“目标满足”“部分完成”“已证该候选不满足、目标仍未完成”；不以读过函数或撰写材料替代目标完成。P01–P06按以下五类分列，不交叉充数：

| 类别 | 正确内容示例 |
| --- | --- |
| 原生静态已证/真正缺边 | insert/lookup ABI已证；B正常解绑/退出caller或Popup flag→Room guard若未读则是实际缺边。 |
| 候选设计与待采纳差异 | A/B witness接口、Authority-pre publisher、held版本、attempt Hook方案及其契约差异。 |
| 现场值未观察 | 当次K/Player*/ordinal/vector/Popup/listener/vptr/COL及有效绑定值。 |
| 生产实现未完成 | own Init/register、A/B路由/S2/Join lane/retire代码尚未编写；不因此把native ABI列UNKNOWN。 |
| 后续整体运行验收事项 | 后续实现目标的一次整体验收应观察的正常行为；本轮只列，不运行。 |

结尾按实际依赖给后续实现清单：固定layout/构造/vtable/control/已证登记可独立编写；S2与Join接入依具体候选接口/契约决定；完整B退休依可用正常解绑或退出及cleanup次序。已有基础可开始不代表完整目标已证；也不以所有候选未采纳一概阻止基础编写。剩余静态项必须具体到对象/caller/字段/条件，说明本轮新证据减少了哪些问题。

## 6. 执行所有权与唯一整体验收

设计交付后以新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行tx-01-27，不恢复已完成的v12实例，不再派子智能体或创建侧栏聊天。执行者先读现行规范、项目技能/任务资料及本文，再按实际工具schema读取。主智能体交出v13文件、topic README编辑权及同一IDA只读会话使用权，期间不并发编辑或操作IDA。

只允许静态文件/source/IDB读取和v13文档/README编辑。不得修改旧报告、生产source、contract/schema，不patch/rename/set_type/define_func/save IDB，不访问游戏进程、调试附着/断点/单步、游戏输入、注入或安装Hook。不得运行测试、构建、harness/probe/smoke/canary/verifier、hash/链接检查脚本或可执行状态模型。PowerShell可用login:false。IDA服务可用只代表静态读取能力，不扩大实机授权。

tx-01-27全部分析和编辑结束后，仅一次人工整体通读本文、v13报告/证据/分册、相关source/contract及README修订。统一核对Popup身份/条件/A结果、B退休实际下一reader、A/B scope和generation、每人publication、重复poll可达性、Join接入时点、原始ABI、分类和引用。过程中可正常纠正文稿；最终审阅后只回填结果，失败记录具体项，不自动修补或重审，不换编号重复同一验收。

分别报告文档准确性、正常功能静态程度、候选内部一致性/采纳、现场未观察和生产未实现。不得预填PASS，不以UNKNOWN标签完整替代功能完成。工具中断时保留实际终点和可用产物，继续不依赖工具的工作，再如实报告具体未完成项。

## 7. tx-01-26 设计交付记录

本次新增本文并更新topic README。已确认IDA只读服务/数据库必要元数据；工作定位沿tx-01-25复核和当前source，未把继承种子列成本轮子智能体的新证据。未运行测试、构建、哈希校验或游戏操作，未修改生产source、contract/schema、旧报告或IDB。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文和README修订范围，对照现行规范、contract v2、v12复核、已读source及IDA元数据，检查Q/W对应、Popup ID/K区分、UI→A与B退出的独立分析、解绑或退出的真实下一reader、A/B witness和generation接入、重复poll去重条件/正常可达性、Join receiver/尝试窗口、分类及引用。审阅后仅回填结果，无第二次审阅；此结果不表示tx-01-27执行、v13结果或正常功能静态完整性已完成。
