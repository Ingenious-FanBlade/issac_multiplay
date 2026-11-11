# MsvcVectorBool32

由唯一布局源生成。布局编号 402；布局版本 60；原生大小 0x10；未知 0B；完整：否。

不将此诊断ABI推广到任意STL编译版本；本次无reader或writer 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 std::vector<u32> word_storage【权威】Host bit存储头；指向外部bit words；仅工具链ABI模型，不作为目标DLL实际对象已核证明。；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（12B；推断） | 0xC u32 bit_count【权威】有效bit数量，原生保存/恢复使用；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `word_storage`：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
- `bit_count`：J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型
