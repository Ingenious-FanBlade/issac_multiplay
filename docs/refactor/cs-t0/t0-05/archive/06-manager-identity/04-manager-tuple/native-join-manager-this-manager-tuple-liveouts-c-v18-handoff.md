> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple live-outs Group C v18 逆向交接

## 结论

本轮只裁决 v15 原样保留的 v11 `blockLiveOutAliases` 中 Group C：精确
15 个函数、16 个 context。输入集合不是重新枚举；analyzer 从 v15
`remainingV11Frontier.blockLiveOutAliases` 原样筛选，并断言：

```text
accepted Group A = 40 context
unknown Group B  = 32 context
current Group C  = 16 context
A ∩ B ∩ C        = empty
A ∪ B ∪ C        = exact original 88 context
```

CFG `(block,state)` 不动点结果为：

- `16/16` context resolved；
- `0` 个 manager tuple `begin/end/capacityEnd` 字段写入；
- `0` 个新 tuple-object helper target、source 或 writer；
- `0` 个未分类 call forward；
- `0` 个 EAX return alias；
- Group B exact-32 原样保持 `UNKNOWN`。

因此本轮只把已接受 A40 后的 B+C48 局部 frontier 再缩减 C16，留下 B32。
它不单独完成 v11 全局 writer universe、backing-pointer closure、whole-image
alias、ABA 或 manager-this 身份；`managerThisPromotionAllowed=false`，全部权限
仍为 false。

## 冻结绑定与范围

- v11 primary/writer/live-out JSON：
  `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- 已接受 helpers-a v13：
  `322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- 已接受 helpers-b v14：
  `A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`
- 已接受 helpers-c v15：
  `B07501A2D6DD6492E6834ECC64A5B23FF3B125DB415643B87F04DE4FB53EDCAD`
- 已接受 liveouts-a v16：
  `511300FC52958E66EEA59E1059663695DF2491480C5E87772E4FADAF15E458D7`
- 冻结 v16 CFG fixed-point analyzer：
  `465EB61B4F66A04577F5839D41F46BF3FC08824BD348CA8BB8B5E79534B37BAD`
- **失败的 helpers-a v12 未进入 binding，也未作为接受证据读取。**
- 冻结 `isaac-ng.exe`：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- x86 image base：`0x00400000`
- tuple receiver：`DWORD[0x00C71678] + 0x0001BAA8`
- tuple fields：`begin@+0 / end@+4 / capacityEnd@+8`

Group C 精确集合为：

```text
009BEBA0(2)
004186C0(1) 004291C0(1) 006DDCD0(1) 007475B0(1) 00757AC0(1)
007584B0(1) 007FEC00(1) 00802740(1) 00911C70(1) 009172F0(1)
00921CE0(1) 009439D0(1) 0095A7B0(1) 0096FD60(1)
```

Group B 未决精确集合继续为 16 个函数、每函数 2 个 context：

```text
007FB250 00800500 008020D0 00816910 00833600 00833C40
0083B850 0084C170 008D19A0 0090D3F0 00944A40 009563B0
0098DBA0 009A8620 009A8A40 009ACA90
```

## CFG fixed-point 与调用边界

每个 context 从 v11 给出的 block live-out state 开始，只向真实 IDA CFG
successor 传播。状态 key 包含 block/start-EA、寄存器别名、EBP spill 和有界
pushed arguments；重复 `(block,state)` 是不动点 revisit。

抽象值区分精确 tuple object、`begin/end/capacityEnd` 的 dereferenced value、
可证明远离三字段的 receiver-relative offset 和 unknown offset。unknown offset、
未分类 tuple-object call forward、tuple-field write、未审计 EAX return 都会
fail closed，而不是当作 alias death。

本轮累计：

```text
visited (block,state) = 214
CFG transitions       = 253
fixed-point revisits  = 71
tuple field reads     = 56
tuple field writes    = 0
unresolved forwards   = 0
return aliases        = 0
```

14 条 call path 只把已经解引用的 tuple value 传给 `0x00A112C0`；callee 没有
收到 tuple object address。其余路径均在显式覆盖、caller-clobber 或函数边界前
丢失 alias；没有新 helper/source/writer。

