# tx-04-19 I10–I14 原生字段与类别适配静态核对

状态：`J273_STATIC`。本文只记录静态资料、官方 ZHL/Source 与目标二进制的只读核对；没有运行游戏、构建或测试，也没有修改 IDB。目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA ImageBase=`0x400000`；下表 RVA 均为 `VA - 0x400000`。

## 来源与读法

- 公开成员名称和语义先按本地原版 Wiki `F:\projects_isaac\IsaacDocs\docs\EntityNPC.md`、`EntityTear.md`、`EntityProjectile.md`、`EntityBomb.md`、`Entity.md`；再按 REPENTOGON Wiki `F:\projects_isaac\REPENTOGON-1.1.3\docs\docs`。
- 官方代码与布局核对使用 REPENTOGON `LuaInterfaces/Entities/LuaEntityNPC.cpp`、相应 Entity Lua 接口，以及 `libzhl/functions/Entity*.zhl`。ZHL 结构未命名的字段只有在 IDA 属性注册、getter/setter 实际读写都吻合时才列为已核。
- IDA `sub_84D340` 是原生 Lua 类型/成员注册器。表中列出注册附近原始 VA 和字段访问器；`push callback(s), push name` 的顺序由 `Entity.SpriteScale`、`EntityNPC.Scale/IsChampion`、Projectile 属性等已可核成员交叉校准。所有偏移均相对该类别对象开头。
- 反编译地址是静态入口证据，不等价于运行时观察。`UNKNOWN` 表示当前证据不能承诺对应行为或类别覆盖范围。

## NPC（I11）

| 公开值 | 实际存储 | 公开注册 / 直接读写证据 | 结论 |
| --- | --- | --- | --- |
| `StateFrame` | `+0x410`, i32 | NPC 注册 `0x8516F1` push `0x410`；ZHL `EntityNPC.zhl` | 采用 |
| `State` | `+0xB64`, i32 | `0x8516D9` push `0xB64`；ZHL | 采用 |
| `ProjectileCooldown` / `ProjectileDelay` | `+0xBA0` / `+0xBA4`, i32 | `0x8516CD` / `0x8516C1` 对应 push 偏移；ZHL | 采用；两者不能合并 |
| `V1` / `V2` | `+0xBB0` / `+0xBB8`, Vector8 | `0x8516B5` / `0x8516A9`；ZHL `Vector` | 采用 |
| `I1` / `I2` | `+0xBC0` / `+0xBC4`, i32 | `0x85169D` / `0x851691`；ZHL | 采用 |
| `GroupIdx` | `+0xC7C`, i32 | `0x851683 push 1; 0x851685 push 0xC7C; 0x85168A "GroupIdx"` | 采用 |
| NPC `Scale` | `+0xC38`, f32 | 注册 `0x85178C–0x851796`; getter `0x4525C0` 读 `this[782]`; setter `0x6D3040` 写 `this[782]` | 采用。setter另按新 scale 更新通用 `SpriteScale` 两个 float；adapter 应以 Source 两个公开量各自的最终值覆写，不能由一方推导另一方 |
| 通用 `SpriteScale` | `+0xE4`, Vector8 | Entity 注册 `0x84F5C4–0x84F5CE`; getter `0x412CC0` 返回 `this+228`；setter `0x429DA0` 写 `[this+0xE4]` 与 `[this+0xE8]` | 采用。和 NPC `Scale +0xC38` 是不同公开值 |
| `IsChampion` | `+0xC63`, u8 | `0x851782` 注册 `sub_6B4410`; `0x6B4410` 读 `this[3171]` | 采用 |
| `GetChampionColorIdx()` | 回调 `0x6A38E0` 读 `this[793] = +0xC64`, i32 | NPC 注册原始相邻项 `0x8517B9 push sub_6A38E0; 0x8517BE "GetChampionColorIdx"`; 本地 Wiki 是无参函数 | 采用此独立 getter 值；不以 `_bosscoloridx` 替代 |
| `GetBossColorIdx()` | 回调 `0x7DF770` 读 `this[798] = +0xC78`, i32 | 注册 `0x8517C3 push sub_7DF770; 0x8517C8 "GetBossColorIdx"`; ZHL `_bosscoloridx +0xC78`; 官方 Lua 源中相关 `_bosscoloridx` 注释 | 采用；它与 `GetChampionColorIdx` 是两个不同绑定和值 |

