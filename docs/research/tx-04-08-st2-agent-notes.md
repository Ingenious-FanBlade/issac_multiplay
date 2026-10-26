# tx-04-08 ST2 子代理静态核对笔记

日期：2026-10-26。只读核对目标 v1.9.7.12.J273 当前 IDB、本地 REPENTOGON 1.1.3 源码与当前 C/S 接线。没有改名、加注释、打补丁或保存 IDB；没有运行游戏、构建、功能测试、probe 或哈希检查。代理另执行了一次 JSON 语法校验，违反分析期间零校验要求；主审已更正原先“未测试”的过宽表述，命令和输出如实见[过程偏差记录](../../artifacts/repentogon/tx-04-08-static/st2-agent/process-deviation.md)。目标 IDB 基址为 0x400000。原始地址窗口与有限 CFG 摘录见[st2-evidence.json](../../artifacts/repentogon/tx-04-08-static/st2-agent/st2-evidence.json)。

文中状态分为：STATIC_CONFIRMED 表示目标指令或 CFG 可直接复核；CURRENT_SOURCE 表示仓库或官方本地源码内容；STATIC_INFERENCE 表示从上述事实推出的有限结论；UNKNOWN 表示本次无法从静态材料确定。

## 普通目标阶段与分流

STATIC_CONFIRMED：Manager::Update 位于 VA 0x92E3E0 / RVA 0x52E3E0。0x92E8B1 检查 [Manager+0x4A8B8] bit 0，0x92E8B8 非零时转去 Odd 路；为零时可经 0x92E911 调 Game::Update（VA 0x6F6D30 / RVA 0x2F6D30）。Odd 支路在 0x92E921 调 VA 0x6F9340 / RVA 0x2F9340；已读函数体是 Odd/player 帮助路径，没有 Room::Update 或标准 PostUpdate 尾部调用。普通与 Odd 是同一 Manager 状态分支的替代路径，不能把 Odd 更新拼成普通完整样本。

普通 Game 主更新段的官方 REPENTOGON marker 位于 VA 0x6F77A4 / RVA 0x2F77A4。它之前有 ReviveMenu 更新判定：0x6F7762 可跳到 0x6F7A39 epilogue，另一条路径可在 0x6F77A3 返回；两者都绕开 Room 与标准尾部。进入 marker 后的普通顺序为：0x6F78DF 调 Room::Update（VA 0x7F7F60 / RVA 0x3F7F60），返回后继续 Game 原生处理，再在 0x6F7A34 调标准尾部（VA 0x846C90 / RVA 0x446C90），最后经 0x6F7A49 返回。Game 尾部调用点的 IDB code xref 只有该处。

CURRENT_SOURCE：官方 ASM.zhl:289-290 将 marker/return 分别命名为 Game_Update_MainUpdate 和 Game_Update_Return；ASMCallbacks.cpp:92-113 在 marker 执行 MC_PRE_UPDATE，返回 true 会跳到目标 0x6F7A39。因此该 callback 可以让 Manager 和 Game wrapper 都正常返回，却不经过普通 Room 与标准尾部。跳过路径仍执行 CommitRemovedEntities、render_sort 和 HUD::PostUpdate（ASMCallbacks.cpp:71-77），所以只能记作缺少普通阶段，不能声称跳过帧没有世界或列表副作用。没有观察到 callback skip 的实际原因时，阶段缺席不能写成已确认 MC_PRE_UPDATE 返回 true。

Room 中唯一对 0x413F00 的直接 call 位于 VA 0x7F93A7 / RVA 0x3F93A7。实参序列为 0x7F9399 push 0、0x7F939B lea ecx,[edi+0x1218]，随后 0x7F93A7 call sub_413F00。Room 主体只有一个 native ret：0x7FBD81。有限 CFG 摘录显示，0x7F9399 调用块支配唯一正常 ret；0x7F9356 和 0x7F935F 两个近前条件分支及中间 warning 路径均汇入该块，没有找到绕开该 EL 调用的正常 Room 返回分支。这个结论只描述目标函数静态正常控制流，不覆盖外部 Hook 不调用 super、异常或内部回调副作用。

实参 0 是直接指令证据。当前 C/S 绑定把 bool 参数名写作 partial（hooks.json:14），官方 EntityManager.cpp 的对应语义参数名为 isTransition；故此处只记录 EL 参数 0 / isTransition=false，不把它提升成完整世界 profile，也不称为所有 EntityList 调用都完整。

标准尾部 0x846C90 的主正常路径在 0x846D15 返回。IDA 函数项还含 0x846D16-0x846D63 错误处理 funclet：读取异常信息、打印 Error in "PostUpdate" call，最后返回恢复地址 0x846D04。0x846D63 不是第二个正常主路径 ret。即使尾部 wrapper 正常返回，也不能推出所有 Lua callback 都成功或全部执行；尾部内部可因 callback 状态关闭而不分派 Lua。

