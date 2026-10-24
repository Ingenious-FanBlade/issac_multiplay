> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v13

日期：2026-10-13  
设计交付推进：`tx-01-29`  
静态执行推进：`tx-01-30`（W01–W05 为同一完整目标的内容编号）  
状态：`DESIGN_ONLY / 承接 v13 复核；确认报告 v14 待执行`  
执行者：用户指定的 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。本文承接 `tx-01-28` 对[报告 v13](virtual-device-owned-abi-static-confirmation-v13.md)、[证据 v13](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v13.md)及[工作设计 v12](virtual-device-owned-abi-static-confirmation-next-work-design-v12.md)的只读复核。v13 有实质新增证据，但正常功能静态目标仍未完整，且存在 Room 条件、状态分支、字节摘录和查询时点问题。旧 v13 原文及其“审阅通过”记录保留，不能当作这些问题已经解决的依据；本轮通过新读取、候选补全及新报告推进，不修改或重审旧交付。

## 1. 完整目标与适用范围

交付 v14 报告、证据及 topic README 实际进度：纠正原始指令/条件和 raw→canonical 推导；接完 disconnect/reselect Popup 到 A 的限定正常行为；定位可用的 B 专属解绑或退出及 native 清理次序；给 A/B 输入分流、witness、S2 发布/查询与正常边沿一套完整工作候选；补全 JoinCommand/pass 和 B 创建尝试的开始、绑定、发布、终结及关闭。依据具体结果说明可开始的实现与真实剩余静态项。

