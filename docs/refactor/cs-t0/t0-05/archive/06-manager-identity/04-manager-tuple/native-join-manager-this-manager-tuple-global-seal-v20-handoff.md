> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple global writer seal 与 `0x005B91EF` exact-one 交接

## 结论

本轮以只读 IDA/PE 与冻结 JSON 聚合完成 `GameRoot+0x1BAA8` manager
Player-pointer tuple 的全局 writer/backing 闭包，并只升级一条 copy row：
`0x005B91EF`。闭包采用纠正后的 **7 个 field-writer entries + 2 个 support
writers**，不是旧的 6+2；增长调用点精确为三个：
`0x009B901A`、`0x009B99B2`、`0x009B9EBA`。

`0x005B91EF` 复用 v7 的 `VERIFIED_BOUNDED` 相对范围结论，在 writer 与
backing 闭包完成后满足：

```text
backingPointerWriterClosureComplete = true
fixedImageSlotAliasExcluded          = true
compositeRuleSatisfied               = true
```

`0x001CBEB5` 属于不同的 save-record tuple，严格保留 `UNKNOWN`。因此 copy/move
账本只允许 `314 -> 315 / 405`，remaining 为 90。本轮是 exact-one 静态证据升级，
不允许 manager-this、T0-05 或运行时权限提升；machine JSON 中所有 promotion/
permission 均为 false，`overallAccepted=false`。

## 唯一接受链

全局 seal 只绑定下列接受证据：

- writers-v11：`D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- helpers-a-v13：`322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- helpers-b-v14：`A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`
- helpers-c-v15：`B07501A2D6DD6492E6834ECC64A5B23FF3B125DB415643B87F04DE4FB53EDCAD`
- liveouts A successor-first v19：`ED54DA25E98F599A1AC84BFFF1D4EF9861D53522D203715641A3A4588A5750A4`
- liveouts B v17-switchfix：`B0BC798C723DC8D4C539C9793391019BA6021B85DDA9F6BE0E98E6D4AC3952B3`
- liveouts C corrected v18：`246CF36EB86BEA22591CCBB5CF506060F63CCAB4DCE7BD470BBF43924143FD44`
- copy-backing-critical-two-v10：`0A5834505E79B4575A2DF53C86FE647E3F6A83E3E8FD4328369C11CC257A65FF`
- copy-range-bounds-v7：`C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`
- corrected whole-image indirect-split-component-v2：`D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`

失败的 helpers-a-v12、旧 liveouts-a-v16 和旧 liveouts-b-v17 均只出现在
`rejectedOrSupersededConclusions`，`acceptedAsEvidence=false`，不参与 binding。
C v18 内部历史 A 结论也不被消费：全局只消费 C 的 exact-16 source rows 与
corrected local C 结果，并单独绑定 A v19；
`cV18HistoricalAConclusionConsumed=false`。

## 集合代数与 live-out 闭包

helper 三组 target 数为 `12+12+12=36`，source 数为
`114+127+41=282`；target/source 组分别两两互斥，union 分别精确等于 v11
原始 36/282 universe。

live-out 分区为 A40、B32、C16，三组两两互斥，union 精确等于 v11 原始 88。
三组的 tuple writer、unresolved 与 new writer 均为 0。A 绑定 successor-first
修正规则。B switchfix 新增的两个 source 精确落到 v13 已接受的 read-only helper：

```text
0x007FBA95 -> 0x009B92C0  READ_ONLY_MANAGER_TUPLE
0x00956408 -> 0x009B9310  READ_ONLY_MANAGER_TUPLE
```

它们不增加 helper target 或 tuple writer。

## 7+2 writer universe 与 begin install 分类

v11 在 helper frontier 尚未解析时报告六个 primary entry writers；v13 随后把
原 unresolved helper `0x009B98E0` 提升为真实 tuple writer。因此全局 entry
writer 数必须为 7，不能退回 6：

```text
0x006F1020  constructor: begin/end/capacityEnd = NULL
0x009B8EA0  via 0x004149D0: free prior backing, then zero triple
0x009B8F40  in-place end-only, or growth at 0x009B901A
0x009B9150  erase end-only; begin/capacityEnd unchanged
0x009B98E0  in-place end-only, or growth at 0x009B99B2
0x009B9CD0  in-place end-only, or growth at 0x009B9EBA
0x009BA000  erase end-only; distinct nested +D0 vector is not this tuple
```

两个 support writers 为 `0x0042C920` 与 `0x0041D150`。`0x004149D0` 是
`0x009B8EA0` 下折叠的传递 clear writer，不另计 entry/support。nested `+D0`
writer、backing-element writer 与 fixed-member writer 均不得算作 tuple field
writer。

全局非 NULL begin 只有 allocator 路径：

```text
0x0042C920 -> 0x0040CF00 -> 0x00A0F4C0 -> 0x00A648B0 -> malloc
                                  |
                                  +-> 0x0041D150 installs triple
```

未发现也不允许外推 inline/SSO 或 external-buffer begin install。

## 32 位算术、allocation extent 与 fixed slot 排除

`sub_42C920` 拒绝 `oldSize==0x3FFFFFFF`，成功路径满足：

```text
0 <= newCapacity <= 0x3FFFFFFF
newSize <= newCapacity
4 * newCapacity <= 0xFFFFFFFC
```

