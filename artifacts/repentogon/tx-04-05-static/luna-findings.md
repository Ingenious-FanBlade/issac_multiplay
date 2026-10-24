# tx-04-05 静态逆向结论（Luna）

范围：仅对 v1.9.7.12 脱壳 IDB 与官方 REPENTOGON tag1.1.3 源码进行只读静态分析。IDB imagebase `0x400000`；下文地址均为 RVA。没有运行构建、游戏或测试，没有修改 IDB。详细原始反编译和汇编见本目录现有 `t1_*` 至 `t8_*`、`primary-ida-evidence.txt`、`source-evidence.txt`，本轮可复核导出脚本为 `luna_export_ida.py`；候选命名错误见 `candidacy-corrections.md`。

| 目标 | 静态确认范围 | 留待运行时观察/未证事项 |
| --- | --- | --- |
| T1 | 三个奇数帧虚调用位置、普通调用点与特殊状态门已确认。 | Player 原生 body 到达只能由内层 +10000 hook 确认；动态 patch overlap 不能由源码检索穷尽证明。 |
| T2 | controller id 与设备对象 `+0x08` 比较、`-1` 遍历路径已确认。 | 目标运行中的设备连接和映射实际值。 |
| T3 | query 无消费写入、Poll 后派生状态顺序已确认。 | 目标运行帧的最终物理采样值。 |
| T4 | Manager/InputManager/设备层路径及 REPENTOGON 短路限制已确认。 | 具体同帧 mask/search 分支是否命中；内层未到达时不可补推设备值。 |
| T5 | Console、PauseScreen、失焦选项到 pause state 的静态门已确认。 | 真实窗口焦点候选值与选项运行值；窗口事件到 setter 的入口在当前证据里未完全闭合。 |
| T6 | `MC_POST_NEW_ROOM` 调用点相对 Room 实体更新循环的位置已确认。 | Player 最终重定位字段写入与 callback 的先后仍未知，静态调用链未闭合。 |
| T7 | 字段访问宽度及 `_moveDirection` 的 int 类型已确认。 | `+0x1624/+0x1628/+0x162C` 的业务语义不能仅靠现有 store 指令命名。 |
| T8 | Query helper、InputManager、设备槽及静态 action immediate 映射已确认。 | movement/shooting 的动态 action 分支运行时实际取值和命中情况。 |

## T1：奇数帧 helper 的 Player 更新路径

**直接事实。** RVA `0x2F9340` 的 helper 对 Room 的实体列表执行三处可见的虚调用 `vtable+0x0C`：`0x2F93F9`、`0x2F949C`、`0x2F9559`。前两处位于特殊状态分支，普通实体循环前先调用 RVA `0x2F92A0`；若它返回真，跳到 helper 尾部。普通循环取 `Game+0x18300` 的 Room，枚举 `Room+0x125C/+0x1264` 实体范围，并按 `[entity+0x28]==1` 分支到 `0x2F9559`。这里的 `Game+0x18300` 是 Room 指针，`+0x125C/+0x1264` 是 Room 实体列表，不是 PlayerManager；PlayerManager 位于 `Game+0x1BAA8`。普通调用后 `0x2F955C` 执行 `push 0`，`0x2F955E` 继续 `mov ecx,esi`。

**静态推导。** 若要在不读 ZHL 桩帧的前提下给奇数帧普通 Player 更新建立上下文，RVA `0x2F9559` 的原始五字节 `FF 52 0C 6A 00` 可作为单调用点 shim 候选：在 shim 中按原寄存器语义执行该虚调用恰好一次，恢复调用点所需状态后压入原来的 0，并跳回 `0x2F955E`。不要用 `_ReturnAddress()` 区分这三个调用点。官方 `CustomCallbacks.cpp:521–534` 的 `HOOK_PRE_UPDATE_CALLBACK(Entity_Player, ..., 1160)` 是活动宏：Lua 返回真会调用 `Entity::Original_Update()`，否则 `super()`；因此普通虚调用仍会进入已有 REPENTOGON Player Update Hook 链一次。该点位候选只针对当前 IDB 与 tag1.1.3 的静态事实，来源搜索没有证明任何可能的运行时动态补丁绝不重叠。

