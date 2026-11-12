# 6：GridEntity

**2026-11-12 显示委托修订**：

- Grid 重标：保留 type／variant／State、Sprite 帧，以及门、便便、石头等可见状态。
- 水面裁剪等本地显示项移出同步，`client_grid_water_*` hook 默认删除。
- 见[修改方案](client-display-delegation.md)第 5、9 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-10 第6步剩余描述编码完成**：[实现与边界](grid-description-implementation.md)。`ClientGridDescription`现在由完整已校验的影子记录生成，按同源数字索引提供公共／Door值、真实Sprite投影及Fire／Rock容器访问，不再拥有字符串字段字典。旧JSON链显式使用`LegacyClientGrid*`，等待第13／14步换接／退役；第10／12／13步的输入生产、网络解码及原生运行接线均未提前实施。线布局未改，UNKNOWN保持，按规范不运行测试。

本轮新增`isaac_cs_grid_description`库及完整Release／Win32 `zhlIsaacCS.dll`纯编译／链接通过，E2E工具关闭。并行产品构建停滞后中止，改串行MSBuild完成，具体记录见上方实现说明；没有测试、部署或游戏操作。

**2026-11-10 版本55：剩余119行公开成员逐行回映射完成**。见[映射说明与证据](grid-member-map.md)及[同源生成的119行汇总](layouts/06-grid/grid-catalog-members.md)。正式源保留三个重复ID的各自原行／签名：16行字段、28行派生、74行Host权威、1行不适用；不适用项为Repentance已移除的ReducedSpawnRate。补清GetBigRockFrame按u32比较Variant的原始语义，未改变线字段／各根UNKNOWN。第6步公共及17派生归属、Plus来源ABI与成员目录静态工作均已登记；实际运行接线仍归后续步骤。普通Release／Win32状态库生成与纯编译成功，无测试，第15步统一验收。以下版本记录保留各自原时点。

**2026-11-10 版本54：GridEntityPlus来源／ABI／生命周期静态核定完成**。见[旁表证据与同步决定](grid-plus-water-source-evidence.md)及[229号ABI表](layouts/06-grid/GridEntityPlus.md)：官方DLL内448×12B共享表，payload+4／engaged+8，Factory返回后与Init进入前清同索引项，没有Free／换房／C/S停用清理Hook。G0只发最终完整u32，不传optional／填充／地址／重置历史。纠正：官方六Hook不含Lock，Lock忽略旁表；现有六个C/S wrapper也漏Lock，第13步必须补第七覆盖点。各Grid根大小／UNKNOWN不变，全119项逐行回映射仍待完成；实际reader／writer归第10／13步。以下版本条目保留原时点，本轮无测试。

**2026-11-10 版本53：PressurePlate／Door字节归属完成**。[静态证据与职责](grid-pressure-door-evidence.md)追加224—228：两类描述不发无消费者的临时Light种子，PressurePlate主Sprite用202、TimerPlate用201；Door主Sprite用226（颜色仅alpha入口布尔）、extra用201，target类型／i32描述索引／ExtraVisible与Light bit0x20保留。previous缓存／四字符串／NextGreedAnimation／GreedModeRNG等留Host，纠正旧Door+154 coin-mode误归（实际dword计数）。Door2EC最后4B是string capacity，27F仍未知；根UNKNOWN31／32B，最小212／223B，生成及Release状态库纯编译通过、无测试。全部17派生现有归属表；全目录／Plus来源／正式接线仍待完成。

**2026-11-10 版本52：本次指定七类字节归属完成**。[静态证据与同步职责](grid-seven-subclasses-evidence.md)及`state/grid-classes.json`新增214—223：Lock／Pit／Spikes／Statue／TNT／Poop／Rock及三个专用视图。TNT+154等控制计数、Poop40B EntityRef和Statue尾部裸指针留Host；Statue自身无Sprite负载。Rock保留可绘碎块资源／有序偏移，state2主动画和layer0位置的旧残留不读不发，overlay及与地板色相乘的原色保留。根UNKNOWN依次20／20／17／17／17／19／17B，生成及Release状态库纯编译通过，无测试。当前公共部分与15个派生已登记；PressurePlate／Door、全目录及正式运行接线仍待完成。以下版本条目保留原时点。

