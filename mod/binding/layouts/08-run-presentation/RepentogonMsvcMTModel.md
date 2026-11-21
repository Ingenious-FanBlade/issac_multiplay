# RepentogonMsvcMTModel

由唯一布局源生成。布局编号 403；布局版本 90；原生大小 0x1388；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

动态来源；不写死官方DLL偏移，不升级为本轮实测 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 index【权威】现有诊断模型的双缓冲索引；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；推断） | 0x4 u32[1248] words【权威】MSVC引擎双缓冲；不是原生MTRNG；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4992B；推断） | 0x1384 u32 mask【权威】现有模型尾掩码0xFFFFFFFF；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；推断） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `index`：random_runtime_lua.cpp::FindMsvcEngine/ReadRandomWords；LuaInit.cpp::gen；旧诊断实现，非本轮DLL ABI核定
- `words`：random_runtime_lua.cpp::FindMsvcEngine/ReadRandomWords；LuaInit.cpp::gen；旧诊断实现，非本轮DLL ABI核定
- `mask`：random_runtime_lua.cpp::FindMsvcEngine/ReadRandomWords；LuaInit.cpp::gen；旧诊断实现，非本轮DLL ABI核定
