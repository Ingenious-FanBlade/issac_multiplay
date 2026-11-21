# 第7步：生产布局源与生成

日期：2026-11-10（Asia/Singapore）。整数布局版本 **57**。本次完成第7步的唯一生产布局源、生成能力和生成表；正常生产缓存、Host reader、可靠资源上下文、Client decode／安装及旧链退役仍按第8—14步接线。第15步仍是唯一统一整体验收，不把布局生成或纯编译写成运行验收。

**同日后续：Room／Level影子描述生成层已实现**，详见[接口、失败方式和构建结果](room-level-description-implementation.md)。`ClientDescribeRoomLevel`从完整影子记录生成自有不可变的Room／Level／Descriptor／配置描述；52个数字角色及L2帧界同源生成，旧JSON DTO明确改名Legacy。Release／Win32描述库及产品DLL纯编译／链接通过，无测试；布局版本和本页原生UNKNOWN保持，实际网络调度／原生应用仍待12／13步。

## 生产输入与范围

新增 [`room.json`](../../../../mod/binding/state/room.json) 的53个类型和 [`level.json`](../../../../mod/binding/state/level.json) 的18个类型，共71个布局，编号 **239—309**。`version.json`将两文件登记为正式域，增加`room-config-visual`和`shader-table`资源空间。生成器仅以`mod/binding/state/`为布局源；设计账本和本次迁移过程不是第二份构建／运行输入。

资料优先级仍是本地原版Wiki、官方REPENTOGON Wiki／ZHL／源码，然后引用既有J273静态证据。本次重新阅读了Room／Level／RoomDescriptor／RoomConfig和FXParams／FXLayers／Backdrop／Camera／RailManager的本地说明，以及官方配置、描述和Minimap结构声明；没有执行Lua Getter、游戏函数或新增实机观察。既有[Room](room-synchronization-review.md)、[Level](level-synchronization-review.md)、[描述](room-descriptor-synchronization-review.md)、[保存／探索](room-save-exploration-synchronization-review.md)及[显示资源设计](room-display-resource-design.md)继续是证据说明。

| 正式类型／编号 | 当前生产合同 |
|---|---|
| Room239 | 13个world块；直接几何／时钟／颜色、八门映射、背景、FX、水光波纹、强制焦点、轨道、地狱背景及活动波纹辅助Sprite |
| Backdrop240／FXLayers242／RailManager249／RoomHellBackdrop253 | 实际用途绑定／显示分支与有效数据；RNG、候选过滤、未来推进、原生指针及GL对象不上线 |
| RoomConfig_Room263 | 原生5Ch字节表与固定15B RoomConfigVisual；StageId／Type／Subtype、两个i8尺寸及检查后窄化的Shape；生成候选／权重／Spawn／Merged图留Host |
| RoomDescriptor270 | L1固定53B、L2为1—12B；两份独立可空配置引用，五位显示flags含HasWater；不传完整保存体／历史／seed |
| LevelGamePrefix276 | `[0,18910)`原生视图，SDK大小1890C另列；L0固定22B，L3固定1014B，D1当前描述颜色谓词1B |
| RoomVeinSprite281／RoomRailSprite282 | 仅裁掉具体绘制消费者必覆写的状态；原生嵌套UNKNOWN仍展开，不通过删除wire消去未知 |
| BackdropBinding283—284／Water285—287 | 最多7个有序唯一用途绑定；水结果1—48B，颜色32B、实际有符号phase／模式／图像／flow同批；不重复发送reverse位或另一份flow |
| Lighting288／Wave289／Focus290 | 1—46B最终光照、1—33B两波纹、1／9B强制焦点；不传Host视口／相机历史，不执行显示生产器 |
| SelectedRay291／Poop292—293／Vein294—295／FXScene296 | 完整可显示有序层、实际选中非空ray组、当前poop alpha、十槽有界vein图；无分支不读旧payload，缺源不折空 |
| SavedMapIconPayload297／FinalSavedMapIcons298／CurrentDescriptorVisual299 | L2的16类OR／max结果与必要动态帧；D1只保留Error→ModelingClay颜色谓词，不伪造原数值Getter |
| MapFocus300—301／MapBadgeList302／QuestionImage303 | 两份配置各自的挑战32锚点／有效描述成员，最多7项有序1—8B徽标，以及独立Lost问号图像；由后续Presentation唯一owner引用，不重复采集 |
| ResourceEntryHeader304／Pixels305／BlobHeader306／Recipe307—309 | 分别登记7／16／14／16／21／196B固定typed值；完整payload分派、blob／recipe调度、具体shader／blend／sampler枚举和GPU leaf ABI仍归第9—13步 |

Room的R0保持36B：descriptor指针经同world结构owner解析成**物理source-slot i32**，维度i32，实际类型／宽／高各u32及四个bounds浮点。有效槽0..526，明确排除525 Deathmatch；它不是原ListIndex，也不是报文地址。R1为8B，直接R2为104B，八门R3为48B。Grid槽成员关系／有序Entity目录由结构消息提供，不能从门primary Slot反推八槽别名。

