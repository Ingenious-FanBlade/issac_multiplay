# Familiar 参数、Incubus 充能与 Gello 绳索同步复核

> 版本39续核：[Player装备与Plus](player-equipment-plus-synchronization-review.md)补Player type15 UmbilicalWhip的真实主引用与Familiar+D76抑制字节；当前基础Familiar614B、Incubus618B，根未知905B。下文版本33的613／617B等保留原时点；Incubus最终amount方案不变，不恢复Familiar玩法Weapon或已排除Plus项。未构建或测试。

日期：2026-11-09（Asia/Singapore）。第 4 步，布局版本 **33**。本轮是静态研究与计划／布局源修订；没有构建、测试、注入或游戏操作。已核事实不等于运行时实现或验收通过，历史记录与 UNKNOWN 保留。

## 1. 逐项决定

| 原生范围 | 字段／用途 | 当前同步决定 |
| --- | --- | --- |
| `+D60`，4B | OrbitAngleOffset | Host 轨道位置求解输入，不传 |
| `+D64—D6B`，8B | OrbitDistance | Host 椭圆轨道半径，不传 |
| `+D6C`，4B | OrbitSpeed | Host 轨道相位推进输入，不传 |
| `+D70`，4B | OrbitLayer | Host 编队／轨道层，不传，不在 Client AddToOrbit |
| `+D80/+D84/+D8C`，各4B | Hearts／Keys／RoomClearCount | Host 持有／奖励及清房计数，不传；展示对象和最终动画单独同步 |
| `+D50`，4B | HeadFrameDelay | Host 射击／动画选择的更新计数，不传；其非统一线性时钟的事实保留，不需要再核同步锚点 |
| `+D54/+D58/+D5C`，各4B | MoveDirection／ShootDirection／LastDirection | Host 移动、瞄准和动画选择输入／历史，不传；最终姿态与独立 selector 保留 |
| `+D4C/+D88/+D7C/+D74` | FireCooldown／State／Coins／IsFollower | **保留**已有真实充能、附属 Sprite 缩放／淡出和 Player 鞭形态绘制依赖，不随上述字段裁剪 |
| variant80 `+DB0` | Incubus 的玩法 Weapon owner | 指针／对象不传；新增 **最终充能条整数百分比**，i32，0—100，仅布局131 |
| variant240 `+ECC` | UmbilicalBabyData（Gello） | **必须补同步**其有序绳索节点 XYZ 和绘制 Sprite；布局130新增四个 payload 投影 |
| variant240 `+DAC` | 绳索全段／后半段选择 | 保留版本29的真实成员引用，依旧通过 SetReference owner 安装；不改成伪非空地址 |

上述 11 个排除范围共 **48B**。基础 Familiar 最小状态由版本32的 661B 降为 **613B**；Incubus 增加4B结果，最小617B；240保留2B关系并增加四个可选payload投影的8B计数，最小623B。非空绳索另含实际 Sprite 和节点数据；这是布局算术，不是实测带宽。

MoveDelayNum、IsDelayed、IsLilDelirium 等不在本轮逐项裁剪范围内，不因相邻字段改归权威而顺带删除。FireCooldown 的保留变体与活动时钟问题仍有未决，不能把未知编码为 Frozen。

## 2. 参数排除的静态依据及边界

目标 IDB 为 J273 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；下列均为 VA。没有修改／保存 IDB，没有计算或校验哈希。

