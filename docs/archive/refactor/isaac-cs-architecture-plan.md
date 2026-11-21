# Isaac C/S 权威联机总体架构规划

> 2026-10-12 规范修订：执行要求先读[现行项目规范](project-conventions.md)。永不设定 gate，不校验哈希；每次 `tx-0x-0x` 推进只做一次最终整体验收，探讨过程不测试；逆向与测试可由同一智能体完成。本文的架构与功能目标继续适用，旧阶段门、哈希比较、独立角色和多轮测试条款不再生效。

## 文档状态

- 状态：**Draft / 总体规划指导**。
- 日期：2026-08-18。
- 当前实现阶段：尚未实现 C/S 联机；近期工作止于 T0 研究原型。
- 首版权威端：运行真实 `isaac-ng.exe` 的 Listen Server。
- 兼容原则：原版 Lockstep、Isaac Sync 1.0.4 自定义 Lockstep 扩展和本规划中的 C/S 模式相互隔离。

本文不是 [Isaac Sync 1.0.4 正式同步设计](../../custom-sync-design.md) 的修订版，也不改变现有 wire protocol、Lua API、IPC ABI、Hook 或发布身份。文中出现的 `NetworkMode`、`InputCmd`、`Snapshot`、`NetEntityId` 等均为未来接口草案，**当前代码尚未实现，也不是公开 API**。

本文使用三种证据标签：

- **已确认**：现有仓库文档中已有静态、动态或双机证据。
- **架构决定**：本规划固定的目标行为，后续实现不得自行改变。
- **待验证**：需要 T0 或更晚阶段逆向、实验后才能升级为实现契约。

## 1. 结论

首版采用“真实游戏进程作为权威模拟器”的 Listen Server：

1. 房主的 `isaac-ng.exe` 持有唯一权威世界，同时为房主渲染画面。
2. 房主输入和远端输入都先进入同一权威输入队列，再在确定的 server tick 应用。
3. 远端客户端不参与最终裁决，只维护权威世界的副本；早期不做预测，成熟后仅对有限表现做预测和校正。
4. 服务端不得因单个客户端输入缺失而停止全组 simulation tick。
5. C/S 模式不进入或复用原版 Online Manager 的逐帧 ready gate；原版 channel `0` 和现有 Isaac Sync channel `0xFD` 均不被 C/S 协议解释。
6. Dedicated Server 是后续演进形态：仍优先运行真实游戏本体，而不是重新实现 Isaac 的游戏规则。

只把 P2P full mesh 改成“所有输入先发给房主，再由房主转发”并不构成 C/S。如果各端仍需等待所有玩家同一帧输入、各自执行完整模拟并依赖确定性保持一致，它仍然是星形拓扑的 Lockstep。

## 2. 术语和模型边界

### 2.1 拓扑不等于同步模型

| 维度 | 可选模型 | 本规划选择 |
| --- | --- | --- |
| 网络拓扑 | full mesh、星形、专用服务器 | 首版星形 Listen Server |
| simulation 推进 | Lockstep、权威实时推进 | 权威实时推进 |
| 世界真相 | 每端各自模拟、单一权威端 | 单一权威端 |
| 客户端表现 | 无预测、插值、预测与回滚 | T1 无预测，之后逐步增加 |
| 服务器进程 | 房主游戏进程、独立游戏进程、规则重实现 | 首版房主游戏进程 |

### 2.2 角色定义

- **Authority Server**：拥有 run、房间、实体、RNG 和 gameplay 结果的最终决定权。
- **Local Client**：Listen Server 中房主的输入、相机、HUD 和本地表现角色；通过 loopback 与权威逻辑交互。
- **Remote Client**：发送原始意图，接收权威快照，在本地维护展示副本。
- **Replica World**：客户端上由权威 baseline、delta 和事件驱动的世界状态。
- **Prediction World**：客户端尚未得到服务端确认的临时表现；它不能覆盖权威状态。
- **Server Tick**：权威模拟单调递增的逻辑时标，不直接等同于墙钟时间、渲染帧或当前已知的 `onlineFrame`。

### 2.3 首版目标与非目标

长期目标是形成可扩展的服务端权威联机模型，但 T0 只建立其可行性证据。

长期目标：

