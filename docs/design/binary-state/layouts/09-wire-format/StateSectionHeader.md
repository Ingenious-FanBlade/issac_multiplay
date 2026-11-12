# StateSectionHeader

由唯一布局源生成。布局编号 410；布局版本 71；原生大小 0x0；未知 0B；完整：是。

52B；其后顺序为记录、结果组、事件。 本表仅定义固定封装字节；不再包12B状态记录头，也不表示完整消息编解码已实现。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

公共封装布局（总计 52—52B，不再附加12B状态记录头）：

固定封装（52—52B；块表中的world仅为生成占位，不限制承运通道或所属序列）：

| +0x0 u8（C/S自有元数据，无原生偏移） stream：0 world、1按Peer统一body序列 | +0x1 u8（C/S自有元数据，无原生偏移） flags：bit0 baseline，当且仅当base_version为0 | +0x2 u32（C/S自有元数据，无原生偏移） member_epoch：引用成员表的精确快照版本 | +0x6 u32（C/S自有元数据，无原生偏移） resource_epoch：引用资源表的精确快照版本 | +0xA u32（C/S自有元数据，无原生偏移） base_version：本Peer本序列已确认继承起点，0完整重建 | +0xE u32（C/S自有元数据，无原生偏移） target_version：本Peer本序列目标，单调非零，不回绕 | +0x12 u64（C/S自有元数据，无原生偏移） source_revision：Host封存的本序列来源修订，先于Peer目标版本存在 | +0x1A u64（C/S自有元数据，无原生偏移） presentation_batch：配对显示批次身份；不是第三条确认序列 | +0x22 u32（C/S自有元数据，无原生偏移） game_frame：Host原生GameFrame位模式，不能代替来源身份 | +0x26 u16（C/S自有元数据，无原生偏移） record_count：后随12B头的成员记录数 | +0x28 u16（C/S自有元数据，无原生偏移） group_count：后随具名结果组数 | +0x2A u16（C/S自有元数据，无原生偏移） event_count：后随音频事件数；body必须为0 | +0x2C u64（C/S自有元数据，无原生偏移） event_watermark：world目标覆盖的连续事件水位；body必须为0 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


