# RoomShockwave

由唯一布局源生成。布局编号 252；布局版本 75；原生大小 0x1C；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 f32 origin.x【状态】Q8冲击波当前origin；只安装Host当前值，不运行Room::Update：X 分量（4B；已核） | 0x4 f32 origin.y【状态】Q8冲击波当前origin；只安装Host当前值，不运行Room::Update：Y 分量（4B；已核） | 0x8 f32 radius【状态】Q8冲击波当前radius；只安装Host当前值，不运行Room::Update（4B；已核） | 0xC f32 strength【状态】Q8冲击波当前strength；只安装Host当前值，不运行Room::Update（4B；已核） | 0x10 f32 speed【状态】Q8冲击波当前speed；只安装Host当前值，不运行Room::Update（4B；已核） | 0x14 i32 current【状态】Q8冲击波当前current；只安装Host当前值，不运行Room::Update（4B；已核） | 0x18 i32 maximum【状态】Q8冲击波当前maximum；只安装Host当前值，不运行Room::Update（4B；已核） |
|---|---|---|---|---|---|---|

状态二进制布局（总计 28—28B，不含第 9 步记录头）：

块 0：Q8原生冲击波当前值（world，28—28B）：

| +0x0 Vector origin：Q8冲击波当前origin；只安装Host当前值，不运行Room::Update | +0x8 f32 radius：Q8冲击波当前radius；只安装Host当前值，不运行Room::Update | +0xC f32 strength：Q8冲击波当前strength；只安装Host当前值，不运行Room::Update | +0x10 f32 speed：Q8冲击波当前speed；只安装Host当前值，不运行Room::Update | +0x14 i32 current：Q8冲击波当前current；只安装Host当前值，不运行Room::Update | +0x18 i32 maximum：Q8冲击波当前maximum；只安装Host当前值，不运行Room::Update |
|---|---|---|---|---|---|

证据：

- `origin.x`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `origin.y`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `radius`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
- `strength`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
- `speed`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
- `current`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
- `maximum`：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0；docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
