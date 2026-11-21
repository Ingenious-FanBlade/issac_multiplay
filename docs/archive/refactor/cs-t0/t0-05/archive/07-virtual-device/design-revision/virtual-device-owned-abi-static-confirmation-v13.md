> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v13

tx：tx-01-27  
状态：四个工作包均已实质静态研究；完整正常功能目标仍未闭合。当前结果是部分完成，不把材料撰写或单个候选通过等同于目标满足。  
本轮只读 IDA 与已有源码/契约；未改生产 source、contract/schema、旧报告/设计或 IDB，未运行任何测试、构建、harness、游戏或哈希/链接检查。

## W01–W04 结果

| 工作包 | 实际结果 | 仍缺的闭合边 |
| --- | --- | --- |
| W01 Popup 身份与 UI→A | Popup ID、K、v91 局部候选索引、vector ordinal、Popup state/result 分开确认。954CD0 顺序与 Join/Room guard已直读；Popup busy确实能挡 state1/2 的 Join pass，但不能据此说整个 A/Room/Player 停止。reselect branch有条件地会对当轮 Player 清 K。 | 6FADC0 后所有 A Player consumer 的实际条件继承 v10 有限链，未在本轮展开每个 consumer target；Popup descriptor/listener的实际运行值未观察。 |
| W02 B 正常解绑/退出 | Main Player vector真实 create writer、全员 erase、全员 unbind、keyed reader与有限 caller均已读。也定位到95B87F对当前候选Player的key=-1 unbind候选。 | 没有闭合只作用B且保证A不变的normal local-co-op退休链；全员clear不是B-only。保留B时所有下一reader不读A/旧B publication且不产生B side effect也未证。未声称全映像没有退出路径。 |
| W03 A/B witness、每人 publication 与 S2 | 当前单A Source约束及generation清理后果已逐函数确认；给出带API输入/调用时序、slot/generation owner、每人current/previous、原生consumer许可的DESIGN_ONLY候选。 | 候选还要决定command→ordinal输入路由，以及Authority pre/post和contract v2的current/previous唯一writer冲突；没有生产代码。 |
| W04 poll与Join attempt | exact same-tick level-only去重条件、native current/previous滚动、BuildRawSample的正常edge writer已串通。9BB415 receiver/selected type、9BB41C selector K、9BB424绑定、6FADAC pass close已读；提出最窄attempt wrapper和成功终结点。 | 未观察真实设备poll轨迹或实际Join attempt。6FADAC只关闭pass，不能当B创建成功。attempt Hook、绑定后唯一B校验和发布仍属DESIGN_ONLY。 |

细节、VA/RVA、raw区间、参数、return/reader及source/contract引用见[证据 v13](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v13.md)。

## 适用前提和来源等级

正常前提为：分配成功、对象和缓冲有效、主线程顺序操作、Offline/local-co-op、Online roster为空、唯一正K、创建B前Main Player vector恰为A。Online roster为空、n298+0x4B3D8 context vector为空、Main Player vector人数/成员是三个独立事实，不互相推导。

继续继承38槽device、6个override、6槽control、S2、NEVER、base ctor前pin DLL、ctor后安装自有vptr、BASE_CONSTRUCTED后不析构或释放对象/control/有效基类状态、恢复后DLL常驻、每session至多一次自有设备创建且失败不重试。B Join attempt owner与设备创建owner分开。A ordinal0、B ordinal1且session内ClientId→ordinal冻结；A/B共用canonical/sequence/target tick/queue/dedupe/Resolve/apply，B无即时旁路。可复用通用InputManager；C/S不依赖Online Manager、NetInputDevice或FrameStore。

全文把结论标为IDA-DIRECT、SOURCE-DIRECT、STATIC-INHERITED、CONTRACT、USER-ASSUMPTION或DESIGN_ONLY。标注、限制与数据详见证据v13末节。

## W01：Popup身份与A后置

当前纠正如下：

| 值 | 对象及用途 | 核心结论 |
| --- | --- | --- |
| Popup首dword / sub_9C39E0 return | Popup ID | 9C01D0从递增global取ID写对象首dword；9C39E0插入后返回首dword。 |
| n298+0x4ABC8 | 初始warning Popup ID | 95B4C0接收9C39E0的return。 |
| n298+0x4ABCC | 断线/重选Popup ID | 95B811保存9C39E0的return，95BAC2/95BACC按Popup ID查表。 |
| device+0x0C / Player+0x1618 | device K / Player绑定K | 真实keyed call是95B6B3；95B6B6才读取返回pair。 |
| v91 | 当轮候选vector索引 | 仅在Main-vector候选分支可能暂时与vector顺序一致，不能当冻结ordinal。 |
| Popup+4、Popup+36、Popup+0x18 | state、busy flags、callback/result字段 | ctor对flags+36的初值随后被descriptor byte覆写；state由独立transition/update推进；callback结果字段与K/Popup ID独立。 |

