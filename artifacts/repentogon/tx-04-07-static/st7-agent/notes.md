# tx-04-07 ST7：ST1–ST4 持久化观测覆盖审查

> **主审批注（2026-10-25）**：原表 `pgd_unlock` 一行写“记录……返回值”，此措辞保留供审阅追溯，但并不准确。`PersistenceScope` 实际记录 `normal`、观测字段前后值等信息；`gen_binding.py` 中 wrapper 的 `auto result` 未传给 `PersistenceScope`，因此当前 trace **不记录 `TryUnlock` API 的 bool 返回值，也不记录文件写入是否成功**。ST3 unlock 窗口 RVA `0x5099DF`、`0x509A16` 中 EDI 指向 Game（n187），不是 manager；对应 `+0x10`、`+0x1839C`、`+0x25AD8`、`+0x25ADC` 均相对 Game。完整 `StageTransition_Update.json` 在 `0x509A04` 重载 EDI=n187，可核对此解释。上文及下文提出的完整回调、sidecar 与文件 API 观察，只是方案补齐选项；本轮没有实现这些观察，也没有要求穷尽第三方回调/间接调用闭包。

日期：2026-10-25。范围：只读静态审查 ST1 B 死亡、ST2 `RevivePlayerGhost`、ST3 换层、ST4 Continue，判断 tx-04-06 ST8 的五类持久入口观测是否足以记录这些操作窗口。结论可供实现方案修订，不代替运行时观察。

## 结论

**现有 ST8 观测只覆盖五个精确的原生 PGD/控制器映射入口，不足以覆盖 ST1–ST4 的完整持久化行为或操作归属。** ST1 的终局死亡计数、ST1/ST2 的条件性 `TryUnlock`、ST3 中 Manager 保存期间调用的 PGD backup/cloud/local 入口会命中其中部分 Hook；但当前没有 `b_death`、`b_revive`、`stage_change`、`continue` 标签，且完整 `GameState` 写入、官方附加文件、删除和成就 JSON 写入都不在五类入口内。ST4 Continue 分支目前没有发现直接保存调用，必须把“恢复/读取”与之后的换层或正常退出保存分开记录，不能把后续保存归到 Continue 读取本身。

所有“命中 PGD Hook”只表示 C/S 可以记录对应 native 方法的 enter/return。当前观测不是通用文件监控，也不是官方 Hook/回调活动状态的运行时证据。有限直接调用图不能证明间接调用闭包完整。

## 环境、标签与证据边界

