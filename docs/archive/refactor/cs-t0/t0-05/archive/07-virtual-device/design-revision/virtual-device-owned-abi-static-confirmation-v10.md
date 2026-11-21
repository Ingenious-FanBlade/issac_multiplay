> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能静态确认 v10

日期：2026-10-13  
推进：tx-01-18  
依据：[执行设计 v9](virtual-device-owned-abi-static-confirmation-next-work-design-v9.md)、[现行项目规范](../../../../../project-conventions.md)  
证据：[静态证据 v10](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v10.md)

## 结论与范围

本轮纠正了 element+0x378 的 pair 身份及 callback receiver，追到了已观察的 lookup/context 列表创建、写入、读取和通知清理路径；把 Player vtable address point、构造时 vptr 写入、两种 Room 内虚分派及六个输入消费者连接起来；读完当前输入队列、dispatcher、Authority、Room hook 与 Join pass 路径。Q01、Q02、Q04、Q06、Q07 的既有缺项已得到实质补充。W01、W03、W05 的有限静态链可闭合；W02 的已观察 writer/guard 已闭合到静态元素类型，但没有做全映像 alias 闭包；W04 的 B 专属正常退出/解绑仍未找到；W06 的普通 selector 副作用与重复查询风险已推导。正常唯一 K 创建后，Player key 与 selector index guard 阻止再次创建；native query 本身不消费 command，也没有一般化的 per-command exactly-once 屏障。

**正常功能静态完整性：未完成。** 仍有明确静态功能缺口：现有 C/S source 没有自有 device 注册到原生 keyed Manager 的实现；没有 B 专属的 normal Player exit/unbind 顺序；也没有现行 contract 可直接容纳独立 Join pass 命令及所需快照 writer。B 的创建/发布只沿用原生 Join DESIGN_ONLY 假设。未知现场 roster、selector、context vector、key、listener 地址和值不能替代上述静态缺边。

五类结果分开判定：

| 维度 | 本轮结论 |
| --- | --- |
| 文档准确性 | 唯一整体人工静态文档审阅通过（文档准确性与所声明范围的一致性）；不代表功能链完整或生产实现完成。 |
| 正常功能静态完整性 | 未完成。B 专属正常退出/解绑及生产 keyed registration 边未闭合；其余已确认链见 W01–W06。 |
| 候选一致性与采纳 | 推荐 Authority pre 为唯一解析和快照发布 owner，并用独立 update epoch 关闭未进入 pass 的 Join 命令；该候选内部一致但改变 contract v2 的 poll 唯一写者语义，属于 DESIGN_ONLY、尚未采纳。 |
| 现场未观察 | 当前 Online roster、n298+0x4B3D8 成员、监听者集合、key/Player/selector 数量、目标进程行为与运行 tick 未观察。静态 writer/guard 不因这些运行值未知而记作 UNKNOWN。 |
| 生产未实现 | 自有设备 ABI 初始化/Manager registration、JoinCommand/pass/admission lane、B 专属 retirement、held 8–11 的版本化事件语义及自有查询接入尚未在生产源码完成。 |

范围限于正常路径。继续按 38 槽 device、六覆写槽、六槽 control、S2、NEVER、base ctor 前固定 DLL、base ctor 后写自有 vptr、构造后不释放、DLL 常驻、每 session 至多创建一次处理。A/B 共用 scheduler、sequence、target tick、queue、Resolve/Apply；没有 B 即时旁路。C/S 不依赖 Online Manager、NetInputDevice 或 FrameStore。保留正常 pause、Room 被 guard 跳过、重复 poll/query/pass 的功能影响。本轮不展开 OOM、EH/unwind、worker 并发、原子镜像、ABA、全映像 alias、持久化、异常恢复或 05-06 全网络证明。

## Q01–Q07 完成情况

