> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Isaac C/S T0-05 执行状态

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

更新时间：2026-09-09
适用目标：Win32/x86 `isaac-ng.exe`  
总体状态：**IN_PROGRESS**  
当前完成项：**CS-T0-05-03 限域适用性 gate 正式 PASS：五份适用 PASS source＋一项 N/A；旧六源结果 5/6 保留**
当前工作包：**CS-T0-05-05 已完成 NATIVE_ALIAS_CLOSURE 固定点清零 Batch 1：v6 四组压缩验收各一次 PASS，IAT/callback/service 三个高收益集合已闭合，receiver frontier 从 38,985 降至 20,744、pair frontier 从 37 降至 34；alias verdict 仍 UNKNOWN、dynamicAliasUniverseClosed=false、promotionCount=0；factory/rollback 与 reference-count/final-release caller/thread 尚待独立执行，formalDecision 为空且状态为 BLOCKED_BY_EVIDENCE；05-06、runtime、active 与全部 mutation 权限保持关闭**

本文是 T0-05 的可更新执行状态入口。阶段要求和停止条件仍以 [T0-05 研究计划](isaac-cs-t0-05-research-plan.md) 为准；本文不覆盖研究计划、历史 artifact、原始 analysis、既有 ledger/catalog 或 gate。

05-04 最新实施见[准入记录](../03-admission/native-join-admission-implementation-v1.md)及[独立测试结果](../03-admission/native-join-admission-test-results-v1.md)，机器入口为[plan v10](../../../../../../cs-t0/coop-plan-v10.json)和[join progress v4](../../../../../../cs-t0/coop-native-join-progress-v4.json)。新49锚点、1,524条指令、283条直接边经独立解码；23个契约用例、104个模型用例、21个解析/引用用例与5组公开黑盒/57项断言通过。180项字段描述有静态来源，11项仍UNKNOWN；完整Init/P0保持、自然更新窗口、reader布局及原生ABA覆盖尚未闭合。generation归adapter所有及永久失能/Unsafe规则已规定，不冒充生产reader或运行实现。旧164文件快照、149基线和三份生产文件未变，旧preflight仍报告七项缺口、退出1；无实机操作，矩阵第11项仍NOT_STARTED。前置[实施v1](../02-active-join/native-join-active-implementation-v1.md)和[条件审查v1](../05-persistence/native-join-static-closure-implementation-v1.md)保持冻结；下文旧“当前”和“下一步”均按当次时间理解。

用户授权的 gate 迁移与完整自动门已完成，不包含实机。[新适用性 gate](../01-passive-lifecycle/native-exit-applicability-gate-v1.md) 的完整自动检查为 28/28 PASS，独立正式审计为 15/15 PASS。[decision v3](../../../../../../cs-t0/coop-passive-decision-v3.json) 只开放 05-04 工作包资格；[plan v6](../../../../../../cs-t0/coop-plan-v6.json) 已关闭 passive capture 资格。live-07 的 BLOCKED、旧六源 gate 及 progress v7 的 5/6 均不变。下文历史记录中的“下一步”和进度均指各次记录当时。

## 1. 当前结论

`CS-T0-05-00 Evidence Bootstrap` 的全部 v1 文件保持逐字节不变。`CS-T0-05-01 Static Local Co-op Graph` 仅使用磁盘 PE、IDA MCP 只读反汇编和纯内存测试，已经闭合静态退出门：

- owner 正向路径固定为 `0x5AB6F2→0x554CD0→0x5551BD→0x2FA540→0x2FADA7→0x5BAC40`；`0x2F8055→0x554CD0` 已分类为 Victory Lap run-restart lifecycle，禁止命名为 Authority tick；
- join/create、assignment、exit、join/exit HUD rebuild、HUD index helper 与 Lua `GetPlayer` vector ordinal 路径均已进入分区图；
- exit `0x5BA000` 固定为 `__thiscall`、两个栈参数和 `ret 8`，第二个栈参数为未读取的 ABI padding；主 Player vector erase/压缩、存活 Player ordinal 重编号、slot reset 与 HUD rebuild 已闭合；
- Lua publication/retirement 与主 Player vector 的序数关系已闭合，但不宣称 exit 中间变更窗口具备原子不可见性；
- `0x5BAC40` 的 online/`NetInputDevice` vector 非空分支跳过整个 local 主体，正向 route 不跨入 `ORIGINAL_ONLINE_FORBIDDEN`；
- 新增 ordinal `45..52` 八个 REL32 passive 草案槽，均具唯一 masked feature、expected bytes、REL32 target、ABI、owner、线程约束和前后置；possessor 两槽只作停止哨兵；合并后总槽数为 53，恢复顺序严格反向；
- `0x554F84→0x55B310` 和 virtual device 按设计保持 `UNKNOWN`，未伪填 RVA/ABI，也未作为正向路径或 observer slot；
- inspection 的 `missingEvidence=[]`，权限仍保持全关。

Static decision 为 `PASS`，SHA-256：

`2CA3A9956FEAED6E08BDAA2F89F2A244DE4CF6B6CB0E2AFA0EFAEE7A37C9E459`

该结论只解锁 `CS-T0-05-02`。它不表示 T0-05 整体完成，不授权 passive Hook 或 active 实机运行，也不解锁 T0-06。

`CS-T0-05-02` 的独立 C0 实现、三份 live source、三模式 comparison、composite no-mutation、继承 tick gate 和最终机器产物现已全部 `PASS`。逆向与测试由两个独立子智能体执行：逆向侧只产出静态布局证据，测试侧独立覆盖真实 CLI/schema、重放、篡改、隐私、路径归一化和 create-new 门禁。OriginalOnline 首份失败 source 与首次失败 decision 都按 create-new 规则永久保留，不覆盖、不改判。

## 2. 工作包进度

| 工作包 | 状态 | 当前出口 |
| --- | --- | --- |
| `CS-T0-05-00` Evidence Bootstrap | `PASS` | predecessor 闭合、v1 产物族建立、`CS-T0-05-01` 解锁 |
| `CS-T0-05-01` Static Local Co-op Graph | `PASS` | 静态正向图、online 隔离、八槽草案与 verifier 闭合；`CS-T0-05-02` 解锁 |
| `CS-T0-05-02` Three-mode C0 Comparison | `PASS` | 三份 analysis、comparison、composite no-mutation 与 inherited tick gate 全部通过；读取权限已关闭 |
| `CS-T0-05-03` Passive Lifecycle / Mapping | `PASS_SCOPED_APPLICABILITY` | 五个适用 source PASS＋一项限域 N/A；旧六源 gate 和 live-07 BLOCKED 保留 |
| `CS-T0-05-04` Native Join Active Proof | `BLOCKED_BY_EVIDENCE / STATIC_CONDITION_REVIEW` | 新静态审查通过；fresh B状态传播及自然更新保存窗口仍未闭合，runtime/实机未开始 |
| `CS-T0-05-05` Virtual Device Decision | `BLOCKED_BY_EVIDENCE / NATIVE_ALIAS_BATCH1_UNKNOWN` | v6 已闭合 IAT/callback/service 三组并将 receiver/pair frontier 收缩至 20,744/34；两个 fixed point 仍非零，factory/rollback 与 final-release/thread 未开始，native/formal 仍 UNKNOWN，05-06 未解锁 |
| `CS-T0-05-06` Dual-endpoint Isolation | `NOT_STARTED` | 等待 virtual decision 条件门 |
| `CS-T0-05-07` Replay / Formal Gate | `NOT_STARTED` | T0-05 formal gate 尚未建立 |

当前权限以 [plan v8](../../../../../../cs-t0/coop-plan-v8.json) 为准；它原样继承plan v7/v6的14个权限字段，仍只有一项为 true：

- `csT00504Unlocked=true`，仅表示工作包资格；
- `passiveHookTransactionAllowed=false`、`passiveObservationAllowed=false`；
- active/input/world/runtime mutation、override、transport、scheduler、apply、ModeLatch、OriginalOnline 正向能力全部 false；
- `t005Unlocked=false`，T0-06 未解锁；
- `liveAuthorizationCreated=false`，历史授权不可复用。

CS-T0-05-01/02/03 的历史资格与完成记录继续保留。`coop-plan-v5.json`、`coop-plan-v5r1.json` 和 verification v5r6 是未改写的历史 pre-live 入口，不是当前 capture 授权；plan v6 已将 passive 事务资格关闭，也未授权 active 实机。

## 3. CS-T0-05-01 机器产物

| 产物 | Identity | SHA-256 |
| --- | --- | --- |
| [`cs-t0/coop-static-evidence-v1.json`](../../../../../../cs-t0/coop-static-evidence-v1.json) | `IsaacCsT0-CoopStaticEvidence-v1` | `501B39DEB198E7F1424328069534438ADC73BA665D1E6988AFA4AD7939DB081B` |
| [`cs-t0/coop-static-graph-v1.json`](../../../../../../cs-t0/coop-static-graph-v1.json) | `IsaacCsT0-CoopStaticGraph-v1` | `F87DA1DF383B7B1AE3724A4DB4EAAE791B9F42B68AF9210AD1727F71D9458934` |
| [`cs-t0/coop-static-inspection-v1.json`](../../../../../../cs-t0/coop-static-inspection-v1.json) | `IsaacCsT0-CoopStaticInspection-v1` | `7E76A04E39F2C6AADD16ADC2EEBFB96F3438FA3D06B06E28716B5ACB97B3BBAB` |
| [`cs-t0/coop-static-decision-v1.json`](../../../../../../cs-t0/coop-static-decision-v1.json) | `IsaacCsT0-CoopStaticDecision-v1` | `2CA3A9956FEAED6E08BDAA2F89F2A244DE4CF6B6CB0E2AFA0EFAEE7A37C9E459` |
| [`cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json`](../../../../../../cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json) | `IsaacCsT0-CoopLedger-v2` | `EA195429231F9CA8575FC6901E792496D6C26AC42984928B1C09504E97B3FCAC` |
| [`cs-t0/coop-catalog-v2.json`](../../../../../../cs-t0/coop-catalog-v2.json) | `IsaacCsT0-CoopCatalog-v2` | `1059711778DAD09870D37118571EE7D5E67761EC28FFAB84C7CF5AC5998A0224` |
| [`cs-t0/coop-plan-v2.json`](../../../../../../cs-t0/coop-plan-v2.json) | `IsaacCsT0-CoopPlan-v2` | `40DB0C0F42AC1DD2B86042633D44357F015459DBDFF276A237946F978706FB34` |
| [`cs-t0/coop-hypothesis-index-v2.json`](../../../../../../cs-t0/coop-hypothesis-index-v2.json) | `IsaacCsT0-CoopHypothesisIndex-v2` | `608D2619F1DA63242D9C60FE0E229244F698C0CC8ADDA8C6908AF70D8F92275C` |
| [`cs-t0/coop-verification-v2.json`](../../../../../../cs-t0/coop-verification-v2.json) | `IsaacCsT0-CoopVerification-v2` | `77DBB448152E2C548E5748DD1ED75D26BA13989F507586AD8B1D2C4A85324828` |
| [`cs-t0/verify-coop-static.ps1`](../../../../../../cs-t0/verify-coop-static.ps1) | `CS-T0-05-01-StaticVerifier` | `3E08D7F2B3CE6536C1AEBD61EE8A6FF0D4ECB3873D28597A513A3C064153FD66` |

全部 v2 产物都记录 `supersedes` 与 `supersedesSha256`。Decision 单向绑定 bootstrap、approved-unpacked equivalence、evidence、graph、inspection、v2 产物和 verifier；被绑定文件不反向绑定 decision，避免循环哈希。所有持久化路径均为仓库相对路径，未保存绝对 EXE/IDB 路径、VA、PID、进程指针、platform handle 或原始身份。

### 冻结的 CS-T0-05-00 基线

| 产物 | Identity | SHA-256 |
| --- | --- | --- |
| [`cs-t0/ledger/isaac-cs-t0-coop-ledger.json`](../../../../../../cs-t0/ledger/isaac-cs-t0-coop-ledger.json) | `IsaacCsT0-CoopLedger-v1` | `DC2E8F5B81929B84AB281779CF3F056F96BAE5446B7F3ED2173200A52421227C` |
| [`cs-t0/coop-predecessor-binding-v1.json`](../../../../../../cs-t0/coop-predecessor-binding-v1.json) | `IsaacCsT0-CoopPredecessorBinding-v1` | `5D29826CD9E92E84D4C97C14E511066EF1FD8576B2DA5CC4639861BD9812DB90` |
| [`cs-t0/coop-catalog-v1.json`](../../../../../../cs-t0/coop-catalog-v1.json) | `IsaacCsT0-CoopCatalog-v1` | `206C715996B262A7A185E92652D0DC3108DC4E97553B5B7E902C620CD51E109A` |
| [`cs-t0/coop-plan-v1.json`](../../../../../../cs-t0/coop-plan-v1.json) | `IsaacCsT0-CoopPlan-v1` | `112B551F666E101C1F644B48F7B6B7BCB18F2DAD74A8464574E9A04855CE59A5` |
| [`cs-t0/coop-verification-v1.json`](../../../../../../cs-t0/coop-verification-v1.json) | `IsaacCsT0-CoopVerification-v1` | `A894867D5B5388C9540B024083F659D1EDBC1CB461D362301519FB18E48ABB48` |
| [`cs-t0/coop-matrix-v1.template.json`](../../../../../../cs-t0/coop-matrix-v1.template.json) | `IsaacCsT0-CoopMatrix-v1` | `C3BB9A97188708E9BCFE9E25E8D9DAFF37EBBA5656CEA52CEFE04F9EEE1D1100` |
| [`cs-t0/coop-hypothesis-index-v1.json`](../../../../../../cs-t0/coop-hypothesis-index-v1.json) | `IsaacCsT0-CoopHypothesisIndex-v1` | `E892F698BFE82F00516B40C6F1DC7354ABA68C7D98B78F37126E2CEB6CB5FC96` |
| [`cs-t0/coop-bootstrap-decision-v1.json`](../../../../../../cs-t0/coop-bootstrap-decision-v1.json) | `IsaacCsT0-CoopBootstrapDecision-v1` | `7EB5004E368A478EBB3CC83149BB05E70F7876C4384A511557EEC9188D1D7598` |
| [`cs-t0/verify-coop-bootstrap.ps1`](../../../../../../cs-t0/verify-coop-bootstrap.ps1) | `CS-T0-05-00-BootstrapVerifier` | `FA9739C90A86BB504C2170F9A4946A8CFFA93B588D56EDB93DE544CE1F119F8A` |

上述 v1 文件仍为冻结基线；`coop-matrix-v1.template.json` 保持未填充。所有后续修订必须 create-new；若 target、equivalence、expected bytes、call target、vtable、COL、ABI、slot 或前置 gate 发生变化，必须创建新 identity，并记录 `supersedes` 与 prior SHA-256。

## 4. 验证结果

只读静态 verifier：

~~~powershell
.\cs-t0\verify-coop-static.ps1 -GameExe <isaac-ng.exe>
~~~

最新结果：

