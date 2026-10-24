# tx-01-97：T0-07操作8结果与复现

日期：2026-10-20（Asia/Shanghai）。**操作8的生产故障与边界矩阵已完成：A/B conflict统一策略、source sequence极值、queue/history、far-future/expired/late/edge、timeout、tick/drain、identity/encoding/generation、phase/return/publication和collector gap均由当前生产Session/Clock/Pipeline/Participant执行。最终Native与故障场景程序纯编译成功，统一离线验收获 27场景、183条命令、184/184检查PASS。** 真实游戏、Hook、注入、采样内存、事件环和网络未运行。

## 1. 交付行为

- `Participant::Initialize`增加可选`source_sequence_start=1`的受控初态构造；Native默认仍为1。MAX-1可生成一次合法source命令，下一次`MakeCommand`在MAX处退休且不回绕；0/MAX wire sequence仍为Invalid。
- `AuthorityTickPipeline::AcceptInput`统一执行已采纳的conflict策略：A sequence conflict使session`Failed/RuntimeFailure`；B conflict由Participant Retire且不关闭session。typed/bytes入口一致。
- 新故障矩阵覆盖：
  - duplicate不推进accepted/processed、edge不重放；
  - A conflict关闭session，B conflict仅退休B且tick继续；
  - future window64/65/UINT64_MAX、expired、edge conflict、wire sequence0/MAX；
  - queue真实容量256后第257次Full；
  - dedupe history4096淘汰后seq1变Stale、新sequence仍Accepted；
  - source sequence MAX-1耗尽；
  - server tick max边界Exhausted；
  - 无Draining时AcceptInput fail closed；
  - abnormal/wrong/missing return、post-before-body、publication failure、repeat publish；
  - Disabled/Gap/Backpressure/Unavailable collector状态及odd Complete→Gap；
  - B连续121 tick无输入→TimedOut但session Active、B不退休；
  - malformed/short/long/wrong session/client/old generation/rebind恢复；
  - duplicate与late/expired/新target；late edge不重放，accepted高水位保持不变；
  - source初始0/MAX拒绝；queue满后256条仍可完成tick；
  - B-only timeout且A持续新鲜输入、坏sequence不保活、合法新输入恢复；
  - repeat-post、wrong-loop post、wrong-tick→Unavailable(18)、simulation后missing return/post；
  - terminal session下直接生产Participant拒绝输入；version/length/edge-flag malformed variants。

## 2. 最终构建与统一验收

最终纯编译入口：

```powershell
pwsh -File ./cs-t0/build-fault-matrix.ps1 `
  -OutputDirectory out/cs-t0/tx-01-97-build-05
```

最终统一离线入口：

```powershell
D:\miniconda3\python.exe cs-t0/accept-fault-matrix.py `
  --build-dir out/cs-t0/tx-01-97-build-05 `
  --artifact-dir artifacts/cs-t0/tx-01-97-fault-matrix-06
```

结果：`PASS`，27 scenarios、183 commands、184/184 checks、exit0、errors=[]、约0.056秒。184项为183条逐步检查加1项完整transcript检查。

## 3. 历史失败与修正

- `out/cs-t0/tx-01-97-build-01` 纯编译成功。
- `artifacts/cs-t0/tx-01-97-fault-matrix-01` FAIL：故障场景程序沿用了4参数入口而统一入口实际传3参数，exit2且无events；修正argc/去掉无用batch流。
- `out/cs-t0/tx-01-97-build-02` 修正后纯编译成功。
- `artifacts/cs-t0/tx-01-97-fault-matrix-02` FAIL 125/133：8项仅是测试预期错误——Activated session的cause是`BindingsReady`而非None，Snapshot未输出`simulation`字段（应检查`current`）。修正testdata预期，不动生产逻辑。
- `artifacts/cs-t0/tx-01-97-fault-matrix-03` PASS 133/133；这是扩大覆盖前的中间结果，保留但不作为最终操作8证据。
- 独立静态审阅指出A-conflict只改session而未Abort、Native transition可能丢失，以及source初值/queue后续/B-only timeout/返回发布/collector mismatch/terminal/identity扩展缺口。生产pipeline冲突路径改为session fail后进入Abort并取消/失败phase，Native`FailFrom`保留pre-failure SessionStatus发布transition；场景扩大到27个。
- `out/cs-t0/tx-01-97-build-03` 纯编译成功；`artifacts/...-04` FAIL 179/184，新增edge/availability字段拼接漏闭合引号导致JSON非法；修正后`build-04`成功。
- `artifacts/cs-t0/tx-01-97-fault-matrix-05` FAIL：场景程序仍在edge/availability字段输出处生成非法JSON；修正字段拼接，未改变生产状态逻辑。
- `out/cs-t0/tx-01-97-build-05` + `artifacts/cs-t0/tx-01-97-fault-matrix-06`：最终PASS 184/184。

上述旧build/artifact、commands、events和failure结果均原样保留，不覆盖、不自动改判。

## 4. 覆盖来源

操作3–7已有场景继续作为历史覆盖来源，不重写为op8结果：

- tx-01-91/92：client input state、typed/bytes统一准入、malformed长度、identity、queue、history、near-U64 future。
- tx-01-93：tick pipeline阶段顺序、collector状态、return/publication、budget、missing-B等。
- tx-01-95：missing/effective输入、120-tick静默诊断、recovery、rebind/retire。
- tx-01-96：runtime config范围、IPC v5、admission budget、clock边界。

op8新矩阵补齐共享生产链上的A/B conflict、source sequence极值、history淘汰、queue满、collector连续状态和跨场景边界。

## 5. 未运行项与限制

**真实游戏、Windows Hook/原生original、注入、真实输入采样/共享内存、事件通知/事件环背压、网络收包/ACK、真实Active和世界动作效果均为NOT_RUN。** 离线故障矩阵调用生产类但使用离线地址标记、显式body/collector结果，不证明实机Hook或世界动作。B仍是脚本源；操作9真实Authority停发实验未实施。

queue/history/source极值测试使用合法初态构造，不通过执行u64次或修改live寄存器。没有计算/校验哈希，没有新增gate/unlock或网络重试。

## 6. 产物

- [最终结果](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/result.json)
- [最终场景命令](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/commands.txt)、[逐step事件](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/events.jsonl)
- [最终源码快照](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/sources/)、[二进制及纯编译日志](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/build/)、[路径/长度/时间出处](../../../../artifacts/cs-t0/tx-01-97-fault-matrix-06/binary-provenance.json)
- [操作8实现与覆盖矩阵](tx-01-97-fault-matrix.md)、[静态审阅](tx-01-97-static-review.md)

后续复现使用新build/artifact目录，不覆盖历史失败。
