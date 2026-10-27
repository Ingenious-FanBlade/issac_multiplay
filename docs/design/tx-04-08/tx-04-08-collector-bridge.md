# tx-04-08：collector、主线程桥与 post-state

> **后续更新（2026-10-27）**：用户要求修复 SDK 构建阻断后，已将 Host/Odd 的 SDK-facing 部分独立以 C++17 编译，并移除 Host 对 C++20 状态头的依赖。完整 DLL 在新构建树编译、链接成功，见[修复与日志](../../../artifacts/repentogon/tx-04-08-sdk-build-fix/README.md)。下文第 7 节记录的是本项最初的构建状态；测试与实机仍全部 NOT_RUN。

日期：2026-10-27，Asia/Singapore。范围：按用户要求完成 collector、主线程桥和 post-state 的代码编写，继续同一个 tx-04-08 目标。**本次仅做源码复读、代码生成与纯编译；没有测试、游戏启动、部署或整体验收。** 命令、异步准备编排、离线工具、Lua 夹具与统一整体 E2E 仍是后续工作。

## 1. 交付与入口

| 文件 | 职责 |
| --- | --- |
| `mod/src/world/collector.h/.cpp` | 主线程窗口、19 项观察的 call/parent、内层 body 凭据、生命周期、来源、只读字段、引用、批次准备与发布 |
| `mod/src/game/world_bridge.h/.cpp` | 后台已准备上下文的主线程接收、A/B exact pair/代次复读、阶段与生命周期值通知、post-state 配对 |
| `mod/src/game/bridge.h` | SDK-facing 的 C++17 `WorldBoundaryScope` 和 Player 前快照值 |
| `mod/src/core/tick_pipeline.*` | 六种 post-state 与本地 receipt；世界采集错误不走输入 Abort |
| `mod/src/world/observe.*` | 常驻值通知 sink、录制内 call 接线、非主线程/异常故障序号 |
| `mod/tools/gen_binding.py`、`mod/binding/hooks.json` | 从同一清单派生 19 个内层 native witness，复用原定义和 ABI |
| `mod/src/world/model.*`、`writer.cpp` | 增补事件种类 8 `NATIVE_BODY`，manifest 图例与 writer 同步 |
| `mod/src/world/identity.*` | run 域退休后断开 radix 关联，保留历史身份及单调序号 |
| `mod/src/ops/config.cpp`、`runtime.h` | 独立 `world_record` 配置，默认关闭，只允许注册能力，不自动录制 |

核心静态库现在通过纯值 `core` 类型表达 post-state，仍不包含 SDK、游戏头或 `runtime.h`。`collector` 只通过 `MemoryReader` 进行有界原始拷贝，偏移、RTTI、来源与几何常量全部来自目标布局表；地址不作为网络身份或跨进程 key。

## 2. 主线程与发布顺序（CURRENT_SOURCE）

1. `FrameBegin` 在 `BeginManager` 的 PollPeer/RefreshLifecycle/DrainPeer 和排队命令之前调用 `WorldFrameBegin`，以现有 Manager owner token 建立窗口。
2. 生成的 Game、普通 Room 与标准 tail wrapper 通过 `WorldBoundaryScope` 记录进入和正常返回；Start/Restore/Door/Join/Render 内部阶段不具有普通完整资格。
3. 既有 Room/Odd `NativeReturned` 先使 pipeline 进入 `AwaitPostState`，再传递实际 `{loop, outer, candidate tick, scope, kind}`。不用最近 completed tick，也不改输入或 Shooting 恢复窗口。
4. Manager 下游链返回后，`FrameEnd` 在 `EndManager` 之前调用 `WorldPreparePostState`。异常、无 Authority、odd-only、生命周期、阶段缺失和停止等分别产生明确 gap；只有同 outer 的普通完整阶段才进入原始复制。
5. 有数据的批次留在 PREPARED。`FinishPostState` 正常提交 clock 后，桥核对 tick/outer/attempt/capture/recording generation/status/reason，才把匹配批次发布为 READY。core 未提交或凭据不符时租约撤销，保留 `UNPAIRED_SCOPE` gap。
6. 输入 receipt、tick trace 与 peer Ack 沿既有路径继续；world 的读取、事件、预算或文件失败不决定输入 publication 成功与否。原有真实 clock/loop/原生异常仍沿既有会话失败路径处理。

