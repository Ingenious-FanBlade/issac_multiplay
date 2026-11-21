> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 fixed-member roots-two v12 交接

## 结论

本轮严格只审计 frozen exact1 `0x005CF2B3` 的两个顶层根 A/B，不扩展到 N298 的其它行。最终决定为：

- `EXACT_ONE_REMAINS_UNKNOWN_WITH_CONCRETE_PROVENANCE_ROOTS`
- 行级计数：`verified=0, unknown=1`
- 六个直接 receiver 中，既有四项重核通过，A/B 两项仍为 UNKNOWN，即 `verifiedDirect=4, unknownDirect=2`
- 不允许 manager-this promotion，也不允许把本产物作为可合并的行证明

这次将 v11 的宽泛缺口缩小到了可复现的最小 provenance 前沿，但没有把局部类型形状或条件 extent 当成全局身份闭包。

## 冻结绑定与边界

- 输入 JSON：`native-join-manager-this-fill-fixed-member-remaining-one-v11.json`
  - SHA-256 `C1F958FB42102A8C02E0993AFD2E3136F020BE92EE1481D2CDCF72E5B815AEF8`
- 输入 reproducer：`analyze-manager-this-fill-fixed-member-remaining-one-v11.py`
  - SHA-256 `741F2833951F11FE97A38BA4F2E24A4AF2CE3AF70E3418FB846A30125CA08EEE`
- 目标 `isaac-ng.exe`
  - SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

仅使用 IDA 只读数据库与磁盘 PE。未启动游戏，未打开、读取或附加进程，未修改 IDB、生产代码、测试或既有证据。

## 调用、构造与字段 writer 全集

IDA code xref 与磁盘 `.text` raw-E8 scan 一致：

- `sub_9CF050` 恰有 6 个直接调用：
  - `0x009061BB, 0x0090C917, 0x009BCC22, 0x009BEFFD, 0x009C7DA2, 0x009C7DC5`
- `sub_9BEE80` 恰有 2 个直接调用：
  - `0x005C4560, 0x007ACFD6`
- `sub_753DA0` 恰有 11 个直接调用：
  - `0x0058ED8F, 0x007ACAAD, 0x007BBF37, 0x007CBE9C, 0x007CF560, 0x009B8FD6, 0x009B9975, 0x009B9D3E, 0x009B9E46, 0x009BCBAF, 0x009BCE87`

11 个 constructor caller 均绑定了同一局部 producer 形状：`push 0x2F00`、调用 allocator、随后调用 `sub_753DA0`。因此，任何确实来自这一 constructor producer universe 的对象都有 `12032` 字节，`receiver=object+0x20EC` 时：

`8428 + (292 + 68) = 8788 <= 12032`，receiver 后可用 `3604` 字节。

磁盘 `.text` 对 disp32 `0x1E6C` 的 155 个出现做了 VA manifest hash；其中 `mov [base+0x1E6C],src` / `mov [base+0x1E6C],imm32` 的 direct writer 恰为 17 个：

- 初始化或清零 6 个：`0x75465D, 0x7574A1, 0x758336, 0x782CBD, 0x782CCD, 0x7A3AA0`
- fresh `0x2F00` mutual-link 6 个：`0x7BBF4F, 0x7BBF68, 0x7CF575, 0x7CF58E, 0x9BCBC4, 0x9BCBF6`
- `Entity_Player+0x1E68` smart-reference bridge 2 个：`0x782F0E, 0x782F1D`
- `sub_9BEE80` transfer-and-clear 3 个：`0x9BF0AD, 0x9BF0BD, 0x9BF0C3`

该 17 项为精确、不重叠的 direct-displacement writer 分区；简单的 `lea/add +0x1E6C` 字段地址逃逸扫描为空。目标 hash、49 个机器码 anchors、writer 与 displacement manifest 一并冻结。这里仍不宣称未经证明的 whole-image indirect alias closure。

## 根 A：缩小到一个 smart-reference producer 前沿

受影响入口：`0x009BCC22`，receiver 为 `DWORD[Entity_Player+0x1E6C]+0x20EC`。

已闭部分：

- null branch 的 fresh allocation 为 `0x2F00`，调用 `sub_753DA0`，条件 extent `8788 <= 12032` 成立。
- 其它 direct nonzero writers 可归入 fresh mutual-link 或 `sub_9BEE80` transfer；清零 writer 不引入外部 pointee。

最小反例为：

