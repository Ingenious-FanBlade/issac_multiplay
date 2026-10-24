> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# 自有 ABI 正常功能静态确认 v12

推进：tx-01-24  
执行日期：2026-10-13  
结果：W01–W04 本轮研究与材料已完成；静态分析确认 native disconnect/reselect 不是已证实的 B-only Player 退休路径。整个自有 ABI 正常功能静态完整性仍未完成：Main Player vector 中 B 的正常退出/移除、随后清理 context 与 Manager pair 的先后和 A 的实际继续/暂停后置仍缺直接证据。

本轮接入 IDA 只读服务。实际 IDB 为 32 位 isaac-ng.exe，imagebase 0x400000，Hex-Rays 与自动分析就绪。95B310 反编译全文及 747 条指令分五页读完，函数末尾读至 95BDC4；另直读其 UI、菜单、context、Player setter、Join callee 与 source caller，并取关键字节及 xrefs。IDA 只作静态读取，未修改 IDB。没有运行测试、构建、harness、probe、状态模型、哈希或链接检查，也未访问或操作游戏进程。

报告和证据分册：[静态证据 v12](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v12.md)。唯一人工整体文档审阅在本报告、证据和入口修改全部完成后进行；结果只在审阅后填写。

范围为正常分配成功、正常返回、主线程有序、Offline/local-co-op、唯一正 K、A-only 起步；保留 38 槽 device、六个 override、六槽 control block、S2、NEVER、base ctor 前固定模块、每 session 至多一次创建、A ordinal 0/B ordinal 1，以及 A/B 共用 scheduler、sequence、target tick、queue、Resolve/apply。C/S 与原版和 Isaac Sync 隔离；不使用 Online Manager、NetInputDevice 或 FrameStore。原生 Join 能可靠创建/发布 B 仍是 DESIGN_ONLY 前提。本报告不扩到 OOM/EH、并发、ABA、全映像 alias、持久化、异常恢复或 05-06 全网络行为。

## 纠正项与研究结果

| 项 | v11 说法或待确认点 | v12 静态结论 |
| --- | --- | --- |
| Q01 锁调用身份 | E05 把 C57B2C 的 vtable +0x0C、参数 -1 叫作按 key 查询。 | 更正：C57B2C = C57B18 + 0x14，是 InputManager 的 embedded Mutex。95B35C 调其 +0x0C 并压入 -1 是锁定操作；95B37F 调 +0x10 是解锁。不是设备查询。按 key 的 Manager 查询位于 95B6B6，ECX=C57B18，Manager vslot +0x28，参数为候选 K。 |
| Q02 A20240 调用点 | 95B395 被标成 call A20240。 | 95B395 只是 mov ecx,C57B18；随后 push output 后，95B39B 才 call A20240。参数为 Manager receiver、输出 pair 地址和 vector index。 |
| Q03 cast-null 回落 | v11 将 Base-to-Gamepad cast-null 后的泛化路径留作未知。 | 95B3C6 的 RTTI cast 为空时走 95B417 的通用后续分支；该失败值在 cast guard 后没有被当作 Gamepad 解引用。完整函数证明 Base 对象仍进入后续 keyed lookup、菜单/UI 和 reselect 主路径。 |
| Q04 roster 与菜单对象 | 旧描述可能把 context vector、Online roster 与菜单状态合并。 | n298+0x4B3D8 的 begin/end 是本地 context vector；n60+0x1BAA8/+0x1BAAC 是 PlayerManager 主 Player vector；n15 是 menu-state 对象。三者分别处理。95B815 的 n15[16]==12 分支调用 98AA30 转菜单状态，不是 Player removal。 |
| Q05 候选索引 | 可将 95B87F 的 v91 直接写为 B ordinal。 | v91 是当前本地候选 key vector 的循环索引，通常不能等同 Player ordinal。仅在 n15 为空、n60 存在、context vector 为空并且候选来自逐个 Player key 的窄路径，合法 v91 才与该 vector 下标相同；仍须验证实际唯一 K 和目标 Player。 |
| Q06 native 断线效果 | 可能把 UI 或 SetControllerId(-1) 当成 B 已移除，并假定 A 的结果。 | 95B310 的 setter/context/UI 操作不删 Main Player vector。95B310 正常返回后 caller 会继续，但随后 Join/Room 有 guard；这段没有证明 A 必继续或必暂停。不得把 popup、菜单切换、解绑和 Player exit 当作同一件事。 |
| Q07 首次无 Room | v11 曾说第一次无 Room 时 queue/planner/pending 原样留存。 | 更正：若同一 outer update 的 poll 通过物理提交 gates，PhysicalExit 可先 SubmitPhysical，更新 planner、queue 和 pending；随后 Authority 因 Room guard 未到，TopUpdateExit 只记第一连续 miss。第二次连续 miss 在普通 recovery 条件下触发 PauseEnter 与 generation clear。若该 update 没有成功 poll 提交，则相应队列部分可以没有改变。 |
| Q08 S2 查询类别 | v11 的 S2 候选要求 consumer 必须等于 ConsumerForAction(action)。 | 不适用于自有 ABI：native 有跨类别读取。明确许可表见 W03。S2 仍须校验真实 consumer witness、Room Authority scope、Player、K、generation 与 publication，而不能仅按 action 所属类别放行。 |

## W01：保留 ABI 与有界勘误

本轮没有重做全部自有 ABI；以下是继承并适用于这次执行的边界：固定 38 槽 Base-compatible device，六个 override 槽、六槽 control block、S2 和 NEVER；模块在首次 base ctor 前固定；设备登记 pair 的 own key 与 embedded source pair key 分开；setter、Manager pair remove、poll-zero auto-erase 和 Player vector remove 是不同动作。A/B 的动作都进入相同 scheduler 与 Authority/apply 链。ABI、对象与 consumer 的早期证据按 STATIC-INHERITED 标记；本轮新确认只升级所直读地址。

Input ABI 的位映射仍是：动作 0–7 由 held level 表达；动作 8–11 各有 edge bit 0–3。现行 v1 held bit0 为 Shoot，缺少 Item/Card-or-Pill/Drop 的 level。原生读取关系与最低版本化方案见 W03；本轮没有修改 contract、schema 或生产源码。

## W02：95B310、断线 UI 与 B 退休边界

IDA 的唯一直接 caller 是 954CD0 内 954F84；同一函数此前 954F7D 进行 native poll。因此本路径的已读局部顺序为 poll → 95B310 reselect → 有 guard 的 Join/Room 后续。VA/RVA、raw、receiver、参数、返回和 branch 后置见证据 E01–E08。

95B310 的 ECX 对象是 n298 selection/context holder。起始条件读取 n298+0x4ABC8；值为 -1 时先遍历 InputManager active pair vector。95B350–95B382 以 C57B2C 做 Mutex 锁/解锁并读 pair vector 长度；95B39B 调 A20240 取 index 对应 pair，ECX=C57B18。随后将 InputDeviceBase 指针做 RTTI cast 到 Gamepad。cast 为 null 时进入普通泛化路径，不因失败 cast 中止整函数。

候选 controller key 的来源由 guard 决定：菜单对象存在时，只有 ArgList_0 有效才加入单个 key；否则若 PlayerManager 存在且 n298 context vector 为空，则从 Main Player vector 的 Player+0x1618 逐项取 key；若 context vector 非空，则由 90AEB0(context) 取本地 context key。当前选择值是 n298+0x4ABCC。v91 只是在这组局部候选里的下标。

对于当前选择尚未确定的候选，95B6B6 经 InputManager vslot +0x28 按候选 K 查设备。lookup miss 位于 95B6EC；函数在 95B720 构造 UI 状态，在 95B783 或 95B7A2 加入 RESELECT_CONTROLLER / CONTROLLER_DISCONNECTED 提示，再于 95B7F4 加入消息。sub_9C2DB0 将事件追加到 UI queue；95B806 读取 9C39E0 对话结果，95B811 写回 n298+0x4ABCC。UI 选择与结果写回没有删除 Player vector 的直接后置。

UI 后的分支再次区分对象：若 n15 菜单对象存在且 n15[16]==12，函数调用 98AA30 改 menu-state；否则只有 n60 存在且 context vector 为空时才由 417870(n60,v91) 取得 Player 并在 95B87F 调 SetControllerId(Player,-1,1)。context 非空时函数改走 90AF30(context,-1)。这里不是对所有 miss 无条件解绑。

在另一设备扫描/匹配路径，函数以 Player+0x1618 和 device+0x0C 比较分配；找不到对应 Player 时清 selection 字段并可能把设备 K 写入 context。95BCA0 的 rebind 只有在菜单为空、PlayerManager 存在、context 为空且扫描候选 K 有效时，才以 Player vector[v91] 调 SetControllerId(Player,K,1)，之后清 PlayerManager selector/assignment 字段并调 A21CB0 refresh。上述字段清零与 refresh 不是 vector erase。

SetControllerId(7A6450) 写 Player+0x1618；propagate=1 会影响其链接的 Player/NPC 关系，但被直读代码没有从 PlayerManager Main Player vector 删除目标。若 B 的 key 被改为 -1 且 context vector 为空，Player lookup 的 all-active fallback 可选中仍登记的 A；这构成“置 -1 不等价于退休”的实际负面依据。它不证明 A 在这个 caller 中最终继续更新，因为 954CD0 后续 UI/Room guards 和 manager effects 仍可能改变流程。该结论是有限且具体的：本函数不实现 B vector removal；被看到的 Manager pair remove 也不写 Player key；UI 结果没有 remove 后置。

有限替代路径定位结果：95B310 只有 954F84 一个直接 caller；SetControllerId 的 31 个 xref 中，95B87F/95BCA0 是上述 reselect，9BB424 是 Join 成功创建后的 key 绑定，908070 与 9BF530 是既有 all-Player reset/remap，不是 B-only remove。A20EB0 的有限代码 callers 为 906020、90A8A0、A21F40：906020 取 pair 后 remove；90A8A0 在对象 teardown 条件下批量清 Manager pairs；A21F40 的 Gamepad disconnect handler 仅匹配真实 Gamepad descriptor 后 remove pair。三者都不能证明 Main Player B erase。

所以顺序 A（先 RETIRING/关闭 admission、使 query neutral、完成 B-specific Player exit，再清 context/Manager association）具备候选方向但 native exit/remove caller 与完整后置未闭合。顺序 B（先 A20EB0 remove，再靠下一次 95B310 lookup miss）不能保证 B Player key/context 已清，也会触发 UI/reselect 路径。两条都不是可直接声称成功的 native B-only retirement。whole-run teardown 在 SOURCE-DIRECT 上可关闭当前 C/S scheduler、generation 和 adapter；这不证明自有 pair/key/context 被删。RETIRING 时须先关 admission 和 publication、使 generation 失效并让 query 中性；登记期间 poll 要持续非零，避免 native 以 poll=0 自动删除 Manager pair。NEVER 对象常驻要求继续成立。

## W03：S2 自有 query 许可和 Authority 窗口

原生读取者的六类 action 映射来自 v8 E03，属 STATIC-INHERITED；本轮没有重做那些 Player query 实现。表中跨类别读同样合法，候选 S2 不得复用现有 adapter 对 synthetic action 的严格同类别比较。

| 硬件 consumer witness | 自有设备可读动作 | 依据与必要语义 |
| --- | --- | --- |
| Movement | 0–3、11 | 本类动作与本类一致；Drop reader 也可由 Movement 路径查询。11 的 pressed 用 level，triggered 用 edge。 |
| Shooting | 4–7、0–3、11 | 原生 Shooting 路径还读取 Movement 和 Drop。 |
| Bomb | 8 | 只需 Bomb trigger edge；保持 edge-only。 |
| Item | 9、10、11 | Item 路径在条件满足时可读取 Card/Pill 和 Drop。 |
| Card/Pill | 10、9、11 | Card/Pill 路径也可读取 Item 和 Drop。 |
| Drop | 11 | pressed level 与 trigger edge 都可观察；原生 pressed=false 会清 Player+0x17C0 timer，pressed=true 会递增。 |

候选许可必须同时满足：session 匹配；apply generation 与 owner generation 匹配；publication_epoch 与本次 current 匹配；Authority target tick 匹配；main thread 和实际 scope 均为受限的 StandardRoom Authority；Player 指针稳定且 ordinal==1；Player+0x1618 等于设备+0x0C 的正 key K；consumer witness 来自现有六类限定 reader；consumer/action 落在许可表内。超出任一 scope、post 之后、无 Authority 或 RETIRING 均读中性。按 action 只做只读 pressed/triggered/value 查询；不在 query 中消费 edge 或改共享状态。

对于 9–11，现行 v1 edge flags 只能表达按下瞬间，不能表达 Item、Card/Pill、Drop 持续按住。最小后续 schema 版本候选为 held bit1=Item9、bit2=Card/Pill10、bit3=Drop11，held bit0=Shoot 保留；动作 8 仍只用 edge bit0。该候选未被采纳且没有修改 schema。Held 与 edge 输入需共同支持 press、hold、release、repress；具体表在证据 E10。

现有 source 的 normal Authority pre 调 destructive Resolve，normal post 才提交 tick 并 EndAuthority。候选采用 Authority-pre 唯一发布 current 或显式 neutral，normal post 更新 last_completed/previous 并立即关闭当前 query，下一次 Authority 才产生新 publication。previous 仅是比较基线，不是 post 后继续开放的 query snapshot。无输入也必须发布显式 neutral，才能释放上一帧 held；现行 active adapter 在无输入时传 null，不能证明自有设备已 neutral。

本地输入真实窗口按顺序分开：

1. 有效 PhysicalExit poll 可在同一 outer Update 的 Room guard 前 SubmitPhysical。观察 completed tick n 时，planner 初始目标是 n+2；边沿只会依单调规则推迟。poll 早于 Authority 不表示这份输入已经是当次 Authority 的输入。
2. StandardRoom guard 正常进入，Authority pre 使用 candidate n+1 执行 Resolve/发布；normal post 成功后提交 n+1 并关闭 publication。queue 中 target=n+2 的命令留给下一 Authority。
3. 若 Room guard 未进入，第一 miss 没有 Authority Resolve、publication 或 post tick；但同 update 的 PhysicalExit 如果成功，之前已经可能改 planner、queue、pending。TopUpdateExit 只将普通 gap 计为 1，不清 generation。
4. 第二次连续普通 miss 且没有 loading recovery 时，TopUpdateExit 达 threshold 2 后 PauseEnter 并 generation clear，清 queue/planner/pending、adapter current/previous 等；这发生在本次 update 其他 poll 提交之后。此时无 Authority tick commit。
5. pause 阻断下一次物理提交。恢复 Authority 前后由现有 PauseExit generation clear 先失效旧 input；loading Enter/Exit 各自也会 generation clear。LoadingExit 后 poll 被 recovery gate 阻断，authority-less recovery miss 不记作普通 pause gap；首个恢复 Authority 解除 gate。

此候选与 contract v2 的明确差异是：contract v2 §4 把 device +0x78 poll 定为唯一 latch writer；Authority-pre publisher 将写入职责移动到 Authority，并新增 publication epoch、normal-post commit/close、无输入 neutral。当前 destructive Resolve 不能在 poll 阶段调用后回滚，所以若不修改 contract，就必须另设计 reserve/stage/commit 才能保留 poll 唯一 writer。本轮不修改或采纳合同。

## W04：Offline Join 的 K、pass 与每 session 一次创建

IDA 直读 9BAC40、9BAAA0、9B9CD0 与 6FA540 caller。native Join selector 的唯一 xref 是 6FADA7，调用位于 sub_6FA540；sub_9BAC40 normal return 回到 6FADAC。Offline 条件检查 context vector begin==end；它与 PlayerManager 主 vector、online roster 分开。按 K 的 selector 表最多四个槽；action19 只在 K>=0、selector guard 成立时复用现有槽或占空槽。action20/21 是导航并最终继续扫描其它 selector；action14 是确认路径，action15 是关闭/取消路径。

可定位的 Player create 调用是 9BB415 调 sub_9B9CD0(PlayerManager Main Player vector, selected player type)。这条边是本轮实读到的真正 create attempt 调用点。成功路径在返回后读取新 Player 的 controller key 关系并于 9BB424 调 7A6450(new Player,K,1)。9B9CD0 自身插入/初始化 Player vector；普通创建存在数量 guard，失败可不产生有效 Player。native selector/action query 本身是读，不是 creation attempt。

DESIGN_ONLY attempt owner 固定为 per-session main-thread state，状态终结规则如下：

| 窗口 | 是否创建尝试 | 完成规则 |
| --- | --- | --- |
| selector admission/open、action19 建立/复用 K slot | 否 | 只建立候选；不占一次创建预算。 |
| action20/21 导航 | 否 | 跳到共用 footer、可继续下一个 selector；不尝试 create。 |
| confirm query 未通过原生 guard，或没有抵达 9BB415 | 否 | 本 update 的命令到期关闭；需要新的显式命令才可再试，不自动重投。 |
| 抵达 9BB415 前 | 否 | 单次 owner 先原子/顺序标为 AttemptStarted，再进入 sub_9B9CD0。 |
| sub_9B9CD0 返回后，新 Player、唯一 K 和发布后置成立 | 是，成功 | 标记成功终态；不接受第二次 session create。Player 后续 gameplay 再走统一 scheduler。 |
| 已调用 sub_9B9CD0，但失败/返回 null/后置身份核对失败 | 是，失败 | 标记失败终态；NEVER/RETIRING 处理按生命周期设计，不自动重试。 |

JoinCommand 候选以 session、generation、K、sequence、target update epoch 及首次有效 join_pass_id 绑定；它不要求 Player 还不存在时就拥有 B ordinal，也不把 update epoch 当 Authority target tick。同一 pass 重复查询只返回相同只读候选；在 6FADAC normal completion 由 owner 关闭/消费，没有 pass 的 UpdateExit 关闭；generation change/RETIRING 也关闭。多 selector 的其它原生副作用、多个外部 caller 或任意 query 型副作用没有通用 exactly-once 保证；query 只读不能推出任意副作用恰好一次。

## P01–P06 状态矩阵

每行分别记录静态事实、仍缺的 native 静态边、候选差异、运行值和生产实现，不用未实现或未观察替代静态结论。

| 项 | 静态已证 | 仍缺的 native 静态边 | 候选差异 | 运行值 | 生产状态 |
| --- | --- | --- | --- | --- | --- |
| P01 注册/枚举 | Manager pair 锁/枚举、A20240 index reader、keyed lookup 已直读；早期 insert/context factory 与 callback reader 继承旧证据。 | 自有对象如何通过正常 keyed registration 发布未实现，因此不能声称本轮有 native owned-registration 路径。 | S2 仍需明确 publisher；contract v2 writer 差异见 P05。 | 实际 pair、context/listener、K 未观察。 | own ABI Init/register/insert 未实现。 |
| P02 Base/类型/consumer | Base cast-null 落回通用路径已 IDA-DIRECT；38/6/6、S2、NEVER 和六个 reader 继承 contract/旧证据。 | 具体自有对象的 COL/name/binding 现场关联未观察；不属于已实现的 production object。 | held bits 9–11 要版本化。 | 实际 vptr、COL、name、binding 未观察。 | 自有构造、vptr、Init 未实现。 |
| P03 Offline Join | 9BAC40、selector K guard、19/20/21/14/15、9BB415 的 create call、9BB424 key bind 直读。 | 原生没有跨 update/pass owner；创建后可靠发布仍按 DESIGN_ONLY premise。 | K-bound JoinCommand/pass/expiry 候选未采纳。 | roster、selector、K、Player count 未观察。 | Join lane 与可靠 B 创建/发布未实现。 |
| P04 B key/ordinal/exit | 95B87F/-1 与 95BCA0/K caller、7A6450 propagate 及 vector 不删的负面路径已直读；A20EB0 不写 Player key。 | B-only Player vector removal、UI 对 A 的后置、context/Manager remove 次序、setter 目标是否是运行时 ordinal1 仍缺。 | RETIRING 要保留 K 到 B exit 后、query neutral、admission closed；不可用 -1 冒充退休。 | B ordinal、K、linked Player/context 未观察。 | K bind 和 B-only retirement 未实现。 |
| P05 snapshot/query | Source 的 poll、SubmitPhysical、Authority Resolve、normal post、six consumer witness 及 contract poll writer 已读/继承。 | native poll/Join/Room/consumer 在实际集成中的 owned-device end-to-end 顺序无 production path 可证明。 | Authority-pre publish/post-close/no-input-neutral 改变 contract v2 poll 唯一 writer；candidate 尚未采纳。 | publication epoch、query 次数和值未观察。 | 自有 S2、query slots、schema/contract 接入未实现。 |
| P06 pause/retirement | threshold=2、loading recovery、generation clear 与 RunTeardown SOURCE-DIRECT；95B310 reselect 与 Manager remove 互异已直读。 | B-only exit→context/Manager cleanup 及其 normal timing 未闭合；whole-run source teardown 不证明 native pair recovery。 | RETIRING/poll nonzero/NEVER 继续作为候选约束。 | Pause、UI、实际 native listeners/cache 未观察。 | 可重复的 B-only retirement 与全 run own pair 回收未实现。 |

## W 状态、仍需闭合的静态边与可开始工作

W01 有界纠错完成。W02 对 95B310 的要求范围已实读完，并识别出“不足以退休 B”的确定负面依据；正常 B-only Player removal 与 A 后置仍是静态开放边。W03 的跨类别许可表和时序候选已补齐，但未采纳且未与 contract 集成验证。W04 已直读 Join pass、guard 和 create call boundary；尝试 owner 是设计候选，不是原生或生产事实。以上是本轮研究工作完成度；不等于 normal-function static completeness complete。

尚未闭合的最小静态边：识别 Main Player vector 的 B-specific normal erase caller，并证明它与 SetControllerId、UI return、context association 与 A20EB0 Manager pair remove 的先后；验证 95B310 的实际 selection branch 是否能锁定 ordinal 1；仅当有该 erase 路径时再判断 A 的 normal update 是否继续或暂停。有限下一步是从 Main Player vector append/remove 所用容器操作定位 erase caller，并沿该 caller 核对 caller guards；不需重做整函数或全网络研究。

可在不触及上述开放行为的前提下，开始自有固定布局与 ABI 声明、S2 按 native cross-category license 的纯转换设计、JoinCommand/attempt owner 的接口草案。正式 Manager registration、运行 S2、创建/发布 B、或宣称 B-only retirement 已安全之前，仍需先确定采纳哪个 publisher/contract 方案，并闭合 B 的 vector erase/关联清理后置。当前候选不能称为已集成或整体内部一致。

## 本轮唯一人工整体静态文档审阅

结果：**未通过（文档准确性/完整性）**。一次整体通读覆盖工作设计 v11、v12 报告/证据、关键 source 与 contract v2、topic README。发现报告正常前提未明确列出 Online roster 为空（只写了 Offline/local-co-op，并区分 context vector；设计 §1 要求两种空条件分别固定）；证据 E08 小节标题将 DESIGN_ONLY 拼为 DESGIN_ONLY。按要求只记录真实结果，不修补、不重审。

