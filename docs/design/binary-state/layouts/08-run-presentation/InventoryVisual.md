# InventoryVisual

由唯一布局源生成。布局编号 314；布局版本 60；原生大小 0xC；未知 0B；完整：是。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

记录编码预算≤4194304B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 i32 visual_id【状态】有序显示身份，含负生成ID（4B；已核） | 0x4 Image* image.image【权威】实际Image与本地引用计数control：实际选中图像由第8步资源身份承接（4B；已核） | 0x8 Control* image.strong_control【本地】实际Image与本地引用计数control：本地引用计数／析构控制块（4B；已核） |
|---|---|---|

状态二进制布局（总计 6—6B，不含第 9 步记录头）：

块 0：InventoryVisual具名当前结果（world，6—6B）：

| +0x0 i32 visual_id：有序显示身份，含负生成ID | +0x4 u16 image：实际Image与本地引用计数control |
|---|---|

证据：

- `visual_id`：J273 832C10/839370/839A80；PlayerHUD+6A4 vector stride12
- `image.image`：J273 832C10/839370/839A80；PlayerHUD+6A4 vector stride12；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `image.strong_control`：J273 832C10/839370/839A80；PlayerHUD+6A4 vector stride12；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
