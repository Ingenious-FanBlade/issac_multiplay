> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R02 frozen-asset census/state+4 graph v1 逆向交接

日期：2026-09-06  
角色：`REVERSE`  
状态：`STAGE_C_COMPLETE / BOUNDED_STOP / 0 STATIC_CONFIRMED_NO_OVERLAP / 2 UNKNOWN`

## 1. 冻结输入与 create-new 产物

本轮只处理 CS-T0-05-04 的 MT-R02 exact rows `0x00008A62`、`0x00008B2A`，
量词为 `ALL_REACHABLE_EXECUTIONS`，资产域为 `FROZEN_VANILLA_ASSETS`，Mods 排除。
没有修改 production、frontier、README/status/index 或既有证据。

- 目标 PE：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，
  SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，
  PE32/I386，ImageBase `0x00400000`，SizeOfImage `0x008FE000`。
- frozen vanilla manifest：
  `out/cs-t0/native-join-manager-this-mt-r02-frozen-vanilla-asset-manifest-v1.json`，
  SHA-256
  `188A1B2A2F97196045E2E70EDDB378161200CABBB4BE6A5B233B6C20A94044A3`。
- 独立 TEST 预封印 spec：
  `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-acceptance-spec-v2.json`，
  SHA-256
  `2F2ECB1C5A76990443C93E579DBCA4272C7B8200AA6C1B430F70AADCB3A879EA`。
- sealed preparation：
  `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-preparation-v2.json`，
  SHA-256
  `D33E603114743CEA529DA2F0EEB43BA8F7AC569008ADF6234A6CDBF0DEADB871`。
- TEST tool 只按独立 TEST 公布的 path/hash 绑定；REVERSE 没有读取、import 或执行：
  `out/cs-t0/prepare-manager-this-mt-r02-stage-b-sealed-v2.py`，
  SHA-256
  `C46FB8C98BFE24602351D73A4A28B8A939639953B3EFE3B0F28D86535E930148`。
- preseal design：
  `docs/refactor/cs-t0/t0-05/06-manager-identity/02-copy-move/native-join-manager-this-mt-r02-stage-b-sealed-v2-preseal-design.md`，
  SHA-256
  `C12634F216BDCF403EFF8DDE1A88EDBBCC3AC209AAF891674A489E0E19294AD5`。

本轮 create-new 文件：

- 结构化 REVERSE candidate：
  `out/cs-t0/native-join-manager-this-mt-r02-asset-census-state4-graph-v1.json`，
  2,376,819 bytes，冻结物理 SHA-256
  `99CC4F2F85711961DD4B89177E97E10DAA1624C4CC2DA9E68C60932979DDA2E7`。
- composition builder：
  `out/cs-t0/build-native-join-manager-this-mt-r02-asset-census-state4-graph-v1.py`，
  SHA-256
  `98DC479DC810F37A960381A6BF66B5060E1CCB3547ABB9C52CAAFDCD517C328F`。
- IDA 只读 census：
  `out/cs-t0/native-join-manager-this-mt-r02-ida-static-census-v1.json`，
  SHA-256
  `6A90599FD0F0655E448E597A32D96B9A88A231AB4C4B9513ACD8C0456C909990`。
- IDA 只读 collector：
  `out/cs-t0/collect-native-join-manager-this-mt-r02-ida-static-census-v1.py`，
  SHA-256
  `DD0C7923B3C28B0714B8B0292EBBE8208CF54C2E831DD6EC553F1CB0C8BE1487`。

builder 是确定性 composition builder，不是独立验收器；它没有读取 TEST tool。第一次运行
在 candidate 创建前因 Stage A manifest 键名不匹配而 fail-fast；修正为 `freshOutput` 后的
一次成功运行以 create-new 模式生成 candidate。没有追加作者侧 mutation 或验收测试。

## 2. parser 字段与 frozen-domain no-wrap

`sub_40ED50` 内的只读静态证据闭合了 frozen asset record 的字段关系：

| XML 元素 | descriptor count | parser 证据 | frozen max | `4*max` |
|---|---|---|---:|---:|
| `LayerAnimations` | `+0x1C` | `0x412284..0x4122C4` | 57 | 228 |
| `NullAnimations` | `+0x24` | `0x413457..0x413479` | 26 | 104 |

