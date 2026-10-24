> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 1 独立 TEST 预封 v1

## 1. 身份与角色边界

- 状态：`PRESEALED_BEFORE_CANDIDATE`。
- 范围：只验收 `NATIVE-ALIAS-CLOSURE / Batch 1`，不验收或解锁 factory/rollback、reference-count、final-release/thread、05-06、T0-05 或 T0-06。
- 本文件由独立 TEST 角色在 `native-alias-universe-v3.json`、ownership/lifecycle decision v6、v6 verifier 和本批 REVERSE 交接产生前 create-new。TEST 不执行逆向、不修改候选；REVERSE 不修改本预封或作最终验收；主智能体负责实现与集成。
- 本批只读固定磁盘 PE 与独立 `objdump`，不得启动或访问游戏进程，不得使用 x32dbg、进程探针、注入、IPC 或保存 IDB。
- 闭世界仅覆盖固定 PE、其 PE loader directories 与固定映像中可达的地址逃逸；第三方注入和任意进程写不属于证明域，亦不得被写成已排除事实。

## 2. 不可变基线与 create-new 产物

- 固定目标：Win32/x86 `isaac-ng.exe`，SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 独立反汇编器 SHA-256：`4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`。
- 父 manifest：`cs-t0/native-alias-universe-v2.json`，SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`。
- 父 decision：`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json`，SHA-256 `E80C7873C0499F5FD920E7D10472E4C6CB7E82157F459DEB356A745E83987448`。
- 本批候选必须 create-new：
  - `cs-t0/native-alias-universe-v3.json`；
  - `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json`；
  - `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1`；
  - v6 verifier、独立 oracle、仅两个 test methods 的参数化负向批次；
  - 本批 create-new REVERSE 交接和最终 TEST 结果。
- v1-v2 alias manifests、v1-v5 decisions、v1-v12 TEST/REVERSE 历史、现有输出及当前未提交工作树必须保留；不得覆盖、删除、改名或通过重写历史文件制造通过。

## 3. 共同阻断不变量

四组测试都必须验证以下状态耦合，不接受自报计数或布尔值代替集合证据：

1. v3/v6 哈希绑定父 v2、父 v5、本 preseal、本批 REVERSE 交接、新增 PE anchors 和各自 verifier；绑定必须与磁盘字节一致。
2. receiver census 与 pair-transfer census 分别由行级集合运算生成；任一 seed/edge 遗漏、重复、身份冲突或未分类均失败。
3. “集合穷尽”和“所有 alias 能在 remove 前 drain”必须是两个独立字段。两个 fixed point 任一非零时：
   - `receiverDispatchFixedPointClosed=false` 或 `pairAliasFixedPointClosed=false`；
   - `dynamicAliasUniverseClosed=false`；
   - `VIRTUAL_DEVICE_OBJECT_OWNER` 不提升；
   - `formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`；
   - 全部权限和 `csT00506Unlocked`、`t005Unlocked`、`t006Unlocked` 为 `false`。
4. v6 必须逐字继承 v5 的 factory、rollback、reference-count、final-release/thread 与权限结论；本批不能用 alias 清零结果改写这些结论。
5. 出现新精确 escape/frontier 时必须保留其首个边和原始指令依据。合格输出允许 `UNKNOWN` 和非空 frontier，不允许为维持预计计数而隐藏新边或伪造 promotion。

## 4. 压缩执行序列

严格只执行下列四组，各一次；任一组失败即停止，不运行后续组，不修订本预封来适配候选，也不在同一轮重试。

### Group 1 — 固定 PE 与独立集合 oracle

一次运行独立 oracle，并从固定 PE/`objdump` 重建而非信任 v3 自报字段：

#### 1A. IAT 半开区间和逐行 exclusions

- 从 PE Import Directory 独立重建 IAT map，IAT RVA 必须严格为半开区间 `[0x00718000,0x00718A28)`；`0x00718A28` 本身及 `.data` 目标不得被归入 import exclusions。
- 父 v2 residual 中必须恰有 `18,107` 个 `FF 15` call 被逐行移动到 IAT import exclusions；indirect-tail 中必须恰有 `133` 个 `FF 25` tail transfer 被逐行移动。不得遗漏、重复，且 source ID/RVA/raw bytes/function provenance/IAT slot/module/import 必须和独立解码一致。
- 每行必须有 `managerTaintDisposition`。import target 身份只排除“目标为 keyed lookup”；若 Manager alias 作为参数、返回值或 callback context 跨越 import，则必须生成有源指令的 `IMPORT_ALIAS_ESCAPE` frontier，不能因 import 身份直接判安全。
- 新 receiver 集合必须满足精确差分：父集合减去这 `18,107 + 133` 行、再减去确实闭合的 Manager service escape、再加入实际发现的 import alias escapes 或其他精确新边。仅在没有新增 import/更早 escape 时，receiver frontier 才应为 `20,744`，首边才应为实际排序得到的 RVA `0x00006A75`；测试不得用硬编码结果替代集合排序。

#### 1B. callback slot `0x0087E814`

- 独立验证 slot 初始值为零，且位于映像的零填充语义范围；现有 `101` 个引用全部是读取。
- 穷尽直接和仿射地址引用、写入、地址常量/指针 payload、导出、IAT、delay-IAT、重定位、TLS 以及其他 loader-mutated directory。只有写入为零、地址逃逸为零且 loader 不会修改该 slot 时，`101` 个条件 callback 才可全部判为不可达。
- 仅在上述闭合成立时，精确移除 3 个 pair unresolved calls 及其 3 个重叠 escapes；不能把重叠项重复减计。pair frontier 必须由集合得到 `34`：33 个 liveouts 加 RVA `0x00023D12` 的异常边；其首边由实际排序生成。
- 若发现 writer、address escape 或 loader mutation，必须保留第一个精确反例，并保留相应 callback/pair frontier。

#### 1C. Manager service escape `0x0052487E`

- 穷尽 service vptr 仅为 `0x00B9EE5C`、`0x00B9FB88` 两值以及两者 slot `+0x08` 的实际目标，同时检查不存在第三个 vptr、未枚举写入或参数继续传播。
- derived 分支必须按原始指令闭合 `0x0065A530 -> 0x0065A570 -> 0x0067F130 -> 0x0065A760 -> 0x006220C0`：Manager context 的暂存、取出和释放均有依据，终端 callback 不读取传入参数。
- base 分支必须按原始指令闭合 `0x00662830 -> 0x006F05D3`，并证明在存储 Manager context 前到达 `_purecall` 终点。
- 只有两个分支及 call-time target set 全部闭合，才可从 `managerArgumentEscapeFrontier` 清除该唯一旧项；否则保留新的首个精确 frontier。

#### 1D. 决策和图一致性

- 独立复算 v3 receiver/pair nodes、edges、各 frontier/exclusion 数量、零遗漏/重复/未分类断言、首边及 closure booleans；v6 必须引用这些实际集合结果。
- 本批预期两个 fixed point 仍非零，因此 v6 结果必须保持 `UNKNOWN` 和第 3 节全部阻断状态。

### Group 2 — v6 公开 CLI create-new

只运行一次：

```powershell
pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1 \
  -GameExe '<fixed-isaac-ng.exe>' \
  -Objdump '<fixed-objdump.exe>' \
  -RepositoryRoot . \
  -Output '<one-absent-output.json>'
