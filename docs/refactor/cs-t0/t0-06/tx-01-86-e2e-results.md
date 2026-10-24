# tx-01-86 剩余 E2E：原始采集完成，汇总失败

2026-10-19（Asia/Shanghai）。统一入口执行一次，最终 **FAIL**，错误为 `KeyError: 'target'`；账本保留 **25 PASS / 8 NOT_COVERED**，六个本轮目标尚未获得最终判定。25 PASS 均沿用历史来源，未宣称本轮重新通过。没有重发故障、重注入或改写旧结果。

见 [result.json](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/result.json)、[范围结果](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/authorized-scope-result.json)、[收尾诊断](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/finalization-diagnostics.json)。诊断 errors=[]，evidence_summary_completed=true，保留最初 KeyError。

## 原始事实

- remaining 新进程 PID 36960，普通样本 4,508，全部阶段匹配；107,956 个 attempt、背压 0，1,350,517 条 probe 全部消费，trace_integrity=true、errors=[]。原生和外部均确认 70 槽恢复。
- object-budget 单次 ack=1，tick 4682→4694；reference-ambiguity 单次 ack=8，tick 5581→5594；两者 coop/world error=0，scheduler_continues=true。
- 引用 child local_id/use=49/1、parent=48/1，保存的字段 0019 为非空 KNOWN_CURRENT。实际 ReferenceFault 记录此前提和受控歧义，后续引用恢复；原始指针没有修改。
- 短命 Pickup seed=9730086，真实 Spawn 参数记录在 trace sequence 228685、subject=0、tag=18，seed 位于 words[6]。对象 local_id/use=50/1、birth_event=5；Spawn 返回 sequence 101，Remove 正常返回 103，ClearReferences 返回 106，完整成员观测 107 的 bitset=0。出生与完整退休事实已记录，但旧汇总错误地只用 Base Init 参数搜 seed，可能漏绑定这种派生 Init。
- 实际暂停两次观察 tick 均为 8117、普通样本数均为 4059；world/coop error=0。普通门过渡有 NewRoom/RoomChange 记录，当前 Window 配对文件未列不匹配项。
- primary 控制器有既有 1800 秒自动停止条件。恢复交互时控制器已经正常退出；父执行器在收到下一条交互输入后于 22:11:38 登记退出。该登记时间不等于控制器实际停止时刻。之后的 after-room 标记未发送，未采到要求的主菜单 run teardown，也没有真实 AlternateEnvelope 加载。
- event-overflow 新进程 PID 11940，单次 collector 自有诊断事件进入实际 Emit 容量失败，ack=6、world error=13/enabled=0，tick 1453→1465、coop error=0，scheduler_continues=true。138,105 条 probe 全部消费、overflow=0；原生及外部 70 槽恢复。原始受控 world CAPTURE_ERROR 保留，不改成健康采集。

上述原始目录：[captures](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/captures/)、[三条故障](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/fault-event-overflow.json)、[窗口配对](../../../../artifacts/cs-t0/tx-01-86-remaining-e2e-01/completion-evidence/window-pairs.jsonl)。

## 汇总缺陷及剩余工作

引用目标为未知状态时，生产 JSON 按 schema 省略 target；汇总直接用 `["target"]` 导致 KeyError，应只读 `.get("target")`，不能给原始记录补造目标。预算原始 reason 为 `BUDGET_EXCEEDED`，旧汇总误匹配 `BUDGET`。短命身份应按有明确 seed 的真实 Spawn 请求、正常返回指针及同 outer 出生事件关联，而不是假设 seed 一定出现在 Base Init 参数。

这些是实质汇总修正；另需实际 Continue 加载和 run teardown，普通门过渡不能冒充 AlternateEnvelope。tx-01-87 将在一个新统一入口内读取本轮原始采集并补采主菜单 Continue→正常 run→主菜单退出；三条故障、短命对象、引用与已通过旧场景均不重跑。本次 FAIL、tx-01-83/84 的失败与两项用户排除均保留。
