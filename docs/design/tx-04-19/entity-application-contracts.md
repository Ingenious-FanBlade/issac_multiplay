# tx-04-19 I04：非 Player 原生应用静态合同

日期：2026-11-03，Asia/Singapore。范围为 I04 的 R01—R13；实现入口仍是 I05—I32，运行验收仍是整个 tx 的 I34。本文与 [J273 必要逆向](entity-native-static.md)、[I03 来源研究](entity-source-research.md)共同使用。研究完成不表示所有类别已经可以原生应用，也不表示下列尚未闭合的原生分支已经安全。

资料顺序为本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs`、本地 REPENTOGON Wiki `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs`、同版本官方源码／ZHL、最后必要 J273 指令。`PUBLIC_DOC` 为公开语义，`SOURCE` 为源码事实，`REUSED_J273` 为 17／18 已核事实，`J273_STATIC` 为本轮二进制静态事实，`DERIVED` 为推导，`DESIGN` 为采用决定，`UNKNOWN` 为尚未解释内容。本文没有新运行观察；不执行试调用、探针、测试或哈希校验。

## R01 分类、字段和存在性

`SOURCE`：正式 `domains/entity/collect.lua::snapshot` 依次尝试 Player／NPC／Tear／Projectile／Bomb／Laser／Knife／Pickup／Familiar／Effect／Slot 的公开转换；NPC 再调用 `ToDelirium`，特殊接口附在同一主体上。转换失败后才读具名 RTTI supplement；不按 `Type>=10` 把 Text／Dummy 当成 NPC。当前 `entity_target.cpp::Category` 已拒绝旧 `NativeDarkness`；`model.cpp::Capabilities` 已声明 NativeDummy，I04 不再新增别名。

| 类别／special | 适用性与字段合同 | 原生 owner／后续任务 |
| --- | --- | --- |
| NPC | 真实 `ToNPC`／动态类型及当前配置；不能由数值范围推出 | NPC 池、配置、内嵌 PathFinder；I11 |
| Delirium | `LuaDelirium.cpp::lua_EntityToDelirium`：原形 type=412，或 NPC 的 BossType／BossVariant 与当前 type／variant 同时相符；不是另一个实体实例 | 同一 NPC，`+BC8` 起 special；I11 |
| Tear／Projectile／Bomb | Type2／9／4 只是已分类后的交叉检查，各自独立布局、flags 与高度语义 | 各自工厂池；I12—I14 |
| Laser | Type7，HomingLaser 是内嵌关联结构，无第二 Entity Key | Laser 池和内嵌容器；I15 |
| Knife／Familiar | Type8／3；Player、Weapon、EntityPtr 必须分类型 | 各自池与真实 Player owner；I16／I18 |
| Pickup／Slot | Type5／6；变体及专用字段分别解释，不从同一个整数推导统一语义 | Pickup／Slot 池；I17／I20 |
| Effect | Type1000；VarData 是随 variant 改变解释的存储，不是统一 payload | Effect 池；I19／I24 |
| NativeText／NativeDummy | Type9001／964 与对应 RTTI；不伪造转换、vptr 或派生字段 | 独立池／动态类型；I21／I22 |
| Player | 保留在完整成员集合，19 只消费具名外部依赖；不复制第二个 Player | 17 代表 owner，完整实现归20 |
| 未知 | 保留具体来源与 Unsupported／Unknown，不过滤后宣称 Complete | I05／I22 写前报告 |

`SOURCE`：`collect.lua::Capture` 合并公开枚举、native membership 与 PlayerSources，按稳定身份去重并保留完整性；`EntityTarget` 分开 source coverage、membership_complete、non_player_complete。`Exists=false`、`IsDead=true`、Retired、缺席四者不能合并。原生 Remove 后仍在 EL 的对象是合法待退休前缀；死亡但仍存在的 NPC 可仍需 draw。I06 只能在完整同 scope 成员集合中按明确缺席退休；不能用一次公开枚举或部分字段覆盖率作删除依据。

### 必须按源码宽度而不是 Wiki 的通用 int 安装

| 字段／接口 | `SOURCE` 已核存储／返回 | 采用方式 |
| --- | --- | --- |
| Delirium TransformationTimer | `EntityNPC.zhl` `uint16 +BD0` | 验证 0..65535，不依靠 Lua 截断 |
| RemainingAttacks／StateD／Angle | `uint8 +BD2/+BD3/+BDC`；Angle 文档虽称 int8，wrapper 实际正整数0..255 | 不把 >127 变成负数 |
| BossType／BossVariant／AttackID／Cycle | `uint32 +BC8/+BCC/+BD8/+BE0` | 保留所有位，不用 red mode／timer getter 重建 Cycle |
| GetTeleportationTimer | `LuaDelirium.cpp` 使用 `(Cycle>>15)&0x3FF`，setter 清 `0xFE007FFF` 后装十位 | Wiki 的“15到25 inclusive”文字不作为十一位布局；当前完整 Cycle 为权威值 |
| Slot State／PrizeType／DonationValue／PrizeCollectible | `int32 +410/+414/+420/+540` | 语义按变体；DonationValue 是实体状态，不是 PGD 捐款总额 |
| Slot ShellGameAnimationIndex | `short +41A`，setter 显式强转 short | 安装前检查 int16，不悄悄溢出 |
| Slot GetTimeout | `short* +41C`，Lua getter 符号扩展；setter 强转 unsigned short 再写同一16位；已读Init/Update也是word访问 | Wiki高位描述与本版本实现冲突；尚未证明有packed timeout存储，不从文档新增高位字段 |
| Slot Touch／TriggerTimer | `uint16 +42A`／`uint32 +424` | 各自范围；不推进计数 |
| TearFlags 等 BitSet128 | 128位值，不是 Lua number 或单个 uint64 | 保留两半和零值；不按 flags 触发玩法 |
| Bomb GetFallSpeed／GetFallAcceleration | `float +41C/+420`；旧 GetHeight／GetFallingSpeed 名字是弃用误标 | Bomb 可见高度使用 PositionOffset.Y；不得套 Tear Height 偏移 |
| Effect136 GridDesc | `GridEntityDesc.zhl` 大小0x20，Initialized在+18，VariableSeed在+1C | 按八个语义字段复制；不复制padding，不以BitSet128的16字节截断 |

字段目录中的 `returns` 保留公开 API 原文，不能偷偷改写历史来源；具体本地宽度由本文／J273合同及后续布局声明约束。模型的 Null、合法空集合、NotApplicable、NotScheduled、Unavailable、ReadFailed、Truncated 不因类型宽度已知而变成零。

## R02 构造、发布、引用和成员 owner

`REUSED_J273`：[17 R2](../tx-04-17/tx-04-17-native-object-lifecycle.md)核定 `EntityFactory::Create VA 6EC540 / RVA 2EC540` 是预构造池 use 的取得，不是每次执行 ctor；Player 不在通用工厂内。已有 Effect 分支的第二参数 true 选择不覆盖活槽，不能按名字 force 理解为强制覆盖。各池分支的新增结果见必要逆向专题。

`SOURCE`：`Patches/EntityManager.cpp` 的 Factory Hook 在 `super` 正常返回非空后执行 ESSM NewEntity；Player ctor 另行通知。`Patches/EntityPlus.cpp` 基 ctor 建立 holder，**基 Init 之前**重建对应类型的 unique_ptr 扩展，随后 `super`；Remove 释放 Plus 的 Hook 是注释，析构才删除 holder。获取池地址、Init 和外层发布是不同前缀，不能清零完整对象或跳过官方链仿造它们。

`J273_STATIC`：主审独立回读 `VA6EEE60` 池构造和 `6EC540` 选择器。Knife 的 `6EF0AD push 520h`／`6ECF1B imul ...,520h` 证明池步长0x520，不采用ZHL末尾的sizeof=0xE78；Effect／Slot的池步长0x4C0／0x550也不等于ZHL声明的0x490／0x548。已核字段布局与整个类型大小分别处理，不因这个差异否定所有Getter，也不按SDK sizeof分配、遍历或整对象复制。Dummy池虽已构造，但Create无964专用分支，默认 `6ED1C6` 进入NPC池；禁止用Create(964,true)伪造Dummy，真实取得owner仍须具名解释。

同一池构造的静态容量为Effect1024、Familiar64、Bomb64、Projectile512、Tear512、Laser128、Pickup512、Slot128、NPC512、Text128、Dummy128、Knife64；它们不是每个Key都能额外分配的数量。I30需同时计A、待退休池use、未发布候选和外部原生成员；纯值模型允许16384主体不扩大任何原生池。池不足选不覆盖活槽并失败／恢复，不临时改变force或从本项目分配一块sizeof内存冒充池项。

`DESIGN` 的通用顺序：解析完整 T 和预算 → 建立 Key／新实例记录 → 记录 Acquire-entered → 工厂取得且官方 New 正常完成 → 满足类别构造前置 → 记录 Init-entered → 动态派生 Init／必要官方维护 → 安装已核最终状态／真实资源 → 分配本地 Index → 具名 Publish → 普通可后置关系 → 只读后态 → Scene 唯一提交。不能在 Publish 时提交 A，不能把原生内部提前可见当作本方允许 draw。

`REUSED_J273`：发布 `VA 413D00 / RVA 013D00`，ABI 为 `void __thiscall(EntityList*, Entity*)` 的采用合同，raw EAX 无成功含义。有效性、main／persistent、render、flags 控制的 update、wisp 和类型缓存分开核对；重新发布已 valid 对象有刷新 update 成员的分支，不能当通用修复器。17 的“所有非Player进mainEL”、拒绝 dead、拒绝非空 `+310` 都只是代表对象保护，I07 不得删除保护后放行全部类别。

| 关联 | 来源与 owner | 安装／取消合同 |
| --- | --- | --- |
| Parent／Child／Target／Spawner及明确 EntityPtr | 原版 EntityPtr Wiki；`EntityPtr.zhl::SetReference`、`Entity.zhl::SetParent/Child/Target/SpawnerEntity` | 共同 Key→当前实例解析后调用 SetReference；Null 同样调用；不直接改 `_ref` |
| Knife hitboxSource | `EntityPlus.h::EntityKnifePlus` 内的 EntityPtr，`LuaEntityKnife.cpp::Lua_SetHitboxParentKnife` | Plus 类型已正确建立，按当前 Knife Key 绑定；Init重建Plus前撤销旧借用 |
| Familiar Player | `EntityFamiliar.zhl` 原生裸指针`+410`；Weapon裸指针`+DB0` | 不是通用EntityPtr；I09显式追踪双向／专用列表，先解除依赖后退休Player |
| NPC PathFinder／Familiar PathFinder | 内嵌结构`+414`／`+5FC` | 只保留权威观测纯值，不运行寻路来补显示，不按网络字节覆写内部容器 |
| 命中集合 | Tear/Laser/Knife AddToHitList调用 `Entity::GetHitListIndex`；不是稳定Key | 19保留Authority纯值，停止本地碰撞消费；若以后需要本地集合，先通过Key转换本地编号，不照写Source整数 |
| Effect136 GridDesc | 描述值，非活动Grid；Grid域lifted owner提供 | 无Grid slot、无第二Key；Source unresolved原Grid不临时编造引用 |
| 构造前置环 | 各类真实caller/Init读取决定，不能仅由字段名推定 | 普通引用环可后置；需要owner才能Init的环在写前报依赖，不能先填首Player |

`J273_STATIC`：主审独立读 `GetHitListIndex VA6B1ED0 / RVA2B1ED0`：一般返回`+20 Index`，但type35/216/239/909/912/918及244 variant2等可经`6A4070`取parent链顶层Index；28 variant2、62 variant1还依`+3C0`选择自身或parent，19/89保持自身。它不是统一的普通Index，也不是可安装的本地字段。目录生成器因此将`Entity.GetHitListIndex`从通用状态路由修正为Authority身份事实；源数字和原查询仍保留。

## R03 NPC、Delirium 和 Morph

`PUBLIC_DOC/SOURCE`：NPC Morph 是操作，不是字段集合 setter。官方 `CustomCallbacks.cpp::Entity_NPC::Morph` 为 void，pre可改参或取消，post仍是外围副作用。`REUSED_J273`：`VA 6BD0C0 / RVA 2BD0C0` 原位改配置／资源；`6BD24F` 调 `904DE0` 可写 PGD bestiary，因此普通字段应用不采用整个 Morph。即使参数相同 native 早退，外围回调仍不能据此当作无副作用 no-op。

`DESIGN`：配置、动态类或组合部件变化优先同逻辑Key的新实例替换；当前NPC字段、HP、死亡表现直接安装已核最终值，不调用AI／Damage／Kill／冠军随机选择。PathFinder、寻敌、伤害归因／bestiary是Authority事实；draw所需公开最终位置、颜色、状态与部件Key则必须安装。特殊组合NPC与Delirium不能以普通NPC一条vtable覆盖而宣布适配完成。

`SOURCE`：`LuaDelirium.cpp::Transform` 往 `delirium::ForcedTransformations[Entity_NPC*]` 写入请求并将 TransformationTimer 置1；`ASMDelirium.cpp::TransformationCallback` 在后续变形分支取出／erase并可能调Lua。它依赖AI，不采用为立即安装形态接口。special各宽度见R01；Source当前变形配置与这些special同一目标安装，重绑部件、资源和引用。不得留下地址键的 ForcedTransformations 来等待客户端Update完成。

**未决与责任：** NPC组合draw的非公开附属owner、冠军/状态专用缓存与特殊Boss资源并非目录可读就已完整；必要逆向中的具体分支、来源缺口与 I11／I25 接线一起处理。它们不改变类别支持目标，也不许可当前把所有NPC降级成单Sprite。

## R04 Tear、Projectile、Bomb

`SOURCE`：Tear、Projectile、Bomb ZHL分别声明自己的派生结构，禁止按字段名共享偏移。Tear `_height/_fallingSpeed/_fallingAccel +410/+414/+418`、flags128 `+428`、hitList `+444`；Bomb fallSpeed/fallAcceleration `+41C/+420`、scale`+434`、hitList`+9C8`。这些是官方声明，具体写入口只在绑定和适配器正式接入后可用。

| 类别 | 必要资源／值 | 明确采用与禁止 |
| --- | --- | --- |
| Tear | 主ANM，halo`+4AC`、effect`+5C0`、dead-eye`+6D4`，各为独立ANM；高度、旋转、scale、父体／附着、flags | `LuaEntityTear.cpp` 三Getter均返回真实成员地址；安装最终姿态。`ResetSpriteScale(force)`会改`_scaleAnimNum`再调native，不作为未核定补缓存兜底；禁止FireSplitTear／MakeMultidimensionalCopy／Update |
| Projectile | 当前高度／下落／角度／轨迹后态和主资源，独立flags与派生draw | Source最终值而非本地推进；`LuaEntityProjectile.cpp::Deflect`是操作，不用它换方向；不执行分裂／落地／命中 |
| Bomb | 主ANM及`_bombCostumesSprites[5] +464`、countdown、rocket/fall/scale、关系 | `LuaEntityBomb.cpp::GetCostumeLayerSprite`有界0..4，成员stride0x114；五份不能合并。Countdown只装结果，不调用explode/damage/update |

`REUSED_J273`：`EntityPlayer.GetBombFlags` 的真实函数 `VA 779880` 即使false也可在Fruit Cake路径消耗随机；只消费真实调用记录或已核当前值，不主动查询。当前来源将其标为实际操作，这一点不因Bomb adapter新增而放宽。GetLootList也不作为初始化／读取兜底。

**失败责任：** Init前缀、独立辅助ANM、命中容器及父体分别登记；资源更换前停止旧draw并退尽CPU使用，池退休不析构内嵌ANM。具体派生draw临时位置／颜色暂存和特殊flags分支见必要逆向；未取得必要最终缓存时 I12—I14/I25 必须补正式来源，不能补一次Update。

`J273_STATIC`：主审独立读Bomb Render `VA6A0F90` 与`6A2420`。Render在`6A0FBB`读`+463 _loadCostumes`，真时`6A0FC4`调用后者；它在`6A246F`清`+462`起word，遍历五个stride0x114 ANM Reset，后经`407AF0` Load、`407DA0` LoadGraphics及动画/层/贴图重建。不是纯draw或音效helper。`DESIGN`：I14安装Source**完成该重建后的**主／五costume最终资源和姿态，并在对应Client用途中抑制再次lazy-rebuild；Source普通窗口若捕获到待重建标志而资源仍旧，不能将那组值当作最终draw结果。I03/I14/I25需显式匹配该来源时点/独立render依赖，不以本地重跑helper补齐。

## R05 Laser、HomingLaser 和几何

`PUBLIC_DOC`：原版 `GetSamples` 是描述路径的优化点集，`GetNonOptimizedSamples` 为另一组采样；getter不保证刚进行计算。`SOURCE`：`HomingLaser.zhl`大小0x54，Entity_Laser内嵌`+464`；optimized vector`+480`、nonoptimized`+48C`、unk vector`+498`、Bezier`+4AC`，四组独立owner，Bezier元素0x18。不可复制vector头或外来allocator的begin/end/capacity。

`REUSED_J273`：[I03主审](entity-source-research.md)确认普通 Render 直接消费 optimized vector；raw signed mode`+45D=-1`早退，0/1选择不同draw，不能压成bool；`+4F0 int32`只知signed<=0控制起点偏移，不猜语义。GetSamples accessor`VA956820`只返回`this+480`，非优化getter`956B40`只返回`this+48C`；采用公开来源而非第二套私有读取。

`DESIGN`：将Source既有优化点值装入**当前Laser原生owner拥有的**正确容器，预算同时计旧容量、新目标、临时安装和在途；不得从本项目CRT分配内存后伪装成游戏vector。可采用何种原生分配/容器操作由I15核定实现，尚无跨CRT安全结论。无更新的draw可消费目标最终几何；未直接消费的非优化/Bezier/unkVec保留Authority纯值，只有具体draw分支证明确需才新增正式源。

`SOURCE`：`EntityPlus.cpp::CheckRecalculateSamples`会同时清Bezier／samples／unkVec／nonoptimized，然后等待更新；`RecalculateSamplesNextUpdate`仅设扩展标志。`LuaEntityLaser.cpp::SetScale`还调ResetSpriteScale、RotateToAngle调native，均不是全都leaf setter；不得为补几何调用这些路径、CalculateSamples、链式生成或PlayInitSound。纯leaf的DisableFollowParent／Shrink／DamageMultiplier／HomingType／NumChainedLasers可按类型装值，但链计数不授权本地生成子束。

## R06 Knife、Familiar 与 Player 依赖

`PUBLIC_DOC`：Familiar.Player是所属身体，不是参与者ordinal。`SOURCE`：Familiar还有独立Weapon、PathFinder和isFollower/isDelayed；`LuaEntityFamiliar.cpp::GetDirtColor`复制当前ColorMod，但UpdateDirtColor会调native，不作为读取替代。RemoveFromPlayer会调native并不等于基Entity Remove，必须单独管理所属Player列表。

`DESIGN`：外部接口按稳定Player Key返回当前实例、类型、代次、owner资格及被该draw实际读取的窄字段／资源。I16/I18只能在依赖满足时进入构造/绘制；主Player非空、首Player存在或普通Isaac支持不等于所有武器形态支持。19不调用FireKnife、EvaluateItems、AddFamiliar、FollowParent/Position、RecalculateOrbitOffset或Shoot来造目标状态。

Knife的parent knife／hitboxSource用已解释EntityPtr，Player查找和Weapon另走原生关系；Familiar跟随、环绕、延迟队形和Weapon分别核owner、列表摘除与NULL含义。具体Init何时绑定及Render读取分支见必要逆向；未解释的Weapon私有资源保持明确依赖缺口，不制造假装备或临时挂首Player。

`J273_STATIC`：主审独立读Knife Init `939C70` 和owner helper `93F5E0/93F650`。DeadEye ANM的`new0x114`是全局`VA BE5594`的lazy资源，经`939CFF`写全局并登记到全局图形资源指针容器，不是Knife内嵌成员；Load/层/blend初始化可在首个Knife Init发生。不能为每个Knife都析构该共享owner，Tear的独立dead-eye成员也不能冒充它。`93F5E0`分别从Parent、Familiar.Player、Effect.Parent、Tear专用关联查Player，SpawnerEntity不能代替这些边；`93F650`沿Knife Parent链，输入必须先检查合法链/循环，不能让不可信目标形成原生死循环。

主审另读Familiar Render `61E360`：`61E533`读全局`BD0674`、`61E5A3`写回inline xorshift、`61E604`形成临时alpha并`61E60D`进颜色路径，不是调用`6EB4B0`。variant48的`+ECC ExtraData`经RTTI转IsaacsBodyData并绘制多份ANM，variant231经AbyssLocustData绘制两组ANM容器；68/78/243调用Player专用draw helper并暂改其颜色/可见性。`DESIGN`：I18/I25/I27按具体分支补资源与当前表现来源、替换随机结果，I03的公共Familiar Sprite不足以覆盖这些专用成员；共享Knife DeadEye、Familiar ExtraData、临时随机颜色分别建owner和scope，不复制私有指针或假装普通Isaac已满足全部依赖。

## R07 Pickup 和 Slot

### Pickup

`SOURCE`：`EntityPickup.zhl`除主ANM、价格ANM`+410`，还有altPedestalANM`+5D0`、ghost`+590`、megaChestCollectible[2]`+594`、最多8项cycle数组`+5A0`／count`+5C0`、flipSaveState智能owner`+5C8`。`LuaEntityPickup.cpp::GetPriceSprite`是借用；GetCollectibleCycle按count复制；AddCycleCollectible先检查配置再append，不是原子替换完整集合。I17安装完整集合需显式count／清空语义，不反复append来实现幂等。

`SOURCE`：`PickupUtils::InitFlipState`可new并注册保存状态、根据seed调用ItemPool选物品，复制/交换ANM，SetupCollectibleGraphics、LoadGraphics、Play、Update；不采用为Client初始化。公开SetAlternatePedestal／SetForceBlind／SetPrizeCollectible等只要进入非leaf必须分别审具体链，不能因名称Set就当纯值写入。`REUSED_J273`：Pickup Morph`VA6DFF30`涉及RNG、派生Init、引用清理和价格ANM Load/Reset，默认同Key替换，不整体放行。

altPedestal及flip owner不能被I03“主/价格已采”覆盖。R10/I17按真实Render消费补正式有界来源；若flip只用于玩法恢复则保留描述纯值且阻止其本地消费，不能为造描述调用InitFlipState。删除需动态Pickup Remove及专用关联责任，不能只基Remove/Erase。

`J273_STATIC`：主审独立复读 `Pickup::Render VA6E35D0 / RVA2E35D0` 的伪码与raw。入口条件是`exists +172`、`visible +171`、`dropDelay +554<=0`，不能误记为valid/dead/timeout。variant100在`6E383C lea ecx,[esi+5D0h]`后`6E384E call405A50`真实绘制altPedestal；主Sprite/price不能替代。`+6D9!=0`且Room`+12BC!=5`分支于`6E3784—6E37CC`执行全局RNG `BD0CA4`的xorshift并写回，`6E3805`写当前`+684`浮点值，然后绘制altPedestal；因此连Render也不能整体当作纯读放行。

`DESIGN`：I17/I25/I27在这个具名分支产生随机前替换为Authority真实表现结果，保留完整资源/几何/必要临时维护；I03回补`+6D9`适用性、`+684`当前值及真实altPedestal资源/精确姿态的同scope正式来源。Source普通窗口与该Render产生值的时点必须声明，不把上次随机缓存误标本次绘制结果。不得全局保存/恢复RNG、固定清该分支、跳过alt层或运行一次本地随机来冒充同步；具体窄分派尚待实现。

### Slot 1—18 的适用性

以下是 `PUBLIC_DOC` EntitySlot及现有`slot_variants.lua`的解释；表中的“未声明”保持公开语义未声明，不声称native永不使用其它字段。所有变体的state／prizeSprite／公共姿态仍采真实值，不能因不常用补零。

| variant | 公开特殊语义 | 安装／禁止消费 |
| --- | --- | --- |
| 1 SlotMachine | PrizeType3..12奖励、13..24无奖；9未明 | 当前值与资源；不开奖 |
| 2 BloodDonation | 非通用DonationValue计数解释 | 装当前状态；不扣心／统计 |
| 3 FortuneTelling | 结果表现，未声明通用PrizeType编码 | 不重抽牌／文字 |
| 4 Beggar | DonationValue硬币 | 不投币／支付 |
| 5 DemonBeggar | DonationValue心量 | 不套硬币值，不扣心 |
| 6 ShellGame | PickupVariant奖品、Prizes层及animation index | 真实奖品ANM；不选择开奖 |
| 7 KeyMaster | 未声明通用PrizeType编码 | 不扣钥匙／生成奖品 |
| 8 DonationMachine | 本地实体表现与持久总捐款不同 | 不写PGD或RandomCoinJamAnim |
| 9 BombBum | PrizeType1硬币/2心/3道具；爆炸trigger30 | 不触发爆炸／奖励 |
| 10 RestockMachine | trigger重抽次数／可能11 | 不重抽商店 |
| 11 GreedDonation | 贪婪捐款表现 | 不写持久总量 |
| 12 DressingTable | 当前换装展示 | 不重跑Player服装裁决 |
| 13 BatteryBum | DonationValue随机增量，出奖归零 | 安装Source后态，不本地随机 |
| 14 HomeClosetPlayer | 真实角色展示依赖 | 由Player窄接口供应，不创建新参与者 |
| 15 HellGame | PickupVariant+PrizeCollectible | 不从奖品标量重造独立Entity |
| 16 CraneGame | PrizeCollectible；Wiki记复合timeout但本版本公开存储为signed16 | 高位存储尚未定位，不补造packed字段，不截断Source真实值 |
| 17 Confessional | 当前交互结果 | 不裁决心量／随机效果 |
| 18 RottenBeggar | DonationValue随机增量，出奖归零 | 同13，奖励实体独立Key |

`SOURCE`：State／PrizeType／DonationValue／ShellGameAnimationIndex／Touch／Timeout／TriggerTimer的官方wrapper均为直接存储写，宽度见R01；仅在合法当前实例上安装。`RandomCoinJamAnim`调用`Isaac::Random(4)`，CreateDropsFromExplosion调用真实掉落，均不采用。捐款相关Source实机操作仍需用户实际授权，静态研究不提供新授权。

`J273_STATIC`：主审独立复读 `SetPrizeCollectible VA6626F0 / RVA2626F0`：`6626FF`先对**所有variant**写`+540`；15随后调用`6F55C0/6F5750`规则查询、`6DF1A0`图形选择，并对prizeANM`+42C`调`407DA0`；16对mainANM`+48`走同类图形维护。它是`__thiscall void(Entity_Slot*,int)`、ret4的非leaf链，不采用整setter兜底。I20直接装已核标量与Source最终主/奖品资源及姿态，避免本地规则/seed选图和ANM Update消费；这些显示后态不得由本地开奖产生。

## R08 Effect族和 lifted Grid136

`SOURCE`：Effect.GetGridEntityDesc仅在variant136返回从`_varData +420`开始的GridEntityDesc借用；描述0x20字节，而VarData的通用C++名字BitSet128仅16字节。Effect.GetTearFlags仅对54／113／126／167解释同一起始区域，setter直接写128位；不得和136描述同时解释。CreateLight wrapper即使默认参数表达式也包含rand，并调用Spawn与genrand，CreateLootPreview接LootList owner；两者不采用为Client最终值安装。

`DESIGN`：按必要逆向识别的Init／draw／附属结构分族，至少分普通sprite、地面持续、透明／光、带子对象／资源、136描述；variant57只有原有代表支持。所有有独立Entity Key的子体从T创建，内嵌资源才随owner；超时、动画事件、light/creep Update不推进世界。136仅装完整描述与真实资源，不建活动Grid；落地／破碎／生成Grid结果等待Authority新T。

具体136 descriptor读写、Grid资源身份及退休见必要逆向；未知Effect附属结构和生成分支不是“Effect无害”可放行的理由。I19实现适配族时将具体variant加入明确表，未核定分支报告具体原因，不回退Effect57。

`J273_STATIC`：本轮找到真实136创建者`VA81AE00 / RVA41AE00`，主审独立读取其调用链和raw。`81AEB5`先调`81A920`准备Grid描述及临时**完整ANM2**；`81AEBE`调用RNG，再经七参桥`424340`创建1000/136。`81AEE9/81AEEF/81AEF2`把临时ANM2置ECX、Effect`+48`置EDX，调用`403160`；官方`Isaac.zhl::SwapANM2`、`ANM2.zhl::construct_from_copy`及`PickupUtils.cpp`与该交换语义一致。`81AEFE/81AF0C`两条MOVUPS将完整32字节描述写到`+420/+430`，`81AF19`从Room`+1D18`写Effect.State`+450`。因此Source136的主Sprite不是只设置文件名，更不是从Grid另创建活动owner；它承接了真实资源、层及姿态。

`81A920`在`81AA26`从Grid内嵌ANM`+40`构造副本，`81AA34`交换到临时输出；`81AA4E/55`复制GridDesc两半。主审还确认它会删除/替换Grid为Decoration，且`81ADBA/81ADC4`对该格关联Type96 Entity写`+B64=16`并直接调用vtable+0Ch Update。它是有世界副作用的lift过程，**不采用整个81AE00或81A920**作为Client构造／只读Getter。

`DESIGN` 的I19/I24采用合同：取得正确Effect池use并在已核136 Init前缀/用途内建立基础owner，显式安装来自同T `Grid.lifted_descriptors`的八个描述字段、Source Effect.State及Source真实主Sprite的resource-pose；不从本地Room.Backdrop或重新lift计算结果。Init自身只清`+420`起16字节且将136置不可见，不提供完整描述／资源，不能在其返回时提交。字段安装使`+430`后半成为本方已定义状态，不读未初始化池残留；无活动Grid、无第二描述Key。父体／举持展示由I09/I16/I20的真实依赖供应；未满足该依赖不把独立Effect Render返回当作举持画面完成。临时ANM与内嵌ANM各由其原生owner收口；不在本轮实现裸swap，后续资源服务负责撤借用、准备和CPU使用退场。

## R09 NativeText、NativeDummy

`REUSED_J273/SOURCE`：[I03来源研究](entity-source-research.md)已确认Text字符串`+418`最多256字节，Init不初始化字符串；完整KColor20从`+518`读取，opaque32不可替换为公共Color。counter／duration资格和字符串writer资格不同。Render不会补造writer见证；仅Current-use真实writer完成才读文本，未终止/无writer/超限不能从池旧内容构造目标。

`DESIGN`：I21用真实Text池use/Init建立必要基结构，显式装合法已捕获字节串、计数／寿命／颜色，再用真实字体draw；不靠第一次Update格式化字符串或重算淡出。独立native-text-render-v1的Manager frame/pause/subframe/font输入保留独立scope，普通快照不能伪装已有该phase；所需Source与关联未齐时等待／报错。其字节编码沿Source原字节，不擅自UTF-8转码。

Dummy独立动态类型，仅已证公共状态；真实取得caller仍UNKNOWN，通用工厂964会进NPC池，不采用。主审独立读 `VA6A3630`：它经 `6AC4E0`转换绘制位置，`6A3667/6A3670`压入variant/type，`6A3673`格式串为`e%d.%d`，最终进字体路径`9E9500`；不是公共Sprite的替代画法。当前未发现需要新派生字段的依据，但字体／主线程／context依赖仍由真实owner供应。不改NPC/Effect vptr或Type伪造Dummy，不把未知类别当Dummy。NativeDarkness旧名称只作历史文本，非别名。

## R10 资源成员、借用和来源缺口

| 真实成员 | `SOURCE` 位置 | owner／失效／来源责任 |
| --- | --- | --- |
| 全类别主Sprite | Entity.zhl `ANM2 +48` | 原生Entity owner；Init/Morph/替换撤旧借用 |
| Tear三辅助 | LuaEntityTear三个Getter、ZHL`+4AC/+5C0/+6D4` | 三独立ANM；I03已有正式精确姿态 |
| Bomb五costume | LuaEntityBomb有界Getter、ZHL`+464`数组 | 五独立ANM，0..4；P3优化已恢复 |
| Pickup价格 | LuaEntityPickup::GetPriceSprite、ZHL`+410` | 独立ANM；I03已有；Morph可Load/Reset |
| Pickup alternate pedestal | ZHL`+5D0`、PickupUtils.cpp | 真实独立ANM，不能合并主Sprite；当前正式源需按draw补齐 |
| Slot奖品 | LuaEntitySlot::GetPrizeSprite、ZHL`+42C` | 独立ANM；SetPrizeCollectible可重载，需重取layers |
| Knife共享DeadEye | Init`939C70`、全局`BE5594`及图形资源容器 | 全局lazy owner而非Knife/Tear成员；正式来源及每次draw借用/姿态管理由I03/I16/I25补足 |
| Familiar特殊ExtraData | Render`61E360`，variant48 IsaacsBodyData／231 AbyssLocustData | 具体RTTI/容器owner与多ANM，不能拿公共Sprite替代；I03/I18补源 |
| Laser四容器 | HomingLaser.zhl | 内嵌owner，清空／重分配／Init失效；只复制值，不复制头 |
| NPC／Knife／Familiar／专用Effect私有资源 | 对应ZHL＋必要逆向 | 指明具体使用分支，不能因无公开Getter宣布不存在 |

`DESIGN`：复用 `ClientParseResourcePose`、`ClientApplyResourcePose`、Image pins、上下文寿命；主/overlay精确phase、每层状态、shader、贴图和Null均来自真实同窗口成员。Null／未加载合法空辅助成员与缺失/失败不是一回事，不能调用Load/Update来“读到”。相同路径/帧不证明别名。只有精确同一成员可声明alias。

资源identity、对象实例、ANM内部代次与容器借用分开。关闭旧draw admission → 当前活跃栈返回 → 原生CPU队列实际消费或已解释取消 → 原位Load／Morph／容器变化 → 重取借用 → 完整后态再开放draw。Image pin只保Image，不能保Entity／ANM／layer指针。副作用失败不会恢复旧A；未知context／SEH不重复析构。

## R11 draw和pass分派

`SOURCE`：当前 `client_presentation_native.cpp::Members`只接受一个普通Isaac与Effect57，`DrawEntity`把所有非Player静态转Effect。这是需要I25替换的代表实现，绝不能删除Members保护后调用。GetRenderZ走vtable slot13；基虚表slot5 Render的采用ABI为`void __thiscall(Entity*,Vector*)`，但各类官方Render包装与额外pass必须按类别接入，不能只做一次vtable替换就宣称全分层完成。

`SOURCE`：`CustomCallbacks.cpp`分别Hook Familiar／NPC／Pickup／Tear／Knife／Effect／Bomb／Slot Render，存在前后Lua及参数／原调用规则；Laser也有独立原生draw链。采用路径必须在本项目唯一用途分派中阻止玩法Lua，同时保留必要资源/官方维护及观察original exactly-once；不全局清CallbackState。

| pass | 采用合同 | 不允许的兜底 |
| --- | --- | --- |
| render排序 | 复用当前FullSort对真实成员取GetRenderZ、以本地Index稳定排序；包含所有目标成员和明确预算 | 调旧sort共享scratch／空表分支；写Source Index伪装本地 |
| shadow／light预pass | 复用18已有FBO／depth／queue顺序，逐类提供真实消费者与必要状态 | 清共享depth、跳过透明层或主画面中途flush |
| main／墙柱交错 | 真正动态类draw，按Z与Grid柱分层；临时position/color按该类原值恢复 | 全部cast Effect、统一只画GetSprite |
| 水面／镜像DrawBand | 原生窄consumer也会间接虚调用，必须包含同一类别允许性与缓存要求 | 主pass已画后再无条件全员draw |
| post／特殊beam／Text | 具名类别的最终缓存／字体／几何与context | RoomPreRender全链、Interpolate、Entity/ANM Update补缓存 |

`REUSED_J273`：17／18实际修复过共享depth、FBO与队列深度，不能倒退到WithoutSwap结束所有offscreen pass或重置z后重画主场景。静态合同仅说明采用条件；实际像素、层次、镜像、透明遮挡与异常恢复留I34，不以返回正常代替画面正确。

## R12 删除、部分失败与重建

`SOURCE`：官方EntityManager::CommitRemovedEntities仅在stagedRemove时扫描update/render/wisp/main/persistent，owner列表的非Player先ESSM Delete再ClearReferences/valid=false；Player另Free。other lists不清。当前本方代表Remove显式处理七列表、ESSM闭包、类型缓存，并拒绝未解释`+310`；它不能推广为全类别清理器。

主审从各ctor对vtable的真实写入独立追出以下`J273_STATIC`分支，并复读所列原始指令。偏移均属于表内具体类，不推广到同名字段或其它Entity。

| 类别／真实入口VA（RVA） | 直接证据 | 对退休与最终Player状态的约束 |
| --- | --- | --- |
| Knife ClearReferences `93A1E0 (53A1E0)` | 调基Clear后，`93A22C/93A241`读`+4FC/+500`；`93A23E/93A253`各调用该关联体vtable+28h Remove；另清`+4F4/+4F8`注册关系 | 撤销/解释这些owner及其独立Key，不能只清四个公共引用后假设无连带删除 |
| Familiar Remove `60A040 (20A040)` | `60A08F`在variant43/subtype0减Player`+136C`；`60A0A2`在variant73减`+1370`；variant68/78/243经`60A0D2/DA`写Player`+1640/+1644` | 非战斗Remove仍改Player状态；保持真实Player有效，按已知前后缀安排重新安装同T的Player窄状态，不能在Player已释放后退休Familiar |
| Familiar ClearReferences `60B800 (20B800)` | `60B936`调用Weapon`+DB0`所指对象vtable+14h，`60B947`再调slot0(1)，`60B949`置空；另有grid/Player特殊分支和专用容器清理 | Weapon是实际附属owner，不是EntityPtr；不能浅拷贝／重复析构，不能在未核定私有Weapon分支上宣称异常恢复 |
| Pickup Remove `6EA840 (2EA840)`／ClearReferences `6EA860 (2EA860)` | `6EA843`读`+590`，`6EA84F`调用关联体Remove，再跳基Remove；派生Clear也处理ghost及`+58C/+594/+598`注册关系 | 动态清理不能替换为基类调用；目标仍活的ghost/关联Pickup必须先按已核方案撤关系，不能让删除父Pickup误删它们 |

这些事实解释了正常链存在的附属调用及跨对象写入，不证明所有variant／子owner的异常可恢复；后续必须在原调用前决定如何处理，不能先让它们删除目标存活对象再用最终图遮盖。

| 实际前缀 | 首要责任 | 恢复选择 |
| --- | --- | --- |
| 未写原生 | 取消纯值／资源候选，正确context释放本方已完成候选 | 保留仍有效旧A |
| 已取得池use／官方New，未Init | 保留新use记录与ESSM责任；不能再从工厂取得同Key掩盖前次 | 仅执行已解释该类的退休前缀；未解释时保持隔离 |
| Init进入未正常返回 | 标记entered早于调用，保留附属owner／已发生副作用 | 不重跑Init、不盲析构；异常不确定时不自动重试 |
| Init返回未发布 | 按该类池/资源/引用清理并完成官方Delete | 不直接delete池项 |
| 已发布／已装引用 | 先停新借用和显示、退CPU使用，撤入出引用／专用成员，再动态非战斗Remove／摘表／官方Delete／缓存失效 | 全前缀完成且实际owner一致后才可复用；不重复Remove |
| 资源部分Load／Morph | 撤所有失效借用，保留实际写入前缀和旧owner退休责任 | 停显受影响A，从完整T重建；不宣称回滚 |
| draw已排队 | 先消费／已解释取消真实raw资源使用 | 不以EL摘除／Image pin代替队列结束 |
| stop／切scope／菜单／Shutdown | 维持Client隔离至Scene安全退休及最后相关持久writer之后 | 正常菜单与进程关闭分别收口；未知时不交还Off |

基 ClearReferences`VA6A8060`会在`6A8167`对`+310`关联体调用virtual Remove；这不是单纯清指针。具体owner尚未解释的实例必须保留保护、首因和待退休记录，不擅自设Null绕过。Familiar专用Player关系、Pickup ghost/flip、NPC组合体和Effect子owner分别纳入；不能让Room全清理与单对象删除释放同一owner。

C++异常、SEH、Lua错误、OOM、context丢失与正常返回是不同能力。只有具名正常前缀的清理已解释时可自动重建；未知ABI／线程／对象代次改变停止新写入。虚拟设备NEVER生命周期保持原约定，不参与Entity池释放。

## R13 用途分派、副作用和持久整链

| 路径／用途 | 已知消费者 | Client决定／后续接线 |
| --- | --- | --- |
| Factory／基ctor／Init | 官方New、EntityPlus、ESSM结构 | 保留必要维护；按主线程entered/returned记录，不走整个Spawn后处理 |
| 派生Init／配置 | RNG、音效、子体、Lua及资源，差异见必要逆向 | 按具名分支保留结构、安装Source结果、在生成前阻止玩法；不能先生成再删多余对象 |
| NPC Morph | bestiary VA904DE0、pre/post Lua、资源 | 默认实例替换；未来真采用时在写入前隔离具体writer |
| Pickup Morph／Flip | RNG、ItemPool、ESSM save、ANM Update、额外资源 | 不采用整链来获得最终值 |
| Slot操作 | Donation／支付／掉落／Isaac::Random | 只装当前状态及核定显示；不支付、不捐款、不RandomCoinJamAnim |
| Laser/Tear/Bomb操作 | PlayInitSound／split／explode／samples计算 | 不重演；必要结果与音画事件由Authority提供 |
| 派生draw／shadow／light/post | 官方pre/post Lua、部分临时写、资源队列；Pickup/Familiar Render另有全局RNG写`BD0CA4/BD0674`，Bomb有lazy资源重建 | 相同用途scope贯穿嵌套调用；替换已核随机分支和资源重建为Authority最终表现，禁止玩法/动画事件更新 |
| 非战斗退休 | Remove回调、Lua registry、官方ESSM Delete、ClearReferences连带Remove | 幂等前缀控制；必要官方闭包可执行，gameplay callback阻止 |
| stop/Exit/Shutdown／Mod保存 | 17已核PGD、平台、GameState/Mod附加JSON与最后保存writer | 准备前启用保护，退休与最后writer结束才Off；不能以最终文件未变证明窗口无污染 |

`DESIGN`：唯一 `hooks.json`／registry 是所有新增Hook owner；按角色Client、操作用途、具名entry、当前主线程、scene/attempt/instance限制实际分支；Host／Off保持原调用，观察wrapper super exactly-once。嵌套构造/退休不能继承一个“Client允许所有native”的布尔值，未知连带路径在调用前停止。不恢复字节、不卸载重注入，不触及原版Online／Isaac Sync。

## 接线责任与完成边界

I04交付的是以上R01—R13的资料核对、采用/拒绝合同、必要逆向及主审和具体缺口；没有新增运行Hook或猜测性写入。I05— I32仍负责实际适配、来源回补和场景接线；I33/I34负责唯一整体E2E。具体未闭合项在必要逆向专题末尾逐条登记，不能将研究状态当成native_apply=true。

`entity-application.json`的静态研究索引指向本文与必要逆向；每个成员原有来源、责任、NotReplicated/AuthorityOnly和native未应用状态保持独立。新发现来源缺口需要随对应类别修订正式来源版本／能力合同，不能通过测试夹具填充、删字段或缩小完整类别目标掩盖。
