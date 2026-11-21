# Entity_Tear、Entity_Pickup、Entity_Laser 静态逆向证据

> 版本37后续复核：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只保留optimized／unoptimized最终点列和真实展示标量；十九项Laser输入、六项Homing标量、indices中间点缓存与Bezier／系数留Host，撤销两个Laser计时锚点。J273 Homing+34实际为8B双f32点对vector，本文旧i32／4B步长记录错误；原记录保留，不按旧类型继续传输。L2／L4／L7／L8预留，当前以版本37唯一源及生成表为准；未构建或测试，旧运行执行器仍待后续退役。

> 版本35后续复核：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait和未来循环列表，以及Slot八项参数留Host。Price／DropDelay、最终主／辅助Sprite与真实引用保留；Slot奖品+460／+470精确对应当前动画指针／is_playing。下文旧传输／原值比较约定保留原时点，现行以版本35唯一源及生成表为准；未构建或测试。


> 版本34后续复核：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)将Knife10／Bomb11／Tear21个攻击、运动与计时参数范围留Host；撤销三个计时锚点，保留真实绘制输入、最终Sprite和Bomb pending清除。下文旧的参数传输／计时见证要求保留原时点，现行传输以版本34唯一源及生成表为准；未构建或测试，运行接线仍待后续步骤。


状态：第 4 步子类静态逆向记录。GPT-6 Luna（max）执行逆向子任务，主智能体复读原始指令和官方源码后采用、纠正并接入布局。逆向子任务没有运行游戏、构建、测试、探针或校验脚本，没有修改 IDB；主智能体随后完成布局版本 20 的普通 Release 纯编译／链接。尚未闭合的字节与语义明确保留 UNKNOWN。

## IDB 与证据范围

- IDA 当前输入模块为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDB 为同名 `.i64`，imagebase `0x400000`；IDA 自动分析和 Hex-Rays 可用。该输入与本目录研究目标 `REPENTOGON v1.9.7.12.J273` 一致。未读取或核验任何哈希。
- 证据等级沿用 `entity-native-contracts.json`：`J273_STATIC` 为本轮直接 IDA 静态读取；`REUSED_J273` 为既有 `entity-native-static.md`／I 专题中已复核的目标静态事实；`SOURCE` 为 REPENTOGON `libzhl/functions/*.zhl` 或官方源码；`PUBLIC_DOC` 为本地 Wiki；`DERIVED` 为从直接指令推出；未闭合者写 `UNKNOWN`。
- 地址均为 IDA VA；减去 `0x400000` 得 RVA。pool stride/capacity 的既有来源为 `docs/archive/design/tx-04-19/entity-native-static.md` 与 `entity-native-contracts.json` 的 EntityFactory 研究；本轮复读同一目标 IDB 的 vtable 与虚函数入口。

## 类身份与入口

| 类 | J273 池步长／容量 | vtable | RTTI TypeDescriptor | ctor（直接写 vptr） | Init | Update | Render | 其他已核入口 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Tear | `0x848 × 512` | `0xB15FB8` | `0xBD152C`，`.?AVEntity_Tear@IsaacRepentancePlus@@` | `0x662C40` | `0x662F50` | `0x663F90` | `0x675AC0` | ClearReferences `0x663570` |
| Pickup | `0x6F0 × 512` | `0xB18E8C` | `0xBD146C`，`.?AVEntity_Pickup@IsaacRepentancePlus@@` | `0x6DD550` | `0x6DD7C0` | `0x6E0650` | `0x6E35D0` | Morph `0x6DFF30`；Remove `0x6EA840`；ClearReferences `0x6EA860` |
| Laser | `0x568 × 128` | `0xB2D81C` | `0xBD140C`，`.?AVEntity_Laser@IsaacRepentancePlus@@` | `0x945A90` | `0x9480E0` | `0x950B90` | `0x9514B0` | ClearReferences `0x956C60`；Remove `0x956E90`；shadow `0x955290`；post-render `0x956F80` |

