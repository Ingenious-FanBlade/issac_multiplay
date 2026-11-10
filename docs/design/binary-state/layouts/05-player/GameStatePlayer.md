# GameStatePlayer

由唯一布局源生成。布局编号 167；布局版本 58；原生大小 0x5BC；未知 1408B；完整：否。

SDK仅命名少量字段；所有间隙逐段UNKNOWN，不把整个保存体称为已知Host字段。

原生字节归属：

| 0x0 i32 player_type【权威】恢复的角色类型（4B；SDK声明） | 0x4 bytes[464] unknown_4_1d3【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（464B；未知） | 0x1D4 vector<EntitySaveState> moving_box【权威】保存的移动箱实体（12B；SDK声明） | 0x1E0 bytes[352] unknown_1e0_33f【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（352B；未知） | 0x340 i32[2] conception_states【权威】ESSM可劫持的保存字段，不能当live Player缓存（8B；SDK声明） | 0x348 bytes[44] unknown_348_373【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（44B；未知） | 0x374 vector<FamiliarData> familiar_data【权威】保存熟悉者的数据，元素逐字节另列（12B；SDK声明） | 0x380 bytes[48] unknown_380_3af【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（48B；未知） | 0x3B0 vector<HistoryItem> history[0].items【权威】保存历史，不借live History初始化／owner证据：SDK给定保存History容器；不借live History的owner证明解释其后8B（12B；SDK声明） | 0x3BC bytes[8] history[0].unknown_c_13【未知】保存历史，不借live History初始化／owner证据：尾部尚未解释；不以对象大小或初始化零值判填充（8B；未知） | 0x3C4 bytes[256] unknown_3c4_4c3【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（256B；未知） | 0x4C4 i32 crafting_output[0].collectible【权威】保存的配方输出：配方输出生产存储，实际结果图像在HUD缓存（4B；SDK声明） | 0x4C8 i32 crafting_output[0].pool【权威】保存的配方输出：配方来源池（4B；SDK声明） | 0x4CC bytes[76] unknown_4cc_517【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（76B；未知） | 0x518 i32 controller_index【权威】Host恢复设备索引；不写Client控制器（4B；SDK声明） | 0x51C bytes[160] unknown_51c_5bb【未知】尾部尚未解释；不以对象大小或初始化零值判填充（160B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `player_type`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_4_1d3`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `moving_box`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_1e0_33f`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `conception_states`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_348_373`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `familiar_data`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_380_3af`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `history[0].items`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `history[0].unknown_c_13`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_3c4_4c3`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `crafting_output[0].collectible`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；嵌套字段：5-player-p3-p5.md
- `crafting_output[0].pool`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；嵌套字段：5-player-p3-p5.md
- `unknown_4cc_517`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
- `controller_index`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_51c_5bb`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0