NPC 属性区 RVA 为 `0x451661` 起。注意绑定字面次序是关键：反编译函数 `0x6BA970` 是下一项 `CalcTargetPosition`，包含随机数、碰撞/寻路及目标缓存写入，不能当作 color-index getter 或用于适配查询。`GetChampionColorIdx` 的注册回调为 `0x6A38E0`（返回 `+0xC64`）；`GetBossColorIdx` 才是 `0x7DF770`（返回 `+0xC78`）。禁止把后者的 ZHL 名称外推为前者。

NPC 组合绘制不是单主 Sprite。IDA 在 NPC ctor `0x6B4520`（RVA `0x2B4520`）可见初始化内嵌 ANM owner：`sub_402B00(this+822*4)` 与 `sub_402B00(this+891*4)`，即 `+0xCD8` 与 `+0xDEC`；NPC Render `0x6CEC90`（RVA `0x2CEC90`）读取这两个 ANM 状态。它们必须随同一来源 NPC 的真实资源/姿态提交；拿 `Entity.GetSprite()` 单独渲染不构成 NPC 完整画面。Render 还按具体类型读取 minecart/EntityRef 及特殊帮助函数；未闭合分支不能借用“NPC通用支持”结论。`Delirium` 私有状态另由 ZHL 定义于 `+0xBC8..+0xBE0`，不能把此状态区误读成颜色字段。

NPC Render 的附属依赖按可见代码分支收窄，不能把私有成员容器头或地址当作可跨进程复制的资源：

| Render依赖 | 静态位置与读取 | 同步/绘制边界 |
| --- | --- | --- |
| 双内嵌 ANM owner | ctor `0x6B4520` 构造 `+0xCD8/+0xDEC`; Render `0x6CEC90`读取 | Source 必须分别提供这两个 owner 的真实ANM和最终姿态，尤其 Delirium；单独复制主 Sprite 不闭合其复合画面。仅看到双owner不能据此放行全部NPC/Delirium变体 |
| Type 835/906/920 特殊layer | helper `0x555410/0x4B52A0/0x4B6680`; 访问 `+0xBF4/+0xBF8/+0xBFC` 私有容器及 `+0xC00`状态/计数；后两者还取主sprite layer owner `+0x78` | 容器节点与元素布局未确认；只允许 I25 对具名来源类型提供显式资源/姿态，禁止复制容器头/指针 |
| Type 921 特殊变换 | helper `0x574930`取 `+0xC04`指针、`+0xC10/+0xC14`范围/尺寸并改变主ANM可见性/旋转 | 成员/节点布局未闭合，缺Source最终值时保持 unsupported |
| Type 865/909:10/912 | `0x4DAB70`读取 `+0x3C4` EntityRef；`0x5891F0`读取 `+0x3BC`及state；`0x56EF50`读取 `+0x3BC` owner颜色并改主ANM | 是明确关联owner/状态的分支；必须解析Source owner和可见/颜色姿态，不可按通用NPC静态图层略过 |
| Type 245/411/816关联Entity | Render `0x6CF743–0x6CF790`通过 `+0xBEC`改关联Entity的Visible并调用其Render；`0x6CFCCE`附近Type 411/816还暂时改关联位置、SpriteScale，主 ANM `Item`需空 | `+0xBEC`是附属实体依赖；安装其Source资源/姿态并以正式空Null pose表达 `Item`，否则不具备该分支绘制资格 |
| OverlayEffect | Render `0x6CFF58`读取 `+0xDE1`、`+0xD0C`、`+0xD1C`; 还依赖主ANM的`OverlayEffect`空Null状态和render cache `+0xD74/+0xD78` | Source需同时提供被加载的overlay资源及上述姿态/标志；不运行NPC Render来生成缓存 |
| Type 965:10 passenger | `0x6CF8A1–0x6CFFAA`按PlayerManager关系（player `+0x3D4 == NPC`）遍历乘员，读取`+0x207C/+0x2080`偏移并暂时改可见性/调用Player Render | 这是额外乘员draw pass，I25需按明确owner单独处理，不可再次调用整段NPC Render |
| Type 951:3曲线绘制 | helper `0x598B10`仅当`this[778]`（`+0xC28`）>=9时进入；访问私有ring/map `+0xC1C/+0xC20/+0xC24/+0xC28`及全局曲线/顶点缓存 | 读到其按点数生成曲线顶点并提交绘制；容器节点语义未闭合，不支持仅同步这些字段即重现。I25应按具名type/Source曲线数据实现或保持unsupported |

