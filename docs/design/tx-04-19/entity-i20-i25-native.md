# tx-04-19 I20–I25 实体绘制原生静态合同

状态：`J273_STATIC`。目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA ImageBase=`0x400000`。本文只记录本地 Wiki、官方 REPENTOGON 资料及目标二进制的静态观察/推导；没有运行游戏、构建或测试，也没有修改 IDB。VA 与 RVA 按 `RVA = VA - 0x400000` 换算。这里的 `SUPPORTED` 只表示已找到可实施静态路径，不表示像素、运行时副作用或异常恢复已经通过验收。

## 阅读顺序与已有合同

先查本地原版 Wiki `F:\projects_isaac\IsaacDocs\docs` 与 REPENTOGON Wiki `F:\projects_isaac\REPENTOGON-1.1.3\docs\docs`，再查官方 `LuaInterfaces/Entities`、`libzhl/functions`，最后用 IDA 只读核目标实现。类别字段、池/构造入口、Effect 136 与 Slot setter 的通用资料继续见 [entity-native-static.md](entity-native-static.md)；实体创建/应用的正式输入合同见 [entity-application-contracts.md](entity-application-contracts.md)。本文只补 I20–I25 消费路径与具名缺口。

## I20 Slot：最终资源与有限派生绘制

REPENTOGON `LuaEntitySlot.cpp` 及 EntitySlot ZHL 把 Slot 成员绑定到下列宽度；各字段互不打包：

| 成员 | Slot 内偏移/宽度 | 说明 |
| --- | --- | --- |
| State | `+0x410`, i32 | Init 后状态 |
| PrizeType | `+0x414`, i32 | 当前奖励类别 |
| ShellGameAnimationIndex | `+0x41A`, i16 | signed short 存储 |
| Timeout | `+0x41C`, i16 | Lua getter 对 short 符号扩展；Wiki 的 16/17 位说明与本版本实现冲突，未找到其它存储 |
| DonationValue | `+0x420`, i32 | 只读当前值，不据此支付/抽取 |
| TriggerTimer | `+0x424`, u32 | 直接字段 |
| Touch | `+0x42A`, u16 | 直接字段 |
| Prize ANM2 | `+0x42C`, 独立内嵌资源 | 与主 Sprite 分离；不能从 PrizeCollectible 本地重建 |
| PrizeCollectible | `+0x540`, i32 | SetPrizeCollectible 对所有 Variant 都先写该标量 |

`Slot::Render VA 0x6620E0` 为 `void __thiscall(Entity_Slot*, Vector*)`：无条件调用 common `0x6AC4E0`，之后只对 Variant 6/15 读取 Slot Variant `+0x2C`、Prize ANM `_loaded`（对象 `+0x460`）及 `_visible`（`+0x470`）；两个条件成立时调 `0x4059E0(prizeANM +0x42C, Entity render-cache +0x1A4, zero, zero)`。Variant 1–5、7–14、16–18 没有 Slot 专属 Render 分支；它们仍需要 common Entity 状态与最终主 Sprite。Shell/Hell Prize ANM 的最终层、动画和姿态必须来自当前 Source。

`Slot::Init VA 0x659AA0` 先调用 base `0x6A4A20`，再按 Variant 组装 config/Main Sprite/动画；Variant 6/15 交换 Prize/Main ANM 状态，8/11 建奖品数字层且有 jam RNG/虚 Remove 路径，14 依 Player 状态选择配置动画。因这些配置、资源、随机与虚调用副作用，不将完整派生 Init 当纯构造/刷新入口。Client 可以选择“base Init + 完整 Source 最终共同字段和资源”这一由最终值供应的原生表现承载前缀；6/15 还须安装最终 Prize ANM。它不等价于执行过官方 Slot 派生 Init，也不能调用 Update、SetPrizeCollectible、开奖或 Jam helper 补画面。

## I21 Text 与 Font：文本状态和文字帧分开取值

Text Init `VA 0x5CC260` 调 base Init，清 counter `+0x410`，按 Variant 设置 duration `+0x414`：0→90，1→25，2→35，其它→1000；初始化完整 KColor 区 `+0x518..+0x52B`（20B）。`sub_9E35B0(-1)` 只是把传入4个颜色字节归一化为 RGBA float 并写 KColor 20B 中前16B和opaque dword，不消费 RNG。Variant 1 另写专用颜色。Init **不清文本 buffer `+0x418`**；槽复用时不得把旧字符串当作新 Source。已知写入路径 `0x6FF270` 会按容量 `0x100` 格式化字符串并进入 Update/计时；它证明该具体 writer 存在，不证明其 caller 是所有 Text 的普遍业务来源。

Text Render `VA 0x5CC580`：Entity Exists byte `+0x172` 为真才继续；读取字符串 `+0x418`，counter `+0x410`、完整 20B KColor `+0x518`。variant 2 还读取游戏 pause 与 Manager frame，使用 `(manager_frame + 1) % 2` 的未暂停子帧（暂停时 0）；绘制大小为 `4 - 2.5 * clamp((subframe + 2 * counter) / 8, 0, 1)`，并据此计算水平居中和 `y - 4*size`，scaled Font 路径。其它 Variant 不使用此交替 phase，走 unscaled Font 路径。该计算是独立文字绘制 phase，不是 ANM Sprite 的 frame/subframe。Render 会更新公共屏幕坐标缓存 `+0x1A4/+0x1A8`。

SDK `Font.zhl` 的可用接口为 `Font::GetStringWidth(const char*)` 与 `Font::DrawString(const char*, Vector position, Vector scale, KColor*, FontSettings*)`。低层 `0x9E9500`（unscaled）及 `0x9E9680`（scaled）都把字符串临时对象、scale/color/settings 拆成混合寄存器/栈参数，再经 `0x9EA2A0` 和临时对象析构；不要按 Hex-Rays `__userpurge` 猜参数直接调用它们。文本宽度 helper `0x9E9E40` 是 `thiscall`，其公开 ZHL 用法以 SDK `GetStringWidth` 为准。用 Manager 已加载的官方字体与 SDK Font 方法窄消费；字体资源、FontSettings、20B KColor 均保留真实所有者/布局。

数据合同：`native-text-render-v1` 保留字符串、counter/duration/color、font、实体 offset 及独立文字 phase。普通 Source 的同一窗口 `application_draw` 可给出 manager frame/pause/subframe；若复用普通窗口 phase，必须有当前 Source 同窗口证明，Variant 2 且 pause/frame 彼此一致。不把旧独立记录拼成当前普通事件。Static research 未找到可代表所有 Text 对象的真实产品 producer；缺少当前 writer 的字符串证据、完整颜色或合法文字 phase 时应具名失败，不读取池残留、不运行 Text Update。

## I22 Dummy：专用预构造池和本方取得策略

目标 `EntityFactory` 内的 Dummy 专用数组位于 `+0x675650`，128项、stride `0x410`；构造/析构 VA `0x6A3570/0x6A35C0`，动态 vtable `0xB181B8`。Dummy pool ctor `0x6A3570` 建好槽后 `_valid +0x170` 与 `_exists +0x172` 均为 false。这个 pool 已预构造，是专用存储，不等于 `EntityFactory::Create` 的通用路径。

通用 Factory 没有 Type 964 case；`Create(964, true)` 会选择 NPC 池，不能作为 Dummy。当前可实施的 Client acquire 是本方策略：主线程、Factory 所有者有效时，只读扫描这 128 项，要求精确 Dummy vtable 且 `valid=false && exists=false` 后借该槽；不更换动态类型、不碰活槽、不维护全局 cursor，之后走限定的 base Init/最终值安装/正常 Add 发布和正常 remove/退休。槽池构造已先建立所有对象；这不是找到/复刻了原版创建 caller，也不是可并发池分配器的普遍合同。

Source 分类应以官方对象动态类型补充（Dummy TypeDescriptor/vtable），输出具名类别 `NativeDummy`，不可仅看 `_type`，更不可将通用 NPC `Type=964` 当作同类。`Dummy::Render VA 0x6A3630` 先 common `0x6AC4E0`，随后格式化 `e%d.%d` 并调字体 `0x9E9500`；独立 Dummy Source 需提供两段实值和当前字体绘制上下文。没有原生合法 caller/业务获取点的静态证据，因此文档只支持该本方专用池取得策略，不能声称追到了原版创建语义。

## I23–I25 绘制分派、临时状态与多 pass 次序

