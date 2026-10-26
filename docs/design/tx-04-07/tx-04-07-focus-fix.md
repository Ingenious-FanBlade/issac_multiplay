# tx-04-07 实质修复：测试期间的焦点维护与意外暂停

日期：2026-10-26，Asia/Shanghai。e2e-04 在失焦后暂停，S8.recovered 因 Authority tick 停止而超时。用户随后要求“继续刚刚的工作，然后继续测试”。本轮先完成前次结果记录，再修改 harness 的焦点维护和阻断诊断，纯构建后经本次准备确认执行一次完整入口 E2E（e2e-05）。修改期间不运行阶段测试，失败不自动重跑，旧四次结果保留。

## 已有直接依据

e2e-04 trace seq=24647、loop=2216：focus=0、console=0、pause=0，block_interval 进入1；后续 pause_state=1、A/B unbound、completed_tick 停在1298。失败现场游戏窗口 foreground=false。网络仍 Established，原生失焦屏蔽和暂停路径没有继续推进 Authority tick；30秒 script_timeout 不能准确报告该阻断。

旧 harness 只在物理按键和截图之前激活窗口，S7 长时间网络编排和 status 轮询没有焦点维护。失焦来源仍 UNKNOWN，本轮不将它归因为用户或某个外部程序。

## 修改范围

- win32 增加只读 focus_state，记录目标窗口、当前前台 HWND/PID、是否最小化，不采集其它窗口标题或内容。激活仍使用已有 focus 方法及其有界等待，不新增进程、注入或新键。
- Control 增加默认关闭的焦点目标；只有 tx07 显式设置为本次 Game 已核对的 GLFW 窗口。每次控制请求前若失焦，先记录 focus_lost，重新激活并记录 focus_restore 的实际结果；失败即返回错误，不在其它窗口发送后续按键。其它 tx 入口保留原行为。
- 持续模拟段使用 watch_running_tx07：稳定绑定、脚本、动画、clean/fault 窗口与短断网段检查 pause_state。刚恢复焦点时只在当前固定等待窗口内给原生状态最多500ms同步，仍暂停则记录 unexpected_pause 与真实 status，明确失败，不再等到30秒脚本超时。
- S6 的预期 console/pause、F8 加载动作、菜单与退休仍用普通观察，不被持续模拟检查误判。没有发 ESC 自动解除暂停，不改 PauseOnFocusLost，不重放旧 generation 或边沿，不延长/重跑故障区间，也不修改 V1–V16 通过标准。实际发生阻断的故障窗口仍保留为失败/未观察，不能因激活恢复而抹去原 trace。

DLL 和 Lua 夹具不变，沿用上轮已编译版本；本轮统一纯构建整个产物以保持部署入口一致。只读构建/源差异与 E2E 结果使用独立 artifact，不修改前次源码快照。

## 构建与唯一整体 E2E

纯构建日志保存至 `artifacts/repentogon/tx-04-07-focus-fix/`。本次实机准备确认后，通过 `harness.exe e2e tx-04-07` 一次游戏启动覆盖四轮 S0–S21/V1–V16，产物 `tx-04-07-e2e-05`；SkipIntro=1，第二档与动作沿用授权，Download Helper 按 D20 临时禁用并恢复，无哈希计算、存档回滚或只读位改写。

统一纯构建已成功，见 [build-01.log](../../artifacts/repentogon/tx-04-07-focus-fix/build-01.log)：Win32 DLL 与 i686 Rust harness 编译链接完成，Rust 无 warning，未执行阶段测试。用户回复“已准备好，构建成功后开始”后，已启动本轮一次完整入口 E2E（e2e-05）；实际结果随后追加。

本轮唯一 E2E 已结束，整体 **FAIL**：S0–S5 完成，S6 主动关闭计划内暂停后的 stable 等待读到 pause=1，新增 watch_running 检查立即报 unexpected_pause_in_running_scene。此处需要等待预期退出，不该直接归为意外暂停，属于本轮引入的 harness 回归；前文对稳定等待的适用范围实现不完整。后续应只对固定 Active 窗口要求始终无暂停，并单独保留预期退出的有界等待。

本轮 focus_lost/focus_restore 均0条，未获得焦点恢复实机样本，不能宣称该功能通过。S4 两方向、V8 投弹和 skip 局部检查再次通过。游戏正常退出、无新 dump、shutdown trace 错误为0，环境恢复；测试后未修改功能或自动重跑。见[第五次结果](tx-04-07-results.md#焦点维护修改后的第五次-e2e2026-10-26asiashanghai)与[产物](../../artifacts/repentogon/tx-04-07-e2e-05/README.md)。