- 原版 Lockstep 模式保持原行为。
- 单个慢客户端不再冻结所有玩家。
- 服务端裁决伤害、拾取、生成、房间、RNG 和 Mod gameplay 副作用。
- 客户端可以通过 baseline/delta 恢复，而不是依赖整个对局始终确定性一致。
- 后续可以把 Listen Server 的 Authority Adapter 移入隐藏的独立游戏进程。

当前非目标：

- 不重写 Isaac 引擎或完整游戏规则。
- T0 不交付可玩的 C/S 联机。
- T0 不实现实体快照写回、房间切换、预测、rollback、late join 或 Mod 兼容。
- 不把现有 `CustomFrameOpV2` 扩成通用 RPC 或状态复制容器。
- 不承诺 EOS、多本地玩家、动态 roster 或 Dedicated Server 的首版支持。

## 3. 现有证据基线

以下事实可以作为新研究的起点，但不能外推到尚未覆盖的 C/S 行为。

| 事实 | 状态 | C/S 意义 |
| --- | --- | --- |
| 当前目标为 Win32/x86 `isaac-ng.exe`，关键目标使用 RVA、机器码和目标镜像验证 | 已确认 | 新 Hook 继续按构建 fingerprint 和特征 fail closed |
| Steam backend `2` 使用 `SteamNetworking006`；原版发送、可用性查询、接收均固定 channel `0` | 已确认 | C/S 不修改或截获后改写 channel `0` 语义 |
| Isaac Sync channel `0xFD` 双机主动传输已通过逐包对应验证 | 已确认 | 证明 sidecar 可行，不代表 C/S 应继续占用 `0xFD` |
| 当前 Steam 后端的游戏 8 字节身份与 SteamID64 一致 | 已确认 | 可以作为初始 peer identity；重连和多本地玩家仍待验证 |
| `RVA 0x5110AB` 原调用前，原版输入已经应用且 `onlineFrame` 尚未递增 | 已确认 | 可作为已有 Lockstep 分发依据，不等同于完整 gameplay tick 边界 |
| 样本中的 simulation 推进频率约为 `29.676 Hz` | 已确认的样本值 | 新协议必须协商/观测 tick，不把 `30 Hz` 写死为跨版本 ABI |
| `lua_close` 是当前 Lua VM 退役的权威边界，游戏对象和 Lua 只在主线程访问 | 已确认 | C/S Lua 角色和资源必须遵守既有生命周期契约 |
| 原生 desync 目录包含 framestate、checksum 和 RNG history 诊断信息 | 已确认 | 可用于研究 canonical state；尚无可逆 snapshot/apply 证据 |

详细证据见：

- [自定义同步运行时分析](../../custom-sync-runtime-analysis.md)
- [onlineInputDelay 与输入帧链路](../../online-input-delay.md)
- [Isaac Hook Research](../../isaac-hook-research.md)
- [Lua State 生命周期契约](../../lua-state-lifecycle-design.md)
- [IssacOnlineSync 设计](../../issac-online-sync-design.md)

## 4. 总体组件架构

```text
IssacOnlineSync / future launcher
├─ 进程发现、构建校验、注入和日志
├─ Session Director：选择并冻结 NetworkMode
└─ Transport/diagnostic supervisor
       │ IPC / control
       ▼
isaac-ng.exe + Native
├─ 单一 Hook Owner
│  ├─ Original/Offline passthrough
│  ├─ Existing Lockstep adapter
│  └─ Authority C/S adapter（仅选定模式激活）
│
├─ CustomAuthorityServer
│  ├─ Server Clock / Tick Boundary
│  ├─ Local + Remote Input Scheduler
│  ├─ Virtual Input Device Adapter
│  ├─ Authority World Registry
│  ├─ Snapshot / Delta Replicator
│  ├─ Gameplay Lua Authority Gate
│  └─ State Hash / Replay / Diagnostics
│
└─ CustomAuthorityClient
   ├─ Local Input Publisher
   ├─ Server Clock Estimator
   ├─ Baseline / Delta Receiver
   ├─ Replica World Applier
   ├─ Interpolation / Prediction / Reconciliation
   ├─ Presentation Lua Gate
   └─ State Hash / Diagnostics
```

