# tx-01-79：跟班创建与独立故障取证结果

日期：2026-10-19（Asia/Shanghai，artifact 内时间为 UTC）。用户确认实质修正后执行的一次统一验收已完成，**INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**。18 项 PASS 为历史沿用，3 项为本轮实际检查：重新确认 W01.optional_70，新增完成 W03.eight_classes、W13.controller_loss。本轮净补齐两个剩余子项；W12 得到部分证据，未改成整项通过。

控制器仅增加 IPC 取证元数据，直接纯编译成功，Native/Helper 沿用冻结产物。没有单元测试、全量 replay、已完成离线用例重跑或哈希操作。外部观察器只读游戏和 mapping，不写游戏字段、共享 consumed/state/stop。

## 正常段与新跟班

primary game PID=41328、controller PID=20796。用户在采集开始后手动拾取道具，并确认新跟班出现、游戏正常。Familiar 首个全字段 VALUE 样本为 tick=1956、local_id=64、birth_event=11、init_evidence=2，有相同 run/local_id/use/birth 的 Spawn 出生事件，没有改变 FIRST_OBSERVED 读取规则。

本轮 primary 序列本身已观察到 Player、NPC、Tear、Projectile、Pickup、Familiar、Effect、Grid 八类完整字段 VALUE，W03.eight_classes PASS。类别字段可读不等于全世界完整可比较：NOT_READY 和 UNKNOWN_CONTINUITY 仍如实保留，未放宽全 profile 资格；上轮 13,567 次连续完整采集仅按历史来源沿用。

用户完成暂停/恢复、普通门换房和返回主菜单。operations.jsonl 中暂停前与确认暂停时 tick 均为 2655，恢复后 8907，换房前 8907/后 10244，菜单前 13061/确认主菜单与 stop 为 14070。这些材料不代替完整 loading、Morph/Restore/Replace、复用和撤销矩阵。

primary 正常退出 0、CAPTURE_CLOSED。控制器报告和独立外部读取均确认 **70 槽原字节恢复**，W01.optional_70 本轮重新检查 PASS。

## 消费暂停与背压

只暂停自有控制器内唯一 world-reader，TID=3368，持续 1000ms。原暂停计数 0，finally 中 ResumeThread 返回此前计数 1。

| 独立观测 | 暂停前 | 仍暂停时 | 恢复后 |
| --- | --- | --- | --- |
| Authority tick | 10245 | 10305 | 10336 |
| Native 背压 | 0 | 57 | 57 |

恢复后 coop/world 错误均为 0，用户确认游戏正常。这证明本次 consumer 暂停期间 producer 明确记录背压、scheduler 继续推进；没有暂停游戏线程。

primary attempts=22,631、持久化记录=22,574、capture=7,006、gap=15,568；57 个背压 attempt 缺口没有冒充成功采集。odd-only=7,007、阶段不完整=8,551、生命周期 gap=10；capture 中 NOT_READY=4,656、UNKNOWN_CONTINUITY=2,350。字段/记录/生命周期事件序号检查无失败。

批次 copied/written 均 22,574，事件 produced/consumed/copied/written 均 5,992。队列峰值 262,800 bytes/19 items，budget_waits=0，最终 flush=true、outstanding=0、consumer failure=null。

W12.fault_isolation 仍 NOT_COVERED：本次只覆盖消费暂停、背压与推进，没有覆盖队列耗尽、对象/关系预算、原生读失败和错误线程等所有分支。

## 控制器失联

用户完全重启为 game PID=7276、controller PID=46544。独立观察器保存注入前 70 槽值，确认安装后 70 槽均处于补丁状态；有实际活样本后只终止本执行器的 Popen 控制器一次（退出码 1）。没有终止游戏、额外 helper 补救、强制卸载或重注入。

- 恢复后 70 个目标逐字节等于本次进程的注入前值；70 条恢复事件的 RVA/宽度/原值/恢复值与外部基线一致。
- coop state=4、restored=1、pinned=1、callbacks=0、overwritten=0；error=7 为预期 Publication 失联错误，未改写为 0。
- world enabled=0、closed=1、callbacks=0、error=0，19 入口进入/返回计数收敛，两个关闭后观测点的发布/尝试/采集计数稳定。
- Native 仍驻留；用户确认失联后游戏正常。
- 两个未消费槽按 metadata/objects/memberships 保存有界原始副本。**被终止控制器的文件最终 flush 与尾部完整性未确认**，没有伪造 consumer 最终报告或宣称所有 co-op 事件已消费。

W13.controller_loss PASS，范围为此次真实受控失联与独立恢复。

## 十二项未覆盖

| 子项 | 仍缺少的范围 |
| --- | --- |
| W01.isolation | 完整 Original/Isaac Sync/Lua API/输入兼容运行证据 |
| W01.native_results | 完整 ABI、栈、返回寄存器和 exactly-once 跟踪 |
| W02.phase_order | 完整 owner/Room/tail 与精确提交的独立分析 |
| W02.blocked_windows | 普通暂停/换房之外的完整 loading 与阻断窗口对齐 |
| W03.retained_unknown | 保留 Player、关系歧义和未知类别场景 |
| W04.identity_history | 嵌套、短命、同址复用及完整成员历史 |
| W05.transitions | Morph/Restore/Replace 与完整退休矩阵 |
| W06.revocation | exact-pair/generation 读取资格撤销与恢复的完整取证 |
| W07.readonly_calls | collector 只读/RNG/线程/原调用/耗时的完整运行证明 |
| W08.replay | 用户主动排除的全量两次重编码；离线输出性能未改 |
| W11.scenario_basis | 严格同 seed/input/模拟时刻/创建映射 |
| W12.fault_isolation | 消费暂停之外的资源、读取及其它观察故障 |

用户选择按现有证据收尾。排除全量 replay 后，尚余 **11 项**未完整覆盖；完整 T0-06 尚未验收完成。

## 失败历史与复查入口

[tx-01-77](tx-01-77-e2e-results.md) 未识别唯一主模块，底层 Toolhelp 差异因缺实际枚举未能确定；[tx-01-78](tx-01-78-e2e-results.md) 已识别主模块，但错误排除三处 USER32 IAT。两次都发生在创建控制器子进程之前，未注入或安装 Hook。用户分别确认实质修正后的新整体验收，没有自动重跑；原 FAIL 不修改，两份报告均复制至本轮 prior-evidence/。

产物根目录：[tx-01-79-live-remaining-01](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/)。

- [最终结果](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/result.json)、[收尾记录](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/finalization.json)、file-inventory.jsonl。
- continuation.json、build/ 的直接纯编译命令/日志、binaries/ 和 sources/ 副本、人工输入及完整场景标记。
- [八类原始位置和出生证据](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/primary-coverage.json)、primary-record-index.jsonl、captures/primary/capture/world/ 原始记录。
- [reader 暂停时序](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/captures/primary/external/reader-pause-outcome.json)、外部正常恢复字节与 Native restoration。
- [失联独立结果](../../../../artifacts/cs-t0/tx-01-79-live-remaining-01/captures/controller-loss/external/controller-loss-outcome.json)、before-injection/installed-before-loss/after-controller-loss、稳定头、post-loss-events 原字节与 unconsumed-*-*.bin。
- user-review-evidence.json、manual-review-accepted.json 保存用户确认和未覆盖理由。

执行器 result=INCOMPLETE、error=null、finalization=COMPLETED。外层 PTY 返回 1，PowerShell 已明确说明存在未覆盖项；失联控制器的退出 1 则是独立的受控终止记录，不能混同为应用失败或全套 PASS。
