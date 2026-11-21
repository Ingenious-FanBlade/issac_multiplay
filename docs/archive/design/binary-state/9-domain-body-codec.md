# 第9步：各域正文编码与解码

**2026-11-11 第9步收尾，版本65**：[完整成本与容量](9-budget-capacity.md)已完成。低层值／record的预算不包含ICSR信封，装入完整Frame后正文限65496／33554392B并再扣各外层／其他记录；不得直接把底层32MiB当实际发送额度。带宽现为模组9Mbps，运行接线及第15步验收仍待后续。

**2026-11-11 第五项交接，版本64**：[完整报文与接受合同](9-acceptance-recovery.md)现承接本层块／投影结构；先检查整帧定界与base，再解码正文。BodyView仍不证明完整候选、实际物化或原生成功；只有第12步完整owner检查通过且第13步共同提交成功才ACK。

**2026-11-11 第四项扩展，版本63**：[资源协议](9-resource-protocol.md)新增425—433本地资源值，349布局／2806字段／85条件；生成器支持纯C/S协议值的显式内嵌。309／366补ColorOffset的pixelation与clip plane共16B，image／shader非Null、有限blend／sampler同源校验；公共头与禁用媒体规则保持。完整Frame和运行链仍待后续。

**2026-11-11 第三项扩展，版本62**：[显示批次外层](9-presentation-envelope.md)复用本正文codec；新增可借用的真实节点observer及实际字段／标量元素访问计数，便于确认条件／有序元素贡献和实际链接。381的audio asset明确非Null，28条外层用途规则同源生成；仍340布局／2772字段／84条件，版本61文字／本地历史及禁用媒体规则保持。版本61的下列实施记录保留其时点；整消息／资源与运行接线仍待后续。

**2026-11-11 按修订合同迁移，版本61**：[本地生成合同](local-media-presentation.md)已落实到本项唯一源与正文编解码：横幅／fortune／共享Prompt改为有界文字和有限样式；hallucination移除Host图像引用，叠加仅传有序颜色描述；374收窄为独立Boss versus，普通切房不再有surface结果。305／306保留固定字节目录但正文入口拒绝，304拒绝像素kind5。实际采集与本地生成仍待10—13步，不表示运行路线已接通。版本60的原实现事实保留在提交历史。

2026-11-11。本项承接[公共报文合同](9-public-wire-contract.md)，实现基础值、Entity及变体、Player、Grid、Room／Level、Run、Presentation／Random和公共固定头的同源正文编解码。不调用Lua、不读游戏对象、不建立JSON树；实际采集、Peer发布、完整影子目标和原生应用继续由10—13步负责。

## 编写前列出的失败方式

- 把原生STL、指针、holder或UNKNOWN直接当正文；把字段号、偏移或函数作为网络输入。
- 标量宽度、字节序或浮点位模式不一致；NaN／Inf、坏bool、枚举、mask、可空覆盖或计数器模式漏检。
- 把有符号ID当无符号排序；整数范围先转double导致u64边界丢精度。
- 容器count窄化、乘加溢出、超元素／字段／记录／正文预算；深层嵌套和大量零长元素消耗无界CPU。
- 固定槽额外加count；u8计数误读成u16；字符串附NUL或Text允许内嵌NUL。
- 条件不成立仍读取旧payload；复用另一个嵌套元素的selector；704效果只看数字ID、不按资源配置type/id确定分支。
- 嵌套投影、共享owner头尾及容器多投影错配；不知道共享presence便猜空；缺少旧base投影时提前声称完整。
- Weapon存在mask高位、view_layout或type不匹配；按相同长度回退未知布局。
- ordered-map／set不规范排序、重复键；有序History／贡献列表却被擅自排序。
- 一条记录越界、部分编码或解码失败后仍发布候选；零长预留块被发送，world/body块混用。
- 解码成功被当资源ready、生命周期合法或原生提交成功，提前ACK；跨scope/use/shape合并投影事实。
- 文字的字节数／UTF-16单元数混用，错误代理项／过长UTF-8／NUL或控制码被替换、截断或作为命令；把本域规则误用到EntityText原始字节串。
- 字体／样式编号越界、未知首码被无声删除；有效空串、隐藏槽与来源失败混为一谈，fortune行或Prompt五槽被排序／合并。
- 只删除资源声明，却留下Host文字图／切房surface／hallucination图引用；把保留的305／306固定头当启用媒体，或因本端无历史阻塞目标。
- 取消普通切房动画时误删Boss versus独立肖像；本地历史的多次叠加被合并成最后一次颜色。

