# tx-01-97：T0-07 操作8，生产故障与边界覆盖

日期：2026-10-20（Asia/Shanghai）。完整目标：在操作1–7的生产状态链上盘点已有故障证据，补齐source sequence极值、pipeline conflict统一策略、history淘汰、queue满、identity/encoding/代际、阶段/返回/发布、collector gap等边界，并形成一个统一离线故障矩阵。真实游戏、Hook、网络和世界动作不在本操作范围；不进入原版Online/Isaac Sync，不计算或校验哈希。

本轮先写失败清单、场景和统一入口，再改生产测试支撑点；全部完成后纯编译Native/场景程序，只执行一次整体离线验收。已有操作3–7场景保留，不重写或冒充本轮新结果；新矩阵重点整合共享边界和此前未覆盖的source/pipeline缺口。

## 1. 已有覆盖与剩余缺口

| 故障类别 | 已有基础 | 本轮补充 |
| --- | --- | --- |
| duplicate | tx-01-92跨typed/bytes duplicate，tx-01-93/95 pipeline重复read | 新矩阵再次用同一生产AcceptInput验证重复不推进accepted/processed，edge不重放 |
| conflict | tx-01-92 B conflict退休，tx-01-93 B-only conflict、Native A conflict wrapper | 将A conflict→session Failed / B conflict→B Retired的统一策略落到`AuthorityTickPipeline`公共调度边界，并用生产pipeline驱动两类 |
| late/expired | tx-01-92 expired/stale，tx-01-93 accept-after-resolve | 新矩阵覆盖过期target、late edge、历史淘汰后的stale |
| far-future | tx-01-92 near-u64-window，tx-01-96 future上界 | 同tick64/65/UINT64_MAX target和future window边界 |
| queue-full | tx-01-92/93 queue-capacity、budget | 256+1实际Participant队列，确认Full不触发world/edge |
| client timeout | tx-01-95 119/120/121阈值、坏包不保活 | 新矩阵连续121 tick再次检查TimedOut不自动退休/结束 |
| source sequence | MakeCommand有u64 max/0 guard但缺可构造初态 | `Participant::Initialize`可选初始source sequence（默认1），离线从MAX-1构造；0/MAX wire值仍Invalid |
| server tick | tx-01-90/93/96 near-max clock | 新矩阵pipeline exact Exhausted |
| identity/encoding | tx-01-92完整typed/bytes/wrong session/client/generation/length | 新矩阵组合malformed、short、long、wrong session/client、old generation、rebind后恢复 |
| phase/return/publication | tx-01-93完整tick阶段和失败场景 | 新矩阵再次贯通accept-outside、abnormal/wrong/missing return、post-before-body、publication fail、repeat publish |
| collector gap | tx-01-93 Disabled/Gap/Backpressure/Unavailable/odd | 新矩阵单Domain连续产生四类post状态及odd→Gap |
| history/dedupe | tx-01-91/92 history-eviction-highwater | 新矩阵4100次production Submit/Begin/End后验证seq1 Stale、新seq仍Accepted |

## 2. 实现前失败方式

1. 只在旧场景清单中勾选已有PASS，不把当前生产链重新执行；或把历史artifact冒充操作8新证据。
2. A/B sequence conflict使用不同策略；A冲突不关闭session、B冲突错误关闭session，或冲突包进入队列/触发edge。
3. source sequence=0或u64 max被接受，或`MakeCommand`在max处回绕；极值测试靠执行数十亿次而不是合法初态构造。
4. queue满沿用旧的“只在替身队列”判断，实际Participant仍接受/覆盖pending，或Full污染accepted高水位/edge。
5. history淘汰后旧sequence重新入队，Stale检查只依赖仍在ring中的history；或者4100循环破坏last accepted和dedupe。
6. expired/late target仍触发edge；far-future/UINT64_MAX发生回绕或错误归一化；duplicate刷新accepted/processed。
7. identity错误按payload client重路由；短包/长包被截断/修补成合法64字节；旧generation在rebind后复活。
8. abnormal/wrong/missing return仍提交completed或发布success；错误observe slot改变已经发生prior tick的事实。
9. post在body前/重复post/repeat publish/cancelled publication被当成成功或自动重试。
10. collector Disabled/Gap/Backpressure/Unavailable被标成完整，odd-only获得完整world资格。
11. queue满、timeout或边界故障被Node错误地变成自动重试、自动取消、AI接管或session结束。
12. 新测试支撑参数改变Native默认source sequence、wire身份、24/64字节codec、Hook事务或Shared/Event布局。
13. 每条路径重建不同validator/queue/fake FSM，未调用生产Session/Clock/Pipeline/Participant。
14. 实现后补写场景、或验收失败后覆盖旧artifact/自动重跑。
15. 把离线scenario direct call冒充实机Hook、真实网络、真实world action。

## 3. 生产测试支撑与行为决定

- `Participant::Initialize`新增可选`source_sequence_start=1`，仅供受控初态构造；默认Native调用不变。0和u64 max是非法wire序列值，source起点本身也拒绝这两个值。从`UINT64_MAX-1`可生成一次合法命令，随后`MakeCommand`检测`UINT64_MAX`并退休，不回绕。
- `AuthorityTickPipeline::AcceptInput`（typed和bytes共用）在Participant返回`SequenceConflict`后执行已采纳策略：A冲突使session `Failed/RuntimeFailure`；B冲突由Participant自身Retire。Native原wrapper的同一策略保留为外层失败发布，不改变输入布局。
- 不修改queue容量256、dedupe history 4096、future window64、timeout120、24/64字节codec、Event/Shared大小或Hook事务。
- `SourceActivity`/`MissingInputReason`、post-state状态和tick receipt继续使用操作5/6已有结构。

## 4. 新离线故障矩阵

`fault_matrix_main.cpp`链接生产Session/Clock/Pipeline/Participant/V2 codec，场景先写入`cs-t0/testdata/tx-01-97-fault-matrix.json`。27个场景覆盖：

- A/B conflict统一策略；
- far-future/expired/edge conflict/0与MAX sequence；
- 256项queue + Full；
- 4100次Submit/Begin/End后的history stale；
- source sequence MAX-1耗尽；
- server tick max边界；
- accept outside drain phase；
- abnormal/wrong/missing native return；
- post-before-body、publication failure、repeat publish；
- Disabled/Gap/Backpressure/Unavailable/odd collector状态；
- B-only 121 tick timeout（A持续新鲜输入）、坏包不保活、合法输入恢复；
- malformed/version/length/edge-flag、short/long、wrong session/client/generation、rebind recovery；
- duplicate与late/expired/新target处理，late edge不重放、accepted高水位不变；
- source初始0/MAX拒绝、queue满后完整tick、repeat-post/wrong-loop/wrong-tick、simulation后missing return/post、terminal session直接输入拒绝。

统一入口`accept-fault-matrix.py`只启动一次场景程序，保存commands、events、源码、二进制、日志和路径长度时间；最终结果为27场景、183命令、184/184检查PASS，不计算或校验哈希。真实Native DLL、Hook、输入采样内存、通知/事件环和游戏动作仍为NOT_RUN。

## 5. 完成依据

- 每类故障均以当前生产代码的可复现输入执行，不用历史报告替代本轮矩阵；
- A/B conflict策略、source极值、queue/history、timeout、return/publication和collector状态与设计一致；
- 失败后不自动重试、不覆盖旧artifact，未引入gate/hash/网络行为；
- 最终只保留一次修正后的统一PASS结果，并把失败记录单独保存。