**2026-11-10 版本51：本次指定八类字节归属完成**。[静态证据与职责](grid-eight-subclasses-evidence.md)及`state/grid-classes.json`新增202—213：Decoration／Gravity／Stairs／Teleporter／Wall／Web／TrapDoor／Fire。五类主Sprite的地板色由同world房间派生，省去44B；Wall非16房间及Teleporter保留颜色。TrapDoor+154计数、Fire计时／样式及ember速度留Host；Fire两附属Sprite／有序粒子保留，第三Sprite旧主帧／选择器由粒子派生，is_playing仍同步。七类根UNKNOWN17B、Fire45B，最小98／142／372B。生成及Release状态库纯编译通过，无测试。其余九类／全119项／实际接线未完成；以下条目保留原时点。

**2026-11-10版本50复核**：[同步必要性与纠错](grid-common-synchronization-review.md)取代上一轮的CollisionClass直接传输及完整通用Sprite复用。Desc198四项16B不变；Grid199的G0为desc16＋index4＋最终WaterClipFlags4，原生+3C留Host、Client本地中性初始化。新增GridLayerState200／GridSprite201，移出默认动画名／待加载路径，保留实际Image等并补Dataminer故障纹理最终选择1B；G1固定94B，公共最小状态142B，根UNKNOWN仍17B。子类、来源owner／绘制与运行接线仍待完成，本轮无测试。 冠军色只保留前四float16B，其余28B不上线，普通Sprite／Layer ColorMod完整11值不变。以下版本48说明保留原时点。

状态：**2026-11-10 公共部分字节归属与正式生成源已落地（版本48）**，见[公共字节证据](grid-common-evidence.md)、[Grid公共生成表](layouts/06-grid/GridEntity.md)及[Desc生成表](layouts/06-grid/GridEntityDesc.md)。340B公共前缀递归UNKNOWN仍17B，不能称完整类型；17个子类、Plus旁表与全目录回映射尚未完成，实际采集／安装仍待第10／13步。没有运行测试，第15步唯一统一验收。上级：[总计划](README.md)。

## 目标

完成 `GridEntity` 公共部分（0x000—0x153）和各 Grid 子类的字节归属，确定 Grid 在房间中的枚举方式、记录布局和状态块，覆盖门、TimerPlate、Fire 等现有专用内容。

## 输入事实

### SDK 声明

`GridEntity.zhl` 声明对象大小 0x154，具名字段只有 6 个：

| 位置 | 字段 |
| --- | --- |
| 0x04 | `_desc`，GridEntityDesc，0x20 字节 |
| 0x24 | `_gridIndex` |
| 0x28 | `_spawnFrame` |
| 0x2C | `_rng` |
| 0x3C | `_collisionClass` |
| 0x40 | `_sprite`，ANM2，到 0x153 |

`GridEntityDesc`（0x20 字节）有 8 个字段：

| 位置 | 字段 |
| --- | --- |
| 0x00 | type |
| 0x04 | variant |
| 0x08 | state |
| 0x0C | spawnCount |
| 0x10 | spawnSeed |
| 0x14 | varData |
| 0x18 | initialized（bool） |
| 0x1C | variableSeed |

### `world_layout.json` 中的已核字段

Grid 有 15 个字段已带 J273 证据：desc 八项、grid_index、spawn_frame、rng 四个字、collision_class。position 是换算项，由格子序号推出：步长 40，原点 (40,120)，getter RVA 0x4087A0。

### 子类大小（SDK 声明与 J273 工厂分配）

J273 的 `GridFactory` 按类型分配对象，分配请求见 [Grid 种类静态确认](../../research/tx-04-08-grid-kinds-static-confirmation.md)第 40—44 行，已登记在 `world_layout.json` 各 Grid 类的 `factory_allocation_size`：

| SDK 声明 | J273 工厂分配 | 子类 |
| --- | --- | --- |
| 0x154 | 0x154 | Decoration、Gravity、Stairs、Teleporter、Wall、Web |
| 0x154 | **0x158** | TrapDoor |
| 0x154 | **0x390** | Fire |
| 0x158 | 0x158 | Lock、Pit、Spikes、Statue、TNT（TNT 有 `_frameCount` 0x154） |
| 0x174 | **0x19C** | Poop |
| 0x198 | 0x198 | Rock |
| 0x27C | 0x27C | PressurePlate |
| 0x2EC | **0x2F0** | Door：`_extraSprite` ANM2 0x168、`_targetRoomIdx` 0x280、`_previousState` 0x284、`_previousVariant` 0x288 |