**限制。** `MC_PRE_PLAYER_UPDATE` 跳过 subclass update 不能从外层 Hook 返回、consumer 未执行或 consumer 计数缺失倒推。若需要报告 `player_skipped`，还需 +10000 内层 Player Update Hook 的原生 body 到达证据；否则只记 `outer_entered`、`native_body_observed/UNKNOWN`。Hex-Rays 返回 `None` 的函数仅有汇编依据。

## T2：controllerIndex 到设备对象

**直接事实。** RVA `0x5EEC40` 的映射函数遍历 InputManager 设备 vector 的 begin/end（InputManager 对象 `+0x04/+0x08`），逐个比较 `device+0x08` 与传入 controller id，匹配时返回设备对象，不匹配则返回空。对象 `+0x0C` 是 vector 的第三个指针槽，但这条 lookup/update 路径不读取它；不要把 `controllerIndex` 当 vector 下标。官方 `libzhl/functions/Input.zhl` 声明 `GetDeviceType(controlleridx)`、`GetDeviceNameByIdx(cidx)` 与 InputManager action 查询，但那两份旧候选导出实际落在 RVA `0x5EED00`，不可用于这两项结论；见更正记录。

**静态推导。** Manager Pressed/Triggered/Scalar 包装层对显式 id 调用该查找；传入 `-1` 时改为遍历设备 vector 并逐个调用设备槽。显式 id 找不到设备的分支返回零值，没有从 vector 下标静默回退的证据。

## T3：原生查询是否消费状态

**直接事实。** Device 公共槽查询 Pressed/Triggered/Scalar（RVA `0x5ECD80/0x5ECDE0/0x5ECE50`）只读输入映射、当前数组 `+0xC8`、前一数组 `+0xCC`、阈值和设备接口；反编译与汇编里均未见写设备 current/previous 或清除边沿。Keyboard raw 槽 5/6（`0x633CA0/0x633CD0`）读取全局键盘状态 `0x7E6B08/0x7E6DC8`。Gamepad raw 槽 5/6（`0x5ECB80/0x5ECC40`）及标记 CD 路径 `0x5ED190` 读取设备状态并只写调用方输出缓冲。Scalar raw 槽 7 的基函数 `0x5ECD00` 也只读。

**静态时序。** `DeviceCommonUpdate` RVA `0x5EC900` 在 Poll 前把 current 复制到 previous 并清 current；Poll 成功返回后，在 `0x5EC9A3..0x5ECA50` 继续派生方向 mask 等公共状态，最后才返回成功。因此仅在 Poll wrapper 刚返回时采样可能早于派生状态完成。建议在该设备的 DeviceCommonUpdate 正常成功返回后采样；只读采样，不写 current/previous。T3 已确认查询幂等的静态依据，但设备在真实运行帧内的最终状态仍由唯一 E2E 观察。

## T4：REPENTOGON 查询短路与真值观察

**直接事实。** 游戏 consumer 使用 Manager 层 helper；helper 在特殊状态可调用 REPENTOGON 的 Lua 输入回调，正常路径再进入 InputManager 的 Pressed/Triggered/Scalar。官方 `LuaMenuManager.cpp:191–204` 对 Pressed/Triggered 使用 `mask && super()` 短路；`ModsMenuStuff.cpp:130` 在 `issearching` 时直接返回 false。`ASMCallbacks.cpp:737–764` 的 `ASMPatchInputAction` 对 Manager::GetActionValue、IsActionPressed、IsActionTriggered 等匹配点写入六字节 NOP，作用于 Manager 包装条件分支，不是 InputManager 设备槽。ImGui 浮层路径在 `ImGui.cpp:306–345` 把 Console state 设为 2/0 并释放键。

**结论与限制。** 内层 InputManager Hook 不保证执行，尤其存在短路/屏蔽时；不能通过补调原生查询来伪造设备真值，也不能把未到达内层记为设备假。首版可记录 outer-only/masked 及已观察的 Console/PauseScreen 状态；“设备层真、Manager 最终假”的同次差值在此方案下不保证可见。

## T5：Console、PauseScreen 与失焦

