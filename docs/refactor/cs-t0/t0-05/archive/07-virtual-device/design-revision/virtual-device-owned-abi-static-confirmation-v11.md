> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# 自有 ABI 正常功能静态确认 v11

推进：tx-01-21  
执行日期：2026-10-13  
状态：W01/W03/W04 已按现有静态材料完成；W02 已定位本地 B 解绑/重选路径，但关键分支后半在保存导出中截断，正常 B-only 退休仍未闭合。**正常功能静态完整性未完成。**

本轮实际工具列表没有 IDA Pro MCP 或 x32dbg。本报告没有新增 IDA-DIRECT 证据：保存反汇编只标 ARTIFACT-READ，旧报告结论标 STATIC-INHERITED，源码阅读标 SOURCE-DIRECT。仍能推进位图、PauseGapTracker、Resolve、Join 跳转和候选时序；不把工具缺失、生产尚未实现、候选待采纳或运行值未观察当作原生静态缺边。证据索引见[静态证据 v11](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v11.md)。

## 本轮完成范围

| 工作 | 结果 |
| --- | --- |
| W01 勘误与分类 | 已完成：更正四项旧报告事实，区分 edge/held；P01–P06 分开归类真实静态缺边、候选差异、现场值和实现工作。 |
| W02 本地 95B310 与 B 退休 | 部分完成：保存原始导出直读至 95B4AB，旧静态摘要定位 95B87F/95BCA0 的条件与 setter 目标；断线 UI 后续、实际 Player removal、A 是否暂停及 B-only 顺序无法从现有材料闭合。不能声称 B-only 正常退休已证。 |
| W03 S2 与 pause/recovery | 已完成候选分析：现行 Resolve/Authority/pause 行为按源码复读；给出 Authority-pre-only 发布候选及查询关闭点。它与 contract v2 的 poll 唯一 latch writer 不同，尚未采纳。 |
| W04 Join K/pass/导航 | 已完成有限控制流：保存指令 artifact 显示 action20/21 到公共 selector footer 后回环；列出 K 绑定与 update/pass 的 DESIGN_ONLY 命令候选。没有声称实现或通用 exactly-once。 |

范围继续限于正常唯一正 K、分配成功、主线程有序、Offline/local-co-op、A-only 起步。B 的可靠原生创建/发布仍是用户接受的 DESIGN_ONLY 前提。C/S 与原版及 Isaac Sync 隔离；A/B 只共用现有 scheduler、sequence、target tick、queue、Resolve/apply；不增加 B 即时旁路。不重做 full Player Init 或 manager-this 证明。

## v10 四项勘误与补充分类

