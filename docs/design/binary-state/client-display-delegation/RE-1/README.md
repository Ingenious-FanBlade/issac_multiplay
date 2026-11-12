# RE-1：显示推进白名单与调用顺序

日期：2026-11-12。状态：原任务及新增补遗均已完成静态研究；没有修改产品代码、安装 Hook、启动游戏、构建或运行测试。归属第 13 步，不另开 tx。

任务来源：[修改方案第 6.2 节](../../client-display-delegation.md#62-显示推进的白名单与调用顺序re-1)。本报告优先读取本地 Wiki、REPENTOGON SDK 和官方实现，再用当前 J273 IDB 的原始指令、交叉引用和反编译补齐地址、ABI、顺序及内联显示工作。未使用 J460 偏移或复制 isaac-lan 代码。

> **2026-11-12 审阅后标注**：用户随后决定 Q8，`Game::Update` 内联显示标量同步 Host 当前值，Client 不本地推进。下表“原提案的值推进”一行及附表中“可拆标量”的处理建议由[修改方案 4.6](../../client-display-delegation.md#46-内联显示标量q8)取代；地址、运算和副作用事实保留。原生 `render_sort` 保持拒绝，排序改由自有 FullSort 负责。调度按方案 6.2 及以下已完成的补遗执行，不将普通分支外推到所有暂停／演出状态。
>
> **2026-11-12 补遗审阅后标注**（IDA J273 复核，原文保留）：
>
> - 关键结论 5 及补遗中的“转场阶段 19”应为 RoomTransition `+0x1C`（`Game+0x1B858`）的 animation 19，即 MINECART。
> - **Q12（用户决定）**：提前返回分支的选择由 Host 每个 world 帧发送的终结分支枚举与 paused 位承接；`client_entity_draw_paused` 保留；Pα 以已安装 primary alpha `>0` 判定（方案 6.2）。
> - **Q11（用户决定）**：关键结论 1、3 中的 `RoomTransition::Update` 在 Client 上完整拦截，不拆分；转场显示字段随 world 安装（方案 4.5）。
> - **RE-10（新开）**：Manager::Update 尾部除相机外还有约 50 个非 Game 调用，至少音乐 tick `0x7D7DA0`（`0x92EB49`）与 `0x908340`（`0x92EB3E`）须在显示推进根中运行；本 RE 未覆盖，见方案第 6 节。

## 结果入口

- [入口与签名](entrypoints.md)：J273 VA／RVA、SDK 声明、对象偏移、原始返回指令及定位模式。
- [调用顺序与前置状态](update-order.md)：普通／过渡／提前返回分支、HUD 依赖、地图缓存、相机、排序与移除提交。
- [遗漏的显示工作与拆分边界](additional-display-work.md)：震屏、颜色、闪电、暗度、眩晕、淡入淡出及其他显示计时。
- [补遗：提前返回分支表](early-return-branches.md)：优先级、条件字段、各分支 U／P／M 序列及综合 paused 的区别。
- [补遗：关闭插值的绘制节拍](manager-render-cadence.md)：主循环仍调用 Render 入口、按 Mpost 跳过绘制体，以及 A／B 保持 M 的例外。
- [补遗：条件需求与实现交接](branch-demand-handoff.md)：明确需求、旧 UNKNOWN 占位、本地／Host 归属及第 7／8／13／15 步交接。

## 关键结论

1. `Game::Update` 继续整体拦截。候选中能够独立调用的是 HUD、Minimap、地图缓存刷新和两个相机入口；排序交自有 FullSort，原生 `render_sort` 不放行。`RoomTransition::Update` 仍是待 RE-9 完成换房替换后的混合入口。
2. 普通原生顺序是 `HUD::Update → Minimap::Update → Room::Update 内的 EntityList::render_sort → HUD::PostUpdate`。`PlayerHUD::Update` 是 PostUpdate 内部对八槽的嵌套调用，外层不得再遍历一次。
3. 过渡分支是 `HUD::Update → RoomTransition::Update → Minimap::Update → HUD::PostUpdate → return`；不能把候选表中“转场永远最先”当作 J273 事实。
4. 地图缓存对象是 `Game+0x2545C`、`Game+0x255BC`，共同构建函数为 VA `0x9635B0`。优先调用 SDK `Minimap::Refresh`（VA `0x965D60`），不用整条 `Game::UpdateVisibility`（VA `0x739AA0`）重算权威 DisplayFlags。
5. 相机在 Manager 更新尾部、两个半帧均有调度机会，**不在 `Game::Update` 内**。`CameraStyle=2` 调 `UpdateDrag2()`，其他值调 `UpdateUltrasmooth(ManagerFrame & 1)`；两者二选一。关闭本机插值的奇数相位先快返，不运行相机；暂停时另有转场阶段 19 例外。
6. 两个相机入口内部都可能调用本地共享 `Random()`。旧材料中的“Manager 相机尾部存在条件 RNG”不能理解为“绕开尾部后这两个入口就没有 RNG”。这些调用用于震屏外观，按 Q4 可在 Client 本地发生，不构成放行玩法随机的授权。
7. `render_sort` 的原生调用在 EntityList 更新尾部。当前静态交叉引用没有显示 `Game::Render` 树再次调用它；Client 应显式安排自有有界 FullSort，原生入口因空表、共享 scratch 容量、按 GameFrame 奇偶渐进等问题保持拒绝。
8. `CommitRemovedEntities` 是生命周期提交，**不进入逐帧显示白名单**。现有 world 应用器已经承担摘表、引用清理和退休；新增每帧提交会重复或绕过它的责任。需要补交官方 staged remove 时，应归入实际安装／退休事务，并在排序前完成。
9. 只有原来的候选函数还不够：震屏计时、颜色渐变、闪电衰减、暗度／眩晕平滑、bloom／hallucination／retrovision 计时都有 `Game::Update` 内联生产。淡出完成分支还会退出、存档、换局，不能整段作为显示工作调用。
10. 补遗已覆盖 Console、hold、GameOver、Leaderboard、换层、切房、Overlay／暂停组合、Prompt、次级 fade、PauseScreen 和额外 UI 的提前返回；单个 paused 不能选出 U／P／M 序列。primary fade 的 Pα 不是 return，原生同次 Update 可能再调 P。
11. 关闭插值时，主循环仍调用 `Manager::Render`，绘制体条件为 `I || (Mpost & 1)`。通常奇数 Mpre 快返自增至偶数而不绘制；`A!=0 && B==0` 快返保持奇数 M 时仍可绘制。Game::Render 还受 Manager 游戏状态分派约束。

## 白名单的使用口径

这是有前置条件的**静态调用清单**，不是已经接通的 Client 实现，也不是运行安全结论。

| 判定 | 项目 |
| --- | --- |
| 可纳入具名显示调用 | `HUD::Update`、`HUD::PostUpdate`（包含 PlayerHUD／HistoryHUD／StatHUD）、`Minimap::Update`、脏地图的 `Minimap::Refresh`；相机二选一；自有 FullSort 负责排序 |
| 嵌套登记，外层不重复调用 | `PlayerHUD::Update`、两份地图缓存构建 `0x9635B0` |
| 原提案的值推进（已被 Q8 取代） | 颜色渐变 helper `0x6EB990` 及附表中的纯标量：保留静态事实，Client 安装 Host 当前值，不本地调用推进 |
| 混合，保留拦截直到替换具体子路径 | `RoomTransition::Update`（RE-9）、完整淡出 FSM；Room／背景／水面由 RE-2 分类 |
| 不作为显示调用 | 整条 `Game::Update`、`Room::Update`、EntityList／Player／PlayerManager Update、Odd helper、`ItemOverlay::Update`、权威可见性重算、`CommitRemovedEntities` |

所有放行路径遵循 Q6：回调分派放行，本项目 Lua 不在 Client 注册玩法回调。HUD 的属性／概率 getter 必须读取已安装结果或 RE-7 注入值；地图输入和 HUD Drop 查询须按既有统一输入语义供应，不能把远端 controller index 当本机物理设备。已同步字段上的本地写入交第 13.4 节漂移校正。

## 本次完成与后续边界

RE-1 的四项原要求均已有记录：候选函数的 J273 绑定／ABI／顺序、两份地图缓存偏移、相机字段与遗漏显示工作、HUD 对原生前段状态的依赖。新增补遗也已完成：分支优先级与条件、各分支显示调用、条件的需求归属，以及关闭插值的 Update→Render 节拍。

RE-2—RE-5 已有静态成果，按各自交接落实；其余研究仍按 RE-7 的权威 getter／地图图标输入、RE-8 的音效来源、RE-9 的转场换房替换推进。本补遗识别出的窄分支条件与 Pα 调度收口归第 7／8／13 步，不等于已接通协议或显示实现。特殊角色／房间的实际显示、停顿与跳帧效果、context／资源失败及性能在第 15 步统一确认。

本次仅作整体静态文档审阅：对照原任务及补遗要求、SDK 声明、调用点、需求清单和现有应用器职责；没有实机观察，不能标为 E2E PASS。
