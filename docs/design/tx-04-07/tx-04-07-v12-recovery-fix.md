# tx-04-07 V12 功能与实时采证修复

> e2e-11 已实际覆盖并判定 V12 PASS：提前B激光0，计划松键 tick648 与新B激光的实时 witness 对齐；两条真实中性空档由逐 tick Shooting 暂停证据验证。整体仍因 S17 净空选择停止而 FAIL，见[本次证据](../../artifacts/repentogon/tx-04-07-e2e-11/README.md)。原中性事实保留，不外推到期分支或所有武器，未自动重跑。

> e2e-11 已实际覆盖并判定 V12 PASS：提前B激光0，计划松键 tick648 与新B激光的实时 witness 对齐；两条真实中性空档由逐 tick Shooting 暂停证据验证。整体仍因 S17 净空选择停止而 FAIL，见[本次证据](../../artifacts/repentogon/tx-04-07-e2e-11/README.md)。原中性事实保留，不外推到期分支或所有武器，未自动重跑。

> 唯一整体 E2E e2e-10 已结束：S9 换房后 B 的 movement_options 为空，方向 helper 报 no_observed_free_direction，S15/V12 尚未运行。前段已观察到新 witness 和恢复暂停分支，但完整 V12 修复未实机验证。游戏正常退出、环境恢复，未自动重跑，见[原始结果与说明](../../artifacts/repentogon/tx-04-07-e2e-10/README.md)。

> 唯一整体 E2E e2e-10 已结束：S9 换房后 B 的 movement_options 为空，方向 helper 报 no_observed_free_direction，S15/V12 尚未运行。前段已观察到新 witness 和恢复暂停分支，但完整 V12 修复未实机验证。游戏正常退出、环境恢复，未自动重跑，见[原始结果与说明](../../artifacts/repentogon/tx-04-07-e2e-10/README.md)。

日期：2026-10-26，Asia/Shanghai。用户要求先查 V12 不通过原因、修复并重跑。修改期间仅阅读现有源码和历史产物、做可逆编辑与纯构建；不执行局部测试、单元测试或旧记录重归约。随后经本次准备确认执行一次完整入口，产物 [e2e-10](../../artifacts/repentogon/tx-04-07-e2e-10/README.md)。原 e2e-06/08/09 的报告与失败保留。

## 两个已确认原因

1. 功能空档：唯一重绑发生于 Authority-pre，新代次 Ack 原先在该 tick 的 TopExit 才公布。远端只有收到新 Ack 才能合法清历史、采集新源并发送新代次命令，所以首 tick 没有输入；Shooting 正常执行中性消费，把握手空档当松键。e2e-09 tick=439、loop=4572、sequence=0 与早期激光相邻。提高普通 lead 不能消除该先后关系。
2. 采证偏移：Lua 每次执行 cs status，仍拿到 FrameEnd 发布的 status_snapshot，模拟内回调通常落后一轮。因此松键 tick 内的正常激光可能被归到前一 loop，造成 before_release=2/after_release=0 的误分类。该误分类不能否定更早的真实提前发射。

## 用户采纳的恢复规则

现行规范原先要求正常缺输入一律中性消费，不能未经决定暂停 Shooting。用户本次明确选择“采用最多 6 tick 的恢复窗口”；已在 [现行规范第 9 节](../refactor/project-conventions.md)补充这项例外，不追溯修改历史验收。

唯一绑定后的首个有效 Authority tick 起，Shooting 在首条可消费的新代次命令之前最多暂停六 tick。Authority、其他 consumer 与 native Player 更新继续，canonical 仍中性，不等待全员、不复制旧 held、不写蓄力。首条新命令用于 current（包括真实中性/松键）立即结束窗口，本代次永久退出；六 tick 上限到期也恢复中性消费。普通后续丢包仍消费中性。consumer 缺席不延长窗口，未绑定期间不提前消耗窗口，A/B 同规则。

`ShootingRecovery` 只持 generation/first_tick/source_seen。BeginTick 在 Resolve 后按绑定代次管理它，ConsumerScope 在原先具备调用资格的路径检查窗口，只主动暂停 Shooting。trace 增加 startup_deferred、recovery_first_tick/limit、input_sequence/source_seen；tick 增加 shooting_startup_deferred。没有新增 Hook、网络字段、输入旁路或游戏字段写入。

## 实时 witness 与判定

新增主线程只读 `cs witness`，返回当前 Manager frame、SimulationTick、session/A/B generation、blocks、owner/token。它直接复制当前边界，status/counters 仍保留原帧尾快照语义。tx07 Lua 事件改用 witness，保存完整 witness 与实时 loop；tx05/06 不变。不从 Game frame 或旧快照补造缺失证据。

V12 激光必须带 witness_version=1，实际松键以匹配 Accepted 的新中性命令为准，再按同 session、B generation、SimulationTick 与有限 loop 窗口归属激光。新接口缺失仍不能 PASS。

连续性判定分别记录：

- fresh_held_continuity：原始 canonical 是否始终 held；中性空档存在时仍为 false。
- neutral_startup_gaps：所有原始中性空档，不删除事实。
- verified_startup_pauses：必须由对应完整 tick 与该 tick 每次 shooting_dispatch 共同证明 original=0、startup_deferred=1、sequence=0、first_tick 正确且 age<6，同时本代次此前尚无新命令。
- effective_shooting_continuity：只有真实 Accepted held，或上述已证明的恢复暂停才成立。首条新命令后的空档、真实松键、到期或未暂停的空档仍失败。

依然要求普通松键正对照、两段实际时长、阻断原调用为零、跨房到计划松键前无 B 激光、松键后确有新 B 激光。没有将“调用了 Hook”“提交过输入”或“编排完成”代替世界证据。

## 构建和验收边界

统一纯构建 DLL 与 i686 Rust harness 成功，日志与相对 e2e-09 受测源码的差异见 [修复产物](../../artifacts/repentogon/tx-04-07-v12-recovery-fix/README.md)。修改期间未部署、运行 harness 或游戏；整体验收仅执行一次 e2e-10。S21 暂停导航、V10 旧 kind 筛选等不属于本次修复，不为通过 V12 顺带改变它们，也不放宽网络/持久判定。
