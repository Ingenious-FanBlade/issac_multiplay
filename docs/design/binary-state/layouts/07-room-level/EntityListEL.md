# EntityListEL

由唯一布局源生成。布局编号 255；布局版本 87；原生大小 0x10；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 bool sublist【本地】是否借用子列表的内存所有权条件；同步决定：不发；本地独立owner维护（1B；SDK声明） | 0x1 bytes[3] unknown_1_3【未知】本轮未解释；包括未核字段／可能对齐，不能称填充；同步决定：不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充（3B；未知） | 0x4 Entity** data【本地】EL本地数组管理；其中顺序语义由正式Entity关系承接；同步决定：不上线原生头；保留真实render tie顺序，不按地址排序（4B；SDK声明） | 0x8 u32 capacity【本地】EL本地数组管理；其中顺序语义由正式Entity关系承接；同步决定：不上线原生头；保留真实render tie顺序，不按地址排序（4B；SDK声明） | 0xC u32 size【本地】EL本地数组管理；其中顺序语义由正式Entity关系承接；同步决定：不上线原生头；保留真实render tie顺序，不按地址排序（4B；SDK声明） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `sublist`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort
- `unknown_1_3`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort；本轮补读：SDK bool+0与ptr+4；构造／Destroy／QueryType只按已核成员访问，+1..3仍未知
- `data`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort；原账本标签：SDK声明＋既有应用
- `capacity`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort；原账本标签：SDK声明＋既有应用
- `size`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort；原账本标签：SDK声明＋既有应用
