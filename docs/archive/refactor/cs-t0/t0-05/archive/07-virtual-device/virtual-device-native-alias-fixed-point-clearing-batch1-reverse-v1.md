> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 1 REVERSE 交接 v1

## 1. 身份、结论和边界

- identity：`IsaacCsT0-NativeAliasFixedPointClearingBatch1Reverse-v1`。
- 日期：2026-09-09。
- 角色：独立 `REVERSE`；验收条件已由独立 TEST 在候选产生前封印。
- 范围：仅 `NATIVE-ALIAS-CLOSURE / Batch 1`。本轮不研究 factory/rollback、reference-count、final-release/thread，也不授予 05-06、T0-05 或 T0-06 权限。
- TEST preseal：`virtual-device-native-alias-fixed-point-clearing-batch1-test-plan-v1.md`，SHA-256 `06A1E0CFEAA8156EF1B23FFAF4183CBBAA46D17B9858D96DE192817859656637`。
- 父 manifest：`cs-t0/native-alias-universe-v2.json`，SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`。
- 父 decision：`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json`，SHA-256 `E80C7873C0499F5FD920E7D10472E4C6CB7E82157F459DEB356A745E83987448`。
- 固定目标：Win32/x86 `isaac-ng.exe`，preferred image base `0x00400000`，SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 当前只读 IDB：同一 PE 的 `isaac-ng.exe.i64`；IDA 报告 image base `0x00400000`、image size `0x008FE000`、`.text VA [0x00401000,0x00B18000)`、`.idata VA [0x00B18000,0x00B18A28)`。
- 独立解码器：GNU objdump 2.44，SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`；所有文中 `objdump` 指令文本来自 `-d -w`，PE 目录来自 `-p/-h`。
- 地址规则：文中主定位均为 RVA；objdump 显示 VA 时减 `0x00400000`。`.text` 内 RVA 到文件偏移为 `fileOffset = RVA - 0x00000C00`。
- 最终结论：Batch 1 可从父 receiver frontier 精确清除 `18,107` 条 IAT `FF 15`、`133` 条 IAT `FF 25` 和唯一 service escape；可从 pair frontier 清除 3 条同一 callback-slot 条件边。新的 receiver frontier 为 `20,744`，pair frontier 为 `34`。两个集合仍非零，所以总体 verdict 保持 `UNKNOWN`，`dynamicAliasUniverseClosed=false`，对象 owner 不提升，所有权限和下游解锁继续为 false。

本文是静态事实交接，不是最终 TEST 报告。没有把计数、导入函数名或自报布尔当作固定点完整性的替代物。

## 2. 工具和只读操作

本轮只执行下列静态操作：

1. 完整读取 C/S 架构、T0 任务书、`cs-t0/README.md`、`isaac-cs-refactor` 技能和已封印 TEST。
2. 对固定 PE 使用 `objdump -p/-h` 复核 section 与全部 PE data directory；使用 `objdump -d -w` 流式解码整个 `.text`，并单独裁决 `FF 15`、`FF 25`、callback slot 和 service 路径。
3. 使用当前 IDA 数据库只读查询包含函数、函数边界和目标函数反汇编；没有重命名、改类型、patch、保存或重分析 IDB。
4. 对磁盘 PE 原始字节计算函数范围 SHA-256，并扫描 callback VA/RVA payload 与 base-relocation target。

未运行 verifier 或任何 TEST；未启动、枚举、打开或探测游戏进程；未使用 x32dbg/调试器、注入、IPC、RPM 或动态实验；未修改 v1-v2 alias manifest、v1-v5 decision、TEST、v3/v6 候选或生产代码。

## 3. IAT import map 与 `FF 15` / `FF 25` 行级裁决

### 3.1 独立 import map

PE Data Directory 12 给出：

```text
IAT RVA  = 0x00718000
IAT size = 0x00000A28
range    = [0x00718000, 0x00718A28)
```

这是严格半开区间；`0x00718A28` 不属于 IAT。Import Directory 位于 RVA `0x007F3088`、size `0x244`。独立解析得到 28 个 import descriptor/module、622 个非零导入 thunk 和 28 个每模块终止零槽，合计恰好覆盖 `0xA28 / 4 = 650` 个 dword；没有把相邻 `.rdata` 或 `.data` 位置算入 import map。Delay Import 与 Bound Import directory 均为零。

### 3.2 whole-`.text` 解码计数

以“objdump 行首地址 + 原始 opcode + 绝对 dword operand”逐行解析，结果为：

| opcode | whole `.text` | operand 落入严格 IAT 半开区间 | IAT 外 |
| --- | ---: | ---: | ---: |
| `FF 15 imm32` indirect call | 18,647 | 18,107 | 540 |
| `FF 25 imm32` indirect tail transfer | 139 | 133 | 6 |

父 v2 的 residual-call/tail 行与独立解码以 `(instructionRva, rawBytes, decoded absolute slot)` 一一对应。Batch 1 的机器候选应保留父 source id、函数 provenance，并为每行加入由 import map 解析出的 `iatSlotRva/module/import`；不得仅按 opcode 或导入名聚合。

行级确定性裁决规则如下：

1. operand 必须落在 `[0x00718000,0x00718A28)`，并命中一个非零导入 thunk；零终止槽、end-exclusive `0x00718A28` 和 IAT 外绝对 cell 均拒绝 import exclusion。
2. `FF 15` 和 `FF 25` 只排除“callee 是 Manager keyed lookup/等价 `slot +0x28` dispatch”。
3. 另以 Manager singleton VA `0x00C57B18` 为 taint source，追踪寄存器、稳定 spill、pending stack arguments 和已识别 context fields；同寄存器/slot 非 tainted 覆盖、`pop` 和 ABI caller-saved clobber 均 kill。taint 到达 import argument、返回或 callback context 时必须独立裁决，不因 import identity 自动清除。
4. 该规则对 18,240 条 IAT transfer 的结果是：18,106 条 `FF 15` 没有 Manager alias 跨越 import 边界，133 条 `FF 25` 没有 live Manager alias 进入 tail import；唯一明确 tainted import 是下面的 `_beginthreadex`，但其有限 context 链已完全解析，故新增 `IMPORT_ALIAS_ESCAPE` 为零。

### 3.3 唯一 tainted import：`_beginthreadex`

- IAT slot：RVA `0x00718854` / VA `0x00B18854`。
- on-disk thunk bytes：`E0 72 7F 00`。
- import：`api-ms-win-crt-runtime-l1-1-0.dll!_beginthreadex`，hint `0x0015`。
- call instruction：RVA `0x0065A663`，bytes `FF 15 54 88 B1 00`，`call *0xb18854`。
- disposition：`RESOLVED_MANAGER_NESTED_CONTEXT_TRANSFER`，不是 `IMPORT_ALIAS_ESCAPE`。

其证明链不是依赖导入函数名，而是依赖下列原始数据流：

```text
RVA 0x0065A5EF..0x0065A5FD
8B 4D 08 89 4B 04 8B 4D 18 89 33 89 4B 08
mov [ebp+8],ecx; mov ecx,[ebx+4]      ; callback
mov [ebp+0x18],ecx; mov esi,[ebx]
mov ecx,[ebx+8]                       ; Manager context

RVA 0x0065A640..0x0065A663
8D 4F 24 89 45 08 51 6A 00 50 68 30 F1 A7 00 6A 00
C7 00 60 A7 A5 00 89 58 04 89 78 08 6A 00 C6 47 20 00
FF 15 54 88 B1 00
```

在这一区间，EBX 从 first-context 成功分配与 `test ebx,ebx` 非零分支后保持为同一对象；中间的 Win32 critical-section/Sleep imports 不覆盖 callee-saved EBX。`mov ebx,[eax+4]` 把同一 first-context 放入 second-context，start routine 固定为 RVA `0x0067F130`。

