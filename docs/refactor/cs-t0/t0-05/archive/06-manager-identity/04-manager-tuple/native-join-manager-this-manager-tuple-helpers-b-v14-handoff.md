> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple helpers-b v14 逆向交接

## 结论

本轮严格消费 v11 `minimalHelperTargetVas` 的第二组 exact-12（索引
`12..23`），逐项绑定全部 v11 `consumerManifest` source。结果为
**12/12 target resolved、127/127 source classified、0 unresolved**。

这只是 helper frontier reduction，不是 `0x005B91EF` 或 manager-this
整体提升。与已接受 helpers-a v13 合并后，v11 尚保留 12 个 helper target、
41 个 source callsite 和 88 个 block-live-out alias；因此 writer universe、
backing closure 和 `managerThisPromotionAllowed` 继续为 false。

## 冻结绑定与边界

- v11 JSON SHA-256：
  `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- 已接受 helpers-a v13 JSON SHA-256：
  `322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- **失败的 v12 不在本证据 bindings 中。**
- `isaac-ng.exe` SHA-256：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- x86 image base：`0x00400000`
- tuple receiver：`DWORD[0x00C71678] + 0x0001BAA8`
- tuple：`begin@+0 / end@+4 / capacityEnd@+8`

全程只读使用已打开的冻结 IDA 数据库；没有启动或读取游戏进程，没有附加
调试器，没有修改 IDB、生产代码、测试、既有证据、索引或状态文档。

## exact-12 / exact-127 分区

| target | v11 source 数 | tuple 分类 | 关键依据 |
| --- | ---: | --- | --- |
| `0x009BAC00` | 1 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end，读取 `player+0x1618`，返回 player/null。 |
| `0x009BB840` | 4 | `READ_ONLY_MANAGER_TUPLE_PLAYER_AND_GLOBAL_CALLEES` | 只读遍历玩家；callee 接收 player 或独立 local/global list。 |
| `0x009BB960` | 1 | `READ_ONLY_MANAGER_TUPLE_PLAYER_AND_GLOBAL_WRITES` | tuple 只读；后续写入 n60 global，不通过 tuple receiver。 |
| `0x009BBA30` | 1 | `READ_ONLY_MANAGER_TUPLE_PLAYER_AND_SCRATCH_WRITES` | 所有 `this+0/+4` 仅重读玩家 span；写入 player 或 `C94360/C94364` scratch。 |
| `0x009BC000` | 11 | `READ_ONLY_MANAGER_TUPLE_PLAYER_AND_GLOBAL_CALLEES` | tuple 只读；callee 接收 player 或独立 local/global list。 |
| `0x009BC120` | 2 | `END_ONLY_MANAGER_TUPLE_WRITER_WITH_NESTED_PLUS_D0_AND_FIXED_MEMBER` | `0x9BC1FB -> sub_9BA000`；manager 路径 `0x9BA5F9` memmove 后 `0x9BA5FE end-=4`。 |
| `0x009BC690` | 1 | `READ_ONLY_MANAGER_TUPLE_CALLER_BUFFER_WRITER` | 只读 tuple 计数，全部 store 通过 caller `a2`。 |
| `0x009BE080` | 83 | `READ_ONLY_MANAGER_TUPLE_IGNORED_DUPLICATE_ALIAS_ARGUMENT` | this 只读 begin/end；83 个来源逐一完成 ABI 分类，见下节。 |
| `0x009BE140` | 1 | `READ_ONLY_MANAGER_TUPLE_CALLER_BUFFER_WRITER` | 遍历 tuple，callee 接收 player/scalar，store 只写 caller `a4`。 |
| `0x009BE630` | 6 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end 和 player 字段，无 call/store。 |
| `0x009BE6B0` | 13 | `READ_ONLY_MANAGER_TUPLE_PLAYER_SCALAR_FORWARD` | callee 接收 player 与 scalar ID，不接收 tuple。 |
| `0x009BE750` | 3 | `READ_ONLY_MANAGER_TUPLE_PLAYER_SCALAR_FORWARD` | global/player 与 scalar forward；tuple 只读并返回 count。 |

计数严格为：

```text
1 + 4 + 1 + 1 + 11 + 2 + 1 + 83 + 1 + 6 + 13 + 3 = 127
```