以上附属分支是 I25 的专用 draw allowlist 输入，不是 I11 通用类别能力。I11 安装前应对未完成该专用接线的具名分支（Type 835/906/920/921、951:3、965:10，以及表内其他未闭合分支）先行拒绝，不能先写一半对象再因 draw 资源缺失而失败。对 Delirium 尤其要保持 NPC 实例的两份内嵌 ANM owner分别从 Source 正式资源取姿态；若其具体形态触发未列明的特殊helper/owner，该形态仍是 `UNKNOWN`，不能仅因基础字段及双 ANM 可安装就宣称全形态支持。

动态生命周期：NPC Init `0x6B4A50`（RVA `0x2B4A50`）先清理/替换已有附属关系，再调 base Init、pathfinder/boss bookkeeping 和 PostNPCInit helper。直接函数体未见一个可普遍放行的 Spawn 调用；PostNPCInit/Lua、配置及间接 helper 不是“无副作用”承诺。NPC Remove `0x6B4E30`（RVA `0x2B4E30`）对 `Type=866, Variant=0` 有特例：读取 `+0x3C0` 子体并调用其 virtual Remove（调用点 `0x6B4E4E`），随后才 base Remove `0x6A8000`（`0x6B4E53`）。adapter 只可对已核 Type/Variant 安装；未被具体 Render/resource/child 分支覆盖的 NPC 保持 unsupported/UNKNOWN。

NPC派生清理补充：Init `0x6B4A50`在调用base前处理旧 `+0xF50/+0xF54/+0xF58`关系（访问点`0x6B4B52–0x6B4C00`）；其中 `+0xF50/+0xF58`若有关联对象会调其virtual Remove并从其 `+0x3CC`表移除本字段地址，`+0xF54`只执行关联表移除。派生ClearReferences `0x6B4E60`先在`0x6B507B–0x6B50ED`分别移除 `+0xF50/+0xF54/+0xF58`反向项并将本地字段清零，然后于`0x6B517B`调用通用 `0x6A8060`；语义名未由ZHL公开。同步侧可使用本地解析出的稳定Source key调用官方 `SetReference`建立关系，retire时先撤边/调用该派生清理；不要复制Source指针或只清本地dword。

## Projectile（I13）

原生 Lua 注册区从 `0x8514A1`（RVA `0x4514A1`）起；如下偏移由注册 getter/setter 与直接访存交叉核对：

| 公开值 | 存储 / 宽度 | 关键证据 |
| --- | --- | --- |
| `Height` | `+0x410`, f32 | 注册序列 `0x8515BF push 0x44BE70`（setter）、`0x8515C4 push 0x42D2C0`（getter）、`0x8515C9 "Height"`; callbacks读写该float |
| `FallingSpeed` / `FallingAccel` | `+0x414` / `+0x418`, f32 | 注册 `0x8515A6–0x8515B0`、`0x85158D–0x851597`; `0x4402F0/0x452270`、`0x440310/0x4314E0` |
| `Damage` | `+0x41C`, f32 | `0x844960/0x844980` |
| `Scale` | `+0x420`, f32 | `0x851501–0x85150B`; `0x44BE90` 写 `this[264]`、`0x555730` 读 |
| `HomingStrength` / `CurvingStrength` / `Acceleration` | `+0x424` / `+0x428` / `+0x42C`, f32 | 注册 `0x851556–0x85157E`; callback 分别 `0x565260/0x6069B0`, `0x4A3C40/0x4CCD80`, `0x4A3C60/0x844920` |
| `ProjectileFlags` | `+0x438`, u64 | `0x8514DE–0x8514E8`; getter `0x413170` 读 qword `this[135]`; setter `0x442D70` 写 `this[270]`、`[271]` 两个 dword |
| `ChangeFlags` | `+0x450`, u64 | `0x85152E–0x851538`; getter `0x844930`; setter `0x4A3C80` 写相邻两个 dword |
| `ChangeVelocity` / `ChangeTimeout` | `+0x458` f32 / `+0x45C` i32 | 注册 `0x851510–0x851529`; callbacks `0x4A3CA0/0x6069C0`、`0x4A3CC0/0x4B06A0`；timeout访问 `this[279]` |
| `WiggleFrameOffset` | `+0x448`, i32 | 注册 `0x851547–0x851551`; `0x440330/0x4CCD90` 访问 `this[274]` |
| `DepthOffset` | `+0x358`, f32 | 注册 `0x8514CF push 0x431380`、`0x8514D4 push 0x4313A0`、`0x8514D9 "DepthOffset"`; callbacks读写该float |