`TickPostState` 包含 `Disabled / Complete / Gap / Backpressure / Unavailable / Faulted`，以及 reason、recording generation、tick、attempt、capture、outer。它只属于本地 receipt，没有增加 UDP Ack 字段。Disabled 表示未参加本轮采集，不能当完整世界。

采集器的序号各自单调递增、耗尽不回绕。没有槽时继续使用独立 gap 环；身份/事件连续性或通知失败停用当前录制，不停 Host。主线程不分配大数组，不等待 reader、writer、文件或线程。

## 3. 原生 body 与身份（CURRENT_SOURCE）

REPENTOGON 的 Morph、Grid 等回调可以短路原调用。原有 19 条 `reach` 历史说明保持原样；本次在 schema 中增加 `world_native_witness_priority=10000`，由同清单为每项派生 `_native` 内层。外层 `-10000` 观察整条链，内层记录 body 进入与 super 正常返回，使用相同 call/parent；任一层异常均不伪造正常完成。该策略仍依赖实际 Hook 链符合已静态确认的排序，运行命中与异常保持尚未验证。

待注册行数调整为 **79/96**：既有 41 + 外层 world 19 + 内层 world 19，distinct RVA 仍为 **59**。ABI 沿用同一清单，GridFactory 仍用官方 ECX/EDX 定义，不使用错误全局别名。自有定义键只登记一次，双层 Hook 共享 ZHL 链，没有另建入口补丁器。`ObservationMask` 只有两层都 registered 才为 true；registered 仍不代表运行时已安装或已调用。

Factory 的正常原生取得增加 use；Player 构造与其外围 Factory 不重复计同一次取得。只有本次已观察候选和真实最外层创建 owner 完成才冻结出生；Publish 不确认出生。Morph 保留出生/use，原生到达才推进 revision，正常外层返回后恢复配置读取依据；未知 Init/Restore/Replace 撤销不成立的连续性。短路保留 gap/未知事件，不授予完成依据。

ManagerRestore 在 GameRestore 的 PostLevelInit 之前返回；新取得候选的出生延后到最外层恢复/加载 owner 正常退出。已有 Player 仍保持 FIRST_OBSERVED，不因 Restore 补造构造或出生。GridSpawn/Free 比较内层实际槽、外层返回槽和所有 Grid/door 别名；只有已理解的原生完成分支及实际槽变化才退休旧身份。Door 的已存在、复用和新构造分开处理，同一 Grid/door 指针仅一个身份。

真实 Start owner 进入时建立新 run 域，覆盖其内的构造；Start 内部清理旧 Game 的 Exit 不取消新 run 的观察域。下一帧的 run 标志不会再次清掉刚观察到的出生。run 退出后断开旧 radix 地址关联，历史身份和序号保留；新 run 的同地址引用不能复活旧对象。

## 4. A/B 读取依据（CURRENT_SOURCE）

PlayerScope 保存进入时的 player/device/key/session/participant generation；正常退出后先完成原有 lifecycle 复核，再由 world 独立复读拓扑。只有本 tick 仍有效、原生 Player body 已到达、前后 exact pair/代次一致、当前来源唯一且存活，才授予依据 3。该依据不补出生历史，也不取代构造/Init。

全体绑定失效、B 单独失效、stop/fault/peer 退休都会撤销 world witness。对 ST7 指出的 B 不可读/不唯一却未必递增原绑定代次的分支，world 在生命周期、NativeReturned 和复制前独立复读并撤销，未更改既有输入策略。录制关闭时不增加这组 world 拓扑读取。

## 5. 来源、字段与保守范围（CURRENT_SOURCE）

