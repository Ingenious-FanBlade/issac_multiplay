# tx-04-21 I16—I20：可靠生命周期、预算、时序与产品交接

日期：2026-11-05，Asia/Singapore。状态：**I16—I20 实现与主智能体静态复读完成；未构建、测试或启动游戏。** 全 tx 的 I39 仍执行 P2 全 E01—E26及子用例→P4 全套，不提前分项试跑、不写新增单元测试、不保留 tx artifact。

本轮只实现协议、调度、纯值后台与既有 Authority 提交点的接线，没有新增游戏字段、Hook、SDK 调用或原生写入合同，不需要新增逆向或子智能体。R07/R09 以现有源码和已主审的 ordinary/odd 提交合同核对；后续需要游戏细节时仍先查两套本地 Wiki，再查源码，具体缺口才交 GPT-6 Luna max 逆向并由主智能体复读。

## I16：可靠操作与进度

`net/stream_session.*::Operations` 与 `replica/stream_transport.*` 共同管理可靠生命周期。消息接纳、数据收齐、主线程执行、精确范围接纳与原生 Applied 分别记录：

- Range 的 Baseline/Context lane 仍使用选择性 ReceiveAck；收齐不产生 Applied。
- ScenePrepare、ContextReady、AppliedAck、Resync、Cancel、Retire、Stop、Disconnect 进入可靠操作记录。名单/时序不占这些序列，不要求等齐旧序号才能停止或取消。
- 新增严格的 OperationAck（kind 19）及 Retire（kind 20），以 operation kind/sequence、stage、progress_sequence、reason 标识结果。接纳/执行/完成/取消/失败与结果收条分开；重复操作比较原值，已经完成只重发结果，不再次交给主线程。各方向最多32个普通活动操作，紧急操作保留4个位置，近期历史各128条；更老序号拒绝。
- RTT采用本机区间、平滑均值/偏差和 Karn 规则；初始RTO 200 ms，限制100—1000 ms。重复片、重复确认、相同执行步号不刷新有效进度。
- 可靠数据的截止按剩余线上字节和保证授予的47500 byte/s计算（初建空闲借用不伪装为保证速率），2 s无有效进度收口，且始终不超过起始120 s。冷准备/等Applied另保留2 s有效进度与120 s绝对界限；I22的实际原生步骤可通过 OperationAck(Range, manifest revision, Executing, 递增实际步骤)报告进度，不能用空心跳延长。
- 取消撤销相应候选、发送游标及迟到片资格。退出取消未完成普通操作，可靠结束结果在网络线程内有界排空；健康确认收口后短暂排空，最迟2 s结束，不让主线程等待网络退出。

每 Peer 的首个复制失败、操作重发/取消/完成/失败及发布阶段通过 WorkerStatus 和 `stream.peer.<ordinal>.*` 输出。它们是生产有界聚合，不要求逐包录制。原有失败记录没有改写。

## I17：同一套线上预算

`Uplink` 由 Host socket 线程独占，所有真实发送统一经 `Worker::State::SendPacket`，成功后才扣令牌。WSA 临时反压不扣费、不推进可靠发送位，完整候选有界保留；旧 route 的待发包在发送前释放。

| 桶 | byte/s | burst |
| --- | ---: | ---: |
| Host总桶 | 937500 | 24576 |
| 每Peer | 312500 | 8192 |
| 每Peer控制/输入时序 | 25000 | 2048 |
| 每Peer PlayerFast | 120000 | 4096 |
| 每Peer WorldDynamic | 120000 | 4096 |
| 每Peer Baseline/Context/补片 | 47500 | 8192 |

线上计费为UDP payload加IPv4/UDP 28字节。ICRB共同头及网络开销按条目编码长度分摊，最后条目承接取整余数，lane归约之和等于物理总额。初建可靠数据可借本Peer的空闲动态令牌，仍受Peer/总桶约束；不借其他Peer份额。P2也固定312500 byte/s。三个Peer轮转，控制先服务、可靠补片有保留份额，动态按完整范围轮转；大WorldDynamic不会因单一FIFO挡住PlayerFast。合包当轮完成，不等待额外凑包；socket轮询以到期时刻收紧至1—2 ms，反压不会形成零等待忙转。

这些是实现参数，**没有30/60Hz、10 Mbps、延迟、公平或CPU实测结论**；E03/E10/E12/E21仍归I39。

## I18：名单与输入时钟分离

Host roster每50 ms发布完整名单；generation、资格、状态、名单位图改变立即发布。每个实际新提交tick单独发布TickProgress，带0普通/1 odd、提交采样时刻、发送时刻、回显及资格；没有新tick仅250 ms心跳，保留原tick与采样时刻。`SessionRuntime::committed_sample_ms/committed_kind`只在Clock实际提交后更新，不把Manager循环时刻当新模拟进度。

