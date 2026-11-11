# 第7步：必要显示与资源设计补齐

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。304资源固定头更新为8B显式table、306 blob头更新为15B；table/index与kind分离，已固定表的字段仍只传u16引用。旧7B／14B为版本57事实；完整typed payload、table/kind矩阵与物化仍待第9步后续。32MiB只作内存／解码保护，不能替代10Mbps总上行／7.5Mbps持续、首次准备1MiB／10秒的真实计费。

2026-11-10；官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。本次完成第7步的**显示／资源设计**：配置投影、背景内容来源、小地图重建输入、水／光／波纹结果、资源依赖与失效、Client安全消费及后续接线职责。它不是生产布局或运行实现；整数布局版本仍56，未构建、启动游戏或测试。原生字节事实分别沿用 [Room](room-byte-ledger.md)、[Level](level-byte-ledger.md)、[Descriptor](room-descriptor-byte-ledger.md)、[Presentation](presentation-byte-ledger.md)；新增配置字节表见[机器设计表](room-display-resource-design.json)和本文第3节。UNKNOWN不借本次设计消除。

## 1. 资料与本轮纠错

先读本地原版 Wiki 的 `RoomConfig_Room`、`RoomConfig_Spawn`、`RoomConfig_Entry`，再读官方 Wiki 的 `RoomConfig`、`RoomConfigRoom`、`RoomConfigStage`、`Backdrop`、`FXParams`、`FXLayers`、`Camera`、`Minimap`、`LayerState`、`renderer/{Image,Renderer}`。根为 `F:/projects_isaac/IsaacDocs/docs/`、`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/`。公开语义有具体疑问后，读官方 `libzhl/functions/{RoomConfigRoom,RoomConfigStage,RoomSet,RoomSpawn,RoomEntry,Backdrop,BackdropConfig,FXParams,Minimap,MinimapConfig,KAGE_Graphics_Image,KAGE_Graphics_ImageManager}.zhl`，`repentogon/LuaInterfaces/{Room/LuaRoomConfig,Room/LuaRoomConfigRoom,Room/LuaRoomConfigStage,LuaBackdrop,LuaRender}.cpp`、`RoomConfigUtility.cpp`、`VirtualRoomConfig/VirtualRoomSetManager.cpp`、`Patches/{MinimapPatches,ImagePatches}.cpp`。

最后对当前J273 IDB作只读静态分析；本文地址为VA，RVA=VA−400000h。没有执行目标函数、附着进程、修改／保存IDB，也没有哈希操作。涉及大函数的反编译有栈分析警告，关键偏移另查指令，不把伪代码当运行观察。

| 发现 | 本轮处理 |
|---|---|
| 旧Door说明误写“RoomConfig.type35” | `802758..80276F`沿Game→Room+4→Data+10取配置，`80276C cmp DWORD [eax],23h`读的是**StageId+0==35（Home）**。`9666CD..9666E6`的小地图隐藏条件同样读StageId35，另查Type+8==16。更正现行说明／布局元数据，保留这项错误记录；不改变Door字段、类型号或版本 |
| 原版Wiki的RoomConfig.Mode返回userdata | 官方 `LuaRoomConfigRoom.cpp`已修为int；这里只保留语义差异，不新增Mode网络字段 |
| 看似只读的GetRoomSet | `LuaRoomConfigStage.cpp::GetRoomSet`在未加载时调用LoadStageBinary；正式采集不能为找配置而调用它。GetRoomByStageTypeAndVariant也经过官方virtual-set fallback；不把它当会话资源身份或完整性证明 |
| Image.GetTexelRegion文档称每像素4个float | `LuaRender.cpp:339..356`只分配`width*height*4`**字节**；J273 `A4C860..A4C879`明确以`GL_RGBA(1908h), GL_UNSIGNED_BYTE(1401h)`读回。设计采用RGBA8字节，不按float解释，也不调用有边界／乘法问题的Lua包装 |
| 原生GetTexelRegion不是普通内存memcpy | `A4C840`切换FBO、调用glReadPixels并恢复／重设部分图形状态。资源获取须在正常生产已提交后的图形主线程阶段，以专用状态保护执行；不能在world reader、任意Hook中或后台调用 |
| MinimapConfig::Render的SDK签名 | 既有核对命中的是含NPC生成／开火的`4ECA30`，不得调用。本次仍采用自有地图绘制adapter，不因名字相似放行；官方MinimapPatches要求图像提交前显式选择ColorOffset shader |
| Minimap+614..61B原8B UNKNOWN | 补核`965BDE`加载`gfx/ui/minimap_questionmark.png`，`965BFF..965C0C`赋入+614的Image strong pair；966680在Lost诅咒分支直接消费它。旧Client的QuestionImage也已使用这个位置。它是**独立问号图像资源**，不是iconsSprite的某帧；账本修订2将Minimap直属UNKNOWN23→15B，其余未知不变 |

## 2. 同步边界与唯一owner

| 项目 | 要同步的事实／结果 | 无需或不应同步 |
|---|---|---|
| 描述双配置 | Data、OverrideData两个独立可空配置引用；当前采用的显示几何／类别／特例输入 | 指针、完整RoomConfig库、全STB、随机选房参数、候选／权重、Spawn数组、MergedData指针图 |
| 背景 | 真正被消费的floor／wall／辅助surface内容、实际图像与Sprite外观、surface范围、所用分支；地狱横向背景当前位移 | Backdrop RNG／seed、Init历史、装饰选择候选、FBO／GL／引用计数、没有被消费的scratch surface |
| 水面 | 最终32B水色乘量结果、实际水流、真实经典／反射模式、实际纹理及本次采用的相位；waterAmount保持原有唯一owner | 四端点／插值timer／背景RNG；选择发送最终有符号相位后不再重复发reversewatergfx；不让Client再插值或二次乘waterAmount |
| 光照与波纹 | world光照资格／最终强度、shadow-fill／light-tint；两波纹各自当前中心／有效半径／包络；必要特殊背景pass | 光照生产查询、物品查询重放、波纹未来speed／current／maximum、去重树、原生Render scratch和Host视口坐标 |
| FX／轨道／雨 | 已核实际层／有序pose／选中ray组／poop当前alpha／vein图／轨道有效字节；真正存在的雨Effect | 未选ray候选、生成过滤／速度／未来倒计时、雨生成器、矿车Init、FXParams配置色生产链 |
| 地图 | L0/L1/L2/L3既定事实、两份独立边框／两类图标及独立Lost问号图像资源、挑战32已选锚点／对应描述、最终震动、有序MapBadgeList及world显示限制 | Host的state／holdTime／按键／窗口／展开alpha、Host裁剪后的FBO、揭图生产flags／命令、保存vector／ESSM ID、Client进度或原版Online状态 |
| 相机／教程／排版 | 必要强制焦点意图、Room当前scroll；同world地图／HUD内容 | 相机历史ring、Host分辨率／HUDOffset／本地选项／设备图标；教程按钮按各端C/S实际绑定渲染，不照搬Host设备 |
| 楼层名、boss spot、player spot、suffix、音乐 | 本次已选文字／图像／Sprite／声音结果，由Presentation／音频现有owner承接 | 整份RoomConfigStage库及其Set／Unload操作；不得仅根据stage推断已选资源，挑战44逐房主题尤其如此 |

Room／Level拥有世界几何、配置引用、探索事实和FX结果；Presentation拥有地图显示资源、MapBadgeList／震动及整屏／转场；资源表拥有不可变内容。相同结果只维护一个owner，其他域引用它。两端的本地FBO、GPU资源、CPU绘制队列和强引用仍各自正常构造、持有、退休。纯本地设置可以改变排版或降低图形质量，不能揭示Host未允许的地图、修改世界或写访客进度。

## 3. 配置资源：发送显示投影，不传生成配方

`RoomConfig_Room`的原生5Ch逐范围表如下；“已知偏移／SDK声明”不等于Flags每一位或生成算法业务语义已解释。完整机器表另列证据等级。`RoomConfigStage=C4h`、`RoomSet=2Ch`、`RoomSpawn=10h`、`RoomEntry=Ch`也登记；两个3B间隙仍UNKNOWN，未把它们改成填充。

| +00 i32 StageId【显示资源】 | +04 i32 Mode【Host／定位元数据】 | +08 i32 Type【显示资源】 | +0C u32 Variant【Host／定位元数据】 | +10 i32 Subtype【显示资源】 | +14 string Name(18h)【Host；实际文字另交Presentation】 | +2C i32 Difficulty【Host】 | +30 f32 InitialWeight【Host】 | +34 f32 Weight【Host】 | +38 u32 Doors【Host；实际门关系归R3/L1】 | +3C bitset32 QuestDoors【Host】 | +40 Spawn*(4B)【Host容器关系】 | +44 u16 SpawnCount【Host】 | +46 i8 Width【显示几何】 | +47 i8 Height【显示几何】 | +48 i32 Shape【显示几何】 | +4C bitset32 Flags【Host；实际结果另承接】 | +50 Config*[2](8B)【Host合并生成图】 | +58 u32 OriginalVariant【Host／定位元数据】 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