~~~text
WorkPackage            : CS-T0-05-01
Status                 : PASS
StaticRoutes           : 2
ObserverSlots          : 8
CombinedSlots          : 53
MissingEvidence        : 0
CsT00502Unlocked       : True
ActiveInputAllowed     : False
RuntimeMutationAllowed : False
NextWorkPackage        : CS-T0-05-02
~~~

同时通过：bootstrap verifier、`cargo test --all-targets --locked`、`cargo fmt --check`、`cargo clippy --all-targets --locked -- -D warnings`、Release build 和既有 `verify-active.ps1` 离线链。既有 active verifier 的最终 `BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE` 是冻结的 T0-04/T0-05-00 native runtime 门，不是本轮静态检查失败。

`coop-inspect` 对 approved-unpacked equivalence、feature 唯一性、bytes/opcode/REL32 target、graph 完整性、online 分区隔离、53 槽布局、逆序恢复、权限和隐私规则做确定性只读验证；指定 `--output` 时执行 create-new，已有文件立即拒绝。测试还覆盖单字节/target 篡改、零/多 feature、`UNKNOWN` 伪填、缺失 exit/HUD/Lua、错误 Authority 命名、online route 穿越、权限提升、绝对路径/指针泄露和非空 `missingEvidence` 伪报 `PASS`。

## 5. CS-T0-05-02 C0 证据与最终门

| 产物 | Identity | SHA-256 |
| --- | --- | --- |
| [`cs-t0/coop-c0-layout-evidence-v1.json`](../../../../../../cs-t0/coop-c0-layout-evidence-v1.json) | `IsaacCsT0-CoopC0LayoutEvidence-v1` | `921EE4C4AA1FDCE86E21F1CFF1B38D5802E13E715242EC6006FA79B5F6316059` |
| [`cs-t0/coop-plan-v3.json`](../../../../../../cs-t0/coop-plan-v3.json) | `IsaacCsT0-CoopPlan-v3` | `12BE7D22A1103A57923A1B6894B3D6C3433A165E431B06C44B28CB1DF21C6850` |
| [`cs-t0/coop-verification-v3.json`](../../../../../../cs-t0/coop-verification-v3.json) | `IsaacCsT0-CoopVerification-v3` | `BE6BBC61D37E351C08067B5897F3EE541F862D83F82857C0A6077325F7DD42B6` |
| [`cs-t0/verify-coop-c0.ps1`](../../../../../../cs-t0/verify-coop-c0.ps1) | `CS-T0-05-02-C0Verifier` | `DBFDA6EF368CC06E4E7F44F7797EBB3AD123C6E7A7D9959EFECEF050B634FA57` |
| [`artifacts/cs-t0/20260827T153600Z-t0-05-02-three-mode-c0-comparison-02-final.json`](../../../../../../artifacts/cs-t0/20260827T153600Z-t0-05-02-three-mode-c0-comparison-02-final.json) | `IsaacCsT0-CoopC0Comparison-v1` | `82A83246A5BB850313BAD6974EBCB7C6746AD22722FCD8DD8A04D84F38F028D8` |
| [`artifacts/cs-t0/20260827T153600Z-t0-05-02-c0-decision-03-final.json`](../../../../../../artifacts/cs-t0/20260827T153600Z-t0-05-02-c0-decision-03-final.json) | `IsaacCsT0-CoopC0Decision-v1` | `46A5F34D5A8FBF1D7E239F1ECB2B572BDD3814B7A4A6AA3216EED57A159DE07D` |
| [`cs-t0/coop-matrix-v2.json`](../../../../../../cs-t0/coop-matrix-v2.json) | `IsaacCsT0-CoopMatrix-v2` | `BC7BF6A4B4AB1A3733545C1A1530E7E6D046079817EE6AA55B39E304D0EA480D` |
| [`cs-t0/ledger/isaac-cs-t0-coop-ledger-v3.json`](../../../../../../cs-t0/ledger/isaac-cs-t0-coop-ledger-v3.json) | `IsaacCsT0-CoopLedger-v3` | `A1F4F464260BF7FE93213840E25411C27495C72CE3BC4AFA5A2436AC3FE10E13` |
| [`cs-t0/coop-verification-v4.json`](../../../../../../cs-t0/coop-verification-v4.json) | `IsaacCsT0-CoopVerification-v4` | `FF667CD7F537EC335487D577AFB3989D6B0D34FF5E806EFE23AF44A22E5C31D4` |
| [`cs-t0/coop-plan-v4.json`](../../../../../../cs-t0/coop-plan-v4.json) | `IsaacCsT0-CoopPlan-v4` | `553E6676F6DECCA0BC25E1BF7806AEB2AEE8A52AB37F85F08C1DD86E8C5932B2` |

`IsaacCsT0-CoopVerification-v3` 为 `PASS_AUTOMATED_PREFLIGHT`，但明确保持 `runtimeContractComplete=false`、`directPostCommitObserved=false` 和 `authorityCommitStatus=PENDING_INHERITED_GATE`。`coop-plan-v3.json` 只为三个 C0 场景开放 `0x00101410` observer；active、passive、mutation 与 T0-05 总解锁继续为 false。

新增接口为 `coop-smoke --probe control-no-native`、`coop-analyze`、`coop-compare` 与 `coop-c0-gate`。采集默认 5 秒/50 ms、每 attempt 正反双读且最多四次局部重试；不一致/读失败只保存无对象图的 dropped record。持久化 source 不包含 PID、VA、module base、pointer、设备/assignment key、raw InitSeed、平台或 peer identity；IMAGE/IAT 目标以单次 source 的 opaque proof 做三方相等验证。

初始自动验证通过 276 个 Rust 单元测试、1 个跨语言测试、8 个 C0 contract 测试、fmt/check/clippy、Release build、Debug/Release Native 39-case harness、Debug/Release Active Native 262-case harness、static/bootstrap/active 回归与 16 个 `isaac-test-mcp` 测试。OriginalOnline 首份 source 暴露 NetInput 分类缺口后，独立测试子智能体补充正反例；最终完整门为 278 个 Rust 单元测试、1 个跨语言测试、9 个 C0 contract、fmt/check/clippy、Release build、Debug/Release 两套 39-case/262-case Native harness、static 与 T0-04 active 回归全部 `PASS`。冻结的 T0-04 active verifier 仍按其原契约报告 `BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE`；这不是本轮 C0 自动门失败，也没有被改写为新的 runtime confirmation。

### Offline single live source

Source：[`20260827T134838Z-t0-05-02-offline-single-c0-02`](../../../../../../artifacts/cs-t0/20260827T134838Z-t0-05-02-offline-single-c0-02)

- capture：90/90 coherent，`unstableDrops=0`，`readErrorDrops=0`；
- scenario：online negative、1 Player、ordinal 0、唯一 Keyboard assignment、HUD/Lua order 一致；
- safety：53 槽 expected/before/after 等价，`noMutationPassed=true`，递归隐私扫描 `PASS`；
- `analysis.json` SHA-256：`F4A455F2E7AAEE1643000A5CDA198AA65032A9551A2F4FD2A550D660E995C947`。

首次确认后的 preflight 曾因仍运行的 x32dbg 立即拒绝；它发生在 source 创建和对象读取之前，没有自动重试。用户关闭 debugger 并再次单独确认后，使用新 case ID 完成本 source。

### Local co-op dual live source

Source：[`20260827T140454Z-t0-05-02-local-coop-dual-c0-01`](../../../../../../artifacts/cs-t0/20260827T140454Z-t0-05-02-local-coop-dual-c0-01)

- capture：90/90 coherent，`unstableDrops=0`，`readErrorDrops=0`；
- scenario：online negative、2 个不同 Player、ordinal 0/1、Keyboard/Gamepad assignment 双射、HUD/Lua order 0/1 一致；
- authority：`PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC`，`directPostCommitObserved=false`，commit 仍等待 inherited gate；
- safety：53 槽 expected/before/after 等价，`noMutationPassed=true`，递归隐私扫描 `PASS`；
- `analysis.json` SHA-256：`3D82FC022D3201044300D9A58B5CD0327AEC0985CC48927212DC6019F3BE2BB5`。

### OriginalOnline dual failed source 01

Source：[`20260827T151840Z-t0-05-02-original-online-dual-disarmed-01`](../../../../../../artifacts/cs-t0/20260827T151840Z-t0-05-02-original-online-dual-disarmed-01)

- verdict：永久保持 `BLOCKED_BY_EVIDENCE`；90 个 attempt 全部为 `DROPPED_UNSTABLE/UNKNOWN_DEVICE_VTABLE`，coherent graph 为 0；
- safety：dropped record 均不携带 partial graph，53 槽与 no-mutation `PASS`，递归隐私扫描 `PASS`；
- root cause：InputManager vector 中的 OriginalOnline NetInputDevice 被旧采样器误要求匹配 Keyboard/Gamepad vtable；
- fix：仅当未知设备同时属于冻结的 OriginalOnline online vector 时分类为隔离的 `NET_INPUT`，不读取 assignment key，且强制不得参与 local mapping；同时验证 `NetInput+0x374` identity object 与 `+0x08` peer identity 的非零、唯一、精确 cardinality 和双读稳定，raw identity 不落盘；
- `analysis.json` SHA-256：`855A387542FA4F1ACCE4B015458CA16C806C6A1E78A5DF04F63A179AA9D7FF73`；最终 Release Controller SHA-256：`9B2013E1C7BEB7C7C0539067BB4266E1911CD0F3C2AC3F9CC77D5F4F25784856`。

### OriginalOnline dual live source 02

Source：[`20260827T153600Z-t0-05-02-original-online-dual-disarmed-02`](../../../../../../artifacts/cs-t0/20260827T153600Z-t0-05-02-original-online-dual-disarmed-02)

- capture：90/90 coherent，`unstableDrops=0`，`readErrorDrops=0`；
- scenario：恰有 2 个 Player/roster，manager、backend、NetInput vector 与 online frame 稳定为正；
- isolation：NetInput 只满足 OriginalOnline 对照，不读取 assignment key，也不参与 local-positive mapping；peer identity 仅在进程内验证非零、唯一、精确 cardinality 和双读稳定，原值不落盘；
- safety：53 槽 expected/before/after 等价，`noMutationPassed=true`，递归隐私扫描 `PASS`；
- `analysis.json` SHA-256：`254F0DF64D4EBC40B7DB7E2FD9C0BBE1D5AFCA4F32BCAF84FFEE961F4AEF5644`。

### 三模式 comparison 与最终 decision

- 三份 source 均为 90/90 coherent，三种拓扑各自稳定，comparison 为 `PASS`；
- composite no-mutation 覆盖 `control-no-native` 行，确认 53 槽三方逐字节相等且 `noFourthLiveCapture=true`；
- 本轮直接结论为 `PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC`；`directPostCommitObserved=false`；
- T0-03R loop gate 与 T0-04 final decision 及其全部根 source 均只读重放，最终 authority commit 为 `PASS_INHERITED_CONTRACT`，未写成新的 `RUNTIME_CONFIRMED`；
- 首次 decision 01 因实现错误调用 create-new analyzer 重放冻结 `analysis.json` 而保持 `BLOCKED_BY_EVIDENCE`；修复为只读 replay 后，decision 03 正式 `PASS`，历史文件不覆盖、不改判；
- `coop-verification-v4.json` 为 `PASS_C0_COMPLETE`，17 项检查全部通过；`coop-plan-v4.json` 已把 observer mask 关闭为 `0x00000000`、`processReadAllowed=false`、`furtherC0CaptureAllowed=false`；
- 只设置 `csT00503Unlocked=true`；`activeInputAllowed`、`passiveProbeAllowed`、`runtimeMutationAllowed` 和 `t005Unlocked` 仍为 false。

## 6. CS-T0-05-03 pre-live 实现与下一授权边界

生产侧已新增独立 `coop_passive` runtime：V5 32 槽、Active original-only 13 槽和八个 co-op REL32 槽由唯一 53 槽事务按 `0..52` 安装，安装失败回滚及停止恢复严格逆序。八个 wrapper 都只调用 original 一次；possessor 只在 original 返回后发布停止/失败。双层 endpoint/binding 状态机、生命周期失效、callback drain、逐槽恢复验证、producer close 与 Unsafe 禁止卸载均已进入 Native 和公开 IPC。

测试侧独立建立黑盒 CLI/schema/replay/gate、六场景离线 fixture、mutation/隐私矩阵、真实 Win32 mapping，以及 Debug/Release Passive Native harness。最新完整自动门结果为：284 个 Rust 单元测试、9 个 C0 contract、10 个 passive contract、两种配置各 39 个旧 Native、262 个 Active Native、407 个 Passive Native case，另有 Debug/Release 各 3 个跨语言 case，全部通过；bootstrap 与 T0-04 构建/测试回归也通过。未提供目标映像时，static/C0 target-image CLI replay 由冻结 hash-bound Rust regression 覆盖，不产生新的 live 观察。

pre-live 自动门冻结时尚未创建 passive source 或消费实机授权。之后每次仍只允许一个明确场景的一份新授权；完成 restore、analyze 与 replay 后才可请求下一场景。source/output 已存在、preflight 失败、Native Unsafe 或证据失败都会消费该次授权，不覆盖且不自动重试。

### native-join-passive live-01（授权已消费）

用户于 2026-08-27T18:41Z 单独授权 `native-join-passive`。唯一一次调用使用 case `t0-05-03-native-join-passive-live-01`，Native 在采集窗口建立前以 `TopologyAmbiguous`（error 9）fail closed；未创建 source，因此没有可执行的 analyze/replay。Controller 随后完成单次 shutdown/unload；只读复核确认游戏仍响应，`IsaacCsT0*`、`tractor_beam_native_hook.dll`、`x32dbg.dll` 与 `x64dbg.dll` 残留均为零。该授权已消费，不覆盖、不自动重试，也不推进 `native-exit-passive`。当前证据只能把原因限定在冻结 topology 完整性判定之一，不能在没有事件 source 的情况下猜测具体字段。

### live-01 修复与重新封门

离线对照确认 error 9 来自 Native 对冻结规则的错误升级：初始化期间的 ordinal/assignment/HUD 未收敛及其它被拒绝 topology 本应只撤销 candidate/BOUND 并保持 `UNBOUND`，旧实现却把其中部分永久升级为 `Failed`；同时生产读取额外加入了 C0 不要求的 device object-time COL 门。修复后所有非 coherent manager pass 都不产生正向证据：未绑定时保持 `UNBOUND`，已绑定时推进 generation 并发布正常 unbind；之后仍须两个连续有效 pass 才能重新 `BOUND`。只有冻结 possessor wrapper 继续发 stop/failure，53 槽事务、ABI、IPC 与 analyzer 均未放宽。

Controller 现在无论 Inject、Capture、Unload、WaitStopped 或 process identity 哪一阶段失败，都会尽最大可能 drain，并以 create-new 方式保存 `FAILED/UNSAFE` source；安全失败仍只卸载一次并等待 Stopped，未知 mapping state 按 Unsafe 处理且禁止卸载。独立测试新增 frozen C0 fake-image 正例、25 类 topology rejection 的 UNBOUND/BOUND 转移矩阵，以及失败 source 的 create-new/replay/restore contract。

