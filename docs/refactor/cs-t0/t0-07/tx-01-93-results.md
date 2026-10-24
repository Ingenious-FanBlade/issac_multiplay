# tx-01-93：操作5结果与复现

2026-10-20（Asia/Shanghai）。**T0-07操作5的生产pipeline与Native接入已完成；Native和场景程序最终纯编译成功；唯一一次统一离线验收36场景、254/254项检查PASS，退出码0、errors=[]。** 保存251条场景命令与120条连续生命周期记录，用时0.076秒。没有实机/Hook/注入/网络访问，没有重跑或另跑旧验收。

## 交付行为

新AuthorityTickPipeline直接协调原Session、Clock和A/B Participant：有界drain → 同一AcceptInput验证 → 双方同tick Resolve/可见快照 → 既有wrapper执行原生body → 正常返回End(true)并关闭消费者可见性 → Outer/tail后post-state复制或明确缺口 → TopExit提交tick → 释放匹配世界值批次并发布本地处理回执。

原生正常返回时completed保持不变；等待采样期间SimulationTick为0，tail/FSM/Shooting不能因保留的clock候选继续消费输入。完整世界复制仍使用原owner/Room/tail/outer条件，authority批次先保持WRITING，完成tick后才发布。odd-only不获得完整世界资格；Disabled、Gap、Backpressure、Unavailable均显式记录。按现行规范不计算或校验哈希，不把采样可用性作为模拟计数条件。

A物理poll只观察，新鲜输入在Authority pre与B脚本共同准入；所有duplicate/invalid/startup尝试计入258的有界预算，现有源的实际最坏值为134。controlled conflict-b移到下一drain并保留generation，重绑后不复活。缺B不阻止双Resolve或A/tick推进。

NativeReturned之后的生命周期/采样变化不会撤销已经确认的正常模拟；若session在正常返回witness之前已Retiring/Failed，则不具备Active完成资格。publication失败保留已经发生的tick及可能发布的前缀，关闭后续输入，不回滚或重试。新增EventKind34本地回执保留processed高水位、实际source序号、处理代际、binding有效性与采样状态，不是连续ACK或网络消息。

按用户要求使用 **GPT-6 Luna / max** 做只读静态分析，主智能体复读并逐项审阅，明确Disabled是无collector采集缺口、当前/处理generation分离、生命周期前后边界、pending槽释放和实际预算。见[完整实现、失败清单及记录格式](tx-01-93-tick-pipeline.md)和[主智能体逐项审阅](tx-01-93-static-review.md)。没有新增Hook或修改Event/Shared大小，24/64字节codec不变；原版Online/Isaac Sync不进入或复用。

## 实际覆盖与限制

唯一入口链接生产pipeline/Session/Clock/codec/Participant，覆盖固定执行顺序、只读重复查询、正常End与边沿确认、异常/错配/缺失返回、禁用/odd/gap/背压/不可用采样、预算与非法输入、warmup/阻断/错误线程/来源、旧代和B冲突退休、失绑不等待、返回后生命周期变化、publication失败不回滚、重复commit/publication拒绝、时钟上界，以及B无输入时A连续完成120次生命周期。

254项检查包括251条场景逐步断言、输出完整性、循环记录完整性及120条循环保留状态的联合核对。120条循环记录不是额外120项顶层检查。每条循环经drain、准入、双Resolve、正常返回、post-state、commit、publication完成，实际A的edge仅确认一次，B submit/processed始终为0。

**真实游戏、Windows Hook/原生original、真实采样内存/共享槽、通知与恢复、网络收包/ACK均为NOT_RUN。** 离线程序用自有地址标记绑定；body调用及collector结果为显式环境输入，不将driver的body_calls计数冒充游戏original证据。生产world staging/Native callback接入只完成静态审阅和纯编译，尚无新的实机运行证明。B生产源仍是脚本，没有snapshot apply或真实协议；操作6–9未实施，不宣称整个T0-07完成。

## 产物

- [机器结果及254项检查](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/result.json)
- [场景命令](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/commands.txt)、[逐阶段状态及回执](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/events.jsonl)、[120条连续生命周期记录](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/batch-events.jsonl)
- [实际源码/场景/审阅快照](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/sources/)、[Native/场景二进制及编译日志](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/build/)、[路径/长度/时间出处](../../../../artifacts/cs-t0/tx-01-93-tick-pipeline-01/binary-provenance.json)

两次均为纯编译：初版在 `out/cs-t0/tx-01-93-build` 成功；主审最后将post-state参数改为const引用，遵循现有x86 LTCG聚合参数约定，并补充预算注释；最终在 `out/cs-t0/tx-01-93-build-final` 成功。唯一验收使用最终二进制。此前没有执行测试，也未在代码之后补写单元测试；所有失败方式、36个场景、驱动及入口在生产修改前写好。

## 复现

本轮只调用一次：

```powershell
D:\miniconda3\python.exe cs-t0/accept-tick-pipeline.py `
  --build-dir out/cs-t0/tx-01-93-build-final `
  --artifact-dir artifacts/cs-t0/tx-01-93-tick-pipeline-01
```

保留本轮目录，后续复现使用新目录，不自动重跑：

```powershell
& ./cs-t0/build-tick-pipeline.ps1 -OutputDirectory out/cs-t0/NEW-TICK-PIPELINE-BUILD
python cs-t0/accept-tick-pipeline.py `
  --build-dir out/cs-t0/NEW-TICK-PIPELINE-BUILD `
  --artifact-dir artifacts/cs-t0/NEW-TICK-PIPELINE-ACCEPTANCE
```

工具链VS2022/v143、Hostx64→Win32 Release；编译读取Windows SDK安装信息，不执行程序。验收创建新目录并保存输入/源码/二进制，调用场景程序一次；不计算或校验哈希。历史结果和失败记录保持。