## 实现边界

正文编码输入来自自有不可变纯值源，字段／路径使用数字索引。解码仅返回调用者持有缓冲的只读块视图与有界投影事实；编码失败不发布部分结果，解码不写影子状态或游戏对象。跨增量的计数组和共享owner依赖必须由同身份、同目标的完整候选补齐，不能由正文层猜测。

完整typed资源表payload／用途允许集、公共Frame装配及会话接受调度分别继续推进，不把普通域正文编解码当作新网络协议已经启用。第15步仍是唯一统一整体验收；本项不新增单元测试或提前运行任何测试。

## 当前输入与生成结果

版本61继续保留版本59的公共固定头尺寸、u32块掩码和12B记录头，修改本合同涉及的Presentation负载及允许规则。布局源为14份域文件，共340种布局、2772个字段计划、84个条件字段；最大单布局117字段。Random的24种新增类型没有下行字段，编码为空值、不补造RNG同步。全部域和字段数量见[同源正文计划表](../../../../mod/binding/layouts/09-wire-format/body-codec-plan.md)。

`when.test`明确登记equal、not-equal、any-bits、one-of和item-config五种规则；原predicate名字及证据保留。原本只写自然语言的条件现在有可执行数字表达，不在运行时解析语义文字。版本60登记的非Null引用、水面可空编码、Familiar计数、Pickup倒计时及规范零负载继续生效；版本61新增`value_rule=display-utf8`和`body_enabled=false`。布局版本必须按61协商，不猜读版本60的旧结果。

生成器新增`state_body_codec.inc`，按原Field全局索引生成标量／数组宽度、资源表号、条件操作、精确整数与浮点范围、枚举位模式、容器计数组和跨字段规范。数字规则与现有复制／字段／块／条件／预算／choice表来自同次生成，不维护第二份手写字段目录。

## 正文接口

入口为[`state/body_codec.h`](../../../../mod/src/state/body_codec.h)，实现为[`body_codec.cpp`](../../../../mod/src/state/body_codec.cpp)，编入`isaac_cs_state`产品库。

| 接口 | 输入与输出 |
|---|---|
| EncodeBodyInto／DecodeBody | 按layout、u32 block_mask及world/body序列处理无头块正文；校验预留位、块归属与精确消费 |
| EncodeValueInto／DecodeValue | 按完整值布局处理嵌套、各域结果或公共固定头；不附块mask或12B头 |
| EncodeRecordInto／DecodeRecordPrefix | 保留12B成员记录头；decode要求调用者提供指定表快照的expected member/layout，返回消费长度 |
| ValidateProjectionFacts | 同根身份／同目标事实合并后校验投影一致与计数组；完整模式拒绝缺项，增量模式明确返回尚需base的事实 |
| TranscodeDisplayTextUtf16Into | 将已合法封存、无终止符的UTF-16片段转成严格UTF-8；预算来自生成计划，拒绝错误／控制码／重叠缓冲，不读取原生指针 |

`BodyValueSource`用布局／字段／规范父路径／元素／动态槽布局提供已封存的小端标量、宽计数、共享owner存在性和Weapon槽。字段值不携带可执行函数、原生偏移或源指针；这里的虚接口是本地C++纯值提供者，不是网络函数分派。普通map/set的源须已经规范排序，正文层拒绝重复或逆序；History、贡献等有序列表保留原顺序。

