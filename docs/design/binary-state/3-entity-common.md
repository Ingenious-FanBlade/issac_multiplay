# 3：Entity 公共部分与主 Sprite

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成 `Entity` 公共部分（0x000—0x40F）的字节归属，以及内嵌主 Sprite（`ANM2 _sprite`，0x048—0x15B）在 Entity 中的用法；划分状态块，产出两张单行表。

## 输入事实

- **SDK 声明**：`Entity.zhl` 声明对象大小 0x410，共 65 个字段。主要位置如下：

| 位置 | 字段 |
| --- | --- |
| 0x020—0x038 | 身份与分类：index、collisionIndex、type、variant、subtype、spawnerType、spawnerVariant |
| 0x048 | 主 Sprite（ANM2，0x114 字节） |
| 0x168—0x177 | flags 与 valid／visible／exists／dead／removedByFactory |
| 0x184—0x192 | 碰撞类 |
| 0x1AC | `_colorParams`（vector） |
| 0x210—0x23C、0x2E0—0x304 | 各类倒计时 |
| 0x33C、0x34C、0x360 | pos、posOffset、velocity |
| 0x3BC—0x3D4 | 五个 EntityPtr |
| 0x3DC | `_dropRNG` |
| 0x3EC | `_initSeed` |
| 0x408 | `_luaBridgeRef` |

- **已核的 Client 写入字段**：`client_entity_fields.inc` 有 57 个公共字段，另有两个特殊字段：
  - `FrameCount`：全局帧 `Game+0x25A70` 减去 `Entity+0x328`（`client_entity_common_native.cpp:51-56`）。
  - `GetWaterClipFlags`：目前经 Lua 写入（同文件 30—37 行）。
- **J273 位置与 SDK 不一致的字段**：
  - `DepthOffset` 在 +0x358，SDK 的 `_depthOffset` +0x2B0 不是它。
  - `HitPoints`／`MaxHitPoints` 在 +0x380／+0x384，`Friction` 在 +0x36C。
  - `Color` 在 +0x1B8，`SplatColor` 在 +0x1E4。
  - `SpriteOffset`／`SpriteScale`／`SpriteRotation` 在 +0xDC／+0xE4／+0xEC，`FlipX` 在 +0x148，均位于内嵌 ANM2 内。
- **REPENTOGON 扩展指针**：`EntityPlus` 把扩展数据指针拆存在 `_collidesWithGrid`（0x190）两侧的 0x18E 和 0x192（SDK 字段名 `_entityExtraDataPtrUpperBytes`／`LowerBytes`）。现有 `flying_override` 来自该扩展（`client_entity_adapters.h:24-25`）。
- **用户采纳的读取规则**：`removedByFactory` +0x177 只在规范第 11 节所述条件下可读。
- **`world_layout.json`**：已有 28 个 Entity 字段，带 J273 反编译证据（例如 `index` +0x20 来自 `Entity Init 0x6A4B1D`）。

## 设计

### 3.1 字节归属要点

- **身份与分类**：type、variant、subtype、spawner 均为状态。这些字段一变化就是形态变化（shape），走成员表的结构路径（第 11 步）。
- **索引**：`_index`、`_collisionIndex` 归本地，不用作网络身份。现有描述中的 `authority_index` 只用于相同 Z 值时的绘制次序，归为换算，语义不变。
- **RNG**：`_dropRNG` 归权威。
- **种子**：`_initSeed` 归状态，Client 绘制和构造需要它（`ClientObjectDescription.seed`）。
- **帧基准**（帧锚点，总计划第 4 节）：
  - `+0x328` 是 FrameCount 的基准帧：FrameCount = 全局帧 − 基准（`client_entity_common_native.cpp:51-56`）。它的值处于 Host 帧空间，归状态，原值发送。
  - FrameCount 本身每帧加 1；如果发送它，每个实体的 E2 都要每帧重发。
  - Client 写回基准原值，再用展示时钟还原 FrameCount（第 13 步 13.6）。
  - 现有 draw 每次绘制前都按 Host 帧重新锚定基准（`client_entity_draw_native.cpp:128,185,195`），改用原值后不再需要。
  - 基准的写入点（Init 或其他路径）待 J273 核对。
- **REPENTOGON 扩展**：0x18E 和 0x192 两段归本地，不发送。扩展数据另设布局，在主线程窗口内经该指针读取；指针的拼合方式以 REPENTOGON 官方源码为证。
- **Lua 引用**：`_luaBridgeRef` 归本地。
- **未覆盖字节**：SDK 没有覆盖的字节必须逐段列为未知，再用 J273 反编译和 Lua 绑定逐段消解。

### 3.2 状态块（初步划分，本步定稿）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| E0 分类 | world | type、variant、subtype、spawner、initSeed |
| E1 运动 | world；Player 为 body | pos、posOffset、velocity、TargetPosition、knockbackDirection、SizeMulti 等 |
| E2 生命与碰撞 | world | flags、valid／visible／exists／dead、HP、mass、size、friction、碰撞类、无敌、帧基准 `+0x328`（帧锚点，原值） |
| E3 状态效果 | world | 各类倒计时、伤害计时、毒／火伤害值、缩小状态 |
| E4 外观 | world；Player 为 body | 主 Sprite 定长外观块、DepthOffset、RenderZOffset、SortingLayer、Color、SplatColor、shadowSize |
| E5 动画 | world；Player 为 body | 主 Sprite 的主动画与叠加动画状态 |
| E6 图层 | world | 主 Sprite 图层变长块 |
| E7 颜色参数 | world | `_colorParams` 变长块 |
| E8 引用 | world | Parent、Child、Target、SpawnerEntity、Minecart |
| E9 扩展 | world | REPENTOGON EntityPlus 扩展数据（布局在本步核定） |

划块原则：同一块内的字段通常一起变化；高频变化的字段（运动、动画）与低频字段分开，使移动中的实体只发送少量字节。

## 改动范围

- 布局源 `state/entity.json` 中的 Entity 条目。
- Client 公共写入表由生成器产出，取代 `client_entity_fields.inc` 的字符串分派；现有原生写入函数的语义保持不变。

## 失败方式

- **用 SDK 名称直接映射**：例如把 +0x2B0 当成 DepthOffset。
- **合并区间混入扩展指针**：区间吞入 0x18E 或 0x192，结果是发出地址，并且每个进程的值都不同。
- **帧基准与时钟不配套**：Client 写回了 Host 的帧基准，却用自己的 `Game+0x25A70` 计算 FrameCount，帧差错误（Client 不运行 Game::Update，该值不代表 Host 时间）；或者改为发送 FrameCount 本身，E2 每帧都要重发。
- **removedByFactory 越权读取**：违背第 11 节的条件读取该字节。
- **引用目标不在成员表**：例如引用指向已离表的实体；这种情况必须报缺口，不能写 Null 冒充。
- **形态变化走了热路径**：形态变化没有经过结构事务，Client 的旧实例接收了新类别的块。

## 完成内容

- **字节归属**：Entity 0x410 字节全部有归属并带证据，未知段逐段列出。
- **状态块**：块划分定稿，两张单行表由生成器产出。
- **回映射**：Entity 的 90 个 `ReplicatedState` 公开成员全部有回映射。

## 未决问题

- `GetWaterClipFlags` 的原生位置，以及能否直接写入。
- EntityPlus 扩展数据中需要同步的成员范围。
