# 原生游戏适配层

`game/` 负责联机系统与真实游戏的主线程接线：输入消费、参与者绑定、世界观测，以及客户端原生世界的构建、应用、展示和退休。

## 目录职责

| 目录 | 职责与主要入口 |
| --- | --- |
| `common/` | 公共执行上下文与 SDK 桥接合同；`context.*`、`bridge.h`、`player_api.cpp`。 |
| `authority/` | 房主会话、权威 tick、加载与退休、持久窗口观测；`tick.*`、`state.h`、`lifecycle.*`、`persistence.*`。 |
| `input/` | 物理输入采样、设备访问、统一输入查询与原生消费者作用域；`input_a.*`、`input_lease.cpp`、`query.*`、`consumers.*`。 |
| `participants/` | 参与者／身体拓扑、虚拟设备 K、原生 Join、远端输入桥与生命状态；`topology.*`、`device_k.*`、`join.*`、`peer.*`、`remote_life.*`。 |
| `observation/` | 世界采集主线程桥与 Entity owner 合同；`world_bridge.*`、`world_entity_owner.h`。`sources/` 保存原生／Lua、随机状态与资源观测来源。 |
| `client/session/` | 客户端连接、输入上行、流接收与恢复；`client.*`、`client_stream.*`。 |
| `client/scene/` | 场景准备、统一提交、借用代次、资源退休和 DTO 内存计量；`client_scene.*`、`client_native_world.*`、`client_native_bridge.cpp`、`client_lifetime.h`、`client_dto_memory.h`。 |
| `client/entity/` | Entity 实例 owner、公共／分类型状态、引用、资源、事件与绘制；`client_objects*`、`client_entity_*`。 |
| `client/player/` | Player 身体、控制组、角色、状态、装备、武器、HUD 与绘制，以及对应 SDK ABI 声明；`client_players*`、`client_player_*`。 |
| `client/room/` | Room、地图描述、背景构建与环境状态；`client_room_*`、`client_map_*`、`client_backdrop_*`。 |
| `client/grid/` | Grid 计划执行、原生字段安装、门与表现事件；`client_grids.*`、`client_grid_*`。 |
| `client/resources/` | 动画／图像资源模型、原生资源 owner 与 shader；`client_resource*`。 |
| `client/presentation/` | 场景展示目标与原生绘制；`client_presentation*`。 |
| `client/isolation/` | 客户端玩法、随机与持久副作用隔离及具名操作作用域；`client_isolation.*`、`client_player_isolation.*`。 |
| `tests/` | E2E 专用原生操作、故障注入和原值检查。实现文件仅在 `ISAACCS_BUILD_E2E_TOOLS` 下编译；共享头保留产品所需的条件编译空实现。 |

## 组织约定

- 先按角色与领域定位，再用现有文件后缀区分层次：`*_model.cpp` 通常解析／校验纯值，`*_native.cpp` 接触原生 SDK 或对象，领域执行器协调应用。此次仅调整目录，不拆分原有翻译单元。
- 项目内 include 从 `mod/src` 起写完整路径，例如 `game/client/player/client_players.h`，不增加旧路径转发头或依赖调用方的相对路径。
- `world/` 持有纯值观测模型与服务，`replica/` 持有复制目标、计划和传输，`state/` 持有原生二进制布局基础设施；`game/` 承担这些层与原生游戏之间的适配。
- 目录划分不改变线程、生命周期、Hook 常驻、统一输入或 C/S 隔离约束。跨域调用继续使用现有合同，所有命名空间与功能入口保持原样。
- CMake 显式登记源码和逐文件 C++17 选项；移动字段表时同时更新生成器的读取路径与 `DEPENDS`。Hook 生成模板也使用新 include 路径。

## 旧路径阅读

2026-11-10，`tx-05-01` 将原 `game/` 下的 136 个文件迁入以上目录，保留文件名。旧 `world_sources/` 对应 `observation/sources/`；E2E 文件对应 `tests/`；其他文件按上表定位。历史 Markdown 和 artifact 继续保留当时路径，当前构建输入与机器可读目录中的源码引用已同步迁移。

此次整理按用户要求未构建、未运行生成器或测试；运行行为的验收尚未执行。
