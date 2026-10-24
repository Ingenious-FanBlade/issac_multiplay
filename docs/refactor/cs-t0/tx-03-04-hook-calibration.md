# tx-03-04：Native 全部 Hook 点位校正

日期：2026-10-21（Asia/Shanghai）。状态：固定点位校正完成；唯一文件/源码验收 **165/165 PASS**，Native/WorldE2E Native/Helper/T09 控制器编译成功。当前未找到游戏，只读内存采集及实机 Hook 安装/恢复未运行。见 [实际结果](tx-03-04-hook-calibration-results.md)。

## 目标与范围

按用户要求核对 `cs-t0/native` 的所有实际 Hook 安装点，包括独立历史模式、当前 C/S 主循环、输入/动作、Join、Lua/IAT、可选世界对象入口；必要时连同这些点位直接使用的原生函数、返回地址及对象布局一起修正。预期结果是可审阅的全量点位清单、静态逆向依据、针对性实现及一次整体离线验收产物。

用户原指定 GPT 6.1 Luna / max；子智能体工具返回不支持该模型后，用户明确选择 GPT 6 Luna / max。该子智能体负责静态逆向，主智能体负责清单、实现及整体验收。

现行项目规范优先：不设 gate，不校验哈希；分析和修改期间不运行测试；最终只执行一次统一验收。该次验收额外只读采集唯一运行中的游戏之模块、点位字节和 IAT 指向（不附着调试器、不暂停线程）。此次不注入或操控真实游戏，不沿用旧实机授权为新实验授权。静态结果不外推为实机线程/时序/恢复已通过，也不修改 tx-03-03 的失败事实。

## 修改与验收前登记的失败方式

以下为本次范围内须考虑的失败方式，先于实现及离线验收代码登记：

1. 当前 IDA 数据库仍指向旧文件，未能代表用户的新脱壳镜像；路径、PE 布局与读取来源必须明确。
2. 把 VA 当 RVA、错误使用加载基址，或将原始指针与重定位后的指针直接比较。
3. call RVA 漂移、起点落在指令中间、原始字节不符、rel32 解析目标不符。
4. 多模块复制的同一 Hook 常量只改一处，返回地址筛选、无创建分支和相关调用者仍用旧值。
5. 虚表、COL、槽偏移或原函数目标不符；原对象虚表与自有设备虚表混用。
6. IAT slot RVA、导入名或实际调用形式不符；脱壳后 OriginalFirstThunk 缺失或导入名称丢失。
7. 误接受同名但错误 DLL 的导入，或把已被其它模块替换的指针作为原函数。
8. Hook ABI 对寄存器、栈参数数量、清栈方和返回值的理解错误；x87/SSE 返回值被破坏。
9. 入口 trampoline 截断指令，复制了相对寻址/跳转而未重定位，或跳回地址错误。
10. naked wrapper 未完整保留原调用所需寄存器、标志或栈，对齐或返回时栈不平衡。
11. 一个逻辑点有多个调用者，仅核对了其中一个；可选 world Hook 未纳入清单。
12. 点位相符但对象字段、类型、生命周期或本地/在线隔离语义已变化。
13. 安装、部分失败回滚及恢复未使用同一 owner/事务，或漏掉新增槽。
14. 未知点位通过宽松字节掩码、猜测地址或关闭校验继续运行。
15. 准备失败仅返回笼统错误，无法记录实际阶段、索引或具体点位。
16. 验收源于与实现相同的自造字节，未读取真实目标 PE 或独立 IDA 指令依据。
17. 离线执行误加载/调用游戏代码，或间接执行旧脚本中的测试/哈希校验。
18. 构建失败、证据缺失、未覆盖 ABI 被误报为通过；失败后自动重跑掩盖首次结果。

## 验收边界

