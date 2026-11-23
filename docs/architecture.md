# 总体架构

本文介绍 isaac-cs（以撒 C/S 联机）的整体结构：系统由哪些部件组成、各部件的职责，以及一帧之内数据怎样流动。各子系统的细节见：

- [采样](capture.md)：Host 怎样读取原生游戏对象，产出二进制状态
- [同步](synchronization.md)：线格式、增量、确认、资源与事件
- [Host 端设计](host.md)：房主进程的会话、权威 tick、输入接入与发布线程
- [Client 端设计](client.md)：客机进程的接收、原生安装、显示与隔离

文中的代码路径都相对仓库根目录。

## 1. 项目定位

| 项目 | 内容 |
| --- | --- |
| 目标游戏 | The Binding of Isaac: Repentance+ v1.9.7.12.J273（Win32） |
| 运行平台 | 官方 REPENTOGON 1.1.3。本项目以 REPENTOGON 的 zhl 插件 DLL（`zhlIsaacCS.dll`）形式加载 |
| 联机模型 | Listen Server：房主进程运行权威游戏，最多 3 个远端客机，共 4 名参与者 |
| 客机职责 | 上传输入和声明的 Mod 意图，安装 Host 状态并显示 |
| 当前范围 | 原版内容和声明式 Mod 联机接口。Mod 适配已实现、待统一 E2E；具体 Mod 按声明和清单协商 |

与原版 Online 和 Isaac Sync 相互隔离：不调用原版联机路径，也不兼容它们的协议。

## 2. 总体模型

```text
      Client（客机 ×1..3）                               Host（房主）
 ┌──────────────────────────────┐            ┌─────────────────────────────────────┐
 │ 物理输入采样 ──────────────────┼── 输入 ──▶│ 远端输入 → 虚拟输入设备 K → 原生 Join 玩家  │
 │                              │            │                                     │
 │                              │            │ 原生 Game::Update（唯一权威模拟）          │
 │                              │            │          │                          │
 │                              │            │          ▼                          │
 │                              │            │ 采样：按生成的复制计划读取原生对象内存       │
 │                              │            │          │                          │
 │                              │            │          ▼                          │
 │ 解码、校验、影子状态 ◀──────────┼── 状态 ───┤ 编码：块比较、增量、按 Peer 组帧            │
 │          │                   │            └─────────────────────────────────────┘
 │          ▼                   │
 │ Manager 边界：一帧内直接写入原生对象 │
 │          │                   │
 │          ▼                   │
 │ 显示推进 + 原生 Game::Render     │
 └──────────────────────────────┘
```

核心原则：

1. **Host 唯一权威**：玩法、随机数、存档和成就只在 Host 上产生。客机拦截 `Game::Update` 等玩法入口，并隔离随机数和持久化副作用。
2. **二进制采集**：原生对象按生成的复制计划读取。Mod 声明在普通 World 窗口读取 Lua 值并按强类型布局编码，两条路径都生成自有二进制数据。
3. **按需采集**：只同步客机需要的字段：原生显示要读而客机算不出的值、原生对象结构需要的值、玩家可见的规则结果，以及事件。字段清单写在 `mod/binding/state/demand.json`。
4. **显示交给原生代码**：客机把状态写进本地原生对象，再调用原生 `Game::Render` 绘制，不自行实现绘制。
5. **最终一致**：ACK 只表示“已收到”。客机在一帧内把最新状态直接写进活对象，单个对象出错只隔离该对象，由后续帧纠正。
6. **帧号方案 B**：客机每装好一帧 World，就把本地 `Game+0x25A70`（帧计数）写成 Host 帧号。随帧变化的计数一律以“锚点”形式同步。

## 3. 部件

