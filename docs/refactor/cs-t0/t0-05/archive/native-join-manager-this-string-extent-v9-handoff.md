> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this string-extent v9 静态交接

## 结论

固定目标 `isaac-ng.exe`（SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`）上，`SSO_STRING_INLINE_OR_ALLOCATED_CAPACITY` 的 exact 23 行现分为：

- `VERIFIED_NUMERIC`: 9 行；
- `UNKNOWN`: 14 行；
- 消息中优先复核的 5 行全部为 `VERIFIED_NUMERIC`。

本交接只回答每个 exact helper call 的字符串 inline extent、heap extent、destination offset、copy count、NUL 字节、全部 owner 调用点的实际对象/包含 record extent，以及 `offset + totalCount <= selectedExtent`。未全称全部调用点对象 extent 的行保持 `UNKNOWN`。

## 冻结输入

| 角色 | 路径 | SHA-256 | 使用范围 |
| --- | --- | --- | --- |
| frozen local-stack v3 | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v3.json` | `73905C2A1D27C12344F82147C4F89781BAC08A447D55D4C6E89D8465BB23626D` | exact call identity、helper、表达式、call bytes |
| negative audit | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v4-independent-audit-v1.json` | `3937A58884C84D8A4653612F6C64980C314C191F70D9D5498653BAD06D22CC7C` | 确认 v4 为 `FAIL`，并交叉核对 exact 23 集合 |
| rejected v4 | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v4.json` | `FFDAD182B2948579A2D5FE6F1BB6BEE7502400B1BD27ADE87CF9276F9FE96265` | **仅**读取 `proofClass == SSO_STRING_INLINE_OR_ALLOCATED_CAPACITY` 的 23 个 callsite 标签；未读取旧的 capacity/closure/finalDecision 结论 |

## 数值口径

- 窄字符串对象为 24B，inline 字符区为 16B；heap 字符区 extent 为 `capacity + 1`。
- 宽字符串对象为 24B，inline 字符区仍为 16B（8 个 2B code unit）；heap 字符区 extent 为 `2 * (capacity + 1)`。
- `copyCountBytes` 是 exact `memcpy`/`memmove` 参数。NUL 若已含在 helper count 中标为 `includedInCopyCount=true`；紧随 helper 的终止写另计入 `totalCountBytes`。
- 最终检查使用 `destinationOffsetBytes + totalCountBytes <= selectedExtentBytes`。
- 数值不等式成立但全部 owner 调用点对象 extent 未全称时，最终状态仍为 `UNKNOWN`。

## 优先 5 行

### `0x0032DE82`, `0x0032DF7B`, `0x0032E07A`

- owner `0x0032CFF0` 在本地创建 `0x70`（112B）record；三个 24B 字符串对象分别位于 record offset `8/32/56`。
- 三个字符串对象在 exact call 前均以 length `0`、capacity `15` 初始化；目标 `memmove` 只在 `sourceLength <= 15` 的 no-grow 分支执行。
- exact selected extent 为 inline 16B，destination offset 为 0，copy count 为 `sourceLength`，随后写 1B NUL；因此 `0 + sourceLength + 1 <= 16`。
- owner 的 2 个入口 callsite RVA 为 `0x004F36D2`、`0x005534A6`；record/object extent 由 owner 内部固定分配，不依赖入口参数。

### `0x0032E340`

- 同一 owner 在本地创建 `0x54`（84B）record；目标 24B 字符串对象位于 record offset `8`。
- 同样由 capacity `15` 的初始化与 no-grow 分支得到 inline 16B、offset 0、copy `sourceLength`、随后 1B NUL，`0 + sourceLength + 1 <= 16`。
- 两个 owner 入口均复用同一内部 `0x54` 分配不变量。

### `0x0045C14A`

- owner `0x0045C080` 的 49/49 caller 都具有相同 raw setup：`sub esp, 0x28; mov ecx, esp`，即每个 caller 为构造对象提供 40B backing。
- 40B 对象中的字符串子对象位于 offset 16，extent 为 24B；构造路径先写 capacity `15`，exact helper 因此选择 16B inline 字符区。
- destination offset 为 0，copy count 为 13，随后写 1B NUL；`0 + 14 <= 16`。
- 49 个 caller RVA 与 caller-set SHA-256 已逐项保存在 v9 JSON；caller-set SHA-256 为 `7150AAE1F272D0F15CCAD575933D52DA9565C9872F10E67965F6EF659CEA4529`。

## 其余 `VERIFIED_NUMERIC` 4 行

