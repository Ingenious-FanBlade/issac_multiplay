# 原生转场的入口、阶段与写集

日期：2026-11-12；J273 静态证据，VA基址`0x400000`。任务范围见 [README](README.md)。

## 1. Wiki／SDK 给出的边界

原版 [Game.md](F:/projects_isaac/IsaacDocs/docs/Game.md) 的 StartRoomTransition 与[RoomTransitionAnim](F:/projects_isaac/IsaacDocs/docs/enums/RoomTransitionAnim.md)描述五个参数和不同动画，特别指出 Glowing Hourglass、复活、Death Certificate 等不是同一种普通门滑动。原版文档还提醒 PIXELATION 中断可能耗尽 RenderTarget 栈；这不是可忽略的纯动画恢复保证。

SDK [RoomTransition.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomTransition.zhl)声明 Update／Render／ChangeRoom／StartBossIntro，但未声明五参数 Start；[Level.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Level.zhl)声明 `ChangeRoom(int,int)`。官方 [LuaRoomTransition.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/Room/LuaRoomTransition.cpp)的 Boss API先准备安全 roomIndex，再调用两个u32参数的 StartBossIntro；不提供按网络定义注册完整转场的 API。

## 2. 启动入口与参数

| 入口 | ABI／证据 | 行为 |
| --- | --- | --- |
| Game::StartRoomTransition `0x6F9710`（RVA `0x2F9710`） | Lua类注册 `0x85408C` 将此地址关联 StartRoomTransition；thiscall，五参数 | 验证index／animation／dimension；可走换层、Hourglass、传送找位、Player位置／保护修改；`0x6F9AB0→0x747470` 可把目标改成特殊房号，再调用内部Start。Client不重跑 |
| RoomTransition::Start `0x823990`（RVA `0x423990`） | caller `0x6F9ABF—0x6F9AD4` 依次push dimension、Player、animation、direction、index，ECX=Game+1B83C；尾 `0x823D03 retn 14h` | 参数依次为 i32 index、i32 direction、i32 animation、Player*、i32 dimension。反编译生成的额外 EBP 参数不是 ABI；EAX无稳定成功返回语义，不用它作接受凭据 |
| RoomTransition::ChangeRoom `0x8254B0`（RVA `0x4254B0`） | SDK `void __thiscall()`；完整ZHL签名匹配 | 读取owner内目标字段并执行换房及后处理；没有“等待成功”返回值 |
| Update `0x826430`（RVA `0x426430`）／Render `0x8273C0`（RVA `0x4273C0`） | SDK `void __thiscall()`，无显式参数 | Update为混合FSM，Render读本地阶段／surface／versus姿态 |
| StartBossIntro `0x823D10`（RVA `0x423D10`） | SDK两个u32参数；调用方 `0x8258E5／0x825912`等 | 构造versus与肖像、播放音乐、清环境声及显示标记，不是只写Sprite的leaf |

Start还被 `0x741A62` 和 `0x909BB7` 调用，不能只挂 Game 外层便认为捕获完整。`0x823AC4` 检查已有 StageTransition，非特定动画可拒绝本次请求；入口采集不能直接当成功事件。

### 2.1 Start实际写入

RoomTransition偏移：mode `+0`，counter `+4`，duration `+8`，progress `+0xC`，direction `+0x10`，roomIndex `+0x14`，dimension `+0x18`，animation `+0x1C`，辅助 `+0x20/+0x24`，旧／新surface strong pair `+0x28/+0x30`，versus ANM2 `+0x38`，肖像map `+0x224`，滚动补偿 `+0x22C/+0x230`，过渡对象关联 `+0x234`，Boss标记 `+0x238`。结构资源不能以裸字节同步。

`0x823AF8—0x823B21` 写目标、counter=0、animation、direction、Boss标记=0和 `Game+0x1BB74` 的 Player*；负dimension转为当前dimension，同房／同维可把direction改成−1。后续枚举 Player，执行传送／死亡动画helper，**清 Velocity+0x360/+0x364 和 Player+0x410**，再设mode1。animation2 duration=30，其余按direction为−1取3、否则7；progress及两滚动补偿清零。animation18还经 `0x823CE1` 播放608／609来源音效。仅避开Game外层的选路不能避开这些直接写入。

