# Pickup／Slot 选择、交互、价格与奖励参数同步复核

日期：2026-11-09（Asia/Singapore）。第 4 步，布局版本 **35**。本轮只读本地 Wiki／官方源码及 J273 IDB，修订计划、布局源并生成表格；没有构建、测试、游戏、注入、部署或 IDB 修改。原生事实与失败记录保留，运行时接线仍待第10／13／14步，第15步统一整体验收。

## 1. 逐项决定

**Pickup 的选择组、触碰、自动价格和商店位置标识不再传输；Slot 的八个交互／状态／奖励参数全部留在 Host。Pickup 最终 Price 与 DropDelay 有实际展示消费者，继续同步。**

| 类型／范围 | 字段 | 当前决定与理由 |
| --- | --- | --- |
| Pickup `+524/+528`，各4B | Charge、OptionsPickupIndex | Host充能结算／选项组选择及成员移除裁决；传最终Player／成员／Sprite，不传输入 |
| Pickup `+52C/+52E`，各1B | Touched、ForceBlind | Host拾取历史／隐藏图形选择；传最终动画、问号图层与实际Image，不在Client重载图形 |
| Pickup `+538/+53C`，1B／4B | AutoUpdatePrice、ShopItemId | Host自动改价、补货与重抽输入；保留最终Price和价格Sprite，不在Client重新定价 |
| Pickup `+540/+544`，各4B | Timeout、Wait | Host消失／不可拾取计时；最终Visible、动画、成员退出由Host给出，撤销两个u64同步锚点 |
| Pickup `+584/+59C`，各4B | State、VarData | Host状态机／变体碰撞业务参数；Client不推进状态机或碰撞，安装最终形态与Sprite |
| Pickup `+5A0—5BF/+5C0`，32B／4B | CollectibleCycle、CycleCount | Host未来循环道具列表及有效数；循环发生后传当前成员形态与资源，不在Client轮换／Morph |
| Pickup `+534`，4B | **Price** | **保留**：公共WaterClip实际读取；当前价格与价格Sprite是不同状态，不能只留图形 |
| Pickup `+554`，4B | **DropDelay** | **保留**：Pickup Render按其是否为正决定整段是否绘制，不能只靠Visible代替 |
| Slot `+410/+414`，各4B | State、PrizeType | Host机器状态机与派奖类别；不在Client开奖／生成，传最终主／奖品Sprite和独立成员 |
| Slot `+41A/+41C`，各2B | ShellGameAnimationIndex、Timeout | Host选动画／开奖计数；真实i16及Wiki高位冲突保留，不传原值或建立锚点 |
| Slot `+420/+424`，各4B | DonationValue、TriggerTimerNum | Host支付累计／奖励资格及触发、重抽计数；不等于展示文本或统一倒计时 |
| Slot `+42A/+540`，2B／4B | Touch、PrizeCollectible | Host触碰交互／未来奖品ID；Client不交互或调用SetPrizeCollectible补图，直接装最终资源 |

Pickup共12个原生范围、Slot共8个范围改归权威；其中CycleCount原来已由容器count派生，因此实际撤销 **19项原线上字段／容器**，不能再把它当作独立4B重复计算。两类各一只，原最小状态Pickup **744→703B**、Slot **564→538B**，合计减少 **67B**；另省去循环列表的 `4×N` 字节（原N为0—8），不含成员／帧头和变长Sprite资源。这是布局算术，不是运行带宽实测。

撤销相应 `wire`、读取资格、validator、公开成员的状态回映射及两个计时换算；回映射改为 `disposition=authority`。CycleCount由“已发送容器的派生值”改归Host，不再由Client恢复Host值。Pickup P3、Slot S0／S1成为显式预留空块，编号不前移，不要求接收零长负载。原生类型、宽度、证据等级、448B／363B未知和所有原生范围保持；不以传输裁剪宣称两类完整。

本轮不裁剪Pickup公开Ghost／MegaChest三条引用、两份辅助Sprite或公共Entity输入；关系仍由真实引用owner维护。Slot独立奖品Sprite全部外观、动画与图层投影保留。整局HUD／进度自己的捐款总额、价格或统计属于各自域，不能因为排除Slot.DonationValue就一起删除。

## 2. 保留项的实际展示依赖

