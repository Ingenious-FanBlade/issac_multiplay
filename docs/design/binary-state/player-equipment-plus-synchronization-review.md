# ActiveItemDesc、Weapon、TemporaryEffect 与 Plus 展示必要性复核

日期：2026-11-09（Asia/Singapore）。属于第 4／5 步，布局版本 **39**。依据本地原版／REPENTOGON Wiki、官方 1.1.3 源码及 J273 IDA 静态分析。没有构建、测试、运行游戏或哈希校验；第 15 步仍是唯一功能整体验收。本轮修订唯一源、生成表和后续合同，不表示 Player、HUD 或正式同步已经接通。

## 1. 采用结果

| 类型 | 当前决定 |
| --- | --- |
| ActiveItemDesc 12 | Item、Charge、BatteryCharge、SubCharge、PartialCharge、VarData 有 HUD 消费者，保留24B；item_pool、TimedRechargeCooldown 留 Host，32→24B，四槽128→96B。 |
| Weapon 13 | 保留 MaxFireDelay、Charge、Modifiers、WeaponType 共16B；owner、FireDelay、Direction、axis_parameters、NumFired 留 Host，42→16B。不能把0x44基础前缀当作具体动态分配大小。 |
| WeaponUmbilicalWhip 135（新增） | 明确实际0x48分配，另保留 `+44` 主实体引用，18B；它有 Familiar Render／Shadow 的跨域消费者，不得随其他玩法关系一起删除。 |
| TemporaryEffect 14 | 配置身份、Count、Cooldown 全部有展示消费者，保持10B；原末4B未知不变。 |
| TemporaryEffects 136（新增） | Player P8 的外层容器：同步 disabled 与完整有序列表；CacheFlag mask／evaluate 请求留 Host、Player回指本地。最短3B，另10B／元素；外层2B未知、每外部元素4B未知分别保留。 |
| EntityPlus 15及其派生 | 保留飞行三态和完整 nullable 水面覆盖，共9B；同版本、真实本地holder／data及标准optional赋值不可省略。 |
| KnifePlus 17 | hitboxSource只被近战生成补丁和Lua getter／setter使用，留 Host；子块1预留、11→9B。移除正式 `ReadKnifeExtension` 的身份采集接口，公共覆盖继续 `ReadEntityExtension`。 |
| Familiar主体 | 新核 `+D76` 的鞭形态展示抑制字节，独立于IsFollower；新增1B状态，原6B未知拆为已核1B及剩余5B。否则保留Weapon主实体仍无法重现相同绘制选择。 |

Familiar、Tear、Pickup、Laser Plus 已排除的倍率缓存／Player本地地址、initSound、重抽资格、重采样请求保持既有权威／本地归属；本轮不恢复。EntityPlayerPlus 专属字段尚未进入当前二进制唯一源，不借本次复核声称已完成其完整布局、ItemSpoof／自定义cache或Mod适配。

## 2. 静态范围与证据边界

目标 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；下文均为 VA，RVA=VA−`0x400000`。本地资料为 `IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md`、`TemporaryEffect.md`／`TemporaryEffects.md`、REP `Weapon.md`，以及官方 `ActiveItemDesc.zhl`、`Weapon.zhl`、`TemporaryEffects.zhl`、`PlayerHUD.zhl`、`HUD.zhl`、`EntityPlus.h/.cpp`、`LuaEntityKnife.cpp`、`ASMEntityKnife.cpp`。

在此前28个 Entity 展示根上加入 HUD Render `97C300`、PlayerHUD RenderActiveItem `835B40`、RenderGfx `835A80`，再加入Scene水面分派415350，共32根、692个直接可达函数；明确停止 Entity Lua render桥，虚分派另按vtable补核。检查 Player 五Weapon槽、四ActiveItemDesc、TemporaryEffects 的直接偏移及取址，并跟踪独立getter与相对字段读取。PlayerHUD Update `832C10` 单独读作缓存／安装依赖，**不作为可放行的纯展示更新**。不把同位移的其他对象、Lua API存在、旧原值比较或旧writer需求当作网络必要性。

现行结论适用于已核官方展示合同；没有把有限静态调用集合外推为任意Mod／Lua／别名的形式化证明。原字节表中未核区域保持，新增字段各有具体正面消费者。完整 Player 字节表、全部动态Weapon尾部及 HUD资源owner仍归第5／8步；本轮的消费者闭包不能替代它们的实现和第15步实际验收。

