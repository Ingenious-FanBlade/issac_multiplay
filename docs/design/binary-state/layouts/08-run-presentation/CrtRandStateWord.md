# CrtRandStateWord

由唯一布局源生成。布局编号 405；布局版本 75；原生大小 0x4；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

仅字段视图；不假设PTD总大小/固定偏移或跨线程共享 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 thread_state【权威】CRT线程状态，PTD字段由当前实际正文解析；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；推断） |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `thread_state`：random_runtime_lua.cpp::PrepareRuntimeRandomHook/CaptureRuntimeRandomWords；CRT IAT与PTD诊断reader
