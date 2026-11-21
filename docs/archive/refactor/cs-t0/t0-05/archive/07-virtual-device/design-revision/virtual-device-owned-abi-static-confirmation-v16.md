> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v16

日期：2026-10-16  
推进：`tx-01-36`（续接；W01–W04 属于同一完整目标）  
入口设计：[续接工作设计 v16](virtual-device-owned-abi-static-confirmation-next-work-design-v16.md)；[完整工作设计 v15](virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)  
状态：**tx-01-36 静态交付已完成；唯一整体审阅通过（文档准确性与声明边界一致性）。**

本文件和[静态证据 v16](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v16.md)是原 `tx-01-36` 的续接交付，不是新执行编号。此前执行实例中断且没有已落盘的 v16 结果；本稿只记录本替代执行者实际已读和正在补齐的内容，不推断旧实例的读取进度。报告范围仍按工作设计 v15 的 Q01–Q06、W01–W04、正常前提和排除项执行。

## 当前进度

已完整阅读现行项目规范、项目技能、架构计划、T0 任务计划、`cs-t0/README.md`、topic README、v16/v15 工作设计、v15 报告/证据和 coordinator contract v2。IDA MCP 本轮 survey/health 已确认 32-bit `isaac-ng.exe`，base `0x400000`、image size `0x8FE000`，auto-analysis 与 Hex-Rays ready；随后直接复读 W01 函数及 W02 `6FA540/9BAC40` 相关路径。tx-01-37 的 `A20EB0` 结论仅作初始定位种子，本轮使用自身 IDA-DIRECT 函数体/raw 复读补证。

| 主题 | 状态 | 尚需完成 |
| --- | --- | --- |
| W01 引用与退休 | **静态路径已闭合** | 已将正常 `906020` caller 的 lookup-local、按值参数、两通知 temporary、移位 temporary、vector strong 和 root/R 分项对账；candidate 只记录 native 已消费 vector responsibility。现场 pair absence 与 source 接入仍未实现。 |
| W02 首次 JoinControl | **静态路径已闭合** | 已补双 action19分支、14/15/20/21、+0x54 native flush/stored-confirm、无 B TLS轨迹、重读/no-pass/重复pass/导航取消/attempt及终态。生产query lane与Join owner尚未实现，现场flags/K/命令值未观察。 |
| W03 A 实际 input 链 | **有限正常链已直接确认** | 已读三种 Player key query/fallback、HUD/context分支、remove callback与95B310正反向setter；`n298[2]==2`模式值、`v91`现场归属、context/成员实值与callback运行时dispatch仍未观察。 |
| W04 合并与分类 | **候选完整；未采纳** | 已对照当前A-only source、queue/Resolve、pause/loading、lifecycle、contract writer；候选与contract v2 §4的`+0x78` sole-writer存在明确冲突，未修改contract/source。 |

## 范围、前提与 Q01–Q06 回答

静态推演范围沿工作设计 v15 §1 和续接设计 v16 §3：有效分配、对象与缓冲，主线程顺序操作，Offline/local-co-op，Online roster 为空，B 使用唯一正 K，创建 B 前 Main Player vector 仅有 A。Online roster、NetInputDevice/context vector、Main Player vector 与 InputManager pair vector 分开判断；A0/B1 与 ClientId→ordinal 在 session 内冻结。用户接受的 native Join 可靠创建并发布 B 是 `USER-ASSUMPTION / DESIGN_ONLY`，只作为正常前提，不证明控制命令已接线。正常 pause/loading/noRoom、重复 poll/query/pass、首次 Join、B 退休和 A 下一查询在范围内；OOM/EH、worker 并发、溢出/ABA、全映像 alias、持久化、异常恢复、全部 Mod/Lua 行为和完整网络证明排除。

