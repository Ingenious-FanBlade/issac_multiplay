# Effect 各臂与附属 payload 同步职责复核

> 版本40后续处理见[Effect五项未决](effect-next-five-evidence.md)：新增五个原生VarData臂、113单列，合计11个Effect布局；FX0改16B、FX1改13B，Trail上限改127；Timeout有双减／刷新。下文保留版本38当时的六臂、块划分和通用上限，不覆盖版本40合同。

日期：2026-11-09（Asia/Singapore）。属于第 4 步，布局版本 **38**。本轮使用本地 Wiki、官方 REPENTOGON 1.1.3 源码及 J273 IDA 静态分析；没有构建、运行游戏或测试。第 15 步仍是唯一功能整体验收。布局和生成链已修订，实际 reader／writer／展示借用仍待第 10／13／14 步接线。

## 1. 结论与范围

撤销 Effect 六项公共更新输入、TearFlags 臂的完整 128 位及 136 描述八成员的传输，共 **15 个原生范围**。State、Timeout、LifeSpan、IsFollowing、ParentOffset、Rotation、Scale 有实际绘制消费者，继续保留。六个布局编号 101—106 不变，全部 FX2 显式预留；不因线上内容相同合并原生语义不同的臂。

三个附属 payload 都有真实绘制用途，不能整体归 Host：MrMe 保留独立 Sprite；Trail 保留完整有序最终节点；Ghost 只发 `[0,count)` 有效 Sprite 前缀。Ghost 的 Offset／Scale 由绘制现场派生，原生 i32 count 由三投影一致的 u16 count 重建。它们均不从资源路径、Sprite loaded 数或本地时间猜测。

MrMe 原未知 16B 已核为 Host 内部目标处理／碰撞类别暂存／格子序号／动作计数；不再发送或要求 Client 重建。Trail 找到真正的 64B 节点分配和完整复制，撤销旧“仅已访问前缀、producer 未定位”的限制。其他原 UNKNOWN 不因同步裁剪而消失，Ghost 未发送槽位中的未知字节亦保留。

## 2. 证据范围与具体消费者

目标 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；下文均为 VA，RVA 为 VA−`0x400000`。未读取或校验哈希。语义先查 `IsaacDocs/docs/EntityEffect.md`、官方 `LuaEntityEffect.cpp`／`EntityEffect.zhl`／`GridEntityDesc.zhl`。公开 getter 证明存储含义，不自动构成 Client 同步需要。

Effect vtable `B14D00`：Init `5CD070`、Update `5D35D0`、Interpolate `5D1960`、Render `600E60`、Shadow `6AD450`、PostRender `6AC660`、GetRenderZ `6AC3F0`、GetForwardVector `6B1FD0`。复用 Laser 复核的 28 个展示根，明确加入 Effect Lua render 桥 `847B20` 的停止边界，共 29 根、581 个直接可达函数；不将 Lua 回调、间接虚分派或第三方 Mod 当作已闭合的全程序证明。全映像目标位移候选涉及 925 个函数，与展示集合交叉后按真实对象基址区分；补查 LEA、ADD 取址及宽读取。Render 使用完整 IDA 伪代码和原始指令；184832B 的 Update 超过本次 Hex-Rays 可反编译范围，采用 switch 表和原始指令，不把 `None` 当作无消费者。

### 公共参数

