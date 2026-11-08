# Familiar、Knife、Bomb 静态证据

日期：2026-11-08。目标为 J273 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA ImageBase `0x400000`。本记录来自只读 IDA、REPENTOGON v1.1.3 SDK／源码和本地 IsaacDocs Wiki；没有改名、改类型或保存 IDB，没有运行游戏、测试、校验脚本或哈希。

本记录覆盖 Entity 子类区。Entity 公共前缀 `[0,0x410)` 由第 3 步处理。所有对象上界采用 J273 `EntityFactory` 池步长：Familiar `0xF30`、Knife `0x520`、Bomb `0x9D8`。Knife SDK 的 `Entity_Knife : 0xE78` 超出 J273 池步长，越界成员不成立。以下未证实的区域保留为 UNKNOWN；构造函数清零不等于填充。

## 入口、RTTI 与虚函数

| 类 | 池步长 | J273 vtable / TypeDescriptor | 构造 | Init / Update / Render | 清理点 |
| --- | ---: | --- | --- | --- | --- |
| Familiar | `0xF30` | `0xB15090` / `0xBD1310` | `0x606F00` | `0x607AA0` / `0x6170F0` / `0x61E360`；Shadow `0x61E320` | Kill `0x60A0F0`；Remove `0x60A040`；ClearReferences `0x60B800` |
| Knife | `0x520` | `0xB2D7A0` / `0xBD1590` | `0x939AD0` | `0x939C70` / `0x93AAF0` / `0x93ECD0` | ClearReferences `0x93A1E0` |
| Bomb | `0x9D8` | `0xB18118` / `0xBD149C` | `0x69D4F0` | `0x69D6C0` / `0x69DB30` / `0x6A0F90` | 派生 ClearReferences 使用 Entity `0x6A8060` |

RTTI、vtable 槽及对应入口已由主审在当前 IDB 独立复读。Knife 的 `0x939AE5` 是 `lea ecx,[esi+0x468]`，不是构造调用；真正调用 `sub_974770` 的指令在 `0x939B63`。

## Entity_Familiar：`[0x410,0xF30)`

| Offset | Size | 类型／名称草案 | 类别 | 证据与边界 |
| --- | ---: | --- | --- | --- |
| `0x410` | 4 | `Entity_Player* _player` | 裸借用引用 | ZHL；Init 从 Spawner `+0x3C8` 找 Player，并在 `0x6086A4` 等路径读取 `Familiar+0x410`。不是 EntityPtr，不能按 EntityPtr 解析。Render `0x61E360` 也读取此借用。 |
| `0x414` | 4 | UNKNOWN | 未知 | ctor 的 60×8 清零区从 `+0x418` 开始，不覆盖此处。 |
| `0x418—0x5F7` | `0x1E0` | ctor 清零的未知区 | 未知 | ctor `0x606F38/0x606F44—0x606F64` 清零 60 个 8-byte 单元；没有据此判为 padding。 |
| `0x5F8` | 4 | `int _moveDelayNum` | 状态 | `EntityFamiliar.zhl` 与现有 Client 字段表；REP Wiki GetMoveDelayNum／SetMoveDelayNum 定义为相对 Player 的移动延迟帧数参数，不能从名字添加递减倒计时语义。 |
| `0x5FC—0xD4B` | `0x750` | `NPCAI_Pathfinder _pathfinder` | 权威（SDK 声明） | `NPCAI_Pathfinder.zhl` 明确大小 0x750；主审更正初稿的 0x778。ctor `0x606F6D` 对 `this+0x5FC` 调 PathFinder ctor；Host寻路状态不在Client执行，内部回指／marker不能整体 memcpy。 |
| `0xD4C—0xD4F` | 4 | FireCooldown | 换算／条件待核 | 公开 i32；已有多种更新分支，未核活动条件报缺口。 |
| `0xD50—0xD53` | 4 | HeadFrameDelay | 状态 | 已有公开 i32 字段映射；计时语义未闭合，因此直接复制原始值，不要求方向或线性时钟。 |
| `0xD54—0xD5F` | `0xC` | MoveDirection、ShootDirection、LastDirection | 状态 | 已有 I18 研究与 `client_entity_class_fields.inc`，三份 i32。 |
| `0xD60—0xD63` | 4 | OrbitAngleOffset | 状态 | 已核公开 float。 |
| `0xD64—0xD6B` | 8 | OrbitDistance | 状态 | 已核公开 Vector。 |
| `0xD6C—0xD6F` | 4 | OrbitSpeed | 状态 | 已核公开 float。 |
| `0xD70—0xD73` | 4 | OrbitLayer | 状态 | 已核公开 i32。 |
| `0xD74—0xD75` | 2 | `IsFollower`, `IsDelayed` | 状态 | ZHL；Init 读取／写入这些标记。 |
| `0xD76—0xD7B` | 6 | UNKNOWN | 未知 | Init 写 `+0xD76—0xD78` 附近值；未命名部分不当填充。 |
| `0xD7C—0xD87` | `0xC` | Coins、Hearts、Keys | 状态 | 已有 I18 研究、应用目录和 Client 字段表已核为三份 i32；主审更正初稿漏查的映射。 |
| `0xD88—0xD8B` | 4 | `int _state` | 状态 | ZHL；Init 在多类分支设值／清零。 |
| `0xD8C—0xD8F` | 4 | RoomClearCount | 状态 | 已有 I18／Client 字段表已核。 |
| `0xD90—0xD9B` | `0xC` | UNKNOWN | 未知 | Init 使用 `+0xD90/+0xD94/+0xD98`；具体类型未闭合。 |
| `0xD9C—0xDA7` | `0xC` | vector header 候选 | 容器／未知元素 | ctor 初始化三个连续 dword；析构 `0x60719F` 调 vector dtor `0x4101E0`。元素语义及消费者待核，不发送原生头。 |
| `0xDA8—0xDAB` | 4 | UNKNOWN | 未知 | Init 写入此处；完整语义未核。 |
| `0xDAC—0xDAF` | 4 | NativeFamiliar.Related | 引用 | `client_entity_links.inc` 已核独立 EntityPtr 槽；`60B800` 在 variant240 还会 Remove 该目标，Client 退休前先撤边，不把此关系丢成 Null。 |
| `0xDB0—0xDB3` | 4 | `Weapon* _weapon` | 本地引用 | ZHL；Init `0x607F79` 清零。是本进程对象指针，不作为远端裸地址。 |
| `0xDB4` | 1 | `_isLilDelirium` | 状态候选 | ZHL；周边 `0xDB5—0xDB7` 未知。 |
| `0xDB5—0xDB7` | 3 | UNKNOWN | 未知 | 未证明为对齐填充。 |
| `0xDB8—0xECB` | `0x114` | 内嵌 ANM2 | 资源／状态 | ctor `0x606FCB` 在 `this+0xDB8` 调 ANM2 ctor；析构路径 `0xA8DA77` 调 `0x403790`。不同于主 Sprite `Entity+0x48`。 |
| `0xECC—0xED3` | 8 | Familiar ExtraData owner 区 | 本地 owner／附属表现 | ctor 初始化此处；Render 对 Variant 48（IsaacsBodyData）和 231（AbyssLocustData）有额外 ANM owner。不要复制 owner 指针／虚表；按独立 payload 和正式 owner 重建。内部控制字段尚未逐字节命名。 |
| `0xED4—0xED7` | 4 | `ItemConfig_Item* _item` | 资源 | ZHL；Init 清空。跨进程需映射稳定资源标识，不发送指针。 |
| `0xED8—0xEDB` | 4 | UNKNOWN | 未知 | ctor 清零；析构访问此处相关 owner 状态，尚未核实完整关系。 |
| `0xEDC—0xF07` | `0x2C` | `ColorMod _dirtColor` | 状态 | SDK 声明与已核公开字段表；ColorMod 为 44B，不能按邻近 ctor 写入范围扩大为 48B。 |
| `0xF08—0xF0B` | 4 | UNKNOWN | 未知 | ctor 在 `0x607066` 清零，不证明是 ColorMod 尾部或填充。 |
| `0xF0C—0xF0F` | 4 | `_wispCollectibleType` | 资源／状态候选 | ZHL 声明；当前尚未复核此字段在目标函数中的完整读写与消费条件。 |
| `0xF10—0xF23` | `0x14` | UNKNOWN | 未知 | Init 写 `+0xF10/+0xF14/+0xF18—0xF20`；未给出语义，不据初始化值命名。 |
| `0xF24—0xF27` | 4 | UNKNOWN | 未知 | ctor `0x607070` 写10；完整计数／参数语义未核。 |
| `0xF28—0xF2B` | 4 | 本地树 sentinel 指针 | 本地 | ctor `0x60708C—0x6070A2` 分配 `0x18` sentinel node。Update `0x61DB46—0x61DC34` 以 Target `+0x3C4` 经 `0x4521A0` 得到 Entity Index（`+0x20`）作为 key，并增减 map value。集合用途尚未闭合；不要按 IDA 模板名推成 mutex，也不发送节点地址。 |
| `0xF2C—0xF2F` | 4 | UNKNOWN | 未知 | 与上述树实现有关但语义未核。 |