| 问题 | 回答与状态 |
| --- | --- |
| Q01：A20EB0 的 ReleaseStrong 在哪里？ | **已确认**：尾 pair vector control `+0x0C` 与 by-value argument control `+0x0C` 都在可选 `n79_0` 之前直接发生；W01/E01 区分其责任。 |
| Q02：正常 caller 如何取得/消耗 pair 与移位引用？ | **限定调用链已闭合**：906020→A20390→902D60→A20EB0 的 lookup-local、by-value、两通知 temporary、shift temporary、vector 和 root/R 已分别记账；B tail/middle 均只消费一份 Manager vector strong。 |
| Q03：B 尚未创建且没有 gameplay TLS 时，19/14 如何返回？ | **候选路径已具体化，尚未实现**：JoinControl scope 在 9BAC40 前开启，优先于 gameplay S2；查询不消费 lane、不 Resolve，允许的控制动作和 no-create/attempt 轨迹见 W02/E02。 |
| Q04：9BAAA0 是否返回物理 query receiver？ | **否**：它给 HUD/controller slot 或 `-1`；实际 Player query 从 `Player+0x1618` 经 956110/9561E0/9562D0、Manager callback 到 device receiver，见 W03/E03。 |
| Q05：remove/reselect 后 A 在何条件下继续查询？ | **条件路径已确认**：95BCA0 可对选中 Player 写正 K；954B40 另有 `n298[2]==2` 条件 setter。A 身份、mode值、现场 K/context 与 callback dispatch仍未知，重绑完成需 exact identity/key 后置。 |
| Q06：证据与合并候选的剩余状态？ | **新引用已修正，候选未采纳**：v16报告和证据互链层级正确；authority-pre/normal-post writer候选与 contract v2 §4 `+0x78` sole-writer冲突，保持 `DESIGN_ONLY`，未修改 contract/source/schema。 |

因此，本轮限定的引用、无 B TLS 的 JoinControl 接线推演和 A 实际 reader/reselect 静态链均有具体证据，但正常功能整体仍**部分完成**：有限 mode/identity/context/现场条件未观测，候选未采纳，自有 ABI、Join lane、双 participant S2 与 B retirement 尚未生产实现。

后续每完成一个主题，就同步更新本文与证据文件。所有结论区分 `IDA-DIRECT`、`SOURCE-DIRECT`、`STATIC-INHERITED`、`CONTRACT`、`USER-ASSUMPTION` 与 `DESIGN_ONLY`。过程不运行测试、构建、harness、probe、状态模型、校验或游戏/进程操作。

## Q01–Q06 与 W01–W04

### Q01 / W01 — 先由 native 释放，再由通知/移位各自结清

本轮 IDA 直接读取闭合了 `906020 → A20390 → 902D60 → A20EB0` 的正常 pair 所有权路径。`902D60` 用 source pair control 的 `+0x04` TryAddStrong 成功后才复制两个 dword；`A20390` 将 vector pair 复制到 caller 的 lookup-local；`906020` 又复制出传给 `A20EB0` 的独立 by-value pair。`A20EB0` 先用两份独立复制调用 Manager `+0x7C/+0x80` 通知，再删除 active pair；Manager 自身随后在 `+0x0C` 调用中消费 vector strong，尾部也单独消费 by-value strong。`906020` 返回前再消费 lookup-local strong。每个副本有自己的 acquire/release，不能以同一个 `ReleaseClass(ManagerVectorStrong)`代替。

`A21570` 每次移动下一 pair 前先用 `902D60` 取得 shift temporary，再与被覆盖的 pair 交换；被覆盖 pair 的 `+0x0C` ReleaseStrong 随后执行。因此 B 位于尾部时由 `A20EB0` 的末项 release 消费 B vector strong；B 在中部时由第一次 shift temporary 的 release 消费 B vector strong，`A20EB0` 的末项 release 消费移位前的尾项副本。两条路径对 B 都恰减一次，下一 reader 中其他 member 的 vector strong 保持一份。第一条 Manager 通知地址由当前 global vptr/slot 静态核对为 `A212C0`（`+0x7C`），第二条为 `A213B0`（`+0x80`）；二者的 by-value pair 在各自 callee 内释放。`A211D0` 是 `+0x78`，不是这两条 remove 通知。

