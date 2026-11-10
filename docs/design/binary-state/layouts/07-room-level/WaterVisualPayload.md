# WaterVisualPayload

由唯一布局源生成。布局编号 286；布局版本 58；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 47—47B，不含第 9 步记录头）：

块 0：WaterVisualPayload当前结果（world，47—47B）：

| +0x0 u8（C/S自有元数据，无原生偏移） mode：经典／反射世界模式，独立于本地图形选项 | +0x1 u16（C/S自有元数据，无原生偏移） image：实际water用途image-table资源引用 | +0x3 f32（C/S自有元数据，无原生偏移） phase_used：91AC70实际采用的有符号phase；816BB0的2*GameFrame另从同批帧头派生 | +0x7 f32[2]（C/S自有元数据，无原生偏移） flow：Room+7460同批真实水流 | +0xF WaterShaderResult（见该类型状态表）（伴随源，无根对象偏移） shader：正常816BB0最终32B水色结果；缺源不折空 |
|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `shader` → `WaterShaderResult`：正常816BB0最终32B水色结果；缺源不折空；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:2955 | Room.GetWaterCurrent | " " / "Vector" | derived → flow；WaterVisualPayload.flow | 保留具体显示消费者所需当前值；Getter本身不作为正式reader；原stream行2955归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