两条 parser loop 在 count `>=0x40` 时只打印 warning，随后继续 increment/parse；没有
native hard clamp。因此不能把 `0x40` 当作全称上界。对 manifest 冻结且成功 parse 的
13,261 个 Animation records，具体最大值 57/26 足以证明其 `4*count` 不 wrap；但只有
在 runtime descriptor 确实来自该 frozen record 且 state+4 identity fixed point 闭合时，
该算术结论才能提升到目标 memcpy 的全称结论。后一个 join 仍开放。

对应 proof atoms 已逐字绑定 PE RVA/bytes/SHA，包括：

- `R02_PARSER_LAYER_COUNT_STORE`；
- `R02_PARSER_NULL_COUNT_STORE`；
- `R02_PARSER_LAYER_WARNING_NO_CLAMP`；
- `R02_PARSER_NULL_WARNING_NO_CLAMP`。

## 3. selector universe 完整覆盖与安全剪枝

collector 对两条 selector 的所有 IDA xref 建立逐边记录，并只在同一 basic block 的
保守 ESP dataflow 证明 flag 为立即数非零时剪枝。任何 register/dynamic/跨块参数均保持
`UNKNOWN`，不参与剪枝。

| selector | original | direct call | address-taken forwarder | proved `flag!=0` pruned | proved `flag=0` retained | unknown flag retained | retained total |
|---|---:|---:|---:|---:|---:|---:|---:|
| primary `sub_40A5D0` / state `+0x30` | 1048 | 1047 | 1 | 935 | 25 | 88 | 113 |
| secondary `sub_40AAF0` / state `+0x50` | 91 | 90 | 1 | 79 | 5 | 7 | 12 |
| total | 1139 | 1137 | 2 | 1014 | 30 | 95 | 125 |

保留边中动态 name 共 100 条；owner/source runtime identity 对所有保留边均未做无根据的
静态认定。已知两条具体 `flag=0` 链仍被明确保留并增强为 finite literal set：

- `0x0003ABE6`：`Idle` 或 `Idle2`；
- `0x000E54C5`：`HeadLaughUp`、`HeadLaughDown` 或 `HeadLaughRight`。

primary thin forwarder `sub_40A5B0` 在 `0x0040A5C2` tail-jump 到 selector，并由
`0x0086739D` 的 `SetAnimation` callback registration address-take；secondary 对应
`sub_40AAD0`、`0x0040AAE2`、`0x0086741B` 与 `SetOverlayAnimation`。callback
receiver/name/flag/reentry 仍显式为 `UNKNOWN`。

## 4. state+4 writer graph 与 fixed point

三个 native writer 的局部写入语义和全部静态 xref 已覆盖：

| writer | state+4 语义 | static xref count | 局部结论 |
|---|---|---:|---|
| `sub_408830` | reset 后绑定 incoming descriptor | 60 | `CLOSED_LOCAL_WRITE_SEMANTICS` |
| `sub_408970` | migrate 后绑定 incoming descriptor | 2（两 selector） | `CLOSED_LOCAL_WRITE_SEMANTICS` |
| `sub_4086E0` | 从 source state 复制 descriptor identity/backings | 2 | `CLOSED_LOCAL_WRITE_SEMANTICS` |

本轮 candidate 覆盖 sealed spec 要求的全部 13 个 risk edge id：

1. `PARSER_LAYER_ANIMATIONS_TO_DESCRIPTOR_PLUS_1C`
2. `PARSER_NULL_ANIMATIONS_TO_DESCRIPTOR_PLUS_24`
3. `PRIMARY_FLAG_ZERO_SELECTOR_TO_SUB_408970`
4. `SECONDARY_FLAG_ZERO_SELECTOR_TO_SUB_408970`
5. `STATE4_SUB_408830_WRITE`
6. `STATE4_SUB_408970_WRITE`
7. `STATE4_SUB_4086E0_COPY`
8. `STATE4_COPY_MOVE_PROPAGATION`
9. `STATE4_RELOAD_REBIND`
10. `STATE4_ADDRESS_TAKEN_REENTRY`
11. `ASSET_MAX_TO_LAYER_REACHABLE_EDGE_JOIN`
12. `ASSET_MAX_TO_NULL_REACHABLE_EDGE_JOIN`
13. `RVA_00008B6A_ADOPTION_TO_STATE_PLUS_08`

其中 parser 的 frozen-record 字段关系和三个 writer 的局部写入语义有界闭合；8 个全称
edge 保持 `OPEN`：两条 flag-zero selector/owner join、copyMove、reload、addressTaken、
两条 asset-max reachable join，以及 `0x8B6A` adoption。

