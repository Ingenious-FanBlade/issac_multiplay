# 第 4 步：Effect 后续五项未决的处理

日期：2026-11-09（Asia/Singapore）。范围经用户确认：原未决前四项之后的 VarData、Timeout、四参数分块、Trail 布局／上限、其他 ExtraData 消费者。布局版本 **40**。本轮只读本地 Wiki、官方 REPENTOGON 1.1.3 和 J273 IDA，修改布局与后续合同；不运行测试。正式采集、安装和验收分别仍归第 10／13／15 步。

## 1. 五项结果

| 项目 | 本轮结论 | 保留的边界 |
|---|---|---|
| VarData 如何复用，后 16B 是否使用 | **确实使用**。补入传播、定时器、生成请求、30／153 计数及 113 的伤害／尺寸基准五个具名原生臂；已核用途均不上线，FX2 继续预留 | 未核变体及各臂剩余空白仍为 UNKNOWN；不把整个基础臂猜成 Host 数据或填充 |
| Timeout 是否严格每游戏帧减一 | **否**。负数／零不由公共尾部递减，101／96 有局部递减，之后仍可到公共尾部；141 等还会刷新常数 | 不外推每个分支的运行频率；每窗重算 `A=F+T`，不依赖线性计时假设 |
| MinRadius／MaxRadius／Rotation／Scale 分块 | 前两项继续 Host；后两项有逐 Update 写路径，移至 FX0，避免带动 LifeSpan／following／ParentOffset 重发 | 这是静态分块决定，未测实际频率、带宽或性能 |
| Trail 节点与上限 | 64B 分配／完整复制由版本38核定；本轮核定 Update 正常返回后的逻辑数量 **0—127**，生成计划上限由16384改127 | Update 中追加后可短暂到128；不在该窗口采集，不截断、抽样或丢弃实际节点 |
| 其他 ExtraData 是否有展示消费者 | 原生 Effect ExtraData RTTI 派生表、构造／替换点与 Render 只找到既有三种具体类型，没有新增 payload | 只适用于目标官方映像；未知 RTTI／不匹配 variant 不得伪装成空，也不涵盖 Mod 新增类型 |

这些结论完成本批静态问题的处理，不表示第4步全部字节已核、实际同步已接通或第15步已通过。

## 2. 来源和分析边界