```text
RVA 0x0067F159..0x0067F168
8B 7D 08 C7 45 FC 00 00 00 00 FF 77 04 8B 07 FF D0
mov [ebp+8],edi; ...; push [edi+4]; mov [edi],eax; call eax
```

second-context `+4` 被传给固定 trampoline RVA `0x0065A760`。trampoline 从 incoming first-context 读取 `+4` callback 和 `+8` Manager context，并在 RVA `0x0065A781..0x0065A785` 执行 `push esi; call edi`。固定 callback RVA `0x006220C0` 的完整 46-byte 函数没有读取 `[ebp+8]` 或进入参数，故 Manager 到这里终止，不发生 retain/copy/store/return。

## 4. callback slot RVA `0x0087E814`

### 4.1 section 与零填充

PE section 事实为：

```text
.data RVA                  0x007F8000
.data VirtualSize          0x000A4AA4
.data virtual end          0x0089CAA4
.data SizeOfRawData        0x00069E00
.data raw-backed end       0x00861E00
callback slot RVA          0x0087E814
```

因此 slot 落在 `.data` 的 virtual zero-fill tail，磁盘上没有 raw backing；初始 loader-visible 值为零。

### 4.2 direct/payload/affine census

- whole-file 精确 VA payload `14 E8 C7 00` 恰有 101 处。
- 101 处全部位于 `.text`，每处前一 byte 都是 `A1`，独立解码全部为 `mov 0xc7e814,%eax`；全部 101 个 operand field 也都有对应 HIGHLOW relocation。
- whole-file 精确 RVA payload `14 E8 87 00` 为 0。
- 因而精确地址常量没有落入数据 pointer payload、IAT、export payload 或其它 address-bearing 数据；101 个命中恰好就是 101 个 direct reads。
- decoded direct writes 为 0；没有 `mov/lea/push` 把 slot 地址本身传出。

邻近绝对地址解码仅出现一条需要单独排除的仿射候选：

```text
RVA 0x0003BA69
8B 04 85 1C E8 C7 00
mov 0xc7e81c(,%eax,4),%eax
```

同一 CFG 先在 RVA `0x0003B946` 从 `[edi+0xD7C]` 读取 EAX，`jle` 排除 `<=0`，随后 `cmp eax,6; ja` 排除 `>6`；到 RVA `0x0003BA5D` 只重新读取同一未被中间路径写入的字段。因此该访问的 EAX 为 `1..6`，有效地址为 `0x00C7E820..0x00C7E834`，不能下溢到 `0x00C7E814`。同邻域以 `0x00C7E820` 为基址的 shuffle 索引也被循环边界限制在非负 `0..5`。该仿射族不读、不写、也不 materialize callback slot。

### 4.3 loader directory 与 relocation 闭合

固定 PE 16 个 data directories 的实际值如下：

| index | directory | RVA | size | 与 slot 的结论 |
| ---: | --- | --- | --- | --- |
| 0 | Export | `0x007D5D70` | `0x0001D318` | 3,226 个 EAT function RVA 中 slot 命中 0 |
| 1 | Import | `0x007F3088` | `0x00000244` | descriptor 只构造上节 IAT；不覆盖 slot |
| 2 | Resource | `0x0089D000` | `0x00003B20` | 不覆盖 slot，不是 loader write target |
| 3 | Exception | `0` | `0` | 空 |
| 4 | Security | `0` | `0` | 空 |
| 5 | Base Relocation | `0x008A1000` | `0x0005C93C` | 见下方逐项 target census |
| 6 | Debug | `0x007ACCCC` | `0x54` | 不覆盖 slot |
| 7 | Architecture | `0` | `0` | 空 |
| 8 | GlobalPtr | `0` | `0` | 空 |
| 9 | TLS | `0x007ACE00` | `0x18` | TLS callback 仅 VA `0x00AEFEC1`；不含 slot |
| 10 | Load Config | `0x007ACD20` | `0x40` | 不覆盖 slot；精确 VA/RVA payload census 无 slot data pointer |
| 11 | Bound Import | `0` | `0` | 空 |
| 12 | IAT | `0x00718000` | `0x00000A28` | 严格半开区间远离 slot |
| 13 | Delay Import | `0` | `0` | 空 |
| 14 | CLR | `0` | `0` | 空 |
| 15 | Reserved | `0` | `0` | 空 |