| 项 | 结果 |
| --- | --- |
| Q01 pair 对象身份 | 已纠正。entry+0x378 是内嵌 pair 的地址；pair[0] 是 source device，pair[1] 是 control reference；source key 在 pair[0]+0x0C。entry own key 是另一字段 entry+0x0C。 |
| Q02 callback receiver | 已纠正。A20FB0 是 Manager this dispatcher；A20940/A209E0/A20AB0 是 cdecl callbacks；只有各 device query slot 以 device 为 ECX。原始参数与返回通道优先于伪代码的错误 callback 类型。 |
| Q03 list factory/清理/Offline | 已追到可见的 6F6500 记录处理分支、NetInputDevice-derived element 的 901930/9015B0 ctor、905F60 Manager insert 和 context vector 插入，以及 913760 通知清理。Offline 分支检查 context begin==end；Online roster 为空本身不能推出该 context 为空。存在多个 caller；本轮不声称完成全映像 alias 闭包。 |
| Q04 Player vtable 消费者 | 已从 B6BDDC 当前 cell 值 782AF0，穿过 COL/RTTI、address point 与 ctor vptr store，接到 Main Player vptr+0x0C 和 782AF0 六消费者；generic Entity vptr+8 保持为另一条调用路径。 |
| Q05 B normal exit | 未闭合。已定 RETIRING 的 admission/neutral 次序并区分 pause、selector cancel、全 Player reset、Manager remove；没有证据指定只处理 B 的 Player key/member 变化与后续 remove 顺序。 |
| Q06 Resolve/接续候选 | 已读破坏性 Resolve、lead=2、Send→Drain→Accept、Room guard 与 pause；明确当前没有可回滚的 speculative Resolve。推荐 Authority pre 单一发布 owner 的候选，列出与 poll 唯一 writer contract 的差异。 |
| Q07 Join 副作用 | 已复读 9BAC40 selector/key/count/confirm/cancel guard 与 9BAAA0 slot reuse。正常唯一 K 路径只会复用/启用一个同 K slot；成功 confirm 后 selector index=-1 且 Player vector 出现 K，阻止正常重建。重复 query 不被消费，多个不同 key selector 仍可各自触发副作用；异常/历史重复 row 不由读-only query 消除。 |

## W01：pair 布局与调用层级

913760 的 element+0x378 不是 pair 指针成员。9137F3 从该内嵌地址直接取 pair[0]，9137FD 再读 source device+0x0C；entry+0x0C 保持为 list element 自身 key。913811 形成 pair 本体地址并在 913817 调 902F10；902F10 接收 pair 作为 this、device 作为栈参数，写 pair[0] 与新 control reference pair[1]，成功分配时参数为零会重置为 {null, control-to-null}。902E80 是交换两项的另一操作，不能与 reset 合并。913760 的 null source 分支使用 0xDEADBEEF 只作局部诊断比较，不是自有设备 key。

| 层级 | ABI/参数 | 返回/作用 |
| --- | --- | --- |
| helper 956110 / 9561E0 / 9562D0 | 由 Player/action helper 传入 key 与 action；具体三层 scalar ABI 沿用 v8 E03。 | 通过 Manager 按 key 查询；scalar 最终以 XMM0 返回是继承证据。 |
| A20FB0 | Manager 是 ECX/this；key 与 callback/payload 是独立参数。key 不为 -1 时走 Manager vslot+0x28 查找；-1 走锁内 active pair 遍历。 | thiscall，尾部 RET 14h。伪代码把 callback 形参写成无参不能覆盖 raw push/call。 |
| A20940 / A209E0 / A20AB0 | 三者是 cdecl callback，入口参数含按值 device/control 与 action/payload；callback 本身没有 Manager this。 | 前两者调用 device vtable+0x30/+0x34 并返回 AL bool；A20AB0 调 +0x38，float 经 output pointer，AL 表示非零。 |
| device query slot | device 才是 ECX；action 指针作为独立实参。 | +0x30/+0x34 为 bool，+0x38 为 x87 ST(0) float；helper 返回路径见继承 E03。 |

证据 E01–E02 记录本轮 raw、receiver 和结论边界。继承 scalar helper 与 six-consumer action mapping 不冒称本轮直接读取。

## W02：context/list 元素、Offline 条件与通知影响

952410 在 952DBC 形成 n298+0x4B3D8 context；906D60 在 906E7B 注册 913760 与 context，注册节点由 A1D610 保存；A212C0 以断开 device/control pair 和 context 调 listener。90AEB0 沿该 context 遍历，local UserProfile 条件成立时返回嵌入 pair source device+0x0C，否则 -1。90AF30 通过 90B070 找 local entry，再走 Manager keyed lookup 与 902E80 替换 pair。90AC70/90B010 的 primary lookup 用 entry+0x0C 对 Player+0x1618；source pair key 是独立诊断/关联字段。A1E050 以全局 dword_C7362C 写 Base+0x0C own key 并递增 counter；它写的是 list element 本身的 key，不是 pair source key。