## 3. Update阶段与副作用

本表是正常路径静态分类，counter按调用次数推进，不随Game帧增量补跑。完整异常／资源调用闭包未外推。

| mode／分支 | 正常逻辑与换房点 | 不允许在Client整根重演的部分 |
| --- | --- | --- |
| 0／其他 | switch无活动路径，直接返回 | 不用未定义mode制造隐藏推进 |
| 1：出房准备 | 玩家出场／传送姿态完成后，`0x8269EB→Room::PreRender`，`0x826A4F→Room::Render`捕获旧surface。若animation不在 `{1,2,11,15,16,17,18,19,20,21}` 且direction≠−1，则mode3并在`0x826AA6`立即ChangeRoom；其余mode2，animation15／21也在`0x826AC6`立即ChangeRoom | `0x8264D3`运行EntityList helper，`0x8264E4`运行环境音；多处虚调用Player Update（如`0x826557／0x8265C5`）、Grid Update、直接写Player Visible／状态、特殊minecart／随机放置等 |
| 2：普通遮罩，Boss标记=0 | counter<duration时`0x826BF3`加一；到期先counter=0、mode3，再在`0x826BE9`调用ChangeRoom | 如果hook ChangeRoom只返回“未就绪”，caller的阶段早已改变，后续会在旧世界进入入房段；必须在上层阻止本次阶段推进 |
| 2：Boss标记非零 | `0x826AE0`推进versus Sprite，随后推进肖像map动画；主动画结束后转mode3 | `0x826B70→0x7E16F0`选曲，`0x826B7B`写Game+22E90=1，`0x826BBC`把每Player+418提高至至少10；不是纯动画结束 |
| 3：入房显示 | counter<duration时加一；到期按animation决定进入mode4或结束，并恢复部分可见性／动画 | `0x826C10`清BossOverlay；多处分派Player动画／物品helper，直接写Visible、额外Player Update（`0x826F43／0x826EE4`）及其他状态；animation7的`0x826D79`声音见RE-8 |
| 4：等待Player动作结束 | counter自增；animation3且counter==8时`0x826FBA`播sound214；所有动作结束后mode=0、counter=0 | `0x826FD9`运行EntityList helper，`0x827036`虚Player Update，检查动画字符串／死亡状态；结束时写Player+410／Visible。不能以此等待Client不运行的Player模拟 |
| 共同尾部 | `0x8272DD`写progress=counter/duration；mode2／3时写Game+22E94／22E9C为2 | 这些Game写入须按真实HUD／显示依赖迁移，不能把整个Game原值写回当作隔离副作用 |

animation11／16的同房分支在 `0x826933—0x826967` 还直接处理玩家可见性／+410并结束；不能假设所有转场都必经三个ChangeRoom点。Hourglass animation12在ChangeRoom里另调 `0x701AA0`，属于Host回溯行为，不在Client恢复原版Save／Restore。

## 4. ChangeRoom链与显示职责

