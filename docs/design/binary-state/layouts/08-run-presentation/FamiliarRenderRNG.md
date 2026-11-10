# FamiliarRenderRNG

由唯一布局源生成。布局编号 385；布局版本 58；原生大小 0x10；未知 0B；完整：是。

原生16B物理布局不变；旧D0 seed重放草稿已由room-random-handoff.md的逐用途结果替代 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 seed【权威】Host正常纯绘制流游标；当前产生的alpha由具体consumer承接；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x4 u32 shift_1【派生】首次合法本地初始化后固定为4；文件初值0不作为绘制算法；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x8 u32 shift_2【派生】首次合法本地初始化后固定为3；文件初值0不作为绘制算法；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0xC u32 shift_3【派生】首次合法本地初始化后固定为27；文件初值0不作为绘制算法；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `seed`：J273 VABD0674；61E533/61E549读、61E5A3写；61E604栈ColorMod.alpha；61F6A2/61F6AA只初始化(4,3,27)，四字直接引用仅对应Render
- `shift_1`：J273 VABD0674；61E533/61E549读、61E5A3写；61E604栈ColorMod.alpha；61F6A2/61F6AA只初始化(4,3,27)，四字直接引用仅对应Render
- `shift_2`：J273 VABD0674；61E533/61E549读、61E5A3写；61E604栈ColorMod.alpha；61F6A2/61F6AA只初始化(4,3,27)，四字直接引用仅对应Render
- `shift_3`：J273 VABD0674；61E533/61E549读、61E5A3写；61E604栈ColorMod.alpha；61F6A2/61F6AA只初始化(4,3,27)，四字直接引用仅对应Render
