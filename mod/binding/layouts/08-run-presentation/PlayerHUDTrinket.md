# PlayerHUDTrinket

由唯一布局源生成。布局编号 312；布局版本 94；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。 版本70：普通图标不发重复visual_id/Image/center；特殊分支仍完整发送，tint/book/charge保留。

记录编码预算≤16B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 i32 visual_id【换算】实际饰品显示身份（4B；已核） | 0x4 i32 tint_override_source【换算】SDK名slot；837720在非golden分支实际判断此DWORD是否非零并选择额外colorize（4B；已核） | 0x8 Image* image.image【权威】实际Image与本地引用计数control：实际选中图像由第8步资源身份承接（4B；已核） | 0xC Control* image.strong_control【本地】实际Image与本地引用计数control：本地引用计数／析构控制块（4B；已核） | 0x10 f32 draw_center.x【状态】Update扫描实际图片非透明范围形成的绘制中心：X 分量（4B；已核） | 0x14 f32 draw_center.y【状态】Update扫描实际图片非透明范围形成的绘制中心：Y 分量（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：PlayerHUDTrinket具名当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `visual_id`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720
- `tint_override_source`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720
- `image.image`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `image.strong_control`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `draw_center.x`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `draw_center.y`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
