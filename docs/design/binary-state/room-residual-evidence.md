# 第7步 Room：1290B与嵌套未知的继续核对

2026-11-10，同一步静态续核。先读本地原版 Wiki、官方 REPENTOGON 1.1.3声明／实现，再读J273 IDB。此次只修改设计字节账本、成员回映射和交接说明；生产布局版本仍56，没有构建、游戏操作或测试。VA均以imagebase `400000`表示，RVA=VA−`400000`；只读反编译／反汇编，没有修改或执行原生代码。

结果：**Room直属UNKNOWN 1290→28B，EntityList 884→3B**。新增必要的水面反向条件；其余本轮核清范围主要属于Host生产／缓存或原版Online私有内存。登记每个字节与消除全部语义未知仍是两回事；剩余间隙没有因“看起来对齐”被改称padding。当前[字节JSON](room-byte-ledger.json)／[单行表](room-byte-ledger.md)为修订2，保留原1290B未知范围作为修订背景；[269行回映射](room-member-map.md)保留原签名／重复ID。

## 1. Wiki与官方源码先回答了什么

资料目录仍为 `F:/projects_isaac/IsaacDocs/docs/` 和 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/`。先复读原版 `Room.md` 的GridPath、敌人计数、首敌死亡、血墙、减速和绘制查询，REPENTOGON的 `Room.md`／`enums/RoomEventOutput.md`，以及 `FXLayers.md`、`FXParams.md`、`Backdrop.md`、`Camera.md`。事件组公开值0—3及9（ROOM_CLEAR）说明其是逻辑触发；不能只因它存储在Room里就下发回调表。收尾复读`GridEntityDecoration.md`及`GridEntityDecoration.zhl`，前者明确暂无成员内容，没有提供Puddle计数说明；原版`enums/BombVariant.md`确认variant3为BOMB_TROLL。

再读 `libzhl/functions/{Room,EntityList,EntityList_EL,BackdropConfig,FXLayers,FXParams,ANM2,AnimationState,LayerState}.zhl` 及生成头 `include/IsaacRepentance.h`，核对 `LuaInterfaces/Room/LuaRoom.cpp`、`Patches/XMLData.cpp`、`Patches/ASMPatches/ASMEntityNPC.cpp`。官方声明留下的孔和过宽字段需要原生补证，不把SDK当实际ABI完整答案：

- `Room.zhl`的456个path DWORD覆盖到了另一张WORD表，两个数组必须分开。
- `BackdropConfig.unk1:u32`实际只使用最低byte的`fullimage`；尾部`waterPitsMode:u32`实际是两个byte，第二个是`reversewatergfx`。XMLData里后者的直接赋值只是**注释掉的代码**，不能把注释当官方已执行的writer。
- 官方 `ASMEntityNPC.cpp::IsRoomSlow`通过SDK `_slowdownDuration`读取+7220；原版Lua的Has/SetSlowDown实际在+722C。这是官方源码与原生语义的差异，不修改仓库外SDK／官方DLL来掩盖。
- `CollectibleType`说明530为死亡名单、650为Plum Flute；Room中的是boss ID，不能把boss ID84误当`EntityType`84（Satan）。音效表确认473／476为水流loop等环境声音。

无法从这些资料得到的字段边界、实际读写宽度、容器类型与副作用，才用以下原生链补证。

## 2. 最大864B：不是path尾部，而是另一张448格WORD表

`Room+76C..E6B`恰好`448×4=700h`。`7F91AA`取`Room+E6C`，`7F91B6`另取`Room+76C`，在同一循环中DWORD指针加4、WORD指针加2；循环上界为实际width×height。两者分别结束于E6C、11EC，后接已知清房延迟。

WORD表的正值会使`55BD00`拒绝NPC的目标格，`546130`择坑时也排除非零项。`4FC240/5464B0/55BEC0/564BC0`有实际WORD写入；`7F7F60`每三帧对1—99执行减1，其他值不按该分支衰减。因此归为 **NPC格子占用／重选避让计时 `i16[448]`**，不是另外448份Grid身份，不是静态碰撞图，也不能按填充删除。

| 原生范围 | 核定类型／用途 | 同步决定 |
|---|---|---|
| `76C..E6B` 1792B | `i32[448]`动态path cost | Host；当前Grid外观／WaterClip已有独立结果 |
| `E6C..11EB` 896B | `i16[448]`NPC占格／择位计时 | Host；Client不运行AI／寻路／碰撞／择坑 |

原1290B里的`E8C..11EB`864B是第二表的后432项；原SDK多出的8个DWORD占了第二表前16项。此次同时纠正原来“456项已知”的32B边界，不能只把未知末尾填上而保留重叠声明。正式Room布局和旧reader后续换接必须使用两张表各自的448边界；不以同步这张表恢复Client模拟。

## 3. Room剩余较大块的实际owner

| 位置／原未知量 | 证据 | 核定用途与同步决定 |
|---|---|---|
| `1214` 4B | `7E1550`按mainEL重新编号；各正常Spawn先读后增并写Entity+20 | 原生Entity.Index分配计数，Host。C/S身份和必要排序index由Entity域承接，不同步分配器游标 |
| `1BBC` 1B、`1BC0` 8B；并补清原`1BB8`用途 | `7F7610`检查物品530、注册目标、生成Effect109、完成后生成Pickup／改Player属性 | 死亡名单target、done、reward position。全部生产控制留Host；当前标记Effect／其目标关系、实际奖励／属性另发。`1BBD..1BBF`3B仍未知 |
| `7084..70C3` 64B | `7E0880`、`7E1550`、`7F5950`、静态`AAD0E0`16×10h配置 | 16个环境音量平滑float，见下一节。原数组不下发，但当前持续声音必须有实际voice结果 |
| `7234..723B` 8B | `7EFDB0/7EFFB0→4DF2F0`插入格序号；`7F9098..7F911A`正常Update消费并清空 | 待处理格子破坏／路径变化集合，是set头，Host；不复制树或在Client重放Grid动作 |
| `72A0` 1B | 原绑定`6B4380/6B4390`，NPC死亡`6D1E33/6D2079`，Init word清零 | FirstEnemyDead玩法资格，Host；不把它当额外显示bool |
| `72FC..72FF` 4B | `7E167D`置−1、`7FC04D`置0、`7F5950`到10播放460并继续按楼层距离更新pan | 延迟声音／pan生产计时，Host；真正voice/event与逻辑pan归音频，不重放计时 |
| `7300..7307` 8B | `815FB0`分配32B node，正常NPC死亡`6D2098/6D20AC`写入，`7F4240`消费271/272死亡历史 | **list头，不是tree**，最多32项。节点8B链接＋24B位置／类型／variant/subtype／seed／friendly记录；历史留Host，不建立回生／奖励历史镜像 |
| `7328..7377` 80B | `817F30`遍历10组树并建立448格组映射；`818250`按组索引读node+10格序号 | 10组`set<i32>`。Host触发后只发送实际Grid／Entity后态，不发组集合驱动Client逻辑 |
| `7378..73EF` 120B | `818250`按group×12取begin/end，真正执行`(*fn)()` | 10组`vector<void(*)()>`，元素是函数地址。**绝不发送／安装／远程调用**，也不将TriggerOutput转成同步命令 |
| `73F0..73F3` 4B | `43AA60`记录GameFrame、检查900帧、写−1；`7F0DE0`检查Room.bossId84与物品650并改奖励／解锁 | Plum Flute奖励资格的时点／哨兵，Host；实际Boss和Pickup结果另发，不复制持久资格 |
| `73F4..7453` 96B | `819530`以index×24构造，`8199D0`据count逐24B采样并Spawn Effect135 | 4个雨区域，每个origin＋两个basis共24B。生成区域、RNG均Host；真实雨Entity和ambient另发 |

另纠正原具名但粗略或错误的计时／控制名：

- `452520 EmitBloodFromWalls`写`+7214 Duration/+7218 Count`，不是任意Room计数。
- 相邻`+721C`并不属于血墙控制：`973250`的GridDecoration动画选择先匹配`Puddle`，`973990`因已有非零计数排除候选，最终选中Puddle时`973A82`递增；`7E8CD0`清零。它留Host，实际装饰Grid资源／动画由第6步承接，不重跑随机选择。
- `+7224`不是减速倒计时。NPC路径`6B4E0B`增量，`7F4240`输出`TriggerBossDeath: %d bosses remaining`并先减再处理死亡奖励，正常Update `7F9358..7F93A1`还检查`last boss died without triggering the deathspawn`并回写当前Boss数。核为待处理Boss deathspawn计数／兜底维护，全部Host。
- 原绑定`852D9B→452550`和`852DA5→452540`证明真正slowdown在`+722C`，Setter为`max(current,arg)`，Getter为`>0`。
- `+7230`也不是只知递减的普通timer：`705CED`某动作分支写35，`7FA998..7FAA0F`在正值且模5为0时随机择位生成type4／variant3的troll bomb，再递减。计时／择位留Host，实际Bomb Entity另发。

这些原生字段均不需进入Room显示状态；它们产生的实际Grid／Entity／奖励结果仍按各自owner承接。

`+7220`仍只有原生波纹draw的一byte相位条件证据。官方Hush补丁经错误SDK int声明连带读取`+7221..7223`，这三字节的真实原生语义仍未知。**不能把过宽读当作正确四字节timer的证明，也不能再说这三字节从不被任何官方代码读取。** 该路径是Host攻击模拟，Client使用实际Laser／NPC结果，不新增这三字节负载。

## 4. 环境声音：原64B不发，声音不能消失

静态表每行`{sound_id:u32, smoothing_step:f32, gain_scale:f32, preserve_on_room_reset:u32}`；Room从7084开始每项存一个current gain。`7E1550`只清最后标记最低位为0的对应项，`7E0880`则逐项清零并停止对应声音；它们不是统一生命周期的16个任意float。

| Room偏移 | sound ID | 步长 | gain scale | 保留标记 |
|---|---:|---:|---:|---:|
|7084|269|0.5|1|0|
|7088|4|0.5|1|0|
|708C|279|0.5|1|0|
|7090|163|0.03|1|1|
|7094|281|0.5|1|0|
|7098|473|0.03|1|1|
|709C|480|0.03|1|1|
|70A0|476|0.03|0.8|1|
|70A4|161|0.5|1|0|
|70A8|505|0.5|1|0|
|70AC|535|0.5|1|0|
|70B0|558|0.5|1.3|0|
|70B4|555|0.5|1.5|0|
|70B8|553|0.5|1.5|0|
|70BC|580|0.5|1|0|
|70C0|612|0.5|1|0|

这些表值来自原生静态数据，不是本轮运行观测。`7F5950`根据真实Entity／水流／雨／熔岩等条件生产目标、平滑后调用`908530`，部分还有独立pitch。正式同步不需要下发16个平滑器和完整环境生产依赖；音频域需要当前实际voice、已经选中的sample、最终**world逻辑**gain／pitch／pan、开始／停止与持续相位。不能发送乘Host Options后的后端音量，不能在静音时反除，也不能因为不发原数组就丢掉火／水流loop。这里补到了具名逻辑增益缓存来源；实际sample／voice身份／生命周期、完整逻辑意图缓存和writer仍待第8／10／13步接线，不称已完成音频同步。

## 5. EntityList：884B缩到3B，隔离原版Online

Wiki／SDK公开EL与参数查询，原生`413270/413740/413A30/413F00/415880/416820`核定此前未知块：

| 相对EntityList | 用途 | 决定 |
|---|---|---|
|`0..1F` 32B|八个`24h`空间分区索引对象指针；`415880`重建并进入碰撞pair处理|本地／Host索引，不发地址、分区或碰撞模拟结果 |
|`A8..B7` 16B|新加入／已统计enemy、boss两组计数；`4524E0/452500`各返回钳非负的sum|Host；不复制为Getter相等 |
|`B8` 4B、`C0` 4B|当前计数scratch／统计快照，`414764..4147A1`轮转|Host |
|`C4..CF` 12B、`D0/D4`各4B|`415880`按碰撞主体N²准备的pair位存储、处理游标、主体数|本地碰撞维护，不传容器／bitset，不在Client放行该更新 |
|`D8..DF` 8B|实际`gfx/shadow.png` Image strong；RenderShadows使用|正常本地标准资源owner，绝不发Image/control指针；不能随查询缓存一起省掉真实阴影绘制 |
|`E0..E7` 8B|`QueryType(Cache=true)`的三元key树；node44B，携带借用EL|本地本帧查询缓存，不发树／借用列表 |
|`E8`4B、`EC`1B、`F0/F4`各4B|原版Online的待处理word／byte数、累计比较值和控制|全留原版私有，C/S不读取、计算、比较或复用 |
|`F8..3F7`768B、`3F8/3FC`各4B|原版Online32×24B环，记录frame、56B事件vector、6EB740返回值及原版比较结果；head/count|全留原版私有，不借作C/S历史／时钟／一致性条件 |

`413F00`只在Manager+4AFB8/+4AFBC所代表的原版Online状态分支维护上述环；这里只读它的静态控制流，没有执行任何比较／累计函数。`413050`写六DWORD，`4130F0→417EB0`按56B元素析构事件vector；24B帧记录中vector头只有12B，后两DWORD是独立记录值，不误当20B vector头。

EntityList剩余仅`ED..EF`3B。每个`EntityListEL+1..3`的未知仍另计，不因外层未知减少而消失。正式Entity成员、实例身份、实际有序render关系仍照原合同发送，不能因为碰撞／查询／Online不上线而丢掉必要成员或排序。

## 6. FX嵌套：先纠正结构，再决定负载

### 6.1 Poop不是36B未知拼接，而是内嵌完整FX

`7DF9EF lea ecx,[FXLayers+38]`后`7DF9F2 call 967D40`直接构造一份`4Ch FX`，恰好到`+84`的poop ColorMod之前。此前`FXLayers+38`的4B与`+64..83`32B未知都属于它。新账本用专用`FXPoop`连续视图，父用途只允许poop图像、有效pose与最终alpha；不能机械继承generic FX的pass负载。

`968E10`从XML逐项解析到栈上同4Ch对象，随后选择／生成实际层；`967D40`、`96BF40`印证其位置：

| 相对FX | 实际内容 | 决定 |
|---|---|---|
|`1C/20/24/28`各4B|xMin/xMax/yMin/yMax，生成pose速度的端点|Host，不重新抽速度 |
|`30`4B、`34`4B|stage编码与altStages mask，`967DB0`筛选|Host，仅发送实际采用层 |
|`38`1B|onlyDefaultBackdrop配置过滤|Host |
|`39`1B|parallax模式，`96BF40`用平均Player位置生产pose位置|模式Host，实际当前pose位置仍需发送 |
|`3A..3B`2B、`49..4B`3B|未取得独立语义|UNKNOWN5B保留 |

FXLayers直属UNKNOWN39→3B（`9..B`），FX本体31→5B；poop子体仍有5B未知，不能只报父表3B就称poop完整。`FXPose+8/+C`不被已核draw读取，但**Host `96E3C0 GetFloorColorAt`的pass4图像采样确实还读它们**；因此撤销“只用于Update”的过强措辞。正式Client不调用这个查询补生产颜色，实际Entity颜色另由Entity域承接，速度仍不进入显示负载。

### 6.2 RayGroup与Vein

`RayGroup+28/+29`是两个byte，不是尾部DWORD：Load中stages匹配资格和token的b后缀标记；`96F8A0`只单字节复制两者。二者只参与Host候选选择，当前只发实际选中的组。尾部`2A..2B`2B仍未知。

`Vein+30`是`96F090`沿child传播／清除的退休／收缩控制bool，影响后续factor及删除；`96EAB0`不读它。Client只需实际factor／opacity和已选图拓扑，不发该未来控制。`+31..33`3B仍未知；`+10..13`4B虽见`96ED60`从父复制到子，业务含义仍未闭合，**不因有copy就标为已知状态或padding**。Vein未知8→7B。

## 7. BackdropConfig：发现实际必要的反向水面条件

`90C3D0`解析`fullimage`写本地配置+74 byte，`90E080`只复制这一byte；`90F2E0`构图时读取Backdrop+id×13C+1C8（即配置+74）。它是构建模式：如果同步最终floor纹理，不需额外发这个原值；如以后选recipe路线，必须含该模式。`+75..77`3B仍未知。

配置尾部`+138`是waterpitsmode byte，`+139`是reversewatergfx bool；构造`90B9C0`仅清一个word，copy `90E080`分别复制两个byte。原SDK的u32会混入另一个字段及未知尾2B，必须撤销该宽读取依据。

**`+139`需要同步。** `91AC70 Backdrop::RenderWater`在`91ACEB`读取`Backdrop+id×13C+28D`，`91ACFD`据此将`GameFrame/2`乘−1。它改变当前水面纹理移动方向，只有最终静态纹理内容无法重建这项动态显示。保留实际采用配置的一byte方向，或同world等价的最终有符号phase；这不是同步背景RNG的新例外。不发全配置、未采用的63份候选、构建seed或进程地址。正式方向来源／writer尚未接通。

BackdropConfig未知原为4B；本轮核清fullimage1B，同时把过宽尾u32纠正为两byte后新增尾2B未知，**当前UNKNOWN为5B／实例**。这个数字上升是纠错，不是新发现这些字节有动态状态；63份配置的物理未知要按实例计算，不能只计一次5B。

## 8. 仍未知的确切范围与处理

Room根剩余28B全部列在账本：

| 范围 | 字节 | 目前结论 |
|---|---:|---|
|`2..3`|2|初始化字节后间隙，未证明填充 |
|`11F7`|1|pit-dirty与Camera之间，未证明填充 |
|`120D..120F`|3|red-heart bool后间隙 |
|`1BBD..1BBF`|3|名单done与奖励Vector间隙 |
|`7221..7223`|3|含官方Hush补丁错误int读取事实，真实语义仍未知 |
|`722A..722B`|2|逻辑byte后间隙 |
|`729C..729F`|4|已核两端counter／first-dead外，未取得该4B独立消费者 |
|`72A2..72A3`|2|pacifist与ColorMod间隙 |
|`745D..745F`|3|rain control与水流Vector间隙 |
|`776A..776B`|2|动作byte与地狱背景视图间隙 |
|`7895..7897`|3|Room末端pending bool后间隙 |

嵌套直属未知：Backdrop3B；BackdropConfig5B／实例；FXLayers3B；FXParams3B；FX5B与FXPoop5B；FXPose2B；RayGroup2B；Vein7B；Camera3B；RoomHellBackdrop3B；EntityList3B；每个EL3B；RoomTemporaryEffects2B。通用AnimationState3B／LayerState4B／ANM2直属8B保持，动态节点、63配置、内嵌多Sprite要按真实数量另计。新增KilledNPCRecord物理视图还明确其payload尾3B未知；该历史整体Host，不因不发送就掩盖未知。

**未完整展开的动态payload另列，不能用上表数字掩盖：**

| 动态对象 | 已核边界／owner | 保留的未知与同步决定 |
|---|---|---|
| EntityList的8份空间索引 | 每对象`24h`，构造／清理／碰撞用途已核 | 内部完整ABI未展开；本地Host索引，不传或重建成Client碰撞系统 |
| 原版Online事件 | vector元素步长56B，帧头／析构已核 | 56B事件内部语义未逐字节解释；原版私有，不为C/S复制、回放或计算 |
| QueryType缓存树 | node44B，key为Type/Variant/Subtype，携带借用EL | 节点全ABI及间隙未另建连续视图；本地查询缓存，不传；EL自身3B未知另计 |
| 补货deque元素 | 补货生产及实际Pickup／价格owner已核 | ShopRestockDesc完整布局未展开，不从deque头猜元素大小；队列留Host，实际商品结果另发 |
| RoomTemporaryEffects动态条目 | 本视图只覆盖18h头；效果481有实际draw消费者 | 完整条目类型未在本轮核清；列表／生产控制Host，具名最终绘制资格另承接 |

这些是用途已能决定无需或不应同步、但内部布局仍未全解释的范围，机器表以`opaque_nested_payloads`单列。Image／图形资源内部及其他已委派域也不因当前表的0B直属未知而视为已完成；它们仍遵守独立资源owner和正式来源缺口。

这些范围本轮逐类补读了构造／清理／copy／swap和已核实际消费者，具体路径登记在各类型`residual_review`：

- `ANM2+108`在`402CD0`被单byte复制、`403160`交换、`403530`与loaded一起word清零；这只证明copy/reset保留它，仍不能命名为shader或独立显示flag。
- `LayerState+31`也只有copy及`407F7A`清零证据；官方自定义shader仍在Anm2Extras旁表，不把+31解释成shader资格。
- AnimationState+15..17、ANM2+71..73/+102..103/+10A..10B、LayerState+75..77等不能因没有独立直接位移命中就认作padding。`FXPose`20B构造只写16B坐标／速度和2B flip，尾2B没有初始化契约，不读取。

**本轮未核清的字节保持UNKNOWN、不读不发，也不靠保留旧内存恢复画面。** 已确定的实际显示消费者若缺来源，仍具名失败；不将静态未决冒充第15步通过。

## 9. 后续接线与唯一验收

本轮更新32个类型／视图、Room91段和原269行回映射，未生成生产Room布局。第10步须按两张448表的真实边界排除Host数据，按实际资格取方向／环境音频及已有最终显示结果；第13步只装结果／自有资源，不调用TriggerOutput／Grid延迟队列／死亡名单／雨／碰撞／原版Online更新；第14步移出旧456-DWORD、过宽BackdropConfig字段和旧callback／历史复制假设。

第15步唯一统一验收再覆盖：两个数组448边界、占格计时不被当path、最高格／轨道；死亡名单标记与奖励不重复；十事件组产生后态而不传函数地址；雨区域不在Client重复生成；16环境voice与静音／跨房／pan；正反向水面与不同FPS／重复展示；空poop／ray／收缩vein；EntityList查询／碰撞／原版Online区不被复制或执行。本轮只静态文档审阅，没有执行这些测试。
