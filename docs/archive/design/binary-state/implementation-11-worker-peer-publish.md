# 第11步：worker／Peer二进制发布与事件确认

日期：2026-11-13（Asia/Singapore）。布局版本74。依据：[显示委托方案](client-display-delegation.md)第14节第6项与[Host发布合同](11-host-publish.md)。本页记录代码接线和实际边界；没有运行测试，第15步仍是唯一统一整体验收。

## 已接入的链路

`BinaryEntities::Background`领取实际consumer的ordinary／body票据，借用票据保护的不可变资源前缀，经`ExportStateResources`和`HostStateCapture`形成自有快照。游戏对象、Lua与主线程scratch不进入快照。ordinary完整枚举决定成员集合；body必须关联当前已封存ordinary，不能凭缺读制造离开。开始网络订阅与发布撤销会提高主线程发布修订下界，重新取得合法ordinary之前不复活旧邮箱值。

`HostStateCapture`按成员／物理块memcmp，保存块的真实最后变化修订。失败或编码中撤销不推进有效快照；变化后又变回仍带新的变化修订。来源修订与Peer的world／body目标号分开。成员号在一个run内只追加，耗尽明确报错；资源按table独立编号并保持同run不可变内容。当前采用扫描已封存块的变化修订，不依赖尚未实现的变化日志环。P6／P7拓扑与绑定修订单独跟踪，不借member shape推断；其变化必须由同ordinary来源的world／body共同提交，fast body还须与该ordinary拓扑相等。

`PublishPeer`把快照交给网络worker，正式发布不再取旧`CurrentLiveBaseline`／`CurrentLiveWorld`／DTO bodies。Live订阅不再申请Lua全域DTO计划；旧PlayerFast Lua正式分支停用，显式Latest／Recorder的独立观测需求仍由各自订阅管理。旧采集／渲染代码的物理删除继续归第14步。

每个`HostPeer`独立使用`HostStatePublisher`：可靠发送成员表和资源表，匹配真实已发送表的`ContextReady`后组帧；首次准备、结构重建和恢复使用共同提交的base=0 world／body帧。普通world与body分别以各自AppliedAck为base扫描变化块。空world仍有帧头和22B事件队列头，二进制正文最小126B，另计40B ICSR封套与实际分片／IP／UDP成本。

封装与zstd经既有`EncodingCache`后台线程完成，网络owner保留不可变待发消息并逐片发送。world／body普通帧沿用非可靠动态通道；表与共同重建走可靠通道。普通帧超出正文预算时请求完整共同恢复，不截断成员／状态。事件段只选择整帧剩余空间内的有序前缀，未选后缀留队。未完整发出的普通候选超过对应发送期限时撤销候选，不登记发送凭据，也不回收事件；持续无法发出的流转共同恢复。

## 两种回收不能混用

1. **采集接管回执**：ordinary快照保留完整`event_first_sequence`／`event_watermark`。网络worker逐个向当前存活游戏Peer复制；全部成功后置该快照的u32接管标记。原consumer仍持有采集票据，看到标记后调用`EventsTransferred`，通过原有SPSC回执让主线程回收采集副本。FIFO满则保留票据重试，不等待主线程。没有网络订阅的独立观测按空Peer集合处理。
2. **Client AppliedAck**：Peer的`StateEventQueue`与实际发送ledger保留事件和正文。只有整帧所有分片真实发送后才`RegisterSent`；片段ReceiveAck只回收可靠传输缓存，不能推进world／body或事件确认。AppliedAck必须匹配实际目标、joint关系、成员／资源epoch与事件水位，才推进确认并回收队列、正文及其来源快照。未知／冲突ACK不推进；超过近期回执保留范围的旧UDP确认也不会倒退版本。

每条队列限制512条／256KiB，队列头与事件头也计入字节预算。容量不足先丢最旧外观事件，外观超过15个Host帧可过期；必要事件不能静默删除。采集、Peer接管与原生应用分别有水位，不能拿接管冒充播放成功。