其余四个原生类型的单行归属表（内嵌子体另按自身表解释，不吞掉未知）：

| RoomConfig_Stage +00 u32 id(4B)【Host】 | +04 string displayName(24B)【实际文字另承接】 | +1C string playerSpot(24B)【实际图像资源】 | +34 string bossSpot(24B)【实际图像资源】 | +4C string suffix(24B)【实际资源／具名上下文】 | +64 RoomSet[2](88B)【Host候选库】 | +BC i32 musicId(4B)【Host选择；音频发结果】 | +C0 i32 backdrop(4B)【Host选择；Room发结果】 |
|---|---|---|---|---|---|---|---|

| RoomSet +00 string filepath(24B)【Host】 | +18 Config*(4B)【Host库】 | +1C u32 count(4B)【Host】 | +20 map头(8B)【本地索引】 | +28 bool loaded(1B)【本地加载】 | +29 bytes[3]【UNKNOWN】 |
|---|---|---|---|---|---|

| RoomSpawn +00 u16 X(2B)【Host】 | +02 u16 Y(2B)【Host】 | +04 Entry*(4B)【Host候选】 | +08 u8 CountEntries(1B)【Host】 | +09 bytes[3]【UNKNOWN】 | +0C f32 SumWeights(4B)【Host】 |
|---|---|---|---|---|---|

| RoomEntry +00 u32 type(4B)【Host】 | +04 u16 variant(2B)【Host】 | +06 u16 subtype(2B)【Host】 | +08 f32 weight(4B)【Host】 |
|---|---|---|---|

新增具名资源投影 **RoomConfigVisual，固定15B**，没有生产layout号：

| +0 i32 stage_id（4B） | +4 i32 type（4B） | +8 i32 subtype（4B） | +12 i8 width（1B） | +13 i8 height（1B） | +14 u8 shape（1B，1..12） |
|---|---|---|---|---|---|

StageId有上述Home正面依赖；Type／Subtype供地图类别、镜面／矿车／挑战图标及特殊boss subtype89 Light；Shape供地图轮廓、L形／水面与受控几何；Width／Height是**配置内域尺寸**，与R0含边界的实际grid extent分开，供本地配置承载和几何一致性。官方RoomConfigUtility给出了十二种标准尺寸，Client可以校对／派生相关边界，但不把实际Room宽高覆盖到所有离房配置。保留实际尺寸可处理独立Data／OverrideData，而不要求二者相同。Shape从已核i32窄化，非法值不能截低字节后接受。

Mode／Variant／OriginalVariant／Name可以在Host本地登记表定位对象，但不进入这个投影。Wiki公开它们不构成同步理由：本方案不在Client选房、生成Spawn、重建获得史或按配置再选图。需要的实际楼层名／头像／图像已由对应结果owner提供；调试配置名称不算玩法显示。Doors／QuestDoors／Flags／MergedData的生成后果由R0/R3、L1、当前Entity/Grid和最终背景承接，不要求原始Getter相等。

资源身份为**会话内分配的u16序号**，绑定Host本地实际配置owner/use及该显示投影内容版本；原始地址不发送。Data和OverrideData分开解析，允许相等、不同或明确Null。不能以`stage/type/variant[/subtype]`拼成唯一键，更不能用当前Level.stage代替STB StageId。官方RoomIdentifier实际还区分mode／originalVariant／shape／name，本身也不是跨进程身份协议。不同原生对象可以有相同15B；不根据内容相同推断对象连续性。

投影原位变化产生新的不可变资源项，L1引用随同一world事务更新；原始Weight／Spawn候选变化而15B不变不触发下行。Client资源owner只构造供显示读取的自有配置承载，正常初始化string／容器，Spawns／MergedData保持本地空且与生成执行器隔离；不裸写5Ch，不借用本地存档库可变对象，不调用LoadStageBinary、GetRandomRoom、PickEntry、Merge／Init／Restore。没有来源／构造未就绪不是Null。

