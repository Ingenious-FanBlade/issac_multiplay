# 第 6 步：Lock、Pit、Spikes、Statue、TNT、Poop、Rock

日期：2026-11-10（Asia/Singapore）。布局版本 **52**。目标：官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。IDA imagebase `0x400000`；下列地址为 VA，RVA 减 `0x400000`。只读核对了本地原版 Wiki、REP Wiki／ZHL／源码和目标 IDB 的工厂、虚表、Init、Update、Render、析构、Lua 注册及原始指令；未修改 IDB、启动游戏、注入、部署或运行任何测试。

**本轮完成指定七类的逐字节归属、同步职责、正式布局源和生成。整个第 6 步尚未完成**：PressurePlate、Door、全 119 项目录回映射及 GridEntityPlus 合法来源仍待处理。正式运行 reader／writer／绘制 adapter 归第 10／13 步；未知字节仍未知，不以“所有范围已登记”冒充“全部语义已知”。

唯一源为 [grid-classes.json](../../../mod/binding/state/grid-classes.json)。214—216 是三个明确的原生展示视图；217—223 是本轮七个真实 RTTI 类。原生字节归属与二进制布局两张单行表均由普通构建生成，见 [layouts 索引](layouts/README.md)。公共描述／索引／最终水面值仍按 [版本50公共合同](grid-common-synchronization-review.md)，父块只继承一次。

## 1. 同步决定

| 类／布局 | 实际原生范围 | 子类尾部：需要与不需要同步 | 主 Sprite／根 UNKNOWN |
| --- | --- | --- | --- |
| Lock／217 | `0x158` | `+154` 金币解锁 bool 留 Host；`+155..157` 3B UNKNOWN，不读不发 | 201，保留原色；根 20B UNKNOWN |
| Pit／218 | `0x158` | `+154` HasLadder 是碰撞控制，留 Host；`+155..157` 3B UNKNOWN | 地板色视图 202；根 20B UNKNOWN |
| Spikes／219 | `0x158` | `+154` i32 Timeout 周期倒计时留 Host | 202；根 17B UNKNOWN |
| Statue／220 | `0x158` | `+154` Entity* 是 Host 创建／销毁关联，不传地址或多造一个引用 | 自身 Render 为空，视图 216 无 Sprite 负载；根 17B UNKNOWN |
| TNT／221 | `0x158` | **`+154` i32 `_frameCount`／FrameCnt** 是爆炸倒计时，留 Host，不是绘制帧 | 202；根 17B UNKNOWN |
| Poop／222 | **`0x19C`** | 复生计时／控制名／碰撞标志、**40B EntityRef** 均留 Host；`+172..173` 2B UNKNOWN | 202；根 19B UNKNOWN |
| Rock／223 | `0x198` | **同步可绘 RubbleAnim 资源和有序 Vector 偏移**；Anim 控制名、FrameCnt、previous_state 留 Host | 215，保留与地板色相乘的原色；根 17B UNKNOWN |

Lock、Pit、Spikes、TNT、Poop 的格式最小值分别为 142／98／98／98／98B；Statue 为 24B。Rock 的条件编码格式下界为 136B，包含碎块块的资源 2B 和空列表计数 2B；非碎块、空层编码的下界为 147B。它们只是格式下界，不是已加载场景带宽或画面验收结果。Rock 可绘态需要实际资源、有效图层与真实偏移，不能拿空编码代替。

公共 17B UNKNOWN = Desc 间隙 3B + ANM2 递归 14B。Lock／Pit 再加各自 3B，Poop 再加 2B。即使 Statue 不发送 Sprite，仍完整登记该 ANM2 的 14B 未知；不通过删除线上状态抹掉原生未知。

## 2. 工厂、虚表和读取资格

`706100` 统一工厂只构造公共前缀、非平凡成员并安装虚表，最后在 `706636/706639` 写 type／index；它**不执行派生 Init／PostInit**。下面 new 地址均为实际 call 指令，区别于反编译标在返回后 `mov esi,eax` 的地址。

| 类／type | new call／请求 | vtable | slot2 Update | slot3 Render | slot10 InitSubclass | TD RVA |
| --- | --- | --- | --- | --- | --- | --- |
| Lock／11 | `7064D5`／158h | `B19F14` | `70C590` | `70CB80` | `70C3C0` | `7D2328` |
| Pit／7 | `7062F6`／158h | `B1A004` | `70CD70` | `70DE30` | `70E160` | `7D235C` |
| Spikes／8、9 | `70647F`／158h | `B1A2E4` | `718EE0` | `719270` | `718750` | `7D2390` |
| Statue／21 | `7065E7`／158h | `B1963C` | `82CEE0` | **`4291D0` 空函数** | `82D230` | `7D22F4` |
| TNT／12 | `706454`／158h | `B1A32C` | `719550` | `719270` | `719350` | `7D23C4` |
| Poop／14 | `706321`／19Ch | `B1960C` | `70F140` | `710130` | `710880` | `7D1374` |
| Rock／2—6、22、24—27 | `70616A`／198h | `B1969C` | `713C30` | `7146E0` | `714B90` | `7D14F8` |

上述对象范围由请求、实际成员构造／访问和析构共同核定，不把分配长度单独当作全部语义证明。五个 158h 类的工厂都没有初始化尾部；Factory 见证不授权读取 +154，更不授权读取间隙。Host-only 字段本就不采；后来若有新消费者，必须补真实当前 use 的来源资格，不能靠 Client 清零倒推 Host 可读。

Free：Lock／Spikes／Statue／TNT 共用 `706870`；Pit 为 `70CD20`；Poop 为 `7067A0`；Rock 为 `7066E0`。Pit Free 还在 Room+11F6 写 dirty 标志；Client 退休须处在已理解的本地 Room owner 中，不能把它当纯释放。它不读 HasLadder 尾部。

## 3. Lock 与 Pit 的 1B，不是两份 u32

### Lock

Init `70C4B0` 查询道具 380，`70C4C0` 为 `88 8F 54 01 00 00`（`mov [edi+154h],cl`），只写 1B。TryUnlock `70C730` 的 `70C74C/70C782` 根据它扣金币或钥匙、选择音效；`70C7F0` 的 `70C83F` 继续按它选择破锁行为。正常外观已由 `IdleCoin`／`Idle`／`Broken`／Break 等实际 Sprite 姿态承接。

Render `70CB80` 只读取公共位置、ANM2 loaded／overlay 顺序和两组姿态，不读该模式。不能为 Client 展示同步扣费模式或调用 TryUnlock；水面需要的是同实例最终 water_clip_flags，不能据 Lock 查询曾读碰撞重新发送玩法 collisionClass。该 Render **不覆写颜色**，故仍用 201。

`+155..157` 没有初始化或填充证明，登记为 UNKNOWN；不能按 u32 复制 +154，或因为模式只有 true／false 就读取额外三字节。

### Pit

Init `70E1C7`：`C6 86 54 01 00 00 00`，只清 +154 一字节；Lua 注册 `852509` 明确 HasLadder 的偏移为 154h。SetLadder `70F0C0` 只写该 bool 并调用 `70EFF0 UpdateCollision`。后者在 state==0 时据此选择 collisionClass=5 或 1，PathTrail 均为 3000；state>=1 分支不以该 bool 决定桥纹理。

桥的真实展示由最终 Desc.State、主姿态、layer1 的独立帧／visible 和实际 Image 承接。`70E8D0` 依据邻接选 pit 帧，并设置桥层；它还可能随机选孤立坑帧，不能在 Client 调用它补状态。MakeBridge `70F0E0` 根据父 type25／previous_state 决定 state=1／2，再更新碰撞，同样留 Host。

Render `70DE30` 无条件覆写地板色，层数>1时临时隐藏 layer1，绘制主体后恢复，并在原 visible 为真时单独画桥层。`70DFC0 TexturedPit` 使用 index、实际 layer0 Image 与当前选择器，也不读 +154。Light `801BE0` 的 type7 分支读公共 state／variant 和场景，不读 HasLadder。**不能因名称有 Ladder 就多同步一个可见 bool，也不能省略真实桥层。** +155..157 仍为 UNKNOWN。

## 4. Spikes、TNT 的计数，以及 Statue 的 Entity*

Spikes 的 Lua 注册 `8526C1` 将 Timeout 绑定到 154h。Init 在 `718989` 写 30，部分 variant 在 `7189C1/718B77` 写 `12*variant` 等周期值；Update `718FBF/718FCA` 读取／递减，`718FD6` 重置 60，`719089` 停用为 -1。这些值生产未来的升降状态／动画；Render `719270` 不读。Client 接最终 state／姿态，不推进周期或执行碰撞伤害。

TNT 的 SDK `GridEntityTNT.zhl` 明确 `_frameCount : 0x154`；J273 Lua 注册 `852612` 的 FrameCnt 是同一 int。Init `71951E—71952C` 先比较 state<4，再以 `lea eax,[eax*4-1]` 得到 3／-1，写入完整 4B。Update 的 state4 分支在 `7195C0` 递减，到零才生成爆炸／掉落／效果；`7195E2` 判断是否恰为零。Destroy `719CBE` 的立即路径写 -1。它与 ANM2 anim_frame 完全不同，不能用网络包到达时间、Client 帧或复制的倒计时再次触发爆炸。TNT／Spikes 共用的 Render `719270` 完整覆写 44B 地板色，均用 202。

Statue Init `82D230` 在 variant0／1 的普通路径创建并向 Room 登记真实 Entity，`82DC18` 写 `mov [eax+154h],ebx`。Destroy `82D14F/82D15B/82D15E` 读该指针、对目标调用虚表 +28h，然后清零；它是 Host 的销毁关联。非该创建路径不保证 +154 已写，不能采未初始化指针。

Statue slot3 `4291D0` 是空 Render；Init 没有装主 ANM2 资源。Room Light `801BE0` 仅有 type7、16、17、20、23 分支，type21 默认返回。当前支持的原版展示由独立 Entity 承担，不消费 Statue 主 Sprite，故 **216 保留 ANM2 原生账本，但所有三块均为空预留**，也不传 Dataminer 地址派生位。不能用 GridEntity 泛型路径重新读取这些无用状态，更不能直接发送尾部 Entity*、把它误改为跨进程引用或在 Client 调用 Init／Destroy 再创建雕像／天使／开门。

## 5. Poop：完整 19Ch，不按 SDK 截到 174h

| 原生范围 | 类型／证据 | 归属 |
| --- | --- | --- |
| `[154,158)` | i32 ReviveTimer；Hurt `71079B`／RespawnRedPoop `711673` 写 120，Update `70F7CC/70F7DB` 递减 | Host 复生控制，不发 |
| `[158,170)` | MSVC string StateAnimation；工厂 `706342—706360` 构造，Lua 注册 `852491` | Host 控制／公开兼容字段；当前 Init／Update 使用栈上 `State%d`，Render 消费已选 Sprite，不发头或名字 |
| `[170,171)` | bool UnderPlayer；`7108D2` 清零、`710A3B` 置1；Update `70FE33/70FF6F` 消费／清零 | Host 延后恢复碰撞，不是 Sprite.visible |
| `[171,172)` | bool，内部真实名字未声明；Update `70FE7C—70FF4E` 查询附近 Entity 后维护碰撞延后 | Host；`7108D2` 的 word 清零覆盖 +170/+171，无须读取作为展示 |
| `[172,174)` | bytes[2] | UNKNOWN，不判填充 |
| `[174,19C)` | 40B EntityRef，详下 | 全部 Host；伤害源记录，不是绘制颜色 |

EntityRef 的字段与 `EntityRef.zhl` 一致：type `174`、variant `178`、spawnerType `17C`、spawnerVariant `180`、position Vector `184`、velocity Vector `18C`、flags `194`、Entity* `198`。源中分为八个具名范围，合计40B，未以 opaque bytes 掩盖指针。Hurt `710751` 取 `lea ebx,[edi+174h]` 用于伤害来源复制，Render `710130` 不读该区；不要求来源坐标有限或非空，因为本路径根本不采该 Host 记录。

补充纠错：旧 [I13—I18笔记](../tx-04-18/tx-04-18-grid-native-research.md)曾将默认向量的反编译来源描述为 `a7` 栈值／语义未知。本轮原始指令 `70638F/70639F/7063AF/7063BF` 明确从**绝对全局地址 `BEB900/BEB904`**取值，写 +184/+188/+18C/+190；第二个 Poop 创建点 `7138B9—7138F1` 相同。因此不能把该默认值说成来自未初始化栈；本轮也没有运行时读取该全局初始化器，不将 IDB 文件内容外推为运行时零向量。旧笔记原文保留，当前结论见本文。

`+172/+173` 在其它 Entity 上常有访问。本轮通过寄存器来源区分了 Poop 对象与其创建的 Entity；例如 `70F6AE`／`719975` 的 +172 是新 Entity 的公共字节，不能误归 Poop 尾部。原版 Wiki 的 ReducedSpawnRate 明确已在 Repentance 移除；J273 注册只有 ReduceSpawnRate 方法，`452660` 实际写 Desc.VarData（对象+18）=1。没有证据把 +171、+172 或 +173 命名成 ReducedSpawnRate。

