# T0-02 模式隔离与 OriginalOnline 旁路证明

状态：`PASS`。Offline lifecycle C1、OriginalOnline 双端 C0、C2 两个单端 analyzer，以及按 packet 语义修订后的双端比较均通过；T0-03 已解锁。正式 gate decision 为 `20260819T080844Z-t0-02-c2-original-online-a-a/t0-02-gate-decision.json`，SHA-256 `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185`。

## ModeLatch 与信号

每个 Lua/run generation 拥有一个独立 latch：

```text
Unresolved -> Offline -----------------------> Retired
           -> OriginalOnline ---------------> Retired
           -> CustomAuthorityServer --------> Retired  (T0-02 实机禁止)
           -> CustomAuthorityClient --------> Retired  (T0-02 实机禁止)
```

- `OriginalOnline` 只在以下三类信号至少两类成立时冻结：frame wrapper 的 `self` 与 manager/sync-object 身份连续匹配两次；2–4 人匿名 roster 连续稳定两次；SteamNetworking006 出现 channel `0` 调用。
- `Offline` 只在精确 `lua_pcallk` simulation callsite 建立候选 generation、累计 60 次主线程回调且 manager/roster/channel `0` 在线信号均为零时冻结。PostGameStart 六个静态调用点共享一个只读 wrapper；run teardown 两个静态调用点共享另一个 wrapper。只有 teardown 调用前后 `g_Game` 身份相同、`[g_Game+0x2658A]` 从 active 变为 `0` 时，才提交 `returned_to_menu` 并退休当前 generation；仅有 manager/Lua root 复用或变化都不足以建立该结论。
- 显式 scenario 只是期望值，不是证据。期望 Offline 却出现两个在线信号，或期望 OriginalOnline 却满足完整离线窗口，都会退休而不会自动改选。
- 非零原版 channel、simulation 线程变化、manager 身份变化、online frame 回退、bound `lua_close` 和菜单/manager 退役都会使当前 generation 进入 `Retired`。新 generation 必须使用新 token；历史 token 保留 tombstone，不能复活。

机器可读定义以 `ledger/isaac-cs-t0-mode-ledger.json` 为准。它用 SHA-256 固定引用 T0-01 ledger；既绑定已有 `STATIC_CONFIRMED` target，也独立登记 13 个 lifecycle `UNKNOWN` target。零匹配、多匹配、错误 call target 或与 T0-01 target 重叠都会 fail closed。

## Hook owner 与旁路不变量

- 固定事务拥有 `RVA 0x50B8E5`、`RVA 0x5110AB`、六个 PostGameStart callsite、两个 run-teardown callsite、四个 Lua IAT 和 `SteamInternal_ContextInit` IAT；每组 lifecycle callsite 共享一个 owner/wrapper。内部 PreGameExit call 只验证、不修改。
- 命中精确 Steam context 后，第二个事务同时验证并替换 SteamNetworking006 的 Send/Available/Read 三个 vtable slot；原地址必须全部属于 `steamclient.dll`。
- 所有 wrapper 恰好调用保存的原函数一次。frame/dispatch/lifecycle/Lua/Steam 参数、调用线程、调用次数和返回值均不改写。lifecycle artifact 只记录整数参数、单调候选序号和 active 前后值，不记录 `g_Game` 指针。
- Send 只在栈上计算 payload 的调用前/后 SHA-256；Read 只在原函数成功返回后计算实际输出的 SHA-256。artifact 不保存 payload。
- SteamID 使用未落盘的双端 pairing key 生成匿名 token；artifact 不保存 pairing key、原始身份、用户名或游戏绝对路径。
- event ring 固定 4096 槽，无 hook 内文件 I/O、堆分配或互斥锁；controller 持续确认消费序号。缺号或真实覆盖直接使证据失败。
- shutdown 先阻止新观测分支，再逆序恢复 Steam/fixed target、等待 active wrapper 为零，发布 `hooks_restored` 后才卸载 DLL。注入前与卸载后的快照同时覆盖 T0-01 baseline 和全部 T0-02 lifecycle target；任何 owner/当前值不符都保持 DLL 加载并进入 `Unsafe`，不得强卸载。

## 实机矩阵

开始每个 run 前必须关闭游戏、IssacOnlineSync、x32dbg/x64dbg、Cheat Engine 及其他网络 hook。控制器启动后再由用户启动游戏；不要让两个端点的控制器看到同一系统上的多个 `isaac-ng.exe`。

