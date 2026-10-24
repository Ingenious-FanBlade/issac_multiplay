> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 fixed-member remaining-one v11 交接

## 结论

本轮仅处理 frozen exact1 `0x005CF2B3`（VA `0x009CF2B3`，`sub_9CF050`，`memset(receiver + 292, 0, 0x44)`，exclusive end `360`）。最终状态为：

- `UNKNOWN_TWO_ENTITY_PLAYER_ROOT_UNIVERSES_NOT_CLOSED`
- 行级计数：`verified=0, unknown=1`
- `sub_9CF050` 六个直接 receiver 入口中，局部闭合 `4/6`；本轮要求的四个动态/嵌入入口闭合 `2/4`
- 不允许 manager-this promotion，不产出可合并的行证明

这不是对失败证明的弱化：reproducer 明确保留已闭合的两个 stack 来源与两个 `sub_9C79A0` 嵌入来源，同时对两个 Entity_Player 动态根保持 fail-closed。

## 冻结绑定

- `native-join-manager-this-fill-fixed-member-three-v10.json`: `FD7E374FB5A0863716042AC695B10928C06F6A173089DB58AA47123B8CF89433`
- `native-join-manager-this-bulk-memset-v3.json`: `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- `native-join-manager-this-bulk-memset-range-audit-v5.json`: `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- `isaac-ng.exe`: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

## 直接入口全集与闭合分区

IDA 只读 code xrefs 与磁盘 `.text` raw-E8 scan 均得到精确六项：

`0x009061BB, 0x0090C917, 0x009BCC22, 0x009BEFFD, 0x009C7DA2, 0x009C7DC5`

无 data xref、无 entry 绝对地址出现、entry 未导出。

已闭合：

- `0x009061BB`：EBP 栈 frame，receiver 起点后的保守可用 extent 为 `1504`，`360 <= 1504`。
- `0x0090C917`：EBP 栈 frame，receiver 起点后的保守可用 extent 为 `1500`，`360 <= 1500`。
- `0x009C7DA2`：`sub_9C79A0` 第一嵌入数组，`root+105852+i*1500, 0<=i<8`，exclusive end `117852 <= 130600`。
- `0x009C7DC5`：第二嵌入数组，`root+117852+i*1500, 0<=i<4`，exclusive end `123852 <= 130600`。

两个嵌入入口的局部 root partition 已覆盖 `n298` pointee、`n60` pointee与 `sub_9172F0` stack GameState：

- `n298`：allocation `315528`，嵌入 offset `4004`，`4004 + 130600 = 134604 <= 315528`。
- `n60`：allocation `429704`，两元素数组 offset `158188`、stride `132704`、receiver-in-element `4`，最大 end `421496 <= 429704`；选择索引保持在 `{0,1}`。
- `sub_9172F0` stack：保守可用 `130604`，`130600 <= 130604`。

上述 receiver 是 heap pointee 或 EBP-relative stack address；这里的 `n60 pointee` 不等于保存该指针的 PE `.data` slot `[0x00C71678,0x00C7167C)`，所以仅对已闭合来源成立 slot-disjoint。

## 最小未闭合根清单

1. `ENTITY_PLAYER_PLUS_0x1E6C_WRITER_AND_ALIAS_UNIVERSE`，影响 `0x009BCC22`。
   - 局部 null 分支确有 `0x2F00` (`12032`) allocation，并在 `+0x20EC` (`8428`) 处取 receiver；`8428 + 360 = 8788 <= 12032`。
   - 但该分支仅在 `Entity_Player+0x1E6C` 为 null 时执行；already-nonnull 字段的完整 writer/alias universe 未闭合。

2. `SUB_9BEE80_V1087_ENTITY_PLAYER_ALLOCATION_UNIVERSE`，影响 `0x009BEFFD`。
   - 同一候选 class extent 可满足 `8428 + 360 = 8788 <= 12032`。
   - 但 `sub_9BEE80` 两个直接 caller `0x005C4560`、`0x007ACFD6` 所传 `v1087` 的完整 allocation/type provenance 未闭合。

只要任一项未闭合，exact1 行就必须保持 UNKNOWN；相邻字段访问和局部候选 allocation 均不足以完成提升。

## Reproducer 与自测

产物：

- `out/cs-t0/native-join-manager-this-fill-fixed-member-remaining-one-v11.json`
- `out/cs-t0/analyze-manager-this-fill-fixed-member-remaining-one-v11.py`

reproducer 绑定三份输入 hash、target hash、exact1 membership、六项直接 caller 全集、16 个机器码 anchor、allocation extent、所有关键算术、slot domain、最终 UNKNOWN decision 与全 false permissions。20 个语义突变均被拒绝。

执行结果：

```text
PASS created=out/cs-t0/native-join-manager-this-fill-fixed-member-remaining-one-v11.json sha256=C1F958FB42102A8C02E0993AFD2E3136F020BE92EE1481D2CDCF72E5B815AEF8 status=UNKNOWN verifiedDirect=4 unknownDirect=2 mutations=20
PASS existing=out/cs-t0/native-join-manager-this-fill-fixed-member-remaining-one-v11.json sha256=C1F958FB42102A8C02E0993AFD2E3136F020BE92EE1481D2CDCF72E5B815AEF8 status=UNKNOWN verifiedDirect=4 unknownDirect=2 mutations=20
```

## 边界

本轮只使用 IDA 只读证据与磁盘 PE；未启动游戏、未附加/打开/读取进程、未修改 IDB、生产代码、测试或既有证据。按照 isaac-cs-refactor 的职责分离，本交接是逆向侧产物；独立测试侧复核仍应由另一执行者完成。
