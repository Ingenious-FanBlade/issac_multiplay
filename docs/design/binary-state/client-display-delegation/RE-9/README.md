# RE-9：J273 原生切房入口、阶段与显示拆分

日期：2026-11-12。状态：**五项静态研究及实现交接完成；未实施、未测试**。目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273，IDB imagebase=`0x400000`；VA／RVA 在正文分别注明。先查本地 Wiki、SDK、官方实现与既有 RE-1／RE-2／RE-5，再补 J273 静态分析。没有运行游戏、调用原生函数、改 IDB／目标内存、安装 Hook、生成、构建或运行测试。

成果：

- [原生入口、阶段、换房链与 Render 写集](native-transition.md)。
- [事件关联、等待与 Client 拆分交接](client-split-contract.md)。

任务来自[方案第6节](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)，五项对应如下：

| 要求 | 结果 |
| --- | --- |
| 1. J273 启动入口／ABI，Game 外层选路边界 | 原生 Start=`VA 0x823990 / RVA 0x423990`，ECX=this，五个32位栈参数，`retn 0x14`；Game::StartRoomTransition=`0x6F9710`，其 `0x6F9AD4` 调用内部 Start。两层都有额外行为，不能仅因地址已找到就整体放行 |
| 2. Update 阶段、ChangeRoom 与等待 | `0x826430` 的 mode1—4 已列表，ChangeRoom callsite为 `0x826AA6／0x826AC6／0x826BE9`。大量 Player 直接写入／Update 不在 ChangeRoom 内；等待必须在调度／阶段 owner 处，不能让 void ChangeRoom 返回“未完成”后继续原 caller |
| 3. ChangeRoom→Level→Init 副作用及显示缺失 | `0x8254B0→0x6FA0F0→0x739DE0→0x7385F0→0x7E8AD0`，还含旧房保存、Maze RNG、玩家状态、额外 RoomUpdate、进房选曲及显式 present。显示资源、两张本地 surface、相机／门／滚动与 Boss 姿态须由 NativeBuild／NativeApply 和显示 owner分别补齐 |
| 4. Game Render resize 分支、过渡 Render | `0x703E40` 由本机 resize byte触发，重建两张surface和本地资源、置pit dirty及surface origin；`0x8273C0`只在mode2／3绘制，Boss分支还改额外肖像Sprite的局部变换／颜色；已列写集与恢复约束 |
| 5. Host参数采集 | Start入口捕获请求与Player稳定身份、返回后读取接受／归一化字段；ChangeRoom正常完成后的实际room／dimension／world版本绑定最终目标。Maze等可能在Level::ChangeRoom才重选，不能把Start入口参数当作最终目的地 |

**对原候选方案的必要修正**：`RoomTransition::Start` 和 `RoomTransition::Update` 都是混合入口。只拦截 `ChangeRoom` 不足以隔离玩法；Boss Update 完成分支也写 `Game+0x22E90` 和 Player `+0x418`。保持原生过渡对象／Render／本地surface的 Q5 方向，Client 需要具名拆分，只推进显示段。本文提出窄显示驱动的实施建议，**没有把它写成新的用户决定，也没有声称已实现**。

原生整根在拆分接通前继续拦截。第三方 Mod、换层 StageTransition、任意异常／GPU恢复不在本轮外推。断线恢复、资源owner、原生特殊动画和性能仍随第15步 E39 整体验收；本报告没有运行结果。

> **2026-11-12 审阅后标注**（IDA J273 复核，原文保留）：
>
> - **Q11（用户决定）取代本文的窄显示驱动建议**：Client 不运行 Start／Update 的任何部分，也不写复现 FSM 的自有驱动。RoomTransition 的 mode `+0`、counter `+4`、duration `+8`、progress `+0xC`、direction `+0x10`、roomIndex `+0x14`、dimension `+0x18`、animation `+0x1C`、Boss 标记 `+0x238` 按 Host 当前值随 world 帧原子安装。Client 本地只做：安装新房间 world 之前捕获旧 surface、安装之后捕获新 surface、按本地相机计算 `+0x22C/+0x230`、推进 versus／肖像 ANM2、按已安装 mode2／3 提出 HUD 刷新请求，以及原生 `RoomTransition::Render`。新房间 world 晚到时，原子安装天然停在“旧房＋Host 当前进度”，不需要冻结 FSM。见[方案 4.5](../../client-display-delegation.md#45-原生切房过渡q5)。
> - `Game+0x22E90` 是 HUD `_isVisible`（`HUD.zhl:66`，HUD 位于 `Game+0x1D9AC`），按 Q8 同步 Host 当前值；Boss 完成分支的 `Player+0x418` 仍是 Host 上的玩法写入。
> - 转场内的声音（`0x823CE1` 608／609、`0x826D79` sound 1、`0x826FBA` sound 214）在 Q11 下全部作为 Host 事件。
> - 本文的事件／最终目标关联、resize、断线与特殊动画结论继续有效；[拆分交接](client-split-contract.md)第 5 节失败方式已并入方案第 12 节第 60—63 条。