地址均为 J273 VA，ImageBase `0x400000`，目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`。本地语义来源：`IsaacDocs/docs/EntityPickup.md`、`REPENTOGON-1.1.3/docs/docs/EntitySlot.md`，以及官方 `libzhl/functions/EntityPickup.zhl`／`EntitySlot.zhl`、`LuaEntityPickup.cpp`／`LuaEntitySlot.cpp`。

- **DropDelay不能一起删除。** Pickup Render `6E35D0` 在 `6E35F8 cmp [esi+554],0`、`6E35FF jg` 对正值直接退出；它与公共exists／visible是三个独立条件。正值期间即使Visible=true也不画，单传最终Sprite不能等价替代。现有条件倒计时编码继续保留；`6E0650` 的 `6E0685—6E06A4` 在碰撞类别等条件下递减／早退或直接清零，仍不得猜成无条件全局帧倒数。实际时钟见证未完成时仍报Gap，不以Frozen掩盖。
- **Price不能随AutoUpdatePrice／ShopItemId删除。** WaterClip `6B2F70` 在 `6B3069`、`6B328E` 读取Pickup `+534`，参与商店物品的水面／反射分层。该查询已经是Client展示合同的一部分；保留完整当前i32价格，不假定可从price ANM2的数字帧反推，也不由ShopItemId重新算。
- **价格图形独立安装。** Pickup Render `6E39AF` 检查 `+519` 后在 `6E39C3—6E39C9` 绘制 `+410 price ANM2`。`+519` 是该ANM2的本地loaded，不是AutoUpdatePrice；沿用版本32的可空资源选择与本地owner，不恢复Host loaded原字节传输。价格变化的最终Sprite、图层数字和播放姿态仍须与Price来自同一窗口／目标版本。
- **替代底座与MegaChest不是上述输入的派生替代。** Variant100还使用 `+5D0` 的独立ANM2、绘制RNG与MegaChest关系，按既有合同保留。Client不执行TryFlip、InitFlipState或重抽来恢复最终画面。
- **Slot必须保留奖品Sprite的动画选择及播放标志。** Render `6620E0` 先在 `6620E9` 调公共Render；仅variant6／15在 `6620FB` 检查 `+460`，`66210A` 检查 `+470`，随后以 `+42C prize ANM2` 绘制。按当前ANM2／AnimationState布局，前者是 `prize.main_state.active_animation`（`42C+30+4`），后者是 `prize.main_state.is_playing`（`42C+30+14`）。它们不是Slot.State、PrizeType或ShellGameAnimationIndex，也不是loaded／通用visible。这两项真实资源／播放状态已在ANM2投影中，继续保留。早期记录把`+460`称loaded、`+470`称visible的描述保留历史语境，以本轮精确回映射为准。

## 3. 排除项的正面用途与调用边界

从公共／各类Render、Shadow、PostRender、Player复合绘制、Room Light、排序及WaterClip共26个入口追直接游戏调用，显式在包括Pickup `849730` 的Lua render桥停止展开，得到532个函数。执行段中目标位移的内存访问、LEA及ADD／SUB取址候选涉及799个函数，与展示集合交叉为18个函数；另核对宽访问覆盖和实际对象基址。相同位移命中不是同类消费者证明：Player helper `786760` 的 `+5A0` 是Player的Sprite播放条件，不是Pickup循环列表；Tear的 `+5C0` 是辅助ANM2；Tear／Knife flags宽读取也不能误认作Slot Touch／Shell动画索引。

以下是实际Host用途，不从“主Render无命中”单独推结论：

| 路径 | 已核用途与Client边界 |
| --- | --- |
| Pickup Update `6E0650` | `6E0E3D—6E0E43` 传播选择组，`6E2626` 按选择组走生成／重置分支；`6E1920` 检查Touched；`6E1CDB/6E1CEF` 写充能结果。它们服务Host交互、生成与收集，Client接收最终成员和Player状态 |
| Pickup自动价格 `6E1B2B—6E1BAB` | 先检查AutoUpdatePrice和当前Price，再将ShopItemId、variant／subtype交 `80A790/80B5A0` 求新价。最终Price及价格Sprite单独采集；不在Client读取商店表或重跑自动价格 |
| Pickup强制盲目 `6DF170→6DF520` | setter写 `+52E`，变化时触发ReloadGraphics；后者在 `6DF65D` 选择blind，调用 `6DF1A0` 设置最终问号／道具图形并LoadGraphics。最终路径、实际Image及图层都已有状态，不在Client执行setter／重载；官方Lua IsBlind查询仍可用于Host／E2E观测，不要求Client保留Host的资源选择输入 |
| Pickup Timeout／Wait | Update中 `6E269A—6E26C1` 处理到期移除／正值等待；还存在DropDelay早退及变体重置。Timeout产生的闪烁／消失接收最终Visible／Sprite／成员结果；Wait只限制玩法拾取。两个旧锚点的活动条件未知不再是它们的同步依赖；原始计数事实保留 |
| Pickup State／VarData | Update `6E31FA/6E3263/6E3331` 等推进State；HandleCollision `6E5690` 在 `6E7F74/6E80DC/6E9528` 等读写VarData。Client不执行这些状态机／碰撞，接收最终姿态与生成结果 |
| Pickup循环 `6DEC30/6EA710` | `6DEC30` 按上限8抽取未来列表，含RNG；`6EA710` 在计时／输入分支旋转列表并在 `6EA7E6` 调Morph `6DFF30`，切换当前道具／最终图形。列表、count及Flip保存往返是Host未来状态；Render不需要它们。Client不调用TryInitOptionCycle／Morph／循环helper，也不运行原生Update |
| Slot Update `65A210` | `65A25C/65A33C/65A377` 等读写State；`65AA3E/65AA65/65AA82` 使用PrizeType；`65DA2D—65DA40` 用有符号ShellGameAnimationIndex选`Shell1Prize`等最终动画；`65A94D—65A961` 的Timeout确为16位；这些不是绘制时再次选择的必需参数 |
| Slot支付／触发 | `65AF75/65AF7E` 等累计并比较DonationValue；`65A7CE/65A7DB/65A83B` 等处理TriggerTimer；`65E6CF/65E6DD/65EB8E` 使用16位Touch。最终支付、奖励、动画与目标状态在Host产生；不传进度owner，不在Client支付、开奖或写持久数据 |
| Slot奖品ID `6626F0` | 先在 `6626FF` 写PrizeCollectible，variant15经 `66274D` 构建奖品Sprite，variant16经 `6627AB` 构建主Sprite，再LoadGraphics。它确实是资源生产输入，但该setter不在Client放行；两种最终Sprite已分别同步，不能把ID和资源生产过程当作必传展示状态 |

官方 `CustomCallbacks.cpp` 的Pickup／Slot Render wrapper按variant传对象和offset、调super及回调，没有内建消费上述Host字段。任意第三方Lua绘制／玩法回调读取任意私有值不在本项目当前支持声明内，不据此恢复全对象复制。这个结论是当前官方展示和Client隔离合同下的静态采用，不是全部虚分派／任意地址别名的全程序形式化证明。

## 4. 接线与验收修订

第10步按版本35计划采集，仅保留Pickup Price／DropDelay及资源、关系；不遍历Host循环数组，不为Timeout／Wait建立锚点或时钟见证。第12／13步不解码、安装、漂移比较已排除项；完整保留真正上线的DropDelay换算及资源／引用约束。Slot不恢复八个原值，仅安装最终主／奖品Sprite。

Client本地构造／复用owner仍须使Pickup固定循环数组和count处于安全空状态，不能因不传Host列表而留下可被本地路径消费的池残留；这属于本地初始化，不是同步Host数据。不得为补画面运行循环、自动定价、ReloadGraphics、SetPrizeCollectible或完整派生Init／Update。第14步退役对应旧Source／字段与容器写入／原值比较，保留正常资源退休、引用撤边和既有其他类的owner保护。运行中的旧Lua／JSON路径本轮未修改。

第15步沿用既有条目，不另开提前测试：

- E09／E25覆盖选项组拾取后独立成员移除、触碰／充能／自动价格／商店重抽补货后的最终画面，盲目↔可见图层、循环切换后的当前形态／Sprite、不同Price的价格图形与水面／反射，以及DropDelay正值隐藏／归零显示。Slot覆盖1—18变体的支付／捐款／触发／开奖后最终主Sprite及6／15奖品Sprite；不比较Host输入原值或未来循环列表。
- E13／E17只检查真实线上字段，不再构造被撤销参数／循环数组的非法负载；预留空块不作为构造必收负载。DropDelay编码、资源存在性、选择器／图层边界与真实引用检查保持。
- E28／E29确认Client未运行购买、捐款、重抽、派奖、循环Morph、定价或资源生产setter，无持久副作用；资源借用恢复、循环本地空状态、引用撤边与正常退休保持。奖品当前动画／is_playing和价格Sprite的本地loaded含义不能互换。
- E34撤销Pickup Timeout／Wait锚点比较；DropDelay及Familiar／Laser等真正保留计时继续各自合同。Slot八个标量不比较，也不改成通用倒计时。E27未知区间和历史失败不变。

本轮完成源与生成表修订、计划联动及整体静态审阅。尚未实施或执行的采集／应用／展示场景不记为通过；没有构建或测试。
