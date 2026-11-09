# 8：Run、Presentation、Random 与 Mod

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成整局（Game 中 Level 之后的部分）、表现（HUD、小地图、音乐音效等）和随机源的字节归属。确定 Mod 域这个唯一例外的二进制承载方式。

## 输入事实

### Game 中的相关位置

`Game` 的 SDK 声明为 0x680E8 字节，79 个具名字段。Level 部分见第 7 步，其余相关位置如下：

| 位置 | 内容 |
| --- | --- |
| 0x18910 | RoomConfig |
| 0x1A740 | ItemPool，0x830 字节 |
| 0x1AF70 | BossPool |
| 0x1B83C／0x1BA78 | RoomTransition／StageTransition |
| 0x1BAA8 | PlayerManager |
| 0x1BB84 | Seeds，0x5C 字节 |
| 0x1C034 | ItemOverlay |
| 0x1D4C8 | GameOver |
| 0x1D9AC | HUD，0x5C90 字节 |
| 0x2363C | PauseScreen |
| 0x25444 | Minimap，0x628 字节 |
| 0x25A70 | 帧计数 `_frameCount`，由帧头 game_frame 承载（见 8.2） |
| 0x25A84 | 屏幕震动偏移 |
| 0x25AC0 | `_gameStateFlags`（u64） |
| 0x25ACC—0x25AEC | 恶魔房、宝箱房、星象房、捐款等计数 |
| 0x25AFC | challenge |
| 0x25B40 | Ambush，0x4C 字节 |
| 0x25BA8 | DailyChallenge |
| 0x25F40 | difficulty |
| 0x25F64 | `_glowingHourglassStates[2]`，约 260 KB，整局状态的两份保存 |
| 0x66920—0x66950 | 三份 ColorModState |
| 0x6699C—0x669A8 | 闪电、眩晕强度 |
| 0x669F0 | BackwardsStages |
| 0x67FD4 | Console |

`Seeds` 包含自定义局标记、开局种子、RNG、14 个楼层种子、玩家初始种子和种子效果数。

### 绘制输入（Game 之外）

Pickup、Familiar 等的原生 Render 还读写 Game 之外的四项全局值。现有实现把它们作为绘制输入复制：

| 位置 | 内容 | 证据 |
| --- | --- | --- |
| `Manager+0x4A8B8` | `_framecount`，u32。Manager 每次 Update 加 1，低位决定是否走 Odd 路 | `Manager.zhl`（[Entity 来源研究](../tx-04-19/entity-source-research.md)第 50 行）；`0x92E8B1` 检查低位（[ST2 笔记](../../research/tx-04-08-st2-agent-notes.md)第 9 行） |
| `Game::IsPaused()` | 暂停判断，VA `0x6F92A0`。Render 的 subframe 为“暂停时 0，否则 `(_framecount+1)%2`” | 同上研究第 52—54 行 |
| VA `0xBD0CA4`（RVA `0x7D0CA4`） | Pickup 绘制用的全局 inline xorshift，4×u32：种子加三个 shift | Pickup Render `0x6E35D0` 读写（[应用合同](../tx-04-19/entity-application-contracts.md)第 130、260 行） |
| VA `0xBD0674`（RVA `0x7D0674`） | Familiar 绘制用的全局 inline xorshift，4×u32 | Familiar Render `0x61E360` 在 `0x61E533` 读、`0x61E5A3` 写回（[Entity 原生静态研究](../tx-04-19/entity-native-static.md)第 73 行） |

现有处理：
- **采集**：Host 在同一普通采集窗口复制 manager_frame、game_frame、subframe、paused 和两份 RNG，不推进它们（`mod/src/game/world_sources/lua_entity.cpp:368-380`）。它们作为 `application_draw` 挂在 Player 记录上，属于 World 层（`mod/src/replica/stream_partition.cpp:36`）。
- **校验**：版本与同窗标记、subframe 与 paused 和 manager_frame 一致、种子不为 0、shift 不超过 31（`mod/src/game/client_entity_class_model.cpp:118-124`）。
- **安装**：draw 的 FrameScope 开始时换入 Host 值（`mod/src/game/client_entity_draw_native.cpp:238-241`），结束时恢复 Client 原值（同文件 136-145 行）。Client 自己的值不会被常驻改写。

### 公开成员（`stream-catalog.json`）

| 域 | 成员数 | 组成 | 归属分布 |
| --- | --- | --- | --- |
| run | 191 | Game 122、Seeds 28、ChallengeParam 28、Ambush 12、DailyChallenge 1 | ReplicatedState 93、AuthorityOnly 87、NotReplicated 11 |
| presentation | 141 | HUD 31、MusicManager 28、Font 16、ColorModifier 12、GenericPrompt 11、SFXManager 10 等 | ReplicatedState 55、AuthorityOnly 70 |
| random | 54 | ItemPool 37、RNG 17 | 全部 AuthorityOnly 或 NotReplicated |
| mod | 12 | — | ReplicatedState 2 |

