# tx-01-95：T0-07操作6结果与复现

日期：2026-10-20（Asia/Shanghai）。**操作6的缺失输入、源静默超时诊断和恢复语义已在生产`Participant`/tick receipt/Native诊断路径实现；Native与离线场景程序最终纯编译成功；修正测试场景后的唯一整体离线验收为PASS：10场景、146条命令、122条批量tick记录、152/152项检查，退出码0、errors=[]。** 没有运行游戏、Hook、注入、采样内存、事件环、网络或世界动作；不宣称实机验证。首次失败与编译失败均原样保留。

## 1. 交付行为

- effective输入按同代、同client、target恰好等于当前Authority tick判定。没有当前target命令时current立即为中性零状态；已接受未来命令只算源到达/队列内容，不算当前effective输入。
- `max_input_age_ticks=0`，held/连续量立即中性；last held/continuous状态不会延长旧输入。edge只来自当前target命令，duplicate/expired/late/old-generation不重放；重复Read不改变任何账本。
- 新增独立有效到达进度：只有完整验证、成功入队的同代命令更新到达tick/sequence/target/generation和累计计数。duplicate、invalid、wrong session/client、old generation、stale、expired、too-far、full、conflict/终态拒绝都不刷新。
- 源静默与effective缺失分开记录。固定`kClientSilenceTimeoutTicks=120`个Authority tick；`Silent`为1–119，`TimedOut`为达到/超过120。阈值只产生诊断和迁移计数，不自动踢B、不结束session、不触发AI或网络断线。
- 静默起点为当前generation最近一次Accepted到达的候选tick；尚无有效到达时取该generation首个已绑定`Begin`的tick。pause/loading/blocked不调用Participant Begin，completed与silent ticks都不前进；Invalidate重置当前代起点和effective source，保留历史计数/history。
- `NoTargetCommand`与`FutureOnly`显式区分；source为Active/Silent/TimedOut/Unbound/Retired/SessionInactive/Unknown。异常返回、生命周期失效或终态不确认missing scope的neutral processed状态或edge。
- `ClientInputProgress`和每个成功tick的`TickClientReceipt`携带同一availability值快照。Native新增EventKind35，每成功tick每client一条；退休EventKind33新增固定query8/9。Event仍128字节，Shared v3/v4偏移、24字节state、64字节command和Hook事务不变。

## 2. 构建与唯一整体验收

最终纯编译入口：

```powershell
pwsh -File ./cs-t0/build-missing-input.ps1 `
  -OutputDirectory out/cs-t0/tx-01-95-build-02
```

最终统一离线入口实际只执行一次：

```powershell
D:\miniconda3\python.exe cs-t0/accept-missing-input.py `
  --build-dir out/cs-t0/tx-01-95-build-02 `
  --artifact-dir artifacts/cs-t0/tx-01-95-missing-input-02
```

结果：`PASS`，10 scenarios、146 commands、122 cycle records、152/152 checks、exit code 0、errors=[]、约0.062秒。152项由146条逐命令检查、完整transcript、完整cycle计数、3条批量边界观察和1条固定策略检查组成；122条批量记录不是122项独立顶层检查。

覆盖：fresh binding无历史缺失、held+edge后缺失中性/edge不重放/重复Read不增加处理、A/B独立且B缺失不阻止A/tick、未来target与FutureOnly、timeout的119/120/121边界、duplicate/invalid/old-generation不保活、timeout后合法新输入恢复、pause/loading blocked不增龄、失绑/rebind/旧代迟到、session Retiring与Participant Retired终态、异常返回不确认missing或edge、固定0/120策略与receipt availability。

## 3. 历史失败与修正

- `out/cs-t0/tx-01-95-build-01` 首次纯编译失败：离线驱动辅助函数参数写成`std::stringstream&`，实际输入流为`std::istringstream`；修正为`std::istream&`后，`build-02`完整编译Native与场景程序成功。首次失败发生在任何场景程序执行之前。
- `artifacts/cs-t0/tx-01-95-missing-input-01` 首次统一入口实际执行并保留为**FAIL 141/152**。失败原因是测试场景自身：`Magnitude`的action轴方向与读取动作不一致、失绑tick错误期望伪造source=0、终态直接输入用了错误的`direct remote`语法。生产availability/neutral/edge行为本身没有在这些失败项上暴露出不同结论。
- 修正只改`testdata/tx-01-95-missing-input.json`的命令/预期和驱动`istream`类型；没有放宽生产validator、门或检查。随后在全新`tx-01-95-missing-input-02`目录执行一次并得到152/152 PASS。两个artifact、旧commands/events和旧FAIL结果均保留，不覆盖、不自动改判。

## 4. 边界与未运行项

**真实游戏、Windows Hook/原生original、真实输入采样/共享内存、通知/事件环背压、网络收包/ACK、实际A动作和B物理中性均为NOT_RUN。** 离线驱动链接真实生产Session/Clock/Pipeline/Participant/V2 codec，但body调用、采样与对象地址是显式离线输入；不能冒充实机Hook或世界动作证据。B目前仍是脚本源，T0-07操作7–9未实施。

timeout是诊断，不是网络liveness；本次不宣称断线检测、kick、reconnect、lead重标、旧edge补发、snapshot/rollback或产品超时策略。EventKind35/Native接入只完成源码审阅与纯编译，未在真实事件环运行。

## 5. 产物

- [最终机器结果](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/result.json)
- [最终命令](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/commands.txt)、[逐命令状态](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/events.jsonl)、[122条批量tick记录](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/batch-events.jsonl)
- [最终源码快照](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/sources/)、[Native/场景二进制与编译日志](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/build/)、[路径/长度/时间出处](../../../../artifacts/cs-t0/tx-01-95-missing-input-02/binary-provenance.json)
- [首次失败结果，原样保留](../../../../artifacts/cs-t0/tx-01-95-missing-input-01/result.json)
- [实现前范围、失败清单与行为口径](tx-01-95-missing-input-timeout-recovery.md)、[静态审阅与测试修正](tx-01-95-static-review.md)

本轮不计算或校验哈希；实际配置与状态以最终artifact的commands/events/result和上述源码为准。后续复现必须使用新build与artifact目录，不覆盖历史结果；旧失败不重跑。
