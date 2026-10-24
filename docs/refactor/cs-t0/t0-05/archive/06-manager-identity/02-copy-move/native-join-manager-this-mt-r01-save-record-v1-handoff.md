> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R01 save-record 正向候选 v1 handoff

## 1. 结论与角色边界

本产物是 **REVERSE 角色候选**，仅覆盖 `MT-R01 {0x001CBEB5}`。最终交付给 TEST 的机器候选为 `native-join-manager-this-mt-r01-save-record-v3.json`。冻结 PE 上的 v23/v24 `167` 个 source/context seed 已由 `165 + {B-016,B-057}` 闭合为 `167/167`；逐行候选 verdict 为：

- `disposition=STATIC_CONFIRMED_NO_OVERLAP`
- `fullyAdjudicatedNoOverlap=true`
- `unresolved=[]`

这里的 no-overlap 指目标写区间不与仍存活的 manager/owner object 重叠；erase 的 source/destination 彼此重叠是预期行为，调用 `memmove` 正是为此。该候选 **没有完成独立 TEST 审计、没有自行接纳、没有修改 v30 frontier**。只有独立 TEST 复算通过后，R01 才能产生 `+1`。

## 2. 冻结目标与目标指令

- PE：`F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe`
- SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- 文件大小：`9176064`
- `IMAGE_FILE_MACHINE_I386 (0x014C)`、PE32、ImageBase `0x00400000`、SizeOfImage `0x008FE000`
- 目标 RVA/VA：`0x001CBEB5` / `0x005CBEB5`
- 指令字节：`E8 03 4A 52 00`
- rel32 目标：`0x00AF08BD`，字节 `FF 25 48 87 B1 00`，IDA 解析为 `memmove` import thunk
- containing function：`sub_5CBEA0 [0x005CBEA0,0x005CBECA)`，`ECX=vector receiver`，栈参数为 return-iterator pointer 与 selected iterator，`RETN 8`；内部按 cdecl 顺序压入 `length/source/destination`，调用后清理 12 字节。

目标调用的精确区间为：

- `destination=[kk,end-4)`
- `source=[kk+4,end)`
- `length=end-(kk+4)`
- `kk=begin+4*j` 且 `0<=j<(end-begin)/4`
- `begin<=end<=capacityEnd`，所以 `kk+4`、`end-(kk+4)`、`end-4` 均不回绕，source/destination 均包含于 `[begin,capacityEnd)`。

## 3. REPENTOGON 只读搜索种子

只读仓库 `D:/projects/REPENTOGON` 的 HEAD 为 `421c91d082db9be0ae02ac49801b60e3c5a0ad0c`；`version.txt=1.1.2`，README 目标游戏为 Repentance+ `v1.9.7.12.J273`。五个固定文件哈希全部吻合 v33：

| 路径 | SHA-256 |
|---|---|
| `libzhl/functions/GameStatePlayer.zhl` | `79566479F9931CE3B542ED3A20A36FEA1058EBA4EB2DA538D818705809832336` |
| `libzhl/functions/EntityPlayer.zhl` | `726F6E58E7413CD1BBAF5E6B0E02CE96D00CC5BF8D51B5ABBE485293F6F6C2A6` |
| `libzhl/functions/EntityList.zhl` | `77D96C3629AC8105258C1EB22560445350AB2D171CC2E3FCC615C834B0BD6192` |
| `libzhl/functions/EntityList_EL.zhl` | `DCC8B627EFC3459C5DE01B5F7FD4F493FF4DD7703F132B3EC51FD36BD57A87F9` |
| `repentogon/SaveStateManagement/EntitySaveStateManagement.cpp` | `08CC49255E11FF8A12D2BD73CB491D02FB84B93CBCF71C95C1238B2D83319C14` |

固定 `authority=EXTERNAL_CANDIDATE_ONLY`、`promotionEligible=false`。四个 signature 在当前 PE `.text` 的精确命中基数为：

