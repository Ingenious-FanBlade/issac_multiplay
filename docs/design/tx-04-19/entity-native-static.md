# I04 原生静态路径补证

本文只记录目标 `isaac-ng v1.9.7.12.J273` 的静态反编译和官方 ZHL/REPENTOGON 源码证据，补充 [entity-application-contracts.md](entity-application-contracts.md)。所有 VA 均对应本地 IDA 数据库 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，image base 为 `0x400000`；RVA = VA − `0x400000`。文中的对象偏移只在明确指出的类型上有效。

范围是 I04/R01—R13 当前选择的原生路径：工厂分派、Spawn/Add、派生类 Init/Render/Remove 的关键差别、特殊前置、安装资源和失败回收。这里的“已读”只表示静态路径有证据，不代表类别 adapter 已实现。没有对二进制作 patch，没有运行程序、构建、测试或游戏。

## 工厂与发布顺序

### EntityFactory::Create

`EntityFactory` 构造函数 `VA 0x6EEE60 / RVA 0x2EEE60` 为各池保留固定槽位。`EntityFactory::Create` 位于 `VA 0x6EC540 / RVA 0x2EC540`。池遍历必须用这里的 stride；官方 ZHL 的声明大小与池 stride 在少数类不一致，不能把 ZHL `sizeof` 当槽距：

| 池类别 | 池 stride × 槽数 | 构造 / 析构关键 VA | 备注 |
| --- | --- | --- | --- |
| Effect | `0x4C0 × 0x400` | ctor `5CCEA0` / dtor `5CCFD0` | `Entity_Effect.zhl` 声明 `0x490` |
| Familiar | `0xF30 × 0x40` | — | Init 有玩家/武器/子实体依赖 |
| Bomb | `0x9D8 × 0x40` | — | 独立 RNG / drop 路径 |
| Projectile | `0x478 × 0x200` | — | 不等同 Tear |
| Tear | `0x848 × 0x200` | — | — |
| Laser | `0x568 × 0x80` | — | 内嵌 HomingLaser 在 `+0x464` |
| Pickup | `0x6F0 × 0x200` | — | — |
| Slot | `0x550 × 0x80` | — | `Entity_Slot.zhl` 声明 `0x548` |
| NPC | `0xF60 × 0x200` | — | 默认类别池，不是所有 Type 的真实派生类 |
| Text | `0x530 × 0x80` | ctor `5CC1F0` / dtor `5CC250` | — |
| Dummy | `0x410 × 0x80` | ctor `6A3570` / dtor `6A35C0` | dedicated pool 存在但通用分派未到达 |
| Knife | `0x520 × 0x40` | — | `Entity_Knife.zhl` 声明 `0xE78` |

`Create(type, force)` 对 Type 1 拒绝；2/3/4/5/6/7/8/9 分别进入 Tear/Familiar/Bomb/Pickup/Slot/Laser/Knife/Projectile 池；1000 进 Effect；9001 进 Text；其余走 NPC 默认池。没有 Type 964 → Dummy 分支：`Create(964, true)` 仍会拿 NPC 池槽，不能借其 `Dummy` 名称或 SDK 类型使用。满池且 `force=true` 返回空；非 force 会轮询 `CanOverwrite`，无可覆写槽时仍会选槽，置 `removedByFactory` 标记，调用虚 `Remove`（vtable `+0x28`）和 `ClearReferences`（`+0x44`），随后清标记复用。该逻辑不证明每个派生类的业务关系都已清完。

### Game::Spawn 与 EntityList::Add

`Game::Spawn` `VA 0x6FA210 / RVA 0x2FA210` 的原生 ABI 是 `__thiscall`；官方 ZHL 声明 8 个 `this` 后参数。路径先运行 PreEntitySpawn/helper `sub_84AE90`，某些配置经过 multiplayer helper `sub_427560`；对部分 Effect 依据 spawner flags 改写参数；随后调用工厂 `Create`（`0x6FA2D3`），设置 spawner 指针 `+0x3C8`、位置与速度，再走虚 Init `vtable+4`（`0x6FA38D`）。Init 后才递增并写 entity Index `+0x20`、调用 `EntityList::Add`（`0x6FA3B3`），然后才做其余后处理与 callback。创建调用链包含 `timeGetTime`；不能把同一个 Entity 的 Init 当成发布完成，也不能认为 post-Init 写 Index 会倒灌 Init 已读取的值。

