# 第 6 步：PressurePlate 与 Door 字节归属

日期：2026-11-10（Asia/Singapore）。布局版本 **53**。目标为官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。IDA imagebase `0x400000`，下列地址为 VA，RVA 减 `0x400000`。本轮只读本地 Wiki、ZHL、官方源码、既有研究、目标反编译及原始指令，并生成布局、纯编译状态库；没有修改 IDB、测试、启动游戏、注入或部署。

**这两类的全部对象范围均已登记归属并进入正式生成源。** 原生语义仍有 UNKNOWN：PressurePlate 根 **31B**、Door 根 **32B**。结合版本51八类、版本52七类，Grid 的全部17个派生类现均有原生字节表和状态二进制表；全119项目录回映射、GridEntityPlus合法来源及第7—14步实际运行接线仍未全部完成，不能宣称整个重构或第6步所有工作完成。

本轮发现工作区七类已经完成到版本52，因此保留202—223及其原始记录，追加 **224—228** 到 [grid-classes.json](../../../../mod/binding/state/grid-classes.json)：224非随机Light描述、225门主颜色、226门主Sprite、227 PressurePlate、228 Door。完整单行原生／二进制表由生成器输出，见 [PressurePlate](../../../../mod/binding/layouts/06-grid/GridEntityPressurePlate.md)、[Door](../../../../mod/binding/layouts/06-grid/GridEntityDoor.md)。

## 1. 同步决定

| 对象／成员 | 最终决定 | 实际理由 |
| --- | --- | --- |
| PressurePlate TimerPlate `+154` | **同步真实内嵌ANM2的最终展示状态** | Render直接绘制；不以variant=2或公开属性的值副本推断存在／姿态 |
| NextGreedAnimation `+268` | **Host；字符串内容也不发** | Update比较／Play成最终主Sprite，Render不读；“仅视觉”不代表仍需下发生产输入 |
| GreedModeRNG `+26C` | **Host** | 奖励／波次生产流，Client不重演Reward／生成 |
| Door ExtraSprite `+168` | **同步** | 主入口通过且ExtraVisible时直接绘制，自己的颜色／资源／动画／图层都有消费者 |
| Door TargetRoomIndex `+280` | **同步i32** | GridLight通过它读目标descriptor.Flags；不只是过门／转场字段，不是指针 |
| Door PreviousState／PreviousVariant `+284/+288` | **Host** | Init／PlayAnimation历史缓存，最终双Sprite已承接选择结果；不是Client差分基线 |
| Door TargetRoomType `+160`、ExtraVisible `+27C` | **同步4B＋1B** | Light分支／实际extra绘制输入 |
| Door flags `+28C` | **仅传bit0x20的1B布尔投影** | 已核Light只读该位，其余门规则位留Host |
| Door计数／Slot／当前房型／Direction／Busted／解锁锁存／四动画名 | **Host** | 玩法／结构生产／动画选择输入；本类Render与Light不消费这些原值，实际双Sprite和Room槽关系另传 |
| PressurePlate主Sprite颜色 | **同world地板色派生** | 绘制前完整覆盖，不发旧44B |
| Door主Sprite颜色 | **只发alpha正值入口判断1B，其余由墙色派生** | alpha在整色覆盖前被检查；不能全删，也不必传原alpha幅值及其余十个旧分量 |
| 两类Desc.SpawnSeed | **本类留Host，公共198不改** | Light虽在公共prologue建临时RNG，这两个case没有消费；不能泛化TrapDoor对Seed的依赖 |

最小线状态（不含第9步记录头）：PressurePlate **212B**，Door **223B**。这是格式下界，实际图层、帧选择器、资源及依赖仍按真实数量编码，不是实测带宽。

## 2. 工厂、范围与证据边界

`706100` 工厂构造公共部分、非平凡成员和真实vtable，最后写type／index，不执行Init／PostInit。

| 类 | new call／请求 | vtable／TD RVA | Update／Render／InitSubclass／Free |
| --- | --- | --- | --- |
| PressurePlate，type20 | `70658B`／`27Ch` | `B195DC`／`7D2280` | `95B420`／`95E140`／`95E420`／`706830` |
| Door，type16 | `7061FD`／`2F0h` | `B1966C`／`7D15C0` | `7074D0`／`707F90`／`70C1D0`／`706730` |

PressurePlate工厂 `7065AA/7065B6` 构造 `+154` ANM2，`7065C8/7065D0/7065DA` 配置尾部RNG四字。Door工厂 `70621C/706228` 构造 `+168` ANM2，并构造四条24B string；`7062B0` 写最后string的 `+2EC=15` capacity。SDK Door只声明到 `2ECh`，不覆盖实际最后4B；工厂和string ABI共同解释到 `2F0h`，没有把分配请求单独当完整语义证明。

