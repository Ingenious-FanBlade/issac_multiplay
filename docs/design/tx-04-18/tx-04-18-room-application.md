# tx-04-18 I07—I12：Room 原生应用

2026-11-02。本文记录同一 tx 的 Room 实现及静态依据；统一实机验收仍在 I29。

## 实现前失败模型

本轮在修改代码前确定以下失败处理；不编写单元测试，不提前运行分项测试。

- Data、OverrideData 分别为 Null、同一配置、不同配置、同查找键不同内容、普通／Greed 资源或版本缺失：分别解析完整内容，禁止合并两个指针；共享资源不重载、不修改，借用失效前先退休原生 Room。
- 配置候选不等于最终成员；任何构建不得执行 Spawn、PickEntry 或 Room.Init 来补布局。
- IH／IV 等窄房、四种 L 房、末端格、缺口和门槽：数组容量、房形有效区域和实际几何分开；拒绝超界、互相矛盾或不完整来源。
- Clear／NoReward／ChallengeDone 等标志与 Getter 冲突、计时倒退、重复目标：写前拒绝矛盾；明确零值及回到旧值实际安装，不触发奖励／波次或本地计时。
- 冷 Client 的未初始化字段、背景变化、环境清空、水模式变化、资源丢失：采用正式来源的最终缓存；清空同样执行，不依赖菜单堆残值。
- 轨道超过 255、末格、空轨道、L 房结构变化：不经过官方截断索引的 Lua GetRail；轨道不是 GridEntity，也不因轨道旗标创建 Minecart。
- 背景／持续 FX 的必要跨域对象缺失、资源预算超限、原生调用异常：具名报告，写前保留 A，写后撤显示并由既有 owner 安全退休；不提交半套目标。
- 任一配置／环境／结构来源 Unknown、Unavailable、Partial、版本不匹配：不补默认值，不将字段保留在 DOM 当作原生应用已经完成。

## 实现范围

