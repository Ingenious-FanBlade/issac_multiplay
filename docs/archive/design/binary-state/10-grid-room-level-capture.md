# 第10步第四项续：Grid与Room／Level原生来源

2026-11-11。接续Entity／Player来源，完成Grid及Room／Level同窗采集接线；第15步为唯一统一整体验收，本轮不运行测试。具体实施和未决以文末实际记录为准。

## 2026-11-12继续闭合：版本69

按本次用户决定，水面／波纹的纯表现由Client本地推进，不要求逐帧画面相等。Host继续提供水量、水流、水色、实际资源，以及波纹世界中心、当前半径、强度、速度和计时锚点；Client只推进显示，不执行Room／Entity玩法Update、伤害、碰撞、Spawn、随机事件或持久写入。经典／反射水面成为Client本地画质选择，删除286.mode的必发要求。其余玩法权威关系不变。

本轮补入了此前缺少的正常背景生产来源、实际用途绑定和完整光照投影，Room／Level当前已登记world根的Host来源接线已实现。这里的“实现”指产品代码和来源路径，不代表第15步验收通过，也不代表第11步发布器或第13步Client显示器已接通。下方版本68“全域未完成”保留为真实历史，不再代表版本69当前实施状态。

### 波纹与水面

- 289改为每波显示锚点：world_center、radius_used、strength、radius_step、elapsed、duration；活动波必须满足`0<=elapsed<duration<=INT32_MAX`。Client以同批GameFrame和本地显示相位计算半径、包络和Idle帧，暂停／重复展示／退休不得推进玩法。Host不再逐波计算pow或捕获Render临时主帧。
- 新增434 RoomWaveSprite：实际文件、Idle动画资源、稳定外观、图层及独立overlay；删除共享main_state、默认选择和未来playback_speed的读取。NamedAnimation遍历已加载的316B AnimationData目录，不执行Getter／SetAnimation／SetFrame。7E7670在Backdrop18／28正常加载`gfx/Effect_SatanRadial.anm2`，与图形选项分开。
- 286保留真实water图像、带方向的相位锚点、flow和最终32B水色结果。经典／反射和局部动画由Client纯显示owner处理；原FXParams.use_water_v2的静态来源事实仍保留，但不再形成必发mode或Host Options依赖。

### 光照

`scene_lighting.cpp`在当前Room根内只计算一次803E20／8134C0的纯值投影：shadow_fill、light_tint、lighting_alpha、lightning及固定特殊背景pass位（bit0 Hush、bit1 Home、bit2镜面维度，允许组合）。保留pitch-black、有效curse、首层倍率、Mines／维度、Home／Hush、闪电、Boss407 HP及stage9特殊−9最近玩家距离。颜色保留原空间标签，输出tint的标签按原生写0；实际绘制和各端FBO仍归Client。

425／460的有效持有覆盖原版7699D0／9950B0的相关路径：官方既有blocked／innate、variant1转本体、状态抑制、共享双身体、wisp计数、临时选择、额外计数、四个active槽、191派生选择、488效果选择和真实收藏计数。原版固定角色／变换和局部随机候选表不含425／460／191，不执行这些无关查询或RNG。伴随玩家／NPC必须在同一Collector窗口具备当前初始化资格；树／容器有界，读回证言统一复核。第三方角色／rework仍按现行规范留后续，不据此声称支持第三方查询语义。

### 背景正常生产与有限recipe

`scene_production.cpp`只观察原本发生的背景生产。既有Backdrop LoadGraphics／pre_render_walls wrapper建立主线程借用栈；另外在当前Room所拥有的四张离屏图正常成为target时覆盖后续辅助surface写入。输入图像在实际调用栈内立即转为不可变路径／几何或已封存程序资源，后台不借用Image／batch／顶点地址。