Entity 主 Render vtable 槽5使用 `void __thiscall(Entity*, Vector* offset)`。Entity Shadow 槽6在 Base/NPC/Familiar 分别落到 `0x6AD450/0x6D0650/0x61E320`，前三者均消费 `Vector*`；Laser 槽6地址 `0x955290` 的 IDA prototype 曾多推断一个EDX形参；主审按实际caller与raw指令更正：`4155E1..4155E9`以ECX=Entity，只push一个offset后调slot6；`955325`首次涉及EDX即赋新值，没有读取传入EDX。因此采用官方virtual同一thiscall单offset，不把Hex-Rays fastcall猜测变成ABI。PostRender 是槽7、无显式参数：Base `0x6AC660` 与 Laser `0x956F80` 均无栈参数；Laser Post 同样不能传 Vector*。

PostRender 的真实外层位置已从 Room `0x804500` call-site 静态确认：`0x80619A–0x8061A3` 调 `0x415350(UINT_MAX)` 准备 EntityList renderEL(+`0x54`)；循环依已经消费的 cursor(+`0xA0`)至当前列表上限，在 `0x8061C8` 调 `[vtable+0x1C]`（槽7，无参数）；`0x8061E8` 才将 mode(+`0xA4`)置0。若 Client 绘制重建这条路径，槽7应按同一 Entity 集合一次消费，排在所有 Main/柱尾 draw 后、Light 合成前。不要把 PostRender 加进每个 Main Render wrapper 造成重复，也不要把它按 `Render(Entity*,Vector*)` 调用。

Base Shadow `0x6AD450` 使用 Entity 的最终位置/高度/缩放/可见及当前 Sprite 状态；NPC/Familiar 覆写在调用 Base 前可做额外分支。Laser Shadow 单独消费 Laser/Homing 几何、样本与状态，不能用 base 阴影代替。Light 是 Room `RenderEntityLight` 专门预/合成 consumer，不是已定位的 Entity 虚槽；不得通过调用 Entity 主 Render 伪造它。Main/Shadow/Light/Post 的 hook guard 必须分别围住实际消费点，包括水面间接虚分派，保证每个对象每个 pass 只消费一次。

本用途原生 draw 并非只读：

- common Render 和类 helper 会临时写 Entity render cache `+0x1A4/+0x1A8`，部分消费者需临时目标 frame/动画层可见状态。Client draw scope 至少以具名宽度保存并恢复其改写列；child 资源借用要覆盖子 ANM2、子公共 draw-cache 和 helper 临时变换，不能只 snapshot parent ANM。
- Pickup/Familiar draw 使用全局 xorshift state：Pickup `Base+0x7D0CA4`（4×u32），Familiar `Base+0x7D0674`（4×u32）。同一 Source 普通窗口给每个 stream 一次 seed，FrameScope 跨全体 draw 连续消费并在整帧退栈时恢复真实 RNG；逐实体重置会让不同实体消费相同序列，改变全局 draw 的随机分配。
- Pickup Variant100消费最终底座/价格ANM，并临时写wobble与inline随机；已确认的lazy资源构造/Reset/Load属于Bomb `6A0F90→6A2420`，由Source五层最终资源和loadCostumes=false阻断。Familiar Variant 231 AbyssLocust 的 `0x647310` 含 ANM Update/Reset，不可为 draw 调；以 Source 最终 ANM/姿态直接 Render，避免淡入、资源缩放、事件和逻辑推进。
- 具名 Player 复合 draw 使用短时借用 scope；仅改该分支读到的 Player 关系、ColorMod/Visible/Position/offset/frame 与 Sprite owner，离开前恢复精确原值。不要把该依赖扩成完整 Player 状态，也不能清掉 Player 所有状态。

## NPC 复合 draw 输入（`NativeNPC.Composite`）

NPC ctor `0x6B4520` 初始化内嵌 ANM2 owners `+0xCD8/+0xDEC`。Type-specific helper 还读 private C++ deque；这些不是可复制的 `std::deque` 头或跨进程地址。按 ctor、push/clear helper 静态登记如下：

