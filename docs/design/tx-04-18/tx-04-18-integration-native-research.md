# tx-04-18 I19—I25：Grid 引用、缓存、绘制与集成静态研究

日期：2026-11-02。目标二进制：Isaac/REPENTOGON v1.9.7.12.J273，IDA image base `0x400000`。VA/RVA 均列出；`RVA = VA - 0x400000`。

## 范围与证据顺序

先复读 [项目规范](../../refactor/project-conventions.md)、[I19—I25计划](tx-04-18-implementation-plan.md)与 [I13—I18 Grid 逆向笔记](tx-04-18-grid-native-research.md)，再查本地[原版 GridEntity](F:/projects_isaac/IsaacDocs/docs/GridEntity.md)、[GridEntityDesc](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md)、[Room](F:/projects_isaac/IsaacDocs/docs/Room.md) 和[REPENTOGON GridEntity](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md)、[Door](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md)、[Statue](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityStatue.md)、[Room](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md)；其后对照 REPENTOGON 源码、libzhl 与当前本地 adapter，最后用 IDA MCP 只读核定 J273。没有运行测试、probe、游戏或构建，没有修改 IDB；本次只新增本文。

下文把“可直接采用”限于原生最终值展示所需的对象／成员／资源／绘制路径。跳过玩法维护后不据此声称本地 Grid 可承担权威碰撞、AI寻路、更新或转场。

