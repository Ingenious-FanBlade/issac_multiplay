# Weapon

由唯一布局源生成。布局编号 13；布局版本 23；原生大小 0x44；未知 20B；完整：否。

**范围仅为已核前缀，不是动态对象分配大小；具体对象仍需完整布局。**

sdk_size 0x40 不是完整基础构造足迹。仅声明到 ctor 写入的 0x44 base_prefix，不能代表动态分配大小；Weapon 子类真实 allocation size 由第5步按 factory/RTTI 接管。GetMaxCharge 是虚函数且父类实现返回 0；IsAxisAligned 还依赖 owner 类/道具条件；IsItemAnimFinished 查询 owner 动画状态；GetMainEntity 为虚函数/子类行为。故不声明 Weapon public_owner。 主审复读基类GetMainEntity槽VA0x9C4E70仅xor eax,eax;ret，不据此推定+8为主实体。 RTTI主审纠错：TypeDescriptor在VA0xBD1218（RVA0x7D1218），VA0xBD1220只是名字起点；Weapon COL VA0xB452C8的+0xC直接指向BD1218。

原生字节归属：

| 0x0 u32 vtable【本地】原生 Weapon 虚表指针（4B；已核） | 0x4 Entity* owner【引用】Weapon 所属实体（4B；已核） | 0x8 Entity* unknown_entity_pointer【未知】SDK 中曾注释为 Entity* _entity；J273 ctor 置空，但用途及与 GetMainEntity 的关系未核（4B；未知） | 0xC f32 fire_delay【状态】当前射击冷却（4B；SDK声明） | 0x10 f32 max_fire_delay【状态】最大射击冷却（4B；SDK声明） | 0x14 f32 charge【状态】武器充能（4B；SDK声明） | 0x18 bytes[8] unknown_18_1F【未知】J273 ctor 初始化 +0x18=1.0f、+0x1C=0；字段语义未核（8B；未知） | 0x20 f32 direction.x【状态】武器方向：X 分量（4B；SDK声明） | 0x24 f32 direction.y【状态】武器方向：Y 分量（4B；SDK声明） | 0x28 f32[2] axis_parameters【状态】IsAxisAligned 读取的两个 float 参数；业务语义未明，按有限 float 保留（8B；推断） | 0x30 i32 weapon_type【状态】武器类型（4B；SDK声明） | 0x34 u32 modifiers【状态】武器修改标志（4B；SDK声明） | 0x38 i32 num_fired【状态】已发射数量（4B；SDK声明） | 0x3C u32 unknown_3C【未知】SDK 未声明；J273 ctor 写零，语义未核（4B；未知） | 0x40 u32 unknown_40【未知】J273 Weapon ctor 写入范围超过 SDK sizeof(Weapon)=0x40；+0x40 字节归属未核（4B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 42—42B，不含第 9 步记录头）：

块 0：武器基础状态（body，42—42B）：

| +0x0 u16 owner：Weapon 所属实体 | +0x2 f32 fire_delay：当前射击冷却 | +0x6 f32 max_fire_delay：最大射击冷却 | +0xA f32 charge：武器充能 | +0xE Vector direction：武器方向 | +0x16 f32[2] axis_parameters：IsAxisAligned 读取的两个 float 参数；业务语义未明，按有限 float 保留 | +0x1E i32 weapon_type：武器类型 | +0x22 u32 modifiers：武器修改标志 | +0x26 i32 num_fired：已发射数量 |
|---|---|---|---|---|---|---|---|---|

证据：

- `vtable`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；J273 VA0x9C4ED9写Weapon vtable，主审已复读
- `owner`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `unknown_entity_pointer`：Weapon.zhl +0x08 注释掉的 Entity*；J273 VA 0x9C4EB0 ctor 初始化 null
- `fire_delay`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `max_fire_delay`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `unknown_18_1F`：J273 Weapon ctor VA0x9C4EB0：VA0x9C4EED/0x9C4F58写+0x18=1.0f，VA0x9C4F3A写+0x1C=0；语义未核，主审已复读
- `direction.x`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `direction.y`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `axis_parameters`：J273 Weapon::IsAxisAligned VA 0x9C5090 读取 this+0x28 与 +0x2C；具体含义未核
- `weapon_type`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `modifiers`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `num_fired`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582
- `unknown_3C`：J273 Weapon ctor VA 0x9C4EB0 初始化 +0x3C
- `unknown_40`：J273 Weapon ctor VA 0x9C4EB0 写 this+0x40；player-application.json:62582
