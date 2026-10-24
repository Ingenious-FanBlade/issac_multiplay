> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 2 独立 REVERSE 交接 v1

## 1. 身份、范围与结论

- 角色：独立 `REVERSE`；本文件产生于 TEST preseal 之后，不承担 TEST 验收。
- 状态：`STATIC_CONFIRMED / CONSERVATIVE_PARTIAL_CLEARING`。
- 范围：仅 `NATIVE_ALIAS_CLOSURE / Batch 2` 的 register-mediated IAT call/tail 与固定映像内 intraprocedural `FF 24` switch-table tail。
- 明确未处理：474 个 base-only、270 个 split-register、33 个 pair liveouts、RVA `0x00023D12` 的 exception/nonlocal edge、factory/rollback、reference-count、final-release/thread。
- 实际结果：register-IAT call 安全排除 `46` 行，register-IAT tail `0` 行，intraprocedural switch 安全排除 `3` 行，新增 `IMPORT_ALIAS_ESCAPE=0`，新增 exact frontier `=0`。
- receiver frontier 从 `20,744` 降至 `20,695`；两个 fixed point 仍非零，结论必须保持 `UNKNOWN`、`formalDecision=null`、owner 不提升且全部权限/解锁关闭。

本批没有把 `2,333 + 763` 机械 census 当作清除量。尚未完成逐行全路径证明的行均以 v3 父 identity 原样保留。

## 2. 固定构建与只读工具

| 证据 | 固定值 |
| --- | --- |
| PE | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` |
| PE SHA-256 | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| Machine / image base | Win32/x86 PE / `0x00400000` |
| GNU objdump | `D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe` |
| objdump SHA-256 | `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064` |
| current IDB | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`; image base `0x00400000`; input SHA-256 与固定 PE 相同 |
| parent manifest | `cs-t0/native-alias-universe-v3.json`, SHA-256 `CD2A6BADFACAECD1FC1A147199CCC24B433F679A34FEF8C223A893E5A3621737` |
| parent decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json`, SHA-256 `65C9EE330F4FBCD51CC2D88EDB32283A0E61FA09254012D8F5E9DB21044BAA50` |
| Batch 2 TEST preseal | `virtual-device-native-alias-fixed-point-clearing-batch2-test-plan-v1.md`, SHA-256 `793527BCC2864A0CFEB3E1BBD7723EFE3B5345EE56D39BEA625A64C74D585F1F` |

IDA 仅用于只读核对函数边界、basic-block predecessor/successor 与 switch successor；磁盘 PE 原始字节和固定 objdump 是可复核依据。未保存 IDB，未启动、枚举或访问游戏进程，未使用 x32dbg、注入、IPC、Hook 或动态测试。

## 3. Register-mediated IAT census 与裁决

### 3.1 独立机械 census

从固定 objdump 的 `.text` 指令和 v3 `residualIndirectCallFrontier` 父行重建，规则固定如下：

1. 只取 `containingFunctionRange.status=IDENTIFIED` 的 residual call 父行。
2. 目标只取 Win32/x86 callee-saved `EBX/ESI/EDI`。
3. 在函数线性序列第一次 `j*`、`ret*` 或 `iret(d)` 控制转移前，匹配 `mov absolute-IAT-content,%reg`；绝对 slot 必须严格位于 IAT 半开区间 `[0x00718000,0x00718A28)`。
4. 从 source 向后扫描至第一个同寄存器语法写；期间 `call *%same_reg` 且 RVA 对应 v3 residual 父行者为机械 sink。`xchg` 含该寄存器即为写；其它非 `call/push/cmp/test` 指令若末 operand 为该寄存器也视为写。
5. 没有 sink 的 source 不进入 census。此机械步骤不声称 CFG、copy/spill、taint 或 closure。

结果为 `1,075` 个 source、`549` 个 containing function、`2,333` 个唯一父 sink；register-mediated tail 为 `0`。每个 sink 与一个 v3 父 ID 一一对应，无遗漏、重复或 call/tail 重叠。

建议 v4 使用以下 canonical projection 并按 `sinkRva,parentId` 升序序列化：

```text
fields = parentId,sinkRva,rawBytes,sourceRva,targetRegister,
         iatSlotRva,functionRva,functionSha256
