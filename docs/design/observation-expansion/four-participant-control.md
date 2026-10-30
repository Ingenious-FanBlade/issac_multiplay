# tx-04-09：恢复四人并支持一人控制多个 EntityPlayer

日期：2026-10-30。状态：四参与者／多身体功能已通过统一整体验收；性能开销与原生限制另列。实现、静态研究主审、失败修复及结果见[实现说明](four-participant-implementation.md)。本计划由[连续推进总览](README.md)统一约束。

## 1. 完整目标

取消双人产品上限，支持一至四名参与者，典型满员结构为房主加三个独立输入远端。支持雅各布／以扫这类一个参与者关联多个 EntityPlayer 的角色；不能只保存 twin 指针、将第二身体当额外玩家或解除数组限制就宣称完成。

该 tx 实施输入、会话、原生 Join、控制组与生命周期，不实现远端游戏画面或世界状态下行。Rust 可继续作为多个独立输入远端的 E2E 工具，不能称为三个游戏 Replica。

## 2. 身份和控制模型

| 身份 | 语义 |
| --- | --- |
| participant／client／ordinal | 一个人的输入与连接身份；最多四名，当前人数不由 EntityPlayer 总数推断 |
| control group／generation | 该参与者当前有效的一组身体、角色与设备关联；可以有多个活动 Player |
| player entity ID／role | 每个身体各自的稳定观测 ID，主要／关联／非活动／备份／替换等角色独立表示 |
| PlayerIndex／ControllerIndex／指针／device key | 原生当前代次关联信息，不直接作 participant ID 或跨代稳定实体 ID |

同一控制组命令只经过一次 Submit／去重／Resolve；多个原生消费者按原生角色规则读取同一不可变意图。边沿不能因第一个身体读取而被全局消费，也不能给每个身体重新提交一次动作。不同参与者的输入、sequence、队列、缺输入策略和设备不得串用。

模型按集合表达身体，不能用 `Player[4]` 同时表达参与者与全部身体。四个双身体控制组至少需要容纳八个活动 Player 的表达空间；目标版本的实际原生创建、roster 和角色组合能力须在本 tx 中查明，不能预先宣称已支持，也不能因超出四个身体静默截断。

## 3. Wiki 优先入口及角色语义

- [原版 EntityPlayer](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)：`GetOtherTwin()`、`GetSubPlayer()`、角色／身体相关查询及 ControllerIndex 等公开信息。
- [REPENTOGON PlayerManager](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PlayerManager.md)：`GetPlayers()`、`GetEsauJrState()` 等列表与备份关系。
- [REPENTOGON EntityPlayer](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)：`GetPlayerIndex()` 及控制器相关语义。`SetControllerIndex(..., IncludePlayerOwned)` 的文档可帮助理解关联范围；它是写接口，不作为无条件重绑方案。

`GetOtherTwin()` 的非空值可能表示双实体或其他形态关系，不能统一当成同时活动、同设备控制的身体。至少建立雅各布／以扫、遗骸／魂及堕化遗骸、堕化拉撒路与长子权、Esau Jr 备份／切换的角色关系表，逐项区分并发活动、切换、历史和备份；原版／REPENTOGON 可枚举的其他 Player 关系也纳入目录。对有真实控制差异的角色逐项落实，不把普通双子结果外推所有角色。

公开Player列表不预设包含所有附属身体；结合列表、关系遍历和各对象活动资格建立完整组，处理关系循环、暂时缺席及冲突。不能只因ControllerIndex相同就认定同一参与者，也不能只因不在主要列表中就忽略twin／sub-player。每个身体使用其Entity观测ID，不在控制组内再创建一套无关联实体身份。

## 4. 工程范围

| 模块 | 必须改变的行为 |
| --- | --- |
| core／session／participant | 去除 Host 固定返回 2 和单 B 语义；参与者集合、稳定身份、状态、容量及单人有效情况 |
| net protocol／worker／Rust 远端 | 四人握手与名单、多 Peer 路由、每 Peer nonce／session／ACK／超时、协议版本和拒绝规则；不是将坐标等所有数字 2 换成 4 |
| state／K／Join | 每个远端独立 owned K 和 Join 状态；原生加入成功与角色／控制组关联，不能继续要求初始 roster 只有一个 EntityPlayer |
| topology／consumer | 取消 roster[ordinal] 且 PlayerIndex==ordinal 的普遍假设；按已确认参与者—组—身体—设备关系消费 |
| 调度与 lifecycle | 房主共用队列；日常不等待全员；一个 Peer 缺输入／退出时不污染其他人；切房、死亡复活、Replace、切换和 teardown 撤销旧关联 |
| collector／ops／UI／trace | anchors、预算、状态输出和命令按参与者集合及多身体映射扩展；显示参与者数与身体数；保持旧录制来源可解释 |

现有 NEVER 策略保持：每个远端参与者在该 session 至多创建一次自有 K，进入 BASE_CONSTRUCTED 后不释放；失败不自动重建。原“每 session 一次 K”是旧单 B 范围，本次按参与者分别管理，并维持 DLL 常驻和主线程约束。新增／离开／失败时的参与者名单策略须在实现文档说明，不将单 B 的全局退休无条件套到所有 Peer。

优先复用已成立的输入语义、启动／重绑窗口与屏蔽规则，不为四人改成锁步。Lua 公共关系可作为正式拓扑来源；仅对其不能解释的设备消费、原生创建或生命周期部分补必要研究。

## 5. 一次整体 E2E 的目标

由一个统一入口覆盖人数从一至四、三个独立远端、满员后越界加入、各人同时不同输入、按键边沿、缺输入／超时／退出隔离，以及暂停／加载／切房／死亡复活／停止等真实生命周期。记录每人输入到实际身体的关联，不能只核对四个 ACK。

角色场景至少包含普通角色混合雅各布／以扫、多组多身体共存、角色形态／备份切换。对四组双身体的目标组合记录实际原生结果；若原生能力仍未解决，明确该功能未完成，不以模型容量代替原生可玩证明。场景中确认不额外占参与者名额、不串控、不丢第二身体动作、不重复有副作用的边沿、不控制退休／备份对象。

验收判断覆盖参与者／控制组／实体 ID 图、公开 API 来源、实际 roster／设备代次、输入／ACK／消费记录、原生 Join／退休和未决组合，结果如实说明。完整实现后进入统一整体 E2E，默认连续修复与续测；进入验收前不试跑。tx 本身不再保留 artifact，E2E 仅在中途停下且需要记录信息时保留必要材料，主题标识为 `tx-04-09-four-participant-*`。既有 [ASM patch 同号历史](../tx-04-09-asm-patch.md)保持原结果。

## 6. 与后续 tx 的分工

本 tx 交付真实四人及多身体控制能力与基本关联观察；tx-04-10 在该模型上扩全 Player 本体字段；tx-04-13 补全跨房、备份和整段控制关系历史。后两项不能被用来推迟本 tx 必需的正确归属、切换与失效处理。