**直接事实。** `Game::Update` RVA `0x2F6D30` 有 Console 判断；真值会提前返回。另在 `0x2F76C9/0x2F76D2` 检查 Manager 焦点候选字节 `+0x4AFAA` 与选项 `PauseOnFocusLost` 字节 `Manager+0x2A1D7`（官方 `OptionsConfig.zhl` 的 Manager options 基址 `+0x2A178`、选项位 `+0x5F`）。分支满足失焦暂停条件且 Manager 内对应 net container 为空时，RVA `0x2F76F8` 调 PauseScreen transition `0x58ED80(Game+0x2363C)`；该函数在 RVA `0x58EE29` 写入 `state=1`。随后 `Game::Update` 在 RVA `0x2F775B` 调 `0x58ED50` 检查 PauseScreen；predicate 真跳到尾部，不再调用普通 Room::Update（普通调用点 `0x2F78DF`）。Odd helper 在 RVA `0x2F94FC..0x2F9518` 也检查焦点候选和该选项，未满足时跳过普通实体更新区。

**静态推导。** 运行时观察可用已存在的 Console 判断、PauseScreen state/predicate 及 Room::Update 到达情况来区分浮层、暂停菜单、失焦导致的跳过；Console=false 不证明游戏已恢复。设置存储、控制流和 PauseScreen 阻断已静态确认；实际焦点值与实际选项值由 E2E 唯一确认。焦点候选字节写入 setter、该 setter 在选项/UI配置路径中的登记细节已见主审 IDA 证据 `primary-ida-evidence.txt`；此处证据未闭合具体 Windows 焦点事件如何抵达该 setter。

## T6：MC_POST_NEW_ROOM 与玩家重定位时序

**直接事实。** `0x84B930` 构造 callback id 19 并分派 `MC_POST_NEW_ROOM`。其调用点 RVA `0x339719` 位于大房间加载函数 RVA `0x3385F0` 尾部；紧邻它之前是 `Game+0x18300` Room 实体列表循环 `0x3396B1..0x339717`，按条件对实体调用 vtable `+0x0C`。正常 `Level::ChangeRoom` 类路径的 caller 是 `sub_739DE0`，其 RVA `0x33A739` 调大加载函数，返回后再调用 `sub_993030`；其他调用路径可见 `0x33FA2B/0x33FA3F`，以及函数 RVA `0x3415E0` 内的 `0x341A45/0x341AA3`。

**静态限制。** 在大加载函数 RVA `0x3385F0..0x339774` 的显式指令中没有找到 Entity position `+0x33C/+0x340` 的直接字段访问；但它调用多个房间/实体子函数，不能据此证明间接调用不会重定位 Player。`sub_739DE0` 在加载函数返回后调用 RVA `0x593030`，该函数枚举 PlayerManager 列表并对每个 Player 调 RVA `0x39FD90`，其可见代码主要处理玩家状态/物品效果，仍未建立最终位置写入点。当前证据只证明 callback 在加载函数内 Player 条件更新循环之后、加载函数返回之前；未能建立“Player 最终位置写入 < callback”或反向的确定顺序。T6 对“回调相对真实重定位”的结论为 **UNKNOWN**，不以名称 `POST_NEW_ROOM` 或函数名替代位置写入证据。原文件 `t6_roomchange_caller_341a45.txt` 的函数头不含 callsite，已在更正记录中剔除。

## T7：Player 字段宽度与语义

**直接事实（主审末次勘误）。** 射击 consumer RVA `0x373990` 内的 `0x3794B0/0x3794B8` 分别以 `movss` 写 `Player+0x1624/+0x1628`，`0x3794C0` 写一个 byte 到 `Player+0x162C`，该写入是 bool flag。此前把两处 float store 仍称为 Player Update 本体也是错误，三处均属于射击 consumer。官方 `EntityPlayer.zhl` 将 `+0x1618` 标为 `_moveDirection`，是整数方向字段。字段访问宽度可静态确认；`+0x1624/+0x1628/+0x162C` 的业务语义不能从这些指令单独命名。设计中的 Q16 是条目编号，不能误读为 `+0x162C` 的定点数类型；现有证据只支持该位置被写入一个 byte flag。另有 `+0x360` 的 Entity velocity 字段定义可见官方 Entity.zhl，但本轮未证明它是本轮输入 trace 所需的权威移动输入字段。

## T8：consumer 到 Manager、InputManager、设备槽与 action

