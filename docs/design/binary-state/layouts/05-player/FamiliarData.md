# FamiliarData

由唯一布局源生成。布局编号 168；布局版本 60；原生大小 0x34；未知 12B；完整：否。

原生字节归属：

| 0x0 i32[8] saved_identifiers_counters【权威】seed／variant／subtype／coins／hearts／keys／state／roomClear，后二者由ESSM劫持（32B；SDK声明） | 0x20 bytes[8] unknown_20_27【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（8B；未知） | 0x28 f32 health【权威】保存健康（4B；SDK声明） | 0x2C u32 drop_rng_seed【权威】保存掉落RNG种子（4B；SDK声明） | 0x30 bytes[4] unknown_30_33【未知】尾部尚未解释；不以对象大小或初始化零值判填充（4B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `saved_identifiers_counters`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_20_27`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `health`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `drop_rng_seed`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_30_33`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