### Familiar 计数与 RNG

- `+0xD4C` 是同一 Update 内被不同分支改写的计数／相位值，不能仅按名字统一锚定：`0x618B38` 设 30；`0x618BF0/0x618BF3` 递减并在 `0x618BF9` 检查旧值，非正时 `0x618C01` 重置 30；另一路 `0x618E7D—0x618E8C` 仅在正值时递减；`0x6198EC/0x61990D` 通过 helper 递增／写回；`0x61998D`、`0x61A014` 还会清零。其变体／分支合同应在确定帧锚点前保留为缺口。
- `+0xD50` 由 Init `0x607C17` 清零；当前 Update `0x61D0A9` 仅见比较，完整写入者与语义 UNKNOWN，不认作计时器。
- Familiar Init `0x607FA1` 消耗随机数。Render `0x61E533/0x61E5A3` 对全局 RNG `0xBD0674` 读写；Render 也临时改颜色等绘制状态。客户端不能通过运行完整 Familiar Render 来生成快照或缓存。
- `+0xF28` 的树节点地址和本地 header 是进程实现细节；Update 对 key/value 的行为与 Render 绘制输入不等价，不能仅凭 `void*` 或 `_Mutex_count_pair` 模板文本断定容器的玩法语义。

## Entity_Knife：`[0x410,0x520)`

| Offset | Size | 类型／名称草案 | 类别 | 证据与边界 |
| --- | ---: | --- | --- | --- |
| `0x410—0x41F` | `0x10` | `BitSet128 _tearFlags` | 状态 | `EntityKnife.zhl`。 |
| `0x420—0x42F` | `0x10` | UNKNOWN | 未知 | 未命名；不能因位于 flags 后面就视为填充。 |
| `0x430—0x433` | 4 | Scale | 状态 | 已有 I16／Client 字段表与公开访问器。 |
| `0x434—0x437` | 4 | UNKNOWN | 未知 | 未命名。 |
| `0x438—0x43F` | 8 | Rotation、RotationOffset | 状态 | 已有 I16 主审追加的公开 getter 证据与 Client 字段表。 |
| `0x440—0x443` | 4 | UNKNOWN | 未知 | 未命名。 |
| `0x444—0x44B` | 8 | `knifeDistance`, `knifeVelocity` | 状态候选 | ZHL，两个 f32。 |
| `0x44C—0x45B` | `0x10` | UNKNOWN | 未知 | 未闭合的派生状态。 |
| `0x45C—0x463` | 8 | MaxDistance、Charge | 状态 | 已核公开 f32 字段。 |
| `0x464` | 1 | IsFlying | 状态 | 既有主审 getter `0x606C10` 和 Client 字段表；不是 Entity 通用飞行覆盖。 |
| `0x465` | 1 | `_isSwinging` | 状态候选 | ZHL。 |
| `0x466—0x467` | 2 | UNKNOWN | 未知 | 未证明为 padding。 |
| `0x468—0x4BB` | `0x54` | 内嵌私有对象，内部布局 UNKNOWN | 状态／未知 | ctor `0x939B63` 调 `sub_974770(this+0x468)`；该 helper 清零对象内若干 dword，最远写至 `+0x50`，但 `+0x14/+0x18/+0x40/+0x44` 未写。Knife Update `0x93C74E/0x93C755` 将两个 Knife 的 `+0x468` 传给 `sub_844610`。Render `0x93ECD0—0x93F567` 未见直接访问此区；不得按完整空白或动画 owner 处理。 |
| `0x4BC—0x4C3` | 8 | UNKNOWN | 未知 | 未命名且未证明为填充。 |
| `0x4C4—0x4CB` | 8 | PathOffset、PathFollowSpeed | 状态 | 两个 SDK／公开字段都在 0x520 之内；主审更正初稿的越界误判。 |
| `0x4CC—0x4D7` | `0xC` | UNKNOWN | 未知 | 尚未命名。 |
| `0x4D8—0x4E3` | `0xC` | `std::vector<uint> _hitEntities` header | 容器 | ctor `0x939B68/0x939B74/0x939B7E` 清零三指针。`GetHitEntities` 的 J273 函数 `0x93A2F0` 沿 Parent `+0x3BC` 追 Type 8 链，按 `+0x4E7` 条件找到根刀后返回 `root+0x4D8`（`0x93A300—0x93A321`）。公开视图不一定来自当前 Knife 本体；布局保存各本体纯值vector，再用完整成员图推导公开视图，缺失或环报Gap。 |
| `0x4E4` | 1 | UNKNOWN | 未知 | getter 附近没有足够证据命名。 |
| `0x4E5` | 1 | `_prismApplied` | 状态候选 | ZHL。 |
| `0x4E6` | 1 | `_multidimensionalApplied` | 状态候选 | ZHL。 |
| `0x4E7` | 1 | Parent/root 选择字节（原始 u8） | 状态／推断 | `0x93A2F0` 按0／非零决定是否继续沿 Parent 找根 vector。完整业务语义未闭合，不提升为“已核”，也不假定只出现0／1。 |
| `0x4E8—0x4E9` | 2 | UNKNOWN | 未知 | 未证明为对齐。 |
| `0x4EA` | 1 | `_isSpinAttack` | 状态候选 | ZHL。 |
| `0x4EB—0x4F3` | 9 | UNKNOWN | 未知 | 未命名。 |
| `0x4F4—0x4FB` | 8 | NativeKnife.LinkedA／LinkedB | 引用 | `client_entity_links.inc` 已核两份独立 EntityPtr；`93A1E0` 撤反向登记，现恢复进新布局，不复制指针值。 |
| `0x4FC—0x503` | 8 | NativeKnife.ChildA／ChildB | 引用 | 旧路径已同步并经 SetReference 安装；ClearReferences `0x93A1E0` 还会 Remove 两个目标。关系必须保留，退休先撤边再退独立实例，不能因此归为未知不采集。 |
| `0x504—0x51F` | `0x1C` | UNKNOWN | 未知 | 池步长内尾段，没有证据可定为 padding。 |

