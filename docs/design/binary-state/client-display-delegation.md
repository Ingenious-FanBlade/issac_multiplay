# 客机显示委托与按需采集：修改方案

状态：修改方案已确认，规范与各步文档已修订；代码未实施。上级：[总计划](README.md)。日期：2026-11-12。

**用户决定（2026-11-12）**：

- 借鉴 isaac-lan 的做法：Host 只采集客机需要的状态，显示交给客机本地完成。
- 帧号采用方案B：每装好一个 world 帧，就把 Client 的原生帧号对齐到 Host 帧号，见第 3 节。
- 内联显示标量（2026-11-12 追加，Q8）：原生 `Game::Update`／`Room::Update` 内联推进的显示标量（震屏、颜色、闪电、暗度、眩晕、fade、水色、冲击波等）同步 Host 当前值，Client 不本地推进，见 4.6。
- 项目继续以官方 REPENTOGON 1.1.3／v1.9.7.12.J273 为目标。isaac-lan 只支持 J460，不能与 REPENTOGON 共存；它的仓库没有 LICENSE，所以只借鉴思路，不拷代码，其 J460 偏移也不能用于 J273。

**本文性质**：

- 只写修改方案并修订文档，没有修改代码，也没有运行任何测试。
- 2026-11-12，用户确认第 13 节的 Q1—Q4 全部按推荐执行。
- 改动前的快照已提交为 `3977b46e`。
- 第 11 节所列的规范第 17 节、`AGENTS.md`、总计划和第 0—15 步文档已于同日修订，被取代的文档已加标注。
- 历史事实、失败记录和 UNKNOWN 全部照实保留。旧方案中被取代的部分只加标注，不删除。
- 2026-11-12 RE-1、RE-2 静态成果完成后，按审阅意见修订 2.2、4.1—4.3、4.5、5、6、6.1、6.2、7—10、12—14 节，新增 4.6（Q8）。仍未修改代码，未运行测试。
- 同日 RE-3—RE-5 审阅后，用户决定同步 RoomDescriptor SpawnSeed、保留 `client_entity_draw_projectile`；并补入关闭插值时的相机节奏、RE-4 的验收覆盖（E40）、Q8 换算值的安装开销，以及 `0x703E40` 的归属。
- 同日 RE-1 补遗完成：补齐提前返回分支、条件需求交接及关闭插值的 Update→Render 节拍；仍仅修改文档，未实施／未测试。
- 同日 RE-7—RE-9 静态成果完成：补齐注入 ABI／限制、音频文字与演出来源、转场阶段和具名显示拆分交接；采集／布局／原生 adapter 尚未实施，未测试。
- 同日审阅 RE-1 补遗与 RE-7—RE-9（IDA J273 复核）后，用户决定：Q11 转场按 Host 当前值同步 RoomTransition 显示字段，取代 RE-9 的自有 FSM 显示驱动（4.5）；Q12 保留 `client_entity_draw_paused`，Host 每个 world 帧发送 paused 位和终结分支枚举（6.2）；新开 RE-10 分类 Manager::Update 尾部非 Game 调用（至少音乐／SFX 逐帧 tick）。同时按审阅建议写入：`Game+0x22E90` 即 `HUD._isVisible`，按 Q8 同步当前值；诅咒同步 raw 字段 `Level+0xC` 与 `Game+0x25AC8`，不再注入有效值；回放 SFX 时 FrameDelay 置 0；Pα 由已安装的 primary alpha 判定。仍仅修改文档，未实施，未测试。

## 1. 背景

**2026-11-12 [RE-5／RE-6 静态成果](client-display-delegation/RE-5/README.md)**：按收窄范围完成，确认 Minimap／ItemOverlay 在原生 Render 树内，定稿已知本地写集与漂移范围，并纠正旧 Tear 偏移误记；仅补布局源说明／证据，不改变 wire、版本或运行代码，未测试。

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
4. **事件与表现状态**：音效、音乐、文字和 Boss 开场等。能用状态表达的（音乐请求、循环音效、全屏演出）按状态同步，其余走事件队列，见 2.3 和 6.4。

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
- Client 本地运行经核实的纯显示更新：HUD、PlayerHUD、Minimap、Camera，以及房间的显示部分，见 6.2。`ItemOverlay::Update` 含玩法，不运行；全屏演出按状态同步，由原生 Render 绘制。
- `Game::Update`／`Room::Update` 内联推进的显示标量不在 Client 推进，同步 Host 当前值（Q8，4.6）。
- Host 不再发送显示结果。验收只要求双方看到同一个权威结果，不要求像素相同（Q4）。
- **切房走原生过渡**（2026-11-12 追加决定，Q5）：Client 用 Host 已决定的参数回放原生转场，见 4.5。它取代 2026-11-11“目标就绪后直接切房”的做法。
- **放行回调分派**（2026-11-12 追加决定，Q6）：
  - Client 放行的路径上，Lua 回调分派一律不拦截，范围包括 Render、HUD、显示推进和原生过渡。
  - 仍拦截的玩法路径不会运行，其中的回调自然不触发。
  - 前提是当前范围不含第三方 Mod，本项目自己的 Lua 不在 Client 注册玩法回调。第三方 Mod 按 [Mod 适配方案](../mod-adaptation.md)另行处理。

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

**事件与状态分开（2026-11-12，借鉴 isaac-lan）**：能用状态表达的内容按状态同步，不做成事件。状态丢包后由下一帧自然纠正，比事件更稳。

- **按状态同步**：
  - 房间音乐请求：模式、曲目 id、参数和排队曲目。
  - 正在播放的循环音效集合：Client 按集合启停。
  - 各控制器的全屏演出（ItemOverlay）：状态、延时和 Sprite。
- **按事件同步**：
  - 非循环音效；
  - 道具横幅；
  - fortune 文字；
  - Boss 开场；
  - 切房转场参数（4.5）。
- **文字带本地化键**：道具横幅和 fortune 同时发送原文和字符串表的键或索引，由 Client 按本机语言解析。直接发送 Host 已翻译的文字，在其他语言的 Client 上会显示不全。

**音效去重（2026-11-12 已定，Q7）**：

