# StateAudioEventHeader

由唯一布局源生成。布局编号 417；布局版本 87；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

40B；只有kind3 Clear允许非零clear_scope，其余事件必须精确voice。 本表仅定义固定封装字节；不再包12B状态记录头，也不表示完整消息编解码已实现。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

公共封装布局（总计 40—40B，不再附加12B状态记录头）：

固定封装（40—40B；块表中的world仅为生成占位，不限制承运通道或所属序列）：

| +0x0 u64（C/S自有元数据，无原生偏移） event_sequence：同session/run稳定事件号，重复发送不重新分配 | +0x8 u32（C/S自有元数据，无原生偏移） logical_frame：原生逻辑时点，非硬件游标 | +0xC u16（C/S自有元数据，无原生偏移） owner_member：voice所属owner成员；FFFF为world/run根 | +0xE u16（C/S自有元数据，无原生偏移） purpose：已登记音频用途，不允许隐式全局Clear | +0x10 u64（C/S自有元数据，无原生偏移） voice_id：稳定voice实例，非SoundID或后端handle | +0x18 u64（C/S自有元数据，无原生偏移） voice_generation：voice代次 | +0x20 u8（C/S自有元数据，无原生偏移） clear_scope：0精确voice、1owner用途、2当前world用途、3当前run用途 | +0x21 u8（C/S自有元数据，无原生偏移） flags：当前必须0 | +0x22 u16（C/S自有元数据，无原生偏移） layout：必须383 AudioWorldEvent | +0x24 u32（C/S自有元数据，无原生偏移） payload_bytes：后随既有AudioWorldEvent精确长度 |
|---|---|---|---|---|---|---|---|---|---|

证据：


