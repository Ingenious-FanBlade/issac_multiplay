> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v16

日期：2026-10-15  
设计交付推进：`tx-01-37`  
静态执行推进：**续接未完成的 `tx-01-36`，仍交付确认报告/证据 v16**  
状态：`DESIGN_ONLY / v16 执行续接设计`  
执行者：用户指定的 **GPT-6-Luna、max** 替代子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文与[完整工作设计 v15](virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)共同作为执行入口：v15 的 Q01–Q06、W01–W04、正常范围与详细交付要求继续适用；本文补充中断交接、已恢复的 IDA 工具状态、执行次序和具体完成口径。冲突以现行规范、用户会话与本文的新交接信息为准，不修改旧设计。

## 1. 当前进度和推进安排

最近完成的确认交付仍为[报告 v15](virtual-device-owned-abi-static-confirmation-v15.md)及[证据 v15](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v15.md)。`tx-01-34` 复核确认三条正常功能缺链：native remove 的引用账本、无 B Player TLS 的首次 JoinControl 查询、A 的实际 input query/reselect 后置；并纠正 HUD slot 与 input receiver 混淆。具体事实见工作设计 v15 §2 和 topic README 复核记录。

此前 v16 子智能体状态查询返回 `interrupted`。2026-10-15 本次检查：live agent 列表无该实例；未找到报告 v16、证据 v16 或 README 的 tx-01-36 最终交付/审阅记录。不推断旧实例究竟读过哪些未落盘内容，不把启动、中断或设计交付当成核验进展。

本次新增设计是 tx-01-37；替代执行者**继续原 tx-01-36 的完整目标**，不改成报告 v17，不另开执行编号来增加验收次数。原执行唯一整体人工审阅尚无已完成记录，仍只在本轮全部分析和编辑结束后进行一次。若发现未列出的真实草稿/执行记录，先阅读并保留其事实，沿准确终点继续；不得覆盖已有最终记录或重跑已完成验收。

完整目标仍是交付一套正常功能静态依据和内部一致候选：①by-value pair/通知/移位/erase 引用责任无重复；②B 未创建时控制命令能经自有设备返回 action19/14；③B 退休及 remove/reselect 后 A 的 key、pair 和真实查询 receiver 有明确条件；④三者与统一 S2、lifecycle、attempt、Hook 恢复 owner 接合。本文不采纳新架构、不修改 production source/contract/schema。

## 2. IDA 恢复事实与使用方式

本轮开始时未发现可调用 IDA MCP；用户随后明确通知已启动并要求继续使用。本轮重新发现实际工具，先调用 `survey_binary(detail_level=minimal)`，再 `server_health`，随后成功静态反编译 `A20EB0`。当前返回：

| 项 | 2026-10-15 实际工具返回 |
| --- | --- |
| 服务 | status=ok，auto-analysis ready，Hex-Rays ready |
| 输入 | `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe` |
| IDB | 同目录 `isaac-ng.exe.i64` |
| 映像 | 32-bit，base `0x400000`，image size `0x8FE000` |
| 本次直接读取 | `A20EB0` 正常函数体；其中 control+0x0C 调用在 A20F56/A20F82，n79_0 是随后条件回调 |

未校验任何哈希。该复读只确认执行设计的定位方向，**不表示完整引用账本已完成**。子智能体继承它时标明 tx-01-37 来源；自己的 IDA-DIRECT 必须对应自己的实际成功读取。此前 tx-01-34 的详细读取摘要属于历史静态证据，不冒称本轮新读。

执行者先发现当前工具/schema，首个分析调用仍用 survey_binary minimal，仅使用非哈希定位信息，再看服务状态。关键 ABI、栈、guard 和 continuation 读取 raw/指令；decompile 参数猜测、disasm cursor/done 或输出截断都不能代替目标区间。可用 get_bytes、纯静态 py_eval/IDA Heads 补足，记录确切地址、对象、调用/返回和限度。