旧表的未确认事件先在旧完整来源上发送并确认，随后才切到含新成员／资源的表。事件选择上限是对应快照实际覆盖的水位，不能把新事件塞进尚不包含其引用的新表之前。跨scope时也先完成旧Peer队列的确认，再退休旧复制route。已发目标强持有其正文、成员／资源与块快照，latest替换不改变这些依赖。

已发目标超龄会使用保留的恢复槽发送共同完整重建；未确认事件和旧发送凭据继续保留，直到完整ACK或明确退休。同一Resync重传不抬高恢复起点，也不反复增加重建尝试。首次准备／恢复使用`StateTransferEpisode`累计实际发包、接收确认和重传，持续上行仍经现有总桶、每Peer桶和通道桶计费。

## 封套与能力

- 复用ICSR／ICRF控制和分片外壳；`ICSR.flags=1`表示正文直接从50B二进制公共头开始，`flags=0`仍解释旧compact控制。标记登记在`wire.json`的`acceptance_policy.binary_payload_flag`，不改变现有布局字段和顺序。
- Frame／Members／Resources分别使用21／22／23，ContextReady／AppliedAck／Resync继续为8／9／10，但必须携带二进制正文。通道、路由、scope、整数布局版本与压缩协商分别核对。small bundle暂不合并flags=1消息，避免旧bundle格式丢失该标记。
- Host要求`binary-state-v1-layout-74`，准备合同为`binary-state-v1`。正式Host不回退旧Range确认或DTO发布。采集会话仍用于来源连续性核对；线上公共头和准备scope使用握手的实际C/S session，其余run／floor／dimension／room／scene保持采集原值，不把进程级观测会话冒充网络会话。
- **当前Client尚不声明该能力**。其完整接收候选、各域安装和第二批事件回放仍未接通；当前两端不能宣称已经正式联机可玩。版本74表示布局一致，不代表Client已经实现对应原生owner。

195／196参与者与控制组是允许作为成员的具名C/S根：195仅身份、没有零长状态记录，196带world正文。源中无原生shape的这两种根使用固定shape=1；其内容变化仍按块比较。其余protocol布局继续不能冒充原生成员。这也同步修正了表／影子层此前一律拒绝protocol根的检查。

## 诊断与仍在的边界

`world`状态输出新增`binary_publication_revision`／`binary_publication_failure`。Peer状态分别输出`binary_world_ack`、`binary_body_ack`、`binary_event_ack`、容量／过期丢弃计数及`binary_unmatched_acks`；它们与采集端`binary_event_transferred`不同。

资源导出只接受已有纯值描述。路径型待准备图像仅可使用同目录中真实Image owner已经观察的同文件几何；缺少几何报告`image_geometry_pending`，不补零或下载媒体。程序surface只承接已核Shape白图Clear，不恢复已退役Host离屏显示生产。Boss肖像／额外资源及生命周期后绑定的非音频请求仍受第10步已有来源边界约束。

已经交接的旧事件有Peer队列和在途目标快照保护；若事件到更晚窗口才首次出现，而其Player身份在该完整成员集合中已经不存在，仍报告引用缺口，不改为Null或复活旧实体。该原生来源问题不能仅靠网络保留凭据补造。

Client完整validator、主线程原子投递／安装、实际事件播放与本地化、音乐mode=5、循环音协调和Boss恢复继续归第12／13步。Host切房时等待的是所属Peer旧事件确认，Authority模拟及其他Peer不等待它。正式FPS、采集耗时、安装耗时、带宽和故障恢复效果均未测试，不作通过声明。

## 纯编译

本轮只构建`zhlIsaacCS`目标，不执行游戏、E2E、单元测试、probe或校验脚本。沙箱内首轮MSBuild子进程停滞，已终止并改用能正常启动子进程的构建环境；随后产品配置的编译／链接通过。最终同一份源码在产品与E2E配置下的Release／Win32 `zhlIsaacCS` DLL均完成纯编译与链接，均为0错误；构建仍有警告。没有运行任何测试、启动游戏或部署DLL，纯编译不表示第15步验收通过。
