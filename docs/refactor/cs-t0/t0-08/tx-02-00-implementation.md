# tx-02-00：T0-08 操作5–9实现

最新实机结果（2026-10-20）：用户明确确认继续后，已进入真实Authority并创建B，物理输入成功；控制器因`protocol trace overflow`退出1。正常停止helper成功，逐槽恢复文件未保存，报告随后因缺文件抛出次要错误；未完成完整实机验收。三次执行及原始失败见[结果记录](tx-02-00-results.md)，下文早期未运行/初次失败描述保留当时事实。

2026-10-20，Asia/Shanghai。范围：输入冗余、独立lane最新消息、严格decoder、golden/负向/fuzz corpus、确定性有界FaultInjectionTransport，以及用户要求的真实Authority E2E接入。沿用无哈希路线和包CRC。

先登记[失败方式](tx-02-00-failure-modes.md)、[E2E清单](tx-02-00-e2e-checklist.md)，编写独立参考向量和完整生命周期驱动，再扩展生产核心。随后完善接入E2E场景和统一执行器，没有新增单元测试，没有提前运行probe/smoke/回归。

操作5–9、真实Authority接入和统一E2E执行器已实现。用户于2026-10-20要求开始E2E后已执行唯一整体入口：**离线19,022/19,022 PASS，未找到运行中的isaac-ng.exe，整体FAIL、实机NOT_RUN**；未自动重跑，详见[实际结果](tx-02-00-results.md)。静态分析由用户指定的GPT-6 Luna / max子智能体完成，具体发现与修正见[静态记录](tx-02-00-static-review.md)。下文编译及执行前描述保留当时事实，不能作为实机已运行依据。

## 操作5：输入冗余

- wire升级为实验版本2、capabilities=15，保留ICT8/64字节header/1200字节包上限；旧v1拒绝，不重新解释。
- INPUT载荷：record_count:u16、reserved:u16、reserved:u32、last_ack_message_sequence:u64，随后1..4条既有64字节V2命令。最新command加最多3条旧冗余，按input sequence严格递增，内部session/client与envelope一致。
- 整个包和全部记录成功解码后，才按旧到新交给真实Participant统一codec/validator/queue/dedupe。packet sequence与input sequence分开。
- 这是unreliable语义：没有后续新包时最后一个丢失edge不保证恢复，超出冗余覆盖的强乱序也可能丢意图；只保证同identity最多消费一次，不宣称可靠交付。
- generation为内存transport的可信本地元数据，不进入canonical；换代清队列/history，取包后在Endpoint.Receive前再次核对当前generation，sequence不重置。

## 操作6：逐lane最新消息

ACK/STATE分别保留最新有效包与自己的接收水位，完整session/sender/sequence随包传输；ACK重复完整协商身份。同lane Progress ACK的server_tick、accepted、processed、processed_tick、processed generation、rejection count和state不回退，mask不能丢失已报告位。Progress仅允许Active及后续状态。processed_source可在中性tick为0，不能作为连续ACK。

状态lane的tick不回退，但可合法落后于另一lane的ACK。状态仍只提供PartialValues/Gap。新增reason19=`ValuesNotExported`明确表示真实采集存在而协议未导出原始字段；不将Complete误称NotReady。已有gap/odd保留真实原因，不提供跨进程相等或完整世界资格。

## 操作7–8：decoder与corpus

严格拒绝未知kind/role/flags、非零reserved、错版本、非精确长度/尾随、CRC、非法canonical/内部身份、0/MAX序号、冗余越界/倒序/重复序号、非法状态和非有限F32；Endpoint另校验冻结peer/session、协商/握手及lane进度。

`protocol-corpus.py`用独立Python struct/zlib构造双方HELLO、首ACK、单条/四条INPUT、中性ACK、PartialValues和Odd Gap golden，不调用C++encoder。最终一次执行保存golden和每个截断长度、扩展、逐位翻转、旧版本、修复CRC后的语义反例、固定seed的1024份随机fuzz。

`protocol_full_main.cpp`链接真实Endpoint/FaultTransport/Participant/codec，覆盖corpus、双端到Participant生命周期、四个故障profile、lane进度回退、generation失效、预算不足及容量满。离线场景使用typed Participant接口；生产AuthorityTickPipeline的bytes入口由同次实机阶段覆盖。CRC标准参考值也在同次核对；尚未运行，不预报PASS数量。

## 操作9与Native接入

`protocol_transport.hpp/.cpp`提供128槽、单owner、无OS网络调用的FaultInjectionTransport。逻辑Authority tick与固定seed的xorshift64控制delay/jitter/loss/duplicate/reorder/outage，副本保留wire identity，按(due,token,copy)稳定交付。Trace区分Send/Drop/Enqueue/Deliver/Receive/Rejected/Full/Admission/Closed。