Base Relocation 独立解析得到 180,939 个非零 relocation targets，均为 type 3/HIGHLOW；另有 915 个 type 0 padding entries。target RVA `0x0087E814` 命中为 0，因此 loader 不会把 image-base delta 写入 zero-filled slot。反向检查显示上述 101 个 callback VA operand field 恰好全部 relocation-backed，这与它们是代码中的绝对读 operand 一致，并未产生额外 address payload。

因此在 preseal 定义的“固定 PE + PE loader directories + 固定映像内可达地址逃逸”闭世界中：

```text
callback initial value       = 0
decoded reads                = 101
decoded writes               = 0
exact/affine address escapes = 0
loader mutations             = 0
```

101 个 `test eax,eax` 后的条件 callback 在固定映像中不可达。pair 中 RVA `0x00024037`、`0x00024095`、`0x000242F1` 的 3 个 unresolved calls 以及与其重叠的 3 个 escape 描述可以同时清除，但集合只减 3，不能把 call/escape 重复减计。保留项为 33 个 liveouts 和 RVA `0x00023D12` 的异常边，共 `34`；排序后的首边为 `0x00023D12`。

## 5. Manager service escape RVA `0x0052487E`

### 5.1 service storage 与 vptr 集合

service global RVA `0x00857B64` / VA `0x00C57B64` 的 whole-image exact decoded/reference census 只有四行：

| instruction RVA | bytes | decode | disposition |
| --- | --- | --- | --- |
| `0x0000234C` | `C7 05 64 7B C5 00 5C EE B9 00` | `movl $0xb9ee5c,0xc57b64` | static-init direct vptr write |
| `0x00524863` | `A1 64 7B C5 00` | `mov 0xc57b64,%eax` | call-time vptr read |
| `0x00524868` | `B9 64 7B C5 00` | `mov $0xc57b64,%ecx` | service `this` materialization |
| `0x00716C4D` | `B9 64 7B C5 00` | `mov $0xc57b64,%ecx` | atexit destructor `this` |

destructor RVA `0x0065A4C0` 中同一 ECX→ESI alias 只有两次 vptr write：

```text
RVA 0x0065A4C7  C7 06 5C EE B9 00  movl $0xb9ee5c,(%esi)
RVA 0x0065A523  C7 06 88 FB B9 00  movl $0xb9fb88,(%esi)
```

没有第三个 service-address payload、write、escaped writer 或第三 vptr 值。故 call-time target set 即使允许 destructor 状态，也只包含这两个 vptr 的 `+0x08` cell，而不是必须假设 call 时只可能为 derived vptr。

vtable pointer cells：

| owner vtable RVA | cell | raw bytes | target RVA |
| --- | --- | --- | --- |
| `0x0079EE5C` | `+0x08` at `0x0079EE64` | `30 A5 A5 00` | `0x0065A530` derived wrapper |
| `0x0079EE5C` | `+0x04` at `0x0079EE60` | `70 A5 A5 00` | `0x0065A570` implementation |
| `0x0079FB88` | `+0x08` at `0x0079FB90` | `30 28 A6 00` | `0x00662830` base wrapper |
| `0x0079FB88` | `+0x04` at `0x0079FB8C` | `D3 05 AF 00` | `0x006F05D3` `_purecall` |

callsite 的完整 dispatch context 为：

```text
RVA 0x00524863..0x0052487E
A1 64 7B C5 00 B9 64 7B C5 00 68 18 7B C5 00
68 00 10 00 00 6A 02 68 C0 20 A2 00 FF 50 08

mov 0xc57b64,%eax
mov $0xc57b64,%ecx
push $0xc57b18                  ; Manager singleton context
push $0x1000
push $0x2
push $0xa220c0                  ; fixed callback
call *0x8(%eax)
```

