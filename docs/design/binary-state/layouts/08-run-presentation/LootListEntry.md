# LootListEntry

由唯一布局源生成。布局编号 401；布局版本 71；原生大小 0x14；未知 0B；完整：否。

Loot deque/closure外壳不由本结构推导；预览与生成数据不可混为一谈 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 type【权威】预生成实体类型；不是已创建实体身份；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x4 u32 variant【权威】预生成变体；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x8 u32 subtype【权威】预生成子类型；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0xC u32 seed【权威】Host预生成seed；不在Client重新spawn；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x10 RNG* rng【本地】可空非拥有指针；官方Getter不确立owner/lifetime；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `type`：LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp
- `variant`：LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp
- `subtype`：LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp
- `seed`：LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp
- `rng`：LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp
