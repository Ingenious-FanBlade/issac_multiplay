# EntityDescHost

由唯一布局源生成。布局编号 160；布局版本 60；原生大小 0x1C；未知 3B；完整：否。

原生字节归属：

| 0x0 i32/f32[6] spawn_descriptor【权威】type／variant／subtype／champion／health／maxHealth生成描述（24B；SDK声明） | 0x18 bool player_controlled【权威】未来生成对象的控制类别（1B；SDK声明） | 0x19 bytes[3] unknown_19_1b【未知】尾部尚未解释；不以对象大小或初始化零值判填充（3B；未知） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `spawn_descriptor`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityDesc.zhl
- `player_controlled`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityDesc.zhl
- `unknown_19_1b`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityPlayer.zhl
