# RNG

由唯一布局源生成。布局编号 8；布局版本 12；原生大小 0x10；未知 0B；完整：是。

J273 SetSeed 写入全部四 DWORD；RandomInt/RandomFloat/Next 读取三个 shift 并更新 seed。此类型是 authority-only，不加入发送块。

原生字节归属：

| 0x0 u32 seed【权威】伪随机状态 seed；仅权威端推进（4B；已核） | 0x4 u32 shift_1【权威】Xorshift 参数一；仅权威端使用（4B；已核） | 0x8 u32 shift_2【权威】Xorshift 参数二；仅权威端使用（4B；已核） | 0xC u32 shift_3【权威】Xorshift 参数三；仅权威端使用（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `seed`：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `shift_1`：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `shift_2`：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
- `shift_3`：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl
