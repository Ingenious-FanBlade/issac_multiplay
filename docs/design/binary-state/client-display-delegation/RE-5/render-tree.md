# J273 Render 树：覆盖与单一绘制 owner

日期：2026-11-12。证据：`WIKI／SDK／SOURCE` 为本地公开 API／声明／官方源码，`J273-STATIC` 为当前 IDB 的签名及指令，`设计交接` 不等于已实施。VA 基址 `0x400000`，RVA＝VA−基址。任务范围及限制见 [README](README.md)。

## 1. 先查 Wiki 与 SDK

| 来源 | 可直接复用的内容 | 仍需 J273 静态确认 |
| --- | --- | --- |
| 原版 [Game.md](F:/projects_isaac/IsaacDocs/docs/Game.md)、[HUD.md](F:/projects_isaac/IsaacDocs/docs/HUD.md) | `Game:Render()`、`HUD:Render()` 与 HUD 可见性 API | API 存在不能证明原生父子调用位置 |
| REPENTOGON [Minimap.md](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Minimap.md) | 地图状态、HoldTime、震动、图标 Sprite、Refresh；说明它有本地显示状态 | Render 的 ABI、地址及它是否由 HUD 调用 |
| REPENTOGON [ItemOverlay.md](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemOverlay.md) | `Show`、overlay id、delay、Player、主 Sprite、MegaMush Sprite | Render 的调用位置与原生早退条件；不能从 `Show` 外推 Update 可放行 |
| SDK [Game.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Game.zhl)、[HUD.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/HUD.zhl)、[Minimap.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Minimap.zhl)、[ItemOverlay.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ItemOverlay.zhl)、[PauseScreen.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PauseScreen.zhl) | Render 均声明为 `void __thiscall T::Render()`，this 在 ECX，无显式参数；Game 内各对象偏移 | 用完整 SDK 签名与 caller 的 ECX 交叉确认，不能采用坏栈伪代码生成的额外参数 |

本次静态字节检索中，Minimap 完整 ZHL 签名唯一匹配 `0x966680`，ItemOverlay 的 `55 8B EC 83 E4 F8 83 EC 48 56 8B F1` 唯一匹配 `0x984660`，PauseScreen 完整 ZHL 签名唯一匹配 `0x98DDA0`。这些是静态定位，不是运行时调用证据，也没有计算哈希。Game／HUD 的现有 J273 定位与原生 caller／SDK 一致。

## 2. 核定的调用树

| 父入口／对象 | J273 VA callsite → 目标 VA（目标 RVA） | 条件、顺序与结论 |
| --- | --- | --- |
| Game::Render `0x6F7A80`（RVA `0x2F7A80`） | `0x6F7AE1 → Room::PreRender 0x803E20`（`0x403E20`） | 从 `Game+0x18300` 取 Room；先于房间主体／过渡绘制 |
| Game::Render，普通房间 | `0x6F7DF4 → Room::Render 0x804500`（`0x404500`） | `Game+0x1B83C` 的转场状态不为 2／3 时走此分支 |
| Game::Render，过渡 | `0x6F7DFD → RoomTransition::Render 0x8273C0`（`0x4273C0`） | 上述状态为 2 或 3 时替代普通 Room::Render；不再从外层补绘 Room；完整内部树归 RE-9 |
| Game::Render | `0x6F7E02 → LuaEngine::PostRender 0x849DD0`（`0x449DD0`） | 在房间／过渡主体后、HUD 前；按 Q6 保留分派 |
| Game::Render，HUD=`Game+0x1D9AC` | `0x6F8108 → HUD::Render 0x97C300`（`0x57C300`） | 是原生场景树的一部分，不由 MenuManager wrapper 再画 |
| HUD::Render，Minimap=`Game+0x25444` | `0x97C5D8 → Minimap::Render 0x966680`（`0x566680`） | `0x97C483` 检查 HUD `+0x54E4` 可见性；到该调用点前 `0x97C5C9` 比较 `Game+0x25B8C`，有符号 `<2` 才调用。此前的隐藏／特殊分支照原生处理，不为“补全显示”绕过条件 |
| Game::Render，StageTransition=`Game+0x1BA78` | `0x6F8127 → 0x909E60`（`0x509E60`） | HUD 后的换层遮罩绘制；这是按对象与函数读取行为确认的显示 helper，不因 SDK 只有前缀类型而编造完整状态布局 |
| Game::Render，ItemOverlay=`Game+0x1C034` | `0x6F8132 → ItemOverlay::Render 0x984660`（`0x584660`） | 紧随前项，由 Game 直接调用 |
| Game::Render，GameOver=`Game+0x1D4C8` | `0x6F8325 → 0x970C10`（`0x570C10`） | 已在树内；按 SDK 对象位置和 callsite 归属，不据此声称完整 GameOver 更新已分类 |
| Game::Render，Leaderboard=`Game+0x1D5F8` | `0x6F8330 → 0x83EDC0`（`0x43EDC0`） | 已在树内；状态／载荷另按所属任务处理 |
| Game::Render，PauseScreen=`Game+0x2363C` | `0x6F8304 → PauseScreen::Render 0x98DDA0`（`0x58DDA0`） | `0x6F82F5` 检查 state 非零才调用；IDB 的 `___std_tzdb_get_time_zones@0_3` 名称是误识别，应以 SDK 签名及对象归属为准 |

