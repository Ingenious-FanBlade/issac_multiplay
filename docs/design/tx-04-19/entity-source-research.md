# Entity I03 只读表现来源研究

本文只记录 I03 所需的只读来源与边界；不研究实体写回、主动构造、退役全链，也未运行游戏、探针或测试。逆向证据来自当前打开的 IDB，目标文件名为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，image base `0x400000`。工程既有核定目标为 Repentance+ `v1.9.7.12.J273`；本轮使用同一 IDB，新增核对的点位字节与既有 J273 线吻合。本轮没有重新鉴定可执行文件的构建号；文件名未带 build 后缀不改变工程既有目标事实。官方代码来源为本地 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions`，该工程 README 标注目标 `v1.9.7.12.J273`。

资料顺序按项目要求：先查 `F:/projects_isaac/IsaacDocs/docs` 原版 Wiki，再查 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs` Wiki，之后查本地官方 REPENTOGON 1.1.3 ZHL，最后读当前 IDB。Wiki 为公开 API 语义来源，ZHL/IDB 为内部布局与调用来源；不能把前者当作私有偏移证明。

## NativeText

### 只读对象来源

`Entity_Text` 当前 IDB 函数地址与 RVA：

| 用途 | VA | RVA | 证据 |
|---|---:|---:|---|
| 构造函数 | `0x5CC1F0` | `0x1CC1F0` | 设置 Text vtable，调用 `sub_9E35B0(this+0x518, 0xFFFFFFFF)` |
| Init | `0x5CC260` | `0x1CC260` | 调 Entity Init、清计数、设置寿命/部分颜色 |
| Update | `0x5CC340` | `0x1CC340` | variant 1/2 首次 Update 格式化字符串；计数与淡出更新 |
| Render | `0x5CC580` | `0x1CC580` | 使用 `+0x418` 做宽度测量及 Font 绘制 |

当前 IDB 的 Text 池在构造路径中为 0x80 个对象、步长 `0x530`（1328）字节。`Entity_Text` 基类/池复用使“地址相同”不能当作同一文本值代次；字符串见证应和主线现有 entity identity/use/revision 绑定。

| 字段 | 偏移 | 类型/尺寸 | 读值资格 |
|---|---:|---|---|
| Variant | `+0x02C` | `int32` | 在有效 Text owner 上读取；决定 Update/Render 分支 |
| Update counter | `+0x410` | `int32` | Init 写 0；Update 每次递增 |
| Lifetime | `+0x414` | `int32` | Init 按 variant 写入，Update/Render 动画使用 |
| Text buffer | `+0x418` | `char[256]` | 只有 writer 完成或 Render 实际消费时才能形成正式字符串来源；永不越过 256 字节 |
| Variant1/2 格式输入 | `+0x334` | `float` | 首次 Update 的 `%g`/`%d` 源值 |
| KColor | `+0x518` | 20 字节 | 构造时设白色，Init/SetColor/Update 会更新；保留完整 20 字节，`+0x528` 的 opaque32 语义 UNKNOWN |
| Render 坐标缓存 | `+0x1A4/+0x1A8` | `float,float` | Render 根据当前位置、偏移与屏幕参数写入，不是字符串或持久几何来源 |
| Base position / offset | `+0x33C/+0x340`, `+0x34C/+0x350` | `float` 分量 | 当前 Entity owner 的位置和偏移；Render 输入 |

构造函数本身没有写 `+0x418`，只初始化基类、vtable 和 `+0x518` KColor。Init 清 `+0x410`，按 variant 设 `+0x414` 和颜色，但没有清 `+0x418`。因此不能以“刚 Init”或“构造白色 KColor”推定 buffer 已清零；池复用会留下旧字节，必须以本代写入见证或真实 Render 消费为准。

### 字符串写入和 hook 资格

当前 IDB 中可证明的写入路径：

