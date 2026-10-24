> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple helpers-a v13 逆向交接

## 结论

本轮严格消费 v11 `minimalHelperTargetVas` 的前 12 个 target，并逐项绑定
其 `consumerManifest` source callsite。局部结果为 **12/12 resolved、0
unresolved**，从 v11 未分类集合中消除 **114** 个 source callsite。

这是局部 frontier reduction，不是 manager-this 完整提升。v11 仍保留
24 个 helper target、168 个 source callsite 和 88 个 block-live-out alias；
因此全局 writer universe、backing closure 和 `managerThisPromotionAllowed`
继续为 false。

## v12 失败记录与 v13 修复

v12 首次生成后，在逐指令复核中发现两个记录地址指向 push/test 而非
call 指令：`0x0060D1A3` 应为 `0x0060D1A7`，`0x009B9419` 应为
`0x009B9417`。修正 analyzer 后其自哈希和期望 JSON 同时变化；独立复跑
因此在 byte-for-byte 比较处拒绝旧 JSON。v12 已按
`FAILED_INTERMEDIATE_CREATE_NEW_DRIFT` 保留，不作为证据输入，也未被
覆盖或提升。

v13 在任何 JSON 创建之前即冻结上述两个修正及 analyzer 全部字节。随后
同一冻结 IDA 中连续取得一次 `PASS created` 和一次 `PASS verified`；
第二次没有编辑 analyzer 或 JSON。

## 冻结绑定与执行边界

