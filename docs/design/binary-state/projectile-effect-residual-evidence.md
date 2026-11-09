# 第4步：Projectile展示来源与Effect残余未知区

日期：2026-11-09（Asia/Singapore）。布局版本45。本轮只处理用户指定的Projectile `+440/+460/+464`和Effect未核variant／各臂残余／三段空白；不重做版本40已经处理的Timeout、四参数分块、Trail节点／上限及ExtraData清单。目标为J273官方映像，ImageBase=0x400000，以下为VA（RVA减基址）。仅只读IDA、文件／本地Wiki／SDK及代码生成，没有改IDB、附着游戏或运行测试。

本轮Effect主Update `5D35D0`的反编译接口失败，其他长函数的工具响应也存在正文截断；采用switch元数据、只读CFG候选定位及原始指令逐目标复读，不把截断伪码当完整函数，也没有为获得伪码修改IDB或猜ABI。

## 1. Projectile：实际要补9B，其他已核输入留Host

| 原生范围 | 新归属 | 正面证据与决定 |
|---|---|---|
| 430—433 f32 | Host轨迹距离比例参数 | Init清0；`516D0E`写80；`65534E/65536A`及`6554F8/655514`在正阈值分支用当前距离除该值，再改变Velocity；不外推物理单位，不上线 |
| 434—437 4B | UNKNOWN | 没有取得实际语义或填充证明，不由相邻f32／64位flags推导 |
| 440 u8 | **状态，新增1B** | ctor／Init清0，FireProjectiles从params+A4写入；Render按零／非零决定临时减Height。只核定byte，不把441—443或444纳入开关 |
| 441—443 3B | UNKNOWN | 没有语义／填充证明 |
| 444—447 i32 | Host动画选择缓存 | Init=-1；Update按variant／Scale比较100、200、300等组键，变化才重选最终动画；Client已有最终ANM2，不同步重选缓存 |
| 44C—44F 4B | UNKNOWN | `650286 lea ecx,[eax+44C]`的eax是整数动画索引，44C是加1100的立即数语义，不是本体基址；仍无填充证明 |
| 460—467 Vector | **状态，新增8B** | ctor／Init默认值；冻结触发保存当前Position；特殊Render使用该锚点；不能用最新Position或Parent替代 |
| 468—46F Vector | Host父速度历史 | ctor／Init默认值；父对象绕行分支从Velocity扣旧父速度，再保存新父速度；不是460向量的后半部，也不是展示字段 |
| 470—473 function* | 本地，既有决定保持 | 原生更新回调，不发送或调用 |
| 474—477 4B | UNKNOWN，既有边界保持 | 对象大小0x478不证明尾字填充 |

原生复制与线布局仍由[EntityProjectile生成表](layouts/EntityProjectile.md)给出。PR0块10保持4B Height；PR1块11继续空预留；追加PR2块12的9B：height_render_mode u8及freeze_render_position Vector，线偏移0／1。Client按memcpy／数字字段解码，不能把未对齐Vector强转为原生结构。开关与冻结锚点独立分块，避免Height变化时重复发送这9B。最小总状态395→404B；这是公共动态部分取最短的布局算术，不是带宽实测。UNKNOWN 375→350B，其中公共335B、本类15B；消除的是有正面用途证据的25B，不是将空白猜成填充。

### 1.1 初始化与发射生产者

真实Projectile ctor `64F420`先调用公共ctor、于`64F42F`安装vtable B15B1C。`64F437`写440单字节0，`64F43E/64F448/64F452/64F45C`清464／460／46C／468四个DWORD。Init `64F4B0`先公共Init，`64F57A`再次清440，`64F5DF/64F5EF`从全局默认Vector复制460／464，`64F6DC/64F6ED`同样初始化468／46C。槽复用由当前正常Init建立资格，不能仅凭首次发现或旧use读取。

