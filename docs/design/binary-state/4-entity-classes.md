# 4：Entity 子类

状态：部分实施。Familiar／Knife／Bomb、Tear／Pickup／Laser 和 Slot／Projectile／Effect 的布局、公开成员回映射和专用表现已接入生成链，未知字节与计时分支缺口保留，见[前三类实现](implementation-4-familiar-knife-bomb.md)、[中间三类实现](implementation-4-tear-pickup-laser.md)和[本轮三类实现](implementation-4-slot-projectile-effect.md)；NPC／Delirium／Text／Dummy 等剩余专用形态仍按各节推进。上级：[总计划](README.md)。

## 目标

完成十个 Entity 子类在公共部分之后（0x410 起）的字节归属和状态块，并覆盖现有的专用形态：Text、Dummy、Delirium、Effect136 搬起的格子、拥有型表现等。

## 输入事实

各子类的对象大小以 J273 `EntityFactory` 池步长为准（构造 `0x6EEE60`，见 [Entity 池静态研究](../tx-04-19/entity-i26-i30-native.md)第 11—24 行与 [Entity 原生静态研究](../tx-04-19/entity-native-static.md)第 13—26 行），SDK 声明大小只作对照；子类字节数按池步长减 0x410 计算。已核偏移来自 `entity-application.json` 的 `class_native`：

| 类型 | J273 池步长 | SDK 声明 | 子类字节数 | SDK 字段数 | 已核字段数 | 已核字节数 | 已核偏移范围 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `Entity_NPC` | 0xF60 | 0xF60 | 2896 | 25 | 19 | 113 | 0x410—0xF1C |
| `Entity_Familiar` | 0xF30 | 0xF30 | 2848 | 11 | 19 | 111 | 0x5F8—0xEDC |
| `Entity_Knife` | 0x520 | 0xE78 | 272 | 9 | 15 | 57 | 0x410—0x4EA |
| `Entity_Bomb` | 0x9D8 | 0x9D8 | 1480 | 10 | 11（另有 NativeBomb 1） | 51 | 0x410—0x462 |
| `Entity_Tear` | 0x848 | 0x848 | 1080 | 22 | 21 | 103 | 0x410—0x7F4 |
| `Entity_Pickup` | 0x6F0 | 0x6F0 | 736 | 12 | 12 | 39 | 0x524—0x59C |
| `Entity_Laser` | 0x568 | 0x568 | 344 | 16 | 30 | 113 | 0x410—0x560 |
| `Entity_Slot` | 0x550 | 0x548 | 320 | 10 | 8 | 26 | 0x410—0x540 |
| `Entity_Effect` | 0x4C0（见 4.1） | 0x490 | 176 | 4 | 13（另有 NativeEffect 1） | 69 | 0x410—0x47C |
| `Entity_Projectile` | 0x478 | 0x470 | 104 | 0 | 14 | 64 | 0x358—0x45C |

- **池步长与 SDK 不一致的四类**：Knife、Slot、Effect、Projectile。对象范围和未知区一律按池步长划定，不按 SDK 声明。
  - **Knife**：SDK 声明 0xE78，池步长只有 0x520（`entity-native-static.md:67`）。SDK 中偏移超过 0x520 的字段在 J273 不成立，本步逐项排除；子类区按 272 字节核对，而不是 2664 字节。
  - **Slot、Projectile**：池步长比 SDK 各多 8 字节，多出的 0x548—0x54F、0x470—0x477 先归为未知。
  - **池步长是上界**：实际 sizeof 可能因对齐小于步长（例如 Effect 的 ctor 只写到 +0x4B8）。步长内、ctor 不写的尾部字节，核实后归为填充。
- **其余专用池**：Text 池步长 0x530，Dummy 0x410（同上研究），按设计中的“布局编号”按虚表区分。
- **Delirium**：`EntityDelirium` 的 8 项公开字段位于 `Entity_NPC` 的 0xBC8—0xBE3，AI 实际使用到 0xBEB，见 4.2。
- **NativeText**：有 2 项，位于 0x410—0x414。
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
- **专用形态**：Effect136 等沿用现有具名 owner 的语义，只更换数据来源。Delirium 不是原生子类，做成 NPC 的变体臂，见 4.2。
- **核对重点**：子类区间中未知字节数很大，NPC 有 2896 字节，现只核了 113 字节。必须先用 J273 反编译划出构造函数、Init 和 Update 写入的范围，再逐段归类；不能把整段未知区当成填充。

### 4.1 EntityEffect 块划分（修订稿，2026-11-08）

