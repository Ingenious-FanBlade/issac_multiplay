# tx-04-07 E2E 结果

> **当前状态（2026-10-26，Asia/Shanghai）：tx-04-07 完成，用户正式接受（D31）**。用户指示“正式视为此e2e测试全部通过”，将 e2e-11 整体 E2E（V1–V16）正式记为验收通过。原始自动报告仍为 FAIL，S17 停止及 S18–S21 未运行、V12 实测 PASS 等事实保持原样，不再补测、不重归约。见[正式验收记录](../../artifacts/repentogon/tx-04-07-e2e-11/acceptance.md)。下方各次结论均保留其原时点。

> 最新：2026-10-26（Asia/Shanghai），暂停等待修复后的 e2e-06 **FAIL**。S0–S15 已执行，S6 回归修复且 V9/V11 通过；第二轮阿萨泻勒身份与动画正常观测。S16 已发起 Bye/主机退休，但 relay receive 报10054，收尾未完成；S17–S21 未执行。V12 缺少所需激光证据，NOT_OBSERVED。正常退出、无新 dump、环境已清理，无自动重跑。六次原始结果分别保留，详见下文。

日期：2026-10-25，America/Los_Angeles。此前编码记录使用当时的 Asia/Shanghai 日期，保留原记录。

结果：**FAIL**。本次仅执行一次统一入口、仅启动一次游戏，没有重跑、重新归约、修复代码或回滚存档。

入口：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-01
```

使用 tx-04-07 3.4 最终纯构建（`tx-04-07-harness/build-04.log`）。启动前说明键盘接管及停止操作时机，用户明确回复“已准备好，开始测试”后才执行。SkipIntro=1，无人工跳动画；按 D20 在本次运行外层临时禁用 Download Helper，并在 finally 中恢复。

原始[报告](../../artifacts/repentogon/tx-04-07-e2e-01/report.json)、[报告摘要](../../artifacts/repentogon/tx-04-07-e2e-01/report.md)、[控制口记录](../../artifacts/repentogon/tx-04-07-e2e-01/control.jsonl)、[原始 trace](../../artifacts/repentogon/tx-04-07-e2e-01/enabled/trace.jsonl)和[准备/恢复记录](../../artifacts/repentogon/tx-04-07-e2e-01/preparation.json)均保留。

## 实际进度与首个阻断

| 范围 | 直接观察 |
| --- | --- |
| S0 | 环境、默认键位、部署与单次启动完成；已选择第二档并发送新局按键 |
| S1 | 菜单中 Lua 发起 Host；进入 Hosting |
| S2 | 经 clean relay 握手，进入 Warmup；远端 establishments=1 |
| 首轮开局 | run_active=1、players=1，但 completed_tick=0；K=Empty、join_creations=0、join_passes=0，尚未 Join B |
| 终止 | loop=553 因 trace_event_lost 失败，RuntimeFailure → Retiring → Idle；远端收到原因 6（InternalFailure）的 Bye |
| 后续 | S3.end 未到达，S4–S21 未执行；没有故障注入、死亡/复活、换层、跨房射击或 Continue 用例结果 |

自动报告的场景字段是 S0/S1/S2 COMPLETE、S3–S21 NOT_RUN。开局操作发生在共用 enter_run helper 内、S3.end 之前，因此 S3 的 NOT_RUN 不能解释为完全没有发送开局输入；本轮已实际进入原生 run，尚未完成 S3 检查点。

自动报告：V1 NOT_OBSERVED，V4 FAIL，其余 V2/V3/V5–V16 NOT_RUN。报告原样保留。事件丢失已实际违反 V16 的 trace_dropped=0 目标；自动 V16 仍为 NOT_RUN，是提前退出后缺少 final_status 的归约结果，不能据此说未发现日志错误。

## 日志丢失证据

shutdown（seq=7423）记录 `trace_dropped=63 trace_truncated=0`。四处缺号均在菜单 Host（seq=3251）之前：

| 缺号区间 | 数量 | 下一条实际记录 |
| --- | --- | --- |
| 1621–1637 | 17 | pgd_cloud return，loop=1，session_generation=0 |
| 2158–2170 | 13 | 同上 |
| 2686–2701 | 16 | 同上 |
| 3214–3230 | 17 | 同上 |

本次原始 trace 中 persistent=5307 条、loading_owner=1104 条、persist_window=553 条。菜单期间出现大量 `first_menu_start` 的 continue 窗口 reopen。结合源码的 512 项 trace 队列，这些高频观测是后续优先排查方向；确切拥塞/写失败触发及上游调用原因仍需后续代码分析，不把本轮关联直接当作唯一原因。

`game/tick.cpp` 的 RuntimeHealthy 在之后进入受控模拟时发现既有 trace 丢失并调用 FailRuntime；首轮不是网络超时或协议错误造成退出。原始 net stop（seq=7279）：stopped=1、InternalFailure、rx=36、tx=517，foreign/rx_drop/tx_drop=0，Bye 尝试和成功均为 3。

严格 trace 解析因 `trace sequence gap` 拒绝整份事件集合。因此自动 V4 中 host=null；这不是原始 trace 缺少 net stop。原包账本中 Remote tx=36/rx=517，relay 两方向分别收到并成功发送 36/517 份，均无 policy_drop/send_error/pending/shutdown_drop；这些是本次提前失败时的局部事实，不构成四轮 V4 通过。

另外，loading end（seq=7282）的 queue/generation/session_generation 文本与控制口快照矛盾。保存的 `lifecycle.cpp` 在 `%llu` 位置直接传入 Win32 `size_t` 的 Depth()，存在可变参数宽度不匹配；该条后续字段不能作为可靠代次证据。这不是本轮首个阻断，未在测试后修改代码。

## 退出、清理与持久结果

- 失败收尾的 WM_CLOSE 正常返回，退出码 0；没有新增崩溃转储，crash_changes=[]。
- 网络线程已停止；未创建 K 或 B，不能声称验证了 K/B 退休。
- 测试 mod 已移除，C/S 配置与控制口 enabled=false，未残留 isaac-ng 进程。
- Download Helper 的 disable.it 在 18:11:46.567Z 创建、18:12:10.673Z 删除，恢复原先启用状态。原始 UTC 时间见 preparation.json。
- 存档首尾快照与 save-changes.json 已生成。当前记录中的变化均在现行 D18/D20/D27 规则内：第二档及其 sidecar 按已有许可；第一/三档的两份云端 PGD 只改历史关闭配置样本允许的末尾 Counter/Checksum 字段。没有计算哈希；每日备份排除判定，原快照保留。此结论不补足本轮缺失的完整持久窗口验收。
- 文件监听正常结束，file_monitor_error=null；由于未到达 S10 等场景，未生成这些动作的窗口快照。

本次 E2E 已结束。后续如修复日志容量/事件频率、加载诊断格式或提前失败归约，须作为实质修改说明范围；不能自动再次启动本次验收。

## 实质修复后的第二次 E2E（2026-10-25）

用户明确要求“修复该问题，然后继续测试”。[修复记录](tx-04-07-trace-fix.md)对应启动 trace 队列/唤醒、ExecuteStartGame 空调用观测、Win32 loading end 格式与提前失败 status 保留；完成统一纯构建后，用户回复“已准备好，构建成功后开始”，才启动本次键盘接管。仅执行一次以下入口、一次游戏启动，没有自动重跑或再次执行归约：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-02
```