默认Vector的VA为BEB900。它位于PE `.data`的零填充尾部：section RVA787000、VirtualSize7C704、SizeOfRawData4E000，原始数据终点VA BD5000，BEB900及BEB904都在其后。IDA的`is_loaded=false`；对无原始字节地址读到的FFFFFFFF不是目标NaN初值。本结论来自PE头和明确复制指令，不是实机观察；正式读取仍要求当前完整窗口及有限值，不以常量猜测代替实际字段读取。

`6C74B0`对应官方 `Entity_NPC::FireProjectiles`（SDK EntityNPC.zhl）。其各模式经Game Spawn Type9，并RTTI转为Entity_Projectile，加入函数本地的弹幕指针数组；共同尾部`6C8C10`开始逐个应用ProjectileParams。`6C8D96—6C8DA2`从当前数组取真实Projectile，读params+A4的byte，写Projectile+440。官方ProjectileParams.zhl大小A8，但未声明+A4成员；Wiki亦未暴露它。因此这里只命名实际用途为height_render_mode，保留原始u8的零／非零语义，不捏造公开参数名或将合法字节限定为0／1。参数对象整体不进入同步。

该尾部还在`6C8E04—6C8E15`把params+A0写入470回调，之后才通知已核发射后置路径。此邻接关系不能把440或470混作Plus，也不授权Client执行发射／回调。无论经何种合法创建，正式采集都在原生创建／后置参数应用已正常完成的既定完整窗口；Init中途不发布。

### 1.2 冻结锚点生产、消费和清除

本地Wiki EntityFlag明确`1<<5`为FLAG_FREEZE。Update `657231—65729B`经过当前游戏条件、随机条件及非dead判断后，`65729D`取本体Position+33C，`6572A4`取本体460，`6572AA`调用Vector赋值`9E5F20`。该helper只复制0／4两DWORD，证明锚点是8B而非16B。随后`6572B5`设置FLAG_FREEZE并清Velocity。冻结且非dead的Update入口`64FA79—64FAB7`保持零Velocity并走公共更新，不再次保存Position。

`6594C7—659501`先查公共FLAG_FREEZE，再将460／464和默认Vector逐分量比较。非默认锚点时`659507—659637`以该锚点、公共PositionOffset和当前渲染offset计算屏幕位置；继续用Host Game展示帧生成局部裁切／抖动。默认锚点或无FREEZE走公共Render。它不是`cached_position==current_position`判断；早期伪码复用了变量名a7，不能据那个表达式误判为恒等比较。

故冻结后的当前位置改变或插值历史不能替代这个独立锚点。`64FA3A—64FA71`的解除并Remove分支先清FREEZE，再把460／464恢复默认Vector。此处只确认这条实际分支，不外推任意移除都先执行相同清除。Client直接接收当前锚点与公共flags，既不触发冻结随机逻辑，也不重放Remove。

440控制的高度借用保持版本36合同：`65946B—659484`进入绘制前从PositionOffset.y减Height，`6597B8—6597D1`加回。第13步必须精确保存／恢复原始公共偏移，不把减加当逐位恢复证明；保留同world Game展示帧，不换为Client本地时钟。

### 1.3 相邻字段不能扩大为展示负载

`6552C7—6552D4`以468为源从Velocity扣除旧值；`655454—65546D`从Parent的公开Velocity getter取8B并赋给468。`9E5DD0`只逐分量做减法，`9E5F20`只赋两DWORD。它是Host父速度历史，虽同为Vector，也不传输。

430的外部producer也有正面类型链：`516CFB`调用FireProjectiles，返回值于`516D00`成为esi，随后写430=80；`516AEE／5176C7`明确Type9的持有对象分支会清430。不能把这些语句当成Effect+430。444的缓存键只影响Host选动画；`650281—6502C1`尤其把整数动画索引加1100后与本体444比较，44C立即数不应认作另一个原生字段。

## 2. Effect未核variant：新增内部flags臂

本地EffectVariant目录分别命名19=CRACK_THE_SKY、71=BRIMSTONE_SWIRL、194=DARK_SNARE。新增149号InternalTearFlags臂，三者420—42F为Host内部BitSet128，430—43F仍UNKNOWN。它们不因原生使用就获得公开Get/Set/Add/Clear/HasTearFlags资格：官方LuaEntityEffect.cpp仍只允许54／113／126／167。

