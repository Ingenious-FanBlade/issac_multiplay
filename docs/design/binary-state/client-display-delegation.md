# 客机显示委托与按需采集：修改方案

状态：修改方案，仅文档，未实施。上级：[总计划](README.md)。日期：2026-11-12。

**用户决定（2026-11-12）**：

- 借鉴 isaac-lan 的做法：Host 只采集客机需要的状态，显示交给客机本地完成。
- 帧号采用方案B：每装好一个 world 帧，就把 Client 的原生帧号对齐到 Host 帧号，见第 3 节。
- 项目继续以官方 REPENTOGON 1.1.3／v1.9.7.12.J273 为目标。isaac-lan 只支持 J460，不能与 REPENTOGON 共存；它的仓库没有 LICENSE，所以只借鉴思路，不拷代码，其 J460 偏移也不能用于 J273。

**本文性质**：

- 只写修改方案，没有修改代码，也没有运行任何测试。
- 第 13 节的 Q1—Q4 尚待用户确认，正文先按其中的推荐写成。用户改选时，再修订对应章节。
- 用户确认后，按第 11 节修订项目规范第 17 节和各步文档。修订完成前，冲突处以用户 2026-11-12 的决定为准，本文是这项决定的展开。
- 历史事实、失败记录和 UNKNOWN 全部照实保留。旧方案中被取代的部分只加标注，不删除。

## 1. 背景

### 1.1 现状

**Host**：

- 按字节归属取全量：每个类型从 0 到对象大小的每个字节都要有归属，未知字节必须列出。
- 另外同步一批显示结果：第 8 步的 HUD 生产、场景光照、地图、绘制上下文 D0（布局 356）和随机 alpha（357／358），以及第 9 步的显示包络（414—417）、文字（422—424）和资源 recipe（425—433）。
- 当前布局源版本是 71。第 10 步第五项“Run 与显示／音频生产缓存”仍在实施中。

**Client**：

- 拦截全部玩法和展示 Update，也把 `Game::Render`、`Room::PreRender` 和 `Room::Render` 当作拒绝入口（`client.cpp:142` 的 `kExpectedNestedMask`，以及 `kClientEntryNames`）。
- 画面在 `MenuManager::Render` 边界内绘制，调用链是 `ClientMenuRenderBoundary`（`client.cpp:817-831`）→ `RenderPublished`（`client_scene.cpp:760`）→ `DrawImpl`（`client_presentation_native.cpp`）。
- 这条链手工拼合各个 pass，绘制参数和帧号都从 Host 借：`FrameBorrow`、`FXDrawContext`、手工调用 `RenderShadows`／`RenderEntityLight`、FX `0x56db20`、`ClientRoomWaterPreDraw`。

**已知问题**：

- tx-04-21 的 Host 低帧率仍未解决：PlayerFast 全部白读，45ms 的来源还没定位，见第 73 轮诊断。
- 第 8 步之后的显示结果布局体量最大：`presentation.json` 有 77 个类型，约 596KB。
- 完整性按字节判定，未知字节会一直挡住完成。

### 1.2 isaac-lan 的做法（仅作参照）

- **采集**：
  - 字段用手写白名单：公共 11 项，每个类型再加 3—15 项，外加房间、门、地图和音乐。
  - 每个 tick 全量读取白名单。三客机同房约 1.33ms／视图，分房约 2.55ms／视图，含编码。
- **Client**：
  - 用 MinHook 挂各实体类 vtable+0xC 的 Update，阻止玩法推进；Player 单独处理。
  - 本地运行原生渲染，在 scope 里补轮廓、透明度和战斗反馈。
- **不能本地算出的值**：恶魔房／天使房几率、商店价格和药丸识别都作为状态同步，在 Client 侧由 getter 注入。
- **瞬时事件**：音效、音乐请求、Boss 开场和 HUD 文字走有界事件队列。

### 1.3 结论

我们的复杂度和开销主要来自两点：“字节完整”和“同步显示结果”。本方案去掉这两点，保留下面这些已有的内容：

- 二进制布局格式与整数布局版本；
- C++ 复制计划执行器；
- memcmp 增量检测、world／body 序列与 Baseline＝版本 0；
- zstd 压缩、事务与 ACK；
- tx-04-17 建立的原生 Client 世界。

## 2. 三条原则

### 2.1 按需采集

字段满足下列任一条件，才进入正式同步：

1. **原生显示要读，Client 又算不出**：Client 的原生显示更新或 Render 会读取这个字段，而 Client 无法从其他已同步状态算出它。
2. **原生对象结构需要**：Client 构建和维护原生对象时需要它，例如身份、type／variant／subtype、父子与持有关系、容器计数和资源引用。
3. **玩家能看到的规则结果**：例如价格、恶魔房／天使房几率、药丸名称。这类值按 2.3 注入。
4. **事件**：音效、音乐、文字和 Boss 开场这类瞬时事件，按 2.3 走事件队列。

以下几类不再同步：

- Host 专属的玩法内部状态：AI 计数器、冷却、随机种子、未显示的物理中间量等。
- Client 原生 Render 会自己重算的渲染缓存，例如 Entity `+0x1A4/+0x1A8` 屏幕坐标缓存。
- 已交给 Client 本地生成的显示结果，例如 HUD 排版、光照、水面、地图绘制和绘制随机量。

**完整性改判（Q1）**：

- 判据从“字节归属完整”改为“需求清单完整”。每个类型列出 Client 需要的字段，每一项写明消费方：哪个原生显示函数读取它、哪个 getter 返回它，或哪个结构 owner 需要它。
- 已有的字节归属账本、成员映射和同步审查文档继续保留，用来定位字段和说明语义，不再作为完成条件。
- UNKNOWN 字节默认不采集，也不再阻塞完成。如果第 15 步出现画面或规则差异，再按差异补入需求清单。
- 需求清单从现有各类型同步审查（`*-synchronization-review.md`）的分类改判得出，不从头重做。

### 2.2 显示委托

- Client 包装原生 `Game::Render`，由原生代码完成整套绘制，见第 4 节。
- Client 本地运行经核实的纯显示更新：HUD、PlayerHUD、Minimap、ItemOverlay、Camera，以及房间的显示部分。
- Host 不再发送显示结果。验收只要求双方看到同一个权威结果，不要求像素相同（Q4）。

### 2.3 不可算的值与瞬时事件

**注入（Q3）**：

- 优先用原生 setter 或 REPENTOGON API 写入 Client 原生字段，让原生 getter 和显示代码自然读到。
- 只有字段没有 API 可写时，才直接写原始内存。
- 只有 Client 上没有对应存储，或者值依赖 Host 专属状态时（例如几率依赖 Host 的整局计数），才挂 getter 并返回已同步的值。

**事件队列**：

- 每个 Peer 一条有界串行队列，按序号排序，带 Host game_frame，随 world 帧发送并确认。
- 条数和字节数都有上限。溢出时先丢最旧的纯外观事件，并计数报告。
- Client 在帧号到达事件帧时播放。落后超过限定帧数的纯外观事件直接丢弃。
- 事件只驱动本地播放和显示，不能修改已同步状态。

### 2.4 不变的约束

- Host 是唯一权威。Client 不运行玩法 Update、玩法随机、玩法 Lua，也不产生持久副作用。
- C/S 与原版 Online、Isaac Sync 保持隔离；线程与生命周期约束不变；Hook 常驻，按逻辑启停。
- 性能与带宽指标不放宽：
  - Host 帧率 ≥50 FPS；
  - 主线程采集 p95 ≤2 ms、p99 ≤4 ms；
  - 模组持续带宽 9Mbps；
  - 首次准备 ≤1 MiB／10 s；
  - Client 安装 p95 ≤2 ms、p99 ≤4 ms。
- 不设 gate，不校验哈希。0—14 期间不运行测试，第 15 步是唯一的统一整体验收。
- 代码为功能、核心函数和数据结构写中文注释。

## 3. 帧号方案B（已定）

**定义**：

- Client 每装好一个 world 帧，就把帧头的 game_frame 写入自己的 `Game._frameCount`（`Game+0x25A70`），并保持到下一个 world 帧。
- 写入与 world 安装在同一个安装点完成，Render 不再改写帧号。
- Client 的原生 getter、显示更新和 Render 都直接读到 Host 帧号。

**它取代的内容**：