1. `C0 Offline control`：`control-no-native / solo`。启用同一个只提供标准 `MC_POST_UPDATE` 的 callback witness，执行两轮离线 run，中间返回菜单；该 Mod 状态必须与 C1 完全相同。
2. `C1 Offline disarmed`：`offline-disarmed / solo`。执行相同两轮离线 run，每轮保证至少 60 个 simulation callback；第一局开始必须出现一个 PostGameStart，返回菜单必须出现一个同候选序号的 run teardown 且 active `nonzero -> 0`，第二局必须出现更大的 PostGameStart 序号并冻结新 generation；按 Enter 后等待安全卸载，再退出游戏。该项已由修复版 lifecycle C1 通过。
3. `C0 OriginalOnline control`：两个端点分别使用 `control-no-native / a|b`，在原版联机中活动至少 20 秒；这是高层 frame/manager 对照，不声称捕获函数级 channel 事件。
4. `C2 OriginalOnline instrumented`：两个端点设置相同 `ISAAC_CS_T0_PAIR_KEY`，分别运行 `original-online / a|b`；进入原版联机并覆盖成功推进和 ready stall，按 Enter 安全卸载。
5. 每个端点独立运行 `mode-analyze`；用 `mode-compare` 比较 no-Native/Native，并在 C2 传入另一个 instrumented endpoint 做双端 payload hash 配对。双端不要求同步启停；首末成功配对 packet 的本地 event sequence 分别界定发送端和接收端的采集内部窗口。

只有以下条件全部满足才可将 T0-02 改为 `PASS`：事件序号连续、C/S 事件为零、所有原版网络调用为 channel `0`、Send 前后 hash 相等、wrapper 原函数调用数均为一、stall 无 dispatch/frame 增量、成功 frame 恰好一次 dispatch 且 `F -> F+1`、同 generation 无热切换、退休 generation 不复活、双向至少两个配对锚点、采集内部窗口未知 Read 为零、可靠 Send 缺失为零、send mode 合法，以及 safe unload 与目标前后字节均通过。mode `0/1` 的未送达 Send 只作为不可靠丢包诊断；采集窗口外 Read/Send 作为 edge 记录。

若 Steam context 在注入前已经越过且无法安装只读 vtable wrapper、不能建立两个互证信号、事件溢出、原函数语义变化或恢复失败，则保留 artifact，记录 `FAIL`/`BLOCKED_BY_EVIDENCE` 并停止，不进入 T0-03。

## 已取得的实机反证

