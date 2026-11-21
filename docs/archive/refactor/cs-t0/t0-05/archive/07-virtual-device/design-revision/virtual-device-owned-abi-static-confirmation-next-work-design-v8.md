> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v8

日期：2026-10-12  
设计交付推进：`tx-01-14`  
静态执行推进：`tx-01-15`（W01–W06 为同一完整目标的内容编号）  
状态：`DESIGN_ONLY / v8 事实修订与正常功能补充；确认报告 v9 待执行`  
执行者：用户指定 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本设计承接 `tx-01-13` 对[报告 v8](virtual-device-owned-abi-static-confirmation-v8.md)和[证据 v8](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)的只读复核；继续限定正常功能，复用有效证据。下列新定位来自上轮主智能体读取，执行者须复读关键点，标明本轮直接与继承来源。

## 1. 完整目标与判定边界

修正v8将不同对象的相同偏移归成一条缓存链的错误，确认通知context、查询fallback list和Player匹配reader的实际关系；从既有C/S Hook锚点追通正常poll、Join、Authority发布与room更新的相对顺序；据此细化私有快照锁存、held/edge与Join事件方案，并补齐B正常退休的静态调用与状态链。

交付新确认报告v9、证据v9和README当前入口。正常功能静态完整性以对象身份、有限调用数据流、条件和时序覆盖判断；**生产未实现、现场值未观察、设计待采纳各自列出，不能替代静态缺口判断。** 如仍有静态未解边，具体列出，不把“已记录UNKNOWN”当功能闭合。

正常条件：分配成功、对象/缓冲有效、顺序操作、Offline/local-co-op online roster为空、virtual key唯一、创建B前主Player vector恰有A。继承[原生Join创建/发布规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)的DESIGN_ONLY前提，不重做全部Player Init或manager-this研究。**空roster不能直接推导另一个fallback/context容器为空**；必须核对各字段的具体来源。

OOM、EH/unwind、锁/worker并发、计数原子/镜像/溢出、ABA、全映像alias、持久化与异常恢复不展开。已采纳的自有38槽device、六槽control、S2、NEVER、原生base ctor前固定DLL、构造后不释放、恢复后DLL常驻、每session至多一次创建继续适用。C/S不得依赖或复用Online Manager、NetInputDevice、FrameStore；研究它们的原生reader用于解释条件，不授权把它们纳入C/S正向实现。

`tx-01-14`交付本文与README后一次人工整体静态文档审阅。`tx-01-15`完成全部分析、必要编辑与v9交付后一次人工整体静态文档审阅；过程零测试，W编号不拆成多次验收。05-06完整双端隔离/网络exactly-once证明不重开，本轮只说明最小正常Join和快照方案所需时序。

## 2. v8 修订清单与可靠进展

| 项 | v8位置 | tx-01-13复核所得边界 | 本轮工作 |
| --- | --- | --- | --- |
| Q01 | 报告审阅第147行；证据第32、35、112行 | tx-01-12实际只直接复读21/14/15，19/20却标成有限bytes复读或IDA-DIRECT；来源错标且原审阅已失败 | W01 |
| Q02 | 报告P06第95行；证据E04第73行 | A22190是Gamepad Init，this+0x378复制0x178硬件描述块；913760中的列表元素+0x378是关联pair。这些字段不能因偏移相同而认作同一对象/构造链 | W02 |
| Q03 | 报告P06第95–97行；证据E04第74行 | Item/PillCard向90AC70传入的ECX也是n298+0x4B3D8，与906D60注册context对应；90AC70主要匹配列表元素自身+0x0C key与Player+0x1618，而+0x378 source key还出现在诊断路径。必须分开主匹配和日志reader | W02–W03 |
| Q04 | 报告阶段第101–103行；证据E01第23行 | 既有loop_runtime锚点加IDA能在相关分支均到达时串起poll→Join→StandardRoom前Authority发布→room更新；v8未完成该静态串联便整体列UNKNOWN | W04 |
| Q05 | P05、DESIGN_ONLY与六轨迹 | held/edge和多次poll已有需求，但缺press/hold/release/repress明确返回表；晚于poll的publication怎样供当前目标tick消费尚未决定 | W05 |
| Q06 | P06正常retirement轨迹 | 暂停query中性、native remove、setter(-1)与B真正解绑/退出仍不等价；完整正常退休调用/selector/context影响未闭合 | W03、W06 |

