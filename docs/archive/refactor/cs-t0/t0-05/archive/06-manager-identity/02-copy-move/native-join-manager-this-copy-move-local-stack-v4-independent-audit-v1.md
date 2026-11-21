> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this copy/move local-stack v4 独立审计

## 结论

独立审计结论为 **FAIL：v4 不接受**。

集合身份本身正确：v3 的精确 `UNKNOWN 100` 与 v4 的 `closed 100` 完全相等，均为 100 个唯一 RVA，没有新增、遗漏或重复；70 个 owner 的集合也一致。两处已登记的 Hex-Rays 漂移 `0x003D830A`、`0x003D839C` 经原始指令复核只改变变量名或指针算术渲染，未改变实际 destination/length 语义。

失败发生在证明本体：reproducer 给 70 个 owner 人工分配七个 family，为每行复制对应的三段公共叙述，然后无条件写入 `capacityAndSpanClosure.closed=true`。它没有逐行检查 `destinationOffset + length <= allocation/object/capacity`，也没有闭合 caller/constructor/pointer-writer universe。90/100 行没有 special anchor；10 条有 anchor 的行也只检查文本出现，不检查支配关系或不等式。因此本审计不接受 v4 的任何新增 closure，v3 的 exact UNKNOWN 100 必须全部保留。

## 三个精确反例

### `0x002EE04F`

owner `0x002EDFA0` 向 32-byte stack object `v8[2]` 执行 `memcpy(v8, ..., 4*n4-4)`。复制路径只保证 `n4 != 0`；要容纳在对象内还必须证明 `n4 <= 9`。原始控制流中没有该支配约束或 clamp：`n4 < 4`、`n4 >= 8` 只选择传给另一调用的参数值。reproducer 的三个 special anchors 仅确认对象声明、调用文本和长度文本存在，`n4=10 -> length 36` 的最小反模型仍成立。

这不是声称 `n4=10` 已被实机观察，而是证明 v4 没有排除它，因而不能将该 RVA 从 UNKNOWN 提升。

### `0x00663CFC`

destination 为 `Buf[57]+1`，length 为 `((Buf[51] * byte@Buf+0x103)+7)>>3`。owner 只形成指针、计算 Size、把 Size 记录到 `Buf[63]` 后调用 `memcpy`；没有本地 allocation extent 或 pre-copy capacity comparison。v4 的 anchors 只证明这些文本存在，完全没有证明 `capacity >= 1+Size`，也没有追踪 `Buf[57]` 的全部 writer。

### `0x004FCD78`

这是 caller-supplied `[a1,a2)` 范围内的 sort shift，局部长度关系只说明写入相对 `a1` 不越过该 caller range；它不证明 `a1` 的地址域。可达 direct caller `0x004F61D0` 传入 `*v3` 与 `v3[1]`，但 v4 只列 caller RVA，不追该 tuple 的 constructor/writer。`a1=imageBase+0x00871674` 且当前 iterator 至少为 `a1+4` 时，destination 恰为 slot RVA `0x00871678`，同时不违反局部 half-open-range 关系。

## caller / constructor / writer universe

70/70 owner 位于 corrected 5,646-function closure 内，但 row 自带的 caller 列表显示：33 行含 closure 外 direct caller，共 351 对、186 个唯一 outside caller。v4 每行却固定写着“all executable producers are inside closure”，该叙述与自身数据并不一致。

更关键的是，`sharedHelperProofRvasUsedLocally` 仅通过“helper 名称是否出现在 owner decompile 文本中”产生；它不证明 helper 支配精确 callsite，不证明传入对象必经 helper，也不枚举其它 pointer-bearing writer。因此 corrected component 的零 exact/split formation 与只读 self-image-base producer只能作为组件事实，不能单独排除 caller-provided/field-loaded pointer 已经等于 `imageBase+split`。

## Hex-Rays 漂移

- `0x003D830A`：call bytes `E8 AE 85 31 00`。原始指令从同一对象的 `+0x1FA4/+0x1FA8` 读取 begin/end，形成 `dest=begin`、`src=begin+4`、`len=end-(begin+4)`；`a6 -> v1087` 只是 implicit-object alpha rename。
- `0x003D839C`：call bytes `E8 1C 85 31 00`。原始指令形成 `dest=begin+4`、`src=begin`、`len=end-begin-4`；`v17+4` 与 `_DWORD* v17+1` 等价。

不过，本轮从同一 exact IDB 执行 no-write reproducer prefix 时又出现第三条 ctree rendering drift `0x004FCD78`（call bytes 仍为 `E8 40 3B 1F 00`）。其原始 sort-range 语义没有变化，但当前 IDB 已不能复现 handoff 所写“恰好两条 drift”的渲染计数。这进一步说明 ctree 文本不能作为证明 oracle。

## Mutation / fail-closed

四种结构性 component mutation 会被拒绝：非零 split count、self-image-base producer 改成 write-capable、indirect/split closure=false、reachable bulk count 647→646。

语义 mutation 不 fail closed：

- 把 `0x002EDFA0` 与 `0x006DCAA0` 的 proof class 对调，70-owner union 仍相等，现有 partition check 会接受；
- 把 fixed-buffer 的 bound narrative 改成矛盾文本，现有逻辑没有消费者，仍会为行写 `closed=true`；
- reproducer 没有 `--verify-existing`，也不读取已有 v4 内容。对已有输出调用 `main()` 只会在内容校验前因 create-new 路径已存在而退出。

因此 hash/shape mutation 有防护，但 proof semantic 与最终 report mutation 没有独立 fail-closed verifier。

## 精确处置

```text
exact v3 UNKNOWN set identity                    PASS (100/100)
declared two Hex-Rays drift semantics            PASS (rendering only)
row-specific offset+length/capacity closure      FAIL
caller/constructor/pointer-writer closure        FAIL
semantic mutation fail-closed                    FAIL
overall v4 acceptance                            FAIL
accepted new closure from v4                     0
v3 UNKNOWN retained                              100
merge with frozen v3 positive 39                 NOT ALLOWED
manager-this / whole-image promotion              NOT ALLOWED
```

下一版必须 create-new：每个 exact RVA 要保存并验证具体 object/allocation extent、destination offset、length、支配性不等式；对 caller/field/`this` 来源要列全相关 constructor 与 pointer-bearing writer，并由独立 verifier 对 existing report 做负向 mutation。不能继续用 family 名称、C++ 容器惯例、零 observed overlap 或 corrected formation=0 单独替代这些证明。

## 安全边界

本轮只读目标 PE/IDA 与现有证据；没有启动游戏、打开进程、附加调试器或写 IDB。没有修改候选 v4、production、tests、v1-v7、状态或索引。只创建本审计 machine JSON 与本 handoff。