`BodyView`返回连续正文、最多32块视图、有界投影事实，以及可选根字段视图。根字段用数字索引定位，失效分支明确present=false。所有span只借用调用者持有的不可变字节／scratch，下一次覆盖存储前必须消费或复制，不能转交原生对象地址；Values／Wire等内存计费仍由持有缓冲的正式owner负责。编解码器本身不分配JSON／字段树、map或逐元素vector。

## 实际处理规则

- 定长标量、小端整数、f32/f64原位模式、bool／枚举／范围／mask以及嵌套纯值均走同一数字计划。浮点拒NaN／Inf，整数边界不经double；所有读写有界且不做未对齐强转。
- 普通容器默认u16 count，具名u8 count、真实fixed_count槽严格按源编码。c-string拒内嵌NUL且不附终止符，普通字节string保留原字节；count先按u32检查再窄化，数量、尺寸和总预算同时限制。
- 条件只用同一递归实例、同块先行selector；false时不调用源、不读或消费旧payload。704冷却条件借用同resource_epoch的item-config纯值，核真实type/id及非零配置除数，缺配置报依赖Gap。
- 动态Weapon仅接受生成choice集合，mask高位拒绝，按槽升序附u16 view_layout，子视图内的weapon_type枚举继续校验。
- shared-owner只有首投影带u8 presence；缺失时直接编码／校验最短全零占位，不递归解释资源。仅收到尾投影时，decode必须取得同候选首投影presence；无法取得就报依赖Gap，不猜空。首尾不一致由投影事实拒绝。
- 各容器投影和共享presence按规范父路径关联，分开同容器投影计数一致、不同slot求和及总上限。P6目录与P0的三个costume count继续同组，固定13份Sprite不加重复槽号或count。
- 空主动槽五个尾值由encoder直接写规范零，不询问旧负载；Null costume的layer同样直接写零。decode拒绝非规范负载；其他有符号值、有效零、负ID和实际浮点位模式保持。
- `display-utf8`只用于DisplayTextRun：校验最短UTF-8、Unicode标量、无NUL／C0／C1；拒绝代理项、超U+10FFFF、不完整和非法续字节。其他string／EntityText仍按原始字节合同处理。编码和解码执行同一规则。
- 源中的`body_enabled=false`同源生成数字拒绝表；根值、块、记录及嵌套入口均拒绝305／306，不能借可靠承运开启。304的kind枚举排除5；完整table/kind矩阵及其他typed资源正文仍待下一项，不把基础头通过当资源接受。

## 文字与本地历史的版本61布局

| 布局／原owner | 当前正文与职责 |
|---|---|
| DisplayTextRun422 | `present:u8`；为1时依次为`font:u8 + style:u8 + RGBA:4f32 + color_space:u32 + utf8_bytes:u16 + utf8`。单串≤2048B，整体1—2073B；present=0确认无此槽，present=1且长度0为有效空串。来源Gap不调用编码器 |
| 字体与样式 | 字体profile0—5：Latin、Japanese、Korean、SimplifiedChinese、TraditionalChinese、Cyrillic；样式0—6：BannerMain、BannerSubtitle、CurseMain、CurseSubtitle、Fortune、PromptTitle、PromptBody。同源常量与枚举校验；这些是自有设计编号，非已核原生Font编号或已实现映射 |
| HUDMessage319／HUD323 | showing=1时main_text、sub_text各嵌套422，并保留streak姿态；11份消息仍按既有owner归属。HUD的fortune_image被fortune_text423替换，保留fortune Sprite；不再为这两种缓存声明image-table引用 |
| FortuneText423 | `present:u8`；有效时`line_count:u8 + 有序422列表`，最多8行、整体≤16586B。行数、所有正文／帧预算同时适用，不能通过分行绕过预算 |
| GenericPromptResult380 | present=1时固定五个422槽（不附额外count）、selection及paper；前二标题、后三说明，缺席与空串分别保留。个人菜单不进入world |
| ScreenHallucinationResult370 | 仅`present:u8`及有效时`amount:f32`，1—5B；无Game+25A98的Host资源依赖。共享背景等状态仍走原owner |
| LocalHistoryOverlay424／ScreenEffectsResult373 | 424为`branch:u16 + ColorMod:11f32`，46B；分支0／1定位两次已核Snapshot叠加。373的local_history_overlays最多32项、有序、≤1474B，取本端Snapshot；独立fades仍保留。无Host图像、像素、坐标或任意GPU命令 |
| BossVersusResult374 | 从旧RoomTransitionResult收窄为`present + 有序Sprite贡献`，移除images与前后surface；普通切房不创建此结果组。逐layer／额外肖像等独立内容仍须完整 |