Poop Render `710142—71017E` 覆写地板色。variant7—10 的巨型便便中只有9在该 Render 绘制，其余块通过真实成员／对应资源体现；`713A20 PostInit` 可以创建另外三个 Poop，不能在 Client 调用来补成员。原生与线格式均保留主 Sprite 所有必要图层和真实姿态，改用地板色视图202。

## 6. Rock：两字符串、一个 vector 和两个 int

尾部68B完整登记：`[154,16C)` Anim string24B；`[16C,184)` RubbleAnim string24B；`[184,190)` vector头12B；`[190,194)` FrameCnt int4B；`[194,198)` previous_state int4B。工厂分别构造两字符串和空 vector；Free `7066E0` 按 vector、RubbleAnim、Anim、公共 Sprite 顺序析构。FrameCnt／previous_state 由 Init／Update 建立，不由工厂赋值。

### 两个名字的职责不同

Anim 仅供 Init／UpdateAnimFrame `7156A0` 等 Host 控制路径选择完整主姿态；既有主 Sprite 已承接，**不发 Anim**。FrameCnt 在 `713EDD/713EE6` 递减，控制炸弹岩石；previous_state 在 `7152FE/7143B4` 写当前 state，另被 Pit MakeBridge 使用；两者均不进入 Rock Render。

RubbleAnim 则在 **state==2** 的 Render `714801—71487A` 直接用于查找并切换动画，必须保留。正式源按当前实际 ANM2 文件转为稳定 u16 动画资源号，不能只根据 type／Backdrop 猜 `rubble`／`rubble_alt`。其它状态或 Sprite 无资源时编码空资源和空列表，**不读取**旧字符串或 vector 元素。合法可绘态缺动画／资源／owner时报 Gap，不用空值冒充成功。

### 有序碎块偏移

`714841/71484A` 读 begin／end，`714931` 每项前进8B。每项是 Vector，`7148B5/7148BB` 写到主 Sprite 的 **layer0.position**，不是邻接列表、绝对屏幕坐标或 Sprite 全局 offset。线上为 u16 count + N*8B，保持顺序和重复元素；禁止重排／按种子重抽。

Init `715101` 选择1—2条，`71510A—715249` 向已有 vector **追加**，该函数未先清空。工厂为空不证明多次 Init 后仍最多2条。因此格式上限取65535，并按帧／记录预算有界接收；这只是格式边界，**不声称原生最大为65535或2**。超限报 Gap／拒绝，不能裁成两条。只复制当前有效元素，不读取 capacity 尾部；Client 本地 owner 管理原生 vector，原生头／capacity／allocator 地址不上线。

### 不能同步上一回绘制的残留

每项以 `i=variant` 开始滚动计算 `i=(i+1)%frameCount`，强制选择 RubbleAnim、SetFrame 并停主动画；主资源／帧／两组选择器／is_playing 及 layer0.position 都被覆盖。它们只是上一项／上一回 Render 留下的 scratch，**state2 不读、不传、不用于跨端相等比较**。但非碎块态1／4需要真实独立主姿态，其它层也需要真实位置，不能全局删除这些字段。

214／215 用条件布局表达这个区别：

- 215 的动画块先带1B `has_main_pose`，必须与父 Rock `state!=2` 一致；为0时整个主 AnimationState 无负载且不读取，overlay仍保留真实状态。
- 214 的图层块先带1B `has_position`，只有父 `state==2` 且实际层序号0才为0；为0时8B位置无负载且不读取。其它层／状态仍安装真实位置。
- 存在性来自真实父状态／层序号，Client 必须核对，不能让报文自行选择分支。父块、资源、图层、碎块列表必须同一目标 world 事务，不能混搭不同时点。

普通生成器此前会将 `nested Vector` 直接降为 `copy`。本轮第一次纯构建因此报告 `GridRockLayerState.position: 条件字段须经具名处理器，不能无条件合并复制`，没有进入测试或运行路径。已修正为**条件或带具名处理器的纯值嵌套保留 Nested**；无条件、无处理器的连续纯值复制仍正常合并。修正后普通构建通过，条件位置没有落入无条件 memcpy 范围。

### 绘制不是可以随意调用的纯函数

Rock `7146E0` 除了 SetFrame 还在每项里调用 `404480 AnimationState::Update`，会推进 overlay；SetAnimation `405F50` 的 force 路径进入 `403DE0`，会 free／重建 selector 数组。原始 Render 不能直接作为 Client “只画图”入口，也不能浅备份指针后调用再写回，那会恢复已释放的地址。

