# EntitySaveStateStrongPair

由唯一布局源生成。布局编号 175；布局版本 94；原生大小 0x8；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

仅8B强引用pair；控制块和递归实体保存体按真实owner持有，不能展开成固定网络图。

原生字节归属：

| 0x0 EntitySaveState* save_state【本地】本地可递归保存体所有权，不是C/S实体StateRef（4B；已核） | 0x4 KAGE_ReferenceCount* counter【本地】本地控制块／引用计数，禁止下行（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `save_state`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `counter`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SmartPointer.zhl
