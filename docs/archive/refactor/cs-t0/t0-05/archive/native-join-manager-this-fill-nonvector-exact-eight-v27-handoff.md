> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 memset nonVector exact8 v27 交接

## 结论

本分片只处理 bulk remaining frontier v26 的 nonVector exact8：

```text
0x000148AA  UNKNOWN
0x0001D1D9  VERIFIED
0x0024CF7A  VERIFIED
0x0029FA85  VERIFIED
0x003DE449  VERIFIED
0x005CF2B3  UNKNOWN
0x0062C316  UNKNOWN
0x006EC167  UNKNOWN
```

合计 `4 VERIFIED / 4 UNKNOWN`。没有消费旧 v4 的 proof-family narrative 或汇总
布尔结论。四个正结论来自本轮对本地冻结 PE 的 exact call bytes、raw caller 集、
局部 allocation/capacity guard、start/count/extent 与 32 位无回绕复算。三个既有
负结论只绑定冻结的 v22、fixed-member v12 与 allocator v14 independent audit。
format-buffer 行则由本轮 PE 复查发现一个最小反例前沿，按 fail-closed 保留
`UNKNOWN`。

本交接不是 whole-image、manager-this 或 T0 的验收与提升；所有 permission、
promotion 与 `overallAccepted` 均为 false。最终验收交给独立代理。

## exact call bytes 与 compiled caller universe

8 个目标 call bytes 均从 SHA-256 固定的磁盘 PE 重放。四个 caller-capacity helper
与 format helper 的 raw `.text` caller 集如下；每组同时确认没有指向 helper entry
的 `E9` tail jump，也没有 helper VA 的 `.text` absolute occurrence：

```text
sub_41D1B0  6 callers
  41CA8B 41D45F 736CDE 73740B 737C88 737EE7

sub_64CF20  3 callers
  64D29A 64D2B1 64D8C4

sub_69FA70  2 callers
  69C666 6A01CE

sub_7DE420  3 callers
  7DA310 9493AB 94E6CF

sub_A2C250 17 callers
  7F6C33 7F6C76 7F6D27 7F6D76 7F6FC6 84892C 85A04F
  85A7EF 8C9AE9 8CFEF9 8F13BD 94FB8D 982D39 982EA9
  9A3169 9A382B 9A395F
```

analyzer 以一次 raw E8/E9 扫描和 target-VA occurrence 扫描重建这些集合，而不是
读取 IDA 汇总或旧 v4 的 `allCallsitesEvidence`。

## 四个 VERIFIED 行

### `0x0001D1D9`

`sub_41D1B0` 写 `4*a2` 字节。6 个 caller 分成容量内 tail 与 fresh allocation：

- `41CA8B`、`73740B` 的支配分支给出 requested 不超过 capacity，destination=end；
- `41D45F`、`737EE7` 是 grow helper 中的 fresh allocation tail；
- `736CDE`、`737C88` 先按 `4*roundedCount` 分配，再从 allocation 起点填充。

所有增长路径拒绝 count 大于 `0x3FFFFFFF`。因此
`start+count=4*requested<=4*capacity<=0xFFFFFFFC`，无 32 位回绕，覆盖不超过
allocation/caller capacity。

### `0x0024CF7A`

`sub_64CF20` 从 receiver 起点写固定 `0x17A=378` 字节。`64D29A` 与 `64D2B1`
使用 `EBP-0x188`，其函数帧保留 `0x198` 字节；`64D8C4` 在对齐后保留
`0x1C0` 字节，三次 push 后 `LEA ECX,[ESP+0x4C]` 仍精确形成
`alignedESP-0x188`。三者都给出至少 `0x188=392` 字节的编译期 stack object，
故 `378<=392`，且栈地址不与 loaded-image manager slot 重叠。

### `0x0029FA85`

`sub_69FA70` 对每个元素写固定 100 字节。`69C666` 只有在
`size<104 && capacity>=104` 时传入 `end` 与 `104-size`；`6A01CE` 是同一外层
`size<104` 条件下的 fresh growth，destination=`allocation+100*size`。因此完整
loop end 恒为：

```text
100*size + 100*(104-size) = 10400 <= 100*capacity
```

`size<104` 还把本行全部算术限制在 10400 以内；grow helper 单独绑定了 allocation
乘法守卫。

### `0x003DE449`

`sub_7DE420` 写 `4*n4` 字节。`7DA310` 在拒绝大于 `0x3FFFFFFF` 后 exact allocate；
`9493AB` 只有 requested 不超过 `(capacityEnd-begin)/4` 才走 direct branch；
`94E6CF` 是 fresh grow tail。统一有
`requested<=capacity<=0x3FFFFFFF`，所以
`4*requested<=4*capacity<=0xFFFFFFFC`，无回绕且覆盖在 backing 内。

## 四个 UNKNOWN 与最小首缺口

### `0x000148AA`

绑定 v22 的独立 superseder，不提升。首缺口仍是
`ACTUAL_RECEIVER_DUAL_ROOT_ALREADY_OFFSET_ALIAS_FIXED_POINT_OPEN`：需要闭合
`n298+0x4C614` 与 `owner+0x123C` 的 split/EH、spill/reload、return/liveout、
address-taken/indirect 与无 literal displacement writer 固定点。

### `0x005CF2B3`

绑定 fixed-member roots-two v12，不提升。最小未闭集合为：

```text
A1_ENTITY_PLAYER_PLUS_0x1E68_POINTEE_PRODUCER_UNIVERSE
B1_SUB_5B39D0_ECX_THIS_ALLOCATION_TYPE_UNIVERSE
B2_SUB_7AC7E0_EDI_ALLOCATION_TYPE_UNIVERSE
```

### `0x0062C316`

site-local 公式本身成立：parser prefix 被截到
`unit*(capacityArgument-1)`，随后在 `base+parsedSize` 写 `unit` 字节，所以条件式
上界为 `parsedSize+unit<=unit*capacityArgument`。17 callers 中 16 个的固定
base/capacity/unit 与局部 extent 可闭；唯一首缺口是 `0x009A382B`：

```text
sub_9A36D0 frame reservation = 0x148
base                         = EBP-0x118
capacityArgument             = 0x100
format code 1 -> unit        = 2
helper-declared upper extent = 0x200
immediately local interval   = EBP-0x118 .. EBP-0x18 = 0x100 bytes
```

因此还必须证明该 caller 的 `parserOutputBytes<=0xFE`，或证明一个不同的 owning/
split-frame backing 从 `EBP-0x118` 起确有 `0x200` 字节。缺少任一证明时不能把
helper 的条件式上界当作真实对象 extent，也不能复用 v4 的“17 个 compiler-sized
stack arrays”总括结论。

### `0x006EC167`

绑定 allocator v14 independent audit，不提升。首缺口为
`AEBE30_AEA9B0_ALTERNATE_BACKING_PRODUCER_NOT_CLOSED`：reachable successful
`sub_AEA9B0` 路径同样向 exact-row 的 EBX destination 供 backing，但候选的 allocator
extent/header 不覆盖该 producer。

## create、双 verify、mutation 与外部分析器

使用同一 analyzer/JSON，先 create-new，再连续两次 verify；第二次显式提供相同 PE
路径以覆盖 CLI mode independence：

```text
created           verified=4 unknown=4 mutationsRejected=30
verified-existing verified=4 unknown=4 mutationsRejected=30
verified-existing verified=4 unknown=4 mutationsRejected=30
JSON SHA-256       0EE93EF99415707B0EA0A43515BB0A9B31B558D7AE85BFA80F1AF2BD77841E9A
```

30 个 mutation sentinel 覆盖 exact8 缺/增行、v4 narrative 误消费、PE/caller census、
tail/address-taken、四个正行的 extent/no-wrap、四个 UNKNOWN 非法提升、format
`16+1` 误改为 `17+0`、外部 TIMEOUT 误报 PASS、计数、permission 与 promotion，
全部 fail closed。

外部分析器当前忙，按任务约束只记录 `TIMEOUT_NO_PASS`；这不是 PASS，也不作为
反证。没有继续轮询或修改 IDB。

## 产物与边界

- analyzer：`out/cs-t0/analyze-manager-this-fill-nonvector-exact-eight-v27.py`
  - SHA-256 `70AF71C3B039C654050E2F17E2622A4A453B8100580DD1B9FD80224A805FD9A2`
- machine JSON：`out/cs-t0/native-join-manager-this-fill-nonvector-exact-eight-v27.json`
  - SHA-256 `0EE93EF99415707B0EA0A43515BB0A9B31B558D7AE85BFA80F1AF2BD77841E9A`
- handoff：`docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-fill-nonvector-exact-eight-v27-handoff.md`
  - SHA-256 在最终交接消息中报告，避免自引用哈希。

全程没有启动或读取游戏进程，没有附加调试器，没有修改 IDB、production、tests、
旧 evidence、状态或索引，也没有 runtime mutation。目标 PE SHA-256 为
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
独立验收代理应重新计算三份 SHA，执行 create-new 于新的 output path 或核对现有
JSON 后连续双 verify，重放 exact8 call bytes、五个 caller census、positive anchors、
format `0x009A382B` 首缺口、30 mutations，以及所有 permissions/promotions=false。
