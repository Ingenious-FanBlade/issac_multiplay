# 第9步第四项：资源协议

**2026-11-12 显示委托修订**：

- 只保留实体、Grid、costume 等资源引用。
- 本地 recipe 和 shader／blend／sampler 规则不再下发，由 Client 原生代码本地加载。
- “不传原始媒体”的原则不变。
- 见[修改方案](client-display-delegation.md)第 7、8 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-11 第9步收尾，版本65**：[预算合同](9-budget-capacity.md)已核资源／recipe及完整Context成本；32MiB完整消息含40B信封，ResourceTable正文上限33554392B。目录仍同run只增不改、65535项／32MiB／131072边；实际物化和共享池owner在后续接线。模组额度现9Mbps，不启用媒体blob。

**2026-11-11 第五项交接，版本64**：[接受与恢复合同](9-acceptance-recovery.md)保持普通Resync／换route不自动清空同run目录。仅在旧复制route排空退休后，RebaseStateResourceEpoch以新route的epoch=1全量重新绑定，并逐项保持全部已知资源号／kind／精确payload；它不回收编号，也不允许在旧route重置。资源物化仍由第13步真实owner完成。

承接[公共合同](9-public-wire-contract.md)、[显示封装](9-presentation-envelope.md)与[本地生成范围](local-media-presentation.md)。本项完成本地资源typed payload、table/kind分派、有限recipe／图形规则、资源表编解码、不可变目录合并及依赖图；不启用像素、文件内容、PCM或通用blob。实际来源与原生物化仍由10／13步接线，第15步统一验收。

**2026-11-11 实现，版本63**：产品库新增[`resource_protocol.h`](../../../mod/src/state/resource_protocol.h)／[`.cpp`](../../../mod/src/state/resource_protocol.cpp)，数字矩阵、布局和图形规则来自`wire.json`及[生成合同](layouts/09-wire-format/resource-protocol-plan.md)。新增425—433九种自有值，当前349布局／2806字段／85条件；304／公共头尺寸保持。309／366补齐固定ColorOffset提交上下文，分别为212／213B，双方必须按63协商。

## 编写前列出的失败方式

- 把资源头存在当作完整payload；跨表解释相同index，接收未知kind、像素或blob。
- 路径越出本地资源根、包含控制字符／NUL／空组件／点组件，或把媒体字节伪装成路径。
- ANM2文件、动画、实际Image与待加载路径混用；以当前实体类型重新猜选中配置，音频只发soundID而再次抽sample。
- shader类型当函数／地址，任意shader参数字典或GPU命令；blend／sampler编号无定义／用途错配。
- recipe无初始完整清理、操作错长、无序或越界，读未定义旧surface；自引用／成环／依赖缺失被当ready。
- 同run原号覆盖／回收、epoch冲突／倒退、full快照遗忘旧内容、恢复复用低编号；半个新表修改已提交世界。
- 数量／乘加／深度／字段访问／整表预算溢出，错误借用scratch或输入输出重叠。
- 表收到、依赖闭包成立与本地物化ready混为一谈，提前ACK或资源失败回退下载原始媒体。

## 资料与静态补证

先读取本地Wiki的renderer/Image、BlendMode、ShaderType／BlendType／BlendFactor／BlendEquation、ItemConfigItem及既有音频来源，再查官方SDK的AnimationData、ItemConfig_Item、EntityConfig_Entity、SoundEffects／MusicEntry、Image与BlendMode及ImageUtils。SDK未给出sampler枚举的具体GPU映射，随后静态读取目标J273：407DA0向Image虚槽64／68传四项filter／wrap；ImagePng虚表B3BA80指向A4C590／A4C600。A4C590的零／非零分别转2600h／2601h（nearest／linear），A4C600分别转2901h／812Fh（repeat／clamp-to-edge）。这些是静态指令事实，未运行游戏或实验。

另按官方BlendMode.zhl特征定位9E17B0，确认NORMAL／ADDITIVE在BAE1B3为0时仅RGB source改为6，alpha source仍为1；因此登记两个非预乘profile。官方`LuaBlendMode.cpp::modes[5]`提供前五种确定值，Wiki另明确Mother背景与Light两组非标准值。不能直接用GetMode(3／4)代替这些显式值：目标J273该函数只处理0—2，3／4是官方Lua接口的自有常量表。协议不执行Lua或该原生函数。

## 正式payload与矩阵