结果 **FAIL**，harness 退出码 1。[原始报告](../../artifacts/repentogon/tx-04-07-e2e-02/report.json)、[场景记录](../../artifacts/repentogon/tx-04-07-e2e-02/steps.jsonl)、[trace](../../artifacts/repentogon/tx-04-07-e2e-02/enabled/trace.jsonl)、[网络账本](../../artifacts/repentogon/tx-04-07-e2e-02/network-ledger.json)、[准备/恢复](../../artifacts/repentogon/tx-04-07-e2e-02/preparation.json)及崩溃资料均保留。

### 修复效果与本次进度

- S0–S12 标记 COMPLETE，S13 FAILED，S14–S21 NOT_RUN。COMPLETE 表示场景编排到达结尾，不等于相应 V 项通过。
- 本轮实际完成 K 注册和一次原生 B 创建，Join 两 pass；完成菜单 Host、握手、输入/屏蔽/故障段、B 死亡和复活、B 已加入后的换层及换层后移动。
- S13.begin 的实际 checkpoint：completed_tick=2246、players=2、A/B bound=1，B Alive、b_deaths=1、b_revives=1、join_creations=1，b_conflicts=0、context_faults=0。
- 同一 checkpoint 的 trace_capacity=4096；trace_dropped、trace_truncated、trace_queue_dropped、trace_write_errors、trace_wake_errors、trace_native_error 均为 0。严格事件解析成功，自动 entry_integrity=true。本次启动阶段未重现第一次的 63 条丢失；进程随后崩溃，不能将最后快照当作正常 shutdown 证据。
- continue 持久窗口只开/关各一次；stage_change、b_death、b_revive 窗口各开/关一次，无异常返回。loading end 记录的冻结代次/队列字段不再发生先前的 varargs 错位。
- S7 clean/fault 各覆盖 602 completed tick，约 59.991/60.005 tick/s，比例 1.00024；实际 drop/duplicate/reorder 命中 52/25/24。这里只说明本次局部观察，完整 V10 因 S17/S18 未执行仍为 NOT_RUN。

### 新的阻断：Stop 退休时游戏崩溃

S13 在原生 run 内发出 `cs stop`，随后等待 Idle。trace 的实际顺序：

| seq | 实际事件 |
| --- | --- |
| 39472 | port command `cs stop`，queued=2，frame=3861 |
| 39483 | session Active → Retiring，reason=Stop，loop=3862 |
| 39493 | retirement snapshot ok=1，B=7F17B040，host=7EC8F810，key=1，count=2 |
| 39494–39495 | K retiring / delete，object=7EC8F3B0，never=1 |
| 39496 | retirement confirm result=ready，B/key 与快照一致 |
| 39498 | 最后一条：下一次 `cs status` 入队；没有正常退休完成或 shutdown |

游戏日志最后为 `Clearing player color assignment for controllerId 1`、空 Lua stack trace 与 `Caught exception, writing minidump...`。控制连接报 os error 10054 是进程崩溃后的表象；报告 failure_cause.kind=game_crash，进程退出码 3221225477（0xC0000005）。本轮没有加载转储执行调试，确切故障指令/调用栈与对象访问原因仍为 UNKNOWN；不能仅凭最后事件把原因断言为 K 释放（never=1）或某个具体 Hook。

新增 `isaacv1.9.7.12.J273-20261025-112429-33856-31632.dmp`（31,968,276 bytes）与同名日志已复制至 artifact 的 `crash/`。原 Documents 文件保留，未删除；minidump 按现有 .gitignore 只留本地。原日志见 [crash 文本](../../artifacts/repentogon/tx-04-07-e2e-02/crash/isaacv1.9.7.12.J273-20261025-112429-33856-31632.txt)。

### 自动 V 项与局部失败

| 状态 | 项目 |
| --- | --- |
| PASS | V1、V7、V11、V15 |
| FAIL | V2、V3、V4、V5、V6、V8、V9、V14、V16 |
| NOT_RUN | V10、V12、V13 |

提前崩溃导致四轮会话、退休、最终状态、Continue 与跨房射击不完整；V14/V16 的失败包含真实崩溃，不能只解释为证据不足。另有已执行片段自身未通过的项：V6 的 S4.right/up movement=false（release_by_lead_plus_one=true），V8 的 S5 A/B 各 accepted=1、resolved=1，但 bombs_delta_one=false；V9 skip=false。原始数值保留，尚未判定其根因，不能将这些失败全部归于后续崩溃。V11 的死亡/复活、同一 K、旧 generation 拒绝及新输入移动等条件本次通过；不外推未执行场景。

### 清理与存档观察

外层 finally 已恢复 Download Helper：disable.it 在 18:23:18.828Z 创建、18:24:52.721Z 删除。测试 mod 已移除，C/S config 的 enabled/control_port.enabled 均为 false，未残留 isaac-ng 进程。报告 cleanup_error=null；崩溃使正常线程停止/Bye/K/B 退休完成证据缺失，不能把文件清理成功视为运行时退休成功。

file_notifications=true，文件监听正常完成。save_mid/save_after、各已执行动作窗口快照和[文件变化](../../artifacts/repentogon/tx-04-07-e2e-02/save-changes.json)均保留，未回滚、未改只读位、未计算哈希。两份新崩溃文件被原规则标为 allowed=false，其余本次 save_changes 均标为 allowed=true：第二档及其 sidecar 按已有许可；第一/三档云 PGD 仍仅为历史关闭配置样本允许的末尾 Counter/Checksum 字段。崩溃文件不加入新白名单；最终 V16 保持 FAIL，完整持久与远端云提交结论仍未完成。

本轮修复后的唯一整体 E2E 已结束，未在结果产生后修改功能代码或再启动游戏。后续首要排查 S13 的退休崩溃，并保留上述已执行片段失败；下一次实质修改需重新说明范围与每次实机接管时机。

## 退休修复后的第三次 E2E（2026-10-25）

用户再次明确要求“修复该问题，然后继续测试”。本轮只读分析 e2e-02 转储，定位到 RemoveCoPlayer RVA 0x590CE5：EBX=this 等于全局 Manager，而函数实际要求 Game 内嵌的 PlayerManager。完成[实质修复](tx-04-07-retirement-fix.md)与最终统一纯构建 build-02 后，用户回复“已准备好，构建成功后开始”，才启动本次接管。没有在分析/修改中运行测试，没有自动重跑。

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-03
```

本轮仅一次游戏启动，结果 **FAIL**，harness 退出码 1，failure=`b_animation_timeout`。[原始报告](../../artifacts/repentogon/tx-04-07-e2e-03/report.json)、[trace](../../artifacts/repentogon/tx-04-07-e2e-03/enabled/trace.jsonl)、[Lua 观测](../../artifacts/repentogon/tx-04-07-e2e-03/lua-observations.json)、[网络账本](../../artifacts/repentogon/tx-04-07-e2e-03/network-ledger.json)、[准备/恢复记录](../../artifacts/repentogon/tx-04-07-e2e-03/preparation.json)和源码/配置快照均保留，未重新归约。

### 直接验证的退休修复

S0–S14 全部标记 COMPLETE，S15–S21 NOT_RUN。S13 已执行原生 run 内 Stop 及回到透传的输入段；S14 已执行第二轮 Host、等待 peer 前的透传段、角色 7 的 K/B Join 与 Active 检查点。场景完成不等于所有 V 项通过。

| seq / loop | 实际事件 |
| --- | --- |
| 39849 / 3947 | retirement snapshot ok=1，Game=50AB0C10，PlayerManager=50ACC6B8，B=7F156028，A=7ECB9A08，key=1 |
| 39852 | confirm ready；PlayerManager 恰为 Game+0x1BAA8 |
| 39853 | 原生移除正常返回，join retired players=1 |
| 39856 | net stop generation=1 stopped=1 terminal=Stop，Bye 尝试/成功=3/3 |
| 39872 | Retiring → Idle，reason=Stop，core_state=Retired |
| round1.retired checkpoint | Idle、players=1、K=Removed、B unbound/None、peer_running=0、peer_stop_finished=1、cleanup_failed=0、join_state=None、join_last=Joined |

第一轮网络账本 pass=true，远端收到 Bye，terminal_cause=peer_bye、error=null。这些直接覆盖前次崩溃位置；未将未完成的四轮 V4/V14 改判通过。

失败后的收尾又实际完成第二轮 B 退休：seq=53546–53568，B=61714CA8、key=2、character=7，同一 Game/PlayerManager，原生移除后 players=1，网络 stopped=1、terminal=PeerBye，Retiring → Idle。该段是提前失败的收尾，不补造计划 S16 或 round2.retired 检查点。

### 新阻断是夹具身份误判，不是已证实的动画未结束

第二轮最后 status：session_generation=2、Active、completed_tick=904、players=2、B character=7、Alive、bound=1、ownership_confirmed=1，join_creations=1；context_faults=0、cleanup_failed=0、last_error 为空。

但 S14.active 和之后的 Lua 周期观测把 ordinal=1 写成 `present=false, resolved_ordinal=17`，没有 animation_finished 字段。从 loop=4083 到 4980 共 148 条此类 observation，覆盖 896 render。`playerView` 和 `lifeView` 用 `player.Index ~= ordinal` 提前返回，`animation_tx07` 又只等待该 view 的 animation_finished=true，15 秒后报 b_animation_timeout。

当前官方 SDK 区分 Entity 的 `_index +0x20` / GetIndex 与 Player 的 `_playerIndex +0x1610` / Lua GetPlayerIndex。第二轮新建 B 的实体 Index=17 不能作为玩家 ordinal=1 的否定证据；harness 已从 DLL 读到真实的唯一 B 与 Active。**本次观测证明夹具缺少 B 动画字段，不证明阿萨泻勒动画持续未结束。**后续须修复 ordinal 身份观测，而不能只放宽超时或硬写 animation_finished。第一次 B 恰好 Index=1 的样本不能推广到多轮新建。

本次没有在测试后修改夹具或功能代码；该缺陷、其它局部失败和未执行场景如实保留。

### 自动结果与清理

| 状态 | 项目 |
| --- | --- |
| PASS | V1、V7、V11、V15 |
| FAIL | V3、V4、V5、V6、V8、V9、V14、V16 |
| NOT_RUN | V2、V10、V12、V13 |

V14 的 round1.pass=true；四轮未完成，整体仍 FAIL。V6 的 S4.right/up movement=false、V8 的 S5 A/B accepted/resolved 各一次但 bombs_delta_one=false、V9 skip=false 再次保留；未将它们解释为动画超时的后果。S9/S12 移动为 true。V16 的 entry_integrity、no_new_dump、save_changes_allowed、file_notifications 均为 true，但 runtime=false 且未完成全部场景，整体仍 FAIL。

正常 WM_CLOSE 退出码 0，crash_changes=[]、cleanup_error=null。shutdown seq=53581：trace_dropped、trace_truncated、trace_queue_dropped、trace_write_errors、trace_wake_errors、trace_native_error 均为 0，capacity=4096。游戏已退出，无残留 isaac-ng；测试 mod 已移除，C/S 配置 enabled/control_port.enabled=false。Download Helper 的临时 disable.it 在 18:35:26.538Z 创建、18:37:29.458Z 删除，恢复原先启用状态。

首尾与窗口快照均保留，16 项 save_changes 全部按现行规则 allowed=true，没有新增崩溃文件；第一/三档云端 PGD 继续只在历史基线允许的末尾 Counter/Checksum 字段内变化。未回滚存档、未改只读位、未计算哈希。此局部文件结果不补足尚未执行的完整持久场景或远端云提交证据。

本次修复与唯一整体验收已结束。原退休崩溃本轮未重现，并有两次原生移除正常返回的直接记录；下一处明确问题是 Lua 夹具的 Entity Index/玩家 ordinal 混淆。当前没有再次启动或放宽验收条件。

## 夹具修复后的第四次 E2E（2026-10-26，Asia/Shanghai）

用户要求“现在修复该问题，然后继续测试”。完成[玩家槽、事件 loop 与投弹资源窗口的实质修复](tx-04-07-fixture-fix.md)、统一纯构建 build-01 后，收到本次“已准备好，构建成功后开始”才接管键盘。只执行一次完整入口、一次游戏启动，原报告未重新归约，无自动重跑。

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-04
```

结果 **FAIL**，harness 退出码 1，failure=`script_timeout:S8.recovered`。S0–S7 COMPLETE，S8 FAILED，S9–S21 NOT_RUN。[原始报告](../../artifacts/repentogon/tx-04-07-e2e-04/report.json)、[trace](../../artifacts/repentogon/tx-04-07-e2e-04/enabled/trace.jsonl)、[Lua 观测](../../artifacts/repentogon/tx-04-07-e2e-04/lua-observations.json)、[步骤](../../artifacts/repentogon/tx-04-07-e2e-04/steps.jsonl)与[准备/恢复记录](../../artifacts/repentogon/tx-04-07-e2e-04/preparation.json)均保留。

### 本轮修复的直接观察

| 范围 | 实际结果 |
| --- | --- |
| 玩家身份 | S4 中 B 的 Entity Index=2、player_index=1、present=true，位置和资源字段可读；不再因实体编号不同误标缺席 |
| S4 右移 | B 从 (325.603,388.283) 到 (416.040,388.283)，movement=true，release_by_lead_plus_one=true |
| S4 上移 | B 从 (418.536,388.283) 到 (424.570,297.846)，movement=true，release_by_lead_plus_one=true |
| S5 A 投弹 | 独立窗口库存 2→1，Accepted=1、Resolve=1，consumer 查询与命令一致，通过 |
| S5 B 投弹 | 独立窗口库存 1→0，Accepted=1、Resolve=1，consumer 查询与命令一致，通过；同一库存两份视图同步变化直接说明不能用共同 2→0 差值分别判断减一 |
| S6 skip | request loop=1085、render=532；arrived loop=1145、render=592；game_frame 均为 175，skipped=30，skip=true，无区间内 StandardRoom tick |
| S6 屏蔽与命令桥 | console/pause 的局部检查通过，Lua/console/port 三种来源检查均通过 |

完整 V6/V9 还包含未执行的 S9/S12/S20 等场景，因此本次仍为 FAIL；不能只凭这些局部通过宣布整项通过。第二轮未执行，故本次没有对阿萨泻勒动画/跨房激光作运行时结论；身份修复在第一轮 Entity Index≠ordinal 的实际对象上已生效。

### 阻断时序：故障段失焦、暂停、恢复脚本没有 tick

S6.end、S7.clean.begin/end、S7.begin 均为 pause_state=0、blocks=0、A/B bound=1。clean 段从 tick=484 到 1088；S7.begin 为 loop=2007、tick=1090。

trace seq=24647：`block_interval from=0 to=1 loop=2216 console=0 pause=0 focus=0 room_transition=0 stage_transition=0`。随后的 S7.end（loop=2603）已为 pause_state=1、blocks=1、A/B bound=0，completed_tick=1298；S8.begin、S8.recovered.begin 和最后 status 都仍是该暂停状态。失败现场 [state.json](../../artifacts/repentogon/tx-04-07-e2e-04/enabled-failure/state.json)同时记录游戏窗口 foreground=false、visible=true、minimized=false。哪个外部窗口/事件造成失焦仍 UNKNOWN，没有把原因归给用户操作。

最后 status 为 loop=4485、Active、run_active=1、players=2、B Alive，但 A/B unbound、pause_state=1、blocks=1、completed_tick=1298；peer 仍 Established/running，net_rx=1465，网络没有进入 PeerTimeout。远端 S8.recovered 的等待依赖 Authority tick，暂停下不会完成，30 秒后脚本超时。这不是本次观察到的网络中断退休，也不是已经证实的普通模拟卡死；失焦/暂停下保持中性和不推进 tick 符合现有约定。

S7 的 drop/duplicate/reorder 实际命中 38/19/16；fault 窗口包含该阻断，active=false，rate_pass=false。已有非阻断片段的比例约 0.967 不能把整个预设 Active 窗口改判通过。后续需要处理实机焦点维护/丢失报告与场景等待，不在本次失败后通过解除暂停、延长或重跑来补结果。

### 自动结果与退出

| 状态 | 项目 |
| --- | --- |
| PASS | V1、V7、V8、V15 |
| FAIL | V3、V4、V5、V6、V9、V10、V14、V16 |
| NOT_RUN | V2、V11、V12、V13 |

V11 在 e2e-03 的通过仍为该次历史结果，本次未到达死亡/复活场景，不能合并为本次完整通过。第一轮失败收尾有正常退休：seq=29373/29376 记录当前 B、Game=5094FF90、PlayerManager=5096BA38；net stop seq=29379 stopped=1、terminal=PeerBye、foreign/rx_drop/tx_drop=0；seq=29395 回到 Idle。计划 S13 未执行，不补造其检查点。

正常关闭退出码 0，crash_changes=[]、cleanup_error=null。shutdown seq=29408：trace_dropped、trace_truncated、trace_queue_dropped、trace_write_errors、trace_wake_errors、trace_native_error 全为 0，capacity=4096。测试 mod 已移除，C/S enabled/control_port.enabled=false，无残留游戏进程，Download Helper 已恢复。准备记录使用 UTC：18:54:25.109Z 临时禁用、18:55:53.669Z 恢复，对应 Asia/Shanghai 2026-10-26 02:54:25–02:55:53。

16 项 save_changes 全部按现行规则 allowed=true，无新崩溃文件；首尾和已执行窗口快照均保留。第一/三档云 PGD 仍仅变化于历史允许的末尾 Counter/Checksum 字段。未回滚、未翻只读位、未计算哈希，未扩大任何白名单。完整持久场景、第二至四轮和后续未执行项仍待完成。

本轮唯一整体 E2E 已结束，测试后没有修改功能/夹具代码或再启动游戏。

## 焦点维护修改后的第五次 E2E（2026-10-26，Asia/Shanghai）

