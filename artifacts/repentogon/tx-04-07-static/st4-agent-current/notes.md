# tx-04-07 ST4：Continue 与 Host Warmup 静态核验

日期：2026-10-25。范围只覆盖当前 IDA 中 Continue / GameState / Player restore 的有限函数与现有 mod 生命周期源码；不把直接引用清单说成完整调用闭包。本轮未启动游戏、未动态调试、未构建或测试，也未读取或校验任何哈希，未修改 IDB。

## 结论

原生菜单的存档读取、`ExecuteStartGame` 的原生 `RestoreState`、恢复后的 controller 重分配，以及原生 co-op Join 是不同阶段。静态代码支持“若当前存档实际包含一个有效 A Player 记录，原生主 restore 会按一个记录同步 Player 列表并恢复记录字段”，但它不能证明实际存档一定只有一条记录，也不能承诺复用同一个 Player 对象或逐字节相同。`AutoReassignControllers` 只负责现存 Player 的 controller 重分配；这不是创建 B 的 Join 路径。

当前 tx-04-06 生命周期源码与“菜单 Host 握手后保留 Warmup，再 Continue 恢复 A、随后创建 K 并 Join”的顺序存在直接冲突：主作用域 `Game::RestoreState` hook 在调用原生 `super` 之前先执行 `RestoreBeginning()` 和 `RunExit()`；两者最终都无条件 `StopSession(RunTeardown)`。已握手的 Host Warmup 可被 `BeginRetirement` 退休。`RefreshLifecycle` 对“尚未经历 run 的菜单 Host”保留 Warmup 的条件不能拦截这个直接 wrapper teardown。因此按现有源码，这条方案在原生 restore 之前就把 Host runtime 标记为 retiring，不能继续到 A bind → K `CreateOnce()` → Join/K 分派。既有 run 的 restore/teardown 语义仍须保留；若后续修改，需要区分首次菜单 Continue 与已经运行过的 restore，并同时处理 wrapper 中的 `RestoreBeginning`、`RunExit` 两条停止路径。本轮没有修改生产代码。

## 原生菜单与 Continue 窗口

| 结论 | 状态 | 当前 IDA 证据（VA / RVA） |
|---|---|---|
| 菜单 helper 在当前菜单状态 `this[16] == 16`、目标状态参数为 `3` 且 `this+55888` 标志为真时，调用 `Manager::LoadGameState`，slot 参数取自 `n4[4]`。菜单数值状态与 UI 字符串标签没有在本轮完整命名，因而不单独把数值 3 作为外部标签证明。 | `STATIC_CONFIRMED` 条件；标签 `UNKNOWN` | helper `sub_962E20` VA `0x962E20` / RVA `0x562E20`；调用 VA `0x962FCC` / RVA `0x562FCC`。`Menu_Game_Update` VA `0x959590` / RVA `0x559590` 有调用该 helper 的菜单路径。 |
| `Manager::LoadGameState` 是将选中 slot 的存储状态读入 GameState 的路径；当前直接代码 xref 共 3 个：菜单 helper、持久数据 wrapper、另一未命名 caller。它本身不同于随后应用 GameState 的 `Game::RestoreState`。 | `STATIC_CONFIRMED` | 目标 VA `0x9332F0` / RVA `0x5332F0`；调用点 VA `0x962FCC` / RVA `0x562FCC`、VA `0x9332B4` / RVA `0x5332B4`、VA `0x8E5019` / RVA `0x4E5019`。`ida-evidence.json` 保存了有限目标导出。 |
| `Manager::ExecuteStartGame` 只在 `Manager+0x4AD10`（项目 `offset::Starting`）为真时进入启动处理。菜单淡出完成的路径写入 Starting；函数尾部清零。 | `STATIC_CONFIRMED` | 菜单写入 VA `0x9596C6` / RVA `0x5596C6` 和 VA `0x959716` / RVA `0x559716`；ExecuteStartGame 检查 VA `0x932A69` / RVA `0x532A69`，尾部清零 VA `0x93302E` / RVA `0x53302E`。源码偏移见 `mod/src/binding/offsets.h:11`。 |
| ExecuteStartGame 在 `Manager+0x4AD11` 分支调用 `Game::RestoreState(state, true)`；调用后立即调用 `PlayerManager::AutoReassignControllers`。在这条分支中 Starting 到 restore 调用前仍为 1，并在函数尾才清零。 | `STATIC_CONFIRMED` | 分支比较 VA `0x932DD9` / RVA `0x532DD9`，RestoreState call VA `0x932E7E` / RVA `0x532E7E`，AutoReassign call VA `0x932E89` / RVA `0x532E89`。`Game::RestoreState` 目标 VA `0x6F43A0` / RVA `0x2F43A0`。另一个相邻 flag `Manager+0x4AD12` 进入不同的 `sub_6F3650` 分支；这里不把它与 Continue restore 混同。 |
| StartGame 首次构造对象时的官方构造函数是 `0x6ED4B0`，随后 `0x6F0AF0` 是启动准备函数。 | `STATIC_CONFIRMED` | constructor VA `0x6ED4B0` / RVA `0x2ED4B0`；startup preparation VA `0x6F0AF0` / RVA `0x2F0AF0`。 |