| 项 | v10 错误及 v11 正确结论 | 来源与影响 |
| --- | --- | --- |
| Q01 device action 实参 | v10 把 device slot 收到的 action 写成指针。cdecl callback 收到 action 指针后解引用；device slot +0x30/+0x34/+0x38 收到 uint32 action 值。 | A20975 的 `FF 30` 是 `push dword ptr [eax]`；A20AE9 同类 seed 由先前复核者提供。对应细节仅沿用 v10 E02 / 本轮输入的 STATIC-INHERITED 证据，没有新 IDA-DIRECT。设备 ECX receiver 与 callback payload 分开。 |
| Q02 action8–11 位图 | 四项各有 edge bit：Bomb8→bit0、Item9→bit1、Card/Pill10→bit2、Drop11→bit3。held bit0 仍是 Shoot；当前 v1 没有 Item/Card-or-Pill/Drop held 位。 | SOURCE-DIRECT：input_core.hpp、active_runtime.cpp、active_apply.cpp。读者明细 STATIC-INHERITED v8 E03：Bomb 只需 triggered；Item/Card/Pill 与 Movement/Shooting/Drop 存在 pressed reader；Drop 计时器依赖持续 pressed。最小版本化候选给 action9–11 增 level 位，action8 保持 edge-only。 |
| Q03 连续无 Authority | 普通 Room guard skip 不会立刻清队列，但“所有无 Room 都不清”过宽：第一连续 TopUpdateExit miss 通常仅计数；未被 loading recovery 阻断时，第二次达到阈值2，进入 Pause 并 AdvanceOwnerGeneration，清 queue/planner/pending 与 adapter snapshot/previous。 | SOURCE-DIRECT：PauseGapTracker、Authority、TopUpdateExit、AdvanceOwnerGeneration、ClearForGeneration/ClearGeneration。Room lifecycle 与 loading enter/exit 各自可能先行清 generation。recovery 的豁免见 W03。 |
| Q04 Join 导航 | action20/21 true 不终止整个 selector scan。20 跳入公共导航/效果块；21 更新当前 selector 后到公共 footer；footer 处理下个 selector，行数仍有剩余时回到 9BB274。它们跳过当前 selector 之后的 confirm/cancel 路径，不退出整个 9BAC40 扫描。 | ARTIFACT-READ：native-join-admission-v2-combined-development-attempt-v2.json 的 RVA 0x005BB2DF→0x005BB334、0x005BB371→0x005BB567、0x005BB56E→0x005BB274 successor。不是本轮 IDA-DIRECT。 |
| Q05 本地退休线索 | 不能再说“没有本地 B 解绑入口”。历史反汇编摘要记录 95B87F 的 SetControllerId(-1,1) 与 95BCA0 的 SetControllerId(K,1)，但两者都有 offline/选择等 guard；调用 setter 本身不是 Player removal，也不证明只影响 B。 | 95B310 导出 ARTIFACT-READ 至 95B4AB；分支后半和两个 setter 站点为 STATIC-INHERITED v2 W09 `3.3 / v2 W0607 A-3。完整 A/B 结果仍缺，见 W02。 |
| Q06 查询有效期 | Authority-pre 发布候选必须在正常 post 关闭 current 查询授权；post 后 poll/Join/无 Room 均不能读取旧 edge。previous 只能作为下一有效 Authority 的比较基线，不能作为自有 query 的独立有效快照。 | DESIGN_ONLY 候选，按现行 active_apply/runtime 时序补足；尚未改 contract/schema/source。 |
| Q07 状态分类 | 原生已证的 Manager insert/keyed lookup、context factory/notification 和消费者链继续继承。没有 C/S registration 是生产实现缺项；active pair/context/selector 是运行值未观察；仅把 95B310 原始后半补读不到、B-only exit/removal caller 与其顺序仍是实际静态缺边。 | 来源标签见证据 v11 与 P01–P06。 |

## W01：ABI、位图和可复用静态链

ABI 只作有限更正，不重做既有链。继承结论包括 38 槽 device、六覆写、六槽 control、S2、NEVER；element+0x378 是嵌入 source/control pair，entry own key 与 source key 不同；Manager dispatcher 与 cdecl callback 分离；Main Player vtable address point B6BDD0，槽 +0x0C→782AF0→六消费者。以上均引用 v8/v10 证据，不伪称本轮二进制复读。

| action | 语义 | edge | 当前 held 能力 | 正常 reader 需要 |
| --- | --- | --- | --- | --- |
| 8 Bomb | 单次触发 | bit0 | 无 | triggered；不需要持续 held。 |
| 9 Item | 按下/触发 | bit1 | 无 | 持续 pressed 与 trigger 边沿均可见。 |
| 10 Card/Pill | 条件相关按下/触发 | bit2 | 无 | 满足对应条件时持续 pressed；trigger 独立。 |
| 11 Drop | 放下 | bit3 | 无 | Drop reader pressed 会延长/清理 Player timer；持续 held 必须可表达。 |

最小版本化方向（DESIGN_ONLY）：保留 held bit0=Shoot；在新输入版本增加 held bit1=Item9、bit2=Card/Pill10、bit3=Drop11；action8 只保留独立 edge bit0。action9–11 的 edge 保持 bit1–3。当前 v1 不接收新增 held 位，本轮未改 schema 或 canonicalizer。

## W02：95B310、本地 reselect 与退休顺序

当前能说的时间关系：STATIC-INHERITED v2 W06/W07 与 v10 E07 记录主更新 954CD0 在 native poll 954F7D 后，于 954F84 调 95B310，再进入有条件 Join。ARTIFACT-READ 的 xrefs3.txt 保存 95B310 开头到 95B4AB；该段可见 Gamepad RTTI/专属字段分支，以及调用断线 UI 的开端，但没有下半函数，更没有 95B87F/95BCA0 指令。

| 路径 | 已有 guard/目标 | 能断定的效果 | 不能推出的效果 |
| --- | --- | --- | --- |
| 95B310 入口与 Gamepad 分支 | 导出可见 [edi+0x4ABC8] 与 -1 比较；遍历 manager 返回的设备；95B3C6 动态转型，cast null 跳到 95B417；Gamepad 再检查 +8 bit0x20、+0x384==3、绑定 vector 边界及 +0xE0/+0xE4 selector 状态。 | Gamepad 专属检查有类型门。旧证据还记载每个枚举设备先调用 +0x1C/+0x20 generic slots。 | 无法从只到 95B4AB 的 raw 导出重建后半 guard、所有 branch join 或 Base-derived device 的完整回落路径。 |
| lookup miss→unassign，95B87F | STATIC-INHERITED：v2 W09 `3.3 记录仅在 `n60!=0`、offline roster begin=end，且断连 prompt 的 device resolution 失败 (`v20==0/v25==0`) 下进入；先以 `sub_417870(n60,v91)` 取 Player，再调用 `SetControllerId(player,-1,1)`。 | 目标 Player 的 controller key 被置 -1；propagate=1 会触及 setter 定义的关联对象。不是只因任意 lookup miss 就无条件解绑。 | 目标是否必为 B ordinal=1、此前 UI 是否已从 Player vector 移除谁、该帧是否暂停 A、setter 后 B 是否还会被查询，均无当前完整 caller/状态链。 |
| lookup/reselect 成功，95BCA0 | STATIC-INHERITED：后续设备扫描选中 key≠-1，offline roster begin=end；目标是 main Player vector 的 `*v49`，然后 `SetControllerId(key,1)`。其后清四个 selector/assigned 字段并调 A21CB0；旧证据把 manager refresh 语义标 UNKNOWN。 | 本地 reselect 可重新赋 key；不是设备 pair 删除。 | 不能证明选择者一定为 B、之后是否改变 whole-run/UI，也不能由 refresh 名字推出通知/context 状态。 |
| disconnect UI / Player removal | ARTIFACT-READ 95B44F 调 sub_9C25E0；STATIC-INHERITED v2 W0607 A-3 记录后续 sub_9C2DB0 与“可能的 Player removal”。 | 95B310 中确有 UI 分支开端。 | 导出在 95B4AB 截断；未见 9C25E0/9C2DB0 完整参数、callee/caller、Player vector 修改指令，也没证明 UI 是否暂停 A。 |

