> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this checksum 尾填充 exact3 v9 交接

日期：2026-09-02  
状态：`EXACT_THREE_CHECKSUM_FILL_ROWS_VERIFIED`  
范围：只裁决 `0x00019701`、`0x005CACD4`、`0x005D8384` 三个 `memset` 行

## 结论

三行在精确目标、`no external module or injection` 的离线静态范围内均已闭合 source / extent / all callers / byte inequality：

| RVA | state 来源 | caller 全集 | 不等式 | 结论 |
| --- | --- | --- | --- | --- |
| `0x00019701` | `DWORD[n60+0x18300]` 指向 `sub_A0F4C0(0x7898)` 的 fresh object；receiver=`object+0x1218`，state=`receiver+0xE8` | `sub_4186C0` 精确 9 个 direct call；8 个直接形成 `DWORD[n60+0x18300]+0x1218`，`0x00803E70` 经 `sub_802980` receiver family 形成 `receiver+0x1218` | state：`used+(4-used)=4<=16`；outer：`0x1218+0xE8+0x10=0x1310<=0x7898` | `VERIFIED_NUMERIC_EXTENT_AND_CALLER_CLOSED` |
| `0x005CACD4` | callee 内 state=`arg0+12`；三个 caller 均构造精确 28-byte stack aggregate，state 占 `[base+12,base+28)` | 精确 3 个 direct call：`0x00917577`、`0x00959076`、`0x009CAE15` | `used+(4-used)=4<=16`，`12+16=28<=28` | `VERIFIED_NUMERIC_EXTENT_AND_CALLER_CLOSED` |
| `0x005D8384` | callee 内 state=`a2+12`；同一 caller 的两条路径均构造精确 28-byte stack aggregate | 精确 2 个 direct call：`0x00959353`、`0x009594B4` | `used+(4-used)=4<=16`，`12+16=28<=28` | `VERIFIED_NUMERIC_EXTENT_AND_CALLER_CLOSED` |

三处原始指令都先要求 `used != 0`，再以无符号比较要求 `used < 4`，随后计算：

```text
destination = state + used
count       = 4 - used
end         = used + (4 - used) = 4
```

共同的 `sub_683410` 原始指令还把该 state 明确定义为 16-byte 布局：首 DWORD 是尾字节暂存，`+4` 是 used byte，`+8` 是 rolling value，`+12` 是 mode。目标 `memset` 只补齐首 DWORD 尾部，不越过 state 的第 4 字节。

## `0x00019701` 的 member producer 与 forwarder 闭包

只读 IDA rendered-text 查询 `18300h],` 得到该 manager field 的完整写者集合：

```text
null only: 0x006F10DB 0x006F3193 0x006F375E
           0x007383F5 0x00738518
non-null:  0x0073854A
```

唯一非零发布路径在 `sub_7384D0` 内：`0x00738527` 以 `0x7898` 调用 allocator，`0x0073853F` 调用唯一 constructor `sub_7E9400`，`0x0073854A` 发布返回值。constructor 自身直接构造 `object+0x1218`，并访问远高于 `0x1310` 的成员，因此目标 state 的完整 16 bytes 均在该 fresh allocation 内。

`sub_4186C0` 的 9 个 code xref 由 IDA 和磁盘 PE 的 `CALL rel32` 全 `.text` 重扫得到完全相同集合：

```text
0x00803E70 0x00831941 0x00832376 0x008323E8 0x008324A2
0x0090D6A6 0x0092F231 0x0092FAE7 0x0092FEE9
```

三个目标函数 VA/RVA 的四字节字面量在整份 PE 中均为零，故没有 address-taken 入口替代上述 direct-call 集。`0x00803E70` 所在 receiver method `sub_802980` 另作一层闭包：磁盘重扫得到 10 个 direct call，唯一地址注册位于 IDA `0x0086C2B7`；其 receiver family 与 `sub_738610` wrapper 均回到 `n60+0x18300` 发布的同一 `0x7898` object。该层没有被误写成“9 个 call 都在指令现场直接读取 n60 field”。

## 两类 28-byte 栈对象

`sub_9C9340` 的三个 caller 分别从 `EBP-0x1FEB8`、`EBP-0x20`、`ESP+0x0C` 建立连续 28-byte aggregate；`sub_959180` 的两条 `sub_9D8190` 路径都从 `ESP+0x14` 建立相同布局。五处 caller 都逐项初始化 offset `0/4/8/12/16/20/24`，并把 aggregate 首地址传入 callee。callee 再取 `+12` 作为 checksum state，因此 16-byte state 精确结束于 aggregate 的第 28 字节。

对应原始 construction byte anchors 已进入复现器；只靠 Hex-Rays 栈变量名称不能通过验证。

## 冻结输入与目标

| 角色 | 路径 | SHA-256 |
| --- | --- | --- |
| memset v3 UNKNOWN 源 | `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| 独立 range audit v5 | `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| `isaac-ng.exe` | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

本轮没有信任或复用已被审计否决的 memset-v4 全称布尔结论。v5 对 exact3 的 `INSUFFICIENT_ROW_LOCAL_RANGE_EVIDENCE` 保持历史原文；本 v9 只提供可并入的新逐行证书。

## 复现与 fail-closed 自测

```powershell
python out\cs-t0\analyze-manager-this-fill-checksum-three-v9.py --verify-existing
```

连续两次结果均为：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-checksum-three-v9.json sha256=6697BA289DE404F95F6D876F2FE1B03F537DBEB0B88574E7ED72B6551F7BAF2B verified=3 unknown=0 mutations=12/12
```

复现器绑定 exact-three 集合等式、两份冻结输入哈希、目标 PE hash/machine/image/text、三个 guard/write 指令窗口、`sub_683410` state 布局、`0x7898` producer/publisher、manager field writer anchors、五个栈 constructor，以及四个函数的 `CALL rel32` 全集。12 个语义 mutation（删/加 caller、缩小栈或 outer extent、改变 RVA/call bytes/state offset/不等式/权限/计数等）全部 fail closed。

## 产物

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-fill-checksum-three-v9.json` | `6697BA289DE404F95F6D876F2FE1B03F537DBEB0B88574E7ED72B6551F7BAF2B` |
| `out/cs-t0/analyze-manager-this-fill-checksum-three-v9.py` | `51CCA213B31F20153BAF45B85EFBDB680E1E1C9764385C23A8574FF740DC2709` |

## 边界

本轮只读取冻结 JSON、目标磁盘 PE 和只读 IDA 数据库；未启动游戏，未打开/读取目标进程，未附加调试器，未修改 IDB。未修改 production、tests、旧证据或正式 v7，也未改 index/status/research plan。

三行可作为后续精确 merger 的 row-level 输入，但本交接不裁决其他 memset、copy/move、whole-image alias、ABA 或完整 manager-this 身份；所有权限仍为 false，且本三行不得单独提升 whole-image 或 manager-this。