目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`。下文为 VA，RVA=VA−`0x400000`；未读取或校验哈希，未附着游戏或修改 IDB。语义来源为本地 `IsaacDocs/docs/EntityEffect.md`、官方 `libzhl/functions/EntityEffect.zhl` 和 `LuaEntityEffect.cpp`。官方 ZHL 也明确 `_varData` 可按 variant 任意复用。

复读 Init `5CD070`、Update `5D35D0`、Interpolate `5D1960`、完整 Render `600E60`，以及下列 producer／consumer 原始指令。全映像代码按内存位移 `420—43F`、`4B4／4B8` 和 `420／430／4B4` 取址立即数收集 **2067 条候选、350 个函数**。这些是含其他对象的候选数量，不是2067条 Effect 访问，也不等于全程序别名闭包。结合真实基址、variant switch、RTTI 和 owner 交叉引用采用结论；未确认的间接访问不能据“没有命中”归为填充。

例：邻近的 `5CCB20` 会复制 `this+438` 的16B，但调用点 `5D3BD4` 的 this 来自当前 Effect 的 Spawner；前置 `412D60` 明确检查 `Type==4`。这是 Bomb Flags 的读取，不是 Effect VarData 或新增 Effect payload。旧版本38的展示调用链证据继续适用，本轮没有把 Lua／第三方回调或间接虚调用声称为已经穷尽。

## 3. VarData：按变体归属，不复制重叠 union

新增布局137—141，原101—106不重排。只更改原生字节归属与选臂，未新增线上字段；全部臂 FX2 仍为空预留。113 从原102中分出，102继续覆盖54／126／167；公开 TearFlags 两项 authority 回映射复制到141，不丢失公开目录关系。

| 布局／variant | 已核原生范围 | 证据及归属 |
|---|---|---|
| 137 Propagation：32／44／45／46／53／204 | `+420 Vector direction`、`+428 i32 remaining`、`+42C f32 step_distance`、`+430 f32 damage`、`+434 f32 scale`、`+438 u32 trigger_frame`、`+43C i32 timeout` | `633330` 明确 Spawn Type1000，RTTI转Effect后在`633404—633489`写满32B；`5D3280`逐字段消费，`600753—60079E`只对所列六变体调用。全32B归 Host，Client不传播、Spawn或调用此helper |
| 138 Timer：119 | `+420 function* callback`、`+424 u32 interval`、`+428 i32 remaining_calls` | `6047F0`创建1000／119，`60482B／604831／604837`写入。Update `5FFBAF`检查callback，`5FFBE3`以interval做无符号除法，余数0才`5FFC00`调用；正remaining减一、零移除、负数不减。函数地址归本地，其余两值归Host；余20B未知 |
| 139 SpawnRequest：120 | `+420 i32 requested_type`、`+424 i32 requested_variant` | Init `5CF49E—5CF4BF`清两值；Update `5FFC5B`在dead分支读取两值及公共SubType，经`5FFD07 → 424340`生成最终成员。两值归Host，其余24B未知；不在Client重演生成／声音 |
| 140 DustCountdown：30／153 | `+424 u32 removal_countdown` | Init `5CFB88—5CFBA5`只对30／153清424／428。Update `5ED577—5ED5A1`仅在无符号正值时减424，归零调用Remove。只核定424的4B归Host；420及428—43F没有因此获得语义，仍28B未知 |
| 141 TearFlags113：113 | `+420 BitSet128`、`+430 f32 damage_basis`、`+434 f32 size_ratio` | `5FD04D—5FD053`由公共CollisionDamage保存430；`5FD06B—5FD07E`由Size／配置Size求比值保存434；`5FD0B9／5FD0CF`与`5FD61D—5FD745`继续调整／使用。两标量与原16B flags均归Host，438—43F仍8B未知 |

特别是167：Init `5D06F9／5D0703`直接写430=3.5、434=1，证明后16B有写入，不能继续写成“未发现使用”。但只有初值不足以将其完整原生语义归为113的同类字段；167仍在102中保留16B UNKNOWN。136完整32B GridDesc与其3B未知维持版本38合同，其他未核变体仍走101，不能自动套传播／Timer的布局。

传播字段的参数名由实际使用确定：438与原生FrameCount比较触发后继；43C传入新Effect的Timeout／LifeSpan。它们不是Client展示锚点。137的方向是Host生成输入；不因类型为Vector就误加状态wire。Timer的interval=0可使原生除法失败，但它不上线，Client既不执行也不为它增加线上校验／恢复逻辑。

生成后的根未知字节：101及104—106为382B；102为366B；103为353B；137为350B；138为370B；139为374B；140为378B；141为358B。它们包含公共Entity与内嵌ANM2未知，不能按全部类型相加声称消除了这么多独立字节。所有根类型 `complete=false`。

## 4. Timeout：否定统一递减假设，保留可逆换算

| 路径 | 原始指令与结论 |
|---|---|
| Init | `5CD1E7`默认写-1；其他变体又写正数及-1。不能把负数全部夹限为0 |
| 公共尾部 | `6006D8` signed `T>0`才进入，`6006E7—6006F6`执行T−1，`600702—600711`在到0时设置dead。T≤0保持；分支也可提前到`6007B5`而不经过尾部 |
| variant101 | `5E6FE2—5E7000`在正值时局部减1；`5E7082`仍跳公共`6006D2`。无其他改写且初值>1时，一次正常Update可减2；初值1只减到0。不能说“只有局部减、跳过公共尾部” |
| variant96 | `5E722C—5E724A`亦有正值局部减1；本轮不把其全部后续分支都概括为固定减2 |
| variant141 | `5D5789—5D5840`按Host状态／Parent条件将Timeout刷新为60，而后仍可进入公共处理 |
| 其他重设 | variant66的`5F35A5—5F35B4`由State−1重写Timeout；该字段不是单一线性时钟 |

每个合法完整采集窗读取当前T与同窗Host game_frame F，仍用模2^32的 `A=F+T`。Client每次安装新的world帧，以该帧F还原 `T=A−F`，没有新world时不自行推进。只在本次实际T变化恰好抵消F变化时A才不变；停走、负值、双减、刷新、跳帧均允许A变化和重发。相同目标版本的所有Effect共用同一展示时钟，回读亦同；不调用Update、SetTimeout或“补一次递减”修正。

第15步E34原表“锚点块在静止时不重发”不能作为对所有Effect的断言。本轮改成仅对已核线性递减区间要求不重发，其他情形按真实A变化发送并精确还原。

## 5. 四参数与FX0／FX1

MinRadius的`5D9502／5D960B`等写、MaxRadius的`5DE1E7／5DEC00`等写均是Host更新；版本38已经排除其同步，不再为测频率读这些原值。Trail淡出只同步最终节点，波纹等显示同步最终Sprite、位置、颜色和真实展示字段。

Rotation在172 Update `5DC640—5DC67E`满足计时条件时每次乘写，再被Render `603998`读取；Scale在125 Update `5D46DC—5D470D`满足动画条件时每次减写，其他已核Render／Light使用仍按版本38保留。故二者按动态输入编排，**静态写路径不是实测频率**，不声称所有variant每帧都改变。

| FX0块10：+0 i32 State | +4 i32 Timeout到期帧 | +8 f32 Rotation | +12 f32 Scale；共16B |
|---|---|---|---|

| FX1块11：+0 i32 LifeSpan | +4 u8 IsFollowing | +5 Vector ParentOffset；共13B |
|---|---|---|

两个f32的validator不变；全部块号、world序列、公共E0—E9及FX2空块保持。普通Effect最低编码总量仍420B，变化在两块内部布局，整数版本必须40；第9／12／13步按同一生成表消费，不硬编码旧8／21B。依赖Parent的仍是FX1，不能因移动旋转／缩放而放松同目标版本提交。

## 6. Trail：64B节点、逻辑deque和127上限

版本38已核 `6061CB／6061CD new(0x40)`，`6061E4—60621D`完整复制位置8B、ColorMod44B、scale4B、render_word4B、alpha4B；本轮复读相同原始指令。`+38`继续u32原位模式，不因距离累加误称Rotation或额外当f32拒绝。

本轮补核唯一append调用`5DA3C6 → 606190`以及后置裁剪：`5DA489`取逻辑数量；首节点alpha已耗尽且次节点满足条件时删除，否则`5DA4F8 cmp eax,80h`，数量≥128时仍在`5DA508 → 5CB250`弹出首节点，并由`5DA50D`回到检查头。因此正常Update退出时严格小于128，已有节点允许保留alpha为0的边界节点，不能在reader额外去零或去重。

节点数量在线仍为u16；源数量须先按原生宽度核对，**0—127有效，128及以上拒绝／Gap**。不能先收窄再判、截断为127或把坏deque当空。新建空payload和1节点均合法，Render自行处理不足2节点；缺失present=0与present=1且count=0仍不同。采集只在既定完整尾窗、原生Update和裁剪已正常返回后，前后核对holder／类型／deque拓扑；中途追加或owner异常报Gap。

单个节点仍64B，节点块上界 `2+127×64=8130B`，取代原 `2+16384×64=1048578B` 的通用预算。计数上限不代替实际字节预算／线程／代次检查。Client通过本地owner为逻辑有序节点分配独立存储，再一次替换；不memcpy活deque，不在Client运行追加、裁剪或衰减。

## 7. ExtraData：三个具体payload的静态闭合范围

原生base TypeDescriptor `BD15F4`、BaseClassDescriptor `B435F0`。其base数组引用仅`B435E4`、`B4360C`、`B436AC`及自身`B4362C`；前三者首项分别指向SpriteTrailData `BD155C`、PickupGhostData `BD1630`、MrMeData `BD14CC`。相应虚表`B14BE0／B14BF8／B14C0C`的写入分别落在三种已知构造路径，base虚表的其余写入为基类构造／析构，不是第四种payload。

`606310`的六个直接调用点：Init清空`5CD0B5`、108创建`5CF2B6`、166创建`5D068C`、191创建`5D1007`、166缺失重建`5DA276`、ClearReferences清空`604502`。加上构造清零、两个析构中的控制块释放，holder的已核生命周期没有新增具体类型。Render直接读holder仅`6012D9／602C2E／60423D`，均紧接精确RTTI转换；Interpolate没有此holder访问。其他全映像同位移候选含Bomb costume、Laser参数、Weapon、保存数据／编辑器对象与栈地址，不能据位移相同认作Effect消费者。

所以不增加第四种同步payload。正式Host选臂时还须检查：非附属臂holder为空；附属臂非空则具体RTTI匹配variant，缺失经规范present表示。非空未知类型、variant不匹配、holder变化或读取失败均报缺口，不能按“其他臂只归Host”静默丢弃。它是窄读取资格，不传输holder地址，不调用虚方法推测类型，也不重建Host私有对象。Client只接受所选布局允许的payload并经既定`606310`正常替换；没有新Hook或payload执行器。

## 8. 失败方式与后续工作

- 错把所有变体的32B解释成flags／传播结构，发送函数指针、池残留或未知字节。
- 113移臂后漏掉TearFlags公开回映射，或使用旧102布局安装113。
- 固定认为T每帧减一，夹限负值、缓存旧锚点，或要求非线性区间绝不重发。
- FX0／FX1仍按旧8／21B偏移解码，Rotation／Scale落入错误字段。
- Trail将128收窄／截断、在Update中途采集、把deque当连续数组或把空payload等同缺失。
- 以RTTI静态清单推断任意第三方类型安全，或将未知非空holder伪装为空。

布局及第10／12／13／14步合同随本轮同步；第15步沿用E09／E25／E32／E34核最终展示、11臂选择、线性与非线性计时、Trail边界和owner变化，E13／E17核非法count／布局／资源／实际预算，E28／E29核隔离及生命周期，E27继续观察真实UNKNOWN。不得在第15步前运行这些用例。

剩余问题已从原五条泛化问题收窄为：未核variant和167等残余union字节、运行时分支／频率覆盖、未知间接类型边界及正式reader／writer接线；NPC／Familiar、Delirium及Text／Dummy等第4步原有工作不在本批完成范围内。

## 9. 静态审阅与纯编译

唯一源为`mod/binding/state/entity-effect.json`，整数版本在`version.json`递增为40；普通产品构建重新生成四份C++计划及全部单行布局表。新137—141原生字节归属表、各臂状态表、127上限与FX0／FX1偏移均由同一源输出；正文列出的未知数与生成表一致。更新第4／9／10／12／13／14／15步及主要资料入口，历史版本只补后续指向，不改写其原始观察或当时编译结果。

普通Release／Win32／v143产品构建使用`ISAACCS_BUILD_E2E_TOOLS=OFF`，`isaac_cs_state`和完整`zhlIsaacCS`编译／链接成功。没有运行单元测试、E2E、独立校验脚本、游戏、注入、部署或未知区实验，没有安装Hook或修改IDB，也没有另外归档artifact。生成器的结构／语义处理属于正常代码生成；本轮不把编译成功写作运行功能通过。