6F6500 处理 20-byte records。local branch 的 n4&1==0 且 UserProfile vslot+0x14 命中时，分配 1016 bytes，以 901930(object,n298[76533]) 构造 NetInputDevice-derived element，写 UserProfile +0x374，并经 905F60 建 control/ref 与 Manager 插入，再把 element 指针插入 n298+77046（地址 n298+0x4B3D8）。remote branch 在 profile+0x14 false、90ABF0 认定 UserId 不重复后，分配同尺寸，调用也初始化 NetInputDevice/UserProfile/pair 的 9015B0，再过 905F60 并插入同 vector；随后 90C7E0 关联。已读 callsite 包括 6F5F80→6F5850、90DE8F→90D9B0、90E3D4→90E2A0。该链说明 native 列表元素不是 Gamepad hardware descriptor，也不是 Manager active pair。

9BAC40 的 Offline 分支显式比较 n298[77046] 与 n298[77047]，即 context vector begin/end。已确认的空条件是这个 context vector 在该分支为空；仅 Online roster 为空不足以推出它为空。对原生 NetInputDevice entry，通知 cast 成功且 element 指针等于断开对象时会 erase；但对自有 InputDeviceBase，913760 cast 为 null，通知最多按 source key reset 内嵌 pair，不能 erase 该 context row。因此“已发送 normal Manager remove 通知”也不足以推出自有相关 row 已从 vector 移除。当前实例的成员/通知次数是未观察运行值，但 writer、guard、reader、通知分支的写入效果已静态确定。

正常 A20EB0 Manager remove 按 device 从 Manager active pair 移除，先触发 +0x7C/+0x80 通知。913760 先将通知对象 cast 为 NetInputDevice。对本轮自有 InputDeviceBase，cast 为 null，因此不会因对象指针相等 erase 原生 Net entry；只有某 entry 的 embedded pair source device+0x0C 与通知设备+0x0C 相等时，才调用 902F10(pair,0) 清 source association。无 key 命中则 context entry 不变。该清理不改 entry+0x0C own key、Player+0x1618、selector，也不证明所有 listener/cache 均清除。实际通知配对及是否命中需留作现场事实。

## W03：Room → Player 虚分派 → 六消费者

B6BDCC 的 COL 指向 BAE31C，offset/cdOffset 为 0，TypeDescriptor C5D6DC 标识 Entity_Player，hierarchy 为 BAE2F8。vtable address point 是 B6BDD0；B6BDDC 是其中 +0x0C 项，当前 cell 值 782AF0。753DA0 ctor 在 753DF2 以 C7 07 D0 BD B6 00 写该 address point；754ED0 destructor 在 754F04 也写回该点。

正常限定 Room 路径为 954F7D poll A1FC00，state=2 且 9C3990/9505E0 两个 subguard 均 false 才于 9551BD 进入 6FA540；6FADA7 调 9BAC40，返回 6FADAC。Join 后 n298+0x4B130 非零，或 n298+0x4ABBC bit0 为 1 时，会跳过 Room；否则 955231→6FADC0，在 room 条件允许时 6FBA4D→802980→803E70→4186C0。Room hook 的 Authority(true) 在 native StandardRoom 前，Authority(false) 在其正常返回后；不是所有 Update 都会有这次 scope。

4186C0 的 generic Entity list 在 41885E 按对象 vptr+8 调用，Player 对应槽值为 6AE7D0。Main Player vector 的另一条路径在 418D86–418E4D 使用 Player vptr+0x0C，对应 782AF0。这两处由不同容器/guard 驱动，不将 +8 和 +0x0C 视作同一次虚调用。782AF0 的六类消费者直接 call 如下：

| Player 函数内地址 | 消费者 | 目标 |
| --- | --- | --- |
| 785BC2 | Movement | 779B40 |
| 785BC9 | Bomb | 780F40 |
| 785BD2 | Item | 791A40 |
| 785BD9 | Pill/Card | 7A3D30 |
| 785BE0 | Shooting | 77B020 |
| 785BE7 | Drop | 7A42B0 |

