# tx-04-05：core 原型移植记录

后续记录（2026-10-24）：[第 3.2 节 game 主线程适配](tx-04-05-game-adapter.md)及必要 Hook/solo 命令接线现已编写并纯构建成功，尚未运行测试。本文下方保留 core 阶段交付时的状态与接线约定，后续进度以新记录为准。

日期：2026-10-24。范围为[实现方案](tx-04-05-implementation-plan.md)第 3.1 节。五个 core 模块已编写，Win32 / v143 Release 纯编译、链接成功；没有运行单元测试、独立 harness、游戏或 E2E。tx-04-05 的唯一一次整体 E2E 尚未执行。

## 文件与职责

| 原型 | 新文件 | 本次适配 |
| --- | --- | --- |
| `authority_session.*` | `mod/src/core/session.*` | `cs::core::Session`，Solo 只包含 ordinal 0；身份和会话 generation 冻结；单次对象，不复位旧身份 |
| `authority_tick.*` | `mod/src/core/tick_clock.*` | `cs::core::TickClock`，用 `owner_kind/scope_token` 配对，不含游戏 RVA、原生地址或版本布局 |
| `authority_tick_pipeline.*` | `mod/src/core/tick_pipeline.*` | `cs::core::TickPipeline`，保留 drain、模拟、原生返回、TopExit 提交、发布的顺序；只访问存在的参与者 |
| `input_core.*` | `mod/src/core/input_core.*` | `cs::core::input`，保留整数轴规范化、V1 codec、目标规划、有界队列与去重基础能力 |
| `coop_input.*` | `mod/src/core/participant.*` | `cs::core::Participant`、V2 codec、ShootingDirections、V2 admission/Resolve/只读查询与进度 |

原型来源为 `cs-t0/native/include/isaac_cs_t0/` 和 `cs-t0/native/src/`。新增功能、核心函数和结构注释使用中文。`core` 仅包含自己的头文件和 C++20 标准库；没有 REPENTOGON、Win32、IPC、Rust 控制器、world 采集或 raw apply 依赖。Binding 中的指针仅作不解引用的本地关联。

`mod/CMakeLists.txt` 新增 `isaac_cs_core` 静态库目标，由 DLL 链接，不继承 SDK include 与游戏宏。实际运行入口尚未引用 core；本次构建的 DLL 仍执行 tx-04-04 的透传逻辑。原 `PlaceholderSession` 移到 `ops/command.cpp` 内，现有 `cs host/stop` 行为保持原有占位语义。

## 会话和生命周期

- `SessionState`、`SessionCause` 保留原型数值；`Negotiating`、`NegotiationStarted`、`ControllerLost`、`RestoreFailure` 只保留历史名称。Solo 启动直接进入 Warmup，没有控制器丢失或字节恢复的生效路径。
- 主线程适配层须先确认 run，生成非零且全新的 session id、会话 generation 和 A client id；Solo 要求 `clients[1]==0`。core 不读取 run、不生成平台随机身份，也不把对象指针当身份。
- `Activate(a_bound)` 只要求 A；单人流水线不 Initialize、Begin、End 或获取 B 的进度。回执的 `participant_count=1`、`clients[1].present=false`，不把 B 解释成缺输入。
- `BeginRetirement` 关闭新 admission、新 sequence 和新 tick。已经进入 Simulating 的窗口仍可读取原 current、正常返回、提交 previous，并在 TopExit 推进 completed 和发布。
- owner 确认没有在途 tick/callback 后，退休 A，再调用 `CompleteRetirement(true)`。失败事实保持 Failed，不因清理完成而擦除。
- core 对象是每个会话一次性使用；下一次 solo start 要重新构造整个会话上下文。适配层把 Dormant/Retired 映射到命令状态 Idle，不重写历史状态枚举。

## game 适配层的接线顺序