UI实际次序为：954CD0先在954F7D poll，再在954F84跑95B310 selector/reselect，随后954F8F更新Popup list。case1/2的Join pass要同时满足Popup+36 bit2未置位和9505E0的Room/Special条件。case1/2的6FADC0 room branch另由n298+307504及n298+306108 bit0控制，未复用该Join guard；因此“有Popup”不等于A/整个Room都暂停。state1的9897D0另有menu-state==17或Popup非busy及9505E0条件。该结论独立于W02 B vector erase。

A绑定影响上可确认：当断线/重选分支选择Main Player候选，95B87F以Player、K=-1、propagate=1调用SetControllerId。它会影响所选Player的key/关联，不删除Player；若当轮唯一候选是A，可能直接清A的K。后续正常输入走向取决于下次selector与所有Player readers，不能由Popup close本身推导。

## W02：B retirement 和 A 保持

Main vector为n60+0x1BAA8/begin与+0x1BAAC/end。9B9CD0容量检查后以当前count写Player+0x161C，追加Player，再做vtable初始化。9B9150是真erase入口，但遍历并销毁每个Player、从其相关列表移除、压缩vector直至empty。它由cleanup/state-transition等whole-group路径调用，不做B predicate。908070把Main vector全员SetControllerId(-1,0)，保留所有Player；9BC000只重置Player，不改begin/end。90AC70 keyed查device并参考related Player；417870越界时可能回到vector首项。以上各自说明setter、device lookup、list removal不能相互代替。

95B87F是一个更窄的真实解绑候选：context vector空时按当前Main-vector候选下标拿Player，再以K=-1/propagate=1解绑。它可能在具体selector/reselect场景选中B，但v91是临时候选索引，不是已冻结B身份；脱离该分支不能称B retirement。即便绑定清除成功，Main vector仍保留B。90AC70覆盖一个keyed reader，不能覆盖所有consumer及fallback；417870暴露A-first越界风险。没有证明之后B不读A、旧B publication或继续产生输入副作用，也没有证明A的绑定/ordinal稳定。

有限查读到了9B9CD0、9B9150、908070、9BC000、90AC70、417870、9BC790与相关erase/reset caller；没有找到可直接用作B-only正常退休的完整caller→解绑/erase→context/Manager cleanup→下一reader链。Online/reconciliation或whole-vector teardown不是本轮Offline B-only后置。仍缺的是具体selective B解绑/退出调用、n298+0x4B3D8 context/listener与InputManager association的清理次序，以及退休后全部正常reader的B/A选择证明；不是“全映像无退出”。

## W03：从单A到A/B的实现候选

SOURCE-DIRECT显示当前Source只有一个g_current_player/device/key、一个g_authority_command和一个SyntheticAdapter snapshot/previous。ReadUniquePlayer、CurrentOwnerStable、bootstrap要求vector恰有一个Player且ordinal0；Eligibility再次要求ordinal0/current owner。ActivePlayerUpdateHook把成功身份变化视为OwnerSwitch并调用AdvanceOwnerGeneration；该函数清dispatcher/queue generation、adapter、authority command、projection history及witness TLS。若把人数改成2但保留该语义，普通A→B traversal会清掉共享输入。active_witness的EnterPlayerScope本身允许非UINT32_MAX ordinal，但上游永远不给B一个有效identity。当前active_apply每action强制唯一consumer family，并非STATIC-INHERITED native跨类别许可全集。

DESIGN_ONLY最小结构与时序如下：

1. Session开始冻结ClientId→ordinal，建立A=0、B=1两slot。Join前B只有reserved ordinal，不建立B Player/TLS/publication。
2. Join成功后以Main-vector membership、Player+0x1618的K和唯一ordinal1共同绑定B，递增B的slot/binding generation；不得只使用Popup v91、Player*或K作session identity。
3. 一个Authority tick对所有有效slot用共享Dispatcher/Queue按相同scheduler、sequence、target、dedupe、Resolve/apply规则处理。A输入来自已证physical projection边；B经其自有device/source边。没有route到ordinal的command不得复制A输入给B。
4. Authority-pre生成每ordinal immutable resolved/publication；各Player原生Update按frozen ordinal进入Player TLS，consumer hook原函数exact-once后读取自己的slot S2。A↔B只切换临时scope，不能推进shared owner generation或清queue/planner/pending。正常post统一commit/close；无输入须对有效slot显式neutral以释放held。poll/query不能二次Resolve。
5. B自有+0x78只处理已发布的B快照，不阻塞、不分配、不查硬件、不创建新command。越权slot、scope外、无Authority、退休、generation不匹配必须B-local中性，不能转到A。
6. S2按每slot current/previous计算current/edge/scalar；consumer/action许可按继承表Movement→0–3/11、Shooting→4–7/0–3/11、Bomb→8、Item→9/10/11、Pill/Card→10/9/11、Drop→11。重复query只读同一slot快照，不推进poll或Resolve。

session、apply、binding、slot、Authority tick分别维护职责。A↔B traversal仅更换scope；真实Player/K重绑或slot退休只失效对应binding/slot generation；pause/loading/lifecycle才清全局owner generation。现有实现没有这些per-slot owner state，也没有当前command到A/B ordinal的已证映射。