上述目标与每个 action/helper 的细分映射沿 v8 E03 继承：Movement 0–3 与 Drop 11；Shooting 4–7 并有条件调用 0–3/Drop 11；Bomb 8；Item 9/条件 10/Drop 11；Pill/Card 10/条件 9/Drop 11；Drop 11 triggered+held。BAB240 与 BAB370 的固定方向表也沿继承证据，不在本轮冒称直接读取。

现有 C/S 早期逻辑仅包住 keyboard/gamepad logical query 与 Player update/consumer。ActivePlayerUpdateHook 先更新 owner identity，再调用原生 Player update；查询 wrapper 先调用 native original，之后才 Evaluate，故它不是 native device slot 前的 generic short-circuit。Evaluate 只有 Authority scope、main thread、steady EntityMutation、唯一 Player/device、generation 相符及自然 consumer/action family 对齐时才给 Snapshot/SuppressNeutral；cross-family mismatch 走 TerminalPassthrough。由此当前 source hook 可能在 logical wrapper 对结果做合资格覆盖，但它不使普通自有 device 自动出现在 native keyed Manager，也不代表该 device 的 +0x30/+0x34/+0x38 已被 native consumer 读取。生产 source 当前没有这项注册/slot实现。

## W04：解绑、RETIRING 与 A/B 影响

7A6450 是 Player+0x1618 的 SetControllerId 路径。propagate=0 时 Player controller key 的写入限于目标 Player，但仍更新相关 NPC key 引用；propagate=1 递归更新关联的子/linked Player。它不是 Player removal。908070 由 Online lost-connection 的 90E6D0 路径触发，在 Online guard 下对 Main Player vector 全部设 -1,0。9BF530 由 959720 状态转换触发，先全 Player 设 -1 再重映射。两者影响全体 Player，不能用作仅移除 B。9BAC40 action15 只把 selector index 设 -1；A20EB0 只删 Manager active pair 并发通知；二者都不替代 Player key 更新。

key=-1 的 helper 路径在 context vector 为空时可落到 A20FB0 all-active-device 查询，因此可能再读 A；context vector 非空时则逐项查 NetInputDevice 条目。对 B 直接 SetControllerId(-1) 会改变 lookup/fallback 行为，不构成 B-only retirement。

现有证据没有可用的正常 B-only Player exit/unbind caller，也没有顺序证明可把 B 从 Player/member/source-context 移除后再安全删除 Manager pair。因而只给出已受[coordinator contract v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)支持且无需等待这个缺边的第一步：进入 RETIRING 时立即关闭 Join/event admission、owner generation 加一，自有 ABI 覆写查询立即 neutral；保留正 key 以避免 -1 fallback，poll 持续返回非零，NEVER 对象/control/DLL常驻。该行为是 contract 要求，并非当前 production 已实现：active_apply.cpp 的 SyntheticAdapter 在 retired_ 分支返回 TerminalPassthrough，不等同自有 vtable slot 的 neutral。之后 B 的正常 Player 处理、通知清理、A20EB0 remove、member/key miss 顺序仍未闭合，不能宣称 retirement 完成。正常 Pause 只要求 neutral 并保留登记；poll=0 是原生 auto-remove，不能拿来表示 Pause/等待。

## W05：Resolve 语义、epoch 和候选比较

当前源码阶段事实：