`EntityList::Add` `VA 0x413D00 / RVA 0x13D00`：接收 Entity 的 `_valid` 位于 `+0x170`。若对象已 valid，raw `0x413D15` 取 `EntityList+0x40` updateEL，走 `sub_412FC0(this+0x40, entity)` 移除匹配项，随后跳到尾部按 flags 重新加入；未 valid 时 Type 1/3 与 Knife Type 8、Subtype 0 进入 persistent list `this+0x30`，其余进 main list `this+0x20`。接着先经 vtable `+0x48` 检查关门、再经 `+0x4C` 检查 boss（Type 866 例外）并更新对应计数；然后置 `_valid +0x170`，以虚 `GetRenderZ`（vtable `+0x34`）计算排序位置并插入 render list `this+0x50`。Familiar Variant 206 之后另进 wisp list。Add 参数 flags 的 `a2[90]` 是十进制 index，实际 byte offset `+0x168`；bit mask `8` 控制 update list，不是“bit 8”位序。最后按该 flags 加入 updateEL（若 bit mask 8 清零），调用 `sub_416D10` 缓存 Type/Variant/SubType。发布前只填共同位置/生成者不足以推定派生绘制输入已齐。

## 派生类路径索引

表内 vtable 列是 vtable 基址；Init/Render/Remove/Clear 列的地址是 IDA 静态目标 VA，不是运行时已安装的 hook trampoline。官方 `Entity.zhl` 给出的基类 ABI 为 Init `__thiscall void(uint32 type, uint32 variant, uint32 subtype, uint32 initSeed)`，原生是 `this` 加4个32-bit stack words；Render 为 `__thiscall void(Vector* offset)`，Remove/ClearReferences 为 `__thiscall void()`。各派生类必须沿用其官方声明，例如 Pickup Init 是 `uint32 Type, int Variant, int Subtype, uint32 Seed`，Pickup Morph 的前三个 id 参数也是 `int`；不能把所有派生签名强制当作无符号。Free 是单独的 `__thiscall void(flags)`，不能当作池退休 Remove。IDA 的 `userpurge` 寄存器注释是反编译推断，不作为可调用 ABI。未列覆盖 Remove 的类仍调用基类，不表示可以省略移除流程。

| 类 | vtable | Init | Render | Remove / ClearReferences | 显式前置与原生副作用 |
| --- | --- | --- | --- | --- | --- |
| NPC | `B187D0` | `6B4A50` | `6CEC90` / Shadow `6D0650` | `6B4E30` | 普通 NPC 与复合 boss 的绘制输入不等价；866/0 Remove 还移除 `+3C0` 子对象 |
| Tear | `B15FB8` | `662F50` | `675AC0` | 基类 | Init 调 base `6A4A20`、复制配置 ANM 并消费 RNG `sub_6EB4B0` (`662FC3`)；Render 调整 Entity 公共绘制临时位置 `+0x1A4/+0x1A8`，Tear 自身 `+0x420/+0x424` 是 scale/baseScale |
| Projectile | `B15B1C` | `64F4B0` | `659460` | 基类 | Init 消费 RNG (`64F4F9`)；Render 可写 `PositionOffset.Y +0x350`（native displacement 848 是十进制）；wiggle 走 frame 确定性路径，不应当成 Tear adapter |
| Bomb | `B18118` | `69D6C0` | `6A0F90` | 基类 | Variant 3/4/8/18 Init 读写自身 RNG/drop (`69D729–778`) 并写 `_explosionCountdown1 +0x410`；Render 可经 `6A2420` 重建主/五 costume ANM，不能把该分支当纯 draw |
| Familiar | `B15090` | `607AA0` | `61E360` / Shadow `61E320` | `60A040` / Clear `60B800` | Spawn 时需先将 SpawnerEntity 写 `+0x3C8`；Init 由此找 Player 写裸指针 `+0x410`、初始化 Weapon `+0xDB0`，随后有 RNG 与子体生成；见下节 |
| Knife | `B2D7A0` | `939C70` | `93ECD0` | 基类 / Clear `93A1E0` | Init 创建 DeadEyeTearEffect ANM 并消费 RNG；Render 沿 Parent 链找有效 player-like owner；必须先建立该 Parent 路径 |
| Pickup | `B18E8C` | `6DD7C0` | `6E35D0` | `6EA840` / Clear `6EA860` | Init 可嵌套 spawn，读取 drop 池/变体状态；variant 100 Render 会读额外 ANM `+0x5D0`；Morph 有 RNG 与配置资源重建 |
| Laser | `B2D81C` | `9480E0` | `9514B0` / Shadow `955290` / Post `956F80` | `956E90` | samples 与普通 Sprite 是分离 owner；Init 尾部进入 Lua PostLaserInit callback |
| Slot | `B15BDC` | `659AA0` | `6620E0` | 基类 | Update `65A210` 为碰撞、计时、支付等状态机，不应为构造安装调用 |
| Effect | `B14D00` | `5CD070` | `600E60` | `5D1860` | Init variant 136 只清 `+0x420` 的 16 字节；完整 32B 由 Room creator 后写入 |
| Text | — | `5CC260` | `5CC580` | 基类/池回收 | Text buffer `+0x418` 由 writer 直接安装；Init 不清 buffer，Render 使用 buffer 与颜色 |
| Dummy | `B181B8` | `6A35D0` | `6A3630` | 基类 | pool 有实例；Render 同时格式化 `e%d.%d` 并调字体，不是纯 Sprite |

## Bomb：Render 中的资源重建分支

Bomb `Render VA 0x6A0F90 / RVA 0x2A0F90` 在 `0x6A0FC4` 可调用 `sub_6A2420`。它不是音效或纯 draw helper：`0x6A246F` 清 WORD `+0x462`；`0x6A248E` 起遍历五个 `0x114` 字节 costume ANM2（数组 `+0x464`, stride `0x114`），逐项调用 `sub_403530` Reset；随后主 Sprite/各 costume 走 `sub_407AF0` Load（`0x6A2EE1/0x6A302F`）和 `sub_407DA0` LoadGraphics（`0x6A2FD9/0x6A314B`），再重建动画、layer、贴图状态。I14 应由 Source 提供主 ANM 与五个 costume 的最终资源/姿态，并保证 draw 不落入这条 lazy rebuild 路径；不能为“刷新”直接调用 `sub_6A2420`。单个 LoadGraphics 下层副作用尚未逐项穷尽。

## Knife 与 Familiar 的 owner 前置

### Knife

Knife Init `VA 0x939C70 / RVA 0x539C70` 先检查全局 ANM2 指针 `Source_0`（VA `0xBE5594` / RVA `0x7E5594`）；为空时 `0x939CAE` 分配 `0x114` 字节，`0x939CC2` 调 ANM2 ctor，`0x939CFF` 写回全局指针，并将该指针加入全局 `dword_BE6F48/BE6F4C` 指针容器（`0x939CE1/CE3`，满容量经 `sub_410F30`）。随后加载 `gfx/DeadEyeTearEffect.anm2` (`0x939D66`)，设置 Idle/layer 状态并调用基类 Init `0x6A4A20`。这不是每个 Knife 对象内嵌/临时 ANM；新建后有进程级资源 owner 与全局注册副作用。Init 之后还消耗 RNG `sub_6EB4B0` (`0x93A00B`) 并初始化 Knife 的数值/关系状态。

Render `0x93ECD0` 进入 `sub_93F5E0`、`sub_93F650`，使用 Knife 的 Parent（`+0x3BC`）向上解析。Player 辅助查找覆盖 Player、Familiar 的 `+0x410`、Effect、Tear；不可用/断裂 parent 路径会跳过绘制。Spawn 的 SpawnerEntity 是 `+0x3C8`，不能把它与 Parent 混为一个前置字段。

ClearReferences `0x93A1E0` 清关系并对 `+0x4FC/+0x500` 两个子指针调虚 Remove；其他 Remove/关系清理来自后续分支。池 stride `0x520` 与 ZHL 声明 `0xE78` 不同，只能按池 stride 遍历；逐字段签名仍须按各自 ZHL/指令核对。

### Familiar

Familiar Init `0x607AA0` 从已存在的 Spawner `+0x3C8` 取引用并通过 helper 找 Player，保存裸指针 `+0x410`；`0x607F79` 将 Weapon 指针 `+0xDB0` 置零，后续分支可能由 `0x609D55` 生成子实体，且 `0x607FA1` 消费 RNG。没有与真实 owner 对应的 Player 前置时，不能把后补 Player 指针当作 Init 等价。

Render `0x61E360` 读取 `+0x410`。Variant 分支 inline 读全局 RNG `VA 0xBD0674` (`0x61E533`) 并写回 (`0x61E5A3`)；还临时改 alpha (`0x61E604`) 并调 `sub_402A70` (`0x61E60D`)，所以该 Render 不是纯绘制，也没有证据说它调用 `sub_6EB4B0`。Variant 48 的 `+0xECC` ExtraData `IsaacsBodyData` 含多份 ANM；Variant 231 的 `AbyssLocustData` 含两份 ANM container；两者是当前 I03/I18 正式 source 尚缺的绘制输入。Variant 68/78/243 复用 Player 绘制 helper，临时改 Player color/visible；不能把“Player pointer 非空”当充分输入，I06 至少要提供被借用 Player 的目标颜色/可见状态及恢复顺序。

Remove `0x60A040` 对 Variant 43/73 调整 Player `+0x136C/+0x1370` 类计数；Variant 68/78/243 路径改 Player `+0x1640/+0x1644` 计数/浮点状态。ClearReferences `0x60B800` 还改 Player/Grid 相关标志，移除 `+0xDAC` 子对象并释放 `+0xD9C` vector backing、清 `+0xDB0` Weapon 指针（这些十进制伪码位移分别为3500/3484/3504）。这些 owner 生命周期必须在引用退休时解除。

## Effect Variant 136：GridEntityDesc 与 ANM

官方 `GridEntityDesc.zhl` 的布局为：Type `+0x00`、Variant `+0x04`、State `+0x08`、SpawnCount `+0x0C`、SpawnSeed `+0x10`、VarData `+0x14`、Initialized byte `+0x18`、VariableSeed `+0x1C`，总计 `0x20`。VarData 字段即使由 `BitSet128` 命名也只有 `0x10`，不能据此把 descriptor 当 16B 复制。

`LuaEntityEffect.cpp::Lua_GetGridEntityDesc` 只在 Variant 136 返回 `&effect->_varData` 的 view，也就是 `this+0x420`；它不是 active Room GridEntity 指针。Effect Init `VA 0x5CD070 / RVA 0x1CD070` 在 `0x5CD265` 仅清 `[this+0x420, +0x430)` 共16B；Variant group 136/150/177/183/187/192 在 `0x5D0714` 写 byte `+0x171=0`，没有初始化完整 descriptor。Render `0x600E60` 不直接消费 `+0x420..+0x43F`。Remove `0x5D1860` 不释放/清这32字节；其可见额外路径是扫描 Effect 1000 Variant 60/161，按分支对上下文 `+0x2A160` 调 `sub_908140`，传入 ID `0x345` 或 `0x34E`（raw `0x5D18DB/0x5D18F5`）；再写 byte `+0x172` Exists 与 `+0x173` Dead，并有 Lua 引用清理。

已定位真实 Grid lift/thrown creator：`VA 0x81AE00 / RVA 0x41AE00`，返回 Entity_Effect*；不是单独调用 Init 的路径。`Room::PickupGridEntity(int)` 的参数临时对象是 ANM2，不是文件名字符串：IDA raw 显示 `0x81AE96` 调 `sub_402B00` 初始化本地 ANM2；`sub_402CD0` 为 ANM2 拷贝构造；`sub_403160` 与官方 `Isaac::SwapANM2` 对应；`sub_403790` 清理本地 ANM2。

