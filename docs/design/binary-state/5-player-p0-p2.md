# 5：Player 前三个状态块的设计

**后续实施（2026-11-09）**：正式Player源与生成已迁入版本47，见[实现说明](implementation-5-player.md)。本文件保留设计时点和证据；构建不读取设计JSON。Plus未知与ABI模型限制保持，实际采集／安装及最终显示缓存仍待后续步骤，没有提前测试。

**2026-11-09 后续补充**：[统一字节账本](player-byte-ledger.md)及[旧成员逐行回映射](player-member-map.md)已完成设计登记；本文末节所列这两项待办是原时点状态，以新登记为准。新记录还区分原版通信域、待移除效果队列、移动箱保存体和Plus ABI推导；正式布局／运行接线及第8步最终缓存仍待实施，布局版本仍46。

**后续P6—P8（2026-11-09）**：[后三块定稿](5-player-p6-p8.md)已补齐P6最终目录与统一Player拓扑、P7结构资格／C/S控制记录、P8效果与绘制历史投影。本文P0固定13份及costume投影保持；`+1C50`额外Sprite随P8当前可绘记录走world，不并入13槽。P6 revision改为会话内统一Player绘制结构版本，覆盖跨身体backup／held依赖；原始History／16索引／form counters及native sub／replaced／703四槽留Host。九块设计完成，全对象账本／生成／运行接线仍未完成，版本仍46。

日期：2026-11-09（Asia/Singapore）。目标：官方 REPENTOGON 1.1.3／J273，映像基址 `0x400000`。本文是[第5步](5-player.md)的 P0、P1、P2 设计定稿；没有实现 Player 布局源、生成表或运行接线，**布局版本仍为46**。P3—P8、整个 Player 的字节闭合及第15步验收没有因此完成。

**后续设计补充**：[P3—P5](5-player-p3-p5.md)现已完成设计，P6—P8仍待完成；P5细化为按具体动态类型的充能输入和Modifiers mask，不能把本文复用版本39通用四值的历史表述当作每种Weapon的必传要求。P0自身布局与五个Render计数不变，尚未生成／接线。

## 1. 同步判据与范围

Client 承载 Host 已产生的世界，停止玩法模拟。因此，字段的公开性、原生 offset、旧安装器已经复制它，均不足以证明它需要同步。每项按以下顺序决定：

1. 原生绘制、受支持的界面或安装 owner 直接读取该值，且不能由已安装的结果代替：同步当前值。
2. Client 已有明确的结果消费者及最终缓存安装路径：同步结果所在的缓存，不重复发送它的全部生产输入。
3. 只参与移动、射击、伤害、属性重算、道具操作、存档等 Host 玩法：留 Host。公开 getter 不自动成为 Client 功能要求。
4. 地址、STL 头、设备键、Lua／Plus owner、加载结果及生命周期资格：本地管理，绝不能照抄。
5. 未解释的字节保留 UNKNOWN；“当前未找到消费者”和“已经证明是填充”不是一回事。

本轮覆盖旧 P0—P2 候选、原生身体绘制需要的标量和 Sprite 快源拆分，并明确与 P3—P8／Presentation 的责任边界。本文的负面归属针对本产品的原生展示路径及已声明接口，不声称任意 gameplay Lua getter 在 Client 可用。第三方 Mod 的额外展示需求已迁至[后续方案](../mod-adaptation.md)，不作为本轮实现条件；不得为所有未知调用者复制整套 Host 模拟输入。

证据分为：**本轮 J273 静态复读**、**官方声明／源码**、**既有 J273 事实复用**和**设计决定**。以下均不冒称运行观察。VA 减 `0x400000` 即 RVA；没有启动游戏、构建、执行测试或校验哈希。

## 2. 决策总表

| 范围 | 决定 | 原因与实际去向 |
| --- | --- | --- |
| P0 身体快源 | 保留身体表现标量、实际 headAnim、全部13份内嵌 ANM2 的表现投影及 costume Sprite 表现投影 | 直接覆盖原生 Render 的读集；物理拆块，避免一个动画变化重发全部 Sprite |
| P1 属性 | **本轮定为1B：CanFly** | Render／RenderBody 直接读取；六项最终 stats 的显示已经由 StatHUD 缓存承接 |
| 六项最终 stats | 不在 Player 重复同步 | MoveSpeed、MaxFireDelay、Damage、TearRange、ShotSpeed、Luck 的原生属性栏消费的是第8步的最终 StatHUD 值／差值／前值／阈值／计时；不能为刷新它而运行 EvaluateItems 或 StatHUD Update |
| P2 生命与资源 | **77B 固定负载** | 保留直接参与原生健康显示、资源计数、主动道具显示及额外生命计算的存储 |
| EternalHearts、BoneHeartsMask | 原值留 Host | 已核普通心量生产器使用它们；本设计安装最终逐心缓存，不重跑 UpdateHearts。两项显示仍在缓存中，不是丢掉永恒心或骨心排列 |
| cacheFlags／属性生产链／攻击模板 | 留 Host | 不让 Client 重算属性或重新生产子弹；实际 Tear／Laser／Knife／Effect 等已有自己的最终状态 |
| 控制器、原版 Online 状态、原生地址、valid／工厂资格 | 不进入这三块 | C/S 控制／P7结构事务／本地 owner 分别负责，不能从可见性或数值相同推导资格 |

