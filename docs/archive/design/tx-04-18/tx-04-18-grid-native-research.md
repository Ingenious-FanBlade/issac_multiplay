# tx-04-18 I13—I18 GridEntity 原生静态逆向笔记

日期：2026-11-02。目标二进制为 Isaac/REPENTOGON v1.9.7.12.J273，IDA image base `0x400000`。

> 2026-11-10补充：二进制重构版本52的[七类字节与同步复核](../binary-state/grid-seven-subclasses-evidence.md)取代本文相关上线决定，并纠正Poop默认EntityRef向量来源：原始指令从BEB900/BEB904全局地址读取，不是反编译标注的a7栈来源。下文保留原时点记录与未决，不追溯改写。

## 范围与证据

本笔记用于 I13—I18 的本地 GridEntity 纯值安装、资源安装、销毁与渲染研究。先核对了 [项目规范](../../refactor/project-conventions.md)、[I13—I18 实施计划](tx-04-18-implementation-plan.md)、[Grid 逐类任务](tx-04-18-grid-application.md)，再查本地原版 Wiki `F:\projects_isaac\IsaacDocs\docs`、REPENTOGON Wiki `F:\projects_isaac\REPENTOGON-1.1.3\docs\docs`、源码及 `include/IsaacRepentance.h`、`libzhl/functions/*.zhl`、`mod/binding/hooks.json`，最后用 IDA MCP 只读核对 J273 反汇编/反编译。没有运行游戏、probe、测试，没有修改代码或 IDB。

地址下文同时给出 VA 与 RVA（RVA = VA - `0x400000`）。“已证实”表示直接见到目标二进制指令或 SDK/Wiki 声明；“推断/未知”会另行标出。

## 工厂、类型与 Init/PostInit

### `Isaac::CreateGridEntity`

- Native VA `0x706100` / RVA `0x306100`；SDK 声明见 `include/IsaacRepentance.h:10821`，ZHL 声明见 `libzhl/functions/Isaac.zhl`。
- ABI 是 x86 fastcall 形式：`ECX=GridEntityType`、`EDX=gridIndex`。ZHL `static cleanup Type<ecx>,Index<edx>` 和 SDK `__fastcall` 一致；`hooks.json` 中的 cdecl 元数据不能用来决定直接 native call 的寄存器传参。
- 函数按类型分配内存、调用基类 `GridEntity` 构造、安装派生 vtable 并初始化派生类非平凡成员。最后写 `Desc.Type`（对象 `+0x04`）与 `gridIndex`（`+0x24`）。它本身不调用 `GridEntity::Init`、派生 `InitSubclass` 或 `PostInit`，不把对象挂入 Room，不触发 Spawn 回调、掉落或派生随机逻辑。
- 基类构造函数 VA `0x704FB0` / RVA `0x304FB0` 初始化基类描述、RNG 与主 ANM2。工厂中由 `std::string`、`std::vector`、ANM2 组成的派生字段也有各自构造/空头初始化，故该工厂是获得真实 vtable 和可正常析构对象的入口。

工厂 switch 的关键映射：`1 Decoration`；`2–6,22,24–27 Rock`；`7 Pit`；`8–9 Spikes`；`10 Web`；`11 Lock`；`12 TNT`；`13 Fire`；`14 Poop`；`15 Wall`；`16 Door`；`17 TrapDoor`；`18 Stairs`；`19 Gravity`；`20 PressurePlate`；`21 Statue`；`23 Teleporter`。官方枚举确认 `GRID_TELEPORTER=23`、`GRID_PILLAR=24`，所以 Pillar 24 使用 Rock vtable，而 27 仍是 Rock 派生类型。工厂为上述 Rock 类型统一安装 Rock vtable；`ToRock` 使用 RTTI dynamic_cast，因此 type 24 等被工厂映射到 Rock 的对象能合法 cast 到 `GridEntity_Rock`。

### Init 与 PostInit 的可用性