旧 `coop-verification-v5.json` 继续作为 live-01 历史绑定保留；修复链采用 append-only revision，最终机器入口为 [`coop-verification-v5r2.json`](../../../../../../cs-t0/coop-verification-v5r2.json)，SHA-256 `54C9AB7131C77EFFD3B5001C8C1805A2DACA858C8238E3406CB590FE7783543A`。它绑定 Release Controller `3697697FBA1088F2552341129AA3B0BB9F12C50B8BD61AF28FA07AC0152081C7`、Native `6515C5D7E5A36C720536AA16A9DAA8BC0396926FB305A7E529C05E3FE69A4E17` 与 Helper `3170D805B6F1F2D0BDE4E4F93D38D982A71AC9F65BC9E973DF00B42F63D03972`。该修复封门阶段未执行新实机 run，之后的 live-02 另行记录如下。

### native-join-passive live-02（授权已消费）

用户随后单独授权修复后的 `native-join-passive`，唯一一次调用使用 case `t0-05-03-native-join-passive-live-02`，采集窗口为 180 秒。Native 初始化、21,012 条事件、shutdown 与 restore 均闭合：53 槽完整安装并按 `52..0` 恢复，before/after snapshot 相同，零 gap/overwrite，callback drain 与 producer close 为 true，Unsafe 为 false；游戏在结束后继续响应，T0、tractor beam 与 debugger 模块残留为零。

该 source 的结构与安全检查全部通过，但场景证据不足，正式结果保持 `BLOCKED_BY_EVIDENCE`：事件仅包含 10,504 对 `MANAGER_PASS_BEGIN/END`，没有命中 ordinary spawn、assignment、join HUD、candidate 或 bound 事件；最终 generation 1 的 binding cardinality 为空。因此不能声称发生了冻结原生 join，也不能推进 `native-exit-passive`。显式 replay 得到相同 analysis SHA-256 `BBAEB43F54DF87AF7E128D3E01F2A52C86ADE1F8DBE7E751C974FBDA25F9670A`。source 的 manifest/events SHA-256 分别为 `7948CA1C9E58ACB058DA286C5460D641E767CBA0785412C7D05F5BBC3BAA7C0E` 与 `943BE39D45A2DDD3C2EDC78C0263CF3E384A62E05D0D63D49ACCECF2BAE7602C`。该授权已消费，本轮不覆盖、不重试。

### native-join-passive live-03（授权已消费）

用户确认需要手动操作后再次单独授权，并在 180 秒窗口开始时按第二手柄 `Start/Menu`。本次 source 仍为 `BLOCKED_BY_EVIDENCE`：4,942 条事件中包含 2,467 对 manager begin/end，但仍没有 ordinary spawn、assignment、join HUD、candidate 或 bound；末尾另观察到一对 phase 8 `RunTeardown` lifecycle bracket，并退休 A/B 稳定层。因此该 source 同时缺少完整 join 序列，且在非 teardown 场景中出现 stable registry retirement，不能合并或改判为 join 证据。

安全闭合继续通过：53 槽完整安装/逆序恢复、snapshot 相同、零 gap/overwrite、callback drain、producer close、Unsafe=false；游戏继续响应且 T0/禁止模块残留为零。显式 replay 与原 analysis 一致，analysis SHA-256 `43645BCF28B3471F623451038F916A408B787A119D1A73D89695E58EE3A6DD67`；manifest/events SHA-256 分别为 `996E7A41A9320CB325A228FCA2478C2AC2DA237816D4FBBEBE15A8B8F96BD581` 与 `B357B37CCD57E68A599E37B0908BEFAB402BA6D15F3624185E189DFFA47B7249`。授权已消费，不覆盖、不重试，`native-exit-passive` 仍未获准。

### native-join-passive live-04（授权已消费）

重新诊断 live-03 后确认第二手柄加入发生在采集末尾的 RunTeardown 之后，因此新授权使用 case `t0-05-03-native-join-passive-live-04` 重新采集。游戏日志出现 `[Frame: 0] Spawning Co-Op Player`；source [`20260827T204247Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-04`](../../../../../../artifacts/cs-t0/20260827T204247Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-04) 共 16,368 条事件、8,177 对 manager begin/end，并在同一 manager transaction 内首次捕获完整链：seq 2080 ManagerBegin，2083/2084 ordinary spawn，2087/2088 ordinary assignment，2089/2090 join HUD，2091 ManagerEnd。所有相关 End 的 `originalCalls=1`，Failure 为零。

该 source 仍正式保持 `BLOCKED_BY_EVIDENCE`：8,177 个 ManagerEnd 的 diagnostic digest 全部对应 code 3 `OnlineBackendPresent`，因此 candidate/bound 均为零，最终 generation 3、binding cardinality 为空。显式 replay 与原 analysis 逐字节一致。manifest、events、analysis/replay SHA-256 分别为 `9DBB82E822BCF91A175B8CE836835F44984ABF532BC1C6A64076BD1784B00B97`、`22BF8F43D698C6912B53BCB4016D82D7145045E672429194315198A4974A3AF0`、`2E16F4E0DCDB0AE3040D2332DA01097C044EF9E97E3067DA416188D1C02B4F1F`。

安全闭合全部通过：安装/恢复 mask 均为 `0x001FFFFFFFFFFFFF`，安装 `0..52`、恢复 `52..0`，before/after snapshot 同为 `B08E969C0B4ACA4DE306C19FE5C964796730785AD9D062A4EAD6C8D80159E205`，overwrite=0、callbacks drained、producer closed、Unsafe=false；最终事件为 `HOOKS_RESTORED→PRODUCER_CLOSED`。游戏继续响应且残留模块数为零。该授权已消费；旧 source 与结论不会被后续规则改写。

### live-04 backend-screen 诊断、修复与重新封门

磁盘静态证据确认 `0x5BAC6D` 的实际本地主体 guard 比较 OnlineManager roster begin/end，仅在 roster 非空时跳往 `0x5BB574`；ordinary 46/49/51 三个 callsite 均位于本地主体 `0x5BAC85..0x5BB56E`。因此 live-04 已实际发生完整本地链这一事实证明：Steam backend 对象指针非空本身不足以宣称本地主体不可达。旧 C0 的 backend=false 证据保持不变，修复没有删除 online screen，而是通过 append-only [`coop-passive-backend-screen-v1.json`](../../../../../../cs-t0/coop-passive-backend-screen-v1.json) 作精确收窄，SHA-256 `9395E0DAD0958E9C517D283F17CE8ADFD24F70E27FADD5D6B4D333C67A233790`。

生产规则现为：null backend 保持既有路径；非零 backend 必须指针对齐可读、vtable 精确为 `main+0x7A2440`、online roster begin/end/capacity 全零、online frame 为零，并且当前 generation 在同一顶层 manager transaction 内完成严格 `46→49→51` route。完整 join 只在 spawn 时递增一次 generation，同 route assignment 不再二次递增；下一连续 manager pass 才可 Candidate→Bound。partial、乱序、stale/cross-generation、任一 intervening rejected pass、possessor、exit、无关 assignment 或 online 信号都会撤销 witness。真实 possessor 仍按冻结契约 original-once 后发布 terminal Failure；其它普通拒绝不升级为 Failure。没有新增 Hook、IPC/EventKind 或 analyzer 放宽。

独立 v4 自动门已实际退出 0：Rust 284/284、C0 contract 9/9、passive contract 10/10；Debug/Release 各 39/262/407 个 legacy/Active/Passive Native case；跨语言 Debug/Release 各 3/3；bootstrap、static/C0 hash-bound 与 T0-04 回归全部通过。23 个新增 Native case 独立覆盖 backend screen、route shape、generation、intervening reject、topology 关系、original-once 与 possessor terminal 边界；旧 live-04 replay 仍为 `BLOCKED_BY_EVIDENCE`。verifier v4 SHA-256 为 `6DDA734C778CC59D925005197EFB15B83B7B2EBEE6581D5C46ECE02A6C046740`。

最终机器入口为 [`coop-verification-v5r4.json`](../../../../../../cs-t0/coop-verification-v5r4.json)，SHA-256 `AA010810FA698CFF9E13ECF29753C7F85D941456BA69D356F7F32AE18017E91E`。它 append-only 继承 v5r3，并绑定 Release Controller `50C934C39FEAECD4D8605862DBA56F0D368B16AF1E6FF3AC3E21BF42D8B213E1`、Native `AE692A3163F171159D86A44976D66C480005C0E6EB2F40488EA378BE93212E2E`、Helper `3170D805B6F1F2D0BDE4E4F93D38D982A71AC9F65BC9E973DF00B42F63D03972`。本次修复和自动门没有执行新的实机注入；下一份 `native-join-passive` 仍需用户重新单独授权。

### native-join-passive live-05（中断，授权已消费）

用户授权后的 live-05 在 capture 运行期间中断 Controller 等待，生成不可覆盖的 [`20260828T071546Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-05`](../../../../../../artifacts/cs-t0/20260828T071546Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-05)。manifest 固定为 `UNSAFE/CAPTURE_FAILED_NATIVE_ERROR_0`：已安装完整 53 槽，但 `restoredSlotCount=0`、producer 未关闭、`unsafeUnloadRequired=true`；analysis 保持 `BLOCKED_BY_EVIDENCE`。随后原目标进程退出，新的 `isaac-ng.exe` 具有不同 creation time 且无 T0/debugger/tractor-beam 模块残留，因此没有对该不确定实例执行 unload 或自动重试。manifest/events/analysis SHA-256 分别为 `0209A1210B0CDD8F66B71E9C3BDF8FA44DF5D3107CD242E13CD1AE27BE3BC9C0`、`A6036EB9F786A41210637D8BCDE052A31DA0660DBBADAC0AAF444DCCC09A9352`、`F5228F304D42D5A849E79B329B6F0691DA6D585F9D89616AC0766673A58BFA7A`。该授权已消费，source 不覆盖、不改判。

### native-join-passive live-06（正式 PASS，1/6）

用户重新明确授权并确认已进入单人房间后，live-06 使用全新进程身份与 case `t0-05-03-native-join-passive-live-06` 完成。source [`20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06`](../../../../../../artifacts/cs-t0/20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06) 共 21,621 条事件，正式 analysis 8/8 `PASS`，显式 create-new replay 与原 analysis 字节一致。

冻结 Join 链精确闭合：seq 6657 ManagerBegin；6658/6659 generation 2 A/B unbind；6660/6661 slot 46 spawn；6662/6663 slot 49 assignment；6664/6665 slot 51 join HUD；6666 ManagerEnd；6667/6668 A/B Candidate；下一连续 pass 6669/6670 后，6671/6673 A/B Bound，并分别以 6672/6674 LuaVectorEquivalent 闭合。三个 wrapper End 均 `originalCalls=1`；Join 只把 generation 从 1 推进到 2，同 route assignment 未再次递增；所有 candidate/bound 关系均同时满足 topology coherent、binding unique、assignment/HUD/Lua match。

安全闭合为：mask `0x001FFFFFFFFFFFFF`，安装 `0..52`、恢复 `52..0`、restored 53；before/after snapshot 均为 `B08E969C0B4ACA4DE306C19FE5C964796730785AD9D062A4EAD6C8D80159E205`，overwrite=0、callbacks drained、producer closed、Unsafe=false，最终事件严格为 seq 21619 `HOOKS_RESTORED`、21620 `PRODUCER_CLOSED`。游戏继续响应且残留模块数为零。manifest/events/analysis/replay SHA-256 分别为 `E268ECE2E43FB2F0834E797E9986D0294A67E03FC347CF39D3801D8F5064B991`、`BE9AEE8A6AE6C96F919563AFA71C0A60A10E9A0762D7E7C12927F3BFC5CBC6B6`、`BD2A8C6E8EE18B7D2E0B684062193731B39750DE6835403358AE196827B1FD53`、`BD2A8C6E8EE18B7D2E0B684062193731B39750DE6835403358AE196827B1FD53`。正式进度为 1/6；只有新的 `native-exit-passive` 单独授权才能启动下一份 source。

### native-exit-passive live-07（BLOCKED_BY_EVIDENCE，授权已消费）

用户单独授权后的 live-07 使用 case `t0-05-03-native-exit-passive-live-07` 完成唯一一次采集。source [`20260830T162513Z-t0-05-03-native-exit-passive-t0-05-03-native-exit-passive-live-07`](../../../../../../artifacts/cs-t0/20260830T162513Z-t0-05-03-native-exit-passive-t0-05-03-native-exit-passive-live-07) 共 5,394 条事件；A/B 在 generation 1 建立 Candidate 与 Bound 后始终保持最终 binding `{0,1}`。事件中没有 `BINDING_UNBOUND`、`EXIT_BEGIN/END`、`EXIT_HUD_BEGIN/END`、stop 或 Failure，因此严格场景链 `BINDING_UNBOUND→EXIT_BEGIN→EXIT_HUD_BEGIN→EXIT_HUD_END→EXIT_RETURN_STABLE` 完整缺失，正式 analysis 保持 `BLOCKED_BY_EVIDENCE`。

安全关闭全部通过：53 槽按 `0..52` 安装、`52..0` 恢复，mask 为 `0x001FFFFFFFFFFFFF`，restored 53；before/after snapshot 同为 `92E90E5E3B953B5BB5C4B3810F2E2997D46F694223EDA2E5D84FAE43F63BCD0D`，overwrite=0、callbacks drained、producer closed、Unsafe=false。manifest/events/analysis SHA-256 分别为 `15AE50FC1A5858C06BAAAE7309C4191FE8E3659E6617A8C7DBFEFAD176C7D262`、`6322C0F24BEC2406907185BE0E718B96DB3CB16DCC6442137A436F4550243B8D`、`FCA986F50D05C18F3F8070EE61AB281C7B2CC47292C358010F136D157A70D5B5`。测试侧只通过公开 CLI 建立 [`live-07-replay-01.json`](../../../../../../artifacts/cs-t0/20260830T162513Z-t0-05-03-native-exit-passive-live-07-replay-01.json)，其 SHA-256 与 analysis 相同。

只读 IDA MCP 与精确静态反汇编把冻结 route 收窄为：slot 48 `0x005BB22A` 位于 `0x005BAC40` 的已关联 main-vector Player 分支，由 action 19 `JoinMultiplayer` 的 rising edge 触发；成功后 `0x005BA000` 删除 Player、压缩 ordinal，并在 slot 52 `0x005BA752` 嵌套重建 HUD。当前 `Steam Virtual Gamepad` 配置中 `JoinMultiplayer` 与 `Pause` 都映射到物理 code 15，即 Start/Menu；退出 route 不查询 LB/RB。LB+RB+Start 属于未关联设备的另一条 possessor 路径，不能作为退出 witness。

更关键的 constructor 交叉证据证明 `Player+0x2C` 是 Variant：ordinary `0x005BB415→0x005B9CD0` 创建 main-vector true-coop Player 时调用 Player Init `0x00355470(Type=1, Variant=0, Subtype=ePlayerType, Seed)`；该 Init 的基类调用 `0x002A93A0` 精确把 Variant 写入 `+0x2C`。slot48 在 action 19 成功之后的 `0x005BB14F` 要求 `+0x2C != 0`，因此 ordinary true-coop Player 必然在此失败。possessor 则以 `Variant=2` 初始化，但进入 secondary vector，拿不到 slot48 所需的 main-vector candidate，也同样不可达。故冻结分支只接受“main vector 且 Variant 非零”的非 ordinary Player shape；当前普通真合作没有用户操作能进入它，原 ordinary-exit 假设分类为 `FALSIFIED_BY_STATIC_EVIDENCE`。完整去标识诊断见 append-only [`coop-native-exit-reachability-v1.json`](../../../../../../cs-t0/coop-native-exit-reachability-v1.json)，SHA-256 `034B28853681C43539F669EC015EF3F9C2F8214F9CC25453E31BA86E751D7483`。

