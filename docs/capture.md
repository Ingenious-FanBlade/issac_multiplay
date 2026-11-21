# 采样

采样是 Host 把原生游戏对象读成二进制状态的过程。本文说明三件事：布局怎样定义、生成器产出什么，以及运行时的读取流程和安全规则。读取结果交给后台以后如何编码和发送，见[同步](synchronization.md)；Host 主线程的调度时序见 [Host 端设计](host.md)。

## 1. 设计要点

- **原生直读**：主线程在 C++ 侧按复制计划读取对象内存，不调用 Lua getter，也不构造 JSON 树。
- **布局驱动**：每个原生类型都有一份布局源，写明每段字节的归属。复制计划、编解码表、客机校验和写入表都由同一份布局源生成，不在代码里另写字段表。
- **按需采集**：只采集客机需要的字段。采集与否由需求清单决定，布局源中的字节归属只用于定位和说明语义。
- **主线程只复制**：主线程只做 memcpy、换算、身份登记和容器元素复制，写入预分配缓冲。成员索引、块比较和编码全部在后台完成。
- **按根隔离失败**：单个对象读坏只影响它自己，本帧对它标记“沿用”；全局根（Game、Room、Level、Run）读坏才丢掉整帧。

## 2. 布局源

### 2.1 文件

| 文件 | 内容 |
| --- | --- |
| `mod/binding/state/version.json` | 布局版本号、域清单、公开成员目录（用于回映射核对）、资源表 |
| `mod/binding/state/schema.json` | 布局源的 JSON Schema |
| `mod/binding/state/base.json` | 基础值类型：Vector、ColorMod、KColor、ANM2、LayerState 等 |
| `entity.json`、`entity-classes.json`、`entity-effect.json`、`entity-text-dummy.json` | Entity 公共部分与各子类 |
| `player.json` | EntityPlayer |
| `grid.json`、`grid-classes.json` | GridEntity 及 17 个子类 |
| `room.json`、`level.json`、`run.json` | 房间、楼层、整局 |
| `presentation.json`、`display.json`、`overlay-display.json`、`hud-local.json` | 显示相关的同步值 |
| `random.json` | RNG 的权威归属与必要绘制输入 |
| `wire.json` | 线格式类型、协议常量、资源协议、预算 |
| `demand.json` | 需求清单 |
| `mod/binding/world_layout.json` | 枚举根、容器、池等布局常量（Game、Room、EntityList 的偏移等） |

### 2.2 类型条目

每个原生类型是 `types` 数组中的一个条目，主要字段：

| 字段 | 含义 |
| --- | --- |
| `type`、`layout` | 类型名和布局编号。布局编号写进线上记录头，客机据此选解码表 |
| `native_size`、`parent`、`rtti` | J273 中的对象大小、父类型、RTTI 名与 vtable RVA。运行时按 vtable 判定对象类型 |
| `ranges` | 按偏移排列的字节段，从 0 覆盖到对象末尾，不留空隙 |
| `blocks` | 把同步字段分成若干块。块是变化检测和增量发送的单位 |
| `conversions`、`validators` | 换算规则（如剩余帧换成到期帧）和客机侧的取值校验 |
| `members`、`public_owners`、`catalog_rows` | 与公开成员目录的回映射，用来检查有没有漏掉需要同步的成员 |
| `contexts`、`companions`、`synthetic_fields` | 不在对象本体里、但随它一起采集的上下文值 |
| `record_limit`、`body_limit`、`extent` | 单条记录的大小上限、Body 流上限、变长部分范围 |

### 2.3 字节段

`ranges` 中每一段写明 `offset`、`size`、`name`、`type`、`semantics`，以及：

- `category`：字节归属类别，见下表。
- `status`、`evidence`：核对状态和证据出处（SDK 声明、J273 静态反编译、REPENTOGON 源码、E2E 观察）。
- `wire`：只有需要同步的段才有。`kind` 取 `copy`（直接复制）、`nested`（嵌套类型，按其自身布局）、`container`（容器，发送元素数和元素）、`convert`（换算后发送）、`resource`（资源指针换成稳定标识）、`reference`（对象指针换成成员身份）、`choice`（按条件选分支）。
- `sequence`：归属哪条流，`world`（普通世界，约 30Hz）或 `body`（逐身体，约 60Hz）。
- `read_requires`：读取资格，例如 `entity-current-window` 表示只能在当前实体读取窗口内读。

