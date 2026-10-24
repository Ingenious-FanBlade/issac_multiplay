> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Isaac C/S T0-05 本地多人创建与虚拟输入设备研究计划

> 2026-10-12 现行约定：[项目规范、约定与要求](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得进行其他测试；逆向与测试可由同一智能体完成。下面的旧状态、机器字段与历史判定保留为资料，旧门禁/解锁、哈希绑定、逐项预验证、预封和独立角色要求已取消。

## 文档状态

- 状态：**CS-T0-05-05 已完成只读必要性判断：物理设备自由 Endpoint B 需要 virtual device，但实现资格的 10 项 ABI/owner/lifecycle 证据未闭合，formalDecision 为空且状态为 BLOCKED_BY_EVIDENCE；05-04 严格证据轨、manager-this v33 remaining 99、人工逆向队列 remaining 96、Init/持久化、reader 与 ABA 依据均不改写**。
- 更新时间：2026-09-06。
- 适用目标：Win32/x86 `isaac-ng.exe`。
- 上游门禁：T0-04 schema-4 final decision 正式 `PASS`。
- 当前证据：**CS-T0-05-03 新门为五份适用 PASS source＋一项限域 N/A，正式 decision `PASS`；旧六源机器 gate 的 `5/6`、`BLOCKED_BY_EVIDENCE` 作为历史结果保持不变**。
- 本轮授权：**仅把“原生 Join 可可靠创建并发布 B”记录为用户规划假设；不改写严格证据、机器 gate、权限或历史结果，不进行实机操作、运行时 mutation 或权限提升**。

05-05 当前入口为 [Virtual Device 决策](../07-virtual-device/README.md)；05-04 的 [原生 Join 创建/发布用户规划假设 v1](../../02-active-join/native-join-active-user-planning-assumption-v1.md)仍仅为 `DESIGN_ONLY`。严格 manager-this 入口继续是 [MT-R02 animation descriptor checkpoint v41](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md) 与 [v33 strict frontier](../06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v33-handoff.md)。后续人工逆向调度仍使用 remaining 96，strict whole-image/formal 链仍只使用 v33 的 remaining 99；plan v10 / join progress v4、active/live、05-06 和 T0-06 均保持冻结。

本文把 [T0 总任务书](../../../isaac-cs-t0-task-plan.md) 中 T0-05 的高层要求细化为可执行工作包。T0-04 的可复用事实、禁止外推项和安全恢复契约以 [T0-04 逆向交接](../../../t0-04/isaac-cs-t0-04-reverse-handoff.md) 为单一入口；时间线和历史停点见 [T0-03R / T0-04 状态索引](../../../isaac-cs-t0-03r-04-status.md)；长期边界见 [Isaac C/S 总体架构](../../../../isaac-cs-architecture-plan.md)。

发生执行约定冲突时，依次遵循用户当前指示、现行项目规范和具体任务的功能说明。旧 gate decision、哈希和解锁字段只作为历史记录，不作为后续推进前置条件，也不运行其校验流程。

实际观察、设计与未知项分别记录；保留既有 artifact、原始 analysis 与历史 `PASS/FAIL` 的事实含义。新结果说明推进编号、位置、结果和未决问题，不要求 create-new 认证链、supersedes/hash 关系或独立审计。

本次适用性迁移也不覆盖上述证据优先级：live-07 的 `BLOCKED_BY_EVIDENCE`、旧六源 gate 和 progress v7 保持原样。新规格由独立 identity、严格 contract 与新的完整 verification 实施；第 17～23 节保留各次实施时的历史状态，不作为当前进度。

## 1. 目标、成功边界与非目标

### 1.1 目标

T0-05 要证明：一个保持 Offline 游戏模式证据的 Listen Server Authority 进程，可以通过游戏自身 local co-op 语义承载固定的两名玩家，并把 Endpoint A/B 的 canonical 输入分别、恰好一次地应用到对应玩家。

固定角色：

| Endpoint | T0-05 角色 | `LocalPlayerOrdinal` | 输入来源 |
| --- | --- | ---: | --- |
| A | 房主 / Local Client | `0` | Keyboard 或当前选定的本地物理设备，经 loopback 进入统一 scheduler |
| B | 远端玩家实验替身 | `1` | scripted / `FaultInjectionTransport`，经同一 scheduler 进入第二玩家 |

T0-05 成功只表示 local co-op 玩家创建、映射、生命周期与输入隔离在固定目标和实验范围内成立，不表示已经实现可玩的 C/S 联机。

### 1.2 完成边界

T0-05 `PASS` 必须同时证明：

- Authority 内存在两个由 local co-op 路径创建的有效 `Entity_Player`；
- `ClientId -> LocalPlayerOrdinal` 稳定，进程指针只作当前 generation 的临时关联；
- A/B 输入共用 T0-04 scheduler，且只影响各自玩家；
- 双人 local co-op 下 Authority tick 不漂移、不重复、不因任一 endpoint 缺输入而形成 all-client ready gate；
- death/respawn、room、device reassignment 和 menu teardown 均有明确的失效、重绑或退休结果；
- OriginalOnline、`NetInputDevice`、ready gate、channel `0` 和 Isaac Sync channel `0xFD` 均不承载 C/S local co-op 正向路径；
- 新增 Hook 与既有 45 槽形成一个协调的 install/rollback/restore 事务；
- 全部适用的必选 primary、条件 primary 与 repeatability source 可独立 replay，matrix、repeatability 与 final decision hash-bound 且无缺失的必选项；原生退出不适用须由独立、范围受限的决议验证，不计作 source PASS。05-03 的限域适用性规则已迁移，T0-05 最终总门仍待后续工作包实现。

### 1.3 明确不包含

- entity container、spawn/remove identity、world hash 或 T0-06 工作；
- snapshot/baseline/delta apply、Replica World、预测、rollback 或 reconciliation；
- 生产 Steam transport、房间发现、重连、late join 或动态 roster；
- 自定义 Endpoint 逻辑退出或普通 B 的物理移除；两者须分别冻结语义、授权、实现与验收，不能由 menu teardown 或本次不适用决议推导为已实现；
- 超过两名玩家、单 peer 多本地玩家或运行中交换 `ClientId -> ordinal`；
- OriginalOnline/`NetInputDevice` 复用；
- save、achievement、解锁、统计或持久化行为的产品级兼容；
- virtual device 必做结论；它是证据驱动的条件分支；
- 修改 T0-02 `ModeLatch`、现有 Isaac Sync wire/IPC/Lua API 或产品版本。

## 2. T0-04 固定输入与继承边界

### 2.1 历史目标与输入引用

下表保留旧目标与输入记录用于追溯，不再逐项引用、计算或校验哈希，不构成启动账本或推进要求：

| 输入 | SHA-256 |
| --- | --- |
| canonical target | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| observed approved-unpacked target | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| target equivalence document | `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138` |
| T0-02 mode gate | `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185` |
| T0-03 scoped gate | `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB` |
| T0-03R V5 loop gate | `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA` |
| T0-04 raw prerequisite | `99BA518991FEA36FE78D91AEA6D91D80634D4FD3B314399C4C4203C1BA27D5AC` |
| T0-04 active catalog v2 | `A89A74A251EEAA2510FC0C681247ABD6B0A31A7CC3ED60833C78EF36C9B0DC77` |
| T0-04 active plan v2 | `FE4FC7ABDE09CBB280DA342BD384AB50107CA948A052AC02BA6ADE37BB1DF3BA` |
| T0-04 formal matrix | `54BDD4F7E0070D14DA913C6993C97272D97A91B193BDA5C04E2FCAE3E9C6889D` |
| T0-04 repeatability | `AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC` |
| T0-04 verification v2 | `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317` |
| T0-04 final decision | `927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C` |

所有 RVA 都相对 canonical image，运行时地址必须是 `moduleBase + RVA`。target、expected bytes、call target、vtable、COL、ABI 或 equivalence 任一变化，均必须创建新的 T0-05 catalog；禁止修改 T0-04 catalog。

### 2.2 可以继承

- `InputStateDraftV1` 24B canonical 和 `InputCmdDraftV1` 64B command；
- sequence、target tick、dedupe、resolve、continuous/edge 合并和 exactly-once；
- queue-full、expired、too-far、stale、identity-conflict 的 reject-without-world-change；
- steady-standard Authority transaction 和正常返回后提交 `authorityTick`；
- `ownerGeneration`、session tombstone、original-once、main-thread 和 fail-closed 原则；
- PE/target preflight、IPC ring、JSONL、离线 analyzer、hash-bound replay；
- Hook 单一 owner、事务安装/回滚、publication close、callback drain、逐槽恢复和 Unsafe 契约。

### 2.3 不得继承

- 任何 T0-04 arm token、claim、mapping、session、PID/creation 或 `Player*`；
- 单 Player vector 恰有一项、ordinal 固定为 `0` 的 active eligibility；
- 单全局 current player/device/source owner；
- OriginalOnline `NetInputDevice`、Online Manager、ready gate 或现有 Lockstep frame store；
- top-level loop 等于 Authority tick、device key 等于网络身份等已推翻假设；
- T0-04 `activeInputAllowed=true` 作为 T0-05 任一新 live run 的授权。

### 2.4 授权层级

| 层级 | 允许行为 | 授权规则 |
| --- | --- | --- |
| Static | 磁盘 PE、反汇编、代码与文档阅读 | 按任务推进；harness 属于最终唯一一次验收 |
| C0 read-only | query/`VM_READ`、对象图观察 | 依用户操作范围；正确性实验计入整体验收 |
| Passive Hook | 观察 join/lifecycle/owner，original exactly-once | 依用户当前授权；不要求旧 gate、哈希或逐场景授权编排 |
| Active | 原生 join 或 synthetic apply | 依用户当前授权和实际接口理解；不以自动门作为前置 |

已有会话授权继续有效；未知范围再补充具体授权。测试失败或中断后记录结果，不自动重跑，不把历史 token、claim 或对象关联用于当前运行。

## 3. 研究模型与实验接口

### 3.1 设备基线

首轮固定使用 Keyboard + 一只 XInput 手柄：

1. 先由用户通过游戏正常操作建立 Offline/local co-op 双人基线；
2. 先观察游戏原生 join、assignment、HUD/index 和 Lua 顺序；
3. 在原生路径和两份 selected-device/apply 被证明前，不创建 virtual device；
4. 物理设备仅为研究载体，不进入 canonical bytes 或网络 identity。

### 3.2 模式证据与实验角色

T0-05 把两类状态正交处理：

| 维度 | 值 | 含义 |
| --- | --- | --- |
| `ModeEvidence` | `Offline` | 原版 Online Manager/backend/roster/online frame 均未激活 |
| `ExperimentRole` | `LocalCoopAuthority` | 当前 create-new T0-05 实验允许研究双玩家 Authority |

`LocalCoopAuthority` 是研究文档术语，不是已实现 enum，不修改 T0-02 `ModeLatch`。OriginalOnline 对照只能使用 `DISARMED_CONTROL` 证据。

### 3.3 两层身份映射

稳定层：

~~~text
session/client registry
  ClientId(A) -> LocalPlayerOrdinal(0)
  ClientId(B) -> LocalPlayerOrdinal(1)
~~~

临时层：

~~~text
(ownerGeneration, LocalPlayerOrdinal)
  -> Player*
  -> assignmentKey
  -> selected InputDevice
  -> Player vector index
  -> HUD/index registration
  -> Lua Isaac.GetPlayer(index)
~~~

约束：

- `ClientId -> ordinal` 在当前实验 session/run 冻结，不随 pointer 或 device key 变化；
- `Player*`、device pointer、assignment key、platform handle 和地址不序列化、不进入 canonical/hash；
- room owner/lifecycle clear、death/respawn、device reassignment 会使临时层失效；重新双采并唯一绑定后才能恢复 apply；
- menu/run teardown 退休稳定层与临时层，旧 generation 只能作为 tombstone；
- null、重复、多匹配、vector 不稳定、HUD/index/Lua 不一致时保持 unbound，禁止猜测；
- `Entity_Player::InitSeed` 仅作为对象关系诊断，不升级为 T0 网络身份。

### 3.4 继承的 scheduler 常量

| 参数 | T0-05 值 |
| --- | ---: |
| input lead | `2` authority ticks |
| future window | `64` ticks |
| transport/input queue capacity | `256` |
| drain budget | `64/tick` |
| dedupe history | `4096` |

这些是 T0 实验参数，不是产品 wire ABI。A/B 必须经过相同 canonical、queue、resolve 与 apply 代码；不得为 A 保留即时输入旁路，也不得让 B 直接写 Player/device/cache/world。

### 3.5 规划中的机器产物

下表保留启动时的 v1 产物家族定义，既有文件及后续版本均不在本轮改写。当前状态以执行状态入口和机器产物为准；适用性迁移按 create-new 规则发布 verification v5r7、decision v3、matrix v3、ledger v4、hypothesis v3、verification v6、plan v6 与 progress v8，只有完整自动门成功后才写入：

| 规划产物 | 责任 |
| --- | --- |
| `cs-t0/ledger/isaac-cs-t0-coop-ledger.json` | 固定目标、候选、证据标签和 supersedes |
| `cs-t0/coop-catalog-v1.json` | RVA/bytes/target/vtable/COL/ABI/slot |
| `cs-t0/coop-plan-v1.json` | 场景、权限层级、阶段门和 fail-closed scope |
| `cs-t0/coop-verification-v1.json` | 纯内存、跨语言、target/owner/lifecycle/restore 自动门 |
| `cs-t0/coop-matrix-v1.template.json` | 历史模板：20 个 base primary、2 个 repeatability 与条件场景；保持不变，新适用性规格见第 13 节 |
| create-new final decision | replay binding、missing case、`t005Unlocked` |

### 3.6 规划中的命令族

接口按工作包逐步实现，任何现有 `input-*` 命令都不得冒充。当前静态、C0、passive capture/analyze、旧六源 passive gate 和新适用性 gate 已实现；loopback 与完整 T0-05 formal gate 尚未实现。命令存在不构成实机授权：

| 规划命令 | 责任 |
| --- | --- |
| `coop-inspect` | 只读静态目标和 ledger 检查 |
| `coop-join-preflight` | 已实现05-04磁盘/历史证据前置检查；当前固定返回静态证据阻断，不打开进程或生成arm资格 |
| `coop-smoke --probe control-no-native` | 已实现：三模式 C0 read-only runtime |
| `coop-smoke --probe passive` | 已实现：original-only passive runtime；非设备场景 53 槽、device v2 为 56 槽，每次实机须独立授权 |
| `coop-smoke --probe loopback` | 未实现：后续 active runtime |
| `coop-analyze` | 已实现：按严格 identity 分派 C0/passive source；既有失败 source 不因文档修订改判 |
| `coop-compare` | 已实现三份独立 C0 source normalized semantic comparison |
| `coop-c0-gate` | 已实现 C0 hash-bound replay、继承 tick gate 与 create-new decision |
| `coop-passive-gate` | 已实现且保持不变：精确六源验收，不接受 N/A 标签，live-07 仍阻断该旧门 |
| `coop-passive-applicability-gate` | 已实现：精确五源＋限域 N/A；无 verification 仅证据 PASS，完整新版 verification 匹配后才开放 05-04 工作包资格 |
| `coop-gate` | 未实现：完整 T0-05 matrix 和 final decision |
| `verify-coop-c0.ps1` | 已实现并通过：C0 自动门、三模式重放、comparison/decision 与 v2/v3/v4 最终产物 |
| `verify-coop-passive.ps1` | 已实现并通过：passive 自动门及继承回归；pre-live PASS 不等于实机 decision PASS |
| `verify-coop-passive-applicability.ps1` | 已实现：完整调用旧 passive verifier，新增独立黑盒、磁盘静态锚点、五源与旧 exit 重放、不变基线及 create-new 版本链 |
| `verify-coop-native-join.ps1` | 已实现05-04阻断门独立检查与继承回归；不验证尚未接入的55槽主动runtime，不解锁实机 |
| `inspect-coop-native-join-static-closure.ps1` | 已实现固定新证据的磁盘条件审查；合法输入仍BLOCKED/exit1，不打开进程或授予arm |
| `verify-coop-native-join-static-closure.ps1` | 已实现独立解码、14工具对抗、9公开黑盒及历史前后哈希复核；不重建生产或认证主动runtime |
| `verify-coop.ps1` | 未实现：完整 T0-05 formal verifier |

## 4. 依赖图

~~~text
T0-04 PASS
  -> CS-T0-05-00 Evidence Bootstrap
  -> CS-T0-05-01 Static Local Co-op Graph
  -> CS-T0-05-02 Three-mode C0 Comparison
  -> CS-T0-05-03 Passive Lifecycle / Mapping
  -> CS-T0-05-04 Native Join Active Proof
  -> CS-T0-05-05 Virtual Device Decision
  -> CS-T0-05-06 Dual-endpoint Isolation
  -> CS-T0-05-07 Replay / Formal Gate
  -> T0-06
~~~

依赖图描述研究内容之间的关系，不设阶段 gate。按当前目标与实际依赖推进；`FAIL` 或 `UNKNOWN` 如实保留，不用旧 `BLOCKED_BY_EVIDENCE` 或解锁字段控制推进。

本次仅修订 native-exit 的适用性契约，不改判失败 source，也不提供通用 skip 豁免。CS-T0-05-03 仍须完成新版本机器迁移和独立验收后，才能解锁 CS-T0-05-04 资格。

## 5. CS-T0-05-00：证据基线与启动账本

### 目的

建立独立、默认不具 active 权限的 T0-05 证据域。

### 前置依赖

- T0-04 final decision 为 `PASS`；
- 工作树中的 T0-04 ledger/catalog/artifact 保持原样。

### 操作

1. 创建 T0-05 ledger、catalog、plan、verification 和 matrix 的新 identity。
2. 绑定第 2.1 节全部 hash，并记录实际文件路径与 schema identity。
3. 将 local-coop manager、join/exit、HUD/index、Lua order、virtual device 全部初始化为 `UNKNOWN`。
4. 登记当前 active runtime 的两个真实缺口：Player vector 必须恰有一项；current player/device/source 为单全局 owner。
5. 冻结 evidence label：`RUNTIME_CONFIRMED`、`STATIC_CONFIRMED`、`EXPERIMENT_CONTRACT`、`REJECTED_HYPOTHESIS`、`UNKNOWN`。
6. 固定 create-new、hash、privacy、零绝对用户路径和零原始 peer identity 规则。

### 产物

- T0-05 ledger v1；
- catalog/plan/verification/matrix v1 草案；
- predecessor hash binding 报告；
- 开放问题与已推翻假设索引。

### 通过标准

- target/equivalence/gate 均唯一且 hash 闭合；
- 所有新候选保持 `UNKNOWN`，没有伪造 RVA/ABI；
- `activeInputAllowed=false`，不存在 arm/token/claim；
- T0-04 文件和 SHA 未改变。

### 失败/停止条件

- predecessor 文件缺失或 SHA 不一致；
- canonical/approved-unpacked identity 混用；
- 需要修改旧 ledger/catalog 才能表达新候选。

### 后续依赖

- 解锁 CS-T0-05-01。

## 6. CS-T0-05-01：Local Co-op 静态路径图

### 目的

在不运行新 Hook 的前提下，定位游戏自身 local co-op 对象创建和发布链。

### 操作

1. 从 Player vector、InputManager/device vector 和 `0x554F84 -> 0x55B310` controller-assignment candidate 向上下游做离线调用图分析；后者继续保持 `UNKNOWN`。
2. 定位 join/exit、assignment owner、第二 `Entity_Player` 创建、vector 插入/移除、HUD 注册、index 更新和 Lua publication。找到移除函数及 ABI 不等于普通 B 可达或可安全主动调用；native-exit 的 caller guard 与构造类型限制见适用性修订。
3. 对每个候选登记 RVA、expected bytes、expected target、ABI、this/stack 参数、返回值、owner、线程、前后置条件、生命周期和反证。
4. 对比 Offline/local co-op 与 OriginalOnline 构造链；OriginalOnline target 只登记为禁止复用对照。
5. 查明 join/exit 是否位于 steady-standard、loading/alternate 或独立 lifecycle owner；不得把 join owner命名为 Authority tick。
6. 为 passive observer 提出最小 slot 集合和完整恢复顺序。

### 产物

- local co-op join/exit 调用图；
- Player/vector/HUD/index/Lua publication 对象图；
- candidate catalog 与 ABI 表；
- passive slot/事件草案和停止清单。

### 通过标准

- 每个拟观察节点都能由固定 bytes/target/ABI 唯一识别；
- OriginalOnline 与 local co-op 路径在账本中明确分区；
- 最小 passive 集合能够纳入单一事务；
- 未确认的字段和 owner 仍标记为 `UNKNOWN`。

### 失败/停止条件

- 第二玩家路径只落入 OriginalOnline/`NetInputDevice`；
- 关键 owner、线程或 ABI 无法静态约束；
- 观察需要读取或持久化不必要的原始输入/身份数据。

### 后续依赖

- 解锁 CS-T0-05-02。

## 7. CS-T0-05-02：三模式 C0 只读对照

### 目的

用零 Hook、零 IPC、零 apply 的双采建立单人、local co-op 双人与 OriginalOnline 双人的对象差异。

### 固定对照

1. Offline 单人 C0；
2. Offline/local co-op 双人 C0，设备为 Keyboard + 一只 XInput 手柄；
3. OriginalOnline 双人 `DISARMED_CONTROL`。

### 操作

- 双采 InputManager/device vector、game root、Player vector、每个 Player assignment、HUD/index 和可用的 Lua 顺序证据；
- 记录 vector ordinal、device kind、assignment relation、`Entity_Player::InitSeed` 摘要和稳定/不稳定 drop；
- local co-op 双人期间只读复核 steady-standard Authority transaction 与 tick 提交次数；
- OriginalOnline 要求零 T0-05 Hook/IPC/mapping/scheduler/apply，并保持现有 online 对象只读；
- 对三组对象图执行结构差异分析，不把地址写入持久 artifact。

### 产物

- 三份 create-new C0 source；
- 三模式对象差异表；
- 双 Player/assignment/HUD/Lua unique-join 报告；
- local co-op tick 复核报告。

### 通过标准

- local co-op 中恰有两个有效、distinct Player；
- 每个 ordinal 恰好对应一个 assignment/device 和一致 HUD/Lua index；
- 双采期间 vector、对象和 assignment 稳定，所有不稳定样本显式 drop；
- 双人 steady-standard pre-attempt proxy 单调且有正增量；正常返回后的 authority commit 只允许通过 T0-03R/T0-04 冻结证据重放判为 `PASS_INHERITED_CONTRACT`，不得冒充本轮直接 `RUNTIME_CONFIRMED`；
- OriginalOnline 对照没有 T0-05 副作用。

### 失败/停止条件

- Player/HUD/index/assignment 多匹配、漂移或无法唯一关联；
- 只能借 OriginalOnline 对象解释第二玩家；
- read-only 对照出现目标写入、Hook mask 或 IPC。

### 后续依赖

- 解锁 CS-T0-05-03。

### 完成记录

截至 2026-08-28，本工作包已 `PASS`：三份 source 均为 90/90 coherent，三模式 comparison、53 槽 composite no-mutation 和 inherited tick gate 全部通过；最终 decision 只设置 `csT00503Unlocked=true`。`coop-plan-v4.json` 已将 observer mask 设为 `0x00000000`、`processReadAllowed=false`，并继续关闭 active、passive、mutation 与 `t005Unlocked`。OriginalOnline 首份失败 source 和首次失败 decision 按 create-new 规则永久保留，不覆盖、不改判。

## 8. CS-T0-05-03：Passive 生命周期与映射契约

### 目的

在 original exactly-once、零 input override 条件下观察双玩家映射的创建、变化和退休。

### 前置依赖

- CS-T0-05-02 `PASS`；
- passive catalog/plan/verification 自动门通过；
- 当前 run 的新独立实机授权。

### 操作

1. 复用现有 passive Player-vector ordinal 脚手架，但为 T0-05 创建新 schema/identity。
2. 捕获 join -> Player create -> vector insert -> assignment -> HUD/index -> Lua publication 的完整有序链。
3. 捕获 death/respawn、ordinary room change、device reassignment 和 menu/run teardown；与 join 合为五个适用场景。普通 B 的冻结 native-exit 路径以静态反证覆盖适用性，不再要求在已证不可达入口新增实测 PASS。
4. 记录 `ownerGeneration`、ordinal、非原始 pointer 摘要、assignment/device 关系、owner/thread/phase 和 original-call count。
5. 生命周期变化时先使临时层 unbound，清 candidate，再重新双采唯一绑定；teardown 退休后不得复活。passive 不创建输入 transport、scheduler、planner 或 apply 管线；后续 active 消费者另须清除失效 generation 的排队输入与快照，遵守生命周期失效契约。
6. 禁止 suppression、synthetic query/apply、world write、`ModeLatch` 修改和自动重试。

### 产物

- 五个适用 passive lifecycle primary PASS source，以及独立、hash-bound 的原生退出适用性 contract 和新版 passive decision；
- `ClientId -> ordinal` 冻结/退休契约；
- `generation + ordinal -> runtime binding` 状态机；
- passive analyzer、slot restore 和事件顺序报告。

### 通过标准

- 五份适用 source 的原分析与独立 replay 均 PASS；不适用决议精确绑定目标、冻结 caller/constructor 反证、live-07 原 BLOCKED 证据与五份 PASS source，不接受泛化 skip；
- join/lifecycle 事件在已证明 owner/main thread 下闭合；
- 每个有效 ordinal 恰好有一份当前 binding；
- 旧 generation 不复活、不消费 command；
- ring 无 gap/overwrite/wrong-thread/Failure；
- 新槽与既有槽全部逐字节恢复。

### 失败/停止条件

- 需要在 passive 阶段改变参数、返回值或世界；
- 生命周期只能由 pointer 相等或墙钟推断；
- restore/drain/producer close 任一不确定。

### 适用性与历史边界

`NOT_APPLICABLE_NATIVE_ROUTE` 由独立 [机器适用性 contract](../../../../../../cs-t0/coop-native-exit-applicability-v1.json) 和新 gate 实施，不是 source analyzer 的新 PASS verdict，也不是通用 skip。范围限定为固定目标、冻结 callsite 与 ordinary true-coop 的 Variant0 构造/main-vector 路径；未证明 Variant 终生不变或整个游戏没有其他退出能力。保留 live-07 原 BLOCKED source、exit Hook、ABI/original-once 和恢复回归，不强行调用删除函数，不用死亡、断连或全 Run teardown 替代单角色退出。

### 后续依赖

- 新版五源＋适用性机器 gate 已正式 PASS，[plan v6](../../../../../../cs-t0/coop-plan-v6.json) 只解锁 CS-T0-05-04 工作包资格并关闭 passive capture 资格；旧六源 gate 仍 BLOCKED，active 实机未授权。

## 9. CS-T0-05-04：游戏原生 Join 主动证明

2026-08-31 实施更新：用户已批准[实施计划](../02-active-join/native-join-active-implementation-v1.md)及基于证据的限域主动离线判据修订。55槽（旧53＋两处PUSH callback operand）为候选路线，尚非runtime-complete。新的只读preflight只核对静态目标、历史链和反证；持久化写入及Lua回调排除未闭合前，保持主动门阻断。`backend`/manager对象存在本身不能等同Online激活，但也不赋予主动权限；不得用未来join后见证证明prejoin。

### 目的

证明 Authority 能通过已理解的游戏原生 local co-op 路径建立第二玩家，而不是借用原版 Online session。

### 前置依赖

- passive 五源＋原生退出适用性的新版本机器 gate `PASS`，此前置已由 [decision v3](../../../../../../cs-t0/coop-passive-decision-v3.json) 满足，可开始本工作包实现与前置验证；
- 本工作包自己的 target/owner/lifecycle/restore verification 为 runtime-complete；当前 passive 自动门不证明 active runtime 已完成；
- 新 create-new arm/token/claim 和单次 active 授权。

### 操作

1. 重放全部 predecessor 与当前 process creation/preflight。
2. 仅调用或驱动已在 CS-T0-05-01/03 证明的原生 local co-op join 路径。
3. 在任何UI覆盖或publication前，以独立主动契约证明 `ModeEvidence=Offline`：已知可读的backend/manager对象、canonical-zero roster三指针、zero online frame及精确local路径；未知对象、在线正向信号或布局漂移均拒绝。此新限域契约尚未证明，不沿用passive后见证，也不修改T0-02 ModeLatch。
4. 验证第二 Player、vector、assignment、HUD/index、Lua order 与 generation binding。
5. 不在本阶段注入 B gameplay input；先证明创建语义本身。
6. 正常停止时按统一事务关闭 publication、drain、退休、恢复并逐槽复核。

### 产物

- `native-join-active` source；
- join invocation/owner/original-once 证明；
- 双玩家发布与零 OriginalOnline 激活报告；
- target before/after 和恢复报告。

### 通过标准

- Authority 内有两个由原生 local co-op 路径创建的有效 Player；
- HUD/index/Lua 顺序与 frozen ordinal 一致；
- 没有 save、achievement、unlock、统计或 run 外持久污染；
- 没有 Online Manager/ready gate/channel `0`/`0xFD` 依赖；
- 所有 Hook 恢复且进程继续响应。

### 失败/停止条件

- 必须进入 OriginalOnline 才能创建第二玩家；
- 只能调用未理解 owner/ABI/lifecycle；
- 观察到不可逆持久化或无法证明恢复。

### 后续依赖

- 解锁 CS-T0-05-05。

## 10. CS-T0-05-05：Virtual Device 决策门

### 目的

以证据判断 virtual adapter 是否为 T0-05 必需，而不是把它预设为组件。

### 决策 A：`NOT_REQUIRED`

若原生 join、两份 selected device 和既有 query/apply adapter 已足以：

- 在无第二个网络/online 对象时维持 P0/P1；
- 让 B canonical input 只进入 ordinal `1`；
- 在 lifecycle 后安全重绑；
- 保持设备/UI 原路径 original-once；

则生成 create-new `NOT_REQUIRED_NATIVE_PATH_SUFFICIENT` decision，后续不得为“架构对称”继续实现 virtual device。

### 决策 B：`REQUIRED`

仅当原生路径无法在不依赖第二物理设备的 active 阶段维持第二 selected-device/apply，才进入 virtual 分支。进入前必须全部证明：

- owner、allocation/deallocation 和调用线程；
- concrete vtable、COL、所有会被调用的 slot 与 ABI；
- assignment、join/exit、generation 和 device-vector ownership；
- UI/gameplay action 分区、original/passthrough 规则；
- install/rollback/drain/restore 与异常退休；
- 不伪造平台 handle、Online Manager 或未知 controller state。

随后只允许一个 passive canary 和一个最小 active canary；两者均为条件 matrix case。

普通 B 的 native-exit 不适用不豁免 virtual device 自身的 detach、退休、析构、引用清理或恢复证明；设备生命周期与游戏角色物理移除是不同契约。

### 产物

- virtual-device decision；
- 若 `REQUIRED`：独立 catalog/ABI/lifecycle 表、passive source 和 active source；
- 若 blocked：保留 `BLOCKED_BY_EVIDENCE` 及缺失项。

### 通过标准

- decision 只能为 `NOT_REQUIRED_NATIVE_PATH_SUFFICIENT` 或证据完整的 `REQUIRED_ELIGIBLE`；
- `REQUIRED` 分支新增槽已纳入同一事务；
- 未知 owner/vtable/lifecycle 不得由 runtime 猜测补齐。

### 失败/停止条件

- 需要伪造未理解 owner、vtable、COL、platform handle 或 controller lifecycle；
- virtual device 无法完整退休/恢复；
- decision 依赖“看起来像手柄”的对象形状而非 provenance。

### 后续依赖

- `NOT_REQUIRED` 或 `REQUIRED` 条件 source `PASS` 后解锁 CS-T0-05-06；
- 否则 T0-05 为 `BLOCKED_BY_EVIDENCE`。

## 11. CS-T0-05-06：双 Endpoint 输入隔离

### 目的

把 T0-04 单 Player active adapter 扩展为按 ordinal 分区的 A/B owner/device/witness，并证明统一 scheduler 下的隔离。

### 操作

1. 将单全局 current player/device/source 改为两个固定 ordinal slot；任何未知或第三 ordinal fail closed。
2. A 物理输入先 canonicalize，再走 loopback；B scripted input 走 `FaultInjectionTransport`；两者共享 queue、dedupe、resolve 和 apply。
3. 每个 Player owner wrapper 只访问本 ordinal 的 current generation binding。
4. 相同 sequence 在不同 `ClientId` 下保持独立；同 client 同 identity 同内容幂等、不同内容冲突退休。
5. continuous state、shoot held 和 edge 分别按 client resolve，禁止跨 client 合并或消费。
6. pause/loading/menu/未绑定 generation 不 apply；room/death/device change 在重绑前 neutral。
7. 记录 canonical、decision、target tick、ordinal、owner generation、apply 和批准 witness 的同一 identity。

### 产物

- active isolation primary source；
- 1,000 authority tick soak source；
- per-ordinal owner/device/witness analyzer；
- rejection isolation 与 lifecycle rebind 报告。

### 通过标准

- A-only/B-only 只改变对应玩家；
- 同时移动、射击和 edge 不串线、不重复、不丢给错误 ordinal；
- 连续 1,000 authority tick 内零 stale generation、pointer 失效或 duplicate apply；
- 所有 reject 保持两名玩家 world witness unchanged；
- host 不保留即时旁路，B 不直接写游戏对象；
- pause/loading/menu 与生命周期 fail-closed 行为符合计划。

### 失败/停止条件

- 正常 P0/P1 交替仍被误判为 owner switch；
- 任一 command 可在两个 ordinal apply；
- pointer/device key 进入 canonical、command 或持久 identity；
- 原始路径在同 tick 再次消费同输入。

### 后续依赖

- 解锁 CS-T0-05-07。

## 12. CS-T0-05-07：Replay、恢复与正式门

2026-10-12 修订：本节下列多 source replay、SHA、矩阵与 final decision 流程为旧方案，不再执行或作为要求。当前推进在完成整个目标后只执行一次整体验收，直接记录映射、输入隔离、生命周期与恢复的实际结果，不另建正式门。

### 目的

把全部 source 固化为可独立重放的 T0-05 正式结论。

### 操作

1. 对每个 source 重算 manifest/events/analysis/target before-after SHA。
2. analyzer 从 source 重新验证 mapping、lifecycle、tick、canonical、decision、apply、witness、ring 和恢复，不信任可编辑摘要。
3. 按新版适用性契约继续填充 create-new matrix：保留 20 个条目编号，19 个适用 base primary 加 1 个原生退出不适用决议；根据 virtual decision 决定是否追加两个条件 primary。旧模板/矩阵保持不变，适用性迁移只闭合 C0/passive 部分，不代替完整 T0-05 门。
4. 取两次独立的 1,000-tick 相同脚本 source，执行 normalized semantic comparison。
5. 只归一化 PID、QPC、绝对 tick、token、进程地址等非确定身份；stage、相对 tick、client/ordinal、generation transition、canonical、edge、decision、apply 和 witness relation 必须相同。
6. 生成 create-new final decision，列出全部 source path/SHA、missing case、原生退出适用性及能力限制、virtual decision 和 `t005Unlocked`。

### 通过标准

- 适用 base primary `19/19 PASS`，另有 `1/1` 原生退出不适用决议通过验证（不算实测 PASS）；virtual `REQUIRED` 时条件 primary 另为 `2/2`；
- repeatability source `2/2`，comparison `1/1 PASS`；
- 所有必选正向 source analysis 独立 replay 后仍为 `PASS`；历史 live-07 保持 BLOCKED，仅作为不适用决议的反证输入，不冒充正向 source；
- sequence 连续、ring overwrite 为零、无 wrong-thread/phase/depth/original-count 错误；
- OriginalOnline disarmed 与 control-no-native 均为零副作用；
- 全部 target slot before/after 逐字节一致，producer closed，client retired；
- final decision 为 `PASS`、missing case 为空、`t005Unlocked=true`。

### 失败/停止条件

- 需要修改 analyzer 规则才能解释既有失败 source；
- source/hash/provenance 无法唯一绑定；
- 任一 mandatory 或条件 mandatory case 缺失；
- restore、drain、Unsafe、pointer stale 或污染问题未闭合。

### 后续依赖

- 仅正式 `PASS` 解锁 T0-06。

## 13. 正式场景矩阵

### 13.1 Base 条目：19 个适用 primary＋1 项适用性决议

保留原 20 项及序号，仅第 6 项改为有证据的不适用条目，不计入 primary PASS 数。[matrix v3](../../../../../../cs-t0/coop-matrix-v3.json) 已按新规则创建：19 个适用 primary 中完成 9 项（4 个继承 C0/composite＋5 个 passive），另有 1 项 N/A；11～20 仍 NOT_STARTED。旧 JSON 矩阵、六源 gate 与 progress v7 均未改写。

| 序号 | 场景 | 层级 | 必须证明 |
| ---: | --- | --- | --- |
| 1 | `control-no-native` | C0 | 零 Hook/IPC/apply，target 不变 |
| 2 | `offline-single-c0` | C0 | 单 Player 基线与旧语义不回归 |
| 3 | `local-coop-dual-c0` | C0 | 双 Player/assignment/HUD/Lua unique mapping |
| 4 | `original-online-dual-disarmed` | C0 | OriginalOnline 只读、T0-05 零干扰 |
| 5 | `native-join-passive` | Passive | join 完整事件链与 original-once |
| 6 | `native-exit-passive` | 适用性决议，非 PASS source | 当前冻结 ordinary route 为 `NOT_APPLICABLE_NATIVE_ROUTE`；hash-bind 静态反证和 live-07 原 BLOCKED 证据 |
| 7 | `death-respawn-passive` | Passive | binding 失效和唯一重绑 |
| 8 | `room-rebind-passive` | Passive | room generation clear/rebind |
| 9 | `device-rebind-passive` | Passive | 两份 assignment 的前后唯一 join |
| 10 | `menu-teardown-passive` | Passive | run teardown 清稳定层和临时层 |
| 11 | `native-join-active` | Active | 原生创建第二玩家且零在线依赖/污染 |
| 12 | `endpoint-a-only` | Active | A 只影响 ordinal 0 |
| 13 | `endpoint-b-only` | Active | B 只影响 ordinal 1 |
| 14 | `simultaneous-move` | Active | 两份 continuous state 隔离 |
| 15 | `simultaneous-shoot` | Active | 两份 aim/shoot held 隔离 |
| 16 | `same-tick-edge` | Active | 跨 client edge 独立、同 client collision 拒绝 |
| 17 | `reject-isolation` | Active | full/expired/too-far/stale/conflict/duplicate 不串扰 |
| 18 | `pause` | Active | pause 期间不 apply，恢复后不补发旧 edge |
| 19 | `loading` | Active | loading/alternate 未证明路径 fail closed |
| 20 | `dual-isolation-soak` | Active | 连续 1,000 authority tick 全不变量 |

### 13.2 条件 primary

仅当 virtual decision 为 `REQUIRED_ELIGIBLE` 时，矩阵额外要求：

| 场景 | 必须证明 |
| --- | --- |
| `virtual-device-passive` | owner/vtable/ABI/generation/join/exit/restore 观察闭合 |
| `virtual-device-active` | B 经 virtual adapter 只影响 ordinal 1，完整退休和恢复 |

### 13.3 Repeatability

- `dual-isolation-soak-extra-a`；
- `dual-isolation-soak-extra-b`。

两次必须是不同 PID/creation、不同 token/claim、相同冻结脚本、相同场景参数和相同 virtual decision。任何 source 不得同时占据 primary 与 extra 槽。

## 14. 自动化与实机顺序

### 14.1 唯一一次整体验收

每次推进最小为 `tx-0x-0x`。先完成整个目标的分析、设计与实现，再选定一个统一入口执行一次整体验收。探讨过程不运行测试，不要求 Cargo/Native 全量、预验证、smoke、probe、四组压缩批次或重复 replay。文档任务只做一次静态验收。

### 14.2 操作与结果

实机操作依用户当前授权执行，不设置 gate，不校验哈希，不强制 TEST/REVERSE 分离或逐场景授权编排。必要用例纳入该次唯一整体验收；完成后直接报告行为、输入隔离、生命周期、恢复、失败和未决项。失败不自动重跑。

历史验证结果和脚本保留为资料，不恢复其执行要求。

## 15. 统一停止条件

任一条件成立即停止当前工作包，不自动重试：

- 第二玩家只能通过 OriginalOnline、`NetInputDevice` 或 ready gate 创建；
- 需要猜测 owner、vtable、COL、ABI、controller/platform lifecycle；
- `Player/HUD/index/assignment/Lua` 无法唯一绑定；
- 出现第三 ordinal、null/duplicate Player 或不稳定 vector；
- save、achievement、unlock、统计或 run 状态发生不可逆污染；
- Hook 无法纳入单一事务，或 rollback/restore/drain/producer close 不确定；
- 事件 sequence gap、ring overwrite、wrong-thread、phase/depth、original-count、hash 或 target bytes 违约；
- reject 改变任一玩家 world witness；
- 旧 generation、pointer、token、claim 或 session 被复用；
- OriginalOnline 对照出现 T0-05 Hook/IPC/scheduler/apply；
- 需要删除字段、跳过必选 case 或放宽 analyzer 才能通过。

不安全停止时保持进程和诊断上下文，不得强制卸载、盲目重注入或覆盖失败 artifact。

原生退出适用性修订只能通过范围受限的静态反证及新版本契约实施；不能将此项扩展为删除其他 mandatory case、接受 Unsafe 或把旧失败 source 改成 PASS 的例外。

## 16. T0-05 最终出口

2026-10-12 修订：当前出口是该次 `tx-0x-0x` 的一次整体验收结果及未决事项，不生成 gate，不校验 SHA，也不要求 `t005Unlocked=true`。后续内容保留旧阶段完成规格和历史状态语义；能力是否实现直接说明，旧解锁要求不再适用。

### `PASS`

需要报告整个目标是否完成，并提供以下功能资料；不以 formal gate 或解锁字段限制后续推进：

- local co-op/player/device/HUD/Lua 对象图；
- 两层 mapping 与 lifecycle 状态机；
- virtual decision 及条件证据；
- per-client scheduler/isolation 结论；
- 新版 matrix：19 个适用 primary＋2 个 repeatability source＋1 个不适用决议；`REQUIRED` 时 primary 为 21 个，其他数量不变；另附 comparison 和 final decision（当前 matrix v3 仅完成 C0/passive 部分，T0-05 总体未完成）；
- 全部 source/hash/restore 索引；
- 已推翻假设和仍为 `UNKNOWN` 的项目。

### `FAIL`

存在反证、污染、隔离错误、重复 apply、恢复失败或正式 source 失败时记 `FAIL`，保留原始证据，不改判。

### `BLOCKED_BY_EVIDENCE`

旧 `BLOCKED_BY_EVIDENCE` 标签保留为历史语义。当前直接说明 owner/ABI/lifecycle、virtual adapter 或操作安全方面的未决问题与实际限制，不设置 T0-06 解锁流程。

T0-05 的成功定义是证明“两名 local co-op 玩家可以被统一 Authority 输入管线安全、可审计地分别控制”，不是把双玩家输入原型包装成状态复制或可玩 C/S 联机。

## 17. CS-T0-05-03 pre-live 实施记录（2026-08-28）

逆向与测试已由不同子智能体独立完成，主线只负责公共 codebook/ABI、Rust CLI/analyzer/gate、工程接线、版本链和交叉验收。生产 `coop_passive` runtime、固定 IPC、八个 original-only wrapper、两层 mapping、唯一有序 53 槽事务及 Unsafe shutdown 已实现；测试侧从公开边界独立验证六场景 contract、真实 Win32 mapping、x86 ABI/original-once、逐点回滚/恢复、mutation 和隐私约束。

`coop-catalog-v3`、`coop-plan-v5` 与 `coop-verification-v5` 构成 pre-live create-new 链。plan v5 唯一为 true 的能力字段是 `passiveHookTransactionAllowed`；本阶段没有创建 live authorization、没有运行 `coop-smoke --probe passive`，也没有生成六场景正式 source。自动门通过后仍按 14.2 的固定顺序，从 `native-join-passive` 开始逐场景请求新授权。

## 18. device-rebind v2 修订（2026-08-31）

用户明确授权修订设备取证计划与判定，并重跑自动门；该授权不包含实机。旧 manager-only 关系变化不能证明设备重分配，旧 device v1 不再允许正向验收。完整修订见 [device-rebind v2 取证计划](../01-passive-lifecycle/device-rebind-v2-evidence-plan.md)。

设备场景增加三个静态已证 original-only assignment callsite，形成独立 schema/IPC v2 的56槽事务；非设备场景保持53槽和v1重放。正向条件是既存双BOUND → 原生unassign/readback → 原生reassign/readback → 完整manager topology → 连续双采BOUND。只接受菜单53→54→55或断连54→55的精确begin/end语法，同设备重接允许；拓扑/摘要变化、用户时间或自报flags均不能替代因果链。

新pre-live入口为 `coop-catalog-v4`、`coop-plan-v5r1`，完整自动门成功后才创建 `coop-verification-v5r5`。历史三份PASS source、native-exit workflow skip与旧阻断记录保持不可变。六源gate不因skip而豁免；原有零主动能力和逐次实机授权/失败不重试边界不变。

## 19. device-v2 shutdown generation 修复（2026-08-31）

live-10 的 B 原生重连链在 gen3 完成，但 shutdown 私有清理推进 generation，导致两个终止事件为 gen4、manifest 仍为 gen3。用户授权修复该一致性问题、补回归并重跑自动门；没有新的实机授权。修复限定为停止观察后的身份清理不推进证据 generation，并使 device-v2 terminal Publish 失败保持 Unsafe；不改变真实生命周期失效、legacy53、IPC、catalog/plan 或 analyzer。

新增真实生产 Shutdown→Event/Publish→独立序列化→公开 CLI 的回归，不再以 fixture 补 terminal envelope 代替端到端收尾验证。修复前须复现 generation 与 terminal ring 失败，修复后覆盖完整/未完成路径、private identity 清空、generation 边界、drain/恢复/terminal failure 和关闭后不复活。完整自动门成功后仅 create-new `coop-verification-v5r6`；live-10 原 source、分析和失败结论永久保留，正式进度仍为3/6，下一次 device 必须新授权。

## 20. 原生退出适用性文档修订（2026-08-31）

用户授权修订文档。依据冻结 caller 的 `Variant != 0` guard 与 ordinary true-coop 的 `Variant=0` 构造反证，native-exit 不再作为当前普通 B 的无条件实机前置；这不等于已证明单角色移除安全。完整范围、证据绑定及后续机器迁移要求见 [适用性修订 v1](../01-passive-lifecycle/native-exit-applicability-revision-v1.md)。

live-06/08/09/11/12 五份适用 source 已 PASS，live-07 仍 BLOCKED；旧机器进度为 5/6，CS-T0-05-04 未解锁。当前待办是另行授权新版适用性 gate 的实现及独立自动验收，不是继续尝试普通 B 不可达的退出入口。自定义逻辑退出和物理移除留待后续独立工作包，不与原生 join 主动证明捆绑。

本轮仅改文档，未修改生产代码、analyzer、测试/verifier、机器 JSON、历史 source/analysis 或二进制，未运行自动门或新实机；所有现有权限与失败结论保持不变。

## 21. 原生退出适用性 gate 迁移（2026-08-31）

用户另行授权实现与重跑自动门。新 `coop-passive-applicability-gate` 精确绑定五份 PASS、live-07 原 BLOCKED、29 个静态锚点和 149 文件不变基线；旧 source analyzer、六源 gate 与 Native 均不变。独立测试覆盖重复权限键、当前 EXE 替换、scope/source/verification 篡改，以及真实 PowerShell 参数边界；不提供通用 skip。

完整自动门 v3 为 28/28 PASS，create-new 生成 verification v5r7、decision v3、matrix v3、ledger v4、hypothesis index v3、verification v6、plan v6 与 progress v8。前两次失败日志及原有失败 source 保留。详细版本链、限定结论与验证记录见 [gate 迁移记录](../01-passive-lifecycle/native-exit-applicability-gate-v1.md)。

当前只开放 `csT00504Unlocked=true`。passive Hook/观察、active/input/world/runtime mutation、override、transport、scheduler、apply、ModeLatch、OriginalOnline 正向能力和 `t005Unlocked` 全部 false；没有新 live source、实机授权或自定义退出实现。后续可开展 05-04 实现与前置验证，active 实机仍须新单次授权。

## 22. CS-T0-05-04 证据前置实施（2026-08-31）

已保存164文件前置快照（包括149文件历史基线和旧生产二进制），冻结53个原始字节锚点及53＋2槽候选目录，并实现只读 `coop-join-preflight`。正常静态输入仍得到 `BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`，不会创建进程读取、mapping、arm/claim或注入路径。输出create-new并拒绝历史source、Windows设备别名和备用数据流。

[静态交接](../02-active-join/native-join-active-reverse-v1.md)与[补核纪要](../02-active-join/native-join-active-followup-v1.md)已把风险定位到ordinary Init、P0更新、join尾部、属性/cache回调和具体保存入口。限定ordinary join正向路径已有独立离线静态依据，仍须形成新版可执行准入/稳定性契约；无持久污染条件尚未闭合。不得用低金币、单一readonly位、无player-init回调或文件哈希不变代替完整证明。

当前实际交付、未实现接口及后续补证顺序见[实施记录](../02-active-join/native-join-active-implementation-v1.md)，独立验收边界见[测试计划](../02-active-join/native-join-active-test-plan-v1.md)。只读阻断门与继承回归的通过不等于55槽主动runtime通过；完整runtime、IPC、授权消费、主动source/replay和单次实机仍待后续条件满足。矩阵第11项及05-05/T0-06解锁状态不变。

完整阻断门自动验证已通过并发布[verification v1](../../../../../../cs-t0/coop-native-join-verification-v1.json)，实际执行与失败保留见[独立验证结果](../02-active-join/native-join-active-test-results-v1.md)。当时机器进度为[plan v7](../../../../../../cs-t0/coop-plan-v7.json)及[join progress v1](../../../../../../cs-t0/coop-native-join-progress-v1.json)，状态仍为`BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`，未发布05-04正式PASS或实机资格。

## 23. CS-T0-05-04 限域条件补证与独立审查（2026-08-31）

普通显式seed的原生readonly建立路径，以及Init/tail/cache的局部分支条件已经细化并冻结。两份新证据共146锚点、1,994条指令、456条边；独立解码、14组工具对抗和9组公开黑盒通过。Init v1的22条空助记符在create-new v2修正，原文件与失败审计保留。完整结果见[测试记录](../05-persistence/native-join-static-closure-test-results-v1.md)；该通过不是全函数可达性、无污染或55槽runtime证明。

新的[plan v8](../../../../../../cs-t0/coop-plan-v8.json)与[progress v2](../../../../../../cs-t0/coop-native-join-progress-v2.json)为`BLOCKED_BY_EVIDENCE / STATIC_CONDITION_REVIEW`。后续静态补证限定为两项：fresh B在创建前不可观测的有效状态必须由constructor/Init数据流证明；Init及中间自然更新必须保持声明的timer/request/lifecycle/readonly条件，避免下一pass读回前已经发生保存。不能把受共同readonly保护的health setter反复扩张为新的无界追踪任务。

完整bounded reader、vector/device替换及generation/ABA规则另列实现义务，不依赖未来passive见证。原生产程序和旧验证链未变，旧preflight仍报告七项缺口；新条件schema将义务细分为12项，不回写旧codebook。主动runtime/IPC/arm、55槽事务、source/replay和单次实机的后续门不变；矩阵第11项及05-05/T0-05整体/T0-06仍未解锁。[发布后独立审计](../../../../../../out/cs-t0/native-join-static-closure-publication-audit-v1.json)已确认新旧绑定、诊断、缺口、计数及权限一致，不授予主动能力。

## 24. CS-T0-05-04 静态接续与准入离线规格（2026-09-01）

本轮只实施静态证据、规格和磁盘工具，不接入主动runtime。Init v3补充pending trinket指针/字节flags、pillHistory、familiar carry与有效来源传播；持久化窗口v2明确覆盖首次publication前、选择pass、确认pass、下一完整pass的双BOUND，以及disable/drain/恢复/terminal之间的所有自然更新。至少三个manager pass，不能以两个UI edge、端点采样或经验秒数代替完整窗口。

[准入契约v1](../../../../../../cs-t0/coop-native-join-admission-contract-v1.json)登记191项reader描述（180项有静态来源，11项UNKNOWN）、21个guard与17类有效来源。InputManager为静态对象，roster必须三指针canonical-zero；其余容器按各自布局规则判定，不能把已分配但全零数组一律当作无效。读取预算仅为离线规格设计值，不宣称引擎限制或生产reader已实现。B创建前的条件不能由未来join见证补齐。

generation归adapter所有；publication前、两个callback pass及后续绑定和停止阶段均规定采样点。owner/vector/device/shared-control替换、生命周期失效、双读漂移、读取失败或无法排除ABA均永久失能，不自动重试；恢复不确定保留Unsafe。这个失败策略已规定，但原生生命周期/ABA覆盖仍未证明，因此readerContractComplete和nativeAbaExclusionProven均为false。

新[verification](../../../../../../cs-t0/coop-native-join-admission-verification-v1.json)从头执行后退出0，只确认静态证据与离线规格审查；合法inspector仍退出1、整体BLOCKED_BY_EVIDENCE。独立语义审查与哈希/锚点检查分开，反例覆盖隐藏有效来源、未来B读回、readonly直接写、dirty置位后flush归零、两次干净采样间保存、遗漏第三pass或停止等待，以及容器/读取/身份错误。实际计数与所有失败记录见[测试结果](../03-admission/native-join-admission-test-results-v1.md)。

完整原生证明仍有四类精确待办：Init中间callee对已归零来源的保持及全部desired/matching-familiar路径；被同步更新P0的完整有效来源与拓扑；Room/Grid/player虚调用、Lua及lifecycle在整个自然窗口内对guard/readonly的保持；未解config/container/callback/entity reader与原生ABA依据。局部反例只推翻不足的前提，未获得满足全部候选条件仍污染的完整反例，也没有证明候选场景安全。固定普通显式seed、初始保存完成的新起始房、Keyboard P0、唯一未分配XInput B、普通Isaac与无mods/特殊effects/相关callback/脏persistent/pending等条件不放宽；无法提供依据时阻断。

v9/v3首次发布遗留旧verifier入口与scope，已保留原件及发布审计失败；create-new的plan v10 / join progress v4仅修正发布元数据，保留原12项未决义务，另列一项SPECIFIED的失败策略；不将规格完成等同运行验证。原生产preflight仍保留七项缺口和退出1，旧脚本/测试/生产文件未改，所有旧失败和新重试产物保留。矩阵第11项NOT_STARTED，active/live、05-05、T0-05整体和T0-06继续关闭。

## 25. manager-this 同代身份静态补证暂停检查点（2026-09-02）

本轮按照当前文档前沿把 manager-this 补证切为候选逆向、作者分离测试、机器账本合成与最终 formal admission 四类任务。用户要求暂停后，所有执行中子任务均已停止，状态完整固化于 [pause checkpoint v30](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v30.md)。暂停时正向计数为 copy/move `319/405`、memset `225/242`，精确剩余 `103`；独立负向覆盖为 memset `17/17`、copy `69/86`。负向覆盖只说明候选尚不能接受，不得作为 proof closure。

恢复顺序固定为：先由独立测试角色完成 callerHeap exact17 的作者分离审计；再由逆向角色从 `0x007B3C61 -> 0x005B39D0` 接续 save-record exact1 stage3；随后 create-new successor frontier，绑定 v29 之后完成的 EntityList、vectorShift、string 以及新验收的 callerHeap 审计。只有剩余行获得正向 backing extent/identity/generation/lifecycle 证明并经独立验收后，才能推进 whole-image raw v2 与 manager-this v7 formal artifacts。

暂停不改变正式计划：plan v10 / join progress v4 继续 `BLOCKED_BY_EVIDENCE`，不开始实机、动态调试、注入、主动输入或权限申请，不更新矩阵与解锁字段。

## 26. manager-this 第二次暂停检查点（2026-09-05）

最新冻结状态见 [pause checkpoint v31](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v31.md)。callerHeap exact17 的独立测试已完成并接受 `0 VERIFIED / 17 UNKNOWN`，因此剩余 copy/move 的作者分离负向覆盖现为 `86/86`，memset 为 `17/17`；但正向账本仍为 `319/405` 与 `225/242`，精确 remaining 仍为 `103`。不得把审计覆盖写成 no-overlap closure。

本轮未合成 successor frontier。测试角色留下的参数化 validator/self-test 在 handoff 和真实 candidate 验收前中断，按未封印准备处理。save-record stage3 将 v24 的首缺口候选从 `0x007B3C61 -> 0x005B39D0` 向前收窄到 `0x007B3C41 -> 0x00753620` 及其后续 live-state kill，但没有新机器产物或独立验收，权威状态仍是 v24 UNKNOWN。

恢复时先完成 validator 的作者分离封印，再由主线 create-new successor、由测试角色验收；任何 save-record 正向候选另走独立测试。当前按用户要求不继续逆向，plan v10 / join progress v4 与所有关闭权限不变。

## 27. manager-this v30 负向覆盖账本收尾（2026-09-05）

按照 v31 的恢复顺序，但不恢复逆向，已完成三阶段作者隔离收尾：TEST 角色封印 successor validator；主线 create-new 生成 v30 candidate；TEST 角色用已封印 validator 验收真实 candidate。详细产物、命令、哈希和暂停边界见 [pause checkpoint v32](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v32.md)。

当前接受机器 frontier 为 v30：copy/move 负向审计覆盖 `86/86`，memset `17/17`；正向账本仍为 `319/405` 与 `225/242`，remaining `103` 全 UNKNOWN。负向审计覆盖不是 no-overlap 或同代 identity 证明，不允许推进 whole-image/raw/formal v7，也不允许提升 manager-this、准入、runtime 或解锁状态。

后续如果用户恢复正向研究，save-record `0x001CBEB5` 仍从 v24 UNKNOWN 与 v31 未封印 observation 开始，不能把 observation 当证据；任何新正向 candidate 必须由独立测试角色验收并进入新的 v31+ frontier。当前按用户要求继续暂停逆向，plan v10 / join progress v4 和全部权限保持不变。

## 28. manager-this 完整证明任务分解 v33（2026-09-05）

基于当前 v30 与 v7 production/pre-seal 代码做了只读完成性审计，结果固化为 [任务分解 v33](../06-manager-identity/00-overview-and-gates/native-join-manager-this-proof-completion-work-breakdown-v33.md)。103 个 UNKNOWN 按实际首缺口拆成 29 个 exact 正向微任务，覆盖 callerHeap 17、localHard 7、vectorGrow 23、vectorShift 25、string 14 和 memset 17；每个逆向 candidate 必须由不同 TEST 角色逐行验收，再由主线 create-new 合成 v31+ frontier。

完整依赖链固定为：29 个正向分片与 audit → copy 405/405、memset 242/242、bulk 647/647 zero frontier → whole-image raw v2 与独立审计 → v7 reader/whole-image/sidecar/contract 四件套 → contract hash 和无环 publication seal → 11 项正式独立验收。最终 manager-this 应为 `STATIC_CONFIRMED`，但 overall 仍为 `BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES / exit 1`，八个其它 manager-context premise 与所有权限保持 UNKNOWN/false。

当前用户未恢复逆向，29 个任务均为 `PAUSED_BY_USER`；本轮没有生成正向 candidate、raw v2 或 formal artifact，不改变 v30 的 319/405、225/242、remaining103 和所有关闭状态。

## 29. MT-R01–R10 follow-up 与 v33 frontier（2026-09-06）

用户恢复前十项尚未闭合行后，两个 REVERSE 角色按冻结 PE/IDA 静态证据分组推进，一个独立 TEST 角色执行 exact-row 验收。`MT-R04` 按用户要求暂时跳过 follow-up TEST，因此正式审计范围为 exact16；R04 两行没有新 verdict 或 promotion。

独立 TEST 新接受 `0x0061F2C2` 与 `0x0068E3DB`，其余 14 行保持 UNKNOWN。主线据此生成 v33 frontier，第二层独立 TEST 从 v32 和 exact16 audit 完整重建并通过：copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`。前十项累计为 `4 positive / 16 UNKNOWN`；详细证据与停止点见 [checkpoint v36](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r01-r10-followup-checkpoint-v36.md)。

测试仅保留 create-new、一次 verify-existing、核心 mutation 和 wrong-hash fail-before-write；没有运行 Cargo、全量 T0 build 或重复 self-test。没有启动游戏、打开进程、使用 x32dbg 或修改 IDB。manager-this、whole-image raw v2、formal v7、active/live、runtime mutation、05-05、T0-05 总门和 T0-06 仍保持关闭。

## 30. MT-R02/R03/R05 exact5 深入复核（2026-09-06）

本轮按用户指定只继续 R02、R03、R05，并在候选出现前由独立 TEST 封印 exact5 验收规则。静态复核确认 R02 没有 `oldCount<=newCount` 硬关系，R03 的 5000 分支只记录后回流，R05 的 aux-nonempty/filtered-empty 合取没有支配性排除。五行均保持 UNKNOWN，详细证据见 [checkpoint v37](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-r03-r05-checkpoint-v37.md)。

最终 independent audit v2 为 `0 positive / 5 UNKNOWN`，SHA-256 `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE`。测试 harness 的两次预写失败均未生成 audit；append-only v2 只修正全 UNKNOWN mutation 的适用性和逐行变量复用，不改变封印 contract。没有正向行，故不创建空 frontier successor，v33 的 `323/405`、`225/242` 与 remaining `99` 全部不变。

本轮没有动态实验。R05 的未来存在性复核需要在 `sub_734F30` 同时观察 aux range 非空与 filtered count 为零，但尚无已证可靠的用户游戏场景；普通采样也不能替代全称正向证明。manager-this、whole-image raw v2、formal v7、active/live、runtime mutation、05-05、T0-05 总门和 T0-06 继续关闭。

## 31. MT-R05 filtered-vector v3 单行有界复核（2026-09-06）

本轮只接续 `MT-R05 / 0x0033570B -> 0x00734F30`。冻结 IDA 静态证据确认 auxiliary vector 的正常构造路径产生 14 项，但 filtered tuple 先被清零；源计数可直接进入空 merge，逐项 reject 分支也允许所有候选均不 append。七个直接 caller near-predecessor 均未发现支配性的 `filteredCount>0` guard 或必 append 不变量，因此不能排除 `auxiliaryRangeCount>0 && filteredCount==0`，首缺口仍为 `countAndNoWrap`。完整地址与边界见 [checkpoint v38](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r05-filtered-vector-checkpoint-v38.md)。

作者分离 TEST 在候选出现前封印 exact1 contract，最终独立接受 `0 positive / 1 UNKNOWN`；audit SHA-256 为 `37195EFEE870E371B8915264F3E1AB45906B54508BE08E985B92501B19E73765`。验收规模只保留一次 create-new、一次 verify-existing、四类决策 mutation 和一次 wrong-candidate-hash fail-before-write；没有重复 Cargo、全量 T0 build、旧 self-test 或游戏/调试器测试。

由于没有正向 promotion，本轮不创建空 frontier successor。v33 继续为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`；前十项仍为 `4 positive / 16 UNKNOWN`。manager-this、whole-image raw v2、formal v7、active/live、runtime mutation、05-05、T0-05 总门和 T0-06 全部保持关闭。

## 32. MT-R05 用户假设策略通过（2026-09-06）

用户基于该版本长期游玩未观察到相关崩溃，明确接受 MT-R05 的剩余风险，并要求先假设 `0x0033570B` 不会发生 32 位长度下溢。该行按 [checkpoint v39](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r05-user-assumption-policy-checkpoint-v39.md) 记录为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`，从后续人工逆向队列移除；除非用户重新打开或目标构建变化，不再继续 MT-R05 producer/lifetime 追踪。

本决策不覆盖 v38：严格静态 verdict 仍为 `UNKNOWN`，`countAndNoWrap=OPEN`，原 `0xFFFFFFFC` countermodel 保留。v33 strict frontier 仍为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`；仅人工逆向策略队列调整为 copy/move `81`、memset `17`、总计 `98`。waiver 不得冒充 `STATIC_CONFIRMED_NO_OVERLAP` 或进入 zero-frontier/whole-image/formal v7 静态正向合成。

独立 TEST 在候选前封印规则，并最终接受 exact1 assumption waiver，audit SHA-256 为 `92FC838C9063514508D79BCE65174D7A57ACDB4BBEA01BF6E28048506E295C5A`。候选与 TEST 均只执行一次 create-new、一次 verify-existing、三类必要 mutation 和相应 wrong-hash 写前拒绝；没有 Cargo、全量 build、历史宽回归、游戏或调试器测试。所有后续权限继续关闭。

## 33. MT-R03 用户假设策略通过（2026-09-06）

用户要求与 MT-R05 相同处理：基于该版本长期游玩未观察到类似崩溃，接受 MT-R03 `0x0001513A` 与 `0x000155A4` 的固定 5000-element backing 风险。[checkpoint v40](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r03-user-assumption-policy-checkpoint-v40.md) 将 exact2 记录为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`，并从后续人工逆向队列移除。

本决策不覆盖 v37：两行 strict verdict 均仍为 `UNKNOWN / backingExtent OPEN`；`5001*84=420084 > 420000` 与 `5001*108=540108 > 540000` countermodel 保留。v33 strict frontier 继续为 remaining `99`；累计 R03/R05 waiver exact3 后，人工逆向策略队列为 copy/move `79`、memset `17`、总计 `96`。waiver 不得进入 zero-frontier、whole-image 或 formal v7 的静态正向合成。

候选前封印与候选后独立 TEST 均通过，最终 audit SHA-256 为 `758F92C60890DE425E19932231019A7270148CD7FA01D29DD2BB92C472EA63D2`。测试继续只保留 create-new、verify-existing、三类必要 mutation 和 wrong-hash 写前拒绝；没有 Cargo、全量 build、历史宽回归、游戏或调试器测试。所有后续权限保持关闭。

## 34. MT-R02 animation descriptor 有界复核（2026-09-06）

本轮只继续 `MT-R02 {0x00008A62, 0x00008B2A}`。静态证据确认这两行迁移 Sprite-like owner 的 primary/secondary animation runtime state：incoming descriptor `+0x1C/+0x24` 是两组 int32 backing 元素数，`flag=0` 会按名字选择新 descriptor 并保留旧 per-element state。`Idle/Idle2` 与 `HeadLaugh*` 两条具体调用链都可进入该路径，但 caller 不比较或 clamp count。完整证据见 [checkpoint v41](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md)。

callee 在计数变化时按 `4*newCount` 分配、按 `4*oldCount` 复制；未找到支配性 `oldCount<=newCount` hard bound。没有发现具体资产中的真实 `old=2,new=1` producer，故不报告运行时越界；但 `allocation 4 bytes / copy 8 bytes` 的最小符号反例也未被排除。两行继续为 `UNKNOWN / backingExtent OPEN`，不创建空 positive frontier successor。v33 strict remaining 保持 `99`，人工逆向队列保持 `96`。

逆向与 TEST 作者分离。TEST 在 candidate 前封印 R02-only exact2 contract，最终 verdict 为 `PASS_SEALED_MT_R02_EXACT2_ALL_UNKNOWN`，audit SHA-256 `9677D086F43A65F4177BA21619DD189481AA6CC10E6108FA1696827260CAAB4B`。验证仅含 preparation 与 audit 各一次 create-new、恰好一次 verify-existing、4 个决策 mutation 和一次 wrong-hash 写前拒绝；未运行 Cargo、全量 build、历史回归、游戏或调试器。所有后续权限保持关闭。

## 35. 原生 Join 创建与发布用户规划假设（2026-09-06）

用户要求现阶段先假定 Authority 可以通过游戏原生 local-co-op Join 路径可靠地创建并发布第二玩家 B。[用户规划假设 v1](../../02-active-join/native-join-active-user-planning-assumption-v1.md) 因此允许后续文档以 fresh B、主 Player vector ordinal `1`、assignment、HUD/index 与 `Isaac.GetPlayer(1)` 同 generation 一致作为 `DESIGN_ONLY` 后置条件，并将只读规划重心转向 `CS-T0-05-05` 的 virtual-device 必要性决策。

该决策不是 evidence waiver 或工作包 PASS。plan v10、join progress v4、矩阵第 11 项、manager-this v33 strict frontier、MT-R02/R03/R05 的原始 verdict、Init/持久化、reader/ABA 与全部历史 hash 均保持不变；不声明零持久污染、Offline 主动 reader、恢复或 active runtime 已通过。

本轮只追加和更新 Markdown。没有启动游戏、打开进程、使用调试器、注入、Hook、runtime mutation、arm/token/claim 或创建实机 source。`activeExperimentAllowed`、`runtimeMutationAllowed`、`csT00505Unlocked`、`t005Unlocked` 仍为 false，T0-06 保持关闭。
