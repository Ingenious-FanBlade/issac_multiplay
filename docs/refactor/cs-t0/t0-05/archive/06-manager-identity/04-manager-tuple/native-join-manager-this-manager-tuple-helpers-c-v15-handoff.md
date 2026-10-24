> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple helpers-c v15 逆向交接

## 结论

本轮严格消费 v11 `minimalHelperTargetVas` 的最后一组 exact-12（索引
`24..35`），从 v11 原始 `consumerManifest` 重算并逐 source 核对全部 41 个
callsite。结果为 **12/12 target resolved、41/41 source classified、0
unresolved**。

与已接受 helpers-a v13、helpers-b v14 合并后，v11 的 36 个 helper target
和 282 个 helper source 均已分类，helper frontier 为 `0/0`。但 88 个
`UNCLASSIFIED_BLOCK_LIVE_OUT` alias 原样保持 `UNKNOWN`；本轮没有分析、删减
或提升它们。因此全局 writer universe、backing pointer closure 和
`managerThisPromotionAllowed` 仍为 false。

## 冻结绑定与执行边界

- v11 主 JSON SHA-256：
  `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- 已接受 helpers-a v13 JSON SHA-256：
  `322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- 已接受 helpers-b v14 JSON SHA-256：
  `A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`
- **失败的 v12 未进入 v15 bindings，也未被读取为接受证据。**
- 冻结 `isaac-ng.exe` SHA-256：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- x86 image base：`0x00400000`
- tuple receiver：`DWORD[0x00C71678] + 0x0001BAA8`
- tuple：`begin@+0 / end@+4 / capacityEnd@+8`

全程只读使用已打开的冻结 IDA 数据库；没有启动或读取游戏进程，没有附加
调试器，没有修改 IDB、生产代码、测试、既有证据、索引或状态文档。

## exact-12 / exact-41 分区

source 数严格由 v11 原始行重算：

```text
1 + 1 + 16 + 1 + 8 + 2 + 1 + 2 + 4 + 3 + 1 + 1 = 41
```

| target | v11 source 数 | tuple 分类 | 关键依据 |
| --- | ---: | --- | --- |
| `0x009BEA10` | 1 | `READ_ONLY_MANAGER_TUPLE_PLAYER_MEMBER_WRITER` | `0x9BEA15/17/2B` 只读 begin/end；唯一 store `0x9BEA25` 写解引用 player `+0x410`。 |
| `0x009BEA40` | 1 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end 和 player 字段，无 call/store。 |
| `0x009BEA80` | 16 | `READ_ONLY_MANAGER_TUPLE_PLAYER_SCALAR_FORWARD` | 遍历 begin/end；`sub_7CB6E0` 只接收 player 或 `player+0x1E6C` 与 scalar `n86`。 |
| `0x009BEB30` | 1 | `READ_ONLY_MANAGER_TUPLE_PLAYER_CALLEE` | 遍历 begin/end；`sub_7DB6B0` 的 ECX 是解引用 player。 |
| `0x009BEBA0` | 8 | `READ_ONLY_MANAGER_TUPLE_WITH_NESTED_PLUS_D0_READER` | `0x9BEBEC/F4` 形成精确 global tuple 并读取 begin/end；`this+0xD0/+0xD4` 是独立 nested vector，只读。 |
| `0x009BEE80` | 2 | `READ_ONLY_MANAGER_TUPLE_BACKING_ELEMENT_WRITER` | `0x9BEEDA..F7` 交换已有 `begin[index]` 元素，`0x9BEFA7..EB` 在线性 span 内替换匹配元素；不写 tuple 字段地址。 |
| `0x009BF530` | 1 | `READ_ONLY_MANAGER_TUPLE_FIXED_MEMBER_WRITER` | 读取 begin/end；receiver-relative store 仅在 `+0x0C..+0x54`，不触达 tuple。 |
| `0x009BF930` | 2 | `READ_ONLY_MANAGER_TUPLE_PLAYER_SCALAR_FORWARD` | 遍历 begin/end；`sub_7706E0` 接收 player、scalar 619 和 zero。 |
| `0x009BF990` | 4 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end、player/parent 字段，无 call/store。 |
| `0x009BFA00` | 3 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end、player/parent 字段，无 call/store。 |
| `0x009BFA70` | 1 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end、player/parent 字段，无 call/store。 |
| `0x009BFB20` | 1 | `READ_ONLY_MANAGER_TUPLE_PLAYER_CALLEES` | 只读 begin/end；callee 只接收 player/related player 或 logger scalar，不接收 tuple。 |

按 tuple 三字段效果汇总：12 个 target 全部只读；0 个 end-only writer、0 个
begin/capacity writer、0 个 non-manager false positive、0 个 unresolved
forwarder。更细的互斥子类为：4 个纯只读、1 个 player-member writer、4 个
player/related-object callee、1 个 nested `+0xD0` reader、1 个 backing-element
writer、1 个 fixed-member writer。

`0x009BEE80` 的 backing-element rewrite 不等于 begin install：它只通过已读
begin 指针写已有槽位；`0x009BF530` 的 receiver stores 最小偏移为 `+0x0C`，
也不能计作 `begin@+0 / end@+4 / capacityEnd@+8` 写入。每个函数的完整 IDA
function extent 与全字节 SHA 均由 analyzer 固定，JSON 还保存全部 41 个 v11
原始 source 行和逐 source adjudication。

## begin install 的限域结论

本 exact-12 没有 begin install。合并已接受 helper 分区后，唯一会改变 begin
的 helper 是 v13 的 `0x009B98E0` 满载增长路径；其关系继续严格绑定为：

```text
begin       = sub_40CF00 allocation
end         = begin + 4 * size
capacityEnd = begin + 4 * capacity
install     = sub_41D150
```

结合 v11 已接受 primary writer，当前已分类 primary/helper 中实际出现的 begin
值类只有 `NULL`，或 `sub_40CF00` allocation 经 `sub_41D150` 安装；没有发现
inline/SSO/external-buffer begin install。

这仍不是 v11 全局 begin-install 闭包。88 个 block-live-out alias 尚未分类，
其中仍可能存在额外 writer/forwarder，所以
`allV11BeginInstallClosureComplete=false`、
`v11GlobalWriterUniverseComplete=false`。

## helper frontier 与保留阻断

v13 消除 114 source，v14 消除 127 source，本轮消除 41 source：

```text
114 + 127 + 41 = 282

remaining helper targets   = 0
remaining helper sources   = 0
remaining block live-outs  = 88 (UNKNOWN)
```

JSON 原样嵌入 v11 的 88 个 `blockLiveOutAliases`，并将状态固定为
`UNKNOWN_BLOCK_LIVE_OUT_ONLY`。本轮没有把 helper 完成误写成 manager-this
完成，也没有授权读取进程、runtime mutation、生产修改或实机操作。

## create-new、连续 byte verify 与 mutation gate

在同一冻结 IDA 中先 create-new，再在没有编辑 analyzer/JSON 的情况下连续
执行两次 byte-for-byte verify。三次实际输出为：

```text
PASS created exact_targets=12 resolved=12 unresolved=0 source_callsites=41 cumulative_sources=282 remaining_helpers=0 remaining_sources=0 block_live_outs=88 mutations=24
PASS verified exact_targets=12 resolved=12 unresolved=0 source_callsites=41 cumulative_sources=282 remaining_helpers=0 remaining_sources=0 block_live_outs=88 mutations=24
PASS verified exact_targets=12 resolved=12 unresolved=0 source_callsites=41 cumulative_sources=282 remaining_helpers=0 remaining_sources=0 block_live_outs=88 mutations=24
```

24 个 semantic mutation 覆盖 exact target/source 数、source partition、
backing-element/fixed-member/nested 分类、helper/source frontier 清零、累计
282、begin-install 非法提升、88 live-out 清零、全局 writer/backing/manager-this
提升及权限漂移，全部 fail closed。

## create-new 产物

- `out/cs-t0/native-join-manager-this-manager-tuple-helpers-c-v15.json`
  - SHA-256 `B07501A2D6DD6492E6834ECC64A5B23FF3B125DB415643B87F04DE4FB53EDCAD`
- `out/cs-t0/analyze-manager-this-manager-tuple-helpers-c-v15.py`
  - SHA-256 `0AC24977D4E25CA942F59791A1555ADF4B7632A63FE9FCACADE737D16A112D7B`
- `docs/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-helpers-c-v15-handoff.md`
  - 本文件 SHA-256 由交接后计算。

## 独立测试交接

本文件是逆向证据交接，不是最终测试验收。独立测试智能体应重新计算三份
SHA，在冻结 IDA 目标上复跑 analyzer，核对 exact-12/41 source 的无重叠
分区、v13/v14/v15 合集恰为 36 target/282 source、12 个 tuple-field
read-only 分类、`0x009BEE80` backing element 与 tuple field 的区分、
`0x009BF530` 的 `+0x0C..+0x54` fixed-member 范围、24 个 mutation，以及
helper/source 为 0 时 88 个 live-out、所有权限和 promotion gate 仍保持
false。不得把本轮自检当作独立验收，也不得在该测试中扩展或改判 live-out。
