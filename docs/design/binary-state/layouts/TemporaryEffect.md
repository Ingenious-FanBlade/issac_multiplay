# TemporaryEffect

由唯一布局源生成。布局编号 14；布局版本 42；原生大小 0x10；未知 4B；完整：否。

+0x0C 必须保留为未知字节范围；官方 SDK 声明结构大小 0x10，但只有前三个字段，未证明末尾 DWORD 是填充。 版本39：item／count／cooldown均有Player/HUD消费者，10B保持；外层disabled及配置资源与同目标版本安装，不能仅安装最终costume代替查询状态。

原生字节归属：

| 0x0 ItemConfig_Item* item【资源】配置资源身份，必须保留(type,id)及本地config.cooldown；90A7A0/90A900按类别与id查找，Player/HUD直接消费。（4B；已核） | 0x4 i32 count【状态】叠加数；HUD835CDF→90A900及Player769E90等查询读取，不由库存或costume推导。（4B；已核） | 0x8 i32 cooldown【状态】最终剩余冷却；角色27进度环758952直接读取+8，除以配置+78 cooldown；保留同帧原值，不在Client递减。（4B；已核） | 0xC u32 unknown_0C【未知】SDK 对象大小 0x10 中未声明的末尾 DWORD；尚未证明可当填充（4B；未知） |
|---|---|---|---|

状态二进制布局（总计 10—10B，不含第 9 步记录头）：

块 0：临时效果（body，10—10B）：

| +0x0 u16 item：配置资源身份，必须保留(type,id)及本地config.cooldown；90A7A0/90A900按类别与id查找，Player/HUD直接消费。 | +0x2 i32 count：叠加数；HUD835CDF→90A900及Player769E90等查询读取，不由库存或costume推导。 | +0x6 i32 cooldown：最终剩余冷却；角色27进度环758952直接读取+8，除以配置+78 cooldown；保留同帧原值，不在Client递减。 |
|---|---|---|

证据：

- `item`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `count`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `cooldown`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_0C`：TemporaryEffect.zhl 只命名 +0x00 item、+0x04 count、+0x08 cooldown；J273 +0x0C 用途未核；J273 VA0x90A194把新记录尾字置0，VA0x90A0D0/0x90A0D3按16B复制传入记录；不能仅以常见零值判为padding，主审已复读
