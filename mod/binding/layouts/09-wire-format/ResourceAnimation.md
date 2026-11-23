# ResourceAnimation

由唯一布局源生成。布局编号 428；布局版本 94；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本63资源描述；无文件内容／纹理像素／PCM／指针，表到达不等于本地物化ready。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

公共封装布局（总计 12—5132B，不再附加12B状态记录头）：

固定封装（12—5132B；块表中的world仅为生成占位，不限制承运通道或所属序列）：

| +0x0 u16（C/S自有元数据，无原生偏移） file：file | +0x2 ResourceName（见该类型状态表）（C/S自有元数据，无原生偏移） name：name | 顺序变长偏移 u32（C/S自有元数据，无原生偏移） length：length | 顺序变长偏移 u16 count + count×ResourceAnimationLayer（上限256）（C/S自有元数据，无原生偏移） layers：layers | 顺序变长偏移 u16 count + count×ResourceAnimationLayer（上限256）（C/S自有元数据，无原生偏移） null_layers：null_layers |
|---|---|---|---|---|

证据：