| 任务 | 正式来源、纯值与原生应用 |
| --- | --- |
| I07 | `RoomConfigContents` 为每份配置复制 Stage/Mode/Type/Variant/Subtype/Shape、尺寸、名字、权重、Doors/QuestDoors/Flags、OriginalVariant、完整候选与 MergedData。`ConfigValue` 有界解析并与公开字段核对；`ResolveConfiguration` 只复用内容完全相同的已加载资源，否则 `OwnConfiguration` 构造自有深副本。两份指针分别安装，Override 可以为 Null；两份均空的未初始化离图槽保留空，不构造替代配置。普通/Greed 按独立 Mode 解释。不调用 LoadStageBinary、不修改共享配置、不执行候选。副本直到旧 Room 退休、descriptor 恢复后才释放。仅 Override 存在的非活动 descriptor 也保留正式身份。活动原生绘制需要 Data，不能把不满足原生前置的状态伪装成合法活动房间。 |
| I08 | `room_geometry.h` 使用主审读取的十二形矩形；数组 stride 分别为 15×9、15×16、28×9、28×16，窄房在大数组内取局部区域。`RoomApplication` 复制真实 bounds、L area/tile、门位、有效/内部 mask；解析核对形状、容量、mask 和门位范围。真实边界/门位先于Camera构造安装，改变配置/几何走既有退休重建。Dungeon 的 Grid 边界不再额外画普通背景墙/地板。 |
| I09 | `RoomState` 交叉核对 Flags 的 Clear/PressurePlatesTriggered/SacrificeDone/ChallengeDone/NoReward 与公开字段；原生安装完整 Flags、Visited/ClearCount、清房延迟、slowdown、Greed 波次/时钟、Ambush 状态/波次、商店计数、钱/便便/坑计数、boss、伤害与 pacifist 缓存。Room 帧数通过真实 baseline 表达；draw 内借用服务器 Game 帧，返回时恢复本地 Game 帧及 baseline。没有计时递减、奖励或波次调用。其它已采事实仍完整保留在提交 T：敌人/boss 汇总由19消费，整局随机/规则由25消费，音频由27消费；目录逐项注明，不声称将所有 Getter 写成私有字段。 |
| I10 | 复制实际选用的 BackdropConfig 十二条资源路径、七个构建参数、真实独立背景构建流及四个内嵌 ANM2 的主/overlay/层/精确姿态；冷 Client 由原生 Backdrop.Init 建立 surface，在真实 Player roster/EL 完成后收尾。18/26/35 等专用 floor 分支继续用真实 floorANM2；Null/清空会重置本 owner 的已加载 Sprite。复制原生绘制消费的 `unkVec1/2`，不赋予 SDK 未声明的额外语义。配置、构建流改变重建，单纯姿态改变不重抽背景。 |
| I11 | `RoomPresentation` v2 复制水量/流/模式/当前及 lerp 目标颜色、FXParams、最终 ColorModifier 三态、暗化/熔岩/雷雨缓存、shading、FX 已核绘制参数/已选 Image/pose、ray、poop 后态、vein 索引图及资源姿态。原生安装全部具名值，执行独立水面预绘制和合成、光照与最终 ColorModifier 合成以及 FX pass1—6。poop 和 vein 使用本方 Image/数组/节点 owner，先建候选再发布，退休断引用后释放；不执行 AddPoopFx、FX Load/Update、闪电触发、雨滴/余烬生成或颜色重新裁决。ray相位、轨道动画和vein的scale/rotation/ColorMod在draw后恢复。 |
| I12 | 在448容量内复制/安装当前真实 extent 的 rail 字节，处理0..447（含256以上）、255空轨道及矿车/矿道变体位；extent以外明确声明NotApplicable并清为255，不读取小房未采用的尾部内存。rail与Grid有效格mask分开，不把原生允许的轨道位置擅自当普通Grid成员。原生 RailManager 绘制，坑上的 rail 依赖后续 GridPit pass。L结构由相同 Data/形状和真实 bounds 供原生 Getter 派生，并保留已采矩形/mask用于后续 Grid/Camera。Minecart 是19的 Entity，镜像/矿道转换是26的流程；不调用进入矿道、镜子或换层来构建结构。 |

实现位置为 `mod/src/replica/room_configuration.h`、`room_geometry.h`、`room_grid_target.cpp`，以及 `mod/src/game/client_room_native.*`、`client_native_world.*`、`client_scene.cpp`、`client_presentation*`；正式只读 Lua 入口在 `world_sources/lua_entity.cpp`，采集在 `domains/room/collect.lua` 与 `domains/level/model.lua`。核心函数、数据结构、失败责任均有中文注释。没有新 Hook、网络线程对象访问或持久写入入口。

### 版本与预算

Room 应用来源 extension 升为2，`RoomPresentation` 升为2，完整配置内容和 Room runtime 分别为1；Grid 来源 extension 仍为1，观测 envelope/各域 schema 仍为2，ICSR/15模型线格式仍为1。能力改为 `room-grid-target-v2`，旧来源明确拒绝；不是静默扩展17代表 profile。`native_apply=false` 继续表示完整18场景尚需 I13—I25，不把本轮 Room adapter 冒称全部 Grid applier。

配置树深度最多8、每份配置最多65,535个 spawn、每个 spawn 最多255个 entry、合计262,144个节点/条目，仍受完整模型32 MiB预算限制。环境预算为256层×每层256 pose、256组×每组64 ray、256 poop pose；vein 最多4,096节点、深度32、每次绘制最多65,536次访问。超预算或循环在写前具名拒绝，不裁剪。成员完整性、空值和资源缺失沿用领域合同。

### 特殊 Room 上下文登记

RoomType 是同一 Room/descriptor 的值，不建立额外类。以下每项都使用来源实际 Data/Override、形状、背景/环境及成员；表中后续责任只指其它域消费，不省略本轮对应的 Room 状态。

| RoomType | 当前承载/专用上下文与后续接口 |
| --- | --- |
| 1 Default | 几何、背景、flags、进度、持续环境；最终 Grid 成员接 I14—I21 |
| 2 Shop | 商店索引/折扣/总钱计数；交易裁决25，货物 Entity19 |
| 3 Error | Error 背景26的真实 floorANM2/overlay 与离图 descriptor |
| 4 Treasure | 真实配置/背景；物品 Entity19，奖励裁决25 |
| 5 Boss | boss ID/进度/flags；boss Entity19、奖励25、音频27 |
| 6 Miniboss | 独立 Data/Override、原生最终配置；敌人19、奖励25 |
| 7 Secret | 背景与真实 flags；门体 I18，地图 I13 |
| 8 Supersecret | Source 已处理 light tint；门/地图 I18/I13 |
| 9 Arcade | 背景/进度；机器 Entity19，交易25 |
| 10 Curse | 背景/进度；门体 I18，伤害裁决25 |
| 11 Challenge | ChallengeDone/NoReward、Ambush 当前波次/active；敌人19，波次裁决25 |
| 12 Library | 配置/背景；物品19，选择裁决25 |
| 13 Sacrifice | SacrificeDone/计数及环境；献祭裁决25 |
| 14 Devil | 背景/flags；货物19、玩家20、交易25 |
| 15 Angel | 背景/flags；雕像 Grid I17/Entity19、奖励25 |
| 16 Dungeon | 实际几何/专用边界，避免重复背景墙地板；Grid I17/I21，夹层转场26 |
| 17 Bossrush | Ambush 当前/最大波数、每波boss数；Entity19、波次裁决25 |
| 18 Isaacs | 当前房间配置/背景/状态；角色/床等 Entity19，路线26 |
| 19 Barren | 配置/背景/flags；专用 Entity19、规则25 |
| 20 Chest | 配置/背景/flags；箱子 Entity19、奖励25 |
| 21 Dice | 真实已选状态/姿态；Entity19、骰子裁决25 |
| 22 Black Market | 独立离图 descriptor/配置/商店计数；货物19、交易25、转场26 |
| 23 Greed Exit | Greed mode 配置/波次/时钟；入口 Grid I17，换层26 |
| 24 Planetarium | 背景35及真实 floorANM2/overlay；物品19，规则25 |
| 25 Teleporter | 当前配置/环境与已提供引用；传送体 I17/19，转场26 |
| 26 Teleporter Exit | 当前配置/环境与已提供引用；转场26 |
| 27 Secret Exit | 最终入口/环境；门/活板门 I17/I18，路线26 |
| 28 Blue | 当前配置、真实维度/入口索引和 flags；地图 I13、转场26 |
| 29 Ultra Secret | 背景/flags 与 Source 最终混色；门 I18、地图 I13 |

轨道不是 GridEntity；矿车标志不触发车辆构造，车辆稳定引用来自完整 `room.entities`/Entity域。镜子当前维度与原生 descriptor 由 I06 安装，整体 Camera/镜像显示由 I13，跨维度流程由26。Backdrop57 Home 的专用 light/foreground pass 直接消费 `Effect166/sub951`，当前明确报告 `presentation_dependency_I19_home_effect166_951`，等待19提供真实主体接口，不用普通背景或虚构对象替代。

## 资料顺序、Luna研究与主审

先读原版本地 Room/RoomDescriptor、RoomShape/RoomType/BackdropType Wiki 与 REPENTOGON 的 Room、RoomConfig、FXParams、RailManager、LRoomAreaDesc/LRoomTileDesc；再读官方 SDK zhl、`IsaacRepentance_static.cpp`、`LuaInterfaces/Room/LuaRoom*.cpp`、`LuaBackdrop.cpp`。未公开的具体需求由一个 GPT-6 Luna / max 子智能体只读逆向；主智能体随后直接复读以下 J273 伪码/指令/常量，不以子智能体摘要替代主审。

| VA / RVA | 主审事实与采用范围 |
| --- | --- |
| `809590 / 409590` | 首指令检查 Room+0 **初始化字节**，不是 descriptor；初始化时返回 Game+25A70 减 Room+11F0。主审纠正了 Luna 最初“descriptor在+0”的措辞；descriptor 在+4。frame 仅作为状态，不随 Client draw 推进。 |
| `7E86C0 / 3E86C0` | true 分支仅写+14/+18/+1C/+20边界；窄房读BAC490形状矩形，Stage35/Type16有专用边界。false 分支会分配、扫描格子，本轮不使用。最终实际 bounds 仍来自Source。 |
| `814DE0 / 414DE0`、`BAC490` | `Room+4 -> Data+10 -> Shape+48`，复制8个DWORD。主审直接读取十二项共96个DWORD，确认两矩形闭区间。官方 `IsValidGridIndex` 的 IIV 内部列条件和 LBR `%27`存在具体笔误，本轮不照搬，使用已核矩形并集。 |
| `814B70 / 414B70` | 非L返回真实Room bounds两份；L形按同表转换到世界坐标。只读、无随机；本轮保留实际area与tile，不调用GetRandomTile。 |
| `7E8410 / 3E8410` | ECX slot、EDX width、栈height/shape、caller清8；纯整数几何。本轮安装Source真实门位并检查范围，后续Door主体仍在I18。 |
| `9B7C70 / 5B7C70` | ECX RailManager、栈offset，ret4；循环允许index447，跳过255/负类型及坑；只选rail动画帧/绘制，不造Minecart。官方Lua GetRail的uint8索引截断不用于本轮来源。 |
| `9B7F00 / 5B7F00`、`405F50 / 005F50`、`403DE0 / 003DE0` | 主审收尾发现rail/pitrail两路径都会以reset=true选动画，403DE0会free并重建main的layer/null索引数组；仅保存旧AnimationState再原样归还会造成悬空指针。修正为draw前暂摘原数组、native只构建临时索引，离栈用其同一`A86BC9 j_j_free`（cdecl、跳import free）释放临时数组后归还完整原state。水面816535/816708的两个rail调用也纳入同一保护；允许来源姿态为rail或pitrail。Luna补查与主审复读一致，不外推全程序别名。 |
| `917870 / 517870` | Init 写Backdrop ID、unkVec1=(52,52)，unkVec2由真实Room宽高产生，loadGraphics=true才进917910资源构建。另以正式来源复制最终向量，避免把它们推断成其它语义。 |
| `918A60 / 518A60`、`9191F0 / 5191F0` | 原生floor/walls；18/26/35等分支使用真实floorANM2，Mega Satan墙分支仍调用该同一动画。`425EA0`仅取Data.Shape；采用分支没有玩法生成入口。 |
| `816250 / 416250` | 原生独立水面surface预绘制，ECX Room、无栈参数、ret；使用真实成员/资源和已核绘制入口，内部在非空FBO Present并恢复。含Grid绘制依赖，完整Grid时由I21供给；本轮不绕过尚未支持的Grid。 |
| `816BB0 / 416BB0` | ECX Room、栈Backdrop offset、ret4，水量>0且Manager+2A203才采用。消费的6AE0向量是Backdrop.unkVec1/2，1B30等是FXParams水色；均已补正式来源/安装。仅构建shader packet，Game帧/lerp状态来自Source。 |
| `803E20 / 403E20` | 主审复读最终shadow fill/light tint，包含pitch black、darkness、Source楼层/房间条件和lightning。只在Source计算；Client复制最终值。`748F30/742450/81CB60/7489A0`为现有条件只读查询，不推进随机。 |
| `96CC00 / 56CC00`、`96CD10 / 56CD10`、`96E2B0 / 56E2B0` | 分别是pass3/2/6；96C620只读已采参数/pose并写渲染packet。pass5的96DF80在原生水面预绘制内消费；pass4为既有96DB20，pass1/ray/poop为96CE20。只见pass1—6消费者，pass0明确拒绝。原先按stride复制12个word过宽，收尾改为具名pass、非mask分支的+3c有限float条件、mask分支的+40整数和+48低字节bool；不读取/传输其它语义未证实word或不采用分支的值。 |
| `9E2210 / 5E2210`、`968E10 / 568E10`局部 | Luna补查并由主审复读SetVertexFormat和描述符原始值：BA47B0为shader1/36字节，B88734为shader3/44字节mask，B88760为shader3/88字节。正式来源逐属性复制，Client按实际格式加载；96C620每44字节顶点写WrapSize，不可用88字节格式混代。FX+48仅采实际bool字节，不复制尾部padding。 |
| `96EA50 / 56EA50`、`96EAB0 / 56EAB0` | 十根vein节点，节点stride34，三个child指针改为网络索引；只复制kind、rotation、scale/factor/opacity、depth、position。+10/+30不消费且不赋予新含义。Client自有图先断原生根再释放，不把自有节点交原生未知析构。主审确认EAB0不SetFrame，但写两只Sprite的+9c/+a4/+a8（scale/rotation/44字节ColorMod），完整draw栈已保护并归还这些值。 |
| `96E570 / 56E570`、`96F6B0 / 56F6B0` | **不调用**前者的随机生成。主审核`96E712 lea ecx,[edi+44]; call96F6B0`及后者的begin/end/capacity三指针、20字节stride，确认poop vector布局；最终Image、pose、颜色/参数直接安装。 |
| `6EFB70 / 2EFB70`、`6F7A80 / 2F7A80`局部 | 前者只准备原生全局图形surface；后者仅作为静态来源复读ColorModifier合成：BAB2EC shader、BE72E4 surface、48字节顶点stride，供应Source最终六值。本轮不调用完整GameRender/703E40。HUD在颜色合成之后画。 |
| `7DF8F0 / 3DF8F0`、`7E0190 / 3E0190` | Room ctor将RailManager._room指回本Room；主审完整复读dtor，不读+0初始化位，逐448格和资源容器清理，不因本轮initialized=true新增保存/奖励路径。退休继续复用17顺序。 |

