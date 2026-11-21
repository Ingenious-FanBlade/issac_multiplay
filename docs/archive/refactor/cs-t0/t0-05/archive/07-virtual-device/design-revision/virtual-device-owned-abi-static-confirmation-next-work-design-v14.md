> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v14

日期：2026-10-13  
设计交付推进：`tx-01-32`  
静态执行推进：`tx-01-33`（W01–W05 属于同一完整目标）  
状态：`DESIGN_ONLY / 承接 v14 复核；确认报告 v15 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-31` 对[报告 v14](virtual-device-owned-abi-static-confirmation-v14.md)、[证据 v14](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v14.md)及[工作设计 v13](virtual-device-owned-abi-static-confirmation-next-work-design-v13.md)的只读复核。v14 找到精确 Main Player 成员删除路径，并推进了输入候选；但正常功能静态确认未完整，“仅剩普通 B Leave 入口一条缺边”不准确。旧报告、证据及其审阅通过记录保留；新报告纠正事实与设计，不修改或重新验收旧交付。

## 1. 完整目标、范围与现行约定

本轮交付 v15 报告、证据及 topic README 的实际结果。完整目标为：纠正 v14 的条件、ABI 和函数边界；补齐两种具体 Popup 后 A 的限定设备选择；完成可执行的 B 退出、context/Manager 清理与 A 继续输入的正常顺序；补齐 A 物理设备与 B 自有设备进入同一 S2 调度结果的接入；给出不会破坏原生后续调用的 Join 一次创建控制方案。已充分分析的 raw/canonical 去重与 held V2 作为继承项，只核实本轮衔接。

