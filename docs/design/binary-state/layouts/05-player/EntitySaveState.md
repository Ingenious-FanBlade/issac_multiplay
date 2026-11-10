# EntitySaveState

由唯一布局源生成。布局编号 176；布局版本 57；原生大小 0x78；未知 74B；完整：否。

移动箱两处vector的真正元素。int/float/boolStorage虽有类型仍为UNKNOWN，不能误用更小的FriendBall EntityDesc替代。保存／离房容器的外部元素不计入根2EA0固定字节总数。

原生字节归属：

| 0x0 i32[3] type_variant_subtype【权威】保存对象身份；恢复时由Host使用（12B；已核） | 0xC u32/i32[5] storage_1_5【未知】SDK仅叫intStorage，类型／偏移已声明，具体用途未核（20B；未知） | 0x20 i16 grid_spawn_idx【权威】保存生成位置索引，ESSM可复用（2B；已核） | 0x22 bool[2] bool_storage【未知】SDK只声明boolStorage1/2，尚无用途（2B；未知） | 0x24 u32 storage_6【未知】SDK仅intStorage6，不能当已知普通Host字段（4B；未知） | 0x28 Vector target_position【权威】保存目标位置；不用于Client重放恢复（8B；已核） | 0x30 u32 storage_7【未知】SDK声明ESSM可复用的intStorage7，未核具体用途（4B；未知） | 0x34 u32[2] init_drop_seeds【权威】保存RNG种子（8B；已核） | 0x3C i32[2] spawner_type_variant【权威】Host恢复生产来源描述（8B；已核） | 0x44 bytes[12] unknown_44_4f【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（12B；未知） | 0x50 f32[2] float_storage【未知】只有类型与偏移，具体用途未核（8B；未知） | 0x58 bytes[4] unknown_58_5b【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（4B；未知） | 0x5C u32 storage_8【未知】SDK intStorage8语义未核（4B；未知） | 0x60 i8 byte_storage【未知】SDK byteStorage语义未核（1B；未知） | 0x61 bytes[11] unknown_61_6b【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（11B；未知） | 0x6C EntitySaveState* nested_save[0].save_state【本地】保存体可递归强引用；不映射线上关系：本地可递归保存体所有权，不是C/S实体StateRef（4B；SDK声明） | 0x70 KAGE_ReferenceCount* nested_save[0].counter【本地】保存体可递归强引用；不映射线上关系：本地控制块／引用计数，禁止下行（4B；SDK声明） | 0x74 bytes[4] unknown_74_77【未知】尾部尚未解释；不以对象大小或初始化零值判填充（4B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `type_variant_subtype`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `storage_1_5`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `grid_spawn_idx`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `bool_storage`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `storage_6`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `target_position`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `storage_7`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `init_drop_seeds`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `spawner_type_variant`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `unknown_44_4f`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `float_storage`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `unknown_58_5b`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `storage_8`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `byte_storage`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `unknown_61_6b`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `nested_save[0].save_state`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
- `nested_save[0].counter`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SmartPointer.zhl
- `unknown_74_77`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl
