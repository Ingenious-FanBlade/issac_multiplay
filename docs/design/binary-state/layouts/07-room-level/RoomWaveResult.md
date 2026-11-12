# RoomWaveResult

由唯一布局源生成。布局编号 289；布局版本 71；原生大小 0x0；未知 0B；完整：是。

水面／波纹纯表现由Client本地推进；Host下发视觉锚点及必要外观，不捕获每次Render scratch，不要求各端逐帧画面相等。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—57B，不含第 9 步记录头）：

块 0：RoomWaveResult当前结果（world，1—57B）：

| +0x0 u8（C/S自有元数据，无原生偏移） active_mask：两波纹当前有效位；不是未来推进状态 | +0x1 f32[2]（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_world_center：世界中心，由本端转换视口 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_radius_used：当前显示半径锚点，含已采用半帧 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_strength：显示包络的原始强度，Client本地计算1-pow(elapsed/duration,0.4f) | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_radius_step：每个原生显示计时帧的半径增量，只供本地显示 | 顺序变长偏移 u32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_elapsed：同批视觉计时锚点；不成为Client玩法计时器 | 顺序变长偏移 u32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_duration：本次视觉持续帧数，必须大于elapsed | 顺序变长偏移 f32[2]（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_world_center：世界中心，由本端转换视口 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_radius_used：当前显示半径锚点，含已采用半帧 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_strength：显示包络的原始强度，Client本地计算1-pow(elapsed/duration,0.4f) | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_radius_step：每个原生显示计时帧的半径增量，只供本地显示 | 顺序变长偏移 u32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_elapsed：同批视觉计时锚点；不成为Client玩法计时器 | 顺序变长偏移 u32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_duration：本次视觉持续帧数，必须大于elapsed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