| callsite RVA | 对象/全部调用点 extent | offset | copy count / NUL | extent 与不等式 |
| --- | --- | --- | --- | --- |
| `0x00284FA5` | owner 的 2/2 call 均指向 24B-stride vector element；对象 extent 24B | `position` | `newLength-position+1`，含 1B NUL | inline 16 或 heap `capacity+1`; endpoint=`newLength+1` |
| `0x0042855A` | owner 10/10 call 的 string object 均为 24B；caller backing 为九个 24B 与一个 28B，目标子对象均为前 24B | `2*prefixLength` | `2*sourceLength+2`，含 2B NUL | heap `2*(movedSourceCapacity+1)`；endpoint=`2*(prefixLength+sourceLength+1)` |
| `0x004285C5` | 与上一行同一 10/10 caller 集 | `0` | `2*prefixLength`，本 helper 不含 NUL | heap `2*(movedSourceCapacity+1)` |
| `0x005F14DE` | owner 的唯一入口内，对 24B-stride local vector element 执行 exact helper | `0` | `newLength+1`，含 1B NUL | inline 16 或 heap `capacity+1`; endpoint=`newLength+1` |

caller-set SHA-256 分别为：

- owner `0x00284F70`: `84511C4FA4B17C87EA30FB22D90CA7AF372646C544A14A125730E42DC45BD7BC`（2 个）；
- owner `0x004284D0`: `1EDA86499D717EA891089D77894977394BCAA8060C7A0A796CA58AF38920DF41`（10 个）；
- owner `0x005F1380`: `032FD15E15045C60F74E29EE70CF8FA143974B55CEB68E72F922F7FC3B9354B6`（1 个）。

## `UNKNOWN` 14 行

下列行的局部字符串布局与条件不等式可表达，但至少一个 owner 调用点没有全称实际字符串对象/包含 record extent，因此不升级：

| owner RVA | exact 行 | owner caller callsite 数 | caller-set SHA-256 |
| --- | --- | ---: | --- |
| `0x00031070` | `0x000310F4`, `0x00031154` | 21 | `FA445C6BDBF77467DB8D2A907B2BEC48B71CC5D957D6E8ECF1C11563A6F62104` |
| `0x00099580` | `0x000995C0`, `0x000995F6`, `0x00099608`, `0x00099684`, `0x00099690`, `0x000996A7` | 14 | `3600888BC8C90A45237B41E4B8B8268FA8F2C9B84BC3C93C30E790557F4E5A0E` |
| `0x000997F0` | `0x00099828` | 166 | `22518D3B3FC12C5CFC22A5BBFF7B5F2C8CF8860EDF73DBCEF2928CC84F5C231E` |
| `0x00251B10` | `0x00251B8E`, `0x00251B9D`, `0x00251BB5` | 22 | `5C0D196D1CF67C69B5E03D7FDC43468584541BB571064D8F12C9ACF2EF2E07DA` |
| `0x00284E70` | `0x00284EB5` | 1 | `5E72EC18A13F7242FE752108FE3AC56E37DBC0ED22AC7BA58ADDEB69214E827D` |
| `0x00284EE0` | `0x00284F19` | 6 | `C7184A5A06FA395848490ADC49BA2768DA84A825418F777602485B6F1648CDB5` |

每一行的 inline/heap extent、offset、copy count、NUL 与条件式均在 JSON 中逐行保存；`allCallsiteObjectExtents.status=UNKNOWN`、`inequality.result=null`，没有用局部公式替代全部 caller 对象 extent。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/reproduce-manager-this-string-extent-v9.py` | `601349A3B79E90C1E2CDF09D23F7BFA26B44A213CB9301239EA0A3D7A14EB583` |
| `out/cs-t0/native-join-manager-this-string-extent-v9.json` | `DA2986BD521AA6967B62DDF99B27725C9AF9C9277BB902C45D2BEF44B4F228D5` |

创建命令（只允许对不存在的输出执行）：

```powershell
python -B .\out\cs-t0\reproduce-manager-this-string-extent-v9.py
```

现有产物核验命令：

```powershell
python -B .\out\cs-t0\reproduce-manager-this-string-extent-v9.py --verify-existing
```

本轮 create-new 输出与随后 replay 均为 `PASS`。复现器对每行 14 个语义字段与 5 个根字段执行 327 次 mutation；extent、offset、copy count、NUL、caller count/hash、inequality 或状态任一改变均被拒绝。该 mutation gate 是复现器自身的 fail-closed 检查，不替代另一测试智能体的独立验收。

## 操作边界

- 仅使用精确输入 IDB 的只读 Hex-Rays/指令与 caller xref 查询；`idbModified=false`。
- 未启动游戏，未打开进程，未附加调试器。
- 未修改 v3、negative audit、rejected v4 或其他历史产物；只新增本交接、JSON 与复现器。