P1 的调整不删除属性栏。第8步必须安装实际选中 Player 的全部 StatHUD 展示缓存及其身份、布局和资源；缺失时报缺口，不能只写六个属性后放行整个刷新函数。

## 3. 原生证据及两处容易误判的路径

### 3.1 身体与充能条

| J273 VA（RVA） | 本轮复读结果 | 设计含义 |
| --- | --- | --- |
| `784A60`（`384A60`）／`784BCA—784C07` | Render 读 `+13B8`，正值且余6小于3时，结合 `7A2630`、challenge30及 NO_TARGET 决定闪隐 | DamageCooldown 有真实展示用途，P0保留；不只凭“无敌计时”将其删掉 |
| `7A2630`（`3A2630`） | 读 `+1398`／`+139A` 两个独立字节 | 两个 full-sprite 开关保留，中间 `+1399` 未解释，不合并成3B |
| `783680`（`383680`），`783803—783C01`／`7849AC` | 29／38备份展示读取六项 `+14E8—14FF` 比例；`783B41`取 `+1320` 的真实 headAnim 字符串 | 六值及实际字符串保留；不能从 Direction 或当前 ANM2 frame 猜 headAnim，也不能用公共 Sprite.Scale 覆盖这些独立比例 |
| `786460`（`386460`）／`7864D0`；`784B32` | 直接读 `+1564 CanFly`，选择 costume 层和飞行偏移；`77B890`先读同目标Room.type==16分支，再按CanFly返回偏移 | P1保留 CanFly；它不能被 Entity.IsFlying 或碰撞类别等价替代 |
| `785F5F`（`385F5F`）→`758850`（`358850`） | **Render 内调用充能条选择／绘制器**，并非仅在 Player Update 预先生成 | 不能以“已发送充能条 Sprite”为由删除所有充能输入 |
| `75889B`／`7588D6` | 读 `ItemState +15F4`；710分支再读 `CraftingUseCharge +1F54` | 两项进 P0，不调用 SetItemState／UseItem |
| `7589A3`／`758A81`／`758ADA`／`758B33` | 分别读 `EveSumptoriumCharge +1F58`、`Maw +18A8`、`Revelation +1EC0`、`Montezuma +1EC4` | 四项进 P0；按采样原值安装，不在 Client 递增、递减或到期 |
| `7589D5—758A5E` | 从 Player `+13DC` 起查四个 Weapon，即槽1—4，调用动态 slot7／slot8；不是主动道具四槽 | 依赖 P5 的真实动态 Weapon；沿用版本39四项 Weapon 值与常零 slot8 的既有核定，不重发两份 chargebar 结果 |
| `758917—758983` | 角色27查 TemporaryEffect704与 config.cooldown | P8／配置同目标提交；不能用库存或 costume 代替效果记录 |
| `758D50`（`358D50`） | 根据进度选择 Charging／StartCharged／Charged／Disappear，设帧／播放，随后由758850绘制；没有推进 ANM2 Update | 允许已核展示选择，禁止为它放行 Player Update／Shooting／动画事件 |
| `78558D—785733` 的持物分支；`784FF5`／`78501E`／`785043` | 使用 `ItemState +15F4`、`SuplexState +1F08` 和真实 held 边／body null-frame | 两个表现状态在 P0，held 边仍由 P7安装；Suplex 的 aim／target／land 不因同名整体复制 |

`GetBlinkLockTime +1338` 与 `GetHeadDirectionLockTime +133C` 是另外两个锁时。官方 `LuaEntityPlayer.cpp::Lua_PlayerGetBlinkLockTime`、`Lua_PlayerSetHeadDirection` 及 J273 `773990` 的动画生产分支说明它们参与选择／锁定头部动画；现有 Render 以已产生的 ANM2、headAnim 和上述 `+13B8` 消费。两项锁时留 Host，不作为伤害闪隐源。头方向、移动方向同样不代替实际 Sprite／headAnim。