Knife 的公开 `PathOffset +0x4C4`、`PathFollowSpeed +0x4C8` 在对象范围内；错误的是 SDK 的总大小 0xE78，不能由此排除范围内的合法字段。Knife Init `0x939C70` 通过进程级 `Source_0` 注册并加载 `DeadEyeTearEffect.anm2`，不是每个 Knife 内的 ANM2 owner；Init 在 `0x93A00B` 消耗 RNG。Render `0x93ECD0` 通过 Parent `+0x3BC` 查找 Player-like 绘制 owner；Spawner `+0x3C8` 与 Parent 不同。

## Entity_Bomb：`[0x410,0x9D8)`

| Offset | Size | 类型／名称草案 | 类别 | 证据与边界 |
| --- | ---: | --- | --- | --- |
| `0x410—0x413` | 4 | `_explosionCountdown1` | 状态／模式待核 | ZHL；Init 与 Update 按 variant 使用不同更新路径。 |
| `0x414—0x417` | 4 | 第二计数候选，语义 UNKNOWN | 未知 | SDK `SetExplosionCountdown` `0x44EAC0` 与 `+0x410` 一起写入；没有证据闭合 getter、递减条件和保存合同。 |
| `0x418—0x41B` | 4 | `ExplosionDamage` | 状态候选 | 本地 Wiki/注册访问器。 |
| `0x41C—0x41F` | 4 | `_fallSpeed` | 状态 | ZHL；旧名称曾误标 Height，现名为 fall speed。 |
| `0x420—0x423` | 4 | `_fallAcceleration` | 状态 | ZHL；此处不是 Flags。 |
| `0x424—0x427` | 4 | UNKNOWN | 未知 | 未命名。 |
| `0x428—0x42F` | 8 | `_rocketAngle`, `_rocketSpeed` | 状态候选 | ZHL 两个 f32。 |
| `0x430—0x433` | 4 | UNKNOWN | 未知 | 未命名。 |
| `0x434—0x437` | 4 | `_scale` | 状态候选 | ZHL。 |
| `0x438—0x447` | `0x10` | `BitSet128 Flags` | 状态 | 注册和访问器核对；getter `0x5CCB20` 读 16B，setter `0x6A1E40` 写 16B。 |
| `0x448` | 1 | `IsFetus` | 状态 | 注册访问器 `0x42D230/0x606AB0` 读写该 byte。 |
| `0x449—0x44B` | 3 | UNKNOWN | 未知 | 未证明为填充。 |
| `0x44C—0x44F` | 4 | `RadiusMultiplier` | 状态候选 | 注册访问器 `0x606AC0/0x431500`。 |
| `0x450—0x45B` | `0xC` | UNKNOWN | 未知 | 其中 Init／Render 特殊状态尚未逐 byte 命名。 |
| `0x45C` | 1 | `_prismTouched` | 状态候选 | ZHL。 |
| `0x45D—0x461` | 5 | UNKNOWN | 未知 | 不猜填充。 |
| `0x462` | 1 | Costume draw enable | 状态候选 | Render `0x6A10F2` 检查并控制五个 costume owner 绘制。不是 pending 标记。 |
| `0x463` | 1 | `_loadCostumes` pending | 本地资源构建标记 | Flags setter `0x6A1E47` 置位；Render `0x6A0FBB—0x6A0FC4` 检查后可调 lazy rebuild `0x6A2420`。安装来源最终 ANM 后应按适配流程阻断此重建分支。 |
| `0x464—0x9C7` | `0x564` | `ANM2 _bombCostumesSprites[5]`，stride `0x114` | 资源／状态 | ctor `0x69D56B` 对 5 项调用 EH ctor iterator；Render 使用主 Sprite 与五份 costume owner。`0x6A2420` 会 Reset/Load/LoadGraphics；不要用 lazy helper 生成客户端姿态。 |
| `0x9C8—0x9D3` | `0xC` | `std::vector<uint> _hitList` header | 容器 | ctor `0x69D570/0x69D57C/0x69D586` 清零三个 dword；官方 `LuaEntityBomb.cpp::Lua_BombGetHitList` 按 vector<uint> 枚举。发送原始Host索引纯值，不发送容器指针，不当作跨进程身份。 |
| `0x9D4—0x9D7` | 4 | UNKNOWN | 未知 | 池步长内尾段，没有填充证据。 |

