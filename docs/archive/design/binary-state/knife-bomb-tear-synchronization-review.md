# Knife／Bomb／Tear 攻击、运动与计时参数同步复核

日期：2026-11-09（Asia/Singapore）。第 4 步，布局版本 **34**。本轮只做资料阅读、J273 静态分析、计划与布局源修订及表格生成；没有构建、测试、游戏、注入或部署。原生用途、未知与历史失败保留，不表示第 10／13 步已接线或第 15 步已通过。

## 1. 采用结论

**42 个原生范围留在 Host，不再同步。** 其中包括用户指定的 Knife Charge／MaxDistance、Bomb 爆炸计时／伤害／范围，以及 Tear BaseDamage／KnockbackMultiplier／WaitFrames。Client 不执行原生玩法 Update 或 Interpolate，接收 Host 已求出的成员、位置、公共绘制偏移、Velocity、主／辅助 Sprite 及独立效果；不为重算这些结果再次传输其攻击和运动输入。

| 类型／原生范围 | 排除字段 | 数量／原生字节 |
| --- | --- | --- |
| Knife `+430/+438/+45C/+460/+4C4/+4C8` | Scale、Rotation、MaxDistance、Charge、PathOffset、PathFollowSpeed | 6／24B |
| Knife `+465/+4E5/+4E6/+4EA` | IsSwinging、IsPrismTouched、IsMultidimensionalTouched、IsSpinAttack | 4／4B |
| Bomb `+410/+418/+41C/+420/+428/+42C/+434/+44C` | ExplosionCountdown、ExplosionDamage、FallSpeed、FallAcceleration、RocketAngle、RocketSpeed、Scale、RadiusMultiplier | 8／32B |
| Bomb `+438/+448/+45C` | Flags、IsFetus、IsPrismTouched | 3／18B |
| Tear `+410/+414/+418/+41C/+420/+424` | Height、FallingSpeed、FallingAcceleration、HomingFriction、Scale、BaseScale | 6／24B |
| Tear `+438/+450/+454/+458/+45C/+460/+484/+820` | WaitFrames、TearIndex、BaseDamage、Rotation、ScaleAnimNum、KnockbackMultiplier、StickTimer、TearRange | 8／32B |
| Tear `+43C/+47C/+7EC/+7F4` | ContinueVelocity、StickDiff、PosDisplacement、ParentOffset，各 Vector | 4／32B |
| Tear `+468/+469/+7E8` | IsMultidimensionalTouched、IsPrismTouched、CanTriggerStreakEnd | 3／3B |

共 169B 原生范围；线上原来三个计数采用 u64 而原生为 i32，因此撤销固定状态 **181B**：Knife 28B、Bomb 54B、Tear 99B。按当前布局的最短合法负载（含继承与辅助 Sprite 的最小投影），Knife **450→422B**、Bomb **1181→1127B**、Tear **953→854B**；不包含成员／帧头或变长资源内容。这是布局算术，不是带宽实测。

原生范围、大小、类型及证据等级保持，尤其 ScaleAnimNum／TearRange 的 SDK 声明和三个计数的“推断”不升级。撤销 `wire`、专用读取资格、线上 validator、计时换算和公开成员的状态回映射，改为 `disposition=authority`。公开 getter 仍可用于 Host／E2E 观测，其存在不构成 Client 逐字节相等要求。

Bomb B0、Tear T0／T2 变为空预留块；其他已空块继续预留。布局号和后续块号不前移，也不要求发送零长负载。`entity_counters.*` 的通用函数仍供 Familiar／Pickup／Laser 使用，不因移除三个调用合同而删除。

## 2. 真正的展示输入继续保留

| 类型 | 保留内容及直接消费者 |
| --- | --- |
| Knife | TearFlags `+410`、RotationOffset `+43C`、KnifeDistance `+444`、KnifeVelocity `+448`、IsFlying `+464`。Render `93ECD0` 在 `93ED08—93ED61` 选择效果／飞行分支，`93EE0A/93EE2A/93EE36` 用偏移、当前距离及速度画拖影；`93F3E1` 再用飞行状态选择 DeadEye 的画法。这些值不能由“运动参数都归 Host”一起删去 |
| Bomb | costume 绘制开关 `+462` 和五份最终 ANM2（`+464` 起，stride `114`），以及公共主 Sprite。Render `6A0F90` 的 `6A10F2` 读取开关，随后借主图层的最终变换／颜色绘制 costume；不从爆炸计数计算闪烁或动画帧 |
| Tear | TearFlags `+428`、DeadEyeIntensity `+464`、三份独立辅助 ANM2，以及公共主 Sprite／位置／PositionOffset。Render `675AC0` 在 `675C14—675C4A` 按效果位绘制，在 `675DB8—675DDE` 按 DeadEye 强度绘制。Room Light `7FC110` 的 Type2 分支在 `7FD5EE` 读完整 flags，在 `7FD8BB` 读 DeadEye 强度；不能只留主 Sprite 就删除它们 |