**直接事实。** consumer 直接调用 Manager 层 helper：Pressed wrapper RVA `0x52F610`（VA `0x92F610`）、Triggered wrapper `0x52F6D0`、Scalar wrapper `0x52F7B0`。对应 InputManager 原生实现 RVA `0x5EE140/0x5EE200/0x5EE2E0`；显式 controller id 通过 `0x5EEC40` 查设备，`-1` 遍历设备数组并调用设备 vtable `+0x20/+0x24/+0x28`。Scalar helper `0x771D70` 对 movement actions 0/1/2/3 调 `0x92F7B0`，结果由 XMM0 返回；不要直接 hook 此 helper。RVA `0x371D30` 是 weapon 查询路径，不是 movement input helper。InputManager 设备槽 10 的标量结果则为 x87 浮点返回，ABI 与 Manager wrapper 不同。

**consumer 动作证据。** 按 IDA immediate 与官方 `enums_ex.lua` 的 `ButtonActionBitwise` 名称映射：movement Scalar 0–3（Left/Right/Up/Down）；movement 在 RVA `0x37388A` 还按常量数组 RVA `0x741020=[0,1,2,3]` 查询动态 Triggered action。shooting Pressed 0–7（0–3 为方向，4–7 依次为 ShootLeft/ShootRight/ShootUp/ShootDown），另经 RVA `0x376013` 按常量数组 RVA `0x741120=[4,6,5,7]` 查询动态动作（依次是 ShootLeft/ShootUp/ShootRight/ShootDown），并检查 Triggered 11。bomb Triggered 8。active 默认查询 action 9；特殊条件/slot 分支会改查 action 10，均有 Pressed/Triggered 查询。card/pill Triggered 在 9/10 间选择且有 Pressed 11 的 Drop 辅助检查；drop 使用 Triggered/Pressed 11。具体每个 consumer 执行的子分支仍应在 trace 中记录，不从单个 immediate 外推所有时刻。

**静态推导与限制。** 本轮目标不支持 gameplay 输入 Lua mod；已确认的 native fallback 从 Manager 包装层进入 InputManager 设备槽，因此方案保持设备槽覆写，并把 Manager Lua 替代路径记录为不可见/未观测，不据此强制新增 Manager hook。`ASMPatchInputAction` 会改变 Manager 层条件分支，应作为 source patch 状态记录，但不能据此把 Lua 返回值当成设备真值。键位配置如何映射成各设备 raw state、运行时各 action 是否在同一帧被 consumer 查询，由唯一 E2E 记录。

## 需要据静态事实修订设计的点

1. T1 采用 `0x2F9559` 单调用点 shim 建立 OddNormal 上下文，不用裸 `_ReturnAddress()`；player skip 的 trace 需要 +10000 内层 Hook，否则使用 `native_body_observed/UNKNOWN`。
2. T2 用 controller id 比较设备对象 `+0x08`，不能把 id 当 vector 下标；找不到 id 时不做下标回退。
3. T3 采样点落在 DeviceCommonUpdate Poll 后的全部派生状态完成且正常返回之后；采样代码只读，不写 current/previous。
4. T4 把短路记录成 outer-only/masked，不新增 direct native 补调；设备真/最终假差值不应承诺可观测。
5. T5 保持 Console、PauseScreen、失焦分开记录；静态已能说明失焦选项触发 PauseScreen 的控制流，不应将失焦整体留为未知。
6. T6 当前静态证据不能断言回调前/后已经完成 Player 最终重定位，生命周期绑定失效应结合房间身份/状态变化，不依赖周期性 owner。
7. T7 `+0x162C` 在射击 consumer 中是一个 byte bool flag；Q16 是设计条目编号，不是字段类型。`+0x1618` 是 int 方向；未命名字段的业务含义留未知。
8. T8 本轮保留设备槽覆写；Manager Lua 替代路径不属于 gameplay 输入支持范围，需标注其结果不可见。movement scalar helper 的 ABI 与设备槽 10 不同，不直接 Hook 标量 helper。

本报告是静态结论草稿，主审可按复核结果修改。静态分析不能证明真实运行帧中的 option/focus 字节取值、Lua/REPENTOGON 实际短路分支、设备 poll 的最终物理状态、玩家最终重定位时刻、consumer 动态分支是否命中，以及这些值在目标运行环境中的时间对应关系；这些留给 tx-04-05 唯一 E2E 观察。