## 3. ActiveItemDesc：四槽与HUD

Player `+1574 + slot×0x20`，slot=0—3。`835B40` 明确先检查slot≤3；完整入口使用IDA伪代码和原始指令，未把工具截断预览当成函数全貌。

| 原生成员 | 实际消费者与决定 |
| --- | --- |
| `+0` Item | `835BCD` 取ID，`835BFC` 通过ItemConfig取得图像／充能模式等；Player库存查询 `769E90` 也检查四槽ID。保留，空槽ID=0仍是状态，不从图片缓存猜有无。 |
| `+4` item_pool | `AddCollectible 758F00` 在 `759864` 保存来源池，获得／保存／掉落相关路径读写；当前Player／HUD／装备展示不读此字段。留Host，不由Client创建道具或重选池。 |
| `+8` Charge | `835C10` 读取，用于普通、特殊道具图像和充能条；不能以一般满充电量推导Jar等道具的储量。保留。 |
| `+C` BatteryCharge | `835C1A—835C23` 以 `(slot+0xAC)<<5` 定位，等价于 `Player+1580+slot×32`；与Charge独立用于额外充能层。保留，不能因没有直接`+1580`位移就漏掉。 |
| `+10` SubCharge | `835C3E` 读取，进入特殊充能／覆盖层分支；保留。 |
| `+14` TimedRechargeCooldown | `789E50` 的主动道具更新在 `78A35B` 读取，`78A3E1/78A61D/78A68F/78A710/78A74F/78A9B4` 等设置11／9／15；它是Host充能调度，当前HUD使用最终charge与partial，不读此原值。留Host，不另加计时锚点。 |
| `+18` PartialCharge | `835C76` 以movss读取，参与半透明充能层。保持f32与有限值约束，不恢复SDK错误的int类型，不按Wiki常见0—1范围新增未经证明的全局范围限制。 |
| `+1C` VarData | `7897C0/789810` 从对应槽取出后交 `7896D0` 计算max／min usable charge；`835CCD`、`836036/8360F1/836179/8361B8` 等还有直接特殊图像／计数消费。保留，不能把“额外玩法数据”整体排除。 |

`835B40` 还依赖Player类型、其他Player资源／角色状态、Game challenge、ItemConfig及TemporaryEffects查询。例如 challenge44／item580 在 `835CDF` 读取NullEffect109的count后调整显示。六个描述字段不足以自动重建全HUD；这些外部依赖必须与P2／P3／P8、Run、资源及HUD选择状态同版本准备。

官方 `CustomCallbacks.cpp` 对 `RenderActiveItem` 打开MaxCharge回调缓存，并在 `GetActiveMaxCharge`／`GetActiveMinUsableCharge` 触发1072／1073回调。Client不得因保留六字段就执行Host玩法Lua补值；声明Mod需要的有效结果与表现回调继续由适配合同承接，未核适配不声称完整。官方预／后HUD绘制及图像回调同理遵守表现隔离。

安装四个描述的最终六值，按slot保持次序和空槽，不调用AddCollectible、Use、AddCharge、充能更新或重算。HUD的itemImage／bookImage、chargebar Sprite／flash计数和布局缓存由第8步的资源／展示owner维护，不能放行含输入／Drop／Lua／History副作用的整个 `PlayerHUD::Update 832C10` 来刷新缓存。已排除两原值的旧writer／读回要求在第14步退役。

## 4. Weapon：保留展示纯字段闭包与真实主实体

### Player／HUD／Familiar消费者

`Player::Render 784A60` 在不dead及正确Room绘制mode时进入 `758850`。该helper从 `Player+13DC` 开始检查四槽（slots1—4），先逐槽调用slot7，取首个正比例，落为f32、乘100、转整数后交 `758D50`，再用同样方式检查slot8。它不是对全部五槽求和，也不从slot0起遍历。`758D50` 选择Charging／StartCharged／Charged／Disappear并组织真实进度条Sprite；安装须保留这些资源／动画和临时写入恢复，不画另一个通用矩形替代。

