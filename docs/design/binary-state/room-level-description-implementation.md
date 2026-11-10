# 第7步：Room／Level影子状态描述生成层

日期：2026-11-10。范围是完整二进制影子状态到自有纯值描述，不接网络解码调度、Host采集、原生安装、Hook或绘制。沿用布局版本57；没有新增线字段。测试统一在第15步进行。

## 资料与语义

先读本地原版Wiki的Level、RoomDescriptor、RoomConfig_Room、Room和GridRooms，再读REPENTOGON Wiki的RoomDescriptor、RoomConfigRoom、Backdrop、Minimap，以及官方RoomDescriptor.zhl、RoomConfigRoom.zhl、Level.zhl、LuaRoomDescriptor.cpp和RoomConfigUtility.cpp。当前入口象限不等于GridIndex／SafeGridIndex；物理source-slot不等于描述成员号或原ListIndex。AllowedDoors、Doors八目标和实际八门成员各自保留，允许无实门的有效目标和门别名。Data与OverrideData独立可空；不按Level.stage替代配置StageId，不恢复Spawn／Merged图。

## 先列失败方式

- 旧JSON DTO误作新描述；辅助／错误布局冒充Room、楼层前缀或描述；布局版本不同。
- Room、Floor、Descriptor、Grid的身份空间／run／owner不符，块混入另一key／use／shape；把最后变化版本误当当前目标版本。
- 缺少完整块、未定义块、多余或截断字节、可选波纹Sprite的presence与负载不一致；读取借用在返回后失效。
- 总负载或Values池预算不足，复制描述重复拷贝负载／收费；输入失败后泄漏租约。
- 当前Room与Floor维度不符；入口象限、物理source-slot和L3映射错误；把-100／-101路由或-19 Deathmatch当可安装房间。
- 活动描述源槽／成员／身份重复，有序普通成员缺项；L3引用缺失、非活动或错误维度描述；假造原ListIndex。
- 双配置引用混合、缺资源伪装Null、同资源号重复定义、配置epoch／布局／尺寸／Shape非法；错误要求两份配置内容相同。
- L2把来源失败当缺席、mask后的动态帧缺失／多余／超界，恢复原保存vector或丢掉楼梯／Portal位。
- 门引用指向另一Room、非Door布局或不存在成员；误拒绝多个槽共享同一门，误将结构目标等同实门存在性。
- 颜色被压成RGBA，房间帧基准重新锚定，水流重复发送，嵌套显示投影被截断／转成字符串字段字典。
- 把本层描述构造当成完整递归解码、GPU资源ready、原生应用或AppliedAck。

## 实现与后续边界

[`client_room_description.h/.cpp`](../../../mod/src/game/client/room/client_room_description.h)提供`ClientDescribeRoomLevel(ClientRoomLevelShadowView, byte_limit)`，输出`ClientRoomLevelDescription`。它只接受已完成第12步递归校验的完整记录及已接受的配置／成员上下文，不接单次增量、JSON、Lua或游戏对象。

`state_room_view.inc`与现有布局表同次生成，登记Room239、LevelGamePrefix276、RoomDescriptor270及配置263的**52个语义角色**、数字字段索引、必需块、真实Grid布局、Door布局及L2九种动态帧的bit／帧界。描述代码不手写另一份布局号、字段线偏移或图标帧上限；任何新增根字段或失去固定起点都由生成器报告，不能无声漏掉。当前根字段最多25个，切片容量同样由生成器给出。

| 输出 | 当前实现 |
|---|---|
| Room | 36B几何、帧基准／Greed timer、两份完整11值ColorMod、水量／熔岩／scroll、八门成员和格序号的类型化访问；Backdrop、FX、水光波纹、强制焦点、轨道、地狱背景及波纹Sprite保留完整带布局的只读投影 |
| Level | stage／stageType、最终有效诅咒、入口象限／维度、Greed波次、所有模式的Boss匹配及D1颜色谓词；L3按维度／格子访问，FFFF明确空 |
| Descriptor | 原始Grid／Safe、维度、门mask／八目标、DisplayFlags、visited布尔、五位显示flags及Greed图标；分别提供source-slot、C/S member、普通顺序，不构造原ListIndex |
| 双配置 | 独立的`optional<ClientRoomConfiguration>`；保存实际资源号、StageId／Type／Subtype、两个尺寸及Shape，允许相等、不同或分别Null；不按内容合并身份，不补Name／Variant／Spawn／Merged |
| L2 | `ClientSavedMapIcons`保留present、16位完整mask及按bit寻址的帧；按生成表消费恰好应有的动态帧，固定帧0不额外读取；不创建保存vector |

