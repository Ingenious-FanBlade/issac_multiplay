# LuaMathRandomPayload

由唯一布局源生成。布局编号 404；布局版本 84；原生大小 0x20；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

只登记32B payload；GC/Userdata头、closure、registry均不在该范围，不冒充完整Lua对象 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u64 s0【权威】xoshiro256**内部状态字；closure实际upvalue所有；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（8B；SDK声明） | 0x8 u64 s1【权威】xoshiro256**内部状态字；closure实际upvalue所有；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（8B；SDK声明） | 0x10 u64 s2【权威】xoshiro256**内部状态字；closure实际upvalue所有；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（8B；SDK声明） | 0x18 u64 s3【权威】xoshiro256**内部状态字；closure实际upvalue所有；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（8B；SDK声明） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `s0`：REPENTOGON libs/lua/lmathlib.c::RanState / math_random；random_runtime_lua.cpp实际closure payload reader
- `s1`：REPENTOGON libs/lua/lmathlib.c::RanState / math_random；random_runtime_lua.cpp实际closure payload reader
- `s2`：REPENTOGON libs/lua/lmathlib.c::RanState / math_random；random_runtime_lua.cpp实际closure payload reader
- `s3`：REPENTOGON libs/lua/lmathlib.c::RanState / math_random；random_runtime_lua.cpp实际closure payload reader
