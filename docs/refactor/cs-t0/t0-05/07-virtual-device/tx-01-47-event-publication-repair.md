# tx-01-47：修复事件发布参数传递

日期：2026-10-16。用户要求检查并修复 tx-01-46 的远程初始化崩溃，并已确认重新准备好隔离 Offline 初始仅 A 场景，修复完成后可以验收。沿用现有全部实机操作授权。

## 实现前失败判据和验收方式

分析、修改期间不编译或试跑。代码完成后，通过统一入口一次构建并进入实机 E2E；直接使用桌面权限，避免已确认的沙箱进程不可见问题。仍使用 [tx-01-45 的整体场景](tx-01-45-e2e.md)，不因修复初始化而声称 Join、双端输入和退休恢复已经通过。

本轮重点失败方式：发布器与调用者仍不一致、引用指向失效临时对象、复制越界或改动发布标记、同一保留序号被重复使用、读取半发布事件、初始化或后续发布再次 AV，以及恢复未确认。新编译产物中的实际调用应传入有效 Event 地址；Installed 与后续事件应能被控制器读取，编码仍为 128 字节。运行中缺少必要场景记 NOT_OBSERVED；异常、事件丢失或恢复不确定记 FAIL，失败不自动重跑。

新产物目录为 `artifacts/cs-t0/tx-01-47-live-01`。统一入口保留构建日志、工具版本、独立对象/PDB/DLL/helper、场景配置，另保存 native-disassembly.txt 以复核实际参数传递。运行阶段保留原有 events/loop-events/operations/helper/restoration/result；不计算哈希。若崩溃，只收集已生成的系统记录并正常清理一次，不重启、再次注入或强制卸载。

## 旧产物静态证据

仅离线读取 tx-01-46 已生成 DLL，未再次运行它：

- `Publish(Event event)` 为 128 字节、alignas(8) 的按值参数。二进制 Publish 位于 RVA 0x3590。
- 安装完成事件的调用者 RVA 0x81F2 设置 `ECX=0x20`，0x81FE 设置 `EDI=ESP`，0x8200 `rep movs` 将 128 字节复制到参数栈并令 ECX=0；0x8202 直接 call Publish，返回后清理 0x80 字节参数。
- Publish 在 0x35AE 将 ECX 保存到 `[EBP-0x18]`；0x371C 又把该槽读入 ESI，随后加 8，在 0x372D 从 `[ESI]` 复制 120 字节到事件槽。因此这个已落盘调用序列会读取地址 0x8，和 Windows 记录的 AV 指令位置完全一致。
- 多个其他调用者也把完整 Event 放到栈上；callee 却读取 ECX 指针。此处调用者/被调者不一致是可直接读取的机器码事实。没有上一轮现场寄存器或 dump，不能外推成所有异常的唯一解释。