用户在 e2e-04 结果整理期间要求继续刚才的工作并继续测试。本轮完成[焦点维护与意外暂停诊断](tx-04-07-focus-fix.md)，统一纯构建成功；用户回复本次“已准备好，构建成功后开始”后，只执行一次完整入口、一次游戏启动。原始报告不重新归约，无自动重跑。

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-05
```

整体 **FAIL**，harness 退出码1，failure=`unexpected_pause_in_running_scene`。S0–S5 COMPLETE，S6 FAILED，S7–S21 NOT_RUN。[原始报告](../../artifacts/repentogon/tx-04-07-e2e-05/report.json)、[控制记录](../../artifacts/repentogon/tx-04-07-e2e-05/control.jsonl)、[步骤](../../artifacts/repentogon/tx-04-07-e2e-05/steps.jsonl)、[trace](../../artifacts/repentogon/tx-04-07-e2e-05/enabled/trace.jsonl)、[准备/恢复记录](../../artifacts/repentogon/tx-04-07-e2e-05/preparation.json)与源码快照完整保留。

### 本次是新增检查过早，不能归为游戏故障

| 检查点/事件 | 实际状态 |
| --- | --- |
| S6.console.end | loop=1266，tick=446，pause=0、blocks=1（console） |
| console 关闭 | trace seq=12197、loop=1270，blocks 1→0、focus=1、pause=0 |
| 计划内打开暂停 | seq=12339、loop=1280，blocks 0→1、focus=1、pause=1 |
| S6.pause.begin/end | loop=1285/1348，tick=456、pause=1，符合计划内暂停 |
| 主动关闭暂停后的 stable 轮询 | unexpected_pause 步骤在 loop=1356 读到 pause=1、selection=1、blocks=1、A/B unbound、tick=456，立即终止 |

`stable_tx07` 原本会等待原生退出阻断与重新绑定；本轮将其观察改为 watch_running_tx07，但没有为 S6 的主动关闭暂停区分预期退出过程。该 helper 只有刚发生 focus_restore 才有短暂同步等待，普通计划内退出读到 pause=1 就直接失败。**这是本轮引入的 harness 回归，不能把该次 status 当作游戏暂停永远不能退出的证据。**测试结束后没有继续运行以验证它是否会自然完成，也没有修改代码补跑。

control.jsonl 中 focus_lost/focus_restore 均为0条，相关 block_interval 均为 focus=1。因此本轮既没有重现 e2e-04 的失焦，也没有实际覆盖焦点重新激活分支；只能确认代码已部署，不能宣称实机焦点恢复通过。后续须将“允许预期阻断退出的稳定等待”与“必须始终 Active 的固定故障窗口”分开，不能直接删除后者或自动发 ESC 掩盖未知暂停。

### 其它局部结果与退出

自动 PASS：V1、V7、V8、V15；FAIL：V3、V4、V5、V6、V9、V14、V16；NOT_RUN：V2、V10、V11、V12、V13。S4 两方向 movement/release 再次通过；S5 A/B 各一个 Accepted/Resolve，独立库存分别2→1、1→0，V8 PASS；skip=true。完整项仍依赖未执行场景，历史通过不并入本轮整体验收。

失败收尾正常退休并退出：net stop seq=12498 stopped=1、terminal=PeerBye、foreign/rx_drop/tx_drop=0；seq=12514 Retiring→Idle。正常关闭退出码0，crash_changes=[]、cleanup_error=null；shutdown seq=12527 的 trace_dropped/truncated/queue_dropped/write_errors/wake_errors/native_error 均为0，capacity=4096。

测试 mod 已移除、C/S enabled/control_port.enabled=false，无残留游戏进程；Download Helper disable.it 在 UTC 04:33:12.822Z 创建、04:33:49.094Z 删除，对应 Asia/Shanghai 2026-10-26 12:33:12–12:33:49。16项 save_changes 按现行规则全部allowed=true，无新转储；首尾和窗口快照保留，未回滚、未翻只读位、未计算哈希或扩大白名单。后续场景仍未执行。

## 暂停等待修复后的第六次 E2E（2026-10-26，Asia/Shanghai）

用户要求“修复，然后继续测试”。本轮[实质修改](tx-04-07-pause-wait-fix.md)将稳定绑定的预期阻断退出等待与固定 Active 窗口检查分开，完成纯构建；用户本次回复“已准备好，构建成功后开始”后，仅运行一次完整入口、一次游戏启动，无自动重跑或重新归约。

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-06
```

整体 **FAIL**，harness 退出码1，failure=`relay_receive:远程主机强迫关闭了一个现有的连接。 (os error 10054)`。自动场景为 S0–S15 COMPLETE、S16–S21 NOT_RUN；S16 已实际执行 remote Bye 和主机退休，但 relay.finish 失败，未到达 S16 完成标记，不能解释为完全没有触发该操作。

原始[报告](../../artifacts/repentogon/tx-04-07-e2e-06/report.json)、[控制记录](../../artifacts/repentogon/tx-04-07-e2e-06/control.jsonl)、[trace](../../artifacts/repentogon/tx-04-07-e2e-06/enabled/trace.jsonl)、[Lua 观测](../../artifacts/repentogon/tx-04-07-e2e-06/lua-observations.json)、[第二轮中继](../../artifacts/repentogon/tx-04-07-e2e-06/relay-2.jsonl)、[远端](../../artifacts/repentogon/tx-04-07-e2e-06/remote-2.jsonl)与源码/场景/存档窗口均保留。

### 本轮修复与已覆盖片段

- S6.pause.end 为 loop=1345、pause=1、tick=453；S6.end 已到 loop=1398、pause=0、blocks=0、A/B bound=1、tick=480。旧过早失败未重现，原生暂停退出与重绑实际完成。V9 的 skip、console/pause、中性、三种命令来源及房间切换检查均通过，整项 PASS。
- S4 right/up、S9/S12 move 的真实移动与释放检查通过；V6 整项仍因 S20 等后续要求未完成而 FAIL。V8 再次 PASS，A/B 各一次 Accepted/Resolve，独立资源差值2→1、1→0。
- S7 clean/fault 各有602 completed tick，均 active=true，推进率比例约1.000018、rate_pass=true；drop/duplicate/reorder 命中50/25/23，non_waiting_input_invariants=true。V10 还需 S17/S18，整项仍 NOT_RUN，不能将局部通过当作完整四种故障通过。
- V11 死亡/复活 PASS。S13 第一轮正常 Stop，net stop seq=40656 stopped=1、Bye3/3，seq=40672 Retiring→Idle；round1.retired 的 K移出、players=1、peer停止等检查通过。
- S14 第二轮角色7实际 Join并进入Active。128条阿萨泻勒观测中，首条为 present=true、Entity Index=46、player_index=1、animation_finished=true；旧 Entity Index/ordinal 误判未重现，动画等待已通过。没有把实体编号作为网络身份。
- control.jsonl 的 focus_lost/focus_restore 仍均0条：本轮没有失焦恢复样本，不能外推焦点恢复能力已实机验证。

