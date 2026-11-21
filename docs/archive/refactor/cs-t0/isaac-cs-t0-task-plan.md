# Isaac C/S T0 研究原型详细任务书

> 2026-10-12 规范修订：先读[现行项目规范](../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，只做一次覆盖整个目标的最终测试验收，探讨过程不得测试；逆向与测试可以由同一智能体完成。本文各阶段的功能目标继续作为任务资料；旧 prerequisite/gate、哈希绑定、预验证、重复验收与独立角色要求不再生效，历史执行记录不改判。

## 文档状态

2026-10-20 当前推进（tx-01-98）：**用户确认 T0-07 为 PASS。** T0-07操作9真实 Authority 的 B 输入停发实验已完成。最终有效 invocation 使用批准 unpacked 目标、显式 v5 2/64/0/64/258、一次 New Run 和 controller 前已开始的 bounded A 物理动作；生产脚本源停发 1/5/30/120 Authority tick，effective 缺口按 lead=2 对齐，B 中性、恢复、edge 5/5 exactly-once、120-tick timeout/recovery、A 实际位置变化和 70 槽恢复通过，统一报告 **24/24 PASS**、controller exit0。前三次失败（错误 GameExe、A 动作晚到、A 穿门死亡）原样保留；world record 仅 partial `NOT_READY` Player 字段证据，无 complete world snapshot；脚本停发不冒充真实网络断线。见[结果](t0-07/tx-01-98-results.md)、[设计](t0-07/tx-01-98-source-stop-e2e.md)和[静态审阅](t0-07/tx-01-98-static-review.md)。

2026-10-20 当前推进（tx-01-97）：T0-07操作8的生产故障与边界矩阵已完成，覆盖A/B conflict、source sequence极值、queue/history、future/expired/late/edge、timeout、tick exhaustion、identity/encoding/generation、phase/return/publication和collector gaps。最终纯编译成功，统一离线验收 **27场景、183命令、184/184检查PASS**；真实游戏/Hook/采样内存/事件环/网络未运行。见[结果](t0-07/tx-01-97-results.md)、[覆盖矩阵](t0-07/tx-01-97-fault-matrix.md)和[审阅](t0-07/tx-01-97-static-review.md)；操作9随后由tx-01-98完成。

2026-10-20 当前推进（tx-01-96）：T0-07操作7已完成CLI/manifest到Native运行输入的配置贯通：严格range/unknown/conflict/null处理、IPC v5、初始化冻结、Participant/Clock/Pipeline/target/script实际生效及EventKind36。最终纯编译成功，统一离线验收 **20配置case、9场景、83命令、104/104检查PASS**；真实游戏/Hook/采样内存/事件环/网络未运行。历史编译和场景失败保留。见[结果](t0-07/tx-01-96-results.md)、[实现](t0-07/tx-01-96-runtime-config.md)和[审阅](t0-07/tx-01-96-static-review.md)。操作8–9仍待实施。

2026-10-20 当前推进（tx-01-95）：T0-07操作6的缺失输入、源静默timeout诊断与恢复已完成生产Participant/tick receipt/Native接入；`max_input_age_ticks=0`、固定120 Authority tick诊断阈值、独立有效到达和终态/代际规则落地。Native与离线程序最终纯编译成功；修正首轮测试场景后的整体离线验收为 **10场景、146命令、122条批量tick记录、152/152检查PASS**。首次FAIL与编译失败保留；真实游戏/Hook/采样内存/事件环/网络/世界动作未运行。见[结果](t0-07/tx-01-95-results.md)、[实现](t0-07/tx-01-95-missing-input-timeout-recovery.md)和[审阅](t0-07/tx-01-95-static-review.md)。操作7–9仍待实施。

2026-10-20 文档更新（tx-01-94）：已将T0-07操作6–9的具体工作、已有基础、分析分工、参数约束、故障矩阵和实机停发口径落实为[剩余操作计划](t0-07/tx-01-94-remaining-operations-plan.md)。本次只改文档，未实施操作6–9、未构建或运行测试/游戏；最新实现结果仍为tx-01-93。

2026-10-20 当前推进（tx-01-93）：T0-07操作5生产pipeline与Native接入已完成，post-state复制/缺口在tick提交之前，值与本地处理回执在提交之后发布；不计算或校验哈希。GPT-6 Luna / max结论经主智能体审阅；Native最终纯编译成功，唯一离线验收 **36场景、254/254检查PASS**，120条连续记录。真实游戏/Hook/采样内存/网络未运行，操作6–9未实施；见[结果及复现](t0-07/tx-01-93-results.md)和[审阅](t0-07/tx-01-93-static-review.md)。下文较早范围保留历史时点。

2026-10-20 当前推进（tx-01-92）：T0-07操作4已完成：A/B共用生产AcceptInput，底层Submit收为私有；GPT-6 Luna / max结论经主智能体逐项审阅。Native最终纯编译成功，唯一统一离线验收 **40场景、303/303检查PASS**，保存4846条准入记录。B仍为脚本源，真实游戏/Hook/网络未运行，操作5–9未实施；见[结果与复现](t0-07/tx-01-92-results.md)和[审阅](t0-07/tx-01-92-static-review.md)。下文较早范围说明保留历史时点。

2026-10-20 当前推进（tx-01-91）：T0-07操作3的逐client输入状态、窗口、edge账本、接收/处理进度与拒绝计数已实现，GPT-6 Luna / max静态分析经主智能体逐项审阅；纯编译成功，唯一离线验收 **22场景、152/152检查PASS**，4357条批量输入记录已保存。真实游戏/Hook未运行，操作4–9未实施；见[结果与复现](t0-07/tx-01-91-results.md)和[审阅](t0-07/tx-01-91-static-review.md)。下文旧推进范围为对应历史时点。

2026-10-20 当前推进（tx-01-90）：T0-07操作2的AuthorityTickClock及Native精确入口/正常返回配对已实现，Luna/max静态结论经主智能体复核；纯编译成功，唯一离线验收 **30场景、175/175检查PASS**。真实游戏/Hook时序未运行，操作3–9未实施；见[实际结果](t0-07/tx-01-90-results.md)和[主智能体审阅](t0-07/tx-01-90-static-review.md)。

2026-10-19 当前推进（tx-01-89）：T0-07 操作 1 AuthoritySession、冻结身份和 Native 准入/退休接入已实现，Native 直接纯编译成功；唯一离线生命周期验收 **33场景、303/303检查 PASS**。真实 Hook/Join/线程时序/恢复未运行，操作 2–9 未在本次实施；不宣称整个 T0-07 完成。见[结果及边界](t0-07/tx-01-89-results.md)。

2026-10-19 当前结果（tx-01-88）：用户永久移除真实 Online/Isaac Sync 的 W01.isolation 测试；保留不进入/不复用原版联机状态的架构约束。W08 全量 replay 性能修复、一次统一离线验收 PASS；2.3 GB 输入两次重编码 14.50/13.58 秒，完整 33,050 条记录和重复输出字节一致。当前清单累计 **32/32 PASS**，30 项历史沿用、2 项 W08 本次检查，无剩余测试项，见[结果与边界](t0-06/tx-01-88-e2e-results.md)。下文状态为历史时点。

2026-10-19 当前结果：tx-01-87 剩余授权 E2E **6/6 PASS**，累计 **31 PASS / 0 FAIL / 2 NOT_COVERED**，其中 25 项沿用历史出处。新采真实 Continue 加载与 run 退出，939 个普通样本阶段匹配、70 槽恢复；旧失败保留且不重发故障。真实 Online/Isaac Sync 隔离与全量 replay 仍按用户选择未覆盖，全局 INCOMPLETE，当前授权范围已完成。见[证据、结论边界与复现步骤](t0-06/tx-01-87-e2e-results.md)。下文状态保留对应历史时点。

2026-10-19 当前专项：tx-01-85 retained Player／Replace 与 GridFree **2/2 PASS**，旧采集中的三次真实 Replace 完成正确汇总，新进程补齐自建岩石实际 GridFree 与退休/槽退出证据，70 槽恢复。tx-01-84 FAIL 和 tx-01-83 总账 25 PASS / 1 FAIL / 7 NOT_COVERED 均保留，完整 T0-06 尚未完成，见[专项结果和剩余范围](t0-06/tx-01-85-e2e-results.md)。下文准备与未运行状态仅代表对应历史时点。

tx-01-83 准备完成（2026-10-19）：控制器安装诊断（原始阶段/Win32 错误与退出后末次事件）和执行器异常部分汇总已实现，**控制器直接纯编译成功，E2E_NOT_RUN**。Python 仅静态审阅；Native/Helper/协议及游戏未操作，tx-01-82 FAIL 与未覆盖账本不重判。新诊断不能补回旧事件缺失字段，失败根因仍未知；后续统一验收需另行确认安排，见[实现与构建记录](t0-06/tx-01-83-diagnostics-and-finalization.md)。

tx-01-82 实际结果（2026-10-19）：宿主统一 E2E 已执行一次并以 **FAIL** 完成报告。已保存精确 Game frame 6000 比较及 field-read/source-changed 两个受控故障证据；event-overflow 初始化 Hook 事务失败，后续 wrong-thread/root-changed 未进入，primary 场景按用户选择保留未覆盖。最终汇总未执行，冻结总表仍为 21 PASS / 12 NOT_COVERED，不能把部分证据改成整体通过。下一步建议补齐安装失败诊断，尚未实施或重新测试，见[完整结果](t0-06/tx-01-82-e2e-results.md)。

tx-01-81 运行更新：直接 Win32 枚举只看见 3 个进程且无游戏候选，首步 FAIL、未注入，十个测试仍未执行；准备在确认后修正为沙箱外完整统一执行，见 [tx-01-82](t0-06/tx-01-82-host-execution.md)。

tx-01-80 运行更新：首个进程识别步骤 FAIL，未启动 controller 或注入，十项测试均未完成；tx-01-81 已准备直接 Win32 枚举/候选诊断修正，尚未运行，见 [失败与边界](t0-06/tx-01-80-e2e-results.md)。

T0-06 当前推进 tx-01-80（2026-10-19）：剩余测试手段和场景 Mod 已实现、纯编译完成，统一实机验收未开始。用户本轮排除 W08 全量 replay 与 W01 真实 Online/Isaac Sync 隔离，目标为其余十子项；不改写 tx-01-79 的历史 21 PASS / 12 NOT_COVERED。详见 [tx-01-80 准备及范围](t0-06/tx-01-80-remaining-test-design.md)。

- 状态：**Draft / 当前阶段执行计划**。
- 日期：2026-08-18。
- 上位设计：[Isaac C/S 权威联机总体架构规划](../isaac-cs-architecture-plan.md)。
- 当前产品基线：Isaac Sync 1.0.4 的现有实现与文档保持不变。
- 目标形态：Listen Server；房主运行真实 `isaac-ng.exe`，同时承担 Authority Server 和逻辑 Local Client。

### 当前门执行记录（更新至 2026-08-27）

- T0-01：`PASS`，只读 C/S 独立证据基线已完成。
- T0-02：`PASS`。packet pairing revision `t0-02-packet-pairing-v2` 使用各端本地事件序号锚定共同采集内部窗口；内部未知 Read、可靠 Send 缺失和非法 send mode 均为零。`sendMode=0/1` 的未送达 Send 只作为不可靠丢包诊断；“每个成功不可靠 Send 都必须被 Read”登记为 `REJECTED_HYPOTHESIS`。正式 gate decision SHA-256 为 `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185`。
- T0-03：保留既有 scoped `PASS`（用户修订矩阵；两项显式豁免）。该结论证明已登记 Authority 切片的顺序、线程与恢复安全，不再解释成“完整游戏事件循环”或“真实 input sample/apply 已闭合”。正式 decision SHA-256 仍为 `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB`。
- T0-03R：V5 静态 `PASS_STATIC`，实机派生 loop gate 为 7/7 `PASS`。冻结 ledger 保持历史 `BLOCKED_BY_EVIDENCE` 且不覆盖；gate-local `activeInputAllowed=true` 只代表 loop prerequisite 已闭合。
- T0-04：正式 `PASS`。七个工作包、20 份 source provenance replay、18+2 matrix、repeatability comparison 与 final gate 均已闭合。approved-unpacked observed target 由只读 equivalence document 绑定 canonical target；verification v2 为 27/27，SHA-256 `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317`，`semanticContractComplete=true`、`runtimeContractComplete=true`。正式 matrix 为 primary `18/18`、extra `2/2`，comparison `1/1`；final schema-4 decision 为 `1/1 PASS`，SHA-256 `927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C`，`t004Unlocked=true`、`activeInputAllowed=true`。既有 source、原 analysis 与历史 FAIL provenance 均保持不变；T0-04 无剩余测试，按依赖可进入 T0-05。完整路径与证据摘要见 [isaac-cs-t0-03r-04-status.md](isaac-cs-t0-03r-04-status.md)。

本文定义 T0 的研究、原型、证据和验收顺序。它不是 wire、IPC 或 Lua API 的正式契约。所有未来类型、消息和目录名称在 T0 完成前只属于实验域，不得复用 `ISCS` protocol 5 身份或写入现有正式 ABI。

## 1. T0 目标

T0 要回答的不是“能否立即做出可玩的 C/S”，而是以下最小闭环是否成立：

```text
冻结 CustomAuthorityServer / CustomAuthorityClient 模式
    -> 识别真实 gameplay tick
    -> 主机与远端输入进入同一 future-tick scheduler
    -> Authority 每 tick 不等待全员地推进一次
    -> 双端在同一 capture phase 计算 canonical state hash
    -> analyzer 能定位首次输入、tick 或状态分歧
    -> 切回 OriginalOnline 时 C/S 完全 disarmed
```

### 1.1 T0 完成标准

全部条件必须满足：

1. Listen Server 能把 Local Client 和一个 Remote Client 的输入放入同一验证、sequence、target tick 和缺失输入管线。
2. 单个远端缺包、迟到或断开时，Authority gameplay tick 不因 C/S input ready 条件全局停住。
3. 双方能按 session、server tick 和 capture phase 交换并比较 canonical state hash；发生分歧时能给出第一个可解释窗口。
4. `OriginalOnline` 模式不创建 C/S session、不收发 C/S packet、不改变原版 channel `0`、ready stall 或 frame increment。
5. 所有新 Hook/探针均按目标构建和原始字节 fail closed，卸载后恢复目标值。
6. 形成一份 T0 验收报告，明确 T1 的 go/no-go 结论和未解决风险。

### 1.2 T0 不要求

- 不要求客户端从 snapshot 重建或写回实体。
- 不要求玩家获得可接受的联网体验。
- 不要求预测、插值、reconciliation 或 rollback。
- 不要求切房、换层、重开、死亡复活或 late join。
- 不要求普通 Lua Mod 兼容。
- 不要求 Dedicated Server、host migration、EOS 或单 peer 多本地玩家。
- 不要求无损复用旧 Steam P2P；Loopback/FaultInjection transport 足以完成大部分 T0 门禁。

## 2. 执行规则和证据规范

现行执行约定如下，其优先级高于本任务书后续保留的旧阶段流程：

1. 不创建或要求任何 gate、解锁决策或哈希校验。
2. 每个 `tx-0x-0x` 先完成整个目标，再执行一次整体验收；过程中的分析、讨论和实现不运行测试。
3. 不要求逆向/测试独立、作者分离、子智能体或预封；同一智能体可以完成全部工作。
4. 直接记录目标、实际观察、结果与未决问题，不以历史矩阵、SHA、decision 或 `*Unlocked` 字段控制推进。

### 2.1 不破坏现有产品

- T0 使用独立实验 build identity、日志名、IPC 名和协议 magic。
- 不修改 `ISCS` protocol 5、channel `0xFD`、ABI 5 或七项 `Isaac.Sync` Lua API。
- 不把 T0 逻辑接入 `GroupSessionProtocolEngine`、`FrameStore` 或现有 all-slot ready gate。
- 所有实验入口默认 disarmed，只有显式 T0 启动参数、目标构建匹配和模式确认后才能改变实验状态。
- `OriginalOnline` 对照运行只允许观测；不得注入 synthetic input、跳过 update 或产生 C/S packet。

### 2.2 证据标签

每个结论在任务产物中标记：

| 标签 | 含义 |
| --- | --- |
| `STATIC_CONFIRMED` | 有唯一特征、反汇编、ABI 和数据流依据 |
| `RUNTIME_OBSERVED` | 有带构建、RVA、线程、sequence 的运行日志 |
| `DUAL_ENDPOINT_CONFIRMED` | 两个独立进程的日志可以按协议 identity 交叉对应 |
| `DESIGN_ONLY` | 仅为待实现设计，不得写成逆向事实 |
| `REJECTED_HYPOTHESIS` | 已有反例，保留失败证据和原因 |
| `UNKNOWN` | 尚无足够证据，不允许以命名替代证明 |

单次成功样本不能自动升级为跨版本契约。VA 只用于对应进程的原始日志；文档和代码定位使用 RVA、特征和目标字节。

### 2.3 实验产物

未来 T0 实现统一使用以下非正式产物布局：

```text
artifacts/cs-t0/<run-id>/
├─ manifest.json
├─ authority.jsonl
├─ client.jsonl
├─ analyzer.json
├─ target-bytes-before.json
├─ target-bytes-after.json
└─ notes.md
```

`run-id` 使用 UTC 时间和 case id，不包含用户名或 SteamID。`manifest.json` 至少记录：

- 游戏 PE timestamp、image size、`.text` SHA-256；
- Native/探针 build hash；
- 目标 RVA、原始字节和已安装 Hook；
- process id、creation time、角色和测试 case；
- transport、delay、jitter、loss、duplicate、reorder 参数；
- 输入脚本、run seed、Mod 列表和游戏配置摘要；
- 开始/结束原因及是否完成安全卸载。

JSONL 公共字段至少包含：

```text
schemaVersion, eventSequence, qpc, threadId,
processRole, networkMode, sessionId,
serverTick, gameFrame, onlineFrame,
eventKind, result, errorCode
```

身份默认匿名化为本次实验的 `Endpoint A/B`。只有原始本地 artifact 可以保留完整 SteamID，进入文档前必须截断或哈希。

### 2.4 通用停止条件

出现任一情况立即停止当前 active 实验，恢复已安装目标，并保存失败 artifact：

- 目标构建、特征、原始字节、IAT/vtable owner 或 ABI 不匹配；
- 原本应为主线程的游戏对象/Lua 操作出现在未知线程；
- Hook 链已有未知第三方修改且无法证明调用顺序；
- event ring/JSONL sequence 出现无法解释的缺口；
- 原版 channel `0` 参数或 payload 被 T0 改写；
- target restore 失败、进程异常、Lua VM 已退役仍被访问；
- 为了让测试通过而需要吞掉状态冲突、放宽 decoder 或跳过未知实体。

不得通过强制写内存、修改返回值或继续运行来“收集更多数据”，除非该写入本身是当前任务已批准且具备回滚的实验内容。

## 3. 任务依赖

任务门按以下顺序推进：

```text
T0-01 Evidence Baseline
  -> T0-02 Mode Isolation
  -> T0-03 Tick/Update Boundaries
  -> T0-04 Canonical Input + Loopback
  -> T0-05 Local Co-op / Virtual Device
  -> T0-06 Entity Inventory + State Hash
  -> T0-07 Authority Scheduler
  -> T0-08 Minimal Protocol
  -> T0-09 Dual-process Fault Matrix
  -> T0-10 T0 Report / T1 Gate
```

任务可以在门内并行分析不同候选，但不能绕过前置门进入后续 active 写入实验。每项任务只有 `PASS`、`FAIL`、`BLOCKED_BY_EVIDENCE` 三种结论；“大致可用”不能作为下游依赖。

## 4. T0-01：C/S 独立证据账本和目标构建基线

### 目的

为所有后续逆向和实验建立唯一、可复定位、可撤销的目标基线，避免把历史 VA、旧构建和设计假设混入 C/S 结论。

### 前置依赖

- 无。
- 使用仓库当前支持的 `isaac-ng.exe` 构建和现有只读研究工具。

### 操作

1. 记录目标 PE machine、timestamp、image size、section 表和 `.text` SHA-256。
2. 从现有文档重新定位并验证：manager pointer、network backend、Steam context、输入推进、`RVA 0x5110AB`、Lua creator/pcall/close。
3. 对每个目标记录 RVA、完整特征、原始字节、预期 call target、ABI、线程和证据来源。
4. 建立新的 C/S 候选账本，先将 gameplay tick、input sample、entity update、render、local co-op manager、entity container 标为 `UNKNOWN`。
5. 将现有 framestate/checksum/RNG history 仅登记为诊断入口，不登记为 snapshot/apply。
6. 执行一次只读启动/退出 smoke，记录模块基址变化并证明全部定位按 RVA 重定位。

### 产物

- `cs-t0-target-ledger`：目标构建、RVA、ABI、特征和证据标签。
- 一份 baseline artifact，包含启动、运行、退出和目标字节前后快照。
- `known / unknown / rejected` 问题表。

### 通过标准

- 所有既有关键目标在唯一位置匹配，call target 和原始字节与现有证据一致。
- 不使用历史 VA 作为实现常量。
- 候选账本明确区分已确认事实和待验证名称。
- 只读 smoke 没有 patch、断点、事件缺口或目标进程异常。

### 失败/停止条件

- 任一正式关键 site 零匹配、多匹配或 call target 改变。
- 当前游戏构建不在已支持 fingerprint 中。
- 现有文档与实时目标冲突且无法由版本差异解释。

### 后续依赖

- 解锁 T0-02。

## 5. T0-02：模式仲裁和原版路径零干扰

### 目的

证明未来 C/S 可以在同一交付中保持 disarmed，并且 `OriginalOnline` 行为不会被 C/S 状态、transport 或 Hook 分支改变。

### 前置依赖

- T0-01 `PASS`。

### 操作

1. 静态定位能够区分菜单、offline/local run、原版 online session 和 manager generation 的信号；不得只依赖窗口标题或单个易变布尔值。
2. 设计进程级 `ModeLatch` 实验状态：`Unresolved -> Offline | OriginalOnline | CustomAuthorityServer | CustomAuthorityClient -> Retired`。
3. 规定模式只在 run 建立前冻结；任何冲突信号进入 `Retired/Disarmed`，不自动选择 C/S。
4. 使用单一 Hook Owner：同一 callsite 只安装一个 wrapper；wrapper 在非 C/S 模式直接调用已保存原函数。
5. 建立三组只读对照：offline run、OriginalOnline 双端、菜单/退出重建。
6. 对 OriginalOnline 记录 channel `0` Send/Available/Read、ready stall、input apply、frame increment、Lua callback 和 manager lifecycle。
7. 在“装有 T0 Native 但 C/S disarmed”和“未装 T0 Native”的可比运行间做事件/统计差异分析。

### 产物

- `ModeLatch` 状态图、输入信号和冲突处理说明。
- OriginalOnline A/B 对照日志与 analyzer 报告。
- 每个 wrapper 的 passthrough 证明和目标恢复记录。

### 通过标准

- OriginalOnline 期间 C/S send/read/drain/session/input queue/state hash 事件均为零。
- channel `0` 参数和 payload 无修改，成功/失败结果由原函数产生。
- ready stall 时仍遵循现有证据：不进入成功 candidate、不递增 frame。
- 模式在同一 generation 中不发生热切换。
- `lua_close`、返回菜单或 manager 重建后 ModeLatch 退役，旧 C/S token 不可恢复。

### 失败/停止条件

- 无法用两个以上相互验证的信号区分 OriginalOnline 与 local run。
- 安装 wrapper 本身改变原版调用次数、线程或返回值。
- 必须 patch 原版 Online Manager 才能让 C/S 保持 disarmed。

### 后续依赖

- 解锁 T0-03。

## 6. T0-03：完整 gameplay tick、输入、实体更新和渲染边界

### 目的

确定 Authority Server 每 tick 应在何处收集输入、应用输入、执行一次游戏逻辑、捕获状态，以及客户端未来能否分离 gameplay update 与 render。

### 前置依赖

- T0-02 `PASS`。
- 目标构建和主线程身份已固定。

### 当前执行状态（2026-08-20）

- 当前 boundary ledger 下 `normal`、`input`、`pause`、`room-transition`、`menu-return`、`lua-rebuild` 的同账本 C0/C1 已通过；Hook 均已恢复，历史 ABI 崩溃保留为失败证据。
- `focus-loss` 因当前游戏失焦即进入 pause，`low-fps` 因用户明确停止本轮验证；二者登记为显式用户豁免，不作为独立 runtime PASS。
- `lua-rebuild` 固定触发为：主菜单至少启用一个普通 Mod，通过 Mods 菜单重新选择该 Mod；进程保持存活、不进入或重建 Run。实机已精确观察到 `LuaClose(g) -> LuaCreated(g+1) -> Retired(g, reason=LuaRebuild)`，旧的泛化 retirement 判据已废止。
- boundary IPC v3/analyzer v4 新增主循环 gameplay suppression 和唯一 `MC_POST_NEW_ROOM` 事件；`room-transition` 必须证明 suppression 后到 new-room 之间零 post-update，不能再只依赖用户“穿门”操作说明。
- 新探针已通过 Rust、x86 Debug/Release-LTCG Native harness、当前 patched variant 静态 inspect 及六组新版实机 C0/C1。旧 room artifact 用 v4 重放仍按预期失败在 `room-loading-boundary`，新版 room artifact 则以 `roomLoadingProof=true` 通过。T0-03 在修订矩阵下为 `PASS`；Replica separation 与正式 FPS 阈值保持 `UNKNOWN`，本轮不进入 T0-04。

### 操作

1. 从主循环、窗口消息、现有输入推进和 Lua `MC_POST_UPDATE` 相对顺序向上追踪候选 gameplay update。
2. 定位并记录以下候选：
   - 物理输入采样；
   - local co-op input state 写入；
   - gameplay tick enter/exit；
   - entity container update enter/exit；
   - spawn/remove drain；
   - Lua simulation callbacks；
   - render enter/exit；
   - pause、加载、切房和菜单分支。
3. 先使用被动探针，只改写已验证 call displacement/IAT/vtable，并在 wrapper 中只记录，不改变返回值。
4. 每个候选记录 QPC、线程、调用深度、返回地址、game frame、online frame、实体计数和前后状态摘要。
5. 覆盖以下场景：普通 local run、移动/射击、pause/unpause、低帧率、窗口失焦、进入新房、退出菜单、Lua VM 重建。
6. 验证一次成功 gameplay tick 是否恰好出现一组严格顺序事件，以及 render 是否可能在没有 gameplay mutation 的情况下继续。
7. 将 `RVA 0x5110AB` 与新边界比较；明确它只属于现有 online input/update 链还是可以复用为更广泛的 C/S 阶段。

### 产物

- 主线程单 tick 全序图。
- gameplay/input/entity/render 候选账本及证据标签。
- 每个场景的 passive JSONL 和顺序 analyzer。
- “Replica 客户端暂停 gameplay、继续 presentation”的可行性结论；允许为 `UNKNOWN`，但必须给出下一实验入口。

### 通过标准

- 找到一个成功 gameplay tick 恰好进入一次、退出一次的候选边界。
- 找到 input sample 早于 input apply、input apply 早于对应 gameplay mutation 的证据。
- 找到稳定的 post-update capture phase，实体容器不处于遍历/析构中。
- 所有游戏对象候选事件位于同一已确认主线程；后台网络事件不直接访问对象。
- pause、加载或无 gameplay tick 时不会误增 future `server_tick`。
- target restore 后所有 site 回到原始字节。

### 失败/停止条件

- 候选函数只返回部分 update，无法与完整 tick 建立一一关系。
- update 在多个线程并行修改同一实体集合而缺少可观察 barrier。
- 记录实体摘要本身引起重入、Lua 调用或容器失效。

### 后续依赖

- 解锁 T0-04。

## 7. T0-04：完整输入规范和 Local Client loopback

> T0-03R V5/T0-04 的时间线、不可覆盖 artifacts 与失败停点见 [isaac-cs-t0-03r-04-status.md](isaac-cs-t0-03r-04-status.md)；截至 T0-04 的可复用逆向事实、反证和 T0-05 输入见 [isaac-cs-t0-04-reverse-handoff.md](t0-04/isaac-cs-t0-04-reverse-handoff.md)。下方 2026-08-20/22 的验证数量是历史快照，不覆盖文首正式 `PASS`。

### 目的

建立不依赖原版网络输入包的 `InputStateDraft`，并证明房主输入可以先排队、后在指定 Authority tick 应用，而不是直接改变世界。

### 前置依赖

- T0-03 scoped `PASS` 保留，不视为完整 loop/input 证明。
- T0-03R loop-envelope gate 必须为 `PASS`，并已确认真实 InputDevice query owner、concrete vtable/ABI、controller ownership、UI/window 分区及 Authority mutation envelope。
- 在 T0-03R gate 前只允许静态检查、纯内存实现、自动 harness 和被动实机探针；主动输入抑制/注入必须 fail closed。

### 操作

1. 逐项记录移动、瞄准、射击、炸弹、主动道具、卡牌/药丸、丢弃、菜单外 gameplay 特殊动作。
2. 同时覆盖键盘和一个手柄；记录 digital bit、analog range、dead zone、方向归一化和设备切换。
3. 区分连续状态与边沿动作：
   - continuous：move、aim、shoot held；
   - edge：bomb、item、card/pill、drop 等一次性动作。
4. 设计 `InputStateDraft` canonical 规则：固定字段顺序、整数/定点量化、保留位为零、非法组合拒绝；不直接传输进程内 input device 内存。
5. 构建纯内存 `LoopbackTransport` 和有界 input queue 实验；Local Client 也生成 `client_id`、sequence 和 `target_tick`。
6. 在显式 T0 server 模式下拦截一组本地输入，阻止其立即进入被测试的 apply 路径，再从 queue 在目标 tick 注入同一输入。
7. 验证 hold/release、同时按键、快速点击、analog 极值、焦点丢失和 pause 前后行为。
8. 记录原始采样、canonical input、排队、应用和游戏可见结果的关联 identity。

### 当前冻结的实验接口（2026-08-20）

- `InputStateDraftV1` 为 24B LE：`u16 version=1`、`u16 length=24`、`u32 heldFlags`（仅 bit0 `shootHeld`）、`u32 edgeFlags`（bit0 bomb、bit1 item、bit2 card/pill、bit3 drop）、四个 `i16` Q15 轴、`u32 reserved=0`。设备类型和进程指针不进入 canonical bytes。
- `InputCmdDraftV1` 为 64B：16B session、`u64 clientId/sequence/targetTick`、24B state。
- raw axis 先 clamp 到 `[-1,1]` 并向零截断为 Q15，再做整数 radial dead-zone `4096`、死区外线性满幅重标、向零截断和单位圆压缩；非有限值、unknown bits、`-32768`、reserved 非零及 shoot/aim 冲突全部拒绝。
- 实验常量：lead `2`、future window `64`、transport/input queue capacity `256`、drain budget `64/tick`、dedupe history `4096`。这些常量必须写入每个 manifest，并标记为非产品契约。
- 同 identity 同内容幂等；同 sequence 内容冲突安全退休；expired/too-far/full/stale 不推进 ledger 或改变世界。同 target 连续量取最高 sequence、不同 edge 合并、相同 edge collision 拒绝，每 target 最多一次 adapter apply。
- `input-inspect/input-smoke/input-analyze/input-compare/input-gate` 使用独立 `ICSI04V1` IPC；该路径不修改 T0-02 `ModeLatch`。主动 probe 仍要求一次性 arm token并保持 fail closed。2026-08-22 新增的 `input-smoke --probe passive` 不接受 arm token：它重放派生 V5 gate 后启动 schema-2 observation-only runtime，在同一事务中组合 V5 32 槽与 input 11 槽，只记录 original 调用/结果与 V5 context，并生成永久非 gate-eligible 的 `PASSIVE_INPUT_RUNTIME` artifact。它不执行 suppression、synthetic apply、queue/world 写入或 `ModeLatch` 修改。

### 当前实际验证状态（2026-08-22）

- Rust canonical/queue/planner、artifact analyzer、schema-2 passive input IPC/writer、additive schema-3 raw/apply IPC/writer/analyzer 和 Win32 Native harness 已实现；首次 schema-3 canary 修复后 Rust 全仓 167 tests、全目标 clippy、Native Debug 与 Release/LTCG 门均通过。
- `MC_INPUT_ACTION` nil-return witness 已完成人工被动矩阵。键盘与手柄分别确认 ownership、移动、射击和同时输入的查询包络/目视相关；另覆盖暂停、房间过渡和 run teardown/menu。焦点切换按已知自动暂停行为标记为 SKIPPED，不计 PASS；同一 Run 的键盘/手柄 device-switch 不可测。
- 新增 `input-import-witness` window/controller 过滤器。实际 keyboard `c=0`、window `1783..1794` artifact 为 `WITNESS_QUERY_ONLY`，217 个事件，analyzer PASS，但 `eligibleForActiveGate=false`。
- 新增 `input-rpm-inspect/capture/analyze`。该观察器只申请 query/VM_READ/synchronize 权限，对 input manager 活跃 span、concrete vtable、cached logical magnitudes、game/player vector 与 assignment key 做双采。当前 schema-v2 静置 capture 为 287/287 coherent、0 drop，keyboard key `0` 每次唯一匹配 Player key `0`。
- RPM 证据类固定为 `RPM_DIAGNOSTIC_ONLY`，不进入 canonical/gate。远端结构没有 generation/sequence，不能排除 ABA、不能捕获 poll 间短 edge，也不能归属 loopIteration/Authority tick。
- 独立 T0-03R V5 Native loop runtime 已完成 7/7 实机 gate。其 32 槽与 schema-2 input observer 的 11 个 vtable 槽现在可在一次 patch transaction 中组合安装并在失败时整体回滚；正常卸载按 input 11 槽恢复/drain → V5 32 槽恢复/drain 分阶段执行。V5 IPC、32 槽 mask、事件和 analyzer 语义保持不变。input ring 容量为 65536、event 固定 80B；manager primitive 仅在完整 binding-table 双快照唯一映射到 action `0..11` 且不与 `12+` alias 时持久化。目标有 debugger attached 或存在冲突模块时 runtime 必定拒绝初始化。
- `loop-gate` 与 `input-gate` 不再只信任可编辑的派生 JSON：gate 会重放绑定 run、逐项核对 checks 与 manifest/events/analysis SHA；input repeatability 也从 matrix 中定位两个 artifact 并重新比较。
- T0-03R 派生 V5 loop gate 已 7/7 `PASS`；冻结 ledger 的历史 `BLOCKED_BY_EVIDENCE` 保持不覆盖。首次 observation-only keyboard canary artifact `20260822T073431Z-t0-04-input-passive-keyboard-t0-04-passive-keyboard-canary-01` 已完成：457,570 条 input events、21/21 PASS；companion V5 为 271,755 条 events、18/18 PASS；11+32 槽逐项恢复，双方 main thread 均为 4976，无 gap、overwrite、Failure 或 wrong-thread。该 artifact 为 `PASSIVE_INPUT_RUNTIME` 且 `eligibleForActiveGate=false`。T0-04 仍缺 raw/apply 被动闭环与新的主动实验授权，因此 local suppression、synthetic apply 与 loopback 继续禁止。
- 首次 controller canary artifact `20260822T080816Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-01` 在 `Ready` 前 fail closed：Gamepad controller `0` / device key `1` 已通过 unique-owner 前检，但 Helper `--inject-input` 返回 exit 4。input/loop 事件和 mask 均为零，input 11 槽与 V5 32 槽 before/after 一致，Native 未加载、游戏继续响应。该历史 FAIL 不覆盖、不重试；随后取得新的单独确认，并在允许 Helper 取得精确注入权限的执行上下文中从头前检后完成 `-02`。
- 用户重新确认后的 controller canary artifact `20260822T081545Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-02` 已在精确权限上下文中完成：input 118,812 events、21/21 PASS；companion V5 170,955 events、7,170 个完整 loop、18/18 PASS；11+32 槽逐项恢复，双方 main thread 均为 20256，无 gap、overwrite、wrong-thread、Failure、非零 error 或 Native 残留。该证据仍为 `eligibleForActiveGate=false`；随后取得独立确认并完成下一条 rapid-tap 窗口。
- Keyboard unique-owner rapid-tap artifact `20260822T083151Z-t0-04-input-passive-rapid-tap-t0-04-passive-rapid-tap-canary-01` 已完成：input 122,240 events、21/21 PASS，Bomb/Item/PillCard/Drop 的 action 8–11 均观察到 true 结果；companion V5 223,306 events、11,508 个完整 loop，17/18 仅 `scenario-evidence` FAIL，其它检查通过。43 槽逐项恢复，双方 main thread 均为 20256，无 gap、overwrite、wrong-thread、Failure、非零 error 或残留。该证据仍为 `eligibleForActiveGate=false`；下一工作转回定位 raw sample 与真实 offline/local apply，不自动开始其它实机场景。
- raw sample 与自然 offline/local consumer 的 v1 固定目标 catalog 曾冻结：Keyboard/Gamepad physical poll 建立 sample generation，action-indexed `device+0xC0/+0xC4[0..11]` 在 common-device-update 返回后双读；Player `Update` 只作 owner envelope，六个 natural consumer call 分别覆盖 movement、bomb、item、pill/card、shooting intent 与 drop。catalog SHA-256 为 `C01E325A592876D0D2C0EC1354EFF1ED72C3B7E4D25E3B4FFAA0212F447A2D0C`，四场景 capture plan SHA-256 为 `DEE4C87298045174F76689D4D5DB4D97AE6F4977303D1E89E84AC598C6847805`。canary-03 已推翻其中 `+0xC0/+0xC4` physical projection 假设；v1 文件不覆盖，状态为 `FROZEN_REJECTED_HYPOTHESIS`，修正必须使用 create-new v2/hash。
- additive `ICSI04V1` schema 3 runtime 已实现：128B event、65,536 槽 ring、V5 32 + legacy input 11 + Player owner 1 + consumer call 6 = 50 槽单事务安装/恢复；artifact class 固定为 `PASSIVE_RAW_APPLY_RUNTIME`，`eligibleForActiveGate=false`。Analyzer 要求 post-poll raw sample、Player-owned nested concrete logical query、natural consumer transaction 和批准 witness 的关联，且 primitive/canonical/command/queue/resolve/applied-state/world-write 字段均为空。action `12+` 从不读取或持久化；manager primitive alias 继续脱敏，但不会错误过滤独立的 action-indexed `0..11` projection。
- 首次 schema-3 `keyboard-held-release canary-01` create-new artifact `20260822T101325Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-01` 在 movement velocity `WorldWitnessBefore` 上 fail closed：Native 按 capture plan 发送 direction 与 velocity witness，Rust allowlist 漏接 velocity。95,866 条 raw 与 126,444 条 companion event 保留；18+32=50 槽完整恢复，游戏响应且无 T0 残留。该历史 artifact 永久 `FAILED`，动作闭环 0/8，不改判不自动重试。
- 同一 artifact 的 `observedRaw=0x1` 又证明 `requiredRaw=0x3` 不能被误读成 Keyboard 场景必须观察 Gamepad。修复后 Keyboard 场景必含 owner bit `0x1`、Controller 场景必含 `0x2`，允许额外已知 source、拒绝未知位；Native/catalog/plan/历史 artifact 均未修改。
- canary-02 前的合约修复自动门为 167/167 Rust tests、`cargo fmt --check`、`cargo clippy --all-targets -- -D warnings`、Win32 Release/LTCG 与两套 Native harness 全部通过；随后在新的单独确认下启动该窗口。
- `keyboard-held-release canary-02` 使用修复后的合约启动并实机确认 `observedRaw=0x1`、movement direction/velocity、六个 consumer 与批准 witness 均可持久化；但 schema-3 raw ring 真实 overwrite=4 后自行 fail closed。85,242 条 raw 与 39,428 条 loop event、50 槽恢复和游戏无残留均保留；窗口在用户动作前退出，闭环仍为0/8，历史 artifact 永久 FAILED。
- canary-02 定量定位为 writer 背压：85,242 条约87.9 MiB在18.013秒产生，原 schema-3 writer直接对 `File` 做大量serde碎片写，落盘约31秒并阻塞下一次drain。修复只将其改为4 MiB `BufWriter<File>`并保留finish/fail显式flush；ring容量、overwrite阈值、controller cursor、Native、catalog和plan均未改变。167/167 tests、fmt、clippy、Release/LTCG与两套Native harness再次通过；随后仅在用户新的单独确认下启动 `canary-03`。
- 用户逐次确认后的 `keyboard-held-release canary-03` artifact `20260822T110518Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-03` 已完成：703,040 条 raw、201,036 条 companion V5、9,160 个完整 loop、overwrite=0、50 槽逐字节恢复，V5 18/18 PASS。action0..7 均有 logical true 与对应 consumer/witness 活动，但 action0..11 共109,920条 RAW_VALUE 全为零，因此结论是安全推翻 v1 raw-source 假设，不是闭环 PASS。
- 静态复核确认 Keyboard/Gamepad concrete poll 不写 `+0xC0/+0xC4`；logical query 直接从 binding 调 primitive query，logical setter 才写 `+0xC0`。旧 raw analyzer 的 O(n²) 与“零值 raw 可绑定”缺陷已改为索引化 identity pairing、finite non-zero earlier raw 约束以及 held/release inactive→active→inactive 约束。canary-03 strict replay约1.95秒完成，六项结构/恢复/companion checks PASS，仅 offline-local closure FAIL；analysis SHA-256为`9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`。
- create-new active catalog `raw-apply-catalog-v2.json` SHA-256 为 `F64FA3FFF87F1B0D1AEA8C4FAE8C849E229459222CAB7CB1B9B1D232E2489266`，capture plan v2 SHA-256 为 `6A7BC25ECDDCA0D556DE64BECAE8F2799B4AFC1E3D3618DF00E494CC6C3633CE`。v2 已完全移除旧 C0/C4 发布，只在 main-thread InputQuery physical-poll original return RVA `0x61F30C` 从 Keyboard referenced cache 或 Gamepad button/axis/deadzone/mapping 的稳定双快照重建唯一 action0..11；alias、CDCC/CDCD、非法/不稳定数据按action脱敏，primitive/指针/完整设备状态不落盘。
- v2 自动门为168/168 Rust、fmt、全目标clippy、Win32 Debug/Release-LTCG与两套Native harness全部PASS；该版本Release Controller/Native/Helper SHA-256分别为`AB256929D1362271DD8E171A0973E8FA87C897F1075EC48A0FA8FE83CB0CE5E0`、`201A600CE74FE47A8A464A60E488E73A4680569F80AE897E72E3ABA5AA02C949`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`；随后仅在用户单独确认后启动canary-04。
- 用户单独确认后的v2 keyboard-held-release canary-04 artifact `20260822T121256Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v2-canary-04` 已安全结束：raw627,338、loop225,849、10,555完整loop、零gap/overwrite/wrong-thread/Failure/error，50槽精确恢复，companion18/18PASS。action0..3各完成2次rising/falling和2条完整closure；action4..7虽有logical true、shoot consumer与witness change，却没有RAW_VALUE，strict analyzer仅offline-local closure FAIL。当前回到shoot binding/alias投影的静态归因，禁止自动重试或启动canary-05。
- canary-04同构建binding correlation固定根因：shoot4..7 primitive `0x107/0x106/0x109/0x108` 分别alias UI20..23，edge8..11也有outside alias；v2错误复用manager全0..31唯一分类器，属于`FULL_SCOPE_ALIAS_CLASSIFIER_MISAPPLIED_TO_ACTION_INDEXED_PROJECTION`。manager分类本身不放宽。
- create-new active v3 catalog SHA-256为`258549E9B107316FDE02C44580741862476AAB7CCABE57AA1958F06729304F51`，capture plan v3为`8908DABFF44E299185438CBC6092F7A3A6B5E308FB95C75BDFA795F0E0042C93`；v1/v2保持冻结。v3 projection只检查gameplay0..11 owner，outside alias不读取/不持久化且不阻断，cross-gameplay仍双向脱敏，同action重复允许；manager仍完整0..31严格分类。
- v3自动门为168/168 Rust、fmt、全目标clippy、Win32 Debug/Release-LTCG与两套Native harness全部PASS；Release Controller/Native/Helper SHA分别为`63EEB616E43181C15ED7CAF272962652DD45A45050A2846EA315C2105136FD6D`、`79FC27358B7A11F0FE86BAEC211D43CF4DC9D0D1141AB7A8DAA7352B0BD6A725`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`；随后仅在用户单独确认下启动首次v3 keyboard-held-release（全局canary-05）。
- 用户单独确认后的v3 keyboard-held-release canary-05 artifact `20260822T124555Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v3-canary-05` 已取得strict raw7/7与companion18/18PASS：raw635,300、loop209,973、9,893完整loop、零gap/overwrite/wrong-thread/Failure/error，50槽精确恢复。action0..7每项各2 rising/2falling、非零held、logical/consumer/witness同sample闭合及2条完整cycle；action12+/primitive/主动字段为零。四场景进度1/4；当前停在v3 keyboard-edge action8..11的新的单独人工确认前，禁止自动启动。
- 用户单独确认后的v3 keyboard-edge canary-06 artifact `20260822T133033Z-t0-04-input-raw-passive-keyboard-edge-t0-04-raw-apply-keyboard-edge-v3-canary-06` 已取得strict raw7/7PASS：raw1,744,176、loop494,136、23,098完整loop、零gap/overwrite/wrong-thread/Failure/error，50槽恢复。action8/9/10/11分别完成11/7/6/7个RF cycle，兼有分离与rapid gaps并闭合consumer/witness；action12+/primitive/主动字段为零。companion V5预期17/18仅scenario-evidence scope FAIL，结构/恢复/hash binding均PASS且不改既有V5 gate。四场景进度2/4；当前停在Gamepad unique-owner `controller-held-release`的新的单独人工确认前，禁止自动启动。
- 用户单独确认后的v3 controller-held-release canary-07 artifact `20260822T160527Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v3-canary-07` 永久保持FAIL：action0..7均有non-zero raw、RF cycle与singleton candidate，但另有229笔同family diagonal（move0+3=45、shoot5+6=88、shoot5+7=96），v3遇任意multi-action transaction即全局FAIL。该历史artifact不按新规则重放改判；raw manifest/events/analysis SHA为`98993BB28FFFDFD73987B5B5F08616814ADFF39CF00EA1B976645AA68646F4D0`、`DB770338CB6138C7FF401A6C5F8B4A2EC31FC01107498CC8B7B3A36DD439529F`、`9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`；loop manifest/events/analysis SHA为`F69EEEF540ACD522E8FA127544FA83F0B5070E0048C21ED9613629668664D163`、`8A63E3A7076086BF45CC3860C56C7A7895198AA24702CDD8E5DBAFF883D23716`、`6EA2CFE61598E6A4795D7FECB8E554AECF18B3BBCAA8E3D18757282B292DC7D1`。
- create-new active v4 catalog SHA-256为`E22323047082E6289891393DE0B467CA5038E15EBD45310F39B5051FD0EB13D6`，capture plan v4为`203F64825EA24EE4EFA21B731FD2CB8E15AB11DA87145F7654C545B676A45562`；v1/v2/v3保持冻结。v4不改Native hook/IPC/投影，只把同family且site正确的multi-candidate transaction跳过且零计分；cross-family/wrong-site全局FAIL；每个required action必须另有exactly-one candidate transaction，ambiguous witness不可复用。active v4 Controller/Native拒绝v3 identity。自动门为171/171 Rust、fmt、全目标clippy、Win32 Debug/Release-LTCG与两套Native harness全部PASS；Release Controller/Native/Helper SHA分别为`442064DE4D4CEAE30CF57BAE18D37A797B8061E544A775812167DABF9CBF753F`、`03751409B5BAD0D21717317030717E36B9EDEB0905E277C616CC07D5248FA954`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`。下一次controller实机仍需新的单独确认。
- 用户单独确认且controller0 unique-owner前检通过后的v4 controller-held-release canary-08 artifact `20260822T165606Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v4-canary-08` 已取得strict raw7/7与companion18/18PASS：raw470,556、loop258,053、11,162完整loop、零gap/overwrite/wrong-thread/Failure/error，50槽恢复。action0..7各有2–4个RF cycle与1–3个严格singleton proof；同family diagonal move0+3=37、shoot5+7=7按v4零计分忽略，cross-family/wrong-site=0，下方向action3/7仍闭合。raw manifest/events/analysis SHA为`03E6BD9FF9346CE791F52A5219D70210C07258FA342624854B99D393E4E763FD`、`29B576F8904B6613B305E151A337791E4D1416AAD65FA57884E4431B4A6B9B95`、`73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；loop manifest/events/analysis为`1AD3B1E6443573F79304638D6E88C1B5260E144B436568583C1BF691A1F2FE4D`、`1EF4DE57BBF2F19C03897AFD9A3942F729FE44608E6EC0BB62B22E557DF48AD2`、`275683A8A2317836F7E2900BDE8C3A5C466629126A7167588F08B74DBD0882CF`。四场景进度3/4；下一步仅为v4 controller-edge的新的单独人工确认，禁止自动启动。
- 用户单独确认后的v4 controller-edge canary-09 artifact `20260822T171644Z-t0-04-input-raw-passive-controller-edge-t0-04-raw-apply-controller-edge-v4-canary-09` 已取得strict raw7/7PASS：raw534,858、V5 275,545、11,806完整loop、零gap/overwrite/wrong-thread/Failure/error，50槽恢复。action8/9/10/11分别完成7/6/7/7个RF cycle，均同时含separated与9–19 loop rapid簇；singleton/proof为7/7、77/77、7/7、162/162，multi/cross-family=0。V5预期17/18仅`scenario-evidence` scope FAIL，其余结构/恢复/hash binding PASS且不改既有V5 gate。raw manifest/events/analysis SHA为`132C6E2E9AC4FE5E8C92B9F13D3E8C25A73B98492D54CA6E81651F568B64A9D7`、`CC61049B0CD90F39463F26EA2FF65263753419158E6F99B39F4D85AF5A397446`、`73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；V5 manifest/events/analysis为`490B3E7027C5D95BC78D39ED70F7383D62866292DE0E93AFD89FF99F92712BBB`、`FED45E85D5EAE0BFE0416F94A3FDC36F1DB4EC2C37F2D1182A01E0D34A130010`、`C0C29A74543100AC4E89976732F542375CBDB7DD5E792CD4496BE12A8AA4864D`。四场景现为4/4PASS，但全部永久`eligibleForActiveGate=false`；当前停在主动方案审查前，禁止沿用本轮确认启动任何active experiment。

### 产物

- `InputStateDraft` 字段表和 canonical 编码说明。
- action 分类与 edge 检测规则。
- Loopback queue 状态机、容量和 rejection 事件说明。
- 输入矩阵日志与逐 tick analyzer。

### 通过标准

- 每个受支持 gameplay 动作都有唯一、可重复的 canonical 表达。
- Local Client 输入只在目标 tick 被 Authority apply 一次。
- hold 不被误当作重复 edge，edge 不因 queue 重试重复触发。
- 输入 queue 满、target 过期/过远、sequence 冲突时拒绝且不改变世界。
- 同一输入脚本重复运行时，canonical bytes 和 apply 序列一致。
- 非 C/S 模式不拦截、不排队、不重放输入。

### 失败/停止条件

- 不能在不破坏 UI/菜单的情况下区分 gameplay input。
- input apply 依赖不可复制的临时设备指针或隐藏副作用。
- 拦截后原始路径仍在同一 tick 第二次应用输入。

### 后续依赖

- 解锁 T0-05。

## 8. T0-05：本地多人创建和虚拟输入设备

> 启动 T0-05 前先读取 [T0-04 逆向交接](t0-04/isaac-cs-t0-04-reverse-handoff.md)。T0-04 的 canonical/queue/Authority/owner-generation/恢复契约可复用；local co-op 第二玩家、HUD/index、controller assignment 与 virtual-device lifecycle 必须建立新的 T0-05 证据。
>
> 具体阶段任务、证据产物、门禁和退出条件见 [T0-05 专属研究计划](t0-05/00-overview/isaac-cs-t0-05-research-plan.md)；本节保留 T0 全局任务书中的高层目标与验收边界。

### 目的

让 Listen Server 的一个游戏进程能够用 local co-op 语义承载房主和至少一个远端玩家，并由 Authority 为两者分别应用输入。

### 前置依赖

- T0-04 `PASS`。
- 已有可控的 canonical input apply。

### 操作

1. 静态分析 local co-op 玩家加入/退出、controller 分配、player entity 创建、HUD 注册和 player index 更新路径。
2. 对比 offline 单人、local co-op 双人和 OriginalOnline 双人的 manager、player、input device 对象；禁止用 OriginalOnline `NetInputDevice` 代替 local co-op 结论。
3. 建立 future `ClientId -> LocalPlayerOrdinal -> Player*` 映射；`Player*` 仅为当前 generation 关联，不进入网络 identity。
4. 先利用游戏已有 local co-op 加入路径创建第二玩家，再研究是否需要 synthetic/virtual controller adapter。
5. 将 Endpoint B 的 scripted input 通过 Loopback/FaultInjectionTransport 送入第二玩家的 apply 路径。
6. 覆盖分别移动、同时移动、同时射击、边沿动作冲突、玩家死亡/复活观察、退出菜单。
7. 记录 player index、controller/device identity、entity InitSeed、pointer 生命周期和 Lua `Isaac.GetPlayer(index)` 的对应关系。

### 产物

- local co-op/player/input device 对象关系图。
- `ClientId -> LocalPlayerOrdinal` 冻结和退役规则草案。
- 双玩家输入隔离日志。
- virtual input device ABI/Hook 候选账本。

### 通过标准

- Authority 进程内存在两个由 local co-op 路径创建的有效玩家。
- Endpoint A/B 的输入只影响各自玩家，连续 1,000 tick 无串线、重复或 pointer 失效。
- 玩家 mapping 在当前房间和当前 run generation 内稳定。
- C/S 实验不创建或激活原版 Online Manager/ready gate。
- Lua/游戏侧玩家顺序与记录 mapping 一致。

### 失败/停止条件

- 第二玩家只能通过进入原版 online session 创建。
- synthetic input 要求伪造未理解的 owner、vtable 或 controller 生命周期。
- 玩家创建导致 save、achievement 或 run 状态不可逆污染；在理解隔离前停止 active 实验。

### 后续依赖

- 解锁 T0-06。

## 9. T0-06：实体枚举、临时身份和 canonical state hash

2026-10-19 `tx-01-79`：新定义场景及独立故障取证一次整体运行 **INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**；18 项历史沿用、3 项本轮实际检查。八类字段和实际失联恢复净补齐两项，正常及失联均确认 70 槽原字节恢复；消费暂停时 scheduler 继续推进、背压显式增加，W12 仍部分覆盖。全量 replay 排除后尚余 11 项。tx-01-77/78 注入前观察器 FAIL 保留，用户确认实质修正后才继续新整体验收。见[结果及剩余范围](t0-06/tx-01-79-e2e-results.md)。

2026-10-19 `tx-01-76`：按用户要求只排除全量 replay 续跑剩余项，单条 decoder/比较与限定双进程比较新增 9 PASS，10 PASS 明确沿用上轮；统一结果 **INCOMPLETE：19 PASS / 0 FAIL / 14 NOT_COVERED**。Projectile 可读，Familiar 专用字段缺初始化依据仍 NOT_READY，补充采集 70 槽正常恢复。用户要求保留缺口收尾，未自动重跑，完整 T0-06 仍未完成。见[结果及十四项未覆盖](t0-06/tx-01-76-e2e-results.md)。

2026-10-18 `tx-01-75` 运行收尾：连续完整普通采集 **13,567**、背压 **0**，新增消费管线及 51/70/70 槽恢复的自动子项通过。用户因离线全量 replay 耗时明确结束本轮，报告 **INTERRUPTED**，33 子项为 10 PASS / 0 FAIL / 23 NOT_COVERED；原始和部分输出保留，没有自动重跑，完整验收未完成。见[本轮结果](t0-06/tx-01-75-e2e-results.md)。下面 tx-01-75 的“运行验收未执行”为实现交付时状态。

2026-10-18 `tx-01-75`：按用户要求修改缓冲即时消费，专用通知读取线程在原始值复制后即归还共享槽，编码与文件输出在另一线程；256 MiB / 256 项自有队列及满时背压行为已获用户确认。本轮只实现与纯编译，运行验收未执行；原 tx-01-74 FAIL 不改判。见[实现与后续统一验收证据](t0-06/tx-01-75-immediate-consumer.md)。

2026-10-18 `tx-01-74` 本轮运行结果：统一 E2E 已执行一次，整体 **FAIL**，32 子项为 17 PASS / 1 FAIL / 14 NOT_COVERED。W07 最长连续完整数 2/10,000，primary 背压 18,868 次；默认 51、主/次各 70 槽正常恢复，W08/W09 通过。T0-06 尚未完整验收，未自动重跑，未在运行中改代码。见[实际结果及剩余内容](t0-06/tx-01-74-e2e-results.md)。下面的未执行状态保留对应实现交付时语境。

2026-10-18 `tx-01-74`：已实现[统一 E2E 执行器](t0-06/tx-01-74-unified-executor.md)，按用户确认的人工检查点组织 W01–W13、实际生产 CLI 与可复查报告；本次没有执行，运行验收仍未进行。新入口不调用旧构建/测试/哈希链，缺少真实场景或恢复证据时保留未覆盖。

2026-10-18 `tx-01-73` 当前状态：本阶段剩余实现已落入代码，**运行验收未执行**。通过可选 `coop-active --world-record min-read-v1` 接入 19 个身份入口和统一 70 槽事务，提供八类只读采集、临时身份/生命周期、固定 profile JSONL、离线重编码与逐字段比较。沿用现行规范不计算或校验哈希；本节旧 hash 操作/验收文字仅保留历史目标语境，当前以结构化字段比较实现。用户确认当前 A/B exact-pair 跨正常 PlayerUpdate 的读取规则，不补造启用前出生历史。见[实现说明](t0-06/tx-01-73-implementation.md)及[实现后 E2E 清单](t0-06/tx-01-73-e2e-checklist.md)。以下旧进展按其推进编号保留，不能解释为当前仍未实现。

2026-10-18 当前进展：`tx-01-63` 完成操作 1 的[静态生命周期定位](t0-06/tx-01-63-entity-lifecycle.md)。`tx-01-64` 完成操作 2 的[八类覆盖规格及偏移修正](t0-06/tx-01-64-category-coverage.md)。`tx-01-65` 至 `tx-01-68` 分别补充 [HP](t0-06/tx-01-65-hp-analysis.md)、[位置/速度和采样阶段](t0-06/tx-01-66-readonly-sampling.md)、state/timer、seed/归属；这些专题按用户要求由 GPT6 Luna / max 分析、主智能体复核。`tx-01-69` 汇总并核对[参考会话列出的偏移](t0-06/tx-01-69-offset-reconciliation.md)，补齐分类、Index、flags 和剩余布局依据；本会话约定的操作 3 最小原样只读静态范围至此补齐。各类通用 HP 表示生命仍是用户接受假设。原始资料与历史限制保留，见 [T0-06 入口](t0-06/README.md)。以上均为静态研究；实际 capture、运行时覆盖、专用字段全集、操作 4–9 尚未完成。旧前置 PASS/解锁条款不作为推进条件。

`tx-01-71`完成[参考吸收与剩余静态分析补全](t0-06/tx-01-71-static-completion.md)：按用户指定阅读旧REPENTOGON参考，使用GPT6 Luna / max子智能体进行IDA分析，主智能体复核更正并补齐实际生产链。43项静态问题已有身份事件/八类覆盖、接入与ABI/指令跨度、逐类读取、结构化记录/比较、原生framestate/summary/RNG history生产与覆盖差异方案。操作4–9必要静态与设计已推进，采集器、身份、编码和比较尚未实现；未运行游戏、构建/代码测试或哈希校验，未改运行时代码和协议，整个T0-06未完成。见[本轮一次整体静态审阅](t0-06/tx-01-71-review.md)。

`tx-01-67` 完成 [state/timer 的最小只读字段登记](t0-06/tx-01-67-state-timer.md)：GPT6 Luna / max 分析，主智能体复核当前绑定、访问指令及初始化锚点，并更正派生 Init 与 Grid 字段解释。范围只含逐类适用性、原始偏移/类型/宽度及必要初始化线索，不要求恢复完整状态机、计时单位或全部重置路径。仍沿用 tx-01-66 的采样阶段，未实现或运行采集器；见[本轮静态审阅](t0-06/tx-01-67-review.md)。

`tx-01-68` 完成 [seed/归属的最小只读字段登记](t0-06/tx-01-68-seed-owner.md)：GPT6 Luna / max 分析，主智能体复核，并补全直接 Grid.Desc 绑定、独立 RNG 和生成者分类。确认逐类偏移、宽度、seed 与当前 RNG 状态的区别、引用存储形式和必要初始化线索；不穷尽 Morph/Restore/复用的保留与清理行为，不执行目标解引用或建立跨 tick 身份，未实现采集器。见[本轮静态审阅](t0-06/tx-01-68-review.md)。

### 目的

建立按 tick 对齐的只读世界摘要，用于证明 Authority/Client 的状态比较和定位首次分歧；此任务不实现 snapshot apply。

### 前置依赖

- T0-05 `PASS`。
- T0-03 已确认安全 post-update capture phase。

### 操作

1. 定位当前房间实体容器、遍历入口、spawn、init、remove 标记和最终析构路径。
2. 覆盖 Player、NPC、Tear、Projectile、Pickup、Familiar、Effect 和 GridEntity；未知类别保留原始分类并报告，不能静默跳过。
3. 记录 Type、Variant、SubType、InitSeed、位置、速度、HP、state、flags、timer、owner/parent 等候选字段的稳定性和变更时机。
4. 建立 T0 临时身份：

   ```text
   T0EntityKey = (entity class, type, variant, subtype,
                  init seed, observed spawn ordinal)
   ```

   `observed spawn ordinal` 由同一 capture generation 的已确认 spawn 顺序产生。裸指针只作为日志关联，不进入 key/hash。
5. 若候选 key 冲突，记录 collision 并停止将该 schema 用于 cross-process equality；不得用 pointer 地址补齐。
6. 定义带 domain/version 的 `WorldHashDraft`：固定实体排序、字段顺序、小端编码、finite float 检查、`-0` 规范化、unknown/NaN 事件。
7. 将 hash 分层：run/room/player/entity class/whole world；analyzer 能在 whole hash 不同时下钻到首个 class/entity/field。
8. 对同一进程重复 capture 验证无 mutation；对相同 seed/input 的两个进程比较最小 canonical subset，并记录首次不一致。
9. 追踪原生 framestate/checksum 的生成函数，比较其字段覆盖，但不调用未知反序列化或写回路径。

### 产物

- entity container 和 spawn/remove 生命周期图。
- `T0EntityKey` 冲突报告。
- `WorldHashDraft` domain、schema 和字段登记。
- 分层 hash JSONL 与 first-divergence analyzer 设计。
- framestate/checksum 覆盖差异表。

### 通过标准

- post-update capture 连续 10,000 次不修改实体、容器或 RNG。
- 相同进程对同一 capture identity 重算 hash 完全一致。
- hash 不包含 pointer、容器地址、线程 id、墙钟或纯展示配置。
- entity create/remove 能在相邻 tick 中明确反映，未知类别被显式报告。
- analyzer 能把人为改变的一个已知字段定位到对应 tick/entity/field。
- cross-process hash 即使不相等，也能输出首次结构化差异而不是只有总 checksum。

### 失败/停止条件

- 遍历期间容器正在并发修改或生命周期不安全。
- InitSeed/spawn ordinal 仍无法消除稳定身份冲突。
- 读取候选字段触发虚函数、副作用、lazy initialization 或 RNG。
- 为追求 hash 相等而删除尚未理解的 gameplay 字段。

### 后续依赖

- 解锁 T0-07。

## 10. T0-07：Authority tick/input scheduler 实验

**状态：PASS（2026-10-20，用户确认）。** 判定依据是本阶段操作1–8的现有生产状态链/故障矩阵验收记录，以及 tx-01-98 在批准目标上完成的真实 Authority B源停发 E2E（24/24 PASS、controller exit0、70槽恢复），另见[阶段状态/证据表](t0-07/STATUS.md)。PASS 的事实范围仍受各操作记录限制：操作6–8的部分结果来自统一离线生产链矩阵；操作9的 B 为脚本源，不冒充真实网络断线/ACK，world record 只有 partial Player 字段而非完整世界快照。

操作6–9按[具体工作计划（tx-01-94）](t0-07/tx-01-94-remaining-operations-plan.md)执行，包含已有基础、剩余实现、源码分析问题、完成依据与产物。操作6–8的离线矩阵与操作9的真实实机验收均已完成；历史离线PASS仍不替代其声明的实机观察范围。后续需要源码分析时按用户要求使用GPT-6 Luna / max，并由主智能体逐项审阅。

当前操作9：tx-01-98已完成真实 Authority 的 B 源停发实验，最终统一实机验收 **24/24 PASS**、controller exit0。批准 unpacked 目标上的一次 New Run 中，生产脚本源停发1/5/30/120 Authority tick，effective缺口按lead=2分别后移2 tick；B中性/双Resolve/标准odd boundary/edge 5/5 exactly-once/120-tick timeout recovery/A实际位置变化/70槽恢复均通过。前三次失败（错误GameExe、A动作晚到、A穿门死亡）原样保留；world record全部为partial `NOT_READY`，仅Player字段明确作为partial证据，无complete world snapshot；脚本停发不冒充网络断线。用户据此于2026-10-20确认 T0-07 整体 **PASS**。见[结果](t0-07/tx-01-98-results.md)、[设计](t0-07/tx-01-98-source-stop-e2e.md)和[静态审阅](t0-07/tx-01-98-static-review.md)。

当前操作8：tx-01-97已完成生产故障与边界矩阵，最终统一离线验收 **27场景、183命令、184/184检查PASS**。A/B conflict、source sequence极值、queue/history、timeout、return/publication和collector状态均由当前生产链执行；真实Native运行、Hook、采样内存、事件环和网络/世界动作未验证，见[结果](t0-07/tx-01-97-results.md)。

当前操作7：tx-01-96已完成CLI/manifest严格解析、IPC v5、初始化冻结，以及lead/future window/script drain/admission budget在Participant、Clock、Pipeline和Native target/script路径的实际生效；最终统一离线验收 **20配置case、9场景、83命令、104/104检查PASS**。真实Native运行、Hook、采样内存、事件环和网络/世界动作未验证；操作8–9仍待实施。见[结果](t0-07/tx-01-96-results.md)。

当前操作6：tx-01-95已完成missing/effective输入分离、max-age=0中性、独立有效到达、静默/超时/恢复诊断、失绑/rebind/终态和Native有界日志；最终统一离线验收 **10场景、146命令、122条批量记录、152/152检查PASS**。真实Native运行、Hook、采样内存、事件环和网络/世界动作未验证；操作7–9仍待实施。见[结果](t0-07/tx-01-95-results.md)。

当前操作5：tx-01-93已实现生产有界drain/统一准入、双Resolve、原生返回witness、Outer采样、tick提交与本地回执顺序；按现行规范使用post-state值副本或明确缺口，不计算hash。GPT-6 Luna / max结论经主智能体审阅，Native最终纯编译成功；唯一离线验收 **36场景、254/254检查PASS**，保存120条连续记录。真实游戏/Hook/采样内存/网络未运行，操作6–9未实施。见[实现](t0-07/tx-01-93-tick-pipeline.md)、[审阅](t0-07/tx-01-93-static-review.md)和[结果](t0-07/tx-01-93-results.md)。下文操作1–4为历史范围。

当前操作4：tx-01-92已完成A/B统一AcceptInput，严格字节解码、原validator/去重/队列共用，Native最终纯编译成功；GPT-6 Luna / max结论经主智能体审阅，唯一统一离线验收 **40场景、303/303检查PASS**。B仍为脚本源，真实游戏/Hook/网络未运行，操作5–9未实施。见[实现](t0-07/tx-01-92-input-admission.md)、[审阅](t0-07/tx-01-92-static-review.md)和[结果](t0-07/tx-01-92-results.md)。下文操作1–3记录保留历史时点。

当前操作3：tx-01-91已实现逐client输入状态，纯编译成功，一次统一离线验收 **22场景、152/152检查PASS**；Luna/max结论由主智能体逐项复核，真实Native/游戏未运行，操作4–9未实施。见[实现及字段契约](t0-07/tx-01-91-client-input-state.md)、[审阅](t0-07/tx-01-91-static-review.md)、[实际结果](t0-07/tx-01-91-results.md)。以下操作1/2记录保留各次范围。

当前操作2：tx-01-90已实现并通过一次统一离线验收，30场景、175/175检查；只在配对的已确认模拟边界正常返回后递增，完整世界样本资格仍独立。子智能体结论已由主智能体逐项复核，真实Native/游戏未运行；操作3–9不在该次范围。见[实现](t0-07/tx-01-90-authority-tick.md)、[审阅](t0-07/tx-01-90-static-review.md)、[结果](t0-07/tx-01-90-results.md)。

当前操作 1：tx-01-89 已实现并通过一次统一离线验收，33场景、303/303检查；真实 Hook/Join/恢复未运行。操作 2–9 不在该次范围。见[阶段入口](t0-07/README.md)、[状态与身份契约](t0-07/tx-01-89-authority-session.md)及[实际结果](t0-07/tx-01-89-results.md)。本节旧 hash/测试/gate文字服从现行规范。

### 目的

验证 C/S 的核心推进差异：Authority 按自身 server tick 前进，统一处理 Local/Remote Client 输入，但不等待每个 client 同 tick ready。

### 已有研究基础

- 使用T0-04、T0-05、T0-06已有的边界、输入/绑定和只读采集依据，按实际依赖与授权推进；历史PASS或旧解锁字段不作为推进前置条件。

### 操作

1. 实现实验域 `AuthoritySession` 状态机：

   ```text
   Dormant -> Negotiating -> Warmup -> Active -> Retiring -> Retired
                                      \-> Failed
   ```

2. `server_tick` 只在 T0-03 已确认的成功 gameplay tick 后增加。
3. 为每名 client 维护 input sequence、future window、last state、edge ledger、last received/processed tick 和 rejection counters。
4. Local Client 和 Remote Client 都通过同一 `AcceptInput` 入口；transport 差异不能绕开 validator。
5. Active tick顺序固定为：bounded drain、统一验证、选择effective input、双玩家apply、一次gameplay update、post-state复制或明确缺口、tick advance、值与本地处理回执publication；按现行规范不计算或校验hash。
6. 落实[缺失输入、超时与恢复](t0-07/tx-01-94-remaining-operations-plan.md#2-操作6缺失输入超时与恢复)：现行 `max_input_age_ticks=0`，continuous/held缺输入立即neutral；edge缺失不执行且不补执行，T0不rollback。区分目标tick缺输入、源静默与client timeout，核对新鲜度/代际/暂停和恢复；重复或非法到达不能保活，B缺失不阻止A/tick。
7. 完成[参数配置贯通](t0-07/tx-01-94-remaining-operations-plan.md#3-操作7climanifest到native的参数注入)：CLI/test manifest → C/S初始化/IPC → Native实际生效，覆盖 `input_lead_ticks`、`future_window_ticks`、`max_input_age_ticks`及明确单位的drain预算。检查范围/组合/溢出、启动例外与版本兼容，session内冻结并记录实际配置；保持已采纳默认，未支持值明确拒绝，不作为产品常量。
8. 完成[生产状态链故障覆盖](t0-07/tx-01-94-remaining-operations-plan.md#4-操作8完整故障与边界覆盖)：对照已有场景，补齐duplicate、conflict、late、far-future、queue-full、client timeout、source sequence和server tick耗尽及身份/代际/阶段/发布边界。先写失败方式与场景再修改，以一个统一离线/E2E入口一次验收并保存复现产物，不在实现后补写单元测试。
9. 执行[真实Authority停发实验](t0-07/tx-01-94-remaining-operations-plan.md#5-操作9真实authority的b输入停发实验)：B停止新输入1、5、30、120个已提交Authority tick，分别记录源停发与effective缺口、队列余量和恢复lead，实际确认A动作与tick持续推进、B中性/恢复、edge不重放及Hook/批次恢复。当前可控制B脚本源，不冒称真实网络断线；按实际授权实施，一次统一E2E覆盖必要段落。

### 产物

- Authority/Input 状态机和不变量说明。
- 贯通生产状态链的离线场景、实际配置与故障矩阵。
- 真实tick/input/阶段/post-state值或缺口/本地回执JSONL；未运行的实机项明确NOT_RUN。
- missing-input 行为和边沿动作 exactly-once 报告。

### 通过标准

- Local/Remote 输入经过相同 validator 和 queue，日志 identity 完整。
- duplicate幂等；冲突、非法或超窗命令不进入有效队列、不触发世界动作。非法拒绝保留诊断统计；冲突继续允许既定的A关闭session/B退休处理，不要求这类失败处理“状态完全不变”。
- B停止新输入120个Authority tick时，Authority不产生C/S ready stall，A继续推进；分别记录源停发与effective缺口，不把队列余量或恢复lead混为即时中性失败。
- 任何 edge identity 最多执行一次，缺失 edge 不在后续 tick 自动补发。
- 每个成功Authority gameplay tick有一次tick increment，post-state值或明确缺口可关联；值与本地回执在提交后发布，不计算/校验hash。
- Failed/Retired session 不接受新 input，旧 packet 不重新激活。

### 失败/停止条件

- Authority 仍依赖“每名玩家当前 tick 都有 record”才能进入 gameplay update。
- Local Client 输入存在未经过 scheduler 的旁路。
- missing-input policy 导致上一 edge 每 tick重复执行。
- server tick 与实际成功 gameplay tick 脱钩。

### 后续工作

- T0-08负责独立协议与后续网络接入；T0-07的完成情况、失败及未知按实际证据报告，不新增解锁或gate要求。

## 11. T0-08：最小 `HELLO / INPUT / ACK / STATE_HASH` 协议

> 最新tx-02-04（2026-10-20）：验收器持有A物理输入后完成唯一整体E2E，**33/33 PASS**、controller exit0、未自动重跑。初始4步与连续64步全程游戏前台；A有646/6269个非零PhysicalSample和真实位置变化，覆盖tick160..179 outage。trace56,070/56,070、lost0，6,268 commit逐条ACK/STATE/身份配对，B中性/恢复、3/3 Drop边沿、70槽恢复/正常stop通过；离线19,022/19,022 PASS。T0-08操作1–9当前范围判定PASS，见[实际结果](t0-08/tx-02-04-results.md)和[输入会话实现](t0-08/tx-02-04-owned-physical-input.md)。实机仍为单游戏Authority+同进程逻辑Remote，不宣称socket/Steam、双游戏、Replica或跨进程世界相等。

> 最新tx-02-01（2026-10-20）：协议日志消费/失败收尾已修复，唯一整体E2E为23/30 FAIL。离线19022/19022通过；trace4007条、lost0、恢复70槽逐项通过。未解决：A原生输入全0；停止最后commit271出现Publication7，ACK/STATE各缺该tick。当时阶段为IN_PROGRESS；未自动重跑，见[历史结果](t0-08/tx-02-01-results.md)及[修复设计](t0-08/tx-02-01-log-recovery.md)。旧tx-02-00记录保留。

> tx-02-00最新实机执行（2026-10-20）：用户明确要求继续并确认准备/聚焦后，真实Authority启动、B创建与物理输入成功，controller因`protocol trace overflow`退出1；保存事件至commit20、协议312条。离线19,022/19,022 PASS；正常停止helper成功，逐槽恢复产物缺失，outage和完整实机验收未完成，当时T0-08为IN_PROGRESS。前三次执行的授权与失败分别保留，见[实际结果](t0-08/tx-02-00-results.md)，下文旧状态代表对应历史时点。未自动重跑。

> 2026-10-20，tx-01-99 用户确认：采用无哈希路线，保留包 CRC；操作1–4仅实现独立协议核心和内存双端场景。`STATE_HASH` 保留历史消息名，载荷改为原样字段观察/明确缺口，不计算或比较构建、文件或状态哈希。详见[协议定义](t0-08/tx-01-99-wire.md)和[当前状态](t0-08/README.md)。

> 历史tx-01-99结果：操作1–4核心与双端握手已实现，唯一内存双端验收305/305 PASS；当时未实现真实网络/游戏接入及操作5–9。见[结果与复现](t0-08/tx-01-99-results.md)。
>
> 当前tx-02-00：操作5–9及真实Authority接入已实现，wire v2/IPC v6、内存逻辑Remote，无socket/Steam或Replica。A/B首个新鲜输入同completed+1，B迟到正常过期，后续显式lead8。唯一整体验收已执行：离线19,022/19,022 PASS，因未找到运行中的isaac-ng.exe而整体FAIL，实机NOT_RUN；未自动重跑，当时T0-08整体为IN_PROGRESS。见[历史结果](t0-08/tx-02-00-results.md)、[E2E清单](t0-08/tx-02-00-e2e-checklist.md)及[实现入口](t0-08/tx-02-00-implementation.md)。

### 目的

把 T0 scheduler 身份与状态观察放入独立、严格、可 fault-inject 的双端协议，同时避免承诺 T1 snapshot 格式。

### 前置依赖

- T0-07 `PASS`。

### 操作

1. 分配仅限 T0 的 magic、protocol version、最大 packet 和状态观察 domain；不得使用 `ISCS`、protocol 5 或 `0xFD` 作为默认 transport channel。
2. 定义公共 envelope：magic、version、kind、header size、session、sender role/id、message sequence、payload size、flags、CRC。
3. 定义四类消息：
   - `HELLO`：显式 build 标识、实验 protocol、role、双方 nonce、input/state schema、capabilities；
   - `INPUT`：client id、input sequence、target tick、canonical `InputStateDraft`、最后收到的 ACK；
   - `ACK`：server tick、最后接受/处理 input、rejection summary、session state；
   - `STATE_HASH`：server tick、capture phase、schema/domain、局部原样字段或明确缺口，不包含哈希。
4. `HELLO`/首个 `ACK` 建立 session；角色、build、schema、nonce 或 capability 不一致时 Active 前失败。
5. `INPUT` 使用 unreliable sequenced 语义，并可携带最近固定上限的输入冗余；每条记录仍按 input sequence 幂等。
6. `ACK`/`STATE_HASH` 可被新包覆盖，但必须携带完整 identity，不能依赖跨 lane 顺序。
7. Decoder 严格拒绝未知 kind/flag、非零 reserved、尾随字节、长度不符、非法 input、错误 session/sender/sequence 和 CRC。
8. 建立 golden vectors、截断/扩展/位翻转/旧版本/sequence wrap/fuzz corpus。
9. 首先在内存 `FaultInjectionTransport` 运行；Legacy Steam channel 的选择留给独立 transport 验证，不阻塞 T0 protocol 语义。

### 产物

- T0 wire Draft 和状态机。
- 编解码 golden vectors 与负向 corpus。
- packet/input/状态观察双端配对 analyzer。
- 协议常量登记，明确“不得发布/不得兼容”的实验身份。

### 通过标准

- 每个合法 encode/decode round-trip 字节稳定。
- 所有非法向量在改变 session/scheduler/world 前被拒绝。
- duplicate/reorder/loss 下 input exactly-once identity 和 ACK 单调性可解释。
- build/input/state schema 不一致时双方均不会建立协议会话；协议Established不替代游戏侧Active所需的绑定。
- T0 packet 不会被现有 `ISCS` decoder 接受，现有 packet也不会被 T0 decoder 接受。

### 失败/停止条件

- 需要借用现有 protocol 5 flags/session 才能区分 T0 状态。
- decoder 在校验完成前修改 scheduler。
- packet identity 无法支持双方日志逐包对应。

### 后续工作

- 后续完成操作5–9及T0-09的网络与故障矩阵；按实际实现和证据推进，不新增解锁或gate要求。

## 12. T0-09：双进程与网络故障矩阵

> 2026-10-21 tx-03-08：[T0-09-C 工作设计](t0-09/tx-03-08-c-design.md)及[实施前失败方式](t0-09/tx-03-08-c-failure-modes.md)已完成，本轮唯一静态文档审阅 PASS；C 生产实现 NOT_STARTED、P2 NOT_RUN。用户确认 C 一次统一入口覆盖 F0 10,000 tick 和 F1–F5 各单 seed/5,000 tick；逐场用户重启双端、准备同基线，runner 保持一次调用；每 Authority tick 留记录，普通完整采样比较、odd-only gap 单列且普通样本无遗漏。完整多 seed 与 C0/C1/C3 留给 D。

> 2026-10-21 tx-03-07 更新：Host+VM 双真实游戏 P1/F0 唯一一次实机验收完成，Orchestrator PASS、Analyzer **38/38 PASS**；10,000 Authority tick、双端停止/退休、游戏存活和 Host 51/51 catalog 恢复均实际观察。原始统一入口因运行中游戏映射 staging DLL、清理删除返回 `WinError 5` 而机械记 FAIL；用户明确裁定“算正式通过，构建产物我自行删除”。因此 **T0-09-B P1/F0 = FORMAL PASS（用户裁定）**，原始 FAIL 保留、不重跑。F1–F5、C0/C1/C3、observer 状态比较、多 seed 和完整 T0-09 仍 IN_PROGRESS。见[正式验收记录](t0-09/tx-03-07-live-f0-formal-acceptance.md)。

> 2026-10-20 设计状态：**DESIGN_COMPLETE / REVIEW_PASSED**。T0-09-B 离线传输/控制基础已完成一次统一离线 F0 验收，**41/41 PASS**；真实 host+VM、双游戏 Native v7、注入/observer 和 P1 均 **NOT_RUN**，B 整体仍 IN_PROGRESS。用户已确认 host Windows + 本机 Windows VM 双机、两个真实同 build `isaac-ng.exe`、host 侧独立 UDP impairment relay、VM 端完整 observer-simulator、双端确定性脚本输入、VM 共享 artifact、C0/C1 双机新鲜对照，以及按现行规范删除 C2。设计见 [T0-09 README](t0-09/README.md)、[主设计](t0-09/tx-03-01-design.md)、[B 结果](t0-09/tx-03-02-b-results.md)、[失败方式](t0-09/tx-03-01-failure-modes.md)和[静态审阅](t0-09/tx-03-01-static-review.md)。T0-08 的 33/33 PASS 只作为协议前置，不替代 T0-09。

### 目的

在一个 Authority/Local Client 进程和一个 Remote Client 进程之间，验证模式、scheduler、最小协议、最小状态观察和失败可观测性。

### 前置依赖

- T0-08 `PASS`；当前已由 tx-02-04 的 33/33 单进程/逻辑 Remote E2E 满足。T0-09 仍须独立完成跨进程证据。
- 两端使用相同游戏、Native、输入/observation schema、run seed、角色和配置；本轮按用户确认使用双端确定性脚本输入和最小状态观察，不使用哈希。

### 操作

1. 按下述固定拓扑分别启动 Authority/Local Client 和 Remote Client。
2. 对每次运行执行同一输入脚本，并保存输入脚本版本和 canonical bytes。
3. 依次执行 F0–F5 网络矩阵；所有 impairment 使用可重放的固定 PRNG seed。
4. 执行 C0/C1/C3 对照，证明未安装、Native disarmed 和 CustomAuthority 的边界；C2 按现行规范删除，不触碰 OriginalOnline/Isaac Sync。
5. 对每次运行检查 server liveness、输入 exactly-once、packet 配对、状态首次分歧和 target restore。

#### 固定拓扑

```text
Endpoint A
  role = Authority Server + Local Client
  players = A(local), B(remote virtual device)

Endpoint B
  role = Remote Client / T0 observer-simulator
  wire input source = B
  local observer replay = A/B deterministic scripts (not authoritative apply)
```

T0 不要求 Endpoint B 应用权威 snapshot。Endpoint B 是真实游戏中的 observer-simulator：本地重放 A/B 确定性脚本，只有 B 输入经 wire 发送；接收 ACK/STATE 仅用于协议观察。双端只比较同 tick 的最小 canonical 字段窗口，不计算状态哈希。

#### 输入脚本

每次 run 使用固定脚本和 seed：

1. 300 tick neutral warmup。
2. A 单独四向移动和停止。
3. B 单独四向移动和停止。
4. A/B 同时相反方向移动。
5. 连续瞄准/射击 hold 与 release。
6. 可安全复现的 edge 动作，每个 identity 只出现一次。
7. B 输入静默 1、5、30、120 tick。
8. B 恢复后发送合法新 sequence，不补发过期 edge。
9. 300 tick cooldown，正常 retire/restore；自有设备与 DLL 遵守常驻策略，不强制 unload。

#### 网络矩阵

| Case | One-way delay | Jitter | Loss | Duplicate | Reorder | 最短观察 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| F0 baseline | 0 ms | 0 ms | 0% | 0% | 0% | 10,000 authority ticks |
| F1 local jitter | 25 ms | 10 ms | 0% | 0% | 0% | 5,000 ticks |
| F2 normal WAN | 50 ms | 20 ms | 1% | 1% | 1% | 5,000 ticks |
| F3 poor WAN | 100 ms | 40 ms | 5% | 2% | 2% | 5,000 ticks |
| F4 reorder/duplicate | 50 ms | 20 ms | 0% | 10% | 10% | 5,000 ticks |
| F5 burst outage | 50 ms | 20 ms | 0% + 2 s burst | 0% | 0% | outage 前后各 1,000 ticks |

延迟和抖动使用固定 PRNG seed，manifest 记录 seed；按相同输入包序列可复算故障决策，不承诺不同宿主/VM 调度下实际到达交错完全一致。完整 D 矩阵每个非 baseline case 至少运行三次不同 impairment seed；C 专项按 tx-03-08 的单 seed 规模执行。

#### 对照矩阵

- C0：未安装 T0 Native 的 offline/local run。
- C1：安装 T0 Native，但 `Offline`/disarmed。
- C2：**按现行规范删除**。原 T0-09 计划中的 OriginalOnline 双端只读对照不再执行；C/S 不进入或触及 Original Online / Isaac Sync，也不把“未触碰”登记为通过。C0/C1 由双机新鲜 offline/disarmed 对照承担，C3 为 custom Authority/Remote 主矩阵。
- C3：CustomAuthorityServer + CustomAuthorityClient，host 侧独立 UDP impairment relay；A 经 loopback、B 经 VM 虚拟网络；不使用 Steam/原版 transport。

#### 需要分析的不变量

- Authority server tick 只跟随成功 gameplay tick，不因 B input 缺失停止。
- A/B input sequence、target tick、accept、apply、ACK 能完整关联。
- edge apply count 为 `0` 或 `1`，永不大于 `1`。
- 同 identity 冲突包不改变后续有效序列。
- JSONL sequence 无缺口；packet 配对、loss、duplicate、reorder 与注入参数一致。
- F0 的最小 canonical subset 应在可比较 baseline 下保持一致；若不一致，analyzer 给出首次字段分歧或明确 observation gap。
- tx-03-08 用户确认的 C 采样规则：每个 Authority tick 均有记录；同逻辑 tick 的普通完整样本比较，odd-only 仅在原生 kind/reason 与连续来源明确时记 gap，不获得完整样本或相等资格；普通样本不能遗漏，两端 phase 不匹配不能移动 tick 凑配对。
- F1–F5 不要求 observer 本地状态始终与 Authority 一致，但每次分歧必须能关联输入到达/apply 或已知未复制状态，并分类为有限窗口。
- C0/C1 的 C/S packet、session 和 synthetic input 为零；C2 不再执行，也不以“未触碰 Original Online”作为通过证据。
- 正常停止后实际安装的 target bytes/IAT/vtable 全部恢复；自有设备与 DLL 按现行规范常驻。

### 产物

- 全部 case 的去身份化 manifest、双端 JSONL 和 analyzer JSON。
- 输入 exactly-once、server liveness、first-divergence 和 restore 汇总表。
- 每个失败 case 的最小复现 artifact。
- T0 参数观测结果；不直接固化为产品常量。

### 通过标准

- F0 连续 10,000 Authority tick 无 C/S ready stall、event gap、重复 edge 或 tick/input/observation identity 缺失。
- F1–F5 中 Remote input 缺失不会冻结 A；缺失策略与 T0-07 一致。
- 所有注入 duplicate/reorder/loss 都由 analyzer 正确计数，无未知 packet 消失或凭空产生。
- 首次状态分歧可以定位到一个有限 tick/input/entity-field 窗口；不允许只报告“checksum mismatch”。
- C0/C1 对照无可归因于 T0 的 gameplay 差异；C2 已删除，不执行 Original Online 零干扰验证。
- 每次运行都完成显式 retire 或记录可解释的失败退役，并成功恢复目标。

### 失败/停止条件

- Authority 在 Remote outage 期间停止 gameplay tick。
- Local Client 存在 scheduler 旁路，或 B edge 因重传执行多次。
- analyzer 无法区分 transport loss、decoder reject、scheduler reject 和 state divergence。
- 原版对照模式出现 C/S packet、输入注入或状态 mutation。

### 后续依赖

- 解锁 T0-10。

## 13. T0-10：验收报告和 T1 准入

### 目的

把 T0 的证据、失败和开放风险固化为可审计结论，决定是否进入“单房间权威 baseline/replica”阶段。

### 前置依赖

- T0-01 至 T0-09 均有正式结论。
- T0-09 必须为 `PASS`；较早任务若有受控限制，必须明确为何不破坏 T0 不变量。

### 操作

1. 生成 T0 report，固定以下章节：
   - build/fingerprint；
   - mode isolation；
   - tick/input/entity/render 边界；
   - local co-op/virtual input mapping；
   - input schema/scheduler；
   - entity/hash schema；
   - T0 protocol；
   - fault matrix；
   - first-divergence findings；
   - target restore；
   - rejected hypotheses；
   - security/privacy；
   - T1 recommendation。
2. 每个结论附 artifact identity、hash、endpoint、case、tick window 和证据标签。
3. 将 T0 设计常量分成：已由实验确定、仅为测试参数、仍未知三类。
4. 审核所有“权威、snapshot、同步、稳定 identity”等措辞，防止把 hash-only 原型写成状态复制实现。
5. 对 T1 建立新的风险/任务入口，但不得在 T0 报告中提前实现 wire 或 apply。

### 产物

- `Isaac C/S T0 验收报告`。
- artifact 索引与 SHA-256 清单。
- T1 go/no-go 决议和阻塞项。
- 更新后的 evidence ledger。

### T1 Go 条件

必须同时满足：

- `NetworkMode` 可以可靠冻结和退役，OriginalOnline 零干扰门通过。
- gameplay tick/input apply/post-state capture 具备主线程、次数和顺序证据。
- Authority 可模拟两个 local co-op 玩家，输入 mapping 在 T0 范围稳定。
- scheduler 在故障矩阵中不等待全员且 edge exactly-once。
- entity container、spawn/remove 和无指针 state hash 可以安全观测。
- first-divergence analyzer 能定位实体/字段级差异。
- 所有 Hook 都能事务安装/恢复，失败路径 fail closed。
- 已找到至少一个可研究的 entity create/update/remove apply 入口，用于 T1 baseline；不要求此时已写回成功。

### T1 No-go 条件

任一条件成立即禁止进入 snapshot apply：

- C/S 模式仍依赖原版 Online Manager 或 channel `0` 才能推进。
- 无法阻止 Local Client 绕过 Authority scheduler。
- 无安全、稳定的 gameplay tick 或 post-state capture phase。
- 第二玩家只能由未理解的原版网络对象创建。
- entity identity 必须包含 pointer 才能避免冲突。
- state capture 会修改 RNG、实体或 Lua。
- OriginalOnline 对照出现行为变化。
- target restore、日志完整性或 analyzer 因果链不可靠。

### 通过标准

- 报告可以让未参与实验的工程人员根据 artifact 重放任一结论。
- 所有未知项仍明确标为 `UNKNOWN`，所有反例保留。
- Go/No-go 只由上述门禁决定，不因排期或已投入成本放宽。

### 失败/停止条件

- artifact 缺失、hash 不一致、身份无法对应或报告引用未保存的临时数据。
- 需要修改 analyzer 规则才能把既有失败日志解释为成功。

### 后续依赖

- 无；T0-10 的 Go/No-go 决议是 T1 任务规划的唯一入口。

## 14. 最终验收矩阵摘要

| 能力 | T0 需要证明 | T0 不需要证明 |
| --- | --- | --- |
| 模式 | C/S 与 OriginalOnline 独立、可退役 | 同 run 热切换 |
| Tick | Authority 单调推进且不等待全员 | 产品级 tick 参数 |
| 输入 | Local/Remote 同管线、edge exactly-once | 完整手柄生态和重绑定 UI |
| 玩家 | 两个 local co-op 玩家可分别控制 | 多 local player/peer、动态 roster |
| 状态 | 安全枚举、canonical hash、首次分歧 | snapshot serialize/apply |
| 网络 | 最小消息、严格 decode、fault matrix | 生产 Steam transport |
| 客户端 | 发送输入、接收 ACK/hash | Replica World、预测、reconcile |
| Lua/Mod | 生命周期不被破坏 | gameplay Mod 兼容 |
| 生命周期 | session/Hook 安全退役 | host migration、late join |

T0 的成功定义是“证明下一阶段值得且能够安全开始”，不是把尚未具备状态复制能力的双端原型包装成新联机模式。