- `A1_ENTITY_PLAYER_PLUS_0x1E68_POINTEE_PRODUCER_UNIVERSE`
- `sub_782AF0` 在 `0x782EAE` 取得 `Entity_Player+0x1E68` 字段地址，经 `sub_40C3A0` 得到 pointee，然后在 `0x782F0E/0x782F1D` 把该 pointee 与当前 Entity_Player 互写到 `+0x1E6C`。
- 尚未闭合该 `+0x1E68` pointee 的完整 allocation/type/embedding producer universe，也就不能证明它必有至少 `8788` 字节 extent。

因此根 A 状态为 `UNKNOWN_SMART_REFERENCE_BRIDGE_PRODUCER_UNIVERSE`。即使所有 fresh constructor 路径都安全，这个非 fresh seed 仍足以阻止提升。

## 根 B：ABI 已闭，两个 caller provenance 前沿未闭

受影响入口：`0x009BEFFD`。

`sub_9BEE80` 的 ABI 已闭合：它有两个 stack arguments，`ret 8`；`0x009BEFF2` 从 entry stack 的第一个参数加载对象，加 `0x20EC` 后调用 `sub_9CF050`。因此 receiver 精确为 `first_stack_argument+0x20EC`。

两个直接 caller 的已闭前缀与最小反例如下：

1. `0x005C4560`
   - `0x005C455D` push ESI，故第一个参数为 ESI。
   - ESI 在 `0x005C445F` 从 `[EBP-0x5D0]` 重载；该槽在 `sub_5B39D0` 入口由 ECX-this 保存。
   - 未闭根：`B1_SUB_5B39D0_ECX_THIS_ALLOCATION_TYPE_UNIVERSE`，即这个 ECX-this 的所有 caller/forwarder 与 allocation/embedding extent。

2. `0x007ACFD6`
   - `0x007ACFC5` push EDI，故第一个参数为 EDI。
   - 未闭根：`B2_SUB_7AC7E0_EDI_ALLOCATION_TYPE_UNIVERSE`，即 EDI 的完整 source/forwarder 与 allocation/embedding extent。

如果两者确实都是上述 `0x2F00` Entity_Player class，则 `8788 <= 12032` 条件成立；但相邻字段访问与调用约定不能替代真实 allocation/type provenance 闭包。因此根 B 状态为 `UNKNOWN_TWO_DIRECT_CALLER_PROVENANCE_FRONTIERS`。

## 既有 4/6 入口独立重核

- `0x009061BB` stack：`292 + 68 = 360 <= 1504`
- `0x0090C917` stack：`292 + 68 = 360 <= 1500`
- `0x009C7DA2` first embedded array：`105852 + 8*1500 = 117852 <= 130600`
- `0x009C7DC5` second embedded array：`117852 + 4*1500 = 123852 <= 130600`

嵌入根 extent 也重新计算：

- n298：`4004 + 130600 = 134604 <= 315528`
- n60：`158188 + 1*132704 + 4 + 130600 = 421496 <= 429704`
- stack GameState：`130600 <= 130604`

这四项的 receiver domain 为 stack 或 frozen heap/embedded root，不是四字节 PE `.data` manager slot `[0x00C71678,0x00C7167C)`。但 A/B 尚未闭合，因此不能把全六项的 domain/slot 分离提升为 true。

## 产物与复现

- JSON：`out/cs-t0/native-join-manager-this-fill-fixed-member-roots-two-v12.json`
  - SHA-256 `0DCDF5505CD947F19A8B75758486748BCA230F3128B223B969856B17C66DB77B`
- Reproducer：`out/cs-t0/analyze-manager-this-fill-fixed-member-roots-two-v12.py`
  - SHA-256 `DEB4415B3983F7DEB2319966DA9675B01104E3AEE36561905025C398B4EEF596`

复现命令：

```powershell
python out/cs-t0/analyze-manager-this-fill-fixed-member-roots-two-v12.py --verify-existing
```

预期：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-fixed-member-roots-two-v12.json sha256=0DCDF5505CD947F19A8B75758486748BCA230F3128B223B969856B17C66DB77B status=UNKNOWN verifiedDirect=4 unknownDirect=2 writers=17 constructors=11 mutations=24
```

24 个语义 mutation 覆盖冻结输入、目标、根集合、调用全集、writer/constructor 全集、ABI、extent 算术、三个最小 provenance 前沿、最终 UNKNOWN 决定与权限位，均 fail-closed。

按照 `isaac-cs-refactor` 的职责分离，本交接是逆向侧产物；独立测试复核必须由另一执行者完成。
