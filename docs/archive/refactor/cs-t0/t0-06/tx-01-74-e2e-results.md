# tx-01-74：统一 E2E 本轮结果

日期：2026-10-18，用户时区 Asia/Shanghai。**统一入口已执行一次，整体 FAIL，退出码 1；没有自动重跑。** 报告已完成收尾，T0-06 尚未通过完整运行验收。

## 1. 最终产物与执行范围

- [最终结果](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/result.json)：32 子项为 17 PASS、1 FAIL、14 NOT_COVERED；13 组为 2 PASS、1 FAIL、10 NOT_COVERED。
- [finalization.json](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/finalization.json)：报告完成，result_status=FAIL，不代表验收通过。
- [逐项检查](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/checks.jsonl)、[组汇总](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/cases.jsonl)、[文件目录](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/file-inventory.jsonl)及[操作时间线](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/timeline.jsonl)均保留。

用户明确要求开始 E2E，逐次确认全新游戏进程、普通换房、结束采集、限定观测配对和最终证据收尾。由 `accept-world-record-e2e.ps1` 在同一交互式执行器会话内组织全部适用子项；B 脚本中性，A 和场景由用户手动操作。使用 tx-01-73 已编译的 Controller/Native/Helper，本轮没有重新构建、运行其它测试入口/单元测试或计算校验哈希。

