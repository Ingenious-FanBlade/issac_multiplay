> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R02 incoming old/new count v1 逆向交接

日期：2026-09-06  
角色：`REVERSE`  
状态：`0 STATIC_CONFIRMED_NO_OVERLAP / 2 UNKNOWN`

## 结论

MT-R02 的 exact rows 为 `0x00008A62`、`0x00008B2A`。两行均不能证明
`oldCount <= newCount`，也没有更强的 allocation/span 不变量，因此失败安全地保持
`UNKNOWN`，不得进入 positive frontier。

冻结 PE 中的 `sub_408970` 先从 current descriptor 的 `+0x1C/+0x24`
读取两组 old count，再从 name-selected incoming descriptor 的相同字段读取 new
count。函数只比较 `oldCount != newCount`：当 new count 为正时按 `4*newCount`
请求新 heap backing；当 old count 为正时则无 clamp、无大小比较地执行
`memcpy(..., 4*oldCount)`。两行各自保留相同的最小 countermodel：

```text
newCount = 1
oldCount = 2
requested destination extent = 4 bytes
memcpy length = 8 bytes
```

旧计数的 `shl eax,2` 也没有独立 no-wrap 证明。故 destination/source span、完整
owner lifetime 以及与 GameRoot slot `[0x00871678,0x0087167C)` 的非别名结论均未闭合。

## 当前 PE fixed-point 结果

- 目标为 x86 PE32、ImageBase `0x00400000`、SizeOfImage `0x008FE000`，SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 两个 call 均位于 `sub_408970`（RVA `0x00008970`），精确 bytes 分别为
  `E8 78 7B 6E 00` 与 `E8 B0 7A 6E 00`，都解析到 `memcpy 0x00AF05DF`；
  指令边界与 cdecl helper ABI 已确认。
- `sub_408970` 的当前 PE caller universe 是两条 direct E8：
  `0x0040A678`、`0x0040AB98`；relative E9 和 raw address literal 均为零。
- 两个 selector 都在 stride `0x13C` 的 descriptor array 中只按名字选择 incoming
  record，不对 prior/new `+0x1C/+0x24` 建立单调关系。
- selector 上游仍然很大：第一组为 `1047` 条 direct E8 加 `1` 条 E9 thin
  forwarder（合计 `1048`），第二组为 `90+1=91`。thin forwarder
  `0x0040A5B0/0x0040AAD0` 均有 address-taken literal；其 `a3` 参数原样进入
  selector，当前 PE 也存在 `a3=0` 的代表性调用路径。
- 因此 direct/indirect/address-taken/split-EH/stack/return/reentry 的 descriptor
  count producer 与 lifecycle fixed point 没有形成可提升的全称不变量。candidate
  明确把这部分写入 `incomingPairProducers`、`callersAndForwarders` 和
  `allWriters` 的 unresolved 列表，而不是以核心函数的两条 direct caller 假装闭环。

## 公开 envelope 与冻结输入

candidate 严格采用
`cs-t0.manager-this.mt-r02-incoming-pair-bound.positive-candidate.v1`，绑定公开 Wave B
envelope：

`out/cs-t0/native-join-manager-this-mt-wave-b-candidate-envelope-v1.md`  
SHA-256：`BDA205AB7A2A6A4DAA61E9B5B1C61EBB9F1AD269CE01AC22805466E62F182C9F`

它同时绑定 v31 frontier、v31 independent audit、callerHeap v29 candidate/audit、
copy-range v7 与 v33 breakdown。builder/analyzer `.py` 没有作为 TEST input。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r02-old-new-count-v1.py` | `091FABE8DB08E017AFD29903CC922D6ADE4416A7FA71E9B0193FB5B9D3054368` |
| `out/cs-t0/native-join-manager-this-mt-r02-old-new-count-v1.json` | `A484882A285B1CB41907F4F02C34FB5B0A540D539C5290172ACFCD0D62241C3A` |

按用户压缩后的测试规模执行：

```powershell
python out/cs-t0/build-manager-this-mt-r02-old-new-count-v1.py
python out/cs-t0/build-manager-this-mt-r02-old-new-count-v1.py --verify-existing
```

create-new 与一次 verify-existing 字节一致。作者自测只保留 6 个核心 mutation，覆盖
目标 PE、exact rows、伪造 `oldCount<=newCount`、删除 countermodel、错误提升与权限；
`6/6` 均 fail closed。没有生成额外 self-test artifact，也没有运行 Cargo 或全量 T0
构建。

## 隔离与权限

本轮没有启动游戏、打开/读取进程、附加调试器、修改 IDB、生产代码、既有证据、
status/index 或 frontier。没有执行独立 TEST 审计；candidate 的 `0V/2U` 仍需由
作者分离的 TEST 角色从冻结 PE 与公开输入独立重算后才能作为本波失败安全结论合入。
