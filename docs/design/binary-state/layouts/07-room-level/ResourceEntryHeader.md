# ResourceEntryHeader

由唯一布局源生成。布局编号 304；布局版本 60；原生大小 0x0；未知 0B；完整：是。

第9步公共合同将旧7B头改为8B：table/index/kind/payload_bytes。布局304保留；完整typed payload和允许集尚待本步后续，不代表资源可接受报文已实现。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 8—8B，不含第 9 步记录头）：

块 0：ResourceEntryHeader当前结果（world，8—8B）：

| +0x0 u8（C/S自有元数据，无原生偏移） table：显式资源表空间0—7；与u16 index共同定位，不因整数相等跨表替换 | +0x1 u16（C/S自有元数据，无原生偏移） index：不可变会话资源项，不得FFFF | +0x3 u8（C/S自有元数据，无原生偏移） kind：公共resource_kind编号0—9；具体table/kind允许集及完整payload待第9步后续登记 | +0x4 u32（C/S自有元数据，无原生偏移） payload_bytes：后续typed payload精确长度 |
|---|---|---|---|

证据：