保留tx-01-12原启动异常、替代实例接续、唯一审阅失败及没有自动修复/重审的事实。tx-01-13后续复核新增技术问题，不覆盖旧原文，不重跑旧验收。

有效依据继续引用：device scalar ST(0)→A20AB0输出指针/AL→9562D0 XMM0三层通道；BAB240=0/1/2/3、BAB370=4/6/5/7；嵌入Mutex receiver=Manager+0x14，notify才Manager；固定32项binding与C0/C4 Init；poll=0自动通知并移除active pair；普通Join直接枚举Main active vector、slot7非Join前置；真实cross-family/action11 held需求和key=-1风险。不要求重读所有38/6槽或重做全部初始化器。

## 3. 输入资料和定位种子

必读[项目规范](../../../../../project-conventions.md)、[架构规划](../../../../../isaac-cs-architecture-plan.md)、[T0任务书](../../../../isaac-cs-t0-task-plan.md)、[cs-t0入口](../../../../../../../../cs-t0/README.md)以及 `C:\Users\114514\.codex\skills\isaac-cs-refactor\SKILL.md`。

任务资料：[报告v8](virtual-device-owned-abi-static-confirmation-v8.md)、[证据v8](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)、[设计v7](virtual-device-owned-abi-static-confirmation-next-work-design-v7.md)、[契约v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[设计修订v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)、[报告v7](virtual-device-owned-abi-static-confirmation-v7.md)、[报告v2](virtual-device-owned-abi-static-confirmation-v2.md)、[W09草稿](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)、[证据v5](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md)、[T0-04 handoff](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md)。按必要段落读旧资料，受后续勘误影响的结论先核对指令。

源码：[input_core.hpp](../../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp)、[active_apply.cpp](../../../../../../../../cs-t0/native/src/active_apply.cpp)、[active_runtime.cpp](../../../../../../../../cs-t0/native/src/active_runtime.cpp)、[loop_runtime.cpp](../../../../../../../../cs-t0/native/src/loop_runtime.cpp)、[boundary_runtime.cpp](../../../../../../../../cs-t0/native/src/boundary_runtime.cpp)。

所有以下地址均为旧IDB VA，旧imagebase=0x400000；执行时记录实际base并给RVA。它们是上轮种子，不预填本轮IDA-DIRECT。

### 3.1 对象和容器

| VA/函数 | 上轮直接读取的行为 | 执行者须保留的区别 |
| --- | --- | --- |
| A22190；caller A21E7F in A21D40 | 查C57B70等Gamepad descriptor，qmemcpy(this+888,descriptor,0x178)，记录VendorId/ProductId/Family，最后A1E210 | Gamepad this+0x378硬件描述块；不能当作Net列表pair的writer |
| 906E70/906E71/906E76/906E7B | `56` / `68 60 37 91 00` / `B9 4C 7B C5 00` / `E8 90 67 11 00` | push owner、push913760、ECX=C57B4C、call A1D610；call是906E7B |
| 952DB6/952DBC/952DC2 | 恢复此前保存的n298；`8D 89 D8 B3 04 00`；`E8 99 3F FB FF` | 906D60 this/context=n298+0x4B3D8；复读保存值来源，不凭变量名认定身份 |
| 791AD3/791AD8/791AF3 | EAX=[n298]；`lea ecx,[eax+4B3D8h]`；push Player后call90AC70 | Item使用同一偏移容器；需追helper实际this与Player栈参 |
| 7A3DA4/7A3DC3/7A3DC9 | ECX=[n298]；`lea ecx,[ecx+4B3D8h]`；call90AC70 | PillCard对应关系；helper receiver并非Player |
| 90ACEA/90ACF5/90AD17 | `mov ebx,[entry+0Ch]`；`cmp [Player+1618h],ebx`；再比较请求Player key | 主要匹配entry自己的key；诊断中source pair key为另一含义 |
| 90AEB0/90AF30/913760 | FirstLocalDeviceID读entry+0x378 source pair；SetLocalDeviceID查Main设备pair并写关联；disconnect清关联或删除Net项 | 需明确容器元素类型、own key与source key，不能将所有reader都概括为同一匹配 |
| 95614E/956305等helper | 从n298+0x4B3D8/4B3DC读fallback begin/end | 将相同字段的读取链连到注册context；membership和动态值仍另列 |

### 3.2 条件正常阶段

| VA / bytes | 上轮直接读取/当前源码事实 | 条件与范围 |
| --- | --- | --- |
| 954F7D `E8 7E AC 0C 00` | callA1FC00 | 上游正常输入更新分支实际到达时才poll |
| 954F84 `E8 87 63 00 00` | call95B310 | manager更新后reselect |
| 9551BD `E8 7E 53 DA FF` | call6FA540 | state2、9C3990/9505E0相关条件允许；内部还需到Join分支 |
| 6FADA7 `E8 94 FE 2B 00` | call9BAC40，返回6FADAC | Join pass边界；不能外推全部其他caller |
| 9551C2/9551CF | 检查n298+0x4B130、+0x4ABBC bit0 | 前者可跳过后续，后者决定standard/其他分支；记录原始guard |
| 955231 `E8 8A 5B DA FF` | call6FADC0 | 同次正常state2后续standard路径 |
| 6FBA4D `E8 2E 6F 10 00` | 6FADC0内call802980，native Room更新 | 对应loop_runtime kCallRvas[15]=0x2FBA4D，kTargetRvas[15]=0x402980 |
| loop_runtime第728、1248–1265行 | g_standard_room目标802980；StandardRoomHook正常观察时pre callback→原函数→post callback | 是当前源码拟安装/已支持的wrapper语义，不声称本轮现场已安装 |
| active_runtime::Authority | pre resolve/publish/BeginAuthority；post EndAuthority(true) | 完整源码检查其tick/command/previous来源，不拼接成无条件运行事实 |
| 803E70 `E8 4B 48 C1 FF` | 802980内call4186C0 entity-list更新 | 从这里有限追到原Player owner六consumer；涉及虚分派须给目标关联证据 |

## 4. 工作内容

### W01：事实来源修订及receiver分层

复读本轮实际使用的Join19/20/21/14/15指令并给有限CFG；或者明确19/20继承而不带“本轮复读”。用本轮实际读取清单记录来源，避免主报告、bytes表和审阅回报矛盾。v8错标与失败原文不修改。

在已有六类consumer证据上补receiver层：consumer owner的Player this、helper栈上的Player/key/action、helper ECX或全局n298、Manager this、callback pair、device this逐层列出。例如77B1DC读取n298至ECX，77B1E5从Player+0x1618压key，77B1EB压action4，77B1ED调956110；不能将所有层写成this=Player。保留已确认三层scalar返回，原始指令优先于Hex-Rays错误形参类型。

交付：Q01来源勘误、实际读取清单、六consumer共享helper的receiver/参数/返回层表；不重做全部槽表。

### W02：以对象身份纠正+0x378并闭合context关系

1. 分别追A22190/Gamepad descriptor和913760/关联pair对象的实际ctor、vptr/RTTI或具名caller、对象大小及字段reader，形成“对象类别→字段→writer→reader”表。A22190从pair构造链撤出；不只改名，必须解释为何它属于另一对象。
2. 沿952410保存的n298、952DC2→906D60、906E7B→A1D610→913760传参，连接791AF3/7A3DC9及956110/9561E0/9562D0 fallback读取的n298+0x4B3D8。相同base与offset支持storage对应，容器元素、当前membership和运行期替换仍分别判断；不强求全程序alias闭包。
3. 分开90AC70主要匹配entry own key和诊断source key；90AEB0取source id、90AF30改source pair、913760清source pair/erase项。准确记录device+0x0C、pair首指针→device key、Player+0x1618、selector key的不同根对象。
4. 定位该列表元素的正常factory/插入/清理路径与local/online条件，确定Offline空roster是否足以使fallback list为空。若仍无依据，给具体writer/guard缺口，不再把已能证明的storage参数关系列UNKNOWN。
5. 解释研究中的原版Net对象对原生helper选择的影响，与C/S自有device Main active pair分开；不得建议C/S复用NetManager/NetInputDevice或在该列表内发布C/S设备。

交付：两个+0x378的准确布局来源、同context读写图、own/source key表及Offline条件；动态listener目标/现场membership保留真实未知。

### W03：正常移除、selector与B退休的具体效果

复用A20EB0/A212C0/A213B0、poll=0自动移除及SetControllerId(-1)事实，连接W02同context的实际影响。对自有Base→Net cast为null的正常通知分支，分析清哪些关联、保留哪些own key/Player key/selector；列表为空与非空分别手工推导，不将正常原版Net分支纳入C/S设计。

定位normal selector取消/confirm完成后的字段与重复pass行为，继承W09但补具名读写点。区分取消选择、暂停B输入、device missing、B解绑、B退出五种状态。B key=-1可能访问A/fallback的结论按实际helper guard说明，不能用中性self query覆盖其他设备返回。

沿7A6450有限直接callers与既有05-03/正常run-menu生命周期资料定位B退出/解绑可用入口。v8的908070→90819C对全Player setter(-1,0)只能作候选种子；必须说明其caller/条件、影响A与关联对象、之后是否仍有Player consumer。选择一个有依据的正常退休路径，列API/ABI/selector/context/key/membership变化；若只有whole-run teardown可确定，明确它的适用范围，不冒充保持A继续游戏的B独立退出。

交付：正常remove影响表和退休调用序列或精确未决；NEVER对象/control及DLL常驻始终保留，不凭裸字段写入或magic sentinel补缺口。

### W04：限定正常调用路径的完整阶段串联

1. 按§3.2核对source数组索引和native call bytes，追954CD0→6FA540/Join→6FADC0→802980。给能同时到达poll、Join与standard room的guard；暂停/其他分支按实际截断点列出，不把条件结论外推所有帧。
2. 将loop_runtime wrapper与active_runtime::Authority的scheduler resolve/publication/BeginAuthority/EndAuthority插入该条件链。给完整“native poll→Join pass→standard-room pre/Authority publication→room更新→post”的有限静态顺序；若关键边不能成立，指出与种子冲突的实际指令，而非直接泛称运行相位未知。
3. 从803E70→4186C0有限追到Player Update/六consumer owner，补动态目标的静态类型/表项关联。止于输入查询进入consumer，不展开全部实体或世界副作用；未解dispatch明确作为静态缺边。
4. 区分本轮条件调用顺序、现场是否走该分支/Hook是否安装、poll/Join每tick实际次数。前者可静态确认，后两者无本轮运行观察，不互相替代。

交付：带callsite/guard/来源的正常阶段表与可读调用图，并纠正v8将可追条件顺序整体列UNKNOWN的摘要。

### W05：晚发布、快照锁存与held/edge的具体接口候选

1. 从W04顺序分析现方案：当前room pre才发布，而同次native poll已发生；自有槽只在poll旋转current/previous会读哪一epoch、哪一target tick。核对active_runtime tick分配、dispatcher resolve/pending输入与contract AUTHORITY_MAIN_LIFECYCLE，不能把上次publication自动当本tick输入。
2. 比较并推荐一个最小DESIGN_ONLY接续方案：在有充分tick/正常路径依据的位置提前统一提交发布；或保持Authority pre统一resolve并增加私有锁存接入。分别说明具体callsite/API、effective tick、epoch、advance条件、poll的职责、query只读、首次Join和pause/未到room时的结果。后者如改变“poll唯一写锁存”须明确contract差异；前者不能以渲染帧/顶层loop次数冒充Authority tick。A/B仍走统一scheduler，不给B即时旁路。
3. 为普通press、连续hold、release、repress写明确表：action0–7幅值/pressed/triggered，action8–11 edge与需要held的查询各如何返回；Join14/15/19/20/21独立event。当前held_flags具体允许位与编码路径要读源码，不因字段存在就认定支持Drop held。列最小schema/私有状态/contract变化候选，不编辑它们。
4. 同一publication多次poll、无新publication、相同gameplay snapshot多次consumer query分别手工追current/previous/epoch。epoch幂等并不自行防止跨Authority tick的旧trigger重播；给事件有效阶段/关闭点和中性条件。只研究最小方案需要的语义，不重开05-06完整网络exactly-once。
5. 保留自有query+0x30/+0x34/+0x38按action提供值与现有SyntheticAdapter family gate的区别；核对956110/9561E0/9562D0早期override是否可跳过device查询，给正常条件与接口边界。

交付：一个具体最小方案、press/hold/release/repress表、tick/epoch时间表与逐条contract/schema差异。推荐不等于用户采纳或生产实现。

### W06：Join、退休轨迹与v9整合

基于W03–W05修订六种手工轨迹：select+confirm同pass、同pass重复query、同gameplay publication跨两个Join pass、同epoch重复poll、cancel+close、正常退休。每条明确命令何时已可见、Join先于room pre时target tick怎样生效、query返回、selector/Player key变化、pass completion、下次pass/tick是否中性；不能只给相同mask“读true”而忽略duplicate selector或重复创建条件。

给JoinCommand/admission/begin/complete与gameplay恢复一个具体候选；原6FADA7/6FADAC只证明这条caller，补必要caller范围或把候选限制在它上面。保留slot21 native C4←C0与private previous区别；若pass完成清除不足，说明是否需要flush参与、具体消费者依据与新增槽/contract差异，禁止自动改契约。

新报告包括Q01–Q06、W01–W06和原P01–P06六条系统功能摘要。P01注册/枚举、P02type/name/binding、P03Offline Join、P04key/B ordinal、P05快照消费、P06正常停用逐项列：本轮/继承依据、条件静态已明确部分、仍缺的具体静态边、候选、现场值与生产未实现。现有v8纠正的ABI等保持有效，禁止为了“完整”改标签或以生产缺失冒充静态未知。

交付：报告v9、证据v9、必要v9分册、真实README和最终回报，保留旧失败/异常/后续勘误历史。

## 5. 工具、产物所有权和接续

仅允许新增：

- 同目录 `virtual-device-owned-abi-static-confirmation-v9.md`。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v9.md`；必要时新建v9对象/context/phase/retirement分册并链接真实文件。
- 更新[07-virtual-device README](../README.md)当前入口和实际结果，保留旧记录。

不编辑设计v8及以前、报告v1–v8、旧证据、project-conventions、contract、生产源码、测试、verifier、decision或权限；不整理无关workspace修改/删除。不新增gate，不计算/校验任何哈希，不以旧解锁字段推进；历史字段可阅读，不作为停止条件。

通过当前ALL_TOOLS发现IDA MCP并读真实schema，记录实际input/IDB/base。旧目标为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe[.i64]`，base0x400000。只允许静态decompile/disasm/xrefs/bytes/数据和纯读IDA Python；禁止patch/rename/set_type/define_func/save IDB。IDA读取不是候选运行；不得执行通过失败校验、候选模拟、测试、构建、游戏、harness/probe/smoke/canary/verifier或链接检查脚本。不检查游戏进程，不读进程内存，不附加调试器，不输入/注入/Hook/IPC或运行修改。

每条证据记录对象根身份、VA/RVA/raw、receiver/参数/返回、正常guard、writer/reader和功能结论，分别标IDA-DIRECT、SOURCE-DIRECT、继承、条件推导、CONTRACT、DESIGN_ONLY、UNKNOWN。本轮IDA-DIRECT只来自实际读到的结果；工具失败或输出截断不能标已复读。未解项给实际分析终点，不要求全映像证明。

主智能体交付设计后，用新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体，不再派其他子智能体。旧v8替代实例已完成；原pending_init实例已撤回v8/IDA所有权，不恢复其任务。主智能体交出IDA只读使用权，不并发读IDA或编辑v9。此前只有静态读取，无IDB保存/patch或新增游戏/调试器操作；不猜其他会话的现场状态。PowerShell资料读取可用login:false避开无关profile初始化错误，不改用户环境。

## 6. tx-01-15 唯一人工整体静态文档审阅

全部分析和必要编辑结束后，人工一次通读v9报告/证据/实际分册、本文、关键contract/source与README，不运行校验程序。

| 检查项 | 整体审阅必须判断 |
| --- | --- |
| 来源与ABI | Q01错标已在新文档纠正；实际读到的19/20与继承边界真实；receiver分层、scalar通道/action/slot清楚 |
| 对象/context | Gamepad descriptor与关联pair分列；same-offset不代替identity；90AC70主匹配/诊断、n298+4B3D8 storage及membership边界正确 |
| 阶段 | poll/Join/Authority/room条件顺序有callee/callsite/guard，虚分派边实际依据或精确未解；静态顺序和现场值分开 |
| 功能方案 | 晚publication接续、effective tick、锁存唯一owner、query只读、held/edge表、多poll/多pass及契约差异明确 |
| 退休 | 暂停/取消/remove/解绑/退出分列；选择的正常调用路径及影响A/B范围明确，未解边不冒充已闭合 |
| 状态与引用 | Q/W/P一致、旧记录保留、报告/证据/README/回报真实；链接按实际位置人工核对，范围不扩张 |

不要预填PASS；审阅后只回填真实结果到报告状态/审阅节、证据、README和回报。失败则记录未完成项，不自动修补或重跑。分析编辑期应正常纠正已发现错误，不故意保留到审阅。design-revision到root out用六级父目录、README用五级、out/cs-t0回docs/cs-t0用两级；人工结合文件位置与元数据核对，不跑链接验证脚本。

最终分别报告文档准确性、限定正常功能静态明确程度、候选待采纳、现场值和生产未实现；剩余静态缺口必须具名，不能因审阅范围通过就称功能完整。

## 7. tx-01-14 设计交付记录

本次使用tx-01-13及既有资料提供执行种子，新增本文与README入口；不新增tx-01-15二进制证据，不修改旧报告/证据、contract、源码或测试。全部编辑结束后一次人工整体静态文档审阅，结果在此回填。

设计整体静态文档审阅：**通过（设计交付范围）**。全部编辑后人工一次通读实际保存的本文与README入口，核对Q01–Q06、对象根身份、own/source key、context/phase原始种子、W01–W06、晚publication候选边界、held/edge表与退休要求、来源/状态区分及产物所有权；结合实际文件位置和元数据核对必要引用，确认与现行规范和用户正常功能范围一致。此结果只说明执行设计交付准确，不表示v9静态确认、接口采纳、生产实现或实机功能完成。未运行程序测试、构建、校验脚本或实机操作，未进行第二次审阅。本段仅回填此次实际结果。