在逆向与修改全部完成后，以一个统一入口执行一次整体离线验收，保存实际目标 PE 的点位观察、原始字节/指令、构建日志与机器可读结果。仅使用纯编译入口，不运行旧 `build.ps1`（该入口包含多组测试与哈希操作）。若需要新的真实注入/恢复实验，另行明确具体操作；本次离线结果不冒充该实验。

## 实现中的校正

- `runtime/input/boundary/raw_apply` 四个旧模式原先仅接受 `SizeOfImage=0x92C000`，会拒绝当前 5 节脱壳文件 `0x8FE000`。现在统一使用已有 Canonical / ApprovedUnpacked 精确 profile；`ApprovedImageSize` 在准入成功时返回实际尺寸，供 input/boundary/raw 的 RVA 边界使用。不向这些模式扩展 T09 专用 experimental profiles。
- 输入观测 11 槽与 RawApply 12 槽原先在准备阶段检查精确目标，但提交给补丁事务时只设置 owner；现在同时传入逐槽 `expected_original`，安装事务准备及暂停线程后的 Apply 都复核该精确目标。两模式共享的点位保留各自隔离的状态与 owner。
- 主循环 call/IAT/COL/query 失败记录对应点位 RVA；image/conflict 的初始化早退也发布现有 Failure 事件。补丁事务的其他失败保留原阶段/索引/Win32 错误，尚无确切点位时 RVA 为 0。
- C/S Failure 的 `query` 保存 `StartEmbeddedObserver` 的原始非零返回码；T0-09 的 loop JSONL 增加既有 `value_bits` 的 `valueBits` 输出，防止安装索引在落盘时丢失。IPC 布局与版本未变化。
- 对 loop Failure，`action` 是安装阶段，`valueBits` 是失败索引，`deviceKey` 是 Win32 错误，`returnRva` 在已知时表示拒绝的点位。image/conflict 早退无槽索引，以 `UINT32_MAX` 标记；相应 errorCode 区分 2（image）和 3（conflict）。
- tx-03-03 live-05 的具体根因保留为 UNKNOWN；历史原文附加澄清，不改写失败产物。当前已核对点位相符，不猜测替换 RVA 或放松字节检查。

统一入口：`cs-t0/accept-native-hook-calibration.py`。它读取源码声明和磁盘 PE，逐条记录实际字节/目标与导入布局，使用 `--runtime-readonly` 时再以 `PROCESS_QUERY_INFORMATION | PROCESS_VM_READ` 句柄读取唯一游戏的模块、点位和 IAT，并纯编译普通 Native、WorldE2E Native、Helper 及 T09 Rust 控制器；它不会加载或调用游戏代码。产物保存目标文件的 `.bin` 副本、源码快照、点位 JSON、可选运行时只读快照、构建日志及 summary；新目录 create-new，失败不重试。静态/构建结果与当前进程观察分开记载，静态 PASS 不代表实机可安装或 Hook 恢复实验 PASS。

## 静态证据入口

- [Luna 逆向记录](tx-03-04-hook-calibration-reverse.md)：全家族覆盖、ABI 和仍未知的动态模块边界。
- `artifacts/cs-t0/tx-03-04-static/hook-observations.json`：Luna 点位目录。
- `artifacts/cs-t0/tx-03-04-static/root-ida-raw.json`：主智能体补充的 21 个历史/间接调用原始字节和指令，以及 world19、setter、manager 的入口及全部原生 return 指令。
- `artifacts/cs-t0/tx-03-04-static/root-ida-abi.json`：另 46 个共享/新增目标函数的入口和原生 return；IDA 类型仅作提示，不替代指令依据。
- `artifacts/cs-t0/tx-03-04-static/before/`：本轮编辑前文件副本，用于区分工作区原有改动与本轮修改；没有撤销其他任务的内容。

World19 的原生清栈字节数为 `8,16,4,0,0,0,0,4,4,8,4,16,24,0,4,20,4,12,32`，与对应 wrapper 及 `kArgumentWords*4` 一致。多条 normal return 路径保留在原始导出中；源码自洽与原生指令证据分别记录。
