# 第 4 步：Slot、Projectile、Effect 实现说明

> 版本38后续修订：[Effect各臂与附属payload复核](effect-synchronization-review.md)取代下文Effect旧传输决定。六项参数、TearFlags及136描述归Host；MrMe独立Sprite、Trail完整64B节点保留；Ghost只发0—4有效Sprite，Offset／Scale派生且count由投影重建。MrMe16B原未知及Trail真实分配有新正面证据，其他UNKNOWN保持。源、生成表和后续合同已更新；本轮未构建或测试，下文版本23普通编译事实保留原时点。

> 版本36后续复核：[Projectile运动与展示参数复核](projectile-synchronization-review.md)将Projectile追踪／曲线／加速／下落／摆动及Scale／ProjectileFlags共八项留Host，保留Render直接消费的Height；公共EntityFlags、最终位置／偏移／尺寸和Sprite保持。私有+440及+460／+464的展示来源仍UNKNOWN，未构建或测试。下文旧传输与原值比较约定保留原时点，现行以版本36源及生成表为准，运行时仍待后续步骤接线。


> 版本35后续复核：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait和未来循环列表，以及Slot八项参数留Host。Price／DropDelay、最终主／辅助Sprite与真实引用保留；Slot奖品+460／+470精确对应当前动画指针／is_playing。下文旧传输／原值比较约定保留原时点，现行以版本35唯一源及生成表为准；未构建或测试。


> 版本28修订：本文保留当时的布局和编译事实；Knife K4、四类命中历史及另外七个玩法范围的当前传输职责，以[同步字段复核](synchronization-field-review.md)和当前生成表为准。旧原值同步／比较约定不再执行。

日期：2026-11-09（Asia/Singapore）。本轮实施第 4 步中的三个类别，继承既有 Entity E0—E9。必要静态逆向由用户指定的一个 GPT-6 Luna（max）子智能体执行，主智能体复读官方 SDK／Lua 源码、J273 指令和 RTTI 后采用；[证据及主审](slot-projectile-effect-evidence.md)保留纠错与未知。本轮不代表第 4 步全部完成，也不代表同步已经接通。

## 布局与范围

布局整数版本 **23**，不计算哈希。Slot／Projectile 追加到 `mod/binding/state/entity-classes.json`；Effect 与其 payload 由 `entity-effect.json` 唯一定义。21 为本轮初次生成；主审撤销轨迹 `+0x38` 的 Rotation／f32 推断及有限值约束后，以 22 记录该校验合同变化；23 将节点收窄为已访问前缀，并要求实际记录布局读取资格。

| 类型 | 布局编号 | J273 原生范围 | 子类块 |
| --- | --- | --- | --- |
| EntitySlot | 128 | 0x550 | 10—14：状态／奖励、状态机计数、奖励 Sprite 外观／动画／图层 |
| EntityProjectile | 129 | 0x478 | 10—11：运动／表现、效果／变更 |
| EntityEffect 基础臂 | 101 | 0x4C0 | 10—12；FX2 显式预留，不复制未知变体数据 |
| Effect.TearFlags／GridDesc | 102／103 | 0x4C0 | FX2 分别为 16B 效果位／29B 完整格子描述 |
| Effect.MrMe／SpriteTrail／PickupGhost | 104／105／106 | 0x4C0 | 13 起增加具体 owner 的分块投影 |
| EffectMrMeData／EffectTrailNode／EffectSpriteTrailData／EffectPickupGhostData | 25／26／27／28 | 0x128／0x40前缀／0x18／0x458 | 独立 payload／元素原生表和二进制表 |

Slot 保留八项公开数值和独立奖励 ANM2。Timeout 是 i16，ShellGameAnimationIndex 是 i16，Touch 是 u16；Wiki 中超出 16 位的说明不用于增加字段。奖励 Sprite 与奖品编号分别保存，不能用 `SetPrizeCollectible`、Update、开奖或 jam RNG 恢复画面。`+0x418`、`+0x41E`、`+0x428` 与尾部的未核字节保持未知。

Projectile 的公开 DepthOffset 与 Entity `+0x358` 是同一个 f32，只在继承块发送一次。ProjectileFlags／ChangeFlags 各为 u64，不能套用 Tear 的 BitSet128。Damage `+0x41C` 与公共 CollisionDamage 分开保存；本类没有专用 Plus，公共 E9 继续使用既存 EntityPlus。

Effect Timeout 继续采用用户已定的 i32 到期帧：Host 每个采集窗从实际原值重算 `A=F+T`，Client 在该 world 帧以 `T=A−F` 还原，全部按模 2^32。若实际分支停走或跳变，A 会变化并重新发送；该转换的正确性不要求所有变体每帧恰好递减一次。只在正常递减分支才会省掉重复发送，不在 Client 两个已应用帧之间自行预测。

## Effect 变体与拥有型表现

种子布局曾把普通变体 `+0x420` 的 16B 按 SDK 占位 BitSet128 复制。本轮撤销该声明：Init 清零不证明后续变体逻辑没有写入指针或其他本地数据。基础臂及三个附属臂的 `0x420—0x43F` 维持未知；只有 54／113／126／167 的 TearFlags 和 136 的完整 GridEntityDesc 具有明确投影。136 描述不在 Grid 域重复发送，三个种子臂的布局号不变。

共享 owner `+0x4B4／+0x4B8` 是 payload／控制块，两字均不发送。新增 `wire.storage=shared-owner`：生成表的原生偏移指向 holder，具名处理器检查具体动态类型后读取另一对象，不能按根对象内嵌字段加偏移。payload 有独立字节归属，根对象的未知统计不把外部 payload 内存算入自身范围。

- FX3：u8 `present`，接具体 payload 的外观子块；SpriteTrail 的外观子块为空，FX3 只有一个存在字节。
- FX4：MrMe 一份／PickupGhost 四份动画，或 SpriteTrail 的有序节点容器。
- FX5：MrMe／PickupGhost 的各份 ANM2 图层；SpriteTrail 没有 FX5。

第 2 步已将 ANM2 默认动画名纳入状态，因此实际外观子块包含变长字符串，不能继续沿用草稿中的“定长外观”。四份 Ghost Sprite 各自的图层计数也分别保留，固定索引对应所属 Sprite，无需扁平化为另一套编号。

`state/owned_payload.*` 提供有界编码／解码。首投影仅一个 0／1 存在标记；尾投影必须依赖同目标版本首块。缺失 owner 时，各投影保持生成计划的最短全零编码，动态字符串／图层／节点计数为零。解码缺失值时返回空视图，禁止把占位资源号递归解释为有效资源。未知动态类型、来源变动和读取失败不能伪装成不存在。

Client 构建具体候选并安装资源／节点后，经现有 `0x606310` owner 替换。全部依赖齐备后统一提交，不能逐投影替换 owner 丢掉其他块，也不能 memcpy 原生虚表、deque 头或共享控制块。PickupGhost 有效数限制为 0—4；节点上限沿用 16384，并受实际编码字节和全局池预算约束，不表示带宽已经达标。

轨迹 `+0x34` 和 `+0x3C` 在 Render 中有实际浮点算术；`+0x38` 仅经 XMM3 透传给绘制记录，不能因旧注释叫 Rotation 就当成角度。当前按 u32 `trail_render_word` 保留位模式，状态为“推断”，不做 f32 有限值检查。0x40仅为已访问前缀，实际producer分配大小和步长仍未闭合，节点标为 base_prefix。真实读取／重建需要 entity-current-window-and-confirmed-trail-record-layout；第10／13步须先补证，未核定则报Gap，不能直接按64B安装。第15步统一覆盖轨迹实际行为。

## 未知字节

| 原生根对象 | 未知字节 | 分解 |
| --- | ---: | --- |
| Slot | 367B | 公共 Entity 335B＋奖励 ANM2 14B＋本类 18B |
| Projectile | 379B | 公共 Entity 335B＋本类 44B |
| Effect 基础／三个附属臂 | 438B | 公共 Entity 335B＋本类 103B |
| Effect TearFlags 臂 | 422B | 公共 Entity 335B＋本类 87B |
| Effect GridDesc 臂 | 409B | 公共 Entity 335B＋本类 74B |

外部 MrMeData 未知 30B，PickupGhostData 未知 56B，不计入根对象的 0x4C0。Trail 节点／容器的证据等级与完整标记见生成表，不能只凭未知计数判断完整。

## 后续接线与验收

主智能体实际复读了官方 `EntitySlot.zhl`／`LuaEntitySlot.cpp`、`EntityPlus.h`，J273 的 Slot Render `0x6620E0`、Projectile Init `0x64F4B0`、DepthOffset 访问器 `0x431380／0x4313A0`、64 位 flags 访问器 `0x413170／0x442D70`、Effect ctor `0x5CCEA0`、MrMe ctor `0x5CCE00`、Trail ctor `0x5D1720`、Ghost dtor `0x5D17E0`、共享 owner 替换 `0x606310` 及轨迹 Render 原始指令 `0x6013F9／0x601478／0x601660—0x601682`。同时直接读取五种 RTTI 的 COL／TypeDescriptor 链。它们是静态审阅，不是运行结果。

主审保留的纠错包括：`0x64F4B0` 是 Projectile 派生 Init，不是 ctor；旧文档对 Slot Render `+0x460／+0x470` 的“已加载／可见”命名不成立，它们分别相对奖励 ANM2 为 `+0x34／+0x44`，本轮只采用实际访问与完整 ANM2 事实；Trail `+0x38` 不宣称角度。旧报告原文保留，后续实现采用此处和证据文档中的更正。

本轮交付布局源、生成的复制／编解码／数字写入表、可选 owner 的纯值编码设施及 SDK ABI 编译断言。第 10 步接入实际采集窗口和 owner；第 12／13 步接入解码、资源／关系依赖和原生安装；第 14 步退役旧 Lua／JSON 路径。Effect 的 IsFollowing／ParentOffset 必须与 E8 Parent 同版本就绪后写入。

原生范围按 J273 池步长，SDK 的 Slot 0x548／Projectile 0x470／Effect 0x490 只作声明对照。构造清零、池对齐和完整公开目录回映射都不证明字节完整。未知区、计时活动分支、附属 payload 与漂移均归第 15 步统一覆盖。

普通 Release／Win32／v143 配置保持 `ISAACCS_BUILD_E2E_TOOLS=OFF`，布局在普通 CMake 构建中生成，基础库、SDK ABI 编译对象与完整 `zhlIsaacCS` 编译／链接通过。首次 `/m:4` 构建停在 MSBuild 的 Checking File Globs 且没有生成新布局，停止后使用 `/m:1 /nr:false` 完成；不将这次中断记录为测试失败。SDK 仍有既有的 `CMAKE_REPENTOGON_VERSION undefined` 提示。

没有运行游戏、注入、单元测试、probe、smoke、校验脚本或 E2E，没有部署 DLL 或建立 artifact 包。生成目录与产品构建输出按工程正常保留，临时编辑脚本已清理。
