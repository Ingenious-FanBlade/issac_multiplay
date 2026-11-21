# 第5步：Player 旧成员逐行回映射

**后续实施（2026-11-09）**：正式Player源与生成已迁入版本47，见[实现说明](implementation-5-player.md)。本文件保留设计时点和证据；构建不读取设计JSON。Plus未知与ABI模型限制保持，实际采集／安装及最终显示缓存仍待后续步骤，没有提前测试。

日期：2026-11-09。完整行、原签名、来源和Entity交叉索引见 [player-member-map.json](player-member-map.json)。配套 [原生字节账本](player-byte-ledger.md)，P0—P8裁剪仍以三份定稿为准。这里只完成设计登记，没有生成Player schema、运行代码、构建或测试；布局版本仍46。

## 1. 范围和行身份

旧Player应用目录1265行全部保留：705行ReplicatedState、523行AuthorityOnly、37行NotReplicated。原生补充14行、旧具名字段宏77行和结构6行另列。Entity应用目录1793行逐行建立交叉索引，其中1244行精确匹配Player应用签名，其余549行属于其他实体范围；EntityPlayer没有落在交叉索引外的行。

行主键为目录路径＋一基source_ordinal。ID不是主键；parameters的null、空白、默认参数及returns保持原样。原EntityPlayer状态295行实际286个ID；旧Captured为195行／188个ID。BabySkin、LaserColor、GetCollectibleNum、HasCollectible、GetPocketItem、GetTearsCap和服装查询重载等不得按ID去重。旧目录及其失败／实施记录不回写。

ClientPlayerDisplayView::Captured当前只找到声明和定义，ClientDescribePlayerDisplay仍建立泛化queries_表，未找到调用方。这个事实只能证明旧入口不是必要消费者，不能否认原生Render里的真实查询。556行旧只读查询不整体下行，也不靠恢复全量模拟字段使其可任意执行。

## 2. 真正的同步边界

| 类别 | 本轮决定 |
|---|---|
| P0—P8字段／容器 | 按已核显示输入保留；公开getter不会扩大字段范围 |
| 继承E层／ANM2／LayerState等 | 复用已核字段和资源；辅助类型嵌于Host对象时不能反向恢复它 |
| 配置、帧元数据、颜色常量 | 本地匹配资源；进度解锁、可用性、池权重留Host |
| 玩法查询／生产计数／保存与恢复 | 留Host；已产生实体、最终ANM与HUD各自承接显示 |
| 本地值操作／类型转换／owner安装／绘制 | 不是网络操作重放；仍须遵守线程、生命周期与副作用隔离 |
| 最终显示缓存 | 第8步实际PlayerHUD、StatHUD、HistoryHUD、inventory与crafting结果；不恢复其全套生产输入 |
| 有副作用风险的显示查询 | 只有真实具名绘制消费者保留最终决定；不是任意getter结果表 |

## 3. 重载、部分投影和几个易混项

- P1为CanFly@1564的1B；move/head direction留Host，六项玩法属性留Host，StatHUD实际五值缓存由第8步承接。TearRange同时属于攻击模板也不因此恢复整套属性。
- P2字段是当前身体存储，不承诺所有getter的身体重定向／回调语义。GetHealthType不是Plus.previousHealthType；健康回调、重入标志与cache map不下行，最终心格按第8步安装。
- P3不发送innate分组名称和定时历史，因此带GroupKey的公共API并不完整可复原。GetWildCardItem/Type是未来Use历史，留Host；不能因名称像“当前道具”而加入P3。
- PocketItem.GetSlot实际返回_id，active类型为ActiveSlot+1，绝非口袋位置。空ID可残留旧type，P3保留；这与P4空Item时其余五值规范零不同。
- QueueItemData只保留实际queued饰品配置和golden，Charge/Touched留Host。Weapon只按具体类型和公式投影；GetMainEntity仅Whip15 +44，不是base+8通用链接。
- P8保留disabled／有序效果config和count，只有704保留cooldown。GetEffects/List可返回本地列表，但不承诺其他cooldown；禁止用+2E94待移除队列替代当前效果。
- P7独立资格、twin/backup/held和C/S组保留；sub/replaced、原版四备份槽、设备索引不写Client。原版通信域+268C及其两份Sprite不并入P0，也不复用原版Online输入。

## 4. 真实显示查询的最终决定合同

P3当前输入足以承载已选择的库存状态，但不保证任意HasCollectible/GetCollectibleNum/饰品查询都纯函数。原生实现含进度、原版Online及局部RNG分支；不能为了一个Render判断额外调用Host getter，也不能让Client进入这些分支。

设计上由第8步具名Presentation adapter承接正常Host原生显示调用已经产生的bool/int/StateRef；第10步仅复制自有缓冲。固定adapter携带subject/use/shape、统一绘制拓扑和显示批次对应的状态／资源版本，参数只来自已声明消费者。未产出保持缺席，不填false/0，不借另一身体或旧批次。返回指针先转合法StateRef。Client只在批准的绘制上下文消费对应结果；无任意参数字典、无调试query日志、无逐consumer全图二读。具体最终缓存布局和批次封装仍由第8/9步实施，Authority tick不等待显示批次。

| 已核消费者／位置（VA） | 已登记结果来源与边界 |
|---|---|
| 839A80路径的GetCollectibleNum(251) | 正常769E90显示调用实际返回；仅确切签名和参数，不开放所有ID |
| 62C0C0／64E3A0／7C42D0，倍率141/156 | 当前正常显示倍率返回；内部递归21/64/75不是新的公共查询集 |
| 7844C8／786614 → 783390 IsHeadless | P8四个效果判断已有输入；HasCollectible206必须用已产出显示决定或整体headless结果，不重跑7699D0 |
| P3—P5文中列明的其余实际HasCollectible调用 | 按实际调用点和常量逐一绑定；闭包候选集不等于全API放行 |
| 9950B0／996930相关跨身体显示路径 | 只接实际bool/int/first-owner结果；最后者转P7合法身份，不传Host指针或设备键 |

这些都是新设计合同，未宣称已实现hook或缓存。IsInvisible在7B9E80读取P8效果／disabled和角色35，可由已安装投影按原分支派生；IsHologram在7D1C90读取+172资格、29/38、twin和索引，可由P7派生，无需再发两项结果。GetFlyingOffset在77B890先读Room.type==16分支，再读CanFly（784B44绘制调用）；依赖同版本Room.type和本地正常初始化的共享默认向量，不新增值。GetTotalActiveCharge在7895D0还按角色18/36和ItemConfig读取P2魂／血充能，不是仅P4普通电量相加；这些输入已有明确owner。GetBodyMoveDirection在772300读取设备/更新输入，GetDeathAnimName在77BB50选择动画，二者留Host，不用它们重建P0姿态。

## 5. 旧原生补充与结构

| 原生行 | 真实范围／去向 |
|---|---|
| 1 NativePlayer.inline_1 | 0x530 → P0：13份内嵌ANM2的物理槽1；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 2 NativePlayer.inline_2 | 0x644 → P0：13份内嵌ANM2的物理槽2；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 3 NativePlayer.inline_3 | 0x758 → P0：13份内嵌ANM2的物理槽3；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 4 NativePlayer.inline_4 | 0x86C → P0：13份内嵌ANM2的物理槽4；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 5 NativePlayer.inline_5 | 0x980 → P0：13份内嵌ANM2的物理槽5；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 6 NativePlayer.inline_6 | 0xA94 → P0：13份内嵌ANM2的物理槽6；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 7 NativePlayer.inline_7 | 0xBA8 → P0：13份内嵌ANM2的物理槽7；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 8 NativePlayer.inline_8 | 0xCBC → P0：13份内嵌ANM2的物理槽8；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 9 NativePlayer.inline_9 | 0xDD0 → P0：13份内嵌ANM2的物理槽9；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 10 NativePlayer.blood_gush | 0xEE4 → P0：13份内嵌ANM2的物理槽10；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 11 NativePlayer.held | 0xFF8 → P0：13份内嵌ANM2的物理槽11；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 12 NativePlayer.inline_12 | 0x110C → P0：13份内嵌ANM2的物理槽12；资源／姿态按各自owner投影，body槽0由公开GetBodySprite覆盖；旧inline_1不是槽0。 |
| 13 NativePlayer.RecordSprite | 0x1C50 → P8：1C50 ANM2＋当前可绘节点；独立可选world绘制序列，不并入P0的13份；不发送全队列或推进cursor。 |
| 14 NativePlayer.DynamicHUD | 0x20A8/0x20AC → 最终显示缓存：第8步PlayerHUD；20A8/20AC本地强pair；传实际HUD最终状态，原生地址与reference-count控制块本地。 |

| 结构行 | 去向 |
|---|---|
| roster_order | P7：roster_rank／C/S group ordered refs；保留真实顺序与成员资格，按普通结构事务提交。 |
| body_membership | P7：qualification／room／render／update＋C/S组／body_role；保留独立资格，Exists/active/本地构造事实各有职责。 |
| backup_slots | Host：原版EsauJr四槽；不是实际绘制必需的C/S关系，不安装完整保存体。 |
| equipment_slots | P3/P4/P5：pocket4／active4／weapon5；保留物理槽序与各自空值规则，不运行装备动作。 |
| private_presentation | P0/P8/第8步：内嵌ANM2／当前记录序列／最终HUD；按真实显示owner拆分；原版通信气泡不并入。 |
| native_membership | P7（裁剪）：资格／成员＋真实twin/backup/held；native sub/replaced四槽只是Host发现／恢复；不沿用旧四storage links全写。 |

## 6. 77个旧具名字段宏

宏顺序及原表达式在JSON保留。这里逐行指回字节账本；保留旧writer事实不表示继续发送该字段。

| 行／ID | 原生位置／范围 | 归属／去向 |
|---|---|---|
| 1 EntityPlayer.Damage | 0x146C _damage | 权威；Host模拟／生产／恢复；不下行 |
| 2 EntityPlayer.MoveSpeed | 0x155C _movespeed | 权威；Host模拟／生产／恢复；不下行 |
| 3 EntityPlayer.MaxFireDelay | 0x145C _maxfiredelay | 权威；Host模拟／生产／恢复；不下行 |
| 4 EntityPlayer.ShotSpeed | 0x1460 _shotspeed | 权威；Host模拟／生产／恢复；不下行 |
| 5 EntityPlayer.TearRange | 0x147C _tearrange | 权威；Host模拟／生产／恢复；不下行 |
| 6 EntityPlayer.TearHeight | 0x1470 _tearheight | 权威；Host模拟／生产／恢复；不下行 |
| 7 EntityPlayer.TearFallingSpeed | 0x1474 _tearfallspeed | 权威；Host模拟／生产／恢复；不下行 |
| 8 EntityPlayer.TearFallingAcceleration | 0x1478 _tearfallaccel | 权威；Host模拟／生产／恢复；不下行 |
| 9 EntityPlayer.Luck | 0x1560 _luck | 权威；Host模拟／生产／恢复；不下行 |
| 10 EntityPlayer.CanFly | 0x1564 _canFly | 状态；P1.can_fly |
| 11 EntityPlayer.GetMovementDirection | 0x1618 _moveDirection | 权威；Host模拟／生产／恢复；不下行 |
| 12 EntityPlayer.GetHeadDirection | 0x161C _headDirection | 权威；Host模拟／生产／恢复；不下行 |
| 13 EntityPlayer.GetDamageCooldown | 0x13B8 _damageCooldown | 状态；P0.damage_cooldown |
| 14 NativePlayer.BlinkTime | 0x1338 _blinkTime | 权威；Host模拟／生产／恢复；不下行 |
| 15 NativePlayer.HeadDirectionTime | 0x133C _headDirectionTime | 权威；Host模拟／生产／恢复；不下行 |
| 16 EntityPlayer.GetMaxHearts | 0x1340 _maxHearts | 状态；P2.max_hearts |
| 17 EntityPlayer.GetHearts | 0x1344 _redHearts | 状态；P2.red_hearts |
| 18 EntityPlayer.GetEternalHearts | 0x1348 _eternalHearts | 权威；Host模拟／生产／恢复；不下行 |
| 19 EntityPlayer.GetSoulHearts | 0x134C _soulHearts | 状态；P2.soul_hearts |
| 20 EntityPlayer.GetBlackHearts | 0x1350 _blackHearts | 状态；P2.black_hearts |
| 21 EntityPlayer.GetGoldenHearts | 0x1938 _goldenHearts | 状态；P2.golden_hearts |
| 22 EntityPlayer.GetBoneHearts | 0x1D74 _boneHearts | 状态；P2.bone_hearts |
| 23 NativePlayer.BoneHeartsMask | 0x1D78 _boneHeartsMask | 权威；Host模拟／生产／恢复；不下行 |
| 24 EntityPlayer.GetBrokenHearts | 0x1D8C _brokenHearts | 状态；P2.broken_hearts |
| 25 EntityPlayer.GetRottenHearts | 0x1D90 _rottenHearts | 状态；P2.rotten_hearts |
| 26 EntityPlayer.IsCoopGhost | 0x207D _isCoopGhost | 状态；P2.coop_ghost |
| 27 EntityPlayer.GetNumCoins | 0x1368 _numCoins | 状态；P2.coins |
| 28 EntityPlayer.GetNumKeys | 0x135C _numKeys | 状态；P2.keys |
| 29 EntityPlayer.GetNumBombs | 0x1364 _numBombs | 状态；P2.bombs |
| 30 EntityPlayer.HasGoldenKey | 0x1360 _hasGoldenKey | 状态；P2.golden_key |
| 31 EntityPlayer.HasGoldenBomb | 0x1361 _hasGoldenBomb | 状态；P2.golden_bomb |
| 32 EntityPlayer.GetNumGigaBombs | 0x1DC0 _numGigaBombs | 状态；P2.giga_bombs |
| 33 EntityPlayer.GetUrnSouls | 0x1DBC _urnSouls | 状态；P2.urn_souls |
| 34 EntityPlayer.GetPoopMana | 0x1F30 _poopMana | 状态；P2.poop_mana |
| 35 EntityPlayer.GetRedStewBonusDuration | 0x1E58 _redStewBonusDuration | 权威；Host模拟／生产／恢复；不下行 |
| 36 EntityPlayer.GetEdenSpeed | 0x1540 _edenSpeed | 权威；Host模拟／生产／恢复；不下行 |
| 37 EntityPlayer.GetEdenFireDelay | 0x1544 _edenFireDelay | 权威；Host模拟／生产／恢复；不下行 |
| 38 EntityPlayer.GetEdenDamage | 0x1548 _edenDamage | 权威；Host模拟／生产／恢复；不下行 |
| 39 EntityPlayer.GetEdenRange | 0x154C _edenTearRange | 权威；Host模拟／生产／恢复；不下行 |
| 40 EntityPlayer.GetEdenShotSpeed | 0x1550 _edenShotSpeed | 权威；Host模拟／生产／恢复；不下行 |
| 41 EntityPlayer.GetEdenLuck | 0x1554 _edenLuck | 权威；Host模拟／生产／恢复；不下行 |
| 42 EntityPlayer.GetD8DamageModifier | 0x1928 _d8DamageModifier | 权威；Host模拟／生产／恢复；不下行 |
| 43 EntityPlayer.GetD8SpeedModifier | 0x192C _d8SpeedModifier | 权威；Host模拟／生产／恢复；不下行 |
| 44 EntityPlayer.GetD8RangeModifier | 0x1930 _d8RangeModifier | 权威；Host模拟／生产／恢复；不下行 |
| 45 EntityPlayer.GetD8FireDelayModifier | 0x1934 _d8FireDelayModifier | 权威；Host模拟／生产／恢复；不下行 |
| 46 EntityPlayer.GetRockBottomSpeed | 0x1E94 _rockBottomMoveSpeed | 权威；Host模拟／生产／恢复；不下行 |
| 47 EntityPlayer.GetRockBottomFireDelay | 0x1E98 _rockBottomMaxFireDelay | 权威；Host模拟／生产／恢复；不下行 |
| 48 EntityPlayer.GetRockBottomDamage | 0x1E9C _rockBottomDamage | 权威；Host模拟／生产／恢复；不下行 |
| 49 EntityPlayer.GetRockBottomRange | 0x1EA0 _rockBottomTearRange | 权威；Host模拟／生产／恢复；不下行 |
| 50 EntityPlayer.GetRockBottomShotSpeed | 0x1EA4 _rockBottomShotSpeed | 权威；Host模拟／生产／恢复；不下行 |
| 51 EntityPlayer.GetRockBottomLuck | 0x1EA8 _rockBottomLuck | 权威；Host模拟／生产／恢复；不下行 |
| 52 NativePlayer.ItemStateCooldown | 0x15F8 _itemStateCooldown | 权威；Host模拟／生产／恢复；不下行 |
| 53 NativePlayer.BombPlaceDelay | 0x1604 _bombPlaceDelay | 权威；Host模拟／生产／恢复；不下行 |
| 54 NativePlayer.ForgottenSwapCooldown | 0x1608 _forgottenSwapFormCooldown | 权威；Host模拟／生产／恢复；不下行 |
| 55 NativePlayer.CanShoot | 0x1811 _canShoot | 权威；Host模拟／生产／恢复；不下行 |
| 56 NativePlayer.MaggyDrainCooldown | 0x1ED0 _maggyHealthDrainCooldown | 权威；Host模拟／生产／恢复；不下行 |
| 57 NativePlayer.MaggySwingCooldown | 0x1ED4 _maggySwingCooldown | 权威；Host模拟／生产／恢复；不下行 |
| 58 NativePlayer.EveSumptoriumCharge | 0x1F58 _eveSumptoriumCharge | 状态；P0.eve_charge |
| 59 EntityPlayer.GetSoulCharge | 0x1D94 soul_charge | 状态；P2.soul_charge |
| 60 EntityPlayer.GetBloodCharge | 0x1D98 blood_charge | 状态；P2.blood_charge |
| 61 EntityPlayer.TearFlags | 0x1480 tear_flags | 权威；Host；不传 |
| 62 EntityPlayer.TearColor | 0x1490 tear_color | 权威；Host；最终实体颜色各自同步 |
| 63 EntityPlayer.LaserColor | 0x14BC _laserColor | 权威；Host模拟／生产／恢复；不下行 |
| 64 NativePlayer.CraftingUseCharge | 0x1F54 crafting_use_charge | 状态；P0.crafting_use_charge |
| 65 EntityPlayer.SamsonBerserkCharge | 0x2070 samson_and_ibs_charge | 权威；Host；最终Sprite／攻击实体承接 |
| 66 EntityPlayer.IBSCharge | 0x2074 samson_and_ibs_charge | 权威；Host；最终Sprite／攻击实体承接 |
| 67 NativePlayer.MawCharge | 0x18A8 _mawOfTheVoidChargeTimer | 状态；P0.maw_charge |
| 68 NativePlayer.RevelationCharge | 0x1EC0 _revelationChargeTimer | 状态；P0.revelation_charge |
| 69 NativePlayer.MontezumaCharge | 0x1EC4 _montezumaChargeTimer | 状态；P0.montezuma_charge |
| 70 NativePlayer.FullSpriteA | 0x1398 full_sprite_a | 状态；P0.full_sprite_a |
| 71 NativePlayer.FullSpriteB | 0x139A full_sprite_b | 状态；P0.full_sprite_b |
| 72 NativePlayer.BodyScaleX | 0x14E8 render_scales | 状态；P0六比例 |
| 73 NativePlayer.BodyScaleY | 0x14EC render_scales | 状态；P0六比例 |
| 74 NativePlayer.BodyLayerScaleX | 0x14F0 render_scales | 状态；P0六比例 |
| 75 NativePlayer.BodyLayerScaleY | 0x14F4 render_scales | 状态；P0六比例 |
| 76 NativePlayer.HeadLayerScaleX | 0x14F8 render_scales | 状态；P0六比例 |
| 77 NativePlayer.HeadLayerScaleY | 0x14FC render_scales | 状态；P0六比例 |

## 7. 全部公开应用行

以下按旧目录一基序号保留1265行，参数与返回值在JSON逐字保留。Host判定仅针对当前声明的原版显示／纯界面适配范围，不证明所有Mod无消费者；新增消费者要在它自己的owner明确需求，不自动恢复整个公共API。

### ActiveItemDesc

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1 ActiveItemDesc.BatteryCharge | P4：ActiveItemDesc.battery_charge | 六个展示值，按P4空槽规范化与四槽顺序安装。 |
| 2 ActiveItemDesc.Charge | P4：ActiveItemDesc.charge | 六个展示值，按P4空槽规范化与四槽顺序安装。 |
| 3 ActiveItemDesc.Item | P4：ActiveItemDesc.item_id | 六个展示值，按P4空槽规范化与四槽顺序安装。 |
| 4 ActiveItemDesc.PartialCharge | P4：ActiveItemDesc.partial_charge | 六个展示值，按P4空槽规范化与四槽顺序安装。 |
| 5 ActiveItemDesc.SubCharge | P4：ActiveItemDesc.sub_charge | 六个展示值，按P4空槽规范化与四槽顺序安装。 |
| 6 ActiveItemDesc.TimedRechargeCooldown | Host：P4排除+14调度冷却 | 到期充能生产留Host；P4只传六值。 |
| 7 ActiveItemDesc.VarData | P4：ActiveItemDesc.var_data | 六个展示值，按P4空槽规范化与四槽顺序安装。 |

### AnimationData

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 8 AnimationData.GetAllLayers | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 9 AnimationData.GetLayer | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 10 AnimationData.GetLength | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 11 AnimationData.GetName | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 12 AnimationData.IsLoopingAnimation | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |

### AnimationFrame

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 13 AnimationFrame.GetColor | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 14 AnimationFrame.GetCrop | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 15 AnimationFrame.GetEndFrame | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 16 AnimationFrame.GetHeight | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 17 AnimationFrame.GetPivot | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 18 AnimationFrame.GetPos | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 19 AnimationFrame.GetRotation | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 20 AnimationFrame.GetScale | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 21 AnimationFrame.GetStartFrame | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 22 AnimationFrame.GetWidth | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 23 AnimationFrame.IsInterpolated | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 24 AnimationFrame.IsVisible | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |

### AnimationLayer

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 25 AnimationLayer.GetFrame | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 26 AnimationLayer.GetLayerID | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 27 AnimationLayer.IsVisible | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |

### BitSet128

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 28 BitSet128.BitSet128 | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 29 BitSet128.Get | 上下文派生：拥有者已声明128位值的低／高64位 | 仅所属状态已保留时投影；Player攻击TearFlags留Host，不能由辅助类型反向恢复。 |
| 30 BitSet128.Set | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 31 BitSet128.__band | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 32 BitSet128.__bnot | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 33 BitSet128.__bor | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 34 BitSet128.__bxor | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 35 BitSet128.__eq | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 36 BitSet128.__le | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 37 BitSet128.__lt | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 38 BitSet128.__shl | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 39 BitSet128.__shr | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 40 BitSet128.__tostring | 本地值操作：BitSet128值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 41 BitSet128.h | 上下文派生：拥有者已声明128位值的低／高64位 | 仅所属状态已保留时投影；Player攻击TearFlags留Host，不能由辅助类型反向恢复。 |
| 42 BitSet128.l | 上下文派生：拥有者已声明128位值的低／高64位 | 仅所属状态已保留时投影；Player攻击TearFlags留Host，不能由辅助类型反向恢复。 |

### BlendMode

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 43 BlendMode.AlphaDestinationFactor | 上下文投影/派生：alpha_destination_factor | Alpha 目标因子 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 44 BlendMode.AlphaSourceFactor | 上下文投影/派生：alpha_source_factor | Alpha 源因子 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 45 BlendMode.Equation | 上下文投影/派生：equation | 混合方程 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 46 BlendMode.New | 本地值操作：BlendMode值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 47 BlendMode.NewFromType | 本地值操作：BlendMode值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 48 BlendMode.RGBDestinationFactor | 上下文投影/派生：rgb_destination_factor | RGB 目标因子 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 49 BlendMode.RGBSourceFactor | 上下文投影/派生：rgb_source_factor | RGB 源因子 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 50 BlendMode.SetMode | 本地值操作：BlendMode值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |

### Color

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 51 Color.A | 上下文投影/派生：tint_a | Color 实例 Alpha 对应 tint[3]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 52 Color.B | 上下文投影/派生：tint_b | Color 实例 Blue 对应 tint[2]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 53 Color.BO | 上下文投影/派生：offset_b | Color 实例 Blue Offset 对应 offset[2]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 54 Color.Color | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 55 Color.Color | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 56 Color.EmberFade | 本地常量：Color.EmberFade | 全局具名常量不是对象实例状态。 |
| 57 Color.G | 上下文投影/派生：tint_g | Color 实例 Green 对应 tint[1]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 58 Color.GO | 上下文投影/派生：offset_g | Color 实例 Green Offset 对应 offset[1]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 59 Color.GetColorize | 上下文投影/派生：colorize_r,colorize_g,colorize_b,colorize_a | 官方 GetColorize 将 _colorize[0..3] 暴露为 RGBA。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 60 Color.GetOffset | 上下文投影/派生：offset_r,offset_g,offset_b | 官方 GetOffset 将 _offset[0..2] 暴露为 RGB。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 61 Color.GetTint | 上下文投影/派生：tint_r,tint_g,tint_b,tint_a | 官方 GetTint 将 _tint[0..3] 暴露为 RGBA。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 62 Color.LaserAlmond | 本地常量：Color.LaserAlmond | 全局具名常量不是对象实例状态。 |
| 63 Color.LaserChocolate | 本地常量：Color.LaserChocolate | 全局具名常量不是对象实例状态。 |
| 64 Color.LaserCoal | 本地常量：Color.LaserCoal | 全局具名常量不是对象实例状态。 |
| 65 Color.LaserFireMind | 本地常量：Color.LaserFireMind | 全局具名常量不是对象实例状态。 |
| 66 Color.LaserHoming | 本地常量：Color.LaserHoming | 全局具名常量不是对象实例状态。 |
| 67 Color.LaserIpecac | 本地常量：Color.LaserIpecac | 全局具名常量不是对象实例状态。 |
| 68 Color.LaserMother | 本地常量：Color.LaserMother | 全局具名常量不是对象实例状态。 |
| 69 Color.LaserNumberOne | 本地常量：Color.LaserNumberOne | 全局具名常量不是对象实例状态。 |
| 70 Color.LaserPoison | 本地常量：Color.LaserPoison | 全局具名常量不是对象实例状态。 |
| 71 Color.LaserSoy | 本地常量：Color.LaserSoy | 全局具名常量不是对象实例状态。 |
| 72 Color.Lerp | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 73 Color.Print | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 74 Color.ProjectileCageBlue | 本地常量：Color.ProjectileCageBlue | 全局具名常量不是对象实例状态。 |
| 75 Color.ProjectileCorpseClusterDark | 本地常量：Color.ProjectileCorpseClusterDark | 全局具名常量不是对象实例状态。 |
| 76 Color.ProjectileCorpseClusterLight | 本地常量：Color.ProjectileCorpseClusterLight | 全局具名常量不是对象实例状态。 |
| 77 Color.ProjectileCorpseGreen | 本地常量：Color.ProjectileCorpseGreen | 全局具名常量不是对象实例状态。 |
| 78 Color.ProjectileCorpsePink | 本地常量：Color.ProjectileCorpsePink | 全局具名常量不是对象实例状态。 |
| 79 Color.ProjectileCorpseWhite | 本地常量：Color.ProjectileCorpseWhite | 全局具名常量不是对象实例状态。 |
| 80 Color.ProjectileCorpseYellow | 本地常量：Color.ProjectileCorpseYellow | 全局具名常量不是对象实例状态。 |
| 81 Color.ProjectileFireWave | 本地常量：Color.ProjectileFireWave | 全局具名常量不是对象实例状态。 |
| 82 Color.ProjectileHoming | 本地常量：Color.ProjectileHoming | 全局具名常量不是对象实例状态。 |
| 83 Color.ProjectileHushBlue | 本地常量：Color.ProjectileHushBlue | 全局具名常量不是对象实例状态。 |
| 84 Color.ProjectileHushGreen | 本地常量：Color.ProjectileHushGreen | 全局具名常量不是对象实例状态。 |
| 85 Color.ProjectileHushYellow | 本地常量：Color.ProjectileHushYellow | 全局具名常量不是对象实例状态。 |
| 86 Color.ProjectileIpecac | 本地常量：Color.ProjectileIpecac | 全局具名常量不是对象实例状态。 |
| 87 Color.ProjectileMegaSatanBlack | 本地常量：Color.ProjectileMegaSatanBlack | 全局具名常量不是对象实例状态。 |
| 88 Color.ProjectileMegaSatanWhite | 本地常量：Color.ProjectileMegaSatanWhite | 全局具名常量不是对象实例状态。 |
| 89 Color.ProjectileSoy | 本地常量：Color.ProjectileSoy | 全局具名常量不是对象实例状态。 |
| 90 Color.ProjectileTar | 本地常量：Color.ProjectileTar | 全局具名常量不是对象实例状态。 |
| 91 Color.R | 上下文投影/派生：tint_r | Color 实例 Red 对应 tint[0]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 92 Color.RO | 上下文投影/派生：offset_r | Color 实例 Red Offset 对应 offset[0]。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 93 Color.Reset | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 94 Color.SetColorize | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 95 Color.SetOffset | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 96 Color.SetTint | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 97 Color.TearAlmond | 本地常量：Color.TearAlmond | 全局具名常量不是对象实例状态。 |
| 98 Color.TearChocolate | 本地常量：Color.TearChocolate | 全局具名常量不是对象实例状态。 |
| 99 Color.TearCoal | 本地常量：Color.TearCoal | 全局具名常量不是对象实例状态。 |
| 100 Color.TearCommonCold | 本地常量：Color.TearCommonCold | 全局具名常量不是对象实例状态。 |
| 101 Color.TearHoming | 本地常量：Color.TearHoming | 全局具名常量不是对象实例状态。 |
| 102 Color.TearIpecac | 本地常量：Color.TearIpecac | 全局具名常量不是对象实例状态。 |
| 103 Color.TearNumberOne | 本地常量：Color.TearNumberOne | 全局具名常量不是对象实例状态。 |
| 104 Color.TearScorpio | 本地常量：Color.TearScorpio | 全局具名常量不是对象实例状态。 |
| 105 Color.TearSerpentsKiss | 本地常量：Color.TearSerpentsKiss | 全局具名常量不是对象实例状态。 |
| 106 Color.TearSoy | 本地常量：Color.TearSoy | 全局具名常量不是对象实例状态。 |
| 107 Color.TearTar | 本地常量：Color.TearTar | 全局具名常量不是对象实例状态。 |
| 108 Color.__mul | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 109 Color.__tostring | 本地值操作：Color值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |

### CostumeSpriteDesc

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 110 CostumeSpriteDesc.CanOverwriteColor | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 111 CostumeSpriteDesc.GetDefaultSkinColor | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 112 CostumeSpriteDesc.GetItemConfig | P6：costume config／isFlying／hasOverlay | 实际绘制使用的目录与分支输入。 |
| 113 CostumeSpriteDesc.GetPlayerType | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 114 CostumeSpriteDesc.GetPriority | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 115 CostumeSpriteDesc.GetSkinColor | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 116 CostumeSpriteDesc.GetSprite | P0：外部costume元素ANM2 | 每个元素独立资源／姿态，结构由P6提交。 |
| 117 CostumeSpriteDesc.HasOverlay | P6：costume config／isFlying／hasOverlay | 实际绘制使用的目录与分支输入。 |
| 118 CostumeSpriteDesc.HasSkinAlt | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 119 CostumeSpriteDesc.IsFlying | P6：costume config／isFlying／hasOverlay | 实际绘制使用的目录与分支输入。 |
| 120 CostumeSpriteDesc.IsItemAnimPlaying | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |
| 121 CostumeSpriteDesc.IsItemStateOnly | Host：服装排序／生成／颜色选择元数据 | P6最终map与P0姿态已经表达结果；不传priority、playerType、skin或itemAnim管理字段。 |