- 无 callback Mod 时，Native 只观察到 Lua root 创建，可信 simulation pcall 为零；因此这种运行不能证明 Offline latch，已按 `FAIL` 保留。
- 启用 callback witness 后，两轮离线 run 共观察到 1259 次连续、单线程、原函数恰好调用一次的 simulation pcall，Offline generation 1 成功冻结，C/S 事件为零且安全卸载/目标字节恢复通过。
- 同一次运行中返回菜单并没有调用 `lua_close`，Lua root 被复用；旧 analyzer 将“第二局必须出现 lua_close”作为硬条件已被实机反证。更新后的门禁要求由带原因标志的 `lua_close`、menu-return 或 manager-rebuild 退休旧 generation，并仍要求第二局冻结为更大的 generation。`lua_close` wrapper 本身继续由 Native harness 和出现该真实事件的 artifact 严格检查。
- 修复版 manager-generation C1 `20260818T163534Z-t0-02-c1-offline-manager-generation-solo` 捕获 1547 次连续 simulation pcall。Offline generation 1 冻结、C/S 事件为零、wrapper 单次原调用、线程契约、安全卸载、磁盘映像与全部目标字节恢复均通过；但两轮 run 之间没有 `lua_close`、menu-return 或 manager identity replacement，最终 `retirements=0`、`frozenGenerations=1`。这将“当前 manager slot 会随离线新局重建”登记为 `REJECTED_HYPOTHESIS`，整体 T0-02 记为 `BLOCKED_BY_EVIDENCE`。
- 首次 lifecycle C1 重跑 `20260819T054918Z-t0-02-c1-offline-lifecycle-rerun-solo` 在注入前快照阶段因 signature 的 `expectedBytes` 长于 masked feature 而 fail closed；`nativeProbeInstalled=false`，没有安装 hook。随后三个 lifecycle signature 已扩展为完整等长 feature，补齐绝对地址重定位掩码，并新增 schema 回归：任一 signature 长度不一致会在离线账本读取阶段直接拒绝。用于成功 C1 的修复版 mode ledger SHA-256 为 `5F33D71EB9D8D56A5B7B358F338C1908A955BED3AA67961F709D7C6DB69A9B4E`；写入该次运行时证据来源后的当前 ledger SHA-256 为 `C6034ED0E2A510852B913AE29F276347809291CCFC993D568CAAF3340F174275`。
- 修复版 lifecycle C1 `20260819T060151Z-t0-02-c1-offline-lifecycle-fixed-solo` 独立重放为 `PASS / RUNTIME_OBSERVED`：745 个 Native 事件连续，735 个 simulation callback；start candidate `1 -> 2`，中间恰好一个 teardown；两次 start 参数均为 `0` 且 active `1 -> 1`，teardown 参数为 `1` 且 active `1 -> 0`；lifecycle 与 simulation 均在线程 `12292`，两个 Offline generation 分别冻结，generation 1 退休后未复活。C/S 事件为零、所有 wrapper 原函数恰好调用一次、safe unload、磁盘 hash 和 T0-01/T0-02 全部目标前后字节均通过。与 callback-equivalent C0 `20260818T162116Z-t0-02-c0-offline-control-callback-mod-solo` 的 `mode-compare` 也为 `PASS`。
- OriginalOnline C0 A `20260819T075721Z-t0-02-c0-original-online-a-final-a` 与 B `20260819T075747Z-t0-02-c0-original-online-b-final-b` 均独立重放为 `PASS / RUNTIME_OBSERVED`。观察窗口重叠；两端完整文件、`.text`、tool、baseline ledger、mode ledger 和 patch state 完全一致；A/B 分别取得 143/255 个 control sample 和 135/242 个有效 frame sample，frame 从 `0 -> 1102`、`0 -> 2757` 且各自单调；两端均无 Native/C/S 事件，磁盘和前后目标快照通过。跨端报告为 `c0-dual-endpoint-comparison.json`。
- OriginalOnline C2 A `20260819T080844Z-t0-02-c2-original-online-a-a` 与 B `20260819T081006Z-t0-02-c2-original-online-b-b` 单端均为 `PASS / RUNTIME_OBSERVED`：事件连续、C/S 事件为零、channel 全为 `0`、payload 前后不变、stall/advance/dispatch 语义、线程契约、OriginalOnline 冻结、safe unload 和目标恢复全部通过。旧 `mode-comparison.json` 保留全窗口精确投递规则的失败，`packet-pairing-diagnostic.json` 保留修订前阻塞结论。新版 `mode-comparison-packet-v2.json` 使用本地事件锚点，不依赖跨机时钟：A→B/B→A 分别匹配 `6264/6281` 个 packet，内部未知 Read 均为 `0`，可靠缺失和非法 mode 均为 `0`；mode 1 内部未送达 Send `1347/1274` 作为诊断。新版比较为 `PASS`，SHA-256 `FF56A0C2ED73281F5B6688A3ABB6163FAD99C5E4260D175EA915C4F1A6E9049F`。“每个成功不可靠 Send 都必须被 Read”登记为 `REJECTED_HYPOTHESIS`。

## 新 lifecycle 候选与待验证项

静态候选包括 PostGameStart dispatcher `RVA 0x464EE0` 的六个完整直接调用点、run teardown `RVA 0x2FA0C0` 的两个完整直接调用点、其内部唯一 PreGameExit call、`g_Game` slot `RVA 0x871678` 与 active word 偏移 `0x2658A`。精确 RVA、masked feature、原始字节、call target、来源与当前证据标签见机器账本及 [lifecycle-candidate-evidence.md](lifecycle-candidate-evidence.md)。

Offline C1 已验证：每局恰好一个 start、返回菜单恰好一个 teardown、二者在 simulation callback 主线程、teardown 前后为同一 game 且 active 从非零变为零、第二局 candidate/generation 单调增加、旧 token 不再产生事件，以及 10 个 relative-call target 与 5 个 IAT target 全部安全恢复。Continue、pause/换层/死亡等补充语义仍保持 `UNKNOWN`，但不再阻塞 OriginalOnline C0/C2 旁路矩阵。
