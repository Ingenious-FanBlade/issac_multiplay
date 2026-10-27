# tx-04-08：Lua 夹具与统一 E2E harness

> **后续ABC工程更新（2026-10-27）**：见[ABC实现与主审](tx-04-08-abc-implementation.md)。A/B接线和C可核定场景/episode索引已编写，V8已有实际故障证据归约；特殊Replace保留UNKNOWN，完整V9退出与F项判定等仍待后续。本文下方保留ABC续作前主体与限制的原时点，尤其“V8固定NOT_RUN”“无故障入口/具体参数”已被后续实现取代；不代表tx-04-08只待运行。

日期：2026-10-27，Asia/Singapore。依据[现行规范](../../refactor/project-conventions.md)、[迁移要求](tx-04-08-implementation-plan.md)与[E2E 方案](tx-04-08-e2e-plan.md)。本项交付夹具、统一编排、文件证据与离线用例代码；仅纯编译，不部署、不启动游戏，不运行 replay/compare、局部测试或整体 E2E。**代码入口已接通，不表示 tx-04-08 已通过，也不表示所有必需覆盖已实现。**

## 入口与执行范围

```powershell
out/repentogon/harness.exe e2e tx-04-08 --game <游戏根目录> --scenario tools/harness/scenarios/tx-04-08.json --artifact-dir <新目录>
```

保留旧 `--out` 别名；tx08 拒绝重复/未知选项、两个输出别名并用和既有产物目录。源码捕获目录内或游戏目录内的 artifact 被拒绝，防止递归复制或混入游戏环境。入口只执行一次；不会自动重启游戏、重发 F8 或重做失败场景。构建不调用入口。

`tools/harness/src/tx08.rs` 编排两个顺序进程。两者都先选第二档、进入单 A 的新局，通过 F8 冻结/核对 seed，要求 A 为普通 Isaac；world 真正 Recording 后才 Host/握手，由原生 Join 创建 B，不调用 Lua SpawnCoPlayer。第一进程的 seed 可由 scenario 指定，也可从实际新局解析；第二进程使用第一进程实际记录的 seed。每个进程有不同 Lua token，避免第二次启动接受第一次动作终态。短输入脚本不射击、不投弹，A 的实际中性输入和 B 的实际消费均保留，不能据同 seed 宣称世界确定性。

primary 的采集窗口目标为 2,000 次普通完整样本；comparison 默认 256。运行期间只用计数确定结束时机，最终用全部文件按 attempt 重新归约稳定性。计数不足只在冻结的 240 s 窗口内等候，超时不延长。world 准备/关闭 30 s、DLL close 10 s、F8 动作 65 s、每个离线 CLI 300 s，均在 scenario/调用记录中明示。

第一进程随后执行暂停/控制台、MC_PRE_UPDATE 跳帧、独立观察、已启用的世界动作、切房、B 死亡/复活、换层。primary 正常 stop 后记录 Host/Ack 继续推进，再在同一 Host 新建 healthy 录制并停止，最后正常 stop Host。保存退菜单后新建 continue 录制，覆盖首次菜单 Host、Continue 与 B 重建，然后显式 world stop、等待 Closed、正常 stop Host，最后退出。第二进程执行相同 seed/角色/脚本的短段，再逐项关闭。动作完成不代替原生生命周期或整体验收。

每次启动前分别输出中文操作说明并保存 `readiness-N.json`，等待本 artifact 中新建的 `ready-N.txt` 内容为 `READY N`；`CANCEL` 取消，等待无默认确认和超时确认。提前存在的确认文件被拒绝。第二次确认不会由第一次确认继承；当前入口持续等待时可在原检查点继续，不提供运行失败后的自动续跑。实际运行时，Codex 必须先向用户说明操作并获得准备确认，再写该次文件；本次代码编写没有创建确认文件或发起验收。

## Lua 夹具

`tests/e2e-mods/isaac-cs-test/main.lua` 复用现有 tx07 的唯一 F8 队列、生命周期动作、超时和观察功能。`world_fixture.lua` 扩展动作，不建立第二个 F8 consumer。旧 tx05/06/07 入口保留。