### EffectList

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 122 EffectList.Get | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 123 EffectList.Size | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 124 EffectList.__len | 本地派生：P8列表长度 | 长度来自已安装有序列表，不新增网络命令。 |

### Entity

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 125 Entity.AddBaited | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 126 Entity.AddBleeding | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 127 Entity.AddBrimstoneMark | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 128 Entity.AddBurn | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 129 Entity.AddBurn | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 130 Entity.AddCharmed | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 131 Entity.AddCharmed | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 132 Entity.AddConfusion | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 133 Entity.AddEntityFlags | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 134 Entity.AddFear | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 135 Entity.AddFear | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 136 Entity.AddFreeze | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 137 Entity.AddFreeze | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 138 Entity.AddHealth | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 139 Entity.AddIce | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 140 Entity.AddKnockback | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 141 Entity.AddMagnetized | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 142 Entity.AddMidasFreeze | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 143 Entity.AddMidasFreeze | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 144 Entity.AddPoison | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 145 Entity.AddPoison | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 146 Entity.AddShrink | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 147 Entity.AddShrink | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 148 Entity.AddSlowing | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 149 Entity.AddSlowing | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 150 Entity.AddVelocity | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 151 Entity.AddVelocity | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 152 Entity.AddWeakness | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 153 Entity.BloodExplode | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 154 Entity.CanDevolve | Host：该类型已核生产域 | Host敌人退化玩法资格；Client只安装Host确定的成员形态，不在本地决定退化。 |
| 155 Entity.CanShutDoors | Host：该类型已核生产域 | Host房门关闭玩法资格；Client安装Room域确定的门状态，不根据本地Entity查询决定开关门。 |
| 156 Entity.Child | 继承E层：child | 对应原生+0x3C0的EntityPtr child；转成员序号发送，目标不在成员表时报缺口，不发送地址。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 157 Entity.ClearEntityFlags | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 158 Entity.CollidesWithGrid | Host：collides_with_grid | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 159 Entity.CollisionDamage | Host：collision_damage | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 160 Entity.Color | 继承E层：color | 直接对应原生+0x1B8的color，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 161 Entity.ComputeStatusEffectDuration | Host：该类型已核生产域 | 依赖调用时长与EntityRef参数的Host状态效果规则计算；不重演玩法查询。 |
| 162 Entity.CopyStatusEffects | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 163 Entity.DepthOffset | 继承E层：depth_offset | 直接对应原生+0x358的depth_offset，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 164 Entity.Die | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 165 Entity.DropSeed | Host：该类型已核生产域 | Host掉落RNG种子/游标；RNG字节归权威，不在Client复制或推进。 |
| 166 Entity.EntityCollisionClass | 继承E层：entity_collision_class | 直接对应原生+0x188的entity_collision_class，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 167 Entity.Exists | 继承E层：exists | 直接对应原生+0x172的exists，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 168 Entity.FlipX | 继承E层：sprite#0 | 主Sprite公开别名，字节在内嵌ANM2的flip_x（Entity +0x148）；随sprite#0外观子块发送，不重复发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 169 Entity.ForceCollide | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 170 Entity.FrameCount | 继承E层：frame_base,@world_game_frame | Host帧空间内作模2^32减法；展示时钟只取最近已应用world帧。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 171 Entity.Friction | Host：friction | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 172 Entity.GetBaitedCountdown | Host：baited_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 173 Entity.GetBleedingCountdown | Host：bleeding_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 174 Entity.GetBossID | 继承E层：selected_config | J273 VA0x7DF5F0：实际selected_config为空返回0，否则取配置+0x30 BossID；不能只用Entity.Type代替。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 175 Entity.GetBossStatusEffectCooldown | Host：boss_status_effect_cooldown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 176 Entity.GetBrimstoneMarkCountdown | Host：brimstone_mark_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 177 Entity.GetBurnCountdown | Host：burn_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 178 Entity.GetBurnDamageTimer | Host：burn_damage_timer | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 179 Entity.GetCharmedCountdown | Host：charmed_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 180 Entity.GetCollisionCapsule | Host/本地：碰撞／调试／AI查询 | 无本轮声明的Player显示消费者；不为这些查询补模拟状态。 |
| 181 Entity.Color | 继承E层：color | 直接对应原生+0x1B8的color，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 182 Entity.GetColorParams | Host：该类型已核生产域 | Host颜色覆盖队列；最终Entity.Color和主Sprite.color已同步，Client不重演颜色合成或还原该队列。版本32见docs/archive/design/binary-state/animation-resource-synchronization-review.md |
| 183 Entity.GetConfusionCountdown | Host：confusion_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 184 Entity.GetDamageCountdown | Host：damage_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 185 Entity.GetData | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 186 Entity.GetDebugShape | Host/本地：碰撞／调试／AI查询 | 无本轮声明的Player显示消费者；不为这些查询补模拟状态。 |
| 187 Entity.GetDropRNG | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 188 Entity.GetEntityConfigEntity | 继承E层：type,variant,subtype,@entity_config_table | 官方LuaEntityConfig.cpp:9-27按当前三形态字段查Manager配置，保留Null；与+330实际选中配置指针区分。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 189 Entity.GetEntityFlags | 继承E层：flags | 直接对应原生+0x168的flags，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 190 Entity.GetFearCountdown | Host：fear_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 191 Entity.GetFireDamageCooldown | Host：fire_damage_cooldown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 192 Entity.GetFreezeCountdown | Host：freeze_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 193 Entity.GetHitListIndex | Host：该类型已核生产域 | Host碰撞系统的本地hit-list索引；Client不运行碰撞模拟，也不把它当网络身份。 |
| 194 Entity.GetIceCountdown | Host：ice_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 195 Entity.GetKnockbackCountdown | Host：knockback_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 196 Entity.GetKnockbackDirection | Host：knockback_direction | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 197 Entity.GetLastChild | 继承E层：child,@entity_member_graph | J273 0x6A4030沿引用链到末端，空首边返回SELF；Wiki写nil与静态getter冲突，留待第15步对照；不执行native遍历，缺边/环报缺口。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 198 Entity.GetLastParent | 继承E层：parent,@entity_member_graph | J273 0x6A4010沿引用链到末端，空首边返回SELF；Wiki写nil与静态getter冲突，留待第15步对照；不执行native遍历，缺边/环报缺口。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 199 Entity.GetMagnetizedCountdown | Host：magnetized_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 200 Entity.GetMidasFreezeCountdown | Host：midas_freeze_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 201 Entity.GetMinecart | 继承E层：minecart | 对应原生+0x3D4的EntityPtr minecart；转成员序号发送，目标不在成员表时报缺口，不发送地址。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 202 Entity.GetNullCapsule | 本地资源/派生：Entity最终Sprite＋本地NullFrame | 仅已声明绘制用途从最终姿态和匹配ANM资源计算。 |
| 203 Entity.GetNullOffset | 本地资源/派生：Entity最终Sprite＋本地NullFrame | 仅已声明绘制用途从最终姿态和匹配ANM资源计算。 |
| 204 Entity.GetPauseTime | Host：pause_timer | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。暂停期间仅同步最终位置与姿态。 |
| 205 Entity.GetPoisonCountdown | Host：poison_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 206 Entity.GetPoisonDamageTimer | Host：poison_damage_timer | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 207 Entity.GetPosVel | 继承E层：position,velocity | 官方LuaEntity.cpp:113只构建PosVel(*_pos,*_velocity)，无需独立查询或额外状态。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 208 Entity.GetPredictedTargetPosition | Host/本地：碰撞／调试／AI查询 | 无本轮声明的Player显示消费者；不为这些查询补模拟状态。 |
| 209 Entity.GetShadowSize | 继承E层：shadow_size | 直接对应原生+0x15C的shadow_size，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 210 Entity.GetShrinkCountdown | Host：shrink_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 211 Entity.GetSlowingCountdown | Host：slowing_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 212 Entity.GetSpeedMultiplier | 继承E层：speed_multiplier | 直接对应原生+0x39C的speed_multiplier，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 213 Entity.GetSprite | 继承E层：sprite#0,sprite#1,sprite#2 | 主Sprite的外观、动画和图层子块共同构成当前ANM2，不只回映外观。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 214 Entity.Type | 继承E层：type | 直接对应原生+0x28的type，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 215 Entity.GetWaterClipFlags | 继承E层：entity_plus,type,variant,subtype,spawner_type,sorting_layer,position_offset,hit_points,grid_collision_class,sprite#1,@water_clip_class_inputs,@room_water_clip | J273 VA0x6B2F70按分类/排序层/偏移Y/HP及子类/Game条件构造WaterClipInfo；REP只覆盖flags。明确依赖Pickup+534、Player道具62查询与Room helper等，后续域尚须接线，不能把最终flags冒充nullable override。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 216 Entity.GetWeaknessCountdown | Host：weakness_countdown | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 217 Entity.GiveMinecart | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 218 Entity.GridCollisionClass | 继承E层：grid_collision_class | 直接对应原生+0x184的grid_collision_class，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 219 Entity.HasCommonParentWithEntity | 本地派生（限已保留关系）：Entity parent图 | 只遍历已有StateRef图，不恢复被裁管理关系。 |
| 220 Entity.HasEntityFlags | 本地派生：Entity.flags | 按已发送位集合求交，不新增查询表。 |
| 221 Entity.HasFullHealth | 继承E层：hit_points,max_hit_points | J273 VA0x6A52B0/0x6A52B8直接比较HP>=MaxHP，已核movss/comiss；非有限值由状态校验拒绝。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 222 Entity.HasMortalDamage | Host：pending_damage | J273 VA0x6A8810/0x6A8815比较Host pending_damage+14>=HP+380；缓冲归Host伤害结算，Client不执行该玩法查询，不能仅用HP派生。 |
| 223 Entity.HitPoints | 继承E层：hit_points | 直接对应原生+0x380的hit_points，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 224 Entity.IgnoreEffectFromFriendly | Host：该类型已核生产域 | 依赖EntityRef参数和阵营规则的Host伤害/状态效果判定；Client不执行该玩法规则。 |
| 225 Entity.Index | 继承E层：authority_order | 保留Host绘制排序值，不把原生索引复用为网络身份。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 226 Entity.InitSeed | 继承E层：init_seed | 直接对应原生+0x3EC的init_seed，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 227 Entity.IsActiveEnemy | Host/本地：碰撞／调试／AI查询 | 无本轮声明的Player显示消费者；不为这些查询补模拟状态。 |
| 228 Entity.IsBoss | Host：该类型已核生产域 | Host Boss玩法分类；Client Boss HUD属于第8步Presentation域，不能据此声称Client查询API已重建。 |
| 229 Entity.IsDead | 继承E层：dead | 直接对应原生+0x173的dead，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 230 Entity.IsEnemy | Host：该类型已核生产域 | Host敌人分类玩法查询；Client不运行AI/战斗判定，表现使用服务器状态。 |
| 231 Entity.IsFlying | 继承E层：entity_plus,grid_collision_class | J273 VA0x6A7C90只读+184，返回值为0/1/2/3之一；REP EntityPlus nullable覆盖优先。主审已反编译，并读官方覆盖顺序。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 232 Entity.IsFrame | 本地派生：Entity frame_base＋world_game_frame | 使用现有帧合同，不复制GetFrame字典或推进实体Update。 |
| 233 Entity.IsInvincible | Host：invincible | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 234 Entity.IsVisible | 继承E层：visible | Visible的另一公开名；直接对应原生+0x171的visible，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 235 Entity.IsVulnerableEnemy | Host：该类型已核生产域 | Host可受击敌人玩法查询；Client不运行伤害规则，最终HP/死亡/表现由Host发布。 |
| 236 Entity.Kill | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 237 Entity.KillWithSource | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 238 Entity.MakeBloodPoof | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 239 Entity.MakeGroundPoof | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 240 Entity.Mass | Host：mass | 对应Host玩法状态，按版本30的Entity公共同步职责复核不传输、不安装，不要求Host与Client原值相等；字段证据和公开观测职责保留。 |
| 241 Entity.MaxHitPoints | 继承E层：max_hit_points | 直接对应原生+0x384的max_hit_points，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 242 Entity.MultiplyFriction | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 243 Entity.Parent | 继承E层：parent | 对应原生+0x3BC的EntityPtr parent；转成员序号发送，目标不在成员表时报缺口，不发送地址。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 244 Entity.Position | 继承E层：position | 直接对应原生+0x33C的position，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 245 Entity.PositionOffset | 继承E层：position_offset | 直接对应原生+0x34C的position_offset，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 246 Entity.PostRender | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 247 Entity.Remove | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 248 Entity.RemoveStatusEffects | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 249 Entity.Render | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 250 Entity.RenderShadowLayer | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 251 Entity.RenderZOffset | 继承E层：render_z_offset | 直接对应原生+0x354的render_z_offset，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 252 Entity.ResetWaterClipFlags | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 253 Entity.SetBaitedCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 254 Entity.SetBleedingCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 255 Entity.SetBossStatusEffectCooldown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 256 Entity.SetBrimstoneMarkCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 257 Entity.SetBurnCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 258 Entity.SetBurnDamageTimer | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 259 Entity.SetCharmedCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 260 Entity.SetColor | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 261 Entity.SetColorParams | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 262 Entity.SetConfusionCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 263 Entity.SetDamageCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 264 Entity.SetDead | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 265 Entity.SetFearCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 266 Entity.SetFireDamageCooldown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 267 Entity.SetFreezeCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 268 Entity.SetIceCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 269 Entity.SetInvincible | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 270 Entity.SetKnockbackCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 271 Entity.SetKnockbackDirection | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 272 Entity.SetMagnetizedCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 273 Entity.SetMidasFreezeCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 274 Entity.SetPauseTime | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 275 Entity.SetPoisonCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 276 Entity.SetPoisonDamageTimer | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 277 Entity.SetShadowSize | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 278 Entity.SetShrinkCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 279 Entity.SetSize | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 280 Entity.SetSlowingCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 281 Entity.SetSpeedMultiplier | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 282 Entity.SetSpriteFrame | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 283 Entity.SetSpriteOverlayFrame | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 284 Entity.SetWaterClipFlags | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 285 Entity.SetWeaknessCountdown | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 286 Entity.Size | 继承E层：size | 直接对应原生+0x370的size，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 287 Entity.SizeMulti | 继承E层：size_multi | 直接对应原生+0x374的size_multi，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 288 Entity.SortingLayer | 继承E层：sorting_layer | 直接对应原生+0x35C的sorting_layer，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 289 Entity.SpawnBloodEffect | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 290 Entity.SpawnGridIndex | 继承E层：spawn_grid_index | J273 getter直接读取+0x32C；-1为未绑定的原生状态，不作为网络身份。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 291 Entity.SpawnWaterImpactEffects | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 292 Entity.SpawnerEntity | 继承E层：spawner | 对应原生+0x3C8的EntityPtr spawner；转成员序号发送，目标不在成员表时报缺口，不发送地址。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 293 Entity.SpawnerType | 继承E层：spawner_type | 直接对应原生+0x34的spawner_type，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 294 Entity.SpawnerVariant | 继承E层：spawner_variant | 直接对应原生+0x38的spawner_variant，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 295 Entity.SplatColor | 继承E层：splat_color | 直接对应原生+0x1E4的splat_color，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 296 Entity.SpriteOffset | 继承E层：sprite#0 | 主Sprite公开别名，字节在内嵌ANM2的offset（Entity +0xDC）；随sprite#0外观子块发送，不重复发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 297 Entity.SpriteRotation | 继承E层：sprite#0 | 主Sprite公开别名，字节在内嵌ANM2的rotation（Entity +0xEC）；随sprite#0外观子块发送，不重复发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 298 Entity.SpriteScale | 继承E层：sprite#0 | 主Sprite公开别名，字节在内嵌ANM2的scale（Entity +0xE4）；随sprite#0外观子块发送，不重复发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 299 Entity.SubType | 继承E层：subtype | 直接对应原生+0x30的subtype，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 300 Entity.TakeDamage | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 301 Entity.Target | 继承E层：target | 对应原生+0x3C4的EntityPtr target；转成员序号发送，目标不在成员表时报缺口，不发送地址。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 302 Entity.TargetPosition | 继承E层：target_position | 直接对应原生+0x334的target_position，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 303 Entity.TeleportToRandomPosition | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 304 Entity.ToBomb | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 305 Entity.ToDelirium | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 306 Entity.ToEffect | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 307 Entity.ToFamiliar | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 308 Entity.ToKnife | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 309 Entity.ToLaser | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 310 Entity.ToNPC | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 311 Entity.ToPickup | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 312 Entity.ToPlayer | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 313 Entity.ToProjectile | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 314 Entity.ToSlot | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 315 Entity.ToTear | 本地类型转换：已安装对象的真实类型/shape | 只作本地借用，不发送转换命令或生成新实体。 |
| 316 Entity.TryThrow | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 317 Entity.Type | 继承E层：type | 直接对应原生+0x28的type，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 318 Entity.Update | Host操作/查询：Entity原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 319 Entity.Variant | 继承E层：variant | 直接对应原生+0x2C的variant，按原值发送；变化走结构事务。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 320 Entity.Velocity | 继承E层：velocity | 直接对应原生+0x360的velocity，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 321 Entity.Visible | 继承E层：visible | 直接对应原生+0x171的visible，按原值发送。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |

### EntityConfigBaby

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 322 EntityConfigBaby.GetAchievementID | 本地资源：匹配版本的EntityConfigBaby配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 323 EntityConfigBaby.GetID | 本地资源：匹配版本的EntityConfigBaby配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 324 EntityConfigBaby.GetName | 本地资源：匹配版本的EntityConfigBaby配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 325 EntityConfigBaby.GetSpritesheetPath | 本地资源：匹配版本的EntityConfigBaby配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |

### EntityConfigPlayer

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 326 EntityConfigPlayer.CanShoot | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 327 EntityConfigPlayer.GetAchievementID | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 328 EntityConfigPlayer.GetBirthrightDescription | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 329 EntityConfigPlayer.GetBlackHearts | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 330 EntityConfigPlayer.GetBombs | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 331 EntityConfigPlayer.GetBrokenHearts | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 332 EntityConfigPlayer.GetCard | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 333 EntityConfigPlayer.GetCoins | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 334 EntityConfigPlayer.GetCollectibles | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 335 EntityConfigPlayer.GetCostumeID | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 336 EntityConfigPlayer.GetCostumeSuffix | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 337 EntityConfigPlayer.GetExtraPortraitPath | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 338 EntityConfigPlayer.GetKeys | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 339 EntityConfigPlayer.GetModdedControlsSprite | 最终显示缓存：第8步菜单实际Sprite资源／姿态owner | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 340 EntityConfigPlayer.GetModdedCoopMenuSprite | 最终显示缓存：第8步菜单实际Sprite资源／姿态owner | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 341 EntityConfigPlayer.GetModdedGameOverSprite | 最终显示缓存：第8步菜单实际Sprite资源／姿态owner | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 342 EntityConfigPlayer.GetModdedMenuBackgroundSprite | 最终显示缓存：第8步菜单实际Sprite资源／姿态owner | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 343 EntityConfigPlayer.GetModdedMenuPortraitSprite | 最终显示缓存：第8步菜单实际Sprite资源／姿态owner | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 344 EntityConfigPlayer.GetName | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 345 EntityConfigPlayer.GetNameImagePath | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 346 EntityConfigPlayer.GetPill | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 347 EntityConfigPlayer.GetPlayerType | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 348 EntityConfigPlayer.GetPocketActive | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 349 EntityConfigPlayer.GetPortraitPath | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 350 EntityConfigPlayer.GetRedHearts | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 351 EntityConfigPlayer.GetSkinColor | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 352 EntityConfigPlayer.GetSkinPath | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 353 EntityConfigPlayer.GetSoulHearts | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 354 EntityConfigPlayer.GetTaintedCounterpart | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 355 EntityConfigPlayer.GetTrinket | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 356 EntityConfigPlayer.IsHidden | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |
| 357 EntityConfigPlayer.IsTainted | 本地资源：匹配版本的EntityConfigPlayer配置 | 配置初始生命／道具不是当前Player状态；实际菜单Sprite必须由自己的owner安装，不能当静态路径猜对象。 |

