> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 普通 Player 初始化证据更正版 v2

- 日期：2026-08-31；仅修正 padding 指令元数据，create-new。
- 整体仍为 **BLOCKED_BY_EVIDENCE**。本版不扩大逆向范围，不改变11项局部分支条件证明、4项missingEvidence及其静态/实现义务分类。
- active/live/unlock/arm/claim/injection/inputOverride仍全部为false；没有新增运行时观察或独立验收结论。
- 原 [JSON v1](../../../../../cs-t0/coop-native-join-init-closure-v1.json)、[报告 v1](native-join-init-closure-v1.md) 和失败审计均保持原始字节。旧v1的独立复核失败不被改判。

## 1. 版本与失败证据绑定

| 对象 | SHA-256 |
| --- | --- |
| JSON v1 | `B5C5D48337F8B066FACBD0DE665254DB785B67EB8C197EA388A981FD6EC7BC94` |
| 报告 v1 | `7DC6B13FFF811B306AA651D5029438597DDA63D67C56056FF633AC343BDE6F39` |
| 独立失败审计 attempt-v4 | `313AF378783B3C5FA556FC3980D099F8092B6E7C53769F82B8087E208D270603` |
| [JSON v2](../../../../../cs-t0/coop-native-join-init-closure-v2.json) | `676A576A1E3ED588B75408C36B9EC3D9B686CC5D80B29434E337E4B21D1BE62C` |

失败审计是 [native-join-init-anchor-audit-attempt-v4.json](../../../../../out/cs-t0/native-join-init-anchor-audit-attempt-v4.json)，状态 `REJECTED_STATIC_ANCHOR_INPUT`。它记录22条空mnemonic，位于5个anchor；早期沟通中的“6个anchor”是计数口误，以该审计完整清单为准。

独立测试报告的其余诊断包括：83个窗口磁盘原始字节相符，251个direct edge及5个常量memoryReference相符，没有其他声明大小/边界/非空助记符差异。这里仅引用已冻结失败审计，不将这些分项诊断升级为v1整体通过，更不预判v2验收。

JSON v2保持schemaVersion=1，identity升为 `IsaacCsT0-CoopNativeJoinInitClosure-v2`；保留v1全部原predecessorBindings，追加绑定上表的JSON v1、报告v1、失败审计。机器JSON包含逐行修订表和修订原因。

## 2. 缺陷原因与精确修订

v1用 `ida_ua.decode_insn` 得到有效指令大小，但用 `idc.print_insn_mnem` 填写助记符。对这些未被IDA定义为指令的alignment字节，后一个文本接口返回空字符串，导致原始字节和边界正确、声明元数据却无效。

本版只读调用 `ida_ua.decode_insn` 和 `insn.get_canon_mnem`，并核对冻结窗口对应的实际opcode：

| anchor | 指令RVA | 数量 | 原始字节 | v1 mnemonic → v2 |
| --- | --- | ---: | --- | --- |
| tail-low-counter-gate | `0x003BE1CF` | 1 | `90` | 空 → `nop` |
| mode-config-lookup | `0x00306999` 至 `0x0030699F`，逐byte | 7 | 各为 `CC` | 空 → `int3` |
| scale-min-helper | `0x0000C78D` 至 `0x0000C78F`，逐byte | 3 | 各为 `CC` | 空 → `int3` |
| coin-heart-computation | `0x003CE416` 至 `0x003CE41F`，逐byte | 10 | 各为 `CC` | 空 → `int3` |
| effects-count-scan | `0x005309BF` | 1 | `CC` | 空 → `int3` |

每条size仍为1。IDA canonical API将单字节 `CC` 返回为 `int` 指令族；v2依据**已解码的单字节CC编码**明确写为 `int3`，不是把任意空mnemonic当成nop。独立验收的非空助记符要求保持不变。

原83个anchor窗口、1154条instruction rows、expectedBytes、RVA、大小、decode context、251个edge、5个memoryReference及float数据全部保留。没有删掉padding或裁剪窗口来绕过缺陷，也没有改变目标PE中的任何字节。`INT3`在此仅是静态padding的原始编码描述，不表示曾安装或运行断点。

## 3. 技术结论和权限边界保持不变

完整初始化分析继续以报告v1的技术叙述和本版机器JSON的条件/字段/锚点为依据。局部条件仍为 `STATIC_CONDITION_SUPPORTED` / `CONDITIONAL_READY`，不等于B状态已在运行时建立；共同native readonly可排除已证achievement setter，但不能代替direct write和窗口稳定性证明。

剩余两项静态数据流义务仍是：

1. `INIT_EFFECTIVE_STATE_PROPAGATION_NOT_FROZEN`：fresh B的constructor/Init → pre-cache链，连接有效count和desired-familiar的前提；不能用创建后读回补事前证明。
2. `INIT_PENDING_LIFECYCLE_WRITE_EXCLUSION_NOT_FROZEN`：与persistence-window证据组合，证明选定初始化不调度pending lifecycle、不清readonly。

另两项 `PREJOIN_FIELD_READER_BINDING` 和 `PREJOIN_GENERATION_AND_FAILURE_POLICY` 仍分别归于实现合同及未来运行验证。没有因这次元数据修订新增caller树、改写验收条件、删除缺口或升级active资格。

目标仍为x86，base `0x00400000`、size `0x008FE000`；canonical SHA `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`，approved-unpacked SHA `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。本次未启动/附着/操控游戏、未枚举或打开目标进程、未修改IDB、未运行测试入口。

v2须由原独立测试实例重新复核；逆向实例不自行宣布通过。55槽driver、原static/reverse/followup v1、plan v7和生产preflight的原资格边界均保持历史原义。