核定16份J273 vtable（含base和Deathmatch；Player原版工厂type1—15中type10／11同为Bone）：slot7十四份为 `9C5E80`，Urn／UmbilicalWhip为常零 `9C4E60`；**全部slot8为9C4E60常零**。`9C5E80` 只读取 `modifiers +34`、`charge +14` 并调用slot6。slot6全量公式已在[Familiar复核](familiar-synchronization-review.md)给出，本轮再次逐函数读取：只用 `max_fire_delay +10` 与modifiers，包括 `9D0A50/9D2A80/9D5150/9D59F0/9D6A80/9D82D0/9DA650/9DC070/9DF230/9DFBA0` 及常零。

因此保留MaxFireDelay／Charge／Modifiers即可供已核原生充能条重建；无需重复发送MaxCharge或两份chargebar结果。保持真实动态类／getter绑定、原生f32落栈位置、阈值与转换语义，不能用一个跨武器通用比值替代公式。Familiar80仍按版本33单独发送最终amount，不恢复其玩法Weapon或重复发送这些字段。

WeaponType另有两个正面消费者：HUD `83606F—836093` 在slot0、再slot1选择首个非空Weapon，判断type12改变图像选择；Familiar `60C190` 也先slot0、再slot1判断type15。它与动态vtable并非可互换身份，尤其type10／11共用Bone vtable，故保留type并作为形态值。五槽的存在性、次序和动态工厂选择仍须在P5完整表达，不能只发送某条充能比例。

| 基类范围 | 决定 |
| --- | --- |
| `+4` owner | 当前上述展示公式不读取，Host关系留Host。Client为本地承载身体构造时所设owner是本地生命周期关系，不是复制Host引用的理由。 |
| `+C` FireDelay | `9C5220` 在非插值更新时读取，按owner速度倍率或1递减；是模拟冷却，留Host。 |
| `+10/+14` MaxFireDelay／Charge | 真正的充能条输入，保留8B。 |
| `+20` Direction | Fire／瞄准及公开查询输入，当前装备展示不用；最终Player方向、Sprite、独立攻击实体各自同步，留Host。 |
| `+28/+2C` axis_parameters | `IsAxisAligned 9C5090` 还依赖owner类型／Player道具条件，不是原生draw输入，留Host；原两个f32和未知业务语义事实不改成可派生常量。 |
| `+30/+34` WeaponType／Modifiers | HUD／Familiar分支与充能公式，保留8B。 |
| `+38` NumFired | Host发射历史和公开查询，当前展示不读，留Host。 |

基类含 `+8` 在内的20B UNKNOWN保持本轮前的表格边界；旧I20对各动态EntityPtr的生命周期信息继续用于安全拆边，但不因为旧writer要求就把每条引用重新上线。GetMaxCharge／IsAxisAligned／GetMainEntity等虚／上下文查询不因此自动获得全局public_owner完整性。

### UmbilicalWhip跨域引用与Familiar D76

`60C190` 在PlayerType32、拥有619、首个slot0／1 WeaponType15、Familiar `is_follower +D74` 为真且 `+D76` 为假时选择借用鞭形态。`60C1F0` 核实际 `Weapon_UmbilicalWhip` RTTI，再调用slot10 GetMainEntity，只有非空且Type3时把该实体交Familiar Render `61E391`／Shadow `61E331` 使用。**这是真实展示关系，不能随其余Weapon玩法引用一起排除。**

工厂 `9CAEE0` 明确 `new(0x48)`、调用base ctor、写vtable `B31BC4`并清 `+44`。COL `B45A08` 指向TypeDescriptor `BD1664`（RVA7D1664）；slot10 `9C4E80` 返回 `this+44`，不同于base常空 `9C4E70`。新增布局135继承44B字节目录并记录尾部EntityPtr，状态为基类16B＋2B引用。不能只按base前缀复制而漏掉此关系，也不能把+8当主实体。

非空目标按真实稳定身份解析，不把helper只消费Type3的分支条件升级为“所有非空目标必为Familiar”的未经证明全局不变量；Client按相同分支处理其他真实类型。先构造目标与资源，再绑定真实EntityPtr；关系撤销由SetReference完成，独立实体仍由其自身owner退休。

