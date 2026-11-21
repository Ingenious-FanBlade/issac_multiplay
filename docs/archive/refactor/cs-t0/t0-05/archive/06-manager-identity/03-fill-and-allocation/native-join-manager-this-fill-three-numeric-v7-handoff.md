> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 三处填充数值闭包 v7 交接

日期：2026-09-02  
状态：`EXACT_THREE_FILL_ROWS_VERIFIED`  
范围：只裁决 `0x0066AFC0`、`0x00686BD1`、`0x006E6F3B` 三个 `memset` 的 source / offset / count / extent / callers / inequality

## 结论

冻结 v5 独立审计留下的三个点状缺口均已在精确目标、`no external module or injection` 范围内闭合：

| RVA | source / offset | count / extent | 完整 caller 与不等式 | 状态 |
| --- | --- | --- | --- | --- |
| `0x0066AFC0` | `sub_A6A9B0` 分配并清零的 808-byte GLFW window object；`offset=156*4=624` | `count=0x44=68`；`extent=808` | `sub_A6AF90` 精确 5 个 code xref，分别传 `this[47]`、旧 `v6`、`i_3` list node、fresh `v9`、`dword_C7999C`；全部回到相同 window family。`624+68=692<=808`。 | `VERIFIED_NUMERIC_AND_CALLER_CLOSED` |
| `0x00686BD1` | `sub_A89C90` 分配 `0x2AF4`，四字节 accounting header 后 payload extent 为 `0x2AF0=10992`；目的从 `10532+i` 开始 | 19-symbol Huffman builder 令 repeat symbol 仅为 `{16,17,18}`；extra bits `{2,3,7}`、base `{3,3,11}`，所以 count 上界为 `138` | `sub_A85710` 精确 2 个 code xref，均从 wrapper `+12` 取得上述 payload。header bits `{5,5,4}` 与 bases `{257,1,4}` 给前两 count 上界 `288/32`，循环执行时 `i<320`，故 `i<=319`；`10532+319+138=10989<=10992`。 | `VERIFIED_NUMERIC_AND_CALLER_CLOSED` |
| `0x006E6F3B` | `buf=DWORD[page+16]`，由 `sub_AEDA50` 按同一 actual block stride 构造 freelist；`offset=0` | flag case `count=4`，ordinary case `count=DWORD[page+24]=actualStride`；extent 为 actual stride | `sub_AE6EE0` 只有 `sub_AED000` 内两处 direct code xref；两处都传 `sub_AED6A0` 返回的同一 page metadata，第二处第四参数为零而不执行目标 fill。额外 data xref 是 `0x00BF8000` slot 0；精确镜像内没有该表字段的 executable consumer。故 `0+(4 or stride)<=stride`。 | `VERIFIED_NUMERIC_AND_CALLER_CLOSED` |

三条结论替换 v5 对这三行的点状 `insufficient`，但本交接本身不裁决其余 64 个 executable memset 行，也不裁决 copy/move、whole-image alias、ABA 或完整 manager-this 身份。

## 冻结输入

| 角色 | 路径 | SHA-256 |
| --- | --- | --- |
| 独立 range audit | `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| exact-78 UNKNOWN 源 | `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| 目标 `isaac-ng.exe` | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

复现器拒绝 predecessor hash、target hash/machine/image、三处 call bytes、常量表、dispatch slots、raw table-address diagnostics、PE exports 或既有权限的漂移。

## `0x0066AFC0`：更正对象来源而不是沿用偶然成立的不等式

v5 已直接否定 v4 的“2664-byte TLS/static object”叙述。本轮没有用 `692<=2664` 掩盖错误来源，而是重新绑定：

1. `0x00A6A9B0` 调用 allocator，size 为十进制 `808`；
2. fresh result 随即以 `0x328` 清零并发布到 `i_3`；
3. `XrefsTo(0x00A6AF90)` 精确为五处：`0x00A19169`、`0x00A19F4F`、`0x00A69C18`、`0x00A6AC26`、`0x00A71A7A`；
4. 目标 fill 是 receiver `+156 DWORDs`、固定 `0x44` bytes，故 end 为 `692`。

