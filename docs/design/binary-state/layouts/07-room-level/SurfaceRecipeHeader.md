# SurfaceRecipeHeader

由唯一布局源生成。布局编号 307；布局版本 58；原生大小 0x0；未知 0B；完整：是。

16B固定头，完整有序操作及GPU状态保护归第9/10/13步；没有建立任意命令解释器。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 16—16B，不含第 9 步记录头）：

块 0：SurfaceRecipeHeader当前结果（world，16—16B）：

| +0x0 u16（C/S自有元数据，无原生偏移） recipe_layout：只接受登记的recipe layout，不能由报文给偏移／函数 | +0x2 u16（C/S自有元数据，无原生偏移） logical_w：逻辑宽 | +0x4 u16（C/S自有元数据，无原生偏移） logical_h：逻辑高 | +0x6 u16（C/S自有元数据，无原生偏移） stored_w：存储宽 | +0x8 u16（C/S自有元数据，无原生偏移） stored_h：存储高 | +0xA u8（C/S自有元数据，无原生偏移） format：RGBA8 | +0xB u8（C/S自有元数据，无原生偏移） origin：GL下起 | +0xC u8（C/S自有元数据，无原生偏移） alpha_encoding：原生编码 | +0xD u8（C/S自有元数据，无原生偏移） color_encoding：原生编码 | +0xE u16（C/S自有元数据，无原生偏移） operation_count：有序操作数量 |
|---|---|---|---|---|---|---|---|---|---|

证据：


