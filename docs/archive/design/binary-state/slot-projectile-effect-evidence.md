# Slot、Projectile、Effect 静态证据

> 版本40后续结论见[Effect五项未决](effect-next-five-evidence.md)：VarData新增具名臂，Timeout已找到101双减和141刷新，Trail上限127，ExtraData具体类型仍三种。下文原始证据、当时未决及失败保留，已解决的条目不作为当前缺口重复处理。

> 版本38后续复核：[Effect各臂与附属payload](effect-synchronization-review.md)撤销六项公共更新输入、TearFlags与136描述；保留真实展示参数及三种payload。MrMe原16B未知已核为Host状态；Trail606190明确分配并复制64B，旧前缀／producer未定位限制由新证据解决；Ghost仅发有效前缀，Offset／Scale绘制派生，原四槽UNKNOWN仍保留。下文旧同步决定与未决仅代表原时点，现行以版本38源／生成表为准；未构建或测试，运行时仍待接线。

> 版本36后续复核：[Projectile运动与展示参数复核](projectile-synchronization-review.md)将Projectile追踪／曲线／加速／下落／摆动及Scale／ProjectileFlags共八项留Host，保留Render直接消费的Height；公共EntityFlags、最终位置／偏移／尺寸和Sprite保持。私有+440及+460／+464的展示来源仍UNKNOWN，未构建或测试。下文旧传输与原值比较约定保留原时点，现行以版本36源及生成表为准，运行时仍待后续步骤接线。


> 版本35后续复核：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait和未来循环列表，以及Slot八项参数留Host。Price／DropDelay、最终主／辅助Sprite与真实引用保留；Slot奖品+460／+470精确对应当前动画指针／is_playing。下文旧传输／原值比较约定保留原时点，现行以版本35唯一源及生成表为准；未构建或测试。


日期：2026-11-09。范围：第 4 步的 Entity_Slot、Entity_Projectile、Entity_Effect。仅记录本地 REPENTOGON 1.1.3 SDK、现有研究记录和 J273 IDA 静态观察；没有运行游戏、探针、构建、测试或哈希校验。

## 证据标记与目标数据库

- **直接观察**：当前 IDA 指令、vtable 数据或目标函数中的实际偏移访问。
- **SDK／公开声明**：REPENTOGON ZHL、头文件及 Lua binding 的声明/访问；只能证明该 SDK 路径怎样解释成员，不能替代池步长或未声明区域的逆向。
- **项目既有证据**：`docs/archive/design/tx-04-19/` 中已记录的 J273 观察，本次用于避免重复读取已闭合范围。
- **推导**：由直接观察和类型/调用关系推出，明示推理边界。
- **UNKNOWN**：当前证据不足；不将构造未写区、SDK padding 或池尾自动当成填充。