公共 Parent／Spawner、KnifePlus hitboxSource、Tear StickTarget 等关系的当前合同不在本轮裁剪范围内。三份 Tear 与五份 Bomb 辅助 Sprite 的资源、独立动画／图层 selector、最终颜色和官方绘制插值输入同样保留。

**Bomb 资源重建是实际间接消费者，须按既有隔离合同处理。** `6A0F90` 在 `6A0FBB—6A0FC4` 检查 `+463` 并调用 `6A2420`；后者确实读取 Bomb Scale `+434`（`6A2590/6A3371/6A3381`）和 Flags `+438`（`6A26A4` 取址）选择、重建资源并改写最终 scale。不能把它们说成“全调用图没有消费者”。当前设计早已禁止 Client 执行这个懒重建 helper，改为安装 Host 的最终主／五份 costume 资源及姿态，然后清 `+463=false` 并读回。版本34据此排除两个中间输入；第13步不得运行 Flags／Scale setter 再置 pending，也不得以重新 LoadCostumes 恢复画面。新路径接管前保留旧执行器的 pending 清除行为。

## 3. 静态依据与边界

目标 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；以下均为 VA。只读 IDB，没有改名、改类型、保存或读取哈希。名称与类型先核对本地 `IsaacDocs/docs/EntityKnife.md`、`EntityBomb.md`、`EntityTear.md`，以及官方 REPENTOGON `libzhl/functions/EntityKnife.zhl`、`EntityBomb.zhl`、`EntityTear.zhl` 和相关 LuaInterfaces／Patches 源码。

从公共／各类 Render、Shadow、PostRender、Player 复合绘制、Room Light、公共／Knife GetRenderZ 和 WaterClip 共26个入口求直接可达集合，534个函数；Lua render回调桥按现有官方／无第三方玩法 Mod 边界停止展开。扫描执行段的目标位移内存访问、LEA 与 ADD／SUB 取址候选，涉及1004个函数；与展示集合交叉为27个函数。另看跨字段的宽访问、真实基址与具体类，不能把相同位移认作相同字段。上述 Bomb 懒重建路径明确保留在发现中，依实际 Client 隔离合同排除。

正面用途与最终结果的关系：

