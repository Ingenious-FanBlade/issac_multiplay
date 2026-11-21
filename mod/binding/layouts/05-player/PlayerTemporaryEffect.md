# PlayerTemporaryEffect

由唯一布局源生成。布局编号 180；布局版本 90；原生大小 0x10；未知 4B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

普通效果6B；仅配置type1/3/4且id704追加i32 cooldown，保持重复项和原顺序。

原生字节归属：

| 0x0 ItemConfig_Item* item【资源】配置资源身份，必须保留(type,id)及本地config.cooldown；90A7A0/90A900按类别与id查找，Player/HUD直接消费。；非空真实配置；704分支还要求实际配置cooldown除数非零；拒绝FFFF空配置（4B；已核） | 0x4 i32 count【状态】叠加数；HUD835CDF→90A900及Player769E90等查询读取，不由库存或costume推导。（4B；已核） | 0x8 i32 cooldown【换算】最终剩余冷却；角色27进度环758952直接读取+8，除以配置+78 cooldown；保留同帧原值，不在Client递减。（4B；已核） | 0xC u32 unknown_0C【未知】SDK 对象大小 0x10 中未声明的末尾 DWORD；尚未证明可当填充（4B；未知） |
|---|---|---|---|

状态二进制布局（总计 6—10B，不含第 9 步记录头）：

块 0：展示值（world，6—10B）：

| +0x0 u16 item：配置资源身份，必须保留(type,id)及本地config.cooldown；90A7A0/90A900按类别与id查找，Player/HUD直接消费。；非空真实配置；704分支还要求实际配置cooldown除数非零；拒绝FFFF空配置 | +0x2 i32 count：叠加数；HUD835CDF→90A900及Player769E90等查询读取，不由库存或costume推导。 | +0x6 i32（仅is_collectible_704(item)成立时存在） cooldown：最终剩余冷却；角色27进度环758952直接读取+8，除以配置+78 cooldown；保留同帧原值，不在Client递减。 |
|---|---|---|

证据：

- `item`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；mod/binding/catalog/entity-application.json TemporaryEffect.*；docs/archive/design/binary-state/player-equipment-plus-synchronization-review.md
- `count`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；mod/binding/catalog/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读；docs/archive/design/binary-state/player-equipment-plus-synchronization-review.md
- `cooldown`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；mod/binding/catalog/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读；docs/archive/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_0C`：TemporaryEffect.zhl 只命名 +0x00 item、+0x04 count、+0x08 cooldown；J273 +0x0C 用途未核；J273 VA0x90A194把新记录尾字置0，VA0x90A0D0/0x90A0D3按16B复制传入记录；不能仅以常见零值判为padding，主审已复读