1. **Variant 0 的已知内部 helper**：`sub_6FF270`（RVA `0x2FF270`）调用 `sub_424340(..., type=9001, variant=0, ...)` 创建 Text，调用 vtable `+0x3C` 设置颜色；随后在 VA `0x6FF335` 调用 `sub_418B70`，目标是 `esi+0x418`、capacity `0x100`、format `"%s"`、来源是已格式化的 `ErrMsg`。`sub_418B70 -> sub_412A50 -> _stdio_common_vsprintf`。这里没有预置 `buffer[0]=0`，但这个有界格式化调用会覆盖 C-string 并写终止符；只在成功完成后才有本次写入资格。
2. helper 紧接着在 VA `0x6FF341` 以 `ECX=esi` 虚调 `vtable+0x0C`（`Entity_Text::Update`），下一指令 VA `0x6FF344` 是原生调用者返回地址。helper 返回 EAX 是 Update 的返回值，不是 Text 指针。静态 xref 仅有两个已见调用者：VA `0x6AAAF0`（`sub_6A9CF0`）与 `0x76D2BC`（`sub_76B900`）；它们构造伤害/受伤提示文字。该证据只覆盖此 helper 路径，不覆盖任意泛型 Spawn/外部 Lua 创建。
3. **Variant 1/2 的原生 Update writer**：`Entity_Text::Init` 把 `+0x410` 计数设为 0。Update 检查的是该计数，不是 `buffer[0]`：variant 1 在 `+0x410==0` 时于 VA `0x5CC420` 调有界 formatter，写 `+0x418`，capacity 256，格式 `%g`，数值来自 `+0x334`；variant 2 在 VA `0x5CC38E` 以 `%d` 格式化同一输入。此路径会直接覆写池里可能残留的字符串。wrapper 可在调用原 Update 前记录 `variant∈{1,2}` 与 counter 0，在原 Update 正常返回后、仍处于同一对象 use/revision 时，bounded-read NUL 结尾字符串并记 writer witness。
4. **Variant 0 未知创建者/其他 variant**：本次仅找到 helper 的 variant 0 写入路径；普通 Variant0 Update 不生成 buffer，variant≥3 的可用创建者也未证明。若它们进入 Render，Render 消费点可独立记录“当前实际被读取的 C-string”，但不能倒推未观察到的创建代次或 writer。

因此，TextUpdate wrapper 的 Variant0 writer witness 要求原生调用返回槽确认为 VA `0x6FF344`；Variant1/2 witness 要求 pre-counter=0 且原 Update 成功返回。HookSystem 1.1.3 的 `FunctionHook_private::EmitPrologue` 为 no-argument/non-void thiscall 生成保存 EBP 与 EDX、EBX、ECX、EDI、ESI 的 adapter；hook 函数内 `_ReturnAddress()` 是 adapter 内返回点。和 `mod/src/game/world_sources/global_random.cpp` 相同，要从 hook 的 `_AddressOfReturnAddress()+28` 取原生调用者返回槽，再校验 VA `0x6FF344`；不可直接比较 `_ReturnAddress()`。Update 的 ABI 是 `int __thiscall(Entity_Text*)`，无显式栈参数。

`Entity_Text::Render` 以 `this+0x172` 的有效标志通过后，先用 `Manager+0x4AB84` 的 Font 对 `this+0x418` 调用 GetStringWidth（max chars `-1`），再把同一个 C-string 传给 DrawString。Render hook 可以在该有效分支之前做 256 字节有界拷贝，形成单独 `ActualCall/Render` consumer-scope 来源；无 NUL 时标记截断/不完整，不得继续扫越界。它能证明本次 Render 实际读取的字节，但不替代 ordinary snapshot 的 writer/代次资格。

### 字体、布局和 phase

官方 `Manager.zhl` 定义：`Manager::_font6_PFTempestaSevenCondensed +0x4AB84`，`Manager::_framecount +0x4A8B8`。Render 把 Font6 传给宽度查询和两条绘制路径：variant2 使用 `sub_9E9680`，其它 variant 使用 `sub_9E9500`；两条路径都使用 Text `+0x518` KColor 与本地 `FontSettings`。当前 IDB 为 FontSettings 设置相同默认值：align 0、maxChars `-1`、maxLineWidth 0、wrap 0、lineHeightModifier 1.0、missingCharOverride `-1`、两个未知 dword 0。官方 `FontSettings.zhl` 大小 0x1C；`Font.zhl` 声明 DrawString 的 str/pos/scale/KColor/FontSettings 接口。

variant2 的 subframe 分支由 `Game::IsPaused()` 控制。IDB VA `0x6F92A0`（RVA `0x2F92A0`）bytes 为 `8B D1 56 8B 35 ...`，与官方 `Game.zhl` 中 `Game::IsPaused()` 签名完全一致；Render 在 VA `0x5CC6FD` 调用，返回地址 `0x5CC702`。paused=true 时本地 subframe 为 0，否则取 `(Manager._framecount + 1) % 2`，该值和 Update counter 参与 variant2 绘制 scale。Render 入口另有短暂的宽度查询与 `+0x1A4/+0x1A8` 缓存写入，静态路径没有 gameplay callback 或 Game/Manager phase 字段写入；在同一主线程内，Render wrapper 入口读取官方 Game::IsPaused 与 Manager +0x4A8B8，可记为本次 Render scope 的 phase inputs，不应并入 ordinary entity snapshot，也不应以 `Game._frameCount` 或 `AuthorityTick` 代替。

