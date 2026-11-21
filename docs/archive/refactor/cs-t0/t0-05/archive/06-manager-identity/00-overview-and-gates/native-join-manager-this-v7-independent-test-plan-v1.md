> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 同代身份 v7 独立验收计划 v1

日期：2026-09-01  
状态：`DRAFT / INDEPENDENT TEST DESIGN`  
角色：独立测试；不承担逆向，不修改验收条件制造通过

## 1. 验收边界和冻结结论

本轮只验收 `frontierManagerThisIdentity`。边界从 caller 在 `0x002FA556` 读取 GameRoot 开始，经 `0x002FADA7` 调用 manager、`0x005BAC68` 保存入口 receiver、ordinary 分支在 `0x005BB408` 重读 GameRoot，止于 `0x005BB415` 调用 `0x005B9CD0`。v6 已确认的局部公式继续作为前提，不在 v7 中重新解释：

```text
receiver@0x005BB415 = DWORD[GameRoot slot RVA 0x00871678]@0x005BB408 + 0x1BAA8
```

manager-this 只在以下条件全部成立时允许提升为 `STATIC_CONFIRMED`：

```text
v6 local receiver formula
AND CALLER_PRECEDING_CALLEE_PRESERVATION
AND GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE
AND SAME_LIVE_GAMEROOT_GENERATION_ABA
AND LIFECYCLE_AND_EXCEPTION_PRESERVATION
AND production manager-this reader / cross-sample generation implementation exists
AND the production implementation is independently exercised by its harness
AND every uncertain or invalidating observation fails closed before publication
```

地址相等、常规 x86 ABI 经验、三个直接 writer、raw literal 数量、局部寄存器复制、合成模型 PASS、JSON 自报布尔或未来 fresh B 读回，均不能单独满足上述合取。

manager-this 子证明完成后，当前其余八个 manager-context frontier、publication identity、fresh B preservation 和四项完整同步证明仍为 `UNKNOWN`。因此 v7 正式 verifier 的预期最终状态必须是：

```text
status = BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES
managerReceiverFormulaStatus = STATIC_CONFIRMED
managerThisIdentityStatus = STATIC_CONFIRMED
managerContextStatus = UNKNOWN
overallAccepted = false
exitCode = 1
```

这不是 T0-05-04 总 PASS，不发布新 plan/progress，不解锁 05-05、T0-05 或 T0-06。

若 production reader / cross-sample generation 实现不存在，或四个 frontier 任一未闭合，预期结果保持：

```text
status = FAILED_MANAGER_THIS_IDENTITY_PREMISE
managerThisIdentityStatus = UNKNOWN
overallAccepted = false
exitCode = 1
```

## 2. 五层独立 oracle

五层证据不可互相替代：

1. **契约层**：严格 schema、状态耦合、完整 frontier 集合和权限全 false。
2. **合成模型层**：验证 counterexample 能到达判定器，只证明有限输入 trace 的状态机语义。
3. **提交证据层**：检查 producer sidecar 的函数、调用、writer、alias、异常、generation 和引用集合自洽。
4. **独立 native / 实现层**：测试方从正式 PE 重建指令与控制流，并直接执行生产 reader/generation harness；这是 manager-this 提升的必要层。
5. **公开 CLI / 发布层**：验证 sealed binding、create-new、退出码、无权限扩张、旧版本不可变和报告不误提升。

仅有 1–3 层 PASS 时必须保持 `UNKNOWN`。第 4 层不能只检查“源文件存在”或 producer 生成的 harness 报告；必须调用生产实现本身，且测试向量由独立测试侧构造。

## 3. v7 最小机器契约

建议 v7 create-new 契约把 manager-this 局部完成与全局 reader/manager-context 完成分开，至少表达：

- 四个 frontier 各有唯一 record、精确边界、完整 inventory、`status`、未决项和证据绑定；
- `managerThisReaderImplemented=true` 只在生产路径真实存在且 harness 直接调用该实现时成立；
- `managerThisReaderContractComplete=true` 不得提升全局 `readerContractComplete`；后者在其余 reader fields 未闭合时继续为 false；
- `managerThisGenerationAbaEstablished=true` 不得提升完整 manager-context 的 `generationAbaEstablished`；
- manager receiver 章节可将 `premisesGloballyEstablished` 与 `entryEstablishedBeforePublication` 置 true，但 manager-context 父章节仍为 `UNKNOWN`，并保留除 `frontierManagerThisIdentity` 外的八个 frontier；
- `runtimeObserved=false`、`processInspected=false` 与 reader **已实现**不矛盾：本轮可静态/离线验证生产实现，但不允许运行它去读游戏进程；
- publication、fresh B、whole synchronous proof 不变且保持 `UNKNOWN`；
- 14 项 admission contract 权限以及 receiver sidecar 的完整权限集合全部为 false。