```

公开参数面只能是必需的 `-GameExe`、`-Objdump`、`-RepositoryRoot` 和可选 `-Output`。入口必须先完成全部只读验证，再以 create-new 语义写唯一不存在的 JSON；验证失败不得留下输出、临时替代物或仓库修改。输出中的数量、首边、哈希和阻断状态必须与 Group 1 独立 oracle 一致。

### Group 3 — v6 公开 CLI verify-existing 与不可变性

- 对 Group 2 的同一输出只运行一次公开 CLI，不给 `-Output`，验证 existing artifact。
- 在调用前后分别快照：仓库工作树、v1-v2 manifests、v1-v5 decisions、v1-v12 TEST/REVERSE 历史、Group 2 输出及其它已存在输出。全部字节和 SHA-256 必须不变；不得接受 mtime/计数替代内容校验。
- verify-existing 必须重放固定 PE、`objdump`、v3/v6 hashes、完整集合和状态耦合，而非只解析 JSON 或信任 Group 2 的 success 字段。

### Group 4 — 单一参数化负向批次

共用一个临时 fixture，只运行一次测试入口；测试代码严格只有两个 test methods：

1. `semantic_mutations` 参数化覆盖：IAT end-exclusive 越界、错误 module/import/slot、遗漏或重复 exclusion、错误 receiver/target/taint disposition、未跟踪 import pair transfer、callback writer/address escape/loader mutation、缺失或伪造 service derived/base 分支、alias escape/liveout/未知 callee/异常边、伪造 closure/promotion、父 v2/v5/preseal/REVERSE/anchor/verifier 哈希漂移、非 alias 父结论或权限/解锁漂移。
2. `write_boundaries` 参数化覆盖：已有输出、输出与任何输入/历史/仓库文件的路径 alias、错误 PE、错误 `objdump`、写前验证失败、仓库外或非允许输出路径。每例必须 fail closed，且 fixture、仓库和所有已有输出字节不变。

参数化 case 可以在 method 内作为 subcase 报告，但不得拆成更多 test methods、重复运行公开 positive CLI 或为每个 mutation 创建一套重复 fixture。

## 5. 明确不运行

- v4 离线 lifecycle harness；
- Cargo 全量、完整 T0 build；
- Native Debug/Release 或 LTCG 矩阵；
- 历史宽回归、重复 self-test、重复 positive CLI；
- 游戏、x32dbg、调试器、进程探针、注入、IPC 或动态测试。

最终 TEST 报告必须逐组记录唯一命令、开始/结束、PASS/FAIL、输出和 SHA-256，并明确“通过只表示本批保守集合/阻断决策可复核，不表示 Alias Universe 完整闭合或 virtual device 已获实现资格”。
