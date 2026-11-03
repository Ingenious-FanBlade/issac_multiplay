# I15—I19 派生实体静态逆向记录（J273）

日期：2026-11-03。范围：Laser、Knife、Pickup、Familiar、Effect。本文只记录 Wiki、REPENTOGON 源码和 IDA 静态证据；不表示适配代码或绘制已经完成，也不构成运行验收。

## 证据顺序与共同采用约束

先核对原版本地 Wiki（`F:/projects_isaac/IsaacDocs/docs/Entity*.md`），再核对 REPENTOGON 文档、`libzhl/functions` 和 `repentogon/LuaInterfaces/Entities`，最后用 J273 IDA 核对注册、访问和虚表路径。下面的 VA 均为当前 J273 IDB 地址。

对象继续由真实工厂和池构造，保留类 vtable、EntityPlus 字段及该类 ctor 建立的容器。受控调用官方 `Entity::Init`（VA `0x6A4A20`）后安装 Source 当前最终值，是新类可采用的基础方案；派生 `Init` 含资源、随机、生成和关系副作用，不整体调用。派生 ctor 建立的 ExtraData、ANM、vector、Pathfinder 等 owner 需分别保留或按已核类型创建；不能复制 Source 的 native 指针或 `std::vector` 三指针头。

各类别的 native `Render` 调度与像素验收仍属 I25/I34。本记录只给出每类的绘制前置和已知退休约束；未解释的分支维持 UNKNOWN。

## I15：EntityLaser / HomingLaser

注册位置为 `sub_84D340`，VA `0x8519C9–0x851AD9`。Getter/setter 和 REPENTOGON 文档确认的字段如下；基础 Entity 的 Parent、SpawnerEntity 等不重复列入。

| 字段 | 类型 | 偏移 | 证据/备注 |
|---|---:|---:|---|
| ParentOffset | Vector | `+0x410` | 注册直读写 |
| StartAngleDegrees | float | `+0x418` | 注册直读写 |
| AngleDegrees | float | `+0x41C` | `Angle` getter `0x844980` 读此处 |
| LastAngleDegrees | float | `+0x420` | 注册直读写 |
| TearFlags | BitSet128 | `+0x428` | 完整 16 字节 |
| Timeout | int32 | `+0x458` | 注册直读写 |
| FirstUpdate | bool | `+0x45C` | 注册直读写 |
| SampleLaser | int8 | `+0x45D` | 原始值需保留：渲染把它作为有符号 `-1/0/1` 模式读；公开 `IsSampleLaser` getter `0x7DF700` 只返回 `!=0` |
| Shrink | bool | `+0x45E` | 注册直读写 |
| LaserLength | float | `+0x460` | 注册直读写 |
| HomingLaser | inline | `+0x464` / `0x54` 字节 | 四个 vector 与几何成员见下 |
| CurveStrength | float | `+0x4B8` | 注册直读写 |
| Radius | float | `+0x4BC` | 注册直读写 |
| IsActiveRotating | bool | `+0x4C0` | 注册直读写 |
| RotationDelay | int32 | `+0x4C4` | 注册直读写 |
| RotationDegrees | float | `+0x4C8` | 注册直读写 |
| RotationSpd | float | `+0x4CC` | 注册直读写 |
| MaxDistance | float | `+0x4D0` | 注册直读写 |
| HomingType | uint32 | `+0x4D4` | 注册直读写 |
| EndPoint | Vector | `+0x4D8` | `GetEndPoint` `0x7DF6F0` 直接返回此处 |
| Scale | float | `+0x4E0` | REPENTOGON 新增；注册源码与访问器 |
| DisableFollowParent | bool | `+0x4E4` | 注册直读写 |
| BounceLaser | Entity* | `+0x4E8` | Entity 关系，按 Key 绑定 |
| BlackHpDropChance | float | `+0x4F4` | 注册直读写 |
| OneHit | bool | `+0x515` | 注册直读写 |
| GridHit | bool | `+0x516` | 注册直读写 |
| HitList | `vector<uint32>` | `+0x520` | REPENTOGON 扩展；整数身份语义/本地索引映射仍需单独处理 |
| NumChainedLasers | int32 | `+0x560` | REPENTOGON 扩展 |