| variant及入口 | 正面consumer | 字节归属 |
|---|---|---|
| 19，主Update switch→5E63A3 | `5E6D38—5E6D4A`将当前Effect+420交`412B10`，检测效果位；`5E6D8D—5E6D99`以其地址调用`5CC9F0`清位 | 这两个helper只操作0／4／8／C四个DWORD；16B完整Host效果位，后16B没有因此得到语义 |
| 71／126，共享入口5F4B61 | variant126的专属支路结束于5F4DBA；共同5F4DCA检查非dead、subtype!=1后，`5F4DF4—5F4E10`对本体420做完整16B位判定。71可由非126分支直接到共同段，不把共享case无条件等同126 | 71新增内部臂；126仍102公开允许臂。两者Host玩法，不进入当前Render |
| 194，主switch→5E3BB5 | `5E3D7E—5E3DA1`从本体420复制四DWORD至参数栈，`5E3DB0`交677940伤害处理 | 16B完整Host效果位；不将该结构与GridDesc或传播结构混用 |

公共Init `5CD265`只清420—42F的16B；它是上述flags的明确默认初始化，而非后半区清零证明。位运算helper `412B10`、`431290`和`5CC9F0`实际边界均为16B，不触及430。当前主Update按variant1—204分派；静态CFG可达集合仅用于定位，具体对象／条件以原指令复读。其他未核variant仍用101的32B未知，不从“这些分支没有直接偏移命中”推成不适用、Host或填充。

## 3. 167：独立148号臂，不套113结构

Init `5D06C0—5D0714`把167的430写3.5、434写1，前16B flags清零。新增producer `7037A0`与官方Game.zhl的`Game::ChainLightning`匹配：`703968`明确生成Type1000／variant167，`70397A`设置最终CollisionDamage，`7039BA`从该实体+388保存至430，`7039C2`写434=1，`7039D1`写传入16B TearFlags。

- 430—433可明确称为**生成时伤害快照**，归Host生产元数据；没有当前展示consumer。没有找到后续独立读取，因此不声称113的damage／size更新公式同样适用于167。
- 434—437仅确认f32常量1的两条写者；未定位后续用途。仍为UNKNOWN，不能据常量或同址名称认成size_ratio。
- 438—43F仍8B UNKNOWN，没有完整初始化或用途证明。

167 Update入口5DE145；`5DE9BA`生成真正的Laser，`5DEA10—5DEA2E`传本体**当前公共CollisionDamage**，`5DEA40—5DEA82`仅按16B flags构造Laser参数，没有读取430／434。Render当前没有167专属读取这些尾字的分支。由此分出148号ChainLightning：flags16B及伤害快照4B归Host，余12B未知；102现在只含54／126，141的113保持原合同。根未知167由366→362B；不宣称类型完整或玩法后续链全已理解。

## 4. 119残余：按callback复用，避免错误统一

本轮发现119的一个具体callback闭包会用到430—437：官方Game::BombExplosionEffects对应`6FBEA0`。`6FBF8B`创建1000／119，`6FBF9B`设callback=6FC690，`6FBFED／6FBFFE`保存传入64位DamageFlags的两DWORD。回调`6FC716／6FC722`原位取出并交`6FAF60`爆炸伤害处理。其他上下文参数存于公共Position／CollisionDamage／Scale／Parent及flags，不把它们误指为新的VarData字段。

这证明**该callback的8B具有语义，不能当填充**；不证明全部119都使用同一种payload。通用119允许其他callback，公共Init也没初始化这8B。138号臂因此把旧20B未知拆为42C—42F 4B、430—437 callback-specific 8B、438—43F 8B，并记录条件及生产／消费证据，全部仍保持UNKNOWN。没有把本进程callback地址变成线上selector，没有给其他callback虚构DamageFlags，也没有新增Client回调执行器。119根未知仍370B。

## 5. 各臂剩余与空白的边界

| 布局 | 420—43F残余UNKNOWN | 其他本类UNKNOWN | 根UNKNOWN（含公共335B） |
|---|---|---|---|
| 101基础；104／105／106继承基础复用区 | 全32B；公共Init仅清前16B | 41C／44C／4BC各4B、465—467 3B | 382B |
| 102：54／126 | 430—43F 16B | 同上15B | 366B |
| 103：136 | GridDesc 439—43B 3B | 同上15B | 353B |
| 137：传播六variant | 无；原32B生产／消费者已核，本轮不重做 | 同上15B | 350B |
| 138：119 | 42C 4B＋430 callback专用8B＋438后8B；具体闭包已知不覆盖全部callback | 同上15B | 370B |
| 139：120 | 428—43F 24B | 同上15B | 374B |
| 140：30／153 | 420 4B＋428—43F 24B；428已见Init清零但未得实际用途 | 同上15B | 378B |
| 141：113 | 438—43F 8B | 同上15B | 358B |
| 148：167 | 434 4B＋438—43F 8B | 同上15B | 362B |
| 149：19／71／194 | 430—43F 16B | 同上15B | 366B |

41C／44C／4BC的既有全映像候选、邻近取址、跨字段宽访问排除见[前四项静态证据](step4-first-four-evidence.md)，原结论保持。本轮按真实指令操作数重新归并指定偏移候选，与Init／Update／Interpolate／Render和上述新增具体producer／callback对照，仍没有这三段属于Effect本体的正面语义或源级填充证明。新看到的81AF0C宽写来自明确Spawn1000／136，覆盖430—43F，止于440；链式flags helper止于430；两者都不覆盖空白。

SDK BitSet128只有四u32；不可恢复“它要求8B对齐所以41C是padding”的旧推理。EntityPtr444只4B，holder4B4只8B止于4BB，也不能据相邻对象边界把44C／4BC改为填充。465后的3B及GridDesc439后的3B同样不因bool存储为1B就获得填充证明。上述UNKNOWN不读、不编码、不补零，不用全函数图或阴性搜索宣称任意地址别名的形式化闭包。

## 6. 后续接线、失败方式与构建范围

唯一源更新entity-classes.json／entity-effect.json及整数版本45，生成表中Projectile PR2新增9B，Effect新148／149不增加线上字段，13个Effect布局仍保留FX2预留。图形与字节完整性分开：Projectile本轮三个指定展示字段来源已闭合，其他未知仍在；Effect剩余表中的未知仍未解决。

第10步须在同一合法完整窗口复制Projectile PR0的Height及PR2的440／冻结Vector，并与公共flags／最终偏移／资源一致；第12步按PR0=4B、PR2=9B解码，检查实际浮点与资源，440保留任意u8零／非零语义；第13步按实际draw分支借用并精确恢复公共偏移，不能冻结随机逻辑、运行原生Update／Interpolate或根据当前Position重算锚点。Effect按新variant分派，不重演伤害、发射、Timer callback或ChainLightning生成；第14步切换旧路径时不恢复已经排除的Host参数。

第15步沿既有E09／E25／E28覆盖高度开关零／非零、非零偏移、冻结锚点与当前位置不同、默认锚点、冻结结束／Remove、Shadow／水面与draw恢复；E13／E17覆盖真实线上浮点非法与截断，不能将u8=2误拒绝为非法bool。Effect的19／71／194／167新选臂、54／126原臂、119已知／其他callback及各未核variant必须保持正确职责；不比较不上线字段。E27按本轮收窄后的UNKNOWN观察，未覆盖或变化均如实保留。这里仅安排，不执行测试。

普通Release／Win32／v143产品纯编译／链接已完成，ISAACCS_BUILD_E2E_TOOLS=OFF；最终PR0=4B／PR2=9B生成表重新编译进isaac_cs_state，完整zhlIsaacCS链接成功，MSBuild退出码0。没有运行测试、游戏、注入、部署或独立校验程序；实际collector／writer接线仍属第10／13步，唯一功能验收在第15步。没有建立单独artifact包。