正常前提：分配成功、对象和缓冲有效、主线程顺序操作、Offline/local-co-op、**Online roster 为空**、B 使用唯一正设备 K、创建 B 前 Main Player vector 恰为 A。Online roster、n298+0x4B3D8 context vector、Main Player vector 独立判断。沿用[原生 Join 可靠创建并发布 B 的用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，标 USER-ASSUMPTION / DESIGN_ONLY；不重做完整 Player Init、全局 manager-this 或运行验证。

保持自有 device 38 槽、六覆写、control 六槽、S2、NEVER；首次原生 base ctor 前固定 DLL，ctor 后写自有 vptr。BASE_CONSTRUCTED 后不析构或释放对象、control 或有效基类状态；DLL 常驻。每 session 至多一次自有设备创建，失败不自动重试；B 实际创建尝试使用独立 owner。

A ordinal0、B ordinal1，ClientId→ordinal 在 session 内冻结；Player*/device*/K/平台 handle 只是当前 generation 的临时关联。A/B 使用同一 canonical、sequence、target tick、queue、dedupe、Resolve/apply 规则，无即时旁路。通用 InputManager 可以复用，C/S 不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。

正常 pause/loading/noRoom、重复 poll/query/pass、Popup、B 退休及 A 后续输入在范围内。排除 OOM/EH、worker 并发、原子镜像/计数溢出、ABA、全映像 alias 闭包、持久化、异常恢复、05-06 全网络证明。保留线程、生命周期及 Hook 恢复约束，不扩展实机操作授权。

设计交付 tx-01-32 与静态执行 tx-01-33 各在完整目标结束后仅做一次人工整体静态文档审阅；过程中零测试、构建、实验、状态模型或校验脚本，不设 gate、不校验任何哈希。W 编号不是额外推进或验收次数。

## 2. tx-01-31 复核事实与本轮修正

以下为主智能体复读的定位种子，执行者继承时标 STATIC-INHERITED / tx-01-31 复核，不能冒称自己的 IDA-DIRECT。关键条件、ABI 和接入点应复读实际指令；新增行为以本轮成功读取为准。

| 编号 | v14 问题与复核事实 | 本轮处理 |
| --- | --- | --- |
| Q01 | 报告 W02／证据 E03 把 9BA000 的 Main erase 限为 Player+0x2C==1。实际 ==2 走特殊列表，==1 有专门清理，其余值也可走到共享 Main erase 尾部；9BC120 则另有非零字段/count/index1 guards。 | W02 区分 wrapper guard、callee 分支和普通 B 构造字段，不再把这三者合并。 |
| Q02 | 9BA000 raw 使用 ECX receiver、两个栈 dword，尾部 9BA8D7 ret 8；目标在 arg_0，当前函数未读取第二栈参数。伪代码的“四参数”不能直接作为未知功能参数。Player vtable+0x28 指向 6ACB00，是移除/清理路径，不是 deleting dtor。 | W02 给准确调用表、字段分支和必要副作用；保留 ABI 栈形状，限定未读取参数的证据范围。 |
| Q03 | v14 把 B 自有设备当 A/B 共用设备，退休后留 REGISTERED、poll 非零；未交付要求的 native remove/UNREGISTERED 顺序，也未证明 A 需要该对象。 | W02 回到现行规范与 contract 的设备退休约定；分别说明 A 物理设备、B 自有设备、共享 Manager。 |
| Q04 | QueryS2 仅允许 own receiver，只覆盖 B 自有槽；A 键盘/手柄逻辑 Hook 接入未说明。退休段 Authority-pre 清 current/previous 又与 normal-post previous 唯一提交冲突。 | W03 给 A/B 两类 receiver 的同一 S2 选择和一致 writer/lifecycle 责任。 |
| Q05 | v14 E08 又把 Room guards 当作 Join 前提；实际 9551BD Join 先于独立 Room guards。6FADAC 是 9BAC40 call 返回后一条，6FA540 自身 RET 在 6FADB2。9BB393/9BB3C2 是 action14/15 的局部参数准备，实际 query call 在 9BB39A/9BB3C9。 | W01/W05 区分 Join、Room、query call、call return 与函数 return，保存连续指令。 |
| Q06 | “第二 actual create 在 wrapper 拒绝”未给控制流/返回方案；9BB415 返回 Player 后直接进入 setter，无空指针保护。n15/state12 也误称旧 Popup；它是 menu-state holder 的 +0x40 状态。 | W01 修正对象；W05 给具体阻止位置及合法 continuation，不用假 null 返回。 |
| Q07 | P01/P02/P05 把生产未写、现场未观察列成 native 缺边；P04 要求全映像旧 pointer alias，超出本轮范围。 | 按功能主题重新归类。未实现、候选未采纳、运行未知不算原生静态缺边。 |

继承无需重做：Base/Control 布局和已证 ABI，native insert/indexed/keyed lookup/remove，source pair/own key 分层，限定通知清理，六消费者/action 表，poll 和 Main Player dispatch，raw→canonical→sequence 的精确去重，held V2 初步候选。tx-01-31 已确认 418D86–418E4D 的 Main Player vector 枚举通过 vtable+0x0C 调 Player update；删除 B 后下一轮该枚举只见 A，是有边界的静态推导，无需把整条 reader 链重新降为 UNKNOWN。

## 3. 资料、工具和定位种子

开工先读现行规范、项目技能 `C:/Users/114514/.codex/skills/isaac-cs-refactor/SKILL.md`、架构计划、T0 任务计划与 cs-t0 README，再读本文、v14 报告/证据、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)及实际相关 source。旧文档中的 gate、哈希、独立角色或提前测试流程不执行。

