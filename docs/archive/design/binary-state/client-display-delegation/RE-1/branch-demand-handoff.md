# RE-1 补遗：分支条件的需求清单与交接

日期：2026-11-12。返回：[RE-1 总览](README.md)。依据[提前返回分支表](early-return-branches.md)和[Manager 绘制节拍](manager-render-cadence.md)，逐项回答“条件字段是否已在需求清单中”。本次只记录研究与设计交接，没有修改布局源、wire、整数版本、Hook 或产品代码。

> **2026-11-12 审阅后标注**（原文保留）：
>
> - **Q12（用户决定）**：表中 hold、GameOver、Leaderboard、StageTransition、Prompt、未命名 UI、PauseScreen 等“缺少窄条件”的分支，不再逐项同步窄谓词，改由 Host 每个 world 帧发送的**终结分支枚举**（按分支表优先级、在 Update 后状态上计算）和 **paused 位**（Host `Game::IsPaused` 结果）承接。`client_entity_draw_paused` 保留，向 IsPaused 的原生调用者返回该位。本地菜单阻断如何合并由第 13 步决定。
> - **Pα**：不另设同步位，以已安装的 primary alpha `Game+0x25B28 >0` 判定，同批可与终结分支的 P 各调一次（修改方案 6.2）。
> - **RoomTransition（Q11）**：mode、counter、duration、progress、direction、roomIndex、dimension、animation、Boss 标记按 Host 当前值随 world 安装（修改方案 4.5）。表中 “phase `Game+0x1B858`” 实为 RoomTransition `+0x1C` animation，19 = MINECART。
> - 分支内显示读取的状态（ItemOverlay、Q8 alpha、转场字段）仍按各自需求同步；第 5 节失败方式已并入修改方案第 12 节第 57—59 条。

## 1. 判定口径

对照当前修改方案第 4.1、4.5、4.6、6.4 节以及 `mod/binding/state/run.json` 的 `GameRun.ranges`。该文件的旧“未知／跨域委派”整段范围，只能证明字节定位，**不能当作已有按需采集字段或已经同步**。旧 D0.paused／显示包络会退役，同样不能算新方案已接通。

下表区分三件事：已有明确设计需求、只有旧占位／缺少窄条件需求、应留在 Host 或本地的字段。“明确设计需求”不代表已经生成复制计划、取得正式 Host source 或接通 Client。主报告中 `Pα` 是 primary fade 前段的 P，见分支表第 2 节；不是一个已确定的新协议字段。

## 2. 逐项需求表

| 分支／消费者 | 条件字段／必要语义 | 当前需求与布局源记录 | 本次交接 |
| --- | --- | --- | --- |
| Console 在 U 前阻断 | `Game+0x67FD4` Console 与其 helper 的阻断结果 | `console +0x67FD4/0x108` 明确为“本地”；没有 Host 控制台复制需求 | 保留本地输入／UI owner。若 Host Console 阻断影响权威显示分支，只承接与 world 对齐的暂停／分支语义，不同步命令、文本、历史或 Host 指针；Console state 非零不能直接代替 helper 返回值 |
| pause_countdown | `i32 Game+0x25A6C >0` | `pause_countdown` 已知且归“权威”，旧语义仅由 D0.paused 供绘制；**缺少新方案下区别于其他暂停原因的分支需求** | 第 8 步收口必要的 hold 谓词／所选显示分支；可使用 Host 当前计数，但不要求发送整套生产状态，也不让 Client 减一 |
| GameOver | `u32 Game+0x1D4C8 !=0` | `game_over_owner +0x1D4C8/0x130` 仍为跨域 UNKNOWN；**未明确窄 active 条件** | 第 8 步补显示存在性／分支输入及必要结果；不复制死亡处理／保存／平台对象，不运行 GameOver 分支中的 PauseScreen 混合更新 |
| Leaderboard | `u32 Game+0x1D5FC !=0`，即 owner `+4` 的 `_displayState` | `leaderboard_owner +0x1D5F8/0x3B4` 旧占位；**未明确窄条件** | 第 8 步记录活跃显示状态／分支；账号、平台请求和提交留本机／Host 对应 owner，不复制整个对象 |
| StageTransition | `u32 Game+0x1BA78 !=0` | `stage_transition_owner +0x1BA78/0x20` 旧占位；**未明确首字判定需求** | 第 7／8 步补换层显示存在性／所选分支；只用 `_sameStage` 不足以替代首字条件，不运行 `0x909100` 推进生命周期 |
| RoomTransition；相机 paused 例外 | mode `Game+0x1B83C !=0`；phase `Game+0x1B858 ==19` | 修改方案 4.5、RE-9 已明确原生转场、参数、阶段与等待需求；`room_transition_owner +0x1B83C/0x23C` 仍只是旧占位 | **设计需求已覆盖，布局／来源待收口**。第 7／8／13 步与 RE-9 使用同一转场状态，不能只发一次开始事件后丢失晚加入／等待时的当前 mode、phase 语义 |
| ItemOverlay 与 PauseScreen 组合 | state `Game+0x1C034` 的 0／1／2；暂停菜单是否非零 | 修改方案 6.4 已明确 ItemOverlay 的 state、id、delay、Sprite；`item_overlay_owner +0x1C034/0x1260` 仍为旧占位 | **Overlay 设计需求已覆盖**。第 8 步提供当前状态；第 13 步按本地菜单和权威分支结果判定。不能执行 `ItemOverlay::Update` 取得“更新后的 state”，也不能将 WAITING 当作 ACTIVE |
| GenericPrompt 活跃分支 | `IsActive`：`Game+0x25D24` 非空且 byte `+0x25D34` 非零，或等价的 Host 当前活跃语义 | `generic_prompt_owner +0x25CF0/0x134` 旧语义已提醒“不能只用 paused”，但**未列新窄谓词／source** | 第 8 步补必要活跃语义；第 13 步从当前本地资源 owner 或 Host 窄结果获得。`+0x25D24` 是指针，不原样上网；不调用含输入的 Prompt::Update |
| 未命名 UI 阻断一 | `byte Game+0x24ECC !=0` | 落在 `additional_ui_owners +0x24430/0x1014` 旧 UNKNOWN 占位；**无单列需求** | 第 8 步保留该确切谓词／所选分支，语义名保持 UNKNOWN。不能以“未命名”为由默认为零并运行普通分支，也不因此扩大为整段 UI 同步 |
| primary fade 的前置 Pα | 进入原生生产段前 `float Game+0x25B28 >0` | Q8 已明确 **当前 alpha** 需要同步；旧 `fade_display_owner +0x25B04/0x34` 不是实现。Q8 当前值不等于“Host 此次是否已调 Pα” | 当前 alpha 需求已覆盖；第 13 步明确当前状态的显示资格／P 合并策略，不补演 Host 调用历史。若实现确需准确前置资格，由第 8 步记录窄结果，不为此重新运行 fade 生产段 |
| secondary fade 的终结分支 | 原生处理后 `float Game+0x25B10 >0` | Q8 已明确 Host 当前 alpha；旧 fade 范围仍 UNKNOWN | **当前值设计需求已覆盖**。以已安装的当前状态选择阻断／P，不依据本机墙钟、N、`+0x25F60` 再减 alpha 或决定 ShowName |
| PauseScreen 阻断 | state `Game+0x2363C !=0` 与 `0x98ED50` 结果 | `pause_screen_owner +0x2363C/0xDF4` 明确需区分本地交互与 Host 暂停，但**尚无新方案的窄分支绑定** | Host 显示分支／paused 与本地菜单各归其 owner。C/S 的原版 Online 集合保持为空；不复制 Host 的选中项／设备或调用混合 PauseScreen 更新 |
| 未命名 UI 阻断二 | `u32 Game+0x24444 !=0`，且原版集合为空 | 同属 `additional_ui_owners` 旧占位；**无单列需求** | 第 8 步补当前存在性／所选分支；不调用 `0x8CCCC0`，不猜其语义为 PauseScreen |
| 相机的综合 paused | `Game::IsPaused 0x6F92A0` 的最终权威语义；并保留 RoomTransition phase 19 例外 | 修改方案 4.1／6.2 已要求取同步 Host 暂停；旧 D0 和原生查询绑定存在，**尚未证明新同 world 输入已接通** | 第 8／13 步建立同一已安装 world 的显示 paused 输入。它是相机资格，不能替代以上各显示分支 |
| 本机 Manager 更新／绘制 | M `+0x4A8B8`、I `+0x2A1FB`、A/B `+0x4A8C0/+0x4A8C1`、state `+0x8`、CameraStyle | 修改方案 4.1 与 RE-3 已明确本地时钟／选项；本补遗补足保持例外和 Mpost 绘制条件 | **本地需求**，第 13 步维护；不发送 Host 这些字段，不把它们混入 Run wire |

