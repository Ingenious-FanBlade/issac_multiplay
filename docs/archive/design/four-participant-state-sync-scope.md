# 状态同步前的四人、世界覆盖与多Player归属范围

> **整局路线已建立（2026-10-30）**：见[tx-04-09—tx-04-34完整推进流程](state-synchronization-roadmap.md)。本文下述当前能力判断与早期单房间安排保留历史语境；四人／观测后继续完成整局复制、切房换层、全内容、重连Continue和产品发布，单房间不再是最终完成边界。本轮只改文档。

> **后续已明确（2026-10-30）**：用户现要求先编写[tx-04-09—tx-04-14连续计划](observation-expansion/README.md)，暂不进入状态同步实现。四人及多身体的实际支持安排在tx-04-09，完整Entity／Grid／世界观测依次安排在后续tx，公开Wiki／Lua优先。本文是较早同日的范围澄清，当前能力判断不因计划形成而变成已实现；下文提及Replica的工作顺序改按最新计划延后。

日期：2026-10-30，Asia/Singapore。来源：用户提出当前DLL、完整世界记录、恢复四人和双子角色四个问题，并选择本轮“先明确现状与下一阶段设计”。只读源码与修订设计，没有修改运行代码、构建或测试。目标仍为官方REPENTOGON1.1.3 / J273。

## 当前能力

| 问题 | 实际能力及边界 |
| --- | --- |
| 跨端共同探索单房间 | 尚未实现。DLL网络只实现Authority，Remote为Rust输入客户端，包只有Hello/Input/Ack/Bye。房主游戏能消费远端B输入，但无世界状态下行、Client/Replica或baseline/delta apply。双进程E2E是顺序独立采集和离线比较，不是两个实时共享世界的游戏客户端。 |
| 整个世界记录完整 | 没有。tx-04-08完成min-read-v1：Player/NPC/Tear/Projectile/Pickup/Familiar/Effect七类专用字段、Grid/门、来源/生命周期/身份与离线工具。78字段是固定读取profile，不是全部游戏状态或完整可重建外观/HUD。 |
| Slot/Machine | 未完整支持。Slot是独立Entity派生类，不能因已有Grid采集就视为机器状态已覆盖。尚无专用schema、目标版本专用字段/表现合同或安全应用路线。未知类保留UnknownEntity头部与UNKNOWN_CLASS，不冒称完整样本。Bomb/Laser/Knife等也在七类schema之外。 |
| 四人 | 当前仍双人。SessionIdentity.clients[2]、Host ParticipantCount=2、协议clients[2]、单PeerBridge/Worker、单DeviceK/Join/life、两份tick/metrics/topology及collector anchors[2]；不是配置开关。 |
| 一人控制两个Player | 尚无完整联机支持。sub_player/twin/backup/replaced关系已登记，但当前按ordinal取roster槽、PlayerIndex==ordinal，将一个主要Player绑定一个设备；Join要求初始roster数量1。复杂角色可能被拒绝、丢失归属或落入未受控透传，不能因原生偶然可动称为支持。 |

依据：`mod/src/net/{protocol.h,worker.h}`、`mod/src/core/session.h`、`mod/src/game/{state.h,join.cpp,topology.cpp,consumers.cpp}`、`mod/src/world/{collector.h,collector.cpp,model.cpp}`和`mod/binding/world_layout.json`。旧版本Slot RTTI索引不能代替J273重新核定；已读取关系也不证明其目标当前活动或可读。

## 用户要求的四人目标

下一阶段取消双人产品上限，恢复**最多四名参与者，典型拓扑为一名房主加三个远端**。这是四个独立输入/连接身份，不是四个Entity_Player对象。当前二人实现与历史PASS保持原范围。

改造贯穿会话/握手与版本、多peer输入和Ack、原生Join与K归属、掉线/死亡/复活/退出、调度、采集及每客户端复制状态。不能机械将所有2替换为4；二维坐标、动作轴等含义不变。每参与者独立sequence/target tick/去重/Resolve/代次，房主共用调度，Authority日常tick不等待全员。K按每远端独立归属，维持NEVER和主线程生命周期，不由单个全局B状态代替三人的隔离。

## 多Player控制组

必须分开以下身份：

| 身份 | 职责 |
| --- | --- |
| participant/client/ordinal | 一名人的输入、网络连接与会话内稳定身份，上限4 |
| control group与generation | 当前被原生确认的一组主要Player、附属Player和设备关联，失效后重建 |
| entity_id与role | 每个Player独立网络实体身份、角色和引用，不以participant ordinal代替 |
| pointer/PlayerIndex/controller key | 当前主线程代次的临时关联证据，不直接作网络ID |

一名双实体角色占一个参与者名额，两实体分别入状态模型。活动twin/sub-player、Esau Jr备份、replaced历史对象要分开，不能把非空关系目标全部当作同时受控实体。四人均有两个活动身体时模型至少须表达八个活动Player；原生创建、roster和设备的具体能力尚须在J273核定，现有读取预算不提供此保证。

一个参与者的命令经一次统一Submit/Resolve；多个原生consumer按已核定控制组和角色读取不可变意图，由原生角色规则决定差异。不能对两个身体各提交一份网络命令，也不能全局消费边沿导致第二身体丢动作或盲目复制边沿产生重复副作用。共享/关联设备的实际规则须按具体角色证明，不能由一个twin指针泛化所有角色。

每个关联实体单独记录创建/更新/读取资格、存在性与引用；切房、Replace、死亡复活和设备重分配撤销旧关联。不把主要Player的可读资格转赠给所有关系目标。字段已登记不等于双子完整可玩，具体角色支持名单与原生路线应随实现明确。

## 状态同步与机器覆盖次序

先统一四参与者及多身体身份模型，并适配Authority输入/Join/生命周期；同时为单房间明确Replica支持集合，避免把A/B与一人一实体固定进新wire。

若首个可探索房间含Slot/Machine，须补齐J273分类、owner/成员关系、可见结果的专用字段、Sprite/动画/外观、引用，以及客户端创建/更新/移除的无gameplay副作用适配。机器裁决、消费与掉落由Authority执行，Replica展示结果；纯展示客户端不需要复制可继续权威模拟的全部RNG/AI内存。具体字段/API/变体尚未核定，不能猜偏移或承诺全部机器已覆盖。

随后形成Replica schema、baseline、创建/更新/移除与引用事务、每peer Delta/Ack/恢复和双端实时闭环。可以先在明确支持的房间建立闭环，再扩类别；若承诺含机器的房间，须在其实际验收前完成机器支持，不能静默忽略。首版仍单房间、无预测；四人不自动增加跨房/late join/任意Mod/全角色武器/Dedicated Server承诺。

未来实现的统一E2E需保留四参与者原始输入/包/Ack、控制组/实体ID、真实Join/退休、双身体动作、机器状态/展示、客户端apply前缀和副作用证据。分析修改期间不预跑，不新增实现后单测，不设gate或哈希要求。

## 本轮静态审阅

统一复读session、network、Join/topology、consumer、collector、layout及状态同步设计，确认当前/目标/未知分开：四人未冒充实现，Slot不冒充Grid/NPC，关系观察不冒充双子可玩，tx-04-08验收不冒充Replica完成。本轮仅一次静态文档审阅，没有程序测试、构建、游戏或离线CLI。