| 容器 | NPC header字段（每项u32） | 元素/块形状 | 消费/生产 |
| --- | --- | --- | --- |
| Composite references | `+0xBF0` proxy, `+0xBF4` map, `+0xBF8` capacity, `+0xBFC` offset, `+0xC00` count | 元素为4B `EntityPtr`，16B block容4个。实体槽需调用官方 `SetReference`，维护 child `+0x3CC` reverse observer | NPC Update helper `0x515AF0`登记/切换；Type835/906/920读取 |
| Secondary path | `+0xC04` proxy, `+0xC08` map, `+0xC0C` capacity, `+0xC10` offset, `+0xC14` count | 元素为 Vector2 8B，16B block容2点 | Type19 helper `0x4A1540`每个新 child 后 append 父NPC当前位置`+0x33C/+0x340`；`0x54C1F0`会清理/限长；Type921主要消费者 |
| Primary path | `+0xC18` proxy, `+0xC1C` map, `+0xC20` capacity, `+0xC24` offset, `+0xC28` count | 元素为 Vector2 8B，16B block容2点 | NPC path 更新/曲线路径；Type835 在合法类型门槛通过时调用子体路径 helper（普通 C/S scope仅10..999 NPC）；951:3消费者 |

C00 元素必须按 Source 顺序稳定 key 两阶段解析成本地 EntityPtr，不写 Source 裸指针。退休先对每个槽 `SetReference(nullptr)` 消 reverse relation，再按游戏 allocator 释放每个16B block 和 map，保留 ctor 建立的 proxy；各容器分开清，不能触发 child Remove。C04/C18 复制完整有序坐标值，按游戏 allocator 重建本地 map/block，保留本地构造的 proxy。所有 count、capacity、offset、index 必须有界，并满足 consumer 的索引读边界；Source 不可只传 count。

| 类型/变体与入口 | 实际消费/临时写 | Source / 本地处理合同 |
| --- | --- | --- |
| Type 835 — NPC Render `0x6CEC90`→`0x555410`，path child helper `0x4E4710` | `555410`在 C00 count 非零时固定读前两项。仅当 `Game+0x25B8C >= 2 && child.Type==1` 或 `10<=child.Type<=999` 才以 child 为 `this` 调 helper。普通 C/S scope（预检证明 state `<2`）不进入 Player 分支；真实 NPC child 仍走其自身 C18 ring。 | C00 children按稳定Key重建；NPC child供完整C18值。Player只保留稳定Key关系，不读／写 `Player+0xC18..0xC28`。若目标 scope不是普通状态，不得据 NPC布局解释Player内存；当前将其作为不支持的 Deathmatch Player-path形态处理。 |
| Type 906 / 920 — Render→`0x4B52A0/0x4B6680` | 遍历 C00 EntityPtr child，按每个 child 当前 Sprite/Layer state 画图；辅助会借用 child ANM及公共 Render 状态 | 为每个 child 供稳定Key、当前动态类别、Main/aux资源pose、动画状态；借用期保存/恢复 child ANM 与确实改写的 common render cache/位置/可见/frame。只 snapshot child Sprite 不够。已有具名Source当前值可以进 native layer consumer，无需凭空造对象字段 |
| Type 921 Variant 0 — Render→`0x574930` | 检查 Variant、从 `+0xC04..+0xC14` 读取 secondary path；切换 main Sprite layers 1/2 的 Visible、计算 `layer13 alpha/layer14 visible/angle`，使用 Entity `+0x148` FlipX；没有随机读写 | Source完整C04序列、`Entity.FlipX`（bool `+0x148`）及精确主ANM layers；逐实体 draw scope 恢复所改可见/alpha/角度和缓存。此处不消费C18 primary path |
| Type 951 Variant 3 — Render→`0x599070`→`0x598B10` | 仅 `C18 count >= 9` 进入；构造全局 scratch 曲线/顶点，每组三个点步进并读取邻点，最终 `0x419D30` draw；未见 NPC 专有随机或附加标量读取 | Source给C18完整有序点列及有效边界；本地提供helper实际访问范围的guard。consumer的步进/邻点读取不等于“只读`count`个点”，长度尾组如不能保证被索引点存在必须失败，不能拿out-of-range map数据补齐 |
| Type 965 Variant 10 passenger | 先扫 PlayerManager roster，过滤 `Player.GetMinecart()`（Player `+0x3D4`）等于本NPC；读取每个 Player `+0x205C/+0x2060` 两个 f32偏移，临时改 Player 位置/可见并 virtual Render | 不需完整 Player20；复用已ready、同窗口的稳定 Player key，借用 `Entity.GetMinecart` 关系与二个偏移，恢复 Player原位置、PositionOffset、Visible、Minecart reverse-ref、frame及主/服装ANM。以稳定 roster 顺序保持原循环消费顺序 |