- 第 13 步 13.6 的独立展示时钟。
- 每个绘制栈的借用与恢复（`client_presentation_native.cpp:826-829`、`client_entity_draw_native.cpp:238-241` 及同文件 136-145 行）。
- 现在“展示时钟”就是 `Game+0x25A70` 本身，不再另设变量。

**帧锚点**：

- Entity `+0x328`、Room `+0x11F0` 直接安装 Host 原值，不做换算，也不逐次重新锚定。
- FrameCount 和 `Room:GetFrameCount()` 由原生 getter 用“帧号 − 锚点”自然得出。

**Effect Timeout**：只有需求清单保留 Timeout 时才适用以下规则。

- 线格式仍然传到期帧（`effect_timeout`），这样块保持静止，节省带宽。
- 安装时写入 `到期帧 − Game._frameCount`，与帧号对齐在同一个安装点完成。
- 第 4 步其他按展示时钟还原的子类计数照此改写，例如 Familiar FireCooldown 和 Pickup DropDelay。

**Client 自身的帧递增**：

- 现在 Client 整个拦截 `Game::Update`，不会递增帧号。
- 按第 4 节改造后，如果原生帧递增仍在放行范围内，就必须拦截这个递增，或者在 Update 返回后、Render 前写回 Host 帧号。
- `Room+0x11F0` 同样处理。两处递增点的位置见 RE-3。

**`Manager._framecount`（`+0x4A8B8`）**：不再借用，保留 Client 本地值，供本地显示动画使用。D0 随之取消，见第 8 节。

**停顿**：

- 没有新的 world 帧时，帧号不动，与 Host 保持一致。
- Host 暂停或网络停顿期间，以 Game 帧号驱动的显示动画会停住；以 Manager 帧或本地显示推进驱动的动画继续。

**空帧与跳帧**：

- Host 静止时仍然发送帧（第 11 步 11.3），帧号随空帧前进。
- world 约 30Hz，帧号通常一次前进 1，丢帧或合帧时会前进 2 或更多。
- 显示代码里按“帧号相等”或“差 1”做判断的地方，可能漏触发或重复触发，见 RE-4。

## 4. Client 更新与渲染边界

### 4.1 入口的三种判定

| 判定 | 做法 | 适用 |
| --- | --- | --- |
| 放行 | 调用原函数 | 已证明是纯显示 |
| 拦截 | 不调用原函数 | 纯玩法，或尚未分类 |
| 拆分 | 调用原函数，但拦截其中的玩法子调用 | 显示工作内联在玩法函数中，无法单独调用 |

- **默认拒绝**：未分类的入口一律拦截，并像现在一样记为 RejectedEntry。分类证据来自 RE-1、RE-2 和 RE-5。
- **复用现有分派**：继续用 `ClientEntryDecision`（CallOriginal／ExpectedSkip／RejectedEntry）、`CountDispatch` 和 `kExpectedNestedMask`（`client.cpp`）。新增的放行和拆分入口都登记为具名入口。
- **`Game::Update` 的推荐做法**：
  - 继续拦截原函数，改由 Client 自有的“显示推进”根按 RE-1 白名单逐个调用显示子更新。这样帧号不会被本地递增，也与默认拒绝一致。
  - 只有某项显示工作内联在玩法函数里、无法单独调用时，才对那个函数改用拆分。

### 4.2 包装原生 `Game::Render`

`game_render`（`Game::Render`，RVA `0x2F7A80`）从拒绝入口改为包装放行，分三段：

1. **前置**：
   - 确认本帧的 world 安装事务已经完成或没有开始，不在半套状态上绘制。
   - 确认 `Game+0x25A70` 等于最近一次已应用的 game_frame。
   - 只在确有需要时设置作用域，例如本地相机跟随对象。
2. **调用原函数**：由原生代码绘制房间、背景、按 `render_sort` 排序的实体、阴影、光照、HUD、Minimap、ItemOverlay 和暂停菜单。`Room::PreRender`（`0x403E20`）、`Room::Render`（`0x404500`）和 `EntityList::render_sort`（`0x150C0`）随之放行，不再单独作为拒绝入口。
3. **后置**：恢复前置设置的作用域，并记录 Render 改写过的已同步字段，交给漂移校正（13.4）。

随之变化的部分：

- `MenuManager::Render` 边界（`ClientMenuRenderBoundary`）不再负责场景绘制，只按原生方式绘制菜单。
- `RenderPublished` 与 `DrawImpl` 退役，见第 9 节。

### 4.3 候选判定表（待 RE 核实）

| 入口 | 现 hook／RVA | 现状 | 新判定（候选） | 待核 |
| --- | --- | --- | --- | --- |
| `Game::Update` | `client_game_update` `0x2F6D30` | 预期跳过 | 拦截，改走“显示推进”根 | RE-1、RE-3 |
| PostUpdate | `client_post_update` | 预期跳过 | 拦截 | — |
| `Room::Update` | `client_room_update` | 预期跳过 | 拦截，显示部分移入“显示推进”；不能单独调用时改拆分 | RE-2 |
| `EntityList::Update` | `client_entity_list` | 预期跳过 | 拦截 | — |
| `Player::Update` | `client_player_update` | 预期跳过 | 拦截 | — |
| OddHelper | `client_odd_helper` | 预期跳过 | 拦截 | — |
| 门过渡 | `client_door_update`／`client_door_nested` | 预期跳过 | 拦截（沿用直接切房） | RE-2 |
| Backdrop 效果 | `client_backdrop_effects` | 预期跳过 | 若是纯显示，移入“显示推进” | RE-2 |
| `Room::Init` | `room_init` `0x3E8AD0` | 受控 | 拆分：放行背景、光照和水面初始化，拦截生成 | RE-2 |
| `Game::Render` | `game_render` `0x2F7A80` | 拒绝 | 包装放行 | RE-5、RE-6 |
| `Room::PreRender`／`Room::Render`／`render_sort` | `room_prerender`／`room_render`／`entitylist_render_sort` | 拒绝 | 随 `Game::Render` 放行 | RE-5 |
| `MenuManager::Render` | `client_menu_render` `0x5623C0` | 场景绘制边界 | 只放行原生菜单 | — |
| `HUD::Update`、`PlayerHUD::Update`、`Minimap::Update`、`ItemOverlay::Update`、`Camera::UpdateDrag`／`UpdateUltrasmooth` | 无（REPENTOGON zhl 已声明） | 不运行 | “显示推进”白名单候选 | RE-1 |
| `LuaEngine::PostRender` | `lua_post_render` | 拦截 | 待定 | RE-5 |

### 4.4 Render 的副作用

原生 Render 并不纯，已知和可能的副作用有：

- base draw `0x6AC4E0` 写 Entity `+0x1A4/+0x1A8`（tx-04-17 R3）；
- 绘制随机量，即原来的 random alpha；
- 可能播放音效；
- 可能触发 REPENTOGON 的 Lua 渲染回调；
- 可能分配和释放资源。

处理规则：

- 只改显示缓存的写入，允许发生。
- 改到已同步字段的写入，进入漂移校正，或在后置阶段覆盖回去。
- Client 上的随机消耗不会回传 Host（Client 只上行输入），所以允许发生，只接受外观差异（Q4）。
- 持久副作用继续由现有的保存、PGD 和成就 hook 阻断。
- 完整清单见 RE-6。

## 5. 仍需同步的显示相关状态（候选）

下表是需求清单的起点，最终以 RE 结果和第 2.1 节的判据定稿。

| 域 | 保留候选 | 安装方式 |
| --- | --- | --- |
| Entity 公共 | 身份、type／variant／subtype；Position、Velocity、PositionOffset、SpriteOffset、SpriteScale、SpriteRotation；Color；Visible 及影响绘制的标志；DepthOffset；父子关系；HP／MaxHP（Boss 血条）；`+0x328` 锚点；主 Sprite 的 ANM2 状态（文件、动画、帧、覆盖层、LayerState） | 原生 owner／setter |
| Entity 子类 | 各类型可见字段：Tear 高度、Laser 端点与采样、Knife、Bomb 外观标志、Pickup 价格／充能／状态、Familiar、Effect 各变体的可见状态、NPC 冠军色、Slot 显示 Sprite | 原生 owner |
| Player | costume 层、身体与头部 Sprite、颜色；HUD 读取的库存、主动道具与充能、口袋、饰品、心、显示用属性、持物；Jacob／Esau 等多身体 | 原生 owner；HUD 由原生代码读取 |
| Grid | type／variant／State、Sprite 帧、门状态、便便与石头状态 | 原生 owner |
| Room／Level | 房间描述（DisplayFlags、VisitedCount、Clear、类型、形状、图标）、诅咒、stage／stageType、背景类型；由 Host 随机选出的装饰结果，同步选中结果，不同步 RNG | 原生 owner；地图由原生 Minimap 绘制 |
| Run | 硬币、炸弹、钥匙；挑战编号、种子效果、难度、计时；贪婪波次 | 原生 setter |
| 注入值 | 恶魔房／天使房几率、药丸识别与名称、商店价格（若不在 Pickup 字段中） | 2.3 注入 |
| 事件 | 音效、音乐请求、Boss 开场、道具名与描述横幅、fortune 文字 | 2.3 事件队列 |

