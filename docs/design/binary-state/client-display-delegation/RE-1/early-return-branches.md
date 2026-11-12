# RE-1 补遗：Game::Update 提前返回分支

日期：2026-11-12。返回：[RE-1 总览](README.md)。本文件完成修改方案 6.2 新增的分支表要求；均为 `J273_STATIC`，没有实机观察。字段的需求归属另见[需求交接](branch-demand-handoff.md)，关闭插值的 Render 问题见[绘制节拍](manager-render-cadence.md)。

## 1. 资料与读表方式

先查本地原版 Wiki `F:\projects_isaac\IsaacDocs\docs\Game.md` 的 `Game:IsPaused`，以及 REPENTOGON Wiki `F:\projects_isaac\REPENTOGON-1.1.3\docs\docs\Game.md` 的 `IsPauseMenuOpen`／`GetPauseMenuState`、`enums\PauseMenuStates.md`。Wiki 的 IsPaused 包含菜单、过渡和演出，不能等同“暂停菜单已打开”。REPENTOGON 的公开暂停菜单枚举只列 0／1／2，不能据此补全全部原生 UI 状态机。

SDK 入口为 `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\` 下的 `Game.zhl`、`Console.zhl`、`ItemOverlay.zhl`、`PauseScreen.zhl`、`StageTransition.zhl`、`GameOver.zhl`、`Leaderboard.zhl`、`GenericPrompt.zhl`。未暴露的判断再查当前 J273 IDB：模块 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，image base `0x400000`。以下地址均为 VA；RVA＝VA−`0x400000`。没有套用 J460 偏移。

- `U`＝`HUD::Update 0x97ACF0`；`P`＝`HUD::PostUpdate 0x97AE90`，已包含 PlayerHUD／HistoryHUD／StatHUD。
- `M`＝`Minimap::Update 0x965DB0`；`T`＝原生 `RoomTransition::Update 0x826430`。T 是混合入口，须由 RE-9 替换换房路径后才可用于 Client。
- 表中列的是上述显示候选的原生调用序列，另列不应照搬的混合 helper；没有把所有 UI helper 宣布为纯显示。
- `N`＝原版 Online 集合非空：`Manager+0x4AFB8 != Manager+0x4AFBC`。C/S 不建立这份原版集合，不将 C/S 参与者数或身体数填进去。

## 2. 所有后续分支共享的前段

`Game::Update` 入口 VA `0x6F6D30`／RVA `0x2F6D30`：

| 先后 | 条件／位置 | 原生行为 | Client 边界 |
| --- | --- | --- | --- |
| 1 | `0x6F6D69 → 0x687020(Game+0x67FD4)`；返回真时 `0x6F6D70` 跳到返回尾 | Console helper 阻断时，连 U 都没有运行 | Console 为本地 owner；不借整条含输入的更新来判定显示分支，不复制 Host 控制台对象 |
| 2 | `0x6F6D81` | U | HUD 与 world 已构建／安装之后才调用 |
| 3 | `0x6F6D91—0x6F72C1` | 各种标量生产；若进入该段时 primary alpha `Game+0x25B28 > 0`，还在 `0x6F72C1` 调一次 P，以下记为 `Pα` | Q8 不在 Client 运行这段标量／fade FSM；它混有保存、退出、换局。Pα 是调用顺序事实，不是整段放行依据 |
| 4 | `0x6F72C6` 起 | 按下一节优先级选择终结分支；普通路径最后才到达 | 不能先用一个 paused 布尔值把全部分支合为一类 |

**Pα 不结束 Game::Update。** 例如 primary fade 与 pause_countdown 同时满足时，原生会先 Pα，后在 `0x6F72D9` 再 P。因此“每次原生 Game::Update 恰好一次 PostUpdate”是错误结论。Q8 安装后的 alpha 也未必能还原 Host 更新前是否进入该段：正数减至零是直接反例。Client 不按到包次数或跳过的 Host 帧补演这段历史；第 13 步需明确当前状态下 Pα 的显示资格及是否合并，不能以 RE-4 的“同本地周期不重复批次”推导原生所有 P 调用点只能有一次。

## 3. 终结分支表：严格按出现次序

能到达以下任意一行时，此前已执行 U，并可能执行 Pα。下表“追加序列”从前段之后开始记，不重复写这两项。命中会返回的高优先级行时，不再检查低优先级行。

| 优先级／判断位置 | 条件字段与精确条件 | 追加的显示候选调用 | 原生同时执行的工作／返回行为 |
| --- | --- | --- | --- |
| 1 `0x6F72C6` | `i32 Game+0x25A6C > 0` | P `0x6F72D9` | 原生先将计数减一，再返回；Client 不递减该 Host 计数 |
| 2 `0x6F72EF` | `u32 Game+0x1D4C8 != 0`（GameOver owner 首字） | 无追加 P／M | 清 `+0x1D5E4` 音频标志，`0x6F737F → 0x98D4E0` 更新 PauseScreen，并改额外 UI 后返回；不能为获得菜单显示运行这条混合更新 |
| 3 `0x6F73C0` | `u32 Game+0x1D5FC != 0`（Leaderboard owner `+4`） | 无追加 P／M | `0x6F73CF → 0x83E2B0` 更新 Leaderboard 后返回 |
| 4 `0x6F73E5` | `u32 Game+0x1BA78 != 0`（StageTransition owner 首字） | P `0x6F73FB` | `0x6F73F4 → 0x909100` 更新换层状态后返回；SDK 只公开该 owner 的 `_sameStage +0x10`，首字条件来自指令，未擅自命名枚举 |
| 5 `0x6F7411` | `u32 Game+0x1B83C != 0`（RoomTransition mode） | T `0x6F7420` → M `0x6F742B` → P `0x6F7436` | `0x6F7441 → 0x6EC520` 还可能清 `+0x25B38`，随后返回；换房点替换、等待归 RE-9，不运行普通 Room 显示子链 |
| 6a `0x6F7469` 起 | ItemOverlay `Game+0x1C034 != 0`，且 PauseScreen `Game+0x2363C == 0` **或 N**；调用更新后重新读 Overlay state，结果 `==2` | P `0x6F74B0` | `0x6F749B → ItemOverlay::Update(false) 0x984110` 在前；暂停菜单非零时另调 `0x98D4E0`，然后返回。此 Update 含治疗／换房等，Client 不调用 |
| 6b 同上 | 上行 Update 后 Overlay state `!=2`；或者 Overlay 非零、PauseScreen 非零且非 N，使 Update 整个被跳过 | 不在此追加；继续检查次级 fade | **Overlay 非零不等于必然返回**。SDK state 1 是 WAITING、2 是 ACTIVE；分支 6a 读的是 Update 之后的值 |
| 7 `0x6F74DE` 起，仅 Overlay state 为零时检查 | `GenericPrompt::IsActive(Game+0x25CF0)` 为真；调用点 `0x6F74E4 → 0x82CB90` | 无追加 P／M | `0x6F74F3 → GenericPrompt::Update(false) 0x82C440` 后返回；选择结果还可能启动 fade target 7。不能用这个含输入／生命周期的 Update 取得分支结果 |
| 8 `0x6F759C`，同样仅 Overlay state 为零且 Prompt 不活跃 | `byte Game+0x24ECC != 0` | 无追加 P／M | 直接返回；语义未命名，保留确切谓词，不猜为某个公开菜单状态 |
| 9 `0x6F75AF—0x6F76C8` | 原生先处理 secondary alpha `Game+0x25B10`，`0x6F76A3` **重读后的值仍 >0** | P `0x6F76B3` | 更新条件含 N、`Game+0x25F60`、本机墙钟；到零可能 ShowName。Q8 只安装 Host 当前 alpha，不重跑这些生产／完成动作。到零后原生可在同次 Update 继续向下 |
| 10 `0x6F7703` 起 | PauseScreen state `Game+0x2363C !=0`，更新后 `0x6F775B → 0x98ED50` 为真 | 无追加 P／M | 先 `0x6F7717 → 0x98D4E0`，还清实体列表；谓词为菜单 state 非零且原版集合为空或仅一个 4-byte 项。若谓词假则继续。C/S 原版集合为空，非零菜单会阻断 |
| 11 `0x6F7771—0x6F778C` | 非 N，且 `u32 Game+0x24444 !=0` | 无追加 P／M | `0x6F778E → 0x8CCCC0` 更新额外 UI，随后返回；该 owner／状态语义未命名 |
| 12 `0x6F77B0` 起 | 未被以上条件截断 | M `0x6F77B0` → Room 内显示工作 → P `0x6F79B9` | 中间还含实体／玩家模拟、帧自增和其他混合 helper，Client 仍只取具名白名单：自有 FullSort、RE-2 的 FX／ANM2 子段等，不调用 Room::Update |

在第 10 行前还有本地自动打开暂停菜单的生产条件：`!Manager[byte +0x4AFAA] && Manager[byte +0x2A1D7] && !N && PauseState==0`，`0x6F76F8 → PauseScreen::Show 0x98ED80`。它不授权 Client 替 Host 打开权威暂停；本地菜单输入仍归本地 owner。

## 4. 相机的 paused 与上述分支并非同一个选择器

SDK `Game::IsPaused` 对应 VA `0x6F92A0`／RVA `0x2F92A0`（入口模式 `8b d1 56 8b 35`）。其组合谓词依次为：

| 条件 | 来源 |
| --- | --- |
| Console state `Game+0x67FD4 !=0`，且原版集合为空或仅一项 | Console／原版模式条件 |
| `Game+0x25A6C >0` | pause_countdown |
| `Game+0x1D4C8 !=0`、`+0x1D5FC !=0`、`+0x1BA78 !=0`、`+0x1B83C !=0` 中任一个 | GameOver／Leaderboard／换层／切房 |
| `Game+0x1C034 ==2` | ACTIVE Overlay；WAITING 1 本身不满足这一项 |
| `0x98ED50(Game+0x2363C)` | PauseScreen 非零与原版集合条件 |
| `Manager+0x8 !=2` | 当前本机 Manager 不在原生游戏状态 |
| `GenericPrompt::IsActive(Game+0x25CF0)` 或 `byte Game+0x24ECC !=0` | Prompt／未命名 UI 阻断 |

GenericPrompt 的 IsActive 实际读取 owner 的 `+0x34` 指针非空及 `+0x44` 播放字节，即 `Game+0x25D24`／`+0x25D34`；不是读取 Prompt 首字。跨进程传递的是必要语义或本地资源状态，不能传 ANM2 指针原值。

**primary／secondary alpha 不直接出现在 IsPaused 中。** 因而 Game::Update 在次级 fade 处返回，不代表相机一定被暂停。Manager 在 `0x92E94A` 查询 paused；为真时，只有 RoomTransition mode 非零且 `Game+0x1B858 ==19` 才继续相机。即便满足阶段 19，若先命中关闭插值的 Manager 快返，也到不了相机，见[绘制节拍](manager-render-cadence.md)。

> **2026-11-12 审阅更正**：`Game+0x1B858` 是 RoomTransition `+0x1C`，由 Start 在 `0x823B05` 写入的 animation 参数，19 为 `RoomTransitionAnim.MINECART`，不是“阶段 19”。本补遗中的“phase／阶段 19”均按此理解。
>
> **Q12（2026-11-12 用户决定）**：上述分支的选择改由 Host 每个 world 帧发送的终结分支枚举承接（按本表优先级、在 Update 之后的状态上计算），paused 位取 Host `Game::IsPaused` 结果；`client_entity_draw_paused` 保留以向 IsPaused 的原生调用者返回该位。见[修改方案 6.2](../../client-display-delegation.md#62-显示推进的白名单与调用顺序re-1)。

公开 `IsPauseMenuOpen` 的 SDK 内联仅返回 `PauseScreen.state >0`，它既不能替代组合 paused，也不能决定 U／P／M 的整个序列。Client 原生 Manager 可能仍在本地菜单状态，直接调用 IsPaused 又会被 `Manager.state !=2` 判真；显示暂停必须来自与已安装 world 对齐的 Host 语义／受控查询，本地菜单阻断另由本地 owner 处理。

## 5. 本补遗完成范围

已给出提前返回顺序、全部相关条件、显示候选调用以及每类混合副作用。各字段是否已进入当前需求清单，逐行登记在[需求交接](branch-demand-handoff.md)。未命名字段仍保留 UNKNOWN 语义，但判断本身已有原始指令依据；不为完成本任务扩大到全部菜单／演出 Update 的副作用闭包。

实现时不能执行被拒绝的混合 helper 来“算出应走哪个显示分支”，也不能在选择器缺失时补零并落到普通分支。新布局／Host 来源／Client 接线归第 7、8、13 步；第 15 步统一确认状态组合、停顿、跳帧和显示差异，本次未运行测试。