| 部件 | 位置 | 说明 |
| --- | --- | --- |
| 主 DLL | `mod/src/`，产物 `zhlIsaacCS.dll` | 全部联机逻辑：Hook、会话、采样、网络、客机安装与显示 |
| 布局源与清单 | `mod/binding/` | Hook 清单 `hooks.json`、原生布局常量 `world_layout.json`、二进制状态布局源 `state/*.json`、成员目录 `catalog/` |
| 生成器 | `mod/tools/gen_*.py` | 构建时从布局源生成 C++ 表（`build/.../generated/*.inc`）和布局文档 `mod/binding/layouts/` |
| 界面 Mod | `ui/isaac-cs-ui/` | REPENTOGON ImGui 菜单“以撒联机”。只通过 `Isaac.ExecuteCommand` 下发 `cs ...` 命令，不注册玩法回调 |
| Mod SDK 与运行时 | `mod/sdk/isaaccs_sdk.lua`、`mod/src/game/mods/` | 加载期声明、握手清单、状态与意图、客机回调过滤。联机层的 `IsaacCS` 实现和回调分派脚本在 `mod/src/game/mods/lua/`，编进 DLL；对外分发的 SDK 是薄壳，只提供没装联机层时的单机行为 |
| 观测 Mod | `mod/observation/isaac-cs-observation/` | 只读 Lua 观测来源，用于语义核对和 E2E 对照，不在正式同步路径上 |
| E2E 工具 | `tools/harness/`（Rust）、`tools/isaac-dual/`、`tools/client-fixture/`、`tools/replica-contract/`、`tests/e2e-mods/` | 只在 `-E2E` 构建中产出，与产品包分开 |
| 构建脚本 | `scripts/build.ps1`、`scripts/package.ps1` | 纯构建与产品打包 |

`cs-t0/` 是早期研究原型（Rust 控制器与原型 Native），不参与当前构建。

## 4. 源码分层

`mod/src/` 按层次划分。上层依赖下层，纯值层不包含 SDK 或游戏头文件。

| 目录 | 层次 | 职责 |
| --- | --- | --- |
| `bootstrap/` | 入口 | `ModInit`：读取配置、核对 binding、按能力组注册 Hook |
| `hooks/`、`binding/` | 原生接入 | Hook 注册与常驻补丁（指针、字节、Odd、退出、显示、随机）；目标版本的偏移和 RVA |
| `core/` | 纯值核心 | 会话、参与者、tick 时钟与流水线、输入编解码、内存池账本。不依赖 SDK |
| `state/` | 纯值核心 | 二进制状态：布局表、采集执行器、交接、Host 编码与发布、线格式、Client 接收、影子状态与校验 |
| `world/` | 采样 | 原生只读来源（`world/binary/`）、本地身份、整帧可读区与读取保护（`world/common/`） |
| `net/` | 网络 | UDP socket 线程、握手协议、Host 端 Peer、客机连接、预算与压缩 |
| `replica/` | 网络 | 流配置、传输层（可靠发送、重组、zstd）、协议版本 |
| `game/` | 游戏适配 | 与真实游戏的主线程接线，见下表 |
| `ops/` | 运维 | 配置、`cs` 命令队列、控制端口、trace、录制命令 |

`game/` 的子目录：

| 目录 | 职责 |
| --- | --- |
| `game/common/` | 主线程 owner 栈、SDK 桥接合同、本机显示设置 |
| `game/authority/` | Host 会话、大厅、权威 tick、加载与持久窗口 |
| `game/input/` | 物理输入采样、统一输入查询、原生输入消费者 |
| `game/participants/` | 参与者与身体拓扑、虚拟设备 K、原生 Join、远端输入、远端生命状态 |
| `game/observation/` | Host 采样的主线程桥（`world_bridge.*`）和二进制采集协调（`binary_entities.*`） |
| `game/mods/` | 主线程 Lua SDK 接入、声明编译、采集和安装、原生 Player／进度显示镜像 |
| `game/client/` | 客机全部逻辑：`session/` 连接与输入上行，`scene/` 安装事务，`display/` 显示委托，`presentation/` 插值与预测，`entity/`、`player/`、`grid/`、`room/`、`resources/` 各域原生写入，`isolation/` 副作用隔离 |
| `game/tests/` | 只在 E2E 构建中编译的故障注入与对照 |

