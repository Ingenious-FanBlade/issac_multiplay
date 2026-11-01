# tx-04-17 ST2：Client 场景根与生命周期

## 2026-11-01 当前设计与补充研究

现行合同与执行顺序见[tx-04-17主流程](tx-04-17-workflow.md)。R1先查公开接口／源码后补J273初始化范围，R2继续对象生命周期并回补前项，R3回补Camera／Room／HUD显示前置；本轮[R4资源寿命](tx-04-17-native-resource-lifetime.md)补Player组合资源／服装容器、原生owner析构及在途draw依赖。各轮必要逆向分别由用户指定的一个GPT-6 Luna／max子智能体执行，主智能体独立回读审阅。[R2结果](tx-04-17-native-object-lifecycle.md)、下节R1事实及后续回补分别保留；R5已补[失败退休与菜单交还](tx-04-17-failure-retirement.md)，R6已形成[模型／角色／应用器归并](tx-04-17-model-role-applier-integration.md)，具体接线与原生UNKNOWN仍未实现，未修改代码、构建或运行测试。

Client复用正常启动已有的Manager／Game，在原生世界中构建Room、Camera、实体、Player和HUD，保留create／remove／change；纯值模型用于规划，不另建完整第二世界。

- 下文“完整世界需要独立Replica场景根”“Start不能用于建壳”等为旧方案设计选择，已由受控复用原生初始化替代；Start／Room setup具有随机、生成、回调和保存副作用的静态事实保持，不因此将整条链视作可直接调用。
- [R1原生初始化](tx-04-17-workflow.md#r1)本轮确定可复用范围和具体缺口：采用已有根上的自有准备编排，选用已解释的原生helper；不整体调用Start／Level Init／Room Init后再清空生成结果。构建前即考虑ST8进度隔离；未完成目标不开放世界展示，同seed不替代服务器状态。
- [R2操作生命周期](tx-04-17-native-object-lifecycle.md)已补key／实例／membership及原生摘除的公共静态合同，结合后续[R5失败与退休](tx-04-17-workflow.md#r5)形成安全清理、原生菜单交还与保护结束条件。MenuReady不再是整个Client生命周期的恒定前提，Exit(false)仍不是现成取消接口。
- 每个候选先查公开接口和官方源码，仅为仍缺少的具体原生合同补J273静态逆向。Room／Grid、Entity及Player全类别适配分别归18—20，17先落实公共环境和代表性路径。

<a id="r1-results"></a>

## R1 原生初始化范围、选型与未决项（2026-11-01）

### R1.1 证据和范围结论

目标是官方REPENTOGON 1.1.3／Repentance+ J273。`PUBLIC_DOC`为两套本地Wiki及[官方回调文档](https://repentogon.com/enums/ModCallbacks.html#mc_pre_new_room)，`SOURCE`为当前官方／工程源码，`J273_STATIC`为本次目标二进制静态事实，`DERIVED`为推导，`DESIGN`为选择，`UNKNOWN`为未解释内容。以下地址默认VA，`RVA=VA-0x400000`；不会把反编译器猜出的参数／返回值或语句标记直接当ABI／callsite。静态反编译没有运行游戏或执行被分析函数。

**`DESIGN`：选择“复用已存在根，自有编排受控setup子路径”。** 保留正常启动已经构造的Manager、Game、LuaEngine、PlayerManager、HUD和基础Room；不重新执行Game ctor或`0x6F0AF0`。独立复用具名HUD资源准备及满足前置条件后的Camera构造；Player／Room内容经受控原生生命周期和服务器目标应用建立。Start、Level Init、Room Init提供依赖顺序和具体原生工作索引，不能把整个函数作为安全建壳API，也不从函数中间地址直接跳入来绕过栈、SEH和局部对象。

与“受控调用完整Start链”相比，此选择避免把访客PGD readonly修改、ItemPool重新抽取、整层生成、初始Player效果、启动保存、Room模拟和game-start回调带入同一大调用。代价是必须明确Room几何／缓存的独立构建段及Player初始化适配；这些尚未闭合的点逐项保留在R1.7，不能把“选型明确”写成“已有可调用的完整Client世界构建器”。没有采用第二套Game／运行中世界、原版Online／NetStart或访客Continue。

### R1.2 公开接口与官方外围链

完整选型表见[接口目录R1补充](tx-04-17-public-interface-catalog.md#r1-init-interfaces)。关键结论如下：

- `SOURCE/PUBLIC_DOC`：1060／1200无取消合同；1200发生于Room完整初始化前。1192只取消该布局Grid生成点，不覆盖随机装饰、普通实体、门或caller其他工作。因此本轮没有选出“注册几个Lua回调后完整Start”的隔离方案。
- `SOURCE`：`RoomPlacement.cpp:172—289`的TryPlaceRoom修改双方门、当前Room对象及可见性；它不是纯descriptor写入，也不保证失败无前序写入。服务器拓扑不能经本地布局规则重新生成后冒充原值。
- `SOURCE`：`CustomCallbacks.cpp:1061—1074,3828—3846`、`LuaRoom.cpp:474—486`、`EntitySaveStateManagement.cpp:3481—3485`分别增加Lua、water／item-pool副表与ESSM检查。要保留必要官方结构维护，同时在Gameplay Lua之前分派；只Hook原生函数体内某条call不足以说明外围链已隔离。
- `SOURCE`：`VanillaTweaks.cpp:61—65`在Level Init后直接取PlayerManager首元素并重算StatHUD；空Player列表不能进入此尾部。`EntityManager.cpp:171—175`及ESSM`:2697—2703`的Player ctor登记／默认restore维护属于真实对象生命周期，不能因其也在Hook链就全部跳过。

### R1.3 Start拆分与确定复用的helper

| 工作／owner | J273事实及前置依赖 | 选定范围与替代责任 |
| --- | --- | --- |
| 正常菜单根 | 既有ST2：Manager Init`0x92BA20`创建／发布Game，ctor创建基础Room，随后`0x6F0AF0`装载配置。基础Room有EntityList等构造成员但Camera为空 | 复用已完成结构；不再new Game或重跑post-ctor来尝试补状态。当前Game／原生代次与稳定菜单必须仍是接管时对象 |
| Start`0x6F16D0`（RVA`0x2F16D0`）的规则／seed／进度前段 | 主审原始指令`0x6F170A`直接读全局g_Game；`0x6F174C/1755`写difficulty／challenge；`0x6F18F2 mov [esi+0x15],al`直接改Manager内PGD readonly。ZHL虽声明`__thiscall`，不能据此给任意Game实例调用；IDA拆出的26个参数不是新ABI合同 | 服务器模式／challenge／seed使用具名适配，不复用整段Start的本地裁决；readonly不是Client隔离位，禁止这处启动修改污染访客。Seeds含容器，不能复制裸内存 |
| HUD资源准备 | Start call`0x6F1772`→`HUD::LoadGraphics 0x979E10`（RVA`0x579E10`），prologue与HUD.zhl签名匹配。guard在HUD`+0x54E5`，加载字体、ANM、Image和Fortune Text Surface，`0x97AADE`置已加载标志 | 选为可独立复用helper，owner仍是Game内嵌HUD；在已解释主线程／context运行。该void及内部标志都不汇总逐资源成功，失败按资源层处理，不发布世界、不调用HUD玩法Update补齐 |
| Start随机／池初始化 | seed helper`0x6EBE10`在`0x6F1877/19FE`被调用，ItemPool在`0x6F1A0D`初始化 | 不用相同seed重建服务器抽取结果；必需本地构造默认值与服务器最终RNG／池状态分开。任何必需局部随机须先解释所属流和消费者，不能笼统放行全局流 |
| 首Player准备 | Start`0x6F1AE6`→`0x98F5C0`，ECX为Game内PlayerManager。空列表时`0x98F63E`分配`0x2EA0`、`0x98F652`调ctor`0x74DC70`，随后先发布PlayerManager列表，再于`0x98F6A7`virtual Init；条件下`0x98F6EB`加入当前Room EntityList。非空列表会复用首对象 | 需要有效原生Player／HUD上下文，但该整helper含Init和后续工作，不作为无副作用工厂直接调用。R2核定构造／Init／成员发布并接服务器Player key；R1保留其“列表先于Init、Room登记随后”的真实部分成功顺序，不能出错后重跑造成第二个身体 |
| 楼层／Room生成 | Start`0x6F1B07`→`Level::Init 0x73EB10`（RVA`0x33EB10`）；Room Init`0x7E8AD0`（RVA`0x3E8AD0`）含实体／布局／随机及Camera替换 | 服务器提供拓扑、descriptor/config与最终内容；保留已解释结构／资源工作，替代自主生成。具体Room依赖见下节，不整体执行后批量Remove |
| Start尾部 | 保存call`0x6F1B43`→`0x932160`；Room Update`0x6F1B9D`→`0x7F7F60`；PostGameStart`0x6F1BAE`→`0x84B660`。保存helper有条件分支，并非每次写盘 | 禁止Client沿这些位置保存／模拟／触发玩法启动Lua。确需官方本地UI收尾时单独保留其工作；例如LuaMenuManager.cpp:208—211在PostGameStart前清菜单input mask，不能因跳过整回调而遗留输入状态 |

### R1.4 Room／Camera与参数归属

`J273_STATIC`：Level Init的普通链在`0x73FA3F`调用房间加载owner`0x7385F0`，其`0x738ACE`设置ECX为当前Room、`0x738AD4`压入descriptor、`0x738AD5`压入所选config、`0x738AD8`调用Room Init。**ZHL中第一个`int param_1`在这条实际调用合同中是`RoomConfig_Room*`，不是mode或可填0的标志。** `0x738842`先取descriptor OverrideData，部分分支调用`0x8214C0`选择替代config，失败回落Data并写回OverrideData；另一路直接取Data。调用前`0x738ACA`还会改变AllowedDoors。因此不调用整个加载owner来重新选择服务器config／门关系。

Room Init在`0x7E8D9C/DA6`从所选config的`+0x46/+0x47`读宽高，各加2写Room`+0xC/+0x10`；`0x7E911A—9156`使用descriptor **Data**的Shape／Type计算门格位置并安装descriptor／roomType。宽高与Shape不一定来自同一配置，Data／OverrideData必须满足几何兼容，不可只替换其一。上述是原函数内的工作说明，不是允许从这些中间地址调用的连续函数。

| 原生段／helper | 核定范围与采用方式 |
| --- | --- |
| Room Init“几何之前的前缀” | `0x7E8B2A`已有helper调用；`0x7E8D90`调用`0x731740`清Game ItemPool位集，具体位集语义仍UNKNOWN；`0x7E8D97`调用`0x7E1550`，后者先处理EntityList，再重新编号保留列表、对448个grid槽有对象时virtual析构，随后清槽并修改Game／Room字段。故没有证据支持“运行前缀到首个生成点就只得到结构”；后续重建必须先解释退休责任 |
| 门格位置算子`0x7E8410`（RVA`0x3E8410`） | 主审复读全函数：仅slot／width／height／shape的算术和switch，没有calls或全局写。原生caller给ECX=slot、EDX=width，栈传height和shape，EAX返回index，caller在`0x7E913F`清栈8字节。不是普通thiscall／标准callee-clean fastcall；后续以明确adapter复用这个计算，输入来自有效config，结果与服务器门拓扑核对，不能以DoorSlot整数直接当grid index |
| 门对象`Room::MakeDoor 0x7E4DE0` | Init在`0x7EA122/166`等点调用；该helper可分配Door、替换旧grid对象，在`0x7E4F7F`调用全局RNG、virtual Init并登记door槽。它是有状态构建，不与纯坐标计算一同放行；R2／18明确种子、对象与slot／网络key归属，未知连带行为不能先运行再清除 |
| 边界墙构建`0x8140A0` | Init的真实callsite为`0x7EA19A`；该helper按shape逐格调`0x7E4B40`，后者缺格时创建Wall并在`0x7E4D9F`取全局RNG，特定房间条件还可创建Decoration／Gravity。不能将“边界墙”一律归为纯显示缓存；实际grid载体及随机归属由受控grid adapter解释 |
| 特殊分支与加载尾部 | Level Init特殊分支`0x73FA17/21`将last／current RoomIdx设为-7，经`0x73FA2B`加载房间并在`0x73FA36`调用Room Update；普通分支之后仍有`0x73FA50`等工作。Room Init的`0x7EA146`实际调特殊门helper`0x81C970`，不是边界墙helper。不能只拦普通分支尾部或只看Room Init返回 |

`J273_STATIC`：`Camera::constructor 0x91C730`（RVA`0x51C730`）接收本地有效Room，建立六组容量60／90／300／15／4／120的环形缓冲，保存Room关联，读取Game进入门与Room几何，最终在`0x91CD4E/57`写Room`+0x1204/+0x1208`。构造默认姿态允许短暂存在，但不能当作Authority最终相机；Camera日常更新的玩法／RNG分派仍归R3。不能对菜单空Room只分配Camera然后声称世界有效。

| 输入／状态 | 来源与处理 |
| --- | --- |
| run模式、difficulty、challenge、stage／stageType、维度、当前／前房和进入门 | 来自服务器目标及场景scope；不读取访客角色选择／挑战菜单替代，不进入Manager的_starting或Continue消费链 |
| RoomDescriptor／RoomConfig | 服务器提供逻辑房间身份、配置／资源描述、形状、门与布局关系，以及seed和保存内容；客户端解析为当前原生owner持有的有效对象。Data／OverrideData、ListIndex／GridIndex／维度及offset映射须一致，网络不携带本地指针 |
| Grid、Entity、Player与多身体 | 服务器决定存在性／关系／最终值；17提供有效容器和代表性初始化，18—20逐域适配。原生构造连带对象在入口分类并关联，不先生成未知玩法对象再删；Player不是参与者数的一比一镜像 |
| vector／set／map、ANM、Image、Camera缓冲、EntityManager／ESSM登记 | 由本地原生构造与官方必要维护建立；禁止memcpy整个Game／Room／descriptor／Player，禁止只填vptr或若干offset冒充构造 |
| 相机起始缓冲、HUD资源与临时显示缓存 | 客户端本地初始化值，待对应服务器目标／展示维护完成后才获得展示资格；不逆向上报为服务器状态或据此推进Authority tick |

### R1.5 准备顺序与调用后不变量（DESIGN）

1. 沿16稳定菜单连接入口，在`ClientManagerBoundary`短路原Manager原链之前取得当前scope和原生根；持久与玩法隔离从首次原生写入之前生效。连接、准备、应用、失败、退休均仍属Client；网断不自动Off。
2. 在自有纯值目标中解析服务器规则、拓扑、资源与依赖；准备本地owner和有界候选。没有满足所选native前置条件就报出具体缺项，不能补一个本地新局或访客Continue。
3. 复用已构造容器，建立规则／descriptor／几何及Player准备依赖。任何需要首Player的Level／HUD后处理必须等Player有效；实际Player与Room成员写入顺序按R2解释，不能把这一步理解成简单全域线性写内存。
4. 经已解释的Room结构／资源工作和Camera构造形成有效承载，受控create／remove／change应用服务器成员、最终字段与引用，维护官方必要副表。HUD资源helper可单独准备；不调用整条Room／Game Update求出缺失状态。
5. 只有当前Game／Room／scene与实例代次未变、几何／门／Camera／容器一致、所需Player／HUD依赖和资源／引用完整、没有未分类连带对象，才发布展示及原生提交结果；17准备完成与21／22的完整目标Applied ACK分开。

当前工程依据：`client.cpp:95—98,111—124`要求稳定菜单，`:171—178`的退休依赖“无自有图形／Lua引用”和MenuReady；`gen_binding.py:536`在Manager原链之前调用Client边界。R1复用其主线程泵和输入上行；准备世界以后必须扩展资源／owner退休条件，不能继续沿用16的“无世界资源”假设。这是接线设计，尚未修改相应代码。

### R1.6 部分失败与初始化期间持久隔离

`J273_STATIC`：Room Init在`0x7ED7B8`读取旧Camera，非空则`0x7ED7C3`调用`0x7E03F0`；主审确认该helper释放六组缓冲并在`0x7E0428`释放Camera对象。随后`0x7ED7CD`才分配新Camera，`0x7ED7E5`构造、`0x7ED7F7`回存Room`+0x11F8`。此窗口没有先清空旧字段，不能假定分配／构造异常保留旧镜头，不能重复free旧地址。回存后还有Backdrop、Lua GC等工作，不能在Camera指针非空时就确认外层Room owner完成。

| 停止／失败位置 | 必须执行的语义；不是本轮已实现行为 |
| --- | --- |
| 首次世界写入之前 | 可丢弃纯值／资源候选，按各自owner取消；保持16的角色／输入退休规则，不能让尚未处理的_starting恢复执行 |
| descriptor、Player列表或Room成员已修改 | 记录已完成前缀及实际owner；停止相关展示和新应用，不重跑整个helper、不声称恢复到旧A，不以返回false／nil／void判断“未改世界” |
| Camera旧对象已释放、新对象尚未发布 | 原生世界不可再绘；旧借用立即失效。异常／OOM的可恢复清理边界仍UNKNOWN，不能调用普通Exit(false)或交还Off继续原生模拟 |
| 部分资源／关系／官方副表建立后取消 | 在主线程按已解释owner清理，R4处理内嵌资源／Lua／在途绘制，R5完成安全退休；没有证据的回收不强行执行，保留Client隔离并明确错误 |
| 正常stop／窗口退出 | 关闭publication／输入与新对象使用，处理原生owner及最后持久消费者后才交还菜单／Off。Shutdown不保证还有下一轮Manager Update；不能依赖下一轮补清理 |

进度保护沿[ST8](tx-04-17-persistence-exit-isolation.md)的整链执行：Start的直接readonly写、Player／楼层初始化中的进度消费者和Lua在进入前处理；不进入`0x932160`启动保存，不借Restore读访客run。PGD／统计／平台通知、GameState原生与ESSM／VirtualRoom／ItemPool附加保存删除、Mod SaveData及退出／Shutdown／Manager析构版本同步均保持归属约束。只拦文件、事后恢复readonly／dirty或使用Exit(false)不构成隔离。正常本地设置和版本同步按其来源保留，前提是服务器状态未污染输入。

### R1.7 仍需解释的具体问题

| 问题 | 本次边界与下一步 |
| --- | --- |
| Room结构可调用切分 | 已选自有编排而非完整Room Init；几何、碰撞／路径、门、FX／Backdrop、装饰和实体生成的最小独立调用合同仍需逐段核定。没有安全连续段就保留具名helper方案，不跳入大函数中部执行 |
| 服务器楼层描述装入原生表 | descriptor、config、三维度offset、负索引特殊房和原生owner寿命需18的具体适配；TryPlaceRoom不是通用替代。17代表性场景也必须落实其所用最小映射，不把公共缺口全部交给18 |
| Player首建与特殊连带对象 | R2已补专用Init内Pre、Level后Post、Room EntityList直接发布、`0x994CD0`合作选择数据与HUD关联，见下节及[R2](tx-04-17-native-object-lifecycle.md#5-与r1及后续任务的关系)。仍缺未审间接Room／descriptor依赖、实际采用角色的连带对象与受控替代，不能以ctor／Init返回当完整Player就绪 |
| 初始化RNG／池与规则 | Start的共享／局部随机消费者不能笼统重放或禁用；所选结构／资源helper实际需要的流、初始化值和后续隔离需继续核定。服务器最终RNG／池状态由后续域应用处理，seed不等于完整状态 |
| 异常清理与安全菜单交还 | Camera替换和列表先发布证明部分成功真实存在；SEH清理、OOM、重建、官方副表和最后资源／持久owner由R4／R5补齐，尚无可调用的完整失败退出路径 |
| 实际可达与显示完整性 | Hook实际顺序、线程／context、安装命中、资源显示、所有caller副作用和失败恢复未经运行观察，留tx-04-17完整实现后的统一E2E；不在本研究运行probe或分项测试 |

### R1.8 主审取舍与文档收束

主智能体独立回读官方绑定／外围Hook及J273 Start、PlayerManager初建、Room Init参数／几何／清理段、门格算子、MakeDoor、边界墙helper、Camera构造和旧Camera释放、HUD资源准备。接受子智能体给出的“没有纯Room Init前缀”及自有编排方向；未接受将容器／成员布局段直接拷入的表述，明确改为原生构造、具名字段应用与计算adapter。

原始指令还纠正了伪码语句地址与含义：Door／Wall的RNG call分别为`0x7E4F7F/0x7E4D9F`，其后`0x7E4F84/0x7E4DA4`是seed回存；`0x7EA146`为特殊门helper，墙构建调用在`0x7EA19A`；Level特殊分支写的是last／current RoomIdx，不是dimension；Player分配／ctor实际call在`0x98F63E/0x98F652`，不是结果赋值指令。这些纠正以本节最终事实为准，没有修改历史ST证据或IDB。

本轮只交付设计文档中的源码依据、静态事实、方案取舍与未决问题；没有代码、绑定／Hook清单、游戏文件或IDB修改，没有构建／测试，不新增artifact包。下方历史研究按当时事实保留，尤其其独立场景／仅Sprite方案不重新成为现行选择。

### R1.9 R2本轮回补（2026-11-01）

以下是用户随后启动R2所得补证，不将R1先前列出的UNKNOWN追溯改成当时已知；完整证据及主审取舍见[R2对象生命周期](tx-04-17-native-object-lifecycle.md)。

| R1缺口 | 新增事实／仍需处理 |
| --- | --- |
| 首Player来源 | 通用EntityFactory明确拒绝Type1；`0x98F5C0`的virtual Init已对应`0x74F6A0`。PlayerManager roster先发布、Room成员条件后发布的部分成功事实保持，不能用Game Spawn／InitTwin替代首建 |
| Init前后层次 | Variant0的Player Init在`0x7518B5`调Pre`0x7B2C90`；原版Start在Level Init后、`0x6F1B2B`对Variant0调Post`0x7B3980`。Client仍采用自有编排，不能据该顺序整体放行Level Init或把Post任意提前 |
| 普通Isaac与连带调用 | Variant0跳过三处特定直接Spawn；尾部EvaluateItems／AddActiveCharge又受SubType=0x12限定，普通Isaac0跳过。共用Pre、Init回调与未审间接callee仍需解释，不能因此宣称普通首建无副作用；直接Player Init还有非零seed要求 |
| Room最低直接依赖 | `0x98F6CD—6EB`明确依赖当前Room计数与有效EntityList，不传descriptor；这不证明Init／Pre／Post及所有callee在菜单空Room可调用，也不证明经过房间重建后成员与目标正确 |
| `0x994CD0` | 官方co-op选择wheel源码签名在`0x994FEA`唯一匹配；原始指令补出局部seed shuffle及本地解锁／配置驱动的候选列表。它不是Room构造，不用其本地选择决定服务器角色；局部by-value RNG不等于Authority共享流 |
| HUD上下文 | `0x98F7A3`→`0x980130`为AssignPlayerHUDs，读roster／controller／parent／twin并重接席位，独立于LoadGraphics。资源已加载不表示关联正确；R3／20还需将服务器身体／控制组适配到本地展示槽 |
| 回收与持久 | Remove、摘表、池复用和物理析构已按R2分开，ClearReferences可连带Remove；NPC Morph已定位PGD bestiary写入并反馈ST8。实际owner、在途借用、部分失败及安全菜单交还仍由R4／R5补齐，不能提前Off |

本轮回补继续只改设计文档，没有实现受控原生调用、修改Hook／绑定或运行测试。R1完整可调用的Room结构准备与首Player／异常退休合同仍有具体未决项，保留R1.7及R2未决表，不把静态研究当作运行通过。

### R1.10 R3本轮回补（2026-11-01）

[R3维护与绘制](tx-04-17-native-maintenance-rendering.md)进一步明确“对象已构造”与“可进入原生draw”的差别。Camera需要关联Room／descriptor／边界／视口及六组有效历史缓冲；DoUpdate进入含共享RNG的链，不能用它补准备。窄Snap会重填历史并写Room显示offset，只适合首次定位或明确重置，尚不覆盖完整Active Camera模式。

Room还需阴影／光照／FX pass使用的集合与资源上下文，Entity需要排序key及实际派生draw读取的姿态／引用，HUD需要LoadGraphics以外的Player关联、heart／active／history／stat缓存。不得通过整条Room／Player Update生成这些值，也不把首帧GameRender当隐式初始化器。R1准备计划按所选代表场景补这些前置，空间与HUD缺值继续列为具体UNKNOWN，不升级为完整可调用bootstrap；内嵌资源／异常退休由R4／R5接续。本轮回补仍无代码或测试。

## 2026-10-31 历史研究正文

以下保留当时事实、纠错、UNKNOWN和设计推导；冲突设计按上文及主流程执行，静态结论不升级为运行结果，历史分工不成为新要求。


研究日期：2026-10-31，Asia/Singapore。状态：**ST2指定静态研究、与ST3两轮互核及主智能体独立审阅完成**。按任务指定 GPT-6 Luna/max 做静态阅读与必要目标二进制查询；Client世界壳仍未实现。

互核状态：已完成与 ST3 的两轮交流和交叉阅读。最终更正记录：将 `0x92E45A` 标为 `0x92E41D` guard 分支的 prelude early return，将 `0x92E80D` 标为 Start/menu 前置处理后的较晚退出；Camera 构造从误指的 `0x91EC20` 更正为 `0x91C730`，并核实 Room setup 的分配、传参及 `+0x11F8` 回存。主审独立回读后提出的角色输入语义与细节修订已合并。关联记录：[ST1 公共接口目录](tx-04-17-public-interface-catalog.md)、[ST3 更新隔离研究](tx-04-17-update-isolation.md)。

## 范围与证据标签

本报告只研究 Client 展示场景所需的根、初始化、准备、取消与退休。依据是仓库源码／本地 Wiki 与目标二进制的静态阅读；没有运行游戏、构建、测试、哈希或校验脚本，也没有修改运行代码或 IDB。

- `SOURCE`：可读的仓库源码、ZHL 声明或本地 Wiki 文档事实。
- `BINARY_STATIC`：当前 IDA 输入中的指令／反编译静态事实；不表示运行时命中或副作用必然发生。
- `DERIVED`：基于上述事实的控制流或风险推导。
- `DESIGN`：给 tx-04-16 的生命周期建议，不代表已有实现。
- `UNKNOWN`：当前证据未闭合的问题。

IDA 当前输入为 `C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe`，image base `0x400000`；IDA 数据库为同路径加 `.i64`。本次仅使用只读反汇编／反编译查询，未切换输入或控制调试器。

## 结论

当前证据没有证明存在可通过原生 `Game::Start`、Continue/Restore 或 Room 更新建立的“无本地玩法副作用 Client 世界壳”。正常启动已经有一个可复用的 `Manager`、`Game` 和空 `Room` 根，但 Game 构造本身会调用全局 RNG helper；这个 Room 的 Camera 指针为空，且没有证据表明它是已填充楼层、实体、HUD 与 Camera 的可显示世界。不能为了 Client 再 `new Game`，也不能把菜单 Sprite 展示称作 Room/HUD 世界。

目前源码和目标二进制能具体描述的最小候选限于菜单 UI/资源显示：复用正常启动的 `Manager`/`MenuManager`/Lua/图形路径，在 `MC_MAIN_MENU_RENDER` 中绘制 Client 自有屏幕 Sprite。它可承载加载状态、连接状态或调试性资源展示，但当前安装上的可用性与安全退休仍未知，不能满足 16 的 Room/Camera/HUD/Entity 世界展示目标。完整世界需要独立的 Replica 场景根与逐项封闭过的构造／更新／销毁适配；这些尚未证明可安全执行。

## 根对象与初始化顺序

### 正常启动已有根

`SOURCE`：REPENTOGON ZHL 声明 `Manager::Init` 为静态函数（`F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Manager.zhl:2`），`Game::constructor` 为 `__thiscall void`（`Game.zhl:2`），`Room::constructor` 为 `__thiscall Room*`（`Room.zhl:2`），`Camera::constructor(Room*)` 为 `__thiscall void`（`Camera.zhl:2`）。`Game::_room` 位于 `Game+0x18300`（`Game.zhl:272`）；`Room::_Camera` 位于 `Room+0x11F8`，`GetCamera()` 只返回该指针（`Room.zhl:157,189`）。

`BINARY_STATIC`：`Manager::Init` VA `0x92BA20` / RVA `0x52BA20` 在 VA `0x92BED0` 调 Game ctor `0x6ED4B0`，VA `0x92BEDC` 发布 `g_Game`，随后 VA `0x92BEE1` 调初始化 helper `0x6F0AF0`。因此正常菜单启动已持有 Manager 与 Game 根；新 Client 不需要、也不应额外创建 Game。

`BINARY_STATIC`：Game ctor `0x6ED4B0` 在 VA `0x6ED5FE` 调 `0x732D20`。该 helper 检查并可能销毁旧 `Game+0x18300` Room，在 VA `0x732D6D` 调分配器取得 `0x7898` 字节，并于 VA `0x732D81` 调 Room ctor `0x7DF8F0`；VA `0x732D88` 写回 Game 的 Room 指针，随后设置房间索引。Room ctor 的反编译在 VA `0x7E00E8` 将 `Room+0x11F8` 设为零。因此，Game ctor 确实创建一个基础 Room 对象，但没有 Camera 指针，也没有证据证明该 Room 已经承载楼层内容。

`BINARY_STATIC`：Game ctor 还在 VA `0x6EE128` 调全局 RNG helper `0x6EB4B0`，条件分支在 VA `0x6EE137` 再调一次，随后 VA `0x6EE14B` 调 `0x4257A0`。tx-04-08 ST6 对 `0x6EB4B0` 的既有研究将其识别为全局 RNG 状态读写／调用追溯相关 helper。这里能证明的是构造路径存在 RNG helper 调用点；是否进入条件分支取决于状态，不能据此声称每次构造都推进相同次数。

`BINARY_STATIC`：`0x6F0AF0` 在 ctor 后由 `Manager::Init` 调用；另一个调用点是 `ExecuteStartGame` 创建 Game 时。已有主审静态阅读发现该 helper 还装载 stages、curses、ambush、preload 并重置若干状态。其确切 ABI、每项副作用的条件和完整清单未由当前 ZHL 签名闭合，不能把它视作空的字段初始化。

`BINARY_STATIC`：Camera.zhl:2 的完整 SEH/prologue signature 在本目标中定位到 `Camera::constructor(Room*)` VA `0x91C730` / RVA `0x51C730`（完整签名定位由主审核定，本轮复核调用与回存），不是 `0x91EC20`。其唯一已见代码 xref 是 VA `0x7ED7E5`，位于 Room setup owner `0x7E8AD0`：VA `0x7ED7C8` 分配 `0x94` 字节，VA `0x7ED7DB` 压入 Room 指针 EDI，VA `0x7ED7DC` 把新对象设为 ECX，随后调用构造函数；VA `0x7ED7F7` 把返回值写到 `Room+0x11F8`。`0x7E8AD0` 的已见调用者为 `0x7385F0`；后者分别从 `0x739DE0`、`0x73EB10`、`0x7415E0` 的房间/楼层处理路径到达。故“基础 Room 的 Camera 为空”只描述 Game ctor 后的初始状态，正常 Room setup 之后会另行构造 Camera。

`BINARY_STATIC`：Camera ctor 初始化六组环形缓冲，保存传入的 Room 指针，并读取全局 Game 的当前房间/进入门及 Room 几何相关字段，最后写入 `Room+0x1204/+0x1208`。这是有效 Room setup 的一部分，不能对 Game ctor 留下的空 Room 直接调用；这条原生路径能构造 Camera，不等于可安全复用它来构造 Client 场景。

`BINARY_STATIC`：Room setup owner `0x7E8AD0` 自身在 VA `0x7ECA34`、条件 VA `0x7ECAA2` 与 `0x7ECAC7` 调用全局 RNG helper `0x6EB4B0`，且有实体创建调用路径。因此连同 Camera ctor 在内的原生 Room setup 不是“只补镜头字段”的安全准备函数。

### 最小依赖与准备顺序

| 顺序 | 依赖 | 当前证据与边界 |
| --- | --- | --- |
| 0 | 窗口、renderer 与当前图形 context | `UNKNOWN`：正常菜单已能进入 `Manager::Render`，但本报告没有证明目标 J273 下 context 的线程归属、加载旁路及在途提交完成点。资源服务必须在已确认的主线程/context 窗口调用。 |
| 1 | 既有 `Manager`、`Game`、`LuaEngine`、`MenuManager` | `SOURCE/BINARY_STATIC`：正常启动 `Manager::Init` 创建/发布 Game；菜单 callback 经现有 LuaEngine 与 MenuManager render owner。Client 应复用这些根，不再构造第二个 Game。 |
| 2 | 菜单 UI 候选：自有 Sprite、资源与 callback | `SOURCE`：`MC_MAIN_MENU_RENDER` 可由 MenuManager render 调用并显示 Sprite。候选只需要菜单屏幕坐标与资源；不需要 Room、实体或 HUD。Lua/GL 资源的最终退休仍 `UNKNOWN`。 |
| 3 | 世界 Room 的布局与成员 | `SOURCE`：Game 布局中嵌入 `PlayerManager +0x1BAA8`、HUD `+0x1D9AC`，Room 指针在 `+0x18300`（`Game.zhl:272,290,295`）。Room 内含 grid width/height `+0xC/+0x10`、grid entities `+0x24`、doors `+0x724`、Camera 指针 `+0x11F8`、EntityList `+0x1218`（`Room.zhl:155,182-195`）。这些是内存布局/成员位置，不证明有有效玩家、房间几何、门、实体、HUD 可绘状态或镜头。 |
| 4 | 有效当前 Room、Camera 与展示成员 | `BINARY_STATIC/DESIGN`：Camera 仅由 Room setup 路径在 Room 指针和布局/几何有效后创建；Start 则创建 Player 并生成楼层/房间。Client 世界若不能调用原生玩法 setup，就需要数据驱动的独立 Room/Camera/render adapter，并逐个证明成员和 draw-state 来源。不能把 `Game` 内嵌字段或基础 Room 分配等同于世界场景准备完成。 |

`DERIVED`：目前能从源码与二进制具体描述的最小 prepare 顺序是“复用正常启动根 → 确认菜单 callback 的主线程图形窗口 → 准备自有 Sprite → 仅在 MenuManager callback 绘制”。世界场景还需要有效布局、Room 成员、Camera 几何以及实体/HUD 的显示输入；现有原生 Start 会提供其中一部分，但同时进入 RNG/玩法链，不能直接拿来填壳。

### 原生启动与恢复不是场景构造接口

`SOURCE`：`Manager::Update(bool)` 是 `Manager.zhl:5` 声明；当前 `hooks.json:18` 将目标 RVA `0x52E3E0` 配为 stdcall hook。`Manager::ExecuteStartGame()` 是 `Manager.zhl:46` 的 `__thiscall`。`Game::Start` 是 `Game.zhl:102` 的 `__thiscall void(int playerType,int challenge,Seeds seeds,unsigned int difficulty)`；`RestoreState` 是 `Game.zhl:11` 的 `__thiscall void(GameState*,bool)`。

`BINARY_STATIC`：`Manager::Update` VA `0x92E3E0` / RVA `0x52E3E0` 在状态 switch 前于 VA `0x92E46E` 调 `ExecuteStartGame` VA `0x932A20` / RVA `0x532A20`。新局分支仅在全局 Game 指针为空时，才于 VA `0x932ACF` 调 Game ctor `0x6ED4B0`，随后 VA `0x932AE0` 调 `0x6F0AF0`；已有 Game 时复用已有对象。VA `0x933007` 调 `Game::Start` VA `0x6F16D0` / RVA `0x2F16D0`。Continue 分支在 VA `0x932E7E` 调 `Game::RestoreState` VA `0x6F43A0` / RVA `0x2F43A0`。

`BINARY_STATIC`：Start 路径 VA `0x6F1877` 与 `0x6F19FE` 调 seed helper `0x6EBE10`，VA `0x6F1A0D` 初始化 ItemPool，VA `0x6F1AE6` 创建 Player，VA `0x6F1B07` 调楼层／房间生成 helper `0x73EB10`，VA `0x6F1B9D` 更新 Room `0x7F7F60`，VA `0x6F1BAE` 调 `PostGameStart` `0x84B660`（state=0）。另在 VA `0x6F1B43` 调 `0x932160`；该 helper 的正常条件分支包含 GameState/PGD 保存 helper 调用，早退条件存在，不能说每次都写盘。对 `0x6EBE10` 的本轮反汇编可见它在 VA `0x6EBE61` 写入 `this+8` 的 RNG 状态，因此该 seed helper 会推进相关 RNG 状态。

`BINARY_STATIC`：Continue 在 Restore 后还执行 PlayerManager／PostLevelInit 后处理，VA `0x932EAA` 更新 Room，并在 VA `0x932EDF` 调 `PostGameStart`（state=1）。既有 `docs/research/tx-04-08-st6-st8-static-confirmation.md:85-89` 将 Restore 继续链定位为 GameRestore `0x6F43A0` → ManagerRestore `0x993A90` → LevelRestore → PostLevelInit `0x993FA0`。它是恢复玩法状态的路径，不是空显示壳。

`DERIVED`：即便只需要画面，调用 Start/Restore 也会带入 seed/RNG、ItemPool、Player、楼层/房间构造、Room 更新及 Lua 回调；Continue 还会进入恢复与 PostLevelInit。`Manager::Update` 的 Client 逻辑必须在 `ExecuteStartGame` 之前截断，不能只屏蔽后来某个普通 Player tick。

## 渲染与可行候选

`BINARY_STATIC`：`Manager::Render` VA `0x92EC40` / RVA `0x52EC40` 是独立于 Update 的路径。其状态分派在 VA `0x92EE08`：菜单态走菜单 render helper `0x9623C0`，Game 态走 `Game::Render` VA `0x6F7A80` / RVA `0x2F7A80`。Game::Render 内 VA `0x6F7E02` 调 `MC_POST_RENDER` callback `0x849DD0`。因此 `MC_POST_RENDER` 不等于菜单展示钩子；保留 Game Render 会进入 Game/Room/Entity/HUD 与 Lua 消费链，不能作为无玩法的通用展示边界。

`SOURCE`：官方 REPENTOGON 将 `MC_MAIN_MENU_RENDER` 定义为 callback id 1023（`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/ModCallbacks.md:1298-1303`）。`F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp:1528-1558` 的 `RunMainMenuRenderCallback(MenuManager*)` 从 MenuManager render 路径触发该 callback。官方脚本 `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/resources/scripts/repentogon_extras/bestiary_menu.lua:49-117` 在此回调中加载 ANM2 并通过 Sprite 绘制菜单内容。原版本地 Wiki 的 `Sprite:Load`/`Sprite:Render` 例子见 `F:/projects_isaac/IsaacDocs/docs/Sprite.md:157-184,219-229`。

`DESIGN`：第一阶段只把菜单态屏幕 Sprite 作为连接／准备状态的可见 UI 候选：复用现存 Lua 与菜单 render owner，自有 Sprite 与回调按 Client scene generation 绑定；不创建 Game、Room、Player 或 Camera，不调用游戏更新。若需要等待期持续显示，可把状态画在菜单 callback 中。它不是世界 replica，不能据此宣称 Client 已有可玩/可见房间。

`UNKNOWN`：当前加载安装是否启用对应 hook、菜单回调在目标 J273 构建的命中与排序、Lua 与图形线程/context 是否满足资源加载和绘制要求、所有屏幕 Sprite 所需资源缓存生命周期，均未由静态源码证明为运行时事实。

## 取消与退休边界

`BINARY_STATIC`：`Game::Exit(bool)` 的 ZHL ABI 是 `__stdcall`（`Game.zhl:135`），目标 VA `0x6F6270` / RVA `0x2F6270`。在 active 分支，VA `0x6F6363` 检查 PGD `+0` / `+0xF8C` 标志，VA `0x6F639A` 可调 cloud save `0x902F00`，或 VA `0x6F63A3` 调 local save `0x903460`；这些分支先于 VA `0x6F63B5` 的 `ShouldSave` 检查。`ShouldSave=false` 走 VA `0x6F63BE` → `0x9324A0`，而 true 走 `0x6F63B7` → `0x932160`。VA `0x6F647B` 还清 Manager `+0x15`（即 PGD `+1`）read-only 标志。保存分支依赖 active/PGD flags，不能声称每次 Exit 都会写入；但 `Exit(false)` 也不能用来保证不写访客进度。

`DERIVED`：不应通过 `Game::Exit(false)` 来取消 Client 场景或建立隔离；它属于 Game/PGD 生命周期，某些条件下在 ShouldSave 分支判定前已进入云/本地保存路径，并且会改只读标志。

`DESIGN`：准备状态先留在 Client 隔离态；候选资源未发布时可直接作废候选；已发布时先停止输入与状态发布，再在主线程阻止新 render callback、等待已进入 callback 的工作退出，最后释放自有 Lua/图形引用。只有自有场景没有残留引用后才能转 `Off`。Client update 的早退必须覆盖 `Preparing`、`Displaying` 和 `Retiring`；不能先切 `Off` 再期待原生菜单/Start 行为安全接管。

`UNKNOWN`：本次没有证明 Sprite Reset、Lua nil/GC 或 callback remove 与原生／GL cache 最终释放的时序，也没有证明在途批次结束点及 Shutdown 的最后可控 owner。`docs/design/tx-04-17/tx-04-17-public-interface-catalog.md` 的 ST6/ST7 项目已把线程/context、资源引用和 lua_close 生命周期列为补证项。若进程即将走 Main/Shutdown，退休动作必须在 Game/Manager/图形 owner 被销毁前完成；Shutdown 不再保证存在下一次 Manager Update。

## 给 tx-04-16 的角色与切换反馈

`SOURCE`：当前 `mod/src/core/session.h:49` 的 `SessionMode` 仅有 `None/Solo/Host`，没有 Client role。`mod/src/game/lifecycle.cpp:186-205` 的 `NoteLoadingOwnerEntered` 只记录 Host/Warmup 首次菜单启动资格；`:252` 的 `StartGameRequested` 读取 Manager `_starting` 请求。`mod/src/ops/command.cpp:559-606` 的 `RestoreScope` 处理现有 Host Restore 观察/退休范围，没有 Client 模式。

`SOURCE`：当前 hook 清单 `mod/binding/hooks.json:18` 把 `manager_update` 的优先级设为 `-10000`。结合当前 hook 安装顺序研究，低优先级 wrapper 位于已知链外层；同优先级顺序与未知更外层 wrapper 仍不是稳定契约。

`BINARY_STATIC`：Manager::Update VA `0x92E3E0` 的 `0x92E41D` 条件分支可直接走 `0x92E45A retn 4`，它位于主 fallthrough 的 `0x92E46E call ExecuteStartGame` 之前。主 fallthrough 会执行 Start/menu 前置处理，之后的其它分支可在 `0x92E80D retn 4` 退出。不能把某个正常 early-return 当成 Client 隔离入口；外层必须先判定 Client，避免有的路径早退、有的路径已进入 Start/menu 后再返回。

`DESIGN`：新增 Client 生命周期应在 Manager 消费 `_starting` 前完成角色切换。建议最少区分 `Connecting/AwaitingScene/Preparing/Displaying/Retiring`，网络 worker 只向有界不可变队列交付数据，游戏对象工作由主线程 client adapter 消费。Client update 边界处理消息、代次/取消与退休后直接返回，不调用原生 `Manager::Update`；Client 可上行本地用户的 input intent，但不执行本地 Authority tick 或原生 input apply，也不把资源 ready 记为 world/applied ACK。Host/Solo/Off 保持各自正常的本地 Start 语义；Client 才禁止原生 Start/Restore 与本地 Authority 模拟。

`DERIVED`：因为 `ExecuteStartGame` 在 Manager 状态 switch 前被调用，若 Client 等到 `Game::Start` hook 或 Manager state 改变后才切角色，已经过晚。禁用整个原生 Manager Update 可能让菜单动态状态冻结；可保留独立菜单渲染，但菜单 Update 的完整玩法／状态副作用尚未证明安全。

`UNKNOWN`：Client prepare 的正式触发来源与跨线程队列契约、同址/同优先级 Hook 实际顺序、Client 模式与 Host loading ownership 的互斥状态字段，需 tx-04-16 在既有 Session/Hook 架构中明确。世界场景需独立限定每项构造和展示调用的写入闭包；ST3 指出的外层 Camera 路径 `0x91E5E0`、`0x91D840` 内部条件调用 `0x6EB4B0`（callsite VA `0x91E65D`、`0x91E6C1`、`0x91E25B`、`0x91E2CA`）也说明 Camera 更新不能原样保留为无 RNG 的 presentation tick。只有条件成立时才发生对应 helper 调用。

## 尚未闭合的问题清单

- `UNKNOWN`：J273 下有无可安全创建、更新与退休的独立 Replica Root；当前已证实路径不够支持房间世界。
- `UNKNOWN`：MenuManager/Sprite 的资源加载、取消、最终缓存释放与 Shutdown 顺序；菜单显示只证明静态候选，不证明目标运行时可用。
- `UNKNOWN`：Room 填充、Camera 所有权、HUD、EntityList、动画与渲染闭包的最小安全子集；Game ctor 的空 Room 不是充分条件。
- `UNKNOWN`：Client 接管时实际 hook 链、主线程队列与正在运行的原生启动请求能否原子隔离；需要在实现设计中保证在 `ExecuteStartGame` 入口前切 Client 并维持至完全退休。

这些 UNKNOWN 是实际缺口；本报告给出的菜单 Sprite 是有限展示候选，不把它包装成已经证明安全的世界展示实现。
