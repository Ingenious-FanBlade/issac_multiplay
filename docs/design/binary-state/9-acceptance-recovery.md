# 第9步第五项：版本、接受与恢复

**2026-11-17 最终一致修订（E1—E9 已同意，尚未实施）**：[设计文档](eventual-consistency-revision.md)、[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。ACK 改为接收确认（E1），基线改为客机快照环与 8 帧／64KiB 窗口（E2），整批 FailedBeforeWrites／FailedAfterWrites 改为对象级失败（E3），版本 84（E8）。本文下列旧合同按时点保留，实施后以修订为准。

**2026-11-16 布局 83 的 World 发送和接纳已接线**。Host 普通 World 发送窗口为 2。普通单流 World 可用唯一保留的成功前代解码完整目标，再相对当前已接受状态准备应用。旧 base 无精确前代或所需表快照时 Reack，版本不匹配时握手失败。运行验证归第 15 步。[World 应用节奏设计](13-client-world-cadence.md)

**2026-11-15 用户采纳Body接纳修订，版本79**：[第13步10.4](13-client-incremental-apply.md#104-普通body单槽等待与前代解码基线2026-11-15用户采纳)允许普通Body用唯一保留的成功前代解码完整目标，再相对当前接受状态准备应用；Begin和实际提交锁定应用基线。旧base无对应保留根仍Reack，World／joint保持严格匹配。原正文／target／ACK身份不改，真实Success才确认；单槽等待、期限及失败方式以新节为准。本条覆盖下方冲突的零等待／所有旧base必拒绝描述，历史记录保留；第15步尚未通过。

**2026-11-15 双通道设计修订，尚未实施**：[增量应用设计](13-client-incremental-apply.md)将普通World／Body的在途接受分别管理，joint独占两流，票据回执允许跨流交错并只推进其覆盖的确认序列。单流自身的成功base、重复处理、事件前缀、写后污染与完整恢复规则保持；共享根失效仍共同撤销。下方全局单inflight描述保留原实现事实，未来以新设计和[容量表](9-budget-capacity.md#4-容量及时间表)改造；不新增线格式、ACK序列或延长期限。本次未改代码、未构建、未测试。

**2026-11-11 第9步收尾，版本65**：[预算／容量／时间合同](9-budget-capacity.md)已给出具体等待、恢复期限、重试／账本／事件／缓存／内存限制，并将带宽按用户决定改为90%＝9Mbps。第五项接受／原生污染恢复原则保持；下方版本64的“剩余预算细化”属于原时点，现以本项合同及唯一源常量为准。

2026-11-11，按用户采纳的讨论方案实施，布局整数版本 **64**。本项完成纯值报文与接受／恢复合同；Host来源、逐Peer发送调度、完整影子及原生提交仍分别由10—13步接线，第14步退役旧路径。当前工作期间不运行测试，第15步唯一统一整体验收。

唯一源为`mod/binding/state/version.json`与`wire.json`，仍349布局／2806字段／85条件；408—421、304／306及309／366尺寸保持。`wire_protocol.*`和`acceptance.*`编入产品状态库，固定头布局／宽度／字段号、消息及保护常量由`state_protocol.inc`同源生成，见[数字合同](layouts/09-wire-format/state-protocol-plan.md)。本项只涉及自有协议，不引入新的游戏字段／偏移；游戏事实沿用本地Wiki／官方REPENTOGON SDK优先的既有来源，不把静态事实写成运行观察。

## 实施前列出的失败方式

- 版本不符仍猜读旧Range，或同一整数版本承载互不相容的接受语义。
- 混用来源、GameFrame、Peer目标、表epoch和绘制拓扑；同号目标换base／正文后重发。
- 完整表／资源声明到达便删除原生对象或提前ACK；按最新表解释旧epoch。
- 逐条记录合法却把缺投影、失效继承、缺必要组或跨代引用当完整目标。
- ACK丢失使旧base帧触发反复全量恢复；joint拆成半个确认或虚构单项回报。
- 同号重复提交重播事件；latest-only淘汰顺便丢掉未确认事件。
- 旧route的主线程完成回调提交到新route；提交前检查后生命周期又变化。
- writer部分写入失败仍回复旧ACK并继续以历史A差分；旧画面已污染却仍消费。
- 无界等待／保存目标，资源缺失无限Resync，或通过可靠通道绕过带宽预算。

## 规则

布局整数版本严格相等，握手声明二进制能力；版本不符明确失败，不回退旧Range。布局源及影响互通的接受合同变化均递增版本。不计算哈希。

完整候选按 scope／route、精确表、base／target、生成布局、实际条件／投影、全部必要组／引用／资源／拓扑检查。普通单流的非零 base 可等于当前成功安装版本或本流唯一合格的成功前代版本，联合的非零 base 必须匹配两流当前成功安装版本。base=0 表示完整基线，target 必须前进。提交前重查生命周期、依赖和 Begin 封存的实际应用基线。联合共同提交与确认，真实 Success 后才确认原目标。

ContextReady仅确认元数据。资源准备只遍历本目标实际闭包；缺声明不能借后来epoch补进旧目标。不可变目录、同epoch精确重传与同run不回收按第四项合同。

布局 82 起，Client 回报当前路由实际已安装的目录对和成员借用下限 `member_floor`。目录或下限变化时立即回报，同路由每 50ms 补发。Host 只用精确发出的目录对解除基线或联合的目录等待，并独立使用经验明的单调下限释放旧成员租约。旧路由的确认随路由退休。

普通 World 在所需表交给可靠发送后即可下发。缺表使用原有 World 等待槽，仍缺表时到期只移除该候选；后续候选继续使用真实成功基线。基线、真正联合及其他恢复条件沿用原时限和回执要求。

普通 World 正在解码、准备或在途时，可另留一个可替换的原始报文。该位满时仅以严格较新的普通 World 目标替换，仍检查同目标内容冲突。该位到期只丢该包，本流空闲后才将存活报文转入候选槽。联合候选、恢复和关闭均清空该位。

同号目标内容固定，精确重复幂等，冲突拒绝。普通单流旧 base 命中合格前代时进入 Decode。未命中的旧 base 帧丢弃并重发最近成功提交的原始 ACK 凭据，joint 凭据保持双项。Host 按实际发出的完整目标核 ACK。

旧route／scope和迟到目标丢弃；缺表／异步准备有界等待，普通单body未就绪即丢；可靠单body完整恢复仍可有界等待。base无法接续、表链断裂及事件缺口请求具名Resync；非法编码和同号冲突拒绝；必需资源缺失明确失败。普通world和joint各至多保留一个最新候选，一次仅有一个原生应用批次。全部自有字节仍计共享内存池；等待截止时间／发送峰值已由[版本65预算](9-budget-capacity.md)细化；仍不捏造性能通过结论。

普通恢复以base=0完整重建所需序列，target不归零；结构有关时共同恢复。音频恢复当前有效voice及水位，不重播起点以前的one-shot。部分原生写入失败后，历史成功版本仅用于诊断，受影响事务禁止增量／ACK重发并请求完整重建；重建前不消费污染对象。没有任意写入回滚承诺。

换scope／route撤销候选和在途提交资格；同session/run资源目录可合法复用。将回绕的计数退休旧复制代次再准备，同run资源号仍不复用。普通Resync和普通换route不自动重置资源epoch；专用`RebaseStateResourceEpoch`只在生命周期owner已排空退休旧复制route后，以新route的epoch=1全量逐项重绑定全部既知资源号／kind／精确payload。它保留资源内容历史，不得调用于同一route或借此回收index；后续新资源仍超过保留的各表最大号。新复制代次的成员退休／回收历史也须由owner保留。Authority不等待Peer恢复。

## 完整消息与元数据

组／事件缓存的Context实例严格绑定当前connection/route，退休后不能把旧代次缓存返回给新代次，即使scope／epoch数字相同。只有资源内容目录按上述显式规则复用；结果与事件资格通过完整目标／Replace重新建立。

| 接口 | 责任 |
|---|---|
| LocalStateAgreement／CompatibleStateAgreement | 二进制能力1与严格相等的u16布局版本；不是旧Range兼容协商 |
| InspectStateFrame | 完整scope／版本、固定头、flags、精确段边界、计数和预算；不解码typed正文，不可安装 |
| DecodeStateFrame／EncodeStateFrameInto | 1／2段、精确成员layout、12B记录／u32 mask、typed结果与事件、整帧数量／字段访问预算 |
| Decode／EncodeStateMemberTable | 50B公共头＋10B表头＋N×33B；严格member升序、非Null身份、合法操作、Key唯一 |
| MergeStateMemberTableInto | 新不可变快照；同epoch精确重复／恢复full、准确previous、真实换形／离开及生命周期回收检查 |
| Decode／EncodeStateContextReady／AppliedAck／Resync | 62B／73或94B／61B完整消息，精确长度、joint关系、合法stream及水位 |
| StateFrameLane | 普通world／body各走原通道；base=0或joint可靠承运，绝不因超预算自动升级通道 |

Inspect先对整帧定界，允许接受状态机在解码旧事件之前识别旧base并补ACK。只有完整Decode／Encode成功的视图才标记decoded；它仍不等于完整影子或原生成功。两个分段固定world随后body，同来源窗口、batch、GameFrame及表epoch，显式提供两项Peer target；不能用来源修订猜配对。

成员表完整快照只含Join；增量的Reshape保持Key/use且shape前进，Leave精确匹配原身份。恢复full允许经生命周期owner核准的真实后继形态或已合法回收的序号，不能只因遗漏中间delta便拒绝恢复，也不能复活退休身份。`StateMemberTransitions`必须检查具名空间／layout及合法身份转移；Host核全体Peer回收条件，Client核本端在途引用与退休历史。表操作不直接触碰原生对象。

所有span借用自有不可变存储，由owner延长租约寿命；编解码scratch区域互不重叠，亦不覆盖正文。记录不保留会被下一记录复用的临时投影facts；第12步对完整根重新合并检查。joint两段的依赖／贡献输出使用不同区间，不被后段重置覆盖。总保护为65535记录、1024组、8192依赖、8192贡献、4096事件及默认262144次字段／标量访问；64KiB普通消息／32MiB可靠消息仍仅是解码保护，不替代每Peer／三端速率或首次准备1MiB／10秒预算。

## 接受与恢复状态机

`ClientStateAcceptance`只持有纯值描述、至多两份原提交凭据及一个inflight，不持有原生地址。`Assess`返回下表动作；`Begin`再次Inspect完整候选，只给一个不回绕serial；`Finish`核当前route与serial后处理整批结果。原始正文租约由外层owner与这些借用凭据同步持有／退休。

| 条件 | 动作 |
|---|---|
| 旧route／scope；全体target迟到 | Drop或合并补发现存成功ACK，不请求全量恢复 |
| 保留凭据／候选／inflight中同target不同正文或joint关系 | Terminal，不覆盖原目标 |
| target 新且 base 小于本端成功版本 | 普通单流命中合格前代时 Decode，其余旧 base 按 Reack 处理 |
| base无法接续，或joint仅一项仍新 | Resync，不能部分安装 |
| 仅完成固定头及整帧定界，base可接续 | Decode；先确认精确表可用再解typed正文，不是等待原生提交 |
| 缺精确表、资源尚在准备、合法来源尚在等待 | world／joint／可靠恢复Wait，普通单body Drop |
| 精确快照缺闭包、事件缺口、基线丢失、生命周期失效、预算不足 | 具名Resync；不伪造Null／空结果 |
| 非法编码／引用，必需本地资源确定缺失 | Terminal，报告具体原因，不无限重发同一坏目标 |
| 完整结构且完整候选owner返回Ready | Apply；真正执行前再次Begin核验 |

`StateCandidateContext::Inspect`是第12步必需接口，必须核base继承、同根投影／条件／计数组、每个空间的成员／layout合法性、必要结果覆盖、记录与结果不重复、全部实际资源用途／闭包／物化、来源／缓存及窄拓扑。单独调用正文、资源或显示校验器成功不能返回Ready。候选队列、截止时间触发、池记账和主线程owner在后续接线，接口存在不代表这些运行组件已经完成。

Finish的三种结果：

- Success：全部原生操作成功才原子推进版本／world水位、保存原ACK；joint同时推进两项。
- FailedBeforeWrites：必须证明没有原生写入／结构／音频／图形提交副作用，淘汰inflight，保留合法旧目标；不ACK失败目标。
- FailedAfterWrites：可能写入或无法证明无副作用，一律标记受影响事务不可用并合并reason5；跨序列结构共同恢复。禁止用历史A继续差分或补发涉及污染事务的原ACK。没有写入回滚承诺。

恢复中的旧成功版本仅作诊断。受影响consumer先停用，正常排空借用、撤边／清理／退休污染承载，再准备完整新承载；成功前不展示污染对象、不前进事件水位。若一次完整baseline应用失败，它可在正常清理及全量重建后精确重试；不得把它降为相对历史A的增量。旧route的完成回调不发布成功，也不能取代实际owner的清理。

`HostStateAcknowledgements`只核实际发出账本：ACK必须与某一完整发送目标完全相等，joint不拆项；所有字段先核完，再单调推进确认，过期数字不倒退状态。账本槽与正文租约须在发送前预留，满时停止新发布并报告预算／恢复需求；未发队列不能登记。`AcceptStateContextReady`也核实际发送的精确表epoch对，它不更新资源ready。

Host的Recover始终计划base=0，installed字段不授权确认。恢复开始时记住已发target起点，受影响流停止普通增量；恢复开始以前的旧baseline／增量ACK不能解除恢复。重复请求不抬高起点，较旧请求不回退恢复状态；结构恢复只接受共同baseline的确认。完整成功ACK后才恢复日常发布。Host／Client的恢复请求与ACK均须合并限速，不能因网络线程每次收到旧包就立即刷控制消息。

## 与后续步骤及旧入口的边界

第11步把StateAgreement接入握手，把完整正文／Lane接入现有40B信封、64B分片、补片、压缩和实际分片计费；第12步实现完整候选、有限队列／截止时间与纯值上下文；第13步驱动真正主线程Begin／Finish、资源物化及污染重建。旧stream_codec的JSON握手／KnownKind／分派本次没有单独放宽，防止已有运行链接受一半新协议；第14步在完整接通后退役旧Range。当前不启用Kind24、305／306或资源kind5，不新增Presentation ACK，不接触原版Online／Isaac Sync。

第13步[13.3](13-client-apply.md#133-热路径)已同步替代“部分失败回复旧版本后继续增量”的旧规则。来源Gap／UNKNOWN、音乐逻辑包络／seek／重连相位、本地字体与原生owner缺口不因本项完成而消失。

外层遇到Resync动作须调用RequireRecovery并合并请求，Terminal须停用该Peer复制；Host按RecoveringStreams逐序列恢复，不能因另一个独立流的Accepted或重复ACK便解除整局恢复。外层connection必须是新鲜的会话身份，同connection的route单调前进，退休值不复用；这些生命周期约束不由GameFrame推断。

## 实施状态

已完成同源生成和Release／Win32状态库纯编译；首次编译发现固定头Write模板不能从枚举长度别名推导参数，改为按数组长度推导后编译通过。静态复核修订旧base先定界、joint暂存寿命、成员Key唯一与full恢复形态、真实编码访问量累计，以及恢复期间旧ACK／重复Resync处理。不存在测试通过结论：未运行游戏、E2E、单元测试、自测、独立校验脚本或任何试跑；未安装产物到游戏目录。