## Hook 顺序、短路与真实 body 证据

CURRENT_SOURCE：hooks.json:4-6、13-14、19、22-23 当前分别声明 Manager -10000、Game/tail/Room/EL/Odd 0、Player 外层 -10000 与内层到达标记 +10000。REPENTOGON 本地 HookSystem.cpp:397-424 使用 greater<int> multimap 并降序安装；Install 在 558 起构建入口 detour 与 super trampoline。高数先安装、低数后包住已装链，因此运行调用时低数先进入、最外层返回时低数最后退出。dllmain.cpp:69-76 对此 priority 语义也有文字说明。loader.cpp:130-190 先枚举并初始化 DLL，再统一调用 ZHL::Init；它支持该实现按 priority 安装，不证明用户机器当前进程里具体有哪些 DLL 或哪些 Hook 安装成功。

已核对的官方短路点包括：

- 官方 Game::Update priority-0 wrapper 在 super 前清 s_callbackPause（ASMCallbacks.cpp:30-34）；另有 priority-0 Game::Update Hook 在 super 后清理碰撞缓存（ASMGridEntityCollision.cpp:28-31）。对已读实现，Room 和标准尾部位于 Game wrapper 的 super 区间内，但不能由此固定同优先 C/S 与两个官方 Hook 的相对注册顺序。
- MC_PRE_UPDATE 在 Game marker 处可直接跳过普通 Game body，具体边界见前节。
- 官方 Player callback 1160 可不调用普通 Player super，改调 Entity::Original_Update（LuaInterfaces/CustomCallbacks.cpp:499-541）。因此外层 Player 返回、Room/tail 正常完成不保证该 Player 的目标派生更新到达。当前 Player ±10000 标记只证明已检阅链中的该层到达；未知更内层 Hook 与实机配置仍 UNKNOWN。

CURRENT_SOURCE：mod/tools/gen_binding.py:37-42 生成 Manager wrapper：CallScope 后构造 FrameScope，再调用一次 super。FrameScope 在 context.cpp:38-45 中调用 FrameBegin/FrameEnd。command.cpp:237-252 检测 Manager 重入并在 BeginManager 前增加 frame depth；tick.cpp:317-332 的 BeginManager 会 PollPeer、刷新生命周期、DrainPeer。若增加 world outer token，必须放在这些动作之前，避免把入口变更算到错误窗口。嵌套 Manager 的 xref 0x6F42AD 来自原生函数 0x6F3E10；当前 frame_depth 视为重入，不能给它分配独立完整样本。

Room SimulationScope（tick.cpp:419-470）只在 Manager+Game owner 下启动 StandardRoom tick；Room 正常返回先调用 NativeReturned，pipeline 进入 AwaitPostState。EndManager（tick.cpp:334-418）随后先 FinishPostState/提交 completed，再 EndOuter、发布 Peer，最后才进行设备中性化、移除与退休。若未来复制完整世界，静态位置要求是 Manager 下游 Hook 整链正常回到当前 -10000 wrapper 后、EndManager 的提交和退休动作前先准备复制；只有 pipeline 正常提交 completed 后才发布该批次。TickPostState 目前只有 Disabled（tick_pipeline.h:24-26）；接口将 NativeReturned、FinishPostState 与 EndOuter 分开（tick_pipeline.h:73-78），实现见 tick_pipeline.cpp:133-180。工程没有完整 world 阶段见证或复制逻辑。

## 完整采样的限度与 UNKNOWN

STATIC_INFERENCE：完整普通样本至少要求同一个最外层 Manager window 内，普通 Game owner、符合当前 Game 的 Room scope、标准尾部按顺序进入并正常完成，随后 Manager 下游整链正常返回；还需本轮恰有一个 StandardRoom candidate、排除重入/重复/不配对与 Start/Restore/Lifecycle/Door/Join 阻断，并在只读复制前后确认 root、Room、描述符、mutation、epoch、generation 稳定。Manager/Game/Room/尾部某一个独立正常返回、单独 EL 参数 0、或尾部 callback wrapper 返回，都不足以完整采样。

本次只确认现有 blocker 与 TopExit 排序：阻断上下文内不应取得完整普通资格；EndManager 中已完成 tick 先于退休清理。ST2 不重新验收当前 Continue/Restore 生命周期；相关完整生命周期在 ST7 再核对。

UNKNOWN：同优先级 Game Hook 的最终链顺序、实际 callback 状态、实机安装与其他 DLL 的额外 Hook、所有 Lua/原生回调副作用、未知更内层 Hook、目标字段是否满足完整 profile，以及所有异常/SEH/终止路径。本次不扩展 ST6/ST7 副作用闭包，不把缺失的动态事实写成静态确认。
