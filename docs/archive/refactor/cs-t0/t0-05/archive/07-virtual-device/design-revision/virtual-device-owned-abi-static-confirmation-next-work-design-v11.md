> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v11

日期：2026-10-13  
设计交付推进：`tx-01-23`  
静态执行推进：`tx-01-24`（W01–W04 是同一完整目标的内容编号）  
状态：`DESIGN_ONLY / IDA 已恢复；承接 v11 复核；确认报告 v12 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-22` 对[报告 v11](virtual-device-owned-abi-static-confirmation-v11.md)、[证据 v11](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v11.md)和[执行设计 v10](virtual-device-owned-abi-static-confirmation-next-work-design-v10.md)的只读复核。v11 静态完整性未完成，原文准确性审阅未通过；旧文档和失败记录保留，不修补或重跑 `tx-01-21` 的审阅。本轮通过新二进制读取、候选规则补全和新文档完成实质推进。

## 1. 完整目标、正常前提和交付

利用恢复的 IDA 补读 `95B310` 完整本地断线/reselect 分支，确认它对自有 Base 设备、B ordinal1、A继续运行的具体作用；得到有依据的正常 B 退休调用序列，或明确该路径不能满足目标的原因及有限替代入口。纠正 v11 的 Mutex/callsite/无 Room 状态和来源分类错误；补全 S2 的跨动作类别查询、有效范围及提交关闭规则；复核 Join 的 K/pass/创建尝试语义。交付报告 v12、证据 v12 和 topic README 的实际进度。