本节是 EntityEffect 的块划分修订稿。它取代[总计划 5.1](README.md#51-示例entityeffect草稿) 中 EntityEffect 的字节归属和 69 字节单块，5.1 今后只作表格格式示例。本节表格都是草稿，生成器建成后以 `layouts/` 的输出为准。

**2026-11-09 实施修订**：以下草稿中基础臂／三个附属臂的 16B 不透明 VarData 复制没有充分证据，已撤销；`0x420—0x43F` 维持未知，FX2 显式预留。四种 TearFlags 与 GridDesc 臂分别保留已核状态。ANM2 经第 2 步展开后包括默认动画名和主／叠加动画的选择器数组，因此外观与动画均可能变长，不能套用草稿的 30／120B 动画常数。MrMe／SpriteTrail／PickupGhost 以 `shared-owner` 间接投影实现 FX3—FX5，首块 u8 present，其余块无重复 count；每份 Sprite 的图层 count 单独保存。缺失 owner 使用最短全零编码，见[本轮实现](implementation-4-slot-projectile-effect.md)及[生成表](layouts/README.md)。下方原草稿保留设计演变，不作为覆盖当前生成表的声明。

#### 4.1.1 补充的输入事实

- **实际对象大小**：
  - J273 中 Effect 池的步长是 0x4C0（[entity-native-static](../tx-04-19/entity-native-static.md) 第 15、92 行；[entity-i26-i30-native](../tx-04-19/entity-i26-i30-native.md) 第 21 行），不是 SDK 声明的 0x490。`world_layout.json` 原来也记为 0x490，已于 2026-11-08 更正为 0x4C0。
  - ctor `0x5CCEA0` 初始化了 +0x484—0x490、+0x4A8—0x4B0 两组内联容器，并清零 +0x4B4／+0x4B8（[entity-i15-i19-native](../tx-04-19/entity-i15-i19-native.md) 第 183 行）。
  - 因此按 0x490 划范围会漏掉 48 字节。子类区实为 0x410—0x4BF，共 176 字节。
- **0x420 是变体复用区**（同文件第 181 行）：
  - variant 54、113、126、167 的前 16 字节是 TearFlags（BitSet128），官方 wrapper 只允许这四个变体读写；
  - variant 136（GRID_ENTITY_PROJECTILE_HELPER）从 0x420 起是完整的 `GridEntityDesc`，大小 0x20，一直到 0x43F（`GridEntityDesc.zhl`）。[应用合同](../tx-04-19/entity-application-contracts.md)第 42 行要求不得按 16 字节截断；
  - 其他变体按各自需要使用这段，尚未逐个核对。
- **ExtraData 共享 owner**：
  - +0x4B4／+0x4B8 两个字分别是 payload 指针和控制块。替换 payload 必须经 `0x606310`，不能直接写指针。
  - Render `0x600E60` 只消费三种 payload（[entity-i15-i19-native](../tx-04-19/entity-i15-i19-native.md) 第 187—195 行）：
    - 108 MR_ME：`MrMeData`，内含 1 个 ANM2；
    - 166 SPRITE_TRAIL：`SpriteTrailData`，轨迹节点队列，每个节点 64 字节（`client_entity_owned_native.cpp:35-36`）；
    - 191 PICKUP_GHOST：`PickupGhostData`，内含 ANM2[4] 和 i32 count。
  - 其他变体如果有 payload，只供 Authority 使用，现有 Client 记为 AuthorityOnly（`client_entity_class_model.cpp:93-95`）。
- **私有容器**：+0x4A8 起的 vector 只供 Authority 更新，Render 没有消费者（`client_entity_class_model.cpp:178-179`，`lua_entity.cpp:423`）。
- **现有写入方式**：13 个字段、TearFlags 和 136 的格子描述，Client 都直接写内存（`client_entity_adapters_native.cpp:120-137`，`client_entity_class_fields.inc:160-173`），可作为 `direct` 的证据。ExtraData 由 `ClientEntityOwnedPresentation` 用游戏分配器和共享 owner 重建。
- **变化频率**：
  - 本地 Wiki（`IsaacDocs/docs/EntityEffect.md`）说明 Timeout 每帧递减，自定义效果也一样，后者从 −1 开始；
  - m_Height 用作粒子的纵向速度；
  - 其他字段大多在生成或状态切换时设置，实际频率由第 15 步实测。

#### 4.1.2 变体臂：同一个类，多个布局编号

0x420 区和 ExtraData 的字节归属随变体不同，同一组字节无法只用一种归属描述。修订如下：

- **按变体分布局**：Entity_Effect 分成下表 6 个布局编号，称为变体臂。Host 先按虚表确定类，再读 `Entity._variant` 选臂，未列出的变体使用基础臂。布局仍然以虚表为准，不按 `_type` 猜。
- **布局编号不变**：variant 变化本来就算形态变化（第 3 步 3.1），要走成员表，所以一个成员在生命期内布局编号不变。Client 只看成员表就知道该成员有哪些块、各块多长，"收齐全部块才可构造"的判定因此是确定的。
- **对其他步骤的要求**：布局源要支持变体臂（第 1 步 1.1），布局识别要增加选臂（第 10 步 10.2），见 4.1.9。

| 布局 | 变体 | 0x420—0x43F | 0x4B4—0x4BB ExtraData | 附加块 |
| --- | --- | --- | --- | --- |
| `Effect` | 其他 | 0x420—0x42F 不透明 VarData；0x430—0x43F 未知 | 权威 | — |
| `Effect.TearFlags` | 54、113、126、167 | 0x420—0x42F TearFlags；0x430—0x43F 未知 | 权威 | — |
| `Effect.GridDesc` | 136 | 完整 GridEntityDesc | 权威 | — |
| `Effect.MrMe` | 108 | 同基础臂 | 容器：0 或 1 个 MrMeData | FX3—FX5 |
| `Effect.SpriteTrail` | 166 | 同基础臂 | 容器：0 或 1 个 SpriteTrailData | FX3—FX4 |
| `Effect.PickupGhost` | 191 | 同基础臂 | 容器：0 或 1 个 PickupGhostData | FX3—FX5 |

ExtraData 在三个附属臂中归为"容器"：指针和控制块本身不发送，只发送 payload 是否存在，以及 payload 的状态。payload 的字节归属见 4.1.3。

#### 4.1.3 原生字节归属（草稿）

各臂的不同只在变体复用区和 ExtraData 两格，下面先列共用部分，再分臂列出这两处。生成器会为每个臂各输出一张完整表格。

**EntityEffect 原生字节归属（子类区 0x410—0x4BF，各臂共用）**

| 0x000—0x40F Entity【—】公共部分，见 Entity 表 | 0x410 f32 m_Height【状态】离地高度，粒子用作纵向速度 | 0x414 f32 FallingSpeed【状态】下落速度 | 0x418 f32 FallingAcceleration【状态】下落加速度 | 0x41C—0x41F 4B【未知】尚无证据 | 0x420—0x43F 32B 变体复用区【按臂】见下方变体区表 | 0x440—0x44F 16B【未知】尚无证据 | 0x450 i32 State【状态】效果自定状态，Init 写 0 | 0x454 f32 MinRadius【状态】冲击波最小半径 | 0x458 f32 MaxRadius【状态】冲击波最大半径 | 0x45C i32 Timeout【换算】剩余时间，每帧递减，Init 写 −1；发送到期帧，见 4.1.5 | 0x460 i32 LifeSpan【状态】寿命，Init 写 1 | 0x464 bool IsFollowing【状态】是否跟随 Parent | 0x465—0x467 3B【未知】疑似对齐填充，待证实 | 0x468 Vector ParentOffset【状态】相对 Parent 的偏移，见 Vector 表 | 0x470 i32 DamageSource【状态】伤害来源实体类型 | 0x474 f32 Rotation【状态】旋转 | 0x478 f32 Scale【状态】缩放 | 0x47C—0x483 8B【未知】尚无证据 | 0x484—0x493 16B【未知】ctor 初始化的内联容器头，类型待核 | 0x494—0x4A7 20B【未知】尚无证据 | 0x4A8 std::vector 私有容器【权威】只供 Authority 更新，Client 没有消费者 | 0x4B4 ExtraData 共享 owner 8B【按臂】payload 指针与控制块，见 4.1.2 | 0x4BC—0x4BF 4B【未知】池步长内的尾部，可能是对齐 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

**变体区 0x420—0x43F：基础臂与三个附属臂**

| 0x420 BitSet128 VarData【状态】按变体使用的 16 字节，不透明复制 | 0x430—0x43F 16B【未知】是否被个别变体使用，待核 |
|---|---|

**变体区 0x420—0x43F：TearFlags 臂**

| 0x420 BitSet128 TearFlags【状态】效果携带的眼泪效果位 | 0x430—0x43F 16B【未知】尚无证据 |
|---|---|

**变体区 0x420—0x43F：GridDesc 臂**

| 0x420 i32 type【状态】被举起格子的类型 | 0x424 i32 variant【状态】格子变体 | 0x428 i32 state【状态】格子状态 | 0x42C i32 spawnCount【状态】生成计数 | 0x430 u32 spawnSeed【状态】生成种子 | 0x434 i32 varData【状态】格子自定数据 | 0x438 bool initialized【状态】描述已初始化 | 0x439—0x43B 3B【未知】疑似对齐填充，待证实 | 0x43C u32 variableSeed【状态】变化种子 |
|---|---|---|---|---|---|---|---|---|

**MrMeData 原生字节归属（0x128）**

| 0x000 vptr【本地】MrMeData 虚表 | 0x004 4B【未知】未知或填充 | 0x008 ANM2 sprite【状态】MrMe 的 Sprite，见 ANM2 表 | 0x11C—0x127 12B【未知】尚无证据 |
|---|---|---|---|

**SpriteTrailData 原生字节归属（0x18）**

| 0x00 vptr【本地】SpriteTrailData 虚表 | 0x04 deque 头 20B【容器】轨迹节点队列，元素见轨迹节点表 |
|---|---|

**PickupGhostData 原生字节归属（0x458）**

| 0x000 vptr【本地】PickupGhostData 虚表 | 0x004 ANM2[4] sprites【状态】四个幽影 Sprite，见 ANM2 表 | 0x454 i32 count【状态】有效幽影数，0—4 |
|---|---|---|

**轨迹节点原生字节归属（64 字节，推断）**

节点布局来自现有代码注释（`client_entity_owned_native.cpp:35`）。tx-04-19 记录中部分字段语义仍为 UNKNOWN，本步需要用 J273 核对。

| 0x00 Vector Position【状态】节点位置 | 0x08 ColorMod Color【状态】节点颜色，见 ColorMod 表 | 0x34 f32 Scale【状态】缩放 | 0x38 f32 Rotation【状态】旋转 | 0x3C f32 Alpha【状态】透明度 |
|---|---|---|---|---|

**未知字节合计**：GridDesc 臂 74 字节，其余各臂 87 字节。总计划 5.1 按 0x490 计算，只算出 59 字节。

#### 4.1.4 状态块

**块号规则**：所有 Entity 布局的 E0—E9 固定为块 0—9，子类块从 10 开始编号。Effect 的块全部属于 world 序列。

| 块号 | 块 | 适用布局 | 内容 | 长度 | 变化频率 |
| --- | --- | --- | --- | --- | --- |
| 0—9 | E0—E9 | 全部 | Entity 公共部分，见第 3 步 3.2 | 见第 3 步 | — |
| 10 | FX0 计时与下落 | 全部 | m_Height、FallingSpeed、FallingAcceleration、State、Timeout（换算为到期帧） | 20B | 高：粒子每帧变化 |
| 11 | FX1 参数 | 全部 | MinRadius、MaxRadius、LifeSpan、IsFollowing、ParentOffset、DamageSource、Rotation、Scale | 33B | 低 |
| 12 | FX2 变体数据 | 全部 | 按臂：VarData、TearFlags 或 GridEntityDesc | 16B；GridDesc 臂 29B | 低 |
| 13 | FX3 附属头 | MrMe、SpriteTrail、PickupGhost | 存在标记和 Sprite 外观；SpriteTrail 只有存在标记 | 按臂 | 低 |
| 14 | FX4 附属动态 | 同上 | Sprite 动画状态，或轨迹节点 | MrMe 30B；PickupGhost 120B；SpriteTrail 变长 | 高 |
| 15 | FX5 附属图层 | MrMe、PickupGhost | 附属 Sprite 的图层 | 变长 | 低 |

划分理由：

- **原单块的问题**：总计划 5.1 把 14 个字段放在一个 69 字节块里，下落中的粒子每帧都要重发全部 69 字节。拆分后，粒子每帧只发 FX0 的 20 字节。
- **memcpy 次数**：FX0 和 FX1 各需 3 次 memcpy，比原方案多 2 次。读取的仍是同几条缓存行，CPU 成本可以忽略；带宽优先。
- **附属块**：ANM2 按第 3 步 E4／E5 的拆法，分成外观、动画、图层三部分。

**FX0 计时与下落（20 字节）**：复制 `[0x410,0x41C)`、`[0x450,0x454)`，再换算 Timeout。

| +0x00 f32 m_Height：离地高度，粒子纵向速度 | +0x04 f32 FallingSpeed：下落速度 | +0x08 f32 FallingAcceleration：下落加速度 | +0x0C i32 State：效果自定状态 | +0x10 i32 expire_frame：Timeout 到期的 Host 全局帧，见 4.1.5 |
|---|---|---|---|---|

**FX1 参数（33 字节）**：复制 `[0x454,0x45C)`、`[0x460,0x465)`、`[0x468,0x47C)`。

| +0x00 f32 MinRadius：冲击波最小半径 | +0x04 f32 MaxRadius：冲击波最大半径 | +0x08 i32 LifeSpan：寿命 | +0x0C u8 IsFollowing：是否跟随 Parent，0／1 | +0x0D Vector ParentOffset：相对 Parent 的偏移 | +0x15 i32 DamageSource：伤害来源实体类型 | +0x19 f32 Rotation：旋转 | +0x1D f32 Scale：缩放 |
|---|---|---|---|---|---|---|---|

**FX2 变体数据：基础臂与三个附属臂（16 字节）**：复制 `[0x420,0x430)`。

| +0x00 BitSet128 VarData：按变体使用的 16 字节原样 |
|---|

**FX2 变体数据：TearFlags 臂（16 字节）**：复制 `[0x420,0x430)`。

| +0x00 BitSet128 TearFlags：眼泪效果位 |
|---|

**FX2 变体数据：GridDesc 臂（29 字节）**：复制 `[0x420,0x439)`、`[0x43C,0x440)`。

| +0x00 i32 type：被举起格子的类型 | +0x04 i32 variant：格子变体 | +0x08 i32 state：格子状态 | +0x0C i32 spawnCount：生成计数 | +0x10 u32 spawnSeed：生成种子 | +0x14 i32 varData：格子自定数据 | +0x18 u8 initialized：描述已初始化，0／1 | +0x19 u32 variableSeed：变化种子 |
|---|---|---|---|---|---|---|---|

**FX3—FX5：MrMe 臂**

| +0x00 u8 present：MrMeData 是否存在，0／1 | +0x01 ANM2 外观 sprite：MrMe Sprite 的定长外观部分，同第 3 步 E4 中的主 Sprite 部分 |
|---|---|

| +0x00 AnimationState anim：主动画，见 AnimationState 表 | +0x0F AnimationState overlay：叠加动画 |
|---|---|

| +0x00 u16 count：图层数 | +0x02 count×LayerState layers：各图层状态，见 LayerState 状态表 |
|---|---|

**FX3—FX4：SpriteTrail 臂**

| +0x00 u8 present：SpriteTrailData 是否存在，0／1 |
|---|

| +0x00 u16 count：节点数 | +0x02 count×TrailNode nodes：轨迹节点，见轨迹节点状态表 |
|---|---|

**轨迹节点状态布局（64 字节）**

| +0x00 Vector Position：节点位置 | +0x08 ColorMod Color：节点颜色 | +0x34 f32 Scale：缩放 | +0x38 f32 Rotation：旋转 | +0x3C f32 Alpha：透明度 |
|---|---|---|---|---|

**FX3—FX5：PickupGhost 臂**

| +0x00 u8 present：PickupGhostData 是否存在，0／1 | +0x01 i32 count：有效幽影数，0—4 | +0x05 4×ANM2 外观 sprites：四个幽影 Sprite 的定长外观部分 |
|---|---|---|

| +0x00 8×AnimationState anims：四个 Sprite 各自的主动画与叠加动画，共 120 字节 |
|---|

| +0x00 u16 count：图层总数 | +0x02 count×(u8 sprite + LayerState) layers：图层所属 Sprite 序号 0—3，以及图层状态 |
|---|---|

**附属臂的两条约定**：

- **存在标记**：附属 owner 最多一个元素，不是 `std::vector`，所以用定长块中的 `u8 present` 表示是否存在，不套用 `u16 count` 规则。
- **补零**：present 为 0 时，同块其余字节必须全部为 0，相关变长块的 count 为 0。这样 memcmp 结果稳定。

#### 4.1.5 Timeout 换算为到期帧（用户 2026-11-08 采纳）

- **问题**：所有 Effect 的 Timeout 每帧都递减。如果原样复制，每个 Effect 每帧至少产生一条记录（12 字节记录头加 20 字节 FX0）。world 预算默认每秒 120000 字节，30 Hz 下每帧约 4000 字节；约 125 个只有 Timeout 在变的 Effect（地面血迹、creep 等）就会占满。
- **做法**：按帧锚点规则（总计划第 4 节），Host 发送 `expire_frame = 全局帧（Game+0x25A70）+ Timeout`，类型 i32。Timeout 按帧递减时到期帧保持不变；SetTimeout 或变体逻辑改写 Timeout 时，到期帧随之改变，块照常发送。
- **Client 不运行原生 Update**：
  - 这是 tx-04-17 已采纳的隔离设计，现有 Client 拦截 Game、Room、EntityList、Player 等 Update 入口（`client.cpp:28,142-146`）。
  - 因此 Client 本地不会推进 Timeout，它自己的 `Game+0x25A70` 也不代表 Host 时间。Client 以展示时钟（第 13 步 13.6）还原 Timeout。
- **Client 写入**：
  - `writer:effect_timeout` 直接写 +0x45C，不经过原生 Update；
  - 计算式为 Timeout = expire_frame − 展示时钟，按 u32 相减后转成 i32；
  - 每装好一个新的 world 帧，展示时钟随之前进，Client 就把所有 Effect 的 Timeout 重写一遍。每个对象只是一次减法加一次 4 字节写入。
- **一致性**：Client 上的 Timeout 总是"已应用版本"那一帧的值，与同一版本的其他状态一致；丢帧时停在上一个已应用帧，也和其余状态一致。漂移校正读回时用同一个展示时钟换算，不会误报。
- **正确性**：换算只影响发送量，不影响正确性。递减不规则时（例如冻结），到期帧会变化，块被重新发送。

#### 4.1.6 写入方式与校验

| 字段 | 写入方式 | 依据 |
| --- | --- | --- |
| FX0、FX1 中除 Timeout 外的字段 | `direct` | 现有 Client 直接写入（`client_entity_adapters_native.cpp:120-127`） |
| Timeout | `writer:effect_timeout` | 见 4.1.5 |
| TearFlags | `direct` | 同上，按 Bits128 写 |
| GridEntityDesc 的 8 个字段 | `direct` | `client_entity_adapters_native.cpp:134-136` |
| 基础臂 VarData | 待核 | 现有 Client 不写这些变体的 VarData，Render `0x600E60` 也未见直接读取。核对 J273 中的写入点后，定为 `direct`，或改归派生、不适用 |
| ExtraData 的存在与 payload | `writer:ClientEntityOwnedPresentation` | 用游戏分配器构造具体类型，经 `0x606310` 替换。不 memcpy owner，不伪造虚表 |
| +0x4A8 私有容器 | 不写 | 归为权威 |

- **写入顺序**：IsFollowing 和 ParentOffset 与 Parent（E8）属于同一组关系，原生 `FollowParent 0x4522E0` 会同时设置这三项。如果 E8 因 Parent 尚未构造而延后，并且 IsFollowing 为 1，那么 FX1 也必须一起延后。
- **校验**：
  - 所有 f32（含轨迹节点）必须是有限值；
  - IsFollowing、initialized、present 只能是 0 或 1；
  - present 为 0 时其余字节必须为 0；
  - PickupGhost 的 count ≤ 4；
  - 轨迹节点数不超过布局上限。现有代码上限是 16384，本步按带宽重新确定。

#### 4.1.7 公开成员回映射

| 公开成员 | 映射 |
| --- | --- |
| m_Height、FallingSpeed、FallingAcceleration、State | FX0 |
| Timeout | FX0 `expire_frame`（换算） |
| MinRadius、MaxRadius、LifeSpan、IsFollowing、ParentOffset、DamageSource、Rotation、Scale | FX1 |
| GetTearFlags、HasTearFlags | TearFlags 臂的 FX2；其他臂不适用 |
| GetGridEntityDesc | GridDesc 臂的 FX2；其他臂不适用 |
| IsPlayerCreep | 派生：Wiki 声明为以 variant 为参数的静态函数，只取决于 E0 的 variant |
| AddTearFlags、ClearTearFlags、SetTearFlags、SetTimeout、SetRadii、SetDamageSource、FollowParent、CreateLight、CreateLootPreview | 不适用：操作，不是状态 |

#### 4.1.8 典型帧

- **下落的粒子（基础臂）**：E1、E5、FX0 变化，`block_mask` 为第 1、5、10 位（0x422）。子类部分每帧 20 字节，原方案是 69 字节。
- **静止的地面效果**：只有 Timeout 在变，换算后块都不变化，通常整条记录都不发送。原方案每帧至少发送 12 字节记录头加 69 字节。
- **SPRITE_TRAIL**：轨迹每帧增加节点，FX4 整块重发，大小为 count×64 字节，是 Effect 中最占带宽的块。

#### 4.1.9 同步修改的文档（2026-11-08 已完成）

本节引出的配套修改已同步到下列文档，FrameCount 按同一规则一并处理：

- **总计划**：
  - 第 3.1 节"换算"一行的例子从 FrameCount 改为 Effect Timeout；
  - 第 4 节新增帧锚点规则；
  - 第 5.1 节注明 EntityEffect 示例已由本节取代。
- **第 0 步**：术语表增加"变体臂""帧锚点""展示时钟"。
- **第 1 步**：
  - `native_size` 以 J273 实际大小为准；
  - 增加 `variants` 变体臂；
  - 写入块号规则；
  - 增加变体臂相关的拒绝条件。
- **第 2 步**：ANM2 拆成外观、动画、图层三部分，与第 3 步 E4／E5／E6 一致。
- **第 3 步**：E2 不再发送 FrameCount，改为原值发送帧基准 `+0x328`。
- **第 6 步**：Effect136 举起的格子由 FX2 承载，Grid 域不另发一份。
- **第 7 步**：`Room+0x11F0` 帧基准按同一规则放入 R1。
- **第 9 步**：帧头 game_frame 兼作 Client 展示时钟。
- **第 10 步**：10.2 增加选臂；10.3 的换算例子改为 Timeout。
- **第 11 步**：没有变化块时也发送空帧，推进展示时钟。
- **第 13 步**：
  - 增加 `effect_timeout` writer 和 13.6 展示时钟；
  - 13.4 改为已定：Client 不运行原生 Update；
  - 退役 draw 期间逐对象重新锚定帧基准的代码。

`world_layout.json` 中 Effect 的 `object_size` 也已更正为 0x4C0，Projectile 更正为 0x478（池步长，见输入事实），`evidence` 中注明了依据。Knife、Slot 不在该文件中，Door 已分列 `factory_allocation_size` 0x2F0。

- **影响范围**：这个值只在生成器中使用，一是作为字段偏移越界检查的上限，二是写进 `world_layout.inc` 的类表（`mod/tools/gen_binding.py:1085-1097,1224-1230,1418-1423`）。运行时代码不读取 `ClassSpec::object_size`。
- **结果**：改大只会放宽越界检查。
- **未验证**：本次没有运行生成器或构建，由第 15 步的统一验收覆盖。

### 4.2 NPC 的 Delirium 变体臂（设计稿，2026-11-08）

本节取代“Delirium 沿用现有具名 owner 语义”的笼统说法。事实来自 J273 静态分析，没有运行游戏，标明“推断”的未实测。表格都是草稿，实施后以 `layouts/` 的输出为准。

#### 4.2.1 为什么是变体臂，不是子类

- **原生没有这个类**：J273 RTTI 的 `Entity_*` 共 13 个（本步开头表中的 10 个，加 Player、Text、Dummy），没有 Delirium。
- **同虚表、同池**：Type 412 进 NPC 默认池（[entity-native-static](../tx-04-19/entity-native-static.md) 第 23、28 行），虚表就是 `Entity_NPC`，按虚表分不出来。
- **`EntityDelirium` 只是 Lua 接口**：REPENTOGON 的元表以 `ENTITY_NPC` 为 `__parent`，底下仍是 `Entity_NPC*`（`LuaDelirium.cpp:260-262`）；8 个字段就声明在 `Entity_NPC` 里（`EntityNPC.zhl:137-144`）。

不过这段字节只对 Type 412 有意义（见 4.2.2），归属随形态字段而不同，这正是变体臂要解决的问题（第 1 步 `variants`）。

#### 4.2.2 J273 事实

对 `.text` 扫描了 `+0xBC8—0xBEB` 的全部 disp32 直接寻址。经基址计算的访问看不到；`0x6ED4B0`、`0x903730`、`0x9041E0`、`0x925C00`、`0x98C690`、`0x98C7E0`、`0x98CCC0`、`0x98DDA0` 的命中上下文不像 NPC，未逐个定类。

- **写入者**：Delirium AI `0x49F3D0`、`0x4A1A30`、`0x4A2720`；NPC ctor `0x6B4520`、Init `0x6B4A50`（`Entity_NPC` 虚表 +4）；`0x670200` 在判定 Type 412 后写红色模式位。
- **其他读者**：
  - `0x44EF70`、`0x45A450`、`0x45B130`、`0x474720`、`0x485F10`、`0x4AC220`、`0x4BF700` 这 7 个函数只读 `+BC8`，判断是否为 0 或与 `_type` 比较，用来区分当前是否由 Delirium 代跑。
  - `0x6C9510`（虚表 +0x5C）对 Type 412 用 BossType／BossVariant 代替自身类型。
  - NPC Render `0x6CEC90` 不读这一段。
- **池槽残留（推断）**：Init 只清 `+BC8`、`+BCC`、`+BD0—BD3`、`+BE8`。ctor 只在工厂建池时运行（唯一引用在 `0x6EF009`），且只写 `+BE0`、`+BE4`；两者都没有范围清零。所以普通 NPC 的 `+BD8—BE7` 在池槽复用后可能留着上一只 Delirium 的值。
- **变身不改 `_type`**：`0x49F3D0` 保存 `+0x28／+0x2C`，临时换成 BossType／BossVariant 运行，返回前恢复。tx-04-19 实测也是 BossType 20、Type 仍为 412（[验收记录](../tx-04-19/tx-04-19-acceptance.md)第 27 行）。
- **BossType 有两条改写路径**：变身分支经虚 Init 重配；state 8 分支中，如果 Boss AI 改了 `_type`，AI 直接把新值写回 `+BC8／+BCC`，只重载主 Sprite（`0x49EC80`），不经过 Init。
- **打包字段**：
  - `+BD8` 的四个字节分别是弹幕编号、剩余次数、继续概率、相位计数（`0x4A1A30`），`+BDC` 低字节是 Angle。REPENTOGON 的 `AttackID` 是这四个字节合成的 u32，只有低字节是编号。
  - `+BE0` 被当作 64 位读取，但 `+BE4` 的全部 disp32 写入点都写 0，有效内容在低 32 位。
- **`+BE8` 是 EntityPtr 槽位**：形态 410 时由 Child（`+0x3C0`）写入，序列与 `EntityPtr::SetReference 0x858760` 相同（`0x4C8EE0` 摘旧、`0x4C79D0` 挂新）。除 Init 外只有 `0x49F3D0` 访问，公开接口也没有。
- **变化频率**：state 8 下，AI 每个 tick 都改 `+BD0`（递减）、`+BD8`（相位计数加 1，形态 45、84:10 除外）和 `+BE0`。

#### 4.2.3 变体臂

| 布局 | 选择字段 `type` | 0xBC8—0xBEB | 附加块 |
| --- | --- | --- | --- |
| `NPC` | 其他 | 权威，36 字节，不发送 | — |
| `NPC.Delirium` | 412 | 见 4.2.4 | DL0、DL1 |

- **选臂顺序**：先按虚表确定 `Entity_NPC`，再读 `type` 选臂，与 Effect 读 `variant` 相同（第 10 步 10.2）。类别仍由虚表决定，这不是“按 type 选布局”。
- **选择字段成立**：`type` 已标 `shape`；生成器按展开后的字节段查选择字段，继承自 `Entity` 的字段可用（[gen_state_layout.py](../../../mod/tools/gen_state_layout.py) 的 `Compiler.make_variants()`，按 `variants['selector']` 查找展开后的 `ranges`；不用易过期的固定行号）。
- **基础臂归为权威**：字节类别里没有“不适用”。这段对普通 NPC 没有玩法含义，Client 也不运行 AI；不发送，残留值也就不会上线。
- **采集时机**：`type` 只有在 AI 之外才是 412。采集窗口不能落在 `0x49F3D0` 内，沿用现有“owner 未退尽不提交采集”的规则（[验收记录](../tx-04-19/tx-04-19-acceptance.md)第 152 行）。
- **编号**：布局编号和块号在实施 NPC 条目时分配，DL0、DL1 接在 NPC 共有块之后。

#### 4.2.4 原生字节归属（草稿）

**NPC.Delirium 原生字节归属（0xBC8—0xBEB）**

| 0x000—0xBC7【—】公共部分与 NPC 前段，见各自表格 | 0xBC8 u32 BossType【状态】当前变身的 Boss 类型，形态字段 | 0xBCC u32 BossVariant【状态】当前变身的 Boss 变体，形态字段 | 0xBD0 u16 TransformationTimer【状态】距下次变身的帧数 | 0xBD2 u8 RemainingAttacks【状态】变身前剩余攻击次数 | 0xBD3 u8 StateD【状态】Delirium 内部状态机 | 0xBD4—0xBD7 4B【未知】无 disp32 访问，疑似对齐填充，待证实 | 0xBD8 u32 AttackID【状态】弹幕打包值，见 4.2.2 | 0xBDC u8 Angle【状态】弹幕角度，0—255 | 0xBDD—0xBDF 3B【未知】写入时恒为 0，疑似位域填充，待证实 | 0xBE0 u32 Cycle【状态】红色模式与传送计时打包值 | 0xBE4—0xBE7 4B【未知】全部写入点写 0，疑似填充，待证实 | 0xBE8 EntityPtr companion【权威】形态 410 的伴随实体，只有 AI 使用 | 0xBEC—0xF5F【—】NPC 其余部分，待划定 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

**未知字节合计**：11 字节。

#### 4.2.5 状态块

| 块 | 适用布局 | 内容 | 长度 | 变化频率 |
| --- | --- | --- | --- | --- |
| DL0 形态 | `NPC.Delirium` | BossType、BossVariant | 8B | 低：只在变身时变化 |
| DL1 动态 | `NPC.Delirium` | TransformationTimer、RemainingAttacks、StateD、AttackID、Angle、Cycle | 13B | 高：state 8 下每个 AI tick 变化 |

**DL0 形态（8 字节）**：复制 `[0xBC8,0xBD0)`。

| +0x00 u32 BossType：当前变身的 Boss 类型 | +0x04 u32 BossVariant：当前变身的 Boss 变体 |
|---|---|

**DL1 动态（13 字节）**：复制 `[0xBD0,0xBD4)`、`[0xBD8,0xBDD)`、`[0xBE0,0xBE4)`。

| +0x00 u16 TransformationTimer：距下次变身的帧数 | +0x02 u8 RemainingAttacks：变身前剩余攻击次数 | +0x03 u8 StateD：内部状态机 | +0x04 u32 AttackID：弹幕打包值 | +0x08 u8 Angle：弹幕角度 | +0x09 u32 Cycle：红色模式与传送计时打包值 |
|---|---|---|---|---|---|

划分理由：

- **DL0 单独成块**：它的变化走形态路径，不和高频字段混在一起。
- **DL1 独立于 NPC 主块**：它每个 tick 都变，独立成块才不会连带 NPC 主块重发。块本身只有 13 字节，一个房间通常只有一只 Delirium，带宽可以忽略。
- **不做帧锚点**：这些计数只在 state 8 前进，并由 AI 按条件改写，属于第 3 步列出的条件计时，不能按全局帧线性还原。

#### 4.2.6 形态变化

- **形态字段**：`BossType`、`BossVariant` 标 `shape`。第 3 步列的是 Entity 公共形态字段，这两个由本臂追加。
- **检测方式**：第 11 步比较当前布局全部 `shape` 字段的值，不写死 type、variant。这样经 Init 和不经 Init 的两条路径都能覆盖。
- **布局编号不变**：变身不改 `type`，成员始终是 `NPC.Delirium`。
- **`world_delirium_owner` 保留，职责收窄**：旧路径用它在 Init 时升 shape（`mod/src/world/collector.cpp:1317-1321`）。新路径由字段比较换代，它只负责在变身内的 Init 之后恢复 `+0x177` 的读取资格（规范第 11 节），不再升 shape，否则同一次变身会换代两次。
- **Client**：按第 13 步 13.2 退休旧实例、按新形态构造，不调用 AI，也不重新抽 Boss。
- **专用 draw 限制不变**：变到 Type 835、906、920、921 等形态时，仍按 [tx-04-19-class-application](../tx-04-19/tx-04-19-class-application.md) 第 47 行写前拒绝，不因分臂放宽。

#### 4.2.7 写入方式与校验

- **写入方式**：DL0、DL1 全部字段用 `direct`。现有 Client 按这些宽度直接写入（`client_entity_class_fields.inc:22-29`），J273 的 AI 也是普通赋值。
- **校验**：字段都是定宽整数，按原宽度写回，不需要额外校验。
- **`+BE8` 不写**：归为权威，Client 上保持 Init 写入的 0。

#### 4.2.8 公开成员回映射

| 公开成员 | 映射 |
| --- | --- |
| BossType、BossVariant | DL0 |
| TransformationTimer、RemainingAttacks、StateD、AttackID、Angle、Cycle | DL1 |
| GetTeleportationTimer | 派生：`(Cycle >> 15) & 0x3FF`（`LuaDelirium.cpp:211`） |
| IsRedMode | 派生：`((Cycle >> 7) & 0xFF) != 0`（`LuaDelirium.cpp:191`） |
| SetRedMode、SetTeleportationTimer、Transform | 不适用：操作，不是状态 |

#### 4.2.9 同步修改的文档（2026-11-08 已完成）

- **第 1 步**：`variants` 增加 NPC 的例子。
- **第 3 步**：3.1 注明子类可以追加形态字段。
- **第 10 步**：读取规则增加 `world_delirium_owner` 的资格见证；10.2 增加 NPC 选臂和采集时机。
- **第 11 步**：11.2 的 shape 判定改为按布局的 `shape` 字段；增加对应失败方式。
- **第 14 步**：14.3 注明该 Hook 保留及其职责。

布局源没有改动，`entity.json` 的 NPC 条目在实施第 4 步时写入，布局版本届时递增。

### 4.3 Familiar、Knife、Bomb 本轮实现（2026-11-08）

布局源为 `mod/binding/state/entity-classes.json`，布局版本 16。本节记录实际实现范围；不把未知字节归属和第 15 步验收提前写成完成。生成表分别见 [Familiar](layouts/EntityFamiliar.md)、[Knife](layouts/EntityKnife.md)、[Bomb](layouts/EntityBomb.md)，证据与主审见[静态证据](familiar-knife-bomb-evidence.md)。

- 三类继承 Entity 的 E0—E9，子类块从 10 起。Familiar／Knife／Bomb 编号分别为 120／121／122；Familiar 的 48／231 变体臂为 123／124，分别传送 IsaacsBodyData 和 AbyssLocustData，不在 Client 创建其他未知玩法 owner。
- Familiar `+0xDB8` 的 ANM2 与主 Sprite 独立；Bomb `+0x464` 的五份 ANM2 按 0x114 步长分别投影。外观、动画、图层分别成块；共享 payload 和 Sprite vector 的元素也按子块投影，避免动画变化重发整组资源。
- Familiar.Player `+0x410` 按裸借用映射；KnifePlus.hitboxSource 按 EntityPtr 映射。派生 Plus 沿用同一份 E9 伴随来源，不重复发送基类字段。
- Familiar FireCooldown、Bomb `+0x410` 保留明确方向的 mode/value 锚点声明。尚未核定活动条件必须报 Gap，条目保持“推断”。HeadFrameDelay 的线性计时语义没有证据，改为直接保存公开 i32 原值，不要求计时方向。Bomb `+0x414` 完整用途未知，继续不复制。
- 恢复旧路径已同步的 Knife `+0x4F4/+0x4F8/+0x4FC/+0x500` 和 Familiar `+0xDAC` 五个 EntityPtr 槽；以身份／待登记句柄采集，后台转成员序号，Client 经引用 owner 恢复。Knife `+0x4E7` 降为“推断”的原始 u8，不强加未证明的 bool 0/1 约束。
- 蝗虫两组 Sprite 的 `count_groups` 合计上限为 16384，不是每组各自独立允许 16384；同组各投影 count 必须一致，数字检查表与生成文档来自同一源。
- 原生范围按 J273 池步长 F30／520／9D8；Knife SDK 的 E78 仅作差异记录。完整未知列表和各嵌套对象的未知均由生成表保留，本轮不声明类型字节完整。

详细交付、编译与后续第 10／13／14 步接线责任见[实现说明](implementation-4-familiar-knife-bomb.md)。

### 4.4 Tear、Pickup、Laser 本轮实现（2026-11-08）

布局版本 20，唯一源仍为 `mod/binding/state/entity-classes.json`。布局编号 125／126／127，继承 E0—E9，子类块从 10 开始。完整生成表见 [Tear](layouts/EntityTear.md)、[Pickup](layouts/EntityPickup.md)、[Laser](layouts/EntityLaser.md)，交付与后续接线见[本轮实现](implementation-4-tear-pickup-laser.md)，静态逆向及主审见[证据](tear-pickup-laser-evidence.md)。

- Tear 三份辅助 ANM2 的外观／动画／图层独立投影，七个引用槽和命中索引分别保留，不借带副作用的 Height／Scale／TearFlags setter 恢复最终状态。
- Pickup 保留两份 ANM2、四个引用槽、八项有界循环道具；FlipSaveState 为 Host 玩法保存对象，Client 不重建。价格与图形最终值分别安装，不调用 SetPrice／InitFlipState 触发玩法。
- Laser 保留原始有符号采样模式、十三个引用槽，以及单独成表的 HomingLaser；其 self 回指不发送。四份几何容器独立建模，Bezier 保留六个 f32 槽，不运行 CalculateSamples。
- 三个 Plus 的派生音效、重抽和重新采样请求归 Host，公共覆盖只经一份 E9。条件计时不凭字段名判定；未知活动分支明确报 Gap。
- 原生范围为 0x848／0x6F0／0x568，未知与 SDK 声明保持实际等级。本轮只生成布局与纯编译，不宣称类型字节完整、正式采集已接通或第 15 步验收完成。

### 4.5 Slot、Projectile、Effect 本轮实现（2026-11-09）

布局版本 23。Slot／Projectile 为 128／129；Effect 保留 101／102／103 并增加 104／105／106 三个附属臂，payload／节点为 25—28。见[实现与边界](implementation-4-slot-projectile-effect.md)、[必要逆向与主审](slot-projectile-effect-evidence.md)，完整字节和二进制单行表由生成器输出。

- Slot 八个数值与独立奖励 ANM2 分块；Timeout 保留 i16，复杂状态机计数不猜全局线性方向。奖励编号不替代最终 Sprite，不在 Client 重演购买／派奖。
- Projectile 的 DepthOffset 直接回映射到继承的 E4 `depth_offset +0x358`，仅发送一次。两项 flags 各 u64，ChangeTimeout 保存公开变更参数，不误建递减倒计时。
- Effect 普通 VarData 未核定则不复制；TearFlags／GridDesc 按变体分开。三种具体 payload 不发送虚表、共享控制块或 deque header，经单一原生 owner 安装全部同版本投影。
- `state/owned_payload.*` 有界处理 u8 present、尾块存在性依赖和缺失值最短全零规范；未知动态类型或读取失败不得当作不存在。
- SDK 大小与池步长分别记录；公开目录已回映射，根对象和 payload 未知仍在各自生成表中，不能据此宣称字节完整或第 15 步通过。

## 改动范围

- 布局源 `state/entity-classes.json`、`state/entity-effect.json` 中的子类条目和专用块；其中 Effect 按 4.1 分为 6 个变体臂布局，NPC 按 4.2 增加 Delirium 变体臂。
- Effect 的 Timeout 换算函数和 `effect_timeout` writer。
- Client 类别写入由生成表分派，取代 `ClientEntityClassState.fields` 的字符串匹配。专用容器继续沿用现有写入函数，例如 `ClientEntityCollections`。

## 失败方式

- **按 type 选布局**：只看 `_type`，把特殊虚表对象套上错误布局。
- **未知区处理错误**：大段未知区被当成填充跳过，导致关键状态漏同步（例如 NPC 的 AI 状态）。
- **借用当成 EntityPtr**：把裸借用当成 EntityPtr 读取，地址解析错误。
- **容器读取越界**：几何容器读取时没核 begin、end，或者超过上限。
- **复活拥有型对象**：在 Client 上重建了不应存在的玩法 actor，例如 NPC +0xF40。
- **按 SDK 大小划范围**：Effect 按 0x490 划定对象范围，漏掉 0x490—0x4BF 的私有容器和 ExtraData；Slot、Projectile 漏掉尾部 8 字节；Knife 按 0xE78 读取，越过对象进入池中下一个槽。
- **Effect 选错变体臂**：variant 136 按基础臂只发 16 字节，格子描述被截断；或者把四个变体以外的 VarData 当成 TearFlags。
- **Effect 直接复制 ExtraData**：复制 owner 指针或 memcpy payload，没有经游戏分配器和 `0x606310` 重建。
- **Effect 跟随关系先写**：IsFollowing 为 1 的 FX1 先于 Parent（E8）写入，而 Parent 目标还没有构造。
- **Effect 到期帧基准不一致**：两端换算 Timeout 用的帧基准不同，例如 Client 误用自己的 `Game+0x25A70`；或者装好新的 world 帧后，没有按新的 game_frame 重写 Timeout。
- **Effect 轨迹失控**：轨迹节点数没有上限或含非有限值；整条轨迹每帧重发，超出带宽。
- **Effect 格子描述重复**：举起的格子描述在 Effect 和 Grid 两处各发一份，而且内容不一致。
- **Delirium 变身漏检**：只比较 type、variant，BossType 变化没有走形态路径，Client 的旧实例继续接收新形态的块。
- **Delirium 选错臂**：采集窗口落在 `0x49F3D0` 内，`type` 是临时的 Boss 类型，对象被套上基础臂。
- **发送池槽残留**：把普通 NPC 的 `+BD8—BE7` 当状态发送，内容是上一只 Delirium 留下的值。
- **Delirium 重复换代**：`world_delirium_owner` 和字段比较各升一次 shape，同一次变身退休两次实例。

## 完成内容

- **字节归属与表格**：十个子类和专用形态的字节归属全部完成，带证据，两张单行表由生成器产出。
- **回映射**：子类全部 `ReplicatedState` 成员都有回映射。

## 未决问题

- NPC、Familiar 子类区的大段未知字节，需要先估算核对工作量，再决定拆分次序。Knife 按池步长只有 272 字节，不再属于大段未知。
- Knife、Slot、Projectile 的实际 sizeof 与池步长尾部字节的归属（填充还是未知）。
- Projectile 在 0x358 的字段已核定为公共 DepthOffset 别名，映射到 E4；不再列为未知。
- Effect：
  - 实际 sizeof 是 0x4C0 还是 0x4BC；
  - 0x484—0x493 内联容器的类型；
  - 0x41C、0x440—0x44F、0x47C—0x483、0x494—0x4A7 的用途。
- Effect 基础臂 VarData 的写入方式；0x430—0x43F 是否被其他变体使用。
- Effect 的 Timeout 是否严格每个游戏帧减 1。这只影响省下多少流量，不影响正确性。
- Effect 的 MinRadius、MaxRadius、Rotation、Scale 的实际变化频率。如果高频，移入 FX0。
- 轨迹节点 64 字节布局的 J273 核对，以及节点数上限。
- 三种 payload 以外，其他变体的 ExtraData 在 Client 上是否有消费者。
- Delirium：
  - `+BD4—BD7`、`+BDD—BDF`、`+BE4—BE7` 共 11 字节能否证实为填充；
  - 普通 NPC 的 `+BD8—BE7` 池槽残留是否真实发生；
  - DL1 的六个字段在 Client 上没有原生消费者（不运行 AI），现按 `ReplicatedState` 保留发送，第 15 步后可以再判断是否改归权威；
  - `+BE8` 归为权威的前提是 Client 无消费者；第 15 步如果发现形态 410 的画面依赖它，再改为引用。