| 动作 | 必填参数与行为 |
| --- | --- |
| `seed` | seed 取动作/scenario/实际新局的值。仅单 A、Host 前使用控制台 seed，随后核对实际 seed；不把命令返回当作新局已到达 |
| `spawn` | 唯一 `id`、`type/variant/subtype/seed/position[2]/velocity[2]`；只用已核定的 `Game:Spawn(type,variant,position,velocity,nil,subtype,seed)`，拒绝 Player、Bomb、Laser |
| `spawn_short_lived` | 同 Spawn 的显式参数；按真实公开 owner 顺序创建后立即 Remove，记录短命对象前缀，不声称已物理销毁或池复用 |
| `remove` | 当前房间自有 `id`；一次 Remove，下一轮观察 Exists=false；不声称已析构 |
| `morph_npc` | 自有 NPC `id`、`type/variant/subtype/champion`；公开四整数 Morph，无 bool 返回值 |
| `morph_pickup` | 自有 Pickup `id`、`type=5/variant/subtype/keep_price/keep_seed/ignore_modifiers`；三项 bool 必须显式传入 |
| `grid_spawn` | `index/type/variant/seed/var_data`，要求原格为空，index 不超过当前 GridSize 和 447；检查 bool 与后态 |
| `grid_remove` | `index/expected_type/expected_variant/path_trail/keep_decoration`，核对原格后调用公开 RemoveGridEntityImmediate |
| `grid_replace` | 上述删除参数加创建参数；删除与创建分别只调用一次，失败保留原始前缀，不复原或重试 |
| `world_observe` | 只读 Entity/Grid/door/seed 视图，附 `cs witness`；标为 MC_POST_RENDER，不冒充完整采样 tick |

所有写世界的新增动作都检查实际参数类型、范围和当前对象状态，参数没有默认 RNG/类型/格子/PathTrail。句柄以本房间、InitSeed 和 Entity.Index 防止错误复用；换房/退 run 全部失效。Morph 后只更新 Lua 自有记录，不伪造 DLL use/birth。B 仍只由 C/S 原生 Join 建立。夹具不开放任意 Lua、文件、地址或原生 ABI 调用。

默认 scenario **不猜测**研究报告尚未核定的具体 Spawn/Morph/Grid 场景，相关项保留 `enabled=false` 与 UNKNOWN 原因。可在实际参数核定后填写并启用；禁用项在报告单列，不能得到覆盖 PASS。`evidence` 只是可读来源说明，不参与执行许可或项目推进判断，没有新增确认/解锁字段。

## 证据与离线用例

`tx08/evidence.rs` 对全部 attempt 建索引，按录制/run/room/mutation、连续 attempt、递增 outer、连续真实 tick 和同线程建立稳定区间。只有明确 ODD_PLAYER_ONLY、Authority kind=2、候选 tick/线程连续的 gap 可跳过，不计完整数；其他缺口均打断。checkpoint 的 attempt watermark 限定场景窗口，生命周期 event 缺号不能获得稳定段通过。

为使这些事实可复查，本项小幅扩充生产诊断：

- 无槽 `GapRecord` 使用原 padding 保存实际生产线程，大小仍 80 bytes；writer 在 gap diagnostics 中输出 thread。旧缺线程 gap 不能补造线程或自动跳过。
- `BatchHeader` 保存已有 loop 以及 owner/Room/tail 的 scope、entered、returned；writer 在 capture diagnostics 输出 `loop/phase_witness`。没有新增原生查询或额外 original 调用。头大小由 472 改为 **528 bytes**，RawBatch 由 43,516,376 改为 **43,516,432 bytes**，三槽合计多 168 bytes；计划/实际预算沿用 sizeof 计算，同一 256 MiB 上限。
- 完整 capture 必须逐一匹配真实 trace 的同 loop/scope StandardRoom pre、commit 和发布，以及三阶段正常返回。输出 `primary-phase-index.jsonl`，不以 complete 标志单独证明阶段链。

归约还输出逐类已知字段/缺字段、来源、door/grid 同对象别名、FIRST_OBSERVED、Player basis 3、生命周期/native-body 配对和真实关闭计数。证据不足的 V1/V4/V5/V6/V7/V9/V12 保持 NOT_OBSERVED/NOT_RUN 或实际 FAIL，**不会为了让入口返回 0 而降低验收要求**。

`tx08/offline.rs` 用当前可执行文件启动真实生产 `world-replay/world-compare` CLI，保存 stdout/stderr、参数、退出码、期限与产物；每个子项只执行计划中的调用。一次调用超时只终止该自有离线子进程，不能作为“预期解码拒绝”通过。独立子项的失败不阻止后续无依赖归约和清理。