文件后缀约定：`*_native.cpp` 接触原生对象或 SDK，通常按 C++17 编译；`*_model.cpp` 只处理纯值；其余为协调器。项目内 `#include` 一律从 `mod/src` 写起完整路径。

## 5. 线程模型

游戏对象只在主线程访问。其他线程只处理自有纯值，跨线程通过有界队列或不可变快照交接。

| 线程 | 所在进程 | 职责 | 可以访问游戏对象 |
| --- | --- | --- | --- |
| 游戏主线程 | 两端 | Hook 回调；Host 采样；客机原生安装、显示推进与绘制 | 是 |
| 采集后台线程（`ops/world_commands.cpp`） | Host | 取走主线程封存的采集包，建立成员索引、块比较，生成来源快照 | 否 |
| Host 网络线程（`net/worker.cpp`） | Host | UDP 收发、握手、远端输入接收；为每个 Peer 组帧、限窗与重传 | 否 |
| 编码与压缩线程（`net/stream_session.cpp`） | Host | 封存字节的线上编码与 zstd，供各 Peer 共享 | 否 |
| 客机连接线程（`net/live.cpp` 的 `live::Client`） | Client | 每个连接一条：收包、重组、解码、纯值校验、影子状态、发送 ACK 与输入 | 否 |
| trace 与控制端口线程（`ops/`） | 两端 | 写日志；本地控制端口收发命令 | 否 |

规则：

- 原生地址不离开主线程。采集包、快照和网络消息里只有自有字节和数字身份。
- 状态变更命令（`cs host`、`cs stop` 等）只排队，在下一次最外层 `Manager::Update` 入口统一执行。
- Mod 意图与本机指令同在 Manager 入口处理。后台只处理意图和状态记录的格式及纯值，Lua handler、getter、setter 和生命周期通知均在主线程调用。
- 交接全部有界：队列满时显式计数并降级，不阻塞主线程。

## 6. 一帧之内

以撒的 `Manager::Update` 驱动整个游戏循环。原生 `Game::Update` 约 30Hz 运行完整世界；在奇数帧只更新玩家（Odd 帧），因此玩家身体按约 60Hz 推进。

### 6.1 Host

1. **Manager 入口**：执行排队的命令；网络线程收到的远端输入在这里成为本 tick 的输入。
2. **权威 tick**：`game/authority/tick.cpp` 绑定参与者、消费统一输入队列，然后让原生 `Game::Update` 运行。远端玩家的输入经虚拟设备 K 交给原生 co-op 玩家。
3. **后态采样**：原生更新返回后，`world_bridge.cpp` 在已核定的读取窗口内调用 `BinaryEntities::Prepare`。普通帧（StandardRoom）采集 World 和到期的 Mod 声明；玩家后态（PlayerPost）采集逐身体的 Body。
4. **交接**：采样结果写入预分配缓冲，经 `CaptureExchange` 非阻塞交给采集后台线程。
5. **后台编码**：后台线程生成不可变来源快照，发布到 `HostStatePublication`。
6. **网络发送**：网络线程为每个 Peer 相对其已确认基线组帧，World 和 Body 两条流各自限窗发送。

### 6.2 Client

1. **连接线程**：收包、重组、解码、纯值校验，更新影子状态，保留可解码的快照并立即发 ACK；把已封存的安装批次放入邮箱。
2. **Manager 边界**：主线程取最新批次，完成原生安装及事件回放，再使用同一 World 的 Mod 影子写入 Lua 值、派发生命周期及状态通知。
3. **显示推进**：`Game::Update` 被拦截不运行。客机改为调用已核实的显示子更新（相机、HUD、地图、动画、音乐等）。
4. **绘制**：包装原生 `Game::Render`。绘制前由插值和预测覆盖层临时写入显示位置，绘制后恢复。
5. **输入上行**：采样本机物理输入，投影到预计抵达 Host 的 tick 后发送。

## 7. Hook 策略

