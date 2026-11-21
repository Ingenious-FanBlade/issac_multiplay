# tx-01-83：剩余 E2E 的统一运行结果

日期：2026-10-19（Asia/Shanghai），17:32:23 至 18:25:08。一次宿主统一入口已执行结束，**整体 FAIL，报告完成**。账本为 **25 PASS / 1 FAIL / 7 NOT_COVERED**；十个剩余目标中 **4 PASS / 1 FAIL / 5 NOT_COVERED**。T0-06 尚未完整验收。

原始目录：[tx-01-83-instrumented-e2e-01](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/)。没有自动重跑、故障重发、同进程重注入、强制卸载或由智能体启动/终止游戏；用户手动准备八个全新游戏进程并操作场景。全量 replay、真实 Online/Isaac Sync 隔离继续按此前选择排除，保留 NOT_COVERED。

## 结果与出处

- [result.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/result.json)：33 个子项及逐项证据；20 个 PASS 标为 CARRIED_FORWARD，1 个可选采集/70 槽 PASS 本轮重新检查，4 个剩余目标新增 PASS。不能称为本轮重新通过 25 项。
- [authorized-scope-result.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/authorized-scope-result.json)：十个剩余目标的最终判定为 INCOMPLETE。
- [completed-evidence.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/completed-evidence.json)、[remaining-review-evidence.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/remaining-review-evidence.json)：八段原始退出、世界/探针状态、恢复及九条故障命令的实际观察。
- [manual-review-accepted.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/manual-review-accepted.json)：主智能体结合用户场景观察和原始材料完成的人工审阅；未将缺口升级为通过。
- [finalization.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/finalization.json)：COMPLETED、result_status=FAIL；[finalization-diagnostics.json](../../../../../artifacts/cs-t0/tx-01-83-instrumented-e2e-01/finalization-diagnostics.json) 为 errors=[]、evidence_summary_completed=true。

tx-01-80/81/82 的历史失败保留。本轮没有安装失败，不能将成功安装外推为 tx-01-82 的 Hook 事务失败已修复，也没有触发新诊断的安装失败/异常 finally 路径。

## 使用实现与复查入口

使用 tx-01-83 已纯编译的 controller、tx-01-80 最终 WorldE2E Native 和 tx-01-79 Helper；没有在验收中修改、重编译 Native/controller/Mod/协议。原始路径、大小和时间见 binary-provenance.json，不计算或校验哈希。

统一入口仍为 `cs-t0/accept-world-record-e2e.ps1 -InstrumentedRemaining -SkipFullReplay -ContinueFrom artifacts/cs-t0/tx-01-79-live-remaining-01`；授权、游戏路径、原 plan、日志路径和运行环境见 manifest.json/continuation.json。binaries/、sources/、inputs/ 保存本轮副本；每段 controller-invocation.json 和 commands/ 下 invocation.json/exit.json 保存实际参数和退出码。fixture 的实际逐条操作、seed/frame/input 记录在各 captures/<role>/fixture/；operator-actions.jsonl、timeline.jsonl、scene-observations.jsonl 保留人工步骤和独立头部观察。file-inventory.jsonl 为本轮产物清单。

复查先读取上述已落盘结果和原始文件，不将再次运行分析器、游戏或验收作为复查前提。以后有实质修改时，按现行规范另行说明推进范围、使用新目录执行其唯一整体验收，不覆盖或重新解释本次 FAIL。

## 八段实机采集与恢复