### 现有处理

- **Random 热路径**：已是 AuthorityOnly，不轮询（`mod/src/game/world_sources/observation.cpp:120-122`）；冷源附加原生随机流。
- **Mod 域**：用具名只读 adapter 注册（`domains/mod/collect.lua`），经有界对象图复制（`domains/mod/graph.lua`）；已有 StageAPI adapter。
- **全局单例**：MusicManager、SFXManager、Font 不在 Game 对象内，是独立的全局单例。

## 设计

### 8.1 Run

- **整局状态**：`_gameStateFlags`、challenge、difficulty、各类计数、Ambush 的波次与激活状态、Seeds 中的种子值，都归状态。
- **RNG**：Seeds 的 `_rng` 和 Ambush 的 `rng` 归权威。
- **大块本地或权威区域**：
  - `_glowingHourglassStates` 是 Host 回溯用的完整保存，归权威，不发送。
  - Console、DebugRenderer、EntityFactory 指针等归本地。
- **布局与块**：Game 不作为整体一个布局，而是按子对象拆成多个布局（Seeds、Ambush、ItemPool 等），每个子对象就是一个记录。Game 自身的零散字段单独组成 Run 块。

### 8.2 Presentation

- **HUD 与小地图**：HUD 和 Minimap 只同步“由状态决定、Client 本地不会自然得到”的部分。
  - 例如：HUD 消息、Minimap 显示状态中由 Host 决定的项。
  - 由玩家、房间状态派生的显示，Client 原生逻辑会重建，归派生。
  - 每一项归为派生，都要给出 Client 重建路径的证据。
- **Sprite资源与颜色**：版本32按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)保留资源选择、实际Image／shader和最终完整ColorMod；不传ColorParams时间线或Host加载维护字节。资源存在性由可空文件资源号表示，等待／失败／成功由Client资源owner维护；路径变化不能替代实际Image变化。主／叠加事件掩码不上线，一次性音画仍由事件通道处理。本步尚未核定的HUD／UI资源owner须核实际绘制消费者，不把Entity四层直接调用的阴性检索外推为全域覆盖；is_playing／anim_frame／playback_speed的官方绘制插值依赖继续保留。
- **ColorModState**：三份 ColorModState 归状态，对应房间色调的渐变。
- **音乐音效**：MusicManager 和 SFXManager 是全局单例，按现有应用目录的职责逐项归属。音效播放是事件，不是状态，沿用现有事件通道，不纳入状态帧。
- **Game 帧计数**：`Game+0x25A70` 由 world 帧头的 game_frame 承载，也就是 Client 的展示时钟（第 13 步 13.6），不进入 Run 块，也不进入下面的绘制输入块。

#### 8.2.1 绘制输入块 D0

全局只有一份，作为 Presentation 记录的一个块随 world 帧发送，取代现有挂在每个 Player 上的 `application_draw`。状态布局共 37 字节：

| +0 u32 manager_frame【状态】Host 的 `Manager._framecount` | +4 u8 paused【换算】Host `Game::IsPaused()` 的结果，0 或 1 | +5 u32[4] pickup_random【状态】Pickup 绘制 xorshift：种子、三个 shift | +21 u32[4] familiar_random【状态】Familiar 绘制 xorshift：种子、三个 shift |
| --- | --- | --- | --- |

- **subframe 不发送**：归派生，Client 按“paused 为 1 时取 0，否则 `(manager_frame+1)%2`”算出。
- **manager_frame 不套帧锚点**：Manager 每次 Update 加 1，Game 只在 `_framecount` 低位为 0 的 Manager 帧推进（ST2 笔记第 9 行），暂停时 game_frame 还会停住，两者没有固定差值。它全局只有 4 字节，随帧发送的代价可以忽略。因此 D0 几乎每帧都在变化，静止场景的帧通常也带着 D0，空帧（第 11 步 11.3）只在 D0 也不变时出现。
- **两份绘制 RNG 归状态**：它们只被 Render 读写，不参与玩法，是 8.3 中“RNG 归权威”的例外。Client 用 Host 的值，Pickup、Familiar 的绘制效果才与 Host 一致。
- **同窗**：D0 与帧头 game_frame 必须来自同一普通采集窗口，沿用现有 `same-ordinary-window` 合同。
- **安装**：不常驻写入 Client 的全局值。展示期间随展示时钟一起借用（第 13 步 13.6）：进入时换入，结束时恢复；Render 推进后的 RNG 值随恢复丢弃，下一帧再使用 Host 的新值。
- **校验**：种子不为 0，shift 不超过 31，paused 只取 0 或 1。这些取值范围写进布局源，不合格时按第 9 步 9.8“字段取值合法”整帧拒绝。

