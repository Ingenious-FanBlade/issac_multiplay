# tx-04-21 I01—I05：目录、采用合同、分层模型、v2 schema与正式consumer

> 后续进度（2026-11-05）：[I21—I25网络原生安装与确认](tx-04-21-installation.md)及[I23静态研究与主审](tx-04-21-i23-native.md)已完成实现；Release纯编译／链接通过，未测试／启动游戏。I26以后及I39统一验收待继续。下文保留原交付时点事实。

> 后续进度（2026-11-05）：[I16—I20可靠生命周期、预算、时序、Host发布与交接](tx-04-21-publication.md)已实现并经主智能体静态复读，未构建／测试；I21以后与I39统一验收待继续。以下原交付时点的事实保留。

> 后续进度（2026-11-05）：[I11—I15传输实现与主审](tx-04-21-transport.md)已完成，未构建／测试；I16以后和I39统一验收待继续。本文原交付时点的事实保留。

> 后续进度：同日 [I06—I10采集与身份](tx-04-21-capture.md)已完成实现与静态主审，I11以后待推进；本文保留I01—I05交付时点的事实和后续安排。仍未构建、测试或启动游戏。

日期：2026-11-05，Asia/Singapore。本轮完成 I01—I05 的实现与静态主审。**没有构建、运行测试或启动游戏**；整个 tx 的统一构建／P2 全 E01—E26→P4 全套验收仍在 I39，I06—I40 待继续。不存在本轮 E2E 通过或 30／60 Hz、10 Mbps 性能达标声明。初版任务范围和历史事实保留在[实施计划](tx-04-21-implementation-plan.md)。

## I01：逐成员责任目录

[stream-catalog.json](stream-catalog.json) 包含五份正式观测目录的 **3,537 条原始签名行**、18—20 应用目录中的 **290 条原生／结构补充条目**。同签名别名保留来源；计数不是独立功能数量或运行覆盖数量。`mod/tools/gen_stream_catalog.py` 同时生成产品 JSON 与 C++ 嵌入目录，`stream::Catalog()` 提供只读访问，构建／打包脚本带入同源目录。本轮只生成产品交付数据，没有执行目录校验程序。

每条公开行保留参数、返回类型、原单位／精度、schema／版本、Wiki 来源、采集阶段／可读依据、投影复核、18—20 具名消费者及应用文件、分层归属、完整性／合法空值、失效条件与 I／R／E。私有 `application_*`、原生 backing、身份／成员／资源结构沿旧目录逐条保留，不能仅因不在 Wiki 中而省略。字段号沿显式 `field_ids` 追加和保留，成员插入／退休不重排或复用旧号；没有计算摘要。

分层规则：构造、成员、引用、形态、装备 owner 属 Structure；安装资源定义属 Resource；已核动态最终量按身体绑定到 PlayerFast，其他主体绑定 WorldDynamic。`Entity` 公共字段、Sprite／layer pose 使用互斥的 subject 绑定：身体归 PlayerFast，非身体归 WorldDynamic。原生公共 Entity 安装器仍是唯一实际存储 owner，不能把线协议分层误解成复制两套身体。WorldDynamic 模型拒绝 Player 记录。

现有投影省略清单没有新增删项：静态资源表／无当前消费者的按参查询沿原来具名复核处理；新消费者需要时恢复来源。任何未闭合的窄读取或快写字段保守保留普通联合来源，目录并不赋予 PlayerPost／快写资格。没有完整 dirty 覆盖的低频成员选有界完整刷新，不凭“通常不变”续期。

## I02：R01—R09及主审

[采用合同与必要逆向](tx-04-21-native-contracts.md) 分别登记复用事实、源码确认、J273 定点逆向、静态推导、UNKNOWN 与受影响的实际 I／E。按本地原版 Wiki→REPENTOGON Wiki→官方／本项目源码→必要逆向的顺序执行。指定 GPT-6 Luna max 子智能体完成必要研究，主智能体独立复读关键指令与消费者。

主审采用／纠正：

