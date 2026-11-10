# RoomWaveResult

由唯一布局源生成。布局编号 289；布局版本 58；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—33B，不含第 9 步记录头）：

块 0：RoomWaveResult当前结果（world，1—33B）：

| +0x0 u8（C/S自有元数据，无原生偏移） active_mask：两波纹当前有效位；不是未来推进状态 | +0x1 f32[2]（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_world_center：世界中心，本端视口换算 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_radius_used：包含有效半帧的当前radius | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit0(active_mask)成立时存在） a_gain_used：当前包络／gain | 顺序变长偏移 f32[2]（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_world_center：世界中心，本端视口换算 | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_radius_used：包含有效半帧的当前radius | 顺序变长偏移 f32（C/S自有元数据，无原生偏移）（仅room_wave_bit1(active_mask)成立时存在） b_gain_used：当前包络／gain |
|---|---|---|---|---|---|---|

证据：


