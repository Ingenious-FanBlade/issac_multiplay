> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this fixed-member exact3 v10 交接

日期：2026-09-02  
状态：`EXACT_THREE_PARTITION_2_VERIFIED_1_UNKNOWN`  
范围：`V4_FIXED_MEMBER_OBJECT_UNIVERSE` 的三个精确 `memset` 行；只裁决 row-local 范围、完整直接入口集合与 manager slot 不相交

## 结论

| RVA | 函数 / 写区间 | 状态 | 结论 |
| --- | --- | --- | --- |
| `0x0034FC5D` | `sub_74FBE0`, `[a2+696,a2+865)` | `VERIFIED_OBJECT_EXTENT_CALLER_CLOSED_AND_SLOT_DISJOINT` | 唯一直接入口 `0x0075096C` 把 `sub_7508C0` 的 `EBP-0x3A8` stack aggregate 作为 `a2` 转发；`865 <= 920`。 |
| `0x005ADB94` | `sub_9ADAF0`, `[this+696,this+865)` | `VERIFIED_OBJECT_EXTENT_CALLER_CLOSED_AND_SLOT_DISJOINT` | 10 个直接入口全部闭合为 9 个 EBP-relative stack allocation source；各来源最小可用 extent 为 920，故全部满足 `865 <= extent`。 |
| `0x005CF2B3` | `sub_9CF050`, `[this+292,this+360)` | `UNKNOWN_RECEIVER_OBJECT_UNIVERSE_NOT_CLOSED` | 6 个直接入口中两个 stack source 局部满足范围；其余四个动态/嵌入 receiver 的根 allocation、containing extent 与 alias-forwarder universe 未全部闭合，整行严格保留 `UNKNOWN`。 |

这份 artifact 只允许合并前两行的点状证明，不允许提升 whole-image alias 或 manager-this 身份。旧 v4 的共享 narrative 和布尔 closure 没有作为证明前提。

## 冻结输入

| 角色 | 路径 | SHA-256 |
| --- | --- | --- |
| exact-78 UNKNOWN 源 | `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| 独立 range audit | `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| 目标 `isaac-ng.exe` | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

复现器还固定 exact3 membership、三行 call bytes、目标 PE machine/image/sections、31 个目标/入口/序言机器码 anchor、函数序言里的 stack reservation immediate、direct-call target、全 `.text` raw-E8 集、绝对入口地址出现集、PE exports、allocation 分区、算术、UNKNOWN 分区、权限和决议。

## 完整直接入口集合

只读 IDA 的非截断 code/data xref 结果与磁盘 `.text` raw-E8 relative-target 扫描一致：

- `sub_74FBE0`：1 个 code xref，`0x0075096C`；data xref 为零。
- `sub_9ADAF0`：10 个 code xref，`0x00741233, 0x00743025, 0x007435F6, 0x00743DC2, 0x0074A580, 0x0074DEAA, 0x0074F410, 0x0074FEC0, 0x0075093A, 0x007517EC`；data xref 为零。
- `sub_9CF050`：6 个 code xref，`0x009061BB, 0x0090C917, 0x009BCC22, 0x009BEFFD, 0x009C7DA2, 0x009C7DC5`；data xref 为零。

三个入口 VA 的 little-endian 绝对地址在目标 PE 全文件中均无出现，三个入口均不导出。最终补取 IDA stack-frame 表时服务超时；该失败调用没有产生可用输出，也没有被纳入证明。上述 xref 集和 receiver 分类来自超时前已完成的只读查询，所有被提升的数值前提另由磁盘字节 fail closed。

## 两行 VERIFIED 的 allocation/extent 闭合

`0x0034FC5D` 的唯一 caller `sub_7508C0` 预留 `0x3C4` bytes，形成 `EBP-0x3A8` receiver 并转发为 `a2`。以 `EBP-0x10` 为保守高界，从 receiver 起仍有 `0x3A8-0x10=920` bytes：

```text
696 + 0xA9 = 865 <= 920
```

