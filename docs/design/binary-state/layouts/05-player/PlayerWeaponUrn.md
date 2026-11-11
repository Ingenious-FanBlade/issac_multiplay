# PlayerWeaponUrn

由唯一布局源生成。布局编号 191；布局版本 60；原生大小 0x54；未知 24B；完整：否。

Player P5专用动态view；2026-11-09只读J273 vtable[-1]→COL+0C→TypeDescriptor核定RTTI。真实factory/Init与窄Free由第13步owner接线。

原生字节归属：

| 0x0 u32 vtable【本地】原生 Weapon 虚表指针（4B；已核） | 0x4 Entity* owner【权威】Host玩法Weapon所属关系；当前充能条公式不读取owner。Client工厂使用本地承载身体，退休前owner=null；不发送Host引用。（4B；已核） | 0x8 EntityPtr common_attack_link【权威】真实公共EntityPtr攻击链接，9C5050/9D4240/9D0990/9E0D50清理会Remove并解绑；不是通用展示MainEntity（4B；已核） | 0xC f32 fire_delay【权威】Host Weapon::Update VA0x9C5220使用owner速度倍率递减的射击冷却；当前展示不用，不作为Client时钟。（4B；SDK声明） | 0x10 f32 max_fire_delay【权威】最大射击冷却；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。（4B；SDK声明） | 0x14 f32 charge【权威】武器充能；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。（4B；SDK声明） | 0x18 bytes[8] unknown_18_1F【未知】J273 ctor 初始化 +0x18=1.0f、+0x1C=0；字段语义未核（8B；未知） | 0x20 Vector direction【权威】Host Fire／瞄准输入；Player最终朝向、Sprite与攻击实体单独安装，当前装备展示不消费此Vector。（8B；SDK声明） | 0x28 f32[2] axis_parameters【权威】IsAxisAligned VA0x9C5090同时读取+28/+2C及owner／道具；不是原生绘制输入，两个f32类型事实保留但不传输。（8B；推断） | 0x30 i32 weapon_type【状态】HUD RenderActiveItem VA0x836093区分type12，Familiar60C190区分type15；由工厂动态类及初始化类型核对，保留。（4B；SDK声明） | 0x34 u32 modifiers【权威】武器修改标志；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。（4B；SDK声明） | 0x38 i32 num_fired【权威】Host发射历史／公开查询值；当前Player/HUD/Familiar展示不消费。（4B；SDK声明） | 0x3C u32 unknown_3C【未知】SDK 未声明；J273 ctor 写零，语义未核（4B；未知） | 0x40 u32 unknown_40【未知】J273 Weapon ctor 写入范围超过 SDK sizeof(Weapon)=0x40；+0x40 字节归属未核（4B；未知） | 0x44 bytes[8] tail_44【未知】动态factory尾部尚未解释；不当填充（8B；未知） | 0x4C EntityPtr[2] tail_4c【权威】动态攻击链接，Host模拟；不作为展示边（8B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：展示值（world，4—4B）：

| +0x0 i32 weapon_type：HUD RenderActiveItem VA0x836093区分type12，Familiar60C190区分type15；由工厂动态类及初始化类型核对，保留。 |
|---|

证据：

- `vtable`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；J273 VA0x9C4ED9写Weapon vtable，主审已复读
- `owner`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `common_attack_link`：5-player-p3-p5.md
- `fire_delay`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `max_fire_delay`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md；5-player-p3-p5.md
- `charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md；5-player-p3-p5.md
- `unknown_18_1F`：J273 Weapon ctor VA0x9C4EB0：VA0x9C4EED/0x9C4F58写+0x18=1.0f，VA0x9C4F3A写+0x1C=0；语义未核，主审已复读
- `direction`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `axis_parameters`：J273 Weapon::IsAxisAligned VA 0x9C5090 读取 this+0x28 与 +0x2C；具体含义未核；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `weapon_type`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `modifiers`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md；5-player-p3-p5.md
- `num_fired`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_3C`：J273 Weapon ctor VA 0x9C4EB0 初始化 +0x3C
- `unknown_40`：J273 Weapon ctor VA 0x9C4EB0 写 this+0x40；player-application.json:62582
- `tail_44`：5-player-p3-p5.md
- `tail_4c`：5-player-p3-p5.md