`ProjectileFlags`、`ChangeFlags` 是独立的 64 位字段，不是 Tear `TearFlags` 的 128 位数据。`AddProjectileFlags`、`AddChangeFlags`、`AddHeight`、`AddFalling*` 是修改操作，不是最终值 getter/安装字段。

Projectile Init `0x64F4B0`（RVA `0x24F4B0`）调 base Init、按 entity config load 主 ANM，并在 `0x64F4F9` 消耗 global RNG；Variant 2 有特定 layer/房间分支，spawner flags 还可修改实体缩放。其直接函数体未见 Spawn/PlaySound 调用，但 common post-init helper 与其间接 Lua/配置路径仍需用途隔离。对适配构造可保留结构，再覆盖 Source 最终值；不能把 RNG 初值或 Init 变体路径当成同步结果。

## Tear（I12）

Tear 公开字段注册区在 `0x84FA3A` 起（RVA `0x44FA3A`）。类布局与类型优先核 REP ZHL `EntityTear.zhl`；注册和具体访问器用于确认公开名/额外指针：

| 公开值 | 存储 / 宽度 | 关键证据 |
| --- | --- | --- |
| `Height` | `+0x410`, f32 | `0x84FB55–0x84FB5F`; getter `0x42D2C0`; setter `0x672A60` |
| `FallingSpeed` / `FallingAcceleration` | `+0x414` / `+0x418`, f32 | `0x84FB46–0x84FB50`、`0x84FB37–0x84FB41`; callbacks与Projectile共用直接 float getter/setter |
| `HomingFriction` | `+0x41C`, f32 | 注册 descriptor `0x84FAB8: push 0x41C; 0x84FABF: "HomingFriction"`; ZHL |
| `Scale` / `BaseScale` | `+0x420` / `+0x424`, f32 | `Scale`注册 `0x84FB28–0x84FB32`; getter `0x555730`; `BaseScale` getter `0x6069B0` 读 `this[265]`; ZHL |
| `TearFlags` | `+0x428`, 16 bytes | 注册 `0x84FB19–0x84FB23`; getter `0x413100` 从 `this+1064` 拷16字节；ZHL `BitSet128` |
| `WaitFrames` | `+0x438`, i32 | descriptor `0x84FAA2: 0x438; 0x84FAA9: "WaitFrames"`; ZHL |
| `Rotation` | `+0x458`, f32 | 注册 `0x84FB00–0x84FB0A`; getter `0x6069C0` 读 `this[278]`，setter `0x4A3CA0` 写同一字段 |
| `KnockbackMultiplier` | `+0x460`, f32 | descriptor `0x84FA80–0x84FA87`; ZHL |
| `StickTarget` | `+0x478`, Entity*（32-bit） | descriptor `0x84FA74: push 1; 0x84FA76: push 0x478; 0x84FA7B: "StickTarget"`; generic EntityPtr getter `0x8A4980` 读 `entity + storedOffset`，setter `0x8A4A20` 写该 dword。写入值必须是已解析的本地目标Entity，不可复制Source裸指针 |
| `ParentOffset` | `+0x7F4`, Vector8 | descriptor `0x84FA3A–0x84FA41` 明确 push `0x7F4` 与名称 |

相邻 sticky 公共量：`StickDiff +0x47C`（descriptor `0x84FA68–0x84FA6F`），`StickTimer +0x484`（`0x84FA5C–0x84FA63`），`CanTriggerStreakEnd +0x7E8`（`0x84FA50–0x84FA57`）。前两项不属于 `StickTarget` 指针本身。

`StickTarget` generic Lua setter 并非原生关系维护器：`0x8A4A20`最终只将值写到 `entity+offset`（`0x8A4AA6`），不会向目标实体 `+0x3CC` 反向 observer/ref表登记。内部 helper `0x6B17C0`才会先从旧target `+0x3CC`移除，再赋值并向新target登记。Tear `ClearReferences 0x663570`（`0x6635CE–0x6635F8`）会尝试从 target `+0x3CC`移除 `&tear+0x478` 后清空此指针；`0x4C8EE0`对不存在的map key可无操作。因此 native 直接安装非空 raw pointer 只适用于适配器确实在目标/tear退休前撤销依赖边的生命周期策略；切换非空旧target时不可假设裸写会维护旧反向关系。

Tear `ClearReferences 0x663570`还真实释放/撤销 `+0x488/+0x48C/+0x490/+0x494/+0x498/+0x49C`（32-bit EntityRef/owner slots；本专题不赋予额外公开语义）：`+0x488/+0x490/+0x49C`存在时调用对象vtable `+0x28` Remove（`0x66358C/0x6635A1/0x6636FC`）；`+0x478/+0x488/+0x48C/+0x490/+0x494/+0x498`通过目标 `+0x3CC`删掉本字段地址并清空（`0x6635CE–0x6636EA`）。`+0x494/+0x498`在Remove前还针对类型值2调用 `0x67A3F0`。这些是有反向关系/可能触发级联Remove的原生引用字段，应由正式稳定Source key走官方 `SetReference`，并在retire前显式撤边；不能仅按无语义raw ptr安装。

不要调用 Tear 公共 setter 作为纯内存写入：`Scale` setter `0x672BC0` 对值先 `fmaxf(value, 0.01)`，再写 `+0x424` 与 `+0x420`，并调用 `ResetSpriteScale 0x674190`；`Height` setter `0x672A60` 除写 `+0x410` 外还重算公共高度缓存 `+0x350` 并按 room type 特判；`TearFlags` setter `0x672BF0` 会根据 flags 载入/改写多个辅助 ANM 和 layer。安装 Source 完整结果时，应写核定字段并使用正式资源姿态，不借 setter 重演游戏效果。

Tear Init `0x662F50`（RVA `0x262F50`）调 base Init、从 EntityConfig 加载主 ANM，`0x662FC3` 调 global RNG，后续可写 tear 轨迹/flags 等临时初值。Variant `37` 分支还按 config 读取/装载 `gfx/promo/gfuel/bullet tear.anm2` 或 `gfx/promo/gfuel/missile.anm2`；这属于明确的特殊资源路径，未匹配 Source 正式资源时不可仅因 Init 返回就发布。直接函数体未见 Spawn 或播放 SFX；common post-init/资源回调路径没有被证明无玩法副作用。

Tear Render `0x675AC0`（RVA `0x275AC0`）首先检查 `Exists +0x172` 与 `Visible +0x171`（IDA 伪码十进制 `370/369`）。Render 把地面/offset 算得的坐标写入 Entity render cache：VA `0x675C0B` 写 `this+420`，`0x675C0F` 写 `this+424`；这里是十进制偏移，实际公共地址分别为 `+0x1A4/+0x1A8`，不是 Tear `Scale +0x420/+0x424`。随后以临时 ColorMod/Sprite 状态调用 common Render `0x6AC4E0`，再恢复原值，并走 `0x849210` post-render helper。原生最终 draw 若消费这些坐标/颜色缓存，需用来源同窗口读数或已核定的正式计算提供；不能跑 Update/Render 来填缓存。

## Bomb（I14）

Bomb 属性注册位于 `0x851922` 起（RVA `0x451922`）。`EntityBomb.zhl` 是常规成员布局来源；注册访问器确认这些公开值：

| 公开值 | 存储 / 宽度 | 关键证据 |
| --- | --- | --- |
| `Flags` | `+0x438`, 16 bytes BitSet128 | 注册 `0x851975–0x85197F`; getter `0x5CCB20` 拷16 bytes; setter `0x6A1E40` 写16 bytes |
| `IsFetus` | `+0x448`, u8 | 注册 `0x851966–0x851970`; `0x42D230/0x606AB0` 读写 byte `this[1096]` |
| `ExplosionDamage` | `+0x418`, f32 | 注册 `0x851957–0x851961`; getter/setter `0x4314E0/0x440310` 读写 `this[262]` |
| `RadiusMultiplier` | `+0x44C`, f32 | 注册 `0x85193E–0x851948`; getter/setter `0x606AC0/0x431500` 读写 `this[275]` |
| `Countdown` storage known | `+0x410`, i32 `_explosionCountdown1`; extra countdown writes | ZHL；`SetExplosionCountdown` `0x44EAC0` 还写 `+0x414`。`+0x414` 的公开/持久语义不由本专题确定，不能把写入覆盖外推为完整双计时模型 |

