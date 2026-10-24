# tx-01-80 可移除测试场景

仅用于用户授权的隔离本地 seeded run；原版 Online / Isaac Sync 实机隔离与全量 replay 均已由用户排除。Mod 不读写存档、不调用 Sync API、不覆盖 MC_INPUT_ACTION；从 run 开始被动记录实际 seed、Game frame、房间与 0..11 的输入值，测试世界操作必须逐条输入控制台命令后，在下一次自然 MC_POST_UPDATE 执行。不要在正式游玩存档或联机会话中启用。

将本目录作为新的 `mods/isaac-cs-world-e2e/` 安装、在 Mod 菜单启用，并关闭其它场景/输入覆盖 Mod。测试完成后退出游戏，在 Mod 菜单禁用并移除本目录即可。不要覆盖已有同名目录。

## primary 操作顺序

1. 全新游戏进程、全新 seeded run，A 已生成；启动执行器，等 B 创建完成后输入 `cswe2e enable`。操作前后在执行器中 `observe <label>` 保存独立 header/tick 标记。
2. `cswe2e scene`：一个 Pickup Init 内有界嵌套一个 Effect；生成后立即移除另一个短命 Effect；保留 NPC/Pickup 十个自然 Game frame 后 Morph，再十帧后清理。另一个 Bomb 保留四帧供未知类别采样，之后每自然帧生成并 Remove 一个 Bomb，共 256 个。当前工厂 case 4 的池为 64 槽（6F0239 的 `&0x3F`），只使用正常空闲复用，不强制覆盖活对象或引爆；等日志 `reuse_finished`。
3. `cswe2e grid-spawn`，自然运行几帧，`cswe2e grid-remove`，再等几帧，`cswe2e grid-respawn`。位置为房间中心右侧 80；已有 Grid 时拒绝，不覆盖未知对象。不要递归调用 Room.Update。
4. `cswe2e esau-equip`；依次执行 `cswe2e esau-use-1`、`cswe2e esau-use-2`、`cswe2e esau-use-3`，每次间隔数秒。只操作 A 的正常 Esau Jr. 活动道具，保存真实保留 Player/Replace/重新发布记录。若游戏或 Native 报错立即正常停止，不自动重试。
5. 标记并人工暂停 2–3 秒、确认暂停 tick 后恢复；标记并正常通过门换房。保存 A 的 FIRST_OBSERVED、exact-pair 撤销及重新正常 PlayerUpdate 的证据。
6. `cswe2e hourglass-equip` 后标记并人工普通换房，过渡结束后 `cswe2e hourglass-use`；自然运行数秒，保存实际 Restore 和 loading/lifecycle/gap，而非仅凭道具动画。
7. 在健康采集中，每个非终止故障执行一次 `fault object-budget`、`fault membership-budget`、`fault relation-budget`、`fault reference-ambiguity`。关系歧义需要保留 Player 的非 NULL 引用。保持游戏前台运行；命令有 12 秒取证窗口。
8. 标记后正常返回主菜单（不结束进程），执行器正常 `stop`，检查 70 原始字节恢复。Mod 的单次 action 失败会禁用本 run 的后续世界操作，保留 fixture_error，不重跑。

## 两个严格对齐场景

各使用完全重启后的新游戏进程、新 seeded run，同角色/难度/seed/初始房间。自 run 开始保持实际 gameplay 输入中性，B 输入脚本为空。不要进行 primary 的任何 scene、道具或换房操作。

采集启动后 `cswe2e enable`，再输入 `cswe2e align <未来绝对 Game frame>`；两边使用同一绝对 frame，至少提前 120 帧。越过该帧后停止。执行器要求完整原始输入帧序列、实际 canonical Submit/Resolve 的中性证据、同 seed/房间、唯一普通样本的精确 native Game frame 和实际 B 创建事件，再提出明确的 Authority tick 与出生映射。B 的出生 counter 单独保存；若 Join 时机不同，保留这项已知场景差异，不补造相同初始世界或声称全局确定性。无准确普通样本不按最近帧补齐。用户确认具体映射及范围后调用一次生产 world-compare；更早未采集世界、确定差异与未映射对象均保留。

## 终止观察故障

`field-read`、`source-changed`、`event-overflow`、`wrong-thread`、`root-changed` 各用一个全新用户启动的游戏进程，在同一次统一验收内依次完成。实际发生健康采样后才触发一次，采集器停用后要求独立观察到 scheduler 继续推进、coop error=0，随后正常停止、核对 70 槽恢复。

field-read 只读取 collector 自有 PAGE_NOACCESS 页以经过真实 guarded Read 异常分支，不破坏游戏指针，不冒称实际游戏对象已失效。其余受控故障仅进入 collector 的预算、稳定性或引用分支。wrong-thread 使用自有 worker 调用实际线程检查，在对象读取前被拒绝；保存 worker/main ID。已通过的 reader 背压和 controller-loss 证据继续保留原出处，不重演。

## 依据与产物

API 已按 [Game.Spawn](https://wofsauge.github.io/IsaacDocs/rep/Game.html#spawn)、[NPC](https://wofsauge.github.io/IsaacDocs/rep/EntityNPC.html)、[Pickup](https://wofsauge.github.io/IsaacDocs/rep/EntityPickup.html)、[Player](https://wofsauge.github.io/IsaacDocs/rep/EntityPlayer.html)、[Room](https://wofsauge.github.io/IsaacDocs/rep/Room.html) 和 [Input.GetActionValue](https://wofsauge.github.io/IsaacDocs/rep/Input.html#getactionvalue) 核对。使用显式非零 Game.Spawn seed，不调用 Random。

保留游戏 log.txt 的 `[CSWORLD-E2E-v1]` JSON、执行器操作、实际 seed/input/scenario、原始 probe GPR/FLAGS/FX/参数与 RNG 字节、world 样本/事件、manifest、对齐文件、比较结果、故障独立 header 和实际恢复字节。脚本完成不等同于对应原生 Hook 被覆盖。
