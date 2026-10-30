# 正式只读 Entity／Grid 来源

目标为 REPENTOGON 1.1.3／J273。随 `zhlIsaacCS.dll` 安装到游戏 `mods/isaac-cs-observation`，与 `mods/isaac-cs-ui`、测试夹具分开。默认没有消费需求，不枚举世界。

`cs observe start|status|stop` 管理正式纯值消费；`cs world start <label>|status|stop` 管理可选录制。关闭或失败的录制器不重置仍活动的 observation epoch。纯值消费者在主线程借用不可变 envelope；若要后台使用，必须复制到自己的有界出口，不持有 Entity userdata。`world_record.enabled=false` 仅禁用录制命令，不禁止正式采集。

普通 Entity 完整来源合并同一 native 窗口内的实际成员与公开 `Isaac.GetRoomEntities()`；先登记主体再复制字段。原生身体身份、创建／Init／出生历史与公开字段来源分别表达。Player 不获得第二套对象 ID。公共作用域的 Entity 与旧 Grid 每 30 次普通采集取一次扩展样本；旧 v1 需要的 native 样本仍沿原时序采集。生命周期及短命对象按实际回调记录；Entity-owned Sprite 每个主体 30 次真实 render 回调单独采样，render 没有 Authority tick，不与后态拼成原子快照。

逐成员目录由 `mod/tools/gen_entity_catalog.py` 生成到 `domains/entity/schema.lua`、本目录 `entity-catalog.json` 和设计目录的 JSON／Markdown。继承和明确同义 Getter 共用规范字段 ID，保留来源。只读本体字段和无参状态查询自动采集；主动／口袋／武器／袋子等有限槽域分别记录参数与实际结果；无界几何、字符串、物品条件等派生查询由 `IsaacCSObservation.Query(owner, class, member, arguments, associatedObject)` 显式调用并记录。参数目录完整不表示所有参数组合均已实际调用。

动作只归入目录及真实事件，不为采集执行。GetData 的 Mod 内容、RNG 流及真实消费历史、离房保存和独立 HUD 等由相应后续域负责，记录明确引用／负责 tx。GetLootList、随机开奖、掉落选择、攻击参数生成等不进入轮询。当前事件来自统一 native 观察清单及公开 post 回调，不声称逐条 Hook 了所有 Lua API 调用。

`NativeText`／`NativeDummy` 的目标版本补充见 `docs/design/observation-expansion/entity-native-supplements.md`。其他未知类保留 UnknownEntity 和具体原因；不能以公共头部替代完整派生状态。

Grid 域由 `domains/grid/collect.lua` 组织全部17个公开派生转换、运行字段、保存描述、主／额外／TimerPlate Sprite、门查询与参数域。`GridEntity.Desc` 与 `GetSaveState` 共用字段；未知类型保持具体缺口。全部固定 Grid 槽与门槽来自相同 native owned 批次，前后复核仍由唯一协调器完成；index 不是身份。同格删除再创建由真实 native owner 退休旧主体并分配新主体。`lifted_descriptors` 使用 Effect 身份并保留无法公开关联源 Grid 的 Unresolved 原因。

`IsaacCSGridObservation.Query(owner, class, member, args)` 仅允许目录只读查询。门 `IsRoomType` 和岩石 `GetAltRockType` 自动枚举有限公开枚举；RNG 复制 seed／shift 并以当前主体和成员限定所属流，不推进随机数，统一随机目录仍归tx-04-14。GridGetRenderPosition 使用已声明SDK的栈缓冲，规避官方Lua包装的临时Vector泄漏。真实post-update复制运行变化，render每30次回调独立复制姿态；spawn与rock-destroy保留实际回调，原调用及GridFree仍由统一native Hook清单观察。不声称拦截所有Lua玩法方法。

Grid逐成员、类型／变体枚举与回调目录由`mod/tools/gen_grid_catalog.py`从本地Wiki转换，录制v2附带`grid-catalog.json`，新schema不改变min-read-v1的旧字段编号和判据。floor／dimension／room descriptor规范身份由后续room/level域建立前明确Unknown，当前run／room instance归属沿现有观察器代次，不以房号冒充跨时间身份。
