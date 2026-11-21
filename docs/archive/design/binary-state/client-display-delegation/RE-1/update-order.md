# RE-1：调用顺序与前置状态

返回：[成果总览](README.md)。本文件的地址均为 J273 VA；`J273_STATIC` 与后续 `DESIGN` 调度分开。

> **2026-11-12 补遗完成后的口径**：完整优先级、条件和各分支调用见[提前返回分支表](early-return-branches.md)；关闭插值的相机快返与 Render 的 Mpost 判定见[绘制节拍](manager-render-cadence.md)。本文件原提案中“本地推进内联标量”的建议已被 Q8 取代；排序使用自有 FullSort，原生 render_sort 保持拒绝。下文原始调用与副作用事实保留。

## 1. 原生实际顺序

### 1.1 普通主更新

`Game::Update 0x6F6D30` 中与本任务有关的实际顺序为：

| 顺序 | 位置 | 实际工作 |
| --- | --- | --- |
| 1 | `0x6F6D69` | 先检查／推进 `Game+0x67FD4` 的 helper，返回真则退出；不把它加入白名单 |
| 2 | `0x6F6D81` | `HUD::Update(Game+0x1D9AC)` |
| 3 | `0x6F6D91—0x6F72C1` | 屏幕计时、颜色、闪电、fade 等内联工作；fade 分支混有退出／保存／换局，详见[附表](additional-display-work.md) |
| 4 | `0x6F72C6—0x6F77A3` | 多个提前返回／演出／暂停／转场分支；不是一个全路径共享的显示序列 |
| 5 | `0x6F77B0` | `Minimap::Update(Game+0x25444)` |
| 6 | `0x6F77BB—0x6F78D8` | 其他演出／转场生产、GameFrame 与 TimeCounter 自增、周期 HUD dirty 请求；不能随显示整段放行 |
| 7 | `0x6F78DF` | `Room::Update`，其中 EntityList Update `0x413F00` 的尾部 `0x415023` 调 `render_sort` |
| 8 | `0x6F78E4`／`0x6F78EF` | `PlayerManager::Update`／其他 helper；不列入显示白名单 |
| 9 | `0x6F78F4—0x6F79AD` | 暗度、眩晕、retrovision 计时等内联显示值推进 |
| 10 | `0x6F79B9` | `HUD::PostUpdate`：先 PlayerHUD，再 HistoryHUD，再 StatHUD |
| 11 | `0x6F79BE` 之后 | DebugRenderer 容器维护／Lua PostUpdate 等原生尾部；不因位于 HUD 后就自动放行 |

因此“HUD Update 与 PlayerHUD Update 一组先调用，然后再 Minimap”并不符合 J273。HUD Update 主要推进消息与显示计时；心形／主动道具等 PlayerHUD 值更新在最后的 PostUpdate 中。

### 1.2 过渡分支与提前返回

`Game+0x1B83C` 的 mode 非零时：

```text
此前已经执行 HUD::Update
0x6F7420  RoomTransition::Update
0x6F742B  Minimap::Update
0x6F7436  HUD::PostUpdate
0x6F7441  过渡状态 helper；条件成立时清 Game+0x25B38
return
```

`RoomTransition::Update` 的原生换房／Boss 标记属于 RE-9；本任务只核入口、位置与相对顺序，不能据此宣布整条函数纯显示。

另外，`HUD::PostUpdate` 在以下路径还有调用点：

- fade 活跃后的 `0x6F72C1`；
- `Game+0x25A6C > 0` 的提前返回 `0x6F72D9`；
- StageTransition 更新后 `0x6F73FB`；
- 某些 ItemOverlay／暂停组合分支 `0x6F74B0`；
- 次级淡入／开场等待分支 `0x6F76B3`。

这些调用不是要求 Client 顺序执行的列表。特别是 fade 调用点之后原函数可能继续前进，再进入普通／其他分支，不能声称原生每次 Game::Update 都只调用一次 PostUpdate。GameOver／Leaderboard 也有自己的提前返回路径，不能以普通分支外推所有演出。

### 1.3 相机在 Manager 的独立阶段

