# BossPool_Pool

由唯一布局源生成。布局编号 396；布局版本 89；原生大小 0x3C；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 std::string name【权威】池配置名；字节/字符均不上线；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（24B；SDK声明） | 0x18 std::vector<BossPool_Entry> entries【权威】Host候选列表与顺序；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（12B；SDK声明） | 0x24 f32 total_weight【权威】候选权重和；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x28 u32 rng.seed【权威】该池独立抽选流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：伪随机状态 seed；仅权威端推进（4B；SDK声明） | 0x2C u32 rng.shift_1【权威】该池独立抽选流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数一；仅权威端使用（4B；SDK声明） | 0x30 u32 rng.shift_2【权威】该池独立抽选流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数二；仅权威端使用（4B；SDK声明） | 0x34 u32 rng.shift_3【权威】该池独立抽选流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数三；仅权威端使用（4B；SDK声明） | 0x38 i32 double_trouble_variant_start【权威】双Boss房间选择起点；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `name`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl
- `entries`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl
- `total_weight`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl
- `rng.seed`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng.shift_1`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng.shift_2`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng.shift_3`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `double_trouble_variant_start`：BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl
