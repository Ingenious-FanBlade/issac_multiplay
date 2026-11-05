> **tx-04-21 I21—I25（2026-11-05）**：[正式路由准备、网络冷构建、同Scene动态／结构安装和精确原生Applied ACK](tx-04-21/tx-04-21-installation.md)已实现，必要逆向由GPT-6 Luna max研究并经主智能体复读。Release纯编译／链接通过；未运行测试或启动游戏。29/38单侧身体快源因跨身体消费而报告gap，沿合格ordinary组合；频率与性能未实测。I26—I40及I39的P2全E01—E26及子用例→P4全套待继续；以下旧记录保留原时点事实。

> **tx-04-21 I01—I05（2026-11-05）**：[基础实现与边界](tx-04-21/tx-04-21-foundation.md)已接逐成员实时目录、R01—R09采用合同与主审、不可变范围模型、v2严格schema／Live小消息及正式consumer3。未构建／测试／启动游戏；I06—I40及I39统一验收待继续。频率、性能、世界传输和ACK运行结果尚未取得。下方初版规划与历史记录保留。

# 复制模型冗余发送优化设计

> **2026-11-05，21具体化（仅文档）**：[实施计划](tx-04-21/tx-04-21-implementation-plan.md)和[网络合同](tx-04-21/tx-04-21-network-contract.md)承接本文21事项。当前采用压缩Baseline初建／恢复＋分层完整状态持续更新，Player调度60 Hz、普通动态30 Hz、roster20 Hz；三Peer共享10 Mbps总上行／7.5 Mbps模组预算。新版拟64字节分片头，净荷1136字节；本文旧32字节头／1168字节、旧压缩样本及估算按原时点保留。先两人E2E通过再四人，均未执行。20当前17.4 MB投影不能套用旧60—90 KiB压缩估计。

日期：2026-11-03，Asia/Singapore。状态：**tx-04-19 前置 P1—P6 已实现，运行验收归 19；尚未开始 Entity 应用主体**。实现、消费者复核与构建状态见第 10 节。第 1—9 节的样本数字仍是原设计阶段的统计或预算，不能当作本次实测收益。

19 的当前执行安排见[具体实现计划](tx-04-19/tx-04-19-implementation-plan.md)：I31 负责新增 Entity 消费者的投影复核，I33／I34 将本文要求的 18 全场景回归、版本／图／资源及体积检查合入同一整体 E2E；新计划仍是文档，不改变本次实现与未运行验收的状态。

同日后续 [I01—I03实施](tx-04-19/tx-04-19-foundation.md)恢复 `EntityBomb.GetCostumeLayerSprite` 的 0—4 参数／真实成员资源／精确姿态／Null 发送，原因是 I10／I14 的必要表现输入。原先 201 行省略清单现为 200 行；历史 P1—P6 统计不追溯改写，实际体积收益仍待 I34。模型／projection v2 保留，新增必需 `entity-target-v1` 防止旧消费者把新合同误当原生应用支持；其余省略项仍须由 I31 随实际 adapter 复核。

历史事实：原设计轮仅做代码阅读、离线样本统计和文档编写，没有修改代码、构建、运行游戏或执行测试；同日补充第 8 节的 10 Mbps 预算和控制流量精简。后续本次实现不改写该轮事实。

执行依据为[现行项目规范](../refactor/project-conventions.md)。文中的 P／F／Q 编号只是本文的工作索引，不是新的 tx，不设 gate，也不做哈希校验。tx-04-19 之前的工作并入 tx-04-19 的完整目标，在 19 的统一整体验收中验证；21／22／32 的工作分别归入各自的 tx。

## 1. 背景

### 1.1 当前事实

- **世界状态还没有走网络**。真实下行只有 Welcome、roster 进度、InputAck、disconnect 和 Error。Client 收到其他消息会以 `live_unexpected_world_message` 失败（`mod/src/net/live.cpp:248`）。世界目标目前只能通过 E2E 的本地文件交给 Client（`mod/src/ops/client_fixture.cpp`、`mod/src/ops/command.cpp` 的 `client_test`）。
- **路线图对 21 的写法**：tx-04-21 "可以先以全量快照持续更新，不要求在此先完成压缩"。所以快照体积直接决定 21 的四端闭环能否成立。
- **编码方式**：ICSR v1 是自描述的类型树（`mod/src/replica/codec.cpp` 的 `Write()`）。每个对象成员都写 4 字节长度加完整键名；数字固定 9 字节；u64 由 `Decimal()` 写成十进制字符串；int64 写成 `{"type":"int64","value":"1"}` 这样的包装对象。
- **预算**：
  - 模型上限（`replica/model.h` 的 `Limits`）：32 MiB 字节、`kNodeBudget` 节点（按用户要求在压缩优化前放宽到 64M）、32768 分片、单个数据报 1200 字节（每片净荷 1168 字节）。
  - 实时传输上限（`net/live.cpp` 的 `Limits()`）：64 KiB、4096 节点、64 分片。
- **带宽底线**：用户给定的传输带宽底线是 10 Mbps，预算见第 8 节。
- **复制投影层**：`replica::CaptureRows` 调用 `Registry::Project`（`replica/model.cpp:170`）。它按复制目录 `Policy()` 逐成员处理：ownership 不是 `ReplicatedState` 的成员换成占位对象，`Sprite` 换成 `resource_pose`，并附带 `resource.logical_path`。目录由 `mod/tools/gen_replica_catalog.py` 在构建时生成（`mod/CMakeLists.txt:42`）。

### 1.2 问题

一个完整房间目标未压缩时有 4–12 MiB，大部分是应用器不读取的信息：静态资源描述、观测时的参数枚举、占位说明、Lua 图节点编号，以及同一对象的重复副本。这些内容既占带宽，也占 Authority 编码、Client 解码和预算检查的 CPU。

## 2. 实测数据

### 2.1 来源与方法

- **样本**：tx-04-18 统一 E2E 中 Source-09 导出的 32 个 Client 目标，以及同次 Source-07／08 的目标，共 96 个。样本生成时间早于提交 `8d33747`，所以不包含该提交新增的 Rock Sprite 别名（见 P1）。这些临时样本已经按规范清理，下表数字不能再从磁盘复现。
- **统计方法**：用离线脚本按 `codec.cpp` 的 `Write()` 规则逐值计算线上字节，没有调用实际编码器。脚本只读样本文件，不运行游戏、不构建，也不作为任何验收依据。
- **复测**：实施后在 19 的统一验收中，用 tx-04-18 I26 新增的 `cs status` 目标字节和节点计数复测。

### 2.2 总量

| 目标 | 线上字节（未压缩） | 节点数 | 分片数 |
|---|---|---|---|
| `baseline`（普通房） | 3.87 MiB | 429,001 | 3,477 |
| `change` | 7.50 MiB | 793,827 | 6,737 |
| `entry-0` | 11.14 MiB | 1,163,665 | 10,003 |
| `dense-shape` | 11.58 MiB | 1,207,139 | 10,396 |

`entry-0` 按组件拆分：

| 组件 | 大小 | 占比 |
|---|---|---|
| grid | 7.31 MiB | 65.6% |
| room_save | 1.29 MiB | 11.6% |
| room | 802.8 KiB | 7.0% |
| level | 757.8 KiB | 6.6% |
| presentation | 596.5 KiB | 5.2% |
| entity | 310.5 KiB | 2.7% |
| 其余 5 个域 | 合计约 130 KiB | — |

整个目标中，**键名开销约占 52%，字符串值约占 32%**。

### 2.3 主要冗余

以下均为 `entry-0` 的数据：

| 冗余 | 体积 | 说明 |
|---|---|---|
| Rock body 中重复的 Sprite | 2.38 MiB | `GridEntityRock.GetSprite` 与 `GridEntity.GetSprite` 去掉 `node` 编号后完全相同 |
| `Sprite.GetAllAnimationData`（Grid 公共 Sprite） | 1.54 MiB | 整份 ANM2 动画表，逐实例复制。184 格去掉 `node` 后只有 3 种不同内容 |
| `GridEntityRock.GetAltRockType` | 1.17 MiB | `parameter_query` 的参数枚举结果，每块石头 63 组 |
| `Room.GetGridPosition`／`GetRail`／`GetGridPath`／`GetGridCollision` | 102.8／98.2／97.1／96.6 KiB | 都是 `parameter_query`，对全部格索引逐个查询。`application_runtime` 里已经有紧凑的 `grid_paths`（3.9 KiB）和 `rails`（3.9 KiB）；格坐标可以由几何推出 |
| `Sprite.GetLayerFrameData` | 207.8 KiB | `parameter_query`，C++ 没有按名读取（见 Q2） |
| `NotScheduled` 占位 | 分散 | 每个实例都带 `reason` 和 `signature` 字符串，内容只取决于成员本身 |
| Lua table 的 `node` 编号 | 每个快照 4,138–18,298 个 | 样本中 `graph_ref` 出现 0 次 |

不是冗余、但量级值得注意：Grid 每格的线上大小中位数为 47.2 KiB，最大 73.9 KiB；Entity 组件在只有 1 个 Player 加代表 Effect 的样本里就有 310 KiB。19／20 接入 NPC、Tear、Projectile 等之后，Sprite 数量会成倍增加。

### 2.4 压缩对比

四种方案：
- **A**：原样
- **B**：通用子树去重，≥48 字节的重复对象或数组从第二次出现起改为引用
- **C**：本文 P1–P5 的语义清理（统计时也去掉了 `HasCustomShader` 这类有消费者的 `parameter_query`，所以 C 略微高估收益）
- **D**：C 加 B

| 方案 | `entry-0` 未压缩 | zlib-6 | zstd-3 | lzma-6 | `baseline` zstd-3 |
|---|---|---|---|---|---|
| A 原样 | 11.14 MiB | 522 KiB | 165 KiB | 117 KiB | 90 KiB |
| B 原样＋去重 | 5.39 MiB | 297 KiB | 155 KiB | 109 KiB | 84 KiB |
| C 语义清理 | 3.98 MiB | 155 KiB | 86 KiB | 53 KiB | 67 KiB |
| D 清理＋去重 | 1.57 MiB | 116 KiB | 79 KiB | 48 KiB | 59 KiB |

结论：
- 用 zstd 时，通用去重只多省 6–9%；
- 语义清理在压缩之后仍能再省一半左右；
- zstd-3 压 11 MiB 约 4 ms。

## 3. 复制模型规则（19／20 起执行）

- **R1 只带实例状态和资源引用**：复制模型只携带应用器需要的逐实例状态。静态资源描述（ANM2 动画表等）由已有的 `resource.logical_path` 引用代替，Client 从本地已安装的同版本资源读取。
- **R2 观测元数据不进复制模型**：参数枚举结果、`NotScheduled` 的说明字符串、Lua 图节点编号，只有在应用器确实消费时才保留。
- **R3 同一原生对象只复制一份**：多个 getter 返回同一个内嵌对象时，复制一份规范值，其余位置写 `resource_alias`。
- **R4 观测保持完整**：裁剪只发生在复制投影层（目录 Policy 加 `Registry::Project`）。正式观测、录制和 E2E 诊断看到的内容不变，符合"完整观测与用于 Replica 的状态选择分开"。
- **R5 省略不等于缺失**：成员只有在目录中声明为不复制时才会被省略，不按运行时的值判断。应用器必需字段缺失时，仍然在写入前拒绝；读取失败（ReadFailed、Unavailable 等）的状态照常复制，不能借省略掩盖采集失败。
- **R6 不改协议**：不改 ICSR v1 的 wire 格式和 codec，不新增值标签。目录或模型版本变化通过现有能力协商（`replica::Capabilities()` 的 model 版本）体现，两端版本不一致时明确拒绝。

## 4. tx-04-19 之前的工作项

### P1 Rock Sprite 别名：已落地

- **现状**：提交 `8d33747` 已经在 Grid 的正式采集中（`c.application` 模式）把 `GridEntityRock.GetSprite` 写成 `{"type":"resource_alias","member":"GridEntity.GetSprite"}`（`domains/grid/collect.lua:31`），并在 `application_source.required_contracts` 中登记了 `grid-sprite-alias-v1`。Client 在 `client_grids.cpp:118` 接受这种规范别名，也接受内容相同的完整副本。Door 的 `ExtraSprite` 和 PressurePlate 的 `TimerPlate` 改为读取真实成员。
- **原计划 19 要做的**：把 R3 推广到 Entity 和 Player。现已静态复核所有相关 Sprite Getter：当前未发现第二个可声明的同成员别名，独立成员保持独立；依据见第 10 节。

### P2 去掉静态 ANM2 动画表

- **内容**：`Sprite.GetAllAnimationData` 不进入复制模型。
- **依据**：C++ 代码中没有任何地方读取它。Client 构建 Sprite 的过程（`ClientParseResourcePose`，`client_resource_model.cpp:95`）用的是 `GetFilename`、当前动画和帧、overlay、变换和颜色、`GetRenderFlags`、shader 状态、`GetAllLayers` 和 `pose_runtime`。
- **作用范围**：所有 `Sprite` 值，包括 Grid、Entity、Room 的 backdrop／rail／vein，以及 presentation 域。
- **暂不处理**：`Sprite.GetAllLayers` 有消费者（`client_resource_model.cpp:109`，`room_grid_budget.cpp:21` 的图层预算），本轮保留，见 Q1。

### P3 去掉无消费者的 parameter_query 成员

- **内容**：目录类别为 `parameter_query` 的成员，如果没有应用器消费者，就不进入复制模型。无论观测结果是 `parameter_results`（有参数枚举）还是 `NotScheduled`（没有参数集），都一样处理。
- **目录规模**：entity 120 个、room-level 32 个、run 37 个、continuity 14 个、grid 2 个。
- **按名统计的已知消费者**：只有 `Sprite.HasCustomShader`、`Sprite.HasCustomChampionShader`、`LayerState.HasCustomShader`、`LayerState.HasCustomChampionShader`（`ShaderAbsent` 检查），这 4 个保留。
- **本轮确认可以去掉**：
  - `GridEntityRock.GetAltRockType`：`client_grids.cpp` 的 Rock 分支只读 `Anim`、`RubbleAnim`、`FrameCnt`、`GetBigRockFrame` 和 Sprite 别名；
  - `Room.GetGridPosition`、`Room.GetRail`、`Room.GetGridPath`、`Room.GetGridCollision`：Client 读取的 Room 字段里没有这 4 个，路径和铁轨使用 `application_runtime` 里的紧凑数组。
- **必须先做的**：在实施时逐项登记"消费者清单"。Grid 字段名有一部分是动态拼接的（`client_grids.cpp:47` 的 `"GridEntity"+cast+"."+name`），所以不能只凭字符串 grep 决定删除，要逐个读应用器代码确认。
- **要同步修改**：tx-04-18 的应用目录对 `parameter_query` 的处理写的是"参数与结果一起保留"（`gen_room_grid_catalog.py:130`），需要相应修订。

