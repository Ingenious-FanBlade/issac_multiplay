# tx-04-06：3.1 net/ 实现记录与接口

后续 game 接线（2026-10-25）：[game 代码与主线程 Host API](tx-04-06-game.md)现已编写并纯构建成功，ops Host 命令仍占位。Worker 的 ReceivedInput 新增本地 acknowledged_generation/state，来自成功发送 Ack 的 last_ack 对照记录，不改 ICSN v1 线格式。Ack generation 只公布已可用的 B 输入代次，暂停/未绑定时保留上次值；Rust 须在新代次清空旧样本/边沿/冗余、保留 session sequence。没有部署或运行测试。下方 net 阶段记录保留原始时点。

日期：2026-10-25。范围为[实现方案](tx-04-06-implementation-plan.md)第 3.1 节，仍属于 tx-04-06 同一次推进。`mod/src/net/` 已编写，加入 Win32 / v143 / C++20 / MD 的独立静态库 `isaac_cs_net`；纯编译、链接成功。没有部署 DLL、启动游戏、发网络包或执行任何测试；tx-04-06 的唯一整体 E2E 留待全目标完成后进行。构建原始输出见[记录](../../artifacts/repentogon/tx-04-06-net/README.md)。

后续修订（2026-10-25）：按第 5.1、5.2、5.4 节新规则修改 Authority 的 stale、重复计数与 Bye 三副本处理，详见末节。Win32 Release 纯编译与 DLL 链接成功，见 [build-04](../../artifacts/repentogon/tx-04-06-net/build-04-network-rules.log)。Remote 专用握手重发及 First Ack 副本缓存仍由后续 Rust 客户端实现；本次没有执行测试。

## 文件与边界

| 文件 | 实现 |
| --- | --- |
| `mod/src/net/protocol.h/.cpp` | ICSN v1 的四种报文、小端 codec、CRC 和严格字段校验；复用 core 的 64 字节 V2 输入 codec |
| `mod/src/net/endpoint.h/.cpp` | 单 owner、一次性握手、发送提交、序号/进度单调性、原字节握手重传、Bye 和墙钟超时 |
| `mod/src/net/queue.h` | 单生产者/单消费者、真实容量 256 的收发队列；满时丢弃当前提交并计数 |
| `mod/src/net/worker.h/.cpp` | DLL 内 Authority 的 IPv4 UDP worker、来源锁定、系统随机身份、心跳、RTT 估计、粘性终止原因及有界停止 |
| `mod/CMakeLists.txt` | 独立库只依赖 core、Winsock 和 BCrypt；DLL 链接该目标，不继承或引入 SDK/游戏头 |

移植依据是 `cs-t0/native/include/isaac_cs_t0/minimal_protocol.hpp` 与 `cs-t0/native/src/minimal_protocol.cpp`。删除全部 StateHash/StateObservation/domain/state_schema 及 IPC 内容。Endpoint 保留 Remote 方向的纯状态机以说明对称合同，Worker 只创建 Authority；没有 DLL 内 Client 模式。

本层尚未被 `game/peer`、`cs host/stop`、状态命令或 trace 调用。它提供可查询的值快照，不直接调用 `ops::Trace` 或游戏代码。静态库未引用的对象可能被链接器裁掉，构建成功不表示网络已在游戏中启动。

## ICSN v1 线格式

所有多字节整数为无符号小端；输入轴沿用 core 的有符号 16 位编码。包总长不得超过 1200，长度必须精确匹配。kind 为 Hello=1、Input=2、Ack=3、Bye=5；4 保留，拒绝接收。role 为 Authority=1、Remote=2。网络身份固定 A=1、B=2，不编码 Player/device 指针或本地绑定 generation。

### 64 字节包头

| 偏移 | 字节数 | 内容 |
| --- | --- | --- |
| 0 | 4 | `ICSN` |
| 4 | 2 | version=1 |
| 6 | 2 | kind |
| 8 | 2 | header_size=64 |
| 10 | 2 | role |
| 12 | 4 | flags=0 |
| 16 | 16 | session id；初始 Hello 全零，其余包非零 |
| 32 | 8 | sender；Authority=1，Remote=2 |
| 40 | 8 | packet sequence；从 1 开始，0 与 UINT64_MAX 无效 |
| 48 | 4 | payload size |
| 52 | 4 | reserved=0 |
| 56 | 4 | CRC-32/ISO-HDLC，整包计算时本字段按零；初值/终异或 0xffffffff，多项式反射值 0xedb88320 |
| 60 | 4 | reserved=0 |

