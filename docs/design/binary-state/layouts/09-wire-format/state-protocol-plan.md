# 完整消息与接受数字合同

布局版本 89；同源生成，不是测试报告。

| 头 | 布局 | 字节 |
|---|---|---|
| StateCommonHeader | 408 | 50 |
| StateFrameHeader | 409 | 2 |
| StateSectionHeader | 410 | 52 |
| StateRecordHeader | 411 | 12 |
| StateTableHeader | 412 | 10 |
| StateMemberEntry | 413 | 33 |
| StateResultGroupHeader | 414 | 42 |
| StateDependencyEntry | 415 | 32 |
| StateContributionEntry | 416 | 16 |
| StateAudioEventHeader | 417 | 40 |
| StateAppliedAckHeader | 419 | 2 |
| StateAppliedAckEntry | 420 | 21 |
| StateResyncHeader | 421 | 11 |

接受、ACK丢失、不可变目标与污染恢复见[第五项合同](../../9-acceptance-recovery.md)。字节保护不替代10Mbps／首次准备预算。