## 3. 不因分支研究而新增的同步／执行权限

以下生产字段有助于解释原生代码，但不是 Client 重演状态机的理由：

| 字段／路径 | 归属与处理 |
| --- | --- |
| fade speed `Game+0x25B30`、退出 target `+0x25B34` | Host 生产／生命周期；Q8 安装当前 alpha，不让 Client 到阈值后保存、退出、换局 |
| `fade_producer_latch Game+0x25F60` | 当前源已为权威生产锁存；不作 Client 显示开关或生命周期许可 |
| 次级 fade 的墙钟 `timeGetTime()` 与 Manager `+0x4AA04` | 原生开场等待生产；Client 不按本机经过 3000ms 结束 Host 等待 |
| RoomTransition helper 可能清零的 `Game+0x25B38` | 运行辅助计数，不是显示时钟；不为复现此 helper 取得写权限 |
| Prompt 的确认选择与 fade target 7、PauseScreen 自动 Show 条件 | 输入／生命周期；Host 决定，本地菜单另归本地 owner |
| 原版 Online 集合的 begin／end、NetInputDevice | 原版模式状态；保持 C/S 隔离，不用远端人数或 Player 数构造替代集合 |
| Manager.state 的 Host 值 | 不复制它来“修好”本地 IsPaused 或强行进入 Game 绘制；当前 Client 场景／显示分派由第 13 步 owner 接通 |

现有 `client_entity_draw_paused`（RVA `0x2F92A0`，`mod/tools/gen_binding.py`、`mod/binding/hooks.json`）在旧 FrameBorrow 作用域内供应 D0.paused，作用域外透传原生。现有 `authority_paused` 会话输入也未在本次研究中证明是新方案与当前 world 绑定的显示谓词。可以在新 owner 下复用已核入口，但不能把旧借帧机制或只供会话控制的布尔值直接视为新需求已经满足；本次不变更 Hook 退役清单。

## 4. 布局与显示调度的落实位置

第 7／8 步收缩版本 72 布局时，将上表已确认的必要条件纳入需求目录，选择原始窄字段或 Host 已选的显示分支／谓词结果。若采用结果，必须说明采集点、优先级、world 对齐和生命周期，不新增一个与原始状态互相矛盾的 owner。本补遗不自行定义 wire 枚举，不要求同步整个 GameOver／Leaderboard／PauseScreen／Prompt 对象。

第 13 步的顺序依据是：先完成 world／Q8 安装，再按有效分支运行本机 30Hz 显示批次；普通分支才有 M、自有 FullSort、RE-2 的 FX／ANM2；转场分支为 U→T→M→P；StageTransition、hold、active Overlay、secondary fade 有各自的 P 位置；GameOver／Leaderboard／Prompt／菜单阻断不外推普通 M／FX。Pα 的原生事实单独处理，不顺带执行被 Q8 取消的生产段。

判定需要使用 native helper **之后**状态的地方，Client 应读取已安装当前状态或 Host 窄结果，不能调用治疗／换房／输入／保存 helper 求结果。未取得某项必要选择器时，明确保留该功能缺口，不补零或默认 Normal。分支表研究已完成，不代表这些原本就属于第 7／8／13 步的采集与应用工作已经实施。

## 5. 留给第 15 步的具体失败方式

1. 将 GameOver／Leaderboard／StageTransition／Overlay／Prompt 全部压成 paused，错误地运行同一组 U／P／M。
2. WAITING Overlay 与打开暂停菜单组合判断错误，或为了读取下一状态执行被拒绝的 ItemOverlay::Update。
3. secondary fade 归零同帧／跳帧安装后仍阻断，或本机墙钟提前放行；primary fade 的 Pα 被误当 return，或无意多跑 PostUpdate。
4. 旧 D0.paused 或过期会话暂停与新 world 混用；本地 Manager.state 让相机永远暂停；本地菜单覆盖了 Host 的权威结果。
5. 关闭插值时按 Mpre 判 Render、遗漏 A/B 保持例外、M 双增，或为了绘制而补跑玩法／半帧更新。
6. 为补齐 UI 条件复制整段带指针／平台／输入的 owner，或者旧 UNKNOWN 占位被误记为正式同步已完成。

这些是统一验收的交接内容，本次没有执行游戏、构建、测试或验证脚本。