1. 调 `sub_81A920(room, GridIndex, ..., &desc32, &anm2)`。前置检查索引 `<=0x1BF`、格点实体存在且 `+0x3C` 非零；按 grid type 分支验证/转换。`0x81AA48–55` 将来源实体 `+0x04..+0x23` 两段各16B复制到 desc 的32B输出；`0x81AA1C–34` 对来源 GridEntity `+0x40` 的 ANM2 做拷贝构造，再 `SwapANM2` 到 caller 的临时 ANM2。该路径携带 ANM layer/animation owner，不是 Load(filename)。
2. `0x81AEBA` 为成功条件的 `test/jz`；成功路径在 `0x81AEBE` 调 `sub_6EB4B0` 取 spawn seed；`0x81AEC9–0x81AEE2` 走 `sub_424340`，参数指定 Type 1000 / Variant 136，位置与速度是零向量，spawner null，subtype 0，seed 为该 RNG 值。该步是 Spawn 类副作用，不适合作为无副作用构造 setter。
3. `0x81AEE9` 将 caller 临时 ANM2 放入 ECX、`0x81AEEF` 将 effect 主 Sprite `+0x48` 放入 EDX，`0x81AEF2` 调 `SwapANM2`；`0x81AEF7–0x81AF0C` 用两条 `movups` 将完整32B GridEntityDesc 写入 effect `+0x420/+0x430`；`0x81AF13–0x81AF19` 从 Room `+0x1D18` 复制 dword 到 Effect `+0x450`。
4. `SwapANM2` `VA 0x403160 / RVA 0x3160` 不是简单设置 ANM 路径。raw `0x403163–0x4031AA` 交换 ANM2 内部字段；另交换 ColorMod 区域 `+0xA8` 和 `+0xD4`（`0x40335D–0x4033FE`）。Layer 数组指针 `+0x7C` 与 count `+0x80` 跟随其余结构成员交换；`0x4034B0–0x4034FC` 按 count 遍历数组，并把每个 LayerState `+0x04` animation owner 重绑到新所属 ANM2。故该调用还转移/重绑资源 phase 与 layer owner。成功后本地 ANM2 持有被换出的旧 Sprite 状态，再在 `0x81AF23–29` 调 `sub_403790` 析构。Helper 返回 false 时创建器不 Spawn Effect 并返回 null；本地 ANM2 会析构，但尚未证明 `sub_81A920` 在所有失败分支中都未先修改 Room/邻格 Entity，不能把整个 creator 失败描述为原子回滚。

`sub_81A920` 不是纯 getter：对特定 grid 类型会进入 `sub_715B70`、`sub_715700`、`sub_715550`、`sub_718680`，并有 room GridEntity slot 移除/替换、旧对象释放和 Decoration 创建路径。更重要的是 `0x81ADBA–0x81ADC4` 扫描邻近 Entity：对 Parent `+0x3BC` 为空且坐标换算命中该 GridIndex 的 Entity，写 `+0xB64=0x10` 并调用 vtable `+0x0C` 虚 Update。creator 的调用者 `0x55BEC0`（Entity Update）是游戏内操作链。`sub_81A920` 各 type 判定和全部 Grid side effect 尚未逐类穷尽；适配器不能调用完整 lift 路径，只消费 Source 最终 desc/ANM/state。

`Entity_Effect.zhl` 声明总长 `0x490`，实际池槽距为 `0x4C0`；两者差异需按上下文处理，不得按声明长度扫池。

## Pickup：Morph、Render、资源 owner

Pickup Init `0x6DD7C0` 会重置多组关系与容器；特定分支通过 `sub_424340` 嵌套生成对象，也读取 DropPool/变体并初始化辅助资源。其 ZHL 签名为 `uint32 Type, int Variant, int Subtype, uint32 Seed`。Morph 位于 `VA 0x6DFF30 / RVA 0x2DFF30`，ABI 按 ZHL 为 `__thiscall void(int EntityType, int Variant, int SubType, bool KeepPrice, bool KeepSeed, bool IgnoreModifiers)`。`0x6DFF5E` 在 `!KeepSeed` 时以 xorshift 改 `+0x3EC` seed；`0x6DFFCE` 可经过 multiplayer 参数调整；随后分支在 `0x6E01B9/0x6E01CF` 调虚 Init，并恢复/重排多个字段、资源、关系列表（含 `6DF890/6DED90/6DF520` helper）。重复 Morph 会重走 Init、副生成与 RNG；不作为字段应用捷径。