两种次序都未能被现有证据证明为完整安全的 B-only retirement：

1. **先 B-specific exit/unbind，再 remove**：可用第一步是 RETIRING：关闭 admission/Join/publication、owner generation++、自有查询 neutral；Manager 登记期间 poll 保持非零，NEVER 对象/control/DLL 常驻。之后必须有一个已证实的正常 B-only Player exit/remove，确认 B 不再以 K 查询，再走显式 Manager remove/通知。现有 95B87F setter(-1,1) 还可能让仍在 Player vector 的对象进入 key=-1 fallback；它是否在下一查询前移除 B 未证。
2. **先 remove pair，再由 95B310 处理 K miss**：A20EB0 会删除 Manager pair并通知，但不改 Player key；poll 返回 0 也会被 native manager 自动 erase/通知。B 可能暂时仍持 K；之后 95B310 是否命中所述断线 UI/unassign、是否要求额外 selector/reselect，以及 -1 后是否走 all-active lookup 读 A，均未闭合。故不能把“先 remove”说成可达且 B-only。

key=-1 的风险是条件性的：在 context vector 空时 helper 可落到 A20FB0 的 all-active-device 查询，可能读到 A；context 非空时会遍历 NetInputDevice context 项。Online roster 空不证明独立 context vector 空。仅 selector cancel、仅设 -1、仅 A20EB0 remove、或永久 neutral 但不退休登记，都不是已完成的 B-only 退休。