### 4.1 Listen Server 中的主机

首版主机不维护两份 Isaac 世界。房主看到的就是 Authority World，但房主输入不能绕过权威管线：

```text
Host input capture
    -> LoopbackTransport
    -> Input Scheduler
    -> target server tick
    -> Virtual/Local Input Apply
    -> one authoritative gameplay tick
    -> host render + remote state publication
```

Loopback 是逻辑边界，不要求实际 socket。它必须复用与远端输入相同的验证、sequence、目标 tick、去重、边沿动作和日志代码，避免出现“主机按键立即生效、远端按键排队”的两套语义。

Listen Server 的固有限制：

- 房主进程退出即失去 Authority World；首版不做 host migration。
- 房主没有网络传输损耗，仍存在天然延迟优势。
- 可以通过统一 `input_lead_ticks` 减少基础时序差异，但不能把 Listen Server 描述为竞技级完全公平。

### 4.2 Dedicated Server 演进

Dedicated Server 仍应优先由 Supervisor 启动真实 `isaac-ng.exe`，注入 Authority Adapter，并禁用或旁路渲染、音频和本地展示。届时房主也成为普通 Remote Client。

在以下条件同时满足前，不开始 Dedicated Server：

- Replica World 已能从完整 baseline 重建。
- 实体状态 apply、房间/楼层事务和 RNG 状态均可验证。
- Authority Adapter 不依赖窗口焦点、本地 controller 或 HUD 生命周期。
- 已解决 Steam 身份、进程许可、会话发现和资源隔离。

重新实现独立 Isaac 规则服务器不属于当前路线；它会要求复制所有角色、物品、敌人、房间生成、Lua Mod 和版本差异，风险远高于复用游戏本体。

## 5. 模式隔离

以下是未来概念类型，当前尚未实现：

```cpp
enum class NetworkMode : std::uint8_t {
    Offline = 0,
    OriginalOnline = 1,
    CustomAuthorityServer = 2,
    CustomAuthorityClient = 3,
};
```

### 5.1 模式不变量

1. `NetworkMode` 必须在对局建立前由唯一的 Session Director 冻结。
2. 同一 run generation 内不得在 `OriginalOnline` 和 `CustomAuthority*` 之间热切换。
3. 未知状态、冲突信号或无法证明的 manager baseline 一律保持 C/S disarmed。
4. Hook 可以进程级安装，但所有 wrapper 默认调用原函数；只有已冻结的 C/S 模式才能进入新逻辑。
5. Hook 安装、发布和回滚继续采用单一事务，不能由 Lockstep DLL 与 C/S DLL 分别 patch 相同 callsite。
6. `lua_close`、新 run、返回菜单或目标 manager 重建会退役当前 C/S generation；旧 session packet 只能作为 tombstone 丢弃。

### 5.2 各模式允许的行为

| 模式 | 原版 Online Manager | C/S transport | 权威输入队列 | 状态复制 |
| --- | --- | --- | --- | --- |
| `Offline` | 未激活 | 禁止 | 禁止 | 禁止 |
| `OriginalOnline` | 原样运行 | 禁止 | 禁止 | 禁止 |
| `CustomAuthorityServer` | 不进入/不依赖 | server endpoint | 激活 | 发布 |
| `CustomAuthorityClient` | 不进入/不依赖 | client endpoint | 仅发送本地输入 | 接收/apply |

现有 Isaac Sync 1.0.4 仍属于原版 Online Lockstep 上的独立 sidecar：它保留 `ISCS`、protocol 5、channel `0xFD`、full mesh、`FrameStore` 和 all-slot ready gate。C/S 实现不得复用这些 session 对象，也不得让同一个 run 同时进入现有 `ACTIVE` 和 `CustomAuthority*`。

### 5.3 “不干扰”的验收含义

`OriginalOnline` 模式必须满足：

- 不创建 C/S session、server tick、input scheduler 或 replica state。
- 不发送、读取或 drain C/S transport。
- 不更改 channel `0` 的参数、payload、次数或返回值。
- 不改变原版 ready stall、frame increment、Lua callback 和房间生命周期。
- C/S wrapper 的可观测结果等同于直接调用已验证的原函数。