按 tuple effect 汇总：11 个 read-only target、1 个 end-only target、0 个
begin/capacity writer、0 unresolved。`0x009BC120` 的 `0x9BC144` 另写
receiver `+0x7C` fixed member；`sub_9BA000` 的 `0x9BA135/13A` 另处理
`receiver+0xD0/+0xD4/+0xD8` nested vector。两者都不能误计为 manager
tuple begin/capacity 写入。

## `0x009BE080` 的 83/83 全来源 ABI 证明

v11 的 83 行都同时记录：

1. `ECX == GameRoot+0x1BAA8`，即 tuple 作为 this；
2. 一个同 offset 的 `push ecx` stack alias。

v14 没有采用代表样本。复现器对 83 个 source 逐行读取 IDA 指令，验证从
该 `pushVa` 到 call 之间严格只有两个 push：先 `push ecx`，再
`push immediate n327`，然后 direct call `0x009BE080`。按三个 stack 参数
的 thiscall 顺序，重复 tuple 是第二个 stack 参数 `a3`；紧邻 call 的是
第一个参数 `n327`。

`sub_9BE080` 的完整 0xBA 字节由 SHA
`D179CCB429D96AD30795A2E1A78066595EC70B320F40D6B6E1DC567F591EA33D`
冻结。它读取 `n327` 和布尔 `a4`，但从未读取 `a3`；`retn 0x0C`
确认三个 stack 参数。函数自身在 `0x9BE0CE/0x9BE0D1` 读取 this 的
begin/end。传递调用中：

- `sub_72FD10` 的 this 是 global `n298+0x2A404`，参数是 scalar `n327`；
- `sub_7706E0` 的 this 是 dereferenced player，参数仍是 scalar `n327`；
- 没有 callee 收到 tuple alias，也没有 tuple store。

JSON 的 `sourceAdjudications` 保留全部 83 个 callsite、source function、
tuple push RVA、scalar push RVA 和 scalar immediate；因此该结论覆盖
83/83，而不是从一个常见 call pattern 外推。

## 剩余 frontier

helpers-a v13 消除 114 source，本轮消除 127 source，累计 241/282。
剩余值由 v11 原集合做差重算：

```text
helper targets = 12
source callsites = 41
block live-outs = 88
status = UNKNOWN
```

剩余 helper target 精确集合：

```text
0x009BEA10  0x009BEA40  0x009BEA80  0x009BEB30
0x009BEBA0  0x009BEE80  0x009BF530  0x009BF930
0x009BF990  0x009BFA00  0x009BFA70  0x009BFB20
```

本轮不得据此单独提升 `0x005B91EF`。即使本组 12/12 已闭合，第三组
helper 和 88 live-out 仍可包含未分类 alias writer/forwarder。

## create-new、byte verify 与 mutation gate

在同一冻结 IDA 中，先 create，再连续执行两次 byte-for-byte verify；三次
输出分别为：

```text
PASS created exact_targets=12 resolved=12 unresolved=0 source_callsites=127 be080_sources=83 remaining_helpers=12 remaining_sources=41 mutations=20
PASS verified exact_targets=12 resolved=12 unresolved=0 source_callsites=127 be080_sources=83 remaining_helpers=12 remaining_sources=41 mutations=20
PASS verified exact_targets=12 resolved=12 unresolved=0 source_callsites=127 be080_sources=83 remaining_helpers=12 remaining_sources=41 mutations=20
```

20 个 mutation 覆盖 exact target/source 数、source partition、83-source
ABI、end-only/nested 分类、begin 误提升、remaining frontier 清零、全局
writer/manager-this 非法提升及权限漂移，全部 fail closed。

## 产物

- `out/cs-t0/native-join-manager-this-manager-tuple-helpers-b-v14.json`
  - SHA-256 `A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`
- `out/cs-t0/analyze-manager-this-manager-tuple-helpers-b-v14.py`
  - SHA-256 `5E521CB170140DD4F0125EC93C485D4EC1CDB27679FE2781BF5A5E55B7141AC8`
- `docs/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-helpers-b-v14-handoff.md`
  - 本文件 SHA-256 由交接后计算。

## 独立测试交接

这是逆向交接，不是最终验收。独立测试智能体应重新计算三份 SHA，在冻结
IDA 目标上复跑 analyzer，核对 exact-12/127 source 的无重叠分区、
`0x009BC120` end-only 与 nested `+0xD0` 区分、`0x009BE080` 的 83/83
argument-order/ignored-a3 证明、20 个 mutation、剩余 12/41/88 frontier，
并确认全部权限与 promotion gate 仍为 false。