| table／kind | typed值 | 当前语义 |
|---|---|---|
| 2／0 | ResourcePath425：u16长度＋≤4096B UTF-8 | 已确认正常加载的ANM2文件VFS路径，gfx/*.anm2 |
| 1／1 | ResourceImageFile426：425＋四个u16尺寸 | 实际文件图像与逻辑／stored尺寸；gfx/*.png或*.pcx，不以待加载路径替换仍使用的旧图 |
| 6／2 | ResourceBuiltinShader427：u8 type | 官方0—19内置类型，不传shader路径、地址、stride或任意参数字典 |
| 5／3 | 既有RoomConfig_Room263显示投影15B | 保留实际Data／OverrideData选择；不包含候选生成、权重、保存体或原生对象 |
| 1／4 | SurfaceRecipeHeader307＋有序308／309 | 完整本地有限合成描述；见下节 |
| 0／6 | ResourceAnimation428 | 文件资源号＋ResourceName429＋u32长度＋有序layer／null-layer元数据列表；430每项为u16 ID＋u32 frame_count |
| 3／7 | ResourceItemConfig431，16B | 实际(type,id,cooldown)及可空图像／costume文件资源；不复制库存、解锁资格或配置指针 |
| 4／8 | ResourceEntityConfig432，22B | 实际选中配置的type／variant／subtype、可空ANM2、portrait、shadow_size；不按Entity当前类型重新猜配置 |
| 7／9 | ResourceAudio433 | audio_class、has_intro、实际main路径、条件intro路径；选择后的文件／音乐声部，不是再次随机选sample的sound ID |

kind5／305／306及Kind24正文继续禁用，无下载或通用媒体分块入口。文件内容、纹理、PCM、视频音轨都不进入payload。相同本地路径可服务不同音频类别，类别不由文件夹推断；只允许music/或sfx/下的ogg／wav／mp3描述。intro仅用于音乐／jingle；voice的volume_class／segment必须和资源类别及intro存在性一致。

路径保留精确Unicode文字与大小写，根名采用上述规范VFS命名。拒绝空路径、非法UTF-8、NUL／C0／C1、绝对／设备／URL路径、反斜线、空组件、点组件、尾空格／点和Windows保留设备组件。Host第10步必须根据已确认的官方加载来源转换为规范路径，不能从Image.name或资源缺失猜路径。名字429沿用严格文字规则但不执行命令；长度／frame_count为0可表达合法空动画／空layer，不能用0冒充未读取。动画实际顺序、独立层与null-layer不能合并，重复ID拒绝，本地ready还须核对已安装文件的真实元数据。

配置描述是本地资源解析及必要显示投影，**不是完整原生配置内存的网络镜像**。第13步必须解析准确的已安装原版配置／构造具名自有显示承载，并核对描述；没有准确本地对应或原版程序资源来源仍为具体Gap，不从availability、生成器或当前Entity的字段回查另一个对象，也不宣称原生字节UNKNOWN已解决。

## Recipe与固定图形规则

307头仍为16B，recipe_layout=1，最多4096项，stored尺寸≤2048且逻辑尺寸不超过stored。必须以308的21B完整Clear开始，之后只接受Clear或309的212B Quad，精确消费全部字节；不继承未声明的旧surface。Quad目标点须在本surface内；输入可以使用已登记wrap，保留SourceQuad的pixel／UV空间、每顶点KColor及ColorMod全部分量。

309与366现增加`pixelation_amount:f32＋clip_plane:3f32`。这16B来自官方ShaderUtils::ColorOffset::FillVertices的真实固定输入；TextureSize由引用图像的padded尺寸派生。Client不借用自己当时的全局clip／pixelation，也不把ColorMod字典当作任意shader参数。Quad的image／shader必须非Null，缺资源不是默认空。

九种blend profile的编号为本协议自有值：前五种CONSTANT／NORMAL／ADDITIVE／MULTIPLICATIVE／OVERLAY，5／6为Mother背景／Light，7／8为非预乘NORMAL／ADDITIVE。精确五元组同源生成，不把Host GL整数直接透传。sampler_view的四位依次为min linear、mag linear、S clamp、T clamp，0位语义为nearest或repeat，穷尽16个固定组合。原生LayerState已保留的u32原值不因此改成新的Host GL枚举，实际零／非零语义由已核adapter转换。

内置shader的20种身份均可声明；固定用途另有精确允许集。SurfaceQuad／PresentationImage只接受具有完整已登记输入的ColorOffset，Sprite、屏幕、Room water／wave等各自用途按[数字合同](layouts/09-wire-format/resource-protocol-plan.md)检查，不能给通用Image quad指定任意其他shader。已有未知的特定shader消费者／来源仍保留具体Gap，不用默认ColorOffset隐藏，也不将本协议登记等同原生adapter已经实现。

## 表、目录与生命周期

ResourceTable正文为`408公共头50B＋412表头10B＋entry_count个304头8B及typed payload`。没有第二层count、12B成员头、重复layout tag或媒体块。既有Kind23与可靠Context承运职责保持，本项不提前修改运行中的KnownKind／分派。整表编码先校验全部自有输入及重叠／预算；解码必须完整收齐、核布局版本／完整scope并恰好消费，表项按(table,index)严格升序。

资源目录跨scene复用只限同session/run，消息仍服务公共头指定的完整目标scope。相同key的kind／精确payload永久不可改，变化分配新index；同表新号必须超过已知最大号，FFFF不占项。full快照previous=0须包含并保持全部既知项；增量须从准确previous_epoch构建。epoch只前进：同epoch增量须与原始previous／更新精确一致，恢复／新scene重发full时则须与已知完整快照完全相同，不能因最初是delta而拒绝合法full表达。冲突或倒退拒绝；普通Resync／换route不重置同run目录或回收编号。

合并只写新的调用者存储，保留旧不可变目录与已提交画面；部分失败不发布。返回的entry／payload／update均为借用span，owner必须延长寿命，不能覆盖旧缓存仍引用的接收缓冲。达到u16总表项容量、编号或epoch边界须明确失败／处理合法命名空间生命周期，不能静默复用；目前完整快照的所有表合计最多65535项。

## 接口、闭包与ready

| 接口 | 工作 |
|---|---|
| Encode／DecodeResourceEntryPrefix | 单项固定头及typed payload，返回精确消费长度 |
| Encode／DecodeResourceTable | 完整资源消息，核scope、整数版本、表头、顺序与整表预算 |
| MergeResourceTableInto／FindResource | 构建不可变目录、同epoch重传、原号冲突、单调分配与精确查找 |
| BuildResourceGraph | 目录epoch变化时一次解码真实链接并生成数字边；检查整个已知目录的环与最长依赖路径 |
| PrepareResourceClosure | 复用图计划，仅准备本目标roots的闭包；不按帧反复解码全目录或让无关资源扩大body等待 |
| ValidateResourceGraph | 冷路径的构图＋准备组合接口 |
| ValidateResourceBindings | 检查正式值／选中块中的实际资源引用、固定图形用途及音频类别／段落；不代替投影／base／完整Frame校验 |
| ResourceBodyContext | 从精确资源快照提供704等正文条件需要的(type,id,cooldown)，不读取原生指针 |

闭包来自真实typed资源字段，recipe中每次image／shader引用同样计边。缺声明返回未闭合且不给可执行物化顺序；自引用／环／超过16层依赖明确失败。最长路径同时记录子图高度，不能借先访问过的子图绕过深度限制。计划绑定目录scope／epoch及自有存储；覆盖或退休后不得复用。

**表收到、闭包成立、本地物化完成是不同状态。** `ResourceReadyContext`只读报告实际已安装／生成且与描述匹配的资源；所有必要依赖ready后才可作为目标资源条件。纯值图计划与检查函数不读写GPU／音频／原生对象、不调用Lua／玩法生产器，不提交影子、不发AppliedAck。首次加载、GPU／音频线程、缓存预算与安全leaf继续由10／13步实现。

整目录编码保护32MiB、总项≤65535、边≤131072、依赖深度16、调用者scratch与typed字段／标量元素访问预算同时生效，默认访问262144次。路径≤4096B、名字≤2048B、layer／null-layer各≤256项；几何乘加在宽整数下检查，不把尺寸窄化或由声明直接分配未经预算的本地纹理。目录保护不是GPU驻留承诺，也不是首次准备或常态带宽额度；首次准备全部消息／状态仍≤1MiB／10秒，Host三端10Mbps／模组持续7.5Mbps及每通道实际分片计费由第11步落实，第15步验收。

## 完成方式与边界

普通同源生成与Release／Win32 `isaac_cs_state`纯编译完成，E2E工具关闭。首次生成依次发现366增加字段后预算未增加、合成容器stride／storage不合现有规则、生成器尚不支持自有协议值内嵌、intro条件缺具名语义，均已修正并重新生成；没有把这些生成失败改写为测试通过。静态复核补齐目录只增不改、精确重传、最长路径、按目标闭包、图计划复用、voice类别／段落、输入输出重叠和主体资源检查。

未运行游戏、单元测试、E2E、probe或独立校验脚本，未安装到游戏目录。第9步仍需完整StateFrame／成员表／控制报文装配、最终会话接受与预算核算；10—14步继续提供实际来源、Peer发布、完整候选、原生资源／显示应用及旧链退休。已有资源来源、程序资源、字体映射、全部音乐包络／seek／重连相位及其他UNKNOWN不变，第15步统一整体验收尚未开始。
