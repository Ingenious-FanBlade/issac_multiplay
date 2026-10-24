# T0-03 gameplay/input/entity/render 边界研究

状态：`PASS`（用户修订矩阵；两项显式豁免）。T0-02 packet-v2 门禁为 `PASS`，正式 decision SHA-256 为 `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185`。当前 boundary ledger 下 `normal`、`input`、`pause`、`room-transition`、`menu-return`、`lua-rebuild` 六组 C0/C1 已全部通过；此前事件发布 ABI 和 spawn ECX owner ABI 两次崩溃仍作为失败证据保留，并由后续实机安全恢复证据覆盖。`focus-loss` 因失焦即进入已验证 pause，`low-fps` 因用户明确要求停止本轮验证，两者从修订矩阵排除且不冒充独立 runtime `PASS`。正式 gate decision SHA-256 为 `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB`。本文件保留 T0-03 scoped 结论；后续 T0-03R/T0-04 已完成，当前可复用结论统一见 [T0-04 逆向交接](../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md)。

## 静态候选

机器可读定义以 `ledger/isaac-cs-t0-boundary-ledger.json` 为准，当前 SHA-256 为 `7E6C14CAF0D92ADCB7326F5E72B963431DE652F2103315C28CFE91D6953CE4BB`。它固定引用 T0-01 ledger、T0-02 mode ledger 和 gate decision hash；当前固定 variant 仍为 `online-mod-patched-2026-04-21`。

| 角色 | target RVA | 静态关系 | 当前语义 |
|---|---:|---|---|
| input sample candidate | `0x554F84` | 位于 local input-state copy 和 gameplay call 之前 | `UNKNOWN` |
| gameplay suppression branch | `0x5551CF` | `[main-root+0x4ABBC].bit0` 为真时跳过 gameplay call | `STATIC_CONFIRMED` 分支，pause/loading 语义待实测 |
| gameplay update candidate | `0x555231 -> 0x2FADC0` | 主循环唯一直接调用 | `UNKNOWN` |
| entity update owner | `0x2FB51C -> 0x4318A0` | 嵌套于 gameplay candidate | `UNKNOWN` |
| common spawn inner call | `0x28B3D -> 0x2FE410` | 公共 wrapper 内唯一 core call；wrapper 有 1807 个调用方，不合并调用方语义 | `UNKNOWN` |
| delayed remove finalize | `0x2FBB66 -> 0x147F0` | 位于延迟删除遍历内 | `UNKNOWN` |
| `MC_POST_UPDATE` | `0x2FBBA5 -> 0x4607A0` | dispatcher id `1`，完整 `.text` 中唯一直接调用 | `STATIC_CONFIRMED`，capture 语义待测 |
| render candidate | `0x5559C1 -> 0x2FBC10` | 主循环唯一直接调用 | `UNKNOWN` |
| `MC_POST_RENDER` | `0x2FC0A6 -> 0x463680` | dispatcher id `2`，完整 `.text` 中唯一直接调用 | `STATIC_CONFIRMED` |
| `MC_POST_NEW_ROOM` | `0x33F5B9 -> 0x465180` | dispatcher id `0x13`，完整 `.text` 中唯一直接调用 | `STATIC_CONFIRMED`，room/loading 顺序待实测 |
| OriginalOnline diagnostic | `0x5110AB -> 0x50D640` | 只比较相对顺序，不视为通用 gameplay 边界 | `STATIC_CONFIRMED` |
| simulation Lua call | `0x4729CB` 的 `lua_pcallk` | 只在 return RVA `0x4729D1` 记录 | `STATIC_CONFIRMED` callsite |
| Lua root close | `0x46E5C0` 的 `lua_close` | return RVA `0x46E5C6`；旧 generation 立即退役 | `STATIC_CONFIRMED` callsite |
| Lua root creator | `0x46057B` / `0x4605A3` | `lua_newstate` / `luaL_newstate`；只记录成功 creator generation，不记录指针 | `STATIC_CONFIRMED` callsite |