目标：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`。每段均保存 PID、创建时间、前后模块、实际输入和恢复材料：

| 段 | 游戏 PID | controller 原始结果 | 本轮确认的收尾 |
| --- | --- | --- | --- |
| default | 28004 | NOT_OBSERVED，exit=1 | v3、无 world 采集，51 个唯一目标原字节/值恢复 |
| primary | 9900 | CAPTURE_CLOSED，exit=0 | v4、70 个唯一目标恢复，producer closed |
| secondary | 11192 | CAPTURE_CLOSED，exit=0 | v4、70 个唯一目标恢复，producer closed |

default 的旧 controller 还分析更大的历史输入套件，因此原始 NOT_OBSERVED/非零退出保留；W01 子项只判定当前的 v3/51 槽事实，没有改写旧套件为 PASS。用户确认 default 移动/射击正常，也确认两段普通换房正常。

正常停止后 DLL/已发布 trampoline 保持既有常驻策略。没有强制卸载、杀游戏或再次注入同一进程。最后的受控 controller-loss 因缺少独立恢复字节证据，经用户明确选择跳过，没有执行该故障动作。

## 2. W07 失败与实机覆盖

[primary 原始统计](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/captures/primary/capture/world/result.json)：

- 20,178 次 attempt，377 条 capture，18,868 次背压。
- 30,590 条生命周期事件全部消费，produced=consumed，native_error=NONE，callback=0。
- 19 入口的进入/返回数组逐项相同；未触发的入口为零，不能据此声明分支覆盖。

[覆盖汇总](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/primary-coverage.json)共 1,310 条记录：377 capture、933 gap。reason 为 NOT_READY 241、ODD_PLAYER_ONLY 398、PHASE_INCOMPLETE 534、LIFECYCLE 1、NONE 131、UNKNOWN_CONTINUITY 5。

**最长连续完整普通区间只有 2 次**，attempt 14323–14325，中间 1 条明确 odd-only gap 保留、不计数。已按用户确认的规则处理；其它不完整和背压缺号打断区间。未达 10,000，W07 FAIL。没有用旧快照补足、删除字段或扩大 odd 例外。

启用前已存在的 Effect 有 24 个专用字段为 NOT_READY。用户选择在同一次采集中普通换房，之后出现 complete=true 的样本，但持续背压仍打断区间。用户随后确认正常停止 primary，保留失败并继续独立后续用例。

可读类别为 Effect、Grid、Player、Tear；缺少 Familiar、NPC、Pickup、Projectile 可读覆盖。13 个已实现来源状态与 Grid/door 别名子项通过。保留 Player、未知类别、特殊生命周期、完整只读/RNG/原调用次数和耗时证明仍不足。

**背压已经实际观察，具体耗时来源尚未测量。** 本轮未在运行中改代码、改变预算/采样规则或修复后重跑。

## 3. 离线检查及指定双进程观测

同一执行器内共执行 34 个离线生产 CLI 子命令，保留全部 argv、退出码、输出和输入副本。

- W08 PASS：1,310 条真实冻结记录两次 replay，逐记录原值/字段/位串和重编码字节一致，冻结输入逐字节未变；自有副本 u64=9007199254740993 精确保留。
- W09 PASS：只改自有副本一个 position.x 能定位 tick/entity/field；±0 比较归一化，NaN/±Inf 为未知，原位值保留。
- W10 的缺映射、READ_FAILED/NOT_READY、缺字段/源/对象/tick、重复 tick、错误阶段、5 种引用状态、10 类非法载荷拒绝和更早 gap 子项通过。冲突组已验证；所选样本没有第二个唯一可映射且带引用字段的对象，因此“引用指向冲突目标”仍 NOT_COVERED，没有另选样本重跑。

双进程对比仅使用[用户明确确认的配对](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/cross-process/alignment.json)：primary run=1/room=10/tick=13960 ↔ secondary run=1/room=10/tick=5853，A→A、B→B，births 为空。没有按地址、位置或最近帧自动补配。

[比较结果](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/cross-process/comparison/result.json)为 DIFFERENT：12 个确定字段差异、134 个未知区域。首个报告位置 Player/player:00/position.x，左 raw32=4322424c、右 raw32=42a5d913；earlier_coverage_unknown=true。**没有等价模拟时刻及严格 seed/input/tick 对齐证明，不据此判定同步缺陷。** W11 的生产命令子项通过，严格场景依据子项未覆盖。

## 4. 各组结论

| 组 | 本轮结果 | 主要事实或未覆盖内容 |
| --- | --- | --- |
| W01 | NOT_COVERED | 51/70 槽通过；真实 Online/Isaac Sync 隔离及完整 ABI/原调用证明不足 |
| W02 | NOT_COVERED | 保存的序号/阶段/kind/thread 通过；独立时序和全部阻断窗口不足 |
| W03 | NOT_COVERED | 字段/来源/别名通过；缺四类及特殊 Player/未知类场景 |
| W04 | NOT_COVERED | 创建入口/birth 存在；短命/嵌套/复用/退休逐链未完整核对 |
| W05 | NOT_COVERED | 普通换房已执行；Morph/Restore/Replace/run 退休等不足 |
| W06 | NOT_COVERED | FIRST_OBSERVED A 的证据 3/witness 通过；全部失效/撤销/异常路径不足 |
| W07 | FAIL | 连续完整数 2/10,000；只读/RNG/时延证明也不足 |
| W08 | PASS | 完整 replay 复现及宽整数 |
| W09 | PASS | 单字段定位、±0 和非有限值 |
| W10 | NOT_COVERED | 其它离线负向子项通过；冲突目标引用未覆盖 |
| W11 | NOT_COVERED | 指定观测对比成功；严格同 seed/input/时刻及创建映射不足 |
| W12 | NOT_COVERED | 自然背压和 Authority 继续推进有记录；受控预算/读失败/错线程不足 |
| W13 | NOT_COVERED | 正常 70 槽恢复/收敛通过；受控失联由用户选择跳过 |

见[接受的人工说明](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/manual-review-accepted.json)、[配对确认](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/alignment-user-confirmation.json)及时间线。没有把用户确认的“表现正常”升级为完整 ABI/只读证明。

后续需处理背压的实际原因、明确 bootstrap 专用字段覆盖办法，并准备剩余类别/生命周期/独立证据场景。实质修改及新的整体验收另行安排，本轮失败和原始材料不覆盖、不自动重跑。
