> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this standard-vector group A residual exact3 v28 逆向交接

本分片只处理 bulk remaining frontier v26 中 `standardVectorA` 的三条残余
`memset`：`0x00308B5A`、`0x003271E2`、`0x003272FC`。它绑定并仅 supersede
group A v12 的这三条 `UNKNOWN`；不修改 v12、v26、production、tests、状态或索引。

结论为 **0 VERIFIED / 3 UNKNOWN**。三条局部 resize range、fresh allocation
extent 与 fresh 分支的 32 位无回绕均已由目标 PE 闭合，但 successor-first
writer/alias fixed point 均在下列首缺口停止；因此不能排除 incoming old backing
指向 manager slot 的反例，不能提升 manager-this 或 whole-image 结论。

## 绑定与目标

- 目标：`isaac-ng.exe`，SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- frontier v26 analyzer：
  `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`。
- frontier v26 JSON：
  `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`。
- group A v12 analyzer：
  `DD03993C883955EE92969C87C82D20844D46E8674A07E64253B8BD2B8A435C06`。
- group A v12 JSON：
  `4D4D3174AA7189394AA7B260583813487A50483856642A0228F2B4FCE9E3B44A`。
- 本分片 analyzer：
  `out/cs-t0/analyze-manager-this-fill-standard-vector-group-a-residual-exact-three-v28.py`，
  SHA-256 `647047858C22A25E61BEF144E988537738B42DC6548230B0DC307200A72DDD87`。
- machine JSON：
  `out/cs-t0/native-join-manager-this-fill-standard-vector-group-a-residual-exact-three-v28.json`，
  SHA-256 `438A03053113FD0102263E8FFF350A512F4978819BBE71FBC85C2F8C3128ACDB`。

精确 call bytes：

| RVA | bytes | containing function |
| --- | --- | --- |
| `0x00308B5A` | `E8 86 7A 3E 00` | `sub_708AE0` |
| `0x003271E2` | `E8 FE 93 3C 00` | `sub_725570` |
| `0x003272FC` | `E8 E4 92 3C 00` | `sub_725570` |

## 逐行结论

| RVA | receiver 与局部闭包 | 状态与首缺口 |
| --- | --- | --- |
| `0x00308B5A` | 三个 direct caller 完整为 `0x7078CF/0x9CA87A/0x9CE061`，分别选择 `GameRoot+0x6774C` 或两个 concrete object `+0x1E790` member。目标只在 `request>old && request<=capacity` 时写 `[oldEnd, oldEnd+12*(request-old))`；grow helper `sub_709480` 限制 `capacity<=0x15555555`，分配 `12*capacity<=0xFFFFFFFC` 并安装 begin/end/capacityEnd。 | `UNKNOWN`。首缺口为 `0xAFB77A..0xAFB786` 与 `0xB0F1CA..0xB0F1D6` 两个 stack-reloaded parent 的 unowned split-EH successor；其 owning predecessor、parent constructor/alias state 未形成固定点。 |
| `0x003271E2` | receiver 是 `(n298+0x2A404)+0x0C` 的 selected-record **pointer vector**，不是每个 `0xE8` record 内嵌 `+0x0C` tuple。`sub_724CB0` 在 `0x724CF7/FE/D05` 清零 triple；target 写 `4*(request-old)`，`sub_732240` 限制 `capacity<=0x3FFFFFFF` 且 `4*capacity<=0xFFFFFFFC`。 | `UNKNOWN`。首缺口为 record temporary split destructor `0xAFEFE7..0xAFEFF1` 与 root split destructor `0xB0BB38..0xB0BB45` 的 owner/predecessor，以及随后 already-offset `+0x0C` successor。`sub_722A80` 的 5 个 direct constructor caller、`sub_722C10` 的 9 个 E8 caller＋1 个 E9 tail 已枚举，但未闭 stack/liveout/indirect fixed point。 |
| `0x003272FC` | receiver 是 `n298+0x2A404` root tuple offset `+0`。`sub_724CB0` 在 `0x724CDC/E2/E9` 清零 triple；`sub_732240` 的 fresh extent/no-wrap 同上；containing helper direct callers 完整为 `0x8F3380/0x9532BD`。 | `UNKNOWN`。首缺口为 `0xB0BB38..0xB0BB45`：从 `[ebp-0x10]` 重载 parent、加 `0x2A404` 后 tail-jump `sub_725190`，但 PE-only replay 无法恢复该 split chunk 的 owning function/异常 predecessor；188 个 root pre-offset seeds 因而未完成 successor-first fixed point。 |

每行仍有可满足局部 guard 的 slot countermodel：令 `begin=end=0x00C71678`，
stride-12 行令 `capacityEnd=0x00C71684`，stride-4 行令
`capacityEnd=0x00C7167C`，`request=1`。首缺口未闭时，现有 PE 证据不能排除该
incoming tuple，所以 `slotOverlapExcluded=false` 是必须的 fail-closed 结论。

## 精确 census 与 caller 封口

- `+0x6774C` parent/pre-offset formation：5 个，ledger SHA-256
  `E9768BCFE7538730D7C4EE0E2508F00C2F88D2D2EF9E8AEBF85F5470C50CA90B`。
- `+0x1E790` formation：8 个，ledger SHA-256
  `C6D25401A40AB5EAC7540066C02C06BE750F5B81459B5067D04F2F9318EE60FC`。
- `+0x2A404` formation：188 个，ledger SHA-256
  `8D5213F7C8F9F343254EE4877DCF620C157161DD77C069BC79128665944C7850`。
- `sub_69DAC0` raw caller universe：82 个 E8、15 个 E9；E9 包含
  `0xAFB783`、`0xB0F1D3`。`sub_708B70` raw E8 caller 为 6 个。
- `sub_725570` raw E8 caller 精确为 `0x8F3380/0x9532BD`；`sub_724CB0`
  唯一 raw E8 caller 为 `0x950964`；`sub_725190` 为一个 E8
  `0x951670` 和一个 E9 `0xB0BB41`；`sub_732240` 为 4 个 E8。

formation census 只枚举 exact `LEA parent+disp32`、`ADD reg,imm32` 与
`ADD EAX,imm32` seed；它不把相同数值的任意 load/store 冒充 alias fixed point。

## 可复现与验证

Create-new：

```powershell
python out/cs-t0/analyze-manager-this-fill-standard-vector-group-a-residual-exact-three-v28.py
```

随后两次独立 PE existing replay 均为：

```text
PASS mode=PE ... sha256=438A03053113FD0102263E8FFF350A512F4978819BBE71FBC85C2F8C3128ACDB verified=0 unknown=3 mutations=28
```

命令：

```powershell
python out/cs-t0/analyze-manager-this-fill-standard-vector-group-a-residual-exact-three-v28.py --verify-existing
python out/cs-t0/analyze-manager-this-fill-standard-vector-group-a-residual-exact-three-v28.py --verify-existing
```

28/28 semantic mutation sentinels 均 fail closed，覆盖 schema/identity、四份冻结
binding、目标 hash、exact3 覆盖、summary/decision/permission、逐行 status/RVA、
firstGap、slot countermodel 与 fixed-point 状态。

外部分析器本分片状态固定为 `TIMEOUT_NO_PASS`，retry count 0；没有把不可用
解释为 PASS，也没有声称完成 IDA replay。没有启动游戏、进程或调试器，没有读
运行时内存，没有修改 IDB。所有 permissions、promotions 与 `overallAccepted`
均为 `false`；最终验收必须由独立角色完成。