### 5.2 derived branch

RVA `0x0065A530` 从完整入口证明 ECX `this` 保存到 ESI，先查询 vtable `+0x10`，随后保持参数顺序，把 wrapper 的四个参数转发到同一 service 对象的 vtable `+0x04`。derived `+0x04` 精确落到 RVA `0x0065A570`。

在 RVA `0x0065A570`：

- first-context 成功分支把 callback 保存到 `+4`、Manager 保存到 `+8`；
- callee-saved EBX 持续标识同一 first-context；
- second-context 把 trampoline `0x0065A760` 写入 `+0`、同一 EBX 写入 `+4`、thread state 写入 `+8`；
- `_beginthreadex` start routine 固定为 `0x0067F130`，argument 固定为该 second-context；
- start routine 取 second-context `+4` 并调用 `+0` trampoline；
- trampoline 从 first-context `+4/+8` 恢复 callback/Manager，并执行 `push Manager; call callback`；
- callback `0x006220C0` 完整函数不读取 incoming argument，因此 Manager alias 到此终止。

已确认的完整链为：

```text
0x0065A530 -> 0x0065A570 -> IAT _beginthreadex
           -> 0x0067F130 -> 0x0065A760 -> 0x006220C0
```

### 5.3 base branch

base `+0x08` target RVA `0x00662830` 与 derived wrapper 具有同类 forwarding 形状：ECX `this` 保存到 ESI，随后准备四个参数，并在 Manager 参数被任何 callee/store 消费前从当前 vtable 读取 `+0x04`：

```text
RVA 0x0066285F  8B 40 04  mov 0x4(%eax),%eax
RVA 0x00662862  FF 75 08  push 0x8(%ebp)     ; callback
RVA 0x00662865  FF D0     call *%eax
```

base vtable `+0x04` cell 的原始 bytes `D3 05 AF 00` 唯一解析为 RVA `0x006F05D3` `_purecall`。因此该分支在形成 first-context、存储 Manager 或跨越另一个 callee 前到达 purecall 终点；它不是未枚举的 callback/context 传播。

两条 call-time target 分支均有终点，且 service global 没有第三 vptr/write/escape，故父 `RECEIVER-ESCAPE-FRONTIER-0001` 可以清除；本批不新增 service frontier。

## 6. 函数范围哈希

哈希均从固定磁盘 PE 的精确函数范围计算：

| role | RVA range | size | file offset | SHA-256 |
| --- | --- | ---: | --- | --- |
| service caller | `[0x00524440,0x00524D0C)` | 2252 | `0x00523840` | `60B8A860A5CF06789C4389D3EAF82D19124B69A00DF1431FF73D97AEFAAEE8A1` |
| derived wrapper | `[0x0065A530,0x0065A56E)` | 62 | `0x00659930` | `79D1041D21CAA4EAB39C4874D236D17DCB871221512E2E71EA31024A6B8BC31A` |
| derived implementation | `[0x0065A570,0x0065A6E5)` | 373 | `0x00659970` | `43F9BEFDD3194CDF67036054E7A91303C2E4F5293642FBBF73741D7F4F69214A` |
| thread start | `[0x0067F130,0x0067F1CA)` | 154 | `0x0067E530` | `01BF950B0857B7DBFA26CDDBCAA293D04043370CA93C70A61F056A3E0FD0153C` |
| trampoline | `[0x0065A760,0x0065A793)` | 51 | `0x00659B60` | `21CFA62D4624C04DCB2BD8058736162F5CF81C383F98747BA984168B5C0FCDD4` |
| terminal callback | `[0x006220C0,0x006220EE)` | 46 | `0x006214C0` | `520BB72C096CEF52FFDB8F1466B1A6CF684DBF653742306777E4F2E67FBBF8A9` |
| base wrapper | `[0x00662830,0x0066286C)` | 60 | `0x00661C30` | `F492C72ECCD96637034F6535724B44ABB3F148015777FF19B53F025AB61E8EDC` |
| `_purecall` | `[0x006F05D3,0x006F05D9)` | 6 | `0x006EF9D3` | `1CBBA7E60BEE928E7E6B80FD29AB78AB26D687CD0A6DE8F02D6D2BAAD8502D55` |
| pair caller | `[0x00023CC0,0x000240AE)` | 1006 | `0x000230C0` | `587A0789C121716CE8CF3F128D839FDD9FF522F490EC164D3B4C82E0EB1B4CA7` |
| generic pair destructor | `[0x000242B0,0x00024306)` | 86 | `0x000236B0` | `DCF09DE1BE0322A06431B818D31686D8623A68AEF17FD678CEEF74644797F9C9` |

