# RE-3：帧号写入点与显示计数

日期：2026-11-12。状态：**静态研究完成，未实施、未测试**。目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273。先查本地原版／REPENTOGON Wiki、SDK 与官方实现，再核对当前 J273 IDB；没有启动游戏、调用原生函数、修改目标内存、安装 Hook、构建或运行校验脚本。

> **2026-11-12 审阅后标注**：[原生时钟第 6 节](native-clocks.md#6-manager-的本地帧)的奇数帧快返（`0x92E441`）已写入方案 4.1：本机关闭插值时，Client 按同一条件跳过奇数帧的相机阶段；这类帧原生是否仍调用 Render，并入 RE-1 补遗。Effect Timeout／TimeCounter 在帧头每次前进时重写的开销，已计入第 13 步 13.3 的安装预算。

> **同日 RE-1 补遗已完成**：[绘制节拍](../RE-1/manager-render-cadence.md)确认主循环仍调用 Manager::Render，其绘制体按 Mpost 与本机插值设置判断；A 非零且 B 为零的奇数帧快返保持 M，不走 `0x92E441` 的自增，仍可绘制。此前的待查问题以该补遗为准。

任务来源：[显示委托方案第 6 节](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)。归入二进制重构第 13 步；不另开 tx，第 15 步仍是唯一的统一整体验收。

## 成果

- [原生时钟、写入位置与证据](native-clocks.md)：Game／Room／Entity getter，Game 普通递增、重置与恢复，Room 入房锚点，Manager 本地递增及其他写入。
- [显示依赖的计数目录](display-counters.md)：区分 Host 帧、Manager 帧、按调用推进的动画／倒计时、原始值与到期帧；交接 RE-4 的具体消费者。
- [安装点与现有代码交接](apply-contract.md)：帧号方案 B 的提交顺序、空帧、失败／停顿／回退、Manager 本地推进与旧借用路径退役。

## 核定结论

1. **Game 帧号的普通递增**在 `Game::Update 0x6F6D30`：`0x6F7889` 读取 `+0x25A70`，`0x6F789A` 在 ESI 加一，`0x6F789D` 写回；同分支 `0x6F7894` 递增独立的 `+0x25A74 TimeCounter`。发生在 Minimap 更新之后、Room 更新之前。Client 保持整条 Game::Update 拦截后，不需要为了显示运行这段递增。
2. **Room `+0x11F0` 没有本次原先假定的逐帧自增**。它在 Room::Init 的 `0x7E8CE8` 被赋为当时的 Game 帧号，是入房基准。`Room::GetFrameCount 0x809590` 在 initialized byte 为真时返回 `GameFrame−基准`，否则返回 −1。需要隔离的是原生 Init 重写与旧 Client 重新锚定，不是寻找一条 Room 的 `++`。
3. **另有 Game 帧写入**：公共重置 helper `0x6F0770`、NextVictoryLap `0x6F3E10`、RestoreState `0x6F43A0`。显示白名单不包含这些生命周期／恢复根；方案 B 不能靠每帧写回 F 掩盖它们已产生的副作用。
4. **Manager `+0x4A8B8` 是本地时钟**。正常 Manager 更新尾 `0x92EC09` 加一；关闭 interpolation 的奇数帧快返分支在 `0x92E441` 写 M+1。两者是不同路径，不能各补一次。初始化、启动奇偶对齐和原版消息恢复还会写它，均不是 Host game_frame 的安装来源。
5. **当前 Client 整条 Manager 原链也会跳过**，因此“保留本地 Manager 值”还必须落实本地推进职责。若维持该隔离边界，就在既有 Manager 主线程调度中安排本地计数及显示调用；不能仅增加一个 GameUpdate hook，等待原生 Manager 调到它。保持一个原生 Manager 字段，不增加另一套展示时钟。
6. **显示计数不是同一种时钟**：ANM2 Update 会按每次调用推进动画，HUD／地图／FX 也有本地计数；Game 帧被冻结不保证这些值冻结。水色／波纹／震屏／颜色等原生内联标量则按现行 **Q8 同步 Host 当前值，Client 不推进**。不能把所有计数都设置成 F 或补跑 ΔF 次完整更新；目录已按当前决定区分地址与 owner。
7. **按现行字段合同区别换算与原值**：TimeCounter 用 F−基准还原；Effect Timeout 若保留则用到期帧−F；Familiar FireCooldown 和 Pickup DropDelay 已按 2026-11-11 用户决定改成原始 i32，继续按采样安装，不恢复旧线性锚点。

## 三种原生帧值

| 值 | Client 来源 | 作用 |
| --- | --- | --- |
| `Game+0x25A70` F | 最近成功提交 world 的帧头原值；保持至下一次成功提交 | Game 时间、Entity／Room 年龄、原生 shader／HUD 等消费 |
| `Room+0x11F0` B、`Entity+0x328` E | 对应 world 的 Host 原始锚点 | 原生 getter 计算 F−B／F−E；不在绘制前重算锚点 |
| `Manager+0x4A8B8` M | Client 本地 Manager 调度 | 本地半帧、显示缓存、忽略暂停的 shader／可选插值；不借用 Host M |

本次只完成定位和职责交接，不声明新 Client 已具备本地 Manager 推进、原子帧安装或原生显示调用。报告收尾已合入工作区方案新增的 Q8／Manager 边界两档调度；RE-1／RE-2 原先可抽内联值推进的建议以现行方案 4.6 为准。RE-4 继续处理相等帧／周期判断、跳帧和重复调用；RE-5 处理 Render 写集，RE-9 处理转场计数及换房点。未分类路径保持拒绝，不扩展成整个引擎计数器或 Render 尾部的静态穷尽任务。

收尾只做静态文档审阅，核对来源、原始指令、地址／ABI、现行字段合同与旧代码差异；没有运行时观察或 E2E PASS。
