# 4：Entity 子类

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

完成十个 Entity 子类在公共部分之后（0x410 起）的字节归属和状态块，并覆盖现有的专用形态：Text、Dummy、Delirium、Effect136 搬起的格子、拥有型表现等。

## 输入事实

各子类的对象大小来自 SDK，已核偏移来自 `entity-application.json` 的 `class_native`：

| 类型 | 对象大小 | 子类字节数 | SDK 字段数 | 已核字段数 | 已核字节数 | 已核偏移范围 |
| --- | --- | --- | --- | --- | --- | --- |
| `Entity_NPC` | 0xF60 | 2896 | 25 | 19 | 113 | 0x410—0xF1C |
| `Entity_Familiar` | 0xF30 | 2848 | 11 | 19 | 111 | 0x5F8—0xEDC |
| `Entity_Knife` | 0xE78 | 2664 | 9 | 15 | 57 | 0x410—0x4EA |
| `Entity_Bomb` | 0x9D8 | 1480 | 10 | 11（另有 NativeBomb 1） | 51 | 0x410—0x462 |
| `Entity_Tear` | 0x848 | 1080 | 22 | 21 | 103 | 0x410—0x7F4 |
| `Entity_Pickup` | 0x6F0 | 736 | 12 | 12 | 39 | 0x524—0x59C |
| `Entity_Laser` | 0x568 | 344 | 16 | 30 | 113 | 0x410—0x560 |
| `Entity_Slot` | 0x548 | 312 | 10 | 8 | 26 | 0x410—0x540 |
| `Entity_Effect` | 0x490 | 128 | 4 | 13（另有 NativeEffect 1） | 69 | 0x410—0x47C |
| `Entity_Projectile` | 0x470 | 96 | 0 | 14 | 64 | 0x358—0x45C |

- **Delirium**：`EntityDelirium` 有 8 项，位于 0xBC8—0xBE0；NativeText 有 2 项，位于 0x410—0x414。
- **Projectile 的特殊情况**：已核偏移从 0x358 开始，落在 Entity 公共区内，本步要查清它与公共 DepthOffset 的关系。
- **现有专用内容**（`client_entity_adapters.h`）：
  - Laser 四份几何容器（optimized／unoptimized 采样点、indices、bezier）；
  - Pickup 循环道具；
  - NPC 复合绘制路径（两份 deque）；
  - Effect136 搬起的格子描述；
  - 拥有型表现 MrMe、SpriteTrail、PickupGhost、IsaacsBody、AbyssLocust；
  - Text 的原始字节与 KColor；
  - 各类辅助 Sprite。
- **几个本地指针**：
  - NPC +0xF40 拥有的玩法对象指针，Client 侧要求为空（`client_entity_adapters_native.cpp:46`）。
  - `Familiar.Player` 是裸借用，不是 EntityPtr 布局（同文件 48 行）。
  - Laser +0x4E8，Pickup +0x590／0x594／0x598 是具名引用（同文件 49—52 行）。

## 设计

- **布局编号**：每个子类一个布局编号。Host 按虚表（RTTI）确定布局编号，不只看 `_type`，因此 Text、Dummy 这类特殊虚表能区分开。
- **子类状态块**：按变化频率拆分，例如 Laser 的几何容器单独成块，Tear 的运动参数与外观参数分开。
- **附加块**：辅助 Sprite、拥有型表现、Laser 几何、Pickup 循环等变长内容各自成块，元素布局按第 2 步的规则处理。
- **专用形态**：Delirium、Effect136 等沿用现有具名 owner 的语义，只更换数据来源。
- **核对重点**：子类区间中未知字节数很大，NPC 有 2896 字节，现只核了 113 字节。必须先用 J273 反编译划出构造函数、Init 和 Update 写入的范围，再逐段归类；不能把整段未知区当成填充。

## 改动范围

- 布局源 `state/entity.json` 中的十个子类条目和专用块。
- Client 类别写入由生成表分派，取代 `ClientEntityClassState.fields` 的字符串匹配。专用容器继续沿用现有写入函数，例如 `ClientEntityCollections`。

## 失败方式

- **按 type 选布局**：只看 `_type`，把特殊虚表对象套上错误布局。
- **未知区处理错误**：大段未知区被当成填充跳过，导致关键状态漏同步（例如 NPC 的 AI 状态）。
- **借用当成 EntityPtr**：把裸借用当成 EntityPtr 读取，地址解析错误。
- **容器读取越界**：几何容器读取时没核 begin、end，或者超过上限。
- **复活拥有型对象**：在 Client 上重建了不应存在的玩法 actor，例如 NPC +0xF40。

## 完成内容

- **字节归属与表格**：十个子类和专用形态的字节归属全部完成，带证据，两张单行表由生成器产出。
- **回映射**：子类全部 `ReplicatedState` 成员都有回映射。

## 未决问题

- NPC、Familiar、Knife 子类区的大段未知字节，需要先估算核对工作量，再决定拆分次序。
- Projectile 在 0x358 的字段与公共 DepthOffset 的关系。
