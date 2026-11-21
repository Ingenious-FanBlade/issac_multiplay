> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 2 独立 TEST 预封 v1

## 1. 身份与作者隔离

- 状态：`PRESEALED_BEFORE_CANDIDATE`。
- 范围：仅验收 `NATIVE_ALIAS_CLOSURE / Batch 2` 的 register-mediated IAT call/tail 与固定映像内 intraprocedural switch-table tail；不验收 base-only、split-register、pair liveout/exception、factory/rollback、reference-count、final-release/thread、05-06、T0-05 或 T0-06。
- 本文件由独立 TEST 角色在 `native-alias-universe-v4.json`、ownership/lifecycle decision v7、v7 verifier、oracle、负向批次和本批 REVERSE 交接产生前 create-new。TEST 不承担逆向或实现；REVERSE 不修改本封印或验收自身结论；主智能体只在两类作者交接后集成。
- oracle 必须由 TEST 独立实现，不得导入、调用、复制 verifier/generator 的证明实现，也不得把候选自报计数、分类或 closure boolean 当作 oracle。
- 本批只读固定磁盘 PE、当前只读 IDB 和独立 `objdump`；不得启动、枚举或访问游戏进程，不得使用 x32dbg、注入、IPC、Hook、进程探针或保存 IDB。

## 2. 不可变基线与候选身份

- 固定目标：Win32/x86 `isaac-ng.exe`；本机固定路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`；SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 独立反汇编器：`D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe`；SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`。
- 父 manifest：`cs-t0/native-alias-universe-v3.json`；SHA-256 `CD2A6BADFACAECD1FC1A147199CCC24B433F679A34FEF8C223A893E5A3621737`。
- 父 decision：`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json`；SHA-256 `65C9EE330F4FBCD51CC2D88EDB32283A0E61FA09254012D8F5E9DB21044BAA50`。
- 当前父集合必须由 v3 行级数组重建：receiver frontier `20,744`，由 base-only `474`、split-register `270`、residual call `16,323`、indirect tail `3,677` 构成；pair frontier `34`，由 `33` 个 liveout 与 RVA `0x00023D12` 的唯一 exception/nonlocal identity 构成。
- 计划给出的 `2,333` 个 register-mediated IAT 与 `763` 个 raw `FF 24` absolute-indexed switch syntactic candidates 只是待独立重建的机械 census，不是预设清除量。任一 census 不一致、重叠、遗漏或不能与父 row identity 一一对应时，Group 1 失败；不得修改候选规则或硬编码清除量来贴合计划。
- 本批候选必须 create-new：`cs-t0/native-alias-universe-v4.json`、`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v7.json`、`cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v7.ps1`、v7 verifier、独立 Batch 2 oracle、恰好两个 test methods 的负向批次、本批 REVERSE 交接和最终 TEST 结果。
- v1-v3 manifests、v1-v6 decisions、全部历史 TEST/REVERSE 和现有输出不得覆盖、删除、改名或重写；v7 必须哈希绑定 v3、v6、本 preseal、本批 REVERSE、v4、wrapper、verifier、oracle 和负向批次的磁盘字节。

## 3. 共同状态与集合不变量

四组均须验证：

1. 父 row `id` 是唯一集合 identity。resolved exclusion、保留 frontier、新 exact frontier 与 import escape 必须互斥；同一父 row 不得遗漏、重复、重编号或同时出现在 resolved 与 unresolved 集合。
2. receiver 集合只能按实际行级结果计算：

```text
receiver_v4 = 20,744
  - resolvedRegisterIatCallRows
  - resolvedRegisterIatTailRows
  - provenIntraproceduralSwitchRows
  + registerMediatedImportAliasEscapes
  + newlyDiscoveredExactFrontiers
```

   各减项必须是父 receiver identity 的互斥子集；新增项必须有新 identity、源 RVA、raw bytes、函数 provenance 和不与父 identity 重叠的理由。失败候选保留原父 row，不得用附加诊断 row 重复计数。