## 4. 图像内容、资源表和获取时点

本轮决定三种图像来源，按已有内容选用，**不以种子重放作为替代**：

1. 已确认是文件图像，且当前没有内容覆盖：规范资源路径＋实际Image选择。路径元数据与已加载Image分开，沿用版本32合同；替换路径而旧Image仍在画时不能抢先LoadGraphics。
2. 程序surface的正常合成已能完整表示：保存**本次真正采用的有序显示recipe**。只记录已发生的Clear／图像quad及其实际资源、局部目标／源矩形、完整颜色、固定shader视图和blend／sampler，不记录随机／Spawn／回调。Client只用自有图像合成器解释已注册的值记录。不能直接回放Host函数、函数地址、GPU命令流，或重新调用随机选择的原函数。所有被引用的输入内容先就绪；资源图有环、依赖未知、缺少某次写入或shader未登记，recipe不完整。
3. 已有最终程序surface，尚无完整recipe，或冷连接时错过生产历史：获取其实际内容。J273 A4C840给出可用的**RGBA8原生读回leaf依据**；在正常生产／绘制已完成且必要batch已消费后，C++资源owner复制内容到自有缓冲。允许资源物化阶段的这项图形读回，不把它塞进通用对象memcpy计划，不经过Lua，不额外执行背景生产或整次Host Render。浮点／特殊格式surface若不能由此无损表示，使用具名已核recipe或另登记精确格式，不能无声量化为RGBA8后声称相等。

主路线是可审阅的已选内容／纯值recipe，像素是完整内容来源而非逐tick整房截图。记录正常合成的显示recipe不重演其中夹杂的装饰Entity生成；实际Entity另由实体域接收。Host图形选项造成某个pass未执行时，不能把“未经过”编码成world禁用；读取已存在内容或使用已核的只读纯值投影。仍无来源时为具名Gap，不能恢复随机或强迫Host改变选项。

资源绑定按真正消费者登记：Backdrop的floorBuffer／wallsBuffer、实际选中的floorSurface2／tempBackdropSurface和gfx／waterGfx／nlfloorGfx；未被任何当前具名pass消费的scratch不下发。Backdrop+4F18/+4F20的两个Vector是surface范围，共16B，不当Host窗口尺寸。四个Sprite按实际采用分支引用既有ANM2姿态／图层投影；教程controls的本地按钮层另建本地资源。地狱横向背景保持已核active／当前位移／实际Sprite，不执行82EE90移动Entity。

BackdropBinding的slot是固定登记的**最终绘制用途**，不是把七个原生smart pointer按非Null列出来。Host在读取分支时把+20 surface选择等消解为当前floor／wall／aux／special／tiles／water／nlfloor用途的实际引用，不另发原始selector。同一用途在不同pass确实采用不同内容时按外层具名pass分别绑定，不能合成一个“最后资源”。

**资源表项设计（替代第9步9.6旧“全是路径”草稿，未注册）**：

| +0 u16 index（2B） | +2 u8 kind（1B） | +3 u32 payload_bytes（4B） | +7 payload（按kind精确消费） |
|---|---|---|---|

kind0/1/2分别为ANM2／文件图像／内置shader路径负载（`u16 bytes + 有界路径`）；kind3为15B RoomConfigVisual；kind4为具名SurfaceRecipe；kind5为SurfacePixels描述。kind4带生成器登记的固定recipe layout编号及有界记录，不能由网络指定原生偏移、顶点stride或任意shader参数字典。未知kind／layout整项拒绝。FFFF仍是明确无资源，不能作为Wait／Gap编码。

SurfaceRecipe的首个具名设计是离屏背景合成，不是整屏录制。头16B如下；ImageGeometry12B依次为下方像素描述的前12B，不含content_bytes。操作按原顺序执行，所有目标坐标均在该surface内；没有Host相机／屏幕坐标。一个layout只允许下述两个操作，其他原版shader如有不同输入必须另登记固定layout或采用精确像素，不能把未知属性省掉：

| +0 u16 recipe_layout | +2 ImageGeometry（12B） | +14 u16 operation_count（≤4096） | +16 有序操作 |
|---|---|---|---|

| Clear +0 u8 op=0 | +1 KColor clear_color（20B） |
|---|---|

