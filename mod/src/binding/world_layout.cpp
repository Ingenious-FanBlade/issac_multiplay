// tx-04-08 world_record 迁移：目标版本布局与 world 观察清单的访问层实现。
//
// 本单元只读取生成表与运行时 Hook 注册状态；不访问游戏内存、不调用原生
// 函数、不打开文件。数据来源与证据见 mod/binding/world_layout.json 与
// mod/binding/hooks.json，生成入口是 mod/tools/gen_binding.py。
//
// 编译期校验：生成的字段表必须与 min-read-v1 的 constexpr 声明逐项一致，
// 类别/来源/观察入口必须完整，目标版本常量之间必须满足已核对的几何关系
// （例如 448 个 Grid 指针之后紧接 8 个 door 指针）。任何一项不符都是编译
// 错误，而不是等到采集时才发现读错偏移。

#include "binding/world_layout.h"

#include <cstddef>
#include <cstring>
#include <iterator>

#include "hook_capacity.h"
#include "runtime.h"

namespace cs::world::layout {
namespace {

// ---- 生成数据（namespace 内，仅本单元可见） ------------------------------
#include "world_layout.inc"

// 字段表与 profile 声明逐项一致：ID、宽度、编码、名称。
constexpr bool SameText(const char *left, const char *right) noexcept {
  if (!left || !right)
    return false;
  while (*left && *right) {
    if (*left++ != *right++)
      return false;
  }
  return *left == *right;
}

// 表长必须相等：用 static_assert 表达，避免在 constexpr 函数里写常量 if
// （/W4 的 C4127），也让尺寸不符在编译期直接定位到这一行。
static_assert(std::size(kLayoutFields) == kProfileFieldTable.size(),
              "world_layout 字段表条数必须等于 min-read-v1 的 78 项");
static_assert(std::size(kManifestFields) == std::size(kLayoutFields),
              "manifest 字段表与完整字段表条数必须一致");
static_assert(std::size(kManifestSources) == std::size(kLayoutSources),
              "manifest 来源表与完整来源表条数必须一致");
static_assert(std::size(kLayoutClasses) == std::size_t(RttiClass::Count),
              "必须登记 10 个 RTTI 类");
static_assert(std::size(kLayoutSources) == kSourceCount,
              "必须登记 15 个来源");
static_assert(std::size(kLayoutObservations) == kObservationCount,
              "必须登记 19 个 world 观察入口");

constexpr bool FieldsMatchProfile() noexcept {
  for (std::size_t i = 0; i < std::size(kLayoutFields); ++i) {
    const auto &spec = kLayoutFields[i];
    const auto &decl = kProfileFieldTable[i];
    if (spec.id != decl.id || spec.width != decl.width ||
        spec.encoding != decl.encoding || !SameText(spec.name, decl.name))
      return false;
  }
  return true;
}

// 三个 manifest 形状必须与完整表一一对应（顺序、ID、偏移、容量）。
constexpr bool ManifestShapesMatch() noexcept {
  for (std::size_t i = 0; i < std::size(kLayoutFields); ++i)
    if (kManifestFields[i].id != kLayoutFields[i].id ||
        kManifestFields[i].offset != kLayoutFields[i].offset)
      return false;
  for (std::size_t i = 0; i < std::size(kLayoutSources); ++i)
    if (kManifestSources[i].source != kLayoutSources[i].source ||
        kManifestSources[i].capacity != kLayoutSources[i].count)
      return false;
  std::size_t schema_classes = 0;
  for (const auto &declaration : kLayoutClasses)
    if (declaration.schema)
      ++schema_classes;
  return std::size(kManifestClasses) == schema_classes;
}

// 表完整性：类别/来源/观察入口按固定顺序登记，且每类都有基类记录。
constexpr bool TablesComplete() noexcept {
  for (std::size_t i = 0; i < std::size(kLayoutClasses); ++i)
    if (std::size_t(kLayoutClasses[i].rtti) != i ||
        kLayoutClasses[i].base_count == 0 || !kLayoutClasses[i].bases)
      return false;
  for (std::size_t i = 0; i < std::size(kLayoutSources); ++i)
    if (kLayoutSources[i].source != i)
      return false;
  for (std::size_t i = 0; i < std::size(kLayoutObservations); ++i)
    if (std::size_t(kLayoutObservations[i].observation) != i)
      return false;
  return true;
}

// 目标版本常量的几何关系：这些数字在 ST3/ST4 里是逐条指令核对过的，
// 生成清单不得各自漂移。写成单一逻辑表达式，避免 constexpr 里的常量 if。
constexpr bool ConstantsConsistent() noexcept {
  const auto &c = kLayoutConstants;
  // 448 个 Grid 指针紧接 8 个 door 指针，再紧接 8 个 door→grid index；
  // Room 构造从 +0x24 起清零 0x720 字节正好覆盖两段槽表。
  const bool grid_tables =
      c.room_grid_slot_count == 448 && c.room_grid_max_index == 447 &&
      c.room_grid_slot_offset + c.room_grid_slot_count * 4 ==
          c.room_door_slot_offset &&
      c.room_door_slot_offset + c.room_door_slot_count * 4 ==
          c.room_door_grid_map_offset &&
      c.room_door_slot_count == 8 &&
      c.room_constructor_clear_offset == c.room_grid_slot_offset &&
      c.room_constructor_clear_bytes ==
          c.room_grid_slot_count * 4 + c.room_door_slot_count * 4;
  // 八个 EL descriptor 从 EntityList+0x20 起按 0x10 步进。
  const bool entity_lists =
      c.room_entity_list_first_el_offset == c.room_entity_list_offset + 0x20 &&
      c.room_entity_list_stride == 0x10 && c.room_entity_list_count == 8;
  // MSVC x86 RTTI 结构尺寸与两个 descriptor 尺寸。
  const bool rtti = c.rtti_complete_object_locator_size == 0x14 &&
                    c.rtti_class_hierarchy_descriptor_size == 0x10 &&
                    c.rtti_base_class_descriptor_size == 0x18;
  const bool descriptors =
      c.entity_list_descriptor_size == 0x10 && c.roster_vector_size == 0x0C;
  return grid_tables && entity_lists && rtti && descriptors;
}

static_assert(FieldsMatchProfile(),
              "world_layout 字段表必须与 min-read-v1 声明逐项一致");
static_assert(ManifestShapesMatch(), "manifest 形状必须与完整表一一对应");
static_assert(TablesComplete(), "类别/来源/观察入口表必须完整且顺序固定");
static_assert(ConstantsConsistent(),
              "目标版本常量之间的几何关系必须与静态确认一致");

constexpr const char *kFieldStatusNames[] = {
    "InitConfirmed", "InitDefault", "InitUnknown", "InitAssumed",
    "LuaBinding", "Derived"};
constexpr const char *kFieldBaseNames[] = {"Entity", "Grid", "Game", "Derived"};
constexpr const char *kSourceKindNames[] = {"EntityList", "Roster",
                                            "FixedArray", "Relation",
                                            "Reserved"};
constexpr const char *kRttiClassNames[] = {
    "Entity", "Player", "NPC", "Tear", "Projectile",
    "Pickup", "Familiar", "Effect", "Grid", "Door"};

} // namespace

const char *FieldStatusName(FieldStatus value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kFieldStatusNames) ? kFieldStatusNames[index]
                                              : "UNKNOWN_FIELD_STATUS";
}
const char *FieldBaseName(FieldBase value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kFieldBaseNames) ? kFieldBaseNames[index]
                                            : "UNKNOWN_FIELD_BASE";
}
const char *SourceKindName(SourceKind value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kSourceKindNames) ? kSourceKindNames[index]
                                             : "UNKNOWN_SOURCE_KIND";
}
const char *RttiClassName(RttiClass value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kRttiClassNames) ? kRttiClassNames[index]
                                            : "UNKNOWN_RTTI_CLASS";
}