如果将来 C/S 与现有 Isaac Sync 交付在同一 Native 中，这些条件仍不允许共享 session token、frame store、activation barrier 或 error state。

## 6. 状态所有权

### 6.1 权威状态

下列状态最终只由 Authority Server 决定：

- run seed、关卡和房间状态；
- 全部 gameplay RNG 状态和调用结果；
- 玩家身份、位置、速度、生命、状态、库存和资源；
- NPC、Tear、Projectile、Pickup、Familiar、Effect 等 gameplay 实体；
- 碰撞、伤害、死亡、拾取、生成、删除和 owner/parent 关系；
- 门、GridEntity、房间 clear、波次、Boss 和换层结果；
- gameplay Lua 回调产生的状态变化；
- pause、restart、disconnect 和 session lifecycle 的最终提交。

客户端发送的是输入意图，不发送“我已经拾取道具”“敌人已受伤”或“应生成某实体”之类结果。

### 6.2 客户端本地状态

下列状态可以保留在客户端，不进入权威 hash：

- 相机、窗口、HUD 布局和可访问性设置；
- 本地音量、纯展示音效实例；
- 不参与碰撞或 RNG 的粒子和装饰效果；
- 网络统计、调试 overlay 和输入设备 UI；
- 插值缓存和预测历史。

无法证明“纯展示”的状态默认按 gameplay 状态处理，不能因为看似是动画或 Effect 就直接排除。

### 6.3 可预测状态

预测不是新的真相来源。成熟阶段可以预测：

- 本地玩家移动表现；
- 瞄准方向、开火动画和不带 gameplay 副作用的即时反馈；
- 远端对象在两个已确认 snapshot 之间的插值位置。

早期不得预测或必须等待服务端确认：

- 伤害、死亡和复活；
- 拾取、交易、道具使用结果；
- 房间/楼层切换；
- 实体生成删除、RNG 结果和 Mod gameplay 操作。

Isaac 的射击、碰撞、掉落、熟悉物和 RNG 会迅速影响整个房间，因此“只回滚本地玩家”不能被假定为完整正确。只有全量或经过证明的回滚状态建立后，才允许扩大预测面。

## 7. Tick、输入与权威推进

### 7.1 Server Tick

`server_tick` 是每个 authority session 内从零或协商 baseline 单调递增的 `uint32`/`uint64` 逻辑值。具体宽度在正式 wire 设计时固定。它必须满足：

- 每次成功执行一个权威 gameplay update 恰好增加一次；
- pause、加载和菜单期间的行为由明确状态机定义；
- 不直接使用渲染 frame、QPC 或 `Game():GetFrameCount()` 替代；
- 溢出前结束 session，不允许静默回绕；
- 与原版 `onlineFrame` 分属不同命名空间。

现有样本约为 `29.676 Hz`，只能用于估算实验时长。T0 必须重新确认 offline/local co-op 中的 gameplay tick 边界和频率；正式握手发布观测或固定的 tick 配置。

### 7.2 输入调度

概念公式：

```text
target_tick = max(
    estimated_server_tick + negotiated_input_lead,
    last_accepted_target_tick + 1 when an edge command requires ordering
)
```

服务端对每名 client 维护：

- 下一个合法 input sequence；
- 最近接受和处理的 target tick；
- 当前连续输入状态；
- 已消费的边沿动作集合；
- 最后收到输入的 tick/单调时间；
- late、duplicate、future-window、invalid 统计。

同一 `(client_id, input_sequence)` 的重复包必须幂等。相同 identity 内容冲突属于协议错误。过早、过晚、越过 future window、未知 client 或非法输入值都不得影响权威世界。

### 7.3 缺失输入策略

服务端不能使用 all-client ready gate。每个 tick 到期时按以下策略生成有效输入：

- 移动和瞄准等连续量：短暂沿用最后已确认状态，达到 `max_input_age_ticks` 后归零。
- 按住射击：作为连续状态处理，但同样受最大年龄限制。
- 炸弹、主动道具、卡牌、药丸、丢弃等边沿动作：只消费唯一 sequence 中的上升沿；缺包时不触发，绝不重复上一上升沿。
- 客户端重新出现后，从新 sequence/tick 恢复，不补执行已过期的 gameplay 边沿。
- 超时达到 session 阈值后，进入明确的断线/AI/neutral policy；首版采用 neutral 并最终断开，不做 AI 接管。

