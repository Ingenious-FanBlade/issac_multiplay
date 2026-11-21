# 第12步第3项：影子状态与候选事务

**2026-11-15 用户采纳，版本79**：普通Body新增一个可替换等待位及一个成功前代解码基线，完整重建后相对当前状态应用；实际应用基线仍锁定。详见[第13步10.4](13-client-incremental-apply.md#104-普通body单槽等待与前代解码基线2026-11-15用户采纳)，覆盖本文冲突的零等待／旧base必拒绝描述。其他完整检查、真实成功ACK与失败恢复原则保持，同一步实机续验尚未通过。

**2026-11-15 后续设计，尚未实施**：[增量应用设计](13-client-incremental-apply.md)在现有未变块共享的基础上，保留稳定承载，增加可复用解码／结构事实和整体发布的块引用表；普通两流各自维护在途任务，接受回执只更新对应流并保留另一流最新引用。旧表／块由读者、计划和原生借用共同决定退休。下方单候选事务描述保留其实现时点，本次仅文档，无构建或测试。

日期：2026-11-11。上级：[第12步](12-client-decode.md)、[接口合同](12-client-contract.md)、[表管理](12-client-tables.md)。状态：**第3项纯值影子与候选事务已实现，完整应用资格与正式运行接线仍待第4—7项／第13步**。先登记以下失败方式，再实现代码；第15步唯一统一验收，本项未运行测试。当前布局版本67，本项不改布局源或版本。

## 实现前列出的失败方式

- 候选继承另一未提交候选，而非准确成功安装base；baseline借旧根补缺块。
- member序号相同就复用旧use／shape／layout，新增／换形后只到一部分块便构造完整对象。
- reserved空块被要求必收；合法条件缺席被造为零；本序列夹入另一序列的块。
- 新epoch重解释旧块的成员／资源序号；省略的记录被误当离开，或忽略准确成员表中的真实退休。
- BodyView字段／facts及帧依赖／贡献仍指向临时scratch；原始正文释放后StateTarget或结果／事件失效。
- immutable根通过父候选链保存旧块，导致无限历史／旧表存活；新目标版本覆盖未变化块的真实变化版本。
- 同根计数组／shared presence只验证delta，漏掉合法base投影；跨world/body只核一半，或者等待无关world内容。
- 解码工作区先按所有最大facts预占；临时事实不足被误判为非法报文；重复解码副作用污染已发布状态。
- 结构完整被当成Ready，未接域／资源／显示检查便发ACK；已接受版本被当成功安装版本。
- joint只提升一个根／ACK一项；原生成功后影子提交才分配，失败留下不同版本。
- 部分写入后仍用历史A增量；逻辑inflight撤销就释放实际writer／draw仍借用的数据或启动第二批。
- 旧route／serial完成更新新会话，重复完成重播事件；取消期间Success被误当作无副作用失败。
- 候选／根／正文最后释放落在主线程，或者Close时清空尚未排空的借用；超限截断数据。

## 实现范围

新增[`client_shadow.h/.cpp`](../../../../mod/src/state/client_shadow.h)，编入isaac_cs_state；它不依赖游戏／SDK／Lua，不创建线程、不发送控制消息、不操作原生对象。所有修改固定在显式指定的后台线程，内存与第2项表管理共用同一ClientStateMemory账户及进程复制三池。

| 实际接口／类型 | 本项已实现内容 |
|---|---|
| ClientStateShadow::Build | 自有规范正文、先定界／接受旧base、准确两表、完整记录合并、计数组／presence闭包、正式Frame typed解码及候选封存 |
| ClientShadowState／ClientShadowRecordView | 分流完整目标、完整身份、每块最后变化版本／原来源、数字字段及投影事实视图 |
| ClientStateCandidate | 自有Frame及组／贡献／事件元数据、两条可选目标根、Join／Reshape／Leave／Remap结构意图 |
| Assess／Begin与ClientShadowValidation | 再核成功base及当前窄投影，必须接入完整第4／5项检查，没有默认Ready实现 |
| Complete与ClientShadowCommit | 主线程真实结果驱动提交；预留的结果／事件缓存同步提升，没有默认成功实现；全部成功后才返回原始整批ACK |
| RequireRecovery／CompleteDrain／Close／Collect | 受影响安装资格撤销、实际busy与清理排空分离、原ACK正文保留、后台最后释放及旧route隔离 |

本地`ClientShadowLayoutBinding`冻结space／layout及正式元数据锚点模式；真实块及所属序列来自生成的Blocks，record预算来自LayoutBudgets。MetadataOnly只用于产品正式登记的纯身份owner，不接受报文要求少收字段；未注册组合拒绝。各域实际注册与语义核定归第4／6项，不把本层注册接口写成已经支持所有游戏域。

## 构建与继承

Build接收当前调用内稳定的正文span，先复制到扣Wire的不可变正文区，按授权lane收紧消息保护额度；baseline／joint不能由超预算普通帧自动升级。InspectStateFrame先完成scope／版本／段界校验，再驱动ClientStateAcceptance：旧base／迟到帧按原ACK规则处理，不先按当前事件水位解码旧事件。

精确表未到时返回有界等待／普通body丢弃所需的前置决策；本地已经越过却未保留的旧表视为缺失闭包，要求Table恢复，不借最新表猜读。实际等待槽、原正文租约、期限和控制发送仍由第6项调度。BuildResult.decision保留的是前置决策，candidate非空才表示已封存结构；它不是完整Ready，调用方仍须Assess／Begin。

每条序列只继承其**成功安装且仍可用**的准确base。baseline不借同流旧根；新增／use变化／shape变化／layout变化不能借旧身份填块。按完整身份查找base，不把member序号当对象身份；新成员及换形必须收齐本序列所有实际非reserved块，条件失效分支仍由正式codec精确消费，保留present=false与合法空值的区别。

成员退出由准确成员表决定，delta未带某条record不等于删除。world或joint构建结构意图，body不擅自执行另一流的结构变化；需要两流的成员在另一流缺失／换形时要求相容共同目标，不能保留半个身体。记录按数字索引存放不改变payload中的真实roster／容器顺序。

变化块单独复制一次到Values，记录本次target、game_frame、source_revision和原编码表epoch；继承块保留原值与最后变化信息。新record共享独立字节owner，必要时重新创建链接元数据，不引用父record／父candidate链。未变化record在同两表及presence资格稳定时直接共享，避免每帧重复解码全部静止世界。

所有实际非Null成员／资源链接解析到精确表；旧块跨epoch继承时比较编码值及原已解析身份／资源kind与精确payload。不能证明含义相同则拒绝继承，要求明确替换／恢复；通过后只固定新绑定快照，保留原编码epoch作来源事实。无链接的旧块无需持有旧表。

## 投影完整性与数字视图

incoming与继承块先组成同序列完整根，复用DecodeBody重新检查实际条件／choice、精确长度、值域及引用，封存必要计数组／shared presence事实。投影facts仅含真实数字路径和数量／存在性，不生成递归字段树。字段片段重新绑定到各自不可变块，绝不指向临时拼接区、facts数组或observer栈路径。

根类型的跨流shared-owner首投影顺序由本地数字计划确定，不猜缺失presence。只在同一完整成员身份上联合核world／body投影及完整记录总字节；required mask与reserved按生成表执行，不能只检查delta本次携带的投影。

候选等待期间，另一流可以继续前进；Assess／Begin重新取成功安装的对应成员，只检查必要投影及成员结构关系，不要求整个world版本静止。普通body与当前world不相容时给SourcePending，供接受状态机丢弃；world／joint按实际缺口恢复。P0—P6的具名绘制修订、组／身体、Room／Grid等更细语义仍归第4项，资源物化、全部必要显示结果及来源归第5项。

## 正文、缓存与寿命

正式DecodeStateFrame在完整根上下文中再次核整帧typed正文。成功后按真实数量封存records、组、事件、依赖及有序贡献数组；它们的payload借用自有正文，所有元数据都有稳定owner，joint后段不会覆盖前段。StateTarget.encoded固定此规范正文，用完整字节比较仍保留目标的冲突，不计算哈希。

原ACK凭据只固定原规范正文及帧元数据，不为保住一次joint ACK而固定整条旧影子根。candidate／state／packet各自通过后台registry保住最后引用；Collect按此顺序回收无外部引用的对象，Installed返回的只读StateRef也受保护。主线程保存RecordView／字段span时必须持有相应StateRef或CandidateRef，不能仅保存裸视图。

两表、已安装根、候选、实际inflight及后续draw借用的真实容量都计共享账户，registry自身capacity同样计费；不额外提供无限历史预算。没有逻辑等待队列藏在本层，普通world／joint各一槽及一次实际inflight仍由第6／7项组织。需要接纳新表时后台先Collect不再被使用的影子，再回收表快照，避免无用旧根延长快照借用。

**本项尚未实现独立的未确认事件／已提交结果缓存owner。**当前Frame拥有组／事件只是寿命保障，第5项仍须在latest替换前登记、去重并保留未确认事件、检查全部必要结果覆盖；不能把丢候选等同于丢事件。ContextReady、结构解码与影子封存均不授权ACK。

## 提交与失败事务

Assess和Begin必须接收ClientShadowValidation，它检查完整candidate而非只看StateTarget；第4／5项未接齐时不得提供恒Ready或仅域内Ready的实现。Begin再次核成功base与实际投影并申请唯一StateApplyTicket，实际busy保住candidate、两表与字节，主线程执行前仍须按第7项／第13步核取消及原生代次。

主线程真实完成后，由后台Complete串行驱动接受状态机。Success一次提升所有涉及根与原始ACK凭据，再调用ClientShadowCommit的已预留纯值缓存提升，成功才返回ACK；此回调按完整candidate核自己的暂存，只执行无分配提升，不再执行原生操作。没有默认成功回调。若纯值提升失败，两流共同变为不可用、停止ACK并等待清理；历史版本只诊断，不能用已污染状态增量继续推进。

FailedBeforeWrites仅在确无副作用时保留仍合法旧A，不确认失败目标。FailedAfterWrites、取消后Success或无法保证完整提交时撤销相关安装资格；即使逻辑inflight已清空，实际busy继续保存到CompleteDrain收到真实主线程清理回报，不能马上启动第二批。只恢复另一条独立流时不撤销无关在途批次；涉及当前joint或结构则按真实影响共同恢复。

成功提升所需的根、索引、registry及纯值结果容量均须在原生写入前预留；成功路径只交换已持有引用和固定值。重复／旧route／旧serial完成不产生新ACK，不重放事件；这里仍只是返回纯值凭据，不发送网络包。Close先撤资格并清可见安装引用，再等外部借用和真实busy排空；Drained不替代第13步原生／GPU退休证明。

## 工作区与类型化失败

第2项与本项共用ClientStateWorkspace大型后台工作位置，竞争立即返回WorkspaceBusy；和剩余编码／压缩owner的统一调度仍由第6项完成。scratch依实际消息长度及生成头尺寸取得保守容量，封存数组按真实数量分配；不预占所有布局的最大fact数组。

BodyCodec新增BodyScratchError／BodyDependencyError／BodyVisitError，保留Gap兼容基类，仅补类型化原因与数字定位／额度，不改序列化或布局。facts不足时有界扩充并重试私有解码，重试不提交任何状态；累计完整根重检的本地访问量耗尽给Budget，非法网络正文访问量仍按编码失败处理，不匹配错误字符串。重检使用可靠大小保护只是为容纳已合法安装的大根，不改变普通网络lane或PlayerFast报文额度。

源缺失、缺base、错误引用、结构不完整、内存／工作区耗尽与依赖未就绪保持类型化区分；任何构建失败均释放未发布暂存并保留原安装根／版本。失败诊断本身不授恢复或操作资格，真正Drop／Wait／Resync／Terminal动作与期限归第6项。

## 实施与编译记录

新模块首次编译成功。静态复核补齐独立流恢复范围、普通body投影不相容分类、跨流总record字节预算、缺表确实丢失与尚未到达的区别、shared presence重检及无分配缓存提交接口，并复用没有新链接绑定的旧块元数据。

最终执行`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`，Release／Win32状态库及产品DLL纯编译／链接成功，产物为`out/repentogon/zhlIsaacCS.dll`，E2E构建选项OFF。本项期间没有编译失败。共享工作区第10步已修正Host桥不完整类型构造问题，第2项记录的当次构建失败保留原事实；本项没有修改Host桥。

没有运行游戏、E2E、单元测试、自测、probe、独立校验脚本或任何提前试跑，没有新增测试、安装／注入产物或写存档。编译成功不表示事务／并发／性能已运行通过。全域具名检查、第5项结果／事件缓存与提升、第6／7项调度／主线程交接及第13步原生安装仍待后续，第15步唯一统一整体验收。
