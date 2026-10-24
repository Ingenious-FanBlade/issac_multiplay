> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v6

日期：2026-10-12  
设计交付推进：`tx-01-08`  
静态执行推进：`tx-01-09`（一个完整目标，R01–R06 是内容编号）  
状态：`DESIGN_ONLY / 正常功能证据修订设计；静态确认 v7 待执行`  
执行者：用户指定 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。继续遵循用户指定的正常功能研究范围：本轮处理 v6 的事实错误、遗漏的输入消费链、binding 来源、正常停用语义及相应接口设计边界；不重开安全证明。

## 1. 完整目标与预期结果

纠正[正常功能静态确认 v6](virtual-device-owned-abi-static-confirmation-v6.md)及[证据 v6](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v6.md)中已定位的错误，补齐 `action → 原生 consumer → Player key/设备解析 → device query → 自有快照` 的逐项关系，查明正常解绑/移除后的 B 输入行为，并把需要由自有代码决定的 Join/poll 接口与原生事实分开。

新交付为正常功能静态确认 v7、证据 v7与当前目录入口。报告应可用于最小自有设备实现，准确列出正常调用 ABI、必要槽/字段、binding 初始化条件、Join 输入来源、快照查询阶段以及停用后的功能结果。不能把文档已写、缺口已列、生产未实现或运行值未观察混为同一种完成状态。

`tx-01-08` 的完整目标是交付本文与目录入口，全部编辑完成后一次人工整体静态文档审阅。`tx-01-09` 的完整目标是完成 R01–R06 与 v7 交付，全部分析/编辑完成后一次人工整体静态文档审阅。不得按 R 编号拆成多次验收，不运行过程测试。

保留 v6 已有且本次核查支持的事实：普通 Join 直接枚举 InputManager active vector，slot7 的辅助表插入不是普通 Join 前置；Join19、confirm14、cancel15、导航20/21；key由device+0x0C经selector传给SetControllerId；原生slot21只执行native C4←C0；当前C/S gameplay adapter覆盖action0–11。这些可按实际引用继承，不机械重读所有历史槽表。

## 2. 范围、分析条件与现行流程

