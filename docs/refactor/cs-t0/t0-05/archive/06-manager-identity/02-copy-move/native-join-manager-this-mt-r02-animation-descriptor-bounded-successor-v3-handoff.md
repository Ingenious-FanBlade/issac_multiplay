> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R02 animation descriptor bounded successor v3 逆向交接

日期：2026-09-06  
角色：`REVERSE`  
状态：`BOUNDED_STOP / 0 STATIC_CONFIRMED_NO_OVERLAP / 2 UNKNOWN`

## 1. 范围与冻结输入

本文件是 MT-R02 的 append-only successor candidate/handoff，只处理 exact rows
`0x00008A62` 与 `0x00008B2A`。它没有修改 README、status、frontier、既有 candidate、
既有 handoff 或其它证据，也不承担独立 TEST 验收。

- 目标：x86 PE32，ImageBase `0x00400000`，SizeOfImage `0x008FE000`。
- 目标 PE SHA-256：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 独立 TEST 在读取本轮 candidate 前封印的 exact2 acceptance spec：
  `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-acceptance-spec-v1.json`，
  物理 SHA-256
  `7CAEAC7ADE8E53255AAC70E03BC58C8835563F966B706D83BC4CE76B6D822169`。
- 与该 sealed spec 绑定的结构化 REVERSE candidate：
  `out/cs-t0/native-join-manager-this-mt-r02-animation-descriptor-bounded-successor-v3.json`，
  冻结物理 SHA-256
  `D441CE7910F448153140E9F1F75B4EA692276246829ABE244ACF566241C4F46C`。
- 直接前件：
  `out/cs-t0/native-join-manager-this-mt-r02-old-new-count-v2.json`，本轮读取时
  SHA-256 `3A6872DB6AB1E7CCC0A61ED752A68DCCF23A85C2FEEE96D7EAA67ED20EE3E0AB`。
- 上一 R02 handoff SHA-256：
  `B58C34A006C66AFD880AB1D62E1E575064038C8EC206EC24BEFC6F74EA165EE4`。
- exact5 follow-up handoff SHA-256：
  `79B2EA5B96771A14CFAA6AB688D56C4B327A3E249073EE220D8033CD196087B8`。
- follow-up priority v35 SHA-256：
  `0F4320F7FEBFEDD5D9CD1421C1D291F2198A476ACABA1C4E2E1EE0E13F77EF3B`。

IDA Pro MCP 只读连接的当前 IDB 精确报告上述 PE SHA-256、ImageBase 与
SizeOfImage；没有执行任何 IDB mutation。

## 2. 新增静态语义

### 2.1 owner、descriptor 与 runtime state

`sub_40A5D0`、`sub_40AAF0` 的字符串诊断为
`[warn] no animation named %s`。两者从同一个 Sprite-like owner 的
`owner+0x74` 取得 animation descriptor 数组、从 `owner+0x78` 取得记录数，
按 descriptor 起始处的 SSO string 做精确名字匹配，记录 stride 为 `0x13C`。

owner 内存在两个 32-byte runtime animation state：

```text
primary state   = owner + 0x30
secondary state = owner + 0x50

state + 0x04 = current descriptor pointer
state + 0x08 = first int32 state backing
state + 0x0C = second int32 state backing
descriptor + 0x1C = first element count
descriptor + 0x24 = second element count
```

这不是仅由字段形状推测：

- `sub_408830(state, descriptor)` 把 `state+4` 设为 descriptor，并分别按
  `4*descriptor[+0x1C]` 与 `4*descriptor[+0x24]` 分配/初始化两个 backing；
- `sub_4086E0` 复制 runtime state 时，仍分别按 current descriptor 的两项 count
  分配并复制 source backing；
- `sub_408E00` 分别以 descriptor `+0x1C/+0x24` 为循环界，写入
  `state+0x08/+0x0C` 的 int32 元素并调用两类 per-element animation update。

因此两项 count 是两个 runtime animation state backing 的元素数，而不是 owner
容量或 selector 数量。当前证据不再把 descriptor 仅称为未知的通用记录。

### 2.2 selector flag 语义

两条 selector 的 ABI 均为 x86 `__thiscall`：ECX 为 owner，栈上依次为 animation
name 与 32-bit flag，`ret 8`。name match 后：

```text
flag != 0 -> sub_408830(state, incomingDescriptor)       // 丢弃并重置 state
flag == 0 -> sub_408970(state, incomingDescriptor)       // 保留旧 per-element state
```

对应精确 selector block：

