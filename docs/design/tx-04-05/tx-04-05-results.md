# tx-04-05 E2E 结果

> 最新状态（2026-10-24）：**tx-04-05 完成（用户接受，D23）**。用户决定不修复也不重跑，将第四次 E2E（`tx-04-05-e2e-04`）算作验收通过：V1、V3–V10 PASS；V2 的 harness 报告为 FAIL，原因是判定写死向右，而 S10 中玩家实际原生左移约 198 px。原报告不改写，各次失败记录原样保留。

日期：2026-10-24。对应[实现方案](tx-04-05-implementation-plan.md)第 12 节与[夹具/harness 记录](tx-04-05-test-harness.md)。

## 第一次 E2E（`artifacts/repentogon/tx-04-05-e2e-01/`）

结果：**FAIL**，S0 启动阶段中止。V1–V9 判为 FAIL，V10 为 NOT_RUN（报告原样），均因缺少证据，并不是观察到了功能错误。按规范第 2 节，不自动重跑。

### 执行过程

- 入口：`out/repentogon/harness.exe e2e tx-04-05 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-05-e2e-01`。使用的构建：DLL 22:51、harness 22:55，之后源码没有改动。
- 启动前已告知用户：只启动一次，harness 接管键盘，期间不要操作电脑。用户确认准备好后才启动。
- 启动前只读检查发现：第三方 mod `repentogonreleasesbackup_3643104060`（REPENTOGON Download Helper，只有启动器备份 zip，没有 `.lua`）处于启用状态，而 harness 的 S0 会拒绝任何第三方启用 mod。按用户选择，运行前在该目录临时创建 `disable.it`，运行后删除，已恢复为启用。tx-04-04 第八次运行时它也是启用的，当时的 harness 没有这项检查。
- S0 的环境检查（SkipIntro、Steam、PE 时间戳、键位、mod）、存档快照和部署都通过，注入启动成功（pid 25424，带 `LAUNCHED_BY_RGONLAUNCHER=1`）。

### 失败点

`scenes_tx05` 在等到窗口后先 `focus` 成功，并连上了控制口。随后 `wait_startup` 每次轮询前都会调用 `win32::focus`：

1. 第一次 `cs status` 等了 4992 ms，返回 `main_thread_timeout`。trace 中 `control_expired wait_ms=5859 frame=1`：第一个 Manager 帧在约 5.86 s 后才开始。
2. 下一轮的 `focus` 在 500 ms 内连续 50 次 `SetForegroundWindow` 都没有让游戏窗口成为前台，于是返回 `game focus could not be acquired`。harness 记录现场后关闭游戏（退出码 0）。

失败现场 `enabled-failure/state.json`：游戏窗口可见、未最小化、不在前台，标题为 `... - Do Manager::LoadConfigs (cur startup time: 5.300 s)`。紧接着的桌面截图中，标题已变为 `Do RoomConfig::LoadStages...`。

对照 tx-04-04 第八次运行：focus 函数和 `wait_startup` 的代码完全相同，当时第一次 `cs status` 在 4263 ms 就返回了 ok。

**原因（推断，未直接观察）：** 这次启动加载时间较长，游戏主线程超过 5 s 没有处理窗口消息。Windows 会把超过 5 s 不响应的窗口判为挂起，并用 ghost 窗口代替它，此时 `GetForegroundWindow` 不会返回游戏窗口句柄。5 s 的时间与 Windows 判定挂起的阈值吻合，但本次没有记录前台窗口句柄的归属，所以 ghost 机制只是推断。能确认的事实是：启动加载期间的 focus 失败被当作致命错误，而这时并不需要发送任何按键（发键时 `scan` 本身会先 focus）。这是 tx-04-04 harness 中就存在的时序缺陷，第八次运行恰好在 5 s 内完成了加载。

### 运行时附带观察（不作为验收）

trace 共 69 条，到关闭前：

- 6 项能力 `ok=1`；28 个 `hook_registered`、8 个 `pointer_patch`；
- `byte_patch id=odd_normal ok=1 resident=1 rva=2f9559 resume=2f955e`，Odd shim 已安装；
- `runtime_version repentogon=1.1.3`；Lua 菜单回应两次，`g_game` 非空；以正常 `shutdown` 结束。

这次没有进入 run，也没有启动会话，所以 tick、输入、生命周期都没有任何观察。

### 存档与清理

- 清理：测试 mod 已删除，`config.json` 已写回关闭状态；没有新增崩溃转储；`counters.json` 不存在，所以评估报错（`evaluation_error`）。
- 存档账本：第二档的变化在允许范围内。第一、三档的 `steam_remote/387111160/rgon_steam_persistentgamedata{1,3}.dat` 及当日 `save_backups` 副本有字节变化，判为不允许；`recorded_baseline.matched=[]`。
- 逐字节比较：每次启动只改这两档文件末尾的 5 个字节（slot1 的偏移 17179 和 17183–17186，slot3 的偏移 16227 和 16231–16234，即倒数第 8 字节和最后 4 字节）。e2e-08 的开启配置启动、关闭配置启动和本次启动，改动位置完全相同。REPENTOGON 日志对这三档都记录 `No synchronization required`。

### 新发现的 V10 规则缺陷

tx-04-05 harness 对 D18 历史基线的规则是：只有本次启动前后的字节分别与 e2e-08 关闭配置启动前后的字节完全相等，才放行。可是本次启动前第一、三档的字节等于 e2e-08 的 `save_after`，不等于它的 `save_mid`（关闭配置启动前）。每次启动都会改写末尾字段，所以这条规则在 e2e-08 之后的任何一次启动中都不可能匹配。即使其余场景全部通过，V10 也必然失败。

## 用户决定（D20）与修正

2026-10-24 用户决定：按建议修 focus 时序，V10 选 (a)，Download Helper 继续临时禁用；保留 tx-04-05 编号，修正后执行第二次 E2E（`tx-04-05-e2e-02`）。已完成的修改（只做纯编译，见 `artifacts/repentogon/tx-04-05-harness/rust-build-04.log`，没有 warning）：

- `Control::wait_startup`：每次轮询前仍调用 `win32::focus`，失败时在 `control.jsonl` 记 `startup_focus_pending` 后继续等待。`scenes_tx05` 等到窗口后的首次 focus 也改为只记录结果（`steps.jsonl` 的 `startup_focus`）。`scan` 发键与 `screenshot` 前的 focus 仍是硬性要求。tx-04-04 专用路径（`enabled_scene` 首次 focus、关闭配置等待）保持原样。
- `apply_recorded_baseline_tx05`：第一、三档文件需同时满足以下条件才放行：e2e-08 关闭配置区间中该文件有字节变化；历史前后、本次前后四份文件都存在；两组前后各自长度不变；所有变化字节都落在末尾 8 字节内（距末尾 5–8 为 Counter，1–4 为 Checksum）；本次改动的字段是历史改动字段的子集。放行记录写明本次和历史的字段。
- 依据：e2e-08 的两次启动和本次启动，slot1/slot3 的云存档及当日备份都只变化末尾 8 字节内的 5 个字节；REPENTOGON 日志 `[SaveFile] Counter/Checksum` 与这两个 u32 的值一致，Counter 每次启动 +4。

### 原列出的选项

1. **修正 focus 时序后再执行一次 E2E。** 建议：`wait_startup` 不再在轮询状态时强制 focus（或把失败记为 pending）；只在实际发键前 focus，失败仍是错误。是否沿用 tx-04-04 的 D16 先例，保留 tx-04-05 编号，作为第二次 E2E 写入 `tx-04-05-e2e-02`，由用户决定。
2. **V10 第一、三档的放行规则。** 可选：(a) 只放行与 e2e-08 关闭配置启动改动位置完全相同的末尾字段变化，其余字节必须不变；(b) 恢复一次关闭配置启动来重新取得同次基线（变成两次启动，需要分别提示用户）；(c) 维持现行规则（V10 必然失败）。
3. Download Helper 这类不含 Lua 的第三方 mod：继续每次临时禁用，还是让 S0 放行不含脚本的 mod。

## 第二次 E2E（`artifacts/repentogon/tx-04-05-e2e-02/`）

结果：**FAIL**。S0–S11 全部执行完毕，游戏正常退出（退出码 0），`Game::Exit` 计数 0→1。V1、V3–V9 PASS，V2、V10 FAIL。两项失败都来自 harness/夹具的时序或按键选择；DLL 侧没有观察到错误。不自动重跑。

### 执行过程

- 按 D20 修正后的 harness（`rust-build-04.log`）。启动前已告知用户，用户确认准备好后启动；Download Helper 运行前临时禁用、运行后已恢复。
- 23:17:59 选中第二档，23:18:31 回到主菜单。测试 mod 已删除，`config.json` 已写回关闭状态，没有崩溃转储，`cleanup_error`、`evaluation_error` 均为空。

### 通过项摘要

| 编号 | 关键观察 |
| --- | --- |
| V1 | 6 项能力为 1；注册 28 个 Hook，与清单一致；8 个设备槽指针补丁；Odd 普通点 shim 已安装并有 `byte_patch ok=1` 记录 |
| V3 | `Idle→Warmup→Active→Retiring→Idle` 两轮：第一轮 reason=Stop，第二轮 reason=RunTeardown；generation 1→2；没有 `tick_reject` |
| V4 | 共 683 个 tick，Odd 配对 341，无故障、无孤立阶段；非普通 Room 调用 1 次且未开始 tick；特殊 Odd 点的实际观察次数为 0 |
| V5 | 移动轴、consumer 非零读数与 Lua 位置变化一致，松键后在 lead+1 内回到中性 |
| V6 | aim 符号依次为 +1、−1、+1（tx-01-52）；泪弹增加；`shooting_suppressed=0`，无 tick 时没有观察到射击原调用 |
| V7 | 接受的 bomb edge 为 1，炸弹数 1→0 |
| V8 | S6、S7 的阻断期间 StandardRoom tick 为 0，恢复后分别有 18、25 个；暂停恢复后重新绑定；跳帧窗口 30 次（loop 1472–1533），期间 OddPlayer tick 1 个（按方案如实记录） |
| V9 | 切房目标 83 与实际一致；加载区间 loop 1560–1577 内没有 StandardRoom tick；A 在 1560 失效（block_enter），1577 重新绑定（stable_authority）；之后移动生效。T6 最终位置次序仍为 UNKNOWN |

最终状态：`trace_dropped=0`、`context_faults=0`、`foreign=0`、`session_error` 为空。

### V2 失败：S1 时游戏尚未开始模拟

- S1 的判据中，查询覆写、两种 tick 与 completed 都没变，会话前后都是 Idle，只有“玩家移动”不满足：S1 期间 3 条 Lua 观测的 `game_frame` 都是 0，位置停在 (320, 380)。
- 进入 run 后，Lua 从 loop 504 开始有观测，`game_frame` 直到 loop 687 才变为 1，约 3 秒（推测是开局关卡过场，未直接确认）。S1 按住 D 的区间（Manager 帧 546–611）整体落在这段时间里。
- 紧接着的 S1.recenter（同属未激活阶段）中游戏开始更新，玩家原生从 x=320 移到 242，计数同样不变；S10 的原生移动也正常（x 377→571）。但 V2 的判定只检查 S1 与 S10 的主区间，所以按定义判 FAIL。
- 原因：`enter_run_tx05` 只等到 `run_active=1` 和第一条 Lua 观测就结束 S0，没有等 `game_frame` 开始前进。

### V10 失败：F9 同时触发了原版手动截图

- 唯一不允许的变化是新增 `documents/screenshots/ManualScreenshot-20.png`（23:18:22.699）。
- 按截图前后两张 BMP 的时间与 Manager 帧（每秒约 59.2 帧）换算，该时刻约为第 1557 帧：S9 在第 1556 帧按 F9；S8 在第 1469 帧左右按 F8，当时没有产生截图。`isaac-ng.exe` 中有 `screenshots/ManualScreenshot` 字符串，REPENTOGON 源码中没有，所以这是原版游戏的截图热键。它不在 inputconfigs 的动作键位里，S0 的键位检查拦不住。截图键的具体分支没有静态确认（ida-mcp 未连接），判断依据是时间对齐。
- 第一、三档按 D20 规则放行：两档的云存档和当日备份都只改了末尾的 Counter/Checksum 字段，`recorded_baseline.matched` 四项。
- 该截图文件仍在用户的 `Documents/My Games/Binding of Isaac Repentance+/screenshots/` 中，未删除。