## 6. 新增逆向任务（J273 静态反编译）

| 编号 | 内容 | 产出 |
| --- | --- | --- |
| RE-1 | `Game::Update`（`0x2F6D30`）子调用清单，逐项标为玩法／显示／混合；重点看 HUD、Minimap、ItemOverlay、Camera、屏幕震动、淡入淡出、PostUpdate Lua 和 Level／Room 更新 | “显示推进”白名单与调用顺序 |
| RE-2 | `Room::Update` 与 `Room::Init`（`0x3E8AD0`）子调用分类：EntityList、Grid、清房与奖励、相机、背景、水面、光照、生成 | 拦截／拆分清单 |
| RE-3 | `Game+0x25A70` 和 `Room+0x11F0` 的递增点，以及显示依赖的其他逐帧计数 | 帧号对齐的写回位置 |
| RE-4 | 显示代码中按帧号相等或差值做判断的地方（闪烁、每帧一次的保护等），以及它们在跳帧和停顿下的行为 | 风险清单与处理方式 |
| RE-5 | `Game::Render`（`0x2F7A80`）调用树：子调用、REPENTOGON 渲染回调、音效播放、资源分配 | 放行范围与 Lua 回调的处理 |
| RE-6 | Render 期间的写集与随机消耗，并与 13.4 已知清单对照 | 漂移校正范围 |
| RE-7 | 注入点：几率、价格、药丸识别等值对应的原生字段、getter 和 REPENTOGON API | 注入方式表 |
| RE-8 | 事件源：SFX 播放、音乐、Boss 开场、道具横幅和 fortune 的 Host 采集点与 Client 播放点 | 事件种类与载荷 |

- 先复用 [docs/hooks/README.md](../../hooks/README.md) 和已有证据文档中的点位，再补齐缺口。
- REPENTOGON zhl 已声明 `HUD::Update`、`PlayerHUD::Update`、`Minimap::Update`、`ItemOverlay::Update`、`Camera::UpdateDrag` 和 `Camera::UpdateUltrasmooth`；其 J273 签名在 RE-1 中核对。

## 7. 各步修改

不另开编号。修改落在原 0—14 步内，第 15 步仍是唯一的统一整体验收。

| 步 | 修改 |
| --- | --- |
| 0 | 增加 2026-11-12 起点说明；改动前先为当前未提交的工作做一次快照提交（第 14 节） |
| 1 | 布局源格式保留，每个字段增加“需求”属性（Client 需要／Host 专属／Client 本地显示）。生成器只为“Client 需要”的字段生成复制计划和编解码表，字节归属表照常生成，作为参照。完整性报告改为需求清单覆盖 |
| 2 | 保留 Vector、ColorMod、ANM2、LayerState 等基础类型。RNG 不再作为同步状态，只同步由它选出的结果 |
| 3、4 | Entity 公共字段和子类按第 5 节重标；Effect 各变体（E32）收窄到可见状态；Render 会重算的缓存一律标为本地 |
| 5 | Player 重标；PlayerPost 身体约 60Hz 的窗口保留，只承载位置和 Sprite 等需要的字段 |
| 6 | Grid 重标 |
| 7 | Room／Level 保留描述和需要的房间状态；[房间显示资源设计](room-display-resource-design.md)中由原生 `Room::Init` 本地产生的部分改为本地 |
| 8 | 改动最大，见第 8 节 |
| 9 | 帧头、记录、块、zstd 和 ACK 保留；显示包络 414—417 退役；文字 422—424 并入事件队列；资源协议 425—433 收窄为实体、Grid 和 costume 等资源引用；新增事件队列段 |
| 10 | 第五项“Run 与显示／音频生产缓存”（版本71）暂停，Run 只保留需要的窄投影；采集窗口和执行器保留，复制计划随布局源收缩；收缩后重新评估 tx-04-21 的瓶颈 |
| 11 | 机制保留；去掉显示结果批次；增加事件队列的序号与确认 |
| 12 | 机制保留；影子状态随域收缩；增加事件队列的接收 |
| 13 | 13.4 的前提改为第 4 节的边界；13.6 由第 3 节取代；新增注入（2.3）；13.5 补入第 9 节的退役项 |
| 14 | 退役清单补入第 9 节 |
| 15 | 见第 10 节 |

## 8. 布局源逐文件修改

布局源在 `mod/binding/state/`，类型数为当前值。第一批收缩入源时，整数布局版本从 71 升到 72，不计算任何哈希。

| 文件 | 类型数／大小 | 修改 |
| --- | --- | --- |
| `base.json` | 16／133KB | 保留；RNG 相关类型移出同步 |
| `entity.json` | 2／103KB | 按需求清单重标 |
| `entity-classes.json` | 21／347KB | 按需求清单重标 |
| `entity-effect.json` | 6／85KB | 收窄到可见状态 |
| `entity-text-dummy.json` | 2／7KB | 保留（E2E 夹具） |
| `player.json` | 48／1775KB | 重标；玩法内部容器移出同步，HUD 读取的库存类字段保留 |
| `grid.json`、`grid-classes.json` | 5／97KB、27／289KB | 重标；水面裁剪等本地显示项移出 |
| `room.json` | 54／587KB | 保留描述与房间状态；光照、水面和显示资源投影移出 |
| `level.json` | 18／350KB | 保留房间描述与探索状态；地图绘制结果移出 |
| `run.json` | 9／293KB | 只保留第 5 节的 Run 项与注入值 |
| `presentation.json` | 77／596KB | 显示结果 310—407、D0 356 退役；保留项并入事件或注入 |
| `hud-local.json` | 2／16KB | 版本70的纯值模块，凡是原生 HUD 能产生的部分退役；只保留原生 HUD 做不到的项（Q2 之后再评估） |
| `random.json` | 24／81KB | 随机 alpha 357／358 及绘制随机退役；只保留 Host 选出的结果，并入相应域 |
| `wire.json` | 23／148KB | 删去显示包络；新增事件队列段；收窄资源协议 |
| `schema.json`、`version.json` | — | 增加“需求”属性；升到版本72 |

## 9. Hook 与代码退役

### 9.1 Hook

**改判**：`game_render`、`room_prerender`、`room_render` 和 `entitylist_render_sort` 从拒绝入口改为随 `Game::Render` 放行；`client_menu_render` 不再承担场景绘制。

**复审，默认删除**。这些 hook 原本用来把 Host 提供的绘制输入塞给 Client。改为委托后，原生绘制直接读已安装的状态，只保留防副作用或注入确实需要的：

- `client_entity_draw_*`（12 个，不含下面单列的 `client_entity_draw_paused`）；
- `client_entity_shadow_*`（5 个）；
- `client_entity_post_*`（2 个）；
- `client_entity_draw_paused`；
- `client_entity_owned_final`；
- `client_player_palette`；
- `client_grid_water_*`（6 个）；
- `client_grid_light_*`（5 个）；
- `client_backdrop_wall`；
- `client_backdrop_graphics`。

**保留**：

- 所有玩法拦截：`client_game_update`、`client_post_update`、`client_room_update`、`client_entity_list`、`client_player_update`、`client_odd_helper`、`client_door_*`、`client_room_lifecycle` 和 `client_new_room`。
- 所有保存、PGD、成就和 Lua 生命周期的隔离 hook。放行的原生代码变多，这些 hook 更不能少。
- `client_hud_achievement_query`，改作注入用。

**诊断 hook**：`client_player_render_body_diagnostic`、`client_player_render_head_diagnostic` 和 `client_minimap_render_diagnostic` 只留在 E2E 构建，或者删除。

**新增**：

- “显示推进”根，挂在 `Game::Update` 拦截点内；
- 帧号写回点（RE-3）；
- 注入 getter（RE-7）；
- 事件采集与播放点（RE-8）。