本次授权已经消费。禁止自动重试、禁止继续试组合键，也禁止用 death/respawn、设备断连、menu teardown、未冻结自动清理 callsite 或 pointer/topology timing 替代退出证据。该停止条件对 live-07 的证据结论继续生效；它不再阻止用户明确要求的工作流跳过，但 skip 不会改写 analysis、补足缺失 source 或计入六源 gate。任何新增 observer/Hook 或改变 Player shape 仍需新的计划与授权。

### native-exit 工作流跳过与 death-respawn 准备

用户于 2026-08-31 明确要求先跳过当前测试并准备下一测试。append-only [`coop-passive-live-progress-v1.json`](../../../../../../cs-t0/coop-passive-live-progress-v1.json) 将 `native-exit-passive` 记录为用户决定的 `SKIPPED_NOT_APPLICABLE`，同时保留 live-07 的正式 `BLOCKED_BY_EVIDENCE`、既有 hash、已消费授权和禁止自动重试；该进度 artifact 的 SHA-256 为 `BCA44D032163E83130F19F0B488D4839A89BB00076A168050FB7ACFF1D455815`。正式进度仍是 1/6 PASS，`coop-passive-gate` 仍要求六份独立 PASS source，因此当前 skip 不具备 gate 资格。

下一场景固定为 `death-respawn-passive`，预留未冲突 case `t0-05-03-death-respawn-passive-live-08`，默认采集窗口 180 秒。测试侧独立公开 contract suite 为 10/10 PASS，当前冻结 bundle 与生产 binding 未见技术预检阻断；正式授权后的原子 preflight 仍必须重新执行。公开 CLI 输入已冻结为当前 target equivalence、layout evidence、catalog v3、static graph、plan v5 与 verification v5r4；未持久化目标绝对路径，也未创建 source/output。实机前应让普通本地双人均存活且已绑定，并停在尚未击杀的楼层 Boss 门外。采集报告 Ready 且初始状态稳定后，进入 Boss 房让 B 通过正常游戏死亡、A 保持存活并击杀 Boss 触发 B 的正常合作复活，最后静置等待双 pass 重绑；不得以换房本身、设备断连/重分配、返回菜单或 possessor 路径替代。

合格证据必须含有有序 `BINDING_UNBOUND→BINDING_CANDIDATE→BINDING_BOUND`，最终恢复 `{0,1}`，并能由冻结的 `ORDINARY_SPAWN_BEGIN`、`EXIT_BEGIN` 或 `LIFECYCLE_ENTER` 之一直接归因。仅 pointer/digest/topology 变化不能命名为 death/respawn。任何 preflight 失败、同名 source/output 已存在、Native Unsafe、gap/overwrite/producer-close 失败、缺少冻结生命周期归因或窗口结束时未恢复双绑定都会消费该次授权，且不自动重试。当前只完成离线准备；必须收到新的明确 `授权 death-respawn-passive` 后才可注入和采集。

### death-respawn-passive live-08（正式 PASS，2/6）

用户单独授权并确认已到未清 Boss 门外后，live-08 使用 case `t0-05-03-death-respawn-passive-live-08` 完成唯一一次 180 秒采集。用户在活跃窗口内按冻结操作完成 B 死亡、A 击杀 Boss 与 B 原生复活，并明确确认 `B已复活`。source [`20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08`](../../../../../../artifacts/cs-t0/20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08) 共 20,130 条事件，生产 analysis 与测试侧公开 CLI create-new [`live-08-replay-01.json`](../../../../../../artifacts/cs-t0/20260830T182858Z-t0-05-03-death-respawn-passive-live-08-replay-01.json) 均为 8/8 PASS，且 1,749 字节逐字节一致。

冻结 provenance 命中的是计划允许的 inherited lifecycle 分支，而不是专用 death wrapper：slot46 ordinary spawn 与 slot48 native exit 计数均为 0；事件包含 28 对 `LIFECYCLE_ENTER/EXIT` 和 generation 2..28 的 54 条 A/B `BINDING_UNBOUND`。最终 seq 5414 lifecycle enter；5415/5416 generation 28 A/B unbound；5417 lifecycle exit；5418/5419 首次完整 manager pass；5420/5421 A/B Candidate；5422/5423 下一连续 manager pass；5424/5426 A/B Bound，并由 5425/5427 Lua equivalent 闭合。最终 ordinal 精确为 `{0,1}`，A→0/B→1 不变，coherent/unique/assignment/HUD/Lua 均为 true。

正式结论限定为 `PASS (operator-confirmed death/respawn, inherited-lifecycle provenance)`：serialized source 直接证明受控场景中的 lifecycle 失效与 A/B 唯一重绑，用户同窗口 witness 证明实际发生死亡和复活；不能声称 slot46/48 直接观测到 death，也不能把 28 对 lifecycle 本身或单纯 Boss-room transition 命名为死亡。该限定符合冻结 contract 的“spawn/exit 或继承 lifecycle”，且没有用 pointer/digest delta 单独归因。

安全闭合全部通过：mask 安装/恢复均为 `0x001FFFFFFFFFFFFF`，顺序严格 `0..52`/`52..0`，restored 53；before/after snapshot 同为 `92E90E5E3B953B5BB5C4B3810F2E2997D46F694223EDA2E5D84FAE43F63BCD0D`，overwrite=0、callbacks drained、producer closed、Unsafe=false、stable registry 未退休，目标继续响应且残留模块为零。override/apply/world write/scheduler/transport/ModeLatch 均为 0，active/input/world/OriginalOnline 正向能力全部为 false。manifest/events/analysis/replay SHA-256 分别为 `50394427D5217CC7CE7587D8C29AEBB377CA58F0DE7D7EEB5AC0B0BA16E006D2`、`B7A479B17DE837EB662376220B17FB8207952D2E49A7DF112F98FF48A70154FB`、`B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78`、`B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78`。

create-new [`coop-passive-live-progress-v2.json`](../../../../../../cs-t0/coop-passive-live-progress-v2.json) 绑定上述 source、replay、限定结论与安全证据，SHA-256 `F434CC8A7434F429BB936695A76BC5294FD2557B9A7B07B3005F4499F57B7311`。当前真实 PASS source 为 2/6；由于 `native-exit-passive` workflow skip 不计入 gate，六源正式 gate 仍不可执行。下一场景为 `room-rebind-passive`，尚未获得独立实机授权。

### room-rebind-passive live-09（正式 PASS，3/6）

用户单独授权并确认已准备换房后，live-09 使用 case `t0-05-03-room-rebind-passive-live-09` 完成唯一一次 180 秒采集。用户在活跃窗口内只穿过一个普通房间门，并确认完成换房。source [`20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09`](../../../../../../artifacts/cs-t0/20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09) 共 20,396 条事件；生产 analysis 与测试侧公开 CLI create-new [`live-09-replay-01.json`](../../../../../../artifacts/cs-t0/20260830T185508Z-t0-05-03-room-rebind-passive-live-09-replay-01.json) 均为 8/8 PASS，且 1,747 字节逐字节一致。

冻结 room contract 完整闭合：初态 generation 1 的 seq 4/5 为 A/B Candidate，下一连续 manager pass 后 seq 8/10 为 A/B Bound；换房窗口捕获 10 个 `LIFECYCLE_ENTER`、10 个 `LIFECYCLE_EXIT`，generation 2..10 共 18 条 A/B `BINDING_UNBOUND`。最终 seq 4386 lifecycle enter；4387/4388 generation 10 A/B unbound；4389 lifecycle exit；4390/4391 首次完整 manager pass；4392/4393 A/B Candidate；4394/4395 下一连续 manager pass；4396/4398 A/B Bound，并由 4397/4399 Lua equivalent 闭合。最终 cardinality 精确 `{0,1}`，A→0/B→1 不变，coherent/unique/assignment/HUD/Lua 均为 true，稳定 registry 未退休。source 中没有 spawn、exit、relation-change、failure、possessor-stop 或 death/device/menu 替代事件。

安全闭合继续通过：mask 安装/恢复均为 `0x001FFFFFFFFFFFFF`，顺序严格 `0..52`/`52..0`，restored 53；before/after snapshot 同为 `92E90E5E3B953B5BB5C4B3810F2E2997D46F694223EDA2E5D84FAE43F63BCD0D`，overwrite=0、callbacks drained、producer closed、Unsafe=false，目标继续响应且残留模块为零。override/apply/world write/scheduler/transport/ModeLatch 均为 0，active/input/world/OriginalOnline 正向能力全部为 false。manifest/events/analysis/replay SHA-256 分别为 `01FE528ABC6BE9C401A1FA5A8785D37D6DED64482C214FE42B6FB9B777882F0E`、`5F69A683587DCC63DB73CC0C641A76208D90369B620E45B40C3936BA4C093EA3`、`FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA`、`FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA`。

create-new [`coop-passive-live-progress-v3.json`](../../../../../../cs-t0/coop-passive-live-progress-v3.json) 绑定上述 source、replay 与安全证据，SHA-256 `148B7E3ED57BFB1ED2FD9A5A2B641FFD4E4FCA2F1BD53FFA9DE8D69501B51EE2`。当前真实 PASS source 为 3/6；`native-exit-passive` workflow skip 仍不计入 gate，因此六源正式 gate 仍不可执行。下一场景为 `device-rebind-passive`，尚未获得独立实机授权。

### device-rebind 实机前阻断（没有执行 live-10）

用户要求继续剩余两项后，主智能体、逆向侧与测试侧先完成只读准备，未调用 `coop-smoke` 或正式原子 preflight，未注入、未创建实机 source、未消费场景授权。当前游戏进程数量为 0；device live-10 与 menu live-11 候选名称均无冲突，磁盘 bundle/生产 binding 没有漂移。设备项的主要阻断不是运行环境，而是不能可靠证明因果关系的现有证据契约。

测试子智能体仅通过公开 serialized schema 与 Release `coop-analyze` 在独立 `out/` 目录建立四份 23-event 离线反例：三份没有任何 assignment、spawn/exit 或 lifecycle 事件的 source 仍被判 8/8 PASS；其中一份设备与 assignment 摘要不变，另一份连 topology/player/device/assignment 摘要全部不变，仅提升 generation 并自报 `relationChanged=true`、`assignmentMatches=true`。只有把 `assignmentMatches` 清为 false 的对照组才被判 `BLOCKED_BY_EVIDENCE`。这些是明确标注 `OFFLINE_ONLY` 的合成诊断，不是 Native 实测 source，绝不能计入 live gate。独立报告见 [`audit-report.json`](../../../../../../out/cs-t0/device-rebind-independent-audit-20260831-v1/audit-report.json)。

主侧确认生产 `ApplyManagerTopology` 在 coherent bound topology 变化时直接发布 `RELATION_CHANGE` 并设置 assignment-match；Rust device 分支只核对上述自报 flag 与事件子序列，没有要求冻结 assignment/lifecycle witness。逆向侧进一步确认 slot49 `0x005BB424` 的 this 只能来自紧前 slot46 `0x005BB415` 新建 Player，不能覆盖既存 B 的重分配。实际 Change Controller/断连重选调用同一 `SetControllerId` 原函数 `0x003A6450`，但 callsite 是未冻结的 `0x005B27EB`、`0x0055B87F`、`0x0055BCA0`；pump `0x00554F84→0x0055B310` 与现有 alternate/input observer 为先后兄弟调用，没有已证冻结 lifecycle 因果关联。候选原生 UI 路径的 B 菜单 ordinal 写入端也仍未闭环，不能现在要求用户按猜测操作。

create-new 阻断记录为 [`coop-device-rebind-prelive-block-v1.json`](../../../../../../cs-t0/coop-device-rebind-prelive-block-v1.json)，SHA-256 `710F839788AD389BB3BCF0B1DEEDA6CB4F3D2AE003B393F59CE36D3E8B98E8D5`。历史 live-06/08/09、旧 verification 与 plan 均未修改；进度仍为 3/6 PASS。下一步需要新的修订授权来冻结 device 因果取证路径、修复生产与 analyzer 判定并重跑独立自动门；不能凭当前 analyzer 的 PASS 启动或接受 device 实机，不能自动跳过，也没有授权增加 Hook。menu 操作准备为双人稳定绑定后正常返回主菜单，仍需运行中的目标与单独实机授权。

### device-rebind v2 修订与完整自动门 PASS（2026-08-31）

用户随后明确授权“先修订device-rebind的取证计划与判定逻辑，再重跑自动门”。逆向与测试继续由不同子智能体负责；主线完成Rust判定、映射、版本链与自动门集成。新增 [v2取证计划](../01-passive-lifecycle/device-rebind-v2-evidence-plan.md) 和冻结机器契约，三个已证原生assignment callsite只对device场景追加为slot53..55，构成56槽/profile v2；其余场景保持原53槽与v1重放。旧v1 device、新v2 manager-only delta、自报flags和不闭合因果链均不得PASS。

`verify-coop-passive.ps1` 从头执行完成、退出码0，create-new [coop-verification-v5r5.json](../../../../../../cs-t0/coop-verification-v5r5.json) 的19项检查全PASS，SHA-256 `12725A8E38CBDFA6348AF6F97DF3042C335A068623B5272DA513999C0CE5418F`。覆盖Rust 284单元、9 C0合同、5设备合同、11被动合同、fmt/check/clippy/Release；既有39/262/407 Native及新增351条Native路径均Debug/Release通过；新真实mapping16阶段、4条实际Native causal→公开CLI桥接、六场景离线fixture/gate、mutation、bootstrap/static/C0/Active及MCP回归均通过。继承Active脚本仍输出它的历史v1 runtime-blocked拒绝模板，未将该模板改成运行时授权。

新增事务测试直接观察实际patch操作后的内存与顺序，覆盖56×5逐点故障、末槽写入后失败、回滚不安全、正常55..0恢复、callback drain与Unsafe。发现并修复ordered transaction末槽部分写入失败可能误报成功的边界，原分组patch API不变。Native→CLI桥接的终端envelope是离线fixture；实际56槽恢复由独立事务矩阵验证，不冒充实机恢复证据。

最终核对verification全部artifact/production哈希与磁盘一致。三份历史source最后又以Release CLI逐字节只读重放通过；旧prelive-block-v1、progress-v3及各历史plan/verification未改写。正式进度仍为3/6，`sixSourceGateEligible=false`、`csT00504Unlocked=false`、`t005Unlocked=false`，所有主动能力保持false。本轮没有启动、注入游戏或创建实机source，也没有消费实机授权；下一步仅在新的单次`device-rebind-passive`授权后进行实机，完成收尾后再单独申请menu。

Death/respawn 与 device rebind 必须由冻结 spawn/exit、assignment 或其它已证生命周期直接归因；若只能观察 pointer/digest/topology delta，则对应场景保持 `BLOCKED_BY_EVIDENCE`，不得放宽 analyzer 或临时增加 Hook。