| 阶段 | 直接源码行为 |
| --- | --- |
| Outer Update | loop_runtime.cpp 的 UpdateHook 在 native g_update 前记录 UpdateEnter，g_update 正常返回后调用 TopUpdateExit。可用它作为独立 update_epoch owner；当前没有这个 epoch 字段。 |
| Physical poll exit | InputDeviceUpdateHook 先调用 native device update；成功且 return RVA/phase/pause/owner guard 通过后，PhysicalExit 才采样并 SubmitPhysical。Pause/loading 等 guard 会禁止提交。 |
| Submit | SubmitPhysical 使用 observed_completed_tick=g_last_completed_tick；EdgeTargetPlanner 的基准 target 是该值+2（kInputLeadTicks），若同一 edge 的先前 target 更晚则继续递增。SubmitEncoded 顺序为 Send、Drain 一条、Decode、Accept(DrainBeforeResolve)。Accept 要 target_tick>=current_tick、未来距离不超上限，edge target 严格递增。当前生产入口是 PhysicalExit→SubmitPhysical；SubmitEncoded 是 private。 |
| Authority pre | StandardRoomHook 仅在 native Room path 被 hook 正常进入时调用 Authority(true)。Authority 令 candidate_tick=g_last_completed_tick+1，调用 dispatcher Resolve，再 BeginAuthority 并在原函数前绑定状态。 |
| Resolve | InputQueue::Resolve(candidate) 会删掉比 candidate 旧的命令，消耗等于 candidate 的命令，保留 future；即使没有输入也置 last_resolved_tick。相同或较小 tick 再 Resolve 会拒绝。dispatcher 在 candidate>=pending_target_tick 时清 pending。 |
| Authority post | native Room 正常返回后 Authority(false) EndAuthority(true)，再将 candidate 记为 last_completed_tick 并提交 tick。EndAuthority 有快照时 current→previous；没有快照且 pending target future 时 previous 清空；没有 snapshot、也没有 future pending 时 previous 保留旧值。 |
| Update exit/pause | TopUpdateExit 只在 pause/loading 状态转换识别 PauseEnter 时调用 AdvanceOwnerGeneration；该函数清 dispatcher queue/planner、adapter snapshot/previous/pending 与 owner projection。普通 Room guard skip 不会因此推进 Authority tick、清队列或触发 pause clear。 |

对已完成 tick n，正常 PhysicalExit sample 的目标是 n+2；同一 outer update 的 Room candidate 是 n+1，因此本次 sample 不是 candidate 输入。若下一次有效 poll 观察到 n+1，它新 sample 目标为 n+3；不会在 n+1 Authority 之后新造一个已经过去的 n+1 edge。由于当前唯一生产 SubmitPhysical 入口位于 Room Authority 前的 poll/query 阶段，真实流程中没有“Resolve n+1 后，再 Accept 一条 target n+1”的物理输入路径。Accept 本身不比较 last_resolved_tick，但 private SubmitEncoded/当前 lane 不暴露一个这样的外部入口；不能杜撰它为现有反例。未来 target n+2 可在较早 Resolve(n+1) 后入队，随后 Resolve(n+2) 仍是合法且不重复的下一 tick。

如果把 v9 的 speculative pre-poll Resolve 放在当前 poll 之前，则 Resolve(n+1) 会无输入也推进 last_resolved_tick；若后续 native Room guard 跳过，下一次真正 Authority 仍是 n+1，其同 tick Resolve 被拒。已从 queue 消耗的 target n+1 edge 不可恢复，且 pre-resolve 无权把 Authority snapshot 当作 native Room 实际执行。当前正常物理 lead=2 让 poll 后新命令落在 future target，并未证明存在同 target 后续输入；但这不使 destructive Resolve 可回滚，也不使其可作为“peek”。若没有 Room，现行源码自身不做 Resolve，queue/last_resolved_tick 留存；TopUpdateExit 只在 pause transition 清 generation。

候选比较与推荐：

1. 保持 poll 唯一 S2 writer，则必须在 poll 只 latch 一个显式的 future publication/neutral marker，另以独立 staging/reserve 语义保存 command；不能在 poll 使用 destructive Resolve。候选需要定义 Room skip/pause 时的取消、同 tick再入、未来 target 与 pending 清理、publication/previous 推进和 generation 失效。当前源码/contract 没有这组 reserve/commit/rollback API，且 Authority 仍必须只读 poll 锁存值。
2. 推荐一个逻辑 owner：已经进入的 Authority pre 负责 Resolve 一次、创建 immutable snapshot 或明确 neutral snapshot、推进 publication_epoch；native poll 对自有 S2 只保持非零并不写 previous/current，六个 query slot 只读 Authority 已发布快照。Authority post 仅在 native Room 正常返回时提交 target_tick。Room guard 未进入则不解析、不发布、不推进 target_tick；pause 由 generation clear 使旧 snapshot/edge 中性失效。该做法和当前 Authority candidate 的单次 queue Resolve 时点一致，避免把无输入 Resolve/过期边藏在提前阶段；但改变 contract v2 的 poll-only writer 与 post-authority lifecycle phase，须先更新 contract/schema，当前为 DESIGN_ONLY，未采纳。