- `GridEntity::Init` VA `0x705070` / RVA `0x305070`：写 `Desc.SpawnSeed`，将 `CollisionClass` 清零，调用 vtable slot `+0x28` 的 `InitSubclass`，增加 `SpawnCount`，设置 `Desc.Initialized=true` 并写当前 `SpawnFrame`。因此它不是一个只做字段归一化的无副作用初始化器。
- Fire `InitSubclass` VA `0x67A430` / RVA `0x27A430` 会载入多组资源、写 PathTrail、清空 Ember vector，并通过 RNG 选择 variant；Fire Update VA `0x67AA20` / RVA `0x27AA20` 会读写随机状态且可创建 projectile。不要为构造最终同步态调用它们。
- Door `InitSubclass` VA `0x70C1D0` / RVA `0x30C1D0` 归一化 Desc.State、重写 PathTrail、previous state/variant 和多个隐藏字段；Door 的 Open/Close/SetRoomTypes 路径还会播放动画并读写 Room/全局状态。
- Rock、Pit、Poop、Wall、Gravity 的 vtable `PostInit` 分别落在 `0x715370`、`0x70ED00`、`0x713A20`、`0x71B870`、`0x973AF0`；这些路径不作为纯值构造步骤。工厂没有调用它们。
- 结论：可以用工厂获得合法派生对象，然后通过具名 setter/字段和有效资源安装静态最终态所需字段，不调用派生 Init/PostInit；这是针对“本地展示投影”的可行构造路径。没有证据证明跳过 Init 后对象可直接承担原生 gameplay/update/碰撞角色，也没有在游戏中验证渲染结果。Room 缓存、碰撞、PathTrail 等语义必须由投影 owner 自己维护或限制使用范围。

## 派生类尾部的真实 Render 输入

### Fire (type 13)

工厂分配 `0x390` 字节，vtable 为 `GridEntity_Fire`。SDK 中 `GridEntity_Fire` 仅声明基类大小 `0x154`，没有覆盖工厂真实布局。

| 对象偏移 | 原生布局/用途 | Render 是否读取 |
|---|---|---|
| `+0x40` | 基类主 ANM2 | 是 |
| `+0x154` | 第二 ANM2；InitSubclass 资源 `gfx/grid/effect_005_fire.anm2`，动画 `FireStage00` | 是 |
| `+0x268` | InitSubclass 写 0 的 timer scalar | Render 未直接读；Update 使用 |
| `+0x26c` | InitSubclass 用 RNG 选出的 0–2 scalar | Render 未直接读；Update 使用 |
| `+0x270/+0x274/+0x278` | Ember `std::vector` begin/end/capacity 三指针 | State 小于 4 时读取 begin/end |
| `+0x27c` | 第三 ANM2；资源 `gfx/grid/Effect_Ember.anm2`，动画 `Idle` | Ember 绘制时读取 |

Fire Render VA `0x67B620` / RVA `0x27B620` 读 Desc.State；State `<4` 时逐个读取 Ember，并用第三 ANM2 绘制。基类主 ANM2 的标准火焰资源为 `gfx/grid/grid_fireplace.anm2` / `Flickering`。Fire InitSubclass 还设置 PathTrail=1400，随机选择资源/variant；该路径不应被描述为“无副作用”。

Ember 元素步长 24 字节：`+0 x`、`+4 y`、`+8 vx`、`+12 vy`、`+16 age`、`+20 maxLife`。证据来自 Fire Update：新粒子分别将 `+8/+12` 写为速度；更新时将其积分到 `+0/+4`，并随机扰动速度；Render 使用位置和 age/maxLife 设置第三 Sprite 帧。故 `+8/+12` 是速度，不是邻居或 opaque padding。若用 owner 管 vector，析构前应按实际 vector 所有权策略将 native header 安全置空/恢复，避免游戏 Free 再释放外部存储。

静态投影要完整覆盖三个有效 ANM2 和有界 Ember 元素；Render 直接读取的额外标量只有 Desc.State 与 ANM2 内部绘制状态。Factory 构造 ANM2 对象及空 vector 头，但不装动画资源，也不会给 Ember vector 提供可绘制元素。

### Rock / Pillar (type 24)

工厂给 Rock 初始化 `Anim`（`std::string +0x154`）、`RubbleAnim`（`std::string +0x16c`）以及空 vector 头；`FrameCnt` 位于 `+0x190`，工厂未写它。Rock Render VA `0x7146E0` / RVA `0x3146E0` 在 Desc.State==2 分支读取 `RubbleAnim` 与 `+0x184/+0x188/+0x18c` 三指针 vector；元素步长 8 字节，为两个 float。Render 将每对值设入临时绘制 Sprite 的位置/偏移，再绘制 RubbleAnim 碎块，因此这是 rubble fragment position 列表，不是邻居表。空 vector 对非破碎静态状态合法；若同步 state==2，必须按真实容器布局提供拥有生命周期的元素。

`SetBigRockFrame(-1)` VA `0x715550` / RVA `0x315550` 会调用随机数并改 Desc.Variant、Anim；不用于无随机安装。`frame>=0` 分支是确定路径（variant 编码 `1000+frame`）。若保持自然变体，应从快照设置 Desc.Variant/Anim 并只读回验，不调用 `-1`。

