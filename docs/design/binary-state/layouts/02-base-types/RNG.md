# RNG

由唯一布局源生成。布局编号 8；布局版本 60；原生大小 0x10；未知 0B；完整：是。

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

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3483 | RNG.GetSeed | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3484 | RNG.GetShiftIdx | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3485 | RNG.Next | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3486 | RNG.PhantomFloat | " " / "float" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| docs/design/tx-04-21/stream-catalog.json:3487 | RNG.PhantomInt | " int Max " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| docs/design/tx-04-21/stream-catalog.json:3488 | RNG.PhantomNext | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| docs/design/tx-04-21/stream-catalog.json:3489 | RNG.PhantomPrevious | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| docs/design/tx-04-21/stream-catalog.json:3490 | RNG.PhantomVector | " " / "Vector" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| docs/design/tx-04-21/stream-catalog.json:3491 | RNG.Previous | " " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3492 | RNG.RNG | " " / "RNG" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3493 | RNG.RNG | " int seed = 2853650767, int shiftIdx = 35 " / "RNG" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3494 | RNG.RandomFloat | " " / "float" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3495 | RNG.RandomInt | " int Max " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3496 | RNG.RandomInt | " int Min, int Max " / "int" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3497 | RNG.RandomVector | " " / "Vector" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3498 | RNG.SetSeed | " int Seed, int ShiftIdx " / "void" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| docs/design/tx-04-21/stream-catalog.json:3499 | RNG.SetSeed | " int Seed, int ShiftIdx = 35 " / "void" | authority → ；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