2026-10-13 tx-01-32 工具元数据实际返回：IDA Pro MCP status=ok，auto-analysis 与 Hex-Rays ready；32-bit `isaac-ng.exe`，base 0x400000，image size 0x8FE000，输入 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe`，IDB 同目录 `.i64`。未计算或校验哈希。执行者发现当前工具及 schema，首个分析调用用 survey_binary minimal，仅使用非哈希定位信息，再看实际服务状态；工具不可用时保留具体失败和已读终点，继续 source/候选工作。

| 主题 | 有限种子与读取用途 |
| --- | --- |
| Popup/菜单/重选 | 95B310；95B476、95B720 两次 descriptor；9C25E0、9C01D0、9C0960、9C1260、9C14C0、9C34F0、9C3990；95B815–95B843 n15/menu 条件；95B867、95B874/95B87F、95BAC2/95BACC/95BBB6、95BCA0。追特定 callback/payload writer 和具体重绑分支。 |
| Join/Room | 954CD0 的 955190–955238；6FA540 的 6FADA7–6FADB2；9BAC40 的 9BB380–9BB440；9B9CD0 的 9B9E63、9B9EAD。只核关键正常路径与调用边界。 |
| B 成员删除 | 7A1090 的 7A10CE/7A1F63，9BC120 的 9BC1FB–9BC1FF；9BA000 的 9BA02B/9BA030、9BA09A–9BA190、9BA459、9BA5D2–9BA6B8、9BA8D7；9BA8F0 的 9BA95C–9BA962。 |
| 清理/下一 update | B6BDD0 deleting dtor 与 B6BDF8 +0x28 区分；6ACB00、757AC0、83BC50；4186C0 内 418D86–418E87 Main vector reader。重点看 A selector/HUD/关系和正常调用阶段，不做全对象遍历。 |
| Manager/context | A20EB0、A212C0、913760、90AC70/90B010、90AF30/90B070、417870；沿 v10 E04/E08 已证 pair、通知及 -1 fallback，只补本轮缺的先后与 A 结果。 |
| 当前输入 source | `cs-t0/native/include/isaac_cs_t0/input_core.hpp:65/67/198`；`input_core.cpp:344/388/467`；`active_dispatcher.cpp:62/265/371`；`active_runtime.cpp:489/527/553/584/653/709/839/908/937/986/1036/1354/1394`；`active_witness.cpp`。行号为种子，按当前文件定位。 |

普通 B 的构造字段另有[历史限定适用性说明](../../01-passive-lifecycle/native-exit-applicability-gate-v1.md)及其引用的 `cs-t0/coop-native-exit-applicability-static-v1.json`。它们仅提供 ordinary Main Player Variant0 与旧 caller 非零 guard 的历史静态事实和指令种子；不要执行旧 gate、校验脚本或哈希。复读必要构造调用参数及字段 store 即可，不重新反编译完整 755470 Init。

## 4. 同一目标内的工作内容

### W01 — 两个具体 Popup 与 A 的下一次查询

追 95B310 两次 descriptor 的 callback/payload/flags 具体 writer、9C25E0 默认值和 9C01D0 复制，区分空 callback、静态函数地址和运行 payload。若 writer 已能证明回调为 null/固定目标，应给结论，不能继续笼统称“callback 值现场未知”。如仍缺，列明确 store、字段、caller/目标地址和未读分支。

修正 n15 是 menu-state holder：+0x40==12 及其列表/98AA30 操作，不与 Popup+0x04 state 混淆。逐层给 Popup 存活、action15/timeout、state3、完成移出列表、下一次 busy/Join 检查的顺序；Room 独立 guards 另列。

交付五场景结果表：①Popup 存活未完成；②完成/取消后；③B 自有设备 remove 后下一 reselect；④A 的 K/物理设备重绑；⑤B 退出后 A 下一 Room/Player/query。各行列 Main/context guard、menu 与 Popup 的对象/ID/state/flags、实际 callback、Join/Room/Player 到达、A key、Manager/context 选择、查询 receiver、neutral/fallback/正常读者结果。受 context 空/非空制约时分支写清，不从 Online roster 空推导 context 空，也不把 setter(-1) 当已重绑。有限接到已证 bool/scalar/query helper，不追全部游戏 consumer。

### W02 — 普通 B 可用的精确删除、设备移除和 A 继续

先区分 7A1090/9BC120 的受限 caller 与 9BA000 的真实 callee ABI。用 raw 列 ECX、两个栈参数、目标、return/ret8、实际参数读取；复读字段 ==2/==1/其余分支到 Main erase 的关系，以及普通 B 初始字段的有限构造证据。不能把旧 caller 不接收 Variant0 推成 callee 也不能处理普通 B。

从已证 member erase 深读必要的 757AC0、+0x28→6ACB00、selector reset、83BC50 和关联列表清理，说明它是完整移除 helper 还是仅部分行为、哪些 A 状态会被刷新及下一 reader 如何恢复。需要顺着结果再读时限定 4–8 条相关 caller/callee 路径；记录实际区间、选择依据和未读终点，避免重复 whole-vector clear 或全 alias 研究。

不强制寻找原版 Leave 按钮。优先给自有 `RetireParticipant(B,slotGeneration)` 主线程请求调用已证 helper 的一个候选，选定准确正常阶段并用调用栈/原始顺序证明该阶段不处于 Main Player 枚举中途、不尚有当前 B consumer original 未返回。若普通 B 不满足 callee guard 或 helper 正常副作用使目标不成立，给具体反例/字段，而非“safe boundary 未知”一句结束。

设备角色固定解释为：A 保留原生键盘/手柄及相应暂时关联；B 自有设备拥有自己的 K/object/control；两者使用同一通用 InputManager，不据此变成同一个 device。保留现行退休目标：B 输出中性、B 解绑或退出、B 自有设备从 Manager vector 移除到 UNREGISTERED，内存和 DLL 常驻。不能用“B-only 不属于设备退休”删掉 remove 要求；也不改 contract 或项目规范。

形成一条明确顺序：关闭 B Join/input admission → 失效 B generation/visible publication → 已选主线程阶段处理 B Main membership/ordinal/关系 → context/listener/selector 清理 → 原生 Manager remove/通知 → 确认不再登记 → 下一 poll/reselect/Room/Player/query 的 A 结果。实际 native remove/通知顺序以指令为准；必要时调整候选事件排序，解释为什么不会错误清 A source pair 或让 A fallback 读旧 B。A 的 K 不被 helper 修改与 A selector 被刷新是不同后置，均应说明。

交付 ABI/分支表、所选 coordinator 时序、A/B 容器与状态前后表、下一 reader 表。已删除 B 后 Main-vector 不再 dispatch 是已证有限结论；需要中性处理的迟到查询只在已知逻辑入口和 generation 范围设计，不以全映像 alias 闭包作为工作。

### W03 — A 物理查询与 B 自有查询共用 S2，统一 writer

沿 v14 的单一候选推进：冻结 ClientId→ordinal，每人独立 queue/sequence/replay/dedupe/planner/pending，共享 Authority tick；每 slot 每 tick 至多一次破坏性 Resolve，不等待全员。A 物理输入先 raw→canonical→Submit，B C/S command 按同一语义接受；物理采样、B 输出设备 poll、Join control lane 和 gameplay apply 分开。

补全两条接入：A keyboard/gamepad 的现有 `InvokeLogicalBool/Scalar` 及 wrapper；B 自有 +0x30/+0x34/+0x38。两者最终读同一 ParticipantSlot S2 API，分别校验物理 receiver/current binding 和 owned receiver/current binding。A 不通过 B own-object 条件，不为接入 S2 把 A key 改成 B K。原始硬件采样须保持读取原物理结果，不能把已经合成的 S2 再提交成新输入。

Player TLS 在 original 前进入；consumer TLS/witness 在 consumer original 前进入；query 在 original 内返回前应用 S2；original consumer exactly once。明确每类 query 的 original 是否调用、调用次数、参数/返回 ABI 与恢复 owner：原生物理查询 wrapper、owned query 实现、Join read、UI/outside gameplay 各不混用。按 v8 真实 cross-family action 表给 bool/trigger/scalar结果，替换当前单 A/strict-family adapter 的部分准确列为新增实现。

唯一时序保留 Authority-pre 写当 tick current/state-or-neutral、normal Authority-post 提交 previous/关闭 visible，owned +0x78 仅返回 native 活跃轮询条件，不写 S2。退休不能再在 pre 擅自同时写 previous：选定 generation/visible 失效使查询中性而不提交旧 tick，或明确独立 lifecycle reset 的唯一 owner、时机及与 normal post 的互斥。只交付一套内部一致的工作候选；与 contract v2 poll latch writer 的差异继续标 DESIGN_ONLY/未采纳，不修改 contract。

手工状态轨迹列 A→B 与 B→A、缺 B 输入/future、同 tick 重复 query/poll、noRoom 第一次已 Submit 而无 Resolve、第二次正常缺 Authority 后 pause 判定、loading/pause 恢复、B 退休后 A 继续。各步包含 Authority tick/updateEpoch、两 queue/pending、slot/binding generation、current/previous/visible、TLS、返回动作和 commit；noRoom 不自动等于禁止 Submit，updateEpoch 不等于 Authority tick。A/B 正常遍历切换只恢复 TLS，不清另一人的 queue/generation。

交付 A/B query-route 表、最小 API/字段/owner 表、唯一 writer 时序、source 修改责任和上述手推轨迹。未安装的新 Hook、未创建的设备及缺运行观测不列成 native 缺边。

### W04 — 继承去重与 held V2，限定衔接核实

v14 E05/E07 的 raw/canonical/sequence 与 held V2 作为 STATIC-INHERITED：当前 v1 action8–11 只存 rising，raw axis 改变可能 canonical 不变，同 tick/source、无 edge、persistent state 相同在 sequence++ 前去重。V2 held bit0 Shoot、bit1 Item9、bit2 Card/Pill10、bit3 Drop11；edge bits0–3仍对应8–11；Bomb8只 edge。这些不因生产尚未写而重新成为原生 ABI 未知。

只补 W03 用到的 V2 receiver/action 结果：首次 history 无合成 rising，但 held level 是否可见；9–11 press/hold/release/repress；同 canonical 量化轴；跨 participant/generation 重置；重复 Trigger 返回与每 tick Resolve 消费的区别。手工写表，不运行模型。更新 Validate/Encode/Decode/Queue/Planner/adapter 的修改责任及版本兼容影响，保持 DESIGN_ONLY，不改 schema/source。不重新跑完已有去重轨迹或列独立验收批次。

### W05 — Join 一次实际创建的控制点与合法后续

纠正完整 raw：9BB393/9BB3C2 参数准备，9BB39A/9BB3C9 query call；9BB3DB stored confirm 清零；9BB415 actual create；9BB41A–9BB429 无 null guard 的 bind/spill；9B9E63 ordinal、9B9EAD end推进。6FADA7 call9BAC40，6FADAC 为该 call 后续/内部 pass close，6FADB2 才是 6FA540 RET。若选择包装9BAC40、6FA540或外层callsite观察，分别写正确 entry/return/ABI，不能混写一个地址。

给一套具体的一次创建阻止方案，优先在确切 native admission/action 分支排除第二次 create；同时处理 stored confirm、非 lane 原生输入、另一 K、同 update 第二 pass 等可能到达 create 的来源。若选控制 wrapper 或分支 continuation，必须复读相关正常路径并列 owner、ECX/stack/保留寄存器/返回值、跳过或调用的原生范围和接续地址。观测 wrapper original exactly once；控制行为若改变 pass 到达，明确它是新增控制设计，不能冒称纯观测。禁止从 create wrapper 假返回 null 让 native setter 使用，也不能仅写“拒绝 AlreadyAttempted”而不交代原生后续。

OwnedDeviceCreateOwner 与 BCreateAttemptOwner 独立。实际 call 开始前唯一 BeginAttempt；9BB429 仅绑定观察；在选定正常 pass close 和用户给定发布后置确认后才 Publish/Succeeded。无 actual create 的 cancel/navigation/guard fail/no pass 只关闭 command；actual attempt 开始后 bind/publish失败终结、不自动 retry。JoinCommand 的 ClientId/K/session/slot generation/updateEpoch/passToken/action 可见范围、关闭、重复 read 不消费、过期后不重放，与 W03 activation 一致。

交付实际 Hook/控制点与恢复 owner 表、状态机、合法 continuation、有限轨迹：首次19+14、导航/取消、重复query、同update第二pass、no pass、另K、guard fail、actual call后正常后置不满足、成功后再次confirm、retire与publish pending。若某方法不满足，明确排除原因并设计可工作的替代；工具缺失时列有限未读地址，不扩大到完整 Join/Init 研究。

## 5. 交付文件与完成口径

- 报告：`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v15.md`。
- 证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v15.md`；必要分册使用同 v15 前缀并全部引用。
- topic README：增加 tx-01-33 实际结果，保留 tx-01-31 复核、本文及所有旧记录。

