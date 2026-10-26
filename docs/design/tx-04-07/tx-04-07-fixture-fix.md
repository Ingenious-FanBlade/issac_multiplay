# tx-04-07 实质修复：玩家槽、观测时刻与投弹窗口

日期：2026-10-26，Asia/Shanghai。用户要求修复当前问题并继续测试。本轮修复 e2e-03 暴露的 Lua 夹具身份误判，同时处理同份原始日志已经明确的 loop 缓存失真与投弹资源窗口混合。修改期间不执行任何阶段测试；完成纯构建后，经本次实机准备确认只执行一次完整入口 E2E（e2e-04）。旧三次结果和源码快照保留，不重新归约，不新增单元测试。

## 依据

1. e2e-03 的第二轮 DLL 确认 B 为角色 7、ordinal 1、Alive/Active，但 Lua 的 148 条观测把它写成 `present=false, resolved_ordinal=17`，没有 animation_finished。第一轮 S4/S5 中也曾把实体 Index=2 的 B 误标缺席。官方 SDK `LuaInterfaces/Entities/LuaEntityPlayer.cpp` 的 Lua_PlayerGetPlayerIndex 返回 `_playerIndex`，`EntityPlayer.zhl` 的该字段在 +0x1610；Entity.zhl 的 `_index` 是另一字段 +0x20，不能把它当成 PlayerManager ordinal。
2. e2e-03 skip request/arrived 的 game_frame 都为 164，render 从 510 到 570，实际 skipped=30。旧 currentLoop 按 game_frame 缓存，因此两端误写 loop=1062；原 trace 在 loop=1062 的 Odd tick 后直到 1123 才再有 StandardRoom tick，S6.console.before 实际 frame=1123。不能为使旧结果通过而放宽 `结束 loop > 起始 loop`，应从事件发生时读取真实 status。
3. e2e-03 S5 的 A 资源从 2 变为 0，外层区间同时包含 A/B 各一次 Accepted/Resolve。合并区间无法把两次资源消耗分别归因，且 B 视图受同一 Index 缺陷影响。本轮将两个输入保持为各一次，分别采样动作窗口，仍要求每次动作自身资源减一，不增补资源或重发边沿。

## 实现

- playerView/lifeView、健康/ghost/复活读取、死亡身份复核、激光 spawner/parent 的 A/B 归属均使用 `GetPlayerIndex()` 核对 ordinal，仍拒绝 Isaac.GetPlayer 回退到另一个槽的对象。没有简单删除身份保护。只读视图保留原 Entity `index`，新增明确的 `entity_index` / `player_index`；实体、激光自身编号仍用 Entity.Index。
- currentLoop 每次结构化事件通过原有只读 `cs status` 取得 frame，不再按不推进的 Game frame 缓存。读取失败为 nil，不沿用旧值；Manager loop 仍只是事件归属，不当作 Authority tick。
- S5 增加 S5.A.begin/end，A 轻按 E 后等待 250ms；之后原 S5.bomb 脚本给 B 一次 edge，并使用它原有的 begin/end。V8 总窗口仍要求 A/B 各一个 Accepted 和 Resolve、与原始命令一致、bomb consumer 实际查询为真；资源减一分别采用两个动作窗口，并写出资源前后值。S7 故障边沿 at-most-once 标准不变。

没有修改 DLL、原生输入、世界数据或动作权限。S4 仍要求真实位移，skip 仍要求 30 次与严格无 StandardRoom tick，动画等待仍为真实字段和原超时，不硬写成功或放宽阈值。未知/未执行项继续保持未验证。

## 构建与本轮唯一 E2E

统一纯构建 DLL 与 i686 harness，日志保存到 `artifacts/repentogon/tx-04-07-fixture-fix/`。构建成功并收到用户本次停止键鼠准备确认后，原 `harness.exe e2e tx-04-07` 单入口、一次游戏启动覆盖 S0–S21/V1–V16，产物 `tx-04-07-e2e-04`。SkipIntro=1，第二档及动作沿用已有授权，Download Helper 按 D20 临时禁用并恢复，无哈希计算、无存档回滚，失败不自动重跑。

统一纯构建已成功，见 [build-01.log](../../artifacts/repentogon/tx-04-07-fixture-fix/build-01.log)：Win32 DLL 与 i686 Rust harness 均编译链接完成，Rust 无 warning，未运行测试或部署。已静态回读身份检查、真实 loop 读取与 S5 两个资源窗口；与 e2e-03 快照的改动保存为 [changes.patch](../../artifacts/repentogon/tx-04-07-fixture-fix/changes.patch)。用户本次回复“已准备好，构建成功后开始”后已启动一次完整 E2E（e2e-04），实际结果随后追加。

本轮唯一 E2E 已结束，整体 **FAIL**：S0–S7 完成，S7 故障段 loop=2216 失焦，随后 pause_state=1、blocks=1、completed_tick 停在1298，S8.recovered 在无 Authority tick 时等待超时。游戏正常退出、无新 dump，环境已清理，未自动重跑或测试后修改。

直接观察到本轮三处修复生效：B Entity Index=2 / player_index=1 的视图不再缺席，S4 两方向移动与释放检查通过；A/B 分别投弹时库存 2→1→0，V8 PASS；skip 事件真实 loop=1085→1145、30 次跳过且 game_frame 不变，局部判定 PASS。第二轮未到达，不能将这些局部事实外推为阿萨泻勒动画或完整 tx-04-07 通过。失焦来源仍 UNKNOWN，后续需处理实机焦点与阻断等待。详见[第四次结果](tx-04-07-results.md#夹具修复后的第四次-e2e2026-10-26asiashanghai)和[产物](../../artifacts/repentogon/tx-04-07-e2e-04/README.md)。
