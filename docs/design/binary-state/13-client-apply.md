# 13：Client 原生应用

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

主线程把应用批次写入原生对象，分为两条路径：

- **结构路径**：成员加入、形态变化、离开，走场景结构事务。
- **热路径**：变化块按生成的分派表写入。

写入完成后回复 `AppliedAck`，并检测、校正 Client 本地状态与影子状态之间的漂移。动态安装的主线程耗时 p95 ≤2 ms、p99 ≤4 ms。

## 输入事实

### 现有原生写入层（保留）

- **场景结构**：`ClientNativeWorld{Prepare,Refresh,Finalize,CheckCommit,Retire}`（`client_native_world.h:68-81`）。
- **Grid**：`ClientNativeGrids{Execute,BindDoors,Retire}`（`client_grid_native.h`）。
- **Entity 与 Player**：
  - Entity 公共字段的原生写入（`client_entity_common_native.cpp`）；
  - Entity 子类的写入（`client_entity_adapters*.cpp`）；
  - Player 的最终量、资源、角色和备份写入（tx-04-20 I11—I27）。

### 描述结构

以下描述结构有一部分字段要靠 JSON 目标填充：

- `ClientObjectDescription`（`client_objects_native.h:42-66`）
- `ClientGridDescription`
- `ClientPlayerDescription`
- `ClientNativeSceneDescription`

### 字段写入现状

公共字段按字符串成员名分派（`client_entity_common_native.cpp:43-50`）；`GetWaterClipFlags` 经 Lua 写入。

## 设计

### 13.1 每个字段的写入方式

布局源为每个状态字段声明一种写入方式，生成器产出分派表：

| 写入方式 | 含义 |
| --- | --- |
| `direct` | 按原生偏移把状态字节原样写回，即复制计划的逆操作。相邻的 `direct` 字段合并成一次 memcpy |
| `writer:<名称>` | 调用具名的原生写入函数。例如 Sprite 加载、`SetAnimation`、容器重建、引用解析、Effect Timeout 由到期帧还原（`effect_timeout`，第 4 步 4.1.5），以及现有的已核写入路径 |
| `construct` | 只在构造时使用，热路径变化时触发形态事务。例如 type、variant |
| `derive` | 不写入，由 Client 原生逻辑重建，需要第 3—8 步给出证据 |

- **`direct` 需要证据**：一个字段只有在证实“直接写内存不会破坏不变量”之后，才能用 `direct`。证据可以是 J273 中对该字段的写入点只有普通赋值，或者现有原生写入正是直接写入。没有证据的字段一律用 `writer`。
- **引用和资源**：引用字段按成员序号查 Client 本地实例表，得到原生地址后写入。目标还没构造时，该块延后到目标构造之后再写，按依赖排序。资源字段按资源序号找到本地路径，交给现有的资源 owner 加载。

### 13.2 结构路径

- **加入**：用类型化视图填写描述结构，取代 JSON 字段，交给现有的场景结构事务构造。
- **形态变化**：退休旧实例，按新形态构造新实例。
- **离开**：按现有的退休流程处理。
- **事务边界**：沿用 `ClientScene` 的结构事务和提交边界，不在事务中途写热块。

### 13.3 热路径

- **写入顺序**：按（成员, 块）调用分派表；每个块内按字段顺序执行。
- **出错处理**：某个块写入失败（例如 writer 返回失败），该成员标为“需要重建”，等下一批次重试；批次里的其他成员照常写入。
- **整批成功才确认**：只有整个批次成功，才回复 `AppliedAck(world_version, body_version, member_epoch, resource_epoch)`。部分失败时回复上一个完整版本，让 Host 继续携带失败的块。

### 13.4 漂移校正

Client 的原生对象可能偏离影子状态。Client 不运行原生 Update，但 Render 和维护 helper 仍会写部分字段，例如维护姿态时推进动画，或者 base draw 写屏幕坐标缓存。Host 只在块变化时发送，如果块没有变化，Client 的偏离就不会被纠正。

- **检测方式**：Client 用与 Host 相同的复制计划，读取本地原生对象，再与影子状态逐块 memcmp。不一致的块加入下一批次重写。
- **执行频率**：轮转执行，每帧只检查一部分成员，总耗时计入安装预算。
- **前提（已定）**：Client 不运行原生 Update。这是 tx-04-17 的隔离设计：现有 Client 拦截 Game、Room、EntityList、Player、PostUpdate 等入口（`client.cpp:28,142-146`），本地不会按玩法推进任何字段。
- **待核问题**：可能造成偏离的只剩 Client 仍在运行的原生路径，即 Render 和维护 helper。例如 base draw `0x6AC4E0` 会写 Entity `+0x1A4/+0x1A8`（tx-04-17 R3）。本步逐类核对这些路径会写哪些字段，据此决定哪些块需要每帧校正，哪些只需要轮转检查。

