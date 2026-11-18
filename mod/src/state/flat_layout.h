#pragma once
#include "state/layout.h"

namespace cs::state {
enum class FlatStream : std::uint8_t { World, Body };
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
  std::uint16_t group, components;
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
};
struct FlatWriteSegment { std::uint32_t mirror_offset, native_offset, size; };

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
};

const FlatLayout *FindFlatLayout(std::uint16_t id) noexcept;
}
