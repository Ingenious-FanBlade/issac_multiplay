# VeinGraphPayload

由唯一布局源生成。布局编号 294；布局版本 58；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 193—17826063B，不含第 9 步记录头）：

块 0：VeinGraphPayload当前结果（world，193—17826063B）：

| +0x0 u8[10]（C/S自有元数据，无原生偏移） roots：原十根到规范节点目录映射，FF为空；非空必须指向本图节点 | +0xA u8 count + count×Vein（上限10）（C/S自有元数据，无原生偏移） nodes：只读实际可达节点，按原十槽顺序建立稳定有界目录；深度／循环／owner错误报Gap | 顺序变长偏移 RoomVeinSprite（见该类型状态表）（伴随源，无根对象偏移） sprite0：实际资源／overlay／层；scale/rotation/color由节点覆写 | 顺序变长偏移 RoomVeinSprite（见该类型状态表）（伴随源，无根对象偏移） sprite1：实际资源／overlay／层；scale/rotation/color由节点覆写 |
|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `sprite0` → `RoomVeinSprite`：实际资源／overlay／层；scale/rotation/color由节点覆写；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
- `sprite1` → `RoomVeinSprite`：实际资源／overlay／层；scale/rotation/color由节点覆写；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