contract v2 §4（+0x78 poll是current/previous唯一writer；+0x30/+0x34/+0x38只读锁存值）与“Authority-pre发布、post commit/close、poll不Resolve”的候选在current/previous writer归属上仍需一次具体选型。要沿用contract，则必须为Authority-pre→poll→Room consumer给出不重复Resolve的stage/reserve/commit与时序证明；若Authority pre/post直接管理每ordinal current/previous，须明确修订contract v2槽语义。两者都未在本轮采纳/改文件。B的独立输入路由到participant ordinal，以及自己的poll与Player S2 query先后关系，是剩余具体设计决定。

候选paper轨迹已逐项列在证据E04：A→B→A、B→A→B、跨类别重复query、重复own poll、无输入Authority、first noRoom先poll submit后无Resolve、second ordinary miss TopUpdateExit清理、loading recovery、pause恢复、B Join前/成功后、B retirement后A下一update。它们仅说明候选owner责任，不冒充Native边或运行结果。

## W04：重复poll与Join attempt

native A1F280每次poll先滚动current/previous类字段、清空current缓冲，再调用device vtable+0x78；返回地址A1F30C正是SOURCE PhysicalExit的poll return RVA 0x61F30C。954CD0在同帧先跑A1FC00。BuildRawSample每K/source记住上次有效状态；连续相同level生成edge_flags=0。因此正常重复的相同level样本不会不断重复同一edge。

SubmitPhysical只在edge_flags==0、completed tick相同、source相同、persistent state相同时且在分配sequence前抑制。edge或状态变化走新sequence；跨completed tick不命中。故API接纳带edge样本不代表稳态poll可重生edge：source history会把上次相同状态记为current；正常第二edge需要中间release/repress或其他状态转换。现场状态没有观察。

Join窗口已静态具体化：6FA540在6FADA7以Main Player vector调用9BAC40。其selector路径受mode/action14/pending-selector条件控制，assignment为-1时从候选数组取selected Player type；Main Player vector是9B9CD0 receiver，selected type为参数。9BB41C读selector K，9BB424以新Player绑定K，9BB429为绑定return。create与setter间没有null guard。故actual attempt应只在已过Join guard且即将调用9BB415时开始；wrapper exact-once后在9BB429检查membership/ordinal1/Player K，再终结成功。6FADAC只代表整个Join pass正常返回，无法区分无创建与成功B。具体wrapper及独立attempt owner仍是DESIGN_ONLY。

## P01–P06 分类与剩余状态

| 编号 | 类别 | 本轮状态 |
| --- | --- | --- |
| P01 | Native静态已证 | Popup递增ID、ID lookup、K keyed call及reader、Main Player create/whole erase/all unbind、normal poll current/previous、Join create-bind顺序均有IDA-DIRECT对象/VA证据。 |
| P02 | Native真正缺边 | B-only正常解绑/退出caller、vector membership与A不变的后置、B退休后完整next-reader/fallback证明、context/listener/Manager顺序尚未闭合。仅指本轮有限candidate边，不声称整个映像不存在。 |
| P03 | 候选设计与既有契约差异 | A/B per-slot identity/publication/S2、command ordinal routing、Authority-pre/post vs contract poll唯一writer、Join-attempt wrapper、B retirement sequence均为DESIGN_ONLY，未改contract/schema。 |
| P04 | 现场未观察 | 实际K、Player*、ordinal、vector/count、Popup descriptor/flags/listener、实际poll边沿、Join次数及成功发布、B retirement后的reader都未观察。 |
| P05 | 生产实现未完成 | 自有ABI正常create/register、per-ordinal A/B witness/S2、Join attempt owner与B-only retirement没有本轮生产source实现；静态缺边和未实现分列。 |
| P06 | 后续整体运行验收 | 整个实现目标完成后再以一次整体验收观察A/B输入路由、Join成功/失败关闭、重复query/poll、pause/loading/noRoom恢复、B退休后A继续、Hook恢复与DLL常驻。本轮没有执行或预设任何验收结果。 |

本轮依赖顺序如下：layout/base ctor/vptr/control和已证的device注册基本路径可按现行contract先做，不受B退出入口阻塞；统一ParticipantMap、S2 routing要等ClientId→ordinal输入字段和contract poll writer决定；Join attempt instrumentation可独立按9BB415/9BB429候选做源码设计，但不能声称创建可靠性；B-only retirement需要先补selective native入口或明确新增本地解绑行为及全部下一reader；随后才能证明A不被影响。不能因为基础实现可开始就写成完整目标已证。

本轮没有将Source/contract/旧报告修改来配合候选；W02和W03未闭合点保持开放。旧v12唯一审阅失败、tx-01-25复核和此前历史如topic README所载原样保留。

## tx-01-27 唯一整体人工静态文档审阅

唯一整体人工静态文档审阅：通过。要求、证据引用、source/contract 与 README 表述一致；W02 的 B-only 正常退休及 W03 的正式路由/契约接入仍未闭合，文档没有将它们表述为已完成。审阅后未自动修补或复审。