### 3.2 属性栏、心量与资源

| J273 VA（RVA） | 本轮复读结果 | 设计含义 |
| --- | --- | --- |
| `83CFD0`（`43CFD0`），`83D01A—83D0FC` | 从 Player 六项最终 stats 生产 StatHUD；射速为 `30/(MaxFireDelay+1)`，射程为 `TearRange/40`，再维护差值等历史缓存 | 六项原值并不能替代完整属性栏状态；复制 Player 后再调用该生产器也会重算其它概率 |
| `83C860`（`43C860`）／`83D210`（`43D210`） | 分别刷新／递减显示缓存、换 Player 并重算概率与差值 | 均不作为 Client 刷新入口 |
| `HUD::Render 97C300` → `StatHUD::Render 83C9B0`（`43C9B0`） | 属性栏绘制格式化实际 StatHUD 缓存；现有 `client_presentation_native.cpp::Stats`／安装路径已经直接安装五值结构 | 第8步继续同步最终显示缓存，P1不重复这六项模拟属性；现有单根安装不外推四人完整覆盖 |
| `PlayerHUD::RenderHearts 834930`（`434930`） | 普通路径使用24个 PlayerHUDHeart 的可见、闪烁、动画／overlay等缓存；还读取 PlayerType、CoopGhost、效果、HealthLimit和ExtraLives | 原生心量存储不能替代最终心量缓存；P2也不能脱离 P3／P7／P8和第8步使用 |
| `834988—8349C7` → `83B160`（`43B160`） | `Game+18304 == -19` 的另一健康显示路径；`83B2DB—83B2F9` 和 `83B355—83BA89` 直接读最大／红／魂／黑／金／骨／腐心等存储 | **不能因普通路径用缓存而删除全部原始心量。** 此处只记录实际条件，不猜该负值的产品名称 |
| `7C1150`（`3C1150`）／`7C12A2` | HealthLimit读 BrokenHearts；Keeper分支另读硬币、配置／库存等 | BrokenHearts 与硬币保留；不使用普通角色的 `red<=max` 约束验证所有角色 |
| `7B67A0`（`3B67A0`）／`7B67A6` | ExtraLives汇总库存及 `movsx word [Player+137E]` | 新增2B有符号额外生命项；不能按邻近 `_bloodLustCounter +137C` 合并，也不能只靠库存推导 |
| `97C300` 的资源分支 | 直接格式化 coins／bombs／keys／soulCharge／bloodCharge，读取 goldenKey／goldenBomb／gigaBombs并选择图标 | 各值保留，不能把金钥匙／金炸弹当普通计数的上限值 |
| `835B40`（`435B40`） | 主动道具显示读 `+1354` JarHearts、`+1358` JarFlies、`+1DBC` UrnSouls、`+1361` GoldenBomb；Bethany分支读独立魂／血充能 | Jar两项、UrnSouls与两种充能保留，不从主动槽charge或VarData替代 |
| `83A840`（`43A840`）／`83A898` | RenderSpellQueue直接读 `+1F30 PoopMana`，再按 `+1F34` 六槽绘制 | P2保留 mana，六槽交 P3；mana不能代替队列内容 |

`EntityPlayer.zhl` 为本轮字段位置与 SDK 类型输入。Jar两个 i32 的 Init store 在 `74FE2F`／`74FE39`；CoopGhost在 `74FD61`；Broken／SoulCharge／Urn／Giga 的 Init store 在 `75140D`／`751421`／`751487`／`751491`。这不是向未观察 Init 的备份身体补造初始化历史；实际读资格仍按第10步及项目规范第11节。

**一次偏移误命中的纠正**：检索 `+1480` 会命中 `97C393—97C3B1`。这里 EDI 是 **HUD**，正在写布局缓存常量，不是读取 Player.TearFlags。不能拿“Render 里有同一 offset”作为同步依据。本设计不据此发送 Player 的128位攻击模板。

## 4. P0：身体快源

### 4.1 标量与字符串，64＋L 字节

P0已审查原生范围的单行归属表如下；这不是全Player覆盖表，省略范围见第7节。ANM2／costume元素仍按各自字节类别展开，不把它们整体归状态。

