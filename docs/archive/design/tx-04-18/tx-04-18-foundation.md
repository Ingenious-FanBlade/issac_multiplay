# tx-04-18 I01—I06：领域目标、计划与 descriptor 承载

> 2026-11-02后续：用户随后指定I07—I12，现已接入双配置/几何/进度/背景/持续环境/专用结构，见[Room应用说明](tx-04-18-room-application.md)。该轮将Room来源extension/RoomPresentation升为2、领域能力改为room-grid-target-v2，解除下文当时记录的双配置/零水量等代表限制。下文保留I01—I06当时事实和构建结果；完整Grid/地图等剩余任务与I29实机仍未完成。

2026-11-02。实现范围为 I01—I06；I07—I30、G01—G27 的原生适配和 I29 统一实机验收仍按完整计划推进。本文为功能说明，不是测试 artifact。静态结论与运行观察分别记录。

## 实现前失败模型

开始编写本轮代码前固定以下失败语义，后续 E01/E02/E03/E05/E11/E13/E14/E17 使用这些场景；本轮不提前运行测试，不添加单元测试。

| 输入或中断 | 处理 |
| --- | --- |
| Partial、Unknown、缺 required 字段、错误版本、混合采样或不完整成员 | 领域解析返回带字段/Key 的错误；不补零，不产生删除 |
| 同版本不同内容、旧 revision、重复目标、T 回到旧值 | 冲突/旧值拒绝；相同目标幂等；差分从真正提交 A 生成 |
| 同 Key 改变构造类型/SpawnSeed、同格新 Key、实例修订倒退 | 规划 replace；同格先退旧主体；倒退或未升修订的构造冲突拒绝；Variant/VariableSeed按同实例状态处理（I29修正，见下文） |
| 入口象限/GridIndex/SafeGridIndex 混用，L 缺口、重复映射、多维同索引 | 各字段分开；只安装来源明确的占用象限；维度是映射的一部分 |
| -19、-100/-101、NO_ROOM_IDX、负槽越界或把负 ListIndex 当数组下标 | Online 拒绝；路由独立枚举；离图按源槽有界寻址，保留 descriptor 原值 |
| 引用未解析、门别名与 Grid 主体矛盾、非法路径/非有限值/超预算 | 写前拒绝；资源和引用依赖进入计划，不创建替代对象 |
| 写前取消/新目标替换，写后取消或失败 | 写前可保留 A；写后必须撤显示/提交资格并退休，不用 A 的值副本冒充回滚 |
| 缺 I07—I24 的 adapter | 解析/规划可完成，但不能声明对应 native applier 或提交完整 Room/Grid |
| 退休或重建 | 恢复本 owner 修改的 descriptor/offset，旧代次不再连接；未知原生异常保留隔离 |

## 接口与实际范围

| 任务 | 实现位置与交付 |
| --- | --- |
| I01 | `mod/tools/gen_room_grid_catalog.py` 从11/12实际目录、共享资源目录和15归属规则逐原始行生成[应用目录](room-grid-application.json)：774条成员行（Grid 119、Room/Level 400、共享资源255）、106条回调、1项公开缺口及7项结构语义，保留重载/别名、来源/版本、参数、状态/操作归属、承载、依赖、缺失处理、实现及E01—E18索引；同一数据嵌入正式 `isaac_cs_replica` |
| I02 | 正式 `DescriptorPlacement` 主线程来源复制当前存储槽、原始Grid/Safe/List/Dimension、实际offset占用；room/level采集传播失败。15投影保留`construction_revision`。Room/Grid的`application_source.version=1`声明版本和required合同；不依赖录制或测试Lua |
| I03 | `replica/room_grid_target.h/.cpp`：有界、不可变、自有DOM的Room/Grid/Desc/Layout/Door/Environment目标；保持完整源模型coverage，类型、版本、引用、门别名、三处成员视图、路径及几何上界在写前解释；专用资源/字段仍在自有DOM中，由后续adapter具名消费 |
| I04 | 下节J273必要静态研究及主审；复用17原生构建、用途/线程、资源和退休合同；未采用的Grid工厂/Init/Remove等在I15之后实施时继续补其实际路径 |
| I05 | `replica/room_grid_plan.h/.cpp`：从真正提交A规划Keep/Change/Rebuild、create/change/replace/remove、引用、邻接、资源、Player依赖及提交。换槽/互换先退全部旧实例再创建。`ClientScene`持有一个T；写前可替换纯值计划，写后走原owner退休，退休后重新从无A规划 |
| I06 | `game/client_native_world.*`：当前入口与GridIndex分开；普通507槽和19个允许的离图槽有界安装；实际占用象限按维度写offset，LTL洞口不写；当前维度缓存指针一起安装。所有本owner改写的descriptor标量、507个offset及缓存指针由退休恢复；原生容器不复制、不析构descriptor，Online槽-19不读写 |