const Constants &LayoutConstants() noexcept { return kLayoutConstants; }

const FieldSpec *Fields(std::size_t &count) noexcept {
  count = std::size(kLayoutFields);
  return kLayoutFields;
}
const SourceSpec *Sources(std::size_t &count) noexcept {
  count = std::size(kLayoutSources);
  return kLayoutSources;
}
const ClassSpec *Classes(std::size_t &count) noexcept {
  count = std::size(kLayoutClasses);
  return kLayoutClasses;
}
const ObservationSpec *Observations(std::size_t &count) noexcept {
  count = std::size(kLayoutObservations);
  return kLayoutObservations;
}
const char *const *Assumptions(std::size_t &count) noexcept {
  count = std::size(kLayoutAssumptions);
  return kLayoutAssumptions;
}
const char *const *Unknowns(std::size_t &count) noexcept {
  count = std::size(kLayoutUnknowns);
  return kLayoutUnknowns;
}

const FieldSpec *FindField(std::uint16_t id) noexcept {
  for (const auto &field : kLayoutFields)
    if (field.id == id)
      return &field;
  return nullptr;
}
const SourceSpec *FindSource(std::uint32_t source) noexcept {
  // 来源 ID 连续登记，直接索引；越界不猜测。
  if (source >= std::size(kLayoutSources) ||
      kLayoutSources[source].source != source)
    return nullptr;
  return &kLayoutSources[source];
}
const ClassSpec *FindClass(Class kind) noexcept {
  for (const auto &declaration : kLayoutClasses)
    if (declaration.schema && declaration.schema_class == kind)
      return &declaration;
  return nullptr;
}
const ObservationSpec *FindObservation(Observation observation) noexcept {
  const auto index = std::size_t(observation);
  if (index >= std::size(kLayoutObservations) ||
      kLayoutObservations[index].observation != observation)
    return nullptr;
  return &kLayoutObservations[index];
}