全 run 的 RunTeardown 在源码中会 generation clear、retire adapter/dispatcher 并禁用 override；这不证明自有 Manager pair 已移除或 Player key/context 已清理。NEVER 对象按设计保持常驻。**B 独立退休：未证。whole-run teardown 的 scheduler 关闭：源码已证；自有 pair/key清理：未证。**

## W03：S2 当前行为、候选发布与状态轨迹

SOURCE-DIRECT 的当前顺序：PhysicalExit 在成功 native poll、pause/loading/owner guard 后 Submit；EdgeTargetPlanner 以 completed tick+2 为 edge target；SubmitEncoded 先 Send/Drain/Decode，再 Accept；Authority pre 为 candidate=last_completed+1 执行一次 dispatcher Resolve 并调用 BeginAuthority；只有 native Room 正常 post 才 EndAuthority(true) 并提交 completed tick。Resolve 消耗等于 tick 的命令、丢弃旧命令、保留 future；无输入仍推进 last_resolved_tick，同一或较小 tick 不会重复 resolve。

现有 SyntheticAdapter 不是自有设备 S2。resolved_state=null 时 BeginAuthority 不建立 snapshot；normal End 若既无 snapshot 又无 future pending，会保留旧 previous。只在 Authority scope 内且 main-thread、自然 consumer、唯一 Player/device、generation/family 相符时 Evaluate 覆写；其它 query 路径先经过 original。因而不能把源码里的 null/no input/no Room 当作已实现的 neutral publication。

| 候选身份/状态 | owner 与有效期 | 关键约束 |
| --- | --- | --- |
| session/apply/owner generation | 当前 session；owner/apply generation；切换或 lifecycle clear 时改变。 | 任何快照、命令、query witness 都必须完全匹配；generation 变化清 queue/planner/pending与snapshot/previous。 |
| update_epoch | UpdateHook 主线程 entry 每次+1；normal UpdateExit 结束。 | 外层 update 次数独立于 Authority target tick；用来让未进入 Join pass 的命令按 update 过期。 |
| publication_epoch | 成功进入合资格 Authority pre，完成一次 Resolve 并发布 immutable state 或显式 neutral 时+1。 | poll 不写 S2、不推进此 epoch；同一发布内 query 重复只读。 |
| candidate/target_tick | Authority pre 为 last_completed+1；仅 normal StandardRoom post 将其提交为 completed。 | 不以 update_epoch 替代；同 target 不能再次 Resolve。 |
| current / previous / authority scope | pre 建 current 和有效 scope；main-thread 主 Player 对正确 K/consumer 查询；normal post 先提交 previous=current，再关闭 scope、使 current invalid。 | post 后自有 query neutral；previous 仅供下一有效 Authority 的 edge 比较，不直接查询。 |
| Join pass | 6FADA7→9BAC40 前 Begin，正常返回 6FADAC Complete。 | 独立于 S2 与 target_tick；不得让 Join query 消费 gameplay queue。 |

候选推荐 Authority-pre-only，而不是让 poll 预 Resolve：Resolve 是 destructive；poll 可重复且不等于 Authority，提前解析会在普通 Room skip 时消耗命令、推进 last_resolved_tick，实际 Authority 同 tick随后无法重算。候选在每个有效 Authority tick发布一次（无输入也发布 neutral），native poll 只维持非零；normal post 提交 tick/previous并关闭 query scope。native C4←C0 buffering 与自有 S2 分开。

需要修改 contract/schema 才能采用：把 poll 的“唯一 latch writer”改为只返回登记状态；定义 Authority pre publish 与 normal post commit/close 的 owner API；增加 publication_epoch 与 current_valid/authority_scope（区分 update_epoch 与 target_tick）；定义无输入 neutral marker、generation 失效和自有 query 的全字段有效条件。现有 contract v2 没有 reserve/commit/rollback；若坚持 poll 唯一 writer，则至少还需 non-destructive staging/reserve、Authority commit/rollback、重复 tick拒绝、room skip取消和 generation 失效机制。本轮不静默改 contract。