解析入口为 `replica::ParseRoomGridTarget(formal_model)`，返回 `shared_ptr<const RoomGridTarget>`；规划入口为 `PlanRoomGridTarget(T,A)`。两者不包含SDK/Lua/地址或线程借用，可由21继续复用。`RoomGridTarget::GridState`在自有DOM中定位派生接口/资源子树，没有从原始外部DOM借用的指针。

`DescriptorState`保留当前/普通列表/离图记录到完整源子树的定位；配置查找键相同但Spawns/版本/内容改变时仍规划重建。普通列表的size/list_position/物理storage一致，离图完整枚举19个允许槽，三处Grid成员视图及不可用计数一致后才允许规划删除。SDK字段宽度分别处理：RoomConfig Variant为uint32，Grid Desc SpawnCount为int32；descriptor三项seed的有符号Lua视图转为等价32位seed值，不误拒绝高位为1的原值。

预算：最多526个descriptor、456个唯一Grid（448格数组加8门槽的联合上限）、4096个引用、8192个步骤和1,048,576条依赖边；调用方只能收紧。完整模型沿用15的32 MiB输入预算。解析保留一份源DOM，场景原有DTO与已提交DTO继续各有owner；计划只共享T/A，提交后释放计划对旧A的引用，不积压历史目标。Grid派生缓存与渲染姿态没有因为纯值解析成功而获得原生就绪资格。

能力增加 `supported: room-grid-target-v1` 及 `model_categories.room_grid_target`。ICSR线格式/15模型版本仍为1，观测envelope仍为2；新增必需语义由独立的source extension v1显式管理，缺该合同或未知required名称会拒绝。原有严格类别协商会拒绝不匹配的旧能力；没有静默放宽17代表profile。`native_appliers`仍不声称18完整应用，领域合同明确`native_apply=false`。

当前`ClientScene`已接相同解析器/计划和I06承载，但I07双配置和I14—I25 Grid执行器仍待实现，因此不同Data/Override的原生应用、非空Grid或搬起描述会在写前明确拒绝。17实体/展示的代表profile和Lost地图限制保留；I13负责解除地图限制。I06承载支持的索引/维度不等于所有房形、专用背景和全部Grid已经具备端到端展示能力。

I02本轮实际补缺是descriptor实际存储/占用、配置来源合同和构建修订的正式传递。17已有`RoomPresentation`继续提供采用的最终背景构建流、环境缓存与资源姿态。`TimerPlate=nil`沿用正式`read.Value`的显式Null，不伪造空Sprite；`GetRenderPosition`继续用栈Vector的现有来源。公开API没有桥父指针/私有邻居容器；当前不执行MakeBridge/邻居维护，这些具体采用需求在I19/I20完成来源或已解释的展示派生之前仍为Unavailable，目录明确保留责任。

## I04：资料顺序、静态依据与主审

先读取本地 `IsaacDocs/docs/RoomDescriptor.md`、`enums/GridRooms.md`、`enums/RoomShape.md`，以及REPENTOGON本地`RoomDescriptor.md`、`RoomConfig.md`。再阅读目标SDK `Game.zhl`、`RoomConfig.zhl`、`RoomConfigRoom.zhl`、`LuaCore.h`及官方 `LuaInterfaces/Room/RoomPlacement.cpp`、`LuaRoomDescriptor.cpp`、`LuaRoomConfig.cpp`、`VirtualRoomConfig/VirtualRoomSetManager.cpp`。公开接口已解释的语义不另做全引擎逆向。

所需的未暴露寻址由一个GPT-6 Luna／max子智能体只读分析J273 IDB，主智能体独立复读以下伪码、原始指令和常量。未执行目标函数、未改IDB、未启动游戏、未校验哈希。下列VA的imagebase为`0x400000`，RVA在表中单列。

| 路径 | VA / RVA与主审直接核对 | 采用方式、副作用和失败责任 |
| --- | --- | --- |
| 当前维度offset表 | `0x606D10 / 0x206D10`：负维度读`[ecx+182C8h]`；非负维度`imul eax,2A4h; add eax,17ADCh; add eax,ecx`，`retn 4` | 每维169个DWORD；helper对维度不作上界保护。Client直接用已核0..2维数组，安装+0x182C8缓存，不调用未初始化的负维度别名 |
| 当前descriptor Getter | `0x73AF00 / 0x33AF00`：依次push`[ecx+1830C]`、`[ecx+18304]`，call`0x73AD10` | 当前索引是入口象限；不可替换为GridIndex或SafeGridIndex |
| GetRoomByIdx | `0x73AD10 / 0x33AD10`：先TLS static guard；`idx<0`返回`negativeGridRooms[-idx-1]`；普通表值仅检查`<=526`，按`0xB8`步长寻址，`retn 8` | 首次调用会构造静态fallback、分配容器/保存缓冲并注册atexit；失败返回静态空descriptor而非nullptr。-100/-101另走递归/扫描路由。Client新寻址不调用此API，也不写fallback |
| 普通放置的映射 | `0x7332F0 / 0x3332F0`；主审`0x733547 call 606D10`、`0x73354C mov ecx,[ebx+8]`、`0x733552 mov [eax+esi*4],ecx`，末尾增加nbRooms | 每个实际占用象限映射到同一ListIndex。原生错误坐标分支报错后仍可能以-1写表，故Client不调用整段place_room；自己核验来源的占用和边界 |
| 维度缓存更新 | `0x739F02 / 0x339F02`、`0x739FD8 / 0x339FD8`写`[ebx+182C8]`，值为`this+0x17ADC+dim*0x2A4` | 主审复读ChangeRoom此局部指令；没有调用整段含保存/过门副作用的ChangeRoom。只等价安装该指针及已采标量；退休恢复原值 |
| 形状占用 | `0x987410 / 0x587410`，`0x9874B5 lea eax,off_BD01B0[edi*4]` | 不调用带TLS初始化的helper；静态常量核定后用纯值表作写前核验，实际映射仍来自正式Authority来源 |

主审直接读取`0xBD01B0`的13个DWORD表项：shape0空；1/2/3→`0xBECB04`，4/5→`0xBECB34`，6/7→`0xBECB4C`，8→`0xBECABC`，9→`0xBECAE4`，10→`0xBECA94`，11→`0xBECB64`，12→`0xBECB14`。复读初始化赋值与`B40D80=[0,0,1,0]`、`B40EA0=[0,0,0,1]`、`B40EB0=[1,0,0,1]`、`B40F10=[0,1,1,1]`后，实际占用mask为 `1,1,1,5,5,3,3,15,14,13,11,7`。LTL缺左上，LTR缺右上，LBL缺左下，LBR缺右下；代码位于`replica/room_geometry.h`。

