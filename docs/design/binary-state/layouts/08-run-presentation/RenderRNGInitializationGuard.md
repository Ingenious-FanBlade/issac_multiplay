# RenderRNGInitializationGuard

由唯一布局源生成。布局编号 386；布局版本 60；原生大小 0x4；未知 0B；完整：是。

只登记这4B独立变量，不把到RNG本体之间的地址空间当作对象 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 i32 local_static_guard【本地】C++线程安全静态初始化guard；含TLS/运行库协议；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；已核） |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `local_static_guard`：J273 BEE364 / BED310；Render内_Init_thread_header/footer