| 范围／参数 | 实际消费者与同步决定 |
| --- | --- |
| `+410` Height | Update `5EA066/5EA088` 等用于粒子纵向更新；Wiki 亦称粒子的 dy。无当前原生展示读取，留 Host；最终公共位置／PositionOffset／Sprite 单独同步。 |
| `+414` FallingSpeed | Update `5E9FC4/5E9FF6/5EA02E` 消费并更新；留 Host，不在 Client 模拟下落。 |
| `+418` FallingAcceleration | Update `5EA018/5EA99C/5EB288` 等进入运动计算；留 Host。 |
| `+450` State | Render `601247/60271F/60400E/604222/6042E2/604456/604471` 直接选择图层、分段或 MrMe 等分支；保留。 |
| `+454` MinRadius | Update `5D9518` 比较半径；Trail `5DA42C—5DA478` 用它与公共速度倍率更新节点 alpha。Render `602C4A/602ED3/602FFF` 的同位移实际基址是 **Ghost payload 的 count**，不能误作 Effect 半径消费者。留 Host，保留最终节点。 |
| `+458` MaxRadius | Update `5D85CF/5D864A/5D8730` 等冲击波计算；留 Host。 |
| `+45C` Timeout | Render `6017BB`、`6036B1`、`603A33/603ACE` 等控制淡入淡出、长度／阶段比例；保留到期帧换算。 |
| `+460` LifeSpan | Render `60370F/603A9E/603B2A` 直接参与比例／分支，不能仅凭“寿命”名称排除；保留。 |
| `+464` IsFollowing | variant196 Render `60106B` 与公共 Parent 一同决定绘制；保留。Light `7FD8BB` 的同位移属于其他类型，不作为 Effect 证据。 |
| `+468/+46C` ParentOffset | variant160 Render `60325F/603251` 与 Parent／Target 最终位置构成连接几何，不能由 PositionOffset 等价替代；保留。 |
| `+470` DamageSource | Update `5E21F7`、variant116 `5FE5E8/5FE614` 等更新／判定；无当前原生展示读取，留 Host。 |
| `+474` Rotation | variant172/subtype1 Render `603998` 与公共 `speed_multiplier +39C` 形成方向旋转；保留两项，不在 Client 推进角度。 |
| `+478` Scale | Render `603180`、30／153 的辅助绘制，以及 Type1000/variant121 Room Light `8002BB` 消费；保留。 |

六项撤销范围共 24B，FX0 从 20→8B，仅 State 与 Timeout 到期帧；FX1 从 33→21B，保留 LifeSpan／IsFollowing／ParentOffset／Rotation／Scale。传播 `+440/+444/+448`、生成请求 `+47C—+4A7`、私有 vector 和其他 Host payload 的既有排除不变。

### 变体复用区与六臂

| 臂／布局 | 具体消费者与决定 |
| --- | --- |
| 基础 101，其余变体 | `+420—+43F` 未逐变体建模，保持 UNKNOWN、不复制。确有 variant119 在 `5FFBFA—5FFC00` 把 `+420` 当函数指针调用；不能用 Init 清零或 SDK BitSet128 占位证明普通 VarData 是跨进程纯值。 |
| TearFlags 102：54／113／126／167 | 官方 API 将 `+420` 解释为完整 BitSet128。Update 主 switch 的相应入口为 `5E79A0`（54 所在组）、`5FD01E`（113）、`5F4B61`（126 所在组）、`5DE145`（167）；取址／位集合消费例为 `5E9056/5FD091/5F4DFA/5DEA40`。它们用于 Host 效果判定、伤害和后继生成；当前原生展示不读取该 16B。全部留 Host，FX2 预留；公共 EntityFlags／最终颜色／Sprite 继续各自同步。 |
| GridDesc 103：136 | 八成员 Type／Variant／State／SpawnCount／SpawnSeed／VarData／Initialized／VariableSeed 共 29B，原生 view 跨完整 32B，三个未知字节保持。`Room::PickupGridEntity 81AE00` 经 `81A920` 取得描述及 ANM2，Spawn 后以 `403160` 交换最终主 Sprite，再写两段 16B 描述。`5B3780` 也创建 136 并写描述。Effect Update case136 `5D5486` 检 Parent；Render、持物调用与其他当前展示路径不消费描述。描述留 Host，Client 按最终主 Sprite、State、Player 持物关系／BodySprite 绘制；不在 Effect 或 Grid 域另发描述，不重演举起／投掷／落地／还原格子。 |
| MrMe 104：108 | 在 `LABEL_256` 的 Render `604222` 检 State=2，`604231` 核 MrMeData，随后检查公共 Target 非空和 Sprite 资源就绪，读取主 Sprite 的 `CarryPos` 并以 payload `+8` ANM2 绘制。独立 Sprite、State、Target 与可空资源选择均保留，不能从 Target 外观重新加载来代替最终 Sprite。 |
| SpriteTrail 105：166 | Render `6012CD` 核 SpriteTrailData，检查有效实体及 count≥2，再按逻辑逆序遍历节点。它消费完整历史几何／颜色／宽度／绘制参数／alpha，保留实际节点，不在 Client 根据当前父位置补造历史。 |
| PickupGhost 106：191 | Render `602C22` 核 PickupGhostData、检查 `+454` count，再由 Parent 的当前动画／图层、或 Effect 自身最终位置决定原点及缩放；`602FB2` 绘制后按 `0x114` 前进，只循环到 count。四槽是容量，实际有效前缀需要同步；不能从相同资源或 loaded 数推断 count。 |

136 的 Player 持物展示有既有正面证据，见[原生研究](../tx-04-19/entity-i20-i25-native.md)第“Effect136”节；删去 descriptor 不得删 HeldEntity、GetItemState、GetSuplexState、BodySprite／空层选择器或暂时置可见的绘制借用。新的落地后 Grid／Effect／Entity 由 Host 作为真实成员正常发布。

## 3. 附属 payload 的取舍

### MrMeData（布局25，0x128）

`5D1EB0` 为更新 helper，在 `5FCAAA` 的 case108 路径调用。`+4` 是寻找／处理目标的分支值；`+11C` 在 `5D2FB5` 保存目标 `Entity+188` 碰撞类别，`5D31C2` 恢复；`+120` 在 `5D21F3/5D2301` 写 -1／格子序号、`5D2978` 读取；`+124` 在 `5D289E/5D28A8` 递增、`5D2875` 清零。四项均为 Host 内部状态，没有 MrMe 展示读取。本轮把原 4B＋12B UNKNOWN 分成上述四个权威 i32；它们原本没有 wire，因此不虚报网络节省。

`+8` ANM2 的可空资源、最终主／叠加姿态、独立选择器和图层继续三投影；公共 Sprite 的 `CarryPos` 是资源／姿态消费者，不能把此独立 Sprite 视作重复。loaded、动画事件位、资源数组指针等沿用版本32的本地／Host／派生归属。外部 payload 未知 30→14B，剩余全部属于 ANM2 未解释字节。

### SpriteTrailData（布局27，0x18）及节点26（0x40）

`5DA1D0` 是 Update case166。它用 `600D80` 准备节点，`5DA2E7` 从最终 Entity 位置拷入 `+0`，`5DA30E` 拷入 ColorMod，`5DA32B/5DA330` 形成 `+34`，`+38` 初值为零或经 `606230` 取末节点后，以 `5DA396/5DA3A9` 的相邻距离累计更新，`+3C` 由 `5DA340` 初始化并在 `5DA42C—5DA478` 衰减。`5DA3C6` 调 `606190` 追加。

`606190` 在 `6061CB/6061CD` 明确 `new(0x40)`；`6061E4—60621D` 逐段复制完整 64B，直接补齐旧 producer／大小缺口。deque 的 map 每槽是独立节点指针，**不是一段按 stride 连续的内存**；`storage=array` 只表示具名 reader 归并后的逻辑顺序输出。proxy／map／容量／逻辑 offset／分配地址由本地 owner 管理。

Render `6013F9` 以 `+34` 筛相邻点距离；从 `+8` 完整复制 44B ColorMod，再乘 `+3C` 到 alpha，并以 `+34×+3C` 形成宽度；`60167D/601682` 将 `+38` 原值交 `419D30`。因此节点全部 64B 都保留。虽然 `+38` 的生产含距离累加，它依赖已退休的历史节点和初始累计值，不能仅用当前截取点列等价重建；继续 u32 位模式，不恢复旧 Rotation 命名或额外施加浮点有限值校验。位置／ColorMod／scale／alpha 的既有有限值校验保持。

### PickupGhostData（布局28）与专用 Sprite 视图134

生产 helper `605830` 先正常 Reset 四个 ANM2、清 count，再按最多四个结果从配置／资源构造独立 Sprite，并执行图层改图、shadow 隐藏、动画选择及颜色处理，最后增加有效前缀。实际显示内容含皮肤／诅咒等 Host 选择，不能从 loot 类型或宿主当前资源重算。

