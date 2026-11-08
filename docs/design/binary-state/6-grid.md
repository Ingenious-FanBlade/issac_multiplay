# 6：GridEntity

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成 `GridEntity` 公共部分（0x000—0x153）和各 Grid 子类的字节归属，确定 Grid 在房间中的枚举方式、记录布局和状态块，覆盖门、TimerPlate、Fire 等现有专用内容。

## 输入事实

### SDK 声明

`GridEntity.zhl` 声明对象大小 0x154，具名字段只有 6 个：

| 位置 | 字段 |
| --- | --- |
| 0x04 | `_desc`，GridEntityDesc，0x20 字节 |
| 0x24 | `_gridIndex` |
| 0x28 | `_spawnFrame` |
| 0x2C | `_rng` |
| 0x3C | `_collisionClass` |
| 0x40 | `_sprite`，ANM2，到 0x153 |

`GridEntityDesc`（0x20 字节）有 8 个字段：

| 位置 | 字段 |
| --- | --- |
| 0x00 | type |
| 0x04 | variant |
| 0x08 | state |
| 0x0C | spawnCount |
| 0x10 | spawnSeed |
| 0x14 | varData |
| 0x18 | initialized（bool） |
| 0x1C | variableSeed |

### `world_layout.json` 中的已核字段

Grid 有 15 个字段已带 J273 证据：desc 八项、grid_index、spawn_frame、rng 四个字、collision_class。position 是换算项，由格子序号推出：步长 40，原点 (40,120)，getter RVA 0x4087A0。

### 子类大小（SDK 声明与 J273 工厂分配）

J273 的 `GridFactory` 按类型分配对象，分配请求见 [Grid 种类静态确认](../../research/tx-04-08-grid-kinds-static-confirmation.md)第 40—44 行，已登记在 `world_layout.json` 各 Grid 类的 `factory_allocation_size`：

| SDK 声明 | J273 工厂分配 | 子类 |
| --- | --- | --- |
| 0x154 | 0x154 | Decoration、Gravity、Stairs、Teleporter、Wall、Web |
| 0x154 | **0x158** | TrapDoor |
| 0x154 | **0x390** | Fire |
| 0x158 | 0x158 | Lock、Pit、Spikes、Statue、TNT（TNT 有 `_frameCount` 0x154） |
| 0x174 | **0x19C** | Poop |
| 0x198 | 0x198 | Rock |
| 0x27C | 0x27C | PressurePlate |
| 0x2EC | **0x2F0** | Door：`_extraSprite` ANM2 0x168、`_targetRoomIdx` 0x280、`_previousState` 0x284、`_previousVariant` 0x288 |

- **四类不一致**：TrapDoor、Fire、Poop、Door 的分配大于 SDK 声明。Fire 多出 0x23C 字节，Poop 多出 0x28 字节，都不是对齐能解释的差距。
- **分配不等于 sizeof**：分配请求只是对象大小的上界。研究已明确“不把分配长度当作 sizeof”（同文第 43—44 行），新类在 `world_layout.json` 中的 `object_size` 仍为 null。本步按工厂分配划定对象范围；SDK 声明之外、分配之内的字节先列为未知，用 J273 构造函数核出写入范围后再归类。
- **RTTI**：`world_layout.json` 记录了 18 个 Grid 类的 RTTI，以及 `grid_types` 中 type 到 RTTI 的映射。

### 房间中的位置

- **格子槽**：`Room+0x24`，共 448 个指针，最大序号 447。
- **门槽**：`Room+0x724`，共 8 个。
- **门所在格子**：`Room+0x744`，8 个格子序号。
- **构造时清零**：Room 构造函数（RVA 0x3DF8F0）清零 `Room+0x24` 起的 0x720 字节。

### 公开成员与现有 Client 描述

- **公开成员**：`room-grid-application.json` 中，`isaac.cs.grid` 有 119 个成员，其中 GridEntity 45 个、Door 32 个、Rock 17 个、Desc 8 个，其余子类各 1—6 个。
- **Client 描述**：`ClientGridDescription`（`mod/src/game/client_grid_native.h:23-42`）包含：
  - desc 八项、collision、water_clip、spawn_frame；
  - 两个位置；
  - 用字符串命名的 `fields`；
  - 角色化 Sprite：主、Door extra、TimerPlate、Fire 两个专用；
  - BigRock 帧、Fire 计时与样式、Door 动画模式与灯光标志；
  - ember 与 rubble 两个容器。
- **门绑定**：`ClientGridDoorBinding` 按 8 个门槽分别保存候选、允许位、实际主体和路由。

## 设计

### 6.1 字节归属要点

- **GridEntityDesc**：desc 八项都是状态。其中 `initialized` 与 Client 本地构造完成没有互推关系（沿用 `client_grid_native.h:22` 的规则），Client 照原值写回。
- **RNG**：`_rng` 归权威，沿用 RNG 不进入 Client 的规则。如果 Client 绘制或查询需要它，在本步给出具体证据后改归状态。
- **主 Sprite**：`_sprite` 按 ANM2 布局处理（第 2 步）；Door `_extraSprite` 同样处理。
- **门目标**：`_targetRoomIdx` 是房间描述序号，归状态（不是指针）。
- **子类区**：子类区按第 4 步的方法核对，先用 J273 反编译划出构造函数、Init 和 Update 写入的范围。Poop、Rock、PressurePlate、Door 的子类区较大，是本步的重点。
- **SDK 声明之外的字节**：Door 0x2EC—0x2EF、TrapDoor 0x154—0x157、Fire 0x154—0x38F、Poop 0x174—0x19B 不能依据 SDK 认定为对象外部，必须按 J273 工厂分配和构造函数核对，在核清之前列为未知。Fire 多出的 0x23C 字节可能正是现有 Fire 专用内容（计时、样式、ember 容器）的原生存放处，本步优先核对。

### 6.2 枚举与记录

- **记录归属**：每个非空格子槽产生一条记录，记录序号即成员序号。
- **身份**：沿用现有 Grid 身份，即房间身份、格子序号和类别。
- **门**：门和装饰可能出现在同一格子序号上，按现有 door_aliases 规则区分。门槽映射作为房间块的一部分发送（第 7 步），不放进 Grid 记录。
- **形态变化**：type 或 RTTI 变化即为形态变化，走结构路径（第 11 步）。
- **被举起的格子**：Effect 136 举起的格子不占格子槽，也不是活动 Grid。它的描述存放在 Effect +0x420 起的 32 字节中，由第 4 步 4.1 的 FX2（GridDesc 臂）承载。Grid 域不再另发一份；现有 room-grid 目标中的 `grid/lifted_descriptors`（`mod/src/replica/room_grid_target.cpp:239-255`）并入 Effect 记录。

### 6.3 状态块（初步划分，本步定稿）

| 块 | 内容 |
| --- | --- |
| G0 描述 | desc 八项、grid_index、spawn_frame、collision_class |
| G1 外观 | 主 Sprite 定长外观块 |
| G2 动画 | 主 Sprite 动画状态 |
| G3 图层 | 主 Sprite 图层变长块 |
| G4 子类 | 按子类划分。例如 Door 的目标与前状态，以及 extraSprite；TNT 帧数；Poop、Rock、PressurePlate 的子类区 |
| G5 附加 | Fire 计时与样式、ember 容器、rubble 容器、TimerPlate Sprite 等现有专用内容 |

## 改动范围

- 布局源 `state/grid.json`。
- `ClientGridDescription` 改由影子状态生成，取代 `std::vector<ClientGridField> fields` 的字符串字段。
- 原生写入函数 `ClientNativeGridsExecute`、`ClientNativeGridsBindDoors` 保留。

## 失败方式

- **按 SDK 大小划范围**：按 SDK 声明处理 Door、TrapDoor、Fire、Poop，漏掉 J273 实际分配的尾部字节；或者反过来，把分配长度直接当作 sizeof，把尾部未写字节当成状态。
- **同格冲突**：门和装饰在同一格子序号上，记录身份冲突。
- **重复构造**：形态变化没有走结构事务，或复用旧实例时重复调用构造函数。
- **RNG 误判**：RNG 被误判为 Client 所需状态，导致权威状态外泄；或者反过来，Client 实际需要却被判为权威。
- **槽位越界**：格子槽读取越过 447，或者把 `_doors[8]` 当成独立的成员来源，造成重复。
- **举起的格子重复**：格子描述在 Grid 域和 Effect FX2 中各发一份，而且内容不一致。

## 完成内容

- **字节归属与表格**：Grid 公共部分和 18 个子类的字节归属全部完成，带证据，两张单行表由生成器产出。
- **回映射**：119 个 Grid 公开成员全部有回映射。

## 未决问题

- Door、TrapDoor、Fire、Poop 在 J273 中的实际对象大小，以及 SDK 声明之外字节的归属；Fire 尾部 0x23C 字节与现有 Fire 专用内容的对应关系。
- Grid `_rng` 是否被 Client 的绘制或查询使用。