具体 tick 数由 T0 fault-injection 数据决定，在正式协议前不得写成产品常量。

### 7.4 单个权威 Tick 的顺序

```text
1. Drain control/input within bounded budget
2. Validate and stage each client's commands
3. Resolve effective input for server_tick T
4. Apply local and remote inputs through the same adapter
5. Execute exactly one authoritative gameplay update
6. Capture spawn/update/remove events and canonical state
7. Advance server_tick to T+1
8. Publish ACK, state hash, delta or scheduled baseline
9. Perform presentation/render work outside authority mutation
```

T0 需要证明步骤 3–7 的真实引擎边界；在证明前，这只是目标顺序。

## 8. 传输与未来 wire 草案

### 8.1 Transport 抽象

C/S 状态机只依赖面向消息的 transport 接口，不直接依赖 `SteamNetworking006`：

```cpp
class IAuthorityTransport {
public:
    virtual SendResult Send(PeerId peer, Lane lane,
                            const void* bytes, std::size_t size) = 0;
    virtual DrainResult Drain(MessageConsumer& consumer,
                              std::uint32_t maximum_messages) = 0;
};
```

未来实现顺序：

1. `LoopbackTransport`：主机本地客户端和确定性单元测试。
2. `FaultInjectionTransport`：延迟、丢包、重复、乱序和带宽实验。
3. `LegacySteamP2PTransport`：仅在独立 channel 冲突扫描、身份和 session 接受路径完成后用于原型。
4. `SteamNetworkingSockets` 或等价现代 transport：生产候选。

`0xFC`、`0xFE` 等数值当前都不是已分配 channel。C/S channel 必须由独立实验确认，并进入 build fingerprint；不得直接占用原版 `0` 或现有 Isaac Sync `0xFD`。

### 8.2 逻辑 lane

| Lane | 可靠性 | 内容 | 规则 |
| --- | --- | --- | --- |
| Control | reliable ordered | 握手、roster、pause、room/floor transaction、disconnect | 必须幂等并有 session/generation |
| Input | unreliable sequenced | `InputCmd` 和最近若干输入冗余 | 新包覆盖旧包，边沿按 sequence 去重 |
| State | unreliable sequenced | snapshot delta、hash、处理进度 | 缺 baseline 时丢弃并请求恢复 |
| Recovery | reliable bounded | 完整 baseline 分块、重同步提交 | 分块有总长、索引、hash 和上限 |

不同 lane 不依赖跨 lane 到达顺序；需要顺序的业务条件必须显式携带 tick、baseline sequence 或 transaction id。

### 8.3 概念消息

以下结构只表达必需信息，布局、大小、字节序和最终名称尚未冻结：

```cpp
struct ClientHelloDraft {
    BuildHash game_and_native_build;
    ManifestHash mod_manifest;
    SessionNonce client_nonce;
    std::uint32_t protocol_version;
    std::uint32_t capabilities;
    ClientRole requested_role;
};

struct InputCmdDraft {
    SessionId session;
    ClientId client;
    std::uint64_t input_sequence;
    std::uint64_t target_server_tick;
    InputStateDraft state;
    std::uint64_t last_received_snapshot;
};

struct SnapshotDraft {
    SessionId session;
    std::uint64_t server_tick;
    std::uint64_t snapshot_sequence;
    std::uint64_t baseline_sequence;
    std::array<std::uint64_t, kMaxClients> last_processed_input;
    WorldHash world_hash;
    EntityDeltaDraft deltas[];
};

struct NetEntityIdDraft {
    std::uint32_t spawn_sequence;
    std::uint16_t generation;
    std::uint16_t authority_domain;
};
```

正式 wire 必须继续采用当前工程已经建立的防御习惯：严格长度、版本、kind、flags、reserved、finite float、canonical zero、sequence、session、上限和 CRC/MAC 校验；未知值 fail closed。不能传输游戏指针、Lua closure 或进程内对象内存。

### 8.4 T0 最小协议

2026-10-12 修订：下面的 `STATE_HASH`/`WorldHash` 名称保留为原协议草案资料，不要求生成或校验哈希，也不作为当前必需消息。后续观察按实际字段和 capture phase 直接记录差异；实验协议不构成项目 gate。

