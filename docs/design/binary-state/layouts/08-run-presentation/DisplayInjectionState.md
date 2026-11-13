# DisplayInjectionState

由唯一布局源生成。布局编号 438；布局版本 75；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤128B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 105—105B，不含第 9 步记录头）：

块 0：Host显示需求纯值（world，105—105B）：

| +0x0 f32（C/S自有元数据，无原生偏移） devil_total：原始Room总率，非HUD百分比 | +0x4 f32（C/S自有元数据，无原生偏移） angel_share：原生XMM0天使占比 | +0x8 u8（C/S自有元数据，无原生偏移） deal_qualified：原生楼层资格 | +0x9 f32（C/S自有元数据，无原生偏移） planetarium：官方原始星象房概率 | +0xD u8（C/S自有元数据，无原生偏移） progress_blocked：Host最终HUD进度查询结果 | +0xE i32[15]（C/S自有元数据，无原生偏移） pill_effects：ItemPool原始15槽，不是各Player最终效果 | +0x4A u8[15]（C/S自有元数据，无原生偏移） pill_identified：原始识别字节 | +0x59 f32[2]（C/S自有元数据，无原生偏移） water_current：Room水流 | +0x61 i32（C/S自有元数据，无原生偏移） map_focus：挑战32 Host选定的当前地图位置 | +0x65 i32（C/S自有元数据，无原生偏移） map_focus_large：挑战32大地图的实际SafeGridIndex；分别观察，不假定与小地图相同 |
|---|---|---|---|---|---|---|---|---|---|

证据：