- **四类不一致**：TrapDoor、Fire、Poop、Door 的分配大于 SDK 声明。Fire 多出 0x23C 字节，Poop 多出 0x28 字节，都不是对齐能解释的差距。
- **分配不等于 sizeof**：分配请求只是对象大小的上界。研究已明确“不把分配长度当作 sizeof”（同文第 43—44 行），新类在 `world_layout.json` 中的 `object_size` 仍为 null。本步按工厂分配划定对象范围；SDK 声明之外、分配之内的字节先列为未知，用 J273 构造函数核出写入范围后再归类。
- **RTTI**：`world_layout.json` 记录了 18 个 Grid 类的 RTTI，以及 `grid_types` 中 type 到 RTTI 的映射。

### 房间中的位置

- **格子槽**：`Room+0x24`，共 448 个指针，最大序号 447。
- **门槽**：`Room+0x724`，共 8 个。
- **门所在格子**：`Room+0x744`，8 个格子序号。
- **构造时清零**：Room 构造函数（RVA 0x3DF8F0）清零 `Room+0x24` 起的 0x720 字节。

### 公开成员与现有 Client 描述

- **公开成员**：`room-grid-application.json` 中，`isaac.cs.grid` 有 119 个成员，其中 GridEntity 45 个、Door 32 个、Rock 17 个、Desc 8 个，其余子类各 1—6 个。
- **Client 描述**：`ClientGridDescription`（`mod/src/game/client_grid_native.h:23-42`）包含：
  - desc 八项、collision、water_clip、spawn_frame；
  - 两个位置；
  - 用字符串命名的 `fields`；
  - 角色化 Sprite：主、Door extra、TimerPlate、Fire 两个专用；
  - BigRock 帧、Fire 计时与样式、Door 动画模式与灯光标志；
  - ember 与 rubble 两个容器。
- **门绑定**：`ClientGridDoorBinding` 按 8 个门槽分别保存候选、允许位、实际主体和路由。

## 设计

### 6.1 字节归属要点

- **GridEntityDesc**：公共198按版本48保留type／variant／state／spawn_seed四项16B；版本53中PressurePlate／Door用224，只发前三项12B，因为两个Light case不消费公共prologue构造的临时RNG。其它Grid不据此删Seed，特别保留TrapDoor实际随机显示。SpawnCount／VarData／Initialized／VariableSeed仍留Host，3B UNKNOWN不复制；Client资格由本地owner建立。
- **RNG**：`_rng`四字留Host。Light `0x801BE0`在栈上用SpawnSeed＋Host GameFrame生成临时绘制RNG，故SpawnSeed保留状态；不能混同两者。VariableSeed不是Light使用的种子。
- **公共其它字段（版本50）**：grid_index保留；collision_class原生值留Host，只在Host需要时参与最终water_clip_flags合成。C/S六个wrapper按实例安装最终值，Client本地+3C先初始化为0，不复制Host碰撞。spawn_frame仍留Host。公共前缀正式源为`mod/binding/state/grid.json`。
- **主 Sprite**：按各类实际nested_layout选201／202及Rock／Statue／Door专用视图，资源／姿态／插值／图层与Dataminer最终位按各自消费者保留。版本53的Door主色只传1B alpha入口，整色取同world墙色；PressurePlate主色取地板色。TimerPlate／Door extra均用201，保留它们自己的完整颜色，不能套主色裁剪。
- **门目标**：`_targetRoomIdx` 是有符号房间描述序号，保留负特殊值，Light通过它读目标descriptor.Flags；与target_room_type一起归显示状态，不是指针，也不授权Client过门。`_previousState/_previousVariant`则是Host动画生产历史缓存，不下发。
- **子类区**：版本51八类、版本52七类、版本53PressurePlate／Door，17个派生均已生成完整范围归属表，具体UNKNOWN保留；版本55另已完成119行全目录回映射，实际运行来源仍归第10步。
- **SDK 声明之外的字节**：Door 0x2EC—0x2EF为最后一个string的capacity，实际范围2F0h；27F为独立UNKNOWN1B。Poop 0x174—0x19B为Host伤害EntityRef；TrapDoor尾4B为Host更新计数；Fire尾572B为两ANM2／两标量／vector头，递归未知保持。

### 6.2 枚举与记录

