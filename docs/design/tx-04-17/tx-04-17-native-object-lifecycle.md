# tx-04-17 R2：原生对象的create／remove／change公共生命周期

日期：2026-11-01，Asia/Singapore。范围归同一tx-04-17，接续[R1初始化范围](tx-04-17-client-scene-lifecycle.md#r1-results)。本轮只读公开接口、官方REPENTOGON 1.1.3源码、当前工程及必要J273静态路径，再更新设计文档；没有修改代码、绑定、游戏文件或IDB，没有构建、运行测试或保存artifact包。

必要逆向按用户要求由一个GPT-6 Luna／max子智能体执行，主智能体独立回读关键指令与源码后取舍。`PUBLIC_DOC`、`SOURCE`、`J273_STATIC`、`DERIVED`、`DESIGN`、`UNKNOWN`分别表示公开语义、源码、目标二进制静态事实、推导、设计选择及未解释内容；静态事实不表示实际安装、命中、显示或失败恢复已观察。本文默认地址为VA，`RVA=VA-0x400000`。

## 1 本轮确定的公共边界

**R6接口归并（2026-11-01）**：[模型／角色／应用器](tx-04-17-model-role-applier-integration.md)明确三类操作从完整T对照最后原生A规划，不按Delta component直接RPC；先处理成员完整性、依赖与实例资格。create／remove／change报告实际前缀和待退休owner，由唯一主线程协调器提交A或撤销其当前差分资格，域handler不自行发Applied ACK。字段／关系／类别适配仍按本项UNKNOWN与18—20落实。

**R5回补（2026-11-01）**：[失败退休与菜单交还](tx-04-17-failure-retirement.md)已核正常Exit先重建Room、后清PlayerManager，再清HUD；Player helper需要当前有效Room，不把该次序机械倒排。Room重建先析构／free旧Room再分配新对象，不是半构造失败的通用回滚；ResetState不释放实体，官方ESSM还先访问有效保存容器。逻辑remove后记录待摘表／Free的实际前缀，重复请求不再重复原生Remove／Free；未知owner／异常保持隔离，不提前Off。

**R4回补（2026-11-01）**：[内嵌资源与在途寿命](tx-04-17-native-resource-lifetime.md)补物理Free、池reuse、Init／Morph和服装容器变化前的资源责任，核定代表对象正常析构及NPC／Pickup原位Morph资源重载。EL摘除、ClearReferences、ESSM Delete与Image强持有不替代实际使用退场；内嵌Sprite由原生owner清理，本方不手动Reset／GC。官方custom-render延后Entity／Animation上下文分支当前关闭，不增加虚假在途依赖；完整取消与异常退休交R5，静态事实不等于已实现。

**`DESIGN`：三个操作继续描述服务器目标的状态语义，由客户端按实际原生owner编排；不把网络操作直接等同于Spawn、Morph或Remove的一次调用。** 同一逻辑key可有不同原生实例／池use；同一实例可有不同形态和资源revision，也可进出多个membership。正式映射不依赖录制成功，原生指针只在主线程和当前代次中使用。

| 阶段 | 能说明什么 | 不能代替什么 |
| --- | --- | --- |
| 工厂取得／ctor | 已取得池槽的新use，或由相应owner构造了对象；官方扩展登记可已执行 | 派生Init、Room／PlayerManager成员、完整资源或服务器key最终提交 |
| Init返回 | 对应动态类型的初始化层正常完成，可能已有嵌套创建／Lua／扩展状态重建 | 外层Spawn／Morph／Player准备完成；基Init不覆盖所有派生层 |
| 原生成员发布 | 对象已进入某些原生列表或grid槽，其他原生代码可能已能看到 | 应用完成、可渲染、客户端输入绑定或Applied ACK |
| 外层操作完成 | 可以核对实际对象、成员、字段、关系、资源与所执行前缀 | 自动成功；回调可改参／短路、对象可被移除／复用、部分目标仍可不完整 |
| 逻辑remove | 停止该key的新使用和展示，原生对象可能已`Exists=false` | 全部摘表、引用清除、池槽可复用、析构或operator delete |
| 摘表／清引用／回收 | 对应owner的真实退休工作完成到所记录阶段 | 所有别名／Lua借用／在途绘制都结束；这些分别由R4／R5处理 |

沿用15的稳定key、不可变B→T重建及A→T规划，不增加wire opcode、通用函数RPC或客户端Authority时钟。服务器目标完整缺席与component.remove、局部枚举缺席不是同一语义；跨房membership退出也不是死亡。观察层的local/use/birth/revision不直接成为网络key，客户端实例换代也不伪造服务器出生历史。

## 2 公开接口与当前源码选型

本地资料根为`F:/projects_isaac/IsaacDocs/docs/`与`F:/projects_isaac/REPENTOGON-1.1.3/`。另参考[原版Entity接口](https://wofsauge.github.io/IsaacDocs/rep/Entity.html#remove)、[官方PlayerManager](https://repentogon.com/PlayerManager.html#removecoplayer)与[官方Morph回调](https://repentogon.com/enums/ModCallbacks.html#mc_pre_npc_morph)；网站不替代本地1.1.3／J273版本依据。

| 接口 | 已读事实／限制 | 本轮采用范围 |
| --- | --- | --- |
| `Game():Spawn(Type,Variant,Position,Velocity,Spawner,SubType,Seed)`／`Isaac.Spawn(...)` | 原版`Game.md:519—522`区分显式seed和随机seed；native `Game.zhl:29`另有第八个word，Lua七参不能直接套native函数指针。官方`MiscFixes.cpp:172—178`在PreEntitySpawn前后钳seed至至少1 | 客户端使用服务器指定的描述／seed，不能额外Random；原版Wiki零seed警告保留其适用语境，不宣称1.1.3没有修复。显式seed仍不保证类型、subtype、成员与后处理不变 |
| `MC_PRE_ENTITY_SPAWN(24)` | 原版回调文档`:609—629`只允许替换`{Type,Variant,SubType,Seed}`；Type=0及跨原生类别替换有崩溃警告，非table返回不是通用取消 | 不用false／Type0／空Effect替换来禁止所有连带生成；控制必须在具体生成owner执行前，按允许的原生调用和目标关系处理 |
| `Entity:Remove`／`Kill`／`Die` | `Entity.md:504—525`为不同操作；Remove无成功码。`MC_POST_ENTITY_REMOVE(67)`还覆盖切房卸载和结束游戏，不能把它当单一服务器remove事件 | 使用对应类型的非战斗退休路径，不用伤害、Kill或清房奖励替代状态删除；不能据回调次数推断物理销毁 |
| NPC／Pickup `Morph` | 原版分别写bool／void；当前`EntityNPC.zhl:2`和官方`CustomCallbacks.cpp:1192—1317`两者均以void包装，pre可改参或false短路，post在super后。NPC相同参数原生早退不保证外围回调无变化 | change保留逻辑key；核对最终形态／关系，不能拿NPC原生AL当官方稳定成功码。不能只按请求参数增加shape revision，不能绕过派生清理调用基Init |
| `PlayerManager.SpawnCoPlayer2`／`InitTwin` | `LuaPlayerManager.cpp:162—170`、`LuaEntityPlayer.cpp:179—201`直接调用native；InitTwin可nil且以既有Player为入口，公开文档另要求特殊角色补PostLevelInitStats | 不是从空PlayerManager构造首Player的通用替代，也不将双身体当第二参与者；所用角色及控制组依赖必须由20适配 |
| `PlayerManager.RemoveCoPlayer` | 官方文档限定额外Player并警告不要传main player；`LuaPlayerManager.cpp:349—356`转发`RemoveCoPlayer(player,false)`，无完整退休结果 | 只作已解释的额外Player退休候选，不用它删除主Player或把任意身体地址传入通用Remove |
| `Room:SpawnGridEntity`／`RemoveGridEntityImmediate` | `LuaRoom.cpp:33—64`检查创建index、显式或默认seed，PathTrail必填；1100可改参／取消或转desc重载，1192仅布局生成；原生槽发布早于Init | Grid独立adapter管理slot占用、旧对象、门别名、路径／碰撞缓存；不把Entity成员协议直接套Grid，不通过Destroy／奖励路径删除 |

### 2.1 必须保留的官方对象维护

`SOURCE`：`Patches/EntityManager.cpp:64—91,145—185`经masked本地指针通知New／Delete；`main_ex.lua:2191—2218`接到ESSM，`repentogon_extras/entity_save_state_manager.lua:157—177`在new时如已登记同ID会先delete旧数据，delete撤销valid记录并清各Mod的对应数据。这是当前Lua／ESSM生命周期，不是服务器身份分配，也不证明所有已有userdata能安全解引用。

`SOURCE`：`EntityPlus.cpp:71—99`在ctor后创建holder，基Init前reset旧扩展并按类型创建新扩展；其Remove清扩展Hook在`:102—105`只是注释代码，不能声称Remove就释放EntityPlus。析构Hook在`:108—113`释放holder。Pickup Morph进入派生Init时可能途经这些重建；同地址不保持旧扩展／借用资格。

`SOURCE`：`EntitySaveStateManagement.cpp:2680—2746`在GameStatePlayer Init、Player ctor／Init／析构分别处理hijack marker、默认restore和moving-box／familiar相关状态；不能跳过这些维护后以裸内存清零补齐。`GridEntityPlus.cpp:11—52`另用448项按grid index索引的副表，在Factory返回后及基Grid Init前重置；同格新实例和绕过这些入口的内联路径须明确副表重建。

`DESIGN`：官方内部登记／清理Lua与可执行玩法／持久回调分别分派；不全局清CallbackState，不用“禁Lua”抹掉必要ESSM维护。原生世界中的自有observer仍只读；第三方gameplay Mod不因支持这些官方结构而自动获得兼容声明。

## 3 工厂、发布、移除与Morph的静态边界

### 3.1 工厂取得和池复用

`J273_STATIC`：`EntityFactory::Create 0x6EC540`（RVA`0x2EC540`）使用预构造池，Type1路径记录`EntityFactory cannot spawn Entity_Player`并返回null；不能用通用Game Spawn创建首Player。以Type1000的Effect池为代表，`0x6ED079`检查的是`+0x170 _valid`，不是`+0x172 _exists`。全池无可用项时，第二个bool非零在`0x6ED09A/9E`走null；为零才进入覆盖选择，设置`removedByFactory +0x177`，`0x6ED17F`virtual Remove、`0x6ED190`virtual ClearReferences，再清该标志并返回同一槽。

ZHL将该bool命名force，但真实分支与“true强制覆盖”的字面理解相反。`Game::Spawn`在`0x6FA2D3—2E3`取第八word的低bit并传给Factory。**`DESIGN`：客户端受控创建选择不覆盖存活池项的分支；池不足明确失败／恢复，不能为新目标驱逐另一个仍存在的服务器key。** 这不是新增Lua参数；若使用native adapter，完整八参ABI及具体路径按绑定合同接入。其他类别只复用已经核定的共同池语义，不以Effect样本外推所有变体副作用。

`J273_STATIC`：另一个七参native桥`0x424340`固定给完整Spawn传第八word=0。它不能用来满足上述不覆盖选择；公开Lua七参与这个native桥也不因参数数量相同就自动具有完全相同ABI。具体包装和绑定必须保留完整原生合同，不能删掉栈word或凭参数名猜测行为。

### 3.2 Spawn、Init和成员发布

`J273_STATIC`：完整Spawn owner `0x6FA210`（RVA`0x2FA210`）先经PreEntitySpawn，再调用Factory`0x6FA2E3`、动态vtable Init`0x6FA38D`、分配当前Room实体index并在`0x6FA3B3`调用成员发布`0x413D00`。之后仍有spawner继承、flags／属性、引用、特殊helper和按条件执行的后续工作；Factory可null，外层正常返回仍须核对实际对象和目标。不能只拦最后Lua回调就宣称创建无副作用，也不能在Publish处提交世界。

主审复读`0x413D00`（RVA`0x013D00`）：`valid==0`才走main／persistent分类、render插入、部分计数和特定wisp成员建立；已valid分支`0x413D18`从updateEL删除匹配项，之后仍根据flags决定updateEL成员。尾部`0x413EAC`处理类型查询缓存，raw返回不作Entity指针或成功bool。重复Publish可能只是membership刷新，不能视作重复create；也不能盲目多调来“修复所有列表”。

**`DESIGN`：优先使用对应owner的受控构造／派生Init／必要成员维护，选择性保留Spawn所需后处理；未解释的整Spawn不整体放行。** 创建期间已有原生成员可见而尚无正式展示资格，客户端禁止相关世界绘制／玩法重入；不能假设对象直到本项目最后一步才进入引擎，也不为此另建第二套完整世界。

### 3.3 Remove与摘表不等于析构

`J273_STATIC`：基`Entity::Remove 0x6A8000`（RVA`0x2A8000`）在`0x6A800C`置exists=false，条件置dead，始终于`0x6A801D`进入remove回调helper，随后解除`+0x408`Lua registry引用并释放其包装。它没有按“已remove”整体早退，因此网络重复remove必须由本项目幂等处理，不能重复调用原生方法期待无副作用。基方法事实不自动覆盖Pickup等派生Remove。

主审复读基`ClearReferences 0x6A8060`（RVA`0x2A8060`）：清Parent／Child／Target／Spawner等注册引用及反向引用表；还在`0x6A8167`对`+0x310`关联对象调用virtual Remove（该字段完整语义仍UNKNOWN）。清引用可连带移除对象，不是纯指针赋零；此函数也不能代替清全部容器、缓存和Player roster。

`SOURCE`：官方`EntityManager::CommitRemovedEntities`（`EntityManager.cpp:94—145`）只在stagedRemove时运行，依次扫描update、render、wisp、main、persistent五类列表。owner列表的非Player先通知Delete；然后ClearReferences、valid=false；Player在wasValid时Free(1)，最后RemoveAt。其他EL明确不处理；`EntityList_EL.zhl:20—39`的RemoveAt仅搬移指针并减size，不是对象析构。整个链可能触发更多Remove或回调，不能把一次提交当全图引用／全部借用的闭合证明。

**`DESIGN`：选择显式的主线程退休阶段，不通过整条EntityList／Room Update完成摘表。** 复用已解释的官方提交工作，但先限定实际owner、Player roster、全部相关成员与副表、重入和连带remove。非Player池项变为可复用与物理析构分别管理；Player、Grid及直接分配的辅助对象由各自owner回收。不能对池槽delete，不能把Player Free与池退休统一成一种释放。

### 3.4 change与形态转换

`J273_STATIC`：NPC Morph `0x6BD0C0`（RVA`0x2BD0C0`）可在相同参数下早退；实际变化是同对象改类型／配置及姿态／属性相关重建，不走常规完整派生Init。Pickup Morph `0x6DFF30`（RVA`0x2DFF30`）可推进本对象RNG、保存部分值、virtual Init并恢复Index／空间与部分引用等，再继续后处理；它还清理注册反向引用并重接特定链。KeepSeed／IgnoreModifiers不是“保持整个对象所有状态”的开关，必须在整个Morph之后重新应用服务器最终字段与关系。

主审发现NPC Morph在`0x6BD24F`以Manager`+0x14`的PGD调用`0x904DE0`（RVA`0x504DE0`）。该helper有readonly条件，但在配置符合条件时于`0x904E77`递增或`0x904E8F`初始化PGD`+0xF64` bestiary表项，另有条件通知分支。因此Morph本体也可能修改访客进度，不仅外围Lua有风险；确切计数项命名不影响这一写入事实。所选Client change必须在这类进度消费者执行前隔离，不能先写后回滚dirty。

`DESIGN`：具名公开setter语义足够且没有对应玩法副作用时优先原位最终值应用，并维护必要缓存。Morph只在该动态类别的配置／资源／生命周期合同已解释时使用；跨C++类别或已知原位操作不能表达目标时采用本地实例替换。替换保持逻辑key、增加本地实例代次并重绑引用，对外仍是change；旧实例按原生owner退休，不能调用Kill，也不能直接写vptr改变C++类别。资源／内嵌Sprite的失效转R4，不能因key没变继续使用旧Layer／Lua借用。

## 4 create／remove／change的公共应用合同（DESIGN）

### 4.1 操作上下文和依赖

每次应用由主线程上下文记录session／scene／目标revision、逻辑key、当前实例代次、用途、外层owner及parent、执行阶段和已产生的原生副作用。用途区分构造、目标应用、退休、必要展示维护；不是让Client默认通行所有native的布尔开关。上下文覆盖完整外围链及嵌套调用，Host／Off透传，观察wrapper保持原链exactly-once；网络线程只有自有纯值队列。

先在纯值目标中形成计划，分类required／optional引用、循环引用、明确空值、原生实例需求和资源依赖。实例／本地key映射可先登记为待完成，以支持循环引用及嵌套归属；未完成映射不能被一般借用／渲染／输入使用。按实际依赖建立对象、应用具名字段、修复关系和成员，再维护缓存；Grid slot替换和多身体关系不能照搬一个固定全域顺序。

原生连带创建在产生之前判明：已在目标中的对象与服务器key唯一关联；原生内部辅助物随owner管理但不能把familiar／弹幕等玩法对象任意降格为显示辅助物；未知自主创建在入口停止相关路径。不能先运行全部Init效果、再按最终枚举删掉多余对象，不能再对连带对象执行一次独立create。

### 4.2 三类操作及提交

| 操作 | 主线程执行步骤 | 逻辑完成／幂等条件 |
| --- | --- | --- |
| create | 核对key与目标；准备资源／owner；受控原生取得或构造，登记新use／实例；派生Init及必要官方维护；建立真实成员、应用初值和引用；核对外层结果 | 同key同目标已提交不再Factory／Init／Spawn；地址被新use占用时旧映射撤销。同key发生部分成功时继续已解释的未完成前缀或进入恢复，不能重跑全部创建 |
| remove | 撤销key的新借用／显示；按目标依赖处理入／出引用；动态类别的非战斗Remove；在明确owner阶段提交摘表／清引用／副表；按实例类型延迟回收 | tombstone保留目标版本和退休进度；重复目标不再Remove／Lua通知。原生逻辑退出可早于内存释放，但必须已证明无可见／可模拟的残留成员；未解释的别名不是“延迟回收”可掩盖的问题 |
| change | 对照完整目标最终值；按类别选择原位setter、受控Morph或实例替换；撤销相关借用／资源资格；重建必要引用、成员和缓存；重新核对目标 | 保持服务器逻辑key；shape revision与本地实例代次分开；同revision冲突明确报错，旧revision不覆盖新目标。同值目标不靠重复调用Morph实现幂等 |

只有完整目标所需字段／引用／成员／资源和维护满足、owner正常完成且代次前后未变，才提交A=T及展示资格。收包、计划生成、原生指针非空、Init／Publish返回、音画事件水位、GPU完成分别记录，不互相代替。资源或成员不完整不得产生Applied ACK；21／22只消费真实提交结果。

### 4.3 失败和重入

失败结果至少保留目标、key、阶段、已完成的取得／Init／发布／摘除前缀、仍存对象／关系、错误原因及可恢复范围；这属于运行时语义，不要求本次保留artifact。无世界写入时可取消候选；已经写入时停显受影响世界，不假定回滚至旧A，保持Client隔离并按完整目标／新Baseline恢复。未知ABI、错误线程、对象代次变化、未分类连带生成、清理不确定或预算耗尽均停止相关新写入。

构造／Morph／Remove及官方Lua维护可重入；内层返回不能抢先提交外层，同址复用不能复活旧借用。对象地址、网络key、形态、资源和membership分别核对；callback返回false也不能假设此前完全无写入。客户端停止／断连发生在部分成功窗口时仍保持Client，退休至最后owner和持久消费者结束后才允许Off。完整异常清理与在途寿命由R4／R5落实。

## 5 与R1及后续任务的关系

R1已选已有Manager／Game上的自有准备编排；本轮进一步确定：通用EntityFactory不接受Player，菜单已有基础Room容器不等于首Player依赖全部有效，PlayerManager成员与Room成员也不是同一次发布。建立首Player须用专用owner，并在任何依赖首Player的Level／HUD后处理之前完成所需层次；不能因有`SpawnCoPlayer2`或`InitTwin`公开接口就绕过bootstrap依赖。

### 5.1 首Player、前后初始化和Room成员

| 工作 | 本轮补证与对R1的约束 |
| --- | --- |
| 专用初建owner`0x98F5C0`（RVA`0x58F5C0`） | R1已确认new`0x2EA0`→ctor`0x74DC70`→PlayerManager列表先发布→`0x98F6A7`virtual Init；本轮复读vtable`0xB1BDB4`的slot1值为Player Init`0x74F6A0`。对象与roster可早于Init完整返回存在，不能仅在最末绑定／清理时才承认其owner |
| Init参数和seed | 普通Isaac为Entity Type=1、Variant=0、角色SubType=0，参与者ordinal不是该SubType。`0x74F6F9—70C`显式检查seed为零并进入错误／INT3路径；不能把PreEntitySpawn的seed修复外推到直接Player Init。服务器目标必须满足所用原生前置条件，不为通过构造静默改服务器seed |
| PreLevelInit | `Entity_Player::InitPreLevelInitStats 0x7B2C90`（RVA`0x3B2C90`）prologue匹配官方ZHL。Player Init在`0x7518AD`检查Variant，Variant=0时`0x7518B5`调用Pre。普通Isaac仍执行角色config和共同统计／初值相关调用，另有seed效果／规则与特种分支；不是单纯填HUD字段，也不是任意可提前调用的空初始化。主审核对`0x7B387C/38BC`指向Game`+0x1BBD8`，即Seeds`+0x54`效果集合，其中33／46为seed效果，不是PGD成就编号 |
| PostLevelInit | `InitPostLevelInitStats 0x7B3980`（RVA`0x3B3980`）签名同样匹配。原版Start在`0x6F1B07`执行Level Init后，遍历Player列表副本，仅对Variant=0在`0x6F1B2B`调Post。Client不能先执行Pre／Post整链，再用服务器状态覆盖结果冒充无副作用；需要按实际采用角色解释必要工作及禁止消费者 |
| 直接Room依赖 | `0x98F6CD`读当前Game Room，`0x98F6D3`读Room`+0x1214`计数，`0x98F6E5`取EntityList，`0x98F6EB`发布；仅在Player valid=false时执行。此helper不传RoomDescriptor；这只解释其直接容器依赖，不能证明Init／Pre／Post间接callee无Room／descriptor读取，或经过Level Init后成员仍满足目标 |
| 共用与特种连带调用 | Player Init三处七参Spawn桥`0x751015/1168/129F`位于Variant=1分支，普通Variant0跳过。主审进一步核定：尾部`EvaluateItems 0x75D4B0`／`AddActiveCharge 0x7899A0`的`0x751A88/99`两处call由`0x751A7D`的角色SubType=0x12条件控制，普通Isaac0也跳过；`0x751A9F`→`0x84A7E0`为共用Player Init回调helper（id9）。这些仅证明具体分支，不能外推未审的其它helper、Pre／Post、官方Hook或角色没有间接生成 |
| 官方外围Pre／Post Hook | `CustomCallbacks.cpp:3809—3825`在Pre之前调1127，`:884—902`在Post之后调1042，均仍进入super；`ItemSpoofSystem.cpp:340`等还有相关包装。保持必要本地结构维护并隔离玩法／持久消费者，不能用一个callback取消整个准备链 |

`DESIGN`：R1的准备依赖现在明确区分“构造／基础Room容器与roster → Player Init及其Pre → 服务器目标驱动的Room／Level准备 → 所需Post层次与最终Player状态 → 引用／HUD关联”。这是责任顺序，不是允许Client原样调用原版Level Init的建议；具体Room结构分段仍按R1未决表补齐。未解释的间接依赖不填零、不用本地run补出。

### 5.2 合作选择数据与HUD关联

`J273_STATIC/SOURCE`：`0x98F76A`还会调用`0x994CD0`（RVA`0x594CD0`）。本次主审将官方`ASMPlayerManager.cpp:20—39`的`init_special_baby_selection`说明／签名与该函数`0x994FEA`唯一匹配：它从角色／baby配置和本地解锁状态准备合作选择数据。原始指令`0x994DF0—4E22`还用栈上传入seed做局部xorshift，`0x994E33—41`打乱候选，随后向PlayerManager`+0x90`拷最多五项并更新`+0xA4`，另建`+0x70`选择vector；`0x9950A1 ret 4`。反编译输出漏掉这段原始指令，不能据伪码“未见RNG”称它纯结构，也不能将局部by-value随机误称推进服务器共享流。

`DESIGN`：合作选择wheel不是Client Room构造或服务器Player选择依据。C/S的角色由服务器目标决定，不能由访客本地解锁筛选重选；确需本地界面时将其单独作为ClientLocal UI解释，不能为填充PlayerManager就整体执行这段选择逻辑。本项使R1对`0x994CD0`的未知从“完全不明的首建后处理”缩小为合作菜单数据准备，仍不宣称每个官方patch已经运行命中。

`J273_STATIC`：首建owner条件尾部`0x98F7A3`调用HUD helper`0x980130`（RVA`0x580130`），其中日志明确为`HUD::AssignPlayerHUDs()`；主审确认它清旧HUD关联、读PlayerManager roster／controller／parent／twin及Player索引，经`0x9911C0`选择席位后绑定HUD，包括特定双身体的额外槽。它与R1的`HUD::LoadGraphics 0x979E10`是不同职责，不是资源成功码，也不自动证明HUD与服务器控制组关系正确。R3／20按受支持控制组、身体与本地展示席位解释必要关联，不能把原生controller index当网络身份；helper内部的完整重入／派生显示闭包仍未完成。

本轮公共代表路径按职责选择：普通Effect覆盖池取得／派生Init／成员发布；NPC与Pickup分别覆盖不同Morph；普通Isaac首Player覆盖专用构造／roster与Room关系；单grid槽和Door覆盖slot／别名退休。它们是静态研究与后续17代表性实现的候选，不是已运行样本，也不外推全部原版内容。18／19／20继续完成Grid、非Player和Player逐类适配；17必须先补其实际使用的公共依赖，不能把全部缺口推到逐类任务。

### 5.3 R3维护／绘制对本轮合同的回补（2026-11-01）

[R3结果](tx-04-17-native-maintenance-rendering.md)确认：render_sort只改renderEL次序，不补成员或关系；空表直接调用不安全，共享scratch容量及渐进排序也有独立限制。create／change需先完成成员与目标字段／引用，再准备排序、空间和HUD缓存；形态或本地实例换代撤销对应缓存及内嵌借用，不能因服务器key稳定保留旧资格。GetRenderZ还读取原生Index等本地值，不能把服务器key直接填成native Index。

remove的受控提交发生于下一次sort／shadow／entity draw之前；ClearReferences／Player Free等仍按实际owner处理。已有绘制窗口及队列raw引用并不随renderEL摘除自动结束，R4负责在途使用，R5负责完整退休。原生Interpolate含位置／碰撞，不能为新实体“刷新显示位置”调用它；派生draw需要的最终值和本地缓存由R3合同供应。以上是静态维护反馈，没有证明全部派生Init／draw／失败回收已闭合。

## 6 仍未闭合的问题

| 问题 | 已缩小的范围／后续责任 |
| --- | --- |
| 所选派生Init与必要Spawn后处理 | 通用Factory／Publish和生命周期顺序已解释；具体Effect／NPC／Pickup的资源、规则、随机、连带生成和回调仍须按实际采用类别解释。没有统一“无玩法Spawn”公开接口 |
| R1首Player准备依赖 | 已定位Init内Pre、Level Init后Post、当前Room EntityList直接发布，以及合作选择数据／HUD关联的不同职责；剩余为Init／Pre／Post未审的间接Room／descriptor依赖、目标几何最低要求、所选角色／物品的连带关系与受控替代。普通路径跳过具体Spawn／特种尾部分支不等于整个闭包无副作用，仍无完整可直接调用Client bootstrap |
| 非战斗退休的完整引用范围 | CommitRemoved不是全部EL／Player roster／Grid door alias／查询缓存／Lua／在途绘制的总清理器；ClearReferences还能连带Remove。R2保留owner限制，R3／R4／R5及18—20按采用路径补齐 |
| 池预算和实例替换 | 不覆盖存活池项的分支已定位；已提交展示、待退休原生use、未发布对象及资源引用须合并计预算，不能因valid=false就认定所有旧借用已结束 |
| Morph持久／玩法消费者 | NPC的PGD bestiary写入点已定位，Pickup也有自身随机／规则后处理；需随实际adapter落实作用域及外围Hook分派，不以readonly或禁一个回调作完整保护 |
| 异常／OOM／取消与重建 | 记录实际前缀，不重复创建／Remove、不交还Off；尚未证明全部原生EH、成员摘除和官方副表可恢复，R4／R5继续收束 |
| 运行事实 | 安装链实际顺序、线程／context、资源显示、停止玩法后的维护、真实连带对象及全过程无持久副作用留完整实现后的统一E2E，本轮不运行任何分项试验 |

源码和静态范围明确不等于R2运行实现完成，也不等于R1完整初始化已闭合。后续以本表具体问题回补，不设置gate、哈希校验或额外分项验收。

## 7 主审取舍与本轮收束

主智能体独立回读Factory、完整Spawn、成员发布及其EL子helper、基Remove／ClearReferences与67回调helper、NPC／Pickup Morph、PGD bestiary写入、首Player专用owner／vtable／Pre和Post调用段、合作选择数据原始指令、HUD关联以及相关官方源码。保留子智能体关于阶段拆分和R1依赖的有效结论；没有将其未审的传递调用当作已闭合。

关键纠正采用原始指令：Publish已valid路径先从updateEL删除匹配项，末尾条件成员同属updateEL，不是effectEL；`0x994CD0`伪码漏出的局部shuffle已补原指令和源码签名依据；Player Init尾部两个物品相关call只在SubType=0x12，不能写成普通Isaac必经；Pre尾的33／46查询对象是SeedEffects，不能标为PGD成就表。NPC Morph的PGD写入为主审补出的具体隔离消费者，未以它概括全部Morph副作用。结论和未决项已回补R1、接口目录、更新／持久隔离及主流程；完成本轮统一人工文本审阅，不运行格式／链接校验器、程序、游戏或测试。