1. 在稳定存储中构造 Session、TickClock、两个 Participant 的存储与 TickPipeline；每个 Participant 包含固定容量队列和去重历史，避免在每帧调用栈上构造或复制整个上下文。
2. 如使用自定义 `InputRuntimeConfig`，在 A 初始化前调用 A 的 `Configure(config)`，并在首个 drain 前调用流水线的 `Configure(config)`；后者同时配置时钟未来窗口。默认配置不要求额外调用。B 不配置、不初始化。
3. `Session::Start(identity)` 进入 Warmup；`players[0].Initialize(identity.session, identity.clients[0], 0, &session)`。初始化不可重复，不能用它清空旧队列或复活旧身份。
4. game 读取唯一拓扑并调用 A 的 `Bind({player, device, controllerIndex})`。不再要求原型虚拟设备的 control block。room/death/pause/loading 的失效判定及稳定重绑时点由 game 提供。
5. 处于 run 且 A 唯一绑定后 `Session::Activate(true)`。Warmup 不分配 sequence、不 Resolve、不推进时钟。
6. 普通 Authority 入口构造 TickBoundary：StandardRoom 对应 `Game`，OddPlayer 对应 shim 确认的 `OddNormal`；填入非零 scope token、Manager loop 和当前/主线程编号。返回时必须带回同一份 boundary。
7. `BeginDrain(boundary, blocked)` → 本轮新鲜采样的 `MakeCommand` / 流水线 `AcceptInput` → `BeginSimulation()`。只有 `SimulationTick()!=0` 的窗口允许查询 Resolve。
8. 配对原生正常返回调用 `NativeReturned(boundary, true)`，它提交 previous；异常路径用 Abort，不提交未正常返回的快照。
9. 同一最外层 Manager 的 TopExit 调用 `FinishPostState(loop)`，即使 post-state 为 Disabled 也必须经过它推进 completed；之后 `PublishCompleted(success)`，再 `EndOuter(loop)`。`PublishCompleted` 自己不推进时钟。

流水线拒绝未挂到当前 Session/ordinal 的参与者，不能用独立 Participant 绕过会话。模拟中递归入口是 Reentry，原生已返回后同一轮的第二个入口是 DuplicateLoop；返回 token、loop 或线程不匹配分别按现有拒绝原因处理。异常和缺失返回关闭本次时钟，不自动重新尝试。

## 输入语义的保留与边界

- V1/V2 的 codec、轴范围/单位圆、射击 held 一致性、保留位、sequence 去重、目标窗口、队列容量、冲突与过期处理沿用原型。V2 沿用小端、24 字节状态和 64 字节命令布局；它不是旧 ISCS/Isaac Sync 协议的兼容承诺。
- 本地命令也序列化后经过同一个字节 admission，不给房主即时旁路。generation 和 completed 是本地元数据，不从 payload 取得。缺输入立即中性，只取当前目标，未来命令保留在队列。
- `Invalidate` 清空绑定、待处理命令、current 和 previous 的可比较资格，递增 participant generation；不重置会话 sequence 或去重高水位。旧 generation、旧边沿不得恢复。
- Read 只读 current，重复查询不消耗或追加边沿；consumer allowlist 保留 9/10/11 的辅助交叉查询。MC_PRE_PLAYER_UPDATE 跳过时，该 tick 已解析的边沿仍按 U4 丢弃，不补发。
- ShootingDirections 保留后按者优先、释放胜出键回退、同时采样保留旧胜出、无历史固定右/下的规则。物理 held/边沿历史须由 game 按设备生命周期保存，不随 participant 重绑重置。
- 持续 held 每个新鲜 Authority-pre 都须提交对应目标；本轮重复样本去重、启动 completed+1 与 edge 清零的 lead=2 第二条命令、屏蔽期间只更新物理历史，都由后续 `game/input_a` 编排。core 提供原型的目标计算与统一 admission，未伪造物理采样器。
- 进度中的 processed 表示该输入所在原生模拟窗口正常返回，不证明某个 consumer 或 Lua 后的 Player 子类 body 一定到达；这些实际观测由 game Hook 记录。

## 构建记录与剩余工作

[构建记录](../../artifacts/repentogon/tx-04-05-core/README.md)保留 CMake 配置、首次沙箱工具链访问失败和成功构建的原始输出。成功生成 `build/repentogon/Release/isaac_cs_core.lib` 与 `out/repentogon/zhlIsaacCS.dll`。core 编译没有 warning；本次输出的 warning 来自 SDK 头文件及现有生成 wrapper。

这次没有部署产物、修改 Hook 注册行为、启动游戏或操作存档；没有新增/运行测试，也没有计算或校验哈希。第 3.2 节 game 适配、Odd shim/Player 成对 Hook、solo 命令与状态/trace、测试 mod 和 harness 仍待完成。tx-04-05 的一次整体 E2E 留待这些工作完成后执行。
