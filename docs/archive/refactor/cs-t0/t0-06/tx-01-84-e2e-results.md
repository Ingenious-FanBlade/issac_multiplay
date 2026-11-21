# tx-01-84 专项 E2E 结果

2026-10-19 19:12–19:22（Asia/Shanghai）。唯一入口 `cs-t0/accept-world-trigger-scenes.py` 执行一次，结果 **FAIL：0 PASS / 2 FAIL**。没有重试或改写机器结果。原始 [result.json](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/result.json)、[收尾记录](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/finalization.json) 与[场景证明](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/trigger-evidence/scene-proof.json) 保留。

## 实际观察

- 用户全新进程 PID 51488，seed=1998345781、room=84，fixture v1.1。只有一个实机采集，没有故障、replay 或跨进程比较。
- Replace Hook 9 实际进入/原函数/返回各 3 次，返回 AL 均为 1。三个 outer token 为 6176、9279、10270；source/target 地址交替为 461712b4 / 460ab5ec。物理身份分别为 local_id/use=1/1、49/1；旧 roster 成员进入 Esau source 9，目标进入 roster source 8 ordinal 0。[原始 lifecycle](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/captures/triggers/capture/world/lifecycle-events.jsonl) 保留 source 的历史 birth_event=1/5。
- 第二、三次 Replace 内的 PlayerRestore 撤销当前 birth 映射可用性，后续样本 birth_event=null；事件仍保留既有出生事实。这不证明跨 Restore 状态可比较，也不应补造出生。旧汇总没有说明这一区别。
- fixture 在 frame 4088 于 index 69 创建普通岩石，seed=9730012，**初次公开记录的实际 variant 已为 2**。frame 4179 的 `gridfree-use` 硬编码要求 variant=0，因此记录 fixture_error 并在 D12 前停止。Hook 17 调用数为 0，GridFree 未覆盖。用户未看到控制台报错，但日志中的失败为实际判据。
- 汇总另有两处实现错误：Kind 18 参数记录同时包含 subject=0 的入口副本和 subject=1 的原函数副本，旧脚本未区分，三次 Replace 被列成六对；ActiveItem 实际 edge mask=2，旧脚本使用 mask=1（Bomb），导致道具边沿被漏计。保留本次 FAIL，不用修正脚本重判原 result。

## 健康与收尾

保存 4,868 个普通样本，全部阶段匹配，trace_integrity=true、errors=[]；930,759 条探针记录全部消费，overflow=0。world native_error=NONE、backpressure=0，15,997 批和 316 个事件全部写出并 flush。控制器正常退出；[独立原始字节观察](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/captures/triggers/external/after-normal-stop.json) 确认 70 槽恢复。[收尾诊断](../../../../../artifacts/cs-t0/tx-01-84-trigger-e2e-01/finalization-diagnostics.json) 无错误。

Replace/retained 的实机触发方法已获得直接证据，但本轮自动专项判定仍为 FAIL；GridFree 触发尚未完成。tx-01-83 的 25 PASS / 1 FAIL / 7 NOT_COVERED 原账本不变，不能据此宣布 T0-06 完成。

后续 tx-01-85 实质修改 fixture 的实际 variant 基线及场景汇总。计划只在新进程补采 GridFree，并在同一新入口内读取本轮封存的 Replace 原始证据；不重复三次 Esau 操作、不改本轮账本。
