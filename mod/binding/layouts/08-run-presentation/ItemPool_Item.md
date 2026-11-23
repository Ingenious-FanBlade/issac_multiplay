# ItemPool_Item

由唯一布局源生成。布局编号 391；布局版本 94；原生大小 0x34；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 f32 total_weight【权威】当前权重和；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x4 std::vector<PoolItem> pool_list【权威】Host有序池条目；外部元素见PoolItem；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（12B；SDK声明） | 0x10 u32 rng_1.seed【权威】第一条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：伪随机状态 seed；仅权威端推进（4B；SDK声明） | 0x14 u32 rng_1.shift_1【权威】第一条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数一；仅权威端使用（4B；SDK声明） | 0x18 u32 rng_1.shift_2【权威】第一条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数二；仅权威端使用（4B；SDK声明） | 0x1C u32 rng_1.shift_3【权威】第一条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数三；仅权威端使用（4B；SDK声明） | 0x20 u32 rng_2.seed【权威】第二条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：伪随机状态 seed；仅权威端推进（4B；SDK声明） | 0x24 u32 rng_2.shift_1【权威】第二条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数一；仅权威端使用（4B；SDK声明） | 0x28 u32 rng_2.shift_2【权威】第二条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数二；仅权威端使用（4B；SDK声明） | 0x2C u32 rng_2.shift_3【权威】第二条池流；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Xorshift 参数三；仅权威端使用（4B；SDK声明） | 0x30 u32 bible_upgrade【权威】Bible加入/替换生产计数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） |
|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `total_weight`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池
- `pool_list`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池
- `rng_1.seed`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_1.shift_1`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_1.shift_2`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_1.shift_3`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_2.seed`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_2.shift_1`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_2.shift_2`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `rng_2.shift_3`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池；嵌套字段：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `bible_upgrade`：ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池