原生 `Manager` 更新函数 `0x92E3E0` 的普通游戏分支：

1. ManagerFrame 偶数时 `0x92E911` 调 `Game::Update`；奇数条件分支 `0x92E921` 调 Odd helper。
2. `0x92E926—0x92E946` 取本地 `Manager+0x4A8B8 & 1`，并重新从当前 `Game.Room+0x11F8` 取 Camera。
3. `0x92E94A→Game::IsPaused 0x6F92A0`；暂停时通常跳过相机，RoomTransition 的特定状态 `Game+0x1B858 == 19` 是原生例外（2026-11-12 审阅更正：该字段是 RoomTransition `+0x1C` animation，19 为 MINECART）。
4. `Options.cameraStyle == 2`：`0x92E980→UpdateDrag2`；否则 `0x92E998→UpdateUltrasmooth(parity)`。
5. `UpdateUltrasmooth` 后总在 `0x92E99D` 清 `Camera+0x8C`；Drag 分支只有 parity 非零时走此清除点。

由此可知：相机不是两个函数依次执行，而是按本机选项二选一。bool 是 **Client 本地 Manager 半帧奇偶**，不是 Host game_frame 的奇偶。`CameraStyle` Wiki 将 1 命名为 ACTIVE_CAM_ON，2 命名为 ACTIVE_CAM_OFF。Lua `Camera:Update(flag)` 包装的 `DoUpdate 0x91D650` 也采用相同选择与清除逻辑，但后续实现宜直接使用已核的两个具名入口，避免误借完整 Manager 尾部。

## 2. HUD 的前置状态与允许副作用

### 2.1 HUD Update

`0x97ACF0` 读取并维护：

- HUD `+0x54EC` refresh 已消费标志、`+0x54F0` countdown、`+0x54E8` refresh type：标志为真时递减 countdown、清标志，countdown 到期再清 type（`0x97ACF9—0x97AD18`）。
- HUD `+0x54B4/+0x54B8` 显示倒计时。
- 主消息、六条堆叠消息、四个玩家消息；`0x8317A0` 只推进消息的 ANM／TextIn／TextStay／TextOut 与显示资源清理。
- fortune 等 Sprite；SlideIn／SlideOut 动画条件在 `0x97AE0C`／`0x97AE75` 调 SFX `0x907C60` 播音效 252。按 Q7，它是 Client 本地显示音效；Host 同路径不再发送重复事件。

**前置**：HUD、消息 owner、Sprite／文字资源已经正常构造；横幅／fortune 由事件回放初始化。不能把裸内存清零当作 HUD 初始化，也不能在每帧安装时用 Host 显示缓存覆盖本地动画历史。

### 2.2 HUD PostUpdate 的嵌套顺序

`0x97AE90` 已核为：

1. `0x97AEA3—0x97AEB8`：八槽，只有 `PlayerHUD+0x0` 非空才调 `0x832C10`，stride `0x6E0`。
2. `Manager.Options.historyHudMode > 0` 时：HUD `+0x5C8C` 尚未初始化则 `0x97AEDE→HistoryHUD::RecomputeAll(false) 0x8304B0`；再按两槽 dirty 位 `0x97AEFE→Recompute(index,true) 0x82FF40`。
3. `0x97AF12→StatHUD` 维护 `0x83C860`。其内部受本地 FoundHUD 选项控制，读取已同步 Player 属性，维护显示缓存和倒计时。

**前置**：world 安装必须已提交实际 Player roster、各身体关系、装备／主动槽／饰品、临时效果、资源和库存／History 记录。PlayerHUD 的 `_player`／`_HUD`／index 必须指向当前本地 owner。没有证据需要先运行 Player Update、EvaluateItems、拾取或 PlayerManager Update 才能取得这些显示结果；应由安装事务提供它们。

**刷新标志并非可删除**：`Game+0x22E94/+0x22E9C` 就是 HUD `+0x54E8/+0x54F0` 的别名。原生主更新会按 GameFrame 的 30 帧周期请求刷新（`0x6F78C5/0x6F78CF`）；PlayerHUD 尾部在图标／显示改变时也写 type=1、countdown=2，地图 Refresh 同样提出该请求。跳过 Game::Update 后，要保留安装变脏／本地维护的刷新语义，不能为了这一请求运行整个前段。周期、停帧和跳帧去重由 RE-4／第 13 步统一处理。

**输入依赖**：`PlayerHUD 0x834734` 查询 action=11（ACTION_DROP），只用于 `+0x6BC` 的显示渐变，并递减 `+0xC/+0xD/+0xE` 三色闪烁。它不执行 Drop 玩法 consumer。当前生成代码已用 `ConsumerScope(..., Drop)` 包装 PlayerHUD，但 Host 的 Armed 查询规则不能直接当 Client 显示查询已经接通；后续要供应该身体对应的 canonical 展示输入，不能按远端 controller index 读本机键盘。

**不可算值**：StatHUD 的默认重算包含恶魔／天使概率读取；默认 flags 不包含 planetarium 的 `0x100` 分支。概率按 RE-7 注入，进度按已有限定 getter 注入，不能读本机存档或重新进行 Host 规则裁决。Q6 放行 HUD／History／输入回调分派；本项目 Client Lua 不注册玩法回调的约束仍适用。

## 3. 地图更新与缓存刷新

`Minimap::Update 0x965DB0` 维护展开／收起状态、hold time、本机透明度和平滑，以及地图震动；**它不会替代脏地图重建**。

- action=13（ACTION_MAP）查询包含普通与原版 NetInputDevice 条件分支。C/S 不创建或复用原版 Online 会话；地图 UI 查询须按明确的本机 UI 所属关系供应，不能为这段查询启用原版联机。
- hold time 到 10 时 `0x965FF9→0x740740(true)` 生成楼层／诅咒横幅文本，随后 `0x831620` 展示；它不是清房／换层。释放地图输入时可能调 `HUD_Message::text_out`。这些是放行地图 UI 后本地产生的文字，后续事件协议不能把相同显示再次回放。
- 地图震动 `+0x61C` 与 `+0x620/+0x624` 在 `0x96600C—0x9660B6` 更新，并在 `0x966028` 消耗本地显示 Random。

权威可见性安装后，调用：

```text
Minimap::Refresh(this = Game+0x25444)
    +0x18  → 0x9635B0  # Game+0x2545C
    +0x178 → 0x9635B0  # Game+0x255BC
    如 refresh type != 2，设 type=1、countdown=2
```

`Game::UpdateVisibility 0x739AA0` 在相同两次缓存调用之前，会对当前房、另一维度和八门邻房调用可见性生产 helper 并修改 DisplayFlags（`0x739AC3/0x739AF4/0x739B27`）。Client 不运行这一前段。

**刷新时机与依赖**：描述表／维度／当前房、DisplayFlags、探索标志、显示诅咒、SavedEntities 图标输入，以及必要的本地图形缓存已经提交后再刷新；新房、视口／地图配置、资源变化也会使缓存失效，不能把触发条件只写成 DisplayFlags 单字段变化。`Minimap::Update` 的本地动画仍按自己的节奏推进，不因网络重复帧重建地图。

**不是纯 CPU 函数**：`0x96360C→PushCurrentRenderTarget`，随后设置本地图像 target、绘图、消费提交，尾 `0x9655A6→RestorePreviousRenderTarget`。`0x965565` 后还依 `dword_BAE1AC`／后端分支选择 depth clear、SwapBuffers 或另一后端提交。因此只能在当前游戏主线程、有效图形 context／资源及原生 target 关系内调用，不能从解码线程或未构造的 MinimapConfig 调用。后端异常／资源失败不具有通用回滚保证。

**特殊地图**：挑战 32（April's Fool）在 `0x963680—0x9636D5` 用当前 Room descriptor 的来源和本地栈 RNG 选择显示索引（`0x7DF4E0 RNG::RandomInt`）；这不是门可见性生产。若原版效果的可见规则要求相同，应保留 Host 选定索引／映射作为 RE-7 的必要输入，不能只以 Q4 忽略房间标记差异。现有 `ClientMinimapBuild` 对该挑战明确拒绝；本次未撤销其限制或宣称已经支持。