读取仍需要真实当前对象／use、Init／资源／容器owner和完整主线程窗口；有Factory见证不代表尚未初始化的标量／借用字符串可读。Client的构造、资源加载与发布资格由本地owner维护，不能安装Host Initialized或loaded字节替代。

## 3. PressurePlate完整子类区

| 原生区间，左闭右开 | 类型／归属 | 证据与决定 |
| --- | --- | --- |
| `[154,268)` 276B | TimerPlate ANM2，递归展示视图201 | 工厂构造，Render `95E23F—95E2B9` 读loaded／主叠加顺序并绘制；Free `70683C` 析构。保留实际资源／Image、Offset／Scale／Color、两套动画与选择器、图层和本Sprite独立glitch位 |
| `[268,26C)` 4B | const char* NextGreedAnimation，Host | Init `95E4EB` 置空串地址；`95B880` 按Greed状态选常量名，`95B990` 在动画可切换时比较并调用Play；Render没有读。地址不能跨进程，值字符串也不必发送 |
| `[26C,27C)` 16B | GreedModeRNG，Host | Init `95E8DB—95E8EE` 写SpawnSeed与4／3／27，区别于Factory默认种子／shift配置。整段四个u32有归属，不在Client恢复奖励／波次流 |

TimerPlate本身没有指针槽或额外present位：每个真实PressurePlate都有已构造内嵌对象。正常Init只在variant2载入TimerPlate资源，但Render对其loaded的检查**没有variant条件**；不能把其它variant的真实已加载成员强行编码Null。使用已有可空文件资源选择表示可绘资源有无，缺来源／失败区别于Null。原版属性返回的ANM2值副本会改变owner相关Offset／flags，继续从真实 `+154` 取值。

主Sprite在 `95E1AF—95E1E6` 用 `Room+72A4` 全44B地板色覆盖，使用202，不发旧主色；TimerPlate没有这项覆写，仍用201保留自身完整ColorMod。它在格子渲染位置上额外加 `(0,21)`（`95E260/95E265`），这是Render参数，不能伪装成额外原生Vector或覆盖其自身Offset。

### 计时文字不是PressurePlate尾部字段

variant2的文字直接读取 `Room.greedWaveTimer +723C`（`95E2D3/95E2D9`），令 `q=(timer-1)/30`，显示 `q/60` 和 `q%60` 为 `%02d:%02d`。保留原生有符号32位运算及向零截断，不按非负时间／本地墙钟改写。闪红条件还读Host GameFrame `%4`，文字位置再加 `Game.screenShakeOffset +25A84`；字体／字宽与视口为本地已准备资源。

第7／8步需要提供同world的Room计时、Game展示帧、震屏偏移／字体；这里登记具名上下文，没有在Grid里重复发送房间状态，也没有宣称计时文字只靠双Sprite即可重建。缺这些来源报Gap，不能开Client Update、扣血启动、Reward或刷波补显示。

根UNKNOWN **31B**：Desc3B＋公共ANM2的14B＋TimerPlate的14B。线G0=20B、主外观50B、主动画最小22B、主图层最小2B，共94B；TimerPlate三块最小118B，合计212B。

## 4. Door完整子类区