一致候选的身份规则：

| 身份 | owner/递增时点 | 用途 |
| --- | --- | --- |
| update_epoch | 主线程 UpdateHook entry，每次 outer update 加一；退出无论 Room 是否进入都结束该 update。 | Join 未进入 pass 的命令在该 update 的 UpdateExit 关闭，不能等待停滞 gameplay tick。 |
| publication_epoch | 成功进入 Authority pre、完成一次 Resolve 并发布一个 immutable state/explicit neutral marker 时加一。 | 区分快照一次性发布与查询/重复 poll；poll 不推进它。 |
| target_tick | 现有 Authority candidate 为 last_completed_tick+1，只在 native Room 正常返回的 post 提交后成为 completed tick。 | gameplay command 生效刻度；不由 outer Update 次数代替。 |
| join_pass_id | 经已观察的 6FADA7 调入 9BAC40 前 Begin；正常返回到 6FADAC Complete。 | 每个 pass 最多取一个该 pass JoinCommand；命令序号、pass id、target update epoch 隔离。其他 caller 仍须另行证明。 |

JoinCommand 在 9BAC40 Begin 之前的 Update epoch 内经独立 lane admission；第一条已观察 pass 读取后即消费，6FADAC Complete 关闭；本 update 无 pass 时由 UpdateExit 关闭；CloseAdmission/RETIRING 随时优先清 pending。输入查询只读，不在 query 中消费命令。press/hold/release/repress 对 gameplay action 0–7 可由 current/previous 推导；action8–11 当前只有 edge_flags bit0 映射，不能代表持续 held，需版本化契约，不用 query 每次 true 修补。

候选与 contract v2 明确不同：快照唯一 writer 从 device poll 改为 Authority pre；Authority pre 成为 publication 生命周期 owner；无输入时必须发布 neutral marker，不能在 adapter 缺 pending 时保留过期 previous；poll 重复不推进 S2；publication_epoch 与 target_tick 分离；JoinCommand/pass/update_epoch 单独定义、无-pass update expiry；RETIRING admission 立即关闭。候选未修改 contract/schema/source，用户尚未采纳。

## W06：Offline Join 副作用与八条手工轨迹

9BAC40 的 Offline path 首先要求 n298 context vector begin==end，再扫描 Manager active device keys并遍历四个 selector slots。action19 要 Player count<64、key 不已分配于 Main Player vector、n4_1[52..53] 范围内每个对象的 +0x1618 key、或四个 active selector 等原生 guard；9BAAA0 对已有同 K selector 返回其 slot，否则找空 slot（最多四项）。有效 selector 逐项跳过 invalid key/index 或 9BA980 条件；action20 为当前 selector 查询上一项，返回 true 时终止本轮 selector scan；action21 为当前 selector 查询下一项，返回 true 时推进并继续。action14 把查询结果与存储 confirm byte 合并，清该 byte 后选择 character、调用 9B9CD0，再 SetControllerId(K,1)，完成该 slot 的后续处理并将 selector index 设 -1。action15 只取消对应 selector index。9BB0B8 的 device slot+0x54 是 native raw flush，不是 logical action21；五个 action immediate 的来源见证据 E12。

在本轮正常前提（唯一正 K、A-only、创建前没有 B selector）下，action19 的 Player/list/四 selector key scan 加 9BAAA0 reuse 使 K 只关联一个 slot；成功 action14 后该 slot index 变为 -1、Player vector 收到 K，后续 pass 的 key guard 阻止重新开启 K，因此在 DESIGN_ONLY 的可靠 create/publish 后置条件下不会重复创建同一个 B。query mask 是只读且不会自行消费 command；多个不同 key 的 active selector 可以在同一 pass 分别看到 action14=true 并各有副作用。若已经存在重复 active rows，其来源不在本轮正常前提内；函数的遍历会按 row 处理，不能从重复 query 的返回值证明只触发一次状态变更。9B9CD0 的 full Player construction 仍沿用 DESIGN_ONLY 假设。

轨迹表中 P/C 表示进入时 S2 previous/current，N 表示 neutral；Join-only 命令和 query 本身不推进 P/C。这些是推荐候选下的手工状态符号，并非现行自有设备生产实现。

| # | 命令/target 与阶段 | S2、query、selector/key/Player 状态 | queue/下一阶段与限度 |
| ---: | --- | --- | --- |
| 1 | 同一 update U、JoinPass J 的 JoinCommand {K:select, confirm} 在 9BAC40 Begin 前被准入；按 J 取一次，target_update_epoch=U。 | S2 P/C 不变。先 action19；只有原生容量/唯一 key guard 成立才选中 K。随后 action14 可 confirm；Player count 从 A-only 增到 A+B 是 DESIGN_ONLY 创建/发布假设。 | native 9BAC40 正常返回 6FADAC 时关闭 J；不会把 E 留给下 pass。未证明原生 B init/全局 manager-this。 |
| 2 | 同 pass 的重复 action query、四 selector 遍历和 query mask=true。 | S2 P/C 不变，query 只读。在唯一 K 正常轨迹中只会有一个 active K slot；action14 true 清 byte、创建后把该 slot index 设 -1，后续 pass 由 Player K guard 阻止再次创建。不同 key slot可分别产生副作用；已有 duplicate rows 不属于正常前提。 | query 不消费 JoinCommand。9BB0B8 slot+0x54 raw flush 不消费 JoinCommand。原生对唯一 K 有状态 guard，但无通用 command/pass exactly-once boundary。 |
| 3 | 同一 gameplay publication G 跨两个 JoinPass J1/J2。 | S2 previous/current 保持 G_prev/G；J1 命令只绑定 J1。J1 Complete 后 J2 不得再拿同一 JoinCommand；S2 不代替 Join event。即使原生重新看到 action19/14=true，成功后 Player K 与 inactive selector guard 阻止再次创建同 K。 | 无新 JoinCommand 时 J2 不应重复应用旧 E；没有 Join lane 时，action20/21 等非创建副作用仍可能重现。 |
| 4 | 相同 publication epoch 重复 poll；下一有效 Authority tick 无新输入。 | 同 epoch poll 不改 P/C；Query 在同一 publication 返回同一值。下一 Authority pre 发布 N 后，previous=C、current=N。队列 Resolve 同 tick 第二次拒绝；无输入首次 Resolve 仍推进 last_resolved_tick。 | dispatcher 在 candidate>=pending target 清 pending。当前 adapter 无 snapshot且无 future pending 时不会自动清 previous，故候选必须明示 neutral；正常 End 才提交 tick。 |
| 5 | update U poll 后 Room 被 guard 跳过，且 Join pass 未进入。 | S2 P/C 保持；没有 Authority scope，target_tick 不变、当前源码也不 Resolve。outer UpdateExit 可结束 U。 | 候选在 UpdateExit 关闭 U 的 JoinCommand，不等 gameplay tick；若 pause transition 则 AdvanceOwnerGeneration 清队列/快照。普通 Room skip 本身不触发 pause clear。 |
| 6 | 正常 Pause 与恢复，恢复后再有物理输入。 | PauseEnter generation clear 令候选 S2 previous/current=N/N，PhysicalExit pause/loading guard 禁止提交。恢复 Authority 观察到 PauseExit 后再开始新 generation；旧 edge 不跨 pause。 | 下一样本按 observed completed tick+2 入队；只有之后对应有效 Authority target 才消费。候选从 neutral 开始，再处理新 state。 |
| 7 | JoinCommand=cancel(15) 后同 update CloseAdmission。 | Cancel 只令 selector index=-1；S2 P/C 保持，不改 Player key、Manager pair、B member。若 CloseAdmission 先到则该命令不向 query 暴露。 | close 删除 pending E 并关闭 admission，不能由 query 重新打开；gameplay S2 不因 Join cancel 推进。 |
| 8 | 正常 retirement：B 已有正 key K、可能有 selector/context/source pair。 | RETIRING 关闭 admission、generation++、S2 previous/current=N/N、query neutral；key 暂保留以避免 -1 all-device fallback 读到 A，poll 非零。 | generation transition 清 queue；B 专属 Player exit/unbind、context notification 是否命中、后续 Manager remove/member miss 顺序没有静态证据，故不能将轨迹后半写成已完成；NEVER 保持对象/control/DLL。 |

这些是基于静态控制流和候选规则的人工推演，不是运行模型。标记未决之处并非以 UNKNOWN 替代已读 guard，而是指出具体无生产 lane/无 B caller 的影响。

