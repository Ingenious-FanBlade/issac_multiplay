#pragma once
// tx-04-08 world_record 迁移：目标版本只读布局与 world 观察清单的访问层。
//
// 数值全部来自 mod/binding/world_layout.json 与 mod/binding/hooks.json，
// 由 mod/tools/gen_binding.py 生成到构建目录的 world_layout.inc /
// world_observations.inc；本头只声明类型和访问器，不含具体偏移或地址。
// 迁移设计 §3.1 要求这些目标版本数据集中存放在 binding/world_layout，
// 不散落进复制逻辑：采集器只读取这里的表，不自己写常量。
//
// 本头依赖 world/model.h（C++20），因此只能被 C++20 编译单元包含；生成的
// SDK-facing wrapper 只需要 world/observe.h 与 world/observation.h。

#include <cstddef>
#include <cstdint>

#include "world/model.h"
#include "world/observation.h"

namespace cs::world::layout {

// ===========================================================================
// 1. 表的行类型
// ===========================================================================

// 字段布局的可信级别。与 ST3 的分层一致：位置确认不等于初始化已确认，
// 也不等于该对象在当期取得读取资格。
enum class FieldStatus : std::uint8_t {
  InitConfirmed = 0, // 位置与初始化写入均静态确认
  InitDefault = 1,   // 只见到构造/Init 默认值，创建 owner 的最终值未闭合
  InitUnknown = 2,   // 未找到明确初始化写入，不补默认零
  InitAssumed = 3,   // 位置确认，语义是历史用户接受的假设（通用 HP 等）
  LuaBinding = 4,    // SDK 未命名，语义由原生 Lua 注册/getter/setter 确认
  Derived = 5        // 由已确认值推导，不读对象内存
};
const char *FieldStatusName(FieldStatus value) noexcept;

// 字段读取的基址。Derived 字段没有对象偏移。
enum class FieldBase : std::uint8_t {
  Entity = 0,
  Grid = 1,
  Game = 2,
  Derived = 3
};
const char *FieldBaseName(FieldBase value) noexcept;

// 来源形状：八个 EL 视图、roster vector、固定数组、保留关系工作表、保留空号。
enum class SourceKind : std::uint8_t {
  EntityList = 0,
  Roster = 1,
  FixedArray = 2,
  Relation = 3,
  Reserved = 4
};
const char *SourceKindName(SourceKind value) noexcept;

// RTTI 分类：Entity 根/七类专用 Entity，以及已核定的 18 类 Grid 实现。
// 它们不是文件类别；所有 Grid 派生类共用 Grid 公共字段，不新增专用 schema。
enum class RttiClass : std::uint8_t {
  Entity = 0,
  Player = 1,
  NPC = 2,
  Tear = 3,
  Projectile = 4,
  Pickup = 5,
  Familiar = 6,
  Effect = 7,
  Grid = 8,
  Door = 9,
  Decoration = 10,
  Rock = 11,
  Pit = 12,
  Spikes = 13,
  Web = 14,
  Lock = 15,
  TNT = 16,
  Fire = 17,
  Poop = 18,
  Wall = 19,
  TrapDoor = 20,
  Stairs = 21,
  Gravity = 22,
  PressurePlate = 23,
  Statue = 24,
  Teleporter = 25,
  Count = 26
};
const char *RttiClassName(RttiClass value) noexcept;

// 单个字段的目标版本读取位置与证据。
struct FieldSpec final {
  std::uint16_t id;
  FieldBase base;
  std::uint32_t offset;   // 基址相对偏移；Derived 为 0
  std::uint8_t width;     // 字节宽度，必须与 min-read-v1 声明一致
  Encoding encoding;      // 必须与 min-read-v1 声明一致
  FieldStatus status;
  const char *name;       // profile 语义名，必须与 min-read-v1 声明一致
  const char *owner;      // Entity/Player/.../Grid/Game/Derived
  const char *sdk_member; // SDK 命名成员；空串表示 SDK 未命名
  const char *evidence;   // 目标版本写入/读取指令与出处
};

// 来源的可读取形状与容量。
struct SourceSpec final {
  std::uint32_t source;
  SourceKind kind;
  const char *base;       // room/game/player/none
  std::uint32_t offset;   // 相对 Room/Game/Player 的偏移
  std::uint32_t count;    // 固定容量；0 表示由 descriptor 动态决定
  std::uint32_t max_index;
  std::uint32_t index_map_offset; // door→grid index 表；0 表示无
  const char *evidence;
};

// RTTI 基类记录：PMD 与 attributes 来自目标 CHD/BCD。
struct BaseClassSpec final {
  std::uint32_t td_rva;
  const char *td_name;
  std::int32_t mdisp, pdisp, vdisp;
  std::uint32_t attributes;
  bool supported; // 本轮是否有已登记的读取布局
};

// 一个分类的目标 RTTI 与虚表槽（只读分类使用，不调用任何方法）。
struct ClassSpec final {
  RttiClass rtti;
  // 是否持有独立文件 schema；Grid 派生类只通过根类使用公共 Grid schema。
  bool schema;
  // 映射到的文件类别；schema=false 时为 Class::Unknown（不使用）。
  Class schema_class;
  // object_size=0 表示完整尺寸未知；公共读取前缀与分配请求在可读清单中分列保存。
  std::uint32_t vtable_rva, col_rva, td_rva, object_size;
  const char *td_name;
  std::uint32_t free_slot_rva; // 虚表 slot 0 删除包装，仅诊断
  std::uint32_t init_rva;      // 已登记的 Init 入口；0 表示无独立入口
  const char *init_source;
  const BaseClassSpec *bases;
  std::size_t base_count;
  const char *evidence;
};

// 一个 world 观察入口的清单行。
struct ObservationSpec final {
  Observation observation;
  const char *hook_id;
  const char *definition; // ZHL 定义名（官方或自有 IsaacCS::*）
  const char *completion; // 正常返回的含义限定
  const char *reach;      // 原生 body 到达证据等级
  std::uint32_t rva;
  int priority;
  bool own_definition;
  bool required;
};

// 目标版本常量。成员顺序即生成清单顺序；生成代码使用指定初始化器，
// 名称或顺序不一致会直接编译失败。
struct Constants final {
  std::uint32_t sprite_null_names_offset;
  std::uint32_t sprite_event_names_offset;
  // Player ctor逐成员构造的13份内嵌ANM；物理slot不猜测未知用途，不按文件合并。
  std::uint32_t player_inline_sprite_first_offset;
  std::uint32_t player_inline_sprite_stride;
  std::uint32_t player_inline_sprite_count;
  std::uint32_t player_sub_storage_offset;
  std::uint32_t game_player_borrow_first_offset;
  std::uint32_t game_player_borrow_count;

