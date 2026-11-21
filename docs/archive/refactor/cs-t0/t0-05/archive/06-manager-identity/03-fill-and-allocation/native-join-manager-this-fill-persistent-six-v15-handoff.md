> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this Persistent exact6 v15 handoff

日期：2026-09-02

## 结论

`V4_PERSISTENT_DATA_CALLER_AND_RECEIVER_CLOSURE` exact6 当前为
`0 VERIFIED / 6 UNKNOWN`。本轮先核对了六个 PE call anchors、逐行 byte
区间、`sub_91B650 -> sub_91ADF0` 与 `sub_9262B0` 的 direct caller 集，随后在
两个首个真实 root 停止，避免把未完成的容器/静态对象证明泛化成安全结论：

- `0x0051B6A7`：`sub_91ADF0` 在 `0x0090C2F4` 接收
  `*(sub_9154F0 result)+0x18`。fresh insertion 分支明确分配 `0xFA8`
  bytes，且 `24+3741=3765<=4008`；但 existing-node 返回分支的完整
  producer/copy/move/alias universe 未闭合，因此该行仍 UNKNOWN。
- 其余五行：`0x007BC713` 在静态存储 `unk_C812B0` 上调用
  `sub_925F80`，后者立即把同一 receiver 送入 `sub_9262B0`。最大末端为
  `3940` bytes；但没有 linker/type/allocation 证书给出该静态逻辑对象至少
  `3940` bytes 的 extent。`sub_925F80` 随后访问 `this+3940` 只是相邻成员
  访问，按门槛明确不能替代 allocation extent，因此五行均 UNKNOWN。

本产物只能作为负向分区合并；manager-this 与 whole-image promotion 均为
`false`。

## 冻结输入

- range audit v5：`out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json`
  SHA-256 `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- rejected v4（只绑定 exact set，不采信 narrative）：
  `out/cs-t0/native-join-manager-this-bulk-memset-v4.json`
  SHA-256 `0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C`
- corrected whole-image indirect split v2：
  `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`
  SHA-256 `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- target `isaac-ng.exe` SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
  PE32 image base `0x00400000`，corrected executable VA range
  `[0x00401000,0x00B18000)`，SizeOfImage `0x008FE000`。

## exact6 与算术

`sub_91B650` 内部单一 memset callsite 的五个 `sub_91ADF0` 调用变体为：

| caller VA | Persistent object start | count | end exclusive |
|---|---:|---:|---:|
| `0x0091AE37` | 56 | 642 | 698 |
| `0x0091AE61` | 2792 | 733 | 3525 |
| `0x0091AE85` | 3584 | 7 | 3591 |
| `0x0091AE95` | 3591 | 104 | 3695 |
| `0x0091AEB9` | 3695 | 46 | 3741 |

其余五行均相对 `sub_9262B0 this`：

| callsite RVA | start | count | end exclusive | 状态 |
|---|---:|---:|---:|---|
| `0x005262DD` | 56 | 642 | 698 | UNKNOWN；static extent open |
| `0x005262F0` | 2792 | 733 | 3525 | UNKNOWN；static extent open |
| `0x00526342` | 3591 | 104 | 3695 | UNKNOWN；static extent open |
| `0x00526352` | 3695 | 46 | 3741 | UNKNOWN；static extent open |
| `0x005263B5` | 3860 | 80 | 3940 | UNKNOWN；static extent open |

全部加法均小于 `2^32`，无 32-bit wrap。已闭合 fresh heap、n298 与 stack
根的区间不触及 manager slot `[0x00C71678,0x00C7167C)`；静态候选实际写区间
`[0x00C812E8,0x00C82214)` 数值上也与 slot 不相交，但该数值不相交不能修复
其逻辑 extent 缺口。

## caller universe

- `sub_91B650` direct exact5：`0x0091AE37`, `0x0091AE61`,
  `0x0091AE85`, `0x0091AE95`, `0x0091AEB9`，全部在 `sub_91ADF0`。
- `sub_91ADF0` direct exact5：`0x0090C2F4`, `0x0090CCC7`,
  `0x009194F3`, `0x00919E39`, `0x00919E4B`。
- `sub_9262B0` direct exact11：`0x00925FFC`, `0x0092646A`,
  `0x0092682F`, `0x00926AD1`, `0x00926C0A`, `0x00926E0B`,
  `0x00926E7F`, `0x0092B5CA`, `0x0092BA19`, `0x00959E8D`,
  `0x009D9E01`。
- `sub_925F80` direct exact8：`0x007BC713`, `0x0090636A`,
  `0x009155A8`, `0x009194DE`, `0x00919E14`, `0x00919E23`,
  `0x0091A4B2`, `0x0095064A`。

whole-file little-endian VA 搜索未发现 `sub_91B650/sub_91ADF0/sub_9262B0/
sub_925F80` 的 absolute address-taken literal。此结论绑定 corrected v2 的完整
executable range；它不把两个未闭合 receiver/extent 根升级为 closed。

## 产物与复现

- `out/cs-t0/native-join-manager-this-fill-persistent-six-v15.json`
  SHA-256 `B5263197325F56B59D776DF8F73E04DD258EAF876BD9C30AF366EC257625A213`
- `out/cs-t0/analyze-manager-this-fill-persistent-six-v15.py`
  SHA-256 `0BAAB4093D74866CADE637CD902D2CD78CE01708C848798FDC71C7E2643B2E87`

```powershell
python out\cs-t0\analyze-manager-this-fill-persistent-six-v15.py --verify-existing
```

预期为 `verified=0 unknown=6 mutations=18`。reproducer 绑定 exact6
membership/order、六个 target anchor、三份冻结输入、corrected PE ranges、四组
direct xref counts、两个最小缺口、逐行无回绕算术、UNKNOWN 分类与全部
permissions=false；18 个 mutation 必须全部 fail closed。

## 权限边界

未启动游戏、未附加 debugger、未打开/读取进程、未修改 IDB、production、测试或
既有证据。`runtimeMutationAllowed=false`、`promotionAllowed=false`。