| 0x48 ANM2 主Sprite【继承】父E4／E5／E6，不重发第二份 | 0x41C—121F ANM2[13]【嵌套】逐槽复用ANM2字节归属，表现归P0投影 | 0x1220 std::vector&lt;CostumeSpriteDesc&gt;【容器】头本地，有序目录归P6、元素Sprite表现归P0 | 0x1320 std::string headAnim【容器】只传有效字节 | 0x1338—133F i32[2]锁时【权威】不传 | 0x1398 bool fullSpriteA【状态】 | 0x1399 1B【未知】 | 0x139A bool fullSpriteB【状态】 | 0x13B8 i32 DamageCooldown【状态】 | 0x14E8—14FF f32[6]绘制比例【状态】 | 0x15F4 i32 ItemState【状态】 | 0x18A8 u32 MawCharge【状态】 | 0x1EC0 u32 RevelationCharge【状态】 | 0x1EC4 u32 MontezumaCharge【状态】 | 0x1F08 i32 SuplexState【状态】 | 0x1F54 i32 CraftingUseCharge【状态】 | 0x1F58 i32 EveCharge【状态】 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

以下均为**草稿线布局**，尚未进入生成链。`L<=2048`，长度以原始字节计，不转十六进制、不要求 UTF-8。headAnim 是实际可变字符串，Client 使用本地 std::string 安装，不复制其24B原生头。

| +0x00 u32 render_topology_revision：所依赖P6绘制拓扑版本，C/S元数据、无原生offset | +0x04 i32 damage_cooldown：原生闪隐输入 | +0x08 u8 full_sprite_a：+1398开关 | +0x09 u8 full_sprite_b：+139A开关 | +0x0A f32 body_scale_x：+14E8 | +0x0E f32 body_scale_y：+14EC | +0x12 f32 body_layer_scale_x：+14F0 | +0x16 f32 body_layer_scale_y：+14F4 | +0x1A f32 head_layer_scale_x：+14F8 | +0x1E f32 head_layer_scale_y：+14FC | +0x22 i32 item_state：+15F4当前表现分支 | +0x26 i32 suplex_state：+1F08当前表现分支 | +0x2A i32 crafting_use_charge：+1F54 | +0x2E i32 eve_charge：+1F58 | +0x32 u32 maw_charge：+18A8原位模式，原生显示按signed转换 | +0x36 u32 revelation_charge：+1EC0 | +0x3A u32 montezuma_charge：+1EC4 | +0x3E u16 L：headAnim字节数 | +0x40 u8[L] head_animation：+1320字符串的有效字节 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

- 不传计时到期锚点。上述值受武器、角色、动作、暂停和刷新条件影响；Client只安装采样值。DamageCooldown不能拿 game_frame 无条件倒推。
- 不从 Direction 重建 headAnim。29／38的跨身体绘制实际查询该字符串，官方 setter 也能改它。
- 标量直接复制区间为 `13B8+4B`、`1398+1B`、`139A+1B`、`14E8+24B`、`15F4+4B`、`18A8+4B`、`1EC0+8B`、`1F08+4B`、`1F54+8B`；按来源各读一次。字符串另作有界有效内容复制。不能为减少 memcpy 吞入中间其它字段或未知字节。
- 六项比例必须有限；在29／38比例换算路径成为除数的值不得为零，沿用既有具名 preflight。不要把所有负比例一律禁掉。Suplex沿既有0—5语义校验；其它有符号值不随意清负值或截到100。

### 4.2 Sprite 拆块及32块限制

P0是**逻辑快源组**，不把所有 Sprite 拼成每次必发的一大块。原生13槽固定为 `41C、530、644、758、86C、980、A94、BA8、CBC、DD0、EE4、FF8、110C`，步长 `114`。`41C`是body Sprite，`758—DD0`是七个充能条，`EE4`是blood-gush，`FF8`是held；其余保留槽名，不猜专用名称。共同主 Sprite仍在 Entity `+48`，**不再复制一份**。

物理块号按父 Entity十块之后追加，避免把13×3投影逐个扩展到32块以外：

| 物理块号（拟定） | 序列 | 内容／草稿单行布局 |
| --- | --- | --- |
| 0—9 | 沿父布局 | 继承 E0—E9；Player的E1／E4／E5以及本设计提升的E6走body，其余world |
| 10 | body | P0标量／headAnim，见4.1 |
| 11／12 | world | P1／P2，见第5／6节 |
| 13—18 | world | 为P3—P8保留，不在本轮定稿这些块的内容 |
| 19 | body | 固定13项 ANM2子块0，槽序同上，最小13×123=1599B |
| 20 | body | 固定13项 ANM2子块1，最小13×22=286B |
| 21 | body | 固定13项 ANM2子块2，最小13×2=26B |
| 22 | body | u16 count＋count×costume ANM2子块0 |
| 23 | body | u16 count＋count×costume ANM2子块1 |
| 24 | body | u16 count＋count×costume ANM2子块2 |

三类投影的单行草稿结构分别为：

