# GameStateItemPool

由唯一布局源生成。布局编号 398；布局版本 58；原生大小 0x2B4；未知 11B；完整：否。

直接UNKNOWN11B；保存体不进入正式网络Baseline 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 std::vector<f32>[31] pool_item_weights【权威】31池当前权重保存，各项外部f32；不含展示负载；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（372B；已核） | 0x174 u32[31] bible_upgrades【权威】保存Bible计数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（124B；已核） | 0x1F0 u32 pool_seed【权威】Host保存/恢复值，不是Client baseline；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x1F4 i32 last_pool【权威】Host保存/恢复值，不是Client baseline；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x1F8 u32 item_pool_rng_seed【权威】Host保存/恢复值，不是Client baseline；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x1FC u32 trinket_pool_rng_seed【权威】Host保存/恢复值，不是Client baseline；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x200 bytes[8] unexplained_saved_values【未知】尚未解释的原生字节；不读取、不发送，不猜填充；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（8B；未知） | 0x208 std::vector<u32> removed_collectibles.word_storage【权威】保存的移除位；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Host bit存储头；指向外部bit words；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（12B；推断） | 0x214 u32 removed_collectibles.bit_count【权威】保存的移除位；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：有效bit数量，原生保存/恢复使用；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x218 std::vector<u32> trinket_in_pool_bits.word_storage【权威】实际保存每饰品inPool位；不是vector<TrinketPoolItem>；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：Host bit存储头；指向外部bit words；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（12B；推断） | 0x224 u32 trinket_in_pool_bits.bit_count【权威】实际保存每饰品inPool位；不是vector<TrinketPoolItem>；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。：有效bit数量，原生保存/恢复使用；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x228 i32[15] pill_effects【权威】Host基础效果保存；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（60B；已核） | 0x264 bool[15] identified_pill_effects【权威】Host鉴定位保存；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（15B；已核） | 0x273 bytes[1] unknown_273【未知】尚未解释的原生字节；不读取、不发送，不猜填充；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（1B；未知） | 0x274 u16[31] remaining_genesis_items【权威】Host Genesis额度保存；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（62B；已核） | 0x2B2 bytes[2] unknown_2b2_2b3【未知】尚未解释的原生字节；不读取、不发送，不猜填充；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（2B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `pool_item_weights`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `bible_upgrades`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `pool_seed`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `last_pool`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `item_pool_rng_seed`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `trinket_pool_rng_seed`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `unexplained_saved_values`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准；对应ItemPool+674/+678
- `removed_collectibles.word_storage`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准；嵌套字段：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
- `removed_collectibles.bit_count`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准；嵌套字段：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
- `trinket_in_pool_bits.word_storage`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准；嵌套字段：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
- `trinket_in_pool_bits.bit_count`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准；嵌套字段：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
- `pill_effects`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `identified_pill_effects`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `unknown_273`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `remaining_genesis_items`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
- `unknown_2b2_2b3`：GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准