因此该行的正确证书是 `808-byte window object`，不是 v4 的旧对象身份。

## `0x00686BD1`：表域、索引和最终 byte inequality

动态 header 三个字段由：

```text
bits  = {5, 5, 4}
bases = {257, 1, 4}
```

生成；前两项因此不超过 `288` 和 `32`。构造 code-length Huffman table 时，叶值严格由 `v211=0..v120-1` 写入，且此分支的 `v120=19`。解码后的 `n16>=16` 因而只能为 `16/17/18`。

repeat 的实际 effective table 位于：

```text
extra bits @ 0x00BA5970 = {2, 3, 7}
base       @ 0x00BA5974 = {3, 3, 11}
```

三个最大 count 分别为 `6/10/138`。fill 执行前的循环 guard 保证 `i < firstCount+secondCount <= 320`，所以：

```text
10532 + i + count
<= 10532 + 319 + 138
= 10989
<= 10992
```

这补上了 v5 指出的 table domain、bit width、table maximum 和 row-local arithmetic 四个缺口；没有继续使用 v4 未证明的 `Size<=460` 叙述。

## `0x006E6F3B`：slot 0 消费者与 block metadata capacity

`XrefsTo(0x00AE6EE0)` 精确得到：

- code `0x00AED103`；
- code `0x00AED15A`；
- data `0x00BF8000`，其值为 `0x00AE6EE0`。

两处 code xref 均在 `sub_AED000`，并把 `sub_AED6A0` 返回的 `v6` 作为 metadata。第二处显式传第四参数零，因此不执行 `0x00AE6F3B`。metadata 的 capacity 关系由下列链闭合：

1. `sub_AEB3F0` 给出 page base 与 page usable bytes；
2. `sub_AED6A0` 把 actual stride 写入 `page+24`（大块 sentinel 路径由 flag 区分），并把 `floor(usable/stride)` 写入 `page+12`；
3. `sub_AED9B0 -> sub_AEDA50` 只以 `pageBase + stride*index` 发布 freelist node；
4. `sub_AE6EE0` 只取 `page+16` 的 head。flag case 清四字节 link；ordinary case 清精确 stride。

表槽消费者使用独立的 whole-text operand scan封闭：对 IDA `.text [0x00401000,0x00B18000)` 的 `1,924,021` 条已定义指令逐 operand 检查 `0x00BF8000..0x00BF8024`，四段计数为 `477547/537901/588383/320190`，semantic match 均为零。目标 PE 也不导出 table 或 `sub_AE6EE0`；raw exact table VA 的四个 `.text` byte matches 均不是 operand，table RVA 在 `.text` 的 raw match 为零。因此，在机器 JSON 明示的 exact-image/no-external 范围内，slot 0 的 executable consumer 集为空；外部模块或注入没有被偷换为已证明范围。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-fill-three-numeric-v7.json` | `242C05FD1AD0FF9D4851EC7A70EF59095B15F592C72A73DF4F4766BD17508807` |
| `out/cs-t0/analyze-manager-this-fill-three-numeric-v7.py` | `F75F769680120467E93CAE18CC3C0BD2DF183655B37A36D9223875AE97A08AEC` |

```powershell
python out\cs-t0\analyze-manager-this-fill-three-numeric-v7.py --verify-existing
```

连续两次结果均为：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-three-numeric-v7.json sha256=242C05FD1AD0FF9D4851EC7A70EF59095B15F592C72A73DF4F4766BD17508807 verified=3 unknown=0
```

## 操作边界

本轮只读取冻结 JSON、目标磁盘 PE 与只读 IDA 数据库。没有启动游戏，没有打开或读取目标进程，没有附加调试器，没有修改 IDB；没有修改 production、tests、v1-v7 既有 formal 产物、index、status 或 research plan。机器 JSON 的全部权限值为 false，且明确禁止由本三行证据直接提升 whole-image 或 manager-this。
