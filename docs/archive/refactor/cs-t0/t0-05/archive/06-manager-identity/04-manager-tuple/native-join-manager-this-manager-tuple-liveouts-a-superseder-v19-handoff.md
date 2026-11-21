> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple live-outs Group A successor-first v19 逆向交接

## 结论

本轮以新产物取代 v16 对 Group A 的固定点实现结论。它不根据 v16 machine
JSON 的 terminal 标签推断，而是从 v15 原样保存的 v11 live-out universe
重新筛选 exact-40 seed，用修正后的 successor-first 引擎重新遍历每个可达
`(block,state)`，显式统计所有满足下列条件的控制流：

```text
instruction mnemonic = jmp
direct target         = none / indirect
IDA CFG successors    = non-empty
```

严格重跑结果：

```text
exact contexts                         = 40/40
exact functions                        = 16
reachable indirect-jmp hit rows        = 0
reachable indirect-jmp hit paths       = 0
reachable indirect-jmp hit seeds       = 0
reachable indirect-jmp unique EAs      = 0
tuple field writers                    = 0
unresolved tuple-object forwards       = 0
new helper targets / sources / writers = 0 / 0 / 0
```

因此 A 的零命中是修正引擎逐 seed、逐指令执行所得，不是旧 JSON 文本推断。
原 v16 的一个 EAX return alias 仍被 exact-31 xref / recursive-SCC caller 审计
闭合。A 外的 B+C exact48 未被分析或改判，继续保持 `UNKNOWN`；所有 promotion
和 permission 仍为 false。

## superseder 与冻结绑定

- v11 primary/writer/live-out JSON：
  `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- 已接受 helpers-a v13：
  `322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- 已接受 helpers-b v14：
  `A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`
- 已接受 helpers-c v15：
  `B07501A2D6DD6492E6834ECC64A5B23FF3B125DB415643B87F04DE4FB53EDCAD`
- 被取代的 liveouts-a v16 machine JSON：
  `511300FC52958E66EEA59E1059663695DF2491480C5E87772E4FADAF15E458D7`
- 被取代的 v16 analyzer source：
  `465EB61B4F66A04577F5839D41F46BF3FC08824BD348CA8BB8B5E79534B37BAD`
- 冻结 `isaac-ng.exe`：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- image base：`0x00400000`
- tuple：`DWORD[0x00C71678]+0x0001BAA8`
- fields：`begin@+0 / end@+4 / capacityEnd@+8`

v16 只作为 superseded input 做 hash、source-row 和 remaining-row 一致性绑定；
`supersededV16ConclusionConsumed=false`。v19 从 v15/v11 重新取得 A40 和 outside48，
不把 v16 的 terminal adjudication 当接受事实。失败的 helpers-a v12 没有进入
binding，也没有被读取为接受证据。

## exact-40 / outside-48 分区

Group A 每函数 context 数：

```text
00419E70(5) 0090E2A0(4) 007A6450(3) 007F2800(3) 008EF990(3)
0041A790(2) 0041AB50(2) 005CB480(2) 006F5850(2) 006FE410(2)
00739370(2) 0073E0A0(2) 0075F0E0(2) 00763570(2) 007AE4B0(2)
007B0BE0(2)
```

机器 JSON 原样嵌入 A40 source rows 和未裁决 B+C48 rows，并固定两组 canonical
SHA。v19 不依赖函数名外推或旧 terminal 行，也不把 A 的零命中扩展到 B/C。

## successor-first 修正

v16 的原分支先判断 `direct_target`。当间接 `jmp` 的 target 为 `None` 时，
即使当前 IDA block 有 CFG successors，也可能进入 tail-exit adjudication。v19
读取并验证 v16 analyzer source hash，然后只在内存中对这个唯一明确的 source
line 做 fail-closed patch：

1. 先计算 `indirect_cfg_jump = jmp && direct_target is None && succs non-empty`；
2. 每次可达命中都把 instruction VA 和 successor VA 列表写入 observation；
3. 把这种 jump 视为当前函数 CFG transfer；
4. block 结束后将全部 successor 与当前 alias state 入队；
5. 只有不满足内部 CFG 条件的 jump 才能进入 tail/call 边界分类。