正常分析条件：分配成功，原生对象及缓冲有效，所研究操作按正常顺序执行；既定 Offline/local-co-op 分支的 online roster 为空，使用可唯一关联的 virtual key，创建B前主Player vector恰有A一个Player。条件不是本轮运行时观察。继承[原生 Join 用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，只把可靠创建/发布B作为DESIGN_ONLY规划前提，不重做完整Player Init与持久化证明。

OOM、EH/unwind、锁残留、worker并发、原子镜像/计数溢出、ABA、全映像别名/未来模块、持久化排除与异常退休不展开；已有问题和失败记录不改判。正常分支中的真实参数、返回值、对象关系、容器读写和触发条件仍须核验。

本轮只做静态资料分析和文档编辑。不改源码、测试、contract、verifier、machine decision、权限或既有报告；不启动/检查游戏进程，不读取进程内存，不附加调试器、输入、注入、Hook、IPC或运行期修改；不构建、运行测试、候选模拟、harness、probe、smoke、canary或旧verifier。

自有38槽对象表、6槽control、S2、NEVER、DLL常驻、每session至多一次创建及C/S与原版/Isaac Sync隔离继续作为设计背景。NetInputDevice和Online Manager不进入C/S正向实现。05-06的完整双Endpoint scheduler隔离与exactly-once总证明不在本轮；只分析最小Join/快照接口所需的正常时序。

现行规范要求**不计算、不校验任何哈希，不以历史哈希/gate/解锁字段推进**，并允许历史字段作为资料阅读。本文不沿用旧工作设计额外的“禁止读取历史哈希文字”要求；尽量只读相关段落，避免输出无关历史值。v6在其当时执行约束下记录的偏差原文保留，不回改历史审阅结果，也不把偶然出现历史字段变成新的停止条件或测试事项。

## 3. 核查问题、输入资料与定位种子

### 3.1 v6 修订表

以下来自上一会话的只读核查，作为执行种子；执行者须复读关键指令，不能标为tx-01-09新直接证据。旧imagebase为0x400000，以下地址除明确RVA外均为IDB VA。

| 编号 | v6位置 | 已定位问题与正确边界 | 执行内容 |
| --- | --- | --- | --- |
| Q1 | 主报告P01/第43行；证据§1.2第55行 | insert后的+0x78通知来自Manager vptr；A20E55读取[edi]，A20E68令ECX=edi；不能写成device vptr | R01 |
| Q2 | 证据§2.3 ctor行/第134行 | C7362C是key计数器，不是Base vptr。A1E084将B82758写入[edi]；A1E08E将计数器值写入device+0x0C | R01 |
| Q3 | 主报告P02/第59行；证据§1.3/§2.3 | C799F4是update辅助表计数。binding默认vector为C799E8/C799EC/C799F0；对象n=(end-begin)>>4。数量与内容来源要沿正常初始化独立追踪 | R02 |
| Q4 | 证据P05/第220行；主报告审阅第139行 | 只有六个consumer RVA集合，缺action/group、owner callsite、参数、key解析与query对应 | R03 |
| Q5 | 主报告第19行 | 证据相对链接少一级..；从design-revision到仓库out需../../../../../../out，README到out需../../../../../out | R06 |
| Q6 | 主报告P05/P06 | Join epoch及query只读尚为候选；SetControllerId(-1)后B输入是否中性未证明。A20FB0把-1解释为枚举全部active devices，不能直接把setter当中性停用 | R04–R05 |

### 3.2 原始指令种子

| IDB VA | 上轮读到的原始字节/行为 | receiver与含义 |
| --- | --- | --- |
| A20E55 | `8B 07` | EDI为Manager，EAX取Manager vptr |
| A20E5C | `8B 70 78` | ESI取Manager slot30/+0x78通知target |
| A20E68 / A20E6A | `8B CF` / `FF D6` | ECX=Manager，调用ESI；按值pair已准备 |
| A1E07C | `8B 15 2C 36 C7 00` | 从C7362C读key计数器到EDX |
| A1E084 | `C7 07 58 27 B8 00` | 写Base vptr=B82758到对象首dword |
| A1E08E | `89 57 0C` | 将key写入device+0x0C |
| A1E091 / A1E09B | `8D 42 01` / `A3 2C 36 C7 00` | 计数器递增并写回；不讨论并发 |
| A1E440 / A1E44F | 读C799E8 / C799EC | slot5读取binding模板begin/end；与C799F4无关 |
| A1E455–A1E45C | `mov ecx,edi; sub ecx,edx; sar ecx,4` | 模板元素数来自(end-begin)>>4 |

### 3.3 六类 consumer 的既有准确映射

来源：[T0-04 handoff §7](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md)。原表使用RVA；下列VA按旧imagebase0x400000换算，执行时记录实际IDB并核对。继承映射不等同本轮六条完整消费链已复读。

| consumer | actions | owner callsite RVA→target RVA | 对应IDB VA callsite→target | 正常ABI |
| --- | --- | --- | --- | --- |
| movement | 0–3 | 385BC2→379B40 | 785BC2→779B40 | void thiscall(Player*) |
| bomb | 8 | 385BC9→380F40 | 785BC9→780F40 | void thiscall(Player*) |
| item slot | 9 | 385BD2→391A40 | 785BD2→791A40 | void thiscall(Player*,int slot) |
| pill/card | 10 | 385BD9→3A3D30 | 785BD9→7A3D30 | void thiscall(Player*) |
| shooting intent | 4–7 | 385BE0→37B020 | 785BE0→77B020 | void thiscall(Player*) |
| drop | 11 | 385BE7→3A42B0 | 785BE7→7A42B0 | void thiscall(Player*) |

### 3.4 必要资料

- [项目规范](../../../../../project-conventions.md)、[架构规划](../../../../../isaac-cs-architecture-plan.md)、[T0任务书](../../../../isaac-cs-t0-task-plan.md)、[cs-t0入口](../../../../../../../cs-t0/README.md)。
- [主报告v6](virtual-device-owned-abi-static-confirmation-v6.md)、[证据v6](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v6.md)、[执行设计v5](virtual-device-owned-abi-static-confirmation-next-work-design-v5.md)：纠错位置、有效事实与历史未通过记录。
- [静态确认v2 §10](virtual-device-owned-abi-static-confirmation-v2.md)、[W09证据草稿](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)：Join、selector、action、key、setter与正常解绑。
- [证据v5](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md)、[证据v4](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md)：receiver、C0/C4、正常注册/Init、相关caller；受后续勘误影响的文字不直接继承。
- [T0-04 handoff](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md)、[input_core.hpp](../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp)、[active_apply.cpp](../../../../../../../cs-t0/native/src/active_apply.cpp)：六类消费者和当前gameplay0–11、四个Q15轴及四个edge flags，避免称为八个输入轴。
- [契约v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[设计修订v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)、[原生Join规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)、[目录入口](../README.md)：行为背景、query只读和已接受创建前提。

## 4. 执行内容

### R01：修正原生receiver、vptr与key来源

1. 从A20DF0入口追EDI/this来源，复读A20E4C–A20E6C与前后按值pair准备；分别记录Manager、Mutex、device、control、栈临时pair，确认通知的表项与receiver。
2. 从A1E050入口追对象this，复读A1E07C–A1E09F；把Base vptr的立即数与key计数器读写独立列出，不从同一段代码中的全局地址猜字段用途。
3. 新报告修正Q1/Q2全部受影响的摘要、原始证据解释、F2/注册顺序和导航。旧v6不改，修正表附原位置、实际字节与影响。
4. 应用既有slot号/COL勘误：device+0x94=slot37、+0x64=slot25；ManagerBase表B827F4与派生表B82950；B827F0/B8294C是COL pointer cell，实际COL为BB033C/BB0424。只在本轮使用处核对，不重做所有38/6槽。

交付：两个准确的寄存器/字段来源表及Q1/Q2勘误；主报告、证据和摘要使用同一receiver与数值。

### R02：binding模板的正常初始化与实际数量来源

1. 沿C799E8/C799EC/C799F0的实际数据xref定位模板构造/填充/修改，优先核对旧资料中的4307C0及相关初始化callee；C799F4的辅助表初始化单独列出。
2. 从模板begin/end/cap追到A1E430复制进device+0x10/+0x14/+0x18，再到A1E210按对象n分配C0/C4。记录正常初始化顺序、元素stride16、元素数量、必要primitive内容来源及已知修改点。
3. 分析能否从固定正常初始化分支推出32项或其他明确数量；若来自配置/参数，给具体reader、参数来源和条件。不要看到IDB初值0就停止为“运行时未知”，也不要把动态值伪称常量。
4. 与835C90注册记录的固定32项读取对应：device每action四primitive、record只保存前两个；区分元素真实存在、内容初始化、vector容量及buffer长度。本轮分析正常功能要求，不重做OOM/越界反例或所有别名writer。
5. 若不能证明目标正常初始化提供至少32项，指出具体未解参数/初始化callee/修改来源；列现有接口能否满足该功能条件。候选自有slot5或初始化变化保持DESIGN_ONLY，不能自动改“Base由原生ctor/Init写入”的契约。

交付：`template producer → template数量/内容 → slot5 → device n → Init → record消费者`链与条件表。C799F4只作为辅助表数量引用。运行期实值和静态初始化规律分别判断。

### R03：补齐六类gameplay consumer到自有查询的逐项链

1. 按§3.3六行逐一核对owner callsite、callee、Player this、item slot参数及action组；写出source RVA和实际IDB VA。可以共享相同helper证据，但每个consumer都要有到该helper的实际调用点。
2. 从各consumer的输入判定追到Player的controller key来源、device解析/helper/Manager槽、最终device+0x30/+0x34/+0x38或直接Base字段访问；记录action参数由谁生成、返回AL/ST(0)怎样驱动该consumer。
3. 核对“key→设备”路径到底使用A20390、A20FB0还是其他helper；按raw寄存器与参数追踪，不把相同vtable offset的其他类当InputDevice。A5F230/A5F370无action栈参的wrapper不作device caller。
4. 记录self query读私有快照能否覆盖每类consumer，哪些还消费C0/C4、raw轴/mask、binding或缓存pair。止于输入值进入正常动作判定，不展开完整实体更新、弹丸生成或世界副作用。
5. 对每条链保留controller key正常值、-1及找不到device时的相关分支，供R04引用。相同helper的负key语义只复读一次，各行注明适用性，不能以Manager全局枚举语义代替Player实际路径。

交付：六行完整`action/group → owner callsite/callee/ABI → key reader → device解析 → query/返回消费 → private snapshot字段`表；必要helper的原始字节与receiver；具体未展开边不能只写一个consumer地址集合。

### R04：正常解绑、移除与B中性输入的实际语义

1. 复用并补读7A6450正常ABI及+0x1618写点，明确SetControllerId(B,-1,1)仅直接改变哪些字段、正常传播到哪些对象；不把AL当成功bool，不展开全部无关caller。
2. 复读A20FB0的key==-1全active-vector枚举与key!=-1查找分支。将其与R03的实际Player消费链连接，分析B key=-1是否到达全设备路径，是否先被Player/helper条件截断，是否可能读取A的物理设备。
3. 手工列正常三种输入解析状态：virtual key有效且device仍在active vector；保留virtual key但device已移除；setter把B key改为-1。逐类或按共享helper给实际返回/后续功能条件，不从“key解绑”直接推“B中性”。
4. 追A20EB0正常remove、Manager+0x7C/+0x80、A213B0辅助表清理和已定位listener。上轮读取913760见其对关联对象+0x378处pair作清理；该对象类别、缓存pair与Player输入的关系要沿注册context追踪，不猜它就是Player或selector。未定位动态target保持UNKNOWN。
5. 追PlayerManager selector的cancel/完成状态及设备移除后的保留字段；确定正常停用需要关闭哪些Join输出、取消哪些selector、处理哪些Player key/缓存。无直接Player key写点只能支持“所读直接路径未写”，不能排除未解listener的间接作用。
6. 以“保留B但中性停用”作为首个正常功能问题，不强迫定位/调用物理删除B。若setter(-1)不能保证中性，列已有API支持的正常停用候选及其仍需明确的条件；不得编造magic sentinel、未理解的字段写入或把缺失设备与已解绑等同。

交付：正常状态转换与输入解析表、三种key/device状态的静态路径、已知callback/缓存清理、selector取消和最小停用候选。B退出入口可仍未定位，但必须说明不依赖它的正常停用路径明确到何处。

### R05：Join事件、query只读与poll阶段的接口边界

1. 复用W09/v6正常Join19→selector→confirm14→9B9CD0/7A6450、cancel15、导航20/21，及slot21 native C4←C0。只补与snapshot/phase/正常关闭有关的consumer顺序；不重做完整Join CFG和Player Init。
2. 区分当前InputState四个Q15轴/四个gameplay edge与Join命令。现有adapter对action>=12透传只说明当前源码能力，不能把尚未实现的自有vtable说成已复用该adapter。
3. 按相关静态caller及现有boundary/input/loop runtime源码定位publication、scheduler提交、native manager update/poll、Join pass之间已能确定的正常顺序。需要额外边界/Hook的方案列为候选，不访问游戏或安装Hook，不重开T0-03/04整体验证。
4. 手工追正常select+confirm、同一pass多次query、同一Authority快照跨两个Join pass、cancel和关闭Join输出的状态轨迹。区分读取次数与pass次数，说明何时同一只读edge应保持真、何时不能再次spawn。不得执行候选模型或脚本模拟。
5. 给出一个具体DESIGN_ONLY接口候选：Join命令字段/有效阶段、private current/previous或独立Join状态、poll锁存/推进规则、query只读返回规则、关闭/取消/完成动作、gameplay恢复顺序。没有现成native phase可绑定时指出具体接入缺口，不用墙钟或渲染帧冒充Authority tick。
6. 比较是否保留六槽、是否需要扩写flush slot21或其他正常功能槽。任何建议都须由消费者依据支持；不预设slot7必须覆写。候选与contract v2原文有差异时明确列出，不能自动采纳或修改contract。

交付：原生阶段事实表、一个具体最小Join/快照接口候选和手工正常轨迹、契约差异及精确未决。接口设计候选的可理解性与原生静态确认完成度分别报告，不要求二进制中存在自有命令协议。

### R06：整合勘误、准确引用与真实状态

1. 建立Q1–Q6修订表，附旧位置、复读字节、纠正/保留、影响路径及新定位。v6主报告、证据和其审阅失败不覆盖、不重跑；v1–v5也保留。
2. 使用R01–R05更新原六条P01–P06功能路径摘要；已有已确认部分按来源引用，新的实际链放主报告/证据，范围外安全问题不纳入完成度。
3. 主报告从design-revision链接root out用六级父目录，README用五级，out/cs-t0证据回到docs用../../docs。人工结合文件位置和文件元数据核对引用，避免只凭文件名判链接成立；不运行链接验证脚本。
4. 术语分列：Base vptr、device key计数器、模板vector、对象binding vector、update辅助表、Join selector、Player key/ordinal、private snapshot。原生float/Q15方向、四轴/八方向、AL/ST(0)、ret4/ret8以及VA/RVA统一。
5. 逐项分类剩余问题：还能补读的静态callee/参数、只可观察的当前运行值、需要决定的自有接口、生产未实现。运行值UNKNOWN不自动否定已成立的条件静态结论；缺口记录准确也不自动让对应功能完整。

交付：相互一致的v7主报告、证据、目录状态和实际执行回报；六类consumer逐项可定位，正常解绑的结论只覆盖已分析路径，设计候选不冒充采纳/实现。

## 5. 文件、证据格式与执行交接

取得实际分析内容后新增：

- 同目录`virtual-device-owned-abi-static-confirmation-v7.md`，标题注明正常功能路径修订，包含Q表、R结果、六条路径摘要、实现接口候选与剩余工作。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md`，必要时新增v7 consumer/binding/停用主题分册并链接真实文件。
- 更新[07-virtual-device README](../README.md)当前入口和真实状态，保留本文、v6及历史失败。

允许编辑仅限新v7文档/分册与README。设计v6及以前的工作设计、确认v1–v6、旧证据、contract、源码、测试、decision、权限不编辑。现有工作区无关修改/删除不覆盖、恢复或整理。

证据记录模板：`Q/R/P编号 → 本轮直接读/继承/手工推导/源码/候选 → input/IDB与imagebase → IDB VA/RVA/原始字节 → receiver/栈参/字段来源 → 正常条件与返回消费 → 功能含义 → 覆盖边界/剩余问题`。

执行者先读`C:\Users\114514\.codex\skills\isaac-cs-refactor\SKILL.md`，通过当前ALL_TOOLS发现IDA MCP并读真实schema。旧input/IDB为`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe[.i64]`，imagebase0x400000，执行时记录实际值，不认证哈希。允许只读decompile/disasm/xrefs/bytes/整数/元数据与纯读IDA Python；禁止patch/rename/set_type/define_func/保存IDB。

检索先用rg或rg --files，并限制相关文件/函数/精确地址，独立读取可批量进行。资料分析、反编译与手工推导不是过程测试；不得写/运行通过失败脚本、候选模拟或旧validator。原始指令优先于伪代码变量类型和历史注释。动态callback目标、当前player count/roster/vptr/thread等没有观察就不填实值。

主智能体设计交付后启动一个`model=gpt-6-luna`、`reasoning_effort=max`、新上下文子智能体执行tx-01-09，不再创建其他子智能体。主智能体交出IDA只读使用权，不并发读取IDA或编辑执行者新报告；此前只有只读观察，没有游戏附加、断点、patch或IDB保存变更。工具不可用时如实记录，继续已有资料，不伪造新直接证据；指定模型失败不默换模型。

## 6. tx-01-09 唯一一次整体静态文档审阅

全部分析与必要编辑结束后，人工一次阅读v7主报告、证据/实际分册、本文、引用的关键事实/契约条款及README。不运行程序、游戏、构建、测试、verifier或链接/校验脚本。

| 检查 | 必须判断的内容 |
| --- | --- |
| Q1/Q2事实纠正 | Manager通知receiver与Base vptr/key来源在摘要、证据、表和导航完全一致 |
| binding来源 | 模板/对象n/辅助count分开；实际初始化callee、参数和正常条件有依据 |
| consumer覆盖 | 六个action/group逐项有caller/callee、ABI、key解析、device query/返回消费及边界 |
| 正常停用 | setter(-1)、device移除、Player实际消费及selector/cache清理分别证明；没有将全设备查询泛化成B一定串线，也未把解绑直接判中性 |
| Join设计边界 | query只读、private/native previous、poll/pass/Authority阶段及设计候选区分准确；未自动修改契约 |
| 文档/状态 | 相对链接路径正确，Q/R/P和VA/RVA一致，旧失败保留，报告/证据/README/回报结果一致 |
| 范围 | 只完成正常功能资料与候选，不恢复安全批次、gate/哈希认证或生产完成声明 |

不要预填PASS。审阅后必须回填真实结果到状态行、审阅节、证据、README与最终回报；事实回填不是修复或再次验收。若失败记录具体问题，不自动修补或重跑。分析和编辑尚未完成时可正常补充/修正文档，不能把已经发现的旧错误故意留到最终审阅；不得通过提前判PASS/FAIL或运行程序增加验收次数。

分别回答：本轮事实修订是否准确、原六条正常功能链明确程度、仍需实现/采纳哪些接口、还缺哪些具体静态依据与运行值。文档准确交付与功能完整性采用不同结论；运行值未知、设计待采纳或生产未实现不替代对静态证据的实际判断。

## 7. tx-01-08 设计交付记录

本次新增本文与目录入口，用上一轮只读核查及已存在资料提供定位种子，未新增tx-01-09二进制证据；不修改旧报告、源码、contract或测试，不运行程序、构建、测试或实机操作。全部编辑完成后一次人工整体静态文档审阅，结果在本节回填。

设计整体静态文档审阅：**通过（设计交付范围）**。全部编辑结束后人工一次通读本文与README当前入口，核对Q1–Q6、原始字节种子、六类consumer的RVA/VA对应、正常功能范围、既有证据引用、历史失败保留、现行哈希约定、指定执行者及v7交付责任，确认相互一致。此结果只说明执行设计交付准确，不表示v7分析、接口采纳、生产实现或实机功能已完成。未运行程序、测试、构建或校验脚本，未进行第二次审阅。