### P4 占位对象只保留状态

- **NotScheduled**：如果 P3 把无消费者的 `parameter_query` 整项去掉，大部分 `NotScheduled` 占位会随之消失。剩下有消费者的，只保留 `{"status":"NotScheduled"}`，`reason` 和 `signature` 留在目录里。
- **非 `ReplicatedState` 的占位**：`Registry::Project` 对这类成员，每个实例都写一个带 `reason` 字符串的占位对象（`model.cpp:197` 和 `:203`）。按 R2 改为只写 `status`；或者目录声明为不复制后整项省略。注意 Entity 应用器的 `FieldsSupported`（`client_objects.cpp:60`）现在会跳过非 `ReplicatedState` 的已出现字段，省略后它的行为不变。这一项样本中没有单独统计，预计体积较小（见 Q4）。

### P5 只保留被引用的 Lua 图节点编号

- **内容**：`type:"table"` 的 `node` 编号只在同一组件内有 `graph_ref` 指向它时才保留。观测层已有同类处理：裁剪后剪掉悬空的 `graph_ref`（`domains/common/read.lua:335—345`）。
- **注意**：Client 的 `SameSpriteValue`（`client_grids.cpp:31`）对 `node` 做双向映射比较。两边都没有 `node` 时比较结果不变；如果有一边保留了，映射仍然成立。
- **附带好处**：去掉 `node` 以后，同值但节点编号不同的子树不再被当成不同，后续 Delta 比较的伪变化也会减少。

### P6 落地方式

1. `gen_replica_catalog.py` 的 `classify` 增加一个 ownership 值 `NotReplicated`，每项带理由和"复核时的消费者结论"。P2、P3、P4 中省略的成员显式列在生成脚本的清单里，不靠类别一刀切。
2. `Registry::Project` 遇到 `NotReplicated` 时不输出该成员。P5 的 `node` 裁剪在投影完成后，按组件内的 `graph_ref` 集合执行，并检查没有悬空引用。
3. `Independent()`（render、runtime 等独立组件）走同一个投影，所以同样适用。需要确认独立姿态更新所需的字段没有被误删。
4. `ops/client_fixture.cpp` 的 `ProjectEntity` 只保留 tx-04-17 声明的字段白名单，不再承担去冗余的职责，`client_profile` 的说明同步修订。
5. 复制目录或模型版本加一，两端通过能力协商确认。

先不把裁剪下移到观测 Lua 层。如果 19 的验收显示 Authority 主线程的采集耗时有问题，再参照 P1 的 `c.application` 模式和 `ctx.runtime` 跳过 Sprite 的先例，把不复制的成员在正式采集阶段就不读取（见 Q3）。

### 预期效果

在 source-09 样本上，P1–P5 合计把 `entry-0` 从 11.14 MiB 降到约 3.98 MiB，把 `baseline` 从 3.87 MiB 降到约 2.42 MiB。其中 P1 的 Rock 别名部分已经由 `8d33747` 实现。最终收益以 19 验收中 `cs status` 的复测为准。

## 5. 失败方式（实施前列出）

| 编号 | 失败方式 | 对策 |
|---|---|---|
| F1 | 删掉了应用器实际读取的成员，包括按动态拼接名读取的成员 | 逐成员登记消费者结论，写进目录；19 验收重跑 18 的 Room／Grid 场景作为回归 |
| F2 | 把省略和采集失败混为一谈，掩盖了 ReadFailed、Unavailable | 省略只由目录声明决定；失败状态照常复制（R5） |
| F3 | 两端目录或模型版本不一致，一端认为某字段必需 | 版本进入能力协商，不一致明确拒绝（R6） |
| F4 | `graph_ref` 指向被删掉的 `node`，引用悬空 | 投影后按组件检查，有悬空引用就拒绝 |
| F5 | 预算统计依赖被删的字段 | 核对 `room_grid_budget.cpp`：它只按 `Sprite.GetAllLayers` 和 `embers`／`rubble`／`poses`／`rays` 计数，这些都保留 |
| F6 | E2E 夹具、harness 读取了被删字段（`tests/e2e-mods/isaac-cs-test`、`tools/harness/src/tx18.rs`） | 实施时同步修改；在 19 统一验收中验证 |
| F7 | fixture 的 `ProjectEntity` 和正式投影重复裁剪，产生冲突 | 职责分开（P6 第 4 条） |
| F8 | 误改观测层，导致观测、录制完整性下降 | 裁剪只在复制投影层（R4）；观测 Lua 不改 |

## 6. 验收

- 在 tx-04-19 的统一整体验收中完成，不单独开 tx。按规范，进入验收前不运行任何测试，也不补单元测试。
- **场景**：tx-04-18 的 Room／Grid 场景完整重跑作为回归，再加上 19 自己的 Entity 场景。
- **检查项**：
  - Client 构建和应用成功；
  - 画面层次与房主一致；
  - 用 `cs status` 的目标字节和节点计数对比实施前后；
  - 别名、`HasCustomShader`、`pose_runtime` 这些保留项的行为不变。