- **Knife 投掷／路径**：Update `93AAF0` 在 `93B809—93B865` 读 MaxDistance／Charge，用当前距离除上限再乘蓄力求轨迹；`93DE67` 读 PathFollowSpeed，`93DF95` 读 PathOffset 后求局部位移。`93B3D7/93B409` 读挥砍状态，`93B06D/93B158` 等读旋转攻击状态。两个分裂 helper `93A5F0/93A870` 消费并置多维／棱镜处理历史。最终位置、Sprite rotation／scale 与独立分裂成员由 Host 产生；Render 的 RotationOffset、Distance／Velocity 是另一组真实输入，继续发送。
- **Bomb 爆炸／下落／火箭**：Update `69DB30` 改写计数并控制触发，`69DDF1—69DE21` 将下落加速度积分到 FallSpeed，`69E230/69E2D6/69EAFA/69EB74` 等求火箭运动。`69FA17—69FA57` 将 RadiusMultiplier、ExplosionDamage 交 `6FBEA0` 爆炸裁决。Client接收最终爆炸效果、目标状态和姿态，不再倒计时触发爆炸或生成效果。`+45C` 的公开低字节与 Update 中宽计数访问并不等于全字段业务已闭合，原有邻接未知保持。
- **Tear 等待／粘附**：WaitFrames 的正值减一、父状态可能先设1再归零，以及 StickTimer 的无夹零有符号递减，继续保留[原始研究](tear-pickup-laser-evidence.md)；它们控制 Host 恢复运动和粘附到期效果，不参与已核绘制。ContinueVelocity `664417`、StickDiff `66EF05`、PosDisplacement `669FD4/66F989` 和 ParentOffset `66E278—66E2EF` 属于这些更新输入。未知活动条件不再是本类型同步锚点的前置依赖，也不改成“已核无条件倒计时”。
- **Tear 伤害／缩放**：Update `669FB9/66A0E6/66A14D` 读 BaseDamage；碰撞处理 helper `677940` 在 `679118—679136` 按 KnockbackMultiplier 缩放目标冲量，再于 `679277` 应用。ResetSpriteScale `674190` 读 Scale／ScaleAnimNum 并选择最终动画、图层和缩放，属于 Host 结果求值，Client不调用；BaseScale供后续更新求值。TearIndex用于轨迹／行为选择，Streak资格用于Host结算，均无额外线上身份／展示值职责。
- **Tear Height 不等于最终绘制偏移**：`672A60` 的高度setter先写 `+410`，调用 GetRenderHeight helper `672AC0`，再于 `672A80` 写公共 `+350 PositionOffset.y`，另有房间特判。helper还读 `+418` 等分支，不能由名字假定Renderer会再次调用它。已核Render用公共最终偏移；其直接调用者为Init／更新／生成／setter，未在展示可达集合。保留 `+350` 的独立最终值，既不由客户端Height重算，也不假设恒等公式。
- **原生半帧仍是模拟**：Knife slot4 `940D80` 会虚调用 Update；Tear slot4 `675E10` 会改位置／速度、关联Laser并重算 `+350`。Client不能为平滑开放它们，否则会恢复已排除输入的消费并重演玩法。网络插值如实施，只使用本地已应用快照历史。
- **排序／水面**：Knife GetRenderZ `93F570` 与公共 `6AC3F0` 已纳入；前者读 `+434` 等既有 UNKNOWN，后者读公共字段，不读本轮排除值。WaterClip `6B2F70` 的类／状态／公共偏移分支也不需要这些攻击输入。公共最终偏移、形态及水面合同保持。

交叉候选中，Effect Render、Laser几何、Player库存及Familiar Player指针有多个相同位移；逐对象区分后不构成Knife／Bomb／Tear这些字段的消费者。宽访问的Tear flags、DeadEye强度、Bomb costume ANM2也不能被误认作相邻类的小标志。

这是当前官方展示及 Client 隔离设计下的静态采用结论，不是任意虚分派、地址别名或第三方 Lua 回调的全程序证明。Knife Render还读取 `+440/+44C/+450` 等原有未知几何输入，Knife排序读取 `+434`；本轮不把它们改成权威／填充，也不以排除42个已知范围宣称类型展示完整。三类生成表的未知仍为497B／441B／478B。

## 4. 接线与唯一整体验收

第10步按版本34生成计划采集，不读取这42项，不为Bomb ExplosionCountdown或Tear WaitFrames／StickTimer建立锚点、时钟见证或Gap条件。第12／13步不安装、还原、校验或漂移比较它们。第14步相应退役旧Source／字段写入／原值相等要求；保留本地构造／析构、必要引用撤边和Bomb pending清除。运行中的旧Lua／JSON链本轮未修改，不能把布局修订表述为已省下运行带宽。

第15步沿用现有条目，不提前执行、不新增验收编号：

- E09／E25覆盖Knife普通投掷、挥砍／旋转、寻路、分裂及拖影／DeadEye；Bomb正负／重置计数对应的最终闪烁、火箭／下落和爆炸效果、不同范围／伤害／Flags／Scale对应的最终画面；Tear等待结束、粘附到期、下落／追踪／父跟随、不同高度／缩放／旋转／连击与分裂的最终成员、Sprite和位置。覆盖Tear flags／DeadEye的Room Light与水面／排序，不要求42个Host原值跨端相等。
- E13／E17只拒绝真实线上字段的非法值，不再构造已撤销字段负载来要求其拒绝。保留真实有限浮点、引用和资源／容器约束。
- E28／E29确认Client未执行Update／Interpolate、ResetSpriteScale、GetRenderHeight重算、LoadCostumes、爆炸或击退；Bomb pending始终为false，辅助Sprite借用／恢复与正常退休保持。
- E34撤销Bomb与Tear上述三个计数的锚点比较；Familiar／Pickup／Laser及其他真正保留的展示计时继续原合同。E27未知范围和历史失败不变。

本轮完成布局生成及整体静态审阅；没有构建或运行任何测试。运行时采集、安装和展示隔离仍须在后续步骤实施，第15步之前不能标为功能通过。