## P01–P06 系统链摘要

| 链 | 静态已证 | 静态缺边 | 候选 / 未采纳 | 现场值未观察 | 生产未实现 |
| --- | --- | --- | --- | --- | --- |
| P01 注册/枚举 | 38-slot 合同约束继承；native Manager keyed lookup、context lookup、各自 writer/reader、通知清 association、Manager remove 都已区分。 | 完整全映像 alias 不在范围；native source cast=null 后并不 erase Net entry，是否有 target custom association 的专门清理未证。 | Authority publication 的职责迁移。 | active pair/context 实际成员与 listener set。 | 自有 device init/register/insert、keyed lookup 复核。 |
| P02 type/name/binding | base ctor 后自有 vptr、38槽/六覆写/六槽 control/S2/NEVER 沿 contract；Native 32-entry template 是历史继承。 | 当前 source 未对真实自有对象注册，尚无 native consumer 到其槽的生产关联。 | 固定 ABI 与新 publication contract。 | actual vptr/name/binding primitive 与运行初始化。 | 自有 ABI 构造、Init、native binding/name 安装。 |
| P03 Offline Join | 9BAC40 empty context guard、四 selector、19/20/21/14/15、key/count/confirm/cancel 和 6FADA7 call 已复读；正常唯一 K 在创建后受 Player key/selector index guard 保护。 | 原生没有跨 query/update 的 JoinCommand admission/pass boundary；不能由 mask read-only 证明导航/其它 selector 副作用 exactly once。 | 独立 JoinCommand/update epoch/pass 关闭。 | Online roster、context vector、selector/key/Player count。 | Join input lane、pass lifecycle、C/S B native create/publish。 |
| P04 key/B ordinal | own list key 与 source association key 分离；90AC70 主匹配 entry key 到 Player+0x1618；SetControllerId propagation 有限作用域清楚。 | B-only normal exit/unbind 和其对 context/cache 的顺序未证。 | 保留正 K 直到 B-specific exit 确认。 | K 值、B ordinal、pair/listener 是否关联。 | 自有 K 分配/绑定与 B unbind。 |
| P05 snapshot/query | Room guard 条件路径到 Player +0x0C→782AF0 六消费者；C/S wrapper 先 native query 再 Evaluate；queue lead/resolve 语义闭合。 | 生产自有 device 进入 native keyed reader 的边缺失；held 8–11 当前编码为 edge，不是 level。 | Authority pre-only publication、explicit neutral、epoch 分离。 | game 内 query频率/实际 epochs 与值。 | 自有 slot 查询、held 8–11 schema、发布器 contract。 |
| P06 pause/退休/cache | Pause generation clear、Manager remove、selector cancel、all-Player resets、native poll-zero auto-remove 已分开。 | 仅影响 B 的正常 exit→notification/context→remove 顺序未闭合。 | RETIRING admission 立即关闭、中性且非零 poll。 | Pause/retirement 是否发生、listener与实际命中。 | B-only 正常 retirement 与验证。 |

## 剩余静态工作与审阅记录

仍需后续工作才能声称正常功能完整：为 B 找到并证实正常 Player exit/unbind caller 及其与 A20EB0、通知/context cleanup 的序；查明所有适用生产 path 上的自有 device native keyed registration；对 candidate 修订 contract/schema 明确 Authority publication、neutral marker、JoinCommand/pass 与 update expiry；再实现自有 ABI、Join lane 与 retirement。当前静态证据也不宣称运行时 vector/listener/key 集合。

唯一整体人工静态文档审阅结果：**通过（文档准确性与所声明范围的一致性）**。审阅覆盖工作设计 v9、v10 报告/证据、相关 SOURCE/CONTRACT 与 topic README，核对 pair/callback、列表对象与 guard、Player 虚分派与消费者、B 退休边界、Resolve/epoch 候选、Join selector 副作用、来源标签、状态分类与引用。未发现事实误述或文档间矛盾。正常功能静态完整性仍未完成，B-only exit/unbind 与生产 keyed registration 等缺边保持明示；本结果不表示设计候选已采纳、现场值已观察或生产功能已实现。审阅后仅回填本结果，没有修补或重审；没有运行测试、构建、哈希脚本或游戏操作。
