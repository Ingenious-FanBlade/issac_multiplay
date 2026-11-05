# tx-04-21 I11—I15：紧凑编码、压缩、路由预算与选择性补片

> 后续进度（2026-11-05）：[I16—I20可靠生命周期、预算、时序、Host发布与交接](tx-04-21-publication.md)已实现并经主智能体静态复读，未构建／测试；I21以后与I39统一验收待继续。以下原交付时点的事实保留。

日期：2026-11-05，Asia/Singapore。状态：**I01—I15 已实现并完成静态主审；未构建、测试或启动游戏。** I01—I10 的历史交付见[基础](tx-04-21-foundation.md)与[采集](tx-04-21-capture.md)。整 tx 的 I39 仍按[统一 E2E](tx-04-21-e2e-plan.md)执行 P2 全 E01—E26→P4 全套。本轮没有提前分项试跑、单元测试、哈希校验或 tx artifact。

## I11：固定二进制正文

`mod/src/replica/stream_binary.cpp` 定义 compact payload v1。ICSR v2 的 40 字节逻辑封套保留，正文首部为小端 u16 schema=1；握手必须具备 `compact-payload-v1`，不让旧通用值树 decoder 猜读新正文。

控制消息按本地 `Schema` 的固定槽位顺序发送，数值为小端 u64，输入轴为有符号 i16，布尔为独立 0/1，ReceiveAck 位图为四个 u64。输入的 -32768 仍按 canonical 规则拒绝。名单、TickProgress、InputAck、世界版本与确认分别保留自己的消息序号；没有新增可靠控制序号合并语义。

范围正文直接读写 `RangeData`：WorldScope、RangeId、revision、Sample、完整标记、依赖／required 版本、成员／use／shape／类别、record 布局及字段 id/status/atom tag。`Message::range` 在解码后直接持有 DTO，`ReadRange` 返回其共享引用；热路径不构造全域 JSON 字符串或 JSON 范围封套。旧 `RangeMessage` 是冷适配视图，字段语义相同。

复杂字段仍是完整自有值树，保留精确 u64/i64、double 位型、字符串、空集合及状态；不做量化、差值、相对旧目标继承或泛型子树去重。只有一张版本固定的常用键编号表，其他键原样发送。来源/API 字符串不擅自转换为数值。字段 Null／Empty／NotApplicable 与 Missing／ReadFailed 等有不同状态；是否允许由本地 `CaptureLayouts` 决定，禁止从收到的字段倒推出“完整布局”。

接收先无分配扫描长度、计数、节点、深度、UTF-8 和展开额度，再申请租约并构造 DTO。UTF-8 直接检查过长编码、代理区和范围，不通过 JSON 往返验证。未知 schema/tag、重复键／成员／字段、缺失必需字段、非法尾随字节和不完整成员明确拒绝。通过 wire 校验不代表原生已应用。

## I12：单帧 zstd 与共享内容

