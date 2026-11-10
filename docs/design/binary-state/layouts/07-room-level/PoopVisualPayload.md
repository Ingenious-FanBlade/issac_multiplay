# PoopVisualPayload

由唯一布局源生成。布局编号 292；布局版本 57；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 52—163892B，不含第 9 步记录头）：

块 0：PoopVisualPayload当前结果（world，52—163892B）：

| +0x0 f32（C/S自有元数据，无原生偏移） alpha：96CE20由countdown产生的当前alpha；不在Client递减 | +0x4 u16（C/S自有元数据，无原生偏移） image：image-table当前实际资源 | +0x6 f32[11]（C/S自有元数据，无原生偏移） color：当前完整ColorMod；不是RGBA裁剪 | +0x32 u16 count + count×FXPose（上限16384）（C/S自有元数据，无原生偏移） poses：当前有效prefix，不发送生成速度／过滤 |
|---|---|---|---|

证据：