- **材料**：默认不保留 E2E artifact，结果写进 19 的实施记录。

## 7. 待确认问题

| 编号 | 问题 | 处理时机 |
|---|---|---|
| Q1 | `Sprite.GetAllLayers` 中，哪些是逐实例的值（可见性、换图等），哪些是 ANM2 的静态数据，能否拆开 | 已读 `Layer()`／`SetPose()`：当前解析器消费整组逐层值与本地资源匹配信息，本轮全部保留；拆静态字段不在本次范围 |
| Q2 | `Sprite.GetLayerFrameData`（207.8 KiB）是否已经被 `pose_runtime` 完全覆盖 | 已静态确认：官方 Getter 按当前 selector 返回本地 AnimationData 帧；`ExactPhase` 安装同一 selector，逐层实例变换另由 `GetAllLayers` 提供。本轮列为不发送，运行回归仍归 19 |
| Q3 | 是否把不复制的成员下移到正式采集阶段就不读取，以节省 Authority 主线程 CPU | 看 19 验收的采集耗时再决定 |
| Q4 | 非 `ReplicatedState` 占位对象的实际体积 | 19 实施时用 `cs status` 测出 |
| Q5 | Entity 目录 120 个 `parameter_query` 成员逐个确认有无消费者 | 已按当前应用器复核，结论逐项进入发送清单与生成目录；19／20 新增消费者时必须显式恢复其必需字段，不能把当前无消费者外推成永久不用 |

## 8. 带宽预算与控制流量（10 Mbps 底线）

本节数字都是按代码推算的，没有做网络实测。包大小按 codec 规则估算，十进制字段取典型位数；预算是纯计算。

### 8.1 前提

- **带宽口径**：10 Mbps 按**房主上行总带宽**计算，由 3 个远端 Client 共享。留 25% 余量，用来应付突发、重传和其他进程占用。每个 UDP 数据报额外算 28 字节的 IPv4／UDP 头。如果 10 Mbps 指每个 Client 单独拥有，下文的每 Client 预算约乘 3。
- **单个数据报保持 1200 字节**（`Limits::datagram`），与带宽无关。超过路径 MTU 会被 IP 分片或丢弃，而公网路径常有 PPPoE、VPN 或隧道开销。1200 是 QUIC 等协议采用的安全值，也能放进 IPv6 的最小 MTU 1280。更大的逻辑消息继续由应用层分片，每片净荷 1168 字节。

### 8.2 当前控制流量（代码事实）

- **主线程**：每个外层循环调用一次 `PublishPeer`（`game/tick.cpp:384`），给每个已连接的 Client 生成一份进度快照，经 `Worker::QueueAck` 入队。频率约等于 Authority tick 的 60 次/秒（tx-04-07 实测 59.99／60.005 tick/s）。
- **网络线程**：每 10 ms 轮询一次（`kPollMs=10`）。`SendBatch`（`net/worker.cpp:470`）一次取空队列，只发最新的一份。250 ms 内没有新快照时补发一次心跳（`heartbeat_ms=250`）。
- **每份快照拆成两条消息**，各自单独分片（`HostPeer::Progress`）：

| 消息 | 估算大小（加 32 字节分片头） | 频率 |
|---|---|---|
| Control `roster` | 约 721 字节，1 片 | 约 60 次/秒 |
| `InputAck` | 约 323 字节，1 片 | 有已处理输入时，约 60 次/秒 |

加上 IP／UDP 头，**每个 Client 约 66 KB/s（约 0.5 Mbps），三个 Client 合计约 1.6 Mbps**，占 10 Mbps 的 16%。主要冗余：
- 每个包都带完整 scope：7 个十进制字符串，约 250 字节；
- 键名逐个完整写出；
- roster 里的名单、计时和资格，在大多数 tick 之间没有变化。

### 8.3 每次状态发布的预算

计算方法：每个 Client 可用 = 10 Mbps × 75% ÷ 3 ≈ 312 KB/s，再扣掉控制流量。

| 发布频率 | 控制流量保持现状（剩约 246 KB/s） | 控制流量精简后（按约 20 KB/s 算，剩约 292 KB/s） |
|---|---|---|
| 60 Hz | 约 4.1 KB／次（3–4 片） | 约 4.9 KB／次 |
| 30 Hz | 约 8.2 KB／次（6–7 片） | 约 9.7 KB／次 |
| 20 Hz | 约 12.3 KB／次（10 片） | 约 14.6 KB／次 |

以约 246 KB/s 计，给一个 Client 发一次完整快照所需的时间（体积来自第 2 节）：

| 内容 | 大小 | 时间 |
|---|---|---|
| 完整快照，未压缩 | 3.87–11.58 MiB | 16–49 秒 |
| 完整快照，zstd-3 | 90–165 KiB | 0.37–0.69 秒 |
| P1–P5 清理后再 zstd-3 | 67–86 KiB | 0.28–0.36 秒 |

### 8.4 对 21／22 的约束

本节为2026-11-03的原预算推导。2026-11-05已由[21网络合同第2—4节](tx-04-21/tx-04-21-network-contract.md)具体化：21不周期刷新全域Baseline，以显式依赖的完整子范围连续更新；22增加基于已确认B的对象／字段Delta。下列“持续更新只能靠Delta”保留其当时对旧大组件实现的判断，不作为禁止21分层完整状态的现行要求，也不把21实时目标提前算作完成。

1. **完整快照只用于 Baseline**，即首次加入、切场景和恢复，而且必须压缩。"持续发送完整快照"在这个带宽下每秒最多 1–3 次，不能作为 21 的持续更新方式。Baseline 分块要按剩余预算限速发送，不能挤占其他 Client 的持续更新，Authority 也不能为它等待。
2. **持续更新只能靠 Delta**，每次只有几 KB。现在 `Difference` 是整组件替换，Grid 任何一处变化都要发几 MiB。所以 22 的对象／字段级 Delta 是能否在 10 Mbps 下运行的前提。如果 21 先于 22 闭环，21 就要先用 Baseline 低频刷新加上 Player 等小 scope 的独立更新来过渡，并如实记录实际帧率。
3. **按原生节奏分层发布**：
   - Entity、Grid、Room 的动态部分按 30 Hz 发布，和原版世界逻辑的 30 帧/秒一致（原版 Wiki 中 `Game:GetFrameCount` 为每秒 30）；
   - Player 身体按 60 Hz 发布，对应奇数帧的 Player 更新；
   - 楼层、level descriptors、room_save、静态几何和 HUD 只在变化时发送。
4. **每个 Peer 有发送预算和优先级**：超出预算时按以下顺序取舍：自己的身体、其他玩家、附近的实体、其他实体。但删除和最终值不能丢，按设计文档第 9 节的要求，它们要一直留在未确认的差异里。持续超出预算时，改为对该 Peer 重建 Baseline 或停止复制，不无限积压。

### 8.5 控制流量精简（归 21）

| 编号 | 内容 | 注意事项 |
|---|---|---|
| C1 | **scope 不逐包携带**：连接建立时由 Welcome 确定 scope，之后的包只带连接 id 加一个短代次号；scope 变化走 Control 消息 | 旧包和过期 scope 的拒绝语义不能弱化：代次不匹配时仍然丢弃 |
| C2 | **roster 降频**：周期发送降到 10–20 Hz；InputAck 仍然每 tick 发送 | 会话状态、`input_enabled`、generation 一变就**立即**补发一次 roster，不等周期，否则 Client 会在旧代次下发送输入并被丢弃。Client 的 `InputTiming` 速率估计要求锚点间隔 ≥250 ms，10–20 Hz 足够，但 RTT 回显字段要保留在每个 roster 里 |
| C3 | **同一 tick 的小消息合并成一个数据报**：比如 roster 加 InputAck，以后再加上 Player 状态 | 合并后超过 1200 字节时退回分开发送；每条消息的 lane 语义不变 |
| C4 | **数值字段用定长或 varint 编码**，不用十进制字符串 | 和第 9 节 32 的编码优化同一方向；21 可以只对控制消息先做 |

- **预计效果**：每个 Client 的控制流量从约 66 KB/s 降到约 15–25 KB/s。这是估算。
- **协议影响**：C1、C3、C4 改变消息格式，C2 改变 `ReadProgress` 的成员检查（目前要求恰好 16 个成员），都要提升协议版本并走能力协商。旧的 ICSN v2（Rust 远端）不在这个范围内。

**失败方式**

| 编号 | 失败方式 | 对策 |
|---|---|---|
| F9 | roster 降频导致资格或代次变化通知不及时，输入被丢弃 | 状态变化时立即发送（C2） |
| F10 | 计时样本变少，目标 tick 估计偏差变大 | 每个 roster 都保留回显字段；在 21 实测目标 tick 偏差 |
| F11 | 合并后的数据报超过 1200 字节 | 退回分开发送（C3） |
| F12 | 去掉逐包 scope 后，旧连接或旧场景的包被误接受 | 连接 id 和代次号一起校验，不匹配就丢弃（C1） |

### 8.6 实测（21 统一验收）

worker 已经有每个 Peer 的 `received_packets`／`received_bytes` 计数，需要补上按 lane 分开的发送字节和包数。四端闭环中实测以下数据，结果写进 21 的实施记录，替换本节的推算值：
- 控制流量；
- Baseline 传输时间；
- 持续更新的每次字节数和实际发布帧率；
- 目标 tick 偏差。

## 9. 之后的工作（简述）

