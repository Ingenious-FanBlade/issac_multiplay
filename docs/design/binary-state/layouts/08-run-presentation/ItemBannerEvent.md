# ItemBannerEvent

由唯一布局源生成。布局编号 449；布局版本 87；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤16400B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 13—16397B，不含第 9 步记录头）：

块 0：Host显示需求纯值（world，13—16397B）：

| +0x0 u16（C/S自有元数据，无原生偏移） player：稳定Player身份 | +0x2 u16（C/S自有元数据，无原生偏移） configuration：typed item配置；custom为Null | +0x4 u8（C/S自有元数据，无原生偏移） style：0 typed／1 custom | +0x5 u16 count + count×u8（上限4096）（C/S自有元数据，无原生偏移） title_key：原始本地化键／类别由Items固定 | 顺序变长偏移 u16 count + count×u8（上限4096）（C/S自有元数据，无原生偏移） subtitle_key：原始本地化键 | 顺序变长偏移 u16 count + count×u8（上限4096）（C/S自有元数据，无原生偏移） title：实际UTF8后备文本 | 顺序变长偏移 u16 count + count×u8（上限4096）（C/S自有元数据，无原生偏移） subtitle：实际UTF8后备文本 |
|---|---|---|---|---|---|---|

证据：