count = 2333
compactJsonByteLength = 632244
compactJsonSha256 = 9EA2095693515EF472F7BF970E9756BF9A2ABB29FA61F527D03880CBFBCAF019
```

### 3.2 严格 resolved 子集

本次只提升固定 slot RVA `0x00718884` 的 `_wassert` 子集：

- IAT slot VA `0x00B18884`，file offset `0x00716E84`，disk bytes `54 72 7F 00`。
- Import Directory 唯一身份：`api-ms-win-crt-runtime-l1-1-0.dll!_wassert`，hint `0x0052`，非 ordinal。
- `46` 个 sink 分布于 `16` 个函数；source 均在无前驱 entry block 的第一次控制转移之前，并为 32-bit IAT **内容读取**到 `EBX/ESI/EDI`，不是 `LEA` 或 thunk 地址 materialization。
- 对这 16 个完整 IDA CFG，entry source 支配对应 sink；所有到达 sink 的 predecessor 都经过同一 source。source 与 sink 间没有同寄存器 full/partial write、`xchg`、算术重定义、alternate/undefined definition、spill alias、stack pivot 或未知 chunk 入口。`EBX/ESI/EDI` 按绑定的 Win32/x86 ABI 为 nonvolatile；这些失败分支在 `_wassert` call 终止，不依赖 caller-clobbered `EAX/ECX/EDX`。
- 每个 sink 前的三个有效参数均为 immediate push：行号、固定 file-string VA、固定 expression-string VA；未把 Manager receiver、返回 pair、callback context 或其地址作为 `_wassert` 参数。因此这 46 行的 `managerTaintDisposition` 为 `NO_MANAGER_ALIAS_CROSSES_IMPORT_BOUNDARY`。

resolved projection：

```text
count = 46
sourceCount = 16
compactJsonByteLength = 12467
compactJsonSha256 = A5A032C081F2CDF2C6EC6A767D5BF45FC8C485631E32E2DE0EF42CC288401FA3
```

父 IDs：

```text
RESIDUAL-INDIRECT-CALL-00004,
RESIDUAL-INDIRECT-CALL-29229, RESIDUAL-INDIRECT-CALL-29230,
RESIDUAL-INDIRECT-CALL-29231, RESIDUAL-INDIRECT-CALL-29234,
RESIDUAL-INDIRECT-CALL-29235, RESIDUAL-INDIRECT-CALL-29236,
RESIDUAL-INDIRECT-CALL-29237, RESIDUAL-INDIRECT-CALL-29238,
RESIDUAL-INDIRECT-CALL-29240, RESIDUAL-INDIRECT-CALL-29241,
RESIDUAL-INDIRECT-CALL-29242, RESIDUAL-INDIRECT-CALL-29243,
RESIDUAL-INDIRECT-CALL-29245, RESIDUAL-INDIRECT-CALL-29246,
RESIDUAL-INDIRECT-CALL-29247, RESIDUAL-INDIRECT-CALL-29248,
RESIDUAL-INDIRECT-CALL-29249, RESIDUAL-INDIRECT-CALL-29251,
RESIDUAL-INDIRECT-CALL-29252, RESIDUAL-INDIRECT-CALL-29253,
RESIDUAL-INDIRECT-CALL-29254, RESIDUAL-INDIRECT-CALL-29255,
RESIDUAL-INDIRECT-CALL-29256, RESIDUAL-INDIRECT-CALL-30985,
RESIDUAL-INDIRECT-CALL-30986, RESIDUAL-INDIRECT-CALL-30987,
RESIDUAL-INDIRECT-CALL-30995, RESIDUAL-INDIRECT-CALL-30996,
RESIDUAL-INDIRECT-CALL-30997, RESIDUAL-INDIRECT-CALL-30999,
RESIDUAL-INDIRECT-CALL-31000, RESIDUAL-INDIRECT-CALL-31001,
RESIDUAL-INDIRECT-CALL-31012, RESIDUAL-INDIRECT-CALL-31013,
RESIDUAL-INDIRECT-CALL-31014, RESIDUAL-INDIRECT-CALL-31016,
RESIDUAL-INDIRECT-CALL-31018, RESIDUAL-INDIRECT-CALL-31022,
RESIDUAL-INDIRECT-CALL-31047, RESIDUAL-INDIRECT-CALL-32120,
RESIDUAL-INDIRECT-CALL-32122, RESIDUAL-INDIRECT-CALL-32307,
RESIDUAL-INDIRECT-CALL-32309, RESIDUAL-INDIRECT-CALL-33286,
RESIDUAL-INDIRECT-CALL-33394
```

source RVAs：

```text
0x000069FE, 0x00625D64, 0x00625E27, 0x00625F26,
0x00626007, 0x00626136, 0x0066A7F6, 0x0066A936,
0x0066A9E2, 0x0066B0AD, 0x0066B581, 0x0066BC56,
0x0067FB91, 0x00682C5B, 0x006A1BC6, 0x006A8C62
```

### 3.3 必查 anchor：`0x000069FE -> 0x00006A75`

Containing function RVA `0x000069E0..0x00006AEC`，file offset `0x00005DE0`，size `268`，SHA-256 `9981E37D5D3E4A6060EC39029D5C1B814DCAB4C0CBCB015289D8A7A9402D3B92`。

```asm
RVA 0x000069FE  8B 3D 84 88 B1 00  mov 0x00B18884,%edi
...
RVA 0x00006A64  73 1A              jae 0x00006A80
RVA 0x00006A66  68 F8 03 00 00     push 0x3F8
RVA 0x00006A6B  68 D8 26 BA 00     push 0x00BA26D8
RVA 0x00006A70  68 70 27 BA 00     push 0x00BA2770
RVA 0x00006A75  FF D7              call *%edi
```

Entry block `0x000069E0..0x00006A06` 无 predecessor；loop backedge `0x00006AC6 -> 0x00006A06` 位于 IAT load 之后，因此每次可到达 `0x00006A75` 时 EDI 仍为同一 `_wassert` thunk 内容。EDI 在 source 到 sink 之间无写入，三个参数均为 immediate，故父 `RESIDUAL-INDIRECT-CALL-00004` 可安全移入 register-IAT exclusion。

### 3.4 保留 register frontier

其余 `2,287` 个机械 sink 本次没有获得完整的逐函数 all-path predecessor、callee-clobber 与 Manager argument/return/callback taint 联合证明，全部保留原父 identity；不生成描述性重复 frontier。

```text
retainedCount = 2287
compactJsonByteLength = 619778
compactJsonSha256 = 2BB9486882E8651D0EF7A761908A7F11B847B02D6DBB5294C1F59671CECFD703
failureBucket = ALL_PATH_CFG_CLOBBER_AND_MANAGER_TAINT_NOT_EXHAUSTED
firstRetainedParentId = RESIDUAL-INDIRECT-CALL-00372
firstRetainedRva = 0x000278B2
firstRetainedRaw = FF D7
sourceRva = 0x000278A1
targetRegister = EDI
iatSlotRva = 0x00718978
functionRva = 0x00027870
functionSha256 = A6FEFC5644240627DC1AAEAC1EDBA2F998225FA8B534E62FE343AD135AB03D0D
```

因为只有完成 target 与 taint 两个证明的 46 行才被排除，所以 `registerMediatedImportAliasEscapes=[]`；未裁决行仍是父 call frontier，不可被误报为零风险 import。

## 4. `FF 24` switch census 与裁决

### 4.1 独立 census

从 v3 `indirectTailTransferFrontier` 与磁盘 `.text` 同时重建 raw `FF 24`、SIB base-absent、scale `4`、absolute-indexed tail，得到 `763` 个唯一父行。按 `sinkRva,parentId` 升序的 projection 为：

```text
fields = parentId,sinkRva,rawBytes,functionRva,functionSha256
count = 763
compactJsonByteLength = 161617
compactJsonSha256 = A1AE9742F144D24296353923923A42C977EE4C9BB59FD6EEE5461E78D1AF6FD4
```

### 4.2 三个已证明的同构二级 switch

仅以下三个父行满足本次完整证明：

| Parent ID | Tail RVA / raw | Function RVA / SHA-256 | Pointer table RVA | Byte-map RVA |
| --- | --- | --- | --- | --- |
| `INDIRECT-TAIL-TRANSFER-00034` | `0x00016987` / `FF 24 85 FC 6A 41 00` | `0x00016920`, `460986AF6EFC4C111A7DE0AFBCA43FBA0418A2DE50C502282E32281DBE60A01D` | `0x00016AFC` | `0x00016B14` |
| `INDIRECT-TAIL-TRANSFER-00035` | `0x00016BD7` / `FF 24 85 4C 6D 41 00` | `0x00016B70`, `00963DF0F872BA504C49254799EF70200AFD8A03B0343AE7385C6DD8C15DE875` | `0x00016D4C` | `0x00016D64` |
| `INDIRECT-TAIL-TRANSFER-00036` | `0x00016EE7` / `FF 24 85 5C 70 41 00` | `0x00016E80`, `32A6E6006CD283CD2E0CA11A3523FECF0CD7A82F929A7CFAECDBDCCE27A1D992` | `0x0001705C` | `0x00017074` |

每个函数的闭合链相同：

1. `movsbl 0x1(%esi),%eax; add $-35,%eax; cmp $0x4E,%eax; ja default`，所以 fallthrough domain 精确为 unsigned `[0,78]`，无 signed ambiguity 或回绕。
2. `movzbl byteMap(%eax),%eax` 读取 raw-backed 79-byte map；三份 map 字节相同，值域精确 `{0,1,2,3,4,5}`，SHA-256 均为 `EF637675F0403206CFA0E2D41CACD198B07EF674FC4F8DDA13940DF002C2C726`。
3. `jmp *pointerTable(,%eax,4)` 只读取六个 4-byte cell。pointer table 与 byte map 均在 `.text` 的 `CONTENTS,ALLOC,LOAD,READONLY,CODE` 范围内且 raw-backed。
4. 六个 cell 均有唯一 PE base-relocation type `3/HIGHLOW`；三表共 `18` 个 relocation target，无遗漏或额外 loader-mutated directory 命中。
5. 每个 table/map VA 的 whole-file exact payload count 为 `1`，分别就是对应 `movzbl` 或 `jmp` operand；没有额外地址逃逸。read-only `.text` 不存在固定映像 writer。
6. 六个 target 全部是当前同一函数的 objdump instruction boundary，IDA switch block 也给出完全相同的六个 successor；没有跨函数 target、坏 cell 或隐藏 successor。Manager taint不跨越调用边界，只沿同函数 CFG 继续。

六 target 按 table index `0..5`：

```text
0x00016987 -> [0x00016A5B,0x0001698E,0x00016A1B,0x00016A3B,0x000169E7,0x00016AD6]
0x00016BD7 -> [0x00016CAB,0x00016BDE,0x00016C6B,0x00016C8B,0x00016C37,0x00016D26]
0x00016EE7 -> [0x00016FBB,0x00016EEE,0x00016F7B,0x00016F9B,0x00016F47,0x00017036]
```

proof projection：

```text
count = 3
compactJsonByteLength = 637
compactJsonSha256 = 3E7EDC53DA8AC115EA643C4ED3A6E54A81909495D73761A1D36DF674DA26271D
```

v4 receiver graph 应追加 `3` 个 switch-site nodes、`18` 个 successor nodes 和 `18` 条 site-to-successor edges；node ID 模板为 `BATCH2-SWITCH-SITE-0001..0003` 与 `BATCH2-SWITCH-TARGET-0001-00..0003-05`，edge ID 模板为 `BATCH2-SWITCH-EDGE-0001-00..0003-05`。所有新增 nodes 标记 `managerTaintIn=false`、`managerAliasTaintOut=false`、`classification=INTRAPROCEDURAL_CONTROL_ONLY`。因此 receiver graph `20/26 -> 41/44`。

### 4.3 保留 switch frontier

其余 `760` 行未在本次 REVERSE 中获得 guard dominance、index domain、table extent、全部 cell relocation、同函数 CFG target 与 writer/address-escape/loader mutation 的联合逐行证明，全部保留父 identity：

```text
retainedCount = 760
compactJsonByteLength = 160981
compactJsonSha256 = C6D6993E81C3FFC12A0BBA939F6A112F374A9752E3990FB29E7AB930F5A17A3C
failureBucket = ALL_PATH_GUARD_DOMAIN_TABLE_AND_CFG_NOT_EXHAUSTED
firstRetainedParentId = INDIRECT-TAIL-TRANSFER-00037
firstRetainedRva = 0x00019A0A
firstRetainedRaw = FF 24 85 3C 9B 41 00
functionRva = 0x000199D0
functionSha256 = 09ABC62E6E2C6E4812B84E6C15A44ADE65FD142E016FBAA2AF5C5E7CDF7D7C1C
```

## 5. 精确集合差分与首边

按父 ID 而不是自报计数计算：

```text
receiver_v4
  = 20,744
  - 46 resolvedRegisterIatCallRows
  - 0  resolvedRegisterIatTailRows
  - 3  provenIntraproceduralSwitchRows
  + 0  registerMediatedImportAliasEscapes
  + 0  newlyDiscoveredExactFrontiers
  = 20,695