Type965 Variant10 helper并未读取 NPC `+0xF40` Siren actor。`+0xF40` 是具名 gameplay owner，不应因为它能在 Source 中观测就强行把它创建为客户端NPC或纳入本轮画面；客户端本用途保持该 owner null/AuthorityOnly。若后续实际 draw consumer出现该actor读取，再另补具名关系。

## Familiar复合Player与Abyss ANM

Familiar Render `0x61E360` 读取 player 裸借用指针 `+0x410`。Variant68使用当前Player主 Sprite的layers 0、14、4..10/overlay；先对 Player ColorMod 经虚 `SetColor` 写临时 tint，再恢复原 ColorMod。variants78/243先保存 Player Visible byte `+0x171`、写1、绘制，再恢复原值；这三支没有临时改Player位置。ColorMod从Player `+0x1B8..+0x1E3` 完整44B保存恢复，不能只保RGBA四float。

相应 Source 必须是同当前Scope已ready的 Player owner及其正确 Sprite/costume ANM；Familiar `+0x410` 是借用，不创建/拥有新Player。Variant 68 color 与78/243 visibility 分支分别按原始对象值恢复。Familiar全局随机 stream 是前节 `Base+0x7D0674` 16B状态；若当前变体确实进入消耗分支，按普通 draw window 的统一 FrameScope 安装/连续消费并恢复。

Variant 231 AbyssLocust 所有副ANM来自具名 ExtraData owner；`0x647310`执行ANM Update/Reset而非纯draw。只将Authority最终ANM frame、layer、shader与纹理状态安装到其真实本地owner并窄Render，不触发该更新helper，也不重算fade/scale或播放事件。

## 835 Type 1 条件、Deathmatch 状态与 Player 字段归属

本节更正首次 I20–I25 静态汇总中的误读。原记录把 `555410` 的 `Type==1` 条件读成“没有早退”，又把 NPC 的 `+0xC18..+0xC28` ring 布局套到 Player 上，因而提出 `npc835_path_empty` 与“非空 Player 私有路径 owner 未知”。这两个推论均撤回：分支有明确的 Deathmatch 状态门槛；Player 对应字节属于一个内嵌 ANM2，并非路径容器。它是静态分析中的纠正，不是一次运行时失败或运行时观察。

本地原版 `enums/EntityType.md` 将 `835` 命名为 `ENTITY_PEEPER_FATTY`。原版 `EntityNPC.md`/`PathFinder.md` 与 REPENTOGON `EntityNPC.md`/`Pathfinder.md`公开的是 NPC PathFinder API；没有 Player waypoint/path-list API。REPENTOGON 官方 `Game.zhl` 将 `Ambush` 放在 `+0x25B40`、`DailyChallenge` 放在 `+0x25BA8`，未命名 `+0x25B8C`；具体语义依据目标二进制而非对 gap 的猜名：

