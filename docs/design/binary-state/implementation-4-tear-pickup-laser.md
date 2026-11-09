# 第 4 步：Tear、Pickup、Laser 实现说明

> 版本37后续复核：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只保留optimized／unoptimized最终点列和真实展示标量；十九项Laser输入、六项Homing标量、indices中间点缓存与Bezier／系数留Host，撤销两个Laser计时锚点。J273 Homing+34实际为8B双f32点对vector，本文旧i32／4B步长记录错误；原记录保留，不按旧类型继续传输。L2／L4／L7／L8预留，当前以版本37唯一源及生成表为准；未构建或测试，旧运行执行器仍待后续退役。

> 版本35后续复核：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait和未来循环列表，以及Slot八项参数留Host。Price／DropDelay、最终主／辅助Sprite与真实引用保留；Slot奖品+460／+470精确对应当前动画指针／is_playing。下文旧传输／原值比较约定保留原时点，现行以版本35唯一源及生成表为准；未构建或测试。


> 版本34后续复核：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)将Knife10／Bomb11／Tear21个攻击、运动与计时参数范围留Host；撤销三个计时锚点，保留真实绘制输入、最终Sprite和Bomb pending清除。下文旧的参数传输／计时见证要求保留原时点，现行传输以版本34唯一源及生成表为准；未构建或测试，运行接线仍待后续步骤。


> 版本29修订：原生关系事实保留；23条内部关系不再同步，Familiar+DAC仅variant240布局130保留。当前采用范围及生命周期边界见[24项关系复核](internal-entity-relations-review.md)，不执行本文旧的全关系原值同步要求。

> 版本28修订：本文保留当时的布局和编译事实；Knife K4、四类命中历史及另外七个玩法范围的当前传输职责，以[同步字段复核](synchronization-field-review.md)和当前生成表为准。旧原值同步／比较约定不再执行。

日期：2026-11-08（Asia/Singapore）。本轮实施[第 4 步](4-entity-classes.md)中的三个子类，不代表第 4 步全部完成。用户指定 GPT-6 Luna（max）子智能体完成必要静态逆向，主智能体复读官方源码、J273 RTTI／虚表、构造与关键指令后采用，详见[证据与主审](tear-pickup-laser-evidence.md)。0—14 步只做静态分析、编辑、布局生成和纯编译；第 15 步是唯一整体验收。

## 交付范围

唯一源仍为 `mod/binding/state/entity-classes.json`，整数布局版本 **20**。Tear／Pickup／Laser 的布局编号为 **125／126／127**，继承 Entity 的 E0—E9，子类块从 10 开始，全部属于 world 序列。三个派生 Plus 编号 20—22；HomingLaserBezier／HomingLaser 编号 23／24。

版本 17 为本轮首次布局生成；主审确认正倒计时与有符号递减不可混用后，版本 18 更新转换声明并加入实际纯值编码函数。版本 19 更正 Laser 的 SubType 选择说明，并撤销另一 helper 没有 Timeout 访问的错误结论：`0x954310` 实际有读取、重置及经字段地址别名的处理，完整时钟条件仍待核定。错误及主审纠正保留在证据文档中。版本 20 澄清 HomingLaser 两个私有标量的语义：旧适配器的 cursor／spacing 名称不证明真实游标或物理间距，布局只保留已核类型与原值，不强加未证明的范围。布局变化只比较整数版本，不计算哈希。

- **Tear**：运动、参数、计时、七个独立引用槽、Host 原始命中索引与三份辅助 ANM2 分块。三份 ANM2 在 `+0x4AC／+0x5C0／+0x6D4`，分别投影外观、动画与图层。StickTarget 不调用裸写 Lua setter，连同六个内部槽统一走引用 owner，退休前先撤边，避免原生清理级联删除仍存活的独立对象。SDK 的 ScaleAnimNum／TearRange 证据等级按实际核对情况保留。
- **Pickup**：价格和替代底座 ANM2、计时、标量、四个引用槽与循环道具分别建模。循环道具原生数组 `+0x5A0` 的有效数来自 u32 `+0x5C0`，线上只发 u16 count 和有效元素，上限八项；Client 同一 owner 清除未用槽并恢复原生计数，不调用 Add／RemoveCycle。Flip 保存对象 `+0x5C8` 保留 Host 权威归属，不浅拷贝指针或在 Client 重抽道具。
- **Laser**：有符号 `sample_mode_raw +0x45D` 保留 −1／0／1 等原始值，公开 bool 从非零派生。公开端点 `+0x4D8` 与 HomingLaser 内部端点是两个独立字段。十三个引用槽逐一保留。HomingLaser 的 self 指针只在本进程使用，四份 vector 独立拥有存储，分别携带 optimized／unoptimized／indices／Bezier。Bezier 只命名六个 f32 槽，不杜撰控制点语义，不调用 CalculateSamples 恢复轨迹。

三种 Plus 仅收窄 E9 的既存伴随来源类型，公共飞行／水面覆盖只发送一次。派生的初始化音效、重抽覆盖和“下次更新重新采样”均为 Host 职责。ABI 断言在已有 SDK 编译对象中核对大小和字段偏移，不链接额外 SDK 配置初始化对象。

