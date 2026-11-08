#pragma once
#include <cstdint>
#include <span>
#include <string_view>

namespace cs::state {
// 生成表只含相对偏移和纯值元数据，不含游戏对象地址或可跨线程使用的原生指针。
inline constexpr std::uint32_t kDynamicOffset = UINT32_MAX;
inline constexpr std::uint16_t kNullIndex = UINT16_MAX;
enum class Operation : std::uint8_t { Copy, Convert, Reference, Resource, Container, Nested };
// 伴随对象由具名 owner 在同一主线程窗口解析；不能对根对象直接加偏移读取。
enum class FieldSource : std::uint8_t { Object, Companion };
enum class Category : std::uint8_t { State, Convert, Reference, Resource, Container, Local, Derived, Authority, Padding, Unknown };
enum class Validation : std::uint8_t { None, Finite, Boolean, Range, Enum };
struct Layout {
  std::uint16_t id, parent;
  const char *name;
  std::uint32_t native_size, unknown_bytes;
  std::uint8_t block_count;
  bool complete, base_prefix;
  const char *rtti;
  std::uint32_t rtti_rva;
  std::uint32_t first_field, field_count, first_block, first_copy, copy_count;
};
// 字段索引在单个布局内稳定；变长项之后的偏移为 kDynamicOffset，必须顺序解码。
struct Field {
  std::uint16_t layout, index;
  std::uint8_t block;
  std::uint32_t native_offset, native_size, wire_offset, min_size, max_size;
  Operation operation;
  const char *name, *type, *handler, *target, *storage, *count_source;
  std::uint16_t nested_layout;
  std::uint8_t nested_block; // 255 表示整个嵌套状态，其余值只投影该子块。
  std::uint32_t limit, native_stride;
  FieldSource source;
  bool shape; // 该字段变化必须经过成员形态/结构事务。
  const char *read_requirement; // 采集owner必须提供的读取资格，不由生成表虚构见证。
};
struct CopyRange {
  std::uint16_t layout;
  std::uint8_t block;
  std::uint32_t native_offset, wire_offset, size;
  const char *read_requirement;
};
struct Block {
  std::uint16_t layout;
  std::uint8_t id;
  bool body;
  bool player_body; // Entity 公共块在 Player 布局中切换到逐身体序列。
  const char *name;
  std::uint32_t min_size, max_size;
};
// 校验表按字段索引关联；数组按元素校验，引用/资源/嵌套另由对应处理器校验。
struct Validator {
  std::uint16_t layout, field;
  Validation kind;
  const char *scalar;
  double minimum, maximum;
  const char *values;
};
struct NativeRange {
  std::uint16_t layout;
  std::uint32_t offset, size;
  Category category;
  const char *name, *type, *semantics, *evidence, *status;
};
struct Member {
  std::uint16_t layout;
  const char *public_name, *disposition, *fields, *reason;
};
// 具名处理器在主线程接线时绑定一次，逐字段写入使用 layout/field 数字索引。
struct ClientField { std::uint16_t layout, field; Operation operation; const char *handler; };
struct Variant { std::uint16_t base, arm; std::uint32_t selector_offset; std::int32_t value; };
// 嵌套布局可从父资源/容器位置获得语义输入；上下文不冒充为对象内的字节或重复发送字段。
struct ContextField { std::uint16_t layout; const char *name, *type, *semantics, *evidence; };
// 这组只读视图由同一次布局生成提供给后续采集、解码、写入和 E2E 对照。
// 布局版本与第 9 步帧头 layout_version 同为 u16，两端按整数直接比较。
std::uint16_t LayoutVersion() noexcept;
std::span<const Layout> Layouts() noexcept;
std::span<const Field> Fields() noexcept;
std::span<const CopyRange> CopyRanges() noexcept;
std::span<const Block> Blocks() noexcept;
std::span<const Validator> Validators() noexcept;
std::span<const NativeRange> NativeRanges() noexcept;
std::span<const Member> Members() noexcept;
std::span<const ClientField> ClientFields() noexcept;
std::span<const Variant> Variants() noexcept;
std::span<const ContextField> ContextFields() noexcept;
const Layout *FindLayout(std::uint16_t id) noexcept;
// 接收布局号和布局内索引，只索引可信生成表；网络不能提供任意原生偏移或处理器地址。
const Field *FindField(std::uint16_t layout, std::uint16_t index) noexcept;
const ClientField *FindClientField(std::uint16_t layout, std::uint16_t index) noexcept;
}