不要复用含糊的单个 `runtimeReaderImplemented` 同时表示“代码存在”和“实机已执行”。建议分别发布：

```text
managerThisReaderImplemented = true
managerThisReaderHarnessVerified = true
runtimeReaderObserved = false
processInspectionAllowed = false
```

## 4. 四个 frontier 的正向验收

### 4.1 CALLER_PRECEDING_CALLEE_PRESERVATION

必须同时证明：

- 从 `0x002FA556` 到 `0x002FADA1/0x002FADA7` 的 39 个先行 callsite 完整、无遗漏、无重复；
- 7 个间接 callsite 的 target universe 已解析或被可审计地证明在该路径不可达，不能保留 unknown target；
- 每个可达 callee 的正常返回、tail transfer、提前退出、异常/unwind 和 callback/reentry effect summary 完整；
- caller 保存的 GameRoot 值、用于形成 manager receiver 的寄存器/栈值以及 GameRoot slot/lifecycle generation 在每个可返回 call 上保持；
- 同值写、写后恢复和“callee 按 ABI 理应保存 EDI”均不算保持证明；
- manager 内从入口到 ordinary 重读之间的可达 callback/callee 也必须纳入 lifecycle/reentry 证明，不能只清点 caller 的 39 个调用。

### 4.2 GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE

必须同时证明：

- whole-image 指令边界和 operand 语义重建覆盖 direct、partial、wide、RMW、atomic/exchange 及计算地址写；
- seed 地址、`lea`/立即数形成、寄存器与栈传播、作为参数传递、memcpy/memmove/helper 写入均有闭合的 alias provenance；
- 三个 v6 direct 4-byte writer 仍在精确集合中，但“只有三个 direct writer”不能等价为 alias closure；
- 每个可达 writer 都与 constructor/destructor/replace/clear/lifecycle 和 generation invalidation 关联；
- unaligned/cross-boundary raw literal 只作诊断，不作为 address formation 或无 alias 的证明；
- 任一 unresolved computed alias 或外部 writer 使 frontier 保持 `UNKNOWN`。

### 4.3 SAME_LIVE_GAMEROOT_GENERATION_ABA

production reader/generation 实现必须持有 adapter-owned identity，最少绑定：

```text
process creation identity
GameRoot allocation identity
derived PlayerManager = GameRoot + 0x1BAA8
monotonic non-wrapping generation
sample point / event order
```

采样点至少覆盖 caller sample、manager entry/save、ordinary slot reload 和 spawn eligibility。任何 slot writer、owner replacement、lifecycle、read failure、double-read drift、process reuse、generation overflow 或未排除 ABA 都必须在 publication 前进入 `PERMANENT_DISARM`；uncertain stop 必须进入 `UNSAFE_RETAIN_MODULE`。同地址不同 generation、A→B→A、同值 writer 和写后恢复均必须失败。

### 4.4 LIFECYCLE_AND_EXCEPTION_PRESERVATION

必须同时证明：

- GameRoot create/replace/clear/destroy 及相关 manager 生命周期路径完整；
- caller 与 manager 路径的 normal return、early return、tail transfer、SEH/unwind、trap/no-return、exception resume 均有 disposition；
- callback reentry/nested manager entry 若可达，必须被 generation 隔离或在 publication 前永久失能；
- exception 后不能带旧 sample 恢复到 ordinary spawn；
- stop/drain/restore/terminal 中的 reader failure 或未决 wrapper 状态不能被报告为安全 Stopped；
- 任何恢复到相同地址的路径仍按 generation 变化处理。

## 5. 独立对抗矩阵

