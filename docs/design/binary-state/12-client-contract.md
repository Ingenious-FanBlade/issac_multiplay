# 第12步第1项：接口、数据寿命与失败处理

日期：2026-11-11（Asia/Singapore）。上级：[第12步](12-client-decode.md#126-实施范围与工作顺序2026-11-11)。

**第3项实施交接（2026-11-11）**：[影子／候选事务](12-client-shadow.md)已实现ClientStateShadow／ClientStateCandidate、分流不可变根／块、完整结构事务及Begin／Complete／CompleteDrain。完整validator与预留缓存提升均为必须提供的接口，第4／5项实际实现未完成时不能恒Ready／恒成功；主线程与网络仍待第6／7项／第13步接线。产品纯编译／链接成功，无测试，下方第1项设计状态保留原时点。

**第2项实施交接（2026-11-11）**：[成员表与资源表管理](12-client-tables.md)已实现ClientStateMemory／ClientStateTables及const快照／准备票据接口，状态库编译完成，实际全域注册／网络／候选／主线程接线继续后续。下方第1项状态与新名称说明保留其定稿时点，不将尚未落地的其他批次／worker接口写成已有实现。

状态：**接口与实现合同已确定，运行owner尚未实现**。本项按当前版本66的已有源码做静态分析，明确第12步后续实现及第13步消费边界；只修改文档，不修改布局源、生成器或产品代码，不生成、不构建、不运行任何测试。没有原生执行、网络联通、并发安全或性能通过结论；第15步是唯一统一整体验收。

第1项定稿时，本文中的`ClientState*`新名称均为待实现的内部接口／数据类型设计；后续实际类型与实现状态以第2项等交接记录为准。已有接口以源码链接标明。后续文件划分可随实现调整，但所有权、提交顺序及失败语义按本文落实；不新增网络消息、确认序列或布局兼容规则，本项不递增布局版本。

## 1. 输入与输出边界

输入为外层已授权连接／route下、已完整重组并按协商解压的**自有正文**，包含消息kind、lane、可信route、接收顺序和本地单调时间。连接／route来自本地会话及外层封套，不能从正文声称的scope建立权限。普通消息／可靠消息分别遵守现行大小与整帧预算；重组／解压成功不授予应用资格。

本步输出三类纯值消息：给主线程的资源／结构准备请求与应用批次、给网络线程的ContextReady／AppliedAck／Resync／终止原因、给后台自身的有界影子与生命周期记录。原生Game／Player／Grid／Lua／GPU／音频对象不进入后台；第13步在主线程管理实际承载与资源，反馈纯值准备状态和实际完成结果。

主线程不解析网络正文、不建立JSON／字符串目标模型，也不在取批次时等待解码线程的锁。已生成数字字段与变长只读视图由后台构建；原生写入、显示消费及实际退休按第13步接线。旧`ClientScene::AdmitStream`／`AdmitTarget`继续代表旧路径，第14步统一退役，不把新正文转成旧Range或JSON作为过渡桥。

## 2. 当前源码事实与复用位置

以下是静态代码事实，不是运行观察：

| 已有组件 | 当前能力 | 本步需要补齐的责任 |
|---|---|---|
| [`wire_protocol.h/.cpp`](../../../mod/src/state/wire_protocol.h)：InspectStateFrame／DecodeStateFrame、成员表Merge | 固定头定界、typed正文校验、不可变表合并视图 | 持有正文与输出区；精确快照管理；先按旧base／重复规则处理，再进行typed解码 |
| [`body_codec.h`](../../../mod/src/state/body_codec.h)：BodyView／BodyNodeObserver／ValidateProjectionFacts | 块、根字段及投影事实视图；递归数字路径；局部值校验 | 同根候选的base继承、全部投影／引用／拓扑及实际条件完整性 |
| [`resource_protocol.h`](../../../mod/src/state/resource_protocol.h)：ResourceDirectoryView／ResourceGraphPlan／ResourceReadyContext | typed元数据、不可变目录合并、数字依赖图、目标闭包 | 持有旧payload及图计划；管理准确epoch和本地准备反馈；区分Pending与确定失败 |
| [`presentation_envelope.h`](../../../mod/src/state/presentation_envelope.h)：PresentationEnvelopeContext | 显式组／贡献／事件校验、缓存与来源查询接口 | 全部必要组覆盖、合法继承、已提交缓存及未确认事件的实际owner |
| [`acceptance.h/.cpp`](../../../mod/src/state/acceptance.h)：ClientStateAcceptance | Assess／Begin／Finish、历史安装版本与原ACK、污染恢复规则 | 完整StateCandidateContext；正文租约；串行调度；实际原生busy与反馈寿命 |
| [`budget.h/.cpp`](../../../mod/src/state/budget.h)：StatePendingCandidates／StateTransferEpisode | 两等待槽、替换不续期、恢复episode及同源预算 | 候选字节及计时owner、共享池／Peer配额、到期动作及实际释放 |
| [`replication_memory.h`](../../../mod/src/core/replication_memory.h) | 进程三池、按owner计费的Charge／Lease，最后释放归还 | 所有正文、索引、快照、候选、描述及工作区的实际capacity记账；不另建脱离总账的池 |
| [`client_scene.h`](../../../mod/src/game/client/scene/client_scene.h)、[`client_objects.h`](../../../mod/src/game/client/entity/client_objects.h) | 已有主线程借用／退休及A→T语义；公开输入仍是旧模型 | 第12步第7项／第13步接入二进制批次及完成反馈；不能将旧域局部完成直接映射为整批Success |
| [`client_grid_description.h`](../../../mod/src/game/client/grid/client_grid_description.h)、[`client_room_description.h`](../../../mod/src/game/client/room/client_room_description.h) | 完整影子根生成自有不可变描述／投影 | 向生成函数提供完整且已递归校验的根，计入新增描述副本的内存；返回描述仍需正常资源及原生准备 |

`state_client.inc`目前生成的是Validator／ClientField／CountConstraint等数字表，`FindClientField`返回分派元数据；它不是设计示例中已经存在的`EntityE1::Velocity`访问器。后续视图通过数字layout／field索引和已解析片段读取，未对齐标量用memcpy，变长字段不假定固定偏移。具名域视图沿生成计划扩展，不添加第二份字段白名单。

## 3. 线程与唯一修改者

| 执行位置 | 可以修改的状态 | 跨边界交接内容 |
|---|---|---|
| 网络线程 | 已授权传输、重组队列、实际包计费及发送 | 自有正文的所有权／有界句柄；可信route／lane／kind／接收顺序；后台形成的纯值控制消息 |
| Client后台owner | 成员／资源快照、生命周期历史、候选、影子、投影／结果／事件缓存、ClientStateAcceptance、恢复episode | 不可变准备／应用描述，批次句柄和票据；接收主线程纯值完成消息 |
| 游戏主线程及其正常资源窗口 | 原生承载、图形／音频物化、native借用／写入／展示／退休 | 带完整票据和本地代次的准备／应用／排空结果；不返回原生指针 |

`ClientStateAcceptance`的Open／Assess／Begin／Finish／RequireRecovery／Retire全部由同一后台owner串行调用。主线程不直接调用此状态机；现有头文件所说“按主线程完成消息串行驱动”据此落地。资源准备状态也由后台根据主线程消息更新，校验回调只读取该owner已有纯值，不主动查询游戏线程。

网络→后台与主线程→后台使用各自有界通道，禁止让两个生产者并发写同一SPSC队列；需要汇合时由后台显式串行消费。后台→主线程的准备、执行、取消／退休按单一有序命令流处理。Win32发布状态／队列索引只用保证无锁的u32原子；route、版本、serial等u64值放在发布后不变的描述中，不用atomic<u64>或atomic<shared_ptr>承担无等待承诺。

执行与完成通道在发布命令前预留返回位置。准备／应用／排空完成不得按latest-only覆盖；槽满时暂停新发布，处理既有反馈，或撤销复制资格并安全退休，不能丢掉完成消息后猜测“没有写入”。生命周期撤销有独立的可观察取消状态，即使数据队列满仍可撤销成功资格；它不覆盖消费者已取得的内存。

## 4. 数据模型与接口合同

### 4.1 后续实现的数据对象

| 待实现对象 | 必须包含 | 所有权与有效期 |
|---|---|---|
| ClientStateMessage | 可信StateRoute、kind／lane、本地接收顺序／时间、不可变正文和实际容量租约 | 接纳后转交后台；没有指向socket接收栈的span。StateTarget的encoded对应解压后的精确状态正文，并非zstd压缩输入 |
| ClientStateTables | 精确成员／资源epoch、不可变目录及原update、数字资源图、成员转移历史的版本化查询 | 由候选／两流安装根／在途批次显式持有；资源payload可共享不可变内容，但目录与原update数组独立稳定 |
| ClientStateShadowRoot | 完整成员身份space／id／use／shape／layout、所属流、每块字节及最后变化版本、解析后的数字视图／引用及原表身份 | 候选私有构建，封存后只读；只对同route／身份／形态和相容依赖的成功安装根继承。块最后变化版本不伪改为目标版本 |
| ClientStateCandidate | 原始StateTarget及正文、精确表、完整目标根、结果／事件引用、就绪状态／具名诊断、首次等待时间 | 候选只代表目标，不能授权下一条增量以它为base。joint两流共享一个候选和整批凭据 |
| ClientStatePreparation | route、候选句柄／本地代次、准备serial、目标／表身份、所需资源闭包／结构描述及取消代次 | 主线程只准备候选资源／临时承载，不提前改变可见世界；准备失败按真实副作用报告，不充当AppliedAck |
| ClientStateApplyBatch | StateApplyTicket、完整目标与结构操作、各域数字视图／描述、必要结果／有序事件、精确表、准备凭据及取消代次 | 一个实际在途批次；包括已从邮箱取出但尚未返回完成／排空的阶段。所有引用随批次固定，不能切换为后台“最新表” |
| ClientStatePrepared／Completed／Drained | 原票据或准备serial、本地槽代次、原生／资源代次、结果与受影响流、具名原因 | 完成消息是时点事实，不随场景刷新；旧route回报只参与旧借用排空，不更新新会话状态 |
| ClientStateControl | 原ContextReady／AppliedAck／Resync纯值或终止原因及控制发送凭据 | 后台产生，网络线程按当前route实际发送；控制反馈／重发按现行50／250ms合并，不触碰原生状态 |

成员／资源序号仅在各自精确快照下解释；identity、source_revision、presentation_batch、game_frame、Peer target和本地取消／准备代次分别保存，不互相推导。缺席、合法Null、合法空值、未就绪、读取／来源失败分开表达。

### 4.2 外部操作顺序

下表定义语义接口，尚不声明C++签名；实现时按值或move交接上述对象，不允许只传span却隐含外部存活要求。

| 设计接口 | 调用位置与输入 | 返回及失败后的状态 |
|---|---|---|
| Open | 后台；新鲜route、经CompatibleStateAgreement核相容的对端声明及从本地配置取得的额度 | 初始化空安装根与共同baseline需求；同connection的route不回绕／复用；实际旧借用未排空时不得复用其槽／native承载 |
| Admit | 后台；ClientStateMessage所有权 | 分派Members／Resources／Frame。先核授权route与保护额度；Frame先Inspect／Assess，再按精确表解typed正文。返回Drop／Reack／Wait／准备／执行／Resync／Terminal和具名诊断 |
| Pump | 后台；单调时间、已接纳完成反馈 | 重查依赖、过期、陈旧与恢复、合并控制、回收无引用存储；任意包到达不自动记为恢复进展 |
| TryTakePreparation／TryTakeApply | 主线程正常窗口；非阻塞读取 | 取得只读票据及临时借用；没有命令立即返回。主线程按票据核当前route／取消／native资源代次，执行前后重新检查 |
| CompletePreparation | 主线程→后台；原准备票据与Pending／Ready／确定失败 | 后台登记本候选／精确资源身份的准备事实；旧反馈不使新epoch或新候选Ready。Pending必须保持合法、有界的进展记录 |
| CompleteApply | 主线程→后台；原StateApplyTicket及Success／FailedBeforeWrites／FailedAfterWrites、affected_streams | 后台核票据与取消资格后串行Finish；只有完整成功才提升影子根／结果缓存和安装版本，产生原始整批ACK。无资格的Success按实际可能写入处理清理，不能强转成未写入失败 |
| TryTakeRetirement／CompleteRetirement | 主线程正常退休窗口及反馈 | 停止相应consumer，正常排空callback／draw／CPU／GPU借用、撤边并退休；完成前不复用原生承载或覆盖其纯值存储 |
| PollControl／ControlSent | 网络线程与后台间的有界控制交接 | 按完整发送凭据确认发送，不按一个bool清空后来生成的ACK／Resync；发送失败／WouldBlock保留必要消息和原合并期限 |
| Close／Drained | 后台撤销route资格；各线程报告排空 | 先禁止新发布／ACK、使旧票据失效，再等待实际借用返回，最后释放存储。Close不是同步析构或native回滚；排空前旧状态保持有界存活 |

同一数值target的正文／base／joint关系固定。重复包可复用原租约和原ACK；对所有仍保留的候选、实际在途及成功凭据比较完整内容，冲突不覆盖。过期且已释放的任意历史正文不无限归档；迟到包按已安装版本与恢复规则处理，不伪造跨全部历史的逐字节认证能力。

ContextReady仅在本端已合法接纳相应member_epoch／resource_epoch元数据快照后生成，不表示资源已物化、候选完整或原生已安装。AppliedAck只由成功完成处理产生，Resync保留具名原因和历史成功版本；三个控制消息的资格不能互相替代。

### 4.3 纯值校验接口接线

- `StateFrameCodecContext::Member`只查本消息精确member_epoch；`BodyContext`从同候选根及resource_epoch提供配置／共享presence。先建立根索引及incoming覆盖规则，必要时分遍解析；尾投影缺少首presence时只能读同候选或合法安装根，不能猜false，也不能先用旧presence接受后再覆盖。
- `StateCandidateContext::Inspect`只针对已完整构建并封存的候选核实际根、所有必要结果、引用、来源、资源与窄拓扑。默认未实现／缺域检查不能返回Ready。结构解码、准备完成、完整候选和实际安装分别记录。
- `PresentationEnvelopeContext`查询同route的已提交缓存及独立保留的已验证事件；必要组覆盖由当前域／条件／owner合同确定，不能只检查报文自己声称存在的组。来源资格不能由payload相等或本地调用顺序补造。
- `ResourceReadyContext::Materialized`从精确准备反馈读取bool；另有具名的Pending／MissingLocalResource／后端失败状态。false本身不足以区分等待和确定失败。校验只遍历实际资源根；当目标没有资源根时直接完成空闭包检查，不能把空roots传给当前“空表示全目录”的PrepareResourceClosure而扩大body等待。
- 已有`Gap`以字符串携带原因，不把`what()`文本匹配作为新的调度协议。后续owner／上下文须在失败点保存类型化状态及数字定位；资源／来源布尔回调先记录具名原因，再由外层统一分类。缺乏明确分类的异常不默认为Wait或成功。

## 5. 借用寿命审阅与封存规则

| 现有视图 | 实际借用 | 使用／保存规则 |
|---|---|---|
| BodyView.bytes／blocks | 当前输入正文；编码时为输出区域 | 只要引用仍存在就固定正文地址／容量；封存块可复制到Values并重新建span，不能复制指针后释放原正文 |
| BodyView.fields／facts | BodyCodecScratch.root_fields／facts | 下一次Decode或校验复用该区域就失效；不保存跨记录scratch视图。完整根按继承后的块重新收集并检查投影事实，临时facts无需永久逐节点保存 |
| BodyNodeView.path及回调location | BodyMachine当前递归栈／当前调用 | 仅在回调内使用；需要记录时立即复制有界数字路径和结果片段位置，不保存栈span |
| StateFrameView.sections.records | StateFrameScratch.records | inspect／decode后必须封存元数据或复制到候选自己的区域，不能在下一消息覆盖时仍当旧候选使用 |
| ResultGroupView及PresentationAudioEvent | groups／events数组来自帧scratch；payload来自正文；dependencies／contributions来自外层scratch | 封存所有会逃逸的元数据及相应正文；只保住payload不足以保住组依赖和有序贡献。joint两段独立区间都随候选持有 |
| StateMemberDirectory.entries／update | 新merge输出数组／原incoming.entries数组 | 冻结当前快照的完整目录和原update；update不能指向下一次成员解码复用区。转移历史另存纯值，不借旧scratch |
| ResourceDirectoryView.entries／update及其payload | 新目录数组／原更新数组，payload还可能来自更早消息 | 新快照拥有稳定数组并固定每个内容片段；共享内容按一次实际分配计费。不以整个旧目录链存活代替必要内容pin，避免无限保留旧epoch数组 |
| ResourceGraphPlan | 精确目录entries和构建时edges／offsets | 随该资源快照保存；重建图不能覆盖旧计划。PrepareResourceClosure的state／heights／order属于工作区，返回的materialization_order须在复用前消费或复制 |
| ResourceBodyContext | 按值保存目录视图，但不拥有其内容；shared_仍是借用指针 | context及其底层快照／共享投影查询一起存活于调用；不能把栈context指针保存在候选或跨线程批次中 |
| StateTarget.encoded、inflight_、committed_与StatePendingCandidates | 只借用精确正文；复制StateTarget不会延寿 | 每个保留凭据绑定相同正文租约；Remove／Expire／RequireRecovery／Retire仅释放它实际解除的描述。RequireRecovery仍保留committed历史凭据；实际主线程借用也另行固定，最后一个使用者退出才释放正文 |
| CommittedResult／KnownEvent返回值 | 缓存payload／contributions或事件payload | 查询期间和后续使用固定其真正owner及原epoch；数值返回结构不构成独立所有权。已提交缓存不能从未安装候选提升 |
| Grid／Room／Level影子输入与返回描述 | 输入块仅借用到ClientDescribe*返回；返回描述／projection共享自有存储 | 后台调用生成接口并计费描述副本；主线程只持返回的不可变描述／视图，不能保存输入栈数组或无owner的Bytes指针 |

封存采用“候选可变构建区→不可变发布”一次转换。发布后不reserve／resize／排序其底层vector；需要更新时新建候选区。跨线程用带槽号及不复用本地代次的句柄固定存储；后台保留真正owner到完成／排空反馈，主线程只读借用。具体C++实现可以在后台使用shared_ptr共享不可变块，但不能用原子shared_ptr交换或让主线程成为最后一个大缓冲／Charge的释放者。

正文、快照、块内容、结果／事件、描述可共享已封存片段，所有引用都计入可达性；不能为了释放原始消息而损坏仍需精确比较的StateTarget。对比用原正文至少保留到相应pending／inflight／committed凭据退休。正文去重只节省一次实际存储，不减少多Peer的网络计费。

## 6. 候选与实际提交顺序

状态顺序为：**自有消息→定界／版本决策→精确表与完整根构建→资源／来源准备→完整候选Ready→后台Begin→主线程执行→后台处理完成→已安装**。缺表或Pending按当前候选规则等待／丢弃；每次重查都保持原等待／episode起点。

1. 新候选只从同流成功安装根继承。未变化块保留原字节／最后变化版本、编码时表身份和已解析引用；表变更后须核同一字节在目标表下仍表示同一合法身份／内容，无法证明时要求显式替换或完整恢复，不能直接用新序号解释旧字节。加入／换形以及跨world／body结构变更必须构成完整合法目标，不能以旧shape块补齐。
2. 全部必要准备完成后，后台保留目标／表／资源／结果与完成槽，调用`Begin`再次Inspect，再发布唯一应用票据。主线程真正进入原生操作前还要核取消代次、准备凭据及当前native资源代次；申请后失效不能凭Begin曾成功继续写入。
3. 主线程执行中出现任意写入／结构／图形／音频可见副作用后，失败不能再报告FailedBeforeWrites。候选资源的临时构造只有在与旧可见目标隔离、可证明正常清理时才属于未提交准备；不能先绑定世界再将失败称为“只是准备”。
4. 整批成功先在主线程完成所有承诺的原生提交，再返回原票据；后台核当前route／serial／取消资格，串行Finish并提升对应影子／结果缓存，最后发布ACK。影子提升所需的索引／元数据在写入前已预留；成功反馈阶段不得再临时分配，避免native成功但影子无法提交。
5. Success的joint保持一次共同提交、一次双项ACK。FailedBeforeWrites保留仍合法旧安装根；FailedAfterWrites撤销受影响流的安装资格，历史版本只作诊断，禁止相关增量／旧ACK／污染展示，等待正常清理与base=0完整重建。
6. **逻辑inflight与实际native busy分开**。RequireRecovery／Retire会清除接受状态机的inflight，但已经交给主线程的票据及物理借用仍可能存在。实际busy及其内存保留到对应Completed／Drained；之前不发布第二个原生批次。超时撤资格不强杀writer，不把inflight为空当作旧承载已干净。
7. 换route、资源／实例代次失效或关闭时先发布撤销并停相关consumer。旧完成不能ACK新会话；若旧操作实际有写入，主线程仍须正常收尾／清理。新route只有在合法排空、获得干净承载及足够租约后才能应用，旧槽代次耗尽时退休，不回绕复用。
8. 已提交描述／结果可能仍被draw／CPU队列消费，完成消息不自动表示这些旧引用全部排空。主线程通过现有借用／退休owner报告具体释放；后台最后释放大缓冲及租约，避免主线程因析构进入共享池mutex或大块释放。

成员退休／身份回收历史按真实session／run保留到相应生命周期结束，不能为普通Resync或换route清空后复活旧身份。资源内容目录可按第9步规则在同session／run复用，仍不得回收编号／改绑内容；专用epoch重编号只通过合法排空后的RebaseStateResourceEpoch。Close释放route候选／缓存的资格与借用，不自动清空这些跨route历史；新的结果／事件资格通过完整目标重新建立。

## 7. 容量、内存与计时

所有数值读取[第9步现行预算](9-budget-capacity.md)及`StateBudgets()`，本项不建立第二份常量源：

- 普通world最多一个等待目标、100ms；可靠／joint共用一个等待目标，ordinary joint500ms，baseline最多原episode10秒；普通单body未Ready即丢，不设等待队列。资源准备可作为独立有界工作继续，不能以此暗留该body候选或先ACK。
- 一次最多一个实际原生应用批次；准备／完成／退休邮箱按单命令预留返回槽，不增加隐藏待执行批次。替换等待候选不释放仍由主线程借用的存储，不把在途批次计入可覆盖槽。
- 每类表最多5个不同的实际引用快照，当前目录、正在构建的新快照、两流安装根、候选、在途、结果／事件及退休借用都参与去重计数；“两流＋inflight＋两候选”不是额外给其他owner无限快照的许可。超过容量先回收无引用快照、按规则淘汰可淘汰候选或恢复；不能覆盖已借用旧表、丢必要事件或让未提交目录假装已安装。
- 总活跃结果≤1024组；未确认事件≤4096条、完整事件头＋正文≤256KiB且最老不足500ms。候选替换先保存已验证且水位未提交的事件并检查同号冲突；事件的精确表／正文寿命独立于被替换候选。历史事件超额请求当前voice／水位完整恢复，不静默跳水位或重播旧one-shot。
- Values／Wire／Workspace共享进程512／128／128MiB；每Peer实际Values≤128MiB、Wire≤24MiB；同进程大型工作最多一个、实际保留≤96MiB。状态正文及用于重复比较的原消息属于Wire，规范块／目录／结果／索引／描述属于Values，短期codec／图遍历／压缩工作区属于Workspace；同一分配不重复归类。
- 所有数组capacity、稳定槽／索引、共享控制块和副本计实际成本。先预留额度再分配，失败立即回滚未发布暂存；不能只按size记账。`Charge::TransferFrom`仅转移同池真实分配的owner计费，不用于跨池重命名或复制内容；复制时在新分配期间同时记两份，重解释归类也不能绕过新池额度。
- Workspace租约可在后台有界等待，继续计算候选／episode年龄；主线程不等待该锁。归还工作区前将所有逃逸的结果复制／转交到持久区域。32MiB可靠解码保护不表示可以突破24MiB Peer Wire或初建1MiB额度，不预占每个字段／容器所有最大值。
- 恢复原起点包括来源／传输／准备；8秒传输、10秒整目标、2秒真实无进展、同episode最多两次全量尝试及双向1MiB不因换候选／重复包刷新。当前不可自行恢复的本地资源／线程／后端故障不通过循环Begin恢复掩盖。

## 8. 失败分类与恢复动作

每个失败保留有界纯值诊断：原route／scope、kind／lane、处理阶段、涉及流／目标／表epoch、成员layout／block／field及最多32层数字路径、类型化原因、实际请求／剩余额度或到期原因。旧首因与当前清理状态分别保存；不为每帧创建报告／artifact，也不依赖日志写成功才停用错误状态。

| 失败／状态 | 动作 | 仍可保留的目标及后续处理 |
|---|---|---|
| 旧route／scope、迟到target | Drop；必要时合并补最近成功的原始ACK | 不解typed正文、不推进版本；不创建新恢复episode |
| 完全相同的已提交目标、旧base | Reack，使用原提交凭据，joint不拆项 | 不重播事件、不重新写世界；受污染流不补旧ACK |
| 同target改base／正文／joint；同epoch表内容冲突 | Terminal | 停该Peer复制，按真实借用状态退休；不覆盖原数据 |
| 固定头／mask／长度／枚举／条件非法、未知或错型引用、依赖环 | Terminal；InvalidEncoding或InvalidReference | 当前候选全部作废，未写入时保留仍合法旧画面；不能跳过坏字段后继续 |
| 缺精确表 | MissingTable：有界Wait；普通单body Drop | 不借最新表猜读；表链确定断裂或旧快照已无法取得时Resync(Table) |
| 精确快照存在但目标闭包缺声明／必需组／实际投影 | Resync，按Table／Dependency及具名诊断 | 不以未来epoch补旧目标；未实现检查／缺必要域不得Ready |
| 本地资源／合法来源仍在准备 | ResourcePending／SourcePending：有界Wait；普通单body Drop | 只保留仍合法旧目标；超时按实际依赖恢复，重复回报不续命 |
| 必需本地文件确定缺失、未知owner无法实现、后端确定不可准备 | Terminal；MissingLocalResource或具名本地失败 | 不无限Resync同一内容，不改Null；不能以可用bool=false掩盖确定失败 |
| base无法接续、完整根继承丢失、需结构重建 | Resync(Base／Lifecycle)，结构有关时共同恢复 | 不用未提交候选、旧shape或受污染历史A继承 |
| 事件缺口／保留超额／超龄 | Resync(Event／Budget)，保留必要事件至完整恢复或安全退休 | 不随latest替换删除，不重用sequence／voice代次；恢复不补播历史one-shot |
| 本地池／Peer额度／快照／scratch容量不足 | Budget；释放未发布暂存，合并Resync或停止无法完成的episode | 保留仍合法安装根；不截断对象／字段，不把本地耗尽伪装成非法报文 |
| 报文本身超过协议硬上限 | Terminal，InvalidEncoding | 在分配／递归前有界拒绝；与合法消息遇到本地容量不足区分 |
| 候选／episode超时、单调时钟倒退 | 对应依赖恢复；episode耗尽／本地时钟故障明确终止 | 不增加期限，不强杀主线程，停止相关成功资格 |
| FailedBeforeWrites且无其他受影响流 | 不ACK失败目标，保留仍合法安装根 | 按具体原因等待／重建；不自动升级为Success或污染所有流 |
| FailedAfterWrites、无法证明无副作用、失效期间返回Success | 撤成功资格，停止受影响消费；reason Apply，完整恢复／清理 | 历史版本仅诊断；实际native busy及租约直到排空，跨序列结构共同恢复 |
| 错误线程、票据内部矛盾、完成／控制丢失、无法安全清理 | 具名本地终止 | 撤销复制资格，正常排空；不得猜测恢复、重注入或强制卸载 |
| 旧route／serial／槽代次的准备或完成反馈 | 不提升目标／版本／资源就绪，不ACK | 仅凭仍登记的原票据处理其实际释放／清理；不忽略旧native副作用 |

`StateReadiness`／`StateAcceptAction`保留第9步现有语义；本地线程、后端及取消诊断不伪装成新的线上错误枚举。Assess返回Resync后，owner必须实际调用RequireRecovery并合并请求；Terminal实际停止复制。开始前已有安装资格但本地代次后来失效时，保留旧画面必须重新证明仍合法，不能只因shared_ptr尚在就继续展示。

## 9. 后续实现前已列明的失败方式

下列是实现约束与第15步覆盖输入，本项不执行任何实验／故障注入：

1. socket／解压输出被复用，候选或StateTarget仍指向旧正文；只复制view未复制／固定其内容。
2. 下一record／validator复用facts／root_fields；BodyNodeView栈路径逃逸；joint后段覆盖前段依赖／贡献。
3. 表快照只固定entries而遗漏update／旧payload；新资源图覆盖旧edges；空资源根误检查整目录。
4. 新epoch重解释旧引用；成员换形／合法序号回收后旧根／结果复活；来源缺失被填false／空值。
5. 增量以未提交候选作base；多个合法record拼成不完整根；必要结果覆盖仅按报文自报组检查。
6. 主线程与后台并发修改接受状态机；错误线程查询原生对象；u64／atomic shared_ptr暗中引入等待。
7. 发布前未预留完成槽，完成被覆盖／丢失；清控制时误删更新的ACK；取消遇满队列失效。
8. Begin后代次改变仍执行；RequireRecovery清inflight后立即启动第二批；超时提前释放实际writer／draw仍借用的数据。
9. native成功后影子提升才分配而失败；局部域完成冒充整批成功；joint只确认一半；旧票据ACK新route。
10. 可能已写入却报FailedBeforeWrites，污染后仍按历史A差分／展示／重发ACK；撤资格误当原生已回滚。
11. latest替换连带删除未确认事件；同号冲突或重复one-shot；事件／结果引用导致超过5个快照而未计费。
12. 按size漏记capacity或共享控制块；复制内容沿用旧租约；跨池转账绕过额度；主线程最后析构进入共享池mutex。
13. 以32MiB保护上限预占每Peer最大空间；工作区租约逃逸；准备／恢复重启刷新期限；确定缺本地资源无限重试。
14. Close直接析构未排空场景／队列；只保留物理内存却继续消费失效世界；清理故障被覆盖为最终成功。

## 10. 本项交付与下一项输入

本项完成：源码与现行协议边界核对；内部数据对象及语义接口、唯一后台修改者、所有借用区域及封存／释放顺序、主线程准备／应用／排空反馈、逻辑inflight与实际busy分离、预算归属、失败分类和后续实现失败方式。主文档中旧访问器示例及过时输入路径同步更正。

下一项按本合同实现精确成员／资源快照、其正文／元数据所有权和生命周期历史，先让每份View都能找到真实稳定owner；随后再实现影子候选／各域检查、准备缓存、调度与主线程交接。真实Host来源由第10／11步提供，不作为本项静态设计的前置条件；现有游戏事实与UNKNOWN不因本文定稿而消失。

本项没有新增可运行接口、布局或版本变化，也没有任何构建／测试通过结论。