  std::uint32_t entity_text_td_rva;
  std::uint32_t entity_dummy_td_rva;
  std::uint32_t entity_text_counter_offset;
  std::uint32_t entity_text_duration_offset;
  std::uint32_t entity_text_buffer_offset;
  std::uint32_t entity_text_capacity;
  std::uint32_t entity_text_color_offset;
  std::uint32_t entity_text_variant_offset;
  std::uint32_t entity_text_writer_return_rva;
  std::uint32_t entity_laser_td_rva;
  std::uint32_t entity_laser_sample_mode_offset;
  std::uint32_t entity_laser_start_offset_mode_offset;

  std::uint32_t game_root_rva;
  std::uint32_t game_frame_offset;
  std::uint32_t game_room_offset;
  std::uint32_t game_room_index_offset;
  std::uint32_t game_dimension_offset;
  std::uint32_t game_object_size;
  std::uint32_t room_object_size;
  std::uint32_t player_manager_object_size;
  std::uint32_t game_player_manager_offset;
  std::uint32_t room_grid_width_offset;
  std::uint32_t room_grid_height_offset;
  std::uint32_t room_entity_list_offset;
  std::uint32_t room_entity_list_first_el_offset;
  std::uint32_t room_entity_list_stride;
  std::uint32_t room_entity_list_count;
  std::uint32_t room_grid_slot_offset;
  std::uint32_t room_grid_slot_count;
  std::uint32_t room_grid_max_index;
  std::uint32_t room_door_slot_offset;
  std::uint32_t room_door_slot_count;
  std::uint32_t room_door_grid_map_offset;
  std::uint32_t room_constructor_rva;
  std::uint32_t room_constructor_clear_offset;
  std::uint32_t room_constructor_clear_bytes;
  std::uint32_t player_manager_roster_offset;
  std::uint32_t player_manager_esau_offset;
  std::uint32_t esau_slot_count;
  std::uint32_t relation_node_budget;
  std::uint32_t entity_flags_offset;
  std::uint32_t entity_hp_offset;
  std::uint32_t entity_max_hp_offset;
  std::uint32_t entity_frame_stamp_offset;
  float grid_position_step;
  float grid_position_origin_x;
  float grid_position_origin_y;
  std::uint32_t grid_position_getter_rva;
  std::uint32_t grid_index_by_tile_rva;
  std::uint32_t rtti_complete_object_locator_size;
  std::uint32_t rtti_class_hierarchy_descriptor_size;
  std::uint32_t rtti_base_class_descriptor_size;
  std::uint32_t rtti_expected_signature;
  std::uint32_t rtti_expected_hierarchy_attributes;
  std::uint32_t rtti_expected_base_attributes;
  std::uint32_t entity_list_descriptor_size;
  std::uint32_t entity_list_descriptor_data_offset;
  std::uint32_t entity_list_descriptor_capacity_offset;
  std::uint32_t entity_list_descriptor_size_offset;
  std::uint32_t roster_vector_size;
  std::uint32_t roster_vector_begin_offset;
  std::uint32_t roster_vector_end_offset;
  std::uint32_t roster_vector_capacity_offset;
};

// ===========================================================================
// 2. 访问器
// ===========================================================================

const Constants &LayoutConstants() noexcept;

const FieldSpec *Fields(std::size_t &count) noexcept;
const SourceSpec *Sources(std::size_t &count) noexcept;
const ClassSpec *Classes(std::size_t &count) noexcept;
const ObservationSpec *Observations(std::size_t &count) noexcept;
const char *const *Assumptions(std::size_t &count) noexcept;
const char *const *Unknowns(std::size_t &count) noexcept;

// 按 ID / 编号查找；未登记返回 nullptr，不猜测最近值。
const FieldSpec *FindField(std::uint16_t id) noexcept;
const SourceSpec *FindSource(std::uint32_t source) noexcept;
const ClassSpec *FindClass(Class kind) noexcept;
const ObservationSpec *FindObservation(Observation observation) noexcept;

// 生成清单的 manifest 形状（writer 只序列化，不再二次解释）。
const LayoutField *ManifestFields(std::size_t &count) noexcept;
const LayoutSource *ManifestSources(std::size_t &count) noexcept;
const LayoutClass *ManifestClasses(std::size_t &count) noexcept;

// ===========================================================================
// 3. manifest 与运行状态
// ===========================================================================

// 清单引用摘要：写入 manifest.json 的 hook_manifest 字段。
const char *HookManifestReference() noexcept;
// 待注册表容量与实际行数（生成期固定；注册期另有运行时容量报告）。
std::size_t HookManifestRows() noexcept;
std::size_t HookManifestCapacity() noexcept;

// 本次构建登记的观察入口是否已在运行时成功注册。mask 长度必须不小于
// kObservationCount；返回写入的项数。未安装（例如 world 组未启用）为 false。
std::size_t ObservationMask(bool *mask, std::size_t capacity) noexcept;

// 内部一致性校验：生成的字段表必须与 min-read-v1 声明逐项一致、偏移必须
// 落在登记的对象尺寸内、来源编号必须完整、观察入口必须覆盖 19 项。
// 通过返回 true；失败时 reason 指向静态字符串。
bool Validate(const char **reason) noexcept;

} // namespace cs::world::layout
