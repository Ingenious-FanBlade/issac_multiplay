# tx-04-06：3.2 core/ 实现记录与接线约定

后续记录（2026-10-25）：[game 主线程适配与必要补丁](tx-04-06-game.md)已编写并纯编译、链接成功；Host 命令等尚未接入，没有部署或运行测试。下文保留 core 阶段原始交付记录。

日期：2026-10-25。范围为[实现方案](tx-04-06-implementation-plan.md)第 3.2 节，仍属于 tx-04-06 同一次推进。已编写 Host 会话扩展和统一双参与者流水线；纯构建结果见文末。本阶段没有新增或运行单元测试、独立 harness、网络程序或游戏；唯一整体 E2E 留待 tx-04-06 全目标完成后执行。

## 文件与行为

| 文件 | 本次改动 |
| --- | --- |
| `mod/src/core/session.h/.cpp` | 新增 `SessionMode::Host`，返回两个参与者；启动校验双 client 身份；首次激活要求 A/B 同时绑定；新增三个对端退休原因和名称 |
| `mod/src/core/tick_pipeline.h/.cpp` | 沿用模式对应的参与者遍历与统一 admission；移除序号冲突收尾中的 `ordinal==0` 限制，A/B 走同一套中止逻辑；说明双参与者 receipt 的含义 |

`Participant`、V2 codec、queue/dedupe/Resolve 和 TickClock 不需要另建 B 版本。core 仍只依赖 C++20 标准库，不引入 Winsock、网络 worker、REPENTOGON 或游戏对象读取。新功能和核心接口均有中文注释。

## 会话身份与激活

- `SessionMode` 追加 Host=2；None=0、Solo=1 不变。`ParticipantCount()` 对 Host 返回 2，对 Solo 返回 1，对未启动对象返回 0；`HasParticipant` 与 `MatchesIdentity` 因而同时覆盖固定 ordinal 0=A、1=B。
- `Start(identity, Host)` 要求 session id 非全零、generation 非零、两个 client id 非零且互异。Solo 仍要求 `clients[1]==0`。未知 mode、非法身份进入 `Failed/Configuration`，不发布部分身份；一次性 Session 不重启、不替换旧身份。
- core 不硬编码网络 client id；后续桥接层使用 Worker 返回的 `HostIdentity`（当前协议固定 A=1、B=2），另外提供新的本地会话 generation。
- 监听与握手由外层管理。Host 的 `Start` 在主线程确认 Worker 为 Established 后调用，直接进入 `Warmup/Prepared`；原有 `Negotiating` 枚举只保留历史数值，不恢复旧控制器流程。纯 Hosting 期间不能调用 core 激活或接管 A。
- Host 在 Warmup 中 `Activate(host_bound, remote_bound)` 要求两者为真；缺任一绑定仍为 Warmup。Solo 保留仅要求 A 的调用方式。Active 后重复 Activate 幂等返回成功，首次双绑定要求不变成逐 tick 等待；后续失效与中性仍由既有 Participant 和 game 生命周期处理。

## 输入、进度与冲突

主线程为两个 Participant 使用相同的运行配置，分别以 ordinal 0/1、对应 client id 和同一个 `Session*` 初始化。A 的采样与 B 的网络命令均须经过原有 canonical codec、`AcceptInput`、Submit、queue/dedupe 与 Resolve；本次没有为 B 新增队列实现、直接 current 写入或世界写入路径。

流水线继续按 `ParticipantCount()` 遍历两个参与者，检查各自归属与时钟 future window；同一 Authority candidate 同时 Begin/Resolve，正常原生返回同时 End，在 TopExit 构造两个 receipt。Host 的 B 即使缺输入也为 `present=true`，并按 `max_input_age_ticks=0` 立即中性；Solo 的第二项保持全零、`present=false`。普通重复、过期和目标窗口拒绝继续由原有 Decision/计数处理，不因正常 UDP 重复或 stale 而新增失败路径。

两种流水线 `AcceptInput` 重载现在共用与 ordinal 无关的序号冲突收尾：同 sequence 不同 payload 时，Participant 按原逻辑退休自身，流水线随后 Abort，本次时钟关闭、Session 为 `Failed/RuntimeFailure`，不再留下 A 继续模拟的窗口。该路径属于尚在 Draining 的输入冲突，不是正常对端断开。后续桥接若在 Manager 入口直接调用 Participant admission，也必须观察返回的 SequenceConflict 并结束整个会话，不能忽略后继续启动 tick。

## 对端原因与有序退休

`SessionCause` 在原有 CleanupComplete=12 之后追加 PeerBye=13、PeerTimeout=14、PeerProtocol=15；历史原因数值、SessionState 数值和 Ack 线格式保持不变，ControllerLost/RestoreFailure 继续保留不用。

后续 game/peer 将 Worker 的终止原因逐项映射到对应 `SessionCause`，通过主线程 `BeginRetirement` 结束 Host；不能把不同枚举直接强转。三个 Peer 原因只被 Host 接纳，Solo 拒绝；`Fail` 仍只接纳原有内部失败原因。进入 Retiring 后，新的输入、sequence 与 tick 关闭，已开始的模拟窗口仍可完成正常返回、previous 提交与 TopExit 发布。

Retiring/Retired 上重复退休请求幂等，不覆盖已经记录的退休原因。沿用既有 core 语义，`CompleteRetirement(true)` 的最终状态是 `Retired/CleanupComplete`；game/trace 应在开始退休时记录原因，Worker 的粘性 terminal 继续保存实际网络原因，不能把 CleanupComplete 当成 Bye 的原因。`CompleteRetirement(false)` 不完成退休，内部 Failed 也不被清理覆盖。K/B 的移除、callback 排空、网络停止与新的 generation 由后续 game owner 执行，core 不代办这些原生操作。

## 后续接线与构建

1. 主线程启动 Worker，持有 HostIdentity；握手前保持原生透传，状态为外层 Hosting。
2. 确认 Established 后，以新的本地 generation 构造 Host Session 与两个 Participant，配置相同的输入参数，进入 Warmup；Warmup 不分配 sequence、不 Resolve、不推进时钟。
3. game 在 run 内创建 K、执行 Join、确认 A/B 的唯一拓扑并建立各自 exact pair；稳定 Authority 入口使用两个真实绑定条件 Activate。
4. Manager 入口有界取网络命令，仅将 B 身份投给 ordinal 1 的统一 admission；绑定失效/阻断窗口的旧积压不得赋予新 generation 后重放。TopExit 将 B 的既有进度复制为 Ack。
5. 发现 Peer 终止时停止新输入与 tick，按实现方案第 9.4 节完成在途窗口、K/B 与网络的退休，再完成 core 退休；下一会话重新构造全部一次性对象。

Win32 / v143 Release 的 core、net 静态库及现有 DLL 纯编译、链接成功，原始输出见[构建记录](../../artifacts/repentogon/tx-04-06-core/README.md)。core 编译没有 warning；net 仍有原有的三条 SPSC 对齐填充 C4324 提示。本节不表示第 3.3 节 game/peer/K/Join、Host 命令、状态/trace/UI 或 Rust 远端已经接入；这些仍是 tx-04-06 剩余目标。