Pickup Render `VA 0x6E35D0 / RVA 0x2E35D0` 的外层读取字节 `+0x172` Exists、`+0x171` Visible；不要把它们误记为 valid/dead。Variant 100 的分支在 `0x6E383C` 取 `this+0x5D0`，`0x6E384E` 调 sprite draw helper；这是 `_altPedestalANM2`，来源需在正式 Source 补足。主 Sprite 是 `+0x48`、price ANM2 `+0x410`。Render 的特殊路线 `0x6E3784–0x6E37CC` 会改全局 RNG；也写 wobble `+0x684`。不要在客户端运行它仅为了得到绘制中间值。

Remove `0x6EA840` 和 ClearReferences `0x6EA860` 均可对 ghost `+0x590` 调虚 Remove；ClearReferences 还拆 `+0x58C/+0x590/+0x594/+0x598` 关系。`PickupUtils::InitFlipState` 不是安全 setter：官方源码显示它会注册 save state、从 ItemPool 抽取并调用 ANM.Update。`_dropDelay +0x554` 只按源码名记录为 dropDelay，未证明为 timeout；`_MegaChestCollectible +0x594` 作为容器按 owner 处理。

## Laser/HomingLaser：samples 与尾部 callback

Laser Init `VA 0x9480E0 / RVA 0x5480E0`：`0x9480F8` 调 base Init；`0x948198` 清 WORD `+0x517`；`0x94829D` 写 `+0x45C=1`、`+0x45D=-1`；`0x9482B0` 写 `+0x460=0`；`0x94835F/36B/377/384` 初始化内嵌 HomingLaser 相关 vector；尾部进入 `sub_847C10`。后者在 `0x847C59–72` 检查 callback mask 并执行 Lua `PostLaserInit`，会操作 registry 引用/错误路径；不是音效。该 Init 本身不能仅凭虚拟调用约定视为资源无副作用。

官方 `HomingLaser.zhl`：HomingLaser 内嵌 `+0x464`，optimized samples vector `+0x480`，nonoptimized `+0x48C`，另一个 vector `+0x498`、Bezier `+0x4AC`；样本元素为 `Vector2`，8B。`GetSamples` `VA 0x956820` 是 leaf，仅 `lea eax,[ecx+480]; ret`；`GetNonOptimizedSamples` 返回 `+0x48C`。Laser Render `0x9514B0` 依 `+0x45D` signed mode 分支；绘制几何路径在 `0x9517B5` 读取 `+0x480/+0x484` 的 vector header/点。当前 Render 读取 optimized samples；未证实它消费 nonoptimized/Bezier/第三 vector。不能复制 vector header 或借用不同 allocator 的 storage。

Laser Remove `0x956E90` 会先调用基类 Remove，再遍历 child 指针：`+0x4F8` 的2项、`+0x504` 的4项；条件分支另清 `+0x4E8/+0x4EC`、`+0x530` 的2项与 `+0x538`，并对仍活跃项虚调用 Remove；最后处理 `+0x3C0`。这些是实际关系 owner；直写 Remove/绕过会遗留子束。Laser Update `0x950B90` 可 spawn 子实体（`0x950D00`）并写位置/角度，再经 `0x951105/1298` 调 `sub_948FF0`；不是安装 samples 的替代方案。

## Slot：timeout 文档冲突与 native Update

Slot vtable `B15BDC`，Init `0x659AA0`、Render `0x6620E0`、Update `0x65A210`；Slot 池 stride `0x550`，ZHL 声明 `0x548`。REPENTOGON `LuaEntitySlot.cpp` setter 对 `GetTimeout` 取 `unsigned short` 后写，`SetTouch` 也是 u16，ShellGameAnimationIndex 是 short，TriggerTimer 是 u32。ZHL 中 `GetTimeout` 返回 `short*`，Lua getter 对 short 解引用并符号扩展；故存储/写入为16-bit 模式，getter 语义为有符号16-bit，不能统一宣称 getter 是 u16。

