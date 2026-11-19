# MusicRequestState

由唯一布局源生成。布局编号 439；布局版本 87；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤80B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 57—57B，不含第 9 步记录头）：

块 0：Host显示需求纯值（world，57—57B）：

| +0x0 u64（C/S自有元数据，无原生偏移） revision：请求状态版本 | +0x8 u64（C/S自有元数据，无原生偏移） generation：播放代次，同曲新代次仍重播 | +0x10 u8（C/S自有元数据，无原生偏移） mode：0静止／1Play／2Crossfade／3Fadein／4Fadeout／5Queue自然接续（本地已接续不重播，恢复时按本机音量以0.08淡入） | +0x11 i32（C/S自有元数据，无原生偏移） id：当前请求id | +0x15 f32（C/S自有元数据，无原生偏移） parameter：Play音量或Crossfade速率 | +0x19 f32（C/S自有元数据，无原生偏移） fade_rate：Fadein／Fadeout参数 | +0x1D i32（C/S自有元数据，无原生偏移） queued：当前排队id，0明确为空 | +0x21 f32（C/S自有元数据，无原生偏移） pitch：原生当前pitch | +0x25 f32（C/S自有元数据，无原生偏移） target_pitch：原生目标pitch | +0x29 u8（C/S自有元数据，无原生偏移） paused：显式暂停请求 | +0x2A u8（C/S自有元数据，无原生偏移） enabled：音乐enabled | +0x2B u8（C/S自有元数据，无原生偏移） layers：已核六层当前启用mask | +0x2C i32（C/S自有元数据，无原生偏移） jingle_id：当前jingle请求 | +0x30 u64（C/S自有元数据，无原生偏移） jingle_generation：同jingle新代次 | +0x38 u8（C/S自有元数据，无原生偏移） jingle_active：jingle当前逻辑状态 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