`coop-plan-v5.json` 保留为历史53槽入口；修订后的capture要求精确 `coop-catalog-v4.json`、`coop-plan-v5r1.json` 和完整门成功后create-new的 `coop-verification-v5r5.json`。新plan只允许device v2的56槽或其他场景原53槽被动事务，不表示已经获得本次注入或实机授权。

任何 active local-coop run 仍需后续工作包的自动门和新的逐次授权。OriginalOnline、`NetInputDevice`、ready gate、channel `0` 与 Isaac Sync channel `0xFD` 继续禁止作为 local co-op 正向路径。

CS-T0-05-03 的自动门入口为 `verify-coop-passive.ps1`，汇总 Rust、三套 Native harness、跨语言 mapping、六场景离线 fixture、mutation suite 与继承回归。完整 T0-05 的 `verify-coop.ps1` 仍待后续工作包实现。

### device-rebind-passive live-10（安全恢复，正式 BLOCKED_BY_EVIDENCE）

用户单独授权并确认单人房间后，2026-08-31 05:53:22Z 至 05:58:22Z 执行唯一一次 300 秒采集。同窗口完成 B 的普通加入后，仅物理断开 B 手柄；用户确认原生重连提示后接回同一手柄，并确认 B 已重连。没有合成输入、菜单替代路线或重试。

正式 [source](../../../../../../artifacts/cs-t0/20260831T055322Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-10) 共 23,618 条事件。gen2 初态双 BOUND 为 seq4747/4749；gen3 的设备链为 A/B UNBOUND seq11111/11112、slot54 unassign seq11113/11114、slot55 reassign seq11115/11116、RelationChange seq11119、Candidate seq11120/11121、下一连续 pass 的 Bound seq11124/11126。其后直到 seq23615 的 manager 仍为 gen3，无额外 UNBOUND、生命周期、FAILURE 或未分类关系变化。独立测试确认同 Player/endpoint、assignment 读回证据、另一端 A 的关系及连续双 pass 拓扑均满足因果要求。

失败根因是 Native v2 shutdown 的记账不一致：admission 关闭并 drain 后，`g_binding.Invalidate(false)` 清除私有绑定并把 generation 从 3 推进到 4；终止事件从私有状态取值，因此 seq23616 `HOOKS_RESTORED`、seq23617 `PRODUCER_CLOSED` 为 gen4，而 shared owner generation 与 manifest `finalOwnerGeneration` 仍为 3。通用绑定检查清除 gen3 的最终绑定，device 因果检查也拒绝事务后的 generation 变化。这不是用户漏操作，不能放宽 analyzer 或修改旧 source 来补救。此前 raw bridge 的终端 envelope 由 fixture 补齐，没有经过真实 shutdown terminal publication；修复后必须补这一完整回归。

生产分析与独立公开 CLI 一次 create-new [replay-01](../../../../../../artifacts/cs-t0/20260831T055322Z-t0-05-03-device-rebind-passive-live-10-replay-01.json) 均为 `BLOCKED_BY_EVIDENCE`（6/8 checks），逐字节一致；SHA-256 均为 `1E611E206C1DDDC9574CB75F1F5BC76BBBC775B1106F58B417103BB638EF56A1`。manifest SHA-256 为 `09E3C363E761DD79BC6583817F227321A0B658CBBD6C51655F3C3DAFAF93C8E5`，events SHA-256 为 `03A88940CE7BCCEAFFDBA84BF0A7163A54A36209710BE50AC75B4151C4BBF489`；原 source 三哈希前后不变。

安全收尾通过：56 槽安装 `0..55`、恢复 `55..0`，mask 均为 `0x00FFFFFFFFFFFFFF`，逐槽 proof 均匹配；before/after snapshot SHA-256 均为 `F3405572D4FB6B6261C7C5310DF03222B5ED0F9BFD1972250878E9EE5188534F`。overwrite=0、callbacksDrained/producerClosed=true、Unsafe=false；采集后目标仍运行且 Native 残留为 0。零能力、隐私检查无异常。

create-new [progress-v4](../../../../../../cs-t0/coop-passive-live-progress-v4.json) 登记授权已消费、失败不重试。真实进度仍为 3/6 PASS，exit skip 不计入 gate，menu 未授权或开始。下一步须明确授权修复 shutdown 的私有清理与证据 generation 语义，并重跑完整自动门；随后才可申请新的 device 单次实机授权。本轮只诊断、验收和登记，未改生产代码、历史 source、plan 或 verification。

### device-v2 shutdown 修复与完整自动门 PASS（2026-08-31）

用户授权修复收尾 generation 一致性、补回归并重跑自动门。逆向子智能体只修改 Native，测试子智能体独立验收，主线完成 CLI verification 门禁、版本链和集成。冻结范围见 [shutdown-repair-v1](../../../../../../cs-t0/coop-device-rebind-shutdown-repair-v1.json)。device-v2 停止 admission 并第一次 drain 后，清空 candidate/bound、普通/设备 route 身份和 token secret，但不推进最后观测 generation；真实运行中的 lifecycle invalidation 不变。两个终止事件任一发布失败均保持 Unsafe，不能误报 producerClosed/Stopped。legacy 53 槽、IPC/codebook、catalog v4、plan v5r1 及两个 analyzer 均未改动。

测试先独立复现旧代码的 generation 漂移以及首/次 terminal ring 失败仍误报成功，再验证修复。新增 24 种真实 shutdown 边界覆盖完整与未完成事务、candidate/unbound/retired、generation 极值、私有身份清空、实际 callback 等待、恢复/验证失败、ring 失败和关闭后不复活；与既有 351 条设备 Native 路径均在 Debug/Release 通过。新的 Native→公开 CLI 桥接直接采集生产 Shutdown/Event/Publish 的事件和共享状态，四条完整路线 PASS、两条未完成路线 BLOCKED；不再由 fixture 补齐 terminal envelope，不重编号或归一化 generation。离线依赖注入和测试内存事务不冒充游戏实机证据。

完整 `verify-coop-passive.ps1` 于 2026-08-31 06:48:02Z 正常退出 0，create-new [verification v5r6](../../../../../../cs-t0/coop-verification-v5r6.json) 的 **22/22** 项检查全部 PASS，SHA-256 `61DB9BCCE41988AFC1D2723B16012E363B0B2F39064762599B6BF11105827B2B`。覆盖 Rust 285 单元、9 C0 / 7 device / 11 passive contract、fmt/check/clippy/Release；既有 39/262/407 Native 和设备套件 Debug/Release、真实跨语言 mapping、六场景 fixture/gate、mutation、bootstrap/static/C0/T0-04 与 MCP 回归。继承 Active 脚本的历史 runtime-blocked 模板保持原状，不解释成主动授权。

最终复核 8 个 artifact binding、3 个生产 binding 及 7 个不变基线全部匹配磁盘。Release CLI 对 live-06/08/09 的分析重放逐字节不变；live-10 也保持原 6/8、BLOCKED 分析及三份 source 哈希，未通过修改旧证据或放宽 analyzer 改判。门日志见 [automatic-gate-v1.log](../../../../../../out/cs-t0/device-shutdown-automatic-gate-20260831-v1.log)，修复前失败记录见 [red-v1.json](../../../../../../out/cs-t0/device-shutdown-red-20260831-v1.json)。

当前 capture 入口为 catalog v4、plan v5r1 和 **verification v5r6**；进度登记为 create-new [progress v5](../../../../../../cs-t0/coop-passive-live-progress-v5.json)。本轮没有启动/注入游戏、没有实机 source 或新授权，真实进度仍为 3/6；旧 live-10 授权已消费，下一次 device 必须新授权和新 source/output。exit skip 不豁免六源 gate，menu 未授权或开始，所有主动能力和后续解锁保持 false。

### device-rebind-passive live-11（正式 PASS，4/6）

用户明确要求重测，并确认已准备安全单人房间后，使用 verification v5r6 执行唯一一次 300 秒采集，窗口为 2026-08-31 06:57:06.202Z 至 07:02:06.477Z。本窗口内用户依次完成并确认 B 普通加入、仅断开 B 手柄出现原生重连提示、同一手柄重新接回。没有合成输入、重复断连、菜单替代或自动重试。新 [source](../../../../../../artifacts/cs-t0/20260831T065706Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-11) 共 26,184 条事件，生产分析和测试子智能体唯一一次公开 Release CLI create-new [replay-01](../../../../../../artifacts/cs-t0/20260831T065706Z-t0-05-03-device-rebind-passive-live-11-replay-01.json) 均 **8/8 PASS**，分析逐字节一致，源文件前后哈希不变。

初始 A/B BOUND 为 gen2 seq5011/5013；设备事务在 gen3 依次闭合 A/B UNBOUND seq11355/11356、B slot54 BEGIN/END seq11357/11358、slot55 BEGIN/END seq11359/11360、完整 manager return seq11362、RelationChange seq11363、Candidate seq11364/11365（pass10592）、下一连续完整 manager return seq11367 及 Bound seq11368/11370（pass10593），Lua 等价为 seq11369/11371。固定 A→0/B→1、既存 B 因果关系和另一端 A 的 player/device/assignment 摘要均保持正确；之后 7,405 对 manager 同为 gen3，未出现失效、额外重绑或替代生命周期。

本次直接闭合修复目标：seq26182 `HOOKS_RESTORED`、seq26183 `PRODUCER_CLOSED` 均为 gen3，与最终 binding、shared 导出的 manifest generation 和 analysis 一致。56 槽安装 `0..55`、恢复 `55..0`，mask 均为 `0x00FFFFFFFFFFFFFF`，逐项 catalog/proof 匹配；before/after snapshot 同为 `F3405572D4FB6B6261C7C5310DF03222B5ED0F9BFD1972250878E9EE5188534F`。gap/QPC 回退/generation 回退/overwrite 为 0，drain/close=true、Unsafe=false；目标继续响应，Native 模块与 passive mapping 残留均为 0。零能力和隐私检查全部通过。

manifest/events/analysis SHA-256 分别为 `53CB7F12A11929CF53DED515F7B2346BCE3EF7AF52CF17442D22FC958074139E`、`795986B50E0BE59378E20B9CA74C425C3EE49D984A1510C0A45D8BB45DE4F650`、`E726A3339B539712510BFE9E0676B3AE005496FCBB9F0BB304438DEB5BB8C3F0`，replay 与 analysis 同哈希。create-new [progress v6](../../../../../../cs-t0/coop-passive-live-progress-v6.json) 记录本次 PASS 与已消费的单次授权，并保留 live-10 的原 source/analysis 和 BLOCKED 结论。

正式进度现为 **4/6**。`native-exit-passive` 仍是工作流 skip，不算 PASS；`menu-teardown-passive` 尚未授权或开始，需新的单次授权。六源 gate 仍不可完成，`csT00504Unlocked`、`t005Unlocked` 和所有主动能力保持 false；没有修改生产代码、analyzer、catalog、plan 或 verification。

### menu-teardown-passive live-12（正式 PASS，5/6）

用户单独授权菜单测试并确认单人房间准备后，于 2026-08-31 07:20:42.125Z 至 07:23:42.343Z 执行唯一一次 180 秒采集。窗口内用户确认 B 已普通加入；主线确认真实 A/B 双 BOUND 后发出正常返回主菜单、保留游戏进程且不重新开局的操作指引。之后直接捕获冻结 run teardown。用户尚未单独回报菜单 UI，因此该人工确认明确登记为 `NOT_RECEIVED`，不伪填为 true；正式判定依据 Native 生命周期事件与独立 source 验收。

新 [source](../../../../../../artifacts/cs-t0/20260831T072042Z-t0-05-03-menu-teardown-passive-t0-05-03-menu-teardown-passive-live-12) 共 13,573 条事件，生产分析与测试子智能体唯一一次公开 Release CLI create-new [replay-01](../../../../../../artifacts/cs-t0/20260831T072042Z-t0-05-03-menu-teardown-passive-live-12-replay-01.json) 均 **8/8 PASS**，分析逐字节一致，源文件前后哈希不变。独立验收额外验证了当前 analyzer 未单独要求的初始双 BOUND，未利用这一较弱检查省略约定前置。

ordinary join 的 slot46/49/51 为 seq6204..6209；gen2 双 Candidate 为 seq6211/6212（pass3099），下一连续 pass3100 双 Bound 为 seq6215/6217，Lua 等价为 seq6216/6218。随后 3,674 对 manager 保持 gen2 的同一完整拓扑。冻结 phase8 `LIFECYCLE_ENTER` seq13567 在 pass7529/depth2/gen2 进入；A/B `BINDING_RETIRED` seq13568/13569 为实际 phase9、gen3，稳定层退休且四类摘要与原绑定一致；phase8 `LIFECYCLE_EXIT` seq13570 在同 pass/depth、gen3 完整闭合。此后无 Candidate/Bound、普通加入或关系变化复活，最终临时 binding 集合为空。

seq13571 `HOOKS_RESTORED`、seq13572 `PRODUCER_CLOSED` 均为 gen3，与 manifest/shared 导出及 analysis 一致，close 为最后事件。菜单仍使用 schema/IPC v1 的 53 槽：安装 `0..52`、恢复 `52..0`，mask 均为 `0x001FFFFFFFFFFFFF`，逐项 catalog 与独立重算 proof 匹配；before/after snapshot 同为 `2F61B6714DFFB29B188E889386D91E20519BB2E385815C9217EC51A57DADFAE8`。gap/QPC 回退/generation 回退/overwrite 为 0，drain/close=true、Unsafe=false、stableRegistryRetired=true；目标继续响应，Native 模块与 passive mapping 残留均为 0。零能力和隐私检查通过，没有注入 device-v2 的额外三槽。

manifest/events/analysis SHA-256 分别为 `AD06F8E6AE60F959717EE54C0833DA216F17444B0B72F0B4A74AE45772420F28`、`8EFF7DD402CFCA959F9586D166D3FEAD0A141E14BE01A0353D584692D982962C`、`05FC7430983B76B9919CA5F52EE6F497B110BC343D059E3D7C43320B9CDB24AA`，replay 与 analysis 同哈希。create-new [progress v7](../../../../../../cs-t0/coop-passive-live-progress-v7.json) 登记 **5/6 PASS** 及已消费的菜单单次授权，保留所有历史 source 和失败结论。

五个未跳过场景现均完成，但 `native-exit-passive` 仍缺少正式 PASS source。因此 `CS-T0-05-03` 的六源 gate 保持 `BLOCKED_BY_EVIDENCE`，不创建六源 PASS decision、不豁免 exit、不解锁 `csT00504Unlocked` 或 `t005Unlocked`。本轮没有修改生产代码、analyzer、catalog、plan 或 verification，也没有自动开始任何后续实机；后续处理该缺项需要用户明确决定。

### 原生退出适用性文档修订（2026-08-31，机器状态未迁移）

用户决定修订文档。原 native-exit 测试的安全目标是单独移除 B 后保持 A、mapping、HUD/Lua 与 generation 正确；但已找到的移除函数不能证明普通真合作 B 有可操作的原生退出入口。静态反证表明冻结 slot48 caller 要求 main-vector candidate 的 `Variant != 0`，而 ordinary 构造固定为 `Variant=0`；possessor 的 Variant=2 在 secondary vector，也不能充当该候选。结论仅限当前目标和已证路径，不外推为游戏绝无其他退出路径。

