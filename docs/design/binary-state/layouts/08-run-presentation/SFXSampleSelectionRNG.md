# SFXSampleSelectionRNG

由唯一布局源生成。布局编号 406；布局版本 58；原生大小 0x10；未知 0B；完整：是。

音效随机选sample的私有16B字段视图，不是SFXManager完整ABI，不是第三个D0流 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 seed【权威】Host选sample当前游标；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x4 u32 shift_1【权威】正常初始化写入5的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0x8 u32 shift_2【权威】正常初始化写入27的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） | 0xC u32 shift_3【权威】正常初始化写入8的shift参数；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `seed`：907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地
- `shift_1`：907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地
- `shift_2`：907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地
- `shift_3`：907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地