- Client 放行的原生显示代码（显示推进、原生 Render、原生转场）自己发出的音效，在 Client 本地播放，不进事件队列。例如 HUD 滑入音效 252。
- Host 采集音效事件时，排除同一批显示代码发出的音效，避免 Client 重复播放。
- Host 只发送玩法路径产生的音效，例如实体 Update、伤害、拾取和道具使用。
- 判断依据是调用来源，即 SFX 播放时所在的原生路径；采集点和来源判定见 6.4（RE-8）。

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
- 第 4 步其他仍采用帧锚点的子类计数照此改写；Familiar FireCooldown 和 Pickup DropDelay 已按 2026-11-11 用户决定改为原始 i32 采样，不再外推，见 [RE-3 字段合同核对](client-display-delegation/RE-3/display-counters.md#3-已有子类计时合同不能混用)。
- **安装开销（RE-3 审阅后补充）**：帧头每前进一次，所有按 F 换算的值（Effect Timeout、TimeCounter 等）都要重写，实体块没变也一样。这部分开销随实体数线性增长，计入第 13 步 13.3 热路径和 Client 安装预算（p95 ≤2 ms）。实现时只遍历保留了换算字段的对象，不扫全部实体。

**Client 自身的帧递增**：

- 现在 Client 整个拦截 `Game::Update`，不会执行其普通递增：`0x6F789A` 加一、`0x6F789D` 写回 `Game+0x25A70`。按具名显示根继续排除此段，无需额外字节补丁。
- 若后续放行路径仍会改写帧号，必须隔离该写入，或在该更新返回后、其他显示消费者／Render 前写回同一目标的 Host 帧号；写回不能撤销玩法／生命周期副作用。
- **RE-3 已核修正**：`Room+0x11F0` 是 Init `0x7E8CE8` 写入的入房帧基准，没有本次假定的逐帧自增。安装 Host 原始基准，隔离完整 Init 重写并退役旧重新锚定。详见[原生写入证据](client-display-delegation/RE-3/native-clocks.md)。

**`Manager._framecount`（`+0x4A8B8`）**：不再借用，保留 Client 本地值，供本地显示动画使用。D0 随之取消，见第 8 节。

**停顿**：

- 没有新的 world 帧时，帧号不动，与 Host 保持一致。
- Host 暂停或网络停顿期间，以 Game 帧号驱动的显示动画会停住；以 Manager 帧或本地显示推进驱动的动画继续。

**空帧与跳帧**：

- Host 静止时仍然发送帧（第 11 步 11.3），帧号随空帧前进。
- world 约 30Hz，帧号通常一次前进 1，丢帧或合帧时会前进 2 或更多。
- [RE-4 已核](client-display-delegation/RE-4/README.md)：相位直接按目标帧求值；周期HUD刷新识别跨30帧边界；本地显示按M与用途帧去重；平均位置缓存按实际world／roster维护。不能按ΔF补跑更新，也不能把所有相等判断改成不等判断。

## 4. Client 更新与渲染边界

### 4.1 入口的三种判定

| 判定 | 做法 | 适用 |
| --- | --- | --- |
| 放行 | 调用原函数 | 已证明是纯显示 |
| 拦截 | 不调用原函数 | 纯玩法，或尚未分类 |
| 拆分 | 调用原函数，但拦截其中的玩法子调用 | 显示工作内联在玩法函数中，无法单独调用 |

- **默认拒绝**：未分类的入口一律拦截，并像现在一样记为 RejectedEntry。分类证据来自 RE-1、RE-2 和 RE-5。
- **复用现有分派**：继续用 `ClientEntryDecision`（CallOriginal／ExpectedSkip／RejectedEntry）、`CountDispatch` 和 `kExpectedNestedMask`（`client.cpp`）。新增的放行和拆分入口都登记为具名入口。
- **`Game::Update` 与显示推进根**（2026-11-12 按 RE-1 修订）：
  - Client 忙时，`ClientManagerBoundary`（`mod/src/game/client/session/client.cpp:575`）整体跳过原生 Manager 更新，`Game::Update` 根本不会被调用。`client_game_update` 继续作为拦截守卫保留。
  - 相机在原生 Manager 更新的尾部，不在 `Game::Update` 里（RE-1）。所以“显示推进”根不挂在 `Game::Update` 拦截点，改挂在 Manager 边界内、world 安装（`scene.Pump`）之后，按两档调度，顺序见 6.2：
    - **30Hz 批次**：本机 Manager 帧为偶数时运行，对应原生 `Game::Update` 的节奏。不等 world 帧到达；没有新 world 帧时 GameFrame 不变（第 3 节）。
    - **相机阶段**：每个 Manager 帧有调度机会，对应原生 Manager 尾部。本机关闭插值（`OptionsConfig._enableInterpolation`，`Manager+0x2A1FB`）且更新前 M 为奇数时先快返，不跑相机；通常在 `0x92E441` 写 M+1，但 `Manager+0x4A8C0!=0 && +0x4A8C1==0` 时不递增。Client 使用本机设置和同一控制条件。**RE-1 补遗已核**：主循环随后仍调用 Manager::Render；绘制体按更新后 M 判 `I || (Mpost&1)`，普通快返自增至偶数后跳过绘制，保持奇数的例外仍可绘制。详见[绘制节拍与真值表](client-display-delegation/RE-1/manager-render-cadence.md)，不以 Host 帧或更新前 M 代替绘制判断。
  - **RE-3 已核**：Manager 原链被跳过后，当前 Client 没有递增本机 `Manager+0x4A8B8`。显示推进 owner 须明确推进这个原生本地字段并保持相机／Render 相位顺序，不借用 Host 值，也不另设独立展示时钟，见[接线交接](client-display-delegation/RE-3/apply-contract.md#3-manager-的本地推进责任)。
  - 显示推进根按 RE-1／RE-2 白名单直接调用具名入口，帧号不会被本地递增，也与默认拒绝一致。
  - **RE-10（2026-11-12 新开，待做）**：Manager 原链整体跳过时，Manager::Update 尾部的非 Game 调用（约 50 个 call site）也一并不运行。审阅已确认至少两项须在显示推进根中运行：音乐逐帧 tick `0x7D7DA0`（唯一调用者 `0x92EB49`）和 `0x908340`（`0x92EB3E` 调用，疑为 SFX tick）。其余调用由 RE-10 逐项分类为放行／拦截／本地，分类前不新增放行。
  - 只有某项显示工作内联在玩法函数里、无法单独调用时，才对那个函数改用拆分。`Game::Update`／`Room::Update` 的内联显示标量不拆分，按 Q8 同步当前值（4.6）。

### 4.2 包装原生 `Game::Render`

`game_render`（`Game::Render`，RVA `0x2F7A80`）从拒绝入口改为包装放行，分三段：

1. **前置**：
   - 确认本帧的 world 安装事务已经完成或没有开始，不在半套状态上绘制。
   - 确认 `Game+0x25A70` 等于最近一次已应用的 game_frame。
   - 确认当前 RoomDescriptor 的 SpawnSeed（`+0x5C`）已按 Host 当前值安装且非零。原生 Render 在房间年龄为 30 时读取它，为 0 时记录 “RNG Seed is zero” 并执行 `int3`（[RE-4 帧号消费者 2.2](client-display-delegation/RE-4/frame-consumers.md#22-roomage30历史-surface-捕获)）。不满足时不进入原函数。
   - 只在确有需要时设置作用域，例如本地相机跟随对象。
2. **调用原函数**：由原生代码绘制房间、背景、按 renderEL 顺序排列的实体、阴影、光照、HUD、Minimap、ItemOverlay 和暂停菜单。`Room::PreRender`（`0x403E20`）和 `Room::Render`（`0x404500`）随之放行，不再单独作为拒绝入口。
   - `EntityList::render_sort`（`0x150C0`）不在 Render 树内（RE-1），原生 Render 直接读已排好的 renderEL。排序由显示推进负责，Client 用自有的有界全排，不调用原函数，见 6.2。
3. **后置**：恢复前置设置的作用域，并记录 Render 改写过的已同步字段，交给漂移校正（13.4）。

随之变化的部分：

- `MenuManager::Render` 边界（`ClientMenuRenderBoundary`）不再负责场景绘制，只按原生方式绘制菜单。
- `RenderPublished` 与 `DrawImpl` 退役，见第 9 节。

### 4.3 候选判定表（待 RE 核实）

| 入口 | 现 hook／RVA | 现状 | 新判定（候选） | 待核 |
| --- | --- | --- | --- | --- |
| `Game::Update` | `client_game_update` `0x2F6D30` | 预期跳过 | 拦截。Client 忙时 Manager 原链整体跳过，本函数不会被调用；显示推进根挂在 Manager 边界（4.1） | [RE-1 已核](client-display-delegation/RE-1/update-order.md)、RE-3 |
| PostUpdate | `client_post_update` | 预期跳过 | 拦截 | — |
| `Room::Update` | `client_room_update` | 预期跳过 | 拦截完整根；FXLayers、背景动画、水色／波纹等显示职责单独安排 | [RE-2 已核](client-display-delegation/RE-2/room-update.md) |
| `EntityList::Update` | `client_entity_list` | 预期跳过 | 拦截 | — |
| `Player::Update` | `client_player_update` | 预期跳过 | 拦截 | — |
| OddHelper | `client_odd_helper` | 预期跳过 | 拦截 | — |
| 门过渡 `RoomTransition::Update` | `client_door_update`／`client_door_nested` | 预期跳过 | 完整拦截，不拆分（Q11）：转场显示字段同步 Host 当前值，Client 只做旧／新 surface 捕获、滚动补偿、versus 动画和原生 Render（4.5）。~~拆分：在显示推进的同一顺序位置运行具名转场显示段~~（RE-9 建议，已被 Q11 取代） | [RE-9 已核](client-display-delegation/RE-9/README.md)、Q11 |
| `RoomTransition::ChangeRoom` | 新增 | — | 拦截原函数，只作守卫：Client 不运行转场 FSM，新房间随 world 帧原子安装（4.5 Q11） | RE-9 |
| Backdrop 装饰生成 | `client_backdrop_effects` `0x515890` | 预期跳过 | 继续拦截：它生成并更新真实 Entity，不是 FX 动画更新 | [RE-2 已核](client-display-delegation/RE-2/room-update.md#3-两个背景路径必须区分) |
| FXLayers 显示更新 | 新增具名入口 `0x56BF40` | 不运行 | 资源／实际 roster 等依赖就绪后纳入“显示推进”，常规参数 false | [RE-2 已核](client-display-delegation/RE-2/room-update.md#2-fxlayers-的显示更新) |
| `Room::Init` | `room_init` `0x3E8AD0` | 受控 | 拦截完整根，NativeBuild／NativeApply 复用已核显示资源子段；只拦 Spawn 不足以隔离副作用 | [RE-2 已核](client-display-delegation/RE-2/room-init.md) |
| Backdrop 构图 | `client_backdrop_graphics` `0x517910`／`client_backdrop_wall` `0x511310` | 受控 | 冷构建时在具名作用域内运行；三类装饰生成点继续由 `client_backdrop_effects` 和两处 byte patch 隔离（9.1） | [RE-2 已核](client-display-delegation/RE-2/room-init.md#2-backdrop-的窄初始化与混合构图) |
| Room 内 ANM2 显示动画 | 无 | 不运行 | Backdrop floor 两路 AnimationState、地狱横移背景 Sprite 在 30Hz 批次中由自有具名函数推进；横移位移按 Q8 同步（4.6） | [RE-2 已核](client-display-delegation/RE-2/room-update.md#4-水面波纹与颜色) |
| `Game::Update`／`Room::Update` 内联显示标量 | 无 | 不运行 | 同步 Host 当前值，Client 不本地推进（Q8，4.6） | RE-1、RE-2 已核 |
| `Game::Render` | `game_render` `0x2F7A80` | 拒绝 | 包装放行 | RE-5 |
| `Room::PreRender`／`Room::Render` | `room_prerender`／`room_render` | 拒绝 | 随 `Game::Render` 放行 | RE-5 |
| `EntityList::render_sort` | `entitylist_render_sort` `0x150C0` | 拒绝 | 保持拒绝：唯一原生调用者是已拦截的 EntityList Update，Render 树不调用它。Client 用自有有界全排维护 renderEL（6.2） | [RE-1 已核](client-display-delegation/RE-1/update-order.md#4-排序与移除提交) |
| `MenuManager::Render` | `client_menu_render` `0x5623C0` | 场景绘制边界 | 只放行原生菜单 | — |
| `HUD::Update`、`HUD::PostUpdate`、`Minimap::Update` | 无（REPENTOGON zhl 已声明） | 不运行 | 显示推进 30Hz 批次（6.2）。`PlayerHUD::Update` 嵌在 `HUD::PostUpdate` 内（同时完成 HistoryHUD／StatHUD），外层不再单独调用 | [RE-1 已核](client-display-delegation/RE-1/update-order.md#2-hud-的前置状态与允许副作用) |
| `Camera::UpdateUltrasmooth`／`Camera::UpdateDrag2` | 无（REPENTOGON zhl 已声明） | 不运行 | 相机阶段，每个 Manager 帧按本机 CameraStyle 二选一，不连续调用两个入口；入口内有本地震屏 RNG，按 Q4 接受 | [RE-1 已核](client-display-delegation/RE-1/update-order.md#13-相机在-manager-的独立阶段) |
| Minimap 缓存刷新 `Minimap::Refresh` | 无（`0x565D60`，重建 `Game+0x2545C`／`+0x255BC` 两份缓存） | 不运行 | 地图输入安装后或缓存失效时，在显示推进中调用；需要有效图形 context。不调用 `Game::UpdateVisibility` | [RE-1 已核](client-display-delegation/RE-1/update-order.md#3-地图更新与缓存刷新)、RE-7（图标输入） |
| `ItemOverlay::Update` | 无（REPENTOGON zhl 已声明） | 不运行 | 拦截。isaac-lan 证实它会治疗玩家、切房，并在动画标记处放音效；全屏演出改为按状态同步（2.3） | — |
| `LuaEngine::PostRender` 及 Render 期间的其他回调分派 | `lua_post_render` | 拦截 | 放行（Q6） | — |

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
- Lua 回调分派放行（Q6），包括 PostRender（回调 2）、HUD 1022/1024、心形 1118/1091、特殊 HUD 元素，以及各实体类 PRE／POST_RENDER（如 NPC 28、Tear 41、Projectile 45）。
- 持久副作用继续由现有的保存、PGD 和成就 hook 阻断。
- 写集只把已知项定稿，其余由第 15 步 E28 实测，见 RE-5。

**已知事实（tx-04-17 已核，J273 VA）**：

- `Game::Render` 在 `0x6F7AE1` 调 `Room::PreRender`（`0x803E20`），在 `0x6F7DF4` 调 `Room::Render`（`0x804500`）；门过渡状态改走 `0x8273C0`。
- `0x6F7E02` 调 `LuaEngine::PostRender`，`0x6F8108` 调 `HUD::Render`（`0x97C300`）。
- 开头的待处理分支 `0x6F7ACF→0x703E40` 会调用资源和全局 helper，并重设 Room 显示字段。触发条件是本机窗口尺寸请求 `Game+0x25AAC`，处理后清零；请求字节为本地状态，不从 Host 安装（RE-5）。
- 房间年龄为 30 时读当前 RoomDescriptor 的 SpawnSeed（`+0x5C`），为 0 即 `int3`；Client 必须安装 Host 当前值（Q9，第 5 节）。
- 基础绘制 `0x6AC4E0` 写 Entity `+0x1A4/+0x1A8`，Tear 同样写此缓存；旧 Tear 伪代码十进制 `420/424` 不是十六进制子类 `+0x420/+0x424`。Projectile 临时改 `+0x350` 后浮点加回，需精确恢复，恢复作用域为保留的 `client_entity_draw_projectile`（Q10，9.1）；Tear 暂存并恢复主 Sprite ColorMod `Entity+0xF0..+0x11B`，不是公共 `Entity.Color+0x1B8`。
- `Room::PreRender` 写 Room `+0x6EDC/+0x6EE0`，实际为本地 LightGradient Sprite 的 scale，并清本地坑纹理 dirty `+0x11F6`。
- `0x6F7D38` 写 `Game+0x25AB8=1`，没有看到写回 RNG。

已知本地范围与同步字段的恢复规则以 [RE-5 写集](client-display-delegation/RE-5/write-set.md)定稿；其余仍交 E28，未声称全树静态闭合。`HUD::Render→Minimap::Render` 与 `Game::Render→ItemOverlay::Render` 已确认，无需单独绘制调度，见[调用树](client-display-delegation/RE-5/render-tree.md)。

历史依据见 [tx-04-17 绘制边界](../tx-04-17/tx-04-17-render-presentation-boundary.md)和[原生维护与绘制](../tx-04-17/tx-04-17-native-maintenance-rendering.md)。已查的主体路径未发现实体 Update 或 Spawn；已定位的动画事件玩法消费者（例如 `Shoot` → Spawn，`0x555E10`）在 Update 中，随 Update 一起拦截。此结论不扩展为所有动态调用或 Render helper 的无副作用证明。

### 4.5 原生切房过渡（Q5）

依据：isaac-lan 的客机用 Host 已决定的参数回放原生转场（J460 `RoomTransition.Start`，五个参数：index、direction、animation、player、dimension）。它不调用会重新选路的 `Game::StartRoomTransition`，也跳过混乱诅咒分支，不消耗该分支的随机数。J273 中 RoomTransition 同样位于 `Game+0x1B83C`（tx-04-17 更新隔离），`RoomTransition::Update` 位于 `0x826430`。

下面保留 Q5 最初的候选接法；第2／3／5项的直接调用及等待方式，实施时以本节末尾的 RE-9 静态核对修正为准，其中的自有显示驱动又被 Q11 取代（本节末尾）。

1. **Host**：换房仍由 Host 原生完成。Host 在转场启动点采集上述参数，作为“转场”事件按序发出（2.3），并与新房间的 world 目标关联。
2. **启动**：Client 在帧号到达事件帧时，用同一组参数调用原生转场启动入口回放，不重新选路。
3. **推进与绘制**：
   - `RoomTransition::Update` 由“显示推进”调用。
   - `Game::Render` 开头的待处理分支和过渡绘制（`0x8273C0`）随 `Game::Render` 放行。
4. **换房点**：`RoomTransition::ChangeRoom` 拦截原函数，改由 Client 已有的原生构建与应用根（NativeBuild／NativeApply）安装 Host 的新房间目标。
   - 不运行 `Level::ChangeRoom` 选路、旧房保存、生成和进房玩法效果。
   - 不调用 `Room::Init`（RE-2：只拦 Spawn 不足以隔离它的副作用）。背景、光照、相机和门 Sprite 等显示初始化，由 NativeBuild／NativeApply 按 [RE-2 冷构建顺序](client-display-delegation/RE-2/room-init.md#4-建议的-client-冷构建顺序)复用已核的显示资源子段。
5. **等待**：到换房点时，如果新房间的完整目标还没就绪，就让转场停在换房点，不再推进；就绪后再继续。不显示半套房间。
6. **Boss 开场**：作为事件发送，Client 优先用 REPENTOGON 公开的 `RoomTransition.StartBossIntro` 启动（Q3）。
7. **换层**：本决定只覆盖切房。换层是否同样回放原生 StageTransition，另行决定。

**2026-11-12 RE-9 静态核对修正**：上面“调用启动入口／Update”描述的是原候选接法。J273 内部 Start 已定位为 `VA 0x823990 / RVA 0x423990`（ECX=this，五个32位栈参数，`retn 0x14`），但 Start 与 Update 都直接写 Player，Boss完成分支还写玩法保护计数；两者不能整根放行。现改为**具名显示拆分**，原生对象／Render和本地surface继续承担Q5显示，完整Game／Level／RoomInit与玩法部分保持拦截。Host采集接受后的启动，并在真实ChangeRoom完成后关联最终目标；Maze可能在Level里再次选路。目标未就绪时在上层显示阶段停住，不能让void ChangeRoom“空返回”后原FSM继续。窄显示驱动是实施建议，未新增用户决定，详见[原生阶段证据](client-display-delegation/RE-9/native-transition.md)与[等待／安装交接](client-display-delegation/RE-9/client-split-contract.md)。

**Q11（2026-11-12 用户决定，取代上段的自有显示驱动建议及第2／3／5项）**：转场状态按 Q8 方式同步 Host 当前值，Client 不运行任何 RoomTransition Start／Update 代码，也不另写复现 FSM 的显示驱动。

- **同步字段**（随 world 帧原子安装，RoomTransition 位于 `Game+0x1B83C`）：mode `+0`、counter `+4`、duration `+8`、progress `+0xC`、direction `+0x10`、roomIndex `+0x14`、dimension `+0x18`、animation `+0x1C`、Boss 标记 `+0x238`。`Game+0x1BB74`（转场 Player*）不上线，需要时按稳定成员身份在本机解析。surface strong pair `+0x28/+0x30`、versus ANM2 `+0x38`、肖像 map `+0x224`、滚动补偿 `+0x22C/+0x230`、`+0x234` 均为本地资源／本地计算，不同步。
- **Client 本地工作**：
  1. **旧 surface**：安装一个“当前房间身份与已安装 world 不同”的 world 帧前，用仍完整的旧 world 按原生 mode1 的方式（`Room::PreRender`→`Room::Render` 写入 `+0x28`）捕获旧画面。原生在 mode1 内捕获，但 Host 的 mode1 可在同一帧内转 mode3 并换房，Client 不一定看到“mode1＋旧房”的帧，所以捕获点放在安装事务之前，不依赖 mode 值。
  2. **新 surface**：同一事务安装新房间 world、F／锚点／SpawnSeed／Q8 与注入后，按 ChangeRoom 尾部的方式（`0x826168`／`0x8261DC`）捕获新画面到 `+0x30`，不调用完整 ChangeRoom（它含旧房保存、额外 RoomUpdate、选曲和显式 SwapBuffers）。
  3. **滚动补偿**：按已安装的 direction 与两房几何、本地 camera scroll 计算 `+0x22C/+0x230`，对应原生 Game::ChangeRoom 的补偿计算，不安装 Host 数值（分辨率、相机为本地）。
  4. **versus／肖像**：Boss 标记非零时，30Hz 批次推进 versus ANM2 与肖像 map 动画（原生 mode2 `0x826AE0` 一段的显示部分）；mode、counter 与 Boss 标记的变化仍来自 Host。资源由具名 BossReplay 调 `StartBossIntro` 建立，其中的选曲 `0x823D8C`、环境声清除 `0x823D9D` 隔离；它对 `Game+0x22E90` 的写入在下一次安装时被同步的 HUD 可见性覆盖（6.4）。
  5. **HUD 刷新请求**：原生共同尾部在 mode2／3 时写 `Game+0x22E94/+0x22E9C`（HUD `_hudRefreshType`／`_hudRefreshCountdown`）为 2。Client 在显示推进中按已安装 mode 本地提出同一请求，属于 RE-1 已定的本地刷新维护，不同步 HUD 内部计数。
  6. **绘制**：`Game::Render` 照原生按 mode 二选一调用 `RoomTransition::Render`（`0x8273C0`），读已安装的 mode／animation／direction／progress 和本地 surface。
- **换房点等待**：world 按帧原子安装，新房间的 world 帧到达前 Client 一直停在上一帧的“旧房＋Host 当前 mode／progress”，天然得到“停在换房点”，不需要拦截 void ChangeRoom 后冻结 FSM。`RoomTransition::ChangeRoom` hook 仍保留拦截，只作为守卫。
- **音效**：Client 不运行 Start／Update，转场内的所有声音（Start `0x823CE1` 的 608／609、mode3 `0x826D79` 的 sound 1、mode4 `0x826FBA` 的 sound 214）都按 Q7 作为 Host 事件发送和回放。
- **不变的部分**：完整 Game／Level／RoomInit、Player 姿态与可见性（Host 当前值随 Entity 同步）、进房选曲和 Boss 完成分支的 `Player+0x418` 写入都只在 Host 发生。Host 侧采集仍按 RE-9 关联转场序号与最终目标，供中途连接、事件幂等和 resize 恢复使用。
- 第 1 节 RE-9 的事件／目标关联、resize 与断线失败方式仍适用；`client-split-contract.md` 第 2—3 节的自有驱动步骤由本决定取代，第 2 节“只 hook ChangeRoom 不够”的结论仍成立（Client 不运行 FSM，自然不触发）。

### 4.6 内联显示标量（Q8）

**决定（2026-11-12）**：原生 `Game::Update` 和 `Room::Update` 里内联推进的显示标量，同步 Host 当前值，Client 不本地推进。

**理由**：

- 这些值在原生上只随 `Game::Update`／`Room::Update` 推进，约 30Hz，与 world 帧同频。方案B 下帧号也已对齐，安装 Host 当前值就能原样复现原生显示。
- 只在值变化时进入增量，体量为几十字节。
- 同一字段既安装又本地推进，会重复推进或逐帧漂移（RE-1 失败方式 11）。同步当前值从根上避开这一类问题，也不需要从原函数中段抽取运算。
- 现行 `client_presentation_native.cpp`（约 785—791 行）已在绘制前直接安装颜色、闪电和暗度的当前值，做法一致。

**范围**（字段来自 RE-1、RE-2；是否保留每个字段，最终由需求清单按消费方判定）：

| 来源 | 显示值 | 同步的当前值 |
| --- | --- | --- |
| Game（RE-1） | Bloom | `+0x25AA0` 计时；`+0x25AA4/+0x25AA8` 参数 |
| Game | 震屏 | `+0x25A80` 倒计时。偏移 `+0x25A84/+0x25A88` 由本地相机写入，不同步 |
| Game | Hallucination | `+0x25AB0` 计时、`+0x25AB4` 总时间；历史画面在本地 |
| Game | 颜色 | current `+0x66920` 和 flag `+0x6691C`；target `+0x66938`、step `+0x66950` 只在有消费方时保留 |
| Game | 闪电 | `+0x6699C` |
| Game | 暗度 | intensity `+0x25A90`；countdown `+0x25A8C` 只在有消费方时保留 |
| Game | 眩晕 | current `+0x669A0` |
| Game | Retrovision | `+0x25F58` 倒计时、`+0x25F5C` 总时间 |
| Game | fade 与开场遮罩 | primary alpha `+0x25B28`、secondary `+0x25B10`。控制值 `+0x25B34` 只作为 Host 生命周期结果，不在 Client 驱动退出、存档或换局 |
| Game（2026-11-12 审阅追加） | HUD 可见性 | `+0x22E90` 即 HUD `_isVisible`（HUD 位于 `Game+0x1D9AC`，字段 `+0x54E4`，`HUD.zhl:66`），1 字节。原生由 StartBossIntro 清 0、Boss 完成分支置 1；Client 安装 Host 当前值，具名 BossReplay 的本地写入在下一次安装时被覆盖（6.4） |
| RoomTransition（Q11） | 转场状态 | `Game+0x1B83C` 起 mode／counter／duration／progress／direction／roomIndex／dimension／animation 与 Boss 标记 `+0x238`；surface、versus、滚动补偿为本地（4.5） |
| Room（RE-2） | 水色过渡 | 四个 KColor 端点、`+0x7294` duration、`+0x7298` countdown |
| Room | 冲击波 A／B | `+0x70C4`、`+0x70E0` 两组的 count、max、radius 等 |
| Room | 水量与熔岩 | `+0x7240` waterAmount、`+0x7740` 熔岩强度 |
| Room | 地狱横移背景 | 视图 `+0x776C` 内的 `+0x11C` 绘制位移、`+0x124` 累计位移 |

**仍在 Client 本地推进的**（原生节奏或状态本来就在 Client）：

- 相机：每个 Manager 帧，按本机 CameraStyle；本机关闭插值时跳过奇数帧（4.1）。
- ANM2／AnimationState：Backdrop floor 两路、地狱横移背景 Sprite。
- FXLayers 更新 `0x56BF40`：含本地 RNG、vein 图和 pose。
- HUD、PlayerHUD、Minimap 自身的动画与消息状态，以及周期 HUD 刷新请求（GameFrame 每 30 帧写 type=1、countdown=2，type=2 不覆盖）。

**安装点补做**：

- 原生推进到阈值时会清理 Client 本地字段。这类动作由安装点按同步值的变化补做。
- 已知一项：震屏倒计时从正数变为 0 时，清零 `Game+0x25A84/+0x25A88`。偏移由本地相机写入，原生在 `Game::Update` 中清零；不补做会留下持续偏移。
- 其余阈值动作在实现时按 [RE-1 附表](client-display-delegation/RE-1/additional-display-work.md)逐项核对。

**不变**：

- Client 不调用 `0x6EB990`、`Game::SetColorModifier`、房间颜色提交 `0x818740`，也不运行 `Game::Update`／`Room::Update` 的任何内联段。
- fade、演出和开场遮罩的生命周期动作（退出、存档、换局、ShowName）只由 Host 决定。
- 放行的显示代码如果改写这些字段，按 13.4 漂移校正处理。

**被取代的研究建议**（地址、运算和副作用事实照常保留，作为字段定位和语义依据）：

- RE-1 成果中“可拆出的值推进”（颜色 helper `0x6EB990` 及纯标量）和附表里“可拆标量”的处理建议。
- RE-2 第 4.1 节的水色计时与冲击波纯值抽取，以及第 4.4 节在 Client 用 `ComputeColorModifier`／`SetColorModifier` 提交房间颜色的设计。

## 5. 仍需同步的显示相关状态（候选）

下表是需求清单的起点，最终以 RE 结果和第 2.1 节的判据定稿。

| 域 | 保留候选 | 安装方式 |
| --- | --- | --- |
| Entity 公共 | 身份、type／variant／subtype；Position、Velocity、PositionOffset、SpriteOffset、SpriteScale、SpriteRotation；Color；Visible 及影响绘制的标志；DepthOffset；父子关系；HP／MaxHP（Boss 血条）；`+0x328` 锚点；主 Sprite 的 ANM2 状态（文件、动画、帧、覆盖层、LayerState） | 原生 owner／setter |
| Entity 子类 | 各类型可见字段：Tear 高度、Laser 端点与采样、Knife、Bomb 外观标志、Pickup 价格／充能／状态、Familiar、Effect 各变体的可见状态、NPC 冠军色、Slot 显示 Sprite | 原生 owner |
| Player | costume 层、身体与头部 Sprite、颜色；HUD 读取的库存、主动道具与充能、口袋、饰品、心、显示用属性、持物；Jacob／Esau 等多身体 | 原生 owner；HUD 由原生代码读取 |
| Grid | type／variant／State、Sprite 帧、门状态、便便与石头状态 | 原生 owner |
| Room／Level | 房间描述（DisplayFlags、VisitedCount、Clear、类型、形状、图标）、raw 诅咒 `Level+0xC`（2026-11-12 审阅，6.3）、stage／stageType、背景类型；由 Host 随机选出的装饰结果，同步选中结果，不同步 RNG；Host 选定的背景 id 与 FX 资源选择（RE-2）；Q8 的 Room 显示标量（4.6）；当前房间描述的 SpawnSeed（`+0x5C`，4 字节，2026-11-12 用户决定，见表后说明） | 原生 owner；地图由原生 Minimap 绘制 |
| Run | 硬币、炸弹、钥匙；挑战编号、种子效果（Seeds 对象 `Game+0x1BBD8`）、难度、计时；贪婪波次；Game 级 raw 诅咒 `Game+0x25AC8`（2026-11-12 审阅，6.3） | 原生 setter；raw 诅咒按普通字段安装 |
| Game 显示标量（Q8） | 4.6 表中的 Game 字段：Bloom、震屏倒计时、Hallucination、颜色、闪电、暗度、眩晕、Retrovision、fade／开场遮罩 alpha；HUD 可见性 `+0x22E90`（2026-11-12 审阅） | 安装点直接写当前值，并补做阈值动作（4.6） |
| 转场状态（Q11） | RoomTransition 的 mode、counter、duration、progress、direction、roomIndex、dimension、animation、Boss 标记（4.5） | 随 world 帧原子安装，Client 不运行转场 FSM |
| 分支与暂停（Q12） | 每个 world 帧：Host `Game::IsPaused` 的结果（1 位），以及 Host 按原生优先级在 Update 后状态上算出的终结分支枚举（6.2） | 随 world 帧安装，供显示推进选分支、相机和 `client_entity_draw_paused` 读取 |
| 注入值 | 见 6.3：恶魔房／天使房几率、药丸效果与识别、故障道具定义、地图拾取图标、水流向量等。~~诅咒~~ 改为同步 raw 字段（上两行） | 2.3 注入 |
| 按状态同步的表现 | 房间音乐请求、正在播放的循环音效集合、各控制器的全屏演出（ItemOverlay 的状态、延时和 Sprite） | 原生 owner；见 2.3、6.4 |
| 事件 | 非循环音效（只含玩法路径产生的，Q7；Q11 后含全部转场声音）、Boss 开场、道具横幅、fortune 文字（均带本地化键）、转场序号与最终目标关联（4.5） | 2.3 事件队列；见 6.4 |

**地图拾取图标（待定）**：RE-7 已定位来源，但 Client 上的接法尚未决定，候选两种：① 镜像最小的 SavedEntities 条目，让原生 Minimap 构建照常读取；② 原生构建完缓存后按 Host 摘要修补图标缓存。实施前在第 8／13 步选定，见 13 节。

**SpawnSeed（Q9，2026-11-12 用户决定）**：

- **原因**：放行原生 `Game::Render` 后，房间年龄为 30 时它读当前 RoomDescriptor 的 `+0x5C`，为 0 就执行 `int3`（[RE-4](client-display-delegation/RE-4/frame-consumers.md#22-roomage30历史-surface-捕获)）。Client 目前没有 descriptor seed 的写入者，不能保证这个值有效。
- **范围**：只同步当前房间描述的 SpawnSeed 一个 u32。DecorSeed（`+0x58`）和 `+0x60`／`+0x64` 两个 seed、其他房间描述的 seed、全局 RNG 游标都继续留在 Host。
- **更新**：按变化同步，不是只在进房时采一次。Host 的补货等正常路径会改写 SpawnSeed（[房间描述核对](room-descriptor-synchronization-review.md)）。换房时，新房间的值必须在 Render 前随 world 一起安装。
- **不授权生成**：写入 SpawnSeed 不让 Client 上任何读它做生成的代码获得执行资格。补货、出生、奖励等生成路径仍由玩法拦截拒绝。
- **对旧方案的影响**：[房间描述核对](room-descriptor-synchronization-review.md)第 54 行原定“完整 seed 留 Host，只发 1B 的 `error_is_modeling_clay` 谓词（D1）”，现被本条取代。完整 seed 已同步后，官方 Error 饰品纯函数能在 Client 上算出同样结果，D1 可以评估退役；在第 4 步布局收缩时决定，不与 SpawnSeed 双发同一信息。

## 6. 新增逆向任务（J273 静态反编译）

| 编号 | 内容 | 产出 |
| --- | --- | --- |
| RE-1 | `Game::Update`（`0x2F6D30`）中 6.2 候选函数的 J273 签名、先后顺序和前置状态；另外核对屏幕震动、淡入淡出等是否还有遗漏的显示工作 | [原任务及补遗静态完成](client-display-delegation/RE-1/README.md)：白名单、调用顺序、[提前返回分支](client-display-delegation/RE-1/early-return-branches.md)、[条件需求](client-display-delegation/RE-1/branch-demand-handoff.md)与[关闭插值的绘制节拍](client-display-delegation/RE-1/manager-render-cadence.md)；未实施／未测试 |
| RE-2 | `Room::Update` 与 `Room::Init`（`0x3E8AD0`）子调用分类：EntityList、Grid、清房与奖励、相机、背景、水面、光照、生成 | [静态成果已完成](client-display-delegation/RE-2/README.md)：拦截／拆分清单、显示资源顺序与 ABI；未实施／未测试 |
| RE-3 | `Game+0x25A70` 的递增／重置／恢复、`Room+0x11F0` 的入房基准写点、显示依赖计数，以及 Manager 原链跳过后的本地递增（4.1） | [静态成果已完成](client-display-delegation/RE-3/README.md)：帧号写入点、Q8／本地计数目录、安装点与30Hz／相机阶段的计数来源；未实施／未测试 |
| RE-4 | 显示代码中按帧号相等或差值做判断的地方（闪烁、每帧一次的保护等），以及它们在跳帧和停顿下的行为。Q8 后，Game／Room 内联标量不再本地推进，重点是 30Hz 批次和 Render 中读 GameFrame 的消费者（如周期 HUD 刷新） | [静态成果已完成](client-display-delegation/RE-4/README.md)：帧号消费者、跳帧／停顿风险、周期刷新／本地去重与缓存处理；未实施／未测试 |
| RE-5（已收窄） | 两项：① `Minimap::Render`、`ItemOverlay::Render` 等是否都在 `Game::Render` 树内，决定是否需要单独挂点；② Render 已知写集（原 RE-6 并入）。4.4 的显示缓存定为 Client 本地；Projectile PositionOffset／Tear 主 Sprite 颜色等同步输入保留并按作用域恢复，其余不做静态闭合，交第 15 步 E28 实测 | [静态成果已完成](client-display-delegation/RE-5/README.md)：调用树、布局源本地职责说明与漂移范围；未实施／未测试 |
| RE-6 | 已并入 RE-5，本次统一完成收窄范围的静态交付 | [同一成果](client-display-delegation/RE-5/README.md)，不另开任务／验收 |
| RE-7 | 6.3 各注入值在 J273 上的字段、getter 或 API；天使占比 getter、药丸识别、地图拾取图标 | [静态成果已完成](client-display-delegation/RE-7/README.md)：注入表、原生ABI、混合setter与地图／程序道具的实现交接；未实施／未测试 |
| RE-8 | 6.4 各事件与状态在 J273 上的 Host 采集点和 Client 播放点，以及 Q7 的音效来源判定 | [静态成果已完成](client-display-delegation/RE-8/README.md)：入口、载荷语义、文字list／本地化、作用域与回放标志；未实施／未测试 |
| RE-9 | 原生切房过渡（4.5），共五项，见表后列表 | [静态成果已完成](client-display-delegation/RE-9/README.md)：启动ABI、mode阶段、换房链／Render写集、Host最终目标关联与等待／拆分交接；未实施／未测试。其中的自有显示驱动建议已被 Q11 取代（4.5） |
| RE-10（2026-11-12 新开） | Manager::Update（`0x92E3E0`）尾部的非 Game 调用（约 50 个 call site）逐项分类：显示推进根放行、继续拦截或本地维护。已知必须放行：音乐逐帧 tick `0x7D7DA0`（唯一调用者 `0x92EB49`）、`0x908340`（`0x92EB3E`，疑为 SFX tick） | **待做**。产出放行／拦截表、前置状态与调用顺序，并入 6.2 相机／音频阶段 |

RE-9 的五项：

1. J273 的转场启动入口（对应 isaac-lan J460 `RoomTransition.Start`）的地址、参数和调用约定，以及 `Game::StartRoomTransition` 中选路和随机部分的边界。
2. `RoomTransition::Update`（`0x826430`）的各个阶段、调用 `ChangeRoom` 的位置，以及等待目标时如何让转场安全停住。
3. `RoomTransition::ChangeRoom` → `Level::ChangeRoom` → `Room::Init` 链上的玩法副作用，并确认换成 Client 构建后不缺显示状态。
4. 过渡绘制（`0x8273C0`）的进入条件与写集。`Game::Render` 开头的 `0x703E40` 由本机窗口尺寸请求 `Game+0x25AAC` 触发，与转场无关（[RE-5](client-display-delegation/RE-5/write-set.md)），请求字节为本地状态；它的完整写集不再作为转场前提，只在第 15 步 E28 中按实测差异补查。
5. Host 侧转场参数的采集点。

- 先复用 [docs/hooks/README.md](../../hooks/README.md) 和已有证据文档中的点位，再补齐缺口。
- REPENTOGON zhl 已声明 `HUD::Update`、`HUD::PostUpdate`、`PlayerHUD::Update`、`Minimap::Update`、`Minimap::Refresh`、`Camera::UpdateDrag2`（本文原称 UpdateDrag）和 `Camera::UpdateUltrasmooth`；其 J273 签名已在 RE-1 中核对。`ItemOverlay::Update` 也已声明，但不进白名单（6.2）。
- REPENTOGON zhl 还声明了 `RoomTransition::Update`／`Render`／`ChangeRoom`／`StartBossIntro` 和 `Level::ChangeRoom`，但没有声明转场启动入口；RE-9已从原生caller定位`0x823990`，其混合副作用亦已核，不能作为纯显示根直接调用。

### 6.1 借鉴 isaac-lan 排除的逆向点

isaac-lan v0.7.2（J460，无 REPENTOGON）的客机把 `Game::Render` 整个放行，树内子调用一个都不拦，只在外层套视图作用域。v0.7.1 已通过双端全路线实机验收。据此排除以下各点：

- **原生 Render 能否在 Client 安全运行**：不再逐个闭合尾部 helper 和光照闭包等未知副作用，改由 E28 实测漂移。
- **绘制顺序与图形状态**：阴影、光照、水面、FX、shader 状态全部由原生自己处理。tx-04-17 ST4 中手工拼合绘制步骤的研究整体作废。
- **HUD 与 Minimap**：原生 Update、PostUpdate、Render 在客机可用。
  - isaac-lan 额外重新绑定了 PlayerHUD 槽位。只换指针会留下队友的心形和库存缓存，所以它走原生 reset、刷新标志，再重算 HistoryHUD。
  - 它这么做是因为玩家分在不同房间。我们客机上的玩家都是原生 Player，由原生自己绑定，只在第 15 步确认，不单独逆向。
- **相机**：isaac-lan 只显式调用 `Camera::UpdateUltrasmooth` 与 `Camera::UpdateDrag`，再清 `camera+0x8c`，绕开了 Manager 的相机尾部（tx-04-17 ST3 发现那里会条件调用共享 RNG）。RE-1 的相机部分只核这两个入口。
  - **RE-1 修正（J273）**：相机按本机 CameraStyle 二选一，不是两个都调：值为 2 调 `UpdateDrag2`（SDK 名），其他值调 `UpdateUltrasmooth(本机 Manager 半帧奇偶)`。
  - 两个入口内部在震屏时也会调用全局 `Random()`，所以绕开 Manager 尾部并不能避开 RNG。这是本地外观随机，按 Q4 接受，不构成放行玩法随机的依据。
- **半帧更新**：isaac-lan 证实半帧更新含有玩家插值和玩法，客机跳过。我们继续拦截 Odd helper（`0x2F9340`）。
- **回调**：Q6 放行回调分派，不再逐个确认回调编号和取消语义。

isaac-lan 排除不了的点：

- **REPENTOGON 专有部分**：回调，以及 `InterpolV2`、`RenderFrameEx` 等渲染时补丁。在方案B下帧号已对齐，这两个补丁可以接受。
- **J273 地址**：转场启动入口等需要自己定位。
- **帧号消费方（RE-4）**：isaac-lan 的客机本地帧号照常递增，没有对齐 Host，排除不了。
- **写集**：isaac-lan 每个 tick 全量重写，不需要漂移校正；我们只发变化块，仍需要 RE-5 的写集清单。

**新增设计项（不是逆向）**：半帧更新被拦截后，Client 没有原生插值，实体会随 world 按约 30Hz 跳动。isaac-lan 为此自己写了纯显示的插值（`motion.lua`）。我们是否在 Render 作用域内做显示插值并在返回后恢复，放到第 13 步设计中决定。

### 6.2 显示推进的白名单与调用顺序（RE-1）

本节为 2026-11-12 依据 isaac-lan v0.7.2 源码和 REPENTOGON 1.1.3 源码整理，J273 地址与签名仍待 RE-1 核对。

**RE-1 同日完成补充**：[成果与入口](client-display-delegation/RE-1/README.md)、[J273 调用顺序及前置状态](client-display-delegation/RE-1/update-order.md)、[内联显示工作](client-display-delegation/RE-1/additional-display-work.md)已记录；以下候选表与“剩余工作”保留研究开始时的内容，J273 核定以成果为准。关键修正：HUD::Update 在前，PlayerHUD::Update 嵌套于最后的 HUD::PostUpdate；相机在 Manager 尾部按 CameraStyle 二选一（SDK 名 UpdateDrag2），两个入口内部仍有本地震屏 RNG；地图缓存位于 Game+0x2545C／+0x255BC；render_sort 需要显式维护，CommitRemovedEntities 不加入逐帧显示白名单。没有实施代码或运行测试。

**J273 核定后的显示推进（2026-11-12 采用）**：

由 Manager 边界调度（4.1）。RVA 均为 J273。Game／Room 内联显示标量不在此推进，按 Q8 在安装点写入（4.6）。下表的普通／切房序列须先通过[补遗分支优先级](client-display-delegation/RE-1/early-return-branches.md)选择，不能只检查“转场活跃”便覆盖更高优先级的 hold／GameOver／换层等分支。

| 阶段 | 时机 | 调用 | 说明 |
| --- | --- | --- | --- |
| 0 | 每个 Manager 帧，显示推进之前 | world 安装、库存与注入、帧号写回、Q8 标量安装及阈值补做 | 第 3 节、4.6、6.3 |
| 1 | 30Hz 批次 | `HUD::Update`（`0x57ACF0`） | 原生在终结分支之前调用；更早的 Console helper 阻断可跳过 U，见补遗 |
| 2 | 30Hz 批次，Host 终结分支为 RoomTransition 时 | 转场本地显示工作（Q11：Boss 标记非零时推进 versus／肖像 ANM2；按已安装 mode2／3 提出 HUD 刷新请求）→ `Minimap::Update` → `HUD::PostUpdate`，本批结束 | 保留原生相对顺序。Client 不推进转场 mode／counter／progress，它们随 world 安装（4.5 Q11）；旧／新 surface 在安装事务中捕获，不在本阶段。~~具名转场显示拆分~~（RE-9 建议，已被 Q11 取代）；完整`RoomTransition::Update`（RVA`0x426430`）含直接玩法写入，继续拦截 |
| 3 | 30Hz 批次，普通分支 | `Minimap::Update`（`0x565DB0`）；地图输入安装或缓存失效后 `Minimap::Refresh`（`0x565D60`） | Refresh 没有固定的原生调用点，按依赖调用 |
| 4 | 30Hz 批次，普通分支 | renderEL 排序 → FXLayers 更新 `0x56BF40(false)` → Backdrop floor 两路 AnimationState、地狱横移背景 Sprite | 对应原生 `Room::Update` 内的顺序。排序用自有有界全排，不调原生 `render_sort`（scratch 容量、空表、GameFrame 奇偶渐进三个问题） |
| 5 | 30Hz 批次，普通分支 | `HUD::PostUpdate`（`0x57AE90`） | 内部完成 PlayerHUD、HistoryHUD、StatHUD，外层不重复调 PlayerHUD |
| 6 | 每个 Manager 帧，Render 前 | 相机阶段：CameraStyle 为 2 调 `UpdateDrag2`（`0x51E5E0`），否则调 `UpdateUltrasmooth(本机半帧奇偶)`（`0x51D840`）；按原生规则清 `Camera+0x8C` | Host 暂停时跳过，例外是转场 mode 非零且 animation `Game+0x1B858`（RoomTransition `+0x1C`）为 19（MINECART）（RE-1；审阅更正：`+0x1C` 是 Start 写入的 animation 参数，不是“阶段 19”）。暂停取 Q12 同步的 Host paused 位，mode／animation 取 Q11 已安装值。本机关闭插值时，奇数帧按原生快返条件跳过（4.1） |
| 6a（RE-10 待核） | 每个 Manager 帧，与相机同属 Manager 尾部 | 音乐逐帧 tick `0x7D7DA0`、`0x908340`（疑为 SFX tick）；其余尾部调用待 RE-10 分类 | 原生在 `0x92EB49`／`0x92EB3E` 调用，走与相机相同的 Manager 尾部路径（含插值快返）。不运行时 Client 的音乐淡入淡出／排队曲目和 SFX 内部状态不前进。具体条件、顺序与前置状态以 RE-10 为准 |
| 7 | 每个主循环 Manager 周期，更新／保持本机 M 后 | 主循环调用原生 `Manager::Render`，实际到达 `Game::Render` 时包装放行（4.2） | 绘制体要求本机插值开启或 Mpost 为奇数；还受当前 Manager 场景／资源分派约束，不额外补调 Game::Render |

- 30Hz 批次在本机 Manager 帧为偶数时运行，不等 world 帧到达；计数来源见 [RE-3](client-display-delegation/RE-3/apply-contract.md)。[RE-4 调度交接](client-display-delegation/RE-4/handling.md)规定每本地周期的批次／相机去重、周期HUD请求合并及安装后缓存刷新；不以F相同跳过整个本地批次或Render。
- `CommitRemovedEntities` 不进入显示推进；确需处理官方 staged remove 时，放进实际安装或退休事务（RE-1）。
- **提前返回分支（RE-1 补遗，静态完成）**：[分支表](client-display-delegation/RE-1/early-return-branches.md)列出 Console、`Game+0x25A6C>0`、GameOver、Leaderboard、StageTransition、RoomTransition、ItemOverlay／暂停组合、Prompt、次级 fade、PauseScreen 和额外 UI 的优先级与 U／P／M 调用。相机的综合 paused 与这些分支不等价；fade 前段的 Pα 也不是 return，同次原生 Update 可再调 P。Client 不运行 Q8 生产段，Pα 的当前状态资格／合并策略由第 13 步收口，不补演 Host 调用历史。
- **终结分支与暂停（Q12，2026-11-12 用户决定）**：
  - Host 每个 world 帧发送两项：本帧 `Game::IsPaused`（`0x6F92A0`）的结果（1 位），以及按原生优先级在 Update 之后的状态上算出的终结分支枚举（按[分支表](client-display-delegation/RE-1/early-return-branches.md#3-终结分支表严格按出现次序)的次序：Console 阻断、hold（`+0x25A6C>0`）、GameOver、Leaderboard、StageTransition、RoomTransition、ItemOverlay ACTIVE（6a）、Prompt、未命名 UI 字节 `+0x24ECC`、次级 fade、PauseScreen、额外 UI、普通）。Client 按枚举选 30Hz 批次的序列（阶段 1—5 的 U／M／P 组合见补遗分支表），不在本机重算谓词，也不复制菜单／平台／指针对象。
  - `client_entity_draw_paused` 从默认删除改为保留：它挂在 IsPaused 上，原生 53 个调用者（含 `Room::Render` 的 `0x807775`、`0x807CB0`）都在 Client 读同步的 paused 位；相机阶段同样读这一位。
  - Client 本地菜单／控制台阻断与 Host 分支如何合并，由第 13 步决定。
- **Pα（2026-11-12 审阅建议采纳）**：Pα 不另设同步位，由已安装的 Q8 primary alpha 判定：`Game+0x25B28 > 0`（对应原生 `0x6F72C1` 前的 `0x6F6EB0` 检查）时，在 Pα 的位置调用 `HUD::PostUpdate`。同一批次可与终结分支的 P 各调一次，与原生相同。alpha 正好在某个 Host 帧降到 0 时，Client 看到的是安装后的值，这一帧的差异按 Q4 接受。
- **条件需求交接**：[逐字段清单](client-display-delegation/RE-1/branch-demand-handoff.md)区分已有设计需求、旧 UNKNOWN 整段占位和缺少窄谓词的项。ItemOverlay、Q8 alpha、转场已有设计需求；hold、GameOver／Leaderboard／StageTransition、Prompt、额外 UI、暂停组合仍需在第 7／8 步收缩时落实必要分支语义，第 13 步接线。旧 D0.paused 不算新方案已接通；不复制菜单／平台／指针对象，也不调用混合 Update 求分支结果。**Q12 后**这些分支的选择由 Host 发送的终结分支枚举承接，Client 不再需要逐项同步窄谓词；分支内显示所读的状态（如 ItemOverlay、Q8 alpha、转场字段）仍按各自需求同步。
- **插值快返（补遗已核）**：主循环依然调用 Manager::Render；其绘制体按 Mpost 与本机插值设置判断，保持 M 的控制字节例外见[真值表](client-display-delegation/RE-1/manager-render-cadence.md)。不将“入口被调用”写成“Game::Render 一定运行”。

**两者的用途**：

- **白名单**：决定 Client 每帧在“显示推进”里本地运行哪些原生显示更新，例如 HUD 动画、地图、相机和转场。名单外的函数一律不运行，与 4.1 的默认拒绝一致。名单同时是 Q7 音效来源判定的依据（6.4）。
- **调用顺序**：这些函数在原生 `Game::Update` 中前后依赖。例如 `HUD::PostUpdate` 读取 `HUD::Update` 和库存刷新的结果，地图缓存要在 DisplayFlags 写入之后刷新。顺序错了，会读到旧值或未初始化的值（失败方式 4）。顺序以原生 `Game::Update` 中的出现次序为准，world 安装与帧号写回排在最前。

**isaac-lan 的做法**（对照，不照搬）：

- **默认放行**：客机运行完整的原生 `Game::Update`，再逐个拦截玩法：实体 Update（各实体类 vtable+0xC）、半帧更新、`ItemOverlay::Update`、`BossOverlay::Update`、交易生成、清房奖励、复活、R 键和房间请求等。碰撞关闭。
- **HUD 与地图**：`HUD::Update`、`HUD::PostUpdate`、`HUD::Render` 和 `Minimap::Update` 套上视图作用域后原样运行。
- **相机**：在每个显示帧末尾显式调用 `Camera::UpdateUltrasmooth`（按半帧奇偶传参）和 `Camera::UpdateDrag`，并清 `camera+0x8c`。
- **地图缓存**：写入权威 DisplayFlags 后，只调用 J460 `Game::UpdateVisibility` 末尾的两次地图缓存刷新（`Game+0x25ee4`／`+0x26044`）。它前面按门重算可见性的部分不调用，因为客机的门不是权威。

我们采用相反的模型：默认拒绝，按白名单放行。isaac-lan 的拦截清单可以当作反面核对表：它拦截的不进白名单，它放行的显示函数是候选。

**REPENTOGON 的参照**：`MC_PRE_UPDATE` 跳过主更新时，REPENTOGON 仍依次执行 `EntityManager::CommitRemovedEntities`、`EntityList::render_sort` 和 `HUD::PostUpdate`（`repentogon/Patches/ASMPatches/ASMCallbacks.cpp` 的 `handle_skipped_update`）。这说明后两步是主更新跳过后仍需的显示收尾，可作顺序参照。Client 上的实体增删由 world 安装负责；`CommitRemovedEntities` 是否还需要调用，与 world 安装是否重复，由 RE-1 核对。

**候选白名单**（顺序暂定）：

| 顺序 | 函数 | 依据 | 备注 |
| --- | --- | --- | --- |
| 0 | world 安装与帧号写回（第 3 节） | 本方案 | 在显示推进之前 |
| 1 | `RoomTransition::Update`（`0x826430`） | Q5 | 其中的 `ChangeRoom` 拦截（4.5） |
| 2 | `HUD::Update`、`PlayerHUD::Update` | isaac-lan 原样运行 | |
| 3 | `Minimap::Update` | isaac-lan 原样运行 | |
| 4 | 地图缓存刷新 | isaac-lan 的地图刷新 | 只在 DisplayFlags 变化后调用；J273 函数地址和 `Game` 偏移待核 |
| 5 | `EntityList::render_sort`、`HUD::PostUpdate` | REPENTOGON 跳帧分支；isaac-lan 原样运行 | `render_sort` 也随 `Game::Render` 放行，是否在此重复调用待核 |
| 6 | `Camera::UpdateUltrasmooth`、`Camera::UpdateDrag` | isaac-lan 相机 | isaac-lan 每个显示帧调用一次；挂在显示推进还是 Render 之前，以及 `camera+0x8c` 的 J273 对应字段，待核 |

**明确不进白名单**：

- `ItemOverlay::Update`：会治疗玩家、切房，并在动画标记处放音效（isaac-lan 源码注释）。全屏演出改为按状态同步（6.4），由原生 `ItemOverlay::Render` 绘制。
- Boss 开场与 TV 演出的更新：isaac-lan 认为其中含原生路线标记，客机不运行，改用同步的姿态绘制。J273 Boss开场在RoomTransition内；RE-9已核完成分支会写Game+22E90和Player+418，故只推进具名动画／显示段，不放行完整Update。Q11 后 mode、counter 与 Boss 标记都随 world 安装，Client 只推进 versus／肖像 ANM2；`Game+0x22E90`（HUD 可见性）按 Q8 同步。
- 半帧更新（Odd helper `0x2F9340`）、实体 Update、`Room::Update` 的玩法部分、交易生成和清房奖励：继续拦截。
- `Game::UpdateVisibility` 中按门重算可见性的部分。

**RE-1 剩余工作**（2026-11-12 已完成，见 RE-1 成果；提前返回分支表的补遗见上）：

1. J273 上表中各函数的签名、调用约定和在 `Game::Update` 中的先后次序。
2. 地图缓存刷新函数的地址，以及它在 `Game` 中的两个对象偏移。
3. 相机字段，以及屏幕震动、淡入淡出等是否另有逐帧更新需要加入白名单。
4. HUD 是否依赖 `Game::Update` 前段建立的状态，例如库存刷新标志。

### 6.3 不可算值的注入方式（RE-7）

**2026-11-12 静态成果已完成**：[完整注入表与证据](client-display-delegation/RE-7/injection-table.md)。以下为核对后的方式，仍未实施；Q3的API优先不将混合setter变成无副作用leaf。

| 值 | isaac-lan（J460）的做法 | 我们的方式（按 Q3） | J273 现状 |
| --- | --- | --- | --- |
| 恶魔房总几率 | hook `Room::GetDevilRoomChance`，返回 Host 值 | getter hook。几率依赖 Host 的整局计数；总率与HUD最终资格分开承接 | VA`0x7EF200`，SDK thiscall float，ST0返回；不把百分比当原始总率 |
| 天使占比 | 用 thunk hook 一个内部函数，返回 Host 值 | getter hook，使用原生XMM0返回shim；仅Client会话启用 | VA`0x7EF590`／RVA`0x3EF590`，无显式参数；不是Level+182F4修正值 |
| 药丸效果 | 直接写 ItemPool（`Game+0x1a740`）`+0x7a4` 处的 15 项 | 直接写 | zhl 声明了 `ItemPool::_pillEffects`（`0x7a4`） |
| 药丸识别 | 直接写 `+0x7e0` 处的识别字节 | true用IdentifyPill；false复用官方UnidentifyPill实现的有界C++窄写，不调用Lua | 15B位于ItemPool+7E0；Identify=`0x713AF0`，IsIdentified=`0x832630`。官方已有UnidentifyPill Lua扩展，原“没有API”说法纠正 |
| 诅咒 | 库存后按差异调用 AddCurse／RemoveCurses | **2026-11-12 审阅改为同步 raw**：`Level+0xC` 与 `Game+0x25AC8` 按普通字段随 world 安装，不注入、不挂 getter。~~先装库存，再注入Host effective_curses~~（RE-7 建议，已取代）。把有效值写进 raw 仍然拒绝 | `0x7442A0／0x742B90`含挑战过滤／黑暗／Pickup图形副作用，不调用。有效 getter `0x742450` 计算 `((Game+0x25AC8 \| Level+0xC) \| F) & ~P`：F=`0x6F55C0`（种子效果 56—62 及 stage／挑战／难度条件），P=`0x6F5750`（种子效果 63—70、79），两者只读 Seeds 对象 `Game+0x1BBD8` 与 stage／挑战／难度，均已在 Run 同步清单中。因此原生 getter 在 Client 上算出与 Host 相同的有效值，直接读 raw `Level+0xC` 的消费者也一致 |
| 贪婪波次 | GreedModeWave setter | 合法范围用具名native setter，不调用SpawnGreedModeWave | 字段+18334；getter`0x7074C0`，setter`0x74D700`将值限制至0—12后普通赋值 |
| 商店价格 | Price随Pickup；Keeper按买家重算 | 继续同步Host最终Price和价格Sprite；Client不重算 | `0x80B5A0`按全体玩家／商店历史计算，并非本机买家纯函数；`SetPrice 0x6DF890`还有Random／Spawn分支，不作窄writer |
| 故障道具（TMTRAINER） | 负id的道具定义随生成结果同步 | 同步显示定义／资源，由Client自有配置owner注册并解析负id；不生成effects | Game+669BC vector；负id下标`~id`，`ItemConfig::GetCollectible 0x72A400`读ProceduralItem+0；本地atlas／owner接线仍待实施 |
| 地图拾取图标 | 构建地图缓存时提供 | 地图刷新时提供每房间Host图标摘要；具名缓存adapter待实施。**接法待定**（第 5 节表后）：镜像最小 SavedEntities 条目，或构建后修补缓存 | `0x9635B0`扫描descriptor SavedEntities+74，步长78h，聚合Pickup／Slot；只装DisplayFlags／Refresh不足。挑战32另需Host选定标记 |
| 水流向量 | 直接写 Room+7460／7464 | C++复用SDK GetWaterCurrent指针做有限Vector赋值，等价官方SetWaterCurrent | 官方Room/LuaRoom.cpp setter即直接赋值；不额外运行Lua／Update |
| 进度（成就、计数） | 同步 642 字节成就和 523 个计数 | 只取显示需要的部分，经 `client_hud_achievement_query` 等查询 hook 注入；不写 PGD，不改本机存档 | 按需求清单裁剪 |

注入顺序：world 安装 → 库存与 Player → 诅咒 → 其余注入值 → 显示推进。（2026-11-12 审阅后诅咒改为 raw 字段随 world 安装，此处的“诅咒”一步不再单独存在；顺序其余不变。）getter hook 只在 Client 会话期间返回同步值，其他时候直接调原函数。

### 6.4 事件与状态（RE-8）

**2026-11-12 静态成果已完成**：[入口与载荷语义](client-display-delegation/RE-8/sources-and-payloads.md)、[音效来源与回放](client-display-delegation/RE-8/audio-scopes.md)。下表保留参照方案并补J273结果；线格式／运行接线尚未实施。

| 内容 | 形式 | isaac-lan（J460）的做法 | Host 采集点（J273） | Client 播放点 |
| --- | --- | --- | --- | --- |
| 非循环音效 | 事件 | Play请求带序号、tick、听众、id、volume、FrameDelay、loop、pitch、pan；参照工程用512条缓冲和10tick期限 | `Play 0x907C60`正常逻辑资格／节流后采玩法事件（Q7），不是每次POST回调 | C/S回放scope内按原生ABI播放；FrameDelay是重播间隔，已接受事件不能因本机批量到达再被二次节流。**回放时 FrameDelay 传 0**（2026-11-12 审阅）：节流 `0x907D2F—0x907D5A` 仅在 `now < next` 且 `next ≥ 0` 时跳过，播放后写 `next = now + FrameDelay`；Host 已做过节流，传 0 使回放不被本机节流，也不为后续本地显示音效设下新的间隔 |
| 循环音效 | 状态 | 参照工程按IsPlaying筛集合 | 正常Play／Stop／Adjust／Ambient来源，维护逻辑实例与显式world Stop；不额外调用会清pending的IsPlaying建baseline | 按完整集合协调Play／Stop和当前参数；同id多sample与本机设备失效另行处理，窄adapter待实施 |
| 房间音乐 | 状态 | hook Play／Crossfade，记录模式、id、参数、排队曲目；13B只是参照请求体 | Play=`0x7D8360`、Crossfade=`0x7D8480`、Queue=`0x7D8A80`及queued正常消费 | 按请求版本／播放代次调用；同曲新代次也重播，未变状态不重播；拦截Client房间／Boss初始化自选曲 |
| 道具横幅 | 事件 | hook `HUD::ShowItemText`／`ShowItemTextCustomUTF8`，带本地化键 | 同名函数（zhl 已声明） | 原生同名函数 |
| fortune | 事件 | 参照工程用竖线拼行与字符串表索引 | ShowFortuneText=`0x97BEE0`；正常`0x958CCD`已选类别／语言／索引可保留 | 本地化后按独立长度行构造兼容的MSVC std::list<string>；8B头／32B节点，不把网络数组强转int**，不重新抽选 |
| 全屏演出 | 状态 | 按控制器同步ItemOverlay | Show=`0x9848B0`及实际owner当前值，含主Sprite／MegaMush Player Sprite | **新演出代次**先Show再装state／delay／姿态；相同id重播也识别。Show维护Player引用；Update继续拦截，Render绘制 |
| Boss 开场 | 事件 | 参照工程用8个u32 | StartBossIntro=`0x823D10`，J273原生仅两个u32，事件另带room／frame／资源关联 | 具名BossReplay准备合法roomIndex／资源；音乐（`0x823D8C`）／环境声清除（`0x823D9D`）子生产须先隔离，才可用SDK构建。`0x823DA9` 对 `Game+0x22E90` 的写入不另隔离：它是 HUD `_isVisible`，按 Q8 同步，下一次安装即覆盖（2026-11-12 审阅）。~~完成阶段用RE-9显示拆分~~：Q11 后 mode／counter／Boss 标记随 world 安装，Client 只推进 versus／肖像 ANM2 |
| 切房转场 | 状态＋关联事件 | 见 4.5 | RE-9；Q11 后采集 RoomTransition 显示字段的当前值，事件只承载转场序号与最终目标关联 | 4.5 Q11：随 world 安装，本地捕获 surface 并调原生 Render |

**Q7 音效来源判定**：

- **isaac-lan 不区分来源**：
  - Host 把有玩家的房间里所有 SFX 都记为事件，客机原生发出的音效只按听众过滤。
  - 它的客机拦截了实体 Update，玩法音效本来不会在客机上原生产生，所以可能双响的主要是 HUD 一类显示音效。
- **我们的判定（显示作用域）**：
  - Host 在“Client 实际也会运行的函数”入口进入显示作用域，出口退出。范围是Game::Render全树和6.2已接通的纯显示白名单；RE-9已核RoomTransition::Start／Update为混合根，只对实际保留的显示子段进入作用域，不能整根排除声音。**Q11 后**Client 不运行 Start／Update 的任何部分，二者整体在显示作用域外，转场内全部声音（Start `0x823CE1` 的 608／609、mode3 `0x826D79` 的 sound 1、mode4 `0x826FBA` 的 sound 214）都作为 Host 事件发送。
  - `SFXManager::Play` 采集钩子在显示作用域内不记事件。
  - 作用域与 Client 的放行判定（`ClientEntryDecision`）由同一张白名单生成，两边不会各维护一份。
- **混合路径**：作用域按“Client 实际运行的路径”判定。
  - Client 拦截的子调用，在 Host 上要暂停显示作用域。例如 `RoomTransition::Update` 里调用的 `ChangeRoom` → `Room::Init` 链，在 Client 上被替换，其中发出的音效由 Host 作为事件发送。
  - `ItemOverlay::Update` 在 Client 上不运行，所以它在动画标记处放的音效属于玩法路径，Host 照常作为事件发送。
- **Client 侧**：放行代码发出的 SFX 直接播放。事件回放时带“回放”标志，防止回放调用再被当作本地显示音效处理。
- **RE-8 已核**：SFX Play=`0x907C60`，thiscall六个栈参数、`retn 18h`；FrameDelay是重播间隔。回放标志放在C/S主线程栈式上下文，不给原生API添加参数。HUD252为本地，ItemOverlay::Update四处SFX为Host事件，三个转场声音点按实际拆分归属（Q11 后全部为 Host 事件）。对Game::Render最多5层直接call检查未命中Play，不外推虚调用／DLL／Lua全闭包；实际显示作用域照常覆盖新发现的Render声音。

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
| 7 | Room／Level 保留描述和需要的房间状态，含 Q8 的 Room 显示标量（4.6）、Q9 的当前房间描述 SpawnSeed（第 5 节）和 raw 诅咒 `Level+0xC`（6.3，2026-11-12 审阅）；[房间显示资源设计](room-display-resource-design.md)中能由 Client 冷构建本地产生的显示资源改为本地（按 RE-2 复用显示资源子段，不调用 `Room::Init`） |
| 8 | 改动最大，见第 8 节 |
| 9 | 帧头、记录、块、zstd 和 ACK 保留；显示包络 414—417 退役；文字 422—424 并入事件队列；资源协议 425—433 收窄为实体、Grid 和 costume 等资源引用；新增事件队列段 |
| 10 | 第五项“Run 与显示／音频生产缓存”（版本71）暂停，Run 只保留需要的窄投影，含 Q8 的 Game 显示标量（4.6）、HUD 可见性 `Game+0x22E90`、raw 诅咒 `Game+0x25AC8`、Q11 的 RoomTransition 显示字段（现 `run.json` 的 `room_transition_owner`／`hud_owner` 只是 UNKNOWN 整段占位，改为具名窄字段）和 Q12 的 paused 位／终结分支枚举（Host 在 Update 后计算）；采集窗口和执行器保留，复制计划随布局源收缩；收缩后重新评估 tx-04-21 的瓶颈 |
| 11 | 机制保留；去掉显示结果批次；增加事件队列的序号与确认 |
| 12 | 机制保留；影子状态随域收缩；增加事件队列的接收 |
| 13 | 13.4 的前提改为第 4 节的边界；13.6 由第 3 节取代；新增注入（2.3）；13.5 补入第 9 节的退役项；显示推进根挂在 Manager 边界，按 6.2 的阶段调度（4.1）；Q8 标量的安装与阈值补做（4.6）；Render 前置确认 SpawnSeed 已安装（4.2）；关闭插值时的相机节奏（4.1）；Q8 换算值的重写开销计入 13.3（第 3 节）；Q11 转场安装与 surface 捕获（4.5）；Q12 按终结分支枚举选序列、paused 位接入相机与 `client_entity_draw_paused`、本地菜单阻断的合并；Pα 规则（6.2）；RE-10 的音频 tick（6.2 阶段 6a）；SFX 回放 FrameDelay=0（6.4） |
| 14 | 退役清单补入第 9 节；`client_entity_draw_projectile` 保留作恢复作用域（Q10）；`client_entity_draw_paused` 保留读同步 paused 位（Q12） |
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
| `run.json` | 9／293KB | 只保留第 5 节的 Run 项与注入值，以及 Q8 的 Game 显示标量（4.6；这些字段现已在本文件中） |
| `presentation.json` | 77／596KB | 显示结果 310—407、D0 356 退役；保留项并入事件或注入；新增 6.4 的表现状态（音乐请求、循环音效集合、各控制器 ItemOverlay 状态） |
| `hud-local.json` | 2／16KB | 版本70的纯值模块，凡是原生 HUD 能产生的部分退役；只保留原生 HUD 做不到的项（Q2 之后再评估） |
| `random.json` | 24／81KB | 随机 alpha 357／358 及绘制随机退役；只保留 Host 选出的结果，并入相应域 |
| `wire.json` | 23／148KB | 删去显示包络；新增事件队列段；收窄资源协议 |
| `schema.json`、`version.json` | — | 增加“需求”属性；升到版本72 |

## 9. Hook 与代码退役

### 9.1 Hook

**改判**：

- `game_render`、`room_prerender` 和 `room_render` 从拒绝入口改为随 `Game::Render` 放行。
- `entitylist_render_sort` 保持拒绝：它的唯一原生调用者是已拦截的 EntityList Update，Render 树不调用它（RE-1）。排序由显示推进的自有全排负责。
- `client_menu_render` 不再承担场景绘制。
- `lua_post_render` 放行（Q6）。
- `client_door_update`（`RoomTransition::Update`）保留拦截完整混合根，不拆分（Q11）：转场显示字段随 world 安装，Client 只做本地 surface 捕获、滚动补偿、versus 动画和原生 Render（4.5）。~~显示推进在对应顺序位置调用具名显示拆分（Q5／RE-9）~~，已被 Q11 取代；不能只换ChangeRoom后CallOriginal 的结论不变。
- `client_door_nested`、`client_room_lifecycle` 和 `client_new_room` 按 RE-9 结果判定。

**复审，默认删除**。这些 hook 原本用来把 Host 提供的绘制输入塞给 Client。改为委托后，原生绘制直接读已安装的状态，只保留防副作用或注入确实需要的：

- `client_entity_draw_*`（11 个，不含移入“保留”的 `client_entity_draw_paused` 和 `client_entity_draw_projectile`）；
- `client_entity_shadow_*`（5 个）；
- `client_entity_post_*`（2 个）；
- ~~`client_entity_draw_paused`~~（2026-11-12 按 Q12 移入“保留”）；
- `client_entity_owned_final`；
- `client_player_palette`；
- `client_grid_water_*`（6 个）；
- `client_grid_light_*`（5 个）。

`client_backdrop_wall` 和 `client_backdrop_graphics` 原在此列，2026-11-12 按 RE-2 移入“保留”。`client_entity_draw_projectile` 同日按 RE-5 及用户决定（Q10）移入“保留”，`client_entity_draw_paused` 同日按 Q12 移入“保留”。

**保留**：

- 所有玩法拦截：`client_game_update`、`client_post_update`、`client_room_update`、`client_entity_list`、`client_player_update` 和 `client_odd_helper`。`client_game_update` 在 Client 忙时不会被调用（4.1），仍作为守卫保留。
- Backdrop 构图与生成隔离（RE-2）：它们与旧手工 Render 的职责不同，不随绘制路径退役。
  - `client_backdrop_graphics`（`0x517910`）和 `client_backdrop_wall`（`pre_render_walls` `0x511310`）：冷构建时运行构图的具名作用域。
  - `client_backdrop_effects`（`0x515890`）：装饰实体生成 helper，继续拒绝原函数。
  - byte patch `client_backdrop_effect_1`／`client_backdrop_effect_2`（`0x51168F`／`0x5119E9`）：两处不经 helper 的直接生成分支。
- `client_entity_draw_projectile`（`Projectile::Render`，RVA `0x259460`，Q10）：改作 PositionOffset 的恢复作用域，不再提供绘制输入。原生 Render 在 `0x659484` 减去 Height、`0x6597D1` 加回，浮点减加不保证逐位还原已同步的 `+0x350`。hook 进入时保存这 4 字节，调用原函数一次，正常返回且对象身份未变时精确写回（[RE-5 写集](client-display-delegation/RE-5/write-set.md)）。旧 hook 中提供绘制输入的部分照常退役。
- `client_entity_draw_paused`（`Game::IsPaused` `0x6F92A0`，Q12）：改为返回 Host 同步的 paused 位。原生 53 个调用者（含 `Room::Render` 的 `0x807775`、`0x807CB0`）在 Client 都读这一位，不再提供旧 D0 绘制输入；与本地菜单阻断的合并由第 13 步决定。
- 所有保存、PGD、成就和 Lua 生命周期的隔离 hook。放行的原生代码变多，这些 hook 更不能少。
- `client_hud_achievement_query`，改作注入用。

**诊断 hook**：`client_player_render_body_diagnostic`、`client_player_render_head_diagnostic` 和 `client_minimap_render_diagnostic` 只留在 E2E 构建，或者删除。

**新增**：

- “显示推进”根，挂在 Manager 边界（`ClientManagerBoundary`）的 world 安装之后，按 6.2 的阶段调度（4.1）。它直接调用 RE-1／RE-2 已核的具名入口，不为这些入口新增 hook；FXLayers 更新 `0x56BF40` 等新调用登记为 `ClientEntryDecision` 具名入口；
- `RoomTransition::ChangeRoom` 的拦截点（Client 只作守卫），以及转场启动入口的 Host 采集（RE-9）。Q11 后 Client 不回放 Start；
- 显示推进根内的 Manager 尾部音频 tick：音乐 `0x7D7DA0`、`0x908340`（疑为 SFX），其余尾部调用待 RE-10 分类（6.2 阶段 6a）；
- world 安装事务中的转场 surface 捕获：换房前捕获旧 surface，安装后捕获新 surface（Q11，4.5）；
- world 安装提交点的帧号写入，以及跳过完整 Manager 后的本地计数推进（[RE-3](client-display-delegation/RE-3/apply-contract.md)）；
- 注入 getter（RE-7，6.3）：`Room::GetDevilRoomChance` 和天使占比函数；
- 事件与状态的 Host 采集点（RE-8，6.4）：`SFXManager::Play`、`Music::Play`／`Crossfade`、`HUD::ShowItemText`／`ShowItemTextCustomUTF8`／`ShowFortuneText`、`ItemOverlay::Show`、Boss 开场；
- Host 显示作用域的进出点（Q7），与 Client 放行判定共用同一张白名单；
- Client 拦截 `ItemOverlay::Update`，以及房间初始化中的原生选曲。

最终清单写入 `mod/binding/hooks.json`，hook 常驻，按逻辑启停。

### 9.2 代码退役候选

- **Client 绘制**：
  - `mod/src/game/client/presentation/*`，其中包括 `DrawImpl`、`FrameBorrow`、`FXDrawContext`、手工阴影／光照调用、FX `0x56db20` 和 `ClientRoomWaterPreDraw`；
  - `mod/src/game/client/resources/client_resource_shaders_native.cpp`；
  - `client_scene.cpp` 中的 `RenderPublished`；
  - `client.cpp` 中 `ClientMenuRenderBoundary` 的场景绘制部分；
  - draw 期间逐实体重新锚定 `+0x328`（`client_entity_draw_native.cpp`），以及展示前重新锚定 `Room+0x11F0`（`client_room_native.cpp`）。
  - **例外，迁出保留**：
    - `FullSort`（`client_presentation_native.cpp:258`）：按 GetRenderZ 全排、以索引定序，不用原生 scratch、空表读取或奇偶渐进。迁出作为 renderEL 排序的 owner（6.2），去掉只为手工绘制服务的成员筛选。
    - 绘制前安装 Game 颜色、闪电、暗度当前值的部分（同文件约 785—791 行）：迁入 world 安装（Q8，4.6）。
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
| E28 | 前提改为第 4 节的边界：漂移来自放行的显示更新、Render 及维护 helper，按 RE-5 的写集检测和纠正。Q8 的 Game／Room 显示标量只由安装写入，不被本地二次推进；震屏结束后本地偏移归零（4.6） |
| E30 | 资源表收窄为实体、Grid 和 costume 资源引用；文字改由事件队列验证 |
| E32 | 只核对 Effect 各变体的可见状态 |
| E33 | 改为方案B：静止场景中 Client 的 `Game+0x25A70` 随帧头前进；空帧被接受、确认并推进帧号；不再构造“屏蔽 D0”的空帧 |
| E34 | 改为方案B：锚点直接安装 Host 原值，FrameCount 和 `Room:GetFrameCount()` 与 Host 一致；Effect Timeout 等计数在安装点按 `Game+0x25A70` 还原；加测跳帧与停顿 |
| E35 | 删除（D0 和随机 alpha 退役） |
| 新增 E36 | 显示边界隔离：放行的显示更新和 Render 不产生玩法写入、玩法随机推进、保存或解锁；默认拒绝的入口计数为 0 或已解释。显示推进由 Manager 边界调度：30Hz 批次与相机阶段在 Client 上实际运行，本机 Manager 帧前进；提前返回分支按分支表执行（4.1、6.2）。2026-11-12 审阅追加：Client 按 Host 终结分支枚举选择批次序列，覆盖 GameOver、Leaderboard、StageTransition、ItemOverlay＋暂停组合、Prompt、次级 fade、PauseScreen 各分支；Host 暂停时 `Game::IsPaused` 的 53 个调用者（含 `Room::Render`）在 Client 读到同步的 paused 位，相机按 paused 位与 MINECART 例外运行；本地菜单打开时的合并行为符合第 13 步规定；primary fade 期间 Pα 只在已安装 alpha >0 时调用（Q12、6.2） |
| 新增 E37 | 事件队列：顺序、去重、有界、溢出计数、过期丢弃、重连后不重放旧事件。表现状态：音乐、循环音效和全屏演出在丢包、切房与重连后收敛到 Host。音效按 Q7：显示代码的音效（如 HUD 滑入 252）只在 Client 本地响一次，玩法音效只经事件响一次，两者都不双响也不缺失。道具横幅和 fortune 在两端语言不同时按本地语言显示。2026-11-12 审阅追加：Manager 尾部的音乐／SFX tick（RE-10）在 Client 运行，音乐淡入淡出、Crossfade 和 Queue 能走完，SFX 状态正常前进；回放事件以 FrameDelay=0 播放，同 id 短间隔的 Host 事件不被本机节流吞掉；转场内的 608／609、sound 1、sound 214 各经事件只响一次（Q11） |
| 新增 E38 | 注入值（6.3）：恶魔房／天使房几率、药丸效果与识别、诅咒、贪婪波次、价格（含 Keeper）、故障道具和地图拾取图标与 Host 一致，并在变化后收敛。诅咒按 raw `Level+0xC`／`Game+0x25AC8` 同步后，原生有效 getter 与直接读 raw 的消费者在 Client 上都与 Host 一致，含种子效果与挑战强制诅咒（2026-11-12 审阅） |
| 新增 E39 | 原生切房过渡（4.5）：各种门方向、动画与维度的转场按 Host 参数回放，不重新选路，也不消耗随机；目标未就绪时停在换房点，不显示半套房间；换房后背景、门和相机正确；连续快速切房、转场中断线与恢复不会卡死。**Q11 修订**：转场显示字段随 world 帧原子安装，Client 不运行 Start／Update；旧 surface 在新房间 world 安装前捕获、新 surface 在安装后捕获，mode1 与换房同帧发生时也不缺旧画面；新房间 world 晚到时画面停在旧房与 Host 当前进度上（由原子安装得到，不靠冻结 FSM）；滚动补偿按本地相机计算；Boss 开场 versus／肖像正常推进，结束后 HUD 可见性与 Host 一致；同房转场（animation 11／16）、Hourglass、MINECART 与活跃转场中 resize 均有明确结果 |
| 新增 E40 | 帧号消费者（[RE-4](client-display-delegation/RE-4/handling.md#5-第13步修改与第15步覆盖)）：HUD 周期刷新在 F 连续、29→31、跨多个周期和停在 30 时不漏、不重复重置，type 2 不被覆盖；Room 年龄 29→31、停在 30 和晚加入（年龄已超过 30）时，历史画面捕获不崩溃，当前 SpawnSeed 非零且等于 Host 值（Q9）；同一 M 换 world 后平均位置不读旧目标；StatHUD 冷绑定后属性值正确、不出虚假变化提示；F 相位与 M 相位各按来源工作；F 回退或 world 重建后清除旧的去重与缓存资格；本机关闭插值时相机按原生节奏运行（4.1）；Projectile 每次 Render 后 PositionOffset 逐位不变（Q10） |

E36 中“默认拒绝入口的计数”不含 Q6 放行的回调分派；回调运行本身不算违规，只检查它们没有引起玩法写入或持久副作用。

- 其余条目按原文执行，其中显示相关的内容按“同一权威结果”理解。
- 第 15 步交接清单（15.5）中，按第 8 节退役的布局不再列入。

## 11. 文档修订清单（2026-11-12 已完成）

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

**原生过渡与回调**（2026-11-12 追加）：

18. 回放转场时误走选路分支，消耗随机或选到与 Host 不同的目的地。
19. 换房点等待目标时，转场状态机超时、重入或卡死；断线时停在过渡画面无法退出。
20. 拦截 `ChangeRoom` 后，缺少原生在其中建立的显示状态，例如背景、相机复位或门 Sprite；或者旧房对象没有正确退休。
21. 放行的回调分派里，有本项目自己的 Lua 在 Client 执行玩法写入。
22. 转场事件与 world 目标的关联错位，回放成另一个房间，或在旧房间上执行了转场。

**事件、表现状态与注入**（2026-11-12 追加）：

23. Host 显示作用域与 Client 放行范围不一致：同一音效在 Client 本地响一次、又经事件响一次；或者两边都不播。
24. 显示作用域没有在 Client 拦截的子调用（例如 `ChangeRoom` → `Room::Init`）处暂停，这些路径的音效丢失。
25. 回放事件调用 `SFXManager::Play` 时没有带“回放”标志，被当作本地显示音效处理，或者被再次采集。
26. 循环音效集合与 Client 实际播放状态不一致，切房或重连后留下停不掉的循环音，或该响的不响。
27. Client 房间初始化中的原生选曲没被拦截，覆盖了 Host 的音乐请求。
28. ItemOverlay 状态写入顺序错误（没先调 `Show` 就写状态），Sprite 没加载或演出结束后玩家可见性没恢复。
29. 本地化键缺失或两端字符串表版本不同，横幅或 fortune 显示为空或错字。
30. 天使占比 getter 的 hook 在 Client 会话外仍返回同步值，污染单机游戏；或会话内漏 hook，Client 显示本地算出的错误几率。
31. 诅咒在库存之前安装，被随后的库存安装覆盖或在显示上闪一次。（2026-11-12 审阅后诅咒改为 raw 同步，此条由 55 取代，事实保留。）

**显示推进调度与内联标量**（2026-11-12 追加，RE-1／RE-2 审阅后）：

32. 显示推进根挂在不会被调用的位置（如 `Game::Update` 拦截点），HUD、地图和相机都不推进。
33. 节奏错误：本机 Manager 帧不递增，30Hz 批次永不运行或每帧都运行；相机误用 Host 帧奇偶，或连续调用两个相机入口。
34. Q8 标量既安装又被本地推进；或者安装点漏做阈值动作，例如震屏结束后偏移没有清零，画面一直偏着。
35. 提前返回分支按普通分支外推，暂停或演出期间运行了原生不会运行的显示更新。
36. 退役 `presentation/*` 时连带删掉 `FullSort` 或 Q8 标量安装，renderEL 不再排序，或颜色、暗度停在旧值。
37. 退役 Backdrop hook 时连带删掉构图作用域或生成隔离，冷构建时生成了重复的装饰实体。

**RE-3—RE-5 审阅后追加**（2026-11-12）：

38. 当前房间描述的 SpawnSeed 未安装或为 0，原生 Render 在房间年龄 30 时执行 `int3`；或者换房后仍是旧房间的值。
39. 同步 SpawnSeed 后，Client 上某条读它的生成路径没被拦截，按 Host seed 生成了实体或推进了玩法随机。
40. D1 谓词与完整 SpawnSeed 同时发送且来源不一致，Error 饰品颜色在两者之间跳变。
41. 退役旧绘制 hook 时连带删掉 `client_entity_draw_projectile`，Projectile 的 PositionOffset 每次 Render 后留下浮点残差，被漂移校正反复纠正；或者 hook 在对象身份已变时仍写回旧值。
42. 本机关闭插值时，Client 仍在奇数 Manager 帧运行相机阶段，相机速度是原生的两倍；或者用 Host 的插值设置代替本机设置、按 Mpre 误判绘制相位、遗漏 A／B 保持奇数 M 时仍可绘制的例外。
43. Q8 换算值（Effect Timeout、TimeCounter 等）在帧头每次前进时全量重写，实体多时超出安装预算。

**Q11／Q12／RE-10 及审阅追加**（2026-11-12）：

44. 旧 surface 在新房间 world 安装之后才捕获，或捕获条件依赖看到“mode1＋旧房”的帧（Host 可在同一帧内 mode1→mode3 并换房），转场两侧都是新房间或旧侧为空。
45. 新 surface 在 world 安装尚未完成时捕获，半套房间进入画面；或为捕获新画面调用了完整 ChangeRoom，带出旧房保存、额外 RoomUpdate、选曲或显式 SwapBuffers。
46. Client 本地推进转场 mode／counter／progress，与随后安装的 Host 值叠加，动画加速或回跳；或 Q11 字段与 world 不在同一事务安装，出现“新 mode＋旧房”的混合帧。
47. 滚动补偿 `+0x22C/+0x230` 装了 Host 数值（Host 分辨率与相机），或没有按本地相机重算，入房滑动错位。
48. 具名 BossReplay 调 StartBossIntro 时没有隔离选曲 `0x823D8C`／环境声清除 `0x823D9D`，覆盖 Host 音乐状态；或者 HUD 可见性 `Game+0x22E90` 没有进入同步，本地写 0 后 HUD 一直隐藏。
49. 转场内声音被 Client 本地再发一次（误放行 Start／Update 的子段），或 Host 漏发事件（显示作用域误包含 Start／Update）。
50. Host 的终结分支枚举按 Update 前的状态计算，与它实际执行的分支差一帧；或 Client 本地重算谓词，结果与 Host 不同。
51. paused 位与旧 D0.paused、本地 Manager 状态混用；或 `client_entity_draw_paused` 被当作旧绘制 hook 删除，53 个原生调用者读到 Client 本机的非暂停结果，Host 暂停时 `Room::Render` 仍推进显示。
52. Pα 判定读了未安装或过期的 alpha，alpha 已为 0 时仍调 PostUpdate；或同一批次把 Pα 与终结分支的 P 合并成一次，或多跑。
53. Manager 尾部的音乐／SFX tick 不运行，音乐淡入淡出、Crossfade、Queue 卡住，SFX 内部状态不前进；或在 RE-10 分类完成前整体放行尾部，带出玩法调用。
54. 回放 SFX 没把 FrameDelay 置 0，本机节流吞掉 Host 已接受的事件，或为随后的本地显示音效设下错误的间隔。
55. raw 诅咒只装了 `Level+0xC` 与 `Game+0x25AC8` 中的一个，或者与 Seeds／stage／挑战／难度不在同一事务安装，有效诅咒短暂不一致；或者把 Host 的有效值写进 raw，种子效果被二次合成。
56. 地图拾取图标的 adapter 与原生构建的时序不对：修补被下一次 Refresh 覆盖，或原生构建读到不完整的 SavedEntities 镜像。

**并入的 RE 成果失败清单**（2026-11-12 审阅；原文保留在各成果中）：

57. （RE-1 补遗）GameOver／Leaderboard／StageTransition／Overlay／Prompt 全被压成 paused，运行同一组 U／P／M；WAITING Overlay 与暂停菜单组合判错，或为读取下一状态运行了被拒绝的 `ItemOverlay::Update`。
58. （RE-1 补遗）secondary fade 归零的同帧或跳帧安装后仍阻断，或本机墙钟提前放行。
59. （RE-1 补遗）为补齐 UI 条件复制整段带指针／平台／输入的 owner，或旧 UNKNOWN 占位被误记为正式同步已完成。
60. （RE-9）快速连续转场或重传没有按 session＋转场序号幂等，旧目标挂到新转场；像素化 RenderTarget 没有按 owner 退休，反复转场耗尽 RenderTarget 栈。
61. （RE-9）断线或会话结束时，没有排空在途图形与引用就退休本地阶段，或向已退休的 Game／Room 写旧 mode；重连只重放旧事件，不安装当前状态。
62. （RE-9）活跃转场中 resize（`0x703E40` 重建两张 surface）后继续使用空 surface，或回读已退休的旧 world。
63. （RE-9）特殊动画（同房、Hourglass、镜子、MINECART、复活、Death Certificate）在 Client 上沿动画编号恢复了对应玩法命令；animation 11／16 等不经 ChangeRoom 的转场让 Client 一直等待换房。
64. （RE-8）同 id 多实例 SFX、同曲重播与 queued 音乐、循环空集合与重连处理错误；容量溢出、事件过期、资源未就绪、原生调用失败、设备丢失被当作“已回放”。
65. （RE-8）fortune 文字按网络数组强转构造 list，或在 Client 重新抽选。

## 13. 未决问题

2026-11-12，用户确认全部按推荐执行。

| 编号 | 问题 | 推荐 |
| --- | --- | --- |
| Q1 | 完整性判据 | 改为“Client 需要的字段”需求清单；UNKNOWN 不再阻塞，差异驱动补查 |
| Q2 | 采集实现 | 保留 C++ 复制计划执行器和二进制格式，只收缩计划；不改回 Lua 白名单。`hud-local` 纯值模块在原生 HUD 接通后评估去留 |
| Q3 | 写入方式 | 优先原生 setter 或 REPENTOGON API；没有 API 时才直接写内存；没有存储时才挂 getter |
| Q4 | 外观随机差异 | 接受。绘制随机、粒子等纯外观差异不同步；Host 选出、影响布局或规则的结果仍然同步 |
| Q5（2026-11-12 已定） | 切房方式 | 走原生过渡，按 Host 参数回放（4.5）。取代 2026-11-11“目标就绪后直接切房”。换层另行决定 |
| Q6（2026-11-12 已定） | Client 上的 Lua 回调 | 先放行所有回调分派（2.2）。前提是当前不含第三方 Mod，第三方 Mod 由 Mod 适配方案处理 |
| Q7（2026-11-12 已定） | 音效去重 | Client 放行的原生显示代码发出的音效在本地播放，不进事件队列；Host 按显示作用域排除这些音效，只发玩法路径的音效（2.3、6.4） |
| Q8（2026-11-12 已定） | `Game::Update`／`Room::Update` 内联显示标量 | 同步 Host 当前值，Client 不本地推进；相机、ANM2、FXLayers 以及 HUD／Minimap 自身的动画仍在本地（4.6） |
| Q9（2026-11-12 已定） | 当前房间描述的 SpawnSeed | 同步 Host 当前值，按变化更新；其他 seed 仍留 Host；D1 谓词在第 4 步评估退役（第 5 节表后说明） |
| Q10（2026-11-12 已定） | Projectile PositionOffset 的恢复方式 | 保留 `client_entity_draw_projectile` 作为保存／恢复作用域，不新增 hook（9.1） |
| Q11（2026-11-12 已定） | 转场显示的驱动方式 | 按 Q8 方式同步 RoomTransition 显示字段的 Host 当前值，Client 不运行 Start／Update，也不写自有 FSM 驱动；本地只做旧／新 surface 捕获、滚动补偿、versus 动画与原生 Render。取代 RE-9 的自有显示驱动建议（4.5） |
| Q12（2026-11-12 已定） | 暂停与提前返回分支 | 保留 `client_entity_draw_paused`；Host 每个 world 帧发送 paused 位和按原生优先级算出的终结分支枚举；Pα 由已安装 primary alpha 判定；与本地菜单的合并由第 13 步决定（6.2、9.1） |

**审阅后的待办（2026-11-12）**：

- **RE-10**（新开，待做）：Manager::Update 尾部非 Game 调用的分类（第 6 节表、6.2 阶段 6a）。
- **地图拾取图标的接法**（待定）：镜像最小 SavedEntities 条目，或构建后修补缓存（第 5 节表后、6.3），在第 8／13 步实施前选定。

另一个可选项：是否为“Client 边界改造”单设一次验收点。现行规范只允许第 15 步统一验收，按推荐不单设。如果以后希望提前确认原生 Render 能在 Client 上安全运行，需要用户明确调整规范。

## 14. 推进顺序

1. **快照**（已完成）：按用户要求，分支 `codex/step4-text-dummy` 上的未提交工作（版本60—71）已提交为 `3977b46e`。
2. **文档修订**（已完成）：已按第 11 节修订。
3. **逆向**：RE-1—RE-4 原任务、RE-1 新增补遗、收窄后的 RE-5／RE-6 及 RE-7—RE-9 静态成果已完成（2026-11-12），均未实施／未测试。RE-1 补遗含提前返回分支、需求缺口及本机关闭插值的 Update→Render 节拍；RE-7—RE-9 补齐注入 ABI／限制、音频文字与演出来源、转场混合副作用及具名显示拆分（其中自有显示驱动被 Q11 取代）。相关采集／布局／显示接线仍归第 7／8／10／13 步。**RE-10 待做**（2026-11-12 审阅新开）：Manager 尾部非 Game 调用分类，须在第 13 步接线显示推进根之前完成。RE-6 已并入 RE-5，排除项见 6.1；RE-5 其余写入交第 15 步 E28，不重新要求 Render 全树静态闭合。
4. **布局源**：重标并收缩布局源，升到版本72，生成器只为“Client 需要”的字段出计划。
5. **Client 边界**：实现第 4 节的显示推进、`Game::Render` 包装、帧号方案B 和注入。
6. **Host 采集**：收缩采集，接入事件队列。
7. **退役**：按第 9 节退役，并做产品与 E2E 构建的纯编译。
8. **验收**：第 15 步统一整体验收。

0—14 期间只做资料分析、静态反编译、设计、实现和纯编译，不运行游戏、E2E 或校验脚本。