### Poop (type 14)

工厂分配 `0x19c`，但 SDK 的 `GridEntity_Poop` 仅声明到 `0x174`。真实尾部大小恰好为 `0x28`，与 SDK `EntityRef` 一致：

| 相对对象偏移 | EntityRef 字段 |
|---|---|
| `+0x174/+0x178` | type / variant |
| `+0x17c/+0x180` | spawner type / variant |
| `+0x184/+0x188` | position x/y |
| `+0x18c/+0x190` | velocity x/y |
| `+0x194` | flags |
| `+0x198` | Entity* |

Poop Render VA `0x710130` / RVA `0x310130` 未读上述尾部。Poop Hurt VA `0x710280` / RVA `0x310280` 会将 `this+0x174` 作为 `EntityRef*` 交给其他 gameplay/effect 逻辑，不能把尾部误命名为 render color。工厂将 EntityRef 的 type/variant/spawner/flags/Entity* 设为零，但 position 与 velocity 的两对 float 来自 IDA 标记为 `a7` 的栈值；它们的来源语义未知。主审决定：渲染不采用该尾部，不允许未知来源补零；若后续消费者需要它，须补正式来源和引用合同。`a7` 是反编译标注，尚不能直接断言是未定义栈内容。

其他可见字段：`ReviveTimer +0x154`、`StateAnimation std::string +0x158`、`UnderPlayer +0x170`。Render 主要消费 Desc.Variant 与基类 Sprite/动画。InitSubclass 路径会根据状态选择动画/游戏状态，不需要为静态投影调用。

### Door (type 16)

工厂分配 `0x2f0`，构造 `ExtraSprite ANM2 +0x168` 和四个 std::string。原版 Lua 注册反汇编（`sub_84D340` 的 `0x852214–0x852378`）给出：

| 字段 | 偏移 | 备注 |
|---|---:|---|
| 隐藏 byte/scalar | `+0x154` | InitSubclass 清零；门动画/更新分支读取。`sub_70C3C0` 按它选择 `IdleCoin`/`Idle` 等资源状态。确切字段名未知，推断为 coin/special door 模式；Door Render 不直接读取它 |
| Slot | `+0x158` | getter 返回该 int；原版 Lua 注册没有 setter。工厂和 InitSubclass 均未写，必须由具名 DoorSlot 源值安装 |
| CurrentRoomType / TargetRoomType | `+0x15c / +0x160` | 公开字段 |
| Direction | `+0x164` | 公开字段；setter 还维护两个 Sprite 方向相关值 |
| ExtraSprite | `+0x168` | ANM2 |
| ExtraVisible / Busted | `+0x27c / +0x27d` | Render 读 ExtraVisible；Busted 由查询读 |
| TargetRoomIndex | `+0x280` | InitSubclass 默认写 -1 |
| PreviousState / PreviousVariant | `+0x284 / +0x288` | InitSubclass 从当前 Desc 写入 |
| OpenAnimation / CloseAnimation | `+0x290 / +0x2a8` | `std::string` |
| LockedAnimation / OpenLockedAnimation | `+0x2c0 / +0x2d8` | `std::string` |

Door Render VA `0x707F90` / RVA `0x307F90` 通过主 ANM2 绘制，并在 `ExtraVisible` 为真时绘制 ExtraSprite。`GetSpriteOffset` VA `0x8446D0` / RVA `0x4446D0` 返回 `this+0xd4`，即基类主 Sprite（对象 `+0x40`）中 ANM2 `_offset`（相对 Sprite `+0x94`）的 Vector；不是 Door 尾部隐藏字段。

`IsOpen` VA `0x431670` / RVA `0x31670` 的原生实现是 `Desc.State==2`。`IsLocked` VA `0x431680` / RVA `0x31680` 按 `Desc.Variant ∈ {1,2,3,4,5}` 返回 true，不依赖一个额外 bool。`IsBusted` VA `0x7329C0` / RVA `0x3329C0` 读 `+0x27d`。

Door `InitSubclass` VA `0x70C1D0` / RVA `0x30C1D0` 将 State 0 或 2 规范为 1，记录 previous state/variant、TargetRoomIndex=-1，写当前 grid index 的 PathTrail=1000，并清零 `+0x154`、CurrentRoomType、TargetRoomType、ExtraVisible/Busted 等。它不写 Slot/Direction。Door Render 不直接读 `+0x154`，但 Door Update/动画路径会读取，若只做静态投影可由源字段明确设置；不要依赖工厂未初始化值。

## Room 发布与析构

