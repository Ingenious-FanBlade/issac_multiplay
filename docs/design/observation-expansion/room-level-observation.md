# tx-04-12：房间身份、构建、进度、环境与楼层结构的完整观测

日期：2026-10-30。状态：代码已实现，统一 E2E 5/5 PASS；见[实现及实际验收](tx-04-12-implementation.md)。遵循[共同观测合同](README.md)。

架构按[按域拆分详细设计](domain-observation-architecture.md)实施，本 tx 接入 room 和 level 两域，并补全公共上下文中的楼层／维度／描述／活动实例身份；复用 10—11 的公共接口与 Entity/Grid 身份。

## 1. 完整目标与 Wiki 入口

补全房间作为世界容器及楼层结构的观测，不把 Entity＋Grid 集合当成整个房间。不开发客户端构建／切房／换层；本标题的“构建”是**观测房间构成与实际生成结果**。

优先逐成员整理：

- 原版 [Room](F:/projects_isaac/IsaacDocs/docs/Room.md)、[RoomDescriptor](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md)、[Level](F:/projects_isaac/IsaacDocs/docs/Level.md)、RoomConfig_Room、Game 及对应枚举／回调。
- REPENTOGON [Room](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md)、[RoomDescriptor](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md)、[Level](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md)、RoomConfig／RoomConfigRoom／RoomConfigStage、RoomTransition、Backdrop、FXParams。

API 读取优先；配置目录和运行对象分开，候选随机房间选择函数不用于“读取当前房间”。

## 2. 四个观测域

| 域 | 至少要覆盖的内容 |
| --- | --- |
| 房间身份与构建 | run／floor／dimension／room 标识及代次；房间索引的实际语义；Type／SubType／Variant／Shape、配置／Override、布局／边界／可用格子、资源／背景、门槽／允许／实际门、入口／出口、实际生成及过渡关系；生成／装饰／奖励种子作为具名引用 |
| 房间进度 | Clear／ClearCount、访问与当前进度、挑战／伏击／波次、清房奖励／NoReward、献祭／机关／特殊事件、商店及奖励相关公开计数、实际事件和状态变化 |
| 房间环境 | 水面／水流／水坑、Backdrop、FXParams、颜色／光照／阴影／暗化、地板／墙面与持续环境效果、装饰／地面遗留物、环境计时与相关表现对象 |
| 楼层结构 | Stage／AbsoluteStage／StageType／分支路线、Dimension 集合、房间描述集合、RoomShape／连接图／邻接和门目标、特殊房间／特殊入口、Ascent 等结构变化、楼层生成结果及结构相关 flags |

这里列出的是主题；实际成员目录需要覆盖全部相关公开状态与参数域。不能因为某个值只在特殊房间有效就从全局目录删掉。

## 3. 状态归属与一致性

房间持久描述、当前活动 Room、配置资源与楼层图分别记录身份和有效期。GridIndex／ListIndex／当前房间索引等按文档分别解释，不假设相同，也不使用原生地址或单一索引作全局稳定身份。

门的实际存在、允许门槽和邻接候选分开。REPENTOGON `RoomDescriptor.AllowedDoors` 描述当前启用门槽，不能用它替代所有形状允许的门；门本体引用 tx-04-11 记录。

跨房／换层期间的读取标明过渡阶段、前后 room／floor／dimension 及有效来源。活动房间的字段不能配上新楼层旧 descriptor；过渡样本单列，不伪装成正常完整房间状态。所有集合需明确 membership，不能把未遍历维度当作空维度。

房间 FrameCount／计时在本 tx 记录原值和局部作用域，统一时钟关系由 tx-04-14 汇总。生成／奖励／装饰种子保留不同用途，不把同 seed 当成布局／装饰／掉落已相同。

## 4. 与其他任务的边界

- tx-04-10／11 管对象本体；本 tx 记录其 room membership、构建来源与环境关系，不重复定义字段。
- tx-04-13 扩展地图可见性、探索历史和非活动房间保存记录；本 tx 先完整观察实际楼层结构和当前 Room／Descriptor。
- 恶魔／天使／星象房相关实际房间和结构在本 tx；概率、影响条件及整局计数归 tx-04-14。发现相关字段时登记引用，不丢失它。
- 独立 Sprite／声音／HUD 的通用记录归 tx-04-14；持续环境对象或参数不能只留一个瞬时表现事件。

## 5. 一次整体 E2E 与交付

一个入口覆盖普通／特殊房间、不同 Shape、门槽及房间连接、清房／奖励／波次／献祭等进度，水／光照／持续环境变化，真实切房／回访／换层和目标版本适用的 Dimension／特殊路线。夹具操作与观测模块分离，记录实际构建结果。

交付 Room／Descriptor／Config／Level／环境的完整成员目录、结构模型、采样时序、对象引用、过渡记录与实际场景结果。未触发路线、维度或状态如实列为未观察，不由同一普通房间的成功外推。全目标完成后仅一次整体 E2E。

## 6. 域归属与时序接入

room 管活动房间的身份、构建、进度、环境和实际 membership；level 管楼层／维度实例、描述集合和结构连接。两域各自拥有模型／schema／采集／编码，collector 仅按[公共时序合同](domain-observation-architecture.md#6-采样时序与提交)调度。字段引用 Entity/Grid，本 tx 不重复定义对象本体或门本体。

楼层结构与配置按生成／变化发布带 revision 的状态；活动房间进度／环境按其实际需要采集。版本复用保存原采样 scope 和失效依据，不以最近一次缓存冒充每 tick 新值；变化通知不足以保证有效时，应重新读取或明确 freshness 未知。

分别建立 Run、Floor、Dimension、RoomDescriptor 和 RoomInstance 的有依据身份。切房／换层进入过渡即撤销受影响窗口和缓存，新的稳定采样单列；不能把新房号配到旧描述，也不能因此认定跨房 Player 已销毁。尚无来源的层级保持未知，不能用地址或单一房间索引填充。

交付包括各层级引用、失效通知、缓存版本有效性、schema／reader 和过渡 scope；跨域混代、旧缓存复用及门本体重复定义等失败情形纳入同一 tx 整体 E2E。