当前源码时序与候选影响：

| 事件 | tick/queue/planner/pending | current/previous 与 query | 关闭点/注记 |
| --- | --- | --- | --- |
| press→hold→release→repress | 每个有效物理 edge 按 lead=2 规划；同 edge target 严格递增。 | 版本化候选 press=(held=1,edge=1)：pressed+triggered；hold=(1,0)：pressed；release=(0,0)：均否；repress=(1,1)：两者恢复。 | query 在一个 publication 内只读；重复真值不消费 edge。action8 仅 edge。 |
| 同一 Update 多次 poll/query | 当前 dispatcher 只对相同 completed tick/source 的 identical level-only 样本去重；edge与状态变化仍提交。future/queue 不因重复 S2 查询改变。 | Authority scope 内多次 query 返回同一 current/previous；poll 后的 Join query 不延长该 scope。 | S2 publish owner 不是 poll。 |
| normal post 后再次 query | completed tick 已提交；没有新 Resolve。 | current invalid，self query neutral；previous 仅留作下次有效 Authority 的比较基线，edge 不复现。 | StandardRoom normal post 是候选 query close。 |
| 第一次普通无 Room | candidate 不进入、不 Resolve、不提交；queue/planner/pending 和 completed tick 原样留存。 | 不存在新有效 scope；对 scope 外的自有槽返回 neutral，旧 current 不可读。 | 无独立 Room lifecycle callback 的前提下只增加 pause gap count。 |
| 第二次连续普通无 Room | threshold=2 触发 PauseEnter/AdvanceOwnerGeneration，清 transport/queue/planner/pending/dedupe、adapter snapshot/previous及 authority scratch；completed tick 单调值保留。 | generation 变化使所有旧 witness/query neutral。 | PhysicalExit pause gate 禁止提交。 |
| loading recovery | LoadingEnter/Exit 已各自 generation clear；LoadingExit 设置 awaiting-authority。 | recovery 未完成前输入提交被 gate；无 Room TopUpdateExit miss 不累加 pause threshold；首个 Authority 清 recovery并重置 gap。 | 本路没有“第二缺失再 PauseEnter”的步骤；生命周期 clear 已先发生。 |
| 无输入但正常 Authority | 现行 Resolve 会前进 last_resolved_tick；source adapter收 null。 | 现行 adapter 可能保留 previous；候选必须发布显式 neutral 并在 post 提交 neutral baseline。 | 不把 null 等同 neutral publication。 |
| Pause退出后的首个 Authority | AuthorityObservedWithLoadingRecovery 先处理 PauseExit 并 generation clear，再以新 generation 做 Resolve。 | 新 scope 从 neutral baseline 开始，不沿用 pause 前 edge。 | 普通 Pause 保留 native 登记，不用 poll=0。 |

PauseGapTracker、LoadingRecovery 的实际实现属 SOURCE-DIRECT；独立 Room lifecycle Hook 可单独 generation clear，因此“无 Authority”表仅指没有同时触发这些 lifecycle signals 的普通 skip。

尚未证实的 S2 边：原版 954CD0 poll/Join 与 C/S StandardRoom/Player consumer 的全局调用先后没有在当前材料中完整闭合；自有生产注册也未实现，所以没有真实自有设备 query 可观测。候选一致性不等于运行行为。

## W04：K 绑定、pass 生命周期与 Join 控制流

ARTIFACT-READ 的 Join 指令 artifact 在 action20 true 处从 RVA 0x005BB2DF 跳 0x005BB334；action21 true 分支已在 0x005BB371 更新当前 selector，再跳 0x005BB567。0x005BB567 推进 selector 指针/剩余行数；0x005BB56E 若还有 selector 则回到 0x005BB274。故两种导航都离开当前 selector 的剩余 confirm/cancel 路径，但随后还会扫描下一个 selector。cancel action15 只清当前 selector index，不清 Player key、Manager pair或 context。