当前工程对照位置：[Room path 与 GridLight 值](../../../mod/src/game/client_room_native.h#L18)、[Source GridLight 查询采集](../../../mod/src/game/client_room_native.cpp#L86)、[窄 Grid draw 与 Pit surface pass](../../../mod/src/game/client_grid_native.cpp#L353)、[绘制 pass / Scope 声明](../../../mod/src/game/client_grid_native.h#L79)、[五个查询的唯一 Hook 清单](../../../mod/binding/hooks.json#L75)。这些链接用于说明本报告对应的 adapter 边界；pass 顺序和副作用仍以本报告所列 J273 原生 callsite 为准。

## I19：搬起描述由 Effect 136 持有

- 原版枚举 `EffectVariant 136` 是 `GRID_ENTITY_PROJECTILE_HELPER`。REPENTOGON 的 [LuaEntityEffect.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/Entities/LuaEntityEffect.cpp#L52) 仅在 `entity->_variant == 136` 时把 `&entity->_varData` 直接以 `GridEntityDesc*` 压入 Lua；这是指针别名，不是新建／拷贝的描述。因此描述的 owner 是 Effect 136 实体，生命周期与该 Effect 一致；采集时应复制八字段值，客户端不得同时把原 Grid 当成仍活动的成员。
- REPENTOGON [EntityEffect wiki](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md) 将此入口暴露为 `GetGridEntityDesc()`；没有公开的 `source_grid` 身份反向映射。若权威目标未提供原格 Key，保持未解析，不用 seed 或位置猜回去。
- `Room::PickupGridEntity(index)` ABI 在 libzhl `Room.zhl`，目标 VA `0x81AE00` / RVA `0x41AE00`。描述／Sprite 低层 overload 位于 VA `0x81A920` / RVA `0x41A920`。反编译的低层移除流程会复制描述和 Sprite、清格引用、写 path trail、释放 Grid，并对 Rock／Poop 等走专用清理，部分路径会触发邻接或装饰维护；它是玩法拾取流程，不是本地展示投影的构造 API。REP wiki 的 `PickupGridEntity` 也明确是“拾起并返回 Effect”。不要在 Client 投影中调用它来“生成”搬起物。
- REP 源码中可证明的是 Effect 136 的描述 accessor 和 Room 的拾取入口；没有找到独立于拾取／Effect owner 的第二个描述消费者或可将描述重新挂回 Grid 的接口。描述值本身可应用，不代表已经证明 source-grid 关系或完整拾取玩法语义。

## I20：原始 Path 足够承载查询；邻接逻辑不要重跑

### Path 与碰撞查询

| 入口 | J273 地址 | 读写语义 |
| --- | --- | --- |
| `Room::GetGridPath`（IDA `sub_4523F0`） | VA `0x4523F0` / RVA `0x523F0` | `index > 0x1BF` 返回 0；否则直接读 Room `+0x76C + index*4`。不做编码转换。 |
| `Room::SetGridPath`（IDA `sub_425EC0`） | VA `0x425EC0` / RVA `0x25EC0` | `index > 0x1BF` 返回 false；否则把传入整数原样写回同一数组。 |
| `Room::GetGridCollision`（`Room.zhl` 原型） | VA `0x7E6AF0` / RVA `0x3E6AF0` | 见下列优先级；函数自身未做 448 上界检查。调用前必须使用当前 Room 有效 extent 限制 index。 |

`_gridPaths` 的公开 SDK 布局为 `Room+0x76C`，数组声明长度 456；公开 Path API 只接受 `0..447`。Source 应按正式 Room extent 复制该数组中的原始整数，不复制“Getter变换结果”（Getter 没有变换），也不要读取／解释 `448..455` 尾部。Room extent 最大 448；不允许以整个 456 槽数组构造格成员。

`GetGridCollision` 先处理一项特殊 Room 条件：`Room+4` 指针链所指结构存在，且该结构 `p[2]==16 && p[0]==35` 时直接返回 0；这两个原生结构字段在本函数上下文中的语义未完全命名。其余情况：负 index 返回 4；若 `Room+0x24+index*4` 上有 Grid，返回该对象 `+0x3C` CollisionClass；无 Grid 时按 Path 值返回——`1000..2999 => 3`，`3000..3999 => 1`，`>=4000 => 3`，其余 `0`。因此当前实现可安装 Source 原始 Path 和每个 Grid 最终 CollisionClass；不存在第三份独立 Room collision 数组需要重算。

### Rock、Pit、Wall 的邻居结果

- Rock `UpdateNeighbors` VA `0x715700` / RVA `0x315700`，不要与 `SpawnDrops` 混淆（后者是 `0x715B70`）。它由 Rock 邻接维护路径遍历周边 8 格，通过 RTTI 找 Rock，并调用邻接帧更新；算法会改邻居／自身的 Rock 帧或动画状态。Rock State 2 的 `Render` VA `0x7146E0` 还会进入 `0x404480`，重置/重播 ANM2 状态并读取 Rubble vector。Client 不调用 `UpdateNeighbors`；安装 Source 最终 ANM2 姿态、big-rock frame 与 Source 的 Rubble 位置即可。岩石投影不执行本地掉落或玩法 RNG。
- Pit `PostInit` VA `0x70ED00` 先把 Room `+0x11F6` 置为 Pit surface dirty，再扫描邻近格；对周围 Pit 调 `0x70E8D0` 计算边缘／邻居动画帧，最后路径按状态进入 `0x70EFF0` 安装 Room Path 与 Grid CollisionClass。这个维护同时混合显示派生帧与寻路／碰撞写入，不能在 Client 通过 `PostInit` 或 `Update` 补算。以 Source 最终 Sprite/layer pose 和原始 Path/CollisionClass 安装；特殊 Backdrop 32 的纹理 surface 单独按 I21 处理。
- Wall `PostInit` VA `0x71B870` 在 RoomType 16、BackdropId 56 分支读取上下格类型（包括类型1/19），选择 `Brick`、`LowBrick` 或 `MiddleBrick`，通过 `sub_407A70` 取得动画资源并改 ANM 帧。Wall `Render` VA `0x71B4E0` 本身只对 RoomType 16 临时将 Room floor color 写入主 ANM2 `_color`，再走 Sprite draw；不重新扫描邻居。因此 Source 已捕获的最终 Sprite pose 可代替 Wall 邻接派生，不调用 PostInit。
- 对这些类别，当前 Room grid 数组和 Source 最终姿态／缓存已经承载显示结果；没有证据要求在 Client 再调用 Rock/Pit/Wall 邻接算法。需要保留的例外是 Pit 的 Room-owned Textured Pit surface，它是另一层预渲染缓存，不是单个 Pit 的邻接数组。

## I21：原生 draw ABI、排序与副作用

### 调用 ABI 与统一入口

- `GridEntity` vtable 的 slot 3 `Render(Vector& offset)`，x86 `__thiscall`：`ECX=GridEntity*`，栈上一个 `Vector*`；slot 8 `BeginBatches()`、slot 9 `EndBatches()`，均 `ECX=GridEntity*`、无显式参数。slot 6 是 `GetWaterClipInfo(WaterClipInfo*)`。原型/字段见 [GridEntity.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GridEntity.zhl#L30)；Room 对应原型见 [Room.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl#L53)。
- `Room::Render` VA `0x804500` / RVA `0x404500` 是原生 Grid 分类与深度排序 owner；`Room::PreRender` VA `0x803E20` / RVA `0x403E20` 单独运行 Light／预绘制路径。直接投影应复用这些窄 Grid pass，不重入完整 `Room::Render`、`Room::PreRender`、`Room::Update`。
- `Room::RenderGridLight(GridEntity*, Vector&)` VA `0x801BE0` / RVA `0x401BE0`，libzhl 是 `__thiscall`；`Room::PreRender` 的逐 Grid 调用点为 `0x80422E` / RVA `0x40422E`，故 Light 是主画面 Room::Render 前的独立 pass。已审分支处理 Pit(7)、Door(16)、TrapDoor(17)、PressurePlate(20)、Teleporter(23)；没有 Statue(21) 分支。
- Door slot 3 是 `0x707F90`；Pit `0x70DE30`；Wall `0x71B4E0`；Rock `0x7146E0`；通用 Lock/Teleporter `0x70CB80`；Gravity/TrapDoor/Stairs/Spikes/TNT `0x719270`。Statue slot 3 指向 VA `0x4291D0` 的空实现。全 Room::Render 中没有另一个 Statue 专用 slot/Room 绘制入口；即使 RoomType 16 专用分支也按 grid vtable slot 3 调用。I21 不应虚构 Sprite 或把 Source Statue 状态误报成可见画面。
- Rock `RenderTop` VA `0x7149E0` / RVA `0x3149E0`，ABI `ECX=GridEntity_Rock*`、栈 `Vector*`。Room 只在 Pillar type24 的逐行深度分支调用它，不应用普通 slot3 冒充柱顶。

### Room::Render 实际 Grid 顺序

以下序列由 `0x804500` 的分支和直接 vtable callsite 还原。Begin/Render/End 是真实分组顺序，不可把 Door 提前到 Under，也不可把普通 Grid 全压成一个 slot3 loop。

| 阶段 | J273 调用点 | 选择与顺序 |
| --- | --- | --- |
| RoomType 16 特殊路径 | grid Begin `0x804F60`，Render `0x804F6C`，End `0x804FBD`；随后共用实体 z5000 call `0x80513E` | 跳过普通类别拆分，按完整 grid 数组遍历 Grid。此处是 `RoomType` 判断，不是 BackdropId 判断。 |
| Decoration | Begin/Render/End：`0x8050AA / 0x8050B6 / 0x80510E` | type 1；其后实体深度 `5000`，call `0x80513E`。 |
| 轨道／Under | `sub_9B7C70` call `0x80529D`；Under Begin/Render/End：`0x805303 / 0x805311 / 0x805382` | Under = type 7 Pit、17 TrapDoor、18 Stairs，另含 type14 Poop 且 `State==1000`。轨道 helper 是独立 Room pass，不能由普通 Grid 分类取代。 |
| Backdrop 32 坑纹理重绘（条件分支） | `sub_81C000` call `0x8053CD`；Pit slot3 `0x805417`；`sub_9B7F00` call `0x80546F`；实体 z 10000 call `0x80548B` | 条件为 `Room+0x1D18 == 32`（这是 BackdropId）且 `RoomDescriptor.Flags & 0x20`。Pit 重绘前把 Pit 主 Sprite Layer0 的 `_visible` 临时写 false，渲染后原生写 true；复用时必须恢复原布尔值。此条件不以 RoomType 范围筛除，BackdropId 32 与 `RoomType` 无关。 |
| Door | Begin/Render/End：`0x8058B2 / 0x8058C0 / 0x805913` | 主 grid 数组中 type16。位置在 Under／坑轨道与 z10000 实体之后，在 shading/shadow 合成之前；不是 Decoration 后的第一个 pass。 |
| Main grid | Begin `0x805EF2`、Render `0x805EFE`、End `0x805F9C`；循环内 WaterClip 重检 call `0x805F88` | 排除 Decoration、Door、Under。处于水面时检查每对象 slot6 的 `WaterClipInfo.bitFlags & 0x02`，置位对象本轮跳过。它位于 Door 后及 shading (`BE7370`)／shadow (`BE7360`) 合成之后。 |
| Pillar row/depth | row Begin `0x8060E0`、`RenderTop(0x7149E0)` `0x8060F0`、row End `0x806163`；实体深度 `0x80613A` | type24 Pillar 按 `index / gridWidth` 分行；每行在对应实体阈值 `24030 + row*4000` 后绘制柱顶。行结束后另绘深度阈值再加5000的实体；最后实体 layer `-1` call `0x8061A3`。普通底层已由 Main pass 绘制。 |

Door 与 Main 的分界也决定 WaterClip 过滤范围：原生在 Main loop 过滤，不把同样过滤擅自扩到 Door/Under/Decoration。各专用 Room surface pass（如 Pit rails、Shading、Shadow）仍由 Room/Presentation owner 排程；Grid pass 本身不负责创建这些 renderer surface。

### Textured Pit surface 的生产与消费

- 全局 `dword_BE73B8` 是资源名为 `Textured Pit Surface` 的 renderer Surface。`sub_7E08E0` VA `0x7E08E0` / RVA `0x3E08E0` 惰性构造该 Surface（宽780、高 `0x1D4`，通过 `sub_9EF4F0` 与 `sub_408110` 建立 owner），并同次加载 `gfx/grid/grid_pit_mask.png`（`BE7368`）、lava `BE73A8`、cold lava `BE7318` 等相邻 renderer 资源；全局初始化/重置在 `sub_703E40` VA `0x703E40` / RVA `0x303E40`，不是 Room-owned。
- `Room::PreRender` 末尾在 Room `+0x11F6` dirty byte 为1时清零并调 `sub_81BB30`（callsite `0x8044D6/0x8044E1`）。`sub_81BB30` VA `0x81BB30` / RVA `0x41BB30` 在同一 `BackdropId==32 && descriptor.Flags&0x20` 条件下把 BE73B8 设为渲染目标，扫当前 Room 格子，只对 type7 Pit 调 `sub_70DFC0` 画 pit mask，之后结束图像 batch 并叠加 lava/cold overlay。Pit 构造／PostInit／析构等会把 dirty byte 设为1。Client 必须先有完整 Grid/Pit 成员和上述 renderer resources，才能生产正确 surface；只创建空 BE73B8 再调用 81C000 会读到空/旧缓存。
- `sub_81C000` VA `0x81C000` / RVA `0x41C000` 为普通 `__thiscall(Room*, Vector*)`、`ret 4`：ECX=Room、栈=offset Vector*；ESI 在该函数 prologue 被保存，不是参数。它从 Room `+0x6AE0..+0x6AEC` 与 offset 组四角，调用 BE73B8 vtable slot3 获取纹理/quad，成功后用 renderer state 与 `sub_67E7B0` 合成。它会改全局 renderer/shader state，不是纯计算 helper。适合只在条件成立时作为 Room-render 窄 pass；Surface 的建立仍由全局 renderer owner 管理，内容由上述 `PreRender→81BB30` 生产。

### Render 会改姿态；不是纯函数

- 已审 Grid slot3 路径未发现调用 `ANM2::Load` 或 `ANM2::Update`；但 Door、Rock、Pit、Wall、Gravity 等路径会临时写 ANM2 pose、颜色或 layer visibility。Rock State2 的 `0x404480` 会 Rewind/重置 selector 并强制选动画；不可只保存几个字段。
- 用完整 draw scope 保存并恢复 Grid 主 Sprite、Door ExtraSprite、Fire/Pit/Room 专用 Sprite 及其 LayerState、scale/color/blend/AnimationState 和 selector array；对原 selector/index 数组做深拷贝且用相同游戏 allocator/free。水面内部 Grid 绘制也必须在这个 scope 内。不得在 draw 中调用 Sprite Update、Grid Update 或资源 Load。
- Wall 与 Gravity/TrapDoor/Stairs/Spikes/TNT 在 Render 中临时以 `Room+0x72A4` floor color 覆盖 Sprite `_color`（Grid+`0xE8`）。Pit `0x70DE30` 自己临时关闭 Layer1 并恢复；Room 专用 Pit 重绘关闭 Layer0 后并不保留其原始 bool，需外层恢复。`RenderGridLight` 也改 Room-owned Gradient/SpotLight（`Room+0x6E40`、`+0x6F54`）的 scale/color/layerblend；一并放入 draw guard。
- `GridEntity::BeginBatches`/`EndBatches` 和调用的 Render 仍改变全局 renderer/batch 状态。要按每组配对并在失败时把场景标成不确定／隔离；不能提前确认 NativeCommitted。水面 `PresentWithoutSwap`／offscreen Surface 顺序属于原生渲染上下文 owner，不得从每个 Grid pass 重置。

## GridLight 必须带来的只读 Source 查询

`Room::RenderGridLight` 的关键分支会调用下列 Game/Seed 条件。当前实现若不从 Source 提供而用 Client 本地 stage/player 推测，会显示错误的坑、门或环境光。只在 Client Render → GridLight 窄调用栈返回复制值，其余原生调用必须原样透传。

| 序号 | J273函数 / VA(RVA) | Source 对应值与结论 |
| --- | --- | --- |
| 0 | `Seeds::HasSeedEffect` `0x452580` (`0x52580`) | `Game::_seedEffects`（SDK `Game+0x1BB84`）对效果79调用公开 `HasSeedEffect(79)`。Hook 校验 receiver 与 effect id。 |
| 1 | `Game::GetStageID(bool unused)` `0x732CC0` (`0x332CC0`) | 可用公开 `Game:GetStageID(false)`；[Game.zhl:141](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Game.zhl#L141) 是官方静态绑定。函数还处理 challenge／维度场景；不要直接复制基础 `_stage` 来代替此 Getter。 |
| 2 | 私有 route query `0x748900` (`0x348900`) | 不等于公开 `Level::IsAltPath`；[Level.zhl:50](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Level.zhl#L50) 的 pattern 在 J273 唯一匹配 VA `0x745AF0`，该函数只检查 stageType 4/5。`0x748900` 另检查 Stage、mode/curse 与 GameStateFlag 46。没有对应公开 Getter，采集 Source 最终 bool；按 `CsGame::GridLightRouteCondition` 专用 Hook 捕获。 |
| 3 | 私有 boss-room query `0x815B80` (`0x415B80`) | 比较当前 RoomDescriptor.ListIndex（descriptor `+8`）与 `Game._lastBossRoomListIdx`。没有已核定公开 getter；采集 Source 最终 bool，按 `CsStatic::GridLightCurrentBoss` 捕获，不用 Client Player 猜测。 |
| 4 | 私有 stage suffix `0x745B10` (`0x345B10`) | 由 Stage/StageType、curse 与 GameStateFlag 派生整数；没有已核定公开 getter，采集 Source 最终 int，按 `CsGame::GridLightStageSuffix` 捕获。 |

Door 光照分支另读 Door `+0x28C & 0x20`（REP SDK 没有给该隐藏字段命名）和 Sprite 隐藏状态；因此需将这个 raw flag 纳入正式 Grid Door snapshot，不能只同步 Door ExtraSprite。房间 `_lightGradient` / SpotLight 仍由 Room display owner 持有，完整 ANM2 draw guard 保护其暂态修改。

## I25：最小可维护接线与明确边界

1. 构造 Grid 后只按正式目标安装 Desc、Grid+`0x3C` CollisionClass、最终 Sprite/派生缓存、Grid slot 关系与 Door alias；保证目标 owner/资源尚有效。不调用 Grid `Init`、`PostInit`、`UpdateNeighbors`、`UpdateCollision`、Door 解锁/开门、Pit/Wall 邻接派生或 Room.Update 来补缓存。
2. Room 应用 owner 按有效 extent 安装 Source 原始 Path 数组（最大448）；碰撞由 Grid CollisionClass 和原始 Path 查询规则组合，不额外算一份 collision graph。Rock/Pit/Wall 已由 Source 最终 Sprite 与必要专用字段承载邻接展示；特殊坑 surface 按 `PreRender→81BB30` 生产。
3. 在统一原生 draw scope 内以 Room Render 的类别、顺序和 depth 参数运行窄 pass；light 输入由上述5项 Source query Hook 供应，Door +`0x28C` 来自 Door 快照。水面仅在原 Main loop 重用 slot6 的 bit2 过滤。Statue 保持原生 null slot 行为，不伪造图像。
4. 每次 Room/Grid plan 检查目标 revision、同代 Room/资源 owner、成员数组与门槽别名；写后中止则保留不确定 owner并隔离旧/新混合画面，撤销借用与关系后按 owner 退休。`Room::RemoveGridEntityImmediate` VA `0x813BD0` / RVA `0x413BD0` 是玩法移除路径（PathTrail、实体释放等），不能拿它代替投影 owner 的析构。
5. Effect136 的 descriptor 以 Effect owner 保存；缺失 source-grid identity 或资源／引用时报告 Unresolved/失败，不合成占位 Grid，也不让 Grid 仍作为当前成员。

当前静态证据的硬边界：没有通过实机证明跳过 Init/PostInit 的本地 Grid 可安全参与玩法；没有证明 448..455 Path 尾槽用途；没有暴露的搬起来源 GridKey；RoomType 16、特殊 backdrop 与水面／镜像／遮罩的像素正确性仍归 I29 整体 E2E 验收。本文只闭合维护／ABI/排序与接线的静态依据，不代表这些运行结果已验证。


## 主智能体审阅补记

主智能体已另取707F90、7146E0、70DE30、7149E0、801BE0、804500的实际分支及81BB30/70DFC0/81C000/7E08E0原始指令/伪码复读，并核五个query及游戏allocator。纠正早期材料的Door顺序、Backdrop与RoomType混用、81C000保存ESI误认参数、Under的Poop条件为State（对象+0xc），以及748900与SDK IsAltPath不同址。代码采用的原生pass/借用/退场及未验收边界见[场景集成](tx-04-18-scene-integration.md)。
