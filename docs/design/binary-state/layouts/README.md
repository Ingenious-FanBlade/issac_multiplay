# 按总计划步骤归类的生成布局索引

上级：[二进制重构总计划](../README.md#7-步骤)。按总计划步骤号阅读，步骤号与布局编号分别管理；各步骤内按布局编号排列。

布局内容只修改 `mod/binding/state/`；目录与索引由 `mod/tools/gen_state_layout.py` 同源生成。未知和未核声明照实保留，不能据本表宣称原生同步已可用。

以布局源所属步骤归类，变体与附属类型随所属域；共享类型只保存一份。第7步已登记的显示／资源结构仍随第7步，后续协议与运行职责见对应计划，不以目录位置宣称已实现。

第0—1步负责起点与生成机制，没有单独类型表；第9步登记公共报文固定头与常量，第10—15步的运行接线和验收仍见总计划。

| 总计划步骤 | 布局表数 | 公开目录表数 |
|---|---|---|
| [2：基础值类型与嵌套规则](02-base-types/README.md) | 16 | 0 |
| [3：Entity 公共与主 Sprite](03-entity-common/README.md) | 2 | 0 |
| [4：Entity 子类](04-entity-classes/README.md) | 47 | 0 |
| [5：EntityPlayer](05-player/README.md) | 48 | 0 |
| [6：GridEntity](06-grid/README.md) | 32 | 1 |
| [7：Room、Level、RoomSave 与 Exploration](07-room-level/README.md) | 72 | 4 |
| [8：Run、Presentation 与 Random](08-run-presentation/README.md) | 129 | 3 |
| [9：公共报文结构](09-wire-format/README.md) | 25 | 0 |

旧证据路径仅保留 [EntitySaveState](EntitySaveState.md) 和 [GridEntityDesc](GridEntityDesc.md) 两个跳转页，正式表见对应步骤目录。

[需求清单覆盖与收缩统计](demand-coverage.md)；UNKNOWN仅作原生字节参照，不阻塞需求覆盖。