| 类别 | 是否同步 | 说明 |
| --- | --- | --- |
| 状态 | 是 | 有语义的值，直接复制 |
| 换算 | 是 | 需要 Host 换算后才有跨进程意义，例如 Timeout 换成到期帧 |
| 引用 | 是 | 指向其他实体或格子的指针，换成成员身份 |
| 资源 | 是 | 指向资源或配置的指针，换成资源表中的稳定编号 |
| 容器 | 是 | `std::vector`、`std::string`、`std::deque`、树等容器头，发送元素 |
| 本地 | 否 | 只在本进程有意义：虚表、缓存指针、锁、Lua 引用 |
| 派生 | 否 | 客机可以从已发送的状态重新得到 |
| 权威 | 否 | 只属于 Host 的玩法状态，例如 RNG 种子与游标、寻路数据 |
| 填充 | 否 | 有证据的对齐字节 |
| 未知 | 否 | 尚未确定。不采集，也不阻塞完成 |

“是否同步”一列是默认处理，最终以需求清单为准：标为状态的字段，客机不需要就不会采集。

### 2.4 需求清单

`demand.json` 有两种写法：

- `all`：整个类型都按布局源同步，例如基础值类型和 Entity 公共部分。
- `fields`：只同步列出的字段，每个字段附上需要它的理由。

一个字段要进入需求清单，至少满足以下一条：原生显示会读它而客机算不出；原生对象结构需要它；它是玩家可见的规则结果；它属于事件。“公开 getter 存在”或“旧版曾经发送”都不构成理由。

### 2.5 帧锚点

随游戏帧递增或递减的计数，不发送计数本身，而是发送不随帧变化的锚点，否则所在块每帧都会变化：

- `Entity+0x328` 帧基准：FrameCount = 当前帧 − 基准，基准按原值发送。
- `Room+0x11F0` 帧基准：同上。
- 倒计时（如 Effect Timeout）换算成到期帧 = 当前帧 + 剩余帧。
- Run TimeCounter 发送 `game_frame − TimeCounter` 作为基准。

客机按帧号方案 B 把本地帧计数写成 Host 帧号，原生 getter 用它和锚点还原计数。锚点运算在 `mod/src/state/anchors.h`，按模 2^32 处理。

## 3. 生成器

`mod/CMakeLists.txt` 在构建时调用生成器，输出到构建目录的 `generated/`：

| 生成器 | 输入 | 主要输出 |
| --- | --- | --- |
| `gen_state_layout.py`（含 `gen_flat_layout.py`） | `binding/state/*.json`、`world_layout.json`、`binding/catalog/` 中的目录 | `state_capture.inc`（复制计划）、`state_wire.inc`、`state_client.inc`、`state_members.inc`、`state_body_codec.inc`、`state_flat.inc`、`state_resource_protocol.inc`、`state_protocol.inc`、`state_budget.inc`、`state_display_install.inc`、`state_host_fields.inc`、`state_grid_factory.inc` 等；布局文档 `mod/binding/layouts/` |
| `gen_binding.py` | `hooks.json`、`world_layout.json`、`v1_9_7_12-static.json`、`repentogon-binding.json` | Hook wrapper、补丁表、`world_layout.inc`、`hook-manifest.json` |
| `gen_entity_application_catalog.py`、`gen_player_application_catalog.py` | `binding/catalog/*-native-contracts.json` 等 | 应用职责目录（E2E 对照用） |

生成器会做一致性检查：布局编号唯一、字节段连续、块和字段的引用有效、公开成员目录逐行回映射。检查失败时构建失败。

布局文档 `mod/binding/layouts/` 由生成器写出，不要手改。每个类型生成一张原生字节归属表和一张线上布局表，`demand-coverage.md` 汇总需求覆盖情况。

## 4. 运行时流程

### 4.1 调用时机

采样只在 Host 主线程、原生更新正常返回之后进行。入口在 `mod/src/game/observation/world_bridge.cpp`：

