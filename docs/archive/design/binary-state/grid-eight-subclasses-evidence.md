# 第 6 步：Decoration、Gravity、Stairs、Teleporter、Wall、Web、TrapDoor、Fire

日期：2026-11-10（Asia/Singapore）。布局版本 **51**。目标为 J273 / 官方 REPENTOGON 1.1.3，IDA imagebase `0x400000`，下列地址均为 VA，RVA 减 `0x400000`。本轮读取本地 Wiki、ZHL、官方源码、现有研究及目标 IDB，核对工厂、虚表、Init／Update／Render／Free 和原始指令；没有修改 IDB、运行测试、启动游戏、部署或注入。

**本次八类的字节目录和生成布局已完成；整个第 6 步尚未完成。** 其余九类、全 119 项目录回映射、GridEntityPlus 来源及实际运行接线仍待处理。已登记每个字节的归属不等于语义全部已知：七类根对象各保留 **17B UNKNOWN**，Fire 根对象保留 **45B UNKNOWN**。对象范围按 J273 工厂请求和实际构造／访问核定，不把 SDK 的 `sizeof` 或分配请求单独当成完整语义证明。

正式源为 [grid-classes.json](../../../../mod/binding/state/grid-classes.json)，公共源仍为 [grid.json](../../../../mod/binding/state/grid.json)。布局 202—205 为明确的展示／元素视图，206—213 为八个实际 RTTI 子类。原生字节归属表、状态二进制表、各范围证据均由生成器输出到 [layouts](../../../../mod/binding/layouts/README.md)。当前 reader／writer 名称是正式接口合同，不能冒充已经运行。

## 1. 每类结论

| 实际类／工厂 type | 原生范围／布局 | 子类字节的归属 | 最小线状态，不含记录头 |
| --- | --- | --- | --- |
| Decoration / 1 | `0x154` / 206 | 无尾部；主 Sprite 用地板色视图 202 | 98B |
| Gravity / 19 | `0x154` / 207 | 无尾部；主 Sprite 用 202 | 98B |
| Stairs / 18 | `0x154` / 208 | 无尾部；主 Sprite 用 202 | 98B |
| Teleporter / 23 | `0x154` / 209 | 无尾部；主 Sprite 保留颜色，用 201 | 142B |
| Wall / 15 | `0x154` / 210 | 无尾部；只有 RoomType=16 时覆写颜色，其余分支需要原色，仍用 201 | 142B |
| Web / 10 | `0x154` / 211 | 无尾部；主 Sprite 用 202 | 98B |
| TrapDoor / 17 | `0x158` / 212 | `+154` 的 4B 更新计数归 Host；主 Sprite 用 202 | 98B |
| Fire / 13 | `0x390` / 213 | 两个 ANM2 展示视图、两个 Host 标量、12B 本地 vector 头及有序粒子投影 | 372B |

最小值仅是空图层／空选择器／空粒子编码的格式下界，不能当实际场景带宽或画面已经实现的证据。已加载动画的实际选择器、图层与资源依赖按真实数量编码，不为凑下界丢弃。

八类均继承公共 G0—G3 一次：描述四项、index、最终 WaterClipFlags 及主 Sprite。公共 SpawnSeed 的 Light 消费者仍有效；不上线的 SpawnCount／VarData／Initialized／VariableSeed、SpawnFrame、玩法 RNG、碰撞原值不因本轮重新传输。只更换五类主 Sprite 的嵌套展示视图，没有改变它们的原生父类或字节位置。

## 2. 工厂、RTTI 和等大六类

`0x706100` 工厂在分配后调用公共构造 `0x704FB0`，安装实际虚表，最后在 `0x706636/0x706639` 写 type／index。它不执行派生 Init／PostInit。六个等大类均只构造公共对象和换虚表，没有子类尾部写入。下面列出工厂调用 operator new 的指令以及 vtable 主要槽；没有把工具为无操作函数显示的错误函数名当成语义。

| 类 | new 指令／请求 | vtable | slot2 Update | slot3 Render | slot10 InitSubclass |
| --- | --- | --- | --- | --- | --- |
| Decoration | `70613F` / `154h` | `B196CC` | `972EB0` | `973190` | `973250` |
| Gravity | `706560` / `154h` | `B2E6DC` | `973F40` | `719270` | `973AE0` |
| Stairs | `706535` / `154h` | `B2E758` | `974290` | `719270` | `974090` |
| Teleporter | `70660F` / `154h` | `B196FC` | `82DC60` | `70CB80` | `82DED0` |
| Wall | `7062CB` / `154h` | `B1972C` | 无操作 `408030` | `71B4E0` | `71B5D0` |
| Web | `7064AA` / `154h` | `B1A578` | `71BCE0` | `719270` | `71BAA0` |
| TrapDoor | `706500` / `158h` | `B1A484` | `71A6E0` | `719270` | `719F90` |
| Fire | `7063ED` / `390h` | `B1978C` | `67AA20` | `67B620` | `67A430` |