Flags setter 的第二个可见副作用是 `0x6A1E47` 置 `byte[this+1123] = 1`，即 Bomb `_loadCostumes +0x463`（ZHL）。Bomb Render `0x6A0F90`（RVA `0x2A0F90`）在 `0x6A0FAE` 检查 Exists/Visible，`0x6A0FBB` 检查该一字节；非零即在 `0x6A0FC4` 调 `0x6A2420`（RVA `0x2A2420`）懒重建。helper `0x6A2469` 取 costumes 数组 `+0x464`，`0x6A246F` 写 `word +0x462=0`，`0x6A2483–0x6A249C` 遍历5项、步幅 `0x114` 并对已加载 ANM 调 Reset；其后进入 EntityConfig 查找、资源 Load/LoadGraphics 与 layer 状态组装。另一个独立byte `+0x462`在 Render `0x6A10F2`被检查，控制随后五个costume owner绘制分支；它不是`_loadCostumes` pending位，adapter须保留/覆写Source该启用状态。故懒重建触发准确为 byte `+0x463`，不能把`+0x462`与pending混用，也不能把它解释成通用 Bomb flags 位。适配应完成五个独立 costume owner 的正式姿态，再于 Bomb Render scope 阻断触发重建的窄路径；不要调用 Bomb Flags setter 来安装最终bitset。

Bomb Init `0x69D6C0`（RVA `0x29D6C0`）先调 base Init。对 Variant 3/4/8/18，它在 `0x69D727` 分支读取/推进自身 RNG (`+0x3DC..+0x3E8`)，在 `0x69D778` 写新 seed 并在 `0x69D78C` 将 `seed % 0x1E` 加入 countdown `+0x410`。Variant 15 直接设 countdown=94；Variant 17/20设为87。Variant 2/3/4/8/17/18 会选择 Idle/Appear/Pulse 动画；Variant 4、Subtype 1 还遍历 Bomb sprite layers，为 `"Hole"` null layer 组装数据（`0x69D96B–0x69DA99`），不是证据支持的独立 Spawn。19/20 改 common entity flags/height。直接函数体未见 Spawn/Sfx 调用；但 RNG、config、ANM、post-init `0x848500` 和间接回调仍须窄 scope，客户端结果最终取自 Source。

## 公共初始化与退休边界

| 入口 | 直接观察 | 对 adapter 的约束 |
| --- | --- | --- |
| Base Init `0x6A4A20` (RVA `0x2A4A20`) | `0x6A4A35–0x6A4A47` 初始化 type/variant/subtype/seed类值；`0x6A4AD6` 调 vtable slot `+0x64`（LoadEntityConfig 路径）；随后初始化 common state / refs | 是动态对象的结构入口，不代表无资源/配置副作用。可以在已核池use与用途 scope 内调用，再覆盖 Source 最终值；不得直接构造不完整 base 字段取代它 |
| Base Remove `0x6A8000` (RVA `0x2A8000`) | `0x6A800C` 清 Exists `+0x172`; dead byte `+0x173` 置位；调 `0x84CA50`，并对 Lua bridge ref `+0x408` unref/free | `Projectile/Tear/Bomb` ZHL vtable跳过派生 Remove，基类Remove是其正常Remove路径；仍须先退资源/引用/绘制借用，且阻止玩法Lua回调 |
| Base ClearReferences `0x6A8060` (RVA `0x2A8060`) | 清多个 EntityRef；若 `+0x310` 非空则 `0x6A8167` 调其 virtual slot `+0x28` Remove，再释放该 ref；遍历 `+0x3CC` observer list清反向关系；若 valid byte `+0x170`，`0x6A81E6/EF/F8`刷新位置等缓存并在 `0x6A81FE` 调 ANM Reset | 不是“只清指针”。对 `+0x310` 附属owner须先分类；当前没有理由全局写空绕过原清理。NPC 866/0额外派生路径另见上表 |

