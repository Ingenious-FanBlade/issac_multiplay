> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v14

日期：2026-10-13  
推进：tx-01-30；W01–W05 是同一项完整推进中的主题，不是额外验收次数。  
入口设计：[下一步工作设计 v13](virtual-device-owned-abi-static-confirmation-next-work-design-v13.md)  
证据：[IDA/source 静态证据 v14](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v14.md)

**结果：** W01–W05 的静态分析、候选接口与 v14 文档已完成。normal path 得到实质补证，但仍有一条具体 native 缺边：Main Player 有逐成员删除函数，现有 caller 受 Player state/context guard 限制；当前正常场景没有证明普通用户 B Leave 请求必然抵达该入口。本轮不把这个条件候选描述为普遍可用的产品退出功能。W03、V2 held 与 W05 完整 owner 流程属于 DESIGN_ONLY，未采纳、未实现；contract/schema/source 均未改。

## 范围与证据口径

正常前提沿用入口设计：有效分配/对象/缓冲，主线程顺序调用，Offline/local co-op，Online roster 为空，唯一正 K，创建 B 前 Main Player vector 恰为 A。Online roster、n298+0x4B3D8 context vector、Main Player vector 是三个独立容器/事实，不互推。C/S 的 ClientId→ordinal 在 session 内冻结为 A=0/B=1；Player*、K、device pointer 与平台 handle 都只是当前 generation 的临时关联。通用 InputManager 可以使用；C/S 候选不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。

维持 38 个 device slots、6 个 overridden slots、6 个 control slots、S2 与 NEVER。自有 DLL 在 base ctor 前 pin；base ctor 后写自有 vptr；对象进入 BASE_CONSTRUCTED 后不析构、不释放对象/control/有效 base 状态；模块常驻。每 session 自有 device 创建最多一次，失败不自动重试。B 的 native create attempt 有单独 owner，不与自有设备创建共用标志。

证据标签：IDA-DIRECT 是本轮只读 IDB 函数/raw；SOURCE-DIRECT 是本轮当前 source 阅读；STATIC-INHERITED 仅表示 v8/v10/v13 已有静态结论；CONTRACT 指 contract v2；USER-ASSUMPTION 指用户规划给定的正常前提；DESIGN_ONLY 指尚未采纳的候选。live values、运行验收和生产行为不由静态地址推出。

保留旧 v13 报告、证据和 README 历史，未修改、未重审旧交付。此次未改 IDB、生产 source、contract、schema、旧报告和工作设计。

## W01 — Room 条件、raw 勘误、Popup 对 A 的具体影响

在 sub_954CD0 case 2 中，IDA-DIRECT 顺序为：VA 0x9551A9 查 9C3990 Popup busy；Popup 不忙才到 0x9551B4 调 9505E0；条件允许才在 0x9551BD 调 6FA540 Join。随后 Room 有独立两级 guard：byte[n298+0x4B130] 必须为 0；VA 0x9551CF 对 byte[n298+0x4ABBC] 测 bit 0。raw F6 87 BC AB 04 00 01 75 60 表明 bit 0 置位跳到 0x955238 替代块；只有 bit 0 清零才在 VA 0x955231 正常调 6FADC0。case 1 的另一条 9897D0 路径保持分开。

因此，Popup busy 只跳过这段 Join precheck，不足以说明 Room 被跳过；Room 必须同时通过独立 byte guard 和 bit-0 guard。byte[n298+0x4ABBC] bit 0 为非零时绝不能写成“进入 6FADC0”。本轮连续重读 0x955190 附近 raw，完整保留 9551CF 的指令起点和 9551D6 分支。

Popup 对 A 的具体有限链如下。954F7D poll/update 后，954F84 调 95B310 扫描 disconnect/reselect 状态，954F8F 调 9C34F0 更新 Popup manager，之后才进入 case 2 的 busy 检查。95B476 创建第一 Popup descriptor；95B4C0 将 9C39E0 返回的 Popup ID 保存到 n298 int index 76530。缺失 selected device 时 95B720 创建第二 Popup，95B806 插入、95B811 保存到 index 76531。两处值都是 Popup ID，不是 K 或 Player identity。