| Quad +0 u8 op=1 | +1 u16 image | +3 u16 shader | +5 u8 blend_view | +6 u8 sampler_view | +7 u8 source_space（0=pixel，1=UV） | +8 源四点f32[8]（32B） | +40 DestinationQuad f32[8]（32B） | +72 KColor vertex_colors[4]（80B） | +152 ColorMod（44B） |
|---|---|---|---|---|---|---|---|---|---|

Clear21B、Quad196B。shader必须是该layout已登记的普通ColorOffset；blend_view／sampler_view是编译期固定语义枚举，不是Host GL整数／地址。保留每顶点颜色和全部ColorMod分量；纹理尺寸等由同一图像资源派生。只允许完整已定义的初始清理或覆盖，不继承未提供的旧surface内容；多阶段合成用不可变输入资源引用，不能自引用目标。普通图片的像素上传与这些有限操作由C++资源owner实现，不执行报文里的原生函数。

补读官方Wiki `renderer/SourceQuad`及`SourceQuad.zhl／DestinationQuad.zhl`：DestinationQuad为四Vector32B；SourceQuad继承它，另有+20的i32 coordinateSpace，**实际36B，不能只复制八个float后丢掉坐标空间**。source_space经0／1检查后窄化，不把UV当pixel，也不混用logical／padded尺寸。两个原生表如下：

| DestinationQuad +00 Vector topLeft(8B)【显示几何】 | +08 Vector topRight(8B)【显示几何】 | +10 Vector bottomLeft(8B)【显示几何】 | +18 Vector bottomRight(8B)【显示几何】 |
|---|---|---|---|

| SourceQuad +00 DestinationQuad(32B)【显示几何】 | +20 i32 coordinateSpace(4B)【换算为u8 0／1】 |
|---|---|

SurfacePixels描述固定16B，后续内容独立在可靠Context资源消息中分块传送：

| +0 u16 logical_w | +2 u16 logical_h | +4 u16 stored_w | +6 u16 stored_h | +8 u8 format（0=RGBA8） | +9 u8 origin（0=GL下起） | +10 u8 alpha_encoding（0=原生编码） | +11 u8 color_encoding（0=原生编码） | +12 u32 content_bytes（RGBA8恰为4×stored_w×stored_h） |
|---|---|---|---|---|---|---|---|---|

逻辑尺寸决定源矩形；stored尺寸决定实际纹理／UV，不能忽略padding或偷偷缩放。0编码表示保持生产结果字节与原管线，不做sRGB／预乘／反预乘转换；不声称所有原版图像都是同一种alpha语义。只读已初始化的有效储存范围，padding如被过滤／UV消费也必须有真实已定义内容，否则用recipe／具名格式处理，不能读取容量残留或补随机边。

**ResourceBlobChunk，头14B**：

| +0 u32 resource_epoch | +4 u16 index | +6 u32 offset | +10 u32 bytes | +14 u8 data[bytes] |
|---|---|---|---|---|

这是资源内容传送，不是另一个世界状态格式。一个资源号在run内永远绑定同一内容；修改产生新号，不在旧号覆盖像素。序号耗尽、内容版本／epoch溢出走明确会话／上下文恢复，不静默回绕。重连按当前引用闭包发送，不需要重放全部历史recipe或离房资源。发送内容版本只用整数，变化比较用自有规范缓冲的memcmp，不计算内容哈希。

第9步登记硬上限：路径4096B、单chunk≤256KiB、单像素内容≤16MiB且每边≤2048、recipe≤4096条、依赖深度≤16、同一待安装上下文自有资源内容≤32MiB；所有尺寸／数量乘加先检查溢出。它们是内存／消息保护，不是实测容量或已证明的性能。继承既有**首次准备≤1MiB、≤10秒、三端总持续7.5Mbps**目标；表项、recipe、压缩后blob、结构和baseline全部计入，不能把纹理改走Context便从统计里排除。大像素不压缩至预算内时改完整recipe／复用已就绪资源或报告实际未满足目标，不能删纹理、降尺寸、改预算或宣称完成。实际预算只在第15步验收。

资源抓取／安装均须保护本地图形状态：FBO、viewport／scissor、projection／transform、shader／blend／sampler、pack/unpack设置与PBO绑定、图形管理器镜像、图像strong和待消费batch。A4C840只恢复一部分，不足以作为任意位置可调用的证明。禁止后台持有Image*；图形主线程封存自有字节后，后台才做比较／压缩／发送。Client完成本地图像上传／recipe合成、依赖安装和CPU队列消费后才发布；失败释放候选，不破坏已展示的旧批次。旧资源必须等CPU队列／GPU引用不再使用后按本地owner正常退休。