最终清单写入 `mod/binding/hooks.json`，hook 常驻，按逻辑启停。

### 9.2 代码退役候选

- **Client 绘制**：
  - `mod/src/game/client/presentation/*`，其中包括 `DrawImpl`、`FrameBorrow`、`FXDrawContext`、手工阴影／光照调用、FX `0x56db20` 和 `ClientRoomWaterPreDraw`；
  - `mod/src/game/client/resources/client_resource_shaders_native.cpp`；
  - `client_scene.cpp` 中的 `RenderPublished`；
  - `client.cpp` 中 `ClientMenuRenderBoundary` 的场景绘制部分；
  - draw 期间逐实体重新锚定 `+0x328`（`client_entity_draw_native.cpp`），以及展示前重新锚定 `Room+0x11F0`（`client_room_native.cpp`）。
- **Host 显示生产**：`mod/src/world/binary/` 下的 `presentation_source`、`hud_production`、`hud_probability`、`scene_production`、`scene_lighting`、`scene_map`、`scene_source` 和 `run_source` 中的显示部分。
- **状态库**：`mod/src/state/` 下的 `presentation_envelope`、`production_cache`、`hud_local`（按 Q2 评估）和 `resource_protocol` 中的 recipe／shader 部分。
- **随机观测**：`mod/src/game/observation/sources/random_*`、`global_random.*` 中只为绘制随机服务的部分。

退役前逐项确认没有其他调用方。删除只在第 14 步进行。

## 10. 第 15 步验收修改

| 条目 | 修改 |
| --- | --- |
| E09 | 对照范围改为需求清单中的字段；不再逐字节比较 |
| E20 | 改为双方看到同一个权威结果；画面由 Client 原生绘制，不要求像素相同 |
| E27 | 未知区探测改为“差异驱动”：只对第 15 步发现画面或规则差异的区域补查，不再要求所有未知区间都有结论 |
| E28 | 前提改为第 4 节的边界：漂移来自放行的显示更新、Render 及维护 helper，按 RE-6 的写集检测和纠正 |
| E30 | 资源表收窄为实体、Grid 和 costume 资源引用；文字改由事件队列验证 |
| E32 | 只核对 Effect 各变体的可见状态 |
| E33 | 改为方案B：静止场景中 Client 的 `Game+0x25A70` 随帧头前进；空帧被接受、确认并推进帧号；不再构造“屏蔽 D0”的空帧 |
| E34 | 改为方案B：锚点直接安装 Host 原值，FrameCount 和 `Room:GetFrameCount()` 与 Host 一致；Effect Timeout 等计数在安装点按 `Game+0x25A70` 还原；加测跳帧与停顿 |
| E35 | 删除（D0 和随机 alpha 退役） |
| 新增 E36 | 显示边界隔离：放行的显示更新和 Render 不产生玩法写入、玩法随机推进、保存或解锁；默认拒绝的入口计数为 0 或已解释 |
| 新增 E37 | 事件队列：顺序、去重、有界、溢出计数、过期丢弃、重连后不重放旧事件 |
| 新增 E38 | 注入值：几率、价格、药丸名称等与 Host 一致，并在变化后收敛 |

- 其余条目按原文执行，其中显示相关的内容按“同一权威结果”理解。
- 第 15 步交接清单（15.5）中，按第 8 节退役的布局不再列入。

## 11. 文档后续修订清单（用户确认后进行）

1. **`AGENTS.md`**：一般规则不变，只需加一行指向本文。
2. **[项目规范第 17 节](../../refactor/project-conventions.md)**：
   - 修订“采集方式”：允许按需字段选择；正式路径仍是 C++ 复制计划，不调用 Lua getter，也不构建 JSON。
   - 修订“完整性”，改为需求清单。
   - 修订“资源与显示”和“HUD本地化”，改为原生显示委托。
   - 增加帧号方案B。
3. **[总计划](README.md)**：修订第 1 节用户决定、第 3 节完整性、第 4 节数据流、第 6 节取代与保留、第 7 节步骤表和第 9 节未决问题。
4. **步骤文档**：第 0、1、8、9、10、11、12、13、14、15 步加日期标注并按第 7 节修订。重点是 13.4、13.6，以及 15.2／15.3／15.5。
5. **被取代文档加标注**（保留原文和事实）：
   - [presentation-result-batches.md](presentation-result-batches.md)
   - [local-media-presentation.md](local-media-presentation.md)
   - [hud-local-presentation.md](hud-local-presentation.md)、[hud-local-implementation.md](hud-local-implementation.md)
   - [implementation-8-presentation.md](implementation-8-presentation.md)
   - [10-run-presentation-capture.md](10-run-presentation-capture.md)
   - 第 8 步的显示与随机审查（`presentation-*`、`random-*`）
