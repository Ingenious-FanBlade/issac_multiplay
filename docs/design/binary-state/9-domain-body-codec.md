# 第9步：各域正文编码与解码

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

## 实现边界

正文编码输入来自自有不可变纯值源，字段／路径使用数字索引。解码仅返回调用者持有缓冲的只读块视图与有界投影事实；编码失败不发布部分结果，解码不写影子状态或游戏对象。跨增量的计数组和共享owner依赖必须由同身份、同目标的完整候选补齐，不能由正文层猜测。

完整typed资源表payload／用途允许集、公共Frame装配及会话接受调度分别继续推进，不把普通域正文编解码当作新网络协议已经启用。第15步仍是唯一统一整体验收；本项不新增单元测试或提前运行任何测试。

## 当前输入与生成结果

版本60延续版本59公共线格式尺寸和所有域负载顺序。布局源为14份域文件，共337种布局、2763个字段计划、79个条件字段；最大单布局117字段。Random的24种新增类型没有下行字段，编码为空值、不补造RNG同步。全部域和字段数量见[同源正文计划表](layouts/09-wire-format/body-codec-plan.md)。

`when.test`明确登记equal、not-equal、any-bits、one-of和item-config五种规则；原predicate名字及证据保留。原本只写自然语言的条件现在有可执行数字表达，不在运行时解析语义文字。`value_rule`登记非Null引用、水面可空编码、Familiar计数与Pickup倒计时的模式；`body_rule`登记空主动槽及Null costume映射的规范零负载。字段字节宽度和顺序没有因这些机器规则改变，整数布局版本按规范递增为60。

生成器新增`state_body_codec.inc`，按原Field全局索引生成标量／数组宽度、资源表号、条件操作、精确整数与浮点范围、枚举位模式、容器计数组和跨字段规范。数字规则与现有复制／字段／块／条件／预算／choice表来自同次生成，不维护第二份手写字段目录。

## 正文接口

入口为[`state/body_codec.h`](../../../mod/src/state/body_codec.h)，实现为[`body_codec.cpp`](../../../mod/src/state/body_codec.cpp)，编入`isaac_cs_state`产品库。

| 接口 | 输入与输出 |
|---|---|
| EncodeBodyInto／DecodeBody | 按layout、u32 block_mask及world/body序列处理无头块正文；校验预留位、块归属与精确消费 |
| EncodeValueInto／DecodeValue | 按完整值布局处理嵌套、各域结果或公共固定头；不附块mask或12B头 |
| EncodeRecordInto／DecodeRecordPrefix | 保留12B成员记录头；decode要求调用者提供指定表快照的expected member/layout，返回消费长度 |
| ValidateProjectionFacts | 同根身份／同目标事实合并后校验投影一致与计数组；完整模式拒绝缺项，增量模式明确返回尚需base的事实 |

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

`needs_base_checks`表示本次增量中的相关投影尚不完整；`links_required`表示仍需成员／资源图验证。两个标志都不能替代完整目标检查，即便都为false也不证明全部未变化块有效。影子层必须在相同scope、根Key/use/shape、layout_version和目标依赖下，以新块覆盖对应旧事实后验证，不能把新旧同一field事实简单重复追加，也不能把不同根身份合并。没有完整合法候选就不安装、不ACK。

## 预算和承运

默认正文／记录上限64KiB，调用者还须传本帧剩余实际字节额度；字段访问默认≤262144次、父路径≤32层，scratch及根字段区由调用者明确给出，上限不足报具名Gap。空元素也计访问预算，不以零长度绕过CPU限制；深度和访问上限只可在明确保护范围内设置。

`BodyCodecLimits.reliable`区分承运方式与确认序列。普通body执行PlayerFast的body_limit；可靠承运的ordinary body-joint仍是body序列，只改用record预算和调用者给定的可靠目标额度，不强行变成world，也不再套fast上限。可靠解码的32MiB仅是旧绝对内存保护；这个标志不授权扩大初次准备1MiB／10秒或Host三端10Mbps／模组7.5Mbps预算。Frame及发送调度还须累计所有记录、组、表和资源。

## 完成内容与尚待工作

本项已实现上述各域正文及12B记录编解码、机器条件和校验计划、投影事实及可选根字段视图，并同步CMake生成依赖。第9步仍需完整资源payload／table-kind矩阵、用途／pass／图形允许集、Frame／表／组／事件装配及严格会话接受入口；10—13步提供真实纯值源、发布、完整影子、资源ready与原生安装。未知原生字节、来源Gap和音频恢复缺口不变。

完成方式为静态代码审阅、普通布局生成和Release／Win32的`isaac_cs_state`纯编译。没有运行游戏、E2E、单元测试、probe、自测或独立校验脚本，也没有把产物安装到游戏目录。编译不能代替第15步功能／性能验收。
