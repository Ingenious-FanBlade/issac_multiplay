# 14：退役旧路径与产品边界

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

从正式同步路径中移除 Lua getter 采集、JSON 树、逐成员目录和字符串分派。划清产品构建与 E2E 构建的边界，使产品 DLL 中不再包含旧路径的代码。

## 输入事实

### 构建边界

`mod/CMakeLists.txt:94` 的 `ISAACCS_BUILD_E2E_TOOLS` 选项区分产品构建和 E2E 构建。E2E 构建的工具、夹具和故障注入只进入 E2E DLL（`CMakeLists.txt:151-157`）。

### 旧路径主要文件

| 位置 | 内容 |
| --- | --- |
| `mod/src/game/world_sources/lua_entity.cpp`（708 行） | Lua 正式采集与 `CopyLuaSource` |
| `mod/src/game/world_sources/observation.cpp`（797 行） | 十一个域的 Lua 正式采集 |
| `mod/src/replica/stream_capture.cpp`（417 行） | 后台树构建与身份投影 |
| `stream_partition.cpp`、`stream_source_value.*`、`projection_graph.*`、`stream_materialize.*` | 分层、纯值块、Lua 图和还原 |
| `entity_plan.*`、`room_grid_plan.*`、`player_plan.*`，以及对应的 `*_target.*` | JSON 目标与 A→T 计划 |
| `stream_binary.cpp` 的字段表部分、`EncodePayload`／`DecodePayload` | 树形正文编码 |
| `mod/tools/gen_stream_catalog.py`（231 行）及其产物 `stream_policy.lua`、`field_ids` | 逐成员目录生成 |
| `client_entity_fields.inc` 的字符串分派 | Client 按名写入 |

### Lua 观测 Mod

`mod/observation/isaac-cs-observation`，包含 11 个域。

## 设计

### 14.1 移除顺序

1. **新路径完整编译**：第 10—13 步的新路径完成后，先确认它们可以独立编译，不依赖旧路径。
2. **逐个移除**：对上表中的每一项，先用 grep 和编译确认已经没有调用方，再删除。
   - 删除前要看清目标文件的实际内容，确认它的职责已被新路径完全接管。
   - 部分接管的文件，只删除已接管的部分。例如 `stream_binary.cpp` 中仍被控制消息使用的编码保留。
3. **同步构建与生成**：同步修改 `mod/CMakeLists.txt` 的源文件列表和生成命令，移除 `gen_stream_catalog.py` 的生成步骤。

### 14.2 Lua 观测 Mod 的新角色

| 部分 | 产品构建 | E2E 构建 |
| --- | --- | --- |
| `domains/mod`（具名只读 adapter） | 保留，作为 Mod 域唯一的正式来源（第 8 步） | 保留 |
| 其余十个域 | 不加载 | 只作为 E2E 对照（第 15 步）加载，不进入同步路径 |

### 14.3 保留不动的部分

- **Hook**：统一 Hook 清单保持常驻、按会话逻辑启停。新采集使用现有窗口，不新增 Hook。其中 `world_delirium_owner` 保留，只负责 Delirium 变身内 Init 之后的读取资格；shape 换代改由字段比较完成，旧路径退役时要去掉它升 shape 的逻辑（第 4 步 4.2.6）。
- **身份**：`IdentityTable`、`Registry`。
- **传输**：通道、可靠发送、分片重组、合包、zstd、内存池。
- **Client 原生写入层**：原生写入函数、结构事务和退休流程。

第 4 步 Tear／Pickup／Laser 已在新布局源中接管的旧行为不能随字符串表一起丢失：Tear 三份辅助 Sprite／七槽关系，Pickup 两份 Sprite／四槽关系／八项循环，Laser 十三槽关系／原始有符号采样模式／四份几何容器均须保留数字入口；引用退休先撤边，几何不调用 CalculateSamples，FlipSaveState 不复活。TearPlus／PickupPlus／LaserPlus 的派生声音、重抽和重采样请求仍属 Host。具体字段和边界见[本轮实现](implementation-4-tear-pickup-laser.md)。

### 14.4 文档同步

- **索引**：更新 `docs/README.md` 和 `docs/hooks/README.md` 中受影响的入口。
- **旧方案与历史资料**：旧方案文档已在第 0 步加了顶部说明；历史记录正文不改。
- **Repo 中的 `stream-catalog.json`**：作为历史资料保留在 `docs/design/tx-04-21/`，不再被构建引用。

## 改动范围

上表列出的文件、`mod/CMakeLists.txt`、观测 Mod 的加载方式，以及相关的文档入口。

## 失败方式

- **误删被依赖的代码**：删除仍被控制消息、E2E 或冷路径使用的代码，导致编译失败，或功能在运行时缺失。
- **旧路径残留**：旧路径仍被某个分支调用，产品 DLL 中保留着 Lua 采集入口。
- **产品依赖 E2E 内容**：E2E 对照 Lua 被产品构建加载，或者产品功能依赖 E2E 文件。
- **生成残留**：旧的生成产物仍然留在构建目录中，被意外包含。
- **Hook 被误删**：顺带删除或改动了 Hook，破坏“Hook 常驻”约束。

## 完成内容

- **移除与编译**：旧路径移除，产品与 E2E 两种构建都能纯编译通过。
- **无残留**：用 grep 确认产品代码中没有对旧接口的引用。
- **验证安排**：功能验证在第 15 步完成。

## 未决问题

- 冷路径（场景准备）中是否还有依赖旧 JSON 的部分没有被第 11 步的 Baseline 完全取代，需要在实现第 11 步时逐项核对。