T0 只需要 `HELLO`、`INPUT`、`ACK`、`STATE_HASH`：

- `HELLO` 证明双方 build、实验 protocol、role 和能力一致。
- `INPUT` 验证统一 input scheduler、去重和 future tick。
- `ACK` 回报 server tick、最后接受/处理的 input sequence 和 rejection。
- `STATE_HASH` 按明确的 capture phase 对齐并定位首次分歧。

T0 不发送实体 delta 或完整 baseline，不能被称为可玩的 Replica World。

## 9. 状态复制与客户端校正路线

### 9.1 Canonical State

任何状态进入 snapshot 前都必须具备：

- 稳定网络 identity，不使用裸指针或临时 vector index；
- 明确的 capture phase；
- 明确的 create/update/remove 生命周期；
- 字段级规范化、上限和版本；
- 可重复的排序和 hash；
- `Capture -> Serialize -> Apply -> Recapture` 验证。

原生 framestate 是重要研究入口，但当前只证明诊断输出和 checksum 存在。除非找到并验证反向 apply 语义，不能把它直接称为可恢复 snapshot。

### 9.2 Baseline 与 Delta

T1 之后的状态复制采用：

- 完整 baseline：run/floor/room、玩家、实体、RNG 和必要全局状态的自包含快照。
- delta：相对一个明确 baseline/snapshot sequence 的 create/update/remove 集合。
- 周期 baseline：限制 delta 链长度并支持恢复。
- world hash：覆盖 authoritative gameplay state，不覆盖纯展示状态。

客户端只有完整接收、完成结构与范围检查并在安全 apply 边界提交 baseline 后，才能把它设为当前副本。分块缺失或 apply 中途失败时必须丢弃候选 baseline，不能留下半套世界；按现行规范不校验哈希。

### 9.3 插值、预测和 Reconciliation

推荐演进：

1. **T1 无预测**：客户端只显示已确认 snapshot，先验证权威状态写回。
2. **远端插值**：对非本地控制实体，在两个确认 snapshot 之间做展示插值。
3. **有限本地预测**：保存输入历史，预测本地移动表现。
4. **Reconciliation**：snapshot 携带 `last_processed_input`；客户端回到权威本地状态并重放未确认输入。
5. **受控 rollback**：只有确定了完整依赖状态、Lua/RNG 和副作用撤销语义后才启用。

小误差平滑和大误差 hard snap 的阈值属于后续实验参数，不进入 T0。

## 10. 房间、楼层和低频事务屏障

逐帧推进不能等待所有客户端，但切房、换层、重开和 late join 可以使用低频可靠事务：

```text
Authority prepares transaction N
    -> freeze new gameplay mutations at a safe boundary
    -> publish transition parameters / target baseline
    -> clients load or stage replica resources
    -> clients ACK staging result
    -> authority commits transaction N
    -> publish full baseline and resume server ticks
```

这不是逐帧 Lockstep，因为 barrier 只保护不可部分提交的生命周期边界。服务端必须对超时客户端执行断开或恢复策略，不能无限等待。

首版支持策略：

- T0：不实现事务，只观测相关边界。
- T1：限制在单房间，不切房。
- 后续：先实现切房 baseline，再实现换层、重开和 reconnect。

## 11. Lua 与 Mod 策略

### 11.1 权威执行原则

普通 Isaac Mod 默认假设每个进程执行完整 gameplay Lua。C/S 中若服务端和客户端都运行同一副作用回调，客户端会额外生成实体、推进 RNG 或改变房间状态。因此长期策略是：

- 服务端执行 gameplay Lua 回调并把结果纳入 snapshot/event。
- 客户端只执行明确声明为 presentation-only 的逻辑。
- 网络感知 Mod 通过未来 role API 判断 Authority/Replica。
- 无法分类的回调默认只允许 Authority 执行。

透明兼容任意既有 Mod 不是首版目标。

### 11.2 Manifest 与能力

沿用现有 canonical manifest/build fingerprint 思路，但 C/S 使用独立 domain/version。握手至少覆盖：