## 用户决定（D21）与修正

2026-10-24 用户决定：V2 改为等 `game_frame` 前进，切房复用 F8，删除测试产生的截图，修完后保留 tx-04-05 编号执行第三次 E2E（`tx-04-05-e2e-03`）。

- 截图：`ManualScreenshot-20.png` 已复制到 `artifacts/repentogon/tx-04-05-e2e-02/`（字节一致），然后从 `Documents/My Games/Binding of Isaac Repentance+/screenshots/` 删除。图中为 S9 切房前的起始房间、炸弹数 0，与时间对齐结论一致。
- `enter_run_tx05`：在 `run_active=1` 和首条观测之后，继续等待 Lua 观测出现两个不同的正 `game_frame`（15 s 超时即失败，期间不发键），并记录 `S0_simulating` 步骤。
- Lua 夹具：F8 按次序复用，第一次触发 30 次跳帧，第二次请求切房（跳帧未结束时记 `rejected/skip_pending`），第三次及以后记 `fixture-error`；不再响应 F9。harness 的 S9 改为发送 F8。S0 对 F8/F9 被映射为动作键位的检查保持不变。
- 依据：第二次运行中 F8 只产生一次 `skip-update start`，没有 `rejected`，也没有新文件，可见 `IsButtonTriggered` 每次按键只触发一次，F8 没有原生副作用。
- 纯编译：`artifacts/repentogon/tx-04-05-harness/rust-build-05.log`，没有 warning，`out/repentogon/harness.exe` 已更新。Lua 只做了代码审阅，没有运行。

## 第三次 E2E（`artifacts/repentogon/tx-04-05-e2e-03/`）

结果：**FAIL**。S0–S11 全部执行，游戏正常退出（退出码 0），`Game::Exit` 计数 0→1，没有崩溃转储。V1、V3–V10 PASS，只有 V2 FAIL，原因是 S10 的移动方向被房间里的岩石挡住。不自动重跑。

### D21 修正的效果

- S0 记录 `S0_simulating game_frames=[14,29]` 后才开始 S1。S1 中玩家原生从 x=320 移到 571，S1 通过。
- S9 改用第二次 F8 后，用户的 screenshots 目录没有新增截图。V10 通过：第一、三档的云存档与当日备份都只改了 `counter`、`checksum` 字段，按 D20 放行；第二档及其 REPENTOGON 附属文件在允许范围内。

### 通过项摘要

| 编号 | 关键观察 |
| --- | --- |
| V3 | 两轮 `Idle→Warmup→Active→Retiring→Idle`，reason 分别为 Stop、RunTeardown；generation 1→2；没有 `tick_reject` |
| V4 | 共 684 个 tick，Odd 配对 343，无故障、无孤立阶段；非普通 Room 观察 2 次、特殊 Odd Player 观察 1 次，都没有开始 tick |
| V6 | aim 符号 +1、−1、+1；泪弹增加；`shooting_suppressed=0` |
| V7 | 1 个 bomb edge 被接受，在 tick 344（StandardRoom）中 Resolve；`bomb_query_mismatches=0`；炸弹数减一 |
| V8 | S6、S7 阻断期间 StandardRoom tick 为 0，恢复后分别 17、28 个；跳帧窗口 loop 1667–1727，期间 OddPlayer tick 0 个 |
| V9 | 切房（slot 3，目标 97）与实际一致；加载区间 loop 1757–1775 内没有 StandardRoom tick；A 在 1757 失效，1775 重新绑定 |
| V10 | 正常退出，无新 dump，没有 REPENTOGON 新错误，存档变化全部允许 |

最终状态：`trace_dropped=0`、`context_faults=0`、`foreign=0`、`session_error` 为空。

### V2 失败：S10 向右被岩石挡住

- S9 从上方门（direction=3）进入房间 97，玩家位于 (320, 160)。harness 按“x≤320 就向右”选了 D，玩家右移到 x≈370 后停住。
- S10（stop 之后按住 D 1 秒）：会话前后都是 Idle，查询覆写（11762）、两种 tick 和 completed 都没有变化，`game_frame` 454→498，说明游戏在模拟；但 x 只在 369.9935–369.9998 之间变化，低于 1 px 的判定门槛。
- `S9-room.bmp` 显示玩家右侧紧贴一排岩石。这一局随机生成的房间布局挡住了固定向右的移动，DLL 没有接管输入。
- 原因：S9.move 与 S10 的方向只按 x 坐标选择，没有考虑房间内的障碍物。