类别派生 Init 的直接函数体均先调 `0x6A4A20`。Projectile/Tear/Bomb 的已见直接体内未发现显式 `Spawn` 或 `PlaySound` 调用；NPC/Bomb/Tear/Projectile 都存在后置 helper、config/ANM或其他间接调用，因此该局部观察不等价于“无副作用 Init”。Projectile/Tear/Bomb 已定位的 global RNG 消耗只允许在类别Init用途 scope内替换为Source seed，再由Source完整后态覆写；不得全局压制RNG。必要 Hook 应只拦截已定位的客户端用途/分支，保持宿主及其它用途原调用；不在完整 Init 前缀后把未知类别或特殊分支默认为可用。

## 未闭合项 / 明确限制

- NPC `+0xCD8/+0xDEC` 已确认是 Render读取的双内嵌 ANM owner，但带额外 Minecart/EntityRef/专用helper 的 Type/Variant 资源与完整依赖仍需逐项 allowlist；不声称任意NPC已闭合。Delirium 公开状态区与颜色字段独立。
- NPC `GetChampionColorIdx` 和 `GetBossColorIdx` 注册名必须严格分别对应 `+0xC64` 与 `+0xC78`；不要用字段历史名或单个返回值替换另一公开接口。
- Bomb `+0x414` 被 `SetExplosionCountdown` 共同写入，但 ZHL未把它标成该类别成员；双计时数据语义仍 `UNKNOWN`。其他未列的 Bomb variant 私有姿态/特殊行为也不由这四项字段证明。
- Tear `BaseScale` 注册表现为 getter（`0x6069B0`）而未见对应属性setter；Scale setter会同步写BaseScale并ResetSpriteScale。native完整快照要同时采用来源值，不借setter推导缺失值。
- NPC特殊Render helper、附属EntityRef/乘员pass以及Type 951私有曲线容器须按具名分支进入 I25 专用draw allowlist；布局或Source资源/姿态不明确的分支维持unsupported/`UNKNOWN`。Delirium 不因已核双内嵌 ANM 而自动覆盖全部形态。
- Tear `StickTarget` Lua setter是裸指针写入，没有反向关系登记；依赖图退出必须先撤边，不能依赖直接替换旧非空指针完成引用维护。
- 类别 Init 的完整转调链、所有Lua回调/第三方Mod回调及异常恢复不是静态读到直接调用列表即可全覆盖；若具体动态路径未解析，应限制到具名类别/用途或保持 `UNKNOWN`。

## I10 动画null层与主layer选择器补充

官方 `AnimationState.zhl` 中 `AdvanceNullLayerPosition(int nullLayerID, float targetFrame)`签名用作目标匹配；目标VA `0x4040B0`（RVA `0x40B0`）。同文件结构声明 `_animData +0x4`、`_nullLayerFrames +0xC`；`AnimationData.zhl`声明 null层表起点 `+0x20`（指针占位未命名）、`_nullLayerCount +0x24`。IDA 指令显示：`0x4040CE`校验 ID `< *(animData+0x24)`；`0x4040F6`按 `animData+0x20+16*id`取记录；`0x404104`用记录 `+0xC` 的 frame count限制前进；`0x404106`读记录 `+0x8` frame-array；`0x404115`比较 `(int*)(frameArray+84*selector+0x50)` 与 targetFrame；`0x40411A`将所选序号写入 `state+0xC` 的 `_nullLayerFrames[id]`。因此 Source 改过的 null selector 可独立安装，不要求等于 `SetFrame` 导出的选择器。每个有效ID只接受 `-1` sentinel或 `[0, record.frameCount-1]`；frameCount为0时只可用`-1`。

`AnimationState::GetNullFrameByID`签名对应 `0x4065B0`：按 `AnimationData+0x20`的16字节记录扫描ID（`+0`），只有 `_nullLayerFrames[id] >=0 && selector < record+0xC` 才返回 `record+0x8 + 84*selector`（`0x4065E7–0x4065FF`）。记录 `+4`尚未命名；帧步幅是`0x54`，但官方 `NullFrame.zhl`只声明可见字段并标 size `0x48`，边界读取 `+0x50`落在未命名尾部，不能把剩余布局假定为完整`NullFrame`。