研究中的纠错事实：Luna补充表曾将shape4/6的行映射错位并报告与Wiki冲突；主审直接读取原始表后确认4/5为竖双格、6/7为横双格，与Wiki一致，未把该错误推导写入代码。Luna早期建议把离图ListIndex限制为507..526；主审以12的Genesis真实记录（room=-12/Grid=-1/List=-12）纠正：reset默认值不是活动对象硬契约。实现以来源槽寻址，原样保留负ListIndex/原始Grid/Safe。

SDK明确`_gridRooms[507]@+0x14`、`_negativeGridRooms[20]@+0x16C7C`、`_roomOffset[507]@+0x17ADC`。离图源槽`-n`映射第二数组`n-1`，不受普通维表影响；-19单独排除。`reset_room_list(false)`保留-20的特殊寿命仍由原生owner保持，本实现不调用该reset，也不重构或复制descriptor中的原生容器。

新来源`DescriptorPlacement`只在现有主线程采集窗口比较已构造数组内的地址并复制标量/offset，不运行随机、生成、动画、Lua回调或持久入口。`GetLuabridgeUserdata`使用官方const/non-const双元表读取重载，未知存储返回Unavailable，room/level域传播不完整。

配置资源沿用17的`LoadStageBinary`/`GetRoomByStageTypeAndVariant`。本轮回读官方`VirtualRoomSetManager.cpp:1281`：加载在super后重新提交插入；`:1291`查找先super再只读虚拟集合fallback。只对未加载且无既有storage/count的集合加载，不重载已有集合，不重选房间、不推进玩法RNG；多descriptor共享同一集合借用。具体双配置、虚拟/Mod配置内容对照与资源版本验证归I07，当前不声称其已完成。Room边界helper、Camera/HUD/Backdrop依赖及Room/Player/HUD退休入口复用17已核合同；本轮不增加Hook，不恢复字节，不运行完整Game/Room/Grid Update，不调用ChangeRoom/SaveState/InitSeeds来补状态。

仍未扩大的边界：J273原生异常/EH下不能证明安全恢复的路径继续撤显示并保留Client隔离；current-map指针所有原版生命周期的全局闭包没有穷尽，当前Client不使用未建立的原生缓存别名，而是由本owner安装/恢复。Grid工厂/派生Init/即时移除/析构及专用缓存的具体副作用在采用时按I04与I15—I24一起补齐，不由这份descriptor研究外推。

## 本轮验证与交接

已完成I01—I06代码、应用目录及静态主审。`pwsh -NoProfile -File scripts/build.ps1`最终纯编译/链接成功：`isaac_cs_replica`、`isaac_cs_net`、Win32 Release `zhlIsaacCS.dll`、现有工具和i686 Rust harness均已构建；没有执行这些工具的测试入口。产品包同步新增应用目录及正式观测Lua。

第一次沙箱内构建停在MSBuild启动阶段，主动中断后在获准的沙箱外构建成功；首次编译发现本轮新增的局部变量遮蔽参数警告，已改名。随后按SDK宽度和完整成员语义修正，最终增量构建成功。官方SDK/RapidJSON头的既有警告和既有Lua采集局部变量遮蔽警告仍存在；未修改第三方SDK，也未以屏蔽警告代替修复。

按本次只推进I01—I06的范围及I29安排，未运行游戏、probe、分项测试或单元测试。全部Room/Grid adapter完成后从同一正式解析器进入统一E01—E18验收；当前不报告I29通过，也不改变11/12/17既有成功与失败记录。不创建本tx artifact或测试报告包。

2026-11-03 I29修正：真实Greed换波中同Key Rock(type2)的Variant由1000变0，实例revision仍0、SpawnSeed与spawnFrame未变；原计划把Variant当构造参数而误拒绝。按本地GridEntityDesc文档，VariableSeed随事件变化；原生工厂按Type/index分配，Variant不参与构造参数。当前同实例change安装Variant/VariableSeed等最终Desc状态，Type/SpawnSeed、身份与实例修订仍按构造/换代合同核对；Door先前局部例外已统一到此规则。完整结果见[统一验收](tx-04-18-acceptance.md)。