沿用正常前提：分配成功、对象与缓冲有效、主线程顺序操作、Offline/local-co-op、Online roster 为空、唯一正 key K、创建 B 前主 Player vector 恰有 A。Online roster 空与 n298+4B3D8 context vector 空分别判断；命名和空条件必须对应实际存储。沿用[原生 Join 创建/发布用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的 DESIGN_ONLY 后置条件，不重做 full Player Init/全局 manager-this，不升级成新的二进制或运行证明。

保持 38 槽 device、六覆写槽、六槽 control、S2、NEVER、base ctor 前固定 DLL、base ctor 后写自有 vptr、构造后不释放、恢复后 DLL 常驻、每 session 至多一次创建/失败不自动重试。A ordinal0、B ordinal1；K是设备关联，不是ordinal。A/B共用canonical、sequence、target tick、queue、dedupe、resolve/apply，没有B即时旁路。复用通用InputManager；C/S不依赖Online Manager、NetInputDevice、FrameStore。

不展开OOM、EH、worker并发、原子镜像/计数溢出、ABA、全映像alias、持久化、异常恢复或05-06全网络证明。正常pause、loading recovery、无Room、重复poll/query/pass、正常设备移除、B后续查询属于范围。

`tx-01-23` 完成本文及README后仅一次人工整体静态文档审阅；`tx-01-24` 完成全部分析、纠正和v12交付后仅一次人工整体静态文档审阅。过程零测试，不设gate、不校验哈希、不运行构建或游戏。W编号不拆成多次验收。

## 2. v11 复核结果与继承边界

| 项 | 当前事实或问题 | 本轮处理 |
| --- | --- | --- |
| Q01 B退休未完成 | v11只能读取95B310至95B4AB，95B87F/95BCA0沿旧摘要；UI、Player removal、ordinal和A影响未证。 | IDA完整读取函数及相关callee、分支和后续读者，W02。 |
| Q02 Mutex receiver | 证据v11 E05把C57B2C、虚槽+0C、参数-1写成设备key查询。C57B2C=C57B18+14，是Manager内嵌Mutex；同片段读active vector数量后用+10解锁。 | 原始指令区分Mutex锁/解锁与Manager查询，W01。 |
| Q03 调用地址 | E05将95B395标成A20240调用；保存raw显示95B395是mov ecx,C57B18，95B39B才call A20240。 | 更正call anchor，区分参数准备与实际call，W01。 |
| Q04 无Room状态 | 报告第91行称首次普通无Room时queue/planner/pending原样留存；同update的PhysicalExit可能已提交输入。第二次缺失的PauseEnter发生于TopUpdateExit，也不能倒推此前所有采样都被禁止。 | 分开poll/提交、Room guard、TopUpdateExit、generation clear和下一次提交，W03。 |
| Q05 跨类别查询 | 候选要求consumer family匹配，却未说明原生Movement→Drop、Shooting→Movement/Drop、Item→Card/Drop、Card→Item/Drop等正常查询如何通过。 | 自有slot给明确许可关系和返回规则；不照搬ConsumerForAction相等条件，W03。 |
| Q06 已证阶段链 | v11重新笼统列poll/Join/Authority/consumer全局顺序未知；v10已证带guard的限定链。 | 继承限定链，只补候选依赖的新窗口、B退入/移除时点等具体边，不扩大成整体未知，W01/W02/W03。 |
| Q07 文档与分类 | v11审阅记录还指出inline-code未闭合；生产缺项和候选待采纳不能算native静态缺边。 | v12正确引用/排版，保留旧失败；状态按事实、候选、现场、实现分别列明，W01及最终汇总。 |

继承有效结论：Base/Control ABI及布局；callback action指针与device action值分层；scalar ST(0)→output/AL→helper XMM0；held bit0=Shoot，action8–11 edge bits0–3；source pair与entry own key分离；context factory/insert/notify清理和empty guard；native insert/keyed lookup/remove；poll=0自动移除；setter与remove不等于Player退出；Player B6BDD0 address point、753DF2 ctor store、Main Player虚槽+0C→782AF0→六消费者；Join19/20/21/14/15和正常唯一K重建抑制。

带guard的正常链已见于[v10 W03](virtual-device-owned-abi-static-confirmation-v10.md)：954F7D poll→954F84 reselect；满足state2及两个subguard时9551BD→6FA540→6FADA7 Join；Join后的标志允许时955231→6FADC0→6FBA4D→802980→803E70→4186C0→Main Player+0C→六消费者。StandardRoomHook包围native原函数的Authority pre/post已由源码确认。此结论不覆盖所有caller、所有实体类别或新增coordinator的安装/时点。

## 3. 恢复后的工具状态与定位种子

本次设计准备已实际调用IDA只读工具：survey_binary(minimal)和server_health返回服务正常、Hex-Rays可用、auto analysis ready；输入为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，IDB为同路径的 `isaac-ng.exe.i64`，32位，imagebase=400000，image size=8FE000。未输出、比较或校验hash值，hash字段不纳入本轮依据。

随后主智能体只为设计摘读95B310带地址伪代码，返回563行，可见后半分支。以下是执行定位种子，不是子智能体 `tx-01-24` 已取得的IDA-DIRECT；执行者按实际读取/继承标来源，涉及ABI、对象身份、guard和callsite必须核对raw。

| VA / 函数 | 设计准备已见或既有定位 | 执行方向 |
| --- | --- | --- |
| 95B310 | 当前decompile签名以n298为this；包含Gamepad检查、UI、unassign、reassign及函数尾。 | 完整复读并分页读raw，补完原先95B4AB截断之后的全部相关出口。 |
| 95B476、95B49A、95B4C0 | 9C25E0、9C3B70、9C39E0调用，随后写n298[76530]并返回。 | 识别UI对象/返回状态及后续主更新guard；不能把UI建构当Player removal。 |
| 95B6EC..95B884 | v20为空时建disconnect/reselect提示；95B7A2/95B783/95B7F4调9C2DB0；95B806取9C39E0结果，95B811写*v17。 | 追v17/断线索引/v91/设备lookup来源和副作用；是否阻止Room/影响A。 |
| 95B815..95B843 | n15分支、n15[16]==12等条件可到98AA30调用。 | n15实际对象和98AA30效果未核实；禁止预称为Player删除。 |
| 95B852..95B87F | else-if(n60)，n298[77046]==[77047]，417870(n60,v91)取对象，7A6450(-1,1)。 | context空条件、目标索引、传播和随后query；不是任意lookup miss无条件解绑。 |
| 95BC7D..95BCA0 | n60[28330]/[28331]的Player vector及v91选择，7A6450(*v49,K,1)。 | guard/越界回落/主Player关联、selector清理/A21CB0后置状态。 |
| 954CD0、9C3990、9505E0、6FA540 | 已知更新guard、Join/Room限定链。 | UI写入后的下一更新是否仍允许A的Room；不得以函数名猜暂停语义。 |
| A20EB0、A1FC00、A212C0、913760、7A6450、956110/9561E0/9562D0、A20390/A20FB0 | native remove/notify、setter、keyed/fallback。 | B退出/解绑前后K解析、关联清理和A输入隔离。 |
| 95B350..95B39B、A20E1F..A20E27、A20E4C | Mutex receiver和A20240 call准备/raw种子。 | 更正Q02/Q03，按实际ECX、push/call/ret列ABI。 |
| 9BAC40、9BAAA0、9BB2DF/9BB334/9BB371/9BB567/9BB56E | selector导航回环与K/Player guard。 | 有限复读W04所需控制流，不重做完整B Init。 |

可用工具按当前schema选择：survey_binary/server_health、decompile、disasm（offset/max_instructions分页）、get_bytes、xrefs_to；py_eval仅做纯静态IDA读取。读取超过页限须接cursor/offset，工具错误或截断不能算函数已读完。旧decompiler类型与形参可能错误，raw优先；不通过set_type/rename等修改IDB来修复显示。

同时读取[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、v10/v11报告证据、[证据v8 E03](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)、[v2 W09](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)与[W06/W07 A-3](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w0607-draft.md)。源码重点为input_core Accept/Resolve/Planner、active_dispatcher Submit/Drain/Resolve/ClearForGeneration、active_apply Begin/End/Evaluate/ClearGeneration/ConsumerForAction、active_runtime PhysicalExit/Authority/TopUpdateExit/lifecycle、loop_runtime的三个相关Hook。

## 4. 执行工作

### W01：有限事实勘误与已证链继承

用IDA raw复读Mutex对象、锁参数、计数读取、解锁、A20240真实call；形成“对象→ECX→虚槽/目标→参数→作用”表，不能把-1锁参数称controller key。有限核对callback到device槽的action值，不重做38/6全表。纠正v11引用/inline-code和不准确call/source定位，在新报告中注明原错误及真实依据，旧文档保留。

对照v10限定poll→Join→Room→Player链与源码Hook，列已证guard和继承来源。凡新增时点需要证明，指出具体caller/事件边；不能因v11缺IDA就丢弃已证链，也不声称全映像/所有caller已闭合。

交付勘误表、精确对象/ABI表、继承链和真正新增的静态问题。native已证注册/lookup不因生产代码未编写变成UNKNOWN；契约差异和实现缺项分别列明。

### W02：完整本地断线分支、B/A结果和退休序列

完整读取95B310及关键raw分支，从当前n298/UI状态、设备枚举/lookup、设备类型、Main Player/其它菜单对象、断线索引、context空条件恢复状态图。识别n15、n60、v17、v91、ArgList等实际对象与writer/reader，不用伪代码变量名代表身份。尤其区分UI/selector清理、Player setter、真正Player/member移除和Manager设备pair移除。

追9C25E0/9C2DB0/9C3B70/9C39E0及实际用到的状态写入；有限跟入98AA30并先确认其对象和作用。沿954CD0已证guard说明这些UI状态是否阻止Room、A是否继续更新、是否要求人工reselect。沿417870和Player vector确认目标什么时候是ordinal1、什么时候可为A或其它对象；propagate=1对子/linked Player及NPC的作用范围必须对应setter。

将正K查找失败、-1 all-active/context fallback、B是否仍在Player vector、setter后下一次consumer接起来。自有Base不满足Gamepad cast时明确实际回落路径；不要由部分cast-null片段推断支持/不支持整个流程。n298[77046..77047]空条件应与已定位context vector对应，不沿旧摘要笼统称Online roster空。

比较正常两序：①合法B处理后Manager remove；②RETIRING先中性并remove，下一主更新由95B310处理K miss。给每个步骤receiver/参数、调用阶段、guard、B key/member/ordinal、A输入、selector/UI/context状态和下一读者。RETIRING首步立即关闭admission/Join/publication、generation失效、自有查询中性；登记期间poll非零；NEVER对象/control/DLL常驻。Pause仍只中性并保留登记。不能把setter(-1)、selector cancel、单独remove或永久登记中性等同完整退休。

若95B310固有地弹UI、暂停整个run、重分配A或不能处理自有Base，给有原始依据的“不满足B独立退休”的结论；有限沿7A6450实际caller及既有local-co-op生命周期定位替代正常入口，不进行无界全映像搜索。可用替代入口也必须证明实际对象/参数/后置状态。若没有足够依据，列精确终点、缺边与需要的设计决定，不能重复“没找到”或捏造成功路径。

分别判定B独立退休且A继续、whole-run正常teardown。已证明scheduler关闭不等于自有native pair/key清理；后者按实际生命周期另说明。不要借Online全Player reset完成本地方案，也不裸写Player字段、发明sentinel或绕过原生guard。

交付完整分支/对象表、B/A后置结果、至少一条有依据的可用退休序列或明确不可用原因和具体剩余边。

### W03：S2跨类别查询与完整正常时序

复读现行source与v8六消费者映射，分清native buffering、自有S2、现有SyntheticAdapter。当前Evaluate要求consumer==ConsumerForAction(action)，不适用于所有原生合法跨类别查询。候选必须给自有query有效条件及允许的consumer/action关系；明确Movement→Drop、Shooting→Movement/Drop、Item→Card/Drop、Card→Item/Drop返回的current/previous/pressed/triggered/scalar。不能把不属于本类别的动作自动置中性，也不能靠未经证明的physical原函数fallback供给B输入。

说明自然Player/consumer witness、K、ordinal、session/generation、Authority scope如何传到自有槽；现行keyboard/gamepad限定reader/owner逻辑需要哪些实现适配。分清原生调用已证与新自有组件待实现，不把现有单Player适配器直接当A/B已接入。

补全Authority-pre发布候选：已进入有效Authority才统一Resolve并发布state或显式neutral；normal post提交completed tick/previous并关闭current查询；scope外旧edge不可见，previous只是下一有效scope基线。继承Resolve破坏性、lead=2、同tick不可重算及future/pending规则。列writer/API/字段、查询许可、关闭点、无输入中性、generation失效和与contract v2的poll唯一writer差异。若另选poll writer方案，必须明确不破坏队列的staging/reserve/commit等新增机制，不假定Resolve可回滚。

按实际事件顺序分段手推：poll→合资格PhysicalExit Submit→native Room guard→有/无Authority→TopUpdateExit→可能PauseEnter clear→下一poll。第一次miss不Resolve/commit，但可有queue/planner/pending提交变化；第二次连续miss在Exit时触发clear，之前的采样可能先发生，之后才受Pause gate；loading recovery与独立Room/Loading lifecycle分别说明。

状态表记录completed/candidate/target tick、update/publication epoch、generation、queue/planner/pending、previous/current、query有效范围、Join pass状态。覆盖press/hold/release/repress、同update重复poll/query、post后query、第一/第二连续miss、loading recovery、无输入正常Authority、Pause退出首个Authority、上述跨类别查询。纸面推导，不执行模型。

保留v1四个edge位和Shoot held事实；按正常pressed reader给action9–11最小held版本化方案，action8是否只需edge按已证reader说明。明确edge与持续量在同publication及下一tick的语义，不把重复query返回true当任意世界副作用恰好一次的证明。

交付当前时序与完整候选、许可关系表、正常轨迹及契约差异。候选推荐不表示用户采纳，本轮不修改contract/schema/source；任何内部未解条件不得笼统标“内部一致”。

### W04：Join限定复读、K/pass及创建尝试

继承v11已纠正的导航回环，有限IDA核对19/20/21/14/15、raw flush与confirm/cancel顺序，明确导航跳过当前selector后仍继续其它selector。确认唯一K正常select/confirm后哪个Player/selector guard抑制重建。

JoinCommand候选明列session/generation、K、sequence、target update epoch、pass id、动作、准入与Bind/Complete/无pass Exit/retire关闭；query只读，仅匹配K和有效pass可见。说明first B尚不存在时control lane如何先于native pass供给命令，成功发布后gameplay才进入A/B统一scheduler；不把update epoch当Authority tick、不增加B即时输入旁路。

补明确每session创建尝试状态：动作准入、selector开启/导航、confirm未到达创建call、实际调用创建、成功发布、失败分别是什么；哪个owner阻止成功后重建或失败自动重试。不能把每次query、pass或导航都算一次创建，也不能只用query mask只读证明创建次数。full B构造/发布仍沿用户假设。

手推19+14同pass、导航/取消、重复query、同update第二pass、无pass update、另一K的selector、guard失败、实际创建失败、retire与pending相交。与W03的scope/epoch关闭规则合并，phase和状态彼此一致。

交付有限native副作用/guard表、K/pass/尝试候选及轨迹；不展开双端网络exactly-once。

## 5. 产物、证据与推进结论

- 新报告：`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v12.md`。
- 新证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v12.md`；必要分册同v12前缀，全部引用。
- 更新topic README执行状态/结果入口，保留旧v11原文、审阅失败和v10等历史资料。

证据逐项记录对象、VA/RVA/bytes、caller、ECX/参数/返回、guard、writer/reader、正常后置和推导限度。IDA-DIRECT是本轮实际成功读取；SOURCE-DIRECT给当前source位置；ARTIFACT-READ给原始导出文件/字段；STATIC-INHERITED给旧报告/节/终点；CONTRACT、DESIGN_ONLY、UNKNOWN分别保真。工具失败、分页未完成或只有xrefs不能算完整读到相关分支。

报告开头给Q/W完成情况及正常功能静态结论。P01–P06（注册枚举、type/name/binding、Offline Join、key/B ordinal、snapshot/query、pause/退休/context）分别列已证、实际静态缺边、候选差异、现场值和实现任务。注册/自有slot/Join lane尚未写生产代码不是native未知；现场K/vector/listener值未知不否定已证guard。

结尾列可按现行已证ABI开展的具体实现、仍需补读的有限正常边、需要采纳的契约差异和整体验收观察事项。不以合同候选未采纳笼统阻止固定layout、注册等可独立实施部分，也不从这些部分可开始推出完整B退休已证。

## 6. 所有权、工具范围与唯一整体验收

设计完成后启动一个新上下文 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行tx-01-24，不另派子智能体或侧栏聊天，不恢复已完成的v11实例。主智能体交出v12文件、topic README编辑权和IDA只读使用权，执行期间不并发编辑或读取同一IDA会话。

允许静态资料/源码/IDB读取及本轮文档编辑；不改旧报告、生产源码或contract/schema，不patch/rename/set_type/define_func/save IDB，不使用debugger功能或访问游戏进程，不输入游戏、注入或安装Hook。PowerShell可用login:false；不跑测试、构建、harness/probe/smoke/canary/verifier、hash/链接检查脚本或可执行状态模型。恢复的IDA服务只扩大本轮静态读取能力，不扩大实机操作范围。

tx-01-24全部分析和编辑结束后一次人工整体通读本文、v12报告/证据/分册、关键source/contract、README。重点检查Mutex/callsite、完整95B310条件/对象/UI/A影响/退休次序、跨类别query、无Room采样与Exit清理时点、S2有效期/post/previous、Join尝试与关闭、来源及引用。分析编辑中正常纠错；最后审阅后只回填结果，失败不自动修补或重审，不仅换编号重复验收。

文档准确性、正常功能静态程度、候选一致性/采纳、现场未观察、生产未实现分别判定；不得预填PASS或以UNKNOWN标签齐全代替完成。

## 7. tx-01-23 设计交付记录

本次只新增本文并更新topic README。IDA服务/数据库元数据已确认，95B310伪代码摘读仅提供设计定位种子；无功能运行实验、测试、构建、hash校验、源码/contract/schema修改或游戏操作。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文及README修订范围，对照现行规范、contract v2、v11复核、v10有效限定链、已读source和本次IDA metadata/95B310伪代码种子，检查Q/W/P对应、Mutex/callsite、UI与Player身份区分、退休结果要求、跨类别query、无Room采样/Exit时点、S2有效期与契约差异、Join尝试/关闭、来源及实际引用路径。本文未把摘读伪代码或工具恢复当作B退休已完成，未把生产未实现/候选未采纳计为native缺边。审阅后仅回填结果，无第二次审阅；此结果不表示tx-01-24核验或正常功能已完成。
