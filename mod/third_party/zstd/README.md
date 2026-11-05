# Zstandard 1.5.7

来源：https://github.com/facebook/zstd/tree/v1.5.7 。保留官方 lib/common、compress、decompress 和公共头，源码未改。采用随附 LICENSE 中的 BSD 许可证；产品打包须一并分发该许可。

本项目只编译 Win32 静态库，使用与 DLL 一致的 /MD，不加载额外 zstd.dll。不构建 CLI、测试、字典训练器、旧格式或多线程压缩；禁用汇编。网络默认 level 3、windowLog=23、单帧、无字典；仅已协商且路由授权的可靠世界范围可压缩。