旧资料把请求归到分配后的 `mov esi,eax`（如 Fire 的 `7063F2`）也可定位，以上区分实际 call 指令。TD RVA 为 Decoration `7D2214`、Gravity `7D3200`、Stairs `7D3238`、Teleporter `7D22BC`、Wall `7D224C`、Web `7D2430`、TrapDoor `7D23F8`、Fire `7D21E0`；只用于本进程识别，不进入负载。

不能因六类无尾部就省略继承状态或执行其玩法方法补画面。Gravity PostInit `973AF0` 按场景选择资源／动画，Update `973F40` 改玩家标志；Wall PostInit `71B870` 依赖邻接并随机选砖块动画；Stairs `974290` 和 Teleporter `82DC60` 会操作玩家／房间转移；Web Update／Destroy `71BCE0/71BF70` 操作最终状态／动画和效果；Decoration `972EB0/973250` 也有非纯绘制工作。这些生产过程留 Host，Client 安装最终资源、状态、姿态及独立产生的 Entity 成员。

本地公开资料方面，原版 Wiki 的 GridEntity 和 GridEntityType 作为公共接口／类型索引；REPENTOGON 的八个类页面均未暴露这些尾部字段，`GridEntityFire.md` 明确无专有内容。官方 `LuaInterfaces/GridEntities/LuaGridEntity*.cpp` 的八个 To 方法只按 type 转元表，没有隐藏状态 Getter；不为研究而调用它们。旧应用目录第 27/29/30/37/40/41/42/43 行的 ToDecoration／ToFire／ToGravity／ToStairs／ToTeleporter／ToTrapDoor／ToWall／ToWeb 已在各类 `members` 映射为实际类关系，无额外负载；没有宣称其余目录已全量回映射。

## 3. 五类必覆写的颜色，与 Wall 的区别

Decoration `973190` 在 variant!=1000 且 loaded 时，从 `Room+72A4` 读取全部 44B ColorMod，写入 `Grid+E8`，再绘制。Gravity／Stairs／Web／TrapDoor 的共用 Render `719270` 在每次绘制前也做同样的完整覆写：`7192B3—7192BE` 写完 `+E8..+113`，随后使用两个 AnimationState 绘制。没有实际画面消费者需要这五类旧 Sprite.color。

因此 202 **GridFloorSprite** 将 ANM2 `+A8..+D3` 的 ColorMod 判为派生，G1 从 94B 降为 **50B**，整体从 142B 降为 **98B**。它仍是 114h 的 ANM2 原生视图，保留实际 Image、图层颜色、姿态／独立选择器、播放插值、冠军前四值和每个 Sprite 的 Dataminer 最终选择位，14B UNKNOWN 不变。地板色由同 world 房间来源提供，缺来源报 Gap，不以白色或历史颜色补齐；绘制借用后精确恢复本地状态。

**Wall 不能套用这一裁剪**：`71B4FD` 先判断 `Room+8==16`，只有该分支才在 `71B529—71B534` 覆写颜色；其它房间直接消费当前 Sprite.color。Teleporter `70CB80` 也不做地板色覆写，Fire 的三份 Sprite 同样没有这项覆写。它们保留颜色是正面消费者的结论，不是沿用旧可写字段清单。

生成器新增通用 `native_view_of`／`nested_views`：仅允许真实子类替换父类的直接等大内嵌视图，并保留原视图各字节边界、名称和 UNKNOWN。块号仍继承，不伪造派生原生字段，也不修改公共 GridEntity 的 201 视图。生成结果按每个实际子类的 nested_layout 递归；不能硬编码父类布局号后又读取被排除的颜色。

## 4. TrapDoor 的额外 4B

| 范围 | 实际证据 | 同步决定 |
| --- | --- | --- |
| `[154,158)` u32 update_counter | 工厂 `706521`：`C7 86 54 01 00 00 00 00 00 00`，清零 4B；Update `71A713` 递增；`71A876` 将该值与 `96h=150` 无符号比较，参与特殊开闭条件 | **Host 权威**；不读作展示负载，不发、不写 Client，不作跨端原值比较 |

InitSubclass `719F90` 依据路线／房间选择资源，并可能创建独立效果，尾部结束时把 Desc.State 设 0、选择 Closed 动画；不是无副作用构造器。开闭结果已由公共 state／Sprite 承接。实际 Render `719270` 不读 `+154`，Light 的公共依赖也没有该计数；Free `706870` 只析构公共 Sprite 并释放对象。这四字节不是填充或未知，但它也不是绘制年龄、动画帧或跨层授权。

## 5. Fire 的完整尾部与粒子元素

工厂 `706403` 构造公共部分；`70640C/706418` 为 `+154` 调 ANM2 ctor；`70641D/427/431` 清空 vector 的三个指针；`70643B/706445` 为 `+27C` 调 ANM2 ctor。第三 Sprite 恰好结束于 `390h`。Free `7067E0` 先析构第三 Sprite，再析构 vector、第二 Sprite、公共 Sprite。两个标量不由工厂初始化，不能仅有 Factory 见证就读取／发出它们；本方案本就不采这两个 Host 字。

| Fire 原生范围 | 类型与语义 | 归属／线状态 |
| --- | --- | --- |
| `[154,268)` 276B | 第二 ANM2，火焰 stage | **递归展示状态**，复用 201 的三投影，最小 118B；真实资源／姿态保留 |
| `[268,26C)` 4B | 发射冷却 i32 | **Host 权威**；`67AC36` 判定，`67B2D4` 写 84，`67B2E0—67B2EB` 递减；Render 不读 |
| `[26C,270)` 4B | 0—2 样式索引 u32 | **Host 权威**；Init 随机选择，Update `67AAC7` 取值后索引 NoFire／Flickering 名称表，最终主 Sprite 已承接；Render 不读 |
| `[270,27C)` 12B | ember vector 三指针 | **本地容器头／有序值投影**，指针和 capacity 不发；线为 u16 count＋每项 16B，最多 82B |
| `[27C,390)` 276B | 第三 ANM2，所有 ember 共用模板 | **递归展示视图 204**，最小 110B；主动画用 203，排除已被逐粒子 SetFrame 覆写的帧／选择器 |

尾部共 **572B**，无未登记空洞；两 ANM2 递归各有 14B UNKNOWN，连同公共 17B 得 Fire 根未知 **45B**。正常颜色全部 11 值保留，冠军仍仅前四 float；默认动画名、待加载路径、事件位和本地准备状态不恢复传输。第二／第三 Sprite 都在 Room 的 Dataminer pass 内绘制，必须各自计算并发送所属 ANM2 地址导出的最终两位，不能复用主 Sprite 的位或发地址。

原生 ember 元素步长 24B，全部字节已核：

| 元素范围 | 类型／用途 | 决定 |
| --- | --- | --- |
| `[0,8)` | Vector 相对位置 | **同步 8B**。`67B774/67B785` 加到 Grid 当前渲染位置；不传 Host 屏幕坐标 |
| `[8,10h)` | Vector 速度 | **Host 权威 8B**。Update 积分并随机扰动，Render 没有读取；Client 本地元素速度可中性初始化，但不模拟／不比较 |
| `[10h,14h)` | i32 age | **同步 4B**。真实绘制帧公式输入，不能仅按“计数器”名称裁剪 |
| `[14h,18h)` | i32 max_life | **同步 4B**。真实绘制帧公式除数，不能当只用于到期删除的玩法值 |

`67B731/67B738` 分别读取 age／max_life，转单精度相除；`67B747` 读当前主 AnimationData+30h 的 frameCount，相乘后 `67B753` 截断为整数，再由 `67B75E` 调 SetFrame `4042A0`。这是直接绘制依赖；继续使用原生粒子绘制时保留这两个真实输入，没有引入 Client 粒子更新器。

### 数量、寿命和隐藏态

Update `67B326/67B329` 仅在 count<5 时新增一个粒子；`67B336` 用 RNG `%30` 产生 maxLife=0—29，初始 age=0。其后同次 Update 积分位置、增加 age，再删除 age>=maxLife 的元素。因此正常普通完整采集尾部最多 **5** 个，存活粒子满足 `0<=age<maxLife<=29`；maxLife=0 的临时元素已在返回前删除，不能在本采集窗口把它视作可发合法除数。这里是原生指令推导，没有运行时观察；后续若真实受支持来源超出此条件，报告 Gap 并复核来源，不能截断为 5 或伪造寿命。