- `Room::RemoveGridEntityImmediate` VA `0x813BD0` / RVA `0x413BD0` 检查 index 不超过 447。若 `KeepDecoration=true` 且对象类型为 Decoration，它会提前保留对象；否则先维护 `PathTrail`，清空 Room grid array 对应项，再通过对象 vtable slot 0 调 `Free(this, true)`。
- vtable slot 0 ABI 是 thiscall（ECX=this，bool 参数在栈上，true 表示释放内存）。各派生 Free 负责销毁非平凡字段，再按 bool 参数释放对象。
- 已核的析构分支：Rock Free `0x7066E0` 调 `std::vector` destructor（`+0x184`）、析构两条字符串及基类 Sprite；Fire Free `0x7067E0` 析构 Ember vector（`+0x270`）、两条附加 Sprite 及基类 Sprite；Door Free `0x706730` 析构四条字符串、ExtraSprite 和基类 Sprite；Poop Free `0x7067A0` 析构 `StateAnimation` 和基类 Sprite，EntityRef 是平凡字段未单独析构。自有 vector 存储若不由原生 vector 所有，Free 前必须先清空/置空其原生 header，避免双重释放。
- Room 的 `_doors`/door-slot 表是另一份 Door 指针别名；`RemoveGridEntityImmediate` 清 grid array 不自动清该门槽。释放 Door 前 owner 必须先撤销所有门槽别名，否则留下悬空指针。
- 手工直接清 grid slot + Free 不会执行 Immediate 的 PathTrail/cache 维护。若采用独立 owner 路径，应明确维护这份缓存和 door alias，或不调用该路径涉及的原生 Room 操作。

## I13 镜像房间 pass（静态补充）

- `Game::Render` VA `0x6F7A80` / RVA `0x2F7A80` 常规路径调用 `Room::Render`；transition 状态 2/3 走转场绘制分支，不构成单独的镜像 pass。`Room::Render` VA `0x804500` / RVA `0x404500` 才包含原生镜面 surface 的生成与后续合成。
- `Room::Render` 中镜面 target pass 的条件为：`Game::_currentDimensionIdx`（`+0x1830c`）`==1`，`Game::_stageType`（`+0x4`）`==4 || ==5`，并且 `Game::_stage`（`+0x0`）`==2`，或 `_stage==1` 且 `sub_742450(game) & 2` 非零。这里是 LevelStage 条件，不是 game mode。该分支先切换渲染状态，再用全局 `dword_BE7340` 作为 active target。此条件来自函数反汇编；在 `Game::Render` 与 `Room::Render` 的目标代码路径中未发现 descriptor.Flags `0x8000` 作为触发条件。
- 后续 `Room::Render` 从同一 surface 取得纹理及尺寸并提交 composite pass。`dword_BE7340` 由 `sub_7E08E0` VA `0x7E08E0` / RVA `0x3E08E0` 惰性创建，资源名为 `Mirror Surface`，尺寸取屏幕渲染尺寸；其重置在全局渲染资源清理路径中。因此该 surface 是全局 renderer 资源，不属于某个 Room。`Room::Render` 会切换全局 active target 和 renderer state，静态证据不支持把整个函数当成无副作用的局部镜像 helper 调用。
- 可独立实现的等价展示路径是由显示 owner 持有专用 surface，将本地镜像场景绘入该 surface，再以 REPENTOGON `Renderer.ShaderType.SHADER_MIRROR` 对 quad 做专用合成。`repentogon/LuaInterfaces/LuaRender.cpp` 注册的 MirrorDescriptor 参数为 `Position vec3`、`Color vec4`、`TexCoord vec2`、`TextureSize vec2`、`Ratio vec4`、`Time`、`Amount`、`PixelationAmount`。以上专用 surface 是 Luna 的设计建议，不是原生约束；当时原生 composite 参数尚未核完。主审随后采用现有 Manager-owned Mirror Surface，并按下段已复读的原生布局安装和退休恢复格式，保持17的原生世界/图形资源 owner。

### 主审后续补证与最终采用

主智能体继续复读 `807961` 前的参数 push，确认 source quad 为 left=1/right=0/top=0/bottom=1，确实水平翻转；复读 `8079C6—807E1C` 的 Mirror Surface composite、`7E08E0` 的资源分支及静态 `B88670` 描述符，并对照实际 `resources/shaders/mirror.vs`、`mirror.fs`。原生增强分支的有效顶点布局是48字节：Position(3)、TexCoord(2)、TextureSize(2)、Ratio(4)、Time(1)，不是直接把 Lua 新渲染器中含额外声明的描述符套到原生 raw batch。每顶点 +20..+44 的七个参数依次是实际/填充宽高比例、填充宽高/(pixelScale*1024)、投影房间左上+scroll+shake 的归一化位置和时间。时间由 Authority 的 pause/Manager parity/Game frame%600 在正式普通采样窗口复制，Client 不推进。

