# tx-01-87：剩余授权 E2E 完成

2026-10-19（Asia/Shanghai）。唯一统一入口执行一次，退出码 0；当前授权的六项剩余检查 **6/6 PASS**。累计 33 子项为 **31 PASS / 0 FAIL / 2 NOT_COVERED**，全局状态 **INCOMPLETE**：`W01.isolation`（真实 Online/Isaac Sync 隔离）和 `W08.replay`（全量 replay）继续按用户选择排除。25 项 PASS 沿用历史出处，六项结合保存的真实采集和本次 Continue/退出窗口完成；不声称本次重新覆盖全部场景或单一新构建的全部 19 Hook。

机器结果：[授权范围](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/authorized-scope-result.json)、[完整账本](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/result.json)、[收尾诊断](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/finalization-diagnostics.json)。收尾 errors=[]、original_error=null，证据汇总完成。tx-01-83/84/86 原 FAIL 均保留，未自动重跑。

## 实质修改与本次新采集

汇总修正未知引用可省略的 `target`、生产 reason 名称 `BUDGET_EXCEEDED`/`UNKNOWN_CONTINUITY`，并用实际 Spawn 请求 seed、正常返回指针、同 outer 出生事件关联短命 Pickup。没有修改原始数据、补造出生或未知目标。Native、controller、Helper 和已安装 Mod v1.3 未修改或重新构建；准备过程零测试。

本次进程 PID **32468**，采集时间 **22:30:06–22:33:00**。在有 Continue 的主菜单安装观察器，初始 tick=0、captures=0；用户手动 Continue 后才出现 A 和单次原生 B Join。工具前台切换被系统拒绝，用户手动切回游戏，未由工具发送 gameplay 输入。之后用户正常返回主菜单，保持同一游戏进程，执行器立即正常 stop。

- 实际 AlternateStateApply 加载 enter/exit 各一次（trace Kind 12/tag 2 共两条），loading 标志和生命周期窗口均被记录；普通门过渡没有被当作加载。
- **9,391** 个 attempt，**939** 个普通采集样本，阶段匹配 **939/939**，背压 **0**；其中 878 条 reason=NONE，61 条仍为 UNKNOWN_CONTINUITY，不能据阶段匹配将未知样本宣称完整或相等。335 个生命周期事件全部写出。
- **208,363** 条 probe 全部消费，closed=true、error=0、overflow=0，本轮 issued_faults=[]。
- 窗口按同 token/attempt 配对，旧 primary 和新段均 `window_pairs_ok=true`，window_mismatches=[]、lifecycle_errors=[]。
- 实际 run 退休后 world enabled=0、error=0，tick=1966；`teardown_ok=true`、retired_captures=[]。没有以控制器关闭代替 run 退出。
- controller 正常退出 0；Native 和外部观察均确认 **70 槽恢复**。DLL 按现行常驻策略保留到游戏进程退出，未强制卸载。

原始目录：[本次 capture](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/captures/continue-window/capture/)、[外部恢复](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/captures/continue-window/external/after-normal-stop.json)、[新窗口证明](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/continue-window-proof/proof.json)。

## 六项结论与证据边界

| 检查 | 结果与实际依据 |
| --- | --- |
| W01.native_results | PASS。tx-01-83/84/85 的原生 entry/original/return/ABI 见证累计覆盖 19 Hook，本次 trace 也健康；未宣称新 Native 单次覆盖全部 19 项。 |
| W02.blocked_windows | PASS。旧段实际暂停 tick=8117、captures=4059 两次均停止，另有 odd/换房；新段补真实加载与退休，各段全部窗口精确配对。 |
| W03.retained_unknown | PASS。沿用真实 retained Player 与未知类记录；旧 tx-01-86 的非空引用实际 KNOWN_CURRENT→AMBIGUOUS_REUSE→恢复，原始指针不变，未知 target 继续省略。 |
| W04.identity_history | PASS。历史嵌套生成/复用/重复 Publish，加上修正解码后的短命 Pickup 50/1、birth=5：真实 Spawn→Remove→ClearReferences→完整源无成员；不推导物理析构。 |
| W05.transitions | PASS。历史 Morph/Restore/Replace/GridFree、旧段跨房 A/B 物理身份保持，以及新段实际 run 退休后无普通采集。Restore 的当前 birth 映射 UNKNOWN 仍保留。 |
| W12.fault_isolation | PASS。读取已保存的九类受控失败及背压事实；三条 tx-01-86 故障均实际消费且 scheduler 继续，预算失败不产完整样本，引用故障有非空前提，event-overflow 有唯一自有诊断投递和容量失败标记。本次没有重发任何故障。 |

[旧 primary 修正证明](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/preserved-primary-proof/proof.json)、[故障合并证明](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/fault-completion.json)、[19 Hook 累计出处](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/cumulative-native-coverage.json)。两段各自的旧单段组合字段 `window_ok=false` 如实保留：旧段没有真实加载/退出，新段不重复短命/引用/换房/暂停。最终检查使用各段 `window_pairs_ok` 和分别实际取得的场景事实，未要求或伪造每段独自覆盖全部场景。

## 可重复入口

冻结执行参数见 [manifest](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/manifest.json)、[binary provenance](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/binary-provenance.json)、[controller invocation](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/captures/continue-window/controller-invocation.json)；执行器、分析代码与依据文档的当次副本保存在 [sources](../../../../../artifacts/cs-t0/tx-01-87-final-windows-e2e-01/sources/)。现有原始材料和最终证明足以逐条复查，无需重新运行游戏。

若未来另有授权执行新的实机验收，使用 `cs-t0/accept-world-final-windows.py`，参数为本次 manifest 中的 game-exe/controller/native-dll/helper/game-log 与真实授权说明，artifact-dir 必须是新的目录，不能覆盖本次结果。用户先在新进程可 Continue 的主菜单等待；入口内 `start continue-window`，安装后 `observe menu-before-continue`，再由用户 Continue；A/B 正常后 `observe loaded-run`，用户正常返回主菜单后 `observe after-run-teardown`、`fixture-log`、`stop`。所有阶段仍属于该统一入口的一次执行，不能单独试跑或自动重试。

当前授权剩余工作至此完成；两项用户排除保持未覆盖，不能将完整 T0-06 宣称为无条件 PASS。