- `0x4257A0` 收到 `this=Game+0x25B8C` 与 selector 1–11 后，按选择创建带 RTTI 名称的 `DeathmatchTypeNormal/Ipecac/Bomberman/Ludovico/Sumo/Knockout/SwordFight/MachineGun/Indigestion/KnifeFight/Venom` 对象并存于 `this+0x0C`。`0x4249D0` 以状态 word `*this-2` 分派该对象；这是 Deathmatch 状态机。
- `0x425180` 将该状态 word 设为 2、清 `+0x04` 计数、播放 Deathmatch intro，并调用 `0x6F9710` 到特殊 room index `-19`。`Game::End` (`0x6F5920`) 仅在状态 `<2` 或 `==6` 走普通结局分支。`0x8B14F0` 在重置处清状态为 0 并选取 DeathmatchType。综合这些直接交叉证据，`Game+0x25B8C>=2` 是 Deathmatch 子状态条件，不是 Online 开关；不能从数字 2 推导联机语义。
- `Entity_NPC::Render` 进入 Type835 后调用 `0x555410`。其 raw 分支 `0x55546F..0x55548E` 先检查 `Game+0x25B8C>=2 && child.Type==1`，否则仍接受 `10<=child.Type<=999`；`0x555463` 已把 child pointer 装入 ECX，故 helper ABI 是 `void __thiscall(Entity* child, Vector* renderOffset, char flag)`，调用传入 offset 与 `flag=0`。所以 type 1 child 确实只在 Deathmatch 子状态到达 `0x4E4710`。
- `Entity_Player` ctor `0x74DC70` 的 `0x74DCF5..0x74DD07` 对 `this+0x758` 调 EH vector constructor：元素 stride `0x114`、count 7、元素 ctor/dtor `0x402B00/0x403790`。索引4（第5项）起点为 `0x758 + 4*0x114 = 0xBA8`。REPENTOGON `ANM2.zhl` 明列 `+0x70 _overlayRenderFirst`、`+0x74 _animData`、`+0x78 _animCount`、`+0x7C _layerState`、`+0x80 _layerCount`；因此 Player `+0xC18..+0xC28` 是该 ANM2 元素内部字段，`+0xC28` 是 `layerCount`。构造与销毁归 Player 持有的七项 ANM2 数组及 ANM2 生命周期，不是 Player 路径 ring；也没有发现任何 Player waypoint writer/owner 合同。
- 通用 helper `0x4E4710` 将 `this+0xC18..+0xC28` 按 NPC ring 的 proxy/map/capacity/offset/count 读取，计数在 `+0xC28`。把真实 Player 传入会把其 ANM2 overlay flag、animation data pointer/count、layer state pointer/layer count 错当 deque header/point count。当前资料无法证明这种 reinterpretation 在 Deathmatch 分支中有合法含义，不能读取、构造或写入这段 Player 内存，也不能用 `npc835_path_empty` 代替真实 owner。

可实施范围限于当前普通 C/S draw：Source 保留 child 的稳定 Player Key 关系，不附加 Player 路径 bool/count/points；native preflight/draw 只读验证 `Game+0x25B8C<2`，不改写该 Deathmatch 对象，也不执行其状态转换。在此普通 scope 下 Type 1 helper 条件为假，Type 10–999 的真实 NPC path 按 NPC C18 合同绘制。若将来 Client 要覆盖 Deathmatch，仍缺少该 type 1 分支为何调用 ring helper、helper 预期数据 owner以及合法当前点列的静态证据；届时应明确拒绝该形态直到取得这些证据，不可伪造为空、把 Player 按 NPC ring 安装，或手动调用 native helper。没有定位到一条有真实 Player path owner 的窄纯绘制替代路径。

## Effect136 与 `LiftedGridTarget`（承接 I18）

2026-11-04 I34补证：136本体Visible=false时由Player持物Render消费；只安装desc／主Sprite不等于举持已显示。必要依赖为当前GetHeldEntity稳定Key／shape、GetItemState、GetSuplexState以及独立GetBodySprite完整pose／null-frame。Player+44C是BodySprite+41C内的AnimationState+30，不是HeldSprite+FF8；Source真实LiftItem frame2已提供该记录。正式资源apply及Render栈借用、原生暂时置可见并回调Entity Render的具体指令、Nullable Body／原生Suplex状态5、运行失败与最终覆盖见[统一验收说明](tx-04-19-acceptance.md)。它补齐19所需的普通Isaac窄支撑，完整Player仍归20。

本地 Wiki 与 REPENTOGON `LuaEntityEffect.cpp::Lua_GetGridEntityDesc` 确认：仅Effect Variant136把 `_varData`（Effect `+0x420`）暴露为 `GridEntityDesc` view。官方 `GridEntityDesc.zhl` 布局总长 `0x20`，含 Type `+0` i32、Variant `+4` i32、State `+8` i32、SpawnCount `+C` i32、SpawnSeed `+10` u32、VarData `+14` i32、Initialized byte `+18`（`+19..+1B` padding）、VariableSeed `+1C` u32。注意这里16B的是Effect通用 `_varData` 起始区域/Init清除长度，不是GridEntityDesc的VarData字段。Effect Init `0x5CD070` 只清`+0x420..+0x42F`的16B；Render `0x600E60`不消费该desc；不因公开 GetGridEntityDesc 就认为Room仍有active Grid owner。

