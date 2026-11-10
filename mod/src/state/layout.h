#pragma once
#include <cstdint>
#include <span>
#include <string_view>

namespace cs::state {
// 生成表只含相对偏移和纯值元数据，不含游戏对象地址或可跨线程使用的原生指针。
inline constexpr std::uint32_t kDynamicOffset = UINT32_MAX;
inline constexpr std::uint16_t kNullIndex = UINT16_MAX;
enum class Operation : std::uint8_t { Copy, Convert, Reference, Resource, Container, Nested, Choice };
// 伴随对象由具名 owner 在同一主线程窗口解析；不能对根对象直接加偏移读取。
enum class FieldSource : std::uint8_t { Object, Companion, Synthetic };
enum class Category : std::uint8_t { State, Convert, Reference, Resource, Container, Local, Derived, Authority, Padding, Unknown };
enum class Validation : std::uint8_t { None, Finite, Boolean, Range, Enum };
struct Layout {
  std::uint16_t id, parent;
  const char *name;
  std::uint32_t native_size, unknown_bytes;
  std::uint8_t block_count;
  bool complete, base_prefix, protocol; // protocol记录没有原生对象或可加偏移的地址。
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
  // storage=shared-owner时native_offset指向8B holder；仅嵌套投影0带u8 present。
  // payload须经具名owner读取/重建；其余投影使用同目标版本的存在性，不能直接加偏移。
  std::uint16_t nested_layout;
  std::uint8_t nested_block; // 255 表示整个嵌套状态，其余值只投影该子块。
  // aggregate的native_stride=0：先经官方成员owner聚合成纯值元素，不能按结果宽度遍历原map。
  std::uint32_t limit, native_stride;
  FieldSource source;
  bool shape; // 该字段变化必须经过成员形态/结构事务。
  const char *read_requirement; // 采集owner必须提供的读取资格，不由生成表虚构见证。
  std::uint16_t fixed_count; // 非零时没有线上u16 count，必须恰好处理全部固定槽。
  std::uint32_t value_mask; // 换算位投影；Host先mask再比较，Client只更新已核位。
  std::uint8_t count_bytes; // 容器计数宽度：0为固定槽／非容器，1为u8，2为默认u16。
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
// 同一slot是同一个原生容器的各投影；不同slot才参与合计上限。
struct CountConstraint { std::uint16_t layout, group, slot, field, limit; };
struct Variant { std::uint16_t base, arm; std::uint32_t selector_offset; std::int32_t value; };
// 嵌套布局可从父资源/容器位置获得语义输入；上下文不冒充为对象内的字节或重复发送字段。
struct ContextField { std::uint16_t layout; const char *name, *type, *semantics, *evidence; };
// 条件只依赖同块先行字段；谓词在注册时绑定，不能由报文提供函数或偏移。
struct FieldCondition { std::uint16_t layout, field, selector; const char *predicate; };
// slots-mask的每个置位槽带一个u16 view_layout，只接受该字段的已登记布局集合。
struct LayoutChoice { std::uint16_t layout, field, choice; };
// 旧目录按来源行定位，保留重载、重复ID和null/空签名的差异；不作为运行查询字典。
struct CatalogRow {
  std::uint16_t layout;
  std::uint32_t ordinal;
  const char *catalog, *public_name, *owner, *parameters, *returns;
  bool parameters_null, returns_null;
  const char *disposition, *fields, *route, *reason;
};
// 14条native成员、77条旧宏及6条结构各自保留来源行，不冒充公开getter签名。
struct SupplementaryMember {
  std::uint16_t layout;
  std::uint32_t ordinal;
  const char *source, *public_name, *disposition, *fields, *route, *detail, *reason;
};
// 同一成员在另一目录的来源行链接；目标仍是保留原签名的正式逐行表。
struct CatalogAlias {
  std::uint16_t layout;
  std::uint32_t ordinal, target_ordinal;
  const char *catalog, *target_catalog;
};
// 编码总预算与单字段count上限分别约束；超body预算只回ordinary，不截断状态。
struct LayoutBudget { std::uint16_t layout; std::uint32_t record_bytes, body_bytes; };
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
std::span<const CountConstraint> CountConstraints() noexcept;
// counts按布局字段索引排列；调用方在完整同版本状态中取数，不能用0补缺失投影。
// Host复制元素前及Client写入前分别调用；本函数不分配内存，也不读取游戏对象。
bool ValidContainerCountGroups(std::uint16_t layout, std::span<const std::uint16_t> counts) noexcept;
std::span<const Variant> Variants() noexcept;
std::span<const ContextField> ContextFields() noexcept;
std::span<const FieldCondition> FieldConditions() noexcept;
std::span<const LayoutChoice> LayoutChoices() noexcept;
std::span<const CatalogRow> CatalogRows() noexcept;
std::span<const SupplementaryMember> SupplementaryMembers() noexcept;
std::span<const CatalogAlias> CatalogAliases() noexcept;
std::span<const LayoutBudget> LayoutBudgets() noexcept;
const Layout *FindLayout(std::uint16_t id) noexcept;
// 接收布局号和布局内索引，只索引可信生成表；网络不能提供任意原生偏移或处理器地址。
const Field *FindField(std::uint16_t layout, std::uint16_t index) noexcept;
const ClientField *FindClientField(std::uint16_t layout, std::uint16_t index) noexcept;
}