fixed point 的首个生产者开放类是 reload/dynamic asset owner-source identity；
address-taken callback reentry 独立开放。copy/move 的 source state descriptor/backing
provenance 与 lifetime 也未闭合。因此不能从 60/2/2 的 direct xref census 外推为
`ALL_REACHABLE_EXECUTIONS` 的完整 writer/identity fixed point。

## 5. asset-max/old<=new join

manifest 中 500 条 asset-local pre-prune risks 全部投影到 candidate：Layer row 253，
Null row 247。它们仍严格标记为 `ASSET_LOCAL_PRE_PRUNE_NOT_NATIVE_REACHABILITY_PROOF`。

- 500 条中共有 173 条资产局部 `assetMaximumOldCount=2, incomingCount=1` witness；
- 已知 Idle/Idle2/HeadLaugh* finite name 集合匹配 17 条 risk，其中恰有 1 条资产局部
  `old=2,new=1` witness；
- 这些是真实 frozen asset XML producer records，但 owner 到具体 ANM2 的绑定、current
  animation 到 incoming animation 的顺序、以及 state+4 可达迁移均未闭合，所以不能把它们
  报告为具体 reachable/runtime counterexample。

两项 `assetMaxEdgeJoin` 均完整报告 required fields：`descriptorField`、
`assetDomainMaximum`、`reachableOldCountSet`、`reachableNewCountSet`、
`joinedRiskEdgeIds`、`state`、`unresolved`。两者 state 都为 `OPEN`；reachable sets 只报告
frozen candidate values，不伪称为已闭合的 runtime reachable sets。

## 6. `0x8B6A` adoption gap

`sub_408970` 的第二 backing 分支在目标 memcpy 后执行：

```text
0x00008B63: C7 46 0C 00 00 00 00    mov [esi+0x0C], 0
0x00008B6A: 89 5E 08                mov [esi+0x08], ebx
0x00008B6D: 89 7E 04                mov [esi+0x04], edi
```

candidate 通过 `RVA_00008B6A_ADOPTION_TO_STATE_PLUS_08` 风险边将该事实纳入 graph 与
`sourceLifetimeAndIdentity`。它不能修复已发生的 memcpy span，并使第二 backing 的
replacement/adoption lifetime fixed point 继续 `OPEN`。

## 7. 两行判定与 bounded stop

| RVA | backingExtent | countAndNoWrap | sourceLifetimeAndIdentity | targetIntervals | verdict |
|---|---|---|---|---|---|
| `0x00008A62` | `OPEN`：未闭合 old/current→incoming 的 `old<=new` | `OPEN`：frozen asset 算术有界，但 runtime descriptor provenance 未闭 | `OPEN`：copy/move、reload、callback owner/source fixed point 未闭 | `OPEN`：extent/identity 未闭，不能全称排除 slot alias | `UNKNOWN` |
| `0x00008B2A` | `OPEN`：同一 old/new join 缺口 | `OPEN`：同上 | `OPEN`：共同缺口外还有 `0x8B6A` adoption | `OPEN`：同上 | `UNKNOWN` |

第一开放义务仍是 `backingExtent`，因此逐字保留 sealed baseline gaps：

- `R02_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN`；
- `R02_SECOND_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN`；
- countermodel：`oldCount=2,newCount=1 allocates 4 destination bytes and copies 8 bytes`。

没有发现支配两条 memcpy 的 hard bound，也没有证明 `oldCount<=newCount` 或更强 span
invariant。资产 census 提供了真实 asset-local `old=2,new=1` records，但最早未闭边仍是
owner/source/state transition reachable join；在无 runtime 授权下保持 `UNKNOWN` 并
bounded stop，不做无界全程序枚举。

## 8. 权限与后续 TEST

本轮仅使用 IDA Pro MCP 只读能力与工作区文件读取/append-only 生成。没有启动游戏，
没有使用 x32dbg，没有打开/读取目标进程，没有修改或保存 IDB，没有运行 TEST tool 或
candidate acceptance。candidate 的 `sealedTestContract.strictV33DefaultExitCode=99`，
所有 permission 字段为 `false`。

下一步只应由独立 TEST 按已封印 v2 spec 做压缩验收：一次 deterministic census 复算、
exact row/target/manifest/risk-edge coverage，以及 3 个核心 negative mutations。REVERSE
不自验收本 candidate。