- J273 **9E2460**是四族Image Render共用leaf。9E2170只是其中一个重载，不能只Hook它；当前接共同leaf，覆盖普通图、ANM2及实心填色。源坐标已转换为UV，保留实际目标quad、四KColor、blend／sampler及必要ColorMod／ColorOffset上下文。
- 同步栈短借最后quad的顶点区，在下一次绘制、切target、Push／Restore或批次提交**之前**读取调用者补写的属性，随即转纯值。核定28／36／88B属性布局及实际batch归属；不将原生顶点缓冲当作网络格式。Clear中的内部Present使用嵌套作用域，不提前丢掉Clear的生产记录。
- 观察真实离屏Clear，初始操作必须明确清理；每个程序资源最多4096操作。Shape Renderer由A2AB60正常以白色创建，使用本地固定白色Clear recipe承接实心图元，不传像素或创建下载路径。
- `g_AllShaders[AACAB0][0]=BAB44C`是公开ColorOffset。BAE1D8为普通贴图基础shader；28／36B绘制只以单位ColorMod等价投影到既有有限ColorOffset合同，不把它误登记成另一个公开shader号。其它未知shader／属性仍失败。
- recipe、输入图和配置目录只追加，资源依赖引用自有handle。相同surface重写产生新的不可变内容；追加已有surface时从已封存描述恢复操作，禁止目标自引用。原生shader、GL句柄、RNG和纹理字节均不下发。
- 固定recipe允许坐标在本surface尺寸的`[-2*extent,3*extent]`有界范围内跨边缘，本地FBO负责裁剪；旧“所有点必须在surface内”会拒绝原生跨边tile，现已修正。SourceQuad仍明确UV空间，逻辑／padding尺寸独立保留。

用途绑定依据918A60／9191F0／919030，而非枚举非Null候选：floor／wall按当前实际分支，`Backdrop+0x20`为追加floorSurface2 pass，water按世界水量和实际图像；gfx／nlfloor和temp作为生产输入或scratch，由recipe依赖承接。必要floor Sprite按18／26／35／52—54分支提供；墙细节已烘入内容，教程／按钮由本端C/S绑定显示，不发送Host设备选择。此前将+0x20描述成“surface2替换selector”的说法在此纠正。

### 边界与后续

正常生产历史必须已被观察，或之后出现一次覆盖所需内容的正常生产。中途才启用采集而错过当前程序图历史时，仍明确ResourcePending，等待正常生产恢复；不额外运行Init／Render、读取纹理或伪造空资源。Host已采集后的Peer冷连接／重连可复用自有不可变目录，不必重跑Host生产。缺输入、未知格式、预算耗尽、异常、资源关系改变均撤销候选，Authority tick不等待。

第11步需要把`NativeSurfaceOperation`中的自有资源handle转换为Peer编号并编码既有SurfaceRecipe；第13步实现其本地物化，以及水面／波纹纯显示推进。Run／Presentation的地图焦点、徽标／问号和音频仍由对应后续项完成，不在Room／Level重复维护。完整联机可玩性、长期资源容量、特殊分支、画质差异和性能留第15步统一验收。

纯产品编译曾先后发现新434继承了不适用的Sprite成员映射／缺布尔validator，以及新增自有leaf未登记静态特征；已修正。统一Hook清单当前198／208槽。最终`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`纯编译／链接成功，生成`out/repentogon/zhlIsaacCS.dll`。没有部署、启动游戏、运行任何测试或校验哈希。

收束时还按生成表纠正了288的字段序号；伴随Player读取另核真实197 RTTI；variant1在原生super内部转向本体时不重复套用本体spoof，而真正的递归HasCollectible调用仍按官方wrapper处理。同时保留Darkness curse绕过首层0.6倍率的原生分支，特殊主题与镜面按独立pass位组合，避免互斥kind漏显示。以上为静态核对与实现修正，未将编译成功写成玩法或性能通过。

## 编码前列出的失败方式