## 5. 必要显示结果与本地变换

用**具名结果**而不是恢复整段原生Render。完整颜色仍是KColor20B或ColorMod44B，不能一律压成RGBA。以下新增布局均为设计，未分配生产编号：

| 记录 | 设计负载 | 来源／消费合同 |
|---|---|---|
| BackdropBinding | `u16 count + (u8 slot,u16 resource)[count]`，slot为固定登记的surface／图像用途，最多7项 | 正常Init／pre_render_walls完成后实际绑定；切换分支时显式替换集合。只为真正使用的内容保留资源，不按非Null候选全发 |
| BackdropExtent | 两个Vector，共16B | 已核+4F18/+4F20，和同批资源绑定提交；本地renderOffset／相机变换另加 |
| WaterVisual | `u8 present`；存在时`u8 mode,u16 image,f32 phase,Vector flow,WaterShaderResult`，共48B最大分支 | mode区分已核经典／反射世界模式；32B色结果取816F51..816F67之后，flow取同包+7460/+7464，phase取91AC70实际采用的有符号时间。既有字段复用，不再发第二份flow／reverse位／颜色 |
| RoomLightingResult | `u8 mask`；bit0附shadow_fill:KColor20B，bit1附light_tint:KColor20B，bit2附lighting_alpha:f32，bit3附特殊背景kind:u8；最大46B | 803E20的实际填色／混色点及804500的最终alpha消费者；资格区分world分支与本地图形开关。不得将缺源标成mask0 |
| RoomWaveResult | `u8 active_mask`低2位；每个活动位按A/B顺序附`Vector world_center,f32 radius_used,f32 gain_used`，1..33B | 804500在写off_BAB3F4对应顶点属性前的有效半帧radius和包络。未活动不读旧payload；速度、未来current／maximum不发。中心按8131D0已核投影在本端视口换算，不传Host归一化屏幕中心／纹理尺寸比例 |
| ForcedCameraFocus | `u8 present + Vector focus`，1／9B | 91E920实际override分支的有效最后焦点；不传ring历史。没有强制意图时各端按真实C/S身体选择维护本地镜头，不执行含设备／Online规则的Camera.Update |
| MinimapQuestionImageBinding（Presentation） | `u16 question_image`，2B | +614实际可空Image资源；965BDE加载／965C0C赋值、966680 Lost分支消费；不是图标Sprite帧 |

WaterVisual.present不是waterAmount的替代；水量、HasWater、mode、water shader／Grid clip有不同消费者。最终phase已经表达reversewatergfx时，Client不再按本地GameFrame反向一次。水面反射／整房中间light/shadow FBO是各端按自身视口渲染的场景资源，**不传Host最终整屏FBO**；从世界内容和本批参数在本端重建。Home特殊背景、特殊Effect166/sub951、GridLight、Door layer、镜面pass仍按原具名owner与顺序消费，不用普通背景覆盖。

phase_used只供91AC70的背景水纹用途；816BB0在817084写入的`2*game_frame`按同批时钟派生，不用这份有符号phase替换，也不新增第二份时钟。两种水绘制管线的图像／shader输入按用途绑定，不能仅因都叫water就把不同资源合并。

原生8134C0不仅取FX.shadowAlpha：有Backdrop57、pitch-black、有效curse、stage/type、425／460有效持有、Run两黑暗值、Mines维度、stage9的NPC407生命比和−9特殊房间距离分支。803E20还叠lightning、献祭房与Home。必须取**正常已产生结果**或把这些已核只读分支作为Host纯值投影完整实现，不能仅发shadowAlpha。Host未走图形分支时使用同world纯值投影补足世界结果，不额外调用原Getter、PlayerManager查询、最近玩家Getter或完整Render；任何依赖未核时保留具名Gap。这里是source实现合同，不声称缓存／所有纯投影代码已经存在。