| 原生区间 | 类型／名称 | 归属及证据 |
| --- | --- | --- |
| `[154,158)` | u32 update_counter | **Host**。Init `70C218` 清4B，Update `7074EF` 明确 `inc dword [ebx+154h]`；不是bool |
| `[158,15C)` | i32 slot | **Host原值不上线**。MakeDoor `7E4DE0` 尾部写primary slot；过门输入／路径消费。Room八槽真实主体／候选／route仍要独立同步，不能凭这个标量构造别名或允许Client过门 |
| `[15C,160)` | i32 current_room_type | **Host**。`70BBF0` SetRoomTypes写入并选资源／动画；实际Door Render、Light不读，不能因与target类型相邻而整段copy |
| `[160,164)` | i32 target_room_type | **状态4B**。Light `8021A8` 比27，`802BB4` 比15，直接决定真实光照分支 |
| `[164,168)` | i32 direction | **Host**。`70C180` 写方向并更新两Sprite.rotation；实际旋转已由各Sprite传，不用setter重放生产过程 |
| `[168,27C)` | 276B ExtraSprite ANM2 | **递归状态201**。官方GetExtraSprite明确返回该内嵌成员，`708070—70808E` 绘制；extra原色未被墙色覆盖，不能套主颜色裁剪 |
| `[27C,27D)` | bool extra_visible | **状态1B**。Render直接检查，区别于资源loaded及主alpha入口 |
| `[27D,27E)` | bool busted | **Host**。破开／IsBusted原值；当前双Sprite和extra开关承担最终显示，Render／Light不读此字节 |
| `[27E,27F)` | bool unlock_latch | **Host**。TryUnlock `70B7E0` 写1，Update `707B4E` 与开闭判断读，不能传成Client解锁权限 |
| `[27F,280)` | u8 unknown_27f | **UNKNOWN1B**。Init `70C240` 仅清27C／27D两字节，`70C249` 仅清27E。没有27F语义或填充证明，不读不发 |
| `[280,284)` | i32 target_room_index | **状态4B**。Light `8028AE` 把它传入目标描述查找，`8028BB` 检查目标Flags `40000h`；负特殊索引不能改成无符号或默认Null |
| `[284,288)` | u32 previous_state | **Host**。Init `70C1EB`、PlayAnimation `709F96/70A030` 等更新；动画选择路径读取。Render／Light没有消费 |
| `[288,28C)` | u32 previous_variant | **Host**。Init `70C1F4`、PlayAnimation `709F9C/70A038` 更新，`709F68` 等读取并选择Vanish等最终附属动画；不与当前Variant重复发 |
| `[28C,290)` | u32 door_flags | **1B换算投影**。`802A16—802A21` 只取bit0x20供Light；其它位是Host选资源、开闭、破开、解锁等规则输入。不能沿用旧完整raw flags同步 |
| `[290,2A8)` | string open_animation | **Host**，24B正常string对象；最终主Sprite另传 |
| `[2A8,2C0)` | string close_animation | **Host**，24B；不在Client执行Update选动画 |
| `[2C0,2D8)` | string locked_animation | **Host**，24B；不重放锁定过程 |
| `[2D8,2F0)` | string open_locked_animation | **Host**，24B，包含SDK声明外的capacity `+2EC..2EF`；不是未知尾字 |

Door尾部共 **412B**，所有范围连续登记；根UNKNOWN **32B**＝Desc3B＋两ANM2各14B＋27F的1B。四string的内部地址／容量属于正常本地存储，整个选择输入留Host，不用网络字节覆写头，也不在Client建立Host字符串借用。

### 纠正旧+154解释，保留原历史

[旧原生笔记](../tx-04-18/tx-04-18-grid-native-research.md)把 `70C3C0` 的IdleCoin分支作为Door+154模式证据，本轮撤销该解释。该函数载入 `grid_locks.anm2`，所在类是Lock（vtable `B19F14` 的InitSubclass）；其byte+154表示金币解锁模式。Door是另一张虚表和不同范围：`7074EF` 对+154做dword递增，`70C218` 对整个dword清零。旧事实原文不删除，但旧 `door_animation_mode bool` 的采集／写入／相等检查不得进入新路径。

同一旧笔记所称“Door Init不写Direction”也由本轮原始目标修正：`70C25A` 明确写 `Direction +164=-1`；不写primary Slot这一点仍成立。MakeDoor之后会按真实slot设置方向和两Sprite旋转。不能把Init默认-1、当前方向与最终渲染旋转混为一个来源，更不能为复现该生产顺序执行Client Init。

### 主颜色只保留入口判断，extra颜色保持

Door Render `707FA1` 在任何绘制之前比较主Sprite `ColorMod.tint_a`（对象+F4），有限值只有alpha>0才进入；随后 `70800E—708019` 将 `Room.wallColor +72D0` 完整44B写入主ColorMod。主loaded检查、主／overlay绘制及extra可见检查都在这个入口内部。

225 GridDoorTint沿用ColorMod的十一段原生范围，只保留alpha4B的**正值布尔投影1B**，另外十个旧分量40B派生；不发原alpha幅值。Client先安装本地0.0／1.0作为同等入口判断，进入draw后借用同world完整墙色并精确恢复。COMISS对NaN的指令分支不能按伪代码外推，本合同对非有限来源报Gap，不用规范化隐藏异常。关闭入口不代表删除Door，也不能跳过独立GridLight pass。

226 GridDoorSprite的外观为51B（94−44＋1），仍保留显式资源／Image、主／overlay动画与独立选择器、全部必要图层、插值和本Sprite的glitch最终两位；extra仍用201，保留它自己的颜色和glitch，不能复用主Sprite的结果。

### 目标索引、标志和Light依赖