- **记录归属**：每个非空格子槽产生一条记录，记录序号即成员序号。
- **身份**：沿用现有 Grid 身份，即房间身份、格子序号和类别。
- **门**：门和装饰可能出现在同一格子序号上，按现有 door_aliases 规则区分。门槽映射作为房间块的一部分发送（第 7 步），不放进 Grid 记录。
- **形态变化**：type 或 RTTI 变化即为形态变化，走结构路径（第 11 步）。
- **被举起的格子**：Effect136不占格子槽，也不是活动Grid。版本38按[Effect复核](effect-synchronization-review.md)将其+0x420的32B描述（八个有名成员29B）留Host；Effect FX2与Grid域均不发描述，旧`grid/lifted_descriptors`同步在第14步退役。Client接收真实最终Effect主Sprite、State及Player持物／BodySprite关系与姿态，不重演举起、投掷、落地或还原格子；落地后真实Grid／Entity由Host正常发布。

### 6.3 状态块（公共按版本50，各类按版本51—53）

| 块 | 内容 |
| --- | --- |
| G0 描述 | 通用desc四项16B＋index4B＋water_clip_flags4B=24B；版本53 PressurePlate／Door用224描述12B，G0=20B；+3C原值仍不发 |
| G1 外观 | GridSprite201固定94B；无默认名字符串，冠军参数仅16B；保留资源／外观及1B glitch最终选择；图层200不发路径元数据 |
| G2 动画 | 主 Sprite 动画状态 |
| G3 图层 | 主 Sprite 图层变长块 |
| Fire 块4—10（版本51已生成） | 第二Sprite外观／动画／图层，第三ember模板外观／动画／图层，有序可绘ember列表；计时／样式／速度不上线，第三主帧／选择器派生，真实age/maxLife及插值开关保留 |
| Rock块4（版本52已生成） | 当前可绘RubbleAnim资源u16＋有序Vector列表；主Sprite用215，state2不传旧主姿态，layer0位置用214条件派生；原色／overlay／其它层姿态保留 |
| Lock／Pit／Spikes／TNT／Poop（版本52） | 没有独立尾部负载，Host控制字段和未知不读不发；Lock主Sprite用201，其余用202 |
| Statue（版本52） | 只继承G0，G1—G3经216视图保持零负载；尾部裸指针留Host，真实外观走独立Entity成员 |
| PressurePlate块4—6（版本53） | TimerPlate外观／动画／图层最小118B；主Sprite202，根最小212B。计时文字由Room／Game同world上下文承接 |
| Door块4—7（版本53） | 块4为target type4＋ExtraVisible1＋target index4＋Light位1共10B，extra三块最小118B；主Sprite226，根最小223B；四动画名／previous不上线 |

## 改动范围

- 布局源 `state/grid.json`、`state/grid-classes.json`。
- `ClientGridDescription` 已改由完整影子记录生成，取代字符串字段，见[实现](grid-description-implementation.md)；旧JSON执行链明确使用`LegacyClientGridDescription`，正式输入及原生消费的运行接线留第12／13步。
- 原生写入函数 `ClientNativeGridsExecute`、`ClientNativeGridsBindDoors` 保留。

## 失败方式

- **按 SDK 大小划范围**：按 SDK 声明处理 Door、TrapDoor、Fire、Poop，漏掉 J273 实际分配的尾部字节；或者反过来，把分配长度直接当作 sizeof，把尾部未写字节当成状态。
- **同格冲突**：门和装饰在同一格子序号上，记录身份冲突。
- **重复构造**：形态变化没有走结构事务，或复用旧实例时重复调用构造函数。
- **RNG 误判**：把对象玩法_rng与Light临时RNG混为一谈，恢复同步对象游标，或错误删除Light实际需要的SpawnSeed／同world Host帧。
- **来源元数据与本地资格混同**：恢复八项Desc／spawn_frame全发或用Host Initialized替代Client构造／资源资格；把Desc3B间隙和ANM2的14B未知吞进memcpy。
- **旁表缺口掩盖**：把GridEntityPlus的12B外部slot伪装成公共区字节，读取未设置payload或把读失败当Null；按index共享最终缓存、在Free／换房时假设官方自动清表，或给Lock错误应用override。来源ABI已核，正式reader仍未接入。
- **中间输入与最终输出混同**：因Lock读取collision而恢复Host碰撞下行，或以本地+3C=0代替其最终结果；应按实例发最终u32，Client先中性初始化并在第13步补Lock第七覆盖点，七族原链各一次。
- **纹理故障跨进程差异**：Dataminer按本地ANM2地址取随机方向；须传最终两个选择位并由绘制adapter消费，不能发地址或让Client再取一次本地随机。
- **槽位越界**：格子槽读取越过 447，或者把 `_doors[8]` 当成独立的成员来源，造成重复。
- **恢复Host举持描述同步**：版本38已排除的136描述被旧lifted_descriptors或FX2重新传入，甚至触发Client还原active Grid；应只用真实最终成员及持物展示状态。

