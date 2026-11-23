# RoomLightingResult

由唯一布局源生成。布局编号 288；布局版本 94；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：RoomLightingResult当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:2928 | Room.GetLightingAlpha | " " / "float" | derived → lighting_alpha；RoomLightingResult.lighting_alpha | 发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖；原stream行2928归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
