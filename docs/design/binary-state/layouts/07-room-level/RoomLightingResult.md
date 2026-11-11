# RoomLightingResult

由唯一布局源生成。布局编号 288；布局版本 60；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—46B，不含第 9 步记录头）：

块 0：RoomLightingResult当前结果（world，1—46B）：

| +0x0 u8（C/S自有元数据，无原生偏移） mask：bit0 shadow_fill/bit1 light_tint/bit2 alpha/bit3特殊背景；上位零 | +0x1 KColor（见该类型状态表）（伴随源，无根对象偏移）（仅room_lighting_bit0(mask)成立时存在） shadow_fill：803E20正常产生的world最终颜色 | 顺序变长偏移 KColor（见该类型状态表）（伴随源，无根对象偏移）（仅room_lighting_bit1(mask)成立时存在） light_tint：803E20正常产生的world最终颜色 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_lighting_bit2(mask)成立时存在） lighting_alpha：804500当前最终强度，不调用8134C0查询补值 | 顺序变长偏移 u8（C/S自有元数据，无原生偏移）（仅room_lighting_bit3(mask)成立时存在） special_background：当前特殊背景具名kind；具体枚举与pass映射仍须第9/13步登记，未知值不能默认为普通；固定值布局不等于已有分派。 |
|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `shadow_fill` → `KColor`：803E20正常产生的world最终颜色；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
- `light_tint` → `KColor`：803E20正常产生的world最终颜色；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:2928 | Room.GetLightingAlpha | " " / "float" | derived → lighting_alpha；RoomLightingResult.lighting_alpha | 发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖；原stream行2928归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
