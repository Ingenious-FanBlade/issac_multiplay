> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v17

日期：2026-10-16  
设计交付推进：`tx-01-39`  
静态执行推进：`tx-01-40`（W01–W04 是同一目标的内容划分）  
预期交付：正常功能路径静态确认报告、证据 **v17**  
状态：`DESIGN_ONLY / 待执行`  
执行者：用户指定 **GPT-6-Luna，max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接[报告 v16](virtual-device-owned-abi-static-confirmation-v16.md)、[证据 v16](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v16.md)和会话中 `tx-01-38` 的只读复核。v16 的 `tx-01-36` 已完成且记录过一次整体人工文档审阅；本轮是有具体勘误与设计修正的新推进，不续跑或重判那次审阅。旧报告、原通过记录及中断历史全部保留。

## 1. 本轮完整目标和范围

交付一套可供后续实现使用的有限正常路径静态说明：修正 Join 跨 pass/selector 轨迹，正确划定 native remove 的引用责任和外部引用变化，明确 S2 查询返回规则与 Authority-post/TopUpdateExit 时序，再统一结论分类和索引。主要是三类收尾及文档同步，不重新展开基础 ABI 逆向。

正常范围：有效分配、对象与缓冲，主线程顺序操作，Offline/local-co-op，**Online roster 为空**，B 使用唯一正 K，创建 B 前 Main Player vector 仅有 A。Online roster、context、Main Player 和 InputManager pair vectors 分别判断，不互相推定为空或为同一集合。A ordinal0/B ordinal1、ClientId→ordinal session 内冻结；Player*/device*/control*/K/handle 只是当前 generation 的关联。