| 时机 | 调用 | 内容 |
| --- | --- | --- |
| Manager 最外层入口 | `BinaryEntities::FrameBegin` | 取消上一帧未完成的候选，开始新的读取段 |
| 原生 Update 返回后 | `WorldPreparePostState` → `BinaryEntities::Prepare(normal, tick)` | 普通帧采集 World；Odd 玩家帧采集 Body |
| 本 tick 提交 | `WorldPublishPostState` → `BinaryEntities::Commit` | tick 提交成功后才把采集包交给后台；失败则丢弃 |
| Manager 出口 | `BinaryEntities::FrameEnd` | 结束读取段 |

另外，`hooks.json` 中 `world` 组的 Hook 在原生生命周期点通知采集器：实体工厂、Init、Remove、析构、Morph、Player 构造与替换、Grid 构造与释放等。它们只更新身份表和失效标记，不在这些点上读取完整状态。

### 4.2 一次普通帧采集

`BinaryEntities::Prepare`（`mod/src/game/observation/binary_entities.cpp`）的步骤：

1. **确认窗口**：`NativeOrdinary` 取得当前 run、room 和读取窗口。run 或 room 变化时整体失效，下一帧发完整基线。
2. **读全局根**：显示输入（`QueryBinaryDisplayInputs`）、场景根（Room、Level、Seeds）。全局根读取失败则整帧放弃。
3. **申请缓冲**：从 `CaptureExchange` 取一张 Ordinary 写票据。缓冲是预分配的，取不到就记失败原因，本帧不采。
4. **逐根采集**：对每个成员根：
   - 在本地身份表登记，取得成员身份；
   - 记下写入检查点；
   - 若上一份已发布快照中该根完全未变，复用旧值（整根复用），跳过逐字段读取；
   - 否则按复制计划执行；
   - 失败时回卷到检查点，把该根记为“沿用”，继续下一个根。
5. **段尾复核**：重新核对成员集合和身份，确认整段读取期间对象没有被替换。
6. **封存**：在 `Commit` 中把写票据发布给后台。

Body 采集走同样的路径，但只覆盖单个玩家身体，在该身体的 PlayerPost 窗口内完成。

### 4.3 成员枚举

成员来源在 `mod/src/world/binary/`：

| 文件 | 来源 |
| --- | --- |
| `entity_source.cpp` | 遍历原生 EntityList，按 vtable 确定布局编号；处理子对象、附件与具名特殊字段 |
| `entity_player.cpp` | PlayerManager 名册、子身体、Twin、备份槽 |
| `entity_containers.cpp` | 读取 MSVC 容器（vector、deque、map、set）的有界元素 |
| `entity_resources.cpp` | ANM2、图像、音效路径等资源，登记进资源目录 |
| `entity_flat_capture.cpp` | 平铺记录的镜像采集（实体、Player、Grid 的现行路径，见第 6 节） |
| `entity_source_reuse.cpp` | 整根复用与跨帧资源缓存 |
| `grid_source.cpp` | 448 个格子与 8 个门 |
| `scene_source.cpp`、`scene_map.cpp` | Room、Level、描述表、地图聚合 |
| `run_source.cpp` | 整局状态与 Seeds |
| `display_source.cpp`、`presentation_source.cpp` | 显示相关的同步值、声音资源 |

“具名读取器”只用于布局无法直接表达的字段（如容器、条件分支）。普通字段一律由生成的复制计划处理，不为单个字段写读取代码。

### 4.4 复制计划执行器

`mod/src/state/capture_executor.cpp` 执行生成的复制计划：

- 相邻的无条件字段合并成一次复制；有条件的字段按条件分支读取。
- 容器先核元素数上限，再逐元素递归。
- 引用写成三种之一：Null、已知身份、待定（目标本帧尚未登记）。
- 资源写成资源目录中的句柄。
- 换算字段按布局源的规则在读取时换算。
- 每根有读取预算，超出即按失败处理。

写入目标是 `CaptureValueWriter`（`mod/src/state/capture_values.*`），它只持有自有缓冲和数字索引。

### 4.5 事件与显示意图

一部分内容不是对象状态，而是“发生了什么”：音乐切换、音效播放、横幅文字、Boss 登场、拾取物提示等。它们由 `world` 组的 Hook 在原生调用点记录：

