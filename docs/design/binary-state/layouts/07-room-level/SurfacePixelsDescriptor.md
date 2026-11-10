# SurfacePixelsDescriptor

由唯一布局源生成。布局编号 305；布局版本 57；原生大小 0x0；未知 0B；完整：是。

16B描述；不是GL handle，不自动量化特殊格式。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 16—16B，不含第 9 步记录头）：

块 0：SurfacePixelsDescriptor当前结果（world，16—16B）：

| +0x0 u16（C/S自有元数据，无原生偏移） logical_w：逻辑宽 | +0x2 u16（C/S自有元数据，无原生偏移） logical_h：逻辑高 | +0x4 u16（C/S自有元数据，无原生偏移） stored_w：实际存储宽 | +0x6 u16（C/S自有元数据，无原生偏移） stored_h：实际存储高 | +0x8 u8（C/S自有元数据，无原生偏移） format：0 RGBA8字节 | +0x9 u8（C/S自有元数据，无原生偏移） origin：0 GL下起 | +0xA u8（C/S自有元数据，无原生偏移） alpha_encoding：0保留原生编码 | +0xB u8（C/S自有元数据，无原生偏移） color_encoding：0保留原生编码 | +0xC u32（C/S自有元数据，无原生偏移） content_bytes：恰好stored_w*stored_h*4，逻辑尺寸不得超过存储，最大16MiB |
|---|---|---|---|---|---|---|---|---|

证据：


