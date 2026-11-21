> **版本46 Delirium未决事项（2026-11-09）**：[静态证据与归属修订](delirium-residual-evidence.md)核定两个8B Host打包存储，BDD／BE4共7B移出独立未知；BD4—BD7的4B仍无填充证明，根未知597→590B，最小线状态仍710B、17／18空预留。池槽残留已有静态可达路径但未实测；补核4A13B0的独立companion创建／BE8登记，BE8继续留Host。实际接线仍归第10／13／14步，唯一功能验收在第15步。 下文保留旧版本记录；BE8仅49F3D0访问的旧结论已纠正。

> 版本43补充：[受控UI来源已闭合](npc-control-ui-evidence.md)，模式仅0/2，不读取原版NetplayManager；上轮误解与纠正保留。

> **版本42后续复核**：[NPC同步职责复核](npc-synchronization-review.md)取代下文版本41传输决定。BossType／Variant取消8B传输与shape；ControllerId不复制本地设备键，改1B受控UI模式；+CD8 OverlayEffect仅排除绘制前完全覆写的8B Scale，使用144号场景视图。NPC／Delirium最小状态均710B，UNKNOWN和SDK边界不变；实际控制UI来源／adapter仍待第10／13步。下文保留版本41原始静态事实与编译记录。

# 第4步 Entity_NPC：J273静态证据与布局

日期：2026-11-09（Asia/Singapore）。本轮为主智能体只读SDK、Wiki、现有实现及IDA静态分析，没有运行游戏、测试或修改IDB。目标IDA输入仍为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；下文地址为VA，RVA减去该基址。本轮布局版本41，类型142／143。代码生成与普通Release编译不等于运行验收。

## 来源与对象边界