- IDA MCP `server_health`：输入 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`，自动分析和 Hex-Rays 已就绪。IDA MCP 地址使用 VA；文中同时列出 RVA，`VA = RVA + 0x400000`。
- IDB 只读。未运行游戏、构建、测试、动态调试、哈希或 gate；未修改 IDB 的名字、注释、类型、字节或数据库状态。
- 官方实现来源：`F:\projects_isaac\REPENTOGON-1.1.3`。官方源码说明 Hook 的设计行为，不证明该 Hook 在目标运行时的安装顺序、实际触发或 Lua callback 已启用。
- `STATIC_CONFIRMED`：目标版本指令、直接调用或交叉引用直接支持；`STATIC_INFERRED`：基于上下文/调用关系的有限推论；`SOURCE`：官方或本工程源码事实；`UNKNOWN`：当前静态资料未闭合。
- 可重复 IDA 导出：[`export_st7.py`](export_st7.py)，执行后生成 [`ida-st7-target-windows.json`](ida-st7-target-windows.json)。脚本只调用 IDA 只读 API，导出有限指令窗、字节、VA/RVA 与直接 xref；不写回 IDB。脚本不是测试，也不声称完整调用闭包。

`SOURCE`：本次核对的方案问题是 `docs/design/tx-04-07-implementation-plan.md:244` 的 ST7；目标标签来自 §3.1 第 77 行的 `b_death/b_revive/stage_change/continue`，V16 第 324 行要求四个窗口记录写入入口与条件。ST3 的标签窗口边界仍是 §10 第 240 行的待确认点；本报告建议用跨 loop 关联窗口表达，不能只在 `StartStageTransition` 同步调用期间打 TLS 标签。

## ST8 现有观测到底覆盖什么

`SOURCE`：[`mod/binding/hooks.json`](../../../../mod/binding/hooks.json:32) 只登记以下五类 Hook；`mod/tools/gen_binding.py:50–55` 只在这些 wrapper 中建立 `PersistenceScope`：

| Hook | RVA / VA | 观测事实与边界 |
| --- | --- | --- |
| `pgd_unlock` / `PersistentGameData::TryUnlock` | `0x503930` / `0x903930` | `mod/src/game/persistence.cpp:13–18, 39–42, 49–52` 记录合法范围内的 achievement 值、dirty/readonly/loaded 和返回值。ID 越界会令 `known=0`；这不是 file writer 观测。 |
| `pgd_counter` / `PersistentGameData::IncreaseEventCounter` | `0x503A50` / `0x903A50` | 记录 event counter、dirty 与入口条件；原生入口在 readonly=false 时更新计数并置 dirty。 |
| `pgd_local` / `SaveLocally` | `0x503460` / `0x903460` | 观察调用该 PGD API；不覆盖其他对象自行打开的文件。 |
| `pgd_cloud` / `SaveToSteamCloud` | `0x502F00` / `0x902F00` | 观察调用该 PGD API；不是所有保存分支都会走 cloud。 |
| `button_maps` / `CsButtonMaps::SaveButtonMaps` | `0x42AC90` / `0x82AC90` | 只记 enter/return，不采集 PGD 数值。官方把文件目标改为 `inputconfigs_rgon.dat`（`SaveSyncing/InputConfigs.cpp:28–45`）。 |

`SOURCE`：`mod/src/game/persistence.cpp:11, 26–27` 的 operation 是线程局部名字；`PersistenceScope` 在 Hook 入口捕获当前名字（28–42），在返回时再读状态（44–52）。现有 operation 创建点只有 `device_register`、`device_remove`、`player_remove` 与 Join owner 回退，见 `mod/src/game/device_k.cpp:210,274`、`mod/src/game/join.cpp:239`、`mod/src/game/persistence.cpp:32–33`。没有 ST1–ST4 的四个 operation 标签。

`SOURCE`：[`tools/harness/src/tx06/evidence.rs`](../../../../tools/harness/src/tx06/evidence.rs:611) 先收集所有 `persistent` 事件，但归约 `writes` 时只取 `phase=return && operation != none`（615–618）。因此新窗口内仍为 `none` 的 enter/return 事件被排除在 `no_added_writes` 外。若只增加 `stage_change` 等标签，旧断言又要求 `known=1`、`normal=1`、禁止 `pgd_local/pgd_cloud/button_maps`，且 `before==after`、`dirty_before==dirty_after`（619–625），会误拒 R5/V16 允许记录的原生死亡计数、解锁和保存。需要先改变归约含义：按操作窗口记录事件，并把“允许的原生窗口写入”与“C/S 新增写入”分别判定；这属于 V16 语义修订建议，不是新增 gate。

## 分路径结论

| 路径 | 已定位 native 入口与条件 | 五类 Hook 覆盖情况 | 未覆盖的持久化效果 / 归属 |
| --- | --- | --- | --- |
| ST1 B 死亡 | `STATIC_CONFIRMED`：正常 ghost 路径在 `TriggerDeath` RVA `0x3995ED` 再判 dead；Morph 内的 Revive 已清 dead，因此这一支绕开终局死亡 helper。若走终局 helper `PlayerManager_DeathHandler` RVA `0x593120`（VA `0x993120`），Variant0 分支在 RVA `0x593147`（VA `0x993147`）调用 `IncreaseEventCounter(10,1)`。Morph 的 `EvaluateItems` 还有条件性 `TryUnlock` 调用点 RVA `0x367520`、`0x367637`、`0x3679B6/0x3679CD`、`0x368610`、`0x368EBD`。 | **部分覆盖**：终局死亡计数会命中 `pgd_counter`；若 `EvaluateItems` 条件路径调用 PGD，也命中 `pgd_unlock`。正常 ghost 绕过的是这一个 death counter 分支，不构成整个窗口无副作用证明。 | 无 `b_death` 归属；`Kill()` 返回与之后 ghost/终局失败可能跨 loop。官方 `MC_POST_ENTITY_KILL` 的回调体可能做额外工作，但测试 fixture 的运行时回调状态未知。直接 JSON/文件、其他官方 Hook/回调副作用不由 PGD 观测覆盖。 |
| ST2 复活 | `STATIC_CONFIRMED`：`RevivePlayerGhost` RVA `0x3CFC60`（VA `0x7CFC60`）复用 Player；在 RVA `0x3CFDB4` 调 `EvaluateItems` 并可触发上述条件性 `TryUnlock`；函数本体的直接调用表未命中 `IncreaseEventCounter` / `TryUnlock`。普通路径仍有 `EvaluateItems` 的条件和附属 Player 分支。 | **部分覆盖**：若 `EvaluateItems` 真正调用 `TryUnlock`，入口被 `pgd_unlock` 观察；本体无已定位直接 counter 调用。 | 无 `b_revive` 归属；当前同步 scope 若仅放在 Lua 请求外层，需要确保包括实际调用与返回。REPENTOGON 的 EvaluateItems/custom-cache、GetHealthType、音效等官方 Hook 提供下游行为，但实际 callback/状态未知；这些 Hook 不是持久文件观测。 |
| ST3 换层 | `STATIC_CONFIRMED`：`StageTransition::Update` RVA `0x509B2B`（VA `0x909B2B`）直接调用 `Manager_SaveGameState` RVA `0x532160`（VA `0x932160`）。另有条件 `TryUnlock(42)` at RVA `0x5099FF`：`[manager+0x10]==0` 时累计 `+0x25AD8` 达到 2；条件 `TryUnlock(0x43)` at `0x509A39`：`[manager+0x1839C] & 0x20 == 0` 且累计 `+0x25ADC` 达到 2。函数入口调用、条件分支及地址见 JSON。 | **不完整覆盖**：两种 PGD `TryUnlock` 以及保存尾部调用的 PGD cloud/local API 可命中已有 Hook。Manager 保存、GameState writer/delete 与 sidecar writer 不在五类 Hook 内。`Manager_SaveGameState` 还在 RVA `0x53244F`（VA `0x93244F`）调用未登记的 PGD backup `0x5032A0`（VA `0x9032A0`）；RVA `0x532475/0x53248A` 再按 Steam 条件分 cloud/local。 | 无 `stage_change` 窗口标签；同步标签不一定覆盖 `StageTransition::Update` 的延迟保存。官方 `GameState::write` 成功后会写 REPENTOGON 附加数据；`DeleteRerun` 可清理相应文件。这些 API、官方 JSON/文件 writer、tracker callback 均不由五 Hook 记录。|
| ST4 Continue | `STATIC_CONFIRMED`：`Manager::ExecuteStartGame` RVA `0x532A20` 的 Continue 分支（指令窗 RVA `0x532DD9–0x532EF1`）在 RVA `0x532E7E` 调 `Game::RestoreState` VA `0x6F43A0`，未发现该分支或该 RestoreState 函数直接调用 `Manager_SaveGameState`。Continue 与 manager save 的直接 xref 清单分别见导出 JSON。`Manager_LoadGameState` 当前已见直接 xref 来自其他地址，不在该 Continue 分支。 | **对“Continue 本身写入”没有已确认命中项**：五类 Hook 不观察 GameState/ESSM 读取。若恢复后另外发生 PGD API 调用，仍可被入口 Hook 看到，但当前没有 Continue operation 标签。 | `GameState::read/read_rerun` 是读/还原路径，官方源中调用 ESSM Deserialize、VirtualRoom/ItemPool 读取；这与本地保存 writer 不同。不能把 `Game::Start` 尾部 `0x2F1B43 → Manager_SaveGameState` 或之后正常退出 `Game::Exit → Manager_SaveGameState` 当成 Continue 分支必经。恢复期间官方 Lua Deserialize 或其他回调的任意副作用仍是 `UNKNOWN`；后续换层/退出应按实际独立 operation 记录。 |

### ST1：死亡窗口的官方分派与统计

`SOURCE`：`Patches/ExtraLives.cpp:30–37,63–136,190` 包装 PlayerManager 更新/TriggerDeath 并加入复活相关回调；`LuaInterfaces/CustomCallbacks.cpp:6500–6521` 包装 Entity Kill，原调用后在 callback bit 开启时派发 `MC_POST_ENTITY_KILL (68)`；`Patches/ItemSpoofSystem.cpp:346–354` 也包装 TriggerDeath。`Patches/AchievementsStuff.cpp:248–257` 在 readonly=false 时先按玩家类型运行 EventCounter tracker 再调用原生 counter。项目 `docs/research/tx-04-07-static-confirmation.md:95–108` 已限定正常 ghost 分支和死亡 helper 条件。

`SOURCE`：`Patches/AchievementsStuff.cpp:105–136` 中，原生 ID `[0,641)` 走 super；REPENTOGON 自定义 ID 分支在 readonly=false 且 `Achievements[achievid] <= 1` 时更新 map，运行 tracker，并在 131 行调用 `SaveAchieveemntsToJson`。该函数在 63–78 行通过 `ofstream` 写成就 JSON。官方 callback wrapper `LuaInterfaces/CustomCallbacks.cpp:4617–4635` 可派发 `MC_POST_ACHIEVEMENT_UNLOCK (1476)`。C/S 的 `pgd_unlock` 可看到入口，但不记录 `SaveAchieveemntsToJson` 文件写入或 callback body；ID ≥641 时当前 `PersistenceScope::Value` 会报告 `known=0`（`mod/src/game/persistence.cpp:13–18`）。

因此 ST1 的准确说法是：**PGD API 原生入口部分覆盖，完整窗口不覆盖**。不要由 ghost 绕过死亡计数推出无副作用，也不要由最终 PGD 文件相同推出没有回调/JSON写。

### ST2：复活窗口

`SOURCE`：Lua `ReviveCoopGhost` 仅在 `_isCoopGhost` 为真时调用原生 `RevivePlayerGhost`（官方 `LuaInterfaces/Entities/LuaEntityPlayer.cpp:2355–2366`）。ST2 直接调用分析显示普通复活函数把 cache 标成 `CACHE_ALL` 后重算道具；`EvaluateItems` 条件项是已有入口 Hook 可捕捉的路径，不代表本轮必然触发。官方 `CustomCache.cpp:318–339`、`CustomCallbacks.cpp:1574`、音效 Hook 等可以执行更多 callback；哪些 callback 注册/触发和 callback 体副作用都不是静态已确认事实。

建议将 `b_revive` 窗口从复活请求延伸到原生调用返回与观察结果稳定，但无需把之后的整局退出保存一并纳入该标签。

### ST3：保存与删除的实际官方路径

`STATIC_CONFIRMED`：`StageTransition::Update` 的 direct xref 在 RVA `0x509B2B` 进入 `Manager_SaveGameState`。保存函数 native checks 包含：`[manager+8] == 2`（RVA `0x53216F`）、`[global+0x25BA8] == 0`（`0x532191`），以及保存对象容器端点相等（`0x5321A4–0x5321AA`）；字段语义不全由指令本身确定。其可达块在 RVA `0x532300` 和 `0x5323F0` 调 `GameState::write`；RVA `0x53223D`、`0x532346` 有条件调用 `GameState::DeleteRerun`。RVA `0x53244F` 进入 PGD backup；PGD loaded 字段非零后才继续保存路径（`0x532441–0x532448`），Steam context 与选项决定 `SaveToSteamCloud` RVA `0x532475` 或 `SaveLocally` RVA `0x53248A`。指令窗和 xref 原始证据在 JSON 中。

`STATIC_CONFIRMED`：IDA 交叉引用到 `GameState::write` VA `0x99F7A0` 的两处 Manager 保存调用为 VA `0x932300/0x9323F0`。Manager 保存自身的原生 PGD API 调用只部分命中五 Hook；`GameState::write`、`GameState::DeleteRerun` 不在五 Hook。`GameState::Delete` RVA `0x59E7B0`（VA `0x99E7B0`）直接 xref 包括 `Manager_DeleteGameState` RVA `0x5324A0` 的 `0x5324CD`，以及 `Manager_LoadGameState` 的 `0x53347D/0x533844`；目前不把这些 load/delete callers 说成 Continue 必经。`DeleteRerun` VA `0x99E870` 在 Manager_SaveGameState 的 `0x93223D/0x932346` 和 Manager_LoadGameState 的 `0x933652/0x9339C4` 被直接调用。

`STATIC_CONFIRMED`：backup RVA `0x5032A0`（VA `0x9032A0`）把路径格式化到 `save_backups/`，调用 RVA `0x503380`（VA `0x903380`）；后者以写模式开 KAGE File，调用 PGD serializer `0x902160`，再检查/flush/close。该 native 入口没在 hooks.json 五类之内。这里只确认静态备份写路径入口与序列化调用；不扩展其完整文件拷贝实现闭包。`SOURCE`：D27 退出存档判定明确不看每日备份；这项发现应进入可观察入口清单，不改成测试限制、阻断条件或新的判定项。

`SOURCE`：官方 `Patches/GameStateManagement.cpp` 的 `get_info` 在 24–47 行检查 slot 是本机保存槽、slot 范围 `1..3`、文件名有效；未知/非 save-file state 可能跳过 sidecars。常规 `GameState::write` 在 261–286 行先对未知 info 只透传；否则收集 ESSM write state、调用 original、恢复 state；仅 original 成功后才调用 `ESSM::Serialize` 与 `write_save`。`write_save` 在 180–191 行先调用 `VirtualRoomSetManager::detail::WriteSave`，失败则停止；成功后才调用 `ItemPoolManager::__SaveToDisk`。`write_rerun` 在 289–314 行有不同流程，也会受 original 成功与 info known 条件约束。不能把所有 Manager_Save entry 等价为所有 sidecar 成功落盘。

`SOURCE`：VirtualRoom `WriteSave`（`VirtualRoomSetManager.cpp:1167–1183`）对 rerun 早退；普通保存取 slot DB 并调用 `WriteDB`。`WriteDB`（953–995）没有空 `restoredRooms` 的早退：仍会创建文件并写含 `"Rooms": []` 的 JSON。路径由 948–951 行拼成 `%REPENTOGON_DATA%/VirtualRoomSetManager/<fileName>.json`。即使没有虚拟房间，也应记录其调用/写尝试；是否成功仍取决于父目录、文件打开等结果。

`SOURCE`：`ItemPoolManager::__SaveToDisk`（1476–1497）尝试打开并写 JSON，`open_game_state_file` 在 1438–1468 行建目录并打开文件。`EntitySaveStateManagement.cpp:1942–1945` 的 ESSM 路径为 `%REPENTOGON_DATA%/EntitySaveStateManagement/<fileName>.dat`。`Serialize`（2234–2242）先执行内部 serialize，再调用 `EntitySaveStateEx::WriteSaveFile`；内部 binary serialize 仅成功 flush 后才运行 `s_luaCallbacks.Serialize`（2193–2232），但 `WriteSaveFile` 在内部函数返回后仍会被调用。`EntitySaveStateEx.cpp:200–255` 写 `ex_<fileName>.json`。因此要把 ESSM `.dat`、Lua Serialize callback 与 `ex_*.json` 分开记录，不能只盯住一次 PGD 或主存档写。

`SOURCE`：官方删除路径 `GameStateManagement.cpp:390–420` 在 original delete 后，只有 `info` known 才调用 `delete_save`；该 helper 207–215 行依次删 ItemPool 文件、VirtualRoom DB、ESSM 附加状态。`EntitySaveStateManagement.cpp:2307–2327` 的 `DeleteGameState` 先调 `EntitySaveStateEx::DeleteSaveFile`，后删除 `.dat`；`EntitySaveStateEx.cpp:353–364` 删除 `ex_*.json`。需把删除入口和写入入口都列进窗口事件；不能用最终文件未变推断无删除/重建。

## Continue 边界

`STATIC_CONFIRMED`：`Manager_ExecuteStartGame` 的 Continue 分支在 VA `0x932E7E` 调 `Game::RestoreState` VA `0x6F43A0`；选定分支指令窗与 `Game::RestoreState` 调用信息见 [`st4-agent/Manager_ExecuteStartGame.json`](../st4-agent/Manager_ExecuteStartGame.json)。当前没有在该 Continue 分支或 RestoreState 本体中定位到直接 `Manager_SaveGameState` 调用。IDA 给出的 `Manager_LoadGameState` 直接 xrefs 是 VA `0x8E5019`、`0x9332B4`、`0x962FCC`，不是 ExecuteStartGame/RestoreState 的直接 caller。此结论是具体直接边的否定证据，不是任意间接/Hook callback 无副作用证明。

`SOURCE`：GameState read Hook（`GameStateManagement.cpp:316–355`）先 original read 与内部校验；只有成功且 `isLocalRun`、info known 后才走 `ESSM::Deserialize` 与 `read_save`，其中读取 VirtualRoom DB、ItemPool 与 EntitySaveStateEx。此处是读文件/恢复，不属于写入入口；Lua PreDeserialize/Deserialize callback 可以扩展行为，回调体效果为 `UNKNOWN`。Normal exit 的 Game::Exit 到 Manager_SaveGameState（native RVA `0x2F63B7`）和 Game::Start 调用 Manager_SaveGameState（`0x2F1B43`）是独立调用者，不得倒推为 Continue 必经。

实现中建议为 Continue 单独记录恢复请求、read hook/结果、加载稳定边界；后续换层或退出另起 operation/window。若 Continue 分支在静态/运行时未出现 save entry，就记录为本窗口未观察到写入口，同时保留 callback/间接边界 UNKNOWN。

## 建议的可观察入口清单

这些是 V16 要求“记录写入入口与条件”所需的观察点，不代表全部都能直接从 C/S DLL 稳定 Hook；在落地前应核 ABI 与官方 Hook 链，不改 readonly、不回滚、不清理保存状态。

1. 保留五个 ST8 PGD/映射入口；在日志增加 operation/window ID、run/session ID、loop/tick、nested-call/callback 关联，并允许 ST1–ST3 的 PGD counter/unlock/backup/cloud/local 作为明确记录的原生事件。
2. 增加/复用 native 观察：`Manager_SaveGameState` RVA `0x532160`、`Manager_LoadGameState` RVA `0x5332F0`、`GameState::write` RVA `0x59F7A0`、`GameState::Delete/DeleteRerun` RVA `0x59E7B0/0x59E870`，以及官方对应的 rerun/read Hook。记录 info 是否 known、slot/filename 类别、original 成功、条件分支与调用结果。按文件名/保存槽匿名化可避免把个人路径暴露进 trace。
3. 在 REPENTOGON SaveStateManagement 已知调用链加侧车事件，或使用可验证的文件 API/路径观察，至少列出：PGD `save_backups/`、本地/cloud 主保存；VirtualRoomSetManager `*.json`；ItemPoolManager state file；ESSM `<name>.dat` 与 `ex_<name>.json`；官方 achievements JSON；以上路径的 open/write/flush/close/delete 结果。不要把只观察 PGD API 等同于观测这些文件。
4. 记录官方 tracker/callback 触发边：Achievement event tracker、CompletionTracker、MC_POST_ENTITY_KILL (68)、MC_POST_ACHIEVEMENT_UNLOCK (1476)、ESSM Serialize/PreDeserialize/Deserialize callback。先记录回调是否注册、调用时刻与父事件，再由 callback 体是否调用其他持久 API/文件写决定额外条目；目前此闭包仍为 `UNKNOWN`。
5. ST1 窗口从 B Kill 请求持续到 ghost/终局/移除/超时结果；ST2 覆盖同步 `RevivePlayerGhost`、`EvaluateItems` 与结果稳定；ST3 覆盖 `StartStageTransition` 到 stage 稳定及关联 Manager/GameState 保存/删除；ST4 只覆盖 Continue restore/read 到加载稳定。跨 loop 的死亡和换层需要持久关联 token/窗口，而不是依赖只包住发起函数的 TLS scope。

## reducer 与计划更正建议

1. V16 从“必须没有写入”的单一断言改为“按窗口列出已观察写入、条件、结果，并按 D18/D20/D27 判定允许项”。将 C/S 新增持久写与原生允许写分开；`b_death`、`b_revive`、`stage_change` 的预期 counter/unlock 和 ST3 保存不能被 `no_added_writes` 的零变化条件误拒。
2. reducer 应把 `operation=none` 的 persistent enter/return 放到“未归属事件”栏并显式计数，不能从写入检查中静默丢弃。新窗口缺标签时应报告归属缺失，而不是按没有新增写入处理。
3. 将 observer completeness 由五个 Hook ID 扩展到本轮所需的 native 保存/删除和 sidecar/backup/JSON 事件；条件为未触发或路径未知时记录 `UNKNOWN/not observed`，不要以文件最后未变化补成 `no side effect`。
4. ST3 需要跨 loop window correlation；ST4 的 Continue read window 必须与后续 stage/exit save 分开。D27 不检查每日备份，备份记录不得新增验收限制。
5. 不声称当前静态地址图覆盖 Lua、虚调用、函数指针或 REPENTOGON Hook 的完整间接闭包；V16 运行时仍需验证真实 Hook/回调顺序、条件值和结果。

## 总体标签判断

- `STATIC_CONFIRMED`：五类 Hook 与实际 RVA；ST1 terminal death counter 分支；ST2 Revive 到 EvaluateItems 的调用；StageTransition_Update 到 Manager_SaveGameState 的直接边；Manager Save 到 GameState write/DeleteRerun/PGD backup/cloud/local 的目标版本指令；Continue Continue 分支的 Game::RestoreState 调用以及该分支无直接 Manager_Save xref；上述官方源码 writer/delete/条件顺序。
- `STATIC_INFERRED`：普通操作中每个条件是否实际成立；PGD 字段/容器字段在目标 build 中的完整业务含义；某个官方 tracker 对当前 fixture 的实际副作用；TLS tag 覆盖跨 loop 是否成功（现状显示没有此类标签）。
- `SOURCE`：REPENTOGON 源码中 trackers、Lua callback、sidecar、backup、JSON 与 file API 的实现意图及调用条件。
- `UNKNOWN`：目标运行时官方 Hook 安装/优先级、回调注册与执行、callback body 的全部副作用、间接/虚调用闭包、真实文件 API 成功情况、Continue 恢复期间任意间接持久化效果。