`0x005ADB94` 的完整 allocation source 如下。`available` 同样只计算到保守的 `EBP-0x10`，没有用相邻字段访问或旧 v4 boolean 补范围。

| caller | call VA | frame reservation | receiver | available | 目标 end |
| --- | --- | ---: | ---: | ---: | ---: |
| `sub_740E10` | `0x00741233` | `0x450` | `EBP-0x3F8` | 1000 | 865 |
| `sub_742EF0` | `0x00743025` | `0x430` | `EBP-0x3F8` | 1000 | 865 |
| `sub_743550` | `0x007435F6`, `0x00743DC2` | `0x3EC` | `EBP-0x3A8` | 920 | 865 |
| `sub_74A500` | `0x0074A580` | `0x3DC` | `EBP-0x3A8` | 920 | 865 |
| `sub_74DE30` | `0x0074DEAA` | `0x3D4` | `EBP-0x3A8` | 920 | 865 |
| `sub_74F360` | `0x0074F410` | `0x3CC` | `EBP-0x3C8` | 952 | 865 |
| `sub_74FBE0` | `0x0074FEC0` | `0x480` | `EBP-0x43C` | 1068 | 865 |
| `sub_7508C0` | `0x0075093A` | `0x3C4` | `EBP-0x3A8` | 920 | 865 |
| `sub_751650` | `0x007517EC` | `0x498` | `EBP-0x428` | 1048 | 865 |

这些 receiver 全部是当前 caller 的 EBP-relative stack allocation，和 PE writable `.data` 中绝对 manager slot `[0x00C71678,0x00C7167C)` 属于不同映射域；因此两行的所有闭合来源均与 slot 不相交。

## 第三行保留 UNKNOWN 的精确原因

两个局部 stack source 已闭合，但不能把局部结果提升为整行：

| caller / call | frame / receiver | 可用 extent | end |
| --- | --- | ---: | ---: |
| `sub_906100 / 0x009061BB` | `0x604 / EBP-0x5F0` | 1504 | 360 |
| `sub_90C7E0 / 0x0090C917` | `0x618 / EBP-0x5EC` | 1500 | 360 |

仍未闭合的四个入口是：

- `0x009BCC22`：从动态 pointee `+0x1E6C` 加 `0x20EC` 得到 receiver；root allocation/extent/forwarder universe 未闭合。
- `0x009BEFFD`：从 caller-owned container 的动态 pointee 加 `0x20EC`；同样未闭合根来源。
- `0x009C7DA2`、`0x009C7DC5`：`sub_9C79A0` 内两组 `0x5DC` stride embedded-object loop；该函数的所有 receiver roots/forwarders 尚未闭合。

所以这一行即使已知 `292+0x44=360` 且两个 stack caller 安全，也不能声明所有对象实例与 manager slot 不相交。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-fill-fixed-member-three-v10.json` | `FD7E374FB5A0863716042AC695B10928C06F6A173089DB58AA47123B8CF89433` |
| `out/cs-t0/analyze-manager-this-fill-fixed-member-three-v10.py` | `29A8DA90C4B8DC8B41BD6BAE9BA8A1F90476E4FBFBD6A8E0C5355E9CE545F9E4` |

```powershell
python out\cs-t0\analyze-manager-this-fill-fixed-member-three-v10.py --verify-existing
```

预期输出：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-fixed-member-three-v10.json sha256=FD7E374FB5A0863716042AC695B10928C06F6A173089DB58AA47123B8CF89433 verified=2 unknown=1 mutations=29
```

29 项语义 mutation 覆盖输入/目标/anchor、1/10/6 xref 集、call bytes、allocation set、offset/count/end、UNKNOWN 缺口、slot domain、classification/decision/permissions；全部 fail closed。

## 操作边界

本轮只读取冻结 JSON、目标磁盘 PE 和只读 IDA 数据。没有启动游戏，没有打开或读取目标进程，没有附加调试器，没有修改 IDB；没有修改 production、tests、既有 formal 产物、index、status 或 research plan。机器 JSON 的权限全部为 false。