## indirect switch-jmp 修正与 A/B 风险审计

初轮自检发现冻结 v16 fixed-point 的外部 call/tail 判断会把 `direct_target=None`
的间接 `jmp` 直接当作 tail exit，即使 IDA block 实际存在 CFG successors。
Group C 的唯一命中是：

```text
function     0x007475B0
source block 0x0074797E
switch jmp   0x00747C96
```

v18 不修改 v16 文件，而是在加载且验证 v16 analyzer hash 后，对唯一明确的
source line做 fail-closed patch：若 indirect `jmp` 所在 IDA block 有 successors，
它属于当前函数 CFG，必须把所有 successor 入队。修正前草稿 JSON 已作废，未
进入本交接；修正后计数从 `210/245/67` 变为 `214/253/71`，结论仍为 16/16、
零 writer/helper/source/unresolved/return。

对 A-v16 和 B-v17 当前 machine JSON 的 terminal outcomes 做了精确命中审计：
二者 `TAIL_EXIT_AFTER_SAFE_FORWARD` 都是 `0` 条。因此该缺陷没有命中 A 或 B
的任何 seed/path；本轮没有修改它们的 analyzer、JSON 或结论。

## 保留阻断

机器 JSON 中 Group B 32 行原样嵌入，状态为：

```text
UNKNOWN_EXACT_GROUP_B_ONLY
```

以下字段仍保持 false：

```text
exactGroupBComplete                   = false
v11BlockLiveOutFrontierComplete       = false
v11GlobalWriterUniverseComplete       = false
backingPointerWriterClosureComplete   = false
managerThisPromotionAllowed           = false
```

## create-new、连续 byte verify 与 mutation gate

在冻结 IDA 中先 create-new，再不编辑 analyzer/JSON 连续执行两次。修正后的
三次实际输出为：

```text
PASS created exact_contexts=16 new_writers=0 new_helpers=0 remaining_liveouts=32 mutations=34
PASS verified exact_contexts=16 new_writers=0 new_helpers=0 remaining_liveouts=32 mutations=34
PASS verified exact_contexts=16 new_writers=0 new_helpers=0 remaining_liveouts=32 mutations=34
```

34 个 semantic mutation 覆盖 exact16/15、接受 A40、保留 B32、A/B/C union88、
禁止消费 v12、helper/source/writer/return 漂移、删减 source/A/B/remaining 行、
非法完成 B/global frontier、manager-this promotion、权限与只读边界，全部
fail closed。

## create-new 产物

- machine JSON：
  `out/cs-t0/native-join-manager-this-manager-tuple-liveouts-c-v18.json`
  - SHA-256 `246CF36EB86BEA22591CCBB5CF506060F63CCAB4DCE7BD470BBF43924143FD44`
- IDA reproducer：
  `out/cs-t0/analyze-manager-this-manager-tuple-liveouts-c-v18.py`
  - SHA-256 `7BF7BF002C462DE5244B6C591C03C124D6BCCC4078D788DEB23D338D4BD1BE40`
- handoff：
  `docs/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-liveouts-c-v18-handoff.md`

复现时在已打开的冻结 IDA 中执行上述 analyzer。已有 JSON 时只允许
byte-for-byte verify；语义或字节漂移均非零失败。

## 执行边界与独立测试交接

全程只读取冻结 JSON、磁盘 PE hash 和已打开 IDA 的反汇编、CFG、xref、函数
范围与字节；没有启动或读取游戏进程，没有附加调试器，没有修改 IDB、生产
代码、测试、历史接受证据、索引或状态文档，也没有执行 runtime mutation。

本文件是逆向交接，不是独立测试验收。独立测试智能体应重算三份 SHA，在
冻结 IDA 上复跑 analyzer，核对 exact16/15、A40+B32+C16=88、switch-jmp
`0x747C96` successor traversal、214/253/71、14 个 value-only call、34 个
mutation、remaining exact32 与所有 promotion/permission false。不得在该
测试中扩展或改判 Group B。