因此新 bridge 必须把“真正调用 control `+0x0C`”和“记账 native 已消费 ManagerVectorStrong”拆成不同责任。`A20EB0` 后对 object+control exact pair 复扫缺席，coordinator 才只更新私有账本并进入 `UNREGISTERED`；不得再经旧 `ReleaseClass` 调 `port.ReleaseStrong`。先关闭 B admission/visible 并使 generation 失效，再通知/移除 B device pair；确认缺席后标记 `UNREGISTERED`，再执行普通 B 的 Main cleanup 和 relation/context/ordinal 后置，最后走 A 的下一 reader。已缺席或已退休请求不得再发一次 remove/消费同一 vector ref。对象/control/root strong/有效 base state 与 pinned DLL 继续由 `NEVER` 保留。

现有离线 lifecycle source 的 `ReleaseClass` 会实际调用 `port.ReleaseStrong`，`RemoveAndDestroy` 在 Manager absence 后先释放 `ManagerVectorStrong`、置 `UNREGISTERED`，之后继续 `FinalRelease(FactoryLocalStrong)` 到 `Destroyed`。这与本轮 native 已经消费 vector ref、以及 `NEVER` 的常驻 root/对象策略都不兼容；只能复用状态意图，不能照搬其 release 调用。

### Q02 / W02 — B 尚不存在时的 JoinControl Reader

`6FA540`在Room/Join guards之后于`6FADA7`调用`9BAC40`；normal return address是`6FADAC`。`9BAC40`在Main Player中还没有B时会经Manager `+0x74`查询B receiver/K的Triggered action19，查询成功后才走`9BAAA0`的HUD/selector slot映射。因此候选scope必须在9BAC40开始前建立，不可等待PlayerUpdate/consumer TLS或action19已经成功。scope key至少固定`session, ClientIdB, owned receiver, K, binding generation, updateEpoch, passToken, commandSeq, visible action mask`，在`6FADAC`关闭；updateEpoch不是Authority tick。

reader优先级为：精确匹配且有效的JoinControl scope先读独立控制lane；否则匹配当前participant、generation、native consumer/action的gameplay S2；无匹配、退休或stale identity neutral；A的物理receiver仍保留A路径。action19 admission、14 confirm、15 cancel、20/21 navigation只按native Triggered边沿读取，Pressed查询返回固定held，Scalar固定0；同pass的重复query稳定返回同值，不consume、不延长scope、不Resolve。8/10不能从0–11 gameplay state映射到B JoinControl；`9BAF63–9BAFA4`已证8+10组合可走另一条`9B9CD0(...,-1)` creator，故unsupported action在Join scope内中性。

原生`+0x54` writeback是`A1EE10` inherited action-state更新（按四个primitive binding调用`+0x50`后复制 `this[48][action]→this[49][action]`），不是Join lane drain/sequence acknowledge；契约规定的6个自有对象override不含`+0x54`，不加第七槽。native还可把19变成stored-confirm并绕过一次14 query，所以attempt只能由精确`9BB415` callsite owner单次admit；9BB424只是K绑定、不是成功后置。deny跳过call、补`push selectedType`的4字节并到`9BB561→9BB567` no-create continuation，不得伪造null返回让无guard setter继续。B的可靠create/publish是用户接受前提，成功/失败/退休终态候选仍为`USER-ASSUMPTION / DESIGN_ONLY`。

E02已逐轨迹记录action19 true/false、normal/stored confirm、14/15、20/21、重复read、`+0x54`flush、同update第二pass、no-pass update、other K、actual attempt之后pending publish/retire。candidate只保护所读`9BB415`普通main-vector callsite，不把它泛化成全局`9B9CD0` admission；callee另有possessor caller。

### Q03–Q04 / W03 — HUD slot与A真实查询分离

`9BAAA0`返回的是HUD/controller slot或`-1`；`90B010`返回NetInputDevice/context记录。实际Player query以`Player+0x1618`为K，经过`956110` Pressed、`9561E0` Triggered或`9562D0` Scalar，进入Manager keyed callback `A20940/A209E0/A20AB0`及device receiver `+0x30/+0x34/+0x38`；K=0是普通非负key。A20FB0的非负key走Manager `+0x28` keyed lookup，K=-1则锁定遍历active pair vector，所以当B还在枚举域时不能假设A的`-1` fallback只看A。

每个helper的关键fallback不同：956110在K=-1且native physical vector非空时枚举Pressed；9562D0对Scalar也枚举非零receiver；9561E0仅在K=-1、vector非空且final flag=0时枚举Triggered，否则仍走Manager callback。另保留三个helper的特殊前置分支：所读normal reader之前有`n298[2]==2 && dword_C71690 && 864C30/864D80`谓词控制的handler路径；该状态不当成false，也不把尚未跟踪的特殊handler输出纳入owned S2。候选必须只覆盖身份和guard均精确匹配的普通receiver读；不唯一时neutral，synthetic reader结果不得回流成物理Submit。

### Q05 / W03 — Remove/reselect如何影响A

`A20EB0`的exact B pair remove不改A `Player+0x1618`。`913760`在`906D60`作为disconnect callback登记；已读函数在NetInputDevice secondary/context vector移除成员，或经`902F10(pair,0)`清source-pair association，不直接写Main Player key或替代InputManager primary erase。仅有callback registration xref，没证明B native remove实际dispatch此callback。

Main poll在`954F7D`后于`954F84`进`95B310`。context-empty分支在`95B87F`以Main Player index `v91`调用`7A6450(Player,-1,1)`令被选Player key失效；后续`95B977`枚举pair并以`device+0x0C`取得K，`95BCA0`在context-empty路径以同一被选Player和K调`7A6450`，是真正正向Player-key setter。仅当`v91==0`且身份仍是A时可写为A重绑完成。context-nonempty路径在`95B87F/95BC4D/95BBB6`调用`90AF30(context,K)`更新NetInputDevice/source关联，不直接写`Player+0x1618`；不能把这个context分支描述成A Player重新绑定。

本轮又直接复读`954B40`全函数/128条指令与xref：`954C2C`仅在`n298[2]==2`时读取Main PlayerManager vector[0]，并在`954C6C`以活动pair device `+0x0C`为K调用`7A6450`；6个caller包含`6FA540@6FAAB3`及`954CD0@954E89`。这是已定位的额外setter，不再列“可能存在setter”；但当前未确定`n298[2]==2`是否/何时适用于Offline/local-co-op目标，也未观察实际值，故保留为有限mode条件而不外推为A重绑必达。

key/context/pair场景、special guard、disconnect callback限度及下一poll→A query条件表见证据E03。Main vector删B后下一轮只见A、TopUpdateExit在完整原生update返回之后属已继承事实，不重列为待静态化。

### Q06 / W04 — 合并候选、当前source与六个功能主题

**统一候选（DESIGN_ONLY / NOT ADOPTED）**：session owner冻结`ClientId→ordinal`；A/B各自持有generation、binding `(device,control,K)`、sequence、canonical/replay/dedupe history、planner、queue与pending；B的JoinControl lane独立于gameplay state和Resolve。Authority owner对每个participant的eligible queue在candidate tick各Resolve至多一次，不等待另一participant输入；same Authority tick只产生一次世界apply。ParticipantReader在原生consumer的实际query处匹配identity/action/consumer/generation，允许原生跨consumer-family查询表，不按当前source的严格`ConsumerForAction`拒绝合法native调用；Query只读，original恰调一次。具体最小API、owner、writer与阶段见E04。

