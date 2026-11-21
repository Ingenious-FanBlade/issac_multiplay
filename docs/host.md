# Host 端设计

Host 是房主进程。它运行唯一的权威游戏，接收远端玩家的输入，并把世界状态发给客机。本文说明会话怎样建立、每个 tick 怎样处理输入、远端玩家怎样进入原生游戏，以及采样结果怎样经后台线程和网络线程发出。

采样的读取细节见[采样](capture.md)，线格式与确认见[同步](synchronization.md)。

## 1. 职责

| 职责 | 主要位置 |
| --- | --- |
| 会话与大厅：建立会话、接纳 Peer、选角、开局 | `mod/src/core/session.*`、`mod/src/game/authority/lobby.*`、`mod/src/ops/command.cpp` |
| 权威 tick：绑定参与者、消费输入、驱动原生更新 | `mod/src/game/authority/tick.*`、`mod/src/core/tick_*` |
| 输入：本机物理输入、远端输入、统一输入查询 | `mod/src/game/input/`、`mod/src/core/input_core.*` |
| 远端玩家：虚拟设备 K、原生 Join、生命状态、退休 | `mod/src/game/participants/` |
| 采样调度 | `mod/src/game/observation/world_bridge.*`、`binary_entities.*` |
| 发布与网络 | `mod/src/ops/world_commands.cpp`、`mod/src/net/worker.*`、`mod/src/net/live.*` |
| 持久窗口：死亡、复活、换层、Continue | `mod/src/game/authority/persistence.*`、`lifecycle.*` |

## 2. 会话

### 2.1 启动

界面 Mod 通过 `Isaac.ExecuteCommand` 下发命令，主要有：

| 命令 | 作用 |
| --- | --- |
| `cs solo` | 单人会话（无网络），用于本机验证权威 tick |
| `cs host` | 以 Host 身份开始会话，监听端口（默认 27090） |
| `cs lobby_host` | 在主菜单开大厅，等待客机加入后再开局 |
| `cs lobby_role <角色>`、`cs lobby_ready <0/1>`、`cs lobby_start` | 选角、准备、开局 |
| `cs stop` | 结束会话 |
| `cs status`、`cs lobby_status`、`cs stream` | 查询状态 |

命令只排队，在下一次最外层 `Manager::Update` 入口统一执行，不在 Lua 回调里直接改状态。

### 2.2 会话对象

`core::Session` 每次启动都重新构造，带新的 session id 和 generation：

```text
Dormant ──启动──▶ Warmup ──Host 绑定完成──▶ Active ──停止或失败──▶ Retiring ──清理完成──▶ Retired
```

- 参与者 ordinal：0 是房主，1—3 是远端。ordinal 在会话内冻结，有人离开时留空位，不压缩他人序号，也不复用退休身份。
- Host 可以先单人运行，Peer 随后独立握手加入。日常 tick 不等待全员。
- 会话代次（generation）独立于参与者的重绑代次，不进入输入编码。
- 只有 `Status()` 可以跨线程读取；其余变更全部在主线程。

### 2.3 大厅

`game/authority/lobby.cpp`：

- 选角只在等待阶段可改，按房主当前存档的解锁情况刷新可选角色。客机选了房主未解锁的角色时不算准备。
- 开局前先冻结名单与角色，再请求原生加载。加载期间迟到的握手不加入本局。
- 开局前要求采集入口已经参与，否则原生开局时的出生和音频事件会早于采集。

## 3. 权威 tick

### 3.1 时钟

以撒的 `Manager::Update` 每帧调用一次。原生 `Game::Update` 在普通帧更新整个世界（约 30Hz），在奇数帧只更新玩家（Odd 帧，玩家约 60Hz）。`core/tick_clock.*` 只接纳主线程中已分类、本轮唯一的模拟入口：

- **StandardRoom**：普通 `Game::Update`；
- **OddPlayer**：Odd 帧的玩家更新，由 `hooks/odd_patch.cpp` 在普通调用点标记。

### 3.2 流程

`game/authority/tick.cpp` 是 Authority 前段的唯一入口：

1. **绑定与激活**：读取参与者拓扑，确认每个参与者对应的原生身体。
2. **drain**：从各 Peer 的输入队列有界取出命令。各 Peer 轮转起点，一方洪水不能挤占他人的预算。
3. **Submit 与 Resolve**：把本 tick 的输入定为每个参与者的 current。缺输入时立即使用中性输入，不等待远端。
4. **原生更新**：放行原生 `Game::Update`。原生输入消费者查询输入时，由 Hook 返回对应参与者的统一输入。
5. **后态**：原生正常返回后进行采样（第 5 节），然后提交 tick。

模拟一旦真实完成，任何采集或发布故障都不会撤销它；采集失败只影响该帧的同步。

### 3.3 统一输入

所有参与者使用同一套输入语义（`core/input_core.*` 的 `InputStateDraftV2`）：

- 房主的物理输入由 `game/input/input_a.cpp` 采样，进入同一条统一队列。
- 远端输入由网络线程收下，主线程复制后经 `game/participants/peer.cpp` 分类，丢弃过期或越界的命令并按原因计数。
- `player` 组的 Hook（移动、射击、炸弹、主动道具、卡牌与药丸、丢弃）拦截原生输入消费者，按当前身体所属的参与者返回输入。`game/input/consumers.cpp` 按冻结的 ordinal 识别身体。
- 新代次尚无命令时，射击短暂保持暂停（最多 6 个 tick），避免把握手空档变成松开武器。