- VA `0x6F9559` 是 ordinary odd Player 的虚调用，后续 `0x6A6A40` 能修改位置／速度并触发 Grid 响应；`0x6F9565—0x6F9575` 还清临时 flag。因此 PlayerUpdate 返回只是更新见证，全部公共动态组仍需等待后置写入、清理及外层正常收束。
- PlayerManager `+0x125C` 是指针数组基址，`+0x1264` 是元素计数；额外向量 `Game+0x1BB78/+0x1BB7C` 的具体类型未知，不能猜成 Player roster。
- `ClientResourcePoseMatches` 是完整值相等／回读检查；实际无 Load 的复用分支为 `ReusableGraphics→SetPose(...,false)`，其当前 frame／selector／Image 条件仍有限。I23 必须对实际新放宽的跨帧快写补合同。

本轮没有新增 native writer、Hook 或 PlayerPost 来源。I08／I23 的所有身体／字段闭包和真实运行频率仍按具名限制实施；具体 UNKNOWN 不被目录完成或本次主审抹去。

## I03：不可变范围与合成视图

`replica/stream_model.*` 定义共享 `WorldScope`、`RangeId/Version`、真实来源 `Sample`、`Member(Key/use/shape/category)`、固定布局 `Record/Field/Atom`，以及 Manifest、Context、Structure、Resource、WorldDynamic、PlayerFast 六种范围。connection／route 不进入共享范围正文。FieldStatus 保留 Value／Null／Empty／NotApplicable／Missing／NotScheduled／ReadFailed／Unsupported／Truncated，布局在本地指定必需字段、精确类型与可空／可空集合条件，报文不能自己缩短必需字段列表。

`Freeze` 在复制前后检查预算与完整性，并深复制复杂值后公开 const 所有权。普通样本与身体样本分开计序；PlayerFast 必须有当前 native update tick、binding、required world sequence 与精确 Structure 依赖。初始 shape revision=0 合法，不伪造新 shape。Partial 成员不能生成删除，完整空成员集合可表达确实清空。禁止内部指针／handle 键。

`Store::Admit` 区分完整、等待依赖、重复、陈旧、同版本冲突、缺字段／缺成员、非法值与超预算；保存精确值而非摘要。`Compose` 按确切版本闭合依赖并拒绝环；`Commit` 是安装器实际完成后使用的纯值接口，本身不写原生或发送 ACK。身体版本单独保留，普通帧不覆盖它；Structure 换代撤销依赖旧结构的身体视图。仓库按总字节／条目／每范围128版本有界，达到上限明确拒绝，I09／I14 将接入实际引用保留／回收。

`ColdModel` 给已有18—20解析器提供一次冷适配：合并所有必需 component、指定连接视图、调整合成模型 revision，同时保留原始 capture 事实。旧入口只接收与 manifest 同一普通 tick／outer／owner／mutation 的十一域；混合时刻明确拒绝，绝不改 tick 伪装同窗。显式引用旧但仍有效 Context 的一般合成视图保存在范围仓库中，I06／I22 接具体上下文有效性与原生安装；不在每个热帧重展开整个 DOM。

## I04：ICSR v2／ICRF v2与严格能力

`replica/stream_codec.*` 与 `stream_range.cpp` 定义新消息、严格嵌套范围 schema、方向／状态和编码；旧 `replica/codec.*` v1 保留给现有离线工具，ICSN Rust 输入工具不变。新增共享无损值树编解码，不静默扩展旧消息顶层。

| 消息 | 方向／用途 |
| --- | --- |
| Hello／Welcome／Ready | 握手、nonce／participant／能力；等待路由不携带真实世界声明 |
| ScenePrepare／ScenePrepared | 明确 next route、world／manifest／generation 的准备事务 schema |
| Range | 六种完整范围、精确来源／版本／成员／字段／状态及依赖 |
| ReceiveAck | 单 transfer 的256片窗口、四个u64位图、合法尾位与完成声明 |
| ContextReady | 具名版本的纯值／资源依赖就绪，不能提升 native 完成 |
| AppliedAck | 具名范围版本、真实 native／retained 域与事件水位 schema |
| Resync／Cancel／Stop／Error／Disconnect | 具名恢复、transfer取消、终态／错误；不依赖世界解析完成 |
| Roster／TickProgress／InputAck／Input | 完整名单、独立时序、真实输入进度和 canonical 输入 schema |

