# EntityEffect_TearFlags

由唯一布局源生成。布局编号 102；布局版本 8；原生大小 0x4C0；未知 1119B；完整：否。

第1步使用第4步已核字段演示继承、块和变体；完整6臂、附属owner及公开Effect目录由第4步接管。未知区不复制，所有臂均不宣称完整。 原生范围直接复核于J273 Effect池VA0x6EEE60/RVA0x2EEE60：VA0x6EEE9B传步长0x4C0、VA0x6EEE96传容量0x400；ctor VA0x5CCEAF写Effect虚表，TD VA0xBD1270/RVA0x7D1270，主审已复读。

变体选择字段 `variant`，取值：[54, 113, 126, 167]。

原生字节归属：

| 0x0 u32 vtable【本地】原生虚表，仅当前进程使用（4B；已核） | 0x4 bytes[40] pending_common_prefix【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（40B；未知） | 0x2C i32 variant【状态】形态选择字段；变化必须更新成员形态（4B；已核） | 0x30 bytes[992] pending_common_suffix【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（992B；未知） | 0x410 f32 height【状态】离地高度，粒子作纵向速度（4B；已核） | 0x414 f32 falling_speed【状态】下落速度（4B；已核） | 0x418 f32 falling_acceleration【状态】下落加速度（4B；已核） | 0x41C bytes[4] gap_41c【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（4B；未知） | 0x420 u32 variant_data.ll【状态】效果携带的眼泪效果位，按128位原样传输：低 64 位低 DWORD（4B；已核） | 0x424 u32 variant_data.lh【状态】效果携带的眼泪效果位，按128位原样传输：低 64 位高 DWORD（4B；已核） | 0x428 u32 variant_data.hl【状态】效果携带的眼泪效果位，按128位原样传输：高 64 位低 DWORD（4B；已核） | 0x42C u32 variant_data.hh【状态】效果携带的眼泪效果位，按128位原样传输：高 64 位高 DWORD（4B；已核） | 0x430 bytes[16] gap_430【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（16B；未知） | 0x440 bytes[16] gap_440【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（16B；未知） | 0x450 i32 state【状态】效果自定状态（4B；已核） | 0x454 f32 min_radius【状态】冲击波最小半径（4B；已核） | 0x458 f32 max_radius【状态】冲击波最大半径（4B；已核） | 0x45C i32 expire_frame【换算】Timeout换算为Host到期帧，Client由展示时钟还原（4B；已核） | 0x460 i32 lifespan【状态】效果寿命（4B；已核） | 0x464 bool following【状态】是否跟随父实体（1B；已核） | 0x465 bytes[3] gap_465【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（3B；未知） | 0x468 f32 parent_offset.x【状态】相对父实体的偏移：X 分量（4B；已核） | 0x46C f32 parent_offset.y【状态】相对父实体的偏移：Y 分量（4B；已核） | 0x470 i32 damage_source【状态】伤害来源实体类型（4B；已核） | 0x474 f32 rotation【状态】旋转（4B；已核） | 0x478 f32 scale【状态】缩放（4B；已核） | 0x47C bytes[44] gap_47c【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（44B；未知） | 0x4A8 std::vector authority_vector【权威】仅Authority更新消费的私有容器，头与内容均不传输（12B；已核） | 0x4B4 shared_owner extra_data【权威】基础臂仅Authority使用；第4步分别研究拥有型表现臂（8B；已核） | 0x4BC bytes[4] gap_4bc【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（4B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 73—73B，不含第 9 步记录头）：

块 0：E0（第3步补齐）（world，4—4B）：

| +0x0 i32 variant：形态选择字段；变化必须更新成员形态 |
|---|

块 1：E1（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 2：E2（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 3：E3（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 4：E4（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 5：E5（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 6：E6（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 7：E7（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 8：E8（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 9：E9（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 10：FX0 计时与下落（world，20—20B）：

| +0x0 f32 height：离地高度，粒子作纵向速度 | +0x4 f32 falling_speed：下落速度 | +0x8 f32 falling_acceleration：下落加速度 | +0xC i32 state：效果自定状态 | +0x10 i32 expire_frame：Timeout换算为Host到期帧，Client由展示时钟还原 |
|---|---|---|---|---|

块 11：FX1 参数（world，33—33B）：

| +0x0 f32 min_radius：冲击波最小半径 | +0x4 f32 max_radius：冲击波最大半径 | +0x8 i32 lifespan：效果寿命 | +0xC u8 following：是否跟随父实体 | +0xD Vector parent_offset：相对父实体的偏移 | +0x15 i32 damage_source：伤害来源实体类型 | +0x19 f32 rotation：旋转 | +0x1D f32 scale：缩放 |
|---|---|---|---|---|---|---|---|

块 12：FX2 变体数据（world，16—16B）：

| +0x0 BitSet128 variant_data：效果携带的眼泪效果位，按128位原样传输 |
|---|

证据：

- `vtable`：mod/binding/world_layout.json:classes.Entity；docs/research/tx-04-08-st5-static-confirmation.md
- `pending_common_prefix`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `variant`：mod/binding/world_layout.json:fields.variant；J273 VA 0x6A4A3B 写 +0x2C（imagebase 0x400000）
- `pending_common_suffix`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `height`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `falling_speed`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `falling_acceleration`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `gap_41c`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `variant_data.ll`：docs/design/binary-state/4-entity-classes.md:4.1.1；NativeEffect.TearFlags@0x420；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `variant_data.lh`：docs/design/binary-state/4-entity-classes.md:4.1.1；NativeEffect.TearFlags@0x420；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `variant_data.hl`：docs/design/binary-state/4-entity-classes.md:4.1.1；NativeEffect.TearFlags@0x420；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `variant_data.hh`：docs/design/binary-state/4-entity-classes.md:4.1.1；NativeEffect.TearFlags@0x420；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `gap_430`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `gap_440`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `state`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `min_radius`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `max_radius`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `expire_frame`：docs/design/binary-state/4-entity-classes.md:4.1.5；Effect Init VA0x5CD1E7
- `lifespan`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `following`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `gap_465`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `parent_offset.x`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `parent_offset.y`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `damage_source`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `rotation`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `scale`：docs/design/tx-04-19/entity-application.json:class_native；client_entity_class_fields.inc
- `gap_47c`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `authority_vector`：docs/design/binary-state/4-entity-classes.md:4.1.1；client_entity_class_model.cpp:178-179
- `extra_data`：docs/design/binary-state/4-entity-classes.md:4.1.2；client_entity_class_model.cpp:93-95
- `gap_4bc`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