| 链／点位 | 已核副作用 |
| --- | --- |
| `0x825700→Game::ChangeRoom 0x6FA0F0` | ChangeRoom自身还会在前后写Player+410，保存旧scroll、算房间几何补偿、修改可见性／位置／动画，特殊动画可随机给队友放置 |
| Game wrapper `0x6FA14A`、`0x6FA171`、`0x6FA17C`、`0x6FA181` | 枚举Player运行`0x79E3E0`，调用Level::ChangeRoom、**额外Room::Update**、全局Random，再选择房间结果。不能只拦Level而保留wrapper |
| Level::ChangeRoom `0x739DE0` | `0x739E46→0x7E5D60`旧房处理；`0x739E80／0x739EB7`涉及持久／进度helper；写当前／前房索引与维度；Maze分支在`0x73A050／0x73A386／0x73A592`等调用Random后重选。Start的请求不保证是最终目的地 |
| `0x73A739→0x7385F0→0x738AD8→Room::Init 0x7E8AD0` | 完整初始化／生成链，按RE-2继续拒绝；不是单纯换资源 |
| `0x8258E5／0x825912→StartBossIntro`，否则`0x825924→0x7E16F0` | 进房Boss／音乐生产，Client应由对应Host事件／状态接管 |
| ChangeRoom尾 `0x826168→PreRender`、`0x8261DC→Room::Render` | 捕获新surface；`0x8261EB`以后flush队列、清图形缓存镜像，并可在`0x8262A2`显式SwapBuffers。它不是可在任意world安装点直接调用的简单结构setter |

Client替换需要保留的是：正常owner管理的两张**本地**surface；旧画面先捕获、新世界完整安装后捕获新画面；Host实际room／dimension／SpawnSeed／bounds／门／背景／Q8输入；本地相机初始化及`+22C/+230`滚动补偿；Boss／全屏演出资源与姿态。复用RE-2的窄显示资源顺序，不为补画面执行完整Init／旧房保存／额外RoomUpdate／选曲或Spawn。两张surface不由Host发送像素。

## 5. Game::Render的resize分支

`Game::Render 0x6F7AC6`在本机`Game+0x25AAC!=0`时调用`0x703E40`，`0x6F7AD4`清请求，随后正常PreRender。

| `0x703E40`中的点位 | 显示写集／owner |
| --- | --- |
| `0x703E50→0x823820` | 释放／重建RoomTransition+28/+30两张strong surface，名称Old／New Room Surface；`0x8238BB／0x823939`经原生共享owner赋值 |
| `0x703E62—0x703EE6`、`0x703F4E—0x703F8A` | 清多份全局Image strong pair；不能按地址字节清零或丢掉control释放 |
| `0x703EEB→0x7E08E0` | 当前Room的本地surface／Image重建；已查正常调用为图像创建／引用、采样设置，不据此宣称完整异常闭包 |
| `0x703F17` | Room+11F6=1，通知坑纹理重建 |
| `0x703F3E／0x703F46` | Room+11FC／+1200 surface origin，按本机逻辑尺寸计算，不安装Host分辨率结果 |
| `0x703F8F`及后续 | 重建shader／本机console显示缓存和字体／资源；存在资源分配和释放，不是只读路径 |

活跃转场中resize会丢掉旧surface内容，重新分配成功不等于画面可继续。Client需要记录本地surface准备／内容代次；无法重建已退休旧世界画面时，按明确的中断／重建策略处理，不能继续消费空surface或复制Host像素兜底。该运行场景留E39验证，本轮未声称任意窗口变化可恢复。

## 6. RoomTransition::Render条件与写集

`0x8273C0`只处理mode2／3；mode2且Boss标记`+238!=0`进入versus分支。其他路径按animation、direction和progress使用旧／新surface：滑动权重为`(3−2s)×s²`；fade／pixelation等路径按`0x827350`给出的phase alpha绘制，RoomTransition本身的mode/counter/progress不在本函数自增。

Boss分支读当前versus动画的有序layer；额外肖像map中存在且loaded时，`0x827618—0x8276E0`写对应本地ANM2：ColorMod `+A8` 44B、rotation `+A4`、scale `+9C/+A0`、offset `+94/+98`，随后绘制。这里的值由主layer重算，不能再按Host最终副本每帧纠正；必要资源／主姿态仍保留。map helper `0x827BD0`有插入能力，只在合法本地map／主线程owner范围使用，不通过它凭空补未知层。

其余写入是本地图形状态、顶点／共享图像引用和原生Sprite绘制缓存，按RE-5／E28处理；正常路径没有看到在Render本体推进Player Update。虚调用、官方渲染补丁、资源异常及实际GPU行为不外推为无副作用全闭包，Q6回调照现行范围放行。
