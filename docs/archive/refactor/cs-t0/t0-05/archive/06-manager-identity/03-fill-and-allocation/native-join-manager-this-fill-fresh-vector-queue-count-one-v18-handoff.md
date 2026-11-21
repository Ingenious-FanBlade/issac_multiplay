> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this fresh-vector queue-count exact-one v18 handoff

## 结论

本报告是 v17 的 superseder，只补查 `memset` exact1 `RVA 0x0068DF56` 所读取的
`DWORD[node+4]` 的唯一发布链。结论仍为：

`UNKNOWN_PUBLISHED_COUNT_FULL_UINT32_DOMAIN_SUM_WRAP_OPEN`

唯一发布函数 `sub_AADBF0`（VA `0x00AADBF0`）把第二参数不经校验写入
`DWORD[node+4]`，然后通过 owning queue 的 `this+8` 发布 12 字节 payload node。
完整可执行 `.text` rel32 扫描只有一个直接调用：`0x00A8D7BB`，位于
`sub_A8D580`（VA `0x00A8D580`）。可执行段没有 publisher VA 的 raw absolute
address-taken occurrence。

该调用的机器表达式为：

```text
v28     = signed32(end - start)
count   = uint32(v28 * channels)
node+4  = count
```

调用路径只证明 `start >= 0`、`start < end`、`v28 > 0`，所以 `v28` 的最强有符号
和无符号界都是 `1..INT32_MAX`。`channels` 是在 `0x00A8D7A3` 重读的原始
`DWORD[this+0xA8]`；本局部 slice 没有它的硬上界。`0x00A8D7B3` 的 `IMUL`
只保留低 32 位，且 flags 未被消费。因此发布 count 的最强结论只能是：

- unsigned：`0..UINT32_MAX`；
- signed reinterpretation：`INT32_MIN..INT32_MAX`。

取允许的 `v28=1` 时，未受局部硬 guard 限制的 raw channels DWORD 可使 count
覆盖整个 `uint32` 域。

## 为什么前置 allocation 不能提供硬上界

同一路径先在 `0x00A8D6DF` 计算同一形式的 32 位 count，再在
`0x00A8D6EC..0x00A8D6F5` 构造 `4*count` 的 allocation request。`MUL` 的 overflow
flag 经 `SETO/NEG/OR` 被折叠为 `UINT32_MAX` request，但函数内没有在 publish 前
按该 flag 分支拒绝。随后仍调用 `sub_A648B0`，并以截断的 `uint32(4*count)` 调用
`memset`。

本任务按约束不向 allocator 或 codec 外部 producer 宇宙扩张，因此不能把“传入
`UINT32_MAX` sentinel”偷换为已经证明的 `count <= UINT32_MAX/4`。需要独立冻结的
allocator no-return/reject 契约，或 channels/count producer 硬界，才能升级该前提。

## 对 later positive request 的影响

v17 已冻结后续 `0x00A8DF39` 为 unchecked：

```text
uint32(old_count + v37),  1 <= v37 <= INT32_MAX
```

本报告证明 `old_count` 的局部最强上界仍是 `UINT32_MAX`，没有建立
`old_count <= UINT32_MAX-v37`。最小反模型是：

```text
producer v28 = 1
producer channels = UINT32_MAX
published old_count = UINT32_MAX
later v37 = 1
mathematical sum = UINT32_MAX + 1
machine result = 0
```

该反模型不被本局部链的任何硬 guard 排除。因此 later positive request 的 32 位
无符号加法可以回绕；该 exact-one 行不能 promotion。

## 冻结绑定

| 角色 | 路径 / VA | SHA-256 |
| --- | --- | --- |
| superseded v17 JSON | `out/cs-t0/native-join-manager-this-fill-fresh-vector-one-v17.json` | `707A6C727C330A95B5A93B9C927284F38EFDB76B7569F5BBD2EF02A175A5AE08` |
| target EXE | `F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| producer blob | VA `0x00A8D580`, size `0x3C8` | `6A59E50CB95FF7C9C7E93E0E92E12F2A9782771366C0DBF85C587F8C20D0D9F2` |
| publisher blob | VA `0x00AADBF0`, size `0x84` | `73C8001D0EE823264FD91F396743F17C47B02994B33C7EA9B82F2F41BCB17511` |

## 新建产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/analyze-manager-this-fill-fresh-vector-queue-count-one-v18.py` | `142B2841D7606E8F3FD80A733FF156567F71F1F0DDFC3193A699E7B03DAE6BD3` |
| `out/cs-t0/native-join-manager-this-fill-fresh-vector-queue-count-one-v18.json` | `10AA255A0ACECBF5DA88ACDD62D2C0A1DD62A53546C2B0D7564E01CD738DCDA4` |

```powershell
python out\cs-t0\analyze-manager-this-fill-fresh-vector-queue-count-one-v18.py --verify-existing
```

预期：exact1 `verified=0 unknown=1`，18/18 mutation 全部 fail closed，并输出
`UNKNOWN (published count spans full uint32 domain; later positive sum can wrap)`。

verifier 从磁盘重新绑定 target PE/hash、producer/publisher 函数 blob、两段关键字节、
publisher 的完整可执行 rel32 caller 集合、无 raw absolute address-taken occurrence、v17
exact-one 身份和本报告全部算术/权限结论。

## 权限边界

只使用 IDA 只读静态反汇编和 PE 磁盘重放；没有启动游戏、附加调试器、打开/读取进程、
修改 IDB、生产代码、测试或旧证据。`rowPromotionAllowed=false`，whole-image alias 与
manager-this promotion 继续为 false，全部权限保持 false。
