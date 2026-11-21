> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this fill-n298 exact6 v13 handoff

日期：2026-09-02

## 结论

`V4_N298_LARGE_LAYOUT_CLOSURE` 的六个 `sub_9C79A0` memset 行均保留为
`UNKNOWN_FORWARDER_RECEIVER_UNIVERSE_NOT_CLOSED`：本轮闭合了逐行 byte
算术、`n298`/`n60` 已知 heap 根、`n60` 嵌入选择器、已知 stack 根和 slot-domain
不相交，但没有闭合 `sub_9C8350` 的两个嵌套 forwarder caller 根
`sub_9CC1A0` / `sub_9CE5A0`。由于 `0x009C83C8` 与 `0x009C8405`
无条件进入同一个 `sub_9C79A0`，该缺口同时污染六行。

本产物只能作为 `0 VERIFIED / 6 UNKNOWN` 的负向分区合并；不能用于
manager-this、whole-image alias 或运行时权限升格。

## 冻结绑定

- `out/cs-t0/native-join-manager-this-bulk-memset-v3.json`
  SHA-256 `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json`
  SHA-256 `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe`
  SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- PE32 machine `0x014C`，preferred image base `0x00400000`。

## exact6 逐行算术

所有区间均为相对 `sub_9C79A0` receiver 的半开 byte 区间。
`_DWORD *` 三行已显式乘四，没有把元素下标误当 byte offset。

| callsite RVA | start byte | count upper bound | end exclusive | 状态与最小缺口 |
|---|---:|---:|---:|---|
| `0x005C7A15` | `14 (0xE)` | `0x35 (53)` | `67 (0x43)` | UNKNOWN；`SUB_9C8350_NESTED_FORWARDER_CALLER_ROOT_UNIVERSE` |
| `0x005C7BAD` | `296 (0x128)` | `0x30 (48)` | `344 (0x158)` | UNKNOWN；同上 |
| `0x005C7C8D` | `25932*4 = 103728 (0x19530)` | `0x7EC (2028)` | `105756 (0x19D1C)` | UNKNOWN；同上 |
| `0x005C7E7A` | `31056*4 = 124224 (0x1E540)` | `0x7C (124)` | `124348 (0x1E5BC)` | UNKNOWN；同上 |
| `0x005C7EC6` | `31101*4 = 124404 (0x1E5F4)` | `0x4B (75)` | `124479 (0x1E63F)` | UNKNOWN；同上 |
| `0x005C7ED6` | `31120*4 = 124480 (0x1E640)` | `0x3E (62)` | `124542 (0x1E67E)` | UNKNOWN；同上 |

## `sub_9C79A0` direct xref universe

IDA 只读查询返回 exact8、非截断：

`0x006F903B`, `0x0091831C`, `0x00959F71`, `0x00959FC7`,
`0x0095A472`, `0x009C7733`, `0x009C83C8`, `0x009C8405`。

- `0x006F903B`：`sub_6F9000` 的 stack 参数；已知来源包含 `n60`
  嵌入状态和一个大 stack aggregate。
- `0x0091831C`：`n298+0xFA4`。
- `0x00959F71`, `0x00959FC7`, `0x0095A472`：`this+0xFA4`；已检查的
  上游调用链均以 `n298` 直接值或别名进入。
- `0x009C7733`：保持同一个 `this`。
- `0x009C83C8`, `0x009C8405`：`sub_9C8350` 保持同一个 `this` 的 tail
  forwarder。其 caller 中 `0x009CC254` 与 `0x009CE68F` 分别接收
  `sub_9CC1A0` / `sub_9CE5A0` 的外部 `this`；本轮未证明这两个函数的完整
  caller/root/alias universe 都归入已知 allocation。因此这是唯一保留缺口。

## 已闭合的 allocation / extent 分区

对全映像 direct memory operand 写入的只读枚举得到：

- `n298` slot `VA 0x00C7169C` 只有 `0x009AAAB0` 非零置值和
  `0x009AB8CC` 清零。非零路径在 `0x009AAA80` push `0x4D088`，
  `0x009AAA85` fresh allocate，调用 `sub_950610`，随后写 slot。
  最小 allocation extent 为 `0x4D088 = 315528` bytes。
- `n60` slot `VA 0x00C71678` 只有 `0x00952847`、`0x009597D0` 两个非零
  置值以及 `0x009AB8A9` 清零。两条非零路径均 push `0x68E88`、fresh
  allocate、调用 `sub_6F1020` 后写 slot；最小 extent 为
  `0x68E88 = 429704` bytes。析构路径以相同大小释放。

`n298+0xFA4` 的最大写入末端：

`0xFA4 + 124542 = 128546 <= 315528 (0x4D088)`。

`n60` constructor 在 offset `158188 (0x269EC)` 构造 count 2、stride
`0x20660 = 132704` 的 embedded state；每项以 `+4` 进入本 receiver。
选择器 `n60+0x676AC` 的完整指令出现集将值约束为 `{-1,0,1}`，
`sub_705EE0` 把 `-1` 映射为 `0`，并把 `0/1` 保持在 `0/1`。最坏项末端：

`158188 + 132704 + 4 + 124542 = 415438 <= 429704 (0x68E88)`。

这些是 allocation extent 证明，不使用相邻字段访问代替 extent。

已闭合 receiver 的 heap pointee 与 stack aggregate 不属于 PE writable-image
slot 存储 `[0x00C71678, 0x00C7167C)`，故已知分区与 manager slot VA 不相交。
这项 domain 结论不补足未闭合 forwarder 的 alias universe。

## 产物与复现

- `out/cs-t0/native-join-manager-this-fill-n298-six-v13.json`
  SHA-256 `9DA0FD4893A6D393D5721B973F4E89623B463CB82506BE5A65DCAE6BB5CACFF4`
- `out/cs-t0/analyze-manager-this-fill-n298-six-v13.py`
  SHA-256 `4F14DB16DE6AFF52768197297A74806D74B4AB18E100FDA09AA4F867FFC94727`

只读重放：

```powershell
python out\cs-t0\analyze-manager-this-fill-n298-six-v13.py --verify-existing
```

预期输出：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-n298-six-v13.json sha256=9DA0FD4893A6D393D5721B973F4E89623B463CB82506BE5A65DCAE6BB5CACFF4 verified=0 unknown=6 mutations=12
```

reproducer 同时绑定 frozen inputs、target、exact6 membership/order、direct exact8、
allocation roots、缩放后的 byte 不等式、UNKNOWN 分类与全部 permission=false；
12 个语义变异必须全部被拒绝。

## 权限边界

本轮未启动游戏、未附加 debugger、未打开或读取进程、未修改 IDB、production、
测试或既有证据。`runtimeMutationAllowed=false`、`promotionAllowed=false`。
