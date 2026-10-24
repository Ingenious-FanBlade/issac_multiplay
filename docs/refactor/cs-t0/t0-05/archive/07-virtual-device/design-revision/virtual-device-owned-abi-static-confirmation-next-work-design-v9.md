> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v9

日期：2026-10-13  
设计交付推进：`tx-01-17`  
静态执行推进：`tx-01-18`（W01–W06 是同一完整目标的内容编号）  
状态：`DESIGN_ONLY / 承接 v9 复核；确认报告 v10 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-16` 对[确认报告 v9](virtual-device-owned-abi-static-confirmation-v9.md)、[证据 v9](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v9.md)及[执行设计 v8](virtual-device-owned-abi-static-confirmation-next-work-design-v8.md)的只读复核。复核结论是：正常功能静态确认未完成，且 pair 布局与 callback receiver 两处表述需要勘误。原 `tx-01-15` 文档审阅记录保留，不重跑或改写该次审阅。

## 1. 完整目标与范围

纠正 v9 的实际事实错误，追完关联列表的正常创建/插入/清理和 Offline 条件，连接 Room 内 Player 虚分派到六类输入消费者，给出 B 正常解绑或退出的可用路径，并修订提前发布、未提交 tick 与 Join 多次查询的最小功能方案。交付确认报告 v10、证据 v10 和真实的 topic README 入口。

正常功能静态完整性按实际对象、调用、条件、数据流和阶段关系判断。**记录 UNKNOWN 是诚实报告，不是完成该核验；生产尚未实现、现场值未观察、候选待采纳也不能替代静态缺口判断。** 必须先完成实际分析和必要纠正，再作一次整体文档审阅；不能把审阅临时缩为“标签齐全”后宣称目标完成。