所以 element-to-byte 的 32 位无符号乘法不回绕。`sub_40CF00` 的大块路径还会
在 aligned allocation 前拒绝 `size+35` 的无符号溢出。`sub_41D150` 精确安装：

```text
begin       = allocation
end         = allocation + 4*size
capacityEnd = allocation + 4*capacity
```

因此成功 allocation extent 覆盖完整已安装范围。非 NULL begin 是 malloc-backed
heap storage；固定槽 `0x00C71678` 则位于已加载映像的 writable/non-executable
`.data`。heap allocation 不与 loaded-image mapping 重叠，NULL 也不别名该槽，
故 `fixedImageSlotAliasExcluded=true`。

对应函数字节绑定：

- `0x0042C920`, size 221：`46443AD5AD72AD526C44AD510A44A41B556DFBA692DBEEBF39BFF42D3D5B1FB2`
- `0x0041D150`, size 90：`788A11BE22A589803AFB3566B37F7D5B5F631338D0CE7CB342D236AC35EEEC85`
- `0x0040CF00`, size 72：`4DE16F4A533E47963F681B4F6330B27CA3B80DF4E8B7C3FA4A4B5E018F507031`
- `0x00A0F4C0`, size 27：`50DC753EB18D0CB132526A8B1F95B5AE9400EA71773F0DDAEB8FB6E935ED4A79`
- `0x00A648B0`, size 177：`3EB869CF2288BF57525B1AC59FF8110F73802FD010A9C39068C951530F725FC3`

## `0x005B91EF` superseding exact-one row

该行仍复用 v7 的相对范围事实，不重新证明或扩大 v7 范围：loop guard 给出
`usedBytes>=4`，copy count 为 `usedBytes-4`，故 `count<=usedBytes`。在本轮 writer/
backing 完整闭包之后，row 才设置 `backingPointerWriterClosureComplete=true`、
`fixedImageSlotAliasExcluded=true` 与 `compositeRuleSatisfied=true`。

`0x001CBEB5` 的 save-record writer universe 不在本闭包范围，machine row 明确保留：

```text
status                              = UNKNOWN
backingPointerWriterClosureComplete = false
fixedImageSlotAliasExcluded         = false
compositeRuleSatisfied               = false
```

## create-new、连续双 verify 与 mutation gate

在冻结 IDA 中执行 analyzer，先 create-new，再不编辑 analyzer/JSON 连续复跑两次：

```text
PASS global=created row=created helpers=36/282 liveouts=88 writers=7+2 growth=3 copy_closed=315/405 retained_unknown=0x001CBEB5 mutations=44+28
PASS global=verified row=verified helpers=36/282 liveouts=88 writers=7+2 growth=3 copy_closed=315/405 retained_unknown=0x001CBEB5 mutations=44+28
PASS global=verified row=verified helpers=36/282 liveouts=88 writers=7+2 growth=3 copy_closed=315/405 retained_unknown=0x001CBEB5 mutations=44+28
```

44 个 global mutation 覆盖集合缺项/重叠、旧结论误消费、entry 降回 6、漏掉
helper writer、漏 `0x009B99B2`、新增 source 映射漂移、writer/support 分类、算术/
extent、fixed slot、`0x001CBEB5` 非法提升、copy 计数、权限与总体 promotion。
28 个 row mutation 覆盖 exact-one、range/backing/composite、7+2、growth exact-three、
保留 UNKNOWN、账本最多 +1、所有 permission/promotion 与全局 binding，全部 fail
closed。

## 产物

- analyzer：`out/cs-t0/build-native-join-manager-this-manager-tuple-global-seal-v20.py`
  - SHA-256 `0A91926ABACB02C0DCA62F6546B1C9E735DA124D46798DF558917A8754F3F356`
- global seal JSON：`out/cs-t0/native-join-manager-this-manager-tuple-global-seal-v20.json`
  - SHA-256 `E1D42FABAF1651B01BCC1C7005A2958C0510B26F29BE361A2FE8F71AA44D58C9`
- exact-one row JSON：`out/cs-t0/native-join-manager-this-copy-backing-5b91ef-v20.json`
  - SHA-256 `17792FD0138A8CDADC1D9950C2C018B9C238F88DD20E3E8C85AD8FE8602E3E0F`
- 本 handoff：
  `docs/refactor/cs-t0/t0-05/native-join-manager-this-manager-tuple-global-seal-v20-handoff.md`

## 执行边界与独立测试交接

全程只读取冻结 JSON、磁盘 PE 和已打开 IDA 的反汇编/字节/函数范围；没有启动
或读取游戏进程，没有附加调试器，没有修改 IDB、生产代码、测试、旧证据、索引
或状态文档，也没有 runtime mutation。

本文件是逆向交接，不是最终测试验收。独立测试智能体应重算三份产物 SHA，
在冻结 IDA 上复跑 analyzer，核对接受链、36/282、A40+B32+C16=88、B 的两个
read-only source、successor-first、7+2、三 growth、32 位算术与 allocation extent、
fixed slot 排除、exact-one `0x005B91EF`、`0x001CBEB5` 保持 UNKNOWN、44+28
mutation，以及所有 permission/promotion 为 false。
