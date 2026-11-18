# StateResultGroupHeader

由唯一布局源生成。布局编号 414；布局版本 84；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

42B；subject的Key/use/shape从member_epoch解析，不重复编码。 本表仅定义固定封装字节；不再包12B状态记录头，也不表示完整消息编解码已实现。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

公共封装布局（总计 42—42B，不再附加12B状态记录头）：

固定封装（42—42B；块表中的world仅为生成占位，不限制承运通道或所属序列）：

| +0x0 u8（C/S自有元数据，无原生偏移） op：0完整替换、1显式引用合法不可变缓存、2退休 | +0x1 u8（C/S自有元数据，无原生偏移） flags：当前必须0 | +0x2 u16（C/S自有元数据，无原生偏移） owner_member：真实owner成员；FFFF为当前公共world/run根 | +0x4 u16（C/S自有元数据，无原生偏移） subject_member：真实subject成员；FFFF只用于合同允许的world结果 | +0x6 u16（C/S自有元数据，无原生偏移） purpose：固定用途编号，按生成用途目录解释 | +0x8 u16（C/S自有元数据，无原生偏移） layout：已有具名结果payload布局，非任意函数／字典 | +0xA u64（C/S自有元数据，无原生偏移） owner_instance：Host在同session/run内唯一分配给用途实例的id；不能改绑其他subject，不是地址 | +0x12 u64（C/S自有元数据，无原生偏移） owner_generation：显示owner实例代次 | +0x1A u64（C/S自有元数据，无原生偏移） produced_revision：实际初次生产的来源修订；缓存引用不能重贴新修订 | +0x22 u16（C/S自有元数据，无原生偏移） dependency_count：32B具名依赖条目数 | +0x24 u16（C/S自有元数据，无原生偏移） contribution_count：16B贡献定位条目数 | +0x26 u32（C/S自有元数据，无原生偏移） payload_bytes：后随依赖、贡献定位及typed结果正文总长度 |
|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


