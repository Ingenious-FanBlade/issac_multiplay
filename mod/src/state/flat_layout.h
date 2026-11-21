#pragma once
#include "state/layout.h"
#include <array>

namespace cs::state {
enum class FlatStream : std::uint8_t { World, Body };
enum class FlatSetMode : std::uint8_t { Bitmap=0, Deltas=1, All=2, Legacy=255 };
inline constexpr std::size_t kFlatRecordHeaderBytes=9;
// 平铺记录合同的起始布局版本。历史：85 为首个平铺试验合同；86—89 回到 84 的块编码
// （86 属性栏输入，87 预测对账字段与寻路阻挡，88 道具历史 _history_items，89 predict_gate）；
// 2026-11-21 起自 90 正式切回平铺记录，之后的版本一律沿用平铺合同。
inline constexpr std::uint16_t kFlatRecordContractVersion=90;
// 实体、Player 两条流与 Grid 是否按平铺记录编码；全局根与控制组 196 仍走块编码。
constexpr bool UsesFlatRecordContract(std::uint16_t version) noexcept {
  return version>=kFlatRecordContractVersion;
}
enum class FlatPatchKind : std::uint8_t {
  Copy, Zero, Reference, Resource, Container, Owner, Convert
};
enum class FlatHeader : std::uint8_t {
  None, Vector, Deque, String, CString, Tree, Unordered, Array,
  LayerArray, InlinePrefix, Aggregate, Owner, Weapon
};
enum class FlatPredicate : std::uint8_t { Equal, NotEqual, AnyBits, OneOf, ItemConfig };
enum class FlatValueRule : std::uint8_t { None, NonNull, NullableWater, EntityCounter, Countdown, DisplayUtf8 };

// 上下界和枚举采用真实标量位模式，避免较宽整数经过 double 丢失精度。
struct FlatValidation {
  Validation kind;
  ScalarKind scalar;
  std::uint64_t minimum, maximum;
  std::span<const std::uint64_t> enum_values;
};

// 主区按 32 字节向上取整。字节表中的 kNullIndex 标记清零填充。
struct FlatField {
  const char *name;
  std::uint32_t offset, size;
  std::uint16_t group, components, tail; // 无附加段的字段使用 kNullIndex。
  ScalarKind scalar;
  FlatStream stream;
  FlatPatchKind kind;
  bool shape;
  FlatValueRule value_rule;
  FlatValidation validation;
};

// Host 一次复制已核连续区间，再处理其中需要规范化的槽。客机写段只含原样状态叶。
struct FlatCopySegment {
  std::uint32_t native_offset, mirror_offset, size;
  const char *read_requirement;
  FlatStream stream;
};
struct FlatWriteSegment {
  std::uint32_t mirror_offset, native_offset, size;
  FlatStream stream;
};

// 条件依赖同一镜像内的字段。命中失败时清零整个槽并跳过该槽的原生写入。
struct FlatCondition {
  std::uint16_t selector;
  FlatPredicate predicate;
  std::uint64_t operand, mask;
};

// 具名内嵌来源可能须通过 owner 取得。scope 用于在已解析承载与名义相对偏移间定位。
struct FlatScope {
  std::uint32_t native_offset, native_size;
  std::uint16_t parent, layout;
  const char *host, *client, *read_requirement;
  std::span<const std::uint16_t> fields;
};

// 槽表按主区偏移排序。native_offset 与 owner_offset 是相对根的名义位置，
// count_offset 是相对 owner 的原生计数位置。非原生来源使用 kDynamicOffset。
struct FlatPatch {
  std::uint32_t mirror_offset, size, native_offset, native_size, owner_offset, count_offset;
  std::uint32_t stride, limit, max_bytes, value_mask;
  std::uint16_t field, scope, child_layout, owner_layout;
  FlatPatchKind kind;
  FlatHeader header;
  FieldSource source;
  std::uint8_t resource_table; // 非资源槽为 255；表号不上线。
  bool seeded; // 原生槽已随复制段进入主区，补丁须覆盖其全部字节。
  bool frame_relative; // 到期帧依赖当前帧号，原生应用每帧重复换算。
  const char *host, *client, *target, *count_source, *read_requirement;
  std::span<const FlatCondition> conditions;
  std::span<const std::uint16_t> choices;
  std::uint16_t recipe_field; // 同源原生读取 recipe 的字段索引。
};

// groups 的下标也是字段的组号。同组可以包含来自两条流的完整镜像值。
struct FlatWriteGroup {
  std::span<const std::uint16_t> fields;
  std::span<const FlatWriteSegment> segments;
};
struct FlatCountGroup {
  std::span<const std::uint16_t> fields;
  std::uint32_t limit;
};
// 当 selector 等于 empty 时，被约束的字段在镜像中须为规范零值。
struct FlatZeroConstraint {
  std::uint16_t selector;
  std::uint32_t empty;
  std::span<const std::uint16_t> fields;
};

// 子描述以 layout id 访问，与根共用一个表。字段下标就是本布局的 u16 序号。
struct FlatLayout {
  std::uint16_t id;
  std::uint32_t mirror_size;
  std::span<const FlatField> fields;
  std::span<const std::uint16_t> byte_fields;
  std::span<const FlatCopySegment> copies;
  std::span<const FlatPatch> patches;
  std::span<const FlatScope> scopes;
  std::span<const FlatWriteGroup> write_groups;
  std::span<const FlatCountGroup> count_groups;
  std::span<const FlatZeroConstraint> zero_constraints;
  std::span<const std::uint16_t> tail_fields; // 从附加段索引反查字段序号。
  std::array<std::span<const std::uint32_t>,2> stream_fields;
};

const FlatLayout *FindFlatLayout(std::uint16_t id) noexcept;
// 正式握手仍由 LayoutVersion 决定；只为已接线根选择新记录格式。
const FlatLayout *FlatRecordLayout(std::uint16_t version,std::uint16_t id) noexcept;
}
