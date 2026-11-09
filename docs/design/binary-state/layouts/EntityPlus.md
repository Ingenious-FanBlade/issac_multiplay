# EntityPlus

由唯一布局源生成。布局编号 15；布局版本 44；原生大小 0x10；未知 5B；完整：否。

**范围仅为已核前缀，不是动态对象分配大小；具体对象仍需完整布局。**

官方1.1.3扩展基类前缀；Player/Familiar等Plus派生类由第4/5步描述。5B未知对齐不发送，不能宣称完整动态扩展。 版本39复核保留两nullable展示输入，安装依赖已有同类holder/data、当前Player/Room/资源与ABI；不调用会分配的GetEntityPlus做采集，缺失不当Null。

原生字节归属：

| 0x0 u32 vtable【本地】REP扩展本地虚表（4B；已核） | 0x4 optional<bool> flying_override【换算】官方IsFlying hook的未设／false／true覆盖；Player GetWaterClipInfo VA0x6B31C3通过IsFlying决定水面偏移，实际绘制经虚槽消费，保留三态。（2B；已核） | 0x6 bytes[2] unknown_6_7【未知】成员间对齐区尚未对已安装REP二进制核定；不复制（2B；未知） | 0x8 optional<u32>::value_and_engaged water_override【换算】官方GetWaterClipInfo hook在super后覆写完整bitFlags；水面／镜面展示使用，保留Null／有效零／完整u32，不只同步effective flags或直接传optional字节。（5B；已核） | 0xD bytes[3] unknown_d_f【未知】optional内部尾部对齐区未对已安装REP二进制核定；不复制（3B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 9—9B，不含第 9 步记录头）：

块 0：EntityPlus 公共可选覆盖（world，9—9B）：

| +0x0 i8 flying_override：官方IsFlying hook的未设／false／true覆盖；Player GetWaterClipInfo VA0x6B31C3通过IsFlying决定水面偏移，实际绘制经虚槽消费，保留三态。 | +0x1 u64 water_override：官方GetWaterClipInfo hook在super后覆写完整bitFlags；水面／镜面展示使用，保留Null／有效零／完整u32，不只同步effective flags或直接传optional字节。 |
|---|---|

证据：

- `vtable`：EntityPlus.h虚析构；client_player_sdk_abi.cpp；PlusHeader sizeof16
- `flying_override`：EntityPlus.h；MSVC optional:70-75；state/entity_extension.cpp；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_6_7`：官方EntityPlus成员顺序与Win32 ABI
- `water_override`：EntityPlus.h；MSVC optional:70-75；state/entity_extension.cpp；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_d_f`：MSVC optional与EntityPlus Win32 ABI
