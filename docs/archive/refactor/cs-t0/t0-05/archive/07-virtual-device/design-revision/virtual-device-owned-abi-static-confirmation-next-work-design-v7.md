> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v7

日期：2026-10-12  
设计交付推进：`tx-01-11`  
静态执行推进：`tx-01-12`（S01–S06 合为一个完整目标）  
状态：`DESIGN_ONLY / v7 纠错与正常功能补充设计；确认报告 v8 待执行`  
执行者：用户指定 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本设计承接 `tx-01-10` 对[确认报告 v7](virtual-device-owned-abi-static-confirmation-v7.md)和[证据 v7](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md)的只读复核。执行者复读下面关键种子，再编写新报告；种子是上轮观察，不能标成执行者本轮直接证据。

## 1. 完整目标、范围与交付结论

纠正 Join 动作编号、scalar 多层返回通道及固定方向表覆盖的错误；补齐六类 gameplay consumer 的实际查询调用点、真实跨 family 查询和持续状态需求；定位正常移除的通知 context/缓存关系；确定正常主路径的 publication、poll、Join pass 和 gameplay 消费顺序，再给出有原生依据的 Join/停用接口候选。

交付为确认报告 v8、证据 v8、必要的新 v8 分册与本目录入口。分别回答：事实勘误是否成立；原六条 P01–P06 正常功能链哪些已闭合、哪些具体静态边仍缺；有哪些自有接口待决定；哪些运行值尚未观察；哪些生产代码尚未实现。**列出了缺口不等于闭合了功能，写了候选也不等于原生事实成立。** 不以文档准确性范围的通过代替正常功能完整性。