| 阶段 | 原始 controller exit / capture 状态 | 世界状态 | 恢复 |
| --- | --- | --- | --- |
| primary | 0 / CAPTURE_CLOSED | NONE，19,924 次普通采集，背压 0 | 原生及外部 70 槽 |
| align-left | 0 / CAPTURE_CLOSED | NONE，实际 frame 6000 | 原生及外部 70 槽 |
| align-right | 0 / CAPTURE_CLOSED | NONE，实际 frame 6000 | 原生及外部 70 槽 |
| field-read | 1 / FAIL | READ_FAILED（4） | 原生及外部 70 槽 |
| source-changed | 1 / FAIL | SOURCE_CHANGED（17） | 原生及外部 70 槽 |
| event-overflow | 1 / FAIL | EVENT_OVERFLOW（13），原命令迟到触发 | 原生及外部 70 槽 |
| wrong-thread | 1 / FAIL | WRONG_THREAD（9） | 原生及外部 70 槽 |
| root-changed | 1 / FAIL | ROOT_CHANGED（12） | 原生及外部 70 槽 |

八段 coop nativeError 均为 0；探针均 closed=true、error=0、overflow=0、produced=consumed。终止观察故障的 capture FAIL / exit=1 和 world CAPTURE_ERROR 是原始结果，不改写成健康采集通过。DLL 和自有设备按既有策略常驻；没有强制卸载。

wrong-thread 初次 focus_game 返回 FOREGROUND_DENIED。当时 tick=0/captures=0，故障未发送；记录在 timeline 后由用户手动切回游戏。取得健康样本后才发送唯一故障命令，没有自动重试前台操作或注入。

## primary 实际观察

实际 fixture 采用此前已确认的 `lua Isaac.RunCallback(22, "cswe2e", "<action>")` 分派。enable、scene、Grid 创建/移除/再创建、Esau Jr. 三次使用、Hour Glass equip/use 均有本轮日志；用户另执行暂停恢复、普通门换房及返回主菜单。

- 普通采集 **19,924/19,924** 均有同 token owner/Room/tail/commit/TopExit 顺序依据，trace_integrity=true、errors=[]；W02.phase_order 新增 PASS。
- A 保留 FIRST_OBSERVED，读取资格出现 0→3→0→3，W06.revocation 新增 PASS。
- 只读复制窗口 19,924 个，字面读取 44,365,281 字节；938,481 条对象 RNG、19,924 个全局 RNG 窗口未报告变化或不可读；未记录复制期间的 native 入口，复制耗时 **0.0432–0.3345 ms**。结合已保存的只读调用链，W07.readonly_calls 在实际观察范围内 PASS；不外推任意 Mod、别的 RNG 或未观察线程。
- 嵌套 Spawn 实际触发；64 组同址复用、697 个成员事件、无重复 birth 记录。fixture 的短命 seed=9730003 对应 Init/Spawn local_id=50/use=1，之后有 ClearReferences，但未取得完整 Remove 对应；重复发布历史也未覆盖。
- NPC/Pickup Morph、ManagerRestore、PlayerRestore、Grid factory/Init/Spawn、Door 和 Destroy 已观察。19 个入口仅覆盖 **17 个**，缺 **Replace（9）和 GridFree（17）**。
- UnknownEntity 已出现，但没有 retained-only Player 证据。Esau Jr. action 成功不等于 Replace/retained 原生链已覆盖。
- 暂停两次独立观察相隔约 12 秒，tick 固定 5795、普通采集数固定 2900，coop/world error=0；原始 odd、生命周期与阶段缺口保留。尚未完成所有 loading/pause/lifecycle 缺口的因果配对，W02.blocked_windows 仍未覆盖。

详见 primary-evidence/evaluation.json、identity-history.json、readiness.json、phase-order.jsonl，以及 captures/primary/capture/world/ 和 fixture/。探针的原始 GPR/FLAGS/FX/读取/RNG 记录完整保留，不以摘要替代。

## 精确帧跨进程比较

用户手动准备两次以撒 / Normal / RD2W XWC9 run；实际 seed **1998345781**、房间 **84**。从 run 开始到 frame **6000** 的原始 gameplay 输入连续且中性，canonical Resolve 包含 ordinal 0/1，双方各有唯一普通样本。

