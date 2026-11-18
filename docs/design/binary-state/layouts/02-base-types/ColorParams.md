# ColorParams

由唯一布局源生成。布局编号 11；布局版本 84；原生大小 0x3C；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本32：整个ColorParams时间线留Host，布局11只保留原生字节归属与空块，不发送元素。J273 Entity::Update VA0x6AA10E—0x6AA22F计算覆盖／淡出，并在0x6AA221调用0x4029B0写主Sprite+0xA8最终颜色；Client不运行该Update。Duration分母与Lifespan剩余次数仍不同，负Lifespan仍递减这一原生事实不变；不再需要颜色锚点／读取时钟／容器writer。未知+3A/+3B仍为2B，ColorMod本身的11个分量继续完整同步。

原生字节归属：

| 0x0 i32 priority【权威】颜色覆盖优先级；仅Host合成队列消费，Client接收最终颜色（4B；已核） | 0x4 ColorMod color【权威】覆盖输入ColorMod；仅Host合成，不能与最终Entity.Color／ANM2.color混同（44B；已核） | 0x30 i32 duration【权威】Duration／+30，淡出比例分母；原生颜色更新不递减，仅Host消费（4B；已核） | 0x34 i32 lifespan【权威】Lifespan／+34，每次颜色更新有符号减一，负值也递减、仅结果0删项；留Host，不再编码颜色锚点（4B；已核） | 0x38 bool fadeout【权威】是否按Lifespan／Duration淡出；仅Host颜色合成消费（1B；已核） | 0x39 bool shared【权威】颜色共享／传播标志；由Host处理关联实体的颜色结果，Client不重演传播（1B；已核） | 0x3A bytes[2] unknown_3A_3B【未知】SDK 类型大小尾端尚未解释的两个字节（2B；未知） |
|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：原颜色参数（预留）（body，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `priority`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `color`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `duration`：ColorParams.zhl +0x30；官方 LuaColorParams.cpp 的 GetDuration/SetDuration 直接读取写入 _duration1；第3步核对J273颜色更新循环，递减位置为+34，+30仅作比例分母；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `lifespan`：ColorParams.zhl +0x34；官方 LuaColorParams.cpp 的 GetLifespan/SetLifespan 直接读取写入 _duration2；J273 Entity::Update VA0x6AA122读+34、VA0x6AA12A dec、VA0x6AA12E写，VA0x6AA13B检查0并删除；主审已复读；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `fadeout`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `shared`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `unknown_3A_3B`：ColorParams.zhl 声明到 +0x39；没有证据可把 +0x3A..+0x3B 认作填充