主layer `-1`同样可安全表达“当前没有绘制帧”。`AnimationState::Render`匹配`0x405990`，按 `AnimationData+0x1C` layer count取 `_layerFrames[id]`（`0x4059C0`）并调用 `AnimationLayer::RenderFrame 0x404570`；其范围条件 `frameIndex >= 0 && frameIndex < numFrames`位于`0x4045DB`，空frame数组count=0也不会通过。`ANM2::RenderLayer 0x405A50`先对layer ID本身执行非负/owner上界检查（`0x405A79–0x405A86`及其他分支），再将选出的帧selector送给同一 `RenderFrame`范围检查。故允许主layer selector `-1`或`[0, numFrames-1]`，拒绝其他越界值；count=0时共同渲染路径不会解引用帧。正count必须配合有效frame-array；畸形的正count/空指针组合没有安全证据，不能放行。

## 主智能体审阅与采用修正

本轮必要逆向由一个GPT-6 Luna max子智能体执行。主智能体先读相应本地Wiki与官方源码／ZHL，再独立复读四类Init、NPC注册区、Projectile全部关键访问器、Tear sticky注册／清理、Bomb Flags/伤害/半径／完整Render、NPC／base清理和动画null／主layer范围指令。以下是最终采用事实，不是运行结果。

- NPC的名称必须与注册调用成对核对。中间曾把`7DF770`配成Champion getter、`6BA970`配成BossColor getter；独立读`8517B9—8517D2`后纠正为Champion=`6A38E0/+C64`、BossColor=`7DF770/+C78`，`6BA970`属于CalcTargetPosition。最终字段表采用这个配对，不用相邻名称推断。
- Projectile `Damage +41C`和`WiggleFrameOffset +448`均由注册与读写双向核定，替代早期对wiggle`+430`的猜测；两份flags各64位。Tear／Bomb的flags各128位，不能套布局。
- Tear／Bomb Render伪码`this+420/424`是十进制公共`+1A4/+1A8`；`369/370`是Visible/Exists `+171/+172`。不把这些数错记为派生`+420/+424`或`+369/+370`。
- `4065B0`只在`0<=selector<count`时解引用null帧；`4045D0—4045DB`也在主帧负值／超过count时直接跳过。空count时0和-1均为不解引用状态；代码保留原生空层0哨兵，拒绝其余越界值。源码的GetFrame同样检查非负和count。此项进一步限定了上文“空null层只可-1”的中间表述，不代表可读未命名尾部。
- 实际ClearReferences虚表槽为NPC `B18814→6B4E60`、Tear `B15FFC→663570`、Projectile `B15B60→6A8060`、Bomb `B1815C→6A8060`；Remove槽分别是NPC `6B4E30`，其余`6A8000`。ZHL的skip不是实际继承证明。
- 主审NPC `6B4E60`还确认`+F28`在`6B5129`虚Remove、`+F38`在`6B516E`撤登记；`6B514C→6D5AA0`处理`+BF0`私有引用容器。代码补固定关系槽Source，并在任何非空私有引用容器目标上明确不完整／拒绝，不按网络值写容器头／count。
- NPC `+F40`在`6B53D5`被虚ClearReferences，`6B53E6`又Free(flags=1)，且会遍历其Familiar关系；无法把SDK的`_sirenPlayerEntity`名字外推成普通Player借用。Source与Client遇到该owned actor均保持明确缺口，交I25／20，绝不把roster身体塞入拥有槽或写空未知owner。
- NPC912:0/10、903:10清理调用的`908140`是全局SoundEffects Stop。子智能体核原始入口`55 8B EC 53 56 8B 75 08 57 85 F6 0F 88 AD 00 00 00`，主审独立复读函数及两返回`908196/908202: C2 04 00`；采用ECX=this、一int栈参的自有薄定义。官方Lua Stop存在，C++ SDK/ZHL无同址声明。只在本方类别退休作用域主动跳过，原调用计数不伪造，Host／Off透传。

`TearFlags` bit127会使原生ClearReferences沿`+3C0`的Tear链Remove；代码先撤该边和六个内部槽，避免重复删除T中的独立实体。具体正常退休、资源候选与缺口边界见[实现说明](tx-04-19-class-application.md)。任意第三方Mod与未知异常闭包仍未证明，不能从本专题推导其可调用资格。
