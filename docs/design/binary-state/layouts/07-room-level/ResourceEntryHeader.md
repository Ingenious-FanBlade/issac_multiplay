# ResourceEntryHeader

由唯一布局源生成。布局编号 304；布局版本 58；原生大小 0x0；未知 0B；完整：是。

只登记7B固定头；payload消费与依赖闭包归第9步，不是完整资源记录。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 7—7B，不含第 9 步记录头）：

块 0：ResourceEntryHeader当前结果（world，7—7B）：

| +0x0 u16（C/S自有元数据，无原生偏移） index：不可变会话资源项，不得FFFF | +0x2 u8（C/S自有元数据，无原生偏移） kind：0 ANM2/1文件图像/2内置shader/3配置/4recipe/5像素 | +0x3 u32（C/S自有元数据，无原生偏移） payload_bytes：后续typed payload精确长度 |
|---|---|---|

证据：


