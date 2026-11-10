# GridEntityDesc

由唯一布局源生成。布局编号 198；布局版本 56；原生大小 0x20；未知 3B；完整：否。

版本50沿用版本48的四项16B描述展示输入，原生3B UNKNOWN保持。SpawnSeed保留Light实际依赖，Host初始化/玩法/保存元数据不恢复。119项全目录及运行接线仍待后续；见grid-common-evidence.md与grid-common-synchronization-review.md。

原生字节归属：

| 0x0 i32 type【状态】真实Grid类型；工厂选类及Room绘制pass/light的分支输入，变化走结构事务（4B；已核） | 0x4 i32 variant【状态】真实变体；Decoration隐藏、Poop绘制、Rock碎块选择及门/压力板光照消费，不自行触发Init或选动画（4B；已核） | 0x8 i32 state【状态】当前状态原值；Rock/Fire绘制与Poop/Rock水面裁剪、Room下层pass及光照的真实输入，不是Client玩法状态机（4B；已核） | 0xC i32 spawn_count【权威】来源初始化次数；Fire Init依据其选择随机变体。Client不重演Init，不能因旧保存描述发送过就恢复传输（4B；已核） | 0x10 u32 spawn_seed【状态】来源种子原值；Room Light以该值加Host GameFrame构造临时绘制RNG，属于正面展示依赖，不是对象_rng的游标（4B；已核） | 0x14 i32 var_data【权威】Host格子玩法附加值；伤害/奖励与TrapDoor Update等消费。Client接收最终Sprite、状态及成员变化，不重演奖励或开关逻辑（4B；已核） | 0x18 bool initialized【权威】Host派生Init正常完成标志，可用于Host读取资格；不表示Client工厂已构造/资源已准备，不将来源字节写入Client充当本地资格（1B；已核） | 0x19 bytes[3] unknown_19_1b【未知】对象+1D—+1F；SDK字段间三字节，公共构造未赋予语义。即使保存时整段复制也不能证明填充，不读取或传输（3B；未知） | 0x1C u32 variable_seed【权威】Host玩法随机后保存的种子；不冒充Room Light实际使用的SpawnSeed，也不作为Client新建玩法RNG的输入（4B；已核） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 16—16B，不含第 9 步记录头）：

块 0：GD0 描述的实际展示输入（world，16—16B）：

| +0x0 i32 type：真实Grid类型；工厂选类及Room绘制pass/light的分支输入，变化走结构事务 | +0x4 i32 variant：真实变体；Decoration隐藏、Poop绘制、Rock碎块选择及门/压力板光照消费，不自行触发Init或选动画 | +0x8 i32 state：当前状态原值；Rock/Fire绘制与Poop/Rock水面裁剪、Room下层pass及光照的真实输入，不是Client玩法状态机 | +0xC u32 spawn_seed：来源种子原值；Room Light以该值加Host GameFrame构造临时绘制RNG，属于正面展示依赖，不是对象_rng的游标 |
|---|---|---|---|

证据：

- `type`：J273 VA0x704FD0初始化对象+4，0x706636写请求type；Room::RenderGridLight 0x801C24读对象+4；grid-common-evidence.md
- `variant`：J273 ctor 0x704FD7；Render 0x973190/0x710130/0x7146E0/0x95E140及Light 0x801BE0读对象+8；grid-common-evidence.md
- `state`：J273 ctor 0x704FDE；Render 0x7146E0/0x67B620，WaterClip 0x711620/0x718650，Light 0x801BE0读对象+C；grid-common-evidence.md
- `spawn_count`：J273 ctor 0x704FE5；Init 0x70508D递增对象+10；Fire Init 0x67A45F检查<=1；grid-common-evidence.md
- `spawn_seed`：J273 ctor 0x704FEC；Init 0x705079；Light 0x801BFF读对象+14，0x801C07读Game+25A70、0x801C0D相加，TrapDoor分支消费临时RNG；grid-common-evidence.md
- `var_data`：J273 ctor 0x704FF3；hurt_func 0x7055D6/0x7055EF读写对象+18；TrapDoor Update 0x71A7B0等；grid-common-evidence.md
- `initialized`：J273 ctor 0x704FFA写对象+1C=0，Init 0x705090在派生InitSubclass返回后写1；client_grid_native.h的来源/构造分离规则；grid-common-evidence.md
- `unknown_19_1b`：GridEntityDesc.zhl的bool+18/VariableSeed+1C间隙；J273 ctor 0x704FFA仅写1B、0x704FFE从对象+20写4B；grid-common-evidence.md
- `variable_seed`：J273 ctor 0x704FFE；hurt_func 0x7056F1等将对象_rng结果写入对象+20；已核绘制/水面/light不消费本字段；grid-common-evidence.md

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/observation-expansion/grid-catalog.json:46 | GridEntityDesc.Initialized | null / "boolean" | authority → ；198.initialized（相对+18／对象+1C）→ Host | 来源Init见证，Client以自己的工厂/资源owner建立资格。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行46原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:47 | GridEntityDesc.SpawnCount | null / "int" | authority → ；198.spawn_count（相对+C／对象+10）→ Host | Host Init次数及其随机分支，不是Client最终显示值。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行47原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:48 | GridEntityDesc.SpawnSeed | null / "int" | field → spawn_seed；198.spawn_seed（相对+10／对象+14，G0）；PressurePlate/Door用224排除 | 通用198保留实际Light临时RNG输入；224在PressurePlate/Door无随机Light消费者，留Host；不得要求全类SpawnSeed原值相等。 Light以种子加同world的Host GameFrame生成临时绘制随机，不发送对象_rng。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行48原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:49 | GridEntityDesc.State | null / "int" | field → state；198/224.state（相对+8／对象+C，G0） | Grid绘制、水面及光照的直接输入。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行49原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:50 | GridEntityDesc.Type | null / "GridEntityType" | field → type；198/224.type（相对+0／对象+4，G0） | 结构类型和Room绘制分支的直接输入。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行50原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:51 | GridEntityDesc.VarData | null / "int" | authority → ；198.var_data（相对+14／对象+18）→ Host | Host伤害/奖励/Update状态；旧观测原值相等不能恢复同步。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行51原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:52 | GridEntityDesc.VariableSeed | null / "int" | authority → ；198.variable_seed（相对+1C／对象+20）→ Host | Host玩法随机保存种子，区别于有Light消费者的SpawnSeed。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行52原归属=ReplicatedState，新处置不回写旧事实。 |
| docs/design/observation-expansion/grid-catalog.json:53 | GridEntityDesc.Variant | null / "int" | field → variant；198/224.variant（相对+4／对象+8，G0） | Grid绘制及光照的直接输入，不因已有最终Sprite而删除。 证据：grid-common-evidence.md / grid-common-synchronization-review.md；旧应用行53原归属=ReplicatedState，新处置不回写旧事实。 |
