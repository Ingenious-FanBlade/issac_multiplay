> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager context v1：条件公式与入口前提

本证据新增子证明 `FRESH_B_MANAGER_CONTEXT_TO_ORDINARY_SPAWN`。静态边界从 `0x005BAC40` 的 manager 入口开始，覆盖 possessor 调用 `0x005BAFFA`、ordinary 调用 `0x005BB415` 及两者之前的输入分派、selector 循环、正常/异常出口；边界止于两个 `0x005B9CD0` 调用点，不进入分配、constructor 或 Init。

正式 sidecar 是 `cs-t0/coop-native-join-manager-context-v1.json`，SHA-256 为 `2D0CB3C4DEE18AED73793AB63B72524D6EF043C2E0AD00611D06B22C78994B06`。它绑定 raw v2（SHA-256 `B613885CEA08A64479A80844A74AD24240A2DBC49C2984A39733E4B20F97A486`）和 reverse draft v4（SHA-256 `6D0657B8EC796441E0238BB0B21150E6B7C6269CA3769615B27050B38CDFD072`），登记 22 个函数、1,863 条指令、116 个调用、530 个写入和 40 个出口。

## 已闭合的 PE 局部公式

目标 PE 将 InputManager 静态对象绑定到 `0x00857B18`，vtable 为 `0x007827F4`，slot `+0x74` 指向 `0x00620FB0`。manager 中 10 个实际查询点均纳入公式：

- `0x005BAEFE`：event 19 Triggered 的 side 查询；
- `0x005BAF78`、`0x005BAFA4`、`0x005BAFD0`：依次为 event 19 Triggered、event 8 Held、event 10 Held；
- `0x005BB064`、`0x005BB145`：另外两个 event 19 Triggered side 查询；
- `0x005BB2B7`、`0x005BB305`：selector 的 event 20/21 Triggered；
- `0x005BB39A`、`0x005BB3C9`：ordinary confirm/cancel 的 event 14/15 Triggered。

possessor 的实际公式是：在 BL 经主玩家向量、secondary 向量和四个 manager association record 检查后仍为零时，三次独立查询 `Triggered(controller,19) && Held(controller,8) && Held(controller,10)` 全为真，才到达 `0x005BAFFA(-1)`。`GameRoot+0x26630` 只控制外层 side/UI 路径，不支配这组三查询，因此不能拿它排除 possessor alternate。

ordinary 路径从 selector 的 event 20/21 循环到 event 14 confirm；event 15 受 auto flag 分支控制，最终 player type 经 `0x005BB3D8..0x005BB40E` 的定义链传入 `0x005BB415`。roster 根只比较 `+0x4B3D8` begin 与 `+0x4B3DC` end，没有读取 capacity；三指针 canonical-zero 仍是更强的准入前提。

每个 callback 返回都作为单独的时间编号谓词。`controller==-1` 的 all-device 路径、controller-specific first-match、8-byte `{device, sharedControl}` pair、短路和 `device+0x0C` controller-key 回写均在 callee/target universe 中登记。相同地址、两次相同返回或写后恢复不能合并成稳定读取。

## 仍未建立的入口前提

`managerGuardFormulaStatus=STATIC_CONFIRMED` 只说明冻结 PE 中的条件、事件号、调用顺序和定义使用链已闭合。`managerContextStatus=UNKNOWN`，因为本轮没有生产 reader 或 generation 实现。九个精确前沿保持未决：manager this 身份、roster canonical-zero、pair lifetime、possessor 排除条件、InputManager/device vtable 运行时身份、引用控制 callback、selector/type 前态、device predicate state，以及异常/提前退出 ABI。

owner、vector、device 或 shared-control 被替换，重复设备、双读漂移和 A→B→A 地址复用都必须使未来 generation 失效。`noPhysicalInput` 不能推出 Held/Triggered 返回；ordinary 场景标签、人工声明和未来 fresh B 读回也不能建立入口前提。

本轮未实现 reader、runtime、IPC、arm/claim，未启动、附加或操作游戏进程。plan v10、join progress v4 和三个当前索引不变；publication、fresh B、完整同步传播、自然更新窗口和所有 active/live 权限继续关闭。