### 第二轮 Bye 后 relay 收尾错误

S15.end 为 session_generation=2、Active、tick=183、pause=0、blocks=0、players=2、B character=7/bound=1。随后远端主动 Bye，remote-2 summary 为 terminal_cause=local_bye、establishments=1、error=null、tx=191、rx=206；报告已保存 remote_rounds[2]。

主机 trace seq=44048：generation=2、stopped=1、terminal=PeerBye、rx=189、tx=206、foreign/rx_drop/tx_drop=0、Bye尝试/发送=0；seq=44065 在 loop=4261 回到 Idle，players=1。正常 PeerBye 退休的主机局部行为已经观察，尚不能把缺失的计划退休检查点补造为PASS。

relay-2 在 elapsed_us=3445321 记录 receive error10054并退出，summary保留相同错误；两方向 received/send_ok 分别191/191、206/206，policy_drop、send_error、pending、shutdown_drop均0，foreign=0。relay.finish 因此返回错误，relay_rounds[2]未写入，round2.retired检查点未到达，第三轮未启动。**这次10054来自UDP中继接收，不是游戏崩溃或控制TCP断开。**具体终端关闭/迟到包/系统错误时序仍需后续静态排查，不能仅凭错误字符串断言某个未捕获的ICMP来源，也不改判三方对账通过。

### S15 执行完成但 V12 仍未观察齐全

V12 为 **NOT_OBSERVED**：armed_new_source=true，但 fresh_held_continuity=false，before_release=[]、after_release=[]、unknown_attribution=0，release_window_ticks=30。没有记录到满足判定所需的新激光，零未知归属不是激光发射成功的证据。S15 场景编排到达结尾不能代表跨房射击通过；后续还需核对该段输入/consumer/激光观测链，不能只处理UDP收尾就宣布目标完成。

### 自动结果、退出与持久观察

| 状态 | 项目 |
| --- | --- |
| PASS | V1、V7、V8、V9、V11、V15 |
| FAIL | V3、V4、V5、V6、V14、V16 |
| NOT_RUN | V2、V10、V13 |
| NOT_OBSERVED | V12 |

S17丢包握手、S18长断网超时、S19保存、S20 Continue和S21 teardown均未执行；完整四轮状态/网络/Join/输入/退休与加载持久证据仍未完成。历史结果不并入本次完整通过。

正常WM_CLOSE退出码0，crash_changes=[]、cleanup_error=null；shutdown seq=44101 的trace_dropped/truncated/queue_dropped/write_errors/wake_errors/native_error均为0，capacity=4096。测试mod已移除、C/S enabled/control_port.enabled=false，无残留游戏进程，Download Helper恢复。准备记录UTC为04:41:50.171Z禁用、04:43:32.753Z恢复，对应Asia/Shanghai 2026-10-26 12:41:50–12:43:32。

16项save_changes按现行D18/D20/D27均allowed=true，无新转储；首尾及已执行窗口快照保留，未回滚、未翻只读位、未计算哈希或扩大白名单。本轮测试结束后未修改功能/夹具代码或再启动游戏。

## 中继与 V12 修复后第七次入口：启动前中止（2026-10-26，Asia/Shanghai）

用户要求继续测试，并回复本次“已准备好，开始测试”后，执行一次完整 E2E 入口，产物为 [e2e-07](../../artifacts/repentogon/tx-04-07-e2e-07/README.md)。退出码 1，失败于 input_environment_tx05：启用的 repentogonreleasesbackup_3643104060 被现有第三方 mod 检查拒绝。静态读取 metadata 已确认它是前几轮按 D20 临时禁用并恢复的 REPENTOGON Download Helper；本轮操作遗漏这一准备步骤。未扩大允许列表或修改游戏目录。

未执行部署、存档快照或游戏启动，S0–S21 / V1–V16 全部 NOT_RUN。中继 UDP 通知分类、100 ms 终止收包与 V12 对照/时长/证据边界修复仍仅有纯构建结果，没有新的实机证据。失败收尾生成了空证据 JSON，不能据文件存在称场景已运行。

原始 report 的 failure_cause=game_crash 是另一项归约错误：crash_before 在本轮失败点之后才采集，实际尚未建立基线；finish_tx05 却仍扫描比较全部现存 dump，误将 12 个历史 dump 记为变化。未启动游戏，不能宣称本轮崩溃。原报告保留，目录 README 明确更正解释。

用户已获知可以恢复键鼠；本轮没有部署或环境修改需要回收。没有自动重跑、重新归约或测试后功能修改。后续准备需补齐 Download Helper 临时禁用/恢复，并修复无 crash_before 基线的崩溃误分类，再按现行规范安排完整验收。

## 中继与 V12 修复后第八次 E2E（2026-10-26，Asia/Shanghai）

用户再次要求继续测试，并确认本次键鼠接管准备。按已授权 D20 临时禁用 Download Helper、finally 恢复原状态，执行一次完整入口、一次游戏启动。没有自动重跑、重新归约或存档回滚。完整原始报告、四轮网络日志、Lua、trace、源码与文件窗口见 [e2e-08](../../artifacts/repentogon/tx-04-07-e2e-08/README.md)。准备窗口为 Asia/Shanghai 13:20:39.539–13:23:20.779。

整体 **FAIL**：S0–S19 COMPLETE，S20 FAILED，S21 NOT_RUN；failure 为 menu_game_selection=1 超时，实际仍为 0。游戏正常关闭退出码 0，无新 dump；cleanup_error、collection_error、evaluation_error 均为空。

### 中继收尾与网络

S16 已完成，前次 relay receive 10054 中断未重现。第二轮主机 stop seq=51128：PeerBye、stopped=1、rx=669、tx=683、terminal_rx=2；两份迟到包实际进入新的终止收包窗口。该轮 relay 两方向 received/send_ok 分别 669/669、683/683，严格三方对账通过。前三轮 network-ledger 均 PASS。第二轮 udp_notices=[0,0]，本次没有直接覆盖 10054/10052 通知分类分支。

