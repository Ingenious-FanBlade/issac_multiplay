# tx-01-98：T0-07操作9，真实 Authority 的 B 输入停发实验

日期：2026-10-20（Asia/Shanghai）。范围：完成[操作6–9计划](tx-01-94-remaining-operations-plan.md#5-操作9真实authority的b输入停发实验)中的操作9。执行规范以[现行项目规范](../../project-conventions.md)为准：不设 gate、不校验哈希；真实游戏、注入和运行输入只使用用户在本会话中“完成操作9”的实际授权；准备阶段只做静态阅读、实现和纯编译，整个目标完成后一个统一入口执行一次整体实机验收。

**后续执行状态（2026-10-20）：最终有效 invocation `tx-01-98-source-stop-e2e-04` 已 PASS 24/24，controller exit0、70 槽恢复；用户随后基于操作1–8记录与本次操作9结果确认 T0-07 整体 PASS。前三次失败（错误 GameExe、A 动作晚到、A 穿门死亡）及其修正原样保留。实际运行和限制见[结果](tx-01-98-results.md)，静态审阅见[审阅](tx-01-98-static-review.md)。本文件以下内容保留设计时口径；实际 world record 全部为 partial `NOT_READY`，仅可用 Player 字段作为明确 partial A/B 位置证据，没有可见的 complete world snapshot。**

## 1. 目标与边界

- 在一个真实 `isaac-ng.exe` 进程、一个稳定 run/room 和一次已绑定的 Authority A/B session 中，覆盖 B **停止新输入 1、5、30、120 个 Authority tick**，每段都有稳定输入前置区间和恢复观察区间。
- B 继续绑定、session 继续 Active；不停 B、不删除世界对象、不退出 run、不重绑、不清空队列、不补造 sequence，也不改 B 世界字段来类比输入静默。
- 停止发生在 B 的生产脚本源发布点：`ScriptInput` 对精确 target tick 没有可发布的脚本 step。这里测试的是脚本源停发，不是真实网络收包/断线；不能标记网络断线、wire ACK 或 transport failure 为 PASS。
- A 保持正常物理键盘输入并产生可观察的世界移动。B 中性按 canonical effective input 为零判定，不由 B 的惯性、碰撞或位置冻结判定。
- 运行使用显式 IPC v5 配置、`min-read-v1` world record 和同一 70 槽恢复事务。world record 提供真实 A/B Player 位置、post-state 状态、标准/odd 边界和恢复证据，不来自测试替身。

## 2. 场景设计

脚本由 `testdata/tx-01-98-source-stop-script.json` 预装进控制器，共 200 个 step，相对脚本 origin 覆盖 0..355；没有脚本 step 的 target 区间就是 B 源停发区间。各稳定段的第一 tick 带一次 drop edge（`edge_flags=8`），之后保持移动轴；间隙内没有命令，因此没有旧 held/edge 可消费。

| 段 | 相对 target | 长度 | B 源行为 | B canonical 状态 |
| --- | ---: | ---: | --- | --- |
| prefix | 0..39 | 40 | 每 tick 发布 | `move_x=-32767`，首 tick drop edge |
| stop-1 | 40 | 1 | 不发布 | 无新命令 |
| stable-1 | 41..80 | 40 | 每 tick 发布 | `move_x=+32767`，首 tick drop edge |
| stop-5 | 81..85 | 5 | 不发布 | 无新命令 |
| stable-2 | 86..125 | 40 | 每 tick 发布 | `move_x=-32767`，首 tick drop edge |
| stop-30 | 126..155 | 30 | 不发布 | 无新命令 |
| stable-3 | 156..195 | 40 | 每 tick 发布 | `move_x=+32767`，首 tick drop edge |
| stop-120 | 196..315 | 120 | 不发布 | 无新命令 |
| recovery-4 | 316..355 | 40 | 每 tick 发布 | `move_x=-32767`，首 tick drop edge |

实际 Authority tick 带 `input_lead_ticks=2` 的发布提前量。分析器不硬编码全局 tick，而从生产 `Submit` 事件测量 `target - source_event_tick`，再要求每个源停发窗口与对应 effective 缺失窗口按该提前量对齐，长度同为 1/5/30/120。源停发窗口开始前最后一个已接受 target 应恰为 effective 缺失前一 tick，并由该 tick 的 B availability 证明它作为队列中的未来命令被自然消费；恢复后的新命令使用连续的新 sequence/target。

由于恢复命令可能在该段最后一个有效缺失 tick 的 drain 中提前接受，缺失原因会出现两种明确形状：较早 tick 为 `NoTargetCommand`，最后一个 tick 可为 `FutureOnly`；1-tick 源停发只有这个 `FutureOnly` effective tick。两者都必须 `effective_available=false`、`neutral_applied=true`，不能把已收到未来命令当作当前 tick 有输入。只有更长的源静默窗口才一定产生 `Silent`/`TimedOut` 诊断；120-tick 窗口必须实际到 120-tick 阈值并在后续新鲜输入处 recovery，且不退休 B 或结束 session。

A 的物理操作由已提供的键盘工具执行。最终实现使用 controller `--start-delay-seconds` 让外部动作先进入运行，再用 bounded `W/D/S/A` 中心方形动作覆盖注入、B 脚本和恢复窗口；不使用会导致穿门/死亡的持续单向动作。分析器要求每个 B 停发窗口有实际 A `Resolve` 非零输入，从同一 run 的 world record 读取可用 A Player 位置并要求真实位移；world 的 complete/partial/reason 分开记录。B 中立只由 B `Resolve` 全零和 availability `neutral_applied` 判定。

## 3. 实现前失败方式

1. B 未真实 Join/绑定，或 Authority 未进入 Active；后续“停发”只发生在 menu/warmup/failed 状态。
2. 源没有按计划停发：`Submit` tick 和 target 集合不存在四个精确 1/5/30/120 内部缺口，或停发窗口被清队列、rebind、脚本重置掩盖。
3. 停发被错当成 effective 缺失：最后一个已接受未来 target 尚未消费、`FutureOnly` 与 `NoTargetCommand` 混淆，或 lead 对齐错误。
4. B 缺输入时仍保留旧 held/连续量或重放旧 edge；`Resolve` 非零、availability `effective_available=1`、或 edge 处理数不是五次。
5. B 缺失成为 A/tick 的等待条件：Authority boundary、A receipt、B receipt、commit 或 post-state 任一缺少；或真实标准/odd boundary 的 thread/owner/return RVA 不成立。
6. 恢复重放旧 queue/edge、复用 sequence、重置 generation，或恢复状态不是新脚本段的状态。
7. 120-tick 静默没有达到固定 120 tick 诊断阈值，或 timeout 错误地退休 B/结束 session；坏包不能代替本次合法恢复。
8. A 只有 query/计数而非真实动作：没有 A 物理 Resolve 或同一 world record 中无 Player 位置变化。
9. world record 采样 gap、背压、事件错配或 Native post-state `Unavailable` 被隐藏成完整世界；不允许靠删除不可读字段或最近 tick 补齐。
10. 正常停止后 70 个目标未恢复、world producer/consumer 未 drain、controller 非零退出、B 未按正常 stop 退休或游戏进程崩溃。
11. 把脚本源停发、一次成功 run 或 A 动作外推成真实网络断线、任意角色/武器/lifecycle 或完整 T0-07。

## 4. 统一验收与判据

构建入口（纯编译，不执行产物）：

```powershell
pwsh -File ./cs-t0/build-authority-source-stop.ps1 `
  -OutputDirectory out/cs-t0/tx-01-98-build-01
```

实机前置：用户启动唯一隔离 `isaac-ng.exe`，进入正常 **New Run**，只有 A，关闭原版 Online/Isaac Sync，无 debugger、Cheat Engine 或已驻留的 T0 Native DLL。统一入口会先校验磁盘 PE 是批准的 canonical/unpacked 目标。最终实机使用 `-StartDelaySeconds 10`：后台入口先准备 artifact 并等待，同时外部授权键盘工具开始 bounded `W/D/S/A` 中心方形动作；延迟结束后才注入，确保 B 首个 gap 之前已有 A 动作。之后统一入口：

```powershell
pwsh -File ./cs-t0/accept-authority-source-stop.ps1 `
  -GameExe '<approved isaac-ng.exe>' `
  -BuildDirectory out/cs-t0/tx-01-98-build-01 `
  -ArtifactDirectory artifacts/cs-t0/tx-01-98-source-stop-e2e-04 `
  -Authorization 'user requested completion of T0-07 operation 9 in this session' `
  -StartDelaySeconds 10
```

`-StartDelaySeconds` 只协调外部测试输入与注入时序，不修改 Authority、lead、timeout 或 world 采样语义；不使用会穿门/死亡的持续单向动作。

统一入口启动一次 controller；等待 `ScriptComplete`、最终 accepted target 的 effective B availability，并再观察一个已提交 Authority boundary 后，只通过 controller stdin 发送一次正常 `stop`。分析器随后读取同一 run 的 `events.jsonl`、`world/captures.jsonl`、`result.json`、`restoration.json` 和进程健康结果，输出：

- `report/result.json`、`checks.jsonl`；
- `report/gaps.json`：源停发、target 缺口、effective 缺失、lead 对齐和队列消费；
- `report/a-world-observation.json`：A 每段物理输入与 world 位置变化，B 位置仅作透明记录；
- `report/runtime-observations.json`：配置、boundary kind、post-state、sequence/generation、timeout 和退休计数；
- `sources/`、`build/`、`invocation.json`、`orchestrator.json`、controller stdout 与完整原始事件。

结果必须区分实测、沿用的生产语义和未验证范围。自动重试固定为 false；失败目录原样保留，只有实质修改后另立推进与新的 artifact。

## 5. 不宣称

- 不声称真实网络 loss/断线、wire ACK、重连、网络线程/收包或超时产品策略。
- 不声称全部角色、武器、死亡、房间切换、loading、restore/Replace、OOM/EH/worker 并发组合。
- 本操作自身不单独证明 snapshot apply、rollback、late join、Dedicated Server 或所有角色/生命周期组合；用户于2026-10-20基于操作1–8记录与本操作9结果确认 **T0-07整体PASS**，该阶段状态不扩大到这些未运行范围。
- 不计算/校验任何哈希，不以旧 gate/unlock 字段作为本次推进条件。