`67B709` 对 **有符号** Desc.State 做 `<4` 判断；不是 `0<=state<4`。state>=4 的 Render 不遍历粒子，即使 Host Update 仍在维护列表。具名 Host 投影此时编码空列表，不读取粒子数据；重新可见时从同窗当前有效列表全量生成，不能复活旧列表。该投影不改变 Host vector，不影响 Entity／Grid 的真实成员生命周期。

### 第三 Sprite 主动画哪些不传

`4042A0` 每次依据该粒子的公式结果覆写 anim_frame，并把两组图层／空层选择器从 0 重新查找，因此它们只是上次粒子 Render 的残留，不应同步。203 保留 **active_animation u16＋is_playing u8=3B**。SetFrame 没有写 `+14` 的 is_playing，而官方 `Patches/InterpolV2.cpp` 的 AnimationLayer::RenderFrame 直接检查它，还读取 anim_frame／playbackSpeed 做插值，所以 is_playing 和 Sprite.playback_speed 不能删。

overlay 没有被上述主 SetFrame 覆写，仍保留完整 AnimationState 及其独立选择器。SetFrame 还调用 `404130` 改两组事件位；这些位保持不上线，不能据此派发 gameplay 事件。Client 需要本地合法资源／选择器数组，在绘制 scope 中精确恢复主帧、两组数组、事件位等临时修改；未知字节不触碰。不得把模板的上次残留帧安装后直接画所有粒子，也不得把 203 套到第二 Sprite 或一般动画。

## 6. 后续实际接线的硬性语义

- **Host**：只在已理解的普通完整主线程窗口读取当前真实 RTTI／index／use 和合法 Sprite／容器 owner；粒子按位置 8B、age/maxLife 8B 连续段读，不读速度或 capacity 尾部。纯值快照与日志解耦。模板与粒子必须同 world／同资源版本。
- **Client**：工厂仅建立合法本地对象、虚表、ANM2 和空 vector；资源／数组准备后按生成布局安装。新 packet 不得调用 Init／PostInit／Update／Destroy／Hurt、生成 projectile、重演换层、随机选动画或刷新 PathTrail 来补状态。公共碰撞中性及最终 water_clip_flags 沿用版本50合同。
- **粒子容器**：线上 16B 不等于原生 24B。按本地 vector owner 建立 24B 元素，只安装位置／真实 age／maxLife，速度本地中性；禁止将 Host 三指针或 16B 线上数组直接当原生 vector。若借用 C/S 自有存储，Free 前先 detach 并 drain 绘制借用，不能让原生析构释放外部 `std::vector` 存储。
- **禁止直接同步的内容**：vptr／RTTI 地址、ANM2／AnimationData／Image／选择器数组指针、vector 头／容量、资源 loaded／来源 Initialized 资格、动画事件位和所有 UNKNOWN。资源传稳定编号，准备状态由本地 owner 建立，无法确定来源／资源／线程时保留 Gap，不补零 ACK。
- **旧路径退役**：旧 ClientGridDescription 的 fire_timer／fire_style、完整 ember 速度、三个 Sprite 的旧通用姿态相等检查均须按本合同修改。不能因为旧 JSON 仍存在这些字段，就在二进制路径重新采集或写回。历史 E2E／旧研究结论保留原时点，不改成当前字节合同已实测。

## 7. 生成、静态复核与未决

Release／Win32 `isaac_cs_state` 纯构建已成功：生成 202—213 的 capture／wire／client／member 表和原生／线格式单行表，并编译静态状态库。没有运行单元、probe、smoke、E2E 或游戏，也没有测试产物。第15步仍是唯一统一功能验收。

后续统一验收必须覆盖八类真实来源与重建、Decoration variant1000、Wall 在 RoomType16／非16、五类地板色变化与 draw 恢复、TrapDoor 计数变化不单独发包及最终开闭变化、Fire 熄灭与再次可见、0／5 粒子／有序删除、原生 24B 与线上 16B、寿命非法／超限拒绝、模板不同主帧／独立 overlay／插值开关／Dataminer 两端不同地址、资源等待／跨房／退休与容器释放。只有未传 Host 值变化且最终保留状态不变才要求块不重发；不把“自然静止”当作 Fire 不变化的保证。

UNKNOWN 是公共 Desc 3B＋主 ANM2 14B，以及 Fire 两份附属 ANM2 各14B，未消除；未提前以实验判填充。来源／writer／绘制恢复／资源解析器与 Client vector owner 还没有接入运行路径；没有把静态构建记为画面或生命周期验收通过。