Wiki的TargetRoomIndex说明只提转场动画，不能作为“此值不影响显示”的证明：`8028AE—8028BB` 是明确的反例。线上保留i32索引和target_room_type，**不调用Client原生过门FSM**。索引必须在同world的Level描述／特殊目标映射中解析到Client自有描述；Light读取目标Flags `40000h`。当前Data.StageId==35（Home）、维度、stage suffix、主Sprite layer3／layer5及动画／帧也是正面依赖，由第7／8步上下文与实际资源承接。Door房型+15C不能代替Data.StageId或Type。

**2026-11-10纠错**：本段原写“RoomConfig.type35”，字段名误认。补查`802758/80275E/802761`的Game→Room→descriptor→Data链和`80276C cmp DWORD [eax],23h`，实际读配置+0 StageId。原错误在此如实保留，现行以StageId35为准；详见[第7步资源设计](room-display-resource-design.md#1-资料与本轮纠错)。只更正依赖说明，不改Door状态字段／类型号／版本。

door_flags只发 `(raw & 0x20)!=0`。工厂没有初始化该word，Client本地owner先中性初始化0，再只设置／清除该位并保留其它本地位；不把其余Host规则位带到Client。资源、目标描述或同world关系未就绪报Gap，不把非法索引解成空、不套旧JSON的值域限制、不读取本地进度／Online来猜。

Door G0=20B，主Sprite最小75B，公共共95B；专用块4为target type4B＋ExtraVisible1B＋target index4B＋Light位1B＝10B；extra三块最小118B，合计 **223B**。旧四动画名和previous缓存不上线不表示动画可忽略，而是直接传Host已经产出的双ANM2。

## 5. 公开成员回映射与禁止事项

本轮在各类members中映射Door全部31个唯一ID（旧应用目录54—85行共32行，ExtraSprite属性／getter共用一个ID），PressurePlate的97—100行，以及ToDoor第28行、ToPressurePlate第34行。ExtraSprite由真实+168承接，GetSpriteOffset是公共主Sprite.Offset（对象+D4）的别名，不新增Vector；公开IsOpen `431670` 和IsLocked `431680` 可由当前Desc派生。PreviousState／Variant的REP getter修复只改变API正确性，不使历史缓存变成网络状态。其它门玩法查询／命令归Host，不建立通用getter／RPC兼容承诺；全119项逐行生成回映射仍待整体验证整理。

- 不直接同步vptr／RTTI地址、ANM2／Image／AnimationData／数组地址、string头、借用const-char*、loaded／Initialized资格、动画事件位和UNKNOWN。
- 不为补显示执行Grid Init／PostInit／Update／Reward、PlayAnimation／SetRoomTypes／SetLocked／Direction setter、破开／解锁／过门／换层或RNG。真实成员与最终资源／姿态按各自owner提交。
- 主／Timer／extra均需独立本地资源owner和glitch结果；draw中墙色／地板色及Room Light的Gradient／SpotLight修改须精确恢复，不把原生Render当纯函数。
- 门槽别名由Room结构事务维护，释放前撤销grid槽和所有door aliases并等待绘制借用结束，再正常析构双ANM2／四string；不能沿TargetRoomIndex释放别的房间，也不因primary Slot值创建或覆盖其它主体。
- SpawnSeed在这两类使用224排除，Client仍有工厂初始化的合法默认值；公共198及其它Grid保持旧合同，特别是TrapDoor的真实临时Light RNG消费者不能删除。

## 6. 生成、未决和第15步覆盖

普通Release／Win32 `isaac_cs_state` **纯构建成功**，生成224—228的capture／wire／client／member表及两张单行布局表，再编译静态状态库。没有运行单元、probe、smoke、harness或E2E，没有测试产物；所有功能测试仍留第15步。

本轮字节目录／线格式已落地，实际采集owner、target descriptor解析、门主alpha／bit5安装、计时文字上下文、资源依赖与draw恢复尚待第7／8／10／13步。UNKNOWN31B／32B保留，不能以静态构建成功代替画面、计时、生命周期或副作用验收。

第15步统一覆盖：普通／Greed压力板、TimerPlate有无资源／实际Offset及非variant2已加载成员、文字0／负计时与分钟边界／震屏／暂停／Host帧闪烁；门主alpha零／负／正及幅值变化、墙色与extra独立色／ExtraVisible、target type27／15、负特殊目标及目标Flags40000、维度与Layer3／5、Light位变化而其它规则位不重发、previous缓存／动画名变化但最终姿态不变、方向最终双旋转、资源等待不ACK、跨房／门别名与退休。拒绝非法布尔、截断／非有限、错use／布局、未知target／资源依赖；不为已删除的Host字段构造线上负载或恢复原值相等要求。静态无访问／自然阴性结果不证明27F及ANM2空白是padding。