原版/REPENTOGON 的完整未知 owner、异常/EH 全局恢复和任意第三方 gameplay mod不被这些局部事实外推为支持。已知必要依赖按域保留；任何真实写入后的失败撤显示、撤提交并由既有场景owner退休，无法证明恢复安全的原生异常继续保留隔离。

## 验证与后续

I07—I12代码和正式来源已实现并完成上述静态主审。`pwsh -NoProfile -File scripts/build.ps1`纯编译/链接已成功生成Win32 DLL及Rust harness；编译出来的fixture/contract程序没有执行。首次沙箱内MSBuild停在Checking File Globs，主动终止后经授权在沙箱外完成构建；SDK/RapidJSON及既有Lua局部名遮蔽警告保留。收尾修复后已重新构建。未启动游戏、未执行probe、分项测试、E2E或单元测试；I29仍未执行，不报告像素/生命周期实机通过。

收尾静态复核另修正了地图占用对Data房形与Override替换房形的混用，冷Client的相同完整配置复用同一自有资源；Source及Client均按官方RailType/IsValidRailType等价表拒绝非法轨道字节。目录补明保留事实的Grid、Entity、整局规则、保存及转场消费责任。

完整场景仍有I13地图/Camera、I14—I25 Grid、19特殊Entity、20玩家及27完整HUD等明确依赖。现有17的普通Isaac/Effect57与Lost地图代表限制保留在所属域，不能把Room接口已实现当作这些其它任务也已完成。后续I28/I29应覆盖本文件失败模型及E01/E02/E03/E08/E09/E13/E14/E17，使用同一正式来源和场景入口连续修复/续测；不为本轮创建artifact或提前试跑。