9C25E0 在 descriptor 首 byte 上初始化 flags，VA 9C26D6 OR 7；9C01D0 的 Popup ctor 又对 Popup+0x24 执行 OR 7。9C3990 只在仍登记的 Popup list 中寻找 Popup+36 bit 2。9C0960 处理状态 2 的用户动作；action 15 或正常 timeout 会先写关闭 sentinel、调用非空 +0xA0 callback，再进入 state 3。9C14C0 等关闭动画/子对象达到完成条件后向 9C34F0 返回完成；manager 从列表移除 Popup。关闭 callbacks 的具体指针和现场值没有被静态固定成 A 输入结果。9C2DB0 是 descriptor 12 个字段的追加/拷贝，未称作 flags normalize。

缺失设备 Popup 随后怎样改 Player，受 context vector 分支制约：context empty 时 95B874 按当前 v91 取 Player，95B87F 调 SetControllerId(Player,-1,1)；context 非空时 95B867 改走 90AF30(context,-1)。只有前一分支且 v91 实际指向 A（例如 vector 恰只有 A 且 v91=0）才证明 A 的 Player+0x1618 被置成 -1，并且 propagate=1 会更新关系对象。Online roster 为空不能推出 context empty；此链不证明 A 已绑定到指定 K 或其后续 native fallback 选中了哪个设备。STATIC-INHERITED v10 E08 已指出 key=-1 的 helper 可能进入 context-empty all-active-device fallback。

## W02 — B-only 原生 member cleanup 与下一 reader

本轮沿 7A1090 的两个 caller 到 sub_9BC120，再到 sub_9BA000，找到一条实质的逐 Player Main-vector 删除序列。IDA-DIRECT 的 sub_7A1090 callsites 在 0x7A10CE、0x7A1F63；第一个由 current Player+0x2C==1 且 caller 参数为 0 的分支到达。ECX 是 n60+0x1BAA8 Main Player vector，目标是明确 Player pointer。9BC120 要求 target Player+0x2C 非零、vector count>1，并从 index 1 起找 exact target pointer；命中后调用 9BA000。它不会把 index 0 的 A 作为目标。

9BA000 的 Main Player 分支要求 target+0x2C==1；它从 vector 删除 exact pointer、end 减 4、调用目标 Player vtable+0x28 cleanup/dtor slot、将目标 ordinal（Player+0x161C）置 -1，并重排其余 Player ordinal。对 [A,B] 删除 index 1 的 B 后，A 仍在 index 0/ordinal 0。该 helper 不调用 SetControllerId(-1)，不执行 InputManager A20EB0 device remove，也不是 whole-vector clear。因此 native member erase 是真实 B-only exit 机制，但不是 Player key 清零。

缺边仍然具体：用户给定正常前提没有保证 Player+0x2C caller guard，也没有将普通 B Leave/Popup action 连接到 9BC120 的其中一个 caller。故不能宣称任意正常 Leave 命令已闭合。第二候选 9BA8F0 在 context vector 非空时逆序扫描 Player；经过 relationship guard 后由 90AC70(context,Player) 找不到 membership 才调用 9BA000。空 context 直接 return。若 context 仅有 A row 且 B 缺 row，B 可能被移除而 A 保留；Online roster empty 不等于该 context 内容。

Room 后续也实质阅读：955231 调 6FADC0；6FBA4D 调 802980，6FBA52 调 9BB5D0。9BB5D0 遍历当时 Main Player vector、按 guard 处理 Player 状态和 508/252 room state；其函数体不 erase Main Player、不写 Player+0x1618。它不是 B exit helper。

可继续评审的 DESIGN_ONLY B-retire 顺序是：按 ClientId/slot generation 关闭 B 命令 admission，Retire B 的独立 queue 并清其 planner/pending；Authority-pre 发布 B 明确 neutral 并失效旧 B publication；在 main-thread safe boundary 验证 vector 恰为 [A,B] 且目标是 index 1、目标类型命中 Main Player 分支，再通过已证的 exact-member native cleanup 移除 B；之后确认 vector 只剩 A、A ordinal/key 不变。B 退休不是 shared device 全 session RETIRING：owned device 留在 REGISTERED 并以 +0x78 返回 native common 所需非零 gate，以便 A 继续工作。

若 native 某个旧 consumer/query 仍持有 B pointer，则自有设备 +0x30/+0x34/+0x38 必须新增 query-level generation 检查：按 consumer TLS 身份对 retiring/stale B 返回 neutral，不 fallback 到 A，不将 B key 设 -1，不跳过整个 consumer，也不直接改世界字段。现生产 source 尚无此 ParticipantSlot query owner。旧 Main-vector reader下一轮不再枚举被删 B；该读者依据 STATIC-INHERITED v8 E03/v10 E07。whole-run teardown 是另一条路径，需关闭两位参与者与 session admission 后处理 shared Manager device，并保持对象和 DLL 常驻；不借用 B-only erase 或 Online all-player reset。

实施前尚需一个有限静态点：对被选择的普通 Leave 来源确认哪个 7A1090 caller/Player state 满足入口 guard；直接请求原生 cleanup时确认 9BA000 的第四参数和 safe boundary。若没有这样的既有入口，增设 main-thread RetireParticipant(B,slotGeneration) coordinator request，调用经 ABI/guard 证实的 selective cleanup，并配合 owned query neutral guard。不能把 7A6450(-1,0)、一个 neutral writer、Manager remove 三者并列当完整 B 退休。

## W03 — ClientId 分流、participant 队列、witness 与唯一 S2 publisher

SOURCE-DIRECT 已证 InputCmdDraftV1 含 client_id（offset 16）；InputQueue 绑定单 client，WrongClient 会拒绝另一身份；queue、dedupe/replay history、EdgeTargetPlanner 和 ActiveDispatcher 都是一人实例；Resolve 会消费/删除命令并推进 last_resolved_tick，同 tick 不可重算。当前 source 只有 ordinal 0 与一个 g_dispatcher/g_adapter。不能把 A/B 两 client 直接塞进这个 Queue，也不能让 B 的 poll 成为硬件输入或另一次 SubmitPhysical。

本轮只保留一个工作候选：Authority-pre 是每个 participant 当 tick current 的唯一 publisher；normal Authority-post 提交 previous=current 并关闭可见 scope；owned +0x78 poll 不锁存 S2、不 Resolve。详细 owner/API/字段、participant 队列、流程表及同序/反序/缺输入/future/noRoom/loading/pause/B retire 手推轨迹见证据 E06。冻结 A ClientId→ordinal0、B ClientId→ordinal1；每人独立 InputQueue、sequence/replay history、dedupe history、edge planner 和 pending。共享 AuthorityCoordinator 只统一 session/apply generation、tick/epoch；对每个有效 participant 分别 Resolve 一次，不等所有参与者到齐。

三层严格分开：Player wrapper 在原生 Player Update 前建立 participant TLS；consumer wrapper 在原生 consumer 前建立 consumer TLS 与 witness，调用 original exactly once，再读 after witness 并关闭 scope；原生 consumer 内的 device/query override按同一 participant S2 current/previous 返回数据，结果在其后续世界动作前生效。当前 InvokeLogicalBool/Scalar 先调 logical query 原函数再 override query 返回，不表示可以在整个 consumer 已执行之后读快照。

现有无输入不是候选的 explicit neutral：SyntheticAdapter 没有 snapshot 时，pending target 未来才 suppress-neutral，其它情况可能 passthrough。因此候选每个有效 participant 的每 tick 都显式发布 state 或全零 neutral，不能将 no input、no Room 或不进入 Authority 混成同一事件。重复 +0x78/query 不写 publication、不重新 Resolve、不消耗 edge；重复 Trigger query仍能读到相同 true，不宣称任意原生世界副作用 exactly-once。