- 按Desc.Type猜动态类，Grid无完整Init／owner证言仍读，门别名遗漏，index与真实房间成员不一致。
- 官方旁表模块／ABI绑定错误，未设置optional读取旧payload，Lock错误应用共享覆盖，同index不同实例混成一个结果。
- Rock碎块态读取被Render覆写的主姿态／layer0位置，非活动碎块仍读旧字符串；Fire读无效粒子前缀、粒子速度或第三Sprite残帧。
- 无RTTI的Room／Level冒充虚表对象，伴随／协议结果偏移直接加到根，隐藏地图保存体仍被扫描。
- 把ListIndex当物理source-slot，将-19、-100／-101当数组索引，把527个存储槽全当普通活动描述。
- Data／OverrideData混成一份配置，配置窄化溢出、资源原位改变却复用旧描述，初始化中／维表变化仍提交。
- 有效诅咒缺Seeds／temporary分支，非Greed抹掉Boss匹配，Error投影推进游戏RNG。
- 保存图标遗漏过滤／default分支／Card类型，截断源扫描，按Host排版前缀裁掉其它端需要的图标。
- 水色再次插值或乘水量，wave／ray混用前后相位，FX隐藏分支读残留、Vein环或外部节点被静默剪枝。
- 背景程序图只用名字当内容，缺实际绑定／正常生产来源却伪空；为了补源额外执行Render／Init／Getter／Lua。
- 根／成员／资源变化后旧候选仍可消费，Clock提交前交付，主线程分配／锁／等待，后台借用游戏内存。

## 实施记录（版本68历史时点）

布局版本68。Grid公共部分和全部17派生的原生来源已接入；Room／Level的结构、L0—L3及下列已核只读投影已实现。**Room／Level全域尚未完成**：背景程序内容／pass、完整光照与水面模式，以及特殊波纹Sprite仍缺正常生产来源。当前强制全域候选会在这些必需位置失败，不发布不完整WorldFrame，不把已编写的原生绑定写成全域完成。

### Grid

`grid_source.cpp`使用冷准备的真实RTTI映射，新增所有Grid继承根及递归布局的计划绑定。桥从真实448格和八门枚举，用初始化证言、当前initialized byte、身份/use、index与真实槽／门索引表共同核资格；相同对象只采一次，各门引用保留别名。

官方旁表只绑定已安装模块：J273进程中的`zhlREPENTOGON.dll`，核对`261C90`的对象+24／448范围／12B寻址和实际表立即数，以及`261C00`的本地vptr写入关系；不执行helper、不校验哈希。读取时再核当前slot的已构造vptr，先读engaged，设置时才读u32。六族分别合成默认值，Lock只读本对象collision而跳过旁表。没有修改官方旁表或Host世界。

Glitch在栈上按J273 shift35的5／9／7做两次顺序xorshift，保留两个最终低位。Rock state2不读主姿态及layer0位置，从实际文件的有界AnimationData目录解析RubbleAnim；非碎块态／未加载不读旧字符串或vector。Fire只在有符号state<4读取有效粒子，保持原生24B步长、16B投影及0—5／寿命约束；第三Sprite不恢复已删除的主帧。Door alpha先核有限性再给正值谓词，light只发0x20投影。

### Room／Level结构与已核投影

`BinaryEntities`现有桥增加独立原生结构快照，不使用Lua层级缓存或Recorder字段完整标志。Room initialized、Level初始化中标记、当前Room／descriptor、实际width×height、当前维表别名、完整456指针、普通描述数量和全部合法离图槽都在同一ordinary窗口读取。准备尾和Clock提交前再次复核；-19／物理525不读，负路由不当数组下标。三维507项严格转换为活动物理source-slot或FFFF；Data／OverrideData分别生成15B配置资源，资源owner身份与内容相等分开。

新增`binary_level_owner`常驻Hook，复用官方`Level::Init(bool)`的J273入口`73EB10`和官方ArgData。整条Init链进入即撤旧floor候选，嵌套合并，异常不恢复资格。初次合法普通窗口只建立当前观察身份，不补造出生。floor、三维身份及当前room由原生结构owner提供；描述physical slot、活动order随packet引用快照复制给worker，后台不按ListIndex重新猜身份。源／字段失败仍回卷整包。