继续采用[原生 Join 可靠创建并发布 B 的用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，标 `USER-ASSUMPTION / DESIGN_ONLY`；不重新证明 full Player Init、global manager-this。它不代替控制命令→query→actual-call 的接线。

保持自有 device 38 槽、六覆写、control 六槽、S2、`NEVER`、base ctor 前固定 DLL、ctor 后自有 vptr。BASE_CONSTRUCTED 后对象/control/有效基类状态不析构不释放，DLL 常驻；每 session 至多一次创建设备，失败不自动重试。设备创建 owner 与 B actual-create attempt owner 分开。退休中性，移除 Manager pair 到 UNREGISTERED，B 解绑或退出，不把保留 REGISTERED 当退休。

A/B 同 canonical、sequence、target tick、queue、dedupe、Resolve/apply，无 A 或 B 即时旁路；C/S 与原版/Isaac Sync 隔离，不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。通用 InputManager 可用。

正常 pause/loading/noRoom、重复 poll/query/pass、selector 跨 pass、首次 Join、B 退休及 A 下一查询在范围。排除 OOM/EH、worker 并发、计数溢出/ABA、全映像 alias/全 observer 闭包、持久化、异常恢复、全部 Mod/Lua 行为、05-06 完整网络证明。不得将排除项、现场值未观察、方案未采纳或程序未实现重新列作本轮原生静态缺口。

## 2. v16 复核结果和本轮起点

以下是 `tx-01-38` 主智能体复核的继承种子，不是本次设计或子智能体的新 IDA-DIRECT。执行者对自己实际读取的区间另作标注；若新读取与种子冲突，保留反例并修正新结论。

| 问题 | 已有依据与需修正的表述 |
| --- | --- |
| stored-confirm | 进入 confirm 分支时 `9BB39A` 先查询 action14；AL=false 才在 `9BB3A0/9BB3A3` 检查 stored byte。stored 非零允许 **14=false 仍创建**，不跳过已经发生的 query。v16 报告 W02、证据 E02 和 README 有相反摘要。 |
| action19=false | selector scan 与本 pass 的新 admission 分开。`9BB274` 起检查已有 row 的 K/selection、`9BA980` 等 guard；若已有有效 row，即使本 pass 19=false，也仍可能走14/15及9BB415。v16 E02 的“19=false必无14/actual create”缺少“本 K 没有既存有效 selector”等前提。 |
| 引用总量 | vector ownership 对 B 净减一次、局部临时 balanced 可以保留；不能由此断言总量 ΔB=-1、外部 R=0。已知通知可经 `A212C0→913760→902F10` 清持久 source pair，其 strong 变化不是新的 vector decrement。 |
| S2 返回规则 | v16 E03 的“neutral/原生 passthrough”没有按运行域分开；活动 C/S gameplay 中身份/key 不确定时透传可能绕过统一 queue。需明确每种返回，不能并列选项。 |
| writer 时序 | v16 E04 把 EndAuthority 和 RetireB 合在 TopUpdateExit 一行，又称“完整 update normal post”，容易把 previous 提交延迟到顶层退出。现 source 的 Authority post 在 StandardRoom original 正常返回后；TopUpdateExit 在完整 `g_update()` 返回后，是另一边界。 |

已可继承：基础布局/ABI/38+6 表、register/keyed lookup、by-value/移位临时的主要引用责任、无 B TLS 的 JoinControl 独立 scope、普通 Main B erase ABI、TopUpdateExit 后置、两个 Popup 空 callback/payload、9BB415 deny 补栈到9BB561→9BB567、A 的真实 query 与有条件 rebind、raw/canonical 去重、held V2 和六 consumer/action 表。只复读本轮依赖的区间。

## 3. 工具、资料和证据口径

先读根 AGENTS.md、现行规范和项目技能 `C:/Users/114514/.codex/skills/isaac-cs-refactor/SKILL.md`，再读架构计划、T0 task plan、`cs-t0/README.md` 的现行要求与本题相关部分。随后读本文、topic README、v16 报告/证据、[contract v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md) §4–§9，按需参考[完整设计 v15](virtual-device-owned-abi-static-confirmation-next-work-design-v15.md)和[续接设计 v16](virtual-device-owned-abi-static-confirmation-next-work-design-v16.md)中的已证接口；冲突以现行规范和本文修正为准。

用户已明确要求使用恢复的 IDA MCP。本次设计已发现 survey_binary、server_health、decompile、get_bytes、py_eval 的实际 schema，但未新读 IDB，不宣称当前连接已成功。上次复核数据库为 `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe.i64`，输入同目录 isaac-ng.exe，32-bit，base `0x400000`、image size `0x8FE000`。执行者发现当前工具，首个 IDA 分析调用 survey_binary minimal，仅使用非哈希定位信息，再读服务状态；不计算、校验或输出哈希。

IDA 只读静态数据库。可反编译、读 raw/指令/xref、用纯静态 py_eval 枚举 IDA Heads；不附着进程、不读进程内存、不设断点/单步，不 patch/rename/set_type/define_func/save。函数输出截断或 cursor.done 不能代替指定区间已读；关键 guard/栈/间接调用核对实际指令和地址。工具中断保留实际终点，继续独立 source/文档工作，不反复轮询或重做全部已证路径。

| 有限读取 | 用途 |
| --- | --- |
| `9BAC40` 的 `9BAE20–9BAE70`、`9BB260–9BB3E0`、`9BB40E–9BB429`、`9BB561–9BB56E`；必要时 caller `6FA540` | 已有 selector、19 admission 与14/stored判断顺序、合法 no-create、pass scope。 |
| `906D60` 的 `906E70–906E7B`，`A1D610`，`A212C0` 的 listener loop/`A2135E`，`913760`、`902F10` | callback 登记到 Manager+0x34、间接 dispatch、清 source pair 的实际 release。A20EB0/A21570/906020 仅按需要复读，不再全体重查。 |
| `954CD0` 的 case2 `95518F–955231`，`954B40`；`864C30/864D80` | 有界澄清 mode 条件与 optional Lua callback，不要求重做所有 mode/Mod。 |
| `active_apply.cpp::BeginAuthority/EndAuthority/Evaluate`；`active_runtime.cpp::Authority/TopUpdateExit/LifecycleCallback`；`loop_runtime.cpp::StandardRoomHook/UpdateHook` | 明确 source 已有 pre/original/post 与顶层退出的先后；区分当前实现和候选。 |
| lifecycle `ReleaseClass/RemoveAndDestroy`，contract v2 §4，input_core/active_dispatcher 的必要接口 | 继承无双减、NEVER、participant queue 与唯一 writer 差异，不运行任何旧入口。 |

地址表均为 VA，RVA=VA−`0x400000`。证据至少含支持结论的原始指令/字节摘录、guard、对象/参数来源、效果及限度。区分 `IDA-DIRECT`、`SOURCE-DIRECT`、`STATIC-INHERITED`、`STATIC-DERIVED`、`CONTRACT`、`USER-ASSUMPTION`、`DESIGN_ONLY`；没有新读取的继承项不得改标 DIRECT。

## 4. 同一执行目标内的工作内容

### W01 — 修正 Join 的 selector 与跨 pass 轨迹

先核对 `9BB39A→9BB39C/9BB39E→9BB3A0/9BB3A3`：仅在确已进入14/15分支的条件下说明 query14 必先发生。区分“14调用”“14返回true”“stored flag允许创建”，也区分 Manager lookup 是否实际命中 owned slot；不能用 stored 跳过 true 要求，推成跳过调用。

把 per-device 新 admission 与 subsequent selector scan 分开，列 K/selectedType 有效、`9BA980`、20/21 导航等确切 guard。已有 row 可以来自前一个 pass，action19 false 只否定当前 admission，不能无条件否定本 K 或其他 selector 的确认/创建。结论注明是“此 B K”还是“整个扫描”，不把所有 native 创建路径纳入 B owner。

保留无 B Player/consumer TLS 的独立 JoinControlScope：在9BAC40/action19前打开，6FADAC关闭；scope/token/command expiry 不依赖19成功，query只读，+0x54仅native C0/C4写回。保持 8/10 neutral、不增加第七覆写、控制命令不混入 gameplay 0–11/Resolve。actual attempt只在精确9BB415，由独立 owner恰一次 admit；deny补4字节栈到9BB561，9BB429仅绑定观察，成功后置仍沿用户接受前提。

交付手工轨迹表，至少覆盖：①无既存 row且19=false；②既存有效 row且19=false、14=true；③同②但14=false/stored=1；④14=false/stored=0、15 true/false；⑤19新 admission但有效row/导航guard不满足；⑥19+14普通首次Join；⑦重复query/flush；⑧同update第二pass和成功后重复confirm；⑨no-pass/过期；⑩retire遇pending publish。每行写初始 row、scope/token/sequence、实际 query顺序/返回、actual call、bind/pass close/终态。继承正确轨迹可引用，不必重复全部函数。

### W02 — 修正引用账本的责任边界

保留 lookup-local、by-value argument、两通知 temporary、shift temporary 各自 acquire/release，B tail/middle 的 vector责任恰消费一次，coordinator只 `RecordVectorRefConsumedByNative`、不再 `ReleaseClass→port.ReleaseStrong`，不用 RemoveAndDestroy/FinalRelease。

限定读已知 listener 链：906D60将913760/用户数据登记到 Manager+0x34的list，A1D610写node，A212C0迭代并间接调用；说明该链依赖初始化/登记成功、节点仍在对应list及匹配条件，不宣称每次remove必定触发913760。913760 source-pair key匹配时经902F10(pair,0)替换关联，核对旧control+0x0C release；这是外部持久引用的变化，可与其通知temporary balanced同时成立。

把 pinned root 与其他外部引用 R 分开；在所列 locals 均结清、root由既定owner保持的限定下，可写 `ΔStrong_B,total = -1 + ΔR_B`。`-1`只表示本次被移除的vector责任；R允许已读callback的持久释放/其他有条件变化，不能强制R=0。survivor也只保证vector责任保持一份，不把它外部总引用宣称恒定。无需穷举所有observer或证明所有R最终为零。

交付一张事件账本及有/无匹配source association、B尾/中部、重复retire/已缺席的责任表。明确NEVER常驻不等于可以多减引用，root/control有效状态保留；exact pair absence、UNREGISTERED、B member/context cleanup和A下一reader顺序沿已证候选，新增变化说明数据依赖。

### W03 — S2 返回规则、writer 与时序收口

给一套明确候选，不留下“neutral/原生透传任选”的实现歧义。按运行域、receiver/participant、绑定状态、generation、consumer/action、返回结果和original调用责任列表：

- 活动 C/S 的受控 gameplay 查询：唯一有效participant读取同一S2；未绑定、stale、K/pair不一致或暂不能唯一归属时中性，不回退为未经queue/Resolve的即时硬件世界输入。
- A 的普通UI/非受控查询、尚未进入C/S或已完成恢复的物理路径：按明确域保留原生行为；原生original是否必须调用恰一次，与最终向gameplay返回S2/neutral是不同责任。
- B owned receiver：有效JoinControl优先，其次有效gameplay S2，其他/retired输出中性；没有可虚构的Base logical original。真实hardware sampler读取原硬件，不读synthetic结果，不回投B输出。

对于normal helper前置optional Lua callback，沿864C30/864D80核对 `dword_C71690`、mask `0x2000`、callback id13、非nil返回等有限条件；注明可能在device-query hook前返回及本轮Mod非目标，不把它说成NetInputDevice/网络未读链。若中性策略需要新的接入位置，只在候选写明source责任与范围，不假称当前hook已覆盖，也不扩展全Mod兼容。

明确至少五个不同阶段：physical poll/Submit；JoinControl pass；StandardRoom Authority-pre/每participant最多一次Resolve及current发布；该original正常返回后的Authority-post/previous提交和visible关闭；完整native update正常返回后的TopUpdateExit/退休排程及pause-gap处理。只使用已证局部先后，不拼接未证的全局必达顺序；一次顶层update可能没有Authority，不把它当tick。

用源码直接说明 `StandardRoomHook` 中 `g_standard_room()` 后调用Authority(false)，再由`EndAuthority(true)`提交previous；`UpdateHook`中的 `g_update()` 后才触发TopUpdateExit。分开EndAuthority和RetireB的API/owner/时点，禁止以顶层退出作为正常previous writer。retire/request只撤visible/使generation失效，TopUpdateExit再做native移除清理；不额外写previous。为generation变化、pause/loading/noRoom、重复poll/query、B退休但A继续给可执行的字段/tag规则与手工时序表，防止旧previous跨generation制造edge。

保持Authority-pre/current、Authority-post/previous、+0x78仅native活动条件这一具体候选；与contract v2 §4 poll唯一writer的差异明确列 `DESIGN_ONLY / 未采纳`，本轮不修改contract，也不把正式采纳或production编码当成静态读取条件。继承participant独立queue/sequence/history/planner/pending、共享tick、raw/canonical去重、held V2/cross-family动作表，不重新设计协议。

### W04 — 有限条件澄清与完整交付

按tx-01-38种子定点确认：954CD0在switch取 `n298+8` 的case2通向9551BD→6FA540，因此该case选择时的 `n298[2]==2` 已是静态条件，不需要现场读值才能陈述；它不等于Offline/Online模式语义，也不证明954B40全部guard必达。保留954B40/95BCA0真实setter和A身份/v91/上下文的条件。将context-nonempty reset的调用定位到95B867，selector-clear的90AF30调用定位到95BBCB，不能拿95B87F/95BBB6的相邻代码当实际call。

新报告开头直接回答：本轮限定正常路径的原生静态依据是否足够；三类问题各自如何解决；候选是否内部一致；哪些设计尚未采纳；哪些运行值未观察；哪些生产代码未实现。P01–P06保留为功能索引，但各栏严格分开，不用后四类状态自动否定第一类静态完成。

继承v16中正确的表格和事实须标来源，勘误集中在v17列旧说法/新说法/依据。同步README的当前入口及真实进度，显式纠正顶部旧摘要中“stored跳过query”“R恒定”“最终审阅未完成”等过时文字；旧v16报告/证据和历史通过记录不改写，旧README记录可原样保留在历史区并由新记录说明被更正。

若仍有真实静态缺口，仅列具体函数/字段/分支、已读终点、为什么影响本目标、最小补读内容；不写泛化的“生命周期/模式/全部功能未闭合”，不因未实现或现场值未知启动又一轮完整逆向。未发现新反例时允许结论为“在声明前提内静态依据已足够；候选、实现和运行验收另列”。这不是编码准入流程，已证基础可继续实现。

## 5. 交付、编辑权和执行约束

子智能体只新增/编辑：

1. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v17.md`。
2. `out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md`；必要分册同v17前缀且全部被主证据引用。
3. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/README.md` 的当前入口、tx-01-40结果与必要勘误说明；保留tx-01-39设计和全部历史事实。

不改工作设计、旧报告/证据、现行规范、contract/schema、production source或IDB，不提交/推送Git，不创建侧栏新聊天或再委派。主智能体交出上述文件及同一IDA只读会话使用权，执行期间不并发编辑/操作IDA；仓库已有其他改动不属于本轮，不还原或清理。

报告到证据用 `../../../../../../out/cs-t0/...`；证据到docs用 `../../docs/...`；topic README到证据用 `../../../../../out/...`。引用人工核对，不运行链接检查脚本。shell可用PowerShell `login:false`。

过程不运行测试、构建、harness、probe、smoke、canary、verifier、hash/link-check或可执行状态模型。不操作游戏、进程、debugger、注入或Hook。文件/source/IDB读取、rg、静态反编译/raw、人工表格推演属于资料分析。

## 6. 整体人工文档审阅与结果记录

`tx-01-40` 全部分析和编辑完成后，只做一次整体人工静态文档审阅，覆盖本文、v17报告/证据/分册、相关source/contract及README修订。核对三类修正、有限条件/地址、query与副作用owner、无重复release、S2运行域与writer时点、事实标签、引用和历史保留。不要边分析边跑检查程序或分项验收。

最终记录分别说明文档准确性、限定正常路径静态完整程度、候选内部一致性及未采纳差异、现场未观察、production未实现。本轮不要求任何现场观测。不得预填PASS；审阅结束后只回填实际结果，失败如实记录，不自动修补、复审或改编号重跑。工具中断也保存已完成证据和草稿，记录可续接终点。

## 7. tx-01-39 设计交付记录

本轮目标是写成可执行的收尾设计并按用户要求启动GPT-6-Luna max子智能体。新增本文，README增加tx-01-38复核摘要和tx-01-39执行入口。依据是v16产物、会话复核、现行规范、contract及相关source；本次设计未新读IDA、未运行测试或游戏，未修改旧报告、source、contract/schema或IDB。

设计交付唯一一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑结束后一次通读实际保存的本文与README修订，对照现行规范、v16交付及会话复核、contract和已读source，核对范围、三类收尾、有限读取种子、候选/事实分类、引用、编辑权及唯一整体审阅安排。审阅后仅回填结果，未复审。本条只记录设计交付，不是tx-01-40静态执行结果。
