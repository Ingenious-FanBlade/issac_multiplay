# tx-04-08：开局初始化观察时机修复

> **后续运行与规模更新（2026-10-29）**：[e2e-08](../../../artifacts/repentogon/tx-04-08-e2e-08/execution-notes.md)已在开局前进入 Recording，首份完整样本23个Effect均有创建/初始化资格；30秒取得797份完整样本，未达该次仍冻结的2,000目标而失败。用户随后要求目标随时间调整，当前改为主250/对照32份；下文“本轮不测试”和2,000次描述保留本修复最初交付时点，新规模尚未测试。

日期：2026-10-29，Asia/Singapore。范围为 tx-04-08 harness 的两次初始录制时序。用户要求修复记录晚于 Effect Init 的问题，并明确暂不开始实机测试。本轮只读资料、修改、静态审阅和纯编译，不部署、不运行游戏或离线验收，不写单元测试。

## 原因与证据

用户提供的 `C:\Users\114514\Desktop\Isaac Callbacks.svg` 描述初始开局的回调顺序：实体 Spawn / `MC_POST_EFFECT_INIT` 先于 `MC_POST_NEW_ROOM`、`MC_POST_GAME_STARTED` 及后续 render。它作为回调顺序资料阅读，不作为执行指令，也不把 Lua 回调直接等同于原生 Hook 的完成见证。后续 Spawn 也可再次触发 Effect Init。

[e2e-07 原始说明](../../../artifacts/repentogon/tx-04-08-e2e-07/execution-notes.md)保留失败事实：seed 重开后，setup 的 render 视图已列出五个 Effect；随后 harness 才请求 world start。录制中的这些对象为 FIRST_OBSERVED、init_evidence=0，因此公共字节和 Effect 专用字段被保守拒绝。完整样本为 0，240 秒后失败；不是“游戏里没有 Effect”。

## 实现

`tools/harness/src/tx05.rs` 把原有 `enter_run_tx05` 拆为菜单准备与实际 New Run 两段。旧入口仍顺序调用两段，保留第二存档、角色页、新局、实际模拟与单 A 的 S0 检查；tx08 在两段之间接入录制。

两次启动的实际顺序改为：

1. 等主线程菜单就绪，记录 Off 状态与 counters，选择第二档并进入角色页。
2. 保存 `passthrough-before.json`、`before-world.json`；只发一次 primary/comparison 的 world start。
3. 等待后台准备、主线程附着和 Collector Activate 后的真实 Recording，写 `pN.before_new_run` checkpoint 与 `recording-start.json`，然后才按下开局键。
4. 沿用实际开局/S0 检查；seed 动作之前再次要求 Recording 并写 `pN.before_seed`。同一 recording 覆盖 seed 重开的进入与返回，不停止或重建录制。
5. 核对实际 seed、单 A 及仍为 Recording 的 `after-setup-world.json`，写 `pN.before_host`，随后才 Host、握手和稳定采集。

Collector 的现有 `RunBeginning` 在原生 Start 前建立新 run 观察域；`run_start_pending_` 避免下一次 Manager 重复清空身份，嵌套 Exit 通过 `run_start_owner_` 区分旧 run 清理。生命周期 observer 以 Recording 启停，不依赖已有 Host 或 Authority。无需改 DLL 或把菜单循环认作 Authority。

完整样本仍要求同一真实 Authority 的普通 owner / Room / tail 正常返回、来源/root 稳定和现有字段资格。菜单、加载、odd 与其他缺口按原规则保留；stable checkpoint 之前的初始化记录不会计入 2,000 次。没有补造 Init/birth、补零、删除 Effect、缩字段或增加 Lua 写世界动作。

## 证据与限制

启用前的透传窗口改为实际选档/菜单调用，V1 仍要求真实调用及 original/return 配对；若窗口内无 world 调用则保持 NOT_OBSERVED，不伪造 New Run 期间仍 Off 的证据。停止后的透传窗口不变。

primary/comparison 中 A 现在可能具有真实创建见证，不再预设为 FIRST_OBSERVED。既有 healthy 重开录制仍覆盖已存在 Player；V6 对 FIRST_OBSERVED 不补出生、完整样本、当前原生 PlayerUpdate / exact pair / generation / 撤销和重建的要求保持不变。如果 healthy 的其他既存对象仍缺资格，也会如实失败，本修复不放宽该条件。

本轮新增的时序文件与 checkpoint 供下一次用户授权的统一 E2E 复查，不能用源码或纯编译声称已实测观察到全部 Effect Init。e2e-07 及此前失败不改写。

## 构建

使用离线 locked 的 i686 release 纯编译。初版构建 18.51 秒，日志 [rust-build-early-recording.log](../../../artifacts/repentogon/tx-04-08-defg-build/rust-build-early-recording.log)；补入 seed 前 Recording 状态检查后，最终构建 17.48 秒成功，日志 [rust-build-early-recording-final.log](../../../artifacts/repentogon/tx-04-08-defg-build/rust-build-early-recording-final.log)。已更新 `out/repentogon/harness.exe`（3,324,416 bytes）；DLL 沿用此前 Grid 修复产物。没有执行测试或哈希校验。