| tx | 内容 | 理由 |
|---|---|---|
| tx-04-21 | **控制流量精简**（第 8.5 节 C1–C4） | 现状约 1.6 Mbps，占 10 Mbps 的 16% |
| tx-04-21 | **分层发布和每 Peer 发送预算**（第 8.4 节） | 60 Hz 下每次发布只有约 4–5 KB |
| tx-04-21 | **按频率拆分 scope**：level descriptors（约 366 KiB）、room_save（约 1.3 MiB）、静态房间几何和配置改为楼层级或场景级低频发送；持续更新只发当前房间动态部分和 Entity／Player；HUD 从 room 组件的 `presentation_runtime` 中拆出 | 现在 Delta 是整组件替换，HUD 一变就要重发整个 room 组件 |
| tx-04-21 | **Baseline 可靠分块传输**：实现已定义的 `ReceiveAck` 选择性重传，超时按传输进度计算，按实际 Baseline 大小调整 `live.cpp` 的 `Limits()` | 现在要求全部分片到齐；1 万片在 0.1% 丢包下几乎不可能收齐 |
| tx-04-21 | **整消息 zstd 压缩** | 实测单个快照约 60–90 KiB（55–80 片），比现在少两个数量级；10 Mbps 下 Baseline 约 0.3–0.7 秒传完 |
| tx-04-22 | **对象／字段级 Delta**、省略默认值；10 Mbps 下的硬前提 | `Difference`（`model.cpp:282`，比较在 `:291`）现在按整个组件替换，任一格变化都要重发整个 Grid 组件 |
| tx-04-32 | 按 schema 给字段编号、varint、去掉 int64 包装对象、针对小 Delta 的 zstd 预训练字典、量化与优先级调度 | 属于编码层优化，不改变正确性语义 |
| — | **不做**通用子树去重 | zstd 下只多省 6–9%；还要付出引用表生命周期、展开后预算和 Delta 纠缠的代价 |

## 10. 2026-11-03 前置优化实现

本次按用户要求先完成 19 之前的优化，实现范围仍并入 `tx-04-19`，未新增 tx 编号。没有开始非 Player Entity 创建／更新／Morph 应用，也没有提前执行整体验收或任何分项测试。下面是代码事实与静态推导，实际字节收益、CPU 和画面回归等待 19 统一 E2E。

| 项目 | 实际落点与结果 |
|---|---|
| P1 | 保留现有 Rock `resource_alias`。已读正式 Entity／Player 采集及官方 Getter／SDK成员：`Entity.GetSprite` 对应基类 `_sprite`，Player 的 `_bodySprite`、`_heldSprite`、`_bloodGushSprite`，Tear 的三份辅助 ANM2、Pickup 的价格 Sprite、Slot 的奖励 Sprite、Bomb 的 costume 数组是各自成员；Entity 派生采集只采一次基类 `GetSprite`，Player 域用规范 Entity 引用，不复制第二份身体。不能因路径或当前值相同将这些成员合并；未添加未经证明的别名 |
| P2／P3 | `mod/tools/replica_projection_policy.py` 显式列出 `Sprite.GetAllAnimationData` 及无消费者参数查询；`gen_replica_catalog.py` 将每项生成 `NotReplicated`、理由和 `consumer_review`。生成目录按行计：Entity 116、Grid 2、Room/Level 32、continuity 14、run 37，合计 201 行参数查询不发送（含同名重载）；四个 Sprite／Layer shader 查询保留。新增或未列出的查询不会按类别自动删除 |
| P3 动态消费者 | `client_grids.cpp:Parse/Field` 的 17 个 cast 分支逐一回读，两个 Grid 参数查询均无读取。Room 路径／铁轨／门几何走正式紧凑结构；18 夹具对原生 `Room:GetGridPosition` 的场景布置调用仍正常，不依赖目标中同名观测枚举；`tx18.rs` 也不读取被省略的参数结果 |
| P4 | `Registry::ProjectValue` 对非复制归属和 `NotScheduled` 只输出 `status`；目录保留原因、参数签名和消费者结论。未知成员仍为带 `undeclared_member` 的 Unsupported，读取失败对象与域 coverage／reasons 不改成成功 |
| P5 | `projection_graph.*` 在完整组件投影后收集 `graph_ref`，只保留被引用 table 的 `node`，拒绝悬空／零编号／被引用定义重号。`ValidateModel` 检查模型内引用闭合，独立 Component 也经共用根投影检查；不跨域借同号定义，不删除引用来伪造成功，不修改 codec |
| P5 消费者 | `SameSpriteValue` 允许 table 定义编号仅一侧存在；双方有编号时仍做双向映射，`graph_ref` 本身不因可选定义编号而放松。普通数值及图连接继续比较 |
| P6 | `Registry::Project` 为普通、Render／runtime 及独立只读 Query 共用的根入口；Query 的 `member/parameters/result` 包装也按成员目录省略参数结果，保留成员／主体和 `NotReplicated` 状态，不能绕过清单 |
| 夹具边界 | `ops/client_fixture.cpp:ProjectEntity` 和 `tools/client-fixture/main.cpp` 只选择 17 的 Entity 字段白名单，说明已同步；`source-full` 明确是完整域的发送投影，不是完整观测原文。观测原文／录制 Lua 未改动 |
| 版本／构建接线 | 复制目录、Room/Grid 应用目录均为 v2；能力的 `model=2`，新增必需能力 `replica-projection-v2`，旧模型沿现有协商路径明确拒绝。ICSR v1 字节格式未改；CMake 已加入图模块及发送清单的生成依赖 |
| 既有 E2E 适配 | `tools/harness/src/tx15.rs` 的逐叶源值审阅已理解目录省略、精简占位和可删除 table 编号；仍比较未省略值、失败状态及图引用编号。没有新增单元测试，也没有运行此工具 |