注意两个 frame 不是同一字段：`Game.zhl::_frameCount +0x25A70` 由 variant1 Update 用于相对 Entity Init 帧计算漂移；variant2 Render 的闪烁分支用的是 `Manager::_framecount +0x4A8B8`。

## Laser 当前绘制几何

原版 Wiki `EntityLaser` 的 `GetSamples()` 定义为仍能正确表达路径的最少点集（直线示例 2 点）；`GetNonOptimizedSamples()` 通常返回 51 点。原版 Wiki 将 `LaserLength`、`SampleLaser` 分别列为 float/boolean 属性。官方 REPENTOGON 1.1.3 ZHL 布局：`Entity_Laser::_sampleLaser bool +0x45D`，`_homingLaser HomingLaser +0x464`；`HomingLaser::_samples std_vector_vector2 +0x1C`、`_nonOptimizedSamples +0x28`、`_origin Vector +0x04`、`_endPoint Vector +0x0C`、Bezier vector `+0x48`。

当前 IDB image base `0x400000`。Entity_Laser vtable VA `0xB2D81C`（RVA `0x72D81C`），前置 RTTI locator 名为 `??_R4Entity_Laser@IsaacRepentancePlus@@6B@`；Init 为 vtable `+0x04`（VA `0x9480E0` / RVA `0x5480E0`），Update 为 `+0x0C`（VA `0x950B90` / RVA `0x550B90`），Render 为 `+0x14`（VA `0x9514B0` / RVA `0x5514B0`）。这组 vtable/RTTI 可作为 NativeLaser owner 的原生类型资格；地址复用仍须绑定主线 entity identity/use/revision。

普通 draw 的最小补充字段及读取资格：

| 字段 | 偏移 / 类型 | 正式来源与资格 |
|---|---|---|
| 样本模式原始值 | `+0x45D: int8_t` | Laser Init VA `0x94829D` 写 `WORD[+0x45C]=0xFF01`，故低字节 `+0x45C=1`、高字节 `+0x45D=0xFF`（signed -1，尚未解析）；Laser Update vtable 路径调用 `sub_948FF0`，后者按路径将 `+0x45D` 写为 0 或 1。Render 开头按**有符号** byte 检查 `<0` 并提前退出，随后以 0/非 0 选择普通/采样绘制。必须同步 raw `int8`；只有 `raw>=0` 时 mode 才通过 Render 的有效性门，不能把初始 -1 归一成公开 bool。 |
| LaserLength | `+0x460: float` | 原版 Wiki `EntityLaser.md` 将 `LaserLength` 列为 float。IDB Lua 属性注册函数 `sub_84D340` 在 VA `0x851A80` 推入字段偏移 `0x460`，紧随其后在 `0x851A85` 注册字符串 `LaserLength`，证明公开属性与私有偏移一一对应。Laser Init 清零；Update helper `sub_948FF0` 按 variant 设置 300 或按端点计算长度；Render 的非 sample 几何读取该 float。有效 Laser owner 初始化后可窄读；ordinary draw 若要复现最新几何应使用该 owner 当前字段，不用其它派生源替代。 |
| Sample vector | `+0x480 begin, +0x484 end, +0x488 capacity`，`Vector2[8]` | 由官方 ZHL 的 `HomingLaser +0x464` 与 `_samples std_vector_vector2 +0x1C` 得出。当前 Render 在 sample 分支遍历 begin/end，使用相邻点构建 quad strip，后段 overlay 再遍历相同 vector；每个点为 8 字节。窄读限定有效 Laser owner、`raw +0x45D == 1`、真实 Render/sample scope，检查 `begin<=end<=capacity`、区间长度按 8 字节整除并作配置上限；不能以 `GetNonOptimizedSamples()` 替代它。 |
| 起点偏移模式原值 | `+0x4F0: int32_t`（按 opaque32 保留） | Laser Init VA `0x948311` 明确写 DWORD 0。Render VA `0x952906` 对该 dword 做 signed `<=0` 分支判断；满足时用 `+0x350` 位置偏移修正绘制起点。语义名称仍 **UNKNOWN**，只按原始 32 位读值和该分支事实保存，不猜成公开属性或 LaserLength。由已完成 Init 的有效 owner 可窄读；当前查询没有找到归属于 Entity_Laser 的后续写者，工程中其它类型在同偏移的访问不得套到 Laser。 |

当前 IDB 的 Laser Render 在 vtable Render slot 中，还会读取基础 Entity/Sprite 状态和既有资源：`+0x151/+0x158` 是主线现有 Sprite loaded/bitflags 来源；`+0x328` 是原生帧字段，需保留与 Entity.FrameCount/当前 Game frame 的既有来源关系；`+0x350` 是位置偏移。它们不新增为本补充的私有 Laser 字段。本段补齐普通 draw 之前缺少的 sample mode、LaserLength、公开 samples 内容及 `+0x4F0` 原值，满足 0/1 sample 分支和起点分支所需输入。

