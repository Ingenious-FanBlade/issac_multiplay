# InnateItems

由唯一布局源生成。布局编号 171；布局版本 87；原生大小 0x4C；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 unordered_map<i32,i32> persistent【权威】每组持久计数参与有效合计；原分组管理不上线（32B；已核） | 0x20 vector<TemporaryInnateItem> temporary_list【权威】原始临时条目／顺序／到期timer留Host，不能当当前count源（12B；已核） | 0x2C unordered_map<i32,i32> temporary_totals【权威】官方查询实际使用的临时有效计数缓存（32B；已核） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `persistent`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `temporary_list`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
- `temporary_totals`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}
