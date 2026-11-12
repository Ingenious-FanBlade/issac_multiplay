# FXScenePayload

由唯一布局源生成。布局编号 296；布局版本 71；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 29—26419580B，不含第 9 步记录头）：

块 0：FXScenePayload当前结果（world，29—26419580B）：

| +0x0 f32[2]（C/S自有元数据，无原生偏移） average_player_pos：同world显示输入，不从本地单身体重算 | +0x8 f32[4]（C/S自有元数据，无原生偏移） bounds：FX显示范围 | +0x18 u16 count + count×FX（上限4096）（C/S自有元数据，无原生偏移） layers：全部当前world可显示有序层，不按Host视口裁剪 | 顺序变长偏移 SelectedRayGroup（见该类型状态表）（伴随源，无根对象偏移） rays：同world具名rays结果 | 顺序变长偏移 PoopVisual（见该类型状态表）（伴随源，无根对象偏移） poop：同world具名poop结果 | 顺序变长偏移 VeinGraphVisual（见该类型状态表）（伴随源，无根对象偏移） veins：同world具名veins结果 |
|---|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `rays` → `SelectedRayGroup`：同world具名rays结果；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
- `poop` → `PoopVisual`：同world具名poop结果；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
- `veins` → `VeinGraphVisual`：同world具名veins结果；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