CRC 仅作报文损坏检测，不是认证或项目文件校验。codec 失败时不修改调用者输出。Endpoint 成功后才提交接收高水位；与已缓存报文字节相同的副本返回 Duplicate，只刷新存活，不再次投递输入。握手完成后，非握手报文先通过格式、角色和 session 身份检查；packet sequence 不高于高水位且不是缓存副本时返回 Stale，丢弃内容，不刷新存活、不改变高水位、不结束会话。更高序号的 Ack 进度回退或 last_ack 越界仍返回协议错误。Hello/First Ack 保留专用阶段规则，异常 Hello 不会被当作 stale 放过。冗余记录仍由 Participant 处理命令级去重与过期。

### Hello/agreement：104 字节

| 偏移 | 字节数 | 内容 |
| --- | --- | --- |
| 0 | 32 | build，1–31 个 ASCII 可见非空格字符，以零结尾，其余全零；默认 `isaac.cs.tx-04-06` |
| 32 | 16 | local_nonce |
| 48 | 16 | peer_nonce |
| 64 | 2 | input_schema=2 |
| 66 | 2 | protocol=1 |
| 68 | 2 | role |
| 70 | 2 | reserved=0 |
| 72 | 8 | capabilities=13（V2 输入、CRC、有界冗余） |
| 80 | 4 | maximum_packet=1200 |
| 84 | 4 | reserved=0 |
| 88 | 8 | A client id=1 |
| 96 | 8 | B client id=2 |

只有远端发送 Hello：包头 session 为零，payload role=Remote，local_nonce 非零，peer_nonce 为零。远端无需预先知道主机随机身份，client id 列表只是固定协议值。

主机首次收到 codec、build 和 nonce 均合法的 Hello 后锁定 IPv4 地址与源端口，回 First Ack。agreement 的 role=Authority、local_nonce 为主机 nonce、peer_nonce 为远端 nonce，两者必须不同。远端确认完整 agreement 和 Hello sequence 后，采用 Ack 包头的 session id 并进入 Established。build 必须完全一致，不作范围兼容协商。

`Worker::Start` 在创建线程前用 `BCryptGenRandom` 生成 session id 和主机 nonce，返回 `HostIdentity`。随机数失败不重试，Start 一次性对象不能再次启动。没有认证，适用于可信网络；对外界面和支持声明仍须按设计说明此边界。

### Input：16 字节前缀 + 1–4 条 64 字节命令

- +0：u16 count，范围 1–4；+2：u16 reserved=0；+4：u32 reserved=0；+8：u64 last_ack（已收到的主机 Ack 包序号）。
- +16 起是完整 V2 canonical 命令，顺序从旧到新；最后一条为 command，其余为最多三条 redundant。每条命令的 session/client 必须等于包头，sequence 严格递增，target_tick 为有效非零值。
- 每条命令内部布局：session[16]、client u64、sequence u64、target_tick u64、24 字节 V2 状态。状态含 version=2、size=24、held u32、edge u32、move_x/y 和 aim_x/y 四个 i16、reserved u32；codec 调用现有 `core::Valid/Encode/Decode`。
- 网络层不改 target，不做 admission、不推进 tick；`last_ack` 不能超出主机实际成功发出的 Ack 序号。远端 Warmup 时也要周期发送中性 Input 维持存活。

### Ack：184 字节

| 偏移 | 字节数 | 内容 |
| --- | --- | --- |
| 0 / 8 / 16 / 24 | 各 8 | completed tick / B client id / accepted sequence / processed sequence |
| 32 / 40 / 48 / 56 | 各 8 | processed_tick / 本次 processed_source / B generation / rejected_count |
| 64 | 4 | rejection_mask，按 core::Decision 的拒绝项位号，允许位 3–14 |
| 68 | 2 | core::SessionState 数值：Warmup=2、Active=3、Retiring=4、Retired=5、Failed=6 |
| 70 | 2 | purpose：First=1、Progress=2 |
| 72 | 8 | First 的 hello_sequence；Progress 为零 |
| 80 | 104 | 完整 agreement |

First 必须为零进度 Warmup，只由 Endpoint 生成。主线程只排入 Progress。accepted、processed 为 sequence 高水位；processed_source 为本次实际来源，缺输入时可为零。沿用 core 的事实：尚无命令被处理时，中性正常 tick 仍可使 processed_tick 非零，因此不把 processed=0 等同于 processed_tick=0。generation 在有 processed_tick 时必须非零。

