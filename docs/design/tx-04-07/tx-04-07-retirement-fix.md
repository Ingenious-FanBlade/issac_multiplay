# tx-04-07 实质修复：S13 Stop 退休崩溃

日期：2026-10-25，America/Los_Angeles。用户在 e2e-02 后要求“修复该问题，然后继续测试”。本轮范围是定位并修复原生 B 退休调用的对象错误，完成后纯构建，再经本次实机准备确认执行一次完整 tx-04-07 E2E（e2e-03）。不在分析/修改中运行测试，不覆盖前两次失败，不自动重跑，不新增单元测试。

## 原始转储与静态依据

只读解析 e2e-02 原转储，见 [dump-analysis.json](../../artifacts/repentogon/tx-04-07-retirement-fix/dump-analysis.json)；[解析源码](../../artifacts/repentogon/tx-04-07-retirement-fix/read_dump.py)可对同一保留的转储重复读取，不启动/附着游戏。转储未包含的对象内存保持 null，没有猜补 vector 内容。

- 模块基址 0xDB0000，异常 EIP=0x1340CE5，即游戏 RVA 0x590CE5，code=0xC0000005，读取地址 0x01BEF000。
- ESI=0x7F17B040，与退休快照的 B 一致；EBX=0x0DF71EB8，与转储内 `*(base+0x7E4A1C)` 全局 Manager 完全一致。
- 当前 v1.9.7.12 IDB 在 VA 0x990CDC–0x990CEE 显示：从 `[ebx]`/`[ebx+4]` 读 begin/end，循环 `cmp [edx],esi`，故障正位于 RVA 0x590CE5 的该比较。EBX 在原函数入口保存 this，函数要求其指向 PlayerManager 的 Player vector；旧实现却从全局 Manager 取值传入。
- 官方 SDK `libzhl/functions/PlayerManager.zhl`：PlayerManager 的 `_playerList` 在 +0；`Game.zhl`：Game 的 `_playerManager` 在 +0x1BAA8，`GetPlayerManager()` 返回内嵌对象地址。RemoveCoPlayer ABI 仍为 thiscall、两个栈参数、retn 8，见既有 ST6。正确 this 是本次 Game 0x5095EEF0 +0x1BAA8 =0x5097A998，不是全局 Manager 0x0DF71EB8。

这组转储/指令/布局与源码相互对应，将前次结果中的准确故障指令与原因 UNKNOWN 更新为：**传错 this 导致原生移除遍历错误对象字段并越界读取**。前次自动报告和当时 UNKNOWN 原文保留，不回写旧证据。

## 修改

`RetirementPlayerSnapshot.manager` 改名为 `player_manager`，明确它是 Game 内嵌 PlayerManager。快照不再读取全局 Manager；在当前 Game 上检查地址加法与活动 vector，要求恰好 A/B 两项且与当前拓扑指针一致，再保存内嵌对象地址。无法确认时返回失败，不调用原生移除。

K 移出后复核 B 身份/controller/character，并复核当前 Game、同一内嵌 PlayerManager、A 身份/controller/character 与两人列表。身份变化时保留现场并报告冲突；明确 NotRun 或已知列表内 slot 缺席才走 Absent，不把 vector 不可读默认的 count=0 误判为已移除。原生调用继续只尝试一次，改传已复核的 `snapshot.player_manager`。snapshot/confirm 的 trace 增加 game/player_manager 地址，便于在后续 E2E 中直接复查。Hook 常驻、K 的 NEVER 策略、原生移除副作用和存档权限均沿用现有约定。

本轮不把 e2e-02 中其它 V 项改判通过：S4 移动、S5 炸弹资源和 skip 的局部失败仍保留，未执行的 Continue/跨房射击也仍待实际观察。

## 构建与唯一整体 E2E

使用 `scripts/build.ps1` 统一纯编译链接 Win32 DLL 与 i686 Rust harness；不带测试。构建完成后运行原 `harness.exe e2e tx-04-07` 入口，单次游戏启动、四轮 Host、S0–S21/V1–V16，产物 `tx-04-07-e2e-03`。每次启动前按现行规范第 8 节说明键盘接管并确认用户已准备好；SkipIntro=1，无需人工跳动画。沿用 D20 临时禁用/恢复 Download Helper，存档不回滚、无哈希计算。

统一纯构建已成功：Win32/v143/Release DLL 和 i686 Rust harness 均编译链接完成，Rust 无 warning，SDK 既有 warning 保留。第一次构建见 [build-01.log](../../artifacts/repentogon/tx-04-07-retirement-fix/build-01.log)；回读时补齐不可读与明确缺席的区分后，最终构建见 [build-02.log](../../artifacts/repentogon/tx-04-07-retirement-fix/build-02.log)。两次均为纯构建，修改期间未运行测试。

用户回复“已准备好，构建成功后开始”后只执行一次完整入口 E2E（e2e-03）：S0–S14 完成，S13 Stop 的原生 B 移除、K 移出、网络停止与 Idle 全部完成，PlayerManager 地址直接等于当前 Game+0x1BAA8。第二轮阿萨泻勒 B 也在失败收尾时正常移除；没有新增崩溃，游戏退出码 0，shutdown 的 trace 错误/截断均为零。

整体仍 **FAIL**：S14 Active 后、S15 前发生 b_animation_timeout。Lua 夹具把实体 Index=17 与 ordinal=1 比较后误标 B 不存在，缺少 animation_finished 字段；不能据此认为真实动画未结束。S15–S21 未执行，其它局部失败也保留。测试后未修改功能/夹具代码或再次启动，测试 mod/C/S 配置与 Download Helper 已恢复。详见[第三次结果](tx-04-07-results.md#退休修复后的第三次-e2e2026-10-25)和[本轮产物](../../artifacts/repentogon/tx-04-07-e2e-03/README.md)。
