# PoopVisual

由唯一布局源生成。布局编号 293；布局版本 71；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—163893B，不含第 9 步记录头）：

块 0：PoopVisual当前结果（world，1—163893B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：当前poop显示资格，无分支时不读旧payload | +0x1 PoopVisualPayload（见该类型状态表）（伴随源，无根对象偏移）（仅poopvisual_present_present(present)成立时存在） payload：当前alpha、资源、颜色、有效pose同批 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `payload` → `PoopVisualPayload`：当前alpha、资源、颜色、有效pose同批；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