继承的native consumer/action许可表保持：Movement `0–3/11`；Shooting `4–7/0–3/11`；Bomb `8`；Item `9/10/11`；Pill/Card `10/9/11`；Drop `11`。V1 raw→canonical、完成tick+source相同且edge=0/persistent state相同的重复poll在sequence分配前去重；不跨tick去重。`DESIGN_ONLY` held V2为24-byte/version2/held mask`0x0F`（Shoot、Item9、Pill/Card10、Drop11），edge mask`0x0F`仍对应8–11，Bomb8 edge-only；需一起更改codec/queue/planner/adapter，当前source和schema仍是V1。重Query不Resolve，不消费edge。

source是单A owner：`active_runtime.cpp`只有一组current Player/device/key、一个`ActiveDispatcher`和一个`SyntheticAdapter`；`ReadUniquePlayer`要求Main Player vector恰有一个成员；`InputQueue`绑定一个`client_id_`，一个Dispatcher持一套sequence/planner/pending；`AdvanceOwnerGeneration`会清该唯一queue、S2和history。它不能直接承载A/B两条identity；B-only retire若照调全局clear会误删A状态。`active_apply.cpp::EndAuthority(true)`当前更新previous；contract v2 §4却指定对象`+0x78`唯一做current←published、previous←current。候选保留Authority-pre/current、normal-post/previous、`+0x78`只代表native poll-active条件，标`DESIGN_ONLY / 未采纳`，不修改contract。

Normal B retirement candidate在TopUpdateExit单线程排序为：关闭B admission/visible并使B generation过期 → 调`A20EB0`通知并exact pair remove → 对`(device,control)`复扫确认缺席 → coordinator只`RecordVectorRefConsumedByNative`并到`UNREGISTERED`（绝不再调`ReleaseStrong`）→ 精确检查B是否仍在Main Player vector；仍在时以已证`9BA000`普通erase ABI恰清B的Player、context/relation和ordinal rows，已缺席则不重复erase → 回到下一Main reader只见A并只在A的唯一key/pair rebind后恢复A Submit/Resolve。任何无法唯一匹配/未确认的状态都保持neutral且保留NEVER对象/control/root/base/DLL。`913760`不能被作为`A20EB0`必经listener假设。

pause/loading/noRoom方面，现有source只在main thread、精确poll-return且owner/K稳定时由真实keyboard/gamepad读取并Submit；第一次missing top update不pause，第二个连续缺失才pause并通过generation clear；loading recovery单独等待Authority再恢复。noRoom poll可在没有本轮Authority Resolve时先提交command，故候选必须区分Submit和Resolve，不能把每次poll直接当tick。source现有room/loading generation清除面是全局A-only，候选需改成participant-scoped失效，B retire只neutralize B并保留A。

P01–P06按native静态已证、有限静态条件、DESIGN_ONLY/未采纳、现场未观察、production未实现分别如下：

| 主题 | 本轮可确认与有限剩余 | 候选/现场/生产状态 | 后续具体责任 |
| --- | --- | --- | --- |
| P01 Register/enumerate | pair/K keyed reader及Main member cleanup已证；现场context row/key→`90B010`命中/返回receiver未观察。 | frozen ordinal与双participant registry为候选；自有ABI注册及双身份reader未接入生产。 | 有效A绑定出现后记录`90B010(context,K)`返回与`9BAAA0` slot映射；不把UI slot当physical receiver。 |
| P02 Type/name/binding | `7A6450`是真实Player key writer；`95BCA0`只对`context empty && v91==A`正向重绑；`954B40`另有`n298[2]==2`条件writer。 | 38槽/六覆写/control与NEVER属于contract；现场K/vptr/generation及`n298[2]`值未知；自有vptr/K绑定未实现。 | 确认Offline/local-co-op的`n298[2]`取值、`v91`对应身份和写后`Player+0x1618`，然后接入exact key binding。 |
| P03 Offline Join/Popup | no-B-TLS的action19查询入口、14/15/20/21、两个Popup空回调与no-create栈continuation已证。 | JoinControl/pass/attempt owner为候选；selector/query/命令/发布为运行值；lane与Admission未实现。 | 实现前按`6FADA1→9BAC40→9BB064/9BB39A/9BB3C9→9BB415/9BB561`接入pass owner与deny continuation；发布后置仍按用户接受前提。 |
| P04 Key/B retirement | Main B erase、`A20EB0` pair remove/临时引用、`913760`局部行为、TopUpdateExit后置已证；callback dispatch和post-remove exact pair absence未观察。 | exact `(device,control)` absence后记录native vector ref已消费；不能再走`ReleaseClass`，也不调用`FinalRelease`；retire coordinator未生产实现。 | B退役时在TopUpdateExit确认exact pair缺席；检查`913760`是否经secondary/context入口实际dispatch，再条件清理；Main B如仍存在才用`9BA000`一次移除。 |
| P05 Snapshot/input S2 | query ABI、A/B实际reader分支、native consumer original/query顺序、action表与cross-family map为已知静态事实。 | participant S2/V2 held及Authority writer候选未采纳；现场每条query命中对象未知；双participant queue/reader未实现。 | 先解决contract §4 `+0x78` sole-writer冲突；随后在六consumer和三query-kind接入participant reader/V2 codec，按ClientId各自Resolve一次。 |
| P06 Pause/cache/lifecycle | source的Main thread gates、noRoom submit/Authority分离、threshold2 pause、loading quarantine、single owner restore入口已读。 | participant generation/visible和NEVER lifetime接合为候选；现场时序、Hook target restore与native vector membership未观察；B lifecycle/Hook恢复接入未实现。 | 接入后覆盖pause前后、loading recovery、无Room poll/Authority间隔和B-only retire；确认Hook target restore、回调排空及DLL常驻。 |

### 静态完整程度与有限未决

W01、W02正常路径和W03 actual reader/reselect函数关系已按本轮限定范围完成静态读取；这不等于全部native状态都已运行确认，也不表示生产功能已完整。尚未收敛的静态条件是`954B40`的`n298[2]==2`与Offline/local-co-op语义对应；special handler从`956110/9561E0/9562D0`前置guard进入`864C30/864D80`后的完整目标未展开，owned query不覆盖该分支。实际`v91`是否为A、`90B010`现场命中对象、native disconnect callback在B retire时是否dispatch、exact pair是否移除，属于现场未知/实现后置，不冒充native缺边。

候选内部已经给出一致的participant身份/命令/reader/retire责任，但并未采纳：尤其Authority-pre/current和normal-post/previous与contract v2 §4 `+0x78` sole-writer相冲突，进入生产实现前必须先解决并采纳 writer 版本。本轮未改contract、source或schema，也未预填审阅通过。

## 唯一整体审阅

tx-01-36 唯一一次人工整体静态文档通读：**通过（文档准确性与声明边界一致性）**。审阅覆盖续接设计 v16、完整工作设计 v15、本报告与静态证据 v16、coordinator contract v2、相关 active runtime/input core/dispatcher/apply/lifecycle/loop source 及本 topic README。引用账本没有把native已消费的vector strong再次交给旧`ReleaseClass`；JoinControl在无B Player/consumer TLS的action19查询前开scope，且query与attempt副作用分离；A的HUD/context路径与真实Player query、key setter和有限mode条件分开；candidate、contract writer差异、现场未观察及production未实现均各自标明。文档准确性与声明范围一致。正常功能静态完整度仍为**部分完成**；Authority-pre/normal-post writer候选仍与contract v2 §4的`+0x78` sole-writer冲突，未采纳。审阅后只回填本结果，没有修补或复审；未运行测试、构建、哈希/链接检查或游戏/进程操作。