| 外部名 | 命中 | 当前 PE 命中 VA | disposition |
|---|---:|---|---|
| `GameStatePlayer::Init` | 1 | `0x009CF050` | `RELOCATED` |
| `Entity_Player::StoreGameState` | 3 | `0x00423CC0,0x00466600,0x0090ECA0` | `NO_USABLE_SEED` |
| `Entity_Player::RestoreGameState` | 2 | `0x0058D8B0,0x007AE4B0` | `NO_USABLE_SEED` |
| `Entity_Player::RestoreGameState_PostLevelInit` | 0 | — | `NO_USABLE_SEED` |

因此整体 `externalSeedDisposition=NO_USABLE_SEED`。没有用 `0x007AE4B0` 的语义相似性绕过 Restore 多匹配规则，也没有导入 REP 的 `0x5BC` record size 或 `+0x160C/+0x1610` 字段。

## 4. 当前 PE 原生 writer/caller fixed point

当前 PE 仍按 v22/v23/v24 原生证据独立识别：

- record 公式：`objectBase+0x19D7C+index*0x5DC`，`index in 0..7`；tuple 为 record `+0x500/+0x504/+0x508`。
- constructor `0x00754980`、destructor `0x00754D90`、reset/init `0x009CF050`。
- player-to-record assignment `0x007B0BE0`，source 为 owner `+0x1FA4`；grow 由 `sub_6A0D30 -> sub_40CF00` 安装 allocator-derived backing。
- record-to-player assignment `0x007AE4B0` 只读 record tuple；deserialize/resize `0x009D05D0` 经 `sub_732240 -> sub_40CF00 -> sub_41D150`；serialize `0x009D45C0` 只读；selected erase `0x005CBEA0` 只更新 end。
- v23 A/B/C partition 为 `78/73/16`、union `167`、intersection `0`。候选保留全部 direct caller sets 和 native function set identity；扫描未应用 offline/online mode filter。

manual insertion、online-specific caller 与 no-literal/already-offset alias 并非按“不存在”处理：A/B/C universe 显式包含 direct/bound writer edge、relation/already-offset、absolute/address-taken、return、stack liveout 和 split-tail source；全冻结 `.text` caller/source census 不排除 online 路径。

## 5. v24 最小缺口的闭合

v24 的唯一语义状态 `U-001` 合并 `B-016/B-057`，先前在 `0x007B3BE0` 保留 `STACK[EBP-0x298]=RECORD_BASE`。本轮只读 IDA 与冻结 PE 字节重放证明：

1. `0x007B3BC9` 唯一 spill `[EBP-0x298]`；在绑定路径内仅 `0x007B3C0D` 重载一次。
2. `0x007B3BE0` 开始的循环只读 record `[+0x104,+0x124)`；`sub_753630` 是 8 字节叶函数，只读 `[ECX+4]`，且唯一 code xref 为 `0x007B3BE2`。
3. shift 只在 `[record+0x104,record+0x124)` 内移动；`sub_753620` 是 14 字节叶函数，只写 `[ECX]=0,[ECX+4]=1`，调用时 `ECX=record+0x11C`，唯一 code xref 为 `0x007B3C47`。
4. 上述范围严格低于 tuple `[record+0x500,record+0x50C)`。
5. 原先怀疑的 `0x007B3C61 -> 0x005B39D0` 实际为 `ECX=EDI_PLAYER`，唯一显式参数是 `&[EBP-0x2F2]`；record/stack slot 未作为参数。随后 `0x007B3C70 -> 0x005CA470` 同样 `ECX=EDI_PLAYER`。
6. `0x007B3C75` 跳至 `0x007B8998`，record base 不再 live-out；下一 case 会把相同 stack slot 用于无关值。

因此两个 seed 都终止为 `BOUND_LOCAL_SUBRECORD_ACCESS_NON_TUPLE_WRITER`，没有新增 tuple writer，也没有跨调用逃逸。最终 `resolved=167, unresolved=0`。

## 6. owner/backing 与 no-overlap

parent 有 2 个 slot、stride `0x20660`、object bias `4`；每个 object extent 为 `0x20660`。record array 位于 object `+0x19D7C`，count `8`、stride `0x5DC`，最坏 receiver end 仍在 object 内。

所有 non-null tuple begin install 均被 fixed point 分类为 allocator-derived，活动 backing extent 为 `[begin,capacityEnd)`。erase 的 source/destination 都在该 backing 内，且 backing 是 record/manager owner 之外的独立活动分配；所以它不会覆盖 owner object 或 manager identity。source/destination 自身的相邻重叠是受 `memmove` 支持的预期 shift。