- `ProductionBegin/End`、`ProductionAudio`、`ProductionBannerText` 等在原生生产函数前后记录参数；
- `Intention` 记录音乐和音效请求；
- `MapFocus`、`OverlayShown`、`BossShown` 记录显示焦点。

这些记录进入 `BinaryDisplayCapture`（`mod/src/game/observation/binary_display_capture.cpp`）的有界事件队列，随下一份 World 发送。客机原生显示代码自己会发出的音效不在 Host 记录，避免重复播放。

## 5. 读取安全

读取闭源原生对象时，读错只会造成同步错误，不能造成 Host 崩溃。

| 机制 | 位置 | 作用 |
| --- | --- | --- |
| 读取会话 | `world/binary/read_session.*` | 每次读取走独立的 SEH 叶函数，访问违例转为该根失败 |
| 整帧可读区表 | `world/common/frame_read_regions.*` | 按 Manager 帧缓存页权限查询结果，避免逐次 `VirtualQuery`；页失效时作废条目并重查一次 |
| 窄读取 | `world/common/checked_read.h` | 页范围检查加窄 SEH |
| 身份见证 | `world/identity.*` | 地址到对象、use、shape、revision 的关联；段尾复核确认对象未被替换 |
| 外部资源写入水位 | `binary_entities.cpp` 的 `BinaryForeignResourceWrite` | 资源被其他路径修改时作废当前读取 |

规则：

- 原生地址只存在于主线程的当前读取段，不写进采集包。
- 不调用任何会修改游戏状态的函数；查询类原生函数只用已核定的。
- 线程、对象范围和读取寿命由调用方负责核定，读取工具只保证不崩溃。

## 6. 交接

`CaptureExchange`（`mod/src/state/capture_exchange.*`）是主线程和采集后台线程之间的交接点：

- 两条通道：Ordinary（World）和 Body（逐身体，多槽）。
- 缓冲预分配，数量有界。主线程用 `Acquire` 取写票据，写完发布；后台用 `Take` 取读票据。
- 控制消息：Begin、Stop、Invalidate、RetireBody、RequestCold（要求下一份为完整基线）。
- 后台只读取票据保护的自有字节，不访问任何游戏内存。

后台取到采集包以后的处理（成员索引、按字段或按块比较、快照、资源目录导出）见[同步](synchronization.md)第 3 节。

**平铺记录**：实体、Player 两条流和 Grid（控制组布局 196 除外）由 `entity_flat_capture.cpp`、`state/flat_capture.*` 采集。主线程按生成的复制段把整个根对象复制进自有镜像，再按补丁表把引用、资源、容器头和换算字段原位改写为规范值（换算字段与块路径共用同一张 recipe 表）。编码线程（`state/flat_encode.*`）逐字节比较镜像、定位变化字段，不再建单元索引。Game、Room、Level、显示域等全局根和控制组仍按单元采集、按块编码。是否启用由 `state/flat_layout.h` 的 `UsesFlatRecordContract` 按布局版本决定，版本 90 起启用。

## 7. 扩展：同步一个新字段

1. **找到字节**：在对应的 `binding/state/*.json` 中找到该字段所在的字节段。如果它还在未知段里，先拆出来，写清 `offset`、`size`、`type`、`semantics` 和证据。
2. **定类别**：按第 2.3 节的表选类别。指针要分清是引用、资源还是本地。
3. **加 wire**：给该段加上 `wire`（选合适的 `kind`）和 `sequence`，必要时写 `read_requires`。
4. **放进块**：把字段编入合适的 `blocks`。经常一起变化的字段放同一块；每帧都变的字段不要和很少变的字段放一起。
5. **登记需求**：在 `demand.json` 中写明客机为什么需要它。
6. **客机写入**：纯值字段由生成的写入表自动处理。如果字段需要具名原生写入（容器、资源、引用之外的特殊语义），在客机写入层登记，见 [Client 端设计](client.md)第 5 节。
7. **改版本**：布局变化影响互通，`version.json` 的 `version` 加 1。
8. **构建**：运行构建，生成器的一致性检查必须通过；查看 `mod/binding/layouts/` 中生成的表是否符合预期。

新增一个原生类型时，还要在成员来源里能枚举到它（通常按 vtable 自动识别），并确认对象大小和 RTTI。