完整 [适用性修订 v1](../01-passive-lifecycle/native-exit-applicability-revision-v1.md) 将本项列为文档标签 `NOT_APPLICABLE_NATIVE_ROUTE`，不计 source PASS；计划改为五份适用 PASS source＋独立 hash-bound 不适用决议。live-07 原 source/analysis/replay 永久 BLOCKED，workflow skip 不升级为 PASS，当前 CLI/schema 仍不接受此文档标签。现有 [progress v7](../../../../../../cs-t0/coop-passive-live-progress-v7.json)、六源 gate、verification v5r6 与全部 plan/ledger 均未改写，正式机器状态仍为 5/6、BLOCKED。

下一待办是另行授权新版本适用性 gate、版本链与独立 mutation/自动门实现；完成前不进入 05-04，不继续要求用户尝试当前不可达入口。自定义 Endpoint 逻辑退出与游戏角色物理移除留待单独规格、实现、授权和验收，不能直接调用 `0x005BA000` 代替证明；现有 stale-generation、停止输入、drain、恢复和 Unsafe 约束继续有效。

本轮只修改 Markdown 文档，没有修改生产实现、测试/verifier、JSON、历史 source 或二进制；未重跑自动门、未实机、未创建新授权或解锁能力。先前 menu UI 人工确认仍为 NOT_RECEIVED，不因这次文档修订补写。

### 适用性 gate 迁移与正式关闭 passive（2026-08-31）

用户另行授权实施 gate 迁移并重跑自动门。主线新增 Rust gate/CLI、严格适用性 contract 和 create-new 版本链；逆向子智能体提交 29 个磁盘静态锚点与 scope 复核，测试子智能体通过公开 CLI、测试导出及临时副本完成 17 项独立黑盒。Native、旧 source analyzer、旧六源 gate、旧 JSON 与历史 source 均保持基线字节不变。

完整 [自动门 v3 日志](../../../../../../out/cs-t0/passive-applicability-automatic-gate-20260831-v3.log) 正常退出 0，[verification v5r7](../../../../../../cs-t0/coop-verification-v5r7.json) 为 **28/28 PASS**，SHA-256 `AF106B5C7082C728423FE68FE2632C57017CE5325A14CAE68929D94945D24421`。覆盖 Rust 285 单元、9 C0 / 7 device / 11 passive / 17 applicability contract、fmt/check/clippy/Release、Native 39/262/407 与 device Debug/Release、真实 mapping/shutdown bridge、六场景 fixture/旧 gate、mutation、bootstrap/static/C0/T0-04 和 MCP 回归。SDK 权限失败及 PowerShell 拼参失败的两份日志均保留，修复后从头完成本次完整门。

新 [decision v3](../../../../../../cs-t0/coop-passive-decision-v3.json) 正式 PASS，SHA-256 `8793F107C1AF1480A619FAA20572FE851FD099E8CB0344CFF4D78B4AD8007D31`。其后按无环 SHA 顺序创建 matrix v3、ledger v4、hypothesis index v3、verification v6、plan v6 和 progress v8。矩阵保留 20 个编号：8 个字面 PASS、1 个继承 PASS_COMPOSITE、1 项限定 N/A、10 项 NOT_STARTED，即 19 个适用条目中完成 9 个；两个 virtual 条件 primary 和两个 repeatability 未减免，comparison 尚无结论。

[独立审计 v2](../../../../../../out/cs-t0/applicability-final-independent-audit-v2.json) 为 **15/15 PASS**；唯一 [Release formal replay](../../../../../../artifacts/cs-t0/coop-passive-decision-v3-independent-replay-01.json) 退出 0，除 `createdUtc` 外与正式 decision 的 UTF-8 字节完全一致。8 产物、19 条向前 SHA 依赖边、20 个构建输入、3 个当前生产文件、149 个不变基线和 18 个 source 文件全部复核通过。审计 v1 将继承的 PASS_COMPOSITE 误要求为字面 PASS，已在运行 CLI 前停止并保留失败记录；只修正审计工具，正式产物未改写。

当前 [plan v6](../../../../../../cs-t0/coop-plan-v6.json) SHA-256 `820F2F2F8410BE93EEFB98A1E2E3C66A1A9A60D090D901EF22FEDFE932991D59`，仅开放 `csT00504Unlocked`，关闭 passive Hook/观察与所有主动能力。[progress v8](../../../../../../cs-t0/coop-passive-live-progress-v8.json) 记录五份 PASS＋一项限定 N/A；live-07 仍 BLOCKED，death 仍是用户确认＋继承生命周期，menu UI 仍 NOT_RECEIVED。没有新实机、自动重试、历史授权复用或自定义退出实现；05-04 可开始实现与前置验证，active 实机另需新授权。

## 7. CS-T0-05-04 证据前置与阻断门（2026-08-31）

本轮从已通过的plan v6开始，已实现只读 `coop-join-preflight` 和专用阻断门verifier。53个静态字节锚点、53＋2槽候选catalog、164文件历史快照及旧三生产二进制均已冻结。新的完整自动门v3退出0，独立黑盒8/8、历史适用性18/18及完整继承回归通过；Rust test/fmt/check/clippy、Release、旧Native Debug/Release、真实mapping和Shutdown桥接均实际执行。详细命令、计数及失败保留见[独立验证结果](../02-active-join/native-join-active-test-results-v1.md)。

| 新产物 | 状态／SHA-256 |
| --- | --- |
| [verification v1](../../../../../../cs-t0/coop-native-join-verification-v1.json) | `PASS_BLOCKED_GATE_VERIFICATION`；`5F2CE26D88D27B3869DE9B01B92DB79737F2A17369A990781BE96FAECDE61CE4` |
| [plan v7](../../../../../../cs-t0/coop-plan-v7.json) | `BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`；`2BFD017329754E89EF4337248DB37468E292C75CD1490FDA30F4BE3A9FB6DCE3` |
| [join progress v1](../../../../../../cs-t0/coop-native-join-progress-v1.json) | `BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`；`FE69F36DF7530CF8CEFB9CBC5315A6E72436B841EB308AAE5FDC060637073D1F` |

当前Release EXE由新verification独立绑定，旧verification及其生产文件保留原哈希。v1完整门因SDK权限失败，v2为修复Windows输出别名拒绝而中止；两份日志及开发期fixture失败均保留，没有覆盖或改判。最终v3不复用其部分结果，旧149文件契约、五PASS、live-07 BLOCKED、live-10 BLOCKED及旧六源结论保持不变。

[补核纪要](../02-active-join/native-join-active-followup-v1.md)的46个字节窗口和相应CALL/分支/虚表目标另经独立磁盘核对通过。它为限域ordinary join正向路径提供静态依据，不要求未来passive witness；但尚未升级完整可执行准入/稳定性契约，生产preflight继续按冻结v1报告七项缺口。首要实际阻断是无持久污染：P0更新、尾部直接计数写、cache中的成就和Lua回调及声明窗口内保存/Cloud/Steam链仍未闭合。下一步补证对象已在纪要中按具体函数列出。

**本轮不是05-04 PASS，也不具备申请单次主动实机的资格。** 新55槽runtime、join专用IPC/arm/claim、两次UI覆盖、主动source/analyzer/replay均未实现或验收；旧53/56槽回归不能替代它们。未打开或注入目标游戏进程，未生成live授权或source，未调用普通B退出。矩阵仍为v3，第11项 `NOT_STARTED`；`csT00505Unlocked=false`、`t005Unlocked=false`，T0-06保持关闭。

[发布后独立终审](../../../../../../out/cs-t0/native-join-publication-independent-audit-v1.json)为 `PASS_PUBLICATION_BINDINGS_AND_ZERO_ACTIVE_AUTHORITY`，SHA-256 `51FBAB9BF684D10D49F23DA502E4B41F83ABC48880AEFFFF48E91ECB671C4582`。18条新产物绑定、20条verification绑定、164快照、149当前原基线、161当前非production历史文件和六源18文件全部匹配；14权限与v6精确一致，矩阵11～20均未开始。T0-06关闭由已绑定progress v8的限制字段继承。首次终审脚本因PowerShell `-join` 比较括号错误在报告发布前退出，仅修正审计工具后从头重跑成功，被审产物及验收条件未改。

## 8. CS-T0-05-04 静态条件审查（2026-08-31）

新[verification](../../../../../../cs-t0/coop-native-join-static-closure-verification-v1.json)为`PASS_STATIC_CONDITION_REVIEW_VERIFICATION`，SHA-256 `319C43D13A28A783976410C896DABE33E4D6E4BD4C17D47594EAFAFFDBF7B8A1`；只认证磁盘证据与新只读工具，`blockingStatus`仍为`BLOCKED_BY_EVIDENCE`。实际14组工具对抗、9组公开黑盒/70项断言通过，核对146锚点、1,994指令、456边、5处常量引用、4个浮点常量以及Init v1→v2的22条助记符修订。失败v1与四份拒绝审计、窗口工具格式失败均保留，不改判、不覆盖。

新[plan v8](../../../../../../cs-t0/coop-plan-v8.json) SHA-256 `AA4966AFF7855F69367C15F7D4403B04F86ECE64D33C41398057407F13CDD1F5`；[join progress v2](../../../../../../cs-t0/coop-native-join-progress-v2.json) SHA-256 `6CBF6095E7CAF27928028D76ED9D6DEF33C131F553AA15982D4114CDC5CF0DC6`。二者仅发布条件审查阶段，12个缺口按静态、实现、未来实机分别保留。独立门前后核164快照、149基线及三份生产文件；未重建生产程序、重签旧verification或重跑旧完整自动门。原Release preflight仍退出1并报告原七项缺口。

剩余静态范围是constructor/Init→effective-count/desired-familiar的状态传播，以及Init与自然GameUpdate→计时器/request/readonly的组合。字段reader与generation/ABA契约随后实现。矩阵第11项仍`NOT_STARTED`，主动55槽runtime及实机未认证，05-05/T0-05整体/T0-06继续关闭；不能申请单次主动实机。

[发布后独立审计](../../../../../../out/cs-t0/native-join-static-closure-publication-audit-v1.json)为`PASS_PUBLICATION_BINDINGS_AND_ZERO_ACTIVE_AUTHORITY`，SHA-256 `749E3F3F7135BBDF0479E7E2A1D3252F2DDBD2971533B9FCC3DA6CD8D1AE3269`。7个发布根、66处新引用、verification32输入及旧164/149/生产3均匹配；落盘inspection与独立黑盒baseline全结构相等，12项缺口、计数和权限一致。该终审不升级任何主动能力。

## 9. CS-T0-05-04 静态接续与准入规格审查（2026-09-01）

新[verification](../../../../../../cs-t0/coop-native-join-admission-verification-v1.json)为 `PASS_STATIC_ADMISSION_REVIEW_VERIFICATION`，SHA-256 `EB11FF0ECE82DB2D6E21D285923E97573BEDCAD7569AD3900B3B359B0BCFE2FC`；[合法inspection](../../../../../../out/cs-t0/native-join-admission-inspection-v1.json)仍为 `BLOCKED_BY_EVIDENCE`、退出1。测试严格区分磁盘字节/独立解码、局部数据流语义、离线模型和公开哈希防篡改；这不是完整无污染或生产准入实现通过。

本轮独立核对新49锚点、1,524指令、283条直接边，并重核旧146锚点、1,994指令、456边。实际通过23个契约用例、104个离线模型用例（4正向、100反例）、21个解析/引用用例、5组公开黑盒/57项断言及继承14组工具对抗；另以60条独立解码指令审查局部前提和结论，并核对21个guard及新增字段owner/宽度。正式门前后164快照、149基线、三份生产文件不变，旧preflight仍为原七项缺口。

新[plan v10](../../../../../../cs-t0/coop-plan-v10.json) SHA-256 `CABE3489941C907F6776E954EC255F49AD8613E274AB6014FA3688FB0EF6E658`；[join progress v4](../../../../../../cs-t0/coop-native-join-progress-v4.json) SHA-256 `887967EA8330EFD5DA5D8A8A536CF88A735CBA731F67C10B68F3A602AF0B31AD`。状态为 `BLOCKED_BY_EVIDENCE / STATIC_ADMISSION_CONTRACT_REVIEW`；191项reader描述中11项仍UNKNOWN，12项原未决义务全部保留，新增失败策略义务仅为SPECIFIED。generation所有权、采样、永久失能和Unsafe规则已规定，不表示原生ABA覆盖或reader实现已验证。

仍未闭合的是Init/callee/P0的顺序状态保持、所有有效familiar来源/匹配行为、Room/Grid/player/Lua/lifecycle在完整自然更新与停止窗口内的保持，以及config/container/callback/entity reader和原生地址复用检测。具体路径分别记录于[Init v3](../04-init-closure/native-join-init-closure-v3.md)、[窗口 v2](../05-persistence/native-join-persistence-window-v2.md)和[准入实施记录](../03-admission/native-join-admission-implementation-v1.md)。不能凭原始道具为空、readonly为真、dirty最终为零或两次干净采样关闭这些缺口。

v9/v3首次发布继承了旧verifier入口、工具列表和scope，独立审计指出后未覆盖原件，另建v10/v4修正发布元数据；证据、契约和正式verification未变。公开入口开发验收先后发现引用格式、继承锚点schema与Windows尾随点/空格别名问题；修复工具后从头执行development v4及正式完整门，未调整验收要求或改写失败。全部日志、失败及重试记录经[发布输入清单](../../../../../../out/cs-t0/native-join-admission-publication-inputs-v3.json)保留，文件写入权限失败也单独记载。无新Hook、保存压制、readonly修改、删除B、生产重建或实机操作；矩阵第11项NOT_STARTED，active/live、05-05、T0-05整体和T0-06保持关闭。

[修正版发布独立审计 v2](../../../../../../out/cs-t0/native-join-admission-publication-audit-v2.json)为 **10/10 PASS**，SHA-256 `36345F72D34D732364CCC1A5294BCF983B3AEFF453D4D14227EBBBEA9E26E935`。检查覆盖v10/v4全部发布绑定与无环链、正式verification的56个当前输入、164/149/3历史保护、相同权限与生产commands、正确新verifier入口/scope/tools、三个可变入口及matrix第11项。首次[发布审计 v1](../../../../../../out/cs-t0/native-join-admission-publication-audit-v1.json)的三项元数据失败保持原结论，v9/v3不覆盖。该终审只接受静态规格发布，不提升原生闭合、reader、runtime或live资格；本段仅追加于可变索引，不回写任何封印输入。

## 10. manager-this 同代身份静态补证暂停检查点（2026-09-02）

当前工作已按用户要求暂停，完整冻结见 [pause checkpoint v30](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v30.md)。已接受正向计数保持 copy/move `319/405`、memset `225/242`，精确剩余 `103`；没有新增正向闭合。全部剩余 memset `17/17` 已有独立负向审计，剩余 copy 的有效独立负向覆盖为 `69/86`，callerHeap residual exact17 的候选虽为 `0V/17U`，但作者分离审计被暂停，不能计入验收。