JoinCommand 是控制面事件，不是 B gameplay InputCmd。候选最小字段：

`JoinCommand { session_id, generation, target_device_key=K, sequence,
 target_update_epoch, join_pass_id, action_mask, admitted, bound_to_pass,
 completed/discarded }`

UpdateHook entry 建立 update_epoch，并在原生 update/pass 之前给准入的控制命令一次可读窗口；6FADA7→9BAC40 的首次 pass Bind 当前 join_pass_id。只有查询设备 key=K、session/generation/epoch/pass 全匹配时才能看到 action mask；相同 pass 的重复查询只读，native normal return 到 6FADAC 时 owner Complete/consume；本 update 没有 pass 时在 UpdateExit 关闭。第二 pass、新 update、generation 改变或 RETIRING 均不复用旧命令。候选没有为任意 selector side effect 提供 exactly-once 屏障。

在正常前提下，action19 可在 action14 同一 pass 先建/复用 K selector再确认；成功创建/SetControllerId(K,1) 后 selector 失活且 Main Player 已有 K，后续 action19 的 key guard 抑制同 K 重建。该抑制来自原生唯一 K 轨迹，不代表多 key selector 都只执行一次。Player full construction/reliable publish 仍是 DESIGN_ONLY。

第一位 B 尚不存在时，JoinCommand 不能用 B Player ordinal或 gameplay Authority target tick准入：先按 K 在独立 control lane接收，并绑定当前 update/首个有效 Join pass；成功创建、身份可读后，B gameplay input 才按 A 相同 scheduler、sequence、lead、queue、Authority Resolve/apply 进入。首个 B poll 相对当前 Join pass 的具体发生时点没有本轮静态证明；不得用 B 即时旁路弥补，也不得把 update_epoch当 target_tick。

| 正常轨迹 | 结果 |
| --- | --- |
| 19+14 同一 pass、目标 K、guard 成立 | K selector 后确认；后续 K key guard抑制 duplicate create；B create/publish仍是 DESIGN_ONLY。 |
| action20/21 | 导航到公共块后继续下一 selector；当前 row 的后续 confirm/cancel 被跳过。 |
| 仅 action15 | 当前 selector inactive；Player key、Manager pair、context不变。 |
| 同 pass 重复 query | 返回只读的同一命令状态；owner在 normal Complete消费，不在 query消费。 |
| 同 update第二 pass | 已绑定/完成的命令不复用；其余 selectors仍受各自原生 guard影响。 |
| 本 update 无 pass | UpdateExit关闭 JoinCommand，不等待停滞 Authority tick。 |
| 另一 key 已有 selector | K-bound command对另一 key不可见；其它已有 selector可能独立有原生副作用。 |
| RETIRING 与待执行命令相交 | generation变更、关闭 admission并丢弃命令；S2同时变 neutral，poll登记期间仍非零。 |

这个 lane、pass witness和明确 expiry 均为 DESIGN_ONLY；没有更改 source 或 schema。

## P01–P06：状态分栏

