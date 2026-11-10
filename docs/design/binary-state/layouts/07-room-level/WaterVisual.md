# WaterVisual

由唯一布局源生成。布局编号 287；布局版本 57；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—48B，不含第 9 步记录头）：

块 0：WaterVisual当前结果（world，1—48B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：明确世界分支存在性；缺源是Gap | +0x1 WaterVisualPayload（见该类型状态表）（伴随源，无根对象偏移）（仅watervisual_present_present(present)成立时存在） payload：水图像／phase／flow／shader同批提交 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `payload` → `WaterVisualPayload`：水图像／phase／flow／shader同批提交；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