| 子项 | 实现 |
| --- | --- |
| L1 | primary 全量双份 canonical replay，包括全部 gap/lifecycle/diagnostics；逐块字节与逐记录 JSON 值比较，不计算摘要 |
| L2 | 实际 capture 派生副本反转集合顺序、最大 u64、raw128、±0、finite/NaN/Inf；核对重编码原始位，比较 ±0 归一化及非有限值不可比 |
| L3 | 只修改一个已知有限 Player 浮点字段，要求生产 compare 输出 DIFFERENT 与唯一对应 field_id |
| L4 | 错 domain/version/profile、重复 source/JSON key、错宽度/长度、u64 溢出、额外字段、截断 JSON、对象预算超限；必须非零、有 failure、无成功 result |
| L5 | key 冲突保留整组；NULL/已映射/未解析/退休/复用歧义引用、NOT_READY/READ_FAILED、缺来源/缺 tick/重复 tick；相同故障副本两侧比较不算相等，并核对具体拒绝/未知原因 |
| L6 | 按双方冻结脚本的实际安装 tick 建相对偏移，只接受双方唯一完整 capture、同代次真实 A/B 消费记录与相同 canonical 输入；保存逐对依据。冻结 ordinal 映射；出生没有真实配对依据则不映射。要求实际 known_field_comparisons>0，不要求世界 equal |
| L7 | scenario 显式提供一份已有完整 legacy 目录，检查旧 domain 后使用 `--legacy-t0` 导入；默认 null 时 NOT_RUN，不从历史摘要重造材料 |

L1 绝不截取。L2–L5 使用一条真实完整 capture 的明确 `TEST_INJECTED_DERIVED_SINGLE_CAPTURE` 副本，保留完整生命周期/诊断和原始终态，派生集只调整自身行数计数；它们证明文件工具行为，不冒充原生游戏覆盖。没有真实完整 capture 时，这些项为 NOT_OBSERVED；L1、独立 L6/L7 仍尽力执行。所有原始输入保留。

## 环境与尚未覆盖事项

每次部署前分别保存 DLL、UI、配置的真实 before 与恢复 journal，分别写入 `environment-before-p1/`、`environment-before-p2/`；每个进程退出后恢复环境，部分部署失败也恢复已保存环境。部署使用本 artifact 已冻结的源码与二进制，人工等待期间不改用后来变化的工作区。只删除本次拥有的测试 mod 和 Helper 标记；恢复路径核对绝对边界，符号链接/所有权变化报错保留。两次进程各有 PID/日志/崩溃窗口、第二档及 Steam/Documents 快照、逐启动变更判定，保存/Continue 另有文件窗口与目录通知。D18/D20/D27 使用既有规则，不改 readonly、不回滚存档、不新增白名单。最终 `cleanup.json/report.json/report.md` 分别保留恢复错误、未覆盖与整体失败；强制子进程清理的请求单独记录，不能冒充正常退出。

当前仍必须如实保留：

1. 现有 collector 的 InitUnknown/UNCONFIRMED 会使批次不完整；本项不放宽字段或 2,000 次规则。默认 scenario 不能据此被称为可通过的完整验收场景。
2. DLL 目前没有 token/generation 限定的 raw reader 暂停或采集器自有故障注入命令。S7/S8/V8 代码明确报告 NOT_RUN，不向控制口发送猜测命令。故障入口实现属于后续生产接线，当前 harness 没有伪造其覆盖。
3. Player Replace/Restore 具体自然夹具路由、各新世界动作的安全具体参数和完整出生/复用 episode 仍需核定；默认禁用项不能冒充已覆盖。
4. capture 有线程和 RNG 当前原值，但没有整个复制窗口前/后的 RNG 见证，V7 不以单快照声称 RNG 不变。
5. 只编排显式 world stop→真实 Closed→退出。直接 Shutdown 的完整协调、文件失败路径仍未覆盖，V9 不据健康路径外推全部退出。
6. 缺少完整旧格式原始材料时 L7 保持 NOT_RUN；不选择残缺 replay 目录伪装完整旧录制。

上述是功能/证据的真实限制，不恢复 gate、哈希、独立角色或额外测试批次。未来真正执行整体 E2E 前须按两次启动分别告知用户并确认准备，实际动作授权以当次会话为准。

纯编译记录见[构建目录](../../../artifacts/repentogon/tx-04-08-harness-build/README.md)。全部运行判定继续 NOT_RUN。