L1的`visited_count`线上为`!=0`的u8；`flags`线上为原bit0／10／11／18／5映射的低五位。L0的Boss匹配在所有模式保留，只有Greed wave在非Greed规范0且不读旧值。L3的507项是物理源槽或FFFF，不能用GridIndex／SafeGridIndex／ListIndex重新猜表；有序普通成员与离图生命周期仍归结构消息。MapFocus中的u16则是活动**描述成员序号**，与source-slot明确区分。

背景绑定的slot固定为floor／wall／aux／special／tiles／water／nlfloor七个用途，按slot升序且唯一，同一用途的不同pass由外层用途身份区分。不是把七个非Null智能指针逐个发送。地狱背景只传活动资格、当前位移和实际Sprite；不执行会移动Entity的82EE90。完整世界可显示事实不能按Host当前视口裁剪。Room记录上限32MiB；容器和资源各有独立上限，超过预算报实际Gap，不能截断后ACK，也不表示性能目标已实测。

## 生成能力与目录

`schema.json`、`gen_state_layout.py`和`state/layout.h`共同补齐：

- `synthetic_fields`支持`storage=aggregate`的复杂结果列表，步长必须为0，元素必须是已登记布局；来源是自有结果缓存，不是假造原生数组偏移。
- 自有资源字段保留`Operation::Resource`及正式target空间，例如水／poop图像、背景用途绑定、独立问号图像及recipe shader；不能因无原生偏移就降成没有资源归属的整数。
- `wire.count_type`可为u8／u16，默认仍u16。生成的`Field.count_bytes`为0（固定槽／非容器）、1或2；u8计数上限不得超过255。MapBadgeList使用u8计数，1—8B；既有固定槽和u16列表不变。
- 可选负载继续使用同块先行选择器和具名谓词。L2把present与内层mask分成具体类型，内层九个动态帧仅由mask对应位决定；缺席不读残留。
- 正式接管旧stream目录时，在生成器内存中把原`type`适配为返回类型、从原ID提取owner、按`source_schema`取domain；原目录文件、原始签名和旧ownership不改写，也不扩大其他既有owner的覆盖要求。

原stream的Room269、Level131、RoomSave98、Exploration99行，共 **597行**，按原目录一基行号完整生成，保留重载、重复ID、null／空参数差异。直接字段及最终投影关联正式数字布局／字段；真正跨owner项保留具名上下文，Host命令／生成库字段明确归Host。它们不是任意Getter执行表，也不承诺被裁原始数值与投影相等。RoomDescriptor44行已包含于Level131，不再次制造第二份采集owner。

生成输出见[布局索引](../../../../mod/binding/layouts/README.md)和四份逐行表：[Room](../../../../mod/binding/layouts/07-room-level/stream-catalog-room-members.md)、[Level](../../../../mod/binding/layouts/07-room-level/stream-catalog-level-members.md)、[RoomSave](../../../../mod/binding/layouts/07-room-level/stream-catalog-room_save-members.md)、[Exploration](../../../../mod/binding/layouts/07-room-level/stream-catalog-exploration-members.md)。CMake已登记旧stream目录依赖，目录变化会重新生成，不再依赖手动调用。

## UNKNOWN与尚待实施

原生UNKNOWN未删除、补零或改称填充。Room直属仍28B；静态内嵌递归总计 **613B**，包含Backdrop374B、FXLayers109B、EntityList27B、轨道14B、地狱背景17B及其他内嵌对象；这是已展开内嵌范围的总数，**不包含所有指针后动态payload、私有图形对象和外部官方owner**。FX／FXPoop各5B、FXPose2B、RayGroup2B、Vein每节点7B、Camera3B等另有各自类型表。Level前缀仍22B，描述直属未归属字节0不等于Flags高位或SDK业务语义全知。保存type5地图输入视图仍70B、通用EntitySaveState仍74B；配置Stage候选RoomSet及RoomSpawn各有3B未知。

运行方面仍需正常生产结果缓存与准确world／presentation／member／resource修订、全部必要显示分支的合法来源、图形资源物化／上传、类型化解码、地图／水光／FX adapter、原生owner安装与借用恢复。头部／字段注册不等于完整资源协议已可接收，处理器名称也不表示已绑定运行代码。保存体不会被伪造成Client保存vector；不恢复Lua采集、背景RNG轨迹、地图生成／分数生产器、Camera.Update或原版Online。第三方Mod继续在本轮范围外。

## 生成与纯编译情况

使用正式入口`python mod/tools/gen_state_layout.py --out build/repentogon/generated`生成复制、线格式、Client分派、成员及Grid描述表和全部布局文档。迁移时实际遇到并修正了设计账本自由证据标签与生产枚举不符、配置目录行转移时上下文遗漏；静态审阅另修正乞丐图标帧上限为5，并补回取消Rail主动画wire后必须继续登记的AnimationState内部3B未知。原始账本事实保留，未以清UNKNOWN规避生成错误。

Release／Win32／v143的`isaac_cs_state`纯编译通过，`ISAACCS_BUILD_E2E_TOOLS=OFF`。没有运行单元、隔离、smoke、E2E、游戏或其他测试，没有注入／部署／实机操作。运行行为与性能统一留到第15步验收。