`Angle` setter VA `0x945DC0` 是耦合操作：将旧 AngleDegrees 移到 LastAngleDegrees，写入 StartAngleDegrees 与 AngleDegrees，并设通用 Sprite rotation 为 `angle-90`。安装统一快照时应明确是否还需要这些 setter 副作用；不能将其误当单字段写入。`EndPoint` Getter 证明公开端点就是 `+0x4D8`，无需从 samples 反算。

官方 ZHL `libzhl/functions/HomingLaser.zhl` 给出完整内联布局：`_entity +0x00`、`_origin Vector +0x04`、`_endPoint Vector +0x0C`、`_unkFloat1 +0x14`、`_sampleNum uint +0x18`、`_samples vector<Vector2> +0x1C`、`_nonOptimizedSamples +0x28`、`_unkVec vector<int> +0x34`、`_unkInt +0x40`、`_unkFloat2 +0x44`、`_bezier vector<HomingLaser_Bezier> +0x48`。换算到 Laser 基址，四个 vector 头偏移为 `+0x480/+0x48C/+0x498/+0x4AC`。

- `Entity_Laser` ctor `0x945A90` 调 `sub_974770(this+0x464)` 建立这些容器，并在尾部执行 `this[281]=this`，即 `HomingLaser::_entity` (`Laser+0x464`) 是指向所属 Laser 的 self backpointer，不是另一 Entity 关系。保留 factory ctor 建立的值。
- Bezier 类型在官方 ZHL 只有 `HomingLaser_Bezier : 0x18`，没有具名成员。`CalculateSamples` 签名匹配的 J273 函数 `0x9768C0` 按每元素 24 字节计数，在 `float*` 上按 `6*i` 步长读取每元素 6 个 f32 槽，参与 cubic Bezier 的 x/y 插值。大小和六个 f32 槽均有消费者证据；把它们命名成三组 `Vector2` 仍属语义推断，具体成员名/生产者布局 UNKNOWN。可传完整 6-float 原值；不能缩成只含三个点的猜测结构。
- 同一 `0x9768C0` 从 Bezier 生成 Vector2 samples。客户端不调用 `CalculateSamples` / `RecalculateSamplesNextUpdate` 恢复 Source 轨迹；必须由来源携带足以绘制的最终 samples 与需要保真的 Bezier 值。不可用 native vector 头浅拷贝。
- `Init` `0x9480E0` 调 `Entity::Init` 后重设多组 Laser 状态及绘制资源，再调 `0x847C10`。`Render` `0x9514B0` 读取 SampleLaser 原始模式与 HomingLaser 几何。Remove `0x956E90` 还有多项 owner/child 清理；ClearReferences 虚函数 `0x956C60` 是 Laser 自己的 override。不得将它们当普通 base 操作互换。

## I16：EntityKnife

字段注册位于 `sub_84D340` 的 `0x851D0E–0x851D52` 段；ZHL `EntityKnife.zhl` 和对应 getter 确认：

| 字段 | 类型 | 偏移 |
|---|---:|---:|
| TearFlags | BitSet128 | `+0x410` |
| Scale | float | `+0x430` |
| RotationOffset | float | `+0x43C` |
| PathOffset | float | `+0x4C4` |
| PathFollowSpeed | float | `+0x4C8` |
| MaxDistance | float | `+0x45C` |
| Charge | float | `+0x460` |
| IsSwinging | bool | `+0x465` |
| KnifeDistance | float | `+0x444` |
| KnifeVelocity | float | `+0x448` |
| PrismApplied | bool | `+0x4E5` |
| MultiDimensionalApplied | bool | `+0x4E6` |
| IsSpinAttack | bool | `+0x4EA` |

`KnifeDistance` / `KnifeVelocity` 是方法可读的当前状态，源码不将它们列为通用 Lua property。Wiki公开 `Rotation`，但本次未确认其 native 存储是否直接等于 Sprite rotation；映射保持 UNKNOWN。ZHL 声明 `size 0xE78` 与池 stride `0x520` 不一致；不得按声明大小写越界字段。

Knife ctor `0x939AD0` 调 base ctor，随后将 PathFinder/path 状态的 inline owner 从 `+0x468` 交给 `sub_974770` 初始化。这个路径块含四个动态容器，头部在 Knife `+0x484/+0x490/+0x49C/+0x4B0`，其中末容器元素步长为 `0x18`；具体各元素所有成员仍 UNKNOWN。`InitHomingPath` `0x93A330` 会清空容器、使用方向/距离并调用 `sub_974C60` 重新生成路径；这属于 Source 玩法轨迹构造，不得在客户端重跑。

`Init` `0x939C70` 除 base Init 外会准备静态 `DeadEyeTearEffect.anm2` 全局资源/图容器（全局地址包含 `0xBE5594`、`0xBE6F48/0xBE6F4C`），并继续消耗 RNG、设关系和状态。需保留全局资源的有效生命周期；每个 Knife 不应独立释放它。如何从派生 Init 提取安全的一次性 Ensure helper 尚未核定，不能默认为派生 Init 无需资源。

Owner 查找 helper `0x93F650` 的循环条件是：从 Knife 的 `Parent` 开始，每次读取 Parent；遇到 `Type != 8` 就停止，沿 Type 8 链继续。IDA 未见 visited set、最大步数或自环检测；全 Knife 父链若成环会无限循环。不要将 `SpawnerEntity` 等同于该 Parent 链。`ClearReferences` `0x93A1E0` 会解绑若干 Parent 关系、对 `+0x4FC/+0x500` 子对象调用 Remove；含外部对象副作用，退役前要按本地实例身份处理。

## I17：EntityPickup

字段表由公开注册 `sub_84D340`、ZHL `EntityPickup.zhl`、REPENTOGON Lua wrapper 交叉确认：

| 字段 | 类型 | 偏移 | 备注 |
|---|---:|---:|---|
| Charge | int32 | `+0x524` | |
| OptionsPickupIndex | int32 | `+0x528` | |
| Touched | bool | `+0x52C` | byte |
| IsForceBlind | bool | `+0x52E` | ZHL 字段 |
| Price | int32 | `+0x534` | `SetPrice` 非纯写 |
| AutoUpdatePrice | bool | `+0x538` | |
| ShopItemId | int32 | `+0x53C` | setter 有范围裁剪 |
| Timeout | int32 | `+0x540` | |
| Wait | int32 | `+0x544` | |
| DropDelay | int32 | `+0x554` | 与 Timeout 不同 |
| State | int32 | `+0x584` | |
| PickupGhost | Entity_Effect* | `+0x590` | 跨 Entity 关系 |
| MegaChestCollectible[2] | Entity_Pickup* | `+0x594` | 两个跨 Entity 关系 |
| VarData | int32 | `+0x59C` | |
| CycleCollectibleList[8] | uint32[8] | `+0x5A0` | 写全量原数组 |
| CycleCollectibleCount | uint32 | `+0x5C0` | 不逐项调用 AddCycle |
| FlipSaveState | SmartPointer<EntitySaveState> | `+0x5C8` | owned/refcount 数据，不能浅拷贝 |
| AltPedestalANM2 | inline ANM2 | `+0x5D0` | variant 100 相关表现 owner |
| PriceANM2 | inline ANM2 | `+0x410` | 独立价格表现 owner |

`SetPrice` `0x6DF890` 写 `+0x534` 后继续按商店/资源状态执行图形、动画分支；`GetPrice` `0x42D2B0` 才是直接读取。价格的 native owner 更新要由具名资源 helper 完成，不能把 setter 当标量 leaf。

`Init` `0x6DD7C0` 有派生状态重置、Loot/ItemPool/RNG、部分变体辅助对象/资源路径，不能拿它来恢复 Source 已抽出的 collectible。`InitFlipState` 的 REPENTOGON 路径会注册/恢复 SaveState、查询 ItemPool/RNG 并配置图形/动画，同样不得用于重新生成。保存 Source 最终值时整组安装 `CycleCollectibleList + Count`，不运行 Add/RemoveCycle。Ghost、MegaChest 是 Entity 关系，按 Key 延迟绑定；不能复制 source 原生指针。

`Remove` `0x6EA840` 会 virtual Remove `PickupGhost(+0x590)` 后调 base Remove；`ClearReferences` `0x6EA860` 会处理 ghost、`+0x58C/+0x590/+0x594/+0x598` 等关系。客户端退休必须先撤掉仅属于本地的绑定，不能级联删除目标里仍活着的 ghost/子对象。渲染 `0x6E35D0` 读取 `+0x554` drop delay，variant100 消费 `+0x5D0`；另有读取 global RNG 的 wobble 支路，故不应调用更新来确定绘制后态。

## I18：EntityFamiliar

公开字段偏移（Lua 注册/ZHL/IDB accessor）：

| 字段 | 类型 | 偏移 |
|---|---:|---:|
| Player | Entity_Player* | `+0x410` |
| FireCooldown | int32 | `+0xD4C` |
| HeadFrameDelay | int32 | `+0xD50` |
| MoveDirection | Direction/int32 | `+0xD54` |
| ShootDirection | Direction/int32 | `+0xD58` |
| LastDirection | Direction/int32 | `+0xD5C` |
| OrbitAngleOffset | float | `+0xD60` |
| OrbitDistance | Vector | `+0xD64` |
| OrbitSpeed | float | `+0xD6C` |
| OrbitLayer | int32 | `+0xD70` |
| IsFollower | bool | `+0xD74` |
| IsDelayed | bool | `+0xD75` |
| Coins / Hearts / Keys | int32 each | `+0xD7C/+0xD80/+0xD84` |
| State | int32 | `+0xD88` |
| RoomClearCount | int32 | `+0xD8C` |
| MoveDelayNum | int32 | `+0x5F8` |
| Pathfinder | inline owner | `+0x5FC` |
| Weapon* | pointer | `+0xDB0` |
| IsLilDelirium | bool | `+0xDB4` |
| ItemConfig* | pointer | `+0xED4` |
| DirtColor | ColorMod | `+0xEDC` |
| WispCollectibleType | int32 | `+0xF0C` |

Familiar ctor `0x606F00` 建立不能跳过的 native 基础：base ctor、60 组容器字清零、`sub_7D97D0` Pathfinder 初始化、`+0xECC/+0xED0` ExtraData shared-owner 清零，以及 `+0xF28` owner-list sentinel 分配。保留 factory ctor；Familiar 池 stride `0xF30`。

Player setter `0x627920` 会从旧 orbit/follower/delayed 维护器解绑并对新 Player 重新登记，不是裸字段 setter。`Remove` `0x60A040` 先执行 orbit/follower/delayed 维护，再按 variant 更新 Player 计数/位置/网格。其 helper `0x60BA20`、`0x626FE0`、`0x627320`、`0x60D9A0`、`0x60D6C0` 具有计时、全局/Player 列表或关系操作；没有核定出可单独调用且只维护链表的安全 helper。普通已同步关系不得假定可调用派生 Remove/Clear。

Familiar `ExtraData` 是 `+0xECC` raw pointer、`+0xED0` control block 的共享 owner；ClearReferences `0x60B800` 释放它、Weapon、`+0xDAC` 子对象和 `+0xD9C` vector storage。它还会处理 variant900、variant206/237 subtype557 等 Player/Grid 特例；含真实副作用，需在本地实例与引用已安全分离后走专用退休处理。

| Render 分支 | 私有 owner / 静态证据 | 安装要求与状态 |
|---|---|---|
| variant 48 (`0x30`) | `IsaacsBodyData` vtable `0xB14F84`；`Init 0x607AA0` 在 `0x60921A–0x609281` 手工分配 `0x578` bytes：`+0` vptr、`+4` 一个 ANM2、`+0x118` 的 ANM2[4]、`+0x568` 的 int32[4]。setter `0x430E40(+0xECC, owner)`；dtor `0x6071F0` | ctor 手工写 vptr，`sub_402B00(+4)` 建第一个 ANM2、EH array ctor 在 `+0x118` 建4个 ANM2。Render `0x61E360` 读四个 int32 selector（负数跳过）并消费对应 ANM2，需完整动画/索引 typed payload。 |
| variant 231 (`0xE7`) | `AbyssLocustData` vtable `0xB1510C`；`Init 0x607AA0` 在 `0x60905D–0x609131` 手工分配 `0x58` bytes；setter `0x430E40(+0xECC, owner)`；dtor `0x607280` | 构造手工写 vptr；`+0x1C` 分配 `0x18` 红黑树 sentinel；两个 `std::vector<ANM2>` 头在 `+0x24/+0x30`，元素步长 `0x114`，Render 分别读 begin/end 并经 `0x647310` 处理/绘制每个 ANM2；`+0x40` 是由 `0x4101E0` 释放的 12-byte header（4-byte 元素类型 UNKNOWN）；`+0x4C` 是 dtor 从外部 owner tree 摘除的关联指针；`+0x50` bool。Init 还查询 Player/配置、载入 `hungry_0.png` 并建立关系/计时状态，需 Source typed payload，不跑完整 Init/Update。 |

`Weapon* +0xDB0` 会被公开 GetWeapon 读取并由 ClearReferences 释放；本次 `Render 0x61E360` 未见直接读取该字段。渲染中存在通过 Player 查 Weapon 的特例（调用 `0x60C1F0` 查 UmbilicalWhip）；这不等于要求给每个 Familiar 构造 `_weapon`。在 Client 不跑 Familiar Update 的范围内，Weapon 可作为 Authority 纯值保留；只在具体 Render 分支确实要消费时才要求本地 owner。不要复制 Weapon 指针。`UmbilicalBabyData` RTTI 在模块中存在，但本次 Render 未找到对它的动态读取，相关分支/资源 UNKNOWN。

Familiar ExtraData shared_ptr setter 是 `0x430E40`，control-block vtable `0xB0F2CC`；Base ExtraData vtable `0xB0F2A4`。两个派生 payload 的析构函数都以 `a2&1` 决定释放 outer block，正常 shared owner 通过 control-block release 调虚析构。variant231 的 dtor 会对 `+0x4C` 做外部 tree unlink；应用层必须保证该关联已在本地建立并以同 owner 解绑，不能随便复制 Source link。

## I19：EntityEffect

公开基础字段由注册、ZHL 和方法 accessor 交叉确认：

| 字段 | 类型 | 偏移 |
|---|---:|---:|
| Height | float | `+0x410` |
| FallingSpeed | float | `+0x414` |
| FallingAcceleration | float | `+0x418` |
| VarData | 16-byte storage | `+0x420` |
| State | int32 | `+0x450` |
| MinRadius / MaxRadius | float | `+0x454/+0x458` |
| Timeout | int32 | `+0x45C` |
| LifeSpan | int32 | `+0x460` |
| IsFollowing | bool | `+0x464` |
| ParentOffset | Vector | `+0x468` |
| DamageSource | int32 | `+0x470` |
| Rotation | float | `+0x474` |
| Scale | float | `+0x478` |

`FollowParent` `0x4522E0` 会解绑旧 Parent、保存新 Parent、设置 IsFollowing/ParentOffset 并维护关联，不能当 `+0x3BC` 指针写入。VarData 是变体复用存储，不统一解释：官方 wrapper 只允许 variant54/113/126/167 读写 TearFlags；variant136 是从 `+0x420` 开始的完整 `GridEntityDesc`，按 I24 owner 合同处理。Effect `Render 0x600E60` 本身未发现直接读取普通 VarData；不意味隐藏 ExtraData 已可忽略。

