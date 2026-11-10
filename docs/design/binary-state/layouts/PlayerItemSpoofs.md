# PlayerItemSpoofs

由唯一布局源生成。布局编号 173；布局版本 56；原生大小 0x5C；未知 0B；完整：是。

原生字节归属：

| 0x0 void* collectible[0].vptr【本地】CollectibleSpoof实际对象：实际CollectibleSpoof／TrinketSpoof虚表，不传地址（4B；已核） | 0x4 unordered_set<i32> collectible[0].blocked【容器】CollectibleSpoof实际对象：实际blocked逻辑ID集合；头／桶／迭代链本地；排序、唯一完整ID；遍历预算16384；不截断（32B；已核） | 0x24 map<string,InnateItems> collectible[0].innate_groups【容器】CollectibleSpoof实际对象：仅提取全部组persistent与temporary.cachedTotals的有效合计；组名与时限不上线；有界遍历所有组persistent+cachedTotals，用宽整数合并，拒绝负项/溢出，按ID排序非零计数；不读timer/group字符串；Client一个本地展示组persistent承载，temporary空；这是多组聚合结果，无一对一原生元素步长，stride=0；8B是自有ID/count值，禁止按它遍历map节点。（8B；已核） | 0x2C void* trinket[0].vptr【本地】TrinketSpoof实际对象，保留黄金ID语义：实际CollectibleSpoof／TrinketSpoof虚表，不传地址（4B；已核） | 0x30 unordered_set<i32> trinket[0].blocked【容器】TrinketSpoof实际对象，保留黄金ID语义：实际blocked逻辑ID集合；头／桶／迭代链本地；排序、唯一完整ID；遍历预算16384；不截断（32B；已核） | 0x50 map<string,InnateItems> trinket[0].innate_groups【容器】TrinketSpoof实际对象，保留黄金ID语义：仅提取全部组persistent与temporary.cachedTotals的有效合计；组名与时限不上线；有界遍历所有组persistent+cachedTotals，用宽整数合并，拒绝负项/溢出，按ID排序非零计数；不读timer/group字符串；Client一个本地展示组persistent承载，temporary空；这是多组聚合结果，无一对一原生元素步长，stride=0；8B是自有ID/count值，禁止按它遍历map节点。（8B；已核） | 0x58 i32 last_player_type【权威】触发类型变更处理的缓存，不参与有效计数查询（4B；已核） |
|---|---|---|---|---|---|---|

状态二进制布局（总计 8—393224B，不含第 9 步记录头）：

块 0：展示值（world，8—393224B）：

| +0x0 ItemSpoof（见该类型状态表） collectible：CollectibleSpoof实际对象 | 顺序变长偏移 ItemSpoof（见该类型状态表） trinket：TrinketSpoof实际对象，保留黄金ID语义 |
|---|---|

证据：

- `collectible[0].vptr`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `collectible[0].blocked`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `collectible[0].innate_groups`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `trinket[0].vptr`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `trinket[0].blocked`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `trinket[0].innate_groups`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `last_player_type`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