3. base-only `474`、split-register `270` 以及 pair graph、nodes、edges、34 个 canonical frontier identities 必须逐字继承 v3。本批不允许通过改写 pair、base/split 分类降低 frontier。
4. 首个 receiver/universe 未闭边必须从最终 canonical row 排序产生。RVA `0x00007455` 仅是 `0x00006A75` 被实际清除且无更早新增边时的诊断预期，不能硬编码。
5. 两个 fixed point 本批结束时仍非零，因此必须保持 `receiverDispatchFixedPointClosed=false`、`pairAliasFixedPointClosed=false`、`dynamicAliasUniverseClosed=false`、drainability/native-owner 为 `UNKNOWN`、`VIRTUAL_DEVICE_OBJECT_OWNER` 不提升、`formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，18 项权限和所有 unlock 为 false。
6. v7 必须逐字继承 v6 的 pair、factory/rollback、reference-count、final-release/thread、离线 lifecycle、权限与解锁结论。新增精确边或 import alias escape 必须保留，不能为维持单调下降或 promotion 而隐藏。

## 4. 压缩执行序列

严格只执行以下四组，各一次，依次进行；任一组失败立即停止，不运行后续组，不在同一轮重试，也不修订本 preseal 适配候选。

### Group 1 — 独立固定 PE/objdump oracle

一次运行独立 oracle，从固定 PE、独立反汇编和父行重建以下事实。

#### 1A. Register-mediated IAT all-path reaching definition

- 从 v3 residual call/tail 父行独立重建 register-target 与先行 IAT-load 机械 census，并确认计划声称的 `2,333`；不得读取 v4 的候选计数作为输入。
- 每条 resolved 行必须在同一 containing function 的完整 CFG 上，对到达间接 call/tail 的所有可达 predecessor 求 reaching definitions。目标寄存器必须在每条路径上唯一追溯到同一 IAT thunk 的**内容读取**，且该 slot 严格位于 `[0x00718000,0x00718A28)`、为非零 import thunk，并能由 Import Directory 唯一解析 module/import。
- 允许：32-bit IAT content load、完整寄存器复制，以及经完整 CFG 证明无覆盖/alias 的稳定 spill/reload。拒绝并保留父 frontier：alternate/undefined definition、LEA 或 thunk 地址 materialization、partial-register write、算术/指针构造、未知 predecessor/chunk、stack pivot、可能 alias 的 spill、未建模 call 或任意 clobber。
- 普通调用后 EAX/ECX/EDX 一律 caller-clobbered；EBX/ESI/EDI 只有在绑定的 Win32/x86 ABI 与具体 callee/CFG 都支持时才保留。EBP/ESP 参与目标或 spill provenance 时必须证明 frame/stack 稳定，不能凭惯例推断。
- resolved 行须保存父 ID/RVA/raw/function hash、target register、定义/复制/spill 链、IAT slot、module/import、all-path predecessor coverage 和 kill/clobber 证据。特别独立重放 RVA `0x000069FE` 的 load、slot RVA `0x00718884`、`_wassert` import 与 RVA `0x00006A75` 的 `FF D7`；是否清除只能由上述 all-path proof 决定。
- target 被识别为 import 之后，另行从 Manager source 追踪到 call/tail 边界的栈参数、寄存器参数、返回值和 callback context。没有跨界 alias 才可进入 register-IAT exclusion；有跨界 alias 时必须保留原 transfer 依据并创建 `IMPORT_ALIAS_ESCAPE`，不能因 import 名称判安全。

#### 1B. `FF 24` intraprocedural switch-table tail

- 从 v3 tail 父行和固定 `.text` 独立重建 raw `FF 24` absolute-indexed syntactic census `763`，并逐条绑定父 identity；不得以 v4 的 switch count 或 resolved flag为 oracle。
- 只有所有函数入口/可达 predecessor 都给 index 一个有限、无符号/有符号语义明确、无整数回绕的 domain，才读取 table。guard dominance、比较方向、default edge、scale、base、entry width、table 半开范围和每个 reachable index 必须逐项闭合。
- 每个 table cell 必须从磁盘 PE 重建，并检查 section/raw backing、relocation 与 image-base 语义；每个 target 必须落在 executable instruction boundary，且属于同一 containing function 或已明确归属的同函数 chunk CFG。未知/坏 cell、非指令边界、跨函数 target 或隐藏 successor 均保留父 tail frontier。
- table 若可写，必须另外穷尽固定映像内 writer、地址逃逸与全部 loader-mutated directories；任一未知 writer/escape/mutation 阻止 switch 排除。
- 已证明的 successor 必须进入 receiver graph 并继续 Manager taint；只有全部 successor 对当前 tail 都是同函数 CFG 分支且未形成新 alias edge，父 row 才可改判 `PROVEN_INTRAPROCEDURAL_SWITCH_BRANCH`。失败行只附失败理由并保留原 identity。

#### 1C. 差分、图和状态

- 独立复算 register-IAT call/tail resolved 集、switch resolved 集、import alias escapes、新 exact frontiers、最终 receiver rows、nodes/edges、零遗漏/重复/未分类和首边。
- 独立核对 v4/v7 的 artifact hashes、raw/function anchors、父继承字段及第 3 节全部状态耦合。实际清除可小于机械 census，甚至为零或伴随新增 frontier；这样的保守结果允许通过。

### Group 2 — v7 公开 CLI create-new

仅运行一次：

```powershell
pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v7.ps1 `
  -GameExe '<fixed-isaac-ng.exe>' `
  -Objdump '<fixed-objdump.exe>' `
  -RepositoryRoot . `
  -Output '<one-absent-output.json>'
