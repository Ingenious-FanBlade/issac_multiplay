# 第 3 步：Entity 公共字节证据

状态：本轮静态研究及主审已完成，SDK 声明、推断和 UNKNOWN 继续保留，没有测试。本文件记录 Entity `[0,0x410)` 的证据，不把 SDK 声明升级成 J273 事实；主 Sprite 复用[第 2 步](base-types-evidence.md#anm2-276-字节)。实施选择和未采纳推导见[主审说明](implementation-3.md)。

## 证据标记

- **J273 直接**：目标 `v1.9.7.12.J273` IDA 中可读到指令、getter/setter 绑定或原生字段访问。
- **SDK 声明**：REPENTOGON 1.1.3 `libzhl/functions/Entity.zhl` 声明类型、偏移或字段名；语义仍须按本表状态判断。
- **REP 源码**：官方 REPENTOGON 1.1.3 源码直接给出的扩展字段和控制流。
- **静态推导**：由初始化/引用/运算指令得出的候选用途，不能提升为稳定契约。
- **UNKNOWN**：用途或是否可安全复制仍未闭合；不得复制，不代表填充。

## 原生字节归属目录

范围左闭右开；相邻行无重叠并覆盖至 `0x410`。类型宽度是当前证据可支持的最细粒度；`UNKNOWN` 内的数字访问或构造初值不构成语义确认。

| 范围 | 类型 / 名称 | 归属与语义 | 证据状态与位置 |
|---|---|---|---|
| `0x000–0x004` | `Entity*` vptr | 本地多态分派；不发送。 | J273 直接：构造器 `0x6A3940` 在 `0x6A396C` 写 Entity vftable。RTTI 见下文。 |
| `0x004–0x014` | 4×`u32` | UNKNOWN；不标填充。构造器写 `+0x08/+0x0C/+0x10=0`；`+0x04` 未见上述初始化。 | J273 直接写入事实；语义 UNKNOWN。构造器 `0x6A3940`。 |
| `0x014–0x018` | `f32` pending-damage buffer | 伤害缓冲量；`TakeDamage` 将新伤害加到此值，`HasMortalDamage` 比较其与当前 HP。属于 Host 伤害结算状态。 | J273 直接：`TakeDamage` 基类 `0x6A8330` 在 `0x6A878A/0x6A8798` 累加写入；getter `0x6A8810` 以 `+0x14 >= +0x380` 返回；Init `0x6A4D92` 清零。Wiki `Entity.md:447–455` 描述缓冲后结算。 |
| `0x018–0x020` | 2×`u32` | UNKNOWN 位域/标记候选；`TakeDamage` 对 `+0x18/+0x1C` 做按位 OR，但其独立位语义未闭合。 | J273 直接：`0x6A8792/0x6A8795`；Init `0x6A4D99/0x6A4DA0` 清零不命名字段。 |
| `0x020–0x024` | `u32 _index` | 本地实体表索引，不是跨进程身份。 | SDK 声明；J273 Init `0x6A4B1D` 清零。`world_layout.json:918`。 |
| `0x024–0x028` | `u32 _collisionIndex` | 本地命中/碰撞索引；不作为网络身份。 | SDK 声明；J273 Init `0x6A4B24` 清零。`world_layout.json:918` 仅前一字段；SDK `Entity.zhl:188`。 |
| `0x028–0x02C` | `u32 _type` | Entity 类型；形态/布局选择字段。 | SDK 声明；J273 Init `0x6A4A35` 写参数。`world_layout.json:919`。 |
| `0x02C–0x030` | `u32 _variant` | Entity 变体；形态/布局选择字段。 | SDK 声明；J273 Init `0x6A4A3B` 写参数。`world_layout.json:920`。 |
| `0x030–0x034` | `u32 _subtype` | Entity 子类型；形态/布局选择字段。 | SDK 声明；J273 Init `0x6A4A41` 写参数。`world_layout.json:921`。 |
| `0x034–0x038` | `u32 _spawnerType` | 生成来源类型。 | SDK 声明；构造器 `0x6A3998` 初始化为 0；非默认写入语义尚未闭合。 |
| `0x038–0x03C` | `u32 _spawnerVariant` | 生成来源变体。 | SDK 声明；构造器 `0x6A399F` 初始化为 0；非默认写入语义尚未闭合。 |
| `0x03C–0x048` | `0x0C` bytes | UNKNOWN。 | SDK 未声明；未将空档认定为填充。 |
| `0x048–0x15C` | `ANM2 _sprite`，`0x114` bytes | 内嵌主 Sprite；其状态、未知字节及资源规则完整见第 2 步 ANM2 表。`Entity+0x48` 是父对象内嵌值，不是 Sprite 指针。 | SDK 声明 `Entity.zhl:202`；J273 Entity 构造器 `0x6A39BB` 调用 ANM2 构造 `sub_402B00(this+0x48)`。 |
| `0x15C–0x160` | `f32 _shadowSize` | 阴影尺寸。 | SDK 声明 `Entity.zhl:195`；Lua/项目映射见 `client_entity_fields.inc`。其余 J273 写入闭包未在本轮复核。 |
| `0x160–0x168` | `2×f32`（候选） | UNKNOWN。Init 分别向 `+0x160`、`+0x164` 写浮点常量；用途未明。 | J273 直接写：`0x6A4AF0`、`0x6A4B0B`；语义 UNKNOWN。 |
| `0x168–0x170` | `u64 _flags` | Entity flags。 | SDK 声明；J273 Init `0x6A4D7B/0x6A4D85` 清完整 64 位。`world_layout.json:922`。 |
| `0x170–0x171` | `bool _valid` | 生命周期有效标记。 | SDK 声明；Entity 构造器 `0x6A39D4` 写 0。 |
| `0x171–0x172` | `bool _visible` | 可见标记。 | SDK 声明；J273 Init `0x6A4BCC` 的 4-byte `0x101` 写入覆盖至 `+0x174`；最终 `+0x171=1`。 |
| `0x172–0x173` | `bool _exists` | 存在标记。 | SDK 声明；同一 `0x6A4BCC` 宽写令 `+0x172=1`。 |
| `0x173–0x174` | `bool _dead` | 死亡标记。 | SDK 声明；同一 `0x6A4BCC` 宽写令 `+0x173=0`。 |
| `0x174–0x175` | `u8` | UNKNOWN。 | SDK 未声明；J273 Init 的宽写会触及该字节。 |
| `0x175–0x176` | `u8` | UNKNOWN；未提升为 `bool` 或插值字段。 | J273 直接（主审复读）：slot 4 `0x6AC320` 写 `+0x175=1`；Init `0x6A4DAE` 清零。仅凭写入值不确定语义。 |
| `0x176–0x177` | `u8` | UNKNOWN。 | J273 Init `0x6A4BD6` 的 word 写清 `+0x176/+0x177`；不据此称填充。 |
| `0x177–0x178` | `bool _removedByFactory` | 仅按项目现行条件读取，不以 Init 的零值推出恒零。 | SDK 声明；J273 Init `0x6A4BD6` 清该位；条件读取规则见 `project-conventions.md` 第 11 节及 `world_layout.json:927`。 |
| `0x178–0x184` | `0x0C` bytes | UNKNOWN。 | SDK 未声明；未认定填充。 |
| `0x184–0x188` | `i32 _entityGridCollisionClass` | Grid 碰撞类别。 | SDK 声明 `Entity.zhl:201`；基类 `IsFlying` 原生 getter `0x6A7C90` 读取此字段并按值 `0..3` 返回。Init 的 `0x6A4BDF` 写的是 `+0x188`，不作为本字段初值证据。 |
| `0x188–0x18C` | `i32 _entityCollisionClass` | Entity 碰撞类别。 | SDK 声明；J273 Init `0x6A4BDF` 写 `+0x188=4`。 |
| `0x18C–0x18D` | `u8` | UNKNOWN。SDK 注释猜测它与 Entity-Entity 碰撞有关，未作为字段名采纳。 | SDK 旁注 `Entity.zhl:203`；J273 Init `0x6A4BE9` 清零。 |
| `0x18D–0x18E` | `u8` | UNKNOWN。 | SDK 未声明；未认定填充。 |
| `0x18E–0x190` | `u16 _entityExtraDataPtrUpperBytes` | 本地 EntityPlus holder 指针高 16 位；不发送。 | SDK 字段声明；REP 源码将 holder 地址的高 16 位写入此处。 |
| `0x190–0x191` | `bool _collidesWithGrid` | 是否与网格碰撞。 | SDK 声明；J273 Init `0x6A4BF0` 清零。 |
| `0x191–0x192` | `u8` | UNKNOWN。 | SDK 未声明；不可并入相邻已知字段复制。 |
| `0x192–0x194` | `u16 _entityExtraDataPtrLowerBytes` | 本地 EntityPlus holder 指针低 16 位；不发送。 | SDK 字段声明；REP 源码与 `+0x18E` 拼为一个 32-bit 指针。 |
| `0x194–0x1A4` | `4×f32`（候选） | UNKNOWN。Init 在 `+0x194/+0x198` 与 `+0x19C/+0x1A0` 分别写 2 个浮点初值；尚未证明它们是可由 Position/Velocity 持续派生的缓存。 | J273 直接：`0x6A4BF7–0x6A4C0F`、`0x6A4C17–0x6A4C2F`；语义 UNKNOWN。 |
| `0x1A4–0x1AC` | `Vector` 屏幕坐标缓存 | 本地：Render 由世界坐标和本地屏幕参数写入，不发送，Client 由原生 Render 重新生成。 | J273 直接（主审复读）：slot 5 `0x6AC4E0` 的 `0x6AC646/0x6AC64A` 写 `+0x1A4/+0x1A8`，`0x6AC64F` 调主 Sprite 渲染；不得用缓存替代基础位置字段。 |
| `0x1AC–0x1B8` | 32-bit `std::vector<ColorParams>` 头，`0x0C` bytes | 容器。头内是本进程 begin/end/capacity 指针；复制 ColorParams 元素为变长容器状态，不复制原指针。 | SDK 声明 `Entity.zhl:208`；REP `LuaEntity.cpp:490–527` 提供枚举、替换、清空元素路径。J273 Update `0x6AA10E–0x6AA22F` 遍历元素并更新其 lifespan，见后文。 |
| `0x1B8–0x1E4` | `ColorMod`，`0x2C` bytes；公开 `Entity.Color` | Entity 自身颜色状态。它在 Entity 上，不是内嵌 Sprite 的 ANM2 颜色。 | J273 公共字段目录 `client_entity_fields.inc:52` 以原始偏移映射；Init `0x6A4AE1` 将 `this+0x1B8` 交给颜色构造/初始化路径。 |
| `0x1E4–0x210` | `ColorMod`，`0x2C` bytes；公开 `Entity.SplatColor` | 死亡溅落颜色状态。 | J273 直接：Init `0x6A4A66–0x6A4ACA` 覆盖 11 个 f32；公共字段目录 `client_entity_fields.inc:53`。 |
| `0x210–0x214` | `i32 _fireDamageCountdown` | 火焰伤害冷却，与 +0x234 Burn 状态倒计时分开。 | SDK 声明；J273 `Entity::Update` `0x6AA396` 仅在值 `>0` 时减一。 |
| `0x214–0x218` | `i32 _damageCountdown` | 受伤计时。 | SDK 声明；J273 `0x6AA3A7` 正值递减。 |
| `0x218–0x21C` | `i32 _freezeCountdown` | 冰冻计时；归零分支清对应状态位。 | SDK 声明；J273 `0x6AA3B8–0x6AA3DC`。 |
| `0x21C–0x220` | `i32 _poisonCountdown` | 中毒状态倒计时；归零分支清位并重置 `+0x320`。 | SDK 声明；J273 `0x6AA3EE–0x6AA426`。 |
| `0x220–0x224` | `i32 _slowingCountdown` | 减速倒计时；正值递减，归零更新 flags。 | SDK 声明；J273 `0x6AA42C–0x6AA470`。 |
| `0x224–0x228` | `i32 _charmedCountdown` | 魅惑倒计时；归零路径涉及 `+0x3C4` 引用。 | SDK 声明；J273 `0x6AA476–0x6AA4DF`。 |
| `0x228–0x22C` | `i32 _confusionCountdown` | 混乱倒计时；归零分支清对应 flags。 | SDK 声明；J273 `0x6AA4E3–0x6AA527`。 |
| `0x22C–0x230` | `i32 _midasFreezeCountdown` | 金化冻结倒计时。 | SDK 声明；J273 `0x6AA646–0x6AA677`。 |
| `0x230–0x234` | `i32 _fearCountdown` | 恐惧倒计时。 | SDK 声明；J273 `0x6AA52B–0x6AA556`。 |
| `0x234–0x238` | `i32 _burnCountdown` | 燃烧状态倒计时。 | SDK 声明；J273 `0x6AA5B7–0x6AA5D6`。 |
| `0x238–0x23C` | `i32 _bossStatusEffectCooldown` | Boss 状态效果冷却。 | SDK 声明；J273 `0x6AA9DC–0x6AA9E7` 正值递减。 |
| `0x23C–0x240` | `i32 _shrinkCountdown` | 缩小效果倒计时；到期调 `sub_6B1B90`。 | SDK 声明；J273 `0x6AA58F–0x6AA5A6`。 |
| `0x240–0x278` | `0x38` bytes | UNKNOWN。 | SDK 未声明；尚未以完整 J273 读写闭包消解。 |
| `0x278–0x27C` | `f32 _poisonDamage` | 中毒伤害数值。 | SDK 声明；REP Lua 接口按该成员读取/写入；J273 公开映射尚未在本轮逐点复核。 |
| `0x27C–0x280` | `f32 _burnDamage` | 燃烧伤害数值。 | SDK 声明；REP `LuaEntity.cpp` 设置接口；J273 公开映射尚未在本轮逐点复核。 |
| `0x280–0x2B0` | `0x30` bytes | UNKNOWN。 | SDK 未声明；尚未消解。 |
| `0x2B0–0x2B4` | `f32 _depthOffset` | 仅能确认 SDK 声明了一个 float；**不是**公开 `Entity.DepthOffset` 的已核位置。不得映射为公开 DepthOffset。 | SDK 声明 `Entity.zhl:223`；公开绑定的 J273 偏移另在 `+0x358`，见下表。 |
| `0x2B4–0x2D4` | `0x20` bytes | UNKNOWN。 | SDK 未声明；未认定填充。 |
| `0x2D4–0x2D8` | `f32 _mass` | Entity 质量。 | SDK 声明；Client 公共字段按 SDK 成员映射。 |
| `0x2D8–0x2E0` | `2×u32` | UNKNOWN。 | SDK 未声明。 |
| `0x2E0–0x2E4` | `i32 _poisonDamageTimer` | 中毒伤害节奏计数器；符合更新条件时递增，不是剩余倒计时。 | SDK 声明；J273 Update `0x6AAE03–0x6AAE17`。`flags & 0x40` 路径每次递增，常量 20 取模余数为 1 时触发伤害路径。 |
| `0x2E4–0x2E8` | `i32 _burnDamageTimer` | 燃烧伤害节奏计数器；符合更新条件时递增，不是剩余倒计时。 | SDK 声明；J273 Update `0x6AAECD–0x6AAEDD`。`flags & 0x1000` 路径每次递增，常量 20 取模余数为 1 时触发伤害路径。 |
| `0x2E8–0x2EC` | `i32 _bleedingCountdown` | 流血倒计时；到期路径清状态位。 | SDK 声明；J273 `0x6AA67D–0x6AA6A5`。 |
| `0x2EC–0x2F0` | `i32` | UNKNOWN。流血到期时写 0，但用途未明；不能作为 padding。 | J273 直接写 `0x6AA6A5`；语义 UNKNOWN。 |
| `0x2F0–0x2F4` | `i32 _magnetizedCountdown` | 磁化倒计时。 | SDK 声明；J273 `0x6AA77E–0x6AA7B6`。 |
| `0x2F4–0x2F8` | `i32 _baitedCountdown` | 诱饵倒计时；归零后遍历受影响对象。 | SDK 声明；J273 `0x6AA7C2–0x6AA8C7`。 |
| `0x2F8–0x2FC` | `i32 _knockbackCountdown` | 击退倒计时；到期清对应标志。 | SDK 声明；J273 `0x6AA926–0x6AA957`。 |
| `0x2FC–0x300` | `i32 _weaknessCountdown` | 虚弱倒计时。 | SDK 声明；J273 `0x6AA8C7` 起的正值递减路径。 |
| `0x300–0x304` | `i32 _iceCountdown` | 冰冻效果倒计时。 | SDK 声明；J273 `0x6AA95D–0x6AA9A1`。 |
| `0x304–0x308` | `i32 _brimstoneMarkCountdown` | Brimstone 标记倒计时。 | SDK 声明；J273 `0x6AA9A7–0x6AA9D6`。 |
| `0x308–0x310` | `Vector _knockbackDirection` | 击退方向值。 | SDK 声明 `Entity.zhl:234`；已存在公共字段映射。 |
| `0x310–0x314` | `Entity*` 附属 owner 候选 | UNKNOWN，不发送。已有 ClearReferences 证据提示附属对象，但精确生命周期／用途尚未闭合，主审未提升为已核本地字段。 | `Entity::ClearReferences 0x6A8060` 相关引用操作；布局源保守保留 UNKNOWN。 |
| `0x314–0x31C` | `Vector` pause-held position | 状态：暂停位置路径保存的坐标，不能仅由当前 Position 派生。 | 主审补证：`0x6C089C/0x6C08A2` 取 +314 写回 +33C，`0x6C08A8/0x6C08AE` 取 +318 写回 +340；条件为 pause timer 递减后仍正。Init `0x6A4E87` 初始化该 Vector。 |
| `0x31C–0x320` | `i32 _pauseTimer` | 移动/位置更新路径中的暂停计时器；只在正值且到达 staged-position 分支时递减，不据此构造全局截止帧。 | SDK 声明 `Entity.zhl:235`；J273 `sub_6C03B0` 在 `0x6C0891/0x6C0892` 正值递减；剩余值仍正时把 staged `+0x314/+0x318` 写回 Position `+0x33C/+0x340`（`0x6C0898–0x6C08AE`）。入口受 `+0x16C` 位 `0x02000000` 及 staged-position 检查分支控制，见倒计时结论。 |
| `0x320–0x328` | `2×f32`（候选 Vector） | UNKNOWN。Init 将两分量写为 `2.0f`；语义未明。 | J273 `0x6A4C9F/0x6A4CA9`。 |
| `0x328–0x32C` | `i32` FrameCount 基准 | 状态帧锚点；发送原值。`FrameCount = Game+0x25A70 - Entity+0x328`。 | J273 getter `0x6A5290`：`0x6A5295` 读 `Game+0x25A70`，`0x6A529B` 减 `[this+0x328]`。Init `0x6A4B2B/0x6A4B31` 从 Game 复制帧值。Client 现存实现 `client_entity_common_native.cpp:51–56` 与公式相同。 |
| `0x32C–0x330` | `i32` `SpawnGridIndex` | 生成所在的房间格索引；无生成格时保留 `-1`。 | J273 注册 `0x84F4F2/0x84F4F7`；getter `0x6EB980` 的 `mov eax,[ecx+32Ch]`；Init `0x6A4B46` 写 `-1`。Wiki `IsaacDocs/docs/Entity.md:863–866`。 |
| `0x330–0x334` | `EntityConfigEntity*` selected config resource | 本地配置资源指针；不复制地址。J273 `GetBossID` 由该对象读取 bossID；这和 REP 新增 `GetEntityConfigEntity` 按 Type/Variant/SubType 查询 Manager 是两条不同路径。 | J273 getter `0x7DF5F0` 读取 `[this+0x330]`；非空时返回 `[config+0x30]`，空时返回 0。REP `LuaEntityConfig.cpp:9–24` 的 manager lookup 不证明本字段。 |
| `0x334–0x33C` | `Vector _targetPosition` | 目标位置值。 | J273 项目字段声明 `client_entity_fields.inc`；该范围不含指针。SDK 未提供等价具名字段。 |
| `0x33C–0x344` | `Vector _pos` | 逻辑位置。 | SDK 声明；J273 Init 在 `0x6A4E37/0x6A4E45` 将位置拷入 `+0x38C/+0x390`。 |
| `0x344–0x34C` | `Vector` staged position | 插值前暂存／历史位置；归状态并发送，不可按当前 `_pos` 派生替代。 | J273 slot 4 `0x6AC2E8/0x6AC2F1` 将 `+0x33C/+0x340` 写至此；与 `+0x175` 插值标志配合。 |
| `0x34C–0x354` | `Vector _posOffset` | Entity 位置偏移。 | SDK 声明；Client公共映射使用该成员。 |
| `0x354–0x358` | `i32` `RenderZOffset` | 公开 RenderZOffset。 | J273 Init `0x6A4BAE` 写 0；公开字段目录以 raw offset 映射。 |
| `0x358–0x35C` | `f32` `DepthOffset` | 公开 DepthOffset；不是 SDK `_depthOffset` `+0x2B0`。 | J273 Init `0x6A4BB8` 写 0；`client_entity_fields.inc:5–6`。 |
| `0x35C–0x360` | `i32` `SortingLayer` | 绘制排序层。 | J273 Init `0x6A4BC2` 写 2；公开字段目录 raw offset。 |
| `0x360–0x368` | `Vector _velocity` | Entity 速度。 | SDK 声明；J273 Init `0x6A4E51/0x6A4E5D` 将其复制至 `+0x394/+0x398`。 |
| `0x368–0x36C` | `f32` | UNKNOWN。Init 将此值复制到 `+0x36C`，尚不能确认其名或是否为 friction 的权威源。 | J273 `0x6A4AE7/0x6A4B00`。 |
| `0x36C–0x370` | `f32` `Friction` | 公开摩擦力。 | J273 Init `0x6A4B00` 从 `+0x368` 拷入；`client_entity_fields.inc:46`。 |
| `0x370–0x374` | `f32 _size` | Entity 尺寸。 | SDK 声明。 |
| `0x374–0x37C` | `Vector` `SizeMulti` | 碰撞胶囊 X/Y 比例。 | J273 `client_entity_fields.inc:47` raw mapping。 |
| `0x37C–0x380` | `u32` | UNKNOWN。 | SDK 未声明。 |
| `0x380–0x384` | `f32` `HitPoints` | 公开当前生命值。 | J273 Init `0x6A4B37/0x6A4B3D` 将 `+0x384` 拷贝到 `+0x380`；Lua/raw 字段表映射。Wiki说明受伤会下一帧结算，不把此字段当作 pending damage 缓冲。 |
| `0x384–0x388` | `f32` `MaxHitPoints` | 公开最大生命值。 | J273 Config路径 `0x6A500F` 写入；Client raw字段映射。 |
| `0x388–0x38C` | `f32 _collisionDamage` | Entity 碰撞伤害。 | SDK 声明。 |
| `0x38C–0x39C` | `4×f32` | UNKNOWN / 位置速度历史缓存候选。Init 复制 `_pos.x/y` 到 `+0x38C/+0x390`、`_velocity.x/y` 到 `+0x394/+0x398`；未证后续始终相等。 | J273 `0x6A4E37–0x6A4E63`；静态推导仅限初始化关系。 |
| `0x39C–0x3A0` | `f32 _speedMultiplier` | 速度倍率公开值。 | SDK 声明；J273 Update 在 `0x6A9D41` 读取。 |
| `0x3A0–0x3A8` | `2×f32` | UNKNOWN，参与速度倍率/渲染更新的内部状态候选；不因与倍率同时访问而命名为缓存。 | J273 Update `0x6A9D37–0x6A9D5E` 读写 `+0x3A0/+0x3A4`；构造/Init写入尚未闭合。 |
| `0x3A8–0x3B0` | `2×u32` | UNKNOWN。 | SDK 未声明。 |
| `0x3B0–0x3B1` | `bool _isInvicible` | 无敌状态。 | SDK 声明；J273 Init `0x6A4DA7` 清零。 |
| `0x3B1–0x3B4` | `3B` | UNKNOWN；不称作填充。 | SDK 未声明。 |
| `0x3B4–0x3B8` | `f32 _shrinkStatus1` | 缩小效果内部状态。 | SDK 声明；J273 Update 在 `0x6AA587` 写入。 |
| `0x3B8–0x3BC` | `f32 _shrinkStatus2` | 缩小效果内部状态。 | SDK 声明。 |
| `0x3BC–0x3C0` | `EntityPtr _parent` | 对父实体的引用；客户端须按 EntityPtr 规则维护引用关系，不能 memcpy 地址。 | SDK 声明；构造器 `0x6A3DF8`、Init `0x6A4DD3` 清零。 |
| `0x3C0–0x3C4` | `EntityPtr _child` | 子实体引用。 | SDK 声明；构造器 `0x6A3E02`、Init `0x6A4DDD` 清零。 |
| `0x3C4–0x3C8` | `EntityPtr _target` | 目标实体引用。 | SDK 声明；构造器 `0x6A3E0C`、Init `0x6A4DE7` 清零。 |
| `0x3C8–0x3CC` | `EntityPtr _spawnerEntity` | 生成者引用。 | SDK 声明；EntityPtr 引用闭包。 |
| `0x3CC–0x3D0` | 本地反向引用集合根/指针 | 指向堆上循环链的 root/sentinel；节点不在 Entity 内嵌。只维护本地 `EntityPtr` 关系，不发送。 | J273 构造器 `0x6A3E32` 分配 `0x14` 字节节点并在 `0x6A3E42–0x6A3E50` 建链、存入 `+0x3CC`；`ClearReferences 0x6A8060` 遍历/清理该集合。 |
| `0x3D0–0x3D4` | `u32` local control word | UNKNOWN；可能是该集合的计数/控制字段，但本轮不能证明 count 语义；不标 padding。 | 构造器初始化为 0，其他访问需进一步闭合；不作为公开字段。 |
| `0x3D4–0x3D8` | `EntityPtr _minecart` | Minecart 实体引用。 | SDK 声明；Entity Init 对 `+0x3D4` 做引用解绑后清零。 |
| `0x3D8–0x3DC` | `u32` | UNKNOWN；Init 做解绑检查后清零，语义/是否另一 EntityPtr 未闭合。 | J273 `0x6A4E0F–0x6A4E31`。 |
| `0x3DC–0x3EC` | `RNG _dropRNG`，`0x10` bytes | Host 权威随机状态；4×u32，不向 Client 复制。 | SDK 声明；构造器 `0x6A3E7B/0x6A3E6B/0x6A3E73` 与 Init `0x6A4A4D–0x6A4A60` 写四字。 |
| `0x3EC–0x3F0` | `u32 _initSeed` | 初始种子状态。 | SDK 声明；J273 Init `0x6A4A47` 写参数。 |
| `0x3F0–0x408` | `0x18` bytes | UNKNOWN。Update 会读写 `+0x3F0` 的正值计时/状态，其他字节的语义尚未闭合；不能把整段并为一个指针或填充。 | J273 Init `0x6A4DB5` 写 `+0x3F0=0`；Update `0x6A9D68`、`0x6AA9ED–0x6AAA02` 访问该成员。 |
| `0x408–0x40C` | `LuaBridgeRef* _luaBridgeRef` | 进程内 Lua registry 引用；不发送。 | SDK 声明；J273 Init `0x6A4EEE–0x6A4F16` 读取、释放并置空。 |
| `0x40C–0x410` | `u32` | UNKNOWN；不并入 4-byte 指针，也不标填充。 | Entity extent 内剩余字节；本轮未闭合。 |

## 直接核实的错位与 SDK 限制

- **FrameCount**：Lua 注册表 `0x84F380–0x84F38A` 将 `FrameCount` 绑定到 getter `0x6A5290`。其原始指令为 `0x6A5290 mov eax,n187`、`0x6A5295 mov eax,[eax+25A70h]`、`0x6A529B sub eax,[ecx+328h]`。这确认 `+0x328` 是 Host Game 帧空间的基准，不是逐帧递减倒计时。Entity Init `0x6A4A20` 的 `0x6A4B2B/0x6A4B31` 初始化该基准为当前 `Game+0x25A70`。
- **SpawnGridIndex**：原始 getter `0x6EB980 mov eax,[ecx+32Ch]`；下一条 `0x6EB986 retn`。不能把 `+0x32C`误用成 FrameCount 或 UNKNOWN。Init `0x6A4B46` 写 `-1`。Wiki含义是生成该Entity时所在房间网格索引。
- **DepthOffset**：官方 SDK 的 `_depthOffset +0x2B0` 目前只作为 SDK 声明保留；J273公开字段表 `client_entity_fields.inc` 将公开 `Entity.DepthOffset` 映射至 `+0x358`。`Entity.Init` 在 `0x6A4BB8` 初始化 `+0x358`。`+0x2B0` 不得替代 `+0x358`。
- **其他公开 raw offsets**：J273 Init `0x6A4BAE` 写 `RenderZOffset +0x354=0`，`0x6A4BC2` 写 `SortingLayer +0x35C=2`；`+0x36C` Friction，`+0x374` SizeMulti，`+0x380/+0x384` HP/MaxHP，均在 `client_entity_fields.inc` 使用 raw offset，因为 SDK 没有对应具名声明。初始写入不等价于字段完整读写闭环。
- **SDK inline 动画帮助器冲突**：`Entity.zhl` inline `GetAnimationState()` 返回 `this+0x6C`；第 2 步 J273 ANM2 布局在 Entity+0x48 内，main `AnimationState` 位于 ANM2+0x30，即 Entity+0x78。该旧 inline helper 不作为 J273 内嵌对象事实；主 Sprite 各字段以第 2 步目标二进制证据为准。

## EntityPlus 扩展指针及可选字段

### 指针链

REPENTOGON 1.1.3 `repentogon/Patches/EntityPlus.cpp:18–60` 定义：Entity 中保存的是指向 `EntityPlusHolder` 的指针，holder 内是 `std::unique_ptr<EntityPlus> data`。32-bit 地址由 `upper<<16 | lower` 拼合，其中高半部位于 Entity `+0x18E`，低半部位于 `+0x192`。这两段是本地句柄字节，绝不可复制到另一进程。

`GetEntityPlus(entity)` 调 `GetEntityPlusHolder(entity,true)`。若两个半字都为 0，该路径会记录日志、分配新 holder 并改写 Entity；因此只读采集不可调用这个 API。应该只读取两个分段、拼出已存在的 holder，再只读 `holder->data`；缺失 holder/data 必须保持缺口/null，不能在采集路径初始化或创建它。

### EntityPlus 公共基类

| REP 字段 | 当前 Win32 ABI | 用途/映射 |
|---|---|---|
| vptr | `+0x00..04` | 本地多态指针，不发送。 |
| `optional<bool> isFlyingOverride` | `+0x04`，size 2；payload `+0x04`，`_Has_value +0x05` | nullable bool；null 表示回退到原生 IsFlying。 |
| 对齐候选未知区 | `+0x06..08` | 不发送；布局源保守归 UNKNOWN，不宣称已核无语义。 |
| `optional<uint32_t> waterClipInfoFlagsOverride` | `+0x08`，size 8；payload `+0x08..0C`，`_Has_value +0x0C` | nullable u32；仅 engaged 时读取 payload。 |
| 尾部对齐候选未知区 | `+0x0D..10` | 不发送；布局源保守归 UNKNOWN。 |

官方声明见 `EntityPlus.h:10–17`；当前 Win32/v143 ABI 由 `mod/src/game/client_player_sdk_abi.cpp` 对应翻译单元的 `static_assert` 核定：`sizeof(optional<bool>)==2`、`sizeof(optional<uint32_t>)==8`，Has_value 位于 `+1/+4`。EntityPlus **不是**可直接 memcpy 的线上结构：编码presence与有效payload，并忽略本地 vptr/padding/disengaged payload。

### 公共 getter 的确切含义

- `IsFlying`：REP `EntityPlus.cpp:152–158` 在 `isFlyingOverride.has_value()` 时返回覆盖值，否则调用 `super()`。J273 Lua绑定表 `0x84F4CA–0x84F4CF` 绑定原生 `0x6A7C90`；该 getter 只读 `+0x184`，返回 grid collision class 属于 `0,1,2,3`。因此结果可由 `+0x184` 和 nullable override 合成，不能只同步 effective bool 丢掉 null/override 状态。
- `GetWaterClipFlags`：J273/SDK没有 Entity 原生 flags 字段。REP `EntityPlus.cpp:163–169` 先调用 vanilla `GetWaterClipInfo(out)`，再在 optional override engaged 时替换 `out->bitFlags`；`LuaEntity.cpp:814–839` 的 getter取最终 bitFlags，setter设置 optional，Reset 设为 nullopt。vanilla `GetWaterClipInfo` 的 J273 实现由 `Entity.zhl:113–114` 的函数签名唯一匹配到 `0x6B2F70`（RVA `0x2B2F70`）；其分支读取 Type/Variant/SubType (`+0x28/+0x2C/+0x30`)，并使用位置偏移、HP、内嵌动画、`IsFlying` 及 Game/global 条件，构造 8-byte `WaterClipInfo`。它不是单一存储字段；REP 只覆盖其 `bitFlags`。可靠的扩展输入是 nullable override；`nullopt` 必须保留为“继续使用 vanilla 公式”。本轮未把所有 type-specific 分支简化成新的客户端公式。
- `GetEntityConfigEntity`（REP新增）与 `GetBossID` 使用不同路径：前者 `LuaEntityConfig.cpp:9–24` 以 Type/Variant/SubType 查 Manager 配置表；后者 `0x7DF5F0` 取 Entity +0x330 实际 selected config，非空时取配置 +0x30，否则 0。+0x330 转稳定资源序号，不发送地址。初稿推测“同形态查表可直接得到 BossID”，主审未采纳：尚未证明重查结果与实际 selected config 恒等，必须使用该指针解析的资源身份。
- `HasFullHealth`：注册 `0x84F498/0x84F49D`；`0x6A52B0` movss 读取 HP，`0x6A52B8` comiss 比较 MaxHP，`0x6A52BF` setnb，`0x6A52C2` 是 ret，语义为 HP≥MaxHP。
- `HasMortalDamage`：注册 `0x84F6DC/0x84F6E1`；`0x6A8810` movss 读取 pending-damage +0x14，`0x6A8815` comiss 比较 HP +0x380，`0x6A881C` setnb，`0x6A881F` 是 ret。TakeDamage `0x6A8330` 的 `0x6A878A/0x6A8798` 累加新伤害到 +0x14。缓冲属于 Host 伤害结算，不能由 HP 单独推导。
- `GetLastChild/GetLastParent`：注册 `0x84F394/0x84F399`、`0x84F39E/0x84F3A3`；handlers `0x6A4030/0x6A4010` 首读 +0x3C0/+0x3BC，循环读取位于 `0x6A4042/0x6A4022`。初稿的 `0x6A4038/0x6A4018` 是 jz，主审已纠正。跟随至空后返回末端；首边为空返回自身，与 Wiki 无关系返回 nil 冲突，保留差异。
- `CanShutDoors/IsBoss`：J273 Lua thunks `0x8ABF4B/0x8ABF14` 虚调用 Entity vtable `+72/+76`；Entity 基类槽目标 `0x5CC1C0` 返回 false。子类可覆写，不能只以公共基类状态字段重建它们。

### 派生类扩展（供后续步骤归属）

以下均在 EntityPlus holder 的动态对象中，不属于 Entity `[0,0x410)`，亦无稳定 `EntityPlus` 基类之后的固定偏移可跨实体类型复用：

- `EntityPlayerPlus`：`ItemSpoofSystem::PlayerItemSpoofs`、`customCacheTags`、`customCacheResults`、`customCacheRequiresEvaluateItemsCall`、`previousHealthType`、`evaluatingHealthType`、`disableHealthTypeModification`、`camoOverride`、`previousControllerIndex`、`restoreTwinID`。
- `EntityFamiliarPlus`：`optional<float> cachedMultiplier`、`uintptr_t cachedMultiplierPlayer`。
- `EntityTearPlus`：`optional<uint32_t> initSound`。
- `EntityPickupPlus`：`optional<bool> canRerollOverride`。
- `EntityLaserPlus`：`bool recalculateSamplesNextUpdate`、`optional<uint32_t> initSound`。
- `EntityKnifePlus`：`EntityPtr hitboxSource`。

来源均为 `repentogon/Patches/EntityPlus.h:19–64`；字段具体归属由第 4/5 步决定。

## 倒计时与帧锚点结论

J273 Entity 基类 Update VA `0x6A9CF0` 经过 Entity flags、状态及非插值更新分支；它不能视为所有 Entity 每个 Game frame 都无条件执行。普通状态 countdown 的所列路径仅在值 `>0` 时递减，零和负哨兵在这些路径原样保持；部分到零分支会清 flags、触发效果或引用副作用。对这类计时器，未核定更新资格前不把 `global_frame + countdown` 宣称为稳定到期锚点。

J273 已核的正值递减点：`+0x210 0x6AA396`、`+0x214 0x6AA3A7`、`+0x218 0x6AA3B8`、`+0x21C 0x6AA3EE`、`+0x220 0x6AA42C`、`+0x224 0x6AA476`、`+0x228 0x6AA4E3`、`+0x22C 0x6AA646`、`+0x230 0x6AA52B`、`+0x234 0x6AA5B7`、`+0x23C 0x6AA58F`、`+0x238 0x6AA9DC`、`+0x2E8 0x6AA67D`、`+0x2F0 0x6AA77E`、`+0x2F4 0x6AA7C2`、`+0x2F8 0x6AA926`、`+0x2FC 0x6AA8C7`、`+0x300 0x6AA95D`、`+0x304 0x6AA9A7`。`+0x2EC` 在 bleeding 到期处被写 0，但该字仍 UNKNOWN。

特殊计时字段使用不同语义，不能并入上述正值倒计时：

| 字段 | J273 更新证据 | 可安全推导的时间语义 |
|---|---|---|
| `+0x2E0` poison / `+0x2E4` burn damage cadence | `Update 0x6A9CF0`：满足其前置 Entity flags/global/type 分支后，`flags & 0x40` 路径在 `0x6AAE03–0x6AAE17` 对 `+0x2E0` 加一；`flags & 0x1000` 路径在 `0x6AAECD–0x6AAEDD` 对 `+0x2E4` 加一。两处以常量 20 做有符号除法，余数等于 1 才进入相应伤害调用路径。 | SDK字段名 `Timer` 容易误读；目标二进制实现是 signed count-up 计数器，而非剩余计时。更新资格和 Entity Update 次数决定节奏；不能以 Game global frame + remaining 换算。保留原值并按有符号 32-bit 增量锚定。 |
| `+0x31C` pause timer | `sub_6C03B0` 在 `0x6C0810` 检查该值及 Entity flags `+0x16C & 0x02000000`；staged-position 分支经 `+0x314/+0x318` 检查后，只对正值在 `0x6C0891/0x6C0892` 递减。若递减后仍正，将 staged vector 写回 Position `+0x33C/+0x340`（`0x6C0898–0x6C08AE`）。 | 在位置更新/暂停路径内递减，不是全局帧到期锚点；零和负值不变。将位置快照和该计数器一起保真，不能用当前 Position 派生 staged vector。 |
| `ColorParams.duration1 +0x30` / `duration2 +0x34` | Entity Update `0x6AA122` 读取 `+0x34`，`0x6AA12A` 无条件 `dec`，`0x6AA12E` 写回；`0x6AA13B` 只在递减结果为 0 时移除该元素。`+0x30` 在 `0x6AA1E2` 读取作为 fadeout 比例分母，没有递减写入。颜色更新在 Entity flags low dword `+0x168 & 0x4000` 时跳过（`0x6AA0FC–0x6AA108`）。 | 官方 ColorParams 文档区分 Duration（fadeout比率分母）与 Lifespan（剩余 update 帧数，30/s）。J273 仅递减 Lifespan `+0x34`，负值仍继续递减，且不 clamp；只有恰好变 0 才删项。必须保留 signed 32-bit 原始值及非插值更新 tick；不可将负值按哨兵冻结。 |

ColorParams 来源：REP `docs/docs/ColorParams.md:27–38`；成员名到偏移由[基础类型证据](base-types-evidence.md)核对。Update 的 `0x6AA61C/0x6AA721` 还将 `Game+0x25A70 - Entity+0x328` 用于某些状态效果的周期性资源/颜色选择；这只证明相关效果路径引用 FrameCount，不证明其他计数器与 global frame 一比一。对公开 `FrameCount` 保存原 `+0x328`，Client 展示时以最近 world frame 还原。

## 查询/公开成员回映射的边界

`client_entity_fields.inc` 含 57 条公共常规/特殊写入声明；`Entity.IsVisible` 是 `Visible` 的另一公开名，字段目录本身不覆盖所有原生字节。`entity-application.json` 含 90 行 Entity 公开责任，`Entity.Color` 与 `Entity.Type` 有重复 ID，故是 88 个唯一 ID。该目录责任不能自动推成原生字段：

- `Entity.FrameCount` → 原生 `+0x328` 锚点；由 J273 getter 精确确认。
- `Entity.SpawnGridIndex` → 原生 `+0x32C`；由 J273 getter 精确确认。
- `Entity.DepthOffset` → `+0x358`；SDK 的 `+0x2B0` 只保留 SDK 状态。
- `Entity.GetWaterClipFlags` → EntityPlus nullable override；不是 Entity 原生字段，也不能只保留 getter 有效值。
- `Entity.HasFullHealth` → `+0x380 >= +0x384`，J273 getter直比。
- `Entity.HasMortalDamage` → pending buffer `+0x14 >= +0x380`；`+0x14` 由 `TakeDamage` 累加，按 Host 伤害结算状态处理。
- `Entity.GetBossID` → selected config pointer +0x330 指向对象的 +0x30；以该指针解析的稳定资源身份为准，不假定同形态重查恒等。
- `Entity.GetLastChild/GetLastParent` → 沿 child `+0x3C0` / parent `+0x3BC` 引用链取末端；原生无链接返回自身，Wiki对nil行为有冲突。
- Entity parent/child/target/spawner/minecart → `EntityPtr` 关系，须转成员身份并维护引用链，不能传地址。
- `Entity.GetDropRNG`/`DropSeed` → Host 权威 RNG 状态/查询；Seed 不是额外副本字段。
- `GetEntityConfigEntity` → REP Config manager 以 Type/Variant/SubType 查询，不是 Entity 内存指针。

其他“参数化玩法查询”和按 Entity 子类虚函数定义的查询，仍应依据其目标/配置依赖逐项回映；本表不把未知查询结果默认映射成派生状态，也不因其未知而宣称已闭合。

## RTTI 与关键 J273 RVA

Entity vftable VA `0xB18224`（RVA `0x718224`）前一槽 `0xB18220` 为 COL 指针 `0xB439E4`（RVA `0x7439E4`）。COL 的 `+0x0C` 指向 TypeDescriptor `0xBD1244`（RVA `0x7D1244`）；TypeDescriptor 名字符串从 `0xBD124C` 开始，为 `.?AVEntity@IsaacRepentancePlus@@`。因此 `+0xBD1244` 才是 descriptor，`+0xBD124C` 仅是名字，不可混为一址。

| 内容 | VA / RVA | 已核事实 |
|---|---|---|
| Entity vftable、COL | `0xB18224 / 0x718224`；COL `0xB439E4 / 0x7439E4` | vftable 前一 DWORD 指向 COL。 |
| Entity TypeDescriptor | `0xBD1244 / 0x7D1244` | COL `+0x0C` 指向这里。 |
| Entity decorated name | `0xBD124C / 0x7D124C` | TD header 后 8 字节开始的名字串。 |
| ctor/init | `0x6A3940 / 0x2A3940`; `0x6A4A20 / 0x2A4A20` | vtable写入、ANM2内嵌构造、初始化 writes。 |
| FrameCount | `0x6A5290 / 0x2A5290`; bindings `0x84F385` | 返回 Game frame minus `this+0x328`。 |
| SpawnGridIndex | `0x6EB980 / 0x2EB980`; binding `0x84F4F2–0x84F4F7` | 读 `[ecx+0x32C]`；`0x6EB986` 是 ret。 |
| IsFlying | `0x6A7C90 / 0x2A7C90`; binding `0x84F4CA–0x84F4CF` | 基类仅读 `+0x184`，REP optional override 优先。 |
| HasFullHealth / HasMortalDamage | `0x6A52B0 / 0x2A52B0`; `0x6A8810 / 0x2A8810` | HP/maxHP直比；pending buffer `+0x14` 与 HP 比较。 |
| GetLastChild / GetLastParent | `0x6A4030 / 0x2A4030`; `0x6A4010 / 0x2A4010` | 沿 `+0x3C0/+0x3BC` 链遍历，空链返回 this。 |
| GetBossID | `0x7DF5F0 / 0x3DF5F0`; binding `0x84F3E4/0x84F3E9` | `0x7DF5F0` 读 Entity `+0x330`；`0x7DF5F8` 判断非空；`0x7DF5FA` 读 config `+0x30`，否则 `0x7DF5FE` 返回 0。 |
| Vanilla GetWaterClipInfo | `0x6B2F70 / 0x2B2F70` | SDK签名定位的原生公式；REP nullable override仅替换输出 flags。 |
| Entity::Update timers | `0x6A9CF0 / 0x2A9CF0` | 含正值倒计时、无夹限ColorParams Lifespan递减、poison/burn计数器递增。 |
| Pause timer position path | `0x6C03B0 / 0x2C03B0` | `+0x31C` 正值分支于 `0x6C0891/0x6C0892` 递减。 |
| SpawnGridIndex init | `0x6A4B46 / 0x2A4B46` | `+0x32C=-1`。 |
| EntityPlus split pointer | `Entity+0x18E/+0x192` | 官方 REP source 证明两半指向 holder，不是直接 EntityPlus。 |

## 主审补证与保留的纠错

- WaterClip 的 `0x6B2F70` 已由主智能体独立反编译；`0x6B303C` 读取 SortingLayer +0x35C，`0x6B2FB5` 读取 SubType +0x30，另用 SpawnerType +0x34、PositionOffsetY +0x350、HP +0x380、IsFlying。Pickup +0x534、Player 道具 62 查询、Game+0x18300 的 Room helper 0x81CB10 分别由第 4／5／7／8 步提供；动画叠加状态用于完整 WaterClipInfo 的分支。新回映射明确这些依赖，不声称当前独立公式已可运行。
- 主审没有仅凭构造清零把 +0x3D0 认成计数，保留 UNKNOWN；+0x175 也未升格为 bool。+0x314/+0x318 则有向 Position 回写的直接证据，作为独立保存坐标纳入状态。
- 研究首报曾将 FrameCount 配为 0x6A4280，已按登记顺序与原始指令更正为 0x6A5290；SpawnGridIndex 的读取指令是 0x6EB980，0x6EB986 为 ret。历史 tx-03 的 slot 4／5 地址属于旧点位，本轮采用当前虚表的 0x6AC2A0／0x6AC4E0。
- 首版把 +0x408—0x410 整段视为 LuaBridgeRef 指针、把 0x6A4BDF 的 +0x188 写入归给 +0x184，已更正为 4B 指针和准确目标；+0x40C—0x410 继续未知。反编译注释行号也不能代替实际访存指令，健康查询与链遍历地址已按原始指令订正。

## 未闭合项目

- `+0x004–0x014`、`+0x018–0x020`、`+0x194–0x1A4`、`+0x240–0x278`、`+0x280–0x2B0`、`+0x2B4–0x2D4` 等 UNKNOWN 仍需以后续调用点闭合；不影响本步标为 UNKNOWN。
- `+0x3F0` 是正值递减状态（Update `0x6AA9ED–0x6AA9FC`），但设置方、语义及全部资格条件未闭合，不赋予计时器名称。
- `+0x31C` 已知在 `sub_6C03B0` 位置更新路径递减；该函数全部调用资格尚未闭合，不能当 Game-frame 锚点。
- `GetWaterClipInfo` 原生 `0x6B2F70` 依赖 type/variant/subtype 和额外 Entity/Game 状态；尚未整理成等价的独立实现。EntityPlus nullable override 布局与覆盖语义已核实。
- `CanShutDoors/IsBoss` 基类虚槽返回 false，实际派生类覆盖表未闭合；其余参数化/按类覆盖查询按第 5 步来源清单继续处理。

本轮仅静态阅读本地 SDK、官方 REPENTOGON 1.1.3 源码/wiki 与 IDA J273；没有运行游戏、实验或测试，没有修改/保存 IDB，没有进行哈希校验。该说明只记录本证据作者完成的静态范围。