当前静态直接 code xref 中，Minimap::Render 的 caller 是 `0x97C5D8`，ItemOverlay::Render 的 caller 是 `0x6F8132`。这支持当前树内调用结论，不用于宣称不存在所有虚调用／外部调用。

**ItemOverlay 的实际绘制条件**：`0x98466D—0x98467A` 检查 state，0 与 1 均返回；`0x984680` 读取 `+0x111`，即主 Sprite `+8+0x109` 的 loaded。已加载时居中绘制主 Sprite；未加载时只对 overlay id45 走特殊分支，后者还会检查 MegaMush Sprite loaded 与 Player 指针，按 Player 位置和 Room scroll 绘制。因此“树内调用”不等于“每帧可见”，也不能在外层另调用 Show／Update 来补可见性。

## 3. 官方 Render 包装继续存在

官方 [CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp) 中 HUD::Render（约 222 行）、Minimap::Render（约 4660 行）、ItemOverlay::Render（约 6078 行）均包装各自 `super()`；分别含 HUD 1022／1024、地图 1479／1480 和 ItemOverlay 1139 回调。当前 Q6 放行这些分派，不新增回调编号拦截；当前范围没有第三方 Mod，本项目 Lua 不在 Client 注册玩法回调。

[InterpolV2.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/InterpolV2.cpp) 包装 Game::Render 与 LuaEngine::PostRender，管理本地 `_interpol_isgamerender`；[Anm2Extras.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Anm2Extras.cpp) 的 RenderFrameEx 使用原生 Game／Manager 帧、提交图形顶点并恢复 blendMode。实现需保留官方 Hook 链的正常调用关系，不以直接跳进未包装的内部地址替代完整原生 Render。共享动画帧的暂存恢复归[写集说明](write-set.md)。

## 4. 实现交接

- **不另设 Minimap／ItemOverlay 绘制调度点**；已有或后续的 hook 若仅用于观测、事件来源或作用域，不再负责第二次调用原函数。Game::Render wrapper 的 original exactly-once，树内节点遵循原生条件。
- `MenuManager::Render` 只按原生绘制菜单，旧 `RenderPublished／DrawImpl` 退役后，不能再叠加 HUD、地图、ItemOverlay 或手工 Room pass。
- 本结论不取消显示推进：HUD::Update／PostUpdate、Minimap::Update／Refresh、FX／动画与相机仍按 RE-1／RE-2／RE-3 安排；renderEL 仍由自有有界全排维护，它不在 Render 树内。
- ItemOverlay::Update 不放行；StageTransition／GameOver 等“已在 Render 树内”的事实不授权其 Update、换局、保存或生成行为。全屏演出输入与事件归 RE-8，换房根与等待归 RE-9。
- Game::Render 开头本机 `+0x25AAC` 触发的 `0x703E40` 仍在原生链内。已知请求字节是本地状态；完整 helper 条件／写集归 RE-9，不能声称其已成为只读函数。
- 既有窄安装约束继续保留，尤其 Bomb 最终主 Sprite／costume 安装后清 `+0x463` pending，避免 Render 再走资源懒重建并覆盖已安装结果，见[第 13 步](../../13-client-apply.md)。放行原生树不等于恢复已退役的生产器。

尚未静态穷尽其余尾部 helper 与派生 Render，也没有运行时覆盖结果；按现行收窄任务交第 15 步 E28 与整体显示验收。