- **唯一清单**：全部 Hook、字节补丁和指针补丁登记在 `mod/binding/hooks.json`，由 `gen_binding.py` 生成 wrapper 和安装表。不在代码里另外手写安装点。
- **能力组**：Hook 分组（`loop`、`input`、`player`、`join`、`control`、`device`、`observe`、`world`、`client`）。`ModInit` 逐组核对目标指令，不符时只关闭该组，并记录原因。
- **常驻安装，逻辑启停**：Hook 在 `ModInit` 时安装，保留到进程结束。是否起作用由当前会话角色（Solo、Host、Client 或空闲）决定，不在运行中卸载 Hook。
- **owner 栈**：`game/common/context.*` 维护主线程调用来源栈（Manager、Game、Render、Odd 等），各 Hook 据此判断自己处在哪个原生阶段。

## 8. 版本与兼容

| 项目 | 位置 | 当前值 | 规则 |
| --- | --- | --- | --- |
| 布局版本 | `mod/binding/state/version.json` 的 `version` | 94 | 布局源或影响互通的合同变化时手工加 1；握手要求双方完全相等 |
| 流协议 | `mod/src/replica/stream_version.h` | framing 2、schema 1、compact 3 | 双方能力声明同时变化 |
| Mod 清单 | `net/mod_manifest.*`、`game/mods/mod_api.cpp` | `ModManifest`（27） | 在 Ready 前比较启用集合、版本、声明文本和内容 ID，拒绝未声明的 Lua Mod |
| 构建合同 | 同上 `kBuildContract` | `binary-state-v1` | — |
| 目标游戏 | `mod/binding/v1_9_7_12-static.json`、`world_layout.json` | J273 | 只支持这一版本；RVA 以 IDB 基址 `0x400000` 记录，运行时加实际模块基址 |

兼容性只靠整数版本号判定，不计算或校验任何哈希。

## 9. 构建与部署

```powershell
# 产品构建（Release / Win32 / v143）
pwsh -NoProfile -File scripts/build.ps1
# 含 E2E 工具与 Rust harness 的独立构建
pwsh -NoProfile -File scripts/build.ps1 -E2E
# 产品包包含 DLL、界面 Mod 与独立 SDK
pwsh -NoProfile -File scripts/package.ps1 -OutputDirectory out/product
# 从 E2E 构建单独选择模组适配夹具包
pwsh -NoProfile -File scripts/package.ps1 -ModAdaptation
```

- 依赖：Visual Studio 2022（v143）、Windows SDK、Python 3、REPENTOGON 1.1.3 源码树（默认 `F:/projects_isaac/REPENTOGON-1.1.3`，可用 `-RepentogonRoot` 指定）。
- CMake 工程在 `mod/CMakeLists.txt`。生成器在构建时运行，修改布局源后无需手工生成。
- 运行时配置读取游戏目录下的 `isaac-cs/config.json`，示例见 `mod/config.example.json`。配置缺失或格式错误时整体关闭，不注册任何 Hook。

## 10. 现状与已知限制

- 二进制同步主链路已接通并通过纯编译；统一的 E2E 整体验收尚未完成，运行行为以验收结果为准。
- 自布局版本 90（2026-11-21）起，实体、Player 两条流和 Grid 使用平铺记录编码，全局根和控制组仍用块编码，见[同步](synchronization.md)第 4 节。平铺编码在版本 85 上做过一轮实测；86—89 新增的字段（属性栏输入、预测对账字段、道具历史、`predict_gate`）在平铺路径上只通过了纯构建，尚未实机验证。
- 模组适配的运行覆盖由[专用夹具](../tests/e2e-mods/isaac-cs-mod-fixture/README.md)在第 14 步统一验收。声明范围、回调类别和限制见 [Mod 联机接口](mod-api.md)。
- 本机预测、插值与外推已实现，运行效果待验收，见[Client 端设计](client.md)。
- 历史设计、逆向报告和验收记录保存在 `docs/archive/`，只作查证用，其中的旧流程不再作为现行要求。