const LayoutField *ManifestFields(std::size_t &count) noexcept {
  count = std::size(kManifestFields);
  return kManifestFields;
}
const LayoutSource *ManifestSources(std::size_t &count) noexcept {
  count = std::size(kManifestSources);
  return kManifestSources;
}
const LayoutClass *ManifestClasses(std::size_t &count) noexcept {
  count = std::size(kManifestClasses);
  return kManifestClasses;
}

const char *HookManifestReference() noexcept { return kHookManifestReference; }
std::size_t HookManifestRows() noexcept { return cs::kHookManifestRows; }
std::size_t HookManifestCapacity() noexcept { return cs::kHookPendingCapacity; }

std::size_t ObservationMask(bool *mask, std::size_t capacity) noexcept {
  if (!mask)
    return 0;
  const auto limit = capacity < kObservationCount ? capacity : kObservationCount;
  for (std::size_t i = 0; i < limit; ++i)
    mask[i] = false;
  // 只读已注册状态：PrepareBindings/RegisterHooks 之后该状态不再变化，
  // 这里不获取任何锁，也不修改 Hook 记录。
  std::size_t hook_count = 0;
  auto hooks = cs::Hooks(hook_count);
  for (std::size_t i = 0; i < hook_count; ++i) {
    const auto &hook = *hooks[i];
    if (!hook.world || !hook.registered)
      continue;
    const auto index = std::size_t(hook.observation);
    if (index < limit)
      mask[index] = true;
  }
  return limit;
}

bool Validate(const char **reason) noexcept {
  // 复用编译期已通过检查的 constexpr 函数：正常构建里不可能失败，运行时
  // 仍显式复核一次，避免手动改动生成文件后静默使用错误布局。
  if (!FieldsMatchProfile()) {
    if (reason)
      *reason = "layout_fields_mismatch_profile";
    return false;
  }
  if (!ManifestShapesMatch()) {
    if (reason)
      *reason = "layout_manifest_shape_mismatch";
    return false;
  }
  if (!TablesComplete()) {
    if (reason)
      *reason = "layout_tables_incomplete";
    return false;
  }
  if (!ConstantsConsistent()) {
    if (reason)
      *reason = "layout_constants_inconsistent";
    return false;
  }
  if (reason)
    *reason = "";
  return true;
}

} // namespace cs::world::layout