### EntityPlayer

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 358 EntityPlayer.AddActiveCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 359 EntityPlayer.AddBlackHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 360 EntityPlayer.AddBloodCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 361 EntityPlayer.AddBlueFlies | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 362 EntityPlayer.AddBlueSpider | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 363 EntityPlayer.AddBombs | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 364 EntityPlayer.AddBoneHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 365 EntityPlayer.AddBoneOrbital | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 366 EntityPlayer.AddBrokenHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 367 EntityPlayer.AddCacheFlags | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 368 EntityPlayer.AddCacheFlags | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 369 EntityPlayer.AddCandyHeartBonus | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 370 EntityPlayer.AddCard | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 371 EntityPlayer.AddCoins | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 372 EntityPlayer.AddCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 373 EntityPlayer.AddCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 374 EntityPlayer.AddCollectibleEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 375 EntityPlayer.AddControlsCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 376 EntityPlayer.AddCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 377 EntityPlayer.AddCurseMistEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 378 EntityPlayer.AddDeadEyeCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 379 EntityPlayer.AddDollarBillEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 380 EntityPlayer.AddEternalHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 381 EntityPlayer.AddFriendlyDip | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 382 EntityPlayer.AddGigaBombs | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 383 EntityPlayer.AddGoldenBomb | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 384 EntityPlayer.AddGoldenHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 385 EntityPlayer.AddGoldenKey | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 386 EntityPlayer.AddHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 387 EntityPlayer.AddInnateCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 388 EntityPlayer.AddInnateTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 389 EntityPlayer.AddItemWisp | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 390 EntityPlayer.AddJarFlies | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 391 EntityPlayer.AddJarHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 392 EntityPlayer.AddKeys | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 393 EntityPlayer.AddLeprosy | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 394 EntityPlayer.AddLocust | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 395 EntityPlayer.AddMaxHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 396 EntityPlayer.AddMinisaac | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 397 EntityPlayer.AddNullCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 398 EntityPlayer.AddNullItemEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 399 EntityPlayer.AddPill | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 400 EntityPlayer.AddPlayerFormCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 401 EntityPlayer.AddPoopMana | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 402 EntityPlayer.AddPrettyFly | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 403 EntityPlayer.AddRottenHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 404 EntityPlayer.AddSmeltedTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 405 EntityPlayer.AddSoulCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 406 EntityPlayer.AddSoulHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 407 EntityPlayer.AddSoulLocketBonus | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 408 EntityPlayer.AddSwarmFlyOrbital | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 409 EntityPlayer.AddTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 410 EntityPlayer.AddTrinketEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 411 EntityPlayer.AddUrnSouls | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 412 EntityPlayer.AddWisp | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 413 EntityPlayer.AnimateAppear | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 414 EntityPlayer.AnimateCard | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 415 EntityPlayer.AnimateCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 416 EntityPlayer.AnimateHappy | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 417 EntityPlayer.AnimateLightTravel | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 418 EntityPlayer.AnimatePickup | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 419 EntityPlayer.AnimatePill | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 420 EntityPlayer.AnimatePitfallIn | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 421 EntityPlayer.AnimatePitfallOut | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 422 EntityPlayer.AnimateSad | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 423 EntityPlayer.AnimateTeleport | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 424 EntityPlayer.AnimateTrapdoor | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 425 EntityPlayer.AnimateTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 426 EntityPlayer.AreControlsEnabled | P7（裁剪）：C/S active／can_consume控制语义 | C/S统一输入语义和组记录决定控制状态；不复制Host原版设备、暂停或ControlsCooldown。；不承诺重放同名原版设备查询。 |
| 427 EntityPlayer.AreOpposingShootDirectionsPressed | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 428 EntityPlayer.BabySkin | 本地资源：P7 playerType／配置引用＋本地同版本配置资源 | 角色配置和静态名字由本地资源按身份解析；可变显示变体通过P0/P6或最终HUD字段表达。 |
| 429 EntityPlayer.BlockCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 430 EntityPlayer.BlockTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 431 EntityPlayer.CalculateBagOfCraftingOutput | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 432 EntityPlayer.CanAddCollectible | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 433 EntityPlayer.CanAddCollectibleToInventory | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 434 EntityPlayer.CanCrushRocks | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 435 EntityPlayer.CanFly | P1：CanFly@0x1564 | 绘制直接读取飞行结果；不复制使其成立的道具、回调与属性生产链。 |
| 436 EntityPlayer.CanOverrideActiveItem | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 437 EntityPlayer.CanPickBlackHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 438 EntityPlayer.CanPickBoneHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 439 EntityPlayer.CanPickGoldenHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 440 EntityPlayer.CanPickRedHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 441 EntityPlayer.CanPickRottenHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 442 EntityPlayer.CanPickSoulHearts | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 443 EntityPlayer.CanPickupItem | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 444 EntityPlayer.CanShoot | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 445 EntityPlayer.CanTurnHead | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 446 EntityPlayer.CanUsePill | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 447 EntityPlayer.ChangePlayerType | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 448 EntityPlayer.CheckFamiliar | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 449 EntityPlayer.CheckFamiliarEx | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 450 EntityPlayer.ClearCollectibleAnim | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 451 EntityPlayer.ClearCostumes | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 452 EntityPlayer.ClearDeadEyeCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 453 EntityPlayer.ClearDeadEyeCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 454 EntityPlayer.ClearInnateItemGroup | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 455 EntityPlayer.ClearQueueItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 456 EntityPlayer.ClearTemporaryEffects | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 457 EntityPlayer.ControllerIndex | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 458 EntityPlayer.ControlsCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 459 EntityPlayer.ControlsEnabled | P7（裁剪）：C/S active／can_consume控制语义 | C/S统一输入语义和组记录决定控制状态；不复制Host原版设备、暂停或ControlsCooldown。；不承诺重放同名原版设备查询。 |
| 460 EntityPlayer.CreateAfterimage | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 461 EntityPlayer.Damage | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 462 EntityPlayer.DischargeActiveItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 463 EntityPlayer.DoZitEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 464 EntityPlayer.DonateLuck | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 465 EntityPlayer.DropCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 466 EntityPlayer.DropCollectibleByHistoryIndex | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 467 EntityPlayer.DropPocketItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 468 EntityPlayer.DropTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 469 EntityPlayer.EnableWeaponType | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 470 EntityPlayer.EvaluateItems | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 471 EntityPlayer.FireBomb | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 472 EntityPlayer.FireBrimstone | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 473 EntityPlayer.FireBrimstoneBall | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 474 EntityPlayer.FireDelay | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 475 EntityPlayer.FireDelayedBrimstone | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 476 EntityPlayer.FireKnife | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 477 EntityPlayer.FireTear | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 478 EntityPlayer.FireTechLaser | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 479 EntityPlayer.FireTechXLaser | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 480 EntityPlayer.FlushQueueItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 481 EntityPlayer.FriendBallEnemy | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 482 EntityPlayer.FullCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 483 EntityPlayer.GetActionHoldDrop | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 484 EntityPlayer.GetActiveCharge | P4：4 ActiveItemDesc六值 | 按Item／charge／battery／subcharge／partial／VarData投影；空Item只读Item并将其余五值编码为零。；不因返回描述对象恢复pool与TimedRechargeCooldown。 |
| 485 EntityPlayer.GetActiveItem | P4：4 ActiveItemDesc六值 | 按Item／charge／battery／subcharge／partial／VarData投影；空Item只读Item并将其余五值编码为零。；不因返回描述对象恢复pool与TimedRechargeCooldown。 |
| 486 EntityPlayer.GetActiveItemDesc | P4：4 ActiveItemDesc六值 | 按Item／charge／battery／subcharge／partial／VarData投影；空Item只读Item并将其余五值编码为零。；不因返回描述对象恢复pool与TimedRechargeCooldown。 |
| 487 EntityPlayer.GetActiveItemSlot | Host：容量／可用槽的玩法查询 | P3/P4固定物理槽及最终HUD布局已独立声明；不用此API决定Client分配或重排装备。 |
| 488 EntityPlayer.GetActiveMaxCharge | 本地资源/派生：P4＋本地匹配ItemConfig；有玩法分支的实际显示改最终缓存 | 固定配置上限由本地同版本资源提供；不复制道具池或生产冷却。；不可宣称所有mod回调／物品特殊分支已由P4六值复原。 |
| 489 EntityPlayer.GetActiveMinUsableCharge | 本地资源/派生：P4＋本地匹配ItemConfig；有玩法分支的实际显示改最终缓存 | 固定配置上限由本地同版本资源提供；不复制道具池或生产冷却。；不可宣称所有mod回调／物品特殊分支已由P4六值复原。 |
| 490 EntityPlayer.GetActiveSubCharge | P4：4 ActiveItemDesc六值 | 按Item／charge／battery／subcharge／partial／VarData投影；空Item只读Item并将其余五值编码为零。；不因返回描述对象恢复pool与TimedRechargeCooldown。 |
| 491 EntityPlayer.GetActiveWeaponEntity | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 492 EntityPlayer.GetActiveWeaponNumFired | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 493 EntityPlayer.GetAimDirection | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 494 EntityPlayer.BabySkin | 本地资源：P7 playerType／配置引用＋本地同版本配置资源 | 角色配置和静态名字由本地资源按身份解析；可变显示变体通过P0/P6或最终HUD字段表达。 |
| 495 EntityPlayer.GetBagOfCraftingContent | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 496 EntityPlayer.GetBagOfCraftingOutput | 最终显示缓存：第8步实际crafting图标／输出显示 | 原生BagOutput为Host配方生产缓存，不靠Client重跑配方／进度查询；发送实际可见结果。 |
| 497 EntityPlayer.GetBagOfCraftingSlot | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 498 EntityPlayer.GetBatteryCharge | P4：4 ActiveItemDesc六值 | 按Item／charge／battery／subcharge／partial／VarData投影；空Item只读Item并将其余五值编码为零。；不因返回描述对象恢复pool与TimedRechargeCooldown。 |
| 499 EntityPlayer.GetBlackHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 500 EntityPlayer.GetBladderCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 501 EntityPlayer.GetBlinkLockTime | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 502 EntityPlayer.GetBloodCharge | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 503 EntityPlayer.GetBloodGushSprite | P0：内嵌ANM2物理槽0／10／11 | 保留各自Sprite资源与最终姿态；不以文件名合并owner。；Getter返回本地对象句柄；网络只投影ANM2，不发送指针。 |
| 504 EntityPlayer.GetBloodLustCounter | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 505 EntityPlayer.GetBodyColor | Host：皮肤生成选择 | 公共SkinColor枚举不是最终像素颜色；P6不复制skin生产元数据，实际资源／Sprite／LayerState已明确发送。 |
| 506 EntityPlayer.GetBodyMoveDirection | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 507 EntityPlayer.GetBodySprite | P0：内嵌ANM2物理槽0／10／11 | 保留各自Sprite资源与最终姿态；不以文件名合并owner。；Getter返回本地对象句柄；网络只投影ANM2，不发送指针。 |
| 508 EntityPlayer.GetBombFlags | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 509 EntityPlayer.GetBombFlags | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 510 EntityPlayer.GetBombPlaceDelay | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 511 EntityPlayer.GetBombVariant | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 512 EntityPlayer.GetBoneHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 513 EntityPlayer.GetBrokenHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 514 EntityPlayer.GetCambionConceptionState | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 515 EntityPlayer.GetCambionPregnancyLevel | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 516 EntityPlayer.GetCandyHeartBonus | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 517 EntityPlayer.GetCard | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 518 EntityPlayer.GetCardRNG | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 519 EntityPlayer.GetCharmOfTheVampireKills | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 520 EntityPlayer.GetCollectibleCount | Host：旧通用collectible总量查询 | 未登记独立实际显示消费者，不为此API新增最终值或模拟字段；实际HUD数量按它自己的最终缓存合同提供。 |
| 521 EntityPlayer.GetCollectibleNum | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 522 EntityPlayer.GetCollectibleNum | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 523 EntityPlayer.GetCollectibleRNG | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 524 EntityPlayer.GetCollectiblesList | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 525 EntityPlayer.GetConceptionFamiliarFlags | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 526 EntityPlayer.GetCostumeLayerMap | P6/P0：P6目录与15项最终map；Sprite归P0 | 只同步最终服装结构和姿态，不发送优先级、皮肤生产或item-state-only元数据。 |
| 527 EntityPlayer.GetCostumeNullPos | 本地资源/派生：P0/P6姿态＋本地ANM NullFrame＋已发送比例 | 几何位置由最终Sprite与匹配资源计算，不发送每个字符串参数的查询结果。 |
| 528 EntityPlayer.GetCostumeSpriteDescs | P6/P0：P6目录与15项最终map；Sprite归P0 | 只同步最终服装结构和姿态，不发送优先级、皮肤生产或item-state-only元数据。 |
| 529 EntityPlayer.GetCustomCacheValue | Host：EntityPlayerPlus.customCacheResults | 任意tag→double表属于玩法缓存；只发送实际StatHUD／健康／全局容量的具名最终显示，不传整个map。 |
| 530 EntityPlayer.GetD8DamageModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 531 EntityPlayer.GetD8FireDelayModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 532 EntityPlayer.GetD8RangeModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 533 EntityPlayer.GetD8SpeedModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 534 EntityPlayer.GetDamageCooldown | P0：P0.damage_cooldown@13B8 | P0具名原生绘制输入；公开便利函数不扩展同步范围。 |
| 535 EntityPlayer.GetDamageModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 536 EntityPlayer.GetDeadEyeCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 537 EntityPlayer.GetDeathAnimName | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 538 EntityPlayer.GetDonateLuck | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 539 EntityPlayer.GetEdenDamage | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 540 EntityPlayer.GetEdenFireDelay | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 541 EntityPlayer.GetEdenLuck | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 542 EntityPlayer.GetEdenRange | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 543 EntityPlayer.GetEdenShotSpeed | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 544 EntityPlayer.GetEdenSpeed | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 545 EntityPlayer.GetEffectiveBloodCharge | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 546 EntityPlayer.GetEffectiveMaxHearts | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 547 EntityPlayer.GetEffectiveSoulCharge | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 548 EntityPlayer.GetEffects | P8：disabled＋有序效果配置／count；704附cooldown | 保留当前效果查询的次序与首匹配语义；不是完整TemporaryEffects管理对象。；非704 cooldown、所有增删与+2E94待移除队列不传。 |
| 549 EntityPlayer.GetEnterPosition | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 550 EntityPlayer.GetEntityConfigPlayer | 本地资源：P7 playerType／配置引用＋本地同版本配置资源 | 角色配置和静态名字由本地资源按身份解析；可变显示变体通过P0/P6或最终HUD字段表达。 |
| 551 EntityPlayer.GetEpiphoraCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 552 EntityPlayer.GetErrorTrinketEffect | Host：特殊饰品／星座效果生产或查询上下文 | 不因名称与P3当前ID邻近而发送；P3只保留+1388/+1390/+1F94实际查询读值。 |
| 553 EntityPlayer.GetEternalHearts | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 554 EntityPlayer.GetEveSumptoriumCharge | P0：EveSumptoriumCharge@0x1F58 | 7589A3实际充能指示输入。 |
| 555 EntityPlayer.GetExtraLives | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 556 EntityPlayer.GetFireDelayModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 557 EntityPlayer.GetFireDirection | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 558 EntityPlayer.GetFlippedForm | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 559 EntityPlayer.GetFlyingOffset | 本地派生：P1 CanFly＋同版本Room.type＋本地共享默认向量 | 77B890先检查Game+18300所指Room+8是否16；该分支返回本地正常初始化的共享默认偏移，其余CanFly=true时为(0,-4)，否则默认偏移。不新增线值。；不能只根据CanFly计算；切房时Room与Player目标必须一致。 |
| 560 EntityPlayer.GetFocusEntity | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 561 EntityPlayer.GetFootprintColor | Host：脚印颜色生产模板 | 官方Lua接口将两份ColorMod的offsetRGB＋tintA转KColor；已产生脚印实体按其实际颜色同步，不发送Player脚印模板。 |
| 562 EntityPlayer.GetForgottenSwapFormCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 563 EntityPlayer.GetGlitchBabySubType | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 564 EntityPlayer.GetGlyphOfBalanceDrop | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 565 EntityPlayer.GetGnawedLeafTimer | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 566 EntityPlayer.GetGoldenHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 567 EntityPlayer.GetGreedDonationBreakChance | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 568 EntityPlayer.GetGreedsGulletHearts | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 569 EntityPlayer.GetHallowedGroundCountdown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 570 EntityPlayer.GetHeadColor | Host：皮肤生成选择 | 公共SkinColor枚举不是最终像素颜色；P6不复制skin生产元数据，实际资源／Sprite／LayerState已明确发送。 |
| 571 EntityPlayer.GetHeadDirection | Host：+1618／+161C动画生产输入 | 头／移动方向用于动画生产；最终ANM2与headAnim已发送，不能据其重选动画。 |
| 572 EntityPlayer.GetHeadDirectionLockTime | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 573 EntityPlayer.GetHealthType | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 574 EntityPlayer.GetHeartLimit | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 575 EntityPlayer.GetHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 576 EntityPlayer.GetHeldEntity | P7：角色／索引／twin／held及C/S组 | 用StateRef与结构记录表达；原生指针、设备索引和恢复槽不作为网络标识。 |
| 577 EntityPlayer.GetHeldSprite | P0：内嵌ANM2物理槽0／10／11 | 保留各自Sprite资源与最终姿态；不以文件名合并owner。；Getter返回本地对象句柄；网络只投影ANM2，不发送指针。 |
| 578 EntityPlayer.GetHistory | 最终显示缓存：第8步最终HistoryHUD／库存图像列表 | 原始History与历史索引留Host；只同步正常Host生产的实际显示条目。；不在Client构造完整History来驱动原生重算。 |
| 579 EntityPlayer.GetImExcitedSpeedupCountdown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 580 EntityPlayer.GetImmaculateConceptionState | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 581 EntityPlayer.GetInnateCollectibleCount | P3（裁剪）：blocked＋有效innate聚合计数 | 只保留persistent与temporary.cachedTotals聚合结果；不发送GroupKey、定时列表或管理历史。；不承诺复原按GroupKey的公共查询；有分组参数的旧行只用于追溯，Client无通用Lua查询接口。 |
| 582 EntityPlayer.GetInnateCollectibleGroup | P3（裁剪）：blocked＋有效innate聚合计数 | 只保留persistent与temporary.cachedTotals聚合结果；不发送GroupKey、定时列表或管理历史。；不承诺复原按GroupKey的公共查询；有分组参数的旧行只用于追溯，Client无通用Lua查询接口。 |
| 583 EntityPlayer.GetInnateTrinketCount | P3（裁剪）：blocked＋有效innate聚合计数 | 只保留persistent与temporary.cachedTotals聚合结果；不发送GroupKey、定时列表或管理历史。；不承诺复原按GroupKey的公共查询；有分组参数的旧行只用于追溯，Client无通用Lua查询接口。 |
| 584 EntityPlayer.GetInnateTrinketGroup | P3（裁剪）：blocked＋有效innate聚合计数 | 只保留persistent与temporary.cachedTotals聚合结果；不发送GroupKey、定时列表或管理历史。；不承诺复原按GroupKey的公共查询；有分组参数的旧行只用于追溯，Client无通用Lua查询接口。 |
| 585 EntityPlayer.GetItemState | P0：P0.item_state@15F4 | P0具名原生绘制输入；公开便利函数不扩展同步范围。 |
| 586 EntityPlayer.GetItemStateCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 587 EntityPlayer.GetJarFlies | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 588 EntityPlayer.GetJarHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 589 EntityPlayer.GetKeepersSackBonus | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 590 EntityPlayer.LaserColor | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 591 EntityPlayer.GetLaserOffset | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 592 EntityPlayer.GetLastActionTriggers | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 593 EntityPlayer.GetLastDamageFlags | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 594 EntityPlayer.GetLastDamageSource | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 595 EntityPlayer.GetLastDirection | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 596 EntityPlayer.GetLuckModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 597 EntityPlayer.GetMaggyHealthDrainCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 598 EntityPlayer.GetMaggySwingCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 599 EntityPlayer.GetMainTwin | P7：角色／索引／twin／held及C/S组 | 用StateRef与结构记录表达；原生指针、设备索引和恢复槽不作为网络标识。 |
| 600 EntityPlayer.GetMarkedTarget | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 601 EntityPlayer.GetMawOfTheVoidCharge | P0：Maw／Revelation／Montezuma charge | 实际Render充能指示读数，按P0已核位宽。 |
| 602 EntityPlayer.GetMaxBladderCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 603 EntityPlayer.GetMaxBombs | 最终显示缓存：第8步全局资源容量／HUD格式结果 | REP custom-cache容量是全局有效结果，不是每身体原始缓存；不能复制整个customCacheResults或在Client运行回调。 |
| 604 EntityPlayer.GetMaxCoins | 最终显示缓存：第8步全局资源容量／HUD格式结果 | REP custom-cache容量是全局有效结果，不是每身体原始缓存；不能复制整个customCacheResults或在Client运行回调。 |
| 605 EntityPlayer.GetMaxHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 606 EntityPlayer.GetMaxKeys | 最终显示缓存：第8步全局资源容量／HUD格式结果 | REP custom-cache容量是全局有效结果，不是每身体原始缓存；不能复制整个customCacheResults或在Client运行回调。 |
| 607 EntityPlayer.GetMaxPeeBurstCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 608 EntityPlayer.GetMaxPocketItems | Host：容量／可用槽的玩法查询 | P3/P4固定物理槽及最终HUD布局已独立声明；不用此API决定Client分配或重排装备。 |
| 609 EntityPlayer.GetMaxPoopMana | Host：容量／可用槽的玩法查询 | P3/P4固定物理槽及最终HUD布局已独立声明；不用此API决定Client分配或重排装备。 |
| 610 EntityPlayer.GetMaxTrinkets | Host：容量／可用槽的玩法查询 | P3/P4固定物理槽及最终HUD布局已独立声明；不用此API决定Client分配或重排装备。 |
| 611 EntityPlayer.GetMegaBlastDuration | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 612 EntityPlayer.GetMetronomeCollectibleID | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 613 EntityPlayer.GetModelingClayEffect | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 614 EntityPlayer.GetMontezumaRevengeCharge | P0：Maw／Revelation／Montezuma charge | 实际Render充能指示读数，按P0已核位宽。 |
| 615 EntityPlayer.GetMovementDirection | Host：+1618／+161C动画生产输入 | 头／移动方向用于动画生产；最终ANM2与headAnim已发送，不能据其重选动画。 |
| 616 EntityPlayer.GetMovementInput | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 617 EntityPlayer.GetMovementJoystick | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 618 EntityPlayer.GetMovementVector | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 619 EntityPlayer.GetMovingBoxContents | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 620 EntityPlayer.GetMultiShotParams | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 621 EntityPlayer.GetMultiShotParams | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 622 EntityPlayer.GetMultiShotPositionVelocity | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 623 EntityPlayer.GetMultiShotPositionVelocity | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 624 EntityPlayer.GetNPCTarget | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 625 EntityPlayer.GetName | 本地资源：P7 playerType／配置引用＋本地同版本配置资源 | 角色配置和静态名字由本地资源按身份解析；可变显示变体通过P0/P6或最终HUD字段表达。 |
| 626 EntityPlayer.GetNextUrethraBlockFrame | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 627 EntityPlayer.GetNumBlueFlies | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 628 EntityPlayer.GetNumBlueSpiders | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 629 EntityPlayer.GetNumBombs | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 630 EntityPlayer.GetNumCoins | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 631 EntityPlayer.GetNumGigaBombs | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 632 EntityPlayer.GetNumKeys | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 633 EntityPlayer.GetOtherTwin | P7：角色／索引／twin／held及C/S组 | 用StateRef与结构记录表达；原生指针、设备索引和恢复槽不作为网络标识。 |
| 634 EntityPlayer.GetPeeBurstCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 635 EntityPlayer.GetPill | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 636 EntityPlayer.GetPillRNG | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 637 EntityPlayer.GetPlanCKillCountdown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 638 EntityPlayer.GetPlayerFormCounter | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 639 EntityPlayer.GetPlayerHUD | 最终显示缓存：第8步动态PlayerHUD owner及最终缓存 | Player+20A8是8B强引用pair，地址／控制块本地；显示字段独立投影。 |
| 640 EntityPlayer.GetPlayerIndex | P7：角色／索引／twin／held及C/S组 | 用StateRef与结构记录表达；原生指针、设备索引和恢复槽不作为网络标识。 |
| 641 EntityPlayer.GetPlayerType | P7：角色／索引／twin／held及C/S组 | 用StateRef与结构记录表达；原生指针、设备索引和恢复槽不作为网络标识。 |
| 642 EntityPlayer.GetPocketItem | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 643 EntityPlayer.GetPocketItem | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 644 EntityPlayer.GetPonyCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 645 EntityPlayer.GetPoopMana | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 646 EntityPlayer.GetPoopSpell | P3：8 crafting材料／6 poop队列／4 pocket ID,type | 保留真实固定槽序及类型；空口袋的原生type不能自行归零。 |
| 647 EntityPlayer.GetPotatoPeelerUses | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 648 EntityPlayer.GetPurityState | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 649 EntityPlayer.GetRUAWizardTimer | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 650 EntityPlayer.GetRecentMovementVector | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 651 EntityPlayer.GetRedStewBonusDuration | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 652 EntityPlayer.GetRevelationCharge | P0：Maw／Revelation／Montezuma charge | 实际Render充能指示读数，按P0已核位宽。 |
| 653 EntityPlayer.GetRockBottomDamage | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 654 EntityPlayer.GetRockBottomLuck | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 655 EntityPlayer.GetRockBottomMaxFireDelay | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 656 EntityPlayer.GetRockBottomMoveSpeed | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 657 EntityPlayer.GetRockBottomShotSpeed | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 658 EntityPlayer.GetRockBottomTearRange | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 659 EntityPlayer.GetRottenHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 660 EntityPlayer.GetShootingInput | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 661 EntityPlayer.GetShootingJoystick | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 662 EntityPlayer.GetShotSpeedModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 663 EntityPlayer.GetSmeltedTrinketDesc | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 664 EntityPlayer.GetSmeltedTrinkets | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 665 EntityPlayer.GetSmoothBodyRotation | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 666 EntityPlayer.GetSoulCharge | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 667 EntityPlayer.GetSoulHearts | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 668 EntityPlayer.GetSoulLocketBonus | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 669 EntityPlayer.GetSpecialGridCollision | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 670 EntityPlayer.GetSpeedModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 671 EntityPlayer.GetSpoofedCollectiblesList | P3（裁剪）：blocked＋有效innate聚合计数 | 只保留persistent与temporary.cachedTotals聚合结果；不发送GroupKey、定时列表或管理历史。；不承诺复原按GroupKey的公共查询；有分组参数的旧行只用于追溯，Client无通用Lua查询接口。 |
| 672 EntityPlayer.GetStatMultiplier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 673 EntityPlayer.GetSubPlayer | P7（裁剪）：C/S body_role／组成员发现 | native +1D84仅Host发现／模拟；Client按已提交C/S组定位相关身体，不写回native sub存储。；不得把sub getter旧关系当作必须发送的原生边。 |
| 674 EntityPlayer.GetSuplexAimCountdown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 675 EntityPlayer.GetSuplexLandPosition | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 676 EntityPlayer.GetSuplexState | P0：P0.suplex_state@1F08 | P0具名原生绘制输入；公开便利函数不扩展同步范围。 |
| 677 EntityPlayer.GetSuplexTargetPosition | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 678 EntityPlayer.GetTearDisplacement | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 679 EntityPlayer.GetTearHitParams | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 680 EntityPlayer.GetTearMovementInheritance | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 681 EntityPlayer.GetTearPoisonDamage | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 682 EntityPlayer.GetTearRangeModifier | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 683 EntityPlayer.GetTearsCap | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |
| 684 EntityPlayer.GetTotalActiveCharge | 本地派生：P4 charge/battery/item＋P2 soul/blood charge＋P7 playerType＋ItemConfig | 7895D0先加普通/电池，再按角色18/36、配置ChargeType/MaxCharges及655/326例外使用魂/血充能；已有域足够，不额外发送总量。；按真实公式保留特殊分支，不是简单charge+battery，也不调用充能/到期操作。 |
| 685 EntityPlayer.GetTotalDamageTaken | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 686 EntityPlayer.GetTractorBeam | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 687 EntityPlayer.GetTrinket | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 688 EntityPlayer.GetTrinketMultiplier | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 689 EntityPlayer.GetTrinketRNG | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 690 EntityPlayer.GetUrnSouls | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 691 EntityPlayer.GetVelocityBeforeUpdate | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 692 EntityPlayer.GetVoidedCollectiblesList | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 693 EntityPlayer.GetWeapon | P5：五槽次序／存在性／动态weapon_type | 只安装P5展示对象，不运行Fire／Update或生产实体。 |
| 694 EntityPlayer.GetWeaponModifiers | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 695 EntityPlayer.GetWildCardItem | Host：+2064类型／+2068道具 | Wild Card未来Use历史，不是当前口袋或当前有效库存输入。 |
| 696 EntityPlayer.GetWildCardItemType | Host：+2064类型／+2068道具 | Wild Card未来Use历史，不是当前口袋或当前有效库存输入。 |
| 697 EntityPlayer.GetWispCollectiblesList | P3：counts／trinkets／smelted／wisp／当前道具与通用查询输入 | P3已核当前库存及展示查询输入；仅实体状态、配置引用与值，不复制getter执行。 |
| 698 EntityPlayer.GetZodiacEffect | Host：特殊饰品／星座效果生产或查询上下文 | 不因名称与P3当前ID邻近而发送；P3只保留+1388/+1390/+1F94实际查询读值。 |
| 699 EntityPlayer.HasCamoEffect | Host/本地：UpdateEffect生产链；最终ANM／flags／颜色已有owner | Plus.camoOverride正面消费者改变UpdateEffect，不新增网络布尔量。 |
| 700 EntityPlayer.HasChanceRevive | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 701 EntityPlayer.HasCollectible | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 702 EntityPlayer.HasCollectible | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 703 EntityPlayer.HasCurseMistEffect | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 704 EntityPlayer.HasForcedCamoEffect | Host/本地：UpdateEffect生产链；最终ANM／flags／颜色已有owner | Plus.camoOverride正面消费者改变UpdateEffect，不新增网络布尔量。 |
| 705 EntityPlayer.HasFullHearts | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 706 EntityPlayer.HasFullHeartsAndSoulHearts | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 707 EntityPlayer.HasGoldenBomb | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 708 EntityPlayer.HasGoldenKey | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 709 EntityPlayer.HasGoldenTrinket | Host/内部查询：饰品查询内部判断；无独立结果下行 | 黄金位／熔炼双计数／innate当前输入已有P3；实际倍率消费者按已登记决定处理，不再给这个公共getter单独造缓存。 |
| 710 EntityPlayer.HasInstantDeathCurse | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 711 EntityPlayer.HasInvincibility | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 712 EntityPlayer.HasPlayerForm | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 713 EntityPlayer.HasPoisonImmunity | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 714 EntityPlayer.HasTimedItem | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 715 EntityPlayer.HasTrinket | 最终显示决定：P3输入＋第8/10步具名展示消费者的最终bool/int | P3只保证已裁剪的当前输入；原生查询可能跨进度／Online／局部RNG分支，不能在Client任意重放，也不能额外调用Host getter补字典。；限受支持消费者、确切参数和同一subject/use/shape；非显示调用仍Host。 |
| 716 EntityPlayer.HasWeaponType | P5：五槽次序／存在性／动态weapon_type | 只安装P5展示对象，不运行Fire／Update或生产实体。 |
| 717 EntityPlayer.HeadFrameDelay | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 718 EntityPlayer.IBSCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 719 EntityPlayer.IncrementPlayerFormCounter | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 720 EntityPlayer.InitBabySkin | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 721 EntityPlayer.InitPostLevelInitStats | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 722 EntityPlayer.InitTwin | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 723 EntityPlayer.IsBlackHeart | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 724 EntityPlayer.IsBoneHeart | 最终显示缓存：第8步 PlayerHUD 逐心／资源布局结果 | 健康类型回调、心格转换及身体重定向不得在Client重新求值；只保留实际HUD生产结果。；不新增每个getter的通用结果表；GetHealthType不等同previousHealthType。 |
| 725 EntityPlayer.IsCollectibleAnimFinished | Host：动画推进／完成事件查询 | 本轮没有独立公共查询消费者；Player不靠这类API重演动画生产，实际Sprite姿态由P0/P8安装。 |
| 726 EntityPlayer.IsCollectibleBlocked | P3：Plus collectible／trinket blocked集合 | blocked是当前有效查询输入，不发送unordered_set桶或分配结构。 |
| 727 EntityPlayer.IsCollectibleCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 728 EntityPlayer.IsCollectibleCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 729 EntityPlayer.IsCoopGhost | P2：P2具名生命／资源存储（77B） | 已有P2当前读数；不复制生产这些值的增减、复活或健康变换过程。；公开getter可能按身体关系转向或聚合，不保证等同单字段；HUD以已核最终缓存安装。 |
| 730 EntityPlayer.IsEntityValidTarget | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 731 EntityPlayer.IsExtraAnimationFinished | Host：动画推进／完成事件查询 | 本轮没有独立公共查询消费者；Player不靠这类API重演动画生产，实际Sprite姿态由P0/P8安装。 |
| 732 EntityPlayer.IsFootstepFrame | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 733 EntityPlayer.IsFullSpriteRendering | P0：P0.full_sprite_a@1398／full_sprite_b@139A | P0具名原生绘制输入；公开便利函数不扩展同步范围。 |
| 734 EntityPlayer.IsHeadless | 最终显示决定：P8当前效果＋P3具名HasCollectible(206)展示决定 | 783390的四个效果查询已有P8；HasCollectible(206)保留实际绘制调用结果，避免在Client进入其进度／Online分支。 |
| 735 EntityPlayer.IsHeldItemVisible | 本地派生（限展示）：P0 ItemState／SuplexState＋P7 held＋最终Sprite | 78558D—785733实际持物绘制读取已列输入；按已核显示分支消费，不新增任意公共getter结果。；只覆盖该绘制读集，不承诺整个游戏API。 |
| 736 EntityPlayer.IsHoldingItem | 本地派生（限展示）：P0 ItemState／SuplexState＋P7 held＋最终Sprite | 78558D—785733实际持物绘制读取已列输入；按已核显示分支消费，不新增任意公共getter结果。；只覆盖该绘制读集，不承诺整个游戏API。 |
| 737 EntityPlayer.IsHologram | 本地派生：P7资格／角色／twin／索引 | 7D1C90检查原生+172资格、29/38、真实twin与索引次序；按P7结构安装已核资格，不另发布尔结果。；资格不是Entity.Exists、资源加载完成或一律true。 |
| 738 EntityPlayer.IsInvisible | 本地派生：P8 disabled／有序效果配置与count＋P7 playerType | 7B9E80检查trinket69、collectible497及playerType35；这些输入已有明确投影。；保留disabled和首匹配／count语义；不把Client本机设置当Host判定。 |
| 739 EntityPlayer.IsItemCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 740 EntityPlayer.IsItemCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 741 EntityPlayer.IsItemQueueEmpty | P3（裁剪）：queued饰品配置与golden标记；空显式表达 | 绘制只需已核饰品队列投影，不能还原整份QueueItemData或重放队列动作。 |
| 742 EntityPlayer.IsLocalPlayer | Host/本地：原生设备／输入与C/S输入调度分离 | 不传Host设备索引、原版本机判断或原生手柄状态；C/S已声明的统一输入包和P7控制语义独立负责。 |
| 743 EntityPlayer.IsNullItemCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 744 EntityPlayer.IsNullItemCostumeVisible | 本地派生（限展示）：P6最终map＋P0最终LayerState | 展示实际选择与可见性从已提交拓扑／层状态读取，不能重新运行服装生产规则。；不保证任意ItemConfig/layer参数的完整旧API；无实际消费者的查询不增加状态。 |
| 745 EntityPlayer.IsP2Appearing | Host：动画／特效生产或原版加入过程 | 动画选择与脚步事件由Host生产；Client安装已产生的P0姿态／实体，不重选死亡动画、重放脚步或原版P2加入流程。 |
| 746 EntityPlayer.IsPosInSpotLight | Host：玩法资格／碰撞／道具或伤害判定 | 这些可读API仍是玩法判断；没有由旧Captured入口构成的Client显示消费者，不为其复制模拟状态。；不是对所有mod脚本承诺完整Player API。 |
| 747 EntityPlayer.IsPostLevelInitFinished | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 748 EntityPlayer.IsSubPlayer | P7（裁剪）：C/S body_role／组成员发现 | native +1D84仅Host发现／模拟；Client按已提交C/S组定位相关身体，不写回native sub存储。；不得把sub getter旧关系当作必须发送的原生边。 |
| 749 EntityPlayer.IsTrinketBlocked | P3：Plus collectible／trinket blocked集合 | blocked是当前有效查询输入，不发送unordered_set桶或分配结构。 |
| 750 EntityPlayer.IsUrethraBlocked | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 751 EntityPlayer.ItemHoldCooldown | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 752 EntityPlayer.LaserColor | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 753 EntityPlayer.Luck | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 754 EntityPlayer.MaxFireDelay | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 755 EntityPlayer.MorphToCoopGhost | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 756 EntityPlayer.MoveSpeed | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 757 EntityPlayer.NeedsCharge | 本地资源/派生：P4＋本地匹配ItemConfig；有玩法分支的实际显示改最终缓存 | 固定配置上限由本地同版本资源提供；不复制道具池或生产冷却。；不可宣称所有mod回调／物品特殊分支已由P4六值复原。 |
| 758 EntityPlayer.PlayCollectibleAnim | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 759 EntityPlayer.PlayDelayedSFX | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 760 EntityPlayer.PlayExtraAnimation | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 761 EntityPlayer.PlayItemNullAnimation | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 762 EntityPlayer.QueueExtraAnimation | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 763 EntityPlayer.QueueItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 764 EntityPlayer.QueuedItem | P3（裁剪）：queued饰品配置与golden标记；空显式表达 | 绘制只需已核饰品队列投影，不能还原整份QueueItemData或重放队列动作。 |
| 765 EntityPlayer.RemoveBlackHeart | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 766 EntityPlayer.RemoveBlueFly | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 767 EntityPlayer.RemoveBlueSpider | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 768 EntityPlayer.RemoveCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 769 EntityPlayer.RemoveCollectibleByHistoryIndex | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 770 EntityPlayer.RemoveCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 771 EntityPlayer.RemoveCurseMistEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 772 EntityPlayer.RemoveGoldenBomb | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 773 EntityPlayer.RemoveGoldenKey | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 774 EntityPlayer.RemoveInnateCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 775 EntityPlayer.RemoveInnateTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 776 EntityPlayer.RemovePocketItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 777 EntityPlayer.RemovePoopSpell | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 778 EntityPlayer.RemoveSkinCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 779 EntityPlayer.RenderBody | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 780 EntityPlayer.RenderGlow | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 781 EntityPlayer.RenderHead | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 782 EntityPlayer.RenderTop | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 783 EntityPlayer.ReplaceCostumeSprite | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 784 EntityPlayer.RerollAllCollectibles | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 785 EntityPlayer.ResetDamageCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 786 EntityPlayer.ResetItemState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 787 EntityPlayer.ResetPlayer | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 788 EntityPlayer.RespawnFamiliars | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 789 EntityPlayer.Revive | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 790 EntityPlayer.ReviveCoopGhost | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 791 EntityPlayer.SalvageCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 792 EntityPlayer.SalvageCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 793 EntityPlayer.SamsonBerserkCharge | Host：模拟计数／冷却／动作生产／历史 | 不是本轮实际Render所需的五个充能计数或DamageCooldown；最终Sprite／已产生实体／HUD结果承接显示，不复制邻接计时器。 |
| 794 EntityPlayer.SecondaryActiveItem | P4：主动槽既有Item | 属于已有主动槽值；只保留源目录别名，不增加第五槽。 |
| 795 EntityPlayer.SetActionHoldDrop | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 796 EntityPlayer.SetActiveCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 797 EntityPlayer.SetActiveVarData | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 798 EntityPlayer.SetBagOfCraftingContent | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 799 EntityPlayer.SetBagOfCraftingOutput | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 800 EntityPlayer.SetBagOfCraftingSlot | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 801 EntityPlayer.SetBlackHeart | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 802 EntityPlayer.SetBladderCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 803 EntityPlayer.SetBlinkLockTime | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 804 EntityPlayer.SetBloodCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 805 EntityPlayer.SetBloodLustCounter | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 806 EntityPlayer.SetBombPlaceDelay | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 807 EntityPlayer.SetCambionConceptionState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 808 EntityPlayer.SetCanShoot | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 809 EntityPlayer.SetCard | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 810 EntityPlayer.SetCharmOfTheVampireKills | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 811 EntityPlayer.SetConceptionFamiliarFlags | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 812 EntityPlayer.SetControllerIndex | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 813 EntityPlayer.SetD8DamageModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 814 EntityPlayer.SetD8FireDelayModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 815 EntityPlayer.SetD8RangeModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 816 EntityPlayer.SetD8SpeedModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 817 EntityPlayer.SetDamageModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 818 EntityPlayer.SetDonateLuck | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 819 EntityPlayer.SetEdenDamage | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 820 EntityPlayer.SetEdenFireDelay | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 821 EntityPlayer.SetEdenLuck | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 822 EntityPlayer.SetEdenRange | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 823 EntityPlayer.SetEdenShotSpeed | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 824 EntityPlayer.SetEdenSpeed | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 825 EntityPlayer.SetEveSumptoriumCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 826 EntityPlayer.SetFireDelayModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 827 EntityPlayer.SetFootprintColor | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 828 EntityPlayer.SetForceCamoEffect | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 829 EntityPlayer.SetForgottenSwapFormCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 830 EntityPlayer.SetFullHearts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 831 EntityPlayer.SetGnawedLeafTimer | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 832 EntityPlayer.SetHallowedGroundCountdown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 833 EntityPlayer.SetHeadDirection | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 834 EntityPlayer.SetHeadDirectionLockTime | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 835 EntityPlayer.SetImExcitedSpeedupCountdown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 836 EntityPlayer.SetImmaculateConceptionState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 837 EntityPlayer.SetInnateCollectibleCount | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 838 EntityPlayer.SetInnateCollectibleGroup | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 839 EntityPlayer.SetInnateTrinketCount | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 840 EntityPlayer.SetInnateTrinketGroup | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 841 EntityPlayer.SetItemState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 842 EntityPlayer.SetItemStateCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 843 EntityPlayer.SetKeepersSackBonus | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 844 EntityPlayer.SetLaserColor | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 845 EntityPlayer.SetLuckModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 846 EntityPlayer.SetMaggyHealthDrainCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 847 EntityPlayer.SetMaggySwingCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 848 EntityPlayer.SetMawOfTheVoidCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 849 EntityPlayer.SetMaxBladderCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 850 EntityPlayer.SetMegaBlastDuration | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 851 EntityPlayer.SetMinDamageCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 852 EntityPlayer.SetMontezumaRevengeCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 853 EntityPlayer.SetNextUrethraBlockFrame | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 854 EntityPlayer.SetPill | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 855 EntityPlayer.SetPlanCKillCountdown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 856 EntityPlayer.SetPocketActiveItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 857 EntityPlayer.SetPonyCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 858 EntityPlayer.SetPoopSpell | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 859 EntityPlayer.SetPotatoPeelerUses | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 860 EntityPlayer.SetPurityState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 861 EntityPlayer.SetRUAWizardTimer | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 862 EntityPlayer.SetRedStewBonusDuration | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 863 EntityPlayer.SetRevelationCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 864 EntityPlayer.SetRockBottomDamage | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 865 EntityPlayer.SetRockBottomLuck | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 866 EntityPlayer.SetRockBottomMaxFireDelay | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 867 EntityPlayer.SetRockBottomMoveSpeed | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 868 EntityPlayer.SetRockBottomShotSpeed | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 869 EntityPlayer.SetRockBottomTearRange | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 870 EntityPlayer.SetShootingCooldown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 871 EntityPlayer.SetShotSpeedModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 872 EntityPlayer.SetSoulCharge | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 873 EntityPlayer.SetSpeedModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 874 EntityPlayer.SetSuplexAimCountdown | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 875 EntityPlayer.SetSuplexLandPosition | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 876 EntityPlayer.SetSuplexState | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 877 EntityPlayer.SetSuplexTargetPosition | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 878 EntityPlayer.SetTargetTrapDoor | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 879 EntityPlayer.SetTearDisplacement | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 880 EntityPlayer.SetTearPoisonDamage | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 881 EntityPlayer.SetTearRangeModifier | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 882 EntityPlayer.SetUrethraBlock | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 883 EntityPlayer.SetWeapon | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 884 EntityPlayer.ShootBlueCandle | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 885 EntityPlayer.ShootRedCandle | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 886 EntityPlayer.ShootRedCandle | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 887 EntityPlayer.ShotSpeed | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 888 EntityPlayer.ShuffleCostumes | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 889 EntityPlayer.SpawnAquariusCreep | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 890 EntityPlayer.SpawnClot | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 891 EntityPlayer.SpawnMawOfVoid | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 892 EntityPlayer.SpawnSaturnusTears | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 893 EntityPlayer.StopExtraAnimation | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 894 EntityPlayer.SwapActiveItems | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 895 EntityPlayer.SwapForgottenForm | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 896 EntityPlayer.SyncConsumableCounts | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 897 EntityPlayer.TearColor | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 898 EntityPlayer.TearFallingAcceleration | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 899 EntityPlayer.TearFallingSpeed | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 900 EntityPlayer.TearFlags | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 901 EntityPlayer.TearHeight | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 902 EntityPlayer.TearRange | 最终显示缓存：第8步 StatHUD 实际显示值／文本 | 这些原生属性是玩法生产输入；P1只保留CanFly，属性栏用正常Host已产出的最终显示。；未显示的属性不因公开API存在而发送；六项原生stats不整组复制。 |
| 903 EntityPlayer.TearsOffset | Host：攻击模板／生成输入 | Tear、Laser等已产生实体按自身布局同步；Player不复制攻击模板以重演射击。 |
| 904 EntityPlayer.Teleport | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 905 EntityPlayer.ThrowBlueSpider | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 906 EntityPlayer.ThrowFriendlyDip | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 907 EntityPlayer.ThrowHeldEntity | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 908 EntityPlayer.TriggerBookOfVirtues | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 909 EntityPlayer.TriggerRoomClear | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 910 EntityPlayer.TryAddToBagOfCrafting | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 911 EntityPlayer.TryDecreaseGlowingHourglassUses | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 912 EntityPlayer.TryFakeDeath | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 913 EntityPlayer.TryForgottenThrow | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 914 EntityPlayer.TryHoldEntity | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 915 EntityPlayer.TryHoldTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 916 EntityPlayer.TryPreventDeath | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 917 EntityPlayer.TryRemoveCollectibleCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 918 EntityPlayer.TryRemoveNullCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 919 EntityPlayer.TryRemoveSmeltedTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 920 EntityPlayer.TryRemoveTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 921 EntityPlayer.TryRemoveTrinketCostume | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 922 EntityPlayer.TryUseKey | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 923 EntityPlayer.UnblockCollectible | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 924 EntityPlayer.UnblockTrinket | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 925 EntityPlayer.UpdateCanShoot | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 926 EntityPlayer.UpdateIsaacPregnancy | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 927 EntityPlayer.UseActiveItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 928 EntityPlayer.UseActiveItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 929 EntityPlayer.UseActiveItem | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 930 EntityPlayer.UseCard | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 931 EntityPlayer.UsePill | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 932 EntityPlayer.UsePoopSpell | Host操作/查询：EntityPlayer原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 933 EntityPlayer.VoidHasCollectible | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 934 EntityPlayer.WillPlayerRevive | Host：恢复／历史／管理关系／模拟对象发现 | 原始历史、储存体和发现查询留Host；已产生实体及C/S结构用显式owner同步。 |
| 1244 EntityPlayer.GetTearsCap | Host：EvaluateItems／属性生产／customCache输入 | 属性加成、倍率和cap留Host，真实StatHUD显示由第8步最终缓存承接；不在Client跑EvaluateItems或Lua回调。 |