当前机器前沿仍是 v29，其 JSON 只绑定 copy 负向覆盖 `28/86`；EntityList `2`、vectorShift `25`、string `14` 的后续独立审计已完成但尚未合成 successor frontier。save-record `0x001CBEB5` stage3 也在唯一 stack liveout 接续处暂停。所有这些负向/未完成结果只保留 UNKNOWN，不构成 manager-this 同代身份的正向证明。

正式发布状态继续沿用 plan v10 / join progress v4 的 `BLOCKED_BY_EVIDENCE / STATIC_ADMISSION_CONTRACT_REVIEW`。manager-this 不得标为 `STATIC_CONFIRMED`，矩阵第11项、active/live、05-05、T0-05整体和T0-06均不解锁；本次只追加 Markdown 进度，不改代码、JSON、verification、plan、progress 或权限。

## 11. manager-this 第二次暂停与 callerHeap 独立审计（2026-09-05）

用户再次要求停止逆向后，三个执行中子任务均已中断；最新完整冻结见 [pause checkpoint v31](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v31.md)。callerHeap residual exact17 的作者分离独立审计已封印为 `0V/17U`，mutation `85/85`；它使证据层的剩余 copy 负向覆盖达到 `86/86`，连同 memset `17/17`，精确剩余 `103/103` 均有负向审计。

负向审计不提供正向闭合。接受计数仍是 copy/move `319/405`、memset `225/242`、remaining `103`；机器 frontier 仍为 v29 并只绑定 `28/86` copy 覆盖。successor validator 只有未封印脚本/self-test，没有 handoff 或真实 candidate 验收；save-record stage3 只有未落盘的 def-use 收窄观察。因此 manager-this 继续 `UNKNOWN / FAILED_MANAGER_THIS_IDENTITY_PREMISE`。

plan v10 / join progress v4、矩阵与全部权限保持不变。本次不继续逆向、不合成 successor、不创建 whole-image raw v2 或 formal v7，不启动游戏、调试器或实机。

## 12. manager-this v30 负向覆盖机器前沿封印（2026-09-05）

在不恢复任何逆向任务的前提下，v31 尚未封印的 successor 验收流程已完成，完整状态见 [pause checkpoint v32](../06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v32.md)。独立测试角色先封印参数化 validator/self-test v2；主线随后 create-new 合成 [bulk frontier v30](../06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v30-handoff.md)；同一未参与 candidate 编写的测试角色再对真实 candidate 完成 [独立审计](../06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1-handoff.md)。

v30 正式把 copy remaining 的作者分离负向审计覆盖从机器 v29 的 `28/86` 更新为 `86/86`，memset 保持 `17/17`。更新只涉及负向覆盖字段；正向接受仍为 copy/move `319/405`、memset `225/242`，精确 remaining `103` 全部继续 `UNKNOWN`。独立审计接受的正向闭合数为 0，manager-this、whole-image、overall acceptance 与全部权限仍为 false。

正式状态因此继续为 `UNKNOWN / FAILED_MANAGER_THIS_IDENTITY_PREMISE / BLOCKED_BY_EVIDENCE`。save-record stage3 未恢复，v31 登记的未封印观察没有进入 v30；whole-image raw v2、contract/formal v7、plan/progress、matrix 与解锁字段均未创建或改写。本轮结束后重新暂停逆向。

## 13. manager-this 完整证明任务分解 v33（2026-09-05）

在不恢复逆向的前提下，主线与独立 TEST 角色完成只读 completion audit，并把 v30 到正式 v7 的全部依赖固化为 [任务分解 v33](../06-manager-identity/00-overview-and-gates/native-join-manager-this-proof-completion-work-breakdown-v33.md)。exact103 被拆为 29 个正向逆向微任务；每个 candidate 都必须另配作者分离 audit，主线再以 create-new v31+ frontier 逐批合成，最终达到 copy `405/405`、memset `242/242`、remaining `0` 与 bulk `647/647 fullyAdjudicatedNoOverlap`。

zero frontier 之后的固定链为 whole-image raw v2、独立 whole-image 审计、v7 四件套、contract hash/publication seal、正式 v7 独立验收。审计同时确认 `cs-t0/admission_v7/inspect_admission.py` 的 `CONTRACT_SHA256` 仍为 `__PENDING__`，且当前 builder contract 未绑定 inspector、公开 wrappers 与 verifier；v33 把无环 publication manifest 列为发布前必做，防止 checker 漂移。

当前没有任何正向计数变化，状态仍是 `UNKNOWN / FAILED_MANAGER_THIS_IDENTITY_PREMISE`。本轮只追加文档，没有启动 29 个逆向任务、whole-image 或 formal 发布；用户暂停边界、plan v10 / progress v4、矩阵和全部权限保持不变。

## 14. MT-R01–R10 裁决与后续完成优先级（2026-09-06）

`MT-R01`–`MT-R10` 的 20 个 copy/move RVA 已完成逐行 REVERSE candidate 与作者分离 TEST 裁决，完整事实状态见 [completion checkpoint v34](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r01-r10-completion-checkpoint-v34.md)。`0x001CBEB5` 与 `0x00689F63` 进入 v32 positive frontier；其余 18 行保持 `UNKNOWN`。当前账本为 copy/move `321/405`、memset `225/242`、总计 `546/647`、remaining `101`。

后续 18 行已在 [follow-up priority v35](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r01-r10-followup-priority-v35.md) 中按闭合概率、穷尽成本和解阻价值排序：先处理 `MT-R06`、`MT-R08 0x0068E3DB`、`MT-R10` 的有限 writer/lifetime fixed point；再处理 `MT-R02/R07/R09`；随后完成 `MT-R04` 的 177+5 producer；最后只对已有直接反例的 `MT-R03/R05/MT-R08 0x00661577` 做有界硬 guard/不可达性搜索。任务范围与 promotion 门仍只来自 v33，优先级不改写 v34 裁决。

测试策略同步压缩为每份新证据一次 create-new、一次 verify-existing 与覆盖不同失败机制的最小独立拒绝集；不重复无关 Cargo、全量 T0 或旧 self-test。没有通过行的波次不创建空 successor。动态分析不是当前前置条件，且即使以后另获授权也只能补充 `RUNTIME_OBSERVED`，不能替代静态无重叠证明。

若 18 行全部正向闭合，数学上限为 copy/move `339/405`、memset `225/242`、总计 `564/647`、remaining `83`；这不是预设结论，也不完成 T0-05。`MT-R11`–`MT-R29`、whole-image raw v2、formal v7、manager context、active/live、05-05、T0-05 总门与 T0-06 仍保持关闭。本次只更新优先级和索引，没有执行新的逆向、动态分析、生产修改或权限变更。

## 15. MT-R01–R10 follow-up 与 v33 frontier（2026-09-06）

按 [follow-up checkpoint v36](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r01-r10-followup-checkpoint-v36.md) 完成冻结 PE/IDA 静态接续。本轮新增 `0x0061F2C2` 与 `0x0068E3DB` 两条 `STATIC_CONFIRMED_NO_OVERLAP`；连同 v34 已接受的 `0x001CBEB5`、`0x00689F63`，前十项当前为 `4 positive / 16 UNKNOWN`。

用户要求暂时跳过 `MT-R04` 测试，因此作者分离 TEST 的范围精确为 exact16。R04 的 `0x0001AAB0`、`0x0001AEB8` 没有 follow-up verdict、mutation 或 promotion，并在后继账本的 callerHeap、copy remaining 和 accepted-negative coverage 中全部保留。exact16 独立审计接受新增两行、保留其余 14 行 UNKNOWN；其 JSON SHA-256 为 `064079AB81F4FD18079209444E4718242A04B08C602810133B7FE4212FC8ABDB`。

主线随后 create-new 合成 v33 frontier，独立 TEST 从 v32 与 exact16 audit 重建整个对象并通过。当前账本为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`；v33 JSON SHA-256 为 `861BEA2C2B26FA999F4043ACDEE53786759CB4341A6F5EB2B37FEA6645E5B032`，独立 audit SHA-256 为 `176DAD8177015908EC7E49FFF020BE0030E2747DB6AB672183D9CD3A1DFDEB69`。

测试规模按用户要求压缩：每个 REVERSE successor 只运行 create-new、一次 verify-existing 与核心 mutation；两层 TEST 各只运行 create-new、一次 verify-existing、六个决策分支 mutation 和 wrong-hash fail-before-write。没有运行 Cargo、全量 T0 build 或重复 self-test。没有启动游戏、打开游戏进程、使用 x32dbg、注入或修改 IDB；保留 UNKNOWN 不需要特定动态场景。manager-this、whole-image、active/live、runtime mutation、05-05、T0-05 总门和 T0-06 仍全部关闭。

## 16. MT-R02/R03/R05 exact5 深入复核（2026-09-06）

按 [checkpoint v37](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-r03-r05-checkpoint-v37.md) 继续审计 R02、R03、R05 五行。R02 两路仍是按 `newCount` 分配、按 `oldCount` 复制，缺少 `oldCount<=newCount` 和两端 backing fixed point；R03 三个 producer 的 5000 比较均只记录并继续，5001 没有硬控制流阻断；R05 仍无法排除 auxiliary range 非空、filtered range 为空的合取。独立 TEST 最终接受 `0 positive / 5 UNKNOWN`，audit v2 SHA-256 为 `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE`。

TEST 在候选前封印 exact5 contract。原 sealed validator 的 candidate mutation harness 暴露两个全 UNKNOWN 候选上的空操作和一处逐行变量复用 bug；两次失败都在写审计产物前退出。append-only v2 wrapper 只纠正执行实现，不改封印 spec、contract、candidate 或历史结果；最终 create-new、唯一 verify-existing、六个决策性 mutation 与 wrong-hash fail-before-write 均通过。

没有正向行，因此没有创建空 frontier successor；v33 继续为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`。本轮未启动游戏、打开进程、使用调试器或修改 IDB。R05 若以后做存在性动态复核，需命中 `sub_734F30` 且同时观测 aux 非空、filtered 为零；当前没有已证可靠的用户场景，故未请求用户操作。所有权限和后续门继续关闭。

## 17. MT-R05 filtered-vector v3 单行裁决（2026-09-06）

按 [checkpoint v38](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r05-filtered-vector-checkpoint-v38.md) 对 `0x0033570B -> 0x00734F30` 做有界静态接续。auxiliary vector 在正常构造路径精确产生 14 项，但 filtered tuple 从零开始；源计数为零可直达空 merge，各 reject 分支也允许全部候选不 append。七个直接 caller near-predecessor 未提供支配性的非空 guard 或必 append 不变量，故 `auxiliaryRangeCount>0 && filteredCount==0` 仍不可排除，首缺口保持 `countAndNoWrap`。

作者分离 TEST 在逆向 candidate 前封印 exact1 规则，最终接受 `0 positive / 1 UNKNOWN`；audit SHA-256 为 `37195EFEE870E371B8915264F3E1AB45906B54508BE08E985B92501B19E73765`。测试仅执行一次 create-new、一次 verify-existing、四类决策 mutation 与一次 wrong-candidate-hash fail-before-write；没有运行 Cargo、全量 T0 build、旧 self-test、游戏或调试器。

没有正向行，因此不创建空 frontier successor。v33 仍为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`，前十项仍为 `4 positive / 16 UNKNOWN`。manager-this、whole-image raw v2、formal v7、active/live、runtime mutation、05-05、T0-05 总门和 T0-06 全部保持关闭。

## 18. MT-R05 用户假设策略通过（2026-09-06）

用户根据该版本长期游玩未观察到相关崩溃，明确接受 MT-R05 的剩余风险，并要求先假设 `0x0033570B` 的 `memmove` 不会发生 32 位长度下溢。[checkpoint v39](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r05-user-assumption-policy-checkpoint-v39.md) 因此将 exact1 确认为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`，并从后续人工逆向队列移除。

该通过属于用户策略，不是静态证据改判。v38 的 `UNKNOWN`、`countAndNoWrap=OPEN` 与 `0xFFFFFFFC` countermodel 全部保留；v33 strict frontier 继续为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`。人工逆向策略队列则为 copy/move `81`、memset `17`、总计 `98`。

候选前封印和候选后独立 TEST 均通过，最终 audit SHA-256 为 `92FC838C9063514508D79BCE65174D7A57ACDB4BBEA01BF6E28048506E295C5A`。waiver 不作为 `STATIC_CONFIRMED_NO_OVERLAP`、zero frontier、whole-image 或 formal v7 的静态正向输入，也不授予 manager-this、runtime、active、05-05、T0-05 总门或 T0-06 权限。

## 19. MT-R03 用户假设策略通过（2026-09-06）

用户要求沿用 MT-R05 的处理，基于长期游玩未观察到类似崩溃，接受 MT-R03 两个固定 5000-element backing callsite 的剩余风险。[checkpoint v40](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r03-user-assumption-policy-checkpoint-v40.md) 将 `0x0001513A` 与 `0x000155A4` 确认为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`，并从人工逆向队列移除。

严格证据没有改判：两行仍为 `UNKNOWN / backingExtent OPEN`，5001 元素的 84-byte 与 108-byte countermodel 均保留。v33 strict frontier remaining 仍为 `99`；累计 R03/R05 waiver exact3 后，人工逆向队列从 `98` 降为 `96`，即 copy/move `79`、memset `17`。

候选前封印与独立 TEST 均通过，audit SHA-256 为 `758F92C60890DE425E19932231019A7270148CD7FA01D29DD2BB92C472EA63D2`。该 policy PASS 不授予静态/运行时 promotion，不修改 manager-this、whole-image、active、05-05、T0-05 总门或 T0-06 权限。

## 20. MT-R02 animation descriptor 有界复核（2026-09-06）

[checkpoint v41](../06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md) 完成 `0x00008A62` 与 `0x00008B2A` 的 R02-only 有界静态接续。descriptor `+0x1C/+0x24` 已确认是 primary/secondary animation runtime state 两组 int32 backing 的元素数；`Idle/Idle2` 与 `HeadLaugh*` 的具体 `flag=0` preserve/migrate 调用链均没有 count compare、clamp 或支配性 hard bound。

目标仍按 `4*newCount` 分配、按 `4*oldCount` 复制。当前 PE 没有找到真实 `old=2,new=1` asset producer，但该组合在已证 CFG 上未被硬条件排除；两行因此继续为 `UNKNOWN / backingExtent OPEN`，不得 promotion。v33 strict frontier 仍为 remaining `99`，人工逆向队列仍为 copy/move `79`、memset `17`、总计 `96`。

独立 TEST 在候选前封印 exact2 contract，最终接受 `0 positive / 2 UNKNOWN`；audit SHA-256 为 `9677D086F43A65F4177BA21619DD189481AA6CC10E6108FA1696827260CAAB4B`。测试压缩为 preparation 与 audit 各一次 create-new、恰好一次 verify-existing、4 个决策 mutation 和一次 wrong-hash fail-before-write；未运行 Cargo、全量 build、历史回归、游戏或调试器。manager-this、whole-image、active、05-05、T0-05 总门和 T0-06 权限全部保持关闭。

## 21. 原生 Join 创建与发布用户规划假设（2026-09-06）