真实 creator `Room::PickupGridEntity VA 0x81AE00` 先用`0x81A920`取desc及 GridEntity内嵌ANM2副本，然后调RNG、`0x424340` Spawn Type1000/Variant136、以`0x403160 SwapANM2`把完整ANM2交换到Effect主Sprite `+0x48`、写全部32B descriptor及 Room State。`0x81A920`还可能移除/替换Room grid、创建Decoration，且会对命中的关联Entity写状态并调用Update；因此不调用creator/helper作为无副作用 getter，也不重新构造源动画文件名。

同I18 `LiftedGridTarget`/`ClientGrids::VisitLifted`：I24按同一Use Effect Key关联已捕获 lift payload，完整安装32B desc中的八个有名字段（不复制padding）、Effect.State、真实最终主ANM resource/pose；不创建active Grid，不建立第二个Key。当前 `source_grid` 无官方 getter给原始 Grid key，仍为 `UNKNOWN`；不得从格点位置/desc推造身份。Lifted Source资源缺失或desc不完整时拒绝该Use，不回调Lift链补值。

## 具名未决项

- 尚未找到原版 Dummy 的合法创建 caller；已定位专用预构造池和新 Client 扫描策略，但未证明原版业务创建语义，也未runtime验证真实池代次/使用窗口。
- 尚未找到所有 Text Entity 的原生 producer。`0x6FF270`只证实一类 writer；未证明所有文本的字符串语义、寿命和业务写入者。
- Deathmatch 状态下 Type835 的 `Type==1` child 会被送入按 NPC C18 ring 读取的 helper，但真实 Player `+0xC18..+0xC28` 是 ANM2 成员。该分支的预期 owner/意图与合法点列来源仍 `UNKNOWN`；普通 C/S scope 通过只读状态 `<2` 排除此分支。未作运行时验证。
- NPC951 Variant3 的 triplet loop 使用前后邻点；任意count不一定意味着`[i+1]/[i+2]`都在正式长度内。Source若不能证明每次消费者访问有界，就失败而非复制超界/旧map残项。全类别动态分布需后续Runtime/E2E确认。
- Effect136源Grid key (`source_grid`) 未有官方getter；Source描述和ANM有独立依据，identity关系仍缺。
- PostRender Room外层调用顺序静态已定位；实际Client拼接时的次数、所有水面/列尾路径与原生画面等价性仍归I34运行验收。
- `NativeDummy`/Text/复合NPC的runtime可达性、所有ANM pose准确性、所有Layer/Post/Light的像素结果、draw异常和raw队列消费顺序均未通过本研究验证；不得写成运行时PASS。

## 主智能体采用审阅

2026-11-03：主智能体先读本地两份Wiki及官方Lua/ZHL，再独立复读`6620E0`、`5CC580`、`6A3630`、`6EEE60/6A3940`、`6CEC90`、`61E360`、`647310`、`6AE430`、`574930`、`415350`、`6AC660/956F80`与实际vtable；逐条raw核`4155E1..E9`的Shadow ABI和`80619A..8061E8`的Post次序。采用结论以上文主审更正为准：+172是Exists；LaserShadow不据IDA猜测增加EDX参数；Pickup的wobble与Bomb lazy重建分开；921消费C04，不对其未消费的C18额外限制；GridDesc.VarData是i32；NPC child缓存/变换与Player持续色完整保护。

本次补核 835：Luna 研究与主审分别复读 `55546F..55548E` 的 Deathmatch状态条件、`0x4257A0/0x4249D0/0x425180` 的 DeathmatchType 状态机、Player ctor `0x74DCF5..0x74DD07` 的七个 ANM2 数组元素，以及官方 `ANM2.zhl` 的 layerCount 布局，结论相同。初稿“无 Type==1 早退”与 `npc835_path_empty` 均保留为已识别的初次误读历史，其结论已按上节纠正。主智能体采用上述更正并静态审阅Source／纯值预检／原生预检／整帧绘制的对应改动。这里全是源代码/目标二进制静态证据，没有运行游戏、构建、测试或 runtime PASS。

主审另核官方`Patches/EntityPlus.cpp`：holder由Entity ctor创建，base Init重置Plus；`Patches/EntityManager.cpp`的Factory Create正常通知New，而Dummy自有acquire没有经过该入口。实现因此在同一官方main_ex绑定窗口同时保留New／Delete闭包，Dummy取得后补一次官方ESSM New，退休继续原Delete；闭包只维护ESSM s_validEntities／自有数据，不运行第三方ModCallback。