Effect ctor `0x5CCEA0` 保留基础 class owner：调用 base ctor，初始化 inline containers（含 `+0x484/+0x488/+0x48C/+0x490` 与 `+0x4A8/+0x4AC/+0x4B0`），并清零 `+0x4B4/+0x4B8`。`+0x4B4/+0x4B8` 是 ExtraData shared owner 两字结构。`sub_606310(+0x4B4, p)` 分配0x18引用计数 owner，替换当前 payload 时经虚方法处理旧对象；不能只把 payload 指针写入第一字。

`Init 0x5CD070` 是4KB+派生 switch：除 base Init 外会建资源/动画/附属实体、设置变体初态和其他游戏路径。使用受控 base Init 后安装公开后态。ClearReferences `0x6044A0` 在 base Clear 后调用 `sub_606310(+0x4B4,0)` 清 ExtraData。此 Clear 还有两个具体分支：variant179 且 `Target(+0x3C4)` 存在并指向 Type1 entity 时调 `sub_7C9C10(1)`；variant193 且 alive/标记条件通过时调 `sub_908140(...,503)`。这里读的是 Entity.Target，不是 SpawnerEntity。清空/退休 Target 后不应误入前者。Remove `0x5D1860` 会遍历全局 Effect 列表查找variant60/161并可调 `sub_908140` 使用音效 ID `0x345/0x34E`；它是跨对象/音频路径，不能默认作为纯本地 retired 标记。

Render 使用且 Init 可建立的 typed ExtraData 当前核定三族：

| Variant | Owner | 静态证据与必要字段 | 状态 |
|---:|---|---|---|
| 108 (`0x6C`) | `MrMeData` | vtable `0xB14C0C`；Init `0x5CF2A1` 分配 `0x128` bytes，ctor `0x5CCE00`；setter `0x606310(+0x4B4, owner)`；dtor `0x5CCE60` | 布局：`+0` vptr，`+4` 未知/填充，`+8` ANM2 `0x114`，尾部 `+0x11C..+0x127` 12 bytes UNKNOWN。ANM2 ctor `0x402B00`，dtor `0x403790`。Render 仅 variant108且 State==2 dynamic_cast，测试 owner 内 `+0x111` 状态位并调用 `sub_4059E0(owner+8, position)`；Source需给 ANM2 的资源/动画最终态。 |
| 166 (`0xA6`) | `SpriteTrailData` | vtable `0xB14BE0`；Init `0x5D067D` 分配 `0x18` bytes 并调用手工 ctor `0x5D1720`；setter `0x606310(+0x4B4, owner)`；dtor `0x5D17B0` 调 `0x606270(+4)` | 布局：`+0` vptr，`+4` 指向内部8-byte sentinel，`+8/+0xC/+0x10/+0x14` 初始化为0。Render检查`+0x14 >= 2`，从`+0x10`与`+0x14`计算trail区间，再按键查询内部hash/bucket结构；每个查得记录至少0x40 bytes，Render读取记录`+0/+4`坐标、`+8/+0x18/+0x28/+0x30`片段和`+0x34/+0x3C`浮点。具体hash值/记录结构字段和来源语义 UNKNOWN；不能只填计数或浅拷贝 sentinel。 |
| 191 (`0xBF`) | `PickupGhostData` | vtable `0xB14BF8`；Init `0x5D0FF2` 分配 `0x458` bytes并手工置 vptr；setter `0x606310(+0x4B4, owner)`；dtor `0x5D17E0` | 布局：`+0` vptr，`+4` ANM2[4] (`0x114` each)，`+0x454` int32 count。Init通过EH array ctor以`0x402B00`构造4个ANM2；dtor逐个调用`0x403790`。Render case191检查count>0，随后读取目标/宿主状态；这段 Render 未见直接按索引读数组，数组仍是 owner 与更新路径的有效状态。 |

上表的 RTTI 消费由 `0x600E60` 确认；当前只列已看到的三种 Render 动态 payload。其他 Effect 分支的专用容器/子实体/资源分组未闭合。不能将 `Effect57` 或基础 Sprite 绘制推广为全 Effect 支持；I25 应按具体已提供 payload owner 分派并保留未实现分支。

