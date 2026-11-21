# ItemSpoof

由唯一布局源生成。布局编号 170；布局版本 90；原生大小 0x2C；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 void* vptr【本地】实际CollectibleSpoof／TrinketSpoof虚表，不传地址（4B；已核） | 0x4 unordered_set<i32> blocked【容器】实际blocked逻辑ID集合；头／桶／迭代链本地；排序、唯一完整ID；遍历预算16384；不截断（32B；已核） | 0x24 map<string,InnateItems> innate_groups【容器】仅提取全部组persistent与temporary.cachedTotals的有效合计；组名与时限不上线；有界遍历所有组persistent+cachedTotals，用宽整数合并，拒绝负项/溢出，按ID排序非零计数；不读timer/group字符串；Client一个本地展示组persistent承载，temporary空；这是多组聚合结果，无一对一原生元素步长，stride=0；8B是自有ID/count值，禁止按它遍历map节点。（8B；已核） |
|---|---|---|

状态二进制布局（总计 4—196612B，不含第 9 步记录头）：

块 0：展示值（world，4—196612B）：

| +0x0 u16 count + count×i32（上限16384） blocked：实际blocked逻辑ID集合；头／桶／迭代链本地；排序、唯一完整ID；遍历预算16384；不截断 | 顺序变长偏移 u16 count + count×PlayerInnateCount（上限16384） innate_groups：仅提取全部组persistent与temporary.cachedTotals的有效合计；组名与时限不上线；有界遍历所有组persistent+cachedTotals，用宽整数合并，拒绝负项/溢出，按ID排序非零计数；不读timer/group字符串；Client一个本地展示组persistent承载，temporary空；这是多组聚合结果，无一对一原生元素步长，stride=0；8B是自有ID/count值，禁止按它遍历map节点。 |
|---|---|

证据：

- `vptr`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `blocked`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `innate_groups`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