| 组 | 必测反例 | 预期 |
| --- | --- | --- |
| Contract promotion | 缺 frontier、重复/虚构 frontier、单个 flag 提升、unresolved 非空却 STATIC、parent manager-context/publication/fresh-B/sync 同步提升 | reject，无报告或 verifier local failure |
| Wrong elevation | 只给局部公式、只给 39/7 数量、只给 direct writer=3、只给相同地址、只给 synthetic PASS、只给 producer report | `managerThisIdentityStatus=UNKNOWN` |
| Preceding callee | callsite 缺失/重排/重复、direct target drift、未知 indirect target、callee clobber owner register、重叠 slot/object write、same-value write、write-restore、未记录 tail/early exit | reject 或 `FAILED_MANAGER_THIS_IDENTITY_PREMISE` |
| Callback/reentry | callee 触发 nested manager、callback 中 replace/clear、reentry 后地址恢复、callback target universe 不全 | generation invalidated，禁止 spawn eligibility |
| Slot alias | `lea`/立即数地址逃逸、base+index computed alias、stack spill/parameter transfer、byte/word partial overlap、8/16-byte wide overlap、xchg/cmpxchg/RMW、memcpy/memmove helper | 必须检出 writer/alias；漏检即测试失败 |
| Raw scan illusion | unaligned literal、跨指令 literal、data-section literal、raw candidate 数相同但 operand 语义改变 | 不得提升 alias closure |
| Generation/ABA | stable A/g pass、同地址 generation+1、A→B→A、same-value writer、write-restore、process identity reuse、generation wrap、sample 缺失/重复/乱序、double-read drift | 仅 stable trace 可 eligible；其余永久失能 |
| Lifecycle | clear/destroy/replace、owner retired、new process、teardown、stop 中 invalidation | 永不以旧 generation 恢复 |
| Exception | throw+unwind、exception resume after write、nonlocal return、trap 恢复、unknown no-return import | 未完整 disposition 时保持 UNKNOWN；旧 sample 不可继续 |
| Reader absent | 只有 `admission_v7` Python checker/JSON，没有生产 reader；生产函数存在但不可达；harness 只读 fixture 不调用生产函数 | `FAILED_MANAGER_THIS_IDENTITY_PREMISE`, exit 1 |
| Reader failure | 任一 read 失败、超预算、owner/vector drift、invalid alignment/width、boolean/int 混淆、generation overflow | `PERMANENT_DISARM`；uncertain stop 为 `UNSAFE_RETAIN_MODULE` |
| Permission | 任一 process read/enumeration、hook transaction、mapping、injection、arm、claim、input/world/runtime mutation、transport、active/live、05-05/T0-05/T0-06 flag 为 true | reject |
| Public CLI | duplicate option、raw JSON duplicate key/nonfinite、repository/output alias、junction、ADS/device name、existing output、bound input mutation | exit 2/非零，无 stdout report，不创建/覆盖输出 |
| Version isolation | v6 schema/identity/phase/checker 被复用为 v7 success；v6 formal report/input bindings/旧失败记录漂移 | reject；v6 全量哈希不变 |

### 5.1 合成 generation 模型的最小操作集

独立用例应至少能表达：

```text
SAMPLE_CALLER, CALL_ENTER, CALL_RETURN, READ_SLOT, WRITE_SLOT,
ALIAS_WRITE, LIFECYCLE, CALLBACK_ENTER, CALLBACK_RETURN,
EXCEPTION_THROW, EXCEPTION_RESUME, EXCEPTION_UNWIND,
PROCESS_REUSE, ADVANCE_GENERATION, SAMPLE_MANAGER_ENTRY,
SAMPLE_ORDINARY_RELOAD, SPAWN_ELIGIBILITY
```

模型必须强制唯一终止事件和严格阶段顺序。event after spawn、重复 spawn、缺 sample、布尔冒充整数、generation 回绕、未知操作及非 JSON identity 均拒绝。模型返回中必须固定 `nativeSemanticProof=false`、`runtimeObserved=false`，不能用模型结果直接提升契约。

### 5.2 生产 reader harness 的必要向量

生产实现的独立 harness 至少覆盖：

- stable A / same generation 的唯一正向 trace；
- caller sample 与 ordinary reload 不同地址；
- A→B→A；
- 同值 writer 和写后恢复；
- writer 发生于 direct callee、indirect callee、callback reentry 和 exception resume；
- GameRoot 不变但 process creation identity 改变；
- read failure、double-read drift、超 read/byte budget；
- generation 最大值后再 invalidation；
- invalidation 后新输入、重复观察或相同地址不能自动重新武装；
- stop 不确定时保留模块并报告 Unsafe。

## 6. v6 测试/发布流程继承审计

### 6.1 可以继承

- `DiskPe`、`Objdump`、SHA-256、严格 JSON、RVA mapping、诊断脱敏等通用只读工具；
- `bound_file`、禁止 reparse/junction、Win32 alias/ADS/device-name 防护和 create-new output；
- `protect_v6` 的思想：v7 建立 create-new frozen-v6 baseline，绑定 v6 formal verification 的 523 个 input binding 与历史失败；
- public blackbox 的完整临时仓库、同字节 target copy、非法参数和 output 不覆盖检查；
- 独立从正式 PE 重建 instructions/calls/writes/exits，再与提交集合逐集合比较；
- explicit binding DAG 的已知 schema 审计、零 self-binding、零 cycle；
- v6 inspector 的 sealed contract SHA 与“合法但阻断时 exit 1”行为；
- old production seven-gap preflight 和 v1-v6 公开行为回归。

