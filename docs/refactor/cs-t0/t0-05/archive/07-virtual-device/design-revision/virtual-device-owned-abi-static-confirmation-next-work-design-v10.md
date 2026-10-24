> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v10

日期：2026-10-13  
设计交付推进：`tx-01-20`  
静态执行推进：`tx-01-21`（W01–W04 是同一完整目标的内容编号）  
状态：`DESIGN_ONLY / 承接 v10 复核及 local-co-op 解绑线索；确认报告 v11 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-19` 对[确认报告 v10](virtual-device-owned-abi-static-confirmation-v10.md)及[证据 v10](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v10.md)的只读复核，并纳入随后从[证据 v2 W06/W07 草稿](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w0607-draft.md)重新定位的 `95B310` 本地断线/reselect 分支。v10 的原审阅记录保留为历史；该次记录中的“未发现事实误述”不能覆盖本轮列明的勘误。

## 1. 完整目标与范围

交付可供自有 ABI 正常功能实现使用的静态说明：纠正 v10 的四项事实误述；确认本地 `95B310` 能否满足仅处理 B 的正常解绑/退出及设备移除要求；把快照的发布、查询有效期、previous 提交、无 Room 与 pause 行为写成内部一致的候选；限定 Join 命令的目标设备、pass 和正常副作用。交付确认报告 v11、证据 v11 和 topic README 的实际进度入口。

本轮不再笼统“寻找是否存在 B 解绑入口”：已有本地候选，工作是追它的完整 guard、受影响对象及后置状态。也不把“生产源码尚无自有设备注册”“候选待采纳”“现场值未观察”算成原生静态缺边。报告必须分别说明原生静态程度、候选一致性/契约差异、生产实现工作和运行时待观察事项。