### 13.5 退役的部分

- `client_entity_fields.inc` 中按字符串匹配的分派，改为生成的索引分派表。
- 描述结构中的 `std::vector<ClientGridField> fields` 等字符串字段。
- `GetWaterClipFlags` 改为原生写入；无法原生写入时，在第 3 步给出证据并归为派生或换算。
- Player 记录上的 `application_draw` 及其 JSON 校验（`client_entity_class_model.cpp:118-124`），改读影子状态中的绘制输入块 D0（13.6）。
- draw 期间逐实体重新锚定 `+0x328`（`client_entity_draw_native.cpp:128,185,195`），以及每次展示前重新锚定 `Room+0x11F0`（`client_room_native.cpp:132-135`）。它们由 13.6 的展示时钟取代。

### 13.6 展示时钟与帧锚点

- **定义**：展示时钟是 Client 最近已应用 world 帧头中的 game_frame（第 9 步 9.3）。
  - 它只在装好新的 world 帧后前进。Host 在静止场景中也发送空帧（第 11 步 11.3），所以时钟会照常推进。
  - Client 自己的 `Game+0x25A70` 不代表 Host 时间，不能作为换算基准。
- **展示期间**：沿用现有借用方式（`client_presentation_native.cpp:826-829`、`client_entity_draw_native.cpp:238-241`），进入展示时换入下列 Host 值，结束后恢复 Client 原值（同文件 136-145 行）：
  - `Game._frameCount`（`+0x25A70`）← 展示时钟；
  - `Manager._framecount`（`+0x4A8B8`）、Pickup 绘制 RNG（VA `0xBD0CA4`）、Familiar 绘制 RNG（VA `0xBD0674`）← 影子状态中的绘制输入块 D0（第 8 步 8.2.1）。D0 与展示时钟来自同一个 world 帧。
- **帧锚点字段**：`Entity+0x328`、`Room+0x11F0` 的基准按原值写回，不再逐次重新锚定。展示期间，FrameCount 和 `Room:GetFrameCount()` 由原生 getter 用"时钟 − 基准"自然得出。
- **Effect Timeout**：每装好一个新的 world 帧，按新的时钟把全部 Effect 的 Timeout 重写为"到期帧 − 时钟"（`effect_timeout`）。
- **对照与校正**：E2E 对照（第 15 步）和漂移校正读取这些字段时，用同一个时钟换算。

## 改动范围

- **新增**：分派表、引用依赖排序、漂移检测。
- **修改**：`ClientScene`、`ClientObjects`、`ClientGrids`、`ClientPlayers` 的输入，改用应用批次和类型化视图。
- **保留**：原生写入函数、结构事务和退休流程。

## 失败方式

- **直接写入破坏不变量**：`direct` 写入绕过了原生 setter 维护的不变量，例如缓存、索引或派生值。
- **引用目标未构造**：引用指向的目标还没构造就写入，写成 Null 或悬空地址。
- **部分失败仍确认**：部分失败后仍然回复了新版本，Host 停止携带失败的块。
- **校正形成振荡**：漂移校正与本地自然推进的字段来回改写。这类字段应改为每帧覆盖，或者归为派生。
- **漂移检查超预算**：检查本身超出主线程预算。
- **结构事务中写热块**：在结构事务中途写热块，对象处于半构造状态。
- **展示时钟错位**：
  - 展示期间没有换入展示时钟，或者用 Client 自己的 `Game+0x25A70` 换算，导致 FrameCount、`Room:GetFrameCount()` 和 Timeout 错位；
  - 装好新帧后没有重写 Timeout。
- **绘制输入未借用或未恢复**：展示期间没有换入 D0，Pickup、Familiar 用 Client 本地的 RNG 和 Manager 帧绘制；或者展示结束后没有恢复，Client 的全局值被常驻改写。

## 完成内容

- **实现与编译**：分派表、结构路径、热路径、漂移校正实现完成，纯编译通过。
- **代码约束**：旧的字符串分派已删除；所有新函数和数据结构带中文注释。

## 未决问题

- 哪些字段可以用 `direct`，需要第 3—8 步逐字段给出证据。
- Client 仍在运行的原生路径（Render、维护 helper）会改写哪些状态字段。这决定哪些块需要漂移校正。