本轮使用 IDA **只读 IDB**，不附着游戏、不读进程内存、不设断点/单步、不 patch/rename/set_type/define_func/save IDB。工具中断时记录已读终点，继续不依赖该工具的 source/候选工作；不要无间隔反复轮询，也不要因服务状态变化重做所有已证事实。

## 3. 继承约定与不重做的部分

完整适用范围见工作设计 v15 §1。保持有效分配/对象/缓冲、主线程顺序、Offline/local-co-op、Online roster 为空、B 唯一正 K、创建前 Main vector 仅 A。Online roster、context、Main Player 和 InputManager vectors 分开判断。沿[用户接受的原生 Join 可靠创建/发布 B 假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，不重做 full Player Init、全局 manager-this 或 05-06 全网络证明；该假设不代替控制命令→query 接线。

保持38槽/六覆写/6-slot control、S2、NEVER、base ctor 前固定 DLL、ctor 后自有 vptr、BASE_CONSTRUCTED 后对象/control/有效 base 状态不析构不释放、DLL常驻、每 session 至多一次创建设备且失败不重试。A0/B1、ClientId session 冻结，同 canonical/sequence/target tick/queue/dedupe/Resolve/apply，无即时旁路；C/S 不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。

继承已证布局、register/keyed lookup、两个 Popup 空 callback/payload、普通 Variant0 B 的 9BA000 Main erase ABI、+0x28 cleanup、下一 Main dispatch 只见 A、TopUpdateExit 在完整 update 返回后、9BB415 deny 补栈到9BB561→9BB567 的局部 no-create、raw/canonical 去重与 held V2、六 consumer/action 表。只复读本轮真正依赖的片段，不把全部基础重新列 UNKNOWN。

正常 pause/loading/noRoom、重复 poll/query/pass、首次 Join、B 退休及 A 下一查询在范围。排除 OOM/EH、worker并发、溢出/ABA、全映像alias、持久化、异常恢复、全部Mod/Lua行为和完整网络证明。原版 Leave 按钮可达不是本轮条件；可设计自有主线程 coordinator 调用已证 helper。

## 4. 执行内容与具体交付

沿工作设计 v15 的 W01–W04 编号推进，以下表是最低交付内容及所用有限种子；详细场景和约束不能仅以表中摘要代替。

| 内容 | 必须实际解决的问题 | 交付与种子 |
| --- | --- | --- |
| W01 引用与退休 | `A20EB0` 会调用 control ReleaseStrong，不能只看 n79_0；by-value 参数需要独立所有权，native 已消费的 vector strong 不能又由旧 ReleaseClass 消费。 | 逐事件手工账本、实际 ABI/调用形状、bridge API 和“只记录已消费责任”的 owner；A20EB0、902D60、A21570、906020、A20390、Manager +0x7C/+0x80。 |
| W02 首次 JoinControl | B 尚未创建、无 Player/consumer TLS，owned slot 仍须能在有效独立控制 scope 内返回19/14；不能所有 scope 外都中性，也不能以19查询已成功才建立scope。 | scope entry/close、reader优先级、Pressed/Triggered/Scalar动作表、command/pass/attempt轨迹；6FA540/9BAC40、9BB064、9BB0B8 +0x54 flush、9BB39A/9BB3C9、9BB415/9BB429、6FADAC。 |
| W03 A实际 input链 | 9BAAA0返回HUD/controller slot，不是physical receiver；remove通知/reselect后A真正走哪个keyed/fallback分支，何时唯一重绑。 | key/context/pair场景表、真实receiver及native setter caller/K来源；956110/9561E0/9562D0→A20FB0/A20390→A20940/A209E0/A20AB0→device slots，95B310/95BC4D/95BCA0/95B87F、913760、7A6450。 |
| W04 合并与分类 | 一套coordinator/JoinControl/S2/lifecycle接口和writer责任，继承去重/held，不以新API未写或live值未知冒充native缺边。 | 最小API参数/调用阶段/owner/终态、source修改责任、Q01–Q06与P01–P06分类、新交付引用勘误及有限未读列表。 |

### W01 的完成要求

沿正常 caller 对 pair lookup-local、by-value argument、两个通知 temporary、移位 temporary、vector、root 及其他已有正常引用 R 分别列 acquire/release。区分 B pair 在尾/中部，移位中 receiver 不必总是 B；给整体净增减和调用返回后责任。n79_0 是否调用不能代替此前的 ReleaseStrong 事实；若回调对本轮账本有实质影响，限定读它的 writer/目标，不扩到全引用 universe。

明确 `RecordVectorRefConsumedByNative` 一类仅更新私有账本的动作，和实际 `ReleaseStrong` 是不同 API 责任。旧 `virtual_device_lifecycle.cpp::ReleaseClass` 会调用 port.ReleaseStrong；旧 RemoveAndDestroy 在 Unregistered 后还会 FinalRelease，不可整条移植到 NEVER。引用计数不能因为内存常驻而随意多减，root/control+0x14 状态仍须正确。

接回 TopUpdateExit 的串行退休顺序：关闭B admission/visible并使generation失效 → native notify/remove及临时引用责任 → exact B pair absence → UNREGISTERED → native B member/context/ordinal cleanup → A 下一 reader。顺序调整给正常数据依赖；重复退休/已缺席不得再次消费同一vector责任。给 source 修改责任，不修改 source。

### W02 的完成要求

在 action19 query 前建立 `JoinControlScope`，字段至少包含 session、B预留ClientId、own receiver/K、updateEpoch、passToken、commandSeq、动作和可见期；不依赖B Player*。明确 owned reader 选择顺序：有效控制scope读control lane；有效gameplay scope读该participant S2；其他/retired/stale中性。A物理UI保留原行为，硬件采样不读合成S2；控制动作不塞进0–11的gameplay state或Resolve。

说明19/14/15/20/21各query kind结果，重复read与命令关闭、+0x54 native C0/C4 flush与私有lane的关系，保持六覆写，不静默加第七槽。避免control输出8/10组合触发另一创建路径；若同一控制输出可到其他已读create，列具体guard/call和正常范围，不将局部9BB415 admission夸成全游戏唯一入口。

继承actual-call admission和合法deny continuation，补齐19+14首次Join、重复query/flush、导航取消、同update第二pass/no-pass/另K/stored confirm、成功后再confirm、retire与pending publish的手工轨迹。每步写scope/token、query返回、是否actual call、bind/publish/terminal。query只读与actual side effect owner分开；9BB429只是bind观察，pass正常close及用户给定发布后置成立后才Succeeded。

### W03 的完成要求

实际input链与HUD链分别出表。覆盖A key>=0（含0）/-1，context空/非空，B pair present/removed，A retained/rebind pending；helper特殊前置分支和fallback按实际guard分别写，不默默新增“A必正K/context必空”的用户假设。唯一绑定不足时给候选中性/重绑状态，不能直接把fallback当统一调度结果。

沿remove通知、source pair reset、95B310的selected-device lookup与setter caller，接到下一poll→reselect→Join/独立Room→Main `[A]` update→consumer/query，说明A key/pair何时保持或改变、何时重新Submit/Resolve、实际receiver读A S2或neutral。A/B正常TLS切换不清另一人queue/generation；B迟到查询不读A或旧B。有条件但已读清的数据流标STATIC-DERIVED；现场成员/实际命中值单列，不能为补现场值开启运行工具。

### W04 的完成要求

统一Session frozen map、participant队列/sequence/replay/dedupe/planner/pending、共享Authority tick、JoinControl scope、gameplay query和retire账本。给关键API最小参数、唯一owner、调用阶段、有效条件、normal-post与lifecycle的互斥/终态，不只列概念名。

沿候选Authority-pre唯一current writer、normal-post previous/close、+0x78仅活跃poll条件；retire使generation/visible失效，不在pre另写previous。明确pause/loading/binding generation变化后的previous tag或中性读取，防止跨generation edge。与contract v2 §4 poll latch writer差异继续DESIGN_ONLY/未采纳，不修改contract，也不把采纳作为本轮阅读前提。

继承raw/canonical/held V2和cross-family语义，只核对control动作不混入gameplay、重复query不Resolve/noRoom可能已Submit但未Resolve、第二连续missingTopUpdate按source阈值2处理pause。报告开头回答Q01–Q06/W01–W04；P01–P06保持六功能主题，各分native已证/未读边、候选未采纳、现场未观察、production未实现、后续整体验收事项。不能仅因写完artifact就称功能完整。

## 5. 文件、交接与操作范围

执行者只可新增/编辑以下交付：

- `docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v16.md`。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v16.md`；必要分册同v16前缀且全部引用。
- `docs/refactor/cs-t0/t0-05/07-virtual-device/README.md`，增加tx-01-36续接的实际结果，保留中断、tx-01-34、两份设计与所有历史记录。

报告入口同时引用本文与完整设计v15。证据从out/cs-t0链接仓库docs时用 `../../docs/...`；新报告到证据用 `../../../../../../out/...`，人工核对，不运行链接检查脚本。保留旧v15错误链接，勘误在新交付说明。

先读项目技能与架构/T0资料，再读v15完整设计、本文、v15报告/证据、contract及相关source。启动新上下文 `model=gpt-6-luna`、`reasoning_effort=max` 替代子智能体；旧实例当前不在live列表，不假称恢复其内存或已读终点。不再委派，不创建侧栏聊天。主智能体交出v16文件、topic README编辑权及同一IDA只读会话使用权，子智能体执行期间不并发编辑或操作IDA。

不改旧交付、工作设计、project conventions、production source、contract/schema或IDB。不运行测试/构建/harness/probe/smoke/canary/verifier/hash/link-check或可执行状态模型，不操作游戏、debugger、注入或Hook。纯静态文件/source/IDB读取、rg、反编译/raw及手工表格推理允许；PowerShell可用login:false。用户已要求IDA静态核验，无需重复确认，但该指示不授权运行时操作。

## 6. 唯一整体审阅与续接结果

tx-01-36全部分析和编辑完成后仅一次人工整体静态文档通读：两份工作设计、v16报告/证据/分册、相关source/contract及README。审阅引用账本/无双减、无B TLS的JoinControl返回、A真实query/重绑后置、S2/lifecycle/writer/attempt/恢复owner一致性、raw证据范围、分类、引用及中断历史。审阅前可修正文稿；唯一最终审阅后只回填实际结果，失败不自动修补、复审或换编号重跑。

分别报告文档准确性、正常功能静态完整程度、候选内部一致性/未采纳、现场未观察、production未实现。不得预填PASS；未读项给具体函数/字段/分支/地址及已读终点，说明哪项新增事实消除了旧问题。已完成的基础ABI可开始编码与完整B功能已确认是不同结论。工具再次中断也须保存已完成的证据和草稿，准确交接，不留一个没有产物的“正在执行”。

## 7. tx-01-37 设计交付记录

本次新增本文、更新README续接入口与中断历史。确认未见旧v16最终交付；用户恢复IDA后已成功读取元数据及A20EB0，但完整执行结果仍待替代子智能体交付。未运行测试、构建、哈希校验或游戏操作，未修改旧交付、source、contract/schema或IDB。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文与README修订范围，对照现行规范、完整设计v15、v15复核、contract及相关source，核对中断记录/tx-01-36续接与一次审阅安排、IDA恢复事实、引用账本、首次JoinControl无B TLS接入、A实际input链与reselect、S2/lifecycle/writer/恢复owner、事实标签、引用及操作范围。审阅后仅回填结果，无第二次审阅；不是tx-01-36执行验收，不代表完整正常功能已确认。