- 15 个来源保留全部编号：八 EL 使用 16 字节 descriptor，roster 只枚举 begin/end 范围，Esau 固定 4，Player 关系固定 64 节点上限，Grid 固定 448，door 固定 8，reserved 明确 NOT_APPLICABLE。
- 同 `(domain,address)` 合并对象，保留所有 source/index membership。来源指针两遍读取，复制尾部再复读动态 descriptor、固定槽和 door 映射；root/Room、当前 recording、mutation 与阶段同窗口检查。不能把这些复读描述为跨线程内存锁。
- RTTI 校验实际映像范围、COL/CHD/BCD、签名、继承数量和当前 TD；不按 Entity Type 数值猜 NPC，不调用 RTTI/getter/query/Lua/RNG/setter。未知派生仅在基 Entity 确认后复制 UnknownEntity 头部。
- 78 项字段按布局宽度原始拷贝。派生字段必须具备对应 readiness；Grid 另核对 initialized、实际 index 和槽关系。保留非有限浮点原始位并使批次不完整；未知/退休/复用引用保持其状态，不强配 local target。
- **当前保守实现把布局 `InitUnknown` 字段标为 UNCONFIRMED，而不是填零或冒称有效。** 例如 `removed_by_factory +0x177` 的初值来源仍 UNKNOWN。这会阻止含该未确认字段的批次获得 Complete；2,000 次目标尚未达成，后续必须依据实际研究或用户明确采纳的当前值规则处理，不能降低计数标准。未观察到创建 owner 的既有非 Player 专用字段同样保持 NOT_READY。
- 只有来源均被完整枚举时才更新成员位集；退出来源不等于物理销毁。成员事件在提交前准备，身份位集在 clock 提交后更新，事件失败不会留下“完整”receipt。

Collector 固定工作表位于堆上，并将本体实际大小计入 storage 共用预算账本。`sizeof(Collector) < kManagementBytes` 由纯编译断言维护；不存在第二份原型大 mapping 或后台原生指针解引用。

## 6. 给命令与准备编排的接线契约

`WorldRecording` 必须由稳定的 `shared_ptr` owner 持有到 storage 和 writer 真正 Closed；其中依次包含 ledger、storage、identity、writer、collector。准备线程按以下顺序工作：

1. 使用上下文 ledger 准备 storage 与 identity；失败保留首因并有界收口。
2. 使用布局表、双层观察 registered mask 和实际限额构造 manifest，打开 writer。
3. 使用主线程预先取得的 `WorldMemoryReader()` 值、同一 storage/identity 和新 recording generation，在后台构造堆上 Collector。
4. 在 Manager 命令边界调用 `AttachPreparedWorld`。成功表示已接收上下文；**下一次最外层 Manager 入口才 Activate**，避免补造当前半个窗口。不能在 Preparing/Recording/Closing 时替换未收口的上下文。
5. stop 调用 `StopWorldRecording`；主线程只做配对收束，最后一个 outer/观察作用域/pending 归还后才向 storage 发停止信号。后续读取 writer/storage 原子状态确认 drain/flush，不能在 DllMain join 或提前销毁上下文。

配置 `world_record.enabled=false`、`output_dir="isaac-cs/world"`、`close_timeout_ms=10000` 已可解析；顶层 C/S 关闭不注册 Hook，world 允许注册也不自动开始录制。此次尚未增加 `cs world start/stop/status/checkpoint` 命令或后台 Preparing worker。直接窗口关闭的完整退出协调仍按 ST8 保留 UNKNOWN，没有新增 Shutdown Hook，也没有声称自动获得十秒排空窗口。

## 7. 构建与验收状态

可重复纯编译入口和日志见 [构建记录](../../../artifacts/repentogon/tx-04-08-collector-build/README.md)。world/core 静态库、主线程桥/相关 DLL 单元以及 C++17 绑定/生成 wrapper 均完成纯编译；SDK 自身警告如实保留。

完整 DLL 构建仍遇到既有 `ASMPatcher.hpp:302` 的 `friend static` C2216，发生在未修改的 `odd_patch.cpp/host_patch.cpp`；未修改 SDK、未消费旧 obj 伪造链接成功。必要纯编译脚本复制 CMake 生成工程，只保留本轮 DLL 编译单元并执行 `ClCompile`，不链接或部署。

**测试和实机全部 NOT_RUN。** 没有新增实现后单元测试，没有运行 smoke/probe/replay 或局部 E2E。2,000 次、双进程比较、原调用保持、实际线程/内存/时延/池复用/文件失败/直接退出均留给完整 tx-04-08 的唯一统一整体 E2E，不沿用原型 PASS。