| ANM2#0[13] inline_appearance：资源选择、最终外观及必要播放参数，复用ANM2子块0 |
|---|

| ANM2#1[13] inline_animation：主／叠加动画及各层独立帧选择器，复用ANM2子块1 |
|---|

| ANM2#2[13] inline_layers：最终图层状态，复用ANM2子块2 |
|---|

| +0 u16 count：与P6真实costume有序目录完全相同 | +2 ANM2#k[count] costume_projection：k分别为0／1／2，不包含任何原生vector头 |
|---|---|

这些范围的原生字节归属逐槽展开[ANM2表](layouts/02-base-types/ANM2.md)，并不将13×0x114全部归状态。每份ANM2原有14B未知、self／地址／加载状态／事件位的归属保持。loaded=false只形成既有可空资源语义，不读无效数组，也不擅自补零；源未准备好与Host明确Null分开。

P6只拥有costume有序目录、配置／属性／层映射等结构信息及其版本，不再重复拥有这些已转P0的Sprite投影或headAnim。主Sprite的E6提升到body，避免动画与其独立层状态采自两种频率后互相覆盖。第8步负责实际Image／shader旁表，不能把已有路径字段当成全部图像资源。

目前使用25个物理块，保留25—31七个位置；**不是**新增一种超32位 block_mask。固定13项不再编码重复slot号；costume三个count必须一致，空目录三个count均为0。count上限沿既有costume安装器的16384保护边界，但真实消息还受fast 64KiB、world32MiB和总内存预算限制；保护上限不等于可发送规模。

合法完整快源若不能容纳于fast消息，改走ordinary body-joint，具名记录尺寸原因并撤销该身体旧快候选；通过既有完整帧／已确认基础恢复规则重新建立快源基础，不截断costume或图层，不部分提交一组投影。超出world或整体内存预算则报实际预算缺口，不宣称该状态已同步。

按现有基础布局下界，P0本类快源组为 `1981+L+147×costume_count` B；Player父快块另有295B下界。此数是全量状态下界，**不是每帧带宽**：memcmp／版本分别作用于各物理块，只发送变化块。动态动画最小286B，外观1599B不能因普通动画推进一并重发。图层、颜色、实际活跃Sprite、costume数量与三端扇出仍须纳入第15步性能验收，不预告已满足预算。

### 4.3 拓扑与采集时机

`render_topology_revision` 是C/S自有u32递增值，P6也携带它。它覆盖costume顺序／配置／层映射及投影解释所需资源结构；CanFly等world值改变对应层选择时，也随P6目录提交新版本，不能让新姿态先按旧飞行分支绘制。它不是原生地址、shape版本或整个world版本。新值不回绕复用，不计算哈希。

- ordinary提交P6目录及同拓扑P0；body帧只有在Client已安装**相等**的P6版本、成员表／资源表及相同Key/use/shape时可安装。等待期间丢该body候选，保留已提交画面；按既有Ack／Resync恢复，不把它误写到旧目录。
- Host在合法PlayerPost复制所需拓扑元数据并与自有已发布内容比较。仅比较vector地址／size不能发现原位重排；不采用这种假稳定判定。发现拓扑改变时先走ordinary结构发布，不能在网络线程遍历或请求主线程临时getter补齐。
- body窗口只产生P0组及父快块，不枚举主动槽、Weapon、库存或随机。costume元数据／Sprite是绘制快源自身的有界读取，不等同枚举装备玩法状态。
- 没有合格PlayerPost时，由完整ordinary窗口采集P0组，保持既有ordinary body-joint语义；不把odd-only或单个PlayerUpdate当完整世界窗口。
- P1／P2只取ordinary。一次采样先进入自有不可变缓冲，再比较／交接；不把任何Player*或容器头交给Rust／网络线程。

P0中的held／备份展示、充能条和投影还依赖P3／P5／P7／P8／第8步。关系、角色或资源结构变化须由结构事务先提交，不能仅靠body新鲜就接受旧结构。拓扑版本只解决目录解释问题，不代替这些owner的合法生命周期。

## 5. P1：展示属性，1字节

原生范围及草稿线布局：

| 0x1564 bool CanFly【状态】Render偏移／飞行costume选择；官方SDK声明与J273784B32／7864D0 |
|---|

| +0x00 u8 can_fly：0／1，直接装Player+1564 |
|---|

以下不上P1，也不改放P0／P2：

| 原生范围／成员 | 归属与替代 |
| --- | --- |
| `145C` MaxFireDelay、`1460` ShotSpeed、`146C` Damage、`147C` TearRange、`155C` MoveSpeed、`1560` Luck，共24B | Player模拟属性留Host；实际属性栏五值缓存归Presentation。六项没有因此被标成“Client可由库存重建” |
| `1470—147B` TearHeight／FallingSpeed／FallingAcceleration，12B | Host生成攻击用；客户端消费已生成实体的最终高度／位置／姿态，不用Player模板重放射击 |
| `1480—148F` TearFlags，`1490—14BB` TearColor，`14BC—14E7` LaserColor，共104B | Player的攻击模板留Host；各实体必要效果位／颜色按自己的已核布局同步。不能据本条删除Tear／Laser／Knife上实际被Render读取的同名字段 |
| `1518—152F` 六个i32 modifier、`1540—1557` Eden六值、`1928—1937` D8四值、`1E94—1EAB` RockBottom六值 | 属性生产历史／倍率留Host。最终属性栏缓存不是重建这些历史输入的证据 |
| `1568—156B` CacheFlags | Host待重算工作位；Client不写入后调用EvaluateItems，不以此同步外观 |
| `156C` TearDisplacement、`1570—1573` TearPoisonDamage | Host攻击生产参数；中间 `156D—156F` 无填充证明，保留UNKNOWN |

`Entity.IsFlying()`的原版leaf `6A7C90`读公共碰撞类别；REP `EntityPlus.cpp::IsFlying`还能覆盖结果。Player+1564却有上述独立消费者。因此P1、父E2和E9 nullable override各有职责，不互相强制相等，不取消版本39公共Plus合同。

## 6. P2：生命与资源，77字节

### 6.1 原生字节归属表（本块涉及范围）

| 0x1340 i32 MaxHearts【状态】健康显示 | 0x1344 i32 RedHearts【状态】健康显示 | 0x1348 i32 EternalHearts【权威】原值不传，显示由最终逐心缓存承接 | 0x134C i32 SoulHearts【状态】健康显示 | 0x1350 i32 BlackHearts【状态】原始掩码，健康显示 | 0x1354 i32 JarHearts【状态】主动图像进度 | 0x1358 i32 JarFlies【状态】主动图像进度 | 0x135C i32 Keys【状态】计数 | 0x1360 bool GoldenKey【状态】图标 | 0x1361 bool GoldenBomb【状态】图标／主动显示 | 0x1362—1363 2B【未知】不并入复制 | 0x1364 i32 Bombs【状态】计数 | 0x1368 i32 Coins【状态】计数／HealthLimit | 0x137E i16 ExtraLivesAdjustment【状态】ExtraLives实际signed求和项 | 0x1938 i32 GoldenHearts【状态】健康显示 | 0x1D74 i32 BoneHearts【状态】健康显示 | 0x1D78 i32 BoneHeartsMask【权威】原值不传，最终逐心排列缓存承接 | 0x1D8C i32 BrokenHearts【状态】HealthLimit／显示布局 | 0x1D90 i32 RottenHearts【状态】健康显示 | 0x1D94 i32 SoulCharge【状态】资源与主动显示 | 0x1D98 i32 BloodCharge【状态】资源与主动显示 | 0x1DBC u32 UrnSouls【状态】Urn主动显示 | 0x1DC0 i32 GigaBombs【状态】图标选择 | 0x1F30 i32 PoopMana【状态】法术显示 | 0x207D bool CoopGhost【状态】Player／HUD绘制分支 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

本表是P2审查范围，不是整个Player从0到2EA0的完整账本。范围间仍有P0／P1、后续块或UNKNOWN，见第7节；不能把表格省略区域默认为填充。

### 6.2 草稿线布局

| +0x00 i32 max_hearts | +0x04 i32 red_hearts | +0x08 i32 soul_hearts | +0x0C i32 black_hearts：原始掩码 | +0x10 i32 jar_hearts | +0x14 i32 jar_flies | +0x18 i32 keys | +0x1C u8 golden_key | +0x1D u8 golden_bomb | +0x1E i32 bombs | +0x22 i32 coins | +0x26 i16 extra_lives_adjustment | +0x28 i32 golden_hearts | +0x2C i32 bone_hearts | +0x30 i32 broken_hearts | +0x34 i32 rotten_hearts | +0x38 i32 soul_charge | +0x3C i32 blood_charge | +0x40 u32 urn_souls | +0x44 i32 giga_bombs | +0x48 i32 poop_mana | +0x4C u8 coop_ghost |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

直接复制11段：`1340+8B`、`134C+16B`、`135C+6B`、`1364+8B`、`137E+2B`、`1938+4B`、`1D74+4B`、`1D8C+16B`、`1DBC+8B`、`1F30+4B`、`207D+1B`，共77B。不能跨过EternalHearts、BoneHeartsMask、1362未知字节或邻近玩法计数。