## 7. 最终产物、拒绝链与复现

冻结公共 envelope 为 `out/cs-t0/native-join-manager-this-mt-wave-a-candidate-envelope-v1.md`，SHA-256 `4A54A94A72BC3D0FC8F296966B2335B49304A635D13DB95C1ACA78DAF085558A`。它作为 supplemental `.md` binding 写入最终候选；候选不向 TEST 暴露任何 `.py` builder/analyzer binding。

保留的 candidate 拒绝链：

1. `out/cs-t0/native-join-manager-this-mt-r01-save-record-v1.json`
   - SHA-256：`AC77EC50C69F60CDAD66B1D213342FF2D7D514C8376A57AC3A1C7B7607450069`
   - 实体证明已闭合，但尚未采用冻结 envelope 的 `workPackage/candidateReplay/scope/rowCertificates/classification/mutationGate/finalDecision` 序列化，TEST 按接口 fail closed。
2. `out/cs-t0/native-join-manager-this-mt-r01-save-record-v2.json`
   - SHA-256：`3212877E3AC56A7EF7633DEC680D70B235A7461232AB534679E09C8E9DFFB56D`
   - envelope 已完整，但 `producerFixedPoint.writerRvas` 只投影 `WRITER` kind，遗漏 TEST 契约下属于 writer-side union 的 `LIFECYCLE/ALIAS` 四项：`0x003B3BC9,0x003B3C0D,0x005BD150,0x005BEE80`；因此再次 fail closed。
3. 最终重发 `out/cs-t0/native-join-manager-this-mt-r01-save-record-v3.json`
   - schema：`cs-t0.manager-this.mt-r01-save-record.positive-candidate.v1`
   - SHA-256：`EB7A61A2F8DF7370055C7444D348794A841582B4456C240F479F1BF3DF2C7932`
   - 仅修正上述 writer-side union，一共 13 个 RVA；CALLER union 41 项、实体证据、row verdict 均未改变。

最终 builder 入口：

- `out/cs-t0/build-manager-this-mt-r01-save-record-v3.py`
  - SHA-256：`58E22F249ADD027A7F26590D6333D8BFF3B54E2ACEA2C21A2FE8DCBAE9B3B4F7`
- 共用证明实现：`out/cs-t0/build-manager-this-mt-r01-save-record-v1.py`
  - SHA-256：`7855F37354F5E5E7E590FFC0900B1D01D3868AD9B56570B5D3FAFC1CD37AE94E`
- v2 builder 入口：`out/cs-t0/build-manager-this-mt-r01-save-record-v2.py`
  - SHA-256：`5160C7D427498263DEE672CF44360EFFE97F6D32104CFE4D95991A68B9B163FA`

最终成功执行：

```powershell
python out/cs-t0/build-manager-this-mt-r01-save-record-v3.py
python out/cs-t0/build-manager-this-mt-r01-save-record-v3.py --verify-existing
python out/cs-t0/build-manager-this-mt-r01-save-record-v3.py --verify-existing
```

三次均输出：

```text
PASS MT-R01 REVERSE candidate seeds=167 resolved=167 unresolved=0 disposition=STATIC_CONFIRMED_NO_OVERLAP independentAudit=false
```

builder 拒绝了 15 个作者侧 mutation（目标 SHA、RVA 遗漏或重复、外部 authority/promotion/offset import、Store 假唯一、proof category 遗漏、universe 重开、no-wrap、countermodel、REP layout import、row unresolved、classification 与 IDB 修改权限等）。这些是 REVERSE 自检，不替代独立 TEST。

## 8. 权限与未做事项

- 未启动游戏、未打开/读取游戏进程、未附加 debugger、未注入 DLL。
- IDA 仅用于静态只读查询；未修改 IDB。
- REPENTOGON 仓库只读；只使用单次命令级 `safe.directory`，未修改全局 Git 配置。
- 未运行 REPENTOGON `offline_scanner`。
- 未修改生产代码、历史证据、v30、索引或状态文档。
- 未执行独立 TEST 审计，未推进 whole-image raw v2、formal v7 或 manager-this 最终状态。
