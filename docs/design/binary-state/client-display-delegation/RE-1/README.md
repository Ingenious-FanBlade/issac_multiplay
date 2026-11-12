# RE-1：显示推进白名单与调用顺序

日期：2026-11-12。状态：本任务静态研究完成；没有修改产品代码、安装 Hook、启动游戏、构建或运行测试。归属第 13 步，不另开 tx。

任务来源：[修改方案第 6.2 节](../../client-display-delegation.md#62-显示推进的白名单与调用顺序re-1)。本报告优先读取本地 Wiki、REPENTOGON SDK 和官方实现，再用当前 J273 IDB 的原始指令、交叉引用和反编译补齐地址、ABI、顺序及内联显示工作。未使用 J460 偏移或复制 isaac-lan 代码。

> **2026-11-12 审阅后标注**：用户随后决定 Q8，`Game::Update` 内联显示标量同步 Host 当前值，Client 不本地推进。下表“可拆出的值推进”一行及附表中“可拆标量”的处理建议由[修改方案 4.6](../../client-display-delegation.md#46-内联显示标量q8)取代；地址、运算和副作用事实保留。调度按方案 6.2 的“J273 核定后的显示推进”执行。提前返回分支表作为本任务补遗，尚未完成。

## 结果入口

- [入口与签名](entrypoints.md)：J273 VA／RVA、SDK 声明、对象偏移、原始返回指令及定位模式。
- [调用顺序与前置状态](update-order.md)：普通／过渡／提前返回分支、HUD 依赖、地图缓存、相机、排序与移除提交。
- [遗漏的显示工作与拆分边界](additional-display-work.md)：震屏、颜色、闪电、暗度、眩晕、淡入淡出及其他显示计时。

## 关键结论

1. `Game::Update` 继续整体拦截。候选中能够独立调用的是 HUD、Minimap、地图缓存刷新、排序和两个相机入口；`RoomTransition::Update` 仍是待 RE-9 完成换房替换后的混合入口。
2. 普通原生顺序是 `HUD::Update → Minimap::Update → Room::Update 内的 EntityList::render_sort → HUD::PostUpdate`。`PlayerHUD::Update` 是 PostUpdate 内部对八槽的嵌套调用，外层不得再遍历一次。
3. 过渡分支是 `HUD::Update → RoomTransition::Update → Minimap::Update → HUD::PostUpdate → return`；不能把候选表中“转场永远最先”当作 J273 事实。
4. 地图缓存对象是 `Game+0x2545C`、`Game+0x255BC`，共同构建函数为 VA `0x9635B0`。优先调用 SDK `Minimap::Refresh`（VA `0x965D60`），不用整条 `Game::UpdateVisibility`（VA `0x739AA0`）重算权威 DisplayFlags。
5. 相机在 Manager 更新尾部、两个半帧均有调度机会，**不在 `Game::Update` 内**。`CameraStyle=2` 调 `UpdateDrag2()`，其他值调 `UpdateUltrasmooth(ManagerFrame & 1)`；两者二选一。不能按候选表连续调用两个入口。
6. 两个相机入口内部都可能调用本地共享 `Random()`。旧材料中的“Manager 相机尾部存在条件 RNG”不能理解为“绕开尾部后这两个入口就没有 RNG”。这些调用用于震屏外观，按 Q4 可在 Client 本地发生，不构成放行玩法随机的授权。
7. `render_sort` 的原生调用在 EntityList 更新尾部。当前静态交叉引用没有显示 `Game::Render` 树再次调用它；Client 应显式安排一次排序维护。它有空列表和共享 scratch 容量的已知限制，不能无条件调用。
8. `CommitRemovedEntities` 是生命周期提交，**不进入逐帧显示白名单**。现有 world 应用器已经承担摘表、引用清理和退休；新增每帧提交会重复或绕过它的责任。需要补交官方 staged remove 时，应归入实际安装／退休事务，并在排序前完成。
9. 只有原来的候选函数还不够：震屏计时、颜色渐变、闪电衰减、暗度／眩晕平滑、bloom／hallucination／retrovision 计时都有 `Game::Update` 内联生产。淡出完成分支还会退出、存档、换局，不能整段作为显示工作调用。

## 白名单的使用口径

这是有前置条件的**静态调用清单**，不是已经接通的 Client 实现，也不是运行安全结论。

| 判定 | 项目 |
| --- | --- |
| 可纳入具名显示调用 | `HUD::Update`、`HUD::PostUpdate`（包含 PlayerHUD／HistoryHUD／StatHUD）、`Minimap::Update`、脏地图的 `Minimap::Refresh`；相机二选一；受列表／scratch 条件约束的 `render_sort` |
| 嵌套登记，外层不重复调用 | `PlayerHUD::Update`、两份地图缓存构建 `0x9635B0` |
| 可拆出的值推进 | 颜色渐变 helper `0x6EB990`，以及附表中已经核定的纯标量更新；由第 13 步实现，不能跳进原函数中段冒充可调用函数 |
| 混合，保留拦截直到替换具体子路径 | `RoomTransition::Update`（RE-9）、完整淡出 FSM；Room／背景／水面由 RE-2 分类 |
| 不作为显示调用 | 整条 `Game::Update`、`Room::Update`、EntityList／Player／PlayerManager Update、Odd helper、`ItemOverlay::Update`、权威可见性重算、`CommitRemovedEntities` |

所有放行路径遵循 Q6：回调分派放行，本项目 Lua 不在 Client 注册玩法回调。HUD 的属性／概率 getter 必须读取已安装结果或 RE-7 注入值；地图输入和 HUD Drop 查询须按既有统一输入语义供应，不能把远端 controller index 当本机物理设备。已同步字段上的本地写入交第 13.4 节漂移校正。

## 本次完成与后续边界

RE-1 的四项要求均已有记录：候选函数的 J273 绑定／ABI／顺序、两份地图缓存偏移、相机字段与遗漏显示工作、HUD 对原生前段状态的依赖。

尚未实施的部分按原任务归属继续处理：RE-2 的房间显示子链、RE-3／RE-4 的帧消费和跳帧策略、RE-7 的权威 getter／地图图标输入、RE-8 的音效来源、RE-9 的转场换房替换。当前报告不把这些任务的未决项扩成新的 RE-1 前置要求，也不提前运行验收。特殊角色／房间的实际显示、停顿与跳帧效果、context／资源失败及性能在第 15 步统一确认。

本次仅作静态文档审阅：对照四项任务要求、SDK 声明、调用点和现有应用器职责；没有实机观察，不能标为 E2E PASS。