所有定位均由离线 PE/反汇编和完整 `.text` rel32 扫描得到；过程中没有断点、pause、单步、调试器 patch、游戏写入或游戏启动。`MC_POST_UPDATE` ABI 已核对为无参数 `void (__cdecl*)()`，函数以普通 `ret` 返回。

## Passive Native 边界

- boundary IPC 使用独立 magic `0x33425343`、version `3` 和初始化导出 `IsaacCsT0BoundaryInitialize`；不复用 T0-02 mapping 状态。version 3 在 Lua lifecycle 事件之外增加 `NewRoomEnter/Exit`、主循环 state 与 gameplay-dispatch suppression 标志；事件结构仍为固定 104 字节且不保存对象指针。
- `DllMain` 只保存模块句柄。helper 只能对用户已启动的精确 x86 目标显式初始化；不提供启动游戏的路径。
- 一个事务验证并改写 12 个 rel32 call；同一事务验证并改写 `lua_newstate`、`luaL_newstate`、`lua_close`、`lua_pcallk` 四个 IAT slot。任一原始字节、rel32 target、IAT callsite 或 owner 不符都会整体失败并恢复。
- wrapper 只记录 QPC、线程、调用深度、return RVA、generation、安全计数和摘要；不记录对象指针、payload、SteamID、用户名或绝对游戏路径。每个原函数恰好调用一次，参数、调用线程和返回值保持不变。
- 公共 spawn wrapper 的内部 call 只用于标记 spawn activity；1807 个上游 caller 不被错误合并为一个 spawn-drain owner。
- spawn core 原目标是 ECX owner + 8 个栈参数并 `ret 0x20` 的 `__thiscall`；replacement 使用 `__fastcall` bridge 显式捕获并原样转发 ECX/EDX。Native harness 验证 owner、8 参数、返回值和恰好一次原调用。
- lifecycle teardown 或 `lua_close` 立即退休旧 generation；旧 token 不再发布游戏事件。普通场景随后立刻恢复。仅 `lua-rebuild` 场景会在已失能状态下等待一次已验证 creator：成功 creator 发布 `old generation + 1`，然后以 `LuaRebuild` reason 完成 retirement 并恢复。缺少 close/creator、顺序错误、错误 callsite、错误线程、空 creator 结果、稳定 OriginalOnline roster、事件覆盖或目标冲突均 fail closed。
- shutdown 逆序恢复所有 call/IAT、等待 active wrapper 归零、发布 `hooks_restored` 后卸载；控制器随后重新抓取 T0-01、T0-02、T0-03 全部目标。
- 所有事件发布均以 const 引用传入，ring slot 的 published sequence 单独维护，只复制其后的 payload。禁止在 x86 Release/LTCG 路径按值传递 `NativeEvent`；Native harness 覆盖 payload 完整且不会提前复制发布序号。

## Analyzer 与实机矩阵

Analyzer revision `t0-03-boundary-passive-v4-room-loading` 要求 artifact/native 两套事件序号均无缺口、enter/exit 严格嵌套、exit 的 `originalCalls=1`、return RVA 精确、input sample 在 apply 前、apply 在 gameplay enter 前、每个成功 gameplay scope 恰好一个 post-update、post-update 进入时 entity/spawn/remove scope 均为零、simulation Lua callback 嵌套于 post-update、所有游戏事件同线程、退役 token 不复活，以及前后磁盘 hash/目标字节完全一致。每个 v4 passive input sample 都必须带主循环 state/suppression 读数；suppression 为真后到下一次 sample 之间不得出现 input apply、gameplay enter 或 post-update。`room-transition` 窗口必须恰有一对 `NewRoomEnter/Exit`，其前存在 suppression sample，且 suppression 到 new-room 之间为零 post-update。`pause` 的测量窗口必须为零 tick；`menu-return` 必须在窗口内 retirement。`lua-rebuild` 仍要求精确 close→created→retired 链。render-only 窗口单独统计，不据此声称 Replica 分离可行。

每个场景先执行 `control-no-native`，再执行同条件 `passive`。控制器启动时游戏必须尚未运行；由用户启动游戏。`lua-rebuild` 固定在主菜单、至少一个普通 Mod 已启用的条件下执行：第一次 Enter 后进入 Mods 菜单，重新选择该 Mod 触发 reload，保持在主菜单且不进入 Run、不退出进程。passive 中 close→creator→retirement 会自动结束窗口并开始恢复；C0 由第二次 Enter 正常结束。Native 原始事件在窗口外也完整保留，另有连续 `nativeSequence`，窗口标记不会掩盖事件缺口。