## 容器与应用边界

激光四份几何容器各自上限 16384，沿用旧 `GeometryLimit` 的逐容器限制；命中索引也有明确上限。这些上限只限制输入，不表示带宽或内存预算已经验收。真实采集与解码须累计检查嵌套编码长度、`Field.max_size` 和全局池预算，不能截断几何或保留半组旧数据。

HomingLaser 的五个子块分别投影到 Laser 的标量和四个几何块。一个目标版本中的依赖齐备后，Client 用当前 owner 创建候选并提交；几何变化无需重发独立标量和其他容器。有限浮点按元素布局递归校验，原生 vector 的 begin／end／capacity 永不发送。

运行中的 Host／Client 仍沿用旧同步链。本轮提供生成的复制／编解码／数字写入表和具名 owner 声明，第 10／12／13 步负责实际采集、解码和安装接线，第 14 步退役旧 Lua／JSON 路径。没有修改 Hook 或部署 DLL。

## 计时与未知区

计数器必须以真实更新分支为依据。Tear WaitFrames／StickTimer、Pickup Timeout／Wait／DropDelay、Laser Timeout／RotationDelay 不能仅凭字段名统一按全局帧递减。早退、正值夹零、父实体条件和重置会改变锚点含义；活动条件未闭合时保留“推断”并报 Gap，不能默认停走。计时编码及逐字段结论以[证据](tear-pickup-laser-evidence.md)和布局源转换声明为准。

`state/entity_counters.cpp` 复用现有 `anchors.h` 算术：

- **正倒计时**：Tear WaitFrames 与 Pickup 的三项计时使用 `EncodeEntityCountdown`／`DecodeEntityCountdown`。u64 低 32 位 mode 为 0（原值）或 1（到期帧），高 32 位为 value；先经 `ValidEntityCountdown`，到期后停在零，负哨兵保持原值。
- **有符号递减**：Tear StickTimer、Laser RotationDelay 的已见减写没有正值夹零保证，沿用 `EncodeEntityCounter`／`DecodeEntityCounter`，这两字段只允许 Frozen／Decreasing（mode 0／2），拒绝 Increasing。零／负值是否活动依赖对应原生分支，不能按符号推断。
- **Laser Timeout**：使用独立的 `EncodeLaserTimeout`／`DecodeLaserTimeout`，mode 0 为真实停走原值、1 为正倒计时、2 为无夹限有符号递减。`0x94FD2A` 可递减到负值，`0x950AC0` 后又有仅正值递减的路径，两者不能共用一种夹零规则。先 `ValidLaserTimeout` 再还原，不在 Client 执行 Remove／Shrink／伤害。其余 helper、早退和实际时钟关系尚未闭合时必须报 Gap。

以上 mode 语义由具体字段的生成处理器固定，不能将 Laser Timeout 的 mode 1 当成 `EntityCounterMode::Increasing`。所有还原均使用最近已应用 world 的 game_frame，展示时钟更新后重新写入计时原值。

每类从零到 J273 池步长全部显式归属，未知保持未知；构造清零和池尾部都不是填充证明。完整表与未知清单见 [Tear](layouts/EntityTear.md)、[Pickup](layouts/EntityPickup.md)、[Laser](layouts/EntityLaser.md)。公开目录回映射齐全不等于字节完整，更不表示原生同步已可运行。

| 根对象 | 原生范围 | 未知字节 | 其中公共 Entity／内嵌 ANM2／本类独有 |
| --- | --- | ---: | --- |
| Tear | 0x848 | 478B | 335B／42B／101B |
| Pickup | 0x6F0 | 448B | 335B／28B／85B |
| Laser | 0x568 | 421B | 335B／0B／86B |

三个伴随 Plus 的未知分别为 5B／7B／8B，另在各自表中列出，不算入根对象。Pickup `+0x558—+0x583` 的 44B 构造形态类似 ColorMod，但尚未核定业务消费者，仍是未知；不能因首四个 1.0、后七个零就直接作为颜色发送。Tear 的 SDK 字段和七项计时的活动条件也不因未知字节计数而升级证据等级。

## 编译与验收安排

提交前静态审阅纠正了 Pickup `+0x5C8` 的原生类型标签：初稿写作 `shared_ptr<EntitySaveState>`，官方 `EntityPickup.zhl`／`SmartPointer.zhl` 实际声明为 `KAGE_SmartPointer_EntitySaveState`，由 `saveState +0x0` 和 `counter +0x4` 组成，大小为 8B。现按原生类型命名，仍归 Host 权威且不发送；此修订只纠正归属表的类型与证据，不改变线布局，整数版本保持 20。

普通 Release／Win32／v143 构建保持 `ISAACCS_BUILD_E2E_TOOLS=OFF`。布局只通过普通 CMake 构建生成；基础库、派生 Plus ABI 编译对象与完整 `zhlIsaacCS` 编译／链接通过。SDK 仍有既有的 `CMAKE_REPENTOGON_VERSION undefined` 提示。

没有执行游戏、注入、单元测试、probe、smoke、校验脚本或 E2E，也没有另建 artifact 包。第 15 步再统一覆盖计时分支、几何、循环道具、引用退休、未知区变化、Lua 对照与性能。
