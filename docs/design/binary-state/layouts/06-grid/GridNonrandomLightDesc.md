# GridNonrandomLightDesc

由唯一布局源生成。布局编号 224；布局版本 89；原生大小 0x20；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本53：仅PressurePlate/Door内嵌描述，原生32B/UNKNOWN3B保持；线12B为type/variant/state。公共Desc198及其它Grid不变；Host种子与Init元数据不发，本地默认种子仍合法。

原生字节归属：

| 0x0 i32 type【状态】真实Grid类型；工厂选类及Room绘制pass/light的分支输入，变化走结构事务（4B；已核） | 0x4 i32 variant【状态】真实变体；Decoration隐藏、Poop绘制、Rock碎块选择及门/压力板光照消费，不自行触发Init或选动画（4B；已核） | 0x8 i32 state【状态】当前状态原值；Rock/Fire绘制与Poop/Rock水面裁剪、Room下层pass及光照的真实输入，不是Client玩法状态机（4B；已核） | 0xC i32 spawn_count【权威】来源初始化次数；Fire Init依据其选择随机变体。Client不重演Init，不能因旧保存描述发送过就恢复传输（4B；已核） | 0x10 u32 spawn_seed【权威】仅PressurePlate/Door：来源初始化/随机种子留Host；Light公共prologue建栈RNG但这两臂不消费，Client保留工厂合法默认值（4B；已核） | 0x14 i32 var_data【权威】Host格子玩法附加值；伤害/奖励与TrapDoor Update等消费。Client接收最终Sprite、状态及成员变化，不重演奖励或开关逻辑（4B；已核） | 0x18 bool initialized【权威】Host派生Init正常完成标志，可用于Host读取资格；不表示Client工厂已构造/资源已准备，不将来源字节写入Client充当本地资格（1B；已核） | 0x19 bytes[3] unknown_19_1b【未知】对象+1D—+1F；SDK字段间三字节，公共构造未赋予语义。即使保存时整段复制也不能证明填充，不读取或传输（3B；未知） | 0x1C u32 variable_seed【权威】Host玩法随机后保存的种子；不冒充Room Light实际使用的SpawnSeed，也不作为Client新建玩法RNG的输入（4B；已核） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 12—12B，不含第 9 步记录头）：

块 0：GD0 确定性Light类型的描述展示输入（world，12—12B）：

| +0x0 i32 type：真实Grid类型；工厂选类及Room绘制pass/light的分支输入，变化走结构事务 | +0x4 i32 variant：真实变体；Decoration隐藏、Poop绘制、Rock碎块选择及门/压力板光照消费，不自行触发Init或选动画 | +0x8 i32 state：当前状态原值；Rock/Fire绘制与Poop/Rock水面裁剪、Room下层pass及光照的真实输入，不是Client玩法状态机 |
|---|---|---|

证据：

- `type`：J273 VA0x704FD0初始化对象+4，0x706636写请求type；Room::RenderGridLight 0x801C24读对象+4；grid-common-evidence.md
- `variant`：J273 ctor 0x704FD7；Render 0x973190/0x710130/0x7146E0/0x95E140及Light 0x801BE0读对象+8；grid-common-evidence.md
- `state`：J273 ctor 0x704FDE；Render 0x7146E0/0x67B620，WaterClip 0x711620/0x718650，Light 0x801BE0读对象+C；grid-common-evidence.md
- `spawn_count`：J273 ctor 0x704FE5；Init 0x70508D递增对象+10；Fire Init 0x67A45F检查<=1；grid-common-evidence.md
- `spawn_seed`：docs/design/binary-state/grid-pressure-door-evidence.md；VA801BFF/801C0D及801BE0各case
- `var_data`：J273 ctor 0x704FF3；hurt_func 0x7055D6/0x7055EF读写对象+18；TrapDoor Update 0x71A7B0等；grid-common-evidence.md
- `initialized`：J273 ctor 0x704FFA写对象+1C=0，Init 0x705090在派生InitSubclass返回后写1；client_grid_native.h的来源/构造分离规则；grid-common-evidence.md
- `unknown_19_1b`：GridEntityDesc.zhl的bool+18/VariableSeed+1C间隙；J273 ctor 0x704FFA仅写1B、0x704FFE从对象+20写4B；grid-common-evidence.md
- `variable_seed`：J273 ctor 0x704FFE；hurt_func 0x7056F1等将对象_rng结果写入对象+20；已核绘制/水面/light不消费本字段；grid-common-evidence.md