L0有效curse只读现有Seeds树、raw／temporary和明确条件，Greed wave非Greed规范零，Boss匹配保留所有模式。L1保留两配置、visited布尔、五位显示flags及Greed宝物房图标；D1按官方u32式计算是否ModelingClay166，不推进游戏RNG。L2只对DisplayFlags非零描述扫描两个合法vector；按78h／20h步长读取具体地图分支，保留16类OR／max、Slot default、CardConfig type、Portal与楼梯。每描述只聚合一次；每集合16384和整个窗口262144源元素为明确扫描保护，超额失败，不截断。隐藏保存体、商店表、ESSM与其它未知不读。

Room直接颜色／时钟／几何、FX实际有序层及有效pose前缀、选中非空ray组、poop当前alpha／图像／颜色、十节点有界vein图与两Sprite、有效轨道字节、地狱背景和强制焦点已实现。Vein所有child必须属于本数组且无环，不剪枝；Camera只取override及有效ring最后坐标，不调用Update或Online查询。水色按816BB0逐f32复现默认／seed79／插值及一次水量乘色，保留KColor空间标签；phase读取真实所选BackdropConfig+139，flow唯一来源保持。两波纹保留当前world中心、半帧radius及`1-pow(current/maximum,0.4f)`包络，不发视口或未来计数。

### 八门关系合同与安全复核

Room八门原为16B纯Convert，无法保留新Grid尚待分配Peer序号的关系。版本68改为`reference/u16[8]/grid`，长度仍16B；生成器、采集link事务及正文观察器支持定长引用数组，每个引用独立进入依赖闭包。所有旧接收者由整数布局版本拒绝，不退回旧格式。Root范围及桥结构读取的地址加法用u64检查；所有tracked头／holder／字段在根完成时再复读，任何变化使整个writer失败。Sprite依赖增加身份空间，切房撤退休来源；自有配置身份、暂存、交接和引用尺寸继续计入共享额度，没有运行期扩容。

### 静态核对得到的实际未完成来源

- `240.binding`、`240.sprite_mask`／相关Sprite：正常背景各pass的真实用途及程序内容还未封存。当前不能把七个非Null smart pointer或文件名当作完整内容。`917DC0`还能对同一floor Sprite临时切Idle2、绘制、再恢复Idle1，普通后态不能代表全部贡献。
- `286.mode`及`288`完整lighting：不能用Host Options来决定其它端world资格。`8134C0`实际依赖425／460的完整有效持有、黑暗／curse、Mines和Boss407等；`7699D0`还包含多种派生有效持有路径。不能只读collectible数组、只发shadowAlpha或把未知折mask0。
- `239.wave_sprite_present`的真实特殊分支：`804500`在Backdrop18／28、未clear且波纹活动时，逐波调用SetAnimation("Idle")／SetFrame并临时绘制同一Sprite。完整通用主姿态是绘制scratch，当前对此分支明确失败；须接逐次正常贡献或补齐具名专用布局及其实际动画／帧来源。
- Presentation唯一owner的挑战32两份已选地图焦点、徽标及问号资源仍属正常结果来源项；本轮L0—L3不额外随机／Render来补它们。Run／HUD／音频及完整发布仍按第10步其余项、第11—14步推进。

以上来源必须补齐后才能称Room／Level全域完成。没有为通过率删除字段、补零、传像素、重跑Render／Init／Getter／Lua或调用随机。

### 编译和验收边界

Release／Win32产品目标`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`最终纯编译／链接成功，E2E工具为OFF。初次生成实际失败为生成器只接受单个u16引用，随后补齐定长引用数组支持；新增代码的C4458遮蔽警告已修正。唯一Hook清单本轮为191／192槽；没有安装、部署或启动游戏，也没有运行单元、隔离、smoke、E2E或其它测试，不校验哈希。功能、长期容量、全部分支、时序和p95／p99留第15步统一整体验收，不能据纯编译声明通过。