## 4. 排序与移除提交

`EntityList::render_sort` 在 EntityList Update `0x415023` 调用，静态 xrefs 没有来自 Game::Render／Room::Render 的第二处调用。原生 Render 直接消费维护好的 renderEL；白名单必须补上排序职责，不能因为包了 Game::Render 就以为它会自动排序。

已有并本次复读的限制：

- `0x415179/0x41517E` 在 size=0 时仍读取首元素并虚调用 GetRenderZ，Client 必须在外层跳过空表。
- 全局 scratch `0xBEBDB0` 首次按 capacity 分配（`0x4150E8`），原函数没有记录并检查后续扩容；跨世界列表增大不能假定旧 scratch 足够。必须在第 13 步保留已有有界全排或明确解决该容量责任，不能盲调原函数。
- 读取 GameFrame 奇偶，并以 capacity 派生预算做渐进排序，不保证新 baseline 一次调用就完成任意全排；不通过改写帧号补排序。普通深度、首次安装完整排序与安全工作区由同一排序 owner 负责，不能再在 Render 外层重复跑另一份。
- 当前列表、GetRenderZ 所需 Entity／Room 状态有效，无半套成员、退休对象、并发／重入，才能执行。

官方 `MC_PRE_UPDATE=true` 的 `handle_skipped_update` 顺序是 `CommitRemovedEntities → render_sort → HUD::PostUpdate`，只证明这是主更新跳过后的收尾安排。它不意味着前段 HUD／地图／相机都已隔离，更不意味着 Commit 是纯显示。

`EntityManager::CommitRemovedEntities` 会清五类 EL，owner 列表执行 NotifyDelete／ClearReferences，Player 在条件下 Free，并可能连带 Remove。当前 `client_objects_native.cpp` 的普通和正式 Remove 分支已由类型 owner 退休、清理多类列表、维护官方删除通知和类型查询缓存；Player 也有独立退链／退休 owner。**决定**：不在每帧显示根额外调用 Commit；确需处理官方 staged remove 时放进实际安装／退休事务，不能同时由两个 owner 管理同一个对象。排序只接已完成生命周期提交的列表。

## 5. 第 13 步可采用的调度安排

以下是依据静态事实给出的 `DESIGN`，本次没有实现：

1. 在主线程完成 world 安装、身份／关系／库存、注入及 GameFrame 写回；完成实际退休，重新取得当前 Room／Camera，不能跨换房持有旧地址。
2. 每次显示推进批次：`HUD::Update`，再执行明确拆出的前段值推进；转场活跃时进入 RE-9 的受控转场调用。
3. `Minimap::Update`；有脏地图时，在满足图形与输入依赖后 `Minimap::Refresh`。原生 Game::Update 没有固定的 Refresh 调用点，此位置是依赖驱动安排，不伪称原生顺序。
4. 维护有效 renderEL 的排序；后段纯标量显示推进；`HUD::PostUpdate` 一次，其内部完成 PlayerHUD／History／Stat。不能在外层再调 PlayerHUD，也不照抄全部提前返回调用点。
5. 每个本地显示帧、Render 前进入独立的相机阶段，按本机 CameraStyle 二选一；沿用本地半帧参数和 override 清理语义，Host 暂停按已有权威暂停状态限制。不要等约 30Hz 的 world 到包才推进约 60Hz 的相机。
6. 包装 `Game::Render`；已同步字段的漂移和写回按 13.4。排序不因每次 Render 再重复执行，地图不因每次 Render 再重复重建。

本地显示批次、停顿、合帧和跳帧的去重／节奏由 RE-3／RE-4 与第 13 步确定。当前 ClientManagerBoundary 整体跳过原生 Manager 更新，方案落地时必须显式接入上述阶段；仅在原生 GameUpdate hook 里写一个函数，不会自动得到被 Manager 拦截掉的调度。任何安排都不能重新运行 Odd／staged 位置恢复、PlayerManager Update 或碰撞来补画面。
