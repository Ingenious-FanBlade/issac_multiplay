# tx-04-14 四域记录与适配接口

版本：`isaac.cs.observation` envelope v2；目标为REPENTOGON 1.1.3 / J273。[实现与实际验收](tx-04-14-implementation.md)、[逐成员目录](run-catalog.md)。这是观测记录合同，不是网络同步ABI。

## 来源与时序

| schema前缀 | 普通后态 | 独立记录 |
| --- | --- | --- |
| `isaac.cs.run` | Game规则/进度、Seeds、Ambush、概率、四类时钟来源 | `.event`、`.transition`、`.query` |
| `isaac.cs.random` | owner限定流状态、pool成员/权重、移除/黑名单、药丸映射/鉴定、饰品可用性 | `.actual-call`、`.event`、`.query` |
| `isaac.cs.presentation` | HUD/PlayerHUD/心布局/HistoryHUD、HUDMessage文本与Sprite、ItemOverlay、音乐/音效控制、独立Sprite | `.render`、`.actual-call`、`.event`、`.query` |
| `isaac.cs.mod` | 具名adapter声明及有界图 | `.event`、`.transition`；显式事件发布可以选Render/ActualCall阶段 |

普通四域在同一个native复制窗口顺序采集，受统一最终提交/取消控制，默认每30次合格capture采样。`sample_revision`为当前scope，不是值内容摘要。独立记录在实际来源时发布，不设置Authority tick；普通窗口取消不撤销此前实际事件。Render不能作为普通世界后态。Lua主线程内只复制已有状态，后台只接收自有JSON值。

通用字段保留既有envelope：session/epoch/scope/sequence、phase、owner/outer、run/floor/dimension/descriptor/room、可空authority_tick、coverage及预算/原因。正式最新值与可选录制是不同consumer，停录不改变采集epoch。所有ID为十进制字符串，不输出原生指针。

## 值、身份与预算

整数通过`{type="int64",value="..."}`或身份字段的十进制字符串保持精度；浮点保留API原值。Null、Empty、Unavailable、NotCaptured、ReadFailed、Nonfinite、Truncated分别表达，未知不补零。scope的Complete不表示任意私有引擎状态完整，目标覆盖范围以range和逐成员目录为准。

每个新普通域payload上限8MiB、262144 nodes、depth 12、16384 objects；编码超预算明确Partial/裁剪原因。Mod默认单adapter图预算为4096 table nodes、16384 entries、1MiB字符串、depth 12；额外受scope预算约束。图使用`table/node/entries`与`graph_ref/node`保留循环/共享/稀疏键。函数、线程、未登记userdata与非有限值明确标记，预算耗尽保留截断原因，不静默省略成员。

Entity/Grid复用已有规范subject/use；Player不是另一套身体ID。random space=11、独立presentation space=12、Mod逻辑实例space=14的local_id经统一原生服务分配，并由session/epoch限定。StageAPI同名/同格/同PersistentIndex重建不继承旧实例ID；表对象只在游戏主线程的弱关联表中作为当前实例见证，Lua地址不进入记录。

## 随机与时钟

流记录包含subject、owner、purpose、parameters、lifetime、first_observation、state和freshness。seed和shift是状态，不是流ID。固定Entity Drop、Grid及Level RNG只在当前资格来源重新取得时读取；原生RNG指针不会跨生命周期缓存解引用。物品RNG不为遍历ID空间而懒创建，只记录真实Getter或具名Mod reader交付的对象。历史状态保留实际source_scope/应用帧，不换成当前采样时刻。

真实Lua调用包含call/parent_call、member、完整参数数目和nil洞、成功/异常、多返回值、可获得的before/after。wrapper原调用一次；观测内部读取静默，不制造消费。池抽取方法不进入轮询器。原生内部消费和wrapper安装前缓存的函数单列来源缺口。

| 时钟/概率 | 记录含义 |
| --- | --- |
| Authority tick | envelope中已提交调度tick，独立事件为空 |
| `Game:GetFrameCount()` | 游戏API原帧数，不换算成项目tick |
| `Game.TimeCounter` | 游戏计时器原值 |
| `Isaac.GetFrameCount()` | 应用/渲染相关原帧数，与暂停中的普通scope分开 |
| Sprite frame/Entity age | 各自对象的原API值，由已有Entity/Sprite模型管理 |
| `Level:GetAngelRoomChance()` | Angel modifier，明确不是最终概率 |
| `Room:GetDevilRoomChance()` / `Level:GetPlanetariumChance()` | 各Getter公开原结果，条件原值引用Level规范来源，不自行覆盖 |

## 具名适配

`IsaacCSModObservation.Register(spec)`要求namespace、version、schema、lifetime、phases和只读read函数；另可声明resources、unexposed、budget、reference及提交outcome。read接受当前纯值位置和复制上下文；返回的已有Lua图在主线程有界复制。phases声明允许的来源，普通read由普通采集调度；其他阶段由真实Mod调用Event交付，不自动触发Mod的Update/Render/保存。

`Event(namespace,name,data,phase)`发布真实Mod事件；`Unregister(namespace)`结束注册。既有Entity附加表由适配reader持有的已存在表读取；保存扩展使用`EntitySaveStateManager.TryGetEntityData`，不调用会建表的Get方法。任意Mod私有闭包、函数代码、未知原生userdata不能由通用表遍历自动补全。

`IsaacCSPresentationObservation.Register(namespace,name,reader)`登记独立Sprite/显示纯值；返回nil表示当前对象不存在。重新登记分配新的presentation主体，Unregister记录退休。Entity-owned Sprite与Minimap仍保留既有规范来源，不另建世界真值。

StageAPI适配版本为metadata 2.43、schema `isaac.cs.mod.stageapi.v1`。读取已有LevelRooms、CustomGridEntities、CustomGrids和实际框架回调；不调用会建表的GetRoomCustomGrids。逻辑RoomType/TypeOverride保持原类型，布局/配置/Data/PersistentData归Mod图。当前逻辑房间关联实际原生Descriptor；离房对象的LevelIndex须与实际描述位置/维度一致，否则Unresolved。CustomGrid的原生载体引用既有Grid主体；GridEntity=nil明确Null且仍保留逻辑实例成员。

当前绑定的HUDMessage Getter替代旧Wiki两个Sprite别名，另复制主/副文本及显示状态。GenericPrompt通过`IsaacCSObservationNative.PromptState`直接复制活根公开值及Sprite姿态，避开官方Getter对未构造C++ userdata的ANM2赋值复制。该桥接不调用Initialize/Update/Render。