和 contract v2 §4 的差异明确为：contract 目前规定 +0x78 是 current/previous 唯一 latch writer；候选把 current/neutral 唯一写者改成 Authority-pre、previous/close 改成 normal post，而 +0x78 只回 native nonzero gate。contract v2 未改，候选未采纳。+0x78 native bool gate 的 ABI 事实沿 STATIC-INHERITED 旧设备 slot 证据，不把它与 S2 publication 混用。

native consumer/action 许可沿 STATIC-INHERITED v8 E03：Movement 0–3/11；Shooting 4–7/0–3/11；Bomb 8；Item 9/10/11；Pill/Card 10/9/11；Drop 11。候选按真实 consumer/action 分支，而不是当前 adapter 的 strict ConsumerForAction 单类别门。A↔B 正常遍历只替换/恢复 TLS slot，不触发 AdvanceOwnerGeneration，不清另一 participant 队列。

## W04 — raw→canonical→sequence 去重和 9–11 held 候选

current BuildRawSample 将键盘/手柄投影为四个轴差，aim 非零才编码 Shoot held；action 8–11 只把 projection history 的 rising 写进 edge_flags，完全不写这些动作的 falling，也不为 action9/10/11设置 held。UpdateProjectionHistory 首样本/换 key 不从先前 level 造 edge；后续仅 0↔非0生成 rising/falling，然后更新history。CanonicalizeRaw 量化/归一化，因此 raw axis 改变可保持同 canonical axes。SubmitPhysical 的精准去重条件是 edge==0、同 completed tick、同 source、SamePersistentState相同；抑制发生在 sequence++ 之前。其它样本分配 sequence，planner 给 target=max(observed tick+2,相应edge前target+1)。

纸面 v1 轨迹见证据 E05；结论涵盖首history样本、重复稳态poll、action9 press→release→repress、Shoot release、量化相同轴、真实canonical轴变化、跨completed tick和source/generation切换。action9按下的edge可让v1提交，但held一直为0；同tick无edge release可因持久canonical状态相同而被抑制；跨tick release仍提交因为tick条件不匹配。raw变化本身不保证canonical状态变化或新sequence。

持久按住候选不改source/schema：InputStateDraftV2、长度仍24、state version=2、held valid mask=0x0F；bit0 Shoot、bit1 Item9、bit2 Card/Pill10、bit3 Drop11；edge mask=0x0F，bits0–3仍对应8–11，action8 Bomb仅edge。BuildRawSample加9–11 raw level到held；release清held；Validate/Encode/Decode/Queue/Planner升级到V2 mask/version。ActionMagnitude 的9–11改读held，ActionTriggered的8–11仍读rising edge。这样同tick release作为persistent state transition不再被旧V1去重吞掉。所有改动仅为 DESIGN_ONLY。

## W05 — JoinCommand/pass 与 B 实际 create attempt 的一次性 owner

native顺序静态证据见E08：action14 query位于9BB393，stored confirm bit于9BB3DB清除，actual Player create在9BB415调用9B9CD0；之后9BB41C读取selector K，9BB41F让ECX指向Player，旧 raw 摘录遗漏的9BB421是89 45 F0，9BB424调用SetControllerId(K,1)，9BB429仅是setter return。create callee在9B9E63设置ordinal并在9B9EAD（不是9B9EAF）推进Main Player vector end。sub_6FA540于6FADA7调用9BAC40，6FADAC是整个Join pass normal return。

两个 owner 分开：OwnedDeviceCreateOwner 是每session一次自有设备对象创建；JoinAttemptOwner 独立记录 B native actual create。JoinCommandLane 依 ClientId 接受命令，给 target K、updateEpoch、pass token 和动作；Query只是读取此lane值，不承担 native side-effect exactly-once。只在实际9BB415前 BeginAttempt；return取得Player后在9BB429之后观察绑定；确认vector membership/ordinal1/K正确才进入 Bound；6FADAC normal close且 B 已完成USER-ASSUMPTION的publish postcondition后才进入 Succeeded。actual call 已开始但 bind/publish失败则 FailedFinal，不自动retry；第二actual call永久拒绝。