这套引擎实际重跑 A40 后，命中列表为空。空列表仍被下列互相核对的字段约束：
`hitRowCount`、`hitPathCount`、`hitSeedCount`、`uniqueInstructionCount`、
`uniqueInstructionVas` 和 `hits`。

## fixed-point 与调用结果

修正后累计：

```text
visited (block,state) = 1239
CFG transitions       = 1215
fixed-point revisits  = 54
```

已接受 helper 的 tuple-object call 命中：

```text
0x009B92C0 = 6
0x009B9310 = 7
0x009B9480 = 4
0x009BAAA0 = 8
```

这些 target 均由已接受 v13 分类。另有 25 次 `0x00A112C0` call 只接收已经
解引用的 end/backing value，不接收 tuple object address。

`sub_7A6450` 的唯一 EAX return alias 仍按 v16 的独立边界方法重新执行：

```text
code xrefs          = 31
recursive SCC edges = 4
external callers    = 27
```

4 条 SCC edge 为 `0x7A65B1/0x7A65C8/0x7A65DF/0x7A6647`；27 个外部 caller
均在写 tuple、再次 return 或未知 forward 前覆盖/丢弃 EAX。严格重跑没有产生
第二个 return source。

## 保留阻断

v19 只关闭 Group A。机器 JSON 保留：

```text
remaining helper targets     = 0
remaining helper sources     = 0
remaining block live-outs    = 48
remaining status             = UNKNOWN_EXACT_B_PLUS_C_ONLY
```

以下值固定为 false：

```text
exactGroupBPlusCComplete             = false
v11BlockLiveOutFrontierComplete      = false
v11GlobalWriterUniverseComplete      = false
backingPointerWriterClosureComplete  = false
managerThisPromotionAllowed          = false
```

## create-new、连续 byte verify 与 mutation gate

在同一冻结 IDA 数据库中先 create-new，再不编辑 analyzer/JSON 连续复跑两次：

```text
PASS created exact_contexts=40 indirect_rows=0 indirect_seeds=0 new_writers=0 remaining_liveouts=48 mutations=38
PASS verified exact_contexts=40 indirect_rows=0 indirect_seeds=0 new_writers=0 remaining_liveouts=48 mutations=38
PASS verified exact_contexts=40 indirect_rows=0 indirect_seeds=0 new_writers=0 remaining_liveouts=48 mutations=38
```

38 个 mutation 覆盖 superseded conclusion 非法消费、A40/16、outside48、helper
分区、writer/unresolved/return closure、indirect hit rows/paths/seeds/EAs、删减
source/remaining/caller 行、非法完成 B+C/global frontier、promotion、权限和只读
边界，全部 fail closed。

## create-new 产物

- machine JSON：
  `out/cs-t0/native-join-manager-this-manager-tuple-liveouts-a-superseder-v19.json`
  - SHA-256 `ED54DA25E98F599A1AC84BFFF1D4EF9861D53522D203715641A3A4588A5750A4`
- IDA reproducer：
  `out/cs-t0/analyze-manager-this-manager-tuple-liveouts-a-superseder-v19.py`
  - SHA-256 `834A331B2331FB94A1F1125661B89C9B7E285EF5868843C49177FF158936420A`
- handoff：
  `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-liveouts-a-superseder-v19-handoff.md`

## 执行边界与独立测试交接

全程只读取冻结 JSON、磁盘 PE hash 与已打开 IDA 的反汇编、CFG、xref、函数
范围和字节。没有启动或读取游戏进程，没有附加调试器，没有修改 IDB、生产
代码、测试、v16 或其他历史证据、索引/状态文档，也没有 runtime mutation。

本文件是逆向交接，不是最终测试验收。独立测试智能体应重算三份 SHA，在
冻结 IDA 上复跑 analyzer，确认不是读取 terminal 文本得到零命中；核对 A40
全部 seed 的 successor-first 遍历、indirect hit 0/0/0/0、1239/1215/54、
return 31=4+27、38 mutation、outside exact48 与 promotion/permission false。
不得在该测试中扩展或改判 B/C。