继续采用正常前提：分配成功、对象与缓冲有效、主线程顺序操作、Offline/local-co-op 的 Online roster 为空、虚拟 key 唯一、创建 B 前主 Player vector 恰有 A。沿用[原生 Join 创建/发布用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的 DESIGN_ONLY 后置条件，不重做完整 Player Init 或全局 manager-this 证明。它不自动证明同一命令不会触发重复 selector/创建，也不证明另一个 lookup/context vector 为空。

自有 38 槽 device、六覆写槽、六槽 control、S2、NEVER、原生 base ctor 前固定 DLL、base ctor 后写自有 vptr、构造后不释放、恢复后 DLL 常驻、每 session 至多一次创建继续适用。A/B 共用 scheduler、sequence、target tick、queue、resolve/apply；不增加 B 即时旁路。研究原版 Net 对象只解释原生读者；C/S 不依赖或复用 Online Manager、NetInputDevice、FrameStore。

本轮不展开 OOM、EH/unwind、worker 并发、原子/镜像/溢出、ABA、全映像 alias、持久化或异常恢复；不重开 05-06 完整双端/网络 exactly-once 证明。正常 pause、Room 被 guard 跳过、正常重复 poll/query/pass 及其功能结果属于本轮范围。

`tx-01-17` 完成本文和 README 后一次人工整体静态文档审阅。`tx-01-18` 完成全部分析、编辑和 v10 交付后一次人工整体静态文档审阅；过程零测试，W 编号不拆成多次验收。不设置 gate，不校验任何哈希。

## 2. v9 复核修订清单

| 项 | v9 位置与问题 | 本轮应完成的工作 |
| --- | --- | --- |
| Q01 | 报告第40–41行、证据 E02/E07 将 element+0x378 当 pair pointer，并称 pair+0x0C 为 key | 准确区分嵌入 pair 的地址、首项 device pointer、control pointer、source device key；W01–W02 |
| Q02 | 报告第27–28行将 Manager dispatcher 与 cdecl callback 混写为 this receiver | 逐层列 helper、dispatcher、按值 pair callback、device slot 的调用约定/参数/返回；W01 |
| Q03 | W02 仅完成 storage/reader 关系，没有完成原设计要求的元素类型/factory/插入/清理与 Offline 条件 | 追实际 writer 和 guard；区分结构未解与现场值未知；W02 |
| Q04 | W04 止于 4186C0 的虚调用，B6BDDC→782AF0 没有连接到 Player vtable 和构造对象 | 补实际 address point、槽、构造 vptr 写入和消费者调用链；W03 |
| Q05 | W03 未找到 B 正常解绑/退出；第61行又把关闭 admission 放到退出语义确认之后，和契约及第117行轨迹不一致 | 修订退入中性与正常 remove 的顺序，选择有依据的解绑/退出路径并说明影响范围；W04 |
| Q06 | W05 提前 Resolve 后无 Room commit 保留 epoch，但没有处理队列已消费、后续合法输入、pause 中性；Join 过期又依赖下一 pre-poll epoch | 读取 scheduler 实际语义，比较接续方案，区分 update epoch/Authority tick/publication epoch，给正常分支状态表；W05 |
| Q07 | W06 重复查询只推返回 mask，不分析 selector/key 的重复选择、confirm 与创建条件 | 核对有限原生控制流和 pass 生命周期，给普通命令的副作用轨迹；W06 |

复用 v9 的有效依据：五个 Join action 19/20/21/14/15 已直接确认；Gamepad descriptor 不等于另一个对象的 source pair；n298+0x4B3D8 callback context 与 lookup 的 storage 关系；带 guard 的 poll→Join→Room/Authority 顺序；action8–11 当前只有 edge、不能表达 Drop held。scalar ST(0)→A20AB0 output/AL→9562D0 XMM0、固定方向表、native C4←C0、poll=0 auto-remove、normal remove 不写 Player key等继续按真实来源继承。不要把已有效的结论重新归成整体 UNKNOWN。

## 3. 输入资料与定位种子

必读：本文、project-conventions、coordinator [contract v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、v9 报告/证据、v8 工作设计 W02–W06、[证据 v8](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md) E03 的六消费者和 helper 原始记录。按需读既有 05-03、正常 menu/run 生命周期资料；其旧 gate、哈希和独立角色条款不执行。

以下二进制种子来自主智能体 `tx-01-16` 只读结果，是执行者的定位资料；**不是 tx-01-18 已取得的 IDA-DIRECT**。执行者必须复读关键点，并分别标直接和继承依据。该次 IDA 目标为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，imagebase `0x400000`；本轮以工具返回的当前 IDB/input 元数据为准，不校验哈希。

| VA / 对象 | 已见原始依据或分析终点 | 执行方向 |
| --- | --- | --- |
| 9137F1/9137F3/9137FD | `8B 3E`取 element；`8B 87 78 03 00 00`取 element+378 的首项；`8B 50 0C`取 source device key | 不是先解引用一个 pair pointer 再读 pair+0x0C |
| 913811/913817 | `8D 8F 78 03 00 00`令 ECX=&element+378；`E8 F4 F6 FE FF`调902F10，栈参0 | pair 本体是传入 receiver；902F10 的 this[0]/this[1]分别是对象/control |
| 90AEB0/90AF30/902E80/902F10 | source id reader、pair setter/swap/clear | 沿 caller 追元素 ctor、factory、列表插入及生命周期 guard |
| 952DBC/952DC2/906E7B | n298+4B3D8→906D60→A1D610 注册913760 context | 同 storage 已有证据，补 writer/正常通知影响 |
| A20FB0 | Manager this dispatcher；keyed lookup 经 Manager slot+0x28，callback 另收 device/control 与 payload | raw push/call/ret 优先于 Hex-Rays 错误的无参数 callback 类型 |
| A20940/A209E0/A20AB0 | cdecl callback；前两项接 device/control，后续 action pointer，scalar另收 output pointer；调用 device +30/+34/+38 | callback 无 Manager this，device slot 才以 device 为 ECX |
| A21E72–A21E88 | 先读 Gamepad vptr+4，比较 A22190；匹配时 A21E7F 直接call，否则 A21E86 `FF D0` | 描述实际 Init 分派，保留 A22190 的硬件描述块身份，不把优化分派写成只有一种调用 |
| 41885E、418D86–418E4D | generic entity vptr+8；Main Player vector vptr+0x0C | 分别追各自对象，不能因在同函数中便合成同一 Player update |
| B6BDDC | 该 cell 当前值 782AF0；782AF0 内有多个消费者直接调用 | 从邻接 COL/RTTI、address point、ctor vptr store 判断真实槽和实例，不只列 data xref |
| 7A6450/908070/90E6D0/9BF530/959720 | Player setter；Online lost-connection 全 Player reset；状态转换全 Player remap | 用作正常解绑/退出搜索种子，不冒充 B 专属入口 |
| 954F7D/9551BD/6FADA7/6FADAC/955231/6FBA4D/803E70 | poll；条件Join；pass begin/return；条件Room；Authority wrapper；entity update | 继承带 guard 的链，在 W03/W05/W06 插入具体对象和事件阶段 |

本轮源码种子（读取实际现行内容，行号仅定位）：

- `cs-t0/native/src/input_core.cpp:467–508`：Resolve 即使无输入也记录 last_resolved_tick；同 tick 再次 Resolve 不解析；已解析/过期命令从 queue 移除。
- `input_core.cpp:372–450`：Accept 以传入 current_tick/DrainBeforeResolve 等条件判有效；须核对是否与 last_resolved_tick 同步。`EdgeTargetPlanner` 与 `kInputLeadTicks=2` 也必须纳入时间表，不能把刚采样命令当立即可用于 next tick。
- `cs-t0/native/src/active_dispatcher.cpp:371–408`：Resolve 调队列并在 candidate_tick≥pending_target_tick 时清 pending。同时读取 SubmitPhysical、编码/接收/drain、same-tick merge，确认真实 drain→resolve 顺序。
- `cs-t0/native/src/active_runtime.cpp`：PhysicalSample 经 g_last_completed_tick 提交；Authority(true) 当前计算 next tick、Resolve、BeginAuthority；正常 post EndAuthority/提交 tick；pause/loading/owner generation 的现有正常转换。
- `cs-t0/native/src/loop_runtime.cpp`：InputManagerSampleHook、StandardRoomHook、TopUpdate 与观察 callback 的具体 before/original/after 顺序；不能只引用 call anchor 就宣称 pre-poll API 已存在。
- `cs-t0/native/src/active_apply.cpp`：Evaluate 的自然 consumer/family/generation 条件、8–11 magnitude/triggered 与 scope active/previous；`active_runtime` 查询 wrapper 是否可在 native device query 之前直接返回。

## 4. 执行工作

### W01：pair 与 callback 的精确勘误

复读 §3 中 pair load/LEA、902F10/902E80 和 Manager dispatcher/callback 的 raw instructions。形成对象布局表：entry own key=entry+0x0C；source pair 地址=entry+0x378；pair+0 device、pair+4 control；source key=pair 首项指向的 device+0x0C。空首项的 sentinel 是诊断/局部比较行为，不引入 C/S magic key。

给六消费者共享查询链的 receiver/参数/返回表：Player owner→helper 的全局/栈参数→Manager dispatcher this→按值 pair callback 的 cdecl 参数→device slot ECX及 action→返回通道。明确 A20FB0 与 A20940/A209E0/A20AB0 是不同调用层。不得照抄错误伪代码形参来覆盖 push/ret/x87 原始依据。继承三层 scalar ABI和六消费者 action，不要求重做全部38/6槽。

交付 Q01/Q02 勘误、准确布局/receiver 表、复读清单与来源；新报告纠正，旧报告保留历史。

### W02：lookup/context 列表的实际元素与正常生命周期

以 90AEB0/90AF30 的 caller、列表 writer 和已知 callback 注册为根，追到元素实际 ctor/factory、vptr/RTTI 或可识别的创建 caller、分配大小、own key/source pair writer、vector 插入与正常 erase/clear。给“对象类型→factory/ctor→字段初始化→插入 guard→lookup→通知清理”的有限链。证明 Gamepad descriptor 属于另一对象后，不再用 A22190 填补该列表元素的构造缺边。

连接 952410/906D60 注册与 Item/PillCard/helper fallback reader。给 Offline roster 空而 context list 空/非空的分别条件；只有 writer/guard 支持时才能推出空。实际 listener 地址集合/现场成员仍可未知，但当前源码/IDB 中能确定的注册 owner、回调 ABI、writer guard 属于静态工作。

沿 A20EB0 两个 notify 槽解释自有 Base→Net cast 为 null 的正常分支：哪些 element 被 erase、哪些 source pair 被清、哪些 own key/Player key/selector 保留。列表为空与非空、source key命中与不命中各列一条手推结果。不把 callback 收到一个断开 Net pointer 误写成“所有元素已经被 cast 为 Net”。不寻找全映像 alias 闭包。

交付元素布局与创建/插入/清理路径、Offline 条件表、通知效果表。若仍未解，列已读函数、最后已证 writer/callsite、具体缺边及其功能影响；不能只写“membership 是运行值”。

### W03：Room 内 Player 虚分派到输入消费者

从 B6BDDC→782AF0 入手识别所属 vtable 的 address point、COL/RTTI 与对应 ctor vptr 写入；核对真实槽偏移。把 4186C0 的 generic entity+8 与 Main Player+0x0C 两条路径分别接到实际对象。若存在 thunk、不同派生 Player 或条件 update，记录 receiver 调整/guard，不要求证明所有实体类别。

从已关联的正常 Player update 追到 Movement/Shooting/Bomb/Item/PillCard/Drop 六消费者的直接调用及 helper 输入查询，给调用点和是否在当前 Authority original 内发生的依据。止于输入消费，不展开所有世界副作用。data cell 指向目标还不够，必须给构造实例/容器与虚调用的关联。

同时核对 native helper 查询之前的现有 C/S override：普通自有 device 正常 key 在哪种条件下真正到达 +30/+34/+38，在哪种条件下被早期 synthetic 结果截断；family 限制与 cross-family 的影响是什么。将 native ABI 静态闭合与未来 consumer wrapper 适配分开，不能把装上自有 vtable 当作自动生效。

交付精确 Room→Player vtable→六消费者阶段图和早期 override 条件表。没有静态目标证据时，只将具体边列未解，不外推整个正常 poll/Join/Room 顺序未知。

### W04：B 正常解绑/退出与退休调用序列

有限沿 7A6450 的直接 caller、正常 local-co-op/Player collection lifecycle 与既有 menu/run 资料定位可用解绑或退出入口。先明确目标：暂停只要求 B 中性且保留登记；正常退休要求 B 解绑或退出并移除 device。保持 A 游戏的 B 独立退休与 whole-run teardown 分别说明，若只能确认后者就报告其适用范围，不冒充前者。

对选中的调用路径列 receiver、参数、正常条件、Player vector/ordinal、B key、selector、关联 Player/source pair/context 与后续 consumer 的变化。尤其核对 SetControllerId 的 propagate=0/1，以及 key=-1 后的 helper fallback/all-device 查找；仅调用 setter(-1)不自动保证 B 不会读取 A。若候选需额外 consumer 中性或停止更新机制，给正常原生依据和明确契约差异，不裸写 Player 字段或发明 sentinel。

依现行契约，进入 RETIRING 即关闭 admission、停止旧事件并中性输出；未知退出路径不妨碍定义这一步，却妨碍声称 remove/恢复已完整。按实际证据给后续 B 处理→notify/context 清理→Manager remove→key/member miss 的合法顺序，poll等待期间保持非零。澄清 v9第61行与第117行的相互矛盾。NEVER对象/control/DLL继续常驻。

交付至少一条有依据的正常退休序列，或精确说明为何现有解绑/退出入口无法满足哪一后置条件；不把 Online lost-connection 全体 reset当 C/S local 方案。

### W05：Resolve 副作用、未提交接续及最小快照方案

完整阅读 §3 scheduler/loop/Authority 源码，列 queue accept/drain/resolve、pending、publication、poll锁存、BeginAuthority、原函数、post commit 的实际 owner与时点。判断 v9 pre-poll候选是否与当前 Resolve 语义兼容；当前源码不存在可回滚 Resolve，不能凭“candidate尚未commit”假定队列也尚未消费。

至少手推三类正常时序：①有效Room正常到达；②pre-poll后Room被native guard跳过/正常pause，completed tick不变；③第一次预解析后有符合当前Accept规则的新command/状态转换。第三类是否可达必须结合lead ticks、当前tick来源和实际drain规则判断，不能杜撰运行反例；如可达，说明缓存候选是否纳入/排除它，以及下一Resolve是否将它过期。追同一target tick重复解析被拒、无输入也推进last_resolved_tick和pending清除的后果。

比较两类最小候选后推荐一个具体方案：

1. 保留poll唯一锁存writer：必须有充分依据的预发布窗口，或明确新增 staging/reserve/commit/取消语义，解释Room未执行、pause中性和generation变化。不能直接把破坏性Resolve当可撤销peek。
2. 保持已进入Room的Authority pre统一resolve：明确如何接入自有S2和previous，若由Authority写锁存则明确改变poll唯一writer契约；不能静默声称兼容。Join仍需独立、在其pass之前可见的命令方案。

不预先指定采纳哪一种，不编辑contract/schema/源码。候选须给具体callsite/API职责、数据字段、单一writer、effective tick、new epoch条件、正常post提交、中性时点、generation失效、无Room结果和旧edge关闭点。保持A/B统一scheduler，不将顶层update次数当Authority tick。

分别定义 `update_epoch`（每次相关outer update的序号及具体owner）、`publication_epoch`（一次快照发布身份）、`target_tick`（Authority逻辑tick）与 `join_pass_id`。不能让Join过期只等待一个可能因Room跳过而不递增的gameplay epoch。给JoinPassBegin/Complete/无pass update结束/CloseAdmission的关闭规则、命令target和首次Join的可见时点。

复用press/hold/release/repress表，补多poll、无新publication、pause/恢复、跨有效Authority tick的triggered有效阶段。action8–11 held/edge仍需版本化方案；当前bit0-only实现如实列出。候选涉及的phase/poll/previous/schema/Join contract差异逐项列明，推荐不表示用户已采纳。

交付当前源码时序表、v9候选问题的条件化推导、一个内部一致的最小候选及明确契约差异。若所需正常guard/调用窗口未证实，候选该项标未闭合，不用DESIGN_ONLY标签掩盖内部矛盾。

### W06：Join 副作用与整合轨迹

复读9BAC40普通Offline分支：action19选择、20/21切换、14confirm、15cancel、按key查询、四selector遍历、当前Player count/已分配key检查、原生slot21 flush的实际位置。解释同一唯一虚拟key是否能进入两个selector、confirm后哪个正常guard阻止同pass/下pass再次创建；只读query每次返回true不是副作用一次的证明。

给Join命令候选的admission/sequence/pass范围：正常一次创建后如何关闭后续创建准入、允许或禁止的cancel/选择组合如何定义、未进入pass何时失效。6FADA7/6FADAC只覆盖已证caller；核对必要直接caller或将候选明确限于此范围。若原生guard不足，列coordinator所需的显式命令/操作状态，不增加query侧消费，不改contract，也不扩为05-06全网络证明。

整合八条人工轨迹：select+confirm同pass；同pass重复查询与selector循环；同gameplay snapshot跨两个Join pass；同epoch重复poll/下一有效tick无输入；poll后Room跳过且Join pass未进入；pause/恢复与正常后续输入；cancel+CloseAdmission；正常retirement。每条列command/target、epoch/pass、S2 previous/current、query返回、selector/key/Player count、队列消费/提交、下次阶段结果及未解点。是静态手推，禁止运行模型或模拟脚本。

v10同时保留P01注册枚举、P02type/name/binding、P03Offline Join、P04key/B ordinal、P05快照消费、P06正常停用六条系统摘要。每条分别列静态已证事实、静态缺边、设计候选/待采纳、现场值未观察、生产未实现；不能把所有剩余项都放入“运行值/生产缺项”列。

## 5. 交付物与证据要求

- 新报告：`docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v10.md`。
- 新证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v10.md`；必要分册使用同一v10前缀，并纳入报告引用。
- topic README：更新执行进展/结果入口，保留v9原交付与tx-01-16勘误、旧v8失败/异常和历史文档。

证据逐条记录对象根、VA/RVA/关键bytes、函数/caller、receiver/参数/返回、正常guard、writer/reader、来源与推导范围。IDA-DIRECT仅用于本轮实际读到结果；SOURCE-DIRECT给实际文件/函数；继承给版本/节/分析终点；CONTRACT、DESIGN_ONLY、UNKNOWN分别保持真实含义。工具失败、截断或只看xref集合不等于已复读函数。

报告开头直接给正常功能静态完整性结论和Q/W完成情况。未完成项要列具体原生/源码边、已证终点、分析内容及下步定位，不能仅复述v9的UNKNOWN。文档准确性、正常功能静态程度、候选一致性/采纳、现场观察、生产实现分别判定。

## 6. 工具与所有权交接

主智能体交付本文后，以新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行tx-01-18，不另派子智能体或新侧栏聊天。旧v9实例已完成；旧pending实例不恢复任务。主智能体交出IDA只读使用权与v10文件/topic README编辑权，执行期间不并发读取IDA或编辑同文件。

工具按当前可用schema发现。IDA只允许读取input/IDB元数据、raw instructions、decompile、xrefs、vtable/COL/RTTI数据；不patch、rename、set_type、define_func或save IDB。PowerShell阅读/rg和必要文档编辑可用login:false；不改用户环境。py_eval只做静态读取，不运行正确性验证程序；globals/locals分离时使用简单循环，避免闭包错误影响实际读取清单。

本次没有新增游戏/调试器会话操作：不检查运行进程、不访问进程内存、不附着/断点/单步、不输入游戏、不注入/安装Hook。旧动态授权和工具可用不扩大本轮范围。不运行构建、测试、harness、probe、smoke、canary、verifier、链接校验/哈希脚本或可执行状态模型；引用靠实际路径人工核对。

## 7. tx-01-18 唯一整体静态文档审阅

全部分析、必要纠正和交付完成后，只进行一次人工整体通读，覆盖本文、v10报告/证据/分册、关键source/contract和README。分析编辑期间发现错误应正常纠正，不故意留给末尾审阅。审阅之后只回填真实结果，失败时记录，不自动修补/重跑；继续修改须说明新的实质工作，不只换编号重验。

| 维度 | 必须给出的判断 |
| --- | --- |
| 事实准确性 | pair地址/首项/control/source key，dispatcher/cdecl/device层和原始调用约定正确，来源没有错标 |
| W02对象链 | 实际元素类型/factory/insert/cleanup和Offline条件是否已证；静态缺边没有被现场值替代 |
| W03阶段 | vtable address point/ctor/container→正确虚槽→Player消费者是否连接，guard和early override明确 |
| W04退休 | admission/neutral及时生效；B处理、fallback、A影响、context与remove顺序有依据或精确未解 |
| W05候选 | Resolve消费/last_resolved/pending、lead/drain、skip/pause、tick/各epoch、旧edge与契约差异一致 |
| W06效果 | 正常selector/key/confirm guard和pass事件关闭已推导，未把重复true当只产生一次副作用 |
| 完整性/记录 | Q01–Q07、W01–W06、P01–P06实际覆盖；历史原文保留；五类状态分开；引用人工正确 |

文档准确性可单独通过；任何正常功能静态缺边仍在时，必须明确正常功能未完成。候选待用户采纳、未编写生产实现、未观察现场值不应被算成原生调用关系未证。不要预填PASS，不将“每项都有UNKNOWN”视为W工作完成。

## 8. tx-01-17 设计交付记录

本文用tx-01-16实际复核和本轮源码阅读提供执行种子；不新增tx-01-18二进制直接证据，不修改旧报告/证据、contract/schema、生产源码或测试。仅新增本文和更新topic README。

一次整体人工静态文档审阅结果：**通过（设计交付范围）**。全部编辑后人工一次通读实际保存的本文与README修订范围，对照现行规范、contract v2、v9复核与源码读取结果，检查Q/W/P映射、pair/receiver原始种子、列表与虚分派缺边、B退休范围、Resolve消费与lead/drain/epoch问题、正常Join副作用轨迹、五类状态、产物所有权及实际路径引用。本文未把历史PASS、UNKNOWN标注、设计待采纳或运行值未观察当成功能完成。未运行链接检查、哈希、构建、测试或实机操作；未进行第二次审阅。本段只回填此次真实结果，不表示tx-01-18静态核验或正常功能已完成。