- 官方 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityNPC.zhl` 声明0xF60及公开成员；`NPCAI_Pathfinder.zhl` 声明0x750。本地原版 `IsaacDocs/docs/EntityNPC.md` 与REPENTOGON `docs/docs/EntityNPC.md` 用于语义核对，公开API不作为必须传输的理由。
- 物理ctor `6B4520` 先调用Entity ctor，写NPC vtable `B187D0`；COL `B43A08` 指向TypeDescriptor `BD1344`，名称 `.?AVEntity_NPC@IsaacRepentancePlus@@`，故布局源RTTI RVA为 `0x7D1344`，不是虚表RVA。
- Factory `6EEE60` 在 `6EF01D` 为NPC池压入单元素大小 `0xF60`；删除析构包装 `6B4870` 的 `6B4881` 同样压入 `0xF60`，再压对象并释放，回收8B参数。二者支持实际sizeof；Hex-Rays隐藏释放函数第二个参数，不能据伪码漏参否认原指令。
- vtable关键槽：+4 Init=`6B4A50`，+C Update=`6C03B0`，+10 Interpolate=`6CE990`，+14 Render=`6CEC90`，+18 Shadow=`6D0650`，+44 ClearReferences=`6B4E60`。本轮不新增Hook、不放行Client AI或Interpolate。

## 构造、Init和Update

`6B4556` 把this+414交给 `7D97D0`，其 `Reset7D9890` 清寻路状态并维护尾部树。NPC Init再次Reset本对象。寻路对象首字是本地Entity回指，内部有448项SDK路径缓存和拥有的树，不发送其指针/容器头或在Client重建算法对象。0x750边界与用途有依据，内部每个成员的完整用途尚未恢复，所以源保持“SDK声明”；它不表示1872B均已逐成员核清，也不将所有空白伪装填充。Familiar仍沿用其自身+5FC来源，不把两类对象互相解释。

ctor分别在+CD8／+DEC调用 `402B00`，析构 `6B48A0` 分别调用 `403790`，核定两份独立0x114 ANM2。它还分别构造+BF0、+C04、+C18的20B deque及+C48的第四个同形头；后三组之外的+C48完整用途本轮未核，仍在UNKNOWN中，不能因为认出头形状就删除它的未知归属。+B88、+F0C的私有拥有容器同样不凭构造清零宣称语义完整。

Init重置State、StateFrame、V1/V2、I1/I2、各条件计数与部分私有字节，不会清整个子类；它在+F50／+F58已有目标时还会调虚Remove，不能作Client补姿态的通用入口。Update `6C03B0` 已静态反编译并核对混合区访问，包括+C80颜色段和+F4C资源/音效选择；这些访问不等于完整语义。本轮保留未核区，不为读取资格虚构初始化历史。

## 实际保留的标量与关系

| 原生槽 | 线上值／归属 | 直接证据与安装含义 |
| --- | --- | --- |
| +B64 i32 State | N0原值 | Render在 `6CF1BF` 比较273分支state9；`5891F0` 比较909:10的state6，`49A160` 用于910绘制。Client不推进状态机。 |
| +BA4 i32 ProjectileDelay | N0原值 | `4E4710` 的 `4E4D48` 读取本槽参与曲线显示；不能按名称归为单纯发射冷却或线性计数。 |
| +BB0 Vector V1 | N0双f32 | `6CFE7C` 用于964数值绘制；`564630` 的815几何和 `54D1A0` 的881展示读取两分量。不能只保留X。 |
| +BC4 i32 I2 | N0原值 | `59CDC0` 在 `59CDED／59CDF7／59D05D` 比较连接段，主Render与Shadow的19:3分支消费。 |
| +C64 i32 ChampionColorIdx | N0原值 | `6CF121／6CFE14` 对21作临时缩放分支；最终颜色不能替代这个选择输入。 |
| +F1C i32 ControllerId | N0原值 | `6D025D／6D0314` 筛选控制归属；影响HP展示与+DEC副Sprite的绘制，保留-1哨兵。 |
| +BEC EntityPtr | N1成员序号 | `6CF743—6CF7E9／6CFCD4—6CFDE3` 的245、411／816绘制借用目标。空与非空保留真实身份，安装使用EntityPtr owner。 |

ParentNPC／ChildNPC沿用公共E8 Parent／Child，GetFlyingOverride沿用E9现有Plus可空值，不复制别名。EntityRef与复合deque引用在主线程转为当前generation身份句柄，后台再分配成员序号；不能把 `ReadDequeInto` 的原生指针暂存交给后台。

留Host的已知输入：StateFrame、ProjectileCooldown、V2、I1、Scale、CanShutDoors、IsBoss、ShieldStrength、IsChampion、DarkRedChampionRegenTimer、BossColorIdx、GroupIdx、DirtColor、命中历史与五个私有AI／退休关系槽。V2／I1保持SDK声明，排除传输不提升语义证据等级。具体依据包括：`6D3040／6D3080` 将Scale换成最终Sprite.Scale；`6BC640` 是DirtColor唯一非ctor直接消费者；`6BD290／6CC300` 由BossColorIdx生成最终主Sprite资源/颜色；+C74用于Update/再生；+F28／+F38／+F50／+F54／+F58由AI、碰撞及清理路径消费，没有在本轮已核draw路径找到展示消费者。最终HP、Sprite、碰撞类别及独立HUD仍由各自布局供应。以后若核到新的实际展示消费者，应补具体字段/域合同，不能由本轮有限静态范围宣称全游戏不存在间接访问。

`+F40`继续是本地拥有的Siren玩法actor。沿用[既有具名owner证据](../tx-04-19/entity-i20-i25-native.md)，Client不以roster借用或第二个Player复活它，不因Host非空就回传进程地址；本地异常非空须由合法owner处理，不能裸清指针泄漏对象。清理五个Host私有关系槽时也只撤本地EntityPtr登记，不执行连带Remove。

## 三组deque

沿用并复读[复合绘制合同](../tx-04-19/entity-i20-i25-native.md)与 `mod/src/game/client_entity_npc_native.cpp` 的真实布局/allocator/关系责任。源现声明 `storage=deque`，不伪装vector或连续数组。

| +0 u32 proxy【本地】 | +4 u32 map【本地】 | +8 u32 map_capacity【本地】 | +C u32 logical_offset【本地】 | +10 u32 count【派生】由线上逻辑count及本地owner维护；完整头20B |
|---|---|---|---|---|

| 根槽／线上块 | 元素 | 展示消费者与约束 |
| --- | --- | --- |
| +BF0／N1 | 4B原生EntityPtr→2B成员序号 | 835的 `555410` 固定读前两项；906／920遍历真实child资源。0是合法空集合，count=1的835不满足consumer前提，须写前拒绝，不能补Null凑数。 |
| +C04／N2 | 8B Vector→8B双f32 | 921 `574930`，815 `564630` 与881 `54D1A0` 复合展示；必须保留逻辑全序。 |
| +C18／N3 | 8B Vector→8B双f32 | `4E4710`／`599070→598B10` 曲线；951:3只有count≥9才进入实际分段读取，邻点/三点步进边界需同当前绘制owner核定。 |

每个容器上限16384，不截断；引用块每16B容4项，点块每16B容2项。`ReadDequeInto` 在主线程窗口把有效元素归并到调用方预分配区，无逐对象vector分配。校验真实proxy、空头、power-of-two map、容量/数量/地址边界、每个被访问block和前后头一致性；允许空容器保留已分配map。环形offset不充当状态，跨map尾和32位逻辑加法均以64位计算处理。输出暂存的4B引用仍须立即转换，点值随后按Vector生成表校验有限值。坏头/超限/窗口失效/读取失败均为Gap，不能冒充空集合。

Client owner需要保留原ctor proxy，以游戏allocator建立候选map与16B blocks；引用先两阶段解析，调用SetReference维护目标反向树。空集合和退休应撤每个引用、释放本地blocks/map且保留proxy，不传原头、不触发child Remove。当前旧Lua／JSON链的owner仍在；新字段处理器的实际接线归第10／13步，本轮没有把旧执行路径悄悄切换。

835真实Player child仍只保留稳定关系，普通C/S须满足Deathmatch state<2；绝不把Player+C18解释为NPC点列。906／920借用child Sprite，965:10借用Player Minecart的跨域依赖继续遵守既有合同。本轮的布局迁移不自动放宽任何实际draw边界。

## Delirium臂

既有[第4步4.2](4-entity-classes.md)的J273事实保留，本轮复读 `49F3D0／4A1A30／4A2720` 原指令。先按NPC RTTI分类，再以公共type=412选择143号臂；AI内临时type不能选臂。BossType／BossVariant分别为+BC8／+BCC，继续发送8B结构shape，覆盖有Init及不经Init的改写路径。它们的保留依据是形态/结构事务，不要求普通Render直接读这两槽。

旧草稿DL1的13B（TransformationTimer、RemainingAttacks、StateD、AttackID、Angle、Cycle）本轮归Host，块18保留空编号，不再发送或在Client复演；它们是AI输入/条件计数，`6CEC90` 不读取此段。AttackID四字节打包、Cycle的64位访问及BE8真实EntityPtr的原事实不变。+BD4的4B、+BDD的3B、+BE4的4B继续UNKNOWN。普通NPC的36B臂归不适用的Host区域，不读取可能池槽残留。`world_delirium_owner`只恢复读取资格，新链的shape统一由shape字段比较推进，不重复换代。

## 原生归属和线布局

完整单行原生表与逐块单行二进制表由唯一源生成：[NPC](../../../../mod/binding/layouts/04-entity-classes/EntityNPC.md)、[Delirium](../../../../mod/binding/layouts/04-entity-classes/EntityNPC_Delirium.md)。下面只摘独立子块，公共E0—E9不重复定义。

| N0块10：+0 i32 State | +4 i32 ProjectileDelay | +8 Vector V1（双f32） | +16 i32 I2 | +20 i32 ChampionColorIdx | +24 i32 ControllerId；共28B |
|---|---|---|---|---|---|

| N1块11：+0 u16 EntityRef | +2 u16 count | +4 count×u16 composite member index；4—32772B |
|---|---|---|

| N2块12／N3块13：+0 u16 count | +2 count×Vector（x:f32,y:f32）；各2—131074B |
|---|---|

N4／N5／N6分别顺序投影两份ANM2的外观／动画／图层，最小246／44／4B，长度及计数各自独立。文件名相同也不合并实例，不能从主Sprite推导辅助Sprite。DL0块17为两个u32共8B，仅143号臂有值；142号块17及两个布局的块18均空预留。根状态最小NPC721B／Delirium729B（含公共391B；不含外层帧/成员头及实际变长负载），不是实测带宽或性能结果。

| 类型 | 根未知 | 分解 |
| --- | ---: | --- |
| EntityNPC 142 | 586B | 公共Entity335B＋两份ANM2各14B＋NPC本类223B |
| EntityNPC_Delirium 143 | 597B | 同上＋Delirium臂11B；其他普通臂36B不算未知也不上线 |

NPC本类223B共15段：B68—B9F 56B、BA8—BAF 8B、C2C—C37 12B、C3E—C3F 2B、C44—C62 31B、C68—C73 12B、C76—C77 2B、C80—CAB 44B、F0C—F1B 16B、F20—F27 8B、F2C—F37 12B、F3C—F3F 4B、F44—F4F 12B、F5C 1B、F5D—F5F 3B。包括已见构造/Update访问但用途未闭合的字节，不是都“未访问”。外部deque元素按对应元素表处理，不算根对象未知；寻路1872B声明状态另计，不能将586B读成所有研究缺口总量。

## 落地与剩余工作

- `entity-classes.json`新增142／143；整数版本41；公开NPC34个唯一ReplicatedState及Delirium10个唯一成员均显式回映射，Pathfinder重复目录ID只映射一次。普通臂对Delirium标不适用，143号臂逐项覆盖，避免拿普通NPC数据冒充已读Delirium。
- schema／生成器认识20B deque，严格要求本轮核定的4／8B步长。`nested.h/.cpp`新增无分配的 `ReadDequeInto`，保留主线程、generation和读取失败语义；不新增网络协议或游戏Hook。
- SDK ABI编译对象新增NPC size、Pathfinder size、已核声明偏移断言，不链接该对象的官方配置初始化进产品。
- 普通 `cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /nr:false /v:minimal` 成功，E2E_TOOLS保持OFF。基础库、SDK ABI对象及完整DLL编译／链接通过；SDK既有 `CMAKE_REPENTOGON_VERSION undefined` 提示仍在。没有运行测试、部署、注入或启动游戏，也未创建artifact包。

第10步接入真实ordinary窗口、具名deque/资源读取与身份预约；第12／13步接入逐数字字段解码、所有依赖齐备后的原生安装与绘制借用恢复；第14步退役旧Lua／JSON来源。NPC专有未知、寻路逐成员缺口、各曲线索引前提、特殊draw跨域依赖及异常前缀仍须继续处理，最终运行覆盖只在第15步统一验收。Text／Dummy不在本次NPC交付范围。
