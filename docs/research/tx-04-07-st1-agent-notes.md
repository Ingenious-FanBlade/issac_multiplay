# tx-04-07 ST1 静态逆向笔记

> **主智能体审阅批注（2026-10-25）**：下方原文保留。`0x7995C8–0x7995CE` 实际为 `cmp esi,dword_BEE6D4; jz loc_7995E6`，检查临时死亡组迭代指针与 end，**不是“排除 Main”**。PlayerManager 的 dead 比较指令起点为 `0x9926CC`，原文 `0x9926D0` 位于指令内部。HUD ghost 分支返回布局结果并跳过常规心绘制，不代表已验证最终画面。以[主审报告](tx-04-07-static-confirmation.md)第 3、4、6 节为最终结论。

本笔记仅记录 v1.9.7.12.J273 的静态证据与推导，不代表运行观察。数据库为 `C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，输入模块 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`。VA `0xB2EEA0` 的版本字符串为 `Binding of Isaac: Repentance+ v1.9.7.12.J273`。未读取或校验哈希。

## ST1 结论与证据

- **夹具建议用 Lua `Kill()`。** 注册点 RVA `0x44F6D2` 推入 wrapper `0x844230`，`0x44F6D7` 引用 `Kill`；wrapper 在 `0x84428D` 通过实体虚表 `+0x24` 分派，Player 对应实现到 `Entity::Kill` VA `0x6A7ED0` / RVA `0x2A7ED0`。该函数在 `0x6A7EEA` 将 `this+0x173` 设为 1（dead 标志），自身没有直接调用 `TriggerDeath`。致命伤害还会经过 `TryPreventDeath` 等免死/复活分支，故不适合作为确定触发此路径的首选夹具。Lua wrapper 原始导出见 `artifacts/repentogon/tx-04-07-static/st1-review/Lua_EntityKill.json`；Kill 汇编和伪代码见本代理导出 `artifacts/repentogon/tx-04-07-static/st1-agent/ida-core-functions.json`。

- **Kill 不等于立即 TriggerDeath。** `PlayerManager::Update` VA `0x992650` / RVA `0x592650` 在 `0x9926D0` 检查 dead 标志；随后检查 `+0x7C/+0x8C` 条件，并沿 `+0x1E4C` 关联对象检查对应条件；条件放行后才在 `0x9926FD–0x9926FF` 调用 `TriggerDeath(false)` VA `0x7992A0` / RVA `0x3992A0`。因此应等待原生死亡动画及关联 twin 动画条件满足，可能跨多个 Manager 更新周期；不能假定 Kill 后下一次更新必然派发。官方 ExtraLives 源码注释也说明常规 `TriggerDeath` 在 PlayerManager 更新中针对已 dead 且不再播放动画的玩家调用（`REPENTOGON-1.1.3/repentogon/Patches/ExtraLives.cpp:30-31`）。

- **普通非 Main co-op 玩家会转 ghost，计数不靠删列表项。** `TriggerDeath` 在 `0x799586` 检查首玩家 dead，在 `0x79958F` 检查关联字段；`0x7995A8` 调用 `IsCoopPlay`，`0x7995B5` 调用 `CountNonGhost`，`0x7995BA–0x7995BD` 要求结果大于 1；再由 `0x7995BF` 排除已有 ghost、`0x7995C8–0x7995CE` 排除 Main，最后 `0x7995D6` 调 `MorphToCoopGhost` VA `0x7CFFA0` / RVA `0x3CFFA0`。这里普通角色字段必须区分：Entity Type=1、Variant=0、playerType=0；`MorphToCoopGhost` 在 `player+0x13BC` 检查 playerType，playerType=35 走特殊 `RemoveCoPlayer`/关联引用清理分支，不能误写成 Entity Type=35。普通 playerType=0 分支未见从 PlayerManager 列表移除；`CountNonGhost` VA `0x996A00` / RVA `0x596A00` 遍历列表并只在 `player+0x207D` ghost 标志为 0 时计数，因此 ghost 可留在列表但不计入非 ghost 数。对应主审原始证据见 `st1-review/Player_TriggerDeath.json`、`Player_MorphToCoopGhost.json`、`PlayerManager_CountNonGhost.json`。

- **Morph 会清 dead 并设 ghost；HUD 跳过普通心显示。** `MorphToCoopGhost` 在 `0x7D0021` 调 `Revive` VA `0x79A400`，随后在 `0x7D02C0` 将 `player+0x207D` 置 1。`PlayerHUD::RenderHearts` VA `0x834930` 在 `0x834BBD` 检查同一标志，ghost 分支于 `0x834BDF` 跳到常规心绘制之后。Morph 中未识别到改写 controller/player index 或通用重分配调用。设备通知总体 **UNKNOWN**：普通局部未识别到通用设备注册、移除或重分配入口；但 `0x7D0058–0x7D006A` 在 `Game+0x25B8C >= 2` 时对 `Game+0x25B98` 的虚表 `+0x60` 间接调用，接收对象身份未确认。虚拟键/输入状态通知也保持 UNKNOWN。HUD 与设备的补充原始证据见 `st1-review/PlayerHUD_RenderHearts.json` 和 `st1-review/supplement-details.json`。

- **死亡统计有条件，其他持久写风险不能排除。** 普通 ghost 路径中 `Revive` 清 dead，所以 `TriggerDeath` 后续 `0x7995ED` 的 dead 检查不通过，绕开 `0x79A141 → 0x993120` 的死亡记账分支。`0x993120` 在 `0x99313F–0x993147` 调 `IncreaseEventCounter(10,1)` 并写管理器字段；不能据此声称所有死亡都不计数，全员死亡及其他分支不在此否定范围。另 `MorphToCoopGhost` 在 `0x7D02D5` 调 `EvaluateItems` VA `0x75D4B0`，其条件分支可调用多个 `TryUnlock`（含 id 144、19、58、370、355、361）；故普通 B ghost 窗口仍存在条件性持久写风险。`0x7D004D` 的 `sub_90A3F0(112,1)` 是 RemoveNullEffect 调用，不是持久 event。REPENTOGON 的 `Entity::Kill` hook 还会触发 `PostEntityKill` callback（`repentogon/LuaInterfaces/CustomCallbacks.cpp:6500-6520`），callback 的外部效果不在本次原生静态闭包中。原始支撑见 `st1-review/PlayerManager_DeathHandler.json`、`Persistent_IncreaseEventCounter.json`、`Player_EvaluateItems.json`、`TemporaryEffects_RemoveNullEffect.json`。

## UNKNOWN 边界

未确认 Morph 的间接虚调用对象身份、设备通知语义、虚拟输入状态副作用、callback/mod 可能进行的持久写，以及全员死亡等非普通 ghost 分支的完整统计闭包。上述均为静态反汇编推导，不是实机验证。