静态来源：官方 `repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp` 的 `Lua_PlayerGetBodySprite`／`Lua_PlayerGetHeldSprite`／`Lua_PlayerGetBloodGushSprite`；`LuaEntityTear.cpp` 的三份 Sprite Getter；`LuaEntityBomb.cpp:Lua_BombGetCostumeLayerSprite`、`LuaEntityPickup.cpp:Lua_PickupGetPriceSprite`、`LuaEntitySlot.cpp:Lua_GetPrizeSprite`；`repentogon/LuaInterfaces/LuaSprite.cpp:LuaGetLayerFrameDataInternal`；本地 SDK `libzhl/functions/Entity.zhl`、`EntityPlayer.zhl`。这些只证明当前目标版本下的成员／消费者关系，不充当运行观察。

纯编译已完成：`cmake --build build/repentogon --config Release --target zhlIsaacCS replica-contract client-fixture -- /m:1 /nr:false /v:minimal` 和 `cargo build --offline --manifest-path tools/harness/Cargo.toml --release --locked --target i686-pc-windows-msvc --target-dir build/harness` 均正常结束；DLL、两份生成目录和辅助工具已更新至 `out/repentogon`。最初 `scripts/build.ps1 -E2E` 的默认并行构建停在 `Checking File Globs`，已主动中断，后改用上述单进程构建完成，不将停滞归因为已证明的代码问题。编译输出仍含官方 RapidJSON 的弃用警告及既有网络代码警告；没有运行生成的 DLL 或测试程序。

统一运行验收未执行：按第 6 节并入 19，重跑 18 Room／Grid 完整场景并结合 19 Entity 场景，覆盖字段缺失／失败、四个 shader 查询、资源姿态／别名、图引用和版本不兼容拒绝，使用 `cs status` 复测目标字节／节点；本次没有可报告的新实测缩减比例。

第 8.5 节 C1—C4、第 8.4／9 节的控制流量、zstd、分层预算、可靠分块和字段 Delta 均未在本次实现，继续归 21／22／32；64M 节点与现有字节预算未擅自收紧。没有制作 tx artifact 或 E2E artifact。


## 11. 2026-11-03 I31新消费者复核

[tx-04-19 I31—I33](tx-04-19/tx-04-19-e2e-integration.md)已逐一复核公共字段表、派生字段表、具名关系、动态Pose/辅助资源及分层draw。Bomb五层参数查询继续保留0..4及Null/失败；四个shader查询、全部动态层与pose_runtime保持。Laser四容器、NPC三deque、typed ExtraData、Text字节与KColor20等使用正式紧凑来源，没有新建Sprite别名；其余逐成员省略理由已写回生成目录。

统一E2E保存同一正式consumer窗口的完整原始envelope、当前复制投影及对应capture token，另读`cs status`目标字节／节点；原始观测与投影差值明确不算本次P1—P6净收益。18的49个Source、49个冷Client及12项检查由统一入口实际重跑，不能引用历史PASS代替。I34前没有执行测试、测量或游戏，原文估算及历史测量保持原事实。

## 12. 2026-11-04 I34实测与消费者补齐

[tx-04-19最终验收与成本口径](tx-04-19/tx-04-19-acceptance.md)已完成18的49／49 Source、49／49冷Client及12检查，19的65／65 Source、65／65冷Client与恢复／拒绝，114对画面已主审。每个19目标实际复核190个省略成员ID，图闭合及独立Query保留失败／Partial语义，四个shader查询和精确动态pose仍在；这是按ID去重的检查口径，与第10节历史按目录行计数不同。

新增消费者逐项回读：静态GetOrbitDistance(Layer)不得遮蔽真实OrbitDistance属性，独立参数查询明确省略而Vector保留；BodySprite与Entity.GetSprite虽然同文件，真实LiftItem frame2与PickupWalkDown pose不同，是独立成员，不新增别名。136所需公开GetHeldEntity／GetItemState／GetSuplexState与BodySprite完整pose保留，经正常apply或Render栈借用消费。Burning generic FX的真实blend_mode是正式来源补齐，不按XML在Client重新选择。

最终Source22同token原始观测／当前投影：最小4,769,700／1,946,562B，中位4,955,164／2,072,820B，dense最大8,857,072／3,834,637B。它们是原始观测与当前发送投影，不能称为P1—P6实施前后净收益。cs status模型预算另计，dense为5,430,843预算bytes／490,727nodes。Source单域采集、Client codec／parse-plan／native-step／draw窗口及含IPC墙钟、全进程峰值均已在验收文档分别记录；没有线程CPU、10Mbps实时能力或全部类别满池的实测声明。21／22／32继续负责真实网络发布、字段Delta与整局性能。