引入[官方 zstd v1.5.7](https://github.com/facebook/zstd/tree/v1.5.7)的 common/compress/decompress 源码，位于 `mod/third_party/zstd`；选择随附 BSD LICENSE。`mod/CMakeLists.txt` 将其编译为 Win32 静态库，与 DLL 同用 /MD；不依赖额外 zstd.dll，不构建 CLI、测试、训练器或旧格式，禁用多线程压缩和汇编。I31 发布包必须带该许可。

`stream_transport.cpp::EncodeBlob` 先编码完整逻辑消息，再以 level=3、windowLog=23、contentSizeFlag=1 压缩，最后按片生成。快速／控制消息只用 none。算法在 `compression` 中协商；窗口上限在独立的 `world_limits.zstd_window_bytes=8388608` 声明。压缩长度与展开长度分别进入 ICRF 头，不把其中之一当另一个预算。

接收只接受已授权可靠 lane 的单一普通 zstd 帧；拒绝字典、skippable、未知展开长度、超过 8 MiB 的窗口、截断／无效帧、拼接／尾随帧、长度不符和超限。zstd 自定义 allocator 每笔分配先向 Workspace 池申请，包含上下文和窗口；C 回调不抛异常。解压输出单独预留，全部失败路径通过 RAII 归还。

`Blob` 不含某个 Peer 的可变路由。共享正文中 ICSR connection/route 的 16 字节为零，发送单片时才加入实际路由；接收按本地已授权的头恢复逻辑封套，再严格解码。源序号和 WorldScope 保留在不可变 blob。多个 Peer 共享同一压缩内容，各自只有发送位图和时刻；Start 复核 blob.world 与正式 route.scope，换代后的旧编码结果不能贴到新世界。

本层是同步、有界纯值工作，不创建异步任务；一次调用结束即释放工作区。I20 的线程交接须继续使用这些共享 owner 并复核路由，不能另加无限任务队列。编码／压缩微秒及实际长度由 Blob 暴露；没有本轮压缩率或性能实测。

## I13：路由头、合包与 socket 背压

保留严格 64 字节 ICRF v2 头，单 UDP payload≤1200、净片≤1136。kind/lane、保留字段、transfer、压缩／展开长度、首中尾几何和计数在候选分配前检查。`Route` 明确 connection、epoch、是否协商／世界授权、方向、共同世界上限与完整 WorldScope。endpoint 由现有 socket owner 先绑定；只有控制协商不开放世界额度，等待连接也不冒充 run/room。

新增 ICRB v2 小消息容器：28 字节共同路由头（magic/version/header、connection、route、u16 count、u16 reserved）；每项是 u16 正文长度、u16 kind、u8 lane、u8 reserved、u64 sequence、u64 transfer，后接完整 compact payload。最多16项，总长≤1200，不跨 Peer/route。容器不重复每项的64/40字节封套；接收先验证整个容器，再恢复有界单片交已有重组器。未知 kind、序号、非法长度、路由和尾随均拒绝。

Host roster＋InputAck 已实际调用 Bundle，Host/Client socket 路径均处理容器。当前同轮立即合并，不额外等待凑包；停止／资格不会因等待凑包延后。真实游戏 worker 与 Client select 等待上限由10ms改为2ms；后续可靠 owner 同时提供 NextWake，供 I17/I19 接入补片到期调度，不将静态等待上限当运行频率证明。

两端使用至多256个完整数据报的计费待发队列。临时 WSAEWOULDBLOCK/WSAENOBUFS 等反压保留未发送数据；已发送的前缀不重排，排队失败明确终止当前 Peer。停止先撤未发送普通队列并立即尝试结束通知。大可靠消息必须走 SharedBlob＋ReliableSender，兼容 Fragment 入口限制为小消息，不允许按 Peer 构造32MiB分片副本。

## I14：同一进程的三池账本

`core/replication_memory.h` 提供进程内共享 Budget、不可复制 Charge 与共享 Lease：Values=256MiB、Wire=64MiB、Workspace=64MiB。申请在对应分配前，失败不扩大额度；每池暴露 current/high_water/limit。共享引用不重新收费，内容复制必须有新预留。账本只管理纯值，析构不调用主线程或原生资源。

| owner | 池与释放时机 |
| --- | --- |
| 正式 Live envelope／解析分层临时值 | Values；回调复制前申请，最后 envelope／窗口释放归还 |
| Freeze 的独立范围、解码 DTO、Store 依赖／A／T | Values；范围带租约，跨 Peer／仓库共享；最后 Range 引用释放归还 |
| 冷 ClientScene 的 DOM、派生计划和资源纯值描述 | Values；写前保守预留，A/T 各保留自己的 owner；失败预检额度累计保留至完整替换或退休 |
| 共享编码／压缩 Blob、接收连续 buffer、发送位图／时刻、socket 待发队列、重复完成历史 | Wire；按实际独有缓存或容量预留，取消、淘汰／终止释放 |
| 编码双 buffer、解压输出、zstd 每笔内部分配、小包还原 | Workspace；调用／受控工作结束归还 |

握手仍为64KiB／4096节点／2候选；`limits` 与独立 `world_limits` 严格协商，报文长度不能提高本地上限。授权可靠消息 wire/decoded 各≤32MiB、≤32768片；快速范围≤64KiB、32K节点。接收按 route 再取本地与共同上限的较小值，最多两个活动可靠 transfer，活动可靠依赖不为新目标淘汰。完成历史允许回收；没有可淘汰内容则具名拒绝当前目标。

冷 Scene 预留采用 `OwnedValueUpperBound` 和原生对象来源 root 数的保守上界，不等同实际 RSS；失败预检可能仍有计划引用，不能在失败返回时无条件返还。该保守策略可能先于32MiB报文上限拒绝旧的大投影，需要在 I39 观察并按实际 owner 优化，不能宣称现有17.4MB冷投影已在新池内运行通过。原生图形／游戏资源与进程地址空间另行统计；384MiB不是32位进程的内存保证。

`Worker::Status` 与 `observation status` 暴露三池统计（0 Values、1 Wire、2 Workspace）；后者列 current/high_water/limit。正式发布、应用线程交接在 I19—I21 接入这些 owner，不能绕过账本另领一套额度。

## I15：ReceiveAck 与选择性重传

接收候选使用连续 buffer 和位图，按片位置写入。描述不可变化；乱序合法，同片同值幂等，同片异值／描述冲突取消并拒绝。新有效片更新 progress；重复片不更新。只有全部收齐、解压、完整 schema、方向及 route.scope 均通过后才交一个 DTO，保留原压缩内容用于完成副本比较；从不产生 AppliedAck。

ReceiveAck 是每窗口至多256片／四个u64位图。正常50ms反馈，尾片或明确缺片立即反馈；每次最多8个窗口。complete 只在完整校验成功后由末窗口表达；完成记录每200ms补确认，收到完成副本立即补确认，且不重新解码／应用。

`ReliableSender` 每 Peer 至多两个活动 transfer，保存不可变 blob、实际 sent、acked、sent_at 和游标。只有 SendResult::Sent 才标 sent；WouldBlock 保留位置。ACK 必须属于当前 connection/route/transfer，count、窗口、尾位和所有确认位均须合法且已实际发送；错误 ACK 不变更进度。重复 ACK 不刷新 progress，只对缺片在初始200ms到期重发；若全部位都确认但 complete 丢失，探测一个尾片以取得完成记录，不重传整份消息。

取消、过期与历史淘汰进入有界退休记录（最近128条及更旧序号边界），迟到片不能复活。接收历史保留原值且同受 Wire 池；额度不足先淘汰完成历史。2s无新有效片／新确认与120s绝对截止均收口实际 transfer。I16 继续实现 RTT/RTO 自适应、按授予速率的截止、可靠 cancel/prepare/retire 等操作生命周期，I17 负责线上流量与公平；本轮不把固定初始RTO当成这些后续任务已完成。

## 主审与后续边界

本轮为 R09 的纯协议／源码实现，无新增游戏 API、字段、Hook、原生 ABI 或玩法行为需求，故没有新游戏逆向，也没有启动 Luna 子智能体。既有必要逆向和主审仍见[采用合同](tx-04-21-native-contracts.md)；后续游戏细节继续遵循本地 Wiki→源码→必要 Luna max 逆向→主审。

静态审阅覆盖固定字段对称性、符号／浮点与状态、allocator 转移、候选分配前检查、原值重复比较、ACK 已发送约束、完成 ACK 丢失、首中尾片补传、路由取消和租约生命周期。过程中纠正了候选按创建时间而非有效进度超时、完整布局由入包自推、普通反压丢剩余数据、完成确认丢失后无探测、以及重复握手不能恢复应答的问题。这些是源码审阅和修改结论，不是运行失败／通过记录。

I16—I40 未完成。尤其 I19/I21 正式世界发布／接纳、I23 原生快写及 I39 统一验收仍待执行；现有等待路由没有因此开放世界，也未发送收包伪 Applied ACK。30/60Hz、10Mbps、压缩体积、CPU、内存实际峰值与画面闭环均未验证。