继续采用正常前提：分配成功、对象与缓冲有效、主线程顺序操作、Offline/local-co-op、Online roster 为空、虚拟正 key K 唯一、创建 B 前主 Player vector 恰有 A。另一个 context vector 是否为空必须按已经定位的 writer/guard 判断，不能由 roster 空推出。沿用[原生 Join 创建/发布用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的 `DESIGN_ONLY` 后置条件，不重做 full Player Init 或全局 manager-this 证明，不升级为静态或运行事实。

已采纳约定保持：38 槽 device、六覆写槽、六槽 control、S2、NEVER、原生 base ctor 前固定 DLL、base ctor 后写自有 vptr、构造后不释放、恢复后 DLL 常驻、每 session 至多一次创建。A ordinal=0、B ordinal=1，设备 K 与 ordinal 分开。A/B 共用 scheduler、sequence、target tick、queue、resolve/apply，无 B 即时旁路。通用 InputManager 可复用；C/S 不依赖或复用 Online Manager、NetInputDevice、FrameStore。本轮读取这些原版对象只解释原生读者和排除不适用路线。

不展开 OOM、EH/unwind、worker 并发、原子镜像/溢出、ABA、全映像 alias、持久化、异常恢复和 05-06 完整网络证明。正常 pause、连续无 Room、正常重复 poll/query/pass、设备正常移除和 B 后续输入属于范围。

`tx-01-20` 是本文与 README 的设计交付，完成全部编辑后仅一次人工整体静态文档审阅。`tx-01-21` 是全部静态核验及 v11 交付，完成全部分析和编辑后仅一次人工整体静态文档审阅。过程零测试，W 编号不拆分验收。不设 gate，不校验任何哈希，不运行游戏或构建。

## 2. 当前进度与 v10 勘误

下列种子来自主智能体先前只读复核或本次源码/历史资料阅读，不是执行者新取得的二进制证据。执行者复读后按真实来源标注；缺少原始结果时保留为继承线索。

| 项 | 当前依据与问题 | v11 应完成的处理 |
| --- | --- | --- |
| Q01 device action 实参 | v10 报告第47行把 device slot 实参写成 action 指针；先前读到 A20975/A20AE9 为 `FF 30`，即 `push dword ptr [eax]`。 | callback 的 payload 含 action 指针；device +30/+34/+38 接收解引用后的 uint32 action 值。复读 +34 的对应点及各返回通道，W01。 |
| Q02 edge/held 位图 | v10 第122行称 action8–11 只有 edge bit0。当前 `active_apply.cpp` 用 `(1u << (action - 8))`，对应 edge bits0–3；`input_core.hpp` 的 held bit0 是 Shoot。 | 明确 Bomb/Item/CardOrPill/Drop 四个独立 edge 位；“没有这四项持续 held 表达”仍是实际功能问题，不能混成只有一个 edge 位，W01/W03。 |
| Q03 连续无 Room | v10 第102、106、140行关于无 Room 不清 queue 的表述过宽。当前 PauseGapTracker 阈值为2；正常 loading recovery 不阻断时，第二个连续无 Authority 的 TopUpdateExit 触发 PauseEnter/AdvanceOwnerGeneration。 | 区分第一次缺失与第二次缺失，以及 loading recovery 特例；追 dispatcher/planner/adapter/previous 等实际 clear 范围，W01/W03。 |
| Q04 Join 导航控制流 | v10 第128行及证据 E12 将 action20=true 写成终止整个 selector scan。先前静态读取定位到 9BB2DF→9BB334、9BB371→9BB567、9BB56E→9BB274，属于当前 selector 后继续循环。 | 复读分支，说明20/21在当前 selector 上是否跳过 confirm、如何到下个 selector；不能宣称扫描整体退出，W01/W04。 |
| Q05 本地解绑线索 | v10 W04遗漏 v2 已记的 95B310：954F84 调入；95B87F 调 setter(-1,1)，95BCA0 重分配；旧草稿明确 lookup miss 后还有分支条件、断线 UI 和可能的玩家移除。 | 从已有本地路径追完整条件，不把摘要中的 lookup miss 简化成无条件解绑，W02。 |
| Q06 快照有效期 | v10 推荐 Authority pre 唯一发布，尚未明确正常 post 之后、下一次 poll/Join、无 Room 时查询旧 current/edge 的有效范围。 | 给 writer、查询授权范围、post关闭点、previous 提交与中性规则；推荐候选不等于采纳，W03。 |
| Q07 状态分类 | v10 将未编写 production registration、多处候选契约变更列为静态缺边，且把候选直接称为内部一致。 | 已证 native insert/keyed lookup 应继承；真正未解控制流单列，候选须经实际时序分析；生产未实现、运行值未知不作静态缺边，W01及最终汇总。 |

复用已经有效的结论，不重做整个 ABI：Base/Control 表与布局；entry+378 内嵌 source pair 与 entry own key 分开；Manager dispatcher 和 cdecl callback receiver 分开；Net-derived context 元素的已见创建/插入/通知清理与 Offline empty-context guard；scalar ST(0)→callback output/AL→helper XMM0；poll=0 自动移除；显式 remove 不写 Player key；Player address point B6BDD0、ctor753DF2、Main Player槽+0C→782AF0→六消费者；唯一 K 的正常 Join selector/已分配 Player guard。

## 3. 资料、代码和工具定位

必读：本文、现行规范、[工作设计 v9](virtual-device-owned-abi-static-confirmation-next-work-design-v9.md)、报告/证据 v10、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[证据 v8](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md) E01–E03、[证据 v2](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md) W06/W07/W09 和 W06/W07 草稿 A-3。

| 对象 / VA 种子 | 应读取的具体内容 |
| --- | --- |
| 954CD0；954F7D/954F84 | poll A1FC00 与本地 reselect 95B310 的相邻调用及其实际 guard；显式 remove 不通过该调用栈。 |
| 95B310；95B879..95B87F、95BC9B..95BCA0 | 失败 lookup 到 unassign 的完整分支、设备类型/数量/Player guard、UI/reselect 状态，成功 lookup 到重新分配的条件。 |
| 9C25E0、9C2DB0及实际 Player removal caller | 从断线 UI/可能移除分支追 receiver、参数、Main Player/member 变化及后续正常执行；不要仅凭函数名称认定为 Player removal。 |
| 7A6450；956110/9561E0/9562D0；A20390/A20FB0 | propagate=0/1 的关联对象；正 K miss 与 -1 fallback；B 留在 Player vector 时是否还会查询 A。 |
| A20EB0、A1FC00、A212C0、913760 | 显式 remove 与 poll-zero erase 的通知、pair/context 清理及 Player key 不变，限定普通自有 Base cast=null 分支。 |
| A20940/A209E0/A20AB0 | callback action 指针解引用、device slot action 值、ECX receiver、AL/x87/output/XMM0。 |
| 9BAC40、9BAAA0；6FADA7/6FADAC | Offline guard、K/Player/selector 条件、19/20/21/14/15 分支、返回点。 |
| 9BB2B0..9BB371、9BB567..9BB56E | 导航后当前 selector 与下个 selector 的分支；完整读取分支目标，不只列 immediate 或 xrefs。 |

源码按实际内容读取，行号仅为定位：`input_core.hpp:15–26` 与 `active_apply.cpp:37–80`；`input_core.cpp` Accept/Resolve/EdgeTargetPlanner；`active_dispatcher.cpp` SubmitPhysical/SubmitEncoded/Drain/Resolve/ClearForGeneration；`active_runtime.hpp` PauseGapTracker 与 loading recovery；`active_runtime.cpp:553–581,709–852` 的 generation/Authority/TopUpdateExit；`loop_runtime.cpp` TopUpdate/ManagerSample/StandardRoom hooks；`active_apply.cpp` BeginAuthority/EndAuthority/Evaluate/ClearGeneration。不要执行其中的 self-test 入口。

本次设计期间当前工具列表未发现 IDA Pro MCP/x32dbg。执行者先按 isaac-cs-refactor 技能发现自身实际工具；可用时只做 IDB 元数据、函数、bytes、decompile、xrefs 的静态读取，不改 IDB。先前目标为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`、imagebase=400000；新读取以实际工具返回为准，不校验哈希。

若仍无 IDA，先读取仓库保存的原始导出、证据和源码，继续所有不依赖新二进制读取的工作。可定位资料包括 `out/cs-t0/_ida_scratch/r03/results/xrefs3.txt`、`r03_notify.txt`、`xrefs3.json` 及 `out/cs-t0/_ida_scratch/r05/results/v7.json`；它们有95B310相关命中，**不保证保存完整函数或分支**。先 `rg -l` 定位文件，再读相关字段/区间，避免整行巨型 JSON 导致截断。静态解析导出属于阅读；不得运行旧工具脚本、隐藏本地 MCP 请求、安装工具或把 scratch 中的服务信息当成已授权连接。新读保存的原始导出应标 `ARTIFACT-READ`，不是新 `IDA-DIRECT`；报告摘要仅为 `STATIC-INHERITED`。若资料不足，列到最后已证分支与需要补读的具体指令区间，不能伪称完成。

## 4. 执行工作

### W01：勘误、继承结论和真实工作分类

完成 Q01–Q04 的四项事实勘误：分别给错误原句、正确结论、实际 raw/source 依据及影响范围；旧 v10 原文保留，v11 开头和证据中明确纠正。callback/device ABI 表不得继续把 action 指针传到 device slot。

held/edge 表至少列 Shoot held bit0、Bomb/Item/CardOrPill/Drop edge bits0–3，以及对应 action8–11。依据查询消费者区分触发与持续量需求，不预先扩大全部 schema；确认 Drop held 和其它需要持续语义的正常 reader，列最小版本化候选，不能新增 magic action/key。

在分析中复用已证注册/lookup链。若自有特定 type/name/binding 对某个原生 reader 的条件仍未证，要列该 reader 与实际类型分支；不能用“尚无 production 实现”代替这个分析。最终 P01–P06 表将静态缺边、设计/契约差异、现场值和实现任务分别列出。

交付：四项勘误表、ABI/位图表、已证链与新增有限静态缺项。

### W02：95B310 本地断线/reselect 与 B 正常退休

优先读取95B310完整相关分支，恢复“设备查找失败→各 guard→断线 UI/可能 Player removal→setter(-1,1)”和“重新找到设备→setter(K,1)”两条正常路径。区分键盘/Gamepad、自有 Base 类型条件、主 Player/关联 Player、selector、context vector、UI/reselect 标志。旧草稿的“无 players/reselect”摘要必须还原为具体对象和布尔条件，不能据此外推所有 lookup miss 都解绑。

给路径表：入口/调用时点、receiver/参数、guard、Player vector及ordinal变化、controller key、关联 Player/NPC、selector/context变化、函数返回后的下一正常读者。沿实际 caller判断可能移除的是哪个 Player；有 UI 或全局状态变更时说明是否阻止 A 继续正常游戏。

比较至少两种次序的可达性：①合法 B-specific Player退出/解绑后显式remove设备；②先关闭自有输入并remove设备，随后由主更新95B310处理失去K的B。按证据选可行序列，不预设必须B先处理或必须remove先做。后者必须解释remove到95B310之间的查询、通知/context状态、分支是否要求额外用户选择，以及-1之后B是否仍在活动Player vector并读取A。

退休第一步不等待路径查明：RETIRING立即关闭admission/Join/publication、generation失效、自有query中性；仍登记期间poll非零；NEVER对象/control/DLL常驻。正常Pause只中性且保留登记，不用poll=0表达Pause。仅key=-1、仅取消selector、仅Manager remove、仅中性但永久保留登记都不能冒充完整退休。

分别交付“B独立退休且A继续”和“whole-run正常teardown”的结论。若本地断线逻辑会暂停整个run、要求人工reselect或只支持某设备类型，如实说明；不拿Online失联全Player reset替代本地方案。不裸写Player字段、不发明sentinel、不绕过原生guard。

交付：本地分支条件表、正常退休时序、B/A后置状态、适用条件；未解时列已读函数、最后已证callsite、缺失的具体边及最小补读范围。

### W03：S2 的查询有效期、previous、无 Room 与 pause

读取当前源码，区分native poll的C4←C0与自有S2、现有SyntheticAdapter与未来自有vtable。继承破坏性Resolve/lead=2/drain已证结论：不把Resolve当peek，不让同target tick重复解析，不让queue消费后凭candidate未commit就假定可恢复。

将v10推荐的Authority-pre方案补全为候选：进入有效Authority后统一Resolve并发布，正常post提交tick和previous；明确post至下一poll/Join、连续无Room、正常pause期间，哪些query可读本次current/edge，何时必须中性，谁关闭有效范围。给一个具体的query有效条件与关闭点，说明main-thread/owner/generation/scope是否足够、原生消费者是否在其范围内。重复query只读不能证明跨scope旧edge不复现。

对照现行contract v2的“poll唯一写锁存”，列出Authority writer迁移所需的最小职责/API/字段及契约差异。若保留poll writer更合适，说明预发布如何在Join/Room前可见而不进行破坏性预Resolve，所需staging/reserve/commit等新增机制必须明列。最终推荐一个内部一致的候选，但不编辑或宣称用户已采纳contract/schema/生产源码变更。

当前PauseGapTracker需完整纳入：第一次无Authority通常不触发generation clear；loading recovery不阻断时第二次连续缺失触发PauseEnter；recovery分支另有控制规则。追ClearForGeneration/ClearGeneration实际清queue、planner、pending、snapshot/previous的范围，区分源码事实与候选的新中性规则。不能把所有无Room都归成“队列保留”或“立即清空”。

状态表至少记录：completed/candidate/target tick、update/publication epoch、generation、queue/planner/pending、previous/current、query是否有效、scope、post关闭、Join pass状态。手推：press→hold→release→repress；同update多poll/query；正常post后再次query；第一次无Room；第二次连续无Room；loading recovery；无输入但正常Authority；Pause退出后首个Authority。这些是纸面静态推导，不运行模型或测试。

交付：当前源码时序、完整候选字段/owner/有效期表、正常轨迹、契约差异与仍未证窗口。普通消费者action映射和Room→Player链继承有效证据，仅复读影响新scope的具体边。

### W04：Join 的目标 K、pass 关闭与导航/确认效果

纠正action20/21控制流，逐当前selector解释select/navigation/confirm/cancel顺序。原生action19和14可在同一pass出现时，给具体guard与selector变化；确认后K进入Player并selector失活，是正常唯一K条件下的重建抑制，不是所有命令的一般exactly-once证明。

为独立JoinCommand候选列字段和owner：session/generation、目标K、command sequence、target update epoch、join pass id、动作种类、准入/首次pass绑定/查询/Complete/无pass UpdateExit关闭点。已有游戏scheduler不能被绕过为B即时输入；解释首次B尚不存在时Join命令如何在native pass前可见，以及它与gameplay target tick的关系，不把update epoch当Authority tick。

限定query结果只给命中的自有K和有效pass，其他key不能因本命令得到true；原生多个selector正常遍历时仍分别分析效果。query保持只读，重复查询不消费命令；说明命令在哪个owner消费/关闭。下一pass、新update、无pass update、RETIRING/generation变化不得复用旧命令。不要宣称command提供原生代码中不存在的任意副作用屏障。

与W03合并手推：19+14正常唯一K加入；导航后继续下selector；仅取消当前selector；同pass重复query；同update第二pass；本update无pass；另一个key已有selector；retire与待执行命令相交。guard不满足则记录本次未加入与后续命令状态，遵守每session至多一次创建/失败不自动重试的现行设计，不重新推演full B Init假设。

交付：有限原生分支图/条件表、JoinCommand候选、K/pass隔离轨迹及与gameplay一致的关闭规则。网络双端exactly-once不在本轮范围。

## 5. 产物与结果格式

- 新报告：`docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v11.md`。
- 新证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v11.md`；必要分册使用同一v11前缀，全部纳入报告引用。
- 更新 `docs/refactor/cs-t0/t0-05/07-virtual-device/README.md` 的实际执行状态、报告入口与最终审阅结果；保留v10及更早原文、失败/异常和审阅历史。

证据逐条给对象、VA/RVA/关键bytes、caller、receiver/参数/返回、guard、writer/reader、推导范围和来源。`IDA-DIRECT`只用于本轮实际工具读取；`ARTIFACT-READ`给保存的原始导出文件/字段/范围；`SOURCE-DIRECT`给实际源码位置；`STATIC-INHERITED`给旧版本/节及终点；CONTRACT、DESIGN_ONLY、UNKNOWN不混写。

报告开头给Q01–Q07/W01–W04完成情况及真实静态程度。P01–P06分别为注册枚举、type/name/binding、Offline Join、key/B ordinal、snapshot/query、pause/退休/context；每条分开列已证、实际静态缺边、候选契约差异、现场值、生产工作。不能用“所有条目都有UNKNOWN”证明核验完成，也不能把无新IDA读数伪称已重新确认。

结尾回答：哪些实现可按已证ABI开始；还有哪些具体正常静态边需要补读；哪些仅需设计采纳/实现；哪些留给整体验收观察。自有生产register/Join lane/S2/retirement不存在属于实现工作。当前实例的vector/listener/key/selector值未观察属于现场事项。正常功能静态完整性、候选一致性/采纳和生产可运行性各自独立，不合并成一个PASS。

## 6. 执行所有权与边界

主智能体完成本文后，以新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行 `tx-01-21`。不另派子智能体或新侧栏聊天，不恢复已完成的旧v10任务。主智能体交出v11文件及topic README编辑权；子智能体使用IDA时拥有该次只读使用权，避免并发控制同一会话。

允许资料/源码读取、静态反汇编/反编译及本轮文档编辑。不得改生产源码、contract/schema或旧报告；不得patch/rename/set_type/define_func/save IDB；不得启动或检查游戏进程、读取进程内存、附着/断点/单步、输入游戏、注入或安装Hook。不运行构建、test/harness/probe/smoke/canary/verifier、哈希/链接检查脚本或可执行状态模型。PowerShell可用login:false，不改变用户环境。

发现工具缺失继续可完成的证据/源码工作。无法补齐的原始分支真实留未解，不把工具缺失变成新增项目流程条件，也不虚构正常退休链。

## 7. 唯一整体人工静态文档审阅

`tx-01-21` 全部分析、纠正和文档交付后一次人工整体通读：本文、v11报告/证据/分册、关键source/contract、README。核对四项勘误、本地95B310的guards/对象/A影响、退休次序与-1 fallback、快照writer/有效期/post/previous、连续无Room/recovery、Join K/pass/nav/confirm/关闭、来源与P01–P06分类。分析编辑中正常纠错；最终审阅后仅回填结果，失败记录后不自动修补或重审，不仅换编号重复验收。

审阅结果分别写文档准确性、正常功能静态程度、候选一致性/未采纳、现场未观察、生产未实现。全部材料读完不等于所有功能已证；实际静态缺边存在时如实报告未完成。不得预填PASS。

## 8. tx-01-20 设计交付记录

本次只新增本文并更新topic README，提供已知勘误、local-co-op定位种子和执行范围。无新IDA二进制直读，无源码/contract/schema修改，无构建、测试、哈希脚本或游戏操作。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文及README修订范围，对照现行规范、contract v2、v10复核、v2本地断线记录和本次源码阅读，检查Q/W/P对应、四项勘误、95B310条件与退休顺序要求、S2有效期/连续无Room/recovery、Join K/pass关闭、证据来源、产物所有权及实际引用路径。执行设计未把未实现、未采纳或现场值未知当作静态缺边，未将95B310候选写成已证B独立退休方案。此结果仅评价设计交付，不表示tx-01-21核验或正常功能已完成；审阅后仅回填记录，没有第二次审阅或其他测试。