```

分桶为：

```text
baseOnlyReceiverFrontier        474   (逐字继承 v3)
splitRegisterReceiverFrontier   270   (逐字继承 v3)
residualIndirectCallFrontier  16,277  (16,323 - 46)
indirectTailTransferFrontier   3,674  (3,677 - 3)
importAliasEscapeFrontier          0
newExactFrontier                   0
totalReceiverFrontier         20,695
```

最终 `id,instructionRva` projection 按 `instructionRva,id` 排序：count `20,695`，compact JSON byte length `1,404,417`，SHA-256 `41BC8237B58B4B9EF66303D3EE2DD1D7D678B5AB62C71720F37065CFA1645811`。

首个未闭 receiver/universe 边由最终集合排序产生：

```text
id = RESIDUAL-INDIRECT-CALL-00007
functionRva = 0x00007440
instructionRva = 0x00007455
rawBytes = FF D3
decoded = call *%ebx
functionEndRvaExclusive = 0x00007466
functionSha256 = 5D5CA69C6D6D3098B8363A8AF646DBC82F870D51CE6628D3ED6D19261C4FC147
edgeKind = RESIDUAL_INDIRECT_CALL_TARGET_PROVENANCE_NOT_CLOSED
reason = not in the resolved register-IAT subset; complete target/Manager-taint provenance remains unbounded
```

RVA `0x00007455` 不是硬编码替代值；它是在删除上述 49 个互斥父 IDs 后对实际剩余集合排序所得。

## 6. Pair、不变量与主线集成要求

- pair graph 必须逐字继承 v3：nodes `10`、edges `8`、frontier `34`，其中 liveouts `33`、exception/nonlocal `1`、unresolved calls/escapes `0`。
- pair/drain 的首缺口仍是 function RVA `0x00023CC0`、instruction RVA `0x00023D12`、raw `E8 79 C6 5F 00`。
- base-only `474`、split-register `270` 原数组逐字继承；不能因本批 register/switch 排除重编号。
- `receiverDispatchFixedPointClosed=false`、`pairAliasFixedPointClosed=false`、`dynamicAliasUniverseClosed=false`。
- `allAliasesBoundedAndReleasedBeforeManagerRemove=false`、`nativeOwnerHeldThroughDrain=false`；drainability/native owner 保持 `UNKNOWN`。
- factory/rollback、reference-count、final-release/thread 与权限/解锁逐字继承 v6；`VIRTUAL_DEVICE_OBJECT_OWNER` 不提升，`formalDecision=null`，`implementationEligibility=false`，`workPackageStatus=BLOCKED_BY_EVIDENCE`。
- v4 resolved exclusions必须复制完整父 row，再追加 source chain、IAT identity、all-path coverage、taint disposition 与 proof refs；失败候选只保留父 row，不另增计数行。
- 本 REVERSE 未执行 TEST 或公开 CLI；最终数量、hash、负向 mutation 与写边界由独立 TEST 角色按 preseal 验收。