## 用户决定（D22）与修正

2026-10-24 用户决定：S10 沿 S9.move 的反方向移动；修完后保留 tx-04-05 编号执行第四次 E2E（`tx-04-05-e2e-04`）。

- `scenes_tx05`：S10 的按键取 S9.move 方向（`post_room_key`）的反向，并写入报告的 `s10_key`。V2 的判据不变。
- 纯编译：`artifacts/repentogon/tx-04-05-harness/rust-build-06.log`，没有 warning，`out/repentogon/harness.exe` 已更新。

## 第四次 E2E（`artifacts/repentogon/tx-04-05-e2e-04/`）

结果：**FAIL**。S0–S11 全部执行，游戏正常退出（退出码 0），`Game::Exit` 计数 0→1，没有崩溃转储，没有新增截图。V1、V3–V10 PASS，只有 V2 FAIL。不自动重跑。

### 通过项摘要

| 编号 | 关键观察 |
| --- | --- |
| V3 | 两轮 `Idle→Warmup→Active→Retiring→Idle`（Stop、RunTeardown），generation 1→2，没有 `tick_reject` |
| V4 | 共 673 个 tick，Odd 配对 336，无故障；非普通 Room 观察 2 次、特殊 Odd Player 观察 1 次，都没有开始 tick |
| V6 | aim 符号 +1、−1、+1 |
| V7 | 1 个 bomb edge 被接受，炸弹数减一 |
| V8 | S6、S7 阻断期间 StandardRoom tick 为 0，恢复后分别 18、24 个；跳帧期间 OddPlayer tick 0 个 |
| V9 | 切房（slot 1，目标 71）一致；加载区间 loop 1749–1767 内没有 StandardRoom tick；A 在 1749 失效，1767 重新绑定 |
| V10 | 存档变化全部允许（第一、三档 4 个文件按 D20 放行），正常退出，无新 dump |

最终状态：`trace_dropped=0`、`context_faults=0`、`foreign=0`、`session_error` 为空。

### V2 失败：判定方向没有随 D22 同步

- S1 通过：会话 Idle 时，玩家原生从 x=320 移到 570。
- S9.move 按 D 从 x≈320 右移到 450。S10 按 D22 取反方向 A（`s10_key=A`），玩家原生从 x≈450 左移到 252（y 400→358）；会话前后都是 Idle，查询覆写（11564）、两种 tick 和 completed 都没有变化。
- 判定函数 `moved(&obs, "x", 1.0)` 要求 x 向右增加超过 2 px。D22 修改 S10 按键时，没有同步修改这里的方向，这是主智能体修改时的疏漏。所以本次 V2 失败不是透传失败，而是判定与场景不一致。

## 用户决定（D23）：验收结论

2026-10-24 用户决定：不修复 V2 对 S10 的判定方向，也不重跑，直接将第四次 E2E 算作 tx-04-05 的验收通过。

- 证据等级：V1、V3–V10 为 harness 判定 PASS。V2 为用户接受：S1 的透传由 harness 判定 PASS；S10 的原生移动是运行时观察（会话 Idle、查询覆写 11564 与 tick 计数不变、x 450→252），harness 判定为 FAIL。
- `artifacts/repentogon/tx-04-05-e2e-04/report.json`/`report.md` 保持原样（FAIL），不改写。
- 已知问题（未修复）：`tools/harness/src/tx05/evidence.rs` 的 V2 对 S10 调用 `moved(&obs, "x", 1.0)`，固定要求向右；而 D22 起 S10 按 `s10_key` 反向移动。之后再用这套 harness 时，应先按 `s10_key` 取判定方向。
- 本次推进中的环境操作：每次运行前在 Download Helper 目录临时创建 `disable.it`、运行后删除（均已恢复）；第二次运行产生的 `ManualScreenshot-20.png` 已存入 e2e-02 artifact 后删除。