UTF-16转码只接受已封存的有界值，最多2048个code unit且结果≤2048B；不扫描原生地址、不附终止符、不做有损替换或截断。实际读取资格、合法终止符上限、font profile到已安装字体的映射、原生特殊首码到有限style的映射及颜色来源仍由第10步核定；未映射时报具体Gap，不能删除首码后冒充完整。解码文本只作为文字交固定adapter，不送回原生控制码解释器；第13步实现本地排版缓存并区分缓存ready与正文有效。

本端无历史时从已合法提交画面初始化，取消的Host历史／surface不形成依赖或Wait。缓存刷新、冻结、scope退休、图形预算及纯显示leaf由第13步落实；本项不运行ShowHallucination／ChangeRoom／HUD Update，也不宣称本地效果已经实现。

`needs_base_checks`表示本次增量中的相关投影尚不完整；`links_required`表示仍需成员／资源图验证。两个标志都不能替代完整目标检查，即便都为false也不证明全部未变化块有效。影子层必须在相同scope、根Key/use/shape、layout_version和目标依赖下，以新块覆盖对应旧事实后验证，不能把新旧同一field事实简单重复追加，也不能把不同根身份合并。没有完整合法候选就不安装、不ACK。

## 预算和承运

默认正文／记录上限64KiB，调用者还须传本帧剩余实际字节额度；字段访问默认≤262144次、父路径≤32层，scratch及根字段区由调用者明确给出，上限不足报具名Gap。空元素也计访问预算，不以零长度绕过CPU限制；深度和访问上限只可在明确保护范围内设置。

`BodyCodecLimits.reliable`区分承运方式与确认序列。普通body执行PlayerFast的body_limit；可靠承运的ordinary body-joint仍是body序列，只改用record预算和调用者给定的可靠目标额度，不强行变成world，也不再套fast上限。可靠解码的32MiB仅是旧绝对内存保护；这个标志不授权扩大初次准备1MiB／10秒或Host三端10Mbps／模组7.5Mbps预算。Frame及发送调度还须累计所有记录、组、表和资源。

## 完成内容与尚待工作

本项已实现上述各域正文及12B记录编解码、机器条件和校验计划、投影事实及可选根字段视图，并按版本61迁移文字／本地历史和取消的切房依赖，同步生成表与CMake依赖。第9步仍需完整本地资源描述payload／table-kind矩阵、用途／pass／图形允许集、Frame／表／组／事件装配及严格会话接受入口；当前不实施原始媒体传送。10—13步提供真实纯值源、发布、完整影子、本地文字／历史缓存、资源ready与原生安装。未知原生字节、来源Gap和音频恢复缺口不变。

完成方式为静态代码审阅、普通布局生成和Release／Win32的`isaac_cs_state`纯编译。没有运行游戏、E2E、单元测试、probe、自测或独立校验脚本，也没有把产物安装到游戏目录。编译不能代替第15步功能／性能验收。
