# tx-04-13：探索、离房保存、控制关系与玩家跨房状态的完整观测

计划日期：2026-10-30。2026-10-31状态：tx-04-13已实现，统一E2E **5/5 PASS、31/31场景**；实际范围、连续修复和边界见[实现及验收](tx-04-13-implementation.md)。下文保留完整实施目标，遵循[共同观测合同](README.md)。

架构按[按域拆分详细设计](domain-observation-architecture.md)实施，本 tx 接入 exploration、room_save、player 三域，复用已有上下文与主体／引用合同，不另造 Player 本体或沿用活动对象 ID 表示保存项。

## 1. 完整目标与 Wiki 入口

从活动房间的单个样本扩展到探索和跨房连续历史，补全地图与探索、离开后的房间、玩家与控制关系及玩家跨房状态。仍只做观测，不实现跨房状态同步或重连。

公开资料优先：

- 原版 RoomDescriptor、Level、Room、EntityPlayer、TemporaryEffects、Seeds 及相关容器／枚举／回调。
- REPENTOGON [Minimap](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Minimap.md)、RoomDescriptor、[EntitiesSaveState](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitiesSaveState.md)、EntitiesSaveStateVector、GridEntitiesSaveStateVector、[PlayerManager](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PlayerManager.md)、EntityPlayer、History 及相关扩展。
- 先用 RoomDescriptor 的 `GetEntitiesSaveState()`、`GetGridEntitiesSaveState()`、`GetDecoSaveState()` 等公开入口，不能默认需逆向全部离房保存布局。

## 2. 四个观测域

| 域 | 至少要覆盖的内容 |
| --- | --- |
| 地图与探索 | 楼层／Dimension 图引用、发现／访问次数／清理、DisplayFlags／图标／显示状态、地图揭示／诅咒／隐藏关系、公开 Minimap 状态及资源；世界已知信息与本地展开／显示偏好分开 |
| 离开后的房间 | 每个描述的有效期、保存 Entity／Grid／装饰集合、已拿走／破坏／改变的内容、机器／门／奖励等可持久结果、从活动对象到保存项及回访对象的来源关系 |
| 玩家与控制关系 | participant—control group—Player—设备的完整关系历史；活动／非活动／备份／替换／twin／sub-player、所有者／关联实体、Join／切换／死亡复活／退出／设备重分配和失效原因 |
| 玩家跨房状态 | Player 本体状态的跨房连续性，装备／饰品／卡药／主动槽／充能、各类心量／资源、临时效果／变身／服装、跟随／持有关系、角色备份与恢复、公开物品历史和相关计数 |

Player 本体字段沿用 tx-04-10，控制语义沿用 tx-04-09；本 tx 补足连续观察和跨房／保存域，不能另建与之矛盾的玩家真值。

## 3. 离房与回访的语义

不为了采集而主动访问每个房间，不调用会切房／恢复／生成的操作来假装读取离房状态。区分已生成但未访问、当前活动、已有离房保存、保存信息不足、实际不存在五种情况。

保存项不自动继承原活动实体 ID，也不以 seed／索引相等强认同一对象。可证明为同一逻辑对象时建立对应；只能证明由某保存项恢复时记录来源边；无法判定时保留未解析关系。房间退出不等于所有 Player／Familiar 已销毁。

保存容器的字段按 Wiki 展开；其未包含的临时字段不能补成“与离开前一样”。若完整离房观测确需额外公开回调或已有内存只读来源，按具体缺口补充，不将 SaveState API 名称当成完整引擎存档证明。

## 4. 多身体与跨房连续性

每个 Player 身体单独维护 ID、角色与活动资格；参与者不因双身体多占名额，也不因房间重建自动换身份。备份／替换／非活动形态与当前受控身体分开；关联为空、未解析或冲突分别记录。

共享资源、每参与者资源、每身体资源及角色特殊共享关系以公开语义和实际结果为准，不将一个身体全部字段复制为另一个身体。Inventory／TemporaryEffects／History 的含义不同：持有、临时效果、历史获得顺序分开，不互相替代。

跨房前后样本标明真实阶段与新代次；菜单／run teardown 后旧关系退休，不通过保留 userdata 继续读取。所有新状态都引用 tx-04-12 的楼层／房间身份，不能只按屏幕上的房号串联。

## 5. 一次整体 E2E 与交付

一个统一入口覆盖探索／地图揭示／隐藏、不同维度、道具拿走／Grid 破坏／机器变化后离开与回访、Player 与 Familiar 跨房、临时效果／充能／库存变化、多身体切换／备份／死亡复活，以及换层／teardown 的旧状态退休。

保留离开前、离房保存、回访后各自原始记录和时间关联；真实未持久字段如实表现，不强行要求所有字段保持原值。输出完整成员目录、探索历史、保存集合、控制关系图、Player 连续记录及实际场景结果。未观察项和缺口不以普通往返成功代替。全目标完成后仅一次整体 E2E。

## 6. 三个域的接口与引用边界

- exploration 管发现／访问／清理、地图知识与历史，引用 level 的维度／描述；Minimap 的显示姿态归后续 presentation，不能与世界探索字段重复建真值。
- room_save 管非活动房间保存集合、SaveEntry 身份及集合 revision。活动对象、保存项、回访对象分别登记，只有有依据时建立 SavedFrom／RestoredFrom 关系；同 seed／索引不是连续身份依据。
- player 管 participant—control group—Entity 身体关系、跨房连续性、备份／替换及共享关系。心量、装备、充能、TemporaryEffects 等本体值沿用 entity 的规范记录，读取封存值时必须保留其来源 scope／有效期；必要新鲜采集由协调器调度，不能直接调用 entity 采集器。

各域独立提供 schema、变化／生命周期采集、编码／reader 和失效条件。保存集合、控制关系与房间过渡按实际来源单列 scope，不能把离开前与回访后值拼成一个原子快照；缺失保存字段、未解析关系、未访问房间和空集合分别表达。

跨房不批量退休仍有连续性的 EntityPlayer；run teardown、控制换代或观察空档则按[身份合同](domain-observation-architecture.md#7-跨域身份与引用解析)撤销相应资格。上述边界与多身体、地图／保存版本变化一起进入本 tx 的统一 E2E。