已完整扫描 native `Slot::Update` `0x65A210`：对 `this+0x41C` 只有 WORD 操作，没有 `[+0x41E]`、dword `[+0x41C]` 或 `0x10000/0x20000` 常量。`0x65A511–0x65A525` 为 `movzx` word、加1、截回 AX 并与3比较；`0x65A4B1–0xA4BA` 读取/测试 word；支付/动画分支在 `0x65E714`、`0x65ED7A` 用 AX 写回 word。Update 的 timeout 读取没有暴露 16/17 位 packed 检查。因此 REP Wiki `EntitySlot.Timeout` 1<<16/1<<17 描述与当前 J273 SDK/native 路径冲突；暂时无法确认 Wiki 指向其他字段或文档错误，原意 UNKNOWN。不要凭高位说明增加额外 offset，也不要把 Update 当构造后态 setter。

Slot SetPrize 路径 `VA 0x6626F0` 在进入所有 variant 分支前都会先写 `+0x540`；Variant 15/16 才继续走图形路径，不能写成仅 Variant 15 写标量。所有变体若要构造 prize 显示，应安装最终已确认标量与最终资源，而不是重放完整 setter/Morph 链。

## Text 与 Dummy：真实工厂来源未闭合

Text 池 stride `0x530 × 0x80`。Init `0x5CC260` 在 `0x5CC293` 用 `sub_9E35B0` 生成 KColor 并拷到 `+0x518`，清 counter `+0x410` 等，但不清 text buffer `+0x418`；Render `0x5CC580` 读取 buffer/颜色并调用 Font Manager `+0x4AB84`。因此同槽复用后若新值未安装会读旧 buffer。Source保持只读：字符串要求当前use真实writer见证，颜色/counter/duration分别按其初始化依据读取；文字绘制phase来自独立`native-text-render-v1`的Manager frame/pause/subframe/font/offset，不是Sprite动画phase。Client按匹配Key/shape/counter/text的这些目标值安装，不依赖Init清buffer或执行Update。已知writer `sub_6FF270` 在`0x6FF335`对`+0x418`以容量`0x100`格式化`%s`，颜色取虚方法结果，并在`0x6FF341`调Update。此writer会更新计时/绘制状态，不能把“buffer+color”两项等同为完整Text状态。

通用Create有Type9001 Text池分支；Game Spawn可调用Init后Add而不需要先执行Update，但Client构造完整Text必须持有Source给出的final buffer、color、counter/duration与匹配的独立文字draw phase。不采用整个Spawn/真实writer操作链来生成这些值。何种产品事件/官方完整业务调用者应创建该Text，当前未追到，不把`sub_6FF270`的任意caller代作普遍语义。

Dummy 池 stride `0x410 × 0x80`，构造/析构 `0x6A3570/0x6A35C0`，vtable `B181B8`，Init `0x6A35D0` 调 base `6A4A20`。Render `0x6A3630` 除 common render `sub_6AC4E0` 外，还格式化 `e%d.%d` 并调字体 `sub_9E9500`；不能叫 pure sprite。Dedicated Dummy 池存在，但 `EntityFactory::Create` 没有 Type 964 case，通用 `Create(964,true)` 会落到 NPC 池。当前资料内没有找到合法的 dedicated pool caller/创建入口；这是一条具体来源缺口，不能把 Dummy 类宣称为通用 Spawn 可构造。

## Morph、命中编号与扩展对象

NPC Morph `VA 0x6BD0C0 / RVA 0x2BD0C0` 会先检查同 Type/Variant/SubType 的早退，再改配置 id 并走 `sub_6902A0` 配置/资源与虚调用；`0x6BD24F` 调 `sub_904DE0(type,variant)` 产生 bestiary/progression 副作用，后面还有 `703280/703350` helper。无可依赖的成功返回合同，不应为同步字段调用整个 Morph。Delirium special 的宽度应引用源码：Angle `uint8`、TransformationTimer `uint16`、Cycle timer 的 mask `0x3FF` 位于 bits 15..24；Wiki 的 15..25 inclusive 描述不准确。