用户明确确认左 Authority tick **11403** 对右 **10258**，A→A、B→B，实际 B birth event 1→1。B birth counter 为 **299/871**，加入时机不同，不补造相同初始世界。生产 world-compare 执行一次，exit=0，结论 **DIFFERENT**，定位 B 的 position.x/position.y/birth_counter/frame_count。未映射 Grid、earlier_coverage_unknown=true、uncompared_regions=true 均保留；不解释为全局确定性缺陷。

W11.scenario_basis 新增 PASS。配对提案、用户确认、具体依据及结果见 cross-process/；W11.production_compare 的账本仍保留历史 CARRIED_FORWARD 方法，本轮新比较事实在独立产物中保存，没有改写其出处。

## 九条故障命令与失败

每条故障命令仅发送一次，没有重发。唯一整体验收保留 **W12.fault_isolation=FAIL**：

| 故障 | 12 秒窗口内实际 tick / ack / world error | 判定 |
| --- | --- | --- |
| object-budget | 7121→7121 / 0 / 0 | 原窗口失败；之后原命令 ack=1、tick=20286，单独保留 |
| membership-budget | 22668→22680 / 2 / 0 | 单分支预期检查通过 |
| relation-budget | 25402→25421 / 3 / 0 | 单分支预期检查通过 |
| reference-ambiguity | 36067→36784 / 0 / 0 | 缺非空可读引用触发证据，原窗口失败 |
| field-read | 1478→1490 / 4 / 4 | 单分支预期检查通过 |
| source-changed | 1788→1800 / 5 / 17 | 单分支预期检查通过 |
| event-overflow | 1731→2447 / 0 / 0 | 原窗口失败；随后原命令迟到触发 |
| wrong-thread | 1671→1683 / 7 / 9 | 单分支预期检查通过；main=18752/worker=16848/rejected=0 |
| root-changed | 2238→2250 / 9 / 12 | 单分支预期检查通过 |

上述各分支的 coop error 均为 0。object-budget 原窗口没有 tick 推进的原因未在当时直接确定，不把后续恢复运行倒填为原窗口成功。

event-overflow 的 Take 位于生命周期事件发布函数；静止房间仅推进采样时没有消费原命令。窗口失败后，用户按检查点正常射击约 1 秒，原命令随后 ack=6、world error=13/enabled=0；独立两次观察 tick **6437→7355**、coop error=0。保存于 scene-observations.jsonl，原 fault-event-overflow.json 的 scheduler_continues=false 保持原样。它支持迟到的预期停用/继续推进行为，不能擦除原超时或把 W12 改成 PASS。

reference-ambiguity 的 Take 仅发生在 ResolveReferences 处理非空 VALUE 引用时；本轮未建立相应 retained Player 场景，没有重发或另造引用来补通过。field-read 使用 collector 自有 PAGE_NOACCESS 页；wrong-thread worker 在游戏对象读取前被实际检查拒绝；source/root 故障均为观察层自有分支，不改原生游戏指针。

## 尚未完成

1. W01.native_results：补实际 Replace/GridFree 调用，才有完整 19 入口证据。
2. W02.blocked_windows：完成所有声明窗口与实际 gap/token/tick 的因果配对。
3. W03.retained_unknown：实际 retained-only Player、非空引用及歧义证据。
4. W04.identity_history：完整短命对象退休与重复发布历史。
5. W05.transitions：实际 Replace/GridFree 及其身份变化。
6. W12.fault_isolation：当前 FAIL；后续实质修改需明确非空引用/生命周期触发前提与模拟继续条件，不能只增大超时或换编号重复验收来冲淡本次失败。

排除的 W01.isolation/W08.replay 另保留两项 NOT_COVERED。本轮不自动补采或重跑，当前游戏 70 槽已恢复；测试 Mod 仍在安装目录中，需用户在正常退出后手动禁用/移除，不能把 Hook 恢复等同于 fixture 已移除。
