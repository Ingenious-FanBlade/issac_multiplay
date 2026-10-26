# tx-04-07 实质修复：预期暂停退出的稳定等待

日期：2026-10-26，Asia/Shanghai。用户在 e2e-05 后要求“修复，然后继续测试”。本轮修复新增 harness 检查造成的 S6 回归，保持真实 Active 窗口的意外暂停诊断与焦点维护；纯构建后经本次准备确认执行一次完整入口 E2E（e2e-06）。修改期间不运行阶段测试，不新增单元测试，不覆盖前五次结果，不自动重跑。

## 原因与修改

e2e-05 的 S6.pause.end 为计划内暂停；发出关闭暂停的 ESC 后，stable_tx07 第一次轮询仍读到 pause_state=1、blocks=1。前轮把 stable 的观察改成了 watch_running，普通计划内退出又没有 focus_restore 的同步等待，因而在原生恢复/重绑前就终止。该事实不证明游戏永远无法退出暂停。

本轮将 stable_tx07 恢复为普通 watch_tx07：允许在既有45秒上限内观察预期 pause/loading 退出，不再立即套用“必须始终运行”的条件。成功条件仍为 Active、blocks=0、A/B bound=1、实际 generation（需要时严格递增）与远端新 Ack 一致，并明确要求 pause_state=0；不提前返回、不伪造重绑。

脚本、动画、S7 clean/fault、S8短断网等已进入持续模拟的窗口继续使用 watch_running，保留意外暂停失败与实际记录；S6的键序列不变，不自动多发 ESC，不改失焦设置、不重放输入，不延长故障窗口。Control 的本次游戏窗口焦点维护不变。DLL、Lua 与 V1–V16 归约标准均不修改。

## 构建与唯一 E2E

统一纯构建 DLL 与 i686 Rust harness，日志保存至 `artifacts/repentogon/tx-04-07-pause-wait-fix/`。收到本次键鼠接管准备确认后，原 `harness.exe e2e tx-04-07` 单入口、一次游戏启动覆盖四轮 S0–S21/V1–V16，产物 `tx-04-07-e2e-06`。第二档及动作沿用授权、SkipIntro=1，Download Helper 按 D20 临时禁用并恢复，无哈希、存档回滚或只读位改写。若失败，记录实际原因及未运行项，不自动再启动。

统一纯构建已成功，见 [build-01.log](../../artifacts/repentogon/tx-04-07-pause-wait-fix/build-01.log)：Win32 DLL 与 i686 Rust harness 编译链接完成，Rust 无 warning，未运行测试或部署。已回读 stable 成功条件和持续运行检查的调用范围，源差异见 [changes.patch](../../artifacts/repentogon/tx-04-07-pause-wait-fix/changes.patch)。用户本次回复“已准备好，构建成功后开始”后已启动一次完整入口 E2E（e2e-06），结果随后追加。

本轮唯一E2E已结束：S6预期退出暂停实际完成，V9整项PASS；S0–S15执行完成，B死亡/复活V11通过，第二轮角色7的Entity Index=46 / player_index=1 / animation_finished=true正常读取。整体仍 **FAIL**：第二轮主动Bye后主机已正常PeerBye退休，但UDP relay接收报10054，finish失败，S17–S21未执行。V12仍NOT_OBSERVED，缺少所需激光证据，不能把S15编排完成算作跨房射击通过。

游戏正常退出、无新dump、shutdown trace错误为0，测试环境恢复；无自动重跑或测试后功能修改。详见[第六次结果](tx-04-07-results.md#暂停等待修复后的第六次-e2e2026-10-26asiashanghai)与[产物](../../artifacts/repentogon/tx-04-07-e2e-06/README.md)。
