# 9：公共报文结构：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../9-wire-format.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节保留原生事实；需求覆盖只说明清单字段已生成计划，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 需求覆盖 |
|---|---|---|---|---|
| [StateCommonHeader](StateCommonHeader.md) | 408 | 0 | 0 | 是 |
| [StateFrameHeader](StateFrameHeader.md) | 409 | 0 | 0 | 是 |
| [StateSectionHeader](StateSectionHeader.md) | 410 | 0 | 0 | 是 |
| [StateRecordHeader](StateRecordHeader.md) | 411 | 0 | 0 | 是 |
| [StateTableHeader](StateTableHeader.md) | 412 | 0 | 0 | 是 |
| [StateMemberEntry](StateMemberEntry.md) | 413 | 0 | 0 | 是 |
| [StateResultGroupHeader](StateResultGroupHeader.md) | 414 | 0 | 0 | 是 |
| [StateDependencyEntry](StateDependencyEntry.md) | 415 | 0 | 0 | 是 |
| [StateContributionEntry](StateContributionEntry.md) | 416 | 0 | 0 | 是 |
| [StateAudioEventHeader](StateAudioEventHeader.md) | 417 | 0 | 0 | 是 |
| [StateAppliedAckHeader](StateAppliedAckHeader.md) | 419 | 0 | 0 | 是 |
| [StateAppliedAckEntry](StateAppliedAckEntry.md) | 420 | 0 | 0 | 是 |
| [StateResyncHeader](StateResyncHeader.md) | 421 | 0 | 0 | 是 |
| [ResourcePath](ResourcePath.md) | 425 | 0 | 0 | 是 |
| [ResourceImageFile](ResourceImageFile.md) | 426 | 0 | 0 | 是 |
| [ResourceBuiltinShader](ResourceBuiltinShader.md) | 427 | 0 | 0 | 是 |
| [ResourceAnimation](ResourceAnimation.md) | 428 | 0 | 0 | 是 |
| [ResourceName](ResourceName.md) | 429 | 0 | 0 | 是 |
| [ResourceAnimationLayer](ResourceAnimationLayer.md) | 430 | 0 | 0 | 是 |
| [ResourceItemConfig](ResourceItemConfig.md) | 431 | 0 | 0 | 是 |
| [ResourceEntityConfig](ResourceEntityConfig.md) | 432 | 0 | 0 | 是 |
| [ResourceAudio](ResourceAudio.md) | 433 | 0 | 0 | 是 |
| [StateEventQueueHeader](StateEventQueueHeader.md) | 446 | 0 | 0 | 是 |
| [StateEventHeader](StateEventHeader.md) | 447 | 0 | 0 | 是 |

[公共报文编号、枚举与预算](protocol-constants.md)。

[各域正文数字编解码计划](body-codec-plan.md)。

[显示封装数字合同](presentation-envelope-plan.md)。

[资源协议数字合同](resource-protocol-plan.md)。

[完整消息与接受数字合同](state-protocol-plan.md)。

[完整消息静态预算](budget-plan.md)。