ST5–ST7 已静态确认：Continue restore 会从 GameState 恢复被保存的 Game flag；这不能外推成 Continue 必经保存、清零或某个后续退出保存。参见 `docs/research/tx-04-07-static-confirmation.md:155-161`、`198-212`。本 ST4 记录同样不从 RestoreState call 推断磁盘写入或清零。

## Player 保存、恢复与 controller 重分配

- `Game::SaveState` 调 PlayerManager save routine（VA `0x9940E0` / RVA `0x5940E0`；上游 call VA `0x6F54C3` / RVA `0x2F54C3`）。该 routine 在 GameState `+105840` 处重置 player count；对符合原生保存条件的当前 Player 写入记录并递增 count。记录首址为 `GameState+105844`，stride 是 1468 字节，最大 count 分支为 8。单记录结论的前提是该次保存实际只计入 A 一条符合条件的记录。
- `Game::RestoreState`（VA `0x6F43A0` / RVA `0x2F43A0`）调用 `PlayerManager::RestoreGameState`（VA `0x993A90` / RVA `0x593A90`），随后恢复 Level，再调用 PostLevelInit。主 Player restore 在 VA `0x993AC9` / RVA `0x593AC9` 调 `SyncPlayersFromGameState`（VA `0x993780` / RVA `0x593780`），参数 `a4=1`。
- Sync 按 `GameState+105840` 个记录迭代，记录 stride 1468；对已有 Player 的已保存标识执行匹配。`a4=1` 时移除未匹配的现有 Player；若记录没有匹配对象，则调用原生 `SpawnCoPlayer2`（VA `0x990250` / RVA `0x590250`；调用 VA `0x993A1C` / RVA `0x593A1C`）补建对象。然后主 restore 对映射 Player 应用保存字段，PostLevelInit 再以 `a4=0` 做另一轮映射同步。
- 因而 `count==1` 能确认的是此原生路径按一条记录映射/保留/创建 Player，并把该记录字段用于 restore；不能确认对象地址或内存整体与保存前相同，也不能在未查看实际存档的情况下确认 count 必为 1。标识匹配、创建或删除取决于当时 Player 列表及记录内容。
- Restore 返回后，`AutoReassignControllers`（VA `0x9964A0` / RVA `0x5964A0`）遍历现存 Player，先把 controller id 置为 `-1`，再基于可用控制器重映射。该有限函数没有直接 `SpawnCoPlayer2` 或 Join 创建调用；间接辅助函数闭包不在本次断言范围。
- `Game::RestoreState` 当前 IDA 直接 code xref 共 3 处：VA `0x932E7E`（ExecuteStartGame）、`0x701D09`（`sub_701AA0`）、`0x8EA596`（`sub_8EA500`）。后两处所属业务情境未由有限静态范围完整命名，保留 `UNKNOWN`；没有把 caller 清单当成全程序闭包。

## Host Warmup、K 创建和 Join 分派

源码证据（按当前 mod checkout）如下：

- `mod/src/game/tick.cpp:224-250` 的 `StartHost` 仅要求 Manager 主线程边界、能力和适配前置条件；该函数本身没有 `RunActive()` 门槛，因此允许在菜单创建 Host worker/runtime。`mod/src/game/peer.cpp:80-90` 在握手成功后把 core Session 从 Hosting 推至 Host Warmup。
- `mod/src/game/lifecycle.cpp:47-60` 的 `RefreshLifecycle` 在 Host 尚未 `run_observed` 且当前没有 run 时保留 Warmup、设置 `Lifecycle` block 并返回。这是 Manager tick/lifecycle 检查上的“首次 run 前等待”条件。
- `mod/src/game/lifecycle.cpp:142-174` 的 `ReadyForAuthority` 要求 runtime 未 retiring、未 block 且 A 已绑定。Host 的 K 仍是 Empty 时只调用一次 `DeviceK::CreateOnce()` 并返回等待；随后要求 K Registered，且 Join 完成（Warmup 时 B 绑定）才允许激活。
- `mod/src/game/join.cpp:54-105` 的 `JoinScope` 要求 Host、peer 已握手、core Warmup、K Registered、还未 Join complete、真实 run 存在且无 starting/pause/transition、A 已绑定，以及正确的 Join/Manager owner 与排除的 Restore/Door/Lifecycle scope。`join.cpp:207-236` 与 `mod/src/hooks/host_patch.cpp:17-18,108-110` 是独立的原生 JoinCreate/兼容分派入口，不是 Player restore 或 AutoReassign。
- 但 `mod/tools/gen_binding.py:73-80` 生成 restore wrapper 时，在 `super({call})` 之前执行 `if (scope.main) { RestoreBeginning(); RunExit(); }`。`mod/src/game/lifecycle.cpp:198-205` 的 RestoreBeginning 直接走 RunTeardownBeginning，设置 `run_torn_down` 并无条件 `StopSession(RunTeardown)`；`mod/src/ops/command.cpp:287-295` 的 RunExit 也无条件调用相同 StopSession。`mod/src/game/tick.cpp:258-270` 对尚未握手的 Host 跳过 core BeginRetirement 但仍设 `r.retiring=true`；对已经握手的 Warmup 则调用 `BeginRetirement`。`mod/src/core/session.cpp:125-143` 接受 RunTeardown，并只排除 Dormant/Failed 等状态，所以 Warmup 可进入 Retiring。
- `RefreshLifecycle` 的 `!run_observed` 特例不会绕过 Restore wrapper 的直接 StopSession。故按当前源码，在 proposed menu Host → handshake Warmup → Continue 的 `Game::RestoreState` 进入原生 `super` 之前，会命中生命周期冲突。若假设这次退休被其他实现改变，之后要等稳定 run、A bind，然后才可能 Create K 和 Join；这条假设链是 `STATIC_INFERRED`，不是动态执行结果。
- 对既有 run 调用 restore 时，`RestoreBeginning` 当前明确复用 RunTeardown 行为；保持这类 teardown 仍然是要求。后续实现需辨别“从主菜单开始的首次 Continue”与“既有 run 中恢复”来源，并同时处理 restore wrapper 中 RestoreBeginning 与 RunExit 两次 StopSession。本次只记录冲突，不提出未经审查的生产改动。

## 当前证据文件与边界

`export_st4_readonly.py` 按显式 RVA 清单使用 IDA 只读 API 导出 26 个函数的反编译、指令、直接 call sites 和指向函数的 xrefs 到 `ida-evidence.json`。脚本把 VA 按 `VA = imagebase + RVA` 计算，不定义函数、不重命名、不改类型、不写 IDB。当前 IDA Health 显示 imagebase `0x400000`、目标输入模块 `isaac-ng-v1.9.7.12.exe.unpacked.exe`、自动分析与 Hex-Rays 就绪；官方源 README 为 `F:\projects_isaac\REPENTOGON-1.1.3` 的 v1.9.7.12.J273。未记录哈希。

状态用语：`STATIC_CONFIRMED` 表示当前 IDA 指令/反编译直接支持的有限函数事实；`SOURCE` 表示现有 mod 源码事实；`STATIC_INFERRED` 表示由上述条件拼接的可能时序；`UNKNOWN` 表示未验证的 UI 数值语义、实际存档内容、其它 RestoreState caller 业务语境及间接调用/回调闭包。这里不代表运行时观察，也不补齐 tx-04-07 整项静态主审。
