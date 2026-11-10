# VeinGraphVisual

由唯一布局源生成。布局编号 295；布局版本 57；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—17826064B，不含第 9 步记录头）：

块 0：VeinGraphVisual当前结果（world，1—17826064B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：96EA50首根child表示当前图资格；无图不读根标量 | +0x1 VeinGraphPayload（见该类型状态表）（伴随源，无根对象偏移）（仅veingraphvisual_present_present(present)成立时存在） payload：十槽有界图与两个Sprite同批 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `payload` → `VeinGraphPayload`：十槽有界图与两个Sprite同批；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