开头分别回答 Q01–Q07、W01–W05：目标满足、部分完成、候选被排除、工具未读终点；“研究完成”不等于功能静态确认完整。关键证据写对象、VA/RVA、实际 raw 区间、call/return、ECX/栈/返回通道、guard、writer/reader、后置和限度。函数输出截断、disasm cursor 未覆盖目标区间、错误调用须实记，不能用工具 done 代替读到该分支；可用 get_bytes、IDA Heads 纯静态指令读取补足。

P01–P06 继续按注册枚举、type/name/binding、Offline Join/Popup、B key/ordinal/退休、snapshot/query、pause/context/lifecycle 六个功能主题，各自分列 native 静态已证/真正缺边、DESIGN_ONLY 差异、现场未观察、生产未实现、后续整体验收事项。本轮候选未采纳不是 native ABI 缺边，production 新 API 不存在不是待逆向函数，全映像 alias 不再进入本轮必做列表。

结尾给可直接开始的基础实现、内部完整且待采纳的候选、有限未读原生行为、后续具体 source 修改责任。不要因剩余功能链把已证 layout/construct/vtable/control/registration 降回未确认；也不要把基础可实现表述成全功能已证。剩余静态项必须包含本轮新增事实及未读的具体字段/分支/函数，避免再次把全部链列成 UNKNOWN。