第四轮在 S20 失败后清理，不具备计划内 S21 退休证据，主机到远端有 1 包缺口；V4 整项仍 FAIL，不放宽严格计数。S17–S19 虽编排完成，不据此宣称所有依赖场景的用例均通过。

### V12 新证据与归约边界问题

普通松键对照出现 1 条归属 B 的激光、control.pass=true，说明本次角色发射与 Lua 新激光观测链均有正证据。预置按住实际 182 tick / 3.017258 秒，跨房后 held 193 tick / 3.200348 秒，两段时长通过。重绑后的 tick 432–434 仍为 input_sequence=0、held/aim=0、shooting_suppressed=0，真实输入连续性失败。

原始 laser-evidence 记录 before_release=2、after_release=0，V12 FAIL。但回读 ops::Execute 和 FrameEnd 确认：Lua currentLoop 每次读取的 cs status 仍是帧尾缓存，模拟回调会取得上一轮 loop。第一条激光 Lua loop=4491 对应相邻 native loop=4492 的 Shooting/查询、tick=432，明显早于计划松键，支持缺输入中性造成提前发射。第二条 Lua loop=4687 紧邻真正松键 tick=628、loop=4688，native seq=50514/50515 位于该次 Shooting/查询；它可能就是正常松键产生的激光，不能直接宣称另一次提前发射或松键后完全未发射。

原报告不修改、不重归约。V12 仍有独立的提前发射与三个输入空档证据，因此不能改判 PASS；下一次修改需要解决跨来源 loop 的时间归属，不能把缓存值当实时 Authority loop。

### S20 的实际原因

Lua 首次 A 死亡观测在 loop=4993，发生于第三轮长断网段，hearts=0/dead=true；S18.end/S19.begin 均已死亡。退出回调 should_save=false、A 仍 dead=true，失败截图 Continue 灰色。S20 没有可继续的 run，并非仅凭选择超时推断按键没有生效。具体伤害来源未取证。

用户随后指出“下次测试建议先在开局控制台输入命令debug 10”。已按其指示增加下一次 harness 的单次原生控制台准备，保存截图/步骤并等待关闭后恢复；仅纯构建成功，未运行新测试。见 [debug10 修改记录](../../artifacts/repentogon/tx-04-07-debug10-preparation/README.md)。e2e-08 实际受测源码不包含这项后续修改，不将它算成本轮已验证行为。

### 其它结果与退出

PASS：V1、V7、V8、V9、V11、V15。FAIL：V2、V3、V4、V5、V6、V12、V13、V14、V16。NOT_OBSERVED：V10。V10 的 S7 推进率/非等待输入和 S8 局部检查通过；S17 缺 Hello retry/重复握手样本。S18 主机/远端超时年龄为 3000/3004 ms，实际 relay-3 丢弃了三份 kind=5 的 Bye；归约却筛选 kind=4，误报数量 0，该缺陷已记录但本次未改结果。

shutdown seq=58625，trace 丢失/截断/队列丢弃/写入/唤醒/native_error 全部为 0。16 项存档变化 allowed=true；不回滚、不翻只读位、不计算哈希。测试 mod 已移除、C/S enabled/control_port.enabled=false、无残留游戏进程、Download Helper 恢复，已告知用户可恢复键鼠。没有焦点丢失/恢复样本，焦点恢复分支仍不能宣称实测通过。

## 开局 debug 10 后第九次 E2E（2026-10-26，Asia/Shanghai）

用户要求继续测试，并确认本次停止键鼠操作。使用已编译的开局 debug10 harness、临时禁用并恢复 Download Helper，执行一次完整入口、一次游戏启动，产物 [e2e-09](../../artifacts/repentogon/tx-04-07-e2e-09/README.md)。无自动重跑、重新归约或存档回滚。

整体 **FAIL**：S0–S20 COMPLETE，S21 FAILED；最终 failure=`pause navigation timeout selection=None`。S21 初始状态为 Active、run_active=1、players=2、pause=0；发出一次 ESC 后未观察到暂停，超时末仍 pause_state=0、blocks=0，completed_tick=420。没有补按或跳过，具体原因 UNKNOWN。游戏正常 WM_CLOSE 退出码0，无新 dump，cleanup/collection/evaluation_error 均为空。

### debug 10 与 Continue 实际覆盖

startup-debug10-console.bmp 显示 `>debug 10`、`Enabled debug flag.`；首轮只提交一次，关闭控制台后继续。报告 native_flag_verified=false 是未直接读取 flag 的标记，实际控制台响应另有截图。S19 run-exit 的 should_save=true，A dead=false、hearts=4、controller=0；本次没有重现上一轮死亡导致 Continue 灰色。

S20_continue_selected 已选择 Continue。S20.end 为 loop=5575、completed_tick=31、Active、players=2、blocks=0；恢复后的 B 创建/绑定和移动编排执行完成。V2/V5 本次 PASS，factory_once_per_round=true。V13 的 continue_preserved、invariants_pass、save_load_topology 均 true，但 coverage_complete=false，整项 NOT_OBSERVED，不靠强制补调用填自然重复查询覆盖。

### 中继、V12 与其余结果

第二轮 stop seq=51125 为正常 PeerBye，rx=673、tx=689、terminal_rx=2；前三轮严格 network-ledger 均 PASS。第四轮双方向包数差也均为0，但没有计划内 S21 RunTeardown/retired 终态，所以 V4 整项仍 FAIL；失败清理的 PeerBye 不能代替计划内退出验收。

V12 正对照 PASS，预置按住185 tick/3.067863秒、跨房后191 tick/3.167168秒均通过；重绑后的 tick=439、loop=4572、seq=47613 实际缺输入（sequence=0、held/aim=0、shooting_suppressed=0），连续性失败。原激光证据 before_release=2、after_release=0。e2e-08 已发现的 Lua status_snapshot/FrameEnd 一轮偏移仍未修复，因此松键附近激光的前后归类仍有限制；跨房早期激光与真实输入空档仍独立支持 FAIL，原报告不重归约。

PASS：V1、V2、V5、V7、V8、V9、V11、V15。FAIL：V3、V4、V6、V12、V14、V16。NOT_OBSERVED：V10、V13。V6 all_non_neutral_matches_accepted=true，但 startup_coverage=false。V10 既有 Bye kind 筛选错误及握手样本不足仍保留，不能将本轮编排完成冒充该整项通过。