6. **[docs/README.md](../../README.md)**：加日期条目。

## 12. 失败方式

按规范，必须隔离验证的系统先写出全部失败方式，再写代码。

**显示边界**：

1. 放行的显示更新或 Render 写入已同步字段，造成振荡，或者漂移检测漏掉。
2. 放行的函数间接调用了玩法子函数，例如生成、清房奖励、随机推进或 Lua 玩法回调。
3. Render 中触发的音效与事件队列重复播放。
4. 显示更新依赖 `Game::Update` 前面某一步建立的状态，单独调用时读到未初始化的值。
5. 拆分入口中被拦截的子调用返回值被后续显示逻辑使用，导致显示错误。

**帧号**：

6. 原生帧递增没有被拦截或写回，Client 帧号领先于 Host。
7. 安装事务失败或部分写入时帧号已经前进，锚点与帧号不配套。
8. 跳帧导致“每帧一次”的显示逻辑漏触发或重复触发；停顿导致动画冻结超出预期。
9. 换房、换层或重建（base=0）时，帧号回退或不连续，触发原生异常路径。

**按需采集**：

10. 漏采某个原生显示必读的字段，Client 读到旧值或构造默认值。
11. UNKNOWN 字节实际影响显示，却因不再阻塞而遗漏。
12. Host 选出的随机结果（如背景装饰）没有同步，Client 本地再随机一次，画面与 Host 不同且会跳变。

**注入与事件**：

13. 注入的 getter 与原生字段同时存在，两处值不一致。
14. 事件队列溢出、乱序、重复，或重连后重放旧事件。
15. 事件引用的实体已被移除，或资源还未就绪。

**隔离与性能**：

16. 放行更多原生代码后，保存、PGD 或成就副作用绕开了现有 hook。
17. 原生 Render 加上显示推进后超出 Client 帧预算，或者 Host 收缩采集后仍达不到 p95 ≤2 ms。

## 13. 未决问题

正文按下列推荐写成，等待用户确认。

| 编号 | 问题 | 推荐 |
| --- | --- | --- |
| Q1 | 完整性判据 | 改为“Client 需要的字段”需求清单；UNKNOWN 不再阻塞，差异驱动补查 |
| Q2 | 采集实现 | 保留 C++ 复制计划执行器和二进制格式，只收缩计划；不改回 Lua 白名单。`hud-local` 纯值模块在原生 HUD 接通后评估去留 |
| Q3 | 写入方式 | 优先原生 setter 或 REPENTOGON API；没有 API 时才直接写内存；没有存储时才挂 getter |
| Q4 | 外观随机差异 | 接受。绘制随机、粒子等纯外观差异不同步；Host 选出、影响布局或规则的结果仍然同步 |

另一个可选项：是否为“Client 边界改造”单设一次验收点。现行规范只允许第 15 步统一验收，推荐不单设。如果用户希望提前确认原生 Render 能在 Client 上安全运行，需要用户明确调整规范。

## 14. 推进顺序

1. **快照**：当前分支 `codex/step4-text-dummy` 有大量未提交改动（版本66—71 的实施与文档）。建议先由用户决定是否提交一次快照，再动手修改。按规范，只在用户要求时提交。
2. **文档修订**：按第 11 节修订。
3. **逆向**：RE-1—RE-8。
4. **布局源**：重标并收缩布局源，升到版本72，生成器只为“Client 需要”的字段出计划。
5. **Client 边界**：实现第 4 节的显示推进、`Game::Render` 包装、帧号方案B 和注入。
6. **Host 采集**：收缩采集，接入事件队列。
7. **退役**：按第 9 节退役，并做产品与 E2E 构建的纯编译。
8. **验收**：第 15 步统一整体验收。

0—14 期间只做资料分析、静态反编译、设计、实现和纯编译，不运行游戏、E2E 或校验脚本。
