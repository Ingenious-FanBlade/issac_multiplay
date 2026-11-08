# 5：EntityPlayer

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成 `Entity_Player` 子类区（0x410—0x2E9F）的字节归属。划分逐身体快源块（body 序列，约 60 Hz）和普通块（world 序列，约 30 Hz），覆盖内嵌 Sprite、容器、装备、关系和查询类状态。

## 输入事实

### 对象大小与 SDK 声明

`EntityPlayer.zhl` 声明对象大小 0x2EA0，共 129 个字段。子类区约 11.5 KB，是所有类型中最大的。

### 已核位置（`world_layout.json` 常量）

| 内容 | 位置 |
| --- | --- |
| 内嵌 Sprite | 自 0x41C 起，步长 0x114，共 13 份（约 3.5 KB） |
| playerType | 0x13BC |
| index | 0x1610 |
| 主动道具描述 | 自 0x1574 起，步长 0x20，VarData 位于各描述内 +0x1C |
| 子身体存储 | 0x1D84 |
| 双生存储 | 0x1E4C |
| 备份存储 | 0x1E50 |
| 替换存储 | 0x267C |

### SDK 中的容器与关键字段

| 字段 | 位置 |
| --- | --- |
| `_costumeSpriteDescs`（vector） | 0x1220 |
| `_weapon[5]`（Weapon 指针） | 0x13D8 |
| `_cacheFlags` | 0x1568 |
| `_controllerIndex` | 0x160C |
| `_trinketsID[2]` | 0x16B0 |
| `_collectibleCounts`（vector<int>） | 0x16B8 |
| `_history` | 0x1DD8 |

### 公开成员现状

- **应用目录**：`player-application.json` 共 1265 个成员。其中 556 个是 I31 加入的只读查询（captured-readonly-query），523 个是 Host 权威操作。
- **Entity 域中的 Player 成员**：EntityPlayer 有 295 个 `ReplicatedState` 成员。
- **只读查询的消费者**：195 个成员路由到 `ClientPlayerDisplayView::Captured`，在 `mod/src` 中找不到调用方。

### 现有逐身体快源

Lua `CapturePlayerFast` 每个身体每次读取约 45.8 ms，第 73 轮 557 次读取全部失败，失败原因没有被记录（静态推断见 `capture-bottleneck` 记录）。逐身体证言、exact-pair 绑定和规范第 11 节的读取规则继续有效。

## 设计

### 5.1 字节归属

- **内嵌 Sprite**：13 份内嵌 Sprite 各按 ANM2 布局处理（第 2 步）。是否存在、是否加载由各自的 `loaded` 和资源序号决定。
- **容器**：`_collectibleCounts`、`_costumeSpriteDescs`、History 等按变长块处理。
  - `GetCollectibleNum` 等查询的结果，原则上应由库存、效果等原生状态在 Client 上重新得到。
  - 若某个查询依赖的状态无法全部同步（例如依赖 Host 权威计算），该查询归为“换算”，Host 读取后作为字段发送。
  - 这是本步的核心核对项，旧目录中 3605 行的参数查询矩阵借此退役。
- **Weapon 指针**：`_weapon[5]` 归“容器／引用”，按武器布局复制所指对象的内容（Weapon 0x40），不发送地址。
- **身体关系**：子身体、双生、备份、替换存储归“引用”，转成成员序号；备份槽的语义沿用 tx-04-20 的四槽规则。
- **权威状态**：RNG、Host 专属的玩法状态归权威。

### 5.2 状态块（初步划分，本步定稿）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| P0 身体快源 | body | Entity E1／E4／E5 中的 Player 部分，加上头部与身体的动画和朝向等逐帧值 |
| P1 属性 | world | 伤害、射速、射程、移速、幸运、飞行等最终值，以及 cacheFlags |
| P2 生命与资源 | world | 各类心、钥匙、炸弹、硬币等 |
| P3 库存 | world | collectibleCounts、饰品、卡牌和药丸槽 |
| P4 主动道具 | world | 四个 ActiveItemDesc（含充能和 VarData） |
| P5 武器 | world | 各 Weapon 的内容 |
| P6 服装与 Sprite | world | costume 列表和 13 份内嵌 Sprite |
| P7 关系 | world | 子身体、双生、备份、替换、控制组 |
| P8 临时效果与 History | world | TemporaryEffects、History |

body 序列只包含 P0。没有合格 PlayerPost 的身体，P0 改由普通窗口采集，沿用现有“ordinary body-joint”语义，见第 11 步。

## 改动范围

- 布局源 `state/player.json`。
- Client 端的 `ClientPlayerFastValues`、`ClientPlayerDescription` 以及状态、装备子结构改由影子状态生成（第 13 步）。
- 现有原生写入保留，包括 I11—I27 已核的最终量、资源、角色和备份路径。

## 失败方式

- **查询派生判断错误**：把依赖 Host 权威计算的查询当成可在 Client 派生，Client 读到不同结果。
- **未加载的 Sprite**：内嵌 Sprite 未加载时，仍按已加载布局复制字段。
- **快源依赖错误**：body 序列的 P0 依赖了旧的成员表版本，身体关系与实体身份错配。
- **按指针判断武器变化**：Weapon 指针在两次采集之间被替换，Host 只比较地址，没发现内容变化。
- **快源静默失败**：沿袭第 73 轮的问题，PlayerFast 失败时不保留原因。新路径必须记录具名首因。

## 完成内容

- **字节归属与表格**：Player 子类区的字节归属全部完成，带证据；P0—P8 块定稿，两张单行表由生成器产出。
- **回映射**：Player 的全部 `ReplicatedState` 成员有回映射，每个查询类成员都标明是派生、换算还是权威。

## 未决问题

- 556 个只读查询里，哪些必须换算发送、哪些可以在 Client 派生，需要逐项核对。
- 195 个路由到 `ClientPlayerDisplayView::Captured` 的成员没有调用方，需要确认它们是否仍需同步。
