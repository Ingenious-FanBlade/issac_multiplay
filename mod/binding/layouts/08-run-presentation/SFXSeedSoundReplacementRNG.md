# SFXSeedSoundReplacementRNG

由唯一布局源生成。布局编号 407；布局版本 94；原生大小 0x10；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

声音改号37/301的独立16B字段视图，先于选sample；不与BD0F40合并，不进入D0 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 seed【权威】Host正常声音改号当前游标；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x4 u32 shift_1【权威】正常初始化写入2的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x8 u32 shift_2【权威】正常初始化写入7的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0xC u32 shift_3【权威】正常初始化写入7的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `seed`：92FAD0正常seed7改号；92FB91写seed；shift(2,7,7)，guard C01184本地；presentation-audio-sources.md
- `shift_1`：92FAD0正常seed7改号；92FB91写seed；shift(2,7,7)，guard C01184本地；presentation-audio-sources.md
- `shift_2`：92FAD0正常seed7改号；92FB91写seed；shift(2,7,7)，guard C01184本地；presentation-audio-sources.md
- `shift_3`：92FAD0正常seed7改号；92FB91写seed；shift(2,7,7)，guard C01184本地；presentation-audio-sources.md