### History

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 935 History.GetCollectiblesHistory | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 936 History.RemoveHistoryItemByIndex | Host操作/查询：History原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 937 History.SearchCollectibles | Host操作/查询：History原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 938 History.SearchTrinkets | Host操作/查询：History原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

### HistoryItem

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 939 HistoryItem.GetItemID | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 940 HistoryItem.GetItemPoolType | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 941 HistoryItem.GetLevelStage | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 942 HistoryItem.GetRoomType | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 943 HistoryItem.GetStageType | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 944 HistoryItem.GetTime | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |
| 945 HistoryItem.IsTrinket | 最终显示缓存：第8步HistoryHUD两份0x28条目／inventory图像 | 原始获得史、时间、池、关卡和16索引不随Player复制；仅实际显示条目所需字段按Presentation独立投影。；旧HistoryItem公共API不是最终图像条目的ABI声明，不由同名成员猜布局。 |

### Image

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 946 Image.GetHeight | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 947 Image.GetName | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 948 Image.GetPaddedHeight | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 949 Image.GetPaddedWidth | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 950 Image.GetTexelRegion | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 951 Image.GetWidth | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 952 Image.Render | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 953 Image.RenderWithShader | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |

### ItemConfigCard

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 954 ItemConfigCard.AchievementID | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 955 ItemConfigCard.AnnouncerDelay | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 956 ItemConfigCard.AnnouncerVoice | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 957 ItemConfigCard.CardType | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 958 ItemConfigCard.ClearAvailabilityCondition | Host操作/查询：ItemConfigCard原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 959 ItemConfigCard.Description | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 960 ItemConfigCard.GetAvailabilityCondition | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |
| 961 ItemConfigCard.GreedModeAllowed | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 962 ItemConfigCard.Hidden | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 963 ItemConfigCard.HudAnim | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 964 ItemConfigCard.ID | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 965 ItemConfigCard.InitialWeight | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |
| 966 ItemConfigCard.IsAvailable | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |
| 967 ItemConfigCard.IsCard | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 968 ItemConfigCard.IsRune | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 969 ItemConfigCard.MimicCharge | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 970 ItemConfigCard.ModdedCardFront | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 971 ItemConfigCard.Name | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 972 ItemConfigCard.PickupSubtype | 本地资源：匹配版本的ItemConfigCard配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 973 ItemConfigCard.SetAvailabilityCondition | Host操作/查询：ItemConfigCard原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 974 ItemConfigCard.Weight | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |

### ItemConfigCostume

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 975 ItemConfigCostume.Anm2Path | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 976 ItemConfigCostume.HasOverlay | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 977 ItemConfigCostume.HasSkinAlt | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 978 ItemConfigCostume.ID | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 979 ItemConfigCostume.IsFlying | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 980 ItemConfigCostume.OverwriteColor | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 981 ItemConfigCostume.Priority | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 982 ItemConfigCostume.SkinColor | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 983 ItemConfigCostume.SkinColor | 本地资源：匹配版本的ItemConfigCostume配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |

### ItemConfigItem

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 984 ItemConfigItem.AchievementID | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 985 ItemConfigItem.AddBlackHearts | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 986 ItemConfigItem.AddBombs | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 987 ItemConfigItem.AddCoins | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 988 ItemConfigItem.AddCostumeOnPickup | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 989 ItemConfigItem.AddCustomCacheTag | Host操作/查询：ItemConfigItem原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 990 ItemConfigItem.AddCustomTag | Host操作/查询：ItemConfigItem原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 991 ItemConfigItem.AddHearts | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 992 ItemConfigItem.AddKeys | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 993 ItemConfigItem.AddMaxHearts | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 994 ItemConfigItem.AddSoulHearts | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 995 ItemConfigItem.CacheFlags | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 996 ItemConfigItem.ChargeType | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 997 ItemConfigItem.ClearEffectsOnRemove | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 998 ItemConfigItem.Costume | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 999 ItemConfigItem.CraftingQuality | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1000 ItemConfigItem.Description | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1001 ItemConfigItem.DevilPrice | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1002 ItemConfigItem.Discharged | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1003 ItemConfigItem.GetCustomCacheTags | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1004 ItemConfigItem.GetCustomTags | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1005 ItemConfigItem.GfxFileName | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1006 ItemConfigItem.HasCustomCacheTag | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1007 ItemConfigItem.HasCustomTag | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1008 ItemConfigItem.HasTags | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1009 ItemConfigItem.Hidden | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1010 ItemConfigItem.ID | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1011 ItemConfigItem.InitCharge | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1012 ItemConfigItem.IsAvailable | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |
| 1013 ItemConfigItem.IsCollectible | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1014 ItemConfigItem.IsNull | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1015 ItemConfigItem.IsTrinket | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1016 ItemConfigItem.MaxCharges | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1017 ItemConfigItem.MaxCooldown | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1018 ItemConfigItem.Name | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1019 ItemConfigItem.PassiveCache | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1020 ItemConfigItem.PersistentEffect | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1021 ItemConfigItem.Quality | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1022 ItemConfigItem.RemoveCustomCacheTag | Host操作/查询：ItemConfigItem原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1023 ItemConfigItem.RemoveCustomTag | Host操作/查询：ItemConfigItem原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1024 ItemConfigItem.ShopPrice | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1025 ItemConfigItem.Special | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1026 ItemConfigItem.Tags | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1027 ItemConfigItem.Type | 本地资源：匹配版本的ItemConfigItem配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |

### ItemConfigPillEffect

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1028 ItemConfigPillEffect.AchievementID | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1029 ItemConfigPillEffect.AnnouncerDelay | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1030 ItemConfigPillEffect.AnnouncerVoice | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1031 ItemConfigPillEffect.AnnouncerVoiceSuper | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1032 ItemConfigPillEffect.EffectClass | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1033 ItemConfigPillEffect.EffectSubClass | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1034 ItemConfigPillEffect.GreedModeAllowed | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1035 ItemConfigPillEffect.ID | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1036 ItemConfigPillEffect.IsAvailable | Host：解锁／进度／池权重 | 资源资格和动态池权重不得在Client用本机存档重算；实际显示只用Host最终选择。 |
| 1037 ItemConfigPillEffect.MimicCharge | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |
| 1038 ItemConfigPillEffect.Name | 本地资源：匹配版本的ItemConfigPillEffect配置表 | 配置身份引用由P3/P4/P6/P8声明，配置内容不按每个身体重复下行；不运行拾取效果。 |

### KColor

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1039 KColor.Alpha | 上下文投影/派生：alpha | 公开 Alpha 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1040 KColor.Blue | 上下文投影/派生：blue | 公开 Blue 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1041 KColor.Green | 上下文投影/派生：green | 公开 Green 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1042 KColor.KColor | 本地值操作：KColor值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1043 KColor.Red | 上下文投影/派生：red | 公开 Red 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |

### LayerState

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1044 LayerState.ClearCustomChampionShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1045 LayerState.ClearCustomShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1046 LayerState.GetBlendMode | 上下文投影/派生：blend_mode | 直接读取 BlendMode 五 DWORD。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1047 LayerState.GetColor | 上下文投影/派生：color | 直接读取图层 ColorMod。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1048 LayerState.GetCropOffset | 上下文投影/派生：crop_offset | 直接读取裁剪 Vector。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1049 LayerState.GetDefaultSpritesheetPath | 上下文投影/派生：@owner_file,@element_index | 由所属 ANM2 文件与数组层序定位默认贴图路径。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1050 LayerState.GetFlipX | 上下文投影/派生：flip_x | 读取水平翻转。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1051 LayerState.GetFlipY | 上下文投影/派生：flip_y | 读取垂直翻转。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1052 LayerState.GetLayerID | 上下文投影/派生：@owner_file,@element_index | J273/SDK 按所属 ANM2 与层顺序给出公开层 ID。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1053 LayerState.GetName | 上下文投影/派生：@owner_file,@element_index | 由所属 ANM2 文件及层定义读取名称。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1054 LayerState.GetPos | 上下文投影/派生：position | 直接读取图层位置。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1055 LayerState.GetRenderFlags | 上下文投影/派生：render_flags | 读取渲染 flags。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1056 LayerState.GetRotation | 上下文投影/派生：rotation | 直接读取旋转角度。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1057 LayerState.GetSize | 上下文投影/派生：size | 直接读取尺寸。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1058 LayerState.GetSpritesheet | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1059 LayerState.GetSpritesheetPath | 上下文投影/派生：spritesheet_path | 读取 +0x08 的路径字符串；不等价于实际 image smart pointer。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1060 LayerState.GetWrapSMode | 上下文投影/派生：wrap_s_mode | 读取 S 环绕模式。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1061 LayerState.GetWrapTMode | 上下文投影/派生：wrap_t_mode | 读取 T 环绕模式。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1062 LayerState.HasCustomChampionShader | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1063 LayerState.HasCustomShader | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1064 LayerState.IsVisible | 上下文投影/派生：visible | 读取可见标志。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1065 LayerState.SetColor | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1066 LayerState.SetCropOffset | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1067 LayerState.SetCustomChampionShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1068 LayerState.SetCustomShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1069 LayerState.SetFlipX | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1070 LayerState.SetFlipY | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1071 LayerState.SetPos | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1072 LayerState.SetRenderFlags | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1073 LayerState.SetRotation | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1074 LayerState.SetSize | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1075 LayerState.SetSpritesheet | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1076 LayerState.SetVisible | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1077 LayerState.SetWrapSMode | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1078 LayerState.SetWrapTMode | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |

### NullFrame

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1079 NullFrame.GetColor | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 1080 NullFrame.GetPos | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 1081 NullFrame.GetRotation | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 1082 NullFrame.GetScale | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |
| 1083 NullFrame.IsVisible | 本地资源：已绑定ANM／Image真实资源及其静态元数据 | 帧、层、NullFrame和Image尺寸由匹配资源提供；引用与生命周期仍由实际Sprite/Image owner持有。；Image实际资源身份和路径是两层，不把同名文件视为同一动态Image。 |

### PocketItem

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1084 PocketItem.GetSlot | P3：PocketItem+0 ID | GetSlot读取_id，不是口袋位置；active类型为ActiveSlot+1。 |
| 1085 PocketItem.GetType | P3：PocketItem+4 type | 空ID仍可能保留旧type，按原生当前位值保留。 |

### PosVel

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1086 PosVel.Position | 继承E层：Entity.position／velocity | 同一对实体向量，不新增Player副本。 |
| 1087 PosVel.Velocity | 继承E层：Entity.position／velocity | 同一对实体向量，不新增Player副本。 |

### QueueItemData

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1088 QueueItemData.Charge | Host：队列Charge／Touched生产 | 只保留当前实际queued饰品绘制读集；不能声称重建完整公开描述。 |
| 1089 QueueItemData.Item | P3（裁剪）：queued饰品config＋golden | 只保留当前实际queued饰品绘制读集；不能声称重建完整公开描述。 |
| 1090 QueueItemData.Touched | Host：队列Charge／Touched生产 | 只保留当前实际queued饰品绘制读集；不能声称重建完整公开描述。 |

### Shape

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1091 Shape.Capsule | Host操作/查询：Shape原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1092 Shape.Circle | Host操作/查询：Shape原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1093 Shape.GetTimeout | Host/本地：debug/碰撞Shape owner | 本轮Player状态没有独立Shape倒计时展示消费者，不复制调试对象。 |
| 1094 Shape.SetTimeout | Host操作/查询：Shape原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

### Sprite

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1095 Sprite.ClearCustomChampionShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1096 Sprite.ClearCustomShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1097 Sprite.Color | 上下文投影/派生：color | ANM2 全局 ColorMod。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1098 Sprite.Continue | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1099 Sprite.ContinueOverlay | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1100 Sprite.Copy | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1101 Sprite.FlipX | 上下文投影/派生：flip_x | 水平翻转。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1102 Sprite.FlipY | 上下文投影/派生：flip_y | 垂直翻转。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1103 Sprite.GetAllAnimationData | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1104 Sprite.GetAllLayers | 上下文投影/派生：layer_states | 读取有序 LayerState 数组。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1105 Sprite.GetAnimation | 上下文投影/派生：main_state | 映射主动画选择/状态；底层动画选择由 AnimationState.active_animation 资源项表示。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1106 Sprite.GetAnimationData | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1107 Sprite.GetCurrentAnimationData | 上下文投影/派生：main_state | 主动画状态内的当前 AnimationData 选择。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1108 Sprite.GetDefaultAnimation | 上下文投影/派生：default_animation_name | 默认动画名字符串。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1109 Sprite.GetDefaultAnimationName | 上下文投影/派生：default_animation_name | 默认动画名字符串。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1110 Sprite.GetFilename | 上下文投影/派生：file_name | ANM2 文件资源身份。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1111 Sprite.GetFrame | 上下文投影/派生：main_state | 主动画当前帧及选择器由 main_state 表示。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1112 Sprite.GetLayer | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1113 Sprite.GetLayer | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1114 Sprite.GetLayerCount | 上下文投影/派生：@owner_file | 层数来自所属 ANM2 资源文件。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1115 Sprite.GetLayerFrameData | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1116 Sprite.GetNullFrame | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1117 Sprite.GetOverlayAnimation | 上下文投影/派生：overlay_state | 叠加动画状态。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1118 Sprite.GetOverlayAnimationData | 上下文投影/派生：overlay_state | 叠加动画当前资源项。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1119 Sprite.GetOverlayFrame | 上下文投影/派生：overlay_state | 叠加动画当前帧及选择器。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1120 Sprite.GetOverlayLayerFrameData | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1121 Sprite.GetOverlayNullFrame | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1122 Sprite.GetRenderFlags | 上下文投影/派生：render_flags | ANM2 渲染 flags。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1123 Sprite.GetSpritesheet | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1124 Sprite.GetTexel | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1125 Sprite.HasCustomChampionShader | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1126 Sprite.HasCustomShader | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1127 Sprite.IsEventTriggered | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1128 Sprite.IsFinished | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1129 Sprite.IsFinished | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1130 Sprite.IsLoaded | 上下文投影/派生：file_name | 成功应用后由可空文件资源选择与本地owner准备结果重建；加载失败不回复AppliedAck，不能直接写Host loaded或将失败当Null。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1131 Sprite.IsOverlayEventTriggered | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1132 Sprite.IsOverlayFinished | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1133 Sprite.IsOverlayFinished | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1134 Sprite.IsOverlayPlaying | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1135 Sprite.IsOverlayPlaying | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1136 Sprite.IsPlaying | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1137 Sprite.IsPlaying | 本地资源/派生：ANM2／LayerState已安装资源、姿态、Image及shader绑定 | 只在实际拥有者已投影时提供本地便利访问；事件触发和播放推进不在Client重演。；无新的任意参数查询字典；shader读取实际绑定而非仅比较路径。 |
| 1138 Sprite.Load | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1139 Sprite.LoadGraphics | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1140 Sprite.Offset | 上下文投影/派生：offset | 绘制偏移。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1141 Sprite.Play | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1142 Sprite.PlayOverlay | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1143 Sprite.PlayRandom | Host操作/查询：Sprite原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1144 Sprite.PlaybackSpeed | 上下文投影/派生：playback_speed | 绘制插值读取的播放速度；与is_playing及anim_frame共同保留，不授权本地Update。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1145 Sprite.Reload | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1146 Sprite.RemoveOverlay | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1147 Sprite.Render | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 1148 Sprite.RenderLayer | 本地绘制入口：第8/13步批准的原生绘制适配 | 入口名不是纯展示保证；须隔离生产副作用并借用已提交owner，调用本身不在线重放。 |
| 1149 Sprite.ReplaceSpritesheet | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1150 Sprite.ReplaceSpritesheet | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1151 Sprite.ReplaceSpritesheet | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1152 Sprite.Reset | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1153 Sprite.Rotation | 上下文投影/派生：rotation | 旋转角度。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1154 Sprite.Scale | 上下文投影/派生：scale | 绘制缩放。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1155 Sprite.SetAnimation | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1156 Sprite.SetCustomChampionShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1157 Sprite.SetCustomShader | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1158 Sprite.SetFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1159 Sprite.SetFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1160 Sprite.SetLastFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1161 Sprite.SetLayerFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1162 Sprite.SetOverlayAnimation | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1163 Sprite.SetOverlayFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1164 Sprite.SetOverlayFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1165 Sprite.SetOverlayLayerFrame | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1166 Sprite.SetOverlayRenderPriority | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1167 Sprite.SetRenderFlags | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1168 Sprite.SetSpritesheet | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1169 Sprite.Sprite | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1170 Sprite.Sprite | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1171 Sprite.Stop | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1172 Sprite.Stop | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1173 Sprite.StopOverlay | 本地owner操作：最终ANM／LayerState／Image资源与姿态安装 | 正式writer按已声明值构造／装载／设置；旧公开操作不作为网络动作队列，不能触发玩法生产。 |
| 1174 Sprite.Update | Host操作/查询：Sprite原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1175 Sprite.WasEventTriggered | Host操作/查询：Sprite原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1176 Sprite.WasOverlayEventTriggered | Host操作/查询：Sprite原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

### TemporaryEffect

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1177 TemporaryEffect.Cooldown | P8（条件）：仅collectible704的cooldown | 758952和836E35有实际正cooldown读；其他效果cooldown留Host。 |
| 1178 TemporaryEffect.Count | P8：有序效果的config／count | 保留次序、非空配置和首匹配语义，不能按ID汇总。 |
| 1179 TemporaryEffect.Item | P8：有序效果的config／count | 保留次序、非空配置和首匹配语义，不能按ID汇总。 |

### TemporaryEffects

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1180 TemporaryEffects.AddCollectibleEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1181 TemporaryEffects.AddNullEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1182 TemporaryEffects.AddTrinketEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1183 TemporaryEffects.ClearEffects | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1184 TemporaryEffects.GetCollectibleEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1185 TemporaryEffects.GetCollectibleEffectNum | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1186 TemporaryEffects.GetEffectsList | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1187 TemporaryEffects.GetNullEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1188 TemporaryEffects.GetNullEffectNum | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1189 TemporaryEffects.GetTrinketEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1190 TemporaryEffects.GetTrinketEffectNum | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1191 TemporaryEffects.HasCollectibleEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1192 TemporaryEffects.HasNullEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1193 TemporaryEffects.HasTrinketEffect | P8（裁剪）：disabled＋有序效果列表config/count，704附cooldown | 列表长度／查找由本地已安装列表提供，不执行增删或effect Update。；不承诺非704返回对象的完整cooldown；EffectList.Get即使旧NotReplicated也只是本地索引。 |
| 1194 TemporaryEffects.RemoveCollectibleEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1195 TemporaryEffects.RemoveNullEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1196 TemporaryEffects.RemoveTrinketEffect | Host操作/查询：TemporaryEffects原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

### Vector

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1197 Vector.Clamp | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1198 Vector.Clamped | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1199 Vector.Cross | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1200 Vector.Distance | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1201 Vector.DistanceSquared | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1202 Vector.Dot | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1203 Vector.FromAngle | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1204 Vector.FromAngle | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1205 Vector.GetAngleDegrees | 上下文投影/派生：x,y | 由 X/Y 计算角度，不是独立存储字段。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1206 Vector.Length | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1207 Vector.LengthSquared | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1208 Vector.Lerp | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1209 Vector.Normalize | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1210 Vector.Normalized | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1211 Vector.Resize | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1212 Vector.Resized | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1213 Vector.Rotated | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1214 Vector.Vector | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1215 Vector.X | 上下文投影/派生：x | 实例 X 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1216 Vector.Y | 上下文投影/派生：y | 实例 Y 分量。；只适用于所属对象已声明的同步域；嵌于Host容器或原版通信对象时整体不发送。 |
| 1217 Vector.__add | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1218 Vector.__div | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1219 Vector.__div | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1220 Vector.__mul | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1221 Vector.__mul | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1222 Vector.__mul | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1223 Vector.__sub | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1224 Vector.__tostring | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |
| 1225 Vector.__unm | 本地值操作：Vector值计算／格式化 | 构造、运算或修改本地值不是独立状态；只有所属字段被声明时结果才随该owner投影。 |

### Weapon

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1226 Weapon.ClearItemAnim | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1227 Weapon.GetCharge | P5（条件）：GetCharge：按实际weapon_type充能公式和mask | 十种类型保留公式实际输入；5/6/8/12/15恒零不传三值；不是通用旧Weapon全字段。 |
| 1228 Weapon.GetDirection | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1229 Weapon.GetFireDelay | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1230 Weapon.GetMainEntity | P5（条件）：仅Whip15 +44 StateRef | 不能把公共base+8或其他攻击引用当所有类型主实体。 |
| 1231 Weapon.GetMaxCharge | P5（条件）：GetMaxCharge：按实际weapon_type充能公式和mask | 十种类型保留公式实际输入；5/6/8/12/15恒零不传三值；不是通用旧Weapon全字段。 |
| 1232 Weapon.GetMaxFireDelay | P5（条件）：GetMaxFireDelay：按实际weapon_type充能公式和mask | 十种类型保留公式实际输入；5/6/8/12/15恒零不传三值；不是通用旧Weapon全字段。 |
| 1233 Weapon.GetModifiers | P5（条件）：GetModifiers：按实际weapon_type充能公式和mask | 十种类型保留公式实际输入；5/6/8/12/15恒零不传三值；不是通用旧Weapon全字段。 |
| 1234 Weapon.GetNumFired | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1235 Weapon.GetOwner | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1236 Weapon.GetWeaponType | P5：Weapon+30类型 | 动态工厂与显示公式身份。 |
| 1237 Weapon.IsAxisAligned | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1238 Weapon.IsItemAnimFinished | Host：Weapon模拟／攻击对象管理 | 方向、冷却、发射数、axis与动画生产不属于P5显示读集；Owner指针仅本地生命周期。 |
| 1239 Weapon.PlayItemAnim | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1240 Weapon.SetCharge | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1241 Weapon.SetFireDelay | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1242 Weapon.SetHeadLockTime | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1243 Weapon.SetModifiers | Host操作/查询：Weapon原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

### PlayerManager

| 原行／ID | 去向 | 理由／适用边界 |
|---|---|---|
| 1245 PlayerManager.AnyPlayerTypeHasBirthright | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1246 PlayerManager.AnyPlayerTypeHasCollectible | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1247 PlayerManager.AnyPlayerTypeHasTrinket | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1248 PlayerManager.AnyoneHasCollectible | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1249 PlayerManager.AnyoneHasTrinket | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1250 PlayerManager.AnyoneIsPlayerType | 本地派生（限C/S）：P7 roster／资格／组与player_type | 按提交的C/S结构查找；不复用原版Online Manager或设备判断。；原生所有玩法遍历资格并非一律等同C/S roster。 |
| 1251 PlayerManager.FirstBirthrightOwner | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1252 PlayerManager.FirstCollectibleOwner | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1253 PlayerManager.FirstPlayerByType | 本地派生（限C/S）：P7 roster／资格／组与player_type | 按提交的C/S结构查找；不复用原版Online Manager或设备判断。；原生所有玩法遍历资格并非一律等同C/S roster。 |
| 1254 PlayerManager.FirstTrinketOwner | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1255 PlayerManager.GetEsauJrState | Host：Game+1BB28四槽恢复/发现 | 703主动VarData和真实绘制并不要求Client备份四槽；不复制原版保存体。 |
| 1256 PlayerManager.GetNumCollectibles | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1257 PlayerManager.GetPlayers | 本地派生（限C/S）：P7 roster／资格／组与player_type | 按提交的C/S结构查找；不复用原版Online Manager或设备判断。；原生所有玩法遍历资格并非一律等同C/S roster。 |
| 1258 PlayerManager.GetRandomCollectibleOwner | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1259 PlayerManager.GetRandomTrinketOwner | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1260 PlayerManager.GetTotalTrinketMultiplier | 最终显示决定：具名展示消费者的全局bool/int/StateRef结果 | 跨身体collectible／trinket及first-owner查询只保留实际绘制用途与确切参数；不恢复任意PlayerManager遍历API。 |
| 1261 PlayerManager.IsCoopPlay | 本地派生（限C/S）：P7 roster／资格／组与player_type | 按提交的C/S结构查找；不复用原版Online Manager或设备判断。；原生所有玩法遍历资格并非一律等同C/S roster。 |
| 1262 PlayerManager.RemoveCoPlayer | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1263 PlayerManager.SpawnCoPlayer2 | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1264 PlayerManager.SpawnSelectedBaby | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |
| 1265 PlayerManager.TriggerRoomClear | Host操作/查询：PlayerManager原有生产／变更／管理域 | 不传操作调用，不让Client重放；需要的当前结果按具名状态或最终显示owner安装。 |

## 8. 完成边界

本轮完成逐行归属与旧Entity目录交叉索引；没有把旧705个ReplicatedState当作705个必发值，没有声称旧公共查询全部可在Client重放。后续第5步仍需迁入唯一player.json、分配正式layout与生成；第8/10/13/14步落实具名最终缓存、采集、安装、隔离及旧路径退出，第15步再统一整体验收。