- 本地 `IsaacDocs/docs/EntityFamiliar.md` 与[公开成员文档](https://wofsauge.github.io/IsaacDocs/rep/EntityFamiliar.html)用于核对名称、类型和轨道含义；字段公开或旧 Client 曾写入并不构成传输理由。
- 从公共／各类 Render、Shadow、PostRender、Player 复合绘制、Room Light 和 GetRenderZ 等24个入口求直接调用可达集合，共581个函数；另扫描 `.text` 中目标位移的直接内存、LEA及ADD／SUB取址候选，共123个候选函数。交叉结果只有 `62C0C0` 对 **FireCooldown +D4C** 的五处读取，没有这11个排除字段的访问。补查展示集合中的宽内存访问，也没有跨入这些字段的候选。
- 熟悉的间接展示反例继续保留：`61E360→62C0C0` 根据 FireCooldown／State 求充能条，`61E360→647310` 读取 State、Coins 和 SizeMulti 维护附属 Sprite；`60C190/60C1F0` 读取 IsFollower 并判断 Player 鞭形态。不是把 Familiar 所有标量都按 AI 删除。
- Familiar vtable `B15090` 的 slot22 GetForwardVector 为 `6B1FD0`，读取公共 Velocity `+360/+364`，没有读取这三项方向枚举。slot13 GetRenderZ 为公共 `6AC3F0`。slot4 **Interpolate `61DFD0`** 会读取／改写轨道角与速度，但它属于原生半帧模拟，Client 不放行，不能据此恢复轨道参数传输。
- 正面更新用途：`60CFA0` 在 `60D11A/60D122/60D1FB/60D24C/60D26D` 用轨道角、速度、层与半径求运动结果；`60BE50/60BEC0` 加入／离开轨道并修改 Host 编队。`612EE0` 在 `61303B/613565` 写移动／射击方向，在 `613570` 设 HeadFrameDelay=13，随后按动作分支读取、递减并选择动画。`6170F0` 经 `619860` 取 LastDirection 地址作更新选择。`623EA0/629BD0` 分别增 Hearts／Keys；`623EB0` 在 `623EE6` 增 RoomClearCount，并在奖励分支读取清房／红心计数。这些用途由 Host 执行，Client 接收其最终位置、动画与生成结果。

这是当前官方原版展示路径的静态采用结论，不是所有虚分派、任意指针算术或第三方 Lua 回调的形式化证明。与既有 Player 复合绘制和原生半帧隔离合同一起采用；未知字节不因此变成填充。11个公开 getter／属性的回映射改为 authority，不再比较 Client 的 Host 更新历史原值。

## 3. variant80：传最终充能结果，保持玩法 Weapon 为空

本地 FamiliarVariant 枚举确认80是 **INCUBUS**（不是61号 Lil Brimstone）。`61F580` 调用 `62C0C0`；其 `62C0E7—62C119` 分支读取 Familiar `+DB0`，调用 Weapon vtable slot7（`+1C`），将返回值落为f32，乘100，再在 `62C2DF` 向零截断为整数。Null Weapon 与比例≤0最终均按amount=0处理。

本轮核对16个 J273 原版 Weapon vtable 的 slot7：14个指向 **`9C5E80`**，UrnOfSouls／UmbilicalWhip 指向返回0的 **`9C4E60`**。`9C5E80` 只读 modifiers `+34`、charge `+14`，并调用 slot6 GetMaxCharge。全部已核 slot6 也是纯字段公式，没有写入、随机或 Lua 回调。正式采集复现这些公式，**不调用 getter**，不发送中间玩法量。

记 `d=f32(Weapon+10)`，`c=f32(+14)`，`m=u32(+34)`，`a=max(d,0.001f)`。slot6的结果须在原生返回／落栈位置按f32舍入：

| 动态类／slot6 VA | 最大充能 M 的原生公式 |
| --- | --- |
| Base、Ludovico、Bombs、Rockets；`9C4E60` | 0 |
| UrnOfSouls、UmbilicalWhip | slot7直接返回0，不必读取充能输入 |
| Deathmatch；`9D5150` | `a` |
| Fetus；`9D59F0` | `max(d*3,0.001f)` |
| TechX；`9DFBA0` | `d*3` |
| Bone；`9D0A50` | `(m&0x80E)==0`或`m&1`时`a*3`，否则`a*2` |
| Knife；`9D6A80` | `m&0x10`时0，否则`a*4` |
| Brimstone；`9D2A80` | `m&0x10`时0，否则`a*(m&1 ? 2.5f : 1.0f)` |
| MonstrosLung；`9DA650` | `a*(m&1 ? 2.5f : 1.0f)` |
| SpiritSword；`9DC070` | `d*4` |
| Laser；`9D82D0` | `s`初始为`m&8 ? 4.3000002f : 1.0f`，再按bit1乘2.5、bit2乘2；`s<=1`时0，否则`(d+1)*s-1` |
| Tears；`9DF230` | `m&0x100`时`s=12`，否则初始`s=1`，再按bit1乘2.5、bit2乘2；`s<=1`时0，否则`(d+1)*s-1` |

通用 slot7 规则：`m&0x80`或`M<=0` →0；`c<=1 && c<M` →0；其余为 `clamp(c/M,0,1)`。然后按 `62C0C0` 的f32乘法／截断求最终整数amount。这里bit1／bit2指掩码`0x1/0x2`。中间f32舍入顺序也属于实现合同，不能用一条double表达式替换全部原生步骤。

布局131的 `incubus_charge_bar_amount` 以 `+DB0` 为**换算来源**输出i32，并不把指针解释为i32。具名 Host handler须核动态vtable／slot6对应关系、来源generation及前后稳定；未知类型、非有限输入／中间值或来源变化报告Gap，不能伪装成0。只读本地对象关系，不把这些地址放进缓冲区。

Client writer只存本地表现缓存，按成员身份／generation退休；**不写 DB0**，不调用 CreateWeapon、Fire或Update。第13步的专用展示adapter从该amount进入原充能helper的选动画／定位／绘制尾部；保留充能条选项、出生6帧限制、Player隐藏判断，以及StartCharged／Charged／Charging／Disappear的状态语义。不能只画一个百分比矩形冒充原生效果，也不能让原helper因 `_weapon=nullptr` 提前跳过该输入。

DB8 ANM2资源与amount来自同目标版本；adapter的临时动画／姿态修改须完整借用并恢复，不污染共享资源帧，不重放动画事件。若采用Hook接入，纳入统一常驻清单，只在Client已激活且合法展示窗口覆写该分支，其他路径保持原行为。这些 handler／展示adapter **尚待第10／13步实现**；本轮补齐的是来源公式、最终字段与执行合同，旧Player Weapon实现不能自动算作已接管Incubus。

## 4. variant240：绳索不是 Position 的派生值

`61F551—61F560` 按 `+DAC` 是否非空向 `4308D0` 传入0／2；后者通过 `Familiar+ECC` 做 UmbilicalBabyData RTTI转换。全段为节点 `[0,n-1]`，后半段从 `floor(n/2)` 开始；实际原生循环还受起止比较约束，不能在Client重排或按距离重采样。

本轮补齐的原生事实：

- `42D778` 的 `operator new(0x158)` 核定payload分配大小。vtable **B0F2E8** → COL **B43470** → TypeDescriptor **BD12A0**，装饰名为 `.?AVUmbilicalBabyData@IsaacRepentancePlus@@`。
- `+04—17` 是五字MSVC deque：proxy、map、map容量、逻辑起点、元素数。`4308D0` 按 `map[(start+i)&(map_size-1)]` 取得节点，物理环布局不能直接复制到网络。`430C30` 每节点分配／清零 **0x78**，初建16项；不是Vector数组或0x54节点。
- 节点 `+00/+04/+08` 是绘制所需的 XYZ 三个f32。`4308D0` 用X／Y变换屏幕位置，并将Z按反射方向加到Y；因此不能只传二维Position。节点 `+0C—77` 的108B仍未逐项命名，不复制、不当填充。
- payload `+18—12B` 是 **ANM2**，由 `42D7DB/42D7E2` 构造，`42D84F—42D89E` 加载文件并选 `Cord`；`4308D0` 将其作为 `419D30/419F40` 的绳索绘制输入。复用完整ANM2外观／动画／图层布局，保留版本32的实际Image及本地loaded规则。
- `+12C—157` 的44B尾部仍未知。更新写入其中多个标量，但它不是已证ColorMod。payload原生表另包含嵌套ANM2的14B未知，合计58B；外部每节点108B未知单列。根Familiar的原有未知数量不变，不因此声明任何类型完整。
- `42D300` 删除析构释放ANM2(+18)与deque(+4)；**`430E40`** 为Familiar ExtraData建立／替换共享owner。不要混用Effect的`606310`，也不能浅写ECC／ED0两字。

新增 `UmbilicalRopeNode`（132，状态12B）与 `FamiliarUmbilicalBabyData`（133）；布局130保留真实DAC引用，追加F7—F10四个可选payload投影：Sprite外观、动画、图层、节点几何。沿用当前Familiar typed owner的每投影u16 count=0/1，并由生成的计数组约束四份count一致；所有投影同版本齐备后一次替换owner。缺失payload和读取失败不等价。

节点reader按逻辑顺序产出数组，`storage=array`表示这一自有顺序输出，**不表示原生deque连续存储**。声明上限16384是编码／内存边界，不声称实测需要该规模；只读稳定窗口、合法map／容量／节点、有限XYZ。非空payload的节点数必须≥1，零节点会使原生`count-1`下溢，报告Gap而非调用Renderer；超过边界不截断。NodeCount的下限由具名reader／writer执行，不能只看通用u16上限。

Client使用游戏兼容分配及具名deque／payload owner建立候选，安装XYZ和完整Sprite；未知模拟字节由本地构造策略维护，不从Host读取。**不运行`42D6F0`**：它是Gello的完整AI／物理更新，包含Player交互，不能当构造或展示维护。真正绘制继续使用已核 `4308D0`，反射、Player透明度及屏幕参数仍来自各自的Room／Player／本地展示合同。退休先撤DAC引用、正常释放payload／图像owner，不能执行Familiar原生级联Remove。

目前布局与已核展示输入已补入，原生候选构造／部分失败清理、采集和安装仍待第10／13步接线；不得将“payload已有表”写成“Gello已可玩／已验收”。

## 5. 联动与唯一验收

第10步不再复制11个Host参数，也不为HeadFrameDelay建立计时锚点。第12／13步不安装或漂移比较这些原值；第14步相应退役旧Source、字段写入和相等要求，保留原生语义记录、本地生命周期及实际展示字段。

第15步沿用已有条目，不新增提前测试：E09／E25覆盖轨道最终运动、拾取／奖励后的最终画面、方向及射击动画；Incubus覆盖各充能Weapon族、0／1／中间／100、modifier组合、空Weapon、隐藏／出生限制及动画阶段；Gello覆盖payload空↔非空、DAC空↔非空、逻辑环偏移、完整／后半绳索、Z与反射、资源变更。E13／E17覆盖amount越界、非有限节点、节点0／超限、错类型和投影count不一致；E28／E29覆盖借用恢复、未知字节不写入、owner失败回滚／退休及无玩法重演；E27另含新增payload尾部与节点未知区。新handler未接线或场景未执行均不能记通过。