### 6.2 不能作为 v7 成功依据

- v6 `admission_contract.py` 中强制 manager-this UNKNOWN 的版本语义；
- v6 `receiver_identity_evidence.py` 的五函数、39/7 call、三个 direct writer 常量和四个 unresolved 强制值；
- v6 `receiver_identity_model.py` 的局部 LOAD/ADD/SPAWN 模型；
- `_aligned_absolute_slot_scan` 的 direct operand 结果；它没有闭合 computed alias；
- v6 `review_receiver_native` 的 `COMPLETED_WITH_UNRESOLVED` 结果；
- 旧版本 helper 的 version-fixed semantic decision；旧 helper 只能提供 generic disk I/O；
- hash/tamper 拒绝本身；它证明 seal，不证明 callee/alias/lifecycle/generation 语义；
- producer 自己生成的 counts、analysis 或 harness report。

### 6.3 v7 发布流程要求

- 新建 `admission_v7/`、`tests/join_admission_v7/`、contract、inspector、verifier、baseline 和 formal report；不改 v1-v6；
- v7 semantic modules 必须是 create-new，不能调用 v6 checker 得到语义 PASS；
- frozen-v6 baseline 使用精确路径列表，不照抄 v6 verifier 的 broad `out/cs-t0/native-join-*` glob；
- v8→下一 Init merge只允许白名单字段与绑定 append，全部旧函数行逐字节相等；
- verifier 报告区分 structural PASS、native manager-this semantic PASS、production harness PASS 和最终 blocked decision；
- 默认 formal output 一次性 create-new；开发失败、空文件、语法错误和被拒报告全部保留，不覆盖改判。

## 7. 建议执行命令和 expected outcome

以下命令只读正式磁盘目标，不枚举/打开进程，不启动或附加游戏。输出名必须是不存在的新文件。

```powershell
$game = 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe'
$objdump = (Get-Command objdump -ErrorAction Stop).Source

pwsh -NoProfile -File .\cs-t0\inspect-coop-native-join-admission-v7.ps1 `
  -GameExe $game -RepositoryRoot . `
  -Output .\out\cs-t0\manager-this-v7-inspection-create-new.json
```

完成 manager-this 后 inspector 的 expected outcome：`exit 1`、`BLOCKED_BY_EVIDENCE`、receiver formula 与 manager-this 为 `STATIC_CONFIRMED`、manager context/publication/fresh B/whole sync 为 `UNKNOWN`；没有 process/mapping/injection/arm/claim/active/live 权限。reader 未实现版本则 manager-this 必须为 `UNKNOWN`。

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-native-join-admission-v7.ps1 `
  -GameExe $game -RepositoryRoot . -Objdump $objdump `
  -Output .\out\cs-t0\manager-this-v7-verification-create-new.json
```

完成 manager-this 后 verifier 的 expected outcome：`exit 1`、`BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES`、`localChecksPassed=true`、`allRequiredChecksExecuted=true`、`overallAccepted=false`。reader 未实现、任一 frontier 未闭合或 native/harness 对抗失败时，expected outcome 是 `FAILED_MANAGER_THIS_IDENTITY_PREMISE` 或 `FAILED_LOCAL_ACCEPTANCE_CHECKS`，仍为非零。

若 v7 修改 Rust/Native 生产 reader，再执行继承回归：

```powershell
cargo test --manifest-path .\cs-t0\Cargo.toml --locked
pwsh -NoProfile -File .\cs-t0\build.ps1 -Configuration Release
```

expected outcome：测试与 Release/Native harness 全 PASS；不得把 build PASS 当作 manager-this semantic PASS。完整 verifier 仍负责最终子证明判定。

## 8. 独立测试交接标准

实现方交付 v7 后，测试方从全新 create-new output 开始，先验证 frozen-v6 baseline 和权限，再执行 contract/model/evidence/native/production-harness/public CLI 五层矩阵。任一失败保留 artifact 和最小复现，不修改 checker 或 expected status 解释旧失败。

最终交接只允许两种结论：

- `manager-this STATIC_CONFIRMED / overall BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES / exit 1`；或
- `manager-this UNKNOWN / FAILED_MANAGER_THIS_IDENTITY_PREMISE (或 local acceptance failure) / nonzero`。

不得报告 T0-05-04 总 PASS、active/live ready、reader runtime observed 或任何下游解锁。