两类消息共享递增eligibility_revision，防止乱序旧名单重新开启输入。InputAck仅在确有processed/processed_tick时产生，并尽量与TickProgress合包；首次没有processed仍能由真实TickProgress建立InputTiming。Client只从高频事件更新速率/新命令机会，保留实际速率估计、lead=2、age=0、目标上限与既有启动规则。

Intent在生成时固定完整Scope、connection/route、generation和命令值。Client失焦/UI阻断的输入epoch通知会撤销网络历史及未发输入，恢复不补旧边沿；路由/代次失效同样撤销。socket背压保留命令原值，成功排队才消费主线程边沿FIFO；真正发送失败具名结束，不伪装已处理。冗余仍发送同一命令原值，由Host统一scheduler去重。

## I19：Host正式产品发布

`game/peer.cpp::PublishPeer`在Host角色从CurrentLiveBaseline/CurrentLiveWorld/CurrentLiveBodies取得正式consumer的共享不可变结果，送入Worker的最新完整信封；录制、E2E文件、Rust都不参与生效。获取/发布失败只撤复制来源或请求新Baseline，Authority及输入处理ACK继续独立推进。

Worker根拥有一个共享EncodingCache和一个纯值编码线程，最多128项缓存/请求，同范围尚未执行动态可被完整新值替换，正在编码及可靠依赖不覆盖；三Peer复用同一Blob。所有值树保持各自allocator和现有三池租约。缓存满回收已完成项，池不足优先回收无人借用的编码结果；任务失败返回具名复制错误。停止唤醒并取消待执行任务，在途线程只持有自有State，不访问游戏、socket或已析构Worker，不在主线程阻塞等待压缩。

每Peer PublicationSender保留WaitingWorld/Preparing/Baseline/WaitingApplied/Active/Recovering/Stopped及自己的精确版本、游标、确认。有效冷Baseline不会被每帧新目标取消。两个可靠接纳信用只有精确ContextReady（或覆盖它的真实Applied）才能释放；ReceiveAck本身只推进数据阶段，慢主线程不会被无限后续依赖淹没。信用/执行无有效进度有具名超时。

初始冷闭包都实际发送且收到精确manifest/域位/水位的Applied，才进入持续发布。之后逐范围精确已发送版本与已应用版本分别维护，近期每范围128个已发送原版本，不用“较新版本存在”证明某个从未发送的旧版本。每范围保留一个在途动态和来源中的最新候选；静止最终量每200 ms仍可完整补发。源撤销可靠通知Retire；复制失败只影响本Peer恢复，不清其他Peer状态或改变scheduler ACK。

## I20：网络与主线程的有界所有权

| 交接 | 容量/策略 |
| --- | --- |
| 输入 Intent | 独立256项SPSC，采样时固定身份 |
| 普通通知 | 独立32项；roster、TickProgress、InputAck各保留最新完整值 |
| 主线程→worker控制 | 独立32项，持有自有const Message |
| worker→主线程可靠依赖 | 2项；Host另有2个主线程接纳信用 |
| 世界可靠控制 | 32项；退休另有4项，不被普通候选挤占 |
| 动态 | 每声明范围只保留最新未取出项，总共最多65个范围（WorldDynamic及最多64个身体范围）；取出的在途值不受替换 |
| 停止/失败 | 固定粘性Event和release/acquire标志，无须分配或等待普通队列，首次原因保留 |

OwnedQueue的pop会清空槽位，Latest交换取出，队列失败不留下无主值树。Event携带Scope/route、范围/依赖/源序号的const DTO及本机接收时刻；其引用共享原租约。`Client::PollWorld`及`game::ClientPollWorld`先后复核网络路由和主线程当前Scope/route/ordinal，过期值释放，不让worker重贴身份。网络、编码和释放线程都只处理自有值；Player/Lua/GL/SDK与旧allocator借用不跨线程。

## 相邻任务与实际验证状态

本轮提供Host发布、网络接收/值交接、确认记录和主线程结果接口；**I21完整双方准备事务（含跨route确认/取消）、I22真实Scene冷应用、I23/I24高频/结构安装、I25原生CommitResult→确认仍待实施**。当前ClientManager不会消费世界值并直接写Scene，也不会因解码自动返回Applied；ClientPollWorld/SendControl/Authorize/Complete留给这些具名后续任务接线。不能据本轮socket能力宣称已有网络原生世界闭环或可玩画面。

本轮主审复读了协议固定槽位、严格能力协商、token扣费与合包归约、旧route队列释放、身体Key字段顺序、数据/接纳信用/执行/提交分层、真实提交时钟、重复完成结果、停止排空和纯值线程生命周期。纠正的是静态阅读发现的问题，不是运行FAIL/PASS。没有构建、测试、游戏启动或运行artifact；最终全部运行覆盖仍统一在I39。