### 6.3 读写、共享与缓存

- 采集**每身体原生backing**，不调用可能转向twin／sub的公开健康或资源getter。共享语义由P7真实owner／关系表达；既不把同值当共享，也不把关系推成所有字段必须相同。
- 不用旧 `ClientPlayerStateApplicable` 的角色谓词将其它身体的值静默补零。受支持身体有本字段当前读取资格才复制；没有资格时报具名Gap，不借别的身体的Init或PlayerPost。
- bool只接受0／1，计数保持实际i32／u32／i16位宽。BlackHearts为掩码，不执行非负普通心量范围验证；extraLives项可有负值。角色、配置、模式和保存恢复会影响合法心量，不能使用 `red<=max`／全部计数0—99等统一限制，也不通过clamp修复报文。
- 只用直接最终值安装器；不调用AddHearts／AddCoins／TakeDamage／Revive／AddCharge／RemovePoopSpell等操作。这些方法可能触发治疗、奖励、随机、Lua、音效或持久写入。
- ordinary事务安装P2、角色／关系、库存／效果和第8步相应显示缓存。普通24心缓存、独立动态PlayerHUD、额外生命、属性栏及八个HUD布局槽的真实owner各自保留；现有缓存写入代码只提供窄先例，不表示全部四人已实现。
- **不运行UpdateHearts**。EternalHearts与BoneHeartsMask原存储不要求与Host相等，也不纳入漂移比较；它们已产生的心图像、overlay和布局仍必须与目标缓存一致。`83B160`直接需要的八项健康值仍安装，不能用这项裁剪扩大到全部健康存储。
- 将来若新增受支持的原生展示消费者读取被裁掉的原存储，须先明确消费者和缓存／直写职责，再修订整数布局版本；不能用旧Lua源回退掩盖缺口。

## 7. 已排除项与尚未完成的字节账本

### 7.1 P0候选的归属

| 原生范围 | 类别／处理 |
| --- | --- |
| `1320—1337` std::string | 【容器】只发送P0有效字节；头内地址、capacity、SSO存储由本地std::string维护 |
| `1338—133F` BlinkLock／HeadDirectionLock | 【权威】动画生产锁时；最终ANM2与headAnim承接结果 |
| `1398`／`139A` | 【状态】两个独立字节进P0；`1399`仍【未知】 |
| `14E8—14FF` 六f32比例 | 【状态】进P0；不是属性生产倍率 |
| `15F4—15F7` ItemState、`1F08—1F0B` SuplexState | 【状态】P0表现分支；相关真实Entity边归P7 |
| `15F8—15FB` ItemStateCooldown、`1604—160B` BombPlaceDelay／ForgottenSwapCooldown | 【权威】动作推进／操作冷却，不从ItemState有展示用途推成整个邻接区都传 |
| `160C—160F` ControllerIndex | 【本地】Client保持中性-1；不以Host设备号选择C/S参与者或原版Online本地性 |
| `1610—1613` PlayerIndex | 由P7核定结构／主备顺序，不放进P0位置快源，不能据此伪造本地设备绑定 |
| `1618—161F` MoveDirection／HeadDirection | 【权威】已核动作／动画生产输入；P0不根据输入或速度重新选择朝向，安装实际headAnim／Sprite |
| `1811` CanShoot | 【权威】玩法输入可消费性，不是C/S的控制组资格 |
| `18A8—18AB`、`1EC0—1EC7`、`1F54—1F5B` | 【状态】五个原生Render充能输入进P0；不删除，也不重复放进P2 |
| `2070—2077` SamsonBerserkCharge／IBSCharge | 【权威】既有静态路径是玩法／动画生产，不同于758850的五项直接Render输入；不在P0／P2重复，已有最终效果、Sprite及必要武器承接表现 |

`P7`的独立激活资格、room／render／update成员、body role及主备关系仍是版本31未完成项。不能因P2保留CoopGhost就推导valid，也不能将所有新构造Player设为原生活动体。

### 7.2 UNKNOWN与后续块范围

本轮不宣称完成整个 `0x2EA0` 对象。至少以下具体间隙没有填充证明：`410—41B`、`131C—131F`、`1362—1363`、`1399`、`1565—1567`、`156D—156F`、`1614—1617`；13份ANM2仍各有14B未知，继承Entity的未知也不变化。这不是全Player UNKNOWN总数。

其余尚未形成完整覆盖表的范围由第5步后续继续核定：

