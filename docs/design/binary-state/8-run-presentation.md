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
| 0x25A70 | 帧计数 |
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
- **ColorModState**：三份 ColorModState 归状态，对应房间色调的渐变。
- **音乐音效**：MusicManager 和 SFXManager 是全局单例，按现有应用目录的职责逐项归属。音效播放是事件，不是状态，沿用现有事件通道，不纳入状态帧。

### 8.3 Random

ItemPool 和 RNG 整体归权威，不进入状态帧。如果 Client 显示需要其中某个结果（例如道具池剩余状态影响 HUD），按换算处理：发送结果，不发送 RNG。

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

- 布局源 `state/run.json`、`state/presentation.json`。
- Mod 域的二进制编码器，Host 端为 C++ 读 Lua 表，Client 端负责解码。
- Run 和 Presentation 的 Client 写入函数，改为由影子状态输入。

## 失败方式

- **回溯状态外泄**：`_glowingHourglassStates` 被误归为状态，单帧发送 260 KB；或者 Client 写入后破坏本地回溯。
- **派生判断不实**：HUD 中的项被判为派生，Client 实际不会重建，显示缺失。
- **Mod 表循环引用**：Mod 表中的循环和共享节点重复编码，或者无限递归。
- **Mod 读取超时**：Mod adapter 抛错或耗时过长，挤占主线程预算。adapter 读取要单独计时，超时降频并报缺口。
- **全局单例时机错误**：MusicManager 等全局单例在会话外或主线程窗口外被读取。

## 完成内容

- **字节归属与表格**：Run、Presentation 的字节归属完成，带证据；单行表由生成器产出。
- **回映射**：Random 全部有回映射。
- **Mod 编码**：Mod 域二进制编码的单行表在第 9 步画出。

## 未决问题

- HUD（0x5C90）和 Minimap 的核对范围，即哪些部分必须同步。
- Mod 域是否需要提供注册新 adapter 的公开接口，还是只保留现有具名 adapter。
