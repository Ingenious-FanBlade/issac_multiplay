# MapFocusPair

由唯一布局源生成。布局编号 301；布局版本 57；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 2—14B，不含第 9 步记录头）：

块 0：MapFocusPair当前结果（world，2—14B）：

| +0x0 MapFocusResult（见该类型状态表）（伴随源，无根对象偏移） small：small config实际选择结果 | 顺序变长偏移 MapFocusResult（见该类型状态表）（伴随源，无根对象偏移） large：large config实际选择结果；未证明相同不能共用 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `small` → `MapFocusResult`：small config实际选择结果；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
- `large` → `MapFocusResult`：large config实际选择结果；未证明相同不能共用；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