无 create call 的cancel/navigation/no pass/guard fail只关闭 JoinCommand pass，B attempt可留在NotAttempted；第二 pass不重放旧passToken。另一K不匹配。成功后再confirm不创建第二B。Retire发生于B发布pending时要使slot generation失效并中性、关闭admission，丢弃pending旧publication并使attempt进入terminal retired/failed，不能再成功或retry。所有这都属 DESIGN_ONLY；可靠B创建/publish继续沿USER-ASSUMPTION，本轮不重做full B Init/OOM/EH/full 05-06网络。

## P01–P06 功能主题及完成边界

### P01 — Register / enumerate

- **Native 已证：** STATIC-INHERITED v10 E04 的 InputManager active pair vector、n298+0x4B3D8 context vector、Player vector、四selector slots是不同容器；normal Manager remove 与 notification callback清理边分别存在。本轮直接连到 context reader和Player成员erase。
- **Native 真正缺边：** 当前自有 Base device 和目标 K 是否已有可用的 native keyed registration/reader闭环；没有把普通B Leave意图接到准确 9BC120 caller guard。
- **DESIGN_ONLY 差异：** Session层固定ClientId→ordinal map和owner generations；B退役只删除其Player member，不清共享Manager vector或A。
- **现场未观察：** Online roster、context members/listeners、Manager pair和Player vector实际内容均未知。
- **生产未实现：** 当前production source没有将自有ABI登记到目标 native keyed path。
- **后续整体验收事项：** 实现后观察单设备登记、A/B路由隔离、删除B后Main vector只剩A且共享A资源仍有效。

### P02 — Type / name / binding

- **Native 已证：** 7A6450写Player+0x1618 key；9BC120/9BA000按Player对象与Main vector member操作；9BB41C的K和9BB421 spill Player*分属不同身份。contract v2保存38/6/6、S2与NEVER约束。
- **Native 真正缺边：** 实际自有设备vptr、K绑定、C/S identity到 native query receiver的生产reader桥接尚无运行证据。
- **DESIGN_ONLY 差异：** ClientId恒定；Player*/K/device pointer只在slot/binding generation有效。A/B各自S2由TLS决定。
- **现场未观察：** 实际K、device pointer、Player*、ordinal、base ctor结果、vptr与buffer长度未知。
- **生产未实现：** 自有设备创建/绑定代码未接到native normal path。
- **后续整体验收事项：** 检查pin/vptr顺序、38槽/6覆写/6 control、有效缓冲、一次创建、对象常驻及key-query目标一致。

### P03 — Offline Join / Popup

- **Native 已证：** Popup descriptor和对象flags初始OR7、bit2 busy reader、state3 callback/关闭/列表移除顺序；Room两个独立guard；Join 9BB415 create→K bind→pass close顺序。
- **Native 真正缺边：** Popup真实callback target和值；case guard及用户命令现场值；用户pass何时会有第二次query不能靠query次数推成exactly-once。
- **DESIGN_ONLY 差异：** 独立JoinCommandLane/pass token/updateEpoch和每session一次B attempt owner；发布后置确认才成功。
- **现场未观察：** 当前Popup flags/callback、Selector K、pass数、B实际创建/发布结果未知。
- **生产未实现：** 没有JoinCommand、pass close或attempt terminal gate。
- **后续整体验收事项：** 覆盖第二pass、导航/取消、无pass、不同K、guard fail、成功后二次confirm和创建后发布失败。

### P04 — Key / B retirement