`+D76` 原在Familiar的6B UNKNOWN中，但 `60C1D6` 直接读取它决定Render／Shadow分支；Init `607C70` 置0，`60808A/608DD0`、`431B3A/431B86/431C01` 等置1，`5B3660`／`606E30` 为读写此字节的独立方法。本轮只命名其已核展示职责 `whip_draw_suppressed`，不推造完整玩法含义；同步原bool，不执行生成／跟随逻辑推导它。原D77—D7B五字节继续未知。Familiar Player、IsFollower、库存619、PlayerType、P5槽与Whip主实体必须同目标版本，不能先看到引用就绕过其他展示条件。

### 构造与退役

第5步按真实factory／vtable／type维护五个本地Weapon对象，仅安装本轮保留字段和15号主引用。其余已核动态引用保持正常构造的空EntityPtr；旧实例有本地边时须正常撤销，不能memcpy零。`ClearReferences`（如Whip `9D7E10`）会Remove关联实体；`DestroyWeapon 9CADE0`不是无副作用资源清理，不能用于客户端同步替换。沿用已核窄释放：撤自身引用、owner=null，再实际Free，最后清本地Player槽；不级联Remove或改Player／Familiar表现。引用、资源和全部状态同目标版本齐备后发布。

## 5. TemporaryEffect与外层容器

三个成员必须保留，不能因为已有最终costume／属性就删除：

- **配置身份**：`90A7A0` 先检查TemporaryEffects `+11 disabled`，再按记录的ItemConfig类别（1／3／4）与ID找collectible；`90A900` 检类别0和ID找NullEffect。trinket查询亦区分类别2，`7B9E80` 的实际展示可达查询直接检查trinket69。线上资源表必须保留(type,id)，Client解析真实本地ItemConfig，不传指针、不按裸ID混同Null／collectible／trinket。
- **Count**：`90A900` 返回记录 `+4`，HUD `835CDF` 与 Room Light 的实际可达查询读取；Player `769E90` 也经 `90A7A0` 读取118号效果count。存在性查询 `7D1C40` 还会检查Null112，disabled不能被“列表非空”代替。
- **Cooldown**：Player角色27的 `758919—75897B` 找collectible704，`758952` 读记录 `+8`，`758957` 读对应配置 `+78` cooldown，转成进度环amount。它是展示输入，不能仅凭倒计时名称归Host，也不能用Samson其他积量代替。

新增外层布局136，直接对应Player `+1500` 的0x18B：首字为Host cache mask，`+4`为0xC vector头，`+10`为Host evaluate请求，`+11`为展示查询disabled，`+12—13`未知，`+14`为本地Player回指。列表上限沿用16384及全局实际字节预算，不放宽。记录按原顺序完整提交，保留有界项数、配置身份、Count／Cooldown；空集合清除旧元素，disabled与内容同目标版本生效。Host的mask／evaluate不是Client查询结果来源，不复制或执行它们。

Client仅重建正常vector／配置关系并安装disabled，使用本地承载Player回指；不调用Add／RemoveEffect、ClearEffects、EvaluateItems、效果更新／到期、加减costume或生成familiar。Cooldown按已应用帧的最终值直接安装，不在Client递减，也不从未知活动条件制造新锚点。最终属性、变身与costume仍各自同步；TemporaryEffects不等价于假库存。末尾4B原未知仍不读取或复制；外层新增2B未知另列，不被容器头吞掉。

## 6. 各Plus剩余值与安装依赖

| Plus | 仍传输／仍排除内容及依据 |
| --- | --- |
| EntityPlus | 保留isFlyingOverride三态、waterClipInfoFlagsOverride的Null／有效零／完整u32。官方IsFlying hook先看override，否则super；`6B31C3` 在Player的GetWaterClipInfo分支调用IsFlying，影响水面偏移。GetWaterClipInfo hook先super再覆盖bitFlags，Scene分派415350在415398／4153D2／4153EA直接调用它并消费水面／反射参数；Entity槽22实际是GetForwardVector，不能误作WaterClip虚槽；不能只同步effective值而丢null。 |
| FamiliarPlus | 继承9B；倍率回调缓存cachedMultiplier、Player地址cachedMultiplierPlayer仍为权威／本地。倍率对最终尺寸／Sprite的影响由主体保留，不因Getter可以计算就重新触发回调。 |
| KnifePlus | 继承9B；hitboxSource是ASMEntityKnife近战hitbox生成后写入、LuaEntityKnife Get/SetHitboxParentKnife读取／写入的关系。官方全部源码引用点只有这些，没有draw／HUD消费者，J273原生也不存在此REP附加字段。留Host，不再预约身份／安装／比较；块1预留。 |
| TearPlus | 继承9B；initSound仍Host初始化音效覆盖，一次性音效按事件通道，不恢复状态字段。 |
| PickupPlus | 继承9B；canRerollOverride仍Host重抽资格，Client不重抽。 |
| LaserPlus | 继承9B；recalculateSamplesNextUpdate和initSound仍Host。重采样请求会清四个Homing容器，Client不得执行并破坏最终两组几何。 |
| PlayerPlus | 当前唯一源未定义专属派生字节表；公共两项覆盖随Player的E9继续保留，专属ItemSpoof／cache／health／camo／controller等依第5步逐项建模，不恢复或声称裁掉尚未审完的内容。 |