- `0x0000A652`，52 bytes，SHA-256
  `93C2D3E550E24307D9509B80C7862A03ADE90B0352E8959CFA5ADCCF294A0DD7`；
  其中 ECX 加 `0x30`，`0x0000A678: E8 F3 E2 FF FF` 调 `sub_408970`。
- `0x0000AB72`，52 bytes，SHA-256
  `7BC8D5D7D4F486CBCB2BD8E5C853BAE6517D51B5CB916A049C6612C88BEE35FC`；
  其中 ECX 加 `0x50`，`0x0000AB98: E8 D3 DD FF FF` 调 `sub_408970`。

`sub_408970` 的 ABI 为 x86 `__thiscall`：ECX 为 32-byte runtime state，唯一栈
参数为 incoming descriptor，`ret 4`。两行目标均为 cdecl `memcpy(dst, src,
size)`，caller 在返回后 `add esp, 0x0C`：

| RVA | bytes | source | destination | size |
|---|---|---|---|---|
| `0x00008A62` | `E8 78 7B 6E 00` | `state+0x08` old backing | fresh allocation requested with `4*incoming[+0x1C]` | `4*current[+0x1C]` |
| `0x00008B2A` | `E8 B0 7A 6E 00` | `state+0x0C` old backing | fresh allocation requested with `4*incoming[+0x24]` | `4*current[+0x24]` |

两个 call 均解析到 `memcpy VA 0x00AF05DF`，且均为指令头。

### 2.3 两条已知 `flag=0` 真实调用链

第一条链位于 `sub_43AA90`：

```text
0x0043ABCA: choose literal "Idle" or "Idle2"
0x0043ABD6: push 0
0x0043ABE3: lea ecx, [edi+0x48]
0x0043ABE6: call sub_40A5D0
0x0040A678: call sub_408970(primary state, name-selected descriptor)
```

其中 `0x0043ABCA..0x0043ABEA` 的 33-byte slice SHA-256 为
`8ACC2C78464BC137336984AFA8BE4E80EA7CACA3496B805D953BEAFD4BFB1D45`。
caller 只按 `[eax+0x10] <= 0x80` 在 `Idle/Idle2` 间选择名字；它不读取、比较或
clamp descriptor `+0x1C/+0x24`。

第二条链位于 `sub_4E5170`：

```text
0x004E549B..0x004E54B8: choose "HeadLaughUp", "HeadLaughDown", or "HeadLaughRight"
0x004E54B9: require caller-local byte [ebp-0x19] != 0
0x004E54BD: lea ecx, [edi+0x48]
0x004E54C2: push 0
0x004E54C5: call sub_40AAF0
0x0040AB98: call sub_408970(secondary state, name-selected descriptor)
```

其中 `0x004E549B..0x004E54C9` 的 47-byte slice SHA-256 为
`8083BC843A5C17E4B9844C6AB2325F1E872138648BD785B77F9ED86C8D2496C9`。
caller 的 hard branch 只约束是否走 secondary selector；它同样不读取、比较或
clamp 两项 descriptor count。

这两条链证明 `flag=0` preserve/migrate 路径有具体游戏调用者及具体 animation name，
但不证明任一被选 animation 的资产 count 数值，也不证明 `oldCount<=newCount`。

## 3. old/new 关系与 bounded countermodel

`sub_408970` 对每一项只做 `oldCount != newCount`：

1. `newCount > 0` 时，以 `mul/seto` 路径形成 allocator 请求；
2. destination 先 `memset(..., 0xFF, 4*newCount)`；
3. `oldCount > 0` 时，以普通 `shl oldCount,2` 形成 `memcpy` 长度；
4. 没有 `oldCount<=newCount`、`min(old,new)`、clamp 或 caller dominance check。

因此两行都保留同一个最小符号 countermodel：

```text
current descriptor count = oldCount = 2
name-selected incoming descriptor count = newCount = 1
requested destination extent = 4 bytes
memcpy transfer = 8 bytes
```

本轮**没有找到**把具体 `old=2,new=1` 写入上述 Idle/Idle2 或 HeadLaugh 记录的
current-PE native producer，所以不能把它报告为 `RUNTIME_OBSERVED` 或已发生越界。
但两条具体 flag=0 CFG 路径也没有任何支配性 hard bound 排除该值组合；在 selector
接收的两个合法 animation records 具有该计数时，callee CFG 必然到达对应 memcpy。