```

公开参数面只能含必需的 `-GameExe`、`-Objdump`、`-RepositoryRoot` 和可选 `-Output`。入口必须先完成全部验证，再 exclusive-create 仓库内既定输出目录下唯一不存在的 `.json`；失败不得留下输出或修改仓库。输出数量、首边、hash 和阻断状态必须与 Group 1 一致。

### Group 3 — v7 verify-existing 与不可变性

- 仅运行一次同一公开 CLI，不给 `-Output`；它必须重放固定 PE/objdump、v4/v7 hashes、完整集合和状态耦合，不能只解析 JSON 或信任 success 字段。
- 调用前后逐字节快照 v1-v4 manifests、v1-v7 decisions、全部历史 TEST/REVERSE、已有输出、Group 2 输出及工作树状态。path、length、SHA-256 与 Group 2 输出均须不变；mtime 或计数不能替代内容校验。

### Group 4 — 一个参数化负向批次

共用一个临时 fixture，只运行一次测试入口；测试源严格恰好两个 test methods：

1. `test_semantic_mutations`：参数化覆盖 alternate/missing reaching definition、load/LEA 混淆、partial-register 写、call clobber、错误 IAT 边界/slot/module/import、遗漏 Manager argument/return/callback escape、重复/重叠 exclusion、错误 switch guard/dominance/signedness/domain/table length、writer/address escape/loader relocation 漂移、坏 cell、非法或跨函数 target、隐藏 successor/frontier、伪造计数/closure/promotion、父 v3/v6/preseal/REVERSE/anchor/tool hash 漂移、pair/base/split/非 alias 父结论及权限/解锁漂移。
2. `test_write_boundaries`：参数化覆盖已有输出、输出与输入/历史/仓库文件的 case/reparse/hardlink/path alias、`..`、ADS、设备名、错误扩展、越权目录、错误 PE、错误 objdump 和任意写前验证失败。每例必须 fail closed，且 fixture、仓库与全部已有输出字节不变。

参数化 subcase 可以在 method 内单独报告，但不得拆成更多 test methods、重复运行 positive CLI 或为每个 mutation 建重复 fixture。

## 5. 明确不运行与通过含义

- 不运行 v4 lifecycle harness、Cargo 全量、完整 T0 build、Native Debug/Release/LTCG 矩阵、历史宽回归、重复 self-test 或重复 positive CLI。
- 不运行游戏、x32dbg、调试器、进程枚举/读取、注入、IPC、Hook、输入或任何动态测试。
- 最终 TEST 报告必须逐组记录唯一命令、UTC 起止、退出码、PASS/FAIL、持久化输出与 SHA-256；Group 1 失败时其后组必须明确标为未运行。
- 通过仅表示 Batch 2 的两类机械裁决、父 identity 差分和保守阻断决策可独立复核；不表示 Alias Universe 完整闭合、alias 可在 remove 前 drain、virtual device 已获实现资格或任何运行时权限已开启。
