# DisplayInjectionState

由唯一布局源生成。布局编号 438；布局版本 89；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤192B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 176—176B，不含第 9 步记录头）：

块 0：Host显示需求纯值（world，176—176B）：

| +0x0 f32（C/S自有元数据，无原生偏移） devil_total：原始Room总率，非HUD百分比 | +0x4 f32（C/S自有元数据，无原生偏移） angel_share：原生XMM0天使占比 | +0x8 u8（C/S自有元数据，无原生偏移） deal_qualified：原生楼层资格 | +0x9 f32（C/S自有元数据，无原生偏移） planetarium：官方原始星象房概率 | +0xD u8（C/S自有元数据，无原生偏移） progress_blocked：Host最终HUD进度查询结果 | +0xE i32[15]（C/S自有元数据，无原生偏移） pill_effects：ItemPool原始15槽，不是各Player最终效果 | +0x4A u8[15]（C/S自有元数据，无原生偏移） pill_identified：原始识别字节 | +0x59 f32[2]（C/S自有元数据，无原生偏移） water_current：Room水流 | +0x61 i32（C/S自有元数据，无原生偏移） map_focus：挑战32 Host选定的当前地图位置 | +0x65 i32（C/S自有元数据，无原生偏移） map_focus_large：挑战32大地图的实际SafeGridIndex；分别观察，不假定与小地图相同 | +0x69 i32（C/S自有元数据，无原生偏移） backdrop_wall_style：背景20墙面构图读取的档位 | +0x6D u8（C/S自有元数据，无原生偏移） backdrop_tutorial：本房满足教程的楼层和起始位置条件；按钮使用Client本机映射 | +0x6E u8（C/S自有元数据，无原生偏移） room_graphics_flags：bit0为原生游戏状态bit46，bit1为bit47。分别供FX背景筛选、特殊Boss房墙面和灯光消费。 | +0x6F i32（C/S自有元数据，无原生偏移） camera_entry_door：原生Camera构造时使用的进入门 | +0x73 i32（C/S自有元数据，无原生偏移） camera_previous_entry：原生Camera构造时判定特殊进房或返回所用前一入口 | +0x77 u8（C/S自有元数据，无原生偏移） camera_return_focus：当前入口等于地牢返回房时允许原生相机判断上一房类型并使用返回位置 | +0x78 f32[2]（C/S自有元数据，无原生偏移） camera_return_position：与当前房关联的原生返回位置；无关联时规范为零 | +0x80 i32（C/S自有元数据，无原生偏移） poop_fx_countdown：本房短时poop云的当前相位；Client按本地外观seed构建图像和pose | +0x84 f32[11]（C/S自有元数据，无原生偏移） poop_fx_color：有poop云时的当前ColorMod |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