**Samples 的公开来源和 backing store：**原版 Wiki 的公开 API `EntityLaser:GetSamples()` 返回描述激光路径的最少 `VectorList`。当前 IDB 的 Lua 属性注册函数 `sub_84D340` 在 VA `0x851B55`（RVA `0x451B55`）推入回调 `sub_956820`，随后于 VA `0x851B5A` 注册名字 `GetSamples`；回调 VA `0x956820`（RVA `0x556820`）只有 `lea eax,[ecx+0x480]; ret`，直接返回 `HomingLaser::_samples` 对象地址。对照 `GetNonOptimizedSamples` 的注册回调 `sub_956B40`，它同样是 leaf accessor，直接返回 `this+0x48C`。因此 I03 的正式语义来源是公开 `GetSamples()`；`+0x480` 起的 vector 是该公开结果的现有 backing store，不另立第二个私有 samples 来源，也不需额外遍历 `_nonOptimizedSamples`。

公开 getter 本身没有 lazy calculation。官方 ZHL 声明 `HomingLaser::CalculateSamples(float)`；当前 IDB 对应函数 VA `0x9768C0`（RVA `0x5768C0`）的调用位于 Laser 更新/插值路径（例如 VA `0x94D25A` / RVA `0x54D25A`、VA `0x955029` / RVA `0x555029`），而 `sub_956820` getter 不调用它。故读取 `GetSamples()` 只反映当时已存在的 vector；其完成状态跟随原生 Laser owner 生命周期与更新阶段，不可从“getter 被读过”推断其刚计算。窄读仍应以有效 Laser owner、raw `+0x45D==1` 和真实 Render/sample scope 为资格，并对该公开结果的 vector bounds 做有界校验。

`+0x48C` 对应 `_nonOptimizedSamples`，Laser Render 不直接读取；`HomingLaser::_origin`、`_endPoint` 和 Bezier vector 也不是该 Render 的直接样本来源。因此不把这些字段列入本轮普通 draw 来源。

## NativeDummy / NativeDarkness 命名

官方 `Global.zhl` Entity Type enum 有 `ENTITY_DUMMY=964` 与 `ENTITY_TEXT=9001`；IDB 有 `Entity_Dummy` RTTI/vtable，Dummy 构造/Update/Render 只证实 Entity 基类行为。`NativeDummy` 是工程对 `Entity_Dummy` RTTI/类型的分类名称，不是官方 API 名。代码层 `NativeDarkness` 仅残留于旧 `Capabilities` 声明；历史设计文档仍有该名称记录，但未找到对应 runtime source、consumer 或 RTTI。主线统一为 `NativeDummy` 是纠正 capability 声明；不把 `NativeDarkness` 立为 Entity_Dummy 的别名，也不据此虚构另一种 Darkness 实体。Darkness API/效果与 NativeDummy 分类不是同一证据域。

## 本轮不作结论的项目

没有扩展审计 NPC、Tear、Projectile、Knife 所有绘制分支的私有缓存；不把“未发现”写成“不需要”。NPC 的公开 Dirt/Champion/Boss 颜色 API 与 private render caches 是否有未覆盖的项目字段，应由对应目录/主审单独验证。`+0x528` KColor opaque32 的语义与 Text variant≥3 的可靠 writer 仍为 UNKNOWN。本轮未重新鉴定可执行文件构建号，既有 J273 目标核定保持不变。

## 主智能体审阅与采用决定

主智能体独立回读了 Text ctor／Init／Update／Render（VA `5CC1F0/5CC260/5CC340/5CC580`）、KColor ctor `9E35B0`、字体 wrapper `9E9680`、writer caller `6FF335—6FF344`、官方 KColor／Font／Manager 布局及 ZHL noarg 非void adapter。纠正了代理过程中的 counter／buffer 首字节判断混淆：最终代码只在已核定 writer 的真实调用正常返回、identity/use/revision 一致时保存资格；Render **不**为未知初始化历史补造 writer 见证，也不额外读无资格文本。

主审另读 Laser Render `9514B0` 的字段访问集合、Init `9480E0` 的三项写入，以及 Getter `956820/956B40`（分别只返回 `this+480/48C`）。通过 vtable 前置 COL 读取 TD，确认 Laser TD RVA `7D140C`，并将两项新增原值偏移统一登记在 `world_layout.json`。采用公开 GetSamples／GetNonOptimizedSamples，不增加第二套私有曲线读取；新增 signed raw mode 与 opaque32 为有初始化依据的必要补充。上述结论均为静态结论，不是运行观察。