## 6. 分工、允许操作与唯一整体审阅

本文交付后以新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行 tx-01-33；不恢复已完成的 v14 实例，不再委派、不创建侧栏聊天。主智能体交出 v15 文件、topic README 编辑权与 IDA 只读会话使用权；执行期间不并发编辑或操作同一 IDA。

只允许静态 IDB/file/source 读取和指定新文档/README 编辑。不改旧报告/证据、工作设计、项目规范、contract/schema、production source，不 patch/rename/set_type/define_func/save IDB，不用 debugger、进程/游戏内存、附着/断点/单步、游戏输入、注入或安装 Hook。纯静态 py_eval 可读取指令/交叉引用，不能执行状态模型。不得运行测试/构建/harness/probe/smoke/canary/verifier/hash 或链接检查脚本。PowerShell 可用 login:false。

tx-01-33 全部分析和编辑结束后，一次人工整体通读本文、v15 报告/证据/分册、相关 source/contract 与 README，统一审阅：Q/W回答、ABI/raw边界、两个Popup→A五场景、普通B helper适用性/副作用/调用阶段、remove通知及A下一查询、A/B S2接入和writer/lifecycle、继承held衔接、Join第二create合法阻止与pass终态、事实分类和引用。审阅前允许修正文稿；唯一最终审阅后只回填结果，失败如实列项，不自动修补、复审或换编号重复验收。

分别报告文档准确性、正常功能静态完整程度、候选一致性及采纳状态、现场未观察、生产未实现。不得预填 PASS，不以 UNKNOWN 标签齐全代替目标完成。工具中断不伪造读取；继续不依赖工具的工作，最后交付已完成结果和具体剩余。

## 7. tx-01-32 设计交付记录

本次新增本文，在 topic README 记录 tx-01-31 复核与 tx-01-32 设计入口；静态事实沿前次直接复读，工具元数据已确认。未运行测试、构建、哈希校验或游戏操作，未修改旧交付、source、contract/schema 或 IDB。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文与 README 修订范围，对照现行规范、v14 复核、contract 及相关 source，核对 Q/W 对应、ABI/函数边界、Popup 五场景、B 删除与 Manager remove/A 下一 reader、两类 receiver 的 S2 接入与 writer/lifecycle、held 继承、Join 第二 create 的合法控制流、功能分类、引用及授权范围。审阅后仅回填结果，无第二次审阅；不代表 tx-01-33 执行或正常功能已确认完整。