官方 `Entity_Tear/Laser/Knife::AddToHitList` 记录 `entity->GetHitListIndex()`。`Entity::GetHitListIndex` ZHL 签名为 `__thiscall uint32_t`，目标 `VA 0x6B1ED0 / RVA 0x2B1ED0`。大多数类型返回自身 `Index(+0x20)`；特殊 parent/root 路径包括 Type 35、216、909、912、918、239、244 variant 2；Type 28/variant2 与 Type 62/variant1 在 `+0x3C0` 非空时用自身，否则取 root；Type 19/89 直接自身；multiplayer Player 直接 Index。root 路径由 `sub_6A4070` 沿 `+0x3BC` Parent 链，边界条件 helper `sub_6A4430` 未完全解码。因此这些 hitlist 数字不是跨实例稳定 Key；Authority 可把已观测集合当纯值，不让本地碰撞消费。

`EntityPlus.cpp::Entity::Init` 每次 Init 都先重置 `EntityPlusHolder::data`，再按 Entity Type 建立 Player/Familiar/Tear/Pickup/Laser/KnifePlus 或基类 Plus。`Entity::Remove` 中 reset Hook 被注释；Holder 在 destructor 才释放。Native 槽复用/池移除不能整体绕过官方 Plus 管理。`EntityManager.cpp` 的 factory `Create` hook 在 Init 前通知 Lua NewEntity；`Remove`/`CommitRemovedEntities` 后续再通知 delete、ClearReferences、`valid=false` 与从列表移除。adapter 的“Init→写 Source→Publish”需核对不能无意触发 callback 两次。

## 当前明确未决

- `sub_81A920` 中每一种 Grid Entity 的 Decoration 转换和 Room side effect 还没有逐 variant 映射；该函数或其 callee 是否触发特定 ItemPool/音效也未确认。已经证实它不是无副作用 descriptor getter，32B 输出、creator 顺序、ANM2 交换和 state owner 已闭合；若实际使用某 variant，应按该 type 对应分支补读。
- `EntityFactory::Create` 非 force 满池复用路径对各派生类的 `CanOverwrite` 与动态 Plus/外部引用仍需接到具体类退休策略。本文只证明 factory 的虚 Remove/Clear 调用顺序。
- Dummy 的合法 dedicated pool caller 未找到；Text 真实业务创建来源未找到。共同 Spawn API 不会替代这两项类别来源确认。
- Bomb Render 的 `sub_6A2420` 是主/五 costume ANM 的 Reset、Load、LoadGraphics 重建路径；其 LoadGraphics 下层副作用未穷尽，不能让构造适配器主动调用。
- 普通/复合 NPC、特殊 Pickup、Laser samples 临时几何变体、Familiar weapon 子类的全部资源分支仍须按实际 I04 选择扩展；本文不声称所有游戏类别的可绘制输入均已穷尽。

## 主智能体审阅与采用收口

2026-11-03。本专题由一个GPT-6 Luna max子智能体执行必要静态逆向，主智能体在先读本地Wiki/官方源码后，独立读取工厂/池构造、ctor写vtable、发布、各代表cleanup/draw、Slot setter、Laser回调和Effect136 creator/copy/swap链的伪码与必要原始指令。采用结果与[逐项合同](entity-application-contracts.md)及[功能目录研究索引](entity-native-contracts.json)一致；本轮没有把静态地址加入可执行Hook清单，没有运行游戏、构建或测试。

审阅纠正并取代了子报告中间推断：伪码十进制位移误写hex、valid刷新分支误当工厂移除、Slot先写标量的分支范围、GetHitListIndex遗漏类型、Pickup的exists/visible/dropDelay、Familiar inline RNG误记函数调用、Knife全局资源误记实例、Effect136完整ANM交换误记字符串，以及ColorMod区与layer数组的混淆。主审自身对`this+16`指针算术的中间误述也经`413D15 lea ecx,[esi+40h]`更正。Text的独立文字phase与Sprite动画phase分别保留，避免再引入错误语义。

I04交付R01—R13的静态采用/否决合同和上述具名UNKNOWN；未解释的路径没有获得可执行资格。I05之后的实现必须补齐当前选择实际需要的owner/来源/窄分派，代码完成、原生后态和像素通过仍归各实现任务与I34统一E2E，不能用本专题覆盖它们。
