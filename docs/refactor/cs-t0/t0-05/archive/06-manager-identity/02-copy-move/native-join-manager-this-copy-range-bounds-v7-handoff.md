> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy-range-bounds v7 交接

日期：2026-09-02  
状态：`PARTIAL_RANGE_BOUND_PROGRESS_REMAINS_UNKNOWN`  
范围：冻结 caller-heap v5 的 exact 22 条 `UNKNOWN`；只裁决 destination 的 `offset + count <= extent`

## 结论

冻结输入
`out/cs-t0/native-join-manager-this-copy-move-caller-heap-v5-candidate.json`
的 SHA-256 精确为
`5D46EB95A7575EDC6384B4D15D6031C7F78945041FF98216AB1F8AB63DCEF6EA`。
其 `v5Status=UNKNOWN` 集合严格为 22 条，顺序、RVA、helper 与 call bytes 均由复现器重新核对。

- `VERIFIED_BOUNDED`：**6**；
- `UNKNOWN`：**16**；
- raw `0x0033570B`：按空 filtered range 反例继续 `UNKNOWN`；
- copy/move frontier、whole-image alias、ABA 与 manager-this promotion：**均不允许**。

这里的 `VERIFIED_BOUNDED` 是严格的 **range-only** 结论：证明精确 helper site
及其完整 caller 集合共享数值关系 `offset+count<=extent`。它不声称 backing
pointer 的完整 writer/points-to 集已经闭合，也不单独声称与 GameRoot slot
无重叠。聚合方只能合并六条 range 子证明，仍须单独满足 pointer/writer
closure；不得把本交接直接解释成 whole-image 或 manager-this 通过。

## 逐行裁决

| RVA | destination / offset | count / extent | 结论与数值依据 |
| --- | --- | --- | --- |
| `0x00008A62` | fresh heap DWORD array；`offset=0` | `count=4*oldCount`；`extent=4*newCount` | `UNKNOWN`：需要 `oldCount<=newCount`；两名完整 caller 均未证明，`new=1, old=2` 时为 `8>4`。 |
| `0x00008B2A` | 第二个 fresh heap DWORD array；`offset=0` | `count=4*oldCount`；`extent=4*newCount` | `UNKNOWN`：同上，第二个 count 字段也没有共同不等式。 |
| `0x0001513A` | caller 的 5000×84 heap array；`offset=0` | `count=84*v4`；`extent=420000 (0x668A0)` | `UNKNOWN`：要求 `v4<=5000`；limit 分支只记录日志并继续。 |
| `0x000155A4` | caller 的 5000×108 heap array；`offset=0` | `count=108*v4`；`extent=540000 (0x83D60)` | `UNKNOWN`：要求 `v4<=5000`；limit 分支只记录日志并继续。 |
| `0x0001AAB0` | `a3+4` DWORD array；`offset=0` | `count=4*sourceCount`；`extent=4*DWORD[a3+8]` | `UNKNOWN`：capacity 失败只记录日志；177 个 producer 未共同证明 `sourceCount<=capacity`。 |
| `0x0001AEB8` | `a3+4` DWORD array；`offset=0` | `count=4*sourceCount`；`extent=4*DWORD[a3+8]` | `UNKNOWN`：capacity 失败只记录日志；五名 caller 未共同证明不等式。 |
| `0x001CBEB5` | receiver 的 4-byte vector erase span；`offset=kk-begin` | `count=end-(kk+4)`；`extent=end-begin` | `VERIFIED_BOUNDED`：4/4 caller 均给同 receiver 的非 end iterator；和为 `extent-4<=extent`。 |
| `0x0033570B` | filtered vector 的 selected element | `count=end-(selected+4)`；`extent=end-begin` | `UNKNOWN`：filtered range 可为空；零 count 分支跳过除法并令 `selected=begin=end`，raw count 变为 `0xFFFFFFFC`。 |
| `0x003534FE` | receiver 4-byte vector insert；`offset=a2-begin` | `count=4*n`；`extent=cap-begin` | `VERIFIED_BOUNDED`：4/4 caller 的 a2 为 receiver begin/end iterator，且本分支有 `4*n<=cap-end`；所以和不超过 extent。 |
| `0x0035352E` | 同一 vector insert 的另一 tail 分支；`offset=a2-begin` | `count=4*n`；`extent=cap-begin` | `VERIFIED_BOUNDED`：使用相同 4/4 caller 与 spare-capacity 不等式。 |
| `0x005B91EF` | Player-pointer vector；`offset=0` | `count=usedBytes-4`；`extent=usedBytes` | `VERIFIED_BOUNDED`：每次动态执行均受非空 4-byte-element loop guard 支配，故 `usedBytes>=4`。 |
| `0x0061F2C2` | secondary double buffer `this[49]`；`offset=0` | `count=4*((this[5]-this[4])>>4)`；extent 未建立 | `UNKNOWN`：length 来自另一 range，没有与 `this[49]` allocation size 的共同关系。 |
| `0x00657E0A` | caller output slice；`offset=0` | first chunk `<=R=a3*a4`；actual extent 未对全部 dispatch 建立 | `UNKNOWN`：direct families 有界，但 `0xB9ECE0/0xB9ED3C` slot-6 间接 caller universe 仍 type-open。 |
| `0x00657E94` | 同一 output slice；`offset=firstChunk` | `secondChunk<=R-firstChunk` | `UNKNOWN`：局部两段和 `<=R`，但全部 indirect caller 的 `actualExtent(a2)>=R` 未闭合。 |
| `0x00658338` | caller output slice；`offset=0` | first chunk `<=R=a3*a4` | `UNKNOWN`：30 名 direct caller 有界，但 `0xB9ED10` 仍允许 untyped indirect dispatch。 |
| `0x006583C2` | 同一 output slice；`offset=firstChunk` | `secondChunk<=R-firstChunk` | `UNKNOWN`：局部两段和 `<=R`，但 indirect output allocation 未闭合。 |
| `0x00658487` | BufferedStream byte vector；`offset=used` | `count=Size`；`extent=end-begin` after resize | `UNKNOWN`：threshold path 经未闭合 `vfunc(+32)` 后把 resize target 改为 `Size+4096`；未证明所有动态类型都令 `usedAfterVfunc<=4096`。 |
| `0x00661577` | fresh heap 或第二个固定 `0x10000` ring；`offset=0` | `count=stream Size` | `UNKNOWN`：fresh path 有界，但 static-ring family 未证明 `Size<=0x10000`；power-of-two 检查只记录日志。 |
| `0x00688B74` | miniz caller output buffer；`offset=current` | `count=min(produced,end-current)`；`extent=end` | `VERIFIED_BOUNDED`：staging flush 分支在 copy 前把 produced clamp 到 remaining，故 `current+count<=end`。 |
| `0x00689F63` | reentrant caller output buffer；`offset=0` | `count=Sizea`；actual extent 未建立 | `UNKNOWN`：该分支不使用另一个 `Size` 参数，两个 direct caller 的 pointer/capacity producer 链均未闭合。 |
| `0x0068E3DB` | realloc-backed read buffer；`offset=0` | `count=oldUsed-consumed`；`extent=capacity` | `UNKNOWN`：copy 前只检查新 used 为正；parser/writer 集未证明 `0<=consumed<=oldUsed<=capacity`。 |
| `0x006957BC` | zlib-style `next_out/avail_out` slice；`offset=0` | `count=min(avail_out,pendingInput)`；`extent=avail_out` | `VERIFIED_BOUNDED`：3/3 direct call 均使用同一 stream slice，count 恒不超过当前 avail_out。 |