- 游戏目标镜像和 Native build；
- C/S protocol/replication schema；
- Mod ID、版本、资源和网络能力；
- Authority/Replica Lua 能力；
- tick、输入、snapshot 和 entity schema 能力。

任一 gameplay Mod 不一致时必须在启动 Authority World 前失败。表现 Mod 是否允许不一致，需要未来通过明确 capability 声明，不能仅凭名称猜测。

### 11.3 Lua 生命周期

现有 `lua_close` generation 契约继续适用：

- Lua VM 退役会终止对应 Authority/Replica generation。
- registry refs、handler、manifest、prediction history 和 Lua-derived snapshot schema 都是 generation-scoped。
- ACTIVE C/S session 不热接续到新 VM；必须建立新 baseline/session。
- Lua 调用仍只在已确认的游戏主线程和安全 callback 边界发生。

## 12. 生命周期与失败策略

| 情况 | 首版行为 |
| --- | --- |
| 房主进程退出 | 全 session 终止，不迁移 authority |
| 远端短时缺包 | 服务端按缺失输入策略继续推进 |
| 远端超时 | 先 neutral，达到阈值后断开 |
| 重复/迟到 input | 幂等丢弃并计数，不回写历史 |
| snapshot baseline 缺失 | 客户端丢弃 delta，请求恢复 |
| build/manifest 不一致 | 启动前 fail closed |
| 未知游戏版本/Hook 特征冲突 | C/S 不激活；原路径保持可调用 |
| Lua VM 重建 | 退役 session，要求新 baseline |
| entity apply 失败 | 客户端停止 replica，要求完整恢复；不得半继续 |
| 可观测权威字段分歧 | 直接记录字段与首次分歧窗口；不比较或校验 world hash |

网络线程只负责 bounded receive、decode 和入队，不直接访问游戏对象或 Lua。Authority simulation、state capture 和 apply 全部在已验证的主线程边界执行。

## 13. 现有能力的复用边界

### 13.1 可以复用的机制

- PE/`.text` build fingerprint、RVA 和机器码校验。
- 事务式 Hook 安装、回滚和第三方冲突检测。
- Steam identity 观测和 transport 抽象的部分实现经验。
- canonical manifest、严格 decoder、sequence、session tombstone 和 fail-closed 原则。
- Lua VM generation、`lua_close` 退役和主线程约束。
- IPC snapshot/event ring、JSONL、日志导出和离线 analyzer 框架。
- fault-injection、双端日志配对和 desync 首次分歧分析方法。

### 13.2 不得直接复用的 Lockstep 语义

- `GroupSessionProtocolEngine` 的 full-mesh/所有成员提交模型。
- `FrameStore` 的 every-sender/every-frame ready 条件。
- 为每名 sender 每帧生成 no-op 的推进规则。
- `activation_frame` all-slot barrier 作为日常 tick gate。
- 按 sender slot 顺序在每端重复执行 gameplay handler。
- 将 `onlineFrame` 直接作为新的 server tick。
- `CustomFrameOpV2` 的单 operation/数值 payload 作为状态复制格式。

可以复用代码层的 CRC、hash、日志或 transport 工具，但必须位于新的 Authority namespace/session 中，不能共享可变状态。

## 14. 分阶段路线

| 阶段 | 目标 | 明确不包含 |
| --- | --- | --- |
| T0 研究原型 | 模式隔离、tick/input/entity 探针、统一 scheduler、最小协议、按 tick 状态 hash | 可玩、snapshot apply、预测 |
| T1 单房副本 | 固定角色/无 Mod，实体 create/remove，全量房间 baseline，无预测客户端 | 切房、库存完整兼容、late join |
| T2 连续复制 | entity delta、周期 baseline、远端插值和恢复 | 完整 rollback |
| T3 本地预测 | 本地移动预测、input ACK、reconciliation、受控 hard correction | 任意 gameplay 预测 |
| T4 完整 run | 房间/楼层事务、玩家库存、RNG、死亡复活、受支持 Lua Mod | Dedicated/host migration |
| T5 Dedicated | 独立隐藏游戏实例、所有玩家远端化、部署和资源隔离 | 规则重实现 |