### 8.3 Random

ItemPool 和 RNG 整体归权威，不进入状态帧。如果 Client 显示需要其中某个结果（例如道具池剩余状态影响 HUD），按换算处理：发送结果，不发送 RNG。唯一的例外是 8.2 中只用于绘制的两份全局 RNG。

### 8.4 Mod 域例外

第三方 Mod 的状态是 Lua 表，没有原生布局，无法按字节归属。处理方式：

- **采集**：保留具名只读 adapter，只在 ordinary 窗口以低频读取。这是正式同步中唯一保留的 Lua 读取。
- **编码**：不再生成 JSON，改用自描述二进制编码：
  - 每个值写 `u8 类型标记` 加值；类型包括 nil、bool、i32、f64、字符串、表、实体引用、Grid 引用、Vector、Color、KColor。
  - 表写成 `u16 项数` 加键值对。
  - 实体和 Grid 引用写成成员序号。
- **分块与比较**：每个 adapter 的输出作为一个块，用 memcmp 判断变化。
- **预算与失败**：沿用 `graph.lua` 的节点、项数、字节和深度预算。超出预算时报具名缺口，不截断后冒充完整。
- **完整性口径**：Mod 域不计入“按字节归属完整”，只按 adapter 声明的 schema 判断完整。

## 改动范围

- 布局源 `state/run.json`、`state/presentation.json`，其中包括绘制输入块 D0。
- 删除 Player 记录上的 `application_draw`，绘制安装改读影子状态中的 D0（第 13 步 13.6）。
- Mod 域的二进制编码器，Host 端为 C++ 读 Lua 表，Client 端负责解码。
- Run 和 Presentation 的 Client 写入函数，改为由影子状态输入。

## 失败方式

- **回溯状态外泄**：`_glowingHourglassStates` 被误归为状态，单帧发送 260 KB；或者 Client 写入后破坏本地回溯。
- **派生判断不实**：HUD 中的项被判为派生，Client 实际不会重建，显示缺失。
- **Mod 表循环引用**：Mod 表中的循环和共享节点重复编码，或者无限递归。
- **Mod 读取超时**：Mod adapter 抛错或耗时过长，挤占主线程预算。adapter 读取要单独计时，超时降频并报缺口。
- **全局单例时机错误**：MusicManager 等全局单例在会话外或主线程窗口外被读取。
- **绘制输入错窗**：D0 与帧头 game_frame 来自不同的采集窗口，subframe 与帧计数相位错开，Pickup、Familiar 的绘制效果与 Host 不一致。
- **绘制输入常驻写入**：D0 直接写进 Client 的全局值而不是展示期间借用，或者展示结束后没有恢复，污染 Client 本地的 Render 状态。
- **绘制 RNG 误归权威**：两份绘制 RNG 被当作玩法 RNG 不发送，Client 用自己的值绘制，效果与 Host 不同。

## 完成内容

- **字节归属与表格**：Run、Presentation 的字节归属完成，带证据；单行表由生成器产出。
- **回映射**：四个域的公开成员全部有回映射：
  - run 的 93 个、presentation 的 55 个 `ReplicatedState` 成员，各自映射到布局字段、某组字段的派生，或者说明为何归 Host 权威或不适用；
  - run 的 87 个、presentation 的 70 个 `AuthorityOnly` 成员，以及 random 的 54 个成员，确认确实落在权威或本地字节上，没有漏判为状态；
  - mod 的 2 个 `ReplicatedState` 成员映射到具名 adapter 的 schema（8.4）。
- **Mod 编码**：Mod 域二进制编码的单行表在第 9 步画出。

## 未决问题

- HUD（0x5C90）和 Minimap 的核对范围，即哪些部分必须同步。
- Mod 域是否需要提供注册新 adapter 的公开接口，还是只保留现有具名 adapter。

**版本39 Player／HUD装备依赖**：[装备与Plus复核](player-equipment-plus-synchronization-review.md)核定PlayerHUD::RenderActiveItem835B40直接／间接使用四主动槽六值、TemporaryEffects分类／count、ItemConfig、Player角色／库存及Game challenge；还在836093读首个slot0／1 Weapon的type12分支。Player758850的进度条依真实slot7／slot6公式与各内嵌Sprite，角色27另读704效果cooldown及配置cooldown；不得把有最终costume当作查询已经可用。HUD资源／image/bookImage／flash／布局等未核owner仍须本步准备，不放行832C10整个PlayerHUD Update刷新缓存。官方1072／1073等有效结果及HUD回调按声明Mod适配／表现隔离承接，不能在Client调用玩法Lua补齐。Plus公共覆盖由Scene415350的水面／反射分派实际消费，主体Room／Player依赖仍在；本轮不宣称完整HUD实现或测试通过。