正常前提明确为：分配成功、对象/缓冲有效、主线程顺序操作、Offline/local-co-op、**Online roster 为空**、唯一正设备 K、B 创建前 Main Player vector 恰有 A。Online roster 空、n298+0x4B3D8 context vector 空、Main Player vector 的人数/成员分别判断。原生 Join 能可靠创建并发布 B 继续沿[用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，标 USER-ASSUMPTION / DESIGN_ONLY；不重做 full Player Init 或全局 manager-this，不升级为新二进制/运行事实。

保持自有 38 槽 device、六覆写槽、六槽 control、S2、NEVER；原生 base ctor 前固定 DLL，ctor 后写自有 vptr，BASE_CONSTRUCTED 后不析构或释放对象/control/有效基类状态，恢复后 DLL 常驻。每 session 至多一次自有设备创建，失败不自动重试；B 的一次实际创建尝试 owner 单独说明。A ordinal0、B ordinal1，ClientId→ordinal 在 session 内冻结；Player*/device*/K/平台 handle 仅为 generation 内临时关联。A/B 使用相同 canonical、sequence、target tick、queue、dedupe、Resolve/apply 规则，没有 B 即时旁路。通用 InputManager 可以复用；C/S 不依赖 Online Manager、NetInputDevice、FrameStore 或旧 all-slot-ready 条件。

本轮只做正常功能静态分析与候选设计，保留线程、生命周期和 Hook 恢复约束。正常 pause/loading/noRoom、重复 poll/query/pass、按住/松开/再按、B 退休及 A 后续读者在范围内；不扩展到 OOM/EH、worker 并发、原子镜像/计数溢出、ABA、全映像 alias、持久化、异常恢复或 05-06 全网络证明。

`tx-01-29` 在本文与 README 编辑完成后只做一次人工整体静态文档审阅。`tx-01-30` 在全部分析和 v14 产物完成后只做一次人工整体静态文档审阅。过程零测试，不设 gate、不校验任何哈希，不运行构建或游戏；W 编号不增加验收次数。

## 2. v13 复核结论与本轮具体改进

| 项 | 已发现问题或剩余要求 | 本轮结果要求 |
| --- | --- | --- |
| Q01 Room flag 写反/分支混用 | v13 E02 写 n298+306108 bit0 非零时调用6FADC0；raw实际非零跳到955238替代分支。显式6FA540/6FADC0位于case2，不可合并称case1/2。9551B4调9505E0，9551BD才调6FA540。 | W01重写精确分支及对A的正常场景结论。 |
| Q02 字节和地址 | v13 E06连续字节摘录遗漏9BB421的89 45 F0。9B9CD0 end推进在9B9EAD，旧证据写9B9EAF。 | W01核对相关raw区间/指令地址；以实际指令为准，不在字节中间省略却声称连续。 |
| Q03 raw变化不等于canonical变化 | v13 E05把release/axis改变概括成persistent field必改变；当前BuildRawSample只持久化axes和Shoot，8–11只传rising edge，falling未编码；量化也可能使不同raw轴值得到同canonical。 | W04分层手推去重、hold/release/repress及候选held版本；不再用“所有状态变化都提交”。 |
| Q04 S2覆写位置混淆 | v13报告及E04写consumer原函数执行后才读slot S2。现行InvokeConsumer在原函数内部提供TLS，逻辑query wrapper在返回给consumer前替换查询结果；自有槽又是另一层。 | W03明确三层调用与作用时点，不能事后读取快照来控制已经发生的原生动作。 |
| Q05 输入路由/唯一writer未定 | v13并列留待决定。InputCmdDraftV1已含client_id；现行InputQueue绑定单一client_id并拒绝WrongClient，Resolve是破坏性的。 | W03给明确的ClientId分流和分区队列/统一Authority方案，不假定一个现有Queue实例能直接服务两人。 |
| Q06 B退休/UI→A未完成 | 已证whole-vector clear、全员unbind以及setter(-1)候选，尚无B-only入口/清理/下一reader完整序列；Popup生命周期只部分说明。 | W01接具体Popup场景；W02沿未读的正常入口实质推进，或给准确不满足原因及有限缺边。 |
| Q07 Join owner不完整 | 标出create/bind/pass return但没有完整的一次尝试终态、发布后置与重复pass关闭规则；setter返回不等于整个Join完成。 | W05给owner状态表、Hook责任及先绑定后发布再成功终结的候选；不重做可靠创建前提。 |

继承已证且无需重做的事实：Base/Control布局与ABI、source pair/own key分层、callback receiver及返回通道、native insert/indexed/keyed lookup/remove、context factory/通知清理、poll-zero自动移除、Popup递增ID/ID lookup、95B310 generic fallthrough、Main Player create/whole erase、Player vtable→六消费者及带guard的限定poll→Join→Room→Player链。复读只针对本轮实际依赖，不能因有未知项就把已证基础全部降回UNKNOWN。

## 3. 工具状态、证据种子与资料

tx-01-29设计准备实际调用survey_binary(minimal)、server_health，IDA服务正常，Hex-Rays/auto analysis ready；输入 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，IDB为同路径 `isaac-ng.exe.i64`，32位，imagebase=0x400000，image size=0x8FE000。未输出、比较或校验哈希值。

以下是tx-01-28主智能体直接读取及当前source的定位种子，不是tx-01-30执行者的新IDA-DIRECT。涉及关键ABI、对象、条件和接入点须复读raw；若继承则标明来源。先发现实际工具及schema；disasm的cursor/total不代替相关地址实际行。如果工具声称done而未返回目标区间（tx-01-28读取9BAC40曾出现该情况），用get_bytes及纯静态IDA指令读取补足，不声称目标已读完。

| 定位 | 种子及用途 |
| --- | --- |
| 954CD0：9551A3/9551B4/9551BD/9551CF/9551D6/955231/955238 | case2入口；9505E0调用；Join调用；test bit0及jnz替代分支；正常Room调用。bit0 raw为F6 87 BC AB 04 00 01 75 60。 |
| 954CD0：9553A6..9553D5、9553C7/9553D0 | case1的menu/Popup/Special条件及9897D0；与case2分列。 |
| 9BB404..9BB429 | create/bind区间：9BB415 call9B9CD0；9BB41C push selector K；9BB41F mov ecx,eax；9BB421 mov[ebp-10],eax；9BB424 call7A6450；9BB429为返回点。 |
| 9B9CD0：9B9E63/9B9EAB/9B9EAD/9B9F1F | count写Player+161C、append、end推进、Player虚槽初始化。 |
| 95B476/95B720；9C25E0；9C01D0；9C0960/9C1260/9C34F0/9C3990 | 两Popup的descriptor创建、复制、更新/关闭/移除与busy读取。9C25E0在9C26D6令descriptor首byte OR7；这条静态writer不能只写成现场值未观察。 |
| 95B811/95BAC2/95BACC/95BBB6；95B87F/95BCA0 | Popup ID保存/查找/清除；条件性解绑/重绑及实际Player选择。 |
| 6FADC0→6FBA4D→802980；9BB5D0 | 已知Room/实体更新链与其后Player管理候选；先确认callee作用，不预称为selective erase。 |
| 9B9150、417670、Player删除/成员清理callee及正常caller | whole-vector clear已证；本轮优先沿关联列表/成员处理的尚未读正常caller查selective入口，不再次仅列全员clear结论。 |
| A20EB0、A212C0、913760、90AC70、417870及v8实际query helpers | Manager/context清理、keyed reader、索引fallback、六消费者到查询路径；连接退休后的实际读者。 |
| input_core.hpp:65/67；input_core.cpp:344/388/467；active_dispatcher.cpp:62/265/371 | client_id已有；Queue绑定单client、WrongClient、destructive Resolve；routing/partition方案的source依据。 |
| active_runtime.cpp:527/553/584/653/709/908/937/986/1036/1394；active_witness.cpp:29/51 | raw投影、generation清理、physical提交、单A身份/Authority、query与consumer两层、Player/TLS。 |

主要入口：[工作设计 v12](virtual-device-owned-abi-static-confirmation-next-work-design-v12.md)、v13报告/证据、[报告 v10](virtual-device-owned-abi-static-confirmation-v10.md)、[证据 v8](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md)、[v2 W06/W07](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w0607-draft.md)、[v2 W09](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)。source为[active_runtime.cpp](../../../../../../../cs-t0/native/src/active_runtime.cpp)、[active_dispatcher.cpp](../../../../../../../cs-t0/native/src/active_dispatcher.cpp)、[input_core.cpp](../../../../../../../cs-t0/native/src/input_core.cpp)、active_apply、active_witness、raw_apply_runtime.hpp和loop_runtime。旧测试/hash入口仅可作为历史source阅读，不执行。

## 4. 本轮静态执行工作

### W01：准确勘误与Popup→A的限定正常结果

先核对Q01/Q02关键raw，给对象/VA/RVA/byte-region/实际instruction/call地址表；正确区分case1、case2、bit=0正常Room和bit≠0替代更新。其后只补相关条件和callee，不重读整函数来凑“已完整读取”。Join被Popup busy挡住不能推出Room/A被挡；Room可到达也不能推出所有consumer无条件到达。

沿95B310两次9C25E0生成的descriptor、对首byte/回调/payload的实际writer、9C01D0复制，追到9C0960/9C1260/9C34F0的state、callback、终结和列表删除。区分descriptor初值、特定Popup后续改写、对象flags及Room/update flags；已定位静态writer继续读取，不笼统把条件列为运行值未知。9C27C0是descriptor布局/文本尺寸处理种子，不预称其为所有flags的normalize函数。

按限定实际链列五个正常场景：Popup存活未完成、完成/取消、设备remove后的下一reselect、K重绑、B处理后A下一Room。每场景列Main/context guards、Popup ID/state/flags、Join/Room/Player/consumer到达、A的K与实际设备查询选择。继承已证六消费者与v10链，只补缺的guard/dataflow；不要求无界追完所有游戏consumer。实际值未观察与某个writer/branch还没读分别列明。

交付完整勘误表、两Popup的限定生命周期和A结果表；某场景仍未知则给具体字段/callback/caller/终点，不能仅写“所有后续reader未知”。

### W02：B专属解绑或退出及native清理

从v13真实vector/关联列表证据继续，优先查尚未读的正常member removal/Player cleanup caller，包括Room后Player管理候选和关联list erase caller。先确认对象与作用，再判断是否能只作用B。有限深读4–8条新相关caller/callee路径；记录选择依据、实际读完的分支、候选排除原因及仍未读的具体VA，不重复研究whole-vector clear来代替selective入口，也不声称全映像无B退出。

退休目标仍为B解绑或退出。如果有selective exit，证明Main membership处理、相关list/child Player/ordinal后置及A保持；如果保留B，必须证明正常input reader不会经-1 all-active fallback读A或旧Bpublication，且不再产生B输入副作用。单setter(-1)、自有slot neutral和Manager pair remove均不能独立证明这一点。

恢复B的六类consumer到实际bool/scalar/query/helper/Manager选择的有限路径。按K/ordinal/Player身份列正K、K=-1、pair已remove、RETIRING的读者分支和B/A结果。若需要额外Player-input query中性方案，具体说明interception层、receiver、作用范围、original调用和guard；标DESIGN_ONLY/新增实现，不冒称原生解绑已经存在，不跳过整个consumer或裸写Player/cache/world来伪造结果。

形成逐事件次序：RETIRING关闭Join/input admission和publication、generation失效、中性；在已证主线程阶段处理B；context/listener/selector清理；native Manager remove；下一poll/reselect/Room/consumer。实际原生callee内部先后以raw为准，不能预设notify先于或晚于setter。登记时poll非零、Pause中性且保留登记、NEVER常驻；不可把永久中性且仍登记当完成退休。

分别给B-only且A继续、whole-run teardown的结果。没有可用原生入口时，对每个候选给具体失败步骤、准确新读取终点、可供实施的新增行为提案及仍需的有限证据；不能只写“待找B退出”。至少给一条实质推进后的序列表，即使其明确结论是某一步无法完成。

### W03：统一参与者调度与一套完整S2工作候选

本轮以 **Authority-pre唯一发布current，normal post提交previous并关闭查询，own poll不推进S2** 为评估主候选。它是未采纳DESIGN_ONLY工作方案，不修改既有contract v2；输出具体条款差异。执行者需给一套内部完整候选，不再并列两个尚未设计的writer选择；若主候选确实不满足目标，给静态反证及同等完整的替代建议，不能静默改成poll提前Resolve。

以已有InputCmdDraftV1.client_id建立冻结ClientId→ordinal路由，分开session identity和generation内Player*/K。现行Queue绑定单client且Resolve破坏性：候选须给参与者分区队列/序列历史/planner/pending与统一Authority调度的具体结构和API；相同策略不等于直接把两client塞入一个现有Queue实例。每个有效参与者每Authority tick至多一次Resolve，统一tick不等待全员；禁止第二人Resolve读空第一人的结果或复制A命令给B。

明确A physical projection→canonical submit，以及B的C/S command入口→同一规则入队→Resolve→自有设备输出。B own poll是输出设备行为，不是B输入采样或再次提交command的来源。本轮只设计有界输入接口，不证明05-06网络实现。API表给参数、返回、身份/generation、调用阶段、字段writer和失败中性行为；现行无该API则标新增候选，不把新名称写成已有source函数。

彻底区分三层：Player wrapper建立当前参与者身份；consumer wrapper在调用原consumer前建立TLS/witness、原consumer只调用一次、返回后关闭scope；原consumer内部的device/query返回按B S2快照生效。自有+30/+34/+38读取自有不可变publication，保持已证ABI；现行InvokeLogicalBool/Scalar先调原query再替换返回只是另一种query-hook实现，不能把该顺序搬到整个consumer后面。新增query interception是否需要调用原query须按实际层次、ABI和用途解释，不为保持观测wrapper一次而重复调用consumer。

候选明确每ordinal的current/previous/publication id、target tick、session/apply/binding/slot generation；Authority-pre一次写current或显式neutral，normal post才提交previous并关闭visible，scope外不读旧edge。owned poll在REGISTERED期间按已证语义非零且不再锁存/改previous；重复poll/query不写共享状态、不Resolve、不消耗edge。给与contract v2 +78及query条款逐项差异，未采纳仍可判断候选内部一致性。

继承并核对跨类别许可：Movement→0–3/11、Shooting→4–7/0–3/11、Bomb→8、Item→9/10/11、Card/Pill→10/9/11、Drop→11。真实B Player、K、scope/generation/publication与consumer/action须一致；失效只给相应输入中性。正常A↔B traversal只换临时TLS，不触发全局OwnerSwitch清队列。相关reader还没证则列具体边；不靠physical fallback实现B。

交付接口/字段owner表和合并事件表，至少纸面覆盖A后B/B后A、缺A/缺B输入、future target、无输入neutral、重复poll/query、post后query、first noRoom先Submit后无Resolve、second普通miss在TopExit clear、loading recovery、pause首个恢复Authority、Join前B reserved/后B激活、B退休后A。表记录逐参与者queue/pending、tick/epoch、previous/current及scope；不执行模型。明确缺输入策略与旧null/passthrough行为的差异，正常缺B不能停A/全局Authority。

### W04：raw/canonical边沿、持续状态与重复提交

逐层复读UpdateProjectionHistory→BuildRawSample→CanonicalizeRaw→SamePersistentState→SubmitPhysical→EdgeTargetPlanner/Queue。区分原始action level/量化轴、rising/falling、v1 held/edge、command identity和query。当前source只把8–11的rising写edge，falling不传；axes量化/归一化后可能同值。因此raw发生变化不保证新canonical，不保证新sequence。

按同tick/source/K且history有效手推：相同level重复poll、8–11按下→松开→再按、Shoot松开、微小轴变化量化后相同、轴canonical实际改变、不同completed tick、source/generation真正改变。对每步列raw level、history、rising/falling、canonical held/edge/axes、抑制条件、sequence/target、队列状态。初次history无效的边沿规则按source真实行为列，不凭想象给first press。

补回v13遗漏的9–11持续按住候选：保持Shoot held bit0，候选bit1=Item9、bit2=Card/Pill10、bit3=Drop11；edge bits0–3仍对应8–11。标schema版本变化、valid mask/encoder/decoder/Validate/BuildRawSample/Pressed/Triggered/Scalar的必要修改责任；不实际改schema/source。动作8是否只需edge按已证native reader说明。将候选state纳入W03唯一publisher，不能只写“以后版本化”。

区分v1当前行为与新候选：v1某些release可被抑制是事实；候选持续量release要可表达并正常释放held。重复query不生成command，不承诺任意原生世界副作用都恰好一次；只在已限定scheduler/consumer范围陈述边沿语义。

交付确切抑制条件、v1轨迹、完整level/edge候选及W03接续。现场未观察不是这套source静态推导的缺边；未实现也是另一类状态。

### W05：JoinCommand/pass与一次B创建尝试的完整owner

继承正常导航/confirm/cancel/K guard，仅有限复读新增Hook/关闭点所需分支。准确区分自有设备创建owner、JoinCommand admission/open/navigation/pass、B实际create attempt。前两个action查询或导航不是actual create；实际9BB415前才进入AttemptStarted。

候选owner给每session状态与准入条件：未尝试→实际开始→原生创建返回→K已绑定→用户规划发布后置确认→成功终态；已实际尝试但后置不满足→失败终态且无自动重试。guard未过或未抵达create不消耗实际尝试，但命令按其epoch/pass关闭；已经成功/失败不能再进入同session actual create。异常null/OOM/EH仍超出本轮正常前提，失败状态只是候选要求，不声称原生无null guard的路径安全处理了它们。

给callsite wrapper/interception的ABI、selector K来源、attempt context/返回Player*存储、Begin/Bind/Publish/Finish各责任与原始callee exactly-once。9BB429仅setter返回/绑定观察点；此后native Join还有Player/Room/位置处理，不能先宣称完全成功，再把参与者发布留到不明确的以后。沿可靠创建/发布前提给正常Join-return后的确认窗口，6FADAC为pass关闭点；说明如何同时区分无create与有create后置。不重做fullB Init，但也不把用户假设当已安装Hook。

明确JoinCommand字段session/generation、K、sequence、target update epoch、pass id、action及可见范围。firstB不存在时control lane可供nativepass；成功发布后gameplay才进统一scheduler。query只读、绑定pass后不供第二pass重用，normal pass completion/无pass UpdateExit/generation/retire都关闭。update epoch不等于Authority tick。

纸面覆盖19+14同pass、导航/取消、重复query、同update第二pass、无pass update、另一K、原生guard未过、到create但候选后置失败、成功后再次confirm、retire与pending相交。与W03 epoch/scope/participant激活时点一致；列最小新增Hook/恢复事务差异，未实现，不安装或修改字节。

交付owner状态/Hook责任表和轨迹，不能只列9BB415/9BB429两个地址便称创建尝试管理完成。

## 5. v14产物与判定方式

- 报告：`docs/refactor/cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v14.md`。
- 证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v14.md`；必要分册同v14前缀且全部引用。
- 更新topic README tx-01-30实际结果，保留v13原文/审阅、tx-01-28复核和本文。

开头逐项回答Q01–Q07及W01–W05，区分目标满足、部分完成、已证某候选不满足但目标仍未完成；“已读取/已研究”不是完成状态。证据给对象、VA/RVA/raw区间/实际call、ECX/参数/返回、guard/writer/reader/后置及限度。IDA-DIRECT仅本轮成功读取；SOURCE-DIRECT给当前文件/位置；STATIC-INHERITED给旧报告/节；CONTRACT、用户假设、DESIGN_ONLY分别保真。分页/函数尾/工具错误与相关分支终点如实记录。

P01–P06保留功能主题：注册枚举、type/name/binding、Offline Join、B key/ordinal/退休、snapshot/query、pause/context/lifecycle。每一主题都分列native静态已证/真正缺边、候选及待采纳差异、现场未观察、生产未实现、后续整体验收事项；不要把P编号仅改作五类分类标题而失去各功能的状态。生产对象未写、现场K/COL未观察、候选未采纳均不是native ABI缺边。

结尾列已证基础可独立实施的layout/construct/vtable/control/登记、内部完整但待采纳的候选、真正还需读取的有限原生行为，以及后续实现的具体修改责任。基础可开始不表示完整B功能已证；候选未采纳也不阻止已有基础编写。对剩余项说明本轮新增事实消除了什么、还缺哪个caller/字段/条件，避免重复把全部链重新列未知。

## 6. 所有权、工具范围与唯一整体验收

本文完成后以新上下文启动一个 `model=gpt-6-luna`、`reasoning_effort=max` 子智能体执行tx-01-30，不恢复已完成v13实例，不再委派或创建侧栏聊天。执行者先读现行规范、项目技能/资料及本文；主智能体交出v14文件、topic README编辑权及同一IDA只读会话使用权，执行期间不并发编辑或操作IDA。

允许静态文件/source/IDB读取和本轮文档编辑；不改旧报告、生产source、contract/schema，不patch/rename/set_type/define_func/save IDB，不用debugger、访问游戏进程/内存、附着/断点/单步、游戏输入、注入或安装Hook。纯静态py_eval可读目标地址指令，不能运行状态模型。不得运行测试、构建、harness/probe/smoke/canary/verifier、hash或链接检查脚本。PowerShell可用login:false。工具可用不扩大实机授权。

tx-01-30全部分析和编辑后一次人工整体通读本文、v14报告/证据/分册、相关source/contract及README。统一核对Room条件/连续字节、Popup→A、B退休下一reader、ClientId分区/Resolve、consumer内query、唯一publisher/previous/epoch、held及去重轨迹、Join owner终态/发布/关闭、功能分类和引用。审阅前可正常纠正文稿；最终审阅后只回填结果，失败列具体项，不自动修补或重审，不换编号重复验收。

分别报告文档准确性、正常功能静态程度、候选内部一致性/采纳、现场未观察和生产未实现。不得预填PASS，不以UNKNOWN标签齐全替代目标完成。工具中断时保留准确已读终点，继续独立的source/候选工作，再报告具体剩余；不中断整个任务去反复请求已有授权。

## 7. tx-01-29设计交付记录

本次新增本文并更新topic README。IDA必要元数据已确认；定位沿tx-01-28复核与当前source，未冒称子智能体新证据。未运行测试、构建、哈希校验或游戏操作，未修改旧报告、生产source、contract/schema或IDB。

设计交付的一次整体人工静态文档审阅：**通过（设计交付范围）**。全部编辑后一次通读实际保存的本文和README修订范围，对照现行规范、contract v2、v13复核、已读source和IDA定位，检查Q/W对应、Room条件/call/raw连续性、Popup→A与B退休的真实剩余边、ClientId分区及破坏性Resolve、consumer内query、唯一publisher与previous关闭、held/edge和去重轨迹、Join尝试/发布/终态、功能分类及引用。审阅后仅回填结果，无第二次审阅；此结果不代表tx-01-30/v14执行或完整正常功能已完成。