用户现要求先假定 Authority 可以通过原生 local-co-op Join 路径可靠地创建并发布第二玩家 B。该决策已记录于[用户规划假设 v1](../../02-active-join/native-join-active-user-planning-assumption-v1.md)，证据标签固定为 `DESIGN_ONLY / USER_ACCEPTED_PLANNING_ASSUMPTION`。后续只读规划可使用 fresh B、ordinal `1`、assignment、HUD/index 与 Lua order 同 generation 一致的抽象后置条件，并准备 `CS-T0-05-05` virtual-device 决策。

严格状态不变：本记录不是 `STATIC_CONFIRMED`、`RUNTIME_OBSERVED` 或正式 gate `PASS`，也不覆盖 manager-this、Init/自然更新、零持久污染、reader/ABA、恢复和全部历史失败/UNKNOWN。plan v10、join progress v4、矩阵第 11 项及机器权限均未修改。

本轮仅更新 Markdown，没有代码、JSON、production、游戏、调试器或实机操作。active/runtime mutation、正式 05-05 解锁、T0-05 总门和 T0-06 继续关闭。

## 22. 只读 Virtual Device 必要性决策（2026-09-08）

`CS-T0-05-05` 已发布 create-new [机器决策 v1](../../../../../../cs-t0/coop-virtual-device-decision-v1.json)、[独立逆向交接](../07-virtual-device/virtual-device-static-reverse-v1.md)和[独立验收报告](../07-virtual-device/virtual-device-decision-test-results-v1.md)。固定产品目标是：Endpoint B 必须在 Authority 没有第二个实体手柄时持续工作。现有 adapter 需要 selected-device key 解析到具体 `InputDevice`，并经过 active device vector、physical poll、logical `+0x30/+0x34/+0x38` query 与 Player consumer 链；live-11 同时保留物理手柄断开后 B unbound 的既有因果证据。因此 `necessityAssessment` 固定为 `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`。

必要性不等于实现资格。virtual device 的对象 owner、创建/析构、引用计数、调用线程、device key、vector 移除所有权、完整 vtable/COL/slot ABI、assignment 集成、generation lifecycle 与 teardown/drain/restore 共 10 项仍为 `UNKNOWN`，所以 `formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，不得填写 `REQUIRED_ELIGIBLE`。05-04 的原生 Join 创建/发布 B 只按 `DESIGN_ONLY` 用户假设引用，不提高证据等级；plan v10、join progress v4、历史 artifact 和 05-04 严格状态均未改写。

新[只读验证入口](../../../../../../cs-t0/verify-coop-virtual-device-decision.ps1)只读取固定磁盘 PE、决策 JSON 和 13 份哈希绑定前序。成功输出同时包含 `verificationStatus=PASS` 与 `workPackageStatus=BLOCKED_BY_EVIDENCE`，只表示阻断结论完整真实，不表示 05-05 通过。独立验收按用户要求压缩为四项；未运行 Cargo 全量、完整 T0 build、Debug/Release Native 矩阵、历史宽回归或重复 self-test。

本轮未实现 virtual device，未启动游戏、未枚举/打开游戏进程、未创建 IPC、未注入，也未申请或消费 active 授权。决策中的 18 项权限全为 `false`；`csT00506Unlocked=false`、`t005Unlocked=false`、`t006Unlocked=false`。

## 23. Virtual Device owner / 创建析构 / 引用计数证明审计（2026-09-08）

按用户指定，05-05 首先只读审计 `VIRTUAL_DEVICE_OBJECT_OWNER`、`VIRTUAL_DEVICE_CREATION_DESTRUCTION` 与 `VIRTUAL_DEVICE_REFERENCE_COUNTING`。独立 REVERSE 的[交接 v1](../07-virtual-device/virtual-device-ownership-lifecycle-reverse-v1.md)闭合了原生正常子链：`0x18` control block、`strong=1/weak=1`、六槽 strong/weak ABI、last-strong inline deleting destructor、last-weak control-block free，以及 manager vector 的正常 copy/grow/erase/stop/destructor 引用释放路径。`NetInputDevice` 仅作 OriginalOnline ABI 参照，不进入 local C/S 正向路径。

三项完整证明均被最小反例阻断。owner 方面，insert/remove callbacks 接收 shared pair，静态证据无法排除回调保留强引用，也不能确定唯一 remove/final-release authority。创建/析构方面，control-block allocation 的 null 分支仍可把 `{object,null-control}` 送入 insert，且 ctor member failure、insert/grow/callback 异常、早退、rollback 与 exactly-once cleanup 未全称闭合。引用计数方面，最后 strong release 在调用线程同步析构对象；该线程/phase 未受约束，`uint16` 计数也没有已证的 overflow/underflow/double-release 排除。

create-new [ownership/lifecycle decision v2](../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v2.json)因此记录 `promotionCount=0`、`promotedIds=[]`、`totalStaticCount=6`、`totalUnknownCount=10`。独立[验收报告](../07-virtual-device/virtual-device-ownership-lifecycle-test-results-v1.md)确认固定 PE、8 个前序哈希和 17 个锚点；真实 create-new 与 verify-existing 各一次退出 0，但成功只验证保守 UNKNOWN 结论，不表示三项证明通过。

本轮未实现 virtual device，未运行 v1 suite、Cargo 全量、完整 T0 build、Native 矩阵或历史宽回归；未启动/枚举/打开游戏进程，未创建 IPC、Hook 或输入，也未申请/消费 active 授权。18 项权限和三项 unlock 继续全为 `false`。

## 24. Virtual Device 生命周期离线实现与剩余静态闭合 v4（2026-09-08）

[决策 v4](../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json)现为 05-05 正式入口。独立 `Release|Win32` Application 实现七态 `VirtualDeviceLifecycleCoordinator`、move-only `ReaderLease`/`CallbackLease`、六类 strong-ref 上限六、external weak 零、authority-thread/drain/vector-absence 顺序与 13 个 failure/rollback 路径；它未加入 `IsaacCsT0.Native.vcxproj`、完整 `build.ps1` 或任何游戏加载路径。最终 group 1 报告为 16 cases、332 assertions、13/13 rollback、0 failures。

独立 [REVERSE v2](../07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md)确认固定 delegate 目标不保留 callback pair，并补齐 13 个静态锚点；但两个 keyed-lookup vtable 间接入口后的 alias universe、真实非 OriginalOnline virtual factory/constructor rollback 图、所有间接 release caller 与 last-release thread 仍无法全称闭合。因此对象 owner、创建/析构、引用计数三行只得到 `offlineImplementationStatus=OFFLINE_IMPLEMENTATION_CONFIRMED`；`nativeEvidenceStatus=UNKNOWN`、`formalStatus=UNKNOWN`，其余七项也继续 `UNKNOWN`，总计仍为 6 static / 10 unknown、`promotionCount=0`。

[最终独立 TEST 报告](../07-virtual-device/virtual-device-ownership-lifecycle-v4-test-results-v1.md)保留了全部单次执行历史：SDK 访问失败、旧 callback 子序列断言、fixed-goal 候选漂移、deny-list 自匹配和首轮负向批次失败均有独立 create-new 记录且未重跑。最终封印下，group 1、49-anchor/18-binding create-new、verify-existing 和一个 10-method 参数化负向批次全部 PASS；最终验证输出为 `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json`。

该成功仅确认离线代码自身义务和保守决策完整，不表示真实 virtual device 已实现。[下一步 v1](../07-virtual-device/virtual-device-native-evidence-closure-next-step-v1.md)只安排 alias universe、真实 factory/rollback 与 last-release caller/thread 三条原生静态闭合，并明确当前不进行注入。`virtualDeviceImplemented=false`、`implementationScope=OFFLINE_NOT_LINKED_NOT_LOADED`、`formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`；18 项权限、全部 runtime/process/game/IPC/hook/input/world 字段和 05-06/T0-05/T0-06 unlock 均为 `false`。未运行 Cargo、完整 T0 build、生产 Native/Debug 矩阵、历史宽回归、游戏或实机实验。

## 25. Native Alias Universe 可审计保守裁决 v5（2026-09-09）

本轮严格只执行 `NATIVE_ALIAS_CLOSURE`。独立 TEST 先封印验收，再由不同 REVERSE 角色只读处理固定 Win32/x86 PE，主线最后集成 [native alias universe v2](../../../../../../cs-t0/native-alias-universe-v2.json)、[decision v5](../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json)和[公开 verifier v5](../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v5.ps1)。v1 manifest、REVERSE v1、v4 decision/harness 和全部历史失败文件保持原哈希，不删除、不覆盖。

receiver census 从 keyed lookup RVA `0x00620390`、vtable cells `0x0078281C/0x00782978` 和 28 个 direct callsites 出发。whole-image 35,189 个 indirect calls 精确分区为 489 个 base-only、270 个 split-register 和 34,430 个 residual 行；另枚举 3,810 个 indirect-tail frontiers。receiver graph 为 18 nodes / 24 edges，frontier 总数 38,985；service-global 另保留 1 个 Manager-argument escape。返回 `{object,control}` pair 有 35 个 seeds，仅 2 个展开；pair graph 为 10 nodes / 8 edges，保留 3 个 unresolved calls、33 个 liveouts、3 个 escapes 和 1 个 exception edge，pair frontier 共 37。

REVERSE 对 preseal 点名的 21 个 residual provenance 逐条裁决：它们都是 IDA noncontiguous tail/chunk，不能用远端 owner main range 冒充包含当前指令的 half-open range，因此全部降为精确 `UNRESOLVED_IDA_CODE_REGION`，并移除 10 个不再引用的孤儿 range。最终 function ranges 为 8,941，residual provenance 为 33,960 identified / 470 unresolved。该修正不减少 frontier，也不改变结论。

最终 [独立 TEST v12](../07-virtual-device/virtual-device-native-alias-closure-test-results-v12.md)按压缩计划只执行四组且各一次：internal oracle、public create-new、public verify-existing、仅两个 test methods 的参数化负向批次，全部 PASS。39 项 path/length/UTC/SHA 快照在 verify-existing 前后相同；正式 [verification retry-v11](../../../../../../out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json) SHA-256 为 `51191D57704C25E643688BB8698309D250E2B16A4717F9275CD275D4D25EB360`。此前 v1–v11 的 fail-fast 结果均作为 append-only 证据保留，后续组在各次 Group 1 失败后没有运行。

验收通过的是保守阻断决策，不是 universe promotion。`receiverDispatchFixedPointClosed=false`、`pairAliasFixedPointClosed=false`、`dynamicAliasUniverseClosed=false`、`allAliasesBoundedAndReleasedBeforeManagerRemove=false`、`nativeOwnerHeldThroughDrain=false`。首个不可排除边为函数 `0x00023CC0` 内 RVA `0x00024037` 的 `FF D0`：control release 后通过 callback slot RVA `0x0087E814` 调用未知目标，无法排除 pair alias 被保留。故 `VIRTUAL_DEVICE_OBJECT_OWNER` 继续 `UNKNOWN`，`formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，全部权限与 05-06/T0-05/T0-06 unlock 仍为 `false`。

真实非 OriginalOnline virtual factory/constructor rollback 图，以及 reference-count/final-release caller universe 与 stop/destructor thread provenance，仍是后续两个独立工作；本轮没有提前开始。未运行 v4 offline harness、Cargo 全量、完整 T0 build、Debug/Release Native 矩阵、历史宽回归或重复 self-test，也未启动游戏、调试器、IPC、Hook、输入或世界修改。

## 26. Native Alias 固定点清零 Batch 1 v6（2026-09-09）

本批仍只执行 `NATIVE_ALIAS_CLOSURE`。独立 [TEST preseal](../07-virtual-device/virtual-device-native-alias-fixed-point-clearing-batch1-test-plan-v1.md)先封印四组压缩验收，不同 [REVERSE 角色](../07-virtual-device/virtual-device-native-alias-fixed-point-clearing-batch1-reverse-v1.md)再只读复核固定 PE/IDB/objdump，主线最后 create-new 集成 [native alias universe v3](../../../../../../cs-t0/native-alias-universe-v3.json)、[decision v6](../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json)和[公开 verifier v6](../../../../../../cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1)。v1-v2 manifests、v1-v5 decisions、v1-v12 TEST/REVERSE 历史与全部既有输出保持不变。

IAT 由 PE Directory 12 独立重建为严格半开区间 `[0x00718000,0x00718A28)`，28 个 modules / 622 个非零 entries。父 residual 中 18,107 个 `FF 15` 和 tail 中 133 个 `FF 25` 按 source ID、RVA、raw bytes、function provenance、IAT slot、module/import 与 Manager-taint disposition 逐行移动到 exclusions。`_beginthreadex` 是唯一明确的 Manager nested-context transfer，但 `0x0065A570 -> 0x0067F130 -> 0x0065A760 -> 0x006220C0` 链证明 context 被取出和释放，固定 callback 不读取传入参数，因此没有新增 unresolved `IMPORT_ALIAS_ESCAPE`。

callback slot RVA `0x0087E814` 位于 `.data` virtual zero-fill tail。101 个精确 VA payload 全部是 `A1` read，decoded writer、额外 pointer payload、export/IAT/delay-IAT、relocation target、TLS/load-config 和其它 loader mutation 命中均为零；唯一邻域仿射候选 RVA `0x0003BA69` 被 CFG 约束为访问 `0x00C7E820..0x00C7E834`，不触及 slot。由此清除 3 个 conditional callback unresolved calls 及其 3 个重叠 escape 描述，pair frontier 按 identity 只减少 3，保留 33 liveouts 和 RVA `0x00023D12` 的 exception/nonlocal edge，共 34。

Manager service whole-image 固定集合只有 vptr `0x00B9EE5C/0x00B9FB88`。derived 分支 `0x0065A530 -> 0x0065A570 -> 0x0067F130 -> 0x0065A760 -> 0x006220C0` 与 base 分支 `0x00662830 -> 0x006F05D3` 均有指令终点；base 在 Manager context store 前进入 `_purecall`。因此旧 service escape 从 1 降为 0。receiver graph 从 18 nodes / 24 edges 扩为 20 / 26，剩余 frontier 是 474 base-only + 270 split-register + 16,323 residual + 3,677 indirect-tail = 20,744；首边由集合排序得到 RVA `0x00006A75`、raw `FF D7`。

[独立 TEST 结果](../07-virtual-device/virtual-device-native-alias-fixed-point-clearing-batch1-test-results-v1.md)严格将 internal oracle、public create-new、public verify-existing 和一个仅两个 test methods 的参数化负向批次各运行一次，全部 PASS；72-file byte snapshot 在 verify-existing 前后相同。正式 [verification v6](../../../../../../out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch1-verification-20260909-v6.json) SHA-256 为 `301D91F8812A82B5F64052C3DAD27872E270F26199015123B4CCC6B19F68915D`。

两个 fixed point 仍非零，所以 `dynamicAliasUniverseClosed=false`、drainability/native-owner 仍 `UNKNOWN`、`VIRTUAL_DEVICE_OBJECT_OWNER` 不提升、`formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`；18 项权限和 05-06/T0-05/T0-06 unlock 全部为 `false`。下一批 alias 清零应从剩余 20,744 receiver 与 34 pair 的实际分桶继续；factory/rollback 和 reference-count/final-release caller/thread 仍未开始。本批未运行 v4 harness、Cargo、完整 build、Native matrix、宽回归、游戏、调试器或任何动态测试。