shutdown seq=64074 的 trace 丢失/截断/队列丢弃/写入/唤醒/native_error 均0，16项存档变化 allowed=true；不回滚、不翻只读位、不计算哈希。测试夹具已移除、C/S/control_port 禁用、无残留游戏进程，Download Helper 已恢复；准备窗口 Asia/Shanghai 13:39:19.964–13:42:04.963。control 无实际失焦/恢复样本。已通知用户可恢复键鼠，测试后未修改功能代码或再次启动。

## V12 恢复与实时采证修复后的第十次 E2E（2026-10-26，Asia/Shanghai）

用户要求检查 V12 原因、修复并重跑，并明确采纳最多六个 Authority tick 的 Shooting 恢复窗口。完成恢复逻辑、实时 cs witness/Lua/激光归属修复与统一纯构建后，用户确认本次接管准备，执行一次完整入口、一次游戏启动，产物 [e2e-10](../../artifacts/repentogon/tx-04-07-e2e-10/README.md)。实现与规则变更见 [说明](tx-04-07-v12-recovery-fix.md)。无局部/单元测试、旧记录重归约或自动重跑。

整体 **FAIL**：S0–S8 COMPLETE，S9 FAILED，S10–S21 NOT_RUN；最终错误 no_observed_free_direction。S9 room 动作从 stage1/type2/index84 到达 index97、target_matched=true、fixture_error=false。最后一条双玩家实时观察 loop=2833 中，B 存在、未死亡、动画结束，位置约(317.3926,158.5162)，但 movement_options=[]；direction_tx07 立即返回错误。没有补发动作、传送、修改碰撞或等待试探该状态是否会自然消失。

本次 **V12 NOT_RUN**，尚未到达第二轮角色7或S15，不能宣称跨房蓄力修复已实测通过。523条Lua记录带 witness_version=1、loop与witness.frame一致；native有7次B的startup_deferred=1，original=0、sequence=0、limit=6，分别覆盖初始与后续重绑的短暂恢复，Authority继续推进。该局部观察不证明到期分支、A实际暂停分支或完整激光行为。

PASS：V1/V7/V8/V15。FAIL：V3/V4/V5/V6/V9/V14/V16。NOT_RUN：V2/V10/V11/V12/V13。原始报告和证据保留，不把历史成功并入本轮，也不把空证据JSON的存在当成场景已执行。

正常WM_CLOSE退出码0，无新dump，cleanup/collection/evaluation_error均为空。shutdown seq=32116，trace丢失/截断/队列丢弃/写入/唤醒/native_error均0；16项存档变化allowed=true。不回滚、不翻只读位、不计算哈希。测试mod移除、C/S/control_port禁用、无残留游戏进程，Download Helper恢复，临时禁用窗口Asia/Shanghai 13:58:50.069–13:59:52.872。用户已获知可恢复键鼠。测试后未修改功能代码、再启动或重跑；后续需先处理S9方向观察阻断，再安排完整验收。

## 净空选择修正后的第十一次 E2E：V12 PASS（2026-10-26，Asia/Shanghai）

用户要求继续测试；先修正过度要求80像素整段畅通的路径预览，记录四方向连续净空、优先长路径、短路径相应缩短输入段，并给S15等待补足六tick恢复余量。Lua夹具与Rust编排修改完成、纯构建成功，经本次接管准备确认后执行一次完整入口、一次游戏启动。见[修正](../../artifacts/repentogon/tx-04-07-movement-clearance-fix/README.md)与[e2e-11原始结果](../../artifacts/repentogon/tx-04-07-e2e-11/README.md)。没有自动重跑、重归约或存档回滚。

**V12本次PASS；整体仍FAIL。** S0–S16 COMPLETE，S17 FAILED，S18–S21 NOT_RUN。最终错误no_observed_free_direction，发生于第三轮S17移动准备。

### V12 的完整实际证据

正对照PASS；跨房前held189 tick/3.132948秒，跨房后held197 tick/3.266893秒，均达到180tick与至少两秒要求。before_release=[]，after_release只有一条归属B的激光、未知归属0，实时witness均有效。

重绑中性空档保留：tick449/450、loop4609/4610、seq48012/48020，sequence=0、held/aim=0；每tick都有shooting_suppressed=1、shooting_startup_deferred=1及实际original=0证据。因此fresh_held_continuity=false仍如实保留，verified_startup_pauses=2、continuity_gaps=[]、effective_shooting_continuity=true，按用户明确采纳的六tick规则通过，不把中性改写成held。

计划松键为tick648、loop4808、sequence650、held/aim=0，native trace seq51042。唯一跨房后新激光laser_id=7:2的witness同为frame4808/simulation_tick648/session_generation2/b_generation2，attribution=B；早期激光为0。功能恢复暂停、首条新命令后恢复射击、真实松键发射与实时归属已在本次观察；不外推六tick到期分支、A实际暂停分支或所有武器/网络时延。

### S17 方向预览阻断及其它结果

S9/S11/S12本次均完成移动编排，但所选净空均80、held18tick，没有短通道实际样本，不能称e2e-10原房间已复测通过。S17最后双玩家观察loop4878，B存活、动画结束、player_index1/Entity Index62，位置约(848.6172,405.0739)，room index71/stage2/type2，四方向clear_distance均0。当前20像素边界/格子碰撞预览不满足条件，helper按规则停止；不能凭此断言世界绝对不可移动，具体几何/生成点原因仍待核对。未发试探移动、传送或绕过碰撞。

前两轮严格network-ledger均PASS。第三轮双方向包数差均0，但缺计划内超时/退休终态，V4整项仍FAIL；第四轮未启动。PASS：V1/V7/V8/V9/V11/V12/V15。FAIL：V3/V4/V5/V6/V10/V14/V16。NOT_RUN：V2/V13。原报告保持原样，历史通过不并入本次整体验收。

正常WM_CLOSE退出码0，无新dump，cleanup/collection/evaluation_error均为空。shutdown seq51812的trace丢失/截断/队列丢弃/写入/唤醒/native_error均0；16项存档变化allowed=true，不回滚、不翻只读位、不计算哈希。测试mod移除、C/S/control_port禁用、无残留游戏进程，Download Helper恢复，准备窗口Asia/Shanghai 14:13:35.469–14:15:42.561。control无实际失焦恢复样本。用户已获知可恢复键鼠；测试后未修改功能代码、再次启动或重跑。