| 链 | 原生静态已证 | 实际原生静态缺边 | 候选/contract差异 | 现场值未观察 | 生产未实现 |
| --- | --- | --- | --- | --- | --- |
| P01 注册/枚举 | STATIC-INHERITED v8/v10 E03–E05：context element ctor/insert、Manager keyed lookup、own key/source pair 分离、notification reader/reset、remove各对象关系。 | 95B310 miss/UI后是否触发 context row/Player erase没有后续指令；自有 Base通知能否命中真实关联依赖具体 caller/值。 | S2 writer迁移需要 contract变化；不把它当 native registration事实。 | Manager pair、context/listener成员和K。 | C/S own Init/registration/insert/key allocation。 |
| P02 type/name/binding | CONTRACT 38槽/六override/六control；STATIC-INHERITED v2 W09对Base-vs-Gamepad RTTI和设备槽；Player→782AF0六消费者。 | 95B310 Gamepad cast null后的完整generic路径与后置行为未从截断导出重建；它是否影响owned Base需补具体call/branch。 | 自有Base COL和固定ABI沿现合同； held v2需版本化。 | 实际vptr/COL/name/binding和native对象关联。 | 自有ABI构造、base ctor后vptr写入、bindings/name/Init。 |
| P03 Offline Join | ARTIFACT-READ 导航回环；STATIC-INHERITED W09 selector 19/20/21/14/15、empty-context/唯一K guards。 | 原生无跨query/update的JoinCommand/pass expiry；任意其它 selector副作用不受通用one-shot保护。 | 独立 K-bound JoinCommand/pass/update_epoch。 | context/roster/selector/key/Player count。 | Join lane、pass lifecycle、可靠B创建发布。 |
| P04 key/B ordinal | STATIC-INHERITED W09 SetControllerId、Player key reader、95B87F/95BCA0 callers摘要；A20EB0不写Player key。 | 95B310 target与B ordinal、Player vector removal、setter后下一query及remove/notification次序未闭合。 | RETIRING期间保留K、避免-1 fallback；需B-specific exit证据。 | 当前B ordinal/key、linked Player/NPC/context状态。 | 自有K绑定与B-only正常退出/退休。 |
| P05 snapshot/query | SOURCE-DIRECT queue/dispatcher/Authority/adapter；STATIC-INHERITED v10 E07/E11 query与六consumer链。 | native update poll/Join相对所有C/S Authority/consumer的完整先后仍未证明；没有自有device实读链。 | Authority-pre publish/normal-post close与contract v2 poll唯一writer不一致；held v2候选未采纳。 | publication/update epoch、具体查询数和值。 | 自有S2发布/查询、query slot、schema/contract更新。 |
| P06 pause/退休/context | SOURCE-DIRECT threshold=2、generation clear、loading recovery；STATIC-INHERITED W08/W09 native remove、poll-zero erase、selector cancel。 | 95B310 disconnect UI→Player removal→A/B影响→Manager/context remove后置关系未证，不能完成B独立退休。 | pause neutral但登记保留；RETIRING关闭admission并neutral；NEVER常驻。 | pause/reselect是否发生、UI状态、实际listeners/cache。 | 可重复B-only退休与whole-run owned pair/key清理。 |

## 可据已证 ABI 开始的工作与未决边

可按已证 ABI 开始不接入原版/Online的固定 layout、Base ctor后vptr构造方案和静态 action 位转换模块；开始产品级 Manager registration、S2或Join lane之前，须先采纳并落地与contract v2兼容或正式修订后的职责/API。B-only retirement 不能仅按目前资料实现成声称安全完成的路径。

仍需补读的静态边是具体且有限的：恢复 95B310 的 95B4AB之后直到 95B87F/95BCA0及函数尾的原始指令/decompile；完整跟入 9C25E0/9C2DB0 与任何实际 Player removal caller，确认 Main Player vector变更、UI对A的作用和后续B查询；将 setter目标关联到 ordinal 1；确认这一路径与 A20EB0 notification/context cleanup的顺序；补齐 native update poll/Join与 C/S StandardRoom consumer之间需要的调用先后。保存现有导出无法证明这些边。

## 本轮唯一整体人工静态文档审阅

结果：**未通过（文档准确性/完整性）**。本轮唯一整体人工静态通读覆盖工作设计 v10、v11报告/证据、关键 source/contract 与 README。发现报告 Q05/W02、证据 E05 的 `v2 W09` / `3.3` inline-code 标记未闭合；另报告 W03“第一次普通无 Room 时 queue/planner/pending 原样留存”过宽，成功 PhysicalExit 可以在同一 top update 的 Authority miss 被确认前 SubmitPhysical，并改变这些状态。按约定只记录结果，没有修补或重审。

五项状态分别记录：文档准确性/完整性未通过；正常功能静态完整性未完成（95B310后续、B-only exit/removal和时序仍缺）；Authority-pre S2 与 JoinCommand 候选内部一致但未采纳且与现contract v2有差异；当前key、vector、listener、selector、epoch和query值未观察；自有 ABI注册、S2、Join lane及B退休未生产实现。