### Bomb 计数、变体与随机数

- `+0x410` 不能统一解释成按帧递减倒计时。Init `0x69D6C0` 对 Variant 3/4/8/18 用 common Entity RNG（`+0x3DC—0x3E8`）生成 `seed % 30` 并加到 `+0x410`（`0x69D727—0x69D78C`）；Variant 15 设为 94，Variant 17/20 设为 87。Update 有 `+0x410` 增量路径（`0x69DB30`、`0x69DCB6/0x69DCB7`）和带条件的递减（`0x69F7F2`）；另有 `0x6A0A06` 比较 Game frame 与 `+0x410` 的路径，完整 variant/计时合同未闭合。不要统一转成到期帧。
- `Flags` 是 `+0x438` 的 BitSet128；`+0x420` 是 fall acceleration。Flags setter还会置 `+0x463`，因此安装最终 flags 不应用 setter 重演副作用。
- Bomb ctor `0x69D4F0` 先构造 Entity，再在 `0x69D537` 写 Bomb vptr，并构造五个内嵌 ANM2；`+0x9C8` 是另一容器头。更新随机数和资源访问都不是可直接跨进程复制的指针状态。

## REPENTOGON 扩展与建议块

REPENTOGON 的 `repentogon/Patches/EntityPlus.h` 有 `EntityFamiliarPlus`（`cachedMultiplier`、`cachedMultiplierPlayer`）和 `EntityKnifePlus`（`hitboxSource`），没有 `EntityBombPlus`。这些是伴随 `EntityPlusHolder` 的对象，不在上述原生池步长内；Base EntityPlus 公共字段随 E9 处理一次，子类布局不要再次序列化同一份基类字段。KnifePlus `hitboxSource` 是 EntityPtr 关系，不发送裸地址。

两种原生 Familiar payload 的 RTTI 已由主审直接复读：IsaacsBodyData vtable `B14F84` → COL `B43798` → TypeDescriptor `BD169C`（RVA `7D169C`），名字 `.?AVIsaacsBodyData@IsaacRepentancePlus@@`；AbyssLocustData vtable `B1510C` → COL `B43728` → TypeDescriptor `BD16D0`（RVA `7D16D0`），名字 `.?AVAbyssLocustData@IsaacRepentancePlus@@`。它们有原生 RTTI，布局不使用“无 RTTI”的通用占位文字；Host 对共享 owner 的动态 payload 仍须核具体类型。

块拆分建议（草案）：

- Familiar：把已证标量/关系、`+0xDB8` ANM2、Variant 48/231 附属 payload 分开；`+0xD4C/+0xD50` 先保留原值和明确缺口，不做未经证明的锚点转换；PathFinder、向量和树头只交给有专用 owner 的读取器，禁止 memcpy 原生头。未知区不参与状态复制。
- Knife：TearFlags、已核 scalar、`+0x468` 更新私有对象、`+0x4D8` hit list、`+0x4FC/+0x500` 引用分别分块/分 owner；`+0x468` 内部布局未核，HitEntities 是根 Knife 的容器视图。对 Parent 关系循环和引用退休保持显式拒绝/撤边策略。
- Bomb：计数与运动状态、Flags/公开标量、五份 costume ANM2、hit list 分块；`+0x410/+0x414` 保留原计数并列明 variant 缺口。渲染资源以 Source 最终姿态为准，客户端不调用 `0x6A2420`。

## 主智能体最终审阅