- `1220`costume vector与`122C`起15个layer map、全部costume元素元数据／特殊资源：P6；本文只确定其P0表现投影和拓扑版本接口。
- P3库存／口袋／法术队列、P4主动槽、P5 Weapon、P7关系／控制结构、P8效果／History，以及专用角色／保存区域：各自按真实消费者补齐，不能用本文的零散已知区间冒充全覆盖。
- `1C44`可选绘制序列／`1C50`其Sprite、`1E38`残影deque、`1FF8`／`2000`Image strong、`20A8`动态HUD pair等已有历史具名事实，但完整二进制归属、块号及其快慢序列仍归后续块；不得默默省略或强塞进P0尾部。
- Suplex aim／target／land、角色特殊值及保存容器不能整体归“状态”；后续仍按最终Render输入与Host生产输入拆分。

上述已知存在的后续工作不是本轮P0—P2设计缺项的隐藏兜底；在全部第5步布局生成前，Player类型仍不得标完整。最终必须从0到2EA0逐字节归属，并保留UNKNOWN，而不是把这些列表当生成器输入。

## 8. 接线合同、失败方式及交付边界

### 接线合同

- **第5／1步**：将本文草稿迁入唯一 `state/player.json` 后才分配实际layout编号、提高整数版本并生成两种单行表／复制计划／回映射；本轮不伪造生成结果。检查继承块数、19—24投影和P6拓扑版本仍一致。
- **第8步**：StatHUD五值缓存、普通／动态PlayerHUD逐心缓存、实际资源及C/S显示选择由具名owner准备；不运行完整HUD Update／RecomputeStats。
- **第9／12步**：沿既有帧／记录头；Player body允许父E1／E4／E5／E6和10／19—24，拒绝body夹带P1／P2；P6拓扑依赖额外校验，不改变成员身份或恢复原版Online字段。
- **第10步**：中文注释说明各核心reader及结构；主线程按已知字段资格采集，不跑Lua getter，不构建JSON，不读取UNKNOWN、STL头地址或原版NetplayManager补值。
- **第11／13步**：逐物理块比较／版本／安装；父快块和P0同一次body目标提交，P1／P2随ordinary。P6拓扑和资源未就绪不得ACK，native临时Sprite／held借用到CPU队列消费完毕后精确恢复；网络平滑只建本地历史，不放行Interpolate或玩法Update。
- **第14步**：旧final-state表里多余属性／锁时／模板等不再强制“必须齐全”；旧Captured查询字典不作新路径缺项回退。历史记录保留，不将原测试PASS改写为本文布局已经通过。
- **第15步**：唯一E2E验收，覆盖普通／特殊健康分支、永恒／骨心缓存、正负生命修正项、Jar／Urn／Bethany／poop资源、真实闪隐、五种Render充能、29／38比例／headAnim与拓扑换代；再覆盖四参与者／多身体、丢帧／重连／恢复及既定性能预算。新增场景必须在用户准备后按规范执行，本文不授予实机操作权限。

### 必须识别的失败方式

1. 将HUD+1480误认成Player+1480，或把cacheFlags当最终外观同步。
2. 只看普通心图缓存就删掉原始健康值，导致83B160特殊健康显示错误；或反过来只发心量、丢掉逐心过渡／永恒心overlay／骨心排列缓存。
3. 将充能条758850误当Update：删除五计数，或运行完整Player／HUD Update来弥补。
4. 只复制头方向，headAnim在29／38备份绘制时不一致；将BlinkLock当DamageCooldown、或计时器在Client自行推进。
5. 把13份ANM2当原始字节连续memcpy，发出self／地址／事件／loaded和未知字节；或普通动画变化重发全部外观大块。
6. body只核vector地址／count，原位重排后将costume帧装到错误槽；三个投影count不一致、P6拓扑或资源尚旧却ACK。
7. 按公开共享getter采样后写回每个身体backing，或把角色不适用值补零，污染原生主备关系。
8. 将BlackHearts当普通非负数量、extraLives当u16、Jar值混成ActiveCharge，或使用普通角色心量上限强改特殊角色。
9. 把CanFly、公共IsFlying与Plus nullable覆盖合并，丢掉飞行层选择、水面或Null／false差异。
10. 为凑快源读资格向未观察初始化的备份借用其它身体证言，或将P1／P2放到PlayerPost并冒称完整世界。

### 本轮实际完成

P0标量／Sprite分块／拓扑依赖、P1的1B最小合同、P2的77B字段与11段复制计划、保留／排除理由、原生消费者、后续owner责任及失败方式已定稿。只修改设计与相关入口；没有改动运行代码、生成文件或布局版本，也没有运行任何测试。整个Player的字节表、P3—P8及正式reader／writer仍待后续第5／8／10／13／14步实现，第15步统一验收。