Render `602F4E/602F5B/602F63/602F68` 在每次 `4059E0` 前，完全覆写每个有效 Sprite 的 ANM2 `+94/+98` Offset 和 `+9C/+A0` Scale。来源为 Parent 最终 Sprite／图层／动画帧或 Effect 自身位置、固定排布表和 count。因此视图134把 **这16B** 标为派生，其余 ANM2 字段保持通用布局事实和证据等级；通用 ANM2 布局7不变，不推广到 MrMe 或其他 Sprite。draw scope 应保存、计算、绘制后精确恢复，不能为派生而执行 Update 或动画推进。

payload 原生 `+4—+453` 声明为四槽内联 ANM2 数组，线上仅 `[0,count)`。三个投影分别带 u16 count，均须≤4且同目标版本一致；Host 在收窄前拒绝原生 i32 负数／>4，Client 用一致 count 写回 `+454`，不另发 i32。present=0 时三个投影均为 count=0 的最短全零编码。缩短、归零、缺失及再增长须清理旧可见槽并正常维护四个 ANM2 生命周期，不能 memset 活容器或仅改 count 掩盖资源泄漏。

为避免有效前缀裁剪吞掉原生 UNKNOWN，生成器支持显式 `wire.inline_count=4`：原生目录始终展开全部四槽，只有线状态使用当前前缀。每槽仍14B未知，payload仍56B未知；新增视图不声称完整。

## 4. 布局变化与后续安装

| 类型 | 版本37→38的最小编码字节数 |
| --- | --- |
| Effect基础／TearFlags／GridDesc | 444／460／473→均420B，分别减少24／40／53B |
| MrMe臂 | 592→568B；独立Sprite保留 |
| SpriteTrail臂 | 447→423B；每节点仍64B，另有u16 count |
| PickupGhost payload | 原592B固定四槽下界→6B空前缀；N个有效Sprite为 `6+131N` 的格式下界（另加真实变长内容），N=4时530B |
| PickupGhost臂 | 原1037B→427B格式下界；四个有效Sprite的格式下界951B，较原减少86B |

这些是生成布局算术，不是实测带宽或合法资源实例大小保证；资源、动画选择器、图层内容仍须完整，不能为了达到下界删内容。根对象 UNKNOWN：基础／三个附属臂382B、TearFlags366B、GridDesc353B，均不变。MrMe 的16B未知有新的正面依据才改归权威；Trail 节点由真实分配与完整复制核定0x40，不把未知尾部猜成填充。

第10步不采集已撤销原值，不因它们的模拟时钟或未观察分支制造同步Gap。仍校验真实payload类型、holder前后稳定、同窗口、数量与实际字节预算；Ghost三个投影同一前缀。第12步不等待FX2空块，拒绝真实线上非法count／present／资源／浮点；第13步由同一具体 owner 在全部投影和资源齐备后提交，不分块替换共享owner。Effect owner继续使用 `606310`，不混用 Familiar helper；缺失及失败候选由正常生命周期清理。客户端不运行原生 Update／Interpolate、传播／Spawn、TearFlags规则、举起／投掷／落地、Trail追加／衰减或Ghost loot选择。

Timeout仍有真实展示消费者，继续每窗 `A=F+T`、同已应用world帧 `T=A−F`（模2^32）；负值、停走、跳变均按原生事实重新计算锚点，不承诺静止Effect永不重发。其他Host参数无原值或漂移比较要求。旧Lua／JSON运行路径的退役仍归第14步，本轮未声称已经切换。

第15步沿用E09／E25／E32覆盖六臂的最终Sprite／Light／连接几何／持物、三种payload存在／缺失／重建、Ghost 0—4前缀及缩短／再增长、Trail增长／裁短／alpha／累计绘制参数；E13／E17覆盖计数不一致、超限／负数收窄、非有限值、资源和预算失败；E29覆盖正常owner清理与失败不部分替换；E28核隔离和draw精确恢复；E34继续核Timeout负值／停走／跳变；E27保留其余UNKNOWN。不比较已排除字段，不提前执行这些测试。