IDA vtable 前段直接读回：Tear slots `Free 662DF0 / Init 662F50 / PreUpdate 6A9CA0 / Update 663F90 / Interpolate 675E10 / Render 675AC0`；Pickup `Free 6DD670 / Init 6DD7C0 / PreUpdate 6A9CA0 / Update 6E0650 / Interpolate 6E05E0 / Render 6E35D0`；Laser `Free 945C20 / Init 9480E0 / PreUpdate 6A9CA0 / Update 950B90 / Interpolate 956B50 / Render 9514B0 / Shadow 955290 / PostRender 956F80`。Laser vtable 的 Remove 槽指向 `956E90`。这些槽位来自 vtable 数据和同一 IDB 函数，不把 slot 0 Free 误作 ctor。

## 构造函数直接写入与对象内联 owner

### Entity_Tear

IDA 伪代码 `0x662C40`：先调 Entity 基类 ctor `0x6A3940`，再于 `0x662C75` 写 Tear vptr。ctor 清零的派生 dword 偏移为 `+0x428—0x434`、`+0x43C—0x44C`、`+0x47C—0x480`、`+0x494—0x4A4`、`+0x7EC—0x814` 中显式出现的字、`+0x828—0x834`。`+0x4AC`、`+0x5C0`、`+0x6D4` 分别调用 ANM2 ctor `0x402B00`；三份 owner 的 stride 是 `0x114`，范围落在 Tear 池槽内。

上述清零为零值写入证据，不是 padding 证据。`+0x438 WaitFrames`、`+0x484 StickTimer` 未由派生 ctor 的 dword 清零列表覆盖；在未同时闭合基类／Init 的初始化路径前，不补零或补出生状态。Tear 的公开字段、命中 vector 和引用逐项在后续“布局与关系”小节补证；不复制任何 STL header 或本地指针。

### Entity_Pickup

IDA 伪代码 `0x6DD550`：先调基类 ctor `0x6A3940`，再写 Pickup vptr；`+0x410` 调 ANM2 ctor `0x402B00`；`+0x5D0` 也调 ANM2 ctor。ctor 对 `+0x558、+0x55C、+0x560、+0x564` 写入 float `1.0`，对 `+0x568—+0x580` 的七个 dword 写 `0`；总范围正好 44 字节，符合一个 11-float ColorMod 的大小，但本轮尚未找到可把它确认为具名颜色的 Render/Update 消费者，故现仍记为 `UNKNOWN[44]`，不是 padding。

ctor 还将 `+0x58C、+0x590、+0x594、+0x598` 清零，并清 `+0x5C8/+0x5CC`；这只证明构造初值，不代表可跨实例发送指针。`+0x5A0` 循环数组、`+0x5C0` count 的 owner/边界由 ZHL 与官方逻辑另行说明，不把它们改成原生 vector。

### Entity_Laser

IDA 伪代码 `0x945A90`：先调基类 ctor `0x6A3940`，再写 Laser vptr；ctor 在 `+0x410/+0x414`、`+0x428—+0x454` 及 `+0x4D8/+0x4DC` 等处写明确 dword 0；于 `+0x464` 调 `sub_974770` 构造 HomingLaser；`+0x530` 用 EH vector-constructor-iterator 构造两项、元素 stride `4` 的局部数组；构造末尾写 `HomingLaser._entity (+0x464)=this`。具体构造范围之外的空隙不因邻近零写定为 padding。

## 既有布局事实、引用、几何与扩展 owner