IDA health 显示当前数据库模块为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`，Hex-Rays 可用。数据库中的 Slot、Projectile、Effect vtable、RTTI 地址与项目已有 J273 记录相符；本报告地址同时给 VA，RVA 按 `VA - 0x400000` 计算。未读取或比对任何哈希。

## Entity_Slot

### 池、虚表、初始化及表现

| 项目 | 证据与结论 |
|---|---|
| 池 | 项目既有 EntityFactory 观察：Slot stride `0x550`、容量 `0x80`；官方 ZHL/头文件结构长度 `0x548`。`+0x548—0x54F` 是 stride 相对 SDK 声明多出的 8B，归属 **UNKNOWN**。池 stride 是上界，不证明 C++ `sizeof` 恰为 `0x550`。 |
| RTTI | 直接数据：vftable `VA 0xB15BDC`；COL `0xB437F8`；TypeDescriptor `0xBD20B0`（RVA `0x7D20B0`）。vtable slot 0=`0x659A30`、slot 1 Init=`0x659AA0`、Update=`0x65A210`、Render=`0x6620E0`。 |
| Init | `VA 0x659AA0`（RVA `0x259AA0`）先调用 Base Init `0x6A4A20`，后按 variant 分支建主 Sprite／奖品 Sprite／动画并设置派生状态。Variant 6/15 会交换 Main/Prize ANM2 状态；8/11 等分支带 RNG、资源或虚调用。完整 Slot Init 不是无副作用的状态安装 setter。详见项目既有 `entity-i20-i25-native.md`。 |
| Render | `VA 0x6620E0`（RVA `0x2620E0`）先无条件调用公共 Render `0x6AC4E0`。Slot 专属奖励绘制只在 variant 6/15：检查对象 `+0x460` 与 byte `+0x470`，再以 prize ANM2 `+0x42C` 调 `0x4059E0`。主审复读确认：`+0x460` 对应 prize ANM2 内偏移 `+0x34` 的动画状态，不是 `_loaded`；ANM2 的 `_loaded` 在 `+0x109`。不要将 `+0x460` 误标为 `_loaded`。 |
| Update | `VA 0x65A210`（RVA `0x25A210`）。Timeout 的 `+0x41C` 路径是 16-bit word：例如 `0x65A94D movzx ecx, word ptr [edi+41Ch]`，`0x65A95B lea eax,[ecx-1]`，`0x65A961 mov [edi+41Ch],cx`。对该字段的访问没有读写 `+0x41E` 的证据；Wiki 所述 16/17 位 packed flags 与本版本 native/SKD 路径冲突，实际来源 UNKNOWN。 |

### 公开成员回映射

来源：`libzhl/functions/EntitySlot.zhl`、`include/IsaacRepentance.h`、`repentogon/LuaInterfaces/Entities/LuaEntitySlot.cpp`。Lua getter/setter 均直接解引用以下字段；`GetPrizeSprite` 返回内嵌 ANM2 的地址。

| 公开成员 | 原生偏移与宽度 | 说明 |
|---|---|---|
| State | `+0x410`, i32 | `GetState`/`SetState` |
| PrizeType | `+0x414`, i32 | `GetPrizeType`/`SetPrizeType` |
| ShellGameAnimationIndex | `+0x41A`, i16 | signed short；Lua setter 转 `short` |
| Timeout | `+0x41C`, i16 | getter 符号扩展；setter 转 `unsigned short` 后写回同一 16-bit 槽 |
| DonationValue | `+0x420`, i32 | `GetDonationValue`/`SetDonationValue` |
| TriggerTimerNum | `+0x424`, u32 | `GetTriggerTimerNum`/`SetTriggerTimerNum` |
| Touch | `+0x42A`, u16 | `GetTouch`/`SetTouch` |
| PrizeSprite | `+0x42C`, 内嵌 ANM2 `0x114`B | 独立于 Entity 主 Sprite；最终资源、动画和图层属于该 ANM2 状态 |
| PrizeCollectible | `+0x540`, i32 | `SetPrizeCollectible` 所有 variant 都先写此标量；不要用 setter 重放开奖/资源路径 |

SDK 声明 `+0x418—0x419`、`+0x41E—0x41F`、`+0x544—0x547` 为 padding；`+0x428—0x429` 是命名为 `_unkShort1` 的 2B，语义仍 **UNKNOWN**，不据名称或相邻公开字段将其归作 padding。池尾 `+0x548—0x54F` 亦 **UNKNOWN**。Variant 6/15 的奖品 ANM2 必须从 Source 的最终状态来；不能根据 PrizeCollectible 本地重建。

## Entity_Projectile

### 池、虚表、派生字段与公开映射

| 项目 | 证据与结论 |
|---|---|
| 池 | 项目既有 EntityFactory 观察：Projectile stride `0x478`、容量 `0x200`；官方 ZHL/头文件长度 `0x470`，故 `+0x470—0x477` 不能按 SDK 长度丢弃，也不能自动认定为 padding。 |
| RTTI | 直接数据：vftable `VA 0xB15B1C`；TypeDescriptor `0xBD13D8`。vtable slot 1=`0x64F4B0`（RVA `0x24F4B0`）、Update=`0x64F990`（RVA `0x24F990`）、Interpolate=`0x658E10`（RVA `0x258E10`）、Render=`0x659460`（RVA `0x259460`）。 |
| `0x64F4B0` | 虽然 `EntityProjectile.zhl` 将匹配签名标作 `Constructor()`，vtable 证明它是 slot 1 派生 Init；函数在 `0x64F4C8` 调 Base Init `0x6A4A20`，后续在 `0x64F4F9` 消耗 RNG，并按 variant 写资源和状态。不是单纯字段清零的 ctor。 |
| DepthOffset 别名 | Entity 公共 Lua binding `DepthOffset` 与 Projectile 类字段目录 `EntityProjectile.DepthOffset` 都指 `+0x358`, f32；项目已有 J273 公开绑定证据 `entity-i10-i14-native.md` 与 `tx-04-19-common-application.md`。它是类字段别名／公共同值，不使用 ZHL `_depthOffset +0x2B0` 代替；后者实际语义 UNKNOWN。 |
| 本类公开 Lua | REPENTOGON `LuaEntityProjectile.cpp` 当前只增加 `Deflect` 操作（调用 `Reflect`），没有逐字段 getter/setter。下表语义来自本地原版 Wiki 与项目类字段目录，不能误称为 LuaEntityProjectile.cpp accessor。 |

项目既有类字段映射（`mod/src/game/client_entity_class_fields.inc`、`entity-application.json`）如下；直接 native 访问可复读例子是 `Update 0x64F990` 中 `+0x45C` 的 dword load/test：`0x65488E mov eax,[edi+45Ch]`、`0x654894 test eax,eax`；`0x656D5F`、`0x65701B` 也有同宽读取。

| 成员 | 偏移 / 宽度 | 证据类别 |
|---|---|---|
| Height | `+0x410`, f32 | 项目字段目录；Init 写 32-bit 初值（`0x64F4EF`） |
| FallingSpeed | `+0x414`, f32 | 项目字段目录；Init 写 f32（`0x64F5CF`） |
| FallingAccel | `+0x418`, f32 | 项目字段目录；Init 写 dword 0（`0x64F4FE`） |
| Damage | `+0x41C`, f32 | 项目字段目录 |
| Scale | `+0x420`, f32 | 项目字段目录 |
| HomingStrength | `+0x424`, f32 | 项目字段目录 |
| CurvingStrength | `+0x428`, f32 | 项目字段目录 |
| Acceleration | `+0x42C`, f32 | 项目字段目录 |
| ProjectileFlags | `+0x438`, u64 | 项目字段目录的宽度为 64-bit；该表不是本轮对全部 flag writer 逐条扫宽的替代 |
| WiggleFrameOffset | `+0x448`, i32 | 项目字段目录 |
| ChangeFlags | `+0x450`, u64 | 项目字段目录的宽度为 64-bit；同上，完整 writer 清点未做 |
| ChangeVelocity | `+0x458`, f32 | 项目字段目录 |
| ChangeTimeout | `+0x45C`, i32 | 直接 native Update 以 dword 读取；Update 三个命中 `0x65488E/0x656D5F/0x65701B` 均先 test 是否 `<= 0`，未见该 Update 中对其逐帧减一写回。具体与 FrameCount 的算法关系在本轮未闭合，语义细节 **UNKNOWN**；不把它叫作每帧倒数。 |

### Plus companion 与尾部

`0x64F4B0` Init 直接在 `0x64F5F7` 将 dword `[edi+0x470]` 置零；Projectile Update 在 `0x654AFB` 读取 `[edi+0x470]`，Interpolate `0x658E10` 在 `0x658EBB` 读取同偏移。此证据证明 `+0x470` 不是可自由复制的 stride padding，但没有证明它是指针、Plus companion、flags 或 scalar 的具体语义。`+0x474—0x477` 在本轮所读 Init/Update/Render/Interpolate 范围内未找到 disp32 访问；是否填充仍 **UNKNOWN**。SDK 不声明 Plus companion 字段；不按“池步长比声明多 8B”推造伴随对象或指针所有权。`+0x430—0x437`、`+0x440—0x447` 也未纳入已命名字段表，保留 **UNKNOWN**。

## Entity_Effect

### 池、类字段与固定区域

| 项目 | 证据与结论 |
|---|---|
| 池 | 项目既有工厂证据：stride `0x4C0`、容量 `0x400`；SDK 声明长度 `0x490`。stride 是对象范围上界，未据它认定 C++ `sizeof`。 |
| RTTI/vtable | vftable `VA 0xB14D00`，Init=`0x5CD070`，Update=`0x5D35D0`，Render=`0x600E60`。 |
| Effect ctor | `VA 0x5CCEA0`（RVA `0x1CCEA0`）先调 Base ctor `0x6A3940`，并显式清零 `+0x484/+0x488/+0x48C/+0x490`、`+0x4A8/+0x4AC/+0x4B0`、`+0x4B4/+0x4B8`。没有初始化 `+0x420`。这些 ctor 写零只证明初始化动作，不单独证明每段的语义或 `sizeof`。 |
| Effect Init | `VA 0x5CD070`（RVA `0x1CD070`）在 `0x5CD21B xorps xmm0,xmm0` 后由 `0x5CD265 movups [esi+420h],xmm0` 清零 16B `+0x420—+0x42F`；该路径不清 `+0x430—+0x43F`。之后巨型 variant switch 可以按变体写该区域。故 Init 基线下前 16B非旧槽残留，但这不证明其他变体运行期语义或指针安全。 |
| 固定公开字段 | `+0x410 Height`, `+0x414 FallingSpeed`, `+0x418 FallingAcceleration` 为 f32；`+0x450 State` i32；`+0x454/+0x458` Radius f32；`+0x45C Timeout` i32；`+0x460 LifeSpan` i32；`+0x464 IsFollowing` byte；`+0x468 ParentOffset` Vector；`+0x470 DamageSource` i32；`+0x474 Rotation` f32；`+0x478 Scale` f32。映射与 ZHL / Lua 注册及现有原生证据一致。 |
| 未知内联区 | `+0x47C—+0x483` 8B UNKNOWN；`+0x484—+0x493` 是 ctor 清零的 16B 内联区，容器具体类型/元素/owner 未闭合；`+0x494—+0x4A7` 20B UNKNOWN；`+0x4A8—+0x4B3` ctor 清零的 12B 内联容器头，现有研究将其识为 Authority 私有 vector，不复制；`+0x4BC—+0x4BF` stride 尾部 UNKNOWN。 |

### 变体复用区 `+0x420—+0x43F`

公开 REPENTOGON `LuaEntityEffect.cpp` 仅允许 variant 54、113、126、167 的 TearFlags API 读写从 `+0x420` 起的 `BitSet128`；variant 136 的 `GetGridEntityDesc` 将完整 `GridEntityDesc` view 映射到 `+0x420`。项目已有 J273 Init/creator/render 证据确认 variant136 的完整 32B descriptor 由 Room creator 后写入，不能按 16B 截断。

| Variant arm | 范围 | 结论 |
|---|---|---|
| 54、113、126、167 | `+0x420—+0x42F` | 16B TearFlags，公开 wrapper 限定变体；`+0x430—+0x43F` 未知 |
| 136 | `+0x420—+0x43F` | 完整 `GridEntityDesc` 0x20B；其 8 个命名成员与 `include/IsaacRepentance.h`/`GridEntityDesc.zhl` 一致，内部 padding 不单独发送 |
| 其他/base | `+0x420—+0x43F` | 本轮没有穷尽各 variant 的写者/读者；前 16B Init 清零不等于可发送。是否含指针、嵌入 owner 或不适用区仍 **UNKNOWN**。不做 opaque 16B memcpy；基础臂保留 32B 未知并在布局中显式预留。 |

### ExtraData holder 与三种表现 payload

Effect 对象 `+0x4B4/+0x4B8` 是共享 owner 的两字槽：第一字为 payload 指针、第二字为控制块。`0x606310`（RVA `0x206310`）通过控制块处理旧 payload 并写 holder 两字；Effect Init 在 `0x5CD0A5—0x5CD0B5` 调 `0x606310(+0x4B4,0)`，ClearReferences `0x6044A0` 也经同一 setter 清除。该 owner 绝不可 memcpy 到另一对象；Client 必须构造具体动态类型并经 native helper 替换。

| Variant / payload | 原生大小与成员 | 证据与限制 |
|---|---|---|
| 108 `MrMeData` | `0x128`B；`+0` vptr、`+4` UNKNOWN、`+8` 内嵌 ANM2 `0x114`B、`+0x11C—+0x127` UNKNOWN 12B | 项目既有 IDA：分配点 `0x5CF2A1`、ctor `0x5CCE00`、dtor `0x5CCE60`、vtable `0xB14C0C`。Render case108 只在状态条件满足时动态取用。ANM2 有资源/内层状态，不能按裸字节指针复用。 |
| 166 `SpriteTrailData` | `0x18`B；`+0` vptr；`+4—+0x17` 为 20B deque/hash 容器状态：proxy/sentinel、map、capacity、offset、count | ctor `0x5D1720`、dtor `0x5D17B0`、vtable `0xB14BE0`；Init 分配大小 `0x18`，setter 为 `0x606310`。Render 检查 count (`+0x14`) 并遍历内部记录；不能浅拷贝 sentinel 或 map。 |
| 191 `PickupGhostData` | `0x458`B；`+0` vptr；`+4` ANM2[4]（各 `0x114`B）；`+0x454` i32 count | Init `0x5D0FF2`、dtor `0x5D17E0`、vtable `0xB14BF8`。Init 按数组构造四个 ANM2；Render case191 检 count/宿主状态。 |

**SpriteTrail 记录置信度**：现有实现注释 `client_entity_owned_native.cpp` 将节点定义为 64B（Position、ColorMod、三个尾部 word）；Render case166 可复读 `0x6013F6` 取记录地址、`0x6013F9` 读 `[record+0x34]` 为 f32、`0x601478` 与 `0x601660` 读 `[record+0x3C]` 为 f32、`0x60167D` 读 `[record+0x38]` 并在 `0x601682` 将该原值传给 `0x419D30`。审阅确认 `0x34` 用于距离阈值并与 `0x3C` 相乘；`0x3C` 用于颜色 alpha；`0x38` 在该路径作为原 32-bit 参数透传，尚无证据称为 Rotation。`+0x38` 应按 opaque 32-bit 保存，不作有限浮点校验。

Render 的 `+0x34/+0x38/+0x3C` 访问证明这些位置被消费者读取，不能单独证明每条记录的实际分配/拷贝步长恰为 0x40。0x40B 节点大小目前是**实现注释与最大已见偏移形成的推断**；本轮没有找到 producer 或记录分配点来闭合 stride。Position/ColorMod 成员名来自现有实现注释；在 producer 未定位前不声称节点完整布局已由 J273 单独证明。

### Timeout 与 ExpireFrame

直接观察：Effect Init 默认在 `0x5CD1E7` 写 `Timeout=-1`；若干 variant Init 分支又写 `0x14` (`0x5CDC73`)、`0xF0` (`0x5CE6A3`)、`8` (`0x5CE6DD`)、`0x19` (`0x5CEE14`) 等值。Update 是巨型 variant switch，不应把一种分支推广到全部变体。

- 在一条分支，`0x5E6FE2 cmp [entity+45Ch],0` 后 `0x5E6FE9 jle 0x5E7006`；仅 Timeout > 0 时 `0x5E6FF7 sub edx,1`、`0x5E7000 mov [entity+45Ch],edx`。另一条路径 `0x5E722C cmp Timeout,0`、`0x5E7233 jle 0x5E7250`、`0x5E7241 sub ecx,1`、`0x5E724A` 写回。`0x6006D8—0x6006F6` 也呈现正值才减一，并在减后不大于零时将 Exists/Dead 相关 byte 置位 (`0x600711 [entity+173h]=1`)。
- 反例/其他更新方式：`0x5F35A5` 从 `+0x450` 读取值，`0x5F35AB` 减一后直接写 Timeout `+0x45C`；其他 switch 分支也将 Timeout 设为常数。`0x5D3F74` 对一个 variant 路径区分负值与非负值。故“所有 Effect 每帧无条件 `Timeout--`”不由这些静态证据支持；完整分支归属仍 **UNKNOWN**。本地 Wiki 仍保留其公开描述，不覆盖这项 J273 代码观察。
- 现有设计按每次 Host 采集窗口重新计算 `expire_frame = game_frame + raw_timeout`，Client 使用同帧锚还原。即使本帧走冻结、变体改写、正值倒计时或负值哨兵路径，变化都会表现为新的锚点并产生块更新；这保证代数一致，但若某路径长期保持 raw timeout 不变，锚点仍会随 frame 改变，可能带来持续重发。不要从“ExpireFrame 已采纳”推断 native 分支恒线性递减。

## 未闭合项

1. Projectile `+0x470` 的对象语义/Plus owner 关系、`+0x474—+0x477` 尾部以及 `+0x430—+0x437`、`+0x440—+0x447` 的字段归属。
2. Effect 基础 arm 的 `+0x420—+0x43F` 以及除 TearFlags/GridDesc 外其他变体对该段的读写和指针使用；未穷尽变体，不发送为普通状态。
3. Effect `+0x484—+0x493` 容器具体类型、元素和消费者；`+0x494—+0x4A7` 的字段；池步长尾部实际是否由对齐构成。
4. SpriteTrail 记录的 producer/分配路径、实际记录 stride 及 `+0x38` 语义；当前 Render 证据仅证明消费者访问所述字节。
5. EntityEffect Timeout 的全量 variant 分支与 negative sentinel 的具体行为；本报告只给出可复读的递减条件和相反赋值证据。

## 主智能体审阅与最终采用（2026-11-09）

研究由用户指定的一个 GPT-6 Luna（max）子智能体执行。主智能体直接复读官方 Slot ZHL／Lua binding／EntityPlus，以及 Slot Render、Projectile Init／DepthOffset／64位 flags 访问器、Effect／MrMe／Trail 构造、Ghost 析构、shared-owner 替换和 RTTI 链，采用结果见[本轮实现](implementation-4-slot-projectile-effect.md)。此外直接读回以下原始指令：`5CD265` 的16B清零、`65488E—6548AF` 的 ChangeTimeout 正值检查、`5E6FE2—5E7000` 的正 Timeout 递减、`65A94D—65A961` 的 Slot signed-word 检查／减写，以及 `6013F9／601478／601660—601682` 的轨迹消费。

- **采用**：Slot 真实 i16／u16 字段及独立奖励 ANM2；Projectile 两项 u64 flags、公共 DepthOffset 单份映射；Effect 六臂和三个具体 owner。共享 helper `606310` 由实际控制块虚方法处理旧 owner，再写两字 holder，禁止直接跨进程复制。
- **纠正**：子智能体中间消息将 `64F4B0` 称为 Projectile ctor，最终按 vtable slot1 和实际 Base Init 调用更正为派生 Init。Slot 旧证据对 `+460/+470` 的“loaded/visible”名称不采用；它们是奖励 ANM2 `+34/+44` 的实际访问。原始失败／中间纠错保留在会话和旧文档语境中，不把旧名称提升为本轮事实。
- **撤销不充分推断**：基础 Effect 的 SDK BitSet128 占位不具备通用字节语义，32B保持未知、FX2预留。轨迹 `+38` 不称 Rotation，按 u32 原位模式保留且标“推断”，不施加 f32 有限值限制；主审亦复读 `419D30` 将该参数写入渲染记录 `+20`，没有据此补造角度语义。
- **记录大小边界**：`EffectTrailNode` 的0x40只作为已访问前缀（`extent=base_prefix`）；容器的64B stride仍是推断，读取资格要求 `entity-current-window-and-confirmed-trail-record-layout`。第10/13步必须先核定producer／实际记录布局，未知则报Gap，不能直接把现有适配器注释当作完整对象定义。节点和容器不宣称完整。
- **计时边界**：ChangeTimeout保留公开原值，不根据字段名构造递减锚点，也不声称本轮已闭合与FrameCount的具体公式。Effect的每窗 `A=F+T`、同world帧 `T=A−F` 是代数可逆转换；原生分支的停走、负值和跳变影响重发量，不能据单条减写推广为全变体线性时钟。

以上为静态主审，不是测试。主侧普通产品编译结果另记于实现说明；本报告开头“没有构建”仅指子智能体的逆向执行范围。