## 7. 集合差分、实际 counts 与 verdict

Receiver frontier 的父集合分解是：

```text
base-only unresolved               474
split-register unresolved          270
residual indirect calls         34,430
indirect tail transfers          3,810
Manager service escape               1
--------------------------------------
parent receiver frontier        38,985
```

Batch 1 差分为：

```text
residual calls after IAT exclusion = 34,430 - 18,107 = 16,323
tail transfers after IAT exclusion =  3,810 -    133 =  3,677
service escapes after closure      =      1 -      1 =      0

receiver frontier = 474 + 270 + 16,323 + 3,677 = 20,744
```

没有新增 `IMPORT_ALIAS_ESCAPE`、callback address/write/loader escape 或第三 service target。按 RVA 排序后的 receiver 首边为 RVA `0x00006A75`；它来自剩余真实集合排序，不是为匹配预期而硬编码。

Pair frontier 的集合语义为：

```text
33 unresolved liveouts
 3 unresolved callback calls (与 3 个 escape 描述一一重叠)
 1 unresolved exception edge
--------------------------------
37 canonical frontier identities
```

callback slot 闭合后同时移除 3 个 callback call 及其重叠 escape 描述，不能减 6：

```text
pair frontier = 33 liveouts + 1 exception edge = 34
first pair edge = RVA 0x00023D12
```

本批最终状态必须保持：

```text
receiverDispatchFixedPointClosed = false
pairAliasFixedPointClosed        = false
dynamicAliasUniverseClosed       = false
universeCompleteness             = UNKNOWN / non-zero frontiers
drainability                     = UNKNOWN
nativeOwnerThroughDrain          = UNKNOWN
VIRTUAL_DEVICE_OBJECT_OWNER      = UNKNOWN
formalDecision                   = null
implementationEligibility        = false
workPackageStatus                = BLOCKED_BY_EVIDENCE
all permissions/unlocks          = false
```

这次清零只缩小了已知 frontier，不构成完整 alias universe 固定点，也不改变 factory/rollback、reference-count、final-release/thread 的父结论。

## 8. 交接要求

主集成生成 v3/v6 时应：

1. 以父行 ID 为 identity，机器级逐行保存 18,107 个 `FF 15` exclusions 和 133 个 `FF 25` exclusions；每行绑定 raw bytes、独立 decode、containing function、IAT slot、module/import 与 manager-taint disposition。
2. `_beginthreadex` 行单列为已解析的 tainted import transfer；不得把它混成一般 `NO_MANAGER_TAINT`，也不得因 import 名称省略 context 链。
3. callback 的 101 reads、零 payload escape、0x43BA69 仿射排除和完整 loader/reloc 清单必须成为独立 oracle 可重建的 anchors/counts，不能只保存 `callbackClosed=true`。
4. service global 四个 direct references、析构两次 alias vptr write、四个 vtable cells 和 derived/base 两条终点必须进入机器集合；不得重新退化为“没有 direct xref 所以闭合”。
5. receiver `20,744` 与 pair `34` 必须由行级差分重算；任一新边应按实际 RVA 重新生成首缺口并保持保守 `UNKNOWN`。
6. TEST 仍严格只运行 preseal 的四组各一次；本 REVERSE 不替代独立 TEST。