实际实现 `client_presentation_native.cpp:MirrorComposite` 使用原生全局图形 owner 的 surface（RVA `0x7e7340`）与原生 shader（`0x7ab294`），普通关闭效果分支沿用 Image shader 和88字节格式。格式选择由正式 Source mirror_shader 给出，首次借用记录原格式，退休恢复；不是复制世界或重入完整 RoomRender。内层 prepass/颜色 surface 都恢复到外层 mirror surface，镜像合成后再绘 HUD。以上属于静态实现依据，视觉结果仍须 I29 验收。

## 其他 GridEntity 的 vtable slot 3 渲染输入

地址以 `mod/binding/world_layout.json` 中当前 J273 vtable RVA 加 image base `0x400000`，并以 IDA 原始表项核对；旧 `docs/archive/hooks/grid-room.md` 的候选地址不用于本表。

| 类型 | slot 3 VA | 直接渲染结论 |
|---|---:|---|
| Lock | `0x70CB80` | 调 `sub_705110` 由 GridIndex 计算屏幕位置；`ANM2::_loaded` 为真时根据 `_overlayRenderFirst` 绘制 base/overlay AnimationState，未加载时不绘制。 |
| Teleporter | `0x70CB80` | 与 Lock 共用上述渲染函数。 |
| Wall | `0x71B4E0` | 当当前 Room `_roomType==16` 时，先将 `Room::_floorColor` 复制到主 Sprite `ANM2::_color`，再按通用双 state 路径绘制。 |
| Gravity / TrapDoor / Stairs / Spikes / TNT | `0x719270` | 每次渲染前把 `Room::_floorColor` 复制到主 Sprite `ANM2::_color`，再按通用双 state 路径绘制。 |
| Statue | `0x4291D0` | slot 3 是 `nullsub`，函数体无读取/绘制；I21 若要展示 Statue，必须覆盖 Room 的专用绘制通路，不能依赖通用 slot-3 vcall。 |

公共输入和临时覆盖的区别：GridIndex 在 GridEntity `+0x24`，`sub_705110` 还读当前 Room `_gridWidth`（`Room+0x0c`）及屏幕缩放/尺寸全局；主 ANM2 位于 GridEntity `+0x40`，`+0xb0` 是 ANM2 `_overlayRenderFirst`，`+0x149` 是 `_loaded`。这些偏移都在基类 ANM2 内，不是派生隐藏字段。两个 AnimationState 分别位于 GridEntity `+0x70` 和 `+0x90`，已安装资源/帧姿态由 `sub_405990` 及下游 ANM2 绘制消费。目标渲染函数没有读取 Lock、TrapDoor、Statue 的派生 `+0x154`，也没有额外派生 Render cache 必须同步。

`Room::_floorColor` 位于 `Room+0x72a4`，`ColorMod` 大小 `0x2c`；主 ANM2 的 `_color` 位于 GridEntity `+0xe8`。Gravity/TrapDoor/Stairs/Spikes/TNT 通用路径及 Wall 的 RoomType 16 分支会将前者临时覆盖后者，作为当前绘制 tint。此为 Render 的写副作用，不是对象最终同步态；I21 若在原生通路绘制本地 GridEntity，应在绘制后恢复 Source 的 `_color`（或记录并回装等价原值），避免污染后续逻辑/绘制。

## 未决项

- 纯静态反汇编不能证明跳过派生 Init 后的每个 GridEntity 都能安全走完整原生 Update/碰撞/Room transition 流程；本研究仅支持真实工厂对象经具名纯值和资源安装后供本地展示投影读取。
- Door `+0x154` 的正式字段名和所有合法值未知。已经确认它是被清零、被动画/更新读取的隐藏 byte/word；coin/special door 含义是根据 `IdleCoin` 分支作出的推断。
- Poop 工厂 EntityRef 中 position/velocity 的 `a7` 反编译来源未进一步核定；渲染不读取，本轮不采用，不补零或据此声称完整跨域引用已完成。
- Rock state==2 Rubble vector 的原生生产算法未穷尽；Render 读取形状及两个坐标已证实，坐标精确参考系仍是基于传入 ANM2/Sprite position 的推断。
- 本文不重复 I13 Minimap/Camera 分析；Minimap SavedEntities 和 challenge RNG 投影边界、Camera 字段由主智能体单独主审。