另一个不能被忽略的精确生命周期事实是：第二组迁移在
`0x00008B63` 清零 `state+0x0C` 后，`0x00008B6A` 的实际指令为
`89 5E 08`（`mov [esi+8], ebx`），把新的第二组 allocation 写到 `state+0x08`，
而不是 `state+0x0C`。该 adoption 位于目标 memcpy 之后，不能修复目标 transfer span；
它也使完整 two-backing replacement/lifetime 语义继续开放。本文件不把它外推为运行时
故障，只把它作为不得声称 owner/lifetime fixed point 已闭合的目标字节事实。

## 4. 逐行四项义务

四项顺序固定为 `backingExtent -> countAndNoWrap ->
sourceLifetimeAndIdentity -> targetIntervals`。

| RVA | backingExtent | countAndNoWrap | sourceLifetimeAndIdentity | targetIntervals | verdict |
|---|---|---|---|---|---|
| `0x00008A62` | `OPEN`：fresh destination 请求 `4*new[+0x1C]`，不足以覆盖 `4*old[+0x1C]` | `OPEN`：allocator 乘法有 overflow signalling，但 old copy 的 `shl 2` 无独立 no-wrap，且无 old<=new | `OPEN`：reset/copy 的局部 source extent 已确认；资产 descriptor producer、全部 state copy/move/adoption/reentry 与 lifetime fixed point 未闭 | `OPEN`：destination/source span 未闭，不能排除与 GameRoot slot `[0x00871678,0x0087167C)` 的 alias countermodel | `UNKNOWN` |
| `0x00008B2A` | `OPEN`：fresh destination 请求 `4*new[+0x24]`，不足以覆盖 `4*old[+0x24]` | `OPEN`：同一无比较/无 copy no-wrap 缺口 | `OPEN`：局部 source extent 已确认；除共同 producer 缺口外，`0x8B6A` 非对称 adoption 使第二 backing replacement fixed point 未闭 | `OPEN`：同样无法闭合 transfer intervals 与 GameRoot slot non-alias | `UNKNOWN` |

两行均为：

```text
fullyAdjudicatedNoOverlap = false
disposition = UNKNOWN
promoted = false
```

## 5. bounded stop 与第一未闭生产者类

本轮按要求不展开全部 `1048/91` 个 selector callers。停止点是：

1. incoming descriptor 的 `+0x1C/+0x24` 来自 name-selected animation asset record；
   其 parser/constructor/adoption、copy/move 和 asset-input count producer 尚未形成
   current-PE 的全称 fixed point；这是 count 关系的第一未闭 producer 类；
2. primary selector 的 thin forwarder `VA 0x0040A5B0` 只有一个 data xref
   `VA 0x0086739D`，secondary selector 的 thin forwarder `VA 0x0040AAD0` 只有一个
   data xref `VA 0x0086741B`；二者均属于 `sub_866960` 内的 address-taken callback
   注册域，callback receiver/name/flag/reentry 的可达 producer 未分类；
3. direct selector universe 已知为 primary `1047 E8 + 1 E9/address-taken = 1048`，
   secondary `90 E8 + 1 E9/address-taken = 91`。继续枚举不能由当前两个具体 caller
   自动推出全称 count 单调关系。

结论是：两组 `oldCount<=newCount` 均未证明，更强 allocation/span 不变量也未证明；
没有找到决定性的真实 `old=2,new=1` 资产 producer，但符号 countermodel 在已证
flag=0 路径上未被任何支配 hard bound 排除。MT-R02 两行保持 `UNKNOWN`，不得进入
positive frontier。

## 6. 工具、测试与权限

本轮只使用 IDA Pro MCP 的只读 `survey_binary`、`analyze_batch`、`decompile`、
`entity_query`、`insn_query`、`get_bytes`、`xrefs_to`，以及工作区只读文件检查。
没有启动游戏、没有使用 x32dbg、没有打开或读取目标进程、没有执行 Cargo/全量 T0
build，也没有运行独立验收。测试规模为零。candidate 冻结前只做了一次作者侧 schema /
物理 binding 完整性检查，并确认 target-hash drift、exact-row omission、permission-enabled
三项核心 mutation 均被拒绝；这不构成独立 TEST 结论。候选仍需由作者分离的 TEST 角色按
sealed exact2 contract 独立验收。

```json
{
  "permissions": {
    "gameStarted": false,
    "processOpened": false,
    "processRead": false,
    "debuggerAttached": false,
    "idbModified": false,
    "priorCandidateModified": false,
    "productionModified": false,
    "priorEvidenceModified": false,
    "statusOrIndexModified": false,
    "frontierModified": false,
    "runtimeMutationAllowed": false,
    "activeExperimentAllowed": false,
    "managerThisPromotionAllowed": false,
    "wholeImagePromotionAllowed": false
  }
}
```