按实际功能依赖和用户任务推进，不设置阶段 gate。每次推进仅在整个目标完成后进行一次整体验收；结果如实记录，未完成字段、失败与未知项明确说明，不用哈希校验或修改结果制造完成结论。

## 15. 主要风险登记

| 风险 | 影响 | 首要缓解 |
| --- | --- | --- |
| 无完整可写回状态 | 无法建立 Replica World | 优先逆向实体 lifecycle 和 apply，不先扩 wire |
| 客户端无法只运行展示逻辑 | 副本继续自行产生 gameplay 副作用 | T1 先冻结/旁路 gameplay update，验证 render 分离 |
| RNG 分散在全局、实体、Lua 和房间系统 | snapshot 后立即再次分叉 | 建立 RNG inventory 和逐调用/逐对象证据 |
| 普通 Mod 同时在客户端执行 | 重复生成和不可重放副作用 | Authority/Presentation role gate，首版禁用未知 Mod |
| 本地多人/虚拟设备 ABI 不稳定 | 服务端无法模拟远端玩家 | T0 单独研究 local co-op，不借用 Online Manager |
| Listen Server 主机优势和退出 | 公平性、可用性受限 | 统一 input lead；长期迁移 Dedicated Server |
| 旧 Steam API/空闲 channel 不稳定 | 连接或兼容风险 | transport 抽象、冲突扫描、现代接口演进 |
| 游戏更新改变布局和 callsite | 错写游戏状态 | fingerprint、严格特征、事务安装、版本白名单 |
| snapshot 带宽和实体数量 | 延迟、队列积压 | 当前房间 interest、delta、周期 baseline、上限 |
| 预测依赖范围被低估 | 反复校正或不可回滚 | T1 无预测，逐类扩大并用 fault matrix 验收 |

## 16. 已冻结决定与开放问题

### 16.1 已冻结决定

- 首版是 Listen Server，真实游戏进程负责 Authority simulation。
- 房主输入走与远端相同的 scheduler 语义。
- 服务端日常 tick 不使用 all-client ready gate。
- Custom C/S 从 local/offline run 建立，不依赖原版 Online Manager。
- 原版、现有 Isaac Sync Lockstep 和 C/S 使用独立 session/protocol/state。
- T0 只研究和验证，不交付可玩状态复制。
- Dedicated Server 在 Replica World 成熟后实施。

### 16.2 截至 T0-04 已回答的部分

- fixed target 上的 top loop、update/render owner、physical poll、steady-standard Authority transaction、room-change、pause、loading、menu 和 device-switch 包络已取得可重放证据。
- host input 已能 canonicalize、排队、在目标 Authority tick exactly-once apply，并保持 reject-without-world-change、owner generation 和完整恢复。
- Offline、OriginalOnline 和 Custom Authority 实验路径已按模式信号、generation 和零干扰规则隔离；T0-04 final gate 为 `PASS`。
- 上述结论只覆盖 single-Player 与 steady-standard active apply，不等于完整 local co-op、全部 gameplay mutation 或状态复制。

完整结论和禁止外推项见 [T0-04 逆向交接](cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md)。

### 16.3 T0-05 及以后仍需回答

- 如何通过游戏自身 local co-op 路径创建远端玩家对应的第二 Player，并绑定 controller/HUD/index/ordinal？
- 是否需要 virtual input device；若需要，其 owner、vtable、generation、加入/退出和恢复 ABI 是什么？
- 能否在客户端阻止 gameplay mutation 而继续渲染、网络和 UI？
- 哪个实体容器和 lifecycle Hook 能形成稳定、无指针的 entity identity？
- 原生 framestate/checksum 的生成函数覆盖哪些字段，是否存在可复用 apply 路径？
- gameplay RNG 对象的完整集合和生命周期是什么？
- 旧 Steam P2P 在不进入原版 online lobby 时如何完成身份、接受和 session 生命周期？

T0-05 的研究内容见 [T0-05 专属研究计划](cs-t0/t0-05/00-overview/isaac-cs-t0-05-research-plan.md)；T0 全阶段目标见 [Isaac C/S T0 研究原型详细任务书](cs-t0/isaac-cs-t0-task-plan.md)。两者的执行流程统一服从[现行项目规范](project-conventions.md)，不设 gate、不校验哈希、每次推进只做一次最终整体验收。
