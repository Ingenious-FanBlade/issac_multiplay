# tx-04-20 I11—I15：玩家最终量、库存及槽安装

日期：2026-11-04，Asia/Singapore。沿用同一 tx-04-20。本轮实现 I11—I15 的正式来源、不可变值解码和原生最终量安装／回读，接入 I08 结构 owner；没有构建、运行测试或启动游戏。I16—I40 继续按主清单推进，整个目标的统一 E2E 仍在 I39。

## 实现与边界

| 项 | 本轮实现 |
| --- | --- |
| I11 | Player20 复用 Entity 公共安装表，包含位置／速度／偏移、碰撞、颜色、flags、FrameCount 和存在／死亡结果；安装最终 stats、方向、冷却、闪烁、TearFlags128 与 Tear／LaserColor。Source Index、RNG 和本地分配身份不混用。不运行 Update、EvaluateItems、输入 consumer 或碰撞。 |
| I12 | 安装红／最大／魂／黑／永恒／金／骨／腐／破碎心和骨心排列掩码，保留实际整数／位模式；dead／exists／coop ghost 分开。无普通 Isaac 的 max>=red 限制，不用伤害／治疗／死亡／复活 API。心量上限、健康类型、额外生命和可拾取结论继续保留正式公开查询值，供角色／HUD 消费，不通过客户端规则重新裁决。 |
| I13 | 安装硬币／钥匙／炸弹、金钥匙／金炸弹、GigaBomb、Urn、魂／血充能及角色计数。魂充能只适用于18、血充能36、poop mana25、Maggy计数22、Sumptorium26；其余身体明确 NotApplicable，不读取该专用值。本地无效专用缓存按表清除，不能将清零声称为来源读取成功。 |
| I14 | 安装完整按 ID 索引的 collectible count vector、voided 顺序、wisp map、两个 trinket 槽、熔炼普通／金色 short 计数。官方 Plus 的阻止集合、全部固有分组（包括空字符串 legacy 组）、持久计数、有序临时项／时限及真实计数缓存分别安装。库存不是 History；获得顺序和 Tainted Isaac 的专用表现仍由 I17／I27 的消费者处理。 |
| I15 | 安装四个 ActiveItemDesc、四个 PocketItem、queued item 的本地配置借用、charge／flags／VarData。普通／电池／小数／子充能和 timed cooldown 分别保留。口袋主动的 ID 是 ActiveSlot+1，不能当物品 ID；空口袋保留真实旧 type 字节。Esau Jr 的 VarData 映射同 T 的具名备份槽，不转发地址，不写 PlayerManager 的全局备份槽。 |

这是一段真实的最终量安装器，不提供空的资源／角色 adapter 来使完整 Player 提前可见。`ClientNativePlayersStructure` 在每次状态调用前复核当前 use，安装公共值与 I11—I15 slice，再调用后续完整 adapter；`Finish` 从真实对象和容器回读后才允许继续 Scene 唯一提交。后续完整资源、角色、武器和关系 adapter 尚缺席时仍在第一次世界写入前拒绝，`native_apply`／完整原生来源能力保持 false。运行覆盖没有由静态实现外推。

## 正式来源及所有权

`application_player_state` v1 属于同一次普通 Entity／Player 采集，声明 `same-ordinary-window`，通过 `player-source-v2.final_state=player-final-state-v1` 明确要求。缺旧扩展不会降级为空库存或空槽。来源调用 `ObservationSubject` 核对 KnownCurrent 和 readiness>=2；不因在 roster 中、地址非空或存在性扩大资格，也不读取独立 Render 的旧窗口。

`client_player_fields.inc` 同时驱动窄字段来源、位宽解码、安装和真实后态；包含完整浮点、128位 flags 及 ColorMod 的11个分量。正式补充不携带原生对象地址、配置指针、vector头、虚表或 RNG。原有公开 fields／完整参数查询保留在同一不可变实体组件，公开 getter 的共享／计算语义与每身体 backing storage 分开，不用二者同名推导共享 owner。

`client_player_model.cpp` 一次解码为 `ClientPlayerState`。数组完整长度、类型／位宽、有限值、重复分组／ID、计数溢出、配置和目标版本均有具名失败；Missing、Unavailable、NotCaptured、合法空和 NotApplicable 分开。容器和槽失败不提升 A，也不继续发布该 scope。

官方 Player／Plus 保留内存所有权。新容器先由与官方相同的 Release v143／MD STL 建成候选，再交换内容；不复制容器头或虚表，不调用 Add／Remove／SetInnateGroup／Block／Use／Queue／Discharge。官方源码表明这些公开变更方法即使不加 costume 仍可能触发 cache、callback 或 EvaluateItems，因此没有采用。

Plus 读取依照官方 `EntityPlus.cpp` 的两个 ushort holder 字段重组方式，先确认现有 holder、动态类型 `EntityPlayerPlus`，再借用其 `itemSpoofs`。私有值布局来自官方 `ItemSpoofSystem.h`，与官方类型进行编译期大小约束；只换 blocked／groups 的内容，保留真实多态 owner。legacy 组也完整传递，不调用会省略该组的 Save／Load 恢复链。临时项的缓存按完整序列构造并与 Source 原缓存核对，不推进 timer。退休只撤 queued config 借用，容器由原生 Player 和官方 Plus 恰一次析构。

## 必要逆向与主审

使用一个 GPT-6 Luna max 子智能体做只读静态研究，主智能体独立复读关键 IDA 和官方源码。顺序为本地原版／REP Wiki、官方 ZHL／Lua wrapper／Plus 源码、最后 J273 具体未暴露点位。没有修改 IDB、补丁、Hook 或进程状态。

| J273 位置／源码 | 采用结论及主审修正 |
| --- | --- |
| Player Lua 注册 VA0x84D340 内，QueuedItem 注册处0x84FDFA—0x84FE04 | QueuedItem 是 Player+0x17B4 的内嵌0x10字节值。ItemConfig_Item* 用本地 type／id 解析，flags／charge／VarData 是独立值。ItemType.TRINKET=2，不能将 Familiar=4 或其他值当饰品配置表。 |
| GetPocketItem VA0x74D7F0／RVA0x34D7F0 | clamp槽0..3，返回 Player+0x1790+8*slot；四个真实槽。Wiki说明空槽 type 可残留，本实现保留而不将空强制编码为 type0。 |
| Active 注册 helper VA0x863B30→getter0x8A4510／setter0x8A4570 | `PartialCharge +0x18` 是 f32；其他整数成员走不同注册 helper0x8639C0。Luna最初依据 ZHL 的 int 声明推断整数；主审复读浮点 getter 后纠正，安装使用 float 位表示，不做整数数值转换。 |
| AddCollectible VA0x758F00，store VA0x759864／RVA0x359864；官方 LuaEntityPlayer.cpp:71—79 | ActiveItemDesc+4虽然未向Lua公开且ZHL名为`_unk`，主审追到其写入来自AddCollectible最后一个item pool参数，Luna再独立核对store及构造／保存路径。正式来源／模型命名`item_pool`，按signed i32保留来源；不是动态配置地址，不复制指针，不调用AddCollectible。 |
| GetSoulCharge VA0x6D85E0、GetBloodCharge VA0x6D85F0 | 分别读取 Player+0x1D94／+0x1D98 的i32。最初研究消息将它们称为 effective；主审复读注册发现 effective 实际为0x7C8E90／0x7C9100，区分原值与计算结论。 |
| Player Lua 属性注册 | TearFlags+0x1480为128位、TearColor+0x1490和LaserColor+0x14BC为ColorMod；逐分量有限值／真实回读，不用统一近似容差。 |
| FireDelay getter VA0x77BCD0／setter0x77BD10 | 从 Player 的 Weapon[0] 或[1] 的+0xC读取／写入，Player没有另一块独立 FireDelay；实际武器存储归 I20，不在 I11 创建影子冷却。公开当前值继续保留。 |
| GetCollectibleCount VA0x758E20；LuaEntityPlayer.cpp GetCollectiblesList／GetWispCollectiblesList／GetSmeltedTrinkets | GetCollectibleCount直接汇总counts，无另一个总数缓存；GetCollectiblesList是计数表，wisp是独立map，熔炼每ID分别有普通／金色short。不能从History反建库存。 |
| Esau Jr VA0x7A3A40／RVA0x3A3A40 | 只对item703解释 VarData：0表示尚无槽，1..4是1-based备份槽，指向Game+0x1BB28+4*(slot-1)。原调用会创建／替换身体，不采用；目标模型携带具名槽和同T引用，角色关系消费归I26。 |

完整 draw 私有量、TemporaryEffects、History、costume、全角色和 Weapon adapter 继续按 I16—I29 接线；这些是同一 tx 的后续实施项，不改变本轮最终量安装器的所有权或唯一提交责任。

## 静态收束

本轮仅在全部编辑后统一静态审阅源合同、字段表、生命周期接线和文档；没有提前运行分项测试，也没有补写单元测试。生成应用目录只生成产品数据。没有新增 tx artifact；后续 I39 按完整清单连续执行 E2E。