正常条件继续限定：分配成功、原生对象/缓冲有效、顺序操作、Offline/local-co-op roster 为空、virtual key 唯一、创建 B 前主 Player vector 只有 A。沿用[原生 Join 创建/发布规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的 `DESIGN_ONLY` 前提；不重做完整 Player Init/manager-this 研究。动态角色/物品分支按代码记录条件，不要求本轮证明全部游戏模式。

不展开 OOM、EH/unwind、锁/worker 并发、原子计数/镜像/溢出、ABA、全映像 alias、持久化与异常恢复。保留已采纳的自有 38 槽 device、六槽 control、S2、NEVER、构造前固定 DLL、恢复后常驻及每 session 至多一次创建；C/S 与原版/Isaac Sync 隔离。05-06 全部双 Endpoint scheduler 隔离与 exactly-once 总证明不在本轮，但本轮必须说明最小 Join/快照方案的正常时序。

`tx-01-11` 只交付本文和 README，编辑完成后一次人工整体静态文档审阅。`tx-01-12` 完成全部静态分析和 v8 编辑后一次人工整体静态文档审阅。S 编号只是内容索引，不拆成多次验收；讨论、分析和修改过程中零测试。

## 2. v7 勘误与可复用事实

### 2.1 修订清单

| 编号 | v7 位置 | 上轮复核结果与影响 | 执行内容 |
| --- | --- | --- | --- |
| E01 | 证据第165行；同节第170行与其矛盾 | `9BB2FE` 写入 `0x15`，即十进制 action21，属于导航；取消 action15 位于 `9BB3C2`，立即数 `0x0F`。原始字节正确但解释错误 | S01 |
| E02 | 证据第101行；报告第68行的层次表达 | device `+0x38` 在 ST(0) 返回 scalar；`A20AB0` 消费 ST(0)、将 scalar 写到输出指针，在 AL 返回非零判定；`9562D0` 在 XMM0 返回 scalar。不能把三个 ABI 合成一个 | S01 |
| E03 | 报告第68、72、120行；证据第109、113行 | `BAB240` 固定 dword 为 0,1,2,3；`BAB370` 为 4,6,5,7。对应循环索引不会自动使 action 数值变为动态未知；这些查询分别属于 Movement/Shooting family。真实 action11、Item/PillCard 9/10 交换和 Shooting 的 0–3 仍单列 | S02–S03 |
| E04 | 证据第20行及 R01.1 unlock 行的 receiver 精度 | `A20E4C` 从 `[Manager+0x14]` 读嵌入 Mutex vptr，`A20E4F` 令 ECX=Manager+0x14，`A20E52` 才调其 +0x10；随后通知才使用 Manager vptr。复读并明确区分 | S01 |
| E05 | 两份 consumer 表与审阅“逐项有链”结论 | 只有 owner callsite 和函数级路径，缺各 consumer 内到 query helper 的具体调用指令及 action/key 的准备；补齐可定位证据 | S02–S03 |
| E06 | R04–R05、剩余待办和 README 摘要 | 保留正 key/注册设备并中性只能是暂停输入候选；正常退休仍需设备移除及 B 解绑/退出。poll/pass/Authority 顺序、同快照跨 pass、接口与契约差异尚未明确 | S04–S06 |

`tx-01-09` 的原审阅记录为“通过（限定文档范围）”；`tx-01-10` 后续复核发现上述错误且未认定完整。两次事实分别保留，不把旧记录改成另一结果，不重跑旧验收。

### 2.2 原始证据种子

旧 IDB imagebase 为 `0x400000`。本表全为 IDB VA，RVA=VA−imagebase；执行时读取实际元数据。

| VA | 上轮直接读取的 bytes/行为 | 需要证明的层次 |
| --- | --- | --- |
| `9BB2FE` | `C7 45 DC 15 00 00 00` | action21 导航；不要把十六进制15读成十进制15 |
| `9BB3C2` | `C7 45 E8 0F 00 00 00` | action15 cancel；下一条 query call 位于 `9BB3C9` |
| `A20AEB` / `A20AF1` | `FF 55 F0` / `D9 5D 10` | 间接 device scalar query；`fstp` 消费 ST(0) |
| `A20B00` / `A20B3B` | `F3 0F 11 00` / `8A C3` | 将 float 写 `[EAX]` 输出指针；AL=bool，callback 正常 `ret` |
| `956394` / `95639F` | `F3 0F 10 45 08` / `C2 10 00` | 上层 helper 返回 XMM0，正常清理16字节参数 |
| `BAB240` | `00 00 00 00 01 00 00 00 02 00 00 00 03 00 00 00` | 四个固定 action0–3 |
| `BAB370` | `04 00 00 00 06 00 00 00 05 00 00 00 07 00 00 00` | 四个固定 action4,6,5,7，顺序有语义 |
| `77AE38` / `77AE58` | `movaps xmm0,[BAB240]` / `E8 83 B3 1D 00` | Movement 复制固定表，按 loop index 取 action，调用 `9561E0` |
| `77D432` / `77D458` | `movaps xmm0,[BAB370]` / `E8 83 8D 1D 00` | Shooting 固定表到 triggered query；复读循环上下界 |
| `A20E4C` / `A20E4F` / `A20E52` | `8B 47 14` / `8D 4F 14` / `FF 50 10` | Mutex receiver=Manager+0x14；不是 Manager 首 dword 的表 |
| `A20E55` / `A20E68` | `8B 07` / `8B CF` | 后续 notify receiver才是Manager |

### 2.3 已有有效依据

默认 generic binding vector 为 C799E8/EC/F0；标准 main 的 `4307C0` 成功分支构造32项，C799F4独立为辅助表count4。Base slot5→对象 n→Init 双缓冲与通用record32项的链可引用v7并注明条件；不要求重复逆向该完整初始化器。构造立即数Base vptr B82758、key counter C7362C与device+0x0C，以及Manager通知身份可复用。

普通 Join 直接枚举 Main active vector，slot7不是普通Join前置；Join19、confirm14、cancel15、导航20/21，selector的key经setter写Player+0x1618；setter不返回成功bool，不直接写ordinal。正key lookup miss不自动变成key=-1枚举；key=-1可走fallback list或全active设备。原生slot21/+0x54只做native C4←C0，不能推进私有previous。这些仍按本轮所需局部复读。

## 3. 必要资料与六类查询定位种子

先读现行规范、[架构规划](../../../../../isaac-cs-architecture-plan.md)、[T0任务书](../../../../isaac-cs-t0-task-plan.md)、[cs-t0入口](../../../../../../../../cs-t0/README.md)和 `C:\Users\114514\.codex\skills\isaac-cs-refactor\SKILL.md`。

具体输入：[报告v7](virtual-device-owned-abi-static-confirmation-v7.md)、[证据v7](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md)、[执行设计v6](virtual-device-owned-abi-static-confirmation-next-work-design-v6.md)、[契约v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[设计修订v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)、[T0-04 handoff](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md)、[确认v2/W09入口](virtual-device-owned-abi-static-confirmation-v2.md)、[W09草稿](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)、[证据v5](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md)。未受勘误影响的历史证据按准确位置继承，受影响内容先复读指令。

源码：[input_core.hpp](../../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp)、[active_apply.cpp](../../../../../../../../cs-t0/native/src/active_apply.cpp)、[active_runtime.cpp](../../../../../../../../cs-t0/native/src/active_runtime.cpp)、[loop_runtime.cpp](../../../../../../../../cs-t0/native/src/loop_runtime.cpp)、[boundary_runtime.cpp](../../../../../../../../cs-t0/native/src/boundary_runtime.cpp)。用rg定位实际定义，不猜不存在的 `*_probe_runtime.cpp` 文件名。

下面地址是上轮读取的限定调用点，不是所有间接/嵌套查询的完整集合。执行者须复读 action/key/this 准备与返回消费；原 handoff owner mapping 按RVA提供，本表按base换成VA。

| consumer | owner VA→target VA；ABI | 到 query helper 的已读 VA 种子 |
| --- | --- | --- |
| Movement | `785BC2→779B40`；thiscall Player* | `779C22/779D3B→779360`；`7793FF/779421/77943C/779458→9562D0`（四方向）；`7793D0→956110`（action11）；`77AE58/77AF71→9561E0`（固定表/action11） |
| Bomb | `785BC9→780F40`；thiscall Player* | `780FC1/7810AB→9561E0`（action8） |
| Item | `785BD2→791A40`；thiscall Player*,int slot，owner传0 | `791B2A→956110`、`791B54→9561E0`（9/条件10）；`791B84/791BAD/791BCB→956110`（11） |
| Pill/card | `785BD9→7A3D30`；thiscall Player* | `7A3DFF→9561E0`（10/条件9）；`7A3E23/7A3E5D/7A3E7F→956110`（11） |
| Shooting | `785BE0→77B020`；thiscall Player* | `77B1ED/77B220/77B248/77B280/77B2B6/77B2DE/77B306→956110`（0–3或4–7，按分支核实共享尾部）；`77B9F1/77D458→9561E0`（11/固定表）。scalar链如经嵌套helper则给实际点，不照抄泛化 |
| Drop | `785BE7→7A42B0`；thiscall Player* | `7A4309→9561E0`、`7A4584→956110`（11）；追held/timer实际使用条件 |

查询中心：`956110` pressed、`9561E0` triggered、`9562D0` scalar；Manager `A20FB0` slot29/+0x74→`A20390` slot10/+0x28；callback `A20940/A209E0/A20AB0`；device查询+0x30/+0x34/+0x38。区分传入helper的Player*参数、key栈参、helper内部从n298读取的容器this与最终device receiver。

## 4. 执行工作

### S01：动作编号、receiver与三层返回ABI纠错

1. 复读E01 raw与后继selector字段修改，将19/14/15/20/21统一用十进制action标签，原始bytes仍按hex记录；区分action21和vtable slot21。
2. 对scalar建立三层表：device slot签名/ECX/action/ret4/ST(0)；A20AB0的按值pair、action指针、float输出指针/cdecl/AL；9562D0的实际栈参数、ECX用途、XMM0/ret16。原始指令优先，不把Hex-Rays错误float形参或`__userpurge`展示当完整C++签名。
3. 正key、fallback list、early override三条scalar分支分别记输入输出；解释zero初始化、lookup miss、callback bool与实际scalar输出的关系。只在原生query返回层讨论，不展开世界动作。
4. 复读E04，把嵌入Mutex unlock与Manager notify分开；同步v8摘要、证据、表和返回寄存器术语。保留v7原文。

交付：E01/E02/E04勘误表、三层scalar ABI和bool对照表、足以复核的连续字节与数据流。

### S02：固定方向表、逐查询调用点和实际coverage

1. 直接读BAB240/BAB370四个dword，沿movaps、栈copy、loop index与循环终止追到triggered调用；判断该限定路径下是否有改写，记录顺序而非排序后集合。不能只因索引来自循环就写“动态action未知”。
2. 按§3六行复读owner ABI及每个输入判定调用点，记录VA/RVA/raw、action来源、Player+0x1618的key栈参、helper内部this、Manager/device receiver和返回消费；同helper负key证据可以共用，但每个consumer须有实际调用位置。
3. 建立查询清单而不止六行概括：主要方向、固定表triggered、action11 gate、Item/PillCard条件交换、Shooting alternate0–3及已读nested helper。止于输入进入consumer的正常判定，避免展开完整弹丸/实体逻辑。
4. 比较原生device self query的快照能力与现有SyntheticAdapter能力。固定方向表查询同family，修正v7误判；其他真正跨family查询继续记录。现有active runtime限定ordinal0/物理device，不代表B或自有vtable已接入。
5. 静态上可定位的常量、条件和helper不能统称运行值UNKNOWN；真正动态来源给具体reader/条件与分析终点，不启动全映像writer闭包。

交付：完整六类查询位置表、两个常量表及循环证据、snapshot覆盖/未覆盖对照；不宣称所有游戏模式已穷尽。

### S03：跨family查询与pressed/triggered/held正常语义

1. 针对S02中实际cross-family调用，复读触发条件与所查询动作：Movement/Shooting的11、Item/PillCard的9/10交换、Shooting alternate0–3。给条件表达式的字段/常量/原生helper位置，不只保留伪代码v4/v6名。
2. 对Item、Pill/Card、Drop的pressed/triggered查询追实际使用时序和hold/timer字段；判断单tick edge pulse能支持哪些普通动作、哪些需要独立held或release。当前four-axis/four-edge/held字段schema与自有device候选分别说明，不能自动修改payload。
3. 查active_apply ConsumerForAction/Evaluate与active_runtime Eligibility/unsafe处理，列复用现有adapter的实际限制。自有device查询是否绕过该family限制、仍有哪些早期override先于device path，是不同问题；给最小接口候选及未决，不把当前adapter的拒绝等同原生device不能提供该值。
4. 用手工表描述正常press、hold、release及再次press时+0x30/+0x34/+0x38应读到什么，依据来自原生consumer还是候选要求。只手工推导，禁止执行模型/脚本。

交付：实际mode条件/查询需求与最小snapshot语义候选，保留schema/契约需要决定的差异；不扩大到05-06完整exactly-once研究。

### S04：暂停输入与正常退休分开，追通知context和缓存

1. 对A20EB0显式remove、A212C0/A213B0通知和913760，沿906D60注册点追owner/容器构造、callback参数、object+0x378 pair来源与正常读者，确定是否关联Player/fallback list/selector。注册raw种子906E70 push ESI、906E71 push913760、906E76 ECX=C57B4C；准确记录实际call地址，不能把push地址写成调用点。
2. 比较暂停输入（B正key、device保留登记、self query中性）、移除设备（B旧正keymiss）、setter(-1)三种状态。共享helper前置override及fallback容器是否会改变结果按静态条件判断，不能只引用A20FB0就外推全部Player输入。
3. 重读selector cancel/confirm后的残留字段、设备正常移除的缓存影响。正常退休仍必须满足现行规范与契约v2：关闭publication/Join admission、设备离开vector、B解绑或退出、DLL常驻。仅暂停输入不满足退休完成。
4. 沿已有Player manager/05-03资料找B正常解绑/退出入口或正常run/menu退休路径；记录实际caller/ABI/影响字段和可达条件。以原生正常调用为依据，不编造未理解字段写入、magic key或物理删除策略；若仍不明，只给精确终点和残留功能问题。
5. 把poll返回策略纳入正常membership：继承证据v5 common在poll前复制C4←C0/清C0，AL=0会在Manager update里通知/erase/release。复读所需有限正常分支；说明保留登记的暂停状态需要什么返回、显式remove与poll自动移除如何协调。只讨论功能顺序，不重做锁/EH/并发证明。

交付：owner/缓存读写链、暂停/正常移除/解绑或退出状态表、poll与remove候选、契约差异。找不到完整退休入口就明确其未闭合，不能以保留设备方案替代。

### S05：publication、poll、Join pass和consumer的静态阶段关系

1. 以954CD0为正常帧路径，复用954F7D→A1FC00、954F84→95B310、state2 9551BD→6FA540→6FADA7→9BAC40，追同一限定正常分支内的standard-room更新/consumer位置。优先查当前loop/boundary原始RVA和调用关系，不重开T0-03/04全部证据。
2. 连接StandardRoomHook前后authority observation、active_runtime::Authority resolve/publish/BeginAuthority/EndAuthority与原生manager/common/poll/Join/Player动作。记录哪些调用有严格先后，哪些不同分支不能排序；调用树和source边界不等于当前运行次数观察。
3. 对原生主路径poll直接caller和已有相关helper做有限追踪，分析同一Authority发布epoch内0/1/多次poll对private previous/current的影响。若每次poll都推进previous会使边沿提前消失或旧快照重复出现，提出按publication epoch锁存的候选并注明与contract v2的精确差异；不自动采纳。
4. 明确Join命令target Authority tick在Join早于/晚于Authority作用域时怎么变成有效event。若现有边界不够，给一个具体原生入口/调用点、receiver、正常return位置及coordinator接入候选，不只写“以后加observer”；本轮不安装Hook。
5. 说明device+0x78 poll的AL返回与锁存是否变化是两个语义；不能因为无新epoch就返回0导致自动移除，也不能未分析就宣布RETIRING必须非零。

交付：有callsite依据的正常阶段顺序表、publication/poll计数条件、可定位的最小接入候选与精确静态未知项。worker/现场线程调度仍不展开。

### S06：Join轨迹、接口差异和完整报告整合

1. 基于S01/S04/S05更新一个具体DESIGN_ONLY协议：JoinCommand字段、验证/提交/锁存、pass begin/end、只读query、cancel/admission、暂停、退休、gameplay恢复。epoch来源、清除位置、ordinal1关联都明确；当前schema没有Join动作则明说。
2. 手工追六种有限正常轨迹：select与confirm同pass；同pass重复query；同Authority快照跨两个Join pass；同publication epoch多次poll；cancel后关闭；正常停用/设备移除。每表含进入状态、发布epoch、各query返回、selector/key变化、结束清除和下一pass结果。不能以两个独立tick示例代替其他轨迹，也不能以唯一device key推断唯一selector或只spawn一次；找不到依据时列具体条件。
3. 原生slot21只flush native C4，若候选需要对私有Join状态响应flush，列consumer依据、返回ABI和六槽数量差异；若不需要覆写，给pass完成清除为何足够的条件。候选保持只读query，当前contract不自动修改。
4. 新报告恢复原六条功能摘要：P01注册/枚举、P02类型/name/binding兼容、P03普通Offline Join、P04 key/B ordinal、P05快照消费、P06正常停用。每行列本轮证据、继承证据、条件静态完成程度、未解边/设计/运行值/未实现，不用六类consumer表替代六条系统功能链。
5. 建E01–E06修订表，保持v7与其原审阅历史；同步新证据、README和最终回报。旧文档不修改，当前摘要说明tx-01-10发现错误，不再把v7笼统当“准确且完整”。

交付：六种手工轨迹、候选与现行契约逐条差异、六条系统功能状态及真实剩余事项。仅运行值未知或生产未实现不自动否定成立的条件静态结论；反过来不能将缺静态数据流当作只缺代码。

## 5. 文件所有权、工具和交接

本轮执行者仅新增：

- 同目录 `virtual-device-owned-abi-static-confirmation-v8.md`。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md`；可新增带v8和主题名的consumer/phase/retirement分册并链接实际文件。
- 更新[07-virtual-device README](../README.md)当前入口和真实状态，保留本文与所有历史结果。

不编辑设计v7及以前、确认v1–v7、旧证据、project-conventions、contract、生产源码、测试、verifier、decision或权限；不整理无关工作区修改/删除。不计算/校验任何哈希，不以gate/解锁字段推进；历史文字可阅读，偶然出现不构成新的停止条件。

执行者通过当前ALL_TOOLS发现IDA MCP并阅读真实schema；记录实际input/IDB/imagebase。旧input/IDB为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe[.i64]`。仅允许静态decompile/disasm/xrefs/bytes/数据和纯读IDA Python；不patch/rename/set_type/define_func/保存IDB。不启动/检查游戏进程，不读进程内存，不附加调试器，不输入/注入/Hook/IPC或进行运行期修改。不构建，不运行游戏、test、模拟、harness、probe、smoke、canary、旧verifier或通过/失败校验脚本。

证据格式：`E/S/P编号 → 来源级别（本轮IDA直接/继承/源码/手工推导/候选/UNKNOWN）→ input/base → VA/RVA/raw → receiver/栈参/字段/返回通道 → 正常条件/返回消费 → 结论边界/未解点`。检索先rg，限制相关函数与文件；同helper共享证据但caller逐项定位。IDA Python用于静态读取，不执行候选模型；若工具不可用如实记录并继续资料阅读，不伪造直接证据。

主智能体设计交付后用新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体。执行者不再创建其他子智能体。主智能体交出IDA只读使用权，不并发操作IDA或编辑v8交付。上轮只有只读IDB分析，没有保存IDB或新增游戏/调试器操作；不推断其他会话运行状态。

## 6. tx-01-12 唯一人工整体静态文档审阅

全部分析和编辑完成后，人工一次通读v8主报告、证据/实际分册、本文、引用的关键契约/源码和README；不运行测试/验证程序或链接检查脚本。

| 检查项 | 最后一次整体审阅须判断 |
| --- | --- |
| 事实 | E01编号、E02三层ABI、E03固定方向表、E04 Mutex/Manager receiver在全部新文档一致；原始字节解释准确 |
| 输入消费 | 六类consumer内实际query调用点、action/key/return可定位；固定表与真正跨family、pressed/triggered/held需求分清 |
| 停用 | 暂停、device remove、B解绑/退出与退休完成分列；通知context/缓存、poll自动移除有有限依据或精确未决 |
| 阶段与候选 | publication/poll/Join/gameplay先后基于具体正常调用点；六种轨迹、候选接入点、契约差异明确，设计不伪称原生事实 |
| 引用 | design-revision到out/cs-t0用六级父目录；README用五级；out/cs-t0回docs/cs-t0用两级；人工结合文件位置和元数据核对，VA/RVA/槽/action一致 |
| 状态与范围 | E/S/P摘要齐全；静态缺口、运行值、候选和未实现分别说明；保留历史且新摘要真实；不重开安全范围、gate/哈希或生产完成声明 |

不要预填PASS。审阅后回填实际结果到主报告状态/审阅节、证据、README和最终回报；结果回填不当作修复或二次审阅。若失败记录问题，不自动修补或重跑。分析编辑期应正常修正已发现错误，不能故意等审阅才处理。文档任务通过只表示该文档范围准确；正常功能链若仍有未解静态边，最终回报必须直说未完整。

## 7. tx-01-11 设计交付记录

本次以tx-01-10只读复核和既有资料提供定位种子，新增本文与README当前设计入口；不新增tx-01-12二进制证据，不修改旧报告/证据、contract、源码或测试。全部编辑完成后一次人工整体静态文档审阅，实际结果在本节回填。

设计整体静态文档审阅：**通过（设计交付范围）**。全部编辑结束后人工一次通读实际保存的本文与README当前入口，核对E01–E06、raw/VA种子、六类query调用位置、S01–S06、六条P功能摘要要求、六种手工轨迹、暂停与退休的区别、契约差异、来源级别及文件所有权；结合文件位置与元数据核对必要引用，确认与现行规范及用户正常功能范围一致。此结果只说明执行设计交付准确，不表示v8分析、接口采纳、生产实现或实机功能已完成。未运行游戏、程序测试、构建或校验脚本，未进行第二次审阅。本段仅回填该次实际结果。