Effect ExtraData base vtable 为 `0xB14C2C`，shared control-block vtable `0xB14BA0`，native setter/replacer `0x606310`，Clear `0x6044A0`。`MrMeData` ctor/dtor、`SpriteTrailData` 手工 ctor/dtor、`PickupGhostData` 手工 array ctor/dtor如上。三个 payload 都以游戏 native outer allocator 建立，再交该 shared owner 统一析构；不要 memcpy owner 或伪造 vtable。

### Native allocator 交接

当前 IDB 的 `operator new(size_t)` 是 `0xA86DE5`，其实现循环调用 CRT `malloc(Size)`；`free_` 是 `0xA8717B`，转到 CRT `free` import `0xAAA58C`（`api-ms-win-crt-heap-l1-1-0`）。上述 ExtraData 外层、sentinel 和 ANM2 数组由这套 allocator 路径分配，dtor/vector dtor 用同一 CRT free。仓库 `mod/CMakeLists.txt` 将 CMake runtime 设为 `MultiThreadedDLL`（动态 `/MD`），与 J273 导入的 UCRT heap API 路径兼容；仍建议 owner 分配/释放走游戏可调用的 `operator new/free` 包装，避免其他构建配置切到静态 CRT 后跨堆释放。ANM2 一律调用原生 ctor/dtor。

## 当前未决项

- Laser Bezier 的六个 f32 消费槽已核定，具体具名成员与生产端布局仍待来源侧核定；Laser 的 source samples/Bezier 转移和 native vector allocator/销毁由适配实现接入，不能在本静态文档声明完成。
- Knife `Rotation` 的 native 语义、DeadEye 全局资源的安全一次性 Ensure 路径、HomingPath 四容器全部元素布局仍 UNKNOWN。
- Pickup FlipSaveState 的来源类型/安装/生命周期、价格与 variant100 ANM2 的安全 native owner 更新 helper、PickupGhost/MegaChest 子对象 key 语义需继续核定。
- Familiar Player 关系的安全局部维护入口未找到；variant48/231 两份 typed payload 的实际源字段、ANM2 状态与资源完整性需进入 Source 协议。`Weapon` 仅确认本次 Familiar Render 未直接访问 `_weapon`，不覆盖 Update/通用玩法调用。
- Effect 三族 ExtraData 的源 payload 尚未定位/补足；其他 Variant 的私有容器与关联 owner 尚未逐一覆盖。I24继续负责 variant136 descriptor/网格所有权。


## 主智能体后续复读与采用补证

主审复读4D13C0/5B3350、606C10，确认Knife Rotation为float +438，IsFlying为byte +464；前文UNKNOWN保留研究当时的判断。主审将Effect179的Target关系、Laser/Effect实际Clear override、HomingLaser self backpointer及六f32槽纳入实现。

主审另外复读SpriteTrail Render与606270析构，确认该结构是带proxy的MSVC deque布局（+4 proxy、+8 map、+C map capacity、+10 offset、+14 count），原“hash”说法仅是子研究的中间推断；记录没有指针成员，当前采用完整64B/16个u32位模式及浮点有限性，按逻辑顺序复制到本地deque，不复制地图/块指针。主审复读60905D—609131、60921A—609281和607280/6071F0，按真实inline构造和原生ANM ctor、shared-owner helper、实际dtor接入两种Familiar payload；Abyss +4C的清理使用Entity反向引用登记(+3CC)，不将其误写成红黑树节点或复制Source关联。游戏专用未消费表/关联保持Authority事实，本地为空。

五类Source及原生owner现已实现，前文“尚未补齐”仍反映静态研究当时状态；当前采用范围、纯值去向、具体拒绝与后续责任见[本轮实现](tx-04-19-laser-effect-application.md)。原始私有字段具名语义、任意第三方Mod、未知异常与完整绘制仍不能从这些静态补证外推；本轮没有运行构建或测试。