- v11 JSON SHA-256：
  `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- v11 analyzer SHA-256：
  `287CACD9143432D61E8FB6EEE37453EDDF0702611D84AC618A0DFF737333C873`
- 冻结 `isaac-ng.exe` SHA-256：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- x86 image base：`0x00400000`
- tuple receiver：`DWORD[0x00C71678] + 0x0001BAA8`
- tuple：`begin@+0 / end@+4 / capacityEnd@+8`

仅使用已打开 IDA 数据库做静态只读分析。没有启动或读取游戏进程，
没有附加调试器，没有修改 IDB、生产代码、测试、既有证据、状态或索引。

## exact-12 分区

| target | v11 source 数 | tuple 分类 | 关键依据 |
| --- | ---: | --- | --- |
| `0x00417840` | 6 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end 并返回 `(end-begin)>>2`；无 call/store。 |
| `0x0044BFB0` | 58 | `READ_ONLY_MANAGER_TUPLE` | 遍历 begin..end；`0x44C01D/0x44C038` 只把解引用后的 player/player relation 传给 `0x771550`。 |
| `0x004F8940` | 8 | `READ_ONLY_MANAGER_TUPLE_TRANSITIVE_FORWARD` | 保持 ECX 到 `0x9BE080`；后者只读 tuple，后续 callee 只接收 global/player，不接收 tuple。 |
| `0x0060D180` | 2 | `READ_ONLY_MANAGER_TUPLE_FIXED_MEMBER_ACCESS` | ECX 先加 `0x60`，只读四个 fixed member；`0x9B9570` 接收独立 caller buffer。 |
| `0x009B92C0` | 11 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end 做 empty/bounds check，返回解引用后的 player。 |
| `0x009B9310` | 5 | `READ_ONLY_MANAGER_TUPLE` | 只读遍历 tuple，返回 player/null；无 tuple store。 |
| `0x009B9380` | 4 | `READ_ONLY_MANAGER_TUPLE` | 只读遍历 tuple；唯一间接 call 的 receiver 是 `player+0x370`。 |
| `0x009B9450` | 1 | `READ_ONLY_MANAGER_TUPLE` | 只读 begin/end 做线性查找，返回 scalar index。 |
| `0x009B9480` | 13 | `READ_ONLY_MANAGER_TUPLE` | 只读过滤计数，返回 scalar count。 |
| `0x009B94E0` | 1 | `READ_ONLY_MANAGER_TUPLE_FIXED_MEMBER_WRITER` | manager 写只发生在 `+0x60+4*n`，`n<4`；tuple 三字段不触达。 |
| `0x009B98E0` | 3 | `BEGIN_END_CAPACITY_WRITER` | 原地 `0x9B99A3 [end]=player`、`0x9B99A5 end+=4`；满载走 allocator-backed growth。 |
| `0x009BAAA0` | 2 | `READ_ONLY_MANAGER_TUPLE_FIXED_MEMBER_WRITER` | receiver-relative 访问从 `+0x0C` 开始，四个 `0x14`-byte slot record 内搬移/写入；tuple 不触达。 |

按 tuple 效果合计：11 个 read-only、1 个 begin/end/capacity writer、0 个
end-only、0 个 `+0xD0` nested-vector、0 个 non-manager alias false positive、
0 个 unresolved forward。

## `0x009B98E0` 的 begin/backing 闭包

`sub_9B98E0` 的 manager tuple 只有两种写入结果：

1. **in-place**：当 `end != capacityEnd`，`0x9B99A3` 写 `[end]`，
   `0x9B99A5` 令 `end += 4`；begin 与 capacityEnd 不变。
2. **growth**：当 `end == capacityEnd`，`0x9B99B2` 将同一 tuple receiver
   传给 `sub_42C920`。后者在 `0x42C96A` 计算 `4*newCapacity`，
   `0x42C971` 调用 `sub_40CF00`，然后 `0x42C9E3` 调用 `sub_41D150`。
   安装结果为：

```text
begin       = sub_40CF00 allocation
end         = begin + 4 * size
capacityEnd = begin + 4 * capacity
```

本 helper 不存在 inline/SSO 或 external-buffer begin install。返回值是新建
player element，不是 tuple alias。这个结论只覆盖 `0x009B98E0`，不能替代
剩余 24 helper 与 88 block-live-out 的全局穷尽。

## source partition 与 fail-closed 复现

JSON 的 `sourcePartitions` 保存 12 个 target 的全部 114 个 v11 原始行，
包括精确 source callsite、source function、register/stack alias 与反汇编。
分项数量为：

```text
6 + 58 + 8 + 2 + 11 + 5 + 4 + 1 + 13 + 1 + 3 + 2 = 114
```

在冻结、已打开的 IDA 数据库内运行：

```text
D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-manager-tuple-helpers-a-v13.py
```

首次仅 create-new JSON；再次运行要求 byte-for-byte 相等。实际两次输出：

```text
PASS created exact_targets=12 resolved=12 unresolved=0 source_callsites=114 remaining_helpers=24 remaining_sources=168 mutations=16
PASS verified exact_targets=12 resolved=12 unresolved=0 source_callsites=114 remaining_helpers=24 remaining_sources=168 mutations=16
```

16 个 semantic mutation 覆盖 exact target/source 数、source partition、
writer/read-only 误分类、transitive edge 删除、allocator relation 篡改、
remaining frontier 清零、全局 writer/manager-this 非法提升、权限与 debugger
状态；全部 fail closed。

## create-new 产物

- `out/cs-t0/native-join-manager-this-manager-tuple-helpers-a-v13.json`
  - SHA-256 `322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- `out/cs-t0/analyze-manager-this-manager-tuple-helpers-a-v13.py`
  - SHA-256 `8650161E0EF73C5F2D90416CC338A8A4E6B0F9E045ADAADDA34A45BD6CA0D6C0`
- `docs/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-helpers-a-v13-handoff.md`
  - 本文件 SHA-256 由交接后计算。

## 独立测试交接

本文件是逆向证据交接，不是最终测试验收。独立测试智能体应重新计算三份
SHA，在冻结 IDA 目标上再次执行 analyzer，核对 exact-12/114 source 的无
重叠分区、`0x9B98E0` 的唯一 writer 分类、`sub_40CF00 -> sub_41D150`
backing relation、11 个 read-only 分类、16 个 mutation，以及全局门仍保持
false；不得把本轮自检当作独立验收。