工程使用 `/GL` + `/LTCG` + `/O2`。Microsoft 说明 x86 LTCG 可以改写内部调用约定（包括使用寄存器），并且参数对齐受调用约定控制：[LTCG](https://learn.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-170)、[align](https://learn.microsoft.com/en-us/cpp/cpp/align-cpp?view=msvc-170)。文档并未确认本机这个具体编译器问题；本轮依据是本机生成的实际指令。

## 修复

将 Publish 参数改为 `const Event&`，同步复制的调用生命周期覆盖整个函数，不再通过按值传递对齐的 128 字节 aggregate。保持发布序号、容量、IPC 大小/字段及 payload 复制范围，采用既有 loop/active 发布器相同的引用接口。保留 Release 优化与 LTCG，不通过关闭优化隐藏调用差异。

## 验收结果

**本轮事件发布崩溃修复已观察有效；完整 Virtual Device E2E 为 NOT_OBSERVED，入口退出 1，没有重跑。**

统一入口以桌面权限执行一次，Rust、Native、helper 构建成功，仍使用 v143/Hostx64→Win32、Release/LTCG 与全新对象/PDB。新 DLL 中 Publish 的入口仍为 RVA 0x3590，但初始化调用者已变为 RVA 0x7FE2 `lea ecx,[esp+88h]`，0x7FE9 call Publish。ECX 现在是本地 Event 地址；不再出现旧初始化调用者把 ECX 清零后直接调用的指令序列。原始反汇编已保存。

实机 PID 40580，creation_time_100ns=134366340500009954。helper 的 initialize 与正常 resident-stop 均返回成功。窗口共约 173 秒，Authority 完成 3,428 tick，控制器读取 120,032 条事件；首条为 Installed，最后一条为 47 槽恢复汇总。没有再发生上轮 Publish 访问地址 0x8 的初始化崩溃；这是本轮窗口内的结果，不外推所有路径。

实际操作：发送 D+UP 600ms、A+RIGHT 600ms 两段物理组合键（工具逐段释放按键）；ESCAPE 暂停、ESCAPE 恢复；用户通过菜单退回主菜单并保留进程；控制器 `stop` 正常恢复。原生事件直接观察到暂停、恢复、run teardown。只发送物理键不等于 C/S 输入隔离已通过。

| 项目 | 本轮结果与证据 |
| --- | --- |
| 初始化/事件发布崩溃修复 | PASS（限定本轮），Installed sequence 1，后续持续发布至 120032 |
| 新调用者传入有效引用 | 静态已确认，native-disassembly.txt 中 RVA 0x7FE2/0x7FE9 与 Publish 入口 |
| 发布/消费一致性 | nativeError=0、transportFailure=null；报告中重复 Resolve/commit/consumer/accept 与 inconsistentQuery 均 false |
| 暂停/恢复 | PASS，事件 69518–69521；返回菜单前另有暂停事件 119980/119981 |
| 原生 run teardown | PASS，事件 119982/119983（Lifecycle detail=9） |
| 正常恢复 | PASS，47 个原始字节比对事件 119985–120031，汇总 120032；nativeRestored=true，helper stop 成功 |
| 游戏和 DLL | shutdown 后同一 PID 40580 仍运行；helper resident-stop 检查 DLL 仍加载。pinned=false，因为未进入设备构造，不声称已验证构造前 pin/常驻 root 用例 |
| A/B 绑定、设备注册、Join、输入隔离、B-only retirement | NOT_OBSERVED，没有对应 Binding/DeviceState/Joined 证据 |
| 全部功能 E2E | NOT_OBSERVED，退出 1；不能标记 T0-05 完成 |

自动报告中的 missing-input-neutral 为 PASS，只观察了当前无绑定条件下的中性 Resolve，不能据此认定“已唯一绑定后缺输入”的完整场景已经通过。按键期间未观察到 A physical loopback。Repeated query 的有效可比条件也未观察到。

本次暴露的后续问题：`ReadTopology` 在运行窗口没有确认普通 A 拓扑，Authority 因此反复记录 Rebind/Invalidate；事件 8/9 是首批证据。由于当前日志没有记录具体失败分支，不能断言是 vtable、ordinal、run/Online 条件或 vector 中哪项拒绝。未在验收途中修改判断、猜写对象或重复注入；这个绑定诊断与修复仍待后续处理。

产物：[result.json](../../../../../artifacts/cs-t0/tx-01-47-live-01/result.json)、[helper.log](../../../../../artifacts/cs-t0/tx-01-47-live-01/capture/helper.log)、[restoration.json](../../../../../artifacts/cs-t0/tx-01-47-live-01/capture/restoration.json)、[events.jsonl](../../../../../artifacts/cs-t0/tx-01-47-live-01/capture/events.jsonl)、[native-disassembly.txt](../../../../../artifacts/cs-t0/tx-01-47-live-01/native-disassembly.txt)。人工/工具操作及修复结论补记在 desktop-operations.jsonl 和 assessment.json，不替代原生事件。