ICSR 逻辑头40字节，带 kind／lane／sequence／connection／route；ICRF 分片头64字节，完整数据报≤1200，净荷≤1136，匹配 transfer／长度／片号／kind／lane／codec。未知版本／kind、flags／reserved、重复字段、非法尾位、错误方向／状态、冲突片、长度／类型／非有限值均具名拒绝。大消息授权、进度超时、选择性补片和全局计费还归 I13—I17；当前重组器为小消息有界接线，不冒称可靠世界传输已完成。

真实 `net/live.*` 已改为 v2 编解码，内部现有 DTO 经明确适配映射到 Hello／Welcome／Ready／Roster／Input／InputAck／Stop／Disconnect／Error。旧真实游戏 v1 被版本拒绝。当前 world kind 在 socket 路径明确拒绝，接线留 I19—I25；不会因解码成功生成 Applied ACK。roster 与可靠控制计序分开；20Hz名单／独立TickProgress的实际发送和输入时钟仍归 I18。

`stream::Capabilities/Negotiate` 区分 protocol=2、framing=2、stream_schema=1、model/projection=2及18—20目标合同。能力名称明确标为 schema；当前压缩只广告 none，不声称 zstd 可用，网络 native_appliers 仍空。Live 的 `GameCapabilities/AgreedLimits` 广告并执行小消息预算64KiB、4096节点、64片、2候选、1000ms；固定 datagram／bytes／fragments 不满足时拒绝，候选和超时只收紧。正式世界授权预算与 zstd 接入分别留 I14／I12。内部旧等待 Scope 的哨兵适配尚待 I21 换成真实世界准备事务，不能把它当已建立 run／room。

## I05：正式Live consumer与生命周期

四槽身份为 Latest=1、Record=2、Live=3，第四槽仍可用。`WorldCapture::SetLiveConsumer/UpdateLiveCapturePlan` 返回重复、满槽、在途 scope、计划／预算失败等明确结果；换计划只有 Service 接纳后才切换 Live 自身计划，Player ordinary 必须带同窗 Entity 来源。`Service` 先构造完整候选再原子交换计划，分配失败保留旧 consumer；`Due` 执行每个 consumer 的实际 interval 并集，回调按各自 interval／预算交付，避免用最短间隔覆盖所有人的需求。

Host session 通过 `PumpWorldCommands→LiveCaptureSession` 自动取得正式采集需求，不需要 Recorder、细日志或手工 observation start。Client／Solo／Off／失败／退休不申请 Live；停止 Live 不关闭仍由 Latest 或 Recorder 使用的源。准备失败在同一 Host 中保留首因停止自动重试，不影响输入裁决；新 Host 使用独立 owner。

当前 Live 计划以完整正式来源每个合格 ordinary 机会取得十一域，I06—I09 再接 Baseline／低频有效性和 typed窄来源；**这不是2ms性能或30／60Hz达标结论**。同一合并计划只执行一轮正式 Lua 域读取，旧 native 兼容投影不进入 Live，不按 Peer 重扫。

`world/common/live_consumer.*` 在短借用回调内复制一份受256MiB账本约束的自有 envelope；同窗 tick／outer／owner／mutation／plan 和全部当次必需域正常 commit 后才公开 `shared_ptr<const LiveBatch>`。多个 Peer 使用同一批值。最后后台引用释放时归还账本，不回调已退休主线程 owner。独立 Render 不冒充 ordinary；odd-only 是原样 gap，不补世界帧。取消、loss、mutation、读失败／Partial、run／room／plan／session变化撤销候选并保留具名原因。Recorder I/O 的失败不作为 Live 或输入成功条件。

`CurrentLiveCapture()` 仅主线程取得自有共享批次；I06／I19 再接来源与网络世界身份、不可变目标和发布。`observation status` 增加 Live 挂接、拒绝原因、交付数、持有字节及 gap 原因。当前没有网络世界快照、PlayerPost、后台游戏指针、文件桥或新原生写入。

## 交接与验收安排

下一步 I06—I10 接实际 Baseline／typed 动态来源、上下文有效性和 Registry 世界身份，再按计划推进传输与应用。I39 前不构建或执行分项测试；本轮只有资料生成、代码／文档静态审阅和必要静态反编译。未增加单元测试、tx artifact、运行日志包或测试快照。实机前按用户要求逐次告知所需配合并确认准备，I39在同一tx内连续修复／续测。最终网络／画面／成本／隔离结论以实际 P2 全套后 P4 全套验收为准。