## 第6步完成内容与后续依赖

- **字节归属与表格**：公共部分及17个派生（共18个RTTI）均已建立完整范围归属及两张单行生成表。UNKNOWN照实保留，不能称全部语义已知；公开目录及旁表来源现也已登记，运行采集／安装归后续步骤。
- **旁表来源**：版本54已核官方共享数组、12B ABI、读取owner、生命周期及六族／Lock例外；229只有本地字节表，无发送块。实际绑定／读取与第七Lock覆盖点仍分别归第10／13步。
- **回映射**：版本55将119行全部写入正式`catalog_rows`，按原目录行保留属性／方法重名、null／空白参数、返回值、真实布局／字段和逐行依据；同源生成`state_members.inc`及[汇总表](layouts/06-grid/grid-catalog-members.md)。16行字段／28行派生／74行权威／1行不适用，不承诺任意公开API兼容或恢复旧Host字段相等要求。
- **描述编码**：`ClientDescribeGrid`及只读类型化描述已实现；生成计划覆盖17个真实类和27个工厂type。完整影子记录的数字投影、可选Seed、最终u32水面值、Door布尔／i32、Fire16B粒子与Rock有序偏移均不依赖旧字符串字段。原生执行器／门绑定保留，旧调用链仅明确更名为legacy；第10／12／13步运行接线不算本次已经完成。

版本48公共部分的实现和完整DLL编译记录见[原证据](grid-common-evidence.md)。版本50公共最小142B、根17B未知，版本51八类及版本52七类的原结果保持，见[八类](grid-eight-subclasses-evidence.md)／[七类](grid-seven-subclasses-evidence.md)。版本53追加224—228，PressurePlate／Door最小212／223B，根31／32B未知，见[证据与纯编译记录](grid-pressure-door-evidence.md)。全部17派生已建表，版本54已完成水面旁表来源与ABI核定，版本55已完成全119行回映射；实际运行绑定／最终水面owner／Lock覆盖点／glitch绘制adapter／Fire与Rock容器／Door目标解析及draw恢复仍未接通，按第7／8／10／13步负责，不因静态目录完成称运行验收通过。

版本55执行普通构建`cmake --build build/repentogon --config Release --target isaac_cs_state --parallel 4`，正式生成四份代码表、各类单行表与119行汇总，成功编译Release／Win32 `isaac_cs_state.lib`。没有完整DLL构建、测试、游戏启动、部署或注入；没有新增单元测试、运行校验脚本或保留额外artifact。

## 未决问题

- Door2F0h范围与最后string capacity已核定，27F仍UNKNOWN1B；PressurePlate／Door两份ANM2各保留14B未知，根分别31／32B。Poop尾部伤害EntityRef留Host、172..173仍未知2B；Lock／Pit各未知3B。其它既有Desc／ANM2／Fire未知全部保留，不能称语义全已知。
- Rock原生Render会推进overlay、force换动画还会释放selector，不能直接当Client纯绘制入口；多pass的实际Host最终姿态来源、纯绘制adapter与精确恢复仍归第8／10／13步，不能以Client帧累计模拟或浅备份指针掩盖缺源。
- Desc3B间隙与ANM2递归14B未知的实际语义；不能在第15步之前通过实验消除。
- GridEntityPlus合法来源、ABI与生命周期已经[版本54核定](grid-plus-water-source-evidence.md)；实际模块绑定／Host reader、第七Lock覆盖点及最终water_clip_flags的Client per-instance安装仍待第10／13步，以及Room／Run的颜色、计时、Light与Dataminer pass上下文；不为Client复制整个旁表optional。故障纹理source-quad adapter仍需实际实现。对象_rng留Host；后续新展示消费者须带证据修订。
