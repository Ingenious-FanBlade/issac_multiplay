# TemporaryEffect

由唯一布局源生成。布局编号 14；布局版本 8；原生大小 0x10；未知 4B；完整：否。

+0x0C 必须保留为未知字节范围；官方 SDK 声明结构大小 0x10，但只有前三个字段，未证明末尾 DWORD 是填充。

原生字节归属：

| 0x0 ItemConfig_Item* item【资源】效果配置表序号；表项必须保留配置type与id，不能只发送裸id（4B；已核） | 0x4 i32 count【状态】效果叠加数（4B；已核） | 0x8 i32 cooldown【状态】效果冷却（4B；已核） | 0xC u32 unknown_0C【未知】SDK 对象大小 0x10 中未声明的末尾 DWORD；尚未证明可当填充（4B；未知） |
|---|---|---|---|

状态二进制布局（总计 10—10B，不含第 9 步记录头）：

块 0：临时效果（body，10—10B）：

| +0x0 u16 item：效果配置表序号；表项必须保留配置type与id，不能只发送裸id | +0x2 i32 count：效果叠加数 | +0x6 i32 cooldown：效果冷却 |
|---|---|---|

证据：

- `item`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*
- `count`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读
- `cooldown`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读
- `unknown_0C`：TemporaryEffect.zhl 只命名 +0x00 item、+0x04 count、+0x08 cooldown；J273 +0x0C 用途未核；J273 VA0x90A194把新记录尾字置0，VA0x90A0D0/0x90A0D3按16B复制传入记录；不能仅以常见零值判为padding，主审已复读