后续绘制 adapter 必须使用已准备的本地资源／有主人的 selector scratch，按 Host 源快照、真实 pass 上下文与碎块顺序纯求值。主帧遵守原生32位有符号递增／取余、frameCount>0；主动画停播。overlay按源帧、is_playing、单精度 playbackSpeed、loop／终止规则求出每项实际姿态，不调用原生 Update、不使用 Client 帧数或墙钟，也不发布事件位。普通 ColorMod 必须先和同world Room 地板色按 `67E940` 的实际合成规则合成，再按作用域恢复；**Rock不能用省去原色的202视图**。

特别是多次 Render／水面／顶层等 pass 可能使 Host overlay 在同tick内继续改变；不能擅自假设“收到的基础overlay对所有pass都相同”。Host 最终绘制来源及 pass 序号／顺序必须由第8／10／13步的展示owner明确，必要时采集实际逐pass输出。这里确认了需要保留的状态和不能直接重演的生产过程，**没有把该真实来源依赖说成已接通或已证明画面一致**。未知pass来源报Gap，不能让Client本地累计推进补齐。

`7149E0 RenderTop` 仅在非state2的 type24 Pillar 分支绘制 layer1，临时强制 visible，使用该层独立 selector；不读尾部。其前后颜色和 layer1.visible 必须精确恢复。主 Render／RenderTop／反射等具体调用关系均须实际接线时统一管理，不能拿“主动画的总帧”替代独立图层选择器。

## 7. 本地资格与不能同步的内容

- 禁止裸复制对象、vptr／RTTI、string／vector头、Entity*／EntityRef指针、ANM2／AnimationData／Image／selector地址、allocator及capacity尾部；合法资源与容器由本地owner准备。
- +154等Host控制字段留Host，不是“为安全两端清零后比较”的字段；Factory没有写的字节不获得读取资格。Client涉及未来本地销毁路径的指针／控制字由本地owner中性初始化，不据此复制Host状态。
- 禁止 Client 为包到达调用 Init、PostInit、Update、Destroy、Hurt、TryUnlock、MakeBridge、RespawnRedPoop、UpdateNeighbors、随机选帧、SpawnDrops／天使生成等玩法路径。真实生成的 Grid／Entity／效果／音画由各自域发布，不从计数或指针重演。
- HasLadder／UnderPlayer 等公开可写字段仍记录原事实，但没有绘制消费者就不等于需要同步。公开 API 回映射不强迫Client保留Host玩法查询原值。
- UNKNOWN全部不读、不发、不写。不能因相邻状态能合并就跨过间隙，也不能把对象的本地loaded或Host initialized复制成资源准备／来源资格。
- 201／202／214／215的真实Image、overlay／独立selector、播放插值与每Sprite的Dataminer最终两位仍按各自合同；216无绘制负载，不计算无用故障位。资源/线程/owner不明或跨房退休时撤销关联。

## 8. 生成记录与后续统一验收

修正上述生成器降级后，执行 `cmake --build build/repentogon --config Release --target isaac_cs_state --parallel 4`，生成 capture／wire／client／member 表和全部单行文档表，并成功编译 Release／Win32 `isaac_cs_state.lib`。没有完整DLL构建、部署、单元测试、probe、smoke、E2E、游戏或实机操作。临时布局编辑脚本已清理；没有额外测试artifact。本轮没有更改旧JSON运行路径或声称这些具名handler已实现。

第15步统一验收覆盖：七类实际owner／type／RTTI／index与重建；Lock金币／钥匙及Unknown间隙不读取；Pit桥层／独立选择器／TexturedPit／HasLadder只改玩法；Spikes周期、TNT倒计时归零与最终真实爆炸成员；Statue variant0／1／非创建路径与Entity独立创建／移除、无重复天使；Poop普通／红色复生／巨型7—10成员与伤害来源不复制；Rock正常／碎块／炸弹／尖刺／Pillar顶层，重复Init追加、空／多条／有序重复偏移，非碎块／碎块来回切换、条件位与父state／层序号不一致、零frameCount／资源缺失／非法容器／超预算，overlay播放／loop／多pass的真实Host展示来源与精确恢复，以及跨房／退休／selector和vector释放。验收不得重新要求未上线Host计数、Anim名、EntityRef或地址原值相等。

字节目录已覆盖到各对象末端，但本轮各根UNKNOWN仍20／20／17／17／17／19／17B；图层元素外部另有4B既有UNKNOWN，不能计成根对象内已消除。PressurePlate、Door、全目录回映射和正式运行接线仍未完成。