REP holder指针仍拆在Entity `+18E/+192`，指向本地holder，再到具体data。Host只读现有链，前后来源／generation稳定；不能调用会分配的GetEntityPlus getter补缺。没有holder／data或换代是实际Gap，不等于optional未设置。只在engaged时读payload，不读disengaged负载／未知对齐字节。

Client在主线程已正常构造／Init且动态类匹配的本地holder/data上，用当前官方Release ABI的标准optional赋值：飞行只接受-1／0／1；水面只接受0或高32位恰为1，低32位保留全部bits。Reset成nullopt与写false／0不同，不用NPC专属Lua setter代替Player／通用覆盖，也不借setter创建新holder或清空整个派生Plus。

安装须先有同代Entity／Player及相关Room、库存、最终位置／Sprite；水面方法仍执行super，其Player道具62、Pickup Price等原有依赖不因保留override而消失。Nullable覆盖与主体同版本提交后才进入绘制；失败不ACK、不部分发布。已有override从未设到false／零、再到nullopt均须正确。旧Knife本地hitboxSource如有绑定，通过真实EntityPtr owner撤边；不恢复已排除的其他Plus项、不调用原生生成／重抽／采样／音效。

正式 `state/entity_extension.*` 删除没有剩余同步职责的 `ReadKnifeExtension`／KnifeExtensionState／Bindings接口；`ReadEntityExtension` 与两optional解码函数保持。历史实现对提前查成员序号的失败和修复仍原样保留，不因现行关系退役而抹去。

## 7. 变化、交接与唯一验收

本轮移出 ActiveItemDesc两范围、Weapon五范围、KnifePlus一范围，共 **8个原生范围**；对应最小编码各减少8／26／2B。Familiar因新正面消费者增加1B，当前基础613→614B、Knife422→420B；普通Entity E9仍9B。新增135／136布局不改变既有编号，KnifePlus子块1预留。原Weapon20B、TemporaryEffect4B及Plus对齐未知保持；Familiar仅D76有证据后少1B未知，其他未知不归零。所有字节数是格式算术，不是实测带宽或完整类型声明。

第10步只读上线字段，按当前Player装备窗口、动态vtable与来源稳定性采集五槽／四描述／效果列表／Plus，Whip主实体经身份／Pending预约，后台转成员序号，不提前要求新成员已编号。第12步按同版本完整P4／P5／P8解码，校验真正的资源／引用／数量／浮点／nullable；第13步按上述依赖、具体资源和本地生命周期安装。第14步退役已排除原值、旧Weapon多余查询／两份chargebar重复值、Knife身份处理和旧TemporaryEffects mask／evaluate写入；旧Lua／JSON路径本轮未切换，不能把声明修订称为已完成运行时退役。

第15步沿用E09／E25的最终Player/HUD/装备、E29 owner／替换清理、E13／E17非法负载与资源／预算、E28隔离及绘制借用、E27未知、E34同帧展示时钟。覆盖四主动槽与空槽、普通／电池／sub／partial／VarData特殊显示、15种Weapon及type10／11同vtable、两个原生充能条（第二条常零也须保持Disappear语义）、Whip主体引用与D74/D76分支、Berserk704环、Null109／112与trinket69效果、disabled且非空列表、Plus未设／false／零／true／再未设及水面／反射。比较实际上线输入与最终画面，不再比较8个撤销范围；不提前执行测试。
