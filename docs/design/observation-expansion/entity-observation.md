# tx-04-10：Entity 及其派生／专用接口的完整观测

日期：2026-10-30。状态：已实现，统一 E2E 最终 5/5 PASS；[实现、失败修复与实际覆盖](tx-04-10-implementation.md)。下文保留本 tx 设计范围与要求。

架构按[按域拆分详细设计](domain-observation-architecture.md)实施。本 tx 同时承接公共采集边界、现有 Entity／旧范围 Grid 的迁移和完整 Entity 扩展；这些组成一个 tx-04-10 全目标，不提前分别测试。现已按该范围实现并统一验收；目录、实现和实际路径覆盖分别报告。

## 1. 完整目标与来源

由 min-read-v1 扩展为 Entity 公共、全部公开派生／专用接口、关联对象与资源的完整观测。保留原生命周期、成员、身份和读取有效性；不把公共头部枚举视为某个派生类完整支持。不实现实体创建／写回式 Replica。

先读[原版类图](F:/projects_isaac/IsaacDocs/docs/snippets/EntityClassDiagram.md)、[原版 Entity](F:/projects_isaac/IsaacDocs/docs/Entity.md)及各子类；再合入[REPENTOGON 类图](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/snippets/EntityClassDiagram_NewFunkyMode.md)、[REPENTOGON Entity](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)和对应扩展页。列表中的示例字段是内容导航，实际逐成员目录须覆盖全部相关公开观测语义。

## 2. 类别与状态范围

| 对象 | 至少要覆盖的观测主题 |
| --- | --- |
| Entity 公共 | Type／Variant／SubType、身份／成员、存在／移除／Morph、位置／速度／大小、碰撞／伤害相关公开状态、血量／状态效果／计时、颜色／偏移／缩放、父子／生成者／目标等关系 |
| EntityPlayer | 全部公开本体字段和可查询状态：角色／形态、各类心量、资源、属性、武器／攻击／蓄力、主动／口袋槽、充能、装备／饰品／卡药、临时效果、服装与身体表现；控制关系引用 tx-04-09 模型 |
| EntityNPC | 状态／状态帧／计时、血量、目标、路径及可查询 AI 数据、冠军／Boss／阶段／部件、特殊形态与关系 |
| EntityTear | flags、伤害／尺寸／高度／下落／旋转、轨迹、附着／目标、专用变量与外观 |
| EntityProjectile | flags、伤害／碰撞、轨迹／高度／下落／加速、目标／变形与外观 |
| EntityBomb | 计时／引爆状态、flags、伤害／范围、附着／所有者、尺寸／闪烁／专用变量 |
| EntityLaser | 类型、起止／角度／长度／半径／宽度、曲线及采样点、碰撞／伤害、父子／附着／存续与专用状态 |
| EntityKnife | 所有者／挂载、角度／距离／旋转、投掷／返回／蓄力阶段、伤害及专用状态 |
| EntityPickup | 价格／商店／选项组、物品标识／循环内容、触碰／等待／开启／拾取状态、隐藏表现与专用变量 |
| EntityFamiliar | 所有者、跟随／环绕／队形／附着关系、计时／攻击／专用变量和形态 |
| EntityEffect | 类型、存续／范围／计时、关联对象、地面／持续效果、专用变量；保留其游戏作用与装饰属性的区别 |
| EntitySlot | 机器／乞丐变体、交互／工作／损坏／卡币、DonationValue／PrizeType／PrizeCollectible、奖品 Sprite、Timeout／Touch／TriggerTimer 等适用状态 |
| 专用扩展及未分类对象 | EntityDelirium 等专用接口；Wiki／枚举／实际枚举发现而未映射的类别，包括旧 UnknownEntity 的具体原因与补充路线 |

不要求每个普通 NPC 变体复制一套相同代码；共用继承字段，但特殊语义有扩展并明确适用域。Text／Dummy 等原生或实际出现类别不能被硬塞进 NPC；公开接口不足时列出缺口并按共同路线补充。

[EntitySlot](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md) 的同名字段在不同机器上含义不同，必须建立变体解释表；记录请求、实际值和生命周期，不按任意预期变体覆盖原值。[EntityDelirium](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md) 的公开专用状态也纳入，不由基本 NPC schema 推定覆盖。

## 3. 关联对象与动态外观

按对象引用继续展开 Sprite、Overlay、图层、资源文件／替换贴图、动画／目标帧、播放速度、颜色、变换、服装／部件、关联武器和公开容器；静态配置与运行状态分开保存。资源只读描述不能用 `Load`、`Update`、`Play`、`Render` 等动作来获取。Entity-owned Sprite 在此建立主记录；tx-04-14 对独立 Sprite／HUD 复用同一资源与姿态模型。

`GetData()` 的关联／命名空间入口纳入目录，表内容的完整适配归 tx-04-14；暂时缺失表内容须注明，不能在 tx-04-10 报告中冒称已记录完整 Mod 状态。实体和 Player 的 RNG 引用在此登记，随机流的统一内容及真实调用历史由 tx-04-14 扩展。

## 4. 采集和生命周期实现要求

逐页列公开字段、Getter 与参数域、继承项、特殊接口、相关回调，优先 Lua 直接采集。需要实体集合时用公开枚举并保留已有原生 membership／生命周期来源；二者时刻不同应显式关联，不将瞬时枚举缺席当成已销毁。

每个关联对象独立确认当前可用性；无效关系不递归解引用。创建后又在同一采样间隔移除的实体由生命周期事件表示，不能要求后态仍存在。Morph、Replace、恢复、地址复用、跨房持久、子实体和专用对象分别保留身份事实。

本 tx 建立完整 Entity 观测 schema 和逐字段目录，不把旧 78 字段的 ID 含义改写。新 profile 显式版本化；旧录制仍按旧 schema 解释。预算／队列不足时报告受影响 scope 和原因，不能通过排除机器／激光等得到“完整”。

## 5. 一次整体 E2E 与交付

一个统一入口编排全部类别、关键变体及字段语义的代表场景：创建／更新／移除、同窗口短命对象、Morph、引用变化、四参与者／多身体、实际 Slot 交互和奖品、Bomb／Laser／Knife、动态外观等。夹具可以制造场景，观测器不得主动开奖、掉落、施加伤害或推进 RNG。

输出完整成员目录、适用变体表、字段来源、生命周期／引用、原始样本、实际场景结果、预算／缺口和无额外玩法副作用的证据。必须区分“所有 API 已整理”和“所有实例／路径已观察”；未触发变体不记为运行通过。完整代码完成后仅一次整体 E2E，不逐子类提前测试，不强制对全部 Lua 成员做 native 比对。

## 6. 域模块与迁移交付

落实[迁移方案](domain-observation-architecture.md#11-现有-entity-与-grid-的迁移方案)：

- 建立 common 的上下文、主体键、引用、覆盖和有界发布合同，collector 保留统一时序／提交；entity 域拥有本体、派生字段、关系、生命周期和 Entity-owned Sprite，不能把协调器的全部可写状态作为模块接口。
- 迁出旧 Grid 的枚举／字段／事件代码，保留原范围与原判据；其 descriptor 声明旧能力，完整 Grid 子类与保存描述仍由 tx-04-11 交付。
- 拆分采集服务与可选录制消费端；world_record 关闭／写盘失败不阻断输入或正式纯值消费。保留 tx-04-09 四参与者、多身体锚点和资格撤销，不从旧双人源码覆盖现状。
- 新记录按公共 envelope 和域 schema 编码，并交付 v2 离线读取；旧 v1 从同一次采集兼容投影，保持旧 ID 含义、事件、gap 和完整性，不双读游戏、不双建身份。
- Player 身体仅有 Entity 身份；participant／control group 和后续跨房历史通过引用接入。random／mod 等未实现目标显式记录负责域与缺口，不能发布空的完整记录。
- 整体 E2E 同时覆盖结构迁移、原调用一次、身份与来源稳定性、录制隔离／停用、兼容输出，以及本计划全部 Entity 内容；失败在同一 tx 修复续测，默认不留 artifact。