- **Native 已证：** 9BC120从index1起匹配目标并调9BA000；9BA000删除精确Main Player成员、调用cleanup并重排ordinal。9BA8F0可在非空context下作条件 member cleanup。
- **Native 真正缺边：** 普通用户B Leave命令到9BC120 caller guard的正常入口；9BA000第四参数/safe boundary；native member delete后的所有外部旧pointer alias未全映像闭包。
- **DESIGN_ONLY 差异：** per-slot neutral/代际失效、B专属队列retire、main-thread exact member remove、owned query层stale B neutral；禁止用key=-1 fallback。
- **现场未观察：** B key/context row/关系与vector membership、真实退出时序均未知。
- **生产未实现：** B-only retire coordinator、query neutral guard和terminal state不存在。
- **后续整体验收事项：** 让B正常退出后A仍ordinal0继续输入；确认旧B query为中性、不读A或旧B publication、不产生B副作用。

### P05 — Snapshot / input S2

- **Native 已证：** normal device poll及Player consumer/query链沿STATIC-INHERITED v8/v10；source单队列、WrongClient、破坏性Resolve、有限去重为SOURCE-DIRECT。
- **Native 真正缺边：** native Player层的二参与者query选择与自有device S2 publication尚不存在。
- **DESIGN_ONLY 差异：** participant queue/sequence history/planner/pending分区；Authority-pre current/neutral，normal-post previous/close；+0x78只回true gate。
- **现场未观察：** actual poll/query时序、Authority tick、A/B action与输入值未知。
- **生产未实现：** 单ordinal0 dispatcher/adapter未扩展为per-slot publication。
- **后续整体验收事项：** 覆盖A→B与B→A、缺输入、future、重复poll/query、noRoom先submit无Resolve、pause/loading恢复与跨family action。

### P06 — Pause / cache / lifecycle / retirement

- **Native 已证：** 本轮Player cleanup会移除单Main member并更新ordinal；v10已证Manager normal pair remove与913760通知有各自对象/guard。
- **Native 真正缺边：** 每个B退休成员删除与context/Manager共有device状态的完整正常先后；未知别名、异常/并发不在scope。
- **DESIGN_ONLY 差异：** slot generation独立失效，不用全局OwnerSwitch清另一participant；B退役与whole-run teardown分开。
- **现场未观察：** pause/loading时的Player/context成员、retire开始时机、真实cleanup callback值未知。
- **生产未实现：** per-slot lifecycle、B final retired状态与恢复连接不存在。
- **后续整体验收事项：** pause/loading/noRoom恢复、B retire/A继续、whole-run teardown、Hook恢复、对象不释放与DLL常驻。

## 实际状态与后续可开始工作

W01、W03、W04、W05 已形成一套明确静态结论/候选，不表示source、schema或contract已实现。W02 找到精确 native per-member removal与条件context清理路径，但普通Leave caller guard仍缺；实施者只能在这条边得到证实后称作任意正常B退休已闭合。

可按设计先准备FrozenParticipantMap和per-participant data model；正式接入Authority-pre/post之前须处理contract v2唯一poll writer差异；held 9–11必须走V2 codec/state变更评审；B retirement需先把普通request来源与9BC120 caller guard、安全调用boundary接上；Join创建必须使用独立AttemptOwner并等bind与用户规划的publish后置。不得将可准备实现材料描述为已经生产接入。

本轮没有运行测试、构建、harness/probe/smoke/canary/verifier、hash/链接检查或可执行状态模型；只做静态阅读和三处授权文档编辑。唯一整体人工静态文档审阅已通过：W01–W05、P01–P06、IDA/source/contract v2 与 README 表述一致；W02 普通 Leave→caller guard 缺边、contract v2 与 W03 publisher 候选差异、DESIGN_ONLY/现场未观察/生产未实现边界均如实保留。审阅后只回填此结果，未修补或复审。

## tx-01-30 唯一整体人工静态文档审阅

审阅结果：通过（tx-01-30 文档范围）。已整体核对本报告、证据 v14、工作设计要求、相关 source、contract v2 与 README；主题覆盖、路径/条件、单成员清理限制、S2 候选与 contract 差异、held V2 候选、Join attempt 终态及证据标签均一致。普通 B Leave 入口等静态缺边及未实现项仍明确标出。审阅后未自动修补或复审。
