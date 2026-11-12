# EntityFamiliarPlus

由唯一布局源生成。布局编号 16；布局版本 71；原生大小 0x1C；未知 5B；完整：否。

原生字节归属：

| 0x0 u32 vtable【本地】REP扩展本地虚表（4B；已核） | 0x4 optional<bool> flying_override【换算】官方IsFlying hook的未设／false／true覆盖；Player GetWaterClipInfo VA0x6B31C3通过IsFlying决定水面偏移，实际绘制经虚槽消费，保留三态。（2B；已核） | 0x6 bytes[2] unknown_6_7【未知】成员间对齐区尚未对已安装REP二进制核定；不复制（2B；未知） | 0x8 optional<u32>::value_and_engaged water_override【换算】官方GetWaterClipInfo hook在super后覆写完整bitFlags；水面／镜面展示使用，保留Null／有效零／完整u32，不只同步effective flags或直接传optional字节。（5B；已核） | 0xD bytes[3] unknown_d_f【未知】optional内部尾部对齐区未对已安装REP二进制核定；不复制（3B；未知） | 0x10 optional<f32> cached_multiplier【权威】Host倍率回调缓存；读取getter可能触发回调计算，正式采集不调用（8B；已核） | 0x18 uintptr_t cached_multiplier_player【本地】缓存归属的本进程Player地址；不作身份（4B；已核） |
|---|---|---|---|---|---|---|

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
- `cached_multiplier`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `cached_multiplier_player`：docs/design/binary-state/familiar-knife-bomb-evidence.md