FX沿用[Room核对](room-synchronization-review.md#5-fx水色光照的实际消费者)：实际层pass1..6、有序10B pose、实际图像／blend；poop取当前present／alpha；ray仅选中非空组，RayVisual10B与几何同批；vein十节点有界图＋两Sprite非覆写外观；rails取真实extent和有效字节。Client重复展示禁止ray.phase+=speed，临时Sprite／颜色／索引借用在CPU消费后恢复，不能恢复已被SetAnimation释放的旧数组。雨／Minecart／红门轮廓是实际Entity，不由显示adapter再生成。

Host的画面裁剪不是其他端的完整绘制目录。房间资源与世界空间FX需要覆盖当前world可显示的有效成员；捕获正常结果若仅覆盖Host视口，其他端新可见分支必须有已核纯值来源，不能把缺结果当透明。原版内容的必要显示尚缺具体生产点／ABI时仍是实现缺口，不为“无额外Render”而丢掉该内容。

## 6. 地图：完整事实，独立查看

**选择自有确定性地图合成adapter**，输入为L0/L1/L2/L3、RoomConfigVisual、两份边框／icons／itemIcons及独立question_image实际资源、世界规则和挑战32结果。question_image由Presentation资源绑定中的u16引用承接，默认文件图可复用已就绪资源；实际选择为空／替换仍须表达，不能硬编码一份文件掩盖Host内容变化。正常原生9635B0仅作为算法／正常结果来源，不调用Refresh／Update／UpdateVisibility，不伪造保存vector给它扫描，也不传Host已裁剪小地图FBO。两份config各自保留资源外观；位置、大小、展开alpha、缓存bounds、每房图标排版上限按本端同版本UI规则产生。

- 对当前维度所有**有显示资格**的描述取得L2完整OR/max聚合，不能只采Host当前小窗口、当前可见前N个图标或visited房间。L2原来的16类mask／动态图标帧不变；DisplayFlags0可不读隐藏保存体。远端展开只从这些已允许事实排版，不扩大地图知识，也不扫描未初始化容量。源预算不足报Gap，不截断后ACK。
- 不额外发送镜面／矿车两个原始查询bool：`748ED0/748F30`可由已有L0(stage,stageType,effective_curses)、Run difficulty／ascent-bit48及L1维度、配置Subtype纯值派生。两者共同先排Greed，stage1..6且ascent时排除；stageType需4/5；前者stage2或stage1+Labyrinth，后者stage4或stage3+Labyrinth。镜面图标另需Subtype34、visited且非dimension2；矿车需Subtype10及相应分支。不能从当前Level.stage直接当STB StageId。
- Home起始房间隐藏是**当前Data.StageId35且Type16**的world条件；NO_HUD种子及有效curse限制沿用Run／L0已有事实。Curse of the Lost的问号图、地图镜像、真实隐藏资格由同world具名规则处理。Host Options／HUDOffset／Map长按和显示alpha不能写入world资格。RoomTransition淡出属于Presentation的当前世界显示结果，不调用827350的整段转场生产来获取。
- 挑战32新增具名 **MapFocusResult**：

| +0 u8 present（0／1） | +1 i32 focus_safe_index（仅present1） | +5 u16 descriptor_member（仅present1；有效描述成员，非源槽／原ListIndex） |
|---|---|---|

  1／7B；外层绑定地图／成员／资源版本。present0只用于非挑战32或该world分支不适用；Client本地折叠不能让它缺席。来源为正常9635B0完成随机选择并确认实际采用的锚点／描述；同一楼层两config的缓存结果只有证明确实相同才共用，否则各留一份。Client按此锚点和本地窗口尺寸重新求裁剪范围，不复制Host窗口坐标，不按DecorationSeed再抽。原生钳到169不授权访问第169格；必须有实际描述关系，不能未经核定改成168掩盖。尚未产出或无法解析是Gap，不是present0。
- MapBadgeList沿用最多7项有序1—8B，位置／间隔本地；不得用itemIconsSprite最后一帧代替全列。world显示时钟／最终震动共用现有Presentation owner；shakeDuration不上线。本地禁用震动作为偏好可以不施加，不能改变权威offset缓存。
- 地图附带Time／Score显示使用Run现有TimeCounter／challenge／difficulty和**已经生产完成的总分缓存**，标签与字体本地；`9679FA`附近调用9BAC90重算分数含随机，必须跳过，不能因采用地图显示便恢复分数生产器。

地图资源本身的可变颜色、图层Image／shader／非覆写overlay继续沿用Sprite合同。轮廓／房间图标／L2每次强制选择的动画帧由具名adapter按已核顺序设置，不能把Host最后一次SetFrame残留当所有房间的姿态。临时借用／资源帧修改仍须与官方InterpolV2及CPU提交生命周期兼容。

## 7. 批次、变化与安全应用

本次不另建模拟tick或可靠事件重放。配置／surface内容走可靠资源上下文；必要显示结果是同一二进制状态模型中的具名投影。已核固定记录由生成器注册，baseline仍base=0，增量比较同一规范字节。

1. Host主线程在已理解的正常生产窗口复制结构、显示值和资源绑定；对正常绘制才产生的栈结果，立即封存其world state revision、presentation revision、member/resource epoch、owner/use/shape及pass／调用身份。GameFrame相同不能代替这些依赖；普通后态不能补一份早先绘制栈值。
2. 后台只接收自有不可变字节／recipe／像素；每Peer按已确认版本发差分。资源变更只增加新内容项，先发送依赖，再发引用它的状态；内容相同／Host seed或权重变化不重发资源。客户端查看输入不会产生Host world更新。
3. Client完成解码边界、配置／资源闭包、所有必要结果及结构依赖检查后，准备本地候选资源／地图缓存／FX。world几何、描述拓扑、L2、焦点和相应显示批次共同提交；body仍遵循已有窄拓扑依赖，不把它变成全网等待。
4. 任何Wait／Gap／不完整结果都不伪装Null、空mask或空列表；不ACK依赖未就绪的新目标。可以继续展示同一world已完整提交的旧批次；旧room退役后不能贴到新world。Authority不等待显示、资源传送或所有Client ACK。
5. 切房／换层、Maze交换、红房／维度、双配置替换、Hourglass／Ascent／合法Continue、资源写入或owner/use退休触发对应源失效／新事务；不能只看指针、size、visited或clear计数。资源引用回收晚于所有在途／已提交队列，不由网络裸free原生对象。

**不可执行的网络含义**包括Save／Restore／Init／Spawn、SetBackdropType、ShowMap／Apply*／Refresh、Camera.Update、82EE90、补货／取随机房间、分数／HUD重算、Set／Unload RoomConfigStage及任何存档／统计／解锁操作。它们都不是这些值记录的消费实现。Hook按统一清单常驻、逻辑启停、非激活透传，正常Host original exactly-once；后台不碰游戏对象或Lua，C/S不进入原版Online／Isaac Sync。

## 8. 后续实施、验收与剩余未知

| 步骤 | 本轮已明确的实施输入 |
|---|---|
| 7／8生产布局 | 注册RoomConfigVisual、实际资源绑定／extent、WaterVisual、lighting／wave／focus；地图由L0—L3及Presentation资源和MapFocusResult／MapBadgeList承接，不新增保存体布局 |
| 9 | 资源表typed payload、不可变内容依赖／blob分块、明确预算和结果布局；未注册前版本56不接收这些新记录 |
| 10／11 | 正常生产缓存／只读纯值投影、图形资源物化、自有缓冲比较与按Peer资源闭包；禁止为补源重新随机／额外Host Render |
| 12／13 | 资源等待与失败、typed配置承载、纯地图／surface合成、水光波纹及本地视口adapter、原生／图形owner的构造与完整恢复；不放行混合函数 |
| 14 | 退出旧完整配置JSON、Spawn／Merged递归采集、背景RNG轨迹、原始水色时间线、Host地图输入镜像／旧窄地图builder及通用查询补丁的正式下行；历史观测资料保持 |
| 15（唯一测试） | 双配置同名不同内容／原位改变；Home StageId35与Type16组合／Door Light；十二房形／L形／八门／特殊背景；冷连接错过生产／程序图同名／padding／RGBA8／资源失败；水色／方向／flow／反射／本地不同图形选项；光照425／460／NPC407／特殊−9／Home；两波纹半帧／重复显示；完整可见地图、不同窗口／展开／curse／挑战32／资源边框／徽标；预算／重连／退休／无额外随机和访客持久副作用 |

设计选择和同步边界已补齐；**具体Hook缓存、shader／recipe的正式注册、GPU格式与上传leaf ABI、正常生产写入的完整追踪、运行reader／writer、生产布局和性能尚未实现／验收**。原版必要分支不能因这些实现项未完成就划为“不用同步”。现有Room直属28B、Level22B、Minimap补核后15B、RoomSet／RoomSpawn各3B及嵌套ANM2／BackdropConfig／保存体／官方私有旁表未知仍如实保留；本轮没有宣称第7步整体实现或整个世界字节语义完成。