后续 Ack 的 completed、accepted、processed、processed_tick、generation、rejected_count、session_state 和累计 rejection_mask 不回退。Worker 在主菜单 Warmup、暂停、无 TopExit 发布时，每 250 ms 重发最近的合法进度快照，使用新的包序号，completed 不变。这样不会用墙钟推进模拟，也不会让远端因主机没有 tick 而误判断线。

### Bye：16 字节

+0：u16 reason；+2：u16 reserved=0；+4：u32 reserved=0；+8：u64 last_ack。reason 为 Stop=1、PeerBye=2、PeerTimeout=3、PeerProtocol=4、RunTeardown=5、InternalFailure=6。Authority 的 last_ack=0，Remote 不得回显尚未发送的 Ack。收到任何合法 Bye 都按本地 PeerBye 结束，远端 reason 单独保留在状态中，不能冒充主机观察到的本地原因。

主动结束仅在传输已建立时发送 Bye：先固定真实结束原因，Prepare 一次，再对同一字节与同一 sequence 连续尝试 3 次 sendto。第一次成功发送后 CommitSent 一次，其余副本直接发送；前一次失败也继续后续尝试，全部失败时取消 pending。发送错误记入 send_errors/last_send_error，不覆盖 terminal/protocol_error/native_error。收到首份 Bye 后立即停止交付输入；原字节副本只能累计 duplicate，不回 Bye、不重开会话。Worker 在本次接收批次的剩余预算内读当前可读副本，不另开等待窗口；关闭 socket 后到达的包不再可观测，不能声称已被计数。

## 主线程接线合同（后续 game/peer）

1. `cs host` 对 Host/Solo 作互斥检查，在主线程创建新的 Worker 并调用 Start；默认绑定 `0.0.0.0:<port>`。把配置 `host.bind_address`、`host.peer_timeout_ms` 映射到 WorkerConfig；E2E 用 `127.0.0.1`。Start 在调用线程完成随机身份、WSAStartup、独占绑定、非阻塞设置及 CreateThread，失败逐项返回并释放自有资源。
2. Start 成功返回的 HostIdentity 用于后续 core Host Session；此处不增加 core::Host 或激活会话。Manager 入口读取 Status：Established 仅表示传输协商成功，不能代替 K/Join/A/B 绑定条件。
3. Manager 在 Authority-pre 之前有界调用 TryReceive；按顺序先处理冗余、最后处理当前命令，经原有 codec/Submit/queue 进入 Participant[1]。Warmup/阻断/绑定无效时不 admission，不能在之后重放边沿。ReceivedInput 的 received_ms/packet_sequence 可用于生命周期接收边界；重绑时须丢弃旧窗口已收到的积压，不能为旧包赋新 generation。墙钟不作 target tick。
4. TopExit 发布 completed 后，从 Participant[1] 的 ClientInputProgress 构造 Ack：高水位、当前 source、generation、拒绝数/掩码和 SessionState。QueueAck 只复制值；满时返回 false 并计数，不等待网络。必须在每次结束前先关闭主线程输入与 publication。
5. 每个 Manager 入口优先读取粘性 terminal。PeerBye/PeerTimeout/PeerProtocol 映射到后续 core 原因码，停止新 tick 并执行完整会话退休。TryReceive 在已请求停止或 terminal 非 None 时不再交付旧输入；不能仅靠 inbox 是否为空推断连接状态。
6. 退休的 B/K/在途 callback 顺序由 game 层完成，再调用 Stop(reason, wait_ms)，默认最多等待 1000 ms，所有请求上限 5000 ms。返回 TimedOut/WaitFailed 必须记录，不能声称线程已停止；仅 running=false 也不足以证明 Win32 线程句柄已 signaled，以 Stop 返回 Stopped 为准。允许后续再次观察/等待同一退出，但不复活旧会话。
7. Worker 析构只发信号、关闭持有的线程句柄，不等待，也不强杀线程。线程持有状态强引用；Stop 超时后仍可安全退出并由其关闭 socket/WSACleanup。它不持有 Worker、Session、Lua 或游戏对象指针。DLL 按项目要求常驻，不能卸载仍在执行的线程代码。不在 DllMain 中调用显式 Stop。

## 背压、错误与观测

