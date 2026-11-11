# ResourceBlobChunkHeader

由唯一布局源生成。布局编号 306；布局版本 60；原生大小 0x0；未知 0B；完整：是。

第9步公共合同将旧14B头改为15B：resource_epoch/table/index/offset/bytes；完整范围、内容冲突、资源就绪和总预算仍须后续编解码执行。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 15—15B，不含第 9 步记录头）：

块 0：ResourceBlobChunkHeader当前结果（world，15—15B）：

| +0x0 u32（C/S自有元数据，无原生偏移） resource_epoch：可靠资源代次 | +0x4 u8（C/S自有元数据，无原生偏移） table：显式资源表空间0—7；与u16 index共同定位，不因整数相等跨表替换 | +0x5 u16（C/S自有元数据，无原生偏移） index：实际资源项 | +0x7 u32（C/S自有元数据，无原生偏移） offset：完整blob字节偏移 | +0xB u32（C/S自有元数据，无原生偏移） bytes：本块实际后随长度 |
|---|---|---|---|---|

证据：