根结构检查覆盖布局版本、同Floor／Room的上下文、身份空间与run、key／use／shape、成员号唯一性、块长／版本／必需块及总预算。未变化块的版本允许早于target_version。world_revision、presentation_revision、member_epoch、resource_epoch和game_frame分别保留，不能因GameFrame相等就混用结果。

同world关系在生成时核定一次：排除source-slot525；普通成员顺序必须稠密且唯一；离图槽不混入普通计数；L3只能引用实际存在、维度相符的描述；当前入口正格经当前维表解析，负入口仅接受真实−1..−20且排−19，−100／−101仍只作为八门路由值保留。实际Door引用必须是本Room的已接受Door成员，多个槽可以共享同一成员；不强迫候选格序号等于Door主GridIndex，也不把AllowedDoors或结构目标等同实门存在。

配置输入必须绑定当前已接受资源epoch、正式配置布局及完整固定负载；缺项报具名Gap，不改为Null。Shape只接受1..12，尺寸保留来源的正i8值，不能拿当前Room含边界的grid尺寸覆盖配置。配置变成自有标量后不再保留输入指针或第二份原始配置字节。

所有Room／Level／Descriptor字节一次性复制到同一个`shared_ptr<const ClientRoomLevelStorage>`。描述、子描述和投影共享此owner与Values池租约，输入只需在生成函数返回前保持有效；最后引用释放才归还计费。负载上限只能收紧32MiB，元数据／容器实际capacity也计入现有Values池；资源配置的原始长度参与输入预算，不能移出统计。异常路径通过RAII释放候选，不发布半份描述。

旧运行链明确改名为`LegacyClientNativeSceneDescription`、`LegacyClientNativeDescriptorDescription`和`LegacyClientBackdropConfiguration`，并同步调用点及旧内存统计。`ClientObjectKey`移入独立纯值头供新旧路径共用，数值布局和身份语义保持。新描述没有旧DTO转换，`ClientNativeWorldPrepare/Refresh`当前仍显式接收Legacy输入，第13步再接新的原生writer；不为了维持旧校验补造seed、全配置或背景RNG。

第12步仍负责嵌套ANM2、复杂FX／图结构、图像／shader资源闭包的完整递归校验；此处对固定根与跨对象关系的检查不等于完成网络解码或本地GPU准备。第13步负责实际安装、地图／水光／FX绘制和原生owner借用恢复。MapFocus／MapBadgeList及地图显示资源仍由第8步Presentation唯一owner按同一上下文交接。Host来源缓存、GPU ABI、原生UNKNOWN与第9—15步未决合同均保持。

## 生成与纯编译

同一次正式生成产出`state_room_view.inc`，CMake新增正式`isaac_cs_room_description`库并链接产品目标。布局源与线字段未改，整数布局版本保持57。没有新增单元测试或隔离脚本，本页失败方式没有提前执行。

Release／Win32／v143描述库纯编译和完整`zhlIsaacCS.dll`产品链接均通过，退出码0，`ISAACCS_BUILD_E2E_TOOLS=OFF`。首次描述库编译的一处有符号比较警告已修正，最终描述库编译无该警告；产品编译仍报告SDK／RapidJSON等既有警告，未将其写成无警告构建。普通沙箱完整构建停在File Globs且无后续输出，直接沙箱内MSBuild也未继续，均已中断；随后在沙箱外通过既有MSBuild工程完成产品纯编译／链接，产物只写仓库`out/repentogon/`，没有安装到游戏目录。

没有运行游戏、E2E、smoke或其他测试，未注入／部署。所有运行验证只在第15步统一整体验收进行。