- 网络线程独占 Endpoint/socket。收发各 256 项 SPSC，生产者使用 release 发布，消费者 acquire 读取；索引为静态确认无锁的 u32 原子。满时不覆盖旧值、不移动读游标，丢弃计数到 UINT32_MAX 饱和。没有在线 reset；每个新会话新建队列。
- 每轮最多收 64 包、发 64 份 Ack，select 最长 10 ms；不因 B 缺输入等待游戏或全员 ready。
- 第一份合法 Hello 才锁定来源；其余地址含畸形包在解析前丢弃并累计 foreign_packets，不能污染已锁定 Endpoint。握手前错误只丢弃，握手后唯一对端的协议错误以 PeerProtocol 结束；缓存副本只累计 duplicate_packets 并刷新存活，Stale 只累计 stale_packets 且不刷新存活。两者均不进入 errors 数组；不把正常 UDP 重复/迟到作为错误。
- Hello/First Ack 丢包时，远端可重发原字节 Hello；主机重发原 First Ack，不重复建立。无新鲜包的默认 3000 ms 超时在第一个合法 Hello 后开始，纯 Hosting 可以无限等待；没有重连、late join 或新来源接管。
- 本地主动 stop、超时和协议错误先保存真实终止原因，再尽力发送 3 份原字节 Bye；内部进度失败且传输仍已建立时也走此规则。Bye 的 sendto 失败不能覆盖原始原因。队列满也不能丢失 terminal。内部 socket/进度错误为 InternalFailure；UDP 背压与 ICMP 错误计数，断线仍按合法包存活判定。
- WorkerStatus 发布收发包/字节数、合法/重复/stale/外国来源计数、每种协议错误、socket 错误、双向队列丢弃、最后接收时间及 RTT 最近值/最大值/样本数。新增 bye_send_attempts、bye_sent_packets、last_send_error，用于区分三次发送尝试、成功发送数和最后发送错误；valid_packets 继续只统计交付成功及已识别副本，不含 stale。RTT 来自成功发送的 Ack 到 Input 回显的墙钟间隔，保存 64 项发送历史，包含远端发送等待；不冒充精确网络延迟。
- 各计数是独立原子快照，运行中不承诺同一瞬间相等；完整收发对齐由后续整体 E2E 结合远端包日志判定。本阶段尚无运行时观测，不声称 CRC/握手/超时/队列在实机上已经通过。

剩余工程为实现方案 3.2 起的 Host core、K/Join/consumer/lifecycle、game/peer、命令/配置/状态/trace、UI 和 Rust 客户端/harness。它们均未因本次 net 编写而完成。

## 2026-10-25 补充网络规则的同步记录

初版 net 编写后，[实现方案](tx-04-06-implementation-plan.md)第 5.1、5.2、5.4 节补充了丢包、重复与乱序规则（乱序规则已由用户确认）。用户随后要求据此修改代码；当前代码及上文已同步以下 Authority 改动。初版的不符行为仍在下表保留，不充当运行时失败记录；本次没有执行测试。

| 规则 | 初版行为 | 本次修改/剩余范围 |
| --- | --- | --- |
| 乱序旧包丢弃并计为 `stale`，不结束会话 | 不高于高水位的包返回 Error::Sequence，握手后会以 PeerProtocol 结束 | 新增 Error::Stale；通过身份校验后只丢弃/计数，不提交状态、刷新存活或交付内容。Hello/First Ack 走专用分支；更高序号的矛盾内容仍报错 |
| 重复与 stale 不算协议错误 | Duplicate 同时增加 errors[Duplicate] | CountError 排除 None/Duplicate/Stale；duplicate_packets 与 stale_packets 单独发布 |
| Bye 原字节连发 3 份 | BestEffortBye 只发一份，CommitSent 后即 Retired | 只 Prepare 一次，固定三次 SendPacket；第一次成功后仅 CommitSent 一次。任何 sendto 失败都继续尝试并保留原始原因；新增发送次数与错误诊断 |
| Bye 副本仅计数，不回 Bye | 首份 Bye 退休后 Receive 返回 Terminal，worker 即退出当前接收批次 | Endpoint 的 Retired 只识别已接受 Bye 的原字节副本；Worker 用剩余接收预算计数当前可读副本，不再交付其他内容，也不额外等待 |
| 远端每 250 ms 原字节重发 Hello、握手超时、First Ack 副本按字节判重复 | Worker 只有 Authority；Remote 每种 kind 只缓存最新一份，First Ack 副本晚于 Progress Ack 到达时会被判成序号错误 | 仍由 Rust 远端客户端实现（方案第 13.2 节）；本次未实现 Rust 客户端，也未增加 Remote 的 First Ack 专用缓存或定时重发 |

继续保留：主机对重传 Hello 原样重发 First Ack；异常 Hello 在握手前丢弃、握手后结束；主机超时从锁定对端开始计时。缓存为每种 kind 最新接受的一份，Hello 会一直保留；更久以前已被替换的 Input 无字节证据可供判 Duplicate，只能作为 stale 丢弃，仍不交付、不续命或断开。Remote 的 First Ack 缓存例外由 Rust 实现。