当前 Release SHA-256：`IsaacCsT0.exe=2F198B49529C7651E0E0AE0FDB91438CE433519FFB4C1EAE1419B298FFBF005E`，`IsaacCsT0.Native.dll=1B8BCDF9BA50C8E111D1442F2F56A2450C68ECE6017F9BD0685FF3CE64821A6A`，`IsaacCsT0.Helper.exe=9DEA37E4EAB2F6D3E71FF54C57A02B8ABAFC09DD532700661E23EA2E3A21D318`。boundary ledger 为 `7E6C14CAF0D92ADCB7326F5E72B963431DE652F2103315C28CFE91D6953CE4BB`；creator/close 目标来自其固定引用的 T0-01 baseline ledger。

旧账本 `6D9E...` 下通过的同账本配对包括：`normal`、`input`、`pause`、`low-fps`、`room-transition`、`menu-return`、`lua-rebuild`。`lua-rebuild` C1 精确捕获了 `LuaClose(g1) -> LuaCreated(g2) -> Retired(g1, LuaRebuild) -> HooksRestored(g2)`；`menu-return` 捕获了 generic retirement 且旧 token 无后续游戏事件。`low-fps` 在单逻辑 CPU、Idle 优先级下的 QPC 派生 candidate 速率约为 `10.08 tick/s`；调度属性尚未写入 artifact，因此不宣称正式 FPS 阈值。旧 room C1 经 v4 非覆盖重放得到 `FAIL / room-loading-boundary`，报告 SHA-256 `70697B3C9A71EF382712EB739FEB87F572E3DFE1BF4835BA03E5860A6215C157`；这不是旧运行退化，而是明确证明旧 schema 没有新增机器信号。Replica separation 继续为 `UNKNOWN`。

旧 v2 analyzer 报告全部保留原 hash；v3 Lua 兼容重放写入按 revision 命名的新文件。报告 writer 使用 `create_new`：同 revision 已存在时拒绝覆盖，不同 revision 只新增版本化文件。早期聚合阻断审计仍保留，SHA-256 `3DAC18F0F8F7EC7DE8DFF142A8418F6B078607B2BD72CFA5B9646BF111323B18`；它已被当前 gate decision 取代，但不会覆盖或改写。

当前账本六组必测配对均通过相同 build/baseline/scenario/probe-role/single-run 检查。`pause` 窗口 candidate tick 与 post-update 均为零；`room-transition` 精确观察到 suppression sample 在唯一 NewRoom pair 之前，二者之间 post-update 为零；`menu-return` 和 `lua-rebuild` 均在 retirement 后停止旧 token 事件并自动恢复全部 38 个目标。正式机器可读决定位于 `artifacts/cs-t0/20260819T185814Z-t0-03-gate-decision/t0-03-gate-decision.json`，SHA-256 为 `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB`。

豁免限制保持显式：`focus-loss` 和 `low-fps` 没有当前账本 runtime PASS；low-fps C0 被用户中断后仍为 `RUNNING`、无 WindowEnd/after/analyzer/C1/compare。T0-03 结论不建立正式 FPS 阈值，Replica separation 继续为 `UNKNOWN`。

历史失败 artifact 继续保留且不被覆盖：`20260819T102143Z...` / `20260819T103625Z...` 记录了 x86 Release/LTCG 事件按值发布 ABI 故障；`20260819T105034Z...` 记录了 spawn core 丢失 ECX owner 的 ABI 故障。这些 run 没有 after snapshot 或 `hooks_restored`，仍是 `FAIL`。后续同账本 `normal` C1 `20260819T110145Z-t0-03-boundaries-passive-normal-c1-normal-spawn-abi-r1` 已以 87,663 条连续事件、85/85 spawn 配对、36 个目标恢复和无 WER 的证据证明修复版路径安全；历史失败不会被重新解释为成功。