- [生成布局 EntityTear](../../../../mod/binding/layouts/04-entity-classes/EntityTear.md)、[EntityPickup](../../../../mod/binding/layouts/04-entity-classes/EntityPickup.md)、[EntityLaser](../../../../mod/binding/layouts/04-entity-classes/EntityLaser.md) 与[实现说明](implementation-4-tear-pickup-laser.md)已列逐字节区间和 UNKNOWN；本文件只补直接入口/消费者证据，不重复布局表。边界均按 IDB 池步长。
- Tear 的 ZHL `EntityTear.zhl` 将三份内嵌 ANM2 分别列在 `+0x4AC` Halo、`+0x5C0` TearEffect、`+0x6D4` DeadEye；ctor `0x662C40` 对三处都实际调用 ANM2 ctor。公开 TearFlags 为 `+0x428` 的 16B BitSet128。`+0x444` 是 3-pointer `std::vector<uint32>` hit list；容器头不能跨进程复制。Tear ClearReferences `0x663570` 清除 `+0x478` 和 `+0x488/+0x48C/+0x490/+0x494/+0x498/+0x49C` 这 7 个 32-bit 关系槽，部分槽会连带 Remove；逐槽证据与边副作用见 [I10—I14 原生研究](../tx-04-19/entity-i10-i14-native.md#tear-i12) 及 [client_entity_links.inc](../../../../mod/src/game/client_entity_links.inc)。非空边需以 Source Key 经正式引用 owner 恢复，退休先撤边。
- Pickup 的 `+0x410` price ANM2、`+0x5D0` alternate pedestal ANM2 是不同内嵌 owner。Update/Init 可初始化价格、variant 资源与循环数组；Render `0x6E35D0` 对存在/可见/DropDelay 设门，variant 100 使用 `+0x5D0`，主 Sprite 在 Entity `+0x48`。Pickup 可循环道具为内嵌 `uint32[8] +0x5A0` 加 `count +0x5C0`，不是 STL vector；`+0x58C/+0x590/+0x594/+0x598` 是 ClearReferences `0x6EA860` 处理的四个关联槽，Remove `0x6EA840` 对 Ghost `+0x590` 有级联 Remove。详见 [I15—I19 原生研究](../tx-04-19/entity-i15-i19-native.md#i17-entitypickup) 和同一 `client_entity_links.inc`。
- Laser 内嵌 HomingLaser 在 `+0x464`，由 `sub_974770` 初始化。官方 [HomingLaser.zhl](../../../../REPENTOGON-1.1.3/libzhl/functions/HomingLaser.zhl) 中四个容器换算到 Laser 基址为 optimized Vector2 samples `+0x480`、nonoptimized samples `+0x48C`、整数向量 `+0x498`、24B HomingLaser_Bezier `+0x4AC`。`CalculateSamples` 为 J273 `0x9768C0`，按每元素 24B、6 个 f32 槽读取 Bezier；槽用途有消费者证据，具名成员/生产者布局 UNKNOWN。Laser Render `0x9514B0` 的几何路径读取 optimized samples 的 header/点（`0x9517B5` 起）；未据此声称 Render 直接消费其余三容器。Laser ClearReferences `0x956C60` 保留 `+0x4E8` 与 `+0x4EC/+0x4F8/+0x4FC/+0x504/+0x508/+0x50C/+0x510/+0x52C/+0x530/+0x534/+0x538/+0x53C` 全部 13 个引用；Remove `0x956E90` 会移除多组子/Terminal owner，不能绕过。向量与关系细节见 [I15—I19 原生研究](../tx-04-19/entity-i15-i19-native.md#i15-entitylaser-hominglaser) 及 [client_entity_links.inc](../../../../mod/src/game/client_entity_links.inc)。
- REPENTOGON [EntityPlus.h](../../../../REPENTOGON-1.1.3/repentogon/Patches/EntityPlus.h) 的派生数据位于独立 `EntityPlusHolder`，不在上述原生池步长内：`EntityTearPlus.initSound`、`EntityPickupPlus.canRerollOverride`、`EntityLaserPlus.recalculateSamplesNextUpdate/initSound`。`EntityPlus.cpp` 的 `Entity::Init` 先 reset 再按 Type 建立对应 Plus；Holder 到 Entity destructor 才释放，Remove reset hook 已注释。`canRerollOverride` 覆盖 CanReroll；Laser 重采样 flag 会清四个 HomingLaser 容器并清 flag。它们是当前进程的 Mod/派生 owner 事实，不序列化为远端对象/地址；应用文档中的公共 EntityPlus 前缀仍以 E9 范围为准。

## 计时字段：静态活动条件与边界

### Tear `+0x438 WaitFrames`

- `Entity_Tear::Update 0x663F90` 的 `0x6643A4` 比较 `[this+0x438]` 与 0，`jle 0x664481`；只有严格正值才进入该段。Parent `+0x3BC` 为空时从 `0x6643B9` 跳到 `0x664408`，仍减 1。Parent 非空时会调用 `0x412C00 / 0x431470 / 0x4318A0 / 0x493170 / 0x42D240` 等状态查询；一个分支在 `0x6643FE` 先将 WaitFrames 写 1，之后仍经 `0x664408` 同次减 1。主要减写指令是 `0x664408 add dword ptr [edi+438h], 0FFFFFFFFh`。
- 减后在 `0x66440F` 读取；归零会走 `0x664417—0x664435` 的 `+0x43C ContinueVelocity` 辅助路径。该路径的更细业务意义尚未完全命名，记 UNKNOWN。其余 `0x438` 的 Update 使用包括比较 0/1（如 `0x664D69、0x667366、0x66795E、0x667A77、0x667C27、0x669FCB、0x66A10B、0x66ABF1、0x66F994`），是行为 gate，不能等同于递减点。
- 可确认“仅正值递减一次”的本地分支；若编码成正倒计时，必须保留零以下/非正值不动的语义和归零路径，不将整个 Update 声称为无条件每全局帧执行。

### Tear `+0x484 StickTimer`

- 在 `0x66EFC2` 先无条件 `dec [edi+484h]`，随后 `0x66EFC8 cmp ... ,0`；post-decrement 为 `>0` 时 `jg 0x66F6F1` 跳过到期效果，`<=0` 执行 `0x66EFE3` 写 `+0x173=1`、`0x66EFF0` 对 `+0x474` OR `0x40`，并改 `+0x420` 等状态。
- 该分支没有先检查正值，也没有保零；负数同样继续递减。计数器的合法初值、哨兵、所属变体和到期后生命周期仍需追 Init/所有写入者，不能套用正值夹零的倒计时规则。`0x66D787` 有一处将其写零的实例路径，尚不足以推成全局初始化契约。

### Pickup `+0x554 DropDelay`、`+0x544 Wait`、`+0x540 Timeout`

- `Entity_Pickup::Update 0x6E0650` 开头：`0x6E0685` 读 DropDelay。正值且 Entity 公共 `+0x188==0` 时在 `0x6E0698—699` 减 1，然后跳到更新尾 `0x6E3579`；正值但 `+0x188!=0` 时在 `0x6E06A4` 清零后继续。该公共门目前按原始偏移记录，不据命名猜暂停或生命周期语义。
- 尾部 `0x6E26B6—0x6E26C1` 对 `+0x544` 仅在值 `>0` 时 `dec`；0/负值不变。Wiki 将 Pickup `Wait` 描述为新生道具的自动等待帧数（通常初始化 20）并随游戏帧递减，但本轮尚未逐个闭合其所有 Init 写者及所有提前返回，故只把 `+0x544` 记作与公开 Wait 候选吻合的字段，不推为任意 Update 入口必减。
- `+0x540 Timeout`：variant `100` 专用路径在 `0x6E2611—0x6E2690` 还要求相应全局状态为 2、`[this+0x528]==1`、全局计数 `>0` 且当前 Timeout `<=0`；通过 `0x6E2641` 的 RNG／spawn 路径后写 `4`。随后公共尾 `0x6E269A—0x6E26A7` 仅当正值时减 1；递减后为 0 时 `0x6E26AF—0x6E26B3` 经 vtable `+0x28` 调虚 Remove。故不能将所有正 Timeout 单独读成任意模式下每帧计数器；公开 Wiki 的消失效果只作为公开语义参照。
- `+0x558—+0x583` 的 44B 仍 UNKNOWN：它的 ctor 初值有 11 个 dword（前四 float=1，后七为0）；Pickup Update 在 `0x6E2BC6` 起把本区地址交给 helper，并在 `0x6E2C0D/18/20/28`、`0x6E2E1F—2EDD` 读写部分 float。Scoped IDA search 在 Render `0x6E35D0—0x6E3A31` 没有该区偏移；因此尚不能定名 ColorMod、证明为绘制状态或当 padding。
- `+0x6E4/+0x6E8` 是两个 Init 清零、Update/helper 会读写的 f32 候选。`0x6DD95B/965` 清零；Update `0x6E06AE—0x6E0728` 基于两值和其他公开/公共状态推进，并设置 Entity `+0x184/+0x188` 状态；`0x6E07A8/B2` 在阈值分支清零，`0x6E0832` 另读 `+0x6E4`。辅助函数 `0x6E0510/0x6E05E0` 也读写这两个槽。Render `0x6E35D0—0x6E3A31` 未见直接引用；没有证据命名为动画进度、计时器或可绘制字段，保留 `UNKNOWN[2×f32]`。

### Laser `+0x4C4 RotationDelay` 与 `+0x458 Timeout`

- `Entity_Laser::Update 0x950B90` 以 Entity 子类的 `SubType +0x30` 选择辅助路径：1/2/3 调 `0x954310`，其他值调用 `0x94C940`。`0x954310` 内存在多处对 `+0x458` 的读写，但其 Alias/活动条件/计数范围尚未完全闭合，故 SubType 1/2/3 的 Timeout 方向与哨兵记 UNKNOWN。主审复读的原指令包括 `0x95468F` 写0、`0x9548C2` 比较0 / `0x9548CF` 写5、`0x954B5F` 读、`0x954B65` 取字段地址且 `0x954B6D` 条件跳转、`0x954CA5` 比较 `0x14`、`0x954FD2` 比较0 / `0x954FE5` 写 `0x2D`。这些写入表明该路径不能因一个简单直接位移搜索无命中而归为无访问。
- **研究中更正记录**：首轮对 `0x954310—0x955287` 的字符串搜索返回0后曾被误述为无字段访问。主审直接遍历 FuncItems，发现直接位移访问和 `ESI` 保存字段地址的别名访问，撤销原结论。原搜索没有覆盖实际指令；因函数同时存在直接访问，不能把返回0的原因单独归于别名。此历史错误保留，不再把一次搜索无命中当作无访问的证明。
- 其他 SubType 的 `sub_94C940` 至少有两种可见路径：
  - 正值路径 `0x950AC0—0x950B59`：Timeout `<=0` 跳过；正值且 Entity 公共 byte `+0x175==0` 才在 `0x950ADB—0x950ADC` 减1。减后仍正值跳过；减至0后 variant 6/15 且 `Shrink +0x45E==0`、dead byte `+0x173==0` 时，经 `sub_945D60`/variant-subtype 条件分支设置 `Shrink=1, Timeout=0`（`0x950B46—4D`）；未进入 shrink 分支则写 `+0x173=1`（`0x950B59`）。这是有明确正值夹零的路径，但其完整前置状态仍受上列 gates 限制。
  - 负侧路径 `0x94FD0A—0x94FD55`：受当前 Sprite/动画状态路径约束；动画检查满足 `0x94FD0A` 后，如果状态值等于 `4` 或 `10`，直接虚 Remove；其余分支在 `0x94FD2A` 不作正值检查而先减1，并于 `0x94FD48` 与 `-18` 比较，post-decrement 小于 `-18` 才虚 Remove。这个值兼作 shrink/动画退场阶段的 signed counter，不是前一正值夹零模式；外层完整变体/动画 gates 仍 UNKNOWN。
- `RotationDelay +0x4C4` 由 `sub_94C940` 更新。`0x94CAF9` 先要求 `byte[this+0x4C0] != 0` 才进入；`0x94CB16—0x94CB1C` 读 delay 并测试“非零”，非零路径于 `0x94CC29—0x94CC2A` 减 1、写回。此处不是 `>0` 判断，所以负值也会递减；只有零 delay 进入后续角度/速度处理。旋转残值跨过零时在 `0x94CBAD` 或 `0x94CC20` 清 ActiveRotation byte `+0x4C0`。不得把 `+0x4C4` 当无条件正倒计时。

## 未决问题

- `Tear +0x484 StickTimer` 的所有初始化/写入者、初值/哨兵和到期变体仍未闭合；正值 `WaitFrames` 的零转移和父链 gate 也不能外推为所有 Update 的无条件全局帧倒计时。
- Pickup `+0x558—+0x583` 的确切类型/更新 helper 语义未知；`+0x6E4/+0x6E8` 的含义和 Source 侧是否需要保留未知。两区 Render 的函数体内没有直接偏移引用，但这不排除间接消费者。
- Laser Timeout SubType 1/2/3 路径 `0x954310` 中写 `0/5/0x2D` 等值的完整状态机未解；其他 SubType 负侧退场路径的全部 variant/animation 前置条件也保留 UNKNOWN。不能因已见正/负两种分支就把所有 Laser 归一为同一个倒计时合同。
- 后续若发现新直接证据，只修改本证据文件；不为未知字节造 padding，不从计数字段名称猜时钟方向，不复用本地容器/表现对象指针作为远端值。

## 主智能体审阅与采用

主智能体直接读取当前 IDB 的 COL→TypeDescriptor 链、三个 vtable 的 Init／Update／Render／Remove／ClearReferences 槽；Tear／Pickup／Laser 的 TypeDescriptor RVA 分别为 `0x7D152C／0x7D146C／0x7D140C`。已复读 `0x662C40／0x6DD550／0x945A90` 三个构造以及 `0x974770`，确认五份辅助 ANM2 的偏移、HomingLaser 四容器头的构造与本地 self 回指。官方 ZHL、EntityPlus.h/.cpp、已有 Client 字段／引用／容器／资源 owner 同时作为类型和最终值写入证据，未将静态结果表述为运行观察。

计时采用前，主审复读 Tear `0x6643A4—0x664435／0x66EFC2—0x66EFCF`、Pickup `0x6E0685—0x6E06A4／0x6E269A—0x6E26C1`、Laser `0x94CAF9—0x94CC2A／0x94FD2A—0x94FD55／0x950AC0—0x950B59` 的原始指令。因此保留正值夹零、有符号递减和字段专属 Laser Timeout 三种编码规则。主审另于 `0x95142B` 确认 helper 选择读取 `SubType +0x30`，不把它错记为 Entity Type，并在 `0x954310—0x955287` 纠正上述漏检。

Pickup `+0x558` 的构造和 `0x6E2BC6／0x6E2EDD` 的地址传递，以及 `+0x6E4/+0x6E8` 的 `0x6E06AE—0x6E0838` 读写也已复读。它们暂不足以证明完整类型／业务归属；本轮继续列未知，既不复制，也不因 Render 函数体搜索无直接命中就归为 Host 权威或派生。三类所有未名区、SDK 声明与活动时钟缺口均在生成表中保留。

主审结果已落入 `mod/binding/state/entity-classes.json`、整数版本 20 和 `state/entity_counters.*`。计时模式由将来的字段 owner 提供真实分支与时钟见证；缺少见证必须报 Gap，不能把未知选成 Frozen。布局和公开成员回映射生成成功不代表已完成第 10／13 步的运行接线，亦不表示类型完整。

## 验收边界

这是第 4 步静态证据文档的一部分，不是运行时支持结论。第 0—14 步期间不运行测试；本轮没有任何测试、游戏、探针、校验脚本或 IDB 修改。第 15 步才进入唯一统一整体验收。