主智能体独立读取当前 IDB 的三份 RTTI 链、虚表槽、三类构造、Knife owner helper、命中表 getter，以及 Familiar／Bomb 计数器关键原始指令，并复读官方 SDK、LuaEntityKnife.cpp、LuaEntityBomb.cpp、EntityPlus.h、ASMEntityKnife.cpp、CustomCache.cpp 和旧 I16／I18 已核字段表。

审阅纠正了子智能体初稿中的以下错误，保留其发生事实：

- Knife +4C4／+4C8 小于 +520，属于合法对象字段；初稿把它们误判为越界。当前布局保留这两个公开字段。
- Pathfinder 的 SDK 大小为 0x750，`5FC+750=D4C`；初稿写到 D74，错误吞入了两个计时字段、方向和轨道参数。当前按正确边界分开。
- ColorMod 是 0x2C，初稿把 +EDC 的范围扩大成 0x30；+F08—F0B 仍未知。
- Coins／Hearts／Keys、RoomClearCount 和 Knife Rotation／Scale／MaxDistance 等已有公开字段证据，不因本轮子研究漏查而退回未知。
- Familiar +F28 的模板符号不能证明 mutex 语义。`61DB46` 取 Target，`61DB50` 调 `4521A0`，后者实际读取 Entity+20 原生 Index；目前只确认索引／计数用途和本地 sentinel，完整业务含义未闭合。
- `939AE5` 是 LEA，实际路径构造调用为 `939B63`；Knife 命中 vector 中间清零是 `939B74`，Bomb 对应中间写为 `69D57C`。
- Familiar vtable +24／+28 分别为 Kill `60A0F0` 和 Remove `60A040`，初稿将 Remove 误标为 Kill。

本轮生成布局采用[实现说明](implementation-4-familiar-knife-bomb.md)中的分块。明确方向锚点只适用于 FireCooldown／Bomb 的对应转换声明；尚未闭合的活动条件仍须 Gap。HeadFrameDelay 直接保存原始 i32，不要求计时方向。Familiar ExtraData 两臂及完整状态由既有具名 owner 契约约束，未知与未完成范围以生成表显示为准。

用户随后审阅指出，版本15实现仍漏掉已核的五个关系槽，HeadFrameDelay 的强制转换与本文证据矛盾，hitbox 采集也错误依赖成员序号；这些是实现及此前主审的遗漏，不能归咎于未知字节。版本16复用旧路径证据修复：关系槽恢复为引用，HeadFrameDelay直接复制，hitbox先判目标类型再获取身份／Pending预约句柄；非Knife目标不要求成员登记。原先“已核bool”的 `+4E7` 也降为推断的原始u8。没有为这轮修复新增逆向或运行观察，详见实现说明的七项修复记录。

## 未决问题

- Familiar `+0x418—0x5F7` 大段 ctor-zeroed 区、PathFinder 内部布局和 D76—D9B 的未命名字段；不能把清零或常用名当作填充证据。
- Familiar `+0xD4C` 各分支递增／递减／复位的互斥条件、`+0xD50` 写入者、`+0xD9C` vector 元素、`+0xECC` ExtraData owner 控制字与 Variant 48/231 的完整 ANM 状态。
- Familiar 树容器的精确元素结构与游戏语义；当前只核出以 Target Entity Index 为 key 且更新计数，不命名为 mutex。
- Knife 表中仍列未知的私有标量、`+0x468—0x4BB` 和 `+0x504—0x51F` 的逐字段布局；`+0x468` 私有对象是否还有非 Update 消费者。PathOffset／PathFollowSpeed 在 0x520 内，已保留，不再列为越界缺口。
- Knife `+0x4E7` 的准确语义和 HitEntities 根选择对所有 Knife 变体的覆盖；`+0x4FC/+0x500` 两子体的创建/retire ownership。
- Bomb `+0x414` 双写计数及 `+0x410` 各更新分支对应的 variant/时钟合同；`+0x450—0x461`、`+0x9D4—0x9D7` 未知字节；costume enabled 与 pending byte 在全体 Bomb 变体中的覆盖。