HELLO/首ACK走可靠自有槽，故障仅作用在Established之后；没有实现握手丢包重传、socket、Steam或transport bitflip/truncate/extend。损坏包在corpus覆盖，不写成实机故障。Full不提交Endpoint发送序号；runtime失败正常收尾，不自动重试。

显式`coop-active --protocol t0-v2 [--protocol-config JSON]`要求world-record。仅该模式使用IPC v6，完整v3/v4/v5前缀保留；v6在4205144后追加64字节FaultConfig、3个8字节计数器、256条1248字节trace，总4524720字节。配置/view大小在Hook发布前验证冻结。

初始化只做协议数据握手，Hook发布后由主线程接管；Established不替代exact-pair Active。B的准时和迟到脚本意图都经Remote编码→内存FaultTransport→Authority解码→共同Submit/bytes准入；迟到项保留旧target，由validator过期拒绝。合法A/B共同调用V2 Encode，只有历史非法故障才raw-copy交原validator；协议模式禁止source faults及conflict-b直提交。

用户明确选择：A/B首个新鲜样本同样生成`completed+1`命令，同一样本的连续量再以新sequence、edge清零排入nominal target；本次显式配置后续`lead=8`。B两条命令均经过普通故障传输，首包迟到则正常过期，不滑动target或恢复旧edge。B的源端startup标记每generation只消费一次，不依赖可能长期为真的接收端StartupPending；换代后仅新鲜源可再次启动。A保留既有共同codec/queue启动路径。

Pump在Draining，整包开始前至少预留4次准入预算，最多32个上行包/128命令、64个反向包；不等待全员输入，静态不证明最坏帧时间。协议模式Submit诊断额外保存完整session、client及提交generation，供64字节命令与wire逐条配对；不改变IPC前缀或旧模式。

TopExit真实commit后用同TickReceipt构建ACK，用Capture保存并按expected tick核对的run/room/token及真实post元数据构建Gap。主线程只写有界trace ring，Rust持续drain到独立`protocol.jsonl`；正常stop、session退休和失败关闭协议，保留原70 Hook恢复事务/DLL常驻策略。

## 唯一验收入口

最终纯编译已完成：`out/cs-t0/tx-02-00-build-ready-02/`，Rust release、Native Win32 DLL、Helper、ProtocolFullScenario均成功，构建脚本退出码0；未执行任何产物或测试。目录内保存四项构建日志与`build.json`工具链记录。该构建包含用户最后选择的A/B同首个+1语义、完整Submit身份诊断及场景标签修正。初版和中间构建目录原样保留，不作为运行证据。

用户确认游戏准备好后，一次入口包含离线corpus/生产场景和实机阶段：

```powershell
python cs-t0/accept-protocol-e2e.py --build-dir out/cs-t0/tx-02-00-build-ready-02 --artifact-dir artifacts/cs-t0/tx-02-00-e2e-01 --authorization "user requested T0-08 operations 5-9 and real-game E2E in this chat"
```

统一入口保存源码/二进制/构建/配置，先执行corpus及生产双端场景；失败不接触游戏、不重试。通过后同次枚举唯一游戏真实exe路径/创建时间并核对批准PE，只启动一次controller/注入。B源240步，176后移动中性，20/100/180有Drop边沿；seed0x20008、delay/jitter各1tick、loss5%、dup/reorder各15%、tick64..83 outage。A以短W/D/S/A闭合动作保持物理输入。脚本结束且最后源target后16个tick提交，再一次正常stop，核对原始包/PRNG/准入/回执/world身份/A partial位移/70恢复及同一进程健康。

`stage.json`及stdout提示物理输入窗口后，执行器最多等待90秒，直到外部工具写入`physical-input-ready.json`；该文件仅用于本次E2E输入定时。操作者聚焦已准备的游戏、开始有界W/D/S/A短程闭合动作并写入该记录，默认2秒后执行器启动controller。动作需持续覆盖Authority tick74..83，工具请求/结果保存在同一artifact；不能只在controller启动前移动。等待超时则记录失败，不启动controller。`stage.json`继续记录controller启动和首次真实commit。

分析器从Send原始字节重算PRNG，核对Enqueue身份/延迟、Deliver顺序、唯一Receive/Rejected以及INPUT每条Admission字节，再有序核对Native Submit的完整命令、generation、tick和Decision。每个真实commit各有一条ACK/STATE Send；ACK全部进度及拒绝计数/掩码/会话状态由Native事件独立重建，STATE身份逐一对应真实capture或gap，并要求远端实际收到两lane有效包。丢失的包保留Drop，不要求每包收到。

外部isaac_test输入属于同次实机窗口；当前未查询游戏进程、未输入或注入。没有Online/Isaac Sync、Steam、双游戏矩阵或Replica操作；失败/未覆盖原样记录，不自动重跑。