## 4. 远端玩家

### 4.1 虚拟设备 K

原生 co-op 需要一个输入设备才能创建玩家。`game/participants/device_k.cpp` 为每个远端参与者注册一个自有的输入设备 K：

- K 在原生 InputManager 中表现为普通非联机设备，原生的 `dynamic_cast` 依赖正确的 RTTI，因此 K 自带完整的 RTTI 表。
- K 的 13 个虚函数槽由本项目实现，返回该参与者的统一输入。
- 设备记录在首次原生构造前发布，进程内常驻，不复用、不释放。

### 4.2 原生 Join

`game/participants/join.cpp` 走原生 co-op 加入路径创建远端玩家的身体：

- `join` 组的字节补丁在原生 Join 判定处只改变“由哪个设备加入、以什么角色加入”，不写存档或角色表。
- 首次 Join 建立生命记录和归属；之后的重绑都走同一 ordinal。
- 一个参与者（控制组）可以拥有多个原生身体，例如 Jacob 与 Esau、Forgotten 与灵魂。`game/participants/topology.*` 维护参与者与身体的关系，冲突时整组输入置为中性，不截断。

### 4.3 生命状态与退休

- `game/participants/remote_life.cpp` 跟踪远端玩家的死亡、鬼魂化和复活。地址相同的 Player 上发生死亡或复活，也要使旧的归属失效。
- 单个 Peer 退休时先关闭它的输入，等 Manager 尾部排空原生回调后，再移除设备和身体。
- 原生移除按真实索引升序进行，先让原生处理 twin 关系，避免重复 Remove。
- 读取暂时失败只让该远端保持中性，不当作“已被原生移除”。

## 5. 采样调度

采样挂在权威 tick 的后态上，详见[采样](capture.md)第 4 节：

```text
Manager 入口      world_bridge: FrameBegin
原生 Update 返回  WorldPreparePostState → BinaryEntities::Prepare
                    普通帧：采 World；Odd 帧：采对应身体的 Body
tick 提交         WorldPublishPostState → BinaryEntities::Commit（成功才交给后台）
Manager 出口      FrameEnd
```

Host 本机不需要显示委托，原生 Update 和 Render 照常运行；采样只读，不改变游戏状态。

## 6. 发布

### 6.1 采集后台线程

`ops/world_commands.cpp` 创建采集后台线程，循环调用 `BinaryEntities::Background`：

1. 处理控制消息（开始、停止、失效、退休 Body）；
2. 从 `CaptureExchange` 取出采集包，每轮最多处理 9 份；后台来不及时可以跳过中间的 World，只处理最新一份；
3. 导出资源目录，调用 `HostStateCapture::Capture` 生成来源快照；
4. 发布到 `HostStatePublication`；
5. 等网络线程把本帧事件交接给全部 Peer 后，再处理下一份 World。

后台线程只处理自有字节，不访问游戏内存。

### 6.2 网络线程

`net/worker.cpp` 的网络线程拥有 UDP socket 和全部 `HostPeer`：

- **握手**：Hello 和 Ack 交换能力声明。二进制状态能力和布局版本必须精确一致，否则拒绝。
- **输入接收**：校验身份、阶段和序号单调性后，放进该 Peer 的输入队列。
- **状态发送**：每个 `HostPeer` 持有一个 `HostStatePublisher`，从 `HostStatePublication` 读最新快照，相对该 Peer 的确认基线组帧，按窗口和预算发送。
- **共享编码**：线上编码和 zstd 在编码线程（`net/stream_session.cpp`）完成，确认进度相同的 Peer 共享结果。
- **ACK 与 Resync**：收到 AppliedAck 推进基线和事件水位；收到 Resync 为对应流重建完整基线。

只有共享的 socket 或线程本身失败时才结束整个 Host；单个 Peer 出错只退休该 Peer。

### 6.3 与主线程的关系

- 世界发布失败只撤销复制来源，不影响输入调度和 Authority 提交，主线程不等待编码或网络。
- 网络线程交给主线程的只有复制好的纯值：输入命令、Peer 事件、统计。

## 7. 持久化与隔离

- Host 的存档、成就和解锁照常写入房主本机。
- `game/authority/persistence.cpp` 记录死亡、复活、换层、Continue 等跨帧的持久窗口，在稳定观察后收口，用于诊断。
- Host 不调用原版 Online 或 Isaac Sync 路径。

## 8. 诊断

- `cs status`、`cs stream`、`cs counters` 输出会话、流和计数器的纯值摘要。
- trace 写入 `isaac-cs/traces`，由独立线程写盘，主线程只格式化到固定缓冲。
- 采集成本按阶段计时（`core/capture_cost.h`）。高频诊断须在配置中显式开启（`diagnostics.enabled`），生产默认只保留低频状态和错误日志。

## 9. 扩展

- **新增一个由输入驱动的原生入口**：在 `hooks.json` 的 `player` 或 `input` 组登记 Hook，在 `game/input/consumers.cpp` 中按 ordinal 返回统一输入；不要直接读物理设备。
- **新增需要观察的生命周期点**：在 `hooks.json` 的 `world` 组登记，处理函数只更新身份表或失效标记，不在 Hook 内做完整读取。
- **新增会话命令**：在 `ops/command.cpp` 添加 verb，状态变更通过命令队列在 Manager 入口执行。
