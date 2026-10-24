# tx-03-01：T0-09 双机双进程与网络故障矩阵设计

> 2026-10-21 当前补充：[tx-03-08 C 工作设计](tx-03-08-c-design.md)细化 C 的实施与用户决定。C 的唯一整体验收为 F0 10,000 tick、F1–F5 各单 seed/5,000 tick；逐场由用户重启双端准备，统一 runner 保持一次调用；复用普通 Room 完整采样，odd-only 保留 gap，普通样本零遗漏。下文 §10/§12 的 C 采样与阶段安排按该设计细化；完整多 seed 矩阵仍属 D，本文设计时点的未实现描述不覆盖后续 B 实际结果。

日期：2026-10-20（Asia/Shanghai）。状态：**DESIGN_COMPLETE / REVIEW_PASSED，等待用户复核；尚未实现、未运行游戏或测试。**

本设计依据 [现行项目规范](../../project-conventions.md)、[T0-08 PASS 结果](../t0-08/tx-02-04-results.md)、[T0-09 任务目标](../isaac-cs-t0-task-plan.md#12-t0-09双进程与网络故障矩阵) 以及本会话用户确认的下述决策。旧任务计划中与现行规范冲突的 C2、gate、哈希类流程不再执行。

## 1. 用户已确认的决策

| 编号 | 决策 |
| --- | --- |
| D1 | 使用两台 Windows 设备：host 本机 + 运行在 host 上的 Windows VM。两端各运行一份相同 build 的真实 `isaac-ng.exe`，加载相同 Native/Helper。 |
| D2 | 使用独立的本机/virtual-network UDP 数据面，host 侧固定 seed impairment relay；不使用 Steam、原版 Online、Isaac Sync、channel 0/0xFD 或 `ISCS`。 |
| D3 | 保留完整 F0–F5 目标，但分阶段实现和执行；最终 T0-09 PASS 只由完整矩阵产生。 |
| D4 | A/B 矩阵输入使用双端确定性脚本输入；两端 canonical bytes 相同，均经共同 codec/queue/apply。 |
| D5 | VM 端 Remote Client 运行完整 observer-simulator：本地重放 A/B 输入，只有 B 输入同时经 wire 发往 Authority；用于同 tick 状态比较，不应用权威 snapshot。 |
| D6 | VM 端原始 artifact 通过 VM 共享文件夹回流到 host artifact 根目录。 |
| D7 | 删除 C2。C0/C1 在双机环境中新鲜执行；C0 不安装 T0 Native，C1 安装 Native 但 disarmed，C3 为完整 custom Authority/Remote。 |
| D8 | 遵照现行规范：不设 gate，不校验哈希，不要求独立子智能体；实机前明确授权和隔离环境。 |

## 2. 目标、范围与非目标

### 2.1 目标

在一个 host Authority/Local Client 进程和一个 VM Remote Client/observer-simulator 进程之间验证：

- 模式隔离和安装/disarmed/active 边界；
- T0 scheduler 与 canonical 输入通过独立 wire 的 exactly-once/幂等/迟到过期语义；
- 固定 seed 的 UDP delay/jitter/loss/duplicate/reorder/outage；
- Authority 在 Remote input 缺失时继续 tick；
- 双端逐输入/逐 commit 的 packet/ACK/apply 关联；
- 有限窗口的 first-divergence 定位；
- 双端正常 retire、Hook 恢复、DLL 常驻和同一游戏进程存活；
- 为 T0-10 提供真实双机证据，而非把单进程逻辑 Remote 外推成网络/双游戏完成。

### 2.2 非目标

- 不进入或读取原版 Online/Isaac Sync，不验证原版 channel 0；C2 已按现行规范删除。
- 不实现 Steam transport、远程房间发现、NAT 穿透或互联网联机。
- 不实现 snapshot/baseline/delta apply、rollback、late join、Replica World 或可玩双端同步。
- 不承诺完整世界相等；状态比较限定为本设计第 10 节的最小 canonical subset。
- 不把 F0 的“配置 impairment 为零”解释为物理网络零延迟或零抖动。
- 不计算或校验任何文件、构建、状态或 artifact 哈希。

## 3. 现有基线与差距

### 3.1 可继承的既有能力

- T0-08 wire v2：ICT8 magic、64 字节 header、1200 字节包上限、严格 decoder/CRC、四类消息、HELLO/首 ACK、输入冗余和 ACK/STATE 独立 lane。
- T0-07 Authority tick pipeline：A/B 共同准入、sequence/target/generation、缺失输入中性、edge exactly-once。
- T0-05/T0-06：owned virtual B、精确 pair/绑定、Player/世界最小观测和主线程边界。
- T0-08 E2E：单真实 Authority + 同进程逻辑 Remote，已 33/33 PASS；只能作为协议语义前置，不等于双进程/网络完成。

### 3.2 必须新增/重构的差距

| 差距 | 当前事实 | T0-09 需要 |
| --- | --- | --- |
| Endpoint 分离 | `ProtocolLink` 同时持有 `authority_`、`remote_`、`FaultInjectionTransport` | 独立 Authority/Remote endpoint 驱动；每进程只持一端 |
| 跨进程数据面 | 无 OS socket；transport 明确不调用 network | UDP datagram、host relay、双向发送/接收、有界队列 |
| 双游戏编排 | `accept-protocol-e2e.py` 和 isaac-test MCP 要求唯一 `isaac-ng.exe` | 按 role/PID/creation time 管理 host/VM 两个游戏进程 |
| 控制面 | 单进程 controller 用 stdin/stdout/共享 mapping | 可靠的 host orchestrator ↔ A/B controller 控制通道、屏障、心跳、stop |
| VM 端 observer | 当前 B 是 Authority 进程内 owned virtual device | VM 上本地 co-op observer-simulator，重放 A/B 脚本并采样本地状态 |
| 网络故障 | 进程内 xorshift logical-tick fault transport | host relay 的真实时间 microsecond 调度、固定 seed、双向独立决策流 |
| 状态比较 | T0-08 `STATE_HASH` 为 partial/gap；无跨进程 first-divergence | 双端最小 canonical subset、observation gap、first-diff 分类 |
| C0/C1 | 旧计划单端点/历史证据 | 双机新鲜 C0/C1；C0 需要独立只读外部观察路径 |
| Artifact | 单 artifact 根、单进程 capture | host/vm/relay/control 四类日志、共享文件夹原子 create-new、资源预算 |

## 4. 拓扑和组件

```text
┌──────────────────────────── host (Windows) ────────────────────────────┐
│  Game A: isaac-ng.exe  (Authority + Local Client)                      │
│    Native A: Authority runtime, local A authoritative pipeline         │
│    Controller A: inject/helper, wire IPC, UDP socket, control client   │
│       │ loopback UDP data                      │ TCP control           │
│       ▼                                        ▼                       │
│  Relay: t0-udp-relay (fixed seed impairment)   Orchestrator            │
│       │ host virtual-adapter UDP               │ shared-folder reader   │
└───────┼─────────────────────────────────────────┼───────────────────────┘
        │                                         │
        │ VM virtual network                      │ VM shared folder
        ▼                                         ▼
┌──────────────────────────── Windows VM ────────────────────────────────┐
│  Game B: isaac-ng.exe  (Remote Client + observer-simulator)            │
│    Native B: local co-op replay, local state observation, remote wire  │
│    Controller B: wire IPC, UDP socket, control client, artifact writer  │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.1 组件职责

- **Game A（host）**：真实 Authority；A 为 ordinal0 本地输入，B 为 authoritative virtual Remote（owned virtual device）；不等待 B 日常输入。
- **Game B（VM）**：真实游戏进程和 observer-simulator；本地重放 A/B 脚本，生成同 tick 的本地状态观察；把 B canonical input 的副本经 UDP 发出；不应用 A 的 authoritative snapshot，不把 ACK/STATE 写回本地世界。
- **Controller A/B**：每个 OS 各一个 owned controller。负责注入/正常 stop、wire IPC、UDP socket、control 连接、artifact 收尾；网络/worker 不写游戏对象、Lua、Player 或 canonical queue。
- **Host orchestrator**：生成唯一 run manifest 和脚本，启动/停止两端、控制 relay、收集 artifact、等待双端 tick 完成、调用 analyzer；不直接写游戏世界。
- **Relay**：只转发原始 ICT8 wire bytes；按固定 seed 注入 delay/jitter/loss/duplicate/reorder/outage；记录每包接收/决策/排程/转发/丢弃。不解析或修改 canonical input，不创建 session。
- **VM shared folder**：只用于 artifact 和启动时 manifest/脚本回流；不作为实时数据面或 Authority tick 时钟。

### 4.2 网络端点和绑定

- Relay 在 host 上运行两个明确的数据监听面：
  - A 面：`127.0.0.1` 的专用 UDP 端口；
  - B 面：host 虚拟网卡地址的专用 UDP 端口，VM 通过该地址访问。
- 数据面均为已 `connect` 的 UDP socket；单 datagram 必须恰好携带一个 64..1200 字节 wire packet，不分片、不拼接、不修改字节。
- Control plane 是独立 TCP 连接，使用独立 magic/version 和 JSONL 有界消息；绑定仅 loopback/VM 地址，不暴露公网。
- 所有真机操作限定 host 与 VM 的专用虚拟网络；不使用 Steam 网络、原版 Online、channel 0/0xFD 或 Isaac Sync。

## 5. Run manifest、session 和启动屏障

### 5.1 Manifest

每个 case/seed 生成一个新 manifest，至少包含：

- `run_id`、case、implied endpoint role、build label、wire version、input/state schema、T0 capabilities；
- session id（16 字节非零）、A/B nonce（不同、新鲜）、A/B client id（A=0、B=1）；
- host/VM 游戏 PID、creation time、HWND、角色；
- 两端脚本 id、绝对 target tick 列表、canonical bytes 来源；
- relay 监听面、fault profile/seed、矩阵重复编号；
- control plane 地址和 artifact 根/角色子目录；
- 用户预置的 safe run、角色、seed、room、只有 A 等前置确认；
- 不包含实际构建/状态哈希。

Manifest 只对本次 run 有效；旧 session/nonce/generation 不复活。VM 通过共享文件夹读取 manifest，host orchestrator 保留原始 invocation 私有副本。

### 5.2 启动屏障

1. 两端 controller 完成游戏发现、Native/Helper 注入（如适用）、wire IPC 建立和 artifact 目录 create-new。
2. 两端加载 manifest/脚本，校验角色、build label、schema、session/nonce、目标 tick 范围。
3. 两端通过 control plane 发 `READY`，包含游戏 PID/creation time、Native 状态、脚本已载入、artifact 可写。
4. Host orchestrator 验证双方 ready 与 manifest 一致后，配置 relay 进入 bootstrap（不作 F1–F5 impairment）。
5. 两端执行 HELLO/首 ACK；只有双方 Established 且 exact pair/observer 本地准备完成才发 `ARMED`。
6. Orchestrator 收到 A/B `ARMED` 后发送一次 `RUN_START`。A 的 Authority tick0 和 B 的 observer tick0 从该屏障开始；不发生自动重试。
7. 不满足任一条件则记录失败，不进入矩阵 case，不伪造后续 tick。

## 6. Wire、IPC 和进程边界

### 6.1 Wire

- 继续使用 T0-08 ICT8 wire v2 语义和严格 decoder；跨进程 transport 只负责原样搬包。
- HELLO/首 ACK 在 bootstrap 阶段使用有界应用层重传；F1–F5 故障只在双方 Established 后生效。
- Established 不等于 Native Authority active 或 exact pair ready；启动屏障单独核对游戏侧状态。
- 每个 datagram 只有一个 packet；长度、magic、version、CRC、session、role、sender、sequence 由接收端原有 wire decoder 校验。Relay 的日志不替代接收端校验。

### 6.2 Wire IPC v1（新增，不改写现有 v6 前缀）

- 新增独立命名 mapping：`Local\IsaacCsT0.Wire.<run-nonce>.<A|B>`。
- Header：magic/version/size、run id、role、session、produced/consumed/overwritten、closed/error、输入/输出 ring 计数。
- 每个方向有界 128 槽；每槽 1248 字节（published/token/direction/generation/tick/length/packet bytes），沿用 T0-08 稳定槽复制/立即发布 consumed 的规则。
- 路径：
  - A：Native Authority 产出 ACK/STATE packet → wire IPC out → Controller A → UDP；Controller A 从 UDP 收 INPUT → wire IPC in → Native 主线程解码/准入。
  - B：Native observer 产出 B input packet → wire IPC out → Controller B → UDP；Controller B 从 UDP 收 ACK/STATE → wire IPC in → Native 主线程观察/记录。
- Controller worker 只做 socket、复制和共享 ring；Native main thread 才触碰 Player、canonical queue、scheduler、world 和 Lua。
- Ring 满、slot 损坏、session/generation 不符、超时或关闭状态一律 fail closed；不丢包后假装完成。
- 旧 T0-08 IPC v6 保持可用；T0-09 不改变其尺寸或前缀。

## 7. Control plane

独立于 ICT8 数据面，建议使用 TCP JSONL，magic `T9C1`，消息有 version、run_id、role、sequence、ack 和有限长度上限：

- `READY`：进程/Native/脚本/artifact 状态。
- `RUN_START`：屏障、seed、tick0、script id、fault profile。
- `TICK_HEARTBEAT`：Authority 当前 tick、基本健康状态；用于 B 按 lead 提前发送 B 输入，不作为游戏状态证据。
- `ARM_FAULTS` / `FAULT_TRIGGER`：host relay 与两端同步 F1–F5 的开始/outage。
- `STOP_REQUEST`、`STOP_COMPLETE`、`RETIRE_COMPLETE`：正常退出和错误退出。
- `ERROR`：带 role/case/tick 和原始错误。

Control plane 不承载 canonical input、world state、Player/device/Lua 指针或 Steam identity。控制连接中断视为失败，不自动重连重试运行。

## 8. UDP relay 和故障模型

### 8.1 数据面

Relay 从 A/B 面接收 datagram，按配置方向原始转发：

- A→B：Authority 发布 ACK/STATE；
- B→A：Remote 发送 INPUT。

每个包记录 `received`、`decision`、`enqueue`、`forward`/`drop`、`closed`；记录接收/排程时间（QPC monotonic）、双向 ordinal、packet length/kind/sender/sequence/session 的前缀身份。relay 不重算 CRC、不修改 payload，不生成或合并包。

### 8.2 固定 seed 决策

- profile 包含 base delay ms、jitter ms、loss ppm、duplicate ppm、reorder ppm、reorder extra ms、outage 触发/持续时间。
- 每个方向有独立 xorshift64* 决策流：seed 由 case seed 与方向常量派生，包 ordinal 在方向内单调；分析器可按相同输入重算 expected decision。
- 每个 packet 的故障决策在 `received` 时确定一次，后续重排/重发不改写决策；副本保留原 wire identity。
- 延迟以 relay monotonic time 计，不以渲染帧或游戏墙钟作为 Authority tick。
- 任何 duplicate/reorder 只发生在数据面；HELLO/首 ACK bootstrap 不受 F0–F5 impairment。
- 网络环境自身造成的 VM/NAT/宿主调度延迟必须记录为测量值；F0 的“0ms”只表示没有 relay 配置延迟，不声称物理零延迟。

### 8.3 故障 profile

| Case | base / jitter | loss | duplicate | reorder | outage | 最短观察 |
| --- | --- | --- | --- | --- | --- | --- |
| F0 | 0 ms / 0 ms | 0% | 0% | 0% | 无 | 连续 10,000 Authority tick |
| F1 | 25 ms / 10 ms | 0% | 0% | 0% | 无 | 5,000 tick |
| F2 | 50 ms / 20 ms | 1% | 1% | 1% | 无 | 5,000 tick |
| F3 | 100 ms / 40 ms | 5% | 2% | 2% | 无 | 5,000 tick |
| F4 | 50 ms / 20 ms | 0% | 10% | 10% | 无 | 5,000 tick |
| F5 | 50 ms / 20 ms | 0% | 0% | 0% | Authority tick2000 起约 2 s burst | outage 前 1,000 tick + 后 1,000 tick |

- 非 baseline case 至少 3 个不同 impairment seed；每个 seed 的单次 final matrix invocation 必须完整保存一份 manifest/relay/两端 artifact。
- F5 的 outage 由 host orchestrator 在观察到 Authority tick2000 后通过 control plane 触发；relay 记录实际开始/结束时间和 tick 范围；运行不得在后 1,000 tick 前 stop。
- 具体数值是实现参数，不是产品协议常量；最终报告必须记录实际值和不确定性。

## 9. Deterministic input 和 observer-simulator

### 9.1 共享脚本

Host orchestrator 生成唯一脚本，至少覆盖任务计划中的：

1. 300 tick neutral warmup；
2. A 单独四向移动和停止；
3. B 单独四向移动和停止；
4. A/B 同时相反方向移动；
5. 连续瞄准/射击 hold 和 release；
6. 可安全复现的 edge，每个 identity 最多一次；
7. B 静默 1/5/30/120 tick；
8. B 恢复后合法新 sequence，不补发旧 edge；
9. 300 tick cooldown。

每一步包含绝对 target tick、ordinal、V2 canonical state/edge、步型。矩阵输入不走物理 OS 键盘；不绕过 A/B 共同 codec、validator、queue、dedupe、Resolve 或 apply。脚本原件和 canonical bytes 同时保存在 host 和 VM artifact。

### 9.2 Endpoint A

- Ordinal0 脚本通过 Native 本地 Authority input source 进入共同流水线。
- Ordinal1 脚本只在 B 端发送时生成 wire INPUT；A 端从 UDP→wire IPC 接收，经同一 decoder/admission 后进入 authoritative virtual B。
- A 不直接读取共享文件夹中的脚本内容来绕过 wire；脚本是否合法仍由 endpoint admission 判定。

### 9.3 Endpoint B observer-simulator

- B 在 VM 上运行同 seed、同角色、同 room 的本地 co-op 观察模拟；本地 A/B 输入均按共享脚本 target tick 重放，使用现有 owned virtual B 和共同 canonical 路径。
- 只有 ordinal1 的 canonical bytes 副本进入 wire 发送路径；本地 observer 与 wire 发送不得重复 apply 同一份 B 输入。
- B 本地状态只用于同 tick 观察，不是 authoritative state；不把 ACK/STATE apply 到本地世界，不把本地状态回写给 A。
- B 本地脚本 apply 与 wire 发送使用同一个 target tick 表；B controller 依据 control plane tick heartbeat 和配置 lead 提前发送，而不是依据屏幕帧或 render 墙钟。
- 若 B 本地 tick 速率落后导致 wire target 过期，记录为 `remote-late`，不重放、不滑动 edge；若本地 observer 尚未到达比较 tick，等待/记录完整超时，不伪造状态。

### 9.4 tick 对齐约束

- 两个游戏从 `RUN_START` 后各自 tick0；脚本以绝对 tick 作为逻辑时间。
- B 的本地 observer 可以在不同墙钟到达同一 tick；分析器按 tick 比较，不要求两端 render/frame 同步。
- wire 发送必须使用 Authority tick heartbeat 与输入 lead；Authority 日常 tick 不等待 B。
- 如果两端 tick rate 偏差使输入超过 future/late 窗口，记录实际偏差和拒绝原因，不自动重试或补发。

## 10. 状态观察和 first-divergence

### 10.1 最小 canonical subset

仅使用既有 T0-05/T0-06/T0-07 已定义且可读的字段，按 tick 记录：

- authority tick、run/room/attempt token、capture phase；
- ordinal（0=A、1=B）与稳定 client id；
- 角色/类型中已确认可用的身份字段；
- Player 位置 `x/y`（仅当 T0-07 现有 current-player witness 有效）；
- 每个 ordinal 的 accepted/processed input sequence、target tick、apply/resolve 计数；
- 当前 STATE 包的 gap/partial 原因和采集 seq。

明确不包含 `Player*`、device/control pointer、vptr、平台 handle、完整 raw world、未初始化字段或不可比 entity pointer。`STATE_HASH` 仍是历史 wire 消息名；T0-09 不计算文件/构建/世界哈希。

### 10.2 同 tick 比较

- A 在 Authority commit 后按同一 token 产出 observation frame；B 在本地 observer tick 后产出同 tick 的 observer frame。
- 仅比较双方均为完整来源的字段；单端 gap/partial 记为 `OBSERVATION_GAP`，不是世界分歧。
- F0：最小 canonical subset 应逐 tick一致；只要出现未知 mismatch，T0-09 F0 失败。
- F1–F5：允许由 Remote 输入 loss/late/apply 或已知未复制状态导致的分歧，但必须能定位到有限 tick/input/field 窗口。
- first-divergence 分类：
  - `INPUT_LATE_OR_MISSING`：wire target/apply/processed 差异；
  - `APPLY_EFFECT`：input identity 相同但 Player 字段差异；
  - `UNCOPIED_STATE`：已知 non-minimal 实体/环境差异；
  - `OBSERVATION_GAP`：单端未采集完整字段；
  - `UNKNOWN`：不可解释，整体失败。
- 每个 case 保存 first-divergence 的前后有限 tick 窗口、双端 frame、双端 packet/relay 因果链和重放入口；不允许只写“checksum mismatch”。

## 11. C0/C1/C3 控制设计

### 11.1 C0：未安装 T0 Native

- Host/VM 各启动唯一安全离线 run，只保留本地 A，不使用 owned virtual B，不安装 T0 Native/Helper，不启动 relay/UDP 数据面。
- 使用独立的只读外部 observer（不注入、不写游戏）记录有限的进程/游戏可观察状态；输入使用短、确定性的 OS 外部序列，仅用于基线稳定性，不冒充 canonical 脚本。
- 断言：无 T0 Native/Helper 注入、无 C/S packet/session/input、无 relay 数据、游戏进程健康且外部观察无明显停摆。
- C0 不声称与 C3 完整同状态或同 canonical 输入；只作为无 T0 注入的模式/行为基线。

### 11.2 C1：Native 安装但 disarmed

- 双机均安装同 build Native/Helper，但 custom Authority/Remote 模式保持 disarmed；不建立 C/S session、不写 canonical、不启动数据面发送。
- 使用短 OS 外部输入序列和只读 observer 对比 C0 的外部可观察行为；检查 T0 hooks 不打开输入/publication，恢复记录完整。
- 断言：C/S packet=0、T0 synthetic input=0、world mutation 不可归因于 T0；不得出现“已安装但仍在写”的旁路。

### 11.3 C3：完整 custom Authority + Remote

- 即本设计第 4–10 节的完整双机双进程拓扑和 F0–F5 矩阵。
- C3 的 A/B canonical 输入必须来自共享确定性脚本；A/T0 本地输入与 B wire 输入都经过生产 codec/queue/admission。
- C3 的 observer-simulator 仅作非权威状态比较，不应用 snapshot。

## 12. 矩阵执行和分阶段计划

### 12.1 完整目标

- F0：1 个 baseline run，连续 10,000 Authority tick。
- F1–F5：每个 case ≥3 个不同 impairment seed，每个 target 5,000 tick；F5 额外保证 outage 前后各 ≥1,000 tick。
- C0/C1：双机各新鲜 run；C3 为主矩阵。
- 最终 T0-09 结论由**完整矩阵的统一 runner 结果**产生；未运行的 case/seed 不得记为通过。

### 12.2 分阶段实现与验收

不设置 gate；以下阶段只是实现里程碑，每个阶段在完整目标完成时使用一个统一入口验收，失败不自动重跑。

1. **P1 传输与双端启动**：独立 endpoint、wire IPC、UDP relay、control plane、双机双进程注入/控制、F0 最小 runner。目标是在实现完成后一次统一 F0 E2E；不提前 smoke/probe。
2. **P2 observer 与状态窗口**：VM 完整 observer-simulator、共享脚本、最小 observation frame、first-divergence、F1–F5 relay 和报告。
3. **P3 模式对照与完整矩阵**：C0/C1 双机新鲜对照、完整 F0–F5 多 seed、全量 analyzer、资源预算和最终 artifacts。

P1/P2 的专项通过不代表 T0-09 PASS；T0-09 PASS 只由 P3 完整矩阵报告产生。每阶段失败原样保留，后续实质修改作为新的推进。

## 13. Artifact、资源和隐私

### 13.1 目录

```text
artifacts/cs-t0/tx-03-xx-e2e-xx/
  manifest.json
  invocation-private.json
  host/                         # A events, wire, controller, restore
  vm/                           # B events, wire, observer state, restore
  relay/                        # UDP receive/decision/schedule/forward logs
  control/                      # control-plane JSONL
  scripts/                      # shared input scripts + canonical bytes
  state/                        # A/B minimal frames, gaps, first-divergence
  analyzer/                     # per-case summary, failures, indices
  sources/ build/ toolchain/    # source snapshot and pure build records
```

- VM 端写入共享文件夹的 `vm/` 子目录；host 在运行中只读轮询/在 stop 后收尾，避免 VM 写入与 analyzer 并发。
- 所有 create-new；不覆盖历史失败。失败 run 至少保留到首个失败点、stop/restore 结果和原始控制/relay 证据。
- 资源预算：完整矩阵至少 16 个 case run、双端 85,000+ Authority tick；必须使用流式 NDJSON、过滤/滚动成功事件、逐 case 目录，避免把双端全量 render/loop 事件无界堆积。矩阵 runner 必须先估算磁盘并 fail closed，不能因空间不足静默丢证据。

### 13.2 隐私

- 公共 manifest 去身份化：不保存完整 SteamID、用户名、绝对 Steam 路径、peer identity 或原始 handle。
- 进程身份只保留 role、PID、creation time、build label、限定路径 basename；详细 invocation 放在私有 artifact，不进入公共报告。
- Relay/control 日志不保存系统密钥、Steam 身份或无关进程信息。

## 14. Stop、失败和恢复

1. 正常路径：orchestrator 先发送 `STOP_REQUEST` 和最后 tick 边界；A/B 停止新 publication/input，等待在途 wire/IPC/controller 处理，relay 进入 closed 并 flush，controller 落盘，再执行 Native 统一正常 stop 和 70 槽逐项目标恢复。
2. 任一端失败：记录原始错误和已完成/未完成 case；同一 run 只执行一次正常 stop 收尾，不自动重跑；不盲杀游戏、不强制卸载常驻 DLL。
3. Control/relay/UDP/共享文件夹失败：保留两端可落盘状态；不能让失败清理覆盖最初错误。
4. 两端必须分别报告 Native error、restored target 列表、DLL resident、同一游戏 PID/creation time 存活；只有实际恢复的槽计入通过。
5. 任意旧 generation/session 的 packet、worker 在 stop 后到达的 packet、wire IPC 满或 control 断连均不得使已结束 session 复活。

## 15. T0-09 不变量和 PASS 判据

### 15.1 不变量

- Authority server tick 只跟随成功 gameplay tick，不因 B 输入缺失、wire loss 或 relay outage 冻结。
- A/B 输入从脚本/canonical bytes 到 accepted/processed/applied 的 identity 可完整关联。
- 同 identity 最多 apply 一次；edge 计数为 0 或 1，不因 duplicate/reorder 重复。
- duplicate/reorder/loss/outage 的实际计数必须与 relay 决策和接收端结果对应；不允许未知包消失或凭空产生。
- C0/C1 不出现 C/S packet、synthetic input 或 T0 可归因世界 mutation。
- C2 按现行规范不存在；不对 Original Online/Isaac Sync 做任何操作或“未触碰即通过”登记。
- 正常/失败退役均恢复每端实际安装的目标槽并保持 DLL 常驻；不得把未恢复写成通过。
- 任何状态差异必须有有限 tick/input/field 窗口和明确分类；不能只报“mismatch”。

### 15.2 完整 PASS 条件

仅在完整矩阵均满足时判定 T0-09 PASS：

- F0 连续 10,000 Authority tick，无 ready stall、event gap、重复 edge、tick/input/apply 身份缺失；
- F1–F5 每个指定 seed 的 actual fault counts、Remote missing policy、A 持续 tick、B 恢复和 edge exactly-once 可解释；
- 所有 case 的 packet/ACK/STATE/apply 逐项配对和 first-divergence 可定位；
- C0/C1 双机新鲜对照无 C/S 旁路或 T0 可归因 mutation；
- 双端正常 stop/restore/进程健康；
- 所有产物、失败和未覆盖项透明保留，无 hash/gate、无自动重跑。

## 16. 未知项与必须先解决的研究问题

以下为设计接受时必须显式保留的 UNKNOWN，不允许在实现/验收时静默假设：

1. Windows VM 中第二份同 build 游戏是否能稳定加载 approved unpacked PE、Native/Helper 和独立 owned virtual B，且不触发 Online/Isaac Sync 或持久副作用。
2. A/B 两端能否从同一 seed/角色/room 进入可比初始状态；owned virtual B 的生成身份、位置和世界 RNG 是否足以让 F0 最小 subset 一致。
3. VM observer-simulator 的本地 A/B 重放是否能在不把 ACK/STATE 写入本地世界的前提下与 Authority 同 tick 对齐；tick rate 偏差与 input lead 的实际边界。
4. `TICK_HEARTBEAT` 的粒度能否让 B 在 F1–F5 的 delay/loss 下按时发送 target 输入；是否需要更高频 control 或提前 lookahead。
5. VM 网络路径自身带来的延迟、抖动和 NAT 重排是否污染 F0；基线测量和分离方法。
6. 共享文件夹的原子性、延迟和断线行为是否支持 create-new artifact 和 VM 回流。
7. C0 无 Native 的外部只读观察能否提供足够稳定的基线；C1 disarmed 的 hook/恢复范围如何界定。
8. 完整矩阵的双端事件/协议/wire IPC 存储量是否在磁盘预算内；需要哪些过滤/滚动/压缩（不使用哈希）策略。
9. 双游戏进程中如何防止同一 Steam/游戏持久状态相互影响（save/achievement/unlock/统计、共享配置和窗口焦点）。
10. Host/VM 时钟、QPC 和 real-time delay 对 fault replay 的影响；能否用 relay 的 due time 和实际到达时间给出确定性解释。

这些未知项必须在对应实现阶段用实际代码/运行观察收束；设计阶段不将其写成已完成事实。

## 17. 实施工作分解

| 工作包 | 内容 | 交付 |
| --- | --- | --- |
| T0-09-A | 本设计与 failure modes、manifest/control/wire IPC schema 冻结 | 设计文稿、静态审阅 |
| T0-09-B | 独立 Authority/Remote endpoint、wire IPC v1、UDP host relay、control plane、双进程启动/输入/停止 | P1 F0 可执行 runner、artifact |
| T0-09-C | VM observer-simulator、共享确定性脚本、minimal state frame、first-divergence、F1–F5 | P2 故障矩阵、双端状态报告 |
| T0-09-D | C0/C1 新鲜控制、C3 完整矩阵、多 seed、资源/隐私/恢复收尾 | 完整 T0-09 PASS 报告或真实失败记录 |

## 18. 附录 A：manifest 草案

实现时冻结严格 schema；以下是设计级字段，不包含真实 SteamID、用户名或绝对游戏路径。

```json
{
  "magic": "T9M1",
  "version": 1,
  "run_id": "tx-03-xx-F0-seed131080-01",
  "case": "F0",
  "repeat": 1,
  "build_label": "same-approved-build",
  "wire": {"version": 2, "max_packet": 1200, "session": "32hex", "authority_nonce": "32hex", "remote_nonce": "32hex"},
  "endpoints": {
    "a": {"os": "host", "role": "authority_local", "pid": 0, "creation_time_100ns": "0", "hwnd": "0x0", "artifact_subdir": "host"},
    "b": {"os": "vm", "role": "remote_observer", "pid": 0, "creation_time_100ns": "0", "hwnd": "0x0", "artifact_subdir": "vm"}
  },
  "script": {"id": "tx03-matrix-v1", "path": "scripts/input.json", "first_tick": 0, "last_tick": 5000},
  "relay": {
    "a": {"bind": "127.0.0.1", "port": 0},
    "b": {"bind": "host-vm-address", "port": 0},
    "faults": {"base_delay_ms": 0, "jitter_ms": 0, "loss_ppm": 0, "duplicate_ppm": 0, "reorder_ppm": 0, "reorder_extra_ms": 20, "outage_trigger_tick": null, "outage_ms": 0, "seed": 131080}
  },
  "control": {"host": "host-vm-address", "port": 0},
  "artifact_root": "artifacts/cs-t0/tx-03-xx-e2e-xx",
  "user_preconditions": {"safe_run": true, "only_a": true, "no_original_online": true}
}
```

## 附录 B：control plane envelope

- TCP JSONL；单条最大 64 KiB；UTF-8；严格字段/类型/长度校验。
- 公共字段：`magic=T9C1`、`version`、`run_id`、`seq`、`ack`、`role`、`type`、`body`。
- 允许的消息：`READY`、`RUN_START`、`TICK_HEARTBEAT`、`ARM_FAULTS`、`FAULT_TRIGGER`、`STOP_REQUEST`、`STOP_COMPLETE`、`RETIRE_COMPLETE`、`ERROR`。
- 旧 `run_id`/`seq`、未知 role/type、超长 body、断连后的重放均拒绝；控制面不承载 canonical input、world state、Player/device/Lua 指针或 Steam 身份。

## 附录 C：Wire IPC v1 布局草案

- 独立命名 mapping，不修改 T0-08 IPC v6。
- Header 至少包含：magic `T9WI`、version=1、size、run nonce、role、session、in/out produced、in/out consumed、in/out overwritten、closed、error、reserved。
- 每个方向 128 个稳定槽；每槽 1248 字节：published、token、direction、generation、tick、length、packet[1200]。
- publisher 写完整槽后 release-store `published`；consumer 复制稳定槽后 release-store consumed，再在本地解析/落盘。槽满、覆盖计数非零、role/session 不符或关闭后数据都 fail closed。

## 附录 D：共享脚本 step 草案

```json
{"tick": 300, "ordinal": 0, "purpose": "a_move_left", "state": {"version": 2, "encoded_length": 24, "held_flags": 0, "edge_flags": 0, "move_x": -32767, "move_y": 0, "aim_x": 0, "aim_y": 0, "reserved": 0}, "wire_send": false, "observer_replay": true}
```

- A 的 ordinal0 由 A 本地 source 和 B observer replay 使用；B 的 ordinal1 在所有 case 中 `wire_send=true`，B observer replay 使用同一 canonical bytes。
- 绝对 target tick 表由 host 生成并复制到两端；不允许运行时补发、滑动或重排已有 step。

## 附录 E：ObservationFrame 草案

```json
{"tick": 1000, "side": "authority", "run": 1, "room": 1, "attempt": 1, "capture_seq": 1000, "ordinal": 1, "client_id": 2, "character": "player-type-id", "position": {"x": 0.0, "y": 0.0, "valid": true}, "accepted_sequence": 0, "processed_sequence": 0, "processed_tick": 0, "target_tick": 0, "apply_count": 0, "gap_reason": null}
```

只有 `valid=true` 且来源完整的字段进入比较；缺失/partial 一律显式 gap。

## 附录 F：first-divergence 记录

```json
{"tick": 1000, "field": "position.x", "authority_value": 0.0, "observer_value": 1.5, "classification": "APPLY_EFFECT", "input_window": [992, 1000], "packet_tokens": [1, 2], "authority_frame": "state/host/1000.json", "observer_frame": "state/vm/1000.json", "relay_slice": "relay/1000-1000.jsonl"}
```

`classification` 只允许设计第 10.2 节列出的有限集合；`UNKNOWN` 视为失败，不写为通过。

## 引用

- [现行项目规范](../../project-conventions.md)
- [T0-09 任务目标](../isaac-cs-t0-task-plan.md#12-t0-09双进程与网络故障矩阵)
- [T0-08 PASS 结果](../t0-08/tx-02-04-results.md)
- [T0-07 PASS 状态](../t0-07/STATUS.md)
- [T0-05 owned virtual device 契约](../t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)
- [T0-06 采样边界](../t0-06/tx-01-66-readonly-sampling.md)
- [本设计失败方式](tx-03-01-failure-modes.md)
