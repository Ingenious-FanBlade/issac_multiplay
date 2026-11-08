# 7：Room、Level、RoomSave 与 Exploration

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成 `Room`（0x7898）、`Level`（`Game` 前缀，到 0x1890C）和 `RoomDescriptor`（0xB8）的字节归属。确定房间块、楼层块、描述表和保存集合的状态布局，并覆盖 Exploration 域（访问、显示标志）。

## 输入事实

### Room（SDK 声明 0x7898 字节，37 个具名字段）

| 位置 | 字段 |
| --- | --- |
| 0x04 | `_descriptor` |
| 0x08 | `_roomType` |
| 0x0C／0x10 | 宽、高 |
| 0x24 | 448 个格子指针 |
| 0x724 | 8 个门指针 |
| 0x744 | 8 个门格子序号 |
| 0x76C | `_gridPaths[456]` |
| 0x11EC | `_roomClearDelay` |
| 0x11F8 | Camera 指针 |
| 0x1204 | `_renderScrollOffset` |
| 0x120C | `_redHeartDamage` |
| 0x1218 | EntityList |
| 0x1618 | FXLayers |
| 0x1BB0 | `_bossId` |
| 0x1BC8 | Backdrop（0x5278 字节） |
| 0x7070 | 商店补货 deque |
| 0x7220—0x7298 | 减速时长、Greed 波次计时、水量与水色，水色含四个 KColor |
| 0x72A1 | `_pacifist` |
| 0x72A4／0x72D0 | 地面色、墙面色（ColorMod） |
| 0x7310 | TemporaryEffects |
| 0x7454—0x7460 | 雨、水流 |
| 0x7468 | RailManager |
| 0x7740 | `_lavaIntensity` |
| 0x7744 | 8 个红门轮廓 Effect 指针 |

### Level（Game 前缀）

| 位置 | 字段 |
| --- | --- |
| 0x0／0x4 | stage、stageType |
| 0x8／0x9 | `_bossChallenge`、`_devilRoomDisabled` |
| 0xC | curses |
| 0x14 | `_gridRooms[507]`，每项 RoomDescriptor 0xB8 字节，共约 91 KB |
| 0x16C7C | `_negativeGridRooms[20]` |
| 0x17ADC | `_roomOffset[507]` |
| 0x182CC 起 | 房间数、起始房间 |
| 0x182D4 | `_generationRNG` |
| 0x18300 | Room 指针 |
| 0x18304—0x18314 | 当前／上次房间与维度 |
| 0x18318／0x1831C | 离开门、进入门 |
| 0x1839C | 楼层状态标志 |
| 0x183A4 | Myosotis 拾取保存 |

### RoomDescriptor（SDK 声明 0xB8 字节，30 个字段）

| 位置 | 字段 |
| --- | --- |
| 0x00—0x0C | 格子序号、安全格子序号、列表序号、维度 |
| 0x10／0x14 | `Data`／`OverrideData`：RoomConfig_Room 指针 |
| 0x18—0x3C | 允许的门、8 个门、显示标志 |
| 0x40—0x66 | 访问与清理计数，各类种子 |
| 0x68 | `SavedGridEntities`（vector<GridEntityDesc>） |
| 0x74 | `SavedEntities`（vector<EntitySaveState>） |
| 0x80 | `RestrictedGridIndexes`（std::set<int>） |
| 0x88 | `SavedEffects` |
| 0xAC—0xB4 | 商店序号、Delirium 距离、分组 |

### 公开成员与现有实现

- **公开成员（`stream-catalog.json`）**：room 269 个、level 131 个、room_save 98 个、exploration 99 个。
- **应用目录（`room-grid-application.json`）**：Room 171 个、Level 87 个、RoomDescriptor 44 个，另有 RoomConfig*、Backdrop、Camera、FXParams、ColorModifier、RoomTransition 等。其中，Sprite、Color、LayerState 等表现类型是挂在 Room／Grid 下的嵌套成员。
- **现有 Client 原生写入**：`client_native_world.h`、`client_grids.h` 已覆盖房间构造、格子事务和门绑定，语义保留。

## 设计

### 7.1 字节归属要点

- **配置指针**：`RoomDescriptor.Data`／`OverrideData` 归“资源”，转成稳定配置标识：房间配置的 stage、type、variant 加 subtype，或者现有的配置键。两端都从同一份配置文件加载，Client 按标识找到本地配置对象。
- **保存集合**：`SavedGridEntities`、`SavedEntities`、`SavedEffects`、`RestrictedGridIndexes` 归“容器”。
  - `EntitySaveState` 的元素布局另立条目核对。
  - `std::set<int>` 是红黑树，不能按 vector 读取，需要按树遍历，或改用已核的原生查询，本步核定。
- **种子**：各类种子归状态，Client 生成房间需要它们。只有生成器和掉落使用的 RNG 游标归权威（例如 `_generationRNG`）。
- **房间中的指针**：
  - Room 中的格子、门指针归“引用”，转成 Grid 成员序号，由第 6 步的记录承载。
  - Camera、EntityList、FXLayers、RailManager 等内部对象：按对象内容单独核对布局，指针本身归本地。
  - `_redDoorOutlines` 中的 Effect 指针归“引用”。
- **寻路数据**：`_gridPaths[456]` 可能由格子布局派生，本步要确认 Client 原生逻辑是否会重建它；无法确认的归为状态。
- **房间帧基准**（帧锚点，总计划第 4 节）：
  - `Room+0x11F0` 是 `Room:GetFrameCount()` 的基准帧：GetFrameCount = 全局帧 − 基准（`client_room_native.cpp:132-135`）。它归状态，原值发送，放入 R1。
  - Client 用展示时钟还原 GetFrameCount（第 13 步 13.6）。现有每次展示前重新锚定的做法，改用原值后不再需要。
- **Backdrop 与 Room 尾部**：Backdrop（约 21 KB）和 0x1BC8 之后的大段内容，要先用 J273 划出构造函数与 Update 写入范围，再逐段归类。

### 7.2 状态块（初步划分，本步定稿）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| R0 房间身份 | world | 描述序号、类型、尺寸、配置标识、维度 |
| R1 房间动态 | world | 房间帧基准 `+0x11F0`（帧锚点，原值）、清理延迟、红心伤害标记、Greed 计时、减速、水、雨、熔岩、和平主义 |
| R2 房间表现 | world | 地面色、墙面色、Backdrop 状态、FX 参数、ColorModifier、Camera、滚动偏移 |
| R3 门映射 | world | 8 个门槽的 Grid 成员序号与门格子序号 |
| R4 房间效果 | world | TemporaryEffects、红门轮廓引用、RailManager |
| L0 楼层 | world | stage、stageType、curses、标志、当前与上次房间、维度、门 |
| L1 描述表 | world | 507＋20 个 RoomDescriptor，每个描述一条子记录，按块比较，只发送变化的描述 |
| L2 保存集合 | world | 每个描述的四类保存容器，单独成块，低频 |

描述表数据量大，但大多数时候静止。按描述分块后，memcmp 只在 Host 本地比较约 97 KB 的定长部分；容器部分在描述的访问或清理计数变化时才重读，以控制主线程成本。重读的触发条件和兜底全量周期在第 10 步定稿，不能依赖通知完整性（沿用 `low_frequency_policy` 的规则：没有证实完整的 dirty 覆盖，就做有界全量读取）。

### 7.3 Exploration

访问计数、显示标志和清理计数都在 RoomDescriptor 内，由 L1 承载；Exploration 不再单独成为一个采集来源。地图显示中的派生部分归第 8 步 Presentation。

## 改动范围

- 布局源 `state/room.json`、`state/level.json`。
- `ClientNativeWorld` 的房间与楼层输入改由影子状态生成。
- 房间构造、格子事务、门绑定等原生写入保留。

## 失败方式

- **按指针比较配置**：用配置指针值比较，或直接发送指针。两端地址不同，导致误判或解析错误。
- **set 当 vector 读**：把 `std::set` 当成 vector 读取，导致越界。
- **低频重读遗漏**：描述表容器的重读条件漏掉某类改动，例如 Myosotis、维度切换。
- **误判寻路派生**：`_gridPaths` 被误判为派生，Client 的寻路或绘制与 Host 不同。
- **局部漏读**：Backdrop、FXLayers 等大块内部有指针或缓存混入合并区间。
- **房间切换错位**：切换房间时，旧房间的块被套到新房间的身份上。房间身份变化必须走结构路径。

## 完成内容

- **字节归属与表格**：Room、Level 前缀和 RoomDescriptor 的字节归属全部完成，带证据，单行表由生成器产出。
- **回映射**：room、level、room_save、exploration 的全部公开成员都有回映射。

## 未决问题

- Backdrop、FXLayers、Camera 的内部布局核对工作量。
- `_gridPaths` 是否由 Client 原生逻辑重建。
- `RestrictedGridIndexes` 的读取方式：遍历树，还是使用原生查询。