六条正向 RVA 为：

```text
0x001CBEB5
0x003534FE
0x0035352E
0x005B91EF
0x00688B74
0x006957BC
```

## 两个本轮新增的精确保留条件

### `0x00658487`

`0xA58457` 在 `used+Size>oldExtent` 时进入扩展。普通分支把目标设为
`oldExtent+Size+4096`；但 `0xA58464` threshold 分支先调用动态
`vfunc(+32)`，随后在 `0xA5846D` 把目标重置为 `Size+4096`。在所有动态
类型的该 vfunc 都证明 `usedAfterVfunc<=4096` 前，不能推出
`usedAfterVfunc+Size<=Size+4096`。`sub_7DCCD0` 的 resize 语义本身不能补上
这个调用前提。

### `0x0068E3DB`

本地 malloc/realloc/read 增长路径会在写入前保证 capacity，但 shift 路径从
`this+188` 读取 consumed，直接执行 `used-=consumed`，只用 signed
`used>0` 决定是否调用 memmove。若未闭合 writer 允许 consumed 为负，新的
used 可大于 capacity；若 consumed 大于旧 used，则没有有效 suffix 范围。
因此共同前提必须是
`0<=consumed<=oldUsed<=capacity`，当前证据没有全称证明。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json` | `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D` |
| `out/cs-t0/build-native-join-manager-this-copy-range-bounds-v7.py` | `95A73B70C24E70E1732B0D65399903C1EC9CFC8F1F3345FEE8349383967B0A9F` |

机器 JSON 逐行保存 destination object、base、offset、count、extent、完整 caller
inventory、静态地址、数值不等式和未决谓词。复现器拒绝 source/hash、exact22
顺序、helper/call-bytes 或 6/16 partition 的漂移。

```powershell
python out\cs-t0\build-native-join-manager-this-copy-range-bounds-v7.py --verify-existing
```

预期：

```text
PASS existing=out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json sha256=C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D verified=6 unknown=16
```

## 操作边界

本轮只使用现有证据与 IDA MCP 的只读反编译、反汇编和 xref 查询。目标
IDB 仍是 x86 `isaac-ng.exe`，image base `0x00400000`、image size
`0x008FE000`、输入 SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
没有启动游戏、没有打开或读取进程、没有附加调试器、没有修改 IDB；没有修改
production、tests、既有 formal v1-v7、index 或任何既有证据文件。
